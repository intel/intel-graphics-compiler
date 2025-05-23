/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXBuiltinFunctions
/// -----------------
///
/// GenXMathFunction is a module pass that implements floating point math
/// functions
///
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"

#include "vc/Support/GenXDiagnostic.h"
#include "vc/Utils/GenX/IntrinsicsWrapper.h"
#include "vc/Utils/GenX/KernelInfo.h"
#include "vc/Utils/General/BiF.h"

#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Module.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Pass.h>

#include <string>

#define DEBUG_TYPE "genx-builtin-functions"

using namespace llvm;

class GenXBuiltinFunctions : public ModulePass,
                             public InstVisitor<GenXBuiltinFunctions, Value *> {
public:
  static char ID;

  explicit GenXBuiltinFunctions(
      BuiltinFunctionKind Kind = BuiltinFunctionKind::PostLegalization)
      : ModulePass(ID), Kind(Kind) {}
  StringRef getPassName() const override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnModule(Module &M) override;
  void runOnFunction(Function &F);

  Value *visitInstruction(Instruction &I) { return nullptr; }

  Value *visitFPToSI(CastInst &I);
  Value *visitFPToUI(CastInst &I);
  Value *visitSIToFP(CastInst &I);
  Value *visitUIToFP(CastInst &I);

  Value *visitFDiv(BinaryOperator &I);
  Value *visitFRem(BinaryOperator &I);
  Value *visitSDiv(BinaryOperator &I);
  Value *visitSRem(BinaryOperator &I);
  Value *visitUDiv(BinaryOperator &I);
  Value *visitURem(BinaryOperator &I);

  Value *visitCallInst(CallInst &II);

private:
  Function *getBuiltinDeclaration(Module &M, StringRef Name, bool IsFast,
                                  ArrayRef<Type *> Types,
                                  StringRef Suffix = "");

  std::unique_ptr<Module> loadBuiltinLib(LLVMContext &Ctx, const DataLayout &DL,
                                         const std::string &Triple);

  Value *createLibraryCall(Instruction &I, Function *Func,
                           ArrayRef<Value *> Args);
  Value *createAtomicLibraryCall(llvm::CallInst &II, StringRef Name);

  bool isHandleUgmAtomics(const CallInst &II) const;
  bool isHandleSlmAtomics(const CallInst &II) const;

  const GenXSubtarget *ST = nullptr;
  BuiltinFunctionKind Kind;
};

char GenXBuiltinFunctions::ID = 0;

StringRef GenXBuiltinFunctions::getPassName() const {
  return "GenX floating-point math functions";
}

void GenXBuiltinFunctions::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetPassConfig>();
  AU.addRequired<GenXBackendConfig>();
}

namespace llvm {
void initializeGenXBuiltinFunctionsPass(PassRegistry &);
} // namespace llvm

INITIALIZE_PASS_BEGIN(GenXBuiltinFunctions, "GenXBuiltinFunctions",
                      "GenXBuiltinFunctions", false, false)
INITIALIZE_PASS_END(GenXBuiltinFunctions, "GenXBuiltinFunctions",
                    "GenXBuiltinFunctions", false, false)

ModulePass *llvm::createGenXBuiltinFunctionsPass(BuiltinFunctionKind Kind) {
  initializeGenXBuiltinFunctionsPass(*PassRegistry::getPassRegistry());
  return new GenXBuiltinFunctions(Kind);
}

bool GenXBuiltinFunctions::runOnModule(Module &M) {
  ST = &getAnalysis<TargetPassConfig>()
            .getTM<GenXTargetMachine>()
            .getGenXSubtarget();

  auto &Ctx = M.getContext();
  auto Lib = loadBuiltinLib(Ctx, M.getDataLayout(), M.getTargetTriple());
  if (Lib && Linker::linkModules(M, std::move(Lib))) {
    vc::diagnose(Ctx, "GenXBuiltinFunctions",
                 "Error linking built-in functions");
    return true;
  }

  for (auto &F : M.getFunctionList())
    runOnFunction(F);

  // Remove unused built-in functions, mark used as internal
  std::vector<Function *> ToErase;
  for (auto &F : M.getFunctionList())
    if (vc::isBuiltinFunction(F)) {
      if (F.use_empty())
        ToErase.push_back(&F);
      else
        F.setLinkage(GlobalValue::InternalLinkage);
    }

  for (auto *F : ToErase)
    F->eraseFromParent();

  return true;
}

void GenXBuiltinFunctions::runOnFunction(Function &F) {
  std::vector<Instruction *> ToErase;
  for (auto &BB : F)
    for (auto &Inst : BB)
      if (auto *NewVal = visit(Inst)) {
        Inst.replaceAllUsesWith(NewVal);
        ToErase.push_back(&Inst);
      }

  for (auto *Inst : ToErase)
    Inst->eraseFromParent();
}

Value *GenXBuiltinFunctions::createLibraryCall(Instruction &I, Function *Func,
                                               ArrayRef<Value *> Args) {
  if (!Func)
    return nullptr;

  IRBuilder<> Builder(&I);
  LLVM_DEBUG(dbgs() << "Replace instruction: " << I << "\n");

  auto *Call = Builder.CreateCall(Func, Args);
  Call->takeName(&I);

  LLVM_DEBUG(dbgs() << "Replaced with: " << *Call << "\n");

  return Call;
}

Value *GenXBuiltinFunctions::visitFPToSI(CastInst &I) {
  auto &M = *I.getModule();
  auto *Arg = I.getOperand(0);
  auto *STy = Arg->getType();
  auto *DTy = I.getType();

  if (!ST->emulateLongLong() || !DTy->getScalarType()->isIntegerTy(64))
    return nullptr;

  auto *Func = getBuiltinDeclaration(M, "fptosi", false, {STy});
  return createLibraryCall(I, Func, {Arg});
}

Value *GenXBuiltinFunctions::visitFPToUI(CastInst &I) {
  auto &M = *I.getModule();
  auto *Arg = I.getOperand(0);
  auto *STy = Arg->getType();
  auto *DTy = I.getType();

  if (!ST->emulateLongLong() || !DTy->getScalarType()->isIntegerTy(64))
    return nullptr;

  auto *Func = getBuiltinDeclaration(M, "fptoui", false, {STy});
  return createLibraryCall(I, Func, {Arg});
}

Value *GenXBuiltinFunctions::visitSIToFP(CastInst &I) {
  auto &M = *I.getModule();
  auto *Arg = I.getOperand(0);
  auto *STy = Arg->getType();
  auto *DTy = I.getType();

  if (!ST->emulateLongLong() || !STy->getScalarType()->isIntegerTy(64))
    return nullptr;

  auto *Func = getBuiltinDeclaration(M, "sitofp", false, {DTy});
  return createLibraryCall(I, Func, {Arg});
}

Value *GenXBuiltinFunctions::visitUIToFP(CastInst &I) {
  auto &M = *I.getModule();
  auto *Arg = I.getOperand(0);
  auto *STy = Arg->getType();
  auto *DTy = I.getType();

  if (!ST->emulateLongLong() || !STy->getScalarType()->isIntegerTy(64))
    return nullptr;

  auto *Func = getBuiltinDeclaration(M, "uitofp", false, {DTy});
  return createLibraryCall(I, Func, {Arg});
}

Value *GenXBuiltinFunctions::visitFDiv(BinaryOperator &I) {
  auto &M = *I.getModule();
  auto *Ty = I.getType();

  if (!Ty->getScalarType()->isDoubleTy())
    return nullptr;

  auto *Func = getBuiltinDeclaration(M, "fdiv", I.hasAllowReciprocal(), {Ty});
  return createLibraryCall(I, Func, {I.getOperand(0), I.getOperand(1)});
}

Value *GenXBuiltinFunctions::visitFRem(BinaryOperator &I) {
  auto &M = *I.getModule();
  auto *Ty = I.getType();
  StringRef Suffix = "__rte_";
  auto *Func =
      getBuiltinDeclaration(M, "frem", I.hasAllowReciprocal(), {Ty}, Suffix);
  return createLibraryCall(I, Func, {I.getOperand(0), I.getOperand(1)});
}

Value *GenXBuiltinFunctions::visitSDiv(BinaryOperator &I) {
  auto &M = *I.getModule();
  auto *Ty = I.getType();
  auto *STy = Ty->getScalarType();

  if (ST->hasIntDivRem32() && !STy->isIntegerTy(64))
    return nullptr;

  StringRef Suffix = STy->isIntegerTy(32) ? "__rtz_" : "";

  auto *Func = getBuiltinDeclaration(M, "sdiv", false, {Ty}, Suffix);
  return createLibraryCall(I, Func, {I.getOperand(0), I.getOperand(1)});
}

Value *GenXBuiltinFunctions::visitSRem(BinaryOperator &I) {
  auto &M = *I.getModule();
  auto *Ty = I.getType();
  auto *STy = Ty->getScalarType();

  if (ST->hasIntDivRem32() && !STy->isIntegerTy(64))
    return nullptr;

  StringRef Suffix = STy->isIntegerTy(32) ? "__rtz_" : "";

  auto *Func = getBuiltinDeclaration(M, "srem", false, {Ty}, Suffix);
  return createLibraryCall(I, Func, {I.getOperand(0), I.getOperand(1)});
}

Value *GenXBuiltinFunctions::visitUDiv(BinaryOperator &I) {
  auto &M = *I.getModule();
  auto *Ty = I.getType();
  auto *STy = Ty->getScalarType();

  if (ST->hasIntDivRem32() && !STy->isIntegerTy(64))
    return nullptr;

  StringRef Suffix = STy->isIntegerTy(32) ? "__rtz_" : "";

  auto *Func = getBuiltinDeclaration(M, "udiv", false, {Ty}, Suffix);
  return createLibraryCall(I, Func, {I.getOperand(0), I.getOperand(1)});
}

Value *GenXBuiltinFunctions::visitURem(BinaryOperator &I) {
  auto &M = *I.getModule();
  auto *Ty = I.getType();
  auto *STy = Ty->getScalarType();

  if (ST->hasIntDivRem32() && !STy->isIntegerTy(64))
    return nullptr;

  StringRef Suffix = STy->isIntegerTy(32) ? "__rtz_" : "";

  auto *Func = getBuiltinDeclaration(M, "urem", false, {Ty}, Suffix);
  return createLibraryCall(I, Func, {I.getOperand(0), I.getOperand(1)});
}

bool GenXBuiltinFunctions::isHandleUgmAtomics(const CallInst &II) const {
  auto *Opcode = cast<ConstantInt>(II.getArgOperand(1));

  auto *Ty = II.getType();
  if (auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty))
    Ty = VTy->getElementType();

  switch (Opcode->getZExtValue()) {
  case LSC_ATOMIC_FADD:
  case LSC_ATOMIC_FSUB:
    return (Ty->isDoubleTy() && !ST->hasGlobalAtomicAddF64()) ||
           cast<ConstantInt>(II.getArgOperand(3))->getZExtValue() ==
               LSC_DATA_SIZE_16c32b;
  case LSC_ATOMIC_FMIN:
  case LSC_ATOMIC_FMAX:
  case LSC_ATOMIC_FCAS:
    return Ty->isDoubleTy();
  default:
    return false;
  }
}

bool GenXBuiltinFunctions::isHandleSlmAtomics(const CallInst &II) const {
  auto *Opcode = cast<ConstantInt>(II.getArgOperand(1));

  auto *Ty = II.getType();
  if (auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty))
    Ty = VTy->getElementType();

  switch (Opcode->getZExtValue()) {
  case LSC_ATOMIC_ICAS:
    return false;
  case LSC_ATOMIC_FADD:
  case LSC_ATOMIC_FSUB:
    return !Ty->isDoubleTy() || ST->hasLocalIntegerCas64();
  default:
    return (Ty->isIntegerTy(64) || Ty->isDoubleTy()) &&
           ST->hasLocalIntegerCas64();
  }
}

Value *GenXBuiltinFunctions::visitCallInst(CallInst &II) {
  auto IID = vc::getAnyIntrinsicID(&II);
  auto *Ty = II.getType();
  auto &M = *II.getModule();
  IRBuilder<> Builder(&II);
  Function *Func = nullptr;
  SmallVector<Value *, 2> Args(II.args());

  switch (IID) {
  case Intrinsic::sqrt:
    Func = getBuiltinDeclaration(M, "fsqrt", II.hasApproxFunc(), {Ty});
    break;

  case GenXIntrinsic::genx_sqrt:
    Func = getBuiltinDeclaration(M, "fsqrt", true, {Ty});
    break;
  case GenXIntrinsic::genx_ieee_sqrt:
    Func = getBuiltinDeclaration(M, "fsqrt", false, {Ty});
    break;
  case GenXIntrinsic::genx_ieee_div:
    Func = getBuiltinDeclaration(M, "fdiv", false, {Ty});
    break;
  case GenXIntrinsic::genx_rsqrt:
    Func = getBuiltinDeclaration(M, "rsqrt", false, {Ty}, "__rte_");
    break;
  case GenXIntrinsic::genx_fptosi_sat: {
    auto *Arg = II.getArgOperand(0);
    auto *STy = Arg->getType();
    if (!ST->emulateLongLong() || !Ty->getScalarType()->isIntegerTy(64))
      return nullptr;
    Func = getBuiltinDeclaration(M, "fptosi", false, {STy});
  } break;
  case GenXIntrinsic::genx_fptoui_sat: {
    auto *Arg = II.getArgOperand(0);
    auto *STy = Arg->getType();
    if (!ST->emulateLongLong() || !Ty->getScalarType()->isIntegerTy(64))
      return nullptr;
    Func = getBuiltinDeclaration(M, "fptoui", false, {STy});
  } break;

  case vc::InternalIntrinsic::lsc_atomic_slm:
    if (!isHandleSlmAtomics(II))
      return nullptr;
    return createAtomicLibraryCall(II, "atomic_slm");
  case vc::InternalIntrinsic::lsc_atomic_ugm:
    if (!isHandleUgmAtomics(II))
      return nullptr;
    return createAtomicLibraryCall(II, "atomic_ugm");
  default:
    break;
  }

  return createLibraryCall(II, Func, Args);
}

Value *GenXBuiltinFunctions::createAtomicLibraryCall(CallInst &II,
                                                     StringRef Name) {
  IRBuilder<> Builder(&II);

  auto *Ty = II.getType();
  auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
  if (!VTy)
    VTy = IGCLLVM::FixedVectorType::get(Ty, 1);

  auto *Opcode = cast<ConstantInt>(II.getArgOperand(1));
  auto *CacheOpts = vc::InternalIntrinsic::getMemoryCacheControlOperand(&II);
  auto *CacheOptsTy = CacheOpts->getType();

  auto *Pred = II.getArgOperand(0);
  if (auto *PredTy = Pred->getType(); !isa<IGCLLVM::FixedVectorType>(PredTy)) {
    PredTy = IGCLLVM::FixedVectorType::get(PredTy, 1);
    Pred = Builder.CreateBitCast(Pred, PredTy);
  }

  auto *MaskVTy =
      IGCLLVM::FixedVectorType::get(Builder.getInt8Ty(), VTy->getNumElements());
  auto *Mask = Builder.CreateZExt(Pred, MaskVTy);

  auto *Index = II.getArgOperand(6);
  auto *IndexVTy = Index->getType();
  if (!isa<IGCLLVM::FixedVectorType>(IndexVTy))
    IndexVTy = IGCLLVM::FixedVectorType::get(IndexVTy, 1);
  Index = Builder.CreateBitCast(Index, IndexVTy);

  SmallVector<Value *, 12> Args = {
      Mask,
      Opcode,
      CacheOpts,
      II.getArgOperand(5), // base
      Index,
      II.getArgOperand(7), // scale
      II.getArgOperand(8), // offset
  };

  std::transform(II.arg_begin() + 9, II.arg_end(), std::back_inserter(Args),
                 [&](Value *V) { return Builder.CreateBitCast(V, VTy); });

  auto &M = *II.getModule();
  auto *Func = getBuiltinDeclaration(M, Name, false, {VTy, CacheOptsTy});

  auto *NewCI = createLibraryCall(II, Func, Args);
  return Builder.CreateBitCast(NewCI, Ty);
}

static std::string getMangledTypeStr(Type *Ty) {
  std::string Result;
  if (auto *VTy = dyn_cast_or_null<IGCLLVM::FixedVectorType>(Ty))
    Result += "v" + utostr(VTy->getNumElements()) +
              getMangledTypeStr(VTy->getElementType());
  else if (Ty)
    Result += EVT::getEVT(Ty).getEVTString();
  return Result;
}

Function *GenXBuiltinFunctions::getBuiltinDeclaration(Module &M, StringRef Name,
                                                      bool IsFast,
                                                      ArrayRef<Type *> Types,
                                                      StringRef Suffix) {
  std::string FuncName = vc::LibraryFunctionPrefix;
  FuncName += Name;
  if (IsFast)
    FuncName += "_fast";

  for (auto *Ty : Types)
    FuncName += "_" + getMangledTypeStr(Ty);

  FuncName += Suffix;

  auto *Func = M.getFunction(FuncName);
  if (!Func)
    return nullptr;

  // We can only inline the functions before legalization
  bool IsInline = Func->hasFnAttribute(Attribute::AlwaysInline);
  if (Kind == (IsInline ? BuiltinFunctionKind::PostLegalization
                        : BuiltinFunctionKind::PreLegalization))
    return nullptr;

  return Func;
}

std::unique_ptr<Module>
GenXBuiltinFunctions::loadBuiltinLib(LLVMContext &Ctx, const DataLayout &DL,
                                     const std::string &Triple) {
  MemoryBufferRef BiFBuffer =
      getAnalysis<GenXBackendConfig>().getBiFModule(BiFKind::VCBuiltins);

  // NOTE: to simplify LIT testing it is legal to have an empty buffer
  if (BiFBuffer.getBufferSize() == 0)
    return nullptr;

  auto BiFModule = vc::getBiFModuleOrReportError(BiFBuffer, Ctx);

  BiFModule->setDataLayout(DL);
  BiFModule->setTargetTriple(Triple);

  return BiFModule;
}
