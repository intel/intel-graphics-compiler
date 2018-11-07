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

#include "Compiler/Optimizer/CodeAssumption.hpp"
#include "Compiler/CodeGenPublic.h"
#include "common/igc_regkeys.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DenseMap.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"


using namespace llvm;
using namespace IGC;

namespace {
	const StringRef OCLBIF_GET_GLOBAL_ID = "_Z13get_global_idj";
	const StringRef OCLBIF_GET_LOCAL_ID = "_Z12get_local_idj";
	const StringRef OCLBIF_GET_GROUP_ID = "_Z12get_group_idj";
}

// Register pass to igc-opt
#define PASS_FLAG "igc-codeassumption"
#define PASS_DESCRIPTION "Generate llvm.assume"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(CodeAssumption, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(CodeAssumption, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)


char CodeAssumption::ID = 0;

bool CodeAssumption::runOnModule(Module& M)
{
	// Do it for 64-bit pointer only
	if (M.getDataLayout().getPointerSize() != 8) {
		return false;
	}

	for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
	{
		Function* F = &(*I);
		StringRef FN = F->getName();
		if (FN == OCLBIF_GET_GLOBAL_ID ||
			FN == OCLBIF_GET_LOCAL_ID ||
			FN == OCLBIF_GET_GROUP_ID)
		{
			for (auto U = F->user_begin(), UE = F->user_end(); U != UE; ++U)
			{
				CallInst *CI = dyn_cast<CallInst>(*U);
				if (!CI || !CI->getType()->isIntegerTy())
				{
					// sanity check
					continue;
				}

				BasicBlock::iterator InsertBefore(CI);
				++InsertBefore;
				IRBuilder<> IRB(CI->getParent(), InsertBefore);

				Constant *Zero = ConstantInt::get(CI->getType(), 0);
				Value *icmp = IRB.CreateICmpSGE(CI, Zero, "assumeCond");
				(void) IRB.CreateAssumption(icmp);

				if (CI->getType()->isIntegerTy(64))
				{
					// y = trunc i64 x to i32
					// Assume y is positive as well.
					for (auto UI = CI->user_begin(), UE = CI->user_end(); UI != UE; ++UI)
					{
						Instruction *userInst = dyn_cast<Instruction>(*UI);
						if (userInst && userInst->getOpcode() == Instruction::Trunc &&
							userInst->getType()->isIntegerTy(32))
						{
							BasicBlock::iterator pos(userInst);
							++pos;
							IRBuilder<> builder(userInst->getParent(), pos);
							Value *tmp = builder.CreateICmpSGE(userInst, Zero, "assumeCond");
							(void)IRB.CreateAssumption(tmp);
						}
					}
				}

				m_changed = true;
			}
		}
	}

	return m_changed;
}


/// APIs used directly (static functions)

// Check if a loop induction variable is always positive.
// If so, add assumption for that (LLVM value tracking does
// not handle this well, thus we will special-handle this
// case here). The pattern we check is something similar
// to the following:
//
// B0:
//    x0 = 0
//
// B1:
//    x = PHI [x0, B0] [x1, B1]
//    ...
// B1:
//    x1 = x + 1
//
// For this case, we are sure x is positive (overflow is a
// undefined behavior, and thus, do not bother overflow)!
//
bool CodeAssumption::isPositiveIndVar(
	PHINode *PN, const DataLayout* DL, AssumptionCache* AC)
{
	auto getCxtInst = [](Value *I) -> Instruction * {
		if (PHINode *phinode = dyn_cast<PHINode>(I)) {
			// llvm.assume for a PHI is inserted right after all
			// PHI instructions in the same BB. This assumption is
			// always true no matter where the PHI is used. To make
			// this work with llvm value tracking, set Cxt instruction
			// to be the last of this BB.
			return phinode->getParent()->getTerminator();
		}
		else if (Instruction *Inst = dyn_cast<Instruction>(I)) {
			return Inst;
		}
		return nullptr;
	};

	int nOpnds = PN->getNumOperands();
	if (nOpnds != 2 || !PN->getType()->isIntegerTy(32)) {
		return false;
	}
	Value *NonConstVal = nullptr;
	for (int i = 0; i < nOpnds; ++i)
	{
		Value *aVal = PN->getOperand(i);
		ConstantInt *IConst = dyn_cast<ConstantInt>(aVal);
		if ((IConst && IConst->getSExtValue() >= 0) ||
			(!IConst &&
				valueIsPositive(aVal, DL, AC, getCxtInst(aVal)))) {
			continue;
		}
		if (NonConstVal) {
			return false;
		}
		NonConstVal = aVal;
	}
	if (!NonConstVal) {
		return true;
	}
	Instruction *Inst = dyn_cast<Instruction>(NonConstVal);
	if (!Inst || Inst->getOpcode() != Instruction::Add) {
		return false;
	}
	ConstantInt *IC = nullptr;
	if (Inst->getOperand(0) == PN) {
		IC = dyn_cast<ConstantInt>(Inst->getOperand(1));
	}
	else if (Inst->getOperand(1) == PN) {
		IC = dyn_cast<ConstantInt>(Inst->getOperand(0));
	}
	if (IC && IC->getSExtValue() >= 0) {
		return true;
	}
	return false;
}

bool CodeAssumption::addAssumption(Function* F, AssumptionCache* AC)
{
	const DataLayout& DL = F->getParent()->getDataLayout();
	DenseMap<Value*, int> assumptionAdded;

	bool assumeAdded = false;
	bool changed = true;
	while (changed)
	{
		changed = false;
		for (auto BI = F->begin(), BE = F->end(); BI != BE; ++BI)
		{
			BasicBlock *BB = &(*BI);
			for (auto II = BB->begin(), IE = BB->end(); II != IE; ++II)
			{
				Instruction *Inst = &(*II);
				PHINode *PN = dyn_cast<PHINode>(Inst);
				if (!PN) break;
				if (assumptionAdded.count(PN) == 0 &&
					CodeAssumption::isPositiveIndVar(PN, &DL, AC))
				{
					IRBuilder<> IRB(BB->getFirstNonPHI());
					Constant *Zero = ConstantInt::get(PN->getType(), 0);
					Value *icmp = IRB.CreateICmpSGE(PN, Zero, "assumeCond");
					CallInst* assumeInst = IRB.CreateAssumption(icmp);

					// Register assumption
					if (AC)
					{
						AC->registerAssumption(assumeInst);
					}

					assumptionAdded[PN] = 1;
					changed = true;
					assumeAdded = true;
				}
			}
		}
	}
	return assumeAdded;
}
