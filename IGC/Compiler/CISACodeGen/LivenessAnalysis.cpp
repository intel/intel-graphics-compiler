/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/LivenessAnalysis.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/LiveVars.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "common/debug/Debug.hpp"
#include "common/igc_regkeys.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/SparseBitVector.h"
#include <llvm/IR/CFG.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstIterator.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

char LivenessAnalysis::ID = 0;

std::string LivenessAnalysis::getllvmValueName(Value* V)
{
    if (V->hasName())
    {
        return std::string(V->getName());
    }
    // For unamed value, try the following:
    std::string Str;
    raw_string_ostream aOS(Str);
    aOS << *V;
    Str = aOS.str();
    // Name starting with %
    int bid = Str.find_first_of('%');
    int end = Str.find_first_of(' ', bid);
    std::string vname = Str.substr(bid, end - bid);
    return vname;
}

// Register pass to igc-opt
#define PASS_FLAG "igc-livenessanalysis"
#define PASS_DESCRIPTION "Calculate liveness based on LiveVars"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(LivenessAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
//IGC_INITIALIZE_PASS_DEPENDENCY(LiveVarsAnalysis)
IGC_INITIALIZE_PASS_END(LivenessAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

// runOnFunction set up Map only. calculate() computes Liveness.
bool LivenessAnalysis::runOnFunction(Function& F)
{
    m_F = &F;

    // Pre-allocate memory to avoid many small alocations.
    // todo: reuse it from LiveVars
    size_t nVals = m_F->arg_size();
    for (auto& BB : *m_F) {
        nVals += BB.size();
    }

    // We will allocate a bit more (~10%) in case it needs expansion.
    // Also, as llvm::DenseMap will resize if the Map's capacity is 75% full,
    // allocate even more to avoid such automatic resizing.
    uint32_t mapCap1 = int_cast<uint32_t>((size_t)(nVals * 1.40f));
    uint32_t vecCap1 = int_cast<uint32_t>((size_t)(nVals * 1.10f));
    ValueIds.grow(mapCap1);
    IdValues.reserve(vecCap1);

    initValueIds();

    return false;
}

LivenessAnalysis::~LivenessAnalysis()
{
    clear();
}

void LivenessAnalysis::clear()
{
    KillInsts.clear();
    ValueIds.clear();
    IdValues.clear();
    BBLiveIns.clear();

    delete m_LV;
    m_LV = nullptr;
}

void LivenessAnalysis::initValueIds()
{
    int ix = 0;
    for (Function::arg_iterator AI = m_F->arg_begin(), AE = m_F->arg_end();
        AI != AE; ++AI)
    {
        Value* Val = &*AI;
        ValueIds.insert(std::make_pair(Val, ix));
        IdValues.push_back(Val);
        ++ix;
    }

    for (inst_iterator II = inst_begin(m_F), IE = inst_end(m_F);
        II != IE; ++II)
    {
        Instruction* Inst = &*II;
        ValueIds.insert(std::make_pair(Inst, ix));
        IdValues.push_back(Inst);
        ++ix;
    }
}

void LivenessAnalysis::setLiveIn(BasicBlock* BB, Value* V)
{
    ValueToIntMap::iterator I = ValueIds.find(V);
    if (I == ValueIds.end())
    {
        return;
    }
    int id = I->second;
    setLiveIn(BB, id);
}

inline void LivenessAnalysis::setLiveIn(BasicBlock* BB, int ValueID)
{
    SBitVector& BV = BBLiveIns[BB];
    BV.set(ValueID);
}

inline void LivenessAnalysis::setKillInsts(Value* V, Instruction* kill)
{
    ValueVec& VS = KillInsts[kill];
    VS.push_back(V);
}

bool LivenessAnalysis::isInstLastUseOfValue(Value* V, Instruction* I)
{
    ValueVec& VS = KillInsts[I];
    for (int i = 0, e = (int)VS.size(); i < e; ++I)
    {
        if (VS[i] == V)
        {
            return true;
        }
    }
    return false;
}

bool LivenessAnalysis::isBefore(Instruction* A, Instruction* B)
{
    IGC_ASSERT(nullptr != A);
    IGC_ASSERT(nullptr != B);
    IGC_ASSERT_MESSAGE(A->getParent() == B->getParent(), "A and B must be within the same BB!");

    int nA = (int)getDistance(A);
    int nB = (int)getDistance(B);
    return nA < nB;
}

void LivenessAnalysis::calculate(Function* F)
{
    // If m_LV is not nullptr, it means that the full liveness has been
    // computed. No need to recompute and just return.
    if (m_LV)
    {
        return;
    }

    // Since we re-calculate, make sure to begin with a clean
    // state. Note that ValueIds are not cleared here, as we
    // want to make sure ValueIds are valid as long as this
    // analysis is valid.  If we re-calculate value ids, a value
    // might be assigned a different id than the one assigned
    // by runOnFunction.
    KillInsts.clear();
    BBLiveIns.clear();

    m_F = F;
    m_WIA = getAnalysisIfAvailable<WIAnalysis>();

    // todo: might use LiveVars as a pass
    m_LV = new LiveVars();
    m_LV->Calculate(F, m_WIA);


    // Pre-allocate memory to avoid many small alocations.
    size_t nVals = IdValues.size();

    // We will allocate a bit more (~10%) in case it needs expansion.
    // Also, as llvm::DenseMap will resize if the Map's capacity is 75% full,
    // allocate even more to avoid such automatic resizing.
    uint32_t mapCap1 = int_cast<uint32_t>((size_t)(nVals * 1.40f));
    uint32_t mapCap2 = int_cast<uint32_t>((size_t)(m_F->size() * 1.40f));
    BBLiveIns.grow(mapCap2);
    KillInsts.grow(mapCap1);

    for (LiveVars::iterator LVI = m_LV->begin(), LVE = m_LV->end();
        LVI != LVE; ++LVI)
    {
        Value* V = LVI->first;
        LiveVars::LVInfo* lvi = LVI->second;
        ValueToIntMap::iterator VI = ValueIds.find(V);
        if (VI == ValueIds.end())
        {
            // Only value in ValueIds are considered.
            continue;
        }
        int valID = VI->second;

        // V is either an instruction or an argument. If defBB is nullptr,
        // V is an argument; otherwise, it is the defining BB.
        BasicBlock* defBB = nullptr;
        if (Instruction * defInst = dyn_cast<Instruction>(V))
        {
            defBB = defInst->getParent();
        }

        for (auto II = lvi->AliveBlocks.begin(), IE = lvi->AliveBlocks.end();
            II != IE; ++II)
        {
            BasicBlock* BB = *II;
            setLiveIn(BB, valID);
        }

        for (std::vector<Instruction*>::iterator II = lvi->Kills.begin(),
            IE = lvi->Kills.end();
            II != IE; ++II)
        {
            Instruction* inst = *II;
            setKillInsts(V, inst);

            // If inst's BB isn't "V"'s defBB, V must be live into this BB.
            // This condition check also works when "V" is an argument.
            BasicBlock* useBB = inst->getParent();
            if (defBB != useBB)
            {
                setLiveIn(useBB, valID);
            }
        }
    }

    if (IGC_IS_FLAG_ENABLED(EnableLivenessDump))
    {
        print(errs());
    }
}

void LivenessAnalysis::print_livein(raw_ostream& OS, BasicBlock* BB)
{
    SBitVector& BitVec = BBLiveIns[BB];
    OS << "    Live-In-Values (#values = " << BitVec.count() << " ):\n";

    SBitVector::iterator I, E;
    int nVals = 0;
    for (I = BitVec.begin(), E = BitVec.end(); I != E; ++I)
    {
        int id = *I;
        Value* V = IdValues[id];
        IGC_ASSERT_MESSAGE(nullptr != V, "Value should be in Value Map!");
        if (nVals == 0) {
            OS << "      ";
        }
        if (V->hasName())
        {
            OS << V->getName() << ",  ";
        }
        else
        {
            OS << getllvmValueName(V) << ",  ";
        }
        ++nVals;
        if (nVals == 8)
        {
            // 8 values per line
            OS << "\n";
            nVals = 0;
        }
    }
    OS << "\n\n";
}

void LivenessAnalysis::print(raw_ostream& OS)
{
    if (!m_F) return;

    std::stringstream ss;
    ss << "LivenessAnalysis: " << m_F->getName().str();
    Debug::Banner(OS, ss.str());

    for (Function::iterator I = m_F->begin(), E = m_F->end(); I != E; ++I) {
        BasicBlock* BB = &*I;
        OS << "BB:";
        if (BB->hasName())
            OS << " " << BB->getName();
        OS << "       ; preds =";
        bool isFirst = true;
        for (pred_iterator PI = pred_begin(BB), PE = pred_end(BB); PI != PE; ++PI) {
            BasicBlock* pred = *PI;
            OS << ((isFirst) ? " " : ", ");
            OS << (pred->hasName() ? pred->getName() : "Unamed(wrong)");
            isFirst = false;
        }
        OS << "\n";
        print_livein(OS, BB);
        for (BasicBlock::iterator it = BB->begin(), ie = BB->end(); it != ie; ++it) {
            Instruction* I = &*it;
            OS << *I << '\n';
        }
        OS << "\n";
    }
}

#if defined( _DEBUG )

void LivenessAnalysis::dump()
{
    print(dbgs());

}

void LivenessAnalysis::dump_livein(BasicBlock* BB)
{
    print_livein(dbgs(), BB);
}

void LivenessAnalysis::dump_livein()
{
    if (!m_F) {
        return;
    }

    for (auto& BI : *m_F) {
        BasicBlock& BB = BI;
        print_livein(dbgs(), &BB);
    }
}

#endif
