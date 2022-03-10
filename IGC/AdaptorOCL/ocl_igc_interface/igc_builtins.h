/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cif/common/id.h"
#include "cif/common/cif.h"

#include "cif/macros/enable.h"

namespace IGC {

namespace GroupSortMemoryScope {
    using MemoryScope_t = uint64_t;
    using MemoryScopeCoder = CIF::Coder<MemoryScope_t>;

    constexpr auto workGroup = MemoryScopeCoder::Enc("WORK_GROUP");
    constexpr auto subGroup = MemoryScopeCoder::Enc("SUB_GROUP");
}

namespace GroupSortKeyType {
    using KeyType_t = uint64_t;
    using KeyTypeCoder = CIF::Coder<KeyType_t>;

    constexpr auto uint8_key_type = KeyTypeCoder::Enc("UINT8");
    constexpr auto uint16_key_type = KeyTypeCoder::Enc("UINT16");
    constexpr auto uint32_key_type = KeyTypeCoder::Enc("UINT32");
    constexpr auto uint64_key_type = KeyTypeCoder::Enc("UINT64");
    constexpr auto int8_key_type = KeyTypeCoder::Enc("INT8");
    constexpr auto int16_key_type = KeyTypeCoder::Enc("INT16");
    constexpr auto int32_key_type = KeyTypeCoder::Enc("INT32");
    constexpr auto int64_key_type = KeyTypeCoder::Enc("INT64");
    constexpr auto half_key_type = KeyTypeCoder::Enc("HALF");
    constexpr auto float_key_type = KeyTypeCoder::Enc("FLOAT");
    constexpr auto double_key_type = KeyTypeCoder::Enc("DOUBLE");
}

// Interface : IGC_BUILTINS
//             IGC builtin functions
// Interface for querying data about IGC builtin functions

CIF_DECLARE_INTERFACE(IgcBuiltins, "IGC_BUILTINS")

CIF_DEFINE_INTERFACE_VER(IgcBuiltins, 1) {
    CIF_INHERIT_CONSTRUCTOR();

    // Returns whether default work-group or sub-group sort is present in builtins
    virtual bool DefaultGroupSortSupported(GroupSortMemoryScope::MemoryScope_t scope,
                                           GroupSortKeyType::KeyType_t keyType,
                                           bool isKeyValue,
                                           bool isJointSort) const;

    // Returns required amount of memory for default joint work-group or sub-group sort
    // deviceib builtin function in bytes per workgroup (or sub-group), >= 0
    // or -1 if the algorithm for the specified parameters is not implemented
    //
    // totalItems -- number of elements to sort
    // rangeSize -- work-group or sub-group size respectively
    //
    // For key-only sort pass valueTypeSizeInBytes = 0
    virtual long DefaultGroupJointSortMemoryRequired(GroupSortMemoryScope::MemoryScope_t scope,
                                                     long totalItems,
                                                     long rangeSize,
                                                     long keyTypeSizeInBytes,
                                                     long valueTypeSizeInBytes) const;

    // Returns required amount of memory for default private memory work-group or sub-group sort
    // deviceib builtin function in bytes per workgroup (or sub-group), >= 0
    // or -1 if the algorithm for the specified parameters is not implemented
    //
    // itemsPerWorkItem -- number of elements in private array to sort
    // rangeSize -- work-group or sub-group size respectively
    //
    // For key-only sort pass valueTypeSizeInBytes = 0
    virtual long DefaultGroupPrivateSortMemoryRequired(GroupSortMemoryScope::MemoryScope_t scope,
                                                       long itemsPerWorkItem,
                                                       long rangeSize,
                                                       long keyTypeSizeInBytes,
                                                       long valueTypeSizeInBytes) const;
};

CIF_GENERATE_VERSIONS_LIST(IgcBuiltins);
CIF_MARK_LATEST_VERSION(IgcBuiltinsLatest, IgcBuiltins);

}

#include "cif/macros/disable.h"
