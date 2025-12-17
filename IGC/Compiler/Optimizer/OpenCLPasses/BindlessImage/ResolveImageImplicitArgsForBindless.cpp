/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ResolveImageImplicitArgsForBindless.hpp"

#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/messageEncoding.hpp"
#include "Compiler/CISACodeGen/Platform.hpp"
#include "GenIntrinsicEnum.h"
#include "LLVMWarningsPush.hpp"
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include "LLVMWarningsPop.hpp"
#include <cstdint>

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
enum ImageImplicitArg {
  IMAGE_ARGS_START = 0,
  IMAGE_WIDTH = IMAGE_ARGS_START,
  IMAGE_HEIGHT,
  IMAGE_DEPTH,
  IMAGE_NUM_MIP_LEVELS,
  IMAGE_CHANNEL_DATA_TYPE,
  IMAGE_CHANNEL_ORDER,
  IMAGE_ARRAY_SIZE,
  IMAGE_NUM_SAMPLES,
  IMAGE_ARGS_END = IMAGE_NUM_SAMPLES,
  IMAGE_ARGS_COUNT
};

struct BuiltinToArgMapType {
  const char *name;
  ImageImplicitArg arg;
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
} // namespace

bool ResolveImageImplicitArgsForBindless::runOnModule(Module &M) {
  mChanged = false;

  CodeGenContext *Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  if (!Ctx->getModuleMetaData()->UseBindlessImage)
    return false; // Bindless exclusive pass.
  mPlatform = &Ctx->platform;

  visit(M);

  for (Instruction *Inst : mInstsToRemove) {
    Inst->eraseFromParent();
  }
  mInstsToRemove.clear();

  return mChanged;
}

void ResolveImageImplicitArgsForBindless::visitCallInst(CallInst &CI) {
  constexpr BuiltinToArgMapType BuiltinToArgMap[] = {
      {"__builtin_IB_get_image_width", IMAGE_WIDTH},
      {"__builtin_IB_get_image_height", IMAGE_HEIGHT},
      {"__builtin_IB_get_image_depth", IMAGE_DEPTH},
      {"__builtin_IB_get_image_num_mip_levels", IMAGE_NUM_MIP_LEVELS},
      {"__builtin_IB_get_image_channel_data_type", IMAGE_CHANNEL_DATA_TYPE},
      {"__builtin_IB_get_image_channel_order", IMAGE_CHANNEL_ORDER},
      {"__builtin_IB_get_image1d_array_size", IMAGE_ARRAY_SIZE},
      {"__builtin_IB_get_image2d_array_size", IMAGE_ARRAY_SIZE},
      {"__builtin_IB_get_image_num_samples", IMAGE_NUM_SAMPLES},
  };

  auto *CalledFunction = CI.getCalledFunction();
  if (!CalledFunction)
    return;

  auto FuncName = CalledFunction->getName().str();
  ImageImplicitArg ImplicitArg = IMAGE_ARGS_COUNT;
  for (const auto &Builtin : BuiltinToArgMap) {
    if (FuncName == Builtin.name) {
      ImplicitArg = Builtin.arg;
      break;
    }
  }

  if (ImplicitArg == IMAGE_ARGS_COUNT)
    return; // Not an image implicit arg builtin, ignore.

  uint64_t ArgOffsetInStruct = 0;
  switch (ImplicitArg) {
  case IMAGE_WIDTH:
    ArgOffsetInStruct = offsetof(ImageImplicitArgs, imageWidth);
    break;
  case IMAGE_HEIGHT:
    ArgOffsetInStruct = offsetof(ImageImplicitArgs, imageHeight);
    break;
  case IMAGE_DEPTH:
    ArgOffsetInStruct = offsetof(ImageImplicitArgs, imageDepth);
    break;
  case IMAGE_ARRAY_SIZE:
    ArgOffsetInStruct = offsetof(ImageImplicitArgs, imageArraySize);
    break;
  case IMAGE_NUM_SAMPLES:
    ArgOffsetInStruct = offsetof(ImageImplicitArgs, numSamples);
    break;
  case IMAGE_CHANNEL_DATA_TYPE:
    ArgOffsetInStruct = offsetof(ImageImplicitArgs, channelType);
    break;
  case IMAGE_CHANNEL_ORDER:
    ArgOffsetInStruct = offsetof(ImageImplicitArgs, channelOrder);
    break;
  case IMAGE_NUM_MIP_LEVELS:
    ArgOffsetInStruct = offsetof(ImageImplicitArgs, numMipLevels);
    break;
  default:
    return;
  }

  uint64_t AlignmentInBytes;
  switch (ImplicitArg) {
  case IMAGE_WIDTH:
  case IMAGE_HEIGHT:
  case IMAGE_DEPTH:
  case IMAGE_ARRAY_SIZE:
    AlignmentInBytes = 8;
    break;
  case IMAGE_NUM_SAMPLES:
  case IMAGE_CHANNEL_DATA_TYPE:
  case IMAGE_CHANNEL_ORDER:
  case IMAGE_NUM_MIP_LEVELS:
    AlignmentInBytes = 4;
    break;
  default:
    return;
  }

  const auto IntType = Type::getInt32Ty(CI.getContext());
  const auto FloatType = Type::getFloatTy(CI.getContext());
  auto Builder = IRBuilder<>(&CI);

  ConstantInt *BindlessIndex = ConstantInt::get(IntType, BINDLESS_BTI);
  uint32_t AddrSpace = EncodeAS4GFXResource(*BindlessIndex, BufferType::BINDLESS);
  Type *BindlessOffsetPtrTy = llvm::PointerType::get(FloatType, AddrSpace);

  // Offset Img arg into ImageImplicitArgs bindless slot:
  Value *Img = CI.getOperand(0);
  Value *ImgToInt = isa<IntegerType>(Img->getType()) ? Img : Builder.CreatePtrToInt(Img, Builder.getInt64Ty());
  uint64_t SurfaceStateSize = mPlatform->getSurfaceStateSize();
  auto *StateSizeValue = Builder.getInt64(SurfaceStateSize);
  auto *ImageImplicitArgsOffset = Builder.CreateAdd(ImgToInt, StateSizeValue);
  Value *ImageImplicitArgs =
      Builder.CreateBitOrPointerCast(ImageImplicitArgsOffset, BindlessOffsetPtrTy, "imageImplicitArgs");

  // Create ldraw decl:
  Module *Module = CI.getParent()->getParent()->getParent();
  Type *const Tys[] = {CI.getType(), ImageImplicitArgs->getType()};
  auto *Callee = GenISAIntrinsic::getDeclaration(Module, GenISAIntrinsic::GenISA_ldraw_indexed, Tys);

  // Create ldraw call:
  Value *const Args[] = {ImageImplicitArgs, Builder.getInt32(ArgOffsetInStruct), Builder.getInt32(AlignmentInBytes),
                         Builder.getInt1(false)};
  CallInst *Result = Builder.CreateCall(Callee, Args);
  Result->setDebugLoc(CI.getDebugLoc());

  CI.replaceAllUsesWith(Result);
  mInstsToRemove.insert(&CI);
  mChanged = true;
}
