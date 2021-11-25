/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMUtils.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/Analysis/BasicAliasAnalysis.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/Local.h>
#include <llvmWrapper/IR/DerivedTypes.h>
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "Compiler/CISACodeGen/MemOpt2.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

namespace {

    class MemOpt2 : public FunctionPass {
        const DataLayout* DL;
        AliasAnalysis* AA;

        unsigned MaxLiveOutThreshold = 16;

        DenseSet<Instruction*> Scheduled;

    public:
        static char ID;

        MemOpt2(int MLT = -1) : FunctionPass(ID), DL(nullptr), AA(nullptr) {
            initializeMemOpt2Pass(*PassRegistry::getPassRegistry());
            if (unsigned T = IGC_GET_FLAG_VALUE(MaxLiveOutThreshold)) {
                MaxLiveOutThreshold = T;
            }
            else if (MLT != -1) {
                MaxLiveOutThreshold = unsigned(MLT);
            }
        }

        bool runOnFunction(Function& F) override;

        bool doFinalization(Module& F) override
        {
            Clear();
            return false;
        }

    private:
        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
            AU.addRequired<AAResultsWrapperPass>();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
        }

        bool clusterSampler(BasicBlock* BB);

        bool clusterMediaBlockRead(BasicBlock* BB);

        bool isSafeToMoveTo(Instruction* I, Instruction* Pos, const SmallVectorImpl<Instruction*>* CheckList) const;

        bool clusterLoad(BasicBlock* BB);
        bool isDefinedBefore(BasicBlock* BB, Instruction* I, Instruction* Pos) const {
            // Shortcut
            if (I == Pos)
                return true;

            auto BI = BB->begin();
            for (; &*BI != I && &*BI != Pos; ++BI)
                /*EMPTY*/;

            return &*BI == I;
        }
        bool isSafeToScheduleLoad(const LoadInst* LD,
            const SmallVectorImpl<Instruction*>* CheckList) const;
        bool schedule(
            BasicBlock* BB, Value* V, Instruction*& InsertPos,
            const SmallVectorImpl<Instruction*>* CheckList = nullptr);

        MemoryLocation getLocation(Instruction* I) const {
            if (LoadInst * LD = dyn_cast<LoadInst>(I))
                return MemoryLocation::get(LD);
            if (StoreInst * ST = dyn_cast<StoreInst>(I))
                return MemoryLocation::get(ST);
            return MemoryLocation();
        }

        unsigned getNumLiveOuts(Instruction* I) const {
            Type* Ty = I->getType();
            if (Ty->isVoidTy())
                return 0;
            // Don't understand non-single-value type.
            if (!Ty->isSingleValueType())
                return UINT_MAX;
            // Simply return 1 so far for scalar types.
            IGCLLVM::FixedVectorType* VecTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
            if (!VecTy)
                return 1;
            // Check how that vector is used.
            unsigned UseCount = 0;
            for (auto UI = I->user_begin(), UE = I->user_end(); UI != UE; ++UI) {
                Instruction* UseI = dyn_cast<Instruction>(*UI);
                if (!UseI || isInstructionTriviallyDead(UseI))
                    continue;
                ExtractElementInst* EEI = dyn_cast<ExtractElementInst>(UseI);
                if (!EEI)
                    return int_cast<unsigned>(VecTy->getNumElements());
                Value* Idx = EEI->getIndexOperand();
                if (!isa<Constant>(Idx))
                    return int_cast<unsigned>(VecTy->getNumElements());
                ++UseCount;
            }
            return UseCount;
        }

        unsigned getNumLiveOutBytes(Instruction* I) const {
            Type* Ty = I->getType();
            if (Ty->isVoidTy())
                return 0;
            // Don't understand non-single-value type.
            if (!Ty->isSingleValueType())
                return UINT_MAX;
            unsigned EltByte = (Ty->getScalarSizeInBits() + 7) / 8;
            // Simply return 1 so far for scalar types.
            IGCLLVM::FixedVectorType* VecTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
            if (!VecTy)
                return EltByte;
            // Check how that vector is used.
            unsigned UseCount = 0;
            for (auto UI = I->user_begin(), UE = I->user_end(); UI != UE; ++UI) {
                Instruction* UseI = dyn_cast<Instruction>(*UI);
                if (!UseI || isInstructionTriviallyDead(UseI))
                    continue;
                ExtractElementInst* EEI = dyn_cast<ExtractElementInst>(UseI);
                if (!EEI)
                    return unsigned(VecTy->getNumElements()) * EltByte;
                Value* Idx = EEI->getIndexOperand();
                if (!isa<Constant>(Idx))
                    return unsigned(VecTy->getNumElements()) * EltByte;
                ++UseCount;
            }
            return UseCount * EltByte;
        }

        unsigned getMaxLiveOutThreshold() const {
            static unsigned MaxLiveOutThreshold = IGC_GET_FLAG_VALUE(MaxLiveOutThreshold) ? IGC_GET_FLAG_VALUE(MaxLiveOutThreshold) : 4;
            return MaxLiveOutThreshold;
        }

        void Clear()
        {
            Scheduled.clear();
        }
    };

    char MemOpt2::ID = 0;

} // End anonymous namespace;

FunctionPass* createMemOpt2Pass(int MLT) {
    return new MemOpt2(MLT);
}

#define PASS_FLAG     "igc-memopt2"
#define PASS_DESC     "IGC Memory Optimization, the 2nd"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(MemOpt2, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(RegisterPressureEstimate)
IGC_INITIALIZE_PASS_END(MemOpt2, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

bool MemOpt2::runOnFunction(Function& F) {
    // Skip non-kernel function.
    MetaDataUtils* MDU = nullptr;
    MDU = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    auto FII = MDU->findFunctionsInfoItem(&F);
    if (FII == MDU->end_FunctionsInfo())
        return false;

    if (F.hasFnAttribute(llvm::Attribute::OptimizeNone))
        return false;

    IGC::CodeGenContext* cgCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    DL = &F.getParent()->getDataLayout();
    AA = &getAnalysis<AAResultsWrapperPass>().getAAResults();

    bool Changed = false;
    for (auto& BB : F) {
        bool LocalChanged = false;
        // Clear bookkeeping.
        Clear();

        if (cgCtx->m_DriverInfo.enableSampleClustering())
        {
            // Cluster samplers.
            LocalChanged = clusterSampler(&BB);
        }

        // Cluster MediaBlockReads
        LocalChanged |= clusterMediaBlockRead(&BB);

        // Cluster memory loads.
        // TODO: Revise that later
        // considering sampler and load together.
        if (!LocalChanged && cgCtx->type == ShaderType::OPENCL_SHADER)
            Changed |= clusterLoad(&BB);
        Changed |= LocalChanged;
    }

    return Changed;
}

bool MemOpt2::isSafeToMoveTo(Instruction* I, Instruction* Pos, const SmallVectorImpl<Instruction*>* CheckList) const {
    // TODO: So far, we simply don't allow rescheduling load/atomic operations.
    // Add alias analysis to allow memory operations to be rescheduled.
    if (auto LD = dyn_cast<LoadInst>(I)) {
        if (CheckList)
            return isSafeToScheduleLoad(LD, CheckList);
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
        case GenISAIntrinsic::GenISA_atomiccounterinc:
        case GenISAIntrinsic::GenISA_atomiccounterpredec:
            return false;
        default:
            break;
        }
    }
    return true;
}

bool MemOpt2::clusterSampler(BasicBlock* BB) {
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
bool MemOpt2::clusterMediaBlockRead(BasicBlock* BB) {
    bool Changed = false;

    Instruction* InsertPos = nullptr;
    unsigned Count = 0;
    Scheduled.clear();

    for (auto& BI : BB->getInstList()) {
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

bool MemOpt2::clusterLoad(BasicBlock* BB) {
    bool Changed = false;

    Instruction* InsertPos = nullptr;
    unsigned MaxLiveOutByte = getMaxLiveOutThreshold() * 4;
    unsigned CountByte = 0;
    for (auto BI = BB->begin(), BE = BB->end(); BI != BE; ++BI) {
        LoadInst* Lead = dyn_cast<LoadInst>(BI);
        if (!Lead || !Lead->isSimple())
            continue;
        if (Lead->getPointerAddressSpace() != ADDRESS_SPACE_LOCAL &&
            Lead->getPointerAddressSpace() != ADDRESS_SPACE_GLOBAL)
            continue;

        CountByte = getNumLiveOutBytes(Lead);
        if (CountByte > MaxLiveOutByte)
            continue;

        SmallVector<Instruction*, 8> CheckList;
        InsertPos = Lead;
        // Find candidate the cluster them.
        BasicBlock::iterator I = BasicBlock::iterator(InsertPos), E;
        for (I = std::next(I), E = BB->end(); I != E; ++I) {
            if (I->mayWriteToMemory())
                CheckList.push_back(&(*I));
            LoadInst* Next = dyn_cast<LoadInst>(I);
            if (!Next || !Next->isSimple())
                continue;
            // Skip memory accesses on different memory address space.
            // FIXME: GetUnderlyingObject() cannot track through `inttoptr`
            // after GEP lowering, we cannot detect loads on the same buffer
            // easily and cannot favor locality from memory accesses on the
            // same buffer.
            if (Next->getPointerAddressSpace() != Lead->getPointerAddressSpace())
                continue;
            CountByte += getNumLiveOutBytes(Next);
            if (CountByte > MaxLiveOutByte) {
                BasicBlock::iterator I = BasicBlock::iterator(Next);
                BI = std::prev(I);
                break;
            }
            Changed |= schedule(BB, Next, InsertPos, &CheckList);
        }
    }
    return Changed;
}

bool MemOpt2::isSafeToScheduleLoad(const LoadInst* LD,
    const SmallVectorImpl<Instruction*>* CheckList) const {
    MemoryLocation A = MemoryLocation::get(LD);

    for (auto* I : *CheckList) {
        // Skip instructions never writing to memory.
        if (!I->mayWriteToMemory())
            continue;
        // Unsafe if there's alias.
        MemoryLocation B = getLocation(I);
        if (!A.Ptr || !B.Ptr || AA->alias(A, B))
            return false;
    }

    return true;
}

bool MemOpt2::schedule(BasicBlock* BB, Value* V, Instruction*& InsertPos,
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
