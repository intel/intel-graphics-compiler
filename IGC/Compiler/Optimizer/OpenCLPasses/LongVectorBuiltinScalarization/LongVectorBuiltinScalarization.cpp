/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "LongVectorBuiltinScalarization.hpp"
#include "LongVectorMangling.hpp"

#include "Compiler/CodeGenPublic.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/Optimizer/OpenCLPasses/BfloatBuiltins/BfloatBuiltinsResolution.hpp"
#include "llvmWrapper/ADT/StringRef.h"
#include "llvmWrapper/IR/DerivedTypes.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/ADT/Twine.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>
#include "common/LLVMWarningsPop.hpp"

#ifdef BIF_LINK_BC
#include <BiFManager/BiFManagerHandler.hpp>
#endif // BIF_LINK_BC

using namespace llvm;
using namespace IGC;
using namespace IGC::LongVectorMangling;

char LongVectorBuiltinScalarizationLPM::ID = 0;

#define PASS_FLAG "igc-long-vector-builtin-scalarization"
#define PASS_DESC "Scalarization of builtin calls on SPV_EXT_long_vector vectors"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LongVectorBuiltinScalarizationLPM, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(LongVectorBuiltinScalarizationLPM, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

LongVectorBuiltinScalarizationLPM::LongVectorBuiltinScalarizationLPM() : ModulePass(ID) {
  initializeLongVectorBuiltinScalarizationLPMPass(*PassRegistry::getPassRegistry());
}

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses LongVectorBuiltinScalarizationNPM::run(Module &M, ModuleAnalysisManager &AM) {
  bool Changed = LongVectorBuiltinScalarization().run(M, AM.getResult<CodeGenContextAnalysis>(M).Ctx);
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16

namespace {

// BiF unrolls its vector variants up to 16 elements. Unrolling further inlines
// the scalar BiF body once per element: pow on 1024 floats took over 250 s.
// Wider vectors get a loop, which LoopUnroll still unrolls after BiF inlining
// if the body is small.
constexpr unsigned MaxUnrolledWidth = 16;

// BiF implements bfloat builtins on i16, and BfloatBuiltinsResolution renames
// the calls only after this pass.
// TODO: Look up the name directly once BiF implements all of these builtins on
// the bfloat type; then getResolvedName is no longer needed here.
bool isDefinedInBiF(Module &M, StringRef Name) {
#ifdef BIF_LINK_BC
  if (std::optional<std::string> I16Name = BfloatBuiltinsResolution::getResolvedName(Name))
    return BiFManager::BiFManagerHandler::IsBiFDefined(&M, *I16Name);
  return BiFManager::BiFManagerHandler::IsBiFDefined(&M, Name);
#else
  // Without the BiF dictionary every call is left to be resolved as before.
  return true;
#endif // BIF_LINK_BC
}

const MangledType *getPointee(const MangledType &T) {
  if (T.K != MangledType::Kind::Pointer)
    return nullptr;
  const MangledType *P = T.Inner.get();
  while (P->K == MangledType::Kind::Qualified)
    P = P->Inner.get();
  return P;
}

const MangledType *getLongVectorPointee(const MangledType &T, unsigned NumElements) {
  const MangledType *P = getPointee(T);
  return P && P->K == MangledType::Kind::Vector && P->NumElements == NumElements ? P : nullptr;
}

Type *getBuiltinType(LLVMContext &C, const MangledType &T) {
  if (T.K != MangledType::Kind::Builtin)
    return nullptr;
  return StringSwitch<Type *>(T.Text)
      .Case("c", Type::getInt8Ty(C))
      .Case("a", Type::getInt8Ty(C))
      .Case("h", Type::getInt8Ty(C))
      .Case("s", Type::getInt16Ty(C))
      .Case("t", Type::getInt16Ty(C))
      .Case("i", Type::getInt32Ty(C))
      .Case("j", Type::getInt32Ty(C))
      .Case("l", Type::getInt64Ty(C))
      .Case("m", Type::getInt64Ty(C))
      .Case("Dh", Type::getHalfTy(C))
      .Case("f", Type::getFloatTy(C))
      .Case("d", Type::getDoubleTy(C))
      .Default(nullptr);
}

Type *getPointeeElementType(LLVMContext &C, const MangledType &Param, unsigned NumElements) {
  const MangledType *Pointee = getLongVectorPointee(Param, NumElements);
  return Pointee ? getBuiltinType(C, *Pointee->Inner) : nullptr;
}

// SPIR-V relational instructions return bool: <N x i8> holding 0 or 1 for a
// vector in the SPIR-V friendly IR, but i1 from the scalar BiF variant.
bool returnsBool(StringRef Name) {
  return StringSwitch<bool>(Name)
      .Case("__spirv_IsNan", true)
      .Case("__spirv_IsInf", true)
      .Case("__spirv_IsFinite", true)
      .Case("__spirv_IsNormal", true)
      .Case("__spirv_SignBitSet", true)
      .Case("__spirv_Ordered", true)
      .Case("__spirv_Unordered", true)
      .Case("__spirv_LessOrGreater", true)
      .Default(false);
}

// Builtins with a scalar BiF variant whose vector form doesn't apply it to each
// element: a texel's components, the values of consecutive work-items in a
// sub-group block access, and geometric functions, which OpenCL C doesn't
// define for user-defined vector types.
bool isNotElementwise(StringRef Name) {
  if (IGCLLVM::starts_with(Name, "__spirv_Image") ||
      (IGCLLVM::starts_with(Name, "__spirv_Subgroup") && Name.contains("Block")))
    return true;
  return StringSwitch<bool>(Name)
      .Case("__spirv_ocl_cross", true)
      .Case("__spirv_ocl_distance", true)
      .Case("__spirv_ocl_fast_distance", true)
      .Case("__spirv_ocl_length", true)
      .Case("__spirv_ocl_fast_length", true)
      .Case("__spirv_ocl_normalize", true)
      .Case("__spirv_ocl_fast_normalize", true)
      .Default(false);
}

// Width of the vectors the call operates on: the result's, or else the first
// vector parameter's, also behind a pointer.
unsigned getVectorWidth(Function &Fn, const MangledFunction &F) {
  if (auto *RetTy = dyn_cast<IGCLLVM::FixedVectorType>(Fn.getReturnType()))
    return RetTy->getNumElements();
  for (const MangledTypeRef &P : F.Params) {
    const MangledType *T = P.get();
    if (const MangledType *Pointee = getPointee(*T))
      T = Pointee;
    if (T->K == MangledType::Kind::Vector)
      return T->NumElements;
  }
  return 0;
}

IGCLLVM::FixedVectorType *getVectorType(Value *V, unsigned NumElements) {
  auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(V->getType());
  return VTy && VTy->getNumElements() == NumElements ? VTy : nullptr;
}

Value *castToElementPointer(IRBuilder<> &B, Value *Ptr, Type *ElemTy) {
  return B.CreatePointerCast(Ptr, IGCLLVM::PointerType::get(ElemTy, Ptr->getType()->getPointerAddressSpace()));
}

void replaceInst(Instruction &I, Value *V) {
  I.replaceAllUsesWith(V);
  I.eraseFromParent();
}

/// Emits an operation on N-wide vectors element by element in place of \p I:
/// unrolled up to MaxUnrolledWidth elements, otherwise as a loop.
class PerElementEmitter {
public:
  PerElementEmitter(Instruction &I, unsigned NumElements) : I(I), NumElements(NumElements), B(&I) {}

  /// Inserts before \p I, or into the loop body while a callback runs.
  IRBuilder<> &builder() { return B; }

  bool isLoop() const { return NumElements > MaxUnrolledWidth; }

  /// Calls \p Body for each element index and returns the vector of its
  /// results, or null if \p ResultTy is null.
  Value *emit(IGCLLVM::FixedVectorType *ResultTy, function_ref<Value *(Value *Lane)> Body) {
    if (!isLoop()) {
      Value *Result = ResultTy ? PoisonValue::get(ResultTy) : nullptr;
      for (unsigned L = 0; L < NumElements; ++L) {
        Value *R = Body(B.getInt32(L));
        if (ResultTy)
          Result = B.CreateInsertElement(Result, R, B.getInt32(L));
      }
      return Result;
    }
    PHINode *Lane = beginLoop();
    PHINode *Acc = nullptr;
    if (ResultTy) {
      Acc = B.CreatePHI(ResultTy, 2, "acc");
      Acc->addIncoming(PoisonValue::get(ResultTy), Head);
    }
    Value *R = Body(Lane);
    Value *Result = nullptr;
    if (Acc) {
      Result = B.CreateInsertElement(Acc, R, Lane);
      Acc->addIncoming(Result, B.GetInsertBlock());
    }
    endLoop(Lane, NumElements);
    return Result;
  }

  /// Folds \p Count steps into \p Init: Acc = Step(Acc, S) for S = 0 .. Count - 1.
  /// Returns the final accumulator.
  Value *reduce(Value *Init, unsigned Count, function_ref<Value *(Value *Acc, Value *S)> Step) {
    if (!isLoop()) {
      Value *Acc = Init;
      for (unsigned S = 0; S < Count; ++S)
        Acc = Step(Acc, B.getInt32(S));
      return Acc;
    }
    PHINode *S = beginLoop();
    PHINode *Acc = B.CreatePHI(Init->getType(), 2, "acc");
    Acc->addIncoming(Init, Head);
    Value *Result = Step(Acc, S);
    Acc->addIncoming(Result, B.GetInsertBlock());
    endLoop(S, Count);
    return Result;
  }

private:
  Instruction &I;
  unsigned NumElements;
  IRBuilder<> B;
  BasicBlock *Head = nullptr;
  BasicBlock *Loop = nullptr;
  BasicBlock *Tail = nullptr;

  // Splits the block at I and returns the counter of a loop placed before it,
  // with the builder in the loop body.
  PHINode *beginLoop() {
    Head = I.getParent();
    Tail = Head->splitBasicBlock(I.getIterator(), "long.vector.tail");
    Loop = BasicBlock::Create(I.getContext(), "long.vector.loop", I.getFunction(), Tail);
    Head->getTerminator()->setSuccessor(0, Loop);
    B.SetInsertPoint(Loop);
    PHINode *Counter = B.CreatePHI(B.getInt32Ty(), 2, "lane");
    Counter->addIncoming(B.getInt32(0), Head);
    return Counter;
  }

  void endLoop(PHINode *Counter, unsigned TripCount) {
    Value *Next = B.CreateAdd(Counter, B.getInt32(1));
    Counter->addIncoming(Next, B.GetInsertBlock());
    B.CreateCondBr(B.CreateICmpULT(Next, B.getInt32(TripCount)), Loop, Tail);
    B.SetInsertPoint(&I);
  }
};

FunctionCallee getScalarBuiltin(Function &VecFn, const MangledFunction &F, Type *RetTy, ArrayRef<Type *> ArgTys) {
  FunctionType *FT = FunctionType::get(RetTy, ArgTys, false);
  Module &M = *VecFn.getParent();
  std::string Name = mangle(F);
  if (Function *Existing = M.getFunction(Name))
    return FunctionCallee(FT, Existing);
  Function *ScalarFn = Function::Create(FT, GlobalValue::ExternalLinkage, Name, &M);
  ScalarFn->setCallingConv(VecFn.getCallingConv());
  ScalarFn->setAttributes(
      AttributeList::get(M.getContext(), VecFn.getAttributes().getFnAttrs(), AttributeSet(), ArrayRef<AttributeSet>()));
  return ScalarFn;
}

CallInst *createScalarCall(IRBuilder<> &B, CallInst &CI, FunctionCallee Callee, ArrayRef<Value *> Args) {
  CallInst *NewCI = B.CreateCall(Callee, Args);
  NewCI->setCallingConv(CI.getCallingConv());
  NewCI->setAttributes(
      AttributeList::get(CI.getContext(), CI.getAttributes().getFnAttrs(), AttributeSet(), ArrayRef<AttributeSet>()));
  NewCI->copyMetadata(CI);
  if (isa<FPMathOperator>(NewCI) && isa<FPMathOperator>(&CI))
    NewCI->copyFastMathFlags(&CI);
  return NewCI;
}

/// An argument of a builtin on N-wide vectors, as passed to the scalar builtin
/// for one element.
struct ElementArg {
  enum class Kind { Uniform, Vector, VectorPointer };
  Kind K = Kind::Uniform;
  Value *V = nullptr;
  Type *ElemTy = nullptr;
};

/// Replaces \p CI with one call of the scalar BiF builtin per element, passing
/// element i of each vector argument, a pointer to element i for each pointer
/// to a vector, and every other argument unchanged.
bool scalarizeElementwise(CallInst &CI, const MangledFunction &F, unsigned NumElements) {
  auto *VecRetTy = getVectorType(&CI, NumElements);
  if (F.Params.size() != CI.arg_size() || (!VecRetTy && !CI.getType()->isVoidTy()))
    return false;

  SmallVector<ElementArg, 4> Args;
  for (unsigned I = 0; I < CI.arg_size(); ++I) {
    const MangledType &P = *F.Params[I];
    ElementArg A{ElementArg::Kind::Uniform, CI.getArgOperand(I)};
    if (P.K == MangledType::Kind::Vector && P.NumElements == NumElements) {
      auto *VTy = getVectorType(A.V, NumElements);
      if (!VTy)
        return false;
      A = {ElementArg::Kind::Vector, A.V, VTy->getElementType()};
    } else if (const MangledType *Pointee = getLongVectorPointee(P, NumElements)) {
      Type *ElemTy = getBuiltinType(CI.getContext(), *Pointee->Inner);
      if (!ElemTy)
        return false;
      A = {ElementArg::Kind::VectorPointer, A.V, ElemTy};
    }
    Args.push_back(A);
  }
  // A vector computed from scalars only isn't elementwise.
  if (all_of(Args, [](const ElementArg &A) { return A.K == ElementArg::Kind::Uniform; }))
    return false;

  MangledFunction ScalarF = scalarize(F, NumElements);
  if (!isDefinedInBiF(*CI.getModule(), mangle(ScalarF)))
    return false;

  bool BoolResult = VecRetTy && VecRetTy->getElementType()->isIntegerTy(8) && returnsBool(F.Name);
  Type *ScalarRetTy = !VecRetTy    ? CI.getType()
                      : BoolResult ? Type::getInt1Ty(CI.getContext())
                                   : VecRetTy->getElementType();

  PerElementEmitter E(CI, NumElements);
  IRBuilder<> &B = E.builder();
  SmallVector<Type *, 4> ArgTys;
  for (ElementArg &A : Args) {
    if (A.K == ElementArg::Kind::VectorPointer)
      A.V = castToElementPointer(B, A.V, A.ElemTy);
    ArgTys.push_back(A.K == ElementArg::Kind::Vector ? A.ElemTy : A.V->getType());
  }
  FunctionCallee Callee = getScalarBuiltin(*CI.getCalledFunction(), ScalarF, ScalarRetTy, ArgTys);

  Value *Result = E.emit(VecRetTy, [&](Value *Lane) -> Value * {
    SmallVector<Value *, 4> CallArgs;
    for (const ElementArg &A : Args) {
      switch (A.K) {
      case ElementArg::Kind::Uniform:
        CallArgs.push_back(A.V);
        break;
      case ElementArg::Kind::Vector:
        CallArgs.push_back(B.CreateExtractElement(A.V, Lane));
        break;
      case ElementArg::Kind::VectorPointer:
        CallArgs.push_back(B.CreateInBoundsGEP(A.ElemTy, A.V, Lane));
        break;
      }
    }
    Value *R = createScalarCall(B, CI, Callee, CallArgs);
    return BoolResult ? B.CreateZExt(R, VecRetTy->getElementType()) : R;
  });
  if (Result)
    replaceInst(CI, Result);
  else
    CI.eraseFromParent();
  return true;
}

// Same expression as BiF's __spirv_Dot for widths 2-4, extended to any width:
// fma(x0, y0, fma(x1, y1, ... fma(xN-2, yN-2, xN-1 * yN-1))).
bool lowerDot(CallInst &CI, const MangledFunction &F, unsigned NumElements) {
  Value *X = CI.getArgOperand(0);
  Value *Y = CI.getArgOperand(1);
  auto *VTy = getVectorType(X, NumElements);
  if (!VTy || !getVectorType(Y, NumElements) || F.Params.size() != 2)
    return false;
  MangledTypeRef ElemT = scalarize(F, NumElements).Params[0];
  MangledFunction FmaF{"__spirv_ocl_fma", {ElemT, ElemT, ElemT}};
  if (!isDefinedInBiF(*CI.getModule(), mangle(FmaF)))
    return false;

  Type *ElemTy = VTy->getElementType();
  PerElementEmitter E(CI, NumElements);
  IRBuilder<> &B = E.builder();
  if (isa<FPMathOperator>(&CI))
    B.setFastMathFlags(CI.getFastMathFlags());
  FunctionCallee Fma = getScalarBuiltin(*CI.getCalledFunction(), FmaF, ElemTy, {ElemTy, ElemTy, ElemTy});
  Value *Last = B.getInt32(NumElements - 1);
  Value *XLast = B.CreateExtractElement(X, Last);
  Value *Init = B.CreateFMul(XLast, B.CreateExtractElement(Y, Last));
  Value *Dot = E.reduce(Init, NumElements - 1, [&](Value *Acc, Value *S) {
    Value *Idx = B.CreateSub(B.getInt32(NumElements - 2), S);
    Value *XI = B.CreateExtractElement(X, Idx);
    return createScalarCall(B, CI, Fma, {XI, B.CreateExtractElement(Y, Idx), Acc});
  });
  replaceInst(CI, Dot);
  return true;
}

// As BiF, the lanes are reduced bitwise and tested once. That differs from a
// per-lane test only for lanes other than 0 and -1, which the translator never
// passes: it sign-extends the bool vector.
template <bool IsAny> bool lowerAnyAll(CallInst &CI, const MangledFunction &, unsigned NumElements) {
  Value *V = CI.getArgOperand(0);
  auto *VTy = getVectorType(V, NumElements);
  if (!VTy || !VTy->getElementType()->isIntegerTy() || !CI.getType()->isIntegerTy())
    return false;
  PerElementEmitter E(CI, NumElements);
  IRBuilder<> &B = E.builder();
  Value *First = B.CreateExtractElement(V, B.getInt32(0));
  Value *Acc = E.reduce(First, NumElements - 1, [&](Value *Acc, Value *S) {
    Value *Lane = B.CreateExtractElement(V, B.CreateAdd(S, B.getInt32(1)));
    return IsAny ? B.CreateOr(Acc, Lane) : B.CreateAnd(Acc, Lane);
  });
  Value *Res = B.CreateICmpNE(Acc, Constant::getNullValue(Acc->getType()));
  replaceInst(CI, B.CreateZExt(Res, CI.getType()));
  return true;
}

// OpenCL.std select tests the MSB of c for vectors but c != 0 for scalars, so
// the scalar BiF variant doesn't apply. This is BiF's per-element expression as
// one vector select, which the scalarizer splits; emitting it per element here
// spilled more at most widths.
bool lowerSelect(CallInst &CI, const MangledFunction &, unsigned NumElements) {
  Value *A = CI.getArgOperand(0);
  Value *Bv = CI.getArgOperand(1);
  Value *C = CI.getArgOperand(2);
  auto *CTy = getVectorType(C, NumElements);
  if (!getVectorType(A, NumElements) || !CTy || !CTy->getElementType()->isIntegerTy() || A->getType() != Bv->getType())
    return false;
  IRBuilder<> B(&CI);
  Value *MSBSet = B.CreateICmpSLT(C, Constant::getNullValue(CTy));
  replaceInst(CI, B.CreateSelect(MSBSet, Bv, A));
  return true;
}

// Element i of the result is element mask[i] of x, or of x and y concatenated
// for shuffle2. OpenCL.std considers only the low ilogb(2m - 1) bits of a mask
// element, one more for shuffle2, which can address past the m elements of x
// (2m of x and y) when m isn't a power of 2. Such an index gives 0, as BiF's
// compare-and-select chains would, since they start from 0.
template <bool IsShuffle2> bool lowerShuffle(CallInst &CI, const MangledFunction &, unsigned) {
  Value *X = CI.getArgOperand(0);
  Value *Y = IsShuffle2 ? CI.getArgOperand(1) : nullptr;
  Value *Mask = CI.getArgOperand(IsShuffle2 ? 2 : 1);
  auto *SrcTy = dyn_cast<IGCLLVM::FixedVectorType>(X->getType());
  auto *MaskTy = dyn_cast<IGCLLVM::FixedVectorType>(Mask->getType());
  auto *RetTy = dyn_cast<IGCLLVM::FixedVectorType>(CI.getType());
  if (!SrcTy || !MaskTy || !RetTy || MaskTy->getNumElements() != RetTy->getNumElements())
    return false;
  if (IsShuffle2 && Y->getType() != SrcTy)
    return false;

  PerElementEmitter E(CI, RetTy->getNumElements());
  IRBuilder<> &B = E.builder();
  unsigned SrcElements = SrcTy->getNumElements();
  uint64_t NumIndices = SrcElements * (IsShuffle2 ? 2 : 1);
  uint64_t IndexRange = PowerOf2Ceil(SrcElements) * (IsShuffle2 ? 2 : 1);
  // The source width doesn't have to fit in a narrow mask element type.
  Type *MaskElemTy = MaskTy->getElementType();
  Type *IdxTy = MaskElemTy->getIntegerBitWidth() < 32 ? B.getInt32Ty() : MaskElemTy;
  Value *Width = ConstantInt::get(IdxTy, SrcElements);
  Value *Result = E.emit(RetTy, [&](Value *Lane) {
    Value *MaskElem = B.CreateZExt(B.CreateExtractElement(Mask, Lane), IdxTy);
    Value *Idx = B.CreateAnd(MaskElem, ConstantInt::get(IdxTy, IndexRange - 1));
    Value *SrcIdx = IndexRange == SrcElements ? Idx : B.CreateURem(Idx, Width);
    Value *R = B.CreateExtractElement(X, SrcIdx);
    if (Y)
      R = B.CreateSelect(B.CreateICmpULT(Idx, Width), R, B.CreateExtractElement(Y, SrcIdx));
    if (IndexRange != NumIndices)
      R = B.CreateSelect(B.CreateICmpULT(Idx, ConstantInt::get(IdxTy, NumIndices)), R,
                         Constant::getNullValue(SrcTy->getElementType()));
    return R;
  });
  replaceInst(CI, Result);
  return true;
}

// Number of elements a vector occupies in memory: N rounded up to a power of 2,
// as MR 248 lays out user-defined vector types and OpenCL does for 3 elements.
uint64_t getPaddedElements(const DataLayout &DL, Type *ElemTy, unsigned NumElements) {
  return DL.getTypeAllocSize(IGCLLVM::FixedVectorType::get(ElemTy, NumElements)) / DL.getTypeAllocSize(ElemTy);
}

// The vload/vstore family addresses p + offset * stride elements, where the
// stride is n, or the padded width for the aligned vloada/vstorea variants.
Value *getVLoadStoreAddress(IRBuilder<> &B, Value *Offset, Value *Ptr, Type *ElemTy, uint64_t Stride) {
  Value *Base = castToElementPointer(B, Ptr, ElemTy);
  Value *Idx = B.CreateMul(Offset, ConstantInt::get(Offset->getType(), Stride));
  return B.CreateInBoundsGEP(ElemTy, Base, Idx);
}

bool lowerVLoadN(CallInst &CI, const MangledFunction &, unsigned NumElements) {
  auto *VTy = getVectorType(&CI, NumElements);
  if (!VTy || CI.arg_size() != 3)
    return false;
  IRBuilder<> B(&CI);
  Type *ElemTy = VTy->getElementType();
  Value *Addr = getVLoadStoreAddress(B, CI.getArgOperand(0), CI.getArgOperand(1), ElemTy, NumElements);
  Align ElemAlign = CI.getModule()->getDataLayout().getABITypeAlign(ElemTy);
  replaceInst(CI, B.CreateAlignedLoad(VTy, castToElementPointer(B, Addr, VTy), ElemAlign));
  return true;
}

bool lowerVStoreN(CallInst &CI, const MangledFunction &, unsigned NumElements) {
  Value *Data = CI.getArgOperand(0);
  auto *VTy = getVectorType(Data, NumElements);
  if (!VTy || CI.arg_size() != 3)
    return false;
  IRBuilder<> B(&CI);
  Type *ElemTy = VTy->getElementType();
  Value *Addr = getVLoadStoreAddress(B, CI.getArgOperand(1), CI.getArgOperand(2), ElemTy, NumElements);
  Align ElemAlign = CI.getModule()->getDataLayout().getABITypeAlign(ElemTy);
  B.CreateAlignedStore(Data, castToElementPointer(B, Addr, VTy), ElemAlign);
  CI.eraseFromParent();
  return true;
}

// Converting half to float is exact, so the BiF conversion isn't needed.
template <bool IsAligned> bool lowerVLoadHalfN(CallInst &CI, const MangledFunction &, unsigned NumElements) {
  auto *VTy = getVectorType(&CI, NumElements);
  if (!VTy || !VTy->getElementType()->isFloatTy() || CI.arg_size() != 3)
    return false;
  IRBuilder<> B(&CI);
  Type *HalfTy = B.getHalfTy();
  auto *HalfVTy = IGCLLVM::FixedVectorType::get(HalfTy, NumElements);
  uint64_t Padded = getPaddedElements(CI.getModule()->getDataLayout(), HalfTy, NumElements);
  uint64_t Stride = IsAligned ? Padded : NumElements;
  Value *Addr = getVLoadStoreAddress(B, CI.getArgOperand(0), CI.getArgOperand(1), HalfTy, Stride);
  Value *Halves =
      B.CreateAlignedLoad(HalfVTy, castToElementPointer(B, Addr, HalfVTy), Align(IsAligned ? 2 * Padded : 2));
  replaceInst(CI, B.CreateFPExt(Halves, VTy));
  return true;
}

// As BiF, each element is converted with __spirv_FConvert_Rhalf_<mode> (rte
// without _r) and the half vector is stored at p + offset * stride. double
// needs the BiF conversion, fptrunc would round twice. The loop form stores
// each element right away; collecting the vector first spilled.
template <bool IsAligned> bool lowerVStoreHalfN(CallInst &CI, const MangledFunction &F, unsigned NumElements) {
  Value *Data = CI.getArgOperand(0);
  auto *VTy = getVectorType(Data, NumElements);
  bool HasRoundingMode = IGCLLVM::ends_with(F.Name, "_r");
  if (!VTy || !VTy->getElementType()->isFloatingPointTy() || CI.arg_size() != (HasRoundingMode ? 4u : 3u) ||
      F.Params.size() != CI.arg_size())
    return false;
  StringRef Mode = "rte";
  if (HasRoundingMode) {
    // SPIR-V FP Rounding Mode: RTE, RTZ, RTP, RTN.
    auto *ModeArg = dyn_cast<ConstantInt>(CI.getArgOperand(3));
    if (!ModeArg || ModeArg->getZExtValue() > 3)
      return false;
    static const char *const Modes[] = {"rte", "rtz", "rtp", "rtn"};
    Mode = Modes[ModeArg->getZExtValue()];
  }
  MangledFunction ConvF{("__spirv_FConvert_Rhalf_" + Mode).str(), {scalarize(F, NumElements).Params[0]}};
  if (!isDefinedInBiF(*CI.getModule(), mangle(ConvF)))
    return false;

  PerElementEmitter E(CI, NumElements);
  IRBuilder<> &B = E.builder();
  Type *HalfTy = B.getHalfTy();
  auto *HalfVTy = IGCLLVM::FixedVectorType::get(HalfTy, NumElements);
  FunctionCallee Conv = getScalarBuiltin(*CI.getCalledFunction(), ConvF, HalfTy, {VTy->getElementType()});
  uint64_t Padded = getPaddedElements(CI.getModule()->getDataLayout(), HalfTy, NumElements);
  uint64_t Stride = IsAligned ? Padded : NumElements;
  Value *Addr = getVLoadStoreAddress(B, CI.getArgOperand(1), CI.getArgOperand(2), HalfTy, Stride);
  auto Convert = [&](Value *Lane) { return createScalarCall(B, CI, Conv, {B.CreateExtractElement(Data, Lane)}); };
  if (E.isLoop()) {
    E.emit(nullptr, [&](Value *Lane) -> Value * {
      B.CreateAlignedStore(Convert(Lane), B.CreateInBoundsGEP(HalfTy, Addr, Lane), Align(2));
      return nullptr;
    });
  } else {
    Value *Halves = E.emit(HalfVTy, Convert);
    B.CreateAlignedStore(Halves, castToElementPointer(B, Addr, HalfVTy), Align(IsAligned ? 2 * Padded : 2));
  }
  CI.eraseFromParent();
  return true;
}

// prefetch(p, num_elements) and an async copy of contiguous vectors are the same
// operation on num_elements times the padded width of scalars.
bool lowerContiguousCopy(CallInst &CI, const MangledFunction &F, unsigned NumElements, ArrayRef<unsigned> PtrArgs,
                         unsigned CountArg) {
  Type *ElemTy = getPointeeElementType(CI.getContext(), *F.Params[PtrArgs[0]], NumElements);
  if (!ElemTy || any_of(PtrArgs, [&](unsigned I) {
        return getPointeeElementType(CI.getContext(), *F.Params[I], NumElements) != ElemTy;
      }))
    return false;
  SmallVector<Value *, 6> Args(CI.args());
  IRBuilder<> B(&CI);
  for (unsigned I : PtrArgs)
    Args[I] = castToElementPointer(B, Args[I], ElemTy);
  Value *Count = Args[CountArg];
  uint64_t Padded = getPaddedElements(CI.getModule()->getDataLayout(), ElemTy, NumElements);
  Args[CountArg] = B.CreateMul(Count, ConstantInt::get(Count->getType(), Padded));
  SmallVector<Type *, 6> ArgTys;
  for (Value *A : Args)
    ArgTys.push_back(A->getType());
  FunctionCallee Callee = getScalarBuiltin(*CI.getCalledFunction(), scalarize(F, NumElements), CI.getType(), ArgTys);
  replaceInst(CI, createScalarCall(B, CI, Callee, Args));
  return true;
}

bool lowerPrefetch(CallInst &CI, const MangledFunction &F, unsigned NumElements) {
  return lowerContiguousCopy(CI, F, NumElements, {0}, 1);
}

// GroupAsyncCopy(scope, dst, src, num_elements, stride, event). The stride
// applies to the global side only, so a strided copy of N-wide vectors isn't one
// strided copy of scalars. Each vector is copied as a contiguous run of its
// padded width instead, in a loop over num_elements. All copies join the event
// of an initial empty copy, so no event value has to flow around the loop.
// Stride 0 is undefined in OpenCL C; as BiF, it copies contiguously.
bool lowerGroupAsyncCopy(CallInst &CI, const MangledFunction &F, unsigned NumElements) {
  auto *Stride = dyn_cast<ConstantInt>(CI.getArgOperand(4));
  if (Stride && (Stride->isOne() || Stride->isZero()))
    return lowerContiguousCopy(CI, F, NumElements, {1, 2}, 3);

  Type *ElemTy = getPointeeElementType(CI.getContext(), *F.Params[1], NumElements);
  if (!ElemTy || getPointeeElementType(CI.getContext(), *F.Params[2], NumElements) != ElemTy)
    return false;
  Value *Dst = CI.getArgOperand(1);
  Value *Src = CI.getArgOperand(2);
  Value *Num = CI.getArgOperand(3);
  Value *StrideV = CI.getArgOperand(4);
  bool DstIsLocal = Dst->getType()->getPointerAddressSpace() == ADDRESS_SPACE_LOCAL;
  uint64_t Padded = getPaddedElements(CI.getModule()->getDataLayout(), ElemTy, NumElements);

  IRBuilder<> B(&CI);
  Value *DstBase = castToElementPointer(B, Dst, ElemTy);
  Value *SrcBase = castToElementPointer(B, Src, ElemTy);
  SmallVector<Type *, 6> ArgTys = {
      CI.getArgOperand(0)->getType(), DstBase->getType(), SrcBase->getType(), Num->getType(), StrideV->getType(),
      CI.getArgOperand(5)->getType()};
  FunctionCallee Callee = getScalarBuiltin(*CI.getCalledFunction(), scalarize(F, NumElements), CI.getType(), ArgTys);
  Value *One = ConstantInt::get(StrideV->getType(), 1);
  Value *GlobalStride =
      B.CreateSelect(B.CreateICmpEQ(StrideV, Constant::getNullValue(StrideV->getType())), One, StrideV);
  Value *Event = createScalarCall(
      B, CI, Callee,
      {CI.getArgOperand(0), DstBase, SrcBase, Constant::getNullValue(Num->getType()), One, CI.getArgOperand(5)});

  BasicBlock *Head = CI.getParent();
  BasicBlock *Tail = Head->splitBasicBlock(CI.getIterator(), "async.copy.tail");
  BasicBlock *Loop = BasicBlock::Create(CI.getContext(), "async.copy.loop", CI.getFunction(), Tail);
  Head->getTerminator()->eraseFromParent();
  B.SetInsertPoint(Head);
  B.CreateCondBr(B.CreateICmpNE(Num, Constant::getNullValue(Num->getType())), Loop, Tail);

  B.SetInsertPoint(Loop);
  PHINode *I = B.CreatePHI(Num->getType(), 2, "i");
  I->addIncoming(Constant::getNullValue(Num->getType()), Head);
  Value *Strided = B.CreateMul(I, B.CreateZExtOrTrunc(GlobalStride, Num->getType()));
  Value *PaddedV = ConstantInt::get(Num->getType(), Padded);
  Value *DstI = B.CreateInBoundsGEP(ElemTy, DstBase, B.CreateMul(DstIsLocal ? I : Strided, PaddedV));
  Value *SrcI = B.CreateInBoundsGEP(ElemTy, SrcBase, B.CreateMul(DstIsLocal ? Strided : I, PaddedV));
  createScalarCall(B, CI, Callee, {CI.getArgOperand(0), DstI, SrcI, PaddedV, One, Event});
  Value *Next = B.CreateAdd(I, ConstantInt::get(Num->getType(), 1));
  I->addIncoming(Next, Loop);
  B.CreateCondBr(B.CreateICmpULT(Next, Num), Loop, Tail);

  replaceInst(CI, Event);
  return true;
}

// The translator returns the {low, high} pair through an sret pointer. As BiF,
// the low half is the plain product and the high half comes from [us]_mul_hi.
// The loop form stores both halves of each element right away; computing the
// low half after the loop kept both operands live across it.
template <bool IsSigned> bool lowerMulExtended(CallInst &CI, const MangledFunction &F, unsigned NumElements) {
  if (CI.arg_size() != 3 || F.Params.size() != 2)
    return false;
  Type *SRetTy = CI.getAttributes().getParamStructRetType(0);
  if (!SRetTy)
    SRetTy = CI.getCalledFunction()->getAttributes().getParamStructRetType(0);
  auto *STy = dyn_cast_or_null<StructType>(SRetTy);
  Value *X = CI.getArgOperand(1);
  Value *Y = CI.getArgOperand(2);
  auto *VTy = getVectorType(X, NumElements);
  if (!STy || STy->getNumElements() != 2 || !VTy || STy->getElementType(0) != VTy || STy->getElementType(1) != VTy)
    return false;

  MangledFunction ScalarHiF =
      scalarize({IsSigned ? "__spirv_ocl_s_mul_hi" : "__spirv_ocl_u_mul_hi", F.Params}, NumElements);
  if (!isDefinedInBiF(*CI.getModule(), mangle(ScalarHiF)))
    return false;

  Value *Ptr = CI.getArgOperand(0);
  PerElementEmitter E(CI, NumElements);
  IRBuilder<> &B = E.builder();
  Type *ElemTy = VTy->getElementType();
  FunctionCallee HiFn = getScalarBuiltin(*CI.getCalledFunction(), ScalarHiF, ElemTy, {ElemTy, ElemTy});
  auto MulHi = [&](Value *XI, Value *YI) { return createScalarCall(B, CI, HiFn, {XI, YI}); };
  if (!E.isLoop()) {
    Value *Hi = E.emit(VTy, [&](Value *Lane) {
      Value *XI = B.CreateExtractElement(X, Lane);
      return MulHi(XI, B.CreateExtractElement(Y, Lane));
    });
    B.CreateStore(B.CreateMul(X, Y), B.CreateStructGEP(STy, Ptr, 0));
    B.CreateStore(Hi, B.CreateStructGEP(STy, Ptr, 1));
    CI.eraseFromParent();
    return true;
  }

  Value *LoBase = castToElementPointer(B, B.CreateStructGEP(STy, Ptr, 0), ElemTy);
  Value *HiBase = castToElementPointer(B, B.CreateStructGEP(STy, Ptr, 1), ElemTy);
  E.emit(nullptr, [&](Value *Lane) -> Value * {
    Value *XI = B.CreateExtractElement(X, Lane);
    Value *YI = B.CreateExtractElement(Y, Lane);
    B.CreateStore(B.CreateMul(XI, YI), B.CreateInBoundsGEP(ElemTy, LoBase, Lane));
    B.CreateStore(MulHi(XI, YI), B.CreateInBoundsGEP(ElemTy, HiBase, Lane));
    return nullptr;
  });
  CI.eraseFromParent();
  return true;
}

using LowerFn = bool (*)(CallInst &CI, const MangledFunction &F, unsigned NumElements);

// Builtins whose vector form isn't the scalar form applied to each element.
LowerFn getCustomLowering(StringRef BaseName) {
  return StringSwitch<LowerFn>(BaseName)
      .Case("__spirv_Dot", lowerDot)
      .Case("__spirv_Any", lowerAnyAll<true>)
      .Case("__spirv_All", lowerAnyAll<false>)
      .Case("__spirv_ocl_select", lowerSelect)
      .Case("__spirv_ocl_shuffle", lowerShuffle<false>)
      .Case("__spirv_ocl_shuffle2", lowerShuffle<true>)
      .Case("__spirv_ocl_vloadn", lowerVLoadN)
      .Case("__spirv_ocl_vstoren", lowerVStoreN)
      .Case("__spirv_ocl_vload_halfn", lowerVLoadHalfN<false>)
      .Case("__spirv_ocl_vloada_halfn", lowerVLoadHalfN<true>)
      .Case("__spirv_ocl_vstore_halfn", lowerVStoreHalfN<false>)
      .Case("__spirv_ocl_vstore_halfn_r", lowerVStoreHalfN<false>)
      .Case("__spirv_ocl_vstorea_halfn", lowerVStoreHalfN<true>)
      .Case("__spirv_ocl_vstorea_halfn_r", lowerVStoreHalfN<true>)
      .Case("__spirv_ocl_prefetch", lowerPrefetch)
      .Case("__spirv_GroupAsyncCopy", lowerGroupAsyncCopy)
      .Case("__spirv_UMulExtended", lowerMulExtended<false>)
      .Case("__spirv_SMulExtended", lowerMulExtended<true>)
      .Default(nullptr);
}

bool isOpenCLVectorWidth(unsigned NumElements) {
  return NumElements == 2 || NumElements == 3 || NumElements == 4 || NumElements == 8 || NumElements == 16;
}

// The kernel metadata isn't set up yet, so EmitError can't attribute the error
// to a kernel; the message names the function instead. The call is removed, so
// its undefined vector variant isn't reported as well.
void reportInvalidWidth(CallInst &CI, StringRef BaseName, unsigned NumElements, CodeGenContext &Ctx) {
  std::string Msg = ("in function '" + CI.getFunction()->getName() + "': " + BaseName + " on " + Twine(NumElements) +
                     "-component vectors is invalid SPIR-V")
                        .str();
  Ctx.EmitError(Msg.c_str(), NoIRContext);
  if (!CI.getType()->isVoidTy())
    CI.replaceAllUsesWith(PoisonValue::get(CI.getType()));
  CI.eraseFromParent();
}

bool lowerCall(CallInst &CI, const MangledFunction &F, unsigned NumElements, CodeGenContext &Ctx) {
  StringRef BaseName = getBaseName(F.Name);
  if (LowerFn Lower = getCustomLowering(BaseName))
    return Lower(CI, F, NumElements);
  if (isNotElementwise(F.Name)) {
    // At the OpenCL C widths some of these calls are valid and are resolved by
    // BiF or a later pass. No extension allows any of them at other widths.
    if (isOpenCLVectorWidth(NumElements))
      return false;
    reportInvalidWidth(CI, BaseName, NumElements, Ctx);
    return true;
  }
  return scalarizeElementwise(CI, F, NumElements);
}

} // namespace

bool LongVectorBuiltinScalarization::run(Module &M, CodeGenContext *Ctx) {
  bool Changed = false;
  for (Function &Fn : make_early_inc_range(M)) {
    if (!Fn.isDeclaration() || !Fn.getName().contains("__spirv_"))
      continue;
    std::optional<MangledFunction> F = demangle(Fn.getName());
    if (!F || !IGCLLVM::starts_with(F->Name, "__spirv_"))
      continue;

    unsigned NumElements = getVectorWidth(Fn, *F);
    if (!NumElements || isDefinedInBiF(M, Fn.getName()))
      continue;

    for (User *U : make_early_inc_range(Fn.users()))
      if (auto *CI = dyn_cast<CallInst>(U); CI && CI->getCalledFunction() == &Fn)
        Changed |= lowerCall(*CI, *F, NumElements, *Ctx);
    if (Fn.use_empty()) {
      Fn.eraseFromParent();
      Changed = true;
    }
  }
  return Changed;
}
