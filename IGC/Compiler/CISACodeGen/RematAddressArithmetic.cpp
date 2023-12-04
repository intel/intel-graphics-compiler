/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/RematAddressArithmetic.h"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/ADT/BreadthFirstIterator.h"
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/IGCLivenessAnalysis.h"

using namespace llvm;
using namespace IGC;

static Value* getPrivateMemoryValue(Function& F);

namespace {

class RematAddressArithmetic : public FunctionPass {

public:
    static char ID;

    RematAddressArithmetic() : FunctionPass(ID)
    {
        initializeRematAddressArithmeticPass(*PassRegistry::getPassRegistry());
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
        AU.setPreservesCFG();
        AU.addRequired<PostDominatorTreeWrapperPass>();
    }

    bool runOnFunction(Function&) override;

private:
    bool rematerializePrivateMemoryAddressCalculation(Function& F);
    bool rematerializePhiMemoryAddressCalculation(Function& F);
    bool rematerialize(Instruction* I, SmallVectorImpl<Value*>& Chain);
};

class CloneAddressArithmetic : public FunctionPass {

public:
    static char ID;

    ~CloneAddressArithmetic() { Uses.clear(); }
    CloneAddressArithmetic() : FunctionPass(ID) {
        initializeCloneAddressArithmeticPass(*PassRegistry::getPassRegistry());
    }
    CloneAddressArithmetic(const CloneAddressArithmetic&) = delete; // Delete copy-constructor
    CloneAddressArithmetic& operator=(const CloneAddressArithmetic&) = delete; // Delete assignment operator

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
        AU.setPreservesCFG();
        AU.addRequired<IGCLivenessAnalysis>();
    }

    bool runOnFunction(Function&) override;
    void rematWholeChain(llvm::IntToPtrInst *I);
    std::unordered_map<llvm::Value*, unsigned int> Uses;

private:
    bool greedyRemat(Function &F);
};




} // end namespace


FunctionPass* IGC::createCloneAddressArithmeticPass() {
    return new CloneAddressArithmetic();
}

char CloneAddressArithmetic::ID = 0;

#define PASS_FLAG_2     "igc-clone-address-arithmetic"
#define PASS_DESC_2     "Clone Address Arithmetic"
#define PASS_CFG_ONLY_2 false
#define PASS_ANALYSIS_2 false
namespace IGC {
IGC_INITIALIZE_PASS_BEGIN(CloneAddressArithmetic, PASS_FLAG_2, PASS_DESC_2, PASS_CFG_ONLY_2, PASS_ANALYSIS_2)
IGC_INITIALIZE_PASS_DEPENDENCY(IGCLivenessAnalysis)
IGC_INITIALIZE_PASS_END(CloneAddressArithmetic, PASS_FLAG_2, PASS_DESC_2, PASS_CFG_ONLY_2, PASS_ANALYSIS_2)
}


static bool isAddressArithmetic(Instruction* I)
{
    if (isa<GetElementPtrInst>(I) ||
        isa<InsertElementInst>(I) ||
        isa<InsertValueInst>(I) ||
        (isa<UnaryInstruction>(I) && !isa<LoadInst>(I)) ||
        isa<BinaryOperator>(I))
        return true;

    return false;
}

void CloneAddressArithmetic::rematWholeChain(llvm::IntToPtrInst *I) {

  llvm::SmallVector<llvm::Instruction *, 4> RematVector;
  std::queue<llvm::Instruction *> BFSQ;
  BFSQ.push((Instruction *)I);

  const unsigned NumOfUsesLimit = IGC_GET_FLAG_VALUE(RematUsesThreshold);
  const unsigned RematChainLimit = IGC_GET_FLAG_VALUE(RematChainLimit);

  // we are traversing ssa-chain for address arithmetic
  while (!BFSQ.empty()) {

    llvm::Instruction *CurrI = BFSQ.front();
    BFSQ.pop();

    for (unsigned int i = 0; i < CurrI->getNumOperands(); ++i) {

      Instruction *Op = llvm::dyn_cast<Instruction>(CurrI->getOperand(i));
      if( Op != NULL) {

        bool NotPHI = !llvm::isa<llvm::PHINode>(Op);
        bool NotConstant = !llvm::isa<llvm::Constant>(Op);
        bool SameBB = IGC_IS_FLAG_ENABLED(RematSameBBScope) ? Op->getParent() == I->getParent() : true;
        bool AddressArithmetic = isAddressArithmetic(Op);

        // if operand has more uses than specified, we do not rematerialize it.
        // helps with situation like this:
        //
        // (we don't want to add this to every rematerialized chain of instructions)
        // someCommonValue = add base, 10000
        //
        // mul r0, someCommonValue
        // load r0
        // ...
        // mul r2 someCommonValue
        // load r2
        bool NotTooManyUses = Uses[Op] < NumOfUsesLimit;

        if (SameBB && NotConstant && NotPHI && NotTooManyUses && AddressArithmetic) {

          BFSQ.push(Op);
          RematVector.push_back(Op);
        }
      }
    }
  }

  // if the remat chain will be too long, probably we don't want that
  if(RematVector.size() > RematChainLimit) return;
  std::unordered_map<Instruction *, Instruction *> OldToNew;
  std::reverse(RematVector.begin(), RematVector.end());

  for (auto el : RematVector) {

    auto Clone = el->clone();
    OldToNew[el] = Clone;
    for (unsigned int i = 0; i < Clone->getNumOperands(); ++i) {

      auto OldOp = llvm::dyn_cast<Instruction>(Clone->getOperand(i));

      if (OldToNew.count(OldOp)) {
        Clone->setOperand(i, OldToNew[OldOp]);
      }
    }

    Clone->setName("remat");
    Clone->insertBefore(I);
  }

  auto OldOp = dyn_cast<Instruction>(I->getOperand(0));
  if (OldToNew.count(OldOp)) I->setOperand(0, OldToNew[OldOp]);

  OldToNew.clear();
  RematVector.clear();
}

bool CloneAddressArithmetic::greedyRemat(Function &F) {

  bool Result = false;

  auto RPE = &getAnalysis<IGCLivenessAnalysis>();
  unsigned int SIMD = numLanes(RPE->bestGuessSIMDSize());
  unsigned int PressureLimit = IGC_GET_FLAG_VALUE(RematRPELimit);
  if(RPE->getMaxRegCountForFunction(F, SIMD) < PressureLimit)
      return Result;

  for (BasicBlock &BB : F) {
    for (auto &I : BB) { Uses[&I] = I.getNumUses(); }
  }

  llvm::SmallVector<llvm::IntToPtrInst *, 4> ToProcess;

  // go through block, collect all inttoptr instructions to do
  // remat on them
  for (BasicBlock &BB : F) {
    // if block has less than required amount of LLVM IR instructions, skip it
    const unsigned Limit = IGC_GET_FLAG_VALUE(RematBlockSize);
    if (BB.getInstList().size() < Limit) continue;

    for (auto &I : BB) {

      auto *CastedIntToPtrInst = llvm::dyn_cast<IntToPtrInst>(&I);
      if (CastedIntToPtrInst) ToProcess.push_back(CastedIntToPtrInst);
    }
  }

  for (auto el : ToProcess) {

    Value *V = el;
    llvm::SmallVector<llvm::Use*, 4> VectorOfUses;
    // collect all uses of particular intoptr inst
    bool usedOnlyInLoadOrStore = true;
    for (auto &use : V->uses()) {

      // check that this inttoptr instruction only used in load or stores
      auto LI = llvm::dyn_cast<LoadInst>(use.getUser());
      auto SI = llvm::dyn_cast<StoreInst>(use.getUser());
      usedOnlyInLoadOrStore &= (LI != NULL) || (SI != NULL);

      VectorOfUses.push_back(&use);
    }

    if(!usedOnlyInLoadOrStore) continue;

    for (auto use : VectorOfUses) {

      // take use of inttoptr instruction, clone instruction,
      // insert clone right before the use, swap use to clone, remat
      auto User = use->getUser();
      auto UserInst = llvm::dyn_cast<Instruction>(User);

      if(UserInst) {
        auto Clone = el->clone();
        Clone->setName("cloned_" + el->getName());
        Clone->insertBefore(UserInst);
        *use = Clone;
        rematWholeChain((llvm::IntToPtrInst *)Clone);
        Result = true;
      }
    }
  }

  return Result;
}

bool CloneAddressArithmetic::runOnFunction(Function& F)
{
    bool Modified = false;
    Modified |= greedyRemat(F);
    return Modified;
}


FunctionPass* IGC::createRematAddressArithmeticPass() {
    return new RematAddressArithmetic();
}

char RematAddressArithmetic::ID = 0;

#define PASS_FLAG     "igc-remat-address-arithmetic"
#define PASS_DESC     "Remat Address Arithmetic"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
namespace IGC {
IGC_INITIALIZE_PASS_BEGIN(RematAddressArithmetic, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_END(RematAddressArithmetic, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
}

bool RematAddressArithmetic::runOnFunction(Function& F)
{
    bool modified = false;
    modified |= rematerializePhiMemoryAddressCalculation(F);
    modified |= rematerializePrivateMemoryAddressCalculation(F);
    return modified;
}

// Compares if two instructions are of the same kind, have the same return
// type and the same types of operands.
template<typename InstT>
static inline bool CompareInst(Value* a, Value* b)
{
    if (a == nullptr || b == nullptr ||
        a->getType() != b->getType() ||
        !isa<InstT>(a) || !isa<InstT>(b))
    {
        return false;
    }
    if (isa<Instruction>(a))
    {
        // For instructions also check opcode and operand types
        InstT* instA = cast<InstT>(a);
        InstT* instB = cast<InstT>(b);
        if (instA->getOpcode() != instB->getOpcode())
        {
            return false;
        }
        for (uint i = 0; i < instA->getNumOperands(); ++i)
        {
            if (instA->getOperand(i)->getType() != instB->getOperand(i)->getType())
            {
                return false;
            }
        }
    }
    return true;
}

// Rematerialize address calculations if address is a Phi instruction and all
// incoming values are results of identical address calculations, e.g.:
//
// true-bb:
//   %addrTrue = add i64 %base, 4
//   %ptrTrue  = inttoptr i64 %addrTrue to i64 addrspace(2)*
//   br label %merge-bb
//
// false-bb:
//   %addrFalse = add i64 %base, 4
//   %ptrFalse  = inttoptr i64 %addrFalse to i64 addrspace(2)*
//   br label %merge-bb
//
// merge-bb:
//   %addr = phi i64 addrspace(2)* [ %ptrTrue, %true-bb ], [ %ptrFalse, %false-bb ]
//   %result = load i64, i64 addrspace(2)* %addr, align 4
//
// Such "diamond-like" pattern can be created by GVN.
//
// The goal of the optimization is to potentially make the final memory
// operation uniform. Note that it many cases it would also be possible
// to hoist address calculations to the dominator basic block instead
// of rematerialization but hoisting could increase register pressure.
bool RematAddressArithmetic::rematerializePhiMemoryAddressCalculation(Function& F)
{
    bool modified = false;
    auto PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
    // Process all basic blocks in postdominator tree breadth first traversal.
    for (auto domIter = bf_begin(PDT->getRootNode()),
        domEnd = bf_end(PDT->getRootNode());
        domIter != domEnd;
        ++domIter)
    {
        BasicBlock* BB = domIter->getBlock();
        if (BB == nullptr)
        {
            continue;
        }
        for (auto II = BB->begin(), IE = BB->end();
            II != IE;
            ++II)
        {
            PHINode* phi = dyn_cast<PHINode>(&*II);
            if (!phi)
            {
                // No more Phi nodes in this BB, go to the next BB
                break;
            }
            if (!phi->getType()->isPointerTy() ||
                phi->hasNUses(0))
            {
                // Not an address, go to the next Phi
                continue;
            }
            bool doRemat = true;
            // For all incoming values compare the address calculations in
            // predecessors.
            for (uint i = 0; i < phi->getNumIncomingValues(); ++i)
            {
                // Current implementation only detects the inttoptr + add
                // pattern, e.g.:
                //   %offset = add i64 %2, 168
                //   %ptr = inttoptr i64 %offset to i64 addrspace(2)*
                Value* first = phi->getIncomingValue(0);
                Value* other = phi->getIncomingValue(i);
                if (!CompareInst<IntToPtrInst>(first, other))
                {
                    doRemat = false;
                    break;
                }
                first = cast<IntToPtrInst>(first)->getOperand(0);
                other = cast<IntToPtrInst>(other)->getOperand(0);
                if (!CompareInst<BinaryOperator>(first, other))
                {
                    doRemat = false;
                    break;
                }
                BinaryOperator* firstBinOp = cast<BinaryOperator>(first);
                BinaryOperator* otherBinOp = cast<BinaryOperator>(other);
                if (firstBinOp->getOpcode() != Instruction::Add ||
                    firstBinOp->getOperand(0) != otherBinOp->getOperand(0) ||
                    firstBinOp->getOperand(1) != otherBinOp->getOperand(1))
                {
                    doRemat = false;
                    break;
                }
            }
            if (doRemat)
            {
                IntToPtrInst* intToPtr = cast<IntToPtrInst>(phi->getIncomingValue(0));
                BinaryOperator* add = cast<BinaryOperator>(intToPtr->getOperand(0));
                // Clone address computations
                Instruction* newAdd = add->clone();
                Instruction* newIntToPtr = intToPtr->clone();
                newIntToPtr->setOperand(0, newAdd);
                // and insert in after the phi
                Instruction* insertPoint = BB->getFirstNonPHIOrDbgOrLifetime();
                newAdd->insertBefore(insertPoint);
                newIntToPtr->insertBefore(insertPoint);
                phi->replaceAllUsesWith(newIntToPtr);
                modified = true;
            }
        }
    }
    return modified;
}

bool RematAddressArithmetic::rematerializePrivateMemoryAddressCalculation(Function& F)
{
    bool changed = false;
    Value* PrivateBase = getPrivateMemoryValue(F);
    if (PrivateBase == nullptr)
        return false;

    DenseMap<Value*, SmallVector<Instruction*, 4>> BaseMap;
    SmallVector<std::pair<Instruction*, IntToPtrInst*>, 32> PointerList;

    SmallVector<std::pair<Value*, Value*>, 16> WorkList;
    WorkList.push_back(std::make_pair(PrivateBase, nullptr));
    while (!WorkList.empty()) {
        Value* V = nullptr;
        Value* U = nullptr;

        std::tie(V, U) = WorkList.back();
        WorkList.pop_back();

        if (auto Ptr = dyn_cast<IntToPtrInst>(V)) {
            BaseMap[U].push_back(Ptr);
            continue;
        }

        for (User* US : V->users())
        {
            // Don't add to chain of uses if it is PHINode
            if (isa<PHINode>(US))
                continue;
            WorkList.push_back(std::make_pair(US, V));
        }
    }

    DenseMap<Value*, SmallVector<Value*, 16>> CommonBaseMap;
    DenseMap<Value*, SmallVector<Value*, 4>> UseChain;
    for (auto& BM : BaseMap) {
        Value* Base = BM.first;
        auto& BaseUsers = BM.second;

        auto BO = dyn_cast<BinaryOperator>(Base);
        if (BO == nullptr)
            continue;

        if (isa<ConstantInt>(BO->getOperand(1))) {
            for (auto U : BaseUsers) {
                if (BO->getParent() != U->getParent()) {
                    CommonBaseMap[BO->getOperand(0)].push_back(U);
                    UseChain[U].push_back(BO);
                }
            }
        }
    }

    for (auto& CB : CommonBaseMap) {
        if (CB.second.size() < 2)
            continue;

        changed = true;
        for (auto V : CB.second) {
            auto I = dyn_cast<Instruction>(V);
            IGC_ASSERT(I != nullptr);
            rematerialize(I, UseChain[I]);
        }
    }
    return changed;
}

bool RematAddressArithmetic::rematerialize(Instruction* I, SmallVectorImpl<Value*>& Chain)
{
    Value* CurV = I;
    for (auto* V : Chain) {
        Instruction* Clone = dyn_cast<Instruction>(V)->clone();
        Clone->insertBefore(dyn_cast<Instruction>(CurV));
        for (auto& U : V->uses()) {
            if (CurV == U.getUser())
                U.set(Clone);
        }
        CurV = V;
    }
    return true;
}

static Value* getPrivateMemoryValue(Function& F)
{
    Value* PrivateBase = nullptr;
    for (auto AI = F.arg_begin(), AE = F.arg_end(); AI != AE; ++AI) {
        if (!AI->hasName())
            continue;
        auto Name = AI->getName().str();
        if (Name == "privateBase" && !AI->use_empty())
            PrivateBase = &*AI;
    }
    return PrivateBase;
}
