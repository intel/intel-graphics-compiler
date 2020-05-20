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
// AlignmentInfo is a cache of information on the alignment of instruction
// values in a function. Alignment is stored as LogAlign and ExtraBits
// (ExtraBits < 1 << LogAlign) where a value is known to be
// A << LogAlign | ExtraBits.
//
// For a vector value, the alignment information is for element 0.
//
// The alignment of a value is computed as it is required, rather than all
// values in a function being computed in a separate analysis pass.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "GENX_ALIGNMENT_INFO"

#include <algorithm>
#include "GenX.h"
#include "GenXAlignmentInfo.h"
#include "GenXBaling.h"
#include "GenXRegion.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/Debug.h"
#include <set>

using namespace llvm;
using namespace genx;

/***********************************************************************
 * AlignmentInfo::get : get the aligmment of a Value
 *
 * Return:  the Alignment
 */
Alignment AlignmentInfo::get(Value *V)
{
  if (auto C = dyn_cast<Constant>(V))
    return Alignment(C);
  auto Inst = dyn_cast<Instruction>(V);
  if (!Inst) {
    // An Argument has unknown alignment.
    // (FIXME: We may need to do better than this, tracing the value of the
    // Argument at call sites, when arg indirection is introduced.)
    return Alignment::getUnknown();
  }
  auto MapEntry = &InstMap[Inst];
  if (!MapEntry->isUncomputed())
    return *MapEntry; // already in cache
  // Need to compute for this instruction.
  LLVM_DEBUG(dbgs() << "AlignmentInfo::get: computing alignment for " << Inst->getName() << "\n");
  // Get the web of instructions related to this one, including going through
  // phi nodes, excluding ones that we already have alignment for.
  std::set<Instruction *> InstWebSet;
  SmallVector<Instruction *, 8> InstWeb;
  InstWebSet.insert(Inst);
  InstWeb.push_back(Inst);
  for (unsigned i = 0; i != InstWeb.size(); ++i) {
    auto WorkInst = InstWeb[i];
    if (auto Phi = dyn_cast<PHINode>(WorkInst)) {
      for (unsigned ii = 0, ie = Phi->getNumIncomingValues(); ii != ie; ++ii)
        if (auto IncomingInst = dyn_cast<Instruction>(Phi->getIncomingValue(ii)))
          if (InstMap.find(IncomingInst) == InstMap.end()
              && InstWebSet.insert(IncomingInst).second)
            InstWeb.push_back(IncomingInst);
    } else if (isa<BinaryOperator>(WorkInst) || isa<CastInst>(WorkInst)) {
      for (unsigned oi = 0, oe = WorkInst->getNumOperands(); oi != oe; ++oi)
        if (auto IncomingInst = dyn_cast<Instruction>(WorkInst->getOperand(oi)))
          if (InstMap.find(IncomingInst) == InstMap.end()
              && InstWebSet.insert(IncomingInst).second)
            InstWeb.push_back(IncomingInst);
    } else if (CastInst *CI = dyn_cast<CastInst>(WorkInst)) {
      if (auto IncomingInst = dyn_cast<Instruction>(WorkInst->getOperand(0)))
        if (InstMap.find(IncomingInst) == InstMap.end()
          && InstWebSet.insert(IncomingInst).second)
          InstWeb.push_back(IncomingInst);
    } else
      switch (GenXIntrinsic::getGenXIntrinsicID(WorkInst)) {
      case GenXIntrinsic::genx_rdregioni:
      case GenXIntrinsic::genx_rdregionf:
      case GenXIntrinsic::genx_convert:
      case GenXIntrinsic::genx_convert_addr:
        if (auto IncomingInst = dyn_cast<Instruction>(WorkInst->getOperand(0)))
          if (InstMap.find(IncomingInst) == InstMap.end()
              && InstWebSet.insert(IncomingInst).second)
            InstWeb.push_back(IncomingInst);
        break;
      case GenXIntrinsic::genx_ssmad:
      case GenXIntrinsic::genx_uumad:
      case GenXIntrinsic::genx_add_addr:
        for (unsigned oi = 0, oe = WorkInst->getNumOperands(); oi != oe; ++oi)
          if (auto IncomingInst = dyn_cast<Instruction>(WorkInst->getOperand(oi)))
            if (InstMap.find(IncomingInst) == InstMap.end()
              && InstWebSet.insert(IncomingInst).second)
              InstWeb.push_back(IncomingInst);
        break;
      default:
        break;
    }
  }
  LLVM_DEBUG(dbgs() << "web:";
        for (unsigned i = 0, e = InstWeb.size(); i != e; ++i)
          dbgs() << " " << InstWeb[i]->getName();
        dbgs() << "\n");
  // Use a worklist algorithm where each instruction in the web is initially on
  // the worklist.
  std::set<Instruction *> WorkSet;
  for (auto i = InstWeb.begin(), e = InstWeb.end(); i != e; ++i)
    WorkSet.insert(*i);
  while (!InstWeb.empty()) {
    Instruction *WorkInst = InstWeb.back();
    InstWeb.pop_back();
    WorkSet.erase(WorkInst);
    LLVM_DEBUG(dbgs() << "  processing " << WorkInst->getName() << "\n");

    Alignment A(0, 0); // assume unknown
    if (BinaryOperator *BO = dyn_cast<BinaryOperator>(WorkInst)) {
      A = Alignment(); // assume uncomputed
      Alignment A0 = getFromInstMap(BO->getOperand(0));
      Alignment A1 = getFromInstMap(BO->getOperand(1));
      if (!A0.isUncomputed() && !A1.isUncomputed()) {
        switch (BO->getOpcode()) {
          case Instruction::Add:
            A = A0.add(A1);
            break;
          case Instruction::Sub:
            if (A1.isConstant())
              A = A0.add(-(A1.getConstBits()));
            else
              A = Alignment::getUnknown();
            break;
          case Instruction::Mul:
            A = A0.mul(A1);
            break;
          case Instruction::Shl:
            if (A1.isConstant()) {
              A1 = Alignment(A1.getConstBits(), 0);
              A = A0.mul(A1);
            } else
              A = Alignment::getUnknown();
            break;
        default:
          A = Alignment::getUnknown();
          break;
        }
      }
    } else if (CastInst *CI = dyn_cast<CastInst>(WorkInst)) {
      // Handle a bitcast for the same reason as above. This also handles
      // trunc, sext, zext.
      A = getFromInstMap(CI->getOperand(0));
      if (!A.isUncomputed()) {
        unsigned LogAlign = A.getLogAlign(), ExtraBits = A.getExtraBits();
        LogAlign = std::min(
            LogAlign,
            static_cast<unsigned>(
                CI->getType()->getScalarType()->getPrimitiveSizeInBits()));
        if (LogAlign < 32)
          ExtraBits &= (1 << LogAlign) - 1;
        A = Alignment(LogAlign, ExtraBits);
      } else if (!CI->isIntegerCast()) {
        // For no-only-integer cast instructions - FPToUI, FPToSI
        A = Alignment::getUnknown();
      }
    } else if (auto Phi = dyn_cast<PHINode>(WorkInst)) {
      // For a phi node, ignore uncomputed incomings so we have an initial
      // guess at alignment value to propagate round a loop and refine in
      // a later visit to this same phi node.
      A = Alignment(); // initialize to uncomputed
      for (unsigned ii = 0, ie = Phi->getNumIncomingValues(); ii != ie; ++ii) {
        LLVM_DEBUG(dbgs() << "  incoming: " << *Phi->getIncomingValue(ii) << "\n");
        LLVM_DEBUG(dbgs() << "  merging " << A << " and " << getFromInstMap(Phi->getIncomingValue(ii)) << "\n");
        A = A.merge(getFromInstMap(Phi->getIncomingValue(ii)));
        LLVM_DEBUG(dbgs() << "  giving " << A << "\n");
      }
    } else {
      switch (GenXIntrinsic::getGenXIntrinsicID(WorkInst)) {
        case GenXIntrinsic::genx_rdregioni:
        case GenXIntrinsic::genx_rdregionf: {
          // Handle the case of reading a scalar from element 0 of a vector, as
          // a trunc from i32 to i16 is lowered to a bitcast to v2i16 then a
          // rdregion.
          Region R(WorkInst, BaleInfo());
          if (!R.Indirect && !R.Offset)
            A = getFromInstMap(WorkInst->getOperand(0));
          else
            A = Alignment(0, 0);
          break;
        }
        case GenXIntrinsic::genx_constanti:
          A = Alignment(cast<Constant>(WorkInst->getOperand(0)));
          break;
        case GenXIntrinsic::genx_convert:
        case GenXIntrinsic::genx_convert_addr:
          A = getFromInstMap(WorkInst->getOperand(0));
          break;
        case GenXIntrinsic::genx_add_addr: {
          Alignment AA[2];
          for (unsigned oi = 0, oe = WorkInst->getNumOperands(); oi != oe && oi < 2; ++oi)
            AA[oi] = getFromInstMap(WorkInst->getOperand(oi));
          if (!AA[0].isUncomputed() && !AA[1].isUncomputed())
            A = AA[0].add(AA[1]);
          else
            A = Alignment(0, 0);
          break;
        }
        case GenXIntrinsic::genx_ssmad:
        case GenXIntrinsic::genx_uumad: {
          A = Alignment(); // assume uncomputed
          // every source operand should be computed or constant
          Alignment SA[3];
          for (unsigned oi = 0, oe = WorkInst->getNumOperands(); oi != oe && oi < 3; ++oi)
            SA[oi] = getFromInstMap(WorkInst->getOperand(oi));
          if (!SA[0].isUncomputed() && !SA[1].isUncomputed() && !SA[2].isUncomputed())
            A = SA[0].mul(SA[1]).add(SA[2]);
          else
            A = Alignment(0, 0);
          break;
        }
        default:
          A = Alignment(0, 0); // no alignment info
          break;
      }
    }
    // See if the alignment has changed for WorkInst.
    auto MapEntry = &InstMap[WorkInst];
    if (*MapEntry == A)
      continue; // no change
    *MapEntry = A;
    LLVM_DEBUG(dbgs() << "  " << WorkInst->getName() << " updated to " << A << "\n");
    // Add all users that are in the original web to the worklist, if
    // not already in the worklist.
    for (auto ui = WorkInst->use_begin(), ue = WorkInst->use_end();
        ui != ue; ++ui) {
      auto user = cast<Instruction>(ui->getUser());
      if (InstWebSet.find(user) != InstWebSet.end()
          && WorkSet.insert(user).second)
        InstWeb.push_back(user);
    }
  }
  MapEntry = &InstMap[Inst];
  assert(!MapEntry->isUncomputed());
  LLVM_DEBUG(dbgs() << "AlignmentInfo::get: returning " << *MapEntry << "\n");
  return *MapEntry;
}

/***********************************************************************
 * Alignment constructor given literal value
 */
Alignment::Alignment(unsigned C)
{
  LogAlign = countTrailingZeros(C);
  ExtraBits = 0;
  ConstBits = (C < 0x7fffffff)? C : 0x7fffffff;
}

/***********************************************************************
 * Alignment constructor given Constant
 */
Alignment::Alignment(Constant *C)
{
  setUncomputed();
  if (isa<VectorType>(C->getType()))
    C = C->getAggregateElement(0U);
  if (isa<UndefValue>(C)) {
    LogAlign = 31;
    ExtraBits = 0;
    ConstBits = 0x7fffffff;
  } else if (auto CI = dyn_cast<ConstantInt>(C)) {
    LogAlign = countTrailingZeros((unsigned)(CI->getSExtValue()));
    ExtraBits = 0;
    ConstBits = 0x7fffffff;
    if (CI->getSExtValue() < 0x7fffffff && CI->getSExtValue() >= 0)
      ConstBits = (unsigned)(CI->getSExtValue());
  }
}

/***********************************************************************
 * merge : merge two alignments
 */
Alignment Alignment::merge(Alignment Other) const
{
  // If either is uncomputed, result is the other one.
  if (isUncomputed())
    return Other;
  if (Other.isUncomputed())
    return *this;
  // Take the minimum of the two logaligns, then chop off some more for
  // disagreeing extrabits.
  unsigned MinLogAlign = std::min(LogAlign, Other.LogAlign);
  if (MinLogAlign) {
    unsigned DisagreeExtraBits = (ExtraBits ^ Other.ExtraBits)
      & ((1 << MinLogAlign) - 1);
    MinLogAlign = std::min(MinLogAlign,
      (unsigned)countTrailingZeros(DisagreeExtraBits, ZB_Width));
  }
  return Alignment(MinLogAlign, ExtraBits & ((1 << MinLogAlign) - 1));
}

/***********************************************************************
 * merge : add two alignments
 */
Alignment Alignment::add(Alignment Other) const
{
  assert(!isUncomputed() && !Other.isUncomputed());
  // Take the minimum of the two logaligns, then chop off some more for
  // disagreeing extrabits.
  unsigned MinLogAlign = std::min(LogAlign, Other.LogAlign);
  unsigned ExtraBits2 = 0;
  if (MinLogAlign) {
    ExtraBits2 = (ExtraBits + Other.ExtraBits)
      & ((1 << MinLogAlign) - 1);
    MinLogAlign = std::min(MinLogAlign,
      (unsigned)countTrailingZeros(ExtraBits2, ZB_Width));
  }
  return Alignment(MinLogAlign, ExtraBits2 & ((1 << MinLogAlign) - 1));
}

/***********************************************************************
* merge : mul two alignments
*/
Alignment Alignment::mul(Alignment Other) const
{
  assert(!isUncomputed() && !Other.isUncomputed());
  // Take the minimum of the two logaligns, then chop off some more for
  // disagreeing extrabits.
  unsigned MinLogAlign = std::min(LogAlign, Other.LogAlign);
  if (ExtraBits == 0 && Other.ExtraBits == 0)
    MinLogAlign = LogAlign + Other.LogAlign;
  else if (ExtraBits == 0)
    MinLogAlign = LogAlign;
  else if (Other.ExtraBits == 0)
    MinLogAlign = Other.LogAlign;
  unsigned ExtraBits2 = 0;
  if (MinLogAlign) {
    ExtraBits2 = (ExtraBits * Other.ExtraBits)
      & ((1 << MinLogAlign) - 1);
    MinLogAlign = std::min(MinLogAlign,
      (unsigned)countTrailingZeros(ExtraBits2, ZB_Width));
  }
  return Alignment(MinLogAlign, ExtraBits2 & ((1 << MinLogAlign) - 1));
}

/***********************************************************************
 * getFromInstMap : get the alignment of a value, direct from InstMap if
 * found else return Unknown, Alignment(0, 0)
 */
Alignment AlignmentInfo::getFromInstMap(Value *V)
{
  if (auto C = dyn_cast<Constant>(V))
    return Alignment(C);
  if (auto Inst = dyn_cast<Instruction>(V)) {
    return InstMap[V];
  }
  return Alignment::getUnknown();
}

/***********************************************************************
 * Alignment debug dump/print
 */
#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
void Alignment::dump() const
{
  errs() << *this << "\n";
}
#endif

void Alignment::print(raw_ostream &OS) const
{
  if (isUncomputed())
    OS << "uncomputed";
  else if (isUnknown())
    OS << "unknown";
  else if (isConstant())
    OS << "const=" << ConstBits;
  else
    OS << "n<<" << LogAlign << "+" << ExtraBits;
}
