/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/ged_base.h"
#include "xcoder/ged_internal_api.h"
#include "xcoder/ged_restrictions_handler.h"
#include "xcoder/ged_interpreters.h"


/*************************************************************************************************
 * class GEDInterpreter static API functions
 *************************************************************************************************/

uint64_t GEDInterpreter::InterpretPositionInternal(uint64_t value, /* GED_PSEUDO_FIELD */ const uint32_t interpId,
                                                   const /* GED_MODEL */ uint8_t modelId, const GED_VALUE_TYPE valueType,
                                                   GED_RETURN_VALUE& ret)
{
    const ModelData& modelData = ModelsArray[modelId];
    if ((NULL == modelData.pseudoFields) || (interpId >= modelData.numberOfPseudoFields))
    {
        ret = GED_RETURN_VALUE_INVALID_INTERPRETER;
        return value;
    }
    const ged_ins_field_entry_t& dataEntry = modelData.pseudoFields[interpId];
    if (GED_TABLE_ENTRY_TYPE_NOT_SUPPORTED == dataEntry._entryType)
    {
        ret = GED_RETURN_VALUE_INVALID_INTERPRETER;
        return value;
    }
    ret = GED_RETURN_VALUE_SUCCESS; // if the interpreter is valid, extracting the value is bound to succeed
    GEDASSERT(GED_TABLE_ENTRY_TYPE_CONSECUTIVE == dataEntry._entryType);
    value &= dataEntry._consecutive._position._bitMask;
    GEDASSERT(dataEntry._consecutive._position._shift >= 0);
    value >>= dataEntry._consecutive._position._shift;
    if (GED_VALUE_TYPE_ENCODED == valueType) return value;
    GEDASSERT(GED_VALUE_TYPE_PROCESSED == valueType);
    value = GEDRestrictionsHandler::HandleDecodingRestrictions(&dataEntry, value, ret);
    return value;
}


GED_RETURN_VALUE GEDInterpreter::SetInterpretedPositionInternal(uint64_t& writeTo, /* GED_PSEUDO_FIELD */ const uint32_t interpId,
                                                                const /* GED_MODEL */ uint8_t modelId, const GED_VALUE_TYPE valueType,
                                                                uint64_t valueToWrite)
{
    const ModelData& modelData = ModelsArray[modelId];
    GEDASSERT(interpId < modelData.numberOfPseudoFields);
    if (NULL == modelData.pseudoFields) return GED_RETURN_VALUE_INVALID_INTERPRETER;

    const ged_ins_field_entry_t& dataEntry = modelData.pseudoFields[interpId];
    if (GED_TABLE_ENTRY_TYPE_NOT_SUPPORTED == dataEntry._entryType) return GED_RETURN_VALUE_INVALID_INTERPRETER;
    GEDASSERT(GED_TABLE_ENTRY_TYPE_CONSECUTIVE == dataEntry._entryType);
    GEDASSERT(dataEntry._consecutive._position._shift >= 0);

    if (!GEDRestrictionsHandler::HandleEncodingRestrictions(&dataEntry, valueType, valueToWrite))
    {
        return GED_RETURN_VALUE_INVALID_VALUE;
    }
    // Shift the value into position.
    valueToWrite <<= dataEntry._consecutive._position._shift;

    // Clear the field in the destination.
    writeTo &= ~dataEntry._consecutive._position._bitMask;

    // No need to apply mask to valueToWrite, as it's always unsigned. If was signed, applying the mask was required (for negatives)

    // Now set the field with the value itself.
    writeTo |= valueToWrite;
    return GED_RETURN_VALUE_SUCCESS;
}


/*************************************************************************************************
 * class GEDInterpreter API functions
 *************************************************************************************************/

uint32_t GEDInterpreter::ReinterpretEnum(uint32_t value, /* GED_REINTERPRETED_ENUM */ const uint32_t interpId,
                                         const /* GED_MODEL */ uint8_t modelId, GED_RETURN_VALUE& ret)
{
    const ModelData& modelData = ModelsArray[modelId];
    GEDASSERT(interpId < modelData.numberOfReinterpretedEnums);
    GEDASSERT(NULL != modelData.reinterpretedEnums);
    if (NULL == modelData.reinterpretedEnums[interpId])
    {
        ret = GED_RETURN_VALUE_INVALID_INTERPRETER;
        return value;
    }
    if (NULL == modelData.reinterpretedEnums[interpId][value])
    {
        ret = GED_RETURN_VALUE_INVALID_VALUE;
        return value;
    }
    ret = GED_RETURN_VALUE_SUCCESS;
    return *(modelData.reinterpretedEnums[interpId][value]);
}


uint32_t GEDInterpreter::CollectFields(GEDIns& ins, /* GED_COLLECTOR */ const uint32_t interpId, GED_RETURN_VALUE& ret)
{
    const ModelData& modelData = ModelsArray[ins.GetCurrentModel()];
    GEDASSERT(interpId < modelData.numberOfCollectors);
    if (NULL == modelData.collectionTables || NULL == modelData.collectionTables[interpId]._table)
    {
        ret = GED_RETURN_VALUE_INVALID_INTERPRETER;
        return MAX_UINT32_T;
    }

    GEDASSERT(0 != modelData.collectionTables[interpId]._numberOfFields);
    uint32_t collectedValue = 0; // this will hold the collected value
    for (unsigned int m = 0; m < modelData.collectionTables[interpId]._numberOfFields; ++m)
    {
        const ged_compact_mapping_entry_t& entry = modelData.collectionTables[interpId]._table[m];
        if (GED_MAPPING_TABLE_ENTRY_TYPE_VALUE_MAPPING_CONSECUTIVE == entry._entryType)
        {
            uint32_t fieldRawValue = ins.GetRawField(entry._field, ret); // get the current field-raw-value to be collected
            if (GED_RETURN_VALUE_SUCCESS != ret) return MAX_UINT32_T;
            fieldRawValue &= entry._consecutive._fromMask;
            GEDASSERT(0 == entry._consecutive._to._dwordIndex); // only 32-bit values are allowed
            GEDASSERT(0 <= entry._consecutive._to._shift); // only left shifts are expected since the source is right-shifted to zero
            fieldRawValue <<= entry._consecutive._to._shift;
            collectedValue |= fieldRawValue;
        }
        else if (GED_MAPPING_TABLE_ENTRY_TYPE_VALUE_MAPPING_FRAGMENTED == entry._entryType)
        {
            const uint32_t fieldRawValue = ins.GetRawField(entry._field, ret); // get the current field-raw-value to be collected
            if (GED_RETURN_VALUE_SUCCESS != ret) return MAX_UINT32_T;
            for (uint32_t f = 0; f < entry._fragmented._numOfMappingFragments; ++f)
            {
                const ged_compact_mapping_fragment_t& fragment = entry._fragmented._fragments[f];
                GEDASSERT(GED_COMPACT_MAPPING_TYPE_1x1 == fragment._mappingType);
                GEDASSERT(0 == fragment._from._dwordIndex); // only 32-bit values are allowed
                GEDASSERT(0 == fragment._to._dwordIndex); // only 32-bit values are allowed
                uint32_t fragmentRawValue = fieldRawValue & fragment._from._bitMask;
                const int8_t shift = fragment._from._shift - fragment._to._shift;
                if (shift > 0)
                {
                    fragmentRawValue >>= shift;
                }
                else
                {
                    fragmentRawValue <<= shift;
                }
                collectedValue |= fragmentRawValue;
            }
        }
        else
        {
            GEDASSERT(0); // only explicit mappings are allowed
        }
    }
    return collectedValue;
}
