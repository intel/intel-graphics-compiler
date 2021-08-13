/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "cmcl/Support/BuiltinTranslator.h"

#include <llvm/GenXIntrinsics/GenXIntrinsics.h>

#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/ErrorHandling.h>

#include "llvmWrapper/IR/DerivedTypes.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <functional>
#include <iterator>
#include <type_traits>
#include <utility>

using namespace llvm;

using FunctionRef = std::reference_wrapper<Function>;
using ValueRef = std::reference_wrapper<Value>;
using InstructionRef = std::reference_wrapper<Instruction>;
using BuiltinSeq = std::vector<FunctionRef>;
using BuiltinCallHandler = std::add_pointer_t<void(CallInst &)>;

constexpr const char BuiltinPrefix[] = "__cm_cl_";

// Imports:
//
// Posible builtin operand kinds:
// namespace OperandKind { enum Enum {.....};}
//
// CMCL builtin IDs. This ID will be used in all other structures to define
// a builtin, e.g. BuiltinID::Select:
// namespace BuiltinID { enum Enum {.....};}
//
// Names of builtin functions:
// constexpr const char* BuiltinNames[] = {.....};
//
// Operand indices for each builtin. 'BuiltinName'Operand::Size holds the
// number of 'BuiltinName' builtin, e.g. SelectOperand::Size:
// namespace 'BuiltinName'Operand { enum Enum {.....};}
//
// Maps builtin ID to builtin operand size:
// constexpr int BuiltinOperandSize[] = {.....};
//
// Maps operand index to its kind for every builtin:
// constexpr OperandKind::Enum 'BuiltinName'OperandKind[] = {.....};
//
// Maps BuiltinID to pointer to corresponding 'BuiltinName'OperandKind array.
// BuiltinOperandKind[BiID][Idx] will return the kind of Idx operand of the
// builtin with BiID ID:
// constexpr const OperandKind::Enum* BuiltinOperandKind[] = {.....};
#define CMCL_AUTOGEN_BUILTIN_DESCS
#include "TranslationInfo.inc"
#undef CMCL_AUTOGEN_BUILTIN_DESCS

static bool isOutputOperand(OperandKind::Enum OpKind) {
  return OpKind == OperandKind::VectorInOut || OpKind == OperandKind::VectorOut;
}

template <BuiltinID::Enum BiID>
static void handleBuiltinCall(CallInst &BiCall);

// Imports:
//
// Maps builtin ID to builtin handler. Builtin handler is a function that will
// translate this builtin:
// constexpr BuiltinCallHandler BuiltinCallHandlers[] = {.....};
//
// Maps builtin ID to ID of intrinsic which this builtin should be translated
// into. Holds ~0u for cases when builtin should be translated not in an
// intrinsic:
// constexpr unsigned IntrinsicForBuiltin[] = {.....};
#define CMCL_AUTOGEN_TRANSLATION_DESCS
#include "TranslationInfo.inc"
#undef CMCL_AUTOGEN_TRANSLATION_DESCS

// Return declaration for intrinsics with provided parameters.
// This is helper function to get genx intrinsic declaration for given
// intrinsic ID, return type and arguments.
// RetTy -- return type of new intrinsic.
// Args -- range of Value * representing new intrinsic arguments. Each value
// must be non-null.
// Id -- new genx intrinsic ID.
// M -- module where to insert function declaration.
//
// NOTE: It was copied from "vc/Utils/GenX/Intrinsics.h" with some changes.
// Cannot link it here because CMCL is a separate independent project.
template <typename Range>
Function *getGenXDeclarationForIdFromArgs(Type &RetTy, Range &&Args,
                                          GenXIntrinsic::ID Id, Module &M) {
  assert(GenXIntrinsic::isGenXIntrinsic(Id) && "Expected genx intrinsic id");

  SmallVector<Type *, 4> Types;
  if (GenXIntrinsic::isOverloadedRet(Id))
    Types.push_back(&RetTy);
  for (auto &&EnumArg : llvm::enumerate(Args)) {
    if (GenXIntrinsic::isOverloadedArg(Id, EnumArg.index()))
      Types.push_back(EnumArg.value()->getType());
  }

  return GenXIntrinsic::getGenXDeclaration(&M, Id, Types);
}

static bool isCMCLBuiltin(const Function &F) {
  return F.getName().startswith(BuiltinPrefix);
}

static BuiltinSeq collectBuiltins(Module &M) {
  BuiltinSeq Builtins;
  std::copy_if(M.begin(), M.end(), std::back_inserter(Builtins),
               [](Function &F) { return isCMCLBuiltin(F); });
  assert(std::all_of(Builtins.begin(), Builtins.end(),
                     [](Function &F) { return F.isDeclaration(); }) &&
         "CM-CL builtins are just function declarations");
  return std::move(Builtins);
}

static void cleanUpBuiltin(Function &F) {
  assert(isCMCLBuiltin(F) && "wrong argument: cm-cl builtin is expected");
  std::vector<InstructionRef> ToErase;
  std::transform(
      F.user_begin(), F.user_end(), std::back_inserter(ToErase),
      [](User *Usr) -> Instruction & { return *cast<Instruction>(Usr); });
  std::for_each(ToErase.begin(), ToErase.end(),
                [](Instruction &I) { I.eraseFromParent(); });
  F.eraseFromParent();
}

static void cleanUpBuiltins(iterator_range<BuiltinSeq::iterator> Builtins) {
  std::for_each(Builtins.begin(), Builtins.end(),
                [](Function &F) { cleanUpBuiltin(F); });
}

static BuiltinID::Enum decodeBuiltin(StringRef BiName) {
  auto BiIt =
      std::find(std::begin(BuiltinNames), std::end(BuiltinNames), BiName);
  assert(BiIt != std::end(BuiltinNames) && "unknown CM-CL builtin");
  return static_cast<BuiltinID::Enum>(BiIt - std::begin(BuiltinNames));
}

static bool isPointerToVector(Type &Ty) {
  if (!Ty.isPointerTy())
    return false;
  auto *PointeeTy = Ty.getPointerElementType();
  return PointeeTy->isVectorTy();
}

// Look for original vector pointer through series of bitcasts.
// For example searchForVectorPointer will return %ptr with %ptr.void.gen
// argument:
//    %ptr = alloca <8 x i32>, align 32
//    %ptr.void = bitcast <8 x i32>* %ptr to i8*
//    %ptr.void.gen = addrspacecast i8* %ptr.void to i8 addrspace(4)*
static Value &searchForVectorPointer(Value &V) {
  assert(V.getType()->isPointerTy() &&
         "wrong argument: the original value must be a pointer");
  Value *CurV;
  for (CurV = &V; !isPointerToVector(*CurV->getType());
       CurV = cast<CastInst>(CurV)->getOperand(0))
    ;
  return *CurV;
}

// Vector operand is passed through void* so we need to look for the real
// pointer to vector first and then load the vector.
static Value &readVectorFromBuiltinOp(Value &BiOp, IRBuilder<> &IRB) {
  auto &Ptr = searchForVectorPointer(BiOp);
  return *IRB.CreateLoad(&Ptr, Ptr.getName() + ".ld.arg");
}

// Output vector operands is also passed as void*.
static void writeVectorFromBuiltinOp(Value &ToWrite, Value &BiOp,
                                     IRBuilder<> &IRB) {
  auto &Ptr = searchForVectorPointer(BiOp);
  IRB.CreateStore(&ToWrite, &Ptr);
}

// Getting vc-intrinsic (or llvm instruction/intrinsic) operand based on cm-cl
// builtin operand.
template <BuiltinID::Enum BiID>
Value &readValueFromBuiltinOp(CallInst &BiCall, int OpIdx, IRBuilder<> &IRB) {
  Value &BiOp = *BiCall.getArgOperand(OpIdx);
  assert(OpIdx < BuiltinOperandSize[BiID] && "operand index is illegal");
  switch (BuiltinOperandKind[BiID][OpIdx]) {
  case OperandKind::VectorOut:
    llvm_unreachable("cannot read value from an output operand");
  case OperandKind::VectorIn:
  case OperandKind::VectorInOut:
    return readVectorFromBuiltinOp(BiOp, IRB);
  case OperandKind::ScalarConst:
    assert((BiOp.getType()->isIntegerTy() ||
            BiOp.getType()->isFloatingPointTy()) &&
           "scalar operand is expected");
    assert(isa<Constant>(BiOp) && "constant operand is expected");
    return BiOp;
  case OperandKind::ScalarIn:
    assert((BiOp.getType()->isIntegerTy() ||
            BiOp.getType()->isFloatingPointTy()) &&
           "scalar operand is expected");
    return BiOp;
  case OperandKind::PointerIn:
    assert(BiOp.getType()->isPointerTy() && "pointer type is expected");
    return BiOp;
  default:
    llvm_unreachable("Unexpected operand kind");
  }
}

// Returns a intended builtin operand type.
// Vector operands are passed through pointer, intended type in this case is
// the vector type, not pointer to vector type.
template <BuiltinID::Enum BiID>
Type &getTypeFromBuiltinOperand(CallInst &BiCall, int OpIdx) {
  assert(OpIdx < BuiltinOperandSize[BiID] && "operand index is illegal");
  Value &BiOp = *BiCall.getArgOperand(OpIdx);
  switch (BuiltinOperandKind[BiID][OpIdx]) {
  case OperandKind::VectorOut:
  case OperandKind::VectorIn:
  case OperandKind::VectorInOut:
    return *searchForVectorPointer(BiOp).getType()->getPointerElementType();
  case OperandKind::ScalarConst:
  case OperandKind::ScalarIn:
  case OperandKind::PointerIn:
    return *BiOp.getType();
  default:
    llvm_unreachable("Unexpected operand kind");
  }
}

// A helper function to get vector type width.
// \p Ty must be a fixed vector type.
static int getVectorWidth(Type &Ty) {
  return cast<IGCLLVM::FixedVectorType>(Ty).getNumElements();
}

// Returns the type wich instruction that a builtin is translated into will
// have.
template <BuiltinID::Enum BiID>
Type &getTranslatedBuiltinType(CallInst &BiCall);

// Prepare vc-intrinsic (or llvm instruction/intrinsic) operands based on
// cm-cl builtin operands.
template <BuiltinID::Enum BiID>
std::vector<Value *> getTranslatedBuiltinOperands(CallInst &BiCall,
                                                  IRBuilder<> &IRB);

// Imports:
//
// getTranslatedBuiltinType specialization for every builtin.
// template <>
// Type &getTranslatedBuiltinType<BuiltinID::'BuiltinName'>(CallInst &BiCall) {
//   return .....;
// }
//
// getTranslatedBuiltinOperands specialization for every builtin.
// template <>
// std::vector<Value *>
// getTranslatedBuiltinOperands<BuiltinID::'BuiltinName'>(CallInst &BiCall,
//                                                        IRBuilder<> &IRB) {
//   return {.....};
// }
#define CMCL_AUTOGEN_TRANSLATION_IMPL
#include "TranslationInfo.inc"
#undef CMCL_AUTOGEN_TRANSLATION_IMPL

// Generates instruction/instructions that represent cm-cl builtin semantics,
// output values (if any) a written into output vector.
// The order of output values are covered in the comment to
// writeBuiltinResults.
// Args:
//    RetTy - type of generated instruction
template <BuiltinID::Enum BiID>
std::vector<ValueRef> createMainInst(const std::vector<Value *> &Operands,
                                     Type &RetTy, IRBuilder<> &IRB) {
  auto *Decl = getGenXDeclarationForIdFromArgs(
      RetTy, Operands,
      static_cast<GenXIntrinsic::ID>(IntrinsicForBuiltin[BiID]),
      *IRB.GetInsertBlock()->getModule());
  auto *CI =
      IRB.CreateCall(Decl, Operands, RetTy.isVoidTy() ? "" : "cmcl.builtin");
  return {*CI};
}

template <>
std::vector<ValueRef>
createMainInst<BuiltinID::Select>(const std::vector<Value *> &Operands, Type &,
                                  IRBuilder<> &IRB) {
  // LLVM select instruction operand indices.
  enum LLVMSelectOperand { Condition, TrueValue, FalseValue };
  // trunc <iW x N> to <i1 x N> for mask
  auto &WrongTypeCond = *Operands[LLVMSelectOperand::Condition];
  auto Width =
      cast<IGCLLVM::FixedVectorType>(WrongTypeCond.getType())->getNumElements();
  auto *CondTy = IGCLLVM::FixedVectorType::get(IRB.getInt1Ty(), Width);
  auto *RightTypeCond = IRB.CreateTrunc(&WrongTypeCond, CondTy,
                                        WrongTypeCond.getName() + ".trunc");
  auto *SelectResult =
      IRB.CreateSelect(RightTypeCond, Operands[LLVMSelectOperand::TrueValue],
                       Operands[LLVMSelectOperand::FalseValue], "cmcl.sel");
  return {*SelectResult};
}

// Writes output values of created "MainInst".
// The order of output values in \p Results:
//    builtin return value if any, output operands in order of builtin
//    arguments (VectorOut, VectorInOut, etc.) if any.
template <BuiltinID::Enum BiID>
void writeBuiltinResults(const std::vector<ValueRef> &Results, CallInst &BiCall,
                         IRBuilder<> &IRB) {
  int ResultIdx = 0;
  // Handle return value.
  if (!BiCall.getType()->isVoidTy()) {
    Results[ResultIdx].get().takeName(&BiCall);
    BiCall.replaceAllUsesWith(&Results[ResultIdx].get());
    ++ResultIdx;
  }

  // Handle output operands.
  for (int BiOpIdx = 0; BiOpIdx != BuiltinOperandSize[BiID]; ++BiOpIdx)
    if (isOutputOperand(BuiltinOperandKind[BiID][BiOpIdx]))
      writeVectorFromBuiltinOp(Results[ResultIdx++],
                               *BiCall.getArgOperand(BiOpIdx), IRB);
}

template <BuiltinID::Enum BiID>
static void handleBuiltinCall(CallInst &BiCall) {
  IRBuilder<> IRB{&BiCall};

  auto Operands = getTranslatedBuiltinOperands<BiID>(BiCall, IRB);
  auto &RetTy = getTranslatedBuiltinType<BiID>(BiCall);
  auto Result = createMainInst<BiID>(Operands, RetTy, IRB);
  writeBuiltinResults<BiID>(Result, BiCall, IRB);
}

static bool handleBuiltin(Function &Builtin) {
  assert(isCMCLBuiltin(Builtin) &&
         "wrong argument: CM-CL builtin was expected");
  if (Builtin.use_empty())
    return false;
  auto BiID = decodeBuiltin(Builtin.getName());
  for (User *Usr : Builtin.users()) {
    assert((BiID >= 0 && BiID < BuiltinID::Size &&
            BuiltinCallHandlers[BiID]) &&
           "no handler for such builtin ID");
    BuiltinCallHandlers[BiID](*cast<CallInst>(Usr));
  }
  return true;
}

bool cmcl::translateBuiltins(Module &M) {
  auto Builtins = collectBuiltins(M);
  if (Builtins.empty())
    return false;
  for (Function &Builtin : Builtins)
    handleBuiltin(Builtin);
  cleanUpBuiltins(Builtins);
  return true;
}
