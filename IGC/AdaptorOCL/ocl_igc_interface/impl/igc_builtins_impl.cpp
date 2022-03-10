/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ocl_igc_interface/igc_builtins.h"
#include "ocl_igc_interface/impl/igc_builtins_impl.h"

#include "cif/macros/enable.h"
#include "Probe/Assertion.h"

namespace IGC {

long CIF_GET_INTERFACE_CLASS(IgcBuiltins, 1)::DefaultGroupJointSortMemoryRequired(GroupSortMemoryScope::MemoryScope_t scope,
                                                                                  long totalItems,
                                                                                  long rangeSize,
                                                                                  long keyTypeSizeInBytes,
                                                                                  long valueTypeSizeInBytes) const
{
    const size_t bits_per_pass = 4;

    if (scope == GroupSortMemoryScope::workGroup) {
        return totalItems * (keyTypeSizeInBytes + valueTypeSizeInBytes) +
            rangeSize * (1 << bits_per_pass) * sizeof(uint32_t) +
            sizeof(uint32_t) +
            (valueTypeSizeInBytes > 0 ? sizeof(uint32_t) : 0);
    }
    else {
        return -1;
    }
}

long CIF_GET_INTERFACE_CLASS(IgcBuiltins, 1)::DefaultGroupPrivateSortMemoryRequired(GroupSortMemoryScope::MemoryScope_t scope,
                                                                                    long itemsPerWorkItem,
                                                                                    long rangeSize,
                                                                                    long keyTypeSizeInBytes,
                                                                                    long valueTypeSizeInBytes) const
{
    const size_t bits_per_pass = 4;

    if (scope == GroupSortMemoryScope::workGroup) {
        return rangeSize * itemsPerWorkItem * (keyTypeSizeInBytes + valueTypeSizeInBytes) +
            rangeSize * (1 << bits_per_pass) * sizeof(uint32_t) +
            sizeof(uint32_t) +
            (valueTypeSizeInBytes > 0 ? sizeof(uint32_t) : 0);
    }
    else if (scope == GroupSortMemoryScope::subGroup) {
        if ((itemsPerWorkItem > 1) ||
                ((rangeSize != 8) && (rangeSize != 16) && (rangeSize != 32))) {
            return -1;
        }
        return 0;
    }
    else {
        return -1;
    }
}

bool CIF_GET_INTERFACE_CLASS(IgcBuiltins, 1)::DefaultGroupSortSupported(GroupSortMemoryScope::MemoryScope_t scope,
                                                                        GroupSortKeyType::KeyType_t keyType,
                                                                        bool isKeyValue,
                                                                        bool isJointSort) const
{
    if (scope == GroupSortMemoryScope::subGroup &&
            keyType == GroupSortKeyType::half_key_type) {
        return false;
    }
    return true;
}

}

#include "cif/macros/disable.h"
