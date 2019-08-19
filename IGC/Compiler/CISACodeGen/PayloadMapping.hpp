/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

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

#define  GENX_GRF_REG_SIZ          32  // # of bytes in a GRF register

namespace IGC
{

    class PayloadMapping
    {
        friend class CoalescingEngine;
    public:
        void ValidateNumberofSources(EOPCODE opCode, uint& numberofSrcs);

        typedef llvm::DenseMap<std::pair<const llvm::Instruction*, uint>, llvm::Value*> PayloadMappingCache;

    protected:
        PayloadMapping() : m_CodeGenContext(nullptr) {}
        PayloadMapping(CodeGenContext* ctx) : m_CodeGenContext(ctx) {}

        /// ------------------------------
        uint GetNumPayloadElements(const llvm::Instruction* inst);
        uint GetNumPayloadElements_URBWrite(const llvm::GenIntrinsicInst* inst);
        uint GetNumPayloadElements_RTWrite(const llvm::GenIntrinsicInst* inst);
        uint GetNumPayloadElements_LDMS(const llvm::GenIntrinsicInst* inst);
        uint GetNonAdjustedNumPayloadElements_Sample(const llvm::SampleIntrinsic* inst);
        uint GetNumPayloadElements_Sample(const llvm::SampleIntrinsic* inst);

        /// ------------------------------
        /// \brief Get the mapping from the payload element (at position index)
        /// to intrinsic argument value. Indexing is zero based.
        llvm::Value* GetPayloadElementToValueMapping(const llvm::Instruction* inst, uint index);
        llvm::Value* GetPayloadElementToValueMapping_URBWrite(const llvm::GenIntrinsicInst* inst, uint index);
        llvm::Value* GetPayloadElementToValueMapping_RTWrite(const llvm::GenIntrinsicInst* inst, const uint index);
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
            const llvm::Instruction* inst1,
            const llvm::Instruction* inst2);
        int GetLeftReservedOffset_RTWrite(const llvm::Instruction* inst, SIMDMode simdMode);
        int GetRightReservedOffset_RTWrite(const llvm::Instruction* inst, SIMDMode simdMode);
        bool HasNonHomogeneousPayloadElements_RTWrite(const llvm::Instruction* inst);

        /// ------------------------------
        bool IsUndefOrZeroImmediate(const llvm::Value* value);
        bool IsZeroLOD(const llvm::SampleIntrinsic* inst);
        bool DoPeelFirstElement(const llvm::Instruction* inst);

        bool DoesAllowSplit(const llvm::Instruction* inst)
        {
            const llvm::GenIntrinsicInst* intrinsicInst = llvm::dyn_cast<llvm::GenIntrinsicInst>(inst);
            assert(intrinsicInst);
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
