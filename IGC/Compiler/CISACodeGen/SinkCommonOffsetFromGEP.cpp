/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/SinkCommonOffsetFromGEP.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/IGCPassSupport.h"
#include "llvm/IR/DerivedTypes.h"
#include <vector>
#include <algorithm>

using namespace llvm;
using namespace IGC;

namespace {
// This is to sink to the successor block the common constant offset from several GEP
// e.g
// ************************************
// bb1:
//    %0 = gep(a, 0)
//    %1 = gep(a, 1)
// bb2:
//    %2 = gep(b, 0)
//    %3 = gep(b, 1)
//bb3:
//    %4 = phi( [%0, bb1], [%2, bb2] )
//    %5 = phi( [%1, bb1], [%3, bb2] )
//    load [%4]
//    load [%5]
// ************************************
//
// converts to
//
// ************************************
// bb1:
//    %0 = a
// bb2:
//    %1 = b
// bb3:
//    %2 = phi( [%0, bb1], [%1, bb2] )
//    %3 = gep(%2, 0)
//    %4 = gep(%2, 1)
//    load[%3]
//    load[%4]
// ************************************
// This have two goals:
// 1) Could decrease register pressure
// 2) The MemOpt pass will merge loads

class SinkCommonOffsetFromGEP : public llvm::FunctionPass
{

public:
    static char ID;

    SinkCommonOffsetFromGEP();
    virtual bool runOnFunction(llvm::Function& F) override;

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
        AU.setPreservesCFG();
    }
};

// Struct for save GEP and corresponding addrespace cast instruction if exist
struct GepPointer
{
    GetElementPtrInst* GEP;
    AddrSpaceCastInst* Cast;

    static GepPointer get(Value* V);
};

// Struct for save divergent pointer phi node and corresponding GEP instructions
struct DivergentPointer
{
    PHINode* Phi = nullptr;
    MapVector<BasicBlock*, GepPointer> Geps;

    static DivergentPointer create(PHINode* PN);
};

// CommonBaseGroup is aimed to save all divergent pointers with common base of GEP
// e.g:
//
// bb1:
//   %0 = gep(a1, 0)
//   %1 = gep(a1, 1)
//
//   %3 = gep(a2, 0)
//   %4 = gep(a2, 1)
// bb2:
//   %5 = gep(b1, 0)
//   %6 = gep(b1, 1)
//
//   %7 = gep(b2, 0)
//   %8 = gep(b2, 1)
//
// CommonBaseGroupArray[0] = { { %0, %5}, {%1, %6} }
// CommonBaseGroupArray[1] = { { %3, %7}, {%4, %8} }
struct CommonBaseGroup
{
    SmallVector<DivergentPointer, 2> DivergentPointers;
    MapVector<BasicBlock*, std::pair<std::pair<Type*, Value*>, SmallVector<Value*, 4>>> GroupFactor;
    MapVector<BasicBlock*, AddrSpaceCastInst*> Casts;
    BasicBlock* Successor = nullptr;
};

class DivergentPointersGroups
{
public:
    DivergentPointersGroups() = default;

    void add(const DivergentPointer& DP);
    const std::vector<CommonBaseGroup>& getGroups() const { return PointerGroups; }

private:
    std::vector<CommonBaseGroup> PointerGroups;

    bool addToExistingGroup(const DivergentPointer& DP);
    bool createAddNewGroups(const DivergentPointer& DP);
    static bool isBelongedToGroup(const CommonBaseGroup& Group, const DivergentPointer& DP);
};

} // End anonymous namespace

char SinkCommonOffsetFromGEP::ID = 0;

#define PASS_FLAG "igc-sink-gep-constant"
#define PASS_DESCRIPTION "IGC Sink Common Offset From GEP"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false

namespace IGC {
IGC_INITIALIZE_PASS_BEGIN(SinkCommonOffsetFromGEP, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(SinkCommonOffsetFromGEP, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
} // End IGC namespace

FunctionPass* IGC::createSinkCommonOffsetFromGEPPass()  {
    return new SinkCommonOffsetFromGEP();
}

SinkCommonOffsetFromGEP::SinkCommonOffsetFromGEP() : FunctionPass(ID) {
    initializeSinkCommonOffsetFromGEPPass(*PassRegistry::getPassRegistry());
}

GepPointer GepPointer::get(Value* V) {
    GepPointer GP{nullptr, nullptr};

    Value* CurV = V;
    if (auto ASC = dyn_cast<AddrSpaceCastInst>(CurV)) {
        GP.Cast = ASC;
        CurV = ASC->getPointerOperand();
    }

    GP.GEP = dyn_cast<GetElementPtrInst>(CurV);
    return GP;
}

// Create DivergentPointer for phi node. If cannot find GEP for any incoming value Phi field is nullptr
DivergentPointer DivergentPointer::create(PHINode* PN) {
    DivergentPointer DP;
    for (Use& IV : PN->incoming_values()) {

        auto GP = GepPointer::get(IV.get());
        if (GP.GEP == nullptr)
            return DP;

        DP.Geps[PN->getIncomingBlock(IV)] = GP;
    }

    DP.Phi = PN;
    return DP;
}

// Get pure pointer skipping the cast operations
static Value* getPointer(Value* V) {
    if (auto ASC = dyn_cast<AddrSpaceCastInst>(V))
        return getPointer(ASC->getPointerOperand());
    if (auto BC = dyn_cast<BitCastInst>(V))
        return getPointer(BC->getOperand(0));
    return V;
}

// Get pointer for load/store
static Value* getPointerForMemRef(Instruction* I) {
    Value* Pointer = nullptr;
    if (auto LD = dyn_cast<LoadInst>(I))
        Pointer = getPointer(LD->getPointerOperand());
    if (auto ST = dyn_cast<StoreInst>(I))
        Pointer = getPointer(ST->getPointerOperand());

    return Pointer;
}

static std::vector<DivergentPointer> findDivergentPointers(BasicBlock* BB) {
    std::vector<DivergentPointer> DivergentPointers;
    for (auto II = BB->begin(), IE = BB->end(); II != IE; ++II) {

        Instruction* I = &(*II);

        if (!isa<LoadInst>(I) && !isa<StoreInst>(I))
            continue;

        auto Pointer = getPointerForMemRef(I);
        if (Pointer == nullptr)
            continue;

        auto PhiPointer = dyn_cast<PHINode>(Pointer);
        if (PhiPointer == nullptr)
            continue;

        auto DP = DivergentPointer::create(PhiPointer);
        if (DP.Phi == nullptr)
            continue;

        DivergentPointers.push_back(DP);
    }
    return DivergentPointers;
}

// Check if current DivergentPointer belonng to any CommonBaseGroup
bool DivergentPointersGroups::isBelongedToGroup(const CommonBaseGroup& Group, const DivergentPointer& DP) {
    DenseMap<BasicBlock*, std::pair<Value*, SmallVector<Value*, 4>>> CurGroupFactor;

    for (auto& IT : DP.Geps) {
        BasicBlock* BB          = IT.first;
        GetElementPtrInst* GEP  = IT.second.GEP;

        auto PtrOperand = GEP->getPointerOperand();
        SmallVector<Value*, 4> BaseIndices(GEP->idx_begin(), GEP->idx_end() - 1);
        CurGroupFactor[BB] = std::make_pair(PtrOperand, BaseIndices);
    }

    BasicBlock* Successor = DP.Phi->getParent();
    if (Successor != Group.Successor)
        return false;

    for (auto& IT : Group.GroupFactor) {
        BasicBlock* BB      = IT.first;
        Value* PtrOperand   = IT.second.first.second;
        auto& Indices       = IT.second.second;

        if (CurGroupFactor[BB].first != PtrOperand)
            return false;
        if (CurGroupFactor[BB].second != Indices)
            return false;
    }
    return true;
}

bool DivergentPointersGroups::addToExistingGroup(const DivergentPointer& DP) {
    for (auto& Group : PointerGroups) {
        if (isBelongedToGroup(Group, DP)) {
            Group.DivergentPointers.push_back(DP);
            return true;
        }
    }
    return false;
}

bool DivergentPointersGroups::createAddNewGroups(const DivergentPointer& DP) {
    CommonBaseGroup Group = {};
    Group.DivergentPointers.push_back(DP);
    for (auto& IT : DP.Geps) {
        BasicBlock* BB          = IT.first;
        AddrSpaceCastInst* ASC  = IT.second.Cast;
        GetElementPtrInst* GEP  = IT.second.GEP;

        if (GEP->getNumIndices() < 1)
            return false;

        SmallVector<Value*, 4> Indices(GEP->idx_begin(), GEP->idx_end() - 1);
        Value* PtrOperand = GEP->getPointerOperand();
        Type* PtrOperandTy = GEP->getSourceElementType();
        Group.GroupFactor[BB] = std::make_pair(std::make_pair(PtrOperandTy, PtrOperand), Indices);
        Group.Casts[BB] = ASC;
    }
    Group.Successor = DP.Phi->getParent();
    PointerGroups.push_back(Group);
    return true;
}

void DivergentPointersGroups::add(const DivergentPointer& DP) {
    // Check all GEPs without last index have the same type
    SmallVector<Type*, 2> Types;
    for (const auto& IT : DP.Geps) {
        GetElementPtrInst* GEP = IT.second.GEP;
        SmallVector<Value*, 2> IdxList { GEP->idx_begin(), GEP->idx_end() - 1 };
        auto Type = GetElementPtrInst::getIndexedType(GEP->getSourceElementType(), IdxList);
        Types.push_back(Type);
    }
    auto predicate = [&Types](Type* T) {
        return T == Types[0];
    };
    if (!std::all_of(Types.begin(), Types.end(), predicate))
        return;

    if (!addToExistingGroup(DP))
        createAddNewGroups(DP);
}

static Value* getLastIdx(GetElementPtrInst* GEP) {
    if (GEP->getNumIndices() == 0)
        return nullptr;

    auto IdxIt = GEP->idx_end() - 1;
    return IdxIt->get();;
};

static ConstantInt* getCommonConstantOffset(const DivergentPointer& DP) {
    SmallVector<Value*, 2> LastIndices;
    for (auto& IT : DP.Geps) {
        GetElementPtrInst* GEP = IT.second.GEP;
        LastIndices.push_back(getLastIdx(GEP));
    }

    auto IB = LastIndices.begin();
    auto ConstantOffset = dyn_cast<ConstantInt>(*IB);
    if (ConstantOffset == nullptr)
        return nullptr;

    for (auto II = IB + 1, IE = LastIndices.end(); II != IE; II++) {
        auto CI = dyn_cast<ConstantInt>(*II);
        if (CI == nullptr)
            return nullptr;

        if (CI != ConstantOffset)
            return nullptr;
    }

    return ConstantOffset;
}

static bool sinkCommonOffsetForGroup(const CommonBaseGroup& Group) {
    // Find pointers with common constant offset
    SmallVector<std::pair<DivergentPointer, ConstantInt*>, 2> PointersToSink;
    for (auto& Pointer : Group.DivergentPointers) {
        auto ConstantOffset = getCommonConstantOffset(Pointer);
        if (ConstantOffset == nullptr)
            continue;

        PointersToSink.push_back(std::make_pair(Pointer, ConstantOffset));
    }

    // If pointers to split and sink are less than 2 than do nothing
    if (PointersToSink.size() < 2)
        return false;

    SmallVector<std::pair<Value*, BasicBlock*>, 2> NewPointers;
    bool NewGEP = false;
    Type* PhiElType = nullptr;
    // Create GEP for the common base in every basic block
    for (auto& IT : Group.GroupFactor) {
        BasicBlock* BB      = IT.first;
        Type* PtrOperandTy  = IT.second.first.first;
        Value* PtrOperand   = IT.second.first.second;
        auto BaseIndices    = IT.second.second;

        Value * NewPointer = PtrOperand;
        if (NewPointer == nullptr)
            return false;

        // If we have gep(A, constant) just take pure A without creating new GEP
        auto NewPointerElType = PtrOperandTy;
        if (BaseIndices.size() > 0) {
            auto BaseGEP = GetElementPtrInst::Create(PtrOperandTy, PtrOperand, BaseIndices, "", BB->getTerminator());
            NewPointer = BaseGEP;
            NewPointerElType = BaseGEP->getResultElementType();

            NewGEP = true;
        }


        if (auto ASC = Group.Casts.find(BB)->second) {
            auto PType = PointerType::get(NewPointerElType, ASC->getDestAddressSpace());
            auto BaseASC = new AddrSpaceCastInst(NewPointer, PType, "", BB->getTerminator());
            NewPointer = BaseASC;
        }
        PhiElType = NewPointerElType;
        NewPointers.push_back(std::make_pair(NewPointer, BB));
    }

    // Create and fill new phi node for base GEPs
    auto PhiType = NewPointers.front().first->getType();
    auto BasePhi = PHINode::Create(PhiType, 2, "", Group.Successor->getFirstNonPHI());
    for (auto& NP : NewPointers)
        BasePhi->addIncoming(NP.first, NP.second);

    // Create offset GEPs
    for (auto& IT : PointersToSink) {
        ConstantInt* Offset = IT.second;
        PHINode* PN         = IT.first.Phi;
        auto Geps           = IT.first.Geps;

        SmallVector<Value*, 4> Indices;
        if (NewGEP)
            Indices.push_back(ConstantInt::get(Offset->getType(), 0));
        Indices.push_back(Offset);

        auto OffsetGEP = GetElementPtrInst::Create(PhiElType, BasePhi, Indices, "", BasePhi->getNextNonDebugInstruction());

        bool isInBounds = false;
        for (const auto& Gep : Geps)
            isInBounds |= Gep.second.GEP->isInBounds();
        OffsetGEP->setIsInBounds(isInBounds);

        PN->replaceAllUsesWith(OffsetGEP);
    }
    return true;
}

static bool sinkCommonOffset(const std::vector<DivergentPointer>& DivergentPointers) {
    DivergentPointersGroups Groups;
    for (auto& DP : DivergentPointers)
        Groups.add(DP);

    bool changed = false;
    for (auto& Group : Groups.getGroups())
        changed |= sinkCommonOffsetForGroup(Group);

    return changed;
}

bool SinkCommonOffsetFromGEP::runOnFunction(Function& F) {
    bool changed = false;
    for (Function::iterator BB = F.begin(), BBE = F.end(); BB != BBE; ++BB) {

        // Collect phi node pointers for basic block
        auto DivergentPointers = findDivergentPointers(&(*BB));

        // Does not make sense operate with less than 2 divergent pointers
        if (DivergentPointers.size() < 2)
            continue;

        changed |= sinkCommonOffset(DivergentPointers);
    }
    return changed;
}
