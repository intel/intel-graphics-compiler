/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/ScaledNumber.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/ComputeShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/messageEncoding.hpp"
#include "common/allocator.h"
#include "common/secure_mem.h"
#include <iStdLib/utility.h>
#include <iStdLib/FloatUtil.h>
#include <algorithm>
#include "Probe/Assertion.h"

using namespace llvm;

namespace IGC
{
    CComputeShader::CComputeShader(llvm::Function* pFunc, CShaderProgram* pProgram)
        : CComputeShaderCommon(pFunc, pProgram)
        , m_dispatchAlongY(false)
        , m_disableMidThreadPreemption(false)
        , m_hasSLM(false)
        , m_threadGroupModifier_X(0)
        , m_threadGroupModifier_Y(0)
    {
    }

    CComputeShader::~CComputeShader()
    {
    }

    void CComputeShader::ParseShaderSpecificOpcode(llvm::Instruction* inst)
    {
        if (LoadInst * load = dyn_cast<LoadInst>(inst))
        {
            BufferType bufType = GetBufferType(load->getPointerAddressSpace());
            if (bufType == RESOURCE || bufType == UAV || bufType == SLM)
            {
                m_numberOfUntypedAccess++;
            }
            if (bufType == SLM)
            {
                m_hasSLM = true;
                m_numSLMAccesses++;
            }
            else if (bufType == RESOURCE || bufType == UAV)
            {
                m_num1DAccesses++;
            }
        }
        else if (StoreInst * store = dyn_cast<StoreInst>(inst))
        {
            BufferType bufType = GetBufferType(store->getPointerAddressSpace());
            if (bufType == RESOURCE || bufType == UAV || bufType == SLM)
            {
                m_numberOfUntypedAccess++;
            }
            if (bufType == SLM)
            {
                m_numSLMAccesses++;
                m_hasSLM = true;
            }
            else if (bufType == RESOURCE || bufType == UAV)
            {
                m_num1DAccesses++;
            }
        }
        else if (GenIntrinsicInst * intr = dyn_cast<GenIntrinsicInst>(inst))
        {
            switch (intr->getIntrinsicID())
            {
            case GenISAIntrinsic::GenISA_storerawvector_indexed:
            case GenISAIntrinsic::GenISA_storeraw_indexed:
            case GenISAIntrinsic::GenISA_ldrawvector_indexed:
            case GenISAIntrinsic::GenISA_ldraw_indexed:
            {
                m_num1DAccesses++;
                break;
            }

            case GenISAIntrinsic::GenISA_typedwrite:
            case GenISAIntrinsic::GenISA_typedread:
                m_numberOfTypedAccess++;
                m_num2DAccesses++;
                break;
            case GenISAIntrinsic::GenISA_storestructured1:
            case GenISAIntrinsic::GenISA_storestructured2:
            case GenISAIntrinsic::GenISA_storestructured3:
            case GenISAIntrinsic::GenISA_storestructured4:
                m_numberOfUntypedAccess++;
                break;
            case GenISAIntrinsic::GenISA_ldstructured:
                m_numberOfUntypedAccess++;
                m_num1DAccesses++;
                break;
            case GenISAIntrinsic::GenISA_ldptr:
                if (llvm::ConstantInt * pInt = llvm::dyn_cast<llvm::ConstantInt>(intr->getOperand(1)))
                {
                    int index = int_cast<int>(pInt->getZExtValue());
                    index == 0 ? m_num1DAccesses++ : m_num2DAccesses++;
                }
                else
                {
                    m_num2DAccesses++;
                }
                break;
            case GenISAIntrinsic::GenISA_sampleptr:
            case GenISAIntrinsic::GenISA_sampleBptr:
            case GenISAIntrinsic::GenISA_sampleCptr:
            case GenISAIntrinsic::GenISA_sampleDptr:
            case GenISAIntrinsic::GenISA_sampleDCptr:
            case GenISAIntrinsic::GenISA_sampleLptr:
            case GenISAIntrinsic::GenISA_sampleLCptr:
            case GenISAIntrinsic::GenISA_sampleBCptr:

            case GenISAIntrinsic::GenISA_gather4ptr:
            case GenISAIntrinsic::GenISA_gather4Cptr:
            case GenISAIntrinsic::GenISA_gather4POptr:
            case GenISAIntrinsic::GenISA_gather4POCptr:
                if (llvm::ConstantInt* pInt = llvm::dyn_cast<llvm::ConstantInt>(intr->getOperand(2)))
                {
                    int index = int_cast<int>(pInt->getZExtValue());
                    index == 0 ? m_num1DAccesses++ : m_num2DAccesses++;
                }
                else
                {
                    m_num2DAccesses++;
                }
                break;
            case GenISAIntrinsic::GenISA_DCL_SystemValue:
                //emitLocal mask has to be set before real compile
                if (m_Platform->supportHWGenerateTID() && m_DriverInfo->SupportHWGenerateTID())
                    setEmitLocalMask(static_cast<SGVUsage>(llvm::cast<llvm::ConstantInt>(inst->getOperand(0))->getZExtValue()));
                break;
            default:
                break;
            }
        }
    }

    void CComputeShader::CreateThreadPayloadData(void*& pThreadPayload, uint& curbeTotalDataLength, uint& curbeReadLength)
    {
        if (m_Platform->supportLoadThreadPayloadForCompute())
        {
            if (m_enableHWGenerateLID)
            {
                curbeTotalDataLength = m_NOSBufferSize;
                curbeReadLength = 0;
                if (!curbeTotalDataLength)
                {
                    return;
                }

                //todo: this is to follow legacy behavior, but I do not know any API using below memory.
                //Not sure why legacy behavior allocates memory for nosdata at IGC side even though IGC never fills it.
                //when inline data is used, IGC assumes UMD is aware of m_NOSBufferSize includes inlinedata.size.
                typedef uint32_t ThreadPayloadEntry;
                unsigned threadPayloadEntries = curbeTotalDataLength / sizeof(ThreadPayloadEntry);
                ThreadPayloadEntry* pThreadPayloadMem =
                    (ThreadPayloadEntry*)IGC::aligned_malloc(threadPayloadEntries * sizeof(ThreadPayloadEntry), getGRFSize());
                std::fill(pThreadPayloadMem, pThreadPayloadMem + threadPayloadEntries, 0);
                pThreadPayload = pThreadPayloadMem;
                return;
            }
        }

        CComputeShaderCommon::CreateThreadPayloadData(
            pThreadPayload,
            curbeTotalDataLength,
            curbeReadLength,
            m_ThreadIDLayout);
     }

    void CComputeShader::InitEncoder(SIMDMode simdMode, bool canAbortOnSpill, ShaderDispatchMode shaderMode)
    {
        m_pThread_ID_in_Group_X = nullptr;
        m_pThread_ID_in_Group_Y = nullptr;
        m_pThread_ID_in_Group_Z = nullptr;
        m_numberOfTypedAccess = 0;
        m_numberOfUntypedAccess = 0;
        m_num1DAccesses = 0;
        m_num2DAccesses = 0;
        m_numSLMAccesses = 0;
        CShader::InitEncoder(simdMode, canAbortOnSpill, shaderMode);
    }

    CVariable* CComputeShader::CreateThreadIDsinGroup(SGVUsage channelNum)
    {
        switch (m_emitMask)
        {
        case IGC::CComputeShader::NONE:
            break;
        case IGC::CComputeShader::X:
            CreateThreadIDinGroup(THREAD_ID_IN_GROUP_X);
            break;
        case IGC::CComputeShader::XY:
            CreateThreadIDinGroup(THREAD_ID_IN_GROUP_X);
            CreateThreadIDinGroup(THREAD_ID_IN_GROUP_Y);
            break;
        case IGC::CComputeShader::XYZ:
            CreateThreadIDinGroup(THREAD_ID_IN_GROUP_X);
            CreateThreadIDinGroup(THREAD_ID_IN_GROUP_Y);
            CreateThreadIDinGroup(THREAD_ID_IN_GROUP_Z);
            break;
        default:
            break;
        }

        return CreateThreadIDinGroup(channelNum);
    }

    // The register payload layout for a compute shaders is
    // the following:
    //-------------------------------------------------------------------------
    //| GRF Register | Example | Description                                  |
    //-------------------------------------------------------------------------
    //| R0*          | R0*     | R0* header                                   |
    //-------------------------------------------------------------------------
    //| R1 R(m)      | n/a     | Constants from CURBE when CURBE is enabled,  |
    //|              |         | m is a non-negative value                    |
    //-------------------------------------------------------------------------
    //| R(m+1)       | R1      | In-line Data block from Media Object         |
    //-------------------------------------------------------------------------
    //
    // *  R0
    //    DWord Bit Description
    //    R0.7  31:0    Thread Group ID Z
    //    R0.6  31:0    Thread Group ID Y
    //    R0.5  31:10   Scratch Space Pointer
    //          7:0     FFTID.
    //    R0.4  31:5    Binding Table Pointer
    //    R0.3  31:5    Sampler State Pointer
    //          3:0     Per Thread Scratch Space
    //    R0.2  27:24   BarrierID
    //          8:4     Interface Descriptor Offset
    //    R0.1  31:0    Thread Group ID X
    //    R0.0  27:24   Shared Local Memory Index
    //          15:0    URB Handle

    void CComputeShader::AllocatePayload()
    {
        uint offset = 0;

        ComputeShaderContext* pctx =
            static_cast<ComputeShaderContext*>(GetContext());

        // R0 is used as a Predefined variable so that vISA doesn't free it later. In CS, we expect the
        // thread group id's in R0.
        IGC_ASSERT(GetR0());

        // We use predefined variables so offset has to be added for R0.
        offset += getGRFSize();

        bool bZeroIDs = !GetNumberOfId();
        bool bSupportLoadThreadPayload = m_Platform->supportLoadThreadPayloadForCompute();
        bZeroIDs &= !bSupportLoadThreadPayload;
        // for indirect threads data payload hardware doesn't allow empty per thread buffer
        // so we allocate a dummy thread id in case no IDs are used
        if (pctx->m_DriverInfo.UsesIndirectPayload() && bZeroIDs)
        {
            CreateThreadIDinGroup(THREAD_ID_IN_GROUP_X);
        }

        if (!pctx->m_DriverInfo.UsesIndirectPayload())
        {
            // Cross-thread constant data.
            AllocateNOSConstants(offset);
        }

        AllocatePerThreadConstantData(offset);

        if (bSupportLoadThreadPayload)
        {
            uint perThreadInputSize = offset - getGRFSize();
            encoder.GetVISAKernel()->AddKernelAttribute("PerThreadInputSize", sizeof(uint16_t), &perThreadInputSize);
        }

        // Cross-thread constant data.
        if (pctx->m_DriverInfo.UsesIndirectPayload())
        {
            AllocateNOSConstants(offset);
        }

        if (bSupportLoadThreadPayload)
        {
            encoder.GetVISAKernel()->AddKernelAttribute(
                "CrossThreadInputSize",
                sizeof(uint16_t),
                &m_NOSBufferSize);
        }
    }

    // Returns true when vISA_useInlineData option must set.
    // The vISA_useInlineData must be set when per-thread payload is loaded from
    // memory and there is inline data to pass.
    bool CComputeShader::passNOSInlineData()
    {
        if (IGC_GET_FLAG_VALUE(EnablePassInlineData) == -1) {
            return false;
        }
        const bool forceEnablePassInlineData = (IGC_GET_FLAG_VALUE(EnablePassInlineData) == 1);
        // Currently we cannot support InlineData in ZEBinary so always disable it
        auto modMD = static_cast<const ComputeShaderContext*>(GetContext())->getModuleMetaData();
        if (IGC_IS_FLAG_ENABLED(EnableZEBinary) || modMD->compOpt.EnableZEBinary)
            return false;

        const bool loadThreadPayload = m_Platform->supportLoadThreadPayloadForCompute();
        const bool hasConstants = pushInfo.constantReg.size() > 0 || modMD->MinNOSPushConstantSize > 0;
        const bool inlineDataSupportEnabled =
            (m_Platform->supportInlineData() &&
            (m_DriverInfo->UseInlineData() || forceEnablePassInlineData));

        const bool passInlineData = inlineDataSupportEnabled && loadThreadPayload && hasConstants;
        return passInlineData;
    }

    bool CComputeShader::loadThreadPayload()
    {
        return true;
    }

    void CShaderProgram::FillProgram(SComputeShaderKernelProgram* pKernelProgram)
    {
        CComputeShader* simd8Shader = static_cast<CComputeShader*>(GetShader(SIMDMode::SIMD8));
        CComputeShader* simd16Shader = static_cast<CComputeShader*>(GetShader(SIMDMode::SIMD16));
        CComputeShader* simd32Shader = static_cast<CComputeShader*>(GetShader(SIMDMode::SIMD32));

        ComputeShaderContext* pctx =
            static_cast<ComputeShaderContext*>(GetContext());
        RetryManager& retryMgr = GetContext()->m_retryManager;
        bool isLastTry = retryMgr.IsLastTry();

        float spillThreshold = pctx->GetSpillThreshold();

        if (hasShaderOutput(simd32Shader))
        {
            if (simd32Shader->m_spillCost <= spillThreshold || isLastTry)
            {
                if (retryMgr.GetSIMDEntry(SIMDMode::SIMD32) == nullptr)
                {
                    retryMgr.SaveSIMDEntry(SIMDMode::SIMD32, simd32Shader);
                    // clean shader entry in CShaderProgram to avoid the CShader
                    // object is destroyed
                    ClearShaderPtr(SIMDMode::SIMD32);
                }
                else
                {
                    IGC_ASSERT_MESSAGE(0, "should not compile again if already got a non spill kernel");
                }
            }
            else
            {
                simd32Shader->ProgramOutput()->Destroy();
            }
        }

        if (hasShaderOutput(simd16Shader))
        {
            if (simd16Shader->m_spillCost <= spillThreshold || isLastTry)
            {
                if (retryMgr.GetSIMDEntry(SIMDMode::SIMD16) == nullptr)
                {
                    retryMgr.SaveSIMDEntry(SIMDMode::SIMD16, simd16Shader);
                    // clean shader entry in CShaderProgram to avoid the CShader
                    // object is destroyed
                    ClearShaderPtr(SIMDMode::SIMD16);
                }
                else
                {
                    IGC_ASSERT_MESSAGE(0, "should not compile again if already got a non spill kernel");
                }
            }
            else
            {
                simd16Shader->ProgramOutput()->Destroy();
            }
        }

        if (hasShaderOutput(simd8Shader))
        {
            if (simd8Shader->m_spillCost == 0 || isLastTry)
            {
                if (retryMgr.GetSIMDEntry(SIMDMode::SIMD8) == nullptr)
                {
                    retryMgr.SaveSIMDEntry(SIMDMode::SIMD8, simd8Shader);
                    // clean shader entry in CShaderProgram to avoid the CShader
                    // object is destroyed
                    ClearShaderPtr(SIMDMode::SIMD8);
                }
                else
                {
                    IGC_ASSERT_MESSAGE(0, "should not compile again if already got a non spill kernel");
                }
            }
            else
            {
                simd8Shader->ProgramOutput()->Destroy();
            }
        }
    }

    void CComputeShader::FillProgram(SComputeShaderKernelProgram* pKernelProgram)
    {
        ComputeShaderContext* pctx =
            static_cast<ComputeShaderContext*>(GetContext());

        CreateGatherMap();
        CreateConstantBufferOutput(pKernelProgram);

        pKernelProgram->m_StagingCtx = pctx->m_StagingCtx;
        pKernelProgram->m_RequestStage2 = RequestStage2(pctx->m_CgFlag, pctx->m_StagingCtx);
        pKernelProgram->ConstantBufferLoaded = m_constantBufferLoaded;
        pKernelProgram->UavLoaded = m_uavLoaded;
        for (int i = 0; i < 4; i++)
        {
            pKernelProgram->ShaderResourceLoaded[i] = m_shaderResourceLoaded[i];
        }
        pKernelProgram->RenderTargetLoaded = m_renderTargetLoaded;

        pKernelProgram->hasControlFlow = m_numBlocks > 1 ? true : false;

        pKernelProgram->MaxNumberOfThreads = m_Platform->getMaxGPGPUShaderThreads() / GetShaderThreadUsageRate();
        pKernelProgram->FloatingPointMode = USC::GFX3DSTATE_FLOATING_POINT_IEEE_754;
        pKernelProgram->SingleProgramFlow = USC::GFX3DSTATE_PROGRAM_FLOW_MULTIPLE;
        pKernelProgram->CurbeReadOffset = 0;
        pKernelProgram->PhysicalThreadsInGroup = static_cast<int>(
            std::ceil((static_cast<float>(m_threadGroupSize) /
                static_cast<float>((numLanes(m_dispatchSize))))));

        pKernelProgram->BarrierUsed = this->GetHasBarrier();

        pKernelProgram->RoundingMode = USC::GFX3DSTATE_ROUNDING_MODE_ROUND_TO_NEAREST_EVEN;

        pKernelProgram->BarrierReturnGRFOffset = 0;

        pKernelProgram->GtwBypass = 1;
        pKernelProgram->GtwResetTimer = 0;

        pKernelProgram->URBEntriesNum = 0;
        pKernelProgram->URBEntryAllocationSize = 0;

        pKernelProgram->ThreadPayloadData = nullptr;
        CreateThreadPayloadData(
            pKernelProgram->ThreadPayloadData,
            pKernelProgram->CurbeTotalDataLength,
            pKernelProgram->CurbeReadLength);

        pKernelProgram->ThreadGroupSize = m_threadGroupSize;
        pKernelProgram->SlmSize = pctx->m_slmSize;

        pKernelProgram->ThreadGroupModifier_X = m_threadGroupModifier_X;
        pKernelProgram->ThreadGroupModifier_Y = m_threadGroupModifier_Y;

        pKernelProgram->CSHThreadDispatchChannel = 0;

        pKernelProgram->CompiledForIndirectPayload = pctx->m_DriverInfo.UsesIndirectPayload();

        pKernelProgram->DispatchAlongY = m_dispatchAlongY;

        pKernelProgram->NOSBufferSize = m_NOSBufferSize / getMinPushConstantBufferAlignmentInBytes();

        pKernelProgram->isMessageTargetDataCacheDataPort = isMessageTargetDataCacheDataPort;

        pKernelProgram->DisableMidThreadPreemption = m_disableMidThreadPreemption;

        pKernelProgram->BindingTableEntryCount = this->GetMaxUsedBindingTableEntryCount();

        if (m_enableHWGenerateLID) {
            pKernelProgram->generateLocalID = true;
            pKernelProgram->emitLocalMask = m_emitMask;
            pKernelProgram->walkOrder = m_walkOrder;
            pKernelProgram->emitInlineParameter = passNOSInlineData();
            pKernelProgram->localXMaximum = m_threadGroupSize_X - 1;
            pKernelProgram->localYMaximum = m_threadGroupSize_Y - 1;
            pKernelProgram->localZMaximum = m_threadGroupSize_Z - 1;
            pKernelProgram->tileY = (m_ThreadIDLayout == ThreadIDLayout::TileY);
        }
        //else
        //{     //use default values
        //    pKernelProgram->generateLocalID = false;
        //    pKernelProgram->emitLocalMask = static_cast<uint>(EMIT_LOCAL_MASK::NONE);
        //    pKernelProgram->walkOrder = static_cast<uint>(WALK_ORDER::WO_XYZ);
        //    pKernelProgram->localXMaximum = 0;
        //    pKernelProgram->localYMaximum = 0;
        //    pKernelProgram->localZMaximum = 0;
        //}
        pKernelProgram->hasEvalSampler = GetHasEval();
    }

    void CComputeShader::ExtractGlobalVariables()
    {
        llvm::Module* module = GetContext()->getModule();

        llvm::GlobalVariable* pGlobal = module->getGlobalVariable("ThreadGroupSize_X");
        m_threadGroupSize_X = int_cast<uint>(llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());

        pGlobal = module->getGlobalVariable("ThreadGroupSize_Y");
        m_threadGroupSize_Y = int_cast<uint>(llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());

        pGlobal = module->getGlobalVariable("ThreadGroupSize_Z");
        m_threadGroupSize_Z = int_cast<uint>(llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());

        m_threadGroupSize = m_threadGroupSize_X * m_threadGroupSize_Y * m_threadGroupSize_Z;

        pGlobal = module->getGlobalVariable("ThreadGroupModifier_X");
        if ((pGlobal != nullptr) && pGlobal->hasInitializer()) {
            m_threadGroupModifier_X = int_cast<uint>(llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());
        }
        pGlobal = module->getGlobalVariable("ThreadGroupModifier_Y");
        if ((pGlobal != nullptr) && pGlobal->hasInitializer()) {
            m_threadGroupModifier_Y = int_cast<uint>(llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());
        }
    }

    void CComputeShader::PreCompile()
    {
        CreateImplicitArgs();

        const ComputeShaderContext* pCtx =
            static_cast<const ComputeShaderContext*>(GetContext());

        if (m_Platform->supportHWGenerateTID() && m_DriverInfo->SupportHWGenerateTID()) {
            if (IGC_GET_FLAG_VALUE(DispatchGPGPUWalkerAlongYFirst) == 1 &&
                !pCtx->getModuleMetaData()->csInfo.disableDispatchAlongY) {
                m_dispatchAlongY = true;
            } else
                m_dispatchAlongY = false;
        }
        else
        // Assume DispatchGPGPUWalkerAlongYFirst is on here unless off explicitly
        if (IGC_GET_FLAG_VALUE(DispatchGPGPUWalkerAlongYFirst) == 0)
            m_dispatchAlongY = false;
        else if ((m_num2DAccesses > m_num1DAccesses) &&
            !pCtx->getModuleMetaData()->csInfo.disableLocalIdOrderOptimizations &&
            GetContext()->m_DriverInfo.SupportsDispatchGPGPUWalkerAlongYFirst())
        {
            m_dispatchAlongY = true;
        }
        selectWalkOrder(
            pCtx->m_UseLinearWalk,
            m_numberOfTypedAccess,
            m_numberOfUntypedAccess,
            m_num1DAccesses,
            m_num2DAccesses,
            m_numSLMAccesses,
            m_threadGroupSize_X,
            m_threadGroupSize_Y,
            m_threadGroupSize_Z);

        encoder.GetVISABuilder()->SetOption(vISA_autoLoadLocalID, m_enableHWGenerateLID);
    }

    void CComputeShader::AddPrologue()
    {
    }

    bool CComputeShader::HasFullDispatchMask()
    {
        if (GetThreadGroupSize() % numLanes(m_dispatchSize) == 0)
        {
            return true;
        }
        return false;
    }

    // CS codegen passes is added with below order:
    //   simd16, simd32, simd8
    bool CComputeShader::CompileSIMDSize(SIMDMode simdMode, EmitPass& EP, llvm::Function& F)
    {
        ComputeShaderContext* ctx = (ComputeShaderContext*)GetContext();

        if (!CompileSIMDSizeInCommon(simdMode))
        {
            // Even if the determination is that we shouldn't compile this
            // SIMD, if it's forced then that must be honored.
            return ctx->m_ForceOneSIMD;
        }

        if (ctx->m_ForceOneSIMD)
            return true;

        // this can be changed to SIMD32 if that is better after testing on HW
        SIMDMode DefaultSimdMode = SIMDMode::SIMD16;

        CShader* simd8Program = getSIMDEntry(ctx, SIMDMode::SIMD8);
        CShader* simd16Program = getSIMDEntry(ctx, SIMDMode::SIMD16);
        CShader* simd32Program = getSIMDEntry(ctx, SIMDMode::SIMD32);

        bool hasSimd8 = simd8Program && simd8Program->ProgramOutput()->m_programSize > 0;
        bool hasSimd16 = simd16Program && simd16Program->ProgramOutput()->m_programSize > 0;
        bool hasSimd32 = simd32Program && simd32Program->ProgramOutput()->m_programSize > 0;

        if (simdMode == SIMDMode::SIMD8 && !hasSimd16 && !hasSimd32)
            return true;

        ////////
        // dynamic rules
        ////////

        // if already has an entry from previous compilation, then skip
        if (ctx->m_retryManager.GetSIMDEntry(simdMode) != nullptr)
        {
            return false;
        }

        if (!ctx->m_retryManager.IsFirstTry())
        {
            ctx->ClearSIMDInfo(simdMode, ShaderDispatchMode::NOT_APPLICABLE);
            ctx->SetSIMDInfo(SIMD_RETRY, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
        }

        // check if SLM fits in DG2 DSS
        if (IGC_IS_FLAG_ENABLED(CheckCSSLMLimit) &&
            ctx->getModuleMetaData()->csInfo.waveSize == 0 &&
            ctx->platform.getPlatformInfo().eProductFamily == IGFX_DG2 &&
            m_DriverInfo->SupportCSSLMLimit() &&
            ctx->m_slmSize > 0)
        {
            unsigned int slmPerDSS = 0; // in kB
            switch (simdMode)
            {
                // # Unfused EU = 16
                // # HW threads/EU = 8
                // SIMD16: 16*8*16 = 2048 pixelsPerDSS
                // SIMD32: 16*8*32 = 4096 pixelsPerDSS
                //
                // To calculate SLM/DSS in kB
                // ThreadGroupSize (TGSize) is # pixels in Thread Group
                // (m_slmSize * pixelsPerDSS / TGSize) / 1024
            case SIMDMode::SIMD16:
                slmPerDSS = ctx->m_slmSize * 2 / ctx->GetThreadGroupSize();
                break;
            case SIMDMode::SIMD32:
                slmPerDSS = ctx->m_slmSize * 4 / ctx->GetThreadGroupSize();
                break;
            default:
                break;
            }

            if (slmPerDSS > 128) {
                ctx->SetSIMDInfo(SIMD_SKIP_PERF, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
                return false;
            }

        }

        // skip simd32 if simd16 spills
        if (simdMode == SIMDMode::SIMD32 && simd16Program &&
            simd16Program->m_spillSize > 0)
        {
            ctx->SetSIMDInfo(SIMD_SKIP_SPILL, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
            return false;
        }

        if (hasSimd16)  // got simd16 kernel, see whether compile simd32/simd8
        {
            if (simdMode == SIMDMode::SIMD32)
            {
                uint sendStallCycle = simd16Program->m_sendStallCycle;
                uint staticCycle = simd16Program->m_staticCycle;


                if ((sendStallCycle / (float)staticCycle > 0.2) ||
                    (m_Platform->AOComputeShadersSIMD32Mode() &&
                        m_threadGroupSize_X == 32 &&
                        m_threadGroupSize_Y == 32 &&
                        m_threadGroupSize_Z == 1))
                {
                    return true;
                }

                float occu16 = ctx->GetThreadOccupancy(SIMDMode::SIMD16);
                float occu32 = ctx->GetThreadOccupancy(SIMDMode::SIMD32);
                if (!ctx->isSecondCompile &&
                    (occu32 > occu16 ||
                    (occu32 == occu16 && m_Platform->loosenSimd32occu()) ||
                    (occu32 == occu16 && ctx->m_instrTypes.numBarrier)))
                {
                    return true;
                }

                ctx->SetSIMDInfo(SIMD_SKIP_THGRPSIZE, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
            }
            else    // SIMD8
            {
                if (simd16Program->m_spillCost <= ctx->GetSpillThreshold())
                {
                    ctx->SetSIMDInfo(SIMD_SKIP_PERF, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
                    return false;
                }
                else if (!ctx->m_retryManager.IsLastTry() && ctx->instrStat[LICM_STAT][EXCEED_THRESHOLD])
                {
                    // skip SIMD8 if LICM threshold is met, unless it's lastTry
                    ctx->SetSIMDInfo(SIMD_SKIP_REGPRES, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
                    return false;
                }
                else
                {
                    return true;
                }
            }
        }

        // static rules

        if (simdMode == SIMDMode::SIMD32)
        {
            if (IGC_IS_FLAG_ENABLED(EnableCSSIMD32) || DefaultSimdMode == SIMDMode::SIMD32)
            {
                return true;
            }
            if (m_threadGroupSize >= 256 && m_hasSLM &&
                !ctx->m_threadCombiningOptDone && !ctx->m_IsPingPongSecond)
            {
                return true;
            }
        }

        //Check if we should switch to SIMD16 based on number of load instr

        int sum = 0;
        int loadStoreCount = 0;
        for (auto BBI = F.getBasicBlockList().begin(); BBI != F.getBasicBlockList().end(); BBI++)
        {
            llvm::BasicBlock* BB = const_cast<llvm::BasicBlock*>(&*BBI);
            for (auto BI = BB->begin(), BE = BB->end(); BI != BE; ++BI)
            {
                if (isa<LoadInst>(&*BI) || isa<StoreInst>(&*BI))
                {
                    loadStoreCount++;
                }
            }
            sum += BB->size();
        }
        float denom = float(sum - loadStoreCount * 4);
        float loadThreshold = 0;
        if (denom > 0.0)
        {
            loadThreshold = ((float(loadStoreCount)) / denom);
        }
        if (loadThreshold > 0.15 && loadStoreCount > 10)
        {
            if (simdMode == SIMDMode::SIMD32 && hasSimd16)
            {
                SIMDMode changeSIMD = SIMDMode::SIMD16;
                ctx->SetSIMDInfo(SIMD_SKIP_PERF, changeSIMD, ShaderDispatchMode::NOT_APPLICABLE);
                return false;
            }
        }

        // default rules

        // Here we see if we have compiled a size for this shader already
        if ((simdMode == SIMDMode::SIMD8 && hasSimd8) ||
            (simdMode == SIMDMode::SIMD16 && hasSimd16))
        {
            return false;
        }
        else
            if (simdMode == SIMDMode::SIMD32)
            {
                if (hasSimd32 || ctx->isSecondCompile)
                {
                    return false;
                }

                if ((hasSimd8 || hasSimd16) && DefaultSimdMode != SIMDMode::SIMD32)
                {
                    return false;
                }
            }

        return true;
    }
}
