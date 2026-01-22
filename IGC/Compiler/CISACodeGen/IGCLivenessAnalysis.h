/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"
#include "DebugInfo/VISAIDebugEmitter.hpp"
#include "DebugInfo/VISAModule.hpp"
#include "Probe/Assertion.h"
#include "ShaderCodeGen.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Config/llvm-config.h>
#include <llvm/ADT/PostOrderIterator.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

using namespace IGC;

namespace IGC {

typedef std::unordered_map<llvm::Value *, unsigned int> InclusionSet;
typedef llvm::SmallPtrSet<llvm::Value *, 32> ValueSet;
typedef llvm::SmallPtrSet<llvm::BasicBlock *, 32> BBSet;
typedef std::unordered_map<llvm::BasicBlock *, ValueSet> DFSet;
typedef std::unordered_map<llvm::BasicBlock *, ValueSet> PhiSet;
typedef std::unordered_map<llvm::BasicBlock *, PhiSet> InPhiSet;
typedef std::unordered_map<llvm::Value *, unsigned int> InsideBlockPressureMap;

class IGCLivenessAnalysisBase {
public:
  // contains all values that liveIn into this block
  DFSet In;
  // this is a redundant set for visualization purposes,
  // contains all values that go into PHIs grouped
  // by the block from which they are coming
  InPhiSet InPhi;
  // contains all of the values that liveOut out of this block
  DFSet Out;

  IGC::CodeGenContext *CGCtx = nullptr;
  IGCMD::MetaDataUtils *MDUtils = nullptr;
  GenXFunctionGroupAnalysis *FGA = nullptr;

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

  unsigned int estimateSizeInBytes(ValueSet &Set, llvm::Function &F, unsigned int SIMD, WIAnalysisRunner *WI = nullptr);
  void collectPressureForBB(llvm::BasicBlock &BB, InsideBlockPressureMap &BBListing, unsigned int SIMD,
                            WIAnalysisRunner *WI = nullptr);

  SIMDMode bestGuessSIMDSize(Function *F = nullptr, GenXFunctionGroupAnalysis *FGA = nullptr);

  unsigned int bytesToRegisters(unsigned int Bytes) {
    unsigned int RegisterSizeInBytes = registerSizeInBytes();
    unsigned int AmountOfRegistersRoundUp = (Bytes + RegisterSizeInBytes - 1) / RegisterSizeInBytes;
    return AmountOfRegistersRoundUp;
  }

  unsigned int registerSizeInBytes();
  void mergeSets(ValueSet *OutSet, llvm::BasicBlock *Succ);
  void combineOut(llvm::BasicBlock *BB);
  void addToPhiSet(llvm::PHINode *Phi, PhiSet *InPhiSet);
  unsigned int addOperandsToSet(llvm::Instruction *Inst, ValueSet &Set, unsigned int SIMD, WIAnalysisRunner *WI,
                                const DataLayout &DL);
  void addNonLocalOperandsToSet(llvm::Instruction *Inst, ValueSet &Set);
  void processBlock(llvm::BasicBlock *BB, ValueSet &Set, PhiSet *PhiSet);
  void livenessAnalysis(llvm::Function &F, BBSet *StartBBs = nullptr);
};

class IGCLivenessAnalysisRunner : public IGCLivenessAnalysisBase {
public:
  IGCLivenessAnalysisRunner() = default;
  IGCLivenessAnalysisRunner(IGC::CodeGenContext *CGCtx, IGCMD::MetaDataUtils *MDUtils, GenXFunctionGroupAnalysis *FGA) {
    this->CGCtx = CGCtx;
    this->MDUtils = MDUtils;
    this->FGA = FGA;
  }

  void publishRegPressureMetadata(llvm::Function &F, unsigned int MaxPressure) {
    publishRegPressureMetadata(F, MaxPressure, MDUtils);
  }

  static void publishRegPressureMetadata(llvm::Function &F, unsigned int MaxPressure,
                                         IGC::IGCMD::MetaDataUtils *MDUtils) {
    if (MDUtils->findFunctionsInfoItem(&F) != MDUtils->end_FunctionsInfo()) {
      IGC::IGCMD::FunctionInfoMetaDataHandle funcInfoMD = MDUtils->getFunctionsInfoItem(&F);
      funcInfoMD->getMaxRegPressure()->setMaxPressure(MaxPressure);
      MDUtils->save(F.getContext());
    }
  }

  unsigned checkPublishRegPressureMetadata(llvm::Function &F) { return checkPublishRegPressureMetadata(F, MDUtils); }

  static unsigned checkPublishRegPressureMetadata(llvm::Function &F, IGC::IGCMD::MetaDataUtils *MDUtils) {
    unsigned Result = 0;
    if (MDUtils->findFunctionsInfoItem(&F) != MDUtils->end_FunctionsInfo()) {
      IGC::IGCMD::FunctionInfoMetaDataHandle funcInfoMD = MDUtils->getFunctionsInfoItem(&F);
      Result = funcInfoMD->getMaxRegPressure()->getMaxPressure();
    }
    return Result;
  }

  unsigned int getMaxRegCountForBB(llvm::BasicBlock &BB, unsigned int SIMD, WIAnalysisRunner *WI = nullptr) {
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
  unsigned int getMaxRegCountForFunction(llvm::Function &F, unsigned int SIMD, WIAnalysisRunner *WI = nullptr) {
    unsigned int Max = 0;
    for (BasicBlock &BB : F) {
      Max = std::max(getMaxRegCountForBB(BB, SIMD, WI), Max);
    }
    return Max;
  }

  unsigned int getMaxRegCountForLoop(llvm::Loop &L, unsigned int SIMD, WIAnalysisRunner *WI = nullptr) {
    unsigned int Max = 0;
    for (BasicBlock *BB : L.getBlocks()) {
      unsigned int BBPressure = getMaxRegCountForBB(*BB, SIMD, WI);
      Max = std::max(BBPressure, Max);
    }
    return Max;
  }

  llvm::BasicBlock *getMaxRegCountBBForFunction(llvm::Function &F, WIAnalysisRunner *WI = nullptr) {
    llvm::BasicBlock *HottestBB = NULL;
    unsigned int Max = 0;
    for (BasicBlock &BB : F) {
      unsigned int BBPressure = getMaxRegCountForBB(BB, 8, WI);
      HottestBB = BBPressure > Max ? &BB : HottestBB;
      Max = std::max(BBPressure, Max);
    }
    return HottestBB;
  }

  void releaseMemory() {
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
    if (BBs != nullptr) {
      for (BasicBlock *BB : *BBs) {
        In[BB].clear();
        Out[BB].clear();
        InPhi[BB].clear();
      }
    } else {
      releaseMemory();
    }
    livenessAnalysis(F, BBs);
  }
};

class IGCLivenessAnalysis : public llvm::FunctionPass {
public:
  static char ID;
  llvm::StringRef getPassName() const override { return "IGCLivenessAnalysis"; }
  IGCLivenessAnalysis();
  virtual ~IGCLivenessAnalysis() {}
  bool runOnFunction(llvm::Function &F) override;
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesAll();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<MetaDataUtilsWrapper>();
  }
  void releaseMemory() override { LivenessRunner.releaseMemory(); }

  IGCLivenessAnalysisRunner &getLivenessRunner() { return LivenessRunner; }

private:
  IGCLivenessAnalysisRunner LivenessRunner;
};

typedef std::unordered_map<llvm::CallInst *, unsigned int> CallSiteToPressureMap;
class IGCFunctionExternalRegPressureAnalysis : public llvm::ModulePass, public IGCLivenessAnalysisBase {
  // this map contains external pressure for a function
  std::unordered_map<Function *, unsigned int> ExternalFunctionPressure;
  // this map contains all the callsites in the module and their pressure
  CallSiteToPressureMap CallSitePressure;
  // contains all spir_func definitions inside our module, to check against
  // if we have 0 of them, we don't have to compute external pressure
  llvm::SmallPtrSet<llvm::Function *, 32> SetOfDefinitions;
  // caches WIAnalysisRunner result for functions so it doesn't have to be
  // recomputed in following passes.
  std::unordered_map<llvm::Function *, WIAnalysisRunner> WIAnalysisMap;

  // already present in IGCLivenessAnalysisBase
  // IGC::CodeGenContext *CGCtx = nullptr;
  ModuleMetaData *ModMD = nullptr;

  WIAnalysisRunner &runWIAnalysis(Function &F);
  void generateTableOfPressure(Module &M);

public:
  static char ID;
  llvm::StringRef getPassName() const override { return "FunctionExternalPressure"; }

  std::unique_ptr<InsideBlockPressureMap> getPressureMapForBB(llvm::BasicBlock &BB, unsigned int SIMD,
                                                              WIAnalysisRunner &WI) {
    std::unique_ptr<InsideBlockPressureMap> PressureMap = std::make_unique<InsideBlockPressureMap>();
    collectPressureForBB(BB, *PressureMap, SIMD, &WI);
    return PressureMap;
  }

  // returns pressure in registers
  unsigned int getExternalPressureForFunction(llvm::Function *F) {
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
    WIAnalysisMap.clear();
  }

  WIAnalysisRunner &getWIAnalysis(Function *F) { return WIAnalysisMap[F]; }
};

class IGCRegisterPressurePrinter : public llvm::FunctionPass {

  IGCLivenessAnalysisRunner *RPE = nullptr;
  WIAnalysis *WI = nullptr;
  CodeGenContext *CGCtx = nullptr;
  GenXFunctionGroupAnalysis *FGA = nullptr;

  bool DumpToFile = false;
  std::string DumpFileName = "default";
  // controls printer verbocity
  // 1 -> print instruction dump
  // 2 -> print with phi listing
  // 3 -> print with ssa value names DEF, KILL, IN, OUT
  unsigned int PrinterType = IGC_GET_FLAG_VALUE(RegPressureVerbocity);
  // maximum potential calling context pressure of a function
  unsigned int ExternalPressure = 0;
  unsigned int MaxPressureInFunction = 0;

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
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<IGCLivenessAnalysis>();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<WIAnalysis>();
    AU.addRequired<IGCFunctionExternalRegPressureAnalysis>();
  }
  IGCRegisterPressurePrinter();
  IGCRegisterPressurePrinter(const std::string &FileName);
  static char ID;
};

class IGCRegisterPressurePublisher : public llvm::ModulePass {
public:
  llvm::StringRef getPassName() const override { return "IGCRegPressurePublisher"; }
  virtual ~IGCRegisterPressurePublisher() {}
  bool runOnModule(llvm::Module &M) override;
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesAll();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<IGCFunctionExternalRegPressureAnalysis>();
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<PostDominatorTreeWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
  }
  IGCRegisterPressurePublisher();
  static char ID;
};

}; // namespace IGC
