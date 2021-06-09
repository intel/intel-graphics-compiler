/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef GED_INTERPRETER_TYPES_H
#define GED_INTERPRETER_TYPES_H

#include "common/ged_compact_mapping_table.h"


/*!
 * Available interpreter types, see inlined documentation.
 */
enum GED_INTERPRETER_TYPE
{
    GED_INTERPRETER_TYPE_NONE,      //< No interpreter should be applied.
    GED_INTERPRETER_TYPE_POSITION,  //< Interpret specific bits of a given value as an independent entity.
    GED_INTERPRETER_TYPE_REENUM,    //< Reinterpret a given string enumeration.
    GED_INTERPRETER_TYPE_DIV,       //< Divide the two given generalized fields.
    GED_INTERPRETER_TYPE_NUMTYPE,   //< Select the base in which to display the given field.
    GED_INTERPRETER_TYPE_COLLECT,   //< Collect a set of fields.
    GED_INTERPRETER_TYPE_SIZE,
    GED_INTERPRETER_TYPE_INVALID = GED_INTERPRETER_TYPE_SIZE
};


// String representation of the GED_INTERPRETER_TYPE enum and padding to be added in order for them to be aligned column-wise.
extern const char* gedInterpreterTypeStrings[GED_INTERPRETER_TYPE_SIZE];
extern const char* gedInterpreterTypePadding[GED_INTERPRETER_TYPE_SIZE];


/*!
 * Structure for holding a generalized field. A generalized field may be be an explicit GED_INS_FIELD, or an interpreted one. If it is
 * interpreted, the _interpType defines which type of interpreter _interpId refers to. For example, it may be a GED_PSEUDO_FIELD, or a
 * GED_REINTERPRETED_ENUM. If no interpreter is used, _interpType will hold GED_INTERPRETER_TYPE_NONE and the value of _interpId is
 * ignored.
 */
struct ged_generalized_field_t
{
    /* GED_INS_FIELD */ uint16_t _field;            // the explicit GED_INS_FIELD
    uint8_t _interpId;                              // the interpreter used to obtain the value from the explicit field
    /* GED_INTERPRETER_TYPE */ uint8_t _interpType; // signifies which interpreter is used (if any)

    bool operator==(const ged_generalized_field_t& rhs) const;
    inline bool operator!=(const ged_generalized_field_t& rhs) const { return !(*this == rhs); }
    bool operator<(const ged_generalized_field_t& rhs) const;
};


/*!
 * String representation of the ged_generalized_field_t type name.
 */
extern const char* ged_generalized_field_t_str;


/*!
 * Structure for holding a collection interpreter (also referred to as a collector or collection table). A collector includes a table
 * of mapping entries and its size. When the interpreter is invoked, the table is traversed and the value of each field in the table
 * is mapped (collected) according to its mapping entry.
 */
struct ged_collector_info_t
{
    uint16_t _numberOfFields; // number of mapping entries in the table
    uint16_t _width; // total width of the collected value
    ged_compact_mapping_table_t _table;

    inline bool operator==(const ged_collector_info_t& rhs) const { return _table == rhs._table; }
    inline bool operator!=(const ged_collector_info_t& rhs) const { return _table != rhs._table; }
    inline bool operator<(const ged_collector_info_t& rhs) const { return _table < rhs._table; }
};


/*!
 * String representation of the ged_collector_info_t type name.
 */
extern const char* ged_collector_info_t_str;


/*!
 * Preinitialized collector entry for indicating an empty (unsupported) collector.
 */
extern const ged_collector_info_t emptyCollector;

#endif // GED_INTERPRETER_TYPES_H
