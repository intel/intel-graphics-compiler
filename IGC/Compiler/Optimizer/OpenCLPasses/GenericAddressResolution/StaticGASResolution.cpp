/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "StaticGASResolution.h"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/InitializePasses.h"
#include "llvmWrapper/IR/DerivedTypes.h"

FunctionPass* IGC::createStaticGASResolution() { return new StaticGASResolution(); }

char StaticGASResolution::ID = 0;

#define PASS_FLAG "static-gas-resolution"
#define PASS_DESC "Statically resolves memory accesses operating on generic pointers"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(StaticGASResolution, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CastToGASAnalysis)
IGC_INITIALIZE_PASS_END(StaticGASResolution, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

bool StaticGASResolution::runOnFunction(llvm::Function& F)
{
    m_GI = &getAnalysis<CastToGASAnalysis>().getGASInfo();
    // Change GAS inst, such as ld/st, etc to global ld/st, etc.
    if (m_GI->canGenericPointToPrivate(F) || m_GI->canGenericPointToLocal(F))
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

    IRBuilder<> IRB(F.getContext());
    bool changed = false;
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
                PointerType* glbPtrTy = IGCLLVM::getWithSamePointeeType(PtrTy, ADDRESS_SPACE_GLOBAL);

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
    }

    return changed;
}
