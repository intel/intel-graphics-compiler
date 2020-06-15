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

#include "AdaptorCommon/IRUpgrader/IRUpgrader.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "Compiler/CISACodeGen/helper.h"

#include <vector>

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;

class UpgradeResourceAccess : public FunctionPass, public InstVisitor<UpgradeResourceAccess>
{
public:
    UpgradeResourceAccess() : FunctionPass(ID) {}
    static char ID;
    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.setPreservesCFG();
    }
    bool runOnFunction(llvm::Function &F) override;
    void visitCallInst(CallInst& C);
private:
    void ChangeIntrinsic(CallInst& C, GenISAIntrinsic::ID ID);
    void FixSamplerSignature(SampleIntrinsic* sample);
    bool m_changed = false;
};
char UpgradeResourceAccess::ID = 0;

bool UpgradeResourceAccess::runOnFunction(llvm::Function &F)
{
    visit(F);
    return m_changed;
}

static Value* GetResource(Module* m, IRBuilder<>& builder, Value* i)
{
    unsigned int addrSpace = IGC::EncodeAS4GFXResource(*i, IGC::RESOURCE, 0);
    PointerType* ptrT = PointerType::get(i->getType(), addrSpace);
    Value* img = nullptr;

    if (i->getType() != builder.getInt32Ty())
    {
        // do not make any pointer conversion
        img = i;
    }
    else if(isa<ConstantInt>(i))
    {
        img = ConstantPointerNull::get(ptrT);
    }
    else
    {
        Function* getBufferPointer = GenISAIntrinsic::getDeclaration(m, GenISAIntrinsic::GenISA_GetBufferPtr, ptrT);
        img = builder.CreateCall(getBufferPointer, { i, builder.getInt32(IGC::RESOURCE) });
    }
    return img;
}

static Value* GetSampler(Module* m, IRBuilder<>& builder, Value* i)
{
    unsigned int addrSpace = IGC::EncodeAS4GFXResource(*i, IGC::SAMPLER, 0);
    PointerType* ptrT = PointerType::get(i->getType(), addrSpace);
    Value* s = nullptr;
    if(isa<ConstantInt>(i))
    {
        s = ConstantPointerNull::get(ptrT);
    }
    else
    {
        Function* getBufferPointer = GenISAIntrinsic::getDeclaration(m, GenISAIntrinsic::GenISA_GetBufferPtr, ptrT);
        s = builder.CreateCall(getBufferPointer, { i, builder.getInt32(IGC::SAMPLER) });
    }
    return s;
}

void UpgradeResourceAccess::ChangeIntrinsic(CallInst& C, GenISAIntrinsic::ID ID)
{
    std::vector<Type*> types;
    std::vector<Value*> args;
    IRBuilder<> builder(&C);
    Module* m = C.getParent()->getParent()->getParent();
    for(unsigned int i = 0; i < C.getNumArgOperands(); i++)
    {
        args.push_back(C.getOperand(i));
    }
    switch(ID)
    {
    case GenISAIntrinsic::GenISA_sampleptr:
    case GenISAIntrinsic::GenISA_sampleBptr:
    case GenISAIntrinsic::GenISA_sampleCptr:
    case GenISAIntrinsic::GenISA_sampleBCptr:
    case GenISAIntrinsic::GenISA_sampleLptr:
    case GenISAIntrinsic::GenISA_sampleDptr:
    case GenISAIntrinsic::GenISA_sampleKillPix: {
        types.push_back(C.getType());
        types.push_back(C.getOperand(0)->getType());
        unsigned int resIndex = C.getNumOperands() - 6;
        unsigned int samplerIndex = C.getNumOperands() - 5;
        args[resIndex] = GetResource(m, builder, args[resIndex]);
        args[samplerIndex] = GetSampler(m, builder, args[samplerIndex]);
        types.push_back(args[resIndex]->getType());
        types.push_back(args[samplerIndex]->getType());
    }
        break;
    case GenISAIntrinsic::GenISA_gather4ptr: {
        types.push_back(C.getType());
        types.push_back(C.getOperand(0)->getType());
        unsigned int resIndex = C.getNumOperands() - 7;
        unsigned int samplerIndex = C.getNumOperands() - 6;
        args[resIndex] = GetResource(m, builder, args[resIndex]);
        args[samplerIndex] = GetSampler(m, builder, args[samplerIndex]);
        types.push_back(args[resIndex]->getType());
        types.push_back(args[samplerIndex]->getType());
    }
        break;
    case GenISAIntrinsic::GenISA_ldmsptr:
    case GenISAIntrinsic::GenISA_ldptr: {
        types.push_back(C.getType());
        unsigned int resIndex = C.getNumOperands() - 5;
        args[resIndex] = GetResource(m, builder, args[resIndex]);
        types.push_back(args[resIndex]->getType());
    }
        break;
    case GenISAIntrinsic::GenISA_ldmcsptr: {
        types.push_back(C.getType());
        types.push_back(C.getArgOperand(0)->getType());
        unsigned int resIndex = C.getNumOperands() - 5;
        args[resIndex] = GetResource(m, builder, args[resIndex]);
        types.push_back(args[resIndex]->getType());
    }
        break;
    default:
        IGC_ASSERT_MESSAGE(0, "unhandled intrinsic upgrade");
        break;
    }
    Function* f = GenISAIntrinsic::getDeclaration(m, ID, types);
    Value* newCall = builder.CreateCall(f, args);
    C.replaceAllUsesWith(newCall);
    C.eraseFromParent();
    m_changed = true;
}

void UpgradeResourceAccess::visitCallInst(CallInst& C)
{
    if (C.isInlineAsm())
    {
        return;
    }

    //Check in place for Indirect Calls
    if(!C.getCalledFunction())
    {
        return;
    }

    //Note : This upgrader pass is handling legacy intrinsics as well,
    // hence we might be dealing with intrinsics starting with @genx.
    if(C.getCalledFunction()->getName().contains("genx.GenISA.sample."))
    {
        ChangeIntrinsic(C, GenISAIntrinsic::GenISA_sampleptr);
    }
    else if(C.getCalledFunction()->getName().contains("genx.GenISA.sampleB."))
    {
        ChangeIntrinsic(C, GenISAIntrinsic::GenISA_sampleBptr);
    }
    else if(C.getCalledFunction()->getName().contains("genx.GenISA.sampleD."))
    {
        ChangeIntrinsic(C, GenISAIntrinsic::GenISA_sampleDptr);
    }
    else if(C.getCalledFunction()->getName().contains("genx.GenISA.sampleC."))
    {
        ChangeIntrinsic(C, GenISAIntrinsic::GenISA_sampleCptr);
    }
    else if(C.getCalledFunction()->getName().contains("genx.GenISA.sampleL."))
    {
        ChangeIntrinsic(C, GenISAIntrinsic::GenISA_sampleLptr);
    }
    else if(C.getCalledFunction()->getName().contains("genx.GenISA.gather4."))
    {
        ChangeIntrinsic(C, GenISAIntrinsic::GenISA_gather4ptr);
    }
    else if(C.getCalledFunction()->getName().contains("genx.GenISA.ldms."))
    {
        ChangeIntrinsic(C, GenISAIntrinsic::GenISA_ldmsptr);
    }
    else if(C.getCalledFunction()->getName().equals("llvm.genx.GenISA.ldmcs") ||
        C.getCalledFunction()->getName().equals("genx.GenISA.ldmcs"))
    {
        ChangeIntrinsic(C, GenISAIntrinsic::GenISA_ldmcsptr);
    }
    else if(C.getCalledFunction()->getName().contains("genx.GenISA.ld."))
    {
        ChangeIntrinsic(C, GenISAIntrinsic::GenISA_ldptr);
    }
    else if(C.getCalledFunction()->getName().contains("genx.GenISA.sampleKill.legacy"))
    {
        ChangeIntrinsic(C, GenISAIntrinsic::GenISA_sampleKillPix);
    }
    else if(SampleIntrinsic* sample = dyn_cast<SampleIntrinsic>(&C))
    {
        if(!sample->getTextureValue()->getType()->isPointerTy())
        {
            ChangeIntrinsic(*sample, sample->getIntrinsicID());
        }
    }
}

class UpgradeGenIntrinsicPrefix : public ModulePass, public InstVisitor<UpgradeGenIntrinsicPrefix>
{
public:
    UpgradeGenIntrinsicPrefix() : ModulePass(ID) {}

    static char ID;
    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.setPreservesCFG();
    }
    //bool runOnFunction(llvm::Function &F) override;
    bool runOnModule(llvm::Module &M) override;

    llvm::StringRef getPassName() const override
    {
        return "UpgradeGenIntrinsicPrefix";
    }

private:
    bool m_changed = false;
};
char UpgradeGenIntrinsicPrefix::ID = 0;

bool UpgradeGenIntrinsicPrefix::runOnModule(llvm::Module &M)
{
    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
        Function* pFunc = &(*I);
        if (pFunc->getName().startswith("genx."))
            pFunc->setName("llvm." + pFunc->getName());
    }

    return m_changed;
}


namespace IGC
{
Pass* CreateUpgradeResourceIntrinsic()
{
    return new UpgradeResourceAccess();
}

Pass* CreateUpgradeGenIntrinsicPrefix()
{
    return new UpgradeGenIntrinsicPrefix();
}


}
