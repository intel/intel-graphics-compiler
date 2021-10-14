/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMUtils.h"
#include "Compiler/CISACodeGen/LowerGSInterface.h"
#include "common/debug/Debug.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/IGCIRBuilder.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include <llvm/IR/Constants.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC::IGCMD;

namespace IGC
{

    class LowerGSInterface : public llvm::FunctionPass, public llvm::InstVisitor<LowerGSInterface>
    {
        static char ID;
    public:

        LowerGSInterface(IGC::CodeGenContext* pContext) :
            FunctionPass(ID),
            m_pContext(pContext),
            m_hasDefaultStreamId(false),
            m_cutBitsRequired(true)
        { }

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual llvm::StringRef getPassName() const override { return "LowerGSInterface"; }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
        }

        void visitCallInst(llvm::CallInst& I);
        void visitReturnInst(llvm::ReturnInst& I);

    private:
        void createCutVariables(llvm::Function& F, unsigned int maxOutputVertexCount);
        void createStreamVariables(llvm::Function& F, unsigned int maxOutputVertexCount);
        void updateCutVariables(llvm::GenIntrinsicInst& endPrimitiveInstr);
        void updateStreamVariables(llvm::GenIntrinsicInst& setStreamInstr);

        /// Emits instructions writing to control header in case of cut bits.
        void writeControlHeaderCuts(llvm::ReturnInst& I);
        /// Emits instructions writing to control header in case of stream id bits.
        void writeControlHeaderStreams(llvm::ReturnInst& I);
        /// True if we need to maintain cut bit data in llvm code.
        bool cutBitsRequired() const;
        /// Determines whether we need to keep track of cut bit data at llvm instructions level.
        void setCutBitsRequired(const llvm::Function& F);

        llvm::Value* m_CutBits[32];
        llvm::Value* m_StreamIdBits[64];
        IGC::CodeGenContext* m_pContext;
        llvm::AllocaInst* m_EmitCounter;
        bool m_hasDefaultStreamId;
        bool m_cutBitsRequired;
        unsigned int m_maxControlHeaderIndex;
    };

    char LowerGSInterface::ID = 0;

    bool LowerGSInterface::runOnFunction(llvm::Function& F)
    {
        MetaDataUtils* pMdUtils = nullptr;
        pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
        if (pMdUtils->findFunctionsInfoItem(&F) == pMdUtils->end_FunctionsInfo())
        {
            return false;
        }

        llvm::IRBuilder<> builder(&F.getEntryBlock(), F.getEntryBlock().begin());

        auto pMaxOutputVertices = F.getParent()->getGlobalVariable("GsMaxOutputVertices");
        IGC_ASSERT_MESSAGE(nullptr != pMaxOutputVertices, "GsMaxOutputVertices must be defined");
        const unsigned int maxOutputVertices = static_cast<unsigned int>(
            (llvm::cast<llvm::ConstantInt>(pMaxOutputVertices->getInitializer())->getZExtValue()));

        // we need to keep the count of emits, we do this in this new variable
        // initialized to -1 since emitCount is:
        // last vertexIndex + 1 if vertices were emitted
        // 0 if no vertices were emitted.
        m_EmitCounter = builder.CreateAlloca(builder.getInt32Ty(), 0, VALUE_NAME("vertexIndex"));
        auto minusOne = llvm::ConstantInt::get(builder.getInt32Ty(), -1);
        builder.CreateStore(minusOne, m_EmitCounter);

        // find out if we have default streamID defined by looking up the value of a global var
        auto pDefaultStreamID = F.getParent()->getGlobalVariable("DefaultStreamID");
        IGC_ASSERT_MESSAGE(nullptr != pDefaultStreamID, "DefaultStreamID needs to be defined");
        const int defaultStreamId = static_cast<int>(llvm::cast<llvm::ConstantInt>(pDefaultStreamID->getInitializer())->getSExtValue());
        m_hasDefaultStreamId = defaultStreamId != -1;
        setCutBitsRequired(F);

        // If we have just one, default streamID, we can use cuts.
        // When writing to multiple streams, we need to update streamId bits and cannot use cuts
        if (m_hasDefaultStreamId) {
            // at the beginning, we initialize cut variables
            if (cutBitsRequired())
            {
                createCutVariables(F, maxOutputVertices);
            }
        }
        else
        {
            // or initialize stream variables
            createStreamVariables(F, maxOutputVertices);
        }

        // visit all instructions and perform actions depending on the instruction type
        visit(F);

        return true;
    }

    /// Replaces some of the interface-level intrinsic instructions with the implementation.
    /// We want to replace EndPrimitive and SetStream with code that handles cut/stream variables.
    void LowerGSInterface::visitCallInst(llvm::CallInst& I)
    {
        if (GenIntrinsicInst * CI = dyn_cast<GenIntrinsicInst>(&I))
        {
            switch (CI->getIntrinsicID())
            {
            case llvm::GenISAIntrinsic::GenISA_EndPrimitive:
            {
                // if the GenISAIntrinsic instruction is EndPrimitive, we need to remove it
                // and update cut variables.
                if (m_hasDefaultStreamId && cutBitsRequired())
                {
                    updateCutVariables(*CI);
                }
                I.eraseFromParent();
                break;
            }

            case llvm::GenISAIntrinsic::GenISA_SetStream:
            {
                if (!m_hasDefaultStreamId)
                {
                    updateStreamVariables(*CI);
                }
                else
                {
                    // if we have a default stream ID, the SetStream() calls can only set
                    // the stream to the default id (which is effectively no op in that case).
                    auto pDefaultStreamID = I.getParent()->getParent()->getParent()->getGlobalVariable("DefaultStreamID");
                    const int defaultStreamId = static_cast<int>(
                        llvm::cast<llvm::ConstantInt>(pDefaultStreamID->getInitializer())->getSExtValue());
                    const int streamID = static_cast<int>(
                        llvm::cast<llvm::ConstantInt>(I.getArgOperand(0))->getSExtValue());
                    IGC_ASSERT_MESSAGE(defaultStreamId == streamID, "When default streamID is set, all SetStream() calls need to set setreamID to the default value");
                }
                I.eraseFromParent();
                break;
            }

            case llvm::GenISAIntrinsic::GenISA_OUTPUTGS:
            {
                // since we need to keep track of emit count, we do this by storing the value
                // passed as the argument to OUTPUTGS
                llvm::IRBuilder<> builder(&I);
                builder.CreateStore(I.getOperand(6), m_EmitCounter);
                break;
            }

            default:
                //nothing to do
                break;
            }
        }
    }

    /// Creates and initializes variables that are used to keep cut bitmask.
    void LowerGSInterface::createCutVariables(llvm::Function& F, unsigned int maxOutputVertexCount)
    {
        // we use one bit per vertex, so max index is ceil(max vertex count divided by 32 bits in dword)
        m_maxControlHeaderIndex = (maxOutputVertexCount + 31) / 32;

        llvm::IGCIRBuilder<> b(&(*F.getEntryBlock().begin()));

        Value* zeroValue = b.getInt32(0);
        for (unsigned int i = 0; i < m_maxControlHeaderIndex; ++i)
        {
            m_CutBits[i] =
                b.CreateAlloca(b.getInt32Ty(), 0, VALUE_NAME(Twine("cutBits").concat(Twine(i))));
            b.CreateStore(zeroValue, m_CutBits[i]);
        }
    }

    /// Creates and initializes variables that are used to keep streamId bitmask.
    void LowerGSInterface::createStreamVariables(llvm::Function& F, unsigned int maxOutputVertexCount)
    {
        // we use TWO bits per vertex, so max index is max vertex count / (32 bits in dword) times 2.
        m_maxControlHeaderIndex = (maxOutputVertexCount + 15) / 16;

        llvm::IGCIRBuilder<> b(&(*F.getEntryBlock().begin()));

        Value* zeroValue = b.getInt32(0);
        for (unsigned int i = 0; i < m_maxControlHeaderIndex; ++i)
        {
            m_StreamIdBits[i] =
                b.CreateAlloca(b.getInt32Ty(), 0, VALUE_NAME(Twine("streamBits").concat(Twine(i))));
            b.CreateStore(zeroValue, m_StreamIdBits[i]);
        }
    }

    /// We translate EndPrimitive to a sequence of instructions that update cut variables.
    /// The idea is as follows: we want to index within an array of cut variables.
    /// However, when EndPrimitive appears before any emits (so emitCount is still zero), we don't want
    /// to do anything.
    /// So first we compare emitCount with zero, and based on the result we either use bit '1' or '0'
    /// as the value that will be or-ed to the appropriate bit within the cut vector.
    /// Pseudocode:
    ///     isNonZeroCount = emitCount == 0 ? 0 : 1
    ///     safeEmitCount = (emitCount == 0) ? 0 : emitCount-1  // not to index outside of the vector
    ///     index = safeEmitCount div 32                        // index of the element in the vector
    ///     modulus = safeEmitCount mod 32                      // index of the bit within the element
    ///     orVal = isNonZeroCount << modulus
    ///     cutBits[index] |= orVal
    void LowerGSInterface::updateCutVariables(llvm::GenIntrinsicInst& I)
    {
        IGCLLVM::IRBuilder<> b(&I);
        Value* emitCount = I.getArgOperand(0);
        Value* zeroValue = b.getInt32(0);

        //   %isNonZeroCount = icmp ne i32 %emitCount, 0
        Value* isNonZeroCount = b.CreateICmp(
            ICmpInst::ICMP_NE,
            emitCount,
            zeroValue,
            VALUE_NAME("isNonZeroCount"));

        //   %isNonZeroCount_i32 = zext i1 %isNonZeroCount_s to i32
        Value* isNonZeroCount_i32 = b.CreateZExt(
            isNonZeroCount,
            b.getInt32Ty(),
            VALUE_NAME("isNonZeroCount_i32"));

        //   %emitCountMinusOne = sub i32 %emitCount, 1
        Value* emitCountMinusOne = b.CreateSub(
            emitCount,
            b.getInt32(1),
            VALUE_NAME("emitCountMinusOne"));

        //   %safeEmitCount = select i1 %isNonZeroCount_s, i32 %emitCountMinusOne_s, i32 0
        Value* safeEmitCount = b.CreateSelect(
            isNonZeroCount,
            emitCountMinusOne,
            zeroValue,
            VALUE_NAME("safeEmitCount"));

        //   %index = lshr i32 %safeEmitCount, 5
        Value* index = b.CreateLShr(
            safeEmitCount,
            b.getInt32(5),
            VALUE_NAME("index"));

        //   %modulus = and i32 %safeEmitCount, 31
        Value* modulus = b.CreateAnd(
            safeEmitCount,
            b.getInt32(31),
            VALUE_NAME("modulus"));

        //   %orVal = shl i32 %isNonZeroCount_s1, %modulus_s
        Value* orVal = b.CreateShl(isNonZeroCount_i32, modulus, VALUE_NAME("orVal"));

        // Load the cut instruction bits and create a vector of values
        Value* tempVector = UndefValue::get(IGCLLVM::FixedVectorType::get(b.getInt32Ty(), m_maxControlHeaderIndex));
        for (uint32_t i = 0; i < m_maxControlHeaderIndex; ++i)
        {
            llvm::Value* pLoadedCutDword = b.CreateLoad(m_CutBits[i]);
            auto iAsVal = b.getInt32(i);
            tempVector = b.CreateInsertElement(
                tempVector,
                pLoadedCutDword,
                iAsVal,
                VALUE_NAME(Twine("tempVec_").concat(Twine(i))));
        }

        // %dynExt = extractelement %tempVec, %index
        Value* dynExt = b.CreateExtractElement(tempVector, index, VALUE_NAME("dynExt"));

        // %cutOr = or %dynExt, %orVal
        Value* theNewOne = b.CreateOr(dynExt, orVal, VALUE_NAME("cutOr"));

        // %updatedCutVec = insertelement %tempVector,
        tempVector = b.CreateInsertElement(tempVector, theNewOne, index, VALUE_NAME("updatedCutVec"));

        // Store the values back into the cut bit memory variables
        for (uint32_t i = 0; i < m_maxControlHeaderIndex; ++i)
        {
            auto iAsVal = b.getInt32(i);
            auto element = b.CreateExtractElement(
                tempVector,
                iAsVal,
                VALUE_NAME(Twine("extElem_").concat(Twine(i))));
            b.CreateStore(element, m_CutBits[i]);
        }
    } //update cut variables

    /// Translates SetStream(%stramID, %emitCount) to the sequence of instructions that update streamID
    /// bits.
    ///
    void LowerGSInterface::updateStreamVariables(llvm::GenIntrinsicInst& I)
    {
        //setting of streamID bits makes sense only when having many streams
        IGC_ASSERT(!m_hasDefaultStreamId);

        Value* streamID = I.getArgOperand(0);
        Value* emitCount = I.getArgOperand(1);

        IGCLLVM::IRBuilder<> builder(&I);
        // %emitCountMulTwo_s = shl i32 %emitCount, 1
        Value* emitCountMulTwo = builder.CreateShl(
            emitCount,
            llvm::ConstantInt::get(builder.getInt32Ty(), 1),
            VALUE_NAME("emitCountMulTwo"));

        // %wordIndex = lshr i32 %emitCountMulTwo_s, 5
        Value* wordIndex = builder.CreateLShr(
            emitCountMulTwo,
            llvm::ConstantInt::get(builder.getInt32Ty(), 5),
            VALUE_NAME("wordIndex"));

        // %modulus = and i32 %emitCountMulTwo, 31
        Value* modulus = builder.CreateAnd(
            emitCountMulTwo,
            llvm::ConstantInt::get(builder.getInt32Ty(), 31),
            VALUE_NAME("modulus"));

        // %orVal = shl i32 %stream_id, %modulus_s
        Value* orVal = builder.CreateShl(streamID, modulus, "orVal");

        // Load the streamID bits and create a vector of values
        Value* tempVector = UndefValue::get(IGCLLVM::FixedVectorType::get(builder.getInt32Ty(), m_maxControlHeaderIndex));
        for (unsigned int i = 0; i < m_maxControlHeaderIndex; ++i)
        {
            llvm::Value* pLoadedStreamIdDword = builder.CreateLoad(m_StreamIdBits[i]);
            auto iAsVal = builder.getInt32(i);
            tempVector = builder.CreateInsertElement(
                tempVector,
                pLoadedStreamIdDword,
                iAsVal,
                VALUE_NAME(Twine("tempVec_").concat(Twine(i))));
        }

        // %dynExt = extractelement %tempVec, %index
        auto theOne = builder.CreateExtractElement(tempVector, wordIndex, VALUE_NAME("dynExt"));

        // %cutOr_s = or i32 %exElm_s, %orVal_s
        Value* theNewOne = builder.CreateOr(theOne, orVal, "cutOr");

        tempVector = builder.CreateInsertElement(
            tempVector,
            theNewOne,
            wordIndex,
            VALUE_NAME("updatedCutVec"));

        // Store the values back into the stream id memory variables
        for (unsigned int i = 0; i < m_maxControlHeaderIndex; ++i)
        {
            auto iAsVal = builder.getInt32(i);
            auto element = builder.CreateExtractElement(
                tempVector,
                iAsVal,
                VALUE_NAME(Twine("extElem_").concat(Twine(i))));
            builder.CreateStore(element, m_StreamIdBits[i]);
        }
    } // updateStreamVariables

    /// Inserts a call to intrinsic to write out control data header as the last instruction before return.
    void LowerGSInterface::visitReturnInst(llvm::ReturnInst& I)
    {
        if (m_hasDefaultStreamId)
        {
            writeControlHeaderCuts(I);
        }
        else
        {
            writeControlHeaderStreams(I);
        }
    }

    /// Adds instructions calling intrinsic that will get translated to writing control data header.
    void LowerGSInterface::writeControlHeaderCuts(llvm::ReturnInst& I)
    {
        IGCLLVM::IRBuilder<> builder(&I);
        Module* M = I.getParent()->getParent()->getParent();
        Function* pFunc = llvm::GenISAIntrinsic::getDeclaration(M, llvm::GenISAIntrinsic::GenISA_GsCutControlHeader);
        IGC_ASSERT(pFunc);

        std::vector<llvm::Value*> params;
        // if we didn't need to keep cut bits, we generate cut header with all zeros
        if (!cutBitsRequired())
        {
            m_maxControlHeaderIndex = 0;
        }
        for (unsigned int i = 0; i < m_maxControlHeaderIndex; ++i)
        {
            params.emplace_back(builder.CreateLoad(
                m_CutBits[i],
                VALUE_NAME(Twine("cbit").concat(Twine(i)))));
        }
        for (unsigned int i = m_maxControlHeaderIndex; i < 16; ++i)
        {
            params.emplace_back(builder.getInt32(0));
        }
        auto one = builder.getInt32(1);
        auto vidxLoaded = builder.CreateLoad(m_EmitCounter, VALUE_NAME("vertexIndexLoaded"));
        params.emplace_back(builder.CreateAdd(vidxLoaded, one));
        IGC_ASSERT(params.size() == 17);
        builder.CreateCall(pFunc, params);
    }

    /// Adds instructions calling intrinsic that will get translated to writing control data header.
    void LowerGSInterface::writeControlHeaderStreams(llvm::ReturnInst& I)
    {
        IGCLLVM::IRBuilder<> builder(&I);
        Module* M = I.getParent()->getParent()->getParent();
        Function* pFunc = llvm::GenISAIntrinsic::getDeclaration(M, llvm::GenISAIntrinsic::GenISA_GsStreamHeader);
        IGC_ASSERT(pFunc);

        std::vector<llvm::Value*> params;
        for (unsigned int i = 0; i < m_maxControlHeaderIndex; ++i)
        {
            params.emplace_back(builder.CreateLoad(
                m_StreamIdBits[i],
                VALUE_NAME(Twine("sbit").concat(Twine(i)))));
        }
        for (unsigned int i = m_maxControlHeaderIndex; i < 64; ++i)
        {
            params.emplace_back(builder.getInt32(0));
        }
        auto one = builder.getInt32(1);
        auto vIdxLoaded = builder.CreateLoad(m_EmitCounter, VALUE_NAME("vertexIndexLoaded"));
        params.emplace_back(builder.CreateAdd(vIdxLoaded, one));
        IGC_ASSERT(params.size() == 65);
        builder.CreateCall(pFunc, params);
    }

    bool LowerGSInterface::cutBitsRequired() const
    {
        return m_cutBitsRequired;
    }

    void LowerGSInterface::setCutBitsRequired(const llvm::Function& F)
    {
        auto pVar = F.getParent()->getGlobalVariable("GsOutputPrimitiveTopology");
        auto outputTopologyType = static_cast<USC::GFX3DPRIMITIVE_TOPOLOGY_TYPE>
            (llvm::cast<llvm::ConstantInt>(pVar->getInitializer())->getZExtValue());

        // by default we need to maintain data with cut bits
        m_cutBitsRequired = true;

        // but if the output topology is pointlist and we don't use different streams, we can skip it
        if (outputTopologyType == USC::GFX3DPRIM_POINTLIST && m_hasDefaultStreamId)
        {
            m_cutBitsRequired = false;
        }
    }

} // namespace IGC

llvm::Pass* createLowerGSInterfacePass(IGC::CodeGenContext* pContext)
{
    return new IGC::LowerGSInterface(pContext);
}

