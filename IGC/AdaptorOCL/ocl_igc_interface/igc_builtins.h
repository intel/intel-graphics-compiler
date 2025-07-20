/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cif/common/id.h"
#include "cif/common/cif.h"

#include "cif/macros/enable.h"
#include "OCLAPI/oclapi.h"

namespace IGC {

// Interface : IGC_BUILTINS
//             IGC builtin functions
// Interface for querying data about IGC builtin functions

CIF_DECLARE_INTERFACE(IgcBuiltins, "IGC_BUILTINS")

CIF_DEFINE_INTERFACE_VER(IgcBuiltins, 1) {
  CIF_INHERIT_CONSTRUCTOR();

  typedef struct BuiltinAlgorithm {
    using Algorithm_t = uint64_t;
    using AlgorithmCoder = CIF::Coder<Algorithm_t>;

    static constexpr auto sort = AlgorithmCoder::Enc("SORT");
    static constexpr auto clusteredSort = AlgorithmCoder::Enc("CLUSTER_SORT");
    static constexpr auto clusteredSortedOrdinal = AlgorithmCoder::Enc("CSORTED_ORD");
  } BuiltinAlgorithm;

  typedef struct BuiltinMemoryType {
    using MemoryType_t = uint64_t;
    using MemoryTypeCoder = CIF::Coder<MemoryType_t>;

    static constexpr auto SLM = MemoryTypeCoder::Enc("SLM");
    static constexpr auto global = MemoryTypeCoder::Enc("GLOBAL");
  } BuiltinMemoryType;

  typedef struct BuiltinMemoryScope {
    using MemoryScope_t = uint64_t;
    using MemoryScopeCoder = CIF::Coder<MemoryScope_t>;

    static constexpr auto workGroup = MemoryScopeCoder::Enc("WORK_GROUP");
    static constexpr auto subGroup = MemoryScopeCoder::Enc("SUB_GROUP");
  } BuiltinMemoryScope;

  typedef struct BuiltinDataType {
    using DataType_t = uint64_t;
    using DataTypeCoder = CIF::Coder<DataType_t>;

    static constexpr auto uint8_type = DataTypeCoder::Enc("UINT8");
    static constexpr auto uint16_type = DataTypeCoder::Enc("UINT16");
    static constexpr auto uint32_type = DataTypeCoder::Enc("UINT32");
    static constexpr auto uint64_type = DataTypeCoder::Enc("UINT64");
    static constexpr auto int8_type = DataTypeCoder::Enc("INT8");
    static constexpr auto int16_type = DataTypeCoder::Enc("INT16");
    static constexpr auto int32_type = DataTypeCoder::Enc("INT32");
    static constexpr auto int64_type = DataTypeCoder::Enc("INT64");
    static constexpr auto half_type = DataTypeCoder::Enc("HALF");
    static constexpr auto float_type = DataTypeCoder::Enc("FLOAT");
    static constexpr auto double_type = DataTypeCoder::Enc("DOUBLE");
  } BuiltinDataType;

  using AlgorithmVariant_t = uint64_t;
  typedef struct SortAlgorithmVariant {
    using SortVariantCoder = CIF::Coder<AlgorithmVariant_t>;

    static constexpr auto defaultJointSort = SortVariantCoder::Enc("JOINT");
    static constexpr auto defaultPrivateSort = SortVariantCoder::Enc("PRIVATE");
  } SortAlgorithmVariant;

  typedef struct IGCBuiltinMemoryInfo {
    // Required amount of memory of a specified type (global or SLM)
    // for a particular algorithm in IGC builtins.
    long globalMemoryInBytes;
    long sharedMemoryInBytes;

    // The value is in bytes per scope (for example, if scope == WORK_GROUP,
    // then the value is per work-group), >=0.
    BuiltinMemoryScope::MemoryScope_t scope;

    // Whether the temporary memory, requested by algorithm in IGC builtins
    // can be used concurrently by different scopes.
    // For example, if scope == WORK_GROUP, "true" means the same pointer can be passed
    // to different work groups.
    bool canMemoryBeUsedConcurrently;
  } IGCBuiltinMemoryInfo;

  // Returns true if the requrested configuration is supported by IGC
  // Populates IGCBuiltinMemoryInfo struct that should be allocated by the caller
  OCL_API_CALL virtual bool GetBuiltinMemoryRequired(
      IGCBuiltinMemoryInfo * memoryInfo, BuiltinAlgorithm::Algorithm_t algorithm, AlgorithmVariant_t variant,
      BuiltinMemoryScope::MemoryScope_t scope, long items, long rangeSize, BuiltinDataType::DataType_t keyType,
      long valueTypeSizeInBytes) const;
};

CIF_GENERATE_VERSIONS_LIST(IgcBuiltins);
CIF_MARK_LATEST_VERSION(IgcBuiltinsLatest, IgcBuiltins);

} // namespace IGC

#include "cif/macros/disable.h"
