/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/PassTimer.hpp"

char PassTimer::ID = 0;

bool PassTimer::runOnModule(llvm::Module& M)
{
    if (m_isStart)
    {
        COMPILER_TIME_START(m_context, m_index);
    }
    else
    {
        COMPILER_TIME_END(m_context, m_index);
    }

    return false;
}
