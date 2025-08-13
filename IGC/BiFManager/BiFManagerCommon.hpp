/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

#include <map>

namespace IGC {
namespace BiFManager {
// Common typedefs
typedef std::vector<llvm::Function *> TFunctionsVec;
typedef int BiFSectionID;

/// <summary>
/// InstVisitor pass for looking built-in function which
/// are used in the module
/// </summary>
class CollectBuiltinsPass : public llvm::InstVisitor<CollectBuiltinsPass> {
private:
  TFunctionsVec &neededBuiltinsFunc;
  std::function<bool(llvm::Function *)> predicate;

public:
  CollectBuiltinsPass(TFunctionsVec &neededBuiltinsFunc, const std::function<bool(llvm::Function *)> &predicate);
  ~CollectBuiltinsPass();

  CollectBuiltinsPass(const CollectBuiltinsPass &) = delete;
  CollectBuiltinsPass &operator=(const CollectBuiltinsPass &) = delete;

  void visitCallInst(llvm::CallInst &callInst);
};

// BiFDataRecord struct which contains the pointer on beginning of the
// section stream and size
struct BiFDataRecord {
  // Section ID
  BiFSectionID ID;
  // Beginning of section in the stream
  int64_t bufferStart;
  // Size of the section in the stream
  int64_t bufferSize;

  BiFDataRecord(BiFSectionID ID, int64_t bufferStart, int64_t bufferSize) {
    this->ID = ID;
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

  static size_t getHash(const std::string &FlagName);

protected:
  inline static const std::string bifMark = "igc_bif";

  llvm::LLVMContext &Context;

  template <class T>
  void FindAllBuiltins(T *ptr, const std::function<bool(llvm::Function *)> &predicate,
                       TFunctionsVec &neededBuiltinInstr) {
    CollectBuiltinsPass pass(neededBuiltinInstr, predicate);
    pass.visit(ptr);
  }
};
} // namespace BiFManager
} // namespace IGC
