/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGA_FORMATTER_JSON
#define _IGA_FORMATTER_JSON

#include "Formatter.hpp"

namespace iga {
void FormatJSON(std::ostream &o, const FormatOpts &opts, const Kernel &k,
                const void *bits);

void FormatInstructionJSON1(std::ostream &o, const FormatOpts &opts,
                            const Instruction &i, const void *bits);
void FormatInstructionJSON2(std::ostream &o, const FormatOpts &opts,
                            const Instruction &i, const void *bits);
} // namespace iga

#endif // _IGA_FORMATTER_JSON
