/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cstring>
#include "common/ged_ins_restrictions.h"

using std::memcmp;
using std::memset;


const char* gedRestrictionTypeStrings[GED_FIELD_RESTRICTIONS_TYPE_SIZE] =
{
    "GED_FIELD_RESTRICTIONS_TYPE_NONE",
    "GED_FIELD_RESTRICTIONS_TYPE_VALUE",
    "GED_FIELD_RESTRICTIONS_TYPE_RANGE",
    "GED_FIELD_RESTRICTIONS_TYPE_MASK",
    "GED_FIELD_RESTRICTIONS_TYPE_PADDING",
    "GED_FIELD_RESTRICTIONS_TYPE_FIELD_TYPE",
    "GED_FIELD_RESTRICTIONS_TYPE_ENUM"
};


const char* gedRestrictionTypePadding[GED_FIELD_RESTRICTIONS_TYPE_SIZE] =
{
    "      ",
    "     ",
    "     ",
    "      ",
    "   ",
    "",
    "      "
};


const char* ged_field_restriction_t_str = "ged_field_restriction_t";


const char* gedRestrictionTypeNames[GED_FIELD_RESTRICTIONS_TYPE_SIZE] =
{
    "",
    "Value",
    "Range",
    "Mask",
    "Padding",
    "Field Type",
    "Enum"
};


bool ged_field_restriction_t::operator==(const ged_field_restriction_t& rhs) const
{
    if (_restrictionType != rhs._restrictionType) return false;
    return (0 == memcmp(this->_dummy._cvsa, rhs._dummy._cvsa, sizeof(field_restriction_union_initializer_t)));
}


bool ged_field_restriction_t::operator<(const ged_field_restriction_t& rhs) const
{
    if (_restrictionType != rhs._restrictionType) return (_restrictionType < rhs._restrictionType);
    return (memcmp(this->_dummy._cvsa, rhs._dummy._cvsa, sizeof(field_restriction_union_initializer_t)) < 0);
}


void InitRestriction(ged_field_restriction_t& restriction)
{
    restriction._restrictionType = GED_FIELD_RESTRICTIONS_TYPE_NONE;
    memset(&(restriction._dummy), 0, sizeof(restriction._dummy));
}
