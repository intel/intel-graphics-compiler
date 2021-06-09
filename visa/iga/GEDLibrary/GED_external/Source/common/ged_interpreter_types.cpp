/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

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
