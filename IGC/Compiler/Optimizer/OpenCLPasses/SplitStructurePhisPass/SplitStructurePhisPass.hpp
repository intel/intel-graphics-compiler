/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/helper.h"

namespace IGC
{
typedef std::tuple<size_t, size_t> IndicesTuple;
typedef std::map<size_t, llvm::ExtractValueInst*> ExtractValueMap;
typedef std::map<size_t, llvm::InsertValueInst*> IncomingValuesMap;

// Constant values to describe zeroinitializer and non-zeroinitializer indices of phi incoming values.
const size_t Zero = 0;
const size_t NonZero = 1;

class SplitStructurePhisPass : public llvm::FunctionPass
    {
    public:
        static char ID;

        SplitStructurePhisPass();

        virtual llvm::StringRef getPassName() const override {
            return "Split structure phis";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
        }

        virtual bool runOnFunction(llvm::Function &F) override;
    private:
        IndicesTuple getIndices(llvm::PHINode *Phi);

        bool isStructOfVectorsType(llvm::Type* Ty);

        bool checkNonZeroIncValue(llvm::Value *IncVal, IncomingValuesMap &InsertValues);
        bool isPhiNodeParsedByExtrVal(llvm::PHINode *Phi, ExtractValueMap &ExtractValues);
        bool isLastInsertValueInst(llvm::InsertValueInst *InsertValInst, llvm::PHINode *OldPhi);

        void createVectorPhi(llvm::PHINode *OldPhi, const IndicesTuple &Indices, llvm::ExtractValueInst *ExtractInst, llvm::InsertValueInst *InsertVal);
        void createScalarPhi(llvm::PHINode *OldPhi, llvm::Type *NewScalarType, const IndicesTuple &Indices,  llvm::ExtractValueInst *OldExtractInst, llvm::InsertValueInst *OldInsertValInst);

        void cleanUp();

        // Phi nodes and their zeroinitializer and non-zeroinitializer indices.
        llvm::DenseMap<llvm::PHINode*, std::tuple<IndicesTuple, ExtractValueMap, IncomingValuesMap>> PhiNodes;
        llvm::SmallSet<llvm::InsertValueInst*, 8> InsertValueInstsToRemove;
        llvm::SmallSet<llvm::ExtractValueInst*, 8> ExtractValueInstsToRemove;
        llvm::SmallSet<llvm::PHINode*, 8> PhiNodeInstsToRemove;
    };
}