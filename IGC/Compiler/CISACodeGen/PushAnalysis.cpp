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

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"
#include "common/LLVMUtils.h"
#include "LLVMWarningsPush.hpp"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Function.h"
#include "LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/GenCodeGenModule.h"
#include "Compiler/CISACodeGen/PushAnalysis.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/PixelShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/CISACodeGen.h"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "common/igc_regkeys.hpp"
#include "Compiler/IGCPassSupport.h"
#include "LLVM3DBuilder/MetadataBuilder.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/debug/Debug.hpp"
#include <list>
#include "Probe/Assertion.h"

/***********************************************************************************
This File contains the logic to decide for each inputs and constant if the data
should be pushed or pull.
In case the data is pushed we remove the load instruction replace the value by
a function argument so that the liveness calculated is correct

************************************************************************************/
using namespace llvm;
using namespace IGC::IGCMD;

namespace IGC
{
#define PASS_FLAG "igc-push-value-info"
#define PASS_DESCRIPTION "hold push value information"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true

    //undef macros to avoid redefinition warnings
#undef PASS_FLAG
#undef PASS_DESCRIPTION
#undef PASS_ANALYSIS

#define PASS_FLAG "igc-push-analysis"
#define PASS_DESCRIPTION "promotes the values to be arguments"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
    IGC_INITIALIZE_PASS_BEGIN(PushAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(PullConstantHeuristics)
        IGC_INITIALIZE_PASS_END(PushAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

        PushAnalysis::PushAnalysis()
        : ModulePass(ID)
        , m_PDT(nullptr)
        , m_cbToLoad((uint)-1)
        , m_maxStatelessOffset(0)
    {
        initializePushAnalysisPass(*PassRegistry::getPassRegistry());
    }

    const uint32_t PushAnalysis::MaxConstantBufferIndexSize = 256;

    /// The maximum number of input attributes that will be pushed as part of the payload.
    /// One attribute is 4 dwords, so e.g. 16 means we allocate max 64 GRFs for input payload.
    const uint32_t PushAnalysis::MaxNumOfPushedInputs = 24;

    /// The size occupied by the tessellation header in dwords
    const uint32_t PushAnalysis::TessFactorsURBHeader = 8;

    /// Maximum number of attributes pushed
    const uint32_t PushAnalysis::m_pMaxNumOfVSPushedInputs = 24;

    const uint32_t PushAnalysis::m_pMaxNumOfHSPushedInputs = 24;

    const uint32_t PushAnalysis::m_pMaxNumOfDSPushedInputs = 24;

    const uint32_t PushAnalysis::m_pMaxNumOfGSPushedInputs = 24;

    template < typename T > std::string to_string(const T& n)
    {
        std::ostringstream stm;
        stm << n;
        return stm.str();
    }

    Value* PushAnalysis::addArgumentAndMetadata(llvm::Type* pType, std::string argName, IGC::WIAnalysis::WIDependancy dependency)
    {
        auto pArgInfo = m_pFuncUpgrade.AddArgument(argName, pType);
        ModuleMetaData* modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
        IGC::ArgDependencyInfoMD argDepInfoMD;
        argDepInfoMD.argDependency = dependency;
        modMD->pushInfo.pushAnalysisWIInfos.push_back(argDepInfoMD);

        //Add it to arglist and increment Argument Index
        m_argList.push_back(pArgInfo);
        m_argIndex++;

        m_funcTypeChanged = true;

        return pArgInfo;
    }

    bool PushAnalysis::IsStatelessCBLoad(
        llvm::Instruction* inst,
        unsigned int& GRFOffset,
        unsigned int& offset)
    {
        /*
            %12 = call i64 @llvm.genx.GenISA.RuntimeValue(i32 2)
            %13 = add i64 %12, 16
            %14 = inttoptr i64 %13 to <3 x float> addrspace(2)*
            %15 = load <3 x float> addrspace(2)* %14, align 16
        */

        if (!llvm::isa<llvm::LoadInst>(inst))
            return false;

        m_PDT = &getAnalysis<PostDominatorTreeWrapperPass>(*m_pFunction).getPostDomTree();
        m_DT = &getAnalysis<DominatorTreeWrapperPass>(*m_pFunction).getDomTree();
        m_entryBB = &m_pFunction->getEntryBlock();

        // %15 = load <3 x float> addrspace(2)* %14, align 16
        llvm::LoadInst* pLoad = llvm::cast<llvm::LoadInst>(inst);
        uint address_space = pLoad->getPointerAddressSpace();
        if (address_space != ADDRESS_SPACE_CONSTANT)
            return false;

        Value* pAddress = pLoad->getOperand(pLoad->getPointerOperandIndex());
        // skip casts
        if (isa<IntToPtrInst>(pAddress) || isa<PtrToIntInst>(pAddress) || isa<BitCastInst>(pAddress))
        {
            pAddress = cast<Instruction>(pAddress)->getOperand(0);
        }

        if (GenIntrinsicInst * genIntr = dyn_cast<GenIntrinsicInst>(pAddress))
        {
            /*
            find 64 bit case
            %29 = call { i32, i32 } @llvm.genx.GenISA.ptr.to.pair.p2v3f32(<3 x float> addrspace(2)* %runtime_value_6)
            %30 = extractvalue { i32, i32 } %29, 1
            %31 = extractvalue { i32, i32 } %29, 0
            %32 = call <4 x float> addrspace(2)* @llvm.genx.GenISA.pair.to.ptr.p2v4f32(i32 %31, i32 %30)
                      or
            %33 = call { i32, i32 } @llvm.genx.GenISA.ptr.to.pair.p2v4f32(<4 x float> addrspace(2)* %runtime_value_4)
            %34 = extractvalue { i32, i32 } %33, 0
            %35 = extractvalue { i32, i32 } %33, 1
            %36 = call { i32, i32 } @llvm.genx.GenISA.add.pair(i32 %34, i32 %35, i32 368, i32 0)
            %37 = extractvalue { i32, i32 } %36, 0
            %38 = extractvalue { i32, i32 } %36, 1
            %39 = call <2 x i32> addrspace(2)* @llvm.genx.GenISA.pair.to.ptr.p2v2i32(i32 %37, i32 %38)
            */
            if (genIntr->getIntrinsicID() == GenISAIntrinsic::GenISA_pair_to_ptr)
            {
                ExtractValueInst* Lo = dyn_cast<ExtractValueInst>(genIntr->getOperand(0));
                ExtractValueInst* Hi = dyn_cast<ExtractValueInst>(genIntr->getOperand(1));
                if (Lo && Hi && Lo->getOperand(0) == Hi->getOperand(0))
                {
                    if (GenIntrinsicInst * genIntr2 = dyn_cast<GenIntrinsicInst>(Lo->getOperand(0)))
                    {
                        if (genIntr2->getIntrinsicID() == GenISAIntrinsic::GenISA_ptr_to_pair)
                        {
                            offset = 0;
                            pAddress = genIntr2->getOperand(0);
                        }

                        if (genIntr2->getIntrinsicID() == GenISAIntrinsic::GenISA_add_pair)
                        {
                            ExtractValueInst* Lo2 = dyn_cast<ExtractValueInst>(genIntr2->getOperand(0));
                            ExtractValueInst* Hi2 = dyn_cast<ExtractValueInst>(genIntr2->getOperand(1));
                            if (Lo2 && Hi2 && Lo2->getOperand(0) == Hi2->getOperand(0))
                            {
                                if (GenIntrinsicInst * ptrToPair = dyn_cast<GenIntrinsicInst>(Lo2->getOperand(0)))
                                {
                                    if (ptrToPair->getIntrinsicID() == GenISAIntrinsic::GenISA_ptr_to_pair)
                                    {
                                        ConstantInt* pConst = dyn_cast<llvm::ConstantInt>(genIntr2->getOperand(2));
                                        if (!pConst)
                                            return false;
                                        offset = (uint)pConst->getZExtValue();
                                        pAddress = ptrToPair->getOperand(0);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            /*
            find 32 bit case
            % 33 = ptrtoint <4 x float> addrspace(2)* %runtime_value_4 to i64
            % 34 = add i64 % 33, 368
            % chunkPtr = inttoptr i64 % 34 to <2 x i32> addrspace(2)*
            */

            offset = 0;
            // % 13 = add i64 % 12, 16
            // This add might or might not be present necessarily.
            if (BinaryOperator * pAdd = dyn_cast<BinaryOperator>(pAddress))
            {
                if (pAdd->getOpcode() == llvm::Instruction::Add)
                {
                    ConstantInt* pConst = dyn_cast<llvm::ConstantInt>(pAdd->getOperand(1));
                    if (!pConst)
                        return false;

                    pAddress = pAdd->getOperand(0);
                    offset = (uint)pConst->getZExtValue();
                }
            }

        }
        // skip casts
        while (1)
        {
            if (isa<IntToPtrInst>(pAddress) || isa<PtrToIntInst>(pAddress) || isa<BitCastInst>(pAddress))
            {
                pAddress = cast<Instruction>(pAddress)->getOperand(0);
            }
            else
            {
                break;
            }
        }

        llvm::GenIntrinsicInst* pRuntimeVal = llvm::dyn_cast<llvm::GenIntrinsicInst>(pAddress);

        if (pRuntimeVal == nullptr ||
            pRuntimeVal->getIntrinsicID() != llvm::GenISAIntrinsic::GenISA_RuntimeValue)
            return false;

        uint runtimeval0 = (uint)llvm::cast<llvm::ConstantInt>(pRuntimeVal->getOperand(0))->getZExtValue();
        PushInfo& pushInfo = m_context->getModuleMetaData()->pushInfo;

        // then check for static flag so that we can do push safely
        for (auto it : pushInfo.pushableAddresses)
        {
            if ((runtimeval0 * 4 == it.addressOffset) && (IGC_IS_FLAG_ENABLED(DisableStaticCheck) || it.isStatic))
            {
                GRFOffset = runtimeval0;
                return true;
            }
        }

        // otherwise the descriptor could bound to uninitialized buffer and we
        // need to avoid pushing in control flow
        // Find the return BB or the return BB before discard lowering.

        bool searchForRetBBBeforeDiscard = false;
        BasicBlock* retBB = m_PDT->getRootNode()->getBlock();
        if (!retBB)
        {
            auto& roots = m_PDT->getRoots();
            IGC_ASSERT_MESSAGE(roots.size() == 1, "Unexpected multiple roots");
            retBB = roots[0];
        }

        for (auto& II : m_pFunction->getEntryBlock())
        {
            if (isa<GenIntrinsicInst>(&II, GenISAIntrinsic::GenISA_InitDiscardMask))
            {
                searchForRetBBBeforeDiscard = true;
                break;
            }
        }

        if (searchForRetBBBeforeDiscard)
        {
            for (auto it = pred_begin(retBB), ie = pred_end(retBB); it != ie; ++it)
            {
                BasicBlock* predBB = *it;
                BranchInst* br = cast<BranchInst>(predBB->getTerminator());
                if (br->isUnconditional())
                {
                    retBB = predBB;
                    break;
                }
            }
        }

        if (m_DT->dominates(inst->getParent(), retBB))
        {
            for (auto it : pushInfo.pushableAddresses)
            {
                if (runtimeval0 * 4 == it.addressOffset)
                {
                    GRFOffset = runtimeval0;
                    return true;
                }
            }
        }

        return false;
    }

    //
    // Calculate the offset in buffer relative to the dynamic uniform buffer offset.
    //
    // Below is an example of pattern we're looking for:
    //
    //   %runtime_value_1 = call fast float @llvm.genx.GenISA.RuntimeValue(i32 1)
    //   %spv.bufferOffset.mdNode0.cr1 = bitcast float %runtime_value_1 to i32
    //   %1 = and i32 %spv.bufferOffset.mdNode1.cr1, -16
    //   %2 = add i32 %1, 24
    //   %fromGBP45 = inttoptr i32 %2 to float addrspace(65536)* // BTI = 0
    //   %ldrawidx46 = load float, float addrspace(65536)* %fromGBP45, align 4
    //
    // For the above example the the function will:
    //  - check if uniform buffer with bti=0 has its dynamic offset in
    //    runtime value 1
    //  - return relativeOffsetInBytes = 24
    // This method will return true if the input UBO is a dynamic uniform buffer
    // and the input offset value is a sum of buffer's dynamic offset and an
    // immediate constant value.
    //
    bool PushAnalysis::GetConstantOffsetForDynamicUniformBuffer(
        uint   bufferId, // buffer id (BTI)
        Value* offsetValue, // total buffer offset i.e. starting from 0
        uint& relativeOffsetInBytes) // offset in bytes starting from dynamic offset in buffer
    {
        if (ConstantInt * constOffset = dyn_cast<ConstantInt>(offsetValue))
        {
            relativeOffsetInBytes = int_cast<uint>(constOffset->getZExtValue());
            return true;
        }
        else if (Operator::getOpcode(offsetValue) == Instruction::Add)
        {
            const Instruction* addInst = cast<Instruction>(offsetValue);
            uint offset0 = 0, offset1 = 0;

            if (GetConstantOffsetForDynamicUniformBuffer(bufferId, addInst->getOperand(0), offset0) &&
                GetConstantOffsetForDynamicUniformBuffer(bufferId, addInst->getOperand(1), offset1))
            {
                relativeOffsetInBytes = offset0 + offset1;
                return true;
            }
        }
        else if (Operator::getOpcode(offsetValue) == Instruction::Mul)
        {
            const Instruction* mulInst = cast<Instruction>(offsetValue);
            uint a = 0, b = 0;

            if (GetConstantOffsetForDynamicUniformBuffer(bufferId, mulInst->getOperand(0), a) &&
                GetConstantOffsetForDynamicUniformBuffer(bufferId, mulInst->getOperand(1), b))
            {
                relativeOffsetInBytes = a * b;
                return true;
            }

        }
        else if (Operator::getOpcode(offsetValue) == Instruction::Shl)
        {
            const Instruction* shlInst = cast<Instruction>(offsetValue);
            uint offset = 0, bitShift = 0;

            if (GetConstantOffsetForDynamicUniformBuffer(bufferId, shlInst->getOperand(0), offset) &&
                GetConstantOffsetForDynamicUniformBuffer(bufferId, shlInst->getOperand(1), bitShift))
            {
                relativeOffsetInBytes = offset << bitShift;
                return true;
            }
        }
        else if (Operator::getOpcode(offsetValue) == Instruction::And)
        {
            const Instruction* andInst = cast<Instruction>(offsetValue);
            ConstantInt* src1 = dyn_cast<ConstantInt>(andInst->getOperand(1));
            if (src1 &&
                (int_cast<uint>(src1->getZExtValue()) == 0xFFFFFFE0 || int_cast<uint>(src1->getZExtValue()) == 0xFFFFFFF0) &&
                isa<BitCastInst>(andInst->getOperand(0)))
            {
                uint offset = 0;
                if (GetConstantOffsetForDynamicUniformBuffer(bufferId, andInst->getOperand(0), offset) &&
                    offset == 0)
                {
                    relativeOffsetInBytes = 0;
                    return true;
                }
            }
        }
        else if (Operator::getOpcode(offsetValue) == Instruction::Or)
        {
            Instruction* orInst = cast<Instruction>(offsetValue);
            Instruction* src0 = dyn_cast<Instruction>(orInst->getOperand(0));
            ConstantInt* src1 = dyn_cast<ConstantInt>(orInst->getOperand(1));
            if (src1 && src0 &&
                src0->getOpcode() == Instruction::And &&
                isa<ConstantInt>(src0->getOperand(1)))
            {
                uint orOffset = int_cast<uint>(src1->getZExtValue());
                uint andMask = int_cast<uint>(cast<ConstantInt>(src0->getOperand(1))->getZExtValue());
                uint offset = 0;
                if ((orOffset & andMask) == 0 &&
                    GetConstantOffsetForDynamicUniformBuffer(bufferId, src0->getOperand(0), offset) &&
                    offset == 0)
                {
                    relativeOffsetInBytes = orOffset;
                    return true;
                }
            }
        }
        else if (BitCastInst * bitCast = dyn_cast<BitCastInst>(offsetValue))
        {
            if (GenIntrinsicInst * genIntr = dyn_cast<GenIntrinsicInst>(bitCast->getOperand(0)))
            {
                if (genIntr->getIntrinsicID() == GenISAIntrinsic::GenISA_RuntimeValue)
                {
                    if (MDNode * bufIdMd = genIntr->getMetadata("dynamicBufferOffset.bufferId"))
                    {
                        ConstantInt* bufIdMdVal = mdconst::extract<ConstantInt>(bufIdMd->getOperand(0));
                        if (bufferId == int_cast<uint>(bufIdMdVal->getZExtValue()))
                        {
                            relativeOffsetInBytes = 0;
                            return true;
                        }
                    }
                }
            }
        }
        else if (IntToPtrInst * i2p = dyn_cast<IntToPtrInst>(offsetValue))
        {
            return GetConstantOffsetForDynamicUniformBuffer(bufferId, i2p->getOperand(0), relativeOffsetInBytes);
        }

        return false;
    }

    /// The constant-buffer id and element id must be compile-time immediate
    /// in order to be added to thread-payload
    bool PushAnalysis::IsPushableShaderConstant(Instruction* inst, uint& bufIdOrGRFOffset, uint& eltId, bool& isStateless)
    {
        Value* eltPtrVal = nullptr;
        if (!llvm::isa<llvm::LoadInst>(inst))
            return false;

        if (inst->getType()->isVectorTy())
        {
            if (!(inst->getType()->getVectorElementType()->isFloatTy() ||
                inst->getType()->getVectorElementType()->isIntegerTy(32)))
                return false;
        }
        else
        {
            if (!(inst->getType()->isFloatTy() || inst->getType()->isIntegerTy(32)))
                return false;
        }

        // \todo, not support vector-load yet
        if (IsLoadFromDirectCB(inst, bufIdOrGRFOffset, eltPtrVal))
        {
            isStateless = false;
            if (bufIdOrGRFOffset == getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData()->pushInfo.inlineConstantBufferSlot)
            {
                return false;
            }
            if (isa<ConstantPointerNull>(eltPtrVal))
            {
                eltId = 0;
                return true;
            }
            if (IntToPtrInst * i2p = dyn_cast<IntToPtrInst>(eltPtrVal))
            {
                Value* eltIdxVal = i2p->getOperand(0);
                if (ConstantInt * eltIdx = dyn_cast<ConstantInt>(eltIdxVal))
                {
                    eltId = int_cast<uint>(eltIdx->getZExtValue());
                    if ((eltId % 4) == 0)
                    {
                        return true;
                    }
                }
            }

            if (m_context->m_DriverInfo.SupportsDynamicUniformBuffers() && IGC_IS_FLAG_DISABLED(DisableSimplePushWithDynamicUniformBuffers))
            {
                unsigned int relativeOffsetInBytes = 0; // offset in bytes starting from dynamic buffer offset
                if (GetConstantOffsetForDynamicUniformBuffer(bufIdOrGRFOffset, eltPtrVal, relativeOffsetInBytes))
                {
                    eltId = relativeOffsetInBytes;
                    if ((eltId % 4) == 0)
                    {
                        return true;
                    }
                }
            }
            return false;
        }
        else if (IsStatelessCBLoad(inst, bufIdOrGRFOffset, eltId))
        {
            isStateless = true;
            return true;
        }
        return false;
    }

    bool PushAnalysis::AreUniformInputsBasedOnDispatchMode()
    {
        if (m_context->type == ShaderType::DOMAIN_SHADER)
        {
            return true;
        }
        else if (m_context->type == ShaderType::HULL_SHADER)
        {
            if (m_hsProps->GetProperties().m_pShaderDispatchMode != EIGHT_PATCH_DISPATCH_MODE)
            {
                return true;
            }
        }
        return false;
    }

    bool PushAnalysis::DispatchGRFHardwareWAForHSAndGSDisabled()
    {
        // If the WA does not apply, we can push constants.
        if (!m_context->platform.WaDispatchGRFHWIssueInGSAndHSUnit())
        {
            return true;
        }

        if (m_context->type == ShaderType::HULL_SHADER)
        {
            if (m_hsProps->GetProperties().m_pShaderDispatchMode == EIGHT_PATCH_DISPATCH_MODE)
            {
                auto tooManyHandles =
                    (m_hsProps->GetProperties().m_pInputControlPointCount >= 29);
                // Can push constants if urb handles don't take too many registers.
                return !tooManyHandles;
            }
            return true;
        }
        else if (m_context->type == ShaderType::GEOMETRY_SHADER)
        {
            // If we need to consider the WA, do the computations
            auto inputVertexCount = m_gsProps->GetProperties().Input().VertexCount();
            auto tooManyHandles =
                inputVertexCount > 13 || (inputVertexCount > 12 && m_gsProps->GetProperties().Input().HasPrimitiveID());

            // Can push constants if urb handles don't take too many registers.
            return !tooManyHandles;
        }
        return true;
    }



    bool PushAnalysis::CanPushConstants()
    {
        if (IGC_GET_FLAG_VALUE(DisablePushConstant) & 0x1 ||
            IGC_GET_FLAG_VALUE(DisablePushConstant) & (1 << static_cast<unsigned int>(m_context->type)))
        {
            return false;
        }

        if (!m_context->getModuleMetaData()->compOpt.PushConstantsEnable)
        {
            return false;
        }


        switch (m_context->type)
        {
        case ShaderType::VERTEX_SHADER:
            if (m_context->platform.WaDisableVSPushConstantsInFusedDownModeWithOnlyTwoSubslices())
            {
                return false;
            }
            return true;

        case ShaderType::HULL_SHADER:
            return DispatchGRFHardwareWAForHSAndGSDisabled() &&
                !m_context->m_DriverInfo.WaDisablePushConstantsForHS();

        case ShaderType::GEOMETRY_SHADER:
            return DispatchGRFHardwareWAForHSAndGSDisabled();

        case ShaderType::DOMAIN_SHADER:
            if (m_context->platform.WaDisableDSPushConstantsInFusedDownModeWithOnlyTwoSubslices())
            {
                return false;
            }
            return true;

        case ShaderType::PIXEL_SHADER:
        {
            NamedMDNode* coarseNode = m_pFunction->getParent()->getNamedMetadata(NAMED_METADATA_COARSE_PHASE);
            NamedMDNode* pixelNode = m_pFunction->getParent()->getNamedMetadata(NAMED_METADATA_PIXEL_PHASE);
            if (coarseNode && pixelNode)
            {
                Function* pixelPhase = llvm::mdconst::dyn_extract<Function>(pixelNode->getOperand(0)->getOperand(0));
                if (pixelPhase == m_pFunction)
                {
                    // no push constants for pixel phase
                    return false;
                }
            }
        }
        return true;
        default:
            break;
        }
        return false;
    }

    unsigned int PushAnalysis::GetMaxNumberOfPushedInputs()
    {

        if (IGC_GET_FLAG_VALUE(DisableAttributePush) & 0x1 ||
            IGC_GET_FLAG_VALUE(DisableAttributePush) & static_cast<unsigned int>(m_context->type))
        {
            return 0;
        }

        switch (m_context->type)
        {
        case ShaderType::VERTEX_SHADER:
            return m_pMaxNumOfVSPushedInputs;
        case ShaderType::HULL_SHADER:
            return m_hsProps->GetProperties().GetMaxInputPushed();
        case ShaderType::DOMAIN_SHADER:
            return m_pMaxNumOfDSPushedInputs;
        case ShaderType::GEOMETRY_SHADER:
        {
            const unsigned int MaxGSPayloadGRF = 96;
            const unsigned int totalPayloadSize = m_gsProps->GetProperties().Input().VertexCount() *
                Unit<Element>(m_gsProps->GetProperties().Input().PerVertex().Size()).Count();

            // For now we either return the vertex size (so we push all attributes) or zero
            // (so we use pure pull model).
            return totalPayloadSize <= MaxGSPayloadGRF ?
                m_gsProps->GetProperties().Input().PerVertex().Size().Count() : 0;
        }
        default:
            break;
        }
        return 0;
    }

    unsigned int PushAnalysis::AllocatePushedConstant(
        Instruction* load, unsigned int cbIdxOrGRFOffset, unsigned int offset, unsigned int maxSizeAllowed, bool isStateless)
    {
        if (cbIdxOrGRFOffset > m_context->m_DriverInfo.MaximumSimplePushBufferID())
        {
            return 0;
        }
        unsigned int size = (unsigned int)load->getType()->getPrimitiveSizeInBits() / 8;
        IGC_ASSERT_MESSAGE(isa<LoadInst>(load), "Expected a load instruction");
        PushInfo& pushInfo = m_context->getModuleMetaData()->pushInfo;

        bool canPromote = false;
        unsigned int sizeGrown = 0;
        // greedy allocation for now
        // first check if we are already pushing from the buffer
        unsigned int piIndex;
        bool cbIdxFound = false;

        //For stateless buffers, we store the GRFOffset into the simplePushInfoArr[piIndex].cbIdx
        //For stateful buffers, we store the bufferId into the simplePushInfoArr[piIndex].cbIdx
        //We traverse the array and identify which CB we're in based on either GFXOffset or bufferId
        for (piIndex = 0; piIndex < pushInfo.simplePushBufferUsed; piIndex++)
        {
            if (pushInfo.simplePushInfoArr[piIndex].cbIdx == cbIdxOrGRFOffset)
            {
                cbIdxFound = true;
                break;
            }
        }
        if (cbIdxFound)
        {
            unsigned int newStartOffset = iSTD::RoundDown(std::min(offset, pushInfo.simplePushInfoArr[piIndex].offset), getGRFSize());
            unsigned int newEndOffset = iSTD::Round(std::max(offset + size, pushInfo.simplePushInfoArr[piIndex].offset + pushInfo.simplePushInfoArr[piIndex].size), getGRFSize());
            unsigned int newSize = newEndOffset - newStartOffset;

            if (newSize - pushInfo.simplePushInfoArr[piIndex].size <= maxSizeAllowed)
            {
                sizeGrown = newSize - pushInfo.simplePushInfoArr[piIndex].size;
                canPromote = true;
                pushInfo.simplePushInfoArr[piIndex].offset = newStartOffset;
                pushInfo.simplePushInfoArr[piIndex].size = newSize;
                pushInfo.simplePushInfoArr[piIndex].isStateless = isStateless;
            }
        }

        const unsigned int maxNumberOfPushedBuffers = pushInfo.MaxNumberOfPushedBuffers;

        // we couldn't add it to an existing buffer try to add a new one if there is a slot available
        if (canPromote == false &&
            maxSizeAllowed > 0 &&
            pushInfo.simplePushBufferUsed < maxNumberOfPushedBuffers)
        {
            unsigned int newStartOffset = iSTD::RoundDown(offset, getGRFSize());
            unsigned int newEndOffset = iSTD::Round(offset + size, getGRFSize());
            unsigned int newSize = newEndOffset - newStartOffset;

            if (newSize <= maxSizeAllowed)
            {
                canPromote = true;
                sizeGrown = newSize;

                piIndex = pushInfo.simplePushBufferUsed;
                pushInfo.simplePushInfoArr[piIndex].cbIdx = cbIdxOrGRFOffset;
                pushInfo.simplePushInfoArr[piIndex].offset = newStartOffset;
                pushInfo.simplePushInfoArr[piIndex].size = newSize;
                pushInfo.simplePushInfoArr[piIndex].isStateless = isStateless;
                pushInfo.simplePushBufferUsed++;
            }
        }

        if (canPromote)
        {
            // promote the load to be pushed
            PromoteLoadToSimplePush(load, pushInfo.simplePushInfoArr[piIndex], offset);
        }
        return sizeGrown;
    }

    void PushAnalysis::PromoteLoadToSimplePush(Instruction* load, SimplePushInfo& info, unsigned int offset)
    {
        unsigned int num_elms = 1;
        llvm::Type* pTypeToPush = load->getType();
        llvm::Value* pReplacedInst = nullptr;
        llvm::Type* pScalarTy = pTypeToPush;

        if (pTypeToPush->isVectorTy())
        {
            num_elms = pTypeToPush->getVectorNumElements();
            pTypeToPush = pTypeToPush->getVectorElementType();
            llvm::Type* pVecTy = llvm::VectorType::get(pTypeToPush, num_elms);
            pReplacedInst = llvm::UndefValue::get(pVecTy);
            pScalarTy = pVecTy->getVectorElementType();
        }

        SmallVector< SmallVector<ExtractElementInst*, 1>, 4> extracts(num_elms);
        bool allExtract = LoadUsedByConstExtractOnly(cast<LoadInst>(load), extracts);

        for (unsigned int i = 0; i < num_elms; ++i)
        {
            uint address = offset + i * ((unsigned int)pScalarTy->getPrimitiveSizeInBits() / 8);
            auto it = info.simplePushLoads.find(address);
            llvm::Value* value = nullptr;
            if (it != info.simplePushLoads.end())
            {
                // Value is already getting pushed
                IGC_ASSERT_MESSAGE((it->second <= m_argIndex), "Function arguments list and metadata are out of sync!");
                value = m_argList[it->second];
                if (pTypeToPush != value->getType())
                    value = CastInst::CreateZExtOrBitCast(value, pTypeToPush, "", load);
            }
            else
            {
                value = addArgumentAndMetadata(pTypeToPush, VALUE_NAME("cb"), WIAnalysis::UNIFORM);
                if (pTypeToPush != value->getType())
                    value = CastInst::CreateZExtOrBitCast(value, pTypeToPush, "", load);

                info.simplePushLoads.insert(std::make_pair(address, m_argIndex));
            }

            if (load->getType()->isVectorTy())
            {
                if (!allExtract)
                {
                    pReplacedInst = llvm::InsertElementInst::Create(
                        pReplacedInst, value, llvm::ConstantInt::get(llvm::IntegerType::get(load->getContext(), 32), i), "", load);
                }
                else
                {
                    for (auto II : extracts[i])
                    {
                        II->replaceAllUsesWith(value);
                    }
                }
            }
            else
            {
                pReplacedInst = value;
            }

        }
        if (!allExtract)
        {
            load->replaceAllUsesWith(pReplacedInst);
        }
    }


    void PushAnalysis::BlockPushConstants()
    {
        // push up to 31 GRF
        static const unsigned int cthreshold = m_pullConstantHeuristics->getPushConstantThreshold(m_pFunction) * getGRFSize();
        unsigned int sizePushed = 0;
        m_entryBB = &m_pFunction->getEntryBlock();

        // Runtime values are changed to intrinsics. So we need to do it before.
        for (auto bb = m_pFunction->begin(), be = m_pFunction->end(); bb != be; ++bb)
        {
            for (auto i = bb->begin(), ie = bb->end(); i != ie; ++i)
            {
                unsigned int cbIdOrGRFOffset = 0;
                unsigned int offset = 0;
                bool isStateless = false;
                if (IsPushableShaderConstant(&(*i), cbIdOrGRFOffset, offset, isStateless))
                {
                    // convert offset in bytes
                    sizePushed += AllocatePushedConstant(&(*i), cbIdOrGRFOffset, offset, cthreshold - sizePushed, isStateless);
                }
            }
        }
    }

    PushConstantMode PushAnalysis::GetPushConstantMode()
    {
        PushConstantMode pushConstantMode = PushConstantMode::NO_PUSH_CONSTANT;
        if (CanPushConstants())
        {
            if (IGC_IS_FLAG_ENABLED(forcePushConstantMode))
            {
                pushConstantMode = (PushConstantMode)IGC_GET_FLAG_VALUE(forcePushConstantMode);
            }
            else if (m_context->m_DriverInfo.SupportsSimplePushOnly())
            {
                pushConstantMode = PushConstantMode::SIMPLE_PUSH;
            }
            else if (m_context->platform.supportsHardwareResourceStreamer())
            {
                pushConstantMode = PushConstantMode::GATHER_CONSTANT;
            }
            else if (m_context->m_DriverInfo.SupportsHWResourceStreameAndSimplePush())
            {
                pushConstantMode = PushConstantMode::SIMPLE_PUSH;
            }
            else
            {
                //CPU copy
                pushConstantMode = PushConstantMode::GATHER_CONSTANT;
            }
        }
        return pushConstantMode;
    }

    // process gather constants, update PushInfo.constants
    void PushAnalysis::processGather(Instruction* inst, uint bufId, uint eltId)
    {
        PushInfo& pushInfo = m_context->getModuleMetaData()->pushInfo;

        eltId = eltId >> 2;
        if (!m_context->m_DriverInfo.Uses3DSTATE_DX9_CONSTANT() &&
            (eltId + inst->getType()->getPrimitiveSizeInBits() / 8) >= (MaxConstantBufferIndexSize * 4))
        {
            // Hardware supports pushing more than 256 constants
            // in case 3DSTATE_DX9_CONSTANT mode is used
            return;
        }
        else if (bufId > 15)
        {
            // resource streamer cannot push above buffer slot 15 and driver doesn't allow
            // pushing inlined constant buffer
            // should not be pushed and should always be pulled
            return;
        }

        unsigned int num_elms =
            inst->getType()->isVectorTy() ? inst->getType()->getVectorNumElements() : 1;
        llvm::Type* pTypeToPush = inst->getType();
        llvm::Value* replaceVector = nullptr;
        unsigned int numberChannelReplaced = 0;
        SmallVector< SmallVector<ExtractElementInst*, 1>, 4> extracts(num_elms);
        bool allExtract = false;
        if (inst->getType()->isVectorTy())
        {
            allExtract = LoadUsedByConstExtractOnly(cast<LoadInst>(inst), extracts);
            pTypeToPush = inst->getType()->getVectorElementType();
        }

        for (unsigned int i = 0; i < num_elms; ++i)
        {
            if (allExtract && extracts[i].empty())
                continue;
            ConstantAddress address;
            address.bufId = bufId;
            address.eltId = eltId + i;

            auto it = pushInfo.constants.find(address);
            if (it != pushInfo.constants.end() ||
                (pushInfo.constantReg.size() + pushInfo.constants.size() < m_pullConstantHeuristics->getPushConstantThreshold(m_pFunction) * 8))
            {
                llvm::Value* value = nullptr;

                // The sum of all the 4 constant buffer read lengths must be <= size of 64 units.
                // Each unit is of size 256-bit units. In some UMDs we program the
                // ConstantRegisters and Constant Buffers in the ConstantBuffer0ReadLength. And this causes
                // shaders to crash when the read length is > 64 units. To be safer we program the total number of
                // GRF's used to 32 registers.
                if (it == pushInfo.constants.end())
                {
                    // We now put the Value as an argument to make sure its liveness starts
                    // at the beginning of the function and then we remove the instruction
                    // We now put the Value as an argument to make sure its liveness starts
                    // at the beginning of the function and then we remove the instruction
                    value = addArgumentAndMetadata(pTypeToPush,
                        VALUE_NAME(std::string("cb_") + to_string(bufId) + std::string("_") + to_string(eltId) + std::string("_elm") + to_string(i)),
                        WIAnalysis::UNIFORM);
                    if (pTypeToPush != value->getType())
                        value = CastInst::CreateZExtOrBitCast(value, pTypeToPush, "", inst);

                    pushInfo.constants[address] = m_argIndex;
                }
                else
                {
                    IGC_ASSERT_MESSAGE((it->second <= m_argIndex), "Function arguments list and metadata are out of sync!");
                    value = m_argList[it->second];
                    if (pTypeToPush != value->getType())
                        value = CastInst::CreateZExtOrBitCast(value, pTypeToPush, "", inst);
                }

                if (inst->getType()->isVectorTy())
                {
                    if (!allExtract)
                    {
                        numberChannelReplaced++;
                        if (replaceVector == nullptr)
                        {
                            replaceVector = llvm::UndefValue::get(inst->getType());
                        }
                        replaceVector = llvm::InsertElementInst::Create(
                            replaceVector, value, llvm::ConstantInt::get(llvm::IntegerType::get(inst->getContext(), 32), i), "", inst);
                    }
                    else
                    {
                        for (auto II : extracts[i])
                        {
                            II->replaceAllUsesWith(value);
                        }
                    }
                }
                else
                {
                    inst->replaceAllUsesWith(value);
                }
            }
            if (replaceVector != nullptr && numberChannelReplaced == num_elms)
            {
                // for vector replace, we only replace the load if we were going to push
                // all the channels
                inst->replaceAllUsesWith(replaceVector);
            }
        }
    }

    void PushAnalysis::processInput(Instruction* inst, bool gsInstancingUsed)
    {
        PushInfo& pushInfo = m_context->getModuleMetaData()->pushInfo;
        e_interpolation mode = (e_interpolation)llvm::cast<llvm::ConstantInt>(inst->getOperand(1))->getZExtValue();
        if (
            mode == EINTERPOLATION_VERTEX)
        {
            // inputs which get pushed are set as function arguments in order to have the correct liveness
            if (llvm::ConstantInt * pIndex = llvm::dyn_cast<llvm::ConstantInt>(inst->getOperand(0)))
            {
                SInputDesc input;
                input.index = static_cast<uint>(pIndex->getZExtValue());
                input.interpolationMode = mode;
                // if we know input is uniform, update WI analysis results.
                const bool uniformInput =
                    (mode == EINTERPOLATION_CONSTANT) || gsInstancingUsed;

                auto it = pushInfo.inputs.find(input.index);
                if (it == pushInfo.inputs.end())
                {
                    IGC_ASSERT(inst->getType()->isHalfTy() || inst->getType()->isFloatTy());
                    llvm::Type* floatTy = Type::getFloatTy(m_pFunction->getContext());
                    addArgumentAndMetadata(floatTy, VALUE_NAME(std::string("input_") + to_string(input.index)), uniformInput ? WIAnalysis::UNIFORM : WIAnalysis::RANDOM);
                    input.argIndex = m_argIndex;
                    pushInfo.inputs[input.index] = input;
                }
                else
                {
                    IGC_ASSERT_MESSAGE((it->second.argIndex <= m_argIndex), "Function arguments list and metadata are out of sync!");
                    input.argIndex = it->second.argIndex;
                }
                llvm::Value* replacementValue = m_argList[input.argIndex];
                if (inst->getType()->isHalfTy() && replacementValue->getType()->isFloatTy())
                {
                    // Input is accessed using the half version of intrinsic, e.g.:
                    //     call half @llvm.genx.GenISA.DCL.inputVec.f16 (i32 13, i32 2)
                    replacementValue = CastInst::CreateFPCast(replacementValue, inst->getType(), "", inst);
                }
                inst->replaceAllUsesWith(replacementValue);
            }
        }
    }

    void PushAnalysis::processInputVec(Instruction* inst, bool gsInstancingUsed)
    {
        PushInfo& pushInfo = m_context->getModuleMetaData()->pushInfo;

        // If the input index of llvm_shaderinputvec is a constant
        if (llvm::ConstantInt * pIndex = llvm::dyn_cast<llvm::ConstantInt>(inst->getOperand(0)))
        {
            uint inputIndex = static_cast<uint>(pIndex->getZExtValue());
            e_interpolation mode = (e_interpolation)llvm::cast<llvm::ConstantInt>(inst->getOperand(1))->getZExtValue();

            // If the input index of llvm_shaderinputvec is a constant then we pull inputs if inputIndex <= MaxNumOfPushedInputs
            if (pIndex && inputIndex <= MaxNumOfPushedInputs)
            {
                if (mode == EINTERPOLATION_CONSTANT || mode == EINTERPOLATION_VERTEX)
                {
                    for (auto I = inst->user_begin(), E = inst->user_end(); I != E; ++I)
                    {
                        if (llvm::ExtractElementInst * extract = llvm::dyn_cast<llvm::ExtractElementInst>(*I))
                        {
                            SInputDesc input;
                            // if input is i1.xyzw, inputIndex = 1*4, extract->getIndexOperand() is the component
                            input.index = inputIndex * 4 +
                                int_cast<uint>(llvm::cast<ConstantInt>(extract->getIndexOperand())->getZExtValue());
                            input.interpolationMode = mode;
                            // if we know input is uniform, update WI analysis results.
                            const bool uniformInput =
                                (mode == EINTERPOLATION_CONSTANT) || gsInstancingUsed;

                            auto it = pushInfo.inputs.find(input.index);
                            if (it == pushInfo.inputs.end())
                            {
                                addArgumentAndMetadata(extract->getType(), VALUE_NAME(std::string("pushedinput_") + to_string(input.index)), uniformInput ? WIAnalysis::UNIFORM : WIAnalysis::RANDOM);
                                input.argIndex = m_argIndex;
                                pushInfo.inputs[input.index] = input;
                            }
                            else
                            {
                                IGC_ASSERT_MESSAGE((it->second.argIndex <= m_argIndex), "Function arguments list and metadata are out of sync!");
                                input.argIndex = it->second.argIndex;
                            }

                            extract->replaceAllUsesWith(m_argList[input.argIndex]);
                        }
                    }
                }
            }
            else
            {
                // This should never happen for geometry shader since we leave GS specific
                // intrinsic if we want pull model earlier in GS lowering.
                IGC_ASSERT(m_context->type != ShaderType::GEOMETRY_SHADER);
            }
        }
    }

    void PushAnalysis::processURBRead(Instruction* inst, bool gsInstancingUsed,
        bool vsHasConstantBufferIndexedWithInstanceId,
        uint32_t vsUrbReadIndexForInstanceIdSGV)
    {
        PushInfo& pushInfo = m_context->getModuleMetaData()->pushInfo;

        if (llvm::ConstantInt * pVertexIndex = llvm::dyn_cast<llvm::ConstantInt>(inst->getOperand(0)))
        {
            uint vertexIndex = static_cast<uint>(pVertexIndex->getZExtValue());
            uint numberOfElementsPerVertexThatAreGoingToBePushed = GetMaxNumberOfPushedInputs();

            // If the input index of llvm_shaderinputvec is a constant
            if (llvm::ConstantInt * pElementIndex = llvm::dyn_cast<llvm::ConstantInt>(inst->getOperand(1)))
            {
                int urbOffset = static_cast<int>(pElementIndex->getZExtValue());
                int urbReadOffsetForPush = 0;

                if (m_context->type == ShaderType::HULL_SHADER)
                {
                    urbReadOffsetForPush = m_context->getModuleMetaData()->URBInfo.hasVertexHeader ?
                        (m_hsProps->GetProperties().m_HasClipCullAsInput ? 4 : 2)
                        : 0;
                }

                bool pushCondition = ((urbOffset >= urbReadOffsetForPush) && (urbOffset - urbReadOffsetForPush) < (int)numberOfElementsPerVertexThatAreGoingToBePushed);

                // If the attribute index of URBRead is a constant then we pull
                // inputs if elementIndex <= minElementsPerVertexThatCanBePushed
                if (pElementIndex && pushCondition)
                {
                    uint elementIndex = urbOffset - urbReadOffsetForPush;
                    uint currentElementIndex = (vertexIndex * numberOfElementsPerVertexThatAreGoingToBePushed * 4) + (elementIndex * 4);

                    for (auto I = inst->user_begin(), E = inst->user_end(); I != E; ++I)
                    {
                        llvm::ExtractElementInst* extract = llvm::dyn_cast<llvm::ExtractElementInst>(*I);
                        if (extract && llvm::isa<ConstantInt>(extract->getIndexOperand()))
                        {
                            SInputDesc input;
                            // if input is i1.xyzw, elementIndex = 1*4, extract->getIndexOperand() is the component
                            input.index =
                                currentElementIndex +
                                int_cast<uint>(cast<ConstantInt>(extract->getIndexOperand())->getZExtValue());
                            input.interpolationMode = AreUniformInputsBasedOnDispatchMode() ?
                                EINTERPOLATION_CONSTANT : EINTERPOLATION_VERTEX;

                            const bool uniformInput =
                                AreUniformInputsBasedOnDispatchMode() ||
                                (vsHasConstantBufferIndexedWithInstanceId &&
                                    vsUrbReadIndexForInstanceIdSGV == input.index) ||
                                gsInstancingUsed;

                            auto it = pushInfo.inputs.find(input.index);
                            if (it == pushInfo.inputs.end())
                            {
                                addArgumentAndMetadata(extract->getType(), VALUE_NAME(std::string("urb_read_") + to_string(input.index)), uniformInput ? WIAnalysis::UNIFORM : WIAnalysis::RANDOM);
                                input.argIndex = m_argIndex;
                                pushInfo.inputs[input.index] = input;
                            }
                            else
                            {
                                IGC_ASSERT_MESSAGE((it->second.argIndex <= m_argIndex), "Function arguments list and metadata are out of sync!");
                                input.argIndex = it->second.argIndex;
                            }
                            extract->replaceAllUsesWith(m_argList[input.argIndex]);
                        }
                    }
                }
            }
        }
    }

    // process runtimve value, update PushInfo.constantReg
    void PushAnalysis::processRuntimeValue(GenIntrinsicInst* intrinsic)
    {
        PushInfo& pushInfo = m_context->getModuleMetaData()->pushInfo;

        uint index = (uint)llvm::cast<llvm::ConstantInt>(intrinsic->getOperand(0))->getZExtValue();
        auto it = pushInfo.constantReg.find(index);
        Value* arg = nullptr;
        Value* runtimeValue = intrinsic;
        if (it == pushInfo.constantReg.end())
        {
            while (runtimeValue->hasOneUse() &&
                (isa<BitCastInst>(runtimeValue->user_back()) ||
                    isa<IntToPtrInst>(runtimeValue->user_back())))
            {
                runtimeValue = runtimeValue->user_back();
            }
            arg = addArgumentAndMetadata(
                runtimeValue->getType(),
                VALUE_NAME(std::string("runtime_value_") + to_string(index)),
                WIAnalysis::UNIFORM);
            pushInfo.constantReg[index] = m_argIndex;
        }
        else
        {
            IGC_ASSERT_MESSAGE(it->second <= m_argIndex, "Function arguments list and metadata are out of sync!");
            arg = m_argList[it->second];
            while (arg->getType() != runtimeValue->getType() &&
                runtimeValue->hasOneUse() &&
                (isa<BitCastInst>(runtimeValue->user_back()) ||
                    isa<IntToPtrInst>(runtimeValue->user_back())))
            {
                runtimeValue = runtimeValue->user_back();
            }
            if (arg->getType() != runtimeValue->getType())
            {
                arg = CastInst::CreateBitOrPointerCast(
                    arg, runtimeValue->getType(), "", cast<Instruction>(runtimeValue));
            }
        }
        runtimeValue->replaceAllUsesWith(arg);
    }

    //// Max number of control point inputs in 8 patch beyond which we always pull
    /// and do not try to use a hybrid approach of pull and push
    void PushAnalysis::ProcessFunction()
    {
        // if it's GS, get the properties object and find out if we use instancing
        // since then payload is laid out differently.
        const bool gsInstancingUsed = m_gsProps && m_gsProps->GetProperties().Input().HasInstancing();

        uint32_t vsUrbReadIndexForInstanceIdSGV = 0;
        bool vsHasConstantBufferIndexedWithInstanceId = false;

        m_funcTypeChanged = false;    // Reset flag at the beginning of processing every function
        if (m_context->type == ShaderType::VERTEX_SHADER)
        {
            llvm::NamedMDNode* pMetaData = m_pFunction->getParent()->getNamedMetadata("ConstantBufferIndexedWithInstanceId");

            if (pMetaData != nullptr)
            {
                llvm::MDNode* pMdNode = pMetaData->getOperand(0);
                if (pMdNode)
                {
                    ConstantInt* pURBReadIndexForInstanceIdSGV = mdconst::dyn_extract<ConstantInt>(pMdNode->getOperand(0));
                    vsUrbReadIndexForInstanceIdSGV = int_cast<uint32_t>(pURBReadIndexForInstanceIdSGV->getZExtValue());
                    vsHasConstantBufferIndexedWithInstanceId = true;
                }
            }
        }

        PushConstantMode pushConstantMode = GetPushConstantMode();

        if (m_context->type == ShaderType::DOMAIN_SHADER)
        {
            auto valueU = addArgumentAndMetadata(Type::getFloatTy(m_pFunction->getContext()), VALUE_NAME("DS_U"), WIAnalysis::RANDOM);
            auto valueV = addArgumentAndMetadata(Type::getFloatTy(m_pFunction->getContext()), VALUE_NAME("DS_V"), WIAnalysis::RANDOM);
            auto valueW = addArgumentAndMetadata(Type::getFloatTy(m_pFunction->getContext()), VALUE_NAME("DS_W"), WIAnalysis::RANDOM);
            m_dsProps->SetDomainPointUArgu(valueU);
            m_dsProps->SetDomainPointVArgu(valueV);
            m_dsProps->SetDomainPointWArgu(valueW);
        }

        if (pushConstantMode == PushConstantMode::SIMPLE_PUSH)
        {
            BlockPushConstants();
        }

        for (auto BB = m_pFunction->begin(), E = m_pFunction->end(); BB != E; ++BB)
        {
            llvm::BasicBlock::InstListType& instructionList = BB->getInstList();
            for (auto instIter = instructionList.begin(), endInstIter = instructionList.end(); instIter != endInstIter; ++instIter)
            {
                llvm::Instruction* inst = &(*instIter);
                // skip dead instructions
                if (inst->use_empty())
                {
                    continue;
                }
                // TODO: we can find a better heuristic to figure out which constant we want to push
                if (m_context->type == ShaderType::DOMAIN_SHADER)
                {
                    if (dyn_cast<GenIntrinsicInst>(inst) &&
                        dyn_cast<GenIntrinsicInst>(inst)->getIntrinsicID() == GenISAIntrinsic::GenISA_DCL_SystemValue)
                    {
                        SGVUsage usage = static_cast<SGVUsage>(llvm::dyn_cast<llvm::ConstantInt>(inst->getOperand(0))->getZExtValue());
                        if (usage == DOMAIN_POINT_ID_X || usage == DOMAIN_POINT_ID_Y || usage == DOMAIN_POINT_ID_Z)
                        {
                            if (usage == DOMAIN_POINT_ID_X)
                            {
                                inst->replaceAllUsesWith(m_dsProps->GetProperties().m_UArg);
                            }
                            else if (usage == DOMAIN_POINT_ID_Y)
                            {
                                inst->replaceAllUsesWith(m_dsProps->GetProperties().m_VArg);
                            }
                            else if (usage == DOMAIN_POINT_ID_Z)
                            {
                                inst->replaceAllUsesWith(m_dsProps->GetProperties().m_WArg);
                            }
                        }
                    }
                }
                // code to push constant-buffer value into thread-payload
                uint bufId, eltId;
                bool isStateless = false;
                if (pushConstantMode == PushConstantMode::GATHER_CONSTANT &&
                    IsPushableShaderConstant(inst, bufId, eltId, isStateless) &&
                    !isStateless)
                {
                    processGather(inst, bufId, eltId);
                }
                else if (GetOpCode(inst) == llvm_input)
                {
                    processInput(inst, gsInstancingUsed);
                }
                else if (GetOpCode(inst) == llvm_shaderinputvec)
                {
                    processInputVec(inst, gsInstancingUsed);
                }
                else if (GenIntrinsicInst * intrinsic = dyn_cast<GenIntrinsicInst>(inst))
                {
                    if (DispatchGRFHardwareWAForHSAndGSDisabled() &&
                        (intrinsic->getIntrinsicID() == GenISAIntrinsic::GenISA_URBRead) &&
                        (m_context->type != ShaderType::DOMAIN_SHADER))
                    {
                        processURBRead(inst, gsInstancingUsed,
                            vsHasConstantBufferIndexedWithInstanceId,
                            vsUrbReadIndexForInstanceIdSGV);
                    }
                    else if (intrinsic->getIntrinsicID() == GenISAIntrinsic::GenISA_RuntimeValue)
                    {
                        processRuntimeValue(intrinsic);
                    }
                }
            }
        }
        // WA: Gen11+ HW doesn't work correctly if doubles are on vertex shader input and the input has unused components,
        // so ElementComponentEnableMask is not full => packing occurs
        // Code below fills gaps in inputs, so the ElementComponentEnableMask if full even if we don't use all
        // components in shader. This is needed, because in case of doubles there can be no URBRead if xy or zw
        // components are not used.
        // Right now only OGL is affected, so there is special disableVertexComponentPacking flag set by GLSL FE
        // if there is double on input to vertex shader
        if (!m_context->getModuleMetaData()->pushInfo.inputs.empty() &&
            m_context->type == ShaderType::VERTEX_SHADER &&
            m_context->getModuleMetaData()->compOpt.disableVertexComponentPacking)
        {
            PushInfo& pushInfo = m_context->getModuleMetaData()->pushInfo;
            int maxIndex = pushInfo.inputs.rbegin()->first;
            for (int i = 0; i < maxIndex; ++i)
            {
                if (pushInfo.inputs.find(i) == pushInfo.inputs.end())
                {
                    SInputDesc input;
                    // if input is i1.xyzw, elementIndex = 1*4, extract->getIndexOperand() is the component
                    input.index = i;
                    input.interpolationMode = EINTERPOLATION_VERTEX;

                    const bool uniformInput =
                        AreUniformInputsBasedOnDispatchMode() ||
                        (vsHasConstantBufferIndexedWithInstanceId &&
                            vsUrbReadIndexForInstanceIdSGV == input.index) ||
                        gsInstancingUsed;

                    addArgumentAndMetadata(Type::getFloatTy(m_pFunction->getContext()), VALUE_NAME(std::string("urb_read_") + to_string(input.index)), uniformInput ? WIAnalysis::UNIFORM : WIAnalysis::RANDOM);
                    input.argIndex = m_argIndex;
                    pushInfo.inputs[input.index] = input;
                }
            }
        }

        if (m_funcTypeChanged)
        {
            m_isFuncTypeChanged[m_pFunction] = true;
        }
        else
        {
            m_isFuncTypeChanged[m_pFunction] = false;
        }

        m_pMdUtils->save(m_pFunction->getContext());
    }

    bool PushAnalysis::runOnModule(llvm::Module& M)
    {
        m_DL = &M.getDataLayout();
        m_pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
        m_pullConstantHeuristics = &getAnalysis<PullConstantHeuristics>();
        m_hsProps = getAnalysisIfAvailable<CollectHullShaderProperties>();
        m_dsProps = getAnalysisIfAvailable<CollectDomainShaderProperties>();
        m_gsProps = getAnalysisIfAvailable<CollectGeometryShaderProperties>();
        m_vsProps = getAnalysisIfAvailable<CollectVertexShaderProperties>();
        m_context = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

        MapList<Function*, Function*> funcsMapping;
        bool retValue = false;

        m_pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
        m_context = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

        for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
        {
            Function* pFunc = &(*I);

            // Only handle functions defined in this module
            if (pFunc->isDeclaration() || !isEntryFunc(m_pMdUtils, pFunc))
                continue;

            m_pFuncUpgrade.SetFunctionToUpgrade(pFunc);

            AnalyzeFunction(pFunc);

            // Find out if function pFunc's type/signature changed
            if (m_isFuncTypeChanged[pFunc])
            {
                retValue = true;

                // Create the new function body and insert it into the module
                Function* pNewFunc = m_pFuncUpgrade.RebuildFunction();

                // Reassign the arguments for domain shader to real arguments
                if (m_context->type == ShaderType::DOMAIN_SHADER)
                    // function right into the new function, leaving the old body of the function empty.
                {
                    // Loop over the argument list, transferring uses of the old arguments over to
                    // the new arguments
                    m_dsProps->SetDomainPointUArgu(
                        m_pFuncUpgrade.GetArgumentFromRebuild((LoadInst*)m_dsProps->GetDomainPointUArgu()));
                    m_dsProps->SetDomainPointVArgu(
                        m_pFuncUpgrade.GetArgumentFromRebuild((LoadInst*)m_dsProps->GetDomainPointVArgu()));
                    m_dsProps->SetDomainPointWArgu(
                        m_pFuncUpgrade.GetArgumentFromRebuild((LoadInst*)m_dsProps->GetDomainPointWArgu()));
                }

                // Map old func to new func
                funcsMapping[pFunc] = pNewFunc;

                // This is a kernel function, so there should not be any call site
                IGC_ASSERT(pFunc->use_empty());
            }
            m_pFuncUpgrade.Clean();
        }

        // Update IGC Metadata and shaders map
        // Function declarations are changing, this needs to be reflected in the metadata.
        MetadataBuilder mbuilder(&M);
        auto& FuncMD = m_context->getModuleMetaData()->FuncMD;
        for (auto i : funcsMapping)
        {
            auto oldFuncIter = m_pMdUtils->findFunctionsInfoItem(i.first);
            m_pMdUtils->setFunctionsInfoItem(i.second, oldFuncIter->second);
            m_pMdUtils->eraseFunctionsInfoItem(oldFuncIter);

            mbuilder.UpdateShadingRate(i.first, i.second);
            auto loc = FuncMD.find(i.first);
            if (loc != FuncMD.end())
            {
                auto funcInfo = loc->second;
                FuncMD.erase(i.first);
                FuncMD[i.second] = funcInfo;
            }
        }
        m_pMdUtils->save(M.getContext());

        GenXFunctionGroupAnalysis* FGA = getAnalysisIfAvailable<GenXFunctionGroupAnalysis>();

        // Go over all changed functions
        for (MapList<Function*, Function*>::const_iterator I = funcsMapping.begin(), E = funcsMapping.end(); I != E; ++I)
        {
            Function* pFunc = I->first;

            IGC_ASSERT_MESSAGE(pFunc->use_empty(), "Assume all user function are inlined at this point");

            if (FGA) {
                FGA->replaceEntryFunc(pFunc, I->second);
            }
            // Now, after changing funciton signature,
            // and validate there are no calls to the old function we can erase it.
            pFunc->eraseFromParent();
        }

        DumpLLVMIR(m_context, "push_analysis");

        return retValue;

    }
    char PushAnalysis::ID = 0;

}//namespace IGC
