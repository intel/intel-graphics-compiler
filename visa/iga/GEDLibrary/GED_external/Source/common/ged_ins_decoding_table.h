/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef GED_INS_DECODING_TABLE_H
#define GED_INS_DECODING_TABLE_H

#include "common/ged_ins_position_fragment.h"
#include "common/ged_ins_restrictions.h"


// On 64-bit systems, alignment should explicitly be set to 4 because otherwise the ins_field_entry_union_initializer_t alignment does
// not match that of ged_ins_field_single_fragment_t. This will cause the mask field in the position fragment to reside in bits 64-95
// instead of 32-63 as expected, which in turn causes incorrect results.
#pragma pack(4)


/*!
 * Structure for describing an entry in the decoding table.
 */
struct ged_ins_field_entry_t;


/*!
 * Instruction decoding table type.
 *
 * Description:
 * These tables are used for decoding instructions i.e. extracting the requested instruction field from the instruction raw bits. An
 * instruction field position may depend on the value of one or more (other) instruction fields. In this case, each dependance is
 * decoded by a subsequent table. The top table and all subsequent tables until the actual position is determined, are referred to as
 * the "Decoding Chain".
 *
 * Each entry in the table holds either:
 *   a) The bit-position of the instruction field.
 *   b) A pointer to the next decoding table in the decoding chain.
 *
 * The table keys for the top level table are the instruction field id. The id is either:
 *   a) enum GED_INS_FIELD for precompiled models
 *   b) dynamically assigned id for custom models
 *
 * The table keys for subsequent tables are the possible values of the relevant field on which the decoding depends on. For example:
 *   If the "DstChanEn" field position depends on the value of the field "AccessMode", then the keys for the subsequent table will
 *   be the possible values of "AccessMode".
 */
typedef const ged_ins_field_entry_t* ged_ins_decoding_table_t;


/*!
 * String representation of the ged_ins_decoding_table_t type name.
 */
extern const char* ged_ins_decoding_table_t_str;


/*!
 * Available entries in the decoding table, see inlined documentation.
 */
enum GED_TABLE_ENTRY_TYPE
{
    /*!
     * The table entry holds a record of the instruction field's position when it is consecutive i.e. composed of a single fragment.
     */
    GED_TABLE_ENTRY_TYPE_CONSECUTIVE = 0,
    /*!
     * The table entry holds a record of the instruction field's position when it is fragmented i.e. composed of multiple fragments.
     */
    GED_TABLE_ENTRY_TYPE_FRAGMENTED,
    GED_TABLE_ENTRY_TYPE_FIXED_VALUE,
    GED_TABLE_ENTRY_TYPE_LAST_EXPLICIT = GED_TABLE_ENTRY_TYPE_FIXED_VALUE,
    GED_TABLE_ENTRY_TYPE_NEXT_TABLE,        ///< The table entry holds a pointer to the next decoding table in the decoding chain.
    GED_TABLE_ENTRY_TYPE_NOT_SUPPORTED,     ///< Indicates that this instruction field is not supported in the current format.
    GED_TABLE_ENTRY_TYPE_SIZE,
    GED_TABLE_ENTRY_TYPE_INVALID = GED_TABLE_ENTRY_TYPE_SIZE
};


// String representation of the GED_TABLE_ENTRY_TYPE enum and the padding to be added in order for them to be aligned column-wise.
extern const char* gedTableEntryTypeStrings[GED_TABLE_ENTRY_TYPE_SIZE];
extern const char* gedTableEntryTypePadding[GED_TABLE_ENTRY_TYPE_SIZE];


/*!
 * Structure for describing an instruction field's consecutive bit-position in the instruction bits i.e. the position is composed of
 * a single fragment.
 * Used when the GED_TABLE_ENTRY_TYPE is GED_TABLE_ENTRY_TYPE_CONSECUTIVE
 */
struct ged_ins_field_single_fragment_t
{
    ged_ins_field_position_fragment_t _position;
};


/*!
 * Structure for describing an instruction field's fragmented bit-position in the instruction bits i.e. the position is composed of
 * multiple fragments.
 * Used when the GED_TABLE_ENTRY_TYPE is GED_TABLE_ENTRY_TYPE_FRAGMENTED
 */
struct ged_ins_field_multiple_fragments_t
{
    uint32_t _numOfPositionFragments;
    const ged_ins_field_position_fragment_t* _fragments;
};


/*!
 * Structure for describing an instruction field which has a (constant and predefined) fixed value.
 * Used when the GED_TABLE_ENTRY_TYPE is GED_TABLE_ENTRY_TYPE_FIXED_VALUE
 */
struct ged_ins_field_fixed_value_t
{
    uint32_t _value;
};


/*!
 * Structure for pointing to the next decoding table in the decoding chain.
 * Used when the GED_TABLE_ENTRY_TYPE is GED_TABLE_ENTRY_TYPE_NEXT_TABLE.
 */
struct ged_ins_field_next_table_t
{
    /* GED_INS_FIELD */ uint32_t _tableKey; // which instruction field serves as the key (indices) for the table
    ged_ins_decoding_table_t _tablePtr;     // pointer to the next table in the decoding chain
};


#define UNION_SIZE MAX_SIZEOF_4(ged_ins_field_single_fragment_t, ged_ins_field_multiple_fragments_t, \
                                ged_ins_field_next_table_t, ged_ins_field_fixed_value_t)
#define VA_SIZE ((UNION_SIZE - sizeof(uint32_t) + sizeof(void*) - 1) / sizeof(void*))
/*!
 * Structure used for explicitly declaring a ged_ins_field_entry_t. The structure is large enough to hold data of all members of the
 * union in ged_ins_field_entry_t (see below) and is compatible for initializing all interpretations of the union.
 * All union interpretations should have uint32_t as the first field, followed by different sized data. It must be possible to break
 * down the data into chunks of void*, hence the _cvsa (Const Void Star Array) field in the initializer.
 * VA_SIZE is rounded up prior to division, to make sure we don't allocate less than we need to (due to integer division).
 * Because the union interpreters have a uint32_t as the first field, we define it as _ui and subtract it from VA_SIZE.
 */
struct ins_field_entry_union_initializer_t
{
    uint32_t _ui;
    const void* _cvsa[VA_SIZE];
};
#undef UNION_SIZE
#undef VA_SIZE


/*!
 * Structure for describing an entry in the decoding table.
 */
struct ged_ins_field_entry_t
{
    /* GED_INS_FIELD */ uint16_t _field;
    /* GED_TABLE_ENTRY_TYPE */ uint8_t _entryType; // used for interpreting the union
    uint8_t _bitSize;   // For an explicit entry (single/multiple fragments), this holds the size (in bits) of the explicit entry,
                        //   including any implicit bits (paddings). i.e. the size of the field in the current decoding chain. For a
                        //   next-table entry, this holds the maximal size of the field, taking into account all possible decoding
                        //   chains.
    union
    {
        ins_field_entry_union_initializer_t _dummy;     // used for explicitly declaring a ged_ins_field_entry_t
        ged_ins_field_single_fragment_t _consecutive;   // the instruction field consecutive (single-fragment) position
        ged_ins_field_multiple_fragments_t _fragmented; // the instruction field fragmented (multi-fragment) position
        ged_ins_field_fixed_value_t _fixed;             // the fixed value of the field
        ged_ins_field_next_table_t _nextTable;          // pointer to the next table
    };
    ged_restrictions_table_t _restrictions; // NULL terminated list of restrictions

    bool operator==(const ged_ins_field_entry_t& rhs) const;
    inline bool operator!=(const ged_ins_field_entry_t& rhs) const { return !(*this == rhs); }
    bool operator<(const ged_ins_field_entry_t& rhs) const;
};

// Finished declaring all structures, go back to normal alignment.
#pragma pack()


/*!
 * String representation of the ged_ins_field_entry_t type name.
 */
extern const char* ged_ins_field_entry_t_str;


/*!
 * Initialize the given ged_ins_field_entry_t as an empty entry as follows:
 *   _field = the given field
 *   _entryType = GED_TABLE_ENTRY_TYPE_NOT_SUPPORTED
 *   _bitSize = 0
 *   _dummy = 0
 *   _restrictions = NULL
 *
 * @param[out]  entry   The entry to initialize.
 * @param[in]   field   The value for initializing _field.
 */
extern void InitTableEntry(ged_ins_field_entry_t& entry, const /* GED_INS_FIELD */ uint32_t field);


/*!
 * Get the maximum possible value for the given field. Only valid for fields with bitsize up to 64 bits.
 *
 * @param[in]   entry   The entry of the field for which to calculate the maximum value.
 *
 * @return      The maximum possible value.
 */
extern uint64_t MaxValue(const ged_ins_field_entry_t& entry);


/*
 * Get the maximum possible value for the given field. Only valid for fields with bitsize up to 64 bits.
 *
 * @param[in]   entry   Pointer to the entry of the field for which to calculate the maximum value.
 *
 * @return      The maximum possible value.
 */
extern uint64_t MaxValue(const ged_ins_field_entry_t* entry);

#endif // GED_INS_DECODING_TABLE_H
