#ifndef GED_INTERNAL_API_H
#define GED_INTERNAL_API_H

#include "common/ged_base.h"
#include "common/ged_ins_decoding_table.h"
#include "common/ged_compact_mapping_table.h"
#include "common/ged_ins_encoding_masks.h"
#include "common/ged_interpreter_types.h"
#include "common/ged_ins_disassembly_table.h"


# if GED_DISASSEMBLY
enum GED_NUMERIC_TYPE
{
    GED_NUMERIC_TYPE_INT = 0,
    GED_NUMERIC_TYPE_FP  = 1
};
# endif // GED_DISASSEMBLY


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
# if GED_DISASSEMBLY
typedef const ged_disassembly_table_t (*DisassemblyTableFunc)(uint32_t);
# endif // GED_DISASSEMBLY

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

# if GED_DISASSEMBLY
struct ModelDisassemblyData
{
    DisassemblyTableFunc DisassemblyTable;
};

extern ModelDisassemblyData ModelsDisassemblyArray[];
# endif // GED_DISASSEMBLY


extern ModelData ModelsArray[];


extern const unsigned int numOfSupportedModels;
extern const char* modelNames[];

extern bool GetModelByName(const string& name, /* GED_MODEL */ unsigned int& model);


extern GED_FIELD_TYPE fieldTypesByField[];
extern GED_FIELD_TYPE pseudoFieldTypesByField[];


# if GED_DISASSEMBLY // TODO: This should be guarded by a different define, probably GED_LOG.
extern const char** stringGettersByField[];
extern const char** stringGettersByPseudoField[];
# endif // GED_DISASSEMBLY

# if GED_VALIDATION_API
extern const char* fieldNameByField[];
extern const char* fieldNameByPseudoField[];
# endif

#endif // GED_INTERNAL_API_H
