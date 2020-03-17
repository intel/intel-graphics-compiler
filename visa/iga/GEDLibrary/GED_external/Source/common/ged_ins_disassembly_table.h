#ifndef GED_INS_DISASSEMBLY_TABLE_H
#define GED_INS_DISASSEMBLY_TABLE_H

# if GED_DISASSEMBLY

#include "common/ged_types_internal.h"
#include "common/ged_interpreter_types.h"


// This pragma is not required here for correctness. However, it is here to conform with ged_ins_decoding_table.h, to prevent future
// problems in case some definitions here change and to reduce memory footprint.
#pragma pack(4)


/*!
 * Structure for describing a disassembly block.
 */
struct ged_disassembly_block_t;


/*!
 * Disassembly (pointer) table entry type.
 */
typedef const ged_disassembly_block_t* ged_disassembly_table_entry_t;


/*!
 * Instruction disassembly table type.
 *
 * A disassembly-blocks table, or disassembly table for short, is a NULL terminated list of (pointers to) disassembly blocks. When
 * generating the disassembly string for an instruction, the list should be iterated (and every block printed) until the NULL
 * terminator is found.
 * A disassembly block may depend on the value of one or more instruction fields. In this case, each dependance leads to a disassembly
 * next-table. A disassembly next-table, not to be confused with the disassembly block-table defined herein, is a table of disassembly
 * tables (see definition of ged_disassembly_next_table_t below), one disassembly table for each possible value of the dependee field.
 * The top table and subsequent next-tables until the final blocks-to-print are found, are referred to as the "Disassembly Chain".
 *
 * Each entry in the disassembly block-table holds either:
 *   a) A tokens block which should be printed.
 *   b) A pointer to the next table in the disassembly chain.
 *
 * All blocks of a disassembly table, whether top level or subsequent, are to be iterated and processed.
 */
typedef const ged_disassembly_table_entry_t* ged_disassembly_table_t;


/*!
 * Available block types in the disassembly tables, see inlined documentation.
 */
enum GED_DISASSEMBLY_BLOCK_TYPE
{
    /*!
     * Disassembly block which contains a pointer to a list of tokens. Each token can either be a single character, or a field ID.
     */
    GED_DISASSEMBLY_BLOCK_TYPE_TOKENS = 0,
    /*!
     * DIsassembly block which contains a list of fields to print.
     */
    GED_DISASSEMBLY_BLOCK_TYPE_FIELD_LIST,
    /*!
     * Disassembly block which contains the pseudo-field to interpret and the input field.
     */
    GED_DISASSEMBLY_BLOCK_TYPE_INTERPRETER,
    /*!
     * The table entry holds a pointer to the next disassembly table in the disassembly chain, similar to the way decoding works.
     * See explanation in common/ged_ins_decoding_table.h.
     */
    GED_DISASSEMBLY_BLOCK_TYPE_NEXT_TABLE,
    /*!
     * Indicates that this disassembly block is not supported in the current instruction configuration (the value of certain fields).
     */
    GED_DISASSEMBLY_BLOCK_TYPE_NOT_SUPPORTED,
    GED_DISASSEMBLY_BLOCK_TYPE_SIZE,
    GED_DISASSEMBLY_BLOCK_TYPE_INVALID = GED_DISASSEMBLY_BLOCK_TYPE_SIZE
};


// String representation of the GED_DISASSEMBLY_BLOCK_TYPE enum and padding to be added in order for them to be aligned column-wise.
extern const char* gedDisassemblyBlockTypeStrings[GED_DISASSEMBLY_BLOCK_TYPE_SIZE];
extern const char* gedDisassemblyBlockTypePadding[GED_DISASSEMBLY_BLOCK_TYPE_SIZE];


/*!
 * Available token types in the disassembly block, see inlined documentation.
 */
enum GED_DISASSEMBLY_TOKEN_TYPE
{
    GED_DISASSEMBLY_TOKEN_TYPE_OPCODE = 0,  //< A token representing the predefined opcode field.
    GED_DISASSEMBLY_TOKEN_TYPE_COMPACTED,   //< A token representing the predefined compact control field.
    GED_DISASSEMBLY_TOKEN_TYPE_CHAR,        //< A token representing a single character.
    GED_DISASSEMBLY_TOKEN_TYPE_FIELD,       //< A token representing a field ID.
    /*!
     * A token representing a field ID, but the field's raw encoding should be printed instead of the processed field value.
     */
    GED_DISASSEMBLY_TOKEN_TYPE_RAW_FIELD,
    GED_DISASSEMBLY_TOKEN_TYPE_SIZE,
    GED_DISASSEMBLY_TOKEN_TYPE_INVALID = GED_DISASSEMBLY_TOKEN_TYPE_SIZE
};


// String representation of the GED_DISASSEMBLY_TOKEN_TYPE enum and padding to be added in order for them to be aligned column-wise.
extern const char* gedDisassemblyTokenTypeStrings[GED_DISASSEMBLY_TOKEN_TYPE_SIZE];
extern const char* gedDisassemblyTokenTypePadding[GED_DISASSEMBLY_TOKEN_TYPE_SIZE];

/*!
 * Type for describing a single token in the disassembly block.
 */
struct ged_disassembly_token_t
{
    /* GED_DISASSEMBLY_TOKEN_TYPE */ uint16_t _tokenType;
    union
    {
        uint16_t _dummy; // initialize the union
        char _char;
        /* GED_INS_FIELD */ uint16_t _field;
    };

    bool operator==(const ged_disassembly_token_t& rhs) const;
    inline bool operator!=(const ged_disassembly_token_t& rhs) const { return !(*this == rhs); }
};


/*!
 * String representation of the ged_disassembly_token_t type name.
 */
extern const char* ged_disassembly_token_t_str;


/*!
 * Structure for holding a list of disassembly tokens.
 */
struct ged_disassembly_block_tokens_t
{
    uint32_t _numOfTokens;
    const ged_disassembly_token_t* _tokens;
};


/*!
 * This type is used for holding a list of generalized fields. Used when the GED_INTERPRETER_TYPE requires more than one field.
 */
typedef const ged_generalized_field_t* ged_generalized_field_list_t;


/*!
 * This type holds the Id of an interpreter which does not take any arguments.
 */
typedef uint8_t ged_noarg_interpreter_id_t;


/*!
 * This type is used for explicitly declaring a ged_disassembly_block_interpreter_t. It is large enough to hold data of all members of
 * the union in ged_disassembly_block_interpreter_t (see below) and is compatible for initializing all interpretations of the union.
 */
typedef const void* interpreter_block_initializer_t;


/*!
 * Structure for describing an interpreter disassembly block.
 */
struct ged_disassembly_block_interpreter_t
{
    /* GED_INTERPRETER_TYPE */ uint8_t _interpType; // used for interpreting the union
    uint8_t _numberOfFields; // should be 1 if _field is used or the number of fields in the _fieldList array
    uint16_t _padding; // ignored, should be 0
    union
    {
        interpreter_block_initializer_t _dummy;     // used for explicitly declaring a ged_disassembly_block_interpreter_t
        ged_generalized_field_t _field;             // used when the interpreter requires only one field
        ged_generalized_field_list_t _fieldList;    // list of arguments for the interpreter based on its type
        ged_noarg_interpreter_id_t _noargId;        // used when the interpreter only has an Id and does not take any arguments
    };
};


/*!
 * Structure for pointing to the next disassembly table in the disassembly chain. Used when the GED_DISASSEMBLY_BLOCK_TYPE is
 * GED_DISASSEMBLY_BLOCK_TYPE_NEXT_TABLE. This structure holds a pointer to a table of disassembly tables. See definition of the
 * disassembly chain above.
 */
struct ged_disassembly_next_table_t
{
    ged_generalized_field_t _tableKey;          // the instruction field which serves as the key (indices) for the table, the value
                                                //   may be the explicit GED_INS_FIELD value or interpreted
    const ged_disassembly_table_t* _tablePtr;   // pointer to a disassembly next-table, use the key to obtain the next disassembly
                                                //   table in the disassembly chain
};


#define UNION_SIZE MAX_SIZEOF_3(ged_disassembly_block_tokens_t, ged_disassembly_block_interpreter_t, ged_disassembly_next_table_t)
#define VA_SIZE ((UNION_SIZE - sizeof(uint32_t) + sizeof(void*) - 1) / sizeof(void*))
/*!
 * Structure used for explicitly declaring a ged_disassembly_block_t. The structure is large enough to hold data of all members of the
 * union in ged_disassembly_block_t (see below) and is compatible for initializing all interpretations of the union.
 * All union interpretations should have uint32_t as the first field, followed by different sized data. It must be possible to break
 * down the data into chunks of void*, hence the _cvsa (Const Void Star Array) field in the initializer.
 * VA_SIZE is rounded up prior to division, to make sure we don't allocate less than we need to (due to integer division).
 * Because the union interpreters have a uint32_t as the first field, we define it as _ui and subtract it from VA_SIZE.
 */
struct ged_disassembly_block_union_initializer_t
{
    uint32_t _ui;
    const void* _cvsa[VA_SIZE];
};
#undef UNION_SIZE
#undef VA_SIZE


/*!
 * Structure for describing a disassembly block.
 */
struct ged_disassembly_block_t
{
    GED_DISASSEMBLY_BLOCK_TYPE _entryType; // used for interpreting the union
    union
    {
        ged_disassembly_block_union_initializer_t _dummy; // used for explicitly declaring a ged_disassembly_block_t
        ged_disassembly_block_tokens_t _tokens;
        ged_disassembly_block_interpreter_t _interpreter;
        ged_disassembly_next_table_t _nextTable;
    };

    bool operator<(const ged_disassembly_block_t& rhs) const;
    bool operator==(const ged_disassembly_block_t& rhs) const;
    inline bool operator!=(const ged_disassembly_block_t& rhs) const { return !(*this == rhs); }
};


// Finished declaring all structures, go back to normal alignment.
#pragma pack()


/*!
 * String representation of the ged_disassembly_block_t type name.
 */
extern const char* ged_disassembly_block_t_str;


/*!
 * String representation of the ged_disassembly_table_t type name.
 */
extern const char* ged_disassembly_table_t_str;


/*!
 * Preinitialized empty disassembly block. It is initialized as follows:
 *   _entryType = GED_DISASSEMBLY_BLOCK_TYPE_NOT_SUPPORTED
 *   _tokens/_nextTable = 0
 */
extern const ged_disassembly_block_t emptyBlock;


/*!
 * Initialize the given ged_disassembly_block_t as an empty entry as follows:
 *   _entryType = GED_DISASSEMBLY_BLOCK_TYPE_NOT_SUPPORTED
 *   _tokens/_nextTable = 0
 *
 * @param[out]  entry   The entry to initialize.
 * @param[in]   dummy   Ignored.
 */
inline void InitTableEntry(ged_disassembly_block_t& entry, const /* GED_INS_FIELD */ uint32_t dummy) { entry = emptyBlock; }

# endif // GED_DISASSEMBLY

#endif // GED_INS_DISASSEMBLY_TABLE_H
