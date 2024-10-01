/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// The GenXVectorDecomposer class is called by by the GenXPostLegalization pass
// to perform vector decomposition. See comment in GenXVectorDecomposer.h.
//
//===----------------------------------------------------------------------===//
#include "GenXVectorDecomposer.h"
#include "GenX.h"
#include "GenXBaling.h"
#include "GenXUtil.h"

#include "vc/Utils/GenX/GlobalVariable.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "Probe/Assertion.h"

#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Support/TypeSize.h"

#define DEBUG_TYPE "GENX_POST_LEGALIZATION"

using namespace llvm;
using namespace genx;
using namespace GenXIntrinsic::GenXRegion;
using namespace vc;

static cl::opt<unsigned>
    LimitGenXVectorDecomposer("limit-genx-vector-decomposer",
                              cl::init(UINT_MAX), cl::Hidden,
                              cl::desc("Limit GenX vector decomposer."));

static cl::opt<unsigned> GenXReportVectorDecomposerFailureThreshold(
    "genx-report-vector-decomposer-failure-threshold", cl::init(UINT_MAX),
    cl::Hidden,
    cl::desc("Byte size threshold for reporting failure of GenX vector "
             "decomposer."));

static cl::opt<unsigned> GenXDefaultSelectPredicateWidth(
    "genx-sel-width", cl::init(32), cl::Hidden,
    cl::desc("The default width for select predicate splitting."));

namespace {

class DiagnosticVectorDecomposition : public DiagnosticInfo {
private:
  const Twine &Description;
  Instruction *Inst;

  static const int KindID;

  static int getKindID() { return KindID; }

public:
  DiagnosticVectorDecomposition(Instruction *I, const Twine &Desc,
                                DiagnosticSeverity Severity = DS_Error)
      : DiagnosticInfo(getKindID(), Severity), Description(Desc), Inst(I) {}

  void print(DiagnosticPrinter &P) const override {
    std::string Str;
    raw_string_ostream OS(Str);

    auto DL = Inst->getDebugLoc();
    DL.print(OS);

    OS << ' ' << Description;
    OS << '\n';
    OS.flush();
    P << Str;
  }

  static bool classof(const DiagnosticInfo *DI) {
    return DI->getKind() == getKindID();
  }
};

const int DiagnosticVectorDecomposition::KindID =
    llvm::getNextAvailablePluginDiagnosticKind();

} // end anonymous namespace

/***********************************************************************
 * VectorDecomposer::run : run the vector decomposer on the start wrregion
 *      instructions added with addStartWrRegion()
 *
 * Return:  true if code modified
 */
bool VectorDecomposer::run(const DataLayout &ArgDL) {
  DL = &ArgDL;

  bool Modified = false;
  // Process each start wrregion added with addStartWrRegion().
  for (auto swi = StartWrRegions.begin(), swe = StartWrRegions.end();
       swi != swe; ++swi) {
    Instruction *Inst = *swi;
    Modified |= processStartWrRegion(Inst);
    clearOne();
  }
  for (auto i = ToDelete.begin(), e = ToDelete.end(); i != e; ++i)
    (*i)->deleteValue();
  clear();
  return Modified;
}

/***********************************************************************
 * VectorDecomposer::processStartWrRegion : process one start wrregion
 *
 * Enter:   Inst = the start wrregion. Note that this might have already
 *                 been erased if it was part of an already processed web,
 *                 so the first thing we have to do is check that.
 *
 * This processes one start wrregion (a wrregion with constant input). If it has
 * not already been seen as part of another web, this processes the web
 * containing the start wrregion.
 *
 * Return:  true if code modified
 */
bool VectorDecomposer::processStartWrRegion(Instruction *Inst) {
  // Determine the web of vectors related by wrregion, phi nodes, bitcast,
  // and determine the decomposition that we can do to the web.
  if (!determineDecomposition(Inst))
    return false;
  if (++DecomposedCount > LimitGenXVectorDecomposer)
    return false;
  if (LimitGenXVectorDecomposer != UINT_MAX)
    dbgs() << "genx vector decomposer " << DecomposedCount << "\n";
  decompose();
  clearOne();
  return true;
}

/***********************************************************************
 * VectorDecomposer::determineDecomposition : determine the web of vectors
 * related by wrregion, phi nodes, bitcast, and determine the decomposition
 * that we can do to the web
 *
 * Enter:   Inst = the start wrregion. Note that this might have already
 *                 been erased if it was part of an already processed web,
 *                 so the first thing we have to do is check that.
 *
 * Return:  true if decomposition possible; Decomposition and Offsets set up
 *          as described in the comment near the end of this function
 */
bool VectorDecomposer::determineDecomposition(Instruction *Inst) {
  if (Seen.find(Inst) != Seen.end())
    return false; // This start wrregion already processed in some other web
                  // (and may have been erased).
  NotDecomposingReportInst = Inst;
  Web.clear();
  Decomposition.clear();
  unsigned GRFWidth = genx::ByteBits * GRFByteSize;
  unsigned NumGrfs =
      alignTo(DL->getTypeSizeInBits(Inst->getType()), GRFWidth) / GRFWidth;
  if (NumGrfs == 1)
    return false; // Ignore single GRF vector.
  LLVM_DEBUG(dbgs() << "VectorDecomposer::determineDecomposition(" << *Inst
                    << ")\n");
  NotDecomposing = false;
  for (unsigned i = 0; i != NumGrfs; ++i)
    Decomposition.push_back(i);
  addToWeb(Inst);
  for (unsigned Idx = 0; Idx != Web.size(); ++Idx) {
    Inst = Web[Idx];
    // Look at the def of this value.
    if (GenXIntrinsic::isWrRegion(Inst)) {
      // wrregion. If the "old value of vector" input is not constant, include
      // it in the web.
      auto *NewVal = dyn_cast<Instruction>(
          Inst->getOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum));
      if (NewVal && GenXIntrinsic::isRdRegion(NewVal))
        if (GenXIntrinsic::isReadPredefReg(NewVal->getOperand(
                GenXIntrinsic::GenXRegion::OldValueOperandNum)))
          setNotDecomposing(Inst, "read predefined reg");
      // we still want to have this value in Seen,
      // even we won't decompose this web
      addToWeb(Inst->getOperand(0), Inst);
    } else if (auto Phi = dyn_cast<PHINode>(Inst)) {
      // Phi node. Add all incomings to the web.
      for (unsigned j = 0, je = Phi->getNumIncomingValues(); j != je; ++j)
        addToWeb(Phi->getIncomingValue(j), Phi);
    } else if (isa<BitCastInst>(Inst)) {
      // Bitcast. Add the input to the web. But a bitcast with non-instruction
      // input confuses this algorithm, so in that case disable it. We're not
      // really expecting a bitcast with constant input anyway, although we
      // might get one with arg input.
      if (isa<Instruction>(Inst->getOperand(0)))
        addToWeb(Inst->getOperand(0), Inst);
      else
        setNotDecomposing(Inst, "use of function argument or constant");
    } else {
      // Any other def. This stops decomposition.
      if ((isa<CallInst>(Inst) &&
           !GenXIntrinsic::isAnyNonTrivialIntrinsic(Inst)) ||
          isa<ExtractValueInst>(Inst))
        setNotDecomposing(Inst, "return value from call");
      else
        setNotDecomposing(Inst, "other non-decomposable definition");
    }
    // Look at the uses of this value.
    for (auto ui = Inst->use_begin(), ue = Inst->use_end(); ui != ue; ++ui) {
      auto user = cast<Instruction>(ui->getUser());
      if (auto Phi = dyn_cast<PHINode>(user)) {
        // Use in a phi node. Add the result of the phi node and all the other
        // incomings to the web.
        addToWeb(Phi);
        for (unsigned j = 0, je = Phi->getNumIncomingValues(); j != je; ++j) {
          auto Incoming = dyn_cast<Instruction>(Phi->getIncomingValue(j));
          if (Incoming && Incoming != Inst)
            addToWeb(Incoming, Phi);
        }
        continue;
      }
      if ((GenXIntrinsic::isWrRegion(user) && !ui->getOperandNo()) ||
          isa<BitCastInst>(user)) {
        // Use as the "old value of vector" operand of a wrregion, or in a
        // bitcast. Add the result of the wrregion to the web.
        addToWeb(user);
        continue;
      }
      if (GenXIntrinsic::isRdRegion(user) && !ui->getOperandNo()) {
        // Use as the vector value in rdregion. Adjust decomposition.
        adjustDecomposition(user);
        continue;
      }
      // We have some other use that stops us decomposing this web. (We
      // continue gathering the web anyway so that all values in it get put
      // in the Seen set.)
      if (isa<InsertValueInst>(user) || isa<ReturnInst>(user))
        setNotDecomposing(user, "use as return value");
      else if (isa<CallInst>(user) &&
               !GenXIntrinsic::isAnyNonTrivialIntrinsic(user))
        setNotDecomposing(user, "use as call argument");
      else
        setNotDecomposing(user, "other non-decomposable use");
    }
  }
  if (NotDecomposing)
    return false;
  // Now we have Decomposition[] set to reflect how we can decompose the GRFs
  // of the vector. A range of Decomposition[i] with the same value need to
  // be kept together in the same vector. Further, for the start of such a
  // range, Decomposition[i] == i. So for example the array might be set to
  // { 0, 0, 2, 2, 4, 4, 4, 4, 8, 8 }.
  //
  // Change Decomposition[] so the indices used are contiguous, changing the
  // example above to { 0, 0, 1, 1, 2, 2, 2, 2, 3, 3 }, and create the Offsets[]
  // array to translate a value from Decomposition[] into the GRF offset, so
  // for this example { 0, 2, 4, 8 }.
  Offsets.clear();
  for (unsigned Last = UINT_MAX, i = 0, e = Decomposition.size(); i != e; ++i) {
    if (Decomposition[i] != Last) {
      Offsets.push_back(Decomposition[i]);
      Last = Decomposition[i];
    }
    Decomposition[i] = Offsets.size() - 1;
  }
  LLVM_DEBUG({
    dbgs() << "decompose to";
    for (unsigned i = 0; i != Decomposition.size(); ++i)
      dbgs() << " " << Decomposition[i];
    dbgs() << ":";
    for (unsigned i = 0; i != Offsets.size(); ++i)
      dbgs() << " " << Offsets[i];
    dbgs() << ":";
    for (unsigned i = 0; i != Web.size(); ++i)
      dbgs() << " " << Web[i]->getName();
    dbgs() << "\n";
  });
  if (Offsets.size() == 1) {
    setNotDecomposing(0, "reads and writes in overlapping regions");
    LLVM_DEBUG(dbgs() << "no decomposition\n");
    return false;
  }
  return true;
}

/***********************************************************************
 * addToWeb : add value to current vector web, adjusting decompose size
 *    if it is a wrregion
 *
 * Enter:   V = value to add (if it is an instruction)
 *          User = instruction V is used in, for reporting failure to
 *                decompose if V is an Argument
 */
void VectorDecomposer::addToWeb(Value *V, Instruction *User) {
  if (isa<Constant>(V))
    return;
  auto Inst = dyn_cast<Instruction>(V);
  if (!Inst) {
    // Cannot decompose with an arg in the web.
    setNotDecomposing(User, "use of function argument");
    return;
  }
  if (Seen.find(Inst) != Seen.end())
    return; // already in the web
  // Add to the web.
  LLVM_DEBUG(dbgs() << "  addToWeb(" << V->getName() << ")\n");
  Seen.insert(Inst);
  Web.push_back(Inst);
  if (!GenXIntrinsic::isWrRegion(Inst))
    return;
  // It is a wrregion. Adjust decomposition.
  adjustDecomposition(Inst);
}

/***********************************************************************
 * adjustDecomposition : adjust web decomposition for region
 *
 * Enter:   Inst = rdregion or wrregion instruction
 *
 * The vector will be decomposed into contiguous blocks of GRFs. This
 * detects if the region accesses multiple GRFs currently slated to be in
 * different decomposed vectors, and if so marks them as needing to be
 * in the same decomposed vector.
 */
void VectorDecomposer::adjustDecomposition(Instruction *Inst) {
  if (Decomposition.empty())
    return; // Decomposition[] not set up yet
  vc::Region R = makeRegionFromBaleInfo(Inst, BaleInfo());
  if (R.Indirect) {
    setNotDecomposing(Inst, "indirect region");
    return; // cannot decompose if indirect
  }
  if (NotDecomposing)
    return; // decomposition of this vector already disabled
  // Compute byte offset of last byte accessed in the region. (This is after
  // legalization so we can assume that strides are non-negative.)
  unsigned Last = 0;
  if (R.Width != R.NumElements)
    Last = (R.NumElements / R.Width - 1) * R.VStride;
  Last += (R.Width - 1) * R.Stride;
  Last = R.Offset + Last * R.ElementBytes;
  // Compute the GRF number of the first and last byte of the region.
  unsigned First = R.Offset / GRFByteSize;
  Last /= GRFByteSize;
  if ((First >= Decomposition.size()) || (Last >= Decomposition.size())) {
    setNotDecomposing(Inst, "out-of-bounds");
    return; // don't attempt to decompose out-of-bounds accesses
  }
  if (First != Last) {
    // This region spans more than one GRF. Ensure they are all in the same
    // decomposed vector.
    for (unsigned i = Last + 1;
         i != Decomposition.size() && Decomposition[i] == Decomposition[Last];
         ++i)
      Decomposition[i] = Decomposition[First];
    for (unsigned i = First + 1; i != Last + 1; ++i)
      Decomposition[i] = Decomposition[First];
  }
}

/***********************************************************************
 * reportLocation : report location of a DebugLoc, with nested inline funcs
 */
static void reportLocation(const LLVMContext &Ctx, const DebugLoc &DL,
                           raw_ostream &OS) {
  if (auto InlinedAt = DL.getInlinedAt()) {
    reportLocation(Ctx, DebugLoc(InlinedAt), OS);
    OS << ": in function inlined here:\n";
  }
  StringRef Filename = "<unknown>";
  unsigned Line = 0;
  unsigned Col = 0;
  if (!DL) {
    Filename = DL->getFilename();
    Line = DL.getLine();
    Col = DL.getCol();
  }
  OS << Filename;
  if (Line) {
    OS << ":" << Line;
    if (Col)
      OS << ":" << Col;
  }
}

static DILocalVariable *getVariable(IntrinsicInst *II) {
  do {
    Value *V = II->getOperand(0);
    Metadata *M = ValueAsMetadata::get(V);
    if (auto DbgNode = MetadataAsValue::getIfExists(V->getContext(), M))
      for (auto *U : DbgNode->users())
        if (auto DVI = dyn_cast<DbgValueInst>(U))
          return DVI->getVariable();
    if (!GenXIntrinsic::isWrRegion(V))
      break;
    II = cast<IntrinsicInst>(V);
  } while (1);

  return nullptr;
}

/***********************************************************************
 * setNotDecomposing : set NotDecomposing flag and report to user
 *
 * Enter:   Inst = instruction to report at (0 to use same location as
 *                  NotDecomposingReportInst, the "first write" to the web)
 *          Text = message
 */
void VectorDecomposer::setNotDecomposing(Instruction *Inst, const char *Text) {
  NotDecomposing = true;
  if (NotDecomposingReportInst) {
    unsigned Bytes =
        NotDecomposingReportInst->getType()->getPrimitiveSizeInBits() / 8U;
    if (Bytes < GenXReportVectorDecomposerFailureThreshold)
      return;
    reportLocation(Inst->getContext(), NotDecomposingReportInst->getDebugLoc(),
                   dbgs());
    dbgs() << ": in decomposition candidate (" << Bytes
           << " byte vector/matrix) written to here:\n";
    NotDecomposingReportInst = nullptr;
  }
  if (!Inst)
    Inst = NotDecomposingReportInst;
  IGC_ASSERT_EXIT(Inst);
  if (Inst->getDebugLoc())
    Inst = Inst->getParent()->getFirstNonPHI();
  reportLocation(Inst->getContext(), Inst->getDebugLoc(), dbgs());
  dbgs() << ": vector decomposition failed because: " << Text << "\n";
}

/***********************************************************************
 * VectorDecomposer::decompose : decompose web of vectors in Web based on
 *  Decomposition[] and Offsets[]
 */
void VectorDecomposer::decompose() {
  // For each phi node in the web, create a phi node for each decomposed
  // part, with all incomings set to the decomposed part of the original
  // incoming if it was constant, otherwise undef.
  for (auto wi = Web.begin(), we = Web.end(); wi != we; ++wi) {
    auto Phi = dyn_cast<PHINode>(*wi);
    if (!Phi)
      continue;
    auto PhiPartsEntry = &PhiParts[Phi];
    auto Undef = UndefValue::get(Phi->getType());
    unsigned NumIncomings = Phi->getNumIncomingValues();
    for (unsigned PartIndex = 0; PartIndex != Offsets.size(); ++PartIndex) {
      auto PartTy = getPartType(Phi->getType(), PartIndex);
      auto NewPhi =
          PHINode::Create(PartTy, NumIncomings,
                          Phi->getName() + ".decomp." + Twine(PartIndex), Phi);
      for (unsigned ii = 0; ii != NumIncomings; ++ii) {
        auto Incoming = dyn_cast<Constant>(Phi->getIncomingValue(ii));
        if (!Incoming)
          Incoming = Undef;
        Incoming = getConstantPart(Incoming, PartIndex);
        NewPhi->addIncoming(Incoming, Phi->getIncomingBlock(ii));
      }
      NewInsts.push_back(NewPhi);
      PhiPartsEntry->push_back(NewPhi);
    }
  }
  // Shorten the list of instructions in Web so it only includes phi nodes
  // and start wrregions (ones with constant input). We need to do this first
  // because other instructions in the web may become erased so checking them
  // in the "decompose each tree of values" loop is invalid.
  unsigned NewLen = 0;
  for (unsigned wi = 0, we = Web.size(); wi != we; ++wi) {
    Instruction *Inst = Web[wi];
    if (isa<PHINode>(Inst) ||
        (GenXIntrinsic::isWrRegion(Inst) && isa<Constant>(Inst->getOperand(0))))
      Web[NewLen++] = Inst;
  }
  Web.resize(NewLen);
  // Decompose each tree of values in the web rooted at a start wrregion (one
  // with constant input) or at each use of a phi node. Each tree can be
  // done independently, as we have already put the phi nodes in place to link
  // them together.
  for (auto wi = Web.begin(), we = Web.end(); wi != we; ++wi) {
    Instruction *Inst = *wi;
    if (auto Phi = dyn_cast<PHINode>(Inst)) {
      auto Parts = &PhiParts[Phi];
      // decomposeTree removes the use, so we repeatedly process the first use
      // until they have all gone.
      while (!Phi->use_empty())
        decomposeTree(&*Phi->use_begin(), Parts);
    } else {
      IGC_ASSERT(GenXIntrinsic::isWrRegion(Inst) &&
                 isa<Constant>(Inst->getOperand(0)));
      decomposeTree(&Inst->getOperandUse(0), nullptr);
    }
  }
  // Erase original phi nodes. (The other original instructions in the web have
  // been erased already.)
  for (auto pi = PhiParts.begin(), pe = PhiParts.end(); pi != pe; ++pi)
    eraseInst(pi->first);
  // Do an aggressive dead code removal pass on instructions that we have added.
  removeDeadCode();
}

/***********************************************************************
 * VectorDecomposer::decomposeTree : decompose vectors in a tree
 *
 * Enter:   U = use at the root of the tree, one of:
 *              - the "old value" operand of wrregion (might be constant)
 *              - the "old value" operand of rdregion
 *              - the input of bitcast
 *              - a phi incoming
 *          PartsIn = decomposed parts of input (not modifiable)
 *                    (0 if *U is constant)
 *
 * This is a tree of wrregion and bitcast instructions, with phi node uses
 * and rdregions at the leaves.
 *
 * This function traverses the tree using self recursion.
 */
void VectorDecomposer::decomposeTree(Use *U,
                                     const SmallVectorImpl<Value *> *PartsIn) {
  auto Inst = cast<Instruction>(U->getUser());
  if (auto Phi = dyn_cast<PHINode>(Inst)) {
    decomposePhiIncoming(Phi, U->getOperandNo(), PartsIn);
    return;
  }
  IGC_ASSERT(!U->getOperandNo());
  if (GenXIntrinsic::isRdRegion(Inst)) {
    decomposeRdRegion(Inst, PartsIn);
    return;
  }
  // Set up the decomposed parts of the incoming value.
  SmallVector<Value *, 8> Parts;
  if (PartsIn)
    for (unsigned i = 0, e = PartsIn->size(); i != e; ++i)
      Parts.push_back((*PartsIn)[i]);
  else
    for (unsigned i = 0, e = Offsets.size(); i != e; ++i)
      Parts.push_back(getConstantPart(cast<Constant>(*U), i));
  // Handle bitcast.
  if (isa<BitCastInst>(Inst)) {
    decomposeBitCast(Inst, &Parts);
    return;
  }
  // Handle wrregion.
  IGC_ASSERT(GenXIntrinsic::isWrRegion(Inst));
  decomposeWrRegion(Inst, &Parts);
}

/***********************************************************************
 * VectorDecomposer::decomposePhiIncoming : decompose a use in a phi node
 *
 * Enter:   Phi = the phi node
 *          OperandNum = operand number in the phi node
 *          PartsIn = decomposed parts of input (not modifiable)
 */
void VectorDecomposer::decomposePhiIncoming(
    PHINode *Phi, unsigned OperandNum,
    const SmallVectorImpl<Value *> *PartsIn) {
  // For each part, find the decomposed phi node and set its
  // corresponding incoming.
  auto PhiPartsEntry = &PhiParts[Phi];
  for (unsigned PartIndex = 0, NumParts = PartsIn->size();
       PartIndex != NumParts; ++PartIndex) {
    auto PhiPart = cast<PHINode>((*PhiPartsEntry)[PartIndex]);
    PhiPart->setIncomingValue(OperandNum, (*PartsIn)[PartIndex]);
  }
  // Set the incoming in the original phi node to undef, to remove the use.
  Phi->setIncomingValue(OperandNum, UndefValue::get(Phi->getType()));
}

/***********************************************************************
 * VectorDecomposer::decomposeRdRegion : decompose a rdregion
 *
 * Enter:   RdRegion = the rdregion instruction
 *          PartsIn = decomposed parts of input (not modifiable)
 */
void VectorDecomposer::decomposeRdRegion(
    Instruction *RdRegion, const SmallVectorImpl<Value *> *PartsIn) {
  vc::Region RdR = makeRegionFromBaleInfo(RdRegion, BaleInfo());
  unsigned PartIndex = getPartIndex(&RdR);
  Value *Part = (*PartsIn)[PartIndex];
  if (isa<UndefValue>(Part)) {
    // Check if this region read is used as a two addr operand.
    auto isUsedInTwoAddr = [](Value *V) {
      for (auto ui = V->use_begin(), ue = V->use_end(); ui != ue; ++ui) {
        auto user = cast<Instruction>(ui->getUser());
        if (auto CI = dyn_cast<CallInst>(user)) {
          if (auto OpndNum = getTwoAddressOperandNum(CI);
              OpndNum && *OpndNum == ui->getOperandNo())
            return true;
        }
      }
      return false;
    };

    // Do not emit a warning if this undef is being used as old value
    // in a two-addr instruction.
    if (!isUsedInTwoAddr(RdRegion)) {
      if (auto N = getVariable(cast<IntrinsicInst>(RdRegion))) {
        emitWarning(RdRegion, "undefined value from '" + N->getName() +
                                  "' is referenced after decomposition");
      } else
        emitWarning(RdRegion,
                    "undefined value is referenced after decomposition");
    }
  }
  if (RdRegion->getType() == Part->getType() && RdR.isContiguous() &&
      isa<VectorType>(RdRegion->getType())) {
    // The rdregion reads the whole of the decomposed part of the vector (and
    // has a vector result even if single element).
    // Just replace uses and erase.
    RdRegion->replaceAllUsesWith(Part);
    eraseInst(RdRegion);
    return;
  }
  // The rdregion reads only some of the decomposed part of the vector.
  // Create a new rdregion to replace the old one, taking its name.
  RdR.Offset -= getPartOffset(PartIndex);
  auto NewRdRegion =
      RdR.createRdRegion(Part, "", RdRegion, RdRegion->getDebugLoc(),
                         /*AllowScalar=*/!isa<VectorType>(RdRegion->getType()));
  NewRdRegion->takeName(RdRegion);
  RdRegion->replaceAllUsesWith(NewRdRegion);
  IGC_ASSERT(Seen.find(RdRegion) == Seen.end());
  eraseInst(RdRegion);
}

/***********************************************************************
 * VectorDecomposer::decomposeWrRegion : decompose a wrregion
 *
 * Enter:   WrRegion = the wrregion instruction
 *          Parts = decomposed parts of input (modifiable)
 */
void VectorDecomposer::decomposeWrRegion(Instruction *WrRegion,
                                         SmallVectorImpl<Value *> *Parts) {
  vc::Region WrR = makeRegionFromBaleInfo(WrRegion, BaleInfo());
  unsigned PartIndex = getPartIndex(&WrR);
  Value *Part = (*Parts)[PartIndex];
  if (WrRegion->getOperand(NewValueOperandNum)->getType() == Part->getType() &&
      !WrR.Mask) {
    // The wrregion writes the whole of the decomposed part of the vector.
    // We can just directly replace the part.
    (*Parts)[PartIndex] = WrRegion->getOperand(NewValueOperandNum);
  } else {
    // The wrregion writes only some of the decomposed part of the vector.
    // Create a new wrregion.
    WrR.Offset -= getPartOffset(PartIndex);
    auto NewInst =
        WrR.createWrRegion(Part, WrRegion->getOperand(NewValueOperandNum),
                           WrRegion->getName() + ".decomp." + Twine(PartIndex),
                           WrRegion, WrRegion->getDebugLoc());
    (*Parts)[PartIndex] = NewInst;
    NewInsts.push_back(NewInst);
  }
  // Decompose its uses. decomposeTree removes the use, so we repeatedly process
  // the first use until they have all gone.
  while (!WrRegion->use_empty())
    decomposeTree(&*WrRegion->use_begin(), Parts);
  // Now the original wrregion has no uses, and we can remove it.
  eraseInst(WrRegion);
}

/***********************************************************************
 * VectorDecomposer::decomposeBitCast : decompose a bitcast
 *
 * Enter:   Inst = the bitcast instruction
 *          Parts = decomposed parts of input (modifiable)
 */
void VectorDecomposer::decomposeBitCast(Instruction *Inst,
                                        SmallVectorImpl<Value *> *Parts) {
  // Create a new bitcast for each decomposed part, other than when the part
  // is undef. (We handle the undef case as it is common, when only some of the
  // vector has been set up. Other constant cases we leave to the EarlyCSE pass
  // that comes after this pass.)
  for (unsigned PartIndex = 0, NumParts = Parts->size(); PartIndex != NumParts;
       ++PartIndex) {
    Type *NewTy = getPartType(Inst->getType(), PartIndex);
    if (isa<UndefValue>((*Parts)[PartIndex]))
      (*Parts)[PartIndex] = UndefValue::get(NewTy);
    else {
      auto NewInst = CastInst::Create(
          Instruction::BitCast, (*Parts)[PartIndex], NewTy,
          Inst->getName() + ".decomp." + Twine(PartIndex), Inst);
      NewInst->setDebugLoc(Inst->getDebugLoc());
      NewInsts.push_back(NewInst);
      (*Parts)[PartIndex] = NewInst;
    }
  }
  // Decompose its uses. decomposeTree removes the use, so we repeatedly process
  // the first use until they have all gone.
  while (!Inst->use_empty())
    decomposeTree(&*Inst->use_begin(), Parts);
  // Now the original wrregion has no uses, and we can remove it.
  eraseInst(Inst);
}

/***********************************************************************
 * VectorDecomposer::getPartIndex : get the part index for the region
 */
unsigned VectorDecomposer::getPartIndex(vc::Region *R) {
  return Decomposition[R->Offset / GRFByteSize];
}

/***********************************************************************
 * VectorDecomposer::getPartOffset : get the byte offset of a part
 */
unsigned VectorDecomposer::getPartOffset(unsigned PartIndex) {
  // Offsets[] has the index in GRFs.
  return Offsets[PartIndex] * GRFByteSize;
}

/***********************************************************************
 * VectorDecomposer::getPartNumBytes : get the size of a part in bytes
 */
unsigned VectorDecomposer::getPartNumBytes(Type *WholeTy, unsigned PartIndex) {
  if (PartIndex + 1 != Offsets.size()) {
    // Not the last part. We can use the offset (in GRFs) difference.
    return GRFByteSize * (Offsets[PartIndex + 1] - Offsets[PartIndex]);
  }
  // For the last part, we need to get the total size from WholeTy.
  return DL->getTypeSizeInBits(WholeTy) / genx::ByteBits -
         GRFByteSize * Offsets[PartIndex];
}

/***********************************************************************
 * VectorDecomposer::getPartNumElements : get the size of a part in elements
 */
unsigned VectorDecomposer::getPartNumElements(Type *WholeTy,
                                              unsigned PartIndex) {
  Type *ElementTy = WholeTy->getScalarType();
  return getPartNumBytes(WholeTy, PartIndex) /
         (DL->getTypeSizeInBits(ElementTy) / genx::ByteBits);
}

/***********************************************************************
 * VectorDecomposer::getPartType : get the type of a part
 */
VectorType *VectorDecomposer::getPartType(Type *WholeTy, unsigned PartIndex) {
  Type *ElementTy = WholeTy->getScalarType();
  return IGCLLVM::FixedVectorType::get(ElementTy,
                                       getPartNumElements(WholeTy, PartIndex));
}

/***********************************************************************
 * VectorDecomposer::getConstantPart : get the decomposed part of a constant
 */
Constant *VectorDecomposer::getConstantPart(Constant *Whole,
                                            unsigned PartIndex) {
  vc::Region R(Whole, DL);
  R.Offset = getPartOffset(PartIndex);
  R.NumElements = R.Width = getPartNumElements(Whole->getType(), PartIndex);
  return R.evaluateConstantRdRegion(Whole, /*AllowScalar=*/false);
}

/***********************************************************************
 * VectorDecomposer::removeDeadCode : aggressive dead code removal on
 *    instructions added by the vector decomposer
 *
 * NewInsts contains the instructions added.
 */
void VectorDecomposer::removeDeadCode() {
  SmallVector<Instruction *, 8> Stack; // the "to be processed" stack
  std::set<Instruction *> Unused;
  // Put all newly added instructions into the Unused set.
  for (auto i = NewInsts.begin(), e = NewInsts.end(); i != e; ++i)
    Unused.insert(*i);
  // Look at each newly added instruction. If it is used in anything other than
  // one of our newly added instructions, add it to the "to be processed" stack
  // and remove it from the Unused set. (It also counts as used an instruction
  // that is used in another of our newly added instructions that happens to
  // have already been seen as used. It doesn't matter either way that this
  // happens.)
  for (auto i = NewInsts.begin(), e = NewInsts.end(); i != e; ++i) {
    Instruction *Inst = *i;
    bool IsUsed = false;
    for (auto ui = Inst->use_begin(), ue = Inst->use_end(); ui != ue; ++ui) {
      auto user = cast<Instruction>(ui->getUser());
      if (Unused.find(user) == Unused.end())
        IsUsed = true;
    }
    if (IsUsed) {
      Stack.push_back(Inst);
      Unused.erase(Inst);
    }
  }
  // Process each entry on the stack.
  while (!Stack.empty()) {
    Instruction *Inst = Stack.back();
    Stack.pop_back();
    // Inst is used, perhaps indirectly, by something outside the web.
    // Mark instructions it uses as used. For wrregion and bitcast, this
    // is just operand 0. For a phi node, it is all incomings.
    if (auto Phi = dyn_cast<PHINode>(Inst)) {
      for (unsigned ii = 0, ie = Phi->getNumIncomingValues(); ii != ie; ++ii) {
        auto Incoming = dyn_cast<Instruction>(Phi->getIncomingValue(ii));
        auto it = Unused.find(Incoming);
        if (it == Unused.end())
          continue;
        // Incoming is an instruction currently in the unused set. Remove it
        // from the set, and add it to the "to be processed" stack.
        Unused.erase(it);
        Stack.push_back(Incoming);
      }
    } else {
      auto Operand = dyn_cast<Instruction>(Inst->getOperand(0));
      auto it = Unused.find(Operand);
      if (it != Unused.end()) {
        // Operand is an instruction currently in the unused set. Remove it
        // from the set, and add it to the "to be processed" stack.
        Unused.erase(it);
        Stack.push_back(Operand);
      }
    }
  }
  // Anything left in Unused is really unused, except for uses by other
  // instructions in Unused (possibly circularly in the case of phi nodes).
  // Erase them all forcibly, by changing all uses to undef first.
  for (auto uui = Unused.begin(), uue = Unused.end(); uui != uue; ++uui) {
    Instruction *Inst = *uui;
    while (!Inst->use_empty())
      *Inst->use_begin() = UndefValue::get((*Inst->use_begin())->getType());
    eraseInst(Inst);
  }
}

/***********************************************************************
 * VectorDecomposer::eraseInst : erase an instruction
 *
 * This is used in the case that the instruction might be in the Seen
 * set. So we delay actually deleting it until the end of processing the
 * function.
 */
void VectorDecomposer::eraseInst(Instruction *Inst) {
  Inst->removeFromParent();
  ToDelete.push_back(Inst);
  // Remove all non-constant operands.
  for (unsigned i = 0, e = Inst->getNumOperands(); i != e; ++i) {
    Value *Opnd = Inst->getOperand(i);
    if (isa<Constant>(Opnd))
      continue;
    Inst->setOperand(i, UndefValue::get(Opnd->getType()));
  }
}

void VectorDecomposer::emitWarning(Instruction *Inst, const Twine &Msg) {
  DiagnosticVectorDecomposition Warn(Inst, Msg, DS_Warning);
  Inst->getContext().diagnose(Warn);
}

// Decompose
//
// %33 = fcmp une <24 x float> %25, zeroinitializer
// %34 = fcmp oeq <24 x float> %24, zeroinitializer
// %35 = and <24 x i1> %33, %34
// %36 = select <24 x i1> %35, <24 x float> <float 0x4071E7A300000000, >
//
// into
//
// %25.0 = rrd(%25, 16, 16, 1)
// %25.1 = rrd(%25, 8, 8, 1)
// %24.0 = rrd(%24, 16, 16, 1)
// %24.1 = rrd(%24, 8, 8, 1)
// %33.0 = fcmp une <16 x float> %25.0, zeroinitializer
// %33.1 = fcmp une <8 x float> %25.1, zeroinitializer
// %34.0 = fcmp oeq <16 x float> %24.0, zeroinitializer
// %34.1 = fcmp oeq <8 x float> %24.1, zeroinitializer
// %35.0 = and <16 x i1> %33.0, %34.0
// %35.1 = and <8 x i1> %33.1, %34.1
// $36.0 = select <16 x i1> %35.0, <16 x float> <float 0x4071E7A300000000, >
// %36.1 = select <8 x i1> %35.1, <8 x float> <float 0x4071E7A300000000, >
// %36.new.0 = wrr(<24 x float> undef, <16 x float> %36.0, 0)
// %36.new.1 = wwr(<24 x float> %36.new.0, <8 x float> %36.1, 16)
//
// This allows register pressure reducer to better reorder the above sequence.
//
bool SelectDecomposer::run() {
  bool Modified = false;
  for (auto Inst : StartSelects) {
    Modified |= processStartSelect(Inst);
    clear();
  }
  return Modified;
}

bool SelectDecomposer::processStartSelect(Instruction *Inst) {
  auto SI = dyn_cast<SelectInst>(Inst);
  if (!SI || !determineDecomposition(Inst))
    return false;

  // Decompose it and its predicate computation recursively.
  decompose(SI);

  // Merge components, starting with undef.
  SmallVectorImpl<Value *> &Parts = DMap[Inst];
  Value *NewInst = UndefValue::get(Inst->getType());
  for (unsigned Idx = 0, N = Decomposition.size(); Idx < N; ++Idx) {
    vc::Region R(NewInst);
    R.getSubregion(getPartOffset(Idx), getPartNumElements(Idx));
    NewInst = R.createWrRegion(NewInst, Parts[Idx], ".join", Inst,
                               Inst->getDebugLoc());
  }
  Inst->replaceAllUsesWith(NewInst);
  return true;
}

template <typename T> bool isGlobalVarOperand(const Value *V) {
  const T *Inst = dyn_cast<T>(V);
  return Inst &&
         vc::getUnderlyingGlobalVariable(Inst->getPointerOperand()) != nullptr;
}

bool SelectDecomposer::determineDecomposition(Instruction *Inst) {
  auto SI = dyn_cast<SelectInst>(Inst);
  IGC_ASSERT_MESSAGE(SI, "select expected");
  auto *Ty = dyn_cast<IGCLLVM::FixedVectorType>(SI->getCondition()->getType());
  if (!Ty)
    return false;
  unsigned NumElts = Ty->getNumElements();
  if (NumElts <= 16)
    return false;
  if (!isa<Instruction>(SI->getCondition()))
    return false;

  // Disable select decomposition if this select may be used in g_store bale.
  // Otherwise, g_store bale cannot be created correctly due to a missing load
  // of a global that will be stored(it is one of the requirements to g_store
  // bales). The change fixes FRC_global and FRC_MC_global tests.
  if (std::any_of(Inst->user_begin(), Inst->user_end(),
                  isGlobalVarOperand<StoreInst>) ||
      std::any_of(Inst->value_op_begin(), Inst->value_op_end(),
                  isGlobalVarOperand<LoadInst>))
    return false;

  // Extra checks to avoid aggressive splitting.
  auto BB = Inst->getParent();
  auto check = [=](Instruction *I) {
    if (!I->hasOneUse() || I->getParent() != BB) {
      setNotDecomposing();
      return false;
    }
    return true;
  };

  // This determines the width of predicate operands.
  // We consider the following two factors
  // - The type size of sel
  // - The input operands
  unsigned Width = GenXDefaultSelectPredicateWidth;
  if (Width > 32)
    Width = 32;
  else if (Width < 16)
    Width = 16;
  else if (SI->getType()->getScalarSizeInBits() >= 32)
    Width = 16;

  // If there is a region read with a non-unit stride,
  // then adjust the splitting width appropriately.
  auto adjustWidth = [=, &Width](Value *V) {
    // If this region read only supports up to 16, then do not split into
    // simd 32. Otherwise it makes difficult to bale in this region read.
    if (Width == 32 && GenXIntrinsic::isRdRegion(V)) {
      CallInst *CI = cast<CallInst>(V);
      vc::Region R = makeRegionFromBaleInfo(CI, BaleInfo());
      IGC_ASSERT(ST);
      unsigned LegalSize = getLegalRegionSizeForTarget(
          *ST, R, 0 /*idx*/, true /*Allow2D*/, true /*UseRealIdx*/,
          cast<IGCLLVM::FixedVectorType>(CI->getOperand(0)->getType())
              ->getNumElements());
      if (LegalSize < 32)
        Width = 16;
    }
  };

  addToWeb(SI->getCondition());
  for (unsigned i = 0; i != Web.size(); ++i) {
    Inst = Web[i];
    if (!check(Inst))
      break;
    unsigned OpCode = Inst->getOpcode();
    switch (OpCode) {
    default:
      setNotDecomposing();
      break;
    case Instruction::And:
    case Instruction::Or:
    case Instruction::Xor:
      addToWeb(Inst->getOperand(0));
      addToWeb(Inst->getOperand(1));
      adjustWidth(Inst->getOperand(0));
      adjustWidth(Inst->getOperand(1));
      break;
    case Instruction::FCmp:
    case Instruction::ICmp:
      adjustWidth(Inst->getOperand(0));
      adjustWidth(Inst->getOperand(1));
      break;
    }
  }

  if (NotDecomposing)
    return false;

  Offsets.clear();
  unsigned Offset = 0;
  unsigned Remaining = NumElts;
  while (Remaining > Width) {
    Decomposition.push_back(Width);
    Offsets.push_back(Offset);
    Remaining -= Width;
    Offset += Width;
  }
  if (Remaining > 0) {
    Decomposition.push_back(Remaining);
    Offsets.push_back(Offset);
  }

  {
    IGC_ASSERT(Width);
    unsigned NumParts = 0; // it will be assigned inside assertion statament
    IGC_ASSERT((NumParts = (NumElts + Width - 1) / Width, 1));
    IGC_ASSERT(NumParts == Decomposition.size());
    IGC_ASSERT(NumParts == Offsets.size());
    (void)NumParts;
  }

  return true;
}

void SelectDecomposer::addToWeb(Value *V) {
  if (isa<Constant>(V))
    return;
  auto Inst = dyn_cast<Instruction>(V);
  if (!Inst) {
    // Cannot decompose with an argument in the web.
    setNotDecomposing();
    return;
  }
  if (Seen.find(Inst) != Seen.end())
    return;

  Seen.insert(Inst);
  Web.push_back(Inst);
}

void SelectDecomposer::decompose(Instruction *Inst) {
  if (isa<SelectInst>(Inst))
    decomposeSelect(Inst);
  else if (isa<CmpInst>(Inst))
    decomposeCmp(Inst);
  else {
    IGC_ASSERT(Inst->getOpcode() == Instruction::And ||
               Inst->getOpcode() == Instruction::Or ||
               Inst->getOpcode() == Instruction::Xor);
    decomposeBinOp(Inst);
  }
}

void SelectDecomposer::decomposeSelect(Instruction *Inst) {
  SelectInst *SI = cast<SelectInst>(Inst);
  if (auto I = dyn_cast<Instruction>(SI->getCondition()))
    decompose(I);

  unsigned N = Decomposition.size();
  SmallVector<Value *, 8> Parts(N);
  IRBuilder<> B(Inst);

  Value *OpC = SI->getCondition();
  Value *OpT = SI->getTrueValue();
  Value *OpF = SI->getFalseValue();

  for (unsigned Idx = 0; Idx < N; ++Idx) {
    Value *OpC_I = getPart(OpC, Idx, Inst);
    Value *OpT_I = getPart(OpT, Idx, Inst);
    Value *OpF_I = getPart(OpF, Idx, Inst);
    Value *NewInst = B.CreateSelect(OpC_I, OpT_I, OpF_I, Inst->getName());
    if (auto I = dyn_cast<Instruction>(NewInst))
      I->setDebugLoc(Inst->getDebugLoc());
    Parts[Idx] = NewInst;
  }

  DMap[Inst].swap(Parts);
}

void SelectDecomposer::decomposeBinOp(Instruction *Inst) {
  Value *Op0 = Inst->getOperand(0);
  Value *Op1 = Inst->getOperand(1);
  if (auto I = dyn_cast<Instruction>(Op0))
    decompose(I);
  if (auto I = dyn_cast<Instruction>(Op1))
    decompose(I);

  unsigned N = Decomposition.size();
  SmallVector<Value *, 8> Parts(N);
  IRBuilder<> B(Inst);

  for (unsigned Idx = 0; Idx < N; ++Idx) {
    Value *Op0_I = getPart(Op0, Idx, Inst);
    Value *Op1_I = getPart(Op1, Idx, Inst);
    Value *NewInst = B.CreateBinOp(Instruction::BinaryOps(Inst->getOpcode()),
                                   Op0_I, Op1_I, Inst->getName());
    if (auto I = dyn_cast<Instruction>(NewInst))
      I->setDebugLoc(Inst->getDebugLoc());
    Parts[Idx] = NewInst;
  }

  DMap[Inst].swap(Parts);
}

void SelectDecomposer::decomposeCmp(Instruction *Inst) {
  Value *Op0 = Inst->getOperand(0);
  Value *Op1 = Inst->getOperand(1);

  unsigned N = Decomposition.size();
  SmallVector<Value *, 8> Parts(N);
  IRBuilder<> B(Inst);
  CmpInst *CI = cast<CmpInst>(Inst);

  for (unsigned Idx = 0; Idx < N; ++Idx) {
    Value *Op0_I = getPart(Op0, Idx, Inst);
    Value *Op1_I = getPart(Op1, Idx, Inst);
    Value *NewInst = nullptr;
    if (isa<ICmpInst>(CI))
      NewInst = B.CreateICmp(CI->getPredicate(), Op0_I, Op1_I, Inst->getName());
    else
      NewInst = B.CreateFCmp(CI->getPredicate(), Op0_I, Op1_I, Inst->getName());
    if (auto I = dyn_cast<Instruction>(NewInst))
      I->setDebugLoc(Inst->getDebugLoc());
    Parts[Idx] = NewInst;
  }

  DMap[Inst].swap(Parts);
}

Value *SelectDecomposer::getPart(Value *Whole, unsigned PartIndex,
                                 Instruction *Inst) const {
  auto I = DMap.find(Whole);
  if (I != DMap.end()) {
    IGC_ASSERT(I->second.size() > PartIndex);
    return I->second[PartIndex];
  }

  unsigned Offset = getPartOffset(PartIndex);
  unsigned NumElts = getPartNumElements(PartIndex);

  if (Whole->getType()->getScalarType()->isIntegerTy(1)) {
    auto C = dyn_cast<Constant>(Whole);
    IGC_ASSERT_MESSAGE(C, "constant expected");
    if (Constant *V = C->getSplatValue())
      return ConstantVector::getSplat(IGCLLVM::getElementCount(NumElts), V);
    SmallVector<Constant *, 8> Values;
    for (unsigned Idx = Offset; Idx < Offset + NumElts; ++Idx)
      Values.push_back(C->getAggregateElement(Idx));
    return ConstantVector::get(Values);
  }

  const DataLayout &DL = Inst->getModule()->getDataLayout();
  vc::Region R(Whole, &DL);
  R.Offset = Offset * R.ElementBytes;
  R.NumElements = R.Width = NumElts;

  if (auto C = dyn_cast<Constant>(Whole))
    return R.evaluateConstantRdRegion(C, /*AllowScalar=*/false);
  return R.createRdRegion(Whole, ".in", Inst, Inst->getDebugLoc());
}
