/*========================== begin_copyright_notice ============================

Copyright (c) 2015-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#ifndef GED_INTERNAL_API_H
#define GED_INTERNAL_API_H

#include "common/ged_base.h"
#include "common/ged_ins_decoding_table.h"
#include "common/ged_compact_mapping_table.h"
#include "common/ged_ins_encoding_masks.h"
#include "common/ged_interpreter_types.h"


union SignedDNum
{
    int32_t _dw;    // signed double-word
    float _f;       // single-precision floating-point
};


union SignedQNum
{
    int64_t _qw;    // signed quad-word
    double _df;     // double-precision floating-point
};


typedef const ged_ins_decoding_table_t (*InsFieldTableFunc)(uint32_t, GED_INS_TYPE);
typedef const ged_compact_mapping_table_t (*MappingTableFunc)(uint32_t);
typedef const ged_instruction_masks_table_t (*EncodingMasksTableFunc)(uint32_t, GED_INS_TYPE);

struct OpcodeTables
{
    const ged_ins_decoding_table_t        nativeDecoding;
    const ged_instruction_masks_table_t   nativeEncodingMasks;
    const ged_ins_decoding_table_t        compactDecoding;
    const ged_instruction_masks_table_t   compactEncodingMasks;
    const ged_compact_mapping_table_t     compactMapping;
};

struct ModelData
{
    const OpcodeTables* opcodeTables;
    const char* modelVersion;
    const uint32_t numberOfInstructionFields;
    const ged_field_enum_table_t Opcodes;
    const uint32_t numberOfPseudoFields;
    const ged_ins_field_entry_t* pseudoFields;
    const uint32_t numberOfReinterpretedEnums;
    const ged_unsigned_table_t* reinterpretedEnums;
    const uint32_t numberOfCollectors;
    const ged_collector_info_t* collectionTables;
};


extern ModelData ModelsArray[];


extern const unsigned int numOfSupportedModels;
extern const char* modelNames[];

extern bool GetModelByName(const string& name, /* GED_MODEL */ unsigned int& model);


extern GED_FIELD_TYPE fieldTypesByField[];
extern GED_FIELD_TYPE pseudoFieldTypesByField[];


# if GED_VALIDATION_API
extern const char* fieldNameByField[];
extern const char* fieldNameByPseudoField[];
# endif

#endif // GED_INTERNAL_API_H
