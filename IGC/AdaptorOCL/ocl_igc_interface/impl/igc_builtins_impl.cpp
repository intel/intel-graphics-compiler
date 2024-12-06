/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ocl_igc_interface/igc_builtins.h"
#include "ocl_igc_interface/impl/igc_builtins_impl.h"

#include "cif/macros/enable.h"
#include "Probe/Assertion.h"

namespace IGC {

static long getDataTypeSize(CIF_GET_INTERFACE_CLASS(IgcBuiltins, 1)::BuiltinDataType::DataType_t t)
{
    switch(t)
    {
    case CIF_GET_INTERFACE_CLASS(IgcBuiltins, 1)::BuiltinDataType::uint8_type:
    case CIF_GET_INTERFACE_CLASS(IgcBuiltins, 1)::BuiltinDataType::int8_type:
        return 8;
    case CIF_GET_INTERFACE_CLASS(IgcBuiltins, 1)::BuiltinDataType::uint16_type:
    case CIF_GET_INTERFACE_CLASS(IgcBuiltins, 1)::BuiltinDataType::int16_type:
    case CIF_GET_INTERFACE_CLASS(IgcBuiltins, 1)::BuiltinDataType::half_type:
        return 16;
    case CIF_GET_INTERFACE_CLASS(IgcBuiltins, 1)::BuiltinDataType::uint32_type:
    case CIF_GET_INTERFACE_CLASS(IgcBuiltins, 1)::BuiltinDataType::int32_type:
    case CIF_GET_INTERFACE_CLASS(IgcBuiltins, 1)::BuiltinDataType::float_type:
        return 32;
    case CIF_GET_INTERFACE_CLASS(IgcBuiltins, 1)::BuiltinDataType::uint64_type:
    case CIF_GET_INTERFACE_CLASS(IgcBuiltins, 1)::BuiltinDataType::int64_type:
    case CIF_GET_INTERFACE_CLASS(IgcBuiltins, 1)::BuiltinDataType::double_type:
        return 64;
    default:
        return 0;
    }
}

bool CIF_GET_INTERFACE_CLASS(IgcBuiltins, 1)::GetBuiltinMemoryRequired(IGCBuiltinMemoryInfo *memoryInfo,
                                                                       BuiltinAlgorithm::Algorithm_t algorithm,
                                                                       AlgorithmVariant_t variant,
                                                                       BuiltinMemoryScope::MemoryScope_t scope,
                                                                       long items,
                                                                       long rangeSize,
                                                                       BuiltinDataType::DataType_t keyType,
                                                                       long valueTypeSizeInBytes) const
{
    memoryInfo->scope = scope;
    memoryInfo->globalMemoryInBytes = -1;
    memoryInfo->sharedMemoryInBytes = -1;
    memoryInfo->canMemoryBeUsedConcurrently = false;

    long keyTypeSizeInBytes = getDataTypeSize(keyType);
    if (keyTypeSizeInBytes == 0)
        return false;

    switch(algorithm)
    {
    case BuiltinAlgorithm::sort:
    case BuiltinAlgorithm::clusteredSort:
    case BuiltinAlgorithm::clusteredSortedOrdinal:
    {
        const size_t bits_per_pass = 4;

        if ((algorithm == BuiltinAlgorithm::clusteredSort || algorithm == BuiltinAlgorithm::clusteredSortedOrdinal) &&
            (scope != BuiltinMemoryScope::subGroup))
        {
            return false;
        }

        if (scope == BuiltinMemoryScope::subGroup)
        {
            if ((variant != SortAlgorithmVariant::defaultPrivateSort) || (items > 1) ||
                    ((rangeSize != 8) && (rangeSize != 16) && (rangeSize != 32)) ||
                    valueTypeSizeInBytes != 0)
            {
                return false;
            }
            if (keyType == BuiltinDataType::half_type)
            {
                return false;
            }

            memoryInfo->globalMemoryInBytes = 0;
            memoryInfo->sharedMemoryInBytes = 0;
            return true;
        }
        else  // WorkGroup Sort
        {
            memoryInfo->sharedMemoryInBytes = 0;

            long totalItems = 0;
            long itemsPerWorkItem = 0;
            long mergeSortMemory = 0;

            long dataPairSizeInBytes = keyTypeSizeInBytes + valueTypeSizeInBytes;

            if (variant == SortAlgorithmVariant::defaultJointSort)
            {
                totalItems = items;
                itemsPerWorkItem = (totalItems - 1) / rangeSize + 1;

                mergeSortMemory =
                    totalItems * dataPairSizeInBytes +
                    sizeof(uint32_t) +
                    (valueTypeSizeInBytes > 0 ? sizeof(uint32_t) : 0);
            }
            else
            {
                totalItems = items * rangeSize;
                itemsPerWorkItem = items;

                mergeSortMemory =
                    2 * totalItems * dataPairSizeInBytes
                    + (valueTypeSizeInBytes > 0 ? 4 : 1) * sizeof(uint32_t);
            }

            long radixSortMemory =
                totalItems * dataPairSizeInBytes +
                rangeSize * (1 << bits_per_pass) * sizeof(uint32_t) +
                sizeof(uint32_t) +
                (valueTypeSizeInBytes > 0 ? sizeof(uint32_t) : 0);

            bool isRadix = (itemsPerWorkItem > 8 &&
                itemsPerWorkItem >= keyTypeSizeInBytes * 8);

            memoryInfo->globalMemoryInBytes = isRadix ? radixSortMemory : mergeSortMemory;
            return true;
        };
        break;
    }
    default:
        break;
    }

    return false;
}



}

#include "cif/macros/disable.h"
