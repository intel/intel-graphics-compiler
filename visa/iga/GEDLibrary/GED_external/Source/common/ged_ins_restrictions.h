/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef GED_INS_RESTRICTIONS_H
#define GED_INS_RESTRICTIONS_H

#include "common/ged_types_internal.h"


// This pragma is not required here for correctness. However, it is here to conform with ged_ins_decoding_table.h, to prevent future
// problems in case some definitions here change and to reduce memory footprint.
#pragma pack(4)


/*!
 * Structure for describing an instruction field's restriction.
 */
struct ged_field_restriction_t;


/*!
 * Instruction-field restrictions-table entry type.
 */
typedef const ged_field_restriction_t* ged_restriction_table_entry_t;


/*!
 * Instruction-field restrictions-table type.
 */
typedef const ged_restriction_table_entry_t* ged_restrictions_table_t;


/*!
 * Four types of available field encoding restrictions, see inlined documentation. Restrictions are mutually exclusive.
 */
enum GED_FIELD_RESTRICTIONS_TYPE
{
    GED_FIELD_RESTRICTIONS_TYPE_NONE,   ///< No restriction.
    GED_FIELD_RESTRICTIONS_TYPE_FIRST_RESTRICTION,
    GED_FIELD_RESTRICTIONS_TYPE_VALUE = GED_FIELD_RESTRICTIONS_TYPE_FIRST_RESTRICTION,  ///< Only one value is allowed for this field.
    GED_FIELD_RESTRICTIONS_TYPE_RANGE,  ///< The field's value is limited to a specific range.
    GED_FIELD_RESTRICTIONS_TYPE_MASK,   ///< A mask should be applied to the field's value.
    GED_FIELD_RESTRICTIONS_TYPE_LAST_RESTRICTION = GED_FIELD_RESTRICTIONS_TYPE_MASK,
    GED_FIELD_RESTRICTIONS_TYPE_FIRST_MODIFIER,
    /*!
     * The field has a padding of bits with a fixed value.
     * For decoding, this means that after the bits have been extracted a fixed-value padding should be added where applicable. This
     * is achieved by applying a bitwise-or with the fixed value.
     * For encoding, this means that the padded bits of the given value should first be compared to the fixed-value padding, then it
     * should be stripped, and only the relevant bits will be encoded. the comparison is achieved by first applying a bitwise-and with
     * a mask which has all padded bits set, and then a simple value comparison with the fixed-value padding.
     */
    GED_FIELD_RESTRICTIONS_TYPE_PADDING = GED_FIELD_RESTRICTIONS_TYPE_FIRST_MODIFIER,
    /*!
     * Define the type of the field. This includes the actual bit-size of the table entry (the bit-size field in the entry represents
     * the maximal possible value), whether it is signed or not and whether duplication is required. Valid only for variable fields.
     */
    GED_FIELD_RESTRICTIONS_TYPE_FIELD_TYPE,
    GED_FIELD_RESTRICTIONS_TYPE_LAST_MODIFIER = GED_FIELD_RESTRICTIONS_TYPE_FIELD_TYPE,
    /*!
    * The field is limited to an enumeration. Enumerations map from the field value (which serves as the index) to the actual
    * (meaningful) value. Enumerations are compared using the the mapped value so in order to validate a field with this restriction,
    * one must iterate the enumeration table entries until the field's value is found, rendering it valid. If no entry is found with
    * the field's value, the value is invalid.
    * This restriction type is not explicitly declared by a "restrict" node in the XML model. Rather, it is implied when the field's
    * type is "enum".
    */
    GED_FIELD_RESTRICTIONS_TYPE_ENUM,
    GED_FIELD_RESTRICTIONS_TYPE_SIZE,
    GED_FIELD_RESTRICTIONS_TYPE_INVALID = GED_FIELD_RESTRICTIONS_TYPE_SIZE
};


// String representation of the GED_FIELD_RESTRICTIONS_TYPE enum and the padding to be added in order for them to be aligned
// column-wise.
extern const char* gedRestrictionTypeStrings[GED_FIELD_RESTRICTIONS_TYPE_SIZE];
extern const char* gedRestrictionTypePadding[GED_FIELD_RESTRICTIONS_TYPE_SIZE];


// String representation of the underlying type of every GED_FIELD_RESTRICTIONS_TYPE enumerator.
extern const char* gedRestrictionTypeNames[GED_FIELD_RESTRICTIONS_TYPE_SIZE];


/*!
 * Type for describing a single allowed value for an instruction field.
 */
typedef uint32_t ged_field_restriction_value_t;


/*!
 * Structure for describing an allowed range of values for an instruction field.
 */
struct ged_field_restriction_range_t
{
    uint32_t _min;
    uint32_t _max;
};


/*!
 * Type for describing a mask of allowed values for an instruction field. This mask should be applied to the field's value and the
 * expected result is zero. If the result is non-zero, the value is illegal.
 */
typedef uint32_t ged_field_restriction_mask_t;


/*!
 * Type for describing a fixed-value padding restriction. See the GED_FIELD_RESTRICTIONS_TYPE_PADDING enumerator description for usage
 * details.
 */
struct get_field_restriction_padding_t
{
    uint32_t _value;
    uint32_t _mask;
};


/*!
 * Value enumeration table type. It is defined as an array of pointers and the underlying pointer type is inferred from the field's
 * GED_FIELD_TYPE.
 */
typedef const void** ged_field_enum_table_t;


/*!
 * Type for describing the field type modifier. See GED_FIELD_RESTRICTIONS_TYPE_FIELD_TYPE enumerator description for usage details.
 */
union ged_field_type_modifier_t
{
    uint32_t _ui;
    struct ged_field_type_attributes
    {
        uint8_t _bits;
        bool _signed;
        bool _duplicate;
    } _attr;
};


#define UNION_SIZE MAX_SIZEOF_6(ged_field_restriction_value_t, ged_field_restriction_range_t, ged_field_restriction_mask_t, \
                                get_field_restriction_padding_t, ged_field_type_modifier_t, ged_field_enum_table_t)
#define VA_SIZE ((UNION_SIZE + sizeof(void*) - 1) / sizeof(void*))
/*!
 * Structure used for explicitly declaring a ged_field_restriction_t. The structure is large enough to hold data of all members of the
 * union in ged_field_restriction_t (see below) and is compatible for initializing all interpretations of the union. It must be
 * possible to break down the data into chunks of void*, hence the _cvsa (Const Void Star Array) field in the initializer.
 * VA_SIZE is rounded up prior to division, to make sure we don't allocate less than we need to (due to integer division).
 */
struct field_restriction_union_initializer_t
{
    const void* _cvsa[VA_SIZE];
};
#undef UNION_SIZE
#undef VA_SIZE


/*!
 * Structure for describing a restriction on an instruction field's value.
 */
struct ged_field_restriction_t
{
    GED_FIELD_RESTRICTIONS_TYPE _restrictionType; // used for interpreting the union
    union
    {
        field_restriction_union_initializer_t _dummy;   // used for explicitly declaring a ged_field_restriction_t
        ged_field_restriction_value_t _value;           // the (single) allowed value for this field
        ged_field_restriction_range_t _range;           // the allowed range for this field
        ged_field_restriction_mask_t _mask;             // the mask which should be applied to the field's value
        get_field_restriction_padding_t _padding;       // the fixed-value padding descriptor
        ged_field_type_modifier_t _fieldType;           // field-type modifier, valid only for variable fields
        ged_field_enum_table_t _enumTable;              // value enumeration table for this field
    };

    bool operator==(const ged_field_restriction_t& rhs) const;
    inline bool operator!=(const ged_field_restriction_t& rhs) const { return !(*this == rhs); }
    bool operator<(const ged_field_restriction_t& rhs) const;
};

// Finished declaring all structures, go back to normal alignment.
#pragma pack()


/*!
 * String representation of the ged_field_restriction_t type name.
 */
extern const char* ged_field_restriction_t_str;


/*!
 * Initialize the restriction with default NULL values as follows:
 *   _restrictionType = GED_FIELD_RESTRICTIONS_TYPE_NONE
 *   _dummy = 0
 *
 * @param   restriction     The restriction to initialize.
 */
extern void InitRestriction(ged_field_restriction_t& restriction);

#endif // GED_INS_RESTRICTIONS_H
