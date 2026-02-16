/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CISACodeGen/PatternMatchPass.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/Optimizer/OCLBIUtils.h"
#include "Compiler/Optimizer/OpenCLPasses/SCEVUtils/SCEVUtils.hpp"

#include "llvmWrapper/Transforms/Utils/ScalarEvolutionExpander.h"
#include <llvmWrapper/IR/Function.h>

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/MapVector.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/ScalarEvolutionExpressions.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Use.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

#include "Probe/Assertion.h"

#include "igc_regkeys.hpp"

#include <cstddef>
#include <array>
#include <algorithm>
#include <optional>
#include <map>
#include <set>

namespace llvm {
class FunctionPass;
}

namespace IGC {
llvm::FunctionPass *createDecompose2DBlockFuncsWithHoistingPass();

// LSC 2D block address payload field names for updating only
// (block width/height/numBlock are not updated).
enum class BlockField : unsigned short {
  BASE = 1,
  WIDTH = 2,
  HEIGHT = 3,
  PITCH = 4,
  BLOCKX = 5,
  BLOCKY = 6,
  END = BLOCKY + 1,
  START = WIDTH,
};

inline BlockField &operator++(BlockField &Bf) {
  IGC_ASSERT_MESSAGE(Bf != BlockField::END, "Can't iterate past end of BlockField enum.");
  Bf = static_cast<BlockField>(static_cast<std::underlying_type<BlockField>::type>(Bf) + 1);
  return Bf;
}

// Algorithm:
// 1) If BlockRead has constants pass them to Payload creation intrinsic.
// 2) For non-constant block X/Y, save SCEV to compare them later across the group

// SetPayloadFieldToCreateType represents a pair of BlockField type to be set
// and the place where SetPayloadField should be inserted.

/// Number of arguments for LSC2DBlockCreateAddrPayload intrinsic:
/// {ImageOffset, Width, Height, Pitch, BlockX, BlockY, TileWidth, TileHeight, VNumBlocks}
static constexpr size_t kCreatePayloadNumArgs = 9;
typedef std::array<llvm::Value *, kCreatePayloadNumArgs> PayloadArgsType;
typedef std::map<BlockField, const llvm::SCEV *> PayloadSCEVArgsType;

/// Provides deterministic total ordering on pointer values.
/// Standard < on pointers is unspecified behavior; std::less gives total order.
static inline bool deterministicPtrLess(const void *L, const void *R) { return std::less<const void *>()(L, R); }

// Describes one LSC2DBlockSetAddrPayloadField we plan to create.
struct SetFieldSpec {
  llvm::Value *Val = nullptr;
  llvm::Instruction *InsertBefore = nullptr;
  llvm::Value *IsAddend = nullptr;
  SCEVUtils::DeconstructedSCEV SCEVInfo;
  int FirstVariantLoopIdx = -1;
  llvm::SmallVector<llvm::Loop *, 4> LoopsToSave;

  bool operator==(const SetFieldSpec &Other) const {
    return Val == Other.Val && InsertBefore == Other.InsertBefore && IsAddend == Other.IsAddend;
  }

  bool operator<(const SetFieldSpec &Other) const {
    if (Val != Other.Val)
      return deterministicPtrLess(Val, Other.Val);
    if (InsertBefore != Other.InsertBefore)
      return deterministicPtrLess(InsertBefore, Other.InsertBefore);
    if (IsAddend != Other.IsAddend)
      return deterministicPtrLess(IsAddend, Other.IsAddend);
    return false;
  }
};

// Grouping key: (where payload is created) + (payload args) + (set-field sequence).
struct PayloadGroupKey {
  llvm::Instruction *PayloadInsertBefore = nullptr;
  PayloadArgsType PayloadArgs;
  std::set<BlockField> PayloadFieldsToUpdate;
  PayloadSCEVArgsType PayloadSCEVArgs;
  // Instructions inside loop that need to be hoisted to preheader (e.g., bitcasts)
  // Maps argIdx to the instruction that needs cloning
  std::map<size_t, llvm::Instruction *> InstsToHoist;

  bool operator<(const PayloadGroupKey &Other) const {
    if (PayloadInsertBefore != Other.PayloadInsertBefore)
      return deterministicPtrLess(PayloadInsertBefore, Other.PayloadInsertBefore);

    for (size_t i = 0; i < PayloadArgs.size(); ++i) {
      if (PayloadArgs[i] == Other.PayloadArgs[i])
        continue;
      return deterministicPtrLess(PayloadArgs[i], Other.PayloadArgs[i]);
    }

    if (PayloadSCEVArgs.size() != Other.PayloadSCEVArgs.size())
      return PayloadSCEVArgs.size() < Other.PayloadSCEVArgs.size();

    auto it1 = PayloadSCEVArgs.begin();
    auto it2 = Other.PayloadSCEVArgs.begin();
    while (it1 != PayloadSCEVArgs.end()) {
      if (it1->first != it2->first)
        return it1->first < it2->first;
      if (it1->second != it2->second)
        return deterministicPtrLess(it1->second, it2->second);
      ++it1;
      ++it2;
    }

    // Compare InstsToHoist
    if (InstsToHoist.size() != Other.InstsToHoist.size())
      return InstsToHoist.size() < Other.InstsToHoist.size();

    auto ih1 = InstsToHoist.begin();
    auto ih2 = Other.InstsToHoist.begin();
    while (ih1 != InstsToHoist.end()) {
      if (ih1->first != ih2->first)
        return ih1->first < ih2->first;
      if (ih1->second != ih2->second)
        return deterministicPtrLess(ih1->second, ih2->second);
      ++ih1;
      ++ih2;
    }

    return false;
  }
};

struct SCEVToExpandType {
  const llvm::SCEV *Scev;
  BlockField Field;
};

struct OldLSCAndRequiredSetFields {
  llvm::GenIntrinsicInst *LSC;
  std::map<BlockField, SetFieldSpec> SetFields;
  std::map<BlockField, llvm::ConstantInt *> ImmOffsets;
};

typedef llvm::SmallVector<OldLSCAndRequiredSetFields *, 4> LSCToDecomposeType;

// Maps each payload-group key to the list of original LSC intrinsics (with per-LSC
// set-field requirements) that will be decomposed using a shared created payload.
// Uses MapVector to maintain deterministic insertion-order iteration.
using DecomposeInfoType = llvm::MapVector<PayloadGroupKey, LSCToDecomposeType, std::map<PayloadGroupKey, unsigned>>;

} // namespace IGC
