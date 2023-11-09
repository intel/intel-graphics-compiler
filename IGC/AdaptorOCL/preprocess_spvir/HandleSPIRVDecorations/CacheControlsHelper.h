/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CISACodeGen/getCacheOpts.h"

namespace IGC
{
    enum class LoadCacheControl
    {
        Uncached = 0,
        Cached = 1,
        Streaming = 2,
        InvalidateAfterRead = 3,
        ConstCached = 4,
        Invalid // This value represents invalid/unsupported cache control value
    };

    enum class StoreCacheControl
    {
        Uncached = 0,
        WriteThrough = 1,
        WriteBack = 2,
        Streaming = 3,
        Invalid // This value represents invalid/unsupported cache control value
    };

    template<typename T>
    using CacheControlMapTy = std::unordered_map<LSC_L1_L3_CC, std::pair<T, T>, std::hash<int>>;

    const CacheControlMapTy<StoreCacheControl> supportedStoreConfigs =
    {
        { LSC_L1UC_L3UC,       {StoreCacheControl::Uncached,     StoreCacheControl::Uncached  } },
        { LSC_L1UC_L3C_WB,     {StoreCacheControl::Uncached,     StoreCacheControl::WriteBack } },
        { LSC_L1C_WT_L3UC,     {StoreCacheControl::WriteThrough, StoreCacheControl::Uncached  } },
        { LSC_L1C_WT_L3C_WB,   {StoreCacheControl::WriteThrough, StoreCacheControl::WriteBack } },
        { LSC_L1S_L3UC,        {StoreCacheControl::Streaming,    StoreCacheControl::Uncached  } },
        { LSC_L1S_L3C_WB,      {StoreCacheControl::Streaming,    StoreCacheControl::WriteBack } },
        { LSC_L1IAR_WB_L3C_WB, {StoreCacheControl::WriteBack,    StoreCacheControl::WriteBack } }
    };

    const CacheControlMapTy<LoadCacheControl> supportedLoadConfigs =
    {
        { LSC_L1UC_L3UC,       { LoadCacheControl::Uncached,            LoadCacheControl::Uncached } },
        { LSC_L1UC_L3C_WB,     { LoadCacheControl::Uncached,            LoadCacheControl::Cached   } },
        { LSC_L1C_WT_L3UC,     { LoadCacheControl::Cached,              LoadCacheControl::Uncached } },
        { LSC_L1C_WT_L3C_WB,   { LoadCacheControl::Cached,              LoadCacheControl::Cached   } },
        { LSC_L1S_L3UC,        { LoadCacheControl::Streaming,           LoadCacheControl::Uncached } },
        { LSC_L1S_L3C_WB,      { LoadCacheControl::Streaming,           LoadCacheControl::Cached   } },
        { LSC_L1IAR_WB_L3C_WB, { LoadCacheControl::InvalidateAfterRead, LoadCacheControl::Cached   } },
        { LSC_L1UC_L3CC,       { LoadCacheControl::Uncached,            LoadCacheControl::ConstCached         } },
        { LSC_L1C_L3CC,        { LoadCacheControl::Cached,              LoadCacheControl::ConstCached         } },
        { LSC_L1IAR_L3IAR,     { LoadCacheControl::InvalidateAfterRead, LoadCacheControl::InvalidateAfterRead } },
    };

    using CacheLevel = uint64_t;

    // This function expects MDNodes to be in the following form:
    // MD =  {  opcode,   cache_level,  cache_control}
    // -----------------------------------------------
    // !8 =  !{i32 6442,    i32 1,          i32 0   }
    // !9 =  !{i32 6442,    i32 3,          i32 0   }
    // !11 = !{i32 6442,    i32 1,          i32 0   }
    // !12 = !{i32 6442,    i32 3,          i32 1   }
    template<typename T>
    llvm::SmallDenseMap<CacheLevel, T> parseCacheControlsMD(llvm::SmallPtrSetImpl<llvm::MDNode*>& MDNodes)
    {
        using namespace llvm;
        auto getLiteral = [](const MDNode* node, const uint32_t index)
        {
            if (auto value = dyn_cast<ValueAsMetadata>(node->getOperand(index)))
                if (auto constantInt = dyn_cast<ConstantInt>(value->getValue()))
                    return std::optional<uint64_t>(constantInt->getZExtValue());

            return std::optional<uint64_t>();
        };

        SmallDenseMap<CacheLevel, T> cacheControls;
        for (auto Node : MDNodes)
        {
            IGC_ASSERT(Node->getNumOperands() == 3);
            IGC_ASSERT(getLiteral(Node, 0).value() == 6442 ||
                       getLiteral(Node, 0).value() == 6443);

            auto cacheLevel = getLiteral(Node, 1);
            auto cacheControl = getLiteral(Node, 2);

            IGC_ASSERT(cacheLevel && cacheControl);
            cacheControls[cacheLevel.value()] =
                static_cast<T>(cacheControl.value());
        }
        return cacheControls;
    }

    template<typename T>
    std::optional<T> getCacheControl(llvm::SmallDenseMap<CacheLevel, T>& cacheControls, CacheLevel level)
    {
        if (auto E = cacheControls.find(level); E != cacheControls.end())
            return E->second;
        return {};
    }

    LSC_L1_L3_CC mapToLSCCacheControl(StoreCacheControl L1Control, StoreCacheControl L3Control)
    {
        for (auto& [LSCEnum, SPIRVEnum] : supportedStoreConfigs)
            if (SPIRVEnum.first == L1Control && SPIRVEnum.second == L3Control)
                return LSCEnum;

        return LSC_CC_INVALID;
    }

    LSC_L1_L3_CC mapToLSCCacheControl(LoadCacheControl L1Control, LoadCacheControl L3Control)
    {
        for (auto& [LSCEnum, SPIRVEnum] : supportedLoadConfigs)
            if (SPIRVEnum.first == L1Control && SPIRVEnum.second == L3Control)
                return LSCEnum;

        return LSC_CC_INVALID;
    }

    template<typename T>
    std::pair<T, T> mapToSPIRVCacheControl(LSC_L1_L3_CC) = delete;

    template <>
    std::pair<LoadCacheControl, LoadCacheControl> mapToSPIRVCacheControl<LoadCacheControl>(LSC_L1_L3_CC LSCControl)
    {
        if (auto I = supportedLoadConfigs.find(LSCControl); I != supportedLoadConfigs.end())
            return I->second;

        IGC_ASSERT_MESSAGE(false, "Unsupported cache controls combination!");
        return { LoadCacheControl::Invalid, LoadCacheControl::Invalid };
    }

    template <>
    std::pair<StoreCacheControl, StoreCacheControl> mapToSPIRVCacheControl<StoreCacheControl>(LSC_L1_L3_CC LSCControl)
    {
        if (auto I = supportedStoreConfigs.find(LSCControl); I != supportedStoreConfigs.end())
            return I->second;

        IGC_ASSERT_MESSAGE(false, "Unsupported cache controls combination!");
        return { StoreCacheControl::Invalid, StoreCacheControl::Invalid };
    }
}
