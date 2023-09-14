/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

Copyright (C) 2014 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal with the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimers.
Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimers in the documentation
and/or other materials provided with the distribution.
Neither the names of Advanced Micro Devices, Inc., nor the names of its
contributors may be used to endorse or promote products derived from this
Software without specific prior written permission.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH
THE SOFTWARE.

============================= end_copyright_notice ===========================*/

// This file implements SPIR-V instructions.

#include "SPIRVInstruction.h"
#include "SPIRVBasicBlock.h"
#include "SPIRVFunction.h"
#include "Probe/Assertion.h"


namespace igc_spv {

// Complete constructor for instruction with type and id
SPIRVInstruction::SPIRVInstruction(unsigned TheWordCount, Op TheOC,
    SPIRVType *TheType, SPIRVId TheId, SPIRVBasicBlock *TheBB)
  :SPIRVValue(TheBB->getModule(), TheWordCount, TheOC, TheType, TheId),
   BB(TheBB){
  validate();
}

SPIRVInstruction::SPIRVInstruction(unsigned TheWordCount, Op TheOC,
  SPIRVType *TheType, SPIRVId TheId, SPIRVBasicBlock *TheBB, SPIRVModule *TheBM)
  : SPIRVValue(TheBM, TheWordCount, TheOC, TheType, TheId), BB(TheBB){
  validate();
}

// Complete constructor for instruction with id but no type
SPIRVInstruction::SPIRVInstruction(unsigned TheWordCount, Op TheOC,
    SPIRVId TheId, SPIRVBasicBlock *TheBB)
  :SPIRVValue(TheBB->getModule(), TheWordCount, TheOC, TheId), BB(TheBB){
  validate();
}
// Complete constructor for instruction without type and id
SPIRVInstruction::SPIRVInstruction(unsigned TheWordCount, Op TheOC,
    SPIRVBasicBlock *TheBB)
  :SPIRVValue(TheBB->getModule(), TheWordCount, TheOC), BB(TheBB){
  validate();
}
// Complete constructor for instruction with type but no id
SPIRVInstruction::SPIRVInstruction(unsigned TheWordCount, Op TheOC,
    SPIRVType *TheType, SPIRVBasicBlock *TheBB)
  :SPIRVValue(TheBB->getModule(), TheWordCount, TheOC, TheType), BB(TheBB){
  validate();
}

void
SPIRVInstruction::setParent(SPIRVBasicBlock *TheBB) {
  IGC_ASSERT_MESSAGE(TheBB, "Invalid BB");
  if (BB == TheBB)
    return;
  IGC_ASSERT_MESSAGE(BB == NULL, "BB cannot change parent");
  BB = TheBB;
}

void
SPIRVInstruction::setScope(SPIRVEntry *Scope) {
  IGC_ASSERT_MESSAGE(Scope && Scope->getOpCode() == OpLabel, "Invalid scope");
  setParent(static_cast<SPIRVBasicBlock*>(Scope));
}

void
SPIRVFunctionCall::validate()const {
  SPIRVFunctionCallGeneric::validate();
}

SPIRVFunctionPointerCallINTEL::SPIRVFunctionPointerCallINTEL(
  SPIRVId TheId, SPIRVValue *TheCalledValue, SPIRVType *TheReturnType,
  const std::vector<SPIRVWord> &TheArgs, SPIRVBasicBlock *BB)
  : SPIRVFunctionCallGeneric(TheReturnType, TheId, TheArgs, BB),
  CalledValueId(TheCalledValue->getId()) {
  validate();
}

void SPIRVFunctionPointerCallINTEL::validate() const {
  SPIRVFunctionCallGeneric::validate();
}

//Each instruction should implement this function
std::vector<SPIRVValue*>
SPIRVInstruction::getOperands() {
  std::vector<SPIRVValue*> Empty;
  IGC_ASSERT_EXIT_MESSAGE(0, "not supported");
  return Empty;
}

std::vector<SPIRVType*>
SPIRVInstruction::getOperandTypes(const std::vector<SPIRVValue *> &Ops) {
  std::vector<SPIRVType*> Tys;
  for (auto& I : Ops) {
    SPIRVType* Ty = nullptr;
    if (I->getOpCode() == OpFunction)
      Ty = reinterpret_cast<SPIRVFunction*>(I)->getFunctionType();
    else
      Ty = I->getType();

    Tys.push_back(Ty);
  }
  return Tys;
}

std::vector<SPIRVType*>
SPIRVInstruction::getOperandTypes() {
  return getOperandTypes(getOperands());
}

bool
isSpecConstantOpAllowedOp(Op OC) {
  static SPIRVWord Table[] =
  {
    OpSConvert,
    OpFConvert,
    OpConvertFToS,
    OpConvertSToF,
    OpConvertFToU,
    OpConvertUToF,
    OpUConvert,
    OpConvertPtrToU,
    OpConvertUToPtr,
    OpGenericCastToPtr,
    OpPtrCastToGeneric,
    OpBitcast,
    OpQuantizeToF16,
    OpSNegate,
    OpNot,
    OpIAdd,
    OpISub,
    OpIMul,
    OpUDiv,
    OpSDiv,
    OpUMod,
    OpSRem,
    OpSMod,
    OpShiftRightLogical,
    OpShiftRightArithmetic,
    OpShiftLeftLogical,
    OpBitwiseOr,
    OpBitwiseXor,
    OpBitwiseAnd,
    OpFNegate,
    OpFAdd,
    OpFSub,
    OpFMul,
    OpFDiv,
    OpFRem,
    OpFMod,
    OpVectorShuffle,
    OpCompositeExtract,
    OpCompositeInsert,
    OpLogicalOr,
    OpLogicalAnd,
    OpLogicalNot,
    OpLogicalEqual,
    OpLogicalNotEqual,
    OpSelect,
    OpIEqual,
    OpINotEqual,
    OpULessThan,
    OpSLessThan,
    OpUGreaterThan,
    OpSGreaterThan,
    OpULessThanEqual,
    OpSLessThanEqual,
    OpUGreaterThanEqual,
    OpSGreaterThanEqual,
    OpAccessChain,
    OpInBoundsAccessChain,
    OpPtrAccessChain,
    OpInBoundsPtrAccessChain,
  };
  static std::unordered_set<SPIRVWord>
    Allow(std::begin(Table), std::end(Table));
  return (Allow.count(OC) > 0);
}

SPIRVSpecConstantOp *
createSpecConstantOpInst(SPIRVInstruction *Inst) {
  auto OC = Inst->getOpCode();
  IGC_ASSERT_MESSAGE(isSpecConstantOpAllowedOp(OC), "Op code not allowed for OpSpecConstantOp");
  auto Ops = Inst->getIds(Inst->getOperands());
  Ops.insert(Ops.begin(), OC);
  return static_cast<SPIRVSpecConstantOp *>(
    SPIRVSpecConstantOp::create(OpSpecConstantOp, Inst->getType(),
        Inst->getId(), Ops, nullptr, Inst->getModule()));
}

SPIRVInstruction *
createInstFromSpecConstantOp(SPIRVSpecConstantOp *Inst) {
  IGC_ASSERT_MESSAGE(Inst->getOpCode() == OpSpecConstantOp, "Not OpSpecConstantOp");
  auto Ops = Inst->getOpWords();
  auto OC = static_cast<Op>(Ops[0]);
  IGC_ASSERT_MESSAGE(isSpecConstantOpAllowedOp(OC), "Op code not allowed for OpSpecConstantOp");
  Ops.erase(Ops.begin(), Ops.begin() + 1);
  return SPIRVInstTemplateBase::create(OC, Inst->getType(),
      Inst->getId(), Ops, nullptr, Inst->getModule());
}

}


