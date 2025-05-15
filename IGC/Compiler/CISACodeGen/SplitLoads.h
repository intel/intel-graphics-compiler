/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CISACodeGen/IGCLivenessAnalysis.h"
#include "Compiler/CodeGenPublic.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "common/LLVMWarningsPop.hpp"

#include <memory>
#include <set>
#include <utility>

namespace llvm {
class FunctionPass;
}

namespace IGC {
namespace LS {

/// A `struct` containing two dimensions of a block.
struct Dims {
  unsigned grSize, numOfGr;
  unsigned size() const { return grSize * numOfGr; }

  bool operator<(const Dims &rhs) const {
    return grSize < rhs.grSize ||
           (grSize == rhs.grSize && numOfGr < rhs.numOfGr);
  }
};

using PossibleDims = std::set<Dims>;

struct Config {
  Module *M = nullptr; // for debug info
  CodeGenContext *CGC = nullptr;
  IGCLivenessAnalysis *RPE = nullptr;

  bool isLegitW8 = false;
  unsigned sizeOfRegs_B = 0;
  unsigned numOfRegs = 0;
  unsigned defaultSimd = 0;
  unsigned actualSimd = 0;

  /// Turns on the splitting pass.
  bool enableLoadSplitting = IGC_IS_FLAG_ENABLED(LS_enableLoadSplitting);

  /// If `true`, the register pressure data is ignored and the pass splits all
  /// loads.
  bool ignoreSplitThreshold = IGC_IS_FLAG_ENABLED(LS_ignoreSplitThreshold);

  /// Minimal split size in terms of GRFs, used in determination of the possible
  /// split dimensions.
  unsigned minSplitSize_GRF = IGC_GET_FLAG_VALUE(LS_minSplitSize_GRF);

  /// Minimal split size in terms of vector elements (bit width-independent),
  /// used in determination of the possible split dimensions.
  unsigned minSplitSize_E = IGC_GET_FLAG_VALUE(LS_minSplitSize_E);

  /// If `ignoreSplitThreshold = false`, the pass splits loads in a given basic
  /// block only if the maximal register pressure exceeds total GRFs by this
  /// much.
  int splitThresholdDelta_GRF = IGC_GET_FLAG_VALUE(LS_splitThresholdDelta_GRF);

  /// Minimal split size in bytes, to be calculated from minSplitSize_GRF.
  unsigned minSplitSize_B = 0;

  /// Absolute split threshold in bytes.
  int splitThreshold_B = 0;

  Config(const Config &) = delete;
  Config(Config &&) = delete;

  /// Value of `SIMD` as reported by metadata.
  unsigned SIMD() const { return actualSimd ? actualSimd : defaultSimd; }

  static Config &get() {
    static Config config;
    return config;
  }

  bool initialize(Function *inF, CodeGenContext *inCGC,
                  IGCLivenessAnalysis *inRPE);

private:
  Config() = default;
};

Config &config();

/// The class `LoadSplitter` is responsible for splitting loads in an LLVM
/// function.
class LoadSplitter {
public:
  /// @brief Factory function to create an instance of `LoadSplitter`.
  /// @param inF   LLVM function pointer.
  /// @param inCGC The code generation context.
  /// @param inRPE The register pressure estimator.
  static std::unique_ptr<LoadSplitter>
  Create(Function *inF, CodeGenContext *inCGC, IGCLivenessAnalysis *inRPE);

  LoadSplitter(const LoadSplitter &) = delete;
  LoadSplitter &operator=(const LoadSplitter &) = delete;

  /// @brief Returns `true` is the register pressure for the basic block exceeds
  /// the threshold given by the flag IGS_LS_splitThresholdDelta_GRF. The
  /// pressure must also exceed the goal, IGC_LS_goalPressureDelta_GRF.
  /// @param BB The basic block to check.
  bool isRPHigh(BasicBlock *BB);

  /// @brief Returns the set of all possible dimensions in which the load or AP
  /// loads can be split into.
  /// @param GII The load or the address payload to split. If `GII` is an AP
  /// Load, all loads associated with its AP are considered.
  PossibleDims possibleDims(GenIntrinsicInst *GII);

  /// @brief Splits the block load into the specified dimensions.
  /// @param GII The load or the address payload to split. If `GII` is an AP
  /// Load, all loads associated with its AP are considered.
  /// @param dims Size of the new blocks.
  /// @return Returns `true` on success, `false` otherwise.
  bool split(GenIntrinsicInst *GII, Dims dims);

  /// @brief Splits all loads in the basic block to the smallest size possible.
  /// @param BB The basic block.
  /// @return Returns `true` on success, `false` otherwise.
  bool splitAllToSmallest(BasicBlock *BB);

private:
  LoadSplitter() = default;
  struct Impl;
  std::unique_ptr<Impl> impl;
};

} // namespace LS

FunctionPass *createSplitLoadsPass();
} // namespace IGC
