/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

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

#include "vc/BiF/PrintfIface.h"
#include "vc/BiF/Tools.h"
#include "vc/Support/BackendConfig.h"

#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/iterator_range.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Pass.h>
#include <llvm/Support/ErrorHandling.h>

#include <algorithm>
#include <functional>
#include <numeric>
#include <sstream>
#include <vector>

using namespace llvm;
using namespace vc::bif::printf;

namespace PrintfImplFunc {
enum Enum { Init, Fmt, Arg, Ret, Size };
static constexpr const char *Name[Size] = {"__vc_printf_init",
                                           "__vc_printf_fmt", "__vc_printf_arg",
                                           "__vc_printf_ret"};
} // namespace PrintfImplFunc

static constexpr int FormatStringAddrSpace = 2;

namespace {
class GenXPrintfResolution final : public ModulePass {
  const DataLayout *DL;
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
  CallInst &createPrintfInitCall(CallInst &OrigPrintf);
  CallInst &createPrintfFmtCall(CallInst &OrigPrintf, CallInst &InitCall);
  CallInst &createPrintfArgCall(CallInst &OrigPrintf, CallInst &PrevCall);
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

void GenXPrintfResolution::handlePrintfCall(CallInst &OrigPrintf) {
  assertPrintfCall(OrigPrintf);

  auto &InitCall = createPrintfInitCall(OrigPrintf);
  auto &FmtCall = createPrintfFmtCall(OrigPrintf, InitCall);
  // potentially FmtCall as there may be no arguments
  auto &LastArgCall = *std::accumulate(
      std::next(OrigPrintf.arg_begin()), OrigPrintf.arg_end(), &FmtCall,
      [&OrigPrintf, this](CallInst *PrevCall, Value *Arg) {
        return &createPrintfArgCall(OrigPrintf, *PrevCall);
      });
  auto &RetCall = createPrintfRetCall(OrigPrintf, LastArgCall);
  RetCall.takeName(&OrigPrintf);
  OrigPrintf.replaceAllUsesWith(&RetCall);
  OrigPrintf.eraseFromParent();
}

using PrintfImplTypeStorage = std::array<FunctionType *, PrintfImplFunc::Size>;

static PrintfImplTypeStorage getPrintfImplTypes(LLVMContext &Ctx) {
  auto *TransferDataTy =
      VectorType::get(Type::getInt32Ty(Ctx), TransferDataSize);
  auto *ArgsInfoTy =
      VectorType::get(Type::getInt32Ty(Ctx), ArgsInfoVector::Size);
  auto *ArgDataTy = VectorType::get(Type::getInt32Ty(Ctx), ArgData::Size);
  constexpr bool IsVarArg = false;

  PrintfImplTypeStorage FuncTys;
  FuncTys[PrintfImplFunc::Init] =
      FunctionType::get(TransferDataTy, ArgsInfoTy, IsVarArg);
  FuncTys[PrintfImplFunc::Fmt] = FunctionType::get(
      TransferDataTy,
      {TransferDataTy,
       PointerType::get(Type::getInt8Ty(Ctx), FormatStringAddrSpace)},
      IsVarArg);
  FuncTys[PrintfImplFunc::Arg] = FunctionType::get(
      TransferDataTy, {TransferDataTy, Type::getInt32Ty(Ctx), ArgDataTy},
      IsVarArg);
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

static ArgsInfoStorage collectArgsInfo(CallInst &OrigPrintf) {
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
  // FIXME: what if we want to print a pointer to a string?
  // Seems like it cannot be handled without parsing the format string.
  ArgsInfo[ArgsInfoVector::NumPtr] = llvm::count_if(PrintfArgs, [](Value *Arg) {
    return Arg->getType()->isPointerTy() &&
           !Arg->getType()->getPointerElementType()->isIntegerTy(8);
  });
  ArgsInfo[ArgsInfoVector::NumStr] = llvm::count_if(PrintfArgs, [](Value *Arg) {
    return Arg->getType()->isPointerTy() &&
           Arg->getType()->getPointerElementType()->isIntegerTy(8);
  });
  return ArgsInfo;
}

CallInst &GenXPrintfResolution::createPrintfInitCall(CallInst &OrigPrintf) {
  assertPrintfCall(OrigPrintf);
  auto ArgsInfo = collectArgsInfo(OrigPrintf);

  IRBuilder<> IRB{&OrigPrintf};
  auto *ArgsInfoV = ConstantDataVector::get(OrigPrintf.getContext(), ArgsInfo);
  return *IRB.CreateCall(PrintfImplDecl[PrintfImplFunc::Init], ArgsInfoV,
                         OrigPrintf.getName() + ".printf.init");
}

CallInst &GenXPrintfResolution::createPrintfFmtCall(CallInst &OrigPrintf,
                                                    CallInst &InitCall) {
  assertPrintfCall(OrigPrintf);
  IRBuilder<> IRB{&OrigPrintf};
  return *IRB.CreateCall(PrintfImplDecl[PrintfImplFunc::Fmt],
                         {&InitCall, OrigPrintf.getOperand(0)},
                         OrigPrintf.getName() + ".printf.fmt");
}

CallInst &GenXPrintfResolution::createPrintfArgCall(CallInst &OrigPrintf,
                                                    CallInst &PrevCall) {
  assertPrintfCall(OrigPrintf);
  return PrevCall;
}

CallInst &GenXPrintfResolution::createPrintfRetCall(CallInst &OrigPrintf,
                                                    CallInst &PrevCall) {
  assertPrintfCall(OrigPrintf);
  IRBuilder<> IRB{&OrigPrintf};
  return *IRB.CreateCall(PrintfImplDecl[PrintfImplFunc::Ret], &PrevCall);
}
