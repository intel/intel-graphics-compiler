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
#include "IGCFunctionExternalPressure.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/DIBuilder.h"
#include "common/LLVMWarningsPop.hpp"

using namespace IGC;

namespace IGC {

class IGCLivenessAnalysis : public llvm::FunctionPass {
    // contains all values that liveIn into this block
    DFSet In;
    // this is a redundant set for visualization purposes,
    // contains all values that go into PHIs grouped
    // by the block from which they are coming
    InPhiSet InPhi;
    // contains all of the values that liveOut out of this block
    DFSet Out;

    IGC::CodeGenContext *CGCtx = nullptr;

  public:

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

    unsigned int estimateSizeInBytes(ValueSet &Set, llvm::Function &F, unsigned int SIMD, WIAnalysisRunner* WI = nullptr);
    void collectPressureForBB(llvm::BasicBlock &BB,
                              InsideBlockPressureMap &BBListing,
                              unsigned int SIMD,
                              WIAnalysisRunner* WI = nullptr);

    SIMDMode bestGuessSIMDSize();
    // I expect it to be used as
    //   InsideBlockPressureMap Map = getPressureMapForBB(...)
    // for copy elision
    // to get SIMD, you can use bestGuessSIMDSize()
    // if you know better, put your own value
    InsideBlockPressureMap getPressureMapForBB(llvm::BasicBlock &BB,
                                               unsigned int SIMD, WIAnalysisRunner* WI = nullptr) {
        InsideBlockPressureMap PressureMap;
        collectPressureForBB(BB, PressureMap, SIMD, WI);
        return PressureMap;
    }

    unsigned int bytesToRegisters(unsigned int Bytes) {
        unsigned int RegisterSizeInBytes = registerSizeInBytes();
        unsigned int AmountOfRegistersRoundUp =
            (Bytes + RegisterSizeInBytes - 1) / RegisterSizeInBytes;
        return AmountOfRegistersRoundUp;
    }

    unsigned int getMaxRegCountForBB(llvm::BasicBlock &BB, unsigned int SIMD, WIAnalysisRunner* WI = nullptr) {
        InsideBlockPressureMap PressureMap;
        collectPressureForBB(BB, PressureMap, SIMD, WI);

        unsigned int MaxSizeInBytes = 0;
        for (const auto &Pair : PressureMap) {
            MaxSizeInBytes = std::max(MaxSizeInBytes, Pair.second);
        }
        return bytesToRegisters(MaxSizeInBytes);
    }

    // be aware, for now, it doesn't count properly nested functions, and their
    // register pressure
    unsigned int getMaxRegCountForFunction(llvm::Function &F,
                                           unsigned int SIMD, WIAnalysisRunner* WI = nullptr) {
        unsigned int Max = 0;
        for (BasicBlock &BB : F) {
            Max = std::max(getMaxRegCountForBB(BB, SIMD, WI), Max);
        }
        return Max;
    }

    unsigned int getMaxRegCountForLoop(llvm::Loop &L,
                                       unsigned int SIMD,
                                       WIAnalysisRunner* WI = nullptr) {
        unsigned int Max = 0;
        for (BasicBlock *BB : L.getBlocks())
        {
            unsigned int BBPressure = getMaxRegCountForBB(*BB, SIMD, WI);
            Max = std::max(BBPressure, Max);
        }
        return Max;
    }

    llvm::BasicBlock *getMaxRegCountBBForFunction(llvm::Function &F, WIAnalysisRunner* WI = nullptr) {
        llvm::BasicBlock *HottestBB = NULL;
        unsigned int Max = 0;
        for (BasicBlock &BB : F) {
            unsigned int BBPressure = getMaxRegCountForBB(BB, 8, WI);
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
    void rerunLivenessAnalysis(llvm::Function &F, BBSet *BBs = nullptr) {
        if (BBs != nullptr)
        {
            for (BasicBlock *BB : *BBs)
            {
                In[BB].clear();
                Out[BB].clear();
                InPhi[BB].clear();
            }
        }
        else
        {
            releaseMemory();
        }
        livenessAnalysis(F, BBs);
    }
    void livenessAnalysis(llvm::Function &F, BBSet *StartBBs);

    static char ID;
    llvm::StringRef getPassName() const override { return "IGCLivenessAnalysis"; }
    IGCLivenessAnalysis();
    virtual ~IGCLivenessAnalysis() {}
    virtual bool runOnFunction(llvm::Function &F) override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
        AU.setPreservesAll();
        AU.addRequired<CodeGenContextWrapper>();
    }
  private:

    unsigned int registerSizeInBytes();
    void mergeSets(ValueSet *OutSet, llvm::BasicBlock *Succ);
    void combineOut(llvm::BasicBlock *BB, ValueSet *Set);
    void addToPhiSet(llvm::PHINode *Phi, PhiSet *InPhiSet);
    void addOperandsToSet(llvm::Instruction *Inst, ValueSet &Set);
    void addNonLocalOperandsToSet(llvm::Instruction *Inst, ValueSet &Set);
    void processBlock(llvm::BasicBlock *BB, ValueSet &Set, PhiSet *PhiSet);

};


class IGCRegisterPressurePrinter : public llvm::FunctionPass {

    IGCLivenessAnalysis *RPE = nullptr;
    WIAnalysis *WI = nullptr;
    CodeGenContext *CGCtx = nullptr;

    bool DumpToFile = false;
    std::string DumpFileName = "default";
    // controls printer verbocity
    // 1 -> print instruction dump
    // 2 -> print with phi listing
    // 3 -> print with ssa value names DEF, KILL, IN, OUT
    unsigned int PrinterType = IGC_GET_FLAG_VALUE(RegPressureVerbocity);
    // maximum potential calling context pressure of a function
    unsigned int ExternalPressure = 0;

    void intraBlock(llvm::BasicBlock &BB, std::string &Output, unsigned int SIMD);
    void dumpRegPressure(llvm::Function &F, unsigned int SIMD);
    void printInstruction(llvm::Instruction *Inst, std::string &Str);
    void printNames(const ValueSet &Set, std::string &name);
    void printName(llvm::Value *Val, std::string &String);
    void printDefNames(const ValueSet &Set, std::string &name);
    void printSets(llvm::BasicBlock *BB, std::string &Output, unsigned int SIMD);
    void printDefs(const ValueSet &In, const ValueSet &Out, std::string &Output);
    void printPhi(const PhiSet &Set, std::string &Output);
    void printIntraBlock(llvm::BasicBlock &BB, std::string &Output, InsideBlockPressureMap &BBListing);

    public:
    llvm::StringRef getPassName() const override { return "IGCRegPressurePrinter"; }
    virtual ~IGCRegisterPressurePrinter() {}
    virtual bool runOnFunction(llvm::Function &F) override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
        AU.setPreservesAll();
        AU.addRequired<IGCLivenessAnalysis>();
        AU.addRequired<CodeGenContextWrapper>();
        AU.addRequired<WIAnalysis>();
        AU.addRequired<IGCFunctionExternalRegPressureAnalysis>();
    }
    IGCRegisterPressurePrinter();
    IGCRegisterPressurePrinter(const std::string& FileName);
    static char ID;
};

}; // namespace IGC
