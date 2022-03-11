/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#define DEBUG_TYPE "gas-resolver"
#include "Compiler/CISACodeGen/ResolveGAS.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
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
    };

    class GASPropagator : public InstVisitor<GASPropagator, bool> {
        friend class InstVisitor<GASPropagator, bool>;

        LoopInfo* const LI;
        BuilderType* const IRB;

        Use* TheUse;
        Value* TheVal;

        // Phi node being able to be resolved from its initial value.
        DenseSet<PHINode*> ResolvableLoopPHIs;

    public:
        GASPropagator(BuilderType* Builder, LoopInfo* LoopInfo)
            : IRB(Builder), LI(LoopInfo), TheUse(nullptr), TheVal(nullptr) {
            populateResolvableLoopPHIs();
        }

        bool propagate(Use* U, Value* V) {
            TheUse = U;
            TheVal = V;
            Instruction* I = cast<Instruction>(U->getUser());
            return visit(*I);
        }

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
        void propagateNonGASPointer(Instruction* I);
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
    BuilderType TheBuilder(F.getContext());
    LoopInfo& LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    GASPropagator ThePropagator(&TheBuilder, &LI);
    IRB = &TheBuilder;
    Propagator = &ThePropagator;

    resolveMemoryFromHost(F);

    bool Changed = false;
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
        Type* DstTy = DstPtrTy->getElementType();
        Value* Src = CI->getOperand(0);
        PointerType* SrcPtrTy = cast<PointerType>(Src->getType());
        // Canonicalize the addrspace cast by separating out the type casting if
        // any.
        if (SrcPtrTy->getElementType() != DstTy) {
            BuilderType::InsertPointGuard Guard(*IRB);
            // Transform the following cast
            //
            //  addrspacecast SrcTy addrspace(S)* to DstTy addrspace(T)*
            //
            // to
            //  bitcast SrcTy addrspace(S)* to DstTy addrspace(S)*
            //  addrspacecast DstTy addrspace(S)* to DstTy addrspace(T)*
            //
            PointerType* TransPtrTy =
                PointerType::get(DstTy, SrcPtrTy->getAddressSpace());

            IRB->SetInsertPoint(CI);
            // Update source.
            Src = IRB->CreateBitCast(Src, TransPtrTy);
            SrcPtrTy = cast<PointerType>(Src->getType());
            CI->setOperand(0, Src);
            Changed = true;
        }

        llvm::Module* ModuleMeta = BB->getModule();

        if (CI->isUsedByMetadata()) {
            auto* L = LocalAsMetadata::getIfExists(CI);
            if (L) {
                auto* MDV = MetadataAsValue::getIfExists(ModuleMeta->getContext(), L);
                if (MDV) {
                    SmallPtrSet<User*, 4> Users(MDV->users().begin(), MDV->users().end());
                    for (auto U : Users) {
                        if (isa<DbgDeclareInst>(cast<Value>(U)) || isa<DbgValueInst>(cast<Value>(U))) {
                            CallInst* CallI = dyn_cast_or_null<CallInst>(cast<Value>(U));
                            if (CallI) {
                                MetadataAsValue* MAV = MetadataAsValue::get(CI->getContext(), ValueAsMetadata::get(Src));
                                CallI->setArgOperand(0, MAV);
                                Changed = true;
                            }
                            else {
                                IGC_ASSERT_MESSAGE(false, "Unexpected instruction");
                            }
                        }
                        else {
                            IGC_ASSERT_MESSAGE(false, "Unexpected user");
                        }
                    }
                }
            }
        }

        // Since %49 is used twice in a phi instruction like the one below:
        // %56 = phi %"class.someclass" addrspace(4)* [ %49, %53 ], [ %49, %742 ]
        // the use iterator was handling such phi instructions twice.
        // This was causing a crash since propagate function might erase instructions.
        SmallPtrSet<Instruction*, 8> InstSet;
        SmallVector<Use*, 8> Uses;
        for (auto UI = CI->use_begin(), UE = CI->use_end(); UI != UE; ++UI) {
            Use* U = &(*UI);
            Instruction* I = cast<Instruction>(U->getUser());
            if (InstSet.insert(I).second) {
                Uses.push_back(U);
            }
        }
        // Propagate that source through all users of this cast.
        for (Use* U : Uses) {
            Changed |= Propagator->propagate(U, Src);
        }
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
    if (SrcPtrTy->getElementType() != DstPtrTy->getElementType()) {
        BuilderType::InsertPointGuard Guard(*IRB);
        IRB->SetInsertPoint(&I);
        Src = IRB->CreateBitCast(Src, DstPtrTy);
    }
    I.replaceAllUsesWith(Src);
    I.eraseFromParent();

    return true;
}

bool GASPropagator::visitBitCastInst(BitCastInst& I) {
    PointerType* SrcPtrTy = cast<PointerType>(TheVal->getType());
    PointerType* DstPtrTy = cast<PointerType>(I.getType());

    BuilderType::InsertPointGuard Guard(*IRB);
    IRB->SetInsertPoint(&(*std::next(BasicBlock::iterator(&I))));
    // Push `addrspacecast` forward by replacing this `bitcast` on GAS with the
    // one on non-GAS followed by a new `addrspacecast` to GAS.
    Type* DstTy = DstPtrTy->getElementType();
    PointerType* TransPtrTy =
        PointerType::get(DstTy, SrcPtrTy->getAddressSpace());
    Value* Src = TheVal;
    if (SrcPtrTy->getElementType() != DstTy)
        Src = IRB->CreateBitCast(Src, TransPtrTy);
    Value* NewPtr = IRB->CreateAddrSpaceCast(Src, DstPtrTy);
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

    BuilderType::InsertPointGuard Guard(*IRB);
    IRB->SetInsertPoint(&(*std::next(BasicBlock::iterator(&I))));
    // Push `getelementptr` forward by replacing this `bitcast` on GAS with the
    // one on non-GAS followed by a new `addrspacecast` to GAS.
    Type* DstTy = DstPtrTy->getElementType();
    PointerType* TransPtrTy =
        PointerType::get(DstTy, SrcPtrTy->getAddressSpace());
    TheUse->set(TheVal);
    I.mutateType(TransPtrTy);
    Value* NewPtr = IRB->CreateAddrSpaceCast(&I, DstPtrTy);
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
            BuilderType::InsertPointGuard Guard(*IRB);
            IRB->SetInsertPoint(&(*std::next(BasicBlock::iterator(I))));

            NewIncomingValues[i] = IRB->CreateAddrSpaceCast(I, NonGASTy);
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

            AddrSpaceCastInst* ASCI = dyn_cast<AddrSpaceCastInst>(V);
            if (!ASCI || ASCI->getSrcTy() != NonGASTy)
                return false;

            NewIncomingValues[i] = ASCI->getOperand(0);
        }
    }

    // Propagate this phi node.
    PHINode* NewPN = PHINode::Create(NonGASTy, e, "", &PN);
    for (unsigned i = 0; i != e; ++i)
        NewPN->addIncoming(NewIncomingValues[i], PN.getIncomingBlock(i));
    NewPN->takeName(&PN);
    NewPN->setDebugLoc(PN.getDebugLoc());

    BuilderType::InsertPointGuard Guard(*IRB);
    IRB->SetInsertPoint(PN.getParent()->getFirstNonPHI());
    Value* NewPtr = IRB->CreateAddrSpaceCast(NewPN, GASTy);
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

    AddrSpaceCastInst* ASCI = dyn_cast<AddrSpaceCastInst>(TheOtherUse->get());
    if (!ASCI || ASCI->getSrcTy() != NonGASTy)
        return false;

    // Change select operands to non-GAS
    TheUse->set(TheVal);
    TheOtherUse->set(ASCI->getOperand(0));

    // Handle select return type
    BuilderType::InsertPointGuard Guard(*IRB);
    IRB->SetInsertPoint(&(*std::next(BasicBlock::iterator(&I))));

    PointerType* DstPtrTy = cast<PointerType>(I.getType());
    PointerType* NonGASPtrTy = dyn_cast<PointerType>(NonGASTy);

    // Push 'addrspacecast' forward by changing the select return type to non-GAS pointer
    // followed by a new 'addrspacecast' to GAS
    PointerType* TransPtrTy = PointerType::get(DstPtrTy->getElementType(), NonGASPtrTy->getAddressSpace());
    I.mutateType(TransPtrTy);
    Value* NewPtr = IRB->CreateAddrSpaceCast(&I, DstPtrTy);

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
            BuilderType::InsertPointGuard Guard(*IRB);
            IRB->SetInsertPoint(&I);
            NewPtr = IRB->CreateBitCast(TheVal, DstTy);
        }
        I.replaceAllUsesWith(NewPtr);
        I.eraseFromParent();

        return true;
    }

    if (Callee->getName().equals("__builtin_IB_to_private")) {
        Type* DstTy = I.getType();
        Value* NewPtr = Constant::getNullValue(DstTy);
        if (SrcPtrTy->getAddressSpace() == ADDRESS_SPACE_PRIVATE) {
            BuilderType::InsertPointGuard Guard(*IRB);
            IRB->SetInsertPoint(&I);
            NewPtr = IRB->CreateBitCast(TheVal, DstTy);
        }
        I.replaceAllUsesWith(NewPtr);
        I.eraseFromParent();

        return true;
    }

    return false;
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
    PointerType* GlobalPtrTy = PointerType::get(PtrTy->getElementType(), ADDRESS_SPACE_GLOBAL);
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
            auto PteeTy = Ty->getElementType();
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
        BuilderType TheBuilder(F->getContext());
        LoopInfo& LI = getAnalysis<LoopInfoWrapperPass>(*F).getLoopInfo();
        GASPropagator ThePropagator(&TheBuilder, &LI);
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
        for (unsigned I = 0, E = cInst->getNumArgOperands(); I != E; ++I) {
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

        propagateNonGASPointer(newCall);
    }

    // Delete old calls
    for (auto call : callsToDelete)
    {
        call->eraseFromParent();
    }
}

void GASRetValuePropagator::propagateNonGASPointer(Instruction* I)
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
        SmallVector<Use*, 8> Uses;
        std::transform(addrSpaceCast->use_begin(), addrSpaceCast->use_end(), std::back_inserter(Uses),
            [](llvm::Use& use) -> llvm::Use* { return &use; });

        for(Use* use : Uses)
            propagated |= m_Propagator->propagate(use, I);

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
                propagateNonGASPointer(userInst);
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
        using GenericPointerArgs = std::vector<std::pair<unsigned int, unsigned int>>;
        struct FuncToUpdate
        {
            Function* oldFunc;
            GenericPointerArgs newArgs;
            Function* newFunc = nullptr;

            FuncToUpdate(Function* f, const GenericPointerArgs& args) : oldFunc(f), newArgs(args)
            {
            }
        };

        IGCMD::MetaDataUtils* m_mdUtils = nullptr;
        CodeGenContext* m_ctx = nullptr;
        std::vector<Instruction*> m_partiallyLoweredInsts;

        bool hasSameOriginAddressSpace(Function* func, unsigned argNo, unsigned& addrSpaceCallSite);
        void updateFunctionArgs(Function* oldFunc, Function* newFunc, GenericPointerArgs& newArgs);
        void updateAllUsesWithNewFunction(FuncToUpdate& f);
        void fixAddressSpaceInAllUses(Value* ptr, uint newAS, uint oldAS, AddrSpaceCastInst* recoverASC);
        bool processCallArg(Module& M);

        bool processGASInst(Module& M);
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
    IGC_INITIALIZE_PASS_END(LowerGPCallArg, GP_PASS_FLAG, GP_PASS_DESC, GP_PASS_CFG_ONLY, GP_PASS_ANALYSIS)
}

bool LowerGPCallArg::runOnModule(llvm::Module& M)
{
    m_ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    m_mdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    bool changed = false;

    // (1) main work
    if (processCallArg(M))
        changed = true;

    // (2) further static resolution
    if (processGASInst(M))
        changed = true;
    return changed;
}

bool LowerGPCallArg::processCallArg(Module& M)
{
    CallGraph& CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();

    std::vector<FuncToUpdate> funcsToUpdate;

    auto skip = [](Function* F)
    {
        // Skip functions with variable number of arguments, e.g. printf.
        if (F->isVarArg())
            return true;

        // Only non-extern functions within the module are optimized
        if (F->hasFnAttribute("referenced-indirectly") || F->isDeclaration()
            || F->isIntrinsic() || F->user_empty())
            return true;

        // Skip functions that return generic pointer for now
        PointerType* returnPointerType = dyn_cast<PointerType>(F->getReturnType());
        if (returnPointerType && returnPointerType->getAddressSpace() == ADDRESS_SPACE_GENERIC)
            return true;

        return false;
    };

    // Step 1: find the candidates, which are functions with generic pointer args.
    // Functions will be updated later in topological ordering (top-down calls).
    for (auto I = po_begin(CG.getExternalCallingNode()), E = po_end(CG.getExternalCallingNode()); I != E; ++I)
    {
        auto CGNode = *I;
        if (auto F = CGNode->getFunction())
        {
            // Skip external and indirect nodes.
            if (skip(F)) continue;

            // To store <argNo, addressSpace> for each generic pointer argument
            GenericPointerArgs genericPointerArgs;

            // Look for function arguments that are generic pointers
            for (auto& arg : F->args())
            {
                if (arg.use_empty() || !arg.getType()->isPointerTy())
                    continue;
                PointerType* argPointerType = dyn_cast<PointerType>(arg.getType());
                if (argPointerType && argPointerType->getAddressSpace() == ADDRESS_SPACE_GENERIC)
                    genericPointerArgs.push_back(std::make_pair(arg.getArgNo(), ADDRESS_SPACE_GENERIC));
            }

            if (!genericPointerArgs.empty())
            {
                funcsToUpdate.push_back(FuncToUpdate(F, genericPointerArgs));
            }
        }
    }

    // If there are no functions to update, finish
    if (funcsToUpdate.empty())
    {
        return false;
    }

    // Step 2: update functions and lower their generic pointer arguments
    // to their non-generic address space.
    for (auto I = funcsToUpdate.rbegin(); I != funcsToUpdate.rend(); I++)
    {
        Function* F = I->oldFunc;
        GenericPointerArgs& GPArgs = I->newArgs;
        // Determine the unique origin address space of generic pointer args
        // If it can't be determined, remove it from the function to update
        GPArgs.erase(std::remove_if(GPArgs.begin(), GPArgs.end(),
            [this, F](std::pair<unsigned, unsigned>& func) {
                return hasSameOriginAddressSpace(F, func.first, func.second) == false;
            }),
            GPArgs.end());

        if (GPArgs.empty())
            continue;

        // Create the new function body and insert it into the module
        FunctionType* pFuncType = F->getFunctionType();
        std::vector<Type*> newParamTypes(pFuncType->param_begin(), pFuncType->param_end());
        for (auto newArg : GPArgs)
        {
            PointerType* ptrType = PointerType::get(newParamTypes[newArg.first]->getPointerElementType(),
                newArg.second);
            newParamTypes[newArg.first] = ptrType;
        }

        // Create new function type with explicit and implicit parameter types
        FunctionType* newFTy = FunctionType::get(F->getReturnType(), newParamTypes, F->isVarArg());

        Function* newFunc = Function::Create(newFTy, F->getLinkage());
        newFunc->copyAttributesFrom(F);
        newFunc->setSubprogram(F->getSubprogram());
        M.getFunctionList().insert(F->getIterator(), newFunc);
        newFunc->takeName(F);
        newFunc->getBasicBlockList().splice(newFunc->begin(), F->getBasicBlockList());

        // Update argument list and transfer their uses from old function
        updateFunctionArgs(F, newFunc, GPArgs);

        I->newFunc = newFunc;
    }

    // At this point, there may be functions without generic pointers to be lowered
    funcsToUpdate.erase(std::remove_if(funcsToUpdate.begin(), funcsToUpdate.end(),
        [](FuncToUpdate& f) { return f.newFunc == nullptr; }),
        funcsToUpdate.end());


    // Step 3: update all call sites with the new pointers address space
    for (auto& I : funcsToUpdate)
    {
        updateAllUsesWithNewFunction(I);
    }

    // Step 4: Update IGC Metadata. Function declarations have changed, so this needs
    // to be reflected in the metadata.
    MetadataBuilder mbuilder(&M);
    auto& FuncMD = m_ctx->getModuleMetaData()->FuncMD;
    for (auto& I : funcsToUpdate)
    {
        auto oldFuncIter = m_mdUtils->findFunctionsInfoItem(I.oldFunc);
        m_mdUtils->setFunctionsInfoItem(I.newFunc, oldFuncIter->second);
        m_mdUtils->eraseFunctionsInfoItem(oldFuncIter);
        mbuilder.UpdateShadingRate(I.oldFunc, I.newFunc);
        auto loc = FuncMD.find(I.oldFunc);
        if (loc != FuncMD.end())
        {
            auto funcInfo = loc->second;
            FuncMD.erase(I.oldFunc);
            FuncMD[I.newFunc] = funcInfo;
        }
    }
    // Update LLVM metadata based on IGC MetadataUtils
    m_mdUtils->save(M.getContext());


    // It's safe now to remove old functions
    for (auto& I : funcsToUpdate)
    {
        IGC_ASSERT(nullptr != I.oldFunc);
        IGC_ASSERT_MESSAGE(I.oldFunc->use_empty(), "All function uses should have been transfered to new function");
        I.oldFunc->eraseFromParent();
    }

    // Step 5: after lowering pointers in function arguments, some of their uses can be
    // other function calls whose arguments couldn't be lowered.
    // e.g. (gp is generic pointer, lp is pointer to local)
    //  kernel()                                                 kernel()
    //     |                                After lowering:         |
    //     ---> foo(gp1, gp2)                                       ---> foo(lp1, lp2)
    //                 |                                                      |
    //                 ---> bar(gp1, gp2)                                     ---> bar(lp1, gp2)
    //
    // gp1 is lowered in foo and bar, gp2 is lowered only in foo. When lowering gp2 in foo,
    // all its uses were optimistically updated to lp2. We fix those unsuccessful cases here.
    for (Function& F : M)
    {
        if (F.isVarArg()) continue;

        for (auto UI = F.user_begin(), UE = F.user_end(); UI != UE; UI++)
        {
            if (CallInst* callInst = dyn_cast<CallInst>(*UI))
            {
                Function::arg_iterator funcArg = F.arg_begin();
                for (unsigned int i = 0; i < callInst->getNumArgOperands(); ++i, ++funcArg)
                {
                    Value* callArg = callInst->getOperand(i);
                    if (callArg->getType() != funcArg->getType())
                    {
                        PointerType* callArgTy = dyn_cast<PointerType>(callArg->getType());
                        PointerType* funcArgTy = dyn_cast<PointerType>(funcArg->getType());

                        // If generic is expected, simple add back the address space cast
                        if (callArgTy && callArgTy->getAddressSpace() != ADDRESS_SPACE_GENERIC &&
                            funcArgTy && funcArgTy->getAddressSpace() == ADDRESS_SPACE_GENERIC)
                        {
                            AddrSpaceCastInst* asc = new AddrSpaceCastInst(callArg, funcArg->getType(), "", callInst);
                            callInst->setArgOperand(i, asc);
                        }
                    }
                }
            }
        }
    }
    return true;
}

bool LowerGPCallArg::processGASInst(Module& M)
{
    if (!m_ctx->hasNoPrivateToGenericCast() || !m_ctx->hasNoLocalToGenericCast())
        return false;

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

    bool changed = false;
    IRBuilder<> IRB(M.getContext());
    // Change GAS inst, such as ld/st, etc to global ld/st, etc.
    for (Function& F : M)
    {
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

                    changed = true;
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

                    changed = true;
                }
            }
        }
    }

    return changed;
}

bool LowerGPCallArg::hasSameOriginAddressSpace(Function* func, unsigned argNo, unsigned& addrSpaceCallSite)
{
    unsigned verifiedCallSites = 0;

    // Check if all the callers have the same pointer address space
    for (auto U : func->users())
    {
        auto CI = cast<CallInst>(U);
        Value* V = CI->getArgOperand(argNo);

        if (!V->getType()->isPointerTy())
            continue;

        unsigned addrSpaceCurrentCallSite = 0;

        if (AddrSpaceCastInst* addrSpaceCastInst = dyn_cast<AddrSpaceCastInst>(V))
            addrSpaceCurrentCallSite = addrSpaceCastInst->getSrcAddressSpace();
        else
            addrSpaceCurrentCallSite = (dyn_cast<PointerType>(V->getType()))->getAddressSpace();

        if (verifiedCallSites == 0)
        {
            addrSpaceCallSite = addrSpaceCurrentCallSite;
            verifiedCallSites++;
            continue;
        }

        if (addrSpaceCallSite != addrSpaceCurrentCallSite)
            break;

        verifiedCallSites++;
    }

    return verifiedCallSites == func->getNumUses();
}


// Modifies address space in uses of pointer argument
void LowerGPCallArg::fixAddressSpaceInAllUses(Value* ptr, uint newAS, uint oldAS, AddrSpaceCastInst* recoverASC)
{
    IGC_ASSERT(newAS != oldAS);
    auto nextUI = ptr->user_begin(), E = ptr->user_end();
    do
    {
        auto UI = nextUI;
        nextUI++;
        Instruction* inst = dyn_cast<Instruction>(*UI);
        PointerType* instType = nullptr;

        if (auto* asc = dyn_cast<AddrSpaceCastInst>(inst))
        {
            // When mutating AS cast and another AS cast is using mutated one,
            // it may result in same space AS cast which is invalid,
            // so we replace the invalid cast with:
            // - a bitcast if pointer types are different
            // - ptr if cast is eliminated
            if (asc->getDestAddressSpace() == asc->getSrcAddressSpace())
            {
                IRBuilder<> IRB(asc);
                Value* bcOrPtr = IRB.CreateBitCast(ptr, asc->getDestTy());
                asc->replaceAllUsesWith(bcOrPtr);
                asc->eraseFromParent();
                continue;
            }
        }

        if (StoreInst* storeInst = dyn_cast<StoreInst>(inst))
        {
            // We cannot propagate the non-generic AS to the value operand of a store.
            // In this situation the pointer operand remains generic, so we add back the
            // addrspacecast.
            if (UI.getUse().getOperandNo() != storeInst->getPointerOperandIndex())
            {
                UI.getUse().set(recoverASC);
                continue;
            }
        }
        else if (CallInst* cInst = dyn_cast<CallInst>(inst))
        {
            // We cannot propagate the non-generic AS to arg of a call.
            // It will be propagate later if it is applicable.
            UI.getUse().set(recoverASC);
            continue;
        }

        // add inst to partiallyLowered Inst list.  cover select and phi for now, and may add others later
        if (isa<SelectInst>(inst) || isa<PHINode>(inst))
        {
            // if not all operands are lowered, add to partiallyLowered list, and don't propagate
            bool partiallylowered = false;
            bool differentLoweredAddrSpaces = false;
            for (unsigned i = 0; i < inst->getNumOperands(); ++i)
            {
                Value* srci = inst->getOperand(i);

                if (PointerType* PtrTy = dyn_cast<PointerType>(srci->getType()))
                {
                    uint srciAddrSpace = PtrTy->getAddressSpace();

                    // handle case when lowered address spaces of sources are different
                    if (srciAddrSpace != oldAS && srciAddrSpace != newAS)
                    {
                        UI.getUse().set(recoverASC);
                        differentLoweredAddrSpaces = true;
                        break;
                    }

                    if (srciAddrSpace == oldAS)
                    {
                        partiallylowered = true;
                        break;
                    }
                }
            }

            if (differentLoweredAddrSpaces)
            {
                continue;
            }
            if (partiallylowered)
            {
                m_partiallyLoweredInsts.push_back(inst);
                continue;
            }
        }

        if (isa<BitCastInst>(inst) || isa<GetElementPtrInst>(inst) || isa<PHINode>(inst) || isa<SelectInst>(inst)) {
            instType = dyn_cast<PointerType>(inst->getType());
        }

        if (instType && instType->getAddressSpace() == oldAS)
        {
            Type* eltType = instType->getElementType();
            PointerType* ptrType = PointerType::get(eltType, newAS);
            inst->mutateType(ptrType);
            // Add an addrspacecast in for cases where the non-generic can't be propagated.
            AddrSpaceCastInst* recoverASC = new AddrSpaceCastInst(inst, instType, "", inst->getNextNode());

            fixAddressSpaceInAllUses(inst, newAS, oldAS, recoverASC);

            // Remove addrspacecast if it wasn't used
            if (recoverASC->use_empty())
                recoverASC->eraseFromParent();
        }
    } while (nextUI != E);
}

// Loops over the argument list transferring uses from old function to new one.
void LowerGPCallArg::updateFunctionArgs(Function* oldFunc, Function* newFunc, GenericPointerArgs& newArgs)
{
    Function::arg_iterator currArg = newFunc->arg_begin();
    unsigned currentArgIdx = 0, newArgIdx = 0;
    m_partiallyLoweredInsts.clear(); // initiate a list for partiallyLoweredInsts

    for (Function::arg_iterator I = oldFunc->arg_begin(), E = oldFunc->arg_end();
        I != E; ++I, ++currArg, ++currentArgIdx)
    {
        Value* newArg = &(*currArg);
        // Check if the next entry in newArgs is for currentArgIdx arg
        if (newArgIdx < newArgs.size() && currentArgIdx == newArgs[newArgIdx].first)
        {
            if (I->getType() != currArg->getType())
            {
                PointerType* originalPointerTy = dyn_cast<PointerType>(I->getType());
                PointerType* newPointerTy = PointerType::get(I->getType()->getPointerElementType(),
                    newArgs[newArgIdx].second);
                I->mutateType(newPointerTy);

                // Add an addrspacecast in for cases where the non-generic can't be propagated.
                AddrSpaceCastInst* recoverASC = new AddrSpaceCastInst(I, originalPointerTy, "",
                    newFunc->getEntryBlock().getFirstNonPHI());

                fixAddressSpaceInAllUses(I, newArgs[newArgIdx].second, ADDRESS_SPACE_GENERIC, recoverASC);

                // Remove addrspacecast if it wasn't used
                if (recoverASC->use_empty())
                {
                    recoverASC->eraseFromParent();
                }
            }
            newArgIdx++;
        }
        I->replaceAllUsesWith(newArg);
        currArg->takeName(&(*I));
    }

    // travese the partiallyLoweredInsts and insert cast for those with incompatible addrspace
    for (auto inst : m_partiallyLoweredInsts)
    {
        IGC_ASSERT((dyn_cast<SelectInst>(inst) || dyn_cast<PHINode>(inst)));
        std::vector<Value*> unloweredSrc; // PHI may have more than 2 operands
        std::vector<Value*> loweredSrc;
        PointerType* ASGPtrTy = nullptr;

        for (unsigned i = 0; i < inst->getNumOperands(); ++i)
        {
            Value* srci = inst->getOperand(i);

            if (PointerType* PtrTy = dyn_cast<PointerType>(srci->getType()))
            {
                if (PtrTy->getAddressSpace() == ADDRESS_SPACE_GENERIC)
                {
                    unloweredSrc.push_back(srci);
                    ASGPtrTy = PtrTy;  // result type of the new ASC inst
                }
                else
                {
                    loweredSrc.push_back(srci);
                }
            }
        }
        // if the inst a partially lowered, insert a cast
        if (!loweredSrc.empty() && !unloweredSrc.empty())
        {
            IGC_ASSERT(ASGPtrTy != nullptr);
            for (auto src : loweredSrc)
            {
                Instruction* srcInst = dyn_cast<Instruction>(src);
                AddrSpaceCastInst* asc;

                // insert addrspacecast after src, so to keep PHI in the beginning of its block
                // find insertBefore to be after src in 3 cases:
                // 1. if src is a phi, getFirstInsertionPt after the last phi
                // 2. if src is a regular inst, find the next inst after it.
                // 3. if src is a function arg, find the beginning of the entry block
                Instruction* insertBefore = nullptr;
                if (srcInst)
                {
                    if (isa<PHINode>(srcInst)) // 1
                    {
                        BasicBlock* BB = srcInst->getParent();
                        insertBefore = &(*BB->getFirstInsertionPt());
                    }
                    else // 2 src is an instruction
                    {
                        BasicBlock::iterator iter(srcInst);
                        ++iter;
                        insertBefore = &(*iter);
                    }
                }
                else // 3 src is an argument and insert at the begin of entry BB
                {
                    BasicBlock& entryBB = inst->getParent()->getParent()->getEntryBlock();
                    insertBefore = &(*entryBB.getFirstInsertionPt());
                }
                IGC_ASSERT(insertBefore);
                asc = new AddrSpaceCastInst(src, ASGPtrTy, "", insertBefore);
                IGC_ASSERT(asc);
                inst->replaceUsesOfWith(src, asc);
            }
        }
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

void LowerGPCallArg::updateAllUsesWithNewFunction(FuncToUpdate& f)
{
    IGC_ASSERT(!f.oldFunc->use_empty());

    // Keep track of old calls and addrspacecast to be deleted later
    std::vector<CallInst*> callsToDelete;
    std::vector<AddrSpaceCastInst*> ASCToDelete;

    for (auto U = f.oldFunc->user_begin(), E = f.oldFunc->user_end(); U != E; ++U)
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

        auto AI = f.newFunc->arg_begin();
        for (unsigned int i = 0; i < cInst->getNumArgOperands(); ++i, ++AI)
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
                            *f.newFunc->getParent());
                        ASCToDelete.push_back(addrSpaceCastInst);
                    }
                }
            }
            newCallArgs.push_back(callArg);
        }

        // Create new call and insert it before old one
        CallInst* inst = CallInst::Create(f.newFunc, newCallArgs,
            f.newFunc->getReturnType()->isVoidTy() ? "" : f.newFunc->getName(),
            cInst);

        inst->setCallingConv(f.newFunc->getCallingConv());
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

namespace IGC
{
    class CastToGASAnalysis : public ModulePass
    {
    public:
        static char ID;

        CastToGASAnalysis() : ModulePass(ID)
        {
            initializeCastToGASAnalysisPass(*PassRegistry::getPassRegistry());
        }

        bool runOnModule(Module&) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
        }

        virtual StringRef getPassName() const override
        {
            return "GenericPointerUsageAnalysis";
        }

    private:
        CodeGenContext* m_ctx = nullptr;
    };
} // End anonymous namespace

ModulePass* IGC::createCastToGASAnalysisPass() { return new CastToGASAnalysis(); }

char CastToGASAnalysis::ID = 0;

#define CAST_TO_GAS_PASS_FLAG "generic-pointer-analysis"
#define CAST_TO_GAS_PASS_DESC "Analyze generic pointer usage"
#define CAST_TO_GAS_PASS_CFG_ONLY false
#define CAST_TO_GAS_PASS_ANALYSIS false
namespace IGC
{
    IGC_INITIALIZE_PASS_BEGIN(CastToGASAnalysis, CAST_TO_GAS_PASS_FLAG, CAST_TO_GAS_PASS_DESC, CAST_TO_GAS_PASS_CFG_ONLY, CAST_TO_GAS_PASS_ANALYSIS)
    IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
    IGC_INITIALIZE_PASS_END(CastToGASAnalysis, CAST_TO_GAS_PASS_FLAG, CAST_TO_GAS_PASS_DESC, CAST_TO_GAS_PASS_CFG_ONLY, CAST_TO_GAS_PASS_ANALYSIS)
}

bool CastToGASAnalysis::runOnModule(llvm::Module& M)
{
    m_ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    bool hasPrivateCast = false; // true if there is a cast from private to GAS
    bool hasLocalCast = false; // true if there is a cast from local to GAS.

    bool hasIntToGeneric = false;
    bool hasLocalToInt = false;
    bool hasPrivateToInt = false;

    for (Function& F : M)
    {
        // ToDo: replace with generic checks for extern functions
        if (F.hasFnAttribute("referenced-indirectly"))
        {
            for (auto& arg : F.args())
            {
                PointerType* argPointerType = dyn_cast<PointerType>(arg.getType());
                if (argPointerType && argPointerType->getAddressSpace() == ADDRESS_SPACE_GENERIC)
                {
                    return false;
                }
            }
        }

        for (auto FI = inst_begin(F), FE = inst_end(F);
            (FI != FE) && !(hasPrivateCast && hasLocalCast); ++FI)
        {
            Instruction* I = &*FI;
            if (auto* ASC = dyn_cast<AddrSpaceCastInst>(I))
            {
                if (ASC->getDestAddressSpace() != ADDRESS_SPACE_GENERIC)
                    continue;

                unsigned AS = ASC->getSrcAddressSpace();
                if (AS == ADDRESS_SPACE_LOCAL)
                    hasLocalCast = true;
                else if (AS == ADDRESS_SPACE_PRIVATE)
                    hasPrivateCast = true;
            }
            else if (auto* ITP = dyn_cast<IntToPtrInst>(I))
            {
                if (ITP->getAddressSpace() == ADDRESS_SPACE_GENERIC)
                    hasIntToGeneric = true;
            }
            else if (auto* PTI = dyn_cast<PtrToIntInst>(I))
            {
                unsigned AS = PTI->getPointerAddressSpace();
                if (AS == ADDRESS_SPACE_LOCAL)
                    hasLocalToInt = true;
                else if (AS == ADDRESS_SPACE_PRIVATE)
                    hasPrivateToInt = true;
            }
        }
    }

    // Take `ptrtoint` instructions into account only if there is a GAS `inttoptr` instruction
    if (hasIntToGeneric)
    {
        if (!hasLocalCast)
            hasLocalCast = hasLocalToInt;
        if (!hasPrivateCast)
            hasPrivateCast = hasPrivateToInt;
    }

    // Set those so that dynamic resolution can use them.
    m_ctx->getModuleMetaData()->hasNoLocalToGenericCast = !hasLocalCast;
    m_ctx->getModuleMetaData()->hasNoPrivateToGenericCast = !hasPrivateCast;

    return true;
}
