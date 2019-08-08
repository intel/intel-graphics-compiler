/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "Compiler/Optimizer/OpenCLPasses/Atomics/ResolveSpinLocks.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constants.h>
#include <llvm/ADT/StringExtras.h>
#include "common/LLVMWarningsPop.hpp"

#include "GenISAIntrinsics/GenIntrinsics.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-resolve-spinlocks"
#define PASS_DESCRIPTION "Resolve spinlocks"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ResolveSpinLocks, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(ResolveSpinLocks, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ResolveSpinLocks::ID = 0;

const llvm::StringRef BUILTIN_GET_LOCAL_LOCK = "__builtin_IB_get_local_lock";
const llvm::StringRef BUILTIN_GET_GLOBAL_LOCK = "__builtin_IB_get_global_lock";

ResolveSpinLocks::ResolveSpinLocks() : ModulePass(ID)
{
    initializeResolveSpinLocksPass(*PassRegistry::getPassRegistry());
}

bool ResolveSpinLocks::runOnModule(Module& M)
{
    m_pModule = &M;

    m_changed = false;

    // Visit all call instuctions in the function F.
    visit(M);

    return m_changed;
}

void ResolveSpinLocks::visitCallInst(CallInst& callInst)
{
    if (!callInst.getCalledFunction())
    {
        return;
    }

    StringRef funcName = callInst.getCalledFunction()->getName();

    if (funcName == BUILTIN_GET_LOCAL_LOCK) {
        processGetLocalLock(callInst);
    }

    if (funcName == BUILTIN_GET_GLOBAL_LOCK) {
        processGetGlobalLock(callInst);
    }
}

// i64 local atomics use a spinlock for emulation. 
// This spinlock needs to be inserted at llvm-ir level, as OpenCL doesn't allow
// local variables in program scope.
void ResolveSpinLocks::processGetLocalLock(CallInst& callInst)
{
    assert(callInst.getCalledFunction()->getName() == BUILTIN_GET_LOCAL_LOCK);
    if (m_localLock == nullptr) {
        Module* M = callInst.getModule();
        auto& C = M->getContext();

        m_localLock = new GlobalVariable(
            *M,
            Type::getInt32Ty(C),
            false,
            GlobalVariable::ExternalLinkage,
            ConstantInt::get(Type::getInt32Ty(C), 0),
            "spinlock",
            nullptr,
            GlobalValue::NotThreadLocal,
            ADDRESS_SPACE_LOCAL);
    }

    callInst.replaceAllUsesWith(m_localLock);
    callInst.eraseFromParent();
    m_changed = true;
}

// i64 global atomics use a spinlock for emulation. 
// This spinlock needs to be inserted at llvm-ir level, as OpenCL doesn't allow
// global variables in program scope.
void ResolveSpinLocks::processGetGlobalLock(CallInst& callInst)
{
    assert(callInst.getCalledFunction()->getName() == BUILTIN_GET_GLOBAL_LOCK);
    if (m_globalLock == nullptr) {
        Module* M = callInst.getModule();
        auto& C = M->getContext();

        m_globalLock = new GlobalVariable(
            *M,
            Type::getInt32Ty(C),
            false,
            GlobalVariable::CommonLinkage,
            ConstantInt::get(Type::getInt32Ty(C), 0),
            "spinlock",
            nullptr,
            GlobalValue::NotThreadLocal,
            ADDRESS_SPACE_GLOBAL);
    }

    callInst.replaceAllUsesWith(m_globalLock);
    callInst.eraseFromParent();
    m_changed = true;
}