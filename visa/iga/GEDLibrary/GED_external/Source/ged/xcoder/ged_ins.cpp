/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cstring>
#include <iomanip>
#include <algorithm>
#include "common/ged_string_utils.h"
#include "xcoder/ged_ins.h"

using std::setw;
using std::setfill;
using std::hex;
using std::memset;
#if GED_VALIDATION_API
using std::sort;
using std::end;
using std::next;
using std::cout;
using std::endl;
using std::dec;
using std::left;
using std::stringstream;
#endif

#if ((!defined _WIN32) && ( !defined __STDC_LIB_EXT1__ ))
#include <errno.h>
#include <string.h>
#include <stdio.h>
typedef int errno_t;
inline errno_t memcpy_s( void *dst, size_t numberOfElements, const void *src, size_t count )
{
    if( ( dst == NULL ) || ( src == NULL ) )
    {
        return EINVAL;
    }
    if( numberOfElements < count )
    {
        return ERANGE;
    }
    memcpy( dst, src, count );
    return 0;
}
#endif


/*************************************************************************************************
 * class GEDIns static data members
 *************************************************************************************************/

const int64_t GEDIns::signExtendTable[] =
{
    (int64_t)0xffffffffffffffff, // 0
    (int64_t)0xfffffffffffffffe, // 1
    (int64_t)0xfffffffffffffffc, // 2
    (int64_t)0xfffffffffffffff8, // 3
    (int64_t)0xfffffffffffffff0, // 4
    (int64_t)0xffffffffffffffe0, // 5
    (int64_t)0xffffffffffffffc0, // 6
    (int64_t)0xffffffffffffff80, // 7
    (int64_t)0xffffffffffffff00, // 8
    (int64_t)0xfffffffffffffe00, // 9
    (int64_t)0xfffffffffffffc00, // 10
    (int64_t)0xfffffffffffff800, // 11
    (int64_t)0xfffffffffffff000, // 12
    (int64_t)0xffffffffffffe000, // 13
    (int64_t)0xffffffffffffc000, // 14
    (int64_t)0xffffffffffff8000, // 15
    (int64_t)0xffffffffffff0000, // 16
    (int64_t)0xfffffffffffe0000, // 17
    (int64_t)0xfffffffffffc0000, // 18
    (int64_t)0xfffffffffff80000, // 19
    (int64_t)0xfffffffffff00000, // 20
    (int64_t)0xffffffffffe00000, // 21
    (int64_t)0xffffffffffc00000, // 22
    (int64_t)0xffffffffff800000, // 23
    (int64_t)0xffffffffff000000, // 24
    (int64_t)0xfffffffffe000000, // 25
    (int64_t)0xfffffffffc000000, // 26
    (int64_t)0xfffffffff8000000, // 27
    (int64_t)0xfffffffff0000000, // 28
    (int64_t)0xffffffffe0000000, // 29
    (int64_t)0xffffffffc0000000, // 30
    (int64_t)0xffffffff80000000, // 31
    (int64_t)0xffffffff00000000, // 32
    (int64_t)0xfffffffe00000000, // 33
    (int64_t)0xfffffffc00000000, // 34
    (int64_t)0xfffffff800000000, // 35
    (int64_t)0xfffffff000000000, // 36
    (int64_t)0xffffffe000000000, // 37
    (int64_t)0xffffffc000000000, // 38
    (int64_t)0xffffff8000000000, // 39
    (int64_t)0xffffff0000000000, // 40
    (int64_t)0xfffffe0000000000, // 41
    (int64_t)0xfffffc0000000000, // 42
    (int64_t)0xfffff80000000000, // 43
    (int64_t)0xfffff00000000000, // 44
    (int64_t)0xffffe00000000000, // 45
    (int64_t)0xffffc00000000000, // 46
    (int64_t)0xffff800000000000, // 47
    (int64_t)0xffff000000000000, // 48
    (int64_t)0xfffe000000000000, // 49
    (int64_t)0xfffc000000000000, // 50
    (int64_t)0xfff8000000000000, // 51
    (int64_t)0xfff0000000000000, // 52
    (int64_t)0xffe0000000000000, // 53
    (int64_t)0xffc0000000000000, // 54
    (int64_t)0xff80000000000000, // 55
    (int64_t)0xff00000000000000, // 56
    (int64_t)0xfe00000000000000, // 57
    (int64_t)0xfc00000000000000, // 58
    (int64_t)0xf800000000000000, // 59
    (int64_t)0xf000000000000000, // 60
    (int64_t)0xe000000000000000, // 61
    (int64_t)0xc000000000000000, // 62
    (int64_t)0x8000000000000000, // 63
};


/*************************************************************************************************
 * class GEDIns API functions
 *************************************************************************************************/

GED_RETURN_VALUE GEDIns::Init(const /* GED_MODEL */ uint8_t modelId, /* GED_OPCODE */ uint32_t opcode)
{
    GED_RETURN_VALUE ret = GED_RETURN_VALUE_SUCCESS;
    if (modelId >= numOfSupportedModels)
    {
        ret = GED_RETURN_VALUE_INVALID_MODEL;
        return ret;
    }
    _modelId = modelId;
    _opcode = invalidOpcode;
    _decodingTable = NULL;
    ret = SetOpcode(opcode);
#if defined(GED_VALIDATE)
    if (GED_RETURN_VALUE_OPCODE_NOT_SUPPORTED != ret)
    {
        GEDASSERT(invalidOpcode != _opcode);
        GEDASSERT(NULL != _decodingTable);
    }
#endif // GED_VALIDATE
    return ret;
}


GED_RETURN_VALUE GEDIns::Decode(const /* GED_MODEL */ uint8_t modelId, const unsigned char* rawBytes, const unsigned int size)
{
    if (NULL == rawBytes)
    {
        return GED_RETURN_VALUE_NULL_POINTER;
    }
    if (modelId >= numOfSupportedModels)
    {
        return GED_RETURN_VALUE_INVALID_MODEL;
    }
    if (size < GED_COMPACT_INS_SIZE)
    {
        // Buffer isn't large enough to extract opcode and compact bit.
        return GED_RETURN_VALUE_BUFFER_TOO_SHORT;
    }
    ClearStatus();
    _modelId = modelId;
    ExtractOpcode(rawBytes);
    _decodingTable = GetCurrentModelData().opcodeTables[_opcode].nativeDecoding;
    if (NULL == _decodingTable)
    {
        return GED_RETURN_VALUE_OPCODE_NOT_SUPPORTED;
    }
    ExtractCmptCtrl(rawBytes);
    if (!IsCompactValid())
    {
        if (size < GED_NATIVE_INS_SIZE)
        {
            // Buffer isn't large enough for the given Native instruction.
            return GED_RETURN_VALUE_BUFFER_TOO_SHORT;
        }
    }

    GED_RETURN_VALUE ret = GED_RETURN_VALUE_SUCCESS;
    if (IsCompactValid())
    {
        SetInstructionBytes(_compactBytes, rawBytes, size, GED_COMPACT_INS_SIZE);
        ret = BuildNativeInsFromCompact(); // should always succeed, however this is not true, see Mantis 3755
        if (GED_RETURN_VALUE_SUCCESS != ret) return ret;

        // The API function IsCompact() checks the GED_INS_STATUS_COMPACT_ENCODED bit, so we must enforce the encoding restrictions
        // and set the instruction's compact form as encoded.
        ApplyCompactEncodingMasks(_compactBytes); // enforce the per-instruction encoding restrictions for the compact format

#if defined (GED_VALIDATE)
        // TODO: Save the current instruction bytes, then call BuildNativeInsFromCompact again and compare the two
        //       to prevent field/padding conflicts in the XMLs.
#endif // GED_VALIDATE
    }
    else
    {
        SetInstructionBytes(_nativeBytes, rawBytes, size, GED_NATIVE_INS_SIZE);
        SetNativeValid();

        // The API function IsModified() checks the GED_INS_STATUS_NATIVE_ENCODED bit, so we must enforce the encoding restrictions
        // and set the instruction's native format as encoded.
        ApplyNativeEncodingMasks(); // enforce the per-instruction encoding restrictions for the native format
    }
    GEDASSERT(IsNativeValid());
    return ret;
}


GED_RETURN_VALUE GEDIns::Encode(const GED_INS_TYPE insType, unsigned char* rawBytes)
{
    GED_RETURN_VALUE ret = GED_RETURN_VALUE_SUCCESS;
    if (GED_INS_TYPE_COMPACT == insType) // retrieve the compact instruction bytes
    {
        if (!IsCompactEncoded())
        {
            if (!IsCompactValid())
            {
                GEDASSERT(IsNativeValid());
                // Try to build a compact format for this instruction.
                if (!BuildCompactInsFromNative())
                {
                    GEDASSERT(!IsCompactValid());
                    ret = GED_RETURN_VALUE_NO_COMPACT_FORM;
                    return ret; // the instruction does not have a compact format in its current state
                }
                GEDASSERT(IsCompactValid());
            }
            ApplyCompactEncodingMasks(_compactBytes); // enforce the per-instruction encoding restrictions for the compact format
        }
        GEDASSERT(IsCompactValid());
        if (NULL != rawBytes)
        {
            memcpy_s(rawBytes, GED_COMPACT_INS_SIZE, _compactBytes, GED_COMPACT_INS_SIZE); // copy the compact instruction bytes
        }
    }
    else // retrieve the native instruction bytes
    {
        GEDASSERT(GED_INS_TYPE_NATIVE == insType);
        if (!IsNativeEncoded())
        {
            if (!IsNativeValid())
            {
                GEDASSERT(IsCompactValid());
                BuildNativeInsFromCompact(); // should always succeed, however this is not true, see Mantis 3755
                GEDASSERT(IsNativeValid());
            }
            ApplyNativeEncodingMasks(); // enforce the per-instruction encoding restrictions for the native format
        }
        GEDASSERT(IsNativeValid());
        if (NULL != rawBytes)
        {
            memcpy_s(rawBytes, GED_NATIVE_INS_SIZE, _nativeBytes, GED_NATIVE_INS_SIZE); // copy the native instruction bytes
        }
    }
    return ret;
}


#if GED_VALIDATION_API

GED_RETURN_VALUE GEDIns::CountCompacted(unsigned int& count)
{
    if (!IsNativeEncoded())
    {
        if (!IsNativeValid())
        {
            // Can only use the native format as a reference, so first uncompact the instruction
            GEDASSERT(IsCompactValid());
            BuildNativeInsFromCompact(); // Should always succeed, however this is not true, see Mantis 3755
            GEDASSERT(IsNativeValid());
        }
        ApplyNativeEncodingMasks(); // enforce the per-instruction encoding restrictions for the native format
    }
    GEDASSERT(IsNativeValid());
    if (!CountCompactFormats(count))
    {
        GEDASSERT(!IsCompactValid());
        return GED_RETURN_VALUE_NO_COMPACT_FORM; // the instruction does not have a compact form in its current state
    }
    return GED_RETURN_VALUE_SUCCESS;
}


GED_RETURN_VALUE GEDIns::RetrieveAllCompactedFormats(const unsigned int size, unsigned char* compactBytesArray)
{
    GEDFORASSERT(unsigned int count = 0);
    GEDASSERT(GED_RETURN_VALUE_SUCCESS == CountCompacted(count));
    GEDASSERT((size / GED_COMPACT_INS_SIZE) <= count);
    if (NULL == compactBytesArray)
    {
        return GED_RETURN_VALUE_NULL_POINTER;
    }
    if (!IsNativeEncoded())
    {
        if (!IsNativeValid())
        {
            // Can only use the native format as a reference, so first uncompact the instruction
            GEDASSERT(IsCompactValid());
            BuildNativeInsFromCompact(); // should always succeed, however this is not true, see Mantis 3755
            GEDASSERT(IsNativeValid());
        }
        ApplyNativeEncodingMasks(); // enforce the per-instruction encoding restrictions for the native format
    }
    GEDASSERT(IsNativeValid());
    if (!BuildAllCompactedFormats(compactBytesArray, size))
    {
        GEDASSERT(!IsCompactValid());
        return GED_RETURN_VALUE_NO_COMPACT_FORM; // the instruction does not have a compact form in its current state
    }
    unsigned char* compactBytesPtr = compactBytesArray;
    for (unsigned int i = 0; i < size / GED_COMPACT_INS_SIZE; ++i)
    {
        ApplyCompactEncodingMasks(compactBytesPtr);
        compactBytesPtr += GED_COMPACT_INS_SIZE;
    }
    return GED_RETURN_VALUE_SUCCESS;
}


GED_RETURN_VALUE GEDIns::PrintFieldBitLocation(const /* GED_INS_FIELD */ uint32_t field) const
{
    GEDASSERT(field < GetCurrentModelData().numberOfInstructionFields);
    GED_RETURN_VALUE status = GED_RETURN_VALUE_INVALID_FIELD;
    ged_ins_decoding_table_t table = _decodingTable;
    const unsigned char* bytes = _nativeBytes;
    if (IsCompactValid())
    {
        table = GetCurrentModelData().opcodeTables[_opcode].compactDecoding;
         bytes = _compactBytes;
    }
    const string& fieldName = fieldNameByField[field];
    const ged_ins_field_entry_t* dataEntry = GetInstructionDataEntry(table, field);
    if (NULL == dataEntry)
    {
        cout << "Field " << fieldName << " is invalid for the current instruction." << endl;
        return status;
    }

    vector<ged_ins_field_mapping_fragment_t> mappingFragments;
    bool hasFixedValue = false;

    // Record padding, if exists.
    hasFixedValue |= RecordPadding(mappingFragments, dataEntry);

    // Record the explicit position fragments, if exists.
    hasFixedValue |= RecordPosition(mappingFragments, dataEntry);

    // Sort the vector by the from.lowBit
    sort(mappingFragments.begin(), mappingFragments.end());

    // Merge fragments which are consecutive by both from and to values.
    MergeFragments(mappingFragments);

    // Print the bit mapping of the field.
    const uint32_t maxBitPosition = (uint32_t)mappingFragments.back()._from._highBit;
    const uint32_t maxBitPositionLength = static_cast<uint32_t>(DecStr(maxBitPosition).length());
    const uint32_t bitSpacing = maxBitPositionLength * 2 + 1; // printing 2 bit positions, plus a colon
    const uint64_t rawValue = GetField<uint64_t>(bytes, table, field, GED_VALUE_TYPE_ENCODED, status);
    static const string implicitString = "Implicit";
    GEDASSERT(GED_RETURN_VALUE_SUCCESS == status);
    cout << "Field " << fieldName << ", width: " << (uint32_t)dataEntry->_bitSize <<
            ((uint32_t)dataEntry->_bitSize == 1 ? " bit, " : " bits, ") << "raw value: " <<
            HexStr(rawValue) << " (" << BinStr(rawValue, dataEntry->_bitSize) << "b)" << endl;
    for (const auto& it : mappingFragments)
    {
        const size_t fragmentBitLength = it._from._highBit - it._from._lowBit + 1;
        stringstream strm;
        strm << (uint32_t)it._from._lowBit << ":" << (uint32_t)it._from._highBit;
        cout << left << setfill(' ') << setw(bitSpacing) << strm.str() << " = ";
        if (it._fixed)
            cout << implicitString << " (" << BinStr(it._value, fragmentBitLength) << "b)" << endl;
        else
        {
            strm.str(string()); // clear stream
            strm.clear();
            uint64_t mask = 0;
            for (uint8_t i = it._from._lowBit; i <= it._from._highBit; ++i)
            {
                mask |= (1ULL << i);
            }
            const uint64_t fragmentVal = ((rawValue & mask) >> it._from._lowBit);
            strm << (uint32_t)it._to._lowBit << ":" << (uint32_t)it._to._highBit;
            cout << left << setfill(' ') << setw(hasFixedValue ? implicitString.length() : bitSpacing) << strm.str() << " (" <<
                    BinStr(fragmentVal, fragmentBitLength) << "b)" << endl;
        }
    }
    return GED_RETURN_VALUE_SUCCESS;
}
#endif //GED_VALIDATION_API

GED_RETURN_VALUE GEDIns::QueryFieldBitLocation(const /* GED_INS_FIELD */ uint32_t field, uint32_t *fragments, uint32_t *length) const
{
    GEDASSERT(field < GetCurrentModelData().numberOfInstructionFields);
    GEDASSERT((fragments != NULL) || (length != NULL));
    GED_RETURN_VALUE status = GED_RETURN_VALUE_INVALID_FIELD;
    ged_ins_decoding_table_t table = _decodingTable;

    if (length != NULL)
    {
        *length = 0;
    }
    const ged_ins_field_entry_t* dataEntry = GetInstructionDataEntry(table, field);
    if (NULL == dataEntry)
    {
        return status;
    }

    vector<ged_ins_field_mapping_fragment_t> mappingFragments;
    RecordPosition(mappingFragments, dataEntry);
    sort(mappingFragments.begin(), mappingFragments.end());
    MergeFragments(mappingFragments);

    uint32_t index = 0;
    for (const auto& it : mappingFragments)
    {
        if (it._fixed)
            ;
        else
        {
            if (fragments != NULL)
            {
                uint32_t len = it._from._highBit - it._from._lowBit + 1;
                fragments[index++] = (len << 16) | it._to._lowBit;
            }
            if (length != NULL)
            {
                (*length)++;
            }
        }
    }
    return GED_RETURN_VALUE_SUCCESS;
}

uint32_t GEDIns::GetFieldSize(const /* GED_INS_FIELD */ uint32_t field) const
{
    if (GetCurrentModelData().numberOfInstructionFields <= field) return 0;

    // First try the native format.
    const ged_ins_field_entry_t* dataEntry = GetInstructionDataEntry(_decodingTable, field);
    if (NULL != dataEntry) return (uint32_t)dataEntry->_bitSize;

    // The field was not found in the native format, try the compact format (if it exists).
    const ged_ins_decoding_table_t compactTable = GetCurrentModelData().opcodeTables[_opcode].compactDecoding;
    if (NULL != compactTable)
    {
        const ged_ins_field_entry_t* compactDataEntry = GetInstructionDataEntry(compactTable, field);
        if (NULL != compactDataEntry) return (uint32_t)compactDataEntry->_bitSize;
    }

    // The field is not valid in either format.
    return 0;
}


GED_RETURN_VALUE GEDIns::SetOpcode(/* GED_OPCODE */ uint32_t opcode)
{
    GED_RETURN_VALUE ret = GED_RETURN_VALUE_SUCCESS;
    const uint32_t highestOpcodeValue = GED_MAX_ENTRIES_IN_OPCODE_TABLE - 1;
    if (!GEDRestrictionsHandler::ConvertEnumeratedValueToRawEncodedValue(opcode, highestOpcodeValue, GetCurrentModelData().Opcodes))
    {
        ret = GED_RETURN_VALUE_OPCODE_NOT_SUPPORTED;
        return ret;
    }

    if (opcode == _opcode)
    {
        return ret;
    }
    _opcode = opcode;

    const ged_ins_decoding_table_t decodingTable = GetCurrentModelData().opcodeTables[opcode].nativeDecoding;
    GEDASSERT(NULL != decodingTable);
    if (decodingTable != _decodingTable)
    {
        ClearStatus();
        memset(_nativeBytes, 0, GED_NATIVE_INS_SIZE);

        // Set the new decoding table.
        _decodingTable = decodingTable;
        SetNativeValid(); // mark that the native instruction bytes are valid
        SetNativeOpcode(); // set the opcode in the native instruction bytes

        // Check if the new opcode has a compact format.
        if (GetCurrentModelData().opcodeTables[_opcode].compactDecoding)
        {
            // TODO: Once the compact bit is set in the encoding masks, remove the next line and uncomment the one after that (Mantis 3732).
            *(reinterpret_cast<uint64_t*>(_compactBytes)) = compactInsInitializer;
            //memset(_compactBytes, 0, GED_COMPACT_INS_SIZE);
            SetCompactValid(); // mark that the compact instruction bytes are valid
            SetCompactOpcode(); // set the opcode in the compact instruction bytes
        }
    }
    GEDASSERT(IsValid()); // at least one format must be valid
    return ret;
}

# if GED_EXPERIMENTAL
GED_RETURN_VALUE GEDIns::SetRawBits(const uint8_t lowBit, const uint8_t highBit, const uint64_t val)
{
    GEDASSERT(lowBit < GED_NATIVE_INS_SIZE_BITS);
    GEDASSERT(highBit < GED_NATIVE_INS_SIZE_BITS);
    GEDASSERT(lowBit <= highBit);
    GEDASSERT(IsNativeValid());
    const uint8_t lowDword = lowBit / GED_DWORD_BITS;
    const uint8_t highDword = highBit / GED_DWORD_BITS;
    //const unsigned int numberOfDwords = highDword - lowDword;
    ged_ins_field_position_fragment_t pos;
    if (lowDword == highDword)
    {
        pos._lowBit = lowBit;
        pos._highBit = highBit;
        pos._dwordIndex = lowBit / GED_DWORD_BITS;
        pos._shift = lowBit % GED_DWORD_BITS;
        pos._bitMask = rightShiftedMasks[(highBit - lowBit)] << pos._shift;
        SetFragment(_nativeBytes, pos, val);
    }
    else
    {
        NYI;
    }
    SetEncodingMasksDisabled();
    return GED_RETURN_VALUE_SUCCESS;
}
# endif // GED_EXPERIMENTAL

/*************************************************************************************************
 * class GEDIns protected member functions
 *************************************************************************************************/

uint8_t GEDIns::GetFieldWidth(const uint16_t field, const bool interpField /* = false */) const
{
    const ged_ins_field_entry_t* entry =
        GetInstructionDataEntry((interpField) ? GetCurrentModelData().pseudoFields : _decodingTable, field);
    if (NULL == entry) return 0;
    if (IsVariableField(entry))
    {
        return entry->_restrictions[0]->_fieldType._attr._bits;
    }
    else
    {
        return entry->_bitSize;
    }
}


string GEDIns::GetInstructionBytes() const
{
    return (IsCompact()) ? GetInstructionBytes(_compactBytes, GED_NUM_OF_COMPACT_INS_DWORDS)
                         : GetInstructionBytes(_nativeBytes, GED_NUM_OF_NATIVE_INS_DWORDS);
}


/*************************************************************************************************
 * class GEDIns private member functions
 *************************************************************************************************/

void GEDIns::SetInstructionBytes(unsigned char* dst, const unsigned char* src, unsigned int size, const unsigned int maxSize) const
{
    if (size > maxSize)
    {
        size = maxSize; // given buffer is too large, ignore the rest of the bytes.
    }
    memcpy_s(dst, size, src, size);
}


uint32_t GEDIns::GetMappedField(const /* GED_INS_FIELD */ uint32_t field, const unsigned char* validBits, bool& extracted) const
{
    GEDASSERT(NULL != validBits);
    GEDASSERT(field < GetCurrentModelData().numberOfInstructionFields);
    GEDASSERT(field == _decodingTable[field]._field);

    // Traverse the intermediate tables (if necessary).
    extracted = false;
    const ged_ins_field_entry_t* dataEntry = GetMappedInstructionDataEntry(_decodingTable, field, validBits, extracted);
    if (!extracted) return MAX_UINT32_T;
    GEDASSERT(NULL != dataEntry);

    // Now that we are at the bottommost table, extract the actual data if they were already mapped. The "extracted" out parameter is
    // already set to TRUE.
    switch (dataEntry->_entryType)
    {
    case GED_TABLE_ENTRY_TYPE_CONSECUTIVE:
        if (0 != ExtractConsecutiveEntryValue(validBits, dataEntry->_consecutive._position)) break;
        return ExtractConsecutiveEntryValue(_nativeBytes, dataEntry->_consecutive._position);
    case GED_TABLE_ENTRY_TYPE_FRAGMENTED:
        if (0 != ExtractFragmentedEntryValue<uint32_t>(validBits, dataEntry)) break;
        return ExtractFragmentedEntryValue<uint32_t>(_nativeBytes, dataEntry);
    case GED_TABLE_ENTRY_TYPE_FIXED_VALUE:
        return dataEntry->_fixed._value;
    default:
        GEDASSERT(0);
    }

    // Mark that some (or all) necessary bits for this field were not yet mapped.
    extracted = false;
    return MAX_UINT32_T;
}


const ged_ins_field_entry_t* GEDIns::GetDependentInstructionDataEntry(ged_ins_decoding_table_t table,
                                                                      /* GED_INS_FIELD */ uint32_t tableIndex) const
{
    GEDASSERT(NULL != table);
    while (GED_TABLE_ENTRY_TYPE_NEXT_TABLE == table[tableIndex]._entryType)
    {
        const ged_ins_field_next_table_t* nextTable = &table[tableIndex]._nextTable;
        GED_RETURN_VALUE ret = GED_RETURN_VALUE_INVALID_FIELD;
        tableIndex = GetField<uint32_t>(_nativeBytes, _decodingTable, nextTable->_tableKey, GED_VALUE_TYPE_ENCODED, ret);

        // GetField is expected to succeed since the models ensure that the dependent fields are always valid. However, we need to
        // account for invalid instructions, i.e. instructions with an illegal encoding. Such instructions may have invalid values for
        // valid fields. If a dependee field is in itself a dependent field, it may even be an invalid field in the given (poorly
        // encoded) instruction.
        if (GED_RETURN_VALUE_INVALID_FIELD == ret || GED_RETURN_VALUE_INVALID_VALUE == ret) return NULL;
        GEDASSERT(GED_RETURN_VALUE_SUCCESS == ret);
        table = nextTable->_tablePtr;
        GEDASSERT(NULL != table);
    }

    // Verify that this is a legal entry.
    GEDASSERT(table[tableIndex]._entryType < GED_TABLE_ENTRY_TYPE_SIZE);

    // Verify that the field is supported in this format.
    if (GED_TABLE_ENTRY_TYPE_NOT_SUPPORTED == table[tableIndex]._entryType)
    {
        return NULL;
    }

    // Verify that the final entry actually holds data.
    GEDASSERT(GED_TABLE_ENTRY_TYPE_NEXT_TABLE != table[tableIndex]._entryType);

    return &table[tableIndex];
}


const ged_ins_field_entry_t* GEDIns::GetDependentMappedInstructionDataEntry(ged_ins_decoding_table_t table,
                                                                            /* GED_INS_FIELD */ uint32_t tableIndex,
                                                                            const unsigned char* validBits, bool& extracted) const
{
    GEDASSERT(NULL != table);
    GEDASSERT(NULL != validBits);
    while (GED_TABLE_ENTRY_TYPE_NEXT_TABLE == table[tableIndex]._entryType)
    {
        // Extract the value from the _nativeBytes array if it is valid (i.e. if it has already been mapped)
        const ged_ins_field_next_table_t* nextTable = &table[tableIndex]._nextTable;
        tableIndex = GetMappedField(nextTable->_tableKey, validBits, extracted);
        if (!extracted) return NULL; // the field is not valid in the _nativeBytes array (i.e. it was not yet mapped)
        GEDASSERT(MAX_UINT32_T != tableIndex);
        table = nextTable->_tablePtr;
        GEDASSERT(NULL != table);
    }

    // Verify that this is a legal entry.
    GEDASSERT(table[tableIndex]._entryType < GED_TABLE_ENTRY_TYPE_SIZE);

    // Verify that the field is supported in this format.
    GEDASSERT(GED_TABLE_ENTRY_TYPE_NOT_SUPPORTED != table[tableIndex]._entryType);

    // Verify that the final entry actually holds data.
    GEDASSERT(GED_TABLE_ENTRY_TYPE_NEXT_TABLE != table[tableIndex]._entryType);

    extracted = true;
    return &table[tableIndex];
}


GED_RETURN_VALUE GEDIns::BuildNativeInsFromCompact()
{
    GEDASSERT(IsCompactValid()); // nothing to do if the instruction is not compacted
    memset(_nativeBytes, 0, GED_NATIVE_INS_SIZE);

    const ged_ins_decoding_table_t compactTable = GetCurrentModelData().opcodeTables[_opcode].compactDecoding;  // map from this
    if (NULL == compactTable) return GED_RETURN_VALUE_NO_COMPACT_FORM;

    const ged_compact_mapping_table_t mappingTable = GetCurrentModelData().opcodeTables[_opcode].compactMapping;
    GEDASSERT(NULL != mappingTable);

    //  Mapping algorithm:
    //
    //  Iterate compactTable. For each valid entry i.e. GED_TABLE_ENTRY_TYPE_CONSECUTIVE, GED_TABLE_ENTRY_TYPE_FRAGMENTED
    //  or GED_TABLE_ENTRY_TYPE_NEXT_TABLE:
    //      a) Get compact field value (from _compactBytes using compactTable). If the field depends on another field which was not
    //           yet mapped, add it to the unmapped list and continue to the next field.
    //      b) Find mapping type:
    //          One of the four combinations between value/index and consecutive/fragmented. The mapping type may have dependencies,
    //          in which case there will be a next table entry and finding the mapping type will require iterating the mapping chain.
    //          The mapping type may be GED_MAPPING_TABLE_ENTRY_TYPE_NOT_SUPPORTED if the field is not supported in this format, in
    //          which case the corresponding entry in the decoding table will be of type GED_TABLE_ENTRY_TYPE_NOT_SUPPORTED as well.
    //      c) If this is a value mapping (either consecutive or fragmented):
    //          Map bits from _compactBytes to _nativeBytes according to the mapping table entry.
    //      d) If this is an index mapping, i.e. mapping from a compaction table:
    //          i)  Get the value from the compaction table (ged_compaction_table_t), in which case the field's value serves
    //              as the index of the entry in the compaction table.
    //          ii) Map the bits from the compaction table entry value to _nativeBytes.
    //      e) Repeat steps (a)-(d) for every field in the unmapped list until it is empty.

    // The validBits array will be used to note which bits of the instruction's native format were already mapped (and can be used as
    // dependencies). We start with all bits set and clear the relevant bits as they are being mapped. Thus, in order to check if a
    // certain field was mapped or not, the relevant bits are simply compared to zero.
    unsigned char validBits[GED_NATIVE_INS_SIZE];
    memset(validBits, 0xff, GED_NATIVE_INS_SIZE);
    set<uint32_t> unMapped;

    for (uint32_t i = 0; i < GetCurrentModelData().numberOfInstructionFields; ++i)
    {
        GEDASSERT(compactTable[i]._entryType < GED_TABLE_ENTRY_TYPE_SIZE); // verify that this is a valid entry
        if (GED_TABLE_ENTRY_TYPE_NOT_SUPPORTED == compactTable[i]._entryType) continue; // this field is not supported in this format
        if (!MapCurrentField(compactTable, mappingTable, i, validBits))
        {
            // The field depends on another field which was not yet mapped. Add it to the unmapped list for later processing.
            GEDASSERT(0 == unMapped.count(i));
            unMapped.insert(i);
            continue;
        }
    }

    // In every iteration of the outer loop, try to map the first unmapped (dependent) field in the unMapped set. If it depends on
    // another dependent field which was not yet mapped, skip it and try the rest one by one (the inner loop) until one field is
    // mapped successfully. If no field was mapped successfully, it means that the remaining fields have a cyclic dependency on each
    // other, in which case an error is emitted.
    const set<uint32_t>::const_iterator end = unMapped.end();
    while (!unMapped.empty())
    {
        set<uint32_t>::const_iterator it = unMapped.begin();
        for (; end != it; ++it)
        {
            if (MapCurrentField(compactTable, mappingTable, *it, validBits)) break;
        }

        // Make sure a dependent field was mapped successfully.
        if (end == it)
        {
            // TODO: Call this function after adding logging to GED.
            // EmitMappingCyclicDependencyError(unMapped, validBits);
            return GED_RETURN_VALUE_BAD_COMPACT_ENCODING;
        }
        unMapped.erase(it);
    }

    // This instruction is marked as compact in the _status field, but the instruction bits in the _nativeBytes array should reflect
    // a native instruction now that it is expanded.
    SetNonCompact();
    SetNativeValid();
    return GED_RETURN_VALUE_SUCCESS;
}


bool GEDIns::MapCurrentField(const ged_ins_decoding_table_t compactTable, const ged_compact_mapping_table_t mappingTable,
                             const /* GED_INS_FIELD */ uint32_t field, unsigned char* validBits)
{
    GEDASSERT(NULL != compactTable);
    GEDASSERT(NULL != mappingTable);
    GEDASSERT(NULL != validBits);

    // Get the mapping table entry for this field.
    const ged_compact_mapping_entry_t* mappingEntry = GetCompactionMappingEntry(mappingTable, field, validBits);
    if (NULL == mappingEntry) return false; // when building compact from native but the mapping is invalid
    if (GED_MAPPING_TABLE_ENTRY_TYPE_NO_MAPPING == mappingEntry->_entryType) return true; // early out
    GEDASSERT(mappingEntry->_field == field);

    // Get the compact field value.
    GED_RETURN_VALUE ret = GED_RETURN_VALUE_INVALID_FIELD;
    uint32_t mappingValue = GetField<uint32_t>(_compactBytes, compactTable, field, GED_VALUE_TYPE_ENCODED, ret);
    GEDASSERT(GED_RETURN_VALUE_SUCCESS == ret);

    // Map the compact field to the native field(s).
    switch (mappingEntry->_entryType)
    {
    case GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_CONSECUTIVE:
        GEDASSERT(NULL != mappingEntry->_compactionTable);
        GEDASSERT(MAX_UINT32_T >= mappingEntry->_compactionTable[mappingValue]);
        MapRawBytes((uint32_t)mappingEntry->_compactionTable[mappingValue], &mappingEntry->_consecutive._to,
                    mappingEntry->_consecutive._fromMask, validBits);
        break;
    case GED_MAPPING_TABLE_ENTRY_TYPE_VALUE_MAPPING_CONSECUTIVE:
        MapRawBytes(mappingValue, &mappingEntry->_consecutive._to, mappingEntry->_consecutive._fromMask, validBits);
        break;
    case GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_FRAGMENTED:
        GEDASSERT(NULL != mappingEntry->_compactionTable);
        MapRawBytes(mappingEntry->_compactionTable[mappingValue], mappingEntry->_fragmented._numOfMappingFragments,
                    mappingEntry->_fragmented._fragments, validBits);
        break;
    case GED_MAPPING_TABLE_ENTRY_TYPE_VALUE_MAPPING_FRAGMENTED:
        MapRawBytes(mappingValue, mappingEntry->_fragmented._numOfMappingFragments, mappingEntry->_fragmented._fragments, validBits);
        break;
    default:
        GEDASSERT(0);
    }
    return true;
}


const ged_compact_mapping_entry_t* GEDIns::GetDependentCompactionMappingEntry(ged_compact_mapping_table_t table,
                                                                              /* GED_INS_FIELD */ uint32_t tableIndex,
                                                                              const unsigned char* validBits) const
{
    GEDASSERT(NULL != table);
    GEDASSERT(NULL != validBits);
    while (GED_MAPPING_TABLE_ENTRY_TYPE_NEXT_TABLE == table[tableIndex]._entryType)
    {
        const ged_compact_mapping_next_table_t* nextTable = &table[tableIndex]._nextTable;
        // Extract the value from the _nativeBytes array if it is valid (i.e. if it has already been mapped)
        bool extracted = false;
        tableIndex = GetMappedField(nextTable->_tableKey, validBits, extracted);
        if (!extracted) return NULL; // the field is not valid in the _nativeBytes array (i.e. it was not yet mapped)
        table = nextTable->_tablePtr;
        GEDASSERT(NULL != table);
    }

    // Verify that this is a legal entry.
    GEDASSERT(table[tableIndex]._entryType < GED_MAPPING_TABLE_ENTRY_TYPE_SIZE);

    // Verify that the field is supported in this format.
    if (GED_MAPPING_TABLE_ENTRY_TYPE_NOT_SUPPORTED == table[tableIndex]._entryType) return NULL;

    // Verify that the final entry actually holds data.
    GEDASSERT(GED_MAPPING_TABLE_ENTRY_TYPE_NEXT_TABLE != table[tableIndex]._entryType);

    return &table[tableIndex];
}


void GEDIns::MapRawBytes(const uint32_t src, const ged_ins_field_position_fragment_t* to, const uint32_t fromMask,
                         unsigned char* validBits)
{
    GEDASSERT(NULL != to);
    GEDASSERT(NULL != validBits);
    uint32_t value = (src & fromMask);
    GEDASSERT(0 <= to->_shift);
    value <<= to->_shift;
    SetMappedBits(to->_dwordIndex, to->_bitMask, value, validBits);
}


void GEDIns::MapRawBytes(const uint32_t src, const uint32_t numOfFragments, const ged_compact_mapping_fragment_t* fragments,
                         unsigned char* validBits)
{
    GEDASSERT(NULL != fragments);
    GEDASSERT(NULL != validBits);
    for (uint32_t i = 0; i < numOfFragments; ++i)
    {
        if (GED_COMPACT_MAPPING_TYPE_REP == fragments[i]._mappingType)
        {
            GEDASSERT(0 == fragments[i]._from._dwordIndex); // src only has 32 bits
            MapReppedValue(src, &fragments[i]._to, &fragments[i]._from, validBits);
        }
        else if (GED_COMPACT_MAPPING_TYPE_1x1 == fragments[i]._mappingType)
        {
            GEDASSERT(0 == fragments[i]._from._dwordIndex); // src only has 32 bits
            MapOneToOneValue(src, &fragments[i]._to, &fragments[i]._from, validBits);
        }
        else if (GED_COMPACT_MAPPING_TYPE_FIXED == fragments[i]._mappingType)
        {
            GEDASSERT(0 == fragments[i]._from._dwordIndex); // src only has 32 bits
            MapFixedValue(fragments[i]._from._lowBit, &fragments[i]._to, validBits);
        }
        else { GEDASSERT(0); }
    }
}


void GEDIns::MapRawBytes(const uint64_t src, const uint32_t numOfFragments, const ged_compact_mapping_fragment_t* fragments,
                         unsigned char* validBits)
{
    GEDASSERT(NULL != fragments);
    GEDASSERT(NULL != validBits);
    for (uint32_t i = 0; i < numOfFragments; ++i)
    {
        if (GED_COMPACT_MAPPING_TYPE_REP == fragments[i]._mappingType)
        {
            GEDASSERT(fragments[i]._from._dwordIndex < 2); // src only has 64 bits
            MapReppedValue((0 == fragments[i]._from._dwordIndex) ? (uint32_t)src : (uint32_t)(src >> GED_DWORD_BITS),
                           &fragments[i]._to, &fragments[i]._from, validBits);
        }
        else if (GED_COMPACT_MAPPING_TYPE_1x1 == fragments[i]._mappingType)
        {
            GEDASSERT(fragments[i]._from._dwordIndex < 2); // src only has 64 bits
            MapOneToOneValue((0 == fragments[i]._from._dwordIndex) ? (uint32_t)src : (uint32_t)(src >> GED_DWORD_BITS),
                             &fragments[i]._to, &fragments[i]._from, validBits);
        }
        else if (GED_COMPACT_MAPPING_TYPE_FIXED == fragments[i]._mappingType)
        {
            NYI; // no fixed mapping for fragmented index mapping
        }
        else { GEDASSERT(0); }
    }
}


void GEDIns::MapReppedValue(const uint32_t src, const ged_ins_field_position_fragment_t* to,
                            const ged_ins_field_position_fragment_t* from, unsigned char* validBits)
{
    GEDASSERT(NULL != to);
    GEDASSERT(NULL != from);
    const uint8_t fromSize = FragmentSize(from);
    const uint8_t toSize = FragmentSize(to);
    GEDASSERT(fromSize > 0);               // verify that the source size is non-zero (about to be used as a denominator)
    GEDASSERT(toSize >= fromSize);         // verify that the target is at least as wide as the source
    GEDASSERT(0 == (toSize % fromSize));   // verify that the target is a whole multiple of the source
    const uint8_t numOfReps = toSize / fromSize;
    const uint32_t rep = ((src & from->_bitMask) >> from->_shift);
    uint32_t value = rep;
    for (uint8_t i = 1; i < numOfReps; ++i)
    {
        value <<= fromSize;
        value |= rep;
    }
    value <<= to->_shift;
    SetMappedBits(to->_dwordIndex, to->_bitMask, value, validBits);
}


void GEDIns::MapOneToOneValue(const uint32_t src, const ged_ins_field_position_fragment_t* to,
                              const ged_ins_field_position_fragment_t* from, unsigned char* validBits)
{
    GEDASSERT(NULL != to);
    GEDASSERT(NULL != from);
    uint32_t value = (src & from->_bitMask);
    const int8_t shift = from->_shift - to->_shift;
    if (shift > 0)
    {
        value >>= shift;
    }
    else
    {
        value <<= abs(shift);
    }
    SetMappedBits(to->_dwordIndex, to->_bitMask, value, validBits);
}


void GEDIns::MapFixedValue(const uint32_t value, const ged_ins_field_position_fragment_t* to, unsigned char* validBits)
{
    GEDASSERT(NULL != to);
    SetMappedBits(to->_dwordIndex, to->_bitMask, value << to->_shift, validBits);
}


void GEDIns::EmitMappingCyclicDependencyError(const set<uint32_t>& unMapped, const unsigned char* validBits) const
{
    GEDASSERT(NULL != validBits);
    set<uint32_t>::const_iterator it = unMapped.begin();
    const set<uint32_t>::const_iterator end = unMapped.end();
    string depErrorStr = DecStr(*it);
    for (++it; end != it; ++it)
    {
        depErrorStr += ", " + DecStr(*it);
    }
#if defined(GED_VALIDATE)
    stringstream strm;
    strm << setfill('0') << hex;
    for (int i = GED_NUM_OF_NATIVE_INS_DWORDS - 1; i >= 0; --i)
    {
        strm << setw(8) << ((uint32_t*)validBits)[i];
    }
    depErrorStr += "\nValidBits: 0x" + strm.str();
#endif // GED_VALIDATE
    // TODO: This should use GED logs instead of emitting an error. Also, consider changing the function name.
    GEDERROR("Unable to map remaining unmapped fields, probably due to an implicit dependency cycle: " + depErrorStr);
}


bool GEDIns::BuildCompactInsFromNative()
{
    GEDASSERT(IsNativeValid());
    GEDASSERT(!IsCompactValid());
    GEDASSERT(!IsCompactEncoded());
    memset(_compactBytes, 0, GED_COMPACT_INS_SIZE); // clear the compact bytes

    const ged_ins_decoding_table_t compactTable = GetCurrentModelData().opcodeTables[_opcode].compactDecoding;  // map to this
    if (NULL == compactTable) return false; // the instruction doesn't have a compact format

    const ged_compact_mapping_table_t mappingTable = GetCurrentModelData().opcodeTables[_opcode].compactMapping;
    GEDASSERT(NULL != mappingTable);

    // Traverse all the compact instruction fields. For every valid field, collect its value from the native instruction and set that
    // value in the compact instruction bytes.
    unsigned char orMask[GED_NATIVE_INS_SIZE] = { 0 }; // used in CollectCurrentField, see documentation therein for its usage
    BuildNativeOrMask(orMask); // prepare the or-mask
    for (uint32_t i = 0; i < GetCurrentModelData().numberOfInstructionFields; ++i)
    {
        GEDASSERT(compactTable[i]._entryType < GED_TABLE_ENTRY_TYPE_SIZE); // verify that this is a valid entry
        if (GED_TABLE_ENTRY_TYPE_NOT_SUPPORTED == compactTable[i]._entryType) continue; // this field is not supported in this format
        if (!CollectCurrentField(compactTable, mappingTable, orMask, i)) return false;
    }

    // Set the CmptCtrl bit - is was cleared by the loop above when reverse-mapping the CmptCtrl field from the native instruction.
    SetCompact(_compactBytes);
    SetCompactValid(); // the compact format is now valid
    return true;
}


#if GED_VALIDATION_API
bool GEDIns::CountCompactFormats(unsigned int& count)
{
    unsigned int tempCounter = 1;
    unsigned char orMask[GED_NATIVE_INS_SIZE] = { 0 }; // used in CountCurrentField, see documentation therein for its usage
    BuildNativeOrMask(orMask); // prepare the or-mask
    const ged_ins_decoding_table_t compactTable = GetCurrentModelData().opcodeTables[_opcode].compactDecoding;  // map to this
    if (NULL == compactTable) return false; // the instruction doesn't have a compact format

    const ged_compact_mapping_table_t mappingTable = GetCurrentModelData().opcodeTables[_opcode].compactMapping;
    GEDASSERT(NULL != mappingTable);
    for (uint32_t i = 0; i < GetCurrentModelData().numberOfInstructionFields; ++i)
    {
        GEDASSERT(compactTable[i]._entryType < GED_TABLE_ENTRY_TYPE_SIZE); // verify that this is a valid entry
        if (GED_TABLE_ENTRY_TYPE_NOT_SUPPORTED == compactTable[i]._entryType) continue; // this field is not supported in this format
        if (!CountCurrentField(compactTable, mappingTable, orMask, i, tempCounter)) return false;
    }
    count = tempCounter;
    return true;
}


bool GEDIns::CountCurrentField(const ged_ins_decoding_table_t compactTable, const ged_compact_mapping_table_t mappingTable,
                               const unsigned char* const orMask, const /* GED_INS_FIELD */ uint32_t field, unsigned int& count)
{
    GEDASSERT(NULL != compactTable);
    GEDASSERT(NULL != mappingTable);
    // Get the mapping table entry for this field.
    static const unsigned char validBits[GED_NATIVE_INS_SIZE] = { 0 }; // assume that all the native instruction fields are valid
    const ged_compact_mapping_entry_t* mappingEntry = GetCompactionMappingEntry(mappingTable, field, validBits);
    if (NULL == mappingEntry) return false; // when building compact from native but the mapping is invalid
    GEDASSERT(mappingEntry->_field == field);

    // For each entry type, count the amount of valid values: Value mappings have 1 valid value, and index mappings require iterating
    // over all indexes, counting the valid ones and multiplying in the end.
    switch (mappingEntry->_entryType)
    {
    case GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_CONSECUTIVE:
    {
        // This is an index field, so the actual value to be encoded is an index in the field's compaction table. Try to find a
        // compaction table entry that matches the collected value. If such an entry is found, "count" is multiplied by the amount
        // of valid entries in the compact table, otherwise the function returns FALSE without setting the field.
        uint64_t qwval = (uint64_t)ExtractConsecutiveEntryValue(_nativeBytes, mappingEntry->_consecutive._to);
        const uint64_t valMask = (uint64_t)ExtractConsecutiveEntryValue(orMask, mappingEntry->_consecutive._to);
        GEDASSERT(GED_TABLE_ENTRY_TYPE_NEXT_TABLE != compactTable[field]._entryType); // index fields must be explicit
        GEDASSERT(MAX_UINT32_T > MaxValue(compactTable[field]));
        const uint32_t tableSize = BitsToNumOfValues(compactTable[field]._bitSize); // size of the compaction table
        GEDASSERT(NULL != mappingEntry->_compactionTable);
        if (!CountCompactionTableEntry(qwval, valMask, tableSize, mappingEntry->_compactionTable, count)) return false;
        break;
    }
    case GED_MAPPING_TABLE_ENTRY_TYPE_VALUE_MAPPING_CONSECUTIVE:
        break;
    case GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_FRAGMENTED:
    {
        // See documentation in the GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_CONSECUTIVE case above.
        uint64_t qwval = 0;
        uint64_t tempValMask = 0;
        if (!CollectFragmentedEntryQWValue(qwval, _nativeBytes, mappingEntry)) return false;
        if (!CollectFragmentedEntryQWValue(tempValMask, orMask, mappingEntry)) return false;
        const uint64_t valMask = tempValMask;
        GEDASSERT(GED_TABLE_ENTRY_TYPE_NEXT_TABLE != compactTable[field]._entryType); // index fields must be explicit
        GEDASSERT(MAX_UINT32_T > MaxValue(compactTable[field]));
        const uint32_t tableSize = BitsToNumOfValues(compactTable[field]._bitSize); // size of the compaction table
        GEDASSERT(NULL != mappingEntry->_compactionTable);
        if (!CountCompactionTableEntry(qwval, valMask, tableSize, mappingEntry->_compactionTable, count)) return false;
        break;
    }
    case GED_MAPPING_TABLE_ENTRY_TYPE_VALUE_MAPPING_FRAGMENTED:
        break;
    case GED_MAPPING_TABLE_ENTRY_TYPE_NO_MAPPING:
        break;
    default:
        GEDASSERT(0);
    }
    return true;
}


bool GEDIns::CountCompactionTableEntry(uint64_t& val, const uint64_t& valMask, const uint32_t tableSize,
                                       ged_compaction_table_t table, unsigned int& count) const
{
    uint32_t counter = 0;
    GEDASSERT(0 != tableSize);
    GEDASSERT(tableSize < GED_MAX_ENTRIES_IN_COMPACT_TABLE); // sanity check
    val |= valMask;
    for (uint32_t i = 0; i < tableSize; ++i)
    {
        if ((table[i] | valMask) == val)
        {
            ++counter;
        }
    }
    count *= counter;
    return (0 != counter);
}


bool GEDIns::BuildAllCompactedFormats(unsigned char* compactBytesArray, const unsigned int size)
{
    const ged_ins_decoding_table_t compactTable = GetCurrentModelData().opcodeTables[_opcode].compactDecoding;  // map to this
    if (NULL == compactTable) return false; // the instruction doesn't have a compact format

    const ged_compact_mapping_table_t mappingTable = GetCurrentModelData().opcodeTables[_opcode].compactMapping;
    GEDASSERT(NULL != mappingTable);

    // Traverse all the compact instruction fields. For every valid non-indexed field, collect its value from the native instruction
    // and set that value in the compact instruction template.
    unsigned char compactBytesTemplate[GED_COMPACT_INS_SIZE] = { 0 };
    for (uint32_t i = 0; i < GetCurrentModelData().numberOfInstructionFields; ++i)
    {
        GEDASSERT(compactTable[i]._entryType < GED_TABLE_ENTRY_TYPE_SIZE); // verify that this is a valid entry
        if (GED_TABLE_ENTRY_TYPE_NOT_SUPPORTED == compactTable[i]._entryType) continue; // this field is not supported in this format
        if (!CollectCurrentValueField(compactTable, mappingTable, i, compactBytesTemplate)) return false;
    }
    SetCompact(compactBytesTemplate);

    // Traverse all the compact instruction fields. For every valid indexed field, collect all possible options.
    // Basically, create a Cartesian product of all possible indexes which create a valid compact encoding.
    unsigned char orMask[GED_NATIVE_INS_SIZE] = { 0 }; // used in CollectCurrentMappedFields, see documentation therein for its usage
    BuildNativeOrMask(orMask); // prepare the or-mask
    bool succeeded = true;
    vector<vector<unsigned char> > compactBytesIndexedVector = CollectCurrentMappedFields(compactTable, mappingTable, orMask, 0,
                                                                                          succeeded);
    if (!succeeded) return false;

    // Merge the mapped values with the indexed ones
    memset(compactBytesArray, 0x0, size); // clear the entire preallocated buffer
    unsigned char* ptr = compactBytesArray;
    for (uint32_t i = 0; i < size / GED_COMPACT_INS_SIZE; ++i)
    {
        for (uint32_t j = 0; j < GED_NUM_OF_COMPACT_INS_DWORDS; ++j)
        {
            vector<unsigned char>* tmpVecPtr = &compactBytesIndexedVector[i];
            unsigned char* tmpWordPtr = &((*tmpVecPtr)[0]);
            ((uint32_t*)ptr)[j] = ((uint32_t*)compactBytesTemplate)[j] | ((uint32_t*)tmpWordPtr)[j];
        }
        ptr += GED_COMPACT_INS_SIZE;
    }
    return true;

    // Set an arbitrary valid encoding into the instruction's compact bytes.
    memcpy_s(_compactBytes, GED_COMPACT_INS_SIZE, compactBytesArray, GED_COMPACT_INS_SIZE);
    SetCompactValid();
    return true;
}


bool GEDIns::CollectCurrentValueField(const ged_ins_decoding_table_t compactTable, const ged_compact_mapping_table_t mappingTable,
                                      const /* GED_INS_FIELD */ uint32_t field, unsigned char* buf)
{
    GEDASSERT(NULL != compactTable);
    GEDASSERT(NULL != mappingTable);

    // Get the mapping table entry for this field.
    static const unsigned char validBits[GED_NATIVE_INS_SIZE] = { 0 }; // assume that all the native instruction fields are valid
    const ged_compact_mapping_entry_t* mappingEntry = GetCompactionMappingEntry(mappingTable, field, validBits);
    if (NULL == mappingEntry) return false; // when building compact from native but the mapping is invalid
    GEDASSERT(mappingEntry->_field == field);

    // Collect the compact field from the native field(s).
    uint32_t val = MAX_UINT32_T;
    switch (mappingEntry->_entryType)
    {
    case GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_CONSECUTIVE:
        return true;
    case GED_MAPPING_TABLE_ENTRY_TYPE_VALUE_MAPPING_CONSECUTIVE:
        val = ExtractConsecutiveEntryValue(_nativeBytes, mappingEntry->_consecutive._to);
        break;
    case GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_FRAGMENTED:
        return true;
    case GED_MAPPING_TABLE_ENTRY_TYPE_VALUE_MAPPING_FRAGMENTED:
        if (!CollectFragmentedEntryDWValue(val, _nativeBytes, mappingEntry)) return false;
        break;
    case GED_MAPPING_TABLE_ENTRY_TYPE_NO_MAPPING:
        return true;
    default:
        GEDASSERT(0);
    }

    // Set the field in the compact bytes. The given value is already the raw value, no need to convert it if it has an enumeration.
    GEDFORASSERT(GED_RETURN_VALUE ret = )
        SetField(buf, compactTable, field, GED_VALUE_TYPE_ENCODED, val);
    GEDASSERT(GED_RETURN_VALUE_SUCCESS == ret);
    return true;
}


vector<vector<unsigned char> > GEDIns::CollectCurrentMappedFields(const ged_ins_decoding_table_t compactTable,
                                                                  const ged_compact_mapping_table_t mappingTable,
                                                                  const unsigned char* const orMask,
                                                                  /* GED_INS_FIELD */ uint32_t field, bool& succeeded)
{
    // Collecting all mapped fields algorithm:
    //
    // CollectCurrentMappedFields is a backtracking recursion designed to generate a Cartesian product of all mapped fields.
    // The slight difference from a regular AxBxCx... Cartesian product where each element is on its own in the n-tuple, here each
    // element in the n-tuple consists of fields and their location. A is a field, set into its position in the compacted bytes by
    // calling SetField()
    //
    // How the recursion works:
    // Stop condition: when finished traversing all fields, return a vector with a single, empty encoding.
    // Recursion step:
    // 1        Declare temporary vector of compact instruction "res"
    // 2        For each Compact Encoding "it" in CollectCurrentMappedFields(..., field + 1, ...):
    // 2.1        For each valid index "compactedTableIndex" in the compacted table:
    // 2.1.1        Create a buffer "buf" and copy the compact encoding of "it" to it
    // 2.1.2        Set the compactedTableIndex in the appropriate location in buf
    // 2.1.3        Push "buf" into the vector of compacted instructions "res".
    // 3        Return "res".

    GEDASSERT(NULL != compactTable);
    GEDASSERT(NULL != mappingTable);
    GEDASSERT(field < GetCurrentModelData().numberOfInstructionFields);

    static const unsigned char validBits[GED_NATIVE_INS_SIZE] = { 0 }; // assume that all the native instruction fields are valid
    const ged_compact_mapping_entry_t* mappingEntry;

    // Locate the next Index-mapped field.
    while (true)
    {
        if (GetCurrentModelData().numberOfInstructionFields == field)
        {
            // Stop condition.
            vector<vector<unsigned char> > stubVector;
            vector<unsigned char> stubEncoding(8); // insert an empty encoding to the vector
            stubVector.push_back(stubEncoding);
            return stubVector;
        }
        if (GED_TABLE_ENTRY_TYPE_NOT_SUPPORTED == compactTable[field]._entryType)
        {
            ++field;
            continue;
        }
        mappingEntry = GetCompactionMappingEntry(mappingTable, field, validBits);
        GEDASSERT(NULL != mappingTable); // this is potentially wrong, see other GetCompactionMappingEntry "if" checks
        if (GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_FRAGMENTED == mappingEntry->_entryType  ||
            GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_CONSECUTIVE == mappingEntry->_entryType)
        {
            break; // Handle index fields - enter the body of the function.
        }
        ++field;
    }

    // Step 1
    vector<vector<unsigned char> > res;

    // Step 2
    vector<vector<unsigned char> > v = CollectCurrentMappedFields(compactTable, mappingTable, orMask, field + 1, succeeded);
    if (!succeeded) return v;

    for (vector<vector<unsigned char> >::const_iterator it = v.begin(); it != v.end(); ++it)
    {
        switch (mappingEntry->_entryType)
        {
        case GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_CONSECUTIVE:
        {
            uint32_t val = MAX_UINT32_T;
            uint64_t qwval = (uint64_t)ExtractConsecutiveEntryValue(_nativeBytes, mappingEntry->_consecutive._to);
            const uint64_t valMask = (uint64_t)ExtractConsecutiveEntryValue(orMask, mappingEntry->_consecutive._to);
            GEDASSERT(GED_TABLE_ENTRY_TYPE_NEXT_TABLE != compactTable[field]._entryType); // index fields must be explicit
            GEDASSERT(MAX_UINT32_T > MaxValue(compactTable[field]));
            const uint32_t tableSize = BitsToNumOfValues(compactTable[field]._bitSize); // size of the compaction table
            GEDASSERT(NULL != mappingEntry->_compactionTable);
            GEDASSERT(0 != tableSize);
            GEDASSERT(tableSize < GED_MAX_ENTRIES_IN_COMPACT_TABLE); // sanity check
            qwval |= valMask;
            // Step 2.1
            for (unsigned int compactedTableIndex = 0; compactedTableIndex < tableSize; ++compactedTableIndex)
            {
                if ((mappingEntry->_compactionTable[compactedTableIndex] | valMask) == qwval)
                {
                    val = (uint32_t)compactedTableIndex;
                    unsigned char buf[GED_COMPACT_INS_SIZE];
                    // Step 2.1.1
                    std::copy(it->begin(), it->end(), buf);
                    // Step 2.1.2
                    GEDFORASSERT(GED_RETURN_VALUE ret = )
                        SetField(buf, compactTable, field, GED_VALUE_TYPE_ENCODED, val);
                    GEDASSERT(GED_RETURN_VALUE_SUCCESS == ret);
                    // Step 2.1.3
                    res.push_back(vector<unsigned char>(buf, buf + GED_COMPACT_INS_SIZE * sizeof(buf[0])));
                }
            }
            if (MAX_UINT32_T == val) succeeded = false;
            break;
        }
        case GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_FRAGMENTED:
        {
            uint32_t val = MAX_UINT32_T;
            uint64_t qwval = 0;
            uint64_t tempValMask = 0;
            if (!CollectFragmentedEntryQWValue(qwval, _nativeBytes, mappingEntry)) succeeded = false;
            if (!CollectFragmentedEntryQWValue(tempValMask, orMask, mappingEntry)) succeeded = false;
            if (!succeeded) break;
            const uint64_t valMask = tempValMask;
            GEDASSERT(GED_TABLE_ENTRY_TYPE_NEXT_TABLE != compactTable[field]._entryType); // index fields must be explicit
            GEDASSERT(MAX_UINT32_T > MaxValue(compactTable[field]));
            const uint32_t tableSize = BitsToNumOfValues(compactTable[field]._bitSize); // size of the compaction table
            GEDASSERT(NULL != mappingEntry->_compactionTable);
            GEDASSERT(0 != tableSize);
            GEDASSERT(tableSize < GED_MAX_ENTRIES_IN_COMPACT_TABLE); // sanity check
            qwval |= valMask;
            // Step 2.1
            for (unsigned int compactedTableIndex = 0; compactedTableIndex < tableSize; ++compactedTableIndex)
            {
                if ((mappingEntry->_compactionTable[compactedTableIndex] | valMask) == qwval)
                {
                    val = (uint32_t)compactedTableIndex;
                    unsigned char buf[GED_COMPACT_INS_SIZE];
                    // Step 2.1.1
                    std::copy(it->begin(), it->end(), buf);
                    // Step 2.1.2
                    GEDFORASSERT(GED_RETURN_VALUE ret = )
                        SetField(buf, compactTable, field, GED_VALUE_TYPE_ENCODED, val);
                    GEDASSERT(GED_RETURN_VALUE_SUCCESS == ret);
                    // Step 2.1.3
                    res.push_back(vector<unsigned char>(buf, buf + GED_COMPACT_INS_SIZE * sizeof(buf[0])));
                }
            }
            if (MAX_UINT32_T == val) succeeded = false;
            break;
        }
        default:
            GEDASSERT(0);
        }
    }
    // Step 3.
    return res;
}
#endif // GED_VALIDATION_API


bool GEDIns::CollectCurrentField(const ged_ins_decoding_table_t compactTable, const ged_compact_mapping_table_t mappingTable,
                                 const unsigned char* const orMask, const /* GED_INS_FIELD */ uint32_t field)
{
    GEDASSERT(NULL != compactTable);
    GEDASSERT(NULL != mappingTable);

    // Get the mapping table entry for this field.
    static const unsigned char validBits[GED_NATIVE_INS_SIZE] = { 0 }; // assume that all the native instruction fields are valid
    const ged_compact_mapping_entry_t* mappingEntry = GetCompactionMappingEntry(mappingTable, field, validBits);
    if (NULL == mappingEntry) return false; // when building compact from native but the mapping is invalid
    GEDASSERT(mappingEntry->_field == field);

    // Collect the compact field from the native field(s).
    uint32_t val = MAX_UINT32_T;
    switch (mappingEntry->_entryType)
    {
    case GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_CONSECUTIVE:
    {
        // This is an index field, so the actual value to be encoded is an index in the field's compaction table. Try to find a
        // compaction table entry that matches the collected value. If such an entry is found, "val" is changed to that entry's
        // index, otherwise the function returns FALSE without setting the field.

        // If a certain bit in a compaction table entry is set but it is mapped to a reserved field in the native instruction, it
        // will be cleared in the native instruction. As a result it will also be cleared in the collected value, which means that
        // the collected value will not match the compaction table entry. To overcome this, an or-mask of the reserved bits is
        // applied to the collected value and to the compaction table entries when they are being compared. This may result in
        // several compaction table entries that match the collected value, in which case the first entry found will be returned.

        uint64_t qwval = (uint64_t)ExtractConsecutiveEntryValue(_nativeBytes, mappingEntry->_consecutive._to);
        const uint64_t valMask = (uint64_t)ExtractConsecutiveEntryValue(orMask, mappingEntry->_consecutive._to);
        GEDASSERT(GED_TABLE_ENTRY_TYPE_NEXT_TABLE != compactTable[field]._entryType); // index fields must be explicit
        GEDASSERT(MAX_UINT32_T > MaxValue(compactTable[field]));
        const uint32_t tableSize = BitsToNumOfValues(compactTable[field]._bitSize); // size of the compaction table
        GEDASSERT(NULL != mappingEntry->_compactionTable);
        if (!FindCompactionTableEntry(qwval, valMask, tableSize, mappingEntry->_compactionTable)) return false;
        GEDASSERT(MAX_UINT32_T >= qwval);
        val = (uint32_t)qwval;
        break;
    }
    case GED_MAPPING_TABLE_ENTRY_TYPE_VALUE_MAPPING_CONSECUTIVE:
        val = ExtractConsecutiveEntryValue(_nativeBytes, mappingEntry->_consecutive._to);
        break;
    case GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_FRAGMENTED:
    {
        // See documentation in the GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_CONSECUTIVE case above.
        uint64_t qwval = 0;
        uint64_t tempValMask = 0;
        if (!CollectFragmentedEntryQWValue(qwval, _nativeBytes, mappingEntry)) return false;
        if (!CollectFragmentedEntryQWValue(tempValMask, orMask, mappingEntry)) return false;
        const uint64_t valMask = tempValMask;
        GEDASSERT(GED_TABLE_ENTRY_TYPE_NEXT_TABLE != compactTable[field]._entryType); // index fields must be explicit
        GEDASSERT(MAX_UINT32_T > MaxValue(compactTable[field]));
        const uint32_t tableSize = BitsToNumOfValues(compactTable[field]._bitSize); // size of the compaction table
        GEDASSERT(NULL != mappingEntry->_compactionTable);
        if (!FindCompactionTableEntry(qwval, valMask, tableSize, mappingEntry->_compactionTable)) return false;
        GEDASSERT(MAX_UINT32_T >= qwval);
        val = (uint32_t)qwval;
        break;
    }
    case GED_MAPPING_TABLE_ENTRY_TYPE_VALUE_MAPPING_FRAGMENTED:
        if (!CollectFragmentedEntryDWValue(val, _nativeBytes, mappingEntry)) return false;
        break;
    case GED_MAPPING_TABLE_ENTRY_TYPE_NO_MAPPING:
        return true;
    default:
        GEDASSERT(0);
    }

    // Set the field in the compact bytes. The given value is already the raw value, no need to convert it if it has an enumeration.
    GEDFORASSERT(GED_RETURN_VALUE ret = )
        SetField(_compactBytes, compactTable, field, GED_VALUE_TYPE_ENCODED, val);
    GEDASSERT(GED_RETURN_VALUE_SUCCESS == ret);
    return true;
}


bool GEDIns::CollectFragmentedEntryDWValue(uint32_t& fullVal, const unsigned char* bytes, const ged_compact_mapping_entry_t* mappingEntry) const
{
    GEDASSERT(NULL != bytes);
    GEDASSERT(NULL != mappingEntry);
    GEDASSERT(GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_FRAGMENTED == mappingEntry->_entryType ||
              GED_MAPPING_TABLE_ENTRY_TYPE_VALUE_MAPPING_FRAGMENTED == mappingEntry->_entryType);
    GEDASSERT(mappingEntry->_fragmented._numOfMappingFragments > 1);

    // The field is fragmented, so all fragments will eventually be gathered in fullVal.
    fullVal = 0;
    for (unsigned int i = 0; i < mappingEntry->_fragmented._numOfMappingFragments; ++i)
    {
        GEDASSERT(0 == mappingEntry->_fragmented._fragments[i]._from._dwordIndex); // the mapping source may only have 32 bits
        uint32_t fragmentVal = 0;
        if (!CollectFragmentValue(fragmentVal, bytes, mappingEntry->_fragmented._fragments[i])) return false;
        fullVal |= fragmentVal;
    }
    return true;
}


bool GEDIns::CollectFragmentedEntryQWValue(uint64_t& val, const unsigned char* bytes, const ged_compact_mapping_entry_t* mappingEntry) const
{
    GEDASSERT(NULL != bytes);
    GEDASSERT(NULL != mappingEntry);
    GEDASSERT(GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_FRAGMENTED == mappingEntry->_entryType ||
              GED_MAPPING_TABLE_ENTRY_TYPE_VALUE_MAPPING_FRAGMENTED == mappingEntry->_entryType);
    GEDASSERT(mappingEntry->_fragmented._numOfMappingFragments > 1);

    // The field is fragmented, so all fragments will eventually be gathered in fullVal. A fragment can't be larger than 32 bits, but
    // the full value may be an entry in a compaction table, which may be 64-bit long.
    val = 0;
    for (unsigned int i = 0; i < mappingEntry->_fragmented._numOfMappingFragments; ++i)
    {
        GEDASSERT(mappingEntry->_fragmented._fragments[i]._from._dwordIndex < 2); // the mapping source may only have 64 bits
        uint32_t fragmentVal = 0;
        if (!CollectFragmentValue(fragmentVal, bytes, mappingEntry->_fragmented._fragments[i])) return false;
        (reinterpret_cast<uint32_t*>(&val))[mappingEntry->_fragmented._fragments[i]._from._dwordIndex] |= fragmentVal;
    }
    return true;
}


bool GEDIns::CollectFragmentValue(uint32_t& val, const unsigned char* bytes, const ged_compact_mapping_fragment_t& mapping) const
{
    val = ((uint32_t*)bytes)[mapping._to._dwordIndex];
    val &= mapping._to._bitMask;
    if (0 == val) return true; // early out - nothing to do
    const int8_t shift = mapping._to._shift - mapping._from._shift;
    if (shift > 0)
    {
        val >>= shift;
    }
    else if (shift < 0)
    {
        val <<= abs(shift);
    }

    // For 1-to-1 mappings there is nothing more to do.
    if (GED_COMPACT_MAPPING_TYPE_1x1 == mapping._mappingType) return true;
    if (GED_COMPACT_MAPPING_TYPE_FIXED == mapping._mappingType)
    {
        return (mapping._from._lowBit == val);
    }
    // Now handle the repped mapping case.
    GEDASSERT(GED_COMPACT_MAPPING_TYPE_REP == mapping._mappingType);
    const uint8_t fromSize = FragmentSize(mapping._from);
    const uint8_t toSize = FragmentSize(mapping._to);
    GEDASSERT(fromSize > 0);               // verify that the source size is non-zero
    GEDASSERT(toSize >= fromSize);         // verify that the target is at least as wide as the source
    GEDASSERT(0 == (toSize % fromSize));   // verify that the target is a whole multiple of the source
    GEDASSERT(0 <= mapping._from._shift);  // verify that the shifting amount is non-negative (in order to shift left)
    // Validate the value is repeated by its substring.
    const uint32_t maxSourceValue = MaxFragmentValue(mapping._from) << mapping._from._shift; // shift the max value to its position
    uint32_t shiftedMask = maxSourceValue;
    uint32_t shiftedRep = val & maxSourceValue;
    // TODO: optimization opportunity: if FragmentSize(fragment) is 1 (which right now, always is), it's possible to simply
    //       compare with all ones (using MaxGragmentValue(mapping._to)) or all-zeros.
    //       The current method used is a general ways, which uses loops and thus slower.
    for (unsigned int i = 0; i < toSize; i += fromSize)
    {
        if (((val & shiftedMask) != shiftedRep)) return false;
        shiftedRep <<= fromSize;
        shiftedMask <<= fromSize;
    }
    val &= maxSourceValue;
    return true;
}


bool GEDIns::FindCompactionTableEntry(uint64_t& val, const uint64_t& valMask, const uint32_t tableSize,
                                      ged_compaction_table_t table) const
{
    GEDASSERT(0 != tableSize);
    GEDASSERT(tableSize < GED_MAX_ENTRIES_IN_COMPACT_TABLE); // sanity check
    val |= valMask;
    for (uint32_t i = 0; i < tableSize; ++i)
    {
        if ((table[i] | valMask) == val)
        {
            val = i;
            return true;
        }
    }
    return false;
}


void GEDIns::ApplyNativeEncodingMasks()
{
# if GED_EXPERIMENTAL
    if (!ShouldApplyEncodingMasks())
    {
        return;
    }
# endif // GED_EXPERIMENTAL
    const ged_instruction_masks_table_t topLevelTable = GetCurrentModelData().opcodeTables[_opcode].nativeEncodingMasks;
    GEDASSERT(NULL != topLevelTable);

    for (unsigned int i = 0; GED_MASKS_TABLE_ENTRY_TYPE_NO_MASKS != topLevelTable[i]._entryType; ++i)
    {
        // Traverse the intermediate tables (if necessary).
        unsigned int tableIndex = i;
        ged_instruction_masks_table_t table = topLevelTable;
        while (GED_MASKS_TABLE_ENTRY_TYPE_NEXT_TABLE == table[tableIndex]._entryType)
        {
            const ged_instruction_masks_next_table_t* nextTable = &table[tableIndex]._nextTable;
            GED_RETURN_VALUE ret = GED_RETURN_VALUE_INVALID_FIELD;
            tableIndex = GetField<uint32_t>(_nativeBytes, _decodingTable, nextTable->_tableKey, GED_VALUE_TYPE_ENCODED, ret);

            // GetField is expected to succeed since the models ensure that the dependent fields are always valid. However, we need to
            // account for invalid instructions, i.e. instructions with an illegal encoding. Such instructions may have invalid values
            // for valid fields. If a dependee field is in itself a dependent field, it may even be an invalid field in the given
            // (poorly encoded) instruction.
            if (GED_RETURN_VALUE_INVALID_FIELD == ret || GED_RETURN_VALUE_INVALID_VALUE == ret) return;
            GEDASSERT(GED_RETURN_VALUE_SUCCESS == ret);
            table = nextTable->_tablePtr;
            GEDASSERT(NULL != table);
        }
        if (GED_MASKS_TABLE_ENTRY_TYPE_NO_MASKS == table[tableIndex]._entryType) continue; // no mask necessary

        // Verify that this is a masks entry.
        GEDASSERTM(GED_MASKS_TABLE_ENTRY_TYPE_MASKS == table[tableIndex]._entryType, DecStr(table[tableIndex]._entryType));
        for (unsigned int dword = 0; dword < GED_NUM_OF_NATIVE_INS_DWORDS; ++dword)
        {
            ((uint32_t*)_nativeBytes)[dword] |= ((uint32_t*)(table[tableIndex]._masks._or))[dword];
            ((uint32_t*)_nativeBytes)[dword] &= ((uint32_t*)(table[tableIndex]._masks._and))[dword];
        }
    }
    SetNativeEncoded();
}


void GEDIns::ApplyCompactEncodingMasks(unsigned char* compactBytes)
{
    const ged_instruction_masks_table_t topLevelTable = GetCurrentModelData().opcodeTables[_opcode].compactEncodingMasks;
    GEDASSERT(NULL != topLevelTable);

    for (unsigned int i = 0; GED_MASKS_TABLE_ENTRY_TYPE_NO_MASKS != topLevelTable[i]._entryType; ++i)
    {
        // Traverse the intermediate tables (if necessary).
        unsigned int tableIndex = i;
        ged_instruction_masks_table_t table = topLevelTable;
        while (GED_MASKS_TABLE_ENTRY_TYPE_NEXT_TABLE == table[tableIndex]._entryType)
        {
            const ged_instruction_masks_next_table_t* nextTable = &table[tableIndex]._nextTable;
            GED_RETURN_VALUE ret = GED_RETURN_VALUE_INVALID_FIELD;
            tableIndex = GetField<uint32_t>(_nativeBytes, _decodingTable, nextTable->_tableKey, GED_VALUE_TYPE_ENCODED, ret);

            // GetField is expected to succeed since the models ensure that the dependent fields are always valid. However, we need to
            // account for invalid instructions, i.e. instructions with an illegal encoding. Such instructions may have invalid values
            // for valid fields. If a dependee field is in itself a dependent field, it may even be an invalid field in the given
            // (poorly encoded) instruction.
            if (GED_RETURN_VALUE_INVALID_FIELD == ret || GED_RETURN_VALUE_INVALID_VALUE == ret) return;
            GEDASSERT(GED_RETURN_VALUE_SUCCESS == ret);
            table = nextTable->_tablePtr;
            GEDASSERT(NULL != table);
        }
        if (GED_MASKS_TABLE_ENTRY_TYPE_NO_MASKS == table[tableIndex]._entryType) continue; // no mask necessary

        // Verify that this is a masks entry.
        GEDASSERT(GED_MASKS_TABLE_ENTRY_TYPE_MASKS == table[tableIndex]._entryType);
        for (unsigned int dword = 0; dword < GED_NUM_OF_COMPACT_INS_DWORDS; ++dword)
        {
            ((uint32_t*)compactBytes)[dword] |= ((uint32_t*)(table[tableIndex]._masks._or))[dword];
            ((uint32_t*)compactBytes)[dword] &= ((uint32_t*)(table[tableIndex]._masks._and))[dword];
        }
    }
    SetCompactEncoded();
}


void GEDIns::BuildNativeOrMask(unsigned char* orMask) const
{
    const ged_instruction_masks_table_t topLevelTable = GetCurrentModelData().opcodeTables[_opcode].nativeEncodingMasks;
    GEDASSERT(NULL != topLevelTable);

    for (unsigned int i = 0; GED_MASKS_TABLE_ENTRY_TYPE_NO_MASKS != topLevelTable[i]._entryType; ++i)
    {
        // Traverse the intermediate tables (if necessary).
        unsigned int tableIndex = i;
        ged_instruction_masks_table_t table = topLevelTable;
        while (GED_MASKS_TABLE_ENTRY_TYPE_NEXT_TABLE == table[tableIndex]._entryType)
        {
            const ged_instruction_masks_next_table_t* nextTable = &table[tableIndex]._nextTable;
            GED_RETURN_VALUE ret = GED_RETURN_VALUE_INVALID_FIELD;
            tableIndex = GetField<uint32_t>(_nativeBytes, _decodingTable, nextTable->_tableKey, GED_VALUE_TYPE_ENCODED, ret);
            GEDASSERT(GED_RETURN_VALUE_SUCCESS == ret);
            table = nextTable->_tablePtr;
            GEDASSERT(NULL != table);
        }
        if (GED_MASKS_TABLE_ENTRY_TYPE_NO_MASKS == table[tableIndex]._entryType) continue; // no mask necessary

        // Verify that this is a masks entry.
        GEDASSERTM(GED_MASKS_TABLE_ENTRY_TYPE_MASKS == table[tableIndex]._entryType, DecStr(table[tableIndex]._entryType));
        for (unsigned int dword = 0; dword < GED_NUM_OF_NATIVE_INS_DWORDS; ++dword)
        {
            ((uint32_t*)orMask)[dword] |= ((uint32_t*)(table[tableIndex]._masks._or))[dword];
            ((uint32_t*)orMask)[dword] &= ((uint32_t*)(table[tableIndex]._masks._and))[dword];
        }
    }
}


string GEDIns::GetInstructionBytes(const unsigned char* instructionBytes, int dwords) const
{
    stringstream strm;
    strm << "0x" << setfill('0') << hex;
    for (--dwords; dwords >= 0; --dwords)
    {
        strm << setw(8) << reinterpret_cast<const uint32_t*>(instructionBytes)[dwords];
    }
    return strm.str();
}


#if GED_VALIDATION_API
bool GEDIns::RecordPadding(vector<ged_ins_field_mapping_fragment_t> &mappingFragments, const ged_ins_field_entry_t* dataEntry) const
{
    if (dataEntry->_restrictions && (dataEntry->_restrictions[0]->_restrictionType == GED_FIELD_RESTRICTIONS_TYPE_PADDING))
    {
        const get_field_restriction_padding_t& padding = dataEntry->_restrictions[0]->_padding;
        const uint32_t paddingMask = padding._mask;
        const uint32_t paddingValue = padding._value;
        for (unsigned int i = 0; i < GED_DWORD_BITS; ++i)
        {
            if (0 == (paddingMask & (1 << i))) continue;

            // Bit number i is set to one, count the length of sequential ones.
            unsigned int sequenceLength = 0;
            for (; paddingMask & (1 << i); ++i)
            {
                ++sequenceLength;
            }

            // Record the padding fragment.
            const uint8_t lowBit = i - sequenceLength;
            const uint8_t mask = (1 << (sequenceLength + 1)) - 1; // starting from LSB
            ged_ins_field_mapping_fragment_t fixedFragment = {};
            fixedFragment._fixed = true;
            fixedFragment._from._lowBit = lowBit;
            fixedFragment._from._highBit = i - 1;
            fixedFragment._value = (paddingValue >> lowBit) & mask;
            mappingFragments.emplace_back(fixedFragment);
        }
        return true;
    }
    return false;
}
#endif // GED_VALIDATION_API

bool GEDIns::RecordPosition(vector<ged_ins_field_mapping_fragment_t> &mappingFragments, const ged_ins_field_entry_t* dataEntry) const
{
    bool hasFixedValue = false;
    switch (dataEntry->_entryType)
    {
    case GED_TABLE_ENTRY_TYPE_CONSECUTIVE:
    {
        GEDASSERT(dataEntry->_bitSize <= GED_DWORD_BITS);
        if (0 == dataEntry->_consecutive._position._bitMask)
        {
            // This field consists only from padded positions, which was already recorded previously.
            GEDASSERT(dataEntry->_restrictions);
            GEDASSERT(dataEntry->_restrictions[0]->_restrictionType == GED_FIELD_RESTRICTIONS_TYPE_PADDING);
            break;
        }
        RecordSingleFragment(mappingFragments, dataEntry->_consecutive._position);
        break;
    }
    case GED_TABLE_ENTRY_TYPE_FRAGMENTED:
    {
        const ged_ins_field_multiple_fragments_t& fragmentedPosition = dataEntry->_fragmented;
        const uint32_t numOfFragments = fragmentedPosition._numOfPositionFragments;
        for (uint8_t i = 0; i < numOfFragments; ++i)
        {
            RecordSingleFragment(mappingFragments, fragmentedPosition._fragments[i]);
        }
        break;
    }
    case GED_TABLE_ENTRY_TYPE_FIXED_VALUE:
    {
        GEDASSERT(dataEntry->_bitSize <= GED_DWORD_BITS);
        ged_ins_field_mapping_fragment_t fixedFragment = {};
        fixedFragment._fixed = true;
        fixedFragment._from._lowBit = 0;
        fixedFragment._from._highBit = dataEntry->_bitSize - 1;
        fixedFragment._value = dataEntry->_fixed._value;
        mappingFragments.emplace_back(fixedFragment);
        hasFixedValue = true;
        break;
    }
    default:
        GEDASSERT(0);
    }
    GEDASSERT(!mappingFragments.empty());
    return hasFixedValue;
}


void GEDIns::RecordSingleFragment(vector<ged_ins_field_mapping_fragment_t> &mappingFragments,
                                  const ged_ins_field_position_fragment_t &position) const
{
    const uint8_t normalizedLowBit = position._lowBit - position._dwordIndex * GED_DWORD_BITS - position._shift;
    ged_ins_field_mapping_fragment_t fragment;
    fragment._fixed = false;
    fragment._from._lowBit = normalizedLowBit;
    fragment._from._highBit = normalizedLowBit + position._highBit - position._lowBit;
    fragment._to = position;
    mappingFragments.emplace_back(fragment);
}


void GEDIns::MergeFragments(vector<ged_ins_field_mapping_fragment_t> &mappingFragments) const
{
    for (auto iter = mappingFragments.begin(); iter != mappingFragments.end(); ++iter)
    {
        if (mappingFragments.end() == iter + 1) break;

        const auto nextIter = next(iter);
        if ((iter->_from._highBit + 1 == nextIter->_from._lowBit) &&
            (iter->_fixed == nextIter->_fixed) &&
            (iter->_to._highBit + 1 == nextIter->_to._lowBit))
        {
            // Merge the next cell into current
            iter->_from._highBit = nextIter->_from._highBit;
            iter->_to._highBit = nextIter->_to._highBit;
            mappingFragments.erase(nextIter);
        }
    }
}

