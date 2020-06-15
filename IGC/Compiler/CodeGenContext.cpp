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
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Probe/Assertion.h"

namespace IGC
{

    typedef struct RetryState {
        bool allowLICM;
        bool allowCodeSinking;
        bool allowSimd32Slicing;
        bool allowPromotePrivateMemory;
        bool allowPreRAScheduler;
        bool allowLargeURBWrite;
        unsigned nextState;
    } RetryState;

    static const RetryState RetryTable[] = {
        { true, true, false, true, true, true, 1 },
        { false, true, true, false, false, false, 500 }
    };

    RetryManager::RetryManager() : enabled(false)
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
    bool RetryManager::AllowPromotePrivateMemory() {
        IGC_ASSERT(stateId < getStateCnt());
        return RetryTable[stateId].allowPromotePrivateMemory;
    }
    bool RetryManager::AllowPreRAScheduler() {
        IGC_ASSERT(stateId < getStateCnt());
        return RetryTable[stateId].allowPreRAScheduler;
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
    void RetryManager::Disable() { enabled = false; }

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

    CShader* RetryManager::PickCSEntryByRegKey(SIMDMode& simdMode)
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
                if (IGC_IS_FLAG_ENABLED(ForceCSLeastSIMD))
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
                PickCSEntryByRegKey(simdMode));
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
                break;

            case SIMDMode::SIMD16:
                pKernelProgram->simd16 = *shader->ProgramOutput();
                pKernelProgram->SimdWidth = USC::GFXMEDIA_GPUWALKER_SIMD16;
                break;

            case SIMDMode::SIMD32:
                pKernelProgram->simd32 = *shader->ProgramOutput();
                pKernelProgram->SimdWidth = USC::GFXMEDIA_GPUWALKER_SIMD32;
                break;

            default:
                IGC_ASSERT_MESSAGE(0, "Invalie SIMDMode");
                break;
            }
            shader->FillProgram(pKernelProgram);

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
        unsigned threadGroupSize_X = int_cast<unsigned>(llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());

        pGlobal = getModule()->getGlobalVariable("ThreadGroupSize_Y");
        unsigned threadGroupSize_Y = int_cast<unsigned>(llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());

        pGlobal = getModule()->getGlobalVariable("ThreadGroupSize_Z");
        unsigned threadGroupSize_Z = int_cast<unsigned>(llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());

        return threadGroupSize_X * threadGroupSize_Y * threadGroupSize_Z;
    }

    unsigned ComputeShaderContext::GetSlmSizePerSubslice()
    {
        return 65536; // TODO: should get this from GTSysInfo instead of hardcoded value
    }

    float ComputeShaderContext::GetThreadOccupancy(SIMDMode simdMode)
    {
        return GetThreadOccupancyPerSubslice(simdMode, GetThreadGroupSize(), GetHwThreadsPerWG(platform), m_slmSize, GetSlmSizePerSubslice());
    }

    /** get smallest SIMD mode allowed based on thread group size */
    SIMDMode ComputeShaderContext::GetLeastSIMDModeAllowed()
    {
        return getLeastSIMDAllowed(GetThreadGroupSize(), GetHwThreadsPerWG(platform));
    }

    /** get largest SIMD mode for performance based on thread group size */
    SIMDMode ComputeShaderContext::GetMaxSIMDMode()
    {
        unsigned threadGroupSize = GetThreadGroupSize();

        if (threadGroupSize <= 8)
        {
            return SIMDMode::SIMD8;
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

    float ComputeShaderContext::GetSpillThreshold() const
    {
        float spillThresholdSLM =
            float(IGC_GET_FLAG_VALUE(CSSpillThresholdSLM)) / 100.0f;
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
        return 0;
    }

    uint32_t OpenCLProgramContext::getNumGRFPerThread() const
    {
        return CodeGenContext::getNumGRFPerThread();
    }

    bool OpenCLProgramContext::forceGlobalMemoryAllocation() const
    {
        return m_InternalOptions.IntelForceGlobalMemoryAllocation;
    }

    bool OpenCLProgramContext::hasNoLocalToGenericCast() const
    {
        return m_InternalOptions.hasNoLocalToGeneric;
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

    void CodeGenContext::initLLVMContextWrapper(bool createResourceDimTypes)
    {
        llvmCtxWrapper = new LLVMContextWrapper(createResourceDimTypes);
        llvmCtxWrapper->AddRef();
    }

    llvm::LLVMContext* CodeGenContext::getLLVMContext() {
        return llvmCtxWrapper;
    }

    IGC::IGCMD::MetaDataUtils* CodeGenContext::getMetaDataUtils()
    {
        IGC_ASSERT_MESSAGE(nullptr != m_pMdUtils, "Metadata Utils is not initialized");
        return m_pMdUtils;
    }

    IGCLLVM::Module* CodeGenContext::getModule() const { return module; }

    static void initCompOptionFromRegkey(CodeGenContext* ctx)
    {
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
        if (m_enableSubroutine || m_enableFunctionPointer)
            return true;

        int FCtrol = IGC_GET_FLAG_VALUE(FunctionControl);
        return FCtrol == FLAG_FCALL_FORCE_SUBROUTINE ||
            FCtrol == FLAG_FCALL_FORCE_STACKCALL;
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

    void CodeGenContext::EmitError(const char* errorstr)
    {
        std::string str(errorstr);
        std::string  msg;
        msg += "\nerror: ";
        msg += str;
        msg += "\nerror: backend compiler failed build.\n";
        str = msg;
        this->oclErrorMessage = str;// where to get this from
        return;
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
        if (IGC_GET_FLAG_VALUE(TotalGRFNum) != 0)
        {
            return IGC_GET_FLAG_VALUE(TotalGRFNum);
        }
        return 128;
    }

    bool CodeGenContext::forceGlobalMemoryAllocation() const
    {
        return false;
    }

    bool CodeGenContext::hasNoLocalToGenericCast() const
    {
        return false;
    }

    int16_t CodeGenContext::getVectorCoalescingControl() const
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


}
