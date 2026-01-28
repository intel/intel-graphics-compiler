/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ModuleAllocaAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs/KernelArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/GenCodeGenModule.h"
#include "llvm/IR/Instructions.h"

using namespace llvm;

namespace IGC {
ModuleAllocaAnalysis::ModuleAllocaAnalysis() : ModulePass(ID) {
  initializeModuleAllocaAnalysisPass(*PassRegistry::getPassRegistry());
}

ModuleAllocaAnalysis::~ModuleAllocaAnalysis() {
  for (auto I = InfoMap.begin(), E = InfoMap.end(); I != E; ++I)
    delete I->second;
}

void ModuleAllocaAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<MetaDataUtilsWrapper>();
  AU.addRequired<CodeGenContextWrapper>();
}

StringRef ModuleAllocaAnalysis::getPassName() const { return "ModuleAllocaAnalysis"; }

bool ModuleAllocaAnalysis::runOnModule(Module &mod) {
  M = &mod;
  FGA = getAnalysisIfAvailable<GenXFunctionGroupAnalysis>();
  analyze();

  return false;
}

bool ModuleAllocaAnalysis::safeToUseScratchSpace() const {
  IGC_ASSERT(M);

  IGCMD::MetaDataUtils *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  ModuleMetaData &modMD = *getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
  CodeGenContext &Ctx = *getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  //
  // Update UseScratchSpacePrivateMemory based on WA and be consistent with
  // the implementation of CEncoder::ByteScatter().
  //
  if (Ctx.m_DriverInfo.NeedWAToTransformA32MessagesToA64() &&
      Ctx.platform.getWATable().WaNoA32ByteScatteredStatelessMessages) {
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
        // IGC has some legacy cases where stateless private memory must be used. This flag is to remove them. If
        // regression happens, revert it.
        bOCLLegacyStatelessCheck =
            !(Ctx.platform.hasScratchSurface() && IGC_IS_FLAG_ENABLED(RemoveLegacyOCLStatelessPrivateMemoryCases));
      }
    }
  }

  if (Ctx.allocatePrivateAsGlobalBuffer()) {
    return false;
  }

  if ((modMD.compOpt.OptDisable && bOCLLegacyStatelessCheck) || !supportsScratchSpacePrivateMemory) {
    return false;
  }

  //
  // Do not use scratch space if module has any stack call.
  // Do not use scratch space if modeule has any variable length alloca
  //
  if (bOCLLegacyStatelessCheck) {
    if (auto *FGA = getAnalysisIfAvailable<GenXFunctionGroupAnalysis>()) {
      if (FGA->getModule() == M) {
        for (auto &I : *FGA) {
          if (FGA->isIndirectCallGroup(I) && !I->isSingle())
            return false;
          if (I->hasStackCall() || I->hasVariableLengthAlloca())
            return false;
        }
      }
    } else {
      // Check individual functions if FGA not available
      for (auto &F : *M) {
        if (F.hasFnAttribute("visaStackCall") || F.hasFnAttribute("hasVLA"))
          return false;
      }
    }
  }

  const llvm::DataLayout *DL = &M->getDataLayout();

  for (auto &F : *M) {
    if (F.isDeclaration())
      continue;

    // Check each instr of this function.
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (AddrSpaceCastInst *CI = dyn_cast<AddrSpaceCastInst>(&I)) {
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
          if (PtrToIntInst *IPI = dyn_cast<PtrToIntInst>(&I)) {
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
        for (const auto &arg : kernelArgs) {
          const KernelArg::ArgType argTy = arg.getArgType();
          if (argTy == KernelArg::ArgType::PTR_DEVICE_QUEUE) {
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
    bool isGeometryStageShader = Ctx.type == ShaderType::VERTEX_SHADER || Ctx.type == ShaderType::HULL_SHADER ||
                                 Ctx.type == ShaderType::DOMAIN_SHADER || Ctx.type == ShaderType::GEOMETRY_SHADER;

    // FIXME: Below heuristics is not a clean design. Revisit this!
    // Start with simd16 or simd32 correspondingly if MinDispatchMode() is 8 or 16, which allows the medium size of
    // space per WI
    //  (simd8: largest, simd32, smallest). In doing so, there will be
    //  some space left for spilling in simd8 if spilling happens.
    int32_t simd_size = isGeometryStageShader
                            ?
                            numLanes(Ctx.platform.getMinDispatchMode())
                            :
                            (Ctx.platform.getMinDispatchMode() == SIMDMode::SIMD8 ? numLanes(SIMDMode::SIMD16)
                                                                                  : numLanes(SIMDMode::SIMD32));
    const int32_t subGrpSize = funcInfoMD->getSubGroupSize()->getSIMDSize();
    if (subGrpSize > simd_size)
      simd_size = std::min(subGrpSize, static_cast<int32_t>(numLanes(SIMDMode::SIMD32)));
    int32_t groupSize = IGCMD::IGCMetaDataHelper::getThreadGroupSize(*pMdUtils, &F);
    if (groupSize == 0)
      groupSize = IGCMD::IGCMetaDataHelper::getThreadGroupSizeHint(*pMdUtils, &F);
    if (groupSize > simd_size)
      simd_size = std::min(groupSize, static_cast<int32_t>(numLanes(SIMDMode::SIMD32)));

    // if one API doesn't support stateless, we should try to use smallest dispatch mode
    // which can hold more pvt_data to avoid error out.
    if (SeparateSpillAndScratch(&Ctx) && !supportsStatelessSpacePrivateMemory)
      simd_size = numLanes(Ctx.platform.getMinDispatchMode());

    unsigned maxScratchSpaceBytes =
        Ctx.platform.maxPerThreadScratchSpace(Ctx.m_DriverInfo.supports16MBPerThreadScratchSpace());
    unsigned scratchSpaceLimitPerWI = maxScratchSpaceBytes / simd_size;
    //
    // If spill happens, since the offset of scratch block rw send message
    // has only 12b, an assertion will be triggered if used scratch space
    // size >= 128 KB, here 128 KB = 2^12 * 256b.
    //
    const unsigned int totalPrivateMemPerWI = getTotalPrivateMemPerWI(&F);

    // FIXME: for now, to shrink size, let's use SIMD8 if have to.
    // later, maybe, we want to change to legacy behavior: SIMD16, to avoid potential spill.
    // but even so, when we support slot0 and slot1, then, we could still use SIMD8.
    if (Ctx.platform.hasScratchSurface() && Ctx.hasSyncRTCalls() && totalPrivateMemPerWI > scratchSpaceLimitPerWI) {
      simd_size = numLanes(Ctx.platform.getMinDispatchMode());
      scratchSpaceLimitPerWI = maxScratchSpaceBytes / simd_size;
    }

    if (totalPrivateMemPerWI > scratchSpaceLimitPerWI) {
      // IGC errors out when we are trying to remove statelesspvtmem of OCL (even though OCl still supports
      // statelesspvtmem). This assertion tests a scenario where (pvt_mem_usage > 256k) while statelessprivatememory is
      // not supported.
      IGC_ASSERT_EXIT(bOCLLegacyStatelessCheck);

      if (!supportsStatelessSpacePrivateMemory) {
        // For XeHP_SDV and above, if any API doesn't support statelesspvtmem, error it out if we find a case where
        // (pvt_mem > 256k). This assertion found a scenario where (pvt_mem_usage > 256k) while statelessprivatememory
        // is not supported.
        IGC_ASSERT(0);
        return true;
      }

      return false;
    }
  }

  // It is safe to use scratch space for private memory.
  return true;
}

unsigned ModuleAllocaAnalysis::getBufferStride(AllocaInst *AI) const {
  IGC_ASSERT(isa<ConstantInt>(AI->getArraySize()));
  Function *F = AI->getParent()->getParent();
  auto allocaInfo = getFuncAllocaInfo(F);
  IGC_ASSERT(allocaInfo);
  auto it = allocaInfo->NonUniformAllocaDesc.find(AI);
  if (it != allocaInfo->NonUniformAllocaDesc.end()) {
    return it->second.second;
  }
  return 0;
}

unsigned ModuleAllocaAnalysis::getBufferOffset(llvm::AllocaInst *AI) const {
  IGC_ASSERT(isa<ConstantInt>(AI->getArraySize()));
  Function *F = AI->getParent()->getParent();
  auto allocaInfo = getFuncAllocaInfo(F);
  IGC_ASSERT(allocaInfo);
  auto uit = allocaInfo->UniformAllocaDesc.find(AI);
  if (uit != allocaInfo->UniformAllocaDesc.end()) {
    return uit->second;
  } else {
    auto nit = allocaInfo->NonUniformAllocaDesc.find(AI);
    IGC_ASSERT(nit != allocaInfo->NonUniformAllocaDesc.end());
    return nit->second.first;
  }
}

unsigned ModuleAllocaAnalysis::isUniform(llvm::AllocaInst *AI) const {
  IGC_ASSERT(isa<ConstantInt>(AI->getArraySize()));
  Function *F = AI->getParent()->getParent();
  auto allocaInfo = getFuncAllocaInfo(F);
  IGC_ASSERT(allocaInfo);
  return allocaInfo->UniformAllocaDesc.count(AI) > 0;
}

llvm::Value *ModuleAllocaAnalysis::getPerThreadOffset(IGCLLVM::IRBuilder<> &IRB, llvm::AllocaInst *AI,
                                                      llvm::Value *simdSize, llvm::Value *threadId,
                                                      bool return64bitOffset /*= false*/) const {
  // Pseudo-code:
  // totalStride = nonUniformBlockOffset + SIMDSize * nonUniformBlockStride
  // totalStride = Align(totalStride, MaxAlignement)
  // perThreadOffset = totalPrivateMemPerWI * HWTID
  Function *F = AI->getParent()->getParent();
  auto allocaInfo = getFuncAllocaInfo(F);
  IGC_ASSERT(allocaInfo);
  Value *nonUniformBlockOffset = IRB.getInt32(allocaInfo->MemoryDescription->GetOffsetForNonUniformAllocas());
  Value *nonUniformBlockStride = IRB.getInt32(allocaInfo->MemoryDescription->GetStrideForNonUniformAllocas());
  Value *nonUniformAllocaBlockSize =
      IRB.CreateMul(nonUniformBlockStride, simdSize, VALUE_NAME(".NonUniformAllocasStride"));
  Value *totalStride =
      IRB.CreateAdd(nonUniformBlockOffset, nonUniformAllocaBlockSize, VALUE_NAME(".TotalPrivateMemPerWI"));
  unsigned maskForTotalPrivateMemPerWI = allocaInfo->MemoryDescription->GetMaxAlignment() - 1u;
  totalStride = IRB.CreateAdd(totalStride, IRB.getInt32(maskForTotalPrivateMemPerWI));
  totalStride =
      IRB.CreateAnd(totalStride, IRB.getInt32(~maskForTotalPrivateMemPerWI), VALUE_NAME("totalPrivateMemStride"));
  if (return64bitOffset) {
    threadId = IRB.CreateZExt(threadId, IRB.getInt64Ty());
    totalStride = IRB.CreateZExt(totalStride, IRB.getInt64Ty());
  }
  Value *perThreadOffset = IRB.CreateMul(threadId, totalStride);
  return perThreadOffset;
}

llvm::Value *ModuleAllocaAnalysis::getOffset(IGCLLVM::IRBuilder<> &IRB, llvm::AllocaInst *AI, llvm::Value *simdSize,
                                             llvm::Value *simdLaneId,
                                             std::optional<uint32_t> perLaneStride /*= std::nullopt*/) const

{
  IGC_ASSERT(isa<ConstantInt>(AI->getArraySize()));
  Function *F = AI->getParent()->getParent();
  auto allocaInfo = getFuncAllocaInfo(F);
  IGC_ASSERT(allocaInfo);

  Value *offset = nullptr;
  auto uit = allocaInfo->UniformAllocaDesc.find(AI);
  if (uit != allocaInfo->UniformAllocaDesc.end()) {
    // Pseudo-code:
    // offset = bufferOffset
    Value *baseOffset = IRB.getInt32(allocaInfo->MemoryDescription->GetOffsetForUniformAllocas());
    Value *relativeOffset = IRB.getInt32(uit->second);
    Value *bufferOffset = IRB.CreateAdd(baseOffset, relativeOffset, VALUE_NAME(AI->getName() + ".BufferOffset"));
    offset = bufferOffset;
  } else {
    // Pseudo-code:
    // offset = nonUniformPrivateMemoryBlockOffset + bufferOffset * SIMDSize + bufferStride * SIMDLane
    auto nit = allocaInfo->NonUniformAllocaDesc.find(AI);
    IGC_ASSERT(nit != allocaInfo->NonUniformAllocaDesc.end());
    auto [localOffset, localStride] = nit->second;
    if (perLaneStride.has_value()) {
      localStride = *perLaneStride;
    }
    Value *localOffsetVal =
        IRB.CreateMul(simdSize, IRB.getInt32(localOffset), VALUE_NAME(AI->getName() + ".SectionOffset"));
    unsigned nonUniformAllocasOffset = allocaInfo->MemoryDescription->GetOffsetForNonUniformAllocas();
    Value *baseOffset = IRB.getInt32(nonUniformAllocasOffset);
    Value *simdBufferOffset = IRB.CreateAdd(baseOffset, localOffsetVal, VALUE_NAME(AI->getName() + ".BufferOffset"));
    bool isUniform = AI->getMetadata("uniform") != nullptr;
    if (!isUniform) {
      Value *localOffset =
          IRB.CreateMul(simdLaneId, IRB.getInt32(localStride), VALUE_NAME(AI->getName() + ".PerLaneOffset"));
      simdBufferOffset = IRB.CreateAdd(simdBufferOffset, localOffset, VALUE_NAME(AI->getName() + ".SIMDBufferOffset"));
    }
    offset = simdBufferOffset;
  }

  return offset;
}

SmallVector<AllocaInst *, 8> &ModuleAllocaAnalysis::getAllocaInsts(Function *F) const {
  return getFuncAllocaInfo(F)->Allocas;
}

unsigned ModuleAllocaAnalysis::getTotalPrivateMemPerWI(Function *F) const {
  // Note that the total size is returned for SIMD1
  // The value is multiplied by the correct SIMD size and
  // reported to UMD. Uniform memory space is shared between
  // all subgroup threads. Therefore, it is safe to divide
  // the value by the smallest simd size.
  // This could be improved if the total size is computed
  // from simd size intrinsic calls instead.
  auto FI = getFuncAllocaInfo(F);
  if (FI) {
    unsigned minSimdSize = getMinSimdSize(F);
    uint64_t totalSize = FI->MemoryDescription->GetTotalSize(minSimdSize);
    totalSize = iSTD::Align(totalSize, minSimdSize);
    totalSize = totalSize / minSimdSize;
    unsigned ret = int_cast<unsigned>(totalSize);

    CodeGenContext &Ctx = *getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    ModuleMetaData *const modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    IGC_ASSERT(nullptr != modMD);
    if (modMD->compOpt.UseScratchSpacePrivateMemory == false &&
        Ctx.m_DriverInfo.supportsStatelessSpacePrivateMemory() &&
        Ctx.m_DriverInfo.requiresPowerOfTwoStatelessSpacePrivateMemorySize()) {
      ret = iSTD::RoundPower2(static_cast<DWORD>(ret));
    }

    if ((!modMD->compOpt.UseScratchSpacePrivateMemory || Ctx.m_DriverInfo.supports16MBPerThreadScratchSpace()) &&
        Ctx.platform.hasEfficient64bEnabled() && Ctx.m_DriverInfo.usePrivateMemoryLimitWorkaround()) {
      static constexpr unsigned SCRATCH_SPACE_STATELESS_PRIVATE_MEMORY_LIMIT = 0x80000000;
      static constexpr unsigned XE3P_MAX_EU_THREAD = 4096;

      // tuned to simd8 for relaxing the size threshold.
      uint32_t privateMemoryLimitPerWI =
          static_cast<DWORD>(SCRATCH_SPACE_STATELESS_PRIVATE_MEMORY_LIMIT / (XE3P_MAX_EU_THREAD * 8));

      if (ret > privateMemoryLimitPerWI) {
        ret = privateMemoryLimitPerWI;
      }
    }
    return ret;
  }
  return 0;
}

unsigned ModuleAllocaAnalysis::getPrivateMemAlignment(Function *F) const {
  auto FI = getFuncAllocaInfo(F);
  return FI ? FI->MemoryDescription->GetMaxAlignment() : 1;
}

ModuleAllocaAnalysis::FunctionAllocaInfo *ModuleAllocaAnalysis::getFuncAllocaInfo(Function *F) const {
  auto Iter = InfoMap.find(F);
  if (Iter != InfoMap.end())
    return Iter->second;
  return nullptr;
}

ModuleAllocaAnalysis::FunctionAllocaInfo *ModuleAllocaAnalysis::getOrCreateFuncAllocaInfo(Function *F) {
  auto Iter = InfoMap.find(F);
  if (Iter != InfoMap.end())
    return Iter->second;

  auto AllocaInfo = new FunctionAllocaInfo;
  InfoMap[F] = AllocaInfo;
  return AllocaInfo;
}

void ModuleAllocaAnalysis::analyze() {
  IGC_ASSERT(M);
  CodeGenContext &Ctx = *getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  const IGC::TriboolFlag ForceSupportUniformPrivateMemorySpace =
      static_cast<IGC::TriboolFlag>(IGC_GET_FLAG_VALUE(SupportUniformPrivateMemorySpace));
  switch (ForceSupportUniformPrivateMemorySpace) {
  case IGC::TriboolFlag::Enabled:
    SupportsUniformPrivateMemory = true;
    break;
  case IGC::TriboolFlag::Disabled:
    SupportsUniformPrivateMemory = false;
    break;
  case IGC::TriboolFlag::Default:
    SupportsUniformPrivateMemory = Ctx.m_DriverInfo.supportsUniformPrivateMemorySpace();
    break;
  }

  if (FGA && FGA->getModule()) {
    IGC_ASSERT(FGA->getModule() == M);
    for (auto FG : *FGA)
      analyze(FG);
  } else {
    for (auto &F : M->getFunctionList()) {
      if (F.empty())
        continue;

      PrivateMemoryDescription &MemoryDescription = MemoryDescriptions.emplace_back();
      analyze(&F, MemoryDescription);
    }
  }
}

void ModuleAllocaAnalysis::analyze(IGC::FunctionGroup *FG) {
  // Calculate the size of private-memory we need to allocate to
  // every function-sub-group. Eache sub-group is led by a kernel or
  // a stack-call function.
  // Note that the function order does affect the final total amount of
  // private memory due to possible alignment constraints.
  for (auto SubG : FG->Functions) {
    PrivateMemoryDescription &MemoryDescription = MemoryDescriptions.emplace_back();
    for (Function *F : *SubG) {
      if (F->empty())
        continue;
      analyze(F, MemoryDescription);
    }
  }
}

void ModuleAllocaAnalysis::analyze(Function *F, PrivateMemoryDescription &MemoryDescription) {
  const DataLayout *DL = &M->getDataLayout();

  // Create alloca info even when there is no alloca, so that each function gets
  // an info entry.
  FunctionAllocaInfo *AllocaInfo = getOrCreateFuncAllocaInfo(F);
  AllocaInfo->MemoryDescription = &MemoryDescription;

  // Collect allocas.
  SmallVector<AllocaInst *, 8> Allocas;
  for (auto &BB : *F) {
    for (auto &Inst : BB) {
      if (AllocaInst *AI = dyn_cast<AllocaInst>(&Inst)) {
        Allocas.push_back(AI);
      }
    }
  }

  if (Allocas.empty())
    return;

  std::sort(Allocas.begin(), Allocas.end(), [&DL](AllocaInst *AI1, AllocaInst *AI2) {
    return FunctionAllocaInfo::getAlignment(AI1, DL) < FunctionAllocaInfo::getAlignment(AI2, DL);
  });

  for (auto AI : Allocas) {
    AllocaInfo->AssignAlloca(AI, DL, SupportsUniformPrivateMemory);
  }

  // Update collected allocas into the function alloca info object.
  AllocaInfo->Allocas.swap(Allocas);
}

unsigned ModuleAllocaAnalysis::FunctionAllocaInfo::getAlignment(llvm::AllocaInst *AI, const llvm::DataLayout *DL) {
  alignment_t Alignment = IGCLLVM::getAlignmentValue(AI);
  if (Alignment == 0)
    Alignment = DL->getABITypeAlign(AI->getAllocatedType()).value();
  return std::max(int_cast<unsigned>(Alignment), 1u);
}

unsigned ModuleAllocaAnalysis::FunctionAllocaInfo::getSize(llvm::AllocaInst *AI, const llvm::DataLayout *DL) {
  IGC_ASSERT(isa<ConstantInt>(AI->getArraySize()));
  ConstantInt *const SizeVal = cast<ConstantInt>(AI->getArraySize());
  IGC_ASSERT(nullptr != SizeVal);
  unsigned CurSize = (unsigned)(SizeVal->getZExtValue() * DL->getTypeAllocSize(AI->getAllocatedType()));
  bool isPackedStruct = false;
  if (auto st = dyn_cast<StructType>(AI->getAllocatedType())) {
    isPackedStruct = st->isPacked();
  }
  if (!isPackedStruct) {
    auto Alignment = getAlignment(AI, DL);
    CurSize = iSTD::Align(CurSize, Alignment);
  }
  return CurSize;
}

void ModuleAllocaAnalysis::FunctionAllocaInfo::AssignAlloca(llvm::AllocaInst *AI, const llvm::DataLayout *DL,
                                                            bool SupportsUniformPrivateMemory) {
  // Dynamic allocas are handled by stack-related instructions
  if (!isa<ConstantInt>(AI->getArraySize())) {
    AI->getParent()->getParent()->addFnAttr("hasVLA");
    return;
  }
  bool isUniform = AI->getMetadata("uniform") != nullptr;
  if (SupportsUniformPrivateMemory && isUniform) {
    unsigned alignement = getAlignment(AI, DL);
    MemoryDescription->MaximumAlignmentForUniformAllocas =
        std::max(MemoryDescription->MaximumAlignmentForUniformAllocas, alignement);
    MemoryDescription->TotalSizeForUniformAllocas =
        iSTD::Align(MemoryDescription->TotalSizeForUniformAllocas, alignement);
    UniformAllocaDesc[AI] = MemoryDescription->TotalSizeForUniformAllocas;
    MemoryDescription->TotalSizeForUniformAllocas += getSize(AI, DL);
  } else {
    unsigned alignement = getAlignment(AI, DL);
    MemoryDescription->MaximumAlignmentForNonUniformAllocas =
        std::max(MemoryDescription->MaximumAlignmentForNonUniformAllocas, alignement);
    unsigned stride = getSize(AI, DL);
    MemoryDescription->TotalStrideForNonUniformAllocas =
        iSTD::Align(MemoryDescription->TotalStrideForNonUniformAllocas, alignement);
    NonUniformAllocaDesc[AI] = std::make_pair(MemoryDescription->TotalStrideForNonUniformAllocas, stride);
    MemoryDescription->TotalStrideForNonUniformAllocas += stride;
  }
}

unsigned ModuleAllocaAnalysis::getMinSimdSize(llvm::Function *pFunc) const {
  ModuleMetaData &modMD = *getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
  CodeGenContext &Ctx = *getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  uint32_t minSimdSize = numLanes(Ctx.platform.getMinDispatchMode());
  switch (Ctx.type) {
  case ShaderType::VERTEX_SHADER:
  case ShaderType::HULL_SHADER:
  case ShaderType::DOMAIN_SHADER:
  case ShaderType::GEOMETRY_SHADER:
    minSimdSize = numLanes(Ctx.GetSIMDMode());
    break;
  case ShaderType::TASK_SHADER:
    minSimdSize = modMD.taskInfo.SubgroupSize > 0 ? modMD.taskInfo.SubgroupSize : minSimdSize;
    break;
  case ShaderType::MESH_SHADER:
    minSimdSize = modMD.msInfo.SubgroupSize > 0 ? modMD.msInfo.SubgroupSize : minSimdSize;
    break;
  case ShaderType::COMPUTE_SHADER:
    minSimdSize = modMD.csInfo.waveSize > 0 ? modMD.csInfo.waveSize : minSimdSize;
    minSimdSize = modMD.csInfo.forcedSIMDSize > 0 ? modMD.csInfo.forcedSIMDSize : minSimdSize;
    break;
  case ShaderType::PIXEL_SHADER:
    break;
  case ShaderType::OPENCL_SHADER: {
    IGCMD::MetaDataUtils *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    auto funcInfoMD = pMdUtils->getFunctionsInfoItem(pFunc);
    const int32_t subGrpSize = funcInfoMD->getSubGroupSize()->getSIMDSize();
    if (subGrpSize > 0 && int_cast<uint16_t>(subGrpSize) > int_cast<uint16_t>(minSimdSize))
      minSimdSize = std::min(int_cast<uint16_t>(subGrpSize), numLanes(SIMDMode::SIMD32));
    int32_t groupSize = IGCMD::IGCMetaDataHelper::getThreadGroupSize(*pMdUtils, pFunc);
    if (groupSize == 0)
      groupSize = IGCMD::IGCMetaDataHelper::getThreadGroupSizeHint(*pMdUtils, pFunc);
    if (groupSize > 0 && int_cast<uint16_t>(groupSize) > int_cast<uint16_t>(minSimdSize))
      minSimdSize = std::min(int_cast<uint16_t>(groupSize), numLanes(SIMDMode::SIMD32));
    break;
  }
  default:
    break;
  }
  return minSimdSize;
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
