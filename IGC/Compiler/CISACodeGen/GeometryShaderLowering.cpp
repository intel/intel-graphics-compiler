/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/GeometryShaderLowering.hpp"
#include "Compiler/CISACodeGen/GeometryShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/CollectGeometryShaderProperties.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/debug/Dump.hpp"
#include "common/LLVMUtils.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvmWrapper/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvmWrapper/IR/DerivedTypes.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/InitializePasses.h"
#include "Probe/Assertion.h"

/***********************************************************************************
This file will lower GS input intrinsics to URBRead instructions.
It will also lower higher-level GS output intrinsics (OUTPUTGS, GsCutControlHeader)
to lower-level URBWrite instructions.
************************************************************************************/
namespace {

    using namespace llvm;
    using namespace IGC;
    using namespace IGC::IGCMD;

    class GeometryShaderLowering : public llvm::FunctionPass
    {

    public:
        GeometryShaderLowering();

        static char         ID;
        virtual bool runOnFunction(llvm::Function& function) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            // we need geometry shader properties to be collected by an earlier analysis pass
            AU.addRequired<CollectGeometryShaderProperties>();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        llvm::StringRef getPassName() const override
        {
            return "GeometryShaderLowering";
        }

    private:
        /// Replaces high-level intrinsics GSinputVec with URBRead instructions.
        void lowerInputGS(llvm::Instruction* pInst);

        /// Replaces high-level intrinsics GSSystemValue with URBRead instructions.
        void lowerSGVInputGS(llvm::Instruction* pInst);

        /// Replaces high-level intrinsics OUTPUTGS with URBWrite instructions.
        void lowerOutputGS(
            llvm::Instruction* pInst,
            std::vector<llvm::Instruction*>& offsetInst,
            bool& immediateAccess);

        /// Replaces high-level intrinsics GsCutControlHeader and GsStreamHeader
        /// with corresponding URBWrite instructions.
        void LowerControlHeader(llvm::Instruction* pInst);

        /// Adds URBWrite instruction that writes up to eight 'data' dwords at given 'offset'
        /// that have corresponding bits in 'mask' set to one.
        void AddURBWrite(
            llvm::Value* offset,
            unsigned int mask,
            llvm::Value* data[8],
            llvm::Instruction* prev);

        /// Creates URBRead instruction that reads data with from 'attribIndex' attribute of
        /// vertex given by 'vertexIndex'. It corresponds to GSinputVec instruction given in 'inst'.
        void AddURBRead(
            llvm::Value* vertexIndex,
            llvm::Value* attribIndex,
            llvm::Instruction* inst);

        /// Adds URBRead followed by extract element corresponding to the read of SGV value contained
        /// in instruction 'inst'.
        void AddSGVURBRead(
            llvm::Value* vertexIndex,
            SGVUsage usage,
            llvm::Instruction* inst);

        /// Adds URB writes that set the content of vertex headers to zero.
        /// TODO: This may not be required.
        void AddVertexURBHeaders(llvm::Function& function, std::vector<llvm::Instruction*>& offsetInst);

        /// Returns URB offset at which attribute with the given 'usage' and 'attributeIndex'
        /// needs to be written.
        QuadEltUnit GetURBWriteOffset(ShaderOutputType usage);

        /// Based on the output semantics of the OUTPUTGS instruction in 'inst'
        /// sets the mask of meaningful values in 'data' and updates the array 'data' with values taken
        /// from arguments of 'inst'.
        /// For default output, we set first four values for OUTPUTGS
        /// For scalar values of e.g. point size or render target array index, we set just one entry.
        void SetMaskAndData(llvm::Instruction* inst, unsigned int& mask, llvm::Value* data[8]);

        /// Returns a value that represents offset in vertex URB entry for the given attribute index.
        /// It takes into account the size of the vertex header and the attribute index computing
        /// attributeOffset = vertexHeaderSize + atrIndex * quadwordsize.
        Value* GetAttributeOffsetWithinVertex(Value* pAtrIdx, Instruction* pInst);

        /// Returns offset relative to the beginning of Vertex URB Entry where data with given SGV
        /// resides.
        QuadEltUnit GetURBReadOffset(SGVUsage usage);

        /// Returns channel number (0..3) which keeps data with the given SGV usage.
        Unit<Element> GetChannel(SGVUsage usage);

        /// Pointer to the module the processed function is a part of.
        IGCLLVM::Module* m_pModule;
        /// Convenience shorthand for floating point zero value.
        llvm::Value* m_fpzero;
        /// Keeps instructions that should be removed from the list at the end of runOnFunction().
        SmallVector<Instruction*, 32> m_instructionToRemove;
        /// Constant value that determine what is the maximum size of the input we push in thread payload.
        /// If the input size exceeds this value, we switch to pull model.
        static const QuadEltUnit MaxPushedInputSize;
        /// Pointer to the analysis pass that collects all the information about GS program.
        CollectGeometryShaderProperties* m_gsProps;
    };

} // end of unnamed namespace


#define PASS_FLAG "igc-geometry-shader-lowering"
#define PASS_DESCRIPTION "Lower inputs outputs for geometry shader"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(GeometryShaderLowering, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CollectGeometryShaderProperties)
IGC_INITIALIZE_PASS_END(GeometryShaderLowering, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

/// Constructor
GeometryShaderLowering::GeometryShaderLowering()
    : FunctionPass(ID)
    , m_pModule(nullptr)
    , m_fpzero(nullptr)
{
    initializeCollectGeometryShaderPropertiesPass(*PassRegistry::getPassRegistry());
    initializeMetaDataUtilsWrapperPass(*PassRegistry::getPassRegistry());
}

bool GeometryShaderLowering::runOnFunction(llvm::Function& function)
{
    MetaDataUtils* pMdUtils = nullptr;
    pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    if (!isEntryFunc(pMdUtils, &function))
    {
        return false;
    }
    m_pModule = (IGCLLVM::Module*)function.getParent();
    auto pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    // get the GS properties from the analysis pass
    m_gsProps = &getAnalysis<CollectGeometryShaderProperties>();
    m_gsProps->gatherInformation(function);
    m_fpzero = llvm::ConstantFP::get(Type::getFloatTy(function.getContext()), 0.0f);

    const QuadEltUnit globalOffset = m_gsProps->GetProperties().Output().GlobalHeaderSize();
    const QuadEltUnit outputVertexSize = m_gsProps->GetProperties().Output().PerVertex().Size();
    uint numOutputVertices = m_gsProps->GetProperties().Output().HasNonstaticVertexCount() ?
        m_gsProps->GetProperties().Output().MaxVertexCount() :
        m_gsProps->GetProperties().Output().ActualStaticVertexCount();
    unsigned int urbSize = globalOffset.Count() + (numOutputVertices * outputVertexSize.Count());
    std::vector<llvm::Instruction*> offsetInst(urbSize, nullptr);

    //URB Padding to 32 byte offset only when both vertex index and attribute index
    //are always directly accessed else will bail out from this optimization
    bool addURBPaddingTo32Bytes = true;

    for (auto I = function.begin(), E = function.end(); I != E; ++I)
    {
        llvm::BasicBlock::InstListType& instList = I->getInstList();

        for (auto instIter = instList.begin(), instIterEnd = instList.end();
            instIter != instIterEnd;
            ++instIter)
        {
            llvm::Instruction* pInst = &(*instIter);
            const auto opCode = GetOpCode(pInst);
            switch (opCode)
            {
            case llvm_gs_input:
                lowerInputGS(pInst);
                break;
            case llvm_gs_sgv:
                lowerSGVInputGS(pInst);
                break;
            case llvm_output:
            case llvm_output_gs:  //fall-through since we want to handle both
            {
                bool immediateAccess = false;
                lowerOutputGS(pInst, offsetInst, immediateAccess);
                addURBPaddingTo32Bytes = addURBPaddingTo32Bytes && immediateAccess;
                break;
            }
            case llvm_gs_cut_control_header: //fall-through since we want to handle both
            case llvm_gs_stream_header:
                LowerControlHeader(pInst);
                break;
            default:
                break;
            } // switch
        } // for
    } // for

    // need to add instructions clearing vertex headers
    // TODO: looks like this could be done more efficiently
    if (pCtx->m_DriverInfo.NeedClearVertexHeader())
    {
        AddVertexURBHeaders(function, offsetInst);
    }

    //URB Padding to 32 byte offset only when both vertex index and attribute index
    //are always directly accessed else will bail out from this optimization
    if (addURBPaddingTo32Bytes)
    {
        Value* undef = llvm::UndefValue::get(Type::getFloatTy(function.getContext()));
        Value* data[8] = { undef, undef, undef, undef, undef, undef, undef, undef };

        for (unsigned int i = globalOffset.Count() + 1; i < urbSize; i = i + 2)
        {
            if (offsetInst[i] && !offsetInst[i - 1])
            {
                Value* offset = ConstantInt::get(
                    Type::getInt32Ty(function.getContext()), i - 1);
                AddURBWrite(offset, 0xF, data, offsetInst[i]);
            }
        }
    }

    // need to remove lowered instructions that are not used anymore afterwards
    for (auto& inst : m_instructionToRemove)
    {
        inst->eraseFromParent();
    }

    return true;
}

// attributeOffsetWithinVertex = vertexHeaderSize + atrIndex
Value* GeometryShaderLowering::GetAttributeOffsetWithinVertex(
    Value* pAtrIdx,
    Instruction* pInst)
{
    const QuadEltUnit inputVertexHeaderSize = m_gsProps->GetProperties().Input().PerVertex().HeaderSize();
    IRBuilder<>builder(pInst);

    if (auto pConstAttribIdx = dyn_cast<ConstantInt>(pAtrIdx))
    {
        const QuadEltUnit aidx(static_cast<uint>(pConstAttribIdx->getZExtValue()));
        const QuadEltUnit offset = aidx + inputVertexHeaderSize;
        return builder.getInt32(offset.Count());
    }
    else
    {
        // attribute index is a runtime value
        // need to issue addition instruction
        Value* pInVertexHeaderSize = builder.getInt32(inputVertexHeaderSize.Count());
        return builder.CreateAdd(pAtrIdx, pInVertexHeaderSize);
    }
    return nullptr;
}

void GeometryShaderLowering::lowerInputGS(llvm::Instruction* pInst)
{
    IGC_ASSERT(GetOpCode(pInst) == llvm_gs_input);
    // We lower input read instruction to URBRead.
    Value* pVtxIdx = pInst->getOperand(0);
    Value* pAtrIdx = pInst->getOperand(1);
    // need to issue URB read instructions that will read inputs from URB
    Value* pOffset = GetAttributeOffsetWithinVertex(pAtrIdx, pInst);
    AddURBRead(pVtxIdx, pOffset, pInst);
    m_instructionToRemove.push_back(pInst);
}

void GeometryShaderLowering::lowerSGVInputGS(llvm::Instruction* pInst)
{
    IGC_ASSERT(GetOpCode(pInst) == llvm_gs_sgv);
    const SGVUsage usage = static_cast<SGVUsage>
        (llvm::cast<llvm::ConstantInt>(pInst->getOperand(1))->getZExtValue());
    // PrimitiveID and InstanceID are not per input vertex basis.
    // We don't lower them to URBReads and let the codegen handle translating such intrinsics.
    if (usage == PRIMITIVEID || usage == GS_INSTANCEID)
    {
        return;
    }
    Value* pVtxIdx = pInst->getOperand(0);
    // Add urb reads for reading sgv values from urb.
    // Since sgv values are scalar, we issue an URB read with appropriate offset
    // and then extract the right channel depending on the usage.
    AddSGVURBRead(pVtxIdx, usage, pInst);
    m_instructionToRemove.push_back(pInst);
    return;
}

/// Replaces high-level intrinsics OUTPUTGS with URBWrite instructions.
/// The difficult part of this task is to correctly compute offsets in URB memory where the writes
/// should take place. We do this based on the information taken from m_gsProps object that
/// keeps all the properties of GS program that determine URB layout of output data.
void GeometryShaderLowering::lowerOutputGS(
    llvm::Instruction* inst,
    std::vector<llvm::Instruction*>& offsetInst,
    bool& immediateAccess)
{
    IGC_ASSERT(GetOpCode(inst) == llvm_output_gs || GetOpCode(inst) == llvm_output);
    // argument positions in the argument list
    const int lastDataArgIndex = 3;
    const uint usageArgPos = 4;
    const uint attribArgPos = 5;
    const uint vertexArgPos = 6;

    // skip OUTPUTGS with all data as undef values.
    bool isAllUndef = true;
    for (int i = 0; i <= lastDataArgIndex; i++)
    {
        if (!isa<UndefValue>(inst->getOperand(i)))
        {
            isAllUndef = false;
            break;
        }
    }
    if (isAllUndef)
    {
        m_instructionToRemove.push_back(inst);
        return;
    }

    llvm::Value* pVertexIndex = inst->getOperand(vertexArgPos);
    llvm::Value* pAttributeIndex = inst->getOperand(attribArgPos);
    const ShaderOutputType usage = static_cast<ShaderOutputType>(
        llvm::cast<llvm::ConstantInt>(inst->getOperand(usageArgPos))->getZExtValue());

    Value* data[8] = { m_fpzero, m_fpzero, m_fpzero, m_fpzero, m_fpzero, m_fpzero, m_fpzero, m_fpzero };
    unsigned int mask;
    SetMaskAndData(inst, mask, data);

    //Set this to true when both vertex index and attribute index are immediates
    immediateAccess = false;

    //globalOffset is always aligned to 32 byte since it is in OctElm size
    const QuadEltUnit globalOffset = m_gsProps->GetProperties().Output().GlobalHeaderSize();
    QuadEltUnit outputVertexSize = m_gsProps->GetProperties().Output().PerVertex().Size();

    IRBuilder<> builder(inst);
    llvm::Value* offsetVal = builder.getInt32(globalOffset.Count());

    bool isZeroAttributeIndex =
        usage != ShaderOutputType::SHADER_OUTPUT_TYPE_DEFAULT;
    if (isZeroAttributeIndex)
    {
        // In this case attribute indices only play an informative role to identify corresponding copies of the value which
        // are defined in user-defined URB space.
        pAttributeIndex = builder.getInt32(0);
    }

    // Depending on the usage and attribute index, find the exact position where to write.
    const QuadEltUnit usageOffset = GetURBWriteOffset(usage);
    llvm::Value* pAttributeOffset = builder.CreateAdd(pAttributeIndex, builder.getInt32(usageOffset.Count()));
    llvm::Value* pVertexOffset = builder.CreateMul(builder.getInt32(outputVertexSize.Count()), pVertexIndex);
    offsetVal = builder.CreateAdd(offsetVal, pAttributeOffset);
    offsetVal = builder.CreateAdd(offsetVal, pVertexOffset);

    if (llvm::isa<llvm::ConstantInt>(offsetVal))
    {
        //For URB padding to 32 byte offset
        //Being conservative and doing it only when vertex index
        //and attribute index are immediates
        const QuadEltUnit offset = QuadEltUnit(int_cast<uint32_t>(llvm::cast<ConstantInt>(offsetVal)->getZExtValue()));
        offsetInst[offset.Count()] = inst;
        immediateAccess = true;
    }
    AddURBWrite(offsetVal, mask, data, inst);
    m_instructionToRemove.push_back(inst);
}

/// Inserts new URBWrite instruction with given mask and arguments before
/// instruction 'prev'.
/// TODO: This should be a common function for all Lowering passes.
void GeometryShaderLowering::AddURBWrite(
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

/// Replaces input instruction inst with URBRead intrinsic reading from URB at position
/// corresponding to vertexIndex and attribIndex
void GeometryShaderLowering::AddURBRead(
    llvm::Value* vertexIndex,
    llvm::Value* attribOffset,
    llvm::Instruction* inst)
{
    // TODO: This method should be a common method for all geometric stages
    // TODO: implemented in a common parent class!
    // TODO: Merge VertexShaderLowering::AddURBRead, HullShaderLowering::AddURBRead,
    // TODO: DomainShaderLowering::AddURBRead and this into one method.

    Value* arguments[] = { vertexIndex, attribOffset };

    Instruction* urbRead = GenIntrinsicInst::Create(
        GenISAIntrinsic::getDeclaration(m_pModule, GenISAIntrinsic::GenISA_URBRead),
        arguments,
        VALUE_NAME("urbread"),
        inst);
    // We assume that the input read is followed by a bunch of extract elements.
    // We need to update extracts so that their first operands are now the urbRead instruction.
    for (auto useIt = inst->user_begin(); useIt != inst->user_end(); )
    {
        Value* I = *useIt++;
        if (ExtractElementInst * elem = dyn_cast<ExtractElementInst>(I))
        {
            elem->setOperand(0, urbRead);
        }
    }
    if (!inst->use_empty())
    {
        Value* vec = UndefValue::get(inst->getType());
        IRBuilder<> builder(inst);
        for (unsigned int i = 0; i < cast<IGCLLVM::FixedVectorType>(inst->getType())->getNumElements(); i++)
        {
            Value* vecElement = builder.CreateExtractElement(urbRead, builder.getInt32(i));
            vec = builder.CreateInsertElement(vec, vecElement, builder.getInt32(i));
        }
        inst->replaceAllUsesWith(vec);
    }

}

void GeometryShaderLowering::AddSGVURBRead(
    llvm::Value* vertexIndex,
    SGVUsage usage,
    llvm::Instruction* inst)
{
    IRBuilder<> builder(inst);
    unsigned int quadOffset = GetURBReadOffset(usage).Count();
    Value* offset = builder.getInt32(quadOffset);
    Value* arguments[] = { vertexIndex, offset };

    // add urb read
    Instruction* urbRead = GenIntrinsicInst::Create(
        GenISAIntrinsic::getDeclaration(m_pModule, GenISAIntrinsic::GenISA_URBRead),
        arguments,
        VALUE_NAME("urbread"),
        inst);
    // extract the appropriate component
    Value* channelVal = builder.getInt32(GetChannel(usage).Count());
    Value* oneChannel = builder.CreateExtractElement(urbRead, channelVal);
    // and use it in place of the old value of GSsystemValue
    inst->replaceAllUsesWith(oneChannel);
}

QuadEltUnit GeometryShaderLowering::GetURBReadOffset(SGVUsage usage)
{
    switch (usage)
    {
    case POINT_WIDTH:
        return QuadEltUnit(0);

        // Explicit fall-through for all POSITION_X/Y/Z/W cases.
        // All these 4 will be read from the same QuadElement offset.
    case POSITION_X:
    case POSITION_Y:
    case POSITION_Z:
    case POSITION_W:
        return QuadEltUnit(1);

    case CLIP_DISTANCE_X:
    case CLIP_DISTANCE_Y:
    case CLIP_DISTANCE_Z:
    case CLIP_DISTANCE_W:
        return QuadEltUnit(2);

    default:
        IGC_ASSERT_MESSAGE(0, "Other URB offsets for GS not yet defined");
        break;
    }

    return QuadEltUnit(0);
}

Unit<Element> GeometryShaderLowering::GetChannel(SGVUsage usage)
{
    switch (usage)
    {
    case POSITION_X:
        return Unit<Element>(0);
    case POSITION_Y:
        return Unit<Element>(1);
    case POSITION_Z:
        return Unit<Element>(2);
    case POSITION_W:
        return Unit<Element>(3);
    case POINT_WIDTH:
        return Unit<Element>(3);
    case CLIP_DISTANCE_X:
        return Unit<Element>(0);
    case CLIP_DISTANCE_Y:
        return Unit<Element>(1);
    case CLIP_DISTANCE_Z:
        return Unit<Element>(2);
    case CLIP_DISTANCE_W:
        return Unit<Element>(3);
    default:
        IGC_ASSERT_MESSAGE(0, "Gs SGV Local offset not yet defined");
        break;
    }

    return Unit<Element>(0);
}

/// Returns URB offset where attribute with the given 'usage' and 'attributeIndex'
/// needs to be written.
QuadEltUnit GeometryShaderLowering::GetURBWriteOffset(
    ShaderOutputType usage)
{
    switch (usage)
    {
    case SHADER_OUTPUT_TYPE_POINTWIDTH:
    case SHADER_OUTPUT_TYPE_VIEWPORT_ARRAY_INDEX:
    case SHADER_OUTPUT_TYPE_RENDER_TARGET_ARRAY_INDEX:
        return QuadEltUnit(0);
    case SHADER_OUTPUT_TYPE_POSITION:
        return QuadEltUnit(1);
    case SHADER_OUTPUT_TYPE_CLIPDISTANCE_LO:
        return QuadEltUnit(2);
    case SHADER_OUTPUT_TYPE_CLIPDISTANCE_HI:
        return QuadEltUnit(3);
    case SHADER_OUTPUT_TYPE_DEFAULT:
    {
        // sum of vertex header size and the size of all preceding attributes
        return
            m_gsProps->GetProperties().Output().PerVertex().HeaderSize();
    }
    default:
        IGC_ASSERT_MESSAGE(0, "Unknown GS output type");
        break;
    }

    return QuadEltUnit(0);
}

void GeometryShaderLowering::SetMaskAndData(
    llvm::Instruction* inst,
    unsigned int& mask,
    llvm::Value* data[8])
{
    // argument positions in the argument list
    const uint usageArgPos = (GetOpCode(inst) == llvm_output) ? 4 :
        inst->getNumOperands() - 4; // 4 for OUTPUTGS

    for (unsigned int i = 0; i < 8; ++i)
    {
        data[i] = m_fpzero;
    }
    // handle cases of writes of single DWORDs that may happen in vertex header
    /// The first four DWORDs of VUE are as follows:
    /// +------------+
    /// |  reserved  |  <--offset = 0
    /// +------------+
    /// |  RTAI      |  <--offset = 1
    /// +------------+
    /// |  VAI       |  <--offset = 2
    /// +------------+
    /// | POINTWIDTH |  <--offset = 3
    /// +------------+

    const ShaderOutputType usage = static_cast<ShaderOutputType>(
        llvm::cast<llvm::ConstantInt>(inst->getOperand(usageArgPos))->getZExtValue());
    switch (usage)
    {
    case SHADER_OUTPUT_TYPE_RENDER_TARGET_ARRAY_INDEX:
        mask = 0x02;
        data[1] = inst->getOperand(0);
        break;

    case SHADER_OUTPUT_TYPE_VIEWPORT_ARRAY_INDEX:
        mask = 0x04;
        data[2] = inst->getOperand(0);
        break;

    case SHADER_OUTPUT_TYPE_POINTWIDTH:
        mask = 0x08;
        data[3] = inst->getOperand(0);
        break;
    default:
        const uint numDataArgs = inst->getNumOperands() - 4;
        IGC_ASSERT(numDataArgs == 4 || numDataArgs == 8);
        mask = (1u << numDataArgs) - 1u; // bit mask of 4 or 8 consecutive ones
        for (unsigned int i = 0; i < numDataArgs; ++i)
        {
            data[i] = inst->getOperand(i);
        }
        break;
    }
}

void GeometryShaderLowering::LowerControlHeader(llvm::Instruction* inst)
{
    const int emitCountPos = inst->getNumOperands() - 2;
    auto pConstVertexIndex = llvm::dyn_cast<ConstantInt>(inst->getOperand(emitCountPos));
    QuadEltUnit offset(0); /// offsets in URB are counted in 4 dwords
    // Add write to vertex count field for non-static number of output vertices
    IRBuilder<> irb(inst);
    Value* undef = llvm::UndefValue::get(Type::getFloatTy(m_pModule->getContext()));
    IGC::CodeGenContext* pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    if (pConstVertexIndex == nullptr || pCtx->platform.disableStaticVertexCount())
    {
        IGC_ASSERT(m_gsProps->GetProperties().Output().HasNonstaticVertexCount());
        // bitcast since data arguments of urbWrite are floats - ugly
        Value* fpEmitCount = irb.CreateBitCast(inst->getOperand(emitCountPos), irb.getFloatTy());
        llvm::Value* data[8] = {
            fpEmitCount, m_fpzero, m_fpzero, m_fpzero,
               m_fpzero, m_fpzero, m_fpzero, m_fpzero };
        Value* zero = irb.getInt32(offset.Count());

        if (m_gsProps->GetProperties().Output().HasVtxCountMsgHalfCLSize())
        {
            Value* data[8] = { fpEmitCount, undef, undef, undef, undef, undef, undef, undef };
            AddURBWrite(zero, 0xFF, data, inst);
        }
        else
        {
            // we need to write just one DWORD at offset zero
            AddURBWrite(zero, 0x1, data, inst);
        }

        OctEltUnit outputVertexCountFieldSize(1); // GS OUTPUT_VERTEX_COUNT_FIELD is 8 dwords
        offset = offset + outputVertexCountFieldSize;
    }

    // emit control data header URB writes
    if (m_gsProps->GetProperties().Output().ControlDataHeaderRequired())
    {
        // how many writes we need
        const uint numWrites = m_gsProps->GetProperties().Output().ControlDataHeaderSize().Count();
        const uint maxWriteSize = 8;

        // issue each of the URB writes
        for (unsigned int i = 0; i < numWrites; ++i)
        {
            llvm::Value* data[maxWriteSize];

            // TODO: for the last write, the size can be smaller in static case
            // depending on the number of vertices.
            const uint writeSize = maxWriteSize;

            for (unsigned int k = 0; k < writeSize; ++k)
            {
                if (m_gsProps->GetProperties().Output().ControlDataHeaderPaddingRequired())
                {
                    if (k <= (m_gsProps->GetProperties().Output().MaxVertexCount() / 32))
                        data[k] = m_fpzero; // zero only the valid part of Control Data Header
                    else
                        data[k] = undef;    // the rest left undefined and safe some 'mov' instructions
                }
                else
                {
                    // need to bitcast to float since arguments of URB write expect float data - ugly
                    data[k] = irb.CreateBitCast(inst->getOperand(maxWriteSize * i + k), irb.getFloatTy());
                }
            }
            // issue URB write
            Value* offsetVal = irb.getInt32(offset.Count());
            const unsigned int mask = (1 << writeSize) - 1; // always consecutive bits
            AddURBWrite(offsetVal, mask, data, inst);
            offset = offset + round_up<OctElement>(EltUnit(writeSize));
        }
    }
    m_instructionToRemove.push_back(inst);
}

void GeometryShaderLowering::AddVertexURBHeaders(
    llvm::Function& function,
    std::vector<llvm::Instruction*>& offsetInst)
{
    if (m_gsProps->GetProperties().Output().PerVertex().HeaderSize().Count() == 0)
    {
        return;
    }
    auto firstInstr = function.getEntryBlock().begin();
    auto numVertices = m_gsProps->GetProperties().Output().HasNonstaticVertexCount() ?
        m_gsProps->GetProperties().Output().MaxVertexCount() :
        m_gsProps->GetProperties().Output().ActualStaticVertexCount();

    auto outVertexSize = m_gsProps->GetProperties().Output().PerVertex().Size();
    auto globalHeaderSize = m_gsProps->GetProperties().Output().GlobalHeaderSize();
    Value* zero = ConstantFP::get(Type::getFloatTy(m_pModule->getContext()), 0.0f);
    const unsigned int writeSize = 4;
    Value* zeros[8] = { zero, zero, zero, zero, zero, zero, zero, zero };
    for (unsigned int i = 0; i < numVertices; ++i)
    {
        QuadEltUnit offset = outVertexSize * i + globalHeaderSize;
        Value* offsetVal = ConstantInt::get(
            Type::getInt32Ty(m_pModule->getContext()),
            offset.Count());
        offsetInst[offset.Count()] = &(*firstInstr);
        AddURBWrite(offsetVal, (1 << writeSize) - 1, zeros, &(*firstInstr));
    }
}

llvm::FunctionPass* IGC::createGeometryShaderLoweringPass()
{
    return new GeometryShaderLowering();
}

char GeometryShaderLowering::ID = 0;
