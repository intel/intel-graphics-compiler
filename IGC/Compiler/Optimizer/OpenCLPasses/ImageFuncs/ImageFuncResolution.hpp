/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
/// @brief  ImageFuncResolution pass used for resolving OpenCL image dimension functions.
///         This pass depends on the ImageFuncAnalysis and AddImplicitArgs passes runing before it

// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class ImageFuncResolution : public llvm::InstVisitor<ImageFuncResolution> {
public:
  /// @brief  Constructor
  ImageFuncResolution() {}

  /// @brief  Destructor
  ~ImageFuncResolution() {}

  /// @brief  Provides name of pass
  static llvm::StringRef getPassName() { return "ImageFuncResolution"; }

  /// @brief  Main entry point.
  ///         Finds all OpenCL image dimension function calls and resolve them into an llvm sequence
  /// @param  F The destination function.
  bool runOnFunction(llvm::Function &F, IGC::IGCMD::MetaDataUtils *pMdUtils, IGC::ModuleMetaData *pModMD,
                     IGC::CodeGenContext *pCtx);

  /// @brief  Call instructions visitor.
  ///         Checks for OpenCL image dimension functions and resolves them into appropriate sequence of code
  /// @param  CI The call instruction.
  void visitCallInst(llvm::CallInst &CI);

private:
  /// @brief  Resolves get_image_height(image).
  ///         Adds the appropriate sequence of code before the given call instruction
  /// @param  CI The call instruction.
  /// @return A value representing the image height
  llvm::Value *getImageHeight(llvm::CallInst &CI);

  /// @brief  Resolves get_image_width(image).
  ///         Adds the appropriate sequence of code before the given call instruction
  /// @param  CI The call instruction.
  /// @return A value representing the image width
  llvm::Value *getImageWidth(llvm::CallInst &CI);

  /// @brief  Resolves get_image_depth(image).
  ///         Adds the appropriate sequence of code before the given call instruction
  /// @param  CI The call instruction.
  /// @return A value representing the image depth
  llvm::Value *getImageDepth(llvm::CallInst &CI);

  /// @brief  Resolves get_image_num_mip_levels(image).
  ///         Adds the appropriate sequence of code before the given call instruction
  /// @param  CI The call instruction.
  /// @return A value representing the image num mip levels
  llvm::Value *getImageNumMipLevels(llvm::CallInst &CI);

  /// @brief  Resolves get_image_channel_data_type(image).
  ///         Adds the approtiate sequence of code before the given call isntruction
  /// @param  CI The call instruction.
  /// @return A value representing the image channel data type
  llvm::Value *getImageChannelDataType(llvm::CallInst &CI);

  /// @brief  Resolves get_image_channel_order(image).
  ///         Adds the approtiate sequence of code before the given call isntruction
  /// @param  CI The call instruction.
  /// @return A value representing the image order
  llvm::Value *getImageChannelOrder(llvm::CallInst &CI);

  /// @brief  Resolves get_image_array_size(image_array).
  ///         Adds the approtiate sequence of code before the given call isntruction
  /// @param  CI The call instruction.
  /// @return A value representing the image array size
  llvm::Value *getImageArraySize(llvm::CallInst &CI);

  /// @brief  Resolves get_image_num_samples(image).
  ///         Adds the approtiate sequence of code before the given call instruction
  /// @param  CI The call instruction.
  /// @return A value representing the image number of samples.
  llvm::Value *getImageNumSamples(llvm::CallInst &CI);

  /// @brief  Resolves sampler pseudo-builtin, e.g. get_sampler_address_mode.
  /// @param  CI The call instruction.
  /// @return A value representing the sampler property, which may either be
  ///         a ConstantInt or an Argument.
  template <ImplicitArg::ArgType ArgTy> llvm::Value *getSamplerProperty(llvm::CallInst &CI);

  /// @brief  Resolves the pseudo-builtin get_sampler_address_mode(sampler_t).
  ///         Adds the approtiate sequence of code before the given call isntruction
  /// @param  CI The call instruction.
  /// @return A value representing the sampler address mode, which may either be
  ///         a ConstantInt or an Argument
  llvm::Value *getSamplerAddressMode(llvm::CallInst &CI);

  /// @brief  Resolves the pseudo-builtin get_sampler_normalized_coords(sampler_t).
  ///         Adds the approtiate sequence of code before the given call isntruction
  /// @param  CI The call instruction.
  /// @return A value representing the sampler normalized coords mode, which may either be
  ///         a ConstantInt or an Argument
  llvm::Value *getSamplerNormalizedCoords(llvm::CallInst &CI);

  /// @brief  Resolves the pseudo-builtin get_sampler_snap_wa_reqd(sampler_t).
  ///         Adds the approtiate sequence of code before the given call isntruction
  /// @param  CI The call instruction.
  /// @return A value representing whether the snap workaround is required for the sample
  ///         which may either be a ConstantInt or an Argument
  llvm::Value *getSamplerSnapWARequired(llvm::CallInst &CI);

  /// @brief  Returns the appropriate implicit argument of the function
  ///         containing the given call instruction, based on the given implicit image
  ///         argument type
  /// @param  CI       The call instruction.
  /// @param  argType  The implicit image argument type.
  /// @return The function argument associated with the given implicit image arg type
  llvm::Argument *getImplicitImageArg(llvm::CallInst &CI, ImplicitArg::ArgType argType);

  /// @brief  The implicit arguments of the current function
  ImplicitArgs m_implicitArgs;

  /// @brief  Indicates if the pass changed the processed function
  bool m_changed{};

  /// @brief provides access to CodeGenContext
  CodeGenContext *m_pCtx = nullptr;

  /// @brief  MetaData utils / module metadata, cached at runOnFunction() entry.
  IGC::IGCMD::MetaDataUtils *m_pMdUtils = nullptr;
  ModuleMetaData *m_modMD = nullptr;
};

// Legacy Pass Manager wrapper.
class ImageFuncResolutionLPM : public llvm::FunctionPass {
public:
  static char ID;

  ImageFuncResolutionLPM();
  ~ImageFuncResolutionLPM() {}

  virtual llvm::StringRef getPassName() const override { return ImageFuncResolution::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  virtual bool runOnFunction(llvm::Function &F) override {
    return m_impl.runOnFunction(F, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                                getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData(),
                                getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  }

private:
  ImageFuncResolution m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. Modeled as a module pass that loops over the defined functions
// (the seeded context analyses are module-level; IGC passes never use skipFunction). name()
// returns the legacy pass argument so PrintBefore/PrintAfter matches under the new pass manager.
class ImageFuncResolutionNPM : public llvm::PassInfoMixin<ImageFuncResolutionNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-image-func-resolution"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
