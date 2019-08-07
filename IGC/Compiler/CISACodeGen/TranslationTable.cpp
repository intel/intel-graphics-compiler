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
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
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
        return run(F);
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
