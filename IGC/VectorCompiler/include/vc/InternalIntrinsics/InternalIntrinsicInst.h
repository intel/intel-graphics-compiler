/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
// This file defines classes that make it really easy to deal with intrinsic
// functions with the isa/dyncast family of functions.  In particular, this
// allows you to do things like:
//
//     if (MemCpyInst *MCI = dyn_cast<MemCpyInst>(Inst))
//        ... MCI->getDest() ... MCI->getSource() ...
//
// All intrinsic function calls are instances of the call instruction, so these
// are all subclasses of the CallInst class.  Note that none of these classes
// has state or virtual methods, which is an important part of this gross/neat
// hack working.
//
//===----------------------------------------------------------------------===//

#ifndef INTERNAL_INTRINSIC_INST_H
#define INTERNAL_INTRINSIC_INST_H

#include "InternalIntrinsics.h"

namespace llvm {
/// IntrinsicInst - A useful wrapper class for inspecting calls to intrinsic
/// functions.  This allows the standard isa/dyncast/cast functionality to
/// work with calls to intrinsic functions.
class InternalIntrinsicInst : public CallInst {
public:
  InternalIntrinsicInst() = delete;
  InternalIntrinsicInst(const InternalIntrinsicInst &) = delete;
  void operator=(const InternalIntrinsicInst &) = delete;

  /// getIntrinsicID - Return the intrinsic ID of this intrinsic.
  ///
  vc::InternalIntrinsic::ID getIntrinsicID() const {
    return vc::InternalIntrinsic::getInternalIntrinsicID(getCalledFunction());
  }

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static bool classof(const CallInst *I) {
    if (const Function *CF = I->getCalledFunction()) {
      return CF->getName().startswith(
          vc::InternalIntrinsic::getInternalIntrinsicPrefix());
    }
    return false;
  }
  static bool classof(const Value *V) {
    return isa<CallInst>(V) && classof(cast<CallInst>(V));
  }
};

// TODO: add more classes to make our intrinsics easier to use

} // namespace llvm

#endif // INTERNAL_INTRINSIC_INST_H
