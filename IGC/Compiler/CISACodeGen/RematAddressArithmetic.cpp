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

} // end namespace

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
