/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/FixInvalidFuncNamePass.hpp"
#include "IGCPassSupport.h"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Function.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;


// LLVM sometimes creates the function with characters which are incorrect for vISA (e.g. with ".")
// Here we check the LLVM names and replace incorrect characters for vISA to "_"
// We need to check call instruction. If contains invalid char, we change func name called by this instruction
class FixInvalidFuncName : public FunctionPass
{
public:
    FixInvalidFuncName() : FunctionPass(ID) {}
    virtual bool runOnFunction(Function& F) override;
    virtual llvm::StringRef getPassName() const override
    {
        return "Fix Invalid Func Name";
    }
    static char ID;

private:
    // Replace invalid char to underscore
    static std::string replaceInvalidCharToUnderline(std::string str);
};

char FixInvalidFuncName::ID = 0;

bool FixInvalidFuncName::runOnFunction(Function& F)
{
    bool modified = false;
    for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I)
    {
        if (CallInst* callInst = dyn_cast<CallInst>(&(*I)))
        {
            if (callInst->getCallingConv() == CallingConv::SPIR_FUNC)
            {
                Function* func = callInst->getCalledFunction();
                if (func)
                {
                    StringRef original = func->getName();
                    std::string changed = replaceInvalidCharToUnderline(original.str());
                    if (original != changed)
                    {
                        func->setName(changed);
                        modified = true;
                    }
                }
            }
        }
    }
    return modified;
}

std::string FixInvalidFuncName::replaceInvalidCharToUnderline(std::string str)
{
    std::replace(str.begin(), str.end(), '.', '_');
    std::replace(str.begin(), str.end(), '$', '_');
    return str;
}

namespace IGC
{
#define PASS_FLAG "fix-invalid-func-name"
#define PASS_DESCRIPTION "Fix Invalid Func Name Pass"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(FixInvalidFuncName, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(FixInvalidFuncName, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

    FunctionPass* createFixInvalidFuncNamePass()
    {
        return new FixInvalidFuncName();
    }
}
