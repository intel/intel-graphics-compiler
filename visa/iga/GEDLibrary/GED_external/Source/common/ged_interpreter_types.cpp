/*========================== begin_copyright_notice ============================

Copyright (c) 2015-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#include "common/ged_base.h"
#include "common/ged_interpreter_types.h"


const char* gedInterpreterTypeStrings[GED_INTERPRETER_TYPE_SIZE] =
{
    "GED_INTERPRETER_TYPE_NONE",
    "GED_INTERPRETER_TYPE_POSITION",
    "GED_INTERPRETER_TYPE_REENUM",
    "GED_INTERPRETER_TYPE_DIV",
    "GED_INTERPRETER_TYPE_NUMTYPE",
    "GED_INTERPRETER_TYPE_COLLECT"
};


const char* gedInterpreterTypePadding[GED_INTERPRETER_TYPE_SIZE] =
{
    "    ",
    "",
    "  ",
    "     ",
    " ",
    " "
};


bool ged_generalized_field_t::operator==(const ged_generalized_field_t& rhs) const
{
    GEDASSERT(sizeof(ged_generalized_field_t) == sizeof(uint32_t));
    return (*reinterpret_cast<const uint32_t*>(this) == *reinterpret_cast<const uint32_t*>(&rhs));
}


bool ged_generalized_field_t::operator<(const ged_generalized_field_t& rhs) const
{
    GEDASSERT(sizeof(ged_generalized_field_t) == sizeof(uint32_t));
    return (*reinterpret_cast<const uint32_t*>(this) < *reinterpret_cast<const uint32_t*>(&rhs));
}


const char* ged_generalized_field_t_str = "ged_generalized_field_t";


const char* ged_collector_info_t_str = "ged_collector_info_t";


const ged_collector_info_t emptyCollector = { 0, 0, NULL };
