/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/ExtensionFuncs/ExtensionArgAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/Optimizer/OCLBIUtils.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-extension-arg-analysis"
#define PASS_DESCRIPTION "Analyzes extension functions arguments"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(ExtensionArgAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(ExtensionArgAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

namespace IGC
{

    char ExtensionArgAnalysis::ID = 0;

    // VME Functions:

    enum VME_FUNCTIONS
    {
        VME_FUNCTION,
        ADVANCED_VME_FUNCTION,
        ADVANCED_VME_BIDIR_FUNCTION,
        NUM_VME_FUNCTIONS
    };

    static const llvm::StringRef VME_FUNCTION_STRINGS[] =
    {
        "block_motion_estimate_intel",
        "block_advanced_motion_estimate_check_intel",
        "block_advanced_motion_estimate_bidirectional_check_intel"
    };

    static_assert(
        sizeof(VME_FUNCTION_STRINGS) / sizeof(*VME_FUNCTION_STRINGS) == NUM_VME_FUNCTIONS,
        "VME_FUNCTION_STRINGS array needs to be in sync with VME_FUNCTIONS enum, fix me!");

    // VA functions:

    enum VA_FUNCTIONS
    {
        VA_FUNCTION_ERODE,
        VA_FUNCTION_DILATE,
        VA_FUNCTION_MIN_FILTER,
        VA_FUNCTION_MAX_FILTER,
        VA_FUNCTION_CONVOLVE,
        VA_FUNCTION_MINMAX,
        VA_FUNCTION_CENTROID,
        VA_FUNCTION_BOOL_CENTROID,
        VA_FUNCTION_BOOL_SUM,
        VA_FUNCTION_CONVOLVE_1D,
        VA_FUNCTION_CONVOLVE_PIXEL,
        VA_FUNCTION_LBP_IMAGE,
        VA_FUNCTION_LBP_CORRELATION,
        VA_FUNCTION_FLOODFILL,
        VA_FUNCTION_CORRELATION_SEARCH,
        NUM_VA_FUNCTIONS
    };

    static const llvm::StringRef VA_FUNCTION_STRINGS[] =
    {
        "erode_2d_intel",
        "dilate_2d_intel",
        "min_filter_2d_intel",
        "max_filter_2d_intel",
        "convolve_2d_intel",
        "minmax_2d_intel",
        "centroid_2d_intel",
        "bool_centroid_2d_intel",
        "bool_sum_2d_intel",
        "convolve_1d_intel",
        "convolve_pixel_intel",
        "lbp_image_intel",
        "lbp_correlation_intel",
        "floodfill_intel",
        "correlation_search_intel",
    };
    static_assert(
        sizeof(VA_FUNCTION_STRINGS) / sizeof(*VA_FUNCTION_STRINGS) == NUM_VA_FUNCTIONS,
        "VA_FUNCTION_STRINGS array needs to be in sync with VA_FUNCTIONS enum, fix me!");

    const ResourceExtensionTypeEnum VA_FUNCTION_SAMPLER_TYPES[] =
    {
        ResourceExtensionTypeEnum::MediaSamplerTypeErode,
        ResourceExtensionTypeEnum::MediaSamplerTypeDilate,
        ResourceExtensionTypeEnum::MediaSamplerTypeMinMaxFilter,
        ResourceExtensionTypeEnum::MediaSamplerTypeMinMaxFilter,
        ResourceExtensionTypeEnum::MediaSamplerTypeConvolve,
        ResourceExtensionTypeEnum::MediaSamplerTypeMinMax,
        ResourceExtensionTypeEnum::MediaSamplerTypeCentroid,
        ResourceExtensionTypeEnum::MediaSamplerTypeBoolCentroid,
        ResourceExtensionTypeEnum::MediaSamplerTypeBoolSum,
        // SKL+ functions:
        ResourceExtensionTypeEnum::MediaSamplerTypeConvolve,
        ResourceExtensionTypeEnum::MediaSamplerTypeConvolve,
        ResourceExtensionTypeEnum::MediaSamplerTypeLbp,
        ResourceExtensionTypeEnum::MediaSamplerTypeLbp,
        ResourceExtensionTypeEnum::MediaSamplerTypeFloodFill,
        ResourceExtensionTypeEnum::MediaSamplerTypeCorrelation,
    };
    static_assert(
        sizeof(VA_FUNCTION_SAMPLER_TYPES) / sizeof(*VA_FUNCTION_SAMPLER_TYPES) == NUM_VA_FUNCTIONS,
        "Sampler mapping array needs to be in sync with VA_FUNCTIONS enum, fix me!");

    ExtensionArgAnalysis::ExtensionArgAnalysis() : FunctionPass(ID)
    {
        m_extensionType = ResourceExtensionTypeEnum::NonExtensionType;

        initializeExtensionArgAnalysisPass(*PassRegistry::getPassRegistry());
    }

    void ExtensionArgAnalysis::visitCallInst(llvm::CallInst& CI)
    {
        auto* F = CI.getCalledFunction();

        if (!F)
        {
            return;
        }

        auto SetExtension = [&](int argIndex, ResourceExtensionTypeEnum expected, SmallPtrSet<Argument*, 3> & Args)
        {
            if (auto * pArg = dyn_cast<Argument>(ValueTracker::track(&CI, argIndex))) {
                if (m_ExtensionMap.count(pArg) != 0)
                {
                    if (m_ExtensionMap[pArg] != expected)
                    {
                        getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->EmitError("Inconsistent use of image!", &CI);
                        return;
                    }
                }
                else
                {
                    m_ExtensionMap[pArg] = expected;
                    Args.insert(pArg);
                }
            }
        };

        //
        // If kernel has device-side VME built-ins, its simd size has to be 16 given that
        // those built-ins only work for SIMD16 kernels.
        //
        auto CheckandSetSIMD16 = [&]()
        {
            if (IGC::IGCMD::MetaDataUtils * pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils())
            {
                auto funcIter = pMdUtils->findFunctionsInfoItem(CI.getParent()->getParent());
                if (funcIter != pMdUtils->end_FunctionsInfo())
                {
                    IGC::IGCMD::SubGroupSizeMetaDataHandle subGroupSize = funcIter->second->getSubGroupSize();
                    if (subGroupSize->hasValue())
                    {
                        if (subGroupSize->getSIMD_size() != 16)
                            getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->EmitError("SIMD16 is expected", &CI);
                    }
                    else
                        subGroupSize->setSIMD_size(16);
                }
            }
        };


        StringRef name = F->getName();

        if (name.startswith("__builtin_IB_media_block_") || name == "__builtin_IB_media_block_rectangle_read")
        {
            SetExtension(0, ResourceExtensionTypeEnum::MediaResourceBlockType, m_MediaBlockArgs);
        }
        else if (name == "__builtin_IB_vme_send_fbr" ||
            name == "__builtin_IB_vme_send_ime")
        {
            SetExtension(3, ResourceExtensionTypeEnum::MediaResourceType, m_MediaArgs);
            SetExtension(4, ResourceExtensionTypeEnum::MediaResourceType, m_MediaArgs);
            CheckandSetSIMD16();
        }
        else if (name == "__builtin_IB_vme_send_sic")
        {
            SetExtension(3, ResourceExtensionTypeEnum::MediaResourceType, m_MediaArgs);
            SetExtension(4, ResourceExtensionTypeEnum::MediaResourceType, m_MediaArgs);
            SetExtension(5, ResourceExtensionTypeEnum::MediaResourceType, m_MediaArgs);
            CheckandSetSIMD16();
        }
        else if (name.startswith("__builtin_IB_vme_send_ime_new") ||
            name == "__builtin_IB_vme_send_sic_new" ||
            name == "__builtin_IB_vme_send_fbr_new")
        {
            // Handle image args.
            SetExtension(1, ResourceExtensionTypeEnum::MediaResourceType, m_MediaArgs);
            SetExtension(2, ResourceExtensionTypeEnum::MediaResourceType, m_MediaArgs);
            SetExtension(3, ResourceExtensionTypeEnum::MediaResourceType, m_MediaArgs);
            // Handle sampler arg.
            SetExtension(4, ResourceExtensionTypeEnum::MediaSamplerType, m_MediaSamplerArgs);
            CheckandSetSIMD16();
        }

    }

    bool ExtensionArgAnalysis::runOnFunction(Function& F)
    {
        m_ExtensionMap.clear();
        m_MediaArgs.clear();
        m_MediaBlockArgs.clear();
        m_extensionType = ResourceExtensionTypeEnum::NonExtensionType;
        m_vaArgs.clear();
        m_MediaSamplerArgs.clear();
        visit(F);

        StringRef funcName = F.getName();

        if (funcName == VME_FUNCTION_STRINGS[VME_FUNCTION] ||
            funcName == VME_FUNCTION_STRINGS[ADVANCED_VME_FUNCTION] ||
            funcName == VME_FUNCTION_STRINGS[ADVANCED_VME_BIDIR_FUNCTION])
        {
            // First function arg is the sampler
            auto arg = F.arg_begin();
            m_MediaSamplerArgs.insert(&(*arg));
        }

        m_extensionType = ResourceExtensionTypeEnum::NonExtensionType;

        for (int func = VA_FUNCTION_ERODE; func < NUM_VA_FUNCTIONS; ++func)
        {
            if (funcName.equals(VA_FUNCTION_STRINGS[func]))
            {
                // First function arg is the src image, second is the sampler,
                // and third arg is output buffer (ignored by this analysis).
                auto arg = F.arg_begin();
                for (int i = 0; i < 2; ++i)
                {
                    m_vaArgs.insert(&(*arg));
                    arg++;
                }

                m_extensionType = VA_FUNCTION_SAMPLER_TYPES[func];
            }
        }

        return false;
    }

} // namespace IGC
