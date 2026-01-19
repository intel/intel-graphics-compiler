/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VISA_PASSES_MERGESCALAR_HPP
#define VISA_PASSES_MERGESCALAR_HPP

#include "../BuildIR.h"
#include "../FlowGraph.h"
#include "../G4_IR.hpp"

namespace vISA {

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
               std::vector<G4_Declare *> &newInputs);

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
