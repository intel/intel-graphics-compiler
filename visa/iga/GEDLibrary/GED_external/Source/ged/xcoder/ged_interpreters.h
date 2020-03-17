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
        { return InterpretPositionInternal(value, interpId, modelId, GED_VALUE_TYPE_ENCODED, ret); }


    inline static uint32_t InterpretPosition(uint32_t value, /* GED_PSEUDO_FIELD */ const uint32_t interpId,
                                             const /* GED_MODEL */ uint8_t modelId, GED_RETURN_VALUE& ret)
        { return InterpretPositionInternal(value, interpId, modelId, GED_VALUE_TYPE_PROCESSED, ret); }


    static uint32_t ReinterpretEnum(uint32_t value, /* GED_REINTERPRETED_ENUM */ const uint32_t interpId,
                                    const /* GED_MODEL */ uint8_t modelId, GED_RETURN_VALUE& ret);


    static uint32_t CollectFields(GEDIns& ins, /* GED_COLLECTOR */ const uint32_t interpId, GED_RETURN_VALUE& ret);


    inline static GED_RETURN_VALUE SetInterpretedPosition(uint32_t& writeTo, /* GED_PSEUDO_FIELD */ const uint32_t interpId,
                                                          const /* GED_MODEL */ uint8_t modelId, uint32_t valueToWrite)
    {
        return SetInterpretedPositionInternal(writeTo, interpId, modelId, GED_VALUE_TYPE_PROCESSED, valueToWrite);
    }

private:
    // PRIVATE MEMBER FUNCTIONS

    static uint32_t InterpretPositionInternal(uint32_t value, /* GED_PSEUDO_FIELD */ const uint32_t interpId,
                                              const /* GED_MODEL */ uint8_t modelId, const GED_VALUE_TYPE valueType,
                                              GED_RETURN_VALUE& ret);

    static GED_RETURN_VALUE SetInterpretedPositionInternal(uint32_t& writeTo, /* GED_PSEUDO_FIELD */ const uint32_t interpId,
                                                           const /* GED_MODEL */ uint8_t modelId, const GED_VALUE_TYPE valueType,
                                                           uint32_t valueToWrite);
};

#endif // GED_POSITION_INTERPRETER_H
