/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/ImageFuncs/ImageFuncsAnalysis.hpp"
#include "Compiler/Optimizer/OCLBIUtils.h"
#include "Compiler/IGCPassSupport.h"
#include <set>
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-image-func-analysis"
#define PASS_DESCRIPTION "Analyzes image height, width, depth functions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ImageFuncsAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(ImageFuncsAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ImageFuncsAnalysis::ID = 0;

ImageFuncsAnalysis::ImageFuncsAnalysis() : ModulePass(ID) {
  initializeImageFuncsAnalysisPass(*PassRegistry::getPassRegistry());
}

// All image functions needed resolved by implicit arguments
const llvm::StringRef ImageFuncsAnalysis::GET_IMAGE_HEIGHT = "__builtin_IB_get_image_height";
const llvm::StringRef ImageFuncsAnalysis::GET_IMAGE_WIDTH = "__builtin_IB_get_image_width";
const llvm::StringRef ImageFuncsAnalysis::GET_IMAGE_DEPTH = "__builtin_IB_get_image_depth";
const llvm::StringRef ImageFuncsAnalysis::GET_IMAGE_NUM_MIP_LEVELS = "__builtin_IB_get_image_num_mip_levels";
const llvm::StringRef ImageFuncsAnalysis::GET_IMAGE_CHANNEL_DATA_TYPE = "__builtin_IB_get_image_channel_data_type";
const llvm::StringRef ImageFuncsAnalysis::GET_IMAGE_CHANNEL_ORDER = "__builtin_IB_get_image_channel_order";
const llvm::StringRef ImageFuncsAnalysis::GET_IMAGE_SRGB_CHANNEL_ORDER = "__builtin_IB_get_image_srgb_channel_order";
const llvm::StringRef ImageFuncsAnalysis::GET_IMAGE1D_ARRAY_SIZE = "__builtin_IB_get_image1d_array_size";
const llvm::StringRef ImageFuncsAnalysis::GET_IMAGE2D_ARRAY_SIZE = "__builtin_IB_get_image2d_array_size";
const llvm::StringRef ImageFuncsAnalysis::GET_IMAGE_NUM_SAMPLES = "__builtin_IB_get_image_num_samples";
const llvm::StringRef ImageFuncsAnalysis::GET_SAMPLER_ADDRESS_MODE = "__builtin_IB_get_address_mode";
const llvm::StringRef ImageFuncsAnalysis::GET_SAMPLER_NORMALIZED_COORDS = "__builtin_IB_is_normalized_coords";
const llvm::StringRef ImageFuncsAnalysis::GET_SAMPLER_SNAP_WA_REQUIRED = "__builtin_IB_get_snap_wa_reqd";

bool ImageFuncsAnalysis::runOnModule(Module &M) {
  bool changed = false;
  m_pMDUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  CodeGenContext *ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  m_useAdvancedBindlessMode = ctx->getModuleMetaData()->UseBindlessImage;

  m_useSPVINTELBindlessImages = ctx->getModuleMetaData()->extensions.spvINTELBindlessImages;

  // Run on all functions defined in this module
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
    Function *pFunc = &(*I);
    if (pFunc->isDeclaration())
      continue;
    if (runOnFunction(*pFunc)) {
      changed = true;
    }
  }

  // Update LLVM metadata based on IGC MetadataUtils
  if (changed)
    m_pMDUtils->save(M.getContext());

  return changed;
}

bool ImageFuncsAnalysis::runOnFunction(Function &F) {

  // Visit the function
  visit(F);

  ImplicitArgs::addImageArgs(F, m_argMap, m_pMDUtils);

  m_argMap.clear();

  return true;
}

void ImageFuncsAnalysis::visitCallInst(CallInst &CI) {
  if (!CI.getCalledFunction()) {
    return;
  }

  StringRef funcName = CI.getCalledFunction()->getName();

  // Check for OpenCL image dimension function calls
  std::set<int> *imageFunc = nullptr;

  if (funcName == GET_IMAGE_HEIGHT && !m_useAdvancedBindlessMode) {
    imageFunc = &m_argMap[ImplicitArg::IMAGE_HEIGHT];
  } else if (funcName == GET_IMAGE_WIDTH && !m_useAdvancedBindlessMode) {
    imageFunc = &m_argMap[ImplicitArg::IMAGE_WIDTH];
  } else if (funcName == GET_IMAGE_DEPTH && !m_useAdvancedBindlessMode) {
    imageFunc = &m_argMap[ImplicitArg::IMAGE_DEPTH];
  } else if (funcName == GET_IMAGE_NUM_MIP_LEVELS) {
    imageFunc = &m_argMap[ImplicitArg::IMAGE_NUM_MIP_LEVELS];
  } else if (funcName == GET_IMAGE_CHANNEL_DATA_TYPE) {
    imageFunc = &m_argMap[ImplicitArg::IMAGE_CHANNEL_DATA_TYPE];
  } else if (funcName == GET_IMAGE_CHANNEL_ORDER) {
    imageFunc = &m_argMap[ImplicitArg::IMAGE_CHANNEL_ORDER];
  } else if (funcName == GET_IMAGE_SRGB_CHANNEL_ORDER) {
    imageFunc = &m_argMap[ImplicitArg::IMAGE_SRGB_CHANNEL_ORDER];
  } else if ((funcName == GET_IMAGE1D_ARRAY_SIZE || funcName == GET_IMAGE2D_ARRAY_SIZE) && !m_useAdvancedBindlessMode) {
    imageFunc = &m_argMap[ImplicitArg::IMAGE_ARRAY_SIZE];
  } else if (funcName == GET_IMAGE_NUM_SAMPLES) {
    imageFunc = &m_argMap[ImplicitArg::IMAGE_NUM_SAMPLES];
  } else if (funcName == GET_SAMPLER_ADDRESS_MODE) {
    imageFunc = &m_argMap[ImplicitArg::SAMPLER_ADDRESS];
  } else if (funcName == GET_SAMPLER_NORMALIZED_COORDS) {
    imageFunc = &m_argMap[ImplicitArg::SAMPLER_NORMALIZED];
  }
  // The SNAP_WA is disabled for SPV_INTEL_bindless_images extension.
  // For further information, refer to the ImageFuncResolution.cpp file.
  else if (funcName == GET_SAMPLER_SNAP_WA_REQUIRED && !m_useSPVINTELBindlessImages) {
    imageFunc = &m_argMap[ImplicitArg::SAMPLER_SNAP_WA];
  } else {
    // Non image function, do nothing
    return;
  }

  // Extract the arg num and add it to the appropriate data structure
  IGC_ASSERT_MESSAGE(IGCLLVM::getNumArgOperands(&CI) == 1,
                     "Supported image/sampler functions are expected to have only one argument");

  // We only care about image and sampler arguments here, inline samplers
  // don't require extra kernel parameters.
  ModuleMetaData *modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
  Value *callArg = ValueTracker::track(&CI, 0, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(), modMD);

  // Return false when sampler track back to a SYCL bindless image argment,
  // since in this case we don't need implicit args.
  auto isImageOrSamplerArgument = [modMD](Argument *arg) {
    FunctionMetaData funcMD = modMD->FuncMD.find(arg->getParent())->second;
    std::string typeName = funcMD.m_OpenCLArgBaseTypes[arg->getArgNo()];
    return typeName.rfind("sampler_t", 0) != std::string::npos || typeName.rfind("image", 0) != std::string::npos;
  };

  // TODO: For now assume that we may not trace a sampler/texture for indirect access.
  // In this case we provide no WA support for indirect case and all WAs will return 0.
  // These WAs need to be reworked to support indirect case in the future.
  if (callArg) {
    if (Argument *arg = dyn_cast<Argument>(callArg)) {
      if (isImageOrSamplerArgument(arg)) {
        imageFunc->insert(arg->getArgNo());
        return;
      }
    }
  }

  // Only these args should be hit by the indirect case
  IGC_ASSERT(funcName == GET_SAMPLER_ADDRESS_MODE || funcName == GET_SAMPLER_NORMALIZED_COORDS ||
             funcName == GET_SAMPLER_SNAP_WA_REQUIRED);
}
