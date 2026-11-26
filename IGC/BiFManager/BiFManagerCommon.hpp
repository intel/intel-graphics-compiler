/*========================== begin_copyright_notice ============================

Copyright (C) 2024-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/ADT/StringSet.h>
#include "common/LLVMWarningsPop.hpp"

#include <map>

namespace IGC {
namespace BiFManager {
// Common typedefs
typedef std::vector<llvm::Function *> TFunctionsVec;
typedef int BiFSectionID;

// BiFDataRecord struct which contains the pointer on beginning of the
// section stream and size
struct BiFDataRecord {
  // Section ID
  BiFSectionID ID;
  // Explicit padding to initialize for reproducible builds
  int32_t padding;
  // Beginning of section in the stream
  int64_t bufferStart;
  // Size of the section in the stream
  int64_t bufferSize;

  BiFDataRecord(BiFSectionID ID, int64_t bufferStart, int64_t bufferSize) {
    this->ID = ID;
    this->padding = 0;
    this->bufferStart = bufferStart;
    this->bufferSize = bufferSize;
  }
};

class BiFManagerCommon {
public:
  BiFManagerCommon(llvm::LLVMContext &Context);
  ~BiFManagerCommon();

  BiFManagerCommon(const BiFManagerCommon &) = delete;
  BiFManagerCommon &operator=(const BiFManagerCommon &) = delete;

  // 64-bit FNV-1a, as name_hash() in BiFModule/depgraph.py
  static uint64_t getHash(llvm::StringRef Name);

protected:

  inline static const std::string bifMark = "igc_bif";

  llvm::LLVMContext &Context;
};
} // namespace BiFManager
} // namespace IGC
