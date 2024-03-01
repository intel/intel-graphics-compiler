/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef GED_POSITION_INTERPRETER_H
#define GED_POSITION_INTERPRETER_H

#include "common/ged_ins_decoding_table.h"
#include "xcoder/ged_ins.h"


class GEDInterpreter
{
public:
    // STATIC FUNCTIONS

    inline static uint32_t InterpretPositionRaw(uint32_t value, /* GED_PSEUDO_FIELD */ const uint32_t interpId,
                                                const /* GED_MODEL */ uint8_t modelId, GED_RETURN_VALUE& ret)
        { return (uint32_t)InterpretPositionInternal(value, interpId, modelId, GED_VALUE_TYPE_ENCODED, ret); }


    inline static uint32_t InterpretPosition(uint32_t value, /* GED_PSEUDO_FIELD */ const uint32_t interpId,
                                             const /* GED_MODEL */ uint8_t modelId, GED_RETURN_VALUE& ret)
        { return (uint32_t)InterpretPositionInternal(value, interpId, modelId, GED_VALUE_TYPE_PROCESSED, ret); }

    inline static uint64_t InterpretPosition(uint64_t value, /* GED_PSEUDO_FIELD */ const uint32_t interpId,
                                             const /* GED_MODEL */ uint8_t modelId, GED_RETURN_VALUE& ret)
        { return InterpretPositionInternal(value, interpId, modelId, GED_VALUE_TYPE_PROCESSED, ret); }



    static uint32_t ReinterpretEnum(uint32_t value, /* GED_REINTERPRETED_ENUM */ const uint32_t interpId,
                                    const /* GED_MODEL */ uint8_t modelId, GED_RETURN_VALUE& ret);


    static uint32_t CollectFields(GEDIns& ins, /* GED_COLLECTOR */ const uint32_t interpId, GED_RETURN_VALUE& ret);


    inline static GED_RETURN_VALUE SetInterpretedPosition(uint32_t& writeTo, /* GED_PSEUDO_FIELD */ const uint32_t interpId,
                                                          const /* GED_MODEL */ uint8_t modelId, uint32_t valueToWrite)
    {
        uint64_t writeTo64 = writeTo, valueToWrite64 = valueToWrite;
        GED_RETURN_VALUE rv = SetInterpretedPositionInternal(writeTo64, interpId, modelId, GED_VALUE_TYPE_PROCESSED, valueToWrite64);
        writeTo = (uint32_t)writeTo64;
        return rv;
    }

    inline static GED_RETURN_VALUE SetInterpretedPosition(uint64_t& writeTo, /* GED_PSEUDO_FIELD */ const uint32_t interpId,
                                                          const /* GED_MODEL */ uint8_t modelId, uint64_t valueToWrite)
    {
        return SetInterpretedPositionInternal(writeTo, interpId, modelId, GED_VALUE_TYPE_PROCESSED, valueToWrite);
    }

private:
    // PRIVATE MEMBER FUNCTIONS

    static uint64_t InterpretPositionInternal(uint64_t value, /* GED_PSEUDO_FIELD */ const uint32_t interpId,
                                              const /* GED_MODEL */ uint8_t modelId, const GED_VALUE_TYPE valueType,
                                              GED_RETURN_VALUE& ret);

    static GED_RETURN_VALUE SetInterpretedPositionInternal(uint64_t& writeTo, /* GED_PSEUDO_FIELD */ const uint32_t interpId,
                                                           const /* GED_MODEL */ uint8_t modelId, const GED_VALUE_TYPE valueType,
                                                           uint64_t valueToWrite);
};

#endif // GED_POSITION_INTERPRETER_H
