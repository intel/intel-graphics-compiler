/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/ResolveGAS.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/CastToGASAnalysis.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/IGCPassSupport.h"
#include "WrapperLLVM/Utils.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Support/Debug.h"
#include "llvmWrapper/IR/Constant.h"
#include "LLVM3DBuilder/MetadataBuilder.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/MemoryLocation.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/NoFolder.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "Probe/Assertion.h"
#include <llvm/IR/PatternMatch.h>

#define DEBUG_TYPE "gas-resolver"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;
using namespace llvm::PatternMatch;

namespace {

    typedef IRBuilder<llvm::NoFolder> BuilderType;

    class GASPropagator;

    // Generic address space (GAS) pointer resolving is done in two steps:
    // 1) Find cast from non-GAS pointer to GAS pointer
    // 2) Propagate that non-GAS pointer to all users of that GAS pointer at best
    //    effort.
    class GASResolving : public FunctionPass {
        const unsigned GAS = ADDRESS_SPACE_GENERIC;

        BuilderType* IRB;
        GASPropagator* Propagator;

    public:
        static char ID;

        GASResolving() : FunctionPass(ID), IRB(nullptr), Propagator(nullptr) {
            initializeGASResolvingPass(*PassRegistry::getPassRegistry());
        }

        bool runOnFunction(Function&) override;

        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
            AU.addRequired<LoopInfoWrapperPass>();
            AU.addRequired<AAResultsWrapperPass>();
            AU.addRequired<MetaDataUtilsWrapper>();
        }

    private:
        bool resolveOnFunction(Function*) const;
        bool resolveOnBasicBlock(BasicBlock*) const;

        bool resolveMemoryFromHost(Function&) const;

        bool checkGenericArguments(Function& F) const;
        void convertLoadToGlobal(LoadInst* LI) const;
        bool isLoadGlobalCandidate(LoadInst* LI) const;

        bool canonicalizeAddrSpaceCasts(Function& F) const;
    };

    class GASPropagator : public InstVisitor<GASPropagator, bool> {
        friend class InstVisitor<GASPropagator, bool>;

        LoopInfo* const LI;
        BuilderType IRB;

        Use* TheUse;
        Value* TheVal;

        // Phi node being able to be resolved from its initial value.
        DenseSet<PHINode*> ResolvableLoopPHIs;

    public:
        GASPropagator(LLVMContext& Ctx, LoopInfo* LoopInfo)
            : IRB(Ctx), LI(LoopInfo), TheUse(nullptr), TheVal(nullptr) {
            populateResolvableLoopPHIs();
        }

        bool propagateToUser(Use* U, Value* V) {
            TheUse = U;
            TheVal = V;
            Instruction* I = cast<Instruction>(U->getUser());
            return visit(*I);
        }
        bool propagateToAllUsers(AddrSpaceCastInst* I);
        void propagate(Value* I);
    private:
        void populateResolvableLoopPHIs();
        void populateResolvableLoopPHIsForLoop(const Loop* L);
        bool isResolvableLoopPHI(PHINode* PN) const {
            return ResolvableLoopPHIs.count(PN) != 0;
        }
        bool isAddrSpaceResolvable(PHINode* PN, const Loop* L,
            BasicBlock* BackEdge) const;

        bool visitInstruction(Instruction& I);

        bool visitLoadInst(LoadInst&);
        bool visitStoreInst(StoreInst&);
        bool visitAddrSpaceCastInst(AddrSpaceCastInst&);
        bool visitBitCastInst(BitCastInst&);
        bool visitPtrToIntInst(PtrToIntInst&);
        bool visitGetElementPtrInst(GetElementPtrInst&);
        bool visitPHINode(PHINode&);
        bool visitICmp(ICmpInst&);
        bool visitSelect(SelectInst&);
        bool visitMemCpyInst(MemCpyInst&);
        bool visitMemMoveInst(MemMoveInst&);
        bool visitMemSetInst(MemSetInst&);
        bool visitCallInst(CallInst&);
        bool visitDbgDeclareInst(DbgDeclareInst&);
        bool visitDbgValueInst(DbgValueInst&);
    };

    class GASRetValuePropagator : public ModulePass {
        Module* m_module = nullptr;
        IGCMD::MetaDataUtils* m_mdUtils = nullptr;
        CodeGenContext* m_ctx = nullptr;
        GASPropagator* m_Propagator;

    public:
        static char ID;

        GASRetValuePropagator() : ModulePass(ID) {
            initializeGASRetValuePropagatorPass(*PassRegistry::getPassRegistry());
        }

        bool runOnModule(Module&) override;

        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<CallGraphWrapperPass>();
            AU.addRequired<LoopInfoWrapperPass>();
        }

        bool propagateReturnValue(Function*&);
        std::vector<Function*> findCandidates(CallGraph&);

    private:
        std::vector<ReturnInst*> getAllRetInstructions(Function&);
        void updateFunctionRetInstruction(Function*);
        PointerType* getRetValueNonGASType(Function*);
        void transferFunctionBody(Function*, Function*);
        void updateAllUsesWithNewFunction(Function*, Function*);
        void updateMetadata(Function*, Function*);
        Function* createNewFunctionDecl(Function*, Type*);
        Function* cloneFunctionWithModifiedRetType(Function*, PointerType*);
    };
} // End anonymous namespace

FunctionPass* IGC::createResolveGASPass() { return new GASResolving(); }

char GASResolving::ID = 0;

#define PASS_FLAG "igc-gas-resolve"
#define PASS_DESC "Resolve generic address space"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false

namespace IGC {
    IGC_INITIALIZE_PASS_BEGIN(GASResolving, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY,
        PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(AAResultsWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
        IGC_INITIALIZE_PASS_END(GASResolving, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY,
            PASS_ANALYSIS)
}

bool GASResolving::runOnFunction(Function& F) {
    LLVMContext& Ctx = F.getContext();
    LoopInfo& LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    BuilderType TheBuilder(Ctx);
    GASPropagator ThePropagator(Ctx, &LI);
    IRB = &TheBuilder;
    Propagator = &ThePropagator;
    bool Changed = false;

    Changed |= canonicalizeAddrSpaceCasts(F);
    Changed |= resolveMemoryFromHost(F);

    bool LocalChanged = false;
    do {
        LocalChanged = resolveOnFunction(&F);
        Changed |= LocalChanged;
    } while (LocalChanged);
    return Changed;
}

bool GASResolving::resolveOnFunction(Function* F) const {
    bool Changed = false;

    ReversePostOrderTraversal<Function*> RPOT(F);
    for (auto& BB : RPOT)
        Changed |= resolveOnBasicBlock(BB);

    return Changed;
}

// Transform the following cast
//
//  addrspacecast SrcTy addrspace(S)* to DstTy addrspace(T)*
//
// to
//
//  bitcast SrcTy addrspace(S)* to DstTy addrspace(S)*
//  addrspacecast DstTy addrspace(S)* to DstTy addrspace(T)*
//
// OpaquePointers TODO: This method will be useless once IGC is switched to opaque pointers
bool GASResolving::canonicalizeAddrSpaceCasts(Function& F) const {
    std::vector<AddrSpaceCastInst*> GASAddrSpaceCasts;
    for (auto& I : make_range(inst_begin(F), inst_end(F)))
        if (AddrSpaceCastInst* ASCI = dyn_cast<AddrSpaceCastInst>(&I))
            if(ASCI->getDestAddressSpace() == GAS)
                GASAddrSpaceCasts.push_back(ASCI);

    bool changed = false;
    BuilderType::InsertPointGuard Guard(*IRB);
    for (auto ASCI : GASAddrSpaceCasts)
    {
        Value* Src = ASCI->getPointerOperand();
        Type* SrcType = Src->getType();
        Type* DstElementType = ASCI->getType()->getPointerElementType();

        if (SrcType->getPointerElementType() == DstElementType)
            continue;

        PointerType* TransPtrTy = PointerType::get(DstElementType, SrcType->getPointerAddressSpace());
        IRB->SetInsertPoint(ASCI);
        Src = IRB->CreateBitCast(Src, TransPtrTy);
        ASCI->setOperand(0, Src);
        changed = true;
    }
    return changed;
}

bool GASResolving::resolveOnBasicBlock(BasicBlock* BB) const {
    bool Changed = false;

    for (auto BI = BB->begin(), BE = BB->end(); BI != BE; /* EMPTY */) {
        Instruction* I = &(*BI++);
        AddrSpaceCastInst* CI = dyn_cast<AddrSpaceCastInst>(I);
        // Skip non `addrspacecast` instructions.
        if (!CI)
            continue;
        PointerType* DstPtrTy = cast<PointerType>(CI->getType());
        // Skip non generic address casting.
        if (DstPtrTy->getAddressSpace() != GAS)
            continue;

        Changed = Propagator->propagateToAllUsers(CI);

        // Re-update next instruction once there's change.
        if (Changed)
            BI = std::next(BasicBlock::iterator(CI));
        // Remove this `addrspacecast` once it's no longer used.
        if (CI->use_empty()) {
            CI->eraseFromParent();
            Changed = true;
        }
    }

    return Changed;
}

bool GASPropagator::isAddrSpaceResolvable(PHINode* PN, const Loop* L,
    BasicBlock* BackEdge) const {
    PointerType* PtrTy = dyn_cast<PointerType>(PN->getType());
    if (!PtrTy || PtrTy->getAddressSpace() != ADDRESS_SPACE_GENERIC)
        return false;

    Instruction* Next =
        dyn_cast<Instruction>(PN->getIncomingValueForBlock(BackEdge));
    if (!Next)
        return false;

    // Walk through use-def chain to figure out whether `Next` is resolvable from
    // `PN`.
    while (Next != PN) {
        // GEP
        if (GetElementPtrInst * GEP = dyn_cast<GetElementPtrInst>(Next)) {
            Next = dyn_cast<Instruction>(GEP->getPointerOperand());
            if (!Next)
                return false;
            continue;
        }
        // TODO: Add other operators.
        return false;
    }

    return true;
}

void GASPropagator::populateResolvableLoopPHIs() {
    for (auto& L : LI->getLoopsInPreorder()) {
        populateResolvableLoopPHIsForLoop(L);
    }
}

void GASPropagator::populateResolvableLoopPHIsForLoop(const Loop* L) {
    BasicBlock* H = L->getHeader();

    pred_iterator PI = pred_begin(H), E = pred_end(H);
    if (PI == E)
        return;

    BasicBlock* Incoming = *PI++;
    if (PI == E)
        return;

    BasicBlock* BackEdge = *PI++;
    if (PI != E)
        return;

    if (L->contains(Incoming)) {
        if (L->contains(BackEdge))
            return;
        std::swap(Incoming, BackEdge);
    }
    else if (!L->contains(BackEdge))
        return;

    for (auto I = H->begin(); isa<PHINode>(I); ++I) {
        PHINode* PN = cast<PHINode>(I);
        if (!isAddrSpaceResolvable(PN, L, BackEdge))
            continue;
        ResolvableLoopPHIs.insert(PN);
    }
}

bool GASPropagator::visitInstruction(Instruction& I) {
    // DO NOTHING.
    LLVM_DEBUG(dbgs() << "PROPAGATE:" << *TheVal << '\n');
    LLVM_DEBUG(dbgs() << "  THROUGH:" << I << '\n');
    return false;
}

bool GASPropagator::visitLoadInst(LoadInst&) {
    TheUse->set(TheVal);
    return true;
}

bool GASPropagator::visitStoreInst(StoreInst& ST) {
    // Only propagate on the `pointer` operand. If that generic pointer is used
    // as value operand and stored in memory, we have to use its value in generic
    // address space.
    if (TheUse->getOperandNo() != ST.getPointerOperandIndex())
        return false;
    TheUse->set(TheVal);
    return true;
}

bool GASPropagator::visitAddrSpaceCastInst(AddrSpaceCastInst& I) {
    PointerType* SrcPtrTy = cast<PointerType>(TheVal->getType());
    PointerType* DstPtrTy = cast<PointerType>(I.getType());
    // Skip if a cast between two different address spaces will be generated.
    if (SrcPtrTy->getAddressSpace() != DstPtrTy->getAddressSpace())
        return false;

    Value* Src = TheVal;
    if (SrcPtrTy->getPointerElementType() != DstPtrTy->getPointerElementType()) {
        BuilderType::InsertPointGuard Guard(IRB);
        IRB.SetInsertPoint(&I);
        Src = IRB.CreateBitCast(Src, DstPtrTy);
    }
    I.replaceAllUsesWith(Src);
    I.eraseFromParent();

    return true;
}

bool GASPropagator::visitBitCastInst(BitCastInst& I) {
    PointerType* SrcPtrTy = cast<PointerType>(TheVal->getType());
    PointerType* DstPtrTy = cast<PointerType>(I.getType());

    BuilderType::InsertPointGuard Guard(IRB);
    IRB.SetInsertPoint(I.getNextNode());
    // Push `addrspacecast` forward by replacing this `bitcast` on GAS with the
    // one on non-GAS followed by a new `addrspacecast` to GAS.
    Type* DstTy = DstPtrTy->getPointerElementType();
    PointerType* TransPtrTy =
        PointerType::get(DstTy, SrcPtrTy->getAddressSpace());
    Value* Src = TheVal;
    if (SrcPtrTy->getPointerElementType() != DstTy)
        Src = IRB.CreateBitCast(Src, TransPtrTy);
    Value* NewPtr = IRB.CreateAddrSpaceCast(Src, DstPtrTy);
    I.replaceAllUsesWith(NewPtr);
    I.eraseFromParent();

    return true;
}

bool GASPropagator::visitPtrToIntInst(PtrToIntInst& I) {
    // Don't propagate through `ptrtoint` as that conversion is different from
    // various address spaces.
    return false;
}

bool GASPropagator::visitGetElementPtrInst(GetElementPtrInst& I) {
    PointerType* SrcPtrTy = cast<PointerType>(TheVal->getType());
    PointerType* DstPtrTy = cast<PointerType>(I.getType());

    BuilderType::InsertPointGuard Guard(IRB);
    IRB.SetInsertPoint(I.getNextNode());
    // Push `getelementptr` forward by replacing this `bitcast` on GAS with the
    // one on non-GAS followed by a new `addrspacecast` to GAS.
    Type* DstTy = DstPtrTy->getPointerElementType();
    PointerType* TransPtrTy =
        PointerType::get(DstTy, SrcPtrTy->getAddressSpace());
    TheUse->set(TheVal);
    I.mutateType(TransPtrTy);
    Value* NewPtr = IRB.CreateAddrSpaceCast(&I, DstPtrTy);
    for (auto UI = I.use_begin(), UE = I.use_end(); UI != UE; /*EMPTY*/) {
        Use& U = *UI++;
        if (U.getUser() == NewPtr)
            continue;
        U.set(NewPtr);
    }
    return true;
}

bool GASPropagator::visitPHINode(PHINode& PN) {
    Type* NonGASTy = TheVal->getType();
    Type* GASTy = PN.getType();

    unsigned e = PN.getNumIncomingValues();
    SmallVector<Value*, 4> NewIncomingValues(e);

    if (isResolvableLoopPHI(&PN)) {
        // For resolvable loop phi, resolve it based on the
        for (unsigned i = 0; i != e; ++i) {
            Value* V = PN.getIncomingValue(i);
            // For incoming value, use the value being propagated.
            if (V == TheUse->get()) {
                NewIncomingValues[i] = TheVal;
                continue;
            }
            Instruction* I = cast<Instruction>(V);
            // For value generated inside loop, cast them to non-GAS pointers.
            BuilderType::InsertPointGuard Guard(IRB);
            IRB.SetInsertPoint(I->getNextNode());

            NewIncomingValues[i] = IRB.CreateAddrSpaceCast(I, NonGASTy);
        }
    }
    else {
        // Otherwise check whether all incoming values are casted from the same
        // address space.
        for (unsigned i = 0; i != e; ++i) {
            Value* V = PN.getIncomingValue(i);
            if (V == TheUse->get()) {
                NewIncomingValues[i] = TheVal;
                continue;
            }

            Value* NewVal = nullptr;
            if (isa<ConstantPointerNull>(V)) {
                NewVal = ConstantPointerNull::get(cast<PointerType>(NonGASTy));
            }
            else if (AddrSpaceCastInst* ASCI = dyn_cast<AddrSpaceCastInst>(V)) {
                if (ASCI->getSrcTy() == NonGASTy)
                    NewVal = ASCI->getOperand(0);;
            }

            if (!NewVal) return false;

            NewIncomingValues[i] = NewVal;
        }
    }

    // Propagate this phi node.
    PHINode* NewPN = PHINode::Create(NonGASTy, e, "", &PN);
    for (unsigned i = 0; i != e; ++i)
        NewPN->addIncoming(NewIncomingValues[i], PN.getIncomingBlock(i));
    NewPN->takeName(&PN);
    NewPN->setDebugLoc(PN.getDebugLoc());

    BuilderType::InsertPointGuard Guard(IRB);
    IRB.SetInsertPoint(PN.getParent()->getFirstNonPHI());
    Value* NewPtr = IRB.CreateAddrSpaceCast(NewPN, GASTy);
    PN.replaceAllUsesWith(NewPtr);
    PN.eraseFromParent();
    return true;
}

bool GASPropagator::visitICmp(ICmpInst& I) {
    Type* NonGASTy = TheVal->getType();

    unsigned OpNo = TheUse->getOperandNo();
    Use* TheOtherUse = &I.getOperandUse(1 - OpNo);

    AddrSpaceCastInst* ASCI = dyn_cast<AddrSpaceCastInst>(TheOtherUse->get());
    if (!ASCI || ASCI->getSrcTy() != NonGASTy)
        return false;

    TheUse->set(TheVal);
    TheOtherUse->set(ASCI->getOperand(0));

    return true;
}

bool GASPropagator::visitSelect(SelectInst& I) {
    Type* NonGASTy = TheVal->getType();

    unsigned OpNo = TheUse->getOperandNo();
    Use* TheOtherUse = &I.getOperandUse(3 - OpNo);

    Value* TheOtherVal = nullptr;
    if (isa<ConstantPointerNull>(TheOtherUse->get())) {
        TheOtherVal = ConstantPointerNull::get(cast<PointerType>(NonGASTy));
    }
    else if (AddrSpaceCastInst* ASCI = dyn_cast<AddrSpaceCastInst>(TheOtherUse->get())) {
        if (ASCI->getSrcTy() == NonGASTy)
            TheOtherVal = ASCI->getPointerOperand();
    }

    if (!TheOtherVal) return false;

    // Change select operands to non-GAS
    TheUse->set(TheVal);
    TheOtherUse->set(TheOtherVal);

    // Handle select return type
    BuilderType::InsertPointGuard Guard(IRB);
    IRB.SetInsertPoint(I.getNextNode());

    PointerType* DstPtrTy = cast<PointerType>(I.getType());
    PointerType* NonGASPtrTy = dyn_cast<PointerType>(NonGASTy);

    // Push 'addrspacecast' forward by changing the select return type to non-GAS pointer
    // followed by a new 'addrspacecast' to GAS
    PointerType* TransPtrTy = PointerType::get(DstPtrTy->getPointerElementType(), NonGASPtrTy->getAddressSpace());
    I.mutateType(TransPtrTy);
    Value* NewPtr = IRB.CreateAddrSpaceCast(&I, DstPtrTy);

    for (auto UI = I.use_begin(), UE = I.use_end(); UI != UE;) {
        Use& U = *UI++;
        if (U.getUser() == NewPtr)
            continue;
        U.set(NewPtr);
    }
    return true;
}

static bool handleMemTransferInst(MemTransferInst& I) {
    Value* NewDst = nullptr;
    Type* NewDstTy = nullptr;
    Use* DstUse = &I.getArgOperandUse(0);
    if (auto ASCI = dyn_cast<AddrSpaceCastInst>(DstUse->get())) {
        NewDst = ASCI->getOperand(0);
        NewDstTy = NewDst->getType();
    }

    Value* NewSrc = nullptr;
    Type* NewSrcTy = nullptr;
    Use* SrcUse = &I.getArgOperandUse(1);
    if (auto ASCI = dyn_cast<AddrSpaceCastInst>(SrcUse->get())) {
        NewSrc = ASCI->getOperand(0);
        NewSrcTy = NewSrc->getType();
    }

    // No address space cast on src or dst.
    if (NewDst == nullptr && NewSrc == nullptr)
        return false;

    Type* Tys[] = { NewDstTy ? NewDstTy : I.getArgOperand(0)->getType(),
                   NewSrcTy ? NewSrcTy : I.getArgOperand(1)->getType(),
                   I.getArgOperand(2)->getType() };
    Function* Fn = nullptr;
    IGC_ASSERT(nullptr != I.getParent());
    IGC_ASSERT(nullptr != I.getParent()->getParent());
    Module* M = I.getParent()->getParent()->getParent();
    if (isa<MemCpyInst>(I))
        Fn = Intrinsic::getDeclaration(M, Intrinsic::memcpy, Tys);
    else if (isa<MemMoveInst>(I))
        Fn = Intrinsic::getDeclaration(M, Intrinsic::memmove, Tys);
    else
        IGC_ASSERT_EXIT_MESSAGE(0, "unsupported memory intrinsic");

    I.setCalledFunction(Fn);
    if (nullptr != NewDst)
    {
        IGC_ASSERT(nullptr != DstUse);
        DstUse->set(NewDst);
    }
    if (nullptr != NewSrc)
    {
        IGC_ASSERT(nullptr != SrcUse);
        SrcUse->set(NewSrc);
    }
    return true;
}

bool GASPropagator::visitMemCpyInst(MemCpyInst& I) {
    return handleMemTransferInst(I);
}

bool GASPropagator::visitMemMoveInst(MemMoveInst& I) {
    return handleMemTransferInst(I);
}

bool GASPropagator::visitMemSetInst(MemSetInst& I) {
    Use* DstUse = &I.getArgOperandUse(0);
    auto ASCI = dyn_cast<AddrSpaceCastInst>(DstUse->get());
    if (!ASCI)
        return false;

    Value* OrigDst = ASCI->getOperand(0);
    Type* OrigDstTy = OrigDst->getType();

    Type* Tys[] = { OrigDstTy, I.getArgOperand(2)->getType() };
    Function* Fn = Intrinsic::getDeclaration(
        I.getParent()->getParent()->getParent(), Intrinsic::memset, Tys);

    I.setCalledFunction(Fn);
    DstUse->set(OrigDst);
    return true;
}

bool GASPropagator::visitCallInst(CallInst& I) {
    Function* Callee = I.getCalledFunction();

    if (!Callee)
        return false;

    PointerType* SrcPtrTy = cast<PointerType>(TheVal->getType());
    bool IsGAS2P =
        Callee->getName().equals("__builtin_IB_memcpy_generic_to_private");
    bool IsP2GAS =
        Callee->getName().equals("__builtin_IB_memcpy_private_to_generic");
    if (IsGAS2P || IsP2GAS) {
        PointerType* SrcPtrTy = cast<PointerType>(TheVal->getType());

        Type* Tys[4];
        Tys[0] = IsGAS2P ? I.getArgOperand(0)->getType() : SrcPtrTy;
        Tys[1] = IsGAS2P ? SrcPtrTy : I.getArgOperand(1)->getType();
        Tys[2] = I.getArgOperand(2)->getType();
        Tys[3] = I.getArgOperand(3)->getType();
        FunctionType* FTy = FunctionType::get(I.getType(), Tys, false);
        Module* M = I.getParent()->getParent()->getParent();
        IGCLLVM::Constant NewF = nullptr;
        switch (SrcPtrTy->getAddressSpace()) {
        case ADDRESS_SPACE_PRIVATE:
            NewF =
                M->getOrInsertFunction("__builtin_IB_memcpy_private_to_private", FTy);
            break;
        case ADDRESS_SPACE_GLOBAL:
            NewF = M->getOrInsertFunction(
                IsGAS2P ? "__builtin_IB_memcpy_global_to_private"
                : "__builtin_IB_memcpy_private_to_global",
                FTy);
            break;
        case ADDRESS_SPACE_CONSTANT:
            NewF = M->getOrInsertFunction(
                IsGAS2P ? "__builtin_IB_memcpy_constant_to_private"
                : "__builtin_IB_memcpy_private_to_constant",
                FTy);
            break;
        case ADDRESS_SPACE_LOCAL:
            NewF = M->getOrInsertFunction(
                IsGAS2P ? "__builtin_IB_memcpy_local_to_private"
                : "__builtin_IB_memcpy_private_to_local",
                FTy);
            break;
        }

        if (NewF) {
            I.setCalledFunction(NewF);
            TheUse->set(TheVal);
            return true;
        }
    }

    if (Callee->getName().equals("__builtin_IB_to_local")) {
        Type* DstTy = I.getType();
        Value* NewPtr = Constant::getNullValue(DstTy);
        if (SrcPtrTy->getAddressSpace() == ADDRESS_SPACE_LOCAL) {
            BuilderType::InsertPointGuard Guard(IRB);
            IRB.SetInsertPoint(&I);
            NewPtr = IRB.CreateBitCast(TheVal, DstTy);
        }
        I.replaceAllUsesWith(NewPtr);
        I.eraseFromParent();

        return true;
    }

    if (Callee->getName().equals("__builtin_IB_to_private")) {
        Type* DstTy = I.getType();
        Value* NewPtr = Constant::getNullValue(DstTy);
        if (SrcPtrTy->getAddressSpace() == ADDRESS_SPACE_PRIVATE) {
            BuilderType::InsertPointGuard Guard(IRB);
            IRB.SetInsertPoint(&I);
            NewPtr = IRB.CreateBitCast(TheVal, DstTy);
        }
        I.replaceAllUsesWith(NewPtr);
        I.eraseFromParent();

        return true;
    }

    return false;
}

bool GASPropagator::visitDbgDeclareInst(DbgDeclareInst & I) {
   MetadataAsValue * MAV = MetadataAsValue::get(TheVal->getContext(), ValueAsMetadata::get(TheVal));
#if LLVM_VERSION_MAJOR >= 13
   I.replaceVariableLocationOp(I.getVariableLocationOp(0), MAV);
#else
   I.setArgOperand(0, MAV);
#endif
   return true;
}

bool GASPropagator::visitDbgValueInst(DbgValueInst & I) {
   MetadataAsValue * MAV = MetadataAsValue::get(TheVal->getContext(), ValueAsMetadata::get(TheVal));
#if LLVM_VERSION_MAJOR >= 13
   I.replaceVariableLocationOp(I.getVariableLocationOp(0), MAV);
#else
   I.setArgOperand(0, MAV);
#endif
   return true;
}

bool GASResolving::resolveMemoryFromHost(Function& F) const {
    MetaDataUtils* pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    // skip all non-entry functions
    if (!isEntryFunc(pMdUtils, &F))
        return false;

    // early check in order not to iterate whole function
    if (!checkGenericArguments(F))
        return false;

    SmallVector<StoreInst*, 32> Stores;
    SmallVector<LoadInst*, 32> Loads;
    AliasAnalysis* AA = &getAnalysis<AAResultsWrapperPass>().getAAResults();

    // collect load candidates and in parallel check for unsafe instructions
    // visitor may be a more beautiful way to do this
    bool HasASCast = false; // if there exists addrspace cast from non global/generic space
    bool HasPtoi = false; // if there exists ptrtoint with global/generic space
    for (BasicBlock& B : F) {
        for (Instruction& I : B) {
            if (auto LI = dyn_cast<LoadInst>(&I)) {
                if (isLoadGlobalCandidate(LI)) {
                    Loads.push_back(LI);
                }
            }
            else if (auto CI = dyn_cast<CallInst>(&I)) {
                if (CI->onlyReadsMemory() || CI->onlyAccessesInaccessibleMemory())
                    continue;

                // currently recognize only these ones
                // in fact intrinsics should be marked as read-only
                if (auto II = dyn_cast<IntrinsicInst>(CI)) {
                    if (II->getIntrinsicID() == Intrinsic::lifetime_start ||
                        II->getIntrinsicID() == Intrinsic::lifetime_end)
                        continue;
                }

                // if we have an unsafe call in the kernel, abort
                // to improve we can collect arguments of writing calls as memlocations for alias analysis
                return false;
            }
            else if (auto PI = dyn_cast<PtrToIntInst>(&I)) {
                // if we have a ptrtoint we need to check data flow which we don't want to
                if (PI->getPointerAddressSpace() != ADDRESS_SPACE_GLOBAL &&
                    PI->getPointerAddressSpace() != ADDRESS_SPACE_GENERIC)
                    return false;
                else {
                    HasPtoi = true;
                }

                return false;
            }
            else if (auto AI = dyn_cast<AddrSpaceCastInst>(&I)) {
                if (AI->getSrcAddressSpace() != ADDRESS_SPACE_GLOBAL &&
                    AI->getSrcAddressSpace() != ADDRESS_SPACE_GENERIC) {
                    HasASCast = true;
                }
            }
            else if (auto SI = dyn_cast<StoreInst>(&I)) {
                Value* V = SI->getValueOperand();
                if (isa<PointerType>(V->getType())) {
                    // this store can potentially write non-global pointer to memory
                    Stores.push_back(SI);
                }
            }
            else if (I.mayWriteToMemory()) {
                // unsupported instruction poisoning memory
                return false;
            }
        }
    }
    if (HasASCast && HasPtoi)
        return false;

    if (Loads.empty())
        return false;

    bool Changed = false;
    while (!Loads.empty())
    {
        LoadInst* LI = Loads.pop_back_val();

        // check that we don't have aliasing stores for this load
        // we expect to have basic and addrspace AA available at the moment
        // on optimization phase
        bool aliases = false;
        for (auto SI : Stores) {
            if (AA->alias(MemoryLocation::get(SI), MemoryLocation::get(LI))) {
                aliases = true;
                break;
            }
        }
        if (aliases)
            continue;

        convertLoadToGlobal(LI);
        Changed = true;
    }
    return Changed;
}

bool GASResolving::isLoadGlobalCandidate(LoadInst* LI) const {
    // first check that loaded address has generic address space
    // otherwise it is not our candidate
    PointerType* PtrTy = dyn_cast<PointerType>(LI->getType());
    if (!PtrTy || PtrTy->getAddressSpace() != ADDRESS_SPACE_GENERIC)
        return false;

    // next check that it is a load from function argument + offset
    // which is necessary to prove that this address has global addrspace
    Value* LoadBase = LI->getPointerOperand()->stripInBoundsOffsets();
    // WA for gep not_inbounds base, 0, 0 that is not handled in stripoffsets
    LoadBase = LoadBase->stripPointerCasts();
    if (!isa<Argument>(LoadBase))
        return false;

    // don't want to process cases when argument is from local address space
    auto LoadTy = cast<PointerType>(LoadBase->getType());
    if (LoadTy->getAddressSpace() != ADDRESS_SPACE_GLOBAL)
        return false;

    // TODO: skip cases that have been fixed on previous traversals

    return true;
}

void GASResolving::convertLoadToGlobal(LoadInst* LI) const {
    // create two addressspace casts: generic -> global -> generic
    // the next scalar phase of this pass will propagate global to all uses of the load

    PointerType* PtrTy = cast<PointerType>(LI->getType());
    IRB->SetInsertPoint(LI->getNextNode());
    PointerType* GlobalPtrTy = PointerType::get(PtrTy->getPointerElementType(), ADDRESS_SPACE_GLOBAL);
    Value* GlobalAddr = IRB->CreateAddrSpaceCast(LI, GlobalPtrTy);
    Value* GenericCopyAddr = IRB->CreateAddrSpaceCast(GlobalAddr, PtrTy);

    for (auto UI = LI->use_begin(), UE = LI->use_end(); UI != UE; /*EMPTY*/) {
        Use& U = *UI++;
        if (U.getUser() == GlobalAddr)
            continue;
        U.set(GenericCopyAddr);
    }
}

bool GASResolving::checkGenericArguments(Function& F) const {
    // check that we have a pointer to pointer or pointer to struct that has pointer elements
    // and main pointer type is global while underlying pointer type is generic

    auto* FT = F.getFunctionType();
    for (unsigned p = 0; p < FT->getNumParams(); ++p) {
        if (auto Ty = dyn_cast<PointerType>(FT->getParamType(p))) {
            if (Ty->getAddressSpace() != ADDRESS_SPACE_GLOBAL)
                continue;
            auto PteeTy = Ty->getPointerElementType();
            if (auto PTy = dyn_cast<PointerType>(PteeTy)) {
                if (PTy->getAddressSpace() == ADDRESS_SPACE_GENERIC)
                    return true;
            }
            if (auto STy = dyn_cast<StructType>(PteeTy)) {
                for (unsigned e = 0; e < STy->getNumElements(); ++e) {
                    if (auto ETy = dyn_cast<PointerType>(STy->getElementType(e))) {
                        if (ETy->getAddressSpace() == ADDRESS_SPACE_GENERIC)
                            return true;
                    }
                }
            }
        }
    }
    return false;
}

ModulePass* IGC::createGASRetValuePropagatorPass() { return new GASRetValuePropagator(); }

char GASRetValuePropagator::ID = 0;

#define GAS_RET_PASS_FLAG "igc-gas-ret-value-propagator"
#define GAS_RET_PASS_DESC "Resolve generic pointer return value"
#define GAS_RET_PASS_CFG_ONLY false
#define GAS_RET_PASS_ANALYSIS false

namespace IGC {
    IGC_INITIALIZE_PASS_BEGIN(GASRetValuePropagator, GAS_RET_PASS_FLAG, GAS_RET_PASS_DESC, GAS_RET_PASS_CFG_ONLY,
        GAS_RET_PASS_ANALYSIS)
    IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
    IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
    IGC_INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
    IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
    IGC_INITIALIZE_PASS_END(GASRetValuePropagator, GAS_RET_PASS_FLAG, GAS_RET_PASS_DESC, GAS_RET_PASS_CFG_ONLY,
        GAS_RET_PASS_ANALYSIS)
}

bool GASRetValuePropagator::runOnModule(Module& M) {
    bool changed = false;
    m_module = &M;
    m_mdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    m_ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    CallGraph& CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
    std::vector<Function*> candidates = findCandidates(CG);

    for (auto* F : candidates)
    {
        LoopInfo& LI = getAnalysis<LoopInfoWrapperPass>(*F).getLoopInfo();
        GASPropagator ThePropagator(F->getContext(), &LI);
        m_Propagator = &ThePropagator;

        if (propagateReturnValue(F))
        {
            changed = true;
        }
    }

    return changed;
}

bool GASRetValuePropagator::propagateReturnValue(Function*& F) {
    PointerType* nonGASPtr = getRetValueNonGASType(F);

    if (!nonGASPtr) return false;

    Function* newFunc = cloneFunctionWithModifiedRetType(F, nonGASPtr);

    updateAllUsesWithNewFunction(F, newFunc);

    IGC_ASSERT(nullptr != F);
    IGC_ASSERT_MESSAGE(F->use_empty(), "All function uses should have been transfered to new function");
    F->eraseFromParent();
    F = newFunc;
    return true;
}

std::vector<Function*> GASRetValuePropagator::findCandidates(CallGraph& CG) {
    std::vector<Function*> candidates;

    auto skip = [](Function* F)
    {
        // Skip functions with variable number of arguments, e.g. printf.
        if (F->isVarArg())
            return true;

        // Only non-extern functions within the module are optimized
        if (F->hasFnAttribute("referenced-indirectly") || F->isDeclaration()
            || F->isIntrinsic() || F->user_empty())
            return true;

        return false;
    };

    auto isGenericPtrTy = [](Type* T)
    {
        return T->isPointerTy() && T->getPointerAddressSpace() == ADDRESS_SPACE_GENERIC;
    };

    // Find the candidates, which are functions returning generic pointer args.
    // Functions will be updated later in down-top ordering (starting from most nested function).
    for(auto I : post_order(&CG))
    {
        auto F = I->getFunction();
        if (F == nullptr)
            continue;
        if (skip(F))
            continue;
        if (!isGenericPtrTy(F->getReturnType()))
            continue;

        candidates.push_back(F);
    }

    return candidates;
}

std::vector<ReturnInst*> GASRetValuePropagator::getAllRetInstructions(Function& F)
{
    std::vector<ReturnInst*> retInstructions;
    for (auto& BB : F)
    {
        if (auto retInst = dyn_cast<ReturnInst>(BB.getTerminator()))
        {
            retInstructions.push_back(retInst);
        }
    }
    return retInstructions;
}

PointerType* GASRetValuePropagator::getRetValueNonGASType(Function* F)
{
    std::vector<ReturnInst*> retInstructions = getAllRetInstructions(*F);

    std::optional<unsigned> originAddrSpace = std::nullopt;
    for (auto retInst : retInstructions)
    {
        Value* retValue = retInst->getReturnValue();

        if (isa<ConstantPointerNull>(retValue))
            continue;

        if (!isa<AddrSpaceCastInst>(retValue))
            return nullptr;

        auto I = cast<AddrSpaceCastInst>(retValue);
        IGC_ASSERT(I->getDestAddressSpace() == ADDRESS_SPACE_GENERIC);

        unsigned AS = I->getSrcAddressSpace();
        if (originAddrSpace && originAddrSpace.value() != AS)
            return nullptr;

        originAddrSpace.emplace(AS);
    }

    return originAddrSpace ?
        PointerType::get(F->getReturnType()->getPointerElementType(), originAddrSpace.value()) :
        nullptr;
}

Function* GASRetValuePropagator::createNewFunctionDecl(Function* oldFunc, Type* newRetTy)
{
    Module* M = oldFunc->getParent();
    ArrayRef<Type*> params = oldFunc->getFunctionType()->params();
    FunctionType* newFTy = FunctionType::get(newRetTy, params, oldFunc->isVarArg());

    Function* newFunc = Function::Create(newFTy, oldFunc->getLinkage());
    newFunc->copyAttributesFrom(oldFunc);
    newFunc->setSubprogram(oldFunc->getSubprogram());
    M->getFunctionList().insert(oldFunc->getIterator(), newFunc);
    newFunc->takeName(oldFunc);
    return newFunc;
}

void GASRetValuePropagator::transferFunctionBody(Function* oldFunc, Function* newFunc)
{
    newFunc->stealArgumentListFrom(*oldFunc);
    newFunc->getBasicBlockList().splice(newFunc->begin(), oldFunc->getBasicBlockList());
}

void GASRetValuePropagator::updateFunctionRetInstruction(Function* F)
{
    std::vector<ReturnInst*> retInstructions = getAllRetInstructions(*F);

    for (auto retInst : retInstructions)
    {
        Value* retValue = retInst->getReturnValue();

        if (isa<ConstantPointerNull>(retValue))
        {
            retInst->setOperand(0, ConstantPointerNull::get(cast<PointerType>(F->getReturnType())));
            continue;
        }

        IGC_ASSERT(isa<AddrSpaceCastInst>(retValue));

        auto ASC = cast<AddrSpaceCastInst>(retValue);
        IGC_ASSERT(ASC->getDestAddressSpace() == ADDRESS_SPACE_GENERIC);

        retInst->setOperand(0, ASC->getPointerOperand());

        if (ASC->use_empty()) ASC->eraseFromParent();
    }
}

void GASRetValuePropagator::updateAllUsesWithNewFunction(Function* oldFunc, Function* newFunc)
{
    IGC_ASSERT(!oldFunc->use_empty());

    // Keep track of old calls and addrspacecast to be deleted later
    std::vector<CallInst*> callsToDelete;

    for( auto U : oldFunc->users())
    {
        CallInst* cInst = dyn_cast<CallInst>(U);
        if (!cInst)
        {
            IGC_ASSERT_MESSAGE(0, "Unknown function usage");
            return;
        }

        // Prepare args for new call
        std::vector<Value*> callArgs;
        for (unsigned I = 0, E = IGCLLVM::getNumArgOperands(cInst); I != E; ++I) {
            callArgs.push_back(cInst->getArgOperand(I));
        }

        // Create new call and insert it before old one
        CallInst* newCall = CallInst::Create(newFunc, callArgs, "", cInst);

        newCall->setCallingConv(newFunc->getCallingConv());
        newCall->setAttributes(cInst->getAttributes());
        newCall->setDebugLoc(cInst->getDebugLoc());

        IGC_ASSERT(oldFunc->getType()->isPointerTy() &&
            newFunc->getReturnType()->isPointerTy());

        auto* oldRetTy = dyn_cast<PointerType>(oldFunc->getReturnType());
        auto* newRetTy = dyn_cast<PointerType>(newFunc->getReturnType());

        IGC_ASSERT(
            oldRetTy->getAddressSpace() == ADDRESS_SPACE_GENERIC &&
            newRetTy->getAddressSpace() != ADDRESS_SPACE_GENERIC);

        auto ASC = CastInst::Create(Instruction::AddrSpaceCast, newCall, oldFunc->getReturnType(), "", cInst);

        cInst->replaceAllUsesWith(ASC);
        callsToDelete.push_back(cInst);

        m_Propagator->propagate(newCall);
    }

    // Delete old calls
    for (auto call : callsToDelete)
    {
        call->eraseFromParent();
    }
}

bool GASPropagator::propagateToAllUsers(AddrSpaceCastInst* I)
{
    // Since %49 is used twice in a phi instruction like the one below:
    // %56 = phi %"class.someclass" addrspace(4)* [ %49, %53 ], [ %49, %742 ]
    // the use iterator was handling such phi instructions twice.
    // This was causing a crash since propagate function might erase instructions.
    SmallPtrSet<Instruction*, 8> InstSet;
    SmallVector<Use*, 8> Uses;
    for (auto UI = I->use_begin(), UE = I->use_end(); UI != UE; ++UI) {
        Use* U = &(*UI);
        Instruction* I = cast<Instruction>(U->getUser());
        if (InstSet.insert(I).second) {
            Uses.push_back(U);
        }
    }

    if (auto* L = LocalAsMetadata::getIfExists(I))
        if (auto* MDV = MetadataAsValue::getIfExists(I->getContext(), L))
            for (auto& Use : MDV->uses())
                Uses.push_back(&Use);

    bool Changed = false;
    // Propagate that source through all users of this cast.
    for (Use* U : Uses) {
        Changed |= propagateToUser(U, I->getOperand(0));
    }
    return Changed;
}

void GASPropagator::propagate(Value* I)
{
    PointerType* ptrTy = dyn_cast<PointerType>(I->getType());

    if (!ptrTy)
        return;

    // propagate only non generic pointers
    if (ptrTy->getAddressSpace() == ADDRESS_SPACE_GENERIC)
        return;

    SmallVector<AddrSpaceCastInst*, 8> addrSpaceCastsToResolve;
    for (User* user : I->users())
        if (auto* addrSpaceCast = dyn_cast<AddrSpaceCastInst>(user))
            if (addrSpaceCast->getDestAddressSpace() == ADDRESS_SPACE_GENERIC)
                addrSpaceCastsToResolve.push_back(addrSpaceCast);

    bool propagated = false;
    for (AddrSpaceCastInst* addrSpaceCast : addrSpaceCastsToResolve)
    {
        propagated |= propagateToAllUsers(addrSpaceCast);

        if (addrSpaceCast->use_empty())
            addrSpaceCast->eraseFromParent();
    }

    if (!propagated)
        return;

    // continue propagation through instructions that may return a pointer
    for (auto user : I->users())
    {
        if (Instruction* userInst = dyn_cast<Instruction>(user))
        {
            switch (userInst->getOpcode())
            {
            case Instruction::PHI:
            case Instruction::GetElementPtr:
            case Instruction::Select:
            case Instruction::BitCast:
                propagate(userInst);
            }
        }
    }
}

Function* GASRetValuePropagator::cloneFunctionWithModifiedRetType(Function* F, PointerType* newRetTy)
{
    Function* newFunc = createNewFunctionDecl(F, newRetTy);
    transferFunctionBody(F, newFunc);
    updateFunctionRetInstruction(newFunc);
    updateMetadata(F, newFunc);
    return newFunc;
}

void GASRetValuePropagator::updateMetadata(Function* oldFunc, Function* newFunc) {
    MetadataBuilder mbuilder(m_module);
    auto& FuncMD = m_ctx->getModuleMetaData()->FuncMD;

    auto oldFuncIter = m_mdUtils->findFunctionsInfoItem(oldFunc);
    m_mdUtils->setFunctionsInfoItem(newFunc, oldFuncIter->second);
    m_mdUtils->eraseFunctionsInfoItem(oldFuncIter);
    mbuilder.UpdateShadingRate(oldFunc, newFunc);
    auto loc = FuncMD.find(oldFunc);
    if (loc != FuncMD.end())
    {
        auto funcInfo = loc->second;
        FuncMD.erase(oldFunc);
        FuncMD[newFunc] = funcInfo;
    }

    m_mdUtils->save(m_module->getContext());
}

namespace IGC
{
    //
    // (1)
    // Optimization pass to lower generic pointers in function arguments.
    // If all call sites have the same origin address space, address space
    // casts with the form of non-generic->generic can safely removed and
    // function updated with non-generic pointer argument.
    //
    // The complete process to lower generic pointer args consists of 5 steps:
    //   1) find all functions that are candidates
    //   2) update functions and their signatures
    //   3) update all call sites
    //   4) update functions metadata
    //   5) validate that all function calls are properly formed
    //
    //
    // Current limitations/considerations:
    // - only arguments of non-extern functions can be lowered
    // - no recursive functions support
    //
    // (2)
    //   Once (1) is done. Do further check if there is a cast from local to GAS or
    //   a cast from private to GAS. If there is no such cast, GAS inst (such as
    //   ld/st, etc, can be converted safely to ld/st on globals.
    class LowerGPCallArg : public ModulePass
    {
    public:
        static char ID;

        LowerGPCallArg() : ModulePass(ID)
        {
            initializeLowerGPCallArgPass(*PassRegistry::getPassRegistry());
        }

        bool runOnModule(Module&) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CallGraphWrapperPass>();
            AU.addRequired<LoopInfoWrapperPass>();
            AU.addRequired<CastToGASWrapperPass>();
        }

        virtual StringRef getPassName() const override
        {
            return "LowerGenericPointerCallArgs";
        }
    private:

        //
        // Functions to be updated.
        // NewArgs keeps track of generic pointer arguments: arg number and address space
        //
        struct ArgDesc {
            unsigned int argNo;
            unsigned int addrSpace;
        };
        using GenericPointerArgs = std::vector<ArgDesc>;

        IGCMD::MetaDataUtils* m_mdUtils = nullptr;
        CodeGenContext* m_ctx = nullptr;
        Module* m_module = nullptr;
        GASInfo* m_GI = nullptr;
        bool m_changed = false;

        void processCallArg(Module& M);
        void processGASInst(Module& M);

        std::optional<unsigned> getOriginAddressSpace(Function* func, unsigned argNo);
        void updateFunctionArgs(Function* oldFunc, Function* newFunc);
        void updateAllUsesWithNewFunction(Function* oldFunc, Function* newFunc);
        void updateMetadata(Function* oldFunc, Function* newFunc);
        Function* createFuncWithLoweredArgs(Function* F, GenericPointerArgs& argsInfo);
        std::vector<Function*> findCandidates(CallGraph& CG);
    };
} // End anonymous namespace

ModulePass* IGC::createLowerGPCallArg() { return new LowerGPCallArg(); }

char LowerGPCallArg::ID = 0;

#define GP_PASS_FLAG "igc-lower-gp-arg"
#define GP_PASS_DESC "Lower generic pointers in call arguments"
#define GP_PASS_CFG_ONLY false
#define GP_PASS_ANALYSIS false
namespace IGC
{
    IGC_INITIALIZE_PASS_BEGIN(LowerGPCallArg, GP_PASS_FLAG, GP_PASS_DESC, GP_PASS_CFG_ONLY, GP_PASS_ANALYSIS)
    IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
    IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
    IGC_INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
    IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
    IGC_INITIALIZE_PASS_DEPENDENCY(CastToGASWrapperPass)
    IGC_INITIALIZE_PASS_END(LowerGPCallArg, GP_PASS_FLAG, GP_PASS_DESC, GP_PASS_CFG_ONLY, GP_PASS_ANALYSIS)
}

bool LowerGPCallArg::runOnModule(llvm::Module& M)
{
    m_ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    m_mdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    m_GI = &getAnalysis<CastToGASWrapperPass>().getGASInfo();
    m_module = &M;

    // (1) main work
    processCallArg(M);

    // (2) further static resolution
    processGASInst(M);

    return m_changed;
}

void LowerGPCallArg::processCallArg(Module& M)
{
    CallGraph& CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
    std::vector<Function*> candidates = findCandidates(CG);
    for (auto F : reverse(candidates))
    {
        GenericPointerArgs genericArgsInfo;
        for (auto& arg : F->args())
        {
            if (arg.use_empty())
                continue;

            Type* argTy = arg.getType();
            if (argTy->isPointerTy() && argTy->getPointerAddressSpace() == ADDRESS_SPACE_GENERIC)
            {
                if (auto originAddrSpace = getOriginAddressSpace(F, arg.getArgNo()))
                    genericArgsInfo.push_back({ arg.getArgNo(), originAddrSpace.value()});
            }
        }

        if (genericArgsInfo.empty())
            continue;

        Function* newFunc = createFuncWithLoweredArgs(F, genericArgsInfo);
        updateFunctionArgs(F, newFunc);
        updateAllUsesWithNewFunction(F, newFunc);
        updateMetadata(F, newFunc);

        F->eraseFromParent();
        m_changed = true;
    }
}

void LowerGPCallArg::processGASInst(Module& M)
{
    // As AddrSpaceCast has been processed already in GASResolving,
    // here only handle non-addrspacecast ptr
    auto toSkip = [](Value* P) {
        if (PointerType* PtrTy = dyn_cast<PointerType>(P->getType()))
        {
            if (PtrTy->getAddressSpace() == ADDRESS_SPACE_GENERIC && !isa<AddrSpaceCastInst>(P))
            {
                return false;
            }
        }
        return true;
    };

    IRBuilder<> IRB(M.getContext());
    // Change GAS inst, such as ld/st, etc to global ld/st, etc.
    for (Function& F : M)
    {
        if (m_GI->canGenericPointToPrivate(F) || m_GI->canGenericPointToLocal(F))
            continue;

        auto NI = inst_begin(F);
        for (auto FI = NI, FE = inst_end(F); FI != FE; FI = NI)
        {
            ++NI;

            Instruction* I = &(*FI);
            LoadInst* LI = dyn_cast<LoadInst>(I);
            StoreInst* SI = dyn_cast<StoreInst>(I);
            if (LI || SI)
            {
                Value* Ptr = LI ? LI->getPointerOperand() : SI->getPointerOperand();
                if (!toSkip(Ptr))
                {
                    PointerType* PtrTy = cast<PointerType>(Ptr->getType());
                    Type* eltTy = PtrTy->getPointerElementType();
                    PointerType* glbPtrTy = PointerType::get(eltTy, ADDRESS_SPACE_GLOBAL);

                    IRB.SetInsertPoint(I);
                    Value* NewPtr = IRB.CreateAddrSpaceCast(Ptr, glbPtrTy);
                    I->setOperand(LI ? 0 : 1, NewPtr);
                    if (Instruction* tI = dyn_cast<Instruction>(NewPtr))
                    {
                        tI->setDebugLoc(I->getDebugLoc());
                    }

                    m_changed = true;
                }
            }
            else if (CallInst* CallI = dyn_cast<CallInst>(I))
            {
                Function* Callee = CallI->getCalledFunction();
                if (Callee &&
                    (Callee->getName().equals("__builtin_IB_to_local") ||
                     Callee->getName().equals("__builtin_IB_to_private")) &&
                    !toSkip(CallI->getOperand(0)))
                {
                    Type* DstTy = I->getType();
                    Value* NewPtr = Constant::getNullValue(DstTy);
                    I->replaceAllUsesWith(NewPtr);
                    I->eraseFromParent();

                    m_changed = true;
                }
            }
        }
    }
}

std::vector<Function*> LowerGPCallArg::findCandidates(CallGraph& CG)
{
    auto skip = [](Function* F)
    {
        // Skip functions with variable number of arguments, e.g. printf.
        if (F->isVarArg())
            return true;

        // Only non-extern functions within the module are optimized
        if (F->hasFnAttribute("referenced-indirectly") || F->isDeclaration()
            || F->isIntrinsic() || F->user_empty())
            return true;

        return false;
    };

    std::vector<Function*> candidates;
    for (auto I : post_order(&CG))
    {
        auto F = I->getFunction();
        if (!F)
            continue;
        if (skip(F))
            continue;

        auto hasGenericArg = [](Argument& arg) {
            Type* argTy = arg.getType();
            return argTy->isPointerTy() && argTy->getPointerAddressSpace() == ADDRESS_SPACE_GENERIC;
        };

        if (std::any_of(F->arg_begin(), F->arg_end(), hasGenericArg))
            candidates.push_back(F);
    }

    return candidates;
}

void LowerGPCallArg::updateMetadata(Function* oldFunc, Function* newFunc) {
    MetadataBuilder mbuilder(m_module);
    auto& FuncMD = m_ctx->getModuleMetaData()->FuncMD;

    auto oldFuncIter = m_mdUtils->findFunctionsInfoItem(oldFunc);
    m_mdUtils->setFunctionsInfoItem(newFunc, oldFuncIter->second);
    m_mdUtils->eraseFunctionsInfoItem(oldFuncIter);
    mbuilder.UpdateShadingRate(oldFunc, newFunc);
    auto loc = FuncMD.find(oldFunc);
    if (loc != FuncMD.end())
    {
        auto funcInfo = loc->second;
        FuncMD.erase(oldFunc);
        FuncMD[newFunc] = funcInfo;
    }

    m_mdUtils->save(m_module->getContext());
}

Function* LowerGPCallArg::createFuncWithLoweredArgs(Function* F, GenericPointerArgs& argsInfo)
{
    FunctionType* pFuncType = F->getFunctionType();
    std::vector<Type*> newParamTypes(pFuncType->param_begin(), pFuncType->param_end());
    for (auto& argInfo : argsInfo)
    {
        PointerType* ptrType = PointerType::get(newParamTypes[argInfo.argNo]->getPointerElementType(),
            argInfo.addrSpace);
        newParamTypes[argInfo.argNo] = ptrType;
    }

    FunctionType* newFTy = FunctionType::get(F->getReturnType(), newParamTypes, F->isVarArg());
    Function* newFunc = Function::Create(newFTy, F->getLinkage());
    newFunc->copyAttributesFrom(F);
    newFunc->setSubprogram(F->getSubprogram());
    m_module->getFunctionList().insert(F->getIterator(), newFunc);
    newFunc->takeName(F);
    newFunc->getBasicBlockList().splice(newFunc->begin(), F->getBasicBlockList());

    return newFunc;
}

std::optional<unsigned> LowerGPCallArg::getOriginAddressSpace(Function* func, unsigned argNo)
{
    std::optional<unsigned> originAddressSpace;

    // Check if all the callers have the same pointer address space
    for (auto U : func->users())
    {
        auto CI = cast<CallInst>(U);
        Value* V = CI->getArgOperand(argNo);

        if (!V->getType()->isPointerTy())
            continue;

        if (AddrSpaceCastInst* ASC = dyn_cast<AddrSpaceCastInst>(V))
        {
            IGC_ASSERT(ASC->getDestAddressSpace() == ADDRESS_SPACE_GENERIC);

            unsigned srcAddrSpace = ASC->getSrcAddressSpace();
            if (originAddressSpace && originAddressSpace.value() != srcAddrSpace)
                return std::nullopt;

            originAddressSpace = srcAddrSpace;
        }
        else
        {
            return std::nullopt;
        }
    }

    return originAddressSpace;
}

// Loops over the argument list transferring uses from old function to new one.
void LowerGPCallArg::updateFunctionArgs(Function* oldFunc, Function* newFunc)
{
    for (auto ArgPair : llvm::zip(oldFunc->args(), newFunc->args()))
    {
        Value* oldArg = &std::get<0>(ArgPair);
        Value* newArg = &std::get<1>(ArgPair);

        newArg->takeName(oldArg);

        if (oldArg->getType() == newArg->getType())
        {
            oldArg->replaceAllUsesWith(newArg);
            continue;
        }

        auto* NewArgToGeneric = CastInst::Create(
            Instruction::AddrSpaceCast, newArg, oldArg->getType(), "", newFunc->getEntryBlock().getFirstNonPHI());
        oldArg->replaceAllUsesWith(NewArgToGeneric);

        LoopInfo& LI = getAnalysis<LoopInfoWrapperPass>(*newFunc).getLoopInfo();
        GASPropagator Propagator(newFunc->getContext(), &LI);
        Propagator.propagate(newArg);
    }
}

// This function takes an Old value and a New value. If Old value is referenced in a
// dbg.value or dbg.declare instruction, it replaces that intrinsic and makes new one
// use the New value.
//
// This function is required anytime a pass modifies IR such that RAUW cannot be
// used to directly update uses in metadata node. In case of GAS, RAUW asserts because
// addrspace used in Old/New values are different and this is interpreted as different
// types by LLVM and RAUW on different types is forbidden.
void replaceValueInDbgInfoIntrinsic(llvm::Value* Old, llvm::Value* New, llvm::Module& M)
{
    if (Old->isUsedByMetadata())
    {
        auto localAsMD = ValueAsMetadata::getIfExists(Old);
        auto addrSpaceMD = MetadataAsValue::getIfExists(Old->getContext(), localAsMD);
        if (addrSpaceMD)
        {
            llvm::DIBuilder DIB(M);
            std::vector<llvm::DbgInfoIntrinsic*> DbgInfoInstToDelete;
            for (auto* User : addrSpaceMD->users())
            {
                if (cast<DbgInfoIntrinsic>(User))
                {
                    //User->dump();
                    if (auto DbgV = cast<DbgValueInst>(User))
                    {
                        DIB.insertDbgValueIntrinsic(New,
                            DbgV->getVariable(), DbgV->getExpression(), DbgV->getDebugLoc().get(),
                            cast<llvm::Instruction>(User));
                    }
                    else if (auto DbgD = cast<DbgDeclareInst>(User))
                    {
                        DIB.insertDeclare(New,
                            DbgD->getVariable(), DbgD->getExpression(), DbgD->getDebugLoc().get(),
                            cast<llvm::Instruction>(User));
                    }
                    DbgInfoInstToDelete.push_back(cast<llvm::DbgInfoIntrinsic>(User));
                }
            }

            for (auto DbgInfoInst : DbgInfoInstToDelete)
                DbgInfoInst->eraseFromParent();
        }
    }
}

void LowerGPCallArg::updateAllUsesWithNewFunction(Function* oldFunc, Function* newFunc)
{
    IGC_ASSERT(!oldFunc->use_empty());

    // Keep track of old calls and addrspacecast to be deleted later
    std::vector<CallInst*> callsToDelete;
    std::vector<AddrSpaceCastInst*> ASCToDelete;

    for (auto U = oldFunc->user_begin(), E = oldFunc->user_end(); U != E; ++U)
    {
        CallInst* cInst = dyn_cast<CallInst>(*U);
        auto BC = dyn_cast<BitCastInst>(*U);
        if (BC && BC->hasOneUse())
            cInst = dyn_cast<CallInst>(BC->user_back());
        if (!cInst)
        {
            IGC_ASSERT_MESSAGE(0, "Unknown function usage");
            return;
        }

        // Prepare args for new call
        std::vector<Value*> newCallArgs;

        auto AI = newFunc->arg_begin();
        for (unsigned int i = 0; i < IGCLLVM::getNumArgOperands(cInst); ++i, ++AI)
        {
            Value* callArg = cInst->getOperand(i);
            Value* funcArg = AI;
            if (callArg->getType() != funcArg->getType())
            {
                IGC_ASSERT(callArg->getType()->isPointerTy() &&
                    funcArg->getType()->isPointerTy());

                PointerType* callArgTy = dyn_cast<PointerType>(callArg->getType());
                PointerType* funcArgTy = dyn_cast<PointerType>(funcArg->getType());
                IGC_ASSERT(
                    callArgTy->getAddressSpace() == ADDRESS_SPACE_GENERIC &&
                    funcArgTy->getAddressSpace() != ADDRESS_SPACE_GENERIC);
                // If call site address space is generic and function arg is non-generic,
                // the addrspacecast is removed and non-generic address space lowered
                // to the function call.
                AddrSpaceCastInst* addrSpaceCastInst = dyn_cast<AddrSpaceCastInst>(callArg);
                if (addrSpaceCastInst)
                {
                    callArg = addrSpaceCastInst->getOperand(0);
                    if (addrSpaceCastInst->hasOneUse())
                    {
                        // when addrspacecast is used in a metadata node, replacing it
                        // requires reconstruction of the node. we cannot used standard
                        // llvm APIs to replace uses as they require that type be
                        // preserved, which is not in this case.
                        replaceValueInDbgInfoIntrinsic(addrSpaceCastInst, addrSpaceCastInst->getPointerOperand(),
                            *newFunc->getParent());
                        ASCToDelete.push_back(addrSpaceCastInst);
                    }
                }
            }
            newCallArgs.push_back(callArg);
        }

        // Create new call and insert it before old one
        CallInst* inst = CallInst::Create(newFunc, newCallArgs,
            newFunc->getReturnType()->isVoidTy() ? "" : newFunc->getName(),
            cInst);

        inst->setCallingConv(newFunc->getCallingConv());
        inst->setDebugLoc(cInst->getDebugLoc());
        cInst->replaceAllUsesWith(inst);
        callsToDelete.push_back(cInst);
    }

    // Delete old calls
    for (auto i : callsToDelete)
    {
        i->eraseFromParent();
    }

    // Delete addrspacecasts that are no longer needed
    for (auto i : ASCToDelete)
    {
        IGC_ASSERT(i->user_empty());
        i->eraseFromParent();
    }
}
