/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/GenericAddressResolution/GenericAddressDynamicResolution.hpp"
#include "Compiler/CISACodeGen/CastToGASAnalysis.h"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CodeGenPublicEnums.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/Support/Alignment.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using IGCLLVM::getAlign;

namespace {
    class GenericAddressAnalysis : public FunctionPass {
    public:
        static char ID;

        GenericAddressAnalysis()
            : FunctionPass(ID)
        {
        }
        ~GenericAddressAnalysis() = default;

        virtual void getAnalysisUsage(AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
        }

        virtual StringRef getPassName() const override
        {
            return "GenericAddressAnalysis";
        }

        virtual bool runOnFunction(Function& F) override;
    };
} // namespace

// Register pass
#define PASS_FLAG2 "igc-generic-address-analysis"
#define PASS_DESCRIPTION2 "Finds when generic pointers are used"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 false
IGC_INITIALIZE_PASS_BEGIN(GenericAddressAnalysis, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CastToGASWrapperPass)
IGC_INITIALIZE_PASS_END(GenericAddressAnalysis, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)

char GenericAddressAnalysis::ID = 0;

bool GenericAddressAnalysis::runOnFunction(Function& F)
{
    for (auto& BB : F.getBasicBlockList()) {
        for (auto& Inst : BB.getInstList()) {
            Type* Ty = Inst.getType();
            if (auto AI = dyn_cast<AllocaInst>(&Inst))
                Ty = AI->getAllocatedType();
            else if (auto LI = dyn_cast<LoadInst>(&Inst))
                Ty = LI->getPointerOperand()->getType();
            else if (auto SI = dyn_cast<StoreInst>(&Inst))
                Ty = SI->getPointerOperand()->getType();
            else if (auto GEP = dyn_cast<GetElementPtrInst>(&Inst))
                Ty = GEP->getPointerOperandType();
            auto PT = dyn_cast<PointerType>(Ty);
            if (PT && PT->getAddressSpace() == ADDRESS_SPACE_GENERIC) {
                return true;
            }
        }
    }

    return false;
}

namespace {
    class GenericAddressDynamicResolution : public FunctionPass {
    public:
        static char ID;
        Module* m_module = nullptr;
        CodeGenContext* m_ctx = nullptr;

        GenericAddressDynamicResolution()
            : FunctionPass(ID)
        {
        }
        ~GenericAddressDynamicResolution() = default;

        virtual StringRef getPassName() const override
        {
            return "GenericAddressDynamicResolution";
        }

        virtual void getAnalysisUsage(AnalysisUsage& AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<CastToGASWrapperPass>();
        }

        virtual bool runOnFunction(Function& F) override;

        bool visitLoadStoreInst(Instruction& I);
        bool visitIntrinsicCall(CallInst& I);
        Module* getModule() { return m_module; }

    private:
        bool m_needPrivateBranches = false;
        bool m_needLocalBranches = false;

        Type* getPointerAsIntType(LLVMContext& Ctx, unsigned AS);
        void resolveGAS(Instruction& I, Value* pointerOperand);
        void resolveGASWithoutBranches(Instruction& I, Value* pointerOperand);
    };
} // namespace


// Register pass to igc-opt
#define PASS_FLAG "igc-generic-address-dynamic-resolution"
#define PASS_DESCRIPTION "Resolve generic address space loads/stores"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(GenericAddressDynamicResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CastToGASWrapperPass)
IGC_INITIALIZE_PASS_END(GenericAddressDynamicResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char GenericAddressDynamicResolution::ID = 0;

bool GenericAddressDynamicResolution::runOnFunction(Function& F)
{
    m_module = F.getParent();
    m_ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    GASInfo& GI = getAnalysis<CastToGASWrapperPass>().getGASInfo();
    m_needPrivateBranches = GI.canGenericPointToPrivate(F);
    m_needLocalBranches = GI.canGenericPointToLocal(F);

    bool modified = false;
    bool changed = false;

    // iterate for all the intrinisics used by to_local, to_global, and to_private
    do {
        changed = false;

        for (inst_iterator i = inst_begin(F); i != inst_end(F); ++i) {
            Instruction& instruction = (*i);

            if (CallInst * intrinsic = dyn_cast<CallInst>(&instruction)) {
                changed = visitIntrinsicCall(*intrinsic);
            }

            if (changed) {
                modified = true;
                break;
            }
        }
    } while (changed);

    // iterate over all loads/stores with generic address space pointers
    do {
        changed = false;

        for (inst_iterator i = inst_begin(F); i != inst_end(F); ++i) {
            Instruction& instruction = (*i);

            if (isa<LoadInst>(instruction) || isa<StoreInst>(instruction)) {
                changed = visitLoadStoreInst(instruction);
            }

            if (changed) {
                modified = true;
                break;
            }
        }
    } while (changed);

    return modified;
}

Type* GenericAddressDynamicResolution::getPointerAsIntType(LLVMContext& ctx, const unsigned AS)
{
    Module* pModule = getModule();
    DataLayout dataLayout = pModule->getDataLayout();
    unsigned ptrBits(dataLayout.getPointerSizeInBits(AS));
    return IntegerType::get(ctx, ptrBits);
}

bool GenericAddressDynamicResolution::visitLoadStoreInst(Instruction& I)
{
    bool changed = false;

    Value* pointerOperand = nullptr;
    unsigned int pointerAddressSpace = ADDRESS_SPACE_NUM_ADDRESSES;

    if (LoadInst * load = dyn_cast<LoadInst>(&I)) {
        pointerOperand = load->getPointerOperand();
        pointerAddressSpace = load->getPointerAddressSpace();
    }
    else if (StoreInst * store = dyn_cast<StoreInst>(&I)) {
        pointerOperand = store->getPointerOperand();
        pointerAddressSpace = store->getPointerAddressSpace();
    }
    else {
        IGC_ASSERT_EXIT_MESSAGE(0, "Unable to resolve generic address space pointer");
    }

    if (pointerAddressSpace == ADDRESS_SPACE_GENERIC) {
        if((m_ctx->allocatePrivateAsGlobalBuffer() || !m_needPrivateBranches) &&
            !m_needLocalBranches)
        {
            resolveGASWithoutBranches(I, pointerOperand);
        }
        else
        {
            resolveGAS(I, pointerOperand);
        }
        changed = true;
    }

    return changed;
}

void GenericAddressDynamicResolution::resolveGAS(Instruction& I, Value* pointerOperand)
{
    std::stringstream warningInfo;
    if (m_ctx->m_instrTypes.hasDebugInfo)
    {
        llvm::DILocation* dbInfo = I.getDebugLoc();
        llvm::Instruction* prevInst = I.getPrevNode();

        while (dbInfo == nullptr)
        {
            if (prevInst == nullptr)
            {
                break;
            }
            dbInfo = prevInst->getDebugLoc();
            prevInst = prevInst->getPrevNode();
        }
        if (dbInfo != nullptr)
        {
            warningInfo << "from dir:" << dbInfo->getDirectory().str();
            warningInfo << " from file:" << dbInfo->getFilename().str();
            warningInfo << " line:" << dbInfo->getLine();
            warningInfo << " :";
        }
    }
    warningInfo << "Adding additional control flow due to presence of generic address space operations";
    getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->EmitWarning(warningInfo.str().c_str());

    // Every time there is a load/store from/to a generic pointer, we have to resolve
    // its corresponding address space by looking at its tag on bits[61:63].
    // First, the generic pointer's tag is obtained to then perform the load/store
    // with the corresponding address space.

    IGCLLVM::IRBuilder<> builder(&I);
    PointerType* pointerType = dyn_cast<PointerType>(pointerOperand->getType());
    IGC_ASSERT( pointerType != nullptr );
    ConstantInt* privateTag = builder.getInt64(1); // tag 001
    ConstantInt* localTag = builder.getInt64(2);   // tag 010

    Type* intPtrTy = getPointerAsIntType(pointerOperand->getContext(), ADDRESS_SPACE_GENERIC);
    Value* ptrAsInt = PtrToIntInst::Create(Instruction::PtrToInt, pointerOperand, intPtrTy, "", &I);
    // Get actual tag
    Value* tag = builder.CreateLShr(ptrAsInt, ConstantInt::get(ptrAsInt->getType(), 61));

    // Three cases for private, local and global pointers
    BasicBlock* currentBlock = I.getParent();
    BasicBlock* convergeBlock = currentBlock->splitBasicBlock(&I);

    BasicBlock* privateBlock = nullptr;
    BasicBlock* localBlock = nullptr;
    BasicBlock* globalBlock = nullptr;

    Value* localLoad = nullptr;
    Value* privateLoad = nullptr;
    Value* globalLoad = nullptr;

    // GAS needs to resolve to private only if
    //     1) private is NOT allocated in global space; and
    //     2) there is a cast from private to GAS.
    bool needPrivateBranch = !(m_ctx->allocatePrivateAsGlobalBuffer()) || m_needPrivateBranches;
    bool needLocalBranch = m_needLocalBranches;

    auto createBlock = [&](const Twine& BlockName, const Twine& LoadName, IGC::ADDRESS_SPACE addressSpace, Value*& load)
    {
        BasicBlock* BB = BasicBlock::Create(I.getContext(), BlockName, convergeBlock->getParent(), convergeBlock);
        builder.SetInsertPoint(BB);
        PointerType* ptrType = pointerType->getPointerElementType()->getPointerTo(addressSpace);
        Value* ptr = builder.CreateAddrSpaceCast(pointerOperand, ptrType);

        if (LoadInst* LI = dyn_cast<LoadInst>(&I))
        {
            load = builder.CreateAlignedLoad(ptr, getAlign(LI->getAlignment()), LI->isVolatile(), LoadName);
        }
        else if (StoreInst* SI = dyn_cast<StoreInst>(&I))
        {
            builder.CreateAlignedStore(I.getOperand(0), ptr, getAlign(SI->getAlignment()), SI->isVolatile());
        }

        builder.CreateBr(convergeBlock);
        return BB;
    };

    // Private branch
    if (needPrivateBranch)
    {
        privateBlock = createBlock("PrivateBlock", "privateLoad", ADDRESS_SPACE_PRIVATE, privateLoad);
    }

    // Local Branch
    if (needLocalBranch)
    {
        localBlock = createBlock("LocalBlock", "localLoad", ADDRESS_SPACE_LOCAL, localLoad);
    }

    // Global Branch
    globalBlock = createBlock("GlobalBlock", "globalLoad", ADDRESS_SPACE_GLOBAL, globalLoad);

    currentBlock->getTerminator()->eraseFromParent();
    builder.SetInsertPoint(currentBlock);

    const int numCases = (needPrivateBranch && needLocalBranch) ? 2 : ((needPrivateBranch || needLocalBranch) ? 1 : 0);
    IGC_ASSERT(0 < numCases);

    {
        SwitchInst* switchTag = builder.CreateSwitch(tag, globalBlock, numCases);
        // Based on tag there are two cases 001: private, 010: local, 000/111: global
        if (needPrivateBranch)
        {
            switchTag->addCase(privateTag, privateBlock);
        }
        if (needLocalBranch)
        {
            switchTag->addCase(localTag, localBlock);
        }

        if (isa<LoadInst>(&I))
        {
            IGCLLVM::IRBuilder<> phiBuilder(&(*convergeBlock->begin()));
            PHINode* phi = phiBuilder.CreatePHI(I.getType(), numCases + 1, I.getName());
            if (privateLoad)
            {
                phi->addIncoming(privateLoad, privateBlock);
            }
            if (localLoad)
            {
                phi->addIncoming(localLoad, localBlock);
            }
            phi->addIncoming(globalLoad, globalBlock);
            I.replaceAllUsesWith(phi);
        }
    }

    I.eraseFromParent();
}

void GenericAddressDynamicResolution::resolveGASWithoutBranches(Instruction& I, Value* pointerOperand)
{
    IGCLLVM::IRBuilder<> builder(&I);
    PointerType* pointerType = dyn_cast<PointerType>(pointerOperand->getType());
    IGC_ASSERT( pointerType != nullptr );

    Value* nonLocalLoad = nullptr;

    PointerType* ptrType = pointerType->getPointerElementType()->getPointerTo(ADDRESS_SPACE_GLOBAL);
    Value* globalPtr = builder.CreateAddrSpaceCast(pointerOperand, ptrType);

    if (LoadInst* LI = dyn_cast<LoadInst>(&I))
    {
        nonLocalLoad = builder.CreateAlignedLoad(globalPtr, getAlign(LI->getAlignment()), LI->isVolatile(), "globalOrPrivateLoad");
    }
    else if (StoreInst* SI = dyn_cast<StoreInst>(&I))
    {
        builder.CreateAlignedStore(I.getOperand(0), globalPtr, getAlign(SI->getAlignment()), SI->isVolatile());
    }

    if (nonLocalLoad != nullptr)
    {
        I.replaceAllUsesWith(nonLocalLoad);
    }
    I.eraseFromParent();
}

bool GenericAddressDynamicResolution::visitIntrinsicCall(CallInst& I)
{
    bool changed = false;
    Function* pCalledFunc = I.getCalledFunction();
    if (pCalledFunc == nullptr)
    {
        // Indirect call
        return false;
    }

    StringRef funcName = pCalledFunc->getName();

    if ((funcName == "__builtin_IB_to_private") || (funcName == "__builtin_IB_to_local")
        || (funcName == "__builtin_IB_to_global"))
    {
        IGC_ASSERT(IGCLLVM::getNumArgOperands(&I) == 1);
        Value* arg = I.getArgOperand(0);
        PointerType* dstType = dyn_cast<PointerType>(I.getType());
        IGC_ASSERT( dstType != nullptr );
        const unsigned targetAS = cast<PointerType>(I.getType())->getAddressSpace();

        IGCLLVM::IRBuilder<> builder(&I);
        PointerType* pointerType = dyn_cast<PointerType>(arg->getType());
        IGC_ASSERT( pointerType != nullptr );
        ConstantInt* globalTag = builder.getInt64(0);  // tag 000/111
        ConstantInt* privateTag = builder.getInt64(1); // tag 001
        ConstantInt* localTag = builder.getInt64(2);   // tag 010

        Type* intPtrTy = getPointerAsIntType(arg->getContext(), ADDRESS_SPACE_GENERIC);
        Value* ptrAsInt = PtrToIntInst::Create(Instruction::PtrToInt, arg, intPtrTy, "", &I);
        // Get actual tag
        Value* tag = builder.CreateLShr(ptrAsInt, ConstantInt::get(ptrAsInt->getType(), 61));

        Value* newPtr = nullptr;
        Value* newPtrNull = nullptr;
        Value* cmpTag = nullptr;

        // Tag was already obtained from GAS pointer, now we check its address space (AS)
        // and the target AS for this intrinsic call
        if (targetAS == ADDRESS_SPACE_PRIVATE)
            cmpTag = builder.CreateICmpEQ(tag, privateTag, "cmpTag");
        else if (targetAS == ADDRESS_SPACE_LOCAL)
            cmpTag = builder.CreateICmpEQ(tag, localTag, "cmpTag");
        else if (targetAS == ADDRESS_SPACE_GLOBAL)
            cmpTag = builder.CreateICmpEQ(tag, globalTag, "cmpTag");

        // Two cases:
        // 1: Generic pointer's AS matches with instrinsic's target AS
        //    So we create the address space cast
        // 2: Generic pointer's AS does not match with instrinsic's target AS
        //    So the instrinsic call returns NULL
        BasicBlock* currentBlock = I.getParent();
        BasicBlock* convergeBlock = currentBlock->splitBasicBlock(&I);
        BasicBlock* ifBlock = BasicBlock::Create(I.getContext(), "IfBlock",
            convergeBlock->getParent(), convergeBlock);
        BasicBlock* elseBlock = BasicBlock::Create(I.getContext(), "ElseBlock",
            convergeBlock->getParent(), convergeBlock);

        // If Block
        {
            IRBuilder<> ifBuilder(ifBlock);
            PointerType* ptrType = pointerType->getPointerElementType()->getPointerTo(targetAS);
            newPtr = ifBuilder.CreateAddrSpaceCast(arg, ptrType);
            ifBuilder.CreateBr(convergeBlock);
        }

        // Else Block
        {
            IRBuilder<> elseBuilder(elseBlock);
            Value* ptrNull = Constant::getNullValue(I.getType());
            newPtrNull = elseBuilder.CreatePointerCast(ptrNull, dstType, "");
            elseBuilder.CreateBr(convergeBlock);
        }

        currentBlock->getTerminator()->eraseFromParent();
        builder.SetInsertPoint(currentBlock);
        builder.CreateCondBr(cmpTag, ifBlock, elseBlock);

        IRBuilder<> phiBuilder(&(*convergeBlock->begin()));
        PHINode* phi = phiBuilder.CreatePHI(I.getType(), 2, I.getName());
        phi->addIncoming(newPtr, ifBlock);
        phi->addIncoming(newPtrNull, elseBlock);
        I.replaceAllUsesWith(phi);
        I.eraseFromParent();
        changed = true;
    }

    return changed;
}

namespace IGC {
    FunctionPass* createGenericAddressAnalysisPass()
    {
        return new GenericAddressAnalysis;
    }
    FunctionPass* createGenericAddressDynamicResolutionPass()
    {
        return new GenericAddressDynamicResolution;
    }
} // namespace IGC
