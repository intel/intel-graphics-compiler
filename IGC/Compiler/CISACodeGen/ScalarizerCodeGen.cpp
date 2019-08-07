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

#include "Compiler/IGCPassSupport.h"
#include "Compiler/InitializePasses.h"
#include "common/Types.hpp"
#include "ScalarizerCodeGen.hpp"

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "igc-scalarizer-in-codegen"
#define PASS_DESCRIPTION "Scalarizer in codegen"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ScalarizerCodeGen, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(ScalarizerCodeGen, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ScalarizerCodeGen::ID = 0;

#define DEBUG_TYPE "ScalarizerCodeGen"

ScalarizerCodeGen::ScalarizerCodeGen() : FunctionPass(ID)
{
    initializeScalarizerCodeGenPass(*PassRegistry::getPassRegistry());
}


bool ScalarizerCodeGen::runOnFunction(Function& F)
{
    llvm::IRBuilder<> builder(F.getContext());
    m_builder = &builder;

    visit(F);
    return false;
}

void ScalarizerCodeGen::visitBinaryOperator(llvm::BinaryOperator& I)
{
    // Scalarizing vector type And/Or instructions
    if (I.getOpcode() == Instruction::And || I.getOpcode() == Instruction::Or || I.getOpcode() == Instruction::Xor)
    {
        if (I.getType()->isVectorTy())
        {
            bool isNewTypeVector = false;

            VectorType* instType = cast<VectorType>(I.getType());
            unsigned numElements = int_cast<unsigned>(instType->getNumElements());
            unsigned scalarSize = instType->getScalarSizeInBits();
            unsigned newScalarBits = numElements * scalarSize;
            Type* newType = nullptr;
            //  Check if the operands can be bitcasted to types int8/16/32
            if (newScalarBits == 8)
                newType = m_builder->getInt8Ty();
            else if (newScalarBits == 16)
                newType = m_builder->getInt16Ty();
            else if (newScalarBits == 32)
                newType = m_builder->getInt32Ty();
            else
            {
                // Check the suitable vector type to cast to, inorder to minimize the number of instructions
                isNewTypeVector = true;
                if (newScalarBits % 32 == 0)
                    newType = VectorType::get(m_builder->getInt32Ty(), newScalarBits / 32);
                else if (newScalarBits % 16 == 0)
                    newType = VectorType::get(m_builder->getInt16Ty(), newScalarBits / 16);
                else if (newScalarBits % 8 == 0)
                    newType = VectorType::get(m_builder->getInt8Ty(), newScalarBits / 8);
                else
                    isNewTypeVector = false;
            }

            if (newType)
            {
                Value* src0 = I.getOperand(0);
                Value* src1 = I.getOperand(1);
                auto logicOp = I.getOpcode();
                m_builder->SetInsertPoint(&I);
                // bitcast the operands to new type
                Value* castedSrc0 = m_builder->CreateBitCast(src0, newType);
                Value* castedSrc1 = m_builder->CreateBitCast(src1, newType);
                Value* newBitCastInst;

                // Generate scalar logic operations, and then bitcast the result to a vector type
                if (!isNewTypeVector)
                {
                    Value* newLogicInst = m_builder->CreateBinOp(logicOp, castedSrc0, castedSrc1);
                    newBitCastInst = m_builder->CreateBitCast(newLogicInst, instType);
                }
                else
                {
                    VectorType* newVecType = cast<VectorType>(newType);
                    unsigned newVecTypeNumEle = int_cast<unsigned>(newVecType->getNumElements());
                    Value* ieLogicOp = UndefValue::get(newType);
                    for (unsigned i = 0; i < newVecTypeNumEle; i++)
                    {
                        Value* constIndex = ConstantInt::get(m_builder->getInt32Ty(), i);
                        Value* eeSrc0 = m_builder->CreateExtractElement(castedSrc0, constIndex);
                        Value* eeSrc1 = m_builder->CreateExtractElement(castedSrc1, constIndex);
                        Value* newLogicInst = m_builder->CreateBinOp(logicOp, eeSrc0, eeSrc1);
                        ieLogicOp = m_builder->CreateInsertElement(ieLogicOp, newLogicInst, constIndex);
                    }
                    newBitCastInst = m_builder->CreateBitCast(ieLogicOp, instType);
                }
                // Now replace all the instruction users with the newly bitcasted Logic Instruction
                I.replaceAllUsesWith(newBitCastInst);
                I.eraseFromParent();
            }
        }
    }
}