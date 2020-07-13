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
/// GenXCategory
/// ------------
///
/// This pass performs five functions:
///
/// 1. It splits any struct phi into a phi for each element of the struct. This
///    is done in GenXLowering, but a subsequent pass can re-insert a struct phi so
///    this pass mops those up.
///
/// 2. It resolves each overlapping circular phi value.
///
///    LLVM IR does not attach
///    any importance to the order of phi nodes in any particular basic block.
///    At the head of a loop, a phi incoming can also be a phi definition in the
///    same block, and they could be in either order.
///
///    However, once we start constructing live ranges in the GenX backend, we
///    attach importance to the order of the phi nodes, so we need to resolve
///    any such overlapping circular phi value. Currently we do this by
///    inserting a copy (actually a bitcast) just after the phi nodes in that
///    basic block. A future enhancement would be to try and re-order the phi
///    nodes, and only fall back to copy insertion if there is circularity and
///    it is impossible to find a correct order, for example when the loop body
///    swaps two variables over.
///
/// 3. It inserts a load for any operand that is constant but not allowed to be.
///    It also catches any case where constant propagation in EarlyCSE has
///    caused a non-simple constant to be propagated into the instruction.
///    See the GenXConstants section above.
//     (in GenXConstants.cpp)
///
/// 4. It determines the register category and increased alignment requirement
///    (e.g. use as a raw operand) of each value, and stores it by creating a
///    LiveRange for the value and storing it there. At this stage the LiveRange
///    does not contain any other information; GenXLiveRanges populates it further
///    (or erases it if the value turns out to be baled in).
///
/// 5. It inserts instructions as required to convert from one register
///    category to another, where a value has its def and uses not all requiring
///    the same category.
///
/// All this pass inserts is a llvm.genx.convert intrinsic. It does not record
/// what the categories are. This information is recalculated in GenXLiveness.
///
/// The reason for inserting the convert intrinsic calls here, before the final
/// run of GenXBaling before GenXLiveRanges, is that we want GenXBaling to spot
/// when a convert intrinsic can be baled with rdregion or wrregion.
///
/// For one value (function argument or instruction), the pass looks at the
/// categories required for the defintion and each use. If there is no address
/// conversion involved, then it inserts a single conversion if possible (all
/// uses are the same category), otherwise it inserts a conversion for each use
/// that requires one.
///
/// **IR restriction**: After this pass, a value must have its def and all uses
/// requiring the same register category.
///
/// Address conversion
/// ^^^^^^^^^^^^^^^^^^
///
/// An address conversion is treated slightly differently.
///
/// A rdregion/wrregion representing an indirect region has a variable index.
/// This index is actually an index, whereas the vISA we need to generate for
/// it uses an address register that has been set up with an ``add_addr``
/// instruction from the index and the base register.
///
/// This pass inserts an ``llvm.genx.convert.addr`` intrinsic, with zero offset,
/// to represent the conversion from index to address register. However, the
/// intrinsic has no way of representing the base register.  Instead, the base
/// register is implicitly the "old value" input of the rdregion/wrregion where
/// the address is used.
///
/// The same index may well be used in multiple rdregions and wrregions,
/// especially after LLVM's CSE. But at this stage we have no idea whether
/// these multiple rdregions/wrregions will have the same base register, so
/// we must assume not and insert a separate ``llvm.genx.convert.addr``
/// for each rdregion/wrregion use of the index.
///
/// These multiple address conversions of the same index are commoned up
/// where possible later on in GenXAddressCommoning. That pass runs after
/// GenXCoalescing, so it can tell whether two address conversions of the
/// same index also have the same base register because the "old value"
/// inputs of the regions have been coalesced together.
///
/// Where an index used in an indirect region is a constant add, this pass
/// inserts the ``llvm.genx.convert.addr`` before that, and turns the constant
/// add into ``llvm.genx.add.addr``. The latter can be baled into rdregion
/// or wrregion, representing a constant offset in the indirect region.
/// Only one ``llvm.genx.add.addr`` is allowed between the
/// ``llvm.genx.convert.addr`` and the use in a rdregion/wrregion.
///
/// However this pass does not check whether the offset is in range (although
/// GenXBaling does check that before deciding to bale it in). The
/// GenXAddressCommoning pass sorts that out.
///
/// **IR restriction**: After this pass, a variable index in a rdregion/wrregion
/// must be the result of ``llvm.genx.convert.addr`` or ``llvm.genx.add.addr``.
/// Operand 0 of ``llvm.genx.add.addr`` must be the result of
/// ``llvm.genx.convert.addr``.
///
/// **IR restriction**: After this pass, up to GenXAddressCommoning, the result
/// of ``llvm.genx.convert.addr`` must have a single use in either a
/// ``llvm.genx.add.addr`` or as the index in rdregion/wrregion. The result
/// of ``llvm.genx.add.addr`` must have a single use as the index in
/// rdregion/wrregion.
///
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "GENX_CATEGORY"

#include "FunctionGroup.h"
#include "GenX.h"
#include "GenXConstants.h"
#include "GenXIntrinsics.h"
#include "GenXLiveness.h"
#include "GenXModule.h"
#include "GenXRegion.h"
#include "GenXUtil.h"
#include "vc/GenXOpts/Utils/KernelInfo.h"
#include "vc/GenXOpts/Utils/RegCategory.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/GenXIntrinsics/GenXIntrinsics.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Metadata.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/Support/Debug.h"
#include "llvmWrapper/IR/InstrTypes.h"

using namespace llvm;
using namespace genx;

namespace {

  // CategoryAndAlignment : values returned from getCategoryAndAlignment*
  // functions
  struct CategoryAndAlignment {
    unsigned Cat;
    unsigned Align;
    CategoryAndAlignment(unsigned Cat, unsigned Align = 0) : Cat(Cat), Align(Align) {}
  };

  class UsesCatInfo;

  // GenX category pass
  class GenXCategory : public FunctionGroupPass {
    Function *Func;
    KernelMetadata KM;
    GenXLiveness *Liveness;
    DominatorTreeGroupWrapperPass *DTs;
    SmallVector<Instruction *, 8> ToErase;
    bool Modified;
    // Vector of arguments and phi nodes that did not get a category.
    SmallVector<Value *, 8> NoCategory;
    bool InFGHead;
  public:
    static char ID;
    explicit GenXCategory() : FunctionGroupPass(ID) { }
    virtual StringRef getPassName() const { return "GenX category conversion"; }
    void getAnalysisUsage(AnalysisUsage &AU) const;
    bool runOnFunctionGroup(FunctionGroup &FG);
    // createPrinterPass : get a pass to print the IR, together with the GenX
    // specific analyses
    virtual Pass *createPrinterPass(raw_ostream &O, const std::string &Banner) const
    { return createGenXGroupPrinterPass(O, Banner); }
    unsigned getCategoryForPhiIncomings(PHINode *Phi) const;
    unsigned getCategoryForCallArg(Function *Callee, unsigned ArgNo) const;
    unsigned getCategoryForInlasmConstraintedOp(CallInst *CI, unsigned ArgNo,
                                                bool IsOutput) const;
    CategoryAndAlignment getCategoryAndAlignmentForDef(Value *V) const;
    CategoryAndAlignment getCategoryAndAlignmentForUse(Value::use_iterator U) const;
  private:
    const GenXSubtarget *Subtarget;
    using ConvListT = std::array<llvm::Instruction *, RegCategory::NUMCATEGORIES>;

    bool processFunction(Function *F);
    bool fixCircularPhis(Function *F);
    bool processValue(Value *V);
    Instruction *createConversion(Value *V, unsigned Cat);
    ConvListT buildConversions(Value *Def, CategoryAndAlignment DefInfo, const UsesCatInfo &UsesInfo);
  };

  // AUse : an address use of a value in processValue()
  struct AUse {
    Instruction *user;
    unsigned OperandNum;
    unsigned Cat;
    AUse(Value::use_iterator U, unsigned Cat)
      : user(cast<Instruction>(U->getUser())),
        OperandNum(U->getOperandNo()), Cat(Cat) {}
  };

  // almost real input iterator, minimum for range for was implemented
  class Iterator final {
    unsigned ShiftedMask_;
    unsigned CurCat_;

  public:
    Iterator(unsigned Mask, unsigned Cat) : ShiftedMask_(Mask), CurCat_(Cat) {
      validate();
    }

    unsigned operator*() const {
      validate();
      return CurCat_;
    }

    Iterator &operator++() {
      validate();
      ShiftedMask_ /= 2;
      ++CurCat_;
      if (ShiftedMask_ == 0) {
        CurCat_ = RegCategory::NUMCATEGORIES;
        validate();
        return *this;
      }
      for (; ShiftedMask_ % 2 == 0; ShiftedMask_ /= 2, ++CurCat_)
        ;
      validate();
      return *this;
    }

    friend bool operator==(const Iterator &lhs, const Iterator &rhs) {
      return (lhs.ShiftedMask_ == rhs.ShiftedMask_ &&
              lhs.CurCat_ == rhs.CurCat_);
    }

    friend bool operator!=(const Iterator &lhs, const Iterator &rhs) {
      return !(lhs == rhs);
    }

  private:
    void validate() const {
      assert((ShiftedMask_ % 2 == 1 || CurCat_ == RegCategory::NUMCATEGORIES) &&
             "invalid state");
    }
  };

  // Implements only begin() and end()
  // to iterate over categories of uses.
  class Categories final {
    unsigned Mask_;

  public:
    explicit Categories(unsigned Mask) : Mask_(Mask) {}

    Iterator begin() const {
      // we have no category
      if (!Mask_)
        return end();
      // we have NONE category
      if (Mask_ % 2 == 1)
        return Iterator(Mask_, 0);
      // we adding NONE category
      Iterator FalseBegin(Mask_ + 1, 0);
      // and now we get the real first category
      return ++FalseBegin;
    }

    Iterator end() const { return Iterator(0, RegCategory::NUMCATEGORIES); }
  };

  // Encapsulates Category'n'Alignment analysis of value uses.
  class UsesCatInfo final {
    using UsesT = llvm::SmallVector<AUse, 8>;
    UsesT Uses_;
    unsigned Mask_;
    unsigned MaxAlign_;
    unsigned MostUsedCat_;

  public:
    UsesCatInfo() : Uses_(), Mask_(0), MaxAlign_(0) {}

    UsesCatInfo(const GenXCategory &PassInfo, Value *V) : UsesCatInfo() {
      std::array<int, RegCategory::NUMCATEGORIES> Stat = {0};
      for (auto ui = V->use_begin(), ue = V->use_end(); ui != ue; ++ui) {
        auto CatAlign = PassInfo.getCategoryAndAlignmentForUse(ui);
        MaxAlign_ = std::max(MaxAlign_, CatAlign.Align);
        Uses_.push_back(AUse(ui, CatAlign.Cat));
        Mask_ |= 1 << CatAlign.Cat;
        if (CatAlign.Cat != RegCategory::NONE)
          ++Stat[CatAlign.Cat];
      }
      auto MaxInStatIt = std::max_element(Stat.begin(), Stat.end());
      MostUsedCat_ = MaxInStatIt - Stat.begin();
    }

    bool empty() const { return !Mask_; }

    bool allHaveCat(unsigned cat) const { return !(Mask_ & ~(1 << cat)); }

    const UsesT &getUses() const { return Uses_; }

    unsigned getMaxAlign() const { return MaxAlign_; }

    // When there's no real category uses (real is anything but NONE)
    // behavior is undefined.
    unsigned getMostUsedCat() const {
      assert(!empty() && !allHaveCat(RegCategory::NONE) &&
             "works only for cases when there are uses with real categories");
      return MostUsedCat_;
    }

    // meant to be used in range for
    Categories getCategories() const { return Categories(Mask_); }
  };

  void placeConvAfterDef(Function *Func, Instruction *Conv, Value *Def) {
    if (Instruction *Inst = dyn_cast<Instruction>(Def)) {
      // Original value is an instruction. Insert just after it.
      Conv->insertAfter(Inst);
      Conv->setDebugLoc(Inst->getDebugLoc());
    } else {
      assert(isa<Argument>(Def) && "must be an argument if not an instruction");
      // Original value is a function argument. Insert at the start of the
      // function.
      Conv->insertBefore(&*Func->begin()->begin());
    }
  }

  void placeConvBeforeUse(Instruction *Conv, Instruction *Use,
                          unsigned UseOperand) {
    if (auto PhiUse = dyn_cast<PHINode>(Use)) {
      // Use is in a phi node. Insert before terminator in corresponding
      // incoming block.
      Conv->insertBefore(PhiUse->getIncomingBlock(UseOperand)->getTerminator());
    } else {
      // Insert just before use.
      Conv->insertBefore(Use);
      Conv->setDebugLoc(Use->getDebugLoc());
    }
  }

  } // end anonymous namespace

char GenXCategory::ID = 0;
namespace llvm { void initializeGenXCategoryPass(PassRegistry &); }
INITIALIZE_PASS_BEGIN(GenXCategory, "GenXCategory", "GenXCategory", false, false)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeGroupWrapperPass)
INITIALIZE_PASS_DEPENDENCY(GenXLiveness)
INITIALIZE_PASS_END(GenXCategory, "GenXCategory", "GenXCategory", false, false)

FunctionGroupPass *llvm::createGenXCategoryPass()
{
  initializeGenXCategoryPass(*PassRegistry::getPassRegistry());
  return new GenXCategory();
}

void GenXCategory::getAnalysisUsage(AnalysisUsage &AU) const
{
  FunctionGroupPass::getAnalysisUsage(AU);
  AU.addRequired<DominatorTreeGroupWrapperPass>();
  AU.addRequired<GenXLiveness>();
  AU.addPreserved<GenXModule>();
  AU.addPreserved<GenXLiveness>();
  AU.addPreserved<FunctionGroupAnalysis>();
  AU.addPreserved<DominatorTreeGroupWrapperPass>();
  AU.setPreservesCFG();
}

/***********************************************************************
 * runOnFunctionGroup : run the category conversion pass for
 *      this FunctionGroup
 */
bool GenXCategory::runOnFunctionGroup(FunctionGroup &FG)
{
  KM = KernelMetadata(FG.getHead());
  DTs = &getAnalysis<DominatorTreeGroupWrapperPass>();
  Liveness = &getAnalysis<GenXLiveness>();
  auto P = getAnalysisIfAvailable<GenXSubtargetPass>();
  Subtarget = P ? P->getSubtarget() : nullptr;
  bool Modified = false;
  if (KM.isKernel()) {
    // Get the offset of each kernel arg.
    for (auto ai = FG.getHead()->arg_begin(), ae = FG.getHead()->arg_end();
        ai != ae; ++ai) {
      Argument *Arg = &*ai;
      Liveness->getOrCreateLiveRange(Arg)->Offset = KM.getArgOffset(Arg->getArgNo());
    }
  }
  // Mop up any struct phis, splitting into elements.
  for (auto i = FG.begin(), e = FG.end(); i != e; ++i)
    Modified |= splitStructPhis(*i);
  // Do category conversion on each function in the group.
  InFGHead = true;
  for (auto i = FG.begin(), e = FG.end(); i != e; ++i) {
    Modified |= processFunction(*i);
    InFGHead = false;
  }
  // Now iteratively process values that did not get a category. A valid
  // category will eventually propagate through a web of phi nodes
  // and/or subroutine args.
  while (NoCategory.size()) {
    SmallVector<Value *, 8> NoCategory2;
    for (unsigned i = 0, e = NoCategory.size(); i != e; ++i) {
      if (!processValue(NoCategory[i]))
        NoCategory2.push_back(NoCategory[i]);
    }
    assert(NoCategory2.size() < NoCategory.size() && "not making any progess");
    NoCategory.clear();
    if (!NoCategory2.size())
      break;
    for (unsigned i = 0, e = NoCategory2.size(); i != e; ++i) {
      if (!processValue(NoCategory2[i]))
        NoCategory.push_back(NoCategory2[i]);
    }
    Modified |= true;
  }
  return Modified;
}

// Common up constpred calls within a block.
static bool commonUpPredicate(BasicBlock *BB) {
  bool Changed = false;
  // Map from flatten predicate value to its constpred calls.
  SmallDenseMap<uint64_t, SmallVector<Instruction *, 8>> ValMap;

  for (auto &Inst : BB->getInstList()) {
    if (GenXIntrinsic::getGenXIntrinsicID(&Inst) == GenXIntrinsic::genx_constantpred) {
      Constant *V = cast<Constant>(Inst.getOperand(0));
      if (auto VT = dyn_cast<VectorType>(V->getType())) {
        unsigned NElts = VT->getVectorNumElements();
        if (NElts > 64)
          continue;
        uint64_t Bits = 0;
        for (unsigned i = 0; i != NElts; ++i)
          if (!V->getAggregateElement(i)->isNullValue())
            Bits |= ((uint64_t)1 << i);
        auto Iter = ValMap.find(Bits);
        if (Iter == ValMap.end())
          ValMap[Bits].push_back(&Inst);
        else if (Inst.hasOneUse() && Inst.user_back()->getParent() == BB)
          // Just in case constpred is not from constant predicate loading. This
          // ensures the first instruction dominates others in the same vector.
          (Iter->second).push_back(&Inst);
      }
    }
  }

  // Common up when there are more than 2 uses, in which case it will not be
  // worse than flag spills.
  for (auto I = ValMap.begin(), E = ValMap.end(); I != E; ++I) {
    auto &V = I->second;
    int n = (int)V.size();
    if (n > 2) {
      Instruction *DomInst = V.front();
      for (int i = 1; i < n; ++i) {
        V[i]->replaceAllUsesWith(DomInst);
        V[i]->eraseFromParent();
      }
      Changed = true;
    }
  }

  return Changed;
}

/***********************************************************************
 * processFunction : run the category conversion pass for this Function
 *
 * This does a postordered depth first traversal of the CFG,
 * processing instructions within a basic block in reverse, to
 * ensure that we see a def after its uses (ignoring phi node uses).
 * This is specifically useful for an address conversion, where we want to
 * see the constant add used in an indirect region (and convert it into a
 * llvm.genx.add.addr) before we see the instruction it uses.
 */
bool GenXCategory::processFunction(Function *F)
{
  Func = F;
  // Before doing the category conversion, fix circular phis.
  Modified = fixCircularPhis(F);
  // Load constants in phi nodes.
  loadPhiConstants(F, DTs->getDomTree(F), false, Subtarget);
  // Process all instructions.
  for (po_iterator<BasicBlock *> i = po_begin(&Func->getEntryBlock()),
      e = po_end(&Func->getEntryBlock()); i != e; ++i) {
    // This loop scans the basic block backwards. If any code is inserted
    // before the current point, that code is scanned too.
    BasicBlock *BB = *i;
    for (Instruction *Inst = &BB->back(); Inst;
        Inst = (Inst == &BB->front() ? nullptr : Inst->getPrevNode())) {
      Modified |= loadNonSimpleConstants(Inst, nullptr, Subtarget);
      Modified |= loadConstants(Inst, Subtarget);
      if (!processValue(Inst))
        NoCategory.push_back(Inst);
    }

    // This commons up constpred calls just loaded.
    Modified |= commonUpPredicate(BB);

    // Erase instructions (and their live ranges) as requested by processValue.
    for (unsigned i = 0, e = ToErase.size(); i != e; ++i) {
      Liveness->eraseLiveRange(ToErase[i]);
      ToErase[i]->eraseFromParent();
    }
    ToErase.clear();
  }
  // Process all args.
  for (auto fi = Func->arg_begin(), fe = Func->arg_end(); fi != fe; ++fi) {
    Value *V = &*fi;
    if (!processValue(V))
      NoCategory.push_back(V);
  }
  return Modified;
}

/***********************************************************************
 * fixCircularPhis : fix up overlapping circular phi nodes
 *
 * A phi node at the head of a loop can have a use in the phi nodes in the same
 * basic block. If the use is after the def, it still refers to the value in
 * the previous loop iteration, but the GenX backend cannot cope with the
 * live range going round the loop and overlapping with its own start.
 *
 * This function spots any such phi node and works around it by inserting an
 * extra copy (bitcast) just after the phi nodes in the basic block.
 *
 * A better solution for the future would be to re-order the phi nodes if
 * possible, and only fall back to inserting a copy if there is circularity
 * (e.g. a loop that swaps two variables in its body).
 */
bool GenXCategory::fixCircularPhis(Function *F)
{
  bool Modified = false;
  for (auto fi = Func->begin(), fe = Func->end(); fi != fe; ++fi) {
    BasicBlock *BB = &*fi;
    // Process phi nodes in one basic block.
    for (auto bi = BB->begin(); ; ++bi) {
      auto Phi = dyn_cast<PHINode>(&*bi);
      if (!Phi)
        break; // end of phi nodes
      if (!GenXLiveness::wrapsAround(Phi, Phi))
        continue;
      // Overlapping circular phi node. Insert a copy.
      // Note that the copy has to be split in the same way as a copy
      // inserted in GenXCoalescing when coalescing fails, but we have
      // our own code here because at this point we do not have any real
      // and possibly coalesced live ranges like GenXCoalescing does.
      Modified = true;
      SmallVector<Use *, 8> Uses;
      for (auto ui = Phi->use_begin(), ue = Phi->use_end(); ui != ue; ++ui)
        Uses.push_back(&*ui);
      // A phi node is never a struct -- GenXLowering removed struct phis.
      assert(!isa<StructType>(Phi->getType()));
      // Insert a copy, split as required to be legal.
      auto NewCopy =
          Liveness->insertCopy(Phi, nullptr, BB->getFirstNonPHI(),
                               Phi->getName() + ".unoverlapper", 0, Subtarget);
      // Change the uses that existed before we added the copy to use the
      // copy instead.
      for (auto ui = Uses.begin(), ue = Uses.end(); ui != ue; ++ui)
        **ui = NewCopy;
    }
  }
  return Modified;
}

/***********************************************************************
 * processValue : category conversion for one value
 *
 * Return:  whether category successfully chosen
 *
 * This returns false only for a function argument or a phi node where all
 * uses are in phi nodes which themselves do not have a category yet.
 */
bool GenXCategory::processValue(Value *V)
{
  // Check for special cases.
  // Ignore void.
  if (V->getType()->isVoidTy())
    return true;
  // Ignore i1 or vector of i1. Predicates do not use category
  // conversion.
  if (V->getType()->getScalarType()->isIntegerTy(1))
    return true;
  // Elements of a struct always have default (general or predicate) category.
  if (isa<StructType>(V->getType()))
    return true;

  auto DefInfo = getCategoryAndAlignmentForDef(V);
  UsesCatInfo UsesInfo(*this, V);

  // more corner cases
  if (UsesInfo.empty()) {
    // Value not used: set its category and then ignore it. If the definition
    // did not give us a category (probably an unused function arg), then
    // arbitrarily make it general.
    if (DefInfo.Cat == RegCategory::NONE)
      Liveness->getOrCreateLiveRange(V, RegCategory::GENERAL, DefInfo.Align);
    else
      Liveness->getOrCreateLiveRange(V, DefInfo.Cat, DefInfo.Align);
    return true;
  }
  else if (UsesInfo.allHaveCat(RegCategory::NONE))
  {
    if (DefInfo.Cat == RegCategory::NONE) {
      // The "no categories at all" case can only happen for a value that is
      // defined by a function argument or a phi node and used only in phi
      // nodes or subroutine call args.
      assert((isa<Argument>(V) || isa<PHINode>(V)) && "no register category");
      return false;
    }
    // Value defined with a category but only used in phi nodes.
    Liveness->getOrCreateLiveRange(V, DefInfo.Cat, DefInfo.Align);
    return true;
  }

  // main case
  if (DefInfo.Cat == RegCategory::NONE) {
    // NONE means that we're free to choose the category
    if (isa<PHINode>(V))
      // currently we'd like to propogate general through phi
      DefInfo.Cat = RegCategory::GENERAL;
    else
      DefInfo.Cat = UsesInfo.getMostUsedCat();
  }

  Liveness->getOrCreateLiveRange(V, DefInfo.Cat, std::max(DefInfo.Align, UsesInfo.getMaxAlign()));
  auto Convs = buildConversions(V, DefInfo, UsesInfo);
  for (auto UseInfo : UsesInfo.getUses()) {
    if (UseInfo.Cat != DefInfo.Cat && UseInfo.Cat != RegCategory::NONE) {
      Instruction *Conv;
      if (UseInfo.Cat == RegCategory::ADDRESS) {
        // Case of address category requires a separate conversion for each use, at least until we
        // get to GenXAddressCommoning where we decide whether we can common some of them up.
        Conv = createConversion(V, UseInfo.Cat);
        placeConvBeforeUse(Conv, UseInfo.user, UseInfo.OperandNum);
        Liveness->getOrCreateLiveRange(Conv)->setCategory(UseInfo.Cat);
      }
      else
        Conv = Convs[UseInfo.Cat];
      assert(Conv && "must have such conversion");
      UseInfo.user->setOperand(UseInfo.OperandNum, Conv);
    }
  }
  // If V is now unused (which happens if it is a constant add and all its
  // uses were addresses), then remove it.
  if (V->use_empty())
    ToErase.push_back(cast<Instruction>(V));
  return true;
}

/***********************************************************************
 * createConversion : create call to llvm.genx.convert intrinsic to represent
 *                    register category conversion
 *
 * The new instruction is not inserted anywhere yet.
 *
 * In the case that we are asked to convert a use of an add or constant sub
 * to an address, we instead create an llvm.genx.add.addr of the input
 * to the add/sub.
 */
Instruction *GenXCategory::createConversion(Value *V, unsigned Cat)
{
  assert(V->getType()->getScalarType()->isIntegerTy() && "createConversion expects int type");
  if (Cat == RegCategory::ADDRESS) {
    Value *Input = V;
    int Offset = 0;
    for (;;) {
      // Check for use of add/sub that can be baled in to a region as a
      // constant offset. This also handles a chain of two or more adds.
      int ThisOffset;
      if (!GenXBaling::getIndexAdd(Input, &ThisOffset) &&
          !GenXBaling::getIndexOr(Input, ThisOffset))
        break;
      if (ThisOffset < G4_MIN_ADDR_IMM)
        break;
      Offset += ThisOffset;
      Input = cast<Instruction>(Input)->getOperand(0);
    }
    if (Input != V) {
      // Turn the add/sub into llvm.genx.add.addr. This could be out of range as
      // a constant offset in an indirect operand at this stage;
      // GenXAddressCommoning sorts that out by adjusting the constant offset in
      // the llvm.genx.convert.addr.
      return createAddAddr(Input, ConstantInt::get(V->getType(), Offset),
          V->getName() + ".addradd", nullptr, Func->getParent());
    }
  }
  // Normal conversion. If the source is an integer creation intrinsic
  // and this isn't an address conversion, use the operand for that
  // intrinsic call directly rather than using the result of the intrinsic.
  // This helps the jitter to generate better code when surface constants
  // are used in send intructions.
  if (Cat != RegCategory::ADDRESS) {
    if (GenXIntrinsic::getGenXIntrinsicID(V) == GenXIntrinsic::genx_constanti)
      V = cast<CallInst>(V)->getArgOperand(0);
    return createConvert(V, V->getName() + ".categoryconv", nullptr,
        Func->getParent());
  }
  return createConvertAddr(V, 0, V->getName() + ".categoryconv", nullptr,
      Func->getParent());
}

/***********************************************************************
 * Creates conversion instructions, places them in the function (next to the
 * def)
 *
 * Returns an array of created conversion (cons[Category] holds
 * instruction if we need conversion to such Category and nullptr otherwise).
 * Doesn't produce address category conversion.
 */
GenXCategory::ConvListT
GenXCategory::buildConversions(Value *Def, CategoryAndAlignment DefInfo,
                               const UsesCatInfo &UsesInfo) {
  ConvListT Convs = {nullptr};
  for (auto Cat : UsesInfo.getCategories()) {
    // NONE doesn't require conversion, ADDRESS requirs conversion before
    // every use (not after def, so we won't create it here)
    if (Cat != DefInfo.Cat && Cat != RegCategory::NONE &&
        Cat != RegCategory::ADDRESS) {
      auto Conv = createConversion(Def, Cat);
      placeConvAfterDef(Func, Conv, Def);
      Liveness->getOrCreateLiveRange(Conv)->setCategory(Cat);
      Convs[Cat] = Conv;
    }
  }
  return Convs;
}

/***********************************************************************
 * intrinsicCategoryToRegCategory : convert intrinsic arg category to
 *      register category
 *
 * This converts a GenXIntrinsicInfo::* category, as returned by
 * GenXIntrinsicInfo::ArgInfo::getCategory(), into a register category
 * as stored in a live range.
 */
static unsigned intrinsicCategoryToRegCategory(unsigned ICat)
{
  switch (ICat) {
    case GenXIntrinsicInfo::ADDRESS:
      return RegCategory::ADDRESS;
    case GenXIntrinsicInfo::PREDICATION:
    case GenXIntrinsicInfo::PREDICATE:
      return RegCategory::PREDICATE;
    case GenXIntrinsicInfo::SAMPLER:
      return RegCategory::SAMPLER;
    case GenXIntrinsicInfo::SURFACE:
      return RegCategory::SURFACE;
    case GenXIntrinsicInfo::VME:
      return RegCategory::VME;
    default:
      return RegCategory::GENERAL;
  }
}

/***********************************************************************
 * getCategoryAndAlignmentForDef : get register category and alignment for a def
 *
 * This returns RegCategory:: value, or RegCategory::NONE if no category
 * is discernable.
 */
CategoryAndAlignment GenXCategory::getCategoryAndAlignmentForDef(Value *V) const
{
  if (V->getType()->getScalarType()->getPrimitiveSizeInBits() == 1)
    return RegCategory::PREDICATE;
  if (Argument *Arg = dyn_cast<Argument>(V)) {
    // This is a function Argument.
    if (!InFGHead) {
      // It is an arg in a subroutine.  Get the category from the corresponding
      // arg at some call site.  (We should not have disagreement among the
      // call sites and the function arg, since whichever one gets a category
      // first forces the category of all the others.)
      return getCategoryForCallArg(Arg->getParent(), Arg->getArgNo());
    }
    unsigned ArgNo = Arg->getArgNo();
    if (KM.getNumArgs() > ArgNo) {
      // The function is a kernel, and has argument kind metadata for
      // this argument. Determine the category from the kind.
      return KM.getArgCategory(ArgNo);
    }
    // The function is not a kernel, or does not have the appropriate
    // metadata. Set to no particular category, so the arg's uses will
    // determine the category. This is the fallback for compatibility with
    // hand coded LLVM IR from before this metadata was added. (If we only
    // had to cope with non-kernel functions, we could just return GENERAL.)
    return RegCategory::NONE;
  }
  // The def is a phi-instruction.
  if (PHINode *Phi = dyn_cast<PHINode>(V)) {
    // This is a phi node. Get the category from one of the incomings. (We
    // should not have disagreement among the incomings, since whichever
    // one gets a category first forces the category of all the others.)
    return getCategoryForPhiIncomings(Phi);
  }
  // Multiple outputs of inline assembly instruction
  // result in a structure and those elements are extracted
  // with extractelement
  if (ExtractValueInst *Extract = dyn_cast<ExtractValueInst>(V)) {
    auto CI = dyn_cast<CallInst>(Extract->getAggregateOperand());
    if (CI && CI->isInlineAsm())
      return getCategoryForInlasmConstraintedOp(CI, Extract->getIndices()[0],
                                                true /*IsOutput*/);
  }
  // The def is a call-inst
  if (CallInst *CI = dyn_cast<CallInst>(V)) {
    if (Function *Callee = CI->getCalledFunction()) {
      unsigned IntrinsicID = GenXIntrinsic::getAnyIntrinsicID(Callee);
      // We should not see genx_convert, as it is inserted into a value after
      // using this function to determine its category.
      assert(IntrinsicID != GenXIntrinsic::genx_convert);
      if (IntrinsicID == GenXIntrinsic::genx_convert_addr)
        return RegCategory::ADDRESS;
      if (GenXIntrinsic::isAnyNonTrivialIntrinsic(IntrinsicID) && !GenXIntrinsic::isRdRegion(IntrinsicID)
          && !GenXIntrinsic::isWrRegion(IntrinsicID) && !GenXIntrinsic::isAbs(IntrinsicID)) {
        // For any normal intrinsic, look up the argument class.
        GenXIntrinsicInfo II(IntrinsicID);
        auto AI = II.getRetInfo();
        return CategoryAndAlignment(
            intrinsicCategoryToRegCategory(AI.getCategory()),
            getLogAlignment(AI.getAlignment(), Subtarget
                                                   ? Subtarget->getGRFWidth()
                                                   : defaultGRFWidth));
      } else if (GenXIntrinsic::isRdRegion(IntrinsicID)) {
        // Add this to avoid conversion in case of read-region on SurfaceIndex
        // or SamplerIndex type
        auto RC = getCategoryAndAlignmentForDef(
            CI->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum));
        if (RC.Cat == RegCategory::SURFACE ||
            RC.Cat == RegCategory::SAMPLER)
          return RC.Cat;
      }
    } else if (CI->isInlineAsm()) {
      return getCategoryForInlasmConstraintedOp(CI, 0, true /*IsOutput*/);
    }
  }
  return RegCategory::GENERAL;
}

/***********************************************************************
 * getCategoryForInlasmConstraintedOp : get register category for a
 *                            operand of inline assembly (both for
 *                            output and for input). Category of
 *                            operand depends on its constraint.
 *
 */
unsigned GenXCategory::getCategoryForInlasmConstraintedOp(CallInst *CI,
                                                          unsigned ArgNo,
                                                          bool IsOutput) const {
  assert(CI->isInlineAsm() && "Inline asm expected");
  InlineAsm *IA = dyn_cast<InlineAsm>(CI->getCalledValue());
  assert(!IA->getConstraintString().empty() && "Here should be constraints");

  auto ConstraintsInfo = genx::getGenXInlineAsmInfo(CI);

  if (!IsOutput)
    ArgNo += genx::getInlineAsmNumOutputs(CI);
  auto Info = ConstraintsInfo[ArgNo];

  switch (Info.getConstraintType()) {
  default:
    llvm_unreachable("unreachable while setting category in constraints");
  case ConstraintType::Constraint_a:
  case ConstraintType::Constraint_rw:
  case ConstraintType::Constraint_r:
    return RegCategory::GENERAL;
  case ConstraintType::Constraint_n:
  case ConstraintType::Constraint_i:
  case ConstraintType::Constraint_F:
    return RegCategory::NONE;
  case ConstraintType::Constraint_cr:
    return RegCategory::PREDICATE;
  }
}

/***********************************************************************
 * getCategoryAndAlignmentForUse : get register category for a use
 *
 * This returns RegCategory:: value, or RegCategory::NONE if no category
 * is discernable.
 */
CategoryAndAlignment GenXCategory::getCategoryAndAlignmentForUse(
      Value::use_iterator U) const
{
  Value *V = U->get();
  if (V->getType()->getScalarType()->isIntegerTy(1))
    return RegCategory::PREDICATE;
  auto user = cast<Instruction>(U->getUser());
  if (PHINode *Phi = dyn_cast<PHINode>(user)) {
    // This is a phi node. Get the category (if any) from the result, or from
    // one of the incomings. (We should not have disagreement among the
    // incomings, since whichever one gets a category first forces the category
    // of all the others.)
    if (auto LR = Liveness->getLiveRangeOrNull(Phi)) {
      auto Cat = LR->getCategory();
      if (Cat != RegCategory::NONE)
        return Cat;
    }
    return getCategoryForPhiIncomings(Phi);
  }
  unsigned Category = RegCategory::GENERAL;
  if (IGCLLVM::CallInst *CI = dyn_cast<IGCLLVM::CallInst>(user)) {
    if (CI->isInlineAsm())
      Category = getCategoryForInlasmConstraintedOp(CI, U->getOperandNo(),
                                                    false /*IsOutput*/);
    else if (CI->isIndirectCall())
      Category = RegCategory::GENERAL;
    else {
      Function *Callee = CI->getCalledFunction();
      unsigned IntrinID = GenXIntrinsic::not_any_intrinsic;
      if (Callee)
        IntrinID = GenXIntrinsic::getAnyIntrinsicID(Callee);
      // We should not see genx_convert, as it is inserted into a value after
      // using this function to determine its category.
      assert(IntrinID != GenXIntrinsic::genx_convert);
      // For a read or write region or element intrisic, where the use we have
      // is the address, mark as needing an address register.
      switch (IntrinID) {
        case GenXIntrinsic::not_any_intrinsic:
          // Arg in subroutine call. Get the category from the function arg,
          // or the arg at another call site. (We should not have disagreement
          // among the call sites and the function arg, since whichever one
          // gets a category first forces the category of all the others.)
          Category = getCategoryForCallArg(Callee, U->getOperandNo());
          break;
        case GenXIntrinsic::genx_convert_addr:
          Category = RegCategory::GENERAL;
          break;
        case GenXIntrinsic::genx_rdregioni:
        case GenXIntrinsic::genx_rdregionf:
          if (U->getOperandNo() == 4) // is addr-operand
            Category = RegCategory::ADDRESS;
          else if (GenXIntrinsic::GenXRegion::OldValueOperandNum == U->getOperandNo())
            Category = RegCategory::NONE; // do not assign use-category
          break;
        case GenXIntrinsic::genx_wrregioni:
        case GenXIntrinsic::genx_wrregionf:
          if (U->getOperandNo() == 5) // is addr-operand
            Category = RegCategory::ADDRESS;
           break;
        case GenXIntrinsic::genx_absf:
        case GenXIntrinsic::genx_absi:
        case GenXIntrinsic::genx_output:
          break;
        default: {
            // For any other intrinsic, look up the argument class.
            GenXIntrinsicInfo II(IntrinID);
            auto AI = II.getArgInfo(U->getOperandNo());
            return CategoryAndAlignment(
                intrinsicCategoryToRegCategory(AI.getCategory()),
                getLogAlignment(AI.getAlignment(),
                                Subtarget ? Subtarget->getGRFWidth()
                                          : defaultGRFWidth));
          }
          break;
          }
    }
  }
  return Category;
}

/***********************************************************************
 * getCategoryForPhiIncomings : get register category from phi incomings
 *
 * Return:  register category from a non-const incoming with a known category
 *          else NONE if at least one incoming is non-constant
 *          else GENERAL
 *
 * We will not have disagreement among the incomings, since whichever one gets
 * a category first forces the category of all the others.
 */
unsigned GenXCategory::getCategoryForPhiIncomings(PHINode *Phi) const
{
  bool AllConst = true;
  for (unsigned i = 0, e = Phi->getNumIncomingValues(); i != e; ++i) {
    Value *Incoming = Phi->getIncomingValue(i);
    if (!isa<Constant>(Incoming)) {
      AllConst = false;
      if (auto LR = Liveness->getLiveRangeOrNull(Incoming)) {
        unsigned Cat = LR->getCategory();
        if (Cat != RegCategory::NONE)
          return Cat;
      }
    }
  }
  if (AllConst) {
    // All incomings are constant. Arbitrarily make the phi node value
    // general category.
    return RegCategory::GENERAL;
  }
  // No incoming has a category yet.
  return RegCategory::NONE;
}

/***********************************************************************
 * getCategoryForCallArg : get register category from subroutine arg or
 *        the corresponding arg at some call site
 *
 * Enter:   Callee = function being called
 *          ArgNo = argument number
 *
 * Return:  register category from subroutine arg or a call arg with a
 *          known category, else NONE if no category found
 *
 * We will not have disagreement among the subroutine arg and its corresponding
 * call args, since whichever one gets a category first forces the category of
 * all the others.
 */
unsigned GenXCategory::getCategoryForCallArg(Function *Callee, unsigned ArgNo) const
{
  assert(Callee);
  // First try the subroutine arg.
  auto ai = Callee->arg_begin();
  for (unsigned i = 0; i != ArgNo; ++i, ++ai)
    ;
  if (auto LR = Liveness->getLiveRangeOrNull(&*ai)) {
    unsigned Cat = LR->getCategory();
    if (Cat != RegCategory::NONE)
      return Cat;
  }
  // Then try the arg at each call site.
  bool UseUndef = true;
  for (auto ui = Callee->use_begin(), ue = Callee->use_end(); ui != ue; ++ui) {
    if (auto CI = dyn_cast<CallInst>(ui->getUser())) {
      auto ArgV = CI->getArgOperand(ArgNo);
      if (!isa<UndefValue>(ArgV)) {
        UseUndef = false;
        if (auto LR = Liveness->getLiveRangeOrNull(ArgV)) {
          unsigned Cat = LR->getCategory();
          if (Cat != RegCategory::NONE)
            return Cat;
        }
      }
    }
  }
  // special case handling to break deadlock when all uses are undef,
  // force the argument to be GENERAL
  return(UseUndef ? RegCategory::GENERAL : RegCategory::NONE);
}

