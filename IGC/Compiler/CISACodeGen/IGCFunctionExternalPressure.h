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
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/IR/DIBuilder.h"
#include "common/LLVMWarningsPop.hpp"

using namespace IGC;
using namespace llvm;

typedef std::unordered_map<llvm::Value *, unsigned int> InclusionSet;
typedef llvm::SmallPtrSet<llvm::Value *, 32> ValueSet;
typedef llvm::SmallPtrSet<llvm::BasicBlock *, 32> BBSet;
typedef std::unordered_map<llvm::BasicBlock *, ValueSet> DFSet;
typedef std::unordered_map<llvm::BasicBlock *, ValueSet> PhiSet;
typedef std::unordered_map<llvm::BasicBlock *, PhiSet> InPhiSet;
typedef std::unordered_map<llvm::Value *, unsigned int> InsideBlockPressureMap;

namespace IGC {

typedef std::unordered_map<llvm::CallInst *, unsigned int> CallSiteToPressureMap;
class IGCFunctionExternalRegPressureAnalysis : public llvm::ModulePass {
    // this map contains external pressure for a function
    std::unordered_map<Function *, unsigned int> ExternalFunctionPressure;
    // this map contains all the callsites in the module and their pressure
    CallSiteToPressureMap CallSitePressure;
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

    IGC::CodeGenContext *CGCtx = nullptr;
    IGCMD::MetaDataUtils* MDUtils = nullptr;
    ModuleMetaData* ModMD = nullptr;

    std::unique_ptr<WIAnalysisRunner> runWIAnalysis(Function &F);
    void generateTableOfPressure(Module &M, unsigned int SIMD);

    unsigned int estimateSizeInBytes(ValueSet &Set, Function &F, unsigned int SIMD, WIAnalysisRunner &WI);

    void collectPressureForBB( llvm::BasicBlock &BB, InsideBlockPressureMap &BBListing, unsigned int SIMD, WIAnalysisRunner& WI);
    void mergeSets(ValueSet *OutSet, llvm::BasicBlock *Succ);
    void combineOut(llvm::BasicBlock *BB, ValueSet *Set);
    void addToPhiSet(llvm::PHINode *Phi, PhiSet *InPhiSet);
    void processBlock(llvm::BasicBlock *BB, ValueSet &Set, PhiSet *PhiSet);
    void livenessAnalysis(llvm::Function &F);
    void addOperandsToSet(llvm::Instruction *Inst, ValueSet &Set);
    void addNonLocalOperandsToSet(llvm::Instruction *Inst, ValueSet &Set);


    unsigned int registerSizeInBytes() {
        if (CGCtx->platform.isProductChildOf(IGFX_PVC))
            return 64;
        return 32;
    }

    SIMDMode bestGuessSIMDSize() {
        switch (IGC_GET_FLAG_VALUE(ForceOCLSIMDWidth)) {
            case 0:
                break;
            case 8:
                return SIMDMode::SIMD8;
            case 16:
                return SIMDMode::SIMD16;
            case 32:
                return SIMDMode::SIMD32;
        }

        if (CGCtx->platform.isProductChildOf(IGFX_PVC))
            return SIMDMode::SIMD32;
        return SIMDMode::SIMD8;
    }

    unsigned int bytesToRegisters(unsigned int Bytes) {
        unsigned int RegisterSizeInBytes = registerSizeInBytes();
        unsigned int AmountOfRegistersRoundUp =
            (Bytes + RegisterSizeInBytes - 1) / RegisterSizeInBytes;
        return AmountOfRegistersRoundUp;
    }

    std::unique_ptr<InsideBlockPressureMap> getPressureMapForBB(llvm::BasicBlock &BB,
                                               unsigned int SIMD, WIAnalysisRunner& WI) {
        std::unique_ptr<InsideBlockPressureMap> PressureMap = std::make_unique<InsideBlockPressureMap>();
        collectPressureForBB(BB, *PressureMap, SIMD, WI);
        return PressureMap;
    }


  public:
    static char ID;
    llvm::StringRef getPassName() const override {
        return "FunctionExternalPressure";
    }

    // returns pressure in registers
    unsigned int getExternalPressureForFunction(llvm::Function* F) {
        unsigned int Registers = bytesToRegisters(ExternalFunctionPressure[F]);
        return Registers;
    }

    IGCFunctionExternalRegPressureAnalysis();
    virtual ~IGCFunctionExternalRegPressureAnalysis() {}

    bool runOnModule(llvm::Module &M) override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
        AU.setPreservesAll();

        AU.addRequired<CallGraphWrapperPass>();
        AU.addRequired<CodeGenContextWrapper>();
        AU.addRequired<MetaDataUtilsWrapper>();

        AU.addRequired<DominatorTreeWrapperPass>();
        AU.addRequired<PostDominatorTreeWrapperPass>();
        AU.addRequired<LoopInfoWrapperPass>();

    }
    void releaseMemory() override {
        In.clear();
        InPhi.clear();
        Out.clear();
        ExternalFunctionPressure.clear();
        CallSitePressure.clear();
    }
};

}
