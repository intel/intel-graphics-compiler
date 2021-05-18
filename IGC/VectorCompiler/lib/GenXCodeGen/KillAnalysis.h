/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// KillAnalysis is an object that can analyze which uses of a value are kills,
// and cache the result.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/ValueMap.h"

namespace llvm {

class Use;
class Value;

class KillAnalysis {
  ValueMap<const Value *, SmallVector<Instruction *, 2>> Map;
public:
  // erase : erase a value from the KillAnalysis
  void erase(Value *V) { Map.erase(V); }
  // isKill : determine whether a use is a kill
  bool isKill(Use *U);
private:
  SmallVectorImpl<Instruction *> *getKills(Value *V);
};

} // namespace llvm
