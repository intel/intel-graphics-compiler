/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// This file contains a table of extra information about the llvm.genx.*
// intrinsics, used by the vISA register allocator and function writer to
// decide exactly what operand type to use. The more usual approach in an LLVM
// target is to have an intrinsic map to an instruction in instruction
// selection, then have register category information on the instruction. But
// we are not using the target independent code generator, we are generating
// code directly from LLVM IR.
//
//===----------------------------------------------------------------------===//
#include "IGC/common/StringMacros.hpp"
#include "GenXIntrinsics.h"
#include "IsaDescription.h"
#include "visa_igc_common_header.h"
#include "llvm/GenXIntrinsics/GenXIntrinsics.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"

#include "llvmWrapper/IR/DerivedTypes.h"

#include "Probe/Assertion.h"

using namespace llvm;

// In this table:
//
// Each ALU and shared function intrinsic has a record giving information
// about its operands, and how it is written as a vISA instruction. The
// record has an initial field giving the intrinsic ID, then a number of
// fields where each corresponds to a field in the vISA instruction.
//
// A field may be several values combined with the | operator. The first
// value is the operand category (GENERAL etc), or one of a set of
// non-register operand categories (LITERAL, BYTE), or END to terminate
// the record. Other modifier values may be combined, such as SIGNED.
// The LLVM IR argument index plus 1 is also combined in, or 0 for the
// return value.

// Video Analytics intrinsic helper macros, mainly to avoid large blocks
// of near-identical code in the intrinsics look-up table and also to
// aid readability.

// Region access intrinsics do not appear in this table
const GenXIntrinsicInfo::TableType GenXIntrinsicInfo::Table = {
#include "GenXIntrinsicInfoTable.inc"
};

// Get the category and modifier for an arg idx (-1 means return value).
// The returned ArgInfo struct contains just the short read from the table,
// and has methods for accessing the various fields.
GenXIntrinsicInfo::ArgInfo GenXIntrinsicInfo::getArgInfo(int Idx) {
  // Read through the fields in the table to find the one with the right
  // arg index...
  for (auto AI : getInstDesc())
    if (AI.isRealArgOrRet() && AI.getArgIdx() == Idx)
      return AI;
  // Field with requested arg index was not found.
  return ArgInfo{};
}

// Return the starting point of any trailing null (zero) arguments
// for this call. If the intrinsic does not have a ARGCOUNT descriptor
// this will always return the number of operands to the call (ie, there
// is no trailing null zone), even if there are some trailing nulls.
unsigned GenXIntrinsicInfo::getTrailingNullZoneStart(CallInst *CI) {
  auto AI =
      std::find_if(getInstDesc().begin(), getInstDesc().end(),
                   [](auto Arg) { return Arg.getCategory() == ARGCOUNT; });
  if (AI == getInstDesc().end())
    return CI->getNumOperands();
  unsigned BaseArg = AI->getArgIdx();
  unsigned TrailingNullStart = BaseArg;
  for (unsigned Idx = BaseArg; Idx < CI->getNumArgOperands(); ++Idx) {
    if (auto CA = dyn_cast<Constant>(CI->getArgOperand(Idx)))
      if (CA->isNullValue())
        continue;
    TrailingNullStart = Idx + 1;
  }
  return std::max(TrailingNullStart, BaseArg + AI->getArgCountMin());
}

/***********************************************************************
 * getExecSizeAllowedBits : get bitmap of which execsize values are allowed
 *                          for this intrinsic
 *
 * Return:  bit N set if execution size 1<<N is allowed
 */
unsigned GenXIntrinsicInfo::getExecSizeAllowedBits() {
  for (auto AI : getInstDesc()) {
    if (!AI.isGeneral()) {
      switch (AI.getCategory()) {
      case EXECSIZE:
        return 0x3f;
      case EXECSIZE_GE2:
        return 0x3e;
      case EXECSIZE_GE4:
        return 0x3c;
      case EXECSIZE_GE8:
        return 0x38;
      case EXECSIZE_NOT2:
        return 0x3d;
      }
    }
  }
  return 0x3f;
}

/***********************************************************************
 * getPredAllowed : determine if this intrinsic is allowed to have
 *                  a predicated destination mask.
 *
 * Return:  true if it permitted, false otherwise.
 */
bool GenXIntrinsicInfo::getPredAllowed() {
  // Simply search the intrinsic description for an IMPLICITPRED
  // entry. Not very efficient, but the situations where this
  // check is needed are expected to be infrequent.
  for (auto AI : getInstDesc()) {
    if (AI.getCategory() == IMPLICITPRED)
      return true;
  }

  return false;
}

unsigned GenXIntrinsicInfo::getOverridedExecSize(CallInst *CI,
                                                 const GenXSubtarget *ST) {
  auto CalledF = CI->getCalledFunction();
  IGC_ASSERT(CalledF);
  auto ID = GenXIntrinsic::getGenXIntrinsicID(CalledF);
  if (isGenXIntrinsic(ID)) {
    // We check the lsc intrinsic by name or we need to introduce a range of lsc
    // intrinsics.
    if (ID == GenXIntrinsic::genx_lsc_load2d_stateless ||
        ID == GenXIntrinsic::genx_lsc_store2d_stateless)
      return 1;

    StringRef Name = CalledF->getName();
    StringRef LSCPrefix = "llvm.genx.lsc.";
    if (Name.startswith(LSCPrefix))
      // Check if this is a block message from predicate type.
      return !CI->getArgOperand(0)->getType()->isVectorTy();
  }

  switch (ID) {
  default:
    break;
  // Exec size of intrinsics with channels are inferred from address operand.
  case GenXIntrinsic::genx_gather4_scaled2:
  case GenXIntrinsic::genx_gather4_masked_scaled2:
  case GenXIntrinsic::genx_gather4_masked_scaled2_predef_surface:
    return cast<IGCLLVM::FixedVectorType>(
               CI->getArgOperand(4 /*address operand idx*/)->getType())
        ->getNumElements();
  case GenXIntrinsic::genx_raw_send:
  case GenXIntrinsic::genx_raw_sends:
  case GenXIntrinsic::genx_raw_send_noresult:
  case GenXIntrinsic::genx_raw_sends_noresult:
    return 16;
  case GenXIntrinsic::genx_subb:
  case GenXIntrinsic::genx_addc:
    if (auto *VT =
            dyn_cast<IGCLLVM::FixedVectorType>(CI->getOperand(0)->getType()))
      return VT->getNumElements();
    return 1;
  case GenXIntrinsic::genx_dpas:
  case GenXIntrinsic::genx_dpas2:
  case GenXIntrinsic::genx_dpas_nosrc0:
  case GenXIntrinsic::genx_dpasw:
  case GenXIntrinsic::genx_dpasw_nosrc0:
    return ST ? ST->dpasWidth() : 8;
  }

  return 0;
}
