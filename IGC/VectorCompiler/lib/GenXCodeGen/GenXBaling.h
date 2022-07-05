/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXBaling
/// ----------
///
/// GenX instruction baling is the process of determining which LLVM instructions
/// can be combined into a single vISA instruction. Such a group of LLVM
/// instructions is known as a *bale*. A bale typically has a *main instruction*
/// and then optionally modifiers and region instructions on the sources and
/// the destination of the main instruction. However it is possible to have a
/// bale with no main instruction, for example just a rdregion, a modifier and
/// a wrregion.
///
/// Bale example
/// ^^^^^^^^^^^^
///
/// .. image:: GenXDesign_bale.png
///
/// This example shows a bale that is pretty much as complicated as you can get in
/// a single bale. Each small blue box is an LLVM IR instruction, with arrows showing
/// how each one is used. Other than the *bale head* instruction at the top, an
/// instruction in a bale has only one use, which is within the bale.
///
/// The baling pass
/// ^^^^^^^^^^^^^^^
///
/// GenX instruction baling happens in two parts:
///
/// 1. The GenXBaling pass sets up a map to give each Instruction
///    a *BaleInfo*, which contains a field giving the role the instruction
///    plays in its enclosing bale (main instruction, rdregion, etc), and a
///    bit vector where a bit is set if the corresponding operand of the
///    instruction is another instruction that is baled in (part of the same
///    bale).
///
///    GenXBaling is in fact two slightly different passes run at two different
///    times:
///
///    * The GenXFuncBaling pass (a FunctionPass) runs before GenXLegalization,
///      which uses it but invalidates it as it changes the code. This is known
///      as *first baling*.
///
///    * The GenXGroupBaling pass (a FunctionGroupPass) runs after GenXLiveness.
///      From GenXLiveness, baling information remains valid through to
///      GenXCisaBuilder, since any code changes made (such as adding
///      copies where coalescing fails) either do not invalidate the analysis,
///      or the pass making the change also updates the baling analysis.
///
///    The GenXBaling pass also detects where an instruction is baled in to
///    another, but the instruction has other uses too. In this case it clones the
///    instruction. Thus we end up with any baled in instruction having only
///    one use (with an exception for goto/join -- see below).
///
///    Thus the GenXBaling pass is not a pure analysis, as it can modify the
///    code.
///
/// 2. Using the map set up by the GenXBaling analysis, several functions are
///    provided for use by other passes:
///
///    * getBaleInfo()/setBaleInfo() allow another pass to directly inspect and modify
///      the baling info for an instruction. The BaleInfo for an instruction gives:
///
///      - Type, the role of the instruction in the bale (e.g. it is a rdregion);
///      - a bitmap of which operands are baled into it, together with methods
///        for getting and setting the bit for a particular operand.
///
///    * getBaleParent() returns the instruction that the given instruction is
///      baled into, if any
///
///    * isBaled() says whether the given instruction is baled into anything
///
///    * getBaleHead() returns the instruction at the head of the bale that the
///      given instruction is baled into, which is the same as the given instruction
///      if it is not baled into anything.
///
///    * buildBale() takes a head instruction (one for which isBaled is false) and
///      fills out a Bale struct with a vector of BaleInst structs for all the
///      instructions in the bale, where each BaleInst contains a pointer to the
///      instruction and its BaleInfo struct (as in getBaleInfo()/setBaleInfo()).
///
/// Criteria for baling
/// ^^^^^^^^^^^^^^^^^^^
///
/// GenXBaling implements the criteria for baling, i.e. when different LLVM IR
/// instructions can be combined into the same vISA instruction:
///
/// * A rdregion with a variable index can bale in an add constant (where the
///   constant is splatted if vector) that generates the index. In second baling,
///   the constant add is in fact a ``llvm.genx.add.addr`` intrinsic, because that
///   is what GenXCategory converted it to.
///
/// * GenXBaling is where an instruction gets recognized as a modifier, for example
///   subtract from 0 is a negate modifier. The instruction is left as it is, and
///   its modifier equivalent (e.g. ``BaleInfo::NEGMOD``) is set up in the
///   instruction's BaleInfo.
///
/// * SExt/ZExt are also treated as modifiers, although not always balable. See
///   below.
///
/// * A modifier can bale in an rdregion.
///
/// * A modifier can bale in another modifier in some circumstances.
///
/// * In particular, SExt/ZExt normally cannot bale in another modifier, but they
///   are allowed to bale in an abs modifier as a bodge to fix a problem where
///   the LLVM IR generated for ``cm_abs`` does not properly represent its
///   semantics. See ``dc93b907 GenXBaling: bodge to work around cm_abs problems``.
///
/// * A main instruction can bale a modifier or rdregion into each operand in some
///   circumstances:
///
///   - Some ALU intrinsics have region requirements, e.g. oword aligned,
///     contiguous. GenXBaling enforces those requirements by only baling in an
///     rdregion that satifies them, but only in second baling. First baling does
///     the baling anyway, as we want GenXLegalization to consider the instructions
///     as one bale as it might legalize in a way that makes the region legal for
///     the instruction.
///
///   - Baling an SExt/ZExt in is how we represent a vISA instruction such as
///     ``add`` with a result type different to operand type. The two operands can
///     have different types too in Gen, but vISA insists they are the same (if not
///     constant). So:
///
///     1. In first baling, we allow SExt/ZExt from different types to be baled in
///        to the two operands. This tends to make GenXLegalization legalize them
///        to the same vector width as the main instruction.
///
///     2. In second baling, we do not allow SExt/ZExt from different types (or one
///        SExt/ZExt where the other operand does not have one) to be baled in. This
///        yields a legal vISA instruction, but having done (1) also allows the
///        finalizer to fold the extend into the instruction.
///
///   - A raw operand (of a send or shared function intrinsic) has its own
///     restrictions -- it can bale in a rdregion, but the region has to be
///     contiguous and GRF aligned.
///
///   - There is special case code for where send or a shared function intrinsic
///     has a ``TWOADDR`` raw operand, one that does not appear as a vISA operand
///     in its own right but is implicitly the same register as the result. The
///     twoaddr raw operand can bale in a rdregion (with region contiguous and GRF
///     aligned) as long as the result can be baled into a wrregion with the same
///     region parameters and the same "old value" input. This represents where a
///     send or shared function intrinsic does a predicated partial write, and the
///     place it does the partial write to is a region in a vISA register.
///
/// * ``llvm.genx.sat`` represents floating point saturation, and is a modifier that
///   is different to the other modifiers because it is not a source modifier. A
///   saturate can bale in a main instruction or modifier or rdregion.
///
/// * A wrregion can do the following baling:
///
///   - It can bale a main instruction (subject to region restrictions in second
///     baling), a saturate, a modifier or a rdregion into its "new value" input.
///
///   - Like rdregion, it can bale a constant add into its index operand.
///
/// * Anything with a predicate (wrregion, select, send, all/any, some shared
///   function intrinsics) can bale in a predicate not, and any of those things,
///   including the not, can bale in an rdpredregion to represent using e.g. an M3
///   flag to use only part of the predicate. However predicate baling is not
///   done in first baling, as GenXLegalization does not want to consider the
///   operations together.
///
/// * Anything with a scalar i1 condition (select, br) can bale in an all/any.
///
/// Baling of goto/join into br
/// ^^^^^^^^^^^^^^^^^^^^^^^^^^^
///
/// The goto and join intrinsics have multiple return values, returned in a single
/// struct. One of the return values is the scalar i1 !any value that is then used
/// in a conditional branch.
///
/// In second baling, we want the goto/join, the extractvalue of the !any
/// result, and the conditional branch to be baled together, so we can generate
/// a single goto/join instruction.
///
/// However the struct result of the goto/join has other uses, the extractvalues of
/// the other results. Thus, in this special case, we have a bale where the
/// goto/join instruction inside the bale has uses other than the inside-bale use.
/// This needs special case code to handle in GenXBaling.
///
/// In the future it may be worth considering a generalization of this idea of a
/// bale that is not a strict tree of instructions, so that we can use LLVM IR
/// to model Gen instructions with a general result and a flag result. Currently
/// we cannot do that, which means:
///
/// 1. we cannot represent addc properly;
///
/// 2. we cannot represent any combined arithmetic-and-set-flags instruction,
///    although that is not too much of a problem as the jitter derives such an
///    instruction by folding a cmp into an arithmetic instruction.
///
/// Alignment requirements
/// ^^^^^^^^^^^^^^^^^^^^^^
///
/// An additional function of the second baling pass is that, when it bales a
/// raw result intrinsic into a wrregion, it marks the wrregion's LiveRange as
/// needing to be 32 aligned, and when it bales a rdregion into a raw operand in
/// an intrinsic, it marks the rdregion's input's LiveRange as needing to be 32
/// aligned. GenXCategory sets most alignment requirements, but baling in
/// a rdregion or baling a main instruction into a wrregion imposes alignment
/// requirements on the vISA register that the region is read from or written to.
///
//===----------------------------------------------------------------------===//
#ifndef GENXBALING_H
#define GENXBALING_H

#include "FunctionGroup.h"
#include "GenX.h"
#include "GenXAlignmentInfo.h"
#include "GenXSubtarget.h"

#include "vc/Utils/GenX/Region.h"

#include "IgnoreRAUWValueMap.h"
#include "Probe/Assertion.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Pass.h"
#include <string>

namespace llvm {
  class BranchInst;
  class CallInst;
  class DebugLoc;
  class GenXLiveness;
  class Instruction;
  class raw_ostream;
  class Twine;
  class Value;

namespace genx {

// BaleInfo : bale info for one instruction
struct BaleInfo {
  // Type is how this instruction relates to its bale, whether it is a
  // rdregion, wrregion, modifier, or main instruction.
  enum { MAININST, WRREGION, SATURATE, NOTMOD, NEGMOD, ABSMOD,
      RDREGION, ADDRADD, ADDROR, FADDR, RDPREDREGION, ALLANY, NOTP, ZEXT, SEXT,
      SHUFFLEPRED, WRPREDREGION, WRPREDPREDREGION, CMPDST, GSTORE, REGINTR };
  uint16_t Type;
  uint16_t Bits; // bitmap of which operands are baled in
  BaleInfo(int Type = MAININST, unsigned Bits = 0) : Type(Type), Bits(Bits) {}
  // isOperandBaled() : read Bits to see if operand is baled
  bool isOperandBaled(unsigned OperandNum) const { return (Bits >> OperandNum) & 1; }
  // clearOperandBaled() : clear bit that says that operand is baled
  void clearOperandBaled(unsigned OperandNum) { Bits &= ~(1 << OperandNum); }
  // setOperandBaled() : set bit that says that operand is baled
  void setOperandBaled(unsigned OperandNum) { Bits |= 1 << OperandNum; }
  // getTypeString : get string for BaleInfo type
  const char *getTypeString() const;
};

bool operator==(const BaleInfo &lhs, const BaleInfo &rhs);

// BaleInst : one instruction in a bale
struct BaleInst {
  Instruction *Inst;
  BaleInfo Info;
  BaleInst(Instruction *Inst, BaleInfo Info) : Inst(Inst), Info(Info) {}
};

bool operator==(const BaleInst &lhs, const BaleInst &rhs);

// Bale : all the instructions in a bale, filled out by buildBale()
class Bale {
  typedef SmallVector<BaleInst, 8> Insts_t;
  Insts_t Insts;
  hash_code Hash;
public:
  Bale() : Hash(0) {}
  void clear() { Insts.clear(); Hash = 0; }
  // push_front : push an instruction onto the "front", i.e. it is baled
  // into an instruction already in the bale
  void push_front(BaleInst BI) { Insts.push_back(BI); }
  BaleInst &front() { return Insts.back(); }
  // push_back : push an instruction onto the "back", i.e. it is the new
  // head instruction, and the old head instruction is baled into it.
  // This does an inefficient insert, but is only used in legalization
  // when adding a wrregion to a bale that does not already have one.
  void push_back(BaleInst BI) { Insts.insert(Insts.begin(), BI); }
  BaleInst &back() { return Insts.front(); }
  // Forward iterator: gives an instruction before any use of it, with the
  // head instruction of the bale coming last.
  typedef Insts_t::reverse_iterator iterator;
  typedef Insts_t::const_reverse_iterator const_iterator;
  iterator begin() { return Insts.rbegin(); }
  iterator end() { return Insts.rend(); }
  const_iterator begin() const { return Insts.rbegin(); }
  const_iterator end() const { return Insts.rend(); }
  unsigned size() const { return Insts.size(); }
  bool empty() const { return Insts.empty(); }
  // getIteratorPos : get 0..31 unsigned representing position of
  // Bale::iterator.
  unsigned getIteratorPos(iterator i) {
    IGC_ASSERT((unsigned)(i - Insts.rbegin()) < 32);
    return i - Insts.rbegin();
  }
  // Reverse iterator: gives an instruction after any use of it, with the
  // head instruction of the bale coming first.
  typedef Insts_t::iterator reverse_iterator;
  typedef Insts_t::const_iterator const_reverse_iterator;
  reverse_iterator rbegin() { return Insts.begin(); }
  const_reverse_iterator rbegin() const { return Insts.begin(); }
  reverse_iterator rend() { return Insts.end(); }
  const_reverse_iterator rend() const { return Insts.end(); }
  // getHead : get head instruction of the bale
  iterator getHeadIt() { return std::prev(end()); }
  const_iterator getHeadIt() const { return std::prev(end()); }
  BaleInst *getHead() { return &*getHeadIt(); }
  const BaleInst *getHead() const { return &*getHeadIt(); }
  // getPreHead : returns instruction prior to head instruction
  // unsafe: if there's no such instruction, behavior is undefined
  iterator getPreHeadIt() { return std::prev(getHeadIt()); }
  BaleInst *getPreHead() { return &*getPreHeadIt(); }
  // If a bale ends with a g_store bale, return the baled instruction prior to
  // this g_store instruction.
  iterator getHeadIgnoreGStoreIt() {
    if (endsWithGStore())
      return getPreHeadIt();
    return getHeadIt();
  }
  BaleInst *getHeadIgnoreGStore() {
    return &*getHeadIgnoreGStoreIt();
  }
  bool endsWithGStore() const {
    return !empty() && rbegin()->Info.Type == BaleInfo::GSTORE;
  }
  // getMainInst : get 0 else the main inst from the bale
  const BaleInst *getMainInst() const;
  // hash : set hash code for bale. Must be called before using comparison
  // operators.
  void hash();
  hash_code getHash() const {
    return Hash;
  }
  // Comparison operators. Two bales are equivalent if they compute the same
  // value, that is, they have the same opcodes in the instructions, the
  // instructions are baled together in the same way, and the operands coming
  // in from outside the bale are the same.
  bool operator==(const Bale &Other) const { return !compare(Other); }
  bool operator!=(const Bale &Other) const { return compare(Other); }
  bool operator<(const Bale &Other) const { return compare(Other) < 0; }
  int compare(const Bale &Other) const;
  // eraseFromParent : do eraseFromParent on all instructions in the bale
  void eraseFromParent();
  // Debug dump/print
  void dump() const;
  void print(raw_ostream &OS) const;
  bool isGstoreBale() const { return endsWithGStore(); }
  bool isGStoreBaleLegal() const;
};

inline raw_ostream &operator<<(raw_ostream &OS, const Bale &B) {
  B.print(OS);
  return OS;
}

} // end namespace genx

//----------------------------------------------------------------------
// GenXBaling : the baling information for a Function or FunctionGroup (depending
// on whether GenXFuncBaling or GenXGroupBaling created it)
class GenXBaling {
  typedef llvm::ValueMap<const Value*, genx::BaleInfo,
                         IgnoreRAUWValueMapConfig<const Value *>>
      InstMap_t;
  const GenXSubtarget *ST;
  InstMap_t InstMap;
  struct NeedClone {
    Instruction *Inst;
    unsigned OperandNum;
    NeedClone(Instruction *Inst = 0, unsigned OperandNum = 0)
        : Inst(Inst), OperandNum(OperandNum) {}
    bool operator==(const NeedClone &Other) const {
      return Inst == Other.Inst && OperandNum == Other.OperandNum;
    }
  };
  typedef SmallVector<NeedClone, 8> NeedCloneStack_t;
  NeedCloneStack_t NeedCloneStack;
  SmallVector<CallInst *, 4> TwoAddrSends;
protected:
  BalingKind Kind;
  const DominatorTree *DT;
  GenXLiveness *Liveness; // only in group baling
public:
  genx::AlignmentInfo AlignInfo;
public:
  explicit GenXBaling(BalingKind BKind, const GenXSubtarget *Subtarget)
      : Kind(BKind), ST(Subtarget), Liveness(nullptr), DT(nullptr) {}
  // clear : clear out the analysis
  void clear() { InstMap.clear(); }
  // processFunction : process one Function
  bool processFunction(Function *F);
  // processInst : recalculate the baling info for an instruction
  void processInst(Instruction *Inst);
  // getBaleInfo : get BaleInfo for an instruction
  genx::BaleInfo getBaleInfo(const Instruction *Inst) const {
    InstMap_t::const_iterator i = InstMap.find(Inst);
    return i == InstMap.end() ? genx::BaleInfo() : i->second;
  }
  // setBaleInfo : set BaleInfo for an instruction
  void setBaleInfo(const Instruction *Inst, genx::BaleInfo BI);
  // isBaled : test whether all uses of an instruction would be baled in to
  // users
  bool isBaled(const Instruction *Inst) const { return getBaleParent(Inst); }
  // canSplitBale : check if instruction can be splitted
  bool canSplitBale(Instruction *Inst) const;
  // getBaleParent : return the instruction baled into, 0 if none
  Instruction *getBaleParent(const Instruction *Inst) const;
  // unbale : unbale an instruction from its bale parent
  void unbale(Instruction *Inst);
  // getBaleHead : return the head of the bale containing this instruction
  Instruction *getBaleHead(Instruction *Inst) const;
  // buildBale : build Bale from head instruction. B assumed empty on entry
  void buildBale(Instruction *Inst, genx::Bale *B, bool IncludeAddr = false) const;
  // store : store updated BaleInfo for Instruction (used to unbale by
  // GenXLegalization)
  void store(genx::BaleInst BI);
  // getIndexAdd : test whether the specified value is a constant add/sub that
  //   could be baled in as a variable index offset, but without checking that
  //   the index is in range
  static bool getIndexAdd(Value *V, int *Offset);
  // getIndexOr : test whether the specified value is a constant or that
  //   could be baled in as a variable index offset, but without checking that
  //   the index is in range
  static bool getIndexOr(Value *V, int &Offset);
  // isBalableIndexAdd : test whether the specified value is a constant
  // add/sub that could be baled in as a variable index offset
  static bool isBalableIndexAdd(Value *V);
  // isBalableIndexOr : test whether the specified value is a constant
  // or that could be baled in as a variable index offset
  static bool isBalableIndexOr(Value *V);
  // isBalableNewValueIntoWrr: check whether the new val operand can
  // be baled into wrr instruction
  bool isBalableNewValueIntoWrr(Value *V, const vc::Region &WrrR);

  static bool isHighCostBaling(uint16_t Type, Instruction *Inst);
  // Debug dump/print
  void dump() const;
  void print(raw_ostream &OS) const;

private:
  // methods to build the info when running the analysis
  void processWrPredRegion(Instruction *Inst);
  void processWrPredPredRegion(Instruction *Inst);
  void processWrRegion(Instruction *Inst);
  bool processSelect(Instruction *Inst);
  void processStore(StoreInst *Inst);
  bool processShufflePred(Instruction *Inst);
  bool processPredicate(Instruction *Inst, unsigned OperandNum);
  void processSat(Instruction *Inst);
  void processRdRegion(Instruction *Inst);
  void processRdPredRegion(Instruction *Inst);
  void processInlineAsm(Instruction *Inst);
  void processExtractValue(ExtractValueInst *EV);
  void processFuncPointer(Instruction *Inst);
  void processRdWrPredefReg(Instruction *Inst);
  void processMainInst(Instruction *Inst, int IntrinID);
  bool processSelectToPredicate(SelectInst *SI);
  // Helper func for buildBale
  void buildBaleSub(Instruction *Inst, genx::Bale *B, bool IncludeAddr) const;
  void processBranch(BranchInst *Branch);
  void processTwoAddrSend(CallInst *CI);
  void setOperandBaled(Instruction *Inst, unsigned OperandNum, genx::BaleInfo *BI);
  void doClones();
  Instruction *getOrUnbaleExtend(Instruction *Inst, genx::BaleInfo *BI,
                                 unsigned OperandNum, bool Unbale);
  int getAddrOperandNum(unsigned IID) const;

  bool operandCanBeBaled(Instruction *Inst, unsigned OperandNum, int ModType,
                         unsigned ArgInfoBits);

  bool isRegionOKForIntrinsic(unsigned ArgInfoBits, const vc::Region &R,
                              bool CanSplitBale);
  bool isSafeToMove(Instruction *Op, Instruction *From, Instruction *To);

  // Cleanup and optimization before do baling on a function.
  bool prologue(Function *F);
};

//----------------------------------------------------------------------
// The GenXFuncBaling analysis pass
// (used for the first baling just before GenXLegalization)
class GenXFuncBaling : public FunctionPass, public GenXBaling {
public:
  static char ID;
  explicit GenXFuncBaling(BalingKind Kind = BalingKind::BK_Legalization,
                          const GenXSubtarget *ST = nullptr)
      : FunctionPass(ID), GenXBaling(Kind, ST) {}
  StringRef getPassName() const override {
    return "GenX instruction baling analysis for a function";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnFunction(Function &F) override {
    clear();
    DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    return processFunction(&F);
  }
  // createPrinterPass : get a pass to print the IR, together with the GenX
  // specific analyses
  Pass *createPrinterPass(raw_ostream &O,
                          const std::string &Banner) const override {
    return createGenXPrinterPass(O, Banner);
  }
  void print(raw_ostream &OS, const Module *M) const override {
    GenXBaling::print(OS);
  }
};
void initializeGenXFuncBalingPass(PassRegistry &);

//----------------------------------------------------------------------
// The GenXGroupBaling analysis pass
// (used for the second baling just before GenXLiveRanges)
class GenXGroupBaling : public FGPassImplInterface,
                        public IDMixin<GenXGroupBaling>,
                        public GenXBaling {
public:
  explicit GenXGroupBaling(BalingKind Kind = BalingKind::BK_Legalization,
                           const GenXSubtarget *ST = nullptr)
      : GenXBaling(Kind, ST) {}
  static StringRef getPassName() {
    return "GenX instruction baling analysis for a function group";
  }
  void releaseMemory() override { clear(); }
  static void getAnalysisUsage(AnalysisUsage &AU);
  bool runOnFunctionGroup(FunctionGroup &FG) override;
  // processFunctionGroup : process all the Functions in a FunctionGroup
  bool processFunctionGroup(FunctionGroup *FG);
};
void initializeGenXGroupBalingWrapperPass(PassRegistry &);
using GenXGroupBalingWrapper = FunctionGroupWrapperPass<GenXGroupBaling>;

} // end namespace llvm

template <> struct std::hash<llvm::genx::Bale> {
  std::size_t operator()(const llvm::genx::Bale &B) const { return B.getHash(); }
};

#endif // GENXBALING_H
