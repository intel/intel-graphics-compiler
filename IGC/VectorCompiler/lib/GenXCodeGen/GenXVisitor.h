/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXVisitor
/// -----------
///
/// GenXVisitor is a wrapper over llvm::InstVisitor, that supports GenX-specific
/// routines: iteration over function groups and GenX intrinsics. All visitors
/// are organized hierarchically, i.e. default non-overrided callbacks delegate
/// control to the next higher level, until the most specific overrided callback
/// for this intrinsic will be reached.
///
/// It's expected that support for the majority of GenX intrinsics will be added
/// here over time.
///
//===----------------------------------------------------------------------===//
#ifndef GENX_VISITOR_H
#define GENX_VISITOR_H

#include "FunctionGroup.h"

#include "vc/InternalIntrinsics/InternalIntrinsicInst.h"

#include "llvm/GenXIntrinsics/GenXIntrinsicInst.h"
#include "llvm/IR/InstVisitor.h"

namespace llvm {
namespace genx {

#define DELEGATE(CLASS_TO_VISIT)                                               \
  return static_cast<SubClass *>(this)->visit##CLASS_TO_VISIT(                 \
      static_cast<CLASS_TO_VISIT &>(I))

#define GENX_DELEGATE(CLASS_TO_VISIT)                                          \
  return static_cast<SubClass *>(this)->visit##CLASS_TO_VISIT(I)

template <typename SubClass, typename RetTy = void>
class GenXVisitor : public InstVisitor<SubClass, RetTy> {
public:
  using InstVisitor<SubClass, RetTy>::visit;
  // Visitor for FunctionGroup.
  void visit(FunctionGroup &FG) {
    static_cast<SubClass *>(this)->visitFunctionGroup(FG);
    visit(FG.begin(), FG.end());
  };

  // Forwarding function so that user can visit with pointer.
  void visit(FunctionGroup *FG) { visit(*FG); }

  // Default fallback for visiting FunctionGroup.
  void visitFunctionGroup(FunctionGroup &FG) {}

  // Redefine callback for Instruction::Call opcode: if it's GenX intrinsic,
  // delegate it to appropriate callback, otherwise use standard call delegate.
  RetTy visitCall(CallInst &I) {
    if (vc::InternalIntrinsic::isInternalIntrinsic(&I))
      return delegateInternalIntrinsic(cast<InternalIntrinsicInst>(I));
    if (GenXIntrinsic::isGenXIntrinsic(&I))
      return delegateGenXIntrinsic(cast<GenXIntrinsicInst>(I));
    return InstVisitor<SubClass, RetTy>::visitCall(I);
  }

  // Implementation of default callbacks for vc internal intrinsics: propagate
  // calls to the next level of their hierarchy.
  RetTy visitInternalIntrinsicInst(InternalIntrinsicInst &I) {
    DELEGATE(CallInst);
  }

  // Implementation of default callbacks for GenX intrinsics: propagate calls to
  // the next level of their hierarchy.
  RetTy visitGenXIntrinsicInst(GenXIntrinsicInst &I) { DELEGATE(CallInst); }

  RetTy visitWrRegion(GenXIntrinsicInst &I) {
    GENX_DELEGATE(GenXIntrinsicInst);
  }

  RetTy visitRdRegion(GenXIntrinsicInst &I) {
    GENX_DELEGATE(GenXIntrinsicInst);
  }

  RetTy visitReadWritePredefReg(GenXIntrinsicInst &I) {
    GENX_DELEGATE(GenXIntrinsicInst);
  }

  RetTy visitReadPredefReg(GenXIntrinsicInst &I) {
    GENX_DELEGATE(ReadWritePredefReg);
  }

  RetTy visitWritePredefReg(GenXIntrinsicInst &I) {
    GENX_DELEGATE(ReadWritePredefReg);
  }

private:
  RetTy delegateInternalIntrinsic(InternalIntrinsicInst &I) {
    auto ID = I.getIntrinsicID();
    if (ID == vc::InternalIntrinsic::not_internal_intrinsic)
      DELEGATE(CallInst);
    else
      GENX_DELEGATE(InternalIntrinsicInst);
  }

  RetTy delegateGenXIntrinsic(GenXIntrinsicInst &I) {
    auto ID = I.getIntrinsicID();
    if (ID == GenXIntrinsic::not_genx_intrinsic)
      DELEGATE(CallInst);
    else if (GenXIntrinsic::isWrRegion(ID))
      GENX_DELEGATE(WrRegion);
    else if (GenXIntrinsic::isRdRegion(ID))
      GENX_DELEGATE(RdRegion);
    else if (GenXIntrinsic::isReadPredefReg(ID))
      GENX_DELEGATE(ReadPredefReg);
    else if (GenXIntrinsic::isWritePredefReg(ID))
      GENX_DELEGATE(WritePredefReg);
    else
      GENX_DELEGATE(GenXIntrinsicInst);
  }
};

#undef DELEGATE
#undef GENX_DELEGATE

} // namespace genx
} // namespace llvm

#endif
