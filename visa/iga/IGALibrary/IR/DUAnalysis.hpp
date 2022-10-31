/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGA_IR_DUANALYSIS_HPP
#define _IGA_IR_DUANALYSIS_HPP

#include "../IR/Kernel.hpp"
#include "../IR/RegSet.hpp"

#include <ostream>
#include <string>
#include <vector>

namespace iga {
// A live range is a path from a definition of some register values to
// a use.  The use may be a read (typical case) or a write (since we need
// to be able to track WAW dependencies).
struct Dep {
  //
  // The producer instruction;
  // this can be nullptr if the value is a program input
  Instruction *def = nullptr;
  //
  // The consumer instruction;
  // this can be nullptr if the value is a program output
  Instruction *use = nullptr;
  //
  // The register values that are live in this path
  RegSet values;
  //
  // The minimum number of instructions this path covers
  int minInsts = 0;
  //
  // The minimum number of instructions this path covers in the same
  // pipe as the def pipe (or 0 if there is no def).
  //   int                      minInstsDefPipe = 0;
  // Need to account syncs in between
  //
  // indicates if the dependency crosses a branch (JEU)
  // N.b. this will false for fallthrough since that isn't a branch
  //        mov r1 ...
  // (f0.0) jmpi TARGET
  //        add ...  r1 // crossesBranch = false here
  // TARGET:
  //        mul ...  r1 // crossesBranch = true here
  bool crossesBranch = false;
  // Similar to the above but includes fallthrough
  bool crossesBlock = false;

  // Dep(const Model &m) : values(m) { }

  Dep(Instruction *_def, const RegSet &_values, Instruction *_use)
      : def(_def), use(_use), values(_values) {}
  Dep(const Dep &) = default;
  Dep &operator=(const Dep &) = default;

  bool operator==(const Dep &p) const;
  bool operator!=(const Dep &p) const { return !(*this == p); }

  // for debug
  void str(std::ostream &os) const;
  std::string str() const;
};

struct LiveCount {
  unsigned grfBytes = 0; // r*
  unsigned flagBytes = 0;   // f*
  unsigned accBytes = 0;    // acc*
  unsigned indexBytes = 0;  // a0*
};

struct DepAnalysis {
  //
  // relation of definitions and uses
  std::vector<Dep> deps;

  //
  // uses without any definition
  std::vector<Dep> liveIn;

  // directly maps instruction ID to the live counts there;
  // the counts are for the live sets *before* the instruction is
  // executed
  std::vector<LiveCount> sums;

  // number of iterations until reaching the fixed-point
  int iterations = 0;
};

// The primary entry point for the live analysis
// The counts are counts *going into* this instruction
DepAnalysis ComputeDepAnalysis(const Kernel *k);
} // namespace iga

#endif // _IGA_IR_ANALYSIS_HPP
