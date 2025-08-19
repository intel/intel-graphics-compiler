/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Spv2dBlockIOResolution.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallVector.h>
#include "llvmWrapper/Support/Regex.h"
#include "common/LLVMWarningsPop.hpp"

#include "AdaptorOCL/Utils/CacheControlsHelper.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;
using namespace IGC;

char Spv2dBlockIOResolution::ID = 0;

#define PASS_FLAG "igc-spv-2dblockio-resolution"
#define PASS_DESC "Lowering of SPIR-V INTEL 2d Block IO instructions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
#define DEBUG_TYPE "spv-2dblockio-resolution"

IGC_INITIALIZE_PASS_BEGIN(Spv2dBlockIOResolution, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(Spv2dBlockIOResolution, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

Spv2dBlockIOResolution::Spv2dBlockIOResolution() : ModulePass(ID) {
  initializeSpv2dBlockIOResolutionPass(*PassRegistry::getPassRegistry());
}

bool Spv2dBlockIOResolution::runOnModule(Module &M) {
  m_BuiltinsToRemove.clear();
  m_Module = &M;
  m_Changed = false;
  m_Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  visit(M);

  for (auto &F : m_BuiltinsToRemove)
    F->eraseFromParent();

  return m_Changed;
}

template <typename CCT>
CacheControlFromMDNodes Spv2dBlockIOResolution::resolveCacheControlDecorations(Value *pointerValue) {
  static_assert(std::is_same_v<CCT, LoadCacheControl> || std::is_same_v<CCT, StoreCacheControl>);
  auto spirvDecorations = parseSPIRVDecorationsFromMD(pointerValue);

  for (auto &[DecorationId, MDNodes] : spirvDecorations) {
    if (DecorationId == getDecorationIdCacheControl<CCT>()) {
      CacheControlFromMDNodes controls = resolveCacheControlFromMDNodes<CCT>(m_Ctx, MDNodes);
      if (controls.isInvalid) {
        m_Ctx->EmitWarning("Unsupported cache controls configuration requested. Applying default configuration.");
        controls.value = 0;
      }
      return controls;
    }
  }

  CacheControlFromMDNodes defaultControls;
  defaultControls.value = 0;
  defaultControls.isEmpty = true;
  defaultControls.isInvalid = false;

  return defaultControls;
}

void Spv2dBlockIOResolution::visitCallInst(CallInst &CI) {
  Function *F = CI.getCalledFunction();
  if (!F)
    return;

  static const Regex pattern2DBlockReadSPV(
      "_Z[0-9]+(__spirv_Subgroup2DBlock(Load|LoadTransform|LoadTranspose|Prefetch)INTEL.+)");
  static const Regex pattern2DBlockWriteSPV("_Z[0-9]+(__spirv_Subgroup2DBlock(Store)INTEL.+)");

  SmallVector<StringRef, 4> Matches;
  StringRef funcName = F->getName();

  if (pattern2DBlockReadSPV.match(funcName, &Matches)) {
    Op op;
    if (Matches[2].equals_insensitive("Load"))
      op = Load;
    else if (Matches[2].equals_insensitive("LoadTransform"))
      op = LoadTransform;
    else if (Matches[2].equals_insensitive("LoadTranspose"))
      op = LoadTranspose;
    else if (Matches[2].equals_insensitive("Prefetch"))
      op = Prefetch;
    else {
      IGC_ASSERT_MESSAGE(0, "Unsupported operation name");
      return;
    }

    visit2DBlockSPVCallInst<LoadCacheControl>(CI, op);
    return;
  }

  if (pattern2DBlockWriteSPV.match(funcName, &Matches))
    visit2DBlockSPVCallInst<StoreCacheControl>(CI, Store);
}

template <typename CCT> void Spv2dBlockIOResolution::visit2DBlockSPVCallInst(CallInst &CI, Op op) {
  Value *elemSizeConstant = CI.getArgOperand(0);
  Value *tileWidthConstant = CI.getArgOperand(1);
  Value *tileHeightConstant = CI.getArgOperand(2);
  Value *numBlocksVConstant = CI.getArgOperand(3);

  if (!isConstantInstruction(elemSizeConstant, "Element Size") ||
      !isConstantInstruction(tileWidthConstant, "Block Width") ||
      !isConstantInstruction(tileHeightConstant, "Block Height") ||
      !isConstantInstruction(numBlocksVConstant, "Block Count")) {
    return;
  }

  Function *F = CI.getCalledFunction();
  IGC_ASSERT(F);

  SmallVector<Value *, 7> args;
  Value *ptrVal;
  if (op == Store) {
    args.append(CI.arg_begin() + 5, CI.arg_end());
    args.push_back(CI.getArgOperand(4));
    ptrVal = CI.getArgOperand(5);
  } else {
    args.append(CI.arg_begin() + 4, CI.arg_end());
    ptrVal = CI.getArgOperand(4);
  }

  CacheControlFromMDNodes controls = resolveCacheControlDecorations<CCT>(ptrVal);

  // Applying all cached configuration for prefetch with invalid or empty cache control
  if (op == Prefetch && m_Ctx->platform.hasLSC() &&
      (controls.isEmpty || controls.isInvalid ||
       !m_Ctx->platform.isSupportedLSCCacheControlsEnum(static_cast<LSC_L1_L3_CC>(controls.value), true))) {
    controls.value = LSC_L1C_WT_L3C_WB;
  }
  args.push_back(ConstantInt::get(Type::getInt32Ty(CI.getContext()), controls.value));

  SmallVector<Type *, 7> argTypes;
  for (const auto &arg : args)
    argTypes.push_back(arg->getType());

  FunctionType *FT = FunctionType::get(CI.getType(), argTypes, false);

  uint32_t elemSize = 8 * (uint32_t)cast<ConstantInt>(elemSizeConstant)->getZExtValue();
  uint32_t tileWidth = (uint32_t)cast<ConstantInt>(tileWidthConstant)->getZExtValue();
  uint32_t tileHeight = (uint32_t)cast<ConstantInt>(tileHeightConstant)->getZExtValue();
  uint32_t numBlocksV = (uint32_t)cast<ConstantInt>(numBlocksVConstant)->getZExtValue();

  std::stringstream newFuncName;
  newFuncName << "__internal_intel_sub_group_2d_block_";

  if (op == Load)
    newFuncName << "read_";
  else if (op == LoadTransform)
    newFuncName << "read_transform_";
  else if (op == LoadTranspose)
    newFuncName << "read_transpose_";
  else if (op == Prefetch)
    newFuncName << "prefetch_";
  else if (op == Store)
    newFuncName << "write_";
  else {
    IGC_ASSERT_MESSAGE(0, "Unsupported operation name");
    return;
  }

  int simdSize = IGC::getSIMDSize(getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(), CI.getParent()->getParent());
  bool isSimd32Op = simdSize == 32 && op != Prefetch;

  newFuncName << elemSize << "b_" << tileHeight << "r" << tileWidth << "x" << numBlocksV << "c"
              << (isSimd32Op ? "_sg32" : "") << "_cache_controls";
  auto newFunction = m_Module->getOrInsertFunction(newFuncName.str(), FT);

  auto newCall = CallInst::Create(newFunction, args, "", &CI);
  CI.replaceAllUsesWith(newCall);
  CI.eraseFromParent();
  m_Changed = true;

  // Cleanup unused function if all calls have been replaced with the internal version
  if (F->getNumUses() == 0)
    m_BuiltinsToRemove.insert(F);
}

// Check if parameter of __spirv_Subgroup2DBlock is constant instruction
bool Spv2dBlockIOResolution::isConstantInstruction(Value *val, StringRef valName) {
  if (isa<ConstantInt>(val))
    return true;

  std::stringstream ss;
  ss << "Expected " << valName.str() << " to be constant instruction in __spirv_Subgroup2DBlock operation\n";
  m_Ctx->EmitError(ss.str().c_str(), val);

  return false;
}
