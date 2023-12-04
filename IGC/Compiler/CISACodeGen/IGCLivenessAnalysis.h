/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

#include "Compiler/IGCPassSupport.h"
#include "DebugInfo/VISAIDebugEmitter.hpp"
#include "DebugInfo/VISAModule.hpp"
#include "Probe/Assertion.h"
#include "ShaderCodeGen.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/DIBuilder.h"

using namespace IGC;

typedef std::unordered_map<llvm::Value *, unsigned int> InclusionSet;
typedef llvm::SmallPtrSet<llvm::Value *, 16> ValueSet;
typedef std::unordered_map<llvm::BasicBlock *, ValueSet> DFSet;
typedef std::unordered_map<llvm::BasicBlock *, ValueSet> PhiSet;
typedef std::unordered_map<llvm::BasicBlock *, PhiSet> InPhiSet;
typedef std::unordered_map<llvm::Value *, unsigned int> InsideBlockPressureMap;

namespace IGC {
FunctionPass *
createIGCEarlyRegEstimator(bool UseWIAnalysis = false, bool DumpToFIle = false,
                           std::string DumpFileName = "RegPressureEstimate.ll");

class IGCLivenessAnalysis : public llvm::FunctionPass {
    // contains all values that liveIn into this block
    DFSet In;
    // this is a redundant set for visualization purposes,
    // contains all values that go into PHIs grouped
    // by the block from which they are coming
    InPhiSet InPhi;
    // contains all of the values that liveOut out of this block
    DFSet Out;
    // we can use WIAnalysis only in codegen part of pipeline
    // but sometimes it can be useful to collect at least some
    // pressure information before
    bool UseWIAnalysis = false;
    bool DumpToFile = false;
    std::string DumpFileName = "default";
    // controls verbocity of
    unsigned PrinterType = IGC_GET_FLAG_VALUE(RegPressureVerbocity);

    WIAnalysis *WI = nullptr;
    IGC::CodeGenContext *CGCtx = nullptr;

  public:
    static char ID;
    llvm::StringRef getPassName() const override {
        return "IGCLivenessAnalysis";
    }

    IGCLivenessAnalysis();
    IGCLivenessAnalysis(
        bool UseWIAnalysis, bool DumpToFile,
        const std::string &DumpFileName);

    virtual ~IGCLivenessAnalysis() {}

    // returns all definitions that were made in the block
    // computed by taking difference between In and Out,
    // everyting that was originated in the block and got into OUT
    ValueSet getDefs(llvm::BasicBlock &BB);
    DFSet &getInSet() { return In; }
    const DFSet &getInSet() const { return In; }
    InPhiSet &getInPhiSet() { return InPhi; }
    const InPhiSet &getInPhiSet() const { return InPhi; }
    DFSet &getOutSet() { return Out; }
    const DFSet &getOutSet() const { return Out; }

    SIMDMode bestGuessSIMDSize();
    // I expect it to be used as
    //   InsideBlockPressureMap Map = getPressureMapForBB(...)
    // for copy elision
    // to get SIMD, you can use bestGuessSIMDSize()
    // if you know better, put your own value
    InsideBlockPressureMap getPressureMapForBB(llvm::BasicBlock &BB,
                                               unsigned int SIMD) {
        InsideBlockPressureMap PressureMap;
        collectPressureForBB(BB, PressureMap, SIMD);
        return PressureMap;
    }

    unsigned int getMaxRegCountForBB(llvm::BasicBlock &BB, unsigned int SIMD) {
        InsideBlockPressureMap PressureMap;
        collectPressureForBB(BB, PressureMap, SIMD);

        unsigned int MaxSizeInBytes = 0;
        for (const auto &Pair : PressureMap) {
            MaxSizeInBytes = std::max(MaxSizeInBytes, Pair.second);
        }
        unsigned int RegisterSizeInBytes = registerSizeInBytes();
        unsigned int MaxAmountOfRegistersRoundUp =
            (MaxSizeInBytes + RegisterSizeInBytes - 1) / RegisterSizeInBytes;
        return MaxAmountOfRegistersRoundUp;
    }

    // be aware, for now, it doesn't count properly nested functions, and their
    // register pressure
    unsigned int getMaxRegCountForFunction(llvm::Function &F,
                                           unsigned int SIMD) {
        unsigned int Max = 0;
        for (BasicBlock &BB : F) {
            Max = std::max(getMaxRegCountForBB(BB, SIMD), Max);
        }
        return Max;
    }

    llvm::BasicBlock *getMaxRegCountBBForFunction(llvm::Function &F) {
        llvm::BasicBlock *HottestBB = NULL;
        unsigned int Max = 0;
        for (BasicBlock &BB : F) {
            unsigned int BBPressure = getMaxRegCountForBB(BB, 8);
            HottestBB = BBPressure > Max ? &BB : HottestBB;
            Max = std::max(BBPressure, Max);
        }
        return HottestBB;
    }

    void releaseMemory() override {
        In.clear();
        InPhi.clear();
        Out.clear();
    }

    // if you need to recompute pressure analysis after modifications were made
    // that can potentially change In Out sets, we need to update them, it's fast
    // collectPressureForBB()
    // ...
    // modifications that change In & Out
    // ...
    // rerunLivenessAnalysis()
    // collectPressureForBB()
    void rerunLivenessAnalysis(llvm::Function &F) {
        releaseMemory();
        livenessAnalysis(F);
    }

  private:
    void dumpRegPressure(llvm::Function &F, unsigned int SIMD);
    void printInstruction(llvm::Instruction *Inst, std::string &Str);
    void printNames(const ValueSet &Set, std::string &name);
    void printName(llvm::Value *Val, std::string &String);
    void printDefNames(const ValueSet &Set, std::string &name);
    void printSets(llvm::BasicBlock *BB, std::string &Output,
                   unsigned int SIMD);
    void printDefs(const ValueSet &In, const ValueSet &Out,
                   std::string &Output);
    void printPhi(const PhiSet &Set, std::string &Output);
    void printIntraBlock(llvm::BasicBlock &BB, std::string &Output,
                         InsideBlockPressureMap &BBListing);

    unsigned int registerSizeInBytes();
    unsigned int estimateSizeInBytes(ValueSet &Set, const DataLayout &DL,
                                     unsigned int SIMD);
    void collectPressureForBB(llvm::BasicBlock &BB,
                              InsideBlockPressureMap &BBListing,
                              unsigned int SIMD);
    void intraBlock(llvm::BasicBlock &BB, std::string &Output,
                    unsigned int SIMD);
    void computeHeatMap();

    void mergeSets(ValueSet *OutSet, llvm::BasicBlock *Succ);
    void combineOut(llvm::BasicBlock *BB, ValueSet *Set);
    void addToPhiSet(llvm::PHINode *Phi, PhiSet *InPhiSet);
    void processBlock(llvm::BasicBlock *BB, ValueSet &Set, PhiSet *PhiSet);
    void livenessAnalysis(llvm::Function &F);

    virtual bool runOnFunction(llvm::Function &F) override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
        AU.setPreservesAll();
        AU.addRequired<CodeGenContextWrapper>();
        AU.addRequired<WIAnalysis>();
    }
};
}; // namespace IGC
