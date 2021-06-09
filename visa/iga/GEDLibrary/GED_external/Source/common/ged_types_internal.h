/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef GED_TYPES_INTERNAL_H
#define GED_TYPES_INTERNAL_H

#include <cstddef>
#include "common/ged_basic_types.h"
#include "common/ged_enum_types.h"

using std::size_t;


const uint8_t  MAX_UINT8_T  = 0xff;
const uint16_t MAX_UINT16_T = 0xffff;
const uint32_t MAX_UINT32_T = 0xffffffff;
const uint64_t MAX_UINT64_T = 0xffffffffffffffffULL;

/*!
 * Unsigned integer of the same size as a pointer on the TARGET architecture.
 */
# if defined(TARGET_IA32)
typedef uint32_t g_uintptr_t;
# elif defined(TARGET_INTEL64)
typedef uint64_t g_uintptr_t;
# else
#  error A target architecture must be defined. One of [TARGET_IA32 | TARGET_INTEL64].
# endif


/*!
 * Type for representing a value's type.
 */
enum GED_VALUE_TYPE
{
    GED_VALUE_TYPE_PROCESSED = 0,  // value requires additional processing
    GED_VALUE_TYPE_ENCODED,        // value is in its raw form
};

/*!
 * Type for representing an instruction field's type information.
 */
typedef uint16_t GED_FIELD_TYPE;

// GED_FIELD_TYPE bits.

/*!
 * If this bit is set, the field's type is string, in which case GED_FIELD_TYPE_ENUM_BIT must be set as well and all other bits must
 * be clear. If it is not set, the field has a numeric type which is determined by the state of the remaining bits.
 */
const GED_FIELD_TYPE GED_FIELD_TYPE_STRING_BIT      = 1 << 0;

/*!
 * If this bit is set, the field has an enumeration, otherwise it has an explicit value.
 */
const GED_FIELD_TYPE GED_FIELD_TYPE_ENUM_BIT        = 1 << 1;

/*!
 * If this bit is set, the field size and sign are not constant and may vary according to some other parameters. The sign and qword
 * bits (below) should be ignored. This only applies to to numeric, non-enumerated fields.
 */
const GED_FIELD_TYPE GED_FIELD_TYPE_VARIABLE_BIT    = 1 << 2;

/*!
 * If this bit is set, the field holds a signed value, otherwise it holds an unsigned value. This bit is used only if the field has a
 * numeric type i.e. GED_FIELD_TYPE_STRING_BIT is clear.
 */
const GED_FIELD_TYPE GED_FIELD_TYPE_SIGNED_BIT      = 1 << 3;

/*!
 * If this bit is set, the field holds a 64-bit value, otherwise it holds a 32-bit value. This bit is used only if the field has a
 * numeric type i.e. GED_FIELD_TYPE_STRING_BIT is clear.
 */
const GED_FIELD_TYPE GED_FIELD_TYPE_QWORD_BIT       = 1 << 4;

/*!
 * If this bit is set, the field representation is hexadecimal, otherwise it is decimal. This bit is used only if the field has a
 * numeric type i.e. GED_FIELD_TYPE_STRING_BIT is clear.
 */
const GED_FIELD_TYPE GED_FIELD_TYPE_HEX_BIT         = 1 << 5;

/*!
 * If this bit is set, the field's numeric type representation is floating point, otherwise it is an integer. This bit is used only if
 * the field has a numeric type i.e. GED_FIELD_TYPE_STRING_BIT is clear.
 */
const GED_FIELD_TYPE GED_FIELD_TYPE_FP_BIT          = 1 << 6;

/*!
 * If this bit is set, the field would have a getter function in GED's API.
 */
const GED_FIELD_TYPE GED_FIELD_TYPE_GETTER_BIT      = 1 << 7;

/*!
 * If this bit is set, the field would have a setter function in GED's API.
 */
const GED_FIELD_TYPE GED_FIELD_TYPE_SETTER_BIT      = 1 << 8;

/*!
 * This bit represents a reserved field. If it is set, all other bits must be clear.
 */
const GED_FIELD_TYPE GED_FIELD_TYPE_RESERVED_BIT    = 1 << 15;

/*!
 * Constant for representing an invalid-FIELD's type. NOT intended to be used for representing an invalid TYPE.
 */
const GED_FIELD_TYPE GED_FIELD_TYPE_INVALID_FIELD   = MAX_UINT16_T;

// GED_FIELD_TYPE query macros.
#define IS_NUMERIC(type)            (0 == (type & GED_FIELD_TYPE_STRING_BIT))
#define IS_STRING(type)             (0 != (type & GED_FIELD_TYPE_STRING_BIT))
#define IS_ENUM(type)               (0 != (type & GED_FIELD_TYPE_ENUM_BIT))
#define IS_VARIABLE(type)           (0 != (type & GED_FIELD_TYPE_VARIABLE_BIT))
#define IS_UNSIGNED(type)           (0 == (type & GED_FIELD_TYPE_SIGNED_BIT))
#define IS_SIGNED(type)             (0 != (type & GED_FIELD_TYPE_SIGNED_BIT))
#define IS_DWORD(type)              (0 == (type & GED_FIELD_TYPE_QWORD_BIT))
#define IS_QWORD(type)              (0 != (type & GED_FIELD_TYPE_QWORD_BIT))
#define MAY_BE_QWORD(type)          (0 != (type & (GED_FIELD_TYPE_VARIABLE_BIT | GED_FIELD_TYPE_QWORD_BIT)))
#define IS_DEC(type)                (0 == (type & GED_FIELD_TYPE_HEX_BIT))
#define IS_HEX(type)                (0 != (type & GED_FIELD_TYPE_HEX_BIT))
#define IS_FP(type)                 (0 != (type & GED_FIELD_TYPE_FP_BIT))
#define HAS_GETTER(type)            (0 != (type & GED_FIELD_TYPE_GETTER_BIT))
#define HAS_SETTER(type)            (0 != (type & GED_FIELD_TYPE_SETTER_BIT))
#define IS_RESERVED(type)           (GED_FIELD_TYPE_RESERVED_BIT == type)

// GED_FIELD_TYPE validation macros.
#define HAS_API_FUNCTIONS(type)     (HAS_GETTER(type) || HAS_SETTER(type))
#define CHECK_VARIABLE_BIT(type)    ((0 == (type & GED_FIELD_TYPE_VARIABLE_BIT)) || (GED_FIELD_TYPE_VARIABLE_BIT == \
                                    (type & (GED_FIELD_TYPE_VARIABLE_BIT | GED_FIELD_TYPE_STRING_BIT | GED_FIELD_TYPE_ENUM_BIT))))
#define IS_VALID_STRING(type)       (((type | GED_FIELD_TYPE_GETTER_BIT | GED_FIELD_TYPE_SETTER_BIT) == \
                                    (GED_FIELD_TYPE_STRING_BIT | GED_FIELD_TYPE_ENUM_BIT | GED_FIELD_TYPE_GETTER_BIT | \
                                    GED_FIELD_TYPE_SETTER_BIT)) && (HAS_API_FUNCTIONS(type)))
#define IS_VALID_NUMERIC(type)      ((0 == (type & (GED_FIELD_TYPE_STRING_BIT | GED_FIELD_TYPE_RESERVED_BIT))) && \
                                    CHECK_VARIABLE_BIT(type))


typedef int32_t ged_signed_enum_value_t;
typedef uint32_t ged_unsigned_enum_value_t;

typedef const ged_signed_enum_value_t* ged_signed_enum_entry_type_t;
typedef const ged_unsigned_enum_value_t* ged_unsigned_enum_entry_type_t;

typedef const ged_signed_enum_entry_type_t* ged_signed_table_t;
typedef const ged_unsigned_enum_entry_type_t* ged_unsigned_table_t;

extern const char* ged_signed_enum_value_t_str;
extern const char* ged_unsigned_enum_value_t_str;


// This defines the standard indentation for GED autogenerated files and output.
const size_t indentationFactor = 4;

const uintptr_t GED_NATIVE_INS_SIZE = 16;
const uintptr_t GED_COMPACT_INS_SIZE = 8;
const uintptr_t GED_NUM_OF_COMPACT_INS_DWORDS = GED_COMPACT_INS_SIZE / sizeof(uint32_t); // 2
const uintptr_t GED_NUM_OF_NATIVE_INS_DWORDS = GED_NATIVE_INS_SIZE / sizeof(uint32_t);   // 4
const uintptr_t GED_BYTE_BITS = 8;                                                       // 8
const uintptr_t GED_WORD_BITS = sizeof(uint16_t) * GED_BYTE_BITS;                        // 16
const uintptr_t GED_DWORD_BITS = sizeof(uint32_t) * 8;                                   // 32
const uintptr_t GED_QWORD_BITS = sizeof(uint64_t) * 8;                                   // 64
const uintptr_t GED_NATIVE_INS_SIZE_BITS = GED_NATIVE_INS_SIZE * 8;                      // 128
const uintptr_t GED_COMPACT_INS_SIZE_BITS = GED_COMPACT_INS_SIZE * 8;                    // 64
const uintptr_t GED_MAX_ENTRIES_IN_COMPACT_TABLE = 128;                                  // this is an arbitrary limit, may be changed
const uintptr_t GED_MAX_ENTRIES_IN_OPCODE_TABLE = 1 << 7;                                // 7 bits used for opcodes, 128 total


const uint8_t dwordLow[GED_NUM_OF_NATIVE_INS_DWORDS] =  {  0, 32, 64,  96 };
const uint8_t dwordHigh[GED_NUM_OF_NATIVE_INS_DWORDS] = { 31, 63, 95, 127 };


const unsigned char GED_NATIVE_INS_MASK[GED_NATIVE_INS_SIZE] =
{
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};


// Some useful macros.
#define MIN_2(a,b) ((a>b) ? b : a)
#define MAX_2(a,b) ((a>b) ? a : b)
#define MAX_SIZEOF_2(a,b) (MAX_2(sizeof(a),sizeof(b)))
#define MAX_SIZEOF_3(a,b,c) (MAX_2((MAX_SIZEOF_2(a,b)),sizeof(c)))
#define MAX_SIZEOF_4(a,b,c,d) (MAX_2(MAX_SIZEOF_2(a,b),MAX_SIZEOF_2(c,d)))
#define MAX_SIZEOF_5(a,b,c,d,e) (MAX_2(MAX_SIZEOF_2(a,b),MAX_SIZEOF_3(c,d,e)))
#define MAX_SIZEOF_6(a,b,c,d,e,f) (MAX_2(MAX_SIZEOF_3(a,b,c),MAX_SIZEOF_3(d,e,f)))

#endif // GED_TYPES_INTERNAL_H
