/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _CISA_MEMOPT2_H_
#define _CISA_MEMOPT2_H_

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DenseSet.h>
#include <llvm/Analysis/BasicAliasAnalysis.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>

#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"

class MemInstCluster {
  IGC::CodeGenContext *CTX = nullptr;
  const DataLayout *DL = nullptr;
  AliasAnalysis *AA = nullptr;
  unsigned MaxLiveOutThreshold = 0;
  llvm::DenseSet<Instruction *> Scheduled;

public:
  MemInstCluster() {}
  ~MemInstCluster() {}

  MemInstCluster(IGC::CodeGenContext *pCTX, const DataLayout *pDL,
                 AliasAnalysis *pAA, unsigned MLT) {
    init(pCTX, pDL, pAA, MLT);
  }
  void init(IGC::CodeGenContext *pCTX, const DataLayout *pDL,
            AliasAnalysis *pAA, unsigned MLT) {
    CTX = pCTX;
    DL = pDL;
    AA = pAA;
    MaxLiveOutThreshold = MLT;
  }
  ///  Called by MemOpt2 to cluster GPGPU kernels
  bool runForOCL(Function &F);
  ///  called by AdvMemOpt to cluster 3D sample-ops
  bool runForGFX(BasicBlock *BB);

private:
  bool clusterSampler(BasicBlock *BB);

  bool clusterMediaBlockRead(BasicBlock *BB);

  bool isSafeToMoveTo(Instruction *I, Instruction *Pos,
                      const SmallVectorImpl<Instruction *> *CheckList) const;

  bool clusterLoad(BasicBlock *BB);
  bool isDefinedBefore(BasicBlock *BB, Instruction *I, Instruction *Pos) const;
  bool
  isSafeToScheduleLoad(const LoadInst *LD,
                       const SmallVectorImpl<Instruction *> *CheckList) const;
  bool schedule(BasicBlock *BB, Value *V, Instruction *&InsertPos,
                const SmallVectorImpl<Instruction *> *CheckList = nullptr);

  MemoryLocation getLocation(Instruction *I) const {
    if (LoadInst *LD = dyn_cast<LoadInst>(I))
      return MemoryLocation::get(LD);
    if (StoreInst *ST = dyn_cast<StoreInst>(I))
      return MemoryLocation::get(ST);
    return MemoryLocation();
  }

  unsigned getNumLiveOuts(Instruction *I) const;

  unsigned getNumLiveOutBytes(Instruction *I) const;

  unsigned getMaxLiveOutThreshold() const {
    static unsigned MaxLiveOutThreshold =
        IGC_GET_FLAG_VALUE(MaxLiveOutThreshold)
            ? IGC_GET_FLAG_VALUE(MaxLiveOutThreshold)
            : 4;
    return MaxLiveOutThreshold;
  }
};

void initializeMemOpt2Pass(llvm::PassRegistry &);
llvm::FunctionPass *createMemOpt2Pass(int MLT = -1);

#endif // _CISA_MEMOPT2_H_
