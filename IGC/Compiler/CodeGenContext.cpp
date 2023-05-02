/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <sstream>
#include <iomanip>
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/ScaledNumber.h>
#include <llvm/Demangle/Demangle.h>
#include <llvm/IR/DebugInfo.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Probe/Assertion.h"

namespace IGC
{
    struct RetryState
    {
        bool allowLICM;
        bool allowCodeSinking;
        bool allowAddressArithmeticSinking;
        bool allowSimd32Slicing;
        bool allowPromotePrivateMemory;
        bool allowPreRAScheduler;
        bool allowVISAPreRAScheduler;
        bool allowLargeURBWrite;
        bool allowConstantCoalescing;
        bool allowLargeGRF;
        bool allowLoadSinking;
        unsigned nextState;
    };

    static const RetryState RetryTable[] = {
        // licm  codSk AdrSk  Slice  PrivM  PreRA  VISAP  URBWr  Coals  GRF    loadSk
        { true,  true, false, false, true,  true,  true,  true,  true,  false, false, 1 },
        { false, true, true,  true,  false, false, false, false, false, true,  true, 500 }
    };

    static constexpr size_t RetryTableSize = sizeof(RetryTable) / sizeof(RetryState);

    RetryManager::RetryManager() : enabled(false), perKernel(false)
    {
        firstStateId = IGC_GET_FLAG_VALUE(RetryManagerFirstStateId);
        stateId = firstStateId;
        prevStateId = 500;
        IGC_ASSERT(stateId < RetryTableSize);
    }

    bool RetryManager::AdvanceState()
    {
        if (!enabled || IGC_IS_FLAG_ENABLED(DisableRecompilation))
        {
            return false;
        }
        IGC_ASSERT(stateId < RetryTableSize);
        prevStateId = stateId;
        stateId = RetryTable[stateId].nextState;
        return (stateId < RetryTableSize);
    }

    unsigned RetryManager::GetPerFuncRetryStateId(Function* F) const
    {
        if (IGC_GET_FLAG_VALUE(AllowStackCallRetry) == 2 &&
            F != nullptr &&
            prevStateId < RetryTableSize &&
            !PerFuncRetrySet.empty())
        {
            std::string FName = StripCloneName(F->getName().str());
            return (PerFuncRetrySet.count(FName) != 0) ? stateId : prevStateId;
        }
        return stateId;
    }

    bool RetryManager::AllowLICM(Function* F) const
    {
        unsigned id = GetPerFuncRetryStateId(F);
        IGC_ASSERT(id < RetryTableSize);
        // Since we currently can't enable/disable LICM per-function, enabling LICM
        // when retrying for stackcalls seems to give better performance. So always
        // enable when recompiling with stackcalls.
        return RetryTable[id].allowLICM || !PerFuncRetrySet.empty();
    }

    bool RetryManager::AllowAddressArithmeticSinking(Function* F) const
    {
        unsigned id = GetPerFuncRetryStateId(F);
        IGC_ASSERT(id < RetryTableSize);
        return RetryTable[id].allowAddressArithmeticSinking;
    }

    bool RetryManager::AllowPromotePrivateMemory(Function* F) const
    {
        unsigned id = GetPerFuncRetryStateId(F);
        IGC_ASSERT(id < RetryTableSize);
        return RetryTable[id].allowPromotePrivateMemory;
    }

    bool RetryManager::AllowPreRAScheduler(Function* F) const
    {
        unsigned id = GetPerFuncRetryStateId(F);
        IGC_ASSERT(id < RetryTableSize);
        return RetryTable[id].allowPreRAScheduler;
    }

    bool RetryManager::AllowVISAPreRAScheduler(Function* F) const
    {
        unsigned id = GetPerFuncRetryStateId(F);
        IGC_ASSERT(id < RetryTableSize);
        return RetryTable[id].allowVISAPreRAScheduler;
    }

    bool RetryManager::AllowCodeSinking(Function* F) const
    {
        unsigned id = GetPerFuncRetryStateId(F);
        IGC_ASSERT(id < RetryTableSize);
        return RetryTable[id].allowCodeSinking;
    }

    bool RetryManager::AllowSimd32Slicing(Function* F) const
    {
        unsigned id = GetPerFuncRetryStateId(F);
        IGC_ASSERT(id < RetryTableSize);
        return RetryTable[id].allowSimd32Slicing;
    }

    bool RetryManager::AllowLargeURBWrite(Function* F) const
    {
        unsigned id = GetPerFuncRetryStateId(F);
        IGC_ASSERT(id < RetryTableSize);
        return RetryTable[id].allowLargeURBWrite;
    }

    bool RetryManager::AllowConstantCoalescing(Function* F) const
    {
        unsigned id = GetPerFuncRetryStateId(F);
        IGC_ASSERT(id < RetryTableSize);
        return RetryTable[id].allowConstantCoalescing;
    }

    bool RetryManager::AllowLargeGRF(Function* F) const
    {
        unsigned id = GetPerFuncRetryStateId(F);
        IGC_ASSERT(id < RetryTableSize);
        return RetryTable[id].allowLargeGRF;
    }

    bool RetryManager::AllowLoadSinking(Function* F) const
    {
        unsigned id = GetPerFuncRetryStateId(F);
        IGC_ASSERT(id < RetryTableSize);
        return RetryTable[id].allowLoadSinking;
    }

    void RetryManager::SetFirstStateId(int id)
    {
        firstStateId = id;
    }

    bool RetryManager::IsFirstTry() const
    {
        return (stateId == firstStateId);
    }

    bool RetryManager::IsLastTry() const
    {
        return (!enabled ||
            IGC_IS_FLAG_ENABLED(DisableRecompilation) ||
            lastSpillSize < IGC_GET_FLAG_VALUE(AllowedSpillRegCount) ||
            (stateId < RetryTableSize && RetryTable[stateId].nextState >= RetryTableSize));
    }

    unsigned RetryManager::GetRetryId() const
    {
        return stateId;
    }

    void RetryManager::Enable()
    {
        enabled = true;
    }

    void RetryManager::Disable()
    {
        if (!perKernel)
        {
            enabled = false;
        }
    }

    void RetryManager::SetSpillSize(unsigned int spillSize)
    {
        lastSpillSize = spillSize;
    }

    unsigned int RetryManager::GetLastSpillSize() const
    {
        return lastSpillSize;
    }

    void RetryManager::ClearSpillParams()
    {
        lastSpillSize = 0;
        numInstructions = 0;
    }

    void RetryManager::Collect(CShaderProgram::UPtr pCurrent)
    {
        // Get previous kernel name
        std::string funcName = pCurrent->getLLVMFunction()->getName().str();
        // Delete/unattach from memory previous version of this kernel
        // and attach new version
        previousKernels[funcName] = std::move(pCurrent);
    }

    CShaderProgram* RetryManager::GetPrevious(
        CShaderProgram* pCurrent,
        bool ReleaseUPtr)
    {
        std::string funcName = pCurrent->getLLVMFunction()->getName().str();
        if (previousKernels.find(funcName) !=
            previousKernels.end())
        {
            if (ReleaseUPtr)
            {
                auto ptr = previousKernels[funcName].release();
                previousKernels.erase(funcName);
                return ptr;
            }
            else
            {
                return previousKernels[funcName].get();
            }
        }
        return nullptr;
    }

    bool RetryManager::IsBetterThanPrevious(CShaderProgram* pCurrent)
    {
        bool isBetter = true;
        auto pPrevious = GetPrevious(pCurrent);
        if (pPrevious)
        {
            auto simdToAnalysis = { SIMDMode::SIMD32, SIMDMode::SIMD16, SIMDMode::SIMD8 };

            CShader* previousShader = nullptr;
            CShader* currentShader = nullptr;

            // Get shaders
            for (auto simd : simdToAnalysis)
            {
                if (!previousShader &&
                    (pPrevious->GetShader(simd)->ProgramOutput()->m_programSize > 0))
                {
                    previousShader = pPrevious->GetShader(simd);
                }
                if (!currentShader &&
                    (pCurrent->GetShader(simd)->ProgramOutput()->m_programSize > 0))
                {
                    currentShader = pCurrent->GetShader(simd);
                }
            }

            // Check if current shader has more than 200% of previous spill size
            bool spillSizeBigger =
                currentShader->m_spillSize > previousShader->m_spillSize << 1;

            if (spillSizeBigger)
            {
                // The previous shader was better, ignore any future retry compilation
                isBetter = false;
            }
        }
        return isBetter;
    }

    // save entry for given SIMD mode, to avoid recompile for next retry.
    void RetryManager::SaveSIMDEntry(SIMDMode simdMode, CShader* shader)
    {
        auto entry = GetCacheEntry(simdMode);
        IGC_ASSERT(entry);
        if (entry)
        {
            entry->shader = shader;
        }
    }

    CShader* RetryManager::GetSIMDEntry(SIMDMode simdMode)
    {
        auto entry = GetCacheEntry(simdMode);
        IGC_ASSERT(entry);
        return entry ? entry->shader : nullptr;
    }

    RetryManager::~RetryManager()
    {
        for (auto& it : cache)
        {
            if (it.shader)
            {
                delete it.shader;
            }
        }
    }

    bool RetryManager::AnyKernelSpills() const
    {
        return std::any_of(std::begin(cache), std::end(cache), [](const CacheEntry& entry) {
            return entry.shader && entry.shader->m_spillCost > 0.0;
        });
    }

    bool RetryManager::PickupKernels(CodeGenContext* cgCtx)
    {
        {
            IGC_ASSERT_MESSAGE(0, "TODO for other shader types");
            return true;
        }
    }


    RetryManager::CacheEntry* RetryManager::GetCacheEntry(SIMDMode simdMode)
    {
        auto result = std::find_if(std::begin(cache), std::end(cache), [&simdMode](const CacheEntry& entry) {
            return entry.simdMode == simdMode;
        });
        return result != std::end(cache) ? result : nullptr;
    }

    LLVMContextWrapper::LLVMContextWrapper(bool createResourceDimTypes)
    {
        if (createResourceDimTypes)
        {
            CreateResourceDimensionTypes(*this);
        }
#ifdef __IGC_OPAQUE_POINTERS_FORCE_DISABLED__
        this->setOpaquePointers(false);
#endif
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
            {
                pointerSizeInRegister = 32;
            }
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

    // check if DP emu is required
    void CodeGenContext::checkDPEmulationEnabled()
    {
        if ((IGC_IS_FLAG_ENABLED(ForceDPEmulation) ||
            (m_DriverInfo.NeedFP64(platform.getPlatformInfo().eProductFamily) && platform.hasNoFP64Inst())) ||
            (getCompilerOption().FP64GenEmulationEnabled && platform.emulateFP64ForPlatformsWithoutHWSupport()))
        {
            m_hasDPEmu = true;
        }
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

    bool CodeGenContext::HasFuncExpensiveLoop(llvm::Function* pFunc)
    {
        if (m_FuncHasExpensiveLoops.find(pFunc) !=
            m_FuncHasExpensiveLoops.end())
        {
            return m_FuncHasExpensiveLoops[pFunc];
        }
        return false;
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

    int32_t CodeGenContext::getNumThreadsPerEU() const
    {
        return -1;
    }

    uint32_t CodeGenContext::getExpGRFSize() const
    {
        return 0;
    }

    bool CodeGenContext::enableZEBinary() const
    {
        return false;
    }

    /// parameter "returnDefault" controls what to return when
    /// there is no user-forced setting
    uint32_t CodeGenContext::getNumGRFPerThread(bool returnDefault)
    {
        constexpr uint32_t DEFAULT_TOTAL_GRF_NUM = 128;

        if (m_NumGRFPerThread)
            return m_NumGRFPerThread;

        if (IGC_GET_FLAG_VALUE(TotalGRFNum) != 0)
        {
            m_NumGRFPerThread = IGC_GET_FLAG_VALUE(TotalGRFNum);
            return m_NumGRFPerThread;
        }
        if (getModuleMetaData()->csInfo.forceTotalGRFNum != 0)
        {
            {
                m_NumGRFPerThread = getModuleMetaData()->csInfo.forceTotalGRFNum;
                return m_NumGRFPerThread;
            }
        }


        // read value from CompOptions first
        DWORD GRFNum4RQToUse = getModuleMetaData()->compOpt.ForceLargeGRFNum4RQ ? 0x100 : 0;

        // override if reg key value is set
        GRFNum4RQToUse = IGC_IS_FLAG_ENABLED( TotalGRFNum4RQ ) ? IGC_GET_FLAG_VALUE( TotalGRFNum4RQ ) : GRFNum4RQToUse;

        if (hasSyncRTCalls() && (this->type != ShaderType::RAYTRACING_SHADER) && GRFNum4RQToUse != 0)
        {
            m_NumGRFPerThread = GRFNum4RQToUse;
            return m_NumGRFPerThread;
        }
        if (this->type == ShaderType::COMPUTE_SHADER && IGC_GET_FLAG_VALUE(TotalGRFNum4CS) != 0)
        {
            m_NumGRFPerThread = IGC_GET_FLAG_VALUE(TotalGRFNum4CS);
            return m_NumGRFPerThread;
        }
        return (returnDefault ? DEFAULT_TOTAL_GRF_NUM : 0);
    }

    bool CodeGenContext::forceGlobalMemoryAllocation() const
    {
        return false;
    }

    bool CodeGenContext::allocatePrivateAsGlobalBuffer() const
    {
        return false;
    }

    bool CodeGenContext::noLocalToGenericOptionEnabled() const
    {
        return false;
    }

    bool CodeGenContext::mustDistinguishBetweenPrivateAndGlobalPtr() const
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
    }

    // [used by shader dump] create unqiue id, starting from 1, for each
    // entry function.
    // Each entry function has 1-1 map b/w its name and its dump name.
    void CodeGenContext::createFunctionIDs()
    {
        if (IGC_IS_FLAG_ENABLED(DumpUseShorterName) &&
            m_functionIDs.empty() &&  module != nullptr && m_pMdUtils != nullptr)
        {
            int id = 0;
            for (auto i = m_pMdUtils->begin_FunctionsInfo(), e = m_pMdUtils->end_FunctionsInfo(); i != e; ++i)
            {
                Function* pFunc = i->first;
                // Skip non-entry functions.
                if (!isEntryFunc(m_pMdUtils, pFunc))
                {
                    continue;
                }

                // Use kernel name so that it is created once and will work for both the first and retry.
                StringRef kernelName = pFunc->getName();
                if (!kernelName.empty()) {
                    // entry without name will not be in the map (getFunctionID() will
                    // return 0. Thus, ID here starts from 1.
                    m_functionIDs[kernelName.str()] = ++id;
                }
            }
            m_enableDumpUseShorterName = true;
        }
    }

    // getFunctionDumpName() is invoked if DumpUseShorterName is set.
    // Expect to be used for any shader (aka entry function).
    std::string CodeGenContext::getFunctionDumpName(int functionId)
    {
        IGC_ASSERT(IGC_IS_FLAG_ENABLED(DumpUseShorterName));
        std::stringstream ss;
        ss << "entry_" << std::setfill('0') << std::setw(4) << functionId;
        return ss.str();
    }

    int CodeGenContext::getFunctionID(Function* F)
    {
        StringRef kernelName = F->getName();
        auto MI = m_functionIDs.find(kernelName.str());
        if (MI == m_functionIDs.end()) {
            return 0;
        }
        return MI->second;
    }
}
