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

// vim:ts=2:sw=2:et:

#include "common/LLVMUtils.h"

#include "Compiler/CISACodeGen/GenNullPointerLowering.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

namespace {

    // This pass lowers ConstantPointerNull to different integer value based on its
    // pointer type. Per C standard, null pointer is a pointer pointing to nowhere.
    // On most targets, nowhere could be represented as 0 internally. However,
    // there are the cases where 0 is a valid offset. E.g. offset 0 in SLM is valid
    // and, due to the limited space of SLM, we cannot scarify offset 0 as an
    // invalid offset. As SLM has limited size, it'd better to use an out-of-range
    // value, such as 0xFFFFFFFF, as the internal value of null pointer.
    //
    // PLEASE note that, after this pass, we still have null pointer but they
    // SHOULD be interpreted as 0 instead of null pointer as we have no way, at
    // LLVM IR level, to present pointer value in native types supported by
    // targets, e.g. 32-bit integer. A low-level IR would be more proper for this
    // purpose.

    class GenNullPointerLowering : public FunctionPass {
        const DataLayout* DL;

    public:
        static char ID;

        GenNullPointerLowering()
            : FunctionPass(ID), DL(0) {}

        virtual llvm::StringRef getPassName() const {
            return "GenNullPointer Lowering";
        }

        virtual bool runOnFunction(Function& F);

        virtual void getAnalysisUsage(AnalysisUsage& AU) const {
            AU.setPreservesAll();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }
    };
} // End of anonymous namespace

char GenNullPointerLowering::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "igc-gennullptrlowering"
#define PASS_DESCRIPTION "Lowering null pointer for GEN"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(GenNullPointerLowering, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(GenNullPointerLowering, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

// Resolve null pointer (from different address spaces, even from different
// types) to integer values. The resolved value has to be re-casted back to
// pointer due to the representation in LLVM IR.
static Constant* ResolveNullPointerValue(const DataLayout* DL, PointerType* PtrTy) {
    unsigned AS = PtrTy->getPointerAddressSpace();
    switch (AS) {
    default:
        break;
    case ADDRESS_SPACE_LOCAL: {
        Constant* C1 = Constant::getAllOnesValue(DL->getIntPtrType(PtrTy));
        return ConstantExpr::getIntToPtr(C1, PtrTy);
    }
    }

    // PLEASE DON'T CONFUSE! Even we return constant null pointer, it really
    // means 0. It's due to the limits we use LLVM IR as a low-level IR.
    return ConstantPointerNull::get(PtrTy);
}

template<typename T>
static Constant* GetNullPointerValue(const DataLayout* DL, T* C) {
    SmallVector<Constant*, 8> Constants;

    for (typename T::const_op_iterator OI = C->op_begin(),
        OE = C->op_end(); OI != OE; ++OI) {
        Constant* C0 = cast<Constant>(*OI);
        Constants.push_back(GetNullPointerValue(DL, C0));
    }

    return T::get(C->getType(), Constants);
}




template<>
Constant* GetNullPointerValue(const DataLayout* DL, Constant* C) {
    if (const ConstantPointerNull * CPN = dyn_cast<ConstantPointerNull>(C))
        return ResolveNullPointerValue(DL, CPN->getType());

    // ConstantArray
    if (ConstantArray * CA = dyn_cast<ConstantArray>(C))
        return GetNullPointerValue(DL, CA);

    // ConstantStruct
    if (ConstantStruct * CS = dyn_cast<ConstantStruct>(C))
        return GetNullPointerValue(DL, CS);

    // TODO: Add missing aggregate constants with null pointers as elements.

    // Otherwise, return the value itself.
    return C;
}

bool GenNullPointerLowering::runOnFunction(Function& F) {
    CodeGenContext* pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    MetaDataUtils* pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    // Only handle GEP for OCL so far.
    if (pCtx->type != ShaderType::OPENCL_SHADER || !isEntryFunc(pMdUtils, &F))
    {
        return false;
    }

    DL = &F.getParent()->getDataLayout();

    bool Changed = false;
    for (Function::const_iterator BB = F.begin(), BE = F.end(); BB != BE; ++BB)
        for (BasicBlock::const_iterator I = BB->begin(), E = BB->end(); I != E; ++I)
            for (Instruction::const_op_iterator Op = I->op_begin(),
                OpEnd = I->op_end(); Op != OpEnd; ++Op) {
                Constant* C = dyn_cast<Constant>(*Op);

                if (!C) continue;

                Constant* NewC = GetNullPointerValue(DL, C);

                if (C != NewC) {
                    C->replaceAllUsesWith(NewC);
                    Changed = true;
                }
            }

    DumpLLVMIR(pCtx, "nullptr-lowering");

    return Changed;
}

FunctionPass* IGC::createGenNullPointerLowerPass() {
    return new GenNullPointerLowering();
}
