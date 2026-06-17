/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ResolveImageImplicitArgsForBindless.hpp"

#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "RelocationInfo.h"
#include "Compiler/CISACodeGen/messageEncoding.hpp"
#include "LLVMWarningsPush.hpp"
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include "LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include <map>

using namespace IGC;
using namespace llvm;

// Register pass to igc-opt
#define PASS_FLAG "resolve-image-implicit-args-for-bindless"
#define PASS_DESCRIPTION "Resolve OCL image implicit args for bindless"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ResolveImageImplicitArgsForBindlessLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY,
                          PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(ResolveImageImplicitArgsForBindlessLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY,
                        PASS_ANALYSIS)

char ResolveImageImplicitArgsForBindlessLPM::ID = 0;

ResolveImageImplicitArgsForBindlessLPM::ResolveImageImplicitArgsForBindlessLPM() : ModulePass(ID) {
  initializeResolveImageImplicitArgsForBindlessLPMPass(*PassRegistry::getPassRegistry());
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

bool ResolveImageImplicitArgsForBindless::run(Module &M, CodeGenContext *pCtx) {
  mChanged = false;

  auto *Ctx = pCtx;
  auto *modMD = Ctx->getModuleMetaData();
  if (!modMD->extensions.spvINTELBindlessImages)
    return false; // Bindless exclusive pass.

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
  Type *BindlessOffsetPtrTy = IGCLLVM::PointerType::get(Builder.getInt8Ty(), AddrSpace);

  Value *Img = CI.getOperand(0);
  Value *ImgToInt = isa<IntegerType>(Img->getType()) ? Builder.CreateZExtOrTrunc(Img, Builder.getInt64Ty())
                                                     : Builder.CreatePtrToInt(Img, Builder.getInt64Ty());

  Value *StateSizeValue;
  if (IGC_GET_FLAG_VALUE(ForceEnableSurfaceStateSizeReloc)) {
    // When SurfaceStateSize is a relocation patched by the runtime, UMD can choose the surface state size without the
    // need to recompile the module.
    auto *PatchNameArg = ConstantDataArray::getString(CI.getContext(), vISA::SURFACE_STATE_SIZE_RELOCATION_NAME, false);
    Type *PatchTys[] = {Builder.getInt32Ty(), PatchNameArg->getType()};
    auto *PatchFunc = GenISAIntrinsic::getDeclaration(M, GenISAIntrinsic::GenISA_staticConstantPatchValue, PatchTys);
    Value *SurfaceStateSizeI32 = Builder.CreateCall(PatchFunc, PatchNameArg, "surfaceStateSize");
    StateSizeValue = Builder.CreateZExt(SurfaceStateSizeI32, Builder.getInt64Ty());
  } else {
    StateSizeValue = Builder.getInt64(64);
  }

  // Offset Img arg into ImageImplicitArgs bindless slot:
  auto *ImageImplicitArgsOffset = Builder.CreateAdd(ImgToInt, StateSizeValue);
  Value *ImplicitArgsPtr =
      Builder.CreateBitOrPointerCast(ImageImplicitArgsOffset, BindlessOffsetPtrTy, "imageImplicitArgs");

  // Create ldraw decl:
  Type *const Tys[] = {CI.getType(), ImplicitArgsPtr->getType()};
  auto *Callee = GenISAIntrinsic::getDeclaration(M, GenISAIntrinsic::GenISA_ldraw_indexed, Tys);

  // Create ldraw call:
  Value *const Args[] = {ImplicitArgsPtr, Builder.getInt32(ArgOffsetInStruct), Builder.getInt32(AlignmentInBytes),
                         Builder.getInt1(false)};
  CallInst *Result = Builder.CreateCall(Callee, Args);
  Result->setDebugLoc(CI.getDebugLoc());

  CI.replaceAllUsesWith(Result);
  mInstsToRemove.insert(&CI);
  mChanged = true;
}

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses ResolveImageImplicitArgsForBindlessNPM::run(Module &M, ModuleAnalysisManager &AM) {
  bool changed = ResolveImageImplicitArgsForBindless().run(M, AM.getResult<CodeGenContextAnalysis>(M).Ctx);
  return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16
