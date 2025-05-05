/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenerateFrequencyData.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/igc_regkeys.hpp"
#include "Probe/Assertion.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstVisitor.h>

#include "llvmWrapper/ADT/Optional.h"

#include <llvm/Analysis/BlockFrequencyInfo.h>
#include <llvm/Analysis/BranchProbabilityInfo.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/SyntheticCountsUtils.h>
#include <llvm/ADT/Optional.h>

#include <llvm/Support/ScaledNumber.h>
#include <unordered_map>
#include <string>


using namespace llvm;
using Scaled64 = ScaledNumber<uint64_t>;

class GenerateFrequencyData : public ModulePass {
  typedef enum {
    PGSS_IGC_DUMP_BLK = 0x1,
    PGSS_IGC_DUMP_FUNC = 0x2,
  } PGSS_DUMP_t;

public:
    static char ID;

    GenerateFrequencyData() : ModulePass(ID), M(nullptr) {
        IGC::initializeGenerateFrequencyDataPass(*PassRegistry::getPassRegistry());
    }

    bool runOnModule(Module&) override;
    void runStaticAnalysis();
    void updateStaticFuncFreq(DenseMap<Function*, ScaledNumber<uint64_t>>& Counts);

    void getAnalysisUsage(AnalysisUsage& AU) const override {
        AU.setPreservesCFG();
        AU.addRequired<LoopInfoWrapperPass>();
        AU.addRequired<BranchProbabilityInfoWrapperPass>();
        AU.addRequired<BlockFrequencyInfoWrapperPass>();
    }

    Module* getModule() { return M;};
    void setModule(Module* m) { M = m; };

private:
    Module *M;
};


char GenerateFrequencyData::ID = 0;

#define PASS_FLAG     "igc-generate-frequency-data"
#define PASS_DESC     "Generate frequency data"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false

namespace IGC {
    IGC_INITIALIZE_PASS_BEGIN(GenerateFrequencyData, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
    IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
    IGC_INITIALIZE_PASS_DEPENDENCY(BranchProbabilityInfoWrapperPass)
    IGC_INITIALIZE_PASS_DEPENDENCY(BlockFrequencyInfoWrapperPass)
    IGC_INITIALIZE_PASS_END(GenerateFrequencyData, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
}

llvm::ModulePass* IGC::createGenerateFrequencyDataPass()
{
    return new GenerateFrequencyData();
}

bool GenerateFrequencyData::runOnModule(Module& M) {
    setModule(&M);
    runStaticAnalysis();
    return false;
}

void GenerateFrequencyData::runStaticAnalysis()
{
    //Analyze function frequencies from SyntheticCountsPropagation
    //PrintStaticProfilingForKernelSizeReduction(0x1, "------------------Static analysis start------------------")
    DenseMap<Function*, ScaledNumber<uint64_t>> F_freqs;
    DenseMap<BasicBlock*, ScaledNumber<uint64_t>> B_freqs;
    LLVMContext& C = M->getContext();
    updateStaticFuncFreq(F_freqs);
    for (auto& F : M->getFunctionList()) {
        if (F.empty() || F_freqs.find(&F) == F_freqs.end())
            continue;
        auto& BFI = getAnalysis<BlockFrequencyInfoWrapperPass>(F).getBFI();
        Scaled64 EntryFreq(BFI.getEntryFreq(), 0);

        if ((IGC_GET_FLAG_VALUE(PrintStaticProfileGuidedSpillCostAnalysis) &
             PGSS_IGC_DUMP_BLK) != 0)
            dbgs() << "Function frequency of " << F.getName().str() << ": " << F_freqs[&F].toString() << "\n";

        for (auto& B : F)
        {
            Scaled64 BBCount(BFI.getBlockFreq(&B).getFrequency(), 0);
            BBCount /= EntryFreq;
            BBCount *= F_freqs[&F];
            B_freqs[&B] = BBCount;
            if ((IGC_GET_FLAG_VALUE(PrintStaticProfileGuidedSpillCostAnalysis) &
                 PGSS_IGC_DUMP_BLK) != 0)
                dbgs() << "Block frequency of " << B.getName().str() << " " <<  &B << ": " << BBCount.toString() << "\n";

            Instruction* last_inst = B.getTerminator();
            if (last_inst)
            {
                //llvm::dbgs() << "Digits: " <<  BBCount.getDigits() << "\n";
                //llvm::dbgs() << "Scale: " << BBCount.getScale() << "\n";
                MDNode* m_node = MDNode::get(C, MDString::get(C, std::to_string(BBCount.getDigits())));
                last_inst->setMetadata("stats.blockFrequency.digits", m_node);
                m_node = MDNode::get(C, MDString::get(C, std::to_string(BBCount.getScale())));
                last_inst->setMetadata("stats.blockFrequency.scale", m_node);
            }
        }
    }
    return;
}


void GenerateFrequencyData::updateStaticFuncFreq(DenseMap<Function*, ScaledNumber<uint64_t>> &Counts)
{
    auto MayHaveIndirectCalls = [](Function& F) {
        for (auto* U : F.users()) {
            if (!isa<CallInst>(U) && !isa<InvokeInst>(U))
                return true;
        }
        return false;
    };
    uint64_t InitialSyntheticCount = 10;
    uint64_t InlineSyntheticCount = 15;
    uint64_t ColdSyntheticCount = 5;
    std::unordered_map<llvm::BasicBlock*, Scaled64> blockFreqs;
    std::unordered_map<llvm::Function*, Scaled64> entryFreqs;
    for (Function& F : getModule()->getFunctionList()) {
        uint64_t InitialCount = InitialSyntheticCount;
        if (!F.empty())
        {
            auto& BFI = getAnalysis<BlockFrequencyInfoWrapperPass>(F).getBFI();
            entryFreqs[&F] = Scaled64(BFI.getEntryFreq(), 0);
            for (auto& B : F)
                blockFreqs[&B] = Scaled64(BFI.getBlockFreq(&B).getFrequency(), 0);
        }
        if (F.isDeclaration())
            continue;
        if (F.hasFnAttribute(llvm::Attribute::AlwaysInline) ||
            F.hasFnAttribute(llvm::Attribute::InlineHint)) {
            // Use a higher value for inline functions to account for the fact that
            // these are usually beneficial to inline.
            InitialCount = InlineSyntheticCount;
        }
        else if (F.hasLocalLinkage() && !MayHaveIndirectCalls(F)) {
            // Local functions without inline hints get counts only through
            // propagation.
            InitialCount = 0;
        }
        else if (F.hasFnAttribute(llvm::Attribute::Cold) ||
            F.hasFnAttribute(llvm::Attribute::NoInline)) {
            // Use a lower value for noinline and cold functions.
            InitialCount = ColdSyntheticCount;
        }
        Counts[&F] = Scaled64(InitialCount, 0);
    }
    // Edge includes information about the source. Hence ignore the first
    // parameter.
    auto GetCallSiteProfCount = [&](const CallGraphNode*,
        const CallGraphNode::CallRecord& Edge) {
            std::optional<Scaled64> Res = std::nullopt;
            if (!Edge.first)
                return IGCLLVM::makeLLVMOptional(Res);
            CallBase& CB = *cast<CallBase>(*Edge.first);
            Function* Caller = CB.getCaller();
            BasicBlock* CSBB = CB.getParent();

            // Now compute the callsite count from relative frequency and
            // entry count:
            Scaled64 EntryFreq = entryFreqs[Caller];
            Scaled64 BBCount = blockFreqs[CSBB];
            IGC_ASSERT(EntryFreq != 0);
            BBCount /= EntryFreq;
            BBCount *= Counts[Caller];
            return IGCLLVM::makeLLVMOptional(std::optional<Scaled64>(BBCount));
    };
    CallGraph CG(*M);
    // Propgate the entry counts on the callgraph.
    SyntheticCountsUtils<const CallGraph*>::propagate(
        &CG, GetCallSiteProfCount, [&](const CallGraphNode* N, Scaled64 New) {
            auto F = N->getFunction();
            if (!F || F->isDeclaration())
                return;
            Counts[F] += New;
        });

    for (auto &F : M->getFunctionList()) {
        if (F.empty())
            continue;
        if (Counts.find(&F) != Counts.end()) {
            if ((IGC_GET_FLAG_VALUE(PrintStaticProfileGuidedSpillCostAnalysis) &
                 PGSS_IGC_DUMP_FUNC) != 0)
                dbgs() << F.getName().str()
                       << " Freq: " << Counts[&F].toString() << "\n";
        }
    }
    return;
}