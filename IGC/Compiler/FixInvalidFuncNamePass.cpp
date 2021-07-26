/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/FixInvalidFuncNamePass.hpp"
#include "IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Function.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

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
    // LLVM sometimes creates the function with characters which are incorrect for vISA (e.g. with ".")
    // Here we check the LLVM names and replace incorrect characters for vISA to "_"
    bool changeInvalidFuncName(Function& F);
};

char FixInvalidFuncName::ID = 0;

bool FixInvalidFuncName::runOnFunction(Function& F)
{
    return changeInvalidFuncName(F);
}

bool FixInvalidFuncName::changeInvalidFuncName(Function& F)
{
    auto replaceInvalidCharToUnderline = [](std::string str) {
        std::replace(str.begin(), str.end(), '.', '_');
        std::replace(str.begin(), str.end(), '$', '_');
        return str;
    };

    std::string str = F.getName();
    str = replaceInvalidCharToUnderline(str);
    if (str != F.getName())
    {
        F.setName(str);
        return true;
    }
    return false;
}

namespace IGC
{
#define PASS_FLAG "fix-invalid-func-name"
#define PASS_DESCRIPTION "Fix Invalid Func Name Pass"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(FixInvalidFuncName, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(FixInvalidFuncName, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

    FunctionPass* createFixInvalidFuncNamePass()
    {
        return new FixInvalidFuncName();
    }
}