/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IR_CHECKER_HPP_
#define _IR_CHECKER_HPP_

#include "../../ErrorHandler.hpp"
#include "../Instruction.hpp"
#include "../Kernel.hpp"
#include <cstdint>

namespace iga {
// logs errors to error log
void CheckSemantics(const Kernel &k, ErrorHandler &err,
                    uint32_t enbabled_warnings);

// asserts on bad IR
void SanityCheckIR(const Kernel &k);
void SanityCheckIR(const Instruction &i);
} // namespace iga

#endif // _IR_CHECKER_HPP_
