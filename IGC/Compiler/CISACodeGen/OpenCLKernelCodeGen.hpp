/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CISACodeGen/ComputeShaderBase.hpp"

namespace IGC
{
    class KernelArg;
}

namespace IGC
{
    class OpenCLProgramContext : public CodeGenContext
    {
    public:
        // We should probably replace all of this with proper option parsing,
        // like RS does
        class InternalOptions
        {
        public:
            InternalOptions(const TC::STB_TranslateInputArgs* pInputArgs) :
                KernelDebugEnable(false),
                IncludeSIPCSR(false),
                IncludeSIPKernelDebug(false),
                IntelGreaterThan4GBBufferRequired(false),
                Use32BitPtrArith(false),
                IncludeSIPKernelDebugWithLocalMemory(false),
                IntelHasPositivePointerOffset(false),
                IntelHasBufferOffsetArg(false),
                IntelBufferOffsetArgOptional(true),
                IntelHasSubDWAlignedPtrArg(false),
                LargeGRFKernels(),
                RegularGRFKernels()
            {
                if (pInputArgs == nullptr)
                    return;

                if (pInputArgs->pInternalOptions != nullptr)
                {
                    parseOptions(pInputArgs->pInternalOptions);
                }

                // Internal options are passed in via pOptions as well.
                if (pInputArgs->pOptions != nullptr)
                {
                    parseOptions(pInputArgs->pOptions);
                }
            }

            bool KernelDebugEnable;
            bool IncludeSIPCSR;
            bool IncludeSIPKernelDebug;
            bool IntelGreaterThan4GBBufferRequired;
            bool IntelDisableA64WA = false;
            bool IntelForceEnableA64WA = false;
            bool Use32BitPtrArith = false;
            bool IncludeSIPKernelDebugWithLocalMemory;

            bool GTPinReRA = false;
            bool GTPinGRFInfo = false;
            bool GTPinScratchAreaSize = false;
            bool GTPinIndirRef = false;
            uint32_t GTPinScratchAreaSizeValue = 0;

            // stateless to stateful optimization
            bool IntelHasPositivePointerOffset; // default: false
            bool IntelHasBufferOffsetArg;       // default: false
            bool IntelBufferOffsetArgOptional;  // default: true
            bool IntelHasSubDWAlignedPtrArg;
            // default: false, meaning kernel's sub-DW ptrArgs (char*, short*) are DW-aligned.
            // This default is stronger than the natural alignment implied by char*/short*. But
            // for historical reason, we have this.

            bool replaceGlobalOffsetsByZero = false;
            bool IntelEnablePreRAScheduling = true;
            bool PromoteStatelessToBindless = false;
            bool PreferBindlessImages = false;
            bool UseBindlessMode = false;
            bool UseBindlessPrintf = false;
            bool UseBindlessLegacyMode = true;
            bool ExcludeIRFromZEBinary = false;
            bool EmitZeBinVISASections = false;
            bool NoSpill = false;
            bool DisableNoMaskWA = false;
            bool IgnoreBFRounding = false;   // If true, ignore BFloat rounding when folding bf operations
            bool CompileOneKernelAtTime = false;

            // Generic address related
            bool NoLocalToGeneric = false;
            bool ForceGlobalMemoryAllocation = false;

            // -1 : initial value that means it is not set from cmdline
            // 0-5: valid values set from the cmdline
            int16_t VectorCoalescingControl = -1;

            bool Intel128GRFPerThread = false;
            bool Intel256GRFPerThread = false;
            bool IntelNumThreadPerEU = false;
            int32_t numThreadsPerEU = -1;
            bool IntelExpGRFSize = false;
            uint32_t expGRFSize = 0;
            std::vector<std::string> LargeGRFKernels;
            std::vector<std::string> RegularGRFKernels;
            // Enable compiler heuristics ("regSharingHeuristics" in VISA) for large GRF selection.
            bool IntelEnableAutoLargeGRF = false;

            // IntelForceInt32DivRemEmu is used only if fp64 is supported natively.
            // IntelForceInt32DivRemEmu wins if both are set and can be applied.
            bool IntelForceInt32DivRemEmu = false;
            bool IntelForceInt32DivRemEmuSP = false;
            bool IntelForceDisable4GBBuffer = false;
            // user-controled option to disable EU Fusion
            bool DisableEUFusion = false;
            // Function Control (same as IGC key FunctionControl)
            int FunctionControl = -1;
            // Fail comilation if spills are present in compiled kernel
            bool FailOnSpill = false;
            // This option forces IGC to poison kernels using fp64
            // operations on platforms without HW support for fp64.
            bool EnableUnsupportedFP64Poisoning = false;
            // This option enables FP64 emulation for platforms that
            // cannot HW support for double operations
            bool EnableFP64GenEmu = false;
            // Cache default. -1 menans not set (thus not used by igc);
            // Valid values are defined as enum type LSC_L1_L3_CC in
            //   visa\include\visa_igc_common_header.h, which are from
            //   macro definitions in igc\common\igc_regkeys_enums_defs.h
            int StoreCacheDefault = -1;
            int LoadCacheDefault = -1;
            // Force high-accuracy math functions from BiFModule
            bool UseHighAccuracyMathFuncs = false;

            bool AllowRelocAdd = true;

            uint32_t IntelPrivateMemoryMinimalSizePerThread = 0;
            uint32_t IntelScratchSpacePrivateMemoryMinimalSizePerThread = 0;

            bool EnableDivergentBarrierHandling = false;
            std::optional<bool> EnableZEBinary;

            // Compile only up to vISA stage.
            bool EmitVisaOnly = false;

        private:
            void parseOptions(const char* IntOptStr);
        };

        class Options
        {
        public:
            Options(const TC::STB_TranslateInputArgs* pInputArgs) :
                CorrectlyRoundedSqrt(false),
                NoSubgroupIFP(false),
                UniformWGS(false)
            {
                if (pInputArgs == nullptr)
                    return;

                if (pInputArgs->pOptions == nullptr)
                    return;

                // Build options are of the form -cl-xxxx and -ze-xxxx
                // So we skip these prefixes when reading the options to be agnostic of their source

                // Runtime passes internal options via pOptions as well, and those
                // internal options will be handled by InternalOptions class (parseOptions).
                // !!! When adding a new internal option, please add it into internalOptions class!!!
                // (Might combine both Options and InternalOptions into a single class!)
                const char* options = pInputArgs->pOptions;
                if (strstr(options, "-fp32-correctly-rounded-divide-sqrt"))
                {
                    CorrectlyRoundedSqrt = true;
                }

                if (strstr(options, "-no-subgroup-ifp"))
                {
                    NoSubgroupIFP = true;
                }

                if (strstr(options, "-uniform-work-group-size"))
                {
                    // Note that this is only available for -cl-std >= 2.0.
                    // This will be checked before we place this into the
                    // the module metadata.
                    UniformWGS = true;
                }
                if (strstr(options, "-take-global-address"))
                {
                    EnableTakeGlobalAddress = true;
                }
                if (strstr(options, "-library-compilation"))
                {
                    IsLibraryCompilation = true;
                }
                if (strstr(options, "-emit-lib-compile-errors"))
                {
                    EmitErrorsForLibCompilation = true;
                }
                if (const char* op = strstr(options, "-intel-reqd-eu-thread-count"))
                {
                    IntelRequiredEUThreadCount = true;
                    // Take an integer value after this option
                    // atoi(..) ignores leading white spaces and characters after the actual number
                    requiredEUThreadCount = atoi(op + strlen("-intel-reqd-eu-thread-count="));
                }
            }

            bool CorrectlyRoundedSqrt;
            bool NoSubgroupIFP;
            bool UniformWGS;
            bool EnableTakeGlobalAddress = false;
            bool IsLibraryCompilation = false;
            bool IntelRequiredEUThreadCount = false;
            bool EmitErrorsForLibCompilation = false;
            uint32_t requiredEUThreadCount = 0;
        };

        // output: shader information
        iOpenCL::CGen8OpenCLProgram m_programOutput;
        SOpenCLProgramInfo m_programInfo;
        const InternalOptions m_InternalOptions;
        const Options m_Options;
        bool isSpirV;
        float m_ProfilingTimerResolution = 0.0f;
        bool m_ShouldUseNonCoherentStatelessBTI;
        uint32_t m_numUAVs = 0;

    private:
        bool m_enableZEBinary;

        // To minimize negative performance implications caused by a dynamic generic address
        // space resolution, private memory can be allocated in the same address space as
        // global memory. It gives a possibility to treat private memory operations as global
        // memory operations, so there is no necessity to distinguish between them.
        // However, when a module uses `to_global` or `to_private` OpenCL builtins, differentiating
        // between private and global pointer is necessary to preserve conformity.
        // Below flag is set to true when IGC detects that any of these builtins is called in
        // a module and could not be resolved statically at compile time.
        bool m_mustDistinguishBetweenPrivateAndGlobalPtr = false;

    public:
        // Additional text visaasm to link.
        std::vector<const char*> m_VISAAsmToLink;
        // Functions that are forced to be direct calls.
        std::unordered_set<std::string> m_DirectCallFunctions;

        OpenCLProgramContext(
            const COCLBTILayout& btiLayout,
            const CPlatform& platform,
            const TC::STB_TranslateInputArgs* pInputArgs,
            const CDriverInfo& driverInfo,
            LLVMContextWrapper* llvmContext = nullptr,
            bool shouldUseNonCoherentStatelessBTI = false,
            const bool createResourceDimTypes = true)
            : CodeGenContext(ShaderType::OPENCL_SHADER, btiLayout, platform, driverInfo, createResourceDimTypes, llvmContext),
            m_programOutput(platform.getPlatformInfo(), *this),
            m_InternalOptions(pInputArgs),
            m_Options(pInputArgs),
            isSpirV(false),
            m_ShouldUseNonCoherentStatelessBTI(shouldUseNonCoherentStatelessBTI)
        {
            if (pInputArgs && pInputArgs->pVISAAsmToLinkArray) {
                for (uint32_t i = 0; i < pInputArgs->NumVISAAsmsToLink; ++i) {
                    m_VISAAsmToLink.push_back(pInputArgs->pVISAAsmToLinkArray[i]);
                }
            }
            if (pInputArgs && pInputArgs->pDirectCallFunctions) {
              for (uint32_t i = 0; i < pInputArgs->NumDirectCallFunctions; ++i) {
                m_DirectCallFunctions.insert(pInputArgs->pDirectCallFunctions[i]);
              }
            }


            // Logic for native ZEBin support
            auto supportsZEBin = [&](CPlatform platformInfo)
            {
                switch (platformInfo.GetProductFamily())
                {
                default:
                    return true;
                case IGFX_BROADWELL:
                case IGFX_BROXTON:
                case IGFX_GEMINILAKE:
                case IGFX_LAKEFIELD:
                case IGFX_ELKHARTLAKE:
                case IGFX_ICELAKE:
                case IGFX_ICELAKE_LP:
                case IGFX_TIGERLAKE_LP:
                case IGFX_ROCKETLAKE:
                case IGFX_ALDERLAKE_S:
                case IGFX_ALDERLAKE_P:
                case IGFX_ALDERLAKE_N:
                    return false;
                }
            };

            if (IGC_IS_FLAG_SET(EnableZEBinary))
                m_enableZEBinary = IGC_IS_FLAG_ENABLED(EnableZEBinary);
            // TODO: Should options/internal options precede the platform
            // support status? Consider changing the priority if any
            // corresponding user scenarios get discovered.
            else if (!supportsZEBin(platform))
                m_enableZEBinary = false;
            else if (m_InternalOptions.EnableZEBinary)
                m_enableZEBinary = *m_InternalOptions.EnableZEBinary;
            else
                // Set the default value from the flag table.
                m_enableZEBinary = IGC_IS_FLAG_ENABLED(EnableZEBinary);
        }

        bool enableZEBinary() const override { return m_enableZEBinary; }
        bool isSPIRV() const;
        void setAsSPIRV();
        float getProfilingTimerResolution();
        uint32_t getNumGRFPerThread(bool returnDefault = true) override;
        int32_t getNumThreadsPerEU() const override;
        uint32_t getExpGRFSize() const override;
        bool forceGlobalMemoryAllocation() const override;
        bool allocatePrivateAsGlobalBuffer() const override;
        bool noLocalToGenericOptionEnabled() const override;
        bool mustDistinguishBetweenPrivateAndGlobalPtr() const override;
        void setDistinguishBetweenPrivateAndGlobalPtr(bool);
        bool enableTakeGlobalAddress() const override;
        int16_t getVectorCoalescingControl() const override;
        uint32_t getPrivateMemoryMinimalSizePerThread() const override;
        uint32_t getIntelScratchSpacePrivateMemoryMinimalSizePerThread() const override;
        void failOnSpills();
        bool needsDivergentBarrierHandling() const;
        unsigned GetSlmSizePerSubslice();
        float GetSpillThreshold(SIMDMode dispatchSize);

        void clearBeforeRetry() {
            m_programOutput.clearBeforeRetry();
        }
    private:
        llvm::DenseMap<llvm::Function*, std::string> m_hashes_per_kernel;
    };

    class COpenCLKernel : public CComputeShaderBase
    {
    public:
        friend class CShaderProgram;
        COpenCLKernel(OpenCLProgramContext* ctx, llvm::Function*, CShaderProgram* pProgram);
        ~COpenCLKernel();

        void PreCompile() override;
        void AllocatePayload() override;
        void ParseShaderSpecificOpcode(llvm::Instruction* inst) override;
        void ExtractGlobalVariables() override {}

        bool        hasReadWriteImage(llvm::Function& F) override;
        bool        CompileSIMDSize(SIMDMode simdMode, EmitPass& EP, llvm::Function& F) override;

        SIMDStatus  checkSIMDCompileConds(SIMDMode simdMode, EmitPass& EP, llvm::Function& F, bool hasSyncRTCalls);
        SIMDStatus  checkSIMDCompileCondsPVC(SIMDMode simdMode, EmitPass& EP, llvm::Function& F, bool hasSyncRTCalls);

        bool IsRegularGRFRequested() override;
        bool IsLargeGRFRequested() override;
        int getAnnotatedNumThreads() override;
        void FillKernel(SIMDMode simdMode);

        // Recomputes the binding table layout according to the present kernel args
        void RecomputeBTLayout();

        // Set m_HasTID to true if TID functions were found
        void SetHasTID();

        // Set m_HasGlobalSize to true if TID functions were found
        void SetHasGlobalSize();

        bool HasFullDispatchMask() override;

        // Returns the immediate value mapped to GlobalVariable c.
        // (GlobalVariables represent the pointer to the global,
        // which is a compile-time constant)
        unsigned int GetGlobalMappingValue(llvm::Value* c) override;
        CVariable* GetGlobalMapping(llvm::Value* c) override;

        const SOpenCLKernelInfo& getKernelInfo() const { return m_kernelInfo; }

        static bool IsValidShader(COpenCLKernel* shader);
        static bool IsVisaCompiledSuccessfullyForShader(COpenCLKernel* shader);
        static bool IsVisaCompileStatusFailureForShader(COpenCLKernel *shader);

    public:
        SOpenCLProgramInfo* m_programInfo;
        SOpenCLKernelInfo m_kernelInfo;

        unsigned int m_perWIStatelessPrivateMemSize;

        bool GetDisableMidThreadPreemption() const { return m_disableMidThreadPreemption; }
        void SetDisableMidthreadPreemption() { m_disableMidThreadPreemption = true; }
        bool passNOSInlineData() override;
        bool loadThreadPayload() override;

    protected:
        // keep track of the pointer arguments' addrspace and access_type for
        // setting the correct attributes to their corresponding bindless offset arguments
        typedef std::pair<zebin::PreDefinedAttrGetter::ArgAddrSpace,
                          zebin::PreDefinedAttrGetter::ArgAccessType> PtrArgAttrType;
        typedef std::map<uint32_t, PtrArgAttrType> PtrArgsAttrMapType;

    protected:
        // Creates appropriate annotation based on the kernel arg
        void CreateAnnotations(IGC::KernelArg* kernelArg, uint payloadPosition);

        // Fill SOpenCLKernelInfo::m_zePayloadArgs
        // Return true: if the argument is supported in ZEBinary and it's created successfully
        // Return false: if the argument cannot be supported by ZEBinary
        bool CreateZEPayloadArguments(
            IGC::KernelArg* kernelArg, uint payloadPosition, PtrArgsAttrMapType& ptrArgsAttrMap);

        // Fill SOpenCLKernelInfo::m_zeUserAttribute for ZEBinary
        // (PT pass: CreateKernelAttributeInfo)
        void FillZEUserAttributes(IGC::IGCMD::FunctionInfoMetaDataHandle& funcInfoMD);

        // Fill SOpenCLKernelInfo::m_zeKernelArgInfo for ZEBinary
        // (PT pass: CreateKernelArgInfo)
        void FillZEKernelArgInfo();

        // a helper function to get image type from kernelArg
        iOpenCL::IMAGE_MEMORY_OBJECT_TYPE getImageTypeFromKernelArg(const KernelArg& kernelArg);

        // a helper function to get sampler type from kernelArg
        iOpenCL::SAMPLER_OBJECT_TYPE getSamplerTypeFromKernelArg(const KernelArg& kernelArg);

        // Creates annotations for inline sampler_t objects
        void CreateInlineSamplerAnnotations();
        void CreateZEInlineSamplerAnnotations();

        // Creates annotations for kernel argument information (kernel reflection)
        void CreateKernelArgInfo();

        // Creates annotations for kernel attribution information (kernel reflection)
        void CreateKernelAttributeInfo();
        std::string getVecTypeHintString(const IGC::IGCMD::VectorTypeHintMetaDataHandle& vecTypeHintInfo) const;
        std::string getVecTypeHintTypeString(const IGC::IGCMD::VectorTypeHintMetaDataHandle& vecTypeHintInfo) const;
        std::string getThreadGroupSizeString(IGC::IGCMD::ThreadGroupSizeMetaDataHandle& threadGroupSize, bool isHint);
        std::string getSubGroupSizeString(IGC::IGCMD::SubGroupSizeMetaDataHandle& subGroupSize);
        std::string getWorkgroupWalkOrderString(const IGC::WorkGroupWalkOrderMD& workgroupWalkOrder);
        // Create annotation for printf strings.
        void CreatePrintfStringAnnotations();

        // Load from MD and return the resource information for argument number argNo
        SOpenCLKernelInfo::SResourceInfo getResourceInfo(int argNo);

        // Load from MD and return the resource extension information for argument number argNo
        ResourceExtensionTypeEnum getExtensionInfo(int argNo);

        // Resolve the binding table index for resource resInfo (using the BTL)
        unsigned int getBTI(SOpenCLKernelInfo::SResourceInfo& resInfo);

        bool hasStatefulAccess(unsigned bti);

        // Find the sum of inline local sizes used by this kernel
        unsigned int getSumFixedTGSMSizes(llvm::Function* F);

        bool m_HasTID;
        bool m_HasGlobalSize;
        bool m_disableMidThreadPreemption;
        bool m_largeGRFRequested;
        bool m_regularGRFRequested;
        int m_annotatedNumThreads;

        // Maps GlobalVariables representing local address-space pointers
        // to their offsets in SLM.
        std::map<llvm::Value*, unsigned int> m_localOffsetsMap;

        OpenCLProgramContext* m_Context;

        void ClearKernelInfo();
    private:
        WorkGroupWalkOrderMD getWorkGroupWalkOrder();
        void tryHWGenerateLocalIDs();
        // helper functions for collecting kernel argument info
        // Format the strings the way the OpenCL runtime expects them
        std::string getKernelArgTypeName(const FunctionMetaData& funcMD, uint argIndex) const;
        std::string getKernelArgTypeQualifier(const FunctionMetaData& funcMD, uint argIndex) const;
        std::string getKernelArgAddressQualifier(const FunctionMetaData& funcMD, uint argIndex) const;
        std::string getKernelArgAccessQualifier(const FunctionMetaData& funcMD, uint argIndex) const;
    };

    void CodeGen(OpenCLProgramContext* ctx);
}
