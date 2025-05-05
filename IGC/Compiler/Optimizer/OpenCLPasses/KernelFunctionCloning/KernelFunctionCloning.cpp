/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/ADT/SmallVector.h>
#include <llvmWrapper/IR/CallSite.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// OpenCL C specification states that a kernel function is just a regular
// function call if a __kernel function is called by another kernel function.
// But, it doesn't clarify what's the behavior exactly. Under certain
// conditions, such a call may be ambiguous without separating kernel function
// and user functions, e.g.
//
// __kernel __attribute__((reqd_work_group_size(1, 1, 1)))
// void bar(...) {
//   ...
// }
//
// __kernel __attribute__((reqd_work_group_size(32, 1, 1)))
// void foo(...) {
//   ...
//   bar(...);
//   ...
// }
//
// Such ambiguity also exists if function call is enabled as well as if there
// are optimizations which may treat kernel function differently from user
// functions, such as inline buffer resolution, which will resolve the first
// local pointer argument at compilation time. If a function is used as both a
// kernel function and a user function. The first local pointer argument is
// ambiguous for optimization to resolve it statically.
//
// OpenCL C++ specification already clarifies the issue and would not allow a
// kernel function to called from another kernel function. However, we still
// need to handle that for OCL 1.2 and OCL 2.0.
//
// This pass is added to clone a kernel function to a user function if it's
// called.
//

namespace {
    class KernelFunctionCloning : public ModulePass {
    public:
        static char ID;

        KernelFunctionCloning() : ModulePass(ID) {}

        bool runOnModule(Module&) override;

        llvm::StringRef getPassName() const override {
            return "KernelFunctionCloning";
        }

    private:
        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
        }
    };

} // End anonymous namespace

namespace IGC {

    ModulePass* createKernelFunctionCloningPass() {
        return new KernelFunctionCloning();
    }

#define PASS_FLAG "igc-kernel-function-cloning"
#define PASS_DESC "Clone kernel functions if it's called."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
    IGC_INITIALIZE_PASS_BEGIN(KernelFunctionCloning, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
        IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
        IGC_INITIALIZE_PASS_END(KernelFunctionCloning, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

} // End IGC namespace

char KernelFunctionCloning::ID = 0;

template <typename PatternTypeFirst, typename... PatternTypeRest>
struct PatternChecker {
    template <typename Checker>
    static bool run(User* user, Checker check) {
        auto casted = dyn_cast<PatternTypeFirst>(user);
        if (!casted)
            return false;

        if constexpr (sizeof...(PatternTypeRest) > 0) {
            for (auto user : casted->users()) {
                if (!PatternChecker<PatternTypeRest...>::run(user, check)) {
                    return false;
                }
            }
        }
        return check(casted);
    }
};

bool KernelFunctionCloning::runOnModule(Module& M) {
    MetaDataUtils* MDU = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    // Collect kernel functions being called.
    SmallVector<Function*, 8> KernelsToClone;
    for (auto& F : M) {
        auto FII = MDU->findFunctionsInfoItem(&F);
        if (FII == MDU->end_FunctionsInfo())
            continue;
        // Check this kernell function is called.
        for (auto* U : F.users()) {
            //
            // Ignore if it's a user semantic decoration on function.
            //
            // GlobalVariable("llvm.global.annotations"):
            //    ConstantArray:
            //       ConstantStruct:
            //          BitCastOperator:
            //             Function = [ANNOTATED_FUNCTION]
            //          GetElementPtr:
            //             GlobalVariable = [ANNOTATION]
            //       ConstantStruct:
            //       ...
            //
            bool user_semantic = PatternChecker<BitCastOperator, ConstantStruct, ConstantArray, GlobalVariable>::run(U, [](User *user) {
                if (auto casted = dyn_cast<GlobalVariable>(user)) {
                    return casted->getName().compare("llvm.global.annotations") == 0;
                }
                return true;
            });

            if (user_semantic) {
                continue;
            }
            IGCLLVM::CallSite* call = nullptr;
#if LLVM_VERSION_MAJOR < 11
            IGCLLVM::CallSite callSite(U);
            call = &callSite;
#else
            call = dyn_cast<IGCLLVM::CallSite>(U);
#endif
            if (!call)
                continue;
            KernelsToClone.push_back(&F);
            break;
        }
    }

    // Clone it
    bool Changed = false;
    for (auto* F : KernelsToClone) {
        ValueToValueMapTy VMap;
        auto* NewF = CloneFunction(F, VMap);
        NewF->setLinkage(GlobalValue::InternalLinkage);
        if (!F->getParent()->getFunction(NewF->getName()))
            F->getParent()->getFunctionList().push_back(NewF);

        // Collect pointers to users (callsites) of the original kernel
        // function and loop through the collection. Otherwise when looping
        // through F->users(), calling call->setCalledFunction(NewF) modifies
        // the F->users() by removing the very first element (second element
        // becomes first) and the loop skips every second element.
        SmallVector<User*, 8> originalKernelFunctionUsers;
        for (auto* U : F->users()) {
              originalKernelFunctionUsers.push_back(U);
        }

        // Replace the original calls to kernel function with calls to user
        // function clone at the callsites.
        for (auto& U : originalKernelFunctionUsers) {
            IGCLLVM::CallSite* call = nullptr;
#if LLVM_VERSION_MAJOR < 11
            IGCLLVM::CallSite callSite(U);
            call = &callSite;
#else
            call = dyn_cast<IGCLLVM::CallSite>(U);
#endif
            if (!call)
                continue;

            if (call->getCalledFunction()->getType() == NewF->getType())
                call->setCalledFunction(NewF);
        }

        Changed = true;
    }

    return Changed;
}
