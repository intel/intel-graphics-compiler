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
copy of this software and associated documentation filesf (the "Software"),
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

// This file defines Instruction class for SPIR-V.

#ifndef SPIRVINSTRUCTION_HPP_
#define SPIRVINSTRUCTION_HPP_

#include "SPIRVEnum.h"
#include "SPIRVStream.h"
#include "SPIRVValue.h"
#include "SPIRVBasicBlock.h"
#include "SPIRVFunction.h"
#include "Probe/Assertion.h"

#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <utility>
#include <vector>
#include <unordered_set>

namespace igc_spv{

typedef std::vector<SPIRVValue *> ValueVec;
typedef std::pair<ValueVec::iterator, ValueVec::iterator> ValueRange;

class SPIRVBasicBlock;
class SPIRVFunction;

bool isSpecConstantOpAllowedOp(Op OC);

class SPIRVComponentExecutionScope {
public:
  SPIRVComponentExecutionScope(Scope TheScope = ScopeInvocation):
    ExecScope(TheScope){}
  Scope ExecScope;
};

class SPIRVComponentMemorySemanticsMask {
public:
  SPIRVComponentMemorySemanticsMask(SPIRVWord TheSema = SPIRVWORD_MAX):
    MemSema(TheSema){}
  SPIRVWord MemSema;
};

class SPIRVComponentOperands {
public:
  SPIRVComponentOperands(){};
  SPIRVComponentOperands(const std::vector<SPIRVValue *> &TheOperands):
    Operands(TheOperands){};
  SPIRVComponentOperands(std::vector<SPIRVValue *> &&TheOperands):
    Operands(std::move(TheOperands)){};
  std::vector<SPIRVValue *> getCompOperands() {
    return Operands;
  }
  std::vector<SPIRVType *> getCompOperandTypes() {
    std::vector<SPIRVType *> Tys;
    for (auto &I:getCompOperands())
      Tys.push_back(I->getType());
    return Tys;
  }
protected:
  std::vector<SPIRVValue *> Operands;
};

class SPIRVInstruction: public SPIRVValue {
protected:
  // Complete constructor for instruction with type and id
  SPIRVInstruction(unsigned TheWordCount, Op TheOC, SPIRVType *TheType,
      SPIRVId TheId, SPIRVBasicBlock *TheBB);
  // Complete constructor for instruction with module, type and id
  SPIRVInstruction(unsigned TheWordCount, Op TheOC,
      SPIRVType *TheType, SPIRVId TheId, SPIRVBasicBlock *TheBB,
      SPIRVModule *TheBM);
  // Complete constructor for instruction with id but no type
  SPIRVInstruction(unsigned TheWordCount, Op TheOC, SPIRVId TheId,
      SPIRVBasicBlock *TheBB);
  // Complete constructor for instruction without type and id
  SPIRVInstruction(unsigned TheWordCount, Op TheOC,
      SPIRVBasicBlock *TheBB);
  // Complete constructor for instruction with type but no id
  SPIRVInstruction(unsigned TheWordCount, Op TheOC, SPIRVType *TheType,
      SPIRVBasicBlock *TheBB);
  // Incomplete constructor
  SPIRVInstruction(Op TheOC) : SPIRVValue(TheOC), BB(NULL) {}

public:
  virtual bool isInst() const { return true; }
  SPIRVBasicBlock *getParent() const {return BB;}
  SPIRVInstruction *getPrevious() const { return BB->getPrevious(this);}
  SPIRVInstruction *getNext() const { return BB->getNext(this);}
  virtual std::vector<SPIRVValue *> getOperands();
  std::vector<SPIRVType*> getOperandTypes();
  static std::vector<SPIRVType*> getOperandTypes(
      const std::vector<SPIRVValue *> &Ops);

  void setParent(SPIRVBasicBlock *);
  void setScope(SPIRVEntry *);
  void addFPRoundingMode(SPIRVFPRoundingModeKind Kind) {
    addDecorate(DecorationFPRoundingMode, Kind);
  }
  void eraseFPRoundingMode() {
    eraseDecorate(DecorationFPRoundingMode);
  }
  void setSaturatedConversion(bool Enable) {
    if (Enable)
      addDecorate(DecorationSaturatedConversion);
    else
      eraseDecorate(DecorationSaturatedConversion);
  }
  bool hasFPRoundingMode(SPIRVFPRoundingModeKind *Kind = nullptr) {
    SPIRVWord V;
    auto Found = hasDecorate(DecorationFPRoundingMode, 0, &V);
    if (Found && Kind)
      *Kind = static_cast<SPIRVFPRoundingModeKind>(V);
    return Found;
  }
  bool isSaturatedConversion() {
    return hasDecorate(DecorationSaturatedConversion) ||
        OpCode == OpSatConvertSToU ||
        OpCode == OpSatConvertUToS;
  }

  SPIRVBasicBlock* getBasicBlock() const {
    return BB;
  }

  void setBasicBlock(SPIRVBasicBlock* TheBB) {
    BB = TheBB;
    if (TheBB)
      setModule(TheBB->getModule());
  }

  virtual bool isOperandLiteral(unsigned Index) const {
    IGC_ASSERT_MESSAGE(0, "not implemented");
    return false;
  }
protected:
  void validate()const {
    SPIRVValue::validate();
  }
private:
  SPIRVBasicBlock *BB;
};

class SPIRVInstTemplateBase:public SPIRVInstruction {
public:
  /// Create an empty instruction. Mainly for getting format information,
  /// e.g. whether an operand is literal.
  static SPIRVInstTemplateBase *create(Op TheOC){
    auto Inst = static_cast<SPIRVInstTemplateBase *>(SPIRVEntry::create(TheOC));
    Inst->init();
    return Inst;
  }
  /// Create a instruction without operands.
  static SPIRVInstTemplateBase *create(Op TheOC, SPIRVType *TheType,
      SPIRVId TheId, SPIRVBasicBlock *TheBB,
      SPIRVModule *TheModule){
    auto Inst = create(TheOC);
    Inst->init(TheType, TheId, TheBB, TheModule);
    return Inst;
  }
  /// Create a complete and valid instruction.
  static SPIRVInstTemplateBase *create(Op TheOC, SPIRVType *TheType,
      SPIRVId TheId, const std::vector<SPIRVWord> &TheOps, SPIRVBasicBlock *TheBB,
      SPIRVModule *TheModule){
    auto Inst = create(TheOC);
    Inst->init(TheType, TheId, TheBB, TheModule);
    Inst->setOpWords(TheOps);
    Inst->validate();
    return Inst;
  }
  SPIRVInstTemplateBase(Op OC = OpNop)
    :SPIRVInstruction(OC), HasVariWC(false){
    init();
  }
  virtual ~SPIRVInstTemplateBase(){}
  SPIRVInstTemplateBase *init(SPIRVType *TheType,
      SPIRVId TheId, SPIRVBasicBlock *TheBB,
      SPIRVModule *TheModule){
    IGC_ASSERT_MESSAGE((TheBB || TheModule), "Invalid BB or Module");
    if (TheBB)
      setBasicBlock(TheBB);
    else {
      setModule(TheModule);
    }
    setId(hasId() ? TheId : SPIRVID_INVALID);
    setType(hasType() ? TheType : nullptr);
    return this;
  }
  virtual void init() {}
  virtual void initImpl(Op OC, bool HasId = true, SPIRVWord WC = 0,
      bool VariWC = false, unsigned Lit1 = ~0U,
      unsigned Lit2 = ~0U, unsigned Lit3 = ~0U){
    OpCode = OC;
    if (!HasId) {
      setHasNoId();
      setHasNoType();
    }
    if (WC)
      SPIRVEntry::setWordCount(WC);
    setHasVariableWordCount(VariWC);
    addLit(Lit1);
    addLit(Lit2);
    addLit(Lit3);
  }
  bool isOperandLiteral(unsigned I) const override {
    return (Lit.count(I) > 0);
  }
  void addLit(unsigned L) {
    if (L != ~0U)
      Lit.insert(L);
  }
  /// \return Expected number of operands. If the instruction has variable
  /// number of words, return the minimum.
  SPIRVWord getExpectedNumOperands() const {
    IGC_ASSERT_MESSAGE(WordCount > 0, "Word count not initialized");
    auto Exp = WordCount - 1;
    if (hasId())
      --Exp;
    if (hasType())
      --Exp;
    return Exp;
  }
  virtual void setOpWordsAndValidate(const std::vector<SPIRVWord> &TheOps) {
    setOpWords(TheOps);
    validate();
  }
  virtual void setOpWords(const std::vector<SPIRVWord> &TheOps) {
    SPIRVWord WC = TheOps.size() + 1;
    if (hasId())
      ++WC;
    if (hasType())
      ++WC;
    if (WordCount) {
      if (WordCount == WC) {
        // do nothing
      } else {
        IGC_ASSERT_MESSAGE(HasVariWC, "Invalid word count");
        IGC_ASSERT_MESSAGE(WC >= WordCount, "Invalid word count");
        SPIRVEntry::setWordCount(WC);
      }
    } else
      SPIRVEntry::setWordCount(WC);
    Ops = TheOps;
  }
  virtual void setWordCount(SPIRVWord TheWordCount) override {
    SPIRVEntry::setWordCount(TheWordCount);
    auto NumOps = WordCount - 1;
    if (hasId())
      --NumOps;
    if (hasType())
      --NumOps;
    Ops.resize(NumOps);
  }

  std::vector<SPIRVWord> &getOpWords() {
    return Ops;
  }

  const std::vector<SPIRVWord> &getOpWords() const {
    return Ops;
  }

  SPIRVWord getOpWord(int I) const {
    return Ops[I];
  }

  // Get operands which are values.
  // Drop execution scope and group operation literals.
  // Return other literals as uint32 constants.
  virtual std::vector<SPIRVValue *> getOperands() override {
    std::vector<SPIRVValue*> VOps;
    for (size_t I = 0, E = Ops.size(); I != E; ++I)
      VOps.push_back(getOperand(I));
    return VOps;
  }

  virtual SPIRVValue *getOperand(unsigned I) {
    return isOperandLiteral(I) ? Module->getLiteralAsConstant(Ops[I]) :
        getValue(Ops[I]);
  }

  bool hasExecScope() const {
    return igc_spv::hasExecScope(OpCode);
  }

  bool hasGroupOperation() const {
    return igc_spv::hasGroupOperation(OpCode);
  }

  SPIRVGroupOperationKind getGroupOperation() const {
    if (!hasGroupOperation())
      return GroupOperationCount;
    return static_cast<SPIRVGroupOperationKind>(Ops[1]);
  }

  Scope getExecutionScope() const {
    if(!hasExecScope())
      return ScopeInvocation;
    return static_cast<Scope>(
        static_cast<SPIRVConstant*>(getValue(Ops[0]))->getZExtIntValue());
  }

  bool hasVariableWordCount() const {
    return HasVariWC;
  }

  void setHasVariableWordCount(bool VariWC) {
    HasVariWC = VariWC;
  }

protected:
  virtual void decode(std::istream &I) override {
    auto D = getDecoder(I);
    if (hasType())
      D >> Type;
    if (hasId())
      D >> Id;
    D >> Ops;
  }
  std::vector<SPIRVWord> Ops;
  bool HasVariWC;
  std::unordered_set<unsigned> Lit; // Literal operand index
};

template<typename BT        = SPIRVInstTemplateBase,
         Op OC              = OpNop,
         bool HasId         = true,
         SPIRVWord WC        = 0,
         bool HasVariableWC = false,
         unsigned Literal1  = ~0U,
         unsigned Literal2  = ~0U,
         unsigned Literal3  = ~0U>
class SPIRVInstTemplate:public BT {
public:
  typedef BT BaseTy;
  SPIRVInstTemplate(){
    init();
  }
  virtual ~SPIRVInstTemplate(){}
  virtual void init() {
    this->initImpl(OC, HasId, WC, HasVariableWC, Literal1, Literal2, Literal3);
  }
};

class SPIRVMemoryAccess {
public:
  SPIRVMemoryAccess(const std::vector<SPIRVWord>& TheMemoryAccess)
    : TheMemoryAccessMask(0), Alignment(0), AliasScopeInstID(0),
      NoAliasInstID(0) {
    MemoryAccessUpdate(TheMemoryAccess);
  }

  SPIRVMemoryAccess()
    : TheMemoryAccessMask(0), Alignment(0), AliasScopeInstID(0),
      NoAliasInstID(0) {}

  void MemoryAccessUpdate(const std::vector<SPIRVWord> &MemoryAccess) {
    if (!MemoryAccess.size())
      return;
    IGC_ASSERT_MESSAGE(MemoryAccess.size() > 0, "Invalid memory access operand size");
    IGC_ASSERT_MESSAGE(MemoryAccess.size() < 5, "Invalid memory access operand size");
    TheMemoryAccessMask = MemoryAccess[0];
    size_t MemAccessNumParam = 1;
    if (MemoryAccess[0] & MemoryAccessAlignedMask) {
      IGC_ASSERT_MESSAGE(MemoryAccess.size() > 1, "Alignment operand is missing");
      Alignment = MemoryAccess[MemAccessNumParam++];
    }
    if (MemoryAccess[0] & MemoryAccessAliasScopeINTELMask) {
      IGC_ASSERT_MESSAGE(MemoryAccess.size() > MemAccessNumParam,
        "Aliasing operand is missing");
      AliasScopeInstID = MemoryAccess[MemAccessNumParam++];
    }
    if (MemoryAccess[0] & MemoryAccessNoAliasINTELMask) {
      IGC_ASSERT_MESSAGE(MemoryAccess.size() > MemAccessNumParam,
        "Aliasing operand is missing");
      NoAliasInstID = MemoryAccess[MemAccessNumParam];
    }
  }

  SPIRVWord isVolatile() const { return getMemoryAccessMask() & MemoryAccessVolatileMask; }
  SPIRVWord isNonTemporal() const { return getMemoryAccessMask() & MemoryAccessNontemporalMask; }
  SPIRVWord isAliasScope() const {
    return getMemoryAccessMask() & MemoryAccessAliasScopeINTELMask;
  }
  SPIRVWord isNoAlias() const {
    return getMemoryAccessMask() & MemoryAccessNoAliasINTELMask;
  }
  SPIRVWord getMemoryAccessMask() const { return TheMemoryAccessMask; }
  SPIRVWord getAlignment() const { return Alignment; }
  SPIRVWord getAliasScopeInstID() const { return AliasScopeInstID; }
  SPIRVWord getNoAliasInstID() const { return NoAliasInstID; }

protected:
  SPIRVWord TheMemoryAccessMask;
  SPIRVWord Alignment;
  SPIRVId AliasScopeInstID;
  SPIRVId NoAliasInstID;
};

class SPIRVVariable : public SPIRVInstruction {
public:
  // Incomplete constructor
  SPIRVVariable() :SPIRVInstruction(OpVariable),
     StorageClass(SPIRVStorageClassKind::StorageClassCount){}

  SPIRVStorageClassKind getStorageClass() const { return StorageClass; }
  SPIRVValue *getInitializer() const {
    if (Initializer.empty())
      return nullptr;
    IGC_ASSERT_EXIT(Initializer.size() == 1);
    return getValue(Initializer[0]);
  }
  bool isConstant() const {
    return hasDecorate(DecorationConstant);
  }
  bool isBuiltin(SPIRVBuiltinVariableKind *BuiltinKind = nullptr) const {
    SPIRVWord Kind;
    bool Found = hasDecorate(DecorationBuiltIn, 0, &Kind);
    if (!Found)
      return false;
    if (BuiltinKind)
      *BuiltinKind = static_cast<SPIRVBuiltinVariableKind>(Kind);
    return true;
  }
  void setBuiltin(SPIRVBuiltinVariableKind Kind) {
    IGC_ASSERT(isValid(Kind));
    addDecorate(new SPIRVDecorate(DecorationBuiltIn, this, Kind));
  }
  void setIsConstant(bool Is) {
    if (Is)
      addDecorate(new SPIRVDecorate(DecorationConstant, this));
    else
      eraseDecorate(DecorationConstant);
  }
protected:
  void validate() const {
    SPIRVValue::validate();
    IGC_ASSERT(isValid(StorageClass));
    IGC_ASSERT(Initializer.size() == 1 || Initializer.empty());
  }
  void setWordCount(SPIRVWord TheWordCount) {
    SPIRVEntry::setWordCount(TheWordCount);
    Initializer.resize(WordCount - 4);
  }
  _SPIRV_DEF_DEC4(Type, Id, StorageClass, Initializer)

    SPIRVStorageClassKind StorageClass;
  std::vector<SPIRVId> Initializer;
};

class SPIRVStore:public SPIRVInstruction, public SPIRVMemoryAccess {
public:
  const static SPIRVWord FixedWords = 3;
  // Incomplete constructor
  SPIRVStore():SPIRVInstruction(OpStore), SPIRVMemoryAccess(),
      PtrId(SPIRVID_INVALID), ValId(SPIRVID_INVALID){
    setAttr();
  }

  SPIRVValue *getSrc() const { return getValue(ValId);}
  SPIRVValue *getDst() const { return getValue(PtrId);}
protected:
  void setAttr() {
    setHasNoType();
    setHasNoId();
  }

  void setWordCount(SPIRVWord TheWordCount) {
    SPIRVEntry::setWordCount(TheWordCount);
    MemoryAccess.resize(TheWordCount - FixedWords);
  }

  void decode(std::istream &I) {
    getDecoder(I) >> PtrId >> ValId >> MemoryAccess;
    MemoryAccessUpdate(MemoryAccess);
  }

  void validate()const {
    SPIRVInstruction::validate();
    if (getSrc()->isForward() || getDst()->isForward())
      return;
    IGC_ASSERT_MESSAGE(getValueType(PtrId)->getPointerElementType()->isTypeVoid() == getValueType(ValId)->isTypeVoid(), "Inconsistent operand type");
    IGC_ASSERT_MESSAGE(getValueType(PtrId)->getPointerElementType()->isTypeArray() == getValueType(ValId)->isTypeArray(), "Inconsistent operand type");
    IGC_ASSERT_MESSAGE(getValueType(PtrId)->getPointerElementType()->isTypeBool() == getValueType(ValId)->isTypeBool(), "Inconsistent operand type");
    IGC_ASSERT_MESSAGE(getValueType(PtrId)->getPointerElementType()->isTypeComposite() == getValueType(ValId)->isTypeComposite(), "Inconsistent operand type");
    IGC_ASSERT_MESSAGE(getValueType(PtrId)->getPointerElementType()->isTypeEvent() == getValueType(ValId)->isTypeEvent(), "Inconsistent operand type");
    IGC_ASSERT_MESSAGE(getValueType(PtrId)->getPointerElementType()->isTypeFloat(0) == getValueType(ValId)->isTypeFloat(0), "Inconsistent operand type");
    IGC_ASSERT_MESSAGE(getValueType(PtrId)->getPointerElementType()->isTypeImage() == getValueType(ValId)->isTypeImage(), "Inconsistent operand type");
    IGC_ASSERT_MESSAGE(getValueType(PtrId)->getPointerElementType()->isTypeOCLImage() == getValueType(ValId)->isTypeOCLImage(), "Inconsistent operand type");
    IGC_ASSERT_MESSAGE(getValueType(PtrId)->getPointerElementType()->isTypePipe() == getValueType(ValId)->isTypePipe(), "Inconsistent operand type");
    IGC_ASSERT_MESSAGE(getValueType(PtrId)->getPointerElementType()->isTypeInt(0) == getValueType(ValId)->isTypeInt(0), "Inconsistent operand type");
    IGC_ASSERT_MESSAGE(getValueType(PtrId)->getPointerElementType()->isTypeSampler() == getValueType(ValId)->isTypeSampler(), "Inconsistent operand type");
    IGC_ASSERT_MESSAGE(getValueType(PtrId)->getPointerElementType()->isTypeStruct() == getValueType(ValId)->isTypeStruct(), "Inconsistent operand type");
    IGC_ASSERT_MESSAGE(getValueType(PtrId)->getPointerElementType()->isTypeVector() == getValueType(ValId)->isTypeVector(), "Inconsistent operand type");
    IGC_ASSERT_MESSAGE(getValueType(PtrId)->getPointerElementType()->isTypeVectorInt() == getValueType(ValId)->isTypeVectorInt(), "Inconsistent operand type");
    IGC_ASSERT_MESSAGE(getValueType(PtrId)->getPointerElementType()->isTypeVectorFloat() == getValueType(ValId)->isTypeVectorFloat(), "Inconsistent operand type");
    IGC_ASSERT_MESSAGE(getValueType(PtrId)->getPointerElementType()->isTypeVectorBool() == getValueType(ValId)->isTypeVectorBool(), "Inconsistent operand type");
    IGC_ASSERT_MESSAGE(getValueType(PtrId)->getPointerElementType()->isTypeNamedBarrier() == getValueType(ValId)->isTypeNamedBarrier(), "Inconsistent operand type");
  }
private:
  std::vector<SPIRVWord> MemoryAccess;
  SPIRVId PtrId;
  SPIRVId ValId;
};

class SPIRVLoad:public SPIRVInstruction, public SPIRVMemoryAccess {
public:
  const static SPIRVWord FixedWords = 4;
  // Incomplete constructor
  SPIRVLoad():SPIRVInstruction(OpLoad), SPIRVMemoryAccess(),
      PtrId(SPIRVID_INVALID){}

  SPIRVValue *getSrc() const { return Module->get<SPIRVValue>(PtrId);}

protected:
  void setWordCount(SPIRVWord TheWordCount) {
    SPIRVEntry::setWordCount(TheWordCount);
    MemoryAccess.resize(TheWordCount - FixedWords);
  }

  void decode(std::istream &I) {
    getDecoder(I) >> Type >> Id >> PtrId >> MemoryAccess;
    MemoryAccessUpdate(MemoryAccess);
  }

  void validate()const {
    SPIRVInstruction::validate();
    IGC_ASSERT_MESSAGE((getValue(PtrId)->isForward() || Type == getValueType(PtrId)->getPointerElementType()), "Inconsistent types");
  }
private:
  SPIRVId PtrId;
  std::vector<SPIRVWord> MemoryAccess;
};

class SPIRVBinary:public SPIRVInstTemplateBase {
protected:
  void validate()const {
    SPIRVId Op1 = Ops[0];
    SPIRVId Op2 = Ops[1];
    SPIRVType *op1Ty, *op2Ty;
    SPIRVInstruction::validate();
    if (getValue(Op1)->isForward() || getValue(Op2)->isForward())
      return;
    if (getValueType(Op1)->isTypeVector()) {
      op1Ty = getValueType(Op1)->getVectorComponentType();
      op2Ty = getValueType(Op2)->getVectorComponentType();
      IGC_ASSERT_MESSAGE(getValueType(Op1)->getVectorComponentCount() == getValueType(Op2)->getVectorComponentCount(), "Inconsistent Vector component width");
    }
    else {
      op1Ty = getValueType(Op1);
      op2Ty = getValueType(Op2);
    }

    if (isBinaryOpCode(OpCode)) {
      IGC_ASSERT_MESSAGE(getValueType(Op1)== getValueType(Op2), "Invalid type for binary instruction");
      IGC_ASSERT_MESSAGE((op1Ty->isTypeInt() || op2Ty->isTypeFloat()), "Invalid type for Binary instruction");
      IGC_ASSERT_MESSAGE((op1Ty->getBitWidth() == op2Ty->getBitWidth()), "Inconsistent BitWidth");
    } else if (isShiftOpCode(OpCode)) {
      IGC_ASSERT_MESSAGE((op1Ty->isTypeInt() || op2Ty->isTypeInt()), "Invalid type for shift instruction");
    } else if (isLogicalOpCode(OpCode)) {
      IGC_ASSERT_MESSAGE((op1Ty->isTypeBool() || op2Ty->isTypeBool()), "Invalid type for logical instruction");
    } else if (isBitwiseOpCode(OpCode)) {
      IGC_ASSERT_MESSAGE((op1Ty->isTypeInt() || op2Ty->isTypeInt()), "Invalid type for bitwise instruction");
      IGC_ASSERT_MESSAGE((op1Ty->getIntegerBitWidth() == op2Ty->getIntegerBitWidth()), "Inconsistent BitWidth");
    } else {
      IGC_ASSERT_EXIT_MESSAGE(0, "Invalid op code!");
    }
  }
};

template<Op OC>
class SPIRVBinaryInst:public SPIRVInstTemplate<SPIRVBinary, OC, true, 5, false> {
};

#define _SPIRV_OP(x) typedef SPIRVBinaryInst<Op##x> SPIRV##x;
_SPIRV_OP(IAdd)
_SPIRV_OP(FAdd)
_SPIRV_OP(ISub)
_SPIRV_OP(FSub)
_SPIRV_OP(IMul)
_SPIRV_OP(FMul)
_SPIRV_OP(UDiv)
_SPIRV_OP(SDiv)
_SPIRV_OP(FDiv)
_SPIRV_OP(SRem)
_SPIRV_OP(SMod)
_SPIRV_OP(FRem)
_SPIRV_OP(FMod)
_SPIRV_OP(UMod)
_SPIRV_OP(ShiftLeftLogical)
_SPIRV_OP(ShiftRightLogical)
_SPIRV_OP(ShiftRightArithmetic)
_SPIRV_OP(LogicalAnd)
_SPIRV_OP(LogicalOr)
_SPIRV_OP(LogicalEqual)
_SPIRV_OP(LogicalNotEqual)
_SPIRV_OP(BitwiseAnd)
_SPIRV_OP(BitwiseOr)
_SPIRV_OP(BitwiseXor)
_SPIRV_OP(Dot)
#undef _SPIRV_OP

template<Op TheOpCode>
class SPIRVInstNoOperand:public SPIRVInstruction {
public:
  // Complete constructor
  SPIRVInstNoOperand(SPIRVBasicBlock *TheBB):SPIRVInstruction(1, TheOpCode,
      TheBB){
    setAttr();
    validate();
  }
  // Incomplete constructor
  SPIRVInstNoOperand():SPIRVInstruction(TheOpCode){
    setAttr();
  }
protected:
  void setAttr() {
    setHasNoId();
    setHasNoType();
  }
  _SPIRV_DEF_DEC0
};

typedef SPIRVInstNoOperand<OpReturn> SPIRVReturn;

class SPIRVReturnValue:public SPIRVInstruction {
public:
  static const Op OC = OpReturnValue;
  // Incomplete constructor
  SPIRVReturnValue():SPIRVInstruction(OC), ReturnValueId(SPIRVID_INVALID) {
    setAttr();
  }

  SPIRVValue *getReturnValue() const {
    return getValue(ReturnValueId);
  }
protected:
  void setAttr() {
    setHasNoId();
    setHasNoType();
  }
  _SPIRV_DEF_DEC1(ReturnValueId)
  void validate()const {
    SPIRVInstruction::validate();
  }
  SPIRVId ReturnValueId;
};

class SPIRVBranch:public SPIRVInstruction {
public:
  static const Op OC = OpBranch;
  // Incomplete constructor
  SPIRVBranch():SPIRVInstruction(OC), TargetLabelId(SPIRVID_INVALID) {
    setHasNoId();
    setHasNoType();
  }
  SPIRVValue *getTargetLabel() const {
    return getValue(TargetLabelId);
  }
protected:
  _SPIRV_DEF_DEC1(TargetLabelId)
  void validate()const {
    SPIRVInstruction::validate();
    IGC_ASSERT(WordCount == 2);
    IGC_ASSERT(OpCode == OC);
    IGC_ASSERT(getTargetLabel()->isLabel() || getTargetLabel()->isForward());
  }
  SPIRVId TargetLabelId;
};

class SPIRVBranchConditional:public SPIRVInstruction {
public:
  static const Op OC = OpBranchConditional;
  // Incomplete constructor
  SPIRVBranchConditional():SPIRVInstruction(OC), ConditionId(SPIRVID_INVALID),
      TrueLabelId(SPIRVID_INVALID), FalseLabelId(SPIRVID_INVALID) {
    setHasNoId();
    setHasNoType();
  }
  SPIRVValue *getCondition() const {
    return getValue(ConditionId);
  }
  SPIRVLabel *getTrueLabel() const {
    return SPIRVEntry::get<SPIRVLabel>(TrueLabelId);
  }
  SPIRVLabel *getFalseLabel() const {
    return SPIRVEntry::get<SPIRVLabel>(FalseLabelId);
  }
protected:
  void setWordCount(SPIRVWord TheWordCount) {
    SPIRVEntry::setWordCount(TheWordCount);
    BranchWeights.resize(TheWordCount - 4);
  }
  _SPIRV_DEF_DEC4(ConditionId, TrueLabelId, FalseLabelId, BranchWeights)
  void validate()const {
    SPIRVInstruction::validate();
    IGC_ASSERT(WordCount == 4 || WordCount == 6);
    IGC_ASSERT(WordCount == BranchWeights.size() + 4);
    IGC_ASSERT(OpCode == OC);
    IGC_ASSERT(getCondition()->isForward() || getCondition()->getType()->isTypeBool());
    IGC_ASSERT(getTrueLabel()->isForward() || getTrueLabel()->isLabel());
    IGC_ASSERT(getFalseLabel()->isForward() || getFalseLabel()->isLabel());
  }
  SPIRVId ConditionId;
  SPIRVId TrueLabelId;
  SPIRVId FalseLabelId;
  std::vector<SPIRVWord> BranchWeights;
};

class SPIRVPhi: public SPIRVInstruction {
public:
  static const Op OC = OpPhi;
  static const SPIRVWord FixedWordCount = 3;
  SPIRVPhi():SPIRVInstruction(OC) {}
  std::vector<SPIRVValue *> getPairs() {
    return getValues(Pairs);
  }
  void addPair(SPIRVValue *Value, SPIRVBasicBlock *BB) {
    Pairs.push_back(Value->getId());
    Pairs.push_back(BB->getId());
    WordCount = Pairs.size() + FixedWordCount;
    validate();
  }
  void setPairs(const std::vector<SPIRVValue *> &ThePairs) {
    Pairs = getIds(ThePairs);
    WordCount = Pairs.size() + FixedWordCount;
    validate();
  }
  void foreachPair(std::function<void(SPIRVValue *, SPIRVBasicBlock *,
      size_t)> Func) {
    for (size_t I = 0, E = Pairs.size()/2; I != E; ++I) {
      SPIRVEntry *Value, *BB;
      if (!Module->exist(Pairs[2*I], &Value) ||
          !Module->exist(Pairs[2*I+1], &BB))
        continue;
      Func(static_cast<SPIRVValue *>(Value), static_cast<SPIRVBasicBlock *>(BB),
          I);
    }
  }
  void foreachPair(std::function<void(SPIRVValue *, SPIRVBasicBlock *)> Func)
    const {
    for (size_t I = 0, E = Pairs.size()/2; I != E; ++I) {
      SPIRVEntry *Value, *BB;
      if (!Module->exist(Pairs[2*I], &Value) ||
          !Module->exist(Pairs[2*I+1], &BB))
        continue;
      Func(static_cast<SPIRVValue *>(Value), static_cast<SPIRVBasicBlock *>(BB));
    }
  }
  void setWordCount(SPIRVWord TheWordCount) {
    SPIRVEntry::setWordCount(TheWordCount);
    Pairs.resize(TheWordCount - FixedWordCount);
  }
  _SPIRV_DEF_DEC3(Type, Id, Pairs)
  void validate()const {
    IGC_ASSERT(WordCount == Pairs.size() + FixedWordCount);
    IGC_ASSERT(OpCode == OC);
    IGC_ASSERT(Pairs.size() % 2 == 0);
    foreachPair([=](SPIRVValue *IncomingV, SPIRVBasicBlock *IncomingBB){
      IGC_ASSERT(IncomingV->isForward() || IncomingV->getType() == Type);
      IGC_ASSERT(IncomingBB->isBasicBlock() || IncomingBB->isForward());
    });
    SPIRVInstruction::validate();
  }
protected:
  std::vector<SPIRVId> Pairs;
};

class SPIRVCompare:public SPIRVInstTemplateBase {
protected:
  void validate()const {
    auto Op1 = Ops[0];
    auto Op2 = Ops[1];
    SPIRVType *op1Ty, *op2Ty, *resTy;
    SPIRVInstruction::validate();
    if (getValue(Op1)->isForward() || getValue(Op2)->isForward())
      return;

    if (getValueType(Op1)->isTypeVector()) {
      op1Ty = getValueType(Op1)->getVectorComponentType();
      op2Ty = getValueType(Op2)->getVectorComponentType();
      resTy = Type->getVectorComponentType();
      IGC_ASSERT_MESSAGE(getValueType(Op1)->getVectorComponentCount() == getValueType(Op2)->getVectorComponentCount(), "Inconsistent Vector component width");
    }
    else {
      op1Ty = getValueType(Op1);
      op2Ty = getValueType(Op2);
      resTy = Type;
    }
    IGC_ASSERT_MESSAGE(isCmpOpCode(OpCode), "Invalid op code for cmp inst");
    IGC_ASSERT_MESSAGE((resTy->isTypeBool() || resTy->isTypeInt()), "Invalid type for compare instruction");
    IGC_ASSERT_MESSAGE(op1Ty == op2Ty, "Inconsistent types");
  }
};

template<Op OC>
class SPIRVCmpInst:public SPIRVInstTemplate<SPIRVCompare, OC, true, 5, false> {
};

#define _SPIRV_OP(x) typedef SPIRVCmpInst<Op##x> SPIRV##x;
_SPIRV_OP(IEqual)
_SPIRV_OP(FOrdEqual)
_SPIRV_OP(FUnordEqual)
_SPIRV_OP(INotEqual)
_SPIRV_OP(FOrdNotEqual)
_SPIRV_OP(FUnordNotEqual)
_SPIRV_OP(ULessThan)
_SPIRV_OP(SLessThan)
_SPIRV_OP(FOrdLessThan)
_SPIRV_OP(FUnordLessThan)
_SPIRV_OP(UGreaterThan)
_SPIRV_OP(SGreaterThan)
_SPIRV_OP(FOrdGreaterThan)
_SPIRV_OP(FUnordGreaterThan)
_SPIRV_OP(ULessThanEqual)
_SPIRV_OP(SLessThanEqual)
_SPIRV_OP(FOrdLessThanEqual)
_SPIRV_OP(FUnordLessThanEqual)
_SPIRV_OP(UGreaterThanEqual)
_SPIRV_OP(SGreaterThanEqual)
_SPIRV_OP(FOrdGreaterThanEqual)
_SPIRV_OP(FUnordGreaterThanEqual)
_SPIRV_OP(LessOrGreater)
_SPIRV_OP(Ordered)
_SPIRV_OP(Unordered)
#undef _SPIRV_OP

class SPIRVSelect:public SPIRVInstruction {
public:
  // Incomplete constructor
  SPIRVSelect():SPIRVInstruction(OpSelect), Condition(SPIRVID_INVALID),
      Op1(SPIRVID_INVALID), Op2(SPIRVID_INVALID){}
  SPIRVValue *getCondition() { return getValue(Condition);}
  SPIRVValue *getTrueValue() { return getValue(Op1);}
  SPIRVValue *getFalseValue() { return getValue(Op2);}
protected:
  _SPIRV_DEF_DEC5(Type, Id, Condition, Op1, Op2)
  void validate()const {
    SPIRVInstruction::validate();
    if (getValue(Condition)->isForward() ||
        getValue(Op1)->isForward() ||
        getValue(Op2)->isForward())
      return;

    SPIRVType *conTy = getValueType(Condition)->isTypeVector() ?
        getValueType(Condition)->getVectorComponentType() :
        getValueType(Condition);
    IGC_ASSERT_MESSAGE(conTy->isTypeBool(), "Invalid type");
    IGC_ASSERT_MESSAGE(getType() == getValueType(Op1), "Inconsistent type");
    IGC_ASSERT_MESSAGE(getType() == getValueType(Op2), "Inconsistent type");
  }
  SPIRVId Condition;
  SPIRVId Op1;
  SPIRVId Op2;
};

class SPIRVLoopMerge : public SPIRVInstruction {
public:
  static const Op OC = OpLoopMerge;
  static const SPIRVWord FixedWordCount = 4;

  SPIRVLoopMerge(SPIRVId TheMergeBlock, SPIRVId TheContinueTarget,
    SPIRVWord TheLoopControl,
    std::vector<SPIRVWord> TheLoopControlParameters,
    SPIRVBasicBlock *BB)
    : SPIRVInstruction(FixedWordCount + TheLoopControlParameters.size(), OC,
      BB),
    MergeBlock(TheMergeBlock), ContinueTarget(TheContinueTarget),
    LoopControl(TheLoopControl),
    LoopControlParameters(TheLoopControlParameters) {
    validate();
    IGC_ASSERT_MESSAGE(BB, "Invalid BB");
  }

  SPIRVLoopMerge()
    : SPIRVInstruction(OC), MergeBlock(SPIRVID_MAX),
    LoopControl(SPIRVWORD_MAX) {
    setHasNoId();
    setHasNoType();
  }

  SPIRVId getMergeBlock() { return MergeBlock; }
  SPIRVId getContinueTarget() { return ContinueTarget; }
  SPIRVWord getLoopControl() const { return LoopControl; }
  std::vector<SPIRVWord> getLoopControlParameters() const {
    return LoopControlParameters;
  }

    void setWordCount(SPIRVWord TheWordCount) override {
    SPIRVEntry::setWordCount(TheWordCount);
    LoopControlParameters.resize(TheWordCount - FixedWordCount);
  }
  _SPIRV_DEF_DEC4_OVERRIDE(MergeBlock, ContinueTarget, LoopControl,
                  LoopControlParameters)

protected:
  SPIRVId MergeBlock;
  SPIRVId ContinueTarget;
  SPIRVWord LoopControl;
  std::vector<SPIRVWord> LoopControlParameters;
};

class SPIRVSwitch: public SPIRVInstruction {
public:
  static const Op OC = OpSwitch;
  static const SPIRVWord FixedWordCount = 3;
  typedef std::vector<SPIRVWord> LiteralTy;
  typedef std::pair<LiteralTy, SPIRVBasicBlock *> PairTy;

  SPIRVSwitch():SPIRVInstruction(OC), Select(SPIRVWORD_MAX),
      Default(SPIRVWORD_MAX) {
    setHasNoId();
    setHasNoType();
  }
  std::vector<SPIRVValue *> getPairs() const {
    return getValues(Pairs);
  }
  SPIRVValue *getSelect() const { return getValue(Select);}
  SPIRVBasicBlock *getDefault() const {
    return static_cast<SPIRVBasicBlock *>(getValue(Default));
  }
  size_t getNumPairs() const { return Pairs.size()/getPairSize();}
  size_t getLiteralsCount() const { return std::max(getSelect()->getType()->getBitWidth(), (uint32_t)32) / (sizeof(SPIRVWord) * 8); }
  size_t getPairSize() const { return getLiteralsCount() + 1; }
  void foreachPair(std::function<void(LiteralTy, SPIRVBasicBlock *)> Func)
      const {
      unsigned PairSize = getPairSize();
      for (size_t I = 0, E = getNumPairs(); I != E; ++I) {
          SPIRVEntry *BB;
          LiteralTy Literals;
          if (!Module->exist(Pairs[PairSize*I + getLiteralsCount()], &BB))
              continue;

          for (unsigned i = 0; i < getLiteralsCount(); ++i) {
              Literals.push_back(Pairs.at(PairSize*I + i));
          }
          Func(Literals, static_cast<SPIRVBasicBlock *>(BB));
      }
  }

  void setWordCount(SPIRVWord TheWordCount) {
    SPIRVEntry::setWordCount(TheWordCount);
    Pairs.resize(TheWordCount - FixedWordCount);
  }
  _SPIRV_DEF_DEC3(Select, Default, Pairs)
  void validate()const {
    IGC_ASSERT(WordCount == Pairs.size() + FixedWordCount);
    IGC_ASSERT(OpCode == OC);
    IGC_ASSERT(getPairSize() > 1);
    IGC_ASSERT(Pairs.size() % getPairSize() == 0);
    foreachPair([=](LiteralTy  Literals, SPIRVBasicBlock *BB){
      IGC_ASSERT(BB->isBasicBlock() || BB->isForward());
    });
    SPIRVInstruction::validate();
  }
protected:
  SPIRVId Select;
  SPIRVId Default;
  std::vector<SPIRVWord> Pairs;
};

class SPIRVUnary:public SPIRVInstTemplateBase {
protected:
  void validate()const {
    auto Op = Ops[0];
    SPIRVInstruction::validate();
    if (getValue(Op)->isForward())
      return;
    if (isGenericNegateOpCode(OpCode)) {
      SPIRVType *resTy = Type->isTypeVector() ?
        Type->getVectorComponentType() : Type;
      SPIRVType *opTy = Type->isTypeVector() ?
        getValueType(Op)->getVectorComponentType() : getValueType(Op);

      IGC_ASSERT_MESSAGE(getType() == getValueType(Op), "Inconsistent type");
      IGC_ASSERT_MESSAGE((resTy->isTypeInt() || resTy->isTypeFloat()), "Invalid type for Generic Negate instruction");
      IGC_ASSERT_MESSAGE((resTy->getBitWidth() == opTy->getBitWidth()), "Invalid bitwidth for Generic Negate instruction");
      IGC_ASSERT_MESSAGE((false == Type->isTypeVector()) || (Type->getVectorComponentCount() == getValueType(Op)->getVectorComponentCount()), "Invalid vector component Width for Generic Negate instruction");
    }
  }
};

template<Op OC>
class SPIRVUnaryInst:public SPIRVInstTemplate<SPIRVUnary, OC, true, 4, false> {
};

#define _SPIRV_OP(x) typedef SPIRVUnaryInst<Op##x> SPIRV##x;
_SPIRV_OP(ConvertFToU)
_SPIRV_OP(ConvertFToS)
_SPIRV_OP(ConvertSToF)
_SPIRV_OP(ConvertUToF)
_SPIRV_OP(UConvert)
_SPIRV_OP(SConvert)
_SPIRV_OP(FConvert)
_SPIRV_OP(SatConvertSToU)
_SPIRV_OP(SatConvertUToS)
_SPIRV_OP(ConvertPtrToU)
_SPIRV_OP(ConvertUToPtr)
_SPIRV_OP(PtrCastToGeneric)
_SPIRV_OP(GenericCastToPtr)
_SPIRV_OP(Bitcast)
_SPIRV_OP(SNegate)
_SPIRV_OP(FNegate)
_SPIRV_OP(Not)
_SPIRV_OP(LogicalNot)
_SPIRV_OP(IsNan)
_SPIRV_OP(IsInf)
_SPIRV_OP(IsFinite)
_SPIRV_OP(IsNormal)
_SPIRV_OP(SignBitSet)
_SPIRV_OP(Any)
_SPIRV_OP(All)
_SPIRV_OP(ConvertFToBF16INTEL)
_SPIRV_OP(ConvertBF16ToFINTEL)
_SPIRV_OP(ArithmeticFenceINTEL)
_SPIRV_OP(BitReverse)
#undef _SPIRV_OP

class SPIRVAccessChainBase :public SPIRVInstTemplateBase {
public:
  SPIRVValue *getBase() { return this->getValue(this->Ops[0]);}
  std::vector<SPIRVValue *> getIndices()const {
    std::vector<SPIRVWord> IndexWords(this->Ops.begin() + 1, this->Ops.end());
    return this->getValues(IndexWords);
  }
  bool isInBounds() {
    return OpCode == OpInBoundsAccessChain ||
        OpCode == OpInBoundsPtrAccessChain;
  }
  bool hasPtrIndex() {
    return OpCode == OpPtrAccessChain ||
        OpCode == OpInBoundsPtrAccessChain;
  }
};

template<Op OC, unsigned FixedWC>
class SPIRVAccessChainGeneric
    :public SPIRVInstTemplate<SPIRVAccessChainBase, OC, true, FixedWC, true> {
};

typedef SPIRVAccessChainGeneric<OpAccessChain, 4> SPIRVAccessChain;
typedef SPIRVAccessChainGeneric<OpInBoundsAccessChain, 4>
  SPIRVInBoundsAccessChain;
typedef SPIRVAccessChainGeneric<OpPtrAccessChain, 5> SPIRVPtrAccessChain;
typedef SPIRVAccessChainGeneric<OpInBoundsPtrAccessChain, 5>
  SPIRVInBoundsPtrAccessChain;

class SPIRVLoopControlINTEL : public SPIRVInstruction {
public:
    static const Op OC = OpLoopControlINTEL;
    static const SPIRVWord FixedWordCount = 2;

    SPIRVLoopControlINTEL(SPIRVWord TheLoopControl,
        std::vector<SPIRVWord> TheLoopControlParameters,
        SPIRVBasicBlock* BB)
        : SPIRVInstruction(FixedWordCount + TheLoopControlParameters.size(), OC,
            BB),
        LoopControl(TheLoopControl),
        LoopControlParameters(TheLoopControlParameters) {
        validate();
        IGC_ASSERT_MESSAGE(BB, "Invalid BB");
    }

    SPIRVLoopControlINTEL() : SPIRVInstruction(OC), LoopControl(SPIRVWORD_MAX) {
        setHasNoId();
        setHasNoType();
    }

    SPIRVWord getLoopControl() const { return LoopControl; }

    std::vector<SPIRVWord> getLoopControlParameters() const {
        return LoopControlParameters;
    }

    CapVec getRequiredCapability() const override {
        return getVec(CapabilityUnstructuredLoopControlsINTEL);
    }

    void setWordCount(SPIRVWord TheWordCount) override {
        SPIRVEntry::setWordCount(TheWordCount);
        LoopControlParameters.resize(TheWordCount - FixedWordCount);
    }
    _SPIRV_DEF_DEC2(LoopControl, LoopControlParameters)

protected:
    SPIRVWord LoopControl;
    std::vector<SPIRVWord> LoopControlParameters;
};

template<Op OC, SPIRVWord FixedWordCount>
class SPIRVFunctionCallGeneric: public SPIRVInstruction {
public:
  SPIRVFunctionCallGeneric(SPIRVType *TheType, SPIRVId TheId,
    const std::vector<SPIRVWord> &TheArgs,
    SPIRVBasicBlock *BB)
    : SPIRVInstruction(TheArgs.size() + FixedWordCount, OC, TheType, TheId,
      BB),
    Args(TheArgs) {
    SPIRVFunctionCallGeneric::validate();
    IGC_ASSERT_MESSAGE(BB, "Invalid BB");
  }
  SPIRVFunctionCallGeneric(SPIRVType *TheType, SPIRVId TheId,
    const std::vector<SPIRVValue *> &TheArgs,
    SPIRVBasicBlock *BB)
    : SPIRVInstruction(TheArgs.size() + FixedWordCount, OC, TheType, TheId,
      BB) {
    Args = getIds(TheArgs);
    SPIRVFunctionCallGeneric::validate();
    IGC_ASSERT_MESSAGE(BB, "Invalid BB");
  }

  SPIRVFunctionCallGeneric(SPIRVModule *BM, SPIRVWord ResId, SPIRVType *TheType,
    const std::vector<SPIRVWord> &TheArgs)
    : SPIRVInstruction(TheArgs.size() + FixedWordCount, OC, TheType, ResId, NULL,
      BM),
    Args(TheArgs) {}
  SPIRVFunctionCallGeneric():SPIRVInstruction(OC) {}
  const std::vector<SPIRVWord> &getArguments() {
    return Args;
  }
  std::vector<SPIRVValue *> getArgumentValues() {
    return getValues(Args);
  }
  std::vector<SPIRVType *> getArgumentValueTypes()const {
    std::vector<SPIRVType *> ArgTypes;
    for (auto &I:Args)
      ArgTypes.push_back(getValue(I)->getType());
    return ArgTypes;
  }
  void setWordCount(SPIRVWord TheWordCount) {
    SPIRVEntry::setWordCount(TheWordCount);
    Args.resize(TheWordCount - FixedWordCount);
  }
  void validate()const {
    SPIRVInstruction::validate();
  }
protected:
  std::vector<SPIRVWord> Args;
};

class SPIRVFunctionCall:
    public SPIRVFunctionCallGeneric<OpFunctionCall, 4> {
public:
  SPIRVFunctionCall():FunctionId(SPIRVID_INVALID) {}
  SPIRVFunction *getFunction()const {
    return SPIRVEntry::get<SPIRVFunction>(FunctionId);
  }
  _SPIRV_DEF_DEC4(Type, Id, FunctionId, Args)
  void validate()const;
  bool isOperandLiteral(unsigned Index) const { return false;}
protected:
  SPIRVId FunctionId;
};

class SPIRVFunctionPointerCallINTEL
  : public SPIRVFunctionCallGeneric<OpFunctionPointerCallINTEL, 4> {
public:
  SPIRVFunctionPointerCallINTEL(SPIRVId TheId, SPIRVValue* TheCalledValue,
    SPIRVType* TheReturnType,
    const std::vector<SPIRVWord>& TheArgs,
    SPIRVBasicBlock* BB);
  SPIRVFunctionPointerCallINTEL() : CalledValueId(SPIRVID_INVALID) {}
  SPIRVValue* getCalledValue() const { return SPIRVEntry::get<SPIRVValue>(CalledValueId); }
  _SPIRV_DEF_DEC4_OVERRIDE(Type, Id, CalledValueId, Args)
  void validate() const override;
  bool isOperandLiteral(unsigned Index) const override{ return false; }
  CapVec getRequiredCapability() const override {
    return getVec(CapabilityFunctionPointersINTEL);
  }

protected:
  SPIRVId CalledValueId;
};

class SPIRVExtInst: public SPIRVFunctionCallGeneric<OpExtInst, 5> {
public:
  SPIRVExtInst(SPIRVExtInstSetKind SetKind = SPIRVEIS_Count,
      unsigned ExtOC = SPIRVWORD_MAX)
    :ExtSetId(SPIRVWORD_MAX), ExtOp(ExtOC), ExtSetKind(SetKind) {}
  void setExtSetId(unsigned Set) { ExtSetId = Set;}
  void setExtOp(unsigned ExtOC) { ExtOp = ExtOC;}
  SPIRVId getExtSetId()const {
    return ExtSetId;
  }
  SPIRVWord getExtOp()const {
    return ExtOp;
  }
  void setExtSetKindById() {
    IGC_ASSERT_MESSAGE(Module, "Invalid module");
    ExtSetKind = Module->getBuiltinSet(ExtSetId);
    IGC_ASSERT_MESSAGE((ExtSetKind == SPIRVEIS_OpenCL) || (ExtSetKind == SPIRVEIS_DebugInfo) ||
        (ExtSetKind == SPIRVEIS_OpenCL_DebugInfo_100), "not supported");
  }
  void decode(std::istream &I) {
    getDecoder(I) >> Type >> Id >> ExtSetId;
    setExtSetKindById();
    switch(ExtSetKind) {
    case SPIRVEIS_OpenCL:
      getDecoder(I) >> ExtOpOCL;
      break;
    case SPIRVEIS_DebugInfo:
    case SPIRVEIS_OpenCL_DebugInfo_100:
        getDecoder(I) >> ExtOpDbgInfo;
        break;
    default:
      IGC_ASSERT_EXIT_MESSAGE(0, "not supported");
      getDecoder(I) >> ExtOp;
      break;
    }
    getDecoder(I) >> Args;
  }
  void validate()const {
    SPIRVFunctionCallGeneric::validate();
    validateBuiltin(ExtSetId, ExtOp);
  }
  bool isOperandLiteral(unsigned Index) const {
    IGC_ASSERT_MESSAGE(ExtSetKind == SPIRVEIS_OpenCL, "Unsupported extended instruction set");
    auto EOC = static_cast<OCLExtOpKind>(ExtOp);
    switch(EOC) {
    default:
      return false;
    case igc_OpenCLLIB::vloadn:
    case igc_OpenCLLIB::vload_halfn:
    case igc_OpenCLLIB::vloada_halfn:
      return Index == 2;
    case igc_OpenCLLIB::vstore_half_r:
    case igc_OpenCLLIB::vstore_halfn_r:
    case igc_OpenCLLIB::vstorea_halfn_r:
      return Index == 3;
    }
  }
  SPIRVExtInstSetKind getExtSetKind()
  {
      return ExtSetKind;
  }
  bool hasNoScope()
  {
      if ((getExtSetKind() == SPIRVExtInstSetKind::SPIRVEIS_DebugInfo ||
          getExtSetKind() == SPIRVExtInstSetKind::SPIRVEIS_OpenCL_DebugInfo_100) &&
          ExtOpDbgInfo != OCLExtOpDbgKind::DbgDcl &&
          ExtOpDbgInfo != OCLExtOpDbgKind::DbgVal)
          return true;
      else
          return false;
  }

  bool isScope() { return startsScope() || endsScope(); }

  bool startsScope()
  {
      if ((getExtSetKind() == SPIRVExtInstSetKind::SPIRVEIS_DebugInfo ||
          getExtSetKind() == SPIRVExtInstSetKind::SPIRVEIS_OpenCL_DebugInfo_100) &&
          ExtOpDbgInfo == OCLExtOpDbgKind::Scope)
          return true;
      return false;
  }

  bool endsScope()
  {
      if ((getExtSetKind() == SPIRVExtInstSetKind::SPIRVEIS_DebugInfo ||
          getExtSetKind() == SPIRVExtInstSetKind::SPIRVEIS_OpenCL_DebugInfo_100) &&
          ExtOpDbgInfo == OCLExtOpDbgKind::NoScope)
          return true;
      return false;
  }

  std::vector<SPIRVValue*> getValues(const std::vector<SPIRVId>& Args) {
      std::vector<SPIRVValue*> VArgs;
      for (size_t I = 0; I < Args.size(); ++I) {
          if (isOperandLiteral(I))
              VArgs.push_back(Module->getLiteralAsConstant(Args[I]));
          else
              VArgs.push_back(getValue(Args[I]));
      }
      return VArgs;
  }

protected:
  SPIRVId ExtSetId;
  union {
    SPIRVWord ExtOp;
    OCLExtOpKind ExtOpOCL;
    OCLExtOpDbgKind ExtOpDbgInfo;
  };
  SPIRVExtInstSetKind ExtSetKind;
};

class SPIRVCompositeExtract:public SPIRVInstruction {
public:
  const static Op OC = OpCompositeExtract;
  // Incomplete constructor
  SPIRVCompositeExtract():SPIRVInstruction(OC), Composite(SPIRVID_INVALID){}

  SPIRVValue *getComposite() { return getValue(Composite);}
  const std::vector<SPIRVWord>& getIndices()const { return Indices;}
protected:
  void setWordCount(SPIRVWord TheWordCount) {
    SPIRVEntry::setWordCount(TheWordCount);
    Indices.resize(TheWordCount - 4);
  }
  _SPIRV_DEF_DEC4(Type, Id, Composite, Indices)
  // ToDo: validate the result type is consistent with the base type and indices
  // need to trace through the base type for struct types
  void validate()const {
    SPIRVInstruction::validate();
    IGC_ASSERT(getValueType(Composite)->isTypeArray() || getValueType(Composite)->isTypeStruct() || getValueType(Composite)->isTypeVector());
  }
  SPIRVId Composite;
  std::vector<SPIRVWord> Indices;
};

class SPIRVCompositeInsert:public SPIRVInstruction {
public:
  const static Op OC = OpCompositeInsert;
  const static SPIRVWord FixedWordCount = 5;
  // Incomplete constructor
  SPIRVCompositeInsert():SPIRVInstruction(OC), Object(SPIRVID_INVALID),
      Composite(SPIRVID_INVALID){}

  SPIRVValue *getObject() { return getValue(Object);}
  SPIRVValue *getComposite() { return getValue(Composite);}
  const std::vector<SPIRVWord>& getIndices()const { return Indices;}
protected:
  void setWordCount(SPIRVWord TheWordCount) {
    SPIRVEntry::setWordCount(TheWordCount);
    Indices.resize(TheWordCount - FixedWordCount);
  }
  _SPIRV_DEF_DEC5(Type, Id, Object, Composite, Indices)
  // ToDo: validate the object type is consistent with the base type and indices
  // need to trace through the base type for struct types
  void validate()const {
    SPIRVInstruction::validate();
    IGC_ASSERT(OpCode == OC);
    IGC_ASSERT(WordCount == Indices.size() + FixedWordCount);
    IGC_ASSERT(getValueType(Composite)->isTypeArray() || getValueType(Composite)->isTypeStruct() || getValueType(Composite)->isTypeVector());
    IGC_ASSERT(Type == getValueType(Composite));
  }
  SPIRVId Object;
  SPIRVId Composite;
  std::vector<SPIRVWord> Indices;
};

class SPIRVCompositeConstruct: public SPIRVInstruction {
public:
  // Complete constructor for composite construct
  const static Op OC = OpCompositeConstruct;

  // Default constructor
  SPIRVCompositeConstruct():SPIRVInstruction( OpCompositeConstruct ){}
  std::vector<SPIRVValue*> getElements()const {
    return getValues( Elements );
  }
protected:
  void validate() const {
    SPIRVValue::validate();
    for(auto &I : Elements)
      getValue( I )->validate();
  }
  void setWordCount( SPIRVWord WordCount ) {
    Elements.resize( WordCount - 3 );
  }
  _SPIRV_DEF_DEC3( Type,Id,Elements )

    std::vector<SPIRVId> Elements;
};

class SPIRVCopyMemory :public SPIRVInstruction, public SPIRVMemoryAccess {
public:
  const static Op OC = OpCopyMemory;
  const static SPIRVWord FixedWords = 3;

  // Incomplete constructor
  SPIRVCopyMemory() :SPIRVInstruction(OC), SPIRVMemoryAccess(),
      Target(SPIRVID_INVALID),
    Source(SPIRVID_INVALID) {
    setHasNoId();
    setHasNoType();
  }

  SPIRVValue *getSource() { return getValue(Source); }
  SPIRVValue *getTarget() { return getValue(Target); }

protected:
  void setWordCount(SPIRVWord TheWordCount) {
    SPIRVEntry::setWordCount(TheWordCount);
    MemoryAccess.resize(TheWordCount - FixedWords);
  }

  void decode(std::istream &I) {
    getDecoder(I) >> Target >> Source >> MemoryAccess;
    MemoryAccessUpdate(MemoryAccess);
  }

  void validate()const {
    IGC_ASSERT_MESSAGE((getValueType(Id) == getValueType(Source)), "Inconsistent type");
    IGC_ASSERT_MESSAGE(getValueType(Id)->isTypePointer(), "Invalid type");
    IGC_ASSERT_MESSAGE(!(getValueType(Id)->getPointerElementType()->isTypeVoid()), "Invalid type");
    SPIRVInstruction::validate();
  }

  std::vector<SPIRVWord> MemoryAccess;
  SPIRVId Target;
  SPIRVId Source;
};

class SPIRVCopyMemorySized :public SPIRVInstruction, public SPIRVMemoryAccess {
public:
  const static Op OC = OpCopyMemorySized;
  const static SPIRVWord FixedWords = 4;
  // Incomplete constructor
  SPIRVCopyMemorySized() :SPIRVInstruction(OC), SPIRVMemoryAccess(),
      Target(SPIRVID_INVALID), Source(SPIRVID_INVALID), Size(0) {
    setHasNoId();
    setHasNoType();
  }

  SPIRVValue *getSource() { return getValue(Source); }
  SPIRVValue *getTarget() { return getValue(Target); }
  SPIRVValue *getSize() { return getValue(Size); }

protected:
  void setWordCount(SPIRVWord TheWordCount) {
    SPIRVEntry::setWordCount(TheWordCount);
    MemoryAccess.resize(TheWordCount - FixedWords);
  }

  void decode(std::istream &I) {
    getDecoder(I) >> Target >> Source >> Size >> MemoryAccess;
    MemoryAccessUpdate(MemoryAccess);
  }

    void validate()const {
    SPIRVInstruction::validate();
  }

  std::vector<SPIRVWord> MemoryAccess;
  SPIRVId Target;
  SPIRVId Source;
  SPIRVId Size;
};

class SPIRVVectorExtractDynamic:public SPIRVInstruction {
public:
  const static Op OC = OpVectorExtractDynamic;
  // Incomplete constructor
  SPIRVVectorExtractDynamic():SPIRVInstruction(OC), VectorId(SPIRVID_INVALID),
      IndexId(SPIRVID_INVALID){}

  SPIRVValue *getVector() { return getValue(VectorId);}
  SPIRVValue *getIndex()const { return getValue(IndexId);}
protected:
  _SPIRV_DEF_DEC4(Type, Id, VectorId, IndexId)
  void validate()const {
    SPIRVInstruction::validate();
    if (getValue(VectorId)->isForward())
      return;
    IGC_ASSERT(getValueType(VectorId)->isTypeVector()
        || getValue(VectorId)->getType()->getOpCode() == OpTypeJointMatrixINTEL);
  }
  SPIRVId VectorId;
  SPIRVId IndexId;
};

class SPIRVVectorInsertDynamic :public SPIRVInstruction {
public:
  const static Op OC = OpVectorInsertDynamic;
  // Incomplete constructor
  SPIRVVectorInsertDynamic() :SPIRVInstruction(OC), VectorId(SPIRVID_INVALID),
    IndexId(SPIRVID_INVALID), ComponentId(SPIRVID_INVALID){}

  SPIRVValue *getVector() { return getValue(VectorId); }
  SPIRVValue *getIndex()const { return getValue(IndexId); }
  SPIRVValue *getComponent() { return getValue(ComponentId); }
protected:
  _SPIRV_DEF_DEC5(Type, Id, VectorId, ComponentId, IndexId)
    void validate()const {
    SPIRVInstruction::validate();
    if (getValue(VectorId)->isForward())
      return;
    IGC_ASSERT(getValueType(VectorId)->isTypeVector()
        || getValue(VectorId)->getType()->getOpCode() == OpTypeJointMatrixINTEL);
  }
  SPIRVId VectorId;
  SPIRVId IndexId;
  SPIRVId ComponentId;
};

class SPIRVVectorShuffle:public SPIRVInstruction {
public:
  const static Op OC = OpVectorShuffle;
  const static SPIRVWord FixedWordCount = 5;
  // Incomplete constructor
  SPIRVVectorShuffle():SPIRVInstruction(OC), Vector1(SPIRVID_INVALID),
      Vector2(SPIRVID_INVALID){}

  SPIRVValue *getVector1() { return getValue(Vector1);}
  SPIRVValue *getVector2() { return getValue(Vector2);}
  const std::vector<SPIRVWord>& getComponents()const { return Components;}
protected:
  void setWordCount(SPIRVWord TheWordCount) {
    SPIRVEntry::setWordCount(TheWordCount);
    Components.resize(TheWordCount - FixedWordCount);
  }
  _SPIRV_DEF_DEC5(Type, Id, Vector1, Vector2, Components)
  void validate()const {
    SPIRVInstruction::validate();
    IGC_ASSERT(OpCode == OC);
    IGC_ASSERT(WordCount == Components.size() + FixedWordCount);
    IGC_ASSERT(Type->isTypeVector());
    IGC_ASSERT(Type->getVectorComponentType() == getValueType(Vector1)->getVectorComponentType());
    if (getValue(Vector1)->isForward() ||
        getValue(Vector2)->isForward())
      return;
    IGC_ASSERT(getValueType(Vector1) == getValueType(Vector2));
    size_t CompCount = Type->getVectorComponentCount();
    IGC_ASSERT(Components.size() == CompCount);
    IGC_ASSERT(Components.size() > 1);
  }
  SPIRVId Vector1;
  SPIRVId Vector2;
  std::vector<SPIRVWord> Components;
};

class SPIRVGroupAsyncCopy:public SPIRVInstruction {
public:
  static const Op OC = OpGroupAsyncCopy;
  static const SPIRVWord WC = 9;
  // Incomplete constructor
  SPIRVGroupAsyncCopy():SPIRVInstruction(OC), ExecScope(ScopeInvocation),
      Destination(SPIRVID_INVALID), Source(SPIRVID_INVALID),
      NumElements(SPIRVID_INVALID), Stride(SPIRVID_INVALID),
      Event(SPIRVID_INVALID){
  }
  Scope getExecScope() const {
    return ExecScope;
  }
  SPIRVValue *getDestination()const { return getValue(Destination);}
  SPIRVValue *getSource()const { return getValue(Source);}
  SPIRVValue *getNumElements()const { return getValue(NumElements);}
  SPIRVValue *getStride()const { return getValue(Stride);}
  SPIRVValue *getEvent()const { return getValue(Event);}
  std::vector<SPIRVValue *> getOperands() {
    std::vector<SPIRVId> Operands;
    Operands.push_back(ExecScope);
    Operands.push_back(Destination);
    Operands.push_back(Source);
    Operands.push_back(NumElements);
    Operands.push_back(Stride);
    Operands.push_back(Event);
    return getValues(Operands);
  }

protected:
  _SPIRV_DEF_DEC8(Type, Id, ExecScope, Destination, Source, NumElements,
      Stride, Event)
  void validate()const {
    IGC_ASSERT(OpCode == OC);
    IGC_ASSERT(WordCount == WC);
    SPIRVInstruction::validate();
    isValid(ExecScope);
  }
  Scope ExecScope;
  SPIRVId Destination;
  SPIRVId Source;
  SPIRVId NumElements;
  SPIRVId Stride;
  SPIRVId Event;
};

enum SPIRVOpKind {
  SPIRVOPK_Id,
  SPIRVOPK_Literal,
  SPIRVOPK_Count
};

class SPIRVDevEnqInstBase:public SPIRVInstTemplateBase {
public:
  CapVec getRequiriedCapability() const {
     return getVec(SPIRVCapabilityKind::CapabilityDeviceEnqueue);
  }
};

class SPIRVSubgroupDispatchInstBase:public SPIRVInstTemplateBase {
public:
  CapVec getRequiriedCapability() const {
     return getVec(SPIRVCapabilityKind::CapabilitySubgroupDispatch);
  }
};

#define _SPIRV_OP(x, ...) \
  typedef SPIRVInstTemplate<SPIRVDevEnqInstBase, Op##x, __VA_ARGS__> \
      SPIRV##x;
// CL 2.0 enqueue kernel builtins
_SPIRV_OP(EnqueueMarker, true, 7)
_SPIRV_OP(EnqueueKernel, true, 13, true)
_SPIRV_OP(GetKernelNDrangeSubGroupCount, true, 8)
_SPIRV_OP(GetKernelNDrangeMaxSubGroupSize, true, 8)
_SPIRV_OP(GetKernelWorkGroupSize, true, 7)
_SPIRV_OP(GetKernelPreferredWorkGroupSizeMultiple, true, 7)
_SPIRV_OP(RetainEvent, false, 2)
_SPIRV_OP(ReleaseEvent, false, 2)
_SPIRV_OP(CreateUserEvent, true, 3)
_SPIRV_OP(IsValidEvent, true, 4)
_SPIRV_OP(SetUserEventStatus, false, 3)
_SPIRV_OP(CaptureEventProfilingInfo, false, 4)
_SPIRV_OP(GetDefaultQueue, true, 3)
_SPIRV_OP(BuildNDRange, true, 6)
#undef _SPIRV_OP

// Subgroup Dispatch opcodes
#define _SPIRV_OP(x, ...) \
  typedef SPIRVInstTemplate<SPIRVSubgroupDispatchInstBase, Op##x, __VA_ARGS__> \
      SPIRV##x;
_SPIRV_OP(GetKernelLocalSizeForSubgroupCount, true, 8)
_SPIRV_OP(GetKernelMaxNumSubgroups, true, 7)
#undef _SPIRV_OP

class SPIRVPipeInstBase:public SPIRVInstTemplateBase {
public:
  CapVec getRequiriedCapability() const {
     return getVec(SPIRVCapabilityKind::CapabilityPipes);
  }
};

class SPIRVPipeStorageInstBase : public SPIRVInstTemplateBase {
public:
  CapVec getRequiriedCapability() const {
     return getVec(SPIRVCapabilityKind::CapabilityPipeStorage);
  }
};

#define _SPIRV_OP(x, ...) \
  typedef SPIRVInstTemplate<SPIRVPipeInstBase, Op##x, __VA_ARGS__> \
      SPIRV##x;
// CL 2.0 pipe builtins
_SPIRV_OP(ReadPipe, true, 7)
_SPIRV_OP(WritePipe, true, 7)
_SPIRV_OP(ReservedReadPipe, true, 9)
_SPIRV_OP(ReservedWritePipe, true, 9)
_SPIRV_OP(ReserveReadPipePackets, true, 7)
_SPIRV_OP(ReserveWritePipePackets, true, 7)
_SPIRV_OP(CommitReadPipe, false, 5)
_SPIRV_OP(CommitWritePipe, false, 5)
_SPIRV_OP(IsValidReserveId, true, 4)
_SPIRV_OP(GetNumPipePackets, true, 6)
_SPIRV_OP(GetMaxPipePackets, true, 6)
#undef _SPIRV_OP

#define _SPIRV_OP(x, ...) \
  typedef SPIRVInstTemplate<SPIRVPipeStorageInstBase, Op##x, __VA_ARGS__> \
      SPIRV##x;
// Pipe storage builtins
_SPIRV_OP(CreatePipeFromPipeStorage, true, 4)
#undef _SPIRV_OP

class SPIRVGroupInstBase:public SPIRVInstTemplateBase {
public:
  CapVec getRequiriedCapability() const {
     return getVec(SPIRVCapabilityKind::CapabilityGroups);
  }
};

#define _SPIRV_OP(x, ...) \
  typedef SPIRVInstTemplate<SPIRVGroupInstBase, Op##x, __VA_ARGS__> \
      SPIRV##x;
// Group instructions
_SPIRV_OP(GroupWaitEvents, false, 4)
_SPIRV_OP(GroupAll, true, 5)
_SPIRV_OP(GroupAny, true, 5)
_SPIRV_OP(GroupBroadcast, true, 6)
_SPIRV_OP(GroupIAdd, true, 6, false, 1)
_SPIRV_OP(GroupFAdd, true, 6, false, 1)
_SPIRV_OP(GroupFMin, true, 6, false, 1)
_SPIRV_OP(GroupUMin, true, 6, false, 1)
_SPIRV_OP(GroupSMin, true, 6, false, 1)
_SPIRV_OP(GroupFMax, true, 6, false, 1)
_SPIRV_OP(GroupUMax, true, 6, false, 1)
_SPIRV_OP(GroupSMax, true, 6, false, 1)
// SPV_KHR_uniform_group_instructions
_SPIRV_OP(GroupIMulKHR, true, 6, false, 1)
_SPIRV_OP(GroupFMulKHR, true, 6, false, 1)
_SPIRV_OP(GroupBitwiseAndKHR, true, 6, false, 1)
_SPIRV_OP(GroupBitwiseOrKHR, true, 6, false, 1)
_SPIRV_OP(GroupBitwiseXorKHR, true, 6, false, 1)
_SPIRV_OP(GroupLogicalAndKHR, true, 6, false, 1)
_SPIRV_OP(GroupLogicalOrKHR, true, 6, false, 1)
_SPIRV_OP(GroupLogicalXorKHR, true, 6, false, 1)
_SPIRV_OP(GroupReserveReadPipePackets, true, 8)
_SPIRV_OP(GroupReserveWritePipePackets, true, 8)
_SPIRV_OP(GroupCommitReadPipe, false, 6)
_SPIRV_OP(GroupCommitWritePipe, false, 6)
#undef _SPIRV_OP

class SPIRVGroupNonUniformInstBase :public SPIRVInstTemplateBase {
public:
    CapVec getRequiriedCapability() const {
        return getVec(SPIRVCapabilityKind::CapabilityNonUniform);
    }
};

#define _SPIRV_OP(x, ...) \
  typedef SPIRVInstTemplate<SPIRVGroupNonUniformInstBase, Op##x, __VA_ARGS__> \
      SPIRV##x;
// Non uniform group instructions
_SPIRV_OP(GroupNonUniformElect, true, 4)
#undef _SPIRV_OP

class SPIRVGroupNonUniformVoteInstBase :public SPIRVInstTemplateBase {
public:
    CapVec getRequiriedCapability() const {
        return getVec(SPIRVCapabilityKind::CapabilityNonUniformVote);
    }
};

#define _SPIRV_OP(x, ...) \
  typedef SPIRVInstTemplate<SPIRVGroupNonUniformVoteInstBase, Op##x, __VA_ARGS__> \
      SPIRV##x;
// Non uniform vote group instructions
_SPIRV_OP(GroupNonUniformAll, true, 5)
_SPIRV_OP(GroupNonUniformAny, true, 5)
_SPIRV_OP(GroupNonUniformAllEqual, true, 5)
#undef _SPIRV_OP

class SPIRVGroupNonUniformBallotInstBase :public SPIRVInstTemplateBase {
public:
    CapVec getRequiriedCapability() const {
        return getVec(SPIRVCapabilityKind::CapabilityNonUniformBallot);
    }
};

#define _SPIRV_OP(x, ...) \
  typedef SPIRVInstTemplate<SPIRVGroupNonUniformBallotInstBase, Op##x, __VA_ARGS__> \
      SPIRV##x;
// Non uniform ballot group instructions
_SPIRV_OP(GroupNonUniformBroadcast, true, 6)
_SPIRV_OP(GroupNonUniformBroadcastFirst, true, 5)
_SPIRV_OP(GroupNonUniformBallot, true, 5)
_SPIRV_OP(GroupNonUniformInverseBallot, true, 5)
_SPIRV_OP(GroupNonUniformBallotBitExtract, true, 6)
_SPIRV_OP(GroupNonUniformBallotBitCount, true, 6, false, 1)
_SPIRV_OP(GroupNonUniformBallotFindLSB, true, 5)
_SPIRV_OP(GroupNonUniformBallotFindMSB, true, 5)
#undef _SPIRV_OP

class SPIRVGroupNonUniformArithmeticInstBase :public SPIRVInstTemplateBase {
public:
    CapVec getRequiriedCapability() const {
        return getVec(SPIRVCapabilityKind::CapabilityNonUniformArithmetic);
    }
};

#define _SPIRV_OP(x, ...) \
  typedef SPIRVInstTemplate<SPIRVGroupNonUniformArithmeticInstBase, Op##x, __VA_ARGS__> \
      SPIRV##x;
// Non uniform ballot group instructions
_SPIRV_OP(GroupNonUniformIAdd, true, 6, true, 1)  // TODO: handle optional ClusterSize
_SPIRV_OP(GroupNonUniformFAdd, true, 6, true, 1)
_SPIRV_OP(GroupNonUniformIMul, true, 6, true, 1)
_SPIRV_OP(GroupNonUniformFMul, true, 6, true, 1)
_SPIRV_OP(GroupNonUniformSMin, true, 6, true, 1)
_SPIRV_OP(GroupNonUniformUMin, true, 6, true, 1)
_SPIRV_OP(GroupNonUniformFMin, true, 6, true, 1)
_SPIRV_OP(GroupNonUniformSMax, true, 6, true, 1)
_SPIRV_OP(GroupNonUniformUMax, true, 6, true, 1)
_SPIRV_OP(GroupNonUniformFMax, true, 6, true, 1)
_SPIRV_OP(GroupNonUniformBitwiseAnd, true, 6, true, 1)
_SPIRV_OP(GroupNonUniformBitwiseOr, true, 6, true, 1)
_SPIRV_OP(GroupNonUniformBitwiseXor, true, 6, true, 1)
_SPIRV_OP(GroupNonUniformLogicalAnd, true, 6, true, 1)
_SPIRV_OP(GroupNonUniformLogicalOr, true, 6, true, 1)
_SPIRV_OP(GroupNonUniformLogicalXor, true, 6, true, 1)
#undef _SPIRV_OP

class SPIRVGroupNonUniformShuffleInstBase :public SPIRVInstTemplateBase {
public:
    CapVec getRequiriedCapability() const {
        return getVec(SPIRVCapabilityKind::CapabilityNonUniformShuffle);
    }
};

#define _SPIRV_OP(x, ...) \
  typedef SPIRVInstTemplate<SPIRVGroupNonUniformVoteInstBase, Op##x, __VA_ARGS__> \
      SPIRV##x;
// Non uniform shuffle group instructions
_SPIRV_OP(GroupNonUniformShuffle, true, 6)
_SPIRV_OP(GroupNonUniformShuffleXor, true, 6)
#undef _SPIRV_OP

class SPIRVGroupNonUniformShuffleRelativeInstBase :public SPIRVInstTemplateBase {
public:
    CapVec getRequiriedCapability() const {
        return getVec(SPIRVCapabilityKind::CapabilityNonUniformShuffle);
    }
};

#define _SPIRV_OP(x, ...) \
  typedef SPIRVInstTemplate<SPIRVGroupNonUniformShuffleRelativeInstBase, Op##x, __VA_ARGS__> \
      SPIRV##x;
// Non uniform shuffle relative group instructions
_SPIRV_OP(GroupNonUniformShuffleUp, true, 6)
_SPIRV_OP(GroupNonUniformShuffleDown, true, 6)
#undef _SPIRV_OP

class SPIRVAtomicInstBase:public SPIRVInstTemplateBase {
public:
  CapVec getRequiriedCapability() const {
     return getVec(SPIRVCapabilityKind::CapabilityInt64Atomics);
  }
};

#define _SPIRV_OP(x, ...) \
  typedef SPIRVInstTemplate<SPIRVAtomicInstBase, Op##x, __VA_ARGS__> \
      SPIRV##x;
// Atomic builtins
_SPIRV_OP(AtomicFlagTestAndSet, true, 6)
_SPIRV_OP(AtomicFlagClear, false, 4)
_SPIRV_OP(AtomicLoad, true, 6)
_SPIRV_OP(AtomicStore, false, 5)
_SPIRV_OP(AtomicExchange, true, 7)
_SPIRV_OP(AtomicCompareExchange, true, 9)
_SPIRV_OP(AtomicCompareExchangeWeak, true, 9)
_SPIRV_OP(AtomicIIncrement, true, 6)
_SPIRV_OP(AtomicIDecrement, true, 6)
_SPIRV_OP(AtomicIAdd, true, 7)
_SPIRV_OP(AtomicISub, true, 7)
_SPIRV_OP(AtomicUMin, true, 7)
_SPIRV_OP(AtomicUMax, true, 7)
_SPIRV_OP(AtomicSMin, true, 7)
_SPIRV_OP(AtomicSMax, true, 7)
_SPIRV_OP(AtomicAnd, true, 7)
_SPIRV_OP(AtomicOr, true, 7)
_SPIRV_OP(AtomicXor, true, 7)
#undef _SPIRV_OP

#define _SPIRV_OP(x, ...) \
  typedef SPIRVInstTemplate<SPIRVInstTemplateBase, Op##x, __VA_ARGS__> \
      SPIRV##x;
// SPV_EXT_shader_atomic_float_min_max
_SPIRV_OP(AtomicFMinEXT, true, 7)
_SPIRV_OP(AtomicFMaxEXT, true, 7)
// SPV_EXT_shader_atomic_float_add
_SPIRV_OP(AtomicFAddEXT, true, 7)
#undef _SPIRV_OP

class SPIRVNamedBarrierInstBase :public SPIRVInstTemplateBase {
public:
    CapVec getRequiriedCapability() const {
        return getVec(SPIRVCapabilityKind::CapabilityNamedBarrier);
    }
};

#define _SPIRV_OP(x, ...) \
    typedef SPIRVInstTemplate<SPIRVInstTemplateBase, Op##x, __VA_ARGS__> \
    SPIRV##x;
_SPIRV_OP(NamedBarrierInitialize, true /*HasId*/, 4 /*WC*/, false /*VariWC*/)
_SPIRV_OP(MemoryNamedBarrier, false /*HasId*/, 4 /*WC*/, false /*VariWC*/)
#undef _SPIRV_OP



#define _SPIRV_OP(x, ...) \
  typedef SPIRVInstTemplate<SPIRVInstTemplateBase, Op##x, __VA_ARGS__> \
      SPIRV##x;
_SPIRV_OP(MemoryBarrier, false /*HasId*/, 3 /*WC*/, false /*VariWC*/)
_SPIRV_OP(ControlBarrier, false /*HasId*/, 4 /*WC*/, false /*VariWC*/)
_SPIRV_OP(ControlBarrierArriveINTEL, false /*HasId*/, 4 /*WC*/, false /*VariWC*/)
_SPIRV_OP(ControlBarrierWaitINTEL, false /*HasId*/, 4 /*WC*/, false /*VariWC*/)
_SPIRV_OP(UMulExtended, true, 5)
_SPIRV_OP(SMulExtended, true, 5)
_SPIRV_OP(BitCount, true, 4)
#undef _SPIRV_OP

class SPIRVImageInstBase:public SPIRVInstTemplateBase {
public:
  CapVec getRequiriedCapability() const {
     return getVec(SPIRVCapabilityKind::CapabilityImageBasic);
  }
};

#define _SPIRV_OP(x, ...) \
  typedef SPIRVInstTemplate<SPIRVImageInstBase, Op##x, __VA_ARGS__> \
      SPIRV##x;
// Image instructions
_SPIRV_OP(SampledImage, true, 5)
_SPIRV_OP(ImageSampleImplicitLod, true, 5, true)
_SPIRV_OP(ImageSampleExplicitLod, true, 7, true, 2)
_SPIRV_OP(ImageRead, true, 5, true, 2)
_SPIRV_OP(ImageWrite, false, 4, true, 3)
_SPIRV_OP(ImageQueryFormat, true, 4)
_SPIRV_OP(ImageQueryOrder, true, 4)
_SPIRV_OP(ImageQuerySizeLod, true, 5)
_SPIRV_OP(ImageQuerySize, true, 4)
_SPIRV_OP(ImageQueryLod, true, 5)
_SPIRV_OP(ImageQueryLevels, true, 4)
_SPIRV_OP(ImageQuerySamples, true, 4)
#undef _SPIRV_OP

#define _SPIRV_OP(x, ...) \
  typedef SPIRVInstTemplate<SPIRVInstTemplateBase, Op##x, __VA_ARGS__> \
      SPIRV##x;
// Other instructions
_SPIRV_OP(SpecConstantOp, true, 4, true)
_SPIRV_OP(GenericPtrMemSemantics, true, 4, false)
_SPIRV_OP(GenericCastToPtrExplicit, true, 5, false, 1)
_SPIRV_OP(SizeOf, true, 4, false)
_SPIRV_OP(CopyObject, true, 4, false)
#undef _SPIRV_OP

// Uncommon instructions
#define _SPIRV_OP(x, ...) \
  typedef SPIRVInstTemplate<SPIRVInstTemplateBase, Op##x, __VA_ARGS__> \
      SPIRV##x;
_SPIRV_OP(Unreachable, false, 1)
_SPIRV_OP(LifetimeStart, false, 3)
_SPIRV_OP(LifetimeStop, false, 3)
_SPIRV_OP(SelectionMerge, false, 3)
_SPIRV_OP(VectorTimesScalar, true, 5)
_SPIRV_OP(ReadClockKHR, true, 4)
#undef _SPIRV_OP

class SPIRVBallotInstBase : public SPIRVInstTemplateBase
{
public:
    CapVec getRequiriedCapability() const
    {
        return getVec(SPIRVCapabilityKind::CapabilitySubgroupBallotKHR);
    }
};

#define _SPIRV_OP(x, ...) \
  typedef SPIRVInstTemplate<SPIRVBallotInstBase, Op##x, __VA_ARGS__> \
      SPIRV##x;
_SPIRV_OP(SubgroupBallotKHR, true /*HasId*/, 4 /*WC*/, false /*VariWC*/)
_SPIRV_OP(SubgroupFirstInvocationKHR, true /*HasId*/, 4 /*WC*/, false /*VariWC*/)
#undef _SPIRV_OP

class SPIRVAssumeTrueKHR : public SPIRVInstruction {
public:
  static const Op OC = OpAssumeTrueKHR;
  static const SPIRVWord FixedWordCount = 2;

  SPIRVAssumeTrueKHR(SPIRVId TheCondition, SPIRVBasicBlock* BB)
    : SPIRVInstruction(FixedWordCount, OC, BB), ConditionId(TheCondition) {
    validate();
    setHasNoId();
    setHasNoType();
    IGC_ASSERT_MESSAGE(BB, "Invalid BB");
  }

  SPIRVAssumeTrueKHR() : SPIRVInstruction(OC), ConditionId(SPIRVID_MAX) {
    setHasNoId();
    setHasNoType();
  }

  CapVec getRequiredCapability() const override {
    return getVec(CapabilityExpectAssumeKHR);
  }

  SPIRVValue* getCondition() const { return getValue(ConditionId); }

  void setWordCount(SPIRVWord TheWordCount) override {
    SPIRVEntry::setWordCount(TheWordCount);
  }
  _SPIRV_DEF_DEC1_OVERRIDE(ConditionId)

protected:
    SPIRVId ConditionId;
};

class SPIRVExpectKHRInstBase : public SPIRVInstTemplateBase {
protected:
  CapVec getRequiredCapability() const override {
    return getVec(CapabilityExpectAssumeKHR);
  }
};

#define _SPIRV_OP(x, ...)                                                      \
  typedef SPIRVInstTemplate<SPIRVExpectKHRInstBase, Op##x, __VA_ARGS__>      \
      SPIRV##x;
_SPIRV_OP(ExpectKHR, true, 5)
#undef _SPIRV_OP

class SPIRVDotKHRBase : public SPIRVInstTemplateBase {
protected:
  CapVec getRequiredCapability() const override {
    // Both vector operands must have the same type, so analyzing the
    // first operand will suffice.
    return getVec(CapabilityDotProductKHR);
  }

  void validate() const override {
    SPIRVInstruction::validate();
    SPIRVId Vec1 = Ops[0];
    SPIRVId Vec2 = Ops[1];
    (void)Vec1;
    (void)Vec2;
    IGC_ASSERT_MESSAGE(getValueType(Vec1) == getValueType(Vec2), "Input vectors must have the same type");
    IGC_ASSERT_MESSAGE(getType()->isTypeInt(), "Result type must be an integer type");
    IGC_ASSERT_MESSAGE(!getType()->isTypeVector(), "Result type must be scalar");
  }

private:
  bool isAccSat() const {
    return (OpCode == OpSDotAccSatKHR || OpCode == OpUDotAccSatKHR || OpCode == OpSUDotAccSatKHR);
  }

  PackedVectorFormat getPackedVectorFormat() const {
    size_t PackFmtIdx = 2;
    if (isAccSat()) {
      // AccSat instructions have an additional Accumulator operand.
      PackFmtIdx++;
    }
    if (PackFmtIdx == Ops.size() - 1)
      return static_cast<PackedVectorFormat>(Ops[PackFmtIdx]);
    return static_cast<PackedVectorFormat>(Ops[PackFmtIdx]);
  }

  SPIRVCapabilityKind getRequiredCapabilityForOperand(SPIRVId ArgId) const {
    const SPIRVType* T = getValueType(ArgId);
    if (auto PackFmt = getPackedVectorFormat()) {
      switch (PackFmt) {
      case PackedVectorFormatPackedVectorFormat4x8BitKHR:
        IGC_ASSERT_MESSAGE(!T->isTypeVector() && T->isTypeInt() && T->getBitWidth() == 32,
          "Type does not match pack format");
        return CapabilityDotProductInput4x8BitPackedKHR;
      case PackedVectorFormatMax:
        break;
      }
      llvm_unreachable("Unknown Packed Vector Format");
    }
    if (T->isTypeVector()) {
      const SPIRVType* EltT = T->getVectorComponentType();
      if (T->getVectorComponentCount() == 4 && EltT->isTypeInt() && EltT->getBitWidth() == 8)
        return CapabilityDotProductInput4x8BitKHR;
      if (EltT->isTypeInt())
        return CapabilityDotProductInputAllKHR;
    }
    llvm_unreachable("No mapping for argument type to capability.");
  }
};

#define _SPIRV_OP(x, ...)                                                      \
  typedef SPIRVInstTemplate<SPIRVDotKHRBase, Op##x, __VA_ARGS__> SPIRV##x;
_SPIRV_OP(SDotKHR, true, 5, true, 2)
_SPIRV_OP(UDotKHR, true, 5, true, 2)
_SPIRV_OP(SUDotKHR, true, 5, true, 2)
_SPIRV_OP(SDotAccSatKHR, true, 6, true, 3)
_SPIRV_OP(UDotAccSatKHR, true, 6, true, 3)
_SPIRV_OP(SUDotAccSatKHR, true, 6, true, 3)
#undef _SPIRV_OP


class SPIRVSubgroupShuffleINTELInstBase : public SPIRVInstTemplateBase {
protected:
  CapVec getRequiredCapability() const override {
    return getVec(CapabilitySubgroupShuffleINTEL);
  }
};

#define _SPIRV_OP(x, ...) \
  typedef SPIRVInstTemplate<SPIRVSubgroupShuffleINTELInstBase, Op##x, __VA_ARGS__> \
      SPIRV##x;
// Intel Subgroup Shuffle Instructions
_SPIRV_OP(SubgroupShuffleINTEL, true, 5)
_SPIRV_OP(SubgroupShuffleDownINTEL, true, 6)
_SPIRV_OP(SubgroupShuffleUpINTEL, true, 6)
_SPIRV_OP(SubgroupShuffleXorINTEL, true, 5)
#undef _SPIRV_OP

class SPIRVSubgroupBufferBlockIOINTELInstBase : public SPIRVInstTemplateBase {
protected:
  CapVec getRequiredCapability() const override {
    return getVec(CapabilitySubgroupBufferBlockIOINTEL);
  }
};

#define _SPIRV_OP(x, ...) \
  typedef SPIRVInstTemplate<SPIRVSubgroupBufferBlockIOINTELInstBase, Op##x, __VA_ARGS__> \
      SPIRV##x;
// Intel Subgroup Buffer Block Read and Write Instructions
_SPIRV_OP(SubgroupBlockReadINTEL, true, 4)
_SPIRV_OP(SubgroupBlockWriteINTEL, false, 3)
#undef _SPIRV_OP

class SPIRVSubgroupImageBlockIOINTELInstBase : public SPIRVInstTemplateBase {
protected:
  CapVec getRequiredCapability() const override {
    return getVec(CapabilitySubgroupImageBlockIOINTEL);
  }
};

#define _SPIRV_OP(x, ...)                                                      \
  typedef SPIRVInstTemplate<SPIRVSubgroupImageBlockIOINTELInstBase,            \
                            Op##x, __VA_ARGS__>                                \
      SPIRV##x;
// Intel Subgroup Image Block Read and Write Instructions
_SPIRV_OP(SubgroupImageBlockReadINTEL, true, 5)
_SPIRV_OP(SubgroupImageBlockWriteINTEL, false, 4)
#undef _SPIRV_OP

class SPIRVSubgroupImageMediaBlockIOINTELInstBase:public SPIRVInstTemplateBase {
protected:
  CapVec getRequiredCapability() const override {
      return getVec(CapabilitySubgroupImageMediaBlockIOINTEL);
  }
};

#define _SPIRV_OP(x, ...)                                                     \
  typedef SPIRVInstTemplate<SPIRVSubgroupImageMediaBlockIOINTELInstBase,      \
                            Op##x, __VA_ARGS__>                               \
      SPIRV##x;
// Intel Subgroup Image Media Block Read and Write Instructions
_SPIRV_OP(SubgroupImageMediaBlockReadINTEL, true, 7)
_SPIRV_OP(SubgroupImageMediaBlockWriteINTEL, false, 6)
#undef _SPIRV_OP

class SPIRVSubgroupAVCIntelInstBase : public SPIRVInstTemplateBase {
protected:
    CapVec getRequiredCapability() const override {
        return getVec(CapabilitySubgroupAvcMotionEstimationINTEL);
    }
};

// Intel Subgroup AVC Motion Estimation Instructions
typedef SPIRVInstTemplate<
    SPIRVSubgroupAVCIntelInstBase, OpVmeImageINTEL, true, 5> SPIRVVmeImageINTEL;

#define _SPIRV_OP(x, ...) \
  typedef SPIRVInstTemplate<SPIRVSubgroupAVCIntelInstBase, \
    OpSubgroupAvc##x##INTEL, __VA_ARGS__> SPIRVSubgroupAvc##x##INTEL;
_SPIRV_OP(MceGetDefaultInterBaseMultiReferencePenalty, true, 5)
_SPIRV_OP(MceSetInterBaseMultiReferencePenalty, true, 5)
_SPIRV_OP(MceGetDefaultInterShapePenalty, true, 5)
_SPIRV_OP(MceSetInterShapePenalty, true, 5)
_SPIRV_OP(MceGetDefaultInterDirectionPenalty, true, 5)
_SPIRV_OP(MceSetInterDirectionPenalty, true, 5)
_SPIRV_OP(MceGetDefaultInterMotionVectorCostTable, true, 5)
_SPIRV_OP(MceGetDefaultHighPenaltyCostTable, true, 3)
_SPIRV_OP(MceGetDefaultMediumPenaltyCostTable, true, 3)
_SPIRV_OP(MceGetDefaultLowPenaltyCostTable, true, 3)
_SPIRV_OP(MceSetMotionVectorCostFunction, true, 7)
_SPIRV_OP(MceSetAcOnlyHaar, true, 4)
_SPIRV_OP(MceSetSourceInterlacedFieldPolarity, true, 5)
_SPIRV_OP(MceSetSingleReferenceInterlacedFieldPolarity, true, 5)
_SPIRV_OP(MceSetDualReferenceInterlacedFieldPolarities, true, 6)
_SPIRV_OP(MceConvertToImePayload, true, 4)
_SPIRV_OP(MceConvertToImeResult, true, 4)
_SPIRV_OP(MceConvertToRefPayload, true, 4)
_SPIRV_OP(MceConvertToRefResult, true, 4)
_SPIRV_OP(MceConvertToSicPayload, true, 4)
_SPIRV_OP(MceConvertToSicResult, true, 4)
_SPIRV_OP(MceGetMotionVectors, true, 4)
_SPIRV_OP(MceGetInterDistortions, true, 4)
_SPIRV_OP(MceGetBestInterDistortions, true, 4)
_SPIRV_OP(MceGetInterMajorShape, true, 4)
_SPIRV_OP(MceGetInterMinorShape, true, 4)
_SPIRV_OP(MceGetInterDirections, true, 4)
_SPIRV_OP(MceGetInterMotionVectorCount, true, 4)
_SPIRV_OP(MceGetInterReferenceIds, true, 4)
_SPIRV_OP(MceGetInterReferenceInterlacedFieldPolarities, true, 6)
_SPIRV_OP(ImeInitialize, true, 6)
_SPIRV_OP(ImeSetSingleReference, true, 6)
_SPIRV_OP(ImeSetDualReference, true, 7)
_SPIRV_OP(ImeRefWindowSize, true, 5)
_SPIRV_OP(ImeAdjustRefOffset, true, 7)
_SPIRV_OP(ImeConvertToMcePayload, true, 4)
_SPIRV_OP(ImeSetMaxMotionVectorCount, true, 5)
_SPIRV_OP(ImeSetUnidirectionalMixDisable, true, 4)
_SPIRV_OP(ImeSetEarlySearchTerminationThreshold, true, 5)
_SPIRV_OP(ImeSetWeightedSad, true, 5)
_SPIRV_OP(ImeEvaluateWithSingleReference, true, 6)
_SPIRV_OP(ImeEvaluateWithDualReference, true, 7)
_SPIRV_OP(ImeEvaluateWithSingleReferenceStreamin, true, 7)
_SPIRV_OP(ImeEvaluateWithDualReferenceStreamin, true, 8)
_SPIRV_OP(ImeEvaluateWithSingleReferenceStreamout, true, 6)
_SPIRV_OP(ImeEvaluateWithDualReferenceStreamout, true, 7)
_SPIRV_OP(ImeEvaluateWithSingleReferenceStreaminout, true, 7)
_SPIRV_OP(ImeEvaluateWithDualReferenceStreaminout, true, 8)
_SPIRV_OP(ImeConvertToMceResult, true, 4)
_SPIRV_OP(ImeGetSingleReferenceStreamin, true, 4)
_SPIRV_OP(ImeGetDualReferenceStreamin, true, 4)
_SPIRV_OP(ImeStripSingleReferenceStreamout, true, 4)
_SPIRV_OP(ImeStripDualReferenceStreamout, true, 4)
_SPIRV_OP(ImeGetStreamoutSingleReferenceMajorShapeMotionVectors, true, 5)
_SPIRV_OP(ImeGetStreamoutSingleReferenceMajorShapeDistortions, true, 5)
_SPIRV_OP(ImeGetStreamoutSingleReferenceMajorShapeReferenceIds, true, 5)
_SPIRV_OP(ImeGetStreamoutDualReferenceMajorShapeMotionVectors, true, 6)
_SPIRV_OP(ImeGetStreamoutDualReferenceMajorShapeDistortions, true, 6)
_SPIRV_OP(ImeGetStreamoutDualReferenceMajorShapeReferenceIds, true, 6)
_SPIRV_OP(ImeGetBorderReached, true, 5)
_SPIRV_OP(ImeGetTruncatedSearchIndication, true, 4)
_SPIRV_OP(ImeGetUnidirectionalEarlySearchTermination, true, 4)
_SPIRV_OP(ImeGetWeightingPatternMinimumMotionVector, true, 4)
_SPIRV_OP(ImeGetWeightingPatternMinimumDistortion, true, 4)
_SPIRV_OP(FmeInitialize, true, 10)
_SPIRV_OP(BmeInitialize, true, 11)
_SPIRV_OP(RefConvertToMcePayload, true, 4)
_SPIRV_OP(RefSetBidirectionalMixDisable, true, 4)
_SPIRV_OP(RefSetBilinearFilterEnable, true, 4)
_SPIRV_OP(RefEvaluateWithSingleReference, true, 6)
_SPIRV_OP(RefEvaluateWithDualReference, true, 7)
_SPIRV_OP(RefEvaluateWithMultiReference, true, 6)
_SPIRV_OP(RefEvaluateWithMultiReferenceInterlaced, true, 7)
_SPIRV_OP(RefConvertToMceResult, true, 4)
_SPIRV_OP(SicInitialize, true, 4)
_SPIRV_OP(SicConfigureSkc, true, 9)
_SPIRV_OP(SicGetMotionVectorMask, true, 5)
_SPIRV_OP(SicConvertToMcePayload, true, 4)
_SPIRV_OP(SicSetIntraLumaShapePenalty, true, 5)
_SPIRV_OP(SicSetBilinearFilterEnable, true, 4)
_SPIRV_OP(SicSetSkcForwardTransformEnable, true, 5)
_SPIRV_OP(SicSetBlockBasedRawSkipSad, true, 5)
_SPIRV_OP(SicEvaluateWithSingleReference, true, 6)
_SPIRV_OP(SicEvaluateWithDualReference, true, 7)
_SPIRV_OP(SicEvaluateWithMultiReference, true, 6)
_SPIRV_OP(SicEvaluateWithMultiReferenceInterlaced, true, 7)
_SPIRV_OP(SicConvertToMceResult, true, 4)
_SPIRV_OP(SicGetInterRawSads, true, 4)
#undef _SPIRV_OP

class SPIRVSubgroupAVCIntelInstBaseIntra : public SPIRVInstTemplateBase {
protected:
    CapVec getRequiredCapability() const override {
        return getVec(CapabilitySubgroupAvcMotionEstimationIntraINTEL);
    }
};

// Intel Subgroup AVC Motion Estimation Intra Instructions
#define _SPIRV_OP(x, ...) \
  typedef SPIRVInstTemplate<SPIRVSubgroupAVCIntelInstBaseIntra, \
    OpSubgroupAvc##x##INTEL, __VA_ARGS__> SPIRVSubgroupAvc##x##INTEL;
_SPIRV_OP(MceGetDefaultIntraLumaShapePenalty, true, 5)
_SPIRV_OP(MceGetDefaultIntraLumaModePenalty, true, 5)
_SPIRV_OP(MceGetDefaultNonDcLumaIntraPenalty, true, 3)
_SPIRV_OP(SicConfigureIpeLuma, true, 11)
_SPIRV_OP(SicSetIntraLumaModeCostFunction, true, 7)
_SPIRV_OP(SicEvaluateIpe, true, 5)
_SPIRV_OP(SicGetIpeLumaShape, true, 4)
_SPIRV_OP(SicGetBestIpeLumaDistortion, true, 4)
_SPIRV_OP(SicGetPackedIpeLumaModes, true, 4)
_SPIRV_OP(SicGetPackedSkcLumaCountThreshold, true, 4)
_SPIRV_OP(SicGetPackedSkcLumaSumThreshold, true, 4)
#undef _SPIRV_OP

class SPIRVSubgroupAVCIntelInstBaseChroma : public SPIRVInstTemplateBase {
protected:
    CapVec getRequiredCapability() const override {
        return getVec(CapabilitySubgroupAvcMotionEstimationChromaINTEL);
    }
};

// Intel Subgroup AVC Motion Estimation Chroma Instructions
#define _SPIRV_OP(x, ...) \
  typedef SPIRVInstTemplate<SPIRVSubgroupAVCIntelInstBaseChroma, \
    OpSubgroupAvc##x##INTEL, __VA_ARGS__> SPIRVSubgroupAvc##x##INTEL;
_SPIRV_OP(MceGetDefaultIntraChromaModeBasePenalty, true, 3)
_SPIRV_OP(SicConfigureIpeLumaChroma, true, 14)
_SPIRV_OP(SicSetIntraChromaModeCostFunction, true, 5)
_SPIRV_OP(SicGetBestIpeChromaDistortion, true, 4)
_SPIRV_OP(SicGetIpeChromaMode, true, 4)
#undef _SPIRV_OP


SPIRVSpecConstantOp *createSpecConstantOpInst(SPIRVInstruction *Inst);
SPIRVInstruction *createInstFromSpecConstantOp(SPIRVSpecConstantOp *C);

class SPIRVVariableLengthArrayINTELInstBase : public SPIRVInstTemplateBase {
protected:
    CapVec getRequiredCapability() const override {
        return getVec(CapabilityVariableLengthArrayINTEL);
    }
};
#define _SPIRV_OP(x, ...)                                                      \
  typedef SPIRVInstTemplate<SPIRVVariableLengthArrayINTELInstBase,             \
    Op##x##INTEL, __VA_ARGS__> SPIRV##x##INTEL;
_SPIRV_OP(VariableLengthArray, true, 4)
_SPIRV_OP(SaveMemory, true, 3)
_SPIRV_OP(RestoreMemory, false, 2)
#undef _SPIRV_OP
class SPIRVJointMatrixINTELInst: public SPIRVInstTemplateBase {
  CapVec getRequiredCapability() const override {
    return getVec(CapabilityJointMatrixINTEL);
  }
};

#define _SPIRV_OP(x, ...)                                                      \
  typedef SPIRVInstTemplate<SPIRVJointMatrixINTELInst,                         \
                            Op##x##INTEL, __VA_ARGS__>                         \
      SPIRV##x##INTEL;

_SPIRV_OP(JointMatrixLoad, true, 6, true)
_SPIRV_OP(JointMatrixStore, false, 5, true)
_SPIRV_OP(JointMatrixMad, true, 7)
_SPIRV_OP(JointMatrixSUMad, true, 7)
_SPIRV_OP(JointMatrixUSMad, true, 7)
_SPIRV_OP(JointMatrixUUMad, true, 7)
_SPIRV_OP(JointMatrixWorkItemLength, true, 4)
#undef _SPIRV_OP
}
#endif // SPIRVINSTRUCTION_HPP_
