/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
// We will sometimes see shaders where multiple continuations have exactly the
// same code in them.  For example, a common TraceRay() use is to make the call
// at the end of a shader.  This may result in a continuation that just releases
// the stack ID (for a raygen shader) or just does a merge (for a closest-hit,
// say).  Here, we try to merge continuations that are identical to help
// STS/BTD out to pack more lanes into an invocation by having less
// continuations to deal with.
//
//===----------------------------------------------------------------------===//

#include "FuseContinuations.h"
#include <set>
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Transforms/Utils/FunctionComparator.h"
#include "llvm/IR/InstIterator.h"
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"

using namespace llvm;
using namespace IGC;

namespace ContinuationFusing {

void fuseContinuations(Module& M, MapVector<Function*, FuncInfo>& ContMap)
{
    // Based off of data structures from MergeFunctions.cpp
    class FunctionNode
    {
      FunctionComparator::FunctionHash Hash;
      Function* F = nullptr;
      Function* RootFn = nullptr;
    public:
      // Note the hash is recalculated potentially multiple times, but it is cheap.
      FunctionNode(Function *F, Function *RootFn)
        : F(F), RootFn(RootFn), Hash(FunctionComparator::functionHash(*F))  {}

      Function *getFunc() const { return F; }
      Function *getRootFunc() const { return RootFn; }
      FunctionComparator::FunctionHash getHash() const { return Hash; }
    };

    // The function comparison operator is provided here so that FunctionNodes do
    // not need to become larger with another pointer.
    class FunctionNodeCmp
    {
        GlobalNumberState* GlobalNumbers;
    public:
        FunctionNodeCmp(GlobalNumberState* GN) : GlobalNumbers(GN) {}

        bool operator()(const FunctionNode& LHS, const FunctionNode& RHS) const
        {
            if (LHS.getRootFunc() != RHS.getRootFunc())
            {
                uint64_t L = GlobalNumbers->getNumber(LHS.getFunc());
                uint64_t R = GlobalNumbers->getNumber(RHS.getFunc());
                return L < R;
            }
            // Order first by hashes, then full function comparison.
            if (LHS.getHash() != RHS.getHash())
                return LHS.getHash() < RHS.getHash();
            FunctionComparator FCmp(LHS.getFunc(), RHS.getFunc(), GlobalNumbers);
            return FCmp.compare() == -1;
        }
    };

    DenseMap<uint32_t, SmallVector<ContinuationHLIntrinsic*, 4>> CIs;
    auto fill = [&]()
    {
        if (!CIs.empty())
            return;

        for (auto& F : M)
        {
            for (auto& I : instructions(F))
            {
                if (auto * CI = dyn_cast<ContinuationHLIntrinsic>(&I))
                    CIs[CI->getContinuationID()].push_back(CI);
            }
        }
    };

    using FnTreeType = std::set<FunctionNode, FunctionNodeCmp>;

    GlobalNumberState GlobalNumbers;
    FnTreeType FnTree{ FunctionNodeCmp(&GlobalNumbers) };

    for (auto& [Fn, FnInfo] : ContMap)
    {
        auto [I, Ok] = FnTree.insert(FunctionNode(Fn, FnInfo.RootFn));
        if (!Ok)
        {
            fill();
            auto* DupFn = I->getFunc();
            auto Entry = ContMap.find(DupFn);
            IGC_ASSERT(Entry != ContMap.end());
            uint32_t NewID = Entry->second.Idx;
            for (auto *CI : CIs[FnInfo.Idx])
            {
                CI->setContinuationID(NewID);
                CI->setContinuationFn(DupFn);
            }

            Fn->removeDeadConstantUsers();
            IGC_ASSERT_MESSAGE(Fn->use_empty(), "other uses?");

            Fn->eraseFromParent();
            Fn = nullptr;
        }
    }

    ContMap.remove_if([](auto& P) { return P.first == nullptr; });
}

} // namespace ContinuationFusing
