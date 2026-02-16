/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/ImageFuncs/ImageFuncResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ImageFuncs/ImageFuncsAnalysis.hpp"
#include "Compiler/Optimizer/OCLBIUtils.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-image-func-resolution"
#define PASS_DESCRIPTION "Resolves image height, width, depth functions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ImageFuncResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(ImageFuncResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ImageFuncResolution::ID = 0;

ImageFuncResolution::ImageFuncResolution() : FunctionPass(ID), m_implicitArgs() {
  initializeImageFuncResolutionPass(*PassRegistry::getPassRegistry());
}

bool ImageFuncResolution::runOnFunction(Function &F) {
  const MetaDataUtils *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  m_implicitArgs = ImplicitArgs(F, pMdUtils);
  m_pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  m_changed = false;
  visit(F);
  return m_changed;
}

void ImageFuncResolution::visitCallInst(CallInst &CI) {
  if (!CI.getCalledFunction()) {
    return;
  }

  bool isImplicitImageArgs = !m_pCtx->getModuleMetaData()->UseBindlessImage;

  Value *imageRes = nullptr;

  // Add appropriate sequence and image dimension func
  StringRef funcName = CI.getCalledFunction()->getName();

  if (funcName.equals(ImageFuncsAnalysis::GET_IMAGE_HEIGHT)) {
    if (!isImplicitImageArgs) {
      IGC_ASSERT_MESSAGE(false, "Getting Image Height from implicit args is supported only in bindful mode");
      return;
    }
    imageRes = getImageHeight(CI);
  } else if (funcName.equals(ImageFuncsAnalysis::GET_IMAGE_WIDTH)) {
    if (!isImplicitImageArgs) {
      IGC_ASSERT_MESSAGE(false, "Getting Image Width from implicit args is supported only in bindful mode");
      return;
    }
    imageRes = getImageWidth(CI);
  } else if (funcName.equals(ImageFuncsAnalysis::GET_IMAGE_DEPTH)) {
    if (!isImplicitImageArgs) {
      IGC_ASSERT_MESSAGE(false, "Getting Image Depth from implicit args is supported only in bindful mode");
      return;
    }
    imageRes = getImageDepth(CI);
  } else if (funcName.equals(ImageFuncsAnalysis::GET_IMAGE_NUM_MIP_LEVELS)) {
    imageRes = getImageNumMipLevels(CI);
  } else if (funcName.equals(ImageFuncsAnalysis::GET_IMAGE_CHANNEL_DATA_TYPE)) {
    imageRes = getImageChannelDataType(CI);
  } else if (funcName.equals(ImageFuncsAnalysis::GET_IMAGE_CHANNEL_ORDER)) {
    imageRes = getImageChannelOrder(CI);
  } else if (funcName.equals(ImageFuncsAnalysis::GET_IMAGE1D_ARRAY_SIZE) ||
             funcName.equals(ImageFuncsAnalysis::GET_IMAGE2D_ARRAY_SIZE)) {
    if (!isImplicitImageArgs) {
      IGC_ASSERT_MESSAGE(false, "Getting Image Array Size from implicit args is supported only in bindful mode");
      return;
    }
    imageRes = getImageArraySize(CI);
  } else if (funcName.equals(ImageFuncsAnalysis::GET_IMAGE_NUM_SAMPLES)) {
    imageRes = getImageNumSamples(CI);
  } else if (funcName.equals(ImageFuncsAnalysis::GET_SAMPLER_ADDRESS_MODE)) {
    imageRes = getSamplerAddressMode(CI);
  } else if (funcName.equals(ImageFuncsAnalysis::GET_SAMPLER_NORMALIZED_COORDS)) {
    imageRes = getSamplerNormalizedCoords(CI);
  } else {
    // Non image function, do nothing
    return;
  }

  // Replace original image dim call instruction by the result of the appropriate sequence
  CI.replaceAllUsesWith(imageRes);
  CI.eraseFromParent();
  m_changed = true;
}

Value *ImageFuncResolution::getImageHeight(CallInst &CI) {
  Argument *arg = getImplicitImageArg(CI, ImplicitArg::IMAGE_HEIGHT);
  return arg;
}

Value *ImageFuncResolution::getImageWidth(CallInst &CI) {
  Argument *arg = getImplicitImageArg(CI, ImplicitArg::IMAGE_WIDTH);
  return arg;
}

Value *ImageFuncResolution::getImageDepth(CallInst &CI) {
  Argument *arg = getImplicitImageArg(CI, ImplicitArg::IMAGE_DEPTH);
  return arg;
}

Value *ImageFuncResolution::getImageNumMipLevels(CallInst &CI) {
  Argument *arg = getImplicitImageArg(CI, ImplicitArg::IMAGE_NUM_MIP_LEVELS);
  return arg;
}

Value *ImageFuncResolution::getImageChannelDataType(CallInst &CI) {
  Argument *arg = getImplicitImageArg(CI, ImplicitArg::IMAGE_CHANNEL_DATA_TYPE);
  return arg;
}

Value *ImageFuncResolution::getImageChannelOrder(CallInst &CI) {
  Argument *arg = getImplicitImageArg(CI, ImplicitArg::IMAGE_CHANNEL_ORDER);
  return arg;
}

Value *ImageFuncResolution::getImageArraySize(CallInst &CI) {
  Argument *arg = getImplicitImageArg(CI, ImplicitArg::IMAGE_ARRAY_SIZE);
  return arg;
}

Value *ImageFuncResolution::getImageNumSamples(CallInst &CI) {
  Argument *arg = getImplicitImageArg(CI, ImplicitArg::IMAGE_NUM_SAMPLES);
  return arg;
}

template <ImplicitArg::ArgType ArgTy> Value *ImageFuncResolution::getSamplerProperty(CallInst &CI) {
  MetaDataUtils *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  ModuleMetaData *modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

  if (Value *sampler = ValueTracker::track(&CI, 0, pMdUtils, modMD)) {
    auto *arg = dyn_cast<Argument>(sampler);
    bool isImplicitInlineSamplerArg = arg ? m_implicitArgs.isImplicitArg(arg) : false;
    if (arg && !isImplicitInlineSamplerArg) {
      if (m_implicitArgs.isImplicitArgExist(ArgTy)) {
        Argument *arg = getImplicitImageArg(CI, ArgTy);
        return arg;
      }
    } else {
      llvm::Function *pFunc = CI.getFunction();

      uint64_t samplerVal = 0;
      if (modMD->FuncMD.find(pFunc) != modMD->FuncMD.end()) {
        FunctionMetaData funcMD = modMD->FuncMD[pFunc];
        ResourceAllocMD resAllocMD = funcMD.resAllocMD;
        unsigned samplerValue;
        if (isImplicitInlineSamplerArg) {
          // Inline sampler value is stored as explicit argument number in ImageFuncsAnalysis pass.
          samplerValue = m_implicitArgs.getExplicitArgNumForArg(arg);
        } else {
          IGC_ASSERT_MESSAGE(isa<ConstantInt>(sampler), "Sampler must be a constant integer");
          samplerValue = int_cast<unsigned int>(cast<ConstantInt>(sampler)->getZExtValue());
        }
        for (auto i = resAllocMD.inlineSamplersMD.begin(), e = resAllocMD.inlineSamplersMD.end(); i != e; i++) {
          IGC::InlineSamplersMD inlineSamplerMD = *i;
          if (samplerValue == inlineSamplerMD.m_Value) {
            InlineSamplerState samplerState{static_cast<uint64_t>(samplerValue)};
            if constexpr (ArgTy == ImplicitArg::SAMPLER_ADDRESS) {
              samplerVal = inlineSamplerMD.addressMode;
            } else if constexpr (ArgTy == ImplicitArg::SAMPLER_NORMALIZED) {
              samplerVal = inlineSamplerMD.NormalizedCoords;
            } else {
              llvm_unreachable("unexpected sampler property");
            }
          }
        }
      }
      return ConstantInt::get(CI.getType(), samplerVal);
    }
  }

  // TODO: For now disable WA if unable to trace sampler argument.
  // Will need to rework WA to add support for indirect sampler case.
  return ConstantInt::get(CI.getType(), 0);
}

Value *ImageFuncResolution::getSamplerAddressMode(CallInst &CI) {
  return getSamplerProperty<ImplicitArg::SAMPLER_ADDRESS>(CI);
}

Value *ImageFuncResolution::getSamplerNormalizedCoords(CallInst &CI) {
  return getSamplerProperty<ImplicitArg::SAMPLER_NORMALIZED>(CI);
}

Argument *ImageFuncResolution::getImplicitImageArg(CallInst &CI, ImplicitArg::ArgType argType) {
  // Only images that are arguments are supported!
  Argument *image = cast<Argument>(ValueTracker::track(&CI, 0));

  unsigned int numImplicitArgs = m_implicitArgs.size();
  unsigned int implicitArgIndex = m_implicitArgs.getImageArgIndex(argType, image);

  Function *pFunc = CI.getParent()->getParent();
  IGC_ASSERT_MESSAGE(pFunc->arg_size() >= numImplicitArgs, "Function arg size does not match meta data args.");
  unsigned int implicitArgIndexInFunc = pFunc->arg_size() - numImplicitArgs + implicitArgIndex;

  return std::next(pFunc->arg_begin(), implicitArgIndexInFunc);
}
