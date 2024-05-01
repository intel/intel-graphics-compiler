/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
// A helper class that returns a mapping from message generating
// intrinsic (e.g. sample, load, urb_write) arguments to their respective
// positions in the payload message.
//
//===----------------------------------------------------------------------===//

#pragma once
#include "Compiler/CISACodeGen/Platform.hpp"
#include "Compiler/CodeGenPublic.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/IntrinsicInst.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "common/Types.hpp"
#include "Probe/Assertion.h"

namespace IGC
{

    class PayloadMapping
    {
        friend class CoalescingEngine;
    public:
        void ValidateNumberofSources(EOPCODE opCode, bool isCube, uint& numberofSrcs);

        typedef llvm::DenseMap<std::pair<const llvm::Instruction*, uint>, llvm::Value*> PayloadMappingCache;

    protected:
        PayloadMapping() : m_CodeGenContext(nullptr) {}
        PayloadMapping(CodeGenContext* ctx) : m_CodeGenContext(ctx) {}

        /// ------------------------------
        uint GetNumPayloadElements(const llvm::Instruction* inst);
        uint GetNumPayloadElements_URBWrite(const llvm::GenIntrinsicInst* inst);
        uint GetNumPayloadElements_RTWrite(const llvm::GenIntrinsicInst* inst);
        uint GetNumPayloadElements_DSRTWrite(const llvm::GenIntrinsicInst* inst);
        uint GetNumPayloadElements_LDMS(const llvm::GenIntrinsicInst* inst);
        uint GetNonAdjustedNumPayloadElements_Sample(const llvm::SampleIntrinsic* inst);
        uint GetNumPayloadElements_Sample(const llvm::SampleIntrinsic* inst);

        /// ------------------------------
        /// \brief Get the mapping from the payload element (at position index)
        /// to intrinsic argument value. Indexing is zero based.
        llvm::Value* GetPayloadElementToValueMapping(const llvm::Instruction* inst, uint index);
        llvm::Value* GetPayloadElementToValueMapping_URBWrite(const llvm::GenIntrinsicInst* inst, uint index);
        llvm::Value* GetPayloadElementToValueMapping_RTWrite(const llvm::GenIntrinsicInst* inst, const uint index);
        llvm::Value* GetPayloadElementToValueMapping_DSRTWrite(const llvm::GenIntrinsicInst* inst, const uint index);
        uint GetNonAdjustedPayloadElementIndexToValueIndexMapping_sample(const llvm::SampleIntrinsic* inst, uint index);
        llvm::Value* GetPayloadElementToValueMapping_LDMS(const llvm::SamplerLoadIntrinsic* inst, const uint index);
        llvm::Value* GetNonAdjustedPayloadElementToValueMapping_sample(const llvm::SampleIntrinsic* inst, const uint index);
        llvm::Value* GetPayloadElementToValueMapping_sample(const llvm::SampleIntrinsic* inst, const uint index);

        //Handling of non-homogeneous payloads (RT write)
        const llvm::Instruction* GetSupremumOfNonHomogeneousPart(
            const llvm::Instruction* inst1,
            const llvm::Instruction* inst2);
        int GetRightReservedOffset(const llvm::Instruction* inst, SIMDMode simdMode);
        int GetLeftReservedOffset(const llvm::Instruction* inst, SIMDMode simdMode);
        bool HasNonHomogeneousPayloadElements(const llvm::Instruction* inst);
        const llvm::Instruction* GetSupremumOfNonHomogeneousPart_RTWrite(
            const llvm::RTWriteIntrinsic* inst1,
            const llvm::RTWriteIntrinsic* inst2);
        template <typename T>
        int GetLeftReservedOffset_RTWrite(const T* inst, SIMDMode simdMode);
        template <typename T>
        int GetRightReservedOffset_RTWrite(const T* inst, SIMDMode simdMode);
        template <typename T>
        bool HasNonHomogeneousPayloadElements_RTWrite(const T* inst);

        /// ------------------------------
        bool IsUndefOrZeroImmediate(const llvm::Value* value);
        bool IsZeroLOD(const llvm::SampleIntrinsic* inst);
        bool DoPeelFirstElement(const llvm::Instruction* inst);

        bool DoesAllowSplit(const llvm::Instruction* inst)
        {
            IGC_ASSERT(llvm::isa<llvm::GenIntrinsicInst>(inst));
            if (llvm::dyn_cast<llvm::SampleIntrinsic>(inst))
            {
                return true;
            }
            return false;
        }

    private:
        CodeGenContext* m_CodeGenContext;
        PayloadMappingCache m_PayloadMappingCache;

    };

} //End namespace IGC
