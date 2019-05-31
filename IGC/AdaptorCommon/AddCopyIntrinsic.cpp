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

#include "AddCopyIntrinsic.hpp"
#include "Compiler/MetaDataApi/IGCMetaDataHelper.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "common/igc_regkeys.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Attributes.h>
#include <llvm/Support/raw_ostream.h>
#include "common/LLVMWarningsPop.hpp"
#include "common/igc_regkeys.hpp"
#include <string>


using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

namespace {

class AddCopyIntrinsic : public FunctionPass
{
public:
    static char ID;
    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.setPreservesCFG();
        AU.addRequired<MetaDataUtilsWrapper>();
        AU.addRequired<CodeGenContextWrapper>();
    }

    AddCopyIntrinsic();

    ~AddCopyIntrinsic() {}

    bool runOnFunction(Function& F) override;

    StringRef getPassName() const override
    {
        return "AddCopyIntrinsic";
    }

private:
    bool m_changed;

    bool isCandidate(AllocaInst* AI);
    bool isCandidate(CallInst* CI);
    bool isCandidate(StoreInst* SI);

    bool getElementsIfShuffleVector(
        Value* V,
        SmallVector<std::pair<Value*, int>, 16>& EltSrcs);
    Value* removeUndefWrite(Value* StoreVal);
    void handleAlloca(AllocaInst* AI);
    void handleShuffleVector(ShuffleVectorInst* SVI);

    void convertCopyBuiltin(CallInst* CI);
};

} // namespace

// Register pass to igc-opt
#define PASS_FLAG "igc-add-copy-intrinsic"
#define PASS_DESCRIPTION "Add intrinsic to mimic copy instruction"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(AddCopyIntrinsic, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
//IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(AddCopyIntrinsic, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char AddCopyIntrinsic::ID = 0;

AddCopyIntrinsic::AddCopyIntrinsic()
    : FunctionPass(ID),
      m_changed(false)
{
    initializeAddCopyIntrinsicPass(*PassRegistry::getPassRegistry());
}

FunctionPass *createAddCopyIntrinsicPass()
{
    return new AddCopyIntrinsic();
}

bool AddCopyIntrinsic::runOnFunction(Function& F)
{
//#define HANDLE_ALLOCA 1
    if (IGC_GET_FLAG_VALUE(EnableVATemp) < 2 &&
        IGC_GET_FLAG_VALUE(VATemp) == 0)
    {
        return false;
    }

    m_changed = false;
    BasicBlock *EntryBB = &(F.getEntryBlock());
    BasicBlock::iterator II = EntryBB->begin();
    BasicBlock::iterator IE = EntryBB->end();
    BasicBlock::iterator nextII = II;
    for (auto BI = F.begin(), BE = F.end(); BI != BE; ++BI)
    {
        BasicBlock* BB = &*BI;
        BasicBlock::iterator II = BB->begin();
        BasicBlock::iterator IE = BB->end();
        BasicBlock::iterator nextII = II;
        for (; II != IE; II = nextII)
        {
            // On entry, nextII == II, so it must be valid
            // to do ++;
            ++nextII;

            Instruction* I = &*II;

            // Handle user-inserted copy
            if (CallInst* CI = dyn_cast<CallInst>(I))
            {
                convertCopyBuiltin(CI);
            }

#ifdef HANDLE_ALLOCA
            if (AllocaInst* allocaInst = dyn_cast<AllocaInst>(I)) {
                handleAlloca(allocaInst);
            }
            else
#endif
            if (ShuffleVectorInst* SVI = dyn_cast<ShuffleVectorInst>(I))
            {
                handleShuffleVector(SVI);
            }
        }
    }
    return m_changed;
}

bool AddCopyIntrinsic::isCandidate(AllocaInst* AI)
{
    Type* allocatedTy = AI->getAllocatedType();
    VectorType* VTy = dyn_cast<VectorType>(allocatedTy);
    ConstantInt *ASizeVal = dyn_cast<ConstantInt>(AI->getArraySize());

    // Only handle private variables of vector type with size >= 8
    if ((!ASizeVal || ASizeVal->getZExtValue() != 1) ||
        (!VTy || VTy->getNumElements() <= 8)) {
        return false;
    }
    return true;
}

bool AddCopyIntrinsic::isCandidate(CallInst* CI)
{
    Function *func = CI->getCalledFunction();
    if (!func)
        return false;
    StringRef funcName = func->getName();

    // block read from global (internal builtins)
    if (funcName.startswith("__builtin_IB_simd_block_read"))
    {
        if (funcName.endswith("_global") ||
            funcName.endswith("_global_l") ||
            funcName.endswith("_global_h") ||
            funcName.endswith("_global_b"))
        {
            return true;
        }

        // block read from local
        else if (funcName.endswith("_local") ||
                 funcName.endswith("_local_l") ||
                 funcName.endswith("_local_h") ||
                 funcName.endswith("_local_b"))
        {
            return true;
        }
        return false;
    }
    else if (funcName.startswith("_Z") &&
             funcName.find("intel_sub_group_block_read") != StringRef::npos)
    {
        Type* Ty = CI->getArgOperand(0)->getType();
        if (PointerType* PTy = dyn_cast<PointerType>(Ty))
        {
            uint32_t AS = PTy->getPointerAddressSpace();
            bool isCand = (AS == ADDRESS_SPACE_GLOBAL);
            Type* PointeeTy = PTy->getPointerElementType();
            StructType* STy = dyn_cast<StructType>(PointeeTy);
            isCand = (isCand || (AS == ADDRESS_SPACE_LOCAL));
            // Skip media block read, may handle it later.
            if (!isCand || (STy && STy->isOpaque())) {
                return false;
            }
            return true;
        }
    }
    return false;
}


bool AddCopyIntrinsic::isCandidate(StoreInst* SI)
{
    if (SI)
    {
        Value* V = SI->getValueOperand();
        Type* Ty = V->getType();
        if (!Ty->isFirstClassType() || Ty->isAggregateType()) {
            return false;
        }

        // May need to strip bitcast if any
        if (BitCastInst* BCI = dyn_cast<BitCastInst>(V))
        {
            do {
                V = BCI->getOperand(0);
                BCI = dyn_cast<BitCastInst>(V);
            } while (BCI);
        }

        if ( false && !isa<Constant>(V)) {
            return false;
        }

        // Insert copy to keep the vector from being optimized away if
        // V is loaded from either global or SLM. This will not affect
        // propagation as there is nothing to propagate.
        if (CallInst* CI = dyn_cast<CallInst>(V)) {
            if (!isCandidate(CI))
                return false;
        }
        else if (LoadInst* LI = dyn_cast<LoadInst>(V))
        {
            uint32_t AS = LI->getPointerAddressSpace();
            if (AS != ADDRESS_SPACE_GLOBAL && AS != ADDRESS_SPACE_LOCAL) {
                return false;
            }
        }
    }
    return true;
}

// EltSrcs[i]: pair <V, ix>, which is the ix'th element of V.
// EltSrc[i]:  ith elements of SVI is V's ix'th element.
bool AddCopyIntrinsic::getElementsIfShuffleVector(
    Value* V,
    SmallVector<std::pair<Value*, int>, 16>& EltSrcs)
{
    ShuffleVectorInst* SVI = dyn_cast<ShuffleVectorInst>(V);
    if (!SVI) {
        return false;
    }

    Value* S0 = SVI->getOperand(0);
    Value* S1 = SVI->getOperand(1);
    Constant* Mask = cast<Constant>(SVI->getOperand(2));
    VectorType* VTy = cast<VectorType>(SVI->getType());
    int nelts = (int)VTy->getNumElements();

    VectorType* SVTy = cast<VectorType>(S0->getType());
    int src_nelts = (int)SVTy->getNumElements();

    SmallVector<std::pair<Value*, int>, 16> S0_Elts(src_nelts);
    SmallVector<std::pair<Value*, int>, 16> S1_Elts(src_nelts);
    bool isS0Shuffle = getElementsIfShuffleVector(S0, S0_Elts);
    bool isS1Shuffle = getElementsIfShuffleVector(S1, S1_Elts);
    for (int i = 0; i < nelts; ++i)
    {
        Constant* IxVal = Mask->getAggregateElement(i);
        if (isa<UndefValue>(IxVal)) {
            continue;
        }
        ConstantInt* CIxVal = cast<ConstantInt>(IxVal);
        int ix = (int)CIxVal->getZExtValue();
        if (ix < src_nelts) {
            if (isS0Shuffle) {
                std::pair<Value*, int>& From0 = S0_Elts[ix];
                EltSrcs[i] = From0;
            }
            else {
                EltSrcs[i] = std::make_pair(S0, ix);
            }
        }
        else {
            int ix1 = ix - src_nelts;
            if (isS1Shuffle) {
                std::pair<Value*, int>& From1 = S1_Elts[ix1];
                EltSrcs[i] = From1;
            }
            else {
                EltSrcs[i] = std::make_pair(S1, ix1);
            }
        }
    }
    return true;
}


Value* AddCopyIntrinsic::removeUndefWrite(Value* V)
{
    Value* newVec = V;
    if (ShuffleVectorInst* SVI = dyn_cast<ShuffleVectorInst>(V))
    {
        VectorType* VTy = cast<VectorType>(V->getType());
        int nelts = (int)VTy->getNumElements();
        SmallVector<std::pair<Value*, int>, 16> V_Elts(nelts);

        BasicBlock::iterator II(SVI);
        ++II;
        Instruction* IEI_insertBefore = &*II;

        Type* I32Ty = Type::getInt32Ty(SVI->getContext());
        (void)getElementsIfShuffleVector(SVI, V_Elts);
        newVec = UndefValue::get(VTy);
        for (int i = 0; i < nelts; ++i)
        {
            std::pair<Value*, int>& From = V_Elts[i];
            if (From.first)
            {
                Value* newElt = ExtractElementInst::Create(
                    From.first,
                    ConstantInt::get(I32Ty, From.second),
                    "",
                    SVI);

                newVec = InsertElementInst::Create(
                    newVec,
                    newElt,
                    ConstantInt::get(I32Ty, i),
                    "",
                    IEI_insertBefore);
            }
        }

        SVI->replaceAllUsesWith(newVec);
        SVI->eraseFromParent();
    }
    return newVec;
}

void  AddCopyIntrinsic::handleAlloca(AllocaInst* AI)
{
    if (!isCandidate(AI))
        return;

    for (auto user : AI->users())
    {
        StoreInst* SI = dyn_cast<StoreInst>(user);
        if (!SI || !isCandidate(SI))
            continue;

        Value* storeVal = SI->getValueOperand();
        storeVal = removeUndefWrite(storeVal);
        if (!storeVal->hasOneUse())
            continue;

        Value*  args[1];
        args[0] = storeVal;

        Type* ITys[1] = { args[0]->getType() };
        Function* copyIntrin = GenISAIntrinsic::getDeclaration(
            AI->getParent()->getParent()->getParent(),
            GenISAIntrinsic::GenISA_Copy,
            ITys);

        Instruction* copyCall = CallInst::Create(copyIntrin, args, "copy", SI);

        copyCall->setDebugLoc(SI->getDebugLoc());
        SI->setOperand(0, copyCall);

        m_changed = true;
    }
}

void AddCopyIntrinsic::handleShuffleVector(ShuffleVectorInst* SVI)
{
    VectorType* VTy = cast<VectorType>(SVI->getType());
    int nelts = (int)VTy->getNumElements();

    if (nelts <= 8)
        return;

    SmallVector<std::pair<Value*, int>, 16> Elts(nelts);
    (void)getElementsIfShuffleVector(SVI, Elts);

    // Only insert Copy intrinsic if
    //    1. there is no element whose value is undef
    //    2. the source are from load/store (too strong?)
    for (int i = 0; i < nelts; ++i) {
        auto& elt = Elts[i];
        Value* Src = elt.first;

        if (nullptr == Src)
            return;

        if (LoadInst* LI = dyn_cast<LoadInst>(Src))
        {
            uint32_t AS = LI->getPointerAddressSpace();
            if (AS != ADDRESS_SPACE_GLOBAL && AS != ADDRESS_SPACE_LOCAL) {
                return;
            }
        }
        else if (GenIntrinsicInst* GII = dyn_cast<GenIntrinsicInst>(Src))
        {
            GenISAIntrinsic::ID id = GII->getIntrinsicID();
            if (id != GenISAIntrinsic::GenISA_simdBlockRead) {
                return;
            }
        }
        else {
            return;
        }
    }

    // Insert Copy intrinsic
    //   1. recreate vector with extract/insert element insts
    //   2. Create a copy intrinsic

    // Let ExtractElement be inserted before SVI, and InsertElement
    // before the next instruction, so all ExtractElements go before
    // InsertElements.
    BasicBlock::iterator II(SVI);
    ++II;
    Instruction* IEI_insertBefore = &*II;

    Type* I32Ty = Type::getInt32Ty(SVI->getContext());
    Value* newVec = UndefValue::get(VTy);
    for (int i = 0; i < nelts; ++i)
    {
        auto& From = Elts[i];
        assert(From.first && "ICE: Src vector should not be null at this point!");
        Value* newElt = ExtractElementInst::Create(
            From.first,
            ConstantInt::get(I32Ty, From.second),
            "",
            SVI);

        newVec = InsertElementInst::Create(
            newVec,
            newElt,
            ConstantInt::get(I32Ty, i),
            "",
            IEI_insertBefore);
    }

    // Finally, insert copy intrinsic
    Value*  args[1];
    args[0] = newVec;

    Type* ITys[1] = { args[0]->getType() };
    Function* copyIntrin = GenISAIntrinsic::getDeclaration(
        SVI->getParent()->getParent()->getParent(),
        GenISAIntrinsic::GenISA_Copy,
        ITys);

    Instruction* copyCall = CallInst::Create(copyIntrin, args, "copy", IEI_insertBefore);

    copyCall->setDebugLoc(SVI->getDebugLoc());

    SVI->replaceAllUsesWith(copyCall);
    SVI->eraseFromParent();

    m_changed = true;
}

void AddCopyIntrinsic::convertCopyBuiltin(CallInst* CI)
{
    Function *func = CI->getCalledFunction();
    if (!func)
        return;

    // Valid names are in the form:
    //   __builtin_IB_RegCopy_[pg|pl][s|u][c|s|i|l][1|2|3|4|8|16]  // int
    //   __builtin_IB_RegCopy_[pg|pl][d|f|h][1|2|3|4|8|16]         // float
    //   For pointer, pg:  global pointer;  pl: local pointer.
    StringRef funcName = func->getName();
    if (funcName.startswith("__builtin_IB_RegCopy_"))
    {
        int sz = (int)funcName.size();
        int pos = (int)sizeof("__builtin_IB_RegCopy_") - 1;
        int rem = sz - pos;
        if (rem < 2 || rem > 6) {
            return;
        }

        if (funcName[pos] == 'p') {
            --rem;
            ++pos;
            if (funcName[pos] == 'g' || funcName[pos] == 'l') {
                --rem;
                ++pos;
            }
            else
                return;
        }

        if (rem < 2 || rem > 4) {
            return;
        }

        switch (funcName[pos]) {
        case 'f':
        case 'd':
        case 'h':
        {
            --rem;
            ++pos;
            break;
        }
        case 'u':
        case 's':
        {
            ++pos;
            --rem;
            switch (funcName[pos])
            {
            case 'c':
            case 's':
            case 'i':
            case 'l':
                ++pos;
                --rem;
                break;
            default:
                return;
            }
            break;
        }
        default:
            return;
        }

        if (rem != 1 && rem != 2) {
            return;
        }
        // rem == 1 || rem == 2
        int vsize = 0;
        for (int i = 0; i < rem; ++i)
        {
            int t = (int)funcName[pos + i] - '0';
            if (t < 0 || t > 9)
                return;
            vsize = vsize * 10 + t;
        }
        if (vsize < 1)
            return;

        Value *args[1] = { CI->getArgOperand(0) };
        Type* ITys[1] = { args[0]->getType() };
        Function* copyIntrin = GenISAIntrinsic::getDeclaration(
            CI->getParent()->getParent()->getParent(),
            GenISAIntrinsic::GenISA_Copy,
            ITys);

        Instruction* copyCall = CallInst::Create(copyIntrin, args, "copy", CI);

        copyCall->setDebugLoc(CI->getDebugLoc());
        CI->replaceAllUsesWith(copyCall);
        CI->eraseFromParent();

        m_changed = true;
    }
    return;
}