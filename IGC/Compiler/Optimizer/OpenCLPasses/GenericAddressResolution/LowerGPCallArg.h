/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GASPropagator.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/CallGraph.h>
#include "common/LLVMWarningsPop.hpp"

using namespace IGC;

namespace IGC {

    llvm::ModulePass* createLowerGPCallArg();

    class LowerGPCallArg : public ModulePass
    {
    public:
        static char ID;

        LowerGPCallArg() : ModulePass(ID)
        {
            initializeLowerGPCallArgPass(*PassRegistry::getPassRegistry());
        }

        bool runOnModule(Module&) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CallGraphWrapperPass>();
            AU.addRequired<LoopInfoWrapperPass>();
        }

        virtual StringRef getPassName() const override
        {
            return "LowerGenericPointerCallArgs";
        }
    private:

        //
        // Functions to be updated.
        // NewArgs keeps track of generic pointer arguments: arg number and address space
        //
        struct ArgDesc {
            unsigned int argNo;
            unsigned int addrSpace;
        };
        using GenericPointerArgs = std::vector<ArgDesc>;

        IGCMD::MetaDataUtils* m_mdUtils = nullptr;
        CodeGenContext* m_ctx = nullptr;
        Module* m_module = nullptr;

        std::optional<unsigned> getOriginAddressSpace(Function* func, unsigned argNo);
        void updateFunctionArgs(Function* oldFunc, Function* newFunc);
        void updateAllUsesWithNewFunction(Function* oldFunc, Function* newFunc);
        void updateMetadata(Function* oldFunc, Function* newFunc);
        Function* createFuncWithLoweredArgs(Function* F, GenericPointerArgs& argsInfo);
        std::vector<Function*> findCandidates(CallGraph& CG);
    };
} // End namespace IGC
