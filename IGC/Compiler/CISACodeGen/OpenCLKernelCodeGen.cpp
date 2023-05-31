/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/ScaledNumber.h>
#include "llvm/IR/DataLayout.h"
#include "llvm/ADT/StringExtras.h"
#include "common/LLVMWarningsPop.hpp"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"
#include "Compiler/CISACodeGen/messageEncoding.hpp"
#include "Compiler/CISACodeGen/DebugInfo.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ResourceAllocator/ResourceAllocator.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ProgramScopeConstants/ProgramScopeConstantAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/LocalBuffers/InlineLocalsResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs.hpp"
#include "Compiler/CISACodeGen/EmitVISAPass.hpp"
#include "Compiler/Optimizer/OCLBIUtils.h"
#include "AdaptorOCL/OCL/KernelAnnotations.hpp"
#include "common/allocator.h"
#include "common/igc_regkeys.hpp"
#include "common/Stats.hpp"
#include "common/SystemThread.h"
#include "common/secure_mem.h"
#include "common/MDFrameWork.h"
#include <iStdLib/utility.h>
#include "Probe/Assertion.h"
#include "ZEBinWriter/zebin/source/ZEELFObjectBuilder.hpp"

/***********************************************************************************
This file contains the code specific to opencl kernels
************************************************************************************/

namespace IGC
{
    using namespace llvm;
    using namespace IGC;
    using namespace IGC::IGCMD;

    bool OpenCLProgramContext::isSPIRV() const
    {
        return isSpirV;
    }

    void OpenCLProgramContext::setAsSPIRV()
    {
        isSpirV = true;
    }

    bool OpenCLProgramContext::needsDivergentBarrierHandling() const {
        return IGC_IS_FLAG_ENABLED(EnableDivergentBarrierWA) ||
            m_InternalOptions.EnableDivergentBarrierHandling;
    }

    float OpenCLProgramContext::getProfilingTimerResolution()
    {
        return m_ProfilingTimerResolution;
    }

    int32_t OpenCLProgramContext::getNumThreadsPerEU() const
    {
        if (m_Options.IntelRequiredEUThreadCount)
        {
            return m_Options.requiredEUThreadCount;
        }
        if (m_InternalOptions.IntelNumThreadPerEU)
        {
            return m_InternalOptions.numThreadsPerEU;
        }

        return -1;
    }

    uint32_t OpenCLProgramContext::getExpGRFSize() const {
        if (m_InternalOptions.IntelExpGRFSize) {
            return m_InternalOptions.expGRFSize;
        }

        return 0;
    }

    uint32_t OpenCLProgramContext::getNumGRFPerThread(bool returnDefault)
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
        return CodeGenContext::getNumGRFPerThread(returnDefault);
    }

    bool OpenCLProgramContext::forceGlobalMemoryAllocation() const
    {
        return m_InternalOptions.ForceGlobalMemoryAllocation ||
            (m_hasGlobalInPrivateAddressSpace && enableZEBinary());
    }

    bool OpenCLProgramContext::allocatePrivateAsGlobalBuffer() const
    {
        return forceGlobalMemoryAllocation() ||
            (m_instrTypes.hasDynamicGenericLoadStore &&
                platform.canForcePrivateToGlobal());
    }

    bool OpenCLProgramContext::noLocalToGenericOptionEnabled() const
    {
        return m_InternalOptions.NoLocalToGeneric;
    }

    bool OpenCLProgramContext::mustDistinguishBetweenPrivateAndGlobalPtr() const
    {
        return m_mustDistinguishBetweenPrivateAndGlobalPtr;
    }

    void OpenCLProgramContext::setDistinguishBetweenPrivateAndGlobalPtr(bool distinguish)
    {
        m_mustDistinguishBetweenPrivateAndGlobalPtr = distinguish;
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

    float OpenCLProgramContext::GetSpillThreshold(SIMDMode dispatchSize)
    {
        float threshold = 0.0f;
        if (this->platform.getGRFSize() >= 64)
        {
            if (dispatchSize == SIMDMode::SIMD32)
                threshold = float(IGC_GET_FLAG_VALUE(SIMD32_SpillThreshold)) / 100.0f;
            else if (dispatchSize == SIMDMode::SIMD16)
                threshold = float(IGC_GET_FLAG_VALUE(SIMD16_SpillThreshold) * 2) / 100.0f;
        }
        else
        {
            if (dispatchSize == SIMDMode::SIMD16)
                threshold = float(IGC_GET_FLAG_VALUE(SIMD16_SpillThreshold)) / 100.0f;
            else if (dispatchSize == SIMDMode::SIMD8)
                threshold = float(IGC_GET_FLAG_VALUE(SIMD8_SpillThreshold)) / 100.0f;
        }
        return threshold;
    }

    void OpenCLProgramContext::InternalOptions::parseOptions(const char* IntOptStr)
    {
        // Assume flags is in the form: <f0>[=<v0>] <f1>[=<v1>] ...
        // flag name and its value are either seperated by one or many ' ' or a single '='.
        // A flag seperator between two flags is always one or many ' '.
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
            // -cl-intel-no-local-to-generic
            else if (suffix.equals("-no-local-to-generic"))
            {
                NoLocalToGeneric = true;
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
            }
            else if (suffix.equals("-256-GRF-per-thread") ||
                suffix.equals("-large-register-file"))
            {
                Intel256GRFPerThread = true;
            }
            else if (suffix.equals("-num-thread-per-eu"))
            {
                IntelNumThreadPerEU = true;

                // Take an integer value after this option:
                //   <flag> <number>
                size_t valStart = opts.find_first_not_of(' ', ePos + 1);
                size_t valEnd = opts.find_first_of(' ', valStart);
                llvm::StringRef valStr = opts.substr(valStart, valEnd - valStart);
                if (valStr.equals("auto"))
                {
                    numThreadsPerEU = 0;
                }
                else if (valStr.getAsInteger(10, numThreadsPerEU))
                {
                    IGC_ASSERT(0);
                }
                Pos = valEnd;
                continue;
            } else if (suffix.equals("-exp-register-file-size")) {
                IntelExpGRFSize = true;

                // Take an integer value after this option:
                //   <flag> <number>
                size_t valStart = opts.find_first_not_of(' ', ePos + 1);
                size_t valEnd = opts.find_first_of(' ', valStart);
                llvm::StringRef valStr =
                    opts.substr(valStart, valEnd - valStart);
                if (valStr.getAsInteger(10, expGRFSize)) {
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
            else if (suffix.equals("-enable-auto-large-GRF-mode"))
            {
                IntelEnableAutoLargeGRF = true;
            }
            // -cl-disable-recompilation
            else if (suffix.equals("-disable-recompilation"))
            {
                IGC_SET_FLAG_VALUE(DisableRecompilation, true);
            }
            // -cl-intel-force-emu-int32divrem
            else if (suffix.equals("-force-emu-int32divrem"))
            {
                IntelForceInt32DivRemEmu = true;
            }
            // -cl-intel-force-emu-sp-int32divrem
            else if (suffix.equals("-force-emu-sp-int32divrem"))
            {
                IntelForceInt32DivRemEmuSP = true;
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

                int16_t val = 0;
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
            // -cl-allow-zebin
            else if (suffix.equals("-allow-zebin"))
            {
                EnableZEBinary = true;
            }
            // -cl-disable-zebin
            else if (suffix.equals("-disable-zebin"))
            {
                EnableZEBinary = false;
            }
            // -cl-intel-exclude-ir-from-zebin
            else if (suffix.equals("-exclude-ir-from-zebin"))
            {
                ExcludeIRFromZEBinary = true;
            }
            else if (suffix.equals("-emit-zebin-visa-sections"))
            {
                EmitZeBinVISASections = true;
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
                int val = 0;
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
            // -[cl|ze]-load-cache-default[=| ]<positive int>
            // -[cl|ze]-store-cache-default[=| ]<positive int>
            else if (suffix.equals("-load-cache-default") || suffix.equals("-store-cache-default"))
            {
                bool isLoad = suffix.equals("-load-cache-default");
                int val = 0;
                size_t valStart = opts.find_first_not_of(' ', ePos + 1);
                size_t valEnd = opts.find_first_of(' ', valStart);
                llvm::StringRef valStr = opts.substr(valStart, valEnd - valStart);
                if (valStr.getAsInteger(10, val))
                {
                    IGC_ASSERT(0);
                }
                if (val >= 0)
                {
                    if (isLoad)
                        LoadCacheDefault = val;
                    else
                        StoreCacheDefault = val;
                }
                Pos = valEnd;
            }
            // -cl-poison-unsupported-fp64-kernels
            // -ze-poison-unsupported-fp64-kernels
            else if (suffix.equals("-poison-unsupported-fp64-kernels"))
            {
                // This option forces IGC to poison kernels using fp64
                // operations on platforms without HW support for fp64.
                EnableUnsupportedFP64Poisoning = true;
            }
            // -cl-fp64-gen-emu
            // -ze-fp64-gen-emu
            else if (suffix.equals("-fp64-gen-emu"))
            {
                // This option enables FP64 emulation for platforms that
                // cannot HW support for double operations
                EnableFP64GenEmu = true;
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
            else if (suffix.equals("-enable-divergent-barrier-handling"))
            {
                EnableDivergentBarrierHandling = true;
            }
            // -cl-intel-high-accuracy-nolut-math
            else if (suffix.equals("-high-accuracy-nolut-math"))
            {
                UseHighAccuracyMathFuncs = true;
            }
            else if (suffix.equals("-emit-visa-only")) {
                EmitVisaOnly = true;
            }

            // advance to the next flag
            Pos = opts.find_first_of(' ', Pos);
        }
        if (IntelForceDisable4GBBuffer)
        {
            IntelGreaterThan4GBBufferRequired = false;
        }
    }

    unsigned OpenCLProgramContext::GetSlmSizePerSubslice()
    {
        return platform.getSlmSizePerSsOrDss();
    }

    COpenCLKernel::COpenCLKernel(OpenCLProgramContext* ctx, Function* pFunc, CShaderProgram* pProgram) :
        CComputeShaderBase(pFunc, pProgram)
    {
        m_HasTID = false;
        m_HasGlobalSize = false;
        m_disableMidThreadPreemption = false;
        m_perWIStatelessPrivateMemSize = 0;
        m_Context = ctx;
        m_localOffsetsMap.clear();
        m_pBtiLayout = &(ctx->btiLayout);
        m_Platform = &(ctx->platform);
        m_DriverInfo = &(ctx->m_DriverInfo);

        m_regularGRFRequested = false;
        m_largeGRFRequested = false;
        m_annotatedNumThreads = -1;
        if (m_Platform->supportsStaticRegSharing())
        {
            // Obtain number of threads from user annotations if it is set
            auto& FuncInfo = m_Context->getModuleMetaData()->FuncMD[pFunc];
            int numThreads = extractAnnotatedNumThreads(FuncInfo);
            if (numThreads >= 0 && m_Platform->isValidNumThreads(numThreads))
            {
                m_annotatedNumThreads = numThreads;
            }

            //check if option is set to use certain GRF size
            auto FuncName = pFunc->getName().str();
            for (auto SubNameR : ctx->m_InternalOptions.RegularGRFKernels)
            {
                if (FuncName.find(SubNameR) != std::string::npos)
                {
                    m_regularGRFRequested = true;
                    break;
                }
            }
            for (auto SubNameL : ctx->m_InternalOptions.LargeGRFKernels)
            {
                if (FuncName.find(SubNameL) != std::string::npos)
                {
                    m_largeGRFRequested = true;
                    break;
                }
            }
        }
    }

    COpenCLKernel::~COpenCLKernel()
    {
        ClearKernelInfo();
        m_simdProgram.Destroy();
    }

    void COpenCLKernel::ClearKernelInfo()
    {
        // Global pointer arguments
        m_kernelInfo.m_pointerArgument.clear();

        // Non-argument pointer inputs
        m_kernelInfo.m_pointerInput.clear();

        // Local pointer arguments
        m_kernelInfo.m_localPointerArgument.clear();

        // Sampler inputs
        m_kernelInfo.m_samplerInput.clear();

        // Sampler arguments
        m_kernelInfo.m_samplerArgument.clear();

        // Scalar inputs
        m_kernelInfo.m_constantInputAnnotation.clear();

        // Scalar arguments
        m_kernelInfo.m_constantArgumentAnnotation.clear();

        // Image arguments
        m_kernelInfo.m_imageInputAnnotations.clear();

        // Kernel Arg Reflection Info
        m_kernelInfo.m_kernelArgInfo.clear();

        // Printf strings
        m_kernelInfo.m_printfStringAnnotations.clear();

        // Argument to BTI/Sampler index map
        m_kernelInfo.m_argIndexMap.clear();
    }

    void COpenCLKernel::PreCompile()
    {
        ClearKernelInfo();
        CreateImplicitArgs();
        //We explicitly want this to be GRF-sized, without relation to simd width

        RecomputeBTLayout();

        ModuleMetaData* modMD = m_Context->getModuleMetaData();
        auto funcIter = modMD->FuncMD.find(entry);

        // Initialize the table of offsets for GlobalVariables representing locals
        if (funcIter != modMD->FuncMD.end())
        {
            auto loIter = funcIter->second.localOffsets.begin();
            auto loEnd = funcIter->second.localOffsets.end();
            for (; loIter != loEnd; ++loIter)
            {
                LocalOffsetMD loHandle = *loIter;
                m_localOffsetsMap[loHandle.m_Var] = loHandle.m_Offset;
            }
        }
        if (m_Platform->supportHWGenerateTID() && m_DriverInfo->SupportHWGenerateTID())
            tryHWGenerateLocalIDs();
    }
    void COpenCLKernel::tryHWGenerateLocalIDs()
    {
        auto Dims = IGCMetaDataHelper::getThreadGroupDims(
            *m_pMdUtils, entry);

        if (!Dims)
            return;

        auto WO = getWorkGroupWalkOrder();
        bool ForcedWalkOrder = false;
        if (WO.dim0 != 0 || WO.dim1 != 0 || WO.dim2 != 0)
        {
            if (auto Order = checkLegalWalkOrder(*Dims, WO))
            {
                ForcedWalkOrder = true;
                // Don't do TileY if forced in this way.
                m_ThreadIDLayout = ThreadIDLayout::X;
                m_walkOrder = *Order;
            }
            else
            {
                auto WalkOrder = getWalkOrder(WO.dim0, WO.dim1);
                if (WalkOrder != CS_WALK_ORDER::WO_XYZ)
                {
                    IGC_ASSERT_MESSAGE(0, "unhandled walk order!");
                }
                return;
            }
        }

        // OpenCL currently emits all local IDs even if only one dimension
        // is requested. Let's mirror that for now.
        ImplicitArgs implicitArgs(*entry, m_pMdUtils);
        if (implicitArgs.isImplicitArgExist(ImplicitArg::LOCAL_ID_X) ||
            implicitArgs.isImplicitArgExist(ImplicitArg::LOCAL_ID_Y) ||
            implicitArgs.isImplicitArgExist(ImplicitArg::LOCAL_ID_Z))
        {
            if (ForcedWalkOrder)
                m_enableHWGenerateLID = true;
            setEmitLocalMask(THREAD_ID_IN_GROUP_Z);
        }

        if (!ForcedWalkOrder)
        {
            selectWalkOrder(
                false,
                0,
                0,
                0, /* dummy 1D accesses */
                0, /* dummy 2D accesses */
                0, /* dummy SLM accessed */
                (*Dims)[0],
                (*Dims)[1],
                (*Dims)[2]);
        }
        encoder.GetVISABuilder()->SetOption(vISA_autoLoadLocalID, m_enableHWGenerateLID);
    }

    WorkGroupWalkOrderMD COpenCLKernel::getWorkGroupWalkOrder()
    {
        const CodeGenContext* pCtx = GetContext();
        const ModuleMetaData* MMD = pCtx->getModuleMetaData();
        if (auto I = MMD->FuncMD.find(entry); I != MMD->FuncMD.end())
        {
            auto& FMD = I->second;
            auto& Order = FMD.workGroupWalkOrder;
            return Order;
        }

        return {};
    }

    SOpenCLKernelInfo::SResourceInfo COpenCLKernel::getResourceInfo(int argNo)
    {
        CodeGenContext* pCtx = GetContext();
        ModuleMetaData* modMD = pCtx->getModuleMetaData();
        FunctionMetaData* funcMD = &modMD->FuncMD[entry];
        ResourceAllocMD* resAllocMD = &funcMD->resAllocMD;
        IGC_ASSERT_MESSAGE(resAllocMD->argAllocMDList.size() > 0, "ArgAllocMD List Out of Bounds");
        ArgAllocMD* argAlloc = &resAllocMD->argAllocMDList[argNo];

        SOpenCLKernelInfo::SResourceInfo resInfo;
        ResourceTypeEnum type = (ResourceTypeEnum)argAlloc->type;

        if (type == ResourceTypeEnum::UAVResourceType ||
            type == ResourceTypeEnum::BindlessUAVResourceType)
        {
            resInfo.Type = SOpenCLKernelInfo::SResourceInfo::RES_UAV;
        }
        else if (type == ResourceTypeEnum::SRVResourceType)
        {
            resInfo.Type = SOpenCLKernelInfo::SResourceInfo::RES_SRV;
        }
        else
        {
            resInfo.Type = SOpenCLKernelInfo::SResourceInfo::RES_OTHER;
        }
        resInfo.Index = argAlloc->indexType;
        return resInfo;
    }

    ResourceExtensionTypeEnum COpenCLKernel::getExtensionInfo(int argNo)
    {
        CodeGenContext* pCtx = GetContext();
        ModuleMetaData* modMD = pCtx->getModuleMetaData();
        FunctionMetaData* funcMD = &modMD->FuncMD[entry];
        ResourceAllocMD* resAllocMD = &funcMD->resAllocMD;
        IGC_ASSERT_MESSAGE(resAllocMD->argAllocMDList.size() > 0, "ArgAllocMD List Out of Bounds");
        ArgAllocMD* argAlloc = &resAllocMD->argAllocMDList[argNo];
        return (ResourceExtensionTypeEnum)argAlloc->extensionType;
    }

    void COpenCLKernel::CreateInlineSamplerAnnotations()
    {
        if (m_Context->getModuleMetaData()->FuncMD.find(entry) != m_Context->getModuleMetaData()->FuncMD.end())
        {
            FunctionMetaData funcMD = m_Context->getModuleMetaData()->FuncMD.find(entry)->second;

            ResourceAllocMD resAllocMD = funcMD.resAllocMD;

            for (const auto &inlineSamplerMD : resAllocMD.inlineSamplersMD)
            {
                auto samplerInput = std::make_unique<iOpenCL::SamplerInputAnnotation>();

                samplerInput->SamplerType = iOpenCL::SAMPLER_OBJECT_TEXTURE;
                samplerInput->SamplerTableIndex = inlineSamplerMD.index;

                samplerInput->TCXAddressMode = iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE(inlineSamplerMD.TCXAddressMode);
                samplerInput->TCYAddressMode = iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE(inlineSamplerMD.TCYAddressMode);
                samplerInput->TCZAddressMode = iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE(inlineSamplerMD.TCZAddressMode);
                samplerInput->NormalizedCoords = inlineSamplerMD.NormalizedCoords != 0 ? true : false;

                samplerInput->MagFilterType = iOpenCL::SAMPLER_MAPFILTER_TYPE(inlineSamplerMD.MagFilterType);
                samplerInput->MinFilterType = iOpenCL::SAMPLER_MAPFILTER_TYPE(inlineSamplerMD.MinFilterType);
                samplerInput->MipFilterType = iOpenCL::SAMPLER_MIPFILTER_TYPE(inlineSamplerMD.MipFilterType);
                samplerInput->CompareFunc = iOpenCL::SAMPLER_COMPARE_FUNC_TYPE(inlineSamplerMD.CompareFunc);

                samplerInput->BorderColorR = inlineSamplerMD.BorderColorR;
                samplerInput->BorderColorG = inlineSamplerMD.BorderColorG;
                samplerInput->BorderColorB = inlineSamplerMD.BorderColorB;
                samplerInput->BorderColorA = inlineSamplerMD.BorderColorA;

                m_kernelInfo.m_samplerInput.push_back(std::move(samplerInput));
            }

            m_kernelInfo.m_HasInlineVmeSamplers = funcMD.hasInlineVmeSamplers;
        }
    }

    void COpenCLKernel::CreateZEInlineSamplerAnnotations()
    {
        IGC_ASSERT(m_Context->enableZEBinary());

        auto getZESamplerAddrMode = [](int addrMode) -> zebin::PreDefinedAttrGetter::ArgSamplerAddrMode
        {
            switch (addrMode) {
            case LEGACY_CLK_ADDRESS_NONE:
                return zebin::PreDefinedAttrGetter::ArgSamplerAddrMode::none;
            case LEGACY_CLK_ADDRESS_CLAMP:
                return zebin::PreDefinedAttrGetter::ArgSamplerAddrMode::clamp_border;
            case LEGACY_CLK_ADDRESS_CLAMP_TO_EDGE:
                return zebin::PreDefinedAttrGetter::ArgSamplerAddrMode::clamp_edge;
            case LEGACY_CLK_ADDRESS_REPEAT:
                return zebin::PreDefinedAttrGetter::ArgSamplerAddrMode::repeat;
            case LEGACY_CLK_ADDRESS_MIRRORED_REPEAT:
                return zebin::PreDefinedAttrGetter::ArgSamplerAddrMode::mirror;
            default:
                IGC_ASSERT_MESSAGE(false, "Unsupported sampler addressing mode");
                return zebin::PreDefinedAttrGetter::ArgSamplerAddrMode::none;
            }
        };

        auto getZESamplerFilterMode = [](int filterMode) -> zebin::PreDefinedAttrGetter::ArgSamplerFilterMode
        {
            switch (filterMode) {
            case iOpenCL::SAMPLER_MAPFILTER_POINT:
                return zebin::PreDefinedAttrGetter::ArgSamplerFilterMode::nearest;
            case iOpenCL::SAMPLER_MAPFILTER_LINEAR:
                return zebin::PreDefinedAttrGetter::ArgSamplerFilterMode::linear;
            default:
                IGC_ASSERT_MESSAGE(false, "Unsupported sampler filter mode");
                return zebin::PreDefinedAttrGetter::ArgSamplerFilterMode::nearest;
            }
        };

        auto funcMDIter = m_Context->getModuleMetaData()->FuncMD.find(entry);
        if (funcMDIter != m_Context->getModuleMetaData()->FuncMD.end())
        {
            const ResourceAllocMD& resAllocMD = funcMDIter->second.resAllocMD;

            for (const auto &inlineSamplerMD : resAllocMD.inlineSamplersMD)
            {
                auto addrMode = getZESamplerAddrMode(inlineSamplerMD.addressMode);
                auto filterMode = getZESamplerFilterMode(inlineSamplerMD.MagFilterType);
                bool normalized = inlineSamplerMD.NormalizedCoords != 0 ? true : false;
                zebin::ZEInfoBuilder::addInlineSampler(m_kernelInfo.m_zeInlineSamplers,
                    inlineSamplerMD.index, addrMode, filterMode, normalized);
            }
        }
    }

    std::string COpenCLKernel::getKernelArgTypeName(const FunctionMetaData& funcMD, uint argIndex) const
    {
        // The type name is expected to also have the type size, appended after a ";"
        std::string result = funcMD.m_OpenCLArgTypes[argIndex] + ";";

        // Unfortunately, unlike SPIR, legacy OCL uses an ABI that has byval pointers.
        // So, if the parameter is a byval pointer, look at the contained type
        Function::arg_iterator argumentIter = entry->arg_begin();
        std::advance(argumentIter, argIndex);

        Type* argType = entry->getFunctionType()->getParamType(argIndex);
        if (argumentIter->hasByValAttr())
        {
            argType = argType->getContainedType(0);
        }

        result += utostr(m_DL->getTypeAllocSize(argType));
        return result;
    }

    std::string COpenCLKernel::getKernelArgTypeQualifier(const FunctionMetaData& funcMD, uint argIndex) const
    {
        // If there are no type qualifiers, "NONE" is expected
        std::string result = funcMD.m_OpenCLArgTypeQualifiers[argIndex];
        if (result.empty())
        {
            result = "NONE";
        }
        return result;
    }

    std::string COpenCLKernel::getKernelArgAddressQualifier(const FunctionMetaData& funcMD, uint argIndex) const
    {
        // The address space is expected to have a __ prefix
        switch (funcMD.m_OpenCLArgAddressSpaces[argIndex])
        {
        case ADDRESS_SPACE_CONSTANT:
            return "__constant";
        case ADDRESS_SPACE_GLOBAL:
            return "__global";
        case ADDRESS_SPACE_LOCAL:
            return "__local";
        case ADDRESS_SPACE_PRIVATE:
            return "__private";
        default:
            m_Context->EmitError("Generic pointers are not allowed as kernel argument storage class!", nullptr);
            IGC_ASSERT_MESSAGE(0, "Unexpected address space");
            break;
        }
        return "";
    }

    std::string COpenCLKernel::getKernelArgAccessQualifier(const FunctionMetaData& funcMD, uint argIndex) const
    {
        // The access qualifier is expected to have a "__" prefix, or an upper-case "NONE" if there is no qualifier
        std::string result = funcMD.m_OpenCLArgAccessQualifiers[argIndex];
        if (result == "none" || result == "")
        {
            result = "NONE";
        }
        else if (result[0] != '_')
        {
            result = "__" + result;
        }
        return result;
    }

    void COpenCLKernel::CreateKernelArgInfo()
    {
        auto funcMDIt = m_Context->getModuleMetaData()->FuncMD.find(entry);
        if (funcMDIt == m_Context->getModuleMetaData()->FuncMD.end())
            return;

        FunctionMetaData& funcMD = (*funcMDIt).second;
        uint count = funcMD.m_OpenCLArgAccessQualifiers.size();
        for (uint i = 0; i < count; ++i)
        {
            auto kernelArgInfo = std::make_unique<iOpenCL::KernelArgumentInfoAnnotation>();

            kernelArgInfo->AccessQualifier = getKernelArgAccessQualifier(funcMD, i);
            kernelArgInfo->AddressQualifier = getKernelArgAddressQualifier(funcMD, i);
            // ArgNames is not guaranteed to be present if -cl-kernel-arg-info
            // is not passed in.
            if (funcMD.m_OpenCLArgNames.size() > i)
            {
                kernelArgInfo->ArgumentName = funcMD.m_OpenCLArgNames[i];
            }
            kernelArgInfo->TypeName = getKernelArgTypeName(funcMD, i);
            kernelArgInfo->TypeQualifier = getKernelArgTypeQualifier(funcMD, i);

            m_kernelInfo.m_kernelArgInfo.push_back(std::move(kernelArgInfo));
        }
    }

    void COpenCLKernel::CreateKernelAttributeInfo()
    {
        FunctionInfoMetaDataHandle funcInfoMD = m_pMdUtils->getFunctionsInfoItem(entry);

        // We need to concatenate 2 things:
        // (a) LLVM attributes, except nounwind. Why? Because that's how IGIL does it.
        // (b) The attributes that get translated into SPIR metadata:
        //     (*) vec_type_hint
        //     (*) reqd_work_group_size
        //     (*) work_group_size_hint
        //

        // Get LLVM function attributes, and erase "nounwind" if necessary
        m_kernelInfo.m_kernelAttributeInfo = entry->getAttributes().getAsString(-1);
        size_t nounwindLoc = m_kernelInfo.m_kernelAttributeInfo.find("nounwind");
        if (nounwindLoc != std::string::npos)
        {
            //8 is the length of "nounwind".
            //If this is not the first attribute, it has a leading space, which we also want to delete.
            int eraseLen = 8;
            if (nounwindLoc != 0)
            {
                nounwindLoc--;
                eraseLen++;
            }
            m_kernelInfo.m_kernelAttributeInfo.erase(nounwindLoc, eraseLen);
        }

        // Now fill in the special OCL attributes from the MD
        VectorTypeHintMetaDataHandle vecTypeHintInfo = funcInfoMD->getOpenCLVectorTypeHint();
        if (vecTypeHintInfo->hasValue())
        {
            m_kernelInfo.m_kernelAttributeInfo += " " + getVecTypeHintString(vecTypeHintInfo);
        }
        SubGroupSizeMetaDataHandle subGroupSize = funcInfoMD->getSubGroupSize();
        if (subGroupSize->hasValue())
        {
            m_kernelInfo.m_kernelAttributeInfo += " " + getSubGroupSizeString(subGroupSize);
        }

        auto it = m_Context->getModuleMetaData()->FuncMD.find(entry);
        if (it != m_Context->getModuleMetaData()->FuncMD.end())
        {
            WorkGroupWalkOrderMD workgroupWalkOrder = it->second.workGroupWalkOrder;
            if (workgroupWalkOrder.dim0 || workgroupWalkOrder.dim1 || workgroupWalkOrder.dim2)
            {
                m_kernelInfo.m_kernelAttributeInfo += " " + getWorkgroupWalkOrderString(workgroupWalkOrder);
            }
        }

        ThreadGroupSizeMetaDataHandle threadGroupSize = funcInfoMD->getThreadGroupSize();
        if (threadGroupSize->hasValue())
        {
            m_kernelInfo.m_kernelAttributeInfo += " " + getThreadGroupSizeString(threadGroupSize, false);
        }

        ThreadGroupSizeMetaDataHandle threadGroupSizeHint = funcInfoMD->getThreadGroupSizeHint();
        if (threadGroupSizeHint->hasValue())
        {
            m_kernelInfo.m_kernelAttributeInfo += " " + getThreadGroupSizeString(threadGroupSizeHint, true);
        }
    }

    std::string COpenCLKernel::getThreadGroupSizeString(ThreadGroupSizeMetaDataHandle& threadGroupSize, bool isHint)
    {
        std::string threadGroupSizeString = "";
        if (isHint)
        {
            threadGroupSizeString = "work_group_size_hint(";
        }
        else
        {
            threadGroupSizeString = "reqd_work_group_size(";
        }

        threadGroupSizeString += utostr(threadGroupSize->getXDim()) + ",";
        threadGroupSizeString += utostr(threadGroupSize->getYDim()) + ",";
        threadGroupSizeString += utostr(threadGroupSize->getZDim());

        threadGroupSizeString += ")";
        return threadGroupSizeString;
    }
    std::string COpenCLKernel::getSubGroupSizeString(SubGroupSizeMetaDataHandle& subGroupSize)
    {
        std::string subTypeString = "intel_reqd_sub_group_size(";
        subTypeString += utostr(subGroupSize->getSIMD_size());
        subTypeString += ")";
        return subTypeString;
    }
    std::string COpenCLKernel::getWorkgroupWalkOrderString(const IGC::WorkGroupWalkOrderMD& workgroupWalkOrder)
    {
        std::string subTypeString = "intel_reqd_workgroup_walk_order(";
        subTypeString += utostr(workgroupWalkOrder.dim0) + ",";
        subTypeString += utostr(workgroupWalkOrder.dim1) + ",";
        subTypeString += utostr(workgroupWalkOrder.dim2) + ",";
        subTypeString += ")";
        return subTypeString;
    }

    std::string COpenCLKernel::getVecTypeHintTypeString(const VectorTypeHintMetaDataHandle& vecTypeHintInfo) const
    {
        std::string vecTypeString;

        // Get the information about the type
        Type* baseType = vecTypeHintInfo->getVecType()->getType();
        unsigned int numElements = 1;
        if (baseType->isVectorTy())
        {
            numElements = (unsigned)cast<IGCLLVM::FixedVectorType>(baseType)->getNumElements();
            baseType = cast<VectorType>(baseType)->getElementType();
        }

        // Integer types need to be qualified with a "u" if they are unsigned
        if (baseType->isIntegerTy())
        {
            std::string signString = vecTypeHintInfo->getSign() ? "" : "u";
            vecTypeString += signString;
        }

        switch (baseType->getTypeID())
        {
        case Type::IntegerTyID:
            switch (baseType->getIntegerBitWidth())
            {
            case 8:
                vecTypeString += "char";
                break;
            case 16:
                vecTypeString += "short";
                break;
            case 32:
                vecTypeString += "int";
                break;
            case 64:
                vecTypeString += "long";
                break;
            default:
                IGC_ASSERT_MESSAGE(0, "Unexpected data type in vec_type_hint");
                break;
            }
            break;
        case Type::DoubleTyID:
            vecTypeString += "double";
            break;
        case Type::FloatTyID:
            vecTypeString += "float";
            break;
        case Type::HalfTyID:
            vecTypeString += "half";
            break;
        default:
            IGC_ASSERT_MESSAGE(0, "Unexpected data type in vec_type_hint");
            break;
        }

        if (numElements != 1)
        {
            vecTypeString += utostr(numElements);
        }

        return vecTypeString;
    }

    std::string COpenCLKernel::getVecTypeHintString(const VectorTypeHintMetaDataHandle& vecTypeHintInfo) const
    {
        std::string vecTypeString = "vec_type_hint(";
        vecTypeString += getVecTypeHintTypeString(vecTypeHintInfo);
        vecTypeString += ")";
        return vecTypeString;
    }

    void COpenCLKernel::CreatePrintfStringAnnotations()
    {
        auto printfStrings = GetPrintfStrings(*entry->getParent());

        for (const auto& printfString : printfStrings)
        {
            auto printfAnnotation = std::make_unique<iOpenCL::PrintfStringAnnotation>();
            printfAnnotation->Index = printfString.first;
            printfAnnotation->StringSize = printfString.second.size() + 1;
            printfAnnotation->StringData = new char[printfAnnotation->StringSize + 1];

            memcpy_s(printfAnnotation->StringData, printfAnnotation->StringSize, printfString.second.c_str(), printfAnnotation->StringSize);
            printfAnnotation->StringData[printfAnnotation->StringSize - 1] = '\0';

            m_kernelInfo.m_printfStringAnnotations.push_back(std::move(printfAnnotation));
        }
    }

    bool COpenCLKernel::CreateZEPayloadArguments(
        IGC::KernelArg* kernelArg, uint payloadPosition,
        PtrArgsAttrMapType& ptrArgsAttrMap)
    {
#ifndef DX_ONLY_IGC
#ifndef VK_ONLY_IGC
        switch (kernelArg->getArgType()) {

        case KernelArg::ArgType::IMPLICIT_PAYLOAD_HEADER:{
            // PayloadHeader contains global work offset x,y,z and local size x,y,z
            // global work offset, size is int32x3
            uint cur_pos = payloadPosition;
            uint32_t size = iOpenCL::DATA_PARAMETER_DATA_SIZE * 3;
            zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                zebin::PreDefinedAttrGetter::ArgType::global_id_offset, cur_pos, size);
            cur_pos += size;
            // local size, size is int32x3, the same as above
            zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                zebin::PreDefinedAttrGetter::ArgType::local_size, cur_pos, size);
            break;
        }
        case KernelArg::ArgType::IMPLICIT_PRIVATE_BASE:
            zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                zebin::PreDefinedAttrGetter::ArgType::private_base_stateless,
                payloadPosition, kernelArg->getAllocateSize());
            break;

        case KernelArg::ArgType::IMPLICIT_NUM_GROUPS:
            zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                zebin::PreDefinedAttrGetter::ArgType::group_count,
                payloadPosition, iOpenCL::DATA_PARAMETER_DATA_SIZE * 3);
            break;

        case KernelArg::ArgType::IMPLICIT_LOCAL_SIZE:
            // FIXME: duplicated information as KernelArg::ArgType::IMPLICIT_PAYLOAD_HEADER?
            zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                zebin::PreDefinedAttrGetter::ArgType::local_size,
                payloadPosition, iOpenCL::DATA_PARAMETER_DATA_SIZE * 3);
            break;

         case KernelArg::ArgType::IMPLICIT_ENQUEUED_LOCAL_WORK_SIZE:
             zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                 zebin::PreDefinedAttrGetter::ArgType::enqueued_local_size,
                 payloadPosition, iOpenCL::DATA_PARAMETER_DATA_SIZE * 3);
             break;

         case KernelArg::ArgType::IMPLICIT_GLOBAL_SIZE:
             zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                 zebin::PreDefinedAttrGetter::ArgType::global_size,
                 payloadPosition, iOpenCL::DATA_PARAMETER_DATA_SIZE * 3);
             break;

         case KernelArg::ArgType::IMPLICIT_WORK_DIM:
             zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                 zebin::PreDefinedAttrGetter::ArgType::work_dimensions,
                 payloadPosition, iOpenCL::DATA_PARAMETER_DATA_SIZE);
             break;

        // pointer args
        case KernelArg::ArgType::PTR_GLOBAL:
        case KernelArg::ArgType::PTR_CONSTANT: {
            uint32_t arg_idx = kernelArg->getAssociatedArgNo();

            FunctionMetaData& funcMD = GetContext()->getModuleMetaData()->FuncMD[entry];
            auto addr_space =
                kernelArg->getArgType() == KernelArg::ArgType::PTR_GLOBAL
                    ? zebin::PreDefinedAttrGetter::ArgAddrSpace::global
                    : zebin::PreDefinedAttrGetter::ArgAddrSpace::constant;
            auto access_type = zebin::PreDefinedAttrGetter::ArgAccessType::readwrite;
            if (kernelArg->getArgType() == KernelArg::ArgType::PTR_CONSTANT ||
                funcMD.m_OpenCLArgTypeQualifiers[arg_idx] == "const")
                access_type = zebin::PreDefinedAttrGetter::ArgAccessType::readonly;

            // FIXME: do not set bti if the number is 0xffffffff (?)
            SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(arg_idx);
            uint32_t bti_idx = getBTI(resInfo);
            // There are 3 stateful modes, we need to create different payload_arguments for
            // each mode: BTI mode, Bindless legacy mode and Bindless advance
            bool is_stateful_mode = bti_idx != 0xffffffff;

            if (is_stateful_mode && (!m_Context->m_InternalOptions.UseBindlessMode)) {
                // Add BTI argument if being promoted to stateful BTI mode
                // promoted arg has 0 offset and 0 size
                zebin::ZEInfoBuilder::addPayloadArgumentByPointer(m_kernelInfo.m_zePayloadArgs,
                    0, 0, arg_idx,
                    zebin::PreDefinedAttrGetter::ArgAddrMode::stateful,
                    addr_space, access_type);
                // add the corresponding BTI table index
                zebin::ZEInfoBuilder::addBindingTableIndex(m_kernelInfo.m_zeBTIArgs,
                    bti_idx, arg_idx);
            }

            // check if all reference are promoted, if it is, we can skip creating stateless payload arg
            bool is_stateful_only =
                is_stateful_mode &&
                IGC_IS_FLAG_ENABLED(EnableStatelessToStateful) &&
                IGC_IS_FLAG_ENABLED(EnableStatefulToken) &&
                m_DriverInfo->SupportStatefulToken() &&
                !m_Context->getModuleMetaData()->compOpt.GreaterThan4GBBufferRequired &&
                kernelArg->getArg() &&
                ((kernelArg->getArgType() == KernelArg::ArgType::PTR_GLOBAL &&
                (kernelArg->getArg()->use_empty() || !GetHasGlobalStatelessAccess())) ||
                    (kernelArg->getArgType() == KernelArg::ArgType::PTR_CONSTANT &&
                    (kernelArg->getArg()->use_empty() || !GetHasConstantStatelessAccess())));

            bool is_bindless_legacy_mode =
                m_Context->m_InternalOptions.UseBindlessMode &&
                m_Context->m_InternalOptions.UseBindlessLegacyMode;
            bool is_bindless_advance_mode =
                m_Context->m_InternalOptions.UseBindlessMode &&
                !m_Context->m_InternalOptions.UseBindlessLegacyMode;

            // When on bindless advance mode, there's an IMPLICIT_BINDLESS_OFFSET argument generated
            // associated to this argument. Keep track of addrspace and access_type for setting the
            // IMPLICIT_BINDLESS_OFFSET's attributes
            if (is_bindless_advance_mode)
                ptrArgsAttrMap[arg_idx] = std::make_pair(addr_space, access_type);

            // For BindlessLegacyMode, this argument represents the bindless offset of the argument. Skip buffer_address
            // creation and fall through to below bindless payload_argument creation
            // For BindlessAdvanceMode and BTI modes, this argument represents either the original stateless address
            // or the "buffer_address" when all accesses are promoted to stateful
            if (is_stateful_only && !is_bindless_legacy_mode) {
                // create buffer_address for statefull only arg in case of address check is needed
                // for example: something like "if(buffer != nullptr)" in the kernel.
                // this address will be accessed as a value and cannot be de-referenced
                zebin::zeInfoPayloadArgument& arg = zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                    zebin::PreDefinedAttrGetter::ArgType::buffer_address,
                    payloadPosition, kernelArg->getAllocateSize());
                arg.arg_index = arg_idx;
                break;
            }

            ResourceAllocMD& resAllocMD = GetContext()->getModuleMetaData()->FuncMD[entry].resAllocMD;
            IGC_ASSERT_MESSAGE(resAllocMD.argAllocMDList.size() > 0, "ArgAllocMDList is empty.");

            ArgAllocMD& argAlloc = resAllocMD.argAllocMDList[arg_idx];

            zebin::PreDefinedAttrGetter::ArgAddrMode addr_mode =
                zebin::PreDefinedAttrGetter::ArgAddrMode::stateless;
            if (argAlloc.type == ResourceTypeEnum::BindlessUAVResourceType)
                addr_mode = zebin::PreDefinedAttrGetter::ArgAddrMode::bindless;

            zebin::zeInfoPayloadArgument& arg =
                zebin::ZEInfoBuilder::addPayloadArgumentByPointer(m_kernelInfo.m_zePayloadArgs,
                    payloadPosition, kernelArg->getAllocateSize(), arg_idx, addr_mode,
                    addr_space, access_type);
            arg.is_pipe = funcMD.m_OpenCLArgTypeQualifiers[arg_idx] == "pipe";
            break;
        }
        case KernelArg::ArgType::PTR_LOCAL:
            zebin::ZEInfoBuilder::addPayloadArgumentByPointer(m_kernelInfo.m_zePayloadArgs,
                payloadPosition, kernelArg->getAllocateSize(),
                kernelArg->getAssociatedArgNo(),
                zebin::PreDefinedAttrGetter::ArgAddrMode::slm,
                zebin::PreDefinedAttrGetter::ArgAddrSpace::local,
                zebin::PreDefinedAttrGetter::ArgAccessType::readwrite,
                kernelArg->getAlignment());
            break;
        // by value arguments
        case KernelArg::ArgType::CONSTANT_REG:
            zebin::ZEInfoBuilder::addPayloadArgumentByValue(m_kernelInfo.m_zePayloadArgs,
                payloadPosition, kernelArg->getAllocateSize(),
                kernelArg->getAssociatedArgNo(), kernelArg->getStructArgOffset(),
                kernelArg->isScalarAsPointer());
            break;

        // Local ids are supported in per-thread payload arguments
        case KernelArg::ArgType::IMPLICIT_LOCAL_IDS:
            break;

        // Bindless offset for pointer argument. This ArgType presents when bindless-advanced-mode
        // is enabled
        case KernelArg::ArgType::IMPLICIT_BINDLESS_OFFSET: {
            auto argidx = kernelArg->getAssociatedArgNo();
            IGC_ASSERT_MESSAGE(
                ptrArgsAttrMap.find(argidx) != ptrArgsAttrMap.end(),
                "Cannot find ptrArgsAttr for IMPLICIT_BINDLESS_OFFSET");
            PtrArgAttrType &attrs = ptrArgsAttrMap[argidx];
            zebin::ZEInfoBuilder::addPayloadArgumentByPointer(
                m_kernelInfo.m_zePayloadArgs, payloadPosition,
                kernelArg->getAllocateSize(), argidx,
                zebin::PreDefinedAttrGetter::ArgAddrMode::bindless,
                attrs.first, attrs.second);
            break;
        }
        // Images
        case KernelArg::ArgType::IMAGE_1D:
        case KernelArg::ArgType::BINDLESS_IMAGE_1D:
        case KernelArg::ArgType::IMAGE_1D_BUFFER:
        case KernelArg::ArgType::BINDLESS_IMAGE_1D_BUFFER:
        case KernelArg::ArgType::IMAGE_2D:
        case KernelArg::ArgType::BINDLESS_IMAGE_2D:
        case KernelArg::ArgType::IMAGE_3D:
        case KernelArg::ArgType::BINDLESS_IMAGE_3D:
        case KernelArg::ArgType::IMAGE_CUBE:
        case KernelArg::ArgType::BINDLESS_IMAGE_CUBE:
        case KernelArg::ArgType::IMAGE_CUBE_DEPTH:
        case KernelArg::ArgType::BINDLESS_IMAGE_CUBE_DEPTH:
        case KernelArg::ArgType::IMAGE_1D_ARRAY:
        case KernelArg::ArgType::BINDLESS_IMAGE_1D_ARRAY:
        case KernelArg::ArgType::IMAGE_2D_ARRAY:
        case KernelArg::ArgType::BINDLESS_IMAGE_2D_ARRAY:
        case KernelArg::ArgType::IMAGE_2D_DEPTH:
        case KernelArg::ArgType::BINDLESS_IMAGE_2D_DEPTH:
        case KernelArg::ArgType::IMAGE_2D_DEPTH_ARRAY:
        case KernelArg::ArgType::BINDLESS_IMAGE_2D_DEPTH_ARRAY:
        case KernelArg::ArgType::IMAGE_2D_MSAA:
        case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA:
        case KernelArg::ArgType::IMAGE_2D_MSAA_ARRAY:
        case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_ARRAY:
        case KernelArg::ArgType::IMAGE_2D_MSAA_DEPTH:
        case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_DEPTH:
        case KernelArg::ArgType::IMAGE_2D_MSAA_DEPTH_ARRAY:
        case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_DEPTH_ARRAY:
        case KernelArg::ArgType::IMAGE_CUBE_ARRAY:
        case KernelArg::ArgType::BINDLESS_IMAGE_CUBE_ARRAY:
        case KernelArg::ArgType::IMAGE_CUBE_DEPTH_ARRAY:
        case KernelArg::ArgType::BINDLESS_IMAGE_CUBE_DEPTH_ARRAY: {
            // the image arg is either bindless or stateful. check from "kernelArg->needsAllocation()"
            // For stateful image argument, the arg has 0 offset and 0 size
            zebin::PreDefinedAttrGetter::ArgAddrMode arg_addrmode =
                zebin::PreDefinedAttrGetter::ArgAddrMode::stateful;
            uint arg_off = 0;
            uint arg_size = 0;

            int arg_idx = kernelArg->getAssociatedArgNo();
            if (kernelArg->needsAllocation()) {
                // set to bindless
                arg_addrmode =
                    zebin::PreDefinedAttrGetter::ArgAddrMode::bindless;
                arg_off = payloadPosition;
                arg_size = kernelArg->getAllocateSize();
            } else {
                // add bti index for this arg if it's stateful
                SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(arg_idx);
                zebin::ZEInfoBuilder::addBindingTableIndex(m_kernelInfo.m_zeBTIArgs,
                    getBTI(resInfo), arg_idx);
            }

            auto access_type = [](KernelArg::AccessQual qual) {
                if (qual == KernelArg::AccessQual::READ_ONLY)
                    return zebin::PreDefinedAttrGetter::ArgAccessType::readonly;
                if (qual == KernelArg::AccessQual::WRITE_ONLY)
                    return zebin::PreDefinedAttrGetter::ArgAccessType::writeonly;
                return zebin::PreDefinedAttrGetter::ArgAccessType::readwrite;
            } (kernelArg->getAccessQual());

            auto image_type = getZEImageType(getImageTypeFromKernelArg(*kernelArg));

            // add the payload argument
            zebin::zeInfoPayloadArgument& arg =
                zebin::ZEInfoBuilder::addPayloadArgumentImage(m_kernelInfo.m_zePayloadArgs,
                    arg_off, arg_size, arg_idx, arg_addrmode, access_type, image_type);
            // TODO: When ZEBIN path supports inline sampler, follow Patch
            // Token path to check InlineSamplersAllow3DImageTransformation().
            arg.image_transformable =
                kernelArg->getArgType() == KernelArg::ArgType::IMAGE_3D &&
                kernelArg->getImgAccessedIntCoords() &&
                !kernelArg->getImgAccessedFloatCoords();
            break;
        }
        case KernelArg::ArgType::IMPLICIT_IMAGE_HEIGHT: {
            zebin::zeInfoPayloadArgument& arg =
                zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                    zebin::PreDefinedAttrGetter::ArgType::image_height,
                    payloadPosition, kernelArg->getAllocateSize());
            arg.arg_index = kernelArg->getAssociatedArgNo();
            break;
        }
        case KernelArg::ArgType::IMPLICIT_IMAGE_WIDTH: {
            zebin::zeInfoPayloadArgument& arg =
                zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                    zebin::PreDefinedAttrGetter::ArgType::image_width,
                    payloadPosition, kernelArg->getAllocateSize());
            arg.arg_index = kernelArg->getAssociatedArgNo();
            break;
        }
        case KernelArg::ArgType::IMPLICIT_IMAGE_DEPTH: {
            zebin::zeInfoPayloadArgument& arg =
                zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                    zebin::PreDefinedAttrGetter::ArgType::image_depth,
                    payloadPosition, kernelArg->getAllocateSize());
            arg.arg_index = kernelArg->getAssociatedArgNo();
            break;
        }
        case KernelArg::ArgType::IMPLICIT_IMAGE_NUM_MIP_LEVELS: {
            zebin::zeInfoPayloadArgument& arg =
                zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                    zebin::PreDefinedAttrGetter::ArgType::image_num_mip_levels,
                    payloadPosition, kernelArg->getAllocateSize());
            arg.arg_index = kernelArg->getAssociatedArgNo();
            break;
        }
        case KernelArg::ArgType::IMPLICIT_IMAGE_CHANNEL_DATA_TYPE: {
            zebin::zeInfoPayloadArgument& arg =
                zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                    zebin::PreDefinedAttrGetter::ArgType::image_channel_data_type,
                    payloadPosition, kernelArg->getAllocateSize());
            arg.arg_index = kernelArg->getAssociatedArgNo();
            break;
        }
        case KernelArg::ArgType::IMPLICIT_IMAGE_CHANNEL_ORDER: {
            zebin::zeInfoPayloadArgument& arg =
                zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                    zebin::PreDefinedAttrGetter::ArgType::image_channel_order,
                    payloadPosition, kernelArg->getAllocateSize());
            arg.arg_index = kernelArg->getAssociatedArgNo();
            break;
        }
        case KernelArg::ArgType::IMPLICIT_IMAGE_SRGB_CHANNEL_ORDER: {
            zebin::zeInfoPayloadArgument& arg =
                zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                    zebin::PreDefinedAttrGetter::ArgType::image_srgb_channel_order,
                    payloadPosition, kernelArg->getAllocateSize());
            arg.arg_index = kernelArg->getAssociatedArgNo();
            break;
        }
        case KernelArg::ArgType::IMPLICIT_IMAGE_ARRAY_SIZE: {
            zebin::zeInfoPayloadArgument& arg =
                zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                    zebin::PreDefinedAttrGetter::ArgType::image_array_size,
                    payloadPosition, kernelArg->getAllocateSize());
            arg.arg_index = kernelArg->getAssociatedArgNo();
            break;
        }
        case KernelArg::ArgType::IMPLICIT_IMAGE_NUM_SAMPLES: {
            zebin::zeInfoPayloadArgument& arg =
                zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                    zebin::PreDefinedAttrGetter::ArgType::image_num_samples,
                    payloadPosition, kernelArg->getAllocateSize());
            arg.arg_index = kernelArg->getAssociatedArgNo();
            break;
        }
        case KernelArg::ArgType::IMPLICIT_FLAT_IMAGE_BASEOFFSET: {
            zebin::zeInfoPayloadArgument& arg =
                zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                    zebin::PreDefinedAttrGetter::ArgType::flat_image_baseoffset,
                    payloadPosition, kernelArg->getAllocateSize());
            arg.arg_index = kernelArg->getAssociatedArgNo();
            break;
        }
        case KernelArg::ArgType::IMPLICIT_FLAT_IMAGE_HEIGHT: {
            zebin::zeInfoPayloadArgument& arg =
                zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                    zebin::PreDefinedAttrGetter::ArgType::flat_image_height,
                    payloadPosition, kernelArg->getAllocateSize());
            arg.arg_index = kernelArg->getAssociatedArgNo();
            break;
        }
        case KernelArg::ArgType::IMPLICIT_FLAT_IMAGE_WIDTH: {
            zebin::zeInfoPayloadArgument& arg =
                zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                    zebin::PreDefinedAttrGetter::ArgType::flat_image_width,
                    payloadPosition, kernelArg->getAllocateSize());
            arg.arg_index = kernelArg->getAssociatedArgNo();
            break;
        }
        case KernelArg::ArgType::IMPLICIT_FLAT_IMAGE_PITCH: {
            zebin::zeInfoPayloadArgument& arg =
                zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                    zebin::PreDefinedAttrGetter::ArgType::flat_image_pitch,
                    payloadPosition, kernelArg->getAllocateSize());
            arg.arg_index = kernelArg->getAssociatedArgNo();
            break;
        }
        // sampler
        case KernelArg::ArgType::SAMPLER:
        case KernelArg::ArgType::BINDLESS_SAMPLER: {
            // the sampler arg is either bindless or stateful. check from "kernelArg->needsAllocation()"
            // For stateful image argument, the arg has 0 offset and 0 size
            // NOTE: we only have stateful sampler now
            zebin::PreDefinedAttrGetter::ArgAddrMode arg_addrmode =
                zebin::PreDefinedAttrGetter::ArgAddrMode::stateful;
            uint arg_off = 0;
            uint arg_size = 0;
            if (kernelArg->needsAllocation()) {
                // set to bindless
                arg_addrmode =
                    zebin::PreDefinedAttrGetter::ArgAddrMode::bindless;
                arg_off = payloadPosition;
                arg_size = kernelArg->getAllocateSize();
            }

            int arg_idx = kernelArg->getAssociatedArgNo();
            SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(arg_idx);
            auto sampler_type = getZESamplerType(getSamplerTypeFromKernelArg(*kernelArg));
            // add the payload argument
            zebin::ZEInfoBuilder::addPayloadArgumentSampler(m_kernelInfo.m_zePayloadArgs,
                arg_off, arg_size, arg_idx, resInfo.Index, arg_addrmode,
                zebin::PreDefinedAttrGetter::ArgAccessType::readwrite, sampler_type);
            break;
        }
        case KernelArg::ArgType::IMPLICIT_SAMPLER_ADDRESS: {
            zebin::zeInfoPayloadArgument& arg =
                zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                    zebin::PreDefinedAttrGetter::ArgType::sampler_address,
                    payloadPosition, kernelArg->getAllocateSize());
            arg.arg_index = kernelArg->getAssociatedArgNo();
            break;
        }
        case KernelArg::ArgType::IMPLICIT_SAMPLER_NORMALIZED: {
            zebin::zeInfoPayloadArgument& arg =
                zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                    zebin::PreDefinedAttrGetter::ArgType::sampler_normalized,
                    payloadPosition, kernelArg->getAllocateSize());
            arg.arg_index = kernelArg->getAssociatedArgNo();
            break;
        }
        case KernelArg::ArgType::IMPLICIT_SAMPLER_SNAP_WA: {
            zebin::zeInfoPayloadArgument& arg =
                zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                    zebin::PreDefinedAttrGetter::ArgType::sampler_snap_wa,
                    payloadPosition, kernelArg->getAllocateSize());
            arg.arg_index = kernelArg->getAssociatedArgNo();
            break;
        }

        case KernelArg::ArgType::IMPLICIT_BUFFER_OFFSET: {
            zebin::zeInfoPayloadArgument& arg = zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                zebin::PreDefinedAttrGetter::ArgType::buffer_offset,
                payloadPosition, kernelArg->getAllocateSize());
            arg.arg_index = kernelArg->getAssociatedArgNo();
            break;
        }
        case KernelArg::ArgType::IMPLICIT_PRINTF_BUFFER:
            zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                zebin::PreDefinedAttrGetter::ArgType::printf_buffer,
                payloadPosition, kernelArg->getAllocateSize());
            break;

        case KernelArg::ArgType::IMPLICIT_ARG_BUFFER:
            zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                zebin::PreDefinedAttrGetter::ArgType::implicit_arg_buffer,
                payloadPosition, kernelArg->getAllocateSize());
            break;

        case KernelArg::ArgType::IMPLICIT_SYNC_BUFFER:
            zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                zebin::PreDefinedAttrGetter::ArgType::sync_buffer,
                payloadPosition, kernelArg->getAllocateSize());
            break;

        case KernelArg::ArgType::IMPLICIT_RT_GLOBAL_BUFFER:
            zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                zebin::PreDefinedAttrGetter::ArgType::rt_global_buffer,
                payloadPosition, kernelArg->getAllocateSize());
            break;

        case KernelArg::ArgType::IMPLICIT_ASSERT_BUFFER:
            zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                zebin::PreDefinedAttrGetter::ArgType::assert_buffer,
                payloadPosition, kernelArg->getAllocateSize());
            break;

        case KernelArg::ArgType::IMPLICIT_CONSTANT_BASE:
        case KernelArg::ArgType::IMPLICIT_GLOBAL_BASE: {
          auto zeArgType =
              kernelArg->getArgType() ==
                      KernelArg::ArgType::IMPLICIT_CONSTANT_BASE
                  ? zebin::PreDefinedAttrGetter::ArgType::const_base
                  : zebin::PreDefinedAttrGetter::ArgType::global_base;
          zebin::zeInfoPayloadArgument &arg =
              zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
                  m_kernelInfo.m_zePayloadArgs, zeArgType, payloadPosition,
                  kernelArg->getAllocateSize());
          SOpenCLKernelInfo::SResourceInfo resInfo =
              getResourceInfo(kernelArg->getAssociatedArgNo());
          unsigned btiValue = getBTI(resInfo);
          if (btiValue != 0xffffffff)
            arg.bti_value = btiValue;
          break;
        }

        // We don't need these in ZEBinary, can safely skip them
        case KernelArg::ArgType::IMPLICIT_R0:
        case KernelArg::ArgType::R1:
        case KernelArg::ArgType::STRUCT:
            break;

        case KernelArg::ArgType::IMPLICIT_STAGE_IN_GRID_ORIGIN:
        case KernelArg::ArgType::IMPLICIT_STAGE_IN_GRID_SIZE:
        default:
            return false;
        } // end switch (kernelArg->getArgType())
#endif // ifndef VK_ONLY_IGC
#endif // ifndef DX_ONLY_IGC
        return true;
    }

    void COpenCLKernel::CreateAnnotations(KernelArg* kernelArg, uint payloadPosition)
    {
        KernelArg::ArgType type = kernelArg->getArgType();

        DWORD constantType = iOpenCL::DATA_PARAMETER_TOKEN_UNKNOWN;
        iOpenCL::POINTER_ADDRESS_SPACE addressSpace = iOpenCL::KERNEL_ARGUMENT_ADDRESS_SPACE_INVALID;
        FunctionInfoMetaDataHandle funcInfoMD = m_pMdUtils->getFunctionsInfoItem(entry);

        static const DWORD DEFAULT_ARG_NUM = 0;
        const llvm::Argument* arg = kernelArg->getArg();

        switch (type) {

        case KernelArg::ArgType::IMPLICIT_R0:
            for (Value::const_user_iterator U = arg->user_begin(), UE = arg->user_end(); U != UE; ++U)
            {
                const ExtractElementInst* EEI = dyn_cast<ExtractElementInst>(*U);

                if (EEI)
                {
                    const ConstantInt* index = dyn_cast<ConstantInt>(EEI->getIndexOperand());
                    if (index)
                    {
                        uint64_t value = index->getZExtValue();
                        if (value == 1 || value == 6 || value == 7)
                        {
                            // group ids x/y/z
                            ModuleMetaData* modMD = m_Context->getModuleMetaData();
                            auto it = modMD->FuncMD.find(entry);
                            if (it != modMD->FuncMD.end())
                            {
                                if (it->second.groupIDPresent == true)
                                    m_kernelInfo.m_threadPayload.HasGroupID = true;
                            }
                            break;
                        }
                    }
                }
            }
            break;

        case KernelArg::ArgType::IMPLICIT_PAYLOAD_HEADER:
            // PayloadHeader contains global work offset x,y,z and local size x,y,z -->
            // total of 6 annotations, 3 of each type
            for (int i = 0; i < 6; ++i)
            {
                auto constInput = std::make_unique<iOpenCL::ConstantInputAnnotation>();

                DWORD sizeInBytes = iOpenCL::DATA_PARAMETER_DATA_SIZE;

                constInput->ConstantType = (i < 3 ?
                    iOpenCL::DATA_PARAMETER_GLOBAL_WORK_OFFSET :
                    iOpenCL::DATA_PARAMETER_LOCAL_WORK_SIZE);
                constInput->Offset = (i % 3) * sizeInBytes;
                constInput->PayloadPosition = payloadPosition;
                constInput->PayloadSizeInBytes = sizeInBytes;
                constInput->ArgumentNumber = DEFAULT_ARG_NUM;
                m_kernelInfo.m_constantInputAnnotation.push_back(std::move(constInput));

                payloadPosition += sizeInBytes;
            }

            for (Value::const_user_iterator U = arg->user_begin(), UE = arg->user_end(); U != UE; ++U)
            {
                const ExtractElementInst* EEI = dyn_cast<ExtractElementInst>(*U);

                if (EEI)
                {
                    const ConstantInt* index = dyn_cast<ConstantInt>(EEI->getIndexOperand());
                    if (index)
                    {
                        uint64_t value = index->getZExtValue();
                        if (value == 0 || value == 1 || value == 2)
                        {
                            // global offset x/y/z
                            ModuleMetaData* modMD = m_Context->getModuleMetaData();
                            auto it = modMD->FuncMD.find(entry);
                            if (it != modMD->FuncMD.end())
                            {
                                if (it->second.globalIDPresent)
                                    m_kernelInfo.m_threadPayload.HasGlobalIDOffset = true;
                            }
                            break;
                        }
                    }
                }
            }
            break;

        case KernelArg::ArgType::IMPLICIT_BINDLESS_OFFSET:
            {
                int argNo = kernelArg->getAssociatedArgNo();
                std::shared_ptr<iOpenCL::PointerArgumentAnnotation> ptrAnnotation = m_kernelInfo.m_argOffsetMap[argNo];
                ptrAnnotation->BindingTableIndex = payloadPosition;
                ptrAnnotation->SecondPayloadSizeInBytes = kernelArg->getAllocateSize();
            }
            break;

        case KernelArg::ArgType::PTR_GLOBAL:
            if (addressSpace == iOpenCL::KERNEL_ARGUMENT_ADDRESS_SPACE_INVALID) {
                addressSpace = iOpenCL::KERNEL_ARGUMENT_ADDRESS_SPACE_GLOBAL;
            }
            // Fall through until PTR_CONSTANT
        case KernelArg::ArgType::PTR_CONSTANT:
            if (addressSpace == iOpenCL::KERNEL_ARGUMENT_ADDRESS_SPACE_INVALID) {
                addressSpace = iOpenCL::KERNEL_ARGUMENT_ADDRESS_SPACE_CONSTANT;
            }
            // may reach here from PTR_GLOBAL, PTR_CONSTANT
            IGC_ASSERT(addressSpace != iOpenCL::KERNEL_ARGUMENT_ADDRESS_SPACE_INVALID);

            {
                int argNo = kernelArg->getAssociatedArgNo();
                SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(argNo);
                unsigned bti = getBTI(resInfo);
                m_kernelInfo.m_argIndexMap[argNo] = bti;
                CodeGenContext* pCtx = GetContext();
                ModuleMetaData* modMD = pCtx->getModuleMetaData();
                FunctionMetaData* funcMD = &modMD->FuncMD[entry];
                ResourceAllocMD* resAllocMD = &funcMD->resAllocMD;
                IGC_ASSERT_MESSAGE(resAllocMD->argAllocMDList.size() > 0, "ArgAllocMDList is empty.");
                ArgAllocMD* argAlloc = &resAllocMD->argAllocMDList[argNo];

                auto ptrAnnotation = std::make_shared<iOpenCL::PointerArgumentAnnotation>();

                if (argAlloc->type == ResourceTypeEnum::BindlessUAVResourceType)
                {
                    ptrAnnotation->IsStateless = false;
                    ptrAnnotation->IsBindlessAccess = true;
                }
                else
                {
                    ptrAnnotation->IsStateless = true;
                    ptrAnnotation->IsBindlessAccess = false;
                }

                m_kernelInfo.m_argOffsetMap[argNo] = ptrAnnotation;

                ptrAnnotation->HasStatefulAccess = hasStatefulAccess(bti);
                ptrAnnotation->AddressSpace = addressSpace;
                ptrAnnotation->ArgumentNumber = argNo;
                ptrAnnotation->BindingTableIndex = bti;
                ptrAnnotation->PayloadPosition = payloadPosition;
                ptrAnnotation->PayloadSizeInBytes = kernelArg->getAllocateSize();
                ptrAnnotation->LocationIndex = kernelArg->getLocationIndex();
                ptrAnnotation->LocationCount = kernelArg->getLocationCount();
                ptrAnnotation->IsEmulationArgument = kernelArg->isEmulationArgument();
                m_kernelInfo.m_pointerArgument.push_back(ptrAnnotation);
            }
            break;

        case KernelArg::ArgType::PTR_LOCAL:
        {
            auto locAnnotation = std::make_unique<iOpenCL::LocalArgumentAnnotation>();

            locAnnotation->Alignment = (DWORD)kernelArg->getAlignment();
            locAnnotation->PayloadPosition = payloadPosition;
            locAnnotation->PayloadSizeInBytes = kernelArg->getAllocateSize();
            locAnnotation->ArgumentNumber = kernelArg->getAssociatedArgNo();
            locAnnotation->LocationIndex = kernelArg->getLocationIndex();
            locAnnotation->LocationCount = kernelArg->getLocationCount();
            m_kernelInfo.m_localPointerArgument.push_back(std::move(locAnnotation));
        }
        break;

        case KernelArg::ArgType::PTR_DEVICE_QUEUE:
        {
            m_kernelInfo.m_executionEnvironment.HasDeviceEnqueue = true;
            unsigned int argNo = kernelArg->getAssociatedArgNo();
            SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(argNo);
            unsigned bti = getBTI(resInfo);
            m_kernelInfo.m_argIndexMap[argNo] = bti;

            auto ptrAnnotation = std::make_shared<iOpenCL::PointerArgumentAnnotation>();

            ptrAnnotation->HasStatefulAccess = hasStatefulAccess(bti);
            ptrAnnotation->AddressSpace = iOpenCL::KERNEL_ARGUMENT_ADDRESS_SPACE_DEVICE_QUEUE;
            ptrAnnotation->ArgumentNumber = argNo;
            ptrAnnotation->BindingTableIndex = bti;
            ptrAnnotation->IsStateless = true;
            ptrAnnotation->PayloadPosition = payloadPosition;
            ptrAnnotation->PayloadSizeInBytes = kernelArg->getAllocateSize();
            m_kernelInfo.m_pointerArgument.push_back(ptrAnnotation);
        }
        break;
        case KernelArg::ArgType::CONSTANT_REG:
        {
            uint sourceOffsetBase = 0;

            // aggregate arguments may have additional source offsets
            if (kernelArg->getStructArgOffset() != -1)
            {
                sourceOffsetBase = kernelArg->getStructArgOffset();
            }

            auto constInput = std::make_unique<iOpenCL::ConstantArgumentAnnotation>();

            DWORD sizeInBytes = kernelArg->getAllocateSize();

            constInput->Offset = sourceOffsetBase;
            constInput->PayloadPosition = payloadPosition;
            constInput->PayloadSizeInBytes = sizeInBytes;
            constInput->ArgumentNumber = kernelArg->getAssociatedArgNo();
            constInput->LocationIndex = kernelArg->getLocationIndex();
            constInput->LocationCount = kernelArg->getLocationCount();
            constInput->IsEmulationArgument = kernelArg->isEmulationArgument();
            m_kernelInfo.m_constantArgumentAnnotation.push_back(std::move(constInput));

            payloadPosition += sizeInBytes;
        }
        break;

        case KernelArg::ArgType::IMPLICIT_CONSTANT_BASE:
        {
            int argNo = kernelArg->getAssociatedArgNo();
            SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(argNo);
            unsigned bti = getBTI(resInfo);;
            m_kernelInfo.m_argIndexMap[argNo] = bti;

            auto ptrAnnotation = std::make_unique<iOpenCL::PointerInputAnnotation>();
            ptrAnnotation->HasStatefulAccess = hasStatefulAccess(bti);
            ptrAnnotation->AddressSpace = iOpenCL::KERNEL_ARGUMENT_ADDRESS_SPACE_CONSTANT;
            ptrAnnotation->BindingTableIndex = 0xffffffff;
            ptrAnnotation->IsStateless = true;
            ptrAnnotation->PayloadPosition = payloadPosition;
            ptrAnnotation->PayloadSizeInBytes = kernelArg->getAllocateSize();
            ptrAnnotation->ArgumentNumber = argNo;
            m_kernelInfo.m_pointerInput.push_back(std::move(ptrAnnotation));
        }
        break;

        case KernelArg::ArgType::IMPLICIT_GLOBAL_BASE:
        {
            int argNo = kernelArg->getAssociatedArgNo();
            SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(argNo);
            unsigned bti = getBTI(resInfo);
            m_kernelInfo.m_argIndexMap[argNo] = bti;

            auto ptrAnnotation = std::make_unique<iOpenCL::PointerInputAnnotation>();
            ptrAnnotation->HasStatefulAccess = hasStatefulAccess(bti);
            ptrAnnotation->AddressSpace = iOpenCL::KERNEL_ARGUMENT_ADDRESS_SPACE_GLOBAL;
            ptrAnnotation->BindingTableIndex = 0xffffffff;
            ptrAnnotation->IsStateless = true;
            ptrAnnotation->PayloadPosition = payloadPosition;
            ptrAnnotation->PayloadSizeInBytes = kernelArg->getAllocateSize();
            ptrAnnotation->ArgumentNumber = argNo;
            m_kernelInfo.m_pointerInput.push_back(std::move(ptrAnnotation));
        }
        break;

        case KernelArg::ArgType::IMPLICIT_PRIVATE_BASE:
        {
            int argNo = kernelArg->getAssociatedArgNo();
            SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(argNo);
            unsigned bti = getBTI(resInfo);
            m_kernelInfo.m_argIndexMap[argNo] = bti;

            auto ptrAnnotation = std::make_unique<iOpenCL::PrivateInputAnnotation>();

            ptrAnnotation->HasStatefulAccess = hasStatefulAccess(bti);
            ptrAnnotation->AddressSpace = iOpenCL::KERNEL_ARGUMENT_ADDRESS_SPACE_PRIVATE;
            ptrAnnotation->ArgumentNumber = argNo;
            // PerThreadPrivateMemorySize is defined as "Total private memory requirements for each OpenCL work-item."
            ptrAnnotation->PerThreadPrivateMemorySize = m_perWIStatelessPrivateMemSize;
            ptrAnnotation->BindingTableIndex = bti;
            ptrAnnotation->IsStateless = true;
            ptrAnnotation->PayloadPosition = payloadPosition;
            ptrAnnotation->PayloadSizeInBytes = kernelArg->getAllocateSize();
            m_kernelInfo.m_pointerInput.push_back(std::move(ptrAnnotation));
        }
        break;

        case KernelArg::ArgType::IMPLICIT_ARG_BUFFER:
        {
            constantType = kernelArg->getDataParamToken();
            IGC_ASSERT(constantType != iOpenCL::DATA_PARAMETER_TOKEN_UNKNOWN);

            auto constInput = std::make_unique<iOpenCL::ConstantInputAnnotation>();

            DWORD sizeInBytes = kernelArg->getAllocateSize();
            constInput->ConstantType = constantType;
            constInput->Offset = sizeInBytes;
            constInput->PayloadPosition = payloadPosition;
            constInput->PayloadSizeInBytes = sizeInBytes;
            constInput->ArgumentNumber = DEFAULT_ARG_NUM;
            m_kernelInfo.m_constantInputAnnotation.push_back(std::move(constInput));

            break;
        }

        case KernelArg::ArgType::IMPLICIT_NUM_GROUPS:
        case KernelArg::ArgType::IMPLICIT_GLOBAL_SIZE:
        case KernelArg::ArgType::IMPLICIT_LOCAL_SIZE:
        case KernelArg::ArgType::IMPLICIT_ENQUEUED_LOCAL_WORK_SIZE:
        case KernelArg::ArgType::IMPLICIT_STAGE_IN_GRID_ORIGIN:
        case KernelArg::ArgType::IMPLICIT_STAGE_IN_GRID_SIZE:

            constantType = kernelArg->getDataParamToken();
            IGC_ASSERT(constantType != iOpenCL::DATA_PARAMETER_TOKEN_UNKNOWN);

            for (int i = 0; i < 3; ++i)
            {
                auto constInput = std::make_unique<iOpenCL::ConstantInputAnnotation>();

                DWORD sizeInBytes = iOpenCL::DATA_PARAMETER_DATA_SIZE;

                constInput->ConstantType = constantType;
                constInput->Offset = i * sizeInBytes;
                constInput->PayloadPosition = payloadPosition;
                constInput->PayloadSizeInBytes = sizeInBytes;
                constInput->ArgumentNumber = DEFAULT_ARG_NUM;
                m_kernelInfo.m_constantInputAnnotation.push_back(std::move(constInput));

                payloadPosition += sizeInBytes;
            }

            if (type == KernelArg::ArgType::IMPLICIT_STAGE_IN_GRID_ORIGIN)
                m_kernelInfo.m_threadPayload.HasStageInGridOrigin = true;
            else if (type == KernelArg::ArgType::IMPLICIT_STAGE_IN_GRID_SIZE)
                m_kernelInfo.m_threadPayload.HasStageInGridSize = true;

            break;

        case KernelArg::ArgType::IMPLICIT_IMAGE_HEIGHT:
        case KernelArg::ArgType::IMPLICIT_IMAGE_WIDTH:
        case KernelArg::ArgType::IMPLICIT_IMAGE_DEPTH:
        case KernelArg::ArgType::IMPLICIT_IMAGE_NUM_MIP_LEVELS:
        case KernelArg::ArgType::IMPLICIT_IMAGE_CHANNEL_DATA_TYPE:
        case KernelArg::ArgType::IMPLICIT_IMAGE_CHANNEL_ORDER:
        case KernelArg::ArgType::IMPLICIT_IMAGE_SRGB_CHANNEL_ORDER:
        case KernelArg::ArgType::IMPLICIT_IMAGE_ARRAY_SIZE:
        case KernelArg::ArgType::IMPLICIT_IMAGE_NUM_SAMPLES:
        case KernelArg::ArgType::IMPLICIT_SAMPLER_ADDRESS:
        case KernelArg::ArgType::IMPLICIT_SAMPLER_NORMALIZED:
        case KernelArg::ArgType::IMPLICIT_SAMPLER_SNAP_WA:
        case KernelArg::ArgType::IMPLICIT_FLAT_IMAGE_BASEOFFSET:
        case KernelArg::ArgType::IMPLICIT_FLAT_IMAGE_HEIGHT:
        case KernelArg::ArgType::IMPLICIT_FLAT_IMAGE_WIDTH:
        case KernelArg::ArgType::IMPLICIT_FLAT_IMAGE_PITCH:
        case KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_DATA_PARAMETER_OBJECT_ID:
        case KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_DISPATCHER_SIMD_SIZE:
        case KernelArg::ArgType::IMPLICIT_BUFFER_OFFSET:
            constantType = kernelArg->getDataParamToken();
            IGC_ASSERT(constantType != iOpenCL::DATA_PARAMETER_TOKEN_UNKNOWN);
            {
                auto constInput = std::make_unique<iOpenCL::ConstantInputAnnotation>();

                constInput->ConstantType = constantType;
                constInput->Offset = 0;
                constInput->PayloadPosition = payloadPosition;
                constInput->PayloadSizeInBytes = iOpenCL::DATA_PARAMETER_DATA_SIZE;
                constInput->ArgumentNumber = kernelArg->getAssociatedArgNo();
                m_kernelInfo.m_constantInputAnnotation.push_back(std::move(constInput));
            }
            break;

        case KernelArg::ArgType::IMAGE_1D:
        case KernelArg::ArgType::BINDLESS_IMAGE_1D:
        case KernelArg::ArgType::IMAGE_1D_BUFFER:
        case KernelArg::ArgType::BINDLESS_IMAGE_1D_BUFFER:
        case KernelArg::ArgType::IMAGE_2D:
        case KernelArg::ArgType::BINDLESS_IMAGE_2D:
        case KernelArg::ArgType::IMAGE_3D:
        case KernelArg::ArgType::BINDLESS_IMAGE_3D:
        case KernelArg::ArgType::IMAGE_CUBE:
        case KernelArg::ArgType::BINDLESS_IMAGE_CUBE:
        case KernelArg::ArgType::IMAGE_CUBE_DEPTH:
        case KernelArg::ArgType::BINDLESS_IMAGE_CUBE_DEPTH:
        case KernelArg::ArgType::IMAGE_1D_ARRAY:
        case KernelArg::ArgType::BINDLESS_IMAGE_1D_ARRAY:
        case KernelArg::ArgType::IMAGE_2D_ARRAY:
        case KernelArg::ArgType::BINDLESS_IMAGE_2D_ARRAY:
        case KernelArg::ArgType::IMAGE_2D_DEPTH:
        case KernelArg::ArgType::BINDLESS_IMAGE_2D_DEPTH:
        case KernelArg::ArgType::IMAGE_2D_DEPTH_ARRAY:
        case KernelArg::ArgType::BINDLESS_IMAGE_2D_DEPTH_ARRAY:
        case KernelArg::ArgType::IMAGE_2D_MSAA:
        case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA:
        case KernelArg::ArgType::IMAGE_2D_MSAA_ARRAY:
        case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_ARRAY:
        case KernelArg::ArgType::IMAGE_2D_MSAA_DEPTH:
        case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_DEPTH:
        case KernelArg::ArgType::IMAGE_2D_MSAA_DEPTH_ARRAY:
        case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_DEPTH_ARRAY:
        case KernelArg::ArgType::IMAGE_CUBE_ARRAY:
        case KernelArg::ArgType::BINDLESS_IMAGE_CUBE_ARRAY:
        case KernelArg::ArgType::IMAGE_CUBE_DEPTH_ARRAY:
        case KernelArg::ArgType::BINDLESS_IMAGE_CUBE_DEPTH_ARRAY:
        {
            int argNo = kernelArg->getAssociatedArgNo();
            SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(argNo);
            m_kernelInfo.m_argIndexMap[argNo] = getBTI(resInfo);

            auto imageInput = std::make_unique<iOpenCL::ImageArgumentAnnotation>();

            imageInput->ArgumentNumber = argNo;
            imageInput->IsFixedBindingTableIndex = true;
            imageInput->BindingTableIndex = getBTI(resInfo);
            imageInput->ImageType = getImageTypeFromKernelArg(*kernelArg);
            IGC_ASSERT(imageInput->ImageType != iOpenCL::IMAGE_MEMORY_OBJECT_INVALID);
            imageInput->LocationIndex = kernelArg->getLocationIndex();
            imageInput->LocationCount = kernelArg->getLocationCount();
            imageInput->IsEmulationArgument = kernelArg->isEmulationArgument();

            imageInput->AccessedByFloatCoords = kernelArg->getImgAccessedFloatCoords();
            imageInput->AccessedByIntCoords = kernelArg->getImgAccessedIntCoords();
            imageInput->IsBindlessAccess = kernelArg->needsAllocation();
            imageInput->PayloadPosition = payloadPosition;

            switch (resInfo.Type)
            {
            case SOpenCLKernelInfo::SResourceInfo::RES_UAV:
                if (kernelArg->getAccessQual() == IGC::KernelArg::AccessQual::READ_ONLY)
                    imageInput->Writeable = false;
                else
                    imageInput->Writeable = true;
                break;
            case SOpenCLKernelInfo::SResourceInfo::RES_SRV:
                imageInput->Writeable = false;
                break;
            default:
                IGC_ASSERT_MESSAGE(0, "Unknown resource type");
                break;
            }
            m_kernelInfo.m_imageInputAnnotations.push_back(std::move(imageInput));

            if (kernelArg->getAccessQual() == IGC::KernelArg::AccessQual::READ_WRITE)
            {
                m_kernelInfo.m_executionEnvironment.HasReadWriteImages = true;
            }
        }
        break;

        case KernelArg::ArgType::SAMPLER:
        case KernelArg::ArgType::BINDLESS_SAMPLER:
        {
            int argNo = kernelArg->getAssociatedArgNo();
            SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(argNo);
            m_kernelInfo.m_argIndexMap[argNo] = resInfo.Index;

            auto samplerArg = std::make_unique<iOpenCL::SamplerArgumentAnnotation>();
            samplerArg->SamplerType = getSamplerTypeFromKernelArg(*kernelArg);
            samplerArg->ArgumentNumber = argNo;
            samplerArg->SamplerTableIndex = resInfo.Index;
            samplerArg->LocationIndex = kernelArg->getLocationIndex();
            samplerArg->LocationCount = kernelArg->getLocationCount();
            samplerArg->IsBindlessAccess = kernelArg->needsAllocation();
            samplerArg->IsEmulationArgument = kernelArg->isEmulationArgument();
            samplerArg->PayloadPosition = payloadPosition;

            m_kernelInfo.m_samplerArgument.push_back(std::move(samplerArg));
        }
        break;

        case KernelArg::ArgType::IMPLICIT_LOCAL_IDS:
        {
            m_kernelInfo.m_threadPayload.HasLocalIDx = true;
            m_kernelInfo.m_threadPayload.HasLocalIDy = true;
            m_kernelInfo.m_threadPayload.HasLocalIDz = true;

            ModuleMetaData* modMD = m_Context->getModuleMetaData();
            auto it = modMD->FuncMD.find(entry);
            if (it != modMD->FuncMD.end())
            {
                if (it->second.localIDPresent == true)
                    m_kernelInfo.m_threadPayload.HasLocalID = true;
            }
        }
        break;
        case KernelArg::ArgType::RT_STACK_ID:
        {
            m_kernelInfo.m_threadPayload.HasRTStackID = true;
        }
        break;

        case KernelArg::ArgType::R1:
            m_kernelInfo.m_threadPayload.UnusedPerThreadConstantPresent = true;
            break;

        case KernelArg::ArgType::IMPLICIT_SYNC_BUFFER:
        {
            int argNo = kernelArg->getAssociatedArgNo();
            SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(argNo);
            unsigned bti = getBTI(resInfo);
            m_kernelInfo.m_argIndexMap[argNo] = bti;

            auto syncBuffer = std::make_unique<iOpenCL::SyncBufferAnnotation>();

            syncBuffer->HasStatefulAccess = hasStatefulAccess(bti);
            syncBuffer->ArgumentNumber = argNo;
            syncBuffer->PayloadPosition = payloadPosition;
            syncBuffer->DataSize = kernelArg->getAllocateSize();

            m_kernelInfo.m_syncBufferAnnotation = std::move(syncBuffer);
        }
        break;

        case KernelArg::ArgType::IMPLICIT_RT_GLOBAL_BUFFER:
        {
            int argNo = kernelArg->getAssociatedArgNo();
            SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(argNo);
            unsigned bti = getBTI(resInfo);
            m_kernelInfo.m_argIndexMap[argNo] = bti;

            auto rtGlobalBuffer = std::make_unique<iOpenCL::RTGlobalBufferAnnotation>();

            rtGlobalBuffer->HasStatefulAccess = hasStatefulAccess(bti);
            rtGlobalBuffer->ArgumentNumber = argNo;
            rtGlobalBuffer->PayloadPosition = payloadPosition;
            rtGlobalBuffer->DataSize = kernelArg->getAllocateSize();

            m_kernelInfo.m_rtGlobalBufferAnnotation = std::move(rtGlobalBuffer);
        }
        break;

        case KernelArg::ArgType::IMPLICIT_PRINTF_BUFFER:
        {
            auto printfBuffer = std::make_unique<iOpenCL::PrintfBufferAnnotation>();

            printfBuffer->ArgumentNumber = kernelArg->getAssociatedArgNo();
            printfBuffer->PayloadPosition = payloadPosition;
            printfBuffer->DataSize = kernelArg->getAllocateSize();
            printfBuffer->Index = 0; // This value is not used by Runtime.

            m_kernelInfo.m_printfBufferAnnotation = std::move(printfBuffer);
        }
        break;

        case KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_DEFAULT_DEVICE_QUEUE:
        {
            m_kernelInfo.m_executionEnvironment.HasDeviceEnqueue = true;
            int argNo = kernelArg->getAssociatedArgNo();
            SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(argNo);
            unsigned bti = getBTI(resInfo);
            m_kernelInfo.m_argIndexMap[argNo] = bti;

            auto ptrAnnotation = std::make_unique<iOpenCL::PointerInputAnnotation>();

            ptrAnnotation->HasStatefulAccess = hasStatefulAccess(bti);
            ptrAnnotation->AddressSpace = iOpenCL::ADDRESS_SPACE_INTERNAL_DEFAULT_DEVICE_QUEUE;
            ptrAnnotation->BindingTableIndex = bti;
            ptrAnnotation->IsStateless = true;
            ptrAnnotation->PayloadPosition = payloadPosition;
            ptrAnnotation->PayloadSizeInBytes = kernelArg->getAllocateSize();
            ptrAnnotation->ArgumentNumber = argNo;
            m_kernelInfo.m_pointerInput.push_back(std::move(ptrAnnotation));
        }
        break;

        case KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_EVENT_POOL:
        {
            m_kernelInfo.m_executionEnvironment.HasDeviceEnqueue = true;
            int argNo = kernelArg->getAssociatedArgNo();
            SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(argNo);
            unsigned bti = getBTI(resInfo);
            m_kernelInfo.m_argIndexMap[argNo] = bti;

            auto ptrAnnotation = std::make_unique<iOpenCL::PointerInputAnnotation>();
            ptrAnnotation->HasStatefulAccess = hasStatefulAccess(bti);
            ptrAnnotation->AddressSpace = iOpenCL::ADDRESS_SPACE_INTERNAL_EVENT_POOL;
            ptrAnnotation->BindingTableIndex = bti;
            ptrAnnotation->IsStateless = true;
            ptrAnnotation->PayloadPosition = payloadPosition;
            ptrAnnotation->PayloadSizeInBytes = kernelArg->getAllocateSize();
            ptrAnnotation->ArgumentNumber = argNo;
            m_kernelInfo.m_pointerInput.push_back(std::move(ptrAnnotation));
        }
        break;

        case KernelArg::ArgType::IMPLICIT_WORK_DIM:
        case KernelArg::ArgType::IMPLICIT_VME_MB_BLOCK_TYPE:
        case KernelArg::ArgType::IMPLICIT_VME_SUBPIXEL_MODE:
        case KernelArg::ArgType::IMPLICIT_VME_SAD_ADJUST_MODE:
        case KernelArg::ArgType::IMPLICIT_VME_SEARCH_PATH_TYPE:
        case KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_MAX_WORKGROUP_SIZE:
        case KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_PARENT_EVENT:
        case KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_PREFERED_WORKGROUP_MULTIPLE:
            constantType = kernelArg->getDataParamToken();
            {
                auto constInput = std::make_unique<iOpenCL::ConstantInputAnnotation>();

                DWORD sizeInBytes = iOpenCL::DATA_PARAMETER_DATA_SIZE;

                constInput->ConstantType = constantType;
                constInput->Offset = 0;
                constInput->PayloadPosition = payloadPosition;
                constInput->PayloadSizeInBytes = sizeInBytes;
                constInput->ArgumentNumber = DEFAULT_ARG_NUM;
                m_kernelInfo.m_constantInputAnnotation.push_back(std::move(constInput));

                payloadPosition += sizeInBytes;
            }
            break;
        case KernelArg::ArgType::IMPLICIT_LOCAL_MEMORY_STATELESS_WINDOW_START_ADDRESS:
        {
            auto GASStart = std::make_unique<iOpenCL::StartGASAnnotation>();
            GASStart->Offset = payloadPosition;
            GASStart->gpuPointerSizeInBytes = kernelArg->getAllocateSize();
            m_kernelInfo.m_startGAS = std::move(GASStart);
        }
        break;
        case KernelArg::ArgType::IMPLICIT_LOCAL_MEMORY_STATELESS_WINDOW_SIZE:
        {
            auto winSizeGAS = std::make_unique<iOpenCL::WindowSizeGASAnnotation>();

            winSizeGAS->Offset = payloadPosition;
            m_kernelInfo.m_WindowSizeGAS = std::move(winSizeGAS);
        }
        break;
        case KernelArg::ArgType::IMPLICIT_PRIVATE_MEMORY_STATELESS_SIZE:
        {
            auto privateMemSize = std::make_unique<iOpenCL::PrivateMemSizeAnnotation>();

            privateMemSize->Offset = payloadPosition;
            m_kernelInfo.m_PrivateMemSize = std::move(privateMemSize);
        }
        break;
        default:
            // Do nothing
            break;
        }


        // DATA_PARAMETER_BUFFER_STATEFUL
        //   ( SPatchDataParameterBuffer for this token only uses one field: ArgumentNumber )
        //   Used to indicate that all memory references via a gobal/constant ptr argument are
        //   converted to stateful (by StatelessToStateful optimization). Thus, the ptr itself
        //   is no longer referenced at all.
        //
        if (IGC_IS_FLAG_ENABLED(EnableStatelessToStateful) &&
            IGC_IS_FLAG_ENABLED(EnableStatefulToken) &&
            m_DriverInfo->SupportStatefulToken() &&
            !m_Context->getModuleMetaData()->compOpt.GreaterThan4GBBufferRequired &&
            arg &&
            ((type == KernelArg::ArgType::PTR_GLOBAL &&
            (arg->use_empty() || !GetHasGlobalStatelessAccess())) ||
                (type == KernelArg::ArgType::PTR_CONSTANT &&
                (arg->use_empty() || !GetHasConstantStatelessAccess()))))
        {
            auto constInput = std::make_unique<iOpenCL::ConstantInputAnnotation>();

            constInput->ConstantType = iOpenCL::DATA_PARAMETER_BUFFER_STATEFUL;
            constInput->Offset = 0;
            constInput->PayloadPosition = payloadPosition;
            constInput->PayloadSizeInBytes = iOpenCL::DATA_PARAMETER_DATA_SIZE;
            constInput->ArgumentNumber = kernelArg->getAssociatedArgNo(); // used only for this token.
            m_kernelInfo.m_constantInputAnnotation.push_back(std::move(constInput));
        }
    }

   iOpenCL::IMAGE_MEMORY_OBJECT_TYPE COpenCLKernel::getImageTypeFromKernelArg(const KernelArg& kernelArg)
   {
       switch(kernelArg.getArgType()) {
           case KernelArg::ArgType::IMAGE_1D:
           case KernelArg::ArgType::BINDLESS_IMAGE_1D:
               return iOpenCL::IMAGE_MEMORY_OBJECT_1D;

           case KernelArg::ArgType::IMAGE_1D_BUFFER:
           case KernelArg::ArgType::BINDLESS_IMAGE_1D_BUFFER:
               return iOpenCL::IMAGE_MEMORY_OBJECT_BUFFER;

           case KernelArg::ArgType::IMAGE_2D:
           case KernelArg::ArgType::BINDLESS_IMAGE_2D:
               if (getExtensionInfo(kernelArg.getAssociatedArgNo()) == ResourceExtensionTypeEnum::MediaResourceType)
                   return iOpenCL::IMAGE_MEMORY_OBJECT_2D_MEDIA;
               else if (getExtensionInfo(kernelArg.getAssociatedArgNo()) == ResourceExtensionTypeEnum::MediaResourceBlockType)
                   return iOpenCL::IMAGE_MEMORY_OBJECT_2D_MEDIA_BLOCK;
               return iOpenCL::IMAGE_MEMORY_OBJECT_2D;

           case KernelArg::ArgType::IMAGE_3D:
           case KernelArg::ArgType::BINDLESS_IMAGE_3D:
               return iOpenCL::IMAGE_MEMORY_OBJECT_3D;

           case KernelArg::ArgType::IMAGE_CUBE:
           case KernelArg::ArgType::BINDLESS_IMAGE_CUBE:
               return iOpenCL::IMAGE_MEMORY_OBJECT_CUBE;

           case KernelArg::ArgType::IMAGE_CUBE_DEPTH:
           case KernelArg::ArgType::BINDLESS_IMAGE_CUBE_DEPTH:
               // Use regular cube texture for depth:
               return iOpenCL::IMAGE_MEMORY_OBJECT_CUBE;

           case KernelArg::ArgType::IMAGE_1D_ARRAY:
           case KernelArg::ArgType::BINDLESS_IMAGE_1D_ARRAY:
               return iOpenCL::IMAGE_MEMORY_OBJECT_1D_ARRAY;

           case KernelArg::ArgType::IMAGE_2D_ARRAY:
           case KernelArg::ArgType::BINDLESS_IMAGE_2D_ARRAY:
               return iOpenCL::IMAGE_MEMORY_OBJECT_2D_ARRAY;


           case KernelArg::ArgType::IMAGE_2D_DEPTH:
           case KernelArg::ArgType::BINDLESS_IMAGE_2D_DEPTH:
               return iOpenCL::IMAGE_MEMORY_OBJECT_2D_DEPTH;

           case KernelArg::ArgType::IMAGE_2D_DEPTH_ARRAY:
           case KernelArg::ArgType::BINDLESS_IMAGE_2D_DEPTH_ARRAY:
               return iOpenCL::IMAGE_MEMORY_OBJECT_2D_ARRAY_DEPTH;

           case KernelArg::ArgType::IMAGE_2D_MSAA:
           case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA:
               return iOpenCL::IMAGE_MEMORY_OBJECT_2D_MSAA;

           case KernelArg::ArgType::IMAGE_2D_MSAA_ARRAY:
           case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_ARRAY:
               return iOpenCL::IMAGE_MEMORY_OBJECT_2D_ARRAY_MSAA;

           case KernelArg::ArgType::IMAGE_2D_MSAA_DEPTH:
           case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_DEPTH:
               return iOpenCL::IMAGE_MEMORY_OBJECT_2D_MSAA_DEPTH;

           case KernelArg::ArgType::IMAGE_2D_MSAA_DEPTH_ARRAY:
           case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_DEPTH_ARRAY:
               return iOpenCL::IMAGE_MEMORY_OBJECT_2D_ARRAY_MSAA_DEPTH;

           case KernelArg::ArgType::IMAGE_CUBE_ARRAY:
           case KernelArg::ArgType::BINDLESS_IMAGE_CUBE_ARRAY:
               return iOpenCL::IMAGE_MEMORY_OBJECT_CUBE_ARRAY;

           case KernelArg::ArgType::IMAGE_CUBE_DEPTH_ARRAY:
           case KernelArg::ArgType::BINDLESS_IMAGE_CUBE_DEPTH_ARRAY:
               // Use regular cube texture array for depth
               return iOpenCL::IMAGE_MEMORY_OBJECT_CUBE_ARRAY;

           default:
               break;
       }
       return iOpenCL::IMAGE_MEMORY_OBJECT_INVALID;
   }

    iOpenCL::SAMPLER_OBJECT_TYPE COpenCLKernel::getSamplerTypeFromKernelArg(const KernelArg& kernelArg)
    {
        IGC_ASSERT(kernelArg.getArgType() == KernelArg::ArgType::SAMPLER ||
            kernelArg.getArgType() == KernelArg::ArgType::BINDLESS_SAMPLER);
        switch (getExtensionInfo(kernelArg.getAssociatedArgNo())) {
        case ResourceExtensionTypeEnum::MediaSamplerType:
            return iOpenCL::SAMPLER_OBJECT_VME;

        case ResourceExtensionTypeEnum::MediaSamplerTypeConvolve:
            return iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_2DCONVOLVE;

        case ResourceExtensionTypeEnum::MediaSamplerTypeErode:
            return iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_ERODE;

        case ResourceExtensionTypeEnum::MediaSamplerTypeDilate:
            return iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_DILATE;

        case ResourceExtensionTypeEnum::MediaSamplerTypeMinMaxFilter:
            return iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_MINMAXFILTER;

        case ResourceExtensionTypeEnum::MediaSamplerTypeMinMax:
            return iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_MINMAX;

        case ResourceExtensionTypeEnum::MediaSamplerTypeCentroid:
            return iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_CENTROID;

        case ResourceExtensionTypeEnum::MediaSamplerTypeBoolCentroid:
            return iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_BOOL_CENTROID;

        case ResourceExtensionTypeEnum::MediaSamplerTypeBoolSum:
            return iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_BOOL_SUM;

        default:
            return iOpenCL::SAMPLER_OBJECT_TEXTURE;
        }
    }

    void COpenCLKernel::ParseShaderSpecificOpcode(llvm::Instruction* inst)
    {
        auto setStatelessAccess = [&](unsigned AS) {
            if (AS == ADDRESS_SPACE_GLOBAL ||
                AS == ADDRESS_SPACE_GENERIC ||
                AS == ADDRESS_SPACE_GLOBAL_OR_PRIVATE)
            {
                SetHasGlobalStatelessAccess();
            }

            if (AS == ADDRESS_SPACE_CONSTANT)
            {
                SetHasConstantStatelessAccess();
            }
        };

        // Currently we see data corruption when we have IEEE macros and midthread preemption enabled.
        // Adding a temporary work around to disable mid thread preemption when we see IEEE Macros.
        switch (inst->getOpcode())
        {
        case Instruction::FDiv:
            if (inst->getType()->isDoubleTy())
            {
                SetDisableMidthreadPreemption();
            }
            break;
        case Instruction::Call:
            if (inst->getType()->isDoubleTy())
            {
                if (GetOpCode(inst) == llvm_sqrt)
                {
                    SetDisableMidthreadPreemption();
                }
            }
            break;
        case Instruction::Load:
        {
            unsigned AS = cast<LoadInst>(inst)->getPointerAddressSpace();
            setStatelessAccess(AS);
            break;
        }
        case Instruction::Store:
        {
            unsigned AS = cast<StoreInst>(inst)->getPointerAddressSpace();
            setStatelessAccess(AS);
            break;
        }
        default:
            break;
        }

        if (CallInst * CallI = dyn_cast<CallInst>(inst))
        {
            bool mayHasMemoryAccess = true;  // for checking stateless access
            if (GenIntrinsicInst * GII = dyn_cast<GenIntrinsicInst>(CallI))
            {
                GenISAIntrinsic::ID id = GII->getIntrinsicID();
                switch (id)
                {
                default:
                    break;
                case GenISAIntrinsic::GenISA_dpas:
                case GenISAIntrinsic::GenISA_sub_group_dpas:
                    SetHasDPAS();
                    break;
                case GenISAIntrinsic::GenISA_ptr_to_pair:
                case GenISAIntrinsic::GenISA_pair_to_ptr:
                    mayHasMemoryAccess = false;
                    break;
                } // End of switch
            }

            if (mayHasMemoryAccess)
            {
                // Checking stateless access info
                if (!isa<IntrinsicInst>(CallI) && !isa<GenIntrinsicInst>(CallI)) {
                    // function/subroutine call. Give up
                    SetHasConstantStatelessAccess();
                    SetHasGlobalStatelessAccess();
                }
                else
                {
                    for (int i = 0, e = (int)IGCLLVM::getNumArgOperands(CallI); i < e; ++i)
                    {
                        Value* arg = CallI->getArgOperand(i);
                        PointerType* PTy = dyn_cast<PointerType>(arg->getType());
                        if (!PTy)
                            continue;
                        unsigned AS = PTy->getAddressSpace();
                        setStatelessAccess(AS);
                    }
                }
            }
        }
    }

    void COpenCLKernel::AllocatePayload()
    {
        IGC_ASSERT(m_Context);

        bool loadThreadPayload = false;

        loadThreadPayload = m_Platform->supportLoadThreadPayloadForCompute();

        // SKL defaults to indirect thread payload storage.
        // BDW needs CURBE payload. Spec says:
        // "CURBE should be used for the payload when using indirect dispatch rather than indirect payload".
        m_kernelInfo.m_threadPayload.CompiledForIndirectPayloadStorage = true;
        if (IGC_IS_FLAG_ENABLED(DisableGPGPUIndirectPayload) ||
            m_Context->platform.getWATable().WaDisableIndirectDataForIndirectDispatch)
        {
            m_kernelInfo.m_threadPayload.CompiledForIndirectPayloadStorage = false;
        }
        if (loadThreadPayload)
        {
            m_kernelInfo.m_threadPayload.CompiledForIndirectPayloadStorage = true;
        }
        m_kernelInfo.m_threadPayload.HasFlattenedLocalID = false;
        m_kernelInfo.m_threadPayload.HasLocalIDx = false;
        m_kernelInfo.m_threadPayload.HasLocalIDy = false;
        m_kernelInfo.m_threadPayload.HasLocalIDz = false;
        m_kernelInfo.m_threadPayload.HasGlobalIDOffset = false;
        m_kernelInfo.m_threadPayload.HasGroupID = false;
        m_kernelInfo.m_threadPayload.HasLocalID = false;
        m_kernelInfo.m_threadPayload.UnusedPerThreadConstantPresent = false;
        m_kernelInfo.m_printfBufferAnnotation = nullptr;
        m_kernelInfo.m_syncBufferAnnotation = nullptr;
        m_kernelInfo.m_rtGlobalBufferAnnotation = nullptr;
        m_kernelInfo.m_threadPayload.HasStageInGridOrigin = false;
        m_kernelInfo.m_threadPayload.HasStageInGridSize = false;
        m_kernelInfo.m_threadPayload.HasRTStackID = false;

        if (m_enableHWGenerateLID)
        {
            m_kernelInfo.m_threadPayload.generateLocalID = true;
            m_kernelInfo.m_threadPayload.emitLocalMask = m_emitMask;
            m_kernelInfo.m_threadPayload.walkOrder = static_cast<unsigned int>(m_walkOrder);
            m_kernelInfo.m_threadPayload.tileY = (m_ThreadIDLayout == ThreadIDLayout::TileY);
        }

        // Set the amount of the private memory used by the kernel
        // Set only if the private memory metadata actually exists and we don't use
        // scratch space for private memory.
        bool noScratchSpacePrivMem = !m_Context->getModuleMetaData()->compOpt.UseScratchSpacePrivateMemory;
        if (noScratchSpacePrivMem)
        {
            auto StackMemIter = m_Context->getModuleMetaData()->PrivateMemoryPerFG.find(entry);
            if (StackMemIter != m_Context->getModuleMetaData()->PrivateMemoryPerFG.end())
            {
                m_perWIStatelessPrivateMemSize = StackMemIter->second;
            }
        }


        m_ConstantBufferLength = 0;
        m_NOSBufferSize = 0;

        uint offset = 0;

        uint constantBufferStart = 0;
        bool constantBufferStartSet = false;

        uint prevOffset = 0;
        bool nosBufferAllocated = false;

        KernelArgsOrder::InputType layout =
            m_kernelInfo.m_threadPayload.CompiledForIndirectPayloadStorage ?
            KernelArgsOrder::InputType::INDIRECT :
            KernelArgsOrder::InputType::CURBE;

        KernelArgs kernelArgs(*entry, m_DL, m_pMdUtils, m_ModuleMetadata, getGRFSize(), layout);

        if (layout == KernelArgsOrder::InputType::INDIRECT && !loadThreadPayload)
        {
            kernelArgs.checkForZeroPerThreadData();
        }

        const bool useInlineData = passNOSInlineData();
        const uint inlineDataSize = m_Platform->getInlineDataSize();
        bool inlineDataProcessed = false;
        uint offsetCorrection = 0;

        // keep track of the pointer arguments' addrspace and access_type for setting the correct
        // attributes to their corresponding bindless offset arguments
        PtrArgsAttrMapType ptrArgsAttrMap;
        for (KernelArgs::const_iterator i = kernelArgs.begin(), e = kernelArgs.end(); i != e; ++i)
        {
            KernelArg arg = *i;
            prevOffset = offset;

            // skip unused arguments
            bool IsUnusedArg = (arg.getArgType() == KernelArg::ArgType::IMPLICIT_BUFFER_OFFSET) &&
                arg.getArg()->use_empty();

            // Runtime Values should not be processed any further. No annotations shall be created for them.
            // Only added to KernelArgs to enforce correct allocation order.
            bool isRuntimeValue = (arg.getArgType() == KernelArg::ArgType::RUNTIME_VALUE);

            if (!constantBufferStartSet && arg.isConstantBuf())
            {
                constantBufferStart = offset;
                constantBufferStartSet = true;
            }

            if (!nosBufferAllocated && isRuntimeValue) {
                IGC_ASSERT_MESSAGE(arg.isConstantBuf(), "RuntimeValues must be marked as isConstantBuf");
                AllocateNOSConstants(offset);
                nosBufferAllocated = true;
            }

            // Local IDs are non-uniform and may have two instances in SIMD32 mode
            int numAllocInstances = arg.getArgType() == KernelArg::ArgType::IMPLICIT_LOCAL_IDS ? m_numberInstance : 1;

            if (arg.getArgType() == KernelArg::ArgType::RT_STACK_ID) {
              numAllocInstances = m_numberInstance;
            }

            auto allocSize = arg.getAllocateSize();

            if (!IsUnusedArg && !isRuntimeValue)
            {
                if (arg.needsAllocation())
                {
                    // Align on the desired alignment for this argument
                    auto alignment = arg.getAlignment();

                    // FIXME: move alignment checks to implicit arg creation
                    if ((arg.getArgType() == KernelArg::ArgType::IMPLICIT_LOCAL_IDS ||
                         arg.getArgType() == KernelArg::ArgType::RT_STACK_ID) &&
                        m_Platform->getGRFSize() == 64)
                    {
                        alignment = 64;
                        // generate a single SIMD32 variable in this case
                        if (m_dispatchSize == SIMDMode::SIMD16 && m_Platform->getGRFSize() == 64)
                        {
                            allocSize = 64;
                        }
                        else
                        {
                            allocSize = PVCLSCEnabled() ? 64 : 32;
                        }
                    }

                    offset = iSTD::Align(offset, alignment);

                    // Arguments larger than a GRF must be at least GRF-aligned.
                    // Arguments smaller than a GRF may not cross GRF boundaries.
                    // This means that arguments that cross a GRF boundary
                    // must be GRF aligned.
                    // Note that this is done AFTER we align on the base alignment,
                    // because of edge cases where aligning on the base alignment
                    // is what causes the "overflow".
                    unsigned int startGRF = offset / getGRFSize();
                    unsigned int endGRF = (offset + allocSize - 1) / getGRFSize();
                    if (startGRF != endGRF)
                    {
                        offset = iSTD::Align(offset, getGRFSize());
                    }

                    // offsetCorrection should be set only when we are loading payload in kenrel prolog
                    if (loadThreadPayload)
                    {
                        bool isFirstCrossThreadArgument = constantBufferStartSet && prevOffset == constantBufferStart;
                        // if we don't use inline data and first argument does not start in first avaliable register
                        // because of its alignment (which can be greater than GRF size), we correct the offset in payload,
                        // so that it can be loaded properly in prolog, we want it to be on 0 offset in payload
                        //
                        // payload_position = offset - constant_buffer_start - correction
                        //
                        // examples:
                        //  alignment   offset   constant_buffer_start  correction  payload_position
                        //   128         128      32                     96          0
                        //   8           32       32                     0           0
                        if (!useInlineData && isFirstCrossThreadArgument)
                        {
                            offsetCorrection = offset - constantBufferStart;
                        }

                        if (useInlineData && !inlineDataProcessed &&
                            arg.getArgType() != KernelArg::ArgType::IMPLICIT_LOCAL_IDS &&
                            arg.getArgType() != KernelArg::ArgType::RT_STACK_ID &&
                            arg.getArgType() != KernelArg::ArgType::IMPLICIT_R0)
                        {
                            // Calc if we can fit this arg in inlinedata:
                            // We check if arg exceeds inline data boundaries,
                            // if it does, we align it to next GRF.
                            if (offset + allocSize - constantBufferStart > inlineDataSize)
                            {
                                inlineDataProcessed = true;
                                if (getGRFSize() > inlineDataSize)
                                {
                                    // If inline data is used and a plaftorm has 64B GRFs,
                                    // we must correct the offset of cross-thread arguments
                                    // which are not loaded in inline data
                                    // the reason behind this is that inline data has only 32B,
                                    // so the position of next arg needs to be aligned to next GRF,
                                    // because the input arguments are loaded with alignment of GRF
                                    offset = iSTD::Align(offset, getGRFSize());
                                }

                                // numAllocInstances can be greater than 1, only when:
                                // artype == IMPLICIT_LOCAL_IDS
                                // or argtype == RT_STACK_ID,
                                // so there is no need to handle it here

                                // current arg is first to be loaded (it does not come in inlinedata)
                                // so we want it to be at 32B offset in payload annotations
                                // (first 32B are for inline data)
                                offsetCorrection = offset - inlineDataSize - constantBufferStart;
                            }
                        }
                    }

                    // And now actually tell vISA we need this space.
                    // (Except for r0, which is a predefined variable, and should never be allocated as input!)
                    const llvm::Argument* A = arg.getArg();
                    if (A != nullptr && arg.getArgType() != KernelArg::ArgType::IMPLICIT_R0)
                    {
                        CVariable* var = GetSymbol(const_cast<Argument*>(A));
                        for (int i = 0; i < numAllocInstances; ++i)
                        {
                            uint totalOffset = offset + (allocSize * i);
                            if ((totalOffset / getGRFSize()) >= m_Context->getNumGRFPerThread())
                            {
                                m_Context->EmitError("Kernel inputs exceed total register size!", A);
                                return;
                            }
                            AllocateInput(var, totalOffset, i);
                        }
                    }
                    // or else we would just need to increase an offset
                }

                const uint offsetInPayload = offset - constantBufferStart - offsetCorrection;

                // Create annotations for the kernel argument
                // If an arg is unused, don't generate patch token for it.
                CreateAnnotations(&arg, offsetInPayload);

                if (m_Context->enableZEBinary()) {
                    // FIXME: once we transit to zebin completely, we don't need to do
                    // CreateAnnotations above. Only CreateZEPayloadArguments is required
                    bool Res = CreateZEPayloadArguments(&arg, offsetInPayload, ptrArgsAttrMap);
                    IGC_ASSERT_MESSAGE(Res, "ZEBin: unsupported KernelArg Type");
                    (void) Res;
                }

                if (arg.needsAllocation())
                {
                    for (int i = 0; i < numAllocInstances; ++i)
                    {
                        offset += allocSize;
                    }
                    // FIXME: Should we allocate R0 to be 64 byte for PVC?
                    if (arg.getArgType() == KernelArg::ArgType::IMPLICIT_R0 && m_Platform->getGRFSize() == 64)
                    {
                        offset += 32;
                    }
                }
            }

            if (arg.isConstantBuf())
            {
                m_ConstantBufferLength += offset - prevOffset;
            }
        }

        // Disable EU Fusion.
        if (IGC_IS_FLAG_ENABLED(DisableEuFusion) ||
            m_Context->m_InternalOptions.DisableEUFusion ||
            m_Context->getModuleMetaData()->compOpt.DisableEUFusion)
        {
            m_kernelInfo.m_executionEnvironment.RequireDisableEUFusion = true;
        }

        // ToDo: we should avoid passing all three dimensions of local id
        if (m_kernelInfo.m_threadPayload.HasLocalIDx ||
            m_kernelInfo.m_threadPayload.HasLocalIDy ||
            m_kernelInfo.m_threadPayload.HasLocalIDz)
        {
            if (loadThreadPayload)
            {
                uint perThreadInputSize = SIZE_WORD * 3 * (m_dispatchSize == SIMDMode::SIMD32 ? 32 : 16);
                if (m_dispatchSize == SIMDMode::SIMD16 && getGRFSize() == 64)
                {
                    perThreadInputSize *= 2;
                }
                encoder.GetVISAKernel()->AddKernelAttribute("PerThreadInputSize", sizeof(uint16_t), &perThreadInputSize);
            }
        }

        m_kernelInfo.m_threadPayload.OffsetToSkipPerThreadDataLoad = 0;
        m_kernelInfo.m_threadPayload.OffsetToSkipSetFFIDGP = 0;

        m_ConstantBufferLength = iSTD::Align(m_ConstantBufferLength, getGRFSize());

        // FIXME: skip this function when EnableZEBinary
        CreateInlineSamplerAnnotations();
        if (m_Context->enableZEBinary())
        {
            CreateZEInlineSamplerAnnotations();
            // Currently we don't support inline vme sampler in zebin.
            // assertion tests if we force to EnableZEBinary but encounter inline vme sampler.
            IGC_ASSERT_MESSAGE(!m_kernelInfo.m_HasInlineVmeSamplers, "ZEBin: Inline vme sampler unsupported");
        }

        // Handle kernel reflection for patch-token format
        if (!m_Context->enableZEBinary()) {
            CreateKernelArgInfo();
            CreateKernelAttributeInfo();
        }

        // Create annotations for printf string.
        CreatePrintfStringAnnotations();
    }

    bool COpenCLKernel::passNOSInlineData()
    {
        if (IGC_GET_FLAG_VALUE(EnablePassInlineData) == -1) {
            return false;
        }
        const bool forceEnablePassInlineData = (IGC_GET_FLAG_VALUE(EnablePassInlineData) == 1);
        bool passInlineData = false;
        const bool loadThreadPayload = m_Platform->supportLoadThreadPayloadForCompute();
        const bool inlineDataSupportEnabled =
            (m_Platform->supportInlineDataOCL() &&
            (m_DriverInfo->UseInlineData() || forceEnablePassInlineData));
        if (loadThreadPayload &&
            inlineDataSupportEnabled)
        {
            passInlineData = true;
            // FIXME: vISA assumes inline data size is 1 GRF, but it's 8 dword in HW.
            // The generated cross-thread-load payload would be incorrect when inline data is enabled on
            // platforms those GRF size are not 8 dword.
            // Passed the value assumed by vISA for error detection at runtime side.
            // vISA should be updated to use 8 dword.
            m_kernelInfo.m_threadPayload.PassInlineDataSize = getGRFSize();
        }
        return passInlineData;
    }

    bool COpenCLKernel::loadThreadPayload()
    {
        return true;
    }

    unsigned int COpenCLKernel::GetGlobalMappingValue(llvm::Value* c)
    {
        unsigned int val = 0;
        auto localIter = m_localOffsetsMap.find(c);
        if (localIter != m_localOffsetsMap.end())
        {
            val = localIter->second;
        }
        else
        {
            IGC_ASSERT_MESSAGE(0, "Trying to access a GlobalVariable not in locals map");
        }
        return val;
    }

    CVariable* COpenCLKernel::GetGlobalMapping(llvm::Value* c)
    {
        VISA_Type type = GetType(c->getType());
        unsigned int val = GetGlobalMappingValue(c);
        return ImmToVariable(val, type);
    }

    unsigned int COpenCLKernel::getSumFixedTGSMSizes(Function* F)
    {
        // Find whether we have size information for this kernel.
        // If not, then the total TGSM is 0, otherwise pull it from the MD
        ModuleMetaData* modMD = m_Context->getModuleMetaData();
        auto funcMD = modMD->FuncMD.find(F);
        if (funcMD == modMD->FuncMD.end())
        {
            return 0;
        }
        return funcMD->second.localSize;
    }

    void COpenCLKernel::FillZEKernelArgInfo()
    {
        auto funcMDIt = m_Context->getModuleMetaData()->FuncMD.find(entry);
        if (funcMDIt == m_Context->getModuleMetaData()->FuncMD.end())
            return;

        FunctionMetaData& funcMD = (*funcMDIt).second;
        uint count = funcMD.m_OpenCLArgAccessQualifiers.size();
        for (uint i = 0; i < count; ++i)
        {
            zebin::zeInfoArgInfo& argInfo = m_kernelInfo.m_zeKernelArgsInfo.emplace_back();
            argInfo.index = i;
            // argument name is not guaranteed to be present if -cl-kernel-arg-info is not passed in.
            if (funcMD.m_OpenCLArgNames.size() > i)
                argInfo.name = funcMD.m_OpenCLArgNames[i];
            argInfo.address_qualifier = getKernelArgAddressQualifier(funcMD, i);
            argInfo.access_qualifier = getKernelArgAccessQualifier(funcMD, i);
            argInfo.type_name = getKernelArgTypeName(funcMD, i);
            argInfo.type_qualifiers = getKernelArgTypeQualifier(funcMD, i);
        }
    }

    void COpenCLKernel::FillZEUserAttributes(IGC::IGCMD::FunctionInfoMetaDataHandle& funcInfoMD)
    {
        // intel_reqd_sub_group_size
        SubGroupSizeMetaDataHandle subGroupSize = funcInfoMD->getSubGroupSize();
        if (subGroupSize->hasValue())
        {
            m_kernelInfo.m_zeUserAttributes.intel_reqd_sub_group_size = subGroupSize->getSIMD_size();
        }

        // intel_reqd_workgroup_walk_order
        auto it = m_Context->getModuleMetaData()->FuncMD.find(entry);
        if (it != m_Context->getModuleMetaData()->FuncMD.end())
        {
            WorkGroupWalkOrderMD workgroupWalkOrder = it->second.workGroupWalkOrder;
            if (workgroupWalkOrder.dim0 || workgroupWalkOrder.dim1 || workgroupWalkOrder.dim2)
            {
                m_kernelInfo.m_zeUserAttributes.intel_reqd_workgroup_walk_order.push_back(workgroupWalkOrder.dim0);
                m_kernelInfo.m_zeUserAttributes.intel_reqd_workgroup_walk_order.push_back(workgroupWalkOrder.dim1);
                m_kernelInfo.m_zeUserAttributes.intel_reqd_workgroup_walk_order.push_back(workgroupWalkOrder.dim2);
            }
        }

        // reqd_work_group_size
        ThreadGroupSizeMetaDataHandle threadGroupSize = funcInfoMD->getThreadGroupSize();
        if (threadGroupSize->hasValue())
        {
            m_kernelInfo.m_zeUserAttributes.reqd_work_group_size.push_back(threadGroupSize->getXDim());
            m_kernelInfo.m_zeUserAttributes.reqd_work_group_size.push_back(threadGroupSize->getYDim());
            m_kernelInfo.m_zeUserAttributes.reqd_work_group_size.push_back(threadGroupSize->getZDim());
        }

        // vec_type_hint
        VectorTypeHintMetaDataHandle vecTypeHintInfo = funcInfoMD->getOpenCLVectorTypeHint();
        if (vecTypeHintInfo->hasValue())
        {
            m_kernelInfo.m_zeUserAttributes.vec_type_hint = getVecTypeHintTypeString(vecTypeHintInfo);
        }

        // work_group_size_hint
        ThreadGroupSizeMetaDataHandle threadGroupSizeHint = funcInfoMD->getThreadGroupSizeHint();
        if (threadGroupSizeHint->hasValue())
        {
            m_kernelInfo.m_zeUserAttributes.work_group_size_hint.push_back(threadGroupSizeHint->getXDim());
            m_kernelInfo.m_zeUserAttributes.work_group_size_hint.push_back(threadGroupSizeHint->getYDim());
            m_kernelInfo.m_zeUserAttributes.work_group_size_hint.push_back(threadGroupSizeHint->getZDim());
        }

        // function attribute added at PoisonFP64KernelsPass for describing the invalid kernel reason "uses-fp64-math"
        const std::string invalidAttributeName = "invalid_kernel(\"uses-fp64-math\")";
        std::string llvmFnAttrStr = entry->getAttributes().getAsString(AttributeList::FunctionIndex);
        if (llvmFnAttrStr.find(invalidAttributeName) != std::string::npos)
        {
            m_kernelInfo.m_zeUserAttributes.invalid_kernel = "uses-fp64-math";
        }
    }

    void COpenCLKernel::FillKernel(SIMDMode simdMode)
    {
        auto pOutput = ProgramOutput();
        if (simdMode == SIMDMode::SIMD32)
            m_kernelInfo.m_kernelProgram.simd32 = *pOutput;
        else if (simdMode == SIMDMode::SIMD16)
            m_kernelInfo.m_kernelProgram.simd16 = *pOutput;
        else if (simdMode == SIMDMode::SIMD8)
            m_kernelInfo.m_kernelProgram.simd8 = *pOutput;

        m_Context->SetSIMDInfo(SIMD_SELECTED, simdMode, ShaderDispatchMode::NOT_APPLICABLE);

        m_kernelInfo.m_executionEnvironment.CompiledSIMDSize = numLanes(simdMode);
        m_kernelInfo.m_executionEnvironment.SIMDInfo = m_Context->GetSIMDInfo();

        m_kernelInfo.m_executionEnvironment.PerThreadScratchSpace = pOutput->getScratchSpaceUsageInSlot0();
        m_kernelInfo.m_executionEnvironment.PerThreadScratchSpaceSlot1 = pOutput->getScratchSpaceUsageInSlot1();
        m_kernelInfo.m_executionEnvironment.PerThreadPrivateOnStatelessSize = m_perWIStatelessPrivateMemSize;
        m_kernelInfo.m_kernelProgram.NOSBufferSize = m_NOSBufferSize / getGRFSize(); // in 256 bits
        m_kernelInfo.m_kernelProgram.ConstantBufferLength = m_ConstantBufferLength / getGRFSize(); // in 256 bits
        m_kernelInfo.m_kernelProgram.MaxNumberOfThreads = m_Platform->getMaxGPGPUShaderThreads() / GetShaderThreadUsageRate();

        m_kernelInfo.m_executionEnvironment.SumFixedTGSMSizes = getSumFixedTGSMSizes(entry);

        // TODO: need to change misleading HasBarriers to NumberofBarriers
        m_kernelInfo.m_executionEnvironment.HasBarriers = this->GetBarrierNumber();
        m_kernelInfo.m_executionEnvironment.HasSample = this->GetHasSample();
        m_kernelInfo.m_executionEnvironment.DisableMidThreadPreemption = GetDisableMidThreadPreemption();
        m_kernelInfo.m_executionEnvironment.SubgroupIndependentForwardProgressRequired =
            m_Context->getModuleMetaData()->compOpt.SubgroupIndependentForwardProgressRequired;
        m_kernelInfo.m_executionEnvironment.CompiledForGreaterThan4GBBuffers =
            m_Context->getModuleMetaData()->compOpt.GreaterThan4GBBufferRequired;
        IGC_ASSERT(gatherMap.size() == 0);
        m_kernelInfo.m_kernelProgram.gatherMapSize = 0;
        m_kernelInfo.m_kernelProgram.bindingTableEntryCount = 0;

        m_kernelInfo.m_executionEnvironment.HasDeviceEnqueue = false;
        m_kernelInfo.m_executionEnvironment.IsSingleProgramFlow = false;
        //m_kernelInfo.m_executionEnvironment.PerSIMDLanePrivateMemorySize = m_perWIStatelessPrivateMemSize;
        m_kernelInfo.m_executionEnvironment.HasFixedWorkGroupSize = false;
        m_kernelInfo.m_kernelName = entry->getName().str();
        m_kernelInfo.m_ShaderHashCode = m_Context->hash.getAsmHash();

        FunctionInfoMetaDataHandle funcInfoMD = m_pMdUtils->getFunctionsInfoItem(entry);

        ThreadGroupSizeMetaDataHandle threadGroupSize = funcInfoMD->getThreadGroupSize();
        if (threadGroupSize->hasValue())
        {
            m_kernelInfo.m_executionEnvironment.HasFixedWorkGroupSize = true;
            m_kernelInfo.m_executionEnvironment.FixedWorkgroupSize[0] = threadGroupSize->getXDim();
            m_kernelInfo.m_executionEnvironment.FixedWorkgroupSize[1] = threadGroupSize->getYDim();
            m_kernelInfo.m_executionEnvironment.FixedWorkgroupSize[2] = threadGroupSize->getZDim();
        }

        SubGroupSizeMetaDataHandle subGroupSize = funcInfoMD->getSubGroupSize();
        if (subGroupSize->hasValue())
        {
            m_kernelInfo.m_executionEnvironment.CompiledSIMDSize = subGroupSize->getSIMD_size();
        }

        auto& FuncMap = m_Context->getModuleMetaData()->FuncMD;
        auto FuncIter = FuncMap.find(entry);
        if (FuncIter != FuncMap.end())
        {
            IGC::FunctionMetaData funcMD = FuncIter->second;
            WorkGroupWalkOrderMD workGroupWalkOrder = funcMD.workGroupWalkOrder;

            if (workGroupWalkOrder.dim0 || workGroupWalkOrder.dim1 || workGroupWalkOrder.dim2)
            {
                m_kernelInfo.m_executionEnvironment.WorkgroupWalkOrder[0] = workGroupWalkOrder.dim0;
                m_kernelInfo.m_executionEnvironment.WorkgroupWalkOrder[1] = workGroupWalkOrder.dim1;
                m_kernelInfo.m_executionEnvironment.WorkgroupWalkOrder[2] = workGroupWalkOrder.dim2;
            }

            m_kernelInfo.m_executionEnvironment.IsInitializer = funcMD.IsInitializer;
            m_kernelInfo.m_executionEnvironment.IsFinalizer = funcMD.IsFinalizer;

            m_kernelInfo.m_executionEnvironment.CompiledSubGroupsNumber = funcMD.CompiledSubGroupsNumber;

            m_kernelInfo.m_executionEnvironment.HasRTCalls = funcMD.hasSyncRTCalls;
        }

        m_kernelInfo.m_executionEnvironment.HasGlobalAtomics = GetHasGlobalAtomics();
        m_kernelInfo.m_threadPayload.OffsetToSkipPerThreadDataLoad = ProgramOutput()->m_offsetToSkipPerThreadDataLoad;
        m_kernelInfo.m_threadPayload.OffsetToSkipSetFFIDGP = ProgramOutput()->m_offsetToSkipSetFFIDGP;

        m_kernelInfo.m_executionEnvironment.NumGRFRequired = ProgramOutput()->m_numGRFTotal;

        m_kernelInfo.m_executionEnvironment.HasDPAS = GetHasDPAS();
        m_kernelInfo.m_executionEnvironment.StatelessWritesCount = GetStatelessWritesCount();
        m_kernelInfo.m_executionEnvironment.IndirectStatelessCount = GetIndirectStatelessCount();
        m_kernelInfo.m_executionEnvironment.numThreads = ProgramOutput()->m_numThreads;

        m_kernelInfo.m_executionEnvironment.UseBindlessMode = m_Context->m_InternalOptions.UseBindlessMode;
        m_kernelInfo.m_executionEnvironment.HasStackCalls = HasStackCalls();

        if (m_Context->enableZEBinary()) {
            FillZEKernelArgInfo();
            FillZEUserAttributes(funcInfoMD);
        }
    }

    void COpenCLKernel::RecomputeBTLayout()
    {
        CodeGenContext* pCtx = GetContext();
        ModuleMetaData* modMD = pCtx->getModuleMetaData();
        FunctionMetaData* funcMD = &modMD->FuncMD[entry];
        ResourceAllocMD* resAllocMD = &funcMD->resAllocMD;
        // Get the number of UAVs and Resources from MD.
        int numUAVs = resAllocMD->uavsNumType;
        int numResources = resAllocMD->srvsNumType;

        // Now, update the layout information
        USC::SShaderStageBTLayout* layout = ((COCLBTILayout*)m_pBtiLayout)->getModifiableLayout();

        // The BT layout contains the minimum and the maximum number BTI for each kind
        // of resource. E.g. UAVs may be mapped to BTIs 0..3, SRVs to 4..5, and the scratch
        // surface to 6.
        // Note that the names are somewhat misleading. They are used for the sake of consistency
        // with the ICBE sources.

        // Some fields are always 0 for OCL.
        layout->resourceNullBoundOffset = 0;
        layout->immediateConstantBufferOffset = 0;
        layout->interfaceConstantBufferOffset = 0;
        layout->constantBufferNullBoundOffset = 0;
        layout->JournalIdx = 0;
        layout->JournalCounterIdx = 0;

        // And TGSM (aka SLM) is always 254.
        layout->TGSMIdx = 254;

        int index = 0;

        // First, allocate BTI for debug surface
        if (m_Context->m_InternalOptions.KernelDebugEnable)
        {
            layout->systemThreadIdx = index++;
        }

        // Now, allocate BTIs for all the SRVs.
        layout->minResourceIdx = index;
        if (numResources)
        {
            index += numResources - 1;
            layout->maxResourceIdx = index++;
        }
        else
        {
            layout->maxResourceIdx = index;
        }

        // Now, ConstantBuffers - used as a placeholder for the inline constants, if present.
        layout->minConstantBufferIdx = index;
        layout->maxConstantBufferIdx = index;

        // Now, the UAVs
        layout->minUAVIdx = index;
        if (numUAVs)
        {
            index += numUAVs - 1;
            layout->maxUAVIdx = index++;
        }
        else
        {
            layout->maxUAVIdx = index;
        }

        // And finally, the scratch surface
        layout->surfaceScratchIdx = index++;

        // Overall number of used BT entries, not including TGSM.
        layout->maxBTsize = index;
    }

    bool COpenCLKernel::HasFullDispatchMask()
    {
        unsigned int groupSize = IGCMetaDataHelper::getThreadGroupSize(*m_pMdUtils, entry);
        if (groupSize != 0)
        {
            if (groupSize % numLanes(m_dispatchSize) == 0)
            {
                return true;
            }
        }
        return false;
    }

    unsigned int COpenCLKernel::getBTI(SOpenCLKernelInfo::SResourceInfo& resInfo)
    {
        switch (resInfo.Type)
        {
        case SOpenCLKernelInfo::SResourceInfo::RES_UAV:
            return m_pBtiLayout->GetUavIndex(resInfo.Index);
        case SOpenCLKernelInfo::SResourceInfo::RES_SRV:
            return m_pBtiLayout->GetTextureIndex(resInfo.Index);
        default:
            return 0xffffffff;
        }
    }

    bool COpenCLKernel::hasStatefulAccess(unsigned bti)
    {
        bool btiAssigned = (bti != 0xffffffff);
        return btiAssigned;
    }

    void CollectProgramInfo(OpenCLProgramContext* ctx)
    {
        MetaDataUtils mdUtils(ctx->getModule());
        ModuleMetaData* modMD = ctx->getModuleMetaData();

        if (!modMD->inlineConstantBuffers.empty())
        {
            // For ZeBin, constants are mantained in two separate buffers
            // the first is for general constants, and the second for string literals

            // General constants
            auto ipsbMDHandle = modMD->inlineConstantBuffers[0];
            std::unique_ptr<iOpenCL::InitConstantAnnotation> initConstant(new iOpenCL::InitConstantAnnotation());
            initConstant->Alignment = ipsbMDHandle.alignment;
            initConstant->AllocSize = ipsbMDHandle.allocSize;

            size_t bufferSize = (ipsbMDHandle.Buffer).size();
            initConstant->InlineData.resize(bufferSize);
            memcpy_s(initConstant->InlineData.data(), bufferSize, ipsbMDHandle.Buffer.data(), bufferSize);

            ctx->m_programInfo.m_initConstantAnnotation = std::move(initConstant);

            if (ctx->enableZEBinary())
            {
                // String literals
                auto ipsbStringMDHandle = modMD->inlineConstantBuffers[1];
                std::unique_ptr<iOpenCL::InitConstantAnnotation> initStringConstant(new iOpenCL::InitConstantAnnotation());
                initStringConstant->Alignment = ipsbStringMDHandle.alignment;
                initStringConstant->AllocSize = ipsbStringMDHandle.allocSize;

                bufferSize = (ipsbStringMDHandle.Buffer).size();
                initStringConstant->InlineData.resize(bufferSize);
                memcpy_s(initStringConstant->InlineData.data(), bufferSize, ipsbStringMDHandle.Buffer.data(), bufferSize);

                ctx->m_programInfo.m_initConstantStringAnnotation = std::move(initStringConstant);
            }
        }

        if (!modMD->inlineGlobalBuffers.empty())
        {
            auto ipsbMDHandle = modMD->inlineGlobalBuffers[0];

            std::unique_ptr<iOpenCL::InitGlobalAnnotation> initGlobal(new iOpenCL::InitGlobalAnnotation());
            initGlobal->Alignment = ipsbMDHandle.alignment;
            initGlobal->AllocSize = ipsbMDHandle.allocSize;

            size_t bufferSize = (ipsbMDHandle.Buffer).size();
            initGlobal->InlineData.resize(bufferSize);
            memcpy_s(initGlobal->InlineData.data(), bufferSize, ipsbMDHandle.Buffer.data(), bufferSize);

            ctx->m_programInfo.m_initGlobalAnnotation = std::move(initGlobal);
        }

        {
            auto& FuncMap = ctx->getModuleMetaData()->FuncMD;
            for (const auto& i : FuncMap)
            {
                std::unique_ptr<iOpenCL::KernelTypeProgramBinaryInfo> initConstant(new iOpenCL::KernelTypeProgramBinaryInfo());
                initConstant->KernelName = i.first->getName().str();
                if (i.second.IsFinalizer)
                {

                    initConstant->Type = iOpenCL::PROGRAM_SCOPE_KERNEL_DESTRUCTOR;
                    ctx->m_programInfo.m_initKernelTypeAnnotation.push_back(std::move(initConstant));
                }
                else if (i.second.IsInitializer)
                {
                    initConstant->Type = iOpenCL::PROGRAM_SCOPE_KERNEL_CONSTRUCTOR;
                    ctx->m_programInfo.m_initKernelTypeAnnotation.push_back(std::move(initConstant));
                }

            }
        }

        for (const auto& globPtrInfo : modMD->GlobalPointerProgramBinaryInfos)
        {
            auto initGlobalPointer = std::make_unique<iOpenCL::GlobalPointerAnnotation>();
            initGlobalPointer->PointeeAddressSpace = globPtrInfo.PointeeAddressSpace;
            initGlobalPointer->PointeeBufferIndex = globPtrInfo.PointeeBufferIndex;
            initGlobalPointer->PointerBufferIndex = globPtrInfo.PointerBufferIndex;
            initGlobalPointer->PointerOffset = globPtrInfo.PointerOffset;
            ctx->m_programInfo.m_initGlobalPointerAnnotation.push_back(std::move(initGlobalPointer));
        }

        for (const auto& constPtrInfo : modMD->ConstantPointerProgramBinaryInfos)
        {
            auto  initConstantPointer = std::make_unique<iOpenCL::ConstantPointerAnnotation>();
            initConstantPointer->PointeeAddressSpace = constPtrInfo.PointeeAddressSpace;
            initConstantPointer->PointeeBufferIndex = constPtrInfo.PointeeBufferIndex;
            initConstantPointer->PointerBufferIndex = constPtrInfo.PointerBufferIndex;
            initConstantPointer->PointerOffset = constPtrInfo.PointerOffset;

            ctx->m_programInfo.m_initConstantPointerAnnotation.push_back(std::move(initConstantPointer));
        }

        // Pointer address relocation table data for GLOBAL buffer
        for (const auto& globalRelocEntry : modMD->GlobalBufferAddressRelocInfo)
        {
            ctx->m_programInfo.m_GlobalPointerAddressRelocAnnotation.globalReloc.emplace_back(
                (globalRelocEntry.PointerSize == 8) ? vISA::GenRelocType::R_SYM_ADDR : vISA::GenRelocType::R_SYM_ADDR_32,
                (uint32_t)globalRelocEntry.BufferOffset,
                globalRelocEntry.Symbol);
        }
        // Pointer address relocation table data for CONST buffer
        for (const auto& constRelocEntry : modMD->ConstantBufferAddressRelocInfo)
        {
            ctx->m_programInfo.m_GlobalPointerAddressRelocAnnotation.globalConstReloc.emplace_back(
                (constRelocEntry.PointerSize == 8) ? vISA::GenRelocType::R_SYM_ADDR : vISA::GenRelocType::R_SYM_ADDR_32,
                (uint32_t)constRelocEntry.BufferOffset,
                constRelocEntry.Symbol);
        }
    }

    bool COpenCLKernel::IsValidShader(COpenCLKernel* pShader)
    {
        return pShader && (pShader->ProgramOutput()->m_programSize > 0);
    }

    bool COpenCLKernel::IsVisaCompiledSuccessfullyForShader(COpenCLKernel* pShader)
    {
        return pShader && pShader->GetEncoder().IsVisaCompiledSuccessfully();
    }

    bool COpenCLKernel::IsVisaCompileStatusFailureForShader(COpenCLKernel *pShader) {
        return pShader && pShader->GetEncoder().IsVisaCompileStatusFailure();
    }

    enum class RetryType
    {
        NO_Retry,
        NO_Retry_Pick_Prv,
        YES_Retry
    };

    RetryType NeedsRetry(
        OpenCLProgramContext* ctx,
        COpenCLKernel* pShader,
        CShaderProgram::UPtr& pKernel,
        Function* pFunc,
        MetaDataUtils* pMdUtils,
        SIMDMode simdMode)
    {
        IGC_ASSERT(pShader != nullptr);
        pShader->FillKernel(simdMode);
        SProgramOutput* pOutput = pShader->ProgramOutput();

        CShader* program = pKernel.get()->GetShader(simdMode);

        bool isWorstThanPrv = false;
        // Look for previous generated shaders
        // ignoring case for multi-simd compilation, or if kernel has stackcalls
        if (!((ctx->m_DriverInfo.sendMultipleSIMDModes() || ctx->m_enableSimdVariantCompilation)
            && (ctx->getModuleMetaData()->csInfo.forcedSIMDSize == 0)) &&
            pKernel.get() != nullptr &&
            !(program->HasStackCalls() || program->IsIntelSymbolTableVoidProgram()))
        {
            isWorstThanPrv =
                !ctx->m_retryManager.IsBetterThanPrevious(pKernel.get());
        }

        if (isWorstThanPrv)
        {
            return RetryType::NO_Retry_Pick_Prv;
        }
        else if (
            pOutput->m_scratchSpaceUsedBySpills == 0 ||
            ctx->getModuleMetaData()->compOpt.OptDisable ||
            ctx->m_retryManager.IsLastTry() ||
            (!ctx->m_retryManager.kernelSkip.empty() &&
             ctx->m_retryManager.kernelSkip.count(pFunc->getName().str())))
        {
            return RetryType::NO_Retry;
        }
        else
        {
            return RetryType::YES_Retry;
        }
    }

    void GatherDataForDriver(
        OpenCLProgramContext* ctx,
        COpenCLKernel* pShader,
        CShaderProgram::UPtr pKernel,
        Function* pFunc,
        MetaDataUtils* pMdUtils,
        SIMDMode simdMode)
    {
        CShaderProgram::UPtr pSelectedKernel;
        switch (NeedsRetry(ctx, pShader, pKernel, pFunc, pMdUtils, simdMode))
        {
        case RetryType::NO_Retry_Pick_Prv:
        {
            // In case retry compilation give worst generated kernel
            // consider using the previous one do not retry on this
            // kernel again
            pSelectedKernel =
                CShaderProgram::UPtr(ctx->m_retryManager.GetPrevious(pKernel.get(), true));
        }
        case RetryType::NO_Retry:
        {
            // Save the shader program to the state processor to be handled later
            if (!pSelectedKernel && pKernel)
            {
                pSelectedKernel = std::move(pKernel);
            }
        }
        // Common part for NO_Retry:
        {
            if (pSelectedKernel)
            {
                COMPILER_SHADER_STATS_PRINT(pSelectedKernel->m_shaderStats, ShaderType::OPENCL_SHADER, ctx->hash, pFunc->getName().str());
                COMPILER_SHADER_STATS_SUM(ctx->m_sumShaderStats, pSelectedKernel->m_shaderStats, ShaderType::OPENCL_SHADER);
                COMPILER_SHADER_STATS_DEL(pSelectedKernel->m_shaderStats);
                ctx->m_programOutput.m_ShaderProgramList.push_back(std::move(pSelectedKernel));
            }
            break;
        }
        case RetryType::YES_Retry:
        {
            // Collect the current compilation for the next compare
            ctx->m_retryManager.Collect(std::move(pKernel));
            ctx->m_retryManager.kernelSet.insert(pShader->m_kernelInfo.m_kernelName);

            break;
        }
        }
    }

    static void verifyOOBScratch(OpenCLProgramContext *ctx,
                                 COpenCLKernel *simd8Shader,
                                 COpenCLKernel *simd16Shader,
                                 COpenCLKernel *simd32Shader) {
        auto verify = [ctx](CShader *shader) {
            unsigned int totalScratchUse =
                shader->ProgramOutput()->m_scratchSpaceUsedBySpills;
            if (shader->ProgramOutput()->m_UseScratchSpacePrivateMemory)
              totalScratchUse +=
                  shader->ProgramOutput()->m_scratchSpaceUsedByShader;
            if (totalScratchUse >
                shader->ProgramOutput()->m_scratchSpaceSizeLimit) {
              std::string errorMsg =
                  "total scratch space exceeds HW "
                  "supported limit for kernel " +
                  shader->entry->getName().str() + ": " +
                  std::to_string(totalScratchUse) + " bytes (max permitted PTSS " +
                  std::to_string(shader->ProgramOutput()->m_scratchSpaceSizeLimit) +
                  " bytes)";

              ctx->EmitError(errorMsg.c_str(), nullptr);
            }
        };

        // Need to check if simd* shader is not nullptr and its vISA compile status,
        // since it may be created without going through full vISA compilation and
        // the spill size record may be invalid
        if (simd8Shader && !COpenCLKernel::IsVisaCompileStatusFailureForShader(simd8Shader))
          verify(simd8Shader);
        else if (simd16Shader && !COpenCLKernel::IsVisaCompileStatusFailureForShader(simd16Shader))
          verify(simd16Shader);
        else if (simd32Shader && !COpenCLKernel::IsVisaCompileStatusFailureForShader(simd32Shader))
          verify(simd32Shader);
    }

    static void CodeGen(OpenCLProgramContext* ctx, CShaderProgram::KernelShaderMap& shaders)
    {
        COMPILER_TIME_START(ctx, TIME_CodeGen);
        COMPILER_TIME_START(ctx, TIME_CG_Add_Passes);

        IGCPassManager Passes(ctx, "CG");

        AddLegalizationPasses(*ctx, Passes);

        AddAnalysisPasses(*ctx, Passes);

        if (ctx->m_enableFunctionPointer
            && (ctx->m_DriverInfo.sendMultipleSIMDModes() || ctx->m_enableSimdVariantCompilation)
            && ctx->getModuleMetaData()->csInfo.forcedSIMDSize == 0)
        {
            // In order to support compiling multiple SIMD modes for function pointer calls,
            // we require a separate pass manager per SIMD mode, due to interdependencies across
            // function compilations.
            SIMDMode pass1Mode;
            SIMDMode pass2Mode;

            if (ctx->platform.getMinDispatchMode() == SIMDMode::SIMD16)
            {
                pass1Mode = SIMDMode::SIMD32;
                pass2Mode = SIMDMode::SIMD16;
            }
            else
            {
                pass1Mode = SIMDMode::SIMD16;
                pass2Mode = SIMDMode::SIMD8;
            }
            // Run first pass
            AddCodeGenPasses(*ctx, shaders, Passes, pass1Mode, false);
            Passes.run(*(ctx->getModule()));

            // Create and run second pass
            IGCPassManager Passes2(ctx, "CG2");
            // Add required immutable passes
            Passes2.add(new MetaDataUtilsWrapper(ctx->getMetaDataUtils(), ctx->getModuleMetaData()));
            Passes2.add(new CodeGenContextWrapper(ctx));
            Passes2.add(createGenXFunctionGroupAnalysisPass());
            AddCodeGenPasses(*ctx, shaders, Passes2, pass2Mode, false);
            COMPILER_TIME_END(ctx, TIME_CG_Add_Passes);
            Passes2.run(*(ctx->getModule()));

            COMPILER_TIME_END(ctx, TIME_CodeGen);
            DumpLLVMIR(ctx, "codegen");
            return;
        }

        if (ctx->m_DriverInfo.sendMultipleSIMDModes())
        {
            unsigned int leastSIMD = 8;
            if (ctx->getModuleMetaData()->csInfo.maxWorkGroupSize)
            {
                const SIMDMode leastSIMDMode = getLeastSIMDAllowed(ctx->getModuleMetaData()->csInfo.maxWorkGroupSize, GetHwThreadsPerWG(ctx->platform));
                leastSIMD = numLanes(leastSIMDMode);
            }
            if (ctx->getModuleMetaData()->csInfo.forcedSIMDSize)
            {
                IGC_ASSERT_MESSAGE((ctx->getModuleMetaData()->csInfo.forcedSIMDSize >= leastSIMD), "Incorrect SIMD forced");
            }
            if (leastSIMD <= 8)
            {
                AddCodeGenPasses(*ctx, shaders, Passes, SIMDMode::SIMD8, false);
                AddCodeGenPasses(*ctx, shaders, Passes, SIMDMode::SIMD16, (ctx->getModuleMetaData()->csInfo.forcedSIMDSize != 16));
                AddCodeGenPasses(*ctx, shaders, Passes, SIMDMode::SIMD32, (ctx->getModuleMetaData()->csInfo.forcedSIMDSize != 32));
            }
            else if (leastSIMD <= 16)
            {
                AddCodeGenPasses(*ctx, shaders, Passes, SIMDMode::SIMD16, false);
                AddCodeGenPasses(*ctx, shaders, Passes, SIMDMode::SIMD32, (ctx->getModuleMetaData()->csInfo.forcedSIMDSize != 32));

                ctx->SetSIMDInfo(SIMD_SKIP_HW, SIMDMode::SIMD8, ShaderDispatchMode::NOT_APPLICABLE);
            }
            else
            {
                AddCodeGenPasses(*ctx, shaders, Passes, SIMDMode::SIMD32, false);

                ctx->SetSIMDInfo(SIMD_SKIP_HW, SIMDMode::SIMD8, ShaderDispatchMode::NOT_APPLICABLE);
                ctx->SetSIMDInfo(SIMD_SKIP_HW, SIMDMode::SIMD16, ShaderDispatchMode::NOT_APPLICABLE);
            }
        }
        else
        {
            if (ctx->platform.getMinDispatchMode() == SIMDMode::SIMD16)
            {
                AddCodeGenPasses(*ctx, shaders, Passes, SIMDMode::SIMD32, false);
                AddCodeGenPasses(*ctx, shaders, Passes, SIMDMode::SIMD16, false);

                ctx->SetSIMDInfo(SIMD_SKIP_HW, SIMDMode::SIMD8, ShaderDispatchMode::NOT_APPLICABLE);
            }
            else
            {
                // The order in which we call AddCodeGenPasses matters, please to not change order
                AddCodeGenPasses(*ctx, shaders, Passes, SIMDMode::SIMD32, (ctx->getModuleMetaData()->csInfo.forcedSIMDSize != 32));
                AddCodeGenPasses(*ctx, shaders, Passes, SIMDMode::SIMD16, (ctx->getModuleMetaData()->csInfo.forcedSIMDSize != 16));
                AddCodeGenPasses(*ctx, shaders, Passes, SIMDMode::SIMD8, false);
            }
        }

        Passes.add(new DebugInfoPass(shaders));
        COMPILER_TIME_END(ctx, TIME_CG_Add_Passes);

        Passes.run(*(ctx->getModule()));
        COMPILER_TIME_END(ctx, TIME_CodeGen);
        DumpLLVMIR(ctx, "codegen");
    }

    void CodeGen(OpenCLProgramContext *ctx)
    {
#ifndef DX_ONLY_IGC
#ifndef VK_ONLY_IGC
        // Do program-wide code generation.
        // Currently, this just creates the program-scope patch stream.
        if (ctx->m_retryManager.IsFirstTry())
        {
            CollectProgramInfo(ctx);
            if (!ctx->enableZEBinary())
            {
                ctx->m_programOutput.CreateProgramScopePatchStream(ctx->m_programInfo);
            }
        }

        MetaDataUtils* pMdUtils = ctx->getMetaDataUtils();

        //Clear spill parameters of retry manager in the very begining of code gen
        ctx->m_retryManager.ClearSpillParams();

        CShaderProgram::KernelShaderMap shaders;
        CodeGen(ctx, shaders);

        if (ctx->m_programOutput.m_pSystemThreadKernelOutput == nullptr)
        {
            const auto options = ctx->m_InternalOptions;
            if (options.IncludeSIPCSR ||
                options.IncludeSIPKernelDebug ||
                options.IncludeSIPKernelDebugWithLocalMemory ||
                options.KernelDebugEnable)
            {
                DWORD systemThreadMode = 0;

                if (options.IncludeSIPCSR)
                {
                    systemThreadMode |= USC::SYSTEM_THREAD_MODE_CSR;
                }

                if (options.KernelDebugEnable ||
                    options.IncludeSIPKernelDebug)
                {
                    systemThreadMode |= USC::SYSTEM_THREAD_MODE_DEBUG;
                }

                if (options.IncludeSIPKernelDebugWithLocalMemory)
                {
                    systemThreadMode |= USC::SYSTEM_THREAD_MODE_DEBUG_LOCAL;
                }

                bool success = SIP::CSystemThread::CreateSystemThreadKernel(
                    ctx->platform,
                    (USC::SYSTEM_THREAD_MODE)systemThreadMode,
                    ctx->m_programOutput.m_pSystemThreadKernelOutput);

                if (!success)
                {
                    ctx->EmitError("System thread kernel could not be created!", nullptr);
                }
            }
        }

        // Clear the retry set and collect kernels for retry in the loop below.
        ctx->m_retryManager.kernelSet.clear();

        // If kernel needs retry, its shaderProgram should be deleted
        SmallVector<CShaderProgram*, 8> toBeDeleted;
        // gather data to send back to the driver
        for (const auto& k : shaders)
        {
            Function* pFunc = k.first;
            CShaderProgram::UPtr pKernel = CShaderProgram::UPtr(static_cast<CShaderProgram*>(k.second));
            COpenCLKernel* simd8Shader = static_cast<COpenCLKernel*>(pKernel->GetShader(SIMDMode::SIMD8));
            COpenCLKernel* simd16Shader = static_cast<COpenCLKernel*>(pKernel->GetShader(SIMDMode::SIMD16));
            COpenCLKernel* simd32Shader = static_cast<COpenCLKernel*>(pKernel->GetShader(SIMDMode::SIMD32));

            if ((ctx->m_DriverInfo.sendMultipleSIMDModes() || ctx->m_enableSimdVariantCompilation)
                && (ctx->getModuleMetaData()->csInfo.forcedSIMDSize == 0))
            {
                //Gather the kernel binary for each compiled kernel
                if (COpenCLKernel::IsValidShader(simd32Shader))
                    GatherDataForDriver(ctx, simd32Shader, std::move(pKernel), pFunc, pMdUtils, SIMDMode::SIMD32);
                if (COpenCLKernel::IsValidShader(simd16Shader))
                    GatherDataForDriver(ctx, simd16Shader, std::move(pKernel), pFunc, pMdUtils, SIMDMode::SIMD16);
                if (COpenCLKernel::IsValidShader(simd8Shader))
                    GatherDataForDriver(ctx, simd8Shader, std::move(pKernel), pFunc, pMdUtils, SIMDMode::SIMD8);
                // TODO: check if we need to invoke verifyOOBScratch(...) here
            }
            else if (ctx->m_InternalOptions.EmitVisaOnly)
            {
                if (COpenCLKernel::IsVisaCompiledSuccessfullyForShader(simd32Shader))
                    GatherDataForDriver(ctx, simd32Shader, std::move(pKernel), pFunc, pMdUtils, SIMDMode::SIMD32);
                else if (COpenCLKernel::IsVisaCompiledSuccessfullyForShader(simd16Shader))
                    GatherDataForDriver(ctx, simd16Shader, std::move(pKernel), pFunc, pMdUtils, SIMDMode::SIMD16);
                else if (COpenCLKernel::IsVisaCompiledSuccessfullyForShader(simd8Shader))
                    GatherDataForDriver(ctx, simd8Shader, std::move(pKernel), pFunc, pMdUtils, SIMDMode::SIMD8);
            }
            else
            {
                //Gather the kernel binary only for 1 SIMD mode of the kernel
                if (COpenCLKernel::IsValidShader(simd32Shader))
                    GatherDataForDriver(ctx, simd32Shader, std::move(pKernel), pFunc, pMdUtils, SIMDMode::SIMD32);
                else if (COpenCLKernel::IsValidShader(simd16Shader))
                    GatherDataForDriver(ctx, simd16Shader, std::move(pKernel), pFunc, pMdUtils, SIMDMode::SIMD16);
                else if (COpenCLKernel::IsValidShader(simd8Shader))
                    GatherDataForDriver(ctx, simd8Shader, std::move(pKernel), pFunc, pMdUtils, SIMDMode::SIMD8);
                else
                  // Verify if compilation failed due to OOB scratch
                  verifyOOBScratch(ctx, simd8Shader, simd16Shader, simd32Shader);
            }
        }

        // The skip set to avoid retry is not needed. Clear it and collect a new set
        // during retry compilation.
        ctx->m_retryManager.kernelSkip.clear();
#endif // ifndef VK_ONLY_IGC
#endif // ifndef DX_ONLY_IGC
    }

    bool COpenCLKernel::hasReadWriteImage(llvm::Function& F)
    {
        if (!isEntryFunc(m_pMdUtils, &F))
        {
            // Ignore read/write flags for subroutines for now.
            // TODO: get access types for subroutines without using kernel args
            return false;
        }

        KernelArgs kernelArgs(F, m_DL, m_pMdUtils, m_ModuleMetadata, getGRFSize(), KernelArgsOrder::InputType::INDEPENDENT);
        for (const auto& KA : kernelArgs)
        {
            // RenderScript annotation sets "read_write" qualifier
            // for any applicable kernel argument, not only for kernel arguments
            // that are images, so we should check if kernel argument is an image.
            if (KA.getAccessQual() == KernelArg::AccessQual::READ_WRITE &&
                KA.getArgType() >= KernelArg::ArgType::IMAGE_1D &&
                KA.getArgType() <= KernelArg::ArgType::BINDLESS_IMAGE_CUBE_DEPTH_ARRAY)
            {
                return true;
            }
        }
        return false;
    }

    bool COpenCLKernel::CompileSIMDSize(SIMDMode simdMode, EmitPass& EP, llvm::Function& F)
    {
        if (!CompileSIMDSizeInCommon(simdMode))
            return false;

        // Skip compiling the simd size if -emit-visa-only and there's already
        // one visa compiled for any other simd size.
        if (m_Context->m_InternalOptions.EmitVisaOnly) {
            for (auto mode : { SIMDMode::SIMD8, SIMDMode::SIMD16, SIMDMode::SIMD32 }) {
                COpenCLKernel* shader = static_cast<COpenCLKernel*>(m_parent->GetShader(mode));
                if (COpenCLKernel::IsVisaCompiledSuccessfullyForShader(shader))
                    return false;
            }
        }

        if (!m_Context->m_retryManager.IsFirstTry())
        {
            m_Context->ClearSIMDInfo(simdMode, ShaderDispatchMode::NOT_APPLICABLE);
            m_Context->SetSIMDInfo(SIMD_RETRY, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
        }

        // Currently the FunctionMetaData is being looked up solely in order to get the hasSyncRTCalls
        // If we would need to get some non-raytracing related field out of the FunctionMetaData,
        // then we can move the lookup out of the #if and just leave the bool hasSyncRTCalls inside.
        auto& FuncMap = m_Context->getModuleMetaData()->FuncMD;
        // we want to check the setting for the associated kernel
        auto FuncIter = FuncMap.find(entry);
        if (FuncIter == FuncMap.end()) {  // wasn't able to find the meta data for the passed in llvm::Function!
            // All of the kernels should have an entry in the map.
            IGC_ASSERT(0);
            return false;
        }
        const FunctionMetaData& funcMD = FuncIter->second;
        bool hasSyncRTCalls = funcMD.hasSyncRTCalls;  // if the function/kernel has sync raytracing calls

        //If forced SIMD Mode (by driver or regkey), then:
        // 1. Compile only that SIMD mode and nothing else
        // 2. Compile that SIMD mode even if it is not profitable, i.e. even if compileThisSIMD() returns false for it.
        //    So, don't bother checking profitability for it
        if (m_Context->getModuleMetaData()->csInfo.forcedSIMDSize != 0)
        {
            // Entered here means driver has requested a specific SIMD mode, which was forced in the regkey ForceOCLSIMDWidth.
            // We return the condition can we compile the given forcedSIMDSize with this simdMode?
            return (
                // These statements are basically equivalent to (simdMode == forcedSIMDSize)
                (simdMode == SIMDMode::SIMD8 && m_Context->getModuleMetaData()->csInfo.forcedSIMDSize == 8)   ||
                (simdMode == SIMDMode::SIMD16 && m_Context->getModuleMetaData()->csInfo.forcedSIMDSize == 16) ||
                // if we want to compile SIMD32, we need to be lacking any raytracing calls; raytracing doesn't support SIMD16
                (simdMode == SIMDMode::SIMD32 && m_Context->getModuleMetaData()->csInfo.forcedSIMDSize == 32 && !hasSyncRTCalls)
            );
        }

        SIMDStatus simdStatus = SIMDStatus::SIMD_FUNC_FAIL;
        if (m_Context->platform.getMinDispatchMode() == SIMDMode::SIMD16)
        {
            simdStatus = checkSIMDCompileCondsPVC(simdMode, EP, F, hasSyncRTCalls);
        }
        else
        {
            simdStatus = checkSIMDCompileConds(simdMode, EP, F, hasSyncRTCalls);
        }

        // Func and Perf checks pass, compile this SIMD
        if (simdStatus == SIMDStatus::SIMD_PASS)
            return true;

        // Functional failure, skip compiling this SIMD
        if (simdStatus == SIMDStatus::SIMD_FUNC_FAIL)
            return false;

        IGC_ASSERT(simdStatus == SIMDStatus::SIMD_PERF_FAIL);
        //not profitable
        if (m_Context->m_DriverInfo.sendMultipleSIMDModes())
        {
            if (EP.m_canAbortOnSpill)
                return false; //not the first functionally correct SIMD, exit
            else
                return true; //is the first functionally correct SIMD, compile
        }
        return simdStatus == SIMDStatus::SIMD_PASS;
    }

    SIMDStatus COpenCLKernel::checkSIMDCompileCondsPVC(SIMDMode simdMode, EmitPass& EP, llvm::Function& F, bool hasSyncRTCalls)
    {
        if (simdMode == SIMDMode::SIMD8)
        {
            return SIMDStatus::SIMD_FUNC_FAIL;
        }

        EP.m_canAbortOnSpill = false; // spill is always allowed since we don't do SIMD size lowering
        // Next we check if there is a required sub group size specified
        CodeGenContext* pCtx = GetContext();
        MetaDataUtils* pMdUtils = EP.getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
        FunctionInfoMetaDataHandle funcInfoMD = pMdUtils->getFunctionsInfoItem(&F);
        int simd_size = funcInfoMD->getSubGroupSize()->getSIMD_size();
        bool hasSubGroupForce = hasSubGroupIntrinsicPVC(F);

        // Finds the kernel and get the group simd size from the kernel
        if (m_FGA)
        {
            llvm::Function* Kernel = &F;
            auto FG = m_FGA->getGroup(&F);
            Kernel = FG->getHead();
            funcInfoMD = pMdUtils->getFunctionsInfoItem(Kernel);
            simd_size = funcInfoMD->getSubGroupSize()->getSIMD_size();
        }

        auto FG = m_FGA ? m_FGA->getGroup(&F) : nullptr;
        bool hasStackCall = FG && FG->hasStackCall();
        bool isIndirectGroup = FG && m_FGA->isIndirectCallGroup(FG);
        bool hasSubroutine = FG && !FG->isSingle() && !hasStackCall && !isIndirectGroup;
        bool forceLowestSIMDForStackCalls = IGC_IS_FLAG_ENABLED(ForceLowestSIMDForStackCalls) && (hasStackCall || isIndirectGroup);

        if (simd_size == 0)
        {
            if (hasSubroutine &&
                simdMode != SIMDMode::SIMD16)
            {
                pCtx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
                return SIMDStatus::SIMD_FUNC_FAIL;
            }
            if (forceLowestSIMDForStackCalls &&
                simdMode != SIMDMode::SIMD16)
            {
                pCtx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
                return SIMDStatus::SIMD_FUNC_FAIL;
            }
        }

        bool optDisable = this->GetContext()->getModuleMetaData()->compOpt.OptDisable;

        if (optDisable && simd_size == 0) // if simd size not requested in MD
        {
            if (simdMode == SIMDMode::SIMD32)
            {
                pCtx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
                return SIMDStatus::SIMD_FUNC_FAIL;
            }

            // simd16 forced when all optimizations disabled due to compile time optimization
            pCtx->getModuleMetaData()->csInfo.forcedSIMDSize = (unsigned char)numLanes(SIMDMode::SIMD16);
        }

        if (simdMode == SIMDMode::SIMD32 && hasSyncRTCalls) {
            return SIMDStatus::SIMD_FUNC_FAIL;
        }
        else if (simdMode == SIMDMode::SIMD16 && hasSyncRTCalls) {
            return SIMDStatus::SIMD_PASS;
        }

        if (simd_size)
        {
            if (simd_size != numLanes(simdMode))
            {
                if (simd_size == 8 && simdMode == SIMDMode::SIMD32)
                {
                    // allow for now to avoid NEO build failures
                    // ToDo: remove once NEO removes all SIMD8 kernel ULTs from driver build
                    return SIMDStatus::SIMD_PASS;
                }
                pCtx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
                return SIMDStatus::SIMD_FUNC_FAIL;
            }
            switch (simd_size)
            {
            case 8:
                //IGC_ASSERT_MESSAGE(0, "Unsupported required sub group size for PVC");
                break;
            case 16:
            case 32:
                break;
            default:
                IGC_ASSERT_MESSAGE(0, "Unsupported required sub group size");
                break;
            }
        }
        else
        {
            // Checking registry/flag here. Note that if ForceOCLSIMDWidth is set to
            // 8/16/32, only corresponding EnableOCLSIMD<N> is set to true. Therefore,
            // if any of EnableOCLSIMD<N> is disabled, ForceOCLSIMDWidth must set to
            // a value other than <N> if set. See igc_regkeys.cpp for detail.
            if ((simdMode == SIMDMode::SIMD32 && IGC_IS_FLAG_DISABLED(EnableOCLSIMD32)) ||
                (simdMode == SIMDMode::SIMD16 && IGC_IS_FLAG_DISABLED(EnableOCLSIMD16)))
            {
                return SIMDStatus::SIMD_FUNC_FAIL;
            }

            // Check if we force code generation for the current SIMD size.
            // Note that for SIMD8, we always force it!
            //ATTN: This check is redundant!
            if (numLanes(simdMode) == pCtx->getModuleMetaData()->csInfo.forcedSIMDSize)
            {
                return SIMDStatus::SIMD_PASS;
            }

            if (simdMode == SIMDMode::SIMD16 && !hasSubGroupForce && !forceLowestSIMDForStackCalls && !hasSubroutine)
            {
                pCtx->SetSIMDInfo(SIMD_SKIP_PERF, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
                return SIMDStatus::SIMD_FUNC_FAIL;
            }

            if (simdMode == SIMDMode::SIMD32 && hasSubGroupForce)
            {
                pCtx->SetSIMDInfo(SIMD_SKIP_PERF, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
                return SIMDStatus::SIMD_FUNC_FAIL;
            }
        }

        return SIMDStatus::SIMD_PASS;
    }

    int COpenCLKernel::getAnnotatedNumThreads() {
        return m_annotatedNumThreads;
    }

    bool COpenCLKernel::IsRegularGRFRequested()
    {
        return m_regularGRFRequested;
    }

    bool COpenCLKernel::IsLargeGRFRequested()
    {
        return m_largeGRFRequested;
    }

    SIMDStatus COpenCLKernel::checkSIMDCompileConds(SIMDMode simdMode, EmitPass& EP, llvm::Function& F, bool hasSyncRTCalls)
    {
        CShader* simd8Program = m_parent->GetShader(SIMDMode::SIMD8);
        CShader* simd16Program = m_parent->GetShader(SIMDMode::SIMD16);
        CShader* simd32Program = m_parent->GetShader(SIMDMode::SIMD32);

        CodeGenContext* pCtx = GetContext();

        bool compileFunctionVariants = pCtx->m_enableSimdVariantCompilation &&
            (m_FGA && IGC::isIntelSymbolTableVoidProgram(m_FGA->getGroupHead(&F)));

        // Here we see if we have compiled a size for this shader already
        if ((simd8Program && simd8Program->ProgramOutput()->m_programSize > 0) ||
            (simd16Program && simd16Program->ProgramOutput()->m_programSize > 0) ||
            (simd32Program && simd32Program->ProgramOutput()->m_programSize > 0))
        {
            bool canCompileMultipleSIMD = pCtx->m_DriverInfo.sendMultipleSIMDModes() || compileFunctionVariants;
            if (!(canCompileMultipleSIMD && (pCtx->getModuleMetaData()->csInfo.forcedSIMDSize == 0)))
                return SIMDStatus::SIMD_FUNC_FAIL;
        }

        // Next we check if there is a required sub group size specified
        MetaDataUtils* pMdUtils = EP.getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
        ModuleMetaData* modMD = pCtx->getModuleMetaData();
        FunctionInfoMetaDataHandle funcInfoMD = pMdUtils->getFunctionsInfoItem(&F);
        int simd_size = funcInfoMD->getSubGroupSize()->getSIMD_size();

        // Finds the kernel and get the group simd size from the kernel
        if (m_FGA)
        {
            llvm::Function* Kernel = &F;
            auto FG = m_FGA->getGroup(&F);
            Kernel = FG->getHead();
            funcInfoMD = pMdUtils->getFunctionsInfoItem(Kernel);
            simd_size = funcInfoMD->getSubGroupSize()->getSIMD_size();
        }

        // For simd variant functions, detect which SIMD sizes are needed
        if (compileFunctionVariants && F.hasFnAttribute("variant-function-def"))
        {
            bool canCompile = true;
            if (simdMode == SIMDMode::SIMD16)
                canCompile = F.hasFnAttribute("CompileSIMD16");
            else if (simdMode == SIMDMode::SIMD8)
                canCompile = F.hasFnAttribute("CompileSIMD8");

            if (!canCompile)
            {
                pCtx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
                return SIMDStatus::SIMD_FUNC_FAIL;
            }
        }

        auto FG = m_FGA ? m_FGA->getGroup(&F) : nullptr;
        bool hasStackCall = FG && FG->hasStackCall();
        bool isIndirectGroup = FG && m_FGA->isIndirectCallGroup(FG);
        bool hasSubroutine = FG && !FG->isSingle() && !hasStackCall && !isIndirectGroup;

        // If stack calls are present, disable simd32 in order to do CallWA in visa
        if (pCtx->platform.requireCallWA() && simdMode == SIMDMode::SIMD32)
        {
            bool hasNestedCall = FG && FG->hasNestedCall();
            bool hasIndirectCall = FG && FG->hasIndirectCall();
            if (hasNestedCall || hasIndirectCall || isIndirectGroup)
            {
                // If sub_group_size is set to 32, resize it to 16 so SIMD16 compilation will still succeed
                if (simd_size == 32)
                {
                    llvm::Function* Kernel = FG->getHead();
                    funcInfoMD = pMdUtils->getFunctionsInfoItem(Kernel);
                    funcInfoMD->getSubGroupSize()->setSIMD_size(16);
                }
                pCtx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
                return SIMDStatus::SIMD_FUNC_FAIL;
            }
        }

        if (simd_size == 0)
        {
            // Default to lowest SIMD mode for stack calls/indirect calls
            if (IGC_IS_FLAG_ENABLED(ForceLowestSIMDForStackCalls) &&
                (hasStackCall || isIndirectGroup) &&
                simdMode != SIMDMode::SIMD8)
            {
                pCtx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
                return SIMDStatus::SIMD_FUNC_FAIL;
            }

            // Just subroutines and subgroup size is not set, default to SIMD8
            if (hasSubroutine &&
                simdMode != SIMDMode::SIMD8)
            {
                pCtx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
                return SIMDStatus::SIMD_FUNC_FAIL;
            }
        }

        uint32_t groupSize = 0;
        if (modMD->csInfo.maxWorkGroupSize)
        {
            groupSize = modMD->csInfo.maxWorkGroupSize;
        }
        else
        {
            groupSize = IGCMetaDataHelper::getThreadGroupSize(*pMdUtils, &F);
        }

        if (groupSize == 0)
        {
            groupSize = IGCMetaDataHelper::getThreadGroupSizeHint(*pMdUtils, &F);
        }

        if (simd_size)
        {
            switch (simd_size)
            {
                // Apparently the only possible simdModes here are SIMD8, SIMD16, SIMD32
            case 8:
                if (simdMode != SIMDMode::SIMD8)
                {
                    pCtx->SetSIMDInfo(SIMD_SKIP_THGRPSIZE, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
                    return SIMDStatus::SIMD_FUNC_FAIL;
                }
                break;
            case 16:
                if (simdMode != SIMDMode::SIMD16)
                {
                    pCtx->SetSIMDInfo(SIMD_SKIP_THGRPSIZE, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
                    return SIMDStatus::SIMD_FUNC_FAIL;
                }
                EP.m_canAbortOnSpill = false;
                break;
            case 32:
                if (simdMode != SIMDMode::SIMD32)
                {
                    pCtx->SetSIMDInfo(SIMD_SKIP_THGRPSIZE, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
                    return SIMDStatus::SIMD_FUNC_FAIL;
                }
                else {
                    if (hasSyncRTCalls) {
                        return SIMDStatus::SIMD_FUNC_FAIL;  // SIMD32 unsupported with raytracing calls
                    }
                    else {  // simdMode == SIMDMode::SIMD32 && !hasSyncRTCalls
                        EP.m_canAbortOnSpill = false;
                    }
                }
                break;
            default:
                IGC_ASSERT_MESSAGE(0, "Unsupported required sub group size");
                break;
            }
        }
        else
        {
            // Checking registry/flag here. Note that if ForceOCLSIMDWidth is set to
            // 8/16/32, only corresponding EnableOCLSIMD<N> is set to true. Therefore,
            // if any of EnableOCLSIMD<N> is disabled, ForceOCLSIMDWidth must set to
            // a value other than <N> if set. See igc_regkeys.cpp for detail.
            if ((simdMode == SIMDMode::SIMD32 && IGC_IS_FLAG_DISABLED(EnableOCLSIMD32)) ||
                (simdMode == SIMDMode::SIMD16 && IGC_IS_FLAG_DISABLED(EnableOCLSIMD16)))
            {
                return SIMDStatus::SIMD_FUNC_FAIL;
            }

            // Check if we force code generation for the current SIMD size.
            // Note that for SIMD8, we always force it!
            //ATTN: This check is redundant!
            if (numLanes(simdMode) == pCtx->getModuleMetaData()->csInfo.forcedSIMDSize ||
                simdMode == SIMDMode::SIMD8)
            {
                return SIMDStatus::SIMD_PASS;
            }

            if (hasSyncRTCalls) {
                // If we get all the way to here, then set it to the preferred SIMD size for Ray Tracing.
                SIMDMode mode = SIMDMode::UNKNOWN;
                mode = m_Context->platform.getPreferredRayTracingSIMDSize();
                return (mode == simdMode) ? SIMDStatus::SIMD_PASS : SIMDStatus::SIMD_FUNC_FAIL;
            }

            if (groupSize != 0 && groupSize <= 16)
            {
                if (simdMode == SIMDMode::SIMD32 ||
                    (groupSize <= 8 && simdMode != SIMDMode::SIMD8))
                {
                    pCtx->SetSIMDInfo(SIMD_SKIP_THGRPSIZE, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
                    return SIMDStatus::SIMD_FUNC_FAIL;
                }
            }

            // Here we check profitablility, etc.
            if (simdMode == SIMDMode::SIMD16)
            {
                bool optDisable = this->GetContext()->getModuleMetaData()->compOpt.OptDisable;

                if (optDisable)
                {
                    return SIMDStatus::SIMD_FUNC_FAIL;
                }

                // bail out of SIMD16 if it's not profitable.
                Simd32ProfitabilityAnalysis& PA = EP.getAnalysis<Simd32ProfitabilityAnalysis>();
                if (!PA.isSimd16Profitable())
                {
                    pCtx->SetSIMDInfo(SIMD_SKIP_PERF, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
                    return SIMDStatus::SIMD_PERF_FAIL;
                }
            }
            if (simdMode == SIMDMode::SIMD32)
            {
                bool optDisable = this->GetContext()->getModuleMetaData()->compOpt.OptDisable;

                if (optDisable)
                {
                    return SIMDStatus::SIMD_FUNC_FAIL;
                }

                // bail out of SIMD32 if it's not profitable.
                Simd32ProfitabilityAnalysis& PA = EP.getAnalysis<Simd32ProfitabilityAnalysis>();
                if (!PA.isSimd32Profitable())
                {
                    pCtx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
                    return SIMDStatus::SIMD_PERF_FAIL;
                }
            }
        }

        return SIMDStatus::SIMD_PASS;
    }

}
