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

#include "vc/BiF/PrintfIface.h"
#include "vc/GenXOpts/GenXOpts.h"
#include "vc/Support/BackendConfig.h"
#include "vc/Support/GenXDiagnostic.h"
#include "vc/Utils/GenX/Printf.h"
#include "vc/Utils/General/BiF.h"
#include "vc/Utils/General/IRBuilder.h"
#include "vc/Utils/General/Types.h"

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
#include "llvmWrapper/IR/Operator.h"

#include <algorithm>
#include <functional>
#include <numeric>
#include <sstream>
#include <vector>

using namespace llvm;
using namespace vc;
using namespace vc::bif::printf;

namespace PrintfImplFunc {
enum Enum {
  Init,
  Fmt,
  FmtGlobal,
  FmtLegacy,
  Arg,
  ArgStr,
  ArgStrGlobal,
  ArgStrLegacy,
  Ret,
  Size
};
static constexpr const char *Name[Size] = {"__vc_printf_init",
                                           "__vc_printf_fmt",
                                           "__vc_printf_fmt_global",
                                           "__vc_printf_fmt_legacy",
                                           "__vc_printf_arg",
                                           "__vc_printf_arg_str",
                                           "__vc_printf_arg_str_global",
                                           "__vc_printf_arg_str_legacy",
                                           "__vc_printf_ret"};
} // namespace PrintfImplFunc

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
  void preparePrintfImplForInlining();
  CallInst &createPrintfInitCall(CallInst &OrigPrintf, int FmtStrSize,
                                 const PrintfArgInfoSeq &ArgsInfo);
  CallInst &createPrintfFmtCall(CallInst &OrigPrintf, CallInst &InitCall);
  CallInst &createPrintfArgCall(CallInst &OrigPrintf, CallInst &PrevCall,
                                Value &Arg, PrintfArgInfo Info);
  CallInst &createPrintfArgStrCall(CallInst &OrigPrintf, CallInst &PrevCall,
                                   Value &Arg);
  CallInst &createPrintfRetCall(CallInst &OrigPrintf, CallInst &PrevCall);

  template <PrintfImplFunc::Enum DefaulDeclID>
  CallInst &createCallWithStringArg(CallInst &OrigPrintf, Value &StrArg,
                                    Value &AuxArg);
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

// \p Workload must be a range of objects convertible to CallInst &. The range
// must not be empty. The set of called by \p Workload elements functions is
// returned. For the most case there should be only one printf declaration.
// Though considering that some frontends tend to place strings in different
// address spaces and that SPIR-V level linking is possible there may be cases
// where more that one printf declaration is present.
template <typename Range>
static SmallPtrSet<Function *, 1> getPrintfDeclarations(const Range &Workload) {
  IGC_ASSERT_MESSAGE(Workload.begin() != Workload.end(),
                     "wrong argument: the input range must not be empty");
  SmallPtrSet<Function *, 1> Declarations;
  for (CallInst &CI : Workload)
    Declarations.insert(CI.getCalledFunction());
  IGC_ASSERT_MESSAGE(!Declarations.empty(),
                     "must be at least 1 printf declaration");
  IGC_ASSERT_MESSAGE(
      llvm::all_of(Declarations,
                   [](Function *F) { return F && F->isDeclaration(); }),
      "printf must be a declaration");
  return Declarations;
}

bool GenXPrintfResolution::runOnModule(Module &M) {
  DL = &M.getDataLayout();

  std::vector<CallInstRef> Workload = collectWorkload(M);
  if (Workload.empty())
    return false;
  auto PrintfDecls = getPrintfDeclarations(Workload);

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
  preparePrintfImplForInlining();

  IGC_ASSERT_MESSAGE(
      llvm::all_of(PrintfDecls, [](Function *F) { return F->use_empty(); }),
      "no users of printf function must be left");
  for (Function *PrintfDecl : PrintfDecls)
    PrintfDecl->eraseFromParent();
  return true;
}

std::unique_ptr<Module> GenXPrintfResolution::getBiFModule(LLVMContext &Ctx) {
  MemoryBufferRef PrintfBiFModuleBuffer =
      getAnalysis<GenXBackendConfig>().getBiFModule(BiFKind::VCPrintf);
  if (!PrintfBiFModuleBuffer.getBufferSize()) {
    IGC_ASSERT_MESSAGE(0, "printf implementation module is absent");
  }
  return vc::getBiFModuleOrReportError(PrintfBiFModuleBuffer, Ctx);
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
    diagnose(FmtStrOp.getContext(), "GenXPrintfResolution",
             PrintfStringAccessError);
  return {FmtStr.getValue().size() + 1, parseFormatString(FmtStr.getValue())};
}

// Marks strings passed as "%s" arguments in printf.
// Recursive function, long instruction chains aren't expected.
static void markStringArgument(Value &Arg) {
  if (isa<GEPOperator>(Arg)) {
    auto *String = getConstStringGVFromOperandOptional(Arg);
    if (!String)
      diagnose(Arg.getContext(), "GenXPrintfResolution",
               PrintfStringAccessError);
    String->addAttribute(PrintfStringVariable);
    return;
  }
  if (isa<SelectInst>(Arg)) {
    auto &SI = cast<SelectInst>(Arg);
    // The same value can be potentially accessed by different paths. Though
    // it is probably OK, since the same string can be marked several times
    // and the most of the time cases would be simple so it is not that
    // critical to pass same values several times in some rare complicated
    // cases.
    markStringArgument(*SI.getFalseValue());
    markStringArgument(*SI.getTrueValue());
    return;
  }
  if (isCastToGenericAS(Arg))
    return markStringArgument(
        *cast<IGCLLVM::AddrSpaceCastOperator>(Arg).getPointerOperand());
  // An unsupported instruction or instruction sequence was met.
  diagnose(Arg.getContext(), "GenXPrintfResolution", PrintfStringAccessError);
}

// Marks printf strings: format strings, strings passed as "%s" arguments.
static void markPrintfStrings(CallInst &OrigPrintf,
                              const PrintfArgInfoSeq &ArgsInfo) {
  auto &FormatString =
      getConstStringGVFromOperand(*OrigPrintf.getArgOperand(0));
  FormatString.addAttribute(PrintfStringVariable);

  // Handle string arguments (%s).
  auto StringArgs = make_filter_range(
      zip(drop_begin(OrigPrintf.args(), 1), ArgsInfo), [](auto &&ArgWithInfo) {
        return std::get<const PrintfArgInfo &>(ArgWithInfo).Type ==
               PrintfArgInfo::String;
      });
  for (auto &&[Arg, ArgInfo] : StringArgs)
    markStringArgument(*Arg.get());
}

void GenXPrintfResolution::handlePrintfCall(CallInst &OrigPrintf) {
  assertPrintfCall(OrigPrintf);
  auto [FmtStrSize, ArgsInfo] =
      analyzeFormatString(*OrigPrintf.getArgOperand(0));
  if (ArgsInfo.size() != OrigPrintf.getNumArgOperands() - 1)
    diagnose(OrigPrintf.getContext(), "GenXPrintfResolution",
             "printf format string and arguments don't correspond");

  markPrintfStrings(OrigPrintf, ArgsInfo);

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
  FuncTys[PrintfImplFunc::Fmt] =
      FunctionType::get(TransferDataTy,
                        {TransferDataTy, PointerType::get(Type::getInt8Ty(Ctx),
                                                          AddrSpace::Constant)},
                        IsVarArg);
  FuncTys[PrintfImplFunc::FmtGlobal] =
      FunctionType::get(TransferDataTy,
                        {TransferDataTy, PointerType::get(Type::getInt8Ty(Ctx),
                                                          AddrSpace::Global)},
                        IsVarArg);
  FuncTys[PrintfImplFunc::FmtLegacy] =
      FunctionType::get(TransferDataTy,
                        {TransferDataTy, PointerType::get(Type::getInt8Ty(Ctx),
                                                          AddrSpace::Private)},
                        IsVarArg);
  FuncTys[PrintfImplFunc::Arg] = FunctionType::get(
      TransferDataTy, {TransferDataTy, Type::getInt32Ty(Ctx), ArgDataTy},
      IsVarArg);
  FuncTys[PrintfImplFunc::ArgStr] = FuncTys[PrintfImplFunc::Fmt];
  FuncTys[PrintfImplFunc::ArgStrGlobal] = FuncTys[PrintfImplFunc::FmtGlobal];
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

// The function must be internal and have always inline attribute for
// always-inline pass to inline it and remove the original function body
// (the both are critical for GenXPrintfLegalization to work correctly).
void GenXPrintfResolution::preparePrintfImplForInlining() {
  for (auto Callee : PrintfImplDecl) {
    auto *Func = cast<Function>(Callee.getCallee());
    Func->setLinkage(GlobalValue::LinkageTypes::InternalLinkage);
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

template <PrintfImplFunc::Enum DefaulDeclID, unsigned StrAS>
static PrintfImplFunc::Enum mutateDeclIDImpl();

template <>
PrintfImplFunc::Enum
mutateDeclIDImpl<PrintfImplFunc::Fmt, AddrSpace::Constant>() {
  return PrintfImplFunc::Fmt;
}

template <>
PrintfImplFunc::Enum
mutateDeclIDImpl<PrintfImplFunc::Fmt, AddrSpace::Global>() {
  return PrintfImplFunc::FmtGlobal;
}

template <>
PrintfImplFunc::Enum
mutateDeclIDImpl<PrintfImplFunc::Fmt, AddrSpace::Private>() {
  return PrintfImplFunc::FmtLegacy;
}

template <>
PrintfImplFunc::Enum
mutateDeclIDImpl<PrintfImplFunc::ArgStr, AddrSpace::Constant>() {
  return PrintfImplFunc::ArgStr;
}

template <>
PrintfImplFunc::Enum
mutateDeclIDImpl<PrintfImplFunc::ArgStr, AddrSpace::Global>() {
  return PrintfImplFunc::ArgStrGlobal;
}

template <>
PrintfImplFunc::Enum
mutateDeclIDImpl<PrintfImplFunc::ArgStr, AddrSpace::Private>() {
  return PrintfImplFunc::ArgStrLegacy;
}

// Transforms the provided declaration ID depending on the string address space
// \p StrAS. Only PrintfImplFunc::Fmt and PrintfImplFunc::ArgStr can be passed
// as \p DefaulDeclID.
template <PrintfImplFunc::Enum DefaulDeclID>
static PrintfImplFunc::Enum mutateDeclID(unsigned StrAS) {
  switch (StrAS) {
  default:
    IGC_ASSERT_MESSAGE(0, "unexpected address space for a string argument");
  case AddrSpace::Constant:
    return mutateDeclIDImpl<DefaulDeclID, AddrSpace::Constant>();
  case AddrSpace::Global:
    return mutateDeclIDImpl<DefaulDeclID, AddrSpace::Global>();
  case AddrSpace::Private:
    return mutateDeclIDImpl<DefaulDeclID, AddrSpace::Private>();
  }
}

// If \p StrArg is a generic pointer this function returns the resolved
// pointer - a non-generic pointer to the same object generic pointer was
// pointing to. When \p StrArg is already an non-generic pointer it is returned
// unchanged.
// \p StrArg must have a i8 pointer type.
static Value &resolveStringInGenericASIf(Value &StrArg) {
  auto StrAS = StrArg.getType()->getPointerAddressSpace();
  if (StrAS != AddrSpace::Generic)
    return StrArg;
  auto *GV = getConstStringGVFromOperandOptional(StrArg);
  if (!GV)
    // FIXME: String marking should already exclude too entangled string
    //        accesses. Though it is still possible to have series of switch
    //        instructions mixed with addrspace casts. This case is not
    //        supported here, but the string marking won't exclude it.
    //        Select instructions should be supported for consistancy.
    diagnose(StrArg.getContext(), "GenXPrintfResolution", &StrArg,
             "The pass cannot resolve generic address space "
             "to access the provided string");
  return castArrayToFirstElemPtr(*GV);
}

// Common interface to generate call for format string or "%s" string argument
// handler.
// PrintfImplFunc::Fmt should be provided in template parameter to handle format
// string, PrintfImplFunc::ArgStr - for "%s" argument. Only those 2 declaration
// IDs can be passed. The function itself will consider string address space and
// mutate the provided declaration ID.
template <PrintfImplFunc::Enum DefaulDeclID>
CallInst &GenXPrintfResolution::createCallWithStringArg(CallInst &OrigPrintf,
                                                        Value &StrArg,
                                                        Value &AuxArg) {
  assertPrintfCall(OrigPrintf);
  IRBuilder<> IRB{&OrigPrintf};
  auto &ResolvedStrArg = resolveStringInGenericASIf(StrArg);
  auto StrAS = ResolvedStrArg.getType()->getPointerAddressSpace();
  PrintfImplFunc::Enum DeclID = mutateDeclID<DefaulDeclID>(StrAS);
  return *IRB.CreateCall(PrintfImplDecl[DeclID], {&AuxArg, &ResolvedStrArg},
                         OrigPrintf.getName() + ".printf.str.arg");
}

CallInst &GenXPrintfResolution::createPrintfFmtCall(CallInst &OrigPrintf,
                                                    CallInst &InitCall) {
  return createCallWithStringArg<PrintfImplFunc::Fmt>(
      OrigPrintf, *OrigPrintf.getOperand(0), InitCall);
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
  return createCallWithStringArg<PrintfImplFunc::ArgStr>(OrigPrintf, Arg,
                                                         PrevCall);
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
