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

#include "Compiler/Optimizer/OpenCLPasses/GenericAddressResolution/GenericAddressDynamicResolution.hpp"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CodeGenPublicEnums.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/DataLayout.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

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
#define PASS_DESCRIPTION2 "Inserts implicit arguments when generic pointers are used"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 false
IGC_INITIALIZE_PASS_BEGIN(GenericAddressAnalysis, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
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
                auto implicitArgs = ImplicitArgs(F, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils());
                SmallVector<ImplicitArg::ArgType, 3> args;
                args.push_back(ImplicitArg::LOCAL_MEMORY_STATELESS_WINDOW_START_ADDRESS);
                args.push_back(ImplicitArg::LOCAL_MEMORY_STATELESS_WINDOW_SIZE);
                args.push_back(ImplicitArg::PRIVATE_MEMORY_STATELESS_SIZE);
                ImplicitArgs::addImplicitArgs(F, args, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils());
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
        }

        virtual bool runOnFunction(Function& F) override;

        bool visitLoadStoreInst(Instruction& I);
        bool visitIntrinsicCall(CallInst& I);
        Module* getModule() { return m_module; }

    private:
        Value* addIsAddrSpaceComparison(Value* pointer, Instruction* insertPoint, unsigned targetAS);
        Type* getPointerAsIntType(LLVMContext& Ctx, unsigned AS);
        Value* getAddrSpaceWindowEndAddress(Instruction& insertPoint, unsigned targetAS);
        void resolveGAS(Instruction& I, Value* pointerOperand, unsigned targetAS);
    };

} // namespace


// Register pass to igc-opt
#define PASS_FLAG "igc-generic-address-dynamic-resolution"
#define PASS_DESCRIPTION "Resolve generic address space loads/stores"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(GenericAddressDynamicResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(GenericAddressDynamicResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char GenericAddressDynamicResolution::ID = 0;

bool GenericAddressDynamicResolution::runOnFunction(Function& F)
{
    m_module = F.getParent();
    bool modified = false;

    DataLayout dataLayout = F.getParent()->getDataLayout();

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

Value* GenericAddressDynamicResolution::addIsAddrSpaceComparison(Value* pointer, Instruction* insertPoint, const unsigned targetAS)
{
    Function* func = insertPoint->getParent()->getParent();

    ImplicitArgs implicitArgs = ImplicitArgs(*func, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils());
    Value* windowStartPtr = targetAS == ADDRESS_SPACE_LOCAL
        ? implicitArgs.getImplicitArg(*func, ImplicitArg::LOCAL_MEMORY_STATELESS_WINDOW_START_ADDRESS)
        : implicitArgs.getImplicitArg(*func, ImplicitArg::PRIVATE_BASE);

    Type* intPtrTy = getPointerAsIntType(pointer->getContext(), ADDRESS_SPACE_GENERIC);

    // (ptr >= window_start) & [ptr < (window_start + window_size)]
    Value* ptrAsInt = PtrToIntInst::Create(Instruction::PtrToInt, pointer, intPtrTy, "", insertPoint);
    Value* windowStartAsInt = nullptr;

    if (windowStartPtr) {
        windowStartAsInt = PtrToIntInst::Create(Instruction::PtrToInt, windowStartPtr, intPtrTy, "", insertPoint);
    }
    else {
        // Kernel might not have implicit argument for ImplicitArg::PRIVATE_BASE
        windowStartAsInt = ConstantInt::get(intPtrTy, 0);
    }

    Value* windowEnd = getAddrSpaceWindowEndAddress(*insertPoint, targetAS);
    Value* cmpLowerBound = CmpInst::Create(Instruction::ICmp, CmpInst::ICMP_UGE, ptrAsInt, windowStartAsInt, "CmpWindowLowerBound", insertPoint);
    Value* cmpUpperBound = CmpInst::Create(Instruction::ICmp, CmpInst::ICMP_ULT, ptrAsInt, windowEnd, "CmpWindowUpperBound", insertPoint);
    Value* isInWindow = BinaryOperator::CreateAnd(cmpLowerBound, cmpUpperBound, "isPtrInWindow", insertPoint);

    return isInWindow;
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
        report_fatal_error("Unable to resolve generic address space pointer");
    }

    if (pointerAddressSpace == ADDRESS_SPACE_GENERIC) {
        // Add runtime check to see whether I is a load/store on local addrspace.
        resolveGAS(I, pointerOperand, ADDRESS_SPACE_LOCAL);
        changed = true;
    }

    return changed;
}

Value* GenericAddressDynamicResolution::getAddrSpaceWindowEndAddress(Instruction& insertPoint, const unsigned targetAS)
{
    Function* pCurrentFunc = insertPoint.getParent()->getParent();
    ImplicitArgs implicitArgs = ImplicitArgs(*pCurrentFunc, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils());
    Argument* windowStart = nullptr, * windowSize = nullptr;

    if (targetAS == ADDRESS_SPACE_LOCAL) {
        windowStart = implicitArgs.getImplicitArg(*pCurrentFunc, ImplicitArg::LOCAL_MEMORY_STATELESS_WINDOW_START_ADDRESS);
        windowSize = implicitArgs.getImplicitArg(*pCurrentFunc, ImplicitArg::LOCAL_MEMORY_STATELESS_WINDOW_SIZE);
    }
    else if (targetAS == ADDRESS_SPACE_PRIVATE) {
        windowStart = implicitArgs.getImplicitArg(*pCurrentFunc, ImplicitArg::PRIVATE_BASE);
        windowSize = implicitArgs.getImplicitArg(*pCurrentFunc, ImplicitArg::PRIVATE_MEMORY_STATELESS_SIZE);
    }
    else
        assert(false && "Unknown AddrSpace");

    if ((windowStart != nullptr) && (windowSize != nullptr)) {
        Type* intPtrTy = getPointerAsIntType(windowStart->getContext(), ADDRESS_SPACE_GENERIC);
        Value* windowStartAsInt = PtrToIntInst::Create(Instruction::PtrToInt, windowStart, intPtrTy, "", &insertPoint);
        Value* windowSizeAsInt = CastInst::CreateZExtOrBitCast(windowSize, intPtrTy, "", &insertPoint);
        Value* windowEnd = BinaryOperator::CreateAdd(windowStartAsInt, windowSizeAsInt, "localWindowEnd", &insertPoint);
        return windowEnd;
    }
    else if ((windowStart == nullptr) && (windowSize != nullptr)) {
        // Assume start from 0 if windowStart is not defined.
        Type* intPtrTy = getPointerAsIntType(pCurrentFunc->getContext(), ADDRESS_SPACE_GENERIC);
        return CastInst::CreateZExtOrBitCast(windowSize, intPtrTy, "", &insertPoint);
    }
    else
        assert(false && "AddrSpace without limit");

    return ConstantInt::get(Type::getInt32Ty(insertPoint.getContext()), 0);
}

void GenericAddressDynamicResolution::resolveGAS(Instruction& I, Value* pointerOperand, const unsigned targetAS)
{
    Value* isPtrInLocalWindow = addIsAddrSpaceComparison(pointerOperand, &I, targetAS);
    PointerType* pointerType = dyn_cast<PointerType>(pointerOperand->getType());
    IRBuilder<> builder(&I);

    BasicBlock* currentBlock = I.getParent();
    BasicBlock* convergeBlock = currentBlock->splitBasicBlock(&I);
    BasicBlock* localLoadBlock = BasicBlock::Create(I.getContext(), "LocalLoadBlock", convergeBlock->getParent(), convergeBlock);
    BasicBlock* nonLocalLoadBlock = BasicBlock::Create(I.getContext(), "GlobalPrivateLoadBlock", convergeBlock->getParent(), convergeBlock);

    Value* localLoad = nullptr;
    Value* nonLocalLoad = nullptr;

    // if is_local(ptr)
    {
        IRBuilder<> localBuilder(localLoadBlock);
        PointerType* localPtrType = pointerType->getElementType()->getPointerTo(targetAS);
        Value* localPtr = localBuilder.CreateAddrSpaceCast(pointerOperand, localPtrType);
        if (LoadInst * LI = dyn_cast<LoadInst>(&I)) {
            localLoad = localBuilder.CreateAlignedLoad(localPtr, LI->getAlignment(), LI->isVolatile(), "localLoad");
        }
        else if (StoreInst * SI = dyn_cast<StoreInst>(&I)) {
            localBuilder.CreateAlignedStore(I.getOperand(0), localPtr, SI->getAlignment(), SI->isVolatile());
        }
        else {
            // Inst I is a to_local(pointerOperand) call, and we can use localPtr as I's result.
            localLoad = localPtr;
        }
        localBuilder.CreateBr(convergeBlock);
    }

    // else (is either global or private)
    {
        IRBuilder<> nonLocalBuilder(nonLocalLoadBlock);
        PointerType* ptrType = pointerType->getElementType()->getPointerTo(ADDRESS_SPACE_GLOBAL_OR_PRIVATE);
        Value* nonLocalPtr = nonLocalBuilder.CreateAddrSpaceCast(pointerOperand, ptrType);

        if (LoadInst * LI = dyn_cast<LoadInst>(&I)) {
            nonLocalLoad = nonLocalBuilder.CreateAlignedLoad(nonLocalPtr, LI->getAlignment(), LI->isVolatile(), "globalOrPrivateLoad");
        }
        else if (StoreInst * SI = dyn_cast<StoreInst>(&I)) {
            nonLocalBuilder.CreateAlignedStore(I.getOperand(0), nonLocalPtr, SI->getAlignment(), SI->isVolatile());
        }
        else {
            // Inst I is a to_local(pointerOperand) call, and we can use null as I's result.
            nonLocalLoad = Constant::getNullValue(pointerType->getElementType()->getPointerTo(targetAS));
        }
        nonLocalBuilder.CreateBr(convergeBlock);
    }

    currentBlock->getTerminator()->eraseFromParent();
    builder.SetInsertPoint(currentBlock);
    builder.CreateCondBr(isPtrInLocalWindow, localLoadBlock, nonLocalLoadBlock);

    if ((localLoad != nullptr) && (nonLocalLoad != nullptr)) {
        IRBuilder<> phiBuilder(&(*convergeBlock->begin()));
        PHINode* phi = phiBuilder.CreatePHI(I.getType(), 2, I.getName());
        phi->addIncoming(localLoad, localLoadBlock);
        phi->addIncoming(nonLocalLoad, nonLocalLoadBlock);
        I.replaceAllUsesWith(phi);
    }

    I.eraseFromParent();
}

bool GenericAddressDynamicResolution::visitIntrinsicCall(CallInst& I)
{
    bool changed = false;
    Function* pCurrentFunc = I.getParent()->getParent();
    Function* pCalledFunc = I.getCalledFunction();
    if (pCalledFunc == nullptr) {
        // Indirect call
        return false;
    }

    ImplicitArgs implicitArgs = ImplicitArgs(*pCurrentFunc, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils());
    StringRef funcName = pCalledFunc->getName();

    if ((funcName == "__builtin_IB_to_local") || (funcName == "__builtin_IB_to_private")) {
        assert(I.getNumArgOperands() == 1);
        Value* arg = I.getArgOperand(0);
        const unsigned targetAS = cast<PointerType>(I.getType())->getAddressSpace();

        //
        // First to check whether we can simplify trivial cases like addrspacecast from
        // global/private to local, or from local to local.
        //
        if (AddrSpaceCastInst * AI = dyn_cast<AddrSpaceCastInst>(arg)) {
            //    to_local(__global*)  -> null
            //    to_local(__private*) -> null
            PointerType* ptrType = cast<PointerType>(AI->getSrcTy());
            if ((ptrType->getAddressSpace() != targetAS) && (ptrType->getAddressSpace() != ADDRESS_SPACE_GENERIC)) {
                Constant* np = Constant::getNullValue(I.getType());
                I.replaceAllUsesWith(np);
                I.eraseFromParent();
                changed = true;
            }
            else if (ptrType->getAddressSpace() == targetAS) {
                //  to_local(__local*)  -> __local*
                I.replaceAllUsesWith(AI->getOperand(0));
                I.eraseFromParent();
                changed = true;
            }
        }

        // Add runtime check to resolve GAS for non-trivial cases.
        if (!changed) {
            resolveGAS(I, arg, targetAS);
            changed = true;
        }
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
