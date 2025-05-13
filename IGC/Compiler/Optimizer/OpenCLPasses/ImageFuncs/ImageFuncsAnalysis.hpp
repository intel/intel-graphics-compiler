/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class CodeGenContext;
}

namespace IGC
{
    /// @brief  ImageFuncsAnalysis pass used for analyzing which OpenCL image dimension functions
    ///         (height, width, depth) are used in the different functions in the module and creating
    ///         metadata that represents  the implicit image information needded by each function for
    ///         resolving these function calls

    class ImageFuncsAnalysis : public llvm::ModulePass, public llvm::InstVisitor<ImageFuncsAnalysis>
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        ImageFuncsAnalysis();

        /// @brief  Destructor
        ~ImageFuncsAnalysis() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "ImageFuncsAnalysis";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        /// @brief  Main entry point.
        ///         Runs on all functions defined in the gven module, finds all OpenCL relevant image
        ///         function calls, analyzes them and creates metadata that represents the implicit image
        ///          information needded by each function for resolving these function calls
        /// @param  M The destination module.
        virtual bool runOnModule(llvm::Module& M) override;

        /// @brief  Function entry point.
        ///         Finds all OpenCL relevant image dimension function calls in this function, analyzes them
        ///          and creates metadata that represents the implicit image information needded by this function
        ///         for resolving these function calls
        /// @param  F The destination function.
        bool runOnFunction(llvm::Function& F);

        /// @brief  Call instrucitons visitor.
        ///         Checks for OpenCL image dimension functions and anlyzes it
        /// @param  CI The call instruction.
        void visitCallInst(llvm::CallInst& CI);


        // All image functions needed resolved by implicit arguments
        static const llvm::StringRef GET_IMAGE_HEIGHT;
        static const llvm::StringRef GET_IMAGE_WIDTH;
        static const llvm::StringRef GET_IMAGE_DEPTH;
        static const llvm::StringRef GET_IMAGE_NUM_MIP_LEVELS;
        static const llvm::StringRef GET_IMAGE_CHANNEL_DATA_TYPE;
        static const llvm::StringRef GET_IMAGE_CHANNEL_ORDER;
        static const llvm::StringRef GET_IMAGE_SRGB_CHANNEL_ORDER;
        static const llvm::StringRef GET_IMAGE1D_ARRAY_SIZE;
        static const llvm::StringRef GET_IMAGE2D_ARRAY_SIZE;
        static const llvm::StringRef GET_IMAGE_NUM_SAMPLES;
        static const llvm::StringRef GET_SAMPLER_ADDRESS_MODE;
        static const llvm::StringRef GET_SAMPLER_NORMALIZED_COORDS;
        static const llvm::StringRef GET_SAMPLER_SNAP_WA_REQUIRED;
        static const llvm::StringRef GET_FLAT_IMAGE_BASEOFFSET;
        static const llvm::StringRef GET_FLAT_IMAGE_HEIGHT;
        static const llvm::StringRef GET_FLAT_IMAGE_WIDTH;
        static const llvm::StringRef GET_FLAT_IMAGE_PITCH;
    private:

        /// @brief Maps each implicit argument type to a set. This set contains
        ///        all function arguments (Value*) that belong to this type.
        ///        For many argument types this set will contain only a single Value*
        ///        (e.g. there is only one payload header), but for others, it will
        ///        contain many values (e.g. there are many IMAGE_WIDTHs, potentially
        ///        one for each image).
        ImplicitArg::ArgMap m_argMap;
        /// @brief  MetaData utils used to generate LLVM metadata
        IGCMD::MetaDataUtils* m_pMDUtils = nullptr;

        /// @brief Indicate whether advanced bindless mode is used.
        ///        If false, implicit image args for information like width, height, etc.
        ///        will be added to m_argMap
        bool m_useAdvancedBindlessMode{};

        // @brief Handling of image/sampler properties query functions varies based on image type.
        //
        //        This distinction arises between standard OpenCL images and bindless images
        //        from the SPV_INTEL_bindless_images extension.
        //
        //        For example: the __builtin_IB_get_snap_wa_reqd function is lowered to an
        //        implicit argument, SAMPLER_SNAP_WA, for standard OpenCL images.
        //        However, for bindless images from the SPV_INTEL_bindless_images extension,
        //        the snap_wa is unsupported. Consequently, it is effectively disabled
        //        by being lowered to a ConstantInt value of 0.
        bool m_useSPVINTELBindlessImages{};

        bool m_useBindlessImageWithSamplerTracking{};
        int m_inlineSamplerIndex = 0;
    };

} // namespace IGC

