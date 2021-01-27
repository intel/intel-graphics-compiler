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
#include <llvm/IR/NoFolder.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

namespace {

    typedef IRBuilder<llvm::NoFolder> BuilderType;

    class GASPropagator;

    // Generaic address space (GAS) pointer resolving is done in two steps:
    // 1) Find cast from non-GAS pointer to GAS pointer
    // 2) Propagate that non-GAS pointer to all users of that GAS pointer at best
    //    effort.
    class GASResolving : public FunctionPass {
        const unsigned GAS = ADDRESS_SPACE_GENERIC;

        BuilderType* IRB;
        GASPropagator* Propagator;

        // Phi node being able to be resolved from its initial value.
        DenseSet<PHINode*> ResolvableLoopPHIs;

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

        bool isResolvableLoopPHI(PHINode* PN) const {
            return ResolvableLoopPHIs.count(PN) != 0;
        }

    private:
        bool resolveOnFunction(Function*) const;
        bool resolveOnBasicBlock(BasicBlock*) const;

        bool resolveMemoryFromHost(Function&) const;

        void populateResolvableLoopPHIs();
        void populateResolvableLoopPHIsForLoop(const Loop*);

        bool isAddrSpaceResolvable(PHINode* PN, const Loop* L,
            BasicBlock* BackEdge) const;

        bool checkGenericArguments(Function& F) const;
        void convertLoadToGlobal(LoadInst* LI) const;
        bool isLoadGlobalCandidate(LoadInst* LI) const;
    };

    class GASPropagator : public InstVisitor<GASPropagator, bool> {
        friend class InstVisitor<GASPropagator, bool>;

        GASResolving* Resolver;
        BuilderType* IRB;

        Use* TheUse;
        Value* TheVal;

    public:
        GASPropagator(GASResolving* R, BuilderType* Builder)
            : Resolver(R), IRB(Builder) {}

        bool propagate(Use* U, Value* V) {
            TheUse = U;
            TheVal = V;
            Instruction* I = cast<Instruction>(U->getUser());
            return visit(*I);
        }

    private:
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
    GASPropagator ThePropagator(this, &TheBuilder);
    IRB = &TheBuilder;
    Propagator = &ThePropagator;

    resolveMemoryFromHost(F);

    populateResolvableLoopPHIs();

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
        DenseSet<Instruction*> instructionSet;
        DenseSet<Use*> useSet;
        for (auto UI = CI->use_begin(), UE = CI->use_end(); UI != UE; ++UI) {
            Use* U = &(*UI);
            Instruction* I = cast<Instruction>(U->getUser());
            if (instructionSet.find(I) == instructionSet.end())
            {
                instructionSet.insert(I);
                useSet.insert(U);
            }
        }
        // Propagate that source through all users of this cast.
        for (auto it = useSet.begin(); it != useSet.end(); ++it) {
            Use* U = *it;
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

void GASResolving::populateResolvableLoopPHIs() {
    LoopInfo& LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    for (LoopInfo::reverse_iterator I = LI.rbegin(), E = LI.rend(); I != E; ++I) {
        Loop* L = *I;
        populateResolvableLoopPHIsForLoop(L);
    }
}

void GASResolving::populateResolvableLoopPHIsForLoop(const Loop* L) {
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

bool GASResolving::isAddrSpaceResolvable(PHINode* PN, const Loop* L,
    BasicBlock* BackEdge) const {
    PointerType* PtrTy = dyn_cast<PointerType>(PN->getType());
    if (!PtrTy || PtrTy->getAddressSpace() != GAS)
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

    if (Resolver->isResolvableLoopPHI(&PN)) {
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

static bool handelMemTransferInst(MemTransferInst& I) {
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
    if(nullptr != NewDst)
    {
        IGC_ASSERT(nullptr != DstUse);
        DstUse->set(NewDst);
    }
    if(nullptr != NewSrc)
    {
        IGC_ASSERT(nullptr != SrcUse);
        SrcUse->set(NewSrc);
    }
    return true;
}

bool GASPropagator::visitMemCpyInst(MemCpyInst& I) {
    return handelMemTransferInst(I);
}

bool GASPropagator::visitMemMoveInst(MemMoveInst& I) {
    return handelMemTransferInst(I);
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
                if (CI->onlyReadsMemory())
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

namespace IGC
{
    //
    // Optimization pass to lower generic pointers in function arguments.
    // If all call sites have the same origin address space, address space
    // casts with the form of non-generic->generic can safely removed and
    // function updated with non-generic pointer argument.
    //
    // The complete process to lower generic pointer args consists of 5 steps.
    //   1) find all functions that are candidates
    //   2) update functions and their signatures
    //   3) update all call sites
    //   4) update functions metadata
    //   5) validate that all function calls are properly formed
    //
    //
    // Current limitations/considerations:
    // - only arguments of non-extern functions can be lowered
    // - no recursive functions supported
    //
    class LowerGPCallArg : public llvm::ModulePass
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
            Function* newFunc;

            FuncToUpdate(Function* f, GenericPointerArgs& args)
            {
                oldFunc = f;
                newArgs = args;
                newFunc = nullptr;
            }
        };

        IGCMD::MetaDataUtils* mdUtils;
        CodeGenContext* ctx;
        bool hasSameOriginAddressSpace(Function* func, unsigned argNo, unsigned& addrSpaceCallSite);
        void updateFunctionArgs(Function* oldFunc, Function* newFunc, GenericPointerArgs& newArgs);
        void updateAllUsesWithNewFunction(FuncToUpdate& f);
        void FixAddressSpaceInAllUses(Value* ptr, uint newAS, uint oldAS, AddrSpaceCastInst* recoverASC);
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
    ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    mdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    CallGraph& CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();

    std::vector<FuncToUpdate> funcsToUpdate;

    // Step 1: find the candidates, which are functions with generic pointer args.
    // Functions will be updated later in topological ordering (top-down calls).
    for (auto I = po_begin(CG.getExternalCallingNode()), E = po_end(CG.getExternalCallingNode()); I != E; ++I)
    {
        auto CGNode = *I;
        // Skip external and indirect nodes.
        if (auto F = CGNode->getFunction())
        {
            // Only non-extern functions within the module are optimized
            if (F->hasFnAttribute("IndirectlyCalled") || F->isDeclaration()
                || F->isIntrinsic() || F->user_empty())
                continue;

            // Skip functions that return generic pointer for now
            PointerType* returnPointerType = dyn_cast<PointerType>(F->getReturnType());
            if (returnPointerType && returnPointerType->getAddressSpace() == ADDRESS_SPACE_GENERIC)
                continue;


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
        return false;

    // Step 2: update functions and lower their generic pointer arguments
    // to their non-generic address space.
    for (auto I = funcsToUpdate.rbegin(); I != funcsToUpdate.rend(); I++)
    {
        Function* F = (*I).oldFunc;
        GenericPointerArgs& GPArgs = (*I).newArgs;
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

        (*I).newFunc = newFunc;
    }

    // At this point, there may be functions without generic pointers to be lowered
    funcsToUpdate.erase(std::remove_if(funcsToUpdate.begin(), funcsToUpdate.end(),
        [](FuncToUpdate& f) { return f.newFunc == nullptr; }),
        funcsToUpdate.end());


    // Step 3: update all call sites with the new pointers address space
    for (auto &I : funcsToUpdate)
    {
        updateAllUsesWithNewFunction(I);
    }

    // Step 4: Update IGC Metadata. Function declarations have changed, so this needs
    // to be reflected in the metadata.
    MetadataBuilder mbuilder(&M);
    auto& FuncMD = ctx->getModuleMetaData()->FuncMD;
    for (auto &I : funcsToUpdate)
    {
        auto oldFuncIter = mdUtils->findFunctionsInfoItem(I.oldFunc);
        mdUtils->setFunctionsInfoItem(I.newFunc, oldFuncIter->second);
        mdUtils->eraseFunctionsInfoItem(oldFuncIter);
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
    mdUtils->save(M.getContext());


    // It's safe now to remove old functions
    for (auto &I : funcsToUpdate)
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
    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    {
        Function* func = &(*I);
        for (auto UI = func->user_begin(), UE = func->user_end(); UI != UE; UI++)
        {
            if (CallInst* callInst = dyn_cast<CallInst>(*UI))
            {
                Function::arg_iterator funcArg = func->arg_begin();
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

bool LowerGPCallArg::hasSameOriginAddressSpace(Function* func, unsigned argNo, unsigned &addrSpaceCallSite)
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
void LowerGPCallArg::FixAddressSpaceInAllUses(Value* ptr, uint newAS, uint oldAS, AddrSpaceCastInst* recoverASC)
{
    IGC_ASSERT(newAS != oldAS);
    for (auto UI = ptr->user_begin(), E = ptr->user_end(); UI != E; ++UI)
    {
        Instruction* inst = dyn_cast<Instruction>(*UI);
        PointerType* instType = nullptr;

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

        if (BitCastInst* bitCastInst = dyn_cast<BitCastInst>(inst))
        {
            instType = dyn_cast<PointerType>(bitCastInst->getType());
        }
        else if (GetElementPtrInst* gepInst = dyn_cast<GetElementPtrInst>(inst))
        {
            instType = dyn_cast<PointerType>(gepInst->getType());
        }

        if (instType && instType->getAddressSpace() == oldAS)
        {
            Type* eltType = instType->getElementType();
            PointerType* ptrType = PointerType::get(eltType, newAS);
            inst->mutateType(ptrType);
            FixAddressSpaceInAllUses(inst, newAS, oldAS, recoverASC);
        }
    }
}

// Loops over the argument list transferring uses from old function to new one.
void LowerGPCallArg::updateFunctionArgs(Function* oldFunc, Function* newFunc, GenericPointerArgs& newArgs)
{

    Function::arg_iterator currArg = newFunc->arg_begin();
    unsigned currentArgIdx = 0, newArgIdx = 0;



    for (Function::arg_iterator I = oldFunc->arg_begin(), E = oldFunc->arg_end();
        I != E; ++I, ++currArg, ++currentArgIdx)
    {
        Value* newArg = &(*currArg);
        if ((*I).getType() != currArg->getType())
        {
            if (currentArgIdx == newArgs[newArgIdx].first)
            {
                PointerType* originalPointerTy = dyn_cast<PointerType>(I->getType());

                PointerType* newPointerTy = PointerType::get(I->getType()->getPointerElementType(),
                    newArgs[newArgIdx].second);
                I->mutateType(newPointerTy);

                // Add an addrspacecast in for cases where the non-generic can't be propagated.
                AddrSpaceCastInst* recoverASC = new AddrSpaceCastInst(I, originalPointerTy, "",
                    newFunc->getEntryBlock().getFirstNonPHI());

                FixAddressSpaceInAllUses(I, newArgs[newArgIdx].second, ADDRESS_SPACE_GENERIC, recoverASC);

                // Remove addrspacecast if it wasn't used
                if (recoverASC->getNumUses() == 0)
                {
                    recoverASC->eraseFromParent();
                }
                newArgIdx++;
            }
        }
        I->replaceAllUsesWith(newArg);
        currArg->takeName(&(*I));
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
                if (callArgTy->getAddressSpace() == ADDRESS_SPACE_GENERIC &&
                    funcArgTy->getAddressSpace() != ADDRESS_SPACE_GENERIC)
                {
                    // If call site address space is generic and function arg is non-generic,
                    // the addrspacecast is removed and non-generic address space lowered
                    // to the function call.
                    AddrSpaceCastInst* addrSpaceCastInst = dyn_cast<AddrSpaceCastInst>(callArg);
                    if (addrSpaceCastInst)
                    {
                        callArg = addrSpaceCastInst->getOperand(0);
                        if (addrSpaceCastInst->getNumUses() == 1)
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