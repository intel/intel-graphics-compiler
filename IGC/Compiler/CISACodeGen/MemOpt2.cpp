/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMUtils.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/Transforms/Utils/Local.h>
#include <llvmWrapper/Analysis/TargetLibraryInfo.h>
#include <llvmWrapper/IR/DerivedTypes.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CISACodeGen/MemOpt2.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

bool MemInstCluster::isDefinedBefore(BasicBlock *BB, Instruction *I, Instruction *Pos) const {
        // Shortcut
    if (I == Pos)
        return true;

    auto BI = BB->begin();
    for (; &*BI != I && &*BI != Pos; ++BI)
        /*EMPTY*/;

    return &*BI == I;
}

unsigned MemInstCluster::getNumLiveOuts(Instruction *I) const {
    Type *Ty = I->getType();
    if (Ty->isVoidTy())
        return 0;
    // Don't understand non-single-value type.
    if (!Ty->isSingleValueType())
        return UINT_MAX;
    // Simply return 1 so far for scalar types.
    IGCLLVM::FixedVectorType *VecTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
    if (!VecTy)
        return 1;
    // Check how that vector is used.
    unsigned UseCount = 0;
    for (auto UI = I->user_begin(), UE = I->user_end(); UI != UE; ++UI) {
        Instruction *UseI = dyn_cast<Instruction>(*UI);
        if (!UseI || isInstructionTriviallyDead(UseI))
            continue;
        ExtractElementInst *EEI = dyn_cast<ExtractElementInst>(UseI);
        if (!EEI)
            return int_cast<unsigned>(VecTy->getNumElements());
        Value *Idx = EEI->getIndexOperand();
        if (!isa<Constant>(Idx))
            return int_cast<unsigned>(VecTy->getNumElements());
        ++UseCount;
    }
    return UseCount;
}

unsigned MemInstCluster::getNumLiveOutBytes(Instruction *I) const {
    Type *Ty = I->getType();
    if (Ty->isVoidTy())
        return 0;
    // Don't understand non-single-value type.
    if (!Ty->isSingleValueType())
        return UINT_MAX;
    unsigned EltByte = (Ty->getScalarSizeInBits() + 7) / 8;
    // Simply return 1 so far for scalar types.
    IGCLLVM::FixedVectorType *VecTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
    if (!VecTy)
        return EltByte;
    // Check how that vector is used.
    unsigned UseCount = 0;
    for (auto UI = I->user_begin(), UE = I->user_end(); UI != UE; ++UI) {
        Instruction *UseI = dyn_cast<Instruction>(*UI);
        if (!UseI || isInstructionTriviallyDead(UseI))
            continue;
        ExtractElementInst *EEI = dyn_cast<ExtractElementInst>(UseI);
        if (!EEI)
            return unsigned(VecTy->getNumElements()) * EltByte;
        Value *Idx = EEI->getIndexOperand();
        if (!isa<Constant>(Idx))
            return unsigned(VecTy->getNumElements()) * EltByte;
        ++UseCount;
    }
    return UseCount * EltByte;
}

bool MemInstCluster::runForOCL(Function& F) {
    bool Changed = false;
    for (auto& BB : F) {
        bool LocalChanged = false;
        // Clear bookkeeping.
        Scheduled.clear();

        // Cluster samplers.
        LocalChanged = clusterSampler(&BB);

        // Cluster MediaBlockReads
        LocalChanged |= clusterMediaBlockRead(&BB);

        // Cluster memory loads.
        // TODO: Revise that later
        // considering sampler and load together.
        if (!LocalChanged)
            Changed |= clusterLoad(&BB);
        Changed |= LocalChanged;
    }
    Scheduled.clear();

    return Changed;
}

bool MemInstCluster::isSafeToMoveTo(Instruction* I, Instruction* Pos, const SmallVectorImpl<Instruction*>* CheckList) const {
    // TODO: So far, we simply don't allow rescheduling load/atomic operations.
    // Add alias analysis to allow memory operations to be rescheduled.
    if (auto LD = ALoadInst::get(I); LD.has_value()) {
        if (CheckList)
            return isSafeToScheduleLoad(LD.value(), CheckList);
        return false;
    }
    if (GenIntrinsicInst * GII = dyn_cast<GenIntrinsicInst>(I)) {
        switch (GII->getIntrinsicID()) {
        case GenISAIntrinsic::GenISA_intatomicraw:
        case GenISAIntrinsic::GenISA_floatatomicraw:
        case GenISAIntrinsic::GenISA_intatomicrawA64:
        case GenISAIntrinsic::GenISA_floatatomicrawA64:
        case GenISAIntrinsic::GenISA_dwordatomicstructured:
        case GenISAIntrinsic::GenISA_floatatomicstructured:
        case GenISAIntrinsic::GenISA_intatomictyped:
        case GenISAIntrinsic::GenISA_icmpxchgatomicraw:
        case GenISAIntrinsic::GenISA_fcmpxchgatomicraw:
        case GenISAIntrinsic::GenISA_icmpxchgatomicrawA64:
        case GenISAIntrinsic::GenISA_fcmpxchgatomicrawA64:
        case GenISAIntrinsic::GenISA_cmpxchgatomicstructured:
        case GenISAIntrinsic::GenISA_fcmpxchgatomicstructured:
        case GenISAIntrinsic::GenISA_icmpxchgatomictyped:
        case GenISAIntrinsic::GenISA_floatatomictyped:
        case GenISAIntrinsic::GenISA_fcmpxchgatomictyped:
        case GenISAIntrinsic::GenISA_atomiccounterinc:
        case GenISAIntrinsic::GenISA_atomiccounterpredec:
            return false;
        default:
            break;
        }
    }
    if (auto CI = dyn_cast<CallInst>(I)) {
        // So far, conservatively assume that any call instruction that can operate on memory
        // is not safe to be moved.
        if (CI->mayReadOrWriteMemory())
            return false;
    }
    return true;
}

// Clustering method does not handle memory dependence.
// Also we only cluster consecutive memory read from
// the same resource
bool MemInstCluster::runForGFX(BasicBlock* BB) {
    const unsigned CLUSTER_SAMPLER_THRESHOLD = 8;

    bool Changed = false;

    Instruction *InsertPos = nullptr;
    Value *CurResVal = nullptr;
    unsigned Count = 0;
    unsigned numSched = 0;
    Scheduled.clear();
    for (auto BI = BB->begin(), BE = BB->end(); BI != BE; ++BI) {
        GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(BI);
        if (!GII)
            continue;
        Value *ResourceVal = nullptr;
        if (auto *SI = dyn_cast<SampleIntrinsic>(GII)) {
            ResourceVal = SI->getTextureValue();
        } else if (auto *SI = dyn_cast<SamplerGatherIntrinsic>(GII)) {
            ResourceVal = SI->getTextureValue();
        } else if (auto *LI = dyn_cast<SamplerLoadIntrinsic>(GII)) {
            ResourceVal = LI->getTextureValue();
        } else if (auto *LI = dyn_cast<LdRawIntrinsic>(GII)) {
            ResourceVal = LI->getResourceValue();
        }
        if (ResourceVal) {
            if (!InsertPos) {
                InsertPos = GII;
                CurResVal = ResourceVal;
            }
            // Reschedule those long-latency op to InsertPos
            Count += getNumLiveOuts(GII);
            // condition for ending a cluster
            if (ResourceVal != CurResVal ||
                Count > MaxLiveOutThreshold ||
                numSched >= CLUSTER_SAMPLER_THRESHOLD) {
              Count = 0;
              InsertPos = GII;
              CurResVal = ResourceVal;
              numSched = 0;
            } else {
              Changed |= schedule(BB, GII, InsertPos);
              numSched++;
            }
        }
    }
    Scheduled.clear();
    return Changed;
}

bool MemInstCluster::clusterSampler(BasicBlock* BB) {
    const unsigned CLUSTER_SAMPLER_THRESHOLD = 5;

    bool Changed = false;

    Instruction* InsertPos = nullptr;
    unsigned Count = 0;
    unsigned numSched = 0;
    Scheduled.clear();
    for (auto BI = BB->begin(), BE = BB->end(); BI != BE; ++BI) {
        GenIntrinsicInst* GII = dyn_cast<GenIntrinsicInst>(BI);
        if (!GII)
            continue;
        switch (GII->getIntrinsicID()) {
            // TODO: Add more samplers.
        case GenISAIntrinsic::GenISA_sampleptr:
        case GenISAIntrinsic::GenISA_sampleLptr:
        case GenISAIntrinsic::GenISA_sampleBptr:
        case GenISAIntrinsic::GenISA_sampleLCptr:
        {
            if (!InsertPos)
                InsertPos = GII;
            // Reschedule the sampler to InsertPos
            Count += getNumLiveOuts(GII);
            if (Count > MaxLiveOutThreshold ||
                numSched >= CLUSTER_SAMPLER_THRESHOLD) {
                Count = 0;
                InsertPos = GII;
                numSched = 0;
            }
            else
            {
                Changed |= schedule(BB, GII, InsertPos);
                numSched++;
            }
            break;
        }
        default:
            break;
        }
    }
    return Changed;
}

//
// Congregate MediaBlockRead instrs to hide latency.
//
bool MemInstCluster::clusterMediaBlockRead(BasicBlock* BB) {
    bool Changed = false;

    Instruction* InsertPos = nullptr;
    unsigned Count = 0;
    Scheduled.clear();

    for (auto& BI : *BB) {
        if (GenIntrinsicInst * GII = dyn_cast<GenIntrinsicInst>(&BI)) {
            if (GII->getIntrinsicID() == GenISAIntrinsic::GenISA_simdMediaBlockRead) {
                if (!InsertPos)
                    InsertPos = GII;
                // Reschedule the MB read to InsertPos
                Count += getNumLiveOuts(GII);
                // Here experimental threshold 40 means compiler can congregate as many as
                // 5 intel_sub_group_block_read8 instrcutions. We can fine tune
                // the heuristic when we see more examples using intel_sub_group_block_read.
                if (Count > 40) {
                    Count = 0;
                    InsertPos = GII;
                }
                else
                    Changed |= schedule(BB, GII, InsertPos);
            }
        }
    }

    return Changed;
}

bool MemInstCluster::clusterLoad(BasicBlock* BB) {
    bool Changed = false;

    Instruction* InsertPos = nullptr;
    unsigned MaxLiveOutByte = getMaxLiveOutThreshold() * 4;
    unsigned CountByte = 0;
    for (auto BI = BB->begin(), BE = BB->end(); BI != BE; ++BI) {
        std::optional<ALoadInst> Lead = ALoadInst::get(&(*BI));
        if (!Lead.has_value() || !Lead->isSimple())
            continue;
        if (Lead->getPointerAddressSpace() != ADDRESS_SPACE_LOCAL &&
            Lead->getPointerAddressSpace() != ADDRESS_SPACE_GLOBAL)
            continue;

        CountByte = getNumLiveOutBytes(Lead->inst());
        if (CountByte > MaxLiveOutByte)
            continue;

        SmallVector<Instruction*, 8> CheckList;
        InsertPos = Lead->inst();
        // Find candidate the cluster them.
        BasicBlock::iterator I = BasicBlock::iterator(InsertPos), E;
        for (I = std::next(I), E = BB->end(); I != E; ++I) {
            if (I->mayWriteToMemory())
                CheckList.push_back(&(*I));
            std::optional<ALoadInst> Next = ALoadInst::get(&(*I));
            if (!Next.has_value() || !Next->isSimple())
                continue;
            // Skip memory accesses on different memory address space.
            // FIXME: GetUnderlyingObject() cannot track through `inttoptr`
            // after GEP lowering, we cannot detect loads on the same buffer
            // easily and cannot favor locality from memory accesses on the
            // same buffer.
            if (Next->getPointerAddressSpace() != Lead->getPointerAddressSpace())
                continue;
            CountByte += getNumLiveOutBytes(Next->inst());
            if (CountByte > MaxLiveOutByte) {
                BasicBlock::iterator I = BasicBlock::iterator(Next->inst());
                BI = std::prev(I);
                break;
            }
            Changed |= schedule(BB, Next->inst(), InsertPos, &CheckList);
        }
    }
    return Changed;
}

bool MemInstCluster::isSafeToScheduleLoad(const ALoadInst& LD,
    const SmallVectorImpl<Instruction*>* CheckList) const {
    MemoryLocation A = getLocation(LD.inst(), TLI);

    for (auto* I : *CheckList) {
        // Skip instructions never writing to memory.
        if (!I->mayWriteToMemory())
            continue;
        if (!AA)
            return false;
        // Unsafe if there's alias.
        MemoryLocation B = getLocation(I, TLI);
        if (!A.Ptr || !B.Ptr || AA->alias(A, B))
            return false;
    }

    return true;
}

bool MemInstCluster::schedule(BasicBlock* BB, Value* V, Instruction*& InsertPos,
    const SmallVectorImpl<Instruction*>* CheckList) {
    Instruction* I = dyn_cast<Instruction>(V);
    if (!I)
        return false;

    // Skip value defined in other BBs.
    if (I->getParent() != BB)
        return false;

    // Skip phi-node as they are eventually defined in other BBs.
    if (isa<PHINode>(I))
        return false;

    // Don't re-schedule instruction again.
    if (Scheduled.count(I)) {
        if (InsertPos && !isDefinedBefore(BB, I, InsertPos))
            InsertPos = I;
        return false;
    }

    bool Changed = false;

    // Try to schedule all its operands first.
    for (auto OI = I->op_begin(), OE = I->op_end(); OI != OE; ++OI)
        Changed |= schedule(BB, OI->get(), InsertPos, CheckList);

    // Mark this instruction `visited`.
    Scheduled.insert(I);

    // Skip if the instruction is already defined before insertion position.
    if (InsertPos && isDefinedBefore(BB, I, InsertPos))
        return Changed;

    // Schedule itself.
    if (InsertPos && isSafeToMoveTo(I, InsertPos, CheckList)) {
        I->removeFromParent();
        I->insertAfter(InsertPos);
    }

    InsertPos = I;
    return true;
}

namespace {

class MemOpt2 : public FunctionPass {

  public:
    static char ID;

    MemOpt2(int MLT = -1) : FunctionPass(ID) {
        initializeMemOpt2Pass(*PassRegistry::getPassRegistry());
        if (unsigned T = IGC_GET_FLAG_VALUE(MaxLiveOutThreshold)) {
            MaxLiveOutThreshold = T;
        } else if (MLT != -1) {
            MaxLiveOutThreshold = unsigned(MLT);
        }
    }

    bool runOnFunction(Function &F) override;

  private:
    void getAnalysisUsage(AnalysisUsage &AU) const override {
        AU.setPreservesCFG();
        AU.addRequired<AAResultsWrapperPass>();
        AU.addRequired<CodeGenContextWrapper>();
        AU.addRequired<MetaDataUtilsWrapper>();
        AU.addRequired<TargetLibraryInfoWrapperPass>();
    }
    MemInstCluster Cluster;
    unsigned MaxLiveOutThreshold = 16;
};

char MemOpt2::ID = 0;

} // namespace

FunctionPass *createMemOpt2Pass(int MLT) { return new MemOpt2(MLT); }

#define PASS_FLAG "igc-memopt2"
#define PASS_DESC "IGC Memory Optimization the 2nd"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(MemOpt2, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY,
                          PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
IGC_INITIALIZE_PASS_END(MemOpt2, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY,
                        PASS_ANALYSIS)

bool MemOpt2::runOnFunction(Function &F) {
    // Skip non-kernel function.
    MetaDataUtils *MDU = nullptr;
    MDU = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    auto FII = MDU->findFunctionsInfoItem(&F);
    if (FII == MDU->end_FunctionsInfo())
        return false;

    if (F.hasFnAttribute(llvm::Attribute::OptimizeNone))
        return false;

    IGC::CodeGenContext *cgCtx =
        getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    auto DL = &F.getParent()->getDataLayout();
    auto AA = &getAnalysis<AAResultsWrapperPass>().getAAResults();
    auto TLI = &getAnalysis<TargetLibraryInfoWrapperPass>().getTLI();

    Cluster.init(cgCtx, DL, AA, TLI, MaxLiveOutThreshold);
    return Cluster.runForOCL(F);
}
