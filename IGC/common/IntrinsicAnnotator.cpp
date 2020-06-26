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

#include "IntrinsicAnnotator.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/FormattedStream.h>
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"

using namespace llvm;
using namespace IGC;
using namespace GenISAIntrinsic;

void IntrinsicAnnotator::emitFunctionAnnot(const Function* func, formatted_raw_ostream& OS)
{
    if (isIntrinsic(func))
    {
        auto ID = getIntrinsicID(func);
        IntrinsicComments comments = getIntrinsicComments(ID);
        OS << "; Function Desc: " << comments.funcDescription << "\n";
        for (auto out : comments.outputs)
        {
            OS << "; Ouput: " << out << "\n";
        }
        for (std::vector<int>::size_type i = 0; i != comments.inputs.size(); i++)
        {
            OS << "; Arg " << i << ": " << comments.inputs[i] << "\n";
        }
    }
}



