/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VISA_PASSES_MERGESCALAR_HPP
#define VISA_PASSES_MERGESCALAR_HPP

#include "../BuildIR.h"
#include "../FlowGraph.h"
#include "../G4_IR.hpp"
#include "../LoopAnalysis.h"

#include <unordered_map>
#include <vector>

namespace vISA {

// Approximate live range over a linear numbering of the kernel's instructions.
// Liveness is not available this early, so this is built from VarReferences
// def/use sites plus loop membership. Heuristic only, never correctness.
class ScalarLiveRangeApprox {
public:
  static constexpr unsigned InvalidId = 0xffffffffu;

  struct Span {
    unsigned start = InvalidId;
    unsigned end = 0;
    bool valid() const { return start != InvalidId && end >= start; }
    unsigned length() const { return valid() ? (end - start + 1) : 0; }
  };

  explicit ScalarLiveRangeApprox(G4_Kernel &k);

  // Invalid Span means unknown; callers must abstain.
  const Span &getSpan(G4_Declare *dcl);

  unsigned getNumInsts() const { return numInsts; }
  unsigned getInstId(const G4_INST *inst) const;

private:
  unsigned getLoopLastId(Loop *loop);

  G4_Kernel &kernel;
  VarReferences refs;
  std::unordered_map<const G4_BB *, unsigned> bbLastId;
  std::unordered_map<Loop *, unsigned> loopLastId;
  std::unordered_map<G4_Declare *, Span> spans;
  unsigned numInsts = 0;
};

// use by mergeScalar
#define OPND_PATTERN_ENUM(DO)                                                  \
  DO(UNKNOWN)                                                                  \
  DO(IDENTICAL)                                                                \
  DO(CONTIGUOUS)                                                               \
  DO(DISJOINT)                                                                 \
  DO(PACKED)

enum OPND_PATTERN { OPND_PATTERN_ENUM(MAKE_ENUM) };

static const char *patternNames[] = {OPND_PATTERN_ENUM(STRINGIFY)};

struct BUNDLE_INFO {
  static constexpr int maxBundleSize = 16;
  static constexpr int maxNumSrc = 3;
  int size;
  int sizeLimit;
  G4_BB *bb;
  INST_LIST_ITER startIter;
  G4_INST *inst[maxBundleSize];
  OPND_PATTERN dstPattern;
  OPND_PATTERN srcPattern[maxNumSrc];

  BUNDLE_INFO(G4_BB *instBB, INST_LIST_ITER &instPos, int limit)
      : sizeLimit(limit), bb(instBB) {

    inst[0] = *instPos;
    startIter = instPos;
    dstPattern = OPND_PATTERN::UNKNOWN;
    for (int i = 0; i < maxNumSrc; i++) {
      srcPattern[i] = OPND_PATTERN::UNKNOWN;
    }
    size = 1;
  }

  void appendInst(G4_INST *lastInst) {
    vISA_ASSERT(size < maxBundleSize, "max bundle size exceeded");
    inst[size++] = lastInst;
  }

  void deleteLastInst() {
    vISA_ASSERT(size > 0, "empty bundle");
    inst[--size] = nullptr;
  }

  bool canMergeDst(G4_DstRegRegion *dst,
                   std::unordered_set<G4_Declare *> &modifiedDcl,
                   const IR_Builder &builder);
  bool canMergeSource(G4_Operand *src, int srcPos,
                      std::unordered_set<G4_Declare *> &modifiedDcl,
                      const IR_Builder &builder);
  bool canMerge(G4_INST *inst, std::unordered_set<G4_Declare *> &modifiedDcl,
                const IR_Builder &builder);

  bool doMerge(IR_Builder &builder,
               std::unordered_set<G4_Declare *> &modifiedDcl,
               std::vector<G4_Declare *> &newInputs,
               ScalarLiveRangeApprox *lra = nullptr);

  // False if the merged live range would be too long to be worth coalescing
  // into. See vISA_MergeScalarLRMaxSpan.
  bool isLiveRangeProfitable(const IR_Builder &builder,
                             ScalarLiveRangeApprox &lra) const;

  void print(std::ostream &output) const {
    output << "Bundle:\n";
    output << "Dst pattern:\t" << patternNames[dstPattern] << "\n";
    output << "Src Pattern:\t";
    for (int i = 0; i < inst[0]->getNumSrc(); ++i) {
      output << patternNames[srcPattern[i]] << " ";
    }
    output << "\n";
    for (int i = 0; i < size; ++i) {
      inst[i]->emit(output);
      output << "\n";
    }
  }

  void dump() const { print(std::cerr); }

  void findInstructionToMerge(INST_LIST_ITER &iter,
                              std::unordered_set<G4_Declare *> &modifiedDcl,
                              const IR_Builder &builder);

  static bool isMergeCandidate(G4_INST *inst, const IR_Builder &builder,
                               std::unordered_set<G4_Declare *> &modifiedDcl,
                               bool isInSimdFlow);
}; // BUNDLE_INFO
} // namespace vISA

#endif // _MERGESCALAR_H
