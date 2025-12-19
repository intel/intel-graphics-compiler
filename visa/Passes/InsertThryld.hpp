/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef G4_PASSES_INSERT_THRYLD_HPP
#define G4_PASSES_INSERT_THRYLD_HPP

#include "Optimizer.h"

namespace vISA {
class InsertThryld {
public:
  InsertThryld(FlowGraph &flowGraph) : fg(flowGraph), builder(*fg.builder) {}
  ~InsertThryld() = default;

  void run();

private:
  FlowGraph &fg;
  IR_Builder &builder;
}; // InsertThryld
}

#endif // G4_PASSES_INSERT_THRYLD_HPP
