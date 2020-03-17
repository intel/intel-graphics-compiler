#if GED_DISASSEMBLY

#include <iomanip>
#include <cstring>
#include "common/ged_interpreter_types.h"
#include "xcoder/ged_interpreters.h"
#include "xcoder/ged_disassembler.h"

using std::dec;
using std::hex;
using std::setw;
using std::setfill;


/*************************************************************************************************
 * class GEDDisassembler API functions
 *************************************************************************************************/

string GEDDisassembler::Disassemble()
{
    const ged_disassembly_table_t topLevelDisassemblyTable = GetCurrentModelDisassemblyData().DisassemblyTable(GetRawOpcode());
    GEDASSERT(NULL != topLevelDisassemblyTable);
    stringstream strm;
    strm << setfill('0');
    if (IterateDisassemblyBlocks(topLevelDisassemblyTable, strm))
    {
        return strm.str();
    }
    else
    {
        return "BAD INSTRUCTION: " + strm.str() + " " + GetInstructionBytes();
    }
}


/*************************************************************************************************
 * class GEDDisassembler private member functions
 *************************************************************************************************/

bool GEDDisassembler::IterateDisassemblyBlocks(const ged_disassembly_table_t blocks, stringstream& strm)
{
    GEDASSERT(NULL != blocks);
    for (unsigned int i = 0; NULL != blocks[i]; ++i)
    {
        switch (blocks[i]->_entryType)
        {
        case GED_DISASSEMBLY_BLOCK_TYPE_TOKENS:
            if (!PrintDisassemblyTokens(*(blocks[i]), strm)) return false;
            break;
        case GED_DISASSEMBLY_BLOCK_TYPE_FIELD_LIST:
            if (!PrintFieldsList(*(blocks[i]), strm)) return false;
            break;
        case GED_DISASSEMBLY_BLOCK_TYPE_INTERPRETER:
            if (!PrintInterpretedField(*(blocks[i]), strm)) return false;
            break;
        case GED_DISASSEMBLY_BLOCK_TYPE_NEXT_TABLE:
        {
            GEDASSERT(blocks[i]->_nextTable._tableKey._field < GetCurrentModelData().numberOfInstructionFields);
            GED_RETURN_VALUE ret = GED_RETURN_VALUE_INVALID_FIELD;
            uint32_t tableIndex = GetRawField(blocks[i]->_nextTable._tableKey._field, ret);
            if (GED_RETURN_VALUE_SUCCESS != ret) return false;
            if (GED_INTERPRETER_TYPE_NONE != blocks[i]->_nextTable._tableKey._interpType)
            {
                // If the dependee is an interpreted pseudo-field then use the position interpreter to obtain the actual index.
                GEDASSERT(blocks[i]->_nextTable._tableKey._interpId < GetCurrentModelData().numberOfPseudoFields);
                tableIndex = GEDInterpreter::InterpretPositionRaw(tableIndex, blocks[i]->_nextTable._tableKey._interpId,
                             GetCurrentModel(), ret);
                if (GED_RETURN_VALUE_SUCCESS != ret) return false;
            }
            GEDASSERT(NULL != blocks[i]->_nextTable._tablePtr);
            const ged_disassembly_table_t nextBlocks = blocks[i]->_nextTable._tablePtr[tableIndex];
            if (NULL != nextBlocks)
            {
                if (!IterateDisassemblyBlocks(nextBlocks, strm)) return false;
            }
            break;
        }
        default:
            GEDASSERT(0);
            return false;
        }
    }
    return true;
}


bool GEDDisassembler::PrintDisassemblyTokens(const ged_disassembly_block_t& block, stringstream& strm)
{
    GEDASSERT(GED_DISASSEMBLY_BLOCK_TYPE_TOKENS == block._entryType);
    for (unsigned int t = 0; t < block._tokens._numOfTokens; ++t)
    {
        if (!PrintToken(block._tokens._tokens[t], strm)) return false;
    }
    return true;
}


bool GEDDisassembler::PrintFieldsList(const ged_disassembly_block_t& block, stringstream& strm)
{
    GEDASSERT(0 != block._tokens._numOfTokens);
    GEDASSERT(GED_DISASSEMBLY_TOKEN_TYPE_CHAR != block._tokens._tokens[0]._tokenType);
    if (!PrintToken(block._tokens._tokens[0], strm)) return false;

    for (unsigned int f = 1; f < block._tokens._numOfTokens; ++f)
    {
        GEDASSERT(GED_DISASSEMBLY_TOKEN_TYPE_CHAR != block._tokens._tokens[f]._tokenType);
        if (!PrintToken(block._tokens._tokens[f], strm, ", ")) return false;
    }
    return true;
}


bool GEDDisassembler::PrintInterpretedField(const ged_disassembly_block_t& block, stringstream& strm)
{
    GEDASSERT(0 == block._interpreter._padding);
    switch (block._interpreter._interpType)
    {
    case GED_INTERPRETER_TYPE_POSITION:
        GEDASSERT(GED_INTERPRETER_TYPE_POSITION == block._interpreter._field._interpType); // verify consistency
        return PrintPositionInterpretedField(block._interpreter, strm);
    case GED_INTERPRETER_TYPE_REENUM:
    {
        // TODO: need to handle types other than unsigned
        GEDASSERT(GED_INTERPRETER_TYPE_REENUM == block._interpreter._field._interpType); // verify consistency
        GEDASSERT(1 == block._interpreter._numberOfFields);
        uint32_t value = MAX_UINT32_T;
        if (!GetGeneralizedFieldValue(block._interpreter._field, value)) return false;
        strm << value;
        break;
    }
    case GED_INTERPRETER_TYPE_DIV:
    {
        // TODO: need to handle types other than unsigned
        GEDASSERT(2 == block._interpreter._numberOfFields);
        uint32_t leftVal = MAX_UINT32_T;
        if (!GetGeneralizedFieldValue(block._interpreter._fieldList[0], leftVal)) return false;
        uint32_t rightVal = MAX_UINT32_T;
        if (!GetGeneralizedFieldValue(block._interpreter._fieldList[1], rightVal)) return false;
        GEDASSERT(0 != rightVal);
        GEDASSERT(0 == leftVal % rightVal);
        strm << (leftVal / rightVal); // TODO: should this be hex/dec etc.
    }
    break;
    case GED_INTERPRETER_TYPE_NUMTYPE:
    {
        GEDASSERT(2 == block._interpreter._numberOfFields);
        // TODO: Currently only explicit field values may be displayed using this interpreter
        GEDASSERT(GED_INTERPRETER_TYPE_NONE == block._interpreter._fieldList[0]._interpType);
        GED_FIELD_TYPE fieldType = fieldTypesByField[block._interpreter._fieldList[0]._field];
        GEDASSERT(IS_VALID_NUMERIC(fieldType)); // only numeric values may be displayed using a base interpreter
        /* GED_NUMERIC_TYPE */ uint32_t numericType = MAX_UINT32_T;
        if (!GetGeneralizedFieldValue(block._interpreter._fieldList[1], numericType)) return false;
        switch (numericType)
        {
        case GED_NUMERIC_TYPE_INT:
            break;
        case GED_NUMERIC_TYPE_FP:
            fieldType |= GED_FIELD_TYPE_FP_BIT;
            break;
        default:
            GEDASSERT(0);
        };
        return PrintNumericField(block._interpreter._fieldList[0]._field, fieldType, strm);
    }
    case GED_INTERPRETER_TYPE_COLLECT:
        GEDASSERT(0 == block._interpreter._numberOfFields);
        {
            GED_RETURN_VALUE ret = GED_RETURN_VALUE_INVALID_FIELD;
            const uint8_t fieldWidth =
                (ModelsArray[GetCurrentModel()].collectionTables[block._interpreter._noargId]._width + (uint8_t)3) / (uint8_t)4;
            strm << "0x" << hex << setw(fieldWidth) << GEDInterpreter::CollectFields(*this, block._interpreter._noargId, ret) << dec;
            return (GED_RETURN_VALUE_SUCCESS == ret);
        }
    default:
        GEDASSERT(0);
        return false;
    }
    return true;
}


bool GEDDisassembler::PrintToken(const ged_disassembly_token_t& token, stringstream& strm, const string& prefix /* = "" */)
{
    switch (token._tokenType)
    {
    case GED_DISASSEMBLY_TOKEN_TYPE_OPCODE:
        strm << GetMnemonic();
        break;
    case GED_DISASSEMBLY_TOKEN_TYPE_COMPACTED:
        // TODO: This string should probably be stored in the predefined DB and exposed via the autogenerated files.
        if (IsCompact()) strm << prefix << "Compacted";
        break;
    case GED_DISASSEMBLY_TOKEN_TYPE_CHAR:
        strm << token._char;
        break;
    case GED_DISASSEMBLY_TOKEN_TYPE_FIELD:
        return PrintField(token._field, strm, prefix);
    case GED_DISASSEMBLY_TOKEN_TYPE_RAW_FIELD:
        return PrintRawField(token._field, strm, prefix);
    default:
        GEDASSERT(0);
        return false;
    }
    return true;
}


bool GEDDisassembler::PrintField(const uint16_t field, stringstream& strm, const string& prefix /* = "" */)
{
    GEDASSERT(field < GetCurrentModelData().numberOfInstructionFields);
    const GED_FIELD_TYPE fieldType = fieldTypesByField[field];
    if (IS_STRING(fieldType))
    {
        GEDASSERT(IS_VALID_STRING(fieldType));
        return PrintStringField(field, strm, prefix);
    }
    else
    {
        GEDASSERT(IS_VALID_NUMERIC(fieldType));
        return PrintNumericField(field, fieldType, strm, prefix);
    }
}


bool GEDDisassembler::PrintStringField(const uint16_t field, stringstream& strm, const string& prefix /* = "" */)
{
    GED_RETURN_VALUE ret = GED_RETURN_VALUE_INVALID_FIELD;
    const uint32_t tableIndex = GetUnsignedField(field, ret);
    if (GED_RETURN_VALUE_SUCCESS != ret) return false;
    GEDASSERT(NULL != stringGettersByField[field]);
    if (NULL != stringGettersByField[field][tableIndex])
    {
        if (0 != strlen(stringGettersByField[field][tableIndex]))
        {
            strm << prefix << stringGettersByField[field][tableIndex];
        }
    }
    return true;
}


bool GEDDisassembler::PrintNumericField(const uint16_t field, const GED_FIELD_TYPE fieldType, stringstream& strm,
                                        const string& prefix /* = "" */)
{
    GED_RETURN_VALUE ret = GED_RETURN_VALUE_INVALID_FIELD;
    strm << prefix;
    if (IS_FP(fieldType))
    {
        // Only signed values are currently allowed to be displayed in floating-point format.
        GEDASSERT(IS_SIGNED(fieldType) || IS_VARIABLE(fieldType));

        // All floating-point numbers will be displayed in decimal base regardless of the hex-bit in the field's type (which is
        // ignored).
        strm << std::setprecision(16) << dec;
    }
    else if (IS_HEX(fieldType))
    {
        const uint8_t fieldWidth = (GetFieldWidth(field, false) + (uint8_t)3) / (uint8_t)4;
        GEDASSERT((uint8_t)0 != fieldWidth);
        strm << hex << "0x" << setw(fieldWidth);
    }
    else strm << dec;

    if (IS_SIGNED(fieldType))
    {
        if (IS_QWORD(fieldType))
        {
            if (IS_FP(fieldType)) strm << GetSigned64Field(field, ret)._df;
            else strm << GetSigned64Field(field, ret)._qw;
        }
        else if (IS_VARIABLE(fieldType))
        {
            const int64_t temp = GetUnsigned64Field(field, ret);
            const SignedQNum val = *(reinterpret_cast<const SignedQNum*>(&temp));
            if (IS_FP(fieldType)) strm << val._df;
            else
            {
                if (IS_HEX(fieldType)) strm << (val._qw & BitsToMaxValue(GetFieldWidth(field)));
                else strm << val._qw;
            }
        }
        else
        {
            if (IS_FP(fieldType)) strm << GetSignedField(field, ret)._f;
            else strm << GetSignedField(field, ret)._dw;
        }
    }
    else
    {
        if (IS_QWORD(fieldType)) strm << GetUnsigned64Field(field, ret);
        else if (IS_VARIABLE(fieldType))
        {
            strm << (GetUnsigned64Field(field, ret) & BitsToMaxValue(GetFieldWidth(field)));
        }
        else strm << GetUnsignedField(field, ret);
    }
    return (GED_RETURN_VALUE_SUCCESS == ret);
}


bool GEDDisassembler::PrintRawField(const uint16_t field, stringstream& strm, const string& prefix /* = "" */)
{
    GEDASSERT(field < GetCurrentModelData().numberOfInstructionFields);
    GEDASSERT(IS_VALID_STRING(fieldTypesByField[field]) || IS_VALID_NUMERIC(fieldTypesByField[field]));
    GED_RETURN_VALUE ret = GED_RETURN_VALUE_INVALID_FIELD;
    const uint8_t fieldWidth = (GetFieldWidth(field) + (uint8_t)3) / (uint8_t)4;
    GEDASSERT((uint8_t)0 != fieldWidth);
    strm << prefix << hex << "0x" << setw(fieldWidth) << GetRawField(field, ret);
    return (GED_RETURN_VALUE_SUCCESS == ret);
}


bool GEDDisassembler::PrintPositionInterpretedField(const ged_disassembly_block_interpreter_t& interpreter, stringstream& strm)
{
    GEDASSERT(1 == interpreter._numberOfFields);
    uint32_t value = MAX_UINT32_T;
    if (!GetGeneralizedFieldValue(interpreter._field, value)) return false; // TODO: Currently this function returns only uint32_t
    if (IS_STRING(pseudoFieldTypesByField[interpreter._field._interpId]))
    {
        GEDASSERT(IS_VALID_STRING(pseudoFieldTypesByField[interpreter._field._interpId]));
        GEDASSERT(NULL != stringGettersByPseudoField[interpreter._field._interpId]);
        if (NULL != stringGettersByPseudoField[interpreter._field._interpId][value])
        {
            strm << stringGettersByPseudoField[interpreter._field._interpId][value];
        }
    }
    else
    {
        GEDASSERT(IS_VALID_NUMERIC(pseudoFieldTypesByField[interpreter._field._interpId]));

        if (IS_HEX(pseudoFieldTypesByField[interpreter._field._interpId]))
        {
            const uint8_t fieldWidth = (GetFieldWidth(interpreter._field._interpId, true) + (uint8_t)3) / (uint8_t)4;
            GEDASSERT((uint8_t)0 != fieldWidth);
            strm << hex << "0x" << setw(fieldWidth);
        }
        else strm << dec;

        if IS_SIGNED(pseudoFieldTypesByField[interpreter._field._interpId])
        {
            strm << (int32_t)value;
        }
        else
        {
            strm << value;
        }
    }
    return true;
}


bool GEDDisassembler::GetGeneralizedFieldValue(const ged_generalized_field_t& genField, uint32_t& value)
{
    // TODO: need to handle other field types besides unsigned
    GED_RETURN_VALUE ret = GED_RETURN_VALUE_INVALID_FIELD;
    switch (genField._interpType)
    {
    case GED_INTERPRETER_TYPE_NONE:
        value = GetUnsignedField(genField._field, ret);
        return (GED_RETURN_VALUE_SUCCESS == ret);
    case GED_INTERPRETER_TYPE_POSITION:
        value = GetRawField(genField._field, ret);
        if (GED_RETURN_VALUE_SUCCESS != ret) return false;
        value = GEDInterpreter::InterpretPosition(value, genField._interpId, GetCurrentModel(), ret);
        return (GED_RETURN_VALUE_SUCCESS == ret);
    case GED_INTERPRETER_TYPE_REENUM:
        GEDASSERT(IS_ENUM(fieldTypesByField[genField._field]));
        value = GetUnsignedField(genField._field, ret);
        if (GED_RETURN_VALUE_SUCCESS != ret) return false;
        value = GEDInterpreter::ReinterpretEnum(value, genField._interpId, GetCurrentModel(), ret);
        return (GED_RETURN_VALUE_SUCCESS == ret);
    default:
        GEDASSERT(0);
        return false;
    }
}

#endif // GED_DISASSEMBLY
