/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef GED_VALIDATION_UTILS_H
#define GED_VALIDATION_UTILS_H

#include "ged_types_internal.h"
#include "ged_ins_restrictions.h"
#include "ged_ins_position_fragment.h"


# define UNION_SIZE MAX_SIZEOF_2(ged_ins_field_position_fragment_t, ged_field_restriction_value_t)
# define VA_SIZE ((UNION_SIZE - sizeof(uint32_t) + sizeof(ged_ins_field_position_fragment_t) - 1) / sizeof(void*))
/*!
* Structure used for explicitly declaring a ged_ins_field_mapping_fragment_t. The structure is large enough to hold data of all
* members of the union in ged_ins_field_mapping_fragment_t (see below) and is compatible for initializing all interpretations of
* the union. All union interpretations should have uint32_t as the first field, followed by different sized data. It must be possible
* to break down the data into chunks of void*, hence the _cvsa (Const Void Star Array) field in the initializer.
* VA_SIZE is rounded up prior to division, to make sure we don't allocate less than we need to (due to integer division).
* Because the union interpreters have a uint32_t as the first field, we define it as _ui and subtract it from VA_SIZE.
*/
struct ins_field_mapping_fragment_union_initializer_t
{
    uint32_t _ui;
    const void* _cvsa[VA_SIZE];
};
#undef UNION_SIZE
#undef VA_SIZE

/*!
* Structure for describing one fragment of an instruction field to its encoding position in the layout.
*/
struct ged_ins_field_mapping_fragment_t
{
    bool _fixed; // is current fragment a fixed-value fragment (either from padding restriction or the fixed field type)
    ged_ins_field_position_fragment_t _from; // source bit location
    union
    {
        ins_field_mapping_fragment_union_initializer_t _dummy;  // used for explicitly declaring a ged_ins_field_mapping_fragment_t
        ged_ins_field_position_fragment_t _to;                  // target bit location
        ged_field_restriction_value_t _value;                   // the (single) allowed value for this field
    };
    bool operator==(const ged_ins_field_mapping_fragment_t& rhs) const;
    bool operator<(const ged_ins_field_mapping_fragment_t& rhs) const;
};

#endif // GED_VALIDATION_UTILS_H
