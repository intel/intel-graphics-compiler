/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "cmcl/Support/BuiltinTranslator.h"
#include "cmcl/Support/AtomicsIface.h"

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
#include "llvmWrapper/IR/IRBuilder.h"

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
  if (GenXIntrinsic::isOverloadedRet(Id)) {
    if (isa<StructType>(RetTy))
      llvm::copy(cast<StructType>(RetTy).elements(), std::back_inserter(Types));
    else
      Types.push_back(&RetTy);
  }
  for (auto &&EnumArg : llvm::enumerate(Args)) {
    if (GenXIntrinsic::isOverloadedArg(Id, EnumArg.index()))
      Types.push_back(EnumArg.value()->getType());
  }

  return GenXIntrinsic::getGenXDeclaration(&M, Id, Types);
}

static bool isCMCLBuiltin(const Function &F) {
  return F.getName().contains(BuiltinPrefix);
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
  auto BiIt = std::find_if(std::begin(BuiltinNames), std::end(BuiltinNames),
                           [BiName](const char *NameFromTable) {
                             return BiName.contains(NameFromTable);
                           });
  assert(BiIt != std::end(BuiltinNames) && "unknown CM-CL builtin");
  return static_cast<BuiltinID::Enum>(BiIt - std::begin(BuiltinNames));
}

// Getting vc-intrinsic (or llvm instruction/intrinsic) operand based on cm-cl
// builtin operand.
template <BuiltinID::Enum BiID>
Value &readValueFromBuiltinOp(CallInst &BiCall, int OpIdx, IRBuilder<> &IRB) {
  Value &BiOp = *BiCall.getArgOperand(OpIdx);
  assert(OpIdx < BuiltinOperandSize[BiID] && "operand index is illegal");
  switch (BuiltinOperandKind[BiID][OpIdx]) {
  case OperandKind::Output:
    llvm_unreachable("cannot read value from an output operand");
  case OperandKind::Constant:
    assert(isa<Constant>(BiOp) && "constant operand is expected");
    return BiOp;
  case OperandKind::Input:
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
  case OperandKind::Output:
    return *BiOp.getType()->getPointerElementType();
  case OperandKind::Input:
  case OperandKind::Constant:
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

// A helper function to get structure type from its element types.
template <typename... ArgTys> Type &getStructureOf(ArgTys &... ElementTys) {
  return *StructType::create("", &ElementTys...);
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
Value &createMainInst(const std::vector<Value *> &Operands, Type &RetTy,
                      IRBuilder<> &IRB) {
  auto *Decl = getGenXDeclarationForIdFromArgs(
      RetTy, Operands,
      static_cast<GenXIntrinsic::ID>(IntrinsicForBuiltin[BiID]),
      *IRB.GetInsertBlock()->getModule());
  auto *CI =
      IRB.CreateCall(Decl, Operands, RetTy.isVoidTy() ? "" : "cmcl.builtin");
  return *CI;
}

// Works only for intrinsics which are overloaded by the return value type.
template <BuiltinID::Enum BiID>
static Value &createLLVMIntrinsic(const std::vector<Value *> &Operands,
                                  Type &RetTy, IRBuilder<> &IRB) {
  auto IID = static_cast<Intrinsic::ID>(IntrinsicForBuiltin[BiID]);
  assert(IID != Intrinsic::not_intrinsic && "Expected LLVM intrinsic");
  Module *M = IRB.GetInsertBlock()->getModule();
  auto *Decl = Intrinsic::getDeclaration(M, IID, {&RetTy});
  return *IRB.CreateCall(Decl, Operands);
}

template <>
Value &createMainInst<BuiltinID::AbsFloat>(const std::vector<Value *> &Operands,
                                           Type &RetTy, IRBuilder<> &IRB) {
  assert(Operands.size() == AbsFloatOperand::Size &&
         "builtin operands should be trasformed into LLVM fabs "
         "intrinsic operands without changes");
  return createLLVMIntrinsic<BuiltinID::AbsFloat>(Operands, RetTy, IRB);
}

//----------------------- Rounding operations ----------------------------//
template <>
Value &createMainInst<BuiltinID::Ceil>(const std::vector<Value *> &Operands,
                                           Type &RetTy, IRBuilder<> &IRB) {
  assert(Operands.size() == CeilOperand::Size &&
         "builtin operands should be trasformed into LLVM ceil "
         "intrinsic operands without changes");
  return createLLVMIntrinsic<BuiltinID::Ceil>(Operands, RetTy, IRB);
}

template <>
Value &createMainInst<BuiltinID::Floor>(const std::vector<Value *> &Operands,
                                            Type &RetTy, IRBuilder<> &IRB) {
  assert(Operands.size() == FloorOperand::Size &&
         "builtin operands should be trasformed into LLVM floor "
         "intrinsic operands without changes");
  return createLLVMIntrinsic<BuiltinID::Floor>(Operands, RetTy, IRB);
}

template <>
Value &createMainInst<BuiltinID::Trunc>(const std::vector<Value *> &Operands,
                                            Type &RetTy, IRBuilder<> &IRB) {
  assert(Operands.size() == TruncOperand::Size &&
         "builtin operands should be trasformed into LLVM trunc "
         "intrinsic operands without changes");
  return createLLVMIntrinsic<BuiltinID::Trunc>(Operands, RetTy, IRB);
}
//------------------------------------------------------------------------//

template <>
Value &createMainInst<BuiltinID::MinNum>(const std::vector<Value *> &Operands,
                                         Type &RetTy, IRBuilder<> &IRB) {
  assert(Operands.size() == MinNumOperand::Size &&
         "builtin operands should be trasformed into LLVM minnum "
         "intrinsic operands without changes");
  return createLLVMIntrinsic<BuiltinID::MinNum>(Operands, RetTy, IRB);
}

template <>
Value &createMainInst<BuiltinID::MaxNum>(const std::vector<Value *> &Operands,
                                         Type &RetTy, IRBuilder<> &IRB) {
  assert(Operands.size() == MaxNumOperand::Size &&
         "builtin operands should be trasformed into LLVM maxnum "
         "intrinsic operands without changes");
  return createLLVMIntrinsic<BuiltinID::MaxNum>(Operands, RetTy, IRB);
}

template <>
Value &createMainInst<BuiltinID::Select>(const std::vector<Value *> &Operands,
                                         Type &, IRBuilder<> &IRB) {
  assert(Operands.size() == SelectOperand::Size &&
         "builtin operands should be trasformed into LLVM select instruction "
         "operands without changes");
  // trunc <iW x N> to <i1 x N> for mask
  auto &WrongTypeCond = *Operands[SelectOperand::Condition];
  auto Width =
      cast<IGCLLVM::FixedVectorType>(WrongTypeCond.getType())->getNumElements();
  auto *CondTy = IGCLLVM::FixedVectorType::get(IRB.getInt1Ty(), Width);
  auto *RightTypeCond = IRB.CreateTrunc(&WrongTypeCond, CondTy,
                                        WrongTypeCond.getName() + ".trunc");
  auto *SelectResult =
      IRB.CreateSelect(RightTypeCond, Operands[SelectOperand::TrueValue],
                       Operands[SelectOperand::FalseValue], "cmcl.sel");
  return *SelectResult;
}

template <>
Value &createMainInst<BuiltinID::Fma>(const std::vector<Value *> &Operands,
                                      Type &RetTy, IRBuilder<> &IRB) {
  assert(Operands.size() == FmaOperand::Size &&
         "builtin operands should be trasformed into LLVM fma "
         "intrinsic operands without changes");
  return createLLVMIntrinsic<BuiltinID::Fma>(Operands, RetTy, IRB);
}

template <>
Value &createMainInst<BuiltinID::Sqrt>(const std::vector<Value *> &Operands,
                                       Type &RetTy, IRBuilder<> &IRB) {
  assert(Operands.size() == SqrtOperand::Size &&
         "builtin operands should be trasformed into LLVM sqrt "
         "intrinsic operands without changes");
  auto *InstSqrt = cast<Instruction>(&createLLVMIntrinsic<BuiltinID::Sqrt>(
      {Operands[SqrtOperand::Source]}, RetTy, IRB));
  if (cast<ConstantInt>(Operands[SqrtOperand::IsFast])->getSExtValue())
    InstSqrt->setFast(true);
  return *InstSqrt;
}

template <>
Value &createMainInst<BuiltinID::Log2>(const std::vector<Value *> &Operands,
                                       Type &RetTy, IRBuilder<> &IRB) {
  assert(Operands.size() == Log2Operand::Size &&
         "builtin operands should be trasformed into LLVM log2 "
         "intrinsic operands without changes");
  auto *InstLog2 = cast<Instruction>(&createLLVMIntrinsic<BuiltinID::Log2>(
      {Operands[Log2Operand::Source]}, RetTy, IRB));
  if (cast<ConstantInt>(Operands[Log2Operand::IsFast])->getSExtValue())
    InstLog2->setFast(true);
  return *InstLog2;
}

template <>
Value &createMainInst<BuiltinID::Exp2>(const std::vector<Value *> &Operands,
                                       Type &RetTy, IRBuilder<> &IRB) {
  assert(Operands.size() == Exp2Operand::Size &&
         "builtin operands should be trasformed into LLVM exp2 "
         "intrinsic operands without changes");
  auto *InstExp2 = cast<Instruction>(&createLLVMIntrinsic<BuiltinID::Exp2>(
      {Operands[Exp2Operand::Source]}, RetTy, IRB));
  if (cast<ConstantInt>(Operands[Exp2Operand::IsFast])->getSExtValue())
    InstExp2->setFast(true);
  return *InstExp2;
}

template <>
Value &createMainInst<BuiltinID::Powr>(const std::vector<Value *> &Operands,
                                       Type &RetTy, IRBuilder<> &IRB) {
  assert(Operands.size() == PowrOperand::Size &&
         "builtin operands should be trasformed into LLVM pow "
         "intrinsic operands without changes");
  std::vector<Value*> Args{ Operands[PowrOperand::Base],
                            Operands[PowrOperand::Exponent] };
  auto *InstPow = cast<Instruction>(&createLLVMIntrinsic<BuiltinID::Powr>(
      Args, RetTy, IRB));
  if (cast<ConstantInt>(Operands[PowrOperand::IsFast])->getSExtValue())
    InstPow->setFast(true);
  return *InstPow;
}

template <>
Value &createMainInst<BuiltinID::Sin>(const std::vector<Value *> &Operands,
                                       Type &RetTy, IRBuilder<> &IRB) {
  assert(Operands.size() == SinOperand::Size &&
         "builtin operands should be trasformed into LLVM sin "
         "intrinsic operands without changes");
  auto *InstSin = cast<Instruction>(&createLLVMIntrinsic<BuiltinID::Sin>(
      {Operands[SinOperand::Source]}, RetTy, IRB));
  if (cast<ConstantInt>(Operands[SinOperand::IsFast])->getSExtValue())
    InstSin->setFast(true);
  return *InstSin;
}

template <>
Value &createMainInst<BuiltinID::Cos>(const std::vector<Value *> &Operands,
                                       Type &RetTy, IRBuilder<> &IRB) {
  assert(Operands.size() == CosOperand::Size &&
         "builtin operands should be trasformed into LLVM cos "
         "intrinsic operands without changes");
  auto *InstCos = cast<Instruction>(&createLLVMIntrinsic<BuiltinID::Cos>(
      {Operands[CosOperand::Source]}, RetTy, IRB));
  if (cast<ConstantInt>(Operands[CosOperand::IsFast])->getSExtValue())
    InstCos->setFast(true);
  return *InstCos;
}

using CMCLSemantics = cmcl::atomic::MemorySemantics::Enum;
using CMCLMemoryScope = cmcl::atomic::MemoryScope::Enum;
using CMCLOperation = cmcl::atomic::Operation::Enum;

static AtomicOrdering getLLVMAtomicOrderingFromCMCL(CMCLSemantics S) {
  switch (S) {
  case CMCLSemantics::Relaxed:
    return AtomicOrdering::Monotonic;
  case CMCLSemantics::Acquire:
    return AtomicOrdering::Acquire;
  case CMCLSemantics::Release:
    return AtomicOrdering::Release;
  case CMCLSemantics::AcquireRelease:
    return AtomicOrdering::AcquireRelease;
  case CMCLSemantics::SequentiallyConsistent:
    return AtomicOrdering::SequentiallyConsistent;
  }
  llvm_unreachable("unhandled cmcl semantics");
}

static AtomicRMWInst::BinOp getLLVMAtomicBinOpFromCMCL(CMCLOperation Op) {
  switch (Op) {
  default:
    llvm_unreachable("unexpected cmcl binary op");
  case CMCLOperation::MinSInt:
    return AtomicRMWInst::Min;
  case CMCLOperation::Xchg:
    return AtomicRMWInst::Xchg;
  case CMCLOperation::MaxSInt:
    return AtomicRMWInst::Max;
  case CMCLOperation::Min:
    return AtomicRMWInst::UMin;
  case CMCLOperation::Max:
    return AtomicRMWInst::UMax;
  case CMCLOperation::Add:
    return AtomicRMWInst::Add;
  case CMCLOperation::Sub:
    return AtomicRMWInst::Sub;
  case CMCLOperation::Orl:
    return AtomicRMWInst::Or;
  case CMCLOperation::Xorl:
    return AtomicRMWInst::Xor;
  case CMCLOperation::Andl:
    return AtomicRMWInst::And;
  }
}

template <>
Value &
createMainInst<BuiltinID::AtomicRMW>(const std::vector<Value *> &Operands,
                                     Type &, IRBuilder<> &IRB) {
  assert(Operands.size() == AtomicRMWOperand::Size &&
         "builtin operands should be trasformed into LLVM atomicrmw "
         "instruction operands without changes");
  auto &Ctx = IRB.getContext();
  auto *Ptr = Operands[AtomicRMWOperand::Ptr];
  auto Ordering = getLLVMAtomicOrderingFromCMCL(static_cast<CMCLSemantics>(
      cast<ConstantInt>(Operands[AtomicRMWOperand::Semantics])
          ->getZExtValue()));
  auto ScopeName = cmcl::atomic::MemoryScope::getScopeNameFromCMCL(
      static_cast<CMCLMemoryScope>(
          cast<ConstantInt>(Operands[AtomicRMWOperand::Scope])
              ->getZExtValue()));
  auto BinOp = getLLVMAtomicBinOpFromCMCL(static_cast<CMCLOperation>(
      cast<ConstantInt>(Operands[AtomicRMWOperand::Operation])
          ->getSExtValue()));
  return *IGCLLVM::createAtomicRMW(
      IRB, BinOp, Ptr, Operands[AtomicRMWOperand::Operand], Ordering,
      Ctx.getOrInsertSyncScopeID(ScopeName));
}

template <>
Value &createMainInst<BuiltinID::CmpXchg>(const std::vector<Value *> &Operands,
                                          Type &, IRBuilder<> &IRB) {
  assert(Operands.size() == CmpXchgOperand::Size &&
         "builtin operands should be trasformed into LLVM cmpxchg "
         "instruction operands without changes");
  auto *Ptr = Operands[CmpXchgOperand::Ptr];
  auto &Ctx = IRB.getContext();
  auto OrderingSuccess =
      getLLVMAtomicOrderingFromCMCL(static_cast<CMCLSemantics>(
          cast<ConstantInt>(Operands[CmpXchgOperand::SemanticsSuccess])
              ->getZExtValue()));
  auto OrderingFalilure =
      getLLVMAtomicOrderingFromCMCL(static_cast<CMCLSemantics>(
          cast<ConstantInt>(Operands[CmpXchgOperand::SemanticsFailure])
              ->getZExtValue()));
  auto ScopeName = cmcl::atomic::MemoryScope::getScopeNameFromCMCL(
      static_cast<CMCLMemoryScope>(
          cast<ConstantInt>(Operands[CmpXchgOperand::Scope])->getZExtValue()));
  auto *CmpXchgInst = IGCLLVM::createAtomicCmpXchg(
      IRB, Ptr, Operands[CmpXchgOperand::Operand0],
      Operands[CmpXchgOperand::Operand1], OrderingSuccess, OrderingFalilure,
      Ctx.getOrInsertSyncScopeID(ScopeName));
  return *IRB.CreateExtractValue(CmpXchgInst, 0 /*CmpXchg result*/,
                                 ".cmpxchg.res");
}

// Produces a vector of main inst results from its value.
// For multiple output an intrinsic may return a structure. This function will
// extract all structure elements and put them in index order into resulting
// vector.
static std::vector<ValueRef> splitMainInstResult(Value &CombinedResult,
                                                 IRBuilder<> &IRB) {
  if (!isa<StructType>(CombinedResult.getType()))
    return {CombinedResult};
  auto *ResTy = cast<StructType>(CombinedResult.getType());
  std::vector<ValueRef> Results;
  for (int Idx = 0; Idx != ResTy->getNumElements(); ++Idx)
    Results.push_back(
        *IRB.CreateExtractValue(&CombinedResult, Idx, "cmcl.extract.res"));
  return Results;
}

// Writes output values of created "MainInst".
// The order of output values in \p Results:
//    builtin return value if any, output operands in order of builtin
//    arguments (VectorOut, VectorInOut, etc.) if any.
template <BuiltinID::Enum BiID>
void writeBuiltinResults(Value &CombinedResult, CallInst &BiCall,
                         IRBuilder<> &IRB) {
  auto Results = splitMainInstResult(CombinedResult, IRB);
  int ResultIdx = 0;
  // Handle return value.
  if (!BiCall.getType()->isVoidTy()) {
    Results[ResultIdx].get().takeName(&BiCall);
    BiCall.replaceAllUsesWith(&Results[ResultIdx].get());
    ++ResultIdx;
  }

  // Handle output operands.
  for (int BiOpIdx = 0; BiOpIdx != BuiltinOperandSize[BiID]; ++BiOpIdx)
    if (BuiltinOperandKind[BiID][BiOpIdx] == OperandKind::Output)
      IRB.CreateStore(&Results[ResultIdx++].get(),
                      BiCall.getArgOperand(BiOpIdx));
}

template <BuiltinID::Enum BiID>
static void handleBuiltinCall(CallInst &BiCall) {
  IRBuilder<> IRB{&BiCall};

  auto Operands = getTranslatedBuiltinOperands<BiID>(BiCall, IRB);
  auto &RetTy = getTranslatedBuiltinType<BiID>(BiCall);
  auto &Result = createMainInst<BiID>(Operands, RetTy, IRB);
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
