/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMUtils.h"
#include "Compiler/CISACodeGen/RematAddressArithmetic.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Scalar.h"
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

static Value* getPrivateMemoryValue(Function& F);

namespace {

class RematAddressArithmetic : public FunctionPass {

private:
    DominatorTree* DT;
    LoopInfo* LI;

public:
    static char ID;

    RematAddressArithmetic() : FunctionPass(ID)
    {
        initializeRematAddressArithmeticPass(*PassRegistry::getPassRegistry());
    }

    bool runOnFunction(Function&) override;

private:
    bool rematerializePrivateMemoryAddressCalculation(Function& F);
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
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(RematAddressArithmetic, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
}

bool RematAddressArithmetic::runOnFunction(Function& F)
{
    return rematerializePrivateMemoryAddressCalculation(F);
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
        Value* V;
        Value* U;

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
