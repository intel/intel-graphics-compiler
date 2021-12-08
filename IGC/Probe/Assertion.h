/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGC_PROBE_ASSERTION_H
#define IGC_PROBE_ASSERTION_H

#include "../common/EmUtils.h"



#include <cassert>
#include <cstdlib>

#define IGC_ASSERT assert

#define IGC_ASSERT_MESSAGE(x, m, ...) IGC_ASSERT(x)

#define IGC_ASSERT_EXIT(x) \
    do \
    { \
        if(0 == (x)) \
        { \
            assert(0); \
            std::exit(EXIT_FAILURE); \
        } \
    } while(0)

#define IGC_ASSERT_EXIT_MESSAGE(x, m, ...) IGC_ASSERT_EXIT(x)


#endif // IGC_PROBE_ASSERTION_H
