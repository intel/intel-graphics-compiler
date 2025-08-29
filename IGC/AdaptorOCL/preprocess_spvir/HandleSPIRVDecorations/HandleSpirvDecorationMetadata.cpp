/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "HandleSpirvDecorationMetadata.h"
#include "AdaptorOCL/Utils/CacheControlsHelper.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"

#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Instructions.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Demangle/Demangle.h>
#include <llvm/IR/Mangler.h>
#include <llvm/Support/Regex.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-handle-spirv-decoration-metadata"
#define PASS_DESCRIPTION "Handle spirv.Decoration metadata"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(HandleSpirvDecorationMetadata, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(HandleSpirvDecorationMetadata, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char HandleSpirvDecorationMetadata::ID = 0;

HandleSpirvDecorationMetadata::HandleSpirvDecorationMetadata() : ModulePass(ID) {
  initializeHandleSpirvDecorationMetadataPass(*PassRegistry::getPassRegistry());
}

bool HandleSpirvDecorationMetadata::runOnModule(Module &module) {
  m_Metadata = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
  m_pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  m_Module = &module;
  m_BuiltinsToRemove.clear();

  handleInstructionsDecorations();
  handleGlobalVariablesDecorations();

  for (auto &F : m_BuiltinsToRemove)
    F->eraseFromParent();

  return m_changed;
}

void HandleSpirvDecorationMetadata::handleInstructionsDecorations() { visit(m_Module); }

void HandleSpirvDecorationMetadata::handleGlobalVariablesDecorations() {
  for (auto &globalVariable : m_Module->globals()) {
    auto spirvDecorations = parseSPIRVDecorationsFromMD(&globalVariable);
    for (auto &[DecorationId, MDNodes] : spirvDecorations) {
      switch (DecorationId) {
      // IDecHostAccessINTEL
      // We use two IDs (6147 and 6188) to preserve backward compatibility with
      // older versions
      case 6147: // Old ID
      case 6188: // New ID
      {
        IGC_ASSERT_MESSAGE(MDNodes.size() == 1, "Only one HostAccessINTEL decoration can be applied "
                                                "to a single global variable!");
        handleHostAccessIntel(globalVariable, *MDNodes.begin());
        break;
      }
      default:
        continue;
      }
    }
  }
}

void HandleSpirvDecorationMetadata::handleHostAccessIntel(GlobalVariable &globalVariable, MDNode *node) {
  globalVariable.addAttribute("host_var_name", dyn_cast<MDString>(node->getOperand(2))->getString());

  m_changed = true;
  m_Metadata->capabilities.globalVariableDecorationsINTEL = true;
}

void HandleSpirvDecorationMetadata::visitLoadInst(LoadInst &I) {
  auto spirvDecorations = parseSPIRVDecorationsFromMD(I.getPointerOperand());
  for (auto &[DecorationId, MDNodes] : spirvDecorations) {
    switch (DecorationId) {
    // IDecCacheControlLoadINTEL
    case DecorationIdCacheControlLoad: {
      handleCacheControlINTEL<LoadCacheControl>(I, MDNodes);
      break;
    }
    default:
      continue;
    }
  }
}

void HandleSpirvDecorationMetadata::visitStoreInst(StoreInst &I) {
  auto spirvDecorations = parseSPIRVDecorationsFromMD(I.getPointerOperand());
  for (auto &[DecorationId, MDNodes] : spirvDecorations) {
    switch (DecorationId) {
    // IDecCacheControlStoreINTEL
    case DecorationIdCacheControlStore: {
      handleCacheControlINTEL<StoreCacheControl>(I, MDNodes);
      break;
    }
    default:
      continue;
    }
  }
}

void HandleSpirvDecorationMetadata::visit2DBlockReadCallInst(CallInst &I, StringRef unmangledName) {
  Value *ptr = I.getArgOperand(0);
  auto spirvDecorations = parseSPIRVDecorationsFromMD(ptr);
  for (auto &[DecorationId, MDNodes] : spirvDecorations) {
    switch (DecorationId) {
    // IDecCacheControlLoadINTEL
    case DecorationIdCacheControlLoad: {
      handleCacheControlINTELFor2DBlockIO<LoadCacheControl>(I, MDNodes, unmangledName);
      break;
    }
    }
  }
}

void HandleSpirvDecorationMetadata::visit2DBlockWriteCallInst(CallInst &I, StringRef unmangledName) {
  Value *ptr = I.getArgOperand(0);
  auto spirvDecorations = parseSPIRVDecorationsFromMD(ptr);
  for (auto &[DecorationId, MDNodes] : spirvDecorations) {
    switch (DecorationId) {
    // IDecCacheControlStoreINTEL
    case DecorationIdCacheControlStore: {
      handleCacheControlINTELFor2DBlockIO<StoreCacheControl>(I, MDNodes, unmangledName);
      break;
    }
    }
  }
}

void HandleSpirvDecorationMetadata::visitPrefetchCallInst(CallInst &I) {
  Value *ptr = I.getArgOperand(0);
  auto spirvDecorations = parseSPIRVDecorationsFromMD(ptr);
  for (auto &[DecorationId, MDNodes] : spirvDecorations) {
    switch (DecorationId) {
    // IDecCacheControlLoadINTEL
    case DecorationIdCacheControlLoad: {
      handleCacheControlINTELForPrefetch(I, MDNodes);
      break;
    }
    }
  }
}

void HandleSpirvDecorationMetadata::visit1DBlockReadCallInst(CallInst &I) {
  Value *ptr = I.getArgOperand(0);
  auto spirvDecorations = parseSPIRVDecorationsFromMD(ptr);
  for (auto &[DecorationId, MDNodes] : spirvDecorations) {
    switch (DecorationId) {
    // IDecCacheControlLoadINTEL
    case DecorationIdCacheControlLoad: {
      handleCacheControlINTELFor1DBlockIO<LoadCacheControl>(I, MDNodes);
      break;
    }
    }
  }
}

void HandleSpirvDecorationMetadata::visit1DBlockWriteCallInst(CallInst &I) {
  Value *ptr = I.getArgOperand(0);
  auto spirvDecorations = parseSPIRVDecorationsFromMD(ptr);
  for (auto &[DecorationId, MDNodes] : spirvDecorations) {
    switch (DecorationId) {
    // IDecCacheControlStoreINTEL
    case DecorationIdCacheControlStore: {
      handleCacheControlINTELFor1DBlockIO<StoreCacheControl>(I, MDNodes);
      break;
    }
    }
  }
}

void HandleSpirvDecorationMetadata::visit1DBlockPrefetchCallInst(CallInst &I) {
  Value *ptr = I.getArgOperand(0);
  auto spirvDecorations = parseSPIRVDecorationsFromMD(ptr);
  for (auto &[DecorationId, MDNodes] : spirvDecorations) {
    switch (DecorationId) {
    // IDecCacheControlLoadINTEL
    case DecorationIdCacheControlLoad: {
      handleCacheControlINTELFor1DBlockIO<LoadCacheControl>(I, MDNodes);
      break;
    }
    }
  }
}

void HandleSpirvDecorationMetadata::visitOCL1DBlockPrefetchCallInst(CallInst &I, SmallVectorImpl<StringRef> &Matches) {
  Value *ptr = I.getArgOperand(0);
  auto spirvDecorations = parseSPIRVDecorationsFromMD(ptr);
  for (auto &[DecorationId, MDNodes] : spirvDecorations) {
    switch (DecorationId) {
    // IDecCacheControlLoadINTEL
    case DecorationIdCacheControlLoad: {
      handleCacheControlINTELForOCL1DBlockPrefetch(I, MDNodes, Matches);
      break;
    }
    }
  }
}

void HandleSpirvDecorationMetadata::visitCallInst(CallInst &I) {
  Function *F = I.getCalledFunction();
  if (!F)
    return;

  static const Regex pattern2DBlockRead(
      "_Z[0-9]+(intel_sub_group_2d_block_(prefetch|read|read_transform|read_transpose)_[0-9]+b_[0-9]+r[0-9]+x[0-9]+c)");
  static const Regex pattern2DBlockWrite("_Z[0-9]+(intel_sub_group_2d_block_write_[0-9]+b_[0-9]+r[0-9]+x[0-9]+c)");
  static const Regex patternOCL1DBlockPrefetch("_Z[0-9]+(intel_sub_group_block_prefetch_(uc|us|ui|ul)(2|4|8|16)?)");
  static const Regex patternPrefetch("_Z[0-9]+__spirv_ocl_prefetch");
  static const Regex pattern1DBlockRead("_Z[0-9]+__spirv_SubgroupBlockReadINTEL");
  static const Regex pattern1DBlockWrite("_Z[0-9]+__spirv_SubgroupBlockWriteINTEL");
  static const Regex pattern1DBlockPrefetch("_Z[0-9]+__spirv_SubgroupBlockPrefetchINTEL");

  SmallVector<StringRef, 4> Matches;
  StringRef funcName = F->getName();

  if (pattern2DBlockRead.match(funcName, &Matches)) {
    visit2DBlockReadCallInst(I, Matches[1]);
  } else if (pattern2DBlockWrite.match(funcName, &Matches)) {
    visit2DBlockWriteCallInst(I, Matches[1]);
  } else if (patternPrefetch.match(funcName, &Matches)) {
    visitPrefetchCallInst(I);
  } else if (pattern1DBlockRead.match(funcName, &Matches)) {
    visit1DBlockReadCallInst(I);
  } else if (pattern1DBlockWrite.match(funcName, &Matches)) {
    visit1DBlockWriteCallInst(I);
  } else if (pattern1DBlockPrefetch.match(funcName, &Matches)) {
    visit1DBlockPrefetchCallInst(I);
  } else if (patternOCL1DBlockPrefetch.match(funcName, &Matches)) {
    visitOCL1DBlockPrefetchCallInst(I, Matches);
  }
}

template <typename T>
void HandleSpirvDecorationMetadata::handleCacheControlINTEL(Instruction &I, SmallPtrSetImpl<MDNode *> &MDNodes) {
  static_assert(std::is_same_v<T, LoadCacheControl> || std::is_same_v<T, StoreCacheControl>);
  CacheControlFromMDNodes cacheControl = resolveCacheControlFromMDNodes<T>(m_pCtx, MDNodes);
  if (cacheControl.isEmpty)
    return;
  if (cacheControl.isInvalid) {
    m_pCtx->EmitWarning("Unsupported cache controls configuration requested. Applying default configuration.");
    return;
  }

  MDNode *CacheCtrlNode = MDNode::get(
      I.getContext(), ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(I.getContext()), cacheControl.value)));
  I.setMetadata("lsc.cache.ctrl", CacheCtrlNode);
  m_changed = true;
}

template <typename T>
void HandleSpirvDecorationMetadata::handleCacheControlINTELFor2DBlockIO(CallInst &I, SmallPtrSetImpl<MDNode *> &MDNodes,
                                                                        StringRef unmangledName) {
  static_assert(std::is_same_v<T, LoadCacheControl> || std::is_same_v<T, StoreCacheControl>);
  CacheControlFromMDNodes cacheControl = resolveCacheControlFromMDNodes<T>(m_pCtx, MDNodes);
  if (cacheControl.isEmpty)
    return;
  if (cacheControl.isInvalid) {
    m_pCtx->EmitWarning("Unsupported cache controls configuration requested. Applying default configuration.");
    return;
  }

  Function *F = I.getCalledFunction();
  IGC_ASSERT(F);

  SmallVector<Value *, 4> args(I.args());
  args.push_back(ConstantInt::get(Type::getInt32Ty(I.getContext()), cacheControl.value));

  SmallVector<Type *, 4> argTypes;
  for (const auto &arg : args)
    argTypes.push_back(arg->getType());

  FunctionType *FT = FunctionType::get(I.getType(), argTypes, false);
  std::string newFuncName = "__internal_" + unmangledName.str() + "_cache_controls";
  auto newFunction = m_Module->getOrInsertFunction(newFuncName, FT);

  auto newCall = CallInst::Create(newFunction, args, "", &I);
  I.replaceAllUsesWith(newCall);
  I.eraseFromParent();
  m_changed = true;

  // Cleanup unused function if all calls have been replaced with the internal version
  if (F->getNumUses() == 0)
    m_BuiltinsToRemove.insert(F);
}

void HandleSpirvDecorationMetadata::handleCacheControlINTELForPrefetch(llvm::CallInst &I,
                                                                       llvm::SmallPtrSetImpl<llvm::MDNode *> &MDNodes) {
  CacheControlFromMDNodes cacheControl = resolveCacheControlFromMDNodes<LoadCacheControl>(m_pCtx, MDNodes);
  if (cacheControl.isEmpty)
    return;
  if (cacheControl.isInvalid) {
    m_pCtx->EmitWarning("Unsupported cache controls configuration requested. Applying default configuration.");
    return;
  }

  Function *F = I.getCalledFunction();
  IGC_ASSERT(F);

  // Convert prefetch call to: __lsc_prefetch_cache_controls(global void* p, int element_size, int num_elements, enum
  // LSC_LDCC cache_opt)
  SmallVector<Value *, 4> args;
  args.push_back(I.getArgOperand(0));

  // OpenCL spec states for prefetch: "Prefetch num_gentypes * sizeof(gentype) bytes into the global cache.".
  // This design is not friendly to opaque pointers, as it assumes element size can be read from pointer.
  // For now read size from typed pointer, and in future this will be replaced with opaque prefetch with
  // explicit element size as arg.
  PointerType *PTy = dyn_cast<PointerType>(I.getArgOperand(0)->getType());
  IGC_ASSERT(PTy);

  // As of today there's no "OpUntypedPrefetch" which as previous comments mentions,
  // is needed to implement this prefetch correctly on opaque pointers.
  // It will of course result in performance penalty and needs to be changed once
  // "OpUntypedPrefetch" is ready.
  if (IGCLLVM::isOpaquePointerTy(PTy))
    return;

  args.push_back(ConstantInt::get(Type::getInt32Ty(I.getContext()),
                                  IGCLLVM::getNonOpaquePtrEltTy(PTy)->getPrimitiveSizeInBits() / 8));

  // OpenCL prefetch overloads num_elements to either i32 or i64. Convert to i32.
  IGCLLVM::IRBuilder<> builder(&I);
  args.push_back(builder.CreateZExtOrTrunc(I.getArgOperand(1), Type::getInt32Ty(I.getContext())));

  auto config = supportedLoadConfigs.find(static_cast<LSC_L1_L3_CC>(cacheControl.value));
  if (m_pCtx->platform.getPlatformInfo().eProductFamily == IGFX_PVC && config != supportedLoadConfigs.end() &&
      config->second.L1 == LoadCacheControl::Cached) {
    m_pCtx->EmitWarning("Prefetch to L1 is unsupported on this platform.");
    args.push_back(ConstantInt::get(Type::getInt32Ty(I.getContext()),
                                    mapToLSCCacheControl(LoadCacheControl::Uncached, config->second.L3)));
  } else {
    args.push_back(ConstantInt::get(Type::getInt32Ty(I.getContext()), cacheControl.value));
  }

  SmallVector<Type *, 4> argTypes;
  for (const auto &arg : args)
    argTypes.push_back(arg->getType());

  FunctionType *FT = FunctionType::get(I.getType(), argTypes, false);
  std::string newFuncName = "__lsc_prefetch_cache_controls";
  auto newFunction = m_Module->getOrInsertFunction(newFuncName, FT);

  auto newCall = CallInst::Create(newFunction, args, "", &I);
  I.replaceAllUsesWith(newCall);
  I.eraseFromParent();
  m_changed = true;

  // Cleanup unused function if all calls have been replaced with the internal version
  if (F->getNumUses() == 0)
    m_BuiltinsToRemove.insert(F);
}

template <typename T>
void HandleSpirvDecorationMetadata::handleCacheControlINTELFor1DBlockIO(CallInst &I,
                                                                        SmallPtrSetImpl<MDNode *> &MDNodes) {
  static_assert(std::is_same_v<T, LoadCacheControl> || std::is_same_v<T, StoreCacheControl>);
  CacheControlFromMDNodes cacheControl = resolveCacheControlFromMDNodes<T>(m_pCtx, MDNodes);
  if (cacheControl.isEmpty)
    return;
  if (cacheControl.isInvalid) {
    m_pCtx->EmitWarning("Unsupported cache controls configuration requested. Applying default configuration.");
    return;
  }

  Function *F = I.getCalledFunction();
  IGC_ASSERT(F);

  Type *operationType = nullptr;
  std::string funcName;
  if constexpr (std::is_same_v<T, LoadCacheControl>) {
    if (auto isPrefetch = I.getType()->isVoidTy()) {
      operationType = IGCLLVM::getNonOpaquePtrEltTy(I.getArgOperand(0)->getType());
      funcName = "SubgroupBlockPrefetchINTEL";
    } else {
      operationType = I.getType();
      funcName = "SubgroupBlockReadINTEL";
    }
  } else {
    operationType = I.getArgOperand(1)->getType();
    funcName = "SubgroupBlockWriteINTEL";
  }

  std::string typeName;
  uint32_t numElements = 1;
  Type *elementType = operationType;
  if (auto *vecTy = dyn_cast<IGCLLVM::FixedVectorType>(operationType)) {
    numElements = (uint32_t)vecTy->getNumElements();
    elementType = vecTy->getElementType();
  }

  if (elementType->isIntegerTy()) {
    switch (elementType->getIntegerBitWidth()) {
    case 8:
      typeName = "char";
      break;
    case 16:
      typeName = "short";
      break;
    case 32:
      typeName = "int";
      break;
    case 64:
      typeName = "long";
      break;
    default:
      IGC_ASSERT(0 && "Unsupported integer type");
      break;
    }
  }

  if (numElements > 1)
    typeName += std::to_string(numElements);

  SmallVector<Value *, 3> args(I.args());
  args.push_back(ConstantInt::get(Type::getInt32Ty(I.getContext()), cacheControl.value));

  SmallVector<Type *, 3> argTypes;
  for (const auto &arg : args)
    argTypes.push_back(arg->getType());

  FunctionType *funcTy = FunctionType::get(I.getType(), argTypes, false);
  std::string newFuncName = "__internal_" + funcName + "_" + typeName + "_cache_controls";
  auto newFunction = m_Module->getOrInsertFunction(newFuncName, funcTy);

  auto newCall = CallInst::Create(newFunction, args, "", &I);
  I.replaceAllUsesWith(newCall);
  I.eraseFromParent();
  m_changed = true;

  // Cleanup unused function if all calls have been replaced with the internal version
  if (F->getNumUses() == 0)
    m_BuiltinsToRemove.insert(F);
}

void HandleSpirvDecorationMetadata::handleCacheControlINTELForOCL1DBlockPrefetch(CallInst &I,
                                                                                 SmallPtrSetImpl<MDNode *> &MDNodes,
                                                                                 SmallVectorImpl<StringRef> &Matches) {
  IGC_ASSERT(Matches[1].startswith("intel_sub_group_block_prefetch"));

  CacheControlFromMDNodes cacheControl = resolveCacheControlFromMDNodes<LoadCacheControl>(m_pCtx, MDNodes);
  if (cacheControl.isEmpty)
    return;
  if (cacheControl.isInvalid) {
    m_pCtx->EmitWarning("Unsupported cache controls configuration requested. Applying default configuration.");
    return;
  }

  Function *F = I.getCalledFunction();
  IGC_ASSERT(F);

  StringRef numElementsFromName = Matches[3] != "" ? Matches[3] : "1";
  uint32_t numElementsToPrefetch = std::stoi(numElementsFromName.str());
  IGC_ASSERT(numElementsToPrefetch == 1 || numElementsToPrefetch == 2 || numElementsToPrefetch == 4 ||
             numElementsToPrefetch == 8 || numElementsToPrefetch == 16);

  uint32_t typeSizeInBytes = 0;
  if (Matches[2].equals("uc"))
    typeSizeInBytes = 1;
  else if (Matches[2].equals("us"))
    typeSizeInBytes = 2;
  else if (Matches[2].equals("ui"))
    typeSizeInBytes = 4;
  else if (Matches[2].equals("ul"))
    typeSizeInBytes = 8;
  else
    IGC_ASSERT(0 && "Unsupported type prefetch!");

  std::string typeName;
  switch (typeSizeInBytes) {
  case 1:
    typeName = "char";
    break;
  case 2:
    typeName = "short";
    break;
  case 4:
    typeName = "int";
    break;
  case 8:
    typeName = "long";
    break;
  default:
    IGC_ASSERT(0 && "Unsupported block prefetch!");
    break;
  }

  Value *numBytesArg = (ConstantInt::get(Type::getInt32Ty(I.getContext()), (typeSizeInBytes * numElementsToPrefetch)));

  SmallVector<Value *, 3> args(I.args());
  args.push_back(numBytesArg);
  args.push_back(ConstantInt::get(Type::getInt32Ty(I.getContext()), cacheControl.value));

  SmallVector<Type *, 3> argTypes;
  for (const auto &arg : args)
    argTypes.push_back(arg->getType());

  auto *funcTy = FunctionType::get(I.getType(), argTypes, false);
  auto newFuncName = "__internal_SubgroupBlockPrefetchINTEL_" + typeName + "_cache_controls";
  auto newFunction = m_Module->getOrInsertFunction(newFuncName, funcTy);

  auto newCall = CallInst::Create(newFunction, args, "", &I);
  I.replaceAllUsesWith(newCall);
  I.eraseFromParent();
  m_changed = true;

  // Cleanup unused function if all calls have been replaced with the internal version
  if (F->getNumUses() == 0)
    m_BuiltinsToRemove.insert(F);
}
