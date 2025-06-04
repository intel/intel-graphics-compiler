/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "BIFFlagCtrlResolution.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-bif-flag-control-resolution"
#define PASS_DESCRIPTION "Resolves bif flag controls"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(BIFFlagCtrlResolution, PASS_FLAG, PASS_DESCRIPTION,
                          PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(BIFFlagCtrlResolution, PASS_FLAG, PASS_DESCRIPTION,
                        PASS_CFG_ONLY, PASS_ANALYSIS)

char BIFFlagCtrlResolution::ID = 0;

void BIFFlagCtrlResolution::FillFlagCtrl() {
  // Here we can add more flags
  // Provide in first argument BiF flag control defined in
  // IGC/BiFModule/Headers/bif_flag_controls.h
  // In second place provide the value which should be
  // inserted for this flag. If needed feed class BIFFlagCtrlResolution
  // with new value from outside.
  BIF_FLAG_CTRL_SET(PlatformType, PtrCGC->platform.GetProductFamily());
  BIF_FLAG_CTRL_SET(RenderFamily, PtrCGC->platform.getPlatformInfo().eRenderCoreFamily);
  BIF_FLAG_CTRL_SET(
      FlushDenormals,
      ((PtrCGC->getModuleMetaData()->compOpt.FloatDenormMode32 == FLOAT_DENORM_FLUSH_TO_ZERO) ||
       PtrCGC->getModuleMetaData()->compOpt.DenormsAreZero));
  BIF_FLAG_CTRL_SET(FastRelaxedMath,
                    PtrCGC->getModuleMetaData()->compOpt.RelaxedBuiltins);
  BIF_FLAG_CTRL_SET(DashGSpecified,
                    PtrCGC->getModuleMetaData()->compOpt.DashGSpecified);
  BIF_FLAG_CTRL_SET(MadEnable, PtrCGC->getModuleMetaData()->compOpt.MadEnable);
  BIF_FLAG_CTRL_SET(OptDisable,
                    PtrCGC->getModuleMetaData()->compOpt.OptDisable);
  BIF_FLAG_CTRL_SET(UseMathWithLUT, IGC_IS_FLAG_ENABLED(UseMathWithLUT));
  BIF_FLAG_CTRL_SET(UseNativeFP32GlobalAtomicAdd,
                    PtrCGC->platform.hasFP32GlobalAtomicAdd());
  BIF_FLAG_CTRL_SET(UseNativeFP16AtomicMinMax,
                    PtrCGC->platform.hasFP16AtomicMinMax());
  BIF_FLAG_CTRL_SET(HasInt64SLMAtomicCAS,
                    PtrCGC->platform.hasInt64SLMAtomicCAS());
  BIF_FLAG_CTRL_SET(UseNativeFP64GlobalAtomicAdd,
                    PtrCGC->platform.hasFP64GlobalAtomicAdd());
  BIF_FLAG_CTRL_SET(UseNative64BitIntBuiltin,
                    !PtrCGC->platform.hasNoFullI64Support());
  BIF_FLAG_CTRL_SET(HasThreadPauseSupport,
                    PtrCGC->platform.hasThreadPauseSupport());
  BIF_FLAG_CTRL_SET(UseNative64BitFloatBuiltin,
                    !PtrCGC->platform.hasNoFP64Inst());
  BIF_FLAG_CTRL_SET(UseBfn, IGC_IS_FLAG_ENABLED(EnableBfn) &&
      PtrCGC->platform.supportBfnInstruction());
  BIF_FLAG_CTRL_SET(hasHWLocalThreadID, PtrCGC->platform.hasHWLocalThreadID());
  BIF_FLAG_CTRL_SET(CRMacros, PtrCGC->platform.hasCorrectlyRoundedMacros());
  BIF_FLAG_CTRL_SET(
      APIRS, !(StringRef(PtrCGC->getModule()->getTargetTriple()).size() > 0));

  if (PtrCGC->type == ShaderType::OPENCL_SHADER) {
    BIF_FLAG_CTRL_SET(IsSPIRV,
                      static_cast<OpenCLProgramContext *>(PtrCGC)->isSPIRV());
    BIF_FLAG_CTRL_SET(ProfilingTimerResolution,
                      static_cast<OpenCLProgramContext *>(PtrCGC)
                          ->getProfilingTimerResolution());
    BIF_FLAG_CTRL_SET(UseLSC, PtrCGC->platform.hasLSC());
    BIF_FLAG_CTRL_SET(ForceL1Prefetch,
                      IGC_IS_FLAG_ENABLED(ForcePrefetchToL1Cache));
    BIF_FLAG_CTRL_SET(UseHighAccuracyMath,
                      static_cast<OpenCLProgramContext *>(PtrCGC)
                          ->m_InternalOptions.UseHighAccuracyMathFuncs);
  } else {
    BIF_FLAG_CTRL_SET(IsSPIRV, false);
    BIF_FLAG_CTRL_SET(ProfilingTimerResolution, 0.0f);
    BIF_FLAG_CTRL_SET(UseLSC, false);
    BIF_FLAG_CTRL_SET(ForceL1Prefetch, false);
    BIF_FLAG_CTRL_SET(UseHighAccuracyMath, false);
  }

  // Stateless to stateful optimization checks if the offset of GEP instruction is positive.
  // This is done with the assumption that the sign bit of global_id will be always off.
  // This assume interferes with instcombine pass, so it is skipped unless really needed, that is:
  //   1) StatelessToStateful pass is enabled, AND
  //   2) Buffers don't use implicit bufferOffsetArg.
  BIF_FLAG_CTRL_SET(UseAssumeInGetGlobalId,
      PtrCGC->m_DriverInfo.SupportsStatelessToStatefulBufferTransformation() &&
      !PtrCGC->getModuleMetaData()->compOpt.GreaterThan4GBBufferRequired &&
      IGC_IS_FLAG_ENABLED(EnableStatelessToStateful) &&
      !((IGC_IS_FLAG_ENABLED(EnableSupportBufferOffset) || PtrCGC->getModuleMetaData()->compOpt.HasBufferOffsetArg)) &&
      !PtrCGC->getModuleMetaData()->compOpt.OptDisable);

  BIF_FLAG_CTRL_SET(EnableSWSrgbWrites,
                    IGC_GET_FLAG_VALUE(cl_khr_srgb_image_writes));
  BIF_FLAG_CTRL_SET(MaxHWThreadIDPerSubDevice,
                    PtrCGC->platform.GetGTSystemInfo().ThreadCount);

  // Set JointMatrixLoadStoreOpt to 2 for systems without 2D block load/store support (default is 3).
  if(!PtrCGC->platform.hasExecSize16DPAS() && IGC_GET_FLAG_VALUE(JointMatrixLoadStoreOpt) == 3) {
    BIF_FLAG_CTRL_SET(JointMatrixLoadStoreOpt, 2);
  } else {
    BIF_FLAG_CTRL_SET(JointMatrixLoadStoreOpt, IGC_GET_FLAG_VALUE(JointMatrixLoadStoreOpt));
  }

  BIF_FLAG_CTRL_SET(UseOOBChecks, PtrCGC->platform.needsOutOfBoundsBuiltinChecks());

  // NOTE: No need to check for UseLegacyBindlessMode,
  //       as it's unrelated to images.
  BIF_FLAG_CTRL_SET(UseBindlessImage, PtrCGC->getModuleMetaData()->UseBindlessImage);
}

#undef BIF_FLAG_CTRL_SET

bool BIFFlagCtrlResolution::runOnModule(Module &M) {
  if (PtrCGC == nullptr) {
    IGC_ASSERT_EXIT_MESSAGE(0, "[BIF_IGC] Not propper initialized BIFFlagCtrlResolution pass");
  }

  pModule = &M;
  FillFlagCtrl();

  std::vector<StringRef> listOfFlagNames{
#include "IGC/BiFModule/Headers/bif_flag_controls.h"
  };

  bool wasModuleUpdated = false;

  for (size_t i = 0; i < listOfFlagNames.size(); ++i) {
    StringRef &bif_flag = listOfFlagNames[i];

    auto iter = ListDelegates.find(bif_flag);

    if (iter != ListDelegates.end()) {
      // Here we are replacing the Bif flag control if present,
      // it's executing the delegate function
      wasModuleUpdated |= iter->second();
    } else {
      IGC_ASSERT_EXIT_MESSAGE(0, "[BIF_IGC] Missing setup for flag %s in FillFlagCtrl function", bif_flag.str().c_str());
    }
  }

  return wasModuleUpdated;
}

#undef BIF_FLAG_CONTROL

template <typename T>
bool BIFFlagCtrlResolution::replace(T Value, llvm::GlobalVariable *GV) {
  bool Changed = false;
  if (!GV)
    return Changed;

  if (std::is_enum<T>() || std::is_integral<T>()) {
    GV->setInitializer(llvm::ConstantInt::getIntegerValue(
        GV->getValueType(),
        APInt((unsigned)GV->getValueType()->getPrimitiveSizeInBits(),
              (unsigned int)Value, std::is_signed<T>())));
    Changed = true;
  } else if (std::is_floating_point<T>()) {
    GV->setInitializer(
        llvm::ConstantFP::get(GV->getValueType(), (double)Value));
    Changed = true;
  } else {
    IGC_ASSERT_EXIT_MESSAGE(0, "[BIF_IGC] Not supported BiF flag control type");
  }

  return Changed;
}

BIFFlagCtrlResolution::BIFFlagCtrlResolution() : ModulePass(ID) {
  PtrCGC = nullptr;
}

BIFFlagCtrlResolution::BIFFlagCtrlResolution(CodeGenContext *PtrCGC)
    : ModulePass(ID) {
  this->PtrCGC = PtrCGC;
}

BIFFlagCtrlResolution::~BIFFlagCtrlResolution(void) {}
