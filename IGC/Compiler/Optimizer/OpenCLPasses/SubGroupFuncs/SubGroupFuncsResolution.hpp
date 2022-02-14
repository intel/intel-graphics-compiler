/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include <llvm/IR/DataLayout.h>
#include "Compiler/Optimizer/OCLBIUtils.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"
#include "IGC/common/Types.hpp"

namespace IGC
{

    /// @brief  SubGroupFuncsResolution pass used for resolving OpenCL Sub Group functions.
    class SubGroupFuncsResolution : public llvm::FunctionPass, public llvm::InstVisitor<SubGroupFuncsResolution, void>
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        SubGroupFuncsResolution();

        /// @brief  Destructor
        ~SubGroupFuncsResolution() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "SubGroupFuncsResolution";
        }

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
        }

        /// @brief  Main entry point.
        ///         Finds all OpenCL Sub Group function calls and resolve them into an llvm sequence
        /// @param  F The destination function.
        virtual bool runOnFunction(llvm::Function& F) override;

        /// @brief  Call instructions visitor.
        ///         Checks for OpenCL Sub Group  functions and resolves them into appropriate sequence of code
        /// @param  CI The call instruction.
        void visitCallInst(llvm::CallInst& CI);

        void BTIHelper(llvm::CallInst& CI);

        void mediaBlockRead(llvm::CallInst& CI);
        void mediaBlockWrite(llvm::CallInst& CI);

        void simdBlockRead(llvm::CallInst& CI);
        void simdBlockWrite(llvm::CallInst& CI);

        void pushMediaBlockArgs(llvm::SmallVector<llvm::Value*, 5> & args, llvm::CallInst& CI);

        void CheckMediaBlockInstError(llvm::GenIntrinsicInst* inst, bool isRead);

        void subGroupArithmetic(llvm::CallInst& CI, WaveOps op, GroupOpType groupType);
        void subGroup2DBlockRead(llvm::CallInst& CI, llvm::StringRef funcName);

        static const llvm::StringRef SUB_GROUP_BARRIER;
        static const llvm::StringRef GET_MAX_SUB_GROUP_SIZE;
        static const llvm::StringRef GET_SUB_GROUP_LOCAL_ID;
        static const llvm::StringRef SUB_GROUP_SHUFFLE;
        static const llvm::StringRef SUB_GROUP_SHUFFLE_US;
        static const llvm::StringRef SUB_GROUP_SHUFFLE_F;
        static const llvm::StringRef SUB_GROUP_SHUFFLE_H;
        static const llvm::StringRef SUB_GROUP_SHUFFLE_C;
        static const llvm::StringRef SUB_GROUP_SHUFFLE_B;
        static const llvm::StringRef SUB_GROUP_SHUFFLE_DF;
        static const llvm::StringRef SUB_GROUP_SHUFFLE_DOWN;
        static const llvm::StringRef SUB_GROUP_SHUFFLE_DOWN_US;
        static const llvm::StringRef SUB_GROUP_SHUFFLE_DOWN_UC;

        static const llvm::StringRef SIMD_BLOCK_READ_1_GBL;
        static const llvm::StringRef SIMD_BLOCK_READ_2_GBL;
        static const llvm::StringRef SIMD_BLOCK_READ_4_GBL;
        static const llvm::StringRef SIMD_BLOCK_READ_8_GBL;
        static const llvm::StringRef SIMD_BLOCK_READ_1_GBL_H;
        static const llvm::StringRef SIMD_BLOCK_READ_2_GBL_H;
        static const llvm::StringRef SIMD_BLOCK_READ_4_GBL_H;
        static const llvm::StringRef SIMD_BLOCK_READ_8_GBL_H;
        static const llvm::StringRef SIMD_BLOCK_READ_16_GBL_H;
        static const llvm::StringRef SIMD_BLOCK_READ_1_GBL_B;
        static const llvm::StringRef SIMD_BLOCK_READ_2_GBL_B;
        static const llvm::StringRef SIMD_BLOCK_READ_4_GBL_B;
        static const llvm::StringRef SIMD_BLOCK_READ_8_GBL_B;
        static const llvm::StringRef SIMD_BLOCK_READ_16_GBL_B;
        static const llvm::StringRef SIMD_BLOCK_READ_1_GBL_L;
        static const llvm::StringRef SIMD_BLOCK_READ_2_GBL_L;
        static const llvm::StringRef SIMD_BLOCK_READ_4_GBL_L;
        static const llvm::StringRef SIMD_BLOCK_READ_8_GBL_L;

        static const llvm::StringRef SIMD_BLOCK_WRITE_1_GBL;
        static const llvm::StringRef SIMD_BLOCK_WRITE_2_GBL;
        static const llvm::StringRef SIMD_BLOCK_WRITE_4_GBL;
        static const llvm::StringRef SIMD_BLOCK_WRITE_8_GBL;
        static const llvm::StringRef SIMD_BLOCK_WRITE_1_GBL_H;
        static const llvm::StringRef SIMD_BLOCK_WRITE_2_GBL_H;
        static const llvm::StringRef SIMD_BLOCK_WRITE_4_GBL_H;
        static const llvm::StringRef SIMD_BLOCK_WRITE_8_GBL_H;
        static const llvm::StringRef SIMD_BLOCK_WRITE_16_GBL_H;
        static const llvm::StringRef SIMD_BLOCK_WRITE_1_GBL_B;
        static const llvm::StringRef SIMD_BLOCK_WRITE_2_GBL_B;
        static const llvm::StringRef SIMD_BLOCK_WRITE_4_GBL_B;
        static const llvm::StringRef SIMD_BLOCK_WRITE_8_GBL_B;
        static const llvm::StringRef SIMD_BLOCK_WRITE_16_GBL_B;
        static const llvm::StringRef SIMD_BLOCK_WRITE_1_GBL_L;
        static const llvm::StringRef SIMD_BLOCK_WRITE_2_GBL_L;
        static const llvm::StringRef SIMD_BLOCK_WRITE_4_GBL_L;
        static const llvm::StringRef SIMD_BLOCK_WRITE_8_GBL_L;

        static const llvm::StringRef SIMD_BLOCK_READ_1_LCL;
        static const llvm::StringRef SIMD_BLOCK_READ_2_LCL;
        static const llvm::StringRef SIMD_BLOCK_READ_4_LCL;
        static const llvm::StringRef SIMD_BLOCK_READ_8_LCL;
        static const llvm::StringRef SIMD_BLOCK_READ_1_LCL_H;
        static const llvm::StringRef SIMD_BLOCK_READ_2_LCL_H;
        static const llvm::StringRef SIMD_BLOCK_READ_4_LCL_H;
        static const llvm::StringRef SIMD_BLOCK_READ_8_LCL_H;
        static const llvm::StringRef SIMD_BLOCK_READ_16_LCL_H;
        static const llvm::StringRef SIMD_BLOCK_READ_1_LCL_B;
        static const llvm::StringRef SIMD_BLOCK_READ_2_LCL_B;
        static const llvm::StringRef SIMD_BLOCK_READ_4_LCL_B;
        static const llvm::StringRef SIMD_BLOCK_READ_8_LCL_B;
        static const llvm::StringRef SIMD_BLOCK_READ_16_LCL_B;
        static const llvm::StringRef SIMD_BLOCK_READ_1_LCL_L;
        static const llvm::StringRef SIMD_BLOCK_READ_2_LCL_L;
        static const llvm::StringRef SIMD_BLOCK_READ_4_LCL_L;
        static const llvm::StringRef SIMD_BLOCK_READ_8_LCL_L;
        static const llvm::StringRef SIMD_BLOCK_WRITE_1_LCL;
        static const llvm::StringRef SIMD_BLOCK_WRITE_2_LCL;
        static const llvm::StringRef SIMD_BLOCK_WRITE_4_LCL;
        static const llvm::StringRef SIMD_BLOCK_WRITE_8_LCL;
        static const llvm::StringRef SIMD_BLOCK_WRITE_1_LCL_H;
        static const llvm::StringRef SIMD_BLOCK_WRITE_2_LCL_H;
        static const llvm::StringRef SIMD_BLOCK_WRITE_4_LCL_H;
        static const llvm::StringRef SIMD_BLOCK_WRITE_8_LCL_H;
        static const llvm::StringRef SIMD_BLOCK_WRITE_16_LCL_H;
        static const llvm::StringRef SIMD_BLOCK_WRITE_1_LCL_B;
        static const llvm::StringRef SIMD_BLOCK_WRITE_2_LCL_B;
        static const llvm::StringRef SIMD_BLOCK_WRITE_4_LCL_B;
        static const llvm::StringRef SIMD_BLOCK_WRITE_8_LCL_B;
        static const llvm::StringRef SIMD_BLOCK_WRITE_16_LCL_B;
        static const llvm::StringRef SIMD_BLOCK_WRITE_1_LCL_L;
        static const llvm::StringRef SIMD_BLOCK_WRITE_2_LCL_L;
        static const llvm::StringRef SIMD_BLOCK_WRITE_4_LCL_L;
        static const llvm::StringRef SIMD_BLOCK_WRITE_8_LCL_L;

        static const llvm::StringRef SIMD_MEDIA_BLOCK_READ_1;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_READ_2;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_READ_4;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_READ_8;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_READ_1_H;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_READ_2_H;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_READ_4_H;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_READ_8_H;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_READ_16_H;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_READ_1_B;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_READ_2_B;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_READ_4_B;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_READ_8_B;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_READ_16_B;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_READ_1_L;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_READ_2_L;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_READ_4_L;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_READ_8_L;

        static const llvm::StringRef SIMD_MEDIA_BLOCK_WRITE_1;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_WRITE_2;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_WRITE_4;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_WRITE_8;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_WRITE_1_H;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_WRITE_2_H;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_WRITE_4_H;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_WRITE_8_H;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_WRITE_16_H;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_WRITE_1_L;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_WRITE_2_L;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_WRITE_4_L;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_WRITE_8_L;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_WRITE_1_B;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_WRITE_2_B;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_WRITE_4_B;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_WRITE_8_B;
        static const llvm::StringRef SIMD_MEDIA_BLOCK_WRITE_16_B;

        static const llvm::StringRef MEDIA_BLOCK_READ;
        static const llvm::StringRef MEDIA_BLOCK_WRITE;

        static const llvm::StringRef MEDIA_BLOCK_RECTANGLE_READ;

        static const llvm::StringRef GET_IMAGE_BTI;

        static const llvm::StringRef SUB_GROUP_REDUCE;
        static const llvm::StringRef SUB_GROUP_SCAN;
        static const llvm::StringRef SUB_GROUP_CLUSTERED_REDUCE;

        static const llvm::StringRef SUBGROUP_BLOCK_READ;

    private:
        /// @brief  Container for instructions to be deleted after visiting a function.
        llvm::SmallVector<llvm::Instruction*, 16>  m_instsToDelete;

        /// @brief - maps image and sampler kernel parameters to BTIs
        CImagesBI::ParamMap m_argIndexMap;

        // @brief - maps SPIR-V operation to vISA operation type
        static const std::array<std::pair<std::string, WaveOps>, 13> m_spvOpToWaveOpMap;

        /// @brief  Indicates if the pass changed the processed function
        bool m_changed;

        CodeGenContext* m_pCtx;

        /// @brief examine metadata for intel_reqd_sub_group_size
        int32_t GetSIMDSize(llvm::Function* F);

        /// @brief emits the given error message in SIMD32.  Used on subgroup functions
        // that aren't currently supported in SIMD32.
        void CheckSIMDSize(llvm::Instruction& I, llvm::StringRef msg);

        // @brief parse function name to extract spir-v operation type
        //        and map it to vISA operation type
        WaveOps GetWaveOp(llvm::StringRef funcName);
    };

} // namespace IGC

