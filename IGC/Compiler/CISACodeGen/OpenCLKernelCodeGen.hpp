/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CISACodeGen/ComputeShaderBase.hpp"
#include "Compiler/CISACodeGen/OpenCLOptions.hpp"

namespace IGC
{
    class KernelArg;
}

namespace IGC
{
    class OpenCLProgramContext : public CodeGenContext
    {
    public:
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
        SComputeShaderWalkOrder m_walkOrderStruct;

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

        }

        bool enableZEBinary() const override { return true; }
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
        bool isBufferBoundsChecking() const override;
        void failOnSpills();
        bool needsDivergentBarrierHandling() const;
        unsigned GetSlmSizePerSubslice();
        float GetSpillThreshold(SIMDMode dispatchSize);
        bool isAutoGRFSelectionEnabled() const override;
        uint64_t getMinimumValidAddress() const override;

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
        COpenCLKernel(const COpenCLKernel&) = delete;
        COpenCLKernel& operator=(const COpenCLKernel&) = delete;

        void PreCompile() override;
        void AllocatePayload() override;
        void ParseShaderSpecificOpcode(llvm::Instruction* inst) override;
        void ExtractGlobalVariables() override {}

        bool        hasReadWriteImage(llvm::Function& F) override;
        bool        CompileSIMDSize(SIMDMode simdMode, EmitPass& EP, llvm::Function& F) override;

        SIMDStatus  checkSIMDCompileConds(SIMDMode simdMode, EmitPass& EP, llvm::Function& F, bool hasSyncRTCalls);
        SIMDStatus  checkSIMDCompileCondsForMin16(SIMDMode simdMode, EmitPass& EP, llvm::Function& F, bool hasSyncRTCalls);

        bool IsRegularGRFRequested() override;
        bool IsLargeGRFRequested() override;
        int getAnnotatedNumThreads() override;
        void FillKernel(SIMDMode simdMode);

        // Recomputes the binding table layout according to the present kernel args
        void RecomputeBTLayout();

        bool HasFullDispatchMask() override;

        // Returns the immediate value mapped to GlobalVariable c.
        // (GlobalVariables represent the pointer to the global,
        // which is a compile-time constant)
        unsigned int GetSLMMappingValue(llvm::Value *c) override;
        CVariable *GetSLMMapping(llvm::Value *c) override;

        const SOpenCLKernelInfo& getKernelInfo() const { return m_kernelInfo; }

        static bool IsValidShader(COpenCLKernel* shader);
        static bool IsVisaCompiledSuccessfullyForShader(COpenCLKernel* shader);
        static bool IsVisaCompileStatusFailureForShader(COpenCLKernel *shader);

        GenericShaderState m_State;

    public:
        SOpenCLProgramInfo* m_programInfo;
        SOpenCLKernelInfo m_kernelInfo;
        SOpenCLKernelCostExpInfo m_kernelCostexpInfo;

        unsigned int m_perWIStatelessPrivateMemSize;

        bool GetDisableMidThreadPreemption() const { return m_disableMidThreadPreemption; }
        void SetDisableMidthreadPreemption() { m_disableMidThreadPreemption = true; }
        bool passNOSInlineData() override;
        bool loadThreadPayload() override;

    protected:
        // keep track of the pointer arguments' addrspace and access_type for
        // setting the correct attributes to their corresponding bindless offset arguments
        typedef std::tuple<zebin::PreDefinedAttrGetter::ArgAddrSpace,
                           zebin::PreDefinedAttrGetter::ArgAccessType,
                           zebin::PreDefinedAttrGetter::ArgType> PtrArgAttrType;
        typedef std::map<uint32_t, PtrArgAttrType> PtrArgsAttrMapType;

    protected:
        // Fill SOpenCLKernelInfo::m_zePayloadArgs
        // Return true: if the argument is supported in ZEBinary and it's created successfully
        // Return false: if the argument cannot be supported by ZEBinary
        bool CreateZEPayloadArguments(
            IGC::KernelArg* kernelArg, uint payloadPosition, PtrArgsAttrMapType& ptrArgsAttrMap);

        // Fill SOpenCLKernelInfo::m_zeUserAttribute for ZEBinary
        void FillZEUserAttributes(IGC::IGCMD::FunctionInfoMetaDataHandle& funcInfoMD);

        // Fill SOpenCLKernelInfo::m_zeKernelArgInfo for ZEBinary
        void FillZEKernelArgInfo();

        // a helper function to get image type from kernelArg
        iOpenCL::IMAGE_MEMORY_OBJECT_TYPE getImageTypeFromKernelArg(const KernelArg& kernelArg);

        // a helper function to get sampler type from kernelArg
        iOpenCL::SAMPLER_OBJECT_TYPE getSamplerTypeFromKernelArg(const KernelArg& kernelArg);

        // Creates annotations for inline sampler_t objects
        void CreateZEInlineSamplerAnnotations();

        // A helper function to get vector type hint string for filling user attributes
        std::string getVecTypeHintTypeString(const IGC::IGCMD::VectorTypeHintMetaDataHandle& vecTypeHintInfo) const;

        // Create annotation for printf strings.
        void CreatePrintfStringAnnotations();

        // Load from MD and return the resource information for argument number argNo
        SOpenCLKernelInfo::SResourceInfo getResourceInfo(int argNo);

        // Load from MD and return the resource extension information for argument number argNo
        ResourceExtensionTypeEnum getExtensionInfo(int argNo);

        // Resolve the binding table index for resource resInfo (using the BTL)
        unsigned int getBTI(SOpenCLKernelInfo::SResourceInfo& resInfo);

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
        // Helper function to get SIMD size specified in intel_reqd_sub_group_size attribute
        uint32_t getReqdSubGroupSize(llvm::Function& F, IGC::IGCMD::MetaDataUtils* MDUtils) const;
        uint32_t getMaxPressure(llvm::Function& F, IGC::IGCMD::MetaDataUtils* MDUtils) const;
        bool isUnusedArg(KernelArg& arg) const;
    };

    void CodeGen(OpenCLProgramContext* ctx);
}
