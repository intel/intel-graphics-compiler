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
#include <cassert>
#include <functional>
#include <iterator>
#include <utility>

using namespace llvm;

using FunctionRef = std::reference_wrapper<Function>;
using ValueRef = std::reference_wrapper<Value>;
using InstructionRef = std::reference_wrapper<Instruction>;
using BuiltinSeq = std::vector<FunctionRef>;

namespace cmcl {

constexpr const char BuiltinPrefix[] = "__cm_cl_";

// FIXME: autogenerate the most of the boilerplate below.

namespace BuiltinID {
enum Enum {
  Select,
  RdRegion,
  WrRegion,
  PrintfBuffer,
  PrintfFormatIndex,
  PrintfFormatIndexLegacy,
  SVMScatter,
  SVMAtomicAdd,
  Size
};
} // namespace BuiltinID

constexpr const char *BuiltinNames[BuiltinID::Size] = {
    "__cm_cl_select",
    "__cm_cl_rdregion",
    "__cm_cl_wrregion",
    "__cm_cl_printf_buffer",
    "__cm_cl_printf_format_index",
    "__cm_cl_printf_format_index_legacy",
    "__cm_cl_svm_scatter",
    "__cm_cl_svm_atomic_add"};

namespace OperandKind {
enum Enum {
  VectorIn,
  VectorOut,
  VectorInOut,
  ScalarIn,
  ScalarConst,
  PointerIn
};
} // namespace OperandKind

static bool isOutputOperand(OperandKind::Enum OpKind) {
  return OpKind == OperandKind::VectorInOut || OpKind == OperandKind::VectorOut;
}

namespace SelectOperand {
enum Enum { Destination, Condition, TrueValue, FalseValue, Size };
} // namespace SelectOperand

namespace RdRegionOperand {
enum Enum { Destination, Source, VStride, Width, Stride, Offset, Size };
} // namespace RdRegionOperand

namespace WrRegionOperand {
enum Enum { Destination, Source, VStride, Width, Stride, Offset, Size };
} // namespace WrRegionOperand

namespace PrintfBufferOperand {
enum Enum { Size };
} // namespace PrintfBufferOperand

namespace PrintfFormatIndexOperand {
enum Enum { Source, Size };
} // namespace PrintfFormatIndexOperand

namespace SVMScatterOperand {
enum Enum { NumBlocks, Address, Source, Size };
} // namespace SVMScatterOperand

namespace SVMAtomicAddOperand {
enum Enum { Destination, Address, Source, Size };
} // namespace SVMAtomicAddOperand

constexpr OperandKind::Enum SelectOperandKind[SelectOperand::Size] = {
    OperandKind::VectorOut, OperandKind::VectorIn, OperandKind::VectorIn,
    OperandKind::VectorIn};

constexpr OperandKind::Enum RdRegionOperandKind[RdRegionOperand::Size] = {
    OperandKind::VectorOut,   OperandKind::VectorIn,
    OperandKind::ScalarConst, OperandKind::ScalarConst,
    OperandKind::ScalarConst, OperandKind::ScalarIn};

constexpr OperandKind::Enum WrRegionOperandKind[WrRegionOperand::Size] = {
    OperandKind::VectorInOut, OperandKind::VectorIn,
    OperandKind::ScalarConst, OperandKind::ScalarConst,
    OperandKind::ScalarConst, OperandKind::ScalarIn};

constexpr OperandKind::Enum
    PrintfFormatIndexOperandKind[PrintfFormatIndexOperand::Size] = {
        OperandKind::PointerIn};

constexpr OperandKind::Enum SVMScatterOperandKind[SVMScatterOperand::Size] = {
    OperandKind::ScalarConst, OperandKind::VectorIn, OperandKind::VectorIn};

constexpr OperandKind::Enum SVMAtomicAddOperandKind[SVMAtomicAddOperand::Size] =
    {OperandKind::VectorOut, OperandKind::VectorIn, OperandKind::VectorIn};

constexpr const OperandKind::Enum *BuiltinOperandKind[BuiltinID::Size] = {
    SelectOperandKind,
    RdRegionOperandKind,
    WrRegionOperandKind,
    nullptr,
    PrintfFormatIndexOperandKind,
    PrintfFormatIndexOperandKind, // Legacy
    SVMScatterOperandKind,
    SVMAtomicAddOperandKind};

constexpr int BuiltinOperandSize[BuiltinID::Size] = {
    SelectOperand::Size,
    RdRegionOperand::Size,
    WrRegionOperand::Size,
    PrintfBufferOperand::Size,
    PrintfFormatIndexOperand::Size,
    PrintfFormatIndexOperand::Size, // Legacy
    SVMScatterOperand::Size,
    SVMAtomicAddOperand::Size};

} // namespace cmcl

static bool isCMCLBuiltin(const Function &F) {
  return F.getName().startswith(cmcl::BuiltinPrefix);
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

static cmcl::BuiltinID::Enum decodeBuiltin(StringRef BiName) {
  auto BiIt = std::find(std::begin(cmcl::BuiltinNames),
                        std::end(cmcl::BuiltinNames), BiName);
  assert(BiIt != std::end(cmcl::BuiltinNames) && "unknown CM-CL builtin");
  return static_cast<cmcl::BuiltinID::Enum>(BiIt -
                                            std::begin(cmcl::BuiltinNames));
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
template <cmcl::BuiltinID::Enum BiID>
Value &readValueFromBuiltinOp(Use &BiOp, IRBuilder<> &IRB) {
  int OpIdx = BiOp.getOperandNo();
  assert(OpIdx < cmcl::BuiltinOperandSize[BiID] && "operand index is illegal");
  switch (cmcl::BuiltinOperandKind[BiID][OpIdx]) {
  case cmcl::OperandKind::VectorOut:
    // Need to know the real vector type to mangle some intrinsics,
    // so we pass this info through operands to createMainInst.
    // FIXME: don't look at output operands here at all.
    return searchForVectorPointer(*BiOp.get());
  case cmcl::OperandKind::VectorIn:
  case cmcl::OperandKind::VectorInOut:
    return readVectorFromBuiltinOp(*BiOp.get(), IRB);
  case cmcl::OperandKind::ScalarConst:
    assert((BiOp.get()->getType()->isIntegerTy() ||
            BiOp.get()->getType()->isFloatingPointTy()) &&
           "scalar operand is expected");
    assert(isa<Constant>(BiOp.get()) && "constant operand is expected");
    return *BiOp.get();
  case cmcl::OperandKind::ScalarIn:
    assert((BiOp.get()->getType()->isIntegerTy() ||
            BiOp.get()->getType()->isFloatingPointTy()) &&
           "scalar operand is expected");
    return *BiOp.get();
  case cmcl::OperandKind::PointerIn:
    assert(BiOp.get()->getType()->isPointerTy() && "pointer type is expected");
    return *BiOp.get();
  default:
    assert(0 && "Unexpected operand kind");
    return *BiOp.get();
  }
}

// Prepare vc-intrinsic (or llvm instruction/intrinsic) operands based on
// cm-cl builtin operands.
template <cmcl::BuiltinID::Enum BiID>
static std::vector<ValueRef> readOperands(CallInst &BiCall, IRBuilder<> &IRB) {
  assert(BiCall.getNumArgOperands() == cmcl::BuiltinOperandSize[BiID] &&
         "wrong number of operands for select");
  std::vector<ValueRef> LegalOps;
  std::transform(BiCall.arg_begin(), BiCall.arg_end(),
                 std::back_inserter(LegalOps), [&IRB](Use &U) -> Value & {
                   return readValueFromBuiltinOp<BiID>(U, IRB);
                 });
  return LegalOps;
}

using BuiltinCallHandler = std::function<void(CallInst &)>;
using BuiltinCallHandlerSeq = std::vector<BuiltinCallHandler>;

// Generates instruction/instructions that represent cm-cl builtin semantics,
// output values (if any) a written into output vector.
// The order of output values are covered in the comment to
// writeBuiltinResults.
// FIXME: find a way to autogenerate it.
template <cmcl::BuiltinID::Enum BiID>
std::vector<ValueRef> createMainInst(const std::vector<ValueRef> &Operands,
                                     Type &BiTy, IRBuilder<> &IRB);

template <>
std::vector<ValueRef>
createMainInst<cmcl::BuiltinID::Select>(const std::vector<ValueRef> &Operands,
                                        Type &, IRBuilder<> &IRB) {

  // trunc <iW x N> to <i1 x N> for mask
  auto &WrongTypeCond = Operands[cmcl::SelectOperand::Condition].get();
  auto Width =
      cast<IGCLLVM::FixedVectorType>(WrongTypeCond.getType())->getNumElements();
  auto *CondTy = IGCLLVM::FixedVectorType::get(IRB.getInt1Ty(), Width);
  auto *RightTypeCond = IRB.CreateTrunc(&WrongTypeCond, CondTy,
                                        WrongTypeCond.getName() + ".trunc");
  auto *SelectResult = IRB.CreateSelect(
      RightTypeCond, &Operands[cmcl::SelectOperand::TrueValue].get(),
      &Operands[cmcl::SelectOperand::FalseValue].get(), "cmcl.sel");
  return {*SelectResult};
}

template <>
std::vector<ValueRef>
createMainInst<cmcl::BuiltinID::RdRegion>(const std::vector<ValueRef> &Operands,
                                          Type &, IRBuilder<> &IRB) {
  auto IID = Operands[cmcl::RdRegionOperand::Source]
                     .get()
                     .getType()
                     ->getScalarType()
                     ->isFloatingPointTy()
                 ? GenXIntrinsic::genx_rdregionf
                 : GenXIntrinsic::genx_rdregioni;
  Type *Tys[] = {Operands[cmcl::RdRegionOperand::Destination]
                     .get()
                     .getType()
                     ->getPointerElementType(),
                 Operands[cmcl::RdRegionOperand::Source].get().getType(),
                 Operands[cmcl::RdRegionOperand::Offset].get().getType()};
  auto *Decl = GenXIntrinsic::getGenXDeclaration(
      IRB.GetInsertBlock()->getParent()->getParent(), IID, Tys);
  Value *Args[] = {&Operands[cmcl::RdRegionOperand::Source].get(),
                   &Operands[cmcl::RdRegionOperand::VStride].get(),
                   &Operands[cmcl::RdRegionOperand::Width].get(),
                   &Operands[cmcl::RdRegionOperand::Stride].get(),
                   &Operands[cmcl::RdRegionOperand::Offset].get(),
                   UndefValue::get(IRB.getInt32Ty())};
  auto *RdR = IRB.CreateCall(Decl, Args, "cmcl.rdr");
  return {*RdR};
}

template <>
std::vector<ValueRef>
createMainInst<cmcl::BuiltinID::WrRegion>(const std::vector<ValueRef> &Operands,
                                          Type &, IRBuilder<> &IRB) {
  auto IID = Operands[cmcl::WrRegionOperand::Source]
                     .get()
                     .getType()
                     ->getScalarType()
                     ->isFloatingPointTy()
                 ? GenXIntrinsic::genx_wrregionf
                 : GenXIntrinsic::genx_wrregioni;
  Type *Tys[] = {Operands[cmcl::WrRegionOperand::Destination].get().getType(),
                 Operands[cmcl::WrRegionOperand::Source].get().getType(),
                 Operands[cmcl::WrRegionOperand::Offset].get().getType(),
                 IRB.getInt1Ty()};
  auto *Decl = GenXIntrinsic::getGenXDeclaration(
      IRB.GetInsertBlock()->getParent()->getParent(), IID, Tys);
  Value *Args[] = {&Operands[cmcl::WrRegionOperand::Destination].get(),
                   &Operands[cmcl::WrRegionOperand::Source].get(),
                   &Operands[cmcl::WrRegionOperand::VStride].get(),
                   &Operands[cmcl::WrRegionOperand::Width].get(),
                   &Operands[cmcl::WrRegionOperand::Stride].get(),
                   &Operands[cmcl::WrRegionOperand::Offset].get(),
                   UndefValue::get(IRB.getInt32Ty()),
                   IRB.getTrue()};
  auto *WrR = IRB.CreateCall(Decl, Args, "cmcl.wrr");
  return {*WrR};
}

template <>
std::vector<ValueRef>
createMainInst<cmcl::BuiltinID::PrintfBuffer>(const std::vector<ValueRef> &,
                                              Type &BiTy, IRBuilder<> &IRB) {
  auto *Decl = GenXIntrinsic::getGenXDeclaration(
      IRB.GetInsertBlock()->getParent()->getParent(),
      GenXIntrinsic::genx_print_buffer, {});
  auto *IntPtr = IRB.CreateCall(Decl, {}, "cmcl.print.buffer");
  auto *Ptr = IRB.CreateIntToPtr(IntPtr, &BiTy);
  return {*Ptr};
}

template <>
std::vector<ValueRef> createMainInst<cmcl::BuiltinID::PrintfFormatIndex>(
    const std::vector<ValueRef> &Operands, Type &BiTy, IRBuilder<> &IRB) {
  auto &StrPtr = Operands[cmcl::PrintfFormatIndexOperand::Source].get();
  auto *Decl = GenXIntrinsic::getGenXDeclaration(
      IRB.GetInsertBlock()->getParent()->getParent(),
      GenXIntrinsic::genx_print_format_index, StrPtr.getType());
  auto *Idx = IRB.CreateCall(Decl, &StrPtr);
  return {*Idx};
}

template <>
std::vector<ValueRef> createMainInst<cmcl::BuiltinID::PrintfFormatIndexLegacy>(
    const std::vector<ValueRef> &Operands, Type &BiTy, IRBuilder<> &IRB) {
  return createMainInst<cmcl::BuiltinID::PrintfFormatIndex>(Operands, BiTy,
                                                            IRB);
}

template <>
std::vector<ValueRef> createMainInst<cmcl::BuiltinID::SVMScatter>(
    const std::vector<ValueRef> &Operands, Type &BiTy, IRBuilder<> &IRB) {
  auto Width = cast<IGCLLVM::FixedVectorType>(
                   Operands[cmcl::SVMScatterOperand::Address].get().getType())
                   ->getNumElements();
  Type *Tys[] = {IGCLLVM::FixedVectorType::get(IRB.getInt1Ty(), Width),
                 Operands[cmcl::SVMScatterOperand::Address].get().getType(),
                 Operands[cmcl::SVMScatterOperand::Source].get().getType()};
  auto *Decl = GenXIntrinsic::getGenXDeclaration(
      IRB.GetInsertBlock()->getParent()->getParent(),
      GenXIntrinsic::genx_svm_scatter, Tys);
  Value *Args[] = {IRB.CreateVectorSplat(Width, IRB.getInt1(true)),
                   &Operands[cmcl::SVMScatterOperand::NumBlocks].get(),
                   &Operands[cmcl::SVMScatterOperand::Address].get(),
                   &Operands[cmcl::SVMScatterOperand::Source].get()};
  IRB.CreateCall(Decl, Args);
  return {};
}

template <>
std::vector<ValueRef> createMainInst<cmcl::BuiltinID::SVMAtomicAdd>(
    const std::vector<ValueRef> &Operands, Type &, IRBuilder<> &IRB) {
  auto Width = cast<IGCLLVM::FixedVectorType>(
                   Operands[cmcl::SVMAtomicAddOperand::Address].get().getType())
                   ->getNumElements();
  auto *SrcTy = Operands[cmcl::SVMAtomicAddOperand::Source].get().getType();
  Type *Tys[] = {SrcTy, IGCLLVM::FixedVectorType::get(IRB.getInt1Ty(), Width),
                 Operands[cmcl::SVMAtomicAddOperand::Address].get().getType()};
  auto *Decl = GenXIntrinsic::getGenXDeclaration(
      IRB.GetInsertBlock()->getParent()->getParent(),
      GenXIntrinsic::genx_svm_atomic_add, Tys);
  Value *Args[] = {IRB.CreateVectorSplat(Width, IRB.getInt1(true)),
                   &Operands[cmcl::SVMAtomicAddOperand::Address].get(),
                   &Operands[cmcl::SVMAtomicAddOperand::Source].get(),
                   UndefValue::get(SrcTy)};
  auto *Result = IRB.CreateCall(Decl, Args, "cmcl.svm.atomic.add");
  return {*Result};
}

// Writes output values of created "MainInst".
// The order of output values in \p Results:
//    builtin return value if any, output operands in order of builtin
//    arguments (VectorOut, VectorInOut, etc.) if any.
template <cmcl::BuiltinID::Enum BiID>
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
  for (int BiOpIdx = 0; BiOpIdx != cmcl::BuiltinOperandSize[BiID]; ++BiOpIdx)
    if (cmcl::isOutputOperand(cmcl::BuiltinOperandKind[BiID][BiOpIdx]))
      writeVectorFromBuiltinOp(Results[ResultIdx++],
                               *BiCall.getArgOperand(BiOpIdx), IRB);
}

template <cmcl::BuiltinID::Enum BiID>
static void handleBuiltinCall(CallInst &BiCall) {
  IRBuilder<> IRB{&BiCall};

  auto Operands = readOperands<BiID>(BiCall, IRB);
  auto Result = createMainInst<BiID>(Operands, *BiCall.getType(), IRB);
  writeBuiltinResults<BiID>(Result, BiCall, IRB);
}

// FIXME: autogenerate it.
static BuiltinCallHandlerSeq getBuiltinCallHandlers() {
  BuiltinCallHandlerSeq Handlers{cmcl::BuiltinID::Size};
  Handlers[cmcl::BuiltinID::Select] =
      handleBuiltinCall<cmcl::BuiltinID::Select>;
  Handlers[cmcl::BuiltinID::RdRegion] =
      handleBuiltinCall<cmcl::BuiltinID::RdRegion>;
  Handlers[cmcl::BuiltinID::WrRegion] =
      handleBuiltinCall<cmcl::BuiltinID::WrRegion>;
  Handlers[cmcl::BuiltinID::PrintfBuffer] =
      handleBuiltinCall<cmcl::BuiltinID::PrintfBuffer>;
  Handlers[cmcl::BuiltinID::PrintfFormatIndex] =
      handleBuiltinCall<cmcl::BuiltinID::PrintfFormatIndex>;
  Handlers[cmcl::BuiltinID::PrintfFormatIndexLegacy] =
      handleBuiltinCall<cmcl::BuiltinID::PrintfFormatIndexLegacy>;
  Handlers[cmcl::BuiltinID::SVMScatter] =
      handleBuiltinCall<cmcl::BuiltinID::SVMScatter>;
  Handlers[cmcl::BuiltinID::SVMAtomicAdd] =
      handleBuiltinCall<cmcl::BuiltinID::SVMAtomicAdd>;
  return Handlers;
}

static bool handleBuiltin(Function &Builtin,
                          const BuiltinCallHandlerSeq &BiCallHandlers) {
  assert(isCMCLBuiltin(Builtin) &&
         "wrong argument: CM-CL builtin was expected");
  if (Builtin.use_empty())
    return false;
  auto BiID = decodeBuiltin(Builtin.getName());
  for (User *Usr : Builtin.users()) {
    assert((BiID >= 0 && BiID < static_cast<int>(BiCallHandlers.size()) &&
            BiCallHandlers[BiID]) &&
           "no handler for such builtin ID");
    BiCallHandlers[BiID](*cast<CallInst>(Usr));
  }
  return true;
}

bool cmcl::translateBuiltins(Module &M) {
  auto Builtins = collectBuiltins(M);
  if (Builtins.empty())
    return false;
  auto BiCallHandlers = getBuiltinCallHandlers();
  for (Function &Builtin : Builtins)
    handleBuiltin(Builtin, BiCallHandlers);
  cleanUpBuiltins(Builtins);
  return true;
}
