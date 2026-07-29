/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ConvertSPIRVExecutionModes.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/helper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include "common/LLVMWarningsPop.hpp"

#include <algorithm>
#include <optional>
#include <string>

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-convert-spirv-execution-modes"
#define PASS_DESCRIPTION "Convert SPIR-V entry point execution modes to function attributes"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ConvertSPIRVExecutionModesLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(ConvertSPIRVExecutionModesLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ConvertSPIRVExecutionModesLPM::ID = 0;

ConvertSPIRVExecutionModesLPM::ConvertSPIRVExecutionModesLPM() : ModulePass(ID) {
  initializeConvertSPIRVExecutionModesLPMPass(*PassRegistry::getPassRegistry());
}

namespace {
// Execution mode tokens defined by SPV_INTEL_maximum_registers.
enum SPIRVExecutionMode : uint32_t {
  MaximumRegistersINTEL = 6461,
  MaximumRegistersIdINTEL = 6462,
  NamedMaximumRegistersINTEL = 6463,
};

// The only "Named Maximum Number of Registers" value defined by revision 1 of
// the extension.
constexpr const char *AutoINTEL = "AutoINTEL";

// Attribute carrying the per-kernel register budget. "0" means "let the
// compiler pick", matching the existing "num-thread-per-eu" convention.
constexpr const char *NumGRFPerThreadAttr = "num-grf-per-thread";

std::optional<uint32_t> extractConstantInt(const Metadata *MD) {
  if (auto *CAM = dyn_cast_or_null<ConstantAsMetadata>(MD))
    if (auto *CI = dyn_cast<ConstantInt>(CAM->getValue()))
      return static_cast<uint32_t>(CI->getZExtValue());
  return std::nullopt;
}
} // anonymous namespace

bool ConvertSPIRVExecutionModesLPM::runOnModule(Module &M) {
  return ConvertSPIRVExecutionModes().run(M, getAnalysis<CodeGenContextWrapper>().getCodeGenContext(),
                                          getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
}

bool ConvertSPIRVExecutionModes::run(Module &M, CodeGenContext *Ctx, ModuleMetaData *MD) {
  NamedMDNode *ExecModesMD = M.getNamedMetadata("spirv.ExecutionMode");
  if (!ExecModesMD)
    return false;

  Context = Ctx;
  Metadata = MD;
  bool Changed = false;

  for (const MDNode *Node : ExecModesMD->operands()) {
    if (Node->getNumOperands() < 3)
      continue;

    auto *EntryPointMD = dyn_cast_or_null<ConstantAsMetadata>(Node->getOperand(0).get());
    if (!EntryPointMD)
      continue;
    auto *EntryPoint = dyn_cast<Function>(EntryPointMD->getValue());
    if (!EntryPoint)
      continue;

    std::optional<uint32_t> ExecutionMode = extractConstantInt(Node->getOperand(1).get());
    if (!ExecutionMode)
      continue;

    if (!isEntryFunc(MD, EntryPoint))
      continue;

    switch (*ExecutionMode) {
    case MaximumRegistersINTEL:
    case MaximumRegistersIdINTEL:
    case NamedMaximumRegistersINTEL:
      Changed |= handleMaximumRegisters(EntryPoint, *ExecutionMode, Node->getOperand(2).get());
      break;
    default:
      break;
    }
  }

  if (MetadataChanged)
    IGC::serialize(*MD, &M);

  return Changed;
}

bool ConvertSPIRVExecutionModes::handleMaximumRegisters(Function *F, uint32_t ExecutionMode,
                                                        const llvm::Metadata *Value) {
  uint32_t Requested = 0;

  switch (ExecutionMode) {
  case MaximumRegistersINTEL: {
    std::optional<uint32_t> Literal = extractConstantInt(Value);
    if (!Literal)
      return false;
    Requested = *Literal;
    break;
  }
  case MaximumRegistersIdINTEL: {
    auto *Wrapper = dyn_cast_or_null<MDNode>(Value);
    if (!Wrapper || Wrapper->getNumOperands() < 1)
      return false;
    std::optional<uint32_t> Id = extractConstantInt(Wrapper->getOperand(0).get());
    if (!Id)
      return false;
    Requested = *Id;
    break;
  }
  case NamedMaximumRegistersINTEL: {
    auto *Name = dyn_cast_or_null<MDString>(Value);
    if (!Name)
      return false;
    if (Name->getString() != AutoINTEL) {
      std::string Msg = "Ignoring unsupported NamedMaximumRegistersINTEL value '" + Name->getString().str() + "'";
      Context->EmitWarning(Msg.c_str(), F);
      return false;
    }
    Requested = 0;
    break;
  }
  default:
    IGC_ASSERT_MESSAGE(0, "Unhandled execution mode");
    return false;
  }

  if (F->hasFnAttribute(NumGRFPerThreadAttr)) {
    Context->EmitWarning(
        "Only one maximum register count is supported per entry point; ignoring subsequent declarations", F);
    return false;
  }

  const uint32_t Legalized = Context->platform.legalizeNumGRF(Requested);
  if (Legalized != Requested) {
    std::string Msg = "Requested maximum of " + std::to_string(Requested) +
                      " registers per thread is not supported on this platform; using " + std::to_string(Legalized);
    Context->EmitWarning(Msg.c_str(), F);
  }

  F->addFnAttr(NumGRFPerThreadAttr, std::to_string(Legalized));

  if (dropNumThreadPerEUAnnotation(F))
    Context->EmitWarning(
        "The kernel specifies both a num-thread-per-eu annotation and a SPIR-V maximum register count; "
        "the maximum register count takes precedence.",
        F);

  return true;
}

bool ConvertSPIRVExecutionModes::dropNumThreadPerEUAnnotation(Function *F) {
  bool Found = false;

  if (F->hasFnAttribute("num-thread-per-eu")) {
    F->removeFnAttr("num-thread-per-eu");
    Found = true;
  }

  auto FuncInfoIt = Metadata->FuncMD.find(F);
  if (FuncInfoIt != Metadata->FuncMD.end()) {
    auto &Annotations = FuncInfoIt->second.UserAnnotations;
    auto NewEnd = std::remove_if(Annotations.begin(), Annotations.end(), [](const std::string &Annotation) {
      return Annotation.rfind("num-thread-per-eu", 0) == 0;
    });
    if (NewEnd != Annotations.end()) {
      Annotations.erase(NewEnd, Annotations.end());
      Found = true;
      MetadataChanged = true;
    }
  }

  return Found;
}

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses ConvertSPIRVExecutionModesNPM::run(Module &M, ModuleAnalysisManager &AM) {
  CodeGenContext *Ctx = AM.getResult<CodeGenContextAnalysis>(M).Ctx;
  MetaDataUtilsResult MD = AM.getResult<MetaDataUtilsAnalysis>(M);
  bool Changed = ConvertSPIRVExecutionModes().run(M, Ctx, MD.ModMD);
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16
