/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenPublic.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    struct JointMatrixTypeDescription;

    class JointMatrixFuncsResolutionPass final
        : public llvm::ModulePass
        , public llvm::InstVisitor<JointMatrixFuncsResolutionPass>
    {
    public:
        static char ID;

        JointMatrixFuncsResolutionPass();
        ~JointMatrixFuncsResolutionPass() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "JointMatrixFuncsResolutionPass";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<IGC::MetaDataUtilsWrapper>();
            AU.addRequired<IGC::CodeGenContextWrapper>();
        }

        virtual bool runOnModule(llvm::Module& M) override;
        bool runOnFunction(llvm::Function& F);
        void visitCallInst(llvm::CallInst& CI);
        void visitAllocaInst(llvm::AllocaInst &I);
        void visitGetElementPtrInst(llvm::GetElementPtrInst &I);
        void visitPtrToIntInst(llvm::PtrToIntInst &I);
        void visitStoreInst(llvm::StoreInst &I);
        void visitBitCastInst(llvm::BitCastInst &I);

    private:
        llvm::Instruction *ResolveLoad(llvm::CallInst *CI);
        llvm::Instruction *ResolveStore(llvm::CallInst *CI);
        llvm::Instruction *ResolveMad(llvm::CallInst *CI, unsigned OperationType);
        int getSliceSize(const JointMatrixTypeDescription *desc);
        llvm::Value *ResolveFill(llvm::CallInst *CI);
        llvm::Value *ResolveWILength(llvm::CallInst *CI);
        llvm::Value *ResolveSliceInsert(llvm::CallInst *CI);
        llvm::Value *ResolveSliceExtract(llvm::CallInst *CI);
        llvm::Instruction *ResolveGetCoord(llvm::CallInst *CI);
        llvm::Value *createSliceExtract(llvm::IRBuilder<> *builder, llvm::Value *matrix, llvm::Value *index, const JointMatrixTypeDescription *desc);
        llvm::Value *ResolveCall(llvm::CallInst *CI);
        llvm::Value *ResolveGeneric(llvm::Instruction *OldInst);
        llvm::Value *Resolve(llvm::Value *value);

        bool parseMatrixTypeNameLegacy(const llvm::Type *opaqueType, JointMatrixTypeDescription *outDescription);
        bool ParseMatrixTypeName(llvm::Type *opaqueType, JointMatrixTypeDescription *outDescription);

        unsigned getNumRowsPerWI(const JointMatrixTypeDescription *desc);
        llvm::Type *ResolveType(llvm::Type *opaqueType, JointMatrixTypeDescription *outDesc);
        llvm::Type *ResolveTypes(llvm::Type *t);
        llvm::Type *ResolveStructType(llvm::Type *t);
        llvm::Type *ResolveArrayType(llvm::Type *t);
        llvm::Type *ResolvePointerType(llvm::Type *t);
        void CacheResolvedValue(llvm::Value *oldValue, llvm::Value *newValue);
        void CacheResolvedTypes(llvm::Type *oldType, llvm::Type *newType);
        void InsertPlaceholder(llvm::Value *v);

        std::string GetMatrixFuncName(bool isGetCoord, bool isLoad,
                                      unsigned operationLayout,
                                      unsigned address_space,
                                      const JointMatrixTypeDescription *desc,
                                      std::string prefix);

        bool ValidateLoadStore
            (bool isLoad, unsigned operationLayout, const JointMatrixTypeDescription *desc, llvm::Value *ctx);

        // SIMD Size helpers
        llvm::Function *getEntryFunction(llvm::Function *F);
        void ResolveSIMDSize(llvm::Function *F);
        int32_t DetermineForcedSIMDSize();
        int32_t DefineKernelSIMDSize();
        bool IsSIMDSizeValid(int32_t simdSize);
        void ForceKernelSIMDSize(llvm::Function *F, int32_t forcedSIMDSize);

        llvm::ValueMap<llvm::Value *, llvm::Instruction *> PlaceholderInstructions;
        llvm::ValueMap<llvm::Value *, llvm::Value *> ResolvedValues;
        std::unordered_map<llvm::Type *, llvm::Type *> ResolvedTypes;
        llvm::SmallPtrSet<llvm::Instruction *, 8> InstsToErase;
        // Maps function to it's kernel entry function
        std::unordered_map<llvm::Function *, llvm::Function *> FunctionsMap;

        ModuleMetaData* MMD = nullptr;
        CodeGenContext* m_Ctx = nullptr;
        IGCMD::MetaDataUtils *m_mdUtils = nullptr;
        bool Changed = false;
        int32_t m_SIMDSize = 0;
    };
};
