/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

#include "GenIntrinsicEnum.h"
#include <array>

namespace IGC
{

struct IntrinsicEntry
{
    llvm::GenISAIntrinsic::ID id;
    // the number of characters in the common prefix with other intrinsics
    unsigned num;
    // the lookup name
    const char* str;
};

<%
from Intrinsic_generator import IntrinsicFormatter
lookup_table = IntrinsicFormatter.get_lookup_table(intrinsic_definitions)
%>\
constexpr auto GetIntrinsicLookupTable()
{
    constexpr unsigned numIntrinsicEntries = ${len(lookup_table)};
    std::array<IntrinsicEntry, numIntrinsicEntries> lookupTable =
    { {
% for el in lookup_table:
        ${IntrinsicFormatter.get_intrinsic_lookup_table_entry_initialization_list(el, loop.last)}
% endfor
    } };
    return lookupTable;
}

} // namespace IGC
