/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CISACodeGen/getCacheOpts.h"

namespace IGC {
constexpr uint64_t DecorationIdCacheControlLoad = 6442;
constexpr uint64_t DecorationIdCacheControlStore = 6443;

enum class LoadCacheControl {
  Uncached = 0,
  Cached = 1,
  Streaming = 2,
  InvalidateAfterRead = 3,
  ConstCached = 4,
  Invalid // This value represents invalid/unsupported cache control value
};

enum class StoreCacheControl {
  Uncached = 0,
  WriteThrough = 1,
  WriteBack = 2,
  Streaming = 3,
  Invalid // This value represents invalid/unsupported cache control value
};

template <typename T> inline uint64_t getDecorationIdCacheControl() {
  static_assert(std::is_same_v<T, LoadCacheControl> || std::is_same_v<T, StoreCacheControl>);
  return (std::is_same_v<T, LoadCacheControl> ? DecorationIdCacheControlLoad : DecorationIdCacheControlStore);
}

template <typename T> inline int getDefaultCacheControlValue(CodeGenContext *ctx) {
  static_assert(std::is_same_v<T, LoadCacheControl> || std::is_same_v<T, StoreCacheControl>);
  return std::is_same_v<T, LoadCacheControl> ? ctx->getModuleMetaData()->compOpt.LoadCacheDefault
                                             : ctx->getModuleMetaData()->compOpt.StoreCacheDefault;
}

template <typename T> struct SeparateCacheControlsL1L3 {
  T L1;
  T L3;
};

template <typename T>
using CacheControlMapTy = std::unordered_map<LSC_L1_L3_CC, SeparateCacheControlsL1L3<T>, std::hash<int>>;

const CacheControlMapTy<StoreCacheControl> supportedStoreConfigs = {
    // clang-format off
        { LSC_L1UC_L3UC,       { StoreCacheControl::Uncached,     StoreCacheControl::Uncached  } },
        { LSC_L1UC_L3C_WB,     { StoreCacheControl::Uncached,     StoreCacheControl::WriteBack } },
        { LSC_L1C_WT_L3UC,     { StoreCacheControl::WriteThrough, StoreCacheControl::Uncached  } },
        { LSC_L1C_WT_L3C_WB,   { StoreCacheControl::WriteThrough, StoreCacheControl::WriteBack } },
        { LSC_L1S_L3UC,        { StoreCacheControl::Streaming,    StoreCacheControl::Uncached  } },
        { LSC_L1S_L3C_WB,      { StoreCacheControl::Streaming,    StoreCacheControl::WriteBack } },
        { LSC_L1IAR_WB_L3C_WB, { StoreCacheControl::WriteBack,    StoreCacheControl::WriteBack } }
    // clang-format on
};

const CacheControlMapTy<LoadCacheControl> supportedLoadConfigs = {
    // clang-format off
        { LSC_L1UC_L3UC,       { LoadCacheControl::Uncached,            LoadCacheControl::Uncached } },
        { LSC_L1UC_L3C_WB,     { LoadCacheControl::Uncached,            LoadCacheControl::Cached   } },
        { LSC_L1C_WT_L3UC,     { LoadCacheControl::Cached,              LoadCacheControl::Uncached } },
        { LSC_L1C_WT_L3C_WB,   { LoadCacheControl::Cached,              LoadCacheControl::Cached   } },
        { LSC_L1S_L3UC,        { LoadCacheControl::Streaming,           LoadCacheControl::Uncached } },
        { LSC_L1S_L3C_WB,      { LoadCacheControl::Streaming,           LoadCacheControl::Cached   } },
        { LSC_L1IAR_WB_L3C_WB, { LoadCacheControl::InvalidateAfterRead, LoadCacheControl::Cached   } },
        { LSC_L1UC_L3CC,       { LoadCacheControl::Uncached,            LoadCacheControl::ConstCached,        } },
        { LSC_L1C_L3CC,        { LoadCacheControl::Cached,              LoadCacheControl::ConstCached,        } },
        { LSC_L1IAR_L3IAR,     { LoadCacheControl::InvalidateAfterRead, LoadCacheControl::InvalidateAfterRead } },
    // clang-format on
};
template <typename T> struct SeparateCacheControlsL1L2L3 {
  T L1;
  T L2;
  T L3;
};

template <typename T>
using L1L2L3StoreCacheControlMapTy =
    std::unordered_map<LSC_STCC_L1_L2_L3, SeparateCacheControlsL1L2L3<T>, std::hash<int>>;
const L1L2L3StoreCacheControlMapTy<StoreCacheControl> supportedL1L2L3StoreConfigs = {
    // clang-format off
        { LSC_STCC_L1UC_L2UC_L3UC, { StoreCacheControl::Uncached,     StoreCacheControl::Uncached,  StoreCacheControl::Uncached } },
        { LSC_STCC_L1UC_L2UC_L3WB, { StoreCacheControl::Uncached,     StoreCacheControl::Uncached,  StoreCacheControl::WriteBack } },
        { LSC_STCC_L1UC_L2WB_L3UC, { StoreCacheControl::Uncached,     StoreCacheControl::WriteBack, StoreCacheControl::Uncached } },
        { LSC_STCC_L1UC_L2WB_L3WB, { StoreCacheControl::Uncached,     StoreCacheControl::WriteBack, StoreCacheControl::WriteBack } },
        { LSC_STCC_L1WT_L2UC_L3UC, { StoreCacheControl::WriteThrough, StoreCacheControl::Uncached,  StoreCacheControl::Uncached } },
        { LSC_STCC_L1WT_L2UC_L3WB, { StoreCacheControl::WriteThrough, StoreCacheControl::Uncached,  StoreCacheControl::WriteBack } },
        { LSC_STCC_L1WT_L2WB_L3UC, { StoreCacheControl::WriteThrough, StoreCacheControl::WriteBack, StoreCacheControl::Uncached } },
        { LSC_STCC_L1WT_L2WB_L3WB, { StoreCacheControl::WriteThrough, StoreCacheControl::WriteBack, StoreCacheControl::WriteBack } },
        { LSC_STCC_L1S_L2UC_L3UC,  { StoreCacheControl::Streaming,    StoreCacheControl::Uncached,  StoreCacheControl::Uncached } },
        { LSC_STCC_L1S_L2UC_L3WB,  { StoreCacheControl::Streaming,    StoreCacheControl::Uncached,  StoreCacheControl::WriteBack } },
        { LSC_STCC_L1S_L2WB_L3UC,  { StoreCacheControl::Streaming,    StoreCacheControl::WriteBack, StoreCacheControl::Uncached } },
        { LSC_STCC_L1WB_L2UC_L3UC, { StoreCacheControl::WriteBack,    StoreCacheControl::Uncached,  StoreCacheControl::Uncached } },
        { LSC_STCC_L1WB_L2WB_L3UC, { StoreCacheControl::WriteBack,    StoreCacheControl::WriteBack, StoreCacheControl::Uncached } },
        { LSC_STCC_L1WB_L2UC_L3WB, { StoreCacheControl::WriteBack,    StoreCacheControl::Uncached,  StoreCacheControl::WriteBack } },
    // clang-format on
};

template <typename T>
using L1L2L3LoadCacheControlMapTy =
    std::unordered_map<LSC_LDCC_L1_L2_L3, SeparateCacheControlsL1L2L3<T>, std::hash<int>>;
const L1L2L3LoadCacheControlMapTy<LoadCacheControl> supportedL1L2L3LoadConfigs = {
    // clang-format off
        { LSC_LDCC_L1UC_L2UC_L3UC,    { LoadCacheControl::Uncached,  LoadCacheControl::Uncached, LoadCacheControl::Uncached } },
        { LSC_LDCC_L1UC_L2UC_L3C,     { LoadCacheControl::Uncached,  LoadCacheControl::Uncached, LoadCacheControl::Cached } },
        { LSC_LDCC_L1UC_L2C_L3UC,     { LoadCacheControl::Uncached,  LoadCacheControl::Cached,   LoadCacheControl::Uncached } },
        { LSC_LDCC_L1UC_L2C_L3C,      { LoadCacheControl::Uncached,  LoadCacheControl::Cached,   LoadCacheControl::Cached } },
        { LSC_LDCC_L1C_L2UC_L3UC,     { LoadCacheControl::Cached,    LoadCacheControl::Uncached, LoadCacheControl::Uncached } },
        { LSC_LDCC_L1C_L2UC_L3C,      { LoadCacheControl::Cached,    LoadCacheControl::Uncached, LoadCacheControl::Cached } },
        { LSC_LDCC_L1C_L2C_L3UC,      { LoadCacheControl::Cached,    LoadCacheControl::Cached,   LoadCacheControl::Uncached } },
        { LSC_LDCC_L1C_L2C_L3C,       { LoadCacheControl::Cached,    LoadCacheControl::Cached,   LoadCacheControl::Cached } },
        { LSC_LDCC_L1S_L2UC_L3UC,     { LoadCacheControl::Streaming, LoadCacheControl::Uncached, LoadCacheControl::Uncached } },
        { LSC_LDCC_L1S_L2UC_L3C,      { LoadCacheControl::Streaming, LoadCacheControl::Uncached, LoadCacheControl::Cached } },
        { LSC_LDCC_L1S_L2C_L3UC,      { LoadCacheControl::Streaming, LoadCacheControl::Cached,   LoadCacheControl::Uncached } },
        { LSC_LDCC_L1S_L2C_L3C,       { LoadCacheControl::Streaming, LoadCacheControl::Cached,   LoadCacheControl::Cached } },
        { LSC_LDCC_L1IAR_L2IAR_L3IAR, { LoadCacheControl::InvalidateAfterRead, LoadCacheControl::InvalidateAfterRead, LoadCacheControl::InvalidateAfterRead } },
    // clang-format on
};

using CacheLevel = uint64_t;

// This function expects MDNodes to be in the following form:
// MD =  {  opcode,   cache_level,  cache_control}
// -----------------------------------------------
// !8 =  !{i32 6442,    i32 1,          i32 0   }
// !9 =  !{i32 6442,    i32 3,          i32 0   }
// !11 = !{i32 6442,    i32 1,          i32 0   }
// !12 = !{i32 6442,    i32 3,          i32 1   }
template <typename T>
inline llvm::SmallDenseMap<CacheLevel, T> parseCacheControlsMD(llvm::SmallPtrSetImpl<llvm::MDNode *> &MDNodes) {
  using namespace llvm;
  auto getLiteral = [](const MDNode *node, const uint32_t index) {
    if (auto value = dyn_cast<ValueAsMetadata>(node->getOperand(index)))
      if (auto constantInt = dyn_cast<ConstantInt>(value->getValue()))
        return std::optional<uint64_t>(constantInt->getZExtValue());

    return std::optional<uint64_t>();
  };

  SmallDenseMap<CacheLevel, T> cacheControls;
  for (auto Node : MDNodes) {
    IGC_ASSERT(Node->getNumOperands() == 3);
    IGC_ASSERT(getLiteral(Node, 0).value() == DecorationIdCacheControlLoad ||
               getLiteral(Node, 0).value() == DecorationIdCacheControlStore);

    auto cacheLevel = getLiteral(Node, 1);
    auto cacheControl = getLiteral(Node, 2);

    IGC_ASSERT(cacheLevel && cacheControl);
    cacheControls[cacheLevel.value()] = static_cast<T>(cacheControl.value());
  }
  return cacheControls;
}

template <typename T>
inline std::optional<T> getCacheControl(llvm::SmallDenseMap<CacheLevel, T> &cacheControls, CacheLevel level) {
  if (auto E = cacheControls.find(level); E != cacheControls.end())
    return E->second;
  return {};
}

inline LSC_L1_L3_CC mapToLSCCacheControl(StoreCacheControl L1Control, StoreCacheControl L3Control) {
  for (auto &[LSCEnum, SPIRVEnum] : supportedStoreConfigs)
    if (SPIRVEnum.L1 == L1Control && SPIRVEnum.L3 == L3Control)
      return LSCEnum;

  return LSC_CC_INVALID;
}

inline LSC_L1_L3_CC mapToLSCCacheControl(LoadCacheControl L1Control, LoadCacheControl L3Control) {
  for (auto &[LSCEnum, SPIRVEnum] : supportedLoadConfigs)
    if (SPIRVEnum.L1 == L1Control && SPIRVEnum.L3 == L3Control)
      return LSCEnum;

  return LSC_CC_INVALID;
}

inline LSC_STCC_L1_L2_L3 mapToLSCL1L2L3CacheControl(StoreCacheControl L1Control, StoreCacheControl L2Control,
                                                    StoreCacheControl L3Control) {
  for (auto &[LSCEnum, SPIRVEnum] : supportedL1L2L3StoreConfigs)
    if (SPIRVEnum.L1 == L1Control && SPIRVEnum.L2 == L2Control && SPIRVEnum.L3 == L3Control)
      return LSCEnum;

  return LSC_STCC_INVALID;
}

inline LSC_LDCC_L1_L2_L3 mapToLSCL1L2L3CacheControl(LoadCacheControl L1Control, LoadCacheControl L2Control,
                                                    LoadCacheControl L3Control) {
  for (auto &[LSCEnum, SPIRVEnum] : supportedL1L2L3LoadConfigs)
    if (SPIRVEnum.L1 == L1Control && SPIRVEnum.L2 == L2Control && SPIRVEnum.L3 == L3Control)
      return LSCEnum;

  return LSC_LDCC_INVALID;
}

template <typename T> SeparateCacheControlsL1L3<T> mapToSPIRVCacheControl(LSC_L1_L3_CC) = delete;

template <>
inline SeparateCacheControlsL1L3<LoadCacheControl> mapToSPIRVCacheControl<LoadCacheControl>(LSC_L1_L3_CC LSCControl) {
  if (auto I = supportedLoadConfigs.find(LSCControl); I != supportedLoadConfigs.end())
    return I->second;

  IGC_ASSERT_MESSAGE(false, "Unsupported cache controls combination!");
  return {LoadCacheControl::Invalid, LoadCacheControl::Invalid};
}

template <>
inline SeparateCacheControlsL1L3<StoreCacheControl> mapToSPIRVCacheControl<StoreCacheControl>(LSC_L1_L3_CC LSCControl) {
  if (auto I = supportedStoreConfigs.find(LSCControl); I != supportedStoreConfigs.end())
    return I->second;

  IGC_ASSERT_MESSAGE(false, "Unsupported cache controls combination!");
  return {StoreCacheControl::Invalid, StoreCacheControl::Invalid};
}

template <typename T> SeparateCacheControlsL1L2L3<T> mapToSPIRVL1L2L3CacheControl(int) = delete;

template <>
inline SeparateCacheControlsL1L2L3<LoadCacheControl> mapToSPIRVL1L2L3CacheControl<LoadCacheControl>(int cacheControl) {
  auto LSCLoadCacheControl = static_cast<LSC_LDCC_L1_L2_L3>(cacheControl);
  if (auto I = supportedL1L2L3LoadConfigs.find(LSCLoadCacheControl); I != supportedL1L2L3LoadConfigs.end())
    return I->second;

  IGC_ASSERT_MESSAGE(false, "Unsupported cache controls combination!");
  return {LoadCacheControl::Invalid, LoadCacheControl::Invalid, LoadCacheControl::Invalid};
}

template <>
inline SeparateCacheControlsL1L2L3<StoreCacheControl>
mapToSPIRVL1L2L3CacheControl<StoreCacheControl>(int cacheControl) {
  auto LSCStoreCacheControl = static_cast<LSC_STCC_L1_L2_L3>(cacheControl);
  if (auto I = supportedL1L2L3StoreConfigs.find(LSCStoreCacheControl); I != supportedL1L2L3StoreConfigs.end())
    return I->second;

  IGC_ASSERT_MESSAGE(false, "Unsupported cache controls combination!");
  return {StoreCacheControl::Invalid, StoreCacheControl::Invalid, StoreCacheControl::Invalid};
}

inline llvm::DenseMap<uint64_t, llvm::SmallPtrSet<llvm::MDNode *, 4>> parseSPIRVDecorationsFromMD(llvm::Value *V) {
  using namespace llvm;
  MDNode *spirvDecorationsMD = nullptr;
  if (auto *GV = dyn_cast<GlobalVariable>(V)) {
    spirvDecorationsMD = GV->getMetadata("spirv.Decorations");
  } else if (auto *II = dyn_cast<Instruction>(V)) {
    spirvDecorationsMD = II->getMetadata("spirv.Decorations");
  } else if (auto *A = dyn_cast<Argument>(V)) {
    Function *F = A->getParent();
    auto *parameterMD = F->getMetadata("spirv.ParameterDecorations");

    if (parameterMD) {
      spirvDecorationsMD = cast<MDNode>(parameterMD->getOperand(A->getArgNo()));
    }
  }

  DenseMap<uint64_t, SmallPtrSet<MDNode *, 4>> spirvDecorations;
  if (spirvDecorationsMD) {
    for (const auto &operand : spirvDecorationsMD->operands()) {
      auto node = dyn_cast<MDNode>(operand.get());
      if (node->getNumOperands() == 0) {
        continue;
      }

      if (auto value = dyn_cast<ValueAsMetadata>(node->getOperand(0))) {
        if (auto constantInt = dyn_cast<ConstantInt>(value->getValue())) {
          uint64_t decorationId = constantInt->getZExtValue();

          spirvDecorations[decorationId].insert(node);
        }
      }
    }
  }
  return spirvDecorations;
}

struct CacheControlFromMDNodes {
  int value; // equal to default if (isEmpty || isInvalid)
  bool isEmpty;
  bool isInvalid;
};
template <typename T>
CacheControlFromMDNodes resolveCacheControlFromMDNodes(CodeGenContext *ctx,
                                                       llvm::SmallPtrSetImpl<llvm::MDNode *> &MDNodes) {
  using namespace llvm;
  static_assert(std::is_same_v<T, LoadCacheControl> || std::is_same_v<T, StoreCacheControl>);
  SmallDenseMap<CacheLevel, T> cacheControls = parseCacheControlsMD<T>(MDNodes);
  IGC_ASSERT(!cacheControls.empty());

  // SPV_INTEL_cache_controls extension specification states the following:
  // "Cache Level is an unsigned 32-bit integer telling the cache level to
  //  which the control applies. The value 0 indicates the cache level closest
  //  to the processing unit, the value 1 indicates the next furthest cache
  //  level, etc. If some cache level does not exist, the decoration is ignored."
  //
  // Therefore Cache Level equal to 0 maps to L1$ and Cache Level equal to 1 maps to L3$.
  // Other Cache Level values are ignored.
  const int cacheDefault = getDefaultCacheControlValue<T>(ctx);

  CacheControlFromMDNodes result = {};
  result.value = cacheDefault;

  auto L1CacheControl = getCacheControl(cacheControls, CacheLevel(0));
  auto L3CacheControl = getCacheControl(cacheControls, CacheLevel(1));
  std::optional<T> L2CacheControl = {};
  auto nextCacheControl = getCacheControl(cacheControls, CacheLevel(2));
  if (nextCacheControl) {
    L2CacheControl = L3CacheControl;
    L3CacheControl = nextCacheControl;
  }

  if (!L1CacheControl && !L2CacheControl && !L3CacheControl) {
    // Early exit if there are no cache controls set for cache levels that are controllable
    // by Intel GPUs.
    result.isEmpty = true;
    return result;
  }

  if (ctx->platform.hasNewLSCCacheEncoding() && L1CacheControl && L3CacheControl &&
      (L2CacheControl || cacheDefault > LSC_CC_INVALID)) {
    if (!L2CacheControl) {
      // Case when only cacheDefault from compOpt has new enum encoding value.
      // Workaround to have L1 L3 settings in L1/L2CacheControl.
      L2CacheControl = L3CacheControl;
      L3CacheControl = {};
    }

    SeparateCacheControlsL1L2L3<T> L1L2L3Default;
    if (cacheDefault > LSC_CC_INVALID) {
      L1L2L3Default = mapToSPIRVL1L2L3CacheControl<T>(cacheDefault);
    } else {
      // Parse old legacy L1 L3 default settings
      LSC_L1_L3_CC defaultLSCCacheControls = static_cast<LSC_L1_L3_CC>(cacheDefault);
      auto L1L3DefaultLegacy = mapToSPIRVCacheControl<T>(defaultLSCCacheControls);
      L1L2L3Default.L1 = L1L3DefaultLegacy.L1;
      L1L2L3Default.L2 = L1L3DefaultLegacy.L3;
      L1L2L3Default.L3 = T::Uncached;
      if (std::is_same_v<T, LoadCacheControl>) {
        // Const cached not available, but it corresponds to
        // L2=cached L3=cached in new cache encoding.
        if (L1L2L3Default.L2 == static_cast<T>(LoadCacheControl::ConstCached)) {
          L1L2L3Default.L2 = static_cast<T>(LoadCacheControl::Cached);
          L1L2L3Default.L3 = L1L2L3Default.L2;
        }

        // Only IAR, IAR, IAR allowed.
        if (L1L2L3Default.L2 == static_cast<T>(LoadCacheControl::InvalidateAfterRead)) {
          L1L2L3Default.L3 = L1L2L3Default.L2;
        }
      }
    }
    IGC_ASSERT(L1L2L3Default.L1 != T::Invalid && L1L2L3Default.L2 != T::Invalid && L1L2L3Default.L3 != T::Invalid);

    T newL1CacheControl = L1CacheControl ? L1CacheControl.value() : L1L2L3Default.L1;
    T newL2CacheControl = L2CacheControl ? L2CacheControl.value() : L1L2L3Default.L2;
    T newL3CacheControl = L3CacheControl ? L3CacheControl.value() : L1L2L3Default.L3;

    if (std::is_same_v<T, LoadCacheControl>) {
      // Const cached not available, but it corresponds to
      // L2=cached L3=cached in new cache encoding.
      if (newL2CacheControl == static_cast<T>(LoadCacheControl::ConstCached)) {
        newL2CacheControl = static_cast<T>(LoadCacheControl::Cached);
        newL3CacheControl = newL2CacheControl;
      }

      // Only IAR, IAR, IAR allowed.
      if (newL2CacheControl == static_cast<T>(LoadCacheControl::InvalidateAfterRead)) {
        newL3CacheControl = newL2CacheControl;
      }
    } else {
      // StoreCacheControl
      // Only for 2-level decorations, otherwise we would hide incorrect settings.
      if (!L3CacheControl && newL2CacheControl == static_cast<T>(StoreCacheControl::WriteBack) &&
          (newL1CacheControl == T::Streaming || newL1CacheControl == static_cast<T>(StoreCacheControl::WriteBack))) {
        // When L1 store cache is streaming or writeback,
        // writeback is not available for both L2 and L3 at the same time.
        newL3CacheControl = T::Uncached;
      }
    }

    auto newLSCCacheControl = mapToLSCL1L2L3CacheControl(newL1CacheControl, newL2CacheControl, newL3CacheControl);

    if (newLSCCacheControl == LSC_CC_INVALID) {
      result.isInvalid = true;
      return result;
    }
    result.value = newLSCCacheControl;
    return result;
  }

  LSC_L1_L3_CC defaultLSCCacheControls = static_cast<LSC_L1_L3_CC>(cacheDefault);
  auto L1L3Default = mapToSPIRVCacheControl<T>(defaultLSCCacheControls);
  IGC_ASSERT(L1L3Default.L1 != T::Invalid && L1L3Default.L3 != T::Invalid);

  T newL1CacheControl = L1CacheControl ? L1CacheControl.value() : L1L3Default.L1;
  T newL3CacheControl = L3CacheControl ? L3CacheControl.value() : L1L3Default.L3;

  LSC_L1_L3_CC newLSCCacheControl = mapToLSCCacheControl(newL1CacheControl, newL3CacheControl);

  if (newLSCCacheControl == LSC_CC_INVALID) {
    result.isInvalid = true;
    return result;
  }
  result.value = newLSCCacheControl;
  return result;
}
} // namespace IGC
