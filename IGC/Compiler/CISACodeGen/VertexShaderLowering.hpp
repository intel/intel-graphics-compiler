/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CISACodeGen/ShaderUnits.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/MetaDataUtilsWrapper.h"

#include <llvm/IR/PassManager.h>

namespace IGC
{
    class VertexShaderProperties
    {
    public:
        VertexShaderProperties();
        // Maximum used input vertex element component +1.
        unsigned int m_MaxUsedInputSlots;
        bool m_HasVertexID;
        unsigned int m_VID;
        bool m_HasInstanceID;
        unsigned int m_IID;
        SVertexFetchSGVExtendedParameters m_VertexFetchSGVExtendedParameters;
        bool m_hasRTAI;
        bool m_hasVPAI;
        bool m_hasClipDistance;
        QuadEltUnit  m_URBOutputLength;
    };

    class CollectVertexShaderProperties : public llvm::ImmutablePass
    {
    public:
        CollectVertexShaderProperties();
        static char ID;
        void SetInputSlotUsed(unsigned int slot);
        void SetVertexIdSlot(unsigned int VIDslot);
        void SetInstanceIdSlot(unsigned int IIDslot);
        void SetShaderDrawParameter(size_t paramIndex, unsigned int slot);
        void DeclareRTAI();
        void DeclareVPAI();
        void DeclareClipDistance();
        void DeclareOutput(QuadEltUnit newMaxOffset);

        const VertexShaderProperties& GetProperties() { return m_vsProps; }
    protected:
        VertexShaderProperties m_vsProps;
    };

    const unsigned int MaxNumOfOutput = 32;

    class VertexShaderLowering : public llvm::FunctionPass
    {
    public:
        VertexShaderLowering();
        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CollectVertexShaderProperties>();
            AU.addRequired<CodeGenContextWrapper>();
        }
        static char         ID;
    private:

        void LowerIntrinsicInputOutput(llvm::Function& F);
        bool determineIfInstructionUsingInstanceIDisConstantBuffer(
            llvm::Value* user);

        /// Adds a new URB Write instruction to the instruction list
        /// \param offset - indicates the offset in URB at which the write will be performed
        /// \param mask   - 8-bit bitfield indicating which dwords of parameter data should be written
        ///                 0x01 means data[0] only, 0x0F means dwords data[0]..data[3],
        ///                 0xFF means data[0]..data[7]
        /// \param data   - array of 8 values that are data to be written to URB
        /// \param prev   - new urb write instruction will be added to instruction list before prev.
        void AddURBWrite(llvm::Value* offset, uint mask, llvm::Value* data[8], llvm::Instruction* prev);
        void AddURBRead(llvm::Value* index, llvm::Value* offset, llvm::Instruction* prev);
        unsigned int GetURBOffset(ShaderOutputType type, llvm::Value* attribute, llvm::Instruction* inst);
        void CalculateVertexHeaderSize(llvm::Function& F);
        unsigned int InsertInEmptySlot(llvm::Instruction* sgv, bool bInsertAfterLastUsedSlot = false);
        void InsertSGVRead(llvm::Instruction* sgv, uint index);
        unsigned int GetUnusedInputSlot();
        unsigned int GetUnusedInputSlotAFterLastUsedOne();
        //HACK: to remove once we don't need header anymore
        void AddInitializedHeader(llvm::Instruction* prev);


    private:
        // Maximum number of input vertex elements. Limited by 3DSTATE_VF.
        static const unsigned int MaxNumOfUserInputs = 32; // in 4*DWORD units

        // Maximum number of inputs including SGVs. SGVs can be located in URB past
        // the number of available vertex elements.
        // Currently additional 2 inputs is enough for all SGVs.
        static const unsigned int MaxNumOfInputs = MaxNumOfUserInputs + 2; // in 4*DWORD units

        CollectVertexShaderProperties* m_vsPropsPass;
        llvm::Module* m_module;
        CodeGenContext* m_context;
        QuadEltUnit m_headerSize;
        bool m_inputUsed[MaxNumOfInputs * 4]; // used vertex elements and SGV slots
        bool m_isHeaderPresent;
    };
    void initializeVertexShaderLoweringPass(llvm::PassRegistry&);
}//namespace IGC
