/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXLegalization
/// ----------------
///
/// GenXLegalization is a function pass that splits vector instructions
/// up to make execution widths legal, and to ensure that the GRF crossing rules
/// are satisfied.
///
/// This pass makes the LLVM IR closer to legal vISA by
/// splitting up any instruction that has an illegal vector width (too big or
/// non power of two) or an illegal region (illegal vstride/width/stride or
/// illegal GRF crossing).
///
/// **IR restriction**: After this pass, LLVM IR represents vISA instructions
/// with legal execution width and region parameters, and with any particular
/// instruction's region restrictions adhered to.
///
/// The pass uses the instruction baling information to tell which
/// regions an instruction has. Splitting an instruction and its regions needs
/// to be done with reference to all the regions at the same time, as they may
/// need splitting at different points.
///
/// For general values, an illegal width instruction is split by
/// creating narrower instructions, each of which uses a rdregion to extract the
/// subregion for each source operand, and then uses a wrregion to insert the
/// resulting subregion into the original destination value. The original
/// illegal width values survive, and that is OK because a vISA register can
/// have any vector width.
///
/// The pass uses the hasIndirectGRFCrossing feature from GenXSubtarget when
/// calculating whether a region is legal, or how a region needs to be split, in
/// the case that the region is indirect.
///
/// The legalization pass considers a bale of instructions as a separate
/// entity which can be split without reference to other bales. This works
/// because the overhead of splitting, which is an extra rdregion per operand
/// and an extra wrregion on the result, is pretty much free in that these extra
/// region accesses are baled in to the split instruction.
///
/// There are some cases where we decide we need to unbale an instruction, i.e.
/// remove it (or rather the subtree of instructions in the bale rooted at it)
/// from the bale, and then re-start the analysis for the bale. This happens
/// when there are two conflicting requirements in the bale, for example a main
/// instruction that needs at least simd4 but a rdregion that can only manage
/// simd2.
///
/// The pass scans backwards through the code, which makes this unbaling a bit
/// easier. An unbaled instruction will be encountered again a bit later, and
/// be processed as its own bale.
///
/// If a source operand being split is already an rdregion, then that rdregion
/// is split, so the new split rdregions read from the original rdregion's
/// input.
///
/// Similarly, if the bale is already headed by an wrregion, it is replaced by
/// the new split wrregions used to join the splits back together.
///
/// BitCast is not split in this pass. A non-category-converting BitCast is
/// always coalesced in GenXCoalescing, so never generates actual code. Thus it
/// does not matter if it has an illegal size.
///
/// Predicate legalization
/// ^^^^^^^^^^^^^^^^^^^^^^
///
/// Predicates (vector of i1) are more complex. A general vISA value can be any
/// vector width, but a predicate can only be a power of two up to 32. Thus the
/// actual predicate values need to be split, not just the reads from and writes
/// to the values.
///
/// Furthermore, although it is possible to read and write a region within a
/// predicate, using H1/H2/Q1..Q4 flags, there are restrictions: the start
/// offset must be 8 aligned (4 aligned for a select or cmp with 64-bit
/// operands), and the size must be no more than the misalignment of the start
/// offset (e.g. for a start offset of 8, the size can be 8 but not 16).
///
/// So this pass splits an arbitrary size predicate value (including predicate
/// phi nodes) into as many as possible 32 bit parts, then descending power of
/// two parts. For example, a predicate of size 37 is split into 32,4,1.
///
/// Then, within each part, a read or write of the predicate can be further
/// split as long as it fits the restrictions above, e.g. a 32 bit part can be
/// read/written in 8 or 16 bit subregions.
///
/// This is achieved in two steps:
///
/// 1. Predicates take part in the main code of GenXLegalization. When deciding
///    how to split a read or write of a predicate, we determine how the
///    predicate value will be split into parts (e.g. the 37 split into 32,4,1
///    example above), then decides how a part could be subregioned if necessary
///    (e.g. the 32 could have a 16 aligned 16 bit region, or an 8 aligned 8 bit
///    region). As well as a maximum, this usually gives a minimum size region.
///    If the rest of the bale cannot achieve that minimum size, then we unbale
///    to avoid the problem and restart the analysis of the bale.
///
/// 2. Then, fixIllegalPredicates() actually divides the illegally sized
///    predicate values, including phi nodes. The splitting in the main part of
///    GenXLegalization ensures that no read or write of a predicate value
///    crosses a part boundary, so it is straightforward to split the values
///    into those parts.
///
/// This is complicated by the case that the IR before legalization has an
/// rdpredregion. This typically happens when a CM select has odd size operands
/// but an i32 mask. Clang codegen bitcasts the i32 mask to v32i1, then does a
/// shufflevector to extract the correct size predicate. GenXLowering turns the
/// shufflevector into rdpredregion. The main code in GenXLegalization splits
/// the rdpredregion into several rdpredregions.
///
/// In that case, we cannot guarantee that fixIllegalPredicates will find legal
/// rdpredregions. For example, suppose the original rdpredregion has a v32i1 as
/// input, and v13i1 as result. It is determined that the 13 bit predicate will
/// be split into 8,4,1 parts. The main GenXLegalization code will generate
/// an rdpredregion from the 32 bit predicate for each part of the 13 bit
/// predicate. However, the rdpredregion for the 1 bit part is illegal, because
/// its start offset is not 8 aligned.
///
/// We currently do not cope with that (it will probably assertion fail
/// somewhere). If we do find a need to cope with it, then the illegal
/// rdpredregion will need to be lowered to bit twiddling code.
///
/// Other tasks of GenXLegalization
/// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
///
/// An additional task of this pass is to lower an any/all intrinsic that is
/// used anywhere other than as the predicate of a scalar wrregion by inserting
/// such a scalar wrregion with a byte 0/1 result and then a compare of that
/// to give an i1.
///
/// A further task of this pass is to lower any predicated wrregion where the
/// value to write is a vector wider than 1 but the predicate is a scalar i1
/// (other than the value 1, which means unpredicated). It inserts code to splat
/// the scalar i1 predicate to v16i1 or v32i1. This is really part of lowering,
/// but we need to do it here because in GenXLowering the value to write might
/// be wider than 32.
///
/// An extra optimization performed in this pass is to transform a move (that
/// is, a lone wrregion or lone rdregion or a rdregion+wrregion baled together)
/// with a byte element type into the equivalent short or int move. This saves
/// the jitter having to split the byte move into even and odd halves. This
/// optimization needs to be done when baling info is available, so legalization
/// is a handy place to put it.
///
//===----------------------------------------------------------------------===//
#include "GenX.h"
#include "GenXAlignmentInfo.h"
#include "GenXBaling.h"
#include "GenXIntrinsics.h"
#include "GenXLiveElements.h"
#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"
#include "GenXUtil.h"

#include "vc/Support/GenXDiagnostic.h"

#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/Function.h"
#include "llvmWrapper/IR/Instructions.h"
#include <llvmWrapper/Support/MathExtras.h>
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/InitializePasses.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/Utils/Local.h"

#include "llvmWrapper/IR/DerivedTypes.h"

#define DEBUG_TYPE "GENX_LEGALIZATION"

#include <set>
#include "Probe/Assertion.h"

using namespace llvm;
using namespace genx;

namespace {

// Information on a part of a predicate.
struct PredPart {
  unsigned Offset;
  unsigned Size;
  unsigned PartNum;
};

// min and max legal size for a predicate split
struct LegalPredSize {
  unsigned Min;
  unsigned Max;
};

// GenXLegalization : legalize execution widths and GRF crossing
class GenXLegalization : public FunctionPass {
  enum { DETERMINEWIDTH_UNBALE = 0, DETERMINEWIDTH_NO_SPLIT = 256 };
  GenXBaling *Baling = nullptr;
  GenXLiveElements *LE = nullptr;
  const GenXSubtarget *ST = nullptr;
  DominatorTree *DT = nullptr;
  ScalarEvolution *SE = nullptr;
  // Work variables when in the process of splitting a bale.
  // The Bale being split. (Also info on whether it has FIXED4 and TWICEWIDTH
  // operands.)
  Bale B;
  Use *Fixed4 = nullptr;
  Use *TwiceWidth = nullptr;
  // Map from the original instruction to the split one for the current index.
  std::map<Instruction *, Value *> SplitMap;

  // Consider reading from and writing to the same region in this bale,
  // bale {
  //   W1 = rdr(V0, R)
  //   W2 = op(W1, ...)
  //   V1 = wrd(V0, W2, R)
  // }
  // if splitting the above bale into two bales
  // bale {
  //    W1.0 = rdr(V0, R.0)
  //    W2.0 = op(W1.0, ...)
  //    V1.0 = wrr(V0, W2.0, R.0)
  // }
  // bale {
  //    W1.1 = rdr(V0, R.1)
  //    W2.1 = op(W1.1, ...)
  //    V1.1 = wrr(V1.0, W2.1, R.1)
  // }
  // V1.0 and V0 are live at the same time. This makes copy-coalescing
  // fail and also increases rp by the size of V0.
  //
  // If we can prove that
  // (*) rdr(V0, R.1) == rdr(V1.0, R.1) = rdr(wrr(V0, W2.0, R.0), R.1)
  // then we could split the bale slightly differently:
  // bale {
  //    W1.0 = rdr(V0, R.0)
  //    W2.0 = op(W1.0, ...)
  //    V1.0 = wrr(V0, W2.0, R.0)
  // }
  // bale {
  //    W1.1 = rdr(V1.0, R.1)
  //    W2.1 = op(W1.1, ...)
  //    V1.1 = wrr(V1.0, W2.1, R.1)
  // }
  // If V0 is killed after this bale, then V1.0, V1.1 and V0
  // could be coalesced into a single variable. This is the pattern
  // for in-place operations.
  //
  // To satisfy equation (*), it suffices to prove there is no overlap for any
  // two neighbor subregions. This holds for the following two cases:
  //  (1) 1D direct regions or indirect regions with single offset
  //  (2) 2D direct regions with VStride >= Width, or indirect regions with
  //      single offset.
  //
  // While legalizing a bale ends with a g_store instruction, we produce the
  // following code sequences.
  // bale {
  //   V1 = rdr(V0, 0, 32)
  //   V2 = fadd V1, 1
  //   store V2, p
  // }
  // ===>
  // bale {
  //  V1.0 = rdr(V0, 0, 16)
  //  V2.0 = fadd V1.0, 1
  //  V3.0 = wrr(load(p), V2.0, 0, 16)
  //  store V3.0, p
  // }
  // bale {
  //  V1.1 = rdr(V0, 16, 32)
  //  V2.1 = fadd V1.1, 1
  //  V3.1 = wrr(load(p), V2.1, 16, 32)
  //  store V3.1, p
  // }
  // The instruction stream looks like:
  //
  //  V1.0 = rdr(V0, 0, 16)
  //  V1.1 = rdr(V0, 16, 32)
  //  V2.0 = fadd V1.0, 1
  //  V2.1 = fadd V1.1, 1
  //  V3.0 = wrr(load(p), V2.0, 0, 16)
  //  store V3.0, p
  //  V3.1 = wrr(load(p), V2.1, 16, 32)
  //  store V3.1, p
  //
  // That is, this process does not produce region joins.
  //
  enum SplitKind {
    SplitKind_Normal,      // split bales without propagation.
    SplitKind_Propagation, // split bales with propagation.
    SplitKind_GStore       // split bales end with g_store.
  };
  SplitKind CurSplitKind = SplitKind_Normal;
  // Current instruction in loop in runOnFunction, which gets adjusted if that
  // instruction is erased.
  Instruction *CurrentInst = nullptr;
  // Illegally sized predicate values that need splitting at the end of
  // processing the function.
  SetVector<Instruction *> IllegalPredicates;

public:
  static char ID;
  explicit GenXLegalization() : FunctionPass(ID) { clearBale(); }
  StringRef getPassName() const override {
    return "GenX execution width and GRF crossing legalization";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnFunction(Function &F) override;
  // createPrinterPass : get a pass to print the IR, together with the GenX
  // specific analyses
  Pass *createPrinterPass(raw_ostream &O,
                          const std::string &Banner) const override {
    return createGenXPrinterPass(O, Banner);
  }

private:
  void clearBale() {
    B.clear();
    Fixed4 = nullptr;
    TwiceWidth = nullptr;
  }
  unsigned adjustTwiceWidthOrFixed4(const Bale &B);
  bool checkIfLongLongSupportNeeded(const Instruction *Inst) const;
  void verifyLSCFence(const Instruction *Inst);
  void verifyLSC2D(const Instruction *Inst);
  void verifyType(const Type *Ty, const Instruction *Inst) const;
  bool checkInst(const Instruction *Inst) const;
  bool processInst(Instruction *Inst);
  bool processBale(Instruction *InsertBefore);
  bool noSplitProcessing();
  bool processAllAny(Instruction *Inst, Instruction *InsertBefore);
  bool processBitCastFromPredicate(Instruction *Inst,
                                   Instruction *InsertBefore);
  bool processBitCastToPredicate(Instruction *Inst, Instruction *InsertBefore);
  Value *getExecWidthValue();
  unsigned splitDeadElements(unsigned Width, unsigned StartIdx);
  unsigned determineWidth(unsigned WholeWidth, unsigned StartIdx);
  unsigned determineNonRegionWidth(Instruction *Inst, unsigned StartIdx);
  LegalPredSize getLegalPredSize(Value *Pred, unsigned StartIdx,
                                 unsigned RemainingSize = 0);
  PredPart getPredPart(Value *V, unsigned Offset);
  Value *splitBale(Value *Last, unsigned StartIdx, unsigned Width,
                   Instruction *InsertBefore);
  Value *joinBaleInsts(Value *Last, unsigned StartIdx,
                       unsigned Width, Instruction *InsertBefore);
  Value *joinBaleResult(Value *Last, Value *LastSplitInst, unsigned StartIdx,
                         unsigned Width, Instruction *InsertBefore);
  Value *joinGStore(Value *Last, BaleInst GStore, BaleInst WrRegion,
                    unsigned StartIdx, unsigned Width,
                    Instruction *InserBefore);
  Value *joinWrRegion(Value *Last, BaleInst BInst, unsigned StartIdx,
                      unsigned Width, Instruction *InserBefore);
  Value *joinPredPredWrRegion(Value *Last, BaleInst BInst, unsigned StartIdx,
                              unsigned Width, Instruction *InserBefore);
  Value *joinAnyWrRegion(Value *Last, BaleInst BInst, unsigned StartIdx,
                         unsigned Width, Instruction *InserBefore);
  Value *splitInst(Value *Last, BaleInst BInst, unsigned StartIdx,
                   unsigned Width, Instruction *InsertBefore,
                   const DebugLoc &DL);
  Value *getSplitOperand(Instruction *Inst, unsigned OperandNum,
                         unsigned StartIdx, unsigned Size,
                         Instruction *InsertBefore, const DebugLoc &DL);
  Instruction *convertToMultiIndirect(Instruction *Inst, Value *LastJoinVal,
                                      Region *R, Instruction *InsertBefore);
  Instruction *transformMoveType(Bale *B, IntegerType *FromTy,
                                 IntegerType *ToTy);
  Value *splatPredicateIfNecessary(Value *V, Type *ValueToWriteTy,
                                   Instruction *InsertBefore,
                                   const DebugLoc &DL);
  Value *splatPredicateIfNecessary(Value *V, unsigned Width,
                                   Instruction *InsertBefore,
                                   const DebugLoc &DL);
  void eraseInst(Instruction *Inst);
  void removingInst(Instruction *Inst);
  void fixIllegalPredicates(Function *F);
  void fixIntrinsicCalls(Function *F);
  SplitKind checkBaleSplittingKind();
};

static const unsigned MaxPredSize = 32;

} // end anonymous namespace

char GenXLegalization::ID = 0;
namespace llvm {
void initializeGenXLegalizationPass(PassRegistry &);
}
INITIALIZE_PASS_BEGIN(GenXLegalization, "GenXLegalization", "GenXLegalization",
                      false, false)
INITIALIZE_PASS_DEPENDENCY(GenXFuncBaling)
INITIALIZE_PASS_DEPENDENCY(GenXFuncLiveElements)
INITIALIZE_PASS_DEPENDENCY(ScalarEvolutionWrapperPass)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_END(GenXLegalization, "GenXLegalization", "GenXLegalization",
                    false, false)

FunctionPass *llvm::createGenXLegalizationPass() {
  initializeGenXLegalizationPass(*PassRegistry::getPassRegistry());
  return new GenXLegalization;
}

void GenXLegalization::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<GenXFuncBaling>();
  AU.addRequired<GenXFuncLiveElements>();
  AU.addRequired<ScalarEvolutionWrapperPass>();
  AU.addRequired<TargetPassConfig>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addPreserved<GenXModule>();
}

/***********************************************************************
 * GenXLegalization::runOnFunction : process one function to
 *    legalize execution width and GRF crossing
 */
bool GenXLegalization::runOnFunction(Function &F) {
  Baling = &getAnalysis<GenXFuncBaling>();
  LE = &getAnalysis<GenXFuncLiveElements>();
  SE = &getAnalysis<ScalarEvolutionWrapperPass>().getSE();
  ST = &getAnalysis<TargetPassConfig>()
            .getTM<GenXTargetMachine>()
            .getGenXSubtarget();
  DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  // Check args for illegal predicates.
  for (Function::arg_iterator fi = F.arg_begin(), fe = F.arg_end(); fi != fe;
       ++fi) {
    Argument *Arg = &*fi;
    if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(Arg->getType()))
      if (VT->getElementType()->isIntegerTy(1))
        IGC_ASSERT_MESSAGE(getPredPart(Arg, 0).Size == VT->getNumElements(),
          "function arg not allowed to be illegally sized predicate");
  }

  // Legalize instructions. This does a postordered depth first traversal of the
  // CFG, and scans backwards in each basic block, to ensure that, if we unbale
  // anything, it then gets processed subsequently.
  for (po_iterator<BasicBlock *> i = po_begin(&F.getEntryBlock()),
                                 e = po_end(&F.getEntryBlock());
       i != e; ++i) {
    BasicBlock *BB = *i;
    // The effect of this loop is that we process the instructions in reverse
    // order, and we re-process anything inserted before the instruction
    // being processed. CurrentInst is a field in the GenXLegalization object,
    // which gets updated if a
    for (CurrentInst = BB->getTerminator(); CurrentInst;) {
      // If processInst returns true, re-process the same instruction. This is
      // used when unbaling.
      while (processInst(CurrentInst))
        LLVM_DEBUG(dbgs() << "reprocessing\n");
      CurrentInst =
          CurrentInst == &BB->front() ? nullptr : CurrentInst->getPrevNode();
    }
  }
  fixIntrinsicCalls(&F);
  fixIllegalPredicates(&F);
  IllegalPredicates.clear();

  return true;
}

unsigned GenXLegalization::adjustTwiceWidthOrFixed4(const Bale &B) {
  auto Main = B.getMainInst();
  if (!Main)
    return 0x3f;
  // Spot whether we have a FIXED operand and/or a TWICEWIDTH operand.
  if (GenXIntrinsic::isGenXIntrinsic(Main->Inst)) {
    GenXIntrinsicInfo II(vc::getAnyIntrinsicID(Main->Inst));
    for (auto &ArgInfo : II.getInstDesc()) {
      if (!ArgInfo.isArgOrRet())
       continue;
      switch (ArgInfo.getRestriction()) {
      case GenXIntrinsicInfo::FIXED4:
        Fixed4 = &Main->Inst->getOperandUse(ArgInfo.getArgIdx());
        break;
      case GenXIntrinsicInfo::TWICEWIDTH:
        TwiceWidth = &Main->Inst->getOperandUse(ArgInfo.getArgIdx());
        break;
      }
    }
  }
  return genx::getExecSizeAllowedBits(Main->Inst, ST);
}

/***********************************************************************
 * checkIfLongLongSupportNeeded: checks if an instruction requires
 *  target to support 64-bit integer operations
 *
 * Some operations like bitcasts or ptrtoint do not really need any HW support
 * to generate a compliant VISA
 */
bool GenXLegalization::checkIfLongLongSupportNeeded(
    const Instruction *Inst) const {
  // for now, we expect that the inspected instruction results in a value
  // 64-bit type (scalar or vector)
  IGC_ASSERT(Inst);
  IGC_ASSERT(Inst->getType()->getScalarType()->isIntegerTy(64));
  auto CheckGenXIntrinsic = [](const Instruction *Inst) {
    // wrregion/rdregion by themselves should not require any HW support
    // since finalizer should handle the respected VISA mov instructions.
    // On the other hand, if the respected intrinsic is baled into something
    // intresting - such situations should be filtered-out by scanning
    // other instructions
    if (GenXIntrinsic::isWrRegion(Inst) || GenXIntrinsic::isRdRegion(Inst)) {
      LLVM_DEBUG(dbgs() << "i64_support - RELAXED, GenX: " << *Inst << "\n");
      return false;
    }
    LLVM_DEBUG(dbgs() << "i64_support - REQUIRED, GenX: " << *Inst << "\n");
    return true;
  };
  if (GenXIntrinsic::isGenXNonTrivialIntrinsic(Inst)) {
    return CheckGenXIntrinsic(Inst);
  }
  // TODO: if number of such llvm instructions increases, consider implementing
  // instruction visitor
  switch (Inst->getOpcode()) {
  case Instruction::PtrToInt:
  case Instruction::IntToPtr:
    LLVM_DEBUG(dbgs() << "i64_support - RELAXED, instr: " << *Inst << "\n");
    return false;
  }
  LLVM_DEBUG(dbgs() << "i64_support - REQUIRED, instr: " << *Inst << "\n");
  return true;
}

/***********************************************************************
 * verifyLSCFence: verify whether an LSC fence instruction is legal and
 *  issue a diagnostic if it is not.
 */
void GenXLegalization::verifyLSCFence(const Instruction *Inst) {}

/***********************************************************************
 * verifyLSC2D: verify whether a 2D LSC load/store instruction is legal
 *  and issue a diagnostic if it is not.
 */
void GenXLegalization::verifyLSC2D(const Instruction *Inst) {}

/***********************************************************************
 * verifyType : check if type is ok according to subtarget
 *
 * Fails with fatal error if we have non-supported type
 */
void GenXLegalization::verifyType(const Type *Ty,
                                  const Instruction *Inst) const {
  if (Ty->isIntegerTy(64) && !ST->hasLongLong() && !ST->emulateLongLong() &&
      checkIfLongLongSupportNeeded(Inst)) {
    auto Target = getAnalysis<TargetPassConfig>()
                      .getTM<GenXTargetMachine>()
                      .getTargetCPU();
    vc::fatal(Ty->getContext(), *this,
              "'i64' data type is not supported by this target <" + Target +
                  ">",
              Inst);
  }

  if (Ty->isDoubleTy() && !ST->hasFP64()) {
    auto Target = getAnalysis<TargetPassConfig>()
                      .getTM<GenXTargetMachine>()
                      .getTargetCPU();
    vc::fatal(Ty->getContext(), *this,
              "'double' data type is not supported by this target <" + Target +
                  ">",
              Inst);
  }
}

/***********************************************************************
 * checkInst : check instruction before GRF width legalization
 *
 * Return: true if everything is ok, false if no legalization required
 */
bool GenXLegalization::checkInst(const Instruction *Inst) const {
  LLVM_DEBUG(dbgs() << "checkInst: " << *Inst << "\n");
  if (Inst->isTerminator())
    return false; // ignore terminator
  if (isa<PHINode>(Inst))
    return false; // ignore phi node

  // ignore predef regs
  switch (auto IID = vc::getAnyIntrinsicID(Inst)) {
  default:
    if (GenXIntrinsic::isReadWritePredefReg(IID))
      return false;
    break;
  case GenXIntrinsic::genx_r0:
  case GenXIntrinsic::genx_sr0:
    return false;
  }

  // Sanity check for illegal operand type
  const auto *ScalarType = Inst->getType()->getScalarType();
  verifyType(ScalarType, Inst);

  // Sanity check for illegal operands
  for (auto &&Op : Inst->operands()) {
    const auto *ScalarOpType = Op->getType()->getScalarType();
    verifyType(ScalarOpType, Inst);
  }

  return true;
}

/***********************************************************************
 * processInst : process one instruction to legalize execution width and GRF
 *    crossing
 *
 * Return:  true to re-process same instruction (typically after unbaling
 *          something from it)
 */
bool GenXLegalization::processInst(Instruction *Inst) {
  if (!checkInst(Inst))
    return false;
  LLVM_DEBUG(dbgs() << "processInst: " << *Inst << "\n");
  if (!ST->hasSad2Support()) {
    switch (GenXIntrinsic::getGenXIntrinsicID(Inst)) {
    case GenXIntrinsic::genx_ssad2:
    case GenXIntrinsic::genx_sssad2add:
    case GenXIntrinsic::genx_sssad2add_sat:
    case GenXIntrinsic::genx_susad2add:
    case GenXIntrinsic::genx_susad2add_sat:
    case GenXIntrinsic::genx_usad2:
    case GenXIntrinsic::genx_ussad2add:
    case GenXIntrinsic::genx_ussad2add_sat:
    case GenXIntrinsic::genx_uusad2add:
    case GenXIntrinsic::genx_uusad2add_sat:
      vc::diagnose(Inst->getContext(), "GenXLegalization",
                   "sad2 and sada2 are not supported by this target", Inst);
    default:
      break;
    }
  }

  if (GenXIntrinsic::isLSC(Inst)) {
    if (GenXIntrinsic::isLSCFence(Inst)) {
      verifyLSCFence(Inst);
      return false;
    }
    if (GenXIntrinsic::isLSC2D(Inst)) {
      verifyLSC2D(Inst);
      return false;
    }
  }

  // Prepare to insert split code after current instruction.
  auto InsertBefore = Inst->getNextNode();

  if (!isa<VectorType>(Inst->getType()) &&
      !vc::InternalIntrinsic::isInternalMemoryIntrinsic(Inst)) {
    if (Inst->getOpcode() == Instruction::BitCast &&
        Inst->getOperand(0)->getType()->getScalarType()->isIntegerTy(1)) {
      // Special processing for bitcast from predicate to scalar int.
      return processBitCastFromPredicate(Inst, InsertBefore);
    }
    switch (GenXIntrinsic::getGenXIntrinsicID(Inst)) {
    case GenXIntrinsic::genx_all:
    case GenXIntrinsic::genx_any:
      // Special processing for all/any
      return processAllAny(Inst, InsertBefore);
    default:
      break;
    }
    if (!isa<StoreInst>(Inst))
      return false; // no splitting needed for other scalar op.
  }
  if (!Baling->canSplitBale(Inst))
    return false;
  if (isa<ExtractValueInst>(Inst))
    return false;
  if (isa<BitCastInst>(Inst)) {
    if (Inst->getType()->getScalarType()->isIntegerTy(1)) {
      // Special processing for bitcast from scalar int to predicate.
      return processBitCastToPredicate(Inst, InsertBefore);
    }
    // Ignore any other bitcast.
    return false;
  }

  if (Baling->isBaled(Inst)) {
    LLVM_DEBUG(dbgs() << "is baled\n");
    return false; // not head of bale, ignore
  }
  // No need to split an llvm.genx.constant with an undef value.
  switch (GenXIntrinsic::getGenXIntrinsicID(Inst)) {
  case GenXIntrinsic::genx_constanti:
  case GenXIntrinsic::genx_constantf:
    if (isa<UndefValue>(Inst->getOperand(0)))
      return false;
    break;
  default:
    break;
  }
  clearBale();
  Baling->buildBale(Inst, &B);
  // Get the main inst from the bale and decide whether it is something we do
  // not split. If there is no main inst, the bale is splittable.
  if (auto MainInst = B.getMainInst()) {
    if (isa<CallInst>(MainInst->Inst)) {
      // No legalization for inline asm
      if (cast<CallInst>(MainInst->Inst)->isInlineAsm())
        return false;
      unsigned IntrinID = vc::getAnyIntrinsicID(MainInst->Inst);
      switch (IntrinID) {
      case GenXIntrinsic::not_any_intrinsic:
        return false; // non-intrinsic call, ignore
      case GenXIntrinsic::genx_constantpred:
        break; // these intrinsics can be split
      case GenXIntrinsic::genx_dpas:
      case GenXIntrinsic::genx_dpas2:
      case GenXIntrinsic::genx_dpasw:
      case GenXIntrinsic::genx_dpas_nosrc0:
      case GenXIntrinsic::genx_dpasw_nosrc0:
        return false;
      default:
        if (GenXIntrinsicInfo(IntrinID).getRetInfo().getCategory() !=
            GenXIntrinsicInfo::GENERAL) {
          // This is not an ALU intrinsic (e.g. cm_add).
          // We have a non-splittable intrinsic. Such an intrinsic can
          // have a scalar arg with a baled in rdregion, which does not
          // need legalizing. It never has a vector arg with a baled in
          // rdregion. So no legalization needed.
          return false;
        }
        break;
      }
    } else if (isa<BitCastInst>(MainInst->Inst)) {
      // BitCast is not splittable in here. A non-category-converting BitCast
      // is always coalesced in GenXCoalescing, so never generates actual
      // code. Thus it does not matter if it has an illegal size.
      return false;
    } else if (auto LI = dyn_cast<LoadInst>(MainInst->Inst)) {
      (void)LI;
      // Do not split a (global) load as it does not produce code.
      return false;
    } else if (isa<ExtractValueInst>(MainInst->Inst)) {
      // If EV is main than it's related to inline assembly with
      // multiple outputs, no legalization
      return false;
    }
    // Any other instruction: split.
  }

  // Check if it is a byte move that we want to transform into a short/int move.
  auto *I8Ty = Type::getInt8Ty(Inst->getContext());
  auto *I16Ty = Type::getInt16Ty(Inst->getContext());
  auto *I32Ty = Type::getInt32Ty(Inst->getContext());
  if (Instruction *MoveHead = nullptr;
      (MoveHead = transformMoveType(&B, I8Ty, I32Ty)) ||
      (MoveHead = transformMoveType(&B, I8Ty, I16Ty))) {
    // Successfully transformed. Run legalization on the new instruction
    // (which got inserted before the existing one, so will be processed
    // next).
    LLVM_DEBUG(dbgs() << "done transform of byte move\n");
    clearBale();
    Baling->buildBale(MoveHead, &B);
    return processBale(MoveHead->getNextNode());
  }

  // Check if it is a 64-bit move that we want to transform into 32-bit move.
  auto *I64Ty = Type::getInt64Ty(Inst->getContext());
  if (ST->emulateLongLong()) {
    if (Instruction *MoveHead = transformMoveType(&B, I64Ty, I32Ty)) {
      // Successfully transformed. Run legalization on the new instruction
      // (which got inserted before the existing one, so will be processed
      // next).
      LLVM_DEBUG(dbgs() << "done transform of long long move\n");
      clearBale();
      Baling->buildBale(MoveHead, &B);
      return processBale(MoveHead->getNextNode());
    }
  }

  // Normal instruction splitting.
  LLVM_DEBUG(dbgs() << "processBale: "; B.print(dbgs()));

  if (B.isGStoreBale() && !B.isGStoreBaleLegal()) {
#ifdef _DEBUG
    dbgs() << "processBale: ";
    B.print(dbgs());
#endif
    vc::diagnose(Inst->getContext(), "GenXLegalization",
                 "g_store bale is not supported yet", Inst);
    return true;
  }

  return processBale(InsertBefore);
}

/***********************************************************************
 * processBale : process one bale to legalize execution width and GRF crossing
 *
 * Return:  true to re-process same head of bale
 */
bool GenXLegalization::processBale(Instruction *InsertBefore) {
  // Get the current execution width.
  auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(getExecWidthValue()->getType());
  unsigned WholeWidth = VT ? VT->getNumElements() : 1;
  // No splitting of scalar or 1-vector
  if (WholeWidth == 1)
    return false;
  // Check the bale split kind if do splitting.
  CurSplitKind = checkBaleSplittingKind();

  // We will be generating a chain of joining wrregions. The initial "old
  // value" input is undef. If the bale is headed by a wrregion or
  // wrpredpredregion that is being split, code inside splitInst uses the
  // original operand 0 for split 0 instead.
  Value *Joined = nullptr;
  // For bales ending with g_store, joining is not through wrr, but through
  // g_load and g_store.
  if (CurSplitKind != SplitKind::SplitKind_GStore)
    Joined = UndefValue::get(B.getHeadIgnoreGStore()->Inst->getType());

  // Do the splits.
  for (unsigned StartIdx = 0; StartIdx != WholeWidth;) {
    // Determine the width of the next split.
    unsigned Width = determineWidth(WholeWidth, StartIdx);
    if (Width == DETERMINEWIDTH_UNBALE) {
      // determineWidth wants us to re-start processing from the head of the
      // bale, because it did some unbaling. First erase any newly added
      // instructions.
      for (;;) {
        Instruction *Erase = InsertBefore->getPrevNode();
        if (Erase == B.getHead()->Inst)
          break;
        eraseInst(Erase);
      }
      return true; // ask to re-start processing
    }
    if (Width == DETERMINEWIDTH_NO_SPLIT)
      return noSplitProcessing(); // no splitting required
    // Some splitting is required. This includes the case that there will be
    // only one split (i.e. no splitting really required), but:
    //  * it includes an indirect rdregion that is converted to multi indirect;
    // Create the next split.
    Joined = splitBale(Joined, StartIdx, Width, InsertBefore);
    StartIdx += Width;
  }
  if (!B.endsWithGStore())
    B.getHead()->Inst->replaceAllUsesWith(Joined);
  // Erase the original bale.  We erase in reverse order so erasing each one
  // removes the uses of earlier ones. However we do not erase an instruction
  // that still has uses; that happens for a FIXED4 operand.
  InsertBefore = B.getHead()->Inst->getNextNode();
  for (auto bi = B.rbegin(), be = B.rend(); bi != be; ++bi) {
    if (bi->Inst->use_empty())
      eraseInst(bi->Inst);
    else {
      // Do not erase this one as it still has a use; it must be a FIXED4
      // operand so it is used by the new split bales. Instead move it so it
      // does not get re-processed by the main loop of this pass.
      removingInst(bi->Inst);
      bi->Inst->removeFromParent();
      bi->Inst->insertBefore(InsertBefore);
      InsertBefore = bi->Inst;
    }
  }
  return false;
}

/***********************************************************************
 * noSplitProcessing : processing of a splttable bale in the case
 *    that it is not split
 *
 * Return:  true to re-process same head of bale
 */
bool GenXLegalization::noSplitProcessing() {
  if (auto SI = dyn_cast<SelectInst>(B.getHeadIgnoreGStore()->Inst)) {
    // Handle the case that a vector select has a scalar condition.
    SI->setOperand(0,
                   splatPredicateIfNecessary(SI->getCondition(), SI->getType(),
                                             SI, SI->getDebugLoc()));
  }
  return false;
}

/***********************************************************************
 * processAllAny : legalize all/any
 *
 * Return:  true to re-process same head of bale
 */
bool GenXLegalization::processAllAny(Instruction *Inst,
                                     Instruction *InsertBefore) {
  // See if the all/any is already legally sized.
  Value *Pred = Inst->getOperand(0);
  unsigned WholeSize =
      cast<IGCLLVM::FixedVectorType>(Pred->getType())->getNumElements();
  if (getPredPart(Pred, 0).Size == WholeSize) {
    // Already legally sized. We need to check whether it is used just in a
    // branch or select, possibly via a not; if not we need to convert the
    // result to a non-predicate then back to a predicate with a cmp, as there
    // is no way of expressing a non-baled-in all/any in the generated code.
    if (Inst->hasOneUse()) {
      auto User = cast<Instruction>(Inst->use_begin()->getUser());
      if (isNot(User)) {
        if (!User->hasOneUse())
          User = nullptr;
        else
          User = cast<Instruction>(User->use_begin()->getUser());
      }
      if (User && (isa<SelectInst>(User) || isa<BranchInst>(User)))
        return false;
    }
    // Do that conversion.
    const DebugLoc &DL = Inst->getDebugLoc();
    auto I16Ty = Type::getInt16Ty(Inst->getContext());
    auto V1I16Ty = IGCLLVM::FixedVectorType::get(I16Ty, 1);
    Region R(V1I16Ty);
    R.Mask = Inst;
    auto NewWr = R.createWrRegion(
        Constant::getNullValue(V1I16Ty), ConstantInt::get(I16Ty, 1),
        Inst->getName() + ".allany_lowered", InsertBefore, DL);
    auto NewBC = CastInst::Create(Instruction::BitCast, NewWr, I16Ty,
                                  NewWr->getName(), InsertBefore);
    auto NewPred = CmpInst::Create(Instruction::ICmp, CmpInst::ICMP_NE, NewBC,
                                   Constant::getNullValue(I16Ty),
                                   NewBC->getName(), InsertBefore);
    NewPred->setDebugLoc(DL);
    NewBC->setDebugLoc(DL);
    NewWr->setOperand(GenXIntrinsic::GenXRegion::PredicateOperandNum,
                      UndefValue::get(Inst->getType()));
    Inst->replaceAllUsesWith(NewPred);
    NewWr->setOperand(GenXIntrinsic::GenXRegion::PredicateOperandNum, Inst);
    return false;
  }
  // It needs to be split. For each part, we have an all/any on that part, and
  // use it to do a select on a scalar that keeps track of whether all/any set
  // bits have been found.
  unsigned IID = vc::getAnyIntrinsicID(Inst);
  Type *I16Ty = Type::getInt16Ty(Inst->getContext());
  Value *Zero = Constant::getNullValue(I16Ty);
  Value *One = ConstantInt::get(I16Ty, 1);
  Value *Result = IID == GenXIntrinsic::genx_all ? One : Zero;
  const DebugLoc &DL = Inst->getDebugLoc();
  for (unsigned StartIdx = 0; StartIdx != WholeSize;) {
    auto PP = getPredPart(Pred, StartIdx);
    auto Part = Region::createRdPredRegionOrConst(
        Pred, StartIdx, PP.Size, Pred->getName() + ".split" + Twine(StartIdx),
        InsertBefore, DL);
    Module *M = InsertBefore->getModule();
    Function *Decl =
        GenXIntrinsic::getAnyDeclaration(M, IID, Part->getType());
    Instruction *NewAllAny = nullptr;
    if (PP.Size != 1)
      NewAllAny = CallInst::Create(Decl, Part,
                                   Inst->getName() + ".split" + Twine(StartIdx),
                                   InsertBefore);
    else {
      // Part is v1i1. All we need to do is bitcast it to i1, which does not
      // generate any code.
      NewAllAny = CastInst::Create(
          Instruction::BitCast, Part, Part->getType()->getScalarType(),
          Inst->getName() + ".split" + Twine(StartIdx), InsertBefore);
    }
    NewAllAny->setDebugLoc(DL);
    SelectInst *Sel = nullptr;
    if (IID == GenXIntrinsic::genx_all)
      Sel = SelectInst::Create(NewAllAny, Result, Zero,
                               Inst->getName() + ".join" + Twine(StartIdx),
                               InsertBefore);
    else
      Sel = SelectInst::Create(NewAllAny, One, Result,
                               Inst->getName() + ".join" + Twine(StartIdx),
                               InsertBefore);
    Sel->setDebugLoc(DL);
    Result = Sel;
    StartIdx += PP.Size;
  }
  // Add a scalar comparison to get the final scalar bool result.
  auto Cmp = CmpInst::Create(Instruction::ICmp, CmpInst::ICMP_NE, Result, Zero,
                             Inst->getName() + ".joincmp", InsertBefore);
  // Replace and erase the old all/any.
  Cmp->setDebugLoc(DL);
  Inst->replaceAllUsesWith(Cmp);
  eraseInst(Inst);
  return false;
}

/***********************************************************************
 * processBitCastFromPredicate : legalize bitcast from predicate (vector of
 *    i1) to scalar int
 */
bool GenXLegalization::processBitCastFromPredicate(Instruction *Inst,
                                                   Instruction *InsertBefore) {
  Value *Pred = Inst->getOperand(0);
  unsigned SplitWidth = getPredPart(Pred, 0).Size;
  if (SplitWidth == 0)
    return false;

  {
    unsigned WholeWidth = 0; // it will be assigned inside assertion statament
    IGC_ASSERT(
        (WholeWidth =
             cast<IGCLLVM::FixedVectorType>(Pred->getType())->getNumElements(),
         1));
    IGC_ASSERT(SplitWidth);
    IGC_ASSERT_MESSAGE(!(WholeWidth % SplitWidth), "does not handle odd predicate sizes");
    (void) WholeWidth;
  }

  // Bitcast each split predicate into an element of an int vector.
  // For example, if the split size is 16, then the result is a vector
  // of i16. Then bitcast that to the original result type.
  Type *IntTy = Type::getIntNTy(Inst->getContext(), SplitWidth);
  unsigned NumSplits = Inst->getType()->getPrimitiveSizeInBits() / SplitWidth;
  if (NumSplits == 1)
    return false;
  const DebugLoc &DL = Inst->getDebugLoc();
  Type *IntVecTy = IGCLLVM::FixedVectorType::get(IntTy, NumSplits);
  Value *Result = UndefValue::get(IntVecTy);
  // For each split...
  for (unsigned i = 0; i != NumSplits; ++i) {
    // Bitcast that split of the predicate.
    auto *NewBitCast =
        CastInst::Create(Instruction::BitCast,
                         getSplitOperand(Inst, /*OperandNum=*/0, i * SplitWidth,
                                         SplitWidth, InsertBefore, DL),
                         IntTy, Inst->getName() + ".split", InsertBefore);
    NewBitCast->setDebugLoc(DL);
    // Write it into the element of the vector.
    Region R(Result);
    R.getSubregion(i, 1);
    Result = R.createWrRegion(Result, NewBitCast,
                              Inst->getName() + ".join" + Twine(i * SplitWidth),
                              InsertBefore, DL);
  }
  // Bitcast the vector to the original type.
  auto *NewBitCast =
      CastInst::Create(Instruction::BitCast, Result, Inst->getType(),
                       Inst->getName() + ".cast", InsertBefore);
  NewBitCast->setDebugLoc(DL);
  // Change uses and erase original.
  Inst->replaceAllUsesWith(NewBitCast);
  eraseInst(Inst);
  return false;
}

/***********************************************************************
 * processBitCastToPredicate : legalize bitcast to predicate (vector of
 *    i1) from scalar int
 */
bool GenXLegalization::processBitCastToPredicate(Instruction *Inst,
                                                 Instruction *InsertBefore) {
  unsigned WholeWidth =
      cast<IGCLLVM::FixedVectorType>(Inst->getType())->getNumElements();
  unsigned SplitWidth = getPredPart(Inst, 0).Size;
  IGC_ASSERT(SplitWidth);
  IGC_ASSERT_MESSAGE(!(WholeWidth % SplitWidth), "does not handle odd predicate sizes");
  unsigned NumSplits = WholeWidth / SplitWidth;
  if (NumSplits == 1)
    return false;
  // Bitcast the scalar int input to a vector of ints each with a number of
  // bits matching the predicate split size.
  const DebugLoc &DL = Inst->getDebugLoc();
  auto IVTy = IGCLLVM::FixedVectorType::get(
      Type::getIntNTy(Inst->getContext(), SplitWidth), WholeWidth / SplitWidth);
  auto IntVec = CastInst::Create(Instruction::BitCast, Inst->getOperand(0),
                                 IVTy, Inst->getName() + ".cast", InsertBefore);
  IntVec->setDebugLoc(DL);
  Value *Result = UndefValue::get(Inst->getType());
  Type *SplitPredTy = IGCLLVM::FixedVectorType::get(
      Inst->getType()->getScalarType(), SplitWidth);
  // For each predicate split...
  for (unsigned i = 0; i != NumSplits; ++i) {
    // Get the element of the vector using rdregion.
    Region R(IntVec);
    R.getSubregion(i, 1);
    auto NewRd = R.createRdRegion(
        IntVec, Inst->getName() + ".rdsplit" + Twine(i), InsertBefore, DL);
    // Bitcast that element of the int vector to a predicate.
    auto NewPred =
        CastInst::Create(Instruction::BitCast, NewRd, SplitPredTy,
                         Inst->getName() + ".split" + Twine(i), InsertBefore);
    NewPred->setDebugLoc(DL);
    // Join into the overall result using wrpredregion.
    auto NewWr = Region::createWrPredRegion(
        Result, NewPred, i * SplitWidth, Inst->getName() + ".join" + Twine(i),
        InsertBefore, DL);
    // If this is the first wrpredregion, add it to IllegalPredicates so it gets
    // processed later in fixIllegalPredicates.
    if (!i)
      IllegalPredicates.insert(NewWr);
    Result = NewWr;
  }
  // Change uses and erase original.
  Inst->replaceAllUsesWith(Result);
  eraseInst(Inst);
  return false;
}

/***********************************************************************
 * getExecWidthValue : get the value, which type represents
 *                     the actual  execution width of the bale
 *
 * Return: If there is no wrregion at the head of the bale, then such
 *         value is the head itself (ignoring gstore). If there is a
 *         wrregion or wrpredpredregion, then this value is the input
 *         to it
 */
Value *GenXLegalization::getExecWidthValue() {
  BaleInst *Head = B.getHeadIgnoreGStore();
  if (Head->Info.Type == BaleInfo::WRREGION ||
      Head->Info.Type == BaleInfo::WRPREDREGION ||
      Head->Info.Type == BaleInfo::WRPREDPREDREGION)
    return Head->Inst->getOperand(1);
  else
    return Head->Inst;
}

/***********************************************************************
 * splitDeadElements : try to reduce the width to isolate unused elements
 *
 * Enter:   Width = current calculated split width
 *          StartIdx = start index of this split
 *
 * Return:  New width when the split is possible or the old one otherwise
 *
 * Live element analysis can provide info about the actual usage of every
 * element of a vector. If some elements are unused we can do the split
 * so that dead elements in the begining and end of a vector are isolated
 * from a single live part in the middle.
 */
unsigned GenXLegalization::splitDeadElements(unsigned Width,
                                             unsigned StartIdx) {
  auto *V = getExecWidthValue();
  auto *ElemTy = V->getType()->getScalarType();
  // The most math instructions require exec size to be 8 or 16 in case of half
  // float. And some of them require it even for other float types. Even if we
  // reduce the width here, it will be very likely set back to the 'native'
  // width by finalizer
  if (ElemTy->isHalfTy())
    return Width;
  if (ElemTy->isFloatTy()) {
    auto Main = B.getMainInst();
    if (Main) {
      unsigned IID = vc::getAnyIntrinsicID(Main->Inst);
      if (IID == GenXIntrinsic::genx_ieee_div ||
          IID == GenXIntrinsic::genx_ieee_sqrt)
        return Width;
    }
  }
  const auto &LiveElems = LE->getLiveElements(V);
  if (LiveElems.size() > 1 || LiveElems.isAllDead() || !LiveElems.isAnyDead())
    return Width;
  // Execution mask must be 4 aligned, which is important for predicates
  unsigned FirstLive = LiveElems[0].find_first() & ~3U;
  // Dead parts don't need to be aligned, because the will be removed anyway
  unsigned LastLive = LiveElems[0].find_last();
  if ((StartIdx > FirstLive || StartIdx + Width < FirstLive) &&
      (StartIdx > LastLive || StartIdx + Width < LastLive))
    return Width;
  // Instruction with current width crosses the boundary between live
  // and dead parts
  FirstLive = std::max(FirstLive, StartIdx);
  LastLive = std::min(LastLive, StartIdx + Width);
  unsigned LiveSize = LastLive - FirstLive + 1;
  unsigned LiveWidth = 1;
  while (LiveWidth < LiveSize)
    LiveWidth <<= 1;
  if (LiveWidth >= Width)
    return Width;
  // Live part can fit in a single instruction with smaller width
  unsigned NewWidth = LiveWidth;
  if (StartIdx < FirstLive) {
    // In this iteration we have to split preceding zeroes
    // Determine the largest width not crossing the live part's boundary
    unsigned DeadSize = FirstLive - StartIdx;
    unsigned DeadWidth = Width;
    while (DeadWidth > DeadSize)
      DeadWidth >>= 1;
    NewWidth = DeadWidth;
  }
  return NewWidth;
}

/***********************************************************************
 * determineWidth : determine width of the next split
 *
 * Enter:   WholeWidth = whole execution width of the bale before splitting
 *          StartIdx = start index of this split
 *
 * Return:  width of next split, DETERMINEWIDTH_UNBALE if unbaling occurred,
 *          DETERMINEWIDTH_NO_SPLIT if no split required
 *
 * If this function returns WholeWidth rather than DETERMINEWIDTH_NO_SPLIT, it
 * means that there is an indirect rdregion that needs to be converted to multi
 * indirect. This is different to the condition of not needing a split at all,
 * which causes this function to return DETERMINEWIDTH_NO_SPLIT.
 */
unsigned GenXLegalization::determineWidth(unsigned WholeWidth,
                                          unsigned StartIdx) {
  auto Head = B.getHeadIgnoreGStore();
  // Prepare to keep track of whether an instruction with a minimum width
  // (e.g. dp4) would be split too small, and whether we need to unbale.
  unsigned ExecSizeAllowedBits = adjustTwiceWidthOrFixed4(B);
  if (!vc::canUseSIMD32(*(Head->Inst->getModule()), ST->hasFusedEU()))
    // Actually, we should legalize with these more strict requirements only FGs
    // of indirectly called functions. But there are two design issues that make
    // us legalize everything if the module has a stack call:
    //   * jmpi to goto transformation is appied in VISA and it transforms more
    //   than necessary
    //   * this legalization pass does not have access to FGs
    ExecSizeAllowedBits &= 0x1f;

  unsigned MainInstMinWidth =
      1 << llvm::countTrailingZeros(ExecSizeAllowedBits);
  // Determine the vector width that we need to split into.
  bool IsReadSameVector = false;
  unsigned Width = WholeWidth - StartIdx;
  unsigned PredMinWidth = 1;
  Value *WrRegionInput = nullptr;
  if (Head->Info.Type == BaleInfo::WRREGION)
    WrRegionInput =
        Head->Inst->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum);
  bool MustSplit = false;
  for (Bale::iterator i = B.begin(), InstWithMinWidth = i, e = B.end(); i != e;
       ++i) {
    unsigned ThisWidth = Width;
    // Determine the width we need for this instruction.
    switch (i->Info.Type) {
    case BaleInfo::WRREGION: {
      bool Unbale = false;
      Region R = makeRegionFromBaleInfo(i->Inst, i->Info);
      if (R.Mask &&
          !i->Info.isOperandBaled(GenXIntrinsic::GenXRegion::PredicateOperandNum)) {
        // We have a predicate, and it is not a baled in rdpredregion. (A
        // baled in rdpredregion is handled when this loop reaches that
        // instruction.) Get the min and max legal predicate size.
        auto PredWidths = getLegalPredSize(R.Mask, StartIdx);
        ThisWidth = std::min(ThisWidth, PredWidths.Max);
        PredMinWidth = PredWidths.Min;
      }
      if (PredMinWidth > Width) {
        // The min predicate size is bigger than the legal size for the rest
        // of the bale other than the wrregion. Unbale the main instruction.
        Unbale = true;
      }
      // Get the max legal size for the wrregion.
      ThisWidth = std::min(
          ThisWidth,
          getLegalRegionSizeForTarget(
              *ST, R, StartIdx, false /*Allow2D*/, true /*UseRealIdx*/,
              cast<IGCLLVM::FixedVectorType>(i->Inst->getOperand(0)->getType())
                  ->getNumElements(),
              &(Baling->AlignInfo)));
      if (B.size() == 1)
        // If wrregion is the single instruction is this bale we have to also
        // check source region
        ThisWidth = std::min(
            ThisWidth,
            getLegalRegionSizeForTarget(*ST, R, StartIdx, false /*Allow2D*/,
                                        false /*UseRealIdx*/,
                                        cast<IGCLLVM::FixedVectorType>(
                                            i->Inst->getOperand(0)->getType())
                                            ->getNumElements(),
                                        &(Baling->AlignInfo)));
      if (!Unbale && R.Mask && PredMinWidth > ThisWidth) {
        // The min predicate size (from this wrregion) is bigger than the
        // legal size for this wrregion. We have to rewrite the wrregion as:
        //    rdregion of the region out of the old value
        //    predicated wrregion, which now has a contiguous region
        //    wrregion (the original wrregion but with no predicate)
        // then set DETERMINEWIDTH_UNBALE to restart.
        auto DL = i->Inst->getDebugLoc();
        auto NewRd = R.createRdRegion(
            i->Inst->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum),
            i->Inst->getName() + ".separatepred.rd", i->Inst, DL, false);
        Baling->setBaleInfo(NewRd, BaleInfo(BaleInfo::RDREGION));
        Region R2(NewRd);
        R2.Mask = R.Mask;
        auto NewWr = R2.createWrRegion(
            NewRd,
            i->Inst->getOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum),
            i->Inst->getName() + ".separatepred.wr", i->Inst, DL);
        auto NewBI = i->Info;
        NewBI.clearOperandBaled(GenXIntrinsic::GenXRegion::WrIndexOperandNum);
        Baling->setBaleInfo(NewWr, NewBI);
        i->Inst->setOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum, NewWr);
        i->Inst->setOperand(GenXIntrinsic::GenXRegion::PredicateOperandNum,
                            Constant::getAllOnesValue(R.Mask->getType()));
        i->Info.clearOperandBaled(GenXIntrinsic::GenXRegion::PredicateOperandNum);
        i->Info.clearOperandBaled(GenXIntrinsic::GenXRegion::NewValueOperandNum);
        Baling->setBaleInfo(i->Inst, i->Info);
        ThisWidth = DETERMINEWIDTH_UNBALE;
        break;
      }
      if (PredMinWidth > ThisWidth) {
        // The min predicate size (from a select baled into this wrregion) is
        // bigger than the legal size for this wrregion. Unbale the select.
        Unbale = true;
      }
      if (ThisWidth < MainInstMinWidth) {
        // The wrregion is split too small for the main instruction. Unbale
        // the main instruction.
        Unbale = true;
      }
      if (Unbale) {
        i->Info.clearOperandBaled(GenXIntrinsic::GenXRegion::NewValueOperandNum);
        Baling->setBaleInfo(i->Inst, i->Info);
        ThisWidth = DETERMINEWIDTH_UNBALE;
      }
      break;
    }
    case BaleInfo::RDREGION: {
      if (i->Inst->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum) ==
          WrRegionInput)
        IsReadSameVector = true; // See use of this flag below.

      // Determine the max region width. If this rdregion is baled into a
      // TWICEWIDTH operand, double the start index and half the resulting
      // size.
      Region R = makeRegionFromBaleInfo(i->Inst, i->Info);
      unsigned Doubling = TwiceWidth && i->Inst == *TwiceWidth;
      unsigned ModifiedStartIdx = StartIdx << Doubling;
      if (Fixed4 && i->Inst == *Fixed4)
        ModifiedStartIdx = 0;
      ThisWidth = getLegalRegionSizeForTarget(
          *ST, R, ModifiedStartIdx, true /*Allow2D*/, true /*UseRealIdx*/,
          cast<IGCLLVM::FixedVectorType>(i->Inst->getOperand(0)->getType())
              ->getNumElements(),
          &(Baling->AlignInfo));
      if (ThisWidth == 1 &&
          (R.ElementBytes != genx::ByteBytes ||
           ST->hasMultiIndirectByteRegioning()) &&
          R.Indirect && !R.isMultiIndirect()) {
        // This is a single indirect rdregion where we failed to make the
        // valid size any more than one. If possible, increase the valid size
        // to 4 or 8 on the assumption that we are going to convert it to a
        // multi indirect.
        IGC_ASSERT_EXIT(R.Width - StartIdx % R.Width > 0);
        auto NewThisWidth = 1 << genx::log2(R.Width - StartIdx % R.Width);
        if (NewThisWidth >= 4) {
          ThisWidth = std::min(NewThisWidth, 8);
          MustSplit = true;
        }
      }
      ThisWidth >>= Doubling;
      if (ThisWidth < MainInstMinWidth) {
        // The rdregion is split too small for the main instruction.
        // Unbale the rdregion from its user (must be exactly one user as
        // it is baled). Note that the user is not necessarily the main
        // inst, it might be a modifier baled in to the main inst.
        Value::use_iterator UI = i->Inst->use_begin();
        Instruction *User = cast<Instruction>(UI->getUser());
        BaleInfo BI = Baling->getBaleInfo(User);
        BI.clearOperandBaled(UI->getOperandNo());
        Baling->setBaleInfo(User, BI);
        ThisWidth = DETERMINEWIDTH_UNBALE;
      }

      if (R.is1D())
        break;
      // The rdregion is split when R.Width crosses the register boundary.
      for (unsigned I = 0; I < ThisWidth / R.Width; I++) {
        auto ChunkOffset = R.Offset + I * R.VStride * R.ElementBytes;
        auto ChunkEndOffset =
            ChunkOffset + (R.Width - 1) * R.Stride * R.ElementBytes;
        if (ChunkOffset / ST->getGRFByteSize() !=
            ChunkEndOffset / ST->getGRFByteSize()) {
          ThisWidth = R.Width;
          MustSplit = true;
          break;
        }
      }

      break;
    }
    case BaleInfo::NOTP:
      // Only process notp
      // - if predicate is a vector and
      // - if it does not have rdpredregion baled in.
      if (!i->Info.isOperandBaled(0) && i->Inst->getType()->isVectorTy()) {
        // Get the min and max legal predicate size.
        auto PredWidths = getLegalPredSize(i->Inst->getOperand(0), StartIdx);
        // If the min legal predicate size is more than the remaining size in
        // the predicate that the rdpredregion extracts, ignore it. This results
        // in an illegal rdpredregion from splitInst, which then has to be
        // lowered to less efficient code by fixIllegalPredicates. This
        // situation arises when the original unsplit bale has an odd size
        // rdpredregion out of a v32i1, from a CM select() where the mask is an
        // i32.
        if (PredWidths.Min <= WholeWidth - StartIdx)
          PredMinWidth = PredWidths.Min;
        ThisWidth = std::min(ThisWidth, PredWidths.Max);
      }
      break;
    case BaleInfo::RDPREDREGION: {
      unsigned RdPredStart =
          cast<ConstantInt>(i->Inst->getOperand(1))->getZExtValue();
      // Get the min and max legal predicate size.
      auto PredWidths =
          getLegalPredSize(i->Inst->getOperand(0), // the input predicate
                           RdPredStart + StartIdx);
      // If the min legal predicate size is more than the remaining size in
      // the predicate that the rdpredregion extracts, ignore it. This results
      // in an illegal rdpredregion from splitInst, which then has to be
      // lowered to less efficient code by fixIllegalPredicates. This situation
      // arises when the original unsplit bale has an odd size rdpredregion
      // out of a v32i1, from a CM select() where the mask is an i32.
      if (PredWidths.Min <= WholeWidth - StartIdx)
        PredMinWidth = PredWidths.Min;
      ThisWidth = std::min(ThisWidth, PredWidths.Max);
      break;
    }
    case BaleInfo::SHUFFLEPRED: {
      // If shufflepred is baled with load with channels then it is always legal.
      if (const BaleInst *BI = B.getMainInst()) {
        unsigned IID = GenXIntrinsic::getGenXIntrinsicID(BI->Inst);
        switch (IID) {
        default:
          break;
        case GenXIntrinsic::genx_gather4_scaled2:
        case GenXIntrinsic::genx_gather4_masked_scaled2:
          continue;
        }
      }

      // In other case we need to legalize it using rdpredregion.
      // Probably later rdpredregion will be legalized further.
      auto *SI = cast<ShuffleVectorInst>(i->Inst);
      return ShuffleVectorAnalyzer::getReplicatedSliceDescriptor(SI).SliceSize;
    }
    case BaleInfo::ADDRADD:
    case BaleInfo::ADDROR:
    case BaleInfo::GSTORE:
    case BaleInfo::REGINTR:
      break;
    default: {
      ThisWidth = determineNonRegionWidth(i->Inst, StartIdx);
      Value *Pred = nullptr;
      if (auto SI = dyn_cast<SelectInst>(i->Inst)) {
        Pred = SI->getCondition();
        if (!isa<VectorType>(Pred->getType())) {
          // For a select with a scalar predicate, the predicate will be
          // splatted by splatPredicateIfNecessary. We need to limit the legal
          // width to the max predicate width.
          ThisWidth = std::min(ThisWidth, MaxPredSize);
          Pred = nullptr;
        }
      } else if (isa<CmpInst>(i->Inst))
        Pred = i->Inst;
      if (Pred && isa<VectorType>(Pred->getType())) {
        // For a select (with a vector predicate) or cmp, we need to take the
        // predicate into account. Get the min and max legal predicate size.
        auto PredWidths = getLegalPredSize(Pred, StartIdx);
        // If the min legal predicate size is more than the remaining size in
        // the predicate that the rdpredregion extracts, ignore it. This results
        // in an illegal rdpredregion from splitInst, which then has to be
        // lowered to less efficient code by fixIllegalPredicates. This
        // situation arises when the original unsplit bale has an odd size
        // rdpredregion out of a v32i1, from a CM select() where the mask is an
        // i32.
        if (PredWidths.Min <= WholeWidth - StartIdx)
          PredMinWidth = PredWidths.Min;
        if (PredMinWidth > Width) {
          // The min predicate size is bigger than the legal size for the
          // rest of the bale so far. There must be a rdregion that needs to
          // be split too much. Unbale it.
          IGC_ASSERT(InstWithMinWidth->Info.Type == BaleInfo::RDREGION);
          Instruction *RdToUnbale = InstWithMinWidth->Inst;
          Use *U = &*RdToUnbale->use_begin();
          auto User = cast<Instruction>(U->getUser());
          BaleInfo BI = Baling->getBaleInfo(User);
          BI.clearOperandBaled(U->getOperandNo());
          Baling->setBaleInfo(User, BI);
          ThisWidth = DETERMINEWIDTH_UNBALE;
        }
        ThisWidth = std::min(ThisWidth, PredWidths.Max);
      }
      break;
    }
    }
    if (ThisWidth < Width) {
      InstWithMinWidth = i;
      Width = ThisWidth;
    }
    if (Width == DETERMINEWIDTH_UNBALE)
      return DETERMINEWIDTH_UNBALE;
  }
  while (!(ExecSizeAllowedBits & Width)) {
    // This width is disallowed by the main instruction. We have already
    // dealt with the case where there is a minimum width above; the
    // code here is for when there is a particular disallowed width
    // (e.g. bfi disallows width 2 but allows 1). Try a smaller width.
    IGC_ASSERT(Width != 1);
    Width >>= 1;
  }
  if (Width > 1) {
    // Try to reduce width to isolate dead parts of the vector if there are any
    unsigned ReducedWidth = splitDeadElements(Width, StartIdx);
    if (ExecSizeAllowedBits & ReducedWidth)
      Width = ReducedWidth;
  }
  if (Width != WholeWidth && IsReadSameVector &&
      CurSplitKind == SplitKind_Normal) {
    // Splitting required, and the bale contains a rdregion from the same
    // vector as the wrregion's old value input, and we're not already
    // unbaling. Splitting that would result
    // in the original value of the vector and a new value being live at the
    // same time, so we avoid it by unbaling the wrregion.  The resulting
    // code will use an intermediate smaller register for the result of the
    // main inst before writing that back in to a region of the vector.
    //
    // Note that this unbaling is necessary despite pretty much the same
    // thing being done in second baling in GenXBaling::unbaleBadOverlaps.
    // Not doing the unbaling here results in code where the split rdregions
    // and wrregions are interleaved, so the unbaling in
    // GenXBaling::unbaleBadOverlaps does not actually stop the bad live range
    // overlap. (This might change if we had a pass to schedule to reduce
    // register pressure.)
    auto Head = B.getHeadIgnoreGStore();
    Head->Info.clearOperandBaled(GenXIntrinsic::GenXRegion::NewValueOperandNum);
    Baling->setBaleInfo(Head->Inst, Head->Info);
    LLVM_DEBUG(
        dbgs()
        << "GenXLegalization unbaling when rdr and wrr use same vector\n");
    return DETERMINEWIDTH_UNBALE;
  }
  if (Width == WholeWidth && !MustSplit) {
    // No split required, so return that to the caller, which then just
    // returns.  However we do not do that if MustSplit is set, because there
    // is some reason we need to go through splitting code anyway, one of:
    // 1. there is an rdregion that needs to be converted to multi indirect;
    // 2. there is an rdpredregion.
    return DETERMINEWIDTH_NO_SPLIT;
  }

  // If join is generated after splitting, need to check destination region rule
  {
    auto Head = B.getHeadIgnoreGStore();
    if (Head->Info.Type != BaleInfo::WRREGION &&
        Head->Info.Type != BaleInfo::WRPREDPREDREGION) {
      auto *VT = cast<IGCLLVM::FixedVectorType>(Head->Inst->getType());
      unsigned VecSize = VT->getNumElements();
      if (VecSize != Width) {
        if (!VT->getElementType()->isIntegerTy(1)) {
          Region R(Head->Inst);
          auto ThisWidth = getLegalRegionSizeForTarget(
              *ST, R, StartIdx, false /*no 2d for dst*/, true /*UseRealIdx*/,
              VecSize, &(Baling->AlignInfo));
          if (ThisWidth < Width) {
            Width = ThisWidth;
          }
        }
      }
    }
  }

  return Width;
}

/***********************************************************************
 * determineNonRegionWidth : determine max valid width of non-region instruction
 *
 * Width is determined based only on input and output vector element sizes and
 * register size (including predicates).
 *
 * Enter:   Inst = the instruction
 *          StartIdx = start index
 *
 * Return:  max valid width
 */
unsigned GenXLegalization::determineNonRegionWidth(Instruction *Inst,
                                                   unsigned StartIdx) {
  auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(Inst->getType());
  if (!VT)
    return 1;
  unsigned Width = VT->getNumElements() - StartIdx;
  unsigned BytesPerElement = VT->getElementType()->getPrimitiveSizeInBits() / genx::ByteBits;
  // Check whether the operand element size is bigger than the result operand
  // size. Normally we just check operand 0. This won't work on a select, and
  // we don't need to do the check on a select anyway as its operand and result
  // type are the same.
  if (!isa<SelectInst>(Inst)) {
    unsigned NumOperands = Inst->getNumOperands();
    if (CallInst *CI = dyn_cast<CallInst>(Inst))
      NumOperands = IGCLLVM::getNumArgOperands(CI);
    if (NumOperands) {
      IGC_ASSERT_MESSAGE(isa<VectorType>(Inst->getOperand(0)->getType()),
        "instruction not supported");
      unsigned InBytesPerElement =
          cast<VectorType>(Inst->getOperand(0)->getType())
              ->getElementType()
              ->getPrimitiveSizeInBits() /
          genx::ByteBits;
      if (InBytesPerElement > BytesPerElement)
        BytesPerElement = InBytesPerElement;
    }
  }
  unsigned NumGRF = 2;

  unsigned int MaxWidthAllowed = ST ? (NumGRF * ST->getGRFByteSize()) : 64;

  if (BytesPerElement) {
    // Non-predicate result.
    if (Width * BytesPerElement > MaxWidthAllowed)
      Width = MaxWidthAllowed / BytesPerElement;
    IGC_ASSERT_EXIT(Width > 0);
    Width = 1 << genx::log2(Width);
  } else {
    // Predicate result. This is to handle and/or/xor/not of predicates; cmp's
    // def of a predicate is handled separately where this function is called
    // in determineWidth().
    Width = getPredPart(Inst, StartIdx).Size;
  }
  return Width;
}

/***********************************************************************
 * getLegalPredSize : get legal predicate size
 *
 * Enter:   Pred = predicate value
 *          ElementTy = element type, 0 to assume not 64 bit
 *          StartIdx = start index in that predicate
 *          RemainingSize = remaining size from StartIdx in whole vector
 *                          operation being split, or 0 to imply from the
 *                          number of elements in the type of Pred
 *
 * Return:  Min = min legal size
 *          Max = max legal size
 */
LegalPredSize GenXLegalization::getLegalPredSize(Value *Pred, unsigned StartIdx,
                                                 unsigned RemainingSize) {
  // Get details of the part containing StartIdx.
  auto PP = getPredPart(Pred, StartIdx);
  LegalPredSize Ret;
  // 4-aligned predicates are not supported for native simd16.
  Ret.Min = ST->getGRFByteSize() > 32 ? 8 : 4;
  // Set Max to the remaining size left in this part, rounded down to a power
  // of two.
  unsigned LogMax = Log2_32(PP.Size - StartIdx + PP.Offset);
  // However, Max cannot be any bigger than the misalignment of the offset into
  // the part. For example. if the offset is 4 or 12, the size must be 4, not 8
  // or 16.
  LogMax = std::min(LogMax, findFirstSet(StartIdx - PP.Offset));
  IGC_ASSERT_EXIT(LogMax < 32);
  Ret.Max = 1 << LogMax;
  // If Min>Max, then we're at the end of that part and we don't need to ensure
  // that the next split in the same part is legally aligned.
  Ret.Min = std::min(Ret.Min, Ret.Max);
  return Ret;
}

/***********************************************************************
 * getPredPart : get info on which part of a predicate an index is in
 *
 * Enter:   V = a value of predicate type
 *          Offset = offset to get info on
 *
 * Return:  PredPart struct with
 *            Offset = start offset of the part
 *            Size = size of the part
 *            PartNum = part number
 *
 * On entry, Offset is allowed to be equal to the total size of V, in which
 * case the function returns PartNum = the number of parts and Size = 0.
 *
 * This function is what determines how an illegally sized predicate is divided
 * into parts. It is constrained by vISA only allowing a power of two size for
 * each part. Therefore it divides into zero or more 32 bit parts (currently 16
 * bit), then descending powers of two to fill up any odd size end.
 *
 * These parts correspond to how predicate values in the IR are divided up, not
 * just how instructions that use or define them get legalized. Thus a
 * predicate of size 13 actually gets divided into parts of 8,4,1 as vISA
 * predicate registers P1,P2,P3 (for example).
 */
PredPart GenXLegalization::getPredPart(Value *V, unsigned Offset) {
  unsigned WholeSize =
      cast<IGCLLVM::FixedVectorType>(V->getType())->getNumElements();
  PredPart Ret;
  if (Offset == WholeSize && !(WholeSize & (MaxPredSize - 1))) {
    Ret.Offset = Offset;
    Ret.Size = 0;
    Ret.PartNum = Offset / MaxPredSize;
    return Ret;
  }
  if ((Offset ^ WholeSize) & -MaxPredSize) {
    // This is in one of the 32 bit parts.
    Ret.Offset = Offset & -MaxPredSize;
    Ret.Size = MaxPredSize;
    Ret.PartNum = Offset / MaxPredSize;
    return Ret;
  }
  // This is in the odd less-than-32 section at the end.
  Ret.Offset = WholeSize & -MaxPredSize;
  Ret.PartNum = WholeSize / MaxPredSize;
  for (unsigned Pwr2 = MaxPredSize / 2U;; Pwr2 >>= 1) {
    if (Pwr2 <= Offset - Ret.Offset) {
      Ret.Offset += Pwr2;
      ++Ret.PartNum;
      if (Offset == WholeSize && Ret.Offset == Offset) {
        Ret.Size = 0;
        break;
      }
    }
    if (Pwr2 <= WholeSize - Ret.Offset && Pwr2 > Offset - Ret.Offset) {
      Ret.Size = Pwr2;
      break;
    }
  }
  return Ret;
}

/************************************************************************
 * SplittableInsts : takes Bale and constructs the range of splittable
 * instructions of this bale
 *
 * Splittable are those instructions that later will be split. By current design
 * it is all instruction except last wrregion or wrregion+gstore.
 *
 * Usage: for (auto BI : SplittableInsts(B)), SplittableInst(B).begin(),...
 */
class SplittableInsts {
  Bale::iterator Begin;
  Bale::iterator End;

public:
  SplittableInsts(Bale &SomeBale) : Begin(SomeBale.begin()) {
    auto HeadIt = SomeBale.getHeadIgnoreGStoreIt();
    // Only WRREGION, WRPREDPREDREGION, GSTORE should be joined, thus the
    // instructions before them should be split
    if (HeadIt->Info.Type == BaleInfo::WRREGION ||
        HeadIt->Info.Type == BaleInfo::WRPREDPREDREGION)
      End = HeadIt;
    else {
      IGC_ASSERT_MESSAGE(HeadIt->Info.Type != BaleInfo::GSTORE,
        "GSTORE must have been considered before");
      End = SomeBale.end();
    }
  }
  Bale::iterator begin() { return Begin; }
  Bale::iterator end() { return End; }
};

/***********************************************************************
 * joinBaleInsts : create join instructions in bale
 *                 (2 in case of gstore, 1 - otherwise)
 */
Value *GenXLegalization::joinBaleInsts(Value *PrevSliceRes, unsigned StartIdx,
                                       unsigned Width,
                                       Instruction *InsertBefore) {
  IGC_ASSERT_MESSAGE(SplittableInsts(B).end() != B.end(),
    "must have some instructions to join in the bale");
  if (B.endsWithGStore()) {
    IGC_ASSERT_MESSAGE(SplittableInsts(B).end() == B.getPreHeadIt(),
      "a bale is considered to have only 1 dst, in case of GSTORE it's "
      "represented by the last 2 instructions");
    return joinGStore(PrevSliceRes, *B.getHead(), *B.getPreHead(), StartIdx,
                      Width, InsertBefore);
  } else {
    IGC_ASSERT_MESSAGE(SplittableInsts(B).end() == B.getHeadIt(),
      "a bale is considered to have only 1 dst, in common case it's "
      "represented by the last instruction");
    return joinAnyWrRegion(PrevSliceRes, *B.getHead(), StartIdx, Width,
                           InsertBefore);
  }
}

/***********************************************************************
 * If the last instruction in the created bale is a split instruction,
 * need to join this result into the overall result with a wrregion or
 * wrpredregion. Do not generate the join if it is a write into the whole
 * of the overall result, which can happen when going through the split
 * code even when no split is required other than conversion to multi
 * indirect.
 */
Value *GenXLegalization::joinBaleResult(Value *PrevSliceRes,
                                        Value *LastSplitInst, unsigned StartIdx,
                                        unsigned Width,
                                        Instruction *InsertBefore) {
  IGC_ASSERT_MESSAGE(PrevSliceRes, "wrong argument");
  IGC_ASSERT_MESSAGE(LastSplitInst, "wrong argument");
  IGC_ASSERT_MESSAGE(InsertBefore, "wrong argument");
  auto Head = B.getHeadIgnoreGStore()->Inst;
  auto *VT = cast<IGCLLVM::FixedVectorType>(Head->getType());
  IGC_ASSERT_MESSAGE(VT->getNumElements() != Width,
    "there's no need to join results if they have the proper type");
  if (VT->getElementType()->isIntegerTy(1)) {
    auto NewWr = Region::createWrPredRegion(
        PrevSliceRes, LastSplitInst, StartIdx,
        LastSplitInst->getName() + ".join" + Twine(StartIdx), InsertBefore,
        Head->getDebugLoc());
    // If this is the first wrpredregion into an illegally sized predicate,
    // save it for processing later. (Only the first one could possibly be
    // the root of a tree of wrpredregions, and only the roots of
    // wrpredregion trees need to be in IllegalPredicates.)
    if (!StartIdx) {
      auto PredSize = getLegalPredSize(NewWr, 0);
      if (PredSize.Max !=
          cast<IGCLLVM::FixedVectorType>(NewWr->getType())->getNumElements())
        IllegalPredicates.insert(NewWr);
    }
    return NewWr;
  } else {
    Region R(Head);
    R.Width = R.NumElements = Width;
    R.Offset = StartIdx * R.ElementBytes;
    return R.createWrRegion(PrevSliceRes, LastSplitInst,
                            LastSplitInst->getName() + ".join" +
                                Twine(StartIdx),
                            InsertBefore, Head->getDebugLoc());
  }
}

/***********************************************************************
 * splitBale : create one slice of the bale
 *
 * Enter:   PrevSliceRes = result of previously created bale slice,
 *          undef if this is the first one
 *          StartIdx = element start index for this slice
 *          Width = number of elements in this slice
 *          InsertBefore = insert new inst before this point
 *
 * Return:  result of this split
 */
Value *GenXLegalization::splitBale(Value *PrevSliceRes, unsigned StartIdx,
                                   unsigned Width, Instruction *InsertBefore) {
  Value *LastCreatedInst = nullptr;
  auto SplittableInstsRange = SplittableInsts(B);
  for (auto &BI : SplittableInstsRange)
    // Split the instruction.
    SplitMap[BI.Inst] = LastCreatedInst =
        splitInst(PrevSliceRes, BI, StartIdx, Width, InsertBefore,
                  BI.Inst->getDebugLoc());
  if (SplittableInstsRange.end() != B.end())
    LastCreatedInst =
        joinBaleInsts(PrevSliceRes, StartIdx, Width, InsertBefore);
  else {
    IGC_ASSERT_MESSAGE(LastCreatedInst, "must have at least some split inst");
    auto Head = B.getHeadIgnoreGStore()->Inst;
    if (cast<IGCLLVM::FixedVectorType>(Head->getType())->getNumElements() !=
        Width) {
      IGC_ASSERT_EXIT(PrevSliceRes);
      LastCreatedInst = joinBaleResult(PrevSliceRes, LastCreatedInst, StartIdx,
                                       Width, InsertBefore);
    }
  }
  SplitMap.clear();
  return LastCreatedInst;
}

// joins both gstore inst and the wrregion which gstore stores
// more info at joinAnyWrRegion
Value *GenXLegalization::joinGStore(Value *PrevSliceRes, BaleInst GStore,
                                    BaleInst WrRegion, unsigned StartIdx,
                                    unsigned Width, Instruction *InsertBefore) {
  IGC_ASSERT_MESSAGE(GStore.Info.Type == BaleInfo::GSTORE, "wrong argument");
  Value *Op =
      joinAnyWrRegion(PrevSliceRes, WrRegion, StartIdx, Width, InsertBefore);
  auto *Inst = new StoreInst(Op, GStore.Inst->getOperand(1), /*volatile*/ true,
                             InsertBefore);
  Inst->setDebugLoc(GStore.Inst->getDebugLoc());
  return Inst;
}

// specialized join function for wrregion instruction
// more info at joinAnyWrRegion
Value *GenXLegalization::joinWrRegion(Value *PrevSliceRes, BaleInst BInst,
                                      unsigned StartIdx, unsigned Width,
                                      Instruction *InsertBefore) {
  IGC_ASSERT_MESSAGE(BInst.Info.Type == BaleInfo::WRREGION, "wrong argument");
  Region R = makeRegionFromBaleInfo(BInst.Inst, BInst.Info);
  // For SplitIdx==0, the old vector value comes from the original
  // wrregion. Otherwise it comes from the split wrregion created
  // last time round.
  Value *In = !StartIdx ? BInst.Inst->getOperand(0) : PrevSliceRes;
  if (CurSplitKind == SplitKind::SplitKind_GStore && StartIdx != 0) {
    auto *SI = cast<StoreInst>(B.getHead()->Inst);
    auto *Load = new LoadInst(SI->getValueOperand()->getType(),
                              SI->getPointerOperand(), ".gload",
                              /*volatile*/ true, InsertBefore);
    Load->setDebugLoc(BInst.Inst->getDebugLoc());
    In = Load;
  }
  R.getSubregion(StartIdx, Width);
  if (R.Mask && isa<VectorType>(R.Mask->getType()))
    R.Mask = getSplitOperand(
        BInst.Inst, GenXIntrinsic::GenXRegion::PredicateOperandNum, StartIdx,
        Width, InsertBefore, BInst.Inst->getDebugLoc());
  Value *WriteValue = getSplitOperand(BInst.Inst, 1, StartIdx, Width,
                                      InsertBefore, BInst.Inst->getDebugLoc());
  return R.createWrRegion(In, WriteValue,
                          BInst.Inst->getName() + ".join" + Twine(StartIdx),
                          InsertBefore, BInst.Inst->getDebugLoc());
}

// specialized join function for wrpredpredregion instruction
// more info at joinAnyWrRegion
Value *GenXLegalization::joinPredPredWrRegion(Value *PrevSliceRes,
                                              BaleInst BInst, unsigned StartIdx,
                                              unsigned Width,
                                              Instruction *InsertBefore) {
  IGC_ASSERT_MESSAGE(BInst.Info.Type == BaleInfo::WRPREDPREDREGION, "wrong argument");
  unsigned WrPredStart =
      cast<ConstantInt>(BInst.Inst->getOperand(2))->getZExtValue();
  Value *WrPredNewVal = getSplitOperand(
      BInst.Inst, 1, StartIdx, Width, InsertBefore, BInst.Inst->getDebugLoc());
  // For SplitIdx==0, the old vector value comes from the original
  // wrregion. Otherwise it comes from the split wrregion created
  // last time round.
  Value *In = !StartIdx ? BInst.Inst->getOperand(0) : PrevSliceRes;
  // Create the split wrpredpredregion. Note that the mask is passed in
  // its original unsplit form; the spec of wrpredpredregion is that the
  // mask is the same size as the result, and the index is used to slice
  // the mask as well as to determine the slice where the value is written
  // in the result.
  return Region::createWrPredPredRegion(
      In, WrPredNewVal, StartIdx + WrPredStart, BInst.Inst->getOperand(3),
      BInst.Inst->getName() + ".join" + Twine(StartIdx), InsertBefore,
      BInst.Inst->getDebugLoc());
}

/***********************************************************************
 * joinAnyWrRegion : join any wrregion instruction in the bale
 *
 * Enter:   PrevSliceRes = result of previously created bale slice,
 *          undef if this is the first one
 *          BInst = the BaleInst to join
 *          StartIdx = element start index for this slice
 *          Width = number of elements in this slice
 *          InsertBefore = insert new inst before this point
 *
 * Return:  the new join value. Join value/instruction has original ("illegal")
 *          width elements. Each bale slice writes its own part of the value.
 */
Value *GenXLegalization::joinAnyWrRegion(Value *PrevSliceRes, BaleInst BInst,
                                         unsigned StartIdx, unsigned Width,
                                         Instruction *InsertBefore) {
  switch (BInst.Info.Type) {
  case BaleInfo::WRREGION:
    return joinWrRegion(PrevSliceRes, BInst, StartIdx, Width, InsertBefore);
    break;
  case BaleInfo::WRPREDPREDREGION:
    return joinPredPredWrRegion(PrevSliceRes, BInst, StartIdx, Width,
                                InsertBefore);
    break;
  default:
      IGC_ASSERT_UNREACHABLE(); // unexpected/unsupported instruction
  }
}

/***********************************************************************
 * splitInst : split an instruction in the bale
 *
 * Enter:   PrevSliceRes = result of previous bale slice,
 *          undef if this is the first one
 *          BInst = the BaleInst to split
 *          StartIdx = element start index for this slice
 *          Width = number of elements in this slice
 *          InsertBefore = insert new inst before this point
 *          DL = debug location to give new instruction(s)
 *
 * Return:  the new split value
 *          Split value/instruction has Width elements.
 */
Value *GenXLegalization::splitInst(Value *PrevSliceRes, BaleInst BInst,
                                   unsigned StartIdx, unsigned Width,
                                   Instruction *InsertBefore,
                                   const DebugLoc &DL) {
  // Clone gvload on users split not to trigger false positives
  // in global volatile clobbering checker.
  if (B.isGStoreBale() &&
      PrevSliceRes /*first slice uses the original vload*/) {
    for (auto *GvLoad : genx::getSrcVLoads(BInst.Inst)) {
      if (genx::getBitCastedValue(B.getHead()->Inst->getOperand(1)) !=
          genx::getBitCastedValue(GvLoad->getOperand(0)))
        continue;
      if (GvLoad->users().end() != llvm::find(GvLoad->users(), BInst.Inst)) {
        auto *GvLoadClone = GvLoad->clone();
        GvLoadClone->insertBefore(InsertBefore);
        GvLoadClone->setName(BInst.Inst->getName() + ".gvload_use_split_clone");
        BInst.Inst->replaceUsesOfWith(GvLoad, GvLoadClone);
      } else {
        IGC_ASSERT_MESSAGE(
            0, "Cloning of gvloads followed by bitcasts is not yet supported.");
      }
    }
  }

  switch (BInst.Info.Type) {
  case BaleInfo::GSTORE:
  case BaleInfo::WRREGION:
  case BaleInfo::WRPREDPREDREGION:
    IGC_ASSERT_EXIT_MESSAGE(0, "these instructions must be processed in join functions");
    break;
  case BaleInfo::RDREGION: {
    // Allow for this being a rdregion baled in to a TWICEWIDTH operand.
    // If it is, double the start index and width.
    unsigned Doubling = TwiceWidth && BInst.Inst == *TwiceWidth;
    StartIdx <<= Doubling;
    Width <<= Doubling;
    // Get the subregion.
    Region R = makeRegionFromBaleInfo(BInst.Inst, BInst.Info);
    // Check whether this is an indirect operand that was allowed only
    // because we assumed that we are going to convert it to a multi
    // indirect.
    bool ConvertToMulti =
        R.Indirect && Width != 1 &&
        getLegalRegionSizeForTarget(
            *ST, R, StartIdx, true /*Allow2D*/, true /*UseRealIdx*/,
            cast<IGCLLVM::FixedVectorType>(BInst.Inst->getOperand(0)->getType())
                ->getNumElements(),
            &(Baling->AlignInfo)) == 1;
    if (R.ElementBytes == genx::ByteBytes &&
        !ST->hasMultiIndirectByteRegioning())
      ConvertToMulti = false;
    // The region to read from. This is normally from the input region baled
    // in. If this is reading from and writing to the same region and
    // split progapation is on, then just reading from the last joined value
    // (but not the initial undef).
    //
    Value *OldVal = BInst.Inst->getOperand(0);
    if (PrevSliceRes && !isa<UndefValue>(PrevSliceRes) &&
        CurSplitKind == SplitKind_Propagation) {
      auto Head = B.getHeadIgnoreGStore();
      if (Head->Info.Type == BaleInfo::WRREGION) {
        Value *WrRegionInput = Head->Inst->getOperand(0);
        if (OldVal == WrRegionInput)
          OldVal = PrevSliceRes;
      }
    }
    R.getSubregion(StartIdx, Width);
    if (!ConvertToMulti) {
      // Not converting to multi indirect.
      return R.createRdRegion(
          OldVal, BInst.Inst->getName() + ".split" + Twine(StartIdx),
          InsertBefore, DL);
    }
    // Converting to multi indirect.
    return convertToMultiIndirect(BInst.Inst, OldVal, &R, InsertBefore);
  }
  case BaleInfo::RDPREDREGION: {
    unsigned RdPredStart =
        cast<ConstantInt>(BInst.Inst->getOperand(1))->getZExtValue();
    Value *RdPredInput = BInst.Inst->getOperand(0);
    return Region::createRdPredRegionOrConst(
        RdPredInput, RdPredStart + StartIdx, Width,
        BInst.Inst->getName() + ".split" + Twine(StartIdx), InsertBefore, DL);
  }
  case BaleInfo::SHUFFLEPRED: {
    // If we need to split predication shuffle vector, then we definitely failed to
    // bale it with channel instruction. In this case we do not need such complicated
    // predication logic anymore and can fallback to rdpredregions.
    auto *SI = cast<ShuffleVectorInst>(BInst.Inst);
    auto RS = ShuffleVectorAnalyzer::getReplicatedSliceDescriptor(SI);
    IGC_ASSERT_MESSAGE(RS.SliceSize == Width, "Unexpected width for predicate shuffle split");
    Value *Pred = SI->getOperand(0);
    return Region::createRdPredRegionOrConst(
        Pred, RS.InitialOffset, Width,
        SI->getName() + ".split" + Twine(StartIdx), InsertBefore, DL);
  }
  }
  // Splitting non-region instruction.
  IGC_ASSERT_MESSAGE(!isa<PHINode>(BInst.Inst), "not expecting to split phi node");
  if (CastInst *CI = dyn_cast<CastInst>(BInst.Inst)) {
    Type *CastToTy = IGCLLVM::FixedVectorType::get(
        cast<VectorType>(CI->getType())->getElementType(), Width);
    Instruction *NewInst = CastInst::Create(
        CI->getOpcode(),
        getSplitOperand(CI, 0, StartIdx, Width, InsertBefore, DL), CastToTy,
        CI->getName() + ".split" + Twine(StartIdx), InsertBefore);
    NewInst->setDebugLoc(DL);
    return NewInst;
  }
  if (BinaryOperator *BO = dyn_cast<BinaryOperator>(BInst.Inst)) {
    auto Split1 = getSplitOperand(BO, 0, StartIdx, Width, InsertBefore, DL),
         Split2 = getSplitOperand(BO, 1, StartIdx, Width, InsertBefore, DL);
    Instruction *NewInst = BinaryOperator::Create(
        BO->getOpcode(), Split1, Split2,
        BO->getName() + ".split" + Twine(StartIdx), InsertBefore);
    NewInst->setDebugLoc(DL);
    return NewInst;
  }
  if (UnaryOperator *UO = dyn_cast<UnaryOperator>(BInst.Inst)) {
    Instruction *NewInst = UnaryOperator::Create(
        UO->getOpcode(),
        getSplitOperand(UO, 0, StartIdx, Width, InsertBefore, DL),
        UO->getName() + ".split" + Twine(StartIdx), InsertBefore);
    NewInst->setDebugLoc(DL);
    return NewInst;
  }
  if (CmpInst *CI = dyn_cast<CmpInst>(BInst.Inst)) {
    auto Split1 = getSplitOperand(CI, 0, StartIdx, Width, InsertBefore, DL),
         Split2 = getSplitOperand(CI, 1, StartIdx, Width, InsertBefore, DL);
    Instruction *NewInst = CmpInst::Create(
        CI->getOpcode(), CI->getPredicate(), Split1, Split2,
        CI->getName() + ".split" + Twine(StartIdx), InsertBefore);
    NewInst->setDebugLoc(DL);
    return NewInst;
  }
  if (auto SI = dyn_cast<SelectInst>(BInst.Inst)) {
    Value *Selector = getSplitOperand(SI, 0, StartIdx, Width, InsertBefore, DL);
    Selector = splatPredicateIfNecessary(Selector, Width, InsertBefore, DL);
    auto Split1 = getSplitOperand(SI, 1, StartIdx, Width, InsertBefore, DL);
    auto Split2 = getSplitOperand(SI, 2, StartIdx, Width, InsertBefore, DL);
    auto NewInst = SelectInst::Create(
        Selector, Split1, Split2, SI->getName() + ".split" + Twine(StartIdx),
        InsertBefore);
    NewInst->setDebugLoc(DL);
    return NewInst;
  }
  // Must be a splittable intrinsic.
  CallInst *CI = dyn_cast<CallInst>(BInst.Inst);
  IGC_ASSERT(CI);
  unsigned IntrinID = vc::getAnyIntrinsicID(CI);
  IGC_ASSERT(vc::isAnyNonTrivialIntrinsic(IntrinID));
  if (IntrinID == GenXIntrinsic::genx_constanti ||
      IntrinID == GenXIntrinsic::genx_constantf) {
    // This is the constant loading intrinsic.
    // We don't need to load the split constants, since a constant value-to-
    // write operand is valid in the wrregions that will be used to link
    // the values back together.
    return getSplitOperand(BInst.Inst, 0, StartIdx, Width, InsertBefore, DL);
  }

  // Some other splittable intrinsic.
  SmallVector<Value *, 2> Args;
  SmallVector<Type *, 2> OverloadedTypes;
  OverloadedTypes.push_back(IGCLLVM::FixedVectorType::get(
      cast<VectorType>(BInst.Inst->getType())->getElementType(),
      Width)); // RetTy
  for (unsigned I = 0, E = IGCLLVM::getNumArgOperands(CI); I != E; ++I) {
    Use *U = &CI->getOperandUse(I);
    if (U == Fixed4) {
      Args.push_back(CI->getArgOperand(I));
    } else if (U == TwiceWidth) {
      // TWICEWIDTH: operand is twice the width of other operand and result
      Args.push_back(getSplitOperand(BInst.Inst, I, StartIdx * 2, Width * 2,
                                     InsertBefore, DL));
    } else
      Args.push_back(
          getSplitOperand(BInst.Inst, I, StartIdx, Width, InsertBefore, DL));
    if (vc::isOverloadedArg(IntrinID, I))
      OverloadedTypes.push_back(Args[I]->getType());
  }
  Module *M = InsertBefore->getModule();
  Function *Decl = vc::getAnyDeclaration(M, IntrinID, OverloadedTypes);
  auto Name = ".split" + Twine(StartIdx);
  Instruction *NewInst = CallInst::Create(Decl, Args, Name, InsertBefore);
  NewInst->setDebugLoc(DL);
  return NewInst;
}

/***********************************************************************
 * getSplitOperand : get a possibly split operand
 *
 * Enter:   Inst = original non-split instruction
 *          OperandNum = operand number we want
 *          StartIdx = element start index for this split
 *          Size = number of elements in this split
 *          InsertBefore = where to insert any added rdregion
 *          DL = debug location to give new instruction(s)
 *
 * If the requested operand is a constant, it splits the constant.
 * Otherwise it creates an rdregion from the original operand.
 */
Value *GenXLegalization::getSplitOperand(Instruction *Inst, unsigned OperandNum,
                                         unsigned StartIdx, unsigned Size,
                                         Instruction *InsertBefore,
                                         const DebugLoc &DL) {
  Value *V = Inst->getOperand(OperandNum);
  if (!isa<VectorType>(V->getType()))
    return V; // operand not vector, e.g. variable index in region
  if (auto C = dyn_cast<Constant>(V))
    return getConstantSubvector(C, StartIdx, Size);
  // Split a non-constant vector.
  if (Instruction *OperandInst = dyn_cast<Instruction>(V)) {
    auto i = SplitMap.find(OperandInst);
    if (i != SplitMap.end()) {
      // Operand is another instruction in the bale being split.
      return i->second;
    }
  }
  // Non-constant operand not baled in.
  // Create an rdregion for the operand.
  if (!V->getType()->getScalarType()->isIntegerTy(1)) {
    Region R(V);
    R.getSubregion(StartIdx, Size);
    return R.createRdRegion(V, V->getName() + ".split" + Twine(StartIdx),
                            InsertBefore, DL);
  }
  // Predicate version.
  return Region::createRdPredRegion(V, StartIdx, Size,
                                    V->getName() + ".split" + Twine(StartIdx),
                                    InsertBefore, DL);
}

/***********************************************************************
 * convertToMultiIndirect : convert a rdregion into multi-indirect
 *
 * Enter:   Inst = original rdregion
 *          LastJoinVal = the acutal region to read from
 *          R = region for it, already subregioned if applicable
 *
 * Return:  new rdregion instruction (old one has not been erased)
 */
Instruction *
GenXLegalization::convertToMultiIndirect(Instruction *Inst, Value *LastJoinVal,
                                         Region *R, Instruction *InsertBefore) {
  IGC_ASSERT(!R->is2D());
  IGC_ASSERT((R->NumElements == 4) || (R->NumElements == 8));
  Value *Indirect = R->Indirect;
  IGC_ASSERT(Indirect);
  const DebugLoc &DL = Inst->getDebugLoc();

  // scalar indirect index
  if (R->Stride == 1 && !R->is2D() && !isa<VectorType>(Indirect->getType()) &&
      ST->hasIndirectGRFCrossing()) {
    Instruction *NewInst =
        R->createRdRegion(LastJoinVal, Inst->getName(), InsertBefore, DL);
    return NewInst;
  }

  // 1. Splat the address. (We will get multiple copies of this
  // instruction, one per split, but they will be CSEd away.)
  Instruction *SplattedIndirect =
      CastInst::Create(Instruction::BitCast, Indirect,
                       IGCLLVM::FixedVectorType::get(Indirect->getType(), 1),
                       Twine(Indirect->getName()) + ".splat", InsertBefore);
  SplattedIndirect->setDebugLoc(DL);
  Region AddrR(SplattedIndirect);
  AddrR.Stride = 0;
  AddrR.Width = AddrR.NumElements = R->NumElements;
  SplattedIndirect = AddrR.createRdRegion(
      SplattedIndirect, SplattedIndirect->getName(), InsertBefore, DL);
  // 2. Add the constant vector <0,1,2,3,4,5,6,7> to it (adjusted
  // for stride in bytes).
  uint16_t OffsetValues[8];
  for (unsigned i = 0; i != 8; ++i)
    OffsetValues[i] = i * (R->Stride * R->ElementBytes);
  Constant *Offsets = ConstantDataVector::get(
      InsertBefore->getContext(),
      ArrayRef<uint16_t>(OffsetValues).slice(0, R->NumElements));
  SplattedIndirect =
      BinaryOperator::Create(Instruction::Add, SplattedIndirect, Offsets,
                             SplattedIndirect->getName(), InsertBefore);
  SplattedIndirect->setDebugLoc(DL);
  // 3. Create the multi indirect subregion.
  R->Indirect = SplattedIndirect;
  R->VStride = R->Stride;
  R->Stride = 1;
  R->Width = 1;
  Instruction *NewInst =
      R->createRdRegion(LastJoinVal, Inst->getName(), InsertBefore, DL);
  return NewInst;
}

// Get value bitcasted to NewTy.
static Value *createBitCastIfNeeded(Value *V, Type *NewTy,
                                    Instruction *InsertBefore,
                                    const DebugLoc &DbgLoc) {
  if (auto *C = dyn_cast<Constant>(V))
    return ConstantFoldCastOperand(Instruction::BitCast, C, NewTy,
                                   InsertBefore->getModule()->getDataLayout());
  if (GenXIntrinsic::isReadPredefReg(V)) {
    // we don't need unnecessary bitcasts of read.predef.reg intrinsics
    // as we can simply create a new call with an appropriate type
    auto *CI = cast<CallInst>(V);
    Function *RegReadIntr = GenXIntrinsic::getGenXDeclaration(
        InsertBefore->getModule(), llvm::GenXIntrinsic::genx_read_predef_reg,
        {NewTy, CI->getOperand(1)->getType()});
    auto *Inst = CallInst::Create(
        RegReadIntr, {CI->getOperand(0), CI->getOperand(1)}, "", InsertBefore);
    Inst->setDebugLoc(DbgLoc);
    return Inst;
  }
  if (auto *BCI = dyn_cast<BitCastInst>(V)) {
    if (BCI->getSrcTy() == NewTy)
      return BCI->getOperand(0);
  }
  auto *Inst = CastInst::Create(Instruction::BitCast, V, NewTy,
                                V->getName() + ".cast", InsertBefore);
  Inst->setDebugLoc(DbgLoc);
  return Inst;
}

/***********************************************************************
 * transformMoveType : transform move bale to new integer type.
 *
 * Enter:   B = bale (not necessarily a byte move)
 *          FromTy = old type of move
 *          ToTy = required new type
 *
 * Return:  0 if nothing changed, else the new head of bale (ignoring the
 *          bitcasts inserted either side)
 *
 * If the code is modified, it updates bale info.
 *
 * This transformation needs to be done when baling info is available, so
 * legalization is a handy place to put it.
 */
Instruction *GenXLegalization::transformMoveType(Bale *B, IntegerType *FromTy,
                                                 IntegerType *ToTy) {
  using GenXIntrinsic::GenXRegion::NewValueOperandNum;
  using GenXIntrinsic::GenXRegion::OldValueOperandNum;

  IGC_ASSERT_MESSAGE(FromTy != ToTy, "Convertion of same types attempted");
  // Recognize move dst and src in bale.
  auto Head = B->getHead();
  auto HeadInst = Head->Inst;
  if (HeadInst->getType()->getScalarType() != FromTy)
    return nullptr;
  Instruction *Wr = nullptr, *Rd = nullptr;
  if (Head->Info.Type == BaleInfo::WRREGION) {
    Wr = HeadInst;
    if (Head->Info.isOperandBaled(NewValueOperandNum)) {
      auto *BaledInst = cast<Instruction>(HeadInst->getOperand(NewValueOperandNum));
      auto BaledInstInfo = Baling->getBaleInfo(BaledInst);
      if (BaledInstInfo.Type == BaleInfo::RDREGION)
        Rd = BaledInst;
      // Allow single bitcast instruction in wrregion input.
      else if (!isa<BitCastInst>(BaledInst) || BaledInstInfo.isOperandBaled(0))
        return nullptr;
    }
  } else if (Head->Info.Type == BaleInfo::RDREGION)
    Rd = HeadInst;
  // Either wrregion or rdregion should be presented.
  if (!Wr && !Rd)
    return nullptr;

  Value *Src = Rd ? Rd->getOperand(OldValueOperandNum)
                  : Wr->getOperand(NewValueOperandNum);
  Region SrcRgn = Rd ? makeRegionFromBaleInfo(Rd, BaleInfo())
                     : Region(Wr->getOperand(NewValueOperandNum));
  Value *Dst = Wr ? Wr : Rd;
  if (Dst->hasOneUse() && GenXIntrinsic::isWritePredefReg(Dst->user_back()))
    return nullptr;
  Region DstRgn = Wr ? makeRegionFromBaleInfo(Wr, BaleInfo()) : Region(Rd);

  // If destination region is not contiguous, changing element type can be achived
  // by conversion to 2D region, but we try to avoid it because such dst operands
  // are not supported in HW and require additional code for emulation.
  if (DstRgn.Stride != 1)
    return nullptr;

  const auto *DL = &HeadInst->getModule()->getDataLayout();
  Type *NewSrcTy = genx::changeVectorType(Src->getType(), ToTy, DL),
       *NewDstTy = genx::changeVectorType(Dst->getType(), ToTy, DL);
  if (!NewSrcTy || !NewDstTy)
    return nullptr;
  if (!SrcRgn.changeElementType(ToTy, DL) ||
      !DstRgn.changeElementType(ToTy, DL))
    return nullptr;

  IGC_ASSERT(SrcRgn.NumElements == DstRgn.NumElements);
  const auto &RdDbgLoc = Rd ? Rd->getDebugLoc() : Wr->getDebugLoc();
  Instruction *NewWr = nullptr, *NewRd = nullptr;
  // Transform src operand.
  Value *NewSrc = createBitCastIfNeeded(Src, NewSrcTy, HeadInst, RdDbgLoc);
  if (Rd) {
    NewSrc = NewRd =
        SrcRgn.createRdRegion(NewSrc, Rd->getName(), HeadInst, RdDbgLoc);
    Baling->setBaleInfo(NewRd, Baling->getBaleInfo(Rd));
  }
  // Transform dst operand.
  Value *NewDst = NewSrc;
  const auto &WrDbgLoc = Wr ? Wr->getDebugLoc() : Rd->getDebugLoc();
  if (Wr) {
    Value *NewOldVal = createBitCastIfNeeded(Wr->getOperand(OldValueOperandNum),
                                             NewDstTy, HeadInst, WrDbgLoc);
    NewDst = NewWr = DstRgn.createWrRegion(NewOldVal, NewSrc, Wr->getName(),
                                           HeadInst, WrDbgLoc);
    Baling->setBaleInfo(NewWr, Baling->getBaleInfo(Wr));
  }
  IGC_ASSERT(NewWr || NewRd);

  // Create bitcast to OldTy of NewDst
  auto *Bitcast = CastInst::Create(Instruction::BitCast, NewDst, Dst->getType(),
                                   Dst->getName() + ".cast", HeadInst);
  Bitcast->setDebugLoc(WrDbgLoc);
  Dst->replaceAllUsesWith(Bitcast);

  if (Wr)
    eraseInst(Wr);
  if (Rd)
    eraseInst(Rd);
  return NewWr ? NewWr : NewRd;
}

/***********************************************************************
 * splatPredicateIfNecessary : splat a wrregion/select predicate if necessary
 *
 * Enter:   V = the predicate
 *          Width = width it needs to be splatted to
 *          InsertBefore = where to insert new instructions
 *          DL = debug loc for new instructions
 *
 * Return:  the predicate, possibly a new instruction
 *
 * From GenXLegalization onwards, the predicate (mask) in a wrregion must
 * either be scalar constant 1, or have the same vector width as the value
 * being written by the wrregion. Similarly for the selector in a vector
 * select, except that is not allowed to be scalar constant 1.
 *
 * It might make more sense to do this in GenXLowering, except that the
 * predicate might be wider than 32 at that point. So we have to do it here.
 */
Value *GenXLegalization::splatPredicateIfNecessary(Value *V,
                                                   Type *ValueToWriteTy,
                                                   Instruction *InsertBefore,
                                                   const DebugLoc &DL) {
  if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(ValueToWriteTy))
    return splatPredicateIfNecessary(V, VT->getNumElements(), InsertBefore, DL);
  return V;
}

Value *GenXLegalization::splatPredicateIfNecessary(Value *V, unsigned Width,
                                                   Instruction *InsertBefore,
                                                   const DebugLoc &DL) {
  if (Width == 1)
    return V;
  if (auto C = dyn_cast<Constant>(V))
    if (C->isAllOnesValue())
      return V;
  if (isa<VectorType>(V->getType()))
    return V;
  // Round Width up to 16 or 32. (No point in using up a 32 bit predicate
  // register if we only need 16.)
  unsigned RoundedWidth = Width > 16 ? 32 : 16;
  // Use a select to turn the predicate into 0 or -1.
  auto ITy = Type::getIntNTy(InsertBefore->getContext(), RoundedWidth);
  auto Sel = SelectInst::Create(
      V, Constant::getAllOnesValue(ITy), Constant::getNullValue(ITy),
      InsertBefore->getName() + ".splatpredicate", InsertBefore);
  Sel->setDebugLoc(DL);
  // Bitcast that to v16i1 or v32i1 predicate (which becomes a setp
  // instruction).
  Instruction *Res = CastInst::Create(
      Instruction::BitCast, Sel,
      IGCLLVM::FixedVectorType::get(Type::getInt1Ty(InsertBefore->getContext()),
                                    RoundedWidth),
      InsertBefore->getName() + ".splatpredicate", InsertBefore);
  Res->setDebugLoc(DL);
  // If the required size is smaller, do an rdpredregion.
  if (Width == RoundedWidth)
    return Res;
  return Region::createRdPredRegionOrConst(
      Res, 0, Width, Res->getName() + ".rdpredregion", InsertBefore, DL);
}

/***********************************************************************
 * eraseInst : erase instruction, updating CurrentInst if we're erasing that
 */
void GenXLegalization::eraseInst(Instruction *Inst) {
  removingInst(Inst);
  // If the result is a predicate, ensure it is removed from IllegalPredicates,
  // just in case it is a wrpredregion that was in IllegalPredicates.
  if (auto VT = dyn_cast<VectorType>(Inst->getType()))
    if (VT->getElementType()->isIntegerTy(1))
      IllegalPredicates.remove(Inst);
  Inst->eraseFromParent();
}

void GenXLegalization::removingInst(Instruction *Inst) {
  if (Inst == CurrentInst)
    CurrentInst = Inst->getNextNode();
}

/***********************************************************************
 * fixIllegalPredicates : fix illegally sized predicate values
 */
struct StackEntry {
  Instruction *Wr;     // the wrpredregion this stack entry is for
  Instruction *Parent; // its parent wrpredregion in the tree
  SmallVector<Value *, 4> Parts;
  // Constructor given wrpredregion and parent.
  StackEntry(Instruction *Wr, Instruction *Parent) : Wr(Wr), Parent(Parent) {}
};

void GenXLegalization::fixIllegalPredicates(Function *F) {
  // First fix illegal size predicate phi nodes, replacing each with multiple
  // phi nodes with rdpredregion on the incomings and wrpredregion on the
  // result. These rdpredregions and wrpredregions then get removed with other
  // illegal size predicates in the code below.
  SmallVector<PHINode *, 4> PhisToErase;
  for (auto fi = F->begin(), fe = F->end(); fi != fe; ++fi) {
    auto BB = &*fi;
    Instruction *FirstNonPhi = BB->getFirstNonPHI();
    for (auto Phi = dyn_cast<PHINode>(BB->begin()); Phi;
         Phi = dyn_cast<PHINode>(Phi->getNextNode())) {
      if (!Phi->getType()->getScalarType()->isIntegerTy(1))
        continue;
      // We have a predicate phi. Get the first part of it, which might show
      // that we do not need to split it at all.
      auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(Phi->getType());
      if (!VT)
        continue;
      unsigned WholeSize = VT->getNumElements();
      auto PP = getPredPart(Phi, 0);
      if (PP.Size == WholeSize)
        continue;
      // We do need to split.
      Value *Joined = UndefValue::get(Phi->getType());
      unsigned NumIncoming = Phi->getNumIncomingValues();
      for (unsigned StartIdx = 0; StartIdx != WholeSize;) {
        // Create a split phi node.
        PP = getPredPart(Phi, StartIdx);
        auto *NewPhi = PHINode::Create(
            IGCLLVM::FixedVectorType::get(Phi->getType()->getScalarType(),
                                          PP.Size),
            NumIncoming, Phi->getName() + ".split" + Twine(StartIdx), Phi);
        NewPhi->setDebugLoc(Phi->getDebugLoc());
        // Do a rdpredregion for each incoming.
        for (unsigned ii = 0; ii != NumIncoming; ++ii) {
          BasicBlock *IncomingBlock = Phi->getIncomingBlock(ii);
          Value *Incoming = Phi->getIncomingValue(ii);
          auto NewRd = Region::createRdPredRegionOrConst(
              Incoming, StartIdx, PP.Size,
              Incoming->getName() + ".split" + Twine(StartIdx),
              IncomingBlock->getTerminator(), DebugLoc());
          NewPhi->addIncoming(NewRd, IncomingBlock);
        }
        // Join with previous new phis for this original phi.
        Joined = Region::createWrPredRegion(Joined, NewPhi, StartIdx,
                                            Phi->getName() + ".join" +
                                                Twine(StartIdx),
                                            FirstNonPhi, DebugLoc());
        // If that was the first join, add it to the IllegalPredicates list for
        // processing its tree of wrpredregions below.
        if (!StartIdx)
          IllegalPredicates.insert(cast<Instruction>(Joined));
        StartIdx += PP.Size;
      }
      // Replace the original phi and mark it for erasing. Also undef out its
      // incomings so it doesn't matter what order we do the erases in.
      auto Undef = UndefValue::get(Phi->getType());
      for (unsigned ii = 0; ii != NumIncoming; ++ii)
        Phi->setIncomingValue(ii, Undef);
      Phi->replaceAllUsesWith(Joined);
      PhisToErase.push_back(Phi);
    }
  }
  for (auto i = PhisToErase.begin(), e = PhisToErase.end(); i != e; ++i)
    (*i)->eraseFromParent();
  // For each entry in IllegalPredicates that is the root of a tree of
  // wrpredregions...
  SmallVector<Instruction *, 4> ToErase;
  for (auto ipi = IllegalPredicates.begin(), ipe = IllegalPredicates.end();
       ipi != ipe; ++ipi) {
    std::vector<StackEntry> Stack;
    auto Root = *ipi;
    if (GenXIntrinsic::getGenXIntrinsicID(Root->getOperand(0)) ==
        GenXIntrinsic::genx_wrpredregion)
      continue; // not root of tree
    IGC_ASSERT_MESSAGE(isa<UndefValue>(Root->getOperand(0)),
      "expecting undef input to root of tree");
    // See if it really is illegally sized.
    if (getPredPart(Root, 0).Size ==
        cast<IGCLLVM::FixedVectorType>(Root->getType())->getNumElements())
      continue;
    // For traversing the tree, create a stack where each entry represents a
    // value in the tree, and contains the values of the parts.  Create an
    // initial entry for the root of the tree.
    Stack.push_back(StackEntry(Root, nullptr));
    // Process stack entries.
    while (!Stack.empty()) {
      auto Entry = &Stack.back();
      if (!Entry->Parts.empty()) {
        // This stack entry has already been processed; we are on the way back
        // down having processed its children. Just pop the stack entry, and
        // mark the wrpredregion for erasing. We do not erase it now because it
        // might be yet to visit in the IllegalPredicates vector.
        ToErase.push_back(Entry->Wr);
        Stack.pop_back();
        continue;
      }
      // Populate Parts with the value of each part from the parent.
      if (!Entry->Parent) {
        // No parent. All parts are undef.
        auto Ty = Entry->Wr->getType();
        unsigned WholeSize =
            cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements();
        for (unsigned Offset = 0; Offset != WholeSize;) {
          auto PP = getPredPart(Entry->Wr, Offset);
          Entry->Parts.push_back(UndefValue::get(
              IGCLLVM::FixedVectorType::get(Ty->getScalarType(), PP.Size)));
          Offset += PP.Size;
        }
      } else {
        // Inherit from parent.
        for (auto i = (Entry - 1)->Parts.begin(), e = (Entry - 1)->Parts.end();
             i != e; ++i)
          Entry->Parts.push_back(*i);
      }
      // For this wrpredregion, determine the part that it writes to, and see
      // if it is the whole part. (It cannot overlap more than one part,
      // because getLegalPredSize ensured that all splits were within parts.)
      unsigned WrOffset =
          cast<ConstantInt>(Entry->Wr->getOperand(2))->getZExtValue();
      unsigned WrSize =
          cast<IGCLLVM::FixedVectorType>(Entry->Wr->getOperand(1)->getType())
              ->getNumElements();
      auto PP = getPredPart(Entry->Wr, WrOffset);
      IGC_ASSERT_MESSAGE(WrOffset + WrSize <= PP.Offset + PP.Size,
        "overlaps multiple parts");
      Value *Part = Entry->Parts[PP.PartNum];
      if (WrSize != PP.Size) {
        // Not the whole part. We need to write into the previous value of this
        // part.
        auto NewWr = Region::createWrPredRegion(
            Part, Entry->Wr->getOperand(1), WrOffset - PP.Offset, "", Entry->Wr,
            Entry->Wr->getDebugLoc());
        NewWr->takeName(Entry->Wr);
        Part = NewWr;
      } else
        Part = Entry->Wr->getOperand(1);
      // Store the new value of this part.
      Entry->Parts[PP.PartNum] = Part;
      // Gather uses in rdpredregion.
      SmallVector<Instruction *, 4> Rds;
      for (auto ui = Entry->Wr->use_begin(), ue = Entry->Wr->use_end();
           ui != ue; ++ui) {
        auto User = cast<Instruction>(ui->getUser());
        if (GenXIntrinsic::getGenXIntrinsicID(User) ==
            GenXIntrinsic::genx_rdpredregion)
          Rds.push_back(User);
      }
      // For each rdpredregion, turn it into a read from the appropriate
      // part.
      for (auto ri = Rds.begin(), re = Rds.end(); ri != re; ++ri) {
        Instruction *Rd = *ri;
        unsigned RdOffset =
            cast<ConstantInt>(Rd->getOperand(1))->getZExtValue();
        unsigned RdSize =
            cast<IGCLLVM::FixedVectorType>(Rd->getType())->getNumElements();
        auto PP = getPredPart(Entry->Wr, RdOffset);
        IGC_ASSERT_MESSAGE(RdOffset + RdSize <= PP.Offset + PP.Size,
          "overlaps multiple parts");
        Value *Part = Entry->Parts[PP.PartNum];
        if (RdSize != PP.Size) {
          // Only reading a subregion of a part.
          // Assertion tests if the rdpredregion is legal. In fact we will probably
          // have to cope with an illegal one, by generating code to bitcast
          // the predicate to a scalar int (or finding code where it is already
          // bitcast from a scalar int), using bit twiddling to get the
          // required subregion, and bitcasting back.  I think this situation
          // will arise where the input to legalization had an odd size
          // rdpredregion in a wrregion where the input predicate is a v32i1
          // from an odd size CM select using an i32 as the mask.

          if (RdOffset) {
            unsigned RdMisalignment = 0; // it will be assigned inside assertion statament
            IGC_ASSERT((RdMisalignment = 1U << findFirstSet(RdOffset), 1));
            IGC_ASSERT_MESSAGE((RdMisalignment >= 8 ||
                    (RdMisalignment == 4 && Rd->hasOneUse() &&
                     cast<Instruction>(Rd->use_begin()->getUser())
                             ->getOperand(1)
                             ->getType()
                             ->getScalarType()
                             ->getPrimitiveSizeInBits() == 64)),
                   "illegal rdpredregion");
            IGC_ASSERT(RdSize);
            IGC_ASSERT_MESSAGE(!((RdOffset - PP.Offset) % RdSize),
                   "illegal rdpredregion");
            (void) RdMisalignment;
          }

          // Create a new rdpredregion.
          auto NewRd = Region::createRdPredRegion(
              Part, RdOffset - PP.Offset, RdSize, "", Rd, Rd->getDebugLoc());
          NewRd->takeName(Rd);
          Part = NewRd;
        }
        // Replace the original rdpredregion with the value of the part.
        Rd->replaceAllUsesWith(Part);
        Rd->eraseFromParent();
      }
      StackEntry EntryCopy =
          *Entry; // Coping ref to don't invalidates Stack below
      // All remaining uses must be wrpredregion. Push them onto the stack.
      for (auto ui = EntryCopy.Wr->use_begin(), ue = EntryCopy.Wr->use_end();
           ui != ue; ++ui) {
        auto User = cast<Instruction>(ui->getUser());
        IGC_ASSERT_MESSAGE(GenXIntrinsic::getGenXIntrinsicID(User) == GenXIntrinsic::genx_wrpredregion,
          "expecting only wrpredregion uses");
        IGC_ASSERT_MESSAGE(!ui->getOperandNo(),
          "expecting only wrpredregion uses");
        Stack.push_back(StackEntry(User, EntryCopy.Wr));
      }
    }
  }
  // Erase the old wrpredregions.
  for (auto i = ToErase.begin(), e = ToErase.end(); i != e; ++i)
    (*i)->eraseFromParent();
}

GenXLegalization::SplitKind GenXLegalization::checkBaleSplittingKind() {
  if (B.endsWithGStore())
    return SplitKind::SplitKind_GStore;

  auto Head = B.getHeadIgnoreGStore();
  SplitKind Kind = SplitKind::SplitKind_Normal;

  if (Head->Info.Type == BaleInfo::WRREGION) {
    Value *WrRegionInput = Head->Inst->getOperand(0);
    Region R1 = makeRegionFromBaleInfo(Head->Inst, Head->Info);
    for (auto &I : B) {
      if (I.Info.Type != BaleInfo::RDREGION)
        continue;
      if (I.Inst->getOperand(0) != WrRegionInput)
        continue;
      Region R2 = makeRegionFromBaleInfo(I.Inst, I.Info);
      if (R1 != R2) {
        // Check if R1 overlaps with R2. Create a new region for R1 as we are
        // rewriting region offsets if their difference is a constant.
        Region R = makeRegionFromBaleInfo(Head->Inst, Head->Info);

        // Analyze dynamic offset difference, but only for a scalar offset.
        if (R1.Indirect && R2.Indirect) {
          if (R1.Indirect->getType()->isVectorTy() ||
              R2.Indirect->getType()->isVectorTy())
            return SplitKind::SplitKind_Normal;

          // Strip truncation from bitcast followed by a region read.
          auto stripConv = [](Value *Val) {
            if (GenXIntrinsic::isRdRegion(Val)) {
              CallInst *CI = cast<CallInst>(Val);
              Region R = makeRegionFromBaleInfo(CI, BaleInfo());
              if (R.Offset == 0 && R.Width == 1)
                Val = CI->getOperand(0);
              if (auto BI = dyn_cast<BitCastInst>(Val))
                Val = BI->getOperand(0);
            }
            return Val;
          };

          Value *Offset1 = stripConv(R.Indirect);
          Value *Offset2 = stripConv(R2.Indirect);
          if (Offset1->getType() == Offset2->getType()) {
            auto S1 = SE->getSCEV(Offset1);
            auto S2 = SE->getSCEV(Offset2);
            auto Diff = SE->getMinusSCEV(S1, S2);
            IGC_ASSERT(R.Indirect);
            Diff = SE->getTruncateOrNoop(Diff, R.Indirect->getType());
            if (auto SCC = dyn_cast<SCEVConstant>(Diff)) {
              ConstantInt *CI = SCC->getValue();
              int OffsetDiff = std::abs(static_cast<int>(CI->getSExtValue()));
              R.Offset = 0;
              R.Indirect = nullptr;
              R2.Offset = OffsetDiff;
              R2.Indirect = nullptr;
            }
          }
        }

        // Ignore the mask and adjust both offsets by a common dynamic
        // value if exists. If the resulting regions do not overlap, then two
        // original regions do not overlap.
        R.Mask = nullptr;
        R2.Mask = nullptr;

        // As both R and R2 have constant offsets, the overlap function
        // should check their footprints accurately.
        if (R.overlap(R2))
          return SplitKind::SplitKind_Normal;
        Kind = SplitKind::SplitKind_Propagation;
        continue;
      }

      // (1) 1D direct regions or indirect regions with single offset.
      // (2) 2D direct regions with VStride >= Width, or indirect regions with
      //     single offset.
      bool IsMultiAddr = R1.Indirect && R1.Indirect->getType()->isVectorTy();
      if (!R1.is2D()) {
        if (IsMultiAddr)
          return SplitKind::SplitKind_Normal;
        Kind = SplitKind::SplitKind_Propagation;
      } else {
        if (R1.VStride < (int)R1.Width || IsMultiAddr)
          return SplitKind::SplitKind_Normal;
        Kind = SplitKind::SplitKind_Propagation;
      }
    }
  }

  return Kind;
}

// This function deals with intrinsic calls with special restrictions.
// - Certain intrinsic calls should be placed in the entry blocks:
//     llvm.genx.predifined.surface
//
void GenXLegalization::fixIntrinsicCalls(Function *F) {
  auto PF = F->getParent()->getFunction("llvm.genx.predefined.surface");
  if (!PF)
    return;

  // Collect all calls to PF in this function.
  std::map<int64_t, std::vector<Instruction *>> Calls;
  for (auto U : PF->users()) {
    if (auto UI = dyn_cast<CallInst>(U)) {
      BasicBlock *BB = UI->getParent();
      if (BB->getParent() != F)
        continue;
      if (auto CI = dyn_cast<ConstantInt>(UI->getOperand(0))) {
        int64_t Arg = CI->getSExtValue();
        Calls[Arg].push_back(UI);
      }
    }
  }

  BasicBlock *EntryBB = &F->getEntryBlock();
  Instruction *InsertPos = &*EntryBB->getFirstInsertionPt();

  for (const auto &I : Calls) {
    Instruction *EntryDef = nullptr;
    for (auto *Inst : I.second) {
      if (Inst->getParent() == EntryBB) {
        EntryDef = Inst;
        break;
      }
    }

    // No entry definition found, then clone one.
    if (EntryDef == nullptr) {
      EntryDef = I.second.front()->clone();
      EntryDef->insertBefore(InsertPos);
      llvm::replaceAllDbgUsesWith(*I.second.front(), *EntryDef, *InsertPos,
                                  *DT);
    } else
      EntryDef->moveBefore(InsertPos);

    // Now replace all uses with this new definition.
    for (auto *Inst : I.second) {
      SmallVector<Instruction *, 8> WorkList{Inst};
      while (!WorkList.empty()) {
        auto *CurI = WorkList.pop_back_val();

        for (auto UI = CurI->use_begin(); UI != CurI->use_end();) {
          Use &U = *UI++;
          // Skip if this use just comes from EntryDef.
          if (EntryDef == U.get())
            continue;
          // All uses of this PHI will be replaced as well.
          if (auto PHI = dyn_cast<PHINode>(U.getUser()))
            WorkList.push_back(PHI);
          U.set(EntryDef);
        }
        if (CurI->use_empty())
          CurI->eraseFromParent();
      }
    }
  }
}
