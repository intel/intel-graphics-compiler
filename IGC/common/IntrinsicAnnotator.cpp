/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IntrinsicAnnotator.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/FormattedStream.h>
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"

#include <sstream>

using namespace llvm;
using namespace IGC;
using namespace GenISAIntrinsic;

void IntrinsicAnnotator::emitFunctionAnnot(const Function* func, formatted_raw_ostream& OS)
{
    if (isIntrinsic(func))
    {
        auto AddCommentPrefixes = [](const char* in)
            {
                std::stringstream ss(in);
                std::string content;
                if (std::getline(ss, content, '\n'))
                {
                    std::string line;
                    while (std::getline(ss, line, '\n')) {
                        content += "\n; " + line;
                    }
                }
                return content;
            };

        auto ID = getIntrinsicID(func);
        IntrinsicComments comments = getIntrinsicComments(ID);
        OS << "; Function Desc: " << AddCommentPrefixes(comments.funcDescription) << "\n";
        for (auto out : comments.outputs)
        {
            OS << "; Output: " << AddCommentPrefixes(out) << "\n";
        }
        for (std::vector<int>::size_type i = 0; i != comments.inputs.size(); i++)
        {
            OS << "; Arg " << i << ": " << AddCommentPrefixes(comments.inputs[i]) << "\n";
        }
    }
}



