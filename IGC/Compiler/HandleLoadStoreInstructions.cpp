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
#include "Compiler/HandleLoadStoreInstructions.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-dp-to-fp-load-store"
#define PASS_DESCRIPTION "Convert load/store on doubles into store/loads on i32 or float types"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(HandleLoadStoreInstructions, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(HandleLoadStoreInstructions, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char HandleLoadStoreInstructions::ID = 0;

HandleLoadStoreInstructions::HandleLoadStoreInstructions() : FunctionPass(ID)
{
    initializeHandleLoadStoreInstructionsPass(*PassRegistry::getPassRegistry());
}

bool HandleLoadStoreInstructions::runOnFunction(llvm::Function& F)
{
    m_changed = false;
    visit(F);
    return m_changed;
}

void HandleLoadStoreInstructions::visitLoadInst(llvm::LoadInst& I)
{
    llvm::IRBuilder<> builder(&I);
    llvm::Value* ptrv = llvm::cast<llvm::LoadInst>(I).getPointerOperand();

    if (I.getType()->isDoubleTy() ||
        (I.getType()->isVectorTy() &&
            I.getType()->getVectorElementType()->isDoubleTy()))
    {
        // scalar/vector double instruction
        // Found an instruction of type
        // %11 = load <2 x double> addrspace(8585216)* %10, align 16
        uint32_t numVectorElements = 1;
        llvm::Type* doubleDstType = builder.getDoubleTy();

        if (I.getType()->isVectorTy())
        {
            numVectorElements = I.getType()->getVectorNumElements();
            doubleDstType = llvm::VectorType::get(builder.getDoubleTy(), numVectorElements);
        }
        uint as = ptrv->getType()->getPointerAddressSpace();
        BufferType bufType = GetBufferType(as);
        llvm::Value* newInst = nullptr;
        // WA: driver does not support gather4(untyped surface read) from constant buffer
        if (bufType == CONSTANT_BUFFER)
        {
            llvm::Type* floatyptr = llvm::PointerType::get(builder.getFloatTy(), as);
            llvm::Value* byteOffset = nullptr;
            if (!isa<ConstantPointerNull>(ptrv))
            {
                if (ConstantExpr * ptrExpr = dyn_cast<ConstantExpr>(ptrv))
                {
                    assert(ptrExpr->getOpcode() == Instruction::IntToPtr);
                    byteOffset = ptrExpr->getOperand(0);
                }
                else if (IntToPtrInst * i2p = dyn_cast<IntToPtrInst>(ptrv))
                {
                    byteOffset = i2p->getOperand(0);
                }
                else
                {
                    byteOffset = builder.CreatePtrToInt(ptrv, builder.getInt32Ty());
                }
            }

            Value* vec = UndefValue::get(llvm::VectorType::get(builder.getFloatTy(), numVectorElements * 2));
            for (unsigned int i = 0; i < numVectorElements * 2; ++i)
            {
                Value* offset = builder.getInt32(i * 4);
                if (byteOffset)
                {
                    offset = builder.CreateAdd(byteOffset, offset);
                }
                // new IntToPtr and new load
                // cannot use irbuilder to create IntToPtr. It may create ConstantExpr instead of instruction
                Value* i2p = llvm::IntToPtrInst::Create(Instruction::IntToPtr, offset, floatyptr, "splitDouble", &I);
                Value* data = builder.CreateLoad(i2p);
                vec = builder.CreateInsertElement(vec, data, builder.getInt32(i));
            }
            newInst = builder.CreateBitCast(vec, doubleDstType);
        }
        // End of WA
        else
        {
            // double to <floatx2> ; <doublex2> to <floatx4>
            llvm::Type* dataType = llvm::VectorType::get(builder.getFloatTy(), numVectorElements * 2);
            llvm::PointerType* ptrType = llvm::PointerType::get(dataType, ptrv->getType()->getPointerAddressSpace());
            ptrv = mutatePtrType(ptrv, ptrType, builder);
            Value* newLoad = builder.CreateLoad(ptrv);
            newInst = builder.CreateBitCast(newLoad, doubleDstType);
        }
        I.replaceAllUsesWith(newInst);
        I.eraseFromParent();
        m_changed = true;
    }
    else if (I.getType()->isIntegerTy(1))
    {
        if (isa<Constant>(ptrv) || isa<IntToPtrInst>(ptrv))
        {
            llvm::PointerType* int32ptr = llvm::PointerType::get(builder.getInt32Ty(), ptrv->getType()->getPointerAddressSpace());
            ptrv = mutatePtrType(ptrv, int32ptr, builder);
            Value* newLoad = builder.CreateLoad(ptrv);
            Value* newInst = builder.CreateTrunc(newLoad, builder.getInt1Ty());
            I.replaceAllUsesWith(newInst);
            I.eraseFromParent();
            m_changed = true;
        }
    }
}

void HandleLoadStoreInstructions::visitStoreInst(llvm::StoreInst& I)
{
    llvm::IRBuilder<> builder(&I);
    llvm::Value* ptrv = llvm::cast<llvm::StoreInst>(I).getPointerOperand();
    if (I.getValueOperand()->getType()->isDoubleTy() ||
        (I.getValueOperand()->getType()->isVectorTy() &&
            I.getValueOperand()->getType()->getVectorElementType()->isDoubleTy()))
    {
        // scalar/vector double instruction
        uint32_t numVectorElements = 1;

        if (I.getValueOperand()->getType()->isVectorTy())
        {
            numVectorElements = I.getValueOperand()->getType()->getVectorNumElements();
        }


        // %9 = bitcast double addrspace(8519681)* %8 to <2 x float> addrspace(8519681)*
        llvm::Type* floatDatType = llvm::VectorType::get(builder.getFloatTy(), numVectorElements * 2);
        llvm::PointerType* floatPtrType = llvm::PointerType::get(floatDatType, ptrv->getType()->getPointerAddressSpace());
        ptrv = mutatePtrType(ptrv, floatPtrType, builder);

        // %10 = bitcast double %4 to <2 x float>
        llvm::Value* srcFloatInst = builder.CreateBitCast(I.getValueOperand(), floatDatType);
        llvm::cast<llvm::StoreInst>(I).setOperand(0, srcFloatInst);
        m_changed = true;
    }
}
