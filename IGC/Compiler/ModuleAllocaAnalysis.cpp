/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ModuleAllocaAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/GenCodeGenModule.h"
#include "llvm/IR/Instructions.h"

using namespace llvm;

namespace IGC
{
ModuleAllocaAnalysis::ModuleAllocaAnalysis() : ModulePass(ID)
{
    initializeModuleAllocaAnalysisPass(*PassRegistry::getPassRegistry());
}

ModuleAllocaAnalysis::~ModuleAllocaAnalysis()
{
    for (auto I = InfoMap.begin(), E = InfoMap.end(); I != E; ++I)
        delete I->second;
}

void ModuleAllocaAnalysis::getAnalysisUsage(AnalysisUsage& AU) const
{
    AU.setPreservesAll();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
}

StringRef ModuleAllocaAnalysis::getPassName() const
{
    return "ModuleAllocaAnalysis";
}

bool ModuleAllocaAnalysis::runOnModule(Module& mod)
{
    M = &mod;
    FGA = getAnalysisIfAvailable<GenXFunctionGroupAnalysis>();
    analyze();

    return false;
}

bool ModuleAllocaAnalysis::safeToUseScratchSpace() const
{
    IGC_ASSERT(M);

    IGCMD::MetaDataUtils* pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    ModuleMetaData& modMD = *getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    CodeGenContext& Ctx = *getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    //
    // Update UseScratchSpacePrivateMemory based on WA and be consistent with
    // the implementation of CEncoder::ByteScatter().
    //
    if (Ctx.m_DriverInfo.NeedWAToTransformA32MessagesToA64()
        && Ctx.platform.getWATable().WaNoA32ByteScatteredStatelessMessages)
    {
        return false;
    }

    //
    // For now, all APIs that use scratch space for private memory, must use scratch
    // memory except OpenCL, which can also use non-scratch space. For debugging
    // purpose, a registry key is used for OCL to turn ocl-use-scratch on/off.
    //
    bool supportsScratchSpacePrivateMemory = Ctx.m_DriverInfo.supportsScratchSpacePrivateMemory();
    bool supportsStatelessSpacePrivateMemory = Ctx.m_DriverInfo.supportsStatelessSpacePrivateMemory();
    bool bOCLLegacyStatelessCheck = true;

    if (supportsScratchSpacePrivateMemory) {
        if (Ctx.type == ShaderType::OPENCL_SHADER) {
            if (Ctx.platform.hasScratchSurface() && !Ctx.m_DriverInfo.UseScratchSpaceForATSPlus()) {
                supportsScratchSpacePrivateMemory = Ctx.platform.useScratchSpaceForOCL();
                //IGC has some legacy cases where stateless private memory must be used. This flag is to remove them. If regression happens, revert it.
                bOCLLegacyStatelessCheck = !(Ctx.platform.hasScratchSurface() && IGC_IS_FLAG_ENABLED(RemoveLegacyOCLStatelessPrivateMemoryCases));
            }
        }
    }

    if (Ctx.allocatePrivateAsGlobalBuffer())
    {
        return false;
    }

    if ((modMD.compOpt.OptDisable && bOCLLegacyStatelessCheck) || !supportsScratchSpacePrivateMemory)
    {
        return false;
    }

    //
    // Do not use scratch space if module has any stack call.
    // Do not use scratch space if modeule has any variable length alloca
    //
    if (bOCLLegacyStatelessCheck) {
        if (auto * FGA = getAnalysisIfAvailable<GenXFunctionGroupAnalysis>()) {
            if (FGA->getModule() == M) {
                for (auto& I : *FGA) {
                    if (FGA->isIndirectCallGroup(I) && !I->isSingle())
                        return false;
                    if (I->hasStackCall() || I->hasVariableLengthAlloca())
                        return false;
                }
            }
        }
        else {
            // Check individual functions if FGA not available
            for (auto& F : *M) {
                if (F.hasFnAttribute("visaStackCall") || F.hasFnAttribute("hasVLA"))
                    return false;
            }
        }
    }

    const llvm::DataLayout* DL = &M->getDataLayout();

    for (auto& F : *M) {
        if (F.isDeclaration())
            continue;

        // Check each instr of this function.
        for (auto& BB : F) {
            for (auto& I : BB) {
                if (AddrSpaceCastInst * CI = dyn_cast<AddrSpaceCastInst>(&I)) {
                    // It is not safe to use scratch space as private memory if kernel does
                    // AS casting to ADDRESS_SPACE_GLOBAL_OR_PRIVATE or ADDRESS_SPACE_PRIVATE.
                    // See speical hack CI code generated at ProgramScopeConstantResolution
                    const ADDRESS_SPACE targetAS = (ADDRESS_SPACE)(cast<PointerType>(CI->getType()))->getAddressSpace();
                    if (targetAS == ADDRESS_SPACE_GLOBAL_OR_PRIVATE || targetAS == ADDRESS_SPACE_PRIVATE) {
                        return false;
                    }
                }
                if (Ctx.type == ShaderType::OPENCL_SHADER) {
                    // PtrToInt may be used to test if pointer is null, then we cannot
                    // distinguish nullptr versus zero offset. This causes a problem
                    // with an OpenCL3.0 test. see cassian/oclc_address_space_qualifiers
                    if (PtrToIntInst* IPI = dyn_cast<PtrToIntInst>(&I)) {
                        if (IPI->getPointerAddressSpace() == ADDRESS_SPACE_PRIVATE) {
                            return false;
                        }
                    }
                }
            }
        }

        if (!isEntryFunc(pMdUtils, &F))
            continue;

        //
        // OCL kernel arguments with type like queue_t and struct are expressed as
        // pointer type. Since there is no explicit AS associated with those pointers,
        // e.g., %opencl.queue_t*, to have both host and device use the same pointer
        // size for those arguments, it is better to disable the use of scratch memory.
        //
        // TODO: fixed those types (they should be in global address space)
        if (Ctx.type == ShaderType::OPENCL_SHADER && IGC_IS_FLAG_ENABLED(ForceStatelessForQueueT)) {
            if (!F.arg_empty()) {
                KernelArgs kernelArgs(F, DL, pMdUtils, &modMD, Ctx.platform.getGRFSize());
                for (auto arg : kernelArgs) {
                    const KernelArg::ArgType argTy = arg.getArgType();
                    if (argTy == KernelArg::ArgType::PTR_DEVICE_QUEUE)
                    {
                        return false;
                    }
                }
            }
        }

        //
        // Each thread has up to 2 MB scratch space to use. That is, each WI
        // has up to (2*1024*1024 / 8) bytes of scratch space in SIMD8 mode.
        //
        auto funcInfoMD = pMdUtils->getFunctionsInfoItem(&F);
        bool isGeometryStageShader = Ctx.type == ShaderType::VERTEX_SHADER ||
            Ctx.type == ShaderType::HULL_SHADER ||
            Ctx.type == ShaderType::DOMAIN_SHADER ||
            Ctx.type == ShaderType::GEOMETRY_SHADER;

        //FIXME: Below heuristics is not a clean design. Revisit this!
        //Start with simd16 or simd32 correspondingly if MinDispatchMode() is 8 or 16, which allows the medium size of space per WI
        // (simd8: largest, simd32, smallest). In doing so, there will be
        // some space left for spilling in simd8 if spilling happens.
        int32_t simd_size = isGeometryStageShader ? numLanes(Ctx.platform.getMinDispatchMode()) :
            (Ctx.platform.getMinDispatchMode() == SIMDMode::SIMD8 ? numLanes(SIMDMode::SIMD16) : numLanes(SIMDMode::SIMD32));
        const int32_t subGrpSize = funcInfoMD->getSubGroupSize()->getSIMD_size();
        if (subGrpSize > simd_size)
            simd_size = std::min(subGrpSize, static_cast<int32_t>(numLanes(SIMDMode::SIMD32)));
        int32_t groupSize = IGCMD::IGCMetaDataHelper::getThreadGroupSize(*pMdUtils, &F);
        if (groupSize == 0)
            groupSize = IGCMD::IGCMetaDataHelper::getThreadGroupSizeHint(*pMdUtils, &F);
        if (groupSize > simd_size)
            simd_size = std::min(groupSize, static_cast<int32_t>(numLanes(SIMDMode::SIMD32)));

        // if one API doesn't support stateless, we should try to use smallest dispatch mode
        // which can hold more pvt_data to avoid error out.
        if (Ctx.platform.hasScratchSurface() && Ctx.m_DriverInfo.supportsSeparatingSpillAndPrivateScratchMemorySpace() && !supportsStatelessSpacePrivateMemory)
            simd_size = numLanes(Ctx.platform.getMinDispatchMode());

        unsigned maxScratchSpaceBytes = Ctx.platform.maxPerThreadScratchSpace();
        unsigned scratchSpaceLimitPerWI = maxScratchSpaceBytes / simd_size;
        //
        // If spill happens, since the offset of scratch block rw send message
        // has only 12b, an assertion will be triggered if used scratch space
        // size >= 128 KB, here 128 KB = 2^12 * 256b.
        //
        const unsigned int totalPrivateMemPerWI = getTotalPrivateMemPerWI(&F);

        //FIXME: for now, to shrink size, let's use SIMD8 if have to.
        //later, maybe, we want to change to legacy behavior: SIMD16, to avoid potential spill.
        //but even so, when we support slot0 and slot1, then, we could still use SIMD8.
        if (Ctx.platform.hasScratchSurface() &&
            Ctx.hasSyncRTCalls() &&
            totalPrivateMemPerWI > scratchSpaceLimitPerWI) {
            simd_size = numLanes(Ctx.platform.getMinDispatchMode());
            scratchSpaceLimitPerWI = maxScratchSpaceBytes / simd_size;
        }

        if (totalPrivateMemPerWI > scratchSpaceLimitPerWI) {
            // IGC errors out when we are trying to remove statelesspvtmem of OCL (even though OCl still supports statelesspvtmem).
            // This assertion tests a scenario where (pvt_mem_usage > 256k) while statelessprivatememory is not supported.
            IGC_ASSERT_EXIT(bOCLLegacyStatelessCheck);

            if (!supportsStatelessSpacePrivateMemory)
            {
                // For XeHP_SDV and above, if any API doesn't support statelesspvtmem, error it out if we find a case where (pvt_mem > 256k).
                // This assertion found a scenario where (pvt_mem_usage > 256k) while statelessprivatememory is not supported.
                IGC_ASSERT(0);
                return true;
            }

            return false;
        }
    }

    // It is safe to use scratch space for private memory.
    return true;
}

unsigned ModuleAllocaAnalysis::getConstBufferOffset(AllocaInst* AI) const {
    IGC_ASSERT(isa<ConstantInt>(AI->getArraySize()));
    Function* F = AI->getParent()->getParent();
    return getFuncAllocaInfo(F)->AllocaDesc[AI].first;
}

unsigned ModuleAllocaAnalysis::getConstBufferSize(AllocaInst* AI) const {
    IGC_ASSERT(isa<ConstantInt>(AI->getArraySize()));
    Function* F = AI->getParent()->getParent();
    return getFuncAllocaInfo(F)->AllocaDesc[AI].second;
}

SmallVector<AllocaInst*, 8>& ModuleAllocaAnalysis::getAllocaInsts(Function* F) const {
    return getFuncAllocaInfo(F)->Allocas;
}

unsigned ModuleAllocaAnalysis::getTotalPrivateMemPerWI(Function* F) const {
    auto FI = getFuncAllocaInfo(F);
    return FI ? FI->TotalSize : 0;
}

ModuleAllocaAnalysis::FunctionAllocaInfo* ModuleAllocaAnalysis::getFuncAllocaInfo(Function* F) const {
    auto Iter = InfoMap.find(F);
    if (Iter != InfoMap.end())
        return Iter->second;
    return nullptr;
}

ModuleAllocaAnalysis::FunctionAllocaInfo* ModuleAllocaAnalysis::getOrCreateFuncAllocaInfo(Function* F) {
    auto Iter = InfoMap.find(F);
    if (Iter != InfoMap.end())
        return Iter->second;

    auto AllocaInfo = new FunctionAllocaInfo;
    InfoMap[F] = AllocaInfo;
    return AllocaInfo;
}

void ModuleAllocaAnalysis::analyze() {
    if (FGA && FGA->getModule()) {
        IGC_ASSERT(FGA->getModule() == M);
        for (auto FG : *FGA)
            analyze(FG);
    }
    else {
        for (auto& F : M->getFunctionList()) {
            if (F.empty())
                continue;

            unsigned Offset = 0;
            alignment_t Alignment = 0;
            analyze(&F, Offset, Alignment);
            if (Alignment > 0)
                Offset = iSTD::Align(Offset, (size_t)Alignment);
            getOrCreateFuncAllocaInfo(&F)->TotalSize = Offset;
        }
    }
}

void ModuleAllocaAnalysis::analyze(IGC::FunctionGroup* FG)
{
    // Calculate the size of private-memory we need to allocate to
    // every function-sub-group. Eache sub-group is led by a kernel or
    // a stack-call function.
    // Note that the function order does affect the final total amount of
    // private memory due to possible alignment constraints.
    //
    for (auto SubG : FG->Functions) {
        unsigned Offset = 0;
        alignment_t Alignment = 0;
        for (Function* F : *SubG) {
            if (F->empty())
                continue;
            analyze(F, Offset, Alignment);
        }

        // Use the final offset as the total size.
        if (Alignment > 0)
            Offset = iSTD::Align(Offset, (size_t)Alignment);

        // All functions in this group will get the same final size.
        for (Function* F : *SubG) {
            if (F->empty())
                continue;
            getOrCreateFuncAllocaInfo(F)->TotalSize = Offset;
        }
    }
}

void ModuleAllocaAnalysis::analyze(Function* F, unsigned& Offset, alignment_t& MaxAlignment)
{
    const DataLayout* DL = &M->getDataLayout();

    // Create alloca info even when there is no alloca, so that each function gets
    // an info entry.
    FunctionAllocaInfo* AllocaInfo = getOrCreateFuncAllocaInfo(F);

    // Collect allocas.
    SmallVector<AllocaInst*, 8> Allocas;
    for (auto& BB : F->getBasicBlockList()) {
        for (auto& Inst : BB.getInstList()) {
            if (AllocaInst* AI = dyn_cast<AllocaInst>(&Inst)) {
                Allocas.push_back(AI);
            }
        }
    }

    if (Allocas.empty())
        return;

    // Group by alignment and smallest first.
    auto getAlignment = [=](AllocaInst* AI) -> alignment_t {
        alignment_t Alignment = IGCLLVM::getAlignmentValue(AI);
        if (Alignment == 0)
            Alignment = DL->getABITypeAlignment(AI->getAllocatedType());
        return Alignment;
    };

    std::sort(Allocas.begin(), Allocas.end(),
        [=](AllocaInst* AI1, AllocaInst* AI2) {
            return getAlignment(AI1) < getAlignment(AI2);
        });

    for (auto AI : Allocas) {
        // Align alloca offset.
        auto Alignment = getAlignment(AI);
        Offset = iSTD::Align(Offset, (size_t)Alignment);

        // Keep track of the maximal alignment seen so far.
        if (Alignment > MaxAlignment)
            MaxAlignment = Alignment;

        // Compute alloca size. We don't know the variable length
        // alloca size so skip it.
        if (!isa<ConstantInt>(AI->getArraySize())) {
            continue;
        }
        ConstantInt* const SizeVal = cast<ConstantInt>(AI->getArraySize());
        IGC_ASSERT(nullptr != SizeVal);
        unsigned CurSize = (unsigned)(SizeVal->getZExtValue() *
            DL->getTypeAllocSize(AI->getAllocatedType()));
        AllocaInfo->setAllocaDesc(AI, Offset, CurSize);

        // Increment the current offset for the next alloca.
        Offset += CurSize;
    }

    // Update collected allocas into the function alloca info object.
    AllocaInfo->Allocas.swap(Allocas);
}

char ModuleAllocaAnalysis::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "igc-module-alloca-info"
#define PASS_DESCRIPTION "Analyse memory usage based on alloca instructions"
#define PASS_CFG_ONLY true
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(ModuleAllocaAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(ModuleAllocaAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

} // namespace IGC
