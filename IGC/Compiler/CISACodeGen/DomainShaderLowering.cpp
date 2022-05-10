/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/ShaderUnits.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/DomainShaderLowering.hpp"
#include "DomainShaderLowering.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/IGCIRBuilder.h"
#include "Probe/Assertion.h"

using namespace IGC;
using namespace IGC::IGCMD;
using namespace llvm;

namespace IGC
{

    class DomainShaderLowering : public llvm::FunctionPass
    {
    public:
        DomainShaderLowering();

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CollectDomainShaderProperties>();
            AU.addRequired<CodeGenContextWrapper>();
        }
        static char ID;
    private:
        void LowerIntrinsicInputOutput(llvm::Function& F);

        void CalculateVertexHeaderSize(Function& F);

        void AddURBRead(Value* index, Value* offset, Instruction* prev);

        /// Replaces calls to OUTPUT with appropriately computed URBWrites.
        void lowerOutput(GenIntrinsicInst* inst, std::vector<llvm::Instruction*>& offsetInst);

        /// Returns the offset in URB of the attribute with attribute index
        /// and output type defined by the parameters.
        QuadEltUnit GetURBOffset(ShaderOutputType outputType, uint attrIdx);


        /// Inserts new URBWrite instruction with given mask and arguments before
        /// instuction 'prev'.
        /// TODO: This should be a common function for all Lowering passes.
        void LowerToInputVecOrURBReadIntrinsic(Value* index, Value* offset, Instruction* prev);
        void AddInputVec(Instruction* prev, uint32_t currentElementIndex);

        void AddURBWrite(
            Value* offset,
            unsigned int mask,
            Value* data[8],
            Instruction* prev);
        /// Sets up mask and data values depending on the instruction's output type.
        /// TODO: This should be a common function for all Lowering passes.
        void SetMaskAndData(
            llvm::Instruction* inst,
            unsigned int& mask,
            llvm::Value* data[8]);

        static const unsigned int m_maxNumOfOutput;
        llvm::Module* m_pModule;    //< remembers the llvm module of this program
        QuadEltUnit    m_headerSize; //< size of the vertex header, always 2 for DS
        const uint32_t m_pMaxNumOfPushedInputs;
        CollectDomainShaderProperties* m_dsPropsPass;
    };

#define PASS_FLAG "igc-collect-domain-shader-properties"
#define PASS_DESCRIPTION "Collect information related to domain shader"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
    IGC_INITIALIZE_PASS_BEGIN(CollectDomainShaderProperties, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_END(CollectDomainShaderProperties, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

        //undef marcros to avoid redefinition warnings
#undef PASS_FLAG
#undef PASS_DESCRIPTION
#undef PASS_ANALYSIS

#define PASS_FLAG "igc-domain-shader-lowering"
#define PASS_DESCRIPTION "Lower inputs outputs for domain shader"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
        IGC_INITIALIZE_PASS_BEGIN(DomainShaderLowering, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_DEPENDENCY(CollectDomainShaderProperties)
        IGC_INITIALIZE_PASS_END(DomainShaderLowering, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

        DomainShaderLowering::DomainShaderLowering()
        : FunctionPass(ID)
        , m_headerSize(QuadEltUnit(2))
        , m_pMaxNumOfPushedInputs(384)  // No. of dword's that can be pushed (about 48 GRF's worth of input data)
    {
        initializeDomainShaderLoweringPass(*PassRegistry::getPassRegistry());
    }

    CollectDomainShaderProperties::CollectDomainShaderProperties() : ImmutablePass(ID)
    {
        initializeCollectDomainShaderPropertiesPass(*PassRegistry::getPassRegistry());
    }

    char DomainShaderLowering::ID = 0;
    const unsigned int DomainShaderLowering::m_maxNumOfOutput = 32;

    bool DomainShaderLowering::runOnFunction(llvm::Function& F)
    {
        MetaDataUtils* pMdUtils = nullptr;
        pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
        if (!isEntryFunc(pMdUtils, &F))
        {
            return false;
        }
        m_dsPropsPass = &getAnalysis<CollectDomainShaderProperties>();
        m_pModule = F.getParent();
        LowerIntrinsicInputOutput(F);
        return true;
    }

    void DomainShaderLowering::LowerIntrinsicInputOutput(Function& F)
    {
        CalculateVertexHeaderSize(F);
        llvm::IRBuilder<> builder(F.getParent()->getContext());
        SmallVector<Instruction*, 10> instructionToRemove;

        Value* zero = builder.getInt32(0);

        llvm::GlobalVariable* pGlobal = m_pModule->getGlobalVariable("MaxNumOfInputSignatureEntries");
        IGC::CodeGenContext* pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
        uint maxInputSignatureCount =
            int_cast<uint>(llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());
        pGlobal = m_pModule->getGlobalVariable("MaxNumOfPatchConstantSignatureEntries");
        uint m_pMaxPatchConstantSignatureDeclarations = int_cast<uint>(llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());

        std::vector<llvm::Instruction*> offsetInst(m_maxNumOfOutput + m_headerSize.Count(), nullptr);
        BasicBlock* retBlock = nullptr;
        for (auto BI = F.begin(), BE = F.end(); BI != BE; ++BI)
        {
            for (auto II = BI->begin(), IE = BI->end(); II != IE; ++II)
            {
                if (isa<ReturnInst>(BI->getTerminator()))
                {
                    retBlock = &(*BI);
                }
                if (GenIntrinsicInst * inst = dyn_cast<GenIntrinsicInst>(II))
                {
                    GenISAIntrinsic::ID IID = inst->getIntrinsicID();
                    if (IID == GenISAIntrinsic::GenISA_DCL_DSCntrlPtInputVec)
                    {
                        builder.SetInsertPoint(inst);
                        Value* inputSignatureCount = builder.getInt32(maxInputSignatureCount);
                        Value* newEleId1 = builder.CreateMul(
                            inst->getOperand(0),
                            inputSignatureCount,
                            VALUE_NAME("DSnewEleId1"));
                        Value* newEleId2 = builder.CreateAdd(
                            newEleId1,
                            inst->getOperand(1),
                            VALUE_NAME("DSnewEleId2"));

                        // Add the offset to the patch constant outputs and tessellation header here
                        newEleId2 = builder.CreateAdd(
                            newEleId2,
                            builder.getInt32(((m_pMaxPatchConstantSignatureDeclarations % 2 == 0) ?
                                m_pMaxPatchConstantSignatureDeclarations :
                                m_pMaxPatchConstantSignatureDeclarations + 1)
                                + 2), // Max Patch constant signatures does not include the tessellation header
                            VALUE_NAME("DSnewEleId2"));

                        LowerToInputVecOrURBReadIntrinsic(zero, newEleId2, inst);
                    }
                    else if (IID == GenISAIntrinsic::GenISA_DCL_DSPatchConstInputVec)
                    {
                        builder.SetInsertPoint(inst);

                        // Add the offset to  tessellation header here
                        Value* newEleId = builder.CreateAdd(
                            inst->getOperand(0),
                            builder.getInt32(2),// the tessellation header occupies 1GRF
                            VALUE_NAME("DSnewEleId2"));
                        LowerToInputVecOrURBReadIntrinsic(zero, newEleId, inst);
                    }
                    else if (IID == GenISAIntrinsic::GenISA_DCL_DSInputTessFactor)
                    {
                        if (llvm::ConstantInt * pIndex = llvm::dyn_cast<llvm::ConstantInt>(inst->getOperand(0)))
                        {
                            uint32_t index = 0;
                            ShaderOutputType tessFactor =
                                (enum ShaderOutputType)pIndex->getZExtValue();

                            switch (tessFactor)
                            {
                            case SHADER_OUTPUT_TYPE_FINAL_QUAD_U_EQ_0_EDGE_TESSFACTOR:
                            case SHADER_OUTPUT_TYPE_FINAL_TRI_U_EQ_0_EDGE_TESSFACTOR:
                            case SHADER_OUTPUT_TYPE_FINAL_LINE_DETAIL_TESSFACTOR:
                                index = 7;
                                break;

                            case SHADER_OUTPUT_TYPE_FINAL_QUAD_V_EQ_0_EDGE_TESSFACTOR:
                            case SHADER_OUTPUT_TYPE_FINAL_TRI_V_EQ_0_EDGE_TESSFACTOR:
                            case SHADER_OUTPUT_TYPE_FINAL_LINE_DENSITY_TESSFACTOR:
                                index = 6;
                                break;

                            case SHADER_OUTPUT_TYPE_FINAL_QUAD_U_EQ_1_EDGE_TESSFACTOR:
                            case SHADER_OUTPUT_TYPE_FINAL_TRI_W_EQ_0_EDGE_TESSFACTOR:
                                index = 5;
                                break;

                            case SHADER_OUTPUT_TYPE_FINAL_QUAD_V_EQ_1_EDGE_TESSFACTOR:
                            case SHADER_OUTPUT_TYPE_FINAL_TRI_INSIDE_TESSFACTOR:
                                index = 4;
                                break;

                            case SHADER_OUTPUT_TYPE_FINAL_QUAD_U_INSIDE_TESSFACTOR:
                                index = 3;
                                break;

                            case SHADER_OUTPUT_TYPE_FINAL_QUAD_V_INSIDE_TESSFACTOR:
                                index = 2;
                                break;
                            default:
                                break;
                            }
                            const e_interpolation interpolant =
                                DSDualPatchEnabled(pCtx) ?
                            EINTERPOLATION_UNDEFINED : EINTERPOLATION_CONSTANT;
                            Value* arguments[] =
                            {
                                builder.getInt32(index),
                                builder.getInt32(interpolant) // arg2 - interpolation mode
                            };

                            Instruction* shaderInputVec = GenIntrinsicInst::Create(
                                GenISAIntrinsic::getDeclaration(
                                    m_pModule,
                                    GenISAIntrinsic::GenISA_DCL_inputVec,
                                    Type::getFloatTy(m_pModule->getContext())),
                                arguments,
                                "",
                                inst);
                            inst->replaceAllUsesWith(shaderInputVec);
                        }
                    }
                    else if (IID == GenISAIntrinsic::GenISA_OUTPUT)
                    {
                        lowerOutput(inst, offsetInst);
                        instructionToRemove.push_back(inst);
                    }
                }
            }
        }

        const CPlatform& platform = getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->platform;

        Value* undef = llvm::UndefValue::get(Type::getFloatTy(F.getContext()));
        if (platform.WaForceDSToWriteURB())
        {
            unsigned int numPhaseWritten = 0;
            for (unsigned int i = 0; i < m_maxNumOfOutput + m_headerSize.Count(); i += 2)
            {
                if (offsetInst[i] != nullptr || offsetInst[i + 1] != nullptr)
                {
                    numPhaseWritten++;
                }
            }
            if (numPhaseWritten < 2)
            {
                for (unsigned int i = 0; i < 4; i += 2)
                {
                    // In case shader doesn't write in t eh URB header we force a dummy write to avoid HW hang
                    if (offsetInst[i] == nullptr && offsetInst[i + 1] == nullptr)
                    {
                        Value* data[8] =
                        {
                            undef,
                            undef,
                            undef,
                            undef,
                            undef,
                            undef,
                            undef,
                            undef,
                        };
                        // write 32bytes of random data in the vertex header
                        Value* offset = ConstantInt::get(Type::getInt32Ty(m_pModule->getContext()), i);
                        m_dsPropsPass->DeclareOutput(QuadEltUnit(i + 1));
                        m_dsPropsPass->DeclareOutput(QuadEltUnit(i + 2));
                        AddURBWrite(offset, 0xFF, data, retBlock->getTerminator());
                    }
                }
            }
        }
        //URB padding to 32Byte offsets
        bool addURBPaddingTo32Bytes = true;
        for (unsigned int i = 0; addURBPaddingTo32Bytes && i < m_maxNumOfOutput + m_headerSize.Count(); i++)
        {
            //If not aligned to 32Byte offset and has valid data
            if (offsetInst[i])
            {
                unsigned int adjacentSlot = i + (i % 2 == 0 ? 1 : -1);
                if (offsetInst[adjacentSlot])
                {
                    //Valid data present
                    continue;
                }
                else
                {
                    //No need to pad URB at offset position 0 for OGL since it is always padded with 0
                    if (i == 1 && pCtx->m_DriverInfo.NeedClearVertexHeader())
                    {
                        break;
                    }
                    else
                    {

                        Value* data[8] =
                        {
                            undef,
                            undef,
                            undef,
                            undef,
                            undef,
                            undef,
                            undef,
                            undef,
                        };

                        Value* offset = ConstantInt::get(Type::getInt32Ty(m_pModule->getContext()), adjacentSlot);
                        AddURBWrite(offset, 0xF, data, offsetInst[i]);
                    }
                }
            }
        }

        for (auto& inst : instructionToRemove)
        {
            inst->eraseFromParent();
        }
    }

    void DomainShaderLowering::CalculateVertexHeaderSize(Function& F)
    {
        IGC::CodeGenContext* context = nullptr;
        context = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
        if (context->getModuleMetaData()->URBInfo.hasVertexHeader)
        {
            if (context->getModuleMetaData()->URBInfo.has64BVertexHeaderOutput)
            {
                m_headerSize = QuadEltUnit(4);
                m_dsPropsPass->DeclareClipDistance();
            }
            else
            {
                m_headerSize = QuadEltUnit(2);
            }
        }
        else
        {
            m_headerSize = QuadEltUnit(0);
        }

        for (auto I = F.begin(), E = F.end(); I != E; ++I)
        {
            llvm::BasicBlock* pLLVMBB = &(*I);
            llvm::BasicBlock::InstListType& instructionList = pLLVMBB->getInstList();
            for (auto I = instructionList.begin(), E = instructionList.end(); I != E; ++I)
            {
                if (GenIntrinsicInst * inst = dyn_cast<GenIntrinsicInst>(I))
                {
                    if (inst->getIntrinsicID() == GenISAIntrinsic::GenISA_OUTPUT)
                    {
                        const ShaderOutputType usage = static_cast<ShaderOutputType>(
                            llvm::cast<llvm::ConstantInt>(inst->getOperand(4))->getZExtValue());
                        if (usage == SHADER_OUTPUT_TYPE_CLIPDISTANCE_LO ||
                            usage == SHADER_OUTPUT_TYPE_CLIPDISTANCE_HI)
                        {
                            context->getModuleMetaData()->URBInfo.hasVertexHeader = true;
                            context->getModuleMetaData()->URBInfo.has64BVertexHeaderOutput = true;
                            m_headerSize = QuadEltUnit(4);
                            m_dsPropsPass->DeclareClipDistance();
                        }
                        else if (usage == SHADER_OUTPUT_TYPE_POSITION ||
                            usage == SHADER_OUTPUT_TYPE_POINTWIDTH ||
                            usage == SHADER_OUTPUT_TYPE_RENDER_TARGET_ARRAY_INDEX ||
                            usage == SHADER_OUTPUT_TYPE_COARSE_PIXEL_SIZE)
                        {
                            context->getModuleMetaData()->URBInfo.hasVertexHeader = true;
                            if (m_headerSize < QuadEltUnit(2))
                            {
                                m_headerSize = QuadEltUnit(2);
                            }
                        }
                    }
                }
            }
        }
    }


    void DomainShaderLowering::LowerToInputVecOrURBReadIntrinsic(Value* index, Value* offset, Instruction* prev)
    {
        llvm::IRBuilder<> builder(prev);
        bool loweredToInputVec = false;

        if (llvm::ConstantInt * pElementIndex = llvm::dyn_cast<llvm::ConstantInt>(offset))
        {
            uint elementIndex = static_cast<uint>(pElementIndex->getZExtValue());
            uint currentElementIndex = elementIndex * 4;

            bool pushCondition = (currentElementIndex < m_pMaxNumOfPushedInputs);

            if (pushCondition)
            {
                loweredToInputVec = true;
                AddInputVec(prev, currentElementIndex);
            }
        }
        if (!loweredToInputVec)
        {
            // Else we need to lower this instruction to URBRead
            AddURBRead(index, offset, prev);
        }
    }

    void DomainShaderLowering::AddInputVec(Instruction* prev, uint32_t currentElementIndex)
    {
        llvm::IGCIRBuilder<> builder(prev);
        Value* channels[4] = { 0 };

        Function* shaderInputVecFn = GenISAIntrinsic::getDeclaration(
            m_pModule,
            GenISAIntrinsic::GenISA_DCL_inputVec,
            builder.getFloatTy());
        for (unsigned int i = 0; i < 4; i++)
        {
            Value* index = builder.getInt32(currentElementIndex + i);
            IGC::CodeGenContext* pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
            const e_interpolation interpolant =
                DSDualPatchEnabled(pCtx) ?
                EINTERPOLATION_UNDEFINED : EINTERPOLATION_CONSTANT;
            Value* interpolation = builder.getInt32(interpolant);
            channels[i] = builder.CreateCall2(shaderInputVecFn, index, interpolation);
        }
        for (auto useIt = prev->user_begin(); useIt != prev->user_end();)
        {
            Value* I = *useIt++;
            if (ExtractElementInst * elem = dyn_cast<ExtractElementInst>(I))
            {
                if (ConstantInt * index = dyn_cast<ConstantInt>(elem->getIndexOperand()))
                {
                    unsigned int channelIndex = static_cast<unsigned int>(index->getZExtValue());
                    elem->replaceAllUsesWith(channels[channelIndex]);
                    elem->eraseFromParent();
                }
            }
        }
        if (!prev->use_empty())
        {
            Value* vec = UndefValue::get(prev->getType());
            for (unsigned int i = 0; i < 4; i++)
            {
                vec = builder.CreateInsertElement(vec, channels[i], builder.getInt32(i));
            }
            prev->replaceAllUsesWith(vec);
        }
    }

    void DomainShaderLowering::AddURBRead(Value* index, Value* offset, Instruction* prev)
    {
        Value* arguments[] =
        {
            index,
            offset
        };

        Instruction* urbRead = GenIntrinsicInst::Create(
            GenISAIntrinsic::getDeclaration(m_pModule, GenISAIntrinsic::GenISA_URBRead),
            arguments,
            "",
            prev);
        Value* vec4 = nullptr;
        while (!prev->use_empty())
        {
            auto I = prev->user_begin();
            if (ExtractElementInst * elem = dyn_cast<ExtractElementInst>(*I))
            {
                Instruction* newExt =
                    ExtractElementInst::Create(urbRead, elem->getIndexOperand(), "", elem);
                elem->replaceAllUsesWith(newExt);
                elem->eraseFromParent();
            }
            else
            {
                // the vector is used directly, extract the first 4 elements and recreate a vec4
                if (vec4 == nullptr)
                {
                    Value* data[4] = { nullptr, nullptr, nullptr, nullptr };
                    Type* int32Ty = Type::getInt32Ty(m_pModule->getContext());

                    VectorToElement(urbRead, data, int32Ty, prev, 4);
                    vec4 = ElementToVector(data, int32Ty, prev, 4);
                }

                (*I)->replaceUsesOfWith(prev, vec4);
            }
        }
    }

    void DomainShaderLowering::lowerOutput(GenIntrinsicInst* inst, std::vector<llvm::Instruction*>& offsetInst)
    {
        llvm::IRBuilder<> builder(inst);

        const ShaderOutputType usage = static_cast<ShaderOutputType>(
            llvm::cast<llvm::ConstantInt>(inst->getOperand(4))->getZExtValue());
        const uint attributeIndex = static_cast<uint>(
            llvm::cast<llvm::ConstantInt>(inst->getOperand(5))->getZExtValue());

        Value* data[8];
        unsigned int mask = 0;
        SetMaskAndData(inst, mask, data);


        // Compute the offset for the given attribute and usage.
        QuadEltUnit offset = GetURBOffset(usage, attributeIndex);
        IGC_ASSERT(offset.Count() < offsetInst.size());
        IGC_ASSERT(offset.Count() < (m_maxNumOfOutput + m_headerSize.Count()));
        offsetInst[offset.Count()] = inst;

        uint offsetWritten = int_cast<unsigned int>(offset.Count() + (mask <= 0xF ? 1 : 2));
        m_dsPropsPass->DeclareOutput(QuadEltUnit(offsetWritten));
        // Create urb write instruction.
        Value* offsetVal = builder.getInt32(offset.Count());
        AddURBWrite(offsetVal, mask, data, inst);
    }

    QuadEltUnit DomainShaderLowering::GetURBOffset(
        ShaderOutputType outputType,
        uint attrIdx)
    {
        switch (outputType)
        {
        case SHADER_OUTPUT_TYPE_POINTWIDTH:
            return QuadEltUnit(0);

        case SHADER_OUTPUT_TYPE_POSITION:
            return QuadEltUnit(1);

        case SHADER_OUTPUT_TYPE_DEFAULT:
            return QuadEltUnit(attrIdx) + m_headerSize;

        case SHADER_OUTPUT_TYPE_CLIPDISTANCE_LO:
            return QuadEltUnit((m_headerSize.Count() - 2));

        case SHADER_OUTPUT_TYPE_CLIPDISTANCE_HI:
            return QuadEltUnit((m_headerSize.Count() - 1));

        case SHADER_OUTPUT_TYPE_VIEWPORT_ARRAY_INDEX:
            m_dsPropsPass->SetVPAIndexDeclared(true);
            return QuadEltUnit(0);
            break;

        case SHADER_OUTPUT_TYPE_RENDER_TARGET_ARRAY_INDEX:
            m_dsPropsPass->SetRTAIndexDeclared(true);
            return QuadEltUnit(0);
            break;
        default:
            IGC_ASSERT_MESSAGE(0, "unknown DS output type");
            break;
        }
        return QuadEltUnit(0);
    }

    /// Inserts new URBWrite instruction with given mask and arguments before
    /// instuction 'prev'.
    /// TODO: This should be a common function for all Lowering passes.
    void DomainShaderLowering::AddURBWrite(
        llvm::Value* offset,
        unsigned int mask,
        llvm::Value* data[8],
        llvm::Instruction* prev)
    {
        IGC_ASSERT_MESSAGE(mask < 256, "mask is an 8-bit bitmask and has to be in range 0..255");
        Value* arguments[] =
        {
            offset,
            ConstantInt::get(Type::getInt32Ty(m_pModule->getContext()), mask),
            data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]
        };

        GenIntrinsicInst::Create(
            GenISAIntrinsic::getDeclaration(m_pModule, GenISAIntrinsic::GenISA_URBWrite),
            arguments,
            "",
            prev);
    }

    // TODO: think about moving this to a common base class for all lowerings
    void DomainShaderLowering::SetMaskAndData(
        llvm::Instruction* inst,
        unsigned int& mask,
        llvm::Value* data[8])
    {
        // argument positions in the argument list
        const uint usageArgPos = 4;

        Value* fpzero = llvm::ConstantFP::get(Type::getFloatTy(m_pModule->getContext()), 0.0f);
        for (unsigned int i = 0; i < 8; ++i)
        {
            data[i] = fpzero;
        }
        // handle cases of writes of single dwords that may happen in vertex header
        /// The first four dwords of VUE are as follows:
        /// +------------+
        /// |  reserved  |  <--offset = 0
        /// +------------+
        /// |            |  <--offset = 1
        /// +------------+
        /// |            |  <--offset = 2
        /// +------------+
        /// | POINTWIDTH |  <--offset = 3
        /// +------------+

        const ShaderOutputType usage = static_cast<ShaderOutputType>(
            llvm::cast<llvm::ConstantInt>(inst->getOperand(usageArgPos))->getZExtValue());
        switch (usage)
        {
        case SHADER_OUTPUT_TYPE_POINTWIDTH:
            mask = 0x08;
            data[3] = inst->getOperand(0);
            break;
        case SHADER_OUTPUT_TYPE_RENDER_TARGET_ARRAY_INDEX:
            mask = 0x02;
            data[1] = inst->getOperand(0);
            break;
        case SHADER_OUTPUT_TYPE_VIEWPORT_ARRAY_INDEX:
            mask = 0x04;
            data[2] = inst->getOperand(0);
            break;
        default:
        {
            const uint numDataArgs = 4;
            mask = (1u << numDataArgs) - 1u; // bitmask of 4 consecutive ones
            for (unsigned int i = 0; i < numDataArgs; ++i)
            {
                data[i] = inst->getOperand(i);
            }
        }
        break;
        }
    }

    char CollectDomainShaderProperties::ID = 0;

    void CollectDomainShaderProperties::DeclareClipDistance()
    {
        m_dsProps.m_hasClipDistance = true;
    }

    void CollectDomainShaderProperties::DeclareOutput(QuadEltUnit newOffset)
    {
        if (m_dsProps.m_URBOutputLength < newOffset)
        {
            m_dsProps.m_URBOutputLength = newOffset;
        }
    }

    void CollectDomainShaderProperties::SetDomainPointUArgu(llvm::Value* Arg)
    {
        m_dsProps.m_UArg = Arg;
    }

    void CollectDomainShaderProperties::SetDomainPointVArgu(llvm::Value* Arg)
    {
        m_dsProps.m_VArg = Arg;
    }

    void CollectDomainShaderProperties::SetDomainPointWArgu(llvm::Value* Arg)
    {
        m_dsProps.m_WArg = Arg;
    }

    llvm::Value* CollectDomainShaderProperties::GetDomainPointUArgu()
    {
        return (m_dsProps.m_UArg);
    }

    llvm::Value* CollectDomainShaderProperties::GetDomainPointVArgu()
    {
        return (m_dsProps.m_VArg);
    }

    llvm::Value* CollectDomainShaderProperties::GetDomainPointWArgu()
    {
        return (m_dsProps.m_WArg);
    }

    llvm::FunctionPass* createDomainShaderLoweringPass()
    {
        return new DomainShaderLowering();
    }

    void CollectDomainShaderProperties::SetRTAIndexDeclared(bool rtaiDeclared)
    {
        m_dsProps.m_isRTAIndexDeclared = rtaiDeclared;
    }

    void CollectDomainShaderProperties::SetVPAIndexDeclared(bool vpaiDeclared)
    {
        m_dsProps.m_isVPAIndexDeclared = vpaiDeclared;
    }

}
