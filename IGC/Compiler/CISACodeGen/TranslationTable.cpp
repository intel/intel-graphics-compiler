/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/TranslationTable.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstIterator.h>
#include "common/LLVMWarningsPop.hpp"


using namespace IGC;
using namespace llvm;

char TranslationTable::ID = 0;

#define PASS_FLAG "TranslationTable"
#define PASS_DESCRIPTION "Compute-once pass that provides a translation table for mapping attributes for LLVM values"
#define PASS_CFG_ONLY true
#define PASS_ANALYSIS true

IGC_INITIALIZE_PASS_BEGIN(TranslationTable, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(TranslationTable, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

namespace IGC
{

    TranslationTable::TranslationTable() : FunctionPass(ID)
    {
        initializeTranslationTablePass(*PassRegistry::getPassRegistry());
    }

    bool TranslationTable::runOnFunction(Function& F)
    {
        (void)run(F);
        return false;
    }

    bool TranslationTable::run(Function& F)
    {
        unsigned int counter = 0;

        //initialize all arguments
        for (llvm::Function::arg_iterator funcArg = F.arg_begin(), funcArge = F.arg_end();
            funcArg != funcArge; ++funcArg)
        {
            counter++;
        }

        inst_iterator it = inst_begin(F);
        inst_iterator  e = inst_end(F);
        for (; it != e; ++it)
        {
            counter++;
        }

        m_NumIDS = counter;

        return true;
    }

    void TranslationTable::RegisterNewValueAndAssignID(Value* val)
    {
        m_NumIDS++;

        auto cvi = m_ValueMaps.begin();
        auto cve = m_ValueMaps.end();
        for (; cvi != cve; ++cvi)
        {
            (*cvi)->Update();
        }
    }
} //namespace IGC
