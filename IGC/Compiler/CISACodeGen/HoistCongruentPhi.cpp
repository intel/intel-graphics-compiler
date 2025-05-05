/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/HoistCongruentPhi.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"

#include "common/debug/Debug.hpp"
#include "common/debug/Dump.hpp"
#include "common/LLVMUtils.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/InstIterator.h"
#include "llvmWrapper/IR/Function.h"
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"


using namespace llvm;
using namespace IGC::Debug;

namespace IGC {
   // Register pass to igc-opt
#define PASS_FLAG "igc-hoist-congruent-phi"
#define PASS_DESCRIPTION "hoist congruent Phi"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
    IGC_INITIALIZE_PASS_BEGIN(HoistCongruentPHI, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
        IGC_INITIALIZE_PASS_END(HoistCongruentPHI, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

        char HoistCongruentPHI::ID = 0;
        HoistCongruentPHI::HoistCongruentPHI() : FunctionPass(ID) {
            initializeHoistCongruentPHIPass(*PassRegistry::getPassRegistry());
    }

    static unsigned numInsts(const Function& F)
    {
        return std::count_if(llvm::inst_begin(F), llvm::inst_end(F), [](const auto& I){ return !isDbgIntrinsic(&I); });
    }

    bool HoistCongruentPHI::runOnFunction(Function& F)
    {
        if (skipFunction(F))
            return false;

        CTX = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
        // only limited code-sinking to several shader-type
        // vs input has the URB-reuse issue to be resolved.
        // Also need to understand the performance benefit better.
        if (CTX->type != ShaderType::PIXEL_SHADER &&
            CTX->type != ShaderType::DOMAIN_SHADER &&
            CTX->type != ShaderType::OPENCL_SHADER &&
            CTX->type != ShaderType::RAYTRACING_SHADER &&
            CTX->type != ShaderType::COMPUTE_SHADER)
        {
            return false;
        }
        if (IGC_IS_FLAG_ENABLED(DisableCodeSinking) ||
            numInsts(F) < IGC_GET_FLAG_VALUE(CodeSinkingMinSize))
        {
            return false;
        }

        DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();

        bool Changed = hoistCongruentPhi(F);

        return Changed;
    }

    bool HoistCongruentPHI::checkCongruent(std::vector<InstPair> &instMap, const InstPair& values, InstVec& leaves, unsigned depth)
    {
        Instruction* src0 = values.first;
        Instruction* src1 = values.second;

        if (depth > 32 ||
            src0->getOpcode() != src1->getOpcode() ||
            src0->getNumOperands() != src1->getNumOperands() ||
            src0->getType() != src1->getType() ||
            isa<PHINode>(src0) ||
            isa<CmpInst>(src0) ||
            !(src0->getNumOperands() == 1 ||
                src0->getNumOperands() == 2) ||
                (isa<AllocaInst>(src0) && src0 != src1))
            return false;

        if (CallInst * call0 = dyn_cast<CallInst>(src0))
        {
            CallInst* call1 = dyn_cast<CallInst>(src1);
            IGC_ASSERT(call1 != nullptr);

            if (!call0->getCalledFunction() ||
                call0->getCalledFunction() != call1->getCalledFunction() ||
                !IGCLLVM::onlyWritesMemory(call0->getCalledFunction()) ||
                call0->isConvergent())
            {
                return false;
            }
        }
        else
            if (LoadInst * ld0 = dyn_cast<LoadInst>(src0))
            {
                LoadInst* ld1 = dyn_cast<LoadInst>(src1);
                IGC_ASSERT(ld1 != nullptr);
                if (ld0->getPointerAddressSpace() != ld1->getPointerAddressSpace())
                {
                    return false;
                }
                unsigned as = ld0->getPointerAddressSpace();
                unsigned bufId = 0;
                bool directBuf = false;
                BufferType bufType = IGC::DecodeAS4GFXResource(as, directBuf, bufId);
                if (bufType != CONSTANT_BUFFER)
                {
                    return false;
                }
            }

        bool equals = true;
        InstVec tmpVec;

        unsigned nopnds = src0->getNumOperands();

        if (nopnds == 2 && src0->getOperand(0) == src0->getOperand(1))
        {
            if (src1->getOperand(0) == src1->getOperand(1))
            {
                nopnds = 1;
            }
            else
            {
                return false;
            }
        }

        for (unsigned i = 0; i < nopnds; i++)
        {
            Value* v0, * v1;
            Instruction* iv0, * iv1;
            v0 = src0->getOperand(i);
            v1 = src1->getOperand(i);
            iv0 = dyn_cast<Instruction>(v0);
            iv1 = dyn_cast<Instruction>(v1);

            if (v0 == v1)
            {
                if (iv0)
                {
                    if (DT->dominates(iv0->getParent(), src0->getParent()) &&
                        DT->dominates(iv0->getParent(), src1->getParent()))
                    {
                        appendIfNotExist(tmpVec, iv0);
                        continue;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                    if (!(isa<Argument>(v0) || isa<Constant>(v0) || isa<GlobalValue>(v0)))
                    {
                        return false;
                    }
                continue;
            }
            if (iv0 && iv0->getParent() == src0->getParent() &&
                iv1 && iv1->getParent() == src1->getParent())
            {
                if (!checkCongruent(instMap, std::make_pair(iv0, iv1), tmpVec, depth + 1))
                {
                    equals = false;
                    break;
                }
            }
            else
            {
                equals = false;
                break;
            }
        }
        if (equals)
        {
            appendIfNotExist(std::make_pair(src0, src1), instMap);
            appendIfNotExist(leaves, tmpVec);
            return equals;
        }

        if (!src0->isCommutative() ||
            (src0->isCommutative() && src0->getOperand(0) == src0->getOperand(1)))
            return equals;

        equals = true;
        tmpVec.clear();
        for (unsigned i = 0; i < src0->getNumOperands(); i++)
        {
            Value* v0, * v1;
            Instruction* iv0, * iv1;
            v0 = src0->getOperand(i);
            v1 = src1->getOperand(1 - i);
            iv0 = dyn_cast<Instruction>(v0);
            iv1 = dyn_cast<Instruction>(v1);

            if (v0 == v1)
            {
                if (iv0)
                {
                    if (DT->dominates(iv0->getParent(), src0->getParent()) &&
                        DT->dominates(iv0->getParent(), src1->getParent()))
                    {
                        appendIfNotExist(tmpVec, iv0);
                        continue;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                    if (!(isa<Argument>(v0) || isa<Constant>(v0) || isa<GlobalValue>(v0)))
                    {
                        return false;
                    }
                continue;
            }

            if (iv0 && iv0->getParent() == src0->getParent() &&
                iv1 && iv1->getParent() == src1->getParent())
            {
                if (!checkCongruent(instMap, std::make_pair(iv0, iv1), leaves, depth + 1))
                {
                    equals = false;
                    break;
                }
            }
            else
            {
                equals = false;
                break;
            }
        }
        if (equals)
        {
            appendIfNotExist(std::make_pair(src0, src1), instMap);
            appendIfNotExist(leaves, tmpVec);
        }
        return equals;
    }

    bool HoistCongruentPHI::hoistCongruentPhi(PHINode* phi)
    {
        if (phi->getNumIncomingValues() != 2)
            return false;

        bool changed = false;
        InstVec leaves;

        Instruction* src0, * src1;
        src0 = dyn_cast<Instruction>(phi->getIncomingValue(0));
        src1 = dyn_cast<Instruction>(phi->getIncomingValue(1));
        if (src0 && src1 && src0 != src1)
        {
            // this vector maps all instructions leading to source0 of phi instruction to
            // the corresponding instructions of source1
            std::vector<InstPair> instMap;

            if (checkCongruent(instMap, std::make_pair(src0, src1), leaves, 0))
            {
                BasicBlock* predBB = nullptr;
                Instruction* insertPos = nullptr;
                bool apply = true;

                if (leaves.size() == 0)
                {
                    if (DT->dominates(src0, phi->getParent()))
                    {
                        phi->replaceAllUsesWith(src0);
                        return true;
                    }
                    else
                        if (DT->dominates(src1, phi->getParent()))
                        {
                            phi->replaceAllUsesWith(src1);
                            return true;
                        }
                        else
                        {
                            predBB = DT->findNearestCommonDominator(
                                src0->getParent(), src1->getParent());
                            insertPos = predBB->getTerminator();
                        }
                }
                else
                {
                    Instruction* last = nullptr;
                    for (auto* I : leaves)
                    {
                        if (!predBB)
                        {
                            predBB = I->getParent();
                            last = I;
                        }
                        else
                            if (predBB != I->getParent() ||
                                !DT->dominates(predBB, src0->getParent()) ||
                                !DT->dominates(predBB, src1->getParent()))
                            {
                                apply = false;
                                break;
                            }
                            else
                                if (!isInstPrecede(I, last))
                                {
                                    last = I;
                                }
                    }
                    if (isa<PHINode>(last))
                    {
                        insertPos = predBB->getFirstNonPHI();
                    }
                    else
                    {
                        insertPos = last->getNextNode();
                    }
                }
                if (apply)
                {
                    auto compareFunc = [](const InstPair& a, const InstPair& b) {
                        return (a.first == b.first) ? false : isInstPrecede(a.first, b.first);
                    };
                    std::sort(instMap.begin(), instMap.end(), compareFunc);
                    for (auto& insts : instMap)
                    {
                        Instruction* I = insts.first;
                        Instruction* ni = I->clone();

                        // It is possible that the `insertPos` is in the same
                        // block as a "replaced" instruction (the second
                        // instruction in an InstPair) and that the "replaced"
                        // instruction has users that are before the
                        // `insertPos`. In such cases the`insertPos` must be
                        // moved before all such users.
                        IGC_ASSERT(std::all_of(insts.first->user_begin(), insts.first->user_end(),
                            [this, insertPos](User* user)
                            {
                                Instruction* userInst = dyn_cast<Instruction>(user);
                                return userInst == nullptr ||
                                    DT->dominates(insertPos, userInst);
                            }));
                        IGC_ASSERT(std::all_of(insts.second->user_begin(), insts.second->user_end(),
                            [this, insertPos](User* user)
                            {
                                Instruction* userInst = dyn_cast<Instruction>(user);
                                return userInst == nullptr ||
                                    DT->dominates(insertPos->getParent(), userInst->getParent());
                            }));
                        Instruction* insertBefore = insertPos;
                        if (insts.second->getParent() == insertBefore->getParent() &&
                            isInstPrecede(insts.second, insertBefore))
                        {
                            for (User* user : insts.second->users())
                            {
                                Instruction* userInst = dyn_cast<Instruction>(user);
                                if (!userInst ||
                                    userInst->getParent() != insertBefore->getParent())
                                {
                                    continue;
                                }
                                if (isInstPrecede(userInst, insertBefore))
                                {
                                    insertBefore = userInst;
                                }
                            }
                        }
                        ni->insertBefore(insertBefore);
                        ni->setName(VALUE_NAME(ni->getName() + ".hoist"));

                        if (phi->getIncomingValue(0) == I)
                        {
                            // replace phi also
                            phi->replaceAllUsesWith(ni);
                        }
                        I->replaceAllUsesWith(ni);
                        insts.second->replaceAllUsesWith(ni);
                        // Note that instructions are removed in the second loop
                        // below to not invalidate the `insertPos` that may also
                        // be in the `instMap`
                    }
                    for (auto& insts : instMap)
                    {
                        insts.first->eraseFromParent();
                        insts.second->eraseFromParent();
                    }
                    changed = true;
                }
            }
        }
        return changed;
    }

    bool HoistCongruentPHI::hoistCongruentPhi(Function& F)
    {
        bool changed = false;
        for (auto& BB : F)
        {
            for (auto II = BB.begin(), IE = BB.end(); II != IE; )
            {
                PHINode* phi = dyn_cast<PHINode>(II);

                if (!phi)
                    break;

                if (hoistCongruentPhi(phi))
                {
                    changed = true;
                    II = phi->eraseFromParent();
                }
                else
                {
                    ++II;
                }
            }
        }
        return changed;
    }

}