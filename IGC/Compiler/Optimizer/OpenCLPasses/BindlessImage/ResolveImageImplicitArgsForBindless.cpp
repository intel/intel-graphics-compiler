/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ResolveImageImplicitArgsForBindless.hpp"

#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/messageEncoding.hpp"
#include "LLVMWarningsPush.hpp"
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include "LLVMWarningsPop.hpp"
#include <map>

using namespace IGC;
using namespace llvm;

// Register pass to igc-opt
#define PASS_FLAG "resolve-image-implicit-args-for-bindless"
#define PASS_DESCRIPTION "Resolve OCL image implicit args for bindless"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ResolveImageImplicitArgsForBindless, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY,
                          PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(ResolveImageImplicitArgsForBindless, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ResolveImageImplicitArgsForBindless::ID = 0;

ResolveImageImplicitArgsForBindless::ResolveImageImplicitArgsForBindless() : ModulePass(ID) {
  initializeResolveImageImplicitArgsForBindlessPass(*PassRegistry::getPassRegistry());
}

namespace {
struct ImageImplicitArgInfo {
  uint32_t OffsetInStruct;
  uint32_t AlignmentInBytes;
};

// Struct layout as defined in NEO runtime under the same name:
// https://github.com/intel/compute-runtime/blob/master/shared/source/helpers/surface_format_info.h
struct ImageImplicitArgs {
  uint8_t structSize;
  uint8_t structVersion;
  //
  // 6 bytes padding for 8-byte alignment of following fields
  //
  uint64_t imageWidth;
  uint64_t imageHeight;
  uint64_t imageDepth;
  uint64_t imageArraySize;
  uint32_t numSamples;
  uint32_t channelType;
  uint32_t channelOrder;
  uint32_t numMipLevels;
  // The flat* fields below are leftover dead code.
  uint64_t flatBaseOffset;
  uint64_t flatWidth;
  uint64_t flagHeight;
  uint64_t flatPitch;
};

std::map<StringRef, ImageImplicitArgInfo> BuiltinToArgMap = {
    {"__builtin_IB_get_image_width", {offsetof(ImageImplicitArgs, imageWidth), 8}},
    {"__builtin_IB_get_image_height", {offsetof(ImageImplicitArgs, imageHeight), 8}},
    {"__builtin_IB_get_image_depth", {offsetof(ImageImplicitArgs, imageDepth), 8}},
    {"__builtin_IB_get_image_num_mip_levels", {offsetof(ImageImplicitArgs, numMipLevels), 4}},
    {"__builtin_IB_get_image_channel_data_type", {offsetof(ImageImplicitArgs, channelType), 4}},
    {"__builtin_IB_get_image_channel_order", {offsetof(ImageImplicitArgs, channelOrder), 4}},
    {"__builtin_IB_get_image1d_array_size", {offsetof(ImageImplicitArgs, imageArraySize), 8}},
    {"__builtin_IB_get_image2d_array_size", {offsetof(ImageImplicitArgs, imageArraySize), 8}},
    {"__builtin_IB_get_image_num_samples", {offsetof(ImageImplicitArgs, numSamples), 4}},
};
} // namespace

bool ResolveImageImplicitArgsForBindless::runOnModule(Module &M) {
  mChanged = false;

  CodeGenContext *Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  if (!Ctx->getModuleMetaData()->extensions.spvINTELBindlessImages)
    return false; // Bindless exclusive pass.
  mDriverInfo = &Ctx->m_DriverInfo;

  visit(M);

  for (Instruction *Inst : mInstsToRemove) {
    Inst->eraseFromParent();
  }
  mInstsToRemove.clear();

  return mChanged;
}

void ResolveImageImplicitArgsForBindless::visitCallInst(CallInst &CI) {
  auto *CalledFunction = CI.getCalledFunction();
  if (!CalledFunction)
    return;

  auto FuncName = CalledFunction->getName();
  if (!BuiltinToArgMap.count(FuncName))
    return;

  auto [ArgOffsetInStruct, AlignmentInBytes] = BuiltinToArgMap.at(FuncName);

  Module *M = CI.getParent()->getParent()->getParent();
  auto Builder = IRBuilder<>(&CI);

  ConstantInt *BindlessIndex = Builder.getInt32(BINDLESS_BTI);
  uint32_t AddrSpace = EncodeAS4GFXResource(*BindlessIndex, BufferType::BINDLESS);
  Type *BindlessOffsetPtrTy = PointerType::get(M->getContext(), AddrSpace);

  Value *Img = CI.getOperand(0);
  Value *ImgToInt = isa<IntegerType>(Img->getType()) ? Builder.CreateZExtOrTrunc(Img, Builder.getInt64Ty())
                                                     : Builder.CreatePtrToInt(Img, Builder.getInt64Ty());
  uint64_t SurfaceStateSize = mDriverInfo->getSurfaceStateSize();
  auto *StateSizeValue = Builder.getInt64(SurfaceStateSize);
  auto *ImageImplicitArgsOffset = Builder.CreateAdd(ImgToInt, StateSizeValue);
  Value *ImageImplicitArgs =
      Builder.CreateBitOrPointerCast(ImageImplicitArgsOffset, BindlessOffsetPtrTy, "imageImplicitArgs");

  // Create ldraw decl:
  Type *const Tys[] = {CI.getType(), ImageImplicitArgs->getType()};
  auto *Callee = GenISAIntrinsic::getDeclaration(M, GenISAIntrinsic::GenISA_ldraw_indexed, Tys);

  // Create ldraw call:
  Value *const Args[] = {ImageImplicitArgs, Builder.getInt32(ArgOffsetInStruct), Builder.getInt32(AlignmentInBytes),
                         Builder.getInt1(false)};
  CallInst *Result = Builder.CreateCall(Callee, Args);
  Result->setDebugLoc(CI.getDebugLoc());

  CI.replaceAllUsesWith(Result);
  mInstsToRemove.insert(&CI);
  mChanged = true;
}
