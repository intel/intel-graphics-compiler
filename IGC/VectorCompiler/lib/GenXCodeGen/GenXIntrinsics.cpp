/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
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
#include "GenXIntrinsics.h"
#include "IsaDescription.h"
#include "visa_igc_common_header.h"
#include "llvm/GenXIntrinsics/GenXIntrinsics.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"

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

const GenXIntrinsicInfo::DescrElementType GenXIntrinsicInfo::Table[] = {

// Region access intrinsics do not appear in this table

#include "GenXIntrinsicInfoTable.inc"

    END};

GenXIntrinsicInfo::GenXIntrinsicInfo(unsigned IntrinId) : Args(0) {
  const auto *p = Table;
  for (;;) {
    if (*p == END)
      break; // intrinsic not found; leave Args pointing at END field
    if (IntrinId == *p++)
      break;
    // Scan past the rest of this entry.
    while (*p++ != END)
      ;
  }
  // We have found the right entry.
  Args = p;
}

// Get the category and modifier for an arg idx (-1 means return value).
// The returned ArgInfo struct contains just the short read from the table,
// and has methods for accessing the various fields.
GenXIntrinsicInfo::ArgInfo GenXIntrinsicInfo::getArgInfo(int Idx) {
  // Read through the fields in the table to find the one with the right
  // arg index...
  for (const auto *p = Args; *p; p++) {
    ArgInfo AI(*p);
    if (AI.isRealArgOrRet() && AI.getArgIdx() == Idx)
      return AI;
  }
  // Field with requested arg index was not found.
  return END;
}

// Return the starting point of any trailing null (zero) arguments
// for this call. If the intrinsic does not have a ARGCOUNT descriptor
// this will always return the number of operands to the call (ie, there
// is no trailing null zone), even if there are some trailing nulls.
unsigned GenXIntrinsicInfo::getTrailingNullZoneStart(CallInst *CI) {
  unsigned TrailingNullStart = CI->getNumArgOperands();

  const auto *p = Args;
  for (; *p; p++) {
    ArgInfo AI(*p);
    if (AI.getCategory() == ARGCOUNT)
      break;
  }

  if (*p) {
    ArgInfo ACI(*p);
    unsigned BaseArg = ACI.getArgIdx();

    TrailingNullStart = BaseArg;
    for (unsigned Idx = BaseArg; Idx < CI->getNumArgOperands(); ++Idx) {
      if (auto CA = dyn_cast<Constant>(CI->getArgOperand(Idx))) {
        if (CA->isNullValue())
          continue;
      }
      TrailingNullStart = Idx + 1;
    }

    if (TrailingNullStart < BaseArg + ACI.getArgCountMin())
      TrailingNullStart = BaseArg + ACI.getArgCountMin();
  }

  return TrailingNullStart;
}

/***********************************************************************
 * getExecSizeAllowedBits : get bitmap of which execsize values are allowed
 *                          for this intrinsic
 *
 * Return:  bit N set if execution size 1<<N is allowed
 */
unsigned GenXIntrinsicInfo::getExecSizeAllowedBits() {
  for (const auto *p = Args; *p; p++) {
    if (!(*p & GENERAL)) {
      switch (*p & CATMASK) {
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
  for (const auto *p = getInstDesc(); *p; ++p) {
    ArgInfo AI(*p);
    if (AI.getCategory() == IMPLICITPRED)
      return true;
  }

  return false;
}

unsigned GenXIntrinsicInfo::getOverridedExecSize(CallInst *CI,
                                                 const GenXSubtarget *ST) {
  auto CalledF = CI->getCalledFunction();
  assert(CalledF);
  auto ID = GenXIntrinsic::getGenXIntrinsicID(CalledF);

  switch (ID) {
  default:
    break;
  // Exec size of intrinsics with channels are inferred from address operand.
  case GenXIntrinsic::genx_gather4_scaled2:
    return CI->getArgOperand(4)->getType()->getVectorNumElements();
  case GenXIntrinsic::genx_raw_send:
  case GenXIntrinsic::genx_raw_sends:
  case GenXIntrinsic::genx_raw_send_noresult:
  case GenXIntrinsic::genx_raw_sends_noresult:
    return 16;
  }

  return 0;
}
