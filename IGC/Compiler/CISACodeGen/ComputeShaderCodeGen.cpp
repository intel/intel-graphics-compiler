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
        : CComputeShaderBase(pFunc, pProgram)
        , m_dispatchAlongY(false)
        , m_disableMidThreadPreemption(false)
        , m_hasSLM(false)
        , m_tileY(false)
        , m_walkOrder(WO_XYZ)
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
            }
            if (bufType == RESOURCE || bufType == UAV)
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
                m_hasSLM = true;
            }
        }
        else if (GenIntrinsicInst * intr = dyn_cast<GenIntrinsicInst>(inst))
        {
            switch (intr->getIntrinsicID())
            {
            case GenISAIntrinsic::GenISA_typedwrite:
            case GenISAIntrinsic::GenISA_typedread:
                m_numberOfTypedAccess++;
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
            default:
                break;
            }
        }
    }

    void CComputeShader::selectWalkOrder()
    {
        if ((m_numberOfTypedAccess >= m_numberOfUntypedAccess) &&
            m_threadGroupSize_Y % 4 == 0 &&
            IGC_IS_FLAG_ENABLED(UseTiledCSThreadOrder)) {
            m_tileY = true;
            m_walkOrder = WO_YXZ;
        }
    }

    void CComputeShader::CreateThreadPayloadData(void*& pThreadPayload, uint& curbeTotalDataLength, uint& curbeReadLength)
    {

        CComputeShaderBase::CreateThreadPayloadData(
            pThreadPayload,
            curbeTotalDataLength,
            curbeReadLength,
            m_tileY);
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
        CShader::InitEncoder(simdMode, canAbortOnSpill, shaderMode);
    }

    CVariable* CComputeShader::CreateThreadIDsinGroup(SGVUsage channelNum)
    {
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

        // R0 is used as a Predefined variable so that vISA doesn't free it later. In CS, we expect the
        // thread group id's in R0.
        IGC_ASSERT(GetR0());

        // We use predefined variables so offset has to be added for R0.
        offset += getGRFSize();

        bool bZeroIDs = !GetNumberOfId();
        // for indirect threads data payload hardware doesn't allow empty per thread buffer
        // so we allocate a dummy thread id in case no IDs are used
        if (bZeroIDs)
        {
            CreateThreadIDinGroup(THREAD_ID_IN_GROUP_X);
        }

        AllocatePerThreadConstantData(offset);

        // Cross-thread constant data.
        AllocateNOSConstants(offset);
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
        }
    }

    void CComputeShader::FillProgram(SComputeShaderKernelProgram* pKernelProgram)
    {
        CreateGatherMap();
        CreateConstantBufferOutput(pKernelProgram);

        pKernelProgram->ConstantBufferLoaded = m_constantBufferLoaded;
        pKernelProgram->UavLoaded = m_uavLoaded;
        for (int i = 0; i < 4; i++)
        {
            pKernelProgram->ShaderResourceLoaded[i] = m_shaderResourceLoaded[i];
        }
        pKernelProgram->RenderTargetLoaded = m_renderTargetLoaded;

        pKernelProgram->hasControlFlow = m_numBlocks > 1 ? true : false;

        pKernelProgram->MaxNumberOfThreads = m_Platform->getMaxGPGPUShaderThreads();
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

        pKernelProgram->CSHThreadDispatchChannel = 0;

        pKernelProgram->CompiledForIndirectPayload = 0;

        pKernelProgram->DispatchAlongY = m_dispatchAlongY;

        pKernelProgram->NOSBufferSize = m_NOSBufferSize / getGRFSize(); // in 256 bits

        pKernelProgram->isMessageTargetDataCacheDataPort = isMessageTargetDataCacheDataPort;

        pKernelProgram->DisableMidThreadPreemption = m_disableMidThreadPreemption;

        pKernelProgram->BindingTableEntryCount = this->GetMaxUsedBindingTableEntryCount();

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
    }

    void CComputeShader::PreCompile()
    {
        CreateImplicitArgs();

        if (IGC_IS_FLAG_ENABLED(DispatchGPGPUWalkerAlongYFirst) && (m_num2DAccesses > m_num1DAccesses))
        {
            m_dispatchAlongY = true;
        }
        selectWalkOrder();
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
        if (!CompileSIMDSizeInCommon())
            return false;

        // this can be changed to SIMD32 if that is better after testing on HW
        static const SIMDMode BestSimdMode = SIMDMode::SIMD16;

        ComputeShaderContext* ctx = (ComputeShaderContext*)GetContext();

        CShader* simd8Program = getSIMDEntry(ctx, SIMDMode::SIMD8);
        CShader* simd16Program = getSIMDEntry(ctx, SIMDMode::SIMD16);
        CShader* simd32Program = getSIMDEntry(ctx, SIMDMode::SIMD32);

        bool hasSimd8 = simd8Program && simd8Program->ProgramOutput()->m_programSize > 0;
        bool hasSimd16 = simd16Program && simd16Program->ProgramOutput()->m_programSize > 0;
        bool hasSimd32 = simd32Program && simd32Program->ProgramOutput()->m_programSize > 0;

        if (!ctx->m_retryManager.IsFirstTry())
        {
            ctx->ClearSIMDInfo(simdMode, ShaderDispatchMode::NOT_APPLICABLE);
            ctx->SetSIMDInfo(SIMD_RETRY, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
        }

        ////////
        // dynamic rules
        ////////

        // if already has an entry from previous compilation, then skip
        if (ctx->m_retryManager.GetSIMDEntry(simdMode) != nullptr)
        {
            return false;
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

                llvm::GlobalVariable* pGlobal = GetContext()->getModule()->getGlobalVariable("ThreadGroupSize_X");
                unsigned threadGroupSize_X = int_cast<unsigned>(llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());
                pGlobal = GetContext()->getModule()->getGlobalVariable("ThreadGroupSize_Y");
                unsigned threadGroupSize_Y = int_cast<unsigned>(llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());
                pGlobal = GetContext()->getModule()->getGlobalVariable("ThreadGroupSize_Z");
                unsigned threadGroupSize_Z = int_cast<unsigned>(llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());

                if ((sendStallCycle / (float)staticCycle > 0.2) ||
                    (m_Platform->AOComputeShadersSIMD32Mode() &&
                        threadGroupSize_X == 32 &&
                        threadGroupSize_Y == 32 &&
                        threadGroupSize_Z == 1))
                {
                    return true;
                }

                float occu16 = ctx->GetThreadOccupancy(SIMDMode::SIMD16);
                float occu32 = ctx->GetThreadOccupancy(SIMDMode::SIMD32);
                if (!ctx->isSecondCompile &&
                    (occu32 > occu16 ||
                    (occu32 == occu16 && ctx->m_instrTypes.hasBarrier)))
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
            if (IGC_IS_FLAG_ENABLED(EnableCSSIMD32) || BestSimdMode == SIMDMode::SIMD32)
            {
                return true;
            }
            if (m_threadGroupSize >= 256 && m_hasSLM &&
                !ctx->m_threadCombiningOptDone && !ctx->m_IsPingPongSecond)
            {
                return true;
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

                if ((hasSimd8 || hasSimd16) && BestSimdMode != SIMDMode::SIMD32)
                {
                    return false;
                }
            }

        return true;
    }
}
