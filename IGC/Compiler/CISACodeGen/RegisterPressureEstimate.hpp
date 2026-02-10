/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

// Estimate the register pressure at a program point.

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include <llvm/IR/InstVisitor.h>
#include "llvm/Analysis/LoopInfo.h"
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/IR/DerivedTypes.h>
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Probe/Assertion.h"

namespace IGC {
constexpr unsigned int SIMD_PRESSURE_MULTIPLIER = 8;

class RegisterPressureEstimate : public llvm::FunctionPass {
public:
  static char ID;
  RegisterPressureEstimate(bool RequireWIA = true, bool CountTemps = false)
      : FunctionPass(ID), m_DL(nullptr), m_pFunc(nullptr), LI(nullptr), WI(nullptr), m_available(false),
        MaxAssignedNumber(0), OVERALL_PRESSURE_UPBOUND(512), m_requireWIA(RequireWIA), m_countTemps(CountTemps) {
    initializeRegisterPressureEstimatePass(*llvm::PassRegistry::getPassRegistry());
  }

  /// @brief  Provides name of pass
  virtual llvm::StringRef getPassName() const override { return "RegisterPressureEstimate"; }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  bool runOnFunction(llvm::Function &F) override;

  bool doFinalization(llvm::Module &F) override {
    for (auto Item : m_pLiveRangePool)
      delete Item;
    m_pLiveRangePool.clear();
    return false;
  }
  /// \brief Describe a value is live at Begin and dead right after End.
  struct Segment {
    unsigned Begin{};
    unsigned End{};
    Segment() {}
    Segment(unsigned Begin, unsigned End) : Begin(Begin), End(End) {}
    bool operator<(const Segment &Other) const {
      if (Begin != Other.Begin) {
        return Begin < Other.Begin;
      }
      return End < Other.End;
    }
  };

  /// \brief A live range consists of a list of live segments.
  struct LiveRange {
    /// Empty live range.
    LiveRange() {}

    LiveRange(const LiveRange &) = delete;
    LiveRange &operator=(const LiveRange &) = delete;

    /// \brief Shorten a segment.
    void setBegin(unsigned B);

    /// \brief Append a new segment.
    void addSegment(unsigned B, unsigned E) {
      if (B < E) {
        Segments.push_back(Segment(B, E));
      }
    }

    /// \brief Sort segments and merge them.
    void sortAndMerge();

    /// \brief Check whether N is contained in this live range.
    bool contains(unsigned N) const {
      // Segments are sorted.
      for (auto &Seg : Segments) {
        if (N < Seg.Begin) {
          return false;
        }
        if (N < Seg.End) {
          return true;
        }
      }
      // N is out of range.
      return false;
    }

    void dump() const;
    void print(llvm::raw_ostream &OS) const;
    llvm::SmallVector<Segment, 8> Segments;
  };

  LiveRange *getOrCreateLiveRange(llvm::Value *V) {
    auto Iter = m_pLiveRanges.find(V);
    if (Iter != m_pLiveRanges.end()) {
      return Iter->second;
    }

    auto Result = m_pLiveRanges.insert(std::make_pair(V, createLiveRange()));
    return Result.first->second;
  }

  LiveRange *getLiveRangeOrNull(llvm::Value *V) {
    auto Iter = m_pLiveRanges.find(V);
    if (Iter != m_pLiveRanges.end()) {
      return Iter->second;
    }
    return nullptr;
  }

  void createLiveRange(llvm::Value *V) {
    IGC_ASSERT(!m_pLiveRanges.count(V));
    m_pLiveRanges[V] = createLiveRange();
  }

  /// \brief Cleanup live ranges.
  void mergeLiveRanges() {
    for (auto &Item : m_pLiveRanges) {
      Item.second->sortAndMerge();
    }
  }

  /// \brief Assign a number to each instruction in a function.
  void assignNumbers();

  /// \brief Print instruction list with numbering.
  void printNumbering(llvm::raw_ostream &OS);
  void dumpNumbering();

  /// \brief Used to fetch number on instructions and BB
  unsigned getAssignedNumberForInst(llvm::Instruction *);
  unsigned getMaxAssignedNumberForFunction() { return MaxAssignedNumber; }
  unsigned getMaxAssignedNumberForBB(llvm::BasicBlock *);
  unsigned getMinAssignedNumberForBB(llvm::BasicBlock *pBB);

  /// \brief Print live ranges.
  void printLiveRanges(llvm::raw_ostream &OS);
  void dumpLiveRanges();

  /// \brief Scan a function and build live ranges for all values of interest. A
  /// live range consists of non-overlapping live intervals [i, j) where i is
  /// the label this value starts to live and j is the label where it ends
  /// living. This value may still be used at j but it does not interfere with
  /// the value defined at j. Thus intervals are open on the right-hand side.
  /// The return value of true indicates that live ranges are available for use.
  bool buildLiveIntervals(bool RemoveLR = false);

  /// \brief Return the register pressure for the whole function.
  unsigned getMaxRegisterPressure();

  /// \brief Return the register pressure for a basic block.
  unsigned getMaxRegisterPressure(llvm::BasicBlock *BB) const;

  void printRegisterPressureInfo(bool Detailed = false, const char *msg = "");

  bool isAvailable() const { return m_available; }

  void clear(bool RemoveLR) {
    for (auto &Item : m_pLiveRanges)
      Item.second->Segments.clear();
    if (RemoveLR)
      m_pLiveRanges.clear();
    for (auto Item : m_pLiveRangePool)
      delete Item;
    m_pLiveRangePool.clear();
  }

  unsigned getRegisterPressureForInstructionFromRPMap(unsigned number) const;

  unsigned getRegisterWeightForInstruction(llvm::Instruction *Inst) const;

  void buildRPMapPerInstruction();

private:
  unsigned int OVERALL_PRESSURE_UPBOUND;

  /// \brief Return the register pressure at location specified by Inst.
  unsigned getRegisterPressure(llvm::Instruction *Inst) const;

  unsigned getValueBytes(llvm::Value *V) const {
    auto Ty = V->getType();
    if (Ty->isVoidTy())
      return 0;
    auto VTy = llvm::dyn_cast<IGCLLVM::FixedVectorType>(Ty);
    auto eltTy = VTy ? VTy->getElementType() : Ty;
    uint32_t nelts = VTy ? int_cast<uint32_t>(VTy->getNumElements()) : 1;
    uint32_t eltBits = (uint32_t)m_DL->getTypeSizeInBits(eltTy);
    uint32_t nBytes = nelts * ((eltBits + 7) / 8);
    unsigned int simdness = (WI && WI->isUniform(V)) ? 1 : SIMD_PRESSURE_MULTIPLIER;
    return simdness * nBytes;
  }

  LiveRange *createLiveRange() {
    LiveRange *LR = new LiveRange();
    m_pLiveRangePool.push_back(LR);
    return LR;
  }

private:
  const llvm::DataLayout *m_DL;
  /// The function being analyzed.
  llvm::Function *m_pFunc;

  /// The loop info object.
  llvm::LoopInfo *LI;

  /// uniform analysis object
  WIAnalysis *WI;

  /// Each instruction gets an ID.
  llvm::DenseMap<llvm::Value *, unsigned> m_pNumbers;

  /// The value to live range map.
  std::map<llvm::Value *, LiveRange *> m_pLiveRanges;

  /// To check if live range info is available
  bool m_available;

  std::vector<LiveRange *> m_pLiveRangePool;

  /// the max assigned number for live-range
  unsigned int MaxAssignedNumber;

  llvm::DenseMap<unsigned, unsigned> m_pRegisterPressureByInstruction;

  bool m_requireWIA;
  bool m_countTemps;
};
} // namespace IGC
