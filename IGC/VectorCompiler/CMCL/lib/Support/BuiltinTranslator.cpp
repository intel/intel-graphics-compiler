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

#include "TranslationInfo.inc"

static bool isOutputOperand(OperandKind::Enum OpKind) {
  return OpKind == OperandKind::VectorInOut || OpKind == OperandKind::VectorOut;
}

template <BuiltinID::Enum BiID>
static void handleBuiltinCall(CallInst &BiCall);

constexpr BuiltinCallHandler BuiltinCallHandlers[] = {
    handleBuiltinCall<BuiltinID::Select>,
    handleBuiltinCall<BuiltinID::RdRegionInt>,
    handleBuiltinCall<BuiltinID::RdRegionFloat>,
    handleBuiltinCall<BuiltinID::WrRegionInt>,
    handleBuiltinCall<BuiltinID::WrRegionFloat>,
    handleBuiltinCall<BuiltinID::PrintfBuffer>,
    handleBuiltinCall<BuiltinID::PrintfFormatIndex>,
    handleBuiltinCall<BuiltinID::PrintfFormatIndexLegacy>,
    handleBuiltinCall<BuiltinID::SVMScatter>,
    handleBuiltinCall<BuiltinID::SVMAtomicAdd>};

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
Value &readValueFromBuiltinOp(Use &BiOp, IRBuilder<> &IRB) {
  int OpIdx = BiOp.getOperandNo();
  assert(OpIdx < BuiltinOperandSize[BiID] && "operand index is illegal");
  switch (BuiltinOperandKind[BiID][OpIdx]) {
  case OperandKind::VectorOut:
    // Need to know the real vector type to mangle some intrinsics,
    // so we pass this info through operands to createMainInst.
    // FIXME: don't look at output operands here at all.
    return searchForVectorPointer(*BiOp.get());
  case OperandKind::VectorIn:
  case OperandKind::VectorInOut:
    return readVectorFromBuiltinOp(*BiOp.get(), IRB);
  case OperandKind::ScalarConst:
    assert((BiOp.get()->getType()->isIntegerTy() ||
            BiOp.get()->getType()->isFloatingPointTy()) &&
           "scalar operand is expected");
    assert(isa<Constant>(BiOp.get()) && "constant operand is expected");
    return *BiOp.get();
  case OperandKind::ScalarIn:
    assert((BiOp.get()->getType()->isIntegerTy() ||
            BiOp.get()->getType()->isFloatingPointTy()) &&
           "scalar operand is expected");
    return *BiOp.get();
  case OperandKind::PointerIn:
    assert(BiOp.get()->getType()->isPointerTy() && "pointer type is expected");
    return *BiOp.get();
  default:
    assert(0 && "Unexpected operand kind");
    return *BiOp.get();
  }
}

// Prepare vc-intrinsic (or llvm instruction/intrinsic) operands based on
// cm-cl builtin operands.
template <BuiltinID::Enum BiID>
static std::vector<ValueRef> readOperands(CallInst &BiCall, IRBuilder<> &IRB) {
  assert(BiCall.getNumArgOperands() == BuiltinOperandSize[BiID] &&
         "wrong number of operands for select");
  std::vector<ValueRef> LegalOps;
  std::transform(BiCall.arg_begin(), BiCall.arg_end(),
                 std::back_inserter(LegalOps), [&IRB](Use &U) -> Value & {
                   return readValueFromBuiltinOp<BiID>(U, IRB);
                 });
  return LegalOps;
}

// Generates instruction/instructions that represent cm-cl builtin semantics,
// output values (if any) a written into output vector.
// The order of output values are covered in the comment to
// writeBuiltinResults.
// FIXME: find a way to autogenerate it.
template <BuiltinID::Enum BiID>
std::vector<ValueRef> createMainInst(const std::vector<ValueRef> &Operands,
                                     Type &BiTy, IRBuilder<> &IRB);

template <>
std::vector<ValueRef>
createMainInst<BuiltinID::Select>(const std::vector<ValueRef> &Operands, Type &,
                                  IRBuilder<> &IRB) {

  // trunc <iW x N> to <i1 x N> for mask
  auto &WrongTypeCond = Operands[SelectOperand::Condition].get();
  auto Width =
      cast<IGCLLVM::FixedVectorType>(WrongTypeCond.getType())->getNumElements();
  auto *CondTy = IGCLLVM::FixedVectorType::get(IRB.getInt1Ty(), Width);
  auto *RightTypeCond = IRB.CreateTrunc(&WrongTypeCond, CondTy,
                                        WrongTypeCond.getName() + ".trunc");
  auto *SelectResult =
      IRB.CreateSelect(RightTypeCond, &Operands[SelectOperand::TrueValue].get(),
                       &Operands[SelectOperand::FalseValue].get(), "cmcl.sel");
  return {*SelectResult};
}

template <>
std::vector<ValueRef>
createMainInst<BuiltinID::RdRegionInt>(const std::vector<ValueRef> &Operands,
                                       Type &, IRBuilder<> &IRB) {
  Type *RetTy = Operands[RdRegionIntOperand::Destination]
                    .get()
                    .getType()
                    ->getPointerElementType();
  Value *Args[] = {&Operands[RdRegionIntOperand::Source].get(),
                   &Operands[RdRegionIntOperand::VStride].get(),
                   &Operands[RdRegionIntOperand::Width].get(),
                   &Operands[RdRegionIntOperand::Stride].get(),
                   &Operands[RdRegionIntOperand::Offset].get(),
                   UndefValue::get(IRB.getInt32Ty())};
  auto *Decl = getGenXDeclarationForIdFromArgs(
      *RetTy, Args, GenXIntrinsic::genx_rdregioni,
      *IRB.GetInsertBlock()->getModule());
  auto *RdR = IRB.CreateCall(Decl, Args, "cmcl.rdr");
  return {*RdR};
}

template <>
std::vector<ValueRef>
createMainInst<BuiltinID::RdRegionFloat>(const std::vector<ValueRef> &Operands,
                                         Type &, IRBuilder<> &IRB) {
  Type *RetTy = Operands[RdRegionFloatOperand::Destination]
                    .get()
                    .getType()
                    ->getPointerElementType();
  Value *Args[] = {&Operands[RdRegionFloatOperand::Source].get(),
                   &Operands[RdRegionFloatOperand::VStride].get(),
                   &Operands[RdRegionFloatOperand::Width].get(),
                   &Operands[RdRegionFloatOperand::Stride].get(),
                   &Operands[RdRegionFloatOperand::Offset].get(),
                   UndefValue::get(IRB.getInt32Ty())};
  auto *Decl = getGenXDeclarationForIdFromArgs(
      *RetTy, Args, GenXIntrinsic::genx_rdregionf,
      *IRB.GetInsertBlock()->getModule());
  auto *RdR = IRB.CreateCall(Decl, Args, "cmcl.rdr");
  return {*RdR};
}

template <>
std::vector<ValueRef>
createMainInst<BuiltinID::WrRegionInt>(const std::vector<ValueRef> &Operands,
                                       Type &, IRBuilder<> &IRB) {
  Type *RetTy = Operands[WrRegionIntOperand::Destination].get().getType();
  Value *Args[] = {&Operands[WrRegionIntOperand::Destination].get(),
                   &Operands[WrRegionIntOperand::Source].get(),
                   &Operands[WrRegionIntOperand::VStride].get(),
                   &Operands[WrRegionIntOperand::Width].get(),
                   &Operands[WrRegionIntOperand::Stride].get(),
                   &Operands[WrRegionIntOperand::Offset].get(),
                   UndefValue::get(IRB.getInt32Ty()),
                   IRB.getTrue()};
  auto *Decl = getGenXDeclarationForIdFromArgs(
      *RetTy, Args, GenXIntrinsic::genx_wrregioni,
      *IRB.GetInsertBlock()->getModule());
  auto *WrR = IRB.CreateCall(Decl, Args, "cmcl.wrr");
  return {*WrR};
}

template <>
std::vector<ValueRef>
createMainInst<BuiltinID::WrRegionFloat>(const std::vector<ValueRef> &Operands,
                                         Type &, IRBuilder<> &IRB) {
  Type *RetTy = Operands[WrRegionFloatOperand::Destination].get().getType();
  Value *Args[] = {&Operands[WrRegionFloatOperand::Destination].get(),
                   &Operands[WrRegionFloatOperand::Source].get(),
                   &Operands[WrRegionFloatOperand::VStride].get(),
                   &Operands[WrRegionFloatOperand::Width].get(),
                   &Operands[WrRegionFloatOperand::Stride].get(),
                   &Operands[WrRegionFloatOperand::Offset].get(),
                   UndefValue::get(IRB.getInt32Ty()),
                   IRB.getTrue()};
  auto *Decl = getGenXDeclarationForIdFromArgs(
      *RetTy, Args, GenXIntrinsic::genx_wrregionf,
      *IRB.GetInsertBlock()->getModule());
  auto *WrR = IRB.CreateCall(Decl, Args, "cmcl.wrr");
  return {*WrR};
}

template <>
std::vector<ValueRef>
createMainInst<BuiltinID::PrintfBuffer>(const std::vector<ValueRef> &,
                                        Type &BiTy, IRBuilder<> &IRB) {
  Type *RetTy = IRB.getInt64Ty();
  std::array<Value *, 0> Args;
  auto *Decl = getGenXDeclarationForIdFromArgs(
      *RetTy, Args, GenXIntrinsic::genx_print_buffer,
      *IRB.GetInsertBlock()->getModule());
  auto *IntPtr = IRB.CreateCall(Decl, Args, "cmcl.print.buffer");
  return {*IntPtr};
}

template <>
std::vector<ValueRef> createMainInst<BuiltinID::PrintfFormatIndex>(
    const std::vector<ValueRef> &Operands, Type &BiTy, IRBuilder<> &IRB) {
  Type *RetTy = IRB.getInt32Ty();
  Value *Args[] = {&Operands[PrintfFormatIndexOperand::Source].get()};
  auto *Decl = getGenXDeclarationForIdFromArgs(
      *RetTy, Args, GenXIntrinsic::genx_print_format_index,
      *IRB.GetInsertBlock()->getModule());
  auto *Idx = IRB.CreateCall(Decl, Args);
  return {*Idx};
}

template <>
std::vector<ValueRef> createMainInst<BuiltinID::PrintfFormatIndexLegacy>(
    const std::vector<ValueRef> &Operands, Type &BiTy, IRBuilder<> &IRB) {
  return createMainInst<BuiltinID::PrintfFormatIndex>(Operands, BiTy, IRB);
}

template <>
std::vector<ValueRef>
createMainInst<BuiltinID::SVMScatter>(const std::vector<ValueRef> &Operands,
                                      Type &BiTy, IRBuilder<> &IRB) {
  Type *RetTy = IRB.getVoidTy();
  auto Width = cast<IGCLLVM::FixedVectorType>(
                   Operands[SVMScatterOperand::Address].get().getType())
                   ->getNumElements();
  Value *Args[] = {IRB.CreateVectorSplat(Width, IRB.getInt1(true)),
                   &Operands[SVMScatterOperand::NumBlocks].get(),
                   &Operands[SVMScatterOperand::Address].get(),
                   &Operands[SVMScatterOperand::Source].get()};
  auto *Decl = getGenXDeclarationForIdFromArgs(
      *RetTy, Args, GenXIntrinsic::genx_svm_scatter,
      *IRB.GetInsertBlock()->getModule());
  IRB.CreateCall(Decl, Args);
  return {};
}

template <>
std::vector<ValueRef>
createMainInst<BuiltinID::SVMAtomicAdd>(const std::vector<ValueRef> &Operands,
                                        Type &, IRBuilder<> &IRB) {
  Type *RetTy = Operands[SVMAtomicAddOperand::Destination]
                    .get()
                    .getType()
                    ->getPointerElementType();
  auto Width = cast<IGCLLVM::FixedVectorType>(
                   Operands[SVMAtomicAddOperand::Address].get().getType())
                   ->getNumElements();
  Value *Args[] = {IRB.CreateVectorSplat(Width, IRB.getInt1(true)),
                   &Operands[SVMAtomicAddOperand::Address].get(),
                   &Operands[SVMAtomicAddOperand::Source].get(),
                   UndefValue::get(RetTy)};
  auto *Decl = getGenXDeclarationForIdFromArgs(
      *RetTy, Args, GenXIntrinsic::genx_svm_atomic_add,
      *IRB.GetInsertBlock()->getModule());
  auto *Result = IRB.CreateCall(Decl, Args, "cmcl.svm.atomic.add");
  return {*Result};
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

  auto Operands = readOperands<BiID>(BiCall, IRB);
  auto Result = createMainInst<BiID>(Operands, *BiCall.getType(), IRB);
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
