/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <sstream>
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/ScaledNumber.h>
#include <llvm/Demangle/Demangle.h>
#include <llvm/IR/DebugInfo.h>
#include "common/LLVMWarningsPop.hpp"
#include "AdaptorCommon/RayTracing/RayTracingConstantsEnums.h"
#include "Compiler/CISACodeGen/ComputeShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Probe/Assertion.h"

namespace IGC
{

    typedef struct RetryState {
        bool allowLICM;
        bool allowCodeSinking;
        bool allowAddressArithmeticSinking;
        bool allowSimd32Slicing;
        bool allowPromotePrivateMemory;
        bool allowPreRAScheduler;
        bool allowVISAPreRAScheduler;
        bool allowLargeURBWrite;
        bool allowConstantCoalescing;
        unsigned nextState;
    } RetryState;

    static const RetryState RetryTable[] = {
        { true, true, false, false, true, true, true, true, true, 1 },
        { false, true, true, true, false, false, false, false, false, 500 }
    };

    RetryManager::RetryManager() : enabled(false), perKernel(false)
    {
        memset(m_simdEntries, 0, sizeof(m_simdEntries));
        firstStateId = IGC_GET_FLAG_VALUE(RetryManagerFirstStateId);
        stateId = firstStateId;
        IGC_ASSERT(stateId < getStateCnt());
    }

    bool RetryManager::AdvanceState() {
        if (!enabled || IGC_IS_FLAG_ENABLED(DisableRecompilation))
        {
            return false;
        }
        IGC_ASSERT(stateId < getStateCnt());
        stateId = RetryTable[stateId].nextState;
        return (stateId < getStateCnt());
    }
    bool RetryManager::AllowLICM() {
        IGC_ASSERT(stateId < getStateCnt());
        return RetryTable[stateId].allowLICM;
    }
    bool RetryManager::AllowAddressArithmeticSinking() {
        IGC_ASSERT(stateId < getStateCnt());
        return RetryTable[stateId].allowAddressArithmeticSinking;
    }
    bool RetryManager::AllowPromotePrivateMemory() {
        IGC_ASSERT(stateId < getStateCnt());
        return RetryTable[stateId].allowPromotePrivateMemory;
    }
    bool RetryManager::AllowPreRAScheduler() {
        IGC_ASSERT(stateId < getStateCnt());
        return RetryTable[stateId].allowPreRAScheduler;
    }
    bool RetryManager::AllowVISAPreRAScheduler() {
        IGC_ASSERT(stateId < getStateCnt());
        return RetryTable[stateId].allowVISAPreRAScheduler;
    }
    bool RetryManager::AllowCodeSinking() {
        IGC_ASSERT(stateId < getStateCnt());
        return RetryTable[stateId].allowCodeSinking;
    }
    bool RetryManager::AllowSimd32Slicing() {
        IGC_ASSERT(stateId < getStateCnt());
        return RetryTable[stateId].allowSimd32Slicing;
    }
    bool RetryManager::AllowLargeURBWrite() {
        IGC_ASSERT(stateId < getStateCnt());
        return RetryTable[stateId].allowLargeURBWrite;
    }
    bool RetryManager::AllowConstantCoalescing() {
        IGC_ASSERT(stateId < getStateCnt());
        return RetryTable[stateId].allowConstantCoalescing;
    }
    void RetryManager::SetFirstStateId(int id) {
        firstStateId = id;
    }
    bool RetryManager::IsFirstTry() {
        return (stateId == firstStateId);
    }
    bool RetryManager::IsLastTry() {
        return (!enabled ||
            IGC_IS_FLAG_ENABLED(DisableRecompilation) ||
            lastSpillSize < IGC_GET_FLAG_VALUE(AllowedSpillRegCount) ||
            (stateId < getStateCnt() && RetryTable[stateId].nextState >= getStateCnt()));
    }
    unsigned RetryManager::GetRetryId() const { return stateId; }

    void RetryManager::Enable() { enabled = true; }
    void RetryManager::Disable() {
        if (!perKernel) {
            enabled = false;
        }
    }

    void RetryManager::SetSpillSize(unsigned int spillSize) { lastSpillSize = spillSize; }
    unsigned int RetryManager::GetLastSpillSize() { return lastSpillSize; }

    void RetryManager::ClearSpillParams() {
        lastSpillSize = 0;
        numInstructions = 0;
    }

    // save entry for given SIMD mode, to avoid recompile for next retry.
    void RetryManager::SaveSIMDEntry(SIMDMode simdMode, CShader* shader)
    {
        switch (simdMode)
        {
        case SIMDMode::SIMD8:   m_simdEntries[0] = shader;  break;
        case SIMDMode::SIMD16:  m_simdEntries[1] = shader;  break;
        case SIMDMode::SIMD32:  m_simdEntries[2] = shader;  break;
        default:
            IGC_ASSERT(0);
            break;
        }
    }

    CShader* RetryManager::GetSIMDEntry(SIMDMode simdMode)
    {
        switch (simdMode)
        {
        case SIMDMode::SIMD8:   return m_simdEntries[0];
        case SIMDMode::SIMD16:  return m_simdEntries[1];
        case SIMDMode::SIMD32:  return m_simdEntries[2];
        default:
            IGC_ASSERT(0);
            return nullptr;
        }
    }

    RetryManager::~RetryManager()
    {
        for (unsigned i = 0; i < 3; i++)
        {
            if (m_simdEntries[i])
            {
                delete m_simdEntries[i];
            }
        }
    }

    bool RetryManager::AnyKernelSpills()
    {
        for (unsigned i = 0; i < 3; i++)
        {
            if (m_simdEntries[i] && m_simdEntries[i]->m_spillCost > 0.0)
            {
                return true;
            }
        }
        return false;
    }

    bool RetryManager::PickupKernels(CodeGenContext* cgCtx)
    {
        if (cgCtx->type == ShaderType::COMPUTE_SHADER)
        {
            return PickupCS(static_cast<ComputeShaderContext*>(cgCtx));
        }
        else
        {
            IGC_ASSERT_MESSAGE(0, "TODO for other shader types");
            return true;
        }
    }

    unsigned RetryManager::getStateCnt()
    {
        return sizeof(RetryTable) / sizeof(RetryState);
    };

    CShader* RetryManager::PickCSEntryForcedFromDriver(SIMDMode& simdMode, unsigned char forcedSIMDModeFromDriver)
    {
        if (forcedSIMDModeFromDriver == 8)
        {
            if ((m_simdEntries[0] && m_simdEntries[0]->m_spillSize == 0) || IsLastTry())
            {
                simdMode = SIMDMode::SIMD8;
                return m_simdEntries[0];
            }
        }
        else if (forcedSIMDModeFromDriver == 16)
        {
            if ((m_simdEntries[1] && m_simdEntries[1]->m_spillSize == 0) || IsLastTry())
            {
                simdMode = SIMDMode::SIMD16;
                return m_simdEntries[1];
            }
        }
        else if (forcedSIMDModeFromDriver == 32)
        {
            if ((m_simdEntries[2] && m_simdEntries[2]->m_spillSize == 0) || IsLastTry())
            {
                simdMode = SIMDMode::SIMD32;
                return m_simdEntries[2];
            }
        }
        return nullptr;
    }

    CShader* RetryManager::PickCSEntryByRegKey(SIMDMode& simdMode, ComputeShaderContext* cgCtx)
    {
        if (IGC_IS_FLAG_ENABLED(ForceCSSIMD32))
        {
            simdMode = SIMDMode::SIMD32;
            return m_simdEntries[2];
        }
        else
            if (IGC_IS_FLAG_ENABLED(ForceCSSIMD16) && m_simdEntries[1])
            {
                simdMode = SIMDMode::SIMD16;
                return m_simdEntries[1];
            }
            else
                if (IGC_IS_FLAG_ENABLED(ForceCSLeastSIMD)
                    || (IGC_IS_FLAG_ENABLED(ForceCSLeastSIMD4RQ) && cgCtx->hasSyncRTCalls())
                    )
                {
                    if (m_simdEntries[0])
                    {
                        simdMode = SIMDMode::SIMD8;
                        return m_simdEntries[0];
                    }
                    else
                        if (m_simdEntries[1])
                        {
                            simdMode = SIMDMode::SIMD16;
                            return m_simdEntries[1];
                        }
                        else
                        {
                            simdMode = SIMDMode::SIMD32;
                            return m_simdEntries[2];
                        }
                }

        return nullptr;
    }

    CShader* RetryManager::PickCSEntryEarly(SIMDMode& simdMode,
        ComputeShaderContext* cgCtx)
    {
        float spillThreshold = cgCtx->GetSpillThreshold();
        float occu8 = cgCtx->GetThreadOccupancy(SIMDMode::SIMD8);
        float occu16 = cgCtx->GetThreadOccupancy(SIMDMode::SIMD16);
        float occu32 = cgCtx->GetThreadOccupancy(SIMDMode::SIMD32);

        bool simd32NoSpill = m_simdEntries[2] && m_simdEntries[2]->m_spillCost <= spillThreshold;
        bool simd16NoSpill = m_simdEntries[1] && m_simdEntries[1]->m_spillCost <= spillThreshold;
        bool simd8NoSpill = m_simdEntries[0] && m_simdEntries[0]->m_spillCost <= spillThreshold;

        // If SIMD32/16/8 are all allowed, then choose one which has highest thread occupancy

        if (IGC_IS_FLAG_ENABLED(EnableHighestSIMDForNoSpill))
        {
            if (simd32NoSpill)
            {
                simdMode = SIMDMode::SIMD32;
                return m_simdEntries[2];
            }

            if (simd16NoSpill)
            {
                simdMode = SIMDMode::SIMD16;
                return m_simdEntries[1];
            }
        }
        else
        {
            if (simd32NoSpill)
            {
                if (occu32 >= occu16 && occu32 >= occu8)
                {
                    simdMode = SIMDMode::SIMD32;
                    return m_simdEntries[2];
                }
                // If SIMD32 doesn't spill, SIMD16 and SIMD8 shouldn't, if they exist
                IGC_ASSERT((m_simdEntries[0] == NULL) || simd8NoSpill == true);
                IGC_ASSERT((m_simdEntries[1] == NULL) || simd16NoSpill == true);
            }

            if (simd16NoSpill)
            {
                if (occu16 >= occu8 && occu16 >= occu32)
                {
                    simdMode = SIMDMode::SIMD16;
                    return m_simdEntries[1];
                }
                IGC_ASSERT_MESSAGE((m_simdEntries[0] == NULL) || simd8NoSpill == true, "If SIMD16 doesn't spill, SIMD8 shouldn't, if it exists");
            }
        }

        bool needToRetry = false;
        if (cgCtx->m_slmSize)
        {
            if (occu16 > occu8 || occu32 > occu16)
            {
                needToRetry = true;
            }
        }

        SIMDMode maxSimdMode = cgCtx->GetMaxSIMDMode();
        if (maxSimdMode == SIMDMode::SIMD8 || !needToRetry)
        {
            if (m_simdEntries[0] && m_simdEntries[0]->m_spillSize == 0)
            {
                simdMode = SIMDMode::SIMD8;
                return m_simdEntries[0];
            }
        }
        return nullptr;
    }

    CShader* RetryManager::PickCSEntryFinally(SIMDMode& simdMode)
    {
        if (m_simdEntries[0])
        {
            simdMode = SIMDMode::SIMD8;
            return m_simdEntries[0];
        }
        else
            if (m_simdEntries[1])
            {
                simdMode = SIMDMode::SIMD16;
                return m_simdEntries[1];
            }
            else
            {
                simdMode = SIMDMode::SIMD32;
                return m_simdEntries[2];
            }
    }

    void RetryManager::FreeAllocatedMemForNotPickedCS(SIMDMode simdMode)
    {
        if (simdMode != SIMDMode::SIMD8 && m_simdEntries[0] != nullptr)
        {
            if (m_simdEntries[0]->ProgramOutput()->m_programBin != nullptr)
                aligned_free(m_simdEntries[0]->ProgramOutput()->m_programBin);
        }
        if (simdMode != SIMDMode::SIMD16 && m_simdEntries[1] != nullptr)
        {
            if (m_simdEntries[1]->ProgramOutput()->m_programBin != nullptr)
                aligned_free(m_simdEntries[1]->ProgramOutput()->m_programBin);
        }
        if (simdMode != SIMDMode::SIMD32 && m_simdEntries[2] != nullptr)
        {
            if (m_simdEntries[2]->ProgramOutput()->m_programBin != nullptr)
                aligned_free(m_simdEntries[2]->ProgramOutput()->m_programBin);
        }
    }

    bool RetryManager::PickupCS(ComputeShaderContext* cgCtx)
    {
        SIMDMode simdMode = SIMDMode::UNKNOWN;
        CComputeShader* shader = nullptr;
        SComputeShaderKernelProgram* pKernelProgram = &cgCtx->programOutput;

        if (cgCtx->getModuleMetaData()->csInfo.forcedSIMDSize != 0)
        {
            shader = static_cast<CComputeShader*>(
                PickCSEntryForcedFromDriver(simdMode, cgCtx->getModuleMetaData()->csInfo.forcedSIMDSize));
        }
        if (!shader)
        {
            shader = static_cast<CComputeShader*>(
                PickCSEntryByRegKey(simdMode, cgCtx));
        }
        if (!shader)
        {
            shader = static_cast<CComputeShader*>(
                PickCSEntryEarly(simdMode, cgCtx));
        }
        if (!shader && IsLastTry())
        {
            shader = static_cast<CComputeShader*>(
                PickCSEntryFinally(simdMode));
            IGC_ASSERT(shader != nullptr);
        }

        if (shader)
        {
            switch (simdMode)
            {
            case SIMDMode::SIMD8:
                pKernelProgram->simd8 = *shader->ProgramOutput();
                pKernelProgram->SimdWidth = USC::GFXMEDIA_GPUWALKER_SIMD8;
                cgCtx->SetSIMDInfo(SIMD_SELECTED, simdMode,
                    ShaderDispatchMode::NOT_APPLICABLE);
                break;

            case SIMDMode::SIMD16:
                pKernelProgram->simd16 = *shader->ProgramOutput();
                pKernelProgram->SimdWidth = USC::GFXMEDIA_GPUWALKER_SIMD16;
                cgCtx->SetSIMDInfo(SIMD_SELECTED, simdMode,
                    ShaderDispatchMode::NOT_APPLICABLE);
                break;

            case SIMDMode::SIMD32:
                pKernelProgram->simd32 = *shader->ProgramOutput();
                pKernelProgram->SimdWidth = USC::GFXMEDIA_GPUWALKER_SIMD32;
                cgCtx->SetSIMDInfo(SIMD_SELECTED, simdMode,
                    ShaderDispatchMode::NOT_APPLICABLE);
                break;

            default:
                IGC_ASSERT_MESSAGE(0, "Invalie SIMDMode");
                break;
            }
            shader->FillProgram(pKernelProgram);
            pKernelProgram->SIMDInfo = cgCtx->GetSIMDInfo();

            // free allocated memory for the remaining kernels
            FreeAllocatedMemForNotPickedCS(simdMode);

            return true;
        }
        return false;
    }

    LLVMContextWrapper::LLVMContextWrapper(bool createResourceDimTypes)
    {
        if (createResourceDimTypes)
        {
            CreateResourceDimensionTypes(*this);
        }
    }

    void LLVMContextWrapper::AddRef()
    {
        refCount++;
    }

    void LLVMContextWrapper::Release()
    {
        refCount--;
        if (refCount == 0)
        {
            delete this;
        }
    }

    /** get shader's thread group size */
    unsigned ComputeShaderContext::GetThreadGroupSize()
    {
        llvm::GlobalVariable* pGlobal = getModule()->getGlobalVariable("ThreadGroupSize_X");
        m_threadGroupSize_X = int_cast<unsigned>(llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());

        pGlobal = getModule()->getGlobalVariable("ThreadGroupSize_Y");
        m_threadGroupSize_Y = int_cast<unsigned>(llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());

        pGlobal = getModule()->getGlobalVariable("ThreadGroupSize_Z");
        m_threadGroupSize_Z = int_cast<unsigned>(llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());

        return m_threadGroupSize_X * m_threadGroupSize_Y * m_threadGroupSize_Z;
    }

    unsigned ComputeShaderContext::GetSlmSizePerSubslice()
    {
        return platform.getSlmSizePerSsOrDss();
    }

    unsigned ComputeShaderContext::GetSlmSize() const
    {
        return m_slmSize;
    }

    float ComputeShaderContext::GetThreadOccupancy(SIMDMode simdMode)
    {
        return GetThreadOccupancyPerSubslice(simdMode, GetThreadGroupSize(), GetHwThreadsPerWG(platform), m_slmSize, GetSlmSizePerSubslice());
    }

    /** get smallest SIMD mode allowed based on thread group size */
    SIMDMode ComputeShaderContext::GetLeastSIMDModeAllowed()
    {
        SIMDMode mode = getLeastSIMDAllowed(
            GetThreadGroupSize(),
            GetHwThreadsPerWG(platform));
        return mode;
    }

    /** get largest SIMD mode for performance based on thread group size */
    SIMDMode ComputeShaderContext::GetMaxSIMDMode()
    {
        unsigned threadGroupSize = GetThreadGroupSize();
        SIMDMode mode;
        if (threadGroupSize <= 8)
        {
            mode = SIMDMode::SIMD8;
        }
        else if (threadGroupSize <= 16)
        {
            mode = SIMDMode::SIMD16;
        }
        else
        {
            mode = SIMDMode::SIMD32;
        }
        return mode;
    }

    float ComputeShaderContext::GetSpillThreshold() const
    {
        float spillThresholdSLM = platform.adjustedSpillThreshold() / 100.0f;
        //enable CSSpillThresholdSLM with desired value to override the default value.
        if(IGC_IS_FLAG_ENABLED(CSSpillThresholdSLM))
            spillThresholdSLM = float(IGC_GET_FLAG_VALUE(CSSpillThresholdSLM)) / 100.0f;
        float spillThresholdNoSLM =
            float(IGC_GET_FLAG_VALUE(CSSpillThresholdNoSLM)) / 100.0f;
        return m_slmSize ? spillThresholdSLM : spillThresholdNoSLM;
    }

    bool OpenCLProgramContext::isSPIRV() const
    {
        return isSpirV;
    }

    void OpenCLProgramContext::setAsSPIRV()
    {
        isSpirV = true;
    }
    float OpenCLProgramContext::getProfilingTimerResolution()
    {
        return m_ProfilingTimerResolution;
    }

    uint32_t OpenCLProgramContext::getNumThreadsPerEU() const
    {
        if (m_Options.IntelRequiredEUThreadCount)
        {
            return m_Options.requiredEUThreadCount;
        }
        if (m_InternalOptions.IntelNumThreadPerEU || m_InternalOptions.Intel256GRFPerThread)
        {
            return m_InternalOptions.numThreadsPerEU;
        }

        return 0;
    }

    uint32_t OpenCLProgramContext::getNumGRFPerThread() const
    {
        if (platform.supportsStaticRegSharing())
        {
            if (m_InternalOptions.Intel128GRFPerThread)
            {
                return 128;
            }
            else if (m_InternalOptions.Intel256GRFPerThread)
            {
                return 256;
            }
        }
        return CodeGenContext::getNumGRFPerThread();
    }

    bool OpenCLProgramContext::forceGlobalMemoryAllocation() const
    {
        return m_InternalOptions.ForceGlobalMemoryAllocation;
    }

    bool OpenCLProgramContext::allocatePrivateAsGlobalBuffer() const
    {
        return forceGlobalMemoryAllocation() || (m_instrTypes.hasDynamicGenericLoadStore && platform.canForcePrivateToGlobal());
    }

    bool OpenCLProgramContext::enableTakeGlobalAddress() const
    {
        return m_Options.EnableTakeGlobalAddress || getModuleMetaData()->capabilities.globalVariableDecorationsINTEL;
    }

    int16_t OpenCLProgramContext::getVectorCoalescingControl() const
    {
        // cmdline option > registry key
        int val = m_InternalOptions.VectorCoalescingControl;
        if (val < 0)
        {
            // no cmdline option
            val = IGC_GET_FLAG_VALUE(VATemp);
        }
        return val;
    }

    uint32_t OpenCLProgramContext::getPrivateMemoryMinimalSizePerThread() const
    {
        return m_InternalOptions.IntelPrivateMemoryMinimalSizePerThread;
    }

    uint32_t OpenCLProgramContext::getIntelScratchSpacePrivateMemoryMinimalSizePerThread() const
    {
        return m_InternalOptions.IntelScratchSpacePrivateMemoryMinimalSizePerThread;
    }

    void OpenCLProgramContext::failOnSpills()
    {
        if (!m_InternalOptions.FailOnSpill)
        {
            return;
        }
        // If there is fail-on-spill option provided
        // and __attribute__((annotate("igc-do-not-spill"))) is present for a kernel,
        // we fail compilation
        auto& programList = m_programOutput.m_ShaderProgramList;
        for (auto& kernel : programList)
        {
            for (auto mode : { SIMDMode::SIMD8, SIMDMode::SIMD16, SIMDMode::SIMD32 })
            {
                COpenCLKernel* shader = static_cast<COpenCLKernel*>(kernel->GetShader(mode));

                if (!COpenCLKernel::IsValidShader(shader))
                {
                    continue;
                }

                auto& funcMD = modMD->FuncMD[shader->entry];
                auto& annotatnions = funcMD.UserAnnotations;
                auto output = shader->ProgramOutput();

                if (output->m_scratchSpaceUsedBySpills > 0 &&
                    std::find(annotatnions.begin(), annotatnions.end(), "igc-do-not-spill") != annotatnions.end())
                {
                    std::string msg =
                        "Spills detected in kernel: "
                        + shader->m_kernelInfo.m_kernelName;
                    EmitError(msg.c_str(), nullptr);
                }
            }
        }
    }

    void OpenCLProgramContext::InternalOptions::parseOptions(const char* IntOptStr)
    {
        // Assume flags is in the form: <f0>[=<v0>] <f1>[=<v1>] ...
        // flag name and its value are either seperated by ' ' or '=';
        // flag seperator is always ' '.
        const char* NAMESEP = " =";  // separator b/w name and its value

        llvm::StringRef opts(IntOptStr);
        size_t Pos = 0;
        while (Pos != llvm::StringRef::npos)
        {
            // Get a flag name
            Pos = opts.find_first_not_of(' ', Pos);
            if (Pos == llvm::StringRef::npos)
                continue;

            size_t ePos = opts.find_first_of(NAMESEP, Pos);
            llvm::StringRef flagName = opts.substr(Pos, ePos - Pos);

            // Build options:  -cl-intel-xxxx, -ze-intel-xxxx, -ze-opt-xxxx
            //                 -cl-xxxx, -ze-xxxx
            // Both cl version and ze version means the same thing.
            // Here, strip off common prefix.
            size_t prefix_len;
            if (flagName.startswith("-cl-intel") || flagName.startswith("-ze-intel"))
            {
                prefix_len = 9;
            }
            else if (flagName.startswith("-ze-opt"))
            {
                prefix_len = 7;
            }
            else if (flagName.startswith("-cl") || flagName.startswith("-ze"))
            {
                prefix_len = 3;
            }
            else
            {
                // not a valid flag, skip
                Pos = opts.find_first_of(' ', Pos);
                continue;
            }

            llvm::StringRef suffix = flagName.drop_front(prefix_len);
            if (suffix.equals("-replace-global-offsets-by-zero"))
            {
                replaceGlobalOffsetsByZero = true;
            }
            else if (suffix.equals("-kernel-debug-enable"))
            {
                KernelDebugEnable = true;
            }
            else if (suffix.equals("-include-sip-csr"))
            {
                IncludeSIPCSR = true;
            }
            else if (suffix.equals("-include-sip-kernel-debug"))
            {
                IncludeSIPKernelDebug = true;
            }
            else if (suffix.equals("-include-sip-kernel-local-debug"))
            {
                IncludeSIPKernelDebugWithLocalMemory = true;
            }
            else if (suffix.equals("-use-32bit-ptr-arith"))
            {
                Use32BitPtrArith = true;
            }

            // -cl-intel-greater-than-4GB-buffer-required, -ze-opt-greater-than-4GB-buffer-required
            else if (suffix.equals("-greater-than-4GB-buffer-required"))
            {
                IntelGreaterThan4GBBufferRequired = true;
            }

            // -cl-intel-has-buffer-offset-arg, -ze-opt-has-buffer-offset-arg
            else if (suffix.equals("-has-buffer-offset-arg"))
            {
                IntelHasBufferOffsetArg = true;
            }

            // -cl-intel-buffer-offset-arg-required, -ze-opt-buffer-offset-arg-required
            else if (suffix.equals("-buffer-offset-arg-required"))
            {
                IntelBufferOffsetArgOptional = false;
            }

            // -cl-intel-has-positive-pointer-offset, -ze-opt-has-positive-pointer-offset
            else if (suffix.equals("-has-positive-pointer-offset"))
            {
                IntelHasPositivePointerOffset = true;
            }

            // -cl-intel-has-subDW-aligned-ptr-arg, -ze-opt-has-subDW-aligned-ptr-arg
            else if (suffix.equals("-has-subDW-aligned-ptr-arg"))
            {
                IntelHasSubDWAlignedPtrArg = true;
            }

            // -cl-intel-disable-a64WA
            else if (suffix.equals("-disable-a64WA"))
            {
                IntelDisableA64WA = true;
            }

            // -cl-intel-force-enable-a64WA
            else if (suffix.equals("-force-enable-a64WA"))
            {
                IntelForceEnableA64WA = true;
            }

            // GTPin flags used by L0 driver runtime
            // -cl-intel-gtpin-rera
            else if (suffix.equals("-gtpin-rera"))
            {
                GTPinReRA = true;
            }
            else if (suffix.equals("-gtpin-grf-info"))
            {
                GTPinGRFInfo = true;
            }
            else if (suffix.equals("-gtpin-scratch-area-size"))
            {
                GTPinScratchAreaSize = true;
                size_t valStart = opts.find_first_not_of(' ', ePos + 1);
                size_t valEnd = opts.find_first_of(' ', valStart);
                llvm::StringRef valStr = opts.substr(valStart, valEnd - valStart);
                if (valStr.getAsInteger(10, GTPinScratchAreaSizeValue))
                {
                    IGC_ASSERT(0);
                }
                Pos = valEnd;
                continue;
            }
            else if (suffix.equals("-gtpin-indir-ref"))
            {
                GTPinIndirRef = true;
            }

            // -cl-intel-no-prera-scheduling
            else if (suffix.equals("-no-prera-scheduling"))
            {
                IntelEnablePreRAScheduling = false;
            }
            // -cl-intel-force-global-mem-allocation
            else if (suffix.equals("-force-global-mem-allocation"))
            {
                ForceGlobalMemoryAllocation = true;
            }

            //
            // Options to set the number of GRF and threads
            // (All start with -cl-intel or -ze-opt)
            else if (suffix.equals("-128-GRF-per-thread"))
            {
                Intel128GRFPerThread = true;
                numThreadsPerEU = 8;
            }
            else if (suffix.equals("-256-GRF-per-thread") ||
                suffix.equals("-large-register-file"))
            {
                Intel256GRFPerThread = true;
                numThreadsPerEU = 4;
            }
            else if (suffix.equals("-num-thread-per-eu"))
            {
                IntelNumThreadPerEU = true;

                // Take an integer value after this option:
                //   <flag> <number>
                size_t valStart = opts.find_first_not_of(' ', ePos + 1);
                size_t valEnd = opts.find_first_of(' ', valStart);
                llvm::StringRef valStr = opts.substr(valStart, valEnd - valStart);
                if (valStr.getAsInteger(10, numThreadsPerEU))
                {
                    IGC_ASSERT(0);
                }
                Pos = valEnd;
                continue;
            }
            else if (suffix.equals("-large-grf-kernel"))
            {
                size_t valStart = opts.find_first_not_of(' ', ePos + 1);
                size_t valEnd = opts.find_first_of(' ', valStart);
                llvm::StringRef valStr = opts.substr(valStart, valEnd - valStart);

                LargeGRFKernels.push_back(valStr.str());
                Pos = valEnd;
                continue;
            }
            else if (suffix.equals("-regular-grf-kernel"))
            {
                size_t valStart = opts.find_first_not_of(' ', ePos + 1);
                size_t valEnd = opts.find_first_of(' ', valStart);
                llvm::StringRef valStr = opts.substr(valStart, valEnd - valStart);

                RegularGRFKernels.push_back(valStr.str());
                Pos = valEnd;
                continue;
            }

            // -cl-intel-force-disable-4GB-buffer
            else if (suffix.equals("-force-disable-4GB-buffer"))
            {
                IntelForceDisable4GBBuffer = true;
            }
            // -cl-intel-use-bindless-buffers
            else if (suffix.equals("-use-bindless-buffers"))
            {
                PromoteStatelessToBindless = true;
            }
            // -cl-intel-use-bindless-images
            else if (suffix.equals("-use-bindless-images"))
            {
                PreferBindlessImages = true;
            }
            // -cl-intel-use-bindless-mode
            else if (suffix.equals("-use-bindless-mode"))
            {
                // This is a new option that combines bindless generation for buffers
                // and images. Keep the old internal options to have compatibility
                // for existing tests. Those (old) options could be removed in future.
                UseBindlessMode = true;
                PreferBindlessImages = true;
                PromoteStatelessToBindless = true;
            }
            // -cl-intel-use-bindless-printf
            else if (suffix.equals("-use-bindless-printf"))
            {
                UseBindlessPrintf = true;
            }
            // -cl-intel-use-bindless-legacy-mode
            else if (suffix.equals("-use-bindless-legacy-mode"))
            {
                UseBindlessLegacyMode = true;
            }
            // -cl-intel-use-bindless-advanced-mode
            else if (suffix.equals("-use-bindless-advanced-mode"))
            {
                UseBindlessLegacyMode = false;
            }
            // -cl-intel-vector-coalesing
            else if (suffix.equals("-vector-coalescing"))
            {
                // -cl-intel-vector-coalescing=<0-5>.
                size_t valStart = opts.find_first_not_of(' ', ePos + 1);
                size_t valEnd = opts.find_first_of(' ', valStart);
                llvm::StringRef valStr = opts.substr(valStart, valEnd - valStart);

                int16_t val;
                if (valStr.getAsInteger(10, val))
                {
                    IGC_ASSERT_MESSAGE(false, "-cl-intel-vector-coalescing: invalid value, ignored!");
                }
                else if (val >= 0 && val <= 5)
                {
                    VectorCoalescingControl = val;
                }
                Pos = valEnd;
                continue;
            }
            // -cl-intel-allow-zebin
            else if (suffix.equals("-allow-zebin"))
            {
                EnableZEBinary = true;
            }
            // -cl-intel-exclude-ir-from-zebin
            else if (suffix.equals("-exclude-ir-from-zebin"))
            {
                ExcludeIRFromZEBinary = true;
            }
            // -cl-intel-no-spill
            else if (suffix.equals("-no-spill"))
            {
                // This is an option to avoid spill/fill instructions in scheduler kernel.
                // OpenCL Runtime triggers scheduler kernel offline compilation while driver building,
                // since scratch space is not supported in this specific case, we cannot end up with
                // spilling kernel. If this option is set, then IGC will recompile the kernel with
                // some some optimizations disabled to avoid spill/fill instructions.
                NoSpill = true;
            }
            // -cl-intel-disable-noMaskWA, -ze-intel-disable-noMaskWA
            else if (suffix.equals("-disable-noMaskWA"))
            {
                DisableNoMaskWA = true;
            }
            // -cl-intel-ignoreBFRounding, -ze-intel-ignoreBFRounding
            else if (suffix.equals("-ignoreBFRounding"))
            {
                IgnoreBFRounding = true;
            }
            // -cl-compile-one-at-time
            else if (suffix.equals("-compile-one-at-time"))
            {
                CompileOneKernelAtTime = true;
            }
            // -cl-skip-reloc-add
            else if (suffix.equals("-skip-reloc-add"))
            {
                AllowRelocAdd = false;
            }
            // -cl-intel-disableEUFusion
            // -ze-intel-disableEUFusion
            else if (suffix.equals("-disableEUFusion"))
            {
                DisableEUFusion = true;
            }
            // -cl-intel-functonControl [<n>]
            // -ze-intel-functionControl [<n>]
            else if (suffix.equals("-functionControl"))
            {
                int val;
                size_t valStart = opts.find_first_not_of(' ', ePos + 1);
                size_t valEnd = opts.find_first_of(' ', valStart);
                llvm::StringRef valStr = opts.substr(valStart, valEnd - valStart);
                if (valStr.getAsInteger(10, val))
                {
                    IGC_ASSERT(0);
                }
                if (val >= 0)
                {
                    FunctionControl = val;
                }
                Pos = valEnd;
            }
            else if (suffix.equals("-fail-on-spill"))
            {
                FailOnSpill = true;
            }
            // -cl-poison-unsupported-fp64-kernels
            // -ze-poison-unsupported-fp64-kernels
            else if (suffix.equals("-poison-unsupported-fp64-kernels"))
            {
                // This option forces IGC to poison kernels using fp64
                // operations on platforms without HW support for fp64.
                EnableUnsupportedFP64Poisoning = true;
            }
            // *-private-memory-minimal-size-per-thread <SIZE>
            // SIZE >= 0
            else if (suffix.equals("-private-memory-minimal-size-per-thread"))
            {
                size_t valueStart = opts.find_first_not_of(' ', ePos + 1);
                size_t valueEnd = opts.find_first_of(' ', valueStart);
                llvm::StringRef valueString = opts.substr(valueStart, valueEnd - valueStart);

                IntelPrivateMemoryMinimalSizePerThread = 0;
                if (valueString.getAsInteger(10, IntelPrivateMemoryMinimalSizePerThread))
                {
                    IGC_ASSERT(0);
                }
                Pos = valueEnd;
                continue;
            }
            // *-scratch-space-private-memory-minimal-size-per-thread <SIZE>
            // SIZE >= 0
            else if (suffix.equals("-scratch-space-private-memory-minimal-size-per-thread"))
            {
                size_t valueStart = opts.find_first_not_of(' ', ePos + 1);
                size_t valueEnd = opts.find_first_of(' ', valueStart);
                llvm::StringRef valueString = opts.substr(valueStart, valueEnd - valueStart);

                IntelScratchSpacePrivateMemoryMinimalSizePerThread = 0;
                if (valueString.getAsInteger(10, IntelScratchSpacePrivateMemoryMinimalSizePerThread))
                {
                    IGC_ASSERT(0);
                }
                Pos = valueEnd;
                continue;
            }

            // advance to the next flag
            Pos = opts.find_first_of(' ', Pos);
        }
        if (IntelForceDisable4GBBuffer)
        {
            IntelGreaterThan4GBBufferRequired = false;
        }
    }

    void CodeGenContext::initLLVMContextWrapper(bool createResourceDimTypes)
    {
        llvmCtxWrapper = new LLVMContextWrapper(createResourceDimTypes);
        llvmCtxWrapper->AddRef();
    }

    llvm::LLVMContext* CodeGenContext::getLLVMContext() const {
        return llvmCtxWrapper;
    }

    IGC::IGCMD::MetaDataUtils* CodeGenContext::getMetaDataUtils() const
    {
        IGC_ASSERT_MESSAGE(nullptr != m_pMdUtils, "Metadata Utils is not initialized");
        return m_pMdUtils;
    }

    IGCLLVM::Module* CodeGenContext::getModule() const { return module; }

    static void initCompOptionFromRegkey(CodeGenContext* ctx)
    {
        SetCurrentDebugHash(ctx->hash);

        CompOptions& opt = ctx->getModuleMetaData()->compOpt;

        opt.pixelShaderDoNotAbortOnSpill =
            IGC_IS_FLAG_ENABLED(PixelShaderDoNotAbortOnSpill);
        opt.forcePixelShaderSIMDMode =
            IGC_GET_FLAG_VALUE(ForcePixelShaderSIMDMode);
    }

    void CodeGenContext::setModule(llvm::Module* m)
    {
        module = (IGCLLVM::Module*)m;
        m_pMdUtils = new IGC::IGCMD::MetaDataUtils(m);
        modMD = new IGC::ModuleMetaData();
        initCompOptionFromRegkey(this);
    }

    // Several clients explicitly delete module without resetting module to null.
    // This causes the issue later when the dtor is invoked (trying to delete a
    // dangling pointer again). This function is used to replace any explicit
    // delete in order to prevent deleting dangling pointers happening.
    void CodeGenContext::deleteModule()
    {
        delete m_pMdUtils;
        delete modMD;
        delete module;
        m_pMdUtils = nullptr;
        modMD = nullptr;
        module = nullptr;
        delete annotater;
        annotater = nullptr;
    }

    IGC::ModuleMetaData* CodeGenContext::getModuleMetaData() const
    {
        IGC_ASSERT_MESSAGE(nullptr != modMD, "Module Metadata is not initialized");
        return modMD;
    }

    unsigned int CodeGenContext::getRegisterPointerSizeInBits(unsigned int AS) const
    {
        unsigned int pointerSizeInRegister = 32;
        switch (AS)
        {
        case ADDRESS_SPACE_GLOBAL:
        case ADDRESS_SPACE_CONSTANT:
        case ADDRESS_SPACE_GENERIC:
        case ADDRESS_SPACE_GLOBAL_OR_PRIVATE:
            pointerSizeInRegister =
                getModule()->getDataLayout().getPointerSizeInBits(AS);
            break;
        case ADDRESS_SPACE_LOCAL:
        case ADDRESS_SPACE_THREAD_ARG:
            pointerSizeInRegister = 32;
            break;
        case ADDRESS_SPACE_PRIVATE:
            if (getModuleMetaData()->compOpt.UseScratchSpacePrivateMemory)
            {
                pointerSizeInRegister = 32;
            }
            else
            {
                pointerSizeInRegister = ((type == ShaderType::OPENCL_SHADER) ?
                    getModule()->getDataLayout().getPointerSizeInBits(AS) : 64);
            }
            break;
        default:
            pointerSizeInRegister = 32;
            break;
        }
        return pointerSizeInRegister;
    }

    bool CodeGenContext::enableFunctionCall() const
    {
        return (m_enableSubroutine || m_enableFunctionPointer);
    }

    /// Check for user functions in the module and enable the m_enableSubroutine flag if exists
    void CodeGenContext::CheckEnableSubroutine(llvm::Module& M)
    {
        bool EnableSubroutine = false;
        bool EnableStackFuncs = false;
        for (auto& F : M)
        {
            if (F.isDeclaration() ||
                F.use_empty() ||
                isEntryFunc(getMetaDataUtils(), &F))
            {
                continue;
            }

            if (F.hasFnAttribute("KMPLOCK") ||
                F.hasFnAttribute(llvm::Attribute::NoInline) ||
                !F.hasFnAttribute(llvm::Attribute::AlwaysInline))
            {
                EnableSubroutine = true;
                if (F.hasFnAttribute("visaStackCall") && !F.user_empty())
                {
                    EnableStackFuncs = true;
                }
            }
        }
        m_enableSubroutine = EnableSubroutine;
        m_hasStackCalls = EnableStackFuncs;
    }

    void CodeGenContext::InitVarMetaData() {}

    CodeGenContext::~CodeGenContext()
    {
        clear();
    }


    void CodeGenContext::clear()
    {
        m_enableSubroutine = false;
        m_enableFunctionPointer = false;

        delete modMD;
        delete m_pMdUtils;
        modMD = nullptr;
        m_pMdUtils = nullptr;

        delete module;
        llvmCtxWrapper->Release();
        module = nullptr;
        llvmCtxWrapper = nullptr;
    }

    void CodeGenContext::clearMD()
    {
        delete modMD;
        delete m_pMdUtils;
        modMD = nullptr;
        m_pMdUtils = nullptr;
    }

    static const llvm::Function *getRelatedFunction(const llvm::Value *value)
    {
        if (value == nullptr)
            return nullptr;

        if (const llvm::Function *F = llvm::dyn_cast<llvm::Function>(value)) {
            return F;
        }
        if (const llvm::Argument *A = llvm::dyn_cast<llvm::Argument>(value)) {
            return A->getParent();
        }
        if (const llvm::BasicBlock *BB = llvm::dyn_cast<llvm::BasicBlock>(value)) {
            return BB->getParent();
        }
        if (const llvm::Instruction *I = llvm::dyn_cast<llvm::Instruction>(value)) {
            return I->getParent()->getParent();
        }

        return nullptr;
    }

    static bool isEntryPoint(const CodeGenContext *ctx, const llvm::Function *F)
    {
        if (F == nullptr) {
            return false;
        }

        auto& FuncMD = ctx->getModuleMetaData()->FuncMD;
        auto FuncInfo = FuncMD.find(const_cast<llvm::Function *>(F));
        if (FuncInfo == FuncMD.end()) {
            return false;
        }

        const FunctionMetaData* MD = &FuncInfo->second;
        return MD->functionType == KernelFunction;
    }

    static void findCallingKernels
        (const CodeGenContext *ctx, const llvm::Function *F, llvm::SmallPtrSetImpl<const llvm::Function *> &kernels)
    {
        if (F == nullptr || kernels.count(F))
            return;

        for (const llvm::User *U : F->users()) {
            auto *CI = llvm::dyn_cast<llvm::CallInst>(U);
            if (CI == nullptr)
                continue;

            if (CI->getCalledFunction() != F)
                continue;

            const llvm::Function *caller = getRelatedFunction(CI);
            if (isEntryPoint(ctx, caller)) {
                kernels.insert(caller);
                continue;
            }
            // Caller is not a kernel, try to check which kerneles might
            // be calling it:
            findCallingKernels(ctx, caller, kernels);
        }
    }

    static bool handleOpenMPDemangling(const std::string &name, std::string *strippedName) {
        // OpenMP mangled names have following structure:
        //
        // __omp_offloading_DD_FFFF_PP_lBB
        //
        // where DD_FFFF is an ID unique to the file (device and file IDs), PP is the
        // mangled name of the function that encloses the target region and BB is the
        // line number of the target region.
        if (name.rfind("__omp_offloading_", 0) != 0) {
            return false;
        }
        size_t offset = sizeof "__omp_offloading_";
        offset = name.find('_', offset + 1); // Find end of DD.
        if (offset == std::string::npos)
            return false;
        offset = name.find('_', offset + 1); // Find end of FFFF.
        if (offset == std::string::npos)
            return false;

        const size_t start = offset + 1;
        const size_t end = name.rfind('_'); // Find beginning of lBB.
        if (end == std::string::npos)
            return false;

        *strippedName = name.substr(start, end - start);
        return true;
    }


    static std::string demangleFuncName(const std::string &rawName) {
        // OpenMP adds additional prefix and suffix to the mangling scheme,
        // remove it if present.
        std::string name;
        if (!handleOpenMPDemangling(rawName, &name)) {
            // If OpenMP demangling didn't succeed just proceed with received
            // symbol name
            name = rawName;
        }
#if LLVM_VERSION_MAJOR >= 10
        return llvm::demangle(name);
#else
        char *demangled = nullptr;

        demangled = llvm::itaniumDemangle(name.c_str(), nullptr, nullptr, nullptr);
        if (demangled == nullptr) {
            demangled = llvm::microsoftDemangle(name.c_str(), nullptr, nullptr, nullptr);
        }

        if (demangled == nullptr) {
            return name;
        }

        std::string result = demangled;
        std::free(demangled);
        return result;
#endif
    }

    void CodeGenContext::EmitError(std::ostream &OS, const char* errorstr, const llvm::Value* context) const
    {
        OS << "\nerror: ";
        OS << errorstr;
        // Try to get debug location to print out the relevant info.
        if (const llvm::Instruction *I = llvm::dyn_cast_or_null<llvm::Instruction>(context)) {
            if (const llvm::DILocation *DL = I->getDebugLoc()) {
                OS << "\nin file: " << DL->getFilename().str() << ":" << DL->getLine() << "\n";
            }
        }
        // Try to find function related to given context
        // to print more informative error message.
        if (const llvm::Function *F = getRelatedFunction(context)) {
            // If the function is a kernel just print the kernel name.
            if (isEntryPoint(this, F)) {
                OS << "\nin kernel: '" << demangleFuncName(std::string(F->getName())) << "'";
            // If the function is not a kernel try to print all kernels that
            // might be using this function.
            } else {
                llvm::SmallPtrSet<const llvm::Function *, 16> kernels;
                findCallingKernels(this, F, kernels);

                const size_t kernelsCount = kernels.size();
                OS << "\nin function: '" << demangleFuncName(std::string(F->getName())) << "' ";
                if (kernelsCount == 0) {
                    OS << "called indirectly by at least one of the kernels.\n";
                } else if (kernelsCount == 1) {
                    const llvm::Function *kernel = *kernels.begin();
                    OS << "called by kernel: '" << demangleFuncName(std::string(kernel->getName())) << "'\n";
                } else {
                    OS << "called by kernels:\n";
                    for (const llvm::Function *kernel : kernels) {
                        OS << "  - '" << demangleFuncName(std::string(kernel->getName())) << "'\n";
                    }
                }
            }
        }
        OS << "\nerror: backend compiler failed build.\n";
    }

    void CodeGenContext::EmitError(const char* errorstr, const llvm::Value *context)
    {
        EmitError(this->oclErrorMessage, errorstr, context);
    }

    void CodeGenContext::EmitWarning(const char* warningstr)
    {
        this->oclWarningMessage << "\nwarning: ";
        this->oclWarningMessage << warningstr;
        this->oclWarningMessage << "\n";
    }

    CompOptions& CodeGenContext::getCompilerOption()
    {
        return getModuleMetaData()->compOpt;
    }

    void CodeGenContext::resetOnRetry()
    {
        m_tempCount = 0;
    }

    uint32_t CodeGenContext::getNumThreadsPerEU() const
    {
        return 0;
    }

    uint32_t CodeGenContext::getNumGRFPerThread() const
    {
        constexpr uint32_t DEFAULT_TOTAL_GRF_NUM = 128;

        if (IGC_GET_FLAG_VALUE(TotalGRFNum) != 0)
        {
            return IGC_GET_FLAG_VALUE(TotalGRFNum);
        }
        if (getModuleMetaData()->csInfo.forceTotalGRFNum != 0)
        {
            {
                return getModuleMetaData()->csInfo.forceTotalGRFNum;
            }
        }
        if (hasSyncRTCalls() && IGC_GET_FLAG_VALUE(TotalGRFNum4RQ) != 0)
        {
            return IGC_GET_FLAG_VALUE(TotalGRFNum4RQ);
        }
        if (this->type == ShaderType::COMPUTE_SHADER && IGC_GET_FLAG_VALUE(TotalGRFNum4CS) != 0)
        {
            return IGC_GET_FLAG_VALUE(TotalGRFNum4CS);
        }
        return DEFAULT_TOTAL_GRF_NUM;
    }

    bool CodeGenContext::forceGlobalMemoryAllocation() const
    {
        return false;
    }

    bool CodeGenContext::allocatePrivateAsGlobalBuffer() const
    {
        return false;
    }

    bool CodeGenContext::enableTakeGlobalAddress() const
    {
        return false;
    }

    int16_t CodeGenContext::getVectorCoalescingControl() const
    {
        return 0;
    }

    uint32_t CodeGenContext::getPrivateMemoryMinimalSizePerThread() const
    {
        return 0;
    }

    uint32_t CodeGenContext::getIntelScratchSpacePrivateMemoryMinimalSizePerThread() const
    {
        return 0;
    }

    bool CodeGenContext::isPOSH() const
    {
        return this->getModule()->getModuleFlag(
            "IGC::PositionOnlyVertexShader") != nullptr;
    }

    void CodeGenContext::setFlagsPerCtx()
    {
        if (m_DriverInfo.DessaAliasLevel() != -1) {
            if ((int)IGC_GET_FLAG_VALUE(EnableDeSSAAlias) > m_DriverInfo.DessaAliasLevel())
            {
                IGC_SET_FLAG_VALUE(EnableDeSSAAlias, m_DriverInfo.DessaAliasLevel());
            }
        }
    }

    unsigned MeshShaderContext::GetThreadGroupSize()
    {
        unsigned int threadGroupSize = ((type == ShaderType::MESH_SHADER) ?
            getModuleMetaData()->msInfo.WorkGroupSize : getModuleMetaData()->taskInfo.WorkGroupSize);

        return threadGroupSize;
    }

    unsigned MeshShaderContext::GetHwThreadPerWorkgroup()
    {
        unsigned hwThreadPerWorkgroup = platform.getMaxNumberHWThreadForEachWG();

        if (platform.supportPooledEU())
        {
            hwThreadPerWorkgroup = platform.getMaxNumberThreadPerWorkgroupPooledMax();
        }
        return hwThreadPerWorkgroup;
    }

    unsigned int MeshShaderContext::GetSlmSize() const
    {
        unsigned int slmSize = ((type == ShaderType::MESH_SHADER) ?
            getModuleMetaData()->msInfo.WorkGroupMemorySizeInBytes :
            getModuleMetaData()->taskInfo.WorkGroupMemorySizeInBytes);
        return slmSize;
    }

    // Use compute shader thresholds.
    float MeshShaderContext::GetSpillThreshold() const
    {
        float spillThresholdSLM =
            float(IGC_GET_FLAG_VALUE(CSSpillThresholdSLM)) / 100.0f;
        float spillThresholdNoSLM =
            float(IGC_GET_FLAG_VALUE(CSSpillThresholdNoSLM)) / 100.0f;
        return GetSlmSize() > 0 ? spillThresholdSLM : spillThresholdNoSLM;
    }

    // Calculates thread occupancy for given simd size.
    float MeshShaderContext::GetThreadOccupancy(SIMDMode simdMode)
    {
        return GetThreadOccupancyPerSubslice(
            simdMode,
            GetThreadGroupSize(),
            GetHwThreadsPerWG(platform),
            GetSlmSize(),
            platform.getSlmSizePerSsOrDss());
    }

    SIMDMode MeshShaderContext::GetLeastSIMDModeAllowed()
    {
        unsigned threadGroupSize = GetThreadGroupSize();
        unsigned hwThreadPerWorkgroup = GetHwThreadPerWorkgroup();

        if ((threadGroupSize <= hwThreadPerWorkgroup * 8) &&
            threadGroupSize <= 512)
        {
            return platform.getMinDispatchMode();
        }
        else
        {
            if (threadGroupSize <= hwThreadPerWorkgroup * 16)
            {
                return SIMDMode::SIMD16;
            }
            else
            {
                return SIMDMode::SIMD32;
            }
        }
    }

    SIMDMode MeshShaderContext::GetMaxSIMDMode()
    {
        unsigned threadGroupSize = GetThreadGroupSize();

        if (threadGroupSize <= 8)
        {
            return platform.getMinDispatchMode();
        }
        else if (threadGroupSize <= 16)
        {
            return SIMDMode::SIMD16;
        }
        else
        {
            return SIMDMode::SIMD32;
        }
    }

    SIMDMode MeshShaderContext::GetBestSIMDMode()
    {
        const SIMDMode minMode = GetLeastSIMDModeAllowed();
        const SIMDMode maxMode = GetMaxSIMDMode();

        SIMDMode bestMode = SIMDMode::SIMD16;

        if (bestMode < minMode)
        {
            bestMode = minMode;
        }

        if (bestMode > maxMode)
        {
            bestMode = maxMode;
        }

        return bestMode;
    }

    void RayDispatchShaderContext::setShaderHash(llvm::Function* F) const
    {
        auto* MD = getModuleMetaData();
        auto I = MD->FuncMD.find(F);
        if (I == MD->FuncMD.end())
            return;

        auto& rtInfo = I->second.rtInfo;
        StringRef Name = F->getName();

        uint64_t Hashes[] = {
            BitcodeHash,
            iSTD::HashFromBuffer(Name.data(), Name.size())
        };

        rtInfo.ShaderHash = iSTD::Hash((DWORD*)Hashes, std::size(Hashes) * 2);
    }

    Optional<RTStackFormat::HIT_GROUP_TYPE> RayDispatchShaderContext::getHitGroupType(
        const std::string &Name) const
    {
        auto I = HitGroupRefs.find(Name);
        if (I == HitGroupRefs.end())
            return None;

        auto &Refs = I->second;
        IGC_ASSERT(!Refs.empty());

        auto HitTy = Refs[0]->Type;

        IGC_ASSERT(llvm::all_of(Refs, [&](auto &HG) { return HG->Type == HitTy; }));

        return HitTy;
    }

    llvm::Optional<SIMDMode> RayDispatchShaderContext::knownSIMDSize() const
    {
        uint32_t ForcedSIMDSize = IGC_GET_FLAG_VALUE(ForceRayTracingSIMDWidth);

        switch (ForcedSIMDSize)
        {
        case 0: // default
            return platform.getPreferredRayTracingSIMDSize();
        case 8:
            return SIMDMode::SIMD8;
        case 16:
            return SIMDMode::SIMD16;
        default:
            IGC_ASSERT_MESSAGE(0, "not an option!");
            return llvm::None;
        }
    }

    bool RayDispatchShaderContext::canEfficientTile() const
    {
        auto canDo = [](uint32_t XDim) {
            return iSTD::IsPowerOfTwo(XDim) && XDim > 1;
        };

        uint32_t XDim1D = opts().TileXDim1D;
        uint32_t XDim2D = opts().TileXDim2D;

        return canDo(XDim1D) && canDo(XDim2D);
    }

    Optional<std::string> RayDispatchShaderContext::getIntersectionAnyHit(
        const std::string& IntersectionName) const
    {
        auto I = HitGroupRefs.find(IntersectionName);
        IGC_ASSERT(I != HitGroupRefs.end());

        auto& Refs = I->second;
        IGC_ASSERT(!Refs.empty());

        auto AnyHit = Refs[0]->AnyHit;

        IGC_ASSERT(llvm::all_of(Refs, [&](auto &HG) { return HG->AnyHit == AnyHit; }));

        return AnyHit;
    }

    const std::vector<HitGroupInfo*>*
    RayDispatchShaderContext::hitgroupRefs(const std::string& Name) const
    {
        auto I = HitGroupRefs.find(Name);
        if (I == HitGroupRefs.end())
            return nullptr;

        auto& Refs = I->second;
        return &Refs;
    }

    const std::vector<HitGroupInfo>&
    RayDispatchShaderContext::hitgroups() const
    {
        return HitGroups;
    }

    void RayDispatchShaderContext::takeHitGroupTable(std::vector<HitGroupInfo>&& Table)
    {
        HitGroupRefs.clear();
        HitGroups = std::move(Table);

        for (auto& HG : HitGroups)
        {
            if (HG.AnyHit)
                HitGroupRefs[*HG.AnyHit].push_back(&HG);
            if (HG.Intersection)
                HitGroupRefs[*HG.Intersection].push_back(&HG);
            if (HG.ClosestHit)
                HitGroupRefs[*HG.ClosestHit].push_back(&HG);
        }
    }

    bool RayDispatchShaderContext::requiresIndirectContinuationHandling() const
    {
        IGC_ASSERT_MESSAGE(modMD->rtInfo.NumContinuations != UINT_MAX,
            "not computed yet!");

        if (IGC_IS_FLAG_ENABLED(EnableInlinedContinuations) &&
            canWholeProgramCompile())
        {
            return false;
        }

        uint32_t Threshold = IGC_GET_FLAG_VALUE(ContinuationInlineThreshold);
        return forcedIndirectContinuations() ||
               !canWholeProgramCompile()     ||
               modMD->rtInfo.NumContinuations > Threshold;
    }

    bool RayDispatchShaderContext::forcedIndirectContinuations() const
    {
        return IGC_IS_FLAG_ENABLED(EnableIndirectContinuations) || forceIndirectContinuations;
    }

    bool RayDispatchShaderContext::canWholeProgramCompile() const
    {
        if (IGC_GET_FLAG_VALUE(ForceWholeProgramCompile))
            return true;

        return canWholeShaderCompile() && canWholeFunctionCompile();
    }

    bool RayDispatchShaderContext::canWholeShaderCompile() const
    {
        if (forceIndirectContinuations)
            return false;

        return config == CompileConfig::IMMUTABLE_RTPSO && !hasPrecompiledObjects;
    }

    bool RayDispatchShaderContext::canWholeFunctionCompile() const
    {
        // This will have more logic once dynamic shader linking is in place.
        return true;
    }

    bool RayDispatchShaderContext::tryPayloadSinking() const
    {
        return pipelineConfig.maxTraceRecursionDepth == 1 &&
               IGC_IS_FLAG_DISABLED(DisablePayloadSinking);
    }

    bool RayDispatchShaderContext::isRTPSO() const
    {
        switch (config)
        {
        case CompileConfig::MUTABLE_RTPSO:
        case CompileConfig::IMMUTABLE_RTPSO:
            return true;
        case CompileConfig::CSO:
            return false;
        }

        return false;
    }

    bool RayDispatchShaderContext::isDispatchAlongY() const
    {
        const bool AlongY =
            m_DriverInfo.supportsRaytracingDispatchComputeWalkerAlongYFirst();
        return opts().DispatchAlongY && AlongY;
    }

    bool RayDispatchShaderContext::doSpillWidening() const
    {
        if (IGC_IS_FLAG_DISABLED(EnableSpillWidening))
            return false;

        // It's easier to just only do this with stateful SWStack so it's easier
        // to find the spills later on.
        auto Offset = getModuleMetaData()->rtInfo.SWStackSurfaceStateOffset;
        return Offset.has_value();
    }

    uint64_t RayDispatchShaderContext::getShaderHash(const CShader* Prog) const
    {
        auto* MD = getModuleMetaData();
        auto I = MD->FuncMD.find(Prog->entry);
        if (I != MD->FuncMD.end())
            return I->second.rtInfo.ShaderHash;
        else
            return 0;
    }

}
