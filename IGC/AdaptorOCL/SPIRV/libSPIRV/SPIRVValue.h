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

// This file defines the values defined in SPIR-V spec with op codes.
// The name of the SPIR-V values follow the op code name in the spec.
// This is for readability and ease of using macro to handle types.

#ifndef SPIRVVALUE_HPP_
#define SPIRVVALUE_HPP_

#include "SPIRVEntry.h"
#include "SPIRVType.h"
#include "SPIRVDecorate.h"
#include "Probe/Assertion.h"

#include <iostream>
#include <map>
#include <memory>

namespace igc_spv{

class SPIRVValue: public SPIRVEntry {
public:
  // Complete constructor for value with id and type
  SPIRVValue(SPIRVModule *M, unsigned TheWordCount, Op TheOpCode,
      SPIRVType *TheType, SPIRVId TheId)
    :SPIRVEntry(M, TheWordCount, TheOpCode, TheId), Type(TheType) {
    validate();
  }
  // Complete constructor for value with type but without id
  SPIRVValue(SPIRVModule *M, unsigned TheWordCount, Op TheOpCode,
      SPIRVType *TheType)
    :SPIRVEntry(M, TheWordCount, TheOpCode), Type(TheType) {
    setHasNoId();
    validate();
  }
  // Complete constructor for value with id but without type
  SPIRVValue(SPIRVModule *M, unsigned TheWordCount, Op TheOpCode,
      SPIRVId TheId)
    :SPIRVEntry(M, TheWordCount, TheOpCode, TheId), Type(NULL) {
    setHasNoType();
    validate();
  }
  // Complete constructor for value without id and type
  SPIRVValue(SPIRVModule *M, unsigned TheWordCount, Op TheOpCode)
    :SPIRVEntry(M, TheWordCount, TheOpCode), Type(NULL) {
    setHasNoId();
    setHasNoType();
    validate();
  }
  // Incomplete constructor
  SPIRVValue(Op TheOpCode):SPIRVEntry(TheOpCode), Type(NULL) {}

  bool hasType()const { return !(Attrib & SPIRVEA_NOTYPE);}
  SPIRVType *getType()const {
    IGC_ASSERT_MESSAGE(hasType(), "value has no type");
    return Type;
  }
  bool isVolatile()const;
  bool hasAlignment(SPIRVWord *Result=0)const;
  bool hasNoSignedWrap() const;
  bool hasNoUnsignedWrap() const;

  void setAlignment(SPIRVWord);
  void setVolatile(bool IsVolatile);
  void setNoSignedWrap(bool HasNoSignedWrap);
  void setNoUnsignedWrap(bool HasNoUnsignedWrap);

  void validate()const {
    SPIRVEntry::validate();
    IGC_ASSERT_MESSAGE((!hasType() || Type), "Invalid type");
  }

  void setType(SPIRVType *Ty) {
    Type = Ty;
    IGC_ASSERT(!Ty || !Ty->isTypeVoid() || OpCode == OpFunction);
    if (Ty && (!Ty->isTypeVoid() || OpCode == OpFunction))
      setHasType();
    else
      setHasNoType();
  }

  CapVec getRequiredCapability() const {
    CapVec CV;
    if (!hasType())
      return CV;
    if (Type->isTypeFloat(16))
       CV.push_back(SPIRVCapabilityKind::CapabilityFloat16);
    else if (Type->isTypeFloat(64))
       CV.push_back(SPIRVCapabilityKind::CapabilityFloat64);
    else if (Type->isTypeInt(16))
       CV.push_back(SPIRVCapabilityKind::CapabilityInt16);
    else if (Type->isTypeInt(64))
       CV.push_back(SPIRVCapabilityKind::CapabilityInt64);
    return CV;
  }

protected:
  void setHasNoType() { Attrib |= SPIRVEA_NOTYPE;}
  void setHasType() { Attrib &= ~SPIRVEA_NOTYPE;}

  SPIRVType *Type;                 // Value Type
};

template<Op OC>
class SPIRVConstantBase: public SPIRVValue {
public:
  // Complete constructor for integer constant
  SPIRVConstantBase(SPIRVModule *M, SPIRVType *TheType, SPIRVId TheId,
      uint64_t TheValue)
    :SPIRVValue(M, 0, OC, TheType, TheId){
    Union.UInt64Val = TheValue;
    recalculateWordCount();
    validate();
  }
  // Complete constructor for float constant
  SPIRVConstantBase(SPIRVModule *M, SPIRVType *TheType, SPIRVId TheId, float TheValue)
    :SPIRVValue(M, 0, OC, TheType, TheId){
    Union.FloatVal = TheValue;
    recalculateWordCount();
    validate();
  }
  // Complete constructor for double constant
  SPIRVConstantBase(SPIRVModule *M, SPIRVType *TheType, SPIRVId TheId, double TheValue)
    :SPIRVValue(M, 0, OC, TheType, TheId){
    Union.DoubleVal = TheValue;
    recalculateWordCount();
    validate();
  }
  // Incomplete constructor
  SPIRVConstantBase():SPIRVValue(OC), NumWords(0){}
  uint64_t getZExtIntValue() const { return Union.UInt64Val;}
  float getFloatValue() const { return Union.FloatVal;}
  double getDoubleValue() const { return Union.DoubleVal;}
  bool isConstant() { return true; }
protected:
  void recalculateWordCount() {
    NumWords = (Type->getBitWidth() + 31) / 32;
    WordCount = 3 + NumWords;
  }
  void validate() const {
    SPIRVValue::validate();
    IGC_ASSERT_EXIT_MESSAGE(1 <= NumWords, "Invalid constant size");
    IGC_ASSERT_EXIT_MESSAGE(NumWords <= 32, "Invalid constant size");
  }
  void setWordCount(SPIRVWord WordCount) {
    SPIRVValue::setWordCount(WordCount);
    NumWords = WordCount - 3;
  }
  void decode(std::istream &I) {
    getDecoder(I) >> Type >> Id;
    validate();
    for (unsigned i = 0; i < NumWords; ++i)
      getDecoder(I) >> Union.Words[i];
  }

  unsigned NumWords;
  union UnionType{
    uint64_t UInt64Val;
    float FloatVal;
    double DoubleVal;
    SPIRVWord Words[32];
    UnionType() {
      UInt64Val = 0;
    }
  } Union;
};

typedef SPIRVConstantBase<OpConstant> SPIRVConstant;
typedef SPIRVConstantBase<OpSpecConstant> SPIRVSpecConstant;

template<Op OC>
class SPIRVConstantEmpty: public SPIRVValue {
public:
  // Complete constructor
  SPIRVConstantEmpty(SPIRVModule *M, SPIRVType *TheType, SPIRVId TheId)
    :SPIRVValue(M, 3, OC, TheType, TheId){
    validate();
  }
  // Incomplete constructor
  SPIRVConstantEmpty():SPIRVValue(OC){}
protected:
  void validate() const override {
    SPIRVValue::validate();
  }
  _SPIRV_DEF_DEC2(Type, Id)
};

template<Op OC>
class SPIRVConstantBool: public SPIRVConstantEmpty<OC> {
public:
  // Complete constructor
  SPIRVConstantBool(SPIRVModule *M, SPIRVType *TheType, SPIRVId TheId)
    :SPIRVConstantEmpty<OC>(M, TheType, TheId){}
  // Incomplete constructor
  SPIRVConstantBool(){}
protected:
  void validate() const {
    SPIRVConstantEmpty<OC>::validate();
    IGC_ASSERT_MESSAGE(this->Type->isTypeBool(), "Invalid type");
  }
};

typedef SPIRVConstantBool<OpConstantTrue> SPIRVConstantTrue;
typedef SPIRVConstantBool<OpConstantFalse> SPIRVConstantFalse;

typedef SPIRVConstantBool<OpSpecConstantTrue> SPIRVSpecConstantTrue;
typedef SPIRVConstantBool<OpSpecConstantFalse> SPIRVSpecConstantFalse;

class SPIRVConstantNull : public SPIRVConstantEmpty<OpConstantNull>
{
public:
    // Complete constructor
    SPIRVConstantNull(SPIRVModule *M, SPIRVType *TheType, SPIRVId TheId)
        : SPIRVConstantEmpty(M, TheType, TheId) {
        validate();
    }
    // Incomplete constructor
    SPIRVConstantNull() {}
protected:
    void validate() const override {
        SPIRVConstantEmpty::validate();
        IGC_ASSERT_MESSAGE((Type->isTypeInt() || Type->isTypeBool() || Type->isTypeFloat() || Type->isTypeComposite() || Type->isTypeOpaque() || Type->isTypeEvent() || Type->isTypePointer() || Type->isTypeReserveId() || Type->isTypeDeviceEvent() || (Type->isTypeSubgroupAvcINTEL())), "Invalid type");
    }
};

class SPIRVUndef : public SPIRVConstantEmpty<OpUndef>
{
public:
    // Incomplete constructor
    SPIRVUndef() {}
protected:
    void validate() const
    {
        SPIRVConstantEmpty::validate();
    }
};

template<Op OC>
class SPIRVConstantCompositeBase: public SPIRVValue {
public:
    // There are always 3 words in this instruction except constituents:
    // 1) WordCount + OpCode
    // 2) Result type
    // 3) Result Id
    constexpr static SPIRVWord FixedWC = 3;
    using ContinuedInstType = typename InstToContinued<OC>::Type;
  // Complete constructor for composite constant
  SPIRVConstantCompositeBase(SPIRVModule *M, SPIRVType *TheType, SPIRVId TheId,
      const std::vector<SPIRVValue *> TheElements)
    :SPIRVValue(M, TheElements.size()+ FixedWC, OC, TheType, TheId){
    Elements = getIds(TheElements);
    validate();
  }
  // Incomplete constructor
  SPIRVConstantCompositeBase():SPIRVValue(OC){}
  std::vector<SPIRVValue*> getElements()const {
    return getValues(Elements);
  }

  std::vector<ContinuedInstType> getContinuedInstructions() {
      return ContinuedInstructions;
  }

  void addContinuedInstruction(ContinuedInstType Inst) {
      ContinuedInstructions.push_back(Inst);
  }

protected:
  void validate() const override {
    SPIRVValue::validate();
    for (auto &I:Elements)
      getValue(I)->validate();
  }
  void setWordCount(SPIRVWord WordCount) override
  {
    Elements.resize(WordCount - FixedWC);
  }

  void decode(std::istream& I) override
  {
      SPIRVDecoder Decoder = getDecoder(I);
      Decoder >> Type >> Id >> Elements;

      for (SPIRVEntry* E : Decoder.getContinuedInstructions(ContinuedOpCode))
      {
          addContinuedInstruction(static_cast<ContinuedInstType>(E));
      }
  }

  std::vector<SPIRVId> Elements;
  std::vector<ContinuedInstType> ContinuedInstructions;
  const igc_spv::Op ContinuedOpCode = InstToContinued<OC>::OpCode;
};

typedef SPIRVConstantCompositeBase<OpConstantComposite> SPIRVConstantComposite;
typedef SPIRVConstantCompositeBase<OpSpecConstantComposite> SPIRVSpecConstantComposite;

class SPIRVConstantSampler: public SPIRVValue {
public:
  const static Op OC = OpConstantSampler;
  const static SPIRVWord WC = 6;
  // Complete constructor
  SPIRVConstantSampler(SPIRVModule *M, SPIRVType *TheType, SPIRVId TheId,
      SPIRVWord TheAddrMode, SPIRVWord TheNormalized, SPIRVWord TheFilterMode)
    :SPIRVValue(M, WC, OC, TheType, TheId), AddrMode(TheAddrMode),
     Normalized(TheNormalized), FilterMode(TheFilterMode){
    validate();
  }
  // Incomplete constructor
  SPIRVConstantSampler() :SPIRVValue(OC), AddrMode(SPIRVSamplerAddressingModeKind::SamplerAddressingModeNone),
     Normalized(SPIRVWORD_MAX), FilterMode(SPIRVSamplerFilterModeKind::SamplerFilterModeNearest){}

  SPIRVWord getAddrMode() const {
    return AddrMode;
  }

  SPIRVWord getFilterMode() const {
    return FilterMode;
  }

  SPIRVWord getNormalized() const {
    return Normalized;
  }
  CapVec getRequiredCapability() const {
     return getVec(SPIRVCapabilityKind::CapabilityLiteralSampler);
  }
protected:
  SPIRVWord AddrMode;
  SPIRVWord Normalized;
  SPIRVWord FilterMode;
  void validate() const {
    SPIRVValue::validate();
    IGC_ASSERT(OpCode == OC);
    IGC_ASSERT(WordCount == WC);
    IGC_ASSERT(Type->isTypeSampler());
  }
  _SPIRV_DEF_DEC5(Type, Id, AddrMode, Normalized, FilterMode)
};

class SPIRVForward:public SPIRVValue, public SPIRVComponentExecutionModes {
public:
  const static Op OC = OpForward;
  // Complete constructor
  SPIRVForward(SPIRVModule *TheModule, SPIRVType *TheTy, SPIRVId TheId):
    SPIRVValue(TheModule, 0, OC, TheId){
    if (TheTy)
      setType(TheTy);
  }
  SPIRVForward():SPIRVValue(OC) {
    IGC_ASSERT_EXIT_MESSAGE(0, "should never be called");
  }
  _SPIRV_DEF_DEC1(Id)
  friend class SPIRVFunction;
protected:
  void validate() const {}
};

class SPIRVConstantPipeStorage : public SPIRVValue
{
public:
    static const Op OC = OpConstantPipeStorage;
    SPIRVConstantPipeStorage() : SPIRVValue(OC) {}
    uint32_t GetPacketSize() { return PacketSize; }
    uint32_t GetPacketAlignment() { return PacketAlignment; }
    uint32_t GetCapacity() { return Capacity; }
private:
    _SPIRV_DEF_DEC5(Type, Id, PacketSize, PacketAlignment, Capacity)
    uint32_t PacketSize;
    uint32_t PacketAlignment;
    uint32_t Capacity;
};

}


#endif /* SPIRVVALUE_HPP_ */
