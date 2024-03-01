/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef GED_RESTRICTIONS_HANDLER_H
#define GED_RESTRICTIONS_HANDLER_H

#include "common/ged_base.h"
#include "common/ged_ins_decoding_table.h"


class GEDRestrictionsHandler
{
public:
    // STATIC FUNCTIONS

    template<typename NumType>
    static NumType HandleDecodingRestrictions(const ged_ins_field_entry_t* dataEntry, NumType val, GED_RETURN_VALUE& ret);

    template<typename NumType>
    inline static bool HandleEncodingRestrictions(const ged_ins_field_entry_t* dataEntry, const GED_VALUE_TYPE valueType,
                                                  NumType& val);

    template<typename NumType>
    static bool ConvertEnumeratedValueToRawEncodedValue(NumType& val, const uint32_t maxValue,
                                                        const ged_field_enum_table_t enumTable);

    template<typename NumType>
    static bool ConvertRawEncodedValueToEnumeratedValue(NumType& val, const ged_field_enum_table_t enumTable);

    template<typename NumType>
    static inline bool IsNegative(const NumType val, const uint8_t highBit) { return (0 != (val & checkNegativeTable[highBit])); }

private:
    inline static bool CheckMaxValue(const int32_t val, const uint8_t bits);
    inline static bool CheckMaxValue(const int64_t val, const uint8_t bits);
    inline static bool CheckMaxValue(const uint32_t val, const uint8_t bits);
    inline static bool CheckMaxValue(const uint64_t val, const uint8_t bits);
    inline static bool CheckMaxValue(const int32_t val, const ged_field_type_modifier_t& fieldType);
    inline static bool CheckMaxValue(const int64_t val, const ged_field_type_modifier_t& fieldType);
    inline static bool CheckMaxValue(const uint32_t val, const ged_field_type_modifier_t& fieldType);
    inline static bool CheckMaxValue(const uint64_t val, const ged_field_type_modifier_t& fieldType);

    template<typename NumType>
    inline static bool VerifyValueRestriction(const NumType val, const ged_field_restriction_value_t resVal);

    template<typename NumType>
    inline static bool VerifyRangeRestriction(const NumType val, const ged_field_restriction_range_t& range);

    template<typename NumType>
    inline static bool VerifyMaskRestriction(const NumType val, const ged_field_restriction_mask_t mask);

    template<typename NumType>
    inline static void HandleDuplication(GEDFORASSERT(const ged_ins_field_entry_t* dataEntry) COMMA NumType& val,
                                         const ged_field_type_modifier_t::ged_field_type_attributes& fieldAttributes);

    template<typename NumType>
    inline static bool HandleNonEnumEncodingRestrictions(const ged_ins_field_entry_t* dataEntry, NumType& val);

    template<typename NumType>
    static bool HandleNonEnumEncodingRestriction(const ged_ins_field_entry_t* dataEntry,
                                                 const ged_field_restriction_t* restriction, NumType& val);

private:
    // PRIVATE STATIC DATA MEMBERS

    // The table below is used for checking if a value is negative. Since instruction fields are currently limited to 64 bits, we only
    // need 64 entries (for the 64 possible locations of the field's MSB).
    static const int64_t checkNegativeTable[GED_QWORD_BITS];
};


/*************************************************************************************************
 * class GEDRestrictionsHandler static functions
 *************************************************************************************************/

template <typename NumType>
NumType GEDRestrictionsHandler::HandleDecodingRestrictions(const ged_ins_field_entry_t* dataEntry, NumType val, GED_RETURN_VALUE& ret)
{
    GEDASSERT(NULL != dataEntry);
    GEDASSERT(GED_RETURN_VALUE_SUCCESS == ret);
    if (NULL == dataEntry->_restrictions) return val; // no restrictions (value modifiers)

    GEDFORASSERT(bool isVariableField = false);
    GEDFORASSERT(static const unsigned int fieldTypeLocation = 0);
    GEDFORASSERT(unsigned int paddingLocation = 0);
    GEDFORASSERT(bool foundEnumeration = false);
    unsigned int restrictionIndex = 0;
    const ged_field_restriction_t* restriction = dataEntry->_restrictions[restrictionIndex];
    while (NULL != restriction)
    {
        GEDASSERT(!foundEnumeration); // enumerations are only allowed as the final restriction (followed by the NULL terminator)

        // When decoding a field, only padding and enums should be handled since they convert the raw encoded value into the
        // actual meaningful value we return to the user. This implicitly validates enumerated values, however we currently do not
        // validate other restrictions.
        switch (restriction->_restrictionType)
        {
        case GED_FIELD_RESTRICTIONS_TYPE_VALUE:
        case GED_FIELD_RESTRICTIONS_TYPE_RANGE:
        case GED_FIELD_RESTRICTIONS_TYPE_MASK:
            break;
        case GED_FIELD_RESTRICTIONS_TYPE_PADDING:
            // Only one padding is allowed and it is either first, or immediately follows the field-type modifier.
            GEDASSERT(paddingLocation == restrictionIndex);
            val |= restriction->_padding._value;
            break;
        case GED_FIELD_RESTRICTIONS_TYPE_ENUM:
            if (!ConvertRawEncodedValueToEnumeratedValue(val, restriction->_enumTable))
            {
                ret = GED_RETURN_VALUE_INVALID_VALUE;
                return val;
            }
            GEDFORASSERT(foundEnumeration = true);
            break;
        case GED_FIELD_RESTRICTIONS_TYPE_FIELD_TYPE:
            GEDASSERT(fieldTypeLocation == restrictionIndex);
            GEDASSERT(!isVariableField); // there may only be one field-type modifier
            GEDFORASSERT(isVariableField = true);
            GEDFORASSERT(paddingLocation = 1);
            // For non-negative values, it may be necessary to trim excess bits since the field's upper-limit bit-size may be larger
            // than the actual bits. For negative numbers, the sign-extension has already taken care of this.
            if (restriction->_fieldType._attr._signed && (0 != (val & checkNegativeTable[restriction->_fieldType._attr._bits - 1])))
            {
                break;
            }
            val &= BitsToMaxValue(restriction->_fieldType._attr._bits);
            break;
        case GED_FIELD_RESTRICTIONS_TYPE_NONE: // if there is a non-null restriction pointer, it should be a valid restriction
        default:
            GEDASSERT(0);
        }
        restriction = dataEntry->_restrictions[++restrictionIndex];
    }
    return val;
}


template<typename NumType>
bool GEDRestrictionsHandler::HandleEncodingRestrictions(const ged_ins_field_entry_t* dataEntry, const GED_VALUE_TYPE valueType,
                                                        NumType& val)
{
    GEDASSERT(NULL != dataEntry);
    if (NULL == dataEntry->_restrictions || GED_VALUE_TYPE_ENCODED == valueType)
    {
        // No restrictions or value modifiers, just check the max value.
        return CheckMaxValue(val, dataEntry->_bitSize);
    }
    GEDASSERT(GED_VALUE_TYPE_PROCESSED == valueType);
    GEDASSERT(NULL != dataEntry->_restrictions[0]);
    if (GED_FIELD_RESTRICTIONS_TYPE_ENUM == dataEntry->_restrictions[0]->_restrictionType)
    {
        GEDASSERT(NULL == dataEntry->_restrictions[1]);
        return ConvertEnumeratedValueToRawEncodedValue(val, (uint32_t)MaxValue(dataEntry), dataEntry->_restrictions[0]->_enumTable);
    }
    return HandleNonEnumEncodingRestrictions(dataEntry, val);
}


template<typename NumType>
bool GEDRestrictionsHandler::ConvertEnumeratedValueToRawEncodedValue(NumType& val, const uint32_t maxValue,
                                                                     const ged_field_enum_table_t enumTable)
{
    // Traverse the enumeration table and find the index for the given value.
    for (uint32_t i = 0; i <= maxValue; ++i)
    {
        if (NULL != enumTable[i])
        {
            const NumType tabVal = *(((const uint32_t* const*)(enumTable))[i]);
            if (tabVal == val)
            {
                val = (NumType)i; // the actual value that needs to be encoded is the index
                return true;
            }
        }
    }
    return false; // the value was not found in the table meaning that it is not a valid value for this field.
}


template<typename NumType>
bool GEDRestrictionsHandler::ConvertRawEncodedValueToEnumeratedValue(NumType& val, const ged_field_enum_table_t enumTable)
{
    // TODO: Need to check that the given val does not exceed the maximum value for the entry containing this enum table

    if (NULL == enumTable[val]) return false; // there is no valid enumeration value for the encoded value
    val = *(((const NumType * const*)(enumTable))[val]); // the actual decoded value should be the enumeration value
    return true;
}


/*************************************************************************************************
 * class GEDRestrictionsHandler private member functions
 *************************************************************************************************/

bool GEDRestrictionsHandler::CheckMaxValue(const int32_t val, const uint8_t bits)
{
    GEDASSERT(GED_DWORD_BITS >= bits);
    if (GED_DWORD_BITS == bits) return true;
    const int32_t shifted = (val >> (bits - 1));
    return (0 == shifted || (int32_t)-1 == shifted);
}


bool GEDRestrictionsHandler::CheckMaxValue(const int64_t val, const uint8_t bits)
{
    GEDASSERT(GED_QWORD_BITS >= bits);
    if (GED_QWORD_BITS == bits) return true;
    const int64_t shifted = (val >> (bits - 1));
    return (0 == shifted || (int64_t)-1 == shifted);
}


bool GEDRestrictionsHandler::CheckMaxValue(const uint32_t val, const uint8_t bits)
{
    GEDASSERT(GED_DWORD_BITS >= bits);
    if (GED_DWORD_BITS == bits) return true;
    return (0 == (val >> bits));
}


bool GEDRestrictionsHandler::CheckMaxValue(const uint64_t val, const uint8_t bits)
{
    GEDASSERT(GED_QWORD_BITS >= bits);
    if (GED_QWORD_BITS == bits) return true;
    return (0 == (val >> bits));
}


bool GEDRestrictionsHandler::CheckMaxValue(const int32_t val, const ged_field_type_modifier_t& fieldType)
{
    // variable fields are not allowed to be signed
    GEDASSERT(0);
    return false;
}


bool GEDRestrictionsHandler::CheckMaxValue(const int64_t val, const ged_field_type_modifier_t& fieldType)
{
    // variable fields are not allowed to be signed
    GEDASSERT(0);
    return false;
}


bool GEDRestrictionsHandler::CheckMaxValue(const uint32_t val, const ged_field_type_modifier_t& fieldType)
{
    return (fieldType._attr._signed) ? CheckMaxValue((int32_t)val, fieldType._attr._bits) : CheckMaxValue(val, fieldType._attr._bits);
}


bool GEDRestrictionsHandler::CheckMaxValue(const uint64_t val, const ged_field_type_modifier_t& fieldType)
{
    return (fieldType._attr._signed) ? CheckMaxValue((int64_t)val, fieldType._attr._bits) : CheckMaxValue(val, fieldType._attr._bits);
}


template<typename NumType>
bool GEDRestrictionsHandler::VerifyValueRestriction(const NumType val, const ged_field_restriction_value_t resVal)
{
    return ((NumType)(resVal) == val);
}


template<typename NumType>
bool GEDRestrictionsHandler::VerifyRangeRestriction(const NumType val, const ged_field_restriction_range_t& range)
{
    return ((NumType)(range._min) <= val && (NumType)(range._max) >= val);
}


template<typename NumType>
bool GEDRestrictionsHandler::VerifyMaskRestriction(const NumType val, const ged_field_restriction_mask_t mask)
{
    return (0 == ((NumType)mask & val));
}


template<typename NumType>
void GEDRestrictionsHandler::HandleDuplication(GEDFORASSERT(const ged_ins_field_entry_t* dataEntry) COMMA NumType& val,
                                               const ged_field_type_modifier_t::ged_field_type_attributes& fieldAttributes)
{
    if (!fieldAttributes._duplicate) return;

    // Apply value duplication.
    GEDASSERT(fieldAttributes._bits * 2 <= dataEntry->_bitSize);
    GEDASSERT(fieldAttributes._bits * 2 <= sizeof(NumType) * GED_BYTE_BITS);
    val &= BitsToMaxValue(fieldAttributes._bits);
    val |= (val << fieldAttributes._bits);
}


template<typename NumType>
bool GEDRestrictionsHandler::HandleNonEnumEncodingRestrictions(const ged_ins_field_entry_t* dataEntry, NumType& val)
{
    // TODO: Currently this is hardcoded to two, need to extend this to a given number.
    for (unsigned int i = 0; i < 2; ++i)
    {
        if (NULL == dataEntry->_restrictions[i]) return true;
        if (!HandleNonEnumEncodingRestriction(dataEntry, dataEntry->_restrictions[i], val)) return false;
    }
    return true;
}


template<typename NumType>
bool GEDRestrictionsHandler::HandleNonEnumEncodingRestriction(const ged_ins_field_entry_t* dataEntry,
                                                              const ged_field_restriction_t* restriction, NumType& val)
{
    GEDASSERT(NULL != restriction);

    switch (restriction->_restrictionType)
    {
    case GED_FIELD_RESTRICTIONS_TYPE_VALUE:
        if (!CheckMaxValue(val, dataEntry->_bitSize)) return false; // TODO: This was an assert, should switch back for optimization?
        return VerifyValueRestriction(val, restriction->_value);
    case GED_FIELD_RESTRICTIONS_TYPE_RANGE:
        if (!CheckMaxValue(val, dataEntry->_bitSize)) return false; // TODO: This was an assert, should switch back for optimization?
        return VerifyRangeRestriction(val, restriction->_range);
    case GED_FIELD_RESTRICTIONS_TYPE_MASK:
        if (!CheckMaxValue(val, dataEntry->_bitSize)) return false; // TODO: This was an assert, should switch back for optimization?
        return VerifyMaskRestriction(val, restriction->_mask);
    case GED_FIELD_RESTRICTIONS_TYPE_PADDING:
        if (!CheckMaxValue(val, dataEntry->_bitSize)) return false;
        return ((NumType)(restriction->_padding._value) == ((NumType)(restriction->_padding._mask) & val));
    case GED_FIELD_RESTRICTIONS_TYPE_FIELD_TYPE:
        GEDASSERT(GED_QWORD_BITS >= restriction->_fieldType._attr._bits);
        GEDASSERT(0 < restriction->_fieldType._attr._bits);
        if (!CheckMaxValue(val, restriction->_fieldType)) return false;
        HandleDuplication(GEDFORASSERT(dataEntry) COMMA val, restriction->_fieldType._attr);
        return true;
    case GED_FIELD_RESTRICTIONS_TYPE_ENUM:
        GEDASSERT(0);
    case GED_FIELD_RESTRICTIONS_TYPE_NONE: // if there is a non-null restriction pointer, it should be a valid restriction
        GEDASSERT(0);
    default:
        GEDASSERT(0);
    }
    return false;
}

#endif // GED_RESTRICTIONS_HANDLER_H
