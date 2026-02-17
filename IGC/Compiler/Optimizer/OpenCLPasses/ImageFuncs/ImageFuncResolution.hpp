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
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
/// @brief  ImageFuncResolution pass used for resolving OpenCL image dimension functions.
///         This pass depends on the ImageFuncAnalysis and AddImplicitArgs passes runing before it

class ImageFuncResolution : public llvm::FunctionPass, public llvm::InstVisitor<ImageFuncResolution> {
public:
  // Pass identification, replacement for typeid
  static char ID;

  /// @brief  Constructor
  ImageFuncResolution();

  /// @brief  Destructor
  ~ImageFuncResolution() {}

  /// @brief  Provides name of pass
  virtual llvm::StringRef getPassName() const override { return "ImageFuncResolution"; }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  /// @brief  Main entry point.
  ///         Finds all OpenCL image dimension function calls and resolve them into an llvm sequence
  /// @param  F The destination function.
  virtual bool runOnFunction(llvm::Function &F) override;

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
};

} // namespace IGC
