/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

#include "Probe/Assertion.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"

using namespace IGC;

namespace IGC {

class DestVector;
class VectorShufflePattern;

typedef llvm::SmallPtrSet<llvm::Instruction *, 32> ShuffleInstSet;
typedef llvm::SmallVector<llvm::InsertElementInst *, 4> IESet;
typedef llvm::SmallDenseMap<llvm::Value *, VectorShufflePattern> SourceVecToVectorShuffleMap;
typedef llvm::DenseMap<llvm::InsertElementInst *, ShuffleInstSet> DestVecToInstMap;

// DescVector represents the vector that is created by a set of InsertElementInst in IR
// If the vector is created by a vector shuffle, SourceVec is not nullptr, use getSourceVec to get the source
// If the vector is created by a set of InsertElementInst from scalars, SourceVec is nullptr, use getSourceScalars to
// get the scalars
class DestVector {
  public:
    DestVector() = delete;

    DestVector(llvm::Value *SourceVec, llvm::InsertElementInst *DestVec, std::vector<int> ShuffleMask,
        std::vector<llvm::InsertElementInst *> IEs, std::vector<llvm::ExtractElementInst *> EEs,
        std::vector<llvm::Value *> SourceScalars)
        : SourceVec(SourceVec), DestVec(DestVec), ShuffleMask(std::move(ShuffleMask)), IEs(std::move(IEs)), EEs(std::move(EEs)),
          SourceScalars(std::move(SourceScalars)){};

    llvm::Value *SourceVec;
    llvm::InsertElementInst *DestVec; // first IE that creates a new vec (writes to Undef)
    std::vector<int> ShuffleMask;
    std::vector<llvm::InsertElementInst *> IEs;
    std::vector<llvm::ExtractElementInst *> EEs;
    std::vector<llvm::Value *> SourceScalars;

    bool isDirectContiguous() const {
        if (!isVectorShuffle())
            return false;

        int First = ShuffleMask.front();
        for (uint i = 1; i < ShuffleMask.size(); i++) {
            if (ShuffleMask[i] != First + i)
                return false;
        }
        return true;
    }

    bool isNoOp() const { return isVectorShuffle() && isDirectContiguous(); }

    llvm::InsertElementInst *getFirstIE() const { return DestVec; }

    llvm::ExtractElementInst *getFirstEE() const {
        llvm::ExtractElementInst *EE = llvm::dyn_cast<llvm::ExtractElementInst>(getFirstIE()->getOperand(1));
        IGC_ASSERT(EE);
        return EE;
    }

    llvm::InsertElementInst *getLastIE() const { return IEs.back(); }

    llvm::ExtractElementInst *getLastEE() const {
        llvm::ExtractElementInst *EE = llvm::dyn_cast<llvm::ExtractElementInst>(getLastIE()->getOperand(1));
        IGC_ASSERT(EE);
        return EE;
    }

    llvm::Value *getSourceVec() const { return SourceVec; }

    const std::vector<llvm::InsertElementInst *> &getIEs() const { return IEs; }

    bool isVectorShuffle() const { return SourceVec != nullptr; }

    std::vector<llvm::Value *> getSourceScalars() const { return SourceScalars; }
};

class VectorToScalarsPattern {
  public:
    VectorToScalarsPattern(llvm::Value *SourceVec, std::vector<llvm::ExtractElementInst *> EEs, bool AllUsesAreScalars)
        : SourceVec(SourceVec), EEs(std::move(EEs)), AllUsesAreScalars(AllUsesAreScalars){};

    bool areAllUsesScalars() const { return AllUsesAreScalars; }

    llvm::Value *getSourceVec() const { return SourceVec; }

    const std::vector<llvm::ExtractElementInst *> &getEEs() const { return EEs; }

  private:
    VectorToScalarsPattern() = delete;

    llvm::Value *SourceVec;
    std::vector<llvm::ExtractElementInst *> EEs;
    bool AllUsesAreScalars;
};

class VectorShuffleAnalysis : public llvm::FunctionPass {
  public:
    static char ID;
    virtual llvm::StringRef getPassName() const override { return "VectorShuffleAnalysis"; };
    VectorShuffleAnalysis();
    virtual ~VectorShuffleAnalysis() {}
    VectorShuffleAnalysis(const VectorShuffleAnalysis &) = delete;
    virtual bool runOnFunction(llvm::Function &F) override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.setPreservesAll(); }

    DestVector *getDestVector(llvm::Value *IE) {
        auto It = ValueToDestVecMap.find(IE);
        if (It != ValueToDestVecMap.end())
            return It->second;
        return nullptr;
    }

    std::vector<DestVector *> getDestVectorsForSourceVector(llvm::Value *SourceVec) {
        auto It = DestVectorsForSourceVector.find(SourceVec);
        if (It != DestVectorsForSourceVector.end())
            return It->second;
        return {};
    }

    VectorToScalarsPattern *getVectorToScalarsPattern(llvm::Value *EE) {
        auto It = VectorToScalarsPatternsMap.find(EE);
        if (It != VectorToScalarsPatternsMap.end())
            return It->second;
        return nullptr;
    }

  private:
    std::vector<std::unique_ptr<DestVector>> DestVectors;
    std::vector<std::unique_ptr<VectorToScalarsPattern>> VectorToScalarsPatterns;
    llvm::DenseMap<llvm::Value *, std::vector<DestVector *>> DestVectorsForSourceVector;
    std::unique_ptr<DestVector> tryCreatingDestVectorForShufflePattern(llvm::InsertElementInst *IE);
    std::unique_ptr<DestVector> tryCreatingDestVectorForVectorization(llvm::InsertElementInst *IE);
    std::unique_ptr<VectorToScalarsPattern> tryCreatingVectorToScalarsPattern(llvm::ExtractElementInst *EE);
    llvm::DenseMap<llvm::Value *, DestVector *> ValueToDestVecMap;
    llvm::DenseMap<llvm::Value *, VectorToScalarsPattern *> VectorToScalarsPatternsMap;
};

}; // namespace IGC
