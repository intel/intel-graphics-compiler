/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Common_ISA.h"
#include "Common_ISA_framework.h"
#include "Common_ISA_util.h"
#include "visa_igc_common_header.h"
#ifdef DLL_MODE
#include "RT_Jitter_Interface.h"
#else
#include "JitterDataStruct.h"
#endif
#include "BinaryEncoding.h"
#include "IsaDisassembly.h"
#include "Timer.h"
#include "VISAKernel.h"

#include "DebugInfo.h"
#include "FlowGraph.h"
#include "G4_IR.hpp"
#include "IsaVerification.h"
#include "IGC/common/StringMacros.hpp"
#include "MetadataDumpRA.h"

#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>

// clang-format off
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/Path.h>
#include "common/LLVMWarningsPop.hpp"
// clang-format on

using namespace vISA;

#define IS_GEN_PATH (mBuildOption == VISA_BUILDER_GEN)
#define IS_BOTH_PATH (mBuildOption == VISA_BUILDER_BOTH)
#define IS_GEN_BOTH_PATH                                                       \
  (mBuildOption == VISA_BUILDER_GEN || mBuildOption == VISA_BUILDER_BOTH)
#define IS_VISA_BOTH_PATH                                                      \
  (mBuildOption == VISA_BUILDER_VISA || mBuildOption == VISA_BUILDER_BOTH)

CISA_IR_Builder::~CISA_IR_Builder() {
  for (auto k : m_kernelsAndFunctions) {
    // don't call delete since vISAKernelImpl is allocated in memory pool
    k->~VISAKernelImpl();
  }

  if (needsToFreeWATable) {
    delete m_pWaTable;
  }
}

static const WA_TABLE *CreateVisaWaTable(TARGET_PLATFORM platform,
                                         Stepping step) {
  WA_TABLE *pWaTable = new WA_TABLE;
  memset(pWaTable, 0, sizeof(WA_TABLE));

  if ((platform == GENX_SKL && (step == Step_A || step == Step_B)) ||
      (platform == GENX_BXT && step == Step_A)) {
    VISA_WA_ENABLE(pWaTable, WaHeaderRequiredOnSimd16Sample16bit);
  } else {
    VISA_WA_DISABLE(pWaTable, WaHeaderRequiredOnSimd16Sample16bit);
  }

  if ((platform == GENX_SKL) && (step == Step_A)) {
    VISA_WA_ENABLE(pWaTable, WaSendsSrc1SizeLimitWhenEOT);
  } else {
    VISA_WA_DISABLE(pWaTable, WaSendsSrc1SizeLimitWhenEOT);
  }

  if ((platform == GENX_SKL && (step == Step_A || step == Step_B)) ||
      (platform == GENX_BXT && step == Step_A)) {
    VISA_WA_ENABLE(pWaTable, WaDisallow64BitImmMov);
  } else {
    VISA_WA_DISABLE(pWaTable, WaDisallow64BitImmMov);
  }

  if (platform == GENX_BDW || platform == GENX_CHV || platform == GENX_BXT ||
      platform == GENX_SKL) {
    VISA_WA_ENABLE(pWaTable, WaThreadSwitchAfterCall);
  } else {
    VISA_WA_DISABLE(pWaTable, WaThreadSwitchAfterCall);
  }

  if ((platform == GENX_SKL && step < Step_E) ||
      (platform == GENX_BXT && step <= Step_B)) {
    VISA_WA_ENABLE(pWaTable, WaSrc1ImmHfNotAllowed);
  } else {
    VISA_WA_DISABLE(pWaTable, WaSrc1ImmHfNotAllowed);
  }

  if (platform == GENX_SKL && step == Step_A) {
    VISA_WA_ENABLE(pWaTable, WaDstSubRegNumNotAllowedWithLowPrecPacked);
  } else {
    VISA_WA_DISABLE(pWaTable, WaDstSubRegNumNotAllowedWithLowPrecPacked);
  }

  if ((platform == GENX_SKL && step < Step_C)) {
    VISA_WA_ENABLE(pWaTable, WaDisableMixedModeLog);
    VISA_WA_ENABLE(pWaTable, WaDisableMixedModeFdiv);
    VISA_WA_ENABLE(pWaTable, WaDisableMixedModePow);
  } else {
    VISA_WA_DISABLE(pWaTable, WaDisableMixedModeLog);
    VISA_WA_DISABLE(pWaTable, WaDisableMixedModeFdiv);
    VISA_WA_DISABLE(pWaTable, WaDisableMixedModePow);
  }

  if ((platform == GENX_SKL && step < Step_C) || platform == GENX_CHV) {
    VISA_WA_ENABLE(pWaTable,
                   WaFloatMixedModeSelNotAllowedWithPackedDestination);
  } else {
    VISA_WA_DISABLE(pWaTable,
                    WaFloatMixedModeSelNotAllowedWithPackedDestination);
  }

  // always disable in offline mode
  VISA_WA_DISABLE(pWaTable, WADisableWriteCommitForPageFault);

  if ((platform == GENX_SKL && step < Step_D) ||
      (platform == GENX_BXT && step == Step_A)) {
    VISA_WA_ENABLE(pWaTable, WaDisableSIMD16On3SrcInstr);
  }

  if (platform == GENX_SKL && (step == Step_C || step == Step_D)) {
    VISA_WA_ENABLE(pWaTable, WaSendSEnableIndirectMsgDesc);
  } else {
    VISA_WA_DISABLE(pWaTable, WaSendSEnableIndirectMsgDesc);
  }

  if (platform == GENX_SKL || platform == GENX_BXT) {
    VISA_WA_ENABLE(pWaTable, WaClearArfDependenciesBeforeEot);
  }

  if (platform == GENX_SKL && step == Step_A) {
    VISA_WA_ENABLE(pWaTable, WaDisableSendsSrc0DstOverlap);
  }

  if (platform >= GENX_SKL) {
    VISA_WA_ENABLE(pWaTable, WaMixModeSelInstDstNotPacked);
  }

  if (platform == GENX_SKL || platform == GENX_BXT) {
    VISA_WA_ENABLE(pWaTable, WaResetN0BeforeGatewayMessage);
  }

  // WA for future platforms
  if (platform == GENX_ICLLP) {
    VISA_WA_ENABLE(pWaTable, Wa_1406306137);
  }
  if (platform == GENX_ICLLP && (step == Step_A || step == Step_B)) {
    VISA_WA_ENABLE(pWaTable, Wa_2201674230);
  }

  if (platform == Xe_PVCXT)
    VISA_WA_ENABLE(pWaTable, Wa_22011647401);

  if (platform >= Xe_PVC)
    VISA_WA_ENABLE(pWaTable, Wa_13010473643);
  if (platform >= Xe3)
    VISA_WA_DISABLE(pWaTable, Wa_13010473643);

  switch (platform) {
  case GENX_ICLLP:
    VISA_WA_ENABLE(pWaTable, Wa_1406950495);
    break;
  case GENX_TGLLP:
    VISA_WA_ENABLE(pWaTable, Wa_1406950495);
    VISA_WA_ENABLE(pWaTable, Wa_16013338947);
    VISA_WA_ENABLE(pWaTable, Wa_14018126777);
    break;
  case Xe_XeHPSDV:
    VISA_WA_ENABLE(pWaTable, Wa_1406950495);
    VISA_WA_ENABLE(pWaTable, Wa_16013338947);
    break;
  case Xe_DG2:
    VISA_WA_ENABLE(pWaTable, Wa_16013338947);
    if (step == Step_C) {
      VISA_WA_ENABLE(pWaTable, Wa_14017322320);
    }
    break;
  case Xe_PVC:
    VISA_WA_ENABLE(pWaTable, Wa_16013338947);
    break;
  case Xe_PVCXT:
    VISA_WA_ENABLE(pWaTable, Wa_16013338947);
    if (step == Step_A) {
      VISA_WA_ENABLE(pWaTable, Wa_16012725276);
    }
    break;
  case Xe_MTL:
    VISA_WA_ENABLE(pWaTable, Wa_14016880151);
    break;
  case Xe2:
    VISA_WA_ENABLE(pWaTable, Wa_14020375314);
    VISA_WA_ENABLE(pWaTable, Wa_18027439769);
    VISA_WA_ENABLE(pWaTable, Wa_22017182272);
    VISA_WA_ENABLE(pWaTable, Wa_14021891663);
    break;
  case Xe3:
    VISA_WA_ENABLE(pWaTable, Wa_22017182272);
    break;
  default:
    break;
  }

  return pWaTable;
}

// Change default values of some options according to WA_TABLE
// The values are set before parsing any flags specified by client
// (either within CreateVISABuilder() call or via VISABuilder interface)
// and may be overriden by client flags
static void AddWAOptions(Options &options, const WA_TABLE &waTable) {
  if (waTable.Wa_1808850743 || waTable.Wa_1409909237) {
    options.setOptionInternally(vISA_noMaskWA, true);
    // Turn off jmpi as there is no wa for jmpi
    if (!options.getOption(vISA_KeepScalarJmp)) {
      options.setOptionInternally(vISA_EnableScalarJmp, false);
    }
  }
}

int CISA_IR_Builder::CreateBuilder(CISA_IR_Builder *&builder,
                                   vISABuilderMode mode,
                                   VISA_BUILDER_OPTION buildOption,
                                   TARGET_PLATFORM platform, int numArgs,
                                   const char *flags[],
                                   const WA_TABLE *pWaTable) {

  initTimer();

  if (builder) {
    vASSERT(builder == nullptr);
    return VISA_FAILURE;
  }

  startTimer(TimerID::TOTAL);
  startTimer(TimerID::BUILDER); // builder time ends with we call compile (i.e.,
                                // it covers the IR construction time)

  builder =
      new CISA_IR_Builder(platform, buildOption, mode, COMMON_ISA_MAJOR_VER,
                          COMMON_ISA_MINOR_VER, pWaTable);

  if (pWaTable) {
    AddWAOptions(builder->m_options, *pWaTable);
  }

  if (!builder->m_options.parseOptions(numArgs, flags)) {
    delete builder;
    builder = nullptr;
    vISA_ASSERT(false, "parsing error");
    return VISA_FAILURE;
  }

  // Set visa platform in the internal option for the case that IR_Builder or
  // G4_Kernel object are not easily available. However, getting platform
  // through options probably would be slower as it requires a hash table
  // lookup, so it should be used with caution.
  // Check if the given platform from function argument is valid.
  vISA_ASSERT((platform != GENX_NONE && platform < TARGET_PLATFORM::ALL),
        "invalid platform");
  TARGET_PLATFORM platformSet = static_cast<TARGET_PLATFORM>(
      builder->m_options.getuInt32Option(vISA_PlatformSet));
  // If the platform is specified in both the function argument and the
  // cmdline argument, the 2 values probably should be same.
  vISA_ASSERT(platformSet == GENX_NONE || platformSet == platform,
    "invalid platformSet");
  if (platformSet == GENX_NONE)
    builder->m_options.setOptionInternally(vISA_PlatformSet,
                                           static_cast<uint32_t>(platform));

#if defined(_DEBUG) || defined(_INTERNAL)
  builder->m_options.getOptionsFromEV();
#endif

#if !defined(NDEBUG) && !defined(DLL_MODE)
  auto debugPassesCstr = builder->m_options.getOptionCstr(vISA_DebugOnly);
  if (debugPassesCstr) {
    DebugFlag = true;
    std::string debugPasses(debugPassesCstr);
    if (debugPasses == "all")
      DebugAllFlag = true;
    else {
      // Support a comma separated list of pass names.
      for (int firstComma = debugPasses.find_first_of(",");
           firstComma != std::string::npos;
           firstComma = debugPasses.find_first_of(",")) {
        auto name = debugPasses.substr(0, firstComma);
        addPassToDebug(name);
        debugPasses = debugPasses.substr(firstComma + 1);
      }
      addPassToDebug(debugPasses);
    }
  }
#endif // NDEBUG

  // This should not matter anymore since each kernel should set its Target
  // attribute to 3D/CM
  auto targetMode = VISA_3D;
  builder->m_options.setTarget(targetMode);
  builder->m_options.setOptionInternally(vISA_isParseMode,
                                         mode == vISA_ASM_READER);

  // emit location info always for these cases
  if (mode == vISABuilderMode::vISA_DEFAULT &&
      builder->m_options.getOption(vISA_outputToFile)) {
    builder->m_options.setOptionInternally(vISA_EmitLocation, true);
  }

  // driver WaTable is not available in offline vISA executable mode
  // We instead create and initialize some of the known ones here
  if (!pWaTable) {
    builder->m_pWaTable =
        CreateVisaWaTable(platform, builder->m_options.GetStepping());
    builder->needsToFreeWATable = true;
  }

  if (mode == vISA_ASM_WRITER) {
    // If writing asm text, clear the stream and print the build version
    builder->ClearAsmTextStreams();
    builder->m_ssIsaAsm << printBuildVersion(builder->getMajorVersion(),
                                             builder->getMinorVersion())
                        << "\n";
  }

  // Give builder phase a name so that we could control debug information in
  // various translateVisaToG4 functions.
  setCurrentDebugPass("translate");
  return VISA_SUCCESS;
}

int CISA_IR_Builder::DestroyBuilder(CISA_IR_Builder *builder) {

  if (builder == NULL) {
    vASSERT(builder != nullptr);
    return VISA_FAILURE;
  }

  delete builder;

  return VISA_SUCCESS;
}

VISAKernel *
CISA_IR_Builder::GetVISAKernel(const std::string &kernelName) const {
  if (kernelName.empty()) {
    if (m_builderMode == vISA_ASM_READER) {
      auto kernel = m_kernelsAndFunctions.front();
      vASSERT(kernel->getIsKernel());
      return static_cast<VISAKernel *>(kernel);
    }
    return static_cast<VISAKernel *>(m_kernel);
  }
  return static_cast<VISAKernel *>(m_nameToKernel.at(kernelName));
}

int CISA_IR_Builder::ClearAsmTextStreams() {
  if (m_builderMode == vISA_ASM_WRITER) {
    m_ssIsaAsm.str(std::string());
    m_ssIsaAsm.clear();
    return VISA_SUCCESS;
  }

  vISA_ASSERT_UNREACHABLE("Should clear streams only in asm text writer mode!");
  return VISA_FAILURE;
}

void CISA_IR_Builder::SetDirectCallFunctionSet(
    const std::unordered_set<std::string> &directCallFunctions) {
  m_directCallFunctions = directCallFunctions;
}

int CISA_IR_Builder::AddKernel(VISAKernel *&kernel, const char *kernelName) {
  if (kernel) {
    vASSERT(kernel == nullptr);
    return VISA_FAILURE;
  }

  unsigned int funcId = this->m_kernel_count++;
  VISAKernelImpl *kerneltemp = new (m_mem)
      VISAKernelImpl(VISA_BUILD_TYPE::KERNEL, this, kernelName, funcId);
  kernel = static_cast<VISAKernel *>(kerneltemp);
  m_kernel = kerneltemp;

  m_kernelsAndFunctions.push_back(kerneltemp);
  this->m_nameToKernel[kernelName] = m_kernel;

  if (m_builderMode == vISA_ASM_WRITER) {
    m_ssIsaAsm << "//// KERNEL: ////\n";
    VISAKernel_format_provider fmt(m_kernel);
    m_ssIsaAsm << printFunctionDecl(&fmt, true) << "\n";
  }
  return VISA_SUCCESS;
}

int CISA_IR_Builder::SetPrevKernel(VISAKernel *&prevKernel) {
  if (prevKernel) {
    m_prevKernel = (VISAKernelImpl *)prevKernel;
  }
  return VISA_SUCCESS;
}

int CISA_IR_Builder::AddFunction(VISAFunction *&function,
                                 const char *functionName) {
  if (function) {
    vASSERT(function == nullptr);
    return VISA_FAILURE;
  }

  unsigned int funcId = this->m_function_count++;
  VISAKernelImpl *kerneltemp = new (m_mem)
      VISAKernelImpl(VISA_BUILD_TYPE::FUNCTION, this, functionName, funcId);
  function = static_cast<VISAFunction *>(kerneltemp);
  m_kernel = kerneltemp;
  m_kernelsAndFunctions.push_back(kerneltemp);
  this->m_nameToKernel[functionName] = m_kernel;

  if (m_builderMode == vISA_ASM_WRITER) {
    m_ssIsaAsm << "\n"
               << "//// FUNCTION: ////\n";
    VISAKernel_format_provider fmt(m_kernel);
    m_ssIsaAsm << printFunctionDecl(&fmt, false) << "\n";
  }
  return VISA_SUCCESS;
}

int CISA_IR_Builder::AddPayloadSection(VISAFunction *&function,
                                       const char *functionName) {
  if (function) {
    vASSERT(function == nullptr);
    return VISA_FAILURE;
  }

  unsigned int funcId = this->m_function_count++;
  VISAKernelImpl *kerneltemp = new (m_mem)
      VISAKernelImpl(VISA_BUILD_TYPE::PAYLOAD, this, functionName, funcId);
  function = static_cast<VISAFunction *>(kerneltemp);
  m_kernel = kerneltemp;
  m_kernelsAndFunctions.push_back(kerneltemp);
  this->m_nameToKernel[functionName] = m_kernel;

  return VISA_SUCCESS;
}

// default size of the physical reg pool mem manager in bytes
#define PHY_REG_MEM_SIZE (16 * 1024)

void restoreFCallState(G4_Kernel *kernel,
                       const std::map<G4_BB *, G4_INST *> &savedFCallState) {
  // Iterate over all BBs in kernel and fix all fcalls converted
  // to calls by reconverting them to fcall. This is required
  // because we want to reuse IR of function for next kernel.

  for (auto &&iter : savedFCallState) {
    auto curBB = iter.first;
    auto genOffset = curBB->back()->getGenOffset();
    curBB->pop_back();
    auto origInst = iter.second;
    vASSERT(origInst->isFCall() || origInst->isFReturn());
    curBB->push_back(origInst);
    // set the genOffset in case of GenOffset being used when creating symbol
    // table
    origInst->setGenOffset(genOffset);

    if (origInst->isFCall() && !origInst->asCFInst()->isIndirectCall()) {
      // curBB must have a physical successor as we don't allow calls that do
      // not return
      G4_BB *retBlock = curBB->getPhysicalSucc();
      G4_BB *retbbToConvert = retBlock->Preds.back();
      kernel->fg.removePredSuccEdges(retbbToConvert, retBlock);
      // Remove edge between call and previously joined function
      while (curBB->Succs.size() > 0) {
        kernel->fg.removePredSuccEdges(curBB, curBB->Succs.front());
      }

      // Restore edge to retBlock
      kernel->fg.addPredSuccEdges(curBB, retBlock);
    }
  }

  // Remove all in-edges to stack call function. These may have been added
  // to connect earlier kernels with the function.
  while (kernel->fg.getEntryBB()->Preds.size() > 0) {
    kernel->fg.removePredSuccEdges(kernel->fg.getEntryBB()->Preds.front(),
                                   kernel->fg.getEntryBB());
  }
}

G4_Kernel *CISA_IR_Builder::GetCallerKernel(G4_INST *inst) {
  return &inst->getBuilder().kernel;
}

G4_Kernel *CISA_IR_Builder::GetCalleeKernel(G4_INST *fcall) {
  vASSERT(fcall->opcode() == G4_pseudo_fcall);
  auto asLabel = fcall->getSrc(0)->asLabel();
  if (!asLabel) {
    return nullptr;
  }
  std::string funcName = asLabel->getLabelName();
  auto iter = functionsNameMap.find(funcName);
  if (iter != functionsNameMap.end()) {
    return iter->second;
  } else {
    return nullptr;
  }
}

void CISA_IR_Builder::ResetHasStackCall(
    std::list<std::list<vISA::G4_INST *>::iterator> &sgInvokeList,
    std::unordered_map<G4_Kernel *, std::list<std::list<G4_INST *>::iterator>>
        &callSites) {
  for (auto &[func, callsites] : callSites) {
    bool hasStackCall = false;
    for (auto &it : callsites) {
      G4_INST *fcall = *it;
      vASSERT(fcall->opcode() == G4_pseudo_fcall);
      bool isInSgInvokeList = false;
      for (auto &it2 : sgInvokeList) {
        G4_INST *inst = *it2;
        if (inst == fcall) {
          isInSgInvokeList = true;
        }
      }
      if (!isInSgInvokeList) {
        hasStackCall = true;
        break;
      }
    }
    if (!hasStackCall) {
      func->fg.resetHasStackCalls();
    }
  }
}

void CISA_IR_Builder::CheckHazardFeatures(
    std::list<std::list<vISA::G4_INST *>::iterator> &sgInvokeList,
    std::unordered_map<G4_Kernel *, std::list<std::list<G4_INST *>::iterator>>
        &callSites) {
  std::function<void(G4_Kernel *, G4_Kernel *, std::set<G4_Kernel *> &)>
      traverse;
  traverse = [&](G4_Kernel *root, G4_Kernel *func,
                 std::set<G4_Kernel *> &visited) {
    for (auto &it : callSites[func]) {
      G4_INST *fcall = *it;
      vASSERT(fcall->opcode() == G4_pseudo_fcall);
      vASSERT(func == GetCallerKernel(fcall));
      G4_Kernel *callee = GetCalleeKernel(fcall);

      vISA_ASSERT(root != callee, "Detected a recursion that is not allowed");

      if (visited.count(callee))
        continue;

      visited.insert(callee);
      traverse(root, callee, visited);
    }
  };

  for (auto &it : sgInvokeList) {
    G4_INST *fcall = *it;
    // Check if there is a indirect call
    vISA_ASSERT(!fcall->asCFInst()->isIndirectCall(),
           "Not supported for indirect calls");

    // Check recursion
    std::set<G4_Kernel *> visited;
    G4_Kernel *root = GetCalleeKernel(fcall);
    visited.insert(root);
    traverse(root, root, visited);
  }
}

void CISA_IR_Builder::CollectCallSites(
    KernelListTy &functions,
    std::unordered_map<G4_Kernel *, std::list<std::list<G4_INST *>::iterator>>
        &callSites,
    std::list<std::list<vISA::G4_INST *>::iterator> &sgInvokeList) {
  auto IsFCall = [](G4_INST *inst) {
    return inst->opcode() == G4_pseudo_fcall;
  };

  for (auto func : functions) {
    functionsNameMap[std::string(func->getName())] = func->getKernel();
    auto &instList = func->getKernel()->fg.builder->instList;
    std::list<G4_INST *>::iterator it = instList.begin();
    while (it != instList.end()) {
      if (!IsFCall(*it)) {
        it++;
        continue;
      }
      callSites[func->getKernel()].push_back(it);
      it++;
    }
  }

  // get sgInvokeList
  for (auto &[func, callsites] : callSites) {
    for (auto &it : callsites) {
      G4_INST *fcall = *it;
      vASSERT(fcall->opcode() == G4_pseudo_fcall);
      // When callee is a invoke_simd target
      auto callee = GetCalleeKernel(fcall);
      if (callee && callee->fg.builder && callee->getBoolKernelAttr(
          Attributes::ATTR_LTOInvokeOptTarget)) {
        sgInvokeList.push_back(it);
      }
    }
  }
}

void CISA_IR_Builder::RemoveOptimizingFunction(
    const std::list<std::list<vISA::G4_INST *>::iterator> &sgInvokeList) {
  std::set<G4_Kernel *> removeList;
  for (auto &it : sgInvokeList) {
    G4_INST *fcall = *it;
    removeList.insert(GetCalleeKernel(fcall));
  }

  llvm::erase_if(m_kernelsAndFunctions, [&](VISAKernelImpl *k) {
    return removeList.count(k->getKernel());
  });

}

void CISA_IR_Builder::ProcessSgInvokeList(
    const std::list<std::list<vISA::G4_INST *>::iterator> &sgInvokeList,
    std::unordered_map<G4_Kernel *,
                       std::list<std::list<vISA::G4_INST *>::iterator>>
        &callee2Callers) {
  for (auto &it : sgInvokeList) {
    G4_INST *fcall = *it;
    G4_Kernel *callee = GetCalleeKernel(fcall);
    callee2Callers[callee].push_back(it);
  }
}

void CISA_IR_Builder::PropagateInfo(
    vISA::G4_Kernel *caller, vISA::G4_Kernel *callee) {
  // Need to propagate HasStackCall from the caller to callee after LTO
  bool callerHasStackCall =
    caller->fg.getHasStackCalls() ||
    callee->fg.getHasStackCalls();
  if (callerHasStackCall)
    caller->fg.setHasStackCalls();
}

// #define DEBUG_LTO
#ifdef DEBUG_LTO
#define DEBUG_PRINT(msg) std::cerr << __LINE__ << " " << msg;
#define DEBUG_UTIL(stmt) stmt;
#else
#define DEBUG_PRINT(msg)
#define DEBUG_UTIL(stmt)
#endif

// Perform LTO including transforming stack calls to subroutine calls,
// subroutine calls to jumps, and inlining
void CISA_IR_Builder::LinkTimeOptimization(
    std::unordered_map<G4_Kernel *,
                       std::list<std::list<vISA::G4_INST *>::iterator>>
        &callee2Callers,
    uint32_t options) {
  bool call2jump = options & (1U << Linker_Call2Jump);
  std::map<G4_INST *, std::list<G4_INST *>::iterator> callsite;
  std::map<G4_INST *, std::list<G4_INST *>> rets;
  std::set<G4_Kernel *> visited;
  std::list<G4_INST *> dummyContainer;
  unsigned int raUID = 0;
  unsigned int funcUID = 0;

  auto initializeRedirectMap = [&](
      IR_Builder *callee,
      IR_Builder *caller,
      std::map<G4_Declare*, G4_Declare*> &redirectMap) {
    redirectMap.insert(std::make_pair(callee->getFE_SP(), caller->getFE_SP()));
    redirectMap.insert(std::make_pair(callee->getFE_FP(), caller->getFE_FP()));
    redirectMap.insert(std::make_pair(callee->getStackCallArg(), caller->getStackCallArg()));
    redirectMap.insert(std::make_pair(callee->getStackCallRet(), caller->getStackCallRet()));
    redirectMap.insert(std::make_pair(callee->getRealR0(), caller->getRealR0()));
    redirectMap.insert(std::make_pair(callee->getBuiltinR0(), caller->getBuiltinR0()));
    redirectMap.insert(std::make_pair(callee->getBuiltinA0(), caller->getBuiltinA0()));
    redirectMap.insert(std::make_pair(callee->getBuiltinA0Dot2(), caller->getBuiltinA0Dot2()));
    redirectMap.insert(std::make_pair(callee->getBuiltinHWTID(), caller->getBuiltinHWTID()));
    redirectMap.insert(std::make_pair(callee->getBuiltinSR0Dot1(), caller->getBuiltinSR0Dot1()));
    redirectMap.insert(std::make_pair(callee->getBuiltinS0(), caller->getBuiltinS0()));
    redirectMap.insert(std::make_pair(callee->getBuiltinT252(), caller->getBuiltinT252()));
    redirectMap.insert(std::make_pair(callee->getBuiltinBindlessSampler(), caller->getBuiltinBindlessSampler()));
    redirectMap.insert(std::make_pair(callee->getBuiltinSamplerHeader(), caller->getBuiltinSamplerHeader()));
    redirectMap.insert(std::make_pair(callee->getBuiltinScratchSurface(), caller->getBuiltinScratchSurface()));
  };

  // append instructions from callee to caller
  for (auto &[callee, sgInvokeList] : callee2Callers) {
    G4_Declare *replacedArgDcl = nullptr;
    G4_Declare *replacedRetDcl = nullptr;

    for (auto &it : sgInvokeList) {
      G4_INST *fcall = *it;
      vASSERT(fcall->opcode() == G4_pseudo_fcall);
      G4_Kernel *caller = GetCallerKernel(fcall);
      G4_Kernel *callee = GetCalleeKernel(fcall);

      std::map<G4_Declare*, G4_Declare*> redirectMap;
      initializeRedirectMap(callee->fg.builder, caller->fg.builder, redirectMap);

      bool inlining = (options & (1U << Linker_Inline));
      bool removeArgRet = (options & (1U << Linker_RemoveArgRet));
      bool removeStackArg = (options & (1U << Linker_RemoveStackArg)) && caller->fg.builder->hasInt64Add();
      bool removeStackFrame = (options & (1U << Linker_RemoveStackFrame));

      PropagateInfo(caller, callee);

      G4_INST *calleeLabel = *callee->fg.builder->instList.begin();
      vISA_ASSERT(calleeLabel->isLabel() == true, "Entry inst is not a label");

      // Change fcall to call
      // FIXME: This is dangerous, also I think we need to convert the label's
      // kind from FUNCTION to SUBROUTINE.
      fcall->setOpcode(G4_call);
      fcall->setSrc(calleeLabel->getSrc(0), 0);
      // we only record a single callsite to the target in order to convert to
      // jumps note that we don't need call2jump when inlining kicks in
      if ((!inlining) && callsite.find(calleeLabel) == callsite.end()) {
        callsite[calleeLabel] = it;
      } else {
        callsite[calleeLabel] = dummyContainer.end();
      }

      auto &callerInsts = caller->fg.builder->instList;
      auto calleeInsts = callee->fg.builder->instList;

      if (removeArgRet) {
        auto &calleeBuilder = callee->fg.builder;
        auto &callerBuilder = caller->fg.builder;
        replacedArgDcl =
            replacedArgDcl
                ? replacedArgDcl
                : callerBuilder->createDeclare(
                      "newArg", G4_GRF, callerBuilder->numEltPerGRF<Type_UD>(),
                      32, Type_UD);
        replacedRetDcl =
            replacedRetDcl
                ? replacedRetDcl
                : callerBuilder->createDeclare(
                      "newRet", G4_GRF, callerBuilder->numEltPerGRF<Type_UD>(),
                      12, Type_UD);

        for (G4_INST *inst : calleeInsts) {
          for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
            G4_Operand *src = inst->getSrc(i);
            if (!src)
              continue;
            G4_Declare *topDcl = src->getTopDcl();
            if (!topDcl)
              continue;
            G4_Declare *rootDcl = topDcl->getRootDeclare();
            if (calleeBuilder->isPreDefArg(rootDcl)) {
              G4_Operand *replacedArgSrc = callerBuilder->createSrc(
                  replacedArgDcl->getRegVar(),
                  src->asSrcRegRegion()->getRegOff(),
                  src->asSrcRegRegion()->getSubRegOff(),
                  src->asSrcRegRegion()->getRegion(), src->getType());
              replacedArgSrc->asSrcRegRegion()->setModifier(
                  src->asSrcRegRegion()->getModifier());
              inst->setSrc(replacedArgSrc, i);
            }
          }

          G4_Operand *dst = inst->getDst();
          if (!dst)
            continue;
          G4_Declare *topDcl = dst->getTopDcl();
          if (!topDcl)
            continue;
          G4_Declare *rootDcl = topDcl->getRootDeclare();

          if (calleeBuilder->isPreDefRet(rootDcl)) {
            G4_DstRegRegion *replacedRetDst = callerBuilder->createDst(
                replacedRetDcl->getRegVar(), dst->asDstRegRegion()->getRegOff(),
                dst->asDstRegRegion()->getSubRegOff(),
                dst->asDstRegRegion()->getHorzStride(), dst->getType());
            inst->setDest(replacedRetDst);
          }
        }

        // Trace backward from callsite to replace Arg with newArg
        auto rIt = it;
        rIt--;
        for (; rIt != callerInsts.begin(); --rIt) {
          G4_INST *inst = *rIt;
          if (inst->opcode() == G4_pseudo_fcall || inst->opcode() == G4_call) {
            break;
          }
          G4_Operand *dst = inst->getDst();
          if (!dst)
            continue;
          G4_Declare *topDcl = dst->getTopDcl();
          if (!topDcl)
            continue;
          G4_Declare *rootDcl = topDcl->getRootDeclare();
          if (callerBuilder->isPreDefArg(rootDcl)) {
            G4_Declare *dcl = dst->getBase()->asRegVar()->getDeclare();
            G4_Declare *newDcl = callerBuilder->createTempVar(
                dcl->getTotalElems(), dcl->getElemType(), Any, dcl->getName());
            newDcl->setAliasDeclare(replacedArgDcl, dcl->getAliasOffset());
            G4_DstRegRegion *replacedArgDst = callerBuilder->createDst(
                newDcl->getRegVar(), dst->asDstRegRegion()->getRegOff(),
                dst->asDstRegRegion()->getSubRegOff(),
                dst->asDstRegRegion()->getHorzStride(), dst->getType());
            inst->setDest(replacedArgDst);
          }
        }
        auto fIt = it;
        fIt++;
        for (; fIt != callerInsts.end(); ++fIt) {
          G4_INST *inst = *fIt;
          if (inst->opcode() == G4_pseudo_fcall || inst->opcode() == G4_call) {
            break;
          }
          for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
            G4_Operand *src = inst->getSrc(i);
            if (!src)
              continue;
            G4_Declare *topDcl = src->getTopDcl();
            if (!topDcl)
              continue;
            G4_Declare *rootDcl = topDcl->getRootDeclare();
            if (callerBuilder->isPreDefRet(rootDcl)) {
              G4_Operand *replacedRetSrc = callerBuilder->createSrc(
                  replacedRetDcl->getRegVar(),
                  src->asSrcRegRegion()->getRegOff(),
                  src->asSrcRegRegion()->getSubRegOff(),
                  src->asSrcRegRegion()->getRegion(), src->getType());
              replacedRetSrc->asSrcRegRegion()->setModifier(
                  src->asSrcRegRegion()->getModifier());
              inst->setSrc(replacedRetSrc, i);
            }
          }
        }
      }

      // A hash map to record how SP is populated from caller to callee
      std::map<G4_Declare *, long long> stackPointers;
      // A hash map to record where the instruction is on defs
      std::map<G4_Declare *, std::list<vISA::G4_INST *>::iterator> defInst;

      if (removeStackArg) {
        // collect instructions which store args to stack
        auto &callerBuilder = caller->fg.builder;
        auto &calleeBuilder = callee->fg.builder;
        auto getPointerOffset = [&](G4_INST *inst, long long offset) {
          [[maybe_unused]] auto execSize = static_cast<int>(inst->getExecSize());
          auto typeSize = inst->getSrc(0)->getTypeSize() * 8;
          switch (inst->opcode()) {
          case G4_mov: {
            return offset;
          }
          case G4_add:
          case G4_addc: {
            vASSERT(execSize == 1);
            if (typeSize == 32 && inst->opcode() == G4_add) {
              return offset;
            }
            vASSERT(inst->getSrc(1)->isImm());
            return offset + inst->getSrc(1)->asImm()->getImm();
          }
          default: {
            vISA_ASSERT_UNREACHABLE("invalid inst opcode");
            return 0LL;
          }
          }
        };

        auto getRootDeclare = [&](G4_Operand *opnd) {
          if (!opnd)
            return (G4_Declare *)nullptr;
          G4_Declare *topDcl = opnd->getTopDcl();
          if (!topDcl)
            return (G4_Declare *)nullptr;
          return topDcl->getRootDeclare();
        };

        auto getBeginIt = [&](std::list<vISA::G4_INST *>::iterator it) {
          // Trace backward until it reaches an update for SP
          // This is where we start to push spilled arguments onto stack
          auto beginIt = it;
          for (; beginIt != callerInsts.begin(); --beginIt) {
            G4_INST *inst = *beginIt;
            for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
              G4_Declare *rootDcl = getRootDeclare(inst->getSrc(i));
              if (!rootDcl)
                continue;
              G4_Operand *dst = inst->getDst();
              if (rootDcl == callerBuilder->getFE_SP()) {
                // the dst is updating SP
                if (dst->getTopDcl() == callerBuilder->getFE_SP()) {
                  auto prevIt = beginIt;
                  prevIt--;
                  G4_INST *prevInst = *prevIt;
                  // It reaches the begining of function where it pushes a new
                  // frame. It is not where we are looking for.
                  if (prevInst->getDst()->getTopDcl() ==
                      callerBuilder->getFE_FP()) {
                    return it;
                  } else {
                    return beginIt;
                  }
                }
              }
            }
          }
          return it;
        };
        std::set<G4_Declare*> visitedDcl;
        std::function<void(G4_Operand *, INST_LIST &, G4_Declare *)>
            removeDeadCode;
        removeDeadCode = [&](G4_Operand *src, INST_LIST &instList,
                             G4_Declare *stopDcl) {
          if (!src || src->getTopDcl() == stopDcl ||
              defInst.find(src->getTopDcl()) == defInst.end()) {
            return;
          }
          auto dcl = src->getTopDcl();
          if (visitedDcl.find(dcl) != visitedDcl.end())
              return;
          visitedDcl.insert(dcl);
          auto instIt = defInst[dcl];
          G4_INST *inst = *instIt;
          if (static_cast<int>(inst->getExecSize() > 2)) {
            return;
          }
          if (inst->opcode() == G4_mov || inst->opcode() == G4_add) {
            DEBUG_PRINT("removeFrame (" << stackPointers[dcl]
                                        << ") ");
            DEBUG_UTIL(inst->dump());
            instList.erase(instIt);
            removeDeadCode(inst->getSrc(0), instList, stopDcl);
          }
        };

        // A list of load/store in the caller in order to perform store-to-load forwarding
        std::list<std::list<vISA::G4_INST *>::iterator> memopList;
        std::list<std::list<vISA::G4_INST *>::iterator> callerFrameInstList;
        std::list<std::list<vISA::G4_INST *>::iterator> calleeFrameInstList;

        auto callsiteIt = it;
        auto beginIt = getBeginIt(it);
        bool noArgOnStack = (beginIt == it);
        for (auto callerIt = beginIt; callerIt != callerInsts.end(); callerIt++) {
          if (callerIt == callsiteIt) {
            // passing SP offset from caller to callee upon reaching the callsite
            stackPointers[calleeBuilder->getFE_SP()] =
                stackPointers[callerBuilder->getFE_SP()];
            stackPointers[calleeBuilder->getFE_FP()] =
                stackPointers[callerBuilder->getFE_FP()];

            if (removeStackFrame) {
              if (defInst.find(callerBuilder->getFE_SP()) != defInst.end()) {
                DEBUG_PRINT("Add frame inst to removal list:");
                DEBUG_UTIL((*defInst[callerBuilder->getFE_SP()])->dump());
                callerFrameInstList.push_back(defInst[callerBuilder->getFE_SP()]);
              }
              if (defInst.find(calleeBuilder->getFE_SP()) != defInst.end()) {
                DEBUG_PRINT("Add frame inst to removal list:");
                DEBUG_UTIL((*defInst[calleeBuilder->getFE_SP()])->dump());
                calleeFrameInstList.push_back(defInst[calleeBuilder->getFE_SP()]);
              }
              if (defInst.find(calleeBuilder->getFE_FP()) != defInst.end()) {
                DEBUG_PRINT("Add frame inst to removal list:");
                DEBUG_UTIL((*defInst[calleeBuilder->getFE_FP()])->dump());
                calleeFrameInstList.push_back(defInst[calleeBuilder->getFE_FP()]);
              }
            }
            continue;
          }
          // Scan the caller to gather store/load instructions
          G4_INST *inst = *callerIt;
          for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
            G4_Declare *rootDcl = getRootDeclare(inst->getSrc(i));
            if (!rootDcl)
              continue;
            G4_Operand *dst = inst->getDst();
            if (rootDcl == callerBuilder->getFE_SP()) {
              stackPointers[dst->getTopDcl()] =
                  getPointerOffset(inst, stackPointers[rootDcl]);
              defInst[dst->getTopDcl()] = callerIt;
              DEBUG_PRINT("(" << stackPointers[dst->getTopDcl()] << ") ");
              DEBUG_UTIL(inst->dump());
            } else if (stackPointers.find(rootDcl) != stackPointers.end()) {
              long long offset = stackPointers[rootDcl];
              if (inst->opcode() == G4_mov || inst->opcode() == G4_add ||
                  inst->opcode() == G4_addc) {
                auto execSize = static_cast<int>(inst->getExecSize());
                if (execSize != 1) {
                  // Currently only support scalar type of operations
                  DEBUG_PRINT("skip nonaddress calc\n");
                  DEBUG_UTIL(inst->dump());
                  continue;
                }
                stackPointers[dst->getTopDcl()] =
                    getPointerOffset(inst, offset);
                defInst[dst->getTopDcl()] = callerIt;
                DEBUG_PRINT("(" << stackPointers[dst->getTopDcl()] << ") ");
                DEBUG_UTIL(inst->dump());
              } else if (inst->isSendUnconditional()) {
                vASSERT(i == 0);
                // Start adding argument stores to the list
                memopList.push_back(callerIt);
                DEBUG_PRINT("[ ]");
                DEBUG_UTIL(inst->dump());
              } else {
                vISA_ASSERT(false, "not implemented");
              }
            }
          }
        }

        for (auto thisIt = calleeInsts.begin();
             thisIt != calleeInsts.end();) {
          // Scan callee instructions to match memops in the list
          // to perform store to load forwarding
          auto calleeIt = thisIt;
          thisIt++;
          G4_INST *inst = *calleeIt;
          for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
            G4_Declare *rootDcl = getRootDeclare(inst->getSrc(i));
            if (!rootDcl)
              continue;
            auto dst = inst->getDst();
            if (rootDcl == calleeBuilder->getFE_SP()) {
              stackPointers[dst->getTopDcl()] =
                  getPointerOffset(inst, stackPointers[rootDcl]);
              defInst[dst->getTopDcl()] = calleeIt;
              DEBUG_PRINT("(" << stackPointers[dst->getTopDcl()] << ") ");
              DEBUG_UTIL(inst->dump());
            } else if (stackPointers.find(rootDcl) != stackPointers.end()) {
              long long offset = stackPointers[rootDcl];
              auto execSize = static_cast<int>(inst->getExecSize());
              if (inst->opcode() == G4_add &&
                  dst->getTopDcl()->getElemSize() == 4) {
                vASSERT(execSize == 1);
                defInst[dst->getTopDcl()] = calleeIt;
                DEBUG_PRINT("(" << stackPointers[dst->getTopDcl()]
                                << ") 64-bit emulated Hi32 ");
                DEBUG_UTIL(inst->dump());
              } else if (inst->opcode() == G4_mov || inst->opcode() == G4_add ||
                         inst->opcode() == G4_addc) {
                if (execSize > 2) {
                  DEBUG_PRINT("Giving up tracking: ");
                  DEBUG_UTIL(inst->dump());
                  continue;
                }
                stackPointers[dst->getTopDcl()] =
                    getPointerOffset(inst, offset);
                defInst[dst->getTopDcl()] = calleeIt;
                DEBUG_PRINT("(" << stackPointers[dst->getTopDcl()] << ") ");
                DEBUG_UTIL(inst->dump());
              } else if (inst->isSendUnconditional()) {
                if (memopList.empty()) {
                  // store prevFP to the callee's frame
                  if (stackPointers[callerBuilder->getFE_SP()] ==
                      stackPointers[getRootDeclare(inst->getSrc(0))]) {
                    DEBUG_PRINT("remove prevFP on callee's frame:\n");
                    DEBUG_UTIL(inst->dump());
                    calleeInsts.erase(calleeIt);
                    removeDeadCode(inst->getSrc(0), calleeInsts,
                                   calleeBuilder->getFE_FP());
                    // Cannot remove inst->getSrc(1) in some cases
                    // old %fp can be used upon return to restore FP
                    // e.g.
                    // mov (M1_NM, 2) FP_1(0,0)<1> %fp(0,0)<1;1,0>   <- cannot
                    // remove mov (M1_NM, 2) FP_2(0,0)<1> %fp(0,0)<1;1,0>   <-
                    // dead code removed by inst->getSrc(0) svm_block_st (1)
                    // FP_2(0,0)<0;1,0> FP_1.0      <- removed prevFP
                    // ...
                    // mov (M1_NM, 2) %fp(0,0)<1> FP_1(0,0)<1;1,0>
                    // fret
                    if (removeStackFrame) {
                      DEBUG_PRINT("Add frame inst to removal list:");
                      DEBUG_UTIL((*defInst[calleeBuilder->getFE_SP()])->dump());
                      calleeFrameInstList.push_back(defInst[calleeBuilder->getFE_SP()]);
                      DEBUG_PRINT("Add frame inst to removal list:");
                      DEBUG_UTIL((*defInst[calleeBuilder->getFE_FP()])->dump());
                      calleeFrameInstList.push_back(defInst[calleeBuilder->getFE_FP()]);
                    }
                    break;
                  } else {
                    DEBUG_PRINT("skip for now (private variable on the "
                                "callee's frame):\n");
                    DEBUG_UTIL(inst->dump());
                    vASSERT(false);
                  }
                }
                auto callerIt = memopList.front();
                bool isStoreAtCaller = (*callerIt)->getDst()->isNullReg();
                bool isLoadAtCallee = !inst->getDst()->isNullReg();
                vISA_ASSERT(!(isStoreAtCaller ^ isLoadAtCallee), "Must be a pair of load and store");
                G4_INST *storeInst = isStoreAtCaller ? *callerIt : inst;
                G4_INST *loadInst  = isLoadAtCallee  ? inst : *callerIt;
                auto storeIt       = isStoreAtCaller ? callerIt : calleeIt;
                auto loadIt        = isLoadAtCallee  ? calleeIt : callerIt;
                auto storeBuilder  = isStoreAtCaller ? callerBuilder : calleeBuilder;
                auto loadBuilder   = isLoadAtCallee  ? calleeBuilder : callerBuilder;
                auto &storeInsts   = isStoreAtCaller ? callerInsts : calleeInsts;
                auto &loadInsts    = isLoadAtCallee  ? calleeInsts : callerInsts;
                memopList.pop_front();
                DEBUG_PRINT("store-to-load forwarding:\n");
                DEBUG_PRINT("\tstore:\t");
                DEBUG_UTIL(storeInst->dump());
                DEBUG_PRINT("\tload :\t");
                DEBUG_UTIL(loadInst->dump());
                vISA_ASSERT(stackPointers[getRootDeclare(storeInst->getSrc(0))] ==
                           stackPointers[getRootDeclare(loadInst->getSrc(0))],
                       "Store and load have different SP offset");
                removeDeadCode(storeInst->getSrc(0), storeInsts,
                               storeBuilder->getFE_SP());
                removeDeadCode(loadInst->getSrc(0), loadInsts,
                               loadBuilder->getFE_SP());
                // promote the load into mov
                auto newSrc = storeInst->getSrc(1);
                auto dst = loadInst->getDst();
                if (isStoreAtCaller)
                  redirectMap.insert(std::make_pair(dst->getTopDcl(), newSrc->getTopDcl()));
                else
                  redirectMap.insert(std::make_pair(newSrc->getTopDcl(), dst->getTopDcl()));
                // erase the store
                loadInsts.erase(loadIt);
                storeInsts.erase(storeIt);
              } else {
                vISA_ASSERT(false, "not implemented");
              }
            }
          }
        }

        // All args has been removed on the stack
        // Remove SP updating instruction
        if (memopList.empty() && !noArgOnStack && removeStackFrame) {
          if (defInst.find(callerBuilder->getFE_SP()) != defInst.end()) {
            DEBUG_PRINT("Add frame inst to removal list:");
            DEBUG_UTIL((*defInst[callerBuilder->getFE_SP()])->dump());
            callerFrameInstList.push_back(defInst[callerBuilder->getFE_SP()]);
          }
          if (defInst.find(calleeBuilder->getFE_SP()) != defInst.end()) {
            DEBUG_PRINT("Add frame inst to removal list:");
            DEBUG_UTIL((*defInst[calleeBuilder->getFE_SP()])->dump());
            calleeFrameInstList.push_back(defInst[calleeBuilder->getFE_SP()]);
          }
          if (defInst.find(calleeBuilder->getFE_FP()) != defInst.end()) {
            DEBUG_PRINT("Add frame inst to removal list:");
            DEBUG_UTIL((*defInst[calleeBuilder->getFE_FP()])->dump());
            calleeFrameInstList.push_back(defInst[calleeBuilder->getFE_FP()]);
          }
        }

        for (const auto& it : callerFrameInstList) {
          callerInsts.erase(it);
        }
        for (const auto& it : calleeFrameInstList) {
          removeDeadCode((*it)->getSrc(0), calleeInsts, calleeBuilder->getFE_FP());
          calleeInsts.erase(it);
        }
      }

      std::map<G4_Declare *, G4_Declare *> newDclMap;

      auto redirectBuiltin = [&](
          G4_Operand *opd,
          G4_Declare *dcl,
          G4_Declare *redirectDcl) {
        G4_Declare *newDcl =
          caller->fg.builder->cloneDeclare(newDclMap, dcl);
        opd->setTopDcl(redirectDcl);
        opd->setBase(newDcl->getRegVar());
        newDcl->setAliasDeclare(redirectDcl,
            newDcl->getAliasOffset());

      };

      auto cloneDcl = [&](G4_Operand *opd) {
        if (opd) {
          G4_Declare *topDcl = opd->getTopDcl();
          G4_RegVar *var = opd->isAddrExp() ? opd->asAddrExp()->getRegVar()
                           : opd->getBase() && opd->getBase()->isRegVar()
                               ? opd->getBase()->asRegVar()
                               : nullptr;
          if (!var)
            return;
          G4_Declare *dcl = var->getDeclare();
          if (topDcl && redirectMap.find(topDcl) != redirectMap.end()) {
            redirectBuiltin(opd, dcl, redirectMap[topDcl]);
          } else if (topDcl &&
                     (topDcl == replacedArgDcl || topDcl == replacedRetDcl)) {
            G4_Declare *newDcl = caller->fg.builder->createTempVar(
                dcl->getTotalElems(), dcl->getElemType(), Any, dcl->getName());
            newDcl->setAliasDeclare(topDcl, dcl->getAliasOffset());
          } else {
            vASSERT(!topDcl || !topDcl->isBuiltin());
            G4_Declare *newDcl =
                caller->fg.builder->cloneDeclare(newDclMap, dcl);
            if (opd->isAddrExp()) {
              vASSERT(topDcl == nullptr);
              opd->asAddrExp()->setRegVar(newDcl->getRegVar());
            } else {
              opd->setTopDcl(newDcl->getAliasDeclare()
                                 ? newDcl->getAliasDeclare()
                                 : newDcl);
              opd->setBase(newDcl->getRegVar());
            }
          }
        }
      };
      std::map<G4_Label *, G4_Label *> labelMap;
      if (inlining) {
        auto &builder = caller->fg.builder;
        std::string funcName = fcall->getSrc(0)->asLabel()->getLabelName();
        G4_Label *raLabel = builder->createLabel(
            funcName + "_ret" + std::to_string(raUID++), LABEL_BLOCK);
        G4_INST *ra = caller->fg.createNewLabelInst(raLabel);
        // We don't need calleeLabel (first instruction) anymore after inlining
        calleeInsts.pop_front();
        // Iterate once to clone labels
        for (G4_INST *inst : calleeInsts) {
          if (inst->opcode() == G4_label) {
            std::string name = inst->getLabelStr();
            G4_Label *newLabel =
                builder->createLabel(name + "_" + std::to_string(funcUID),
                                     inst->getLabel()->getLabelKind());
            labelMap[inst->getSrc(0)->asLabel()] = newLabel;
          }
        }

        // clone instructions
        bool isSubroutine = false;
        for (G4_INST *fret : calleeInsts) {
          G4_INST *inst = nullptr;
          if (fret->opcode() == G4_label) {
            inst = caller->fg.createNewLabelInst(
                labelMap[fret->getSrc(0)->asLabel()]);
          } else {
            inst = fret->cloneInst(builder);
          }

          // restore fcall info for indirect calls
          if (inst->opcode() == G4_pseudo_fcall) {
            auto orig_fcallinfo = callee->fg.builder->getFcallInfo(fret);
            if (orig_fcallinfo) {
              builder->addFcallInfo(inst, orig_fcallinfo->getArgSize(),
                                    orig_fcallinfo->getRetSize(),
                                    orig_fcallinfo->isUniform());
            }
          }

          // Based on vISA's assumption, after visiting fret in callee,
          // it is entering subroutines' instructions (callee #7-#9 below)
          // inside the callee.
          // We have to insert subroutines' instructions to the end of
          // the instruction list of caller for inlinig rather than the callsite
          // Example (Before inlining)
          //   _main_0:                 // caller #1
          //   ...                      // caller #2
          //   fcall <<invoke_simd>>    // caller #3
          //
          //   <<invoke_simd_ra>>:      // caller #4
          //   ...                      // caller #5
          //   pseudo_exit(1)           // caller #6
          //
          //   <<invoke_simd>>:         // callee #1
          //   ((invoke_simd_context))  // callee #2
          //   call <<subroutine_call>> // callee #3
          //
          //   <<subroutine_call_ra>>:  // callee #4
          //   ...                      // callee #5
          //   fret (16)                // callee #6
          //
          //   <<subroutine_call>>:     // callee #7
          //   ...                      // callee #8
          //   return (1)               // callee #9
          //
          // If we clone all callee instructions (callee #1 to #9) to the
          // callsite (caller #3), it will break the assumption of vISA in
          // which from the entry (caller #1) to exit (caller #6), vISA cannot
          // have a call-return sequence as subroutines. The correct sequence
          // is to insert from callee #1 to #6 to the callsite and append the
          // rest of the callee's instruction to the end of caller's instruction
          // list.
          if (inst->opcode() == G4_pseudo_fret) {
            vISA_ASSERT(isSubroutine == false, "It is impossible to visit fret twice");
            isSubroutine = true;
          }

          if (inst->opcode() == G4_mov && fret->getSrc(0)->isRightBoundSet()) {
            inst->getSrc(0)->computeRightBound(inst->getExecSize());
          }

          auto isPredefinedAreg = [](G4_Operand *opnd) {
            if (opnd) {
              G4_Declare *topDcl = opnd->getTopDcl();
              if (topDcl && topDcl->isPreDefinedVar() && opnd->isAreg())
                return true;
            }
            return false;
          };

          for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
            auto src = inst->getSrc(i);
            if (!isPredefinedAreg(src)) {
              cloneDcl(src);
            }
          }
          if (inst->opcode() == G4_goto) {
            inst->asCFInst()->setUip(labelMap[fret->asCFInst()->getUip()]);
          } else if ((inst->opcode() == G4_jmpi || inst->isCall()) &&
                     inst->getSrc(0) && inst->getSrc(0)->isLabel()) {
            inst->setSrc(labelMap[fret->getSrc(0)->asLabel()], 0);
          }
          auto dst = inst->getDst();
          if (!isPredefinedAreg(dst)) {
            cloneDcl(dst);
          }
          cloneDcl(inst->getPredicate());
          // add predicate into declaration list
          if (G4_VarBase *flag = inst->getCondModBase()) {
            G4_Declare *newDcl = caller->fg.builder->cloneDeclare(
                newDclMap, flag->asRegVar()->getDeclare());
            inst->getCondMod()->setBase(newDcl->getRegVar());
            inst->getCondMod()->setTopDcl(newDcl);
          }
          callerInsts.insert(isSubroutine ? callerInsts.end() : it, inst);
          if (inst->opcode() != G4_pseudo_fret)
            continue;
          // Change inst to goto
          inst->setOpcode(G4_goto);
          inst->asCFInst()->setUip(raLabel);
        }
        // insert return label for goto
        callerInsts.insert(it, ra);
        // remove the call
        callerInsts.erase(it);
        funcUID++;
      } else {
        // We only have to copy callee's instructions once for subrountine calls
        if (visited.find(callee) != visited.end()) {
          continue;
        }
        visited.insert(callee);
        for (G4_INST *fret : calleeInsts) {
          G4_INST *inst = fret->cloneInst(caller->fg.builder);
          // restore fcall info for indirect calls
          if (inst->opcode() == G4_pseudo_fcall) {
            auto orig_fcallinfo = callee->fg.builder->getFcallInfo(fret);
            if (orig_fcallinfo) {
              caller->fg.builder->addFcallInfo(
                  inst, orig_fcallinfo->getArgSize(),
                  orig_fcallinfo->getRetSize(), orig_fcallinfo->isUniform());
            }
          }
          for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
            cloneDcl(inst->getSrc(i));
          }
          cloneDcl(inst->getDst());
          cloneDcl(inst->getPredicate());
          // add predicate into declaration list
          if (G4_VarBase *flag = inst->getCondModBase()) {
            G4_Declare *newDcl = caller->fg.builder->cloneDeclare(
                newDclMap, flag->asRegVar()->getDeclare());
            inst->getCondMod()->setBase(newDcl->getRegVar());
            inst->getCondMod()->setTopDcl(newDcl);
          }
          if (inst->opcode() == G4_goto) {
            inst->asCFInst()->setUip(fret->asCFInst()->getUip());
          }
          callerInsts.insert(callerInsts.end(), inst);
          if (inst->opcode() != G4_pseudo_fret)
            continue;
          // Change inst to ret
          inst->setOpcode(G4_return);
          rets[calleeLabel].push_back(inst);
        }
      }

      // Merge callee's barrier info into caller
      caller->fg.builder->usedBarries() |= callee->fg.builder->usedBarries();
    }

    callee->fg.builder->~IR_Builder();
    callee->fg.builder = nullptr;
  }

  if (call2jump) {
    for (auto &[label, itCall] : callsite) {
      if (itCall == dummyContainer.end())
        continue;
      G4_INST *call = *itCall;
      G4_Kernel *caller = GetCallerKernel(call);
      auto &builder = caller->fg.builder;
      call->setOpcode(G4_goto);
      call->asCFInst()->setUip(label->getLabel());
      std::string funcName = call->getSrc(0)->asLabel()->getLabelName();
      G4_Label *raLabel = builder->createLabel(funcName + "_ret", LABEL_BLOCK);
      G4_INST *ra = caller->fg.createNewLabelInst(raLabel);
      auto &callerInsts = caller->fg.builder->instList;
      callerInsts.insert(++itCall, ra);

      for (G4_INST *ret : rets[label]) {
        ret->setOpcode(G4_goto);
        ret->asCFInst()->setUip(raLabel);
      }
    }
  }
}

static void retrieveBarrierInfoFromCallee(VISAKernelImpl *entry,
                                          std::set<VISAKernelImpl *> &visited) {
  auto res = visited.insert(entry);
  if (!res.second)
    return;

  for (G4_BB *bb : entry->getKernel()->fg) {
    if (!bb->isEndWithFCall())
      continue;

    G4_INST *fcall = bb->back();
    if (fcall->asCFInst()->isIndirectCall())
      continue;

    const char *funcName = fcall->getSrc(0)->asLabel()->getLabelName();
    VISAKernelImpl *callee = entry->getCISABuilder()->getKernel(funcName);
    // Propagate properties of callee to caller recursively.
    retrieveBarrierInfoFromCallee(callee, visited);
    entry->getIRBuilder()->usedBarries() |=
        callee->getIRBuilder()->usedBarries();
  }
  // numBarriers property is propagated to IGC and onwards in to NEO patch
  // token. We need this logic here to propagate barrier usage to IGC and
  // further to NEO so it can set up WG size appropriately.  Without this
  // setting barrier would cause machine to hang.
  // TODO: How to set numBarriers when callee is indirect in patch token
  // path? For zebin path, the barrier information of an indirect (external)
  // function will be provided in the corresponding .ze_info field.
  entry->getIRBuilder()->getJitInfo()->numBarriers =
      entry->getIRBuilder()->numBarriers();
}

// Stitch the FG of subFunctions to mainFunc
// mainFunc could be a kernel or a non-kernel function.
// It also modifies pseudo_fcall/fret in to call/ret opcodes.
// ToDo: may consider stitching only functions that may be called by this
// kernel/function
static void Stitch_Compiled_Units(G4_Kernel *mainFunc,
                                  std::map<std::string, G4_Kernel *> &subFuncs,
                                  std::map<G4_BB *, G4_INST *> &FCallRetMap) {

  // Append subFunctions to mainFunc
  for (auto &&iter : subFuncs) {
    G4_Kernel *callee = iter.second;
    mainFunc->fg.append(callee->fg);

    // merge the relocation when append
    if (!callee->getRelocationTable().empty())
      mainFunc->getRelocationTable().insert(
          mainFunc->getRelocationTable().end(),
          callee->getRelocationTable().begin(),
          callee->getRelocationTable().end());
  }

  mainFunc->fg.reassignBlockIDs();
  mainFunc->fg
      .setPhysicalPredSucc(); // this is to locate the next BB after an fcall

  auto builder = mainFunc->fg.builder;
  mainFunc->fg.canUpdateFuncInfo = false;
  // Change fcall/fret to call/ret and setup caller/callee edges
  for (G4_BB *cur : mainFunc->fg) {
    if (cur->isEndWithFCall()) {
      // Setup successor/predecessor
      G4_INST *fcall = cur->back();

      if (!fcall->asCFInst()->isIndirectCall()) {
        // Setup caller/callee edges for direct call
        // ToDo: remove this once SWSB is moved before stithcing, as we would
        // not need to maintain CFG otherwise
        std::string funcName = fcall->getSrc(0)->asLabel()->getLabelName();

        auto iter = subFuncs.find(funcName);
        vISA_ASSERT(iter != subFuncs.end(), "can't find function with given name");
        G4_Kernel *callee = iter->second;
        G4_BB *retBlock = cur->Succs.front();
        vISA_ASSERT(cur->Succs.size() == 1,
                    "fcall basic block cannot have more than 1 successor");
        vISA_ASSERT(retBlock->Preds.size() == 1,
                    "block after fcall cannot have more than 1 predecessor");

        // Remove old edge
        retBlock->Preds.erase(retBlock->Preds.begin());
        cur->Succs.erase(cur->Succs.begin());

        // Connect new fg
        mainFunc->fg.addPredSuccEdges(cur, callee->fg.getEntryBB());
        mainFunc->fg.addPredSuccEdges(callee->fg.getUniqueReturnBlock(),
                                      retBlock);

        G4_INST *calleeLabel = callee->fg.getEntryBB()->front();
        vISA_ASSERT(calleeLabel->isLabel() == true, "Entry inst is not label");

        auto callInst = builder->createInternalInst(
            fcall->getPredicate(), G4_call, nullptr, g4::NOSAT,
            fcall->getExecSize(), fcall->getDst(), calleeLabel->getSrc(0),
            fcall->getSrc(0), fcall->getOption());
        callInst->inheritDIFrom(fcall);
        callInst->inheritSWSBFrom(fcall);
        cur->pop_back();
        cur->push_back(callInst);
      } else {
        // src0 is dont care for indirect call as long it's not a label
        auto callInst = builder->createInternalInst(
            fcall->getPredicate(), G4_call, nullptr, g4::NOSAT,
            fcall->getExecSize(), fcall->getDst(), fcall->getSrc(0),
            fcall->getSrc(0), fcall->getOption());
        callInst->inheritDIFrom(fcall);
        callInst->inheritSWSBFrom(fcall);
        cur->pop_back();
        cur->push_back(callInst);
      }
      FCallRetMap[cur] = fcall;
    }
  }

  // Change fret to ret
  for (G4_BB *cur : mainFunc->fg) {
    if (cur->isEndWithFRet()) {
      G4_INST *fret = cur->back();
      auto retInst = builder->createInternalInst(
          fret->getPredicate(), G4_return, nullptr, g4::NOSAT,
          fret->getExecSize(), builder->createNullDst(Type_UD), fret->getSrc(0),
          fret->getSrc(1), fret->getOption());
      retInst->inheritDIFrom(fret);
      retInst->inheritSWSBFrom(fret);
      cur->pop_back();
      cur->push_back(retInst);
      FCallRetMap[cur] = fret;
    }
  }

  // Append declarations and color attributes from all callees to mainFunc
  for (const auto &iter : subFuncs) {
    G4_Kernel *callee = iter.second;
    for (const auto &curDcl : callee->Declares) {
      mainFunc->Declares.push_back(curDcl);
    }
  }

  mainFunc->dumpToFile("after.stitched");
}


typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern int CISAparse(CISA_IR_Builder *builder);
extern YY_BUFFER_STATE CISA_scan_string(const char *yy_str);
extern void CISA_delete_buffer(YY_BUFFER_STATE buf);
static std::mutex mtx;
extern void resetGlobalVariables();

int CISA_IR_Builder::ParseVISAText(const std::string &visaText,
                                   const std::string &visaTextFile) {
  const std::lock_guard<std::mutex> lock(mtx);
  // Direct output of parser to null
#if defined(_WIN32)
  CISAout = fopen("nul", "w");
#else
  CISAout = fopen("/dev/null", "w");
#endif

  int status = VISA_SUCCESS;

  // Dump the visa text
  if (m_options.getOption(vISA_GenerateISAASM) && !visaTextFile.empty()) {
    std::ofstream ofs(visaTextFile.c_str(), std::ofstream::out);
    if (ofs.good()) {
      ofs << visaText;
      ofs.close();
    }
  }

  resetGlobalVariables();
  YY_BUFFER_STATE visaBuf = CISA_scan_string(visaText.c_str());
  if (CISAparse(this) != 0) {
#ifndef DLL_MODE
    std::cerr << "Parsing visa text failed.";
    if (!visaTextFile.empty()) {
      std::cerr << " Please examine " << visaTextFile << " and fix the error";
    }
    std::cerr << "\n" << criticalMsg.str();
#endif // DLL_MODE
    status = VISA_FAILURE;
  }
  CISA_delete_buffer(visaBuf);

  if (CISAout) {
    fclose(CISAout);
  }

  // run vISA verifier to cath any additional errors.
  // the subsequent vISABuilder::Compile() call is assumed to always succeed
  // after verifier checks.
  if (status == VISA_SUCCESS) {
    status = verifyVISAIR();
  }

  return status;
}

// Parses inline asm file from ShaderOverride
int CISA_IR_Builder::ParseVISAText(const std::string &visaFile) {
  // Direct output of parser to null
#if defined(_WIN32)
  CISAout = fopen("nul", "w");
#else
  CISAout = fopen("/dev/null", "w");
#endif
  CISAin = fopen(visaFile.c_str(), "r");
  if (!CISAin) {
    vISA_ASSERT(false, "Failed to open file");
    return VISA_FAILURE;
  }

  resetGlobalVariables();
  if (CISAparse(this) != 0) {
    vISA_ASSERT(false, "Parsing visa text failed");
    return VISA_FAILURE;
  }
  fclose(CISAin);

  if (CISAout) {
    fclose(CISAout);
  }
  return VISA_SUCCESS;

}

// default size of the kernel mem manager in bytes
int CISA_IR_Builder::Compile(const char *isaasmFileName, bool emit_visa_only) {
  // TIMER_BUILDER is started when builder is created
  stopTimer(TimerID::BUILDER);
  int status = VISA_SUCCESS;

  if (IS_VISA_BOTH_PATH) {
    if (m_builderMode == vISA_ASM_WRITER) {
      vISA_ASSERT(false, "Should not be calling Compile() in asm text writer mode!");
      return VISA_FAILURE;
    }

    int status = VISA_SUCCESS;
    for (auto func : m_kernelsAndFunctions) {
      bool attrWithoutError = func->finalizeAttributes();
      if (!attrWithoutError)
        return VISA_FAILURE;
    }

    if (m_options.getOption(vISA_GenerateISAASM) ||
        m_options.getOption(vISA_GenerateCombinedISAASM)) {
      status = isaDump(isaasmFileName);
      if (status != VISA_SUCCESS) {
        // Treat VISA_EARLY_EXIT as VISA_SUCCESS.
        return status == VISA_EARLY_EXIT ? VISA_SUCCESS : status;
      }
    }

    if (!m_options.getOption(vISA_NoVerifyvISA)) {
      status = verifyVISAIR();
      if (status != VISA_SUCCESS) {
        return status;
      }
    }
  }

  // Early return if emit_visa_only is true.
  if (emit_visa_only)
    return status;

  if (m_options.getuInt32Option(vISA_Linker) & (1U << Linker_Subroutine)) {
    std::map<std::string, G4_Kernel *> functionsNameMap;
    vISA_ASSERT(m_kernelsAndFunctions.front()->getIsKernel(),
        "the first function must be the kernel entry");
    std::unordered_map<G4_Kernel *, std::list<std::list<G4_INST *>::iterator>>
        callSites;
    std::list<std::list<G4_INST *>::iterator> sgInvokeList;
    CollectCallSites(m_kernelsAndFunctions, callSites, sgInvokeList);

    if (sgInvokeList.size()) {
      ResetHasStackCall(sgInvokeList, callSites);

      RemoveOptimizingFunction(sgInvokeList);

      std::unordered_map<G4_Kernel *,
                         std::list<std::list<vISA::G4_INST *>::iterator>>
          callee2Callers;
      ProcessSgInvokeList(sgInvokeList, callee2Callers);

      // Copy callees' context to callers and convert to subroutine calls
      LinkTimeOptimization(callee2Callers,
                           m_options.getuInt32Option(vISA_Linker));
    }
  }

  VISAKernelImpl *oldMainKernel = nullptr;
  if (IS_GEN_BOTH_PATH) {
    bool isInPatchingMode =
        m_options.getuInt32Option(vISA_CodePatch) >= CodePatch_Enable_NoLTO &&
        m_prevKernel;
    uint32_t localScheduleStartKernelId =
        m_options.getuInt32Option(vISA_LocalScheduleingStartKernel);
    uint32_t localScheduleEndKernelId =
        m_options.getuInt32Option(vISA_LocalScheduleingEndKernel);
    VISAKernelImpl *mainKernel = nullptr;
    KernelListTy::iterator iter = kernel_begin();
    KernelListTy::iterator iend = kernel_end();
    bool hasEarlyExit = false;
    for (int i = 0; iter != iend; iter++, i++) {
      VISAKernelImpl *kernel = (*iter);
      if ((uint32_t)i < localScheduleStartKernelId ||
          (uint32_t)i > localScheduleEndKernelId) {
        kernel->setLocalSheduleable(false);
      }

      mainKernel = (kernel->getIsKernel()) ? kernel : mainKernel;
      bool attrWithoutError = kernel->finalizeAttributes();
      if (!attrWithoutError)
        return VISA_FAILURE;
      kernel->getIRBuilder()->setType(kernel->getType());
      if (kernel->getIsKernel() == false) {
        if (kernel->getIRBuilder()->getArgSize() <
            kernel->getKernelFormat()->input_size) {
          kernel->getIRBuilder()->setArgSize(
              kernel->getKernelFormat()->input_size);
        }
        if (kernel->getIRBuilder()->getRetVarSize() <
            kernel->getKernelFormat()->return_value_size) {
          kernel->getIRBuilder()->setRetVarSize(
              kernel->getKernelFormat()->return_value_size);
        }
      }

      if (kernel->getIsPayload()) {
        vASSERT(mainKernel);
        // Copy main kernel's declarations (shader body) into payload section
        kernel->CopyVars(mainKernel);
        kernel->getKernel()->Declares = mainKernel->getKernel()->Declares;
        kernel->getIRBuilder()->setInputR1(
            mainKernel->getIRBuilder()->getInputR1());
        kernel->getIRBuilder()->setRealR0(
            mainKernel->getIRBuilder()->getRealR0());
        kernel->getIRBuilder()->setBuiltInR0(
            mainKernel->getIRBuilder()->getBuiltinR0());
        // Set payload LiveOuts to be output
        uint32_t inputCount = mainKernel->getIRBuilder()->getInputCount();
        for (unsigned int id = 0; id < inputCount; id++) {
          input_info_t *input_info =
              mainKernel->getIRBuilder()->getInputArg(id);
          // skip pseudo input for register bindings.
          if (input_info->isPseudoInput()) {
            continue;
          }
          vISA::G4_Declare *dcl = input_info->dcl;
          if (dcl->isPayloadLiveOut()) {
            dcl->getRootDeclare()->setLiveOut();
          }
        }
        mainKernel->getIRBuilder()->getRealR0()->setLiveOut();
      }

      if ((kernel->getIsKernel() && isInPatchingMode) ||
          (kernel->getvIsaInstCount() == 0 && kernel->getIsPayload())) {
        continue;
      }
      int status = kernel->compileFastPath();
      if (status != VISA_SUCCESS) {
        if (status == VISA_EARLY_EXIT) {
          // Consider stackcall or muti-kernel cases, need to continue
          // to compile next one until the last one.
          hasEarlyExit = true;
          continue;
        } else {
          stopTimer(TimerID::TOTAL);
          return status;
        }
      }

      if (kernel->getIsPayload()) {
        // Remove payload live-outs from the kernel after the compilation since
        // they will not be outputs anymore after stitching.
        mainKernel->getIRBuilder()->getRealR0()->resetLiveOut();
        const std::vector<input_info_t *> &inputs =
            mainKernel->getIRBuilder()->m_inputVect;
        for (const input_info_t *input_info : inputs) {
          vISA::G4_Declare *dcl = input_info->dcl;
          if (dcl->isPayloadLiveOut()) {
            dcl->resetLiveOut();
          }
        }
      }
    }

    if (hasEarlyExit) {
      // Consider early exit to still be a success as test run
      // lines may check exit status.
      status = VISA_SUCCESS;
      return status;
    }

    // Here we change the payload section as the main kernel in
    // m_kernelsAndFunctions During stitching, all functions will be cloned and
    // stitched to the main kernel. Demoting the shader body to a function type
    // makes it intact so we can stitch it again to another SIMD size of payload
    // section
    if (m_options.getuInt32Option(vISA_CodePatch)) {
      vASSERT(m_kernelsAndFunctions.front()->getIsKernel());
      for (auto func : m_kernelsAndFunctions) {
        if (func->getIsKernel()) {
          oldMainKernel = func;
          func->setType(VISA_BUILD_TYPE::FUNCTION);
        } else if (func->getIsPayload()) {
          func->setType(VISA_BUILD_TYPE::KERNEL);
        }
      }
      m_kernelsAndFunctions.erase(m_kernelsAndFunctions.begin());
      if (isInPatchingMode) {
        m_kernelsAndFunctions.push_back(m_prevKernel);
      } else {
        m_kernelsAndFunctions.push_back(oldMainKernel);
      }
      // payloadSection is the main kernel at this point
      // We need to copy essential info from the shader body to the main kernel
      auto payloadSection = m_kernelsAndFunctions.front()->getIRBuilder();
      auto shaderBody = m_kernelsAndFunctions.back()->getIRBuilder();
      *payloadSection->getJitInfo() = *shaderBody->getJitInfo();

      payloadSection->m_inputVect = shaderBody->m_inputVect;
      if (shaderBody->kernel.hasKernelDebugInfo()) {
        payloadSection->kernel.updateKernelDebugInfo(shaderBody->kernel);
      }

      if (shaderBody->kernel.hasGTPinInit()) {
        std::vector<unsigned> globalFreeRegs;
        unsigned int i = 0, j = 0;
        auto payloadSectionGTPin = payloadSection->kernel.getGTPinData();
        auto shaderBodyGTPin = shaderBody->kernel.getGTPinData();
        while (i < payloadSectionGTPin->getNumFreeGlobalRegs() &&
               j < shaderBodyGTPin->getNumFreeGlobalRegs()) {
          unsigned int iFreeGRF = payloadSectionGTPin->getFreeGlobalReg(i);
          unsigned int jFreeGRF = shaderBodyGTPin->getFreeGlobalReg(j);
          if (iFreeGRF < jFreeGRF) {
            i++;
          } else if (iFreeGRF > jFreeGRF) {
            j++;
          } else // iFreeGRF == jFreeGRF
          {
            globalFreeRegs.push_back(iFreeGRF);
            i++;
            j++;
          }
        }
        payloadSection->kernel.updateGTPinData(shaderBody->kernel);
        // If the number of free regs in payload section is 0,
        // it means the compilation is skipped and we don't have to do anything
        if (payloadSectionGTPin->getNumFreeGlobalRegs()) {
          payloadSection->kernel.getGTPinData()->setFreeGlobalRegs(
              globalFreeRegs);
        }
      }
    }

    // Preparing for stitching some functions to other functions
    // There are two stiching policies:
    // 1. vISA_noStitchExternFunc == false
    //    Stitch all non-kernel functions to all kernels
    // 2. vISA_noStitchExternFunc == true
    //    Stitch only non-external functions. Stitch them to all kernels and
    //    external functions

    // mainFunctions: functions or kernels those will be stitched by others
    // These functions/kernels will be the unit of compilePostOptimize
    KernelListTy mainFunctions;
    // subFunctions: functions those will stitch to others
    KernelListTy subFunctions;
    std::map<std::string, G4_Kernel *> subFunctionsNameMap;
    // For functions those will be stitch to others, create table to map their
    // name to G4_Kernel
    for (auto func : m_kernelsAndFunctions) {
      if (func->getIsKernel()) {
        // kernels must be stitched
        mainFunctions.push_back(func);
        continue;
      } else {

        if (!m_options.getOption(vISA_noStitchExternFunc) ||
            m_directCallFunctions.count(func->getName())) {
          // Policy 1: all functions will stitch to kernels
          subFunctions.push_back(func);
          subFunctionsNameMap[std::string(func->getName())] = func->getKernel();
        } else {
          // Policy 2: external functions will be stitched, non-external
          // functions will stitch to others
          if (func->getKernel()->getBoolKernelAttr(Attributes::ATTR_Extern)) {
            mainFunctions.push_back(func);
          } else {
            subFunctions.push_back(func);
            subFunctionsNameMap[std::string(func->getName())] =
                func->getKernel();
          }
        }
      }
    }

    // reset debug info offset of functionsToStitch
    for (auto func : subFunctions) {
      if (m_options.getOption(vISA_GenerateDebugInfo)) {
        func->getKernel()->getKernelDebugInfo()->resetRelocOffset();
        resetGenOffsets(*func->getKernel());
      }
    }

    // before stitching, propagate sub-functions' info into main functions
    // This function is better called before stitching to avoid redundant
    // search of BB for barrier counts after stitching.
    summarizeFunctionInfo(mainFunctions, subFunctions);

    bool hasPayloadPrologue =
        m_options.getuInt32Option(vISA_CodePatch) >= CodePatch_Payload_Prologue;

    // initialize new metadata object
    std::unique_ptr<MetadataDumpRA> metadata;
    const char* raFileName = nullptr;
    const char* kernelName = nullptr;
    bool dumpMetadata = m_options.getOption(vISA_dumpRAMetadata);
    bool decodeMetadata = m_options.getOption(vISA_DecodeRAMetadata);
    if (dumpMetadata) {
        metadata = std::make_unique<vISA::MetadataDumpRA>();
    }
    if (dumpMetadata || decodeMetadata) {
        m_options.getOption(VISA_AsmFileName, raFileName);
    }

    // stitch functions and compile to gen binary
    for (auto func : mainFunctions) {
      unsigned int genxBufferSize = 0;

      if (func->getKernel()->getBEFPSetupInst())
      {
        auto& builder = *func->getKernel()->fg.builder;
        auto& spillMemUsed = builder.getJitInfo()->stats.spillMemUsed;
        auto& scratchSpaceSizeLimit = builder.getJitInfo()->stats.scratchSpaceSizeLimit;
        // Skip padding when
        // 1. spill size is smaller than the threshold
        // 2. spill size is larger than scratchSpaceSizeLimit
        // (it cannot be encoded into the binary due to the limit)
        bool skip_padding =
          builder.kernel.getuInt32Option(vISA_SkipPaddingScratchSpaceSize) >= spillMemUsed ||
          (scratchSpaceSizeLimit != 0 &&
           spillMemUsed + (1U << builder.kernel.getuInt32Option(vISA_SSOShifter)) * 8 > scratchSpaceSizeLimit);
        bool SSO_padding =
          !skip_padding &&
          builder.getPlatform() == Xe_DG2 &&
          builder.kernel.getuInt32Option(vISA_SSOShifter) > 0;
        if (SSO_padding) {
          G4_Declare *delta = nullptr;
          G4_Declare *stackPtr = builder.kernel.fg.stackPtrDcl;

          // Temporarily use stackPtrDcl as a tmp register if padding is needed
          // Here we are working on physical registers after RA. stackPtrDcl is
          // guraantee no use at this point
          G4_Declare *tmp = stackPtr;
          BB_LIST &BBs = builder.kernel.fg.getBBList();
          // skip prolog sections (per_thread_prolog and cross_thread_prolog)
          G4_BB *entryBB = nullptr;
          for (BB_LIST_ITER it = BBs.begin(); it != BBs.end(); it++) {
            G4_BB *BB = *it;
            auto label = BB->getLabel()->getLabelName();
            if (strcmp(label, "per_thread_prolog") && strcmp(label, "cross_thread_prolog")) {
              entryBB = BB;
              break;
            }
          }
          vASSERT(entryBB);
          auto fpInst = func->getKernel()->getBEFPSetupInst();
          vISA_ASSERT(fpInst->opcode() == G4_mov, "unexpected pattern");
          auto spInst = func->getKernel()->getBESPSetupInst();
          vISA_ASSERT(spInst->opcode() == G4_mov, "unexpected pattern");
          auto insertIt = std::find_if(entryBB->begin(), entryBB->end(),
                                     [&](G4_INST *inst) { return inst == fpInst; });
          // If we cannot find fpInst in the entryBB, it is either
          // 1. getBEFPSetupInst() does not get correct place we initialize the FP
          // 2. entryBB is not where we expect to contain fpInst
          vISA_ASSERT(*insertIt == fpInst, "Cannot find fp setup inst");
          // hwtid = sr0 & 0x7
          auto sr0 = builder.createSrc(builder.phyregpool.getSr0Reg(), 0, 0,
              builder.getRegionScalar(), Type_UD);
          auto mask = builder.createImm(0x7, Type_UD);
          auto hwtid = builder.createDst(tmp->getRegVar(), 0, 0, 1, Type_UD);

          auto andInst = builder.createBinOp(G4_and, g4::SIMD1, hwtid, sr0, mask,
              InstOpt_WriteEnable, false);
          andInst->setVISAId(UNMAPPABLE_VISA_INDEX);
          andInst->addComment("hwtid = sr0 & 0x7");
          entryBB->insertBefore(insertIt, andInst);

          // delta = hwtid << vISA_SSOShifter
          auto tidSrc = builder.createSrc(hwtid->getBase(), 0, 0, builder.getRegionScalar(), Type_UD);
          auto imm = builder.createImm(builder.kernel.getuInt32Option(vISA_SSOShifter), Type_UD);
          delta = tmp;
          auto shlInst = builder.createBinOp(G4_shl, g4::SIMD1,
              builder.createDst(tmp->getRegVar(), 0, 0, 1, Type_UD),
              tidSrc, imm,
              InstOpt_WriteEnable, false);
          shlInst->setVISAId(UNMAPPABLE_VISA_INDEX);
          shlInst->addComment("delta = hwtid << vISA_SSOShifter");
          shlInst->setDistance(1);
          shlInst->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);
          entryBB->insertBefore(insertIt, shlInst);

          // add delta to the initialization of SP and FP
          fpInst->setOpcode(G4_add);
          fpInst->setSrc(builder.createImm(fpInst->getSrc(0)->asImm()->getImm(), Type_UD), 1);
          fpInst->setSrc(builder.createSrcRegRegion(delta, builder.getRegionScalar()), 0);
          fpInst->getDst()->setType(builder, Type_UD);

          spInst->setOpcode(G4_add);
          spInst->setSrc(builder.createImm(spInst->getSrc(0)->asImm()->getImm(), Type_UD), 1);
          spInst->setSrc(builder.createSrcRegRegion(delta, builder.getRegionScalar()), 0);
          spInst->getDst()->setType(builder, Type_UD);

          // Token is inherited by fpInst
          andInst->inheritSWSBFrom(fpInst);
          fpInst->setDistance(1);
          fpInst->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);
          // Remove token
          fpInst->setTokenType(G4_INST::TOKEN_NONE);

          // preserve additional size to prevent out of bound access
          spillMemUsed += (1U << builder.kernel.getuInt32Option(vISA_SSOShifter)) * 7;
          spillMemUsed = ROUND(spillMemUsed, builder.kernel.numEltPerGRF<Type_UB>());
        }
      }

      // store the BBs with FCall and FRet, which must terminate the BB
      std::map<G4_BB *, G4_INST *> origFCallFRet;
      if (!hasPayloadPrologue) {
        Stitch_Compiled_Units(func->getKernel(), subFunctionsNameMap,
                              origFCallFRet);
      }

      func->compilePostOptimize();
      if (hasPayloadPrologue) {
        if (!isInPatchingMode) {
          for (auto shaderBody : subFunctions) {
            shaderBody->getKernel()->fg.reassignBlockIDs();
            shaderBody->getKernel()->fg.setPhysicalPredSucc();
            shaderBody->compilePostOptimize();
          }
        }
        // Append shader body to payload section
        for (auto &&iter : subFunctionsNameMap) {
          G4_Kernel *callee = iter.second;
          func->getKernel()->fg.append(callee->fg);
        }
      }

      void *genxBuffer = func->encodeAndEmit(genxBufferSize);

      // create metadata for this kernel if enabled
      if (dumpMetadata) {
          metadata->addKernelMD(func->getKernel());
      }
      if (dumpMetadata || decodeMetadata) {
          kernelName = func->getKernel()->getName();
      }

      func->setGenxBinaryBuffer(genxBuffer, genxBufferSize);
      if (m_options.getOption(vISA_GenerateDebugInfo)) {
        func->computeAndEmitDebugInfo(subFunctions);
      }
      restoreFCallState(func->getKernel(), origFCallFRet);

    }

    if (dumpMetadata) {
        // emit the metadata file
        metadata->emitMetadataFile(raFileName, kernelName);
    }
    // output metadata to console

    if (decodeMetadata) {
        stringstream ssInit;
        ssInit << raFileName << "_" << kernelName << ".ra_metadata";

        MetadataReader MDReader;
        MDReader.readMetadata(ssInit.str());
    }

  }

  stopTimer(TimerID::TOTAL); // have to record total time before dump the timer
  if (m_options.getOption(vISA_dumpTimer)) {
    const char *asmName = nullptr;
    m_options.getOption(VISA_AsmFileName, asmName);
    dumpAllTimers(asmName, true);
  }

#ifndef DLL_MODE
  if (criticalMsg.str().length() > 0) {
    std::cerr << "[vISA Finalizer Messages]\n" << criticalMsg.str();
    status = VISA_FAILURE;
  }
#endif // DLL_MODE

  if (m_options.getuInt32Option(vISA_CodePatch) && oldMainKernel) {
    m_kernelsAndFunctions.pop_back();
    m_kernelsAndFunctions.push_back(oldMainKernel);
  }

  return status;
}

void CISA_IR_Builder::summarizeFunctionInfo(
    KernelListTy &mainFunctions,
    KernelListTy &subFunction) {

  // Set usesBarrier property for each kernel and function appropriately.
  // resursively propagate barrier information from callees to their caller.
  // This helps to report the correct barrier setting for direct/internal call.
  // For indirect call, the information is reported separately per function
  // and relying on runtime to analyze it.
  {
    std::set<VISAKernelImpl *> visited;
    for (VISAKernelImpl *f : m_kernelsAndFunctions)
      retrieveBarrierInfoFromCallee(f, visited);
  }

  // helper functions for visa stack size setting
  auto getStackSize = [](const VISAKernelImpl &k) {
    return k.getKernel()->fg.builder->getJitInfo()->stats.spillMemUsed;
  };
  auto setStackSizeAndClamp = [](const VISAKernelImpl &k, uint32_t size) {
    auto maxPTSS = k.getKernel()->fg.builder->getMaxPTSS();
    size = size > maxPTSS ? maxPTSS : size;
    k.getKernel()->fg.builder->getJitInfo()->stats.spillMemUsed = size;
  };

  for (auto mfunc : mainFunctions) {
    uint32_t totalStackSize = getStackSize(*mfunc);
    for (auto sfunc: subFunction) {
      // propagate subFunctions' perf stats into mainFunctions'
      mfunc->addFuncPerfStats(
          sfunc->getKernel()->fg.builder->getJitInfo()->statsVerbose);

      // Accumulate all subFunctions' spill size and set it to the main
      // function. vISA doesn't have the call graph so conservatively
      // estimate the required size.
      totalStackSize += getStackSize(*sfunc);
    }

    // The estimated size might be larger than what is actually needed.
    // Clamp to max scratch size if the estimated size exceeds it.
    setStackSizeAndClamp(*mfunc, totalStackSize);
  }
}

int CISA_IR_Builder::verifyVISAIR() {

#ifdef IS_RELEASE_DLL
  return VISA_SUCCESS;
#endif

  bool hasErrors = false;
  unsigned totalErrors = 0;
  std::string
      testName; // base kernel name saved for function's isaasm file name

  for (auto kTemp : m_kernelsAndFunctions) {
    if (kTemp->getIsKernel()) {
      // if asmName is test9_genx_0.asm, the testName is test9_genx.
      std::string asmName = kTemp->getOutputAsmPath();
      std::string::size_type asmNameEnd = asmName.find_last_of('_');
      if (asmNameEnd != std::string::npos) {
        testName = asmName.substr(0, asmNameEnd);
      } else {
        testName = std::move(asmName);
      }
      break;
    }
  }

  std::vector<std::string> failedFiles;
  VISAKernelImpl *mainKernel = nullptr;
  for (auto kTemp : m_kernelsAndFunctions) {
    unsigned funcId = 0;
    if (kTemp->getIsKernel()) {
      mainKernel = kTemp;
    }
    // Payload section is a mirror compilation to the main kernel
    // Load the main kernel to access its symbol table
    VISAKernelImpl *fmtKernel = kTemp->getIsPayload() ? mainKernel : kTemp;
    vASSERT(fmtKernel);

    VISAKernel_format_provider fmt(fmtKernel);

    vISAVerifier verifier(&fmt, getOptions(), fmtKernel->getIRBuilder());
    verifier.run(kTemp);

    if (verifier.hasErrors()) {
      std::stringstream verifierName;

      if (kTemp->getIsKernel()) {
        verifierName << kTemp->getOutputAsmPath();
      } else {
        kTemp->GetFunctionId(funcId);
        verifierName << testName;
        verifierName << "_f";
        verifierName << funcId;
      }
      verifierName << ".errors.txt";
      verifier.writeReport(verifierName.str().c_str());
      failedFiles.push_back(verifierName.str());
      hasErrors = true;
      totalErrors += (uint32_t)verifier.getNumErrors();
    }
  }
  if (hasErrors) {
    std::stringstream ss;
    ss << "Found a total of " << totalErrors << " errors in vISA input.\n";
    ss << "Please check\n";
    for (auto &&name : failedFiles) {
      ss << "\t" << name << "\n";
    }
    ss << "for the exact error messages\n";
    criticalMsgStream() << ss.str();
    return VISA_FAILURE;
  }

  return VISA_SUCCESS;
}

bool CISA_IR_Builder::CISA_lookup_builtin_constant(int lineNum,
                                                   const char *symbol,
                                                   int64_t &val) {
  std::string sym(symbol);
  if (sym == "%DispatchSimd") {
    if (m_dispatchSimdSize <= 0) {
      m_dispatchSimdSize = -1;
      RecordParseError(
          lineNum,
          "symbol cannot be used before .kernel_attr DispatchSimd=... is set");
      return false;
    }
    val = m_dispatchSimdSize;
    return true;
  } else {
    RecordParseError(lineNum, std::move(sym), ": invalid built-in symbol");
    val = -1;
    return false;
  }
}

bool CISA_IR_Builder::CISA_eval_sizeof_decl(int lineNum, const char *var,
                                            int64_t &val) {
  auto *decl = (VISA_GenVar *)m_kernel->getDeclFromName(var);
  if (!decl) {
    if (std::string(var) == "GRF") {
      val = m_kernel->getIRBuilder()->getGRFSize();
      return true;
    }
    RecordParseError(lineNum, var, ": unbound variable");
    return false;
  }
  switch (decl->type) {
  case GENERAL_VAR:
    val = (int64_t)decl->genVar.getSize();
    break;
  case ADDRESS_VAR:
    val = (int64_t)decl->addrVar.num_elements * 2;
    break;
  default:
    RecordParseError(lineNum, var,
                     ": unsupported operator on this variable kind");
    return false;
  }
  return true;
}

// Use in a function returning bool (returns false on failure)
// requires: int lineNum
//
// TODO: the long term goal is to have the vISA builder class store a
// "last error" of some sort and then we can just change this macro.
//
// Note: this is exactly what C++ exceptions are for.  This ugliness is the
// cost.
#define VISA_CALL_TO_BOOL(FUNC, ...)                                           \
  do {                                                                         \
    int __status = m_kernel->FUNC(__VA_ARGS__);                                \
    if (__status != VISA_SUCCESS) {                                            \
      RecordParseError(lineNum, #FUNC,                                         \
                       ": unknown error (internal line: ", __LINE__, ")");     \
      return false;                                                            \
    }                                                                          \
  } while (0)
#define VISA_RESULT_CALL_TO_BOOL(FUNC_RESULT)                                  \
  do {                                                                         \
    int __status = FUNC_RESULT;                                                \
    if (__status != VISA_SUCCESS) {                                            \
      RecordParseError(lineNum, "" /*__FUNCTION__*/,                           \
                       ": unknown error (internal line: ", __LINE__, ")");     \
      return false;                                                            \
    }                                                                          \
  } while (0)
// similar to above, but returns nullptr on failure.
#define VISA_CALL_TO_NULLPTR(FUNC, ...)                                        \
  do {                                                                         \
    int __status = m_kernel->FUNC(__VA_ARGS__);                                \
    if (__status != VISA_SUCCESS) {                                            \
      RecordParseError(lineNum, #FUNC,                                         \
                       ": unknown error (internal line: ", __LINE__, ")");     \
      return nullptr;                                                          \
    }                                                                          \
  } while (0)

#define VISA_CALL_TO_BOOL_NOLINE(FUNC, ...)                                    \
  do {                                                                         \
    int lineNum = 0;                                                           \
    VISA_CALL_TO_BOOL(FUNC, __VA_ARGS__);                                      \
  } while (0)

VISA_StateOpndHandle *
CISA_IR_Builder::CISA_get_surface_variable(const char *varName, int lineNum) {
  VISA_StateOpndHandle *surface = nullptr;
  VISA_SurfaceVar *surfaceVar =
      (VISA_SurfaceVar *)m_kernel->getDeclFromName(varName);
  if (!surfaceVar) {
    RecordParseError(lineNum, varName, ": undefined surface variable");
  } else if (surfaceVar->type != SURFACE_VAR &&
             surfaceVar->type != SAMPLER_VAR) {
    RecordParseError(lineNum, varName, ": not a surface variable");
  } else {
    if (m_kernel->CreateVISAStateOperandHandle(surface, surfaceVar) !=
        VISA_SUCCESS) {
      RecordParseError(lineNum, varName,
                       ": internal error: creating surface variable");
      surface = nullptr;
    }
  }
  return surface;
}

VISA_StateOpndHandle *
CISA_IR_Builder::CISA_get_sampler_variable(const char *varName, int lineNum) {
  VISA_StateOpndHandle *surface = nullptr;
  VISA_SamplerVar *samplerVar =
      (VISA_SamplerVar *)m_kernel->getDeclFromName(varName);
  if (!samplerVar) {
    RecordParseError(lineNum, varName, ": undefined sampler variable");
  } else if (samplerVar->type != SURFACE_VAR &&
             samplerVar->type != SAMPLER_VAR) {
    RecordParseError(lineNum, varName, ": not a sampler variable");
  } else {
    if (m_kernel->CreateVISAStateOperandHandle(surface, samplerVar) !=
        VISA_SUCCESS) {
      RecordParseError(lineNum, varName,
                       ": internal error: creating sampler variable");
      surface = nullptr;
    }
  }
  return surface;
}

bool CISA_IR_Builder::CISA_general_variable_decl(
    const char *var_name, unsigned int var_elemts_num, VISA_Type data_type,
    VISA_Align var_align, const char *var_alias_name, int var_alias_offset,
    std::vector<attr_gen_struct *> &scope, int lineNum) {
  VISA_GenVar *genVar = NULL;

  VISA_GenVar *parentDecl = NULL;

  if (m_kernel->declExistsInCurrentScope(var_name)) {
    RecordParseError(lineNum, var_name, ": variable redeclaration");
    return false;
  }

  if (var_alias_name && strcmp(var_alias_name, "") != 0) {
    parentDecl = (VISA_GenVar *)m_kernel->getDeclFromName(var_alias_name);
    if (parentDecl == nullptr) {
      // alias=... points to a nonexistent variable
      RecordParseError(lineNum, var_alias_name, ": unbound alias referent");
      return false;
    }
  }

  m_kernel->CreateVISAGenVar(genVar, var_name, var_elemts_num, data_type,
                             var_align, parentDecl, var_alias_offset);

  if (!addAllVarAttributes((CISA_GEN_VAR *)genVar, scope, lineNum)) {
    return false;
  }
  return true;
}

bool CISA_IR_Builder::CISA_addr_variable_decl(
    const char *var_name, unsigned int var_elements, VISA_Type data_type,
    std::vector<attr_gen_struct *> &scope, int lineNum) {
  if (m_kernel->declExistsInCurrentScope(var_name)) {
    RecordParseError(lineNum, var_name, ": variable redeclaration");
    return false;
  }

  VISA_AddrVar *decl = NULL;
  m_kernel->CreateVISAAddrVar(decl, var_name, var_elements);
  if (!addAllVarAttributes((CISA_GEN_VAR *)decl, scope, lineNum)) {
    return false;
  }
  return true;
}

bool CISA_IR_Builder::CISA_predicate_variable_decl(
    const char *var_name, unsigned int var_elements,
    std::vector<attr_gen_struct *> &attrs, int lineNum) {
  if (m_kernel->declExistsInCurrentScope(var_name)) {
    RecordParseError(lineNum, var_name, ": variable redeclaration");
    return false;
  }

  VISA_PredVar *decl = NULL;
  m_kernel->CreateVISAPredVar(decl, var_name, (unsigned short)var_elements);
  if (!addAllVarAttributes((CISA_GEN_VAR *)decl, attrs, lineNum)) {
    return false;
  }
  return true;
}

bool CISA_IR_Builder::CISA_sampler_variable_decl(const char *var_name,
                                                 int num_elts, const char *name,
                                                 int lineNum) {
  if (m_kernel->declExistsInCurrentScope(var_name)) {
    RecordParseError(lineNum, var_name, ": variable redeclaration");
    return false;
  }

  VISA_SamplerVar *decl = NULL;
  m_kernel->CreateVISASamplerVar(decl, var_name, num_elts);
  return true;
}

bool CISA_IR_Builder::CISA_surface_variable_decl(
    const char *var_name, int num_elts, const char *name,
    std::vector<attr_gen_struct *> &attrs, int lineNum) {
  if (m_kernel->declExistsInCurrentScope(var_name)) {
    RecordParseError(lineNum, var_name, ": variable redeclaration");
    return false;
  }

  VISA_SurfaceVar *decl = NULL;
  m_kernel->CreateVISASurfaceVar(decl, var_name, num_elts);
  if (!addAllVarAttributes((CISA_GEN_VAR *)decl, attrs, lineNum)) {
    return false;
  }
  return true;
}

bool CISA_IR_Builder::CISA_implicit_input_directive(const char *argName,
                                                    const char *varName,
                                                    short offset,
                                                    unsigned short size,
                                                    int lineNum) {
  std::string implicitArgName = argName;
  std::string undefStr("UNDEFINED_");
  auto pos = implicitArgName.find(undefStr);
  uint32_t numVal = 0;
  if (pos != std::string::npos) {
    pos += undefStr.length();
    auto numValString = implicitArgName.substr(pos, implicitArgName.length());
    numVal = std::stoi(numValString);
  } else {
    std::string implicitStr(".implicit_");
    auto implicitInputName =
        implicitArgName.substr(implicitStr.length(), implicitArgName.length());
    for (; numVal < IMPLICIT_INPUT_COUNT; ++numVal) {
      if (!implicitInputName.compare(implictKindStrings[numVal])) {
        numVal = implictKindValues[numVal];
        break;
      }
    }
  }

  int status = VISA_SUCCESS;
  CISA_GEN_VAR *temp = m_kernel->getDeclFromName(varName);
  if (!temp) {
    RecordParseError(lineNum, varName, ": undefined variable");
    return false;
  }
  status = m_kernel->CreateVISAImplicitInputVar(
      (VISA_GenVar *)temp, offset, size, numVal);
  if (status != VISA_SUCCESS) {
    RecordParseError(lineNum, "failed to create input variable");
    return false;
  }
  return true;
}

bool CISA_IR_Builder::CISA_input_directive(const char *var_name, short offset,
                                           unsigned short size, int lineNum) {
  int status = VISA_SUCCESS;
  CISA_GEN_VAR *var = m_kernel->getDeclFromName(var_name);
  if (var == nullptr) {
    RecordParseError(lineNum, var_name, ": unbound identifier");
    return false;
  }

  status = m_kernel->CreateVISAInputVar((VISA_GenVar *)var, offset, size);
  if (status != VISA_SUCCESS) {
    RecordParseError(lineNum, var_name,
                     ": internal error: failed to create input variable");
    return false;
  }
  return true;
}

bool CISA_IR_Builder::CISA_attr_directive(const char *input_name,
                                          const char *input_var, int lineNum) {
  Attributes::ID attrID = Attributes::getAttributeID(input_name);
  if (!m_options.getOption(vISA_AsmFileNameOverridden) &&
      attrID == Attributes::ATTR_OutputAsmPath) {
    if (strcmp(input_name, "AsmName") == 0) {
      RecordParseWarning(lineNum,
                         "AsmName deprecated (replace with OutputAsmPath)");
    }
    input_name = "OutputAsmPath"; // normalize to new name

    // OutputAsmPath attribute value is copied to an arena allocated buffer.
    // So we don't need to explicitly deallocate the buffer.
    const unsigned int maxStrSize = 1024;
    unsigned int strLength = strnlen_s(input_var, maxStrSize);
    unsigned int allocSize = strLength + 1;
    char *asmFileName = (char *)(m_mem.alloc(allocSize));
    strncpy_s(asmFileName, allocSize, input_var, strLength);
    char *pos = strstr(asmFileName, ".asm");
    if (pos != NULL) {
      *pos = '\0';
    }
    m_options.setOptionInternally(VISA_AsmFileName, asmFileName);
  }

  if (attrID == Attributes::ATTR_Target) {
    unsigned char visa_target;
    if (input_var == nullptr) {
      RecordParseError(lineNum,
                       ".kernel_attr Target=.. must be \"cm\" or \"3d\"");
      return false;
    }
    if (strcmp(input_var, "cm") == 0) {
      visa_target = VISA_CM;
    } else if (strcmp(input_var, "3d") == 0) {
      visa_target = VISA_3D;
    } else {
      RecordParseError(lineNum, "invalid kernel target attribute");
      return false;
    }
    m_kernel->AddKernelAttribute(input_name, sizeof(visa_target), &visa_target);
  } else {
    m_kernel->AddKernelAttribute(
        input_name,
        input_var == nullptr ? 0 : (int)std::string_view(input_var).size(),
        input_var);
  }

  return true;
}

bool CISA_IR_Builder::CISA_attr_directiveNum(const char *input_name,
                                             uint32_t input_var, int lineNum) {
  if (std::string(input_name) == "SimdSize" ||
      std::string(input_name) == "DispatchSimdSize") {
    m_dispatchSimdSize = (int)input_var;
  }
  VISA_CALL_TO_BOOL(AddKernelAttribute, input_name, sizeof(uint32_t),
                    &input_var);
  return true;
}

bool CISA_IR_Builder::CISA_create_label(const char *label_name, int lineNum) {
  VISA_LabelOpnd *opnd[1] = {nullptr};

  // when we print out ./function from isa we also print out label.
  // if we don't skip it during re-parsing then we will have duplicate labels
  if (!m_kernel->getLabelOperandFromFunctionName(std::string(label_name))) {
    opnd[0] = m_kernel->getLabelOpndFromLabelName(std::string(label_name));
    if (!opnd[0]) {

      VISA_Label_Kind kind = LABEL_BLOCK;
      if (std::string(label_name).find("__opt_resource_loo") != std::string::npos) {
        kind = LABEL_DIVERGENT_RESOURCE_LOOP;
      }
      // forward jump
      VISA_CALL_TO_BOOL(CreateVISALabelVar, opnd[0], label_name, kind);
      if (!m_kernel->setLabelOpndNameMap(label_name, opnd[0], kind))
        return false;
    }
    VISA_CALL_TO_BOOL(AppendVISACFLabelInst, opnd[0]);
  }

  return true;
}

bool CISA_IR_Builder::CISA_function_directive(const char *func_name,
                                              int lineNum) {
  VISA_LabelOpnd *opnd[1] = {nullptr};
  opnd[0] = m_kernel->getLabelOperandFromFunctionName(std::string(func_name));
  if (!opnd[0]) {
    VISA_CALL_TO_BOOL(CreateVISALabelVar, opnd[0], func_name, LABEL_SUBROUTINE);
    if (!m_kernel->setLabelOpndNameMap(func_name, opnd[0], LABEL_SUBROUTINE))
      return false;
  }

  VISA_CALL_TO_BOOL(AppendVISACFLabelInst, opnd[0]);
  return true;
}

bool CISA_IR_Builder::CISA_create_arith_instruction(
    VISA_opnd *pred, ISA_Opcode opcode, bool sat, VISA_EMask_Ctrl emask,
    unsigned exec_size, VISA_opnd *dst_cisa, VISA_opnd *src0_cisa,
    VISA_opnd *src1_cisa, VISA_opnd *src2_cisa, int lineNum) {
  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  VISA_CALL_TO_BOOL(AppendVISAArithmeticInst, opcode, (VISA_PredOpnd *)pred,
                    sat, emask, executionSize, (VISA_VectorOpnd *)dst_cisa,
                    (VISA_VectorOpnd *)src0_cisa, (VISA_VectorOpnd *)src1_cisa,
                    (VISA_VectorOpnd *)src2_cisa);
  return true;
}

bool CISA_IR_Builder::CISA_create_arith_instruction2(
    VISA_opnd *pred, ISA_Opcode opcode, VISA_EMask_Ctrl emask,
    unsigned exec_size, VISA_opnd *dst_cisa, VISA_opnd *carry_borrow,
    VISA_opnd *src1_cisa, VISA_opnd *src2_cisa, int lineNum) {
  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  VISA_CALL_TO_BOOL(AppendVISATwoDstArithmeticInst, opcode,
                    (VISA_PredOpnd *)pred, emask, executionSize,
                    (VISA_VectorOpnd *)dst_cisa,
                    (VISA_VectorOpnd *)carry_borrow,
                    (VISA_VectorOpnd *)src1_cisa, (VISA_VectorOpnd *)src2_cisa);
  return true;
}

// Two destination instruction:
//    (p) op  (dst pred_dst)  src0   src1
bool CISA_IR_Builder::CISA_create_arith_instruction2_predDst(
    VISA_opnd *pred, ISA_Opcode opcode, VISA_EMask_Ctrl emask,
    unsigned exec_size, VISA_opnd *dst, CISA_GEN_VAR *pred_dst, VISA_opnd *src0,
    VISA_opnd *src1, int lineNum) {
  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  VISA_CALL_TO_BOOL(AppendVISAPredDstArithmeticInst, opcode,
                    (VISA_PredOpnd *)pred, emask, executionSize,
                    (VISA_VectorOpnd *)dst, (VISA_PredVar *)pred_dst,
                    (VISA_VectorOpnd *)src0, (VISA_VectorOpnd *)src1);
  return true;
}

bool CISA_IR_Builder::CISA_create_breakpoint_instruction(int lineNum) {
  VISA_CALL_TO_BOOL(AppendVISABreakpointInst);
  return true;
}

bool CISA_IR_Builder::CISA_create_mov_instruction(
    VISA_opnd *pred, ISA_Opcode opcode, VISA_EMask_Ctrl emask,
    unsigned exec_size, bool sat, VISA_opnd *dst, VISA_opnd *src0,
    int lineNum) {
  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  VISA_CALL_TO_BOOL(AppendVISADataMovementInst, opcode, (VISA_PredOpnd *)pred,
                    sat, emask, executionSize, (VISA_VectorOpnd *)dst,
                    (VISA_VectorOpnd *)src0);
  return true;
}

bool CISA_IR_Builder::CISA_create_mov_instruction(VISA_opnd *dst,
                                                  CISA_GEN_VAR *src0,
                                                  int lineNum) {
  vISA_ASSERT_INPUT(src0 != NULL,
                "The source operand of a move instruction was null, lineNum %d", lineNum);
  VISA_CALL_TO_BOOL(AppendVISAPredicateMove, (VISA_VectorOpnd *)dst,
                    (VISA_PredVar *)src0);
  return true;
}

bool CISA_IR_Builder::CISA_create_movs_instruction(
    VISA_EMask_Ctrl emask, ISA_Opcode opcode, unsigned exec_size,
    VISA_opnd *dst, VISA_opnd *src0, int lineNum) {
  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  VISA_CALL_TO_BOOL(AppendVISADataMovementInst, ISA_MOVS, NULL, false, emask,
                    executionSize, (VISA_VectorOpnd *)dst,
                    (VISA_VectorOpnd *)src0);
  return true;
}

bool CISA_IR_Builder::CISA_create_branch_instruction(
    VISA_opnd *pred, ISA_Opcode opcode, VISA_EMask_Ctrl emask,
    unsigned exec_size, const char *target_label, bool is_fccall, int lineNum) {
  VISA_LabelOpnd *opnd[1];
  int i = 0;

  switch (opcode) {
  case ISA_CALL: {
    // need second path over instruction stream to
    // determine correct IDs since function directive might not have been
    // encountered yet
    opnd[i] =
        m_kernel->getLabelOperandFromFunctionName(std::string(target_label));

    VISA_Label_Kind lblKind = is_fccall ? LABEL_FC : LABEL_SUBROUTINE;
    if (!opnd[i]) {
      VISA_CALL_TO_BOOL(CreateVISALabelVar, opnd[i], target_label, lblKind);
      if (!m_kernel->setLabelOpndNameMap(target_label, opnd[0], lblKind))
        return false;
      opnd[i]->tag = ISA_SUBROUTINE;
    }
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISACFCallInst, (VISA_PredOpnd *)pred, emask,
                      executionSize, opnd[i]);
    return true;
  }
  case ISA_JMP: {
    opnd[i] = m_kernel->getLabelOpndFromLabelName(std::string(target_label));

    // forward jump label: create the label optimistically
    if (!opnd[i]) {
      VISA_CALL_TO_BOOL(CreateVISALabelVar, opnd[i], target_label, LABEL_BLOCK);
      if (!m_kernel->setLabelOpndNameMap(target_label, opnd[0], LABEL_BLOCK))
        return false;
    }

    VISA_CALL_TO_BOOL(AppendVISACFJmpInst, (VISA_PredOpnd *)pred, opnd[i]);
    return true;
  }
  case ISA_GOTO: {
    opnd[i] = m_kernel->getLabelOpndFromLabelName(std::string(target_label));

    // forward jump label: create the label optimistically
    if (!opnd[i]) {
      VISA_CALL_TO_BOOL(CreateVISALabelVar, opnd[i], target_label, LABEL_BLOCK);
      if (!m_kernel->setLabelOpndNameMap(target_label, opnd[0], LABEL_BLOCK))
        return false;
    }
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISACFGotoInst, (VISA_PredOpnd *)pred, emask,
                      executionSize, opnd[i]);
    return true;
  }
  default: {
    vISA_ASSERT_UNREACHABLE("UNKNOWN Branch OP not supported.");
    return false;
  }
  }

  return true;
}

bool CISA_IR_Builder::CISA_create_cmp_instruction(
    VISA_Cond_Mod sub_op, VISA_EMask_Ctrl emask, unsigned exec_size,
    CISA_GEN_VAR *decl, VISA_opnd *src0, VISA_opnd *src1, int lineNum) {
  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  VISA_CALL_TO_BOOL(AppendVISAComparisonInst, sub_op, emask, executionSize,
                    (VISA_PredVar *)decl, (VISA_VectorOpnd *)src0,
                    (VISA_VectorOpnd *)src1);
  return true;
}

bool CISA_IR_Builder::CISA_create_cmp_instruction(
    VISA_Cond_Mod sub_op, ISA_Opcode opcode, VISA_EMask_Ctrl emask,
    unsigned exec_size, VISA_opnd *dst, VISA_opnd *src0, VISA_opnd *src1,
    int lineNum) {
  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  VISA_CALL_TO_BOOL(AppendVISAComparisonInst, sub_op, emask, executionSize,
                    (VISA_VectorOpnd *)dst, (VISA_VectorOpnd *)src0,
                    (VISA_VectorOpnd *)src1);
  return true;
}

bool CISA_IR_Builder::CISA_create_media_instruction(
    ISA_Opcode opcode, MEDIA_LD_mod media_mod, int block_width,
    int block_height, unsigned int plane_ID, const char *surfaceName,
    VISA_opnd *xOffset, VISA_opnd *yOffset, VISA_opnd *raw_dst, int lineNum) {
  unsigned char mod;
  mod = media_mod & 0x7;
  if (mod >= MEDIA_LD_Mod_NUM) {
    RecordParseError(lineNum, "ISA_MEDIA_LD uses illegal exec size");
    return false;
  }

  VISA_StateOpndHandle *surface =
      CISA_get_surface_variable(surfaceName, lineNum);
  if (surface == nullptr)
    return false; // error already reported

  VISA_CALL_TO_BOOL(AppendVISASurfAccessMediaLoadStoreInst, opcode, media_mod,
                    surface, (unsigned char)block_width,
                    (unsigned char)block_height, (VISA_VectorOpnd *)xOffset,
                    (VISA_VectorOpnd *)yOffset, (VISA_RawOpnd *)raw_dst,
                    (CISA_PLANE_ID)plane_ID);

  return true;
}

/*
For both RET and FRET instructions
*/
bool CISA_IR_Builder::CISA_Create_Ret(VISA_opnd *pred_opnd, ISA_Opcode opcode,
                                      VISA_EMask_Ctrl emask,
                                      unsigned int exec_size, int lineNum) {
  if (opcode == ISA_RET) {
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISACFRetInst, (VISA_PredOpnd *)pred_opnd, emask,
                      executionSize);
  } else {
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISACFFunctionRetInst, (VISA_PredOpnd *)pred_opnd,
                      emask, executionSize);
  }

  return true;
}

bool CISA_IR_Builder::CISA_create_oword_instruction(
    ISA_Opcode opcode, bool media_mod, unsigned int size,
    const char *surfaceName, VISA_opnd *offset_opnd, VISA_opnd *raw_dst_src,
    int lineNum) {
  VISA_StateOpndHandle *surface =
      CISA_get_surface_variable(surfaceName, lineNum);
  if (!surface)
    return false; // error recorded

  VISA_CALL_TO_BOOL(
      AppendVISASurfAccessOwordLoadStoreInst, opcode, vISA_EMASK_M1, surface,
      Get_VISA_Oword_Num_From_Number(size), (VISA_VectorOpnd *)offset_opnd,
      (VISA_RawOpnd *)raw_dst_src);
  return true;
}

bool CISA_IR_Builder::CISA_create_svm_block_instruction(
    SVMSubOpcode subopcode, unsigned owords, bool unaligned, VISA_opnd *address,
    VISA_opnd *srcDst, int lineNum) {
  switch (subopcode) {
  case SVM_BLOCK_LD:
    VISA_CALL_TO_BOOL(AppendVISASvmBlockLoadInst,
                      Get_VISA_Oword_Num_From_Number(owords), unaligned,
                      (VISA_VectorOpnd *)address, (VISA_RawOpnd *)srcDst);
    return true;
  case SVM_BLOCK_ST:
    VISA_CALL_TO_BOOL(AppendVISASvmBlockStoreInst,
                      Get_VISA_Oword_Num_From_Number(owords), unaligned,
                      (VISA_VectorOpnd *)address, (VISA_RawOpnd *)srcDst);
    return true;
  default:
    return false;
  }

  return false;
}

bool CISA_IR_Builder::CISA_create_svm_scatter_instruction(
    VISA_opnd *pred, SVMSubOpcode subopcode, VISA_EMask_Ctrl emask,
    unsigned exec_size, unsigned blockSize, unsigned numBlocks,
    VISA_opnd *addresses, VISA_opnd *srcDst, int lineNum) {
  VISA_SVM_Block_Type blockType = valueToVISASVMBlockType(blockSize);
  VISA_SVM_Block_Num blockNum = valueToVISASVMBlockNum(numBlocks);
  switch (subopcode) {
  case SVM_SCATTER:
    VISA_CALL_TO_BOOL(AppendVISASvmScatterInst, (VISA_PredOpnd *)pred, emask,
                      Get_VISA_Exec_Size_From_Raw_Size(exec_size), blockType,
                      blockNum, (VISA_RawOpnd *)addresses,
                      (VISA_RawOpnd *)srcDst);
    return true;
  case SVM_GATHER:
    VISA_CALL_TO_BOOL(AppendVISASvmGatherInst, (VISA_PredOpnd *)pred, emask,
                      Get_VISA_Exec_Size_From_Raw_Size(exec_size), blockType,
                      blockNum, (VISA_RawOpnd *)addresses,
                      (VISA_RawOpnd *)srcDst);
    return true;
  default:
    return false;
  }

  return false;
}

bool CISA_IR_Builder::CISA_create_svm_gather4_scaled(
    VISA_opnd *pred, VISA_EMask_Ctrl eMask, unsigned execSize,
    ChannelMask chMask, VISA_opnd *address, VISA_opnd *offsets, VISA_opnd *dst,
    int lineNum) {
  VISA_CALL_TO_BOOL(
      AppendVISASvmGather4ScaledInst, static_cast<VISA_PredOpnd *>(pred), eMask,
      Get_VISA_Exec_Size_From_Raw_Size(execSize), chMask.getAPI(),
      static_cast<VISA_VectorOpnd *>(address),
      static_cast<VISA_RawOpnd *>(offsets), static_cast<VISA_RawOpnd *>(dst));

  return true;
}

bool CISA_IR_Builder::CISA_create_svm_scatter4_scaled(
    VISA_opnd *pred, VISA_EMask_Ctrl eMask, unsigned execSize,
    ChannelMask chMask, VISA_opnd *address, VISA_opnd *offsets, VISA_opnd *src,
    int lineNum) {
  VISA_CALL_TO_BOOL(
      AppendVISASvmScatter4ScaledInst, static_cast<VISA_PredOpnd *>(pred),
      eMask, Get_VISA_Exec_Size_From_Raw_Size(execSize), chMask.getAPI(),
      static_cast<VISA_VectorOpnd *>(address),
      static_cast<VISA_RawOpnd *>(offsets), static_cast<VISA_RawOpnd *>(src));

  return true;
}

bool CISA_IR_Builder::CISA_create_svm_atomic_instruction(
    VISA_opnd *pred, VISA_EMask_Ctrl emask, unsigned exec_size,
    VISAAtomicOps op, unsigned short bitwidth, VISA_opnd *addresses,
    VISA_opnd *src0, VISA_opnd *src1, VISA_opnd *dst, int lineNum) {
  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  VISA_CALL_TO_BOOL(AppendVISASvmAtomicInst, (VISA_PredOpnd *)pred, emask,
                    executionSize, op, bitwidth, (VISA_RawOpnd *)addresses,
                    (VISA_RawOpnd *)src0, (VISA_RawOpnd *)src1,
                    (VISA_RawOpnd *)dst);
  return true;
}

bool CISA_IR_Builder::CISA_create_address_instruction(
    ISA_Opcode opcode, VISA_EMask_Ctrl emask, unsigned exec_size,
    VISA_opnd *dst, VISA_opnd *src0, VISA_opnd *src1, int lineNum) {
  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  VISA_CALL_TO_BOOL(AppendVISAAddrAddInst, emask, executionSize,
                    (VISA_VectorOpnd *)dst, (VISA_VectorOpnd *)src0,
                    (VISA_VectorOpnd *)src1);
  return true;
}

bool CISA_IR_Builder::CISA_create_logic_instruction(
    VISA_opnd *pred, ISA_Opcode opcode, bool sat, VISA_EMask_Ctrl emask,
    unsigned exec_size, VISA_opnd *dst, VISA_opnd *src0, VISA_opnd *src1,
    VISA_opnd *src2, VISA_opnd *src3, int lineNum) {
  if (opcode != ISA_SHR && opcode != ISA_SHL && opcode != ISA_ASR) {
    if (sat) {
      RecordParseError(lineNum, "saturation is not supported on this op");
    }
    sat = false;
    // fallthrough
  }

  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  VISA_CALL_TO_BOOL(AppendVISALogicOrShiftInst, opcode, (VISA_PredOpnd *)pred,
                    sat, emask, executionSize, (VISA_VectorOpnd *)dst,
                    (VISA_VectorOpnd *)src0, (VISA_VectorOpnd *)src1,
                    (VISA_VectorOpnd *)src2, (VISA_VectorOpnd *)src3);
  return true;
}

bool CISA_IR_Builder::CISA_create_logic_instruction(
    ISA_Opcode opcode, VISA_EMask_Ctrl emask, unsigned exec_size,
    CISA_GEN_VAR *dst, CISA_GEN_VAR *src0, CISA_GEN_VAR *src1, int lineNum) {
  if (opcode != ISA_AND && opcode != ISA_OR && opcode != ISA_NOT &&
      opcode != ISA_XOR) {
    RecordParseError(lineNum,
                     "prediate variables are not supported for this op");
    return false;
  }
  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  if (!dst) {
    RecordParseError(lineNum, "null dst in logic op");
    return false;
  }
  if (!src0) {
    RecordParseError(lineNum, "null src0 in logic op");
    return false;
  }
  if (opcode != ISA_NOT) {
    if (!src1) {
      RecordParseError(lineNum, "null src1 in logic op");
    }
  }
  VISA_CALL_TO_BOOL(AppendVISALogicOrShiftInst, opcode, emask, executionSize,
                    (VISA_PredVar *)dst, (VISA_PredVar *)src0,
                    (VISA_PredVar *)src1);
  return true;
}

bool CISA_IR_Builder::CISA_create_math_instruction(
    VISA_opnd *pred, ISA_Opcode opcode, bool sat, VISA_EMask_Ctrl emask,
    unsigned exec_size, VISA_opnd *dst, VISA_opnd *src0, VISA_opnd *src1,
    int lineNum) {
  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  VISA_CALL_TO_BOOL(AppendVISAArithmeticInst, opcode, (VISA_PredOpnd *)pred,
                    sat, emask, executionSize, (VISA_VectorOpnd *)dst,
                    (VISA_VectorOpnd *)src0, (VISA_VectorOpnd *)src1, NULL);
  return true;
}

bool CISA_IR_Builder::CISA_create_setp_instruction(
    ISA_Opcode opcode, VISA_EMask_Ctrl emask, unsigned exec_size,
    CISA_GEN_VAR *dst, VISA_opnd *src0, int lineNum) {
  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  VISA_CALL_TO_BOOL(AppendVISASetP, emask, executionSize, (VISA_PredVar *)dst,
                    (VISA_VectorOpnd *)src0);
  return true;
}

bool CISA_IR_Builder::CISA_create_sel_instruction(
    ISA_Opcode opcode, bool sat, VISA_opnd *pred, VISA_EMask_Ctrl emask,
    unsigned exec_size, VISA_opnd *dst, VISA_opnd *src0, VISA_opnd *src1,
    int lineNum) {
  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  VISA_CALL_TO_BOOL(AppendVISADataMovementInst, opcode, (VISA_PredOpnd *)pred,
                    sat, emask, executionSize, (VISA_VectorOpnd *)dst,
                    (VISA_VectorOpnd *)src0, (VISA_VectorOpnd *)src1);
  return true;
}

bool CISA_IR_Builder::CISA_create_fminmax_instruction(
    bool minmax, ISA_Opcode opcode, bool sat, VISA_opnd *pred,
    VISA_EMask_Ctrl emask, unsigned exec_size, VISA_opnd *dst, VISA_opnd *src0,
    VISA_opnd *src1, int lineNum) {
  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  VISA_CALL_TO_BOOL(AppendVISAMinMaxInst,
                    (minmax ? CISA_DM_FMAX : CISA_DM_FMIN), sat, emask,
                    executionSize, (VISA_VectorOpnd *)dst,
                    (VISA_VectorOpnd *)src0, (VISA_VectorOpnd *)src1);
  return true;
}

bool CISA_IR_Builder::CISA_create_scatter_instruction(
    ISA_Opcode opcode, int elt_size, VISA_EMask_Ctrl emask, unsigned elemNum,
    bool modifier, const char *surfaceName,
    VISA_opnd *global_offset,  // global_offset
    VISA_opnd *element_offset, // element_offset
    VISA_opnd *raw_dst_src,    // dst/src
    int lineNum) {
  // GATHER  0x39 (GATHER)  Elt_size   Is_modified Num_elts    Surface
  // Global_Offset   Element_Offset  Dst SCATTER 0x3A (SCATTER) Elt_size
  // Num_elts    Surface Global_Offset   Element_Offset  Src
  VISA_StateOpndHandle *surface =
      CISA_get_surface_variable(surfaceName, lineNum);
  if (!surface)
    return false; // error recorded

  if (elemNum != 16 && elemNum != 8 && elemNum != 1) {
    RecordParseError(
        lineNum,
        "unsupported number of elements for gather/scatter instruction.");
  }

  VISA_Exec_Size executionSize = EXEC_SIZE_16;
  if (elemNum == 16) {
    executionSize = EXEC_SIZE_16;
  } else if (elemNum == 8) {
    executionSize = EXEC_SIZE_8;
  } else if (elemNum == 1) {
    executionSize = EXEC_SIZE_1;
  }

  GATHER_SCATTER_ELEMENT_SIZE elementSize = GATHER_SCATTER_BYTE_UNDEF;
  if (elt_size == 1) {
    elementSize = GATHER_SCATTER_BYTE;
  } else if (elt_size == 2) {
    elementSize = GATHER_SCATTER_WORD;
  } else if (elt_size == 4) {
    elementSize = GATHER_SCATTER_DWORD;
  }

  VISA_CALL_TO_BOOL(
      AppendVISASurfAccessGatherScatterInst, opcode, emask, elementSize,
      executionSize, surface, (VISA_VectorOpnd *)global_offset,
      (VISA_RawOpnd *)element_offset, (VISA_RawOpnd *)raw_dst_src);
  return true;
}

bool CISA_IR_Builder::CISA_create_scatter4_typed_instruction(
    ISA_Opcode opcode, VISA_opnd *pred, ChannelMask ch_mask,
    VISA_EMask_Ctrl emask, unsigned execSize, const char *surfaceName,
    VISA_opnd *uOffset, VISA_opnd *vOffset, VISA_opnd *rOffset, VISA_opnd *lod,
    VISA_opnd *dst, int lineNum) {
  VISA_StateOpndHandle *surface =
      CISA_get_surface_variable(surfaceName, lineNum);
  if (!surface)
    return false; // error recorded

  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(execSize);
  VISA_CALL_TO_BOOL(AppendVISASurfAccessGather4Scatter4TypedInst, opcode,
                    (VISA_PredOpnd *)pred, ch_mask.getAPI(), emask,
                    executionSize, surface, (VISA_RawOpnd *)uOffset,
                    (VISA_RawOpnd *)vOffset, (VISA_RawOpnd *)rOffset,
                    (VISA_RawOpnd *)lod, (VISA_RawOpnd *)dst);
  return true;
}

bool CISA_IR_Builder::CISA_create_scatter4_scaled_instruction(
    ISA_Opcode opcode, VISA_opnd *pred, VISA_EMask_Ctrl eMask,
    unsigned execSize, ChannelMask chMask, const char *surfaceName,
    VISA_opnd *globalOffset, VISA_opnd *offsets, VISA_opnd *dstSrc,
    int lineNum) {
  VISA_StateOpndHandle *surface =
      CISA_get_surface_variable(surfaceName, lineNum);
  if (!surface)
    return false; // error recorded

  VISA_CALL_TO_BOOL(AppendVISASurfAccessGather4Scatter4ScaledInst, opcode,
                    static_cast<VISA_PredOpnd *>(pred), eMask,
                    Get_VISA_Exec_Size_From_Raw_Size(execSize), chMask.getAPI(),
                    surface, static_cast<VISA_VectorOpnd *>(globalOffset),
                    static_cast<VISA_RawOpnd *>(offsets),
                    static_cast<VISA_RawOpnd *>(dstSrc));

  return true;
}

bool CISA_IR_Builder::CISA_create_scatter_scaled_instruction(
    ISA_Opcode opcode, VISA_opnd *pred, VISA_EMask_Ctrl eMask,
    unsigned execSize, unsigned numBlocks, const char *surfaceName,
    VISA_opnd *globalOffset, VISA_opnd *offsets, VISA_opnd *dstSrc,
    int lineNum) {
  VISA_StateOpndHandle *surface =
      CISA_get_surface_variable(surfaceName, lineNum);
  if (!surface)
    return false; // error recorded

  VISA_CALL_TO_BOOL(AppendVISASurfAccessScatterScaledInst, opcode,
                    static_cast<VISA_PredOpnd *>(pred), eMask,
                    Get_VISA_Exec_Size_From_Raw_Size(execSize),
                    valueToVISASVMBlockNum(numBlocks), surface,
                    static_cast<VISA_VectorOpnd *>(globalOffset),
                    static_cast<VISA_RawOpnd *>(offsets),
                    static_cast<VISA_RawOpnd *>(dstSrc));

  return true;
}

bool CISA_IR_Builder::CISA_create_sync_instruction(ISA_Opcode opcode,
                                                   int lineNum) {
  VISA_CALL_TO_BOOL(AppendVISASyncInst, opcode);
  return true;
}

bool CISA_IR_Builder::CISA_create_sbarrier_instruction(bool isSignal,
                                                       int lineNum) {
  VISA_CALL_TO_BOOL(AppendVISASplitBarrierInst, isSignal);
  return true;
}

bool CISA_IR_Builder::CISA_create_FILE_instruction(ISA_Opcode opcode,
                                                   const char *file_name,
                                                   int lineNum) {
  VISA_CALL_TO_BOOL(AppendVISAMiscFileInst, file_name);
  return true;
}

bool CISA_IR_Builder::CISA_create_LOC_instruction(ISA_Opcode opcode,
                                                  unsigned int loc,
                                                  int lineNum) {
  VISA_CALL_TO_BOOL(AppendVISAMiscLOC, loc);
  return true;
}

bool CISA_IR_Builder::CISA_create_invtri_inst(
    VISA_opnd *pred, ISA_Opcode opcode, bool sat, VISA_EMask_Ctrl emask,
    unsigned exec_size, VISA_opnd *dst, VISA_opnd *src0, int lineNum) {
  int num_operands = 0;
  VISA_INST_Desc *inst_desc = NULL;
  VISA_opnd *opnd[4];
  inst_desc = &CISA_INST_table[opcode];
  VISA_Modifier mod = MODIFIER_NONE;

  if (sat)
    mod = MODIFIER_SAT;

  if (dst != NULL) {
    dst->_opnd.v_opnd.tag += mod << 3;
    opnd[num_operands] = dst;
    num_operands++;
  }

  if (src0 != NULL) {
    opnd[num_operands] = src0;
    num_operands++;
  }

  PredicateOpnd predOpnd =
      pred ? pred->convertToPred() : PredicateOpnd::getNullPred();
  CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

  unsigned char size = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  size += emask << 4;
  inst->createCisaInstruction(opcode, size, 0, predOpnd, opnd, num_operands,
                              inst_desc);
  m_kernel->addInstructionToEnd(inst);

  return true;
}

bool CISA_IR_Builder::CISA_create_dword_atomic_instruction(
    VISA_opnd *pred, VISAAtomicOps subOpc, bool is16Bit, VISA_EMask_Ctrl eMask,
    unsigned execSize, const char *surfaceName, VISA_opnd *offsets,
    VISA_opnd *src0, VISA_opnd *src1, VISA_opnd *dst, int lineNum) {
  VISA_StateOpndHandle *surface =
      CISA_get_surface_variable(surfaceName, lineNum);
  if (!surface)
    return false; // error recorded

  VISA_CALL_TO_BOOL(
      AppendVISASurfAccessDwordAtomicInst, static_cast<VISA_PredOpnd *>(pred),
      subOpc, is16Bit, eMask, Get_VISA_Exec_Size_From_Raw_Size(execSize),
      surface, static_cast<VISA_RawOpnd *>(offsets),
      static_cast<VISA_RawOpnd *>(src0), static_cast<VISA_RawOpnd *>(src1),
      static_cast<VISA_RawOpnd *>(dst));

  return true;
}

bool CISA_IR_Builder::CISA_create_typed_atomic_instruction(
    VISA_opnd *pred, VISAAtomicOps subOpc, bool is16Bit, VISA_EMask_Ctrl eMask,
    unsigned execSize, const char *surfaceName, VISA_opnd *u, VISA_opnd *v,
    VISA_opnd *r, VISA_opnd *lod, VISA_opnd *src0, VISA_opnd *src1,
    VISA_opnd *dst, int lineNum) {
  VISA_StateOpndHandle *surface =
      CISA_get_surface_variable(surfaceName, lineNum);
  if (!surface)
    return false; // error recorded

  VISA_CALL_TO_BOOL(
      AppendVISA3dTypedAtomic, subOpc, is16Bit,
      static_cast<VISA_PredOpnd *>(pred), eMask,
      Get_VISA_Exec_Size_From_Raw_Size(execSize), surface,
      static_cast<VISA_RawOpnd *>(u), static_cast<VISA_RawOpnd *>(v),
      static_cast<VISA_RawOpnd *>(r), static_cast<VISA_RawOpnd *>(lod),
      static_cast<VISA_RawOpnd *>(src0), static_cast<VISA_RawOpnd *>(src1),
      static_cast<VISA_RawOpnd *>(dst));

  return true;
}

bool CISA_IR_Builder::CISA_create_avs_instruction(
    ChannelMask channel, const char *surfaceName, const char *samplerName,
    VISA_opnd *u_offset, VISA_opnd *v_offset, VISA_opnd *deltaU,
    VISA_opnd *deltaV, VISA_opnd *u2d, VISA_opnd *groupID,
    VISA_opnd *verticalBlockNumber, OutputFormatControl cntrl, VISA_opnd *v2d,
    AVSExecMode execMode, VISA_opnd *iefbypass, VISA_opnd *dst, int lineNum) {
  VISA_StateOpndHandle *surface =
      CISA_get_surface_variable(surfaceName, lineNum);
  if (!surface)
    return false; // error recorded

  VISA_StateOpndHandle *sampler =
      CISA_get_sampler_variable(samplerName, lineNum);
  if (!sampler)
    return false; // error already reported

  VISA_CALL_TO_BOOL(AppendVISAMEAVS, surface, sampler, channel.getAPI(),
                    (VISA_VectorOpnd *)u_offset, (VISA_VectorOpnd *)v_offset,
                    (VISA_VectorOpnd *)deltaU, (VISA_VectorOpnd *)deltaV,
                    (VISA_VectorOpnd *)u2d, (VISA_VectorOpnd *)v2d,
                    (VISA_VectorOpnd *)groupID,
                    (VISA_VectorOpnd *)verticalBlockNumber, cntrl, execMode,
                    (VISA_VectorOpnd *)iefbypass, (VISA_RawOpnd *)dst);
  return true;
}

bool CISA_IR_Builder::CISA_create_urb_write_3d_instruction(
    VISA_opnd *pred, VISA_EMask_Ctrl emask, unsigned exec_size,
    unsigned int num_out, unsigned int global_offset, VISA_opnd *channel_mask,
    VISA_opnd *urb_handle, VISA_opnd *per_slot_offset, VISA_opnd *vertex_data,
    int lineNum) {
  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  VISA_CALL_TO_BOOL(AppendVISA3dURBWrite, (VISA_PredOpnd *)pred, emask,
                    executionSize, (unsigned char)num_out,
                    (VISA_RawOpnd *)channel_mask, (unsigned short)global_offset,
                    (VISA_RawOpnd *)urb_handle, (VISA_RawOpnd *)per_slot_offset,
                    (VISA_RawOpnd *)vertex_data);
  return true;
}

bool CISA_IR_Builder::CISA_create_rtwrite_3d_instruction(
    VISA_opnd *pred, const char *mode, VISA_EMask_Ctrl emask,
    unsigned exec_size, const char *surfaceName,
    const std::vector<VISA_opnd *> &operands, int lineNum) {
  vISA_RT_CONTROLS cntrls;

  memset(&cntrls, 0, sizeof(vISA_RT_CONTROLS));

  VISA_opnd *s0a = NULL;
  VISA_opnd *oM = NULL;
  VISA_opnd *R = NULL;
  VISA_opnd *G = NULL;
  VISA_opnd *B = NULL;
  VISA_opnd *A = NULL;
  VISA_opnd *Z = NULL;
  VISA_opnd *Stencil = NULL;
  VISA_opnd *CPSCounter = NULL;
  VISA_opnd *SamplerIndex = NULL;
  VISA_opnd *r1Header = NULL;
  VISA_opnd *rti = NULL;
  uint8_t counter = 0;

  r1Header = operands[counter++];

  if (mode != NULL) {
    if (strstr(mode, "<SI>")) {
      SamplerIndex = operands[counter++];
    }

    if (strstr(mode, "<CPS>")) {
      CPSCounter = operands[counter++];
    }

    if (strstr(mode, "<RTI>")) {
      cntrls.RTIndexPresent = true;
      rti = operands[counter++];
    }

    if (strstr(mode, "<A>")) {
      cntrls.s0aPresent = true;
      s0a = operands[counter++];
    }

    if (strstr(mode, "<O>")) {
      cntrls.oMPresent = true;
      oM = operands[counter++];
    }
    R = operands[counter++];
    G = operands[counter++];
    B = operands[counter++];
    A = operands[counter++];

    if (strstr(mode, "<Z>")) {
      cntrls.zPresent = true;
      Z = operands[counter++];
    }

    if (strstr(mode, "<ST>")) {
      cntrls.isStencil = true;
      Stencil = operands[counter++];
    }

    if (strstr(mode, "<LRTW>")) {
      cntrls.isLastWrite = true;
    }

    if (strstr(mode, "<PS>")) {
      cntrls.isPerSample = true;
    }

    if (strstr(mode, "CM")) {
      cntrls.isCoarseMode = true;
    }

    if (strstr(mode, "NULLRT")) {
      cntrls.isNullRT = true;
    }
  } else {
    R = operands[counter++];
    G = operands[counter++];
    B = operands[counter++];
    A = operands[counter++];
  }

  VISA_StateOpndHandle* surface = nullptr;
  if (surfaceName != nullptr) {
    surface = CISA_get_surface_variable(surfaceName, lineNum);
    if (!surface)
      return false; // error recorded
  }

  uint8_t numMsgSpecificOpnd = 0;
  VISA_RawOpnd *rawOpnds[20];

#define APPEND_NON_NULL_RAW_OPND(opnd)                                         \
  if (opnd != NULL) {                                                          \
    rawOpnds[numMsgSpecificOpnd++] = (VISA_RawOpnd *)opnd;                     \
  }

  APPEND_NON_NULL_RAW_OPND(s0a);
  APPEND_NON_NULL_RAW_OPND(oM);
  APPEND_NON_NULL_RAW_OPND(R);
  APPEND_NON_NULL_RAW_OPND(G);
  APPEND_NON_NULL_RAW_OPND(B);
  APPEND_NON_NULL_RAW_OPND(A);
  APPEND_NON_NULL_RAW_OPND(Z);
  APPEND_NON_NULL_RAW_OPND(Stencil);
  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  VISA_CALL_TO_BOOL(AppendVISA3dRTWriteCPS, (VISA_PredOpnd *)pred, emask,
                    executionSize, (VISA_VectorOpnd *)rti, cntrls, surface,
                    (VISA_RawOpnd *)r1Header, (VISA_VectorOpnd *)SamplerIndex,
                    (VISA_VectorOpnd *)CPSCounter, numMsgSpecificOpnd,
                    rawOpnds);

  return true;
}


bool CISA_IR_Builder::CISA_create_info_3d_instruction(
    VISASampler3DSubOpCode subOpcode, VISA_EMask_Ctrl emask, unsigned exec_size,
    ChannelMask channel, const char *surfaceName, unsigned int surfaceIndex,
    VISA_opnd *lod, VISA_opnd *dst, int lineNum) {

  VISA_StateOpndHandle *surface = nullptr;
  surface = CISA_get_surface_variable(surfaceName, lineNum);
  if (!surface)
    return false; // error recorded

  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  VISA_CALL_TO_BOOL(AppendVISA3dInfo, subOpcode, emask, executionSize,
                    channel.getAPI(), surface, surfaceIndex, (VISA_RawOpnd *)lod,
                    (VISA_RawOpnd *)dst);
  return true;
}

bool CISA_IR_Builder::createSample4Instruction(
    VISA_opnd *pred, VISASampler3DSubOpCode subOpcode, bool pixelNullMask,
    ChannelMask channel, VISA_EMask_Ctrl emask, unsigned exec_size,
    VISA_opnd *aoffimmi, const char *samplerName, unsigned int samplerIndex,
    const char *surfaceName, unsigned int surfaceIndex,
    VISA_opnd *pairedSurface, VISA_opnd *dst, unsigned int numParameters,
    VISA_RawOpnd **params, int lineNum) {
  VISA_StateOpndHandle *surface = nullptr;

  surface = CISA_get_surface_variable(surfaceName, lineNum);
  if (!surface)
    return false; // error recorded

  VISA_StateOpndHandle *sampler = nullptr;
  sampler = CISA_get_sampler_variable(samplerName, lineNum);
  if (!sampler)
    return false; // error already reported

  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);

  if (channel.getNumEnabledChannels() != 1) {
    RecordParseError(
        lineNum, "one one of R,G,B,A may be specified for sample4 instruction");
    return false;
  }
  int status = m_kernel->AppendVISA3dGather4(
      subOpcode, pixelNullMask, (VISA_PredOpnd *)pred, emask, executionSize,
      channel.getSingleChannel(), (VISA_VectorOpnd *)aoffimmi, sampler,
      samplerIndex, surface, surfaceIndex, (VISA_RawOpnd *)pairedSurface,
      (VISA_RawOpnd *)dst, numParameters, params);
  VISA_RESULT_CALL_TO_BOOL(status);
  return true;
}

bool CISA_IR_Builder::create3DLoadInstruction(
    VISA_opnd *pred, VISASampler3DSubOpCode subOpcode, bool pixelNullMask,
    ChannelMask channels, VISA_EMask_Ctrl emask, unsigned exec_size,
    VISA_opnd *aoffimmi, const char *surfaceName, unsigned int surfaceIndex,
    VISA_opnd *pairedSurface, VISA_opnd *dst, unsigned int numParameters,
    VISA_RawOpnd **params, int lineNum) {
  VISA_StateOpndHandle *surface = nullptr;
  surface = CISA_get_surface_variable(surfaceName, lineNum);
  if (!surface)
    return false; // error recorded

  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  int status = m_kernel->AppendVISA3dLoad(
      subOpcode, pixelNullMask, (VISA_PredOpnd *)pred, emask, executionSize,
      channels.getAPI(), (VISA_VectorOpnd *)aoffimmi, surface, surfaceIndex,
      (VISA_RawOpnd *)pairedSurface, (VISA_RawOpnd *)dst, numParameters,
      params);
  VISA_RESULT_CALL_TO_BOOL(status);
  return true;
}

bool CISA_IR_Builder::create3DSampleInstruction(
    VISA_opnd *pred, VISASampler3DSubOpCode subOpcode, bool pixelNullMask,
    bool cpsEnable, bool uniformSampler, ChannelMask channels,
    VISA_EMask_Ctrl emask, unsigned exec_size, VISA_opnd *aoffimmi,
    const char *samplerName, unsigned int samplerIdx, const char *surfaceName,
    unsigned int surfaceIdx, VISA_opnd *pairedSurface, VISA_opnd *dst,
    unsigned int numParameters, VISA_RawOpnd **params, int lineNum) {
  VISA_StateOpndHandle *surface = nullptr;
  vISA_ASSERT_INPUT(samplerIdx == 0 && surfaceIdx == 0,
      "sampler and surface index must be 0");

  surface = CISA_get_surface_variable(surfaceName, lineNum);
  if (!surface) {
    return false; // error already reported
  }

  VISA_StateOpndHandle *sampler = nullptr;
  sampler = CISA_get_sampler_variable(samplerName, lineNum);
  if (!sampler) {
    return false; // error already reported
  }

  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);

  int status = m_kernel->AppendVISA3dSampler(
      subOpcode, pixelNullMask, cpsEnable, uniformSampler,
      (VISA_PredOpnd *)pred, emask, executionSize, channels.getAPI(),
      (VISA_VectorOpnd *)aoffimmi, sampler, samplerIdx, surface, surfaceIdx,
      (VISA_RawOpnd *)pairedSurface, (VISA_RawOpnd *)dst, numParameters,
      params);
  VISA_RESULT_CALL_TO_BOOL(status);
  return true;
}

bool CISA_IR_Builder::CISA_create_sample_instruction(
    ISA_Opcode opcode, ChannelMask channel, int simd_mode,
    const char *samplerName, const char *surfaceName, VISA_opnd *u_opnd,
    VISA_opnd *v_opnd, VISA_opnd *r_opnd, VISA_opnd *dst, int lineNum) {
  VISA_StateOpndHandle *surface =
      CISA_get_surface_variable(surfaceName, lineNum);
  if (!surface)
    return false; // error recorded

  if (opcode == ISA_SAMPLE) {
    VISA_StateOpndHandle *sampler =
        CISA_get_sampler_variable(samplerName, lineNum);
    if (!sampler)
      return false; // error recorded

    VISA_CALL_TO_BOOL(AppendVISASISample, vISA_EMASK_M1, surface, sampler,
                      channel.getAPI(), simd_mode == 16, (VISA_RawOpnd *)u_opnd,
                      (VISA_RawOpnd *)v_opnd, (VISA_RawOpnd *)r_opnd,
                      (VISA_RawOpnd *)dst);

  } else if (opcode == ISA_LOAD) {
    VISA_CALL_TO_BOOL(AppendVISASILoad, surface, channel.getAPI(),
                      simd_mode == 16, (VISA_RawOpnd *)u_opnd,
                      (VISA_RawOpnd *)v_opnd, (VISA_RawOpnd *)r_opnd,
                      (VISA_RawOpnd *)dst);
  } else {
    RecordParseError(lineNum, (int)opcode, ": unsupported sampler mnemonic");
    return false;
  }

  return true;
}

bool CISA_IR_Builder::CISA_create_sampleunorm_instruction(
    ISA_Opcode opcode, ChannelMask channel, CHANNEL_OUTPUT_FORMAT out,
    const char *samplerName, const char *surfaceName, VISA_opnd *src0,
    VISA_opnd *src1, VISA_opnd *src2, VISA_opnd *src3, VISA_opnd *dst,
    int lineNum) {
  VISA_StateOpndHandle *surface =
      CISA_get_surface_variable(surfaceName, lineNum);
  if (!surface)
    return false; // error recorded

  VISA_StateOpndHandle *sampler =
      CISA_get_sampler_variable(samplerName, lineNum);
  if (!sampler)
    return false; // error recorded

  VISA_CALL_TO_BOOL(AppendVISASISampleUnorm, surface, sampler, channel.getAPI(),
                    (VISA_VectorOpnd *)src0, (VISA_VectorOpnd *)src1,
                    (VISA_VectorOpnd *)src2, (VISA_VectorOpnd *)src3,
                    (VISA_RawOpnd *)dst, out);

  return true;
}

bool CISA_IR_Builder::CISA_create_vme_ime_instruction(
    ISA_Opcode opcode, unsigned char stream_mode, unsigned char searchCtrl,
    VISA_opnd *input_opnd, VISA_opnd *ime_input_opnd, const char *surfaceName,
    VISA_opnd *ref0_opnd, VISA_opnd *ref1_opnd, VISA_opnd *costCenter_opnd,
    VISA_opnd *dst_opnd, int lineNum) {
  VISA_StateOpndHandle *surface =
      CISA_get_surface_variable(surfaceName, lineNum);
  if (!surface)
    return false; // error recorded

  VISA_CALL_TO_BOOL(AppendVISAMiscVME_IME, surface, stream_mode, searchCtrl,
                    (VISA_RawOpnd *)input_opnd, (VISA_RawOpnd *)ime_input_opnd,
                    (VISA_RawOpnd *)ref0_opnd, (VISA_RawOpnd *)ref1_opnd,
                    (VISA_RawOpnd *)costCenter_opnd, (VISA_RawOpnd *)dst_opnd);

  return true;
}

bool CISA_IR_Builder::CISA_create_vme_sic_instruction(
    ISA_Opcode opcode, VISA_opnd *input_opnd, VISA_opnd *sic_input_opnd,
    const char *surfaceName, VISA_opnd *dst, int lineNum) {
  VISA_StateOpndHandle *surface =
      CISA_get_surface_variable(surfaceName, lineNum);
  if (!surface)
    return false; // error recorded

  VISA_CALL_TO_BOOL(AppendVISAMiscVME_SIC, surface, (VISA_RawOpnd *)input_opnd,
                    (VISA_RawOpnd *)sic_input_opnd, (VISA_RawOpnd *)dst);
  return true;
}

bool CISA_IR_Builder::CISA_create_vme_fbr_instruction(
    ISA_Opcode opcode, VISA_opnd *input_opnd, VISA_opnd *fbr_input_opnd,
    const char *surfaceName, VISA_opnd *fbrMbMode, VISA_opnd *fbrSubMbShape,
    VISA_opnd *fbrSubPredMode, VISA_opnd *dst, int lineNum) {
  VISA_StateOpndHandle *surface =
      CISA_get_surface_variable(surfaceName, lineNum);
  if (!surface)
    return false; // error recorded

  VISA_CALL_TO_BOOL(AppendVISAMiscVME_FBR, surface, (VISA_RawOpnd *)input_opnd,
                    (VISA_RawOpnd *)fbr_input_opnd,
                    (VISA_VectorOpnd *)fbrMbMode,
                    (VISA_VectorOpnd *)fbrSubMbShape,
                    (VISA_VectorOpnd *)fbrSubPredMode, (VISA_RawOpnd *)dst);
  return true;
}

bool CISA_IR_Builder::CISA_create_NO_OPND_instruction(ISA_Opcode opcode,
                                                      int lineNum) {
  VISA_CALL_TO_BOOL(AppendVISASyncInst, opcode);
  return true;
}

bool CISA_IR_Builder::CISA_create_switch_instruction(
    ISA_Opcode opcode, unsigned exec_size, VISA_opnd *indexOpnd,
    const std::deque<const char *> &labels, int lineNum) {
  int numLabels = (int)labels.size();
  std::vector<VISA_LabelOpnd *> jmpTargets(numLabels);
  for (int i = 0; i < numLabels; ++i) {
    auto labelOpnd = m_kernel->getLabelOpndFromLabelName(labels[i]);

    // forward jump label: create the label optimistically
    if (!labelOpnd) {
      VISA_CALL_TO_BOOL(CreateVISALabelVar, labelOpnd, labels[i], LABEL_BLOCK);
      if (!m_kernel->setLabelOpndNameMap(labels[i], labelOpnd, LABEL_BLOCK))
        return false;
    }
    jmpTargets[i] = labelOpnd;
  }

  VISA_CALL_TO_BOOL(AppendVISACFSwitchJMPInst, (VISA_VectorOpnd *)indexOpnd,
                    (uint8_t)numLabels, jmpTargets.data());

  return true;
}

bool CISA_IR_Builder::CISA_create_fcall_instruction(
    VISA_opnd *pred_opnd, ISA_Opcode opcode, VISA_EMask_Ctrl emask,
    unsigned exec_size, const char *funcName, unsigned arg_size,
    unsigned return_size,
    int lineNum) // last index
{
  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  VISA_CALL_TO_BOOL(AppendVISACFFunctionCallInst, (VISA_PredOpnd *)pred_opnd,
                    emask, executionSize, std::string(funcName),
                    (unsigned char)arg_size, (unsigned char)return_size);
  return true;
}

bool CISA_IR_Builder::CISA_create_ifcall_instruction(
    VISA_opnd *pred_opnd, VISA_EMask_Ctrl emask, unsigned exec_size,
    bool isUniform, VISA_opnd *funcAddr, unsigned arg_size,
    unsigned return_size,
    int lineNum) // last index
{
  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  VISA_CALL_TO_BOOL(AppendVISACFIndirectFuncCallInst,
                    (VISA_PredOpnd *)pred_opnd, emask, executionSize, isUniform,
                    (VISA_VectorOpnd *)funcAddr, (uint8_t)arg_size,
                    (uint8_t)return_size);
  return true;
}

bool CISA_IR_Builder::CISA_create_faddr_instruction(const char *sym_name,
                                                    VISA_opnd *dst,
                                                    int lineNum) {
  VISA_CALL_TO_BOOL(AppendVISACFSymbolInst, std::string(sym_name),
                    (VISA_VectorOpnd *)dst);
  return true;
}

bool CISA_IR_Builder::CISA_create_raw_send_instruction(
    ISA_Opcode opcode, unsigned char modifier, VISA_EMask_Ctrl emask,
    unsigned exec_size, VISA_opnd *pred_opnd, unsigned int exMsgDesc,
    unsigned char srcSize, unsigned char dstSize, VISA_opnd *Desc,
    VISA_opnd *Src, VISA_opnd *Dst, int lineNum) {
  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  VISA_CALL_TO_BOOL(AppendVISAMiscRawSend, (VISA_PredOpnd *)pred_opnd, emask,
                    executionSize, modifier, exMsgDesc, srcSize, dstSize,
                    (VISA_VectorOpnd *)Desc, (VISA_RawOpnd *)Src,
                    (VISA_RawOpnd *)Dst);
  return true;
}

bool CISA_IR_Builder::CISA_create_lifetime_inst(unsigned char startOrEnd,
                                                const char *src, int lineNum) {
  // src is a string representation of variable.
  // Scan entire symbol table to find variable whose name
  // corresponds to src.
  CISA_GEN_VAR *cisaVar = m_kernel->getDeclFromName(src);
  if (!cisaVar) {
    RecordParseError(lineNum, "lifetime operand not found");
    return false;
  }

  VISA_opnd *var = NULL;
  if (cisaVar->type == GENERAL_VAR) {
    var =
        CISA_create_gen_src_operand(src, 0, 1, 0, 0, 0, MODIFIER_NONE, lineNum);
  } else if (cisaVar->type == ADDRESS_VAR) {
    var = CISA_set_address_operand(cisaVar, 0, 1, (startOrEnd == 0), lineNum);
  } else if (cisaVar->type == PREDICATE_VAR) {
    var = CISA_create_predicate_operand(cisaVar, PredState_NO_INVERSE,
                                        PRED_CTRL_NON, lineNum);
  } else {
    RecordParseError(lineNum, src, ": invalid variable type for lifetime");
    return false;
  }

  VISA_CALL_TO_BOOL(AppendVISALifetime, (VISAVarLifetime)startOrEnd,
                    (VISA_VectorOpnd *)var);
  return true;
}

bool CISA_IR_Builder::CISA_create_raw_sends_instruction(
    ISA_Opcode opcode, unsigned char modifier, bool hasEOT,
    VISA_EMask_Ctrl emask, unsigned exec_size, VISA_opnd *pred_opnd,
    VISA_opnd *exMsgDesc, unsigned char ffid, unsigned char src0Size,
    unsigned char src1Size, unsigned char dstSize, VISA_opnd *Desc,
    VISA_opnd *Src0, VISA_opnd *Src1, VISA_opnd *Dst, int lineNum) {
  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);

  VISA_CALL_TO_BOOL(AppendVISAMiscRawSends, (VISA_PredOpnd *)pred_opnd, emask,
                    executionSize, modifier, ffid, (VISA_VectorOpnd *)exMsgDesc,
                    src0Size, src1Size, dstSize, (VISA_VectorOpnd *)Desc,
                    (VISA_RawOpnd *)Src0, (VISA_RawOpnd *)Src1,
                    (VISA_RawOpnd *)Dst, hasEOT);

  return true;
}

/*
Should be only called from CISA 2.4+
*/
bool CISA_IR_Builder::CISA_create_fence_instruction(ISA_Opcode opcode,
                                                    unsigned char mode,
                                                    int lineNum) {
  VISA_CALL_TO_BOOL(AppendVISASyncInst, opcode, mode);
  return true;
}

bool CISA_IR_Builder::CISA_create_wait_instruction(VISA_opnd *mask,
                                                   int lineNum) {
  VISA_CALL_TO_BOOL(AppendVISAWaitInst, (VISA_VectorOpnd *)mask);
  return true;
}

/*** CISA 3.0 and later ***/
bool CISA_IR_Builder::CISA_create_yield_instruction(ISA_Opcode opcode,
                                                    int lineNum) {
  VISA_CALL_TO_BOOL(AppendVISASyncInst, opcode);
  return true;
}

VISA_opnd *CISA_IR_Builder::CISA_create_gen_src_operand(
    const char *var_name, short v_stride, short width, short h_stride,
    unsigned char row_offset, unsigned char col_offset, VISA_Modifier mod,
    int lineNum) {
  auto *decl = (VISA_GenVar *)m_kernel->getDeclFromName(var_name);
  if (!decl) {
    RecordParseError(lineNum, var_name, ": unbound identifier");
    return nullptr;
  } else if (decl->type != GENERAL_VAR) {
    RecordParseError(lineNum, var_name, ": not a general register variable");
    return nullptr;
  }

  VISA_VectorOpnd *cisa_opnd = nullptr;
  int status = m_kernel->CreateVISASrcOperand(
      cisa_opnd, decl, mod, v_stride, width, h_stride, row_offset, col_offset);
  if (status != VISA_SUCCESS)
    RecordParseError(lineNum, "unknown error creating src operand");
  return (VISA_opnd *)cisa_opnd;
}

VISA_opnd *CISA_IR_Builder::CISA_dst_general_operand(const char *var_name,
                                                     unsigned char roff,
                                                     unsigned char sroff,
                                                     unsigned short hstride,
                                                     int lineNum) {
  auto *decl = (VISA_GenVar *)m_kernel->getDeclFromName(var_name);
  if (!decl) {
    RecordParseError(lineNum, var_name, ": unbound identifier");
    return nullptr;
  } else if (decl->type != GENERAL_VAR) {
    RecordParseError(lineNum, var_name, ": not a general register variable");
    return nullptr;
  }

  VISA_VectorOpnd *cisa_opnd = nullptr;
  int status =
      m_kernel->CreateVISADstOperand(cisa_opnd, decl, hstride, roff, sroff);
  if (status != VISA_SUCCESS)
    RecordParseError(lineNum, "unknown error creating dst operand");
  return (VISA_opnd *)cisa_opnd;
}

attr_gen_struct *CISA_IR_Builder::CISA_Create_Attr(const char *AttrName,
                                                   int64_t I64Val,
                                                   const char *CStrVal) {
  attr_gen_struct *newAttr =
      (attr_gen_struct *)m_mem.alloc(sizeof(attr_gen_struct));
  Attributes::ID aID = Attributes::getAttributeID(AttrName);
  vISA_ASSERT_INPUT(Attributes::isValid(aID), "vISA: unknown attribute!");
  if (Attributes::isInt32(aID) || Attributes::isBool(aID)) {
    newAttr->isInt = true;
    // No i64 attribute value yet
    newAttr->value = (int32_t)I64Val;
  } else if (Attributes::isCStr(aID)) {
    newAttr->isInt = false;
    newAttr->string_val = CStrVal;
  }
  newAttr->name = AttrName;
  newAttr->attr_set = true;
  return newAttr;
}

VISA_opnd *CISA_IR_Builder::CISA_create_immed(uint64_t value, VISA_Type type,
                                              int lineNum) {
  VISA_VectorOpnd *cisa_opnd = NULL;

  VISA_CALL_TO_NULLPTR(CreateVISAImmediate, cisa_opnd, &value, type);
  if (type == ISA_TYPE_Q || type == ISA_TYPE_UQ) {
    cisa_opnd->_opnd.v_opnd.opnd_val.const_opnd._val.lval = value;
  } else {
    cisa_opnd->_opnd.v_opnd.opnd_val.const_opnd._val.ival = (uint32_t)value;
  }
  return (VISA_opnd *)cisa_opnd;
}

VISA_opnd *CISA_IR_Builder::CISA_create_float_immed(double value,
                                                    VISA_Type type,
                                                    int lineNum) {
  VISA_VectorOpnd *cisa_opnd = nullptr;
  if (type == ISA_TYPE_F) {
    float temp = (float)value;
    VISA_CALL_TO_NULLPTR(CreateVISAImmediate, cisa_opnd, &temp, type);
  } else {
    VISA_CALL_TO_NULLPTR(CreateVISAImmediate, cisa_opnd, &value, type);
  }

  return (VISA_opnd *)cisa_opnd;
}

CISA_GEN_VAR *CISA_IR_Builder::CISA_find_decl(const char *var_name) {
  return m_kernel->getDeclFromName(var_name);
}

VISA_opnd *CISA_IR_Builder::CISA_set_address_operand(CISA_GEN_VAR *cisa_decl,
                                                     unsigned char offset,
                                                     short width, bool isDst,
                                                     int lineNum) {
  VISA_VectorOpnd *cisa_opnd = nullptr;
  VISA_CALL_TO_NULLPTR(CreateVISAAddressOperand, cisa_opnd,
                       (VISA_AddrVar *)cisa_decl, offset, width, isDst);

  return (VISA_opnd *)cisa_opnd;
}

VISA_opnd *CISA_IR_Builder::CISA_set_address_expression(CISA_GEN_VAR *cisa_decl,
                                                        short offset,
                                                        int lineNum) {
  VISA_VectorOpnd *cisa_opnd = NULL;
  VISA_CALL_TO_NULLPTR(CreateVISAAddressOfOperand, cisa_opnd,
                       (VISA_GenVar *)cisa_decl, offset);
  return (VISA_opnd *)cisa_opnd;
}

VISA_opnd *CISA_IR_Builder::CISA_create_indirect(
    CISA_GEN_VAR *cisa_decl, VISA_Modifier mod, unsigned short row_offset,
    unsigned char col_offset, unsigned short immedOffset,
    unsigned short vertical_stride, unsigned short width,
    unsigned short horizontal_stride, VISA_Type type, int lineNum) {
  VISA_VectorOpnd *cisa_opnd = NULL;
  VISA_CALL_TO_NULLPTR(CreateVISAIndirectSrcOperand, cisa_opnd,
                       (VISA_AddrVar *)cisa_decl, mod, col_offset, immedOffset,
                       vertical_stride, width, horizontal_stride, type);
  return cisa_opnd;
}

VISA_opnd *CISA_IR_Builder::CISA_create_indirect_dst(
    CISA_GEN_VAR *cisa_decl, VISA_Modifier mod, unsigned short row_offset,
    unsigned char col_offset, unsigned short immedOffset,
    unsigned short horizontal_stride, VISA_Type type, int lineNum) {
  vISA_ASSERT_INPUT(cisa_decl->type == ADDRESS_VAR,
               "predication variable type is wrong"); // grammar enforced
  VISA_VectorOpnd *cisa_opnd = nullptr;
  VISA_CALL_TO_NULLPTR(CreateVISAIndirectDstOperand, cisa_opnd,
                       (VISA_AddrVar *)cisa_decl, col_offset, immedOffset,
                       horizontal_stride, type);
  return (VISA_opnd *)cisa_opnd;
}

VISA_opnd *CISA_IR_Builder::CISA_create_state_operand(const char *var_name,
                                                      unsigned char offset,
                                                      int lineNum, bool isDst) {
  CISA_GEN_VAR *decl = m_kernel->getDeclFromName(var_name);
  if (decl == nullptr) {
    RecordParseError(lineNum, var_name, ": undefined state operand");
    return nullptr;
  }

  VISA_VectorOpnd *cisa_opnd = nullptr;
  int status = VISA_SUCCESS;
  switch (decl->type) {
  case SURFACE_VAR:
    status = m_kernel->CreateVISAStateOperand(
        cisa_opnd, (VISA_SurfaceVar *)decl, offset, isDst);
    break;
  case SAMPLER_VAR:
    status = m_kernel->CreateVISAStateOperand(
        cisa_opnd, (VISA_SamplerVar *)decl, offset, isDst);
    break;
  default:
    RecordParseError(lineNum, var_name,
                     ": invalid variable type for state operand");
    break;
  }

  if (status != VISA_SUCCESS) {
    RecordParseError(lineNum, "unknown error creating state operand");
  }

  return (VISA_opnd *)cisa_opnd;
}

VISA_opnd *CISA_IR_Builder::CISA_create_predicate_operand(
    CISA_GEN_VAR *decl, VISA_PREDICATE_STATE state,
    VISA_PREDICATE_CONTROL control, int lineNum) {
  vISA_ASSERT_INPUT(decl->type == PREDICATE_VAR,
                "predication variable type is wrong, lineNum %d", lineNum); // parser enforces type
  VISA_PredOpnd *cisa_opnd = nullptr;
  int status = m_kernel->CreateVISAPredicateOperand(
      cisa_opnd, (VISA_PredVar *)decl, state, control);
  vISA_ASSERT_INPUT((status == VISA_SUCCESS),
                "Failed to create predicate operand lineNum: %d.", lineNum);
  if (status != VISA_SUCCESS) {
    RecordParseError(lineNum, "unknown error creating predicate operand");
  }
  return (VISA_opnd *)cisa_opnd;
}

VISA_opnd *CISA_IR_Builder::CISA_create_RAW_NULL_operand(int lineNum) {
  VISA_RawOpnd *cisa_opnd = nullptr;
  int status = m_kernel->CreateVISANullRawOperand(cisa_opnd, true);
  vISA_ASSERT(status == VISA_SUCCESS,
                "Was not able to create NULL RAW operand, line number: %d.", lineNum);
  if (status != VISA_SUCCESS) {
    RecordParseError(lineNum, "unknown error creating raw null operand");
  }
  return (VISA_opnd *)cisa_opnd;
}

VISA_opnd *CISA_IR_Builder::CISA_create_RAW_operand(const char *var_name,
                                                    unsigned short offset,
                                                    int lineNum) {
  VISA_RawOpnd *cisa_opnd = NULL;
  auto *decl = (VISA_GenVar *)m_kernel->getDeclFromName(var_name);
  if (decl == nullptr) {
    RecordParseError(lineNum, var_name, ": undefined raw operand variable");
    return nullptr;
  }
  int status = m_kernel->CreateVISARawOperand(cisa_opnd, decl, offset);
  if (status != VISA_SUCCESS) {
    RecordParseError(lineNum, "unknown error creating raw operand");
  }
  return (VISA_opnd *)
      cisa_opnd; // delay the decision of src or dst until translate stage
}

void CISA_IR_Builder::CISA_push_decl_scope() {
  m_kernel->pushIndexMapScopeLevel();
}
void CISA_IR_Builder::CISA_pop_decl_scope() {
  m_kernel->popIndexMapScopeLevel();
}

bool CISA_IR_Builder::addAllVarAttributes(CISA_GEN_VAR *GenVar,
                                          std::vector<attr_gen_struct *> &Attrs,
                                          int lineNum) {
  if (Attrs.size() > 0) {
    (void)m_kernel->resizeAttribute(GenVar, (uint32_t)Attrs.size());
  }

  for (int i = 0, e = (int)Attrs.size(); i < e; ++i) {
    attr_gen_struct *pAttr = Attrs[i];
    Attributes::ID aID = Attributes::getAttributeID(pAttr->name);
    if (Attributes::isBool(aID)) {
      m_kernel->AddAttributeToVarGeneric(GenVar, pAttr->name, 0, nullptr);
    } else if (Attributes::isInt32(aID)) {
      m_kernel->AddAttributeToVarGeneric(GenVar, pAttr->name, 4, &pAttr->value);
    } else if (Attributes::isCStr(aID)) {
      unsigned int sz = (unsigned)std::string_view(pAttr->string_val).size();
      m_kernel->AddAttributeToVarGeneric(GenVar, pAttr->name, sz,
                                         &pAttr->string_val);
    } else {
      RecordParseError(lineNum, pAttr->name, ": unknown attribute");
      return false;
    }
  }
  return true;
}

Common_ISA_Input_Class
CISA_IR_Builder::get_input_class(Common_ISA_Var_Class var_class) {
  if (var_class == GENERAL_VAR)
    return INPUT_GENERAL;

  if (var_class == SAMPLER_VAR)
    return INPUT_SAMPLER;

  if (var_class == SURFACE_VAR)
    return INPUT_SURFACE;

  return INPUT_UNKNOWN;
}
void CISA_IR_Builder::CISA_post_file_parse() { return; }

void CISA_IR_Builder::CISA_parse_build_options(const char* argStr) {
  std::string args(argStr);
  size_t first_quote = args.find_first_of('"');
  size_t last_quote = args.find_last_of('"');

  if (first_quote != std::string::npos &&
      last_quote != std::string::npos &&
      first_quote < last_quote) {
    args = args.substr(first_quote + 1, last_quote - first_quote - 1);
    std::vector<std::string> argvStrings;
    std::vector<const char*> argv;
    std::string token;
    std::istringstream ss(args);
    while (ss >> token) {
      argvStrings.push_back(token);
    }
    argv.reserve(argvStrings.size());
    for (const std::string& s : argvStrings) {
        argv.push_back(s.c_str());
    }
    getOptions()->parseOptions(argv.size(), argv.data());
  }
}

// place it here so that internal Gen_IR files don't have to include
// VISAKernel.h
std::stringstream &IR_Builder::criticalMsgStream() {
  return const_cast<CISA_IR_Builder *>(parentBuilder)->criticalMsgStream();
}

bool CISA_IR_Builder::CISA_create_dpas_instruction(
    ISA_Opcode opcode, VISA_EMask_Ctrl emask, unsigned exec_size,
    VISA_opnd *dst_cisa, VISA_opnd *src0_cisa, VISA_opnd *src1_cisa,
    VISA_opnd *src2_cisa, GenPrecision A, GenPrecision W, uint8_t D, uint8_t C,
    int lineNum) {
  // rcount !
  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  VISA_CALL_TO_BOOL(AppendVISADpasInst, opcode, emask, executionSize,
                    (VISA_RawOpnd *)dst_cisa, (VISA_RawOpnd *)src0_cisa,
                    (VISA_RawOpnd *)src1_cisa, (VISA_VectorOpnd *)src2_cisa, A,
                    W, D, C);
  return true;
}


bool CISA_IR_Builder::CISA_create_bfn_instruction(
    VISA_opnd *pred, uint8_t func_ctrl, bool sat, VISA_EMask_Ctrl emask,
    unsigned exec_size, VISA_opnd *dst_cisa, VISA_opnd *src0_cisa,
    VISA_opnd *src1_cisa, VISA_opnd *src2_cisa, int lineNum) {
  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  VISA_CALL_TO_BOOL(AppendVISABfnInst, func_ctrl, (VISA_PredOpnd *)pred, sat,
                    emask, executionSize, (VISA_VectorOpnd *)dst_cisa,
                    (VISA_VectorOpnd *)src0_cisa, (VISA_VectorOpnd *)src1_cisa,
                    (VISA_VectorOpnd *)src2_cisa);
  return true;
}

bool CISA_IR_Builder::CISA_create_qword_scatter_instruction(
    ISA_Opcode opcode, VISA_opnd *pred, VISA_EMask_Ctrl eMask,
    unsigned execSize, unsigned numBlocks, const char *surfaceName,
    VISA_opnd *offsets, VISA_opnd *dstSrc, int lineNum) {
  VISA_StateOpndHandle *surface =
      CISA_get_surface_variable(surfaceName, lineNum);
  if (!surface)
    return false; // error recorded

  if (opcode == ISA_QW_GATHER) {
    VISA_CALL_TO_BOOL(AppendVISAQwordGatherInst,
                      static_cast<VISA_PredOpnd *>(pred), eMask,
                      Get_VISA_Exec_Size_From_Raw_Size(execSize),
                      valueToVISASVMBlockNum(numBlocks), surface,
                      static_cast<VISA_RawOpnd *>(offsets),
                      static_cast<VISA_RawOpnd *>(dstSrc));
  } else {
    VISA_CALL_TO_BOOL(AppendVISAQwordScatterInst,
                      static_cast<VISA_PredOpnd *>(pred), eMask,
                      Get_VISA_Exec_Size_From_Raw_Size(execSize),
                      valueToVISASVMBlockNum(numBlocks), surface,
                      static_cast<VISA_RawOpnd *>(offsets),
                      static_cast<VISA_RawOpnd *>(dstSrc));
  }
  return true;
}

bool CISA_IR_Builder::CISA_create_fcvt_instruction(
    bool sat, VISA_EMask_Ctrl emask, unsigned exec_size, VISA_opnd *dst,
    VISA_opnd *src0, int lineNum) {
  VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
  VISA_CALL_TO_BOOL(AppendVISADataMovementInst, ISA_FCVT, nullptr, sat, emask,
                    executionSize, (VISA_VectorOpnd *)dst,
                    (VISA_VectorOpnd *)src0);
  return true;
}

bool CISA_IR_Builder::CISA_create_lsc_untyped_inst(
    VISA_opnd *pred, LSC_OP opcode, LSC_SFID sfid, LSC_CACHE_OPTS caching, bool ov,
    VISA_Exec_Size execSize, VISA_EMask_Ctrl emask, LSC_ADDR addr,
    LSC_DATA_SHAPE dataShape,
    VISA_opnd *surface, unsigned surfaceIndex,
    VISA_opnd *dstData, VISA_opnd *src0Addr, VISA_opnd *src1Data,
    VISA_opnd *src2Data, int lineNum) {
  VISA_CALL_TO_BOOL(AppendVISALscUntypedInst, opcode, sfid,
                    static_cast<VISA_PredOpnd *>(pred), execSize, emask,
                    caching, ov, addr, dataShape,
                    static_cast<VISA_VectorOpnd *>(surface), surfaceIndex,
                    static_cast<VISA_RawOpnd *>(dstData),
                    static_cast<VISA_RawOpnd *>(src0Addr),
                    static_cast<VISA_RawOpnd *>(src1Data),
                    static_cast<VISA_RawOpnd *>(src2Data));
  return true;
}
bool CISA_IR_Builder::CISA_create_lsc_untyped_strided_inst(
    VISA_opnd *pred, LSC_OP opcode, LSC_SFID sfid, LSC_CACHE_OPTS caching,
    VISA_Exec_Size execSize, VISA_EMask_Ctrl emask, LSC_ADDR addr,
    LSC_DATA_SHAPE dataShape,
    VISA_opnd *surface, unsigned surfaceIndex,
    VISA_opnd *dst,
    VISA_opnd *src0Base, VISA_opnd *src0Stride, VISA_opnd *src1Data,
    int lineNum) {
  VISA_CALL_TO_BOOL(
      AppendVISALscUntypedStridedInst, opcode, sfid,
      static_cast<VISA_PredOpnd *>(pred), execSize, emask, caching, addr,
      dataShape,
      static_cast<VISA_VectorOpnd *>(surface), surfaceIndex,
      static_cast<VISA_RawOpnd *>(dst), static_cast<VISA_RawOpnd *>(src0Base),
      static_cast<VISA_VectorOpnd *>(src0Stride),
      static_cast<VISA_RawOpnd *>(src1Data));
  return true;
}

bool CISA_IR_Builder::CISA_create_lsc_untyped_block2d_inst(
    VISA_opnd *pred, LSC_OP opcode, LSC_SFID sfid, LSC_CACHE_OPTS caching,
    VISA_Exec_Size execSize, VISA_EMask_Ctrl emask,
    LSC_DATA_SHAPE_BLOCK2D dataShape2d, VISA_opnd *dstData,
    VISA_opnd *src0AddrsOps[LSC_BLOCK2D_ADDR_PARAMS], VISA_opnd *src1Data,
    int xImmOffset, int yImmOffset, int lineNum) {
  if (src0AddrsOps[1] == nullptr) {
    // using the optimized block2d instruction
    VISA_CALL_TO_BOOL(AppendVISALscUntypedBlock2DInst, opcode,
        static_cast<VISA_PredOpnd*>(pred), execSize, emask,
        caching, dataShape2d, static_cast<VISA_RawOpnd*>(dstData),
        static_cast<VISA_VectorOpnd*>(src0AddrsOps[0]),
        xImmOffset, yImmOffset, static_cast<VISA_RawOpnd*>(src1Data));
  } else {
    VISA_VectorOpnd *src0Addrs[LSC_BLOCK2D_ADDR_PARAMS]{
      static_cast<VISA_VectorOpnd *>(src0AddrsOps[0]),
      static_cast<VISA_VectorOpnd *>(src0AddrsOps[1]),
      static_cast<VISA_VectorOpnd *>(src0AddrsOps[2]),
      static_cast<VISA_VectorOpnd *>(src0AddrsOps[3]),
      static_cast<VISA_VectorOpnd *>(src0AddrsOps[4]),
      static_cast<VISA_VectorOpnd *>(src0AddrsOps[5]),
    };
    VISA_CALL_TO_BOOL(AppendVISALscUntypedBlock2DInst, opcode, sfid,
                    static_cast<VISA_PredOpnd *>(pred), execSize, emask,
                    caching, dataShape2d, static_cast<VISA_RawOpnd *>(dstData),
                    src0Addrs, xImmOffset, yImmOffset,
                    static_cast<VISA_RawOpnd *>(src1Data));
  }
  return true;
}

bool CISA_IR_Builder::CISA_create_lsc_typed_inst(
    VISA_opnd *pred, LSC_OP opcode, LSC_SFID sfid, LSC_CACHE_OPTS caching,
    VISA_Exec_Size execSize, VISA_EMask_Ctrl emask, LSC_ADDR_TYPE addrModel,
    LSC_ADDR_SIZE addrSize, LSC_DATA_SHAPE dataShape,
    VISA_opnd *surface, unsigned surfaceIndex,
    VISA_opnd *dst_data,
    VISA_opnd *coord0s, int coord0Offset,
    VISA_opnd *coord1s, int coord1Offset,
    VISA_opnd *coord2s, int coord2Offset,
    VISA_opnd *features,
    VISA_opnd *src1_data, VISA_opnd *src2_data,
    int lineNum) {
  VISA_CALL_TO_BOOL(AppendVISALscTypedInst, opcode,
                    static_cast<VISA_PredOpnd *>(pred), execSize, emask,
                    caching, addrModel, addrSize, dataShape,
                    static_cast<VISA_VectorOpnd *>(surface), surfaceIndex,
                    static_cast<VISA_RawOpnd *>(dst_data),
                    static_cast<VISA_RawOpnd *>(coord0s), coord0Offset,
                    static_cast<VISA_RawOpnd *>(coord1s), coord1Offset,
                    static_cast<VISA_RawOpnd *>(coord2s), coord2Offset,
                    static_cast<VISA_RawOpnd *>(features),
                    static_cast<VISA_RawOpnd *>(src1_data),
                    static_cast<VISA_RawOpnd *>(src2_data));
  return true;
}

LSC_CACHE_OPTS CISA_IR_Builder::CISA_create_caching_opts(int lineNum) {
  return LSC_CACHE_OPTS(LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT);
}

LSC_CACHE_OPTS CISA_IR_Builder::CISA_create_caching_opts(LSC_CACHE_OPT l1,
                                                         LSC_CACHE_OPT l3,
                                                         int lineNum) {
  return LSC_CACHE_OPTS(l1, l3);
}

bool CISA_IR_Builder::CISA_create_lsc_fence(LSC_SFID sfid, LSC_FENCE_OP fence,
                                            LSC_SCOPE scope, int lineNum) {
  VISA_CALL_TO_BOOL(AppendVISALscFence, sfid, fence, scope);

  return true;
}

bool CISA_IR_Builder::CISA_create_nbarrier(bool isWait,
                                           VISA_opnd *barrierId,
                                           VISA_opnd *threadCount,
                                           int lineNum) {
  if (isWait) {
    // wait
    VISA_CALL_TO_BOOL(AppendVISANamedBarrierWait,
                      static_cast<VISA_VectorOpnd *>(barrierId));
  } else {
    // signal
    VISA_CALL_TO_BOOL(AppendVISANamedBarrierSignal,
                      static_cast<VISA_VectorOpnd *>(barrierId),
                      static_cast<VISA_VectorOpnd *>(threadCount));
  }
  return true;
}

bool CISA_IR_Builder::CISA_create_nbarrier_signal(VISA_opnd *barrierId,
                                                  VISA_opnd *barrierType,
                                                  VISA_opnd *numProds,
                                                  VISA_opnd *numCons,
                                                  int lineNum) {
  VISA_CALL_TO_BOOL(AppendVISANamedBarrierSignal,
                    static_cast<VISA_VectorOpnd *>(barrierId),
                    static_cast<VISA_VectorOpnd *>(barrierType),
                    static_cast<VISA_VectorOpnd *>(numProds),
                    static_cast<VISA_VectorOpnd *>(numCons));
  return true;
}


bool CISA_IR_Builder::CISA_create_lsc_typed_block2d_inst(
    LSC_OP opcode, LSC_CACHE_OPTS caching, LSC_ADDR_TYPE addrModel,
    LSC_DATA_SHAPE_TYPED_BLOCK2D dataShape2d,
    VISA_opnd *surface, unsigned surfaceIndex,
    VISA_opnd *dstData, VISA_opnd *xOffset, VISA_opnd *yOffset,
    int xImmOffset, int yImmOffset, VISA_opnd *src1Data, int lineNum) {
  if (surfaceIndex != 0)
    RecordParseError(lineNum,
                     "non-zero SS_IDX not yet supported on this message");
  VISA_CALL_TO_BOOL(AppendVISALscTypedBlock2DInst, opcode, caching, addrModel,
                    dataShape2d,
                    static_cast<VISA_VectorOpnd *>(surface),
                    surfaceIndex,
                    static_cast<VISA_RawOpnd *>(dstData),
                    static_cast<VISA_VectorOpnd *>(xOffset),
                    static_cast<VISA_VectorOpnd *>(yOffset),
                    xImmOffset, yImmOffset,
                    static_cast<VISA_RawOpnd *>(src1Data));
  return true;
}

bool CISA_IR_Builder::CISA_create_lsc_untyped_append_counter_atomic_inst(
    LSC_OP opcode, VISA_opnd *pred, VISA_Exec_Size execSize,
    VISA_EMask_Ctrl emask,
    LSC_CACHE_OPTS caching, LSC_ADDR_TYPE addr, LSC_DATA_SHAPE dataShape,
    VISA_opnd *surface, unsigned surfaceIndex,
    VISA_opnd *dst, VISA_opnd *srcData, int lineNum) {
  VISA_CALL_TO_BOOL(
      AppendVISALscUntypedAppendCounterAtomicInst, opcode,
      static_cast<VISA_PredOpnd *>(pred), execSize, emask,
      caching, addr, dataShape,
      static_cast<VISA_VectorOpnd *>(surface), surfaceIndex,
      static_cast<VISA_RawOpnd *>(dst), static_cast<VISA_RawOpnd *>(srcData));
  return true;
}

const VISAKernelImpl *
CISA_IR_Builder::getKernel(const std::string &name) const {
  auto it = m_nameToKernel.find(name);
  if (it == m_nameToKernel.end())
    return nullptr;
  return it->second;
}

VISAKernelImpl *CISA_IR_Builder::getKernel(const std::string &name) {
  return const_cast<VISAKernelImpl *>(
      const_cast<const CISA_IR_Builder *>(this)->getKernel(name));
}
