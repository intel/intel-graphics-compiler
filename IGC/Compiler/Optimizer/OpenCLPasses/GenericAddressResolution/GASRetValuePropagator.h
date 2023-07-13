/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GASPropagator.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Analysis/CallGraph.h"
#include "common/LLVMWarningsPop.hpp"

using namespace IGC;

namespace IGC {

    llvm::ModulePass* createGASRetValuePropagatorPass();

    class GASRetValuePropagator : public ModulePass {
        Module* m_module = nullptr;
        IGCMD::MetaDataUtils* m_mdUtils = nullptr;
        CodeGenContext* m_ctx = nullptr;
        GASPropagator* m_Propagator = nullptr;

    public:
        static char ID;

        GASRetValuePropagator() : ModulePass(ID) {
            initializeGASRetValuePropagatorPass(*PassRegistry::getPassRegistry());
        }

        bool runOnModule(Module&) override;

        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<CallGraphWrapperPass>();
            AU.addRequired<LoopInfoWrapperPass>();
        }

        bool propagateReturnValue(Function*&);
        std::vector<Function*> findCandidates(CallGraph&);

    private:
        std::vector<ReturnInst*> getAllRetInstructions(Function&);
        void updateFunctionRetInstruction(Function*);
        PointerType* getRetValueNonGASType(Function*);
        void transferFunctionBody(Function*, Function*);
        void updateAllUsesWithNewFunction(Function*, Function*);
        void updateMetadata(Function*, Function*);
        void updateDwarfAddressSpace(Function *);
        DIDerivedType *getDIDerivedTypeWithDwarfAddrspace(DIDerivedType *, unsigned);
        Function* createNewFunctionDecl(Function*, Type*);
        Function* cloneFunctionWithModifiedRetType(Function*, PointerType*);
    };
} // End namespace IGC
