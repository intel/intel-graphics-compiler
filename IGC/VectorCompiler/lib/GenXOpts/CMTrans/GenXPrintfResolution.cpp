/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXPrintfResolution
/// --------------------
/// This pass finds every call to printf function and replaces it with a series
/// of printf implementation functions from BiF. A proper version of
/// implementation (32/64 bit, cm/ocl/ze binary) is provided by outer logic.
/// Before:
/// %p = call spir_func i32 (i8 as(2)*, ...) @printf(i8 as(2)* %str)
/// After:
/// %init = call <4 x i32> @__vc_printf_init(<4 x i32> zeroinitializer)
/// %fmt = call <4 x i32> @__vc_printf_fmt(<4 x i32> %init, i8 as(2)* %str)
/// %printf = call i32 @__vc_printf_ret(<4 x i32> %fmt)
///
/// Vector <4 x i32> is passed between functions to transfer their internal
/// data. This data is handled by the function implementations themselves,
/// this pass knows nothing about it.
//===----------------------------------------------------------------------===//

#include "vc/GenXOpts/GenXOpts.h"
#include "vc/GenXOpts/Utils/BiFTools.h"
#include "vc/GenXOpts/Utils/Printf.h"

#include "vc/BiF/PrintfIface.h"
#include "vc/BiF/Tools.h"
#include "vc/Support/BackendConfig.h"

#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/iterator_range.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Pass.h>
#include <llvm/Support/ErrorHandling.h>

#include "llvmWrapper/IR/DerivedTypes.h"

#include <algorithm>
#include <functional>
#include <numeric>
#include <sstream>
#include <vector>

using namespace llvm;
using namespace vc::bif::printf;

namespace PrintfImplFunc {
enum Enum { Init, Fmt, FmtLegacy, Arg, ArgStr, ArgStrLegacy, Ret, Size };
static constexpr const char *Name[Size] = {
    "__vc_printf_init", "__vc_printf_fmt",     "__vc_printf_fmt_legacy",
    "__vc_printf_arg",  "__vc_printf_arg_str", "__vc_printf_arg_str_legacy",
    "__vc_printf_ret"};
} // namespace PrintfImplFunc

static constexpr int FormatStringAddrSpace = 2;
static constexpr int LegacyFormatStringAddrSpace = 0;

namespace {
class GenXPrintfResolution final : public ModulePass {
  const DataLayout *DL = nullptr;
  std::array<FunctionCallee, PrintfImplFunc::Size> PrintfImplDecl;

public:
  static char ID;
  GenXPrintfResolution() : ModulePass(ID) {}
  StringRef getPassName() const override { return "GenX printf resolution"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnModule(Module &M) override;

private:
  std::unique_ptr<Module> getBiFModule(LLVMContext &Ctx);
  void handlePrintfCall(CallInst &OrigPrintf);
  void addPrintfImplDeclarations(Module &M);
  void updatePrintfImplDeclarations(Module &M);
  void setAlwaysInlineForPrintfImpl();
  CallInst &createPrintfInitCall(CallInst &OrigPrintf, int FmtStrSize,
                                 const PrintfArgInfoSeq &ArgsInfo);
  CallInst &createPrintfFmtCall(CallInst &OrigPrintf, CallInst &InitCall);
  CallInst &createPrintfArgCall(CallInst &OrigPrintf, CallInst &PrevCall,
                                Value &Arg, PrintfArgInfo Info);
  CallInst &createPrintfArgStrCall(CallInst &OrigPrintf, CallInst &PrevCall,
                                   Value &Arg);
  CallInst &createPrintfRetCall(CallInst &OrigPrintf, CallInst &PrevCall);
};
} // namespace

char GenXPrintfResolution::ID = 0;
namespace llvm {
void initializeGenXPrintfResolutionPass(PassRegistry &);
}

INITIALIZE_PASS_BEGIN(GenXPrintfResolution, "GenXPrintfResolution",
                      "GenXPrintfResolution", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_END(GenXPrintfResolution, "GenXPrintfResolution",
                    "GenXPrintfResolution", false, false)

ModulePass *llvm::createGenXPrintfResolutionPass() {
  initializeGenXPrintfResolutionPass(*PassRegistry::getPassRegistry());
  return new GenXPrintfResolution;
}

void GenXPrintfResolution::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<GenXBackendConfig>();
}

using CallInstRef = std::reference_wrapper<CallInst>;

static bool isPrintfCall(const CallInst &CI) {
  auto *CalledFunc = CI.getCalledFunction();
  if (!CalledFunc)
    return false;
  if (!CalledFunc->isDeclaration())
    return false;
  return CalledFunc->getName() == "printf" ||
    CalledFunc->getName().contains("__spirv_ocl_printf");
}

static bool isPrintfCall(const Instruction &Inst) {
  if (!isa<CallInst>(Inst))
    return false;
  return isPrintfCall(cast<CallInst>(Inst));
}

static std::vector<CallInstRef> collectWorkload(Module &M) {
  std::vector<CallInstRef> Workload;
  for (Function &F : M)
    llvm::transform(
        make_filter_range(instructions(F),
                          [](Instruction &Inst) { return isPrintfCall(Inst); }),
        std::back_inserter(Workload),
        [](Instruction &Inst) { return std::ref(cast<CallInst>(Inst)); });
  return Workload;
}

bool GenXPrintfResolution::runOnModule(Module &M) {
  DL = &M.getDataLayout();

  std::vector<CallInstRef> Workload = collectWorkload(M);
  if (Workload.empty())
    return false;
  addPrintfImplDeclarations(M);
  for (CallInst &CI : Workload)
    handlePrintfCall(CI);

  std::unique_ptr<Module> PrintfImplModule = getBiFModule(M.getContext());
  PrintfImplModule->setDataLayout(M.getDataLayout());
  PrintfImplModule->setTargetTriple(M.getTargetTriple());
  if (Linker::linkModules(M, std::move(PrintfImplModule),
                          Linker::Flags::LinkOnlyNeeded)) {
    IGC_ASSERT_MESSAGE(0, "Error linking printf implementation builtin module");
  }
  updatePrintfImplDeclarations(M);
  setAlwaysInlineForPrintfImpl();
  return true;
}

std::unique_ptr<Module> GenXPrintfResolution::getBiFModule(LLVMContext &Ctx) {
  MemoryBufferRef PrintfBiFModuleBuffer =
      getAnalysis<GenXBackendConfig>().getBiFModule(BiFKind::VCPrintf);
  if (!PrintfBiFModuleBuffer.getBufferSize()) {
    IGC_ASSERT_MESSAGE(
        vc::bif::disabled(),
        "printf bif module can be empty only if vc bif was disabled");
    report_fatal_error("printf is not supported when VC BiF is disabled");
  }
  return getBiFModuleOrReportError(PrintfBiFModuleBuffer, Ctx);
}

static void assertPrintfCall(const CallInst &CI) {
  IGC_ASSERT_MESSAGE(isPrintfCall(CI), "printf call is expected");
  IGC_ASSERT_MESSAGE(CI.arg_size() > 0,
                     "printf call must have at least format string argument");
  (void)CI;
}

// Returns pair of format string size (including '\0') and argument information.
static std::pair<int, PrintfArgInfoSeq>
analyzeFormatString(const Value &FmtStrOp) {
  auto FmtStr = getConstStringFromOperandOptional(FmtStrOp);
  if (!FmtStr)
    report_fatal_error(
        "printf resolution cannot access format string during compile time");
  return {FmtStr.getValue().size() + 1, parseFormatString(FmtStr.getValue())};
}

void GenXPrintfResolution::handlePrintfCall(CallInst &OrigPrintf) {
  assertPrintfCall(OrigPrintf);
  auto [FmtStrSize, ArgsInfo] =
      analyzeFormatString(*OrigPrintf.getArgOperand(0));
  if (ArgsInfo.size() != OrigPrintf.getNumArgOperands() - 1)
    report_fatal_error("printf format string and arguments don't correspond");

  auto &InitCall = createPrintfInitCall(OrigPrintf, FmtStrSize, ArgsInfo);
  auto &FmtCall = createPrintfFmtCall(OrigPrintf, InitCall);

  // FIXME: combine LLVM call args type and format string info in more
  // intelligent way.
  auto ArgsWithInfo = zip(ArgsInfo, drop_begin(OrigPrintf.args(), 1));
  // potentially FmtCall as there may be no arguments
  auto &LastArgCall = *std::accumulate(
      ArgsWithInfo.begin(), ArgsWithInfo.end(), &FmtCall,
      [&OrigPrintf, this](CallInst *PrevCall, auto &&ArgWithInfo) {
        return &createPrintfArgCall(OrigPrintf, *PrevCall,
                                    *std::get<Use &>(ArgWithInfo).get(),
                                    std::get<PrintfArgInfo &>(ArgWithInfo));
      });
  auto &RetCall = createPrintfRetCall(OrigPrintf, LastArgCall);
  RetCall.takeName(&OrigPrintf);
  OrigPrintf.replaceAllUsesWith(&RetCall);
  OrigPrintf.eraseFromParent();
}

using PrintfImplTypeStorage = std::array<FunctionType *, PrintfImplFunc::Size>;

static PrintfImplTypeStorage getPrintfImplTypes(LLVMContext &Ctx) {
  auto *TransferDataTy =
      IGCLLVM::FixedVectorType::get(Type::getInt32Ty(Ctx), TransferDataSize);
  auto *ArgsInfoTy =
      IGCLLVM::FixedVectorType::get(Type::getInt32Ty(Ctx), ArgsInfoVector::Size);
  auto *ArgDataTy = IGCLLVM::FixedVectorType::get(Type::getInt32Ty(Ctx), ArgData::Size);
  constexpr bool IsVarArg = false;

  PrintfImplTypeStorage FuncTys;
  FuncTys[PrintfImplFunc::Init] =
      FunctionType::get(TransferDataTy, ArgsInfoTy, IsVarArg);
  FuncTys[PrintfImplFunc::Fmt] = FunctionType::get(
      TransferDataTy,
      {TransferDataTy,
       PointerType::get(Type::getInt8Ty(Ctx), FormatStringAddrSpace)},
      IsVarArg);
  FuncTys[PrintfImplFunc::FmtLegacy] = FunctionType::get(
      TransferDataTy,
      {TransferDataTy,
       PointerType::get(Type::getInt8Ty(Ctx), LegacyFormatStringAddrSpace)},
      IsVarArg);
  FuncTys[PrintfImplFunc::Arg] = FunctionType::get(
      TransferDataTy, {TransferDataTy, Type::getInt32Ty(Ctx), ArgDataTy},
      IsVarArg);
  FuncTys[PrintfImplFunc::ArgStr] = FuncTys[PrintfImplFunc::Fmt];
  FuncTys[PrintfImplFunc::ArgStrLegacy] = FuncTys[PrintfImplFunc::FmtLegacy];
  FuncTys[PrintfImplFunc::Ret] =
      FunctionType::get(Type::getInt32Ty(Ctx), TransferDataTy, IsVarArg);
  return FuncTys;
}

void GenXPrintfResolution::addPrintfImplDeclarations(Module &M) {
  auto PrintfImplTy = getPrintfImplTypes(M.getContext());

  for (int FuncID = 0; FuncID != PrintfImplFunc::Size; ++FuncID)
    PrintfImplDecl[FuncID] = M.getOrInsertFunction(PrintfImplFunc::Name[FuncID],
                                                   PrintfImplTy[FuncID]);
}

void GenXPrintfResolution::setAlwaysInlineForPrintfImpl() {
  for (auto Callee : PrintfImplDecl) {
    auto *Func = cast<Function>(Callee.getCallee());
    if (!Func->hasFnAttribute(Attribute::AlwaysInline))
      Func->addFnAttr(Attribute::AlwaysInline);
  }
}

void GenXPrintfResolution::updatePrintfImplDeclarations(Module &M) {
  std::transform(
      std::begin(PrintfImplFunc::Name), std::end(PrintfImplFunc::Name),
      PrintfImplDecl.begin(),
      [&M](const char *Name) -> FunctionCallee { return M.getFunction(Name); });
}

using ArgsInfoStorage = std::array<unsigned, ArgsInfoVector::Size>;

// Returns arguments information required by init implementation function.
// FIXME: combine LLVM call args type and format string info before this
// function.
static ArgsInfoStorage collectArgsInfo(CallInst &OrigPrintf, int FmtStrSize,
                                       const PrintfArgInfoSeq &FmtArgsInfo) {
  assertPrintfCall(OrigPrintf);

  ArgsInfoStorage ArgsInfo;
  // It's not about format string.
  ArgsInfo[ArgsInfoVector::NumTotal] = OrigPrintf.arg_size() - 1;
  auto PrintfArgs =
      make_range(std::next(OrigPrintf.arg_begin()), OrigPrintf.arg_end());

  ArgsInfo[ArgsInfoVector::Num64Bit] =
      llvm::count_if(PrintfArgs, [](Value *Arg) {
        return Arg->getType()->getPrimitiveSizeInBits() == 64;
      });
  ArgsInfo[ArgsInfoVector::NumPtr] =
      llvm::count_if(FmtArgsInfo, [](PrintfArgInfo Info) {
        return Info.Type == PrintfArgInfo::Pointer;
      });
  ArgsInfo[ArgsInfoVector::NumStr] =
      llvm::count_if(FmtArgsInfo, [](PrintfArgInfo Info) {
        return Info.Type == PrintfArgInfo::String;
      });
  ArgsInfo[ArgsInfoVector::FormatStrSize] = FmtStrSize;
  return ArgsInfo;
}

CallInst &GenXPrintfResolution::createPrintfInitCall(
    CallInst &OrigPrintf, int FmtStrSize, const PrintfArgInfoSeq &FmtArgsInfo) {
  assertPrintfCall(OrigPrintf);
  auto ImplArgsInfo = collectArgsInfo(OrigPrintf, FmtStrSize, FmtArgsInfo);

  IRBuilder<> IRB{&OrigPrintf};
  auto *ArgsInfoV =
      ConstantDataVector::get(OrigPrintf.getContext(), ImplArgsInfo);
  return *IRB.CreateCall(PrintfImplDecl[PrintfImplFunc::Init], ArgsInfoV,
                         OrigPrintf.getName() + ".printf.init");
}

CallInst &GenXPrintfResolution::createPrintfFmtCall(CallInst &OrigPrintf,
                                                    CallInst &InitCall) {
  assertPrintfCall(OrigPrintf);
  IRBuilder<> IRB{&OrigPrintf};
  auto FmtAS =
      cast<PointerType>(OrigPrintf.getOperand(0)->getType())->getAddressSpace();
  if (FmtAS == FormatStringAddrSpace)
    return *IRB.CreateCall(PrintfImplDecl[PrintfImplFunc::Fmt],
                           {&InitCall, OrigPrintf.getOperand(0)},
                           OrigPrintf.getName() + ".printf.fmt");
  IGC_ASSERT_MESSAGE(FmtAS == LegacyFormatStringAddrSpace,
                     "unexpected address space for format string");
  return *IRB.CreateCall(PrintfImplDecl[PrintfImplFunc::FmtLegacy],
                         {&InitCall, OrigPrintf.getOperand(0)},
                         OrigPrintf.getName() + ".printf.fmt");
}

static ArgKind::Enum getIntegerArgKind(Type &ArgTy) {
  IGC_ASSERT_MESSAGE(ArgTy.isIntegerTy(),
                     "wrong argument: integer type was expected");
  auto BitWidth = ArgTy.getIntegerBitWidth();
  switch (BitWidth) {
  case 64:
    return ArgKind::Long;
  case 32:
    return ArgKind::Int;
  case 16:
    return ArgKind::Short;
  default:
    IGC_ASSERT_MESSAGE(BitWidth == 8, "unexpected integer type");
    return ArgKind::Char;
  }
}

static ArgKind::Enum getFloatingPointArgKind(Type &ArgTy) {
  IGC_ASSERT_MESSAGE(ArgTy.isFloatingPointTy(),
                     "wrong argument: floating point type was expected");
  if (ArgTy.isDoubleTy())
    return ArgKind::Double;
  // FIXME: what about half?
  IGC_ASSERT_MESSAGE(ArgTy.isFloatTy(), "unexpected floating point type");
  return ArgKind::Float;
}

static ArgKind::Enum getPointerArgKind(Type &ArgTy, PrintfArgInfo Info) {
  IGC_ASSERT_MESSAGE(ArgTy.isPointerTy(),
                     "wrong argument: pointer type was expected");
  IGC_ASSERT_MESSAGE(Info.Type == PrintfArgInfo::Pointer ||
                         Info.Type == PrintfArgInfo::String,
                     "only %s and %p should correspond to pointer argument");
  (void)ArgTy;
  if (Info.Type == PrintfArgInfo::String)
    return ArgKind::String;
  return ArgKind::Pointer;
}

static ArgKind::Enum getArgKind(Type &ArgTy, PrintfArgInfo Info) {
  if (ArgTy.isIntegerTy())
    return getIntegerArgKind(ArgTy);
  if (ArgTy.isFloatingPointTy())
    return getFloatingPointArgKind(ArgTy);
  return getPointerArgKind(ArgTy, Info);
}

// sizeof(<2 x i32>) == 64
static constexpr unsigned VecArgSize = 64;
static constexpr auto VecArgElementSize = VecArgSize / ArgData::Size;

// Casts Arg to <2 x i32> vector. For pointers ptrtoint i64 should be generated
// first.
Value &get64BitArgAsVector(Value &Arg, IRBuilder<> &IRB, const DataLayout &DL) {
  IGC_ASSERT_MESSAGE(DL.getTypeSizeInBits(Arg.getType()) == 64,
                     "64-bit argument was expected");
  auto *VecArgTy =
      IGCLLVM::FixedVectorType::get(IRB.getInt32Ty(), ArgData::Size);
  Value *ArgToBitCast = &Arg;
  if (Arg.getType()->isPointerTy())
    ArgToBitCast =
        IRB.CreatePtrToInt(&Arg, IRB.getInt64Ty(), Arg.getName() + ".arg.p2i");
  return *IRB.CreateBitCast(ArgToBitCast, VecArgTy, Arg.getName() + ".arg.bc");
}

// Just creates this instruction:
// insertelement <2 x i32> zeroinitializer, i32 %arg, i32 0
// \p Arg must be i32 type.
Value &get32BitIntArgAsVector(Value &Arg, IRBuilder<> &IRB,
                              const DataLayout &DL) {
  IGC_ASSERT_MESSAGE(Arg.getType()->isIntegerTy(32),
                     "i32 argument was expected");
  auto *VecArgTy =
      IGCLLVM::FixedVectorType::get(IRB.getInt32Ty(), ArgData::Size);
  auto *BlankVec = ConstantAggregateZero::get(VecArgTy);
  return *IRB.CreateInsertElement(BlankVec, &Arg, IRB.getInt32(0),
                                  Arg.getName() + ".arg.insert");
}

// Takes arg that is not greater than 32 bit and casts it to i32 with possible
// zero extension.
static Value &getArgAs32BitInt(Value &Arg, IRBuilder<> &IRB,
                               const DataLayout &DL) {
  auto ArgSize = DL.getTypeSizeInBits(Arg.getType());
  IGC_ASSERT_MESSAGE(ArgSize <= VecArgElementSize,
                     "argument isn't expected to be greater than 32 bit");
  if (ArgSize < VecArgElementSize) {
    // FIXME: seems like there may be some problems with signed types, depending
    // on our BiF and runtime implementation.
    // FIXME: What about half?
    IGC_ASSERT_MESSAGE(Arg.getType()->isIntegerTy(),
                       "only integers are expected to be less than 32 bits");
    return *IRB.CreateZExt(&Arg, IRB.getInt32Ty(), Arg.getName() + ".arg.zext");
  }
  if (Arg.getType()->isPointerTy())
    return *IRB.CreatePtrToInt(&Arg, IRB.getInt32Ty(),
                               Arg.getName() + ".arg.p2i");
  if (!Arg.getType()->isIntegerTy())
    return *IRB.CreateBitCast(&Arg, IRB.getInt32Ty(),
                              Arg.getName() + ".arg.bc");
  return Arg;
}

// Args are passed via <2 x i32> vector. This function casts \p Arg to this
// vector type. \p Arg is zext if necessary (zext in common sense - writing
// top element of a vector with zeros is zero extending too).
static Value &getArgAsVector(Value &Arg, IRBuilder<> &IRB,
                             const DataLayout &DL) {
  IGC_ASSERT_MESSAGE(!isa<IGCLLVM::FixedVectorType>(Arg.getType()),
                     "scalar type is expected");
  auto ArgSize = DL.getTypeSizeInBits(Arg.getType());

  if (ArgSize == VecArgSize)
    return get64BitArgAsVector(Arg, IRB, DL);
  IGC_ASSERT_MESSAGE(ArgSize < VecArgSize,
                     "arg is expected to be not greater than 64 bit");
  Value &Arg32Bit = getArgAs32BitInt(Arg, IRB, DL);
  return get32BitIntArgAsVector(Arg32Bit, IRB, DL);
}

// Create call to printf argument handler implementation for string argument
// (%s). Strings require a separate implementation.
CallInst &GenXPrintfResolution::createPrintfArgStrCall(CallInst &OrigPrintf,
                                                       CallInst &PrevCall,
                                                       Value &Arg) {
  assertPrintfCall(OrigPrintf);
  IRBuilder<> IRB{&OrigPrintf};
  auto StrAS = cast<PointerType>(Arg.getType())->getAddressSpace();
  if (StrAS == FormatStringAddrSpace)
    return *IRB.CreateCall(PrintfImplDecl[PrintfImplFunc::ArgStr],
                           {&PrevCall, &Arg},
                           OrigPrintf.getName() + ".printf.arg");
  IGC_ASSERT_MESSAGE(StrAS == LegacyFormatStringAddrSpace,
                     "unexpected address space for a string argument");
  return *IRB.CreateCall(PrintfImplDecl[PrintfImplFunc::ArgStrLegacy],
                         {&PrevCall, &Arg},
                         OrigPrintf.getName() + ".printf.arg");
}

CallInst &GenXPrintfResolution::createPrintfArgCall(CallInst &OrigPrintf,
                                                    CallInst &PrevCall,
                                                    Value &Arg,
                                                    PrintfArgInfo Info) {
  assertPrintfCall(OrigPrintf);
  ArgKind::Enum Kind = getArgKind(*Arg.getType(), Info);
  IRBuilder<> IRB{&OrigPrintf};
  if (Kind == ArgKind::String)
    return createPrintfArgStrCall(OrigPrintf, PrevCall, Arg);
  Value &ArgVec = getArgAsVector(Arg, IRB, *DL);
  return *IRB.CreateCall(PrintfImplDecl[PrintfImplFunc::Arg],
                         {&PrevCall, IRB.getInt32(Kind), &ArgVec},
                         OrigPrintf.getName() + ".printf.arg");
}

CallInst &GenXPrintfResolution::createPrintfRetCall(CallInst &OrigPrintf,
                                                    CallInst &PrevCall) {
  assertPrintfCall(OrigPrintf);
  IRBuilder<> IRB{&OrigPrintf};
  return *IRB.CreateCall(PrintfImplDecl[PrintfImplFunc::Ret], &PrevCall);
}
