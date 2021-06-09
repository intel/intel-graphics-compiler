/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef GED_INS_ENCODING_MASKS_H
#define GED_INS_ENCODING_MASKS_H

#include "common/ged_types_internal.h"


// This pragma is not required here for correctness. However, it is here to conform with ged_ins_decoding_table.h, to prevent future
// problems in case some definitions here change and to reduce memory footprint.
#pragma pack(4)


/*!
 * Structure for describing an entry in the instruction masks table.
 */
struct ged_instruction_masks_entry_t;


/*!
 * Instruction masks table type.
 *
 * Description:
 * Every instruction layout may have reserved bits which may be required to have specific values. To enforce these values, two masks
 * are used. First, an "or-mask" is applied (bitwise-or with the instruction bytes) to set all the reserved bits. Then, an "and-mask"
 * is applied (bitwise-and with the instruction bytes) to clear the MBZ (must be zero) bits.
 *
 * Since an instruction's layout may depend on the value of one or more instruction fields within the same instruction format, each
 * entry may hold either:
 *   a) The actual masks to be applied.
 *   b) A pointer to the next table in the encoding chain, similar to the way decoding works. See explanation in
 *      common/ged_ins_decoding_table.h.
 *
 * The table keys for the top level table are the opcodes.
 *
 * The table keys for subsequent tables are the possible values of the relevant field on which the masks depend on.
 */
typedef const ged_instruction_masks_entry_t* ged_instruction_masks_table_t;


/*!
 * String representation of the ged_ins_decoding_table_t type name.
 */
extern const char* ged_instruction_masks_table_t_str;


/*!
 * Available entries in the instruction masks table, see inlined documentation.
 */
enum GED_MASKS_TABLE_ENTRY_TYPE
{
    GED_MASKS_TABLE_ENTRY_TYPE_MASKS,       ///< The table entry holds the actual masks to be applied.
    GED_MASKS_TABLE_ENTRY_TYPE_NEXT_TABLE,  ///< The table entry holds a pointer to the next masks table in the encoding chain.
    GED_MASKS_TABLE_ENTRY_TYPE_NO_MASKS,    ///< Indicates that this configuration does not have any special masks.
    GED_MASKS_TABLE_ENTRY_TYPE_SIZE,
    GED_MASKS_TABLE_ENTRY_TYPE_INVALID = GED_MASKS_TABLE_ENTRY_TYPE_SIZE
};


// String representation of the GED_MASKS_TABLE_ENTRY_TYPE enum and the padding to be added in order for them to be aligned
// column-wise.
extern const char* gedMaskTableEntryTypeStrings[GED_MASKS_TABLE_ENTRY_TYPE_SIZE];
extern const char* gedMaskTableEntryTypePadding[GED_MASKS_TABLE_ENTRY_TYPE_SIZE];


/*!
 * Structure for describing the masks to be applied to an instruction.
 * For full details see documentation for ged_instruction_masks_table_t (above).
 */
struct ged_instruction_masks_t
{
    unsigned char _or[GED_NATIVE_INS_SIZE];     // or-mask to be applied to the instruction
    unsigned char _and[GED_NATIVE_INS_SIZE];    // and-mask to be applied to the instruction

    bool operator==(const ged_instruction_masks_t& cmp) const;
};


/*!
 * Structure for pointing to the next masks table in the encoding chain.
 * Used when the GED_MASKS_TABLE_ENTRY_TYPE is GED_MASKS_TABLE_ENTRY_TYPE_NEXT_TABLE.
 */
struct ged_instruction_masks_next_table_t
{
    ged_instruction_masks_table_t _tablePtr;    // pointer to the next table in the chain
    /* GED_INS_FIELD */ uint32_t _tableKey;     // which instruction field serves as the key (indices) for the table

    bool operator==(const ged_instruction_masks_next_table_t& cmp) const;
};


#define UNION_SIZE MAX_SIZEOF_2(ged_instruction_masks_t, ged_instruction_masks_next_table_t)
#define VA_SIZE ((UNION_SIZE + sizeof(void*) - 1) / sizeof(void*))
/*!
 * Structure used for explicitly declaring a ged_instruction_masks_entry_t. The structure is large enough to hold data of all members
 * of the union in ged_instruction_masks_entry_t (see below) and is compatible for initializing all interpretations of the union. It
 * must be possible to break down the data into chunks of void*, hence the _cvsa (Const Void Star Array) field in the initializer.
 * VA_SIZE is rounded up prior to division, to make sure we don't allocate less than we need to (due to integer division).
 */
struct instruction_masks_union_initializer_t
{
    const void* _cvsa[VA_SIZE];
};
#undef UNION_SIZE
#undef VA_SIZE


/*!
 * Structure for describing an entry in the instruction masks table.
 */
struct ged_instruction_masks_entry_t
{
    GED_MASKS_TABLE_ENTRY_TYPE _entryType; // used for interpreting the union
    union
    {
        instruction_masks_union_initializer_t _dummy;   // used for explicitly declaring a ged_instruction_masks_entry_t
        ged_instruction_masks_t _masks;                 // the masks that need to be applied to the instruction
        ged_instruction_masks_next_table_t _nextTable;  // pointer to the next table
    };

    bool operator==(const ged_instruction_masks_entry_t& cmp) const;
    inline bool operator!=(const ged_instruction_masks_entry_t& cmp) const { return !(*this == cmp); }
};

// Finished declaring all structures, go back to normal alignment.
#pragma pack()


/*!
 * String representation of the ged_instruction_masks_entry_t type name.
 */
extern const char* ged_instruction_masks_entry_t_str;


/*!
 * Preinitialized encoding masks entry for indicating an empty GED_MASKS_TABLE_ENTRY_TYPE_MASKS entry i.e. a masks entry which will be
 * filled with actual masks to be applied. However, if applied as-is it will not modify the instruction.
 */
extern const ged_instruction_masks_entry_t emptyMasks;


/*!
 * Preinitialized encoding masks entry for indicating a GED_MASKS_TABLE_ENTRY_TYPE_NO_MASKS entry. This entry is not supposed to be
*  applied. However, if applied as-is it will not modify the instruction so it is safe to do so.
 */
extern const ged_instruction_masks_entry_t noMasks;

#endif // GED_INS_ENCODING_MASKS_H
