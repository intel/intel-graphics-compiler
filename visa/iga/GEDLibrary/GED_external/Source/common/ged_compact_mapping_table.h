/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef GED_COMPACT_MAPPING_TABLE_H
#define GED_COMPACT_MAPPING_TABLE_H

#include "common/ged_ins_position_fragment.h"


// This pragma is not required here for correctness. However, it is here to conform with ged_ins_decoding_table.h, to prevent future
// problems in case some definitions here change and to reduce memory footprint.
#pragma pack(4)


/*!
 * Structure for describing how every field in a compact instruction maps to the regular instruction format.
 */
struct ged_compact_mapping_entry_t;


/*!
 * When decoding a compact instruction, we first construct the instruction's native format using compact instruction mapping tables.
 * Then we parse the native instruction normally.
 *
 * For this we have several tables:
 * 1. An instruction decoding table (ged_ins_decoding_table_t - see common/ged_ins_decoding_table.h) same as any instruction, for
 *    extracting the instruction fields in the compact format.
 * 2. A mapping table (ged_compact_mapping_table_t - see below) which maps every field in the compact format to the corresponding
 *    field(s) in the regular format. This table holds only the metadata i.e. the position mappings. The data itself, i.e. the field
 *    values in the regular format are obtained from the value mapping tables (see #3 below).
 * 3. Compaction tables (ged_compaction_table_t - see below). These tables are used for decoding the special index fields in the
 *    compact format.
 */

/*!
 * Mapping table type.
 */
typedef const ged_compact_mapping_entry_t* ged_compact_mapping_table_t; // see table #2 above


/*!
 * String representation of the ged_ins_decoding_table_t type name.
 */
extern const char* ged_compact_mapping_table_t_str;


/*!
 * Compaction table entry type.
 */
typedef uint64_t ged_compaction_table_entry_t; // see table #3 above


/*!
 * String representation of the ged_compaction_table_entry_t type name.
 */
extern const char* ged_compaction_table_entry_t_str;


/*!
 * Compaction table type.
 */
typedef const ged_compaction_table_entry_t* ged_compaction_table_t; // see table #3 above


/*!
 * Available entries in the mapping table (#2), see inlined documentation.
 */
enum GED_MAPPING_TABLE_ENTRY_TYPE
{
    /*!
     * The value of the current compact instruction field is the mapping source.
     * The mapping target is consecutive i.e. composed of a single fragment.
     */
    GED_MAPPING_TABLE_ENTRY_TYPE_VALUE_MAPPING_CONSECUTIVE = 0,
    /*!
     * The value of the current compact instruction field is the mapping source.
     * The mapping target is fragmented i.e. composed of multiple fragments.
     */
    GED_MAPPING_TABLE_ENTRY_TYPE_VALUE_MAPPING_FRAGMENTED,
    /*!
     * The mapping source is an entry in a compaction table and the value of the current compact instruction field is the index
     * of that entry.
     * The mapping target is consecutive i.e. composed of a single fragment.
     */
    GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_CONSECUTIVE,
    /*!
     * The mapping source is an entry in a compaction table and the value of the current compact instruction field is the index
     * of that entry.
     * The mapping target is fragmented i.e. composed of multiple fragments.
     */
    GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_FRAGMENTED,
    GED_MAPPING_TABLE_ENTRY_TYPE_NO_MAPPING,
    GED_MAPPING_TABLE_ENTRY_TYPE_FIXED,
    GED_MAPPING_TABLE_ENTRY_TYPE_LAST_EXPLICIT = GED_MAPPING_TABLE_ENTRY_TYPE_FIXED,
    /*!
     * The table entry holds a pointer to the next mapping table in the mapping chain, similar to the way decoding works.
     * See explanation in common/ged_ins_decoding_table.h.
     */
    GED_MAPPING_TABLE_ENTRY_TYPE_NEXT_TABLE,
    GED_MAPPING_TABLE_ENTRY_TYPE_NOT_SUPPORTED, ///< indicates that this instruction field is not supported in the current format
    GED_MAPPING_TABLE_ENTRY_TYPE_SIZE,
    GED_MAPPING_TABLE_ENTRY_TYPE_INVALID = GED_MAPPING_TABLE_ENTRY_TYPE_SIZE
};


// String representation of the GED_MAPPING_TABLE_ENTRY_TYPE enum and the padding to be added in order for them to be aligned
// column-wise.
extern const char* gedCompactTableEntryTypeStrings[GED_MAPPING_TABLE_ENTRY_TYPE_SIZE];
extern const char* gedCompactTableEntryTypePadding[GED_MAPPING_TABLE_ENTRY_TYPE_SIZE];


/*!
 * Two types of mappings, see inlined documentation.
 */
enum GED_COMPACT_MAPPING_TYPE
{
    /*!
     * Every bit in the source is mapped to exactly one bit in the target. The source and target sizes must be equal.
     */
    GED_COMPACT_MAPPING_TYPE_1x1 = 0,
    /*!
     * The specified source bits are replicated in order to completely fill the target. The target size must be a whole multiple
     * of the source size.
     */
    GED_COMPACT_MAPPING_TYPE_REP,
    GED_COMPACT_MAPPING_TYPE_NO_MAPPING,
    GED_COMPACT_MAPPING_TYPE_FIXED,
    GED_COMPACT_MAPPING_TYPE_SIZE,
    GED_COMPACT_MAPPING_TYPE_INVALID = GED_COMPACT_MAPPING_TYPE_SIZE
};


// String representation of the GED_COMPACT_MAPPING_TYPE enum and the padding to be added in order for them to be aligned column-wise.
extern const char* gedCompactMappingTypeStrings[GED_COMPACT_MAPPING_TYPE_SIZE];
extern const char* gedCompactMappingTypePadding[GED_COMPACT_MAPPING_TYPE_SIZE];


/*!
 * Structure for describing one fragment of a compact instruction field's mapping.
 */
struct ged_compact_mapping_fragment_t
{
    GED_COMPACT_MAPPING_TYPE _mappingType;      // either 1-to-1 or rep
    ged_ins_field_position_fragment_t _from;    // source bit location
    ged_ins_field_position_fragment_t _to;      // target bit location

    bool operator==(const ged_compact_mapping_fragment_t& rhs) const;
    inline bool operator!=(const ged_compact_mapping_fragment_t& rhs) const { return !(*this == rhs); }
};


/*!
 * String representation of the ged_compact_mapping_fragment_t type name.
 */
extern const char* ged_compact_mapping_fragment_t_str;


/*!
 * Structure for describing a compact instruction field's consecutive mapping i.e. the mapping is composed of a single fragment.
 * In this case, the mapping is always 1-to-1 and we only need a mask for the mapping source because the source value is
 * right-shifted to LSB 0.
 * Used when the GED_MAPPING_TABLE_ENTRY_TYPE is GED_MAPPING_TABLE_ENTRY_TYPE_VALUE_MAPPING_CONSECUTIVE or
 * GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_CONSECUTIVE.
 */
struct ged_compact_mapping_single_fragment_t
{
    uint32_t _fromMask;
    ged_ins_field_position_fragment_t _to;
};


/*!
 * Structure for describing a compact instruction field's fragmented mapping i.e. the mapping is composed of multiple fragments.
 * In this case, each fragment can be either 1-to-1 or rep, so the ged_compact_mapping_fragment_t is used.
 * Used when the GED_MAPPING_TABLE_ENTRY_TYPE is GED_MAPPING_TABLE_ENTRY_TYPE_VALUE_MAPPING_FRAGMENTED or
 * GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_FRAGMENTED
 */
struct ged_compact_mapping_multiple_fragments_t
{
    uint32_t _numOfMappingFragments;
    const ged_compact_mapping_fragment_t* _fragments;
};


/*!
 * Structure for pointing to the next mapping table in the mapping chain.
 * Used when the GED_MAPPING_TABLE_ENTRY_TYPE is GED_MAPPING_TABLE_ENTRY_TYPE_NEXT_TABLE.
 */
struct ged_compact_mapping_next_table_t
{
    /* GED_INS_FIELD */ uint32_t _tableKey; // which instruction field serves as the key (indices) for the table
    ged_compact_mapping_table_t _tablePtr;  // pointer to the next table in the decoding chain
};


#define UNION_SIZE MAX_SIZEOF_3(ged_compact_mapping_single_fragment_t, ged_compact_mapping_multiple_fragments_t, \
                                ged_compact_mapping_next_table_t)
#define VA_SIZE ((UNION_SIZE - sizeof(uint32_t) + sizeof(void*) - 1) / sizeof(void*))
/*!
 * Structure used for explicitly declaring a ged_compact_mapping_entry_t. The structure is large enough to hold data of all members
 * of the union in ged_compact_mapping_entry_t (see below) and is compatible for initializing all interpretations of the union.
 * All union interpretations should have a uint32_t as the first field, followed by different sized data. It must be possible to
 * break down the data into chunks of void*, hence the _cvsa (Const Void Star Array) field in the initializer.
 * VA_SIZE is rounded up prior to division, to make sure we don't allocate less than we need to (due to integer division).
 * Because the union interpreters have a uint32_t as the first field, we define it as _ui and subtract it from VA_SIZE.
 */
struct ged_compact_mapping_entry_union_initializer_t
{
    uint32_t _ui;
    const void* _cvsa[VA_SIZE];
};
#undef UNION_SIZE
#undef VA_SIZE


/*!
 * Structure for describing an entry in the mapping table.
 */
struct ged_compact_mapping_entry_t
{
    /* GED_INS_FIELD */ uint16_t  _field; // for internal validation only
    /* GED_MAPPING_TABLE_ENTRY_TYPE */ uint16_t _entryType; // used for interpreting the union
    ged_compaction_table_t _compactionTable; // the compaction mapping table for indexed mapping, NULL otherwise
    union
    {
        ged_compact_mapping_entry_union_initializer_t _dummy;   // used for explicitly declaring a ged_compact_mapping_entry_t
        ged_compact_mapping_single_fragment_t _consecutive;     // the instruction field consecutive (single-fragment) mapping
        ged_compact_mapping_multiple_fragments_t _fragmented;   // the instruction field fragmented (multi-fragment) mapping

        // Pointer to the next table. This is tricky because it may depend on values of the native instruction so some compact fields
        // may need to be decoded prior to this one. This is handled by the decoder.
        ged_compact_mapping_next_table_t _nextTable;
    };

    bool operator==(const ged_compact_mapping_entry_t& rhs) const;
    inline bool operator!=(const ged_compact_mapping_entry_t& rhs) const { return !(*this == rhs); }
};

// Finished declaring all structures, go back to normal alignment.
#pragma pack()


/*!
 * String representation of the ged_compact_mapping_entry_t type name.
 */
extern const char* ged_compact_mapping_entry_t_str;


/*!
 * Initialize the given ged_compact_mapping_entry_t as an empty entry as follows:
 *   _field = the given field
 *   _entryType = GED_MAPPING_TABLE_ENTRY_TYPE_NOT_SUPPORTED
 *   _dummy = 0
 *   _compactionTable = NULL
 *
 * @param[out]  entry   The entry to initialize.
 * @param[in]   field   The value for initializing _field.
 */
extern void InitTableEntry(ged_compact_mapping_entry_t& entry, /* GED_INS_FIELD */ uint32_t field);


/*
 * Get the number of bits for the given mapping source. The target may have a different size in case of a repped mapping.
 *
 * @param[in]   entryPtr    Pointer to the entry of the mapping for which to calculate the size.
 *
 * @return      The number of bits for the mapping source.
 */
extern uint32_t MappingSourceBitSize(const ged_compact_mapping_entry_t* entryPtr);


/*
 * Get the number of bits for the given mapping source. The target may have a different size in case of a repped mapping.
 *
 * @param[in]   entry   The entry of the mapping for which to calculate the size.
 *
 * @return      The number of bits for the mapping source.
 */
extern uint32_t MappingSourceBitSize(const ged_compact_mapping_entry_t& entry);


/*!
 * Get the number of bits for the given mapping target. The source may have a different size in case of a repped mapping.
 *
 * @param[in]   entryPtr    The entry of the mapping for which to calculate the size.
 *
 * @return      The number of bits for the mapping target.
 */
extern uint32_t MappingTargetBitSize(const ged_compact_mapping_entry_t* entryPtr);


/*!
 * Get the number of bits for the given mapping target. The source may have a different size in case of a repped mapping.
 *
 * @param[in]   entry   The entry of the mapping for which to calculate the size.
 *
 * @return      The number of bits for the mapping target.
 */
extern uint32_t MappingTargetBitSize(const ged_compact_mapping_entry_t& entry);

#endif // GED_COMPACT_MAPPING_TABLE_H
