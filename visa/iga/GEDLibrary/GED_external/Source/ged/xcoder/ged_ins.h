/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef GED_INS_H
#define GED_INS_H

#include <set>
#include <vector>
#include "common/ged_base.h"
#include "common/ged_ins_decoding_table.h"
#include "common/ged_compact_mapping_table.h"
#include "common/ged_validation_utils.h"
#include "xcoder/ged_internal_api.h"
#include "xcoder/ged_restrictions_handler.h"

using std::set;
using std::vector;


enum GED_INS_STATUS
{
    GED_INS_STATUS_CLEAR            = 0,
    GED_INS_STATUS_NATIVE_VALID     = 1,
    GED_INS_STATUS_COMPACT_VALID    = 1 << 1,
    GED_INS_STATUS_NATIVE_ENCODED   = 1 << 2,
    GED_INS_STATUS_COMPACT_ENCODED  = 1 << 3,
# if GED_EXPERIMENTAL
    GED_INS_STATUS_RAW_BIT_SET      = 1 << 4,
# endif // GED_EXPERIMENTAL

    // Utility enum entries - useful combinations
    GED_INS_STATUS_VALID    = GED_INS_STATUS_NATIVE_VALID | GED_INS_STATUS_COMPACT_VALID,
    GED_INS_STATUS_ENCODED  = GED_INS_STATUS_NATIVE_ENCODED | GED_INS_STATUS_COMPACT_ENCODED
};


class GEDIns
{
public:
    // API FUNCTIONS

    /*!
     * Initialize an empty instruction with a new opcode.
     *
     * @param[in]   modelId     GED model version as specified by @ref GED_MODEL.
     * @param[in]   opcode      The enumerator representing the new opcode.
     *
     * @return      GED_RETURN_VALUE indicating success or invalid opcode.
     */
    GED_RETURN_VALUE Init(const /* GED_MODEL */ uint8_t modelId, /* GED_OPCODE */ uint32_t opcode);


    /*!
     * Assign raw (undecoded) bytes to the GEDIns object. If the instruction is compact, it is mapped to the native format.
     *
     * @param[in]   modelId     GED model version as specified by @ref GED_MODEL.
     * @param[in]   rawBytes    An array of size "size" containing the raw bytes.
     * @param[in]   size        The size of the array.
     *
     * @return      GED_RETURN_VALUE indicating success or decoding error.
     *
     * @note        If the array is larger than GED_NATIVE_INS_SIZE, the remaining bytes are ignored.
     */
    GED_RETURN_VALUE Decode(const /* GED_MODEL */ uint8_t modelId, const unsigned char* rawBytes, const unsigned int size);


    /*!
     * Copy the raw bytes to the given rawBytes buffer. If the instruction cannot be encoded as requested, the given buffer will
     * remain unchanged. The buffer is expected to be preallocated and large enough to hold the requested instruction bytes. One
     * may pass a NULL pointer instead of a preallocated buffer, in which case the function will apply all encoding restrictions and
     * return.
     *
     * @param[in]   insType    Determine if instruction should be compacted or not. If the instruction does not have a valid
     *                           compact form, the rawBytes buffer will remain unchanged and error will be returned.
     * @param[out]  rawBytes   A preallocated buffer in which to store the instruction bytes. The buffer is expected to be large
     *                           enough to hold the requested instruction bytes.
     *
     * @return      GED_RETURN_VALUE indicating success or encoding error.
     */
    GED_RETURN_VALUE Encode(const GED_INS_TYPE insType, unsigned char* rawBytes);


# if GED_VALIDATION_API
    /*!
     * Count the amount of valid compact encodings the instruction has.
     *
     * @param[out]  count       The number of valid compact encodings the instruction has. If an instruction does not have a valid
     *                            compact encoding, result is undefined.
     *
     * @return      GED_RETURN_VALUE indicating success or encoding error.
     *
     * @note        If the given instruction is encoded in its compacted format, the function would first uncompact the instruction.
     */
    GED_RETURN_VALUE CountCompacted(unsigned int& count);


    /*!
     * Generate all valid compact encodings of the instruction. The buffer is expected to be preallocated and large enough to hold the
     * requested instructions' bytes. Use @ref CountCompacted get the amount of expected compact encodings.
     *
     * @param[in]   size                The size of the preallocated buffer @ref rawBytes, in bytes.
     * @param[out]  compactBytesArray   A preallocated buffer in which to store the instruction bytes. The buffer is expected to be
     *                                  large enough to hold the requested instructions' bytes.
     *
     * @return      GED_RETURN_VALUE indicating success or encoding error.
     *
     * @note        If the given instruction is encoded in its compacted format, the function would first uncompact the instruction.
     */
    GED_RETURN_VALUE RetrieveAllCompactedFormats(const unsigned int size, unsigned char* compactBytesArray);


    /*!
    * Print all bit positions and their mapping of given field in the instruction. If a field is invalid, print accordingly.
    * If a fragment of the field has a fixed value ("padded"), print its fixed value.
    *
    * @param[in]   field   Id of the requested field.
    *
    * @return      GED_RETURN_VALUE indicating success or encoding error.
    *
    */
    GED_RETURN_VALUE PrintFieldBitLocation(const /* GED_INS_FIELD */ uint32_t field) const;

# endif // GED_VALIDATION_API

    /*!
    * Return description of how a field is encoded within instructions. Top 16bits of
    * each fragment specifies the length, whereas bottom 16bits specifies LSB of field.
    * Pass NULL as fragments to have a function return the number of elements in 'length'.
    *
    * @param[in]   field   Id of the requested field.
    * @param[in]   fragments Array of fragments (*length in size)
    * @param[in]   length  Number of entries in fragments
    *
    * @return      GED_RETURN_VALUE indicating success or encoding error.
    *
    */
    GED_RETURN_VALUE QueryFieldBitLocation(const /* GED_INS_FIELD */ uint32_t field, uint32_t *fragments, uint32_t *length) const;

    /*!
     * Get the GED_MODEL Id for the GED model version by which this instruction is decoded.
     *
     * @return      The requested GED_MODEL Id.
     */
    inline /* GED_MODEL */ uint8_t GetCurrentModel() const { return _modelId; }


    /*!
     * Get the instruction's opcode in the form of a GED_OPCODE enumerator. Use @ref GetRawOpcode for obtaining the raw encoding of
     * the opcode in the GED model instruction.
     *
     * @return      The enumerator representing the instruction's opcode.
     *
     * @note        This function is predefined and does not depend on the active model.
     */
    inline /* GED_OPCODE */ uint32_t GetOpcode() const { return *((uint32_t*)(GetCurrentModelData().Opcodes)[_opcode]); }


    /*!
     * Get the instruction raw opcode as it is encoded in the GED model instruction. Use @ref GetOpcode for obtaining the GED_OPCODE for
     * this opcode.
     *
     * @return      The opcode encoding.
     *
     * @note        This function is predefined and does not depend on the active model.
     */
    inline uint8_t GetRawOpcode() const { return _opcode; }


    /*!
     * Ascertain whether this instruction was modified (i.e. on of the Set***Field APIs was called) since it was last decoded/encoded
     * (i.e since the last call to @ref Decode or @ref Encode).
     *
     * @return      TRUE if the instruction was modified since the last decode/encode operation.
     */
    inline bool IsModified() const { return !IsEncoded(); }


    /*!
     * Indicates whether the instruction is encoded as compact or not. For decoded instructions it reflects the state of the CmptCtrl
     * bit. For Encoded instructions, it reflects the encoded status. If @ref Encode was successfully called with compact=TRUE then
     * the function will return TRUE. If @ref Encode was successfully called with GED_INS_TYPE_COMPACT, then the function will return
     * FALSE regardless of the validity of the instruction's compact form. This means that even if the instruction can be encoded as
     * compact the function will still return FALSE. If @ref IsModified returns TRUE, this function will return FALSE regardless of the
     * validity of the instruction's compact form.
     *
     * @return      TRUE if the instruction is in compact format.
     *
     * @note        This function is predefined and does not depend on the active model.
     */
    inline bool IsCompact() const { return IsCompactEncoded(); }


    /*!
     * Return the size (in bytes) of the instruction as it is encoded. See @ref IsCompact for details.
     *
     * @return      The size of the instruction.
     */
    inline uint32_t GetInstructionSize() const { return (IsCompact()) ? GED_COMPACT_INS_SIZE : GED_NATIVE_INS_SIZE; }


    /*!
     * Get the size (in bits) of the given field. If the field is invalid for this instruction, the function returns 0.
     *
     * @param[in]   field   Id of the requested field. For precompiled models, these are the possible values of GED_INS_FIELD.
     *                        For custom models, these are Ids assigned by the model parser.
     *
     * @return      The field's size (in bits) if it is valid, 0 otherwise.
     */
    uint32_t GetFieldSize(const /* GED_INS_FIELD */ uint32_t field) const;

# if GED_VALIDATION_API
    /*!
    * Get the name of the given field.
    *
    * @param[in]   field   Id of the requested field. For precompiled models, these are the possible values of GED_INS_FIELD.
    *                        For custom models, these are Ids assigned by the model parser.
    *
    * @return      The field's name.
    */
    inline const char* GetFieldName(const /* GED_INS_FIELD */ uint32_t field) const { return fieldNameByField[opcodeFieldEnumId]; }
# endif // GED_VALIDATION_API


    /*!
     * Generic function for retrieving the value of a signed instruction field of size up to 32 bits. If the field size is less than
     * the size of int32_t, the returned value is sign extended. If the field is not valid for this instruction, -1 is returned.
     *
     * @param[in]   field   Id of the requested field. For precompiled models, these are the possible values of GED_INS_FIELD.
     *                        For custom models, these are Ids assigned by the model parser.
     * @param[out]  ret     The function stores a GED_RETURN_VALUE indicating success or decoding error.
     *
     * @return      On success (the field is valid), the function returns the requested field's value, -1 otherwise.
     *
     * @note        -1 may be a valid value for this field, so it is important to check the GED_RETURN_VALUE result.
     */
    inline SignedDNum GetSignedField(const /* GED_INS_FIELD */ uint32_t field, GED_RETURN_VALUE& ret)
        { SignedDNum num = { GetField<int32_t>(field, ret) }; return num; }


    /*!
     * Generic function for retrieving the value of an unsigned instruction field of size up to 32 bits. If the field is not valid for
     * this instruction, the uint32_t equivalent of -1 is returned (0xFFFFFFFF).
     *
     * @param[in]   field   Id of the requested field. For precompiled models, these are the possible values of GED_INS_FIELD.
     *                        For custom models, these are Ids assigned by the model parser.
     * @param[out]  ret     The function stores a GED_RETURN_VALUE indicating success or decoding error.
     *
     * @return      On success (the field is valid), the function returns the requested field's value, 0xFFFFFFFF otherwise.
     *
     * @note        Although unlikely, 0xFFFFFFFF may be a valid value for this field, so it is important to check the
     *                GED_RETURN_VALUE result.
     */
    inline uint32_t GetUnsignedField(const /* GED_INS_FIELD */ uint32_t field, GED_RETURN_VALUE& ret)
        { uint32_t num = GetField<uint32_t>(field, ret); return num; }


    /*!
     * Generic function for retrieving the value of a signed instruction field of size up to 64 bits. If the field size is less than
     * the size of int64_t, the returned value is sign extended. If the field is not valid for this instruction, -1 is returned.
     *
     * @param[in]   field   Id of the requested field. For precompiled models, these are the possible values of GED_INS_FIELD.
     *                        For custom models, these are Ids assigned by the model parser.
     * @param[out]  ret     The function stores a GED_RETURN_VALUE indicating success or decoding error.
     *
     * @return      On success (the field is valid), the function returns the requested field's value, -1 otherwise.
     *
     * @note        -1 may be a valid value for this field, so it is important to check the GED_RETURN_VALUE result.
     */
    inline SignedQNum GetSigned64Field(const /* GED_INS_FIELD */ uint32_t field, GED_RETURN_VALUE& ret)
        { SignedQNum num = { GetField<int64_t>(field, ret) }; return num; }


    /*!
     * Generic function for retrieving the value of an unsigned instruction field of size up to 64 bits. If the field is not valid for
     * this instruction, the uint64_t equivalent of -1 is returned (0xFFFFFFFFFFFFFFFF).
     *
     * @param[in]   field   Id of the requested field. For precompiled models, these are the possible values of GED_INS_FIELD.
     *                        For custom models, these are Ids assigned by the model parser.
     * @param[out]  ret     The function stores a GED_RETURN_VALUE indicating success or decoding error.
     *
     * @return      On success (the field is valid), the function returns the requested field's value, 0xFFFFFFFFFFFFFFFF otherwise.
     *
     * @note        Although unlikely, 0xFFFFFFFFFFFFFFFF may be a valid value for this field, so it is important to check the
     *                GED_RETURN_VALUE result.
     */
    inline uint64_t GetUnsigned64Field(const /* GED_INS_FIELD */ uint32_t field, GED_RETURN_VALUE& ret)
        { uint64_t num = GetField<uint64_t>(field, ret); return num; }


    /*!
     * In some cases, it may be useful to get the raw encoded value of the given field in the instruction bytes. This value is always
     * an unsigned value. It is not padded or sign extended, and is not translated to an enumeration.
     *
     * @param[in]   field   Id of the requested field. For precompiled models, these are the possible values of GED_INS_FIELD.
     *                        For custom models, these are Ids assigned by the model parser.
     * @param[out]  ret     The function stores a GED_RETURN_VALUE indicating success or decoding error.
     *
     * @return      On success (the field is valid), the function returns the requested field's value, 0xFFFFFFFF otherwise.
     *
     * @note        Although unlikely, 0xFFFFFFFF may be a valid value for this field, so it is important to check the
     *                GED_RETURN_VALUE result.
     */
    inline uint32_t GetRawField(const /* GED_INS_FIELD */ uint32_t field, GED_RETURN_VALUE& ret) const;


    /*!
    * In some cases, it may be useful to get the raw encoded value of the given field in the instruction bytes. This value is always
    * an unsigned value. It is not padded or sign extended, and is not translated to an enumeration.
    *
    * @param[in]   field   Id of the requested field. For precompiled models, these are the possible values of GED_INS_FIELD.
    *                        For custom models, these are Ids assigned by the model parser.
    * @param[out]  ret     The function stores a GED_RETURN_VALUE indicating success or decoding error.
    *
    * @return      On success (the field is valid), the function returns the requested field's value, 0xFFFFFFFF otherwise.
    *
    * @note        Although unlikely, 0xFFFFFFFFFFFFFFFF may be a valid value for this field, so it is important to check the
    *                GED_RETURN_VALUE result.
    */
    inline uint64_t GetRawQuadField(const /* GED_INS_FIELD */ uint32_t field, GED_RETURN_VALUE& ret) const;


    /*!
     * Set the instruction opcode.
     *
     * @param[in]   opcode  The enumerator representing the new opcode.
     *
     * @return      GED_RETURN_VALUE indicating success or encoding error.
     *
     * @note        This function is predefined and does not depend on the active model.
     */
    GED_RETURN_VALUE SetOpcode(/* GED_OPCODE */ uint32_t opcode);


    /*!
     * Generic function for modifying the value of a signed instruction field of size up to 32 bits.
     *
     * @param[in]   field   Id of the requested field. For precompiled models, these are the possible values of GED_INS_FIELD.
     *                        For custom models, these are Ids assigned by the model parser.
     * @param[in]   val     The value to store in the given field.
     *
     * @return      GED_RETURN_VALUE indicating success or encoding error.
     */
    inline GED_RETURN_VALUE SetSignedField(const /* GED_INS_FIELD */ uint32_t field, const int32_t val)
        { GED_RETURN_VALUE ret = GED_RETURN_VALUE_SUCCESS; ret = SetField(field, val); return ret; }


    /*!
     * Generic function for modifying the value of an unsigned instruction field of size up to 32 bits.
     *
     * @param[in]   field   Id of the requested field. For precompiled models, these are the possible values of GED_INS_FIELD.
     *                        For custom models, these are Ids assigned by the model parser.
     * @param[in]   val     The value to store in the given field.
     *
     * @return      GED_RETURN_VALUE indicating success or encoding error.
     */
    inline GED_RETURN_VALUE SetUnsignedField(const /* GED_INS_FIELD */ uint32_t field, const uint32_t val)
        { GED_RETURN_VALUE ret = GED_RETURN_VALUE_SUCCESS; ret = SetField(field, val); return ret; }


    /*!
     * Generic function for modifying the value of a signed instruction field of size up to 64 bits.
     *
     * @param[in]   field   Id of the requested field. For precompiled models, these are the possible values of GED_INS_FIELD.
     *                        For custom models, these are Ids assigned by the model parser.
     * @param[in]   val     The value to store in the given field.
     *
     * @return      GED_RETURN_VALUE indicating success or encoding error.
     */
    inline GED_RETURN_VALUE SetSigned64Field(const /* GED_INS_FIELD */ uint32_t field, const int64_t val)
        { GED_RETURN_VALUE ret = GED_RETURN_VALUE_SUCCESS; ret = SetField(field, val); return ret; }


    /*!
     * Generic function for modifying the value of an unsigned instruction field of size up to 64 bits.
     *
     * @param[in]   field   Id of the requested field. For precompiled models, these are the possible values of GED_INS_FIELD.
     *                        For custom models, these are Ids assigned by the model parser.
     * @param[in]   val     The value to store in the given field.
     *
     * @return      GED_RETURN_VALUE indicating success or encoding error.
     */
    inline GED_RETURN_VALUE SetUnsigned64Field(const /* GED_INS_FIELD */ uint32_t field, const uint64_t val)
        { GED_RETURN_VALUE ret = GED_RETURN_VALUE_SUCCESS; ret = SetField(field, val); return ret; }


# if GED_EXPERIMENTAL
    /*!
    * Set specific raw bits in native instruction, ignoring restrictions and encoding masks.
    *
    * @param[in]   low      First bit of the area in the instruction asked to modify
    * @param[in]   high     Last bit of the area in the instruction asked to modify
    * @param[in]   val      The value to store in the given range.
    *
    * @return      GED_RETURN_VALUE indicating success or encoding error.
    *
    * @note        Calling this function prevents the application of encoding masks when calling Encode().
    */
    GED_RETURN_VALUE SetRawBits(const uint8_t low, const uint8_t high, const uint64_t val);
# endif // GED_EXPERIMENTAL

public:
    // BLOCKED API FUNCTIONS

    // We expect the GEDIns class to be supplied by the user. However, the user allocates a POD struct and we reinterpret_cast it
    // to GEDIns. To enforce this usage model, the constructors, destructor and assignments operator are deleted.
    GEDIns() = delete;                          // default constructor
    GEDIns(const GEDIns&) = delete;             // copy constructor
    GEDIns& operator=(const GEDIns&) = delete;  // copy-assign
    ~GEDIns() = delete;                         // destructor
    GEDIns(GEDIns&&) = delete;                  // move constructor
    GEDIns& operator=(GEDIns&&) = delete;       // move-assign

protected:
    // PROTECTED MEMBER FUNCTIONS

    inline const ModelData& GetCurrentModelData() const { GEDASSERT(_modelId < numOfSupportedModels); return ModelsArray[_modelId]; }


    /*!
    * Get the field's size in bits. If the field depends on other fields, and may have different sizes, the maximal size is
    * returned.
    *
    * @return      The field's size in bits.
    */
    uint8_t GetFieldWidth(const uint16_t field, const bool interpField = false) const;


    /*!
     * Get a string representation of the instruction's bytes.
     *
     * @return      The requested string.
     */
    string GetInstructionBytes() const;

private:
    // PRIVATE MEMBER FUNCTIONS

    // Functions for handling predefined fields
    // Any change to the following functions should be complemented by a corresponding change to the relevant decoding entries defined
    // in ged_predefined_fields.cpp. These entries are hardcoded and the data therein must match the data in the functions below.
    inline void ExtractOpcode(const unsigned char* bytes) { _opcode = uint32_t(bytes[0] & uint8_t(0x7f)); } // extract bits 0-6
    inline void ExtractOpcode() { ExtractOpcode(_nativeBytes); }
    inline void ExtractCmptCtrl(const unsigned char* bytes) // extract bit 29
    {
        const uint32_t* rawBytes = reinterpret_cast<const uint32_t*>(bytes);
        if (0 != (rawBytes[0] & (uint32_t)0x20000000)) _status |= GED_INS_STATUS_COMPACT_VALID ;
    }
    inline void ExtractCmptCtrl() { ExtractCmptCtrl(_nativeBytes); }
    inline void SetNativeOpcode()
    {
        uint32_t* nativeBytes = reinterpret_cast<uint32_t*>(_nativeBytes);
        (nativeBytes[0] &= 0xffffff80) |= _opcode;
    }
    inline void SetCompactOpcode()
    {
        uint32_t* compactBytes = reinterpret_cast<uint32_t*>(_compactBytes);
        (compactBytes[0] &= 0xffffff80) |= _opcode;
    }
    inline void SetNonCompact() // TODO: Remove this when implementing Mantis 3732
    {
        uint32_t* nativeBytes = reinterpret_cast<uint32_t*>(_nativeBytes);
        nativeBytes[0] &= (uint32_t)0xdfffffff;
    }
    inline void SetCompact(unsigned char* compactBytes) // TODO: Remove this when implementing Mantis 3732
    {
        *(reinterpret_cast<uint32_t*>(compactBytes)) |= (uint32_t)compactInsInitializer;
    }

    // Status utility functions
    inline bool IsNativeValid() const { return (0 != (_status & (uint8_t)GED_INS_STATUS_NATIVE_VALID)); }
    inline bool IsCompactValid() const { return (0 != (_status & (uint8_t)GED_INS_STATUS_COMPACT_VALID)); }
    inline bool IsValid() const { return (0 != (_status & (uint8_t)GED_INS_STATUS_VALID)); }
    inline bool IsNativeEncoded() const { return(0 != (_status & (uint8_t)GED_INS_STATUS_NATIVE_ENCODED)); }
    inline bool IsCompactEncoded() const { return(0 != (_status & (uint8_t)GED_INS_STATUS_COMPACT_ENCODED)); }
    inline bool IsEncoded() const { return (0 != (_status & (uint8_t)GED_INS_STATUS_ENCODED)); }
    inline void ClearStatus() { _status = (uint8_t)GED_INS_STATUS_CLEAR; }
    inline void SetNativeValid() { _status |= (uint8_t)GED_INS_STATUS_NATIVE_VALID; }
    inline void SetCompactValid() { _status |= (uint8_t)GED_INS_STATUS_COMPACT_VALID; }
    inline void SetNativeEncoded() { _status |= (uint8_t)GED_INS_STATUS_NATIVE_ENCODED; }
    inline void SetCompactEncoded() { _status |= (uint8_t)GED_INS_STATUS_COMPACT_ENCODED; }
    inline void SetNativeNotValid() { _status &= (uint8_t)(~GED_INS_STATUS_NATIVE_VALID); }
    inline void SetCompactNotValid() { _status &= (uint8_t)(~GED_INS_STATUS_COMPACT_VALID); }
    inline void SetNativeNotEncoded() { _status &= (uint8_t)(~GED_INS_STATUS_NATIVE_ENCODED); }
    inline void SetCompactNotEncoded() { _status &= (uint8_t)(~GED_INS_STATUS_COMPACT_ENCODED); }
    inline void SetNotEncoded() { _status &= (uint8_t)(~GED_INS_STATUS_ENCODED); }
# if GED_EXPERIMENTAL
    inline void SetEncodingMasksDisabled() { _status |= (uint8_t)(GED_INS_STATUS_RAW_BIT_SET); }
    inline void SetEncodingMasksEnabled() { _status &= (uint8_t)(~GED_INS_STATUS_RAW_BIT_SET); }
    inline bool ShouldApplyEncodingMasks() const { return (0 == (_status & (uint8_t)GED_INS_STATUS_RAW_BIT_SET)); }
# endif // GED_EXPERIMENTAL


    // General utility functions
    inline void SignExtend(int32_t& val, const ged_ins_field_entry_t* dataEntry) const;
    inline void SignExtend(int64_t& val, const ged_ins_field_entry_t* dataEntry) const;
    inline void SignExtend(uint32_t& val, const ged_ins_field_entry_t* dataEntry) const;
    inline void SignExtend(uint64_t& val, const ged_ins_field_entry_t* dataEntry) const;
    void SetInstructionBytes(unsigned char* dst, const unsigned char* src, unsigned int size, const unsigned int maxSize) const;

    // Internal decoding functions
    template <typename NumType>
    NumType GetField(const /* GED_INS_FIELD */ uint32_t field, GED_RETURN_VALUE& ret);

    template <typename NumType>
    NumType GetField(const unsigned char* bytes, const ged_ins_decoding_table_t table, /* GED_INS_FIELD */ const uint32_t field,
                     const GED_VALUE_TYPE valueType, GED_RETURN_VALUE& ret) const;

    uint32_t GetMappedField(const /* GED_INS_FIELD */ uint32_t field, const unsigned char* validBits, bool& extracted) const;

    // Decoding utility functions
    inline const ged_ins_field_entry_t* GetInstructionDataEntry(ged_ins_decoding_table_t table,
                                                                /* GED_INS_FIELD */ uint32_t tableIndex) const;
    const ged_ins_field_entry_t* GetDependentInstructionDataEntry(ged_ins_decoding_table_t table,
                                                                  /* GED_INS_FIELD */ uint32_t tableIndex) const;
    inline const ged_ins_field_entry_t* GetMappedInstructionDataEntry(ged_ins_decoding_table_t table,
                                                                      /* GED_INS_FIELD */ uint32_t tableIndex,
                                                                      const unsigned char* validBits, bool& extracted) const;
    const ged_ins_field_entry_t* GetDependentMappedInstructionDataEntry(ged_ins_decoding_table_t table,
                                                                        /* GED_INS_FIELD */ uint32_t tableIndex,
                                                                        const unsigned char* validBits, bool& extracted) const;

    inline uint32_t ExtractConsecutiveEntryValue(const unsigned char* bytes, const ged_ins_field_position_fragment_t& pos) const;

    template<typename NumType>
    NumType ExtractFragmentedEntryValue(const unsigned char* bytes, const ged_ins_field_entry_t* dataEntry) const;

    template<typename NumType>
    inline NumType ExtractFragmentValue(const unsigned char* bytes, const ged_ins_field_position_fragment_t& pos) const;

    inline bool IsVariableField(const ged_ins_field_entry_t* dataEntry) const;
    inline bool IsSignedVariableField(const ged_ins_field_entry_t* dataEntry) const;

    // Mapping utility functions
    GED_RETURN_VALUE BuildNativeInsFromCompact();
    bool MapCurrentField(const ged_ins_decoding_table_t compactTable, const ged_compact_mapping_table_t mappingTable,
                         const /* GED_INS_FIELD */ uint32_t field, unsigned char* validBits);
    inline const ged_compact_mapping_entry_t* GetCompactionMappingEntry(ged_compact_mapping_table_t table,
                                                                        /* GED_INS_FIELD */ uint32_t tableIndex,
                                                                        const unsigned char* validBits) const;
    const ged_compact_mapping_entry_t* GetDependentCompactionMappingEntry(ged_compact_mapping_table_t table,
                                                                          /* GED_INS_FIELD */ uint32_t tableIndex,
                                                                          const unsigned char* validBits) const;
    void MapRawBytes(const uint32_t src, const ged_ins_field_position_fragment_t* to, const uint32_t fromMask,
                     unsigned char* validBits);
    void MapRawBytes(const uint32_t src, const uint32_t numOfFragments, const ged_compact_mapping_fragment_t* fragments,
                     unsigned char* validBits);
    void MapRawBytes(const uint64_t src, const uint32_t numOfFragments, const ged_compact_mapping_fragment_t* fragments,
                     unsigned char* validBits);
    void MapReppedValue(const uint32_t src, const ged_ins_field_position_fragment_t* to,
                        const ged_ins_field_position_fragment_t* from, unsigned char* validBits);
    void MapOneToOneValue(const uint32_t src, const ged_ins_field_position_fragment_t* to,
                          const ged_ins_field_position_fragment_t* from, unsigned char* validBits);
    void MapFixedValue(const uint32_t value, const ged_ins_field_position_fragment_t* to, unsigned char* validBits);
    inline void SetMappedBits(const uint8_t dwordIndex, const uint32_t mask, const uint32_t value, unsigned char* validBits);

    void EmitMappingCyclicDependencyError(const set<uint32_t>& unMapped, const unsigned char* validBits) const;


    // Internal encoding functions
    template<typename NumType>
    GED_RETURN_VALUE SetField(const /* GED_INS_FIELD */ uint32_t field, const NumType val);

    template<typename NumType>
    GED_RETURN_VALUE SetField(unsigned char* bytes, const ged_ins_decoding_table_t table, const /* GED_INS_FIELD */ uint32_t field,
                              const GED_VALUE_TYPE valueType, NumType val) const;

    // Encoding utility functions
    bool BuildCompactInsFromNative();
# if GED_VALIDATION_API
    bool CountCompactFormats(unsigned int& count);
    bool CountCurrentField(const ged_ins_decoding_table_t compactTable, const ged_compact_mapping_table_t mappingTable,
                           const unsigned char* const orMask, const /* GED_INS_FIELD */ uint32_t field, unsigned int& count);
    bool CountCompactionTableEntry(uint64_t& val, const uint64_t& valMask, const uint32_t tableSize,
                                   ged_compaction_table_t table, unsigned int& count) const;
    bool BuildAllCompactedFormats(unsigned char* compactBytesArray, const unsigned size);
    bool CollectCurrentValueField(const ged_ins_decoding_table_t compactTable, const ged_compact_mapping_table_t mappingTable,
                                  const /* GED_INS_FIELD */ uint32_t field, unsigned char* buf);
    vector<vector<unsigned char> > CollectCurrentMappedFields(const ged_ins_decoding_table_t compactTable,
            const ged_compact_mapping_table_t mappingTable,
            const unsigned char* const orMask,
            /* GED_INS_FIELD */ uint32_t field, bool& succeeded);
# endif // GED_VALIDATION_API
    bool CollectCurrentField(const ged_ins_decoding_table_t compactTable, const ged_compact_mapping_table_t mappingTable,
                             const unsigned char* const orMask, const /* GED_INS_FIELD */ uint32_t field);

    bool CollectFragmentedEntryDWValue(uint32_t& val, const unsigned char* bytes, const ged_compact_mapping_entry_t* mappingEntry) const;
    bool CollectFragmentedEntryQWValue(uint64_t& val, const unsigned char* bytes, const ged_compact_mapping_entry_t* mappingEntry) const;
    bool CollectFragmentValue(uint32_t& val, const unsigned char* bytes, const ged_compact_mapping_fragment_t& mapping) const;
    bool FindCompactionTableEntry(uint64_t& val, const uint64_t& valMask, const uint32_t tableSize,
                                  ged_compaction_table_t table) const;
    void ApplyNativeEncodingMasks();
    void ApplyCompactEncodingMasks(unsigned char* compactBytes);
    void BuildNativeOrMask(unsigned char* orMask) const;

    template<typename NumType>
    void SetFragment(unsigned char* bytes, const ged_ins_field_position_fragment_t& fragment, NumType fullVal) const;

    // String utilities
    string GetInstructionBytes(const unsigned char* instructionBytes, int dwords) const;
# if GED_VALIDATION_API
    bool RecordPadding(vector<ged_ins_field_mapping_fragment_t> &mappingFragments, const ged_ins_field_entry_t* dataEntry) const;
# endif // GED_VALIDATION_API
    bool RecordPosition(vector<ged_ins_field_mapping_fragment_t> &mappingFragments, const ged_ins_field_entry_t* dataEntry) const;
    void RecordSingleFragment(vector<ged_ins_field_mapping_fragment_t> &mappingFragments,
                              const ged_ins_field_position_fragment_t &position) const;
    void MergeFragments(vector<ged_ins_field_mapping_fragment_t> &mappingFragments) const;

private:
    // PRIVATE STATIC DATA MEMBERS

    // The table below is used for sign-extending signed fields. Since instruction fields are currently limited to 64 bits, we only
    // need 64 entries (for the 64 possible locations of the field's MSB). The field is expected to be extracted from the instruction
    // bytes prior to performing the sign-extension check and apply operations.
    static const int64_t signExtendTable[GED_QWORD_BITS];

    // This is the predefined enumerator (index) of the opcode field in the GED_INS_FIELD autogenerated enumeration.
    static const unsigned int opcodeFieldEnumId{ 0 };

    // Initialization constants.
    static const uint64_t compactInsInitializer{ 0x20000000 }; // TODO: Remove this when implementing Mantis 3732
    static const uint8_t invalidOpcode{ MAX_UINT8_T };

private:
    // PRIVATE DATA MEMBERS

    unsigned char _nativeBytes[GED_NATIVE_INS_SIZE];
    unsigned char _compactBytes[GED_COMPACT_INS_SIZE];

    uint8_t _opcode;

    // Indicates the instruction's status:
    // When a new instruction is assigned, it is initially stored in the _nativeBytes array. If the compact bit is set, the instruction
    // is immediately expanded into its native format, in which case _status will show that both formats are valid.
    /* GED_INS_STATUS */ uint8_t _status;

    /* GED_MODEL */ uint8_t _modelId;

    ged_ins_decoding_table_t _decodingTable;
};


/*************************************************************************************************
 * class GEDIns API functions
 *************************************************************************************************/

uint32_t GEDIns::GetRawField(const /* GED_INS_FIELD */ uint32_t field, GED_RETURN_VALUE& ret) const
{
    GEDASSERT(!MAY_BE_QWORD(fieldTypesByField[field]));
    uint32_t num = GetField<uint32_t>(_nativeBytes, _decodingTable, field, GED_VALUE_TYPE_ENCODED, ret);
    return num;
}

uint64_t GEDIns::GetRawQuadField(const /* GED_INS_FIELD */ uint32_t field, GED_RETURN_VALUE& ret) const
{
    uint64_t num = GetField<uint64_t>(_nativeBytes, _decodingTable, field, GED_VALUE_TYPE_ENCODED, ret);
    return num;
}


/*************************************************************************************************
 * class GEDIns private member functions
 *************************************************************************************************/

void GEDIns::SignExtend(int32_t& val, const ged_ins_field_entry_t* dataEntry) const
{
#if defined(GED_VALIDATE)
    if (NULL != dataEntry->_restrictions)
    {
        // Make sure this isn't a variable field since variable fields must use unsigned getters. The variable-field modifier must be
        // first in the restrictions table if it exists.
        GEDASSERT(GED_FIELD_RESTRICTIONS_TYPE_FIELD_TYPE != dataEntry->_restrictions[0]->_restrictionType);
    }
#endif // GED_VALIDATE
    const uint8_t highBit = dataEntry->_bitSize - 1;
    if (GEDRestrictionsHandler::IsNegative(val, highBit))
    {
        val |= (int32_t)signExtendTable[highBit]; // sign extend
    }
}


void GEDIns::SignExtend(int64_t& val, const ged_ins_field_entry_t* dataEntry) const
{
#if defined(GED_VALIDATE)
    if (NULL != dataEntry->_restrictions)
    {
        // Make sure this isn't a variable field since variable fields must use unsigned getters. The variable-field modifier must be
        // first in the restrictions table if it exists.
        GEDASSERT(GED_FIELD_RESTRICTIONS_TYPE_FIELD_TYPE != dataEntry->_restrictions[0]->_restrictionType);
    }
#endif // GED_VALIDATE
    const uint8_t highBit = dataEntry->_bitSize - 1;
    if (GEDRestrictionsHandler::IsNegative(val, highBit))
    {
        val |= signExtendTable[highBit]; // sign extend
    }
}


void GEDIns::SignExtend(uint32_t& val, const ged_ins_field_entry_t* dataEntry) const
{
    if (IsVariableField(dataEntry) && IsSignedVariableField(dataEntry))
    {
        // Apply sign extension if the field's value is negative.
        const uint8_t highBit = dataEntry->_restrictions[0]->_fieldType._attr._bits - 1;
        if (GEDRestrictionsHandler::IsNegative(val, highBit))
        {
            val |= (uint32_t)signExtendTable[highBit]; // sign extend
        }
    }
}


void GEDIns::SignExtend(uint64_t& val, const ged_ins_field_entry_t* dataEntry) const
{
    if (IsVariableField(dataEntry) && IsSignedVariableField(dataEntry))
    {
        // Apply sign extension if the field's value is negative.
        const uint8_t highBit = dataEntry->_restrictions[0]->_fieldType._attr._bits - 1;
        if (GEDRestrictionsHandler::IsNegative(val, highBit))
        {
            val |= (uint64_t)signExtendTable[highBit]; // sign extend
        }
    }
}


template <typename NumType>
NumType GEDIns::GetField(const /* GED_INS_FIELD */ uint32_t field, GED_RETURN_VALUE& ret)
{
    ret = GED_RETURN_VALUE_INVALID_FIELD;
    if (GetCurrentModelData().numberOfInstructionFields <= field)
    {
        return (NumType) - 1;
    }

    // Try to use the native format first. It may not be valid if we are in the middle of encoding a compact instruction, in which
    // case expand the instruction first.
    if (!IsNativeValid())
    {
        GEDASSERT(IsCompactValid());
        BuildNativeInsFromCompact(); // Should always succeed, however this is not true, see Mantis 3755
    }
    GEDASSERT(IsNativeValid());
    NumType fieldValue = GetField<NumType>(_nativeBytes, _decodingTable, field, GED_VALUE_TYPE_PROCESSED, ret);

    // If the field was not found and this is a compact instruction, it is possible that this field is only valid in the compact
    // instruction format, so try it now.
    if (GED_RETURN_VALUE_INVALID_FIELD == ret && IsCompactValid())
    {
        GEDASSERT(NULL != GetCurrentModelData().opcodeTables[_opcode].compactDecoding);
        fieldValue = GetField<NumType>(_compactBytes, GetCurrentModelData().opcodeTables[_opcode].compactDecoding, field,
                                       GED_VALUE_TYPE_PROCESSED, ret);
    }
    return fieldValue;
}


template <typename NumType>
NumType GEDIns::GetField(const unsigned char* bytes, const ged_ins_decoding_table_t table, const /* GED_INS_FIELD */ uint32_t field,
                         const GED_VALUE_TYPE valueType, GED_RETURN_VALUE& ret) const
{
    GEDASSERT(NULL != bytes);
    GEDASSERT(NULL != table);
    GEDASSERT(field < GetCurrentModelData().numberOfInstructionFields);
    GEDASSERT(field == table[field]._field);
    ret = GED_RETURN_VALUE_INVALID_FIELD; // initialize the status return value

    // Traverse the intermediate tables (if necessary).
    const ged_ins_field_entry_t* dataEntry = GetInstructionDataEntry(table, field);
    if (NULL == dataEntry)
    {
        return (NumType) - 1;
    }

    // Now that we are at the bottommost table, extract the actual data.
    NumType val = (NumType) - 1;
    switch (dataEntry->_entryType)
    {
    case GED_TABLE_ENTRY_TYPE_CONSECUTIVE:
        // Consecutive entries are limited to one dword, but NumType may be a 64-bit type so verify the bit size of the field.
        GEDASSERT(dataEntry->_bitSize <= GED_DWORD_BITS);
        val = (NumType)ExtractConsecutiveEntryValue(bytes, dataEntry->_consecutive._position);
        break;
    case GED_TABLE_ENTRY_TYPE_FRAGMENTED:
        val = ExtractFragmentedEntryValue<NumType>(bytes, dataEntry);
        break;
    case GED_TABLE_ENTRY_TYPE_FIXED_VALUE:
        GEDASSERT(dataEntry->_bitSize <= GED_DWORD_BITS);
        val = dataEntry->_fixed._value;
        break;
    default:
        GEDASSERT(0);
    }
    ret = GED_RETURN_VALUE_SUCCESS;

    // When using the field's encoded value as an index, we need the raw encoded value without any modifications. This includes
    // paddings, sign extension and enumeration.
    if (GED_VALUE_TYPE_ENCODED == valueType) return val;

    // Sign extend if necessary.
    SignExtend(val, dataEntry);

    return GEDRestrictionsHandler::HandleDecodingRestrictions(dataEntry, val, ret);
}


const ged_ins_field_entry_t* GEDIns::GetInstructionDataEntry(ged_ins_decoding_table_t table,
                                                             /* GED_INS_FIELD */ uint32_t tableIndex) const
{
    GEDASSERT(NULL != table);
    if (table[tableIndex]._entryType <= GED_TABLE_ENTRY_TYPE_LAST_EXPLICIT) return &table[tableIndex];
    return GetDependentInstructionDataEntry(table, tableIndex);
}


const ged_ins_field_entry_t* GEDIns::GetMappedInstructionDataEntry(ged_ins_decoding_table_t table,
                                                                   /* GED_INS_FIELD */ uint32_t tableIndex,
                                                                   const unsigned char* validBits, bool& extracted) const
{
    GEDASSERT(NULL != table);
    if (table[tableIndex]._entryType <= GED_TABLE_ENTRY_TYPE_LAST_EXPLICIT)
    {
        extracted = true;
        return &table[tableIndex];
    }
    return GetDependentMappedInstructionDataEntry(table, tableIndex, validBits, extracted);
}


uint32_t GEDIns::ExtractConsecutiveEntryValue(const unsigned char* bytes, const ged_ins_field_position_fragment_t& pos) const
{
    GEDASSERT(NULL != bytes);
    uint32_t val = ((uint32_t*)bytes)[pos._dwordIndex];
    val &= pos._bitMask;
    val >>= pos._shift;
    return val;
}


template<typename NumType>
NumType GEDIns::ExtractFragmentedEntryValue(const unsigned char* bytes, const ged_ins_field_entry_t* dataEntry) const
{
    GEDASSERT(NULL != bytes);
    GEDASSERT(NULL != dataEntry);
    GEDASSERT(GED_TABLE_ENTRY_TYPE_FRAGMENTED == dataEntry->_entryType);
    GEDASSERT(dataEntry->_fragmented._numOfPositionFragments > 1);

    // The field is fragmented, so all fragments will eventually be gathered in fullVal.
    NumType fullVal = 0;
    for (unsigned int i = 0; i < dataEntry->_fragmented._numOfPositionFragments; ++i)
    {
        NumType fragmentVal = ExtractFragmentValue<NumType>(bytes, dataEntry->_fragmented._fragments[i]);
        fullVal |= fragmentVal;
    }
    return fullVal;
}


template<typename NumType>
NumType GEDIns::ExtractFragmentValue(const unsigned char* bytes, const ged_ins_field_position_fragment_t& pos) const
{
    NumType val = ((uint32_t*)bytes)[pos._dwordIndex];
    val &= pos._bitMask;
    if (0 == val) return val; // early out - nothing to do
    if (pos._shift > 0)
    {
        val >>= pos._shift;
    }
    else if (pos._shift < 0)
    {
        val <<= abs(pos._shift);
    }
    return val;
}


bool GEDIns::IsVariableField(const ged_ins_field_entry_t* dataEntry) const
{
    return ((NULL != dataEntry->_restrictions) && // the field has modifiers/restrictions
            (GED_FIELD_RESTRICTIONS_TYPE_FIELD_TYPE == dataEntry->_restrictions[0]->_restrictionType)); // this is a variable field
}


bool GEDIns::IsSignedVariableField(const ged_ins_field_entry_t* dataEntry) const
{
    GEDASSERT(IsVariableField(dataEntry));
    return (dataEntry->_restrictions[0]->_fieldType._attr._signed); // the field is signed
}


const ged_compact_mapping_entry_t* GEDIns::GetCompactionMappingEntry(ged_compact_mapping_table_t table,
                                                                     /* GED_INS_FIELD */ uint32_t tableIndex,
                                                                     const unsigned char* validBits) const
{
    GEDASSERT(NULL != table);
    if (table[tableIndex]._entryType <= GED_MAPPING_TABLE_ENTRY_TYPE_LAST_EXPLICIT) return &table[tableIndex];
    return GetDependentCompactionMappingEntry(table, tableIndex, validBits);
}


void GEDIns::SetMappedBits(const uint8_t dwordIndex, const uint32_t mask, const uint32_t value, unsigned char* validBits)
{
    // GEDASSERT(0 == (((uint32_t*)_nativeBytes)[dwordIndex] & mask)); // prevents overwriting bits when uncompacting, see Mantis 4342
    GEDASSERT((value == ((((uint32_t*)_nativeBytes)[dwordIndex] & mask) | value)));
    ((uint32_t*)_nativeBytes)[dwordIndex] |= value;
    ((uint32_t*)validBits)[dwordIndex] &= ~mask;
}


template<typename NumType>
GED_RETURN_VALUE GEDIns::SetField(const /* GED_INS_FIELD */ uint32_t field, const NumType val)
{
    if (GetCurrentModelData().numberOfInstructionFields <= field) return GED_RETURN_VALUE_INVALID_FIELD;

    // We try to keep both the native and compact formats valid as long as we can. The following scenarios are possible:
    // 1. If both formats are valid:
    //    a. If the field was set in both formats, don't change the instruction's status.
    //    b. If the field was set in only one format, mark the other format invalid in the instruction's status.
    //    c. If the field was not set in both formats, the field is invalid, don't change the instruction's status.
    // 2. If only one format is valid, only attempt to set the field in that format.
    //    a. If the field was set, it is valid, don't change the instruction's status.
    //    b. If the field was not set, it is invalid, don't change the instruction's status.
    // 3. No format is valid - this is an internal error.

    GEDASSERT(IsValid()); // at least one format must be valid

    // Try to encode the field in the native format first.
    GED_RETURN_VALUE nativeRet = GED_RETURN_VALUE_INVALID_FIELD;
    if (IsNativeValid())
    {
        nativeRet = SetField(_nativeBytes, _decodingTable, field, GED_VALUE_TYPE_PROCESSED, val);
    }

    // Now try the compact format.
    GED_RETURN_VALUE compactRet = GED_RETURN_VALUE_INVALID_FIELD;
    if (IsCompactValid())
    {
        compactRet = SetField(_compactBytes, GetCurrentModelData().opcodeTables[_opcode].compactDecoding, field,
                              GED_VALUE_TYPE_PROCESSED, val);
        if (GED_RETURN_VALUE_SUCCESS == compactRet)
        {
            // The compact format was updated. Check if the native format was updated as well.
            if (GED_RETURN_VALUE_SUCCESS != nativeRet)
            {
                // The field was only updated in the the compact format, mark that the native format is no longer valid.
                SetNativeNotValid();
            }
        }
        else
        {
            if (GED_RETURN_VALUE_SUCCESS == nativeRet)
            {
                // The field was only updated in the native format, mark that the compact format is no longer valid.
                SetCompactNotValid();
            }
        }
    }

    GEDASSERT(IsValid()); // make sure that the instruction is still valid
    if (GED_RETURN_VALUE_SUCCESS == nativeRet || GED_RETURN_VALUE_SUCCESS == compactRet)
    {
        SetNotEncoded(); // the field was encoded, mark that the instruction needs to be re-encoded
        return GED_RETURN_VALUE_SUCCESS;
    }

    // The field was not encoded. Check why.
    if (GED_RETURN_VALUE_INVALID_VALUE == nativeRet || GED_RETURN_VALUE_INVALID_VALUE == compactRet)
    {
        return GED_RETURN_VALUE_INVALID_VALUE; // the field is valid, but the value is not
    }

    GEDASSERT(GED_RETURN_VALUE_INVALID_FIELD == nativeRet && GED_RETURN_VALUE_INVALID_FIELD == compactRet);
    return GED_RETURN_VALUE_INVALID_FIELD;
}


template<typename NumType>
GED_RETURN_VALUE GEDIns::SetField(unsigned char* bytes, const ged_ins_decoding_table_t table, const /* GED_INS_FIELD */ uint32_t field,
                                  const GED_VALUE_TYPE valueType, NumType val) const
{
    GEDASSERT(NULL != bytes);
    GEDASSERT(NULL != table);
    GEDASSERT(field < GetCurrentModelData().numberOfInstructionFields);
    GEDASSERT(field == table[field]._field);

    // Traverse the intermediate tables (if necessary).
    const ged_ins_field_entry_t* dataEntry = GetInstructionDataEntry(table, field);
    if (NULL == dataEntry) return GED_RETURN_VALUE_INVALID_FIELD; // the field is not valid for this instruction

    // Now that we are at the bottommost table, validate the new value and store it. Validate the field's value and handle any field
    // modifiers e.g. if this is an enumerated field, this function convert the enumerated value to the raw value that should be
    // encoded.
    if (!GEDRestrictionsHandler::HandleEncodingRestrictions(dataEntry, valueType, val)) return GED_RETURN_VALUE_INVALID_VALUE;

    // Now that we know the value is legal, we can encode it.
    if (GED_TABLE_ENTRY_TYPE_CONSECUTIVE == dataEntry->_entryType)
    {
        // Consecutive entries are limited to one dword, but NumType may be a 64-bit type so verify the bit size of the field.
        GEDASSERT(dataEntry->_bitSize <= GED_DWORD_BITS);

        // Shift the value into position.
        val <<= dataEntry->_consecutive._position._shift;

        // Apply the mask.
        val &= dataEntry->_consecutive._position._bitMask;

        // Clear the field in the destination.
        ((uint32_t*)bytes)[dataEntry->_consecutive._position._dwordIndex] &= ~dataEntry->_consecutive._position._bitMask;

        // Now set the field with the value itself.
        ((uint32_t*)bytes)[dataEntry->_consecutive._position._dwordIndex] |= val;
    }
    else if (GED_TABLE_ENTRY_TYPE_FRAGMENTED == dataEntry->_entryType)
    {
        for (unsigned int i = 0; i < dataEntry->_fragmented._numOfPositionFragments; ++i)
        {
            SetFragment(bytes, dataEntry->_fragmented._fragments[i], val);
        }
    }
    else if (GED_TABLE_ENTRY_TYPE_FIXED_VALUE == dataEntry->_entryType)
    {
        if (dataEntry->_fixed._value != static_cast<uint32_t>(val))
        {
            return GED_RETURN_VALUE_INVALID_VALUE;
        }
    }
    else
    {
        GEDASSERT(0);
    }

    return GED_RETURN_VALUE_SUCCESS;
}


template<typename NumType>
void GEDIns::SetFragment(unsigned char* bytes, const ged_ins_field_position_fragment_t& fragment, NumType val) const
{
    GEDASSERT(NULL != bytes);

    // Shift the value into position.
    if (fragment._shift > 0)
    {
        val <<= fragment._shift;
    }
    else if (fragment._shift < 0)
    {
        val >>= abs(fragment._shift);
    }

    // Apply the mask.
    val &= fragment._bitMask;

    // Clear the fragment in the destination.
    ((uint32_t*)bytes)[fragment._dwordIndex] &= ~fragment._bitMask;

    // Set the fragment value in the encoded bytes.
    ((uint32_t*)bytes)[fragment._dwordIndex] |= (uint32_t)val;
}

#endif // GED_INS_H
