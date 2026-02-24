/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/VectorShuffleAnalysis.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DenseMap.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

class LatencyHidingAnalysis : public llvm::FunctionPass {
public:
  static char ID;

  // Analysis results - public so other passes can access via getAnalysis<>().

  struct LoadAnalysisInfo {
    llvm::Instruction *LoadInst;
    bool IsDecomposed;                            // ReadAddrPayload variant
    int SetupInsts;                               // # SetField + CreatePayload before this load
    int Distance;                                 // instruction distance to closest consumer DPAS
    int DpasBetween;                              // # DPAS between load and closest consumer (-1 = no consumer in BB)
    int LoadsBetween;                             // # 2D block loads between load and closest consumer
    int OtherBetween;                             // # other hiding insts (fmul/fadd/fptrunc/maxnum/exp2/WaveAll)
    int TotalConsumers;                           // number of DPAS using this load
    bool HasNonContiguousShuffle;                 // non-NoOp shuffle -> DpasBetween forced to 0
    bool HasNonDPASConsumers;                     // non-DPAS, non-IE/EE users exist
    int ShuffleInsts;                             // # IE/EE instructions in shuffle chains
    llvm::Instruction *ClosestConsumer = nullptr; // first consumer DPAS in BB order
  };

  struct BBAnalysisResult {
    std::string BBName;
    int NumLoads;
    int NumDPAS;
    int TotalInstructions;
    int NumNonContiguousShuffleLoads; // loads requiring real MOV shuffles
    std::vector<LoadAnalysisInfo> Loads;
    int DpasHidingSum;    // sum of dpas_between across valid loads
    int LoadHidingSum;    // sum of loads_between across valid loads
    int OtherHidingSum;   // sum of other_between across valid loads
    int LoadOrderPenalty; // inversion count vs ideal DPAS-driven load order

    struct LoadOrderInversion {
      int EarlyLoadIdx; // index in Loads[] placed earlier (wrongly)
      int LateLoadIdx;  // index in Loads[] that should have been earlier
      std::string Reason;
    };
    std::vector<LoadOrderInversion> LoadOrderInversions; // verbose details
  };

  LatencyHidingAnalysis();
  LatencyHidingAnalysis(const std::string &Tag, int Verbose = 1);

  virtual bool runOnFunction(llvm::Function &F) override;

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesAll();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<VectorShuffleAnalysis>();
  }

  virtual llvm::StringRef getPassName() const override { return "LatencyHidingAnalysis"; }

  /// Access all BB results after the pass has run.
  const std::vector<BBAnalysisResult> &getResults() const { return FunctionResults; }

  /// Look up the result for a specific basic block (nullptr if BB was skipped).
  const BBAnalysisResult *getBBResult(llvm::BasicBlock *BB) const {
    auto It = BBToResultIdx.find(BB);
    if (It == BBToResultIdx.end())
      return nullptr;
    return &FunctionResults[It->second];
  }

private:
  std::string DumpTag = "afterCS";
  int VerboseLevel = 1;
  bool ShouldDump = false;
  bool FirstDump = true;
  CodeGenContext *CTX = nullptr;
  VectorShuffleAnalysis *VSA = nullptr;

  std::vector<BBAnalysisResult> FunctionResults;
  llvm::DenseMap<llvm::BasicBlock *, size_t> BBToResultIdx;

  BBAnalysisResult analyzeBB(llvm::BasicBlock &BB);
  void dumpResults(llvm::Function &F);
};

void initializeLatencyHidingAnalysisPass(llvm::PassRegistry &);

} // namespace IGC
