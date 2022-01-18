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

// This file defines the types defined in SPIRV spec with op codes.
// The name of the SPIR-V types follow the op code name in the spec, e.g.
// SPIR-V type with op code name OpTypeInt is named as SPIRVTypeInt. This is
// for readability and ease of using macro to handle types.

#ifndef SPIRVTYPE_HPP_
#define SPIRVTYPE_HPP_

#include "SPIRVEntry.h"
#include "SPIRVStream.h"

#include <tuple>
#include <vector>
#include <map>
#include <iostream>
#include "Probe/Assertion.h"

namespace igc_spv{

class SPIRVType: public SPIRVEntry {
public:
  // Complete constructor
  SPIRVType(SPIRVModule *M, unsigned TheWordCount, Op TheOpCode,
      SPIRVId TheId)
    :SPIRVEntry(M, TheWordCount, TheOpCode, TheId){}
  // Incomplete constructor
  SPIRVType(Op TheOpCode):SPIRVEntry(TheOpCode){}

  SPIRVType *getArrayElementType() const;
  uint64_t getArrayLength() const;
  unsigned getBitWidth() const;
  unsigned getFloatBitWidth() const;
  SPIRVType *getFunctionReturnType() const;
  unsigned getIntegerBitWidth() const;
  SPIRVType *getPointerElementType() const;
  SPIRVStorageClassKind getPointerStorageClass() const;
  SPIRVType *getStructMemberType(size_t) const;
  SPIRVWord getStructMemberCount() const;
  SPIRVWord getVectorComponentCount() const;
  SPIRVType *getVectorComponentType() const;

  bool isTypeVoid() const;
  bool isTypeArray() const;
  bool isTypeBool() const;
  bool isTypeComposite() const;
  bool isTypeEvent() const;
  bool isTypeDeviceEvent() const;
  bool isTypeReserveId() const;
  bool isTypeFloat(unsigned Bits = 0) const;
  bool isTypeImage() const;
  bool isTypeSampledImage() const;
  bool isTypeVmeImageINTEL() const;
  bool isTypeSubgroupAvcINTEL() const;
  bool isTypeOCLImage() const;
  bool isTypePipe()const;
  bool isTypeInt(unsigned Bits = 0) const;
  bool isTypeNamedBarrier() const;
  bool isTypeQueue() const;
  bool isTypeOpaque() const;
  bool isTypePointer() const;
  bool isTypeSampler() const;
  bool isTypeStruct() const;
  bool isTypeVector() const;
  bool isTypeVectorInt() const;
  bool isTypeVectorFloat() const;
  bool isTypeVectorBool() const;
  bool isTypeVectorOrScalarInt() const;
  bool isTypeVectorOrScalarFloat() const;
  bool isTypeVectorOrScalarBool() const;
};

class SPIRVTypeVoid:public SPIRVType {
public:
  // Complete constructor
  SPIRVTypeVoid(SPIRVModule *M, SPIRVId TheId)
    :SPIRVType(M, 2, OpTypeVoid, TheId){}
  // Incomplete constructor
  SPIRVTypeVoid():SPIRVType(OpTypeVoid){}
protected:
  _SPIRV_DEF_DEC1(Id)
};

class SPIRVTypeBool:public SPIRVType {
public:
  // Complete constructor
  SPIRVTypeBool(SPIRVModule *M, SPIRVId TheId)
    :SPIRVType(M, 2, OpTypeBool, TheId){}
  // Incomplete constructor
  SPIRVTypeBool():SPIRVType(OpTypeBool){}
protected:
  _SPIRV_DEF_DEC1(Id)
};

class SPIRVTypeInt:public SPIRVType {
public:
  static const Op OC = OpTypeInt;
  // Complete constructor
  SPIRVTypeInt(SPIRVModule *M, SPIRVId TheId, unsigned TheBitWidth,
      bool ItIsSigned)
    :SPIRVType(M, 4, OC , TheId), BitWidth(TheBitWidth),
     IsSigned(ItIsSigned){
     validate();
     }
  // Incomplete constructor
  SPIRVTypeInt():SPIRVType(OC), BitWidth(0), IsSigned(false){}

  unsigned getBitWidth() const { return BitWidth;}
  bool isSigned() const { return IsSigned;}

protected:
  _SPIRV_DEF_DEC3(Id, BitWidth, IsSigned)
  void validate()const {
    SPIRVEntry::validate();
    IGC_ASSERT_EXIT_MESSAGE(1 < BitWidth, "Invalid bit width");
    IGC_ASSERT_EXIT_MESSAGE(BitWidth <= 1024, "Invalid bit width");
  }
private:
  unsigned BitWidth;    // Bit width
  bool IsSigned;        // Whether it is signed
};

class SPIRVTypeFloat:public SPIRVType {
public:
  static const Op OC = OpTypeFloat;
  // Complete constructor
  SPIRVTypeFloat(SPIRVModule *M, SPIRVId TheId, unsigned TheBitWidth)
    :SPIRVType(M, 3, OC, TheId), BitWidth(TheBitWidth){}
  // Incomplete constructor
  SPIRVTypeFloat():SPIRVType(OC), BitWidth(0){}

  unsigned getBitWidth() const { return BitWidth;}

protected:
  _SPIRV_DEF_DEC2(Id, BitWidth)
  void validate()const override {
    SPIRVEntry::validate();
    IGC_ASSERT_EXIT_MESSAGE(16 <= BitWidth, "Invalid bit width");
    IGC_ASSERT_EXIT_MESSAGE(BitWidth <= 64, "Invalid bit width");
  }
private:
  unsigned BitWidth;    // Bit width
};

class SPIRVTypePointer:public SPIRVType {
public:
  // Complete constructor
  SPIRVTypePointer(SPIRVModule *M, SPIRVId TheId,
      SPIRVStorageClassKind TheStorageClass,
      SPIRVType *ElementType)
    :SPIRVType(M, 4, OpTypePointer, TheId), ElemStorageClass(TheStorageClass),
     ElemTypeId(0){
    validate();
  }
  // Incomplete constructor
  SPIRVTypePointer():SPIRVType(OpTypePointer),
      ElemStorageClass(SPIRVStorageClassKind::StorageClassPrivateGlobal),
      ElemTypeId(0){}

  SPIRVType *getElementType() const {
    return static_cast<SPIRVType*>(getEntry(ElemTypeId));
  }
  SPIRVStorageClassKind getStorageClass() const { return ElemStorageClass;}
  CapVec getRequiredCapability() const {
    auto Cap = getVec(SPIRVCapabilityKind::CapabilityAddresses);
    if (getElementType()->isTypeFloat(16))
       Cap.push_back(SPIRVCapabilityKind::CapabilityFloat16Buffer);
    Cap.push_back(getCapability(ElemStorageClass));
    return Cap;
  }
protected:
  _SPIRV_DEF_DEC3(Id, ElemStorageClass, ElemTypeId)
  void validate()const {
    SPIRVEntry::validate();
    IGC_ASSERT(isValid(ElemStorageClass));
  }
private:
  SPIRVStorageClassKind ElemStorageClass;   // Storage Class
  SPIRVId ElemTypeId;                       // Element Type
};

class SPIRVTypeForwardPointer : public SPIRVEntryNoId<OpTypeForwardPointer> {
public:
  SPIRVTypeForwardPointer(SPIRVModule* M, SPIRVTypePointer* Pointer,
      SPIRVStorageClassKind SC)
      : SPIRVEntryNoId(M, 3), Pointer(Pointer), SC(SC) {}

  SPIRVTypeForwardPointer()
      : Pointer(nullptr), SC(StorageClassUniformConstant) {}

  _SPIRV_DCL_DEC
private:
  SPIRVTypePointer* Pointer;
  SPIRVStorageClassKind SC;
};

class SPIRVTypeVector:public SPIRVType {
public:
  // Complete constructor
  SPIRVTypeVector(SPIRVModule *M, SPIRVId TheId, SPIRVType *TheCompType,
      SPIRVWord TheCompCount)
    :SPIRVType(M, 4, OpTypeVector, TheId), CompType(TheCompType),
     CompCount(TheCompCount){
    validate();
  }
  // Incomplete constructor
  SPIRVTypeVector():SPIRVType(OpTypeVector), CompType(nullptr),
      CompCount(0){}

  SPIRVType *getComponentType() const { return CompType;}
  SPIRVWord getComponentCount() const { return CompCount;}
  bool isValidIndex(SPIRVWord Index) const { return Index < CompCount;}
  CapVec getRequiredCapability() const {
    if (CompCount > 8)
      return getVec(SPIRVCapabilityKind::CapabilityVector16);
    return CapVec();
  }

protected:
  _SPIRV_DEF_DEC3(Id, CompType, CompCount)
  void validate()const {
    SPIRVEntry::validate();
    CompType->validate();
#if 1
    // Allow any vector from 1 to 16 for testing.
    IGC_ASSERT(CompCount >= 1 && CompCount <= 16);
#else
    IGC_ASSERT(CompCount == 2 || CompCount == 3 || CompCount == 4 ||
        CompCount == 8 || CompCount == 16);
#endif
  }
private:
  SPIRVType *CompType;                // Component Type
  SPIRVWord CompCount;                // Component Count
};

class SPIRVTypeArray:public SPIRVType {
public:
  // Complete constructor
  SPIRVTypeArray(SPIRVModule *M, SPIRVId TheId, SPIRVType *TheElemType,
      SPIRVConstant* TheLength);
  // Incomplete constructor
  SPIRVTypeArray():SPIRVType(OpTypeArray), ElemType(nullptr),
      Length(SPIRVID_INVALID){}

  SPIRVType *getElementType() const { return ElemType;}
  SPIRVConstant *getLength() const;

protected:
  _SPIRV_DCL_DEC
  void validate()const;
private:
  SPIRVType *ElemType;                // Element Type
  SPIRVId Length;                     // Array Length
};

class SPIRVTypeOpaque:public SPIRVType {
public:
  // Complete constructor
  SPIRVTypeOpaque(SPIRVModule *M, SPIRVId TheId, const std::string& TheName)
    :SPIRVType(M, 2 + getSizeInWords(TheName), OpTypeOpaque, TheId) {
    Name = TheName;
    validate();
  }
  // Incomplete constructor
  SPIRVTypeOpaque():SPIRVType(OpTypeOpaque){}

protected:
  _SPIRV_DEF_DEC2(Id, Name)
  void validate()const override{
    SPIRVEntry::validate();
  }
};

struct SPIRVTypeImageDescriptor {
  SPIRVImageDimKind Dim;
  SPIRVWord Depth;
  SPIRVWord Arrayed;
  SPIRVWord MS;
  SPIRVWord Sampled;
  SPIRVWord Format;
  static std::tuple<std::tuple<SPIRVImageDimKind, SPIRVWord, SPIRVWord, SPIRVWord,
    SPIRVWord>, SPIRVWord>
    getAsTuple (const SPIRVTypeImageDescriptor &Desc) {
    return std::make_tuple(std::make_tuple(Desc.Dim, Desc.Depth, Desc.Arrayed,
      Desc.MS, Desc.Sampled), Desc.Format);
  }
  SPIRVTypeImageDescriptor():Dim(Dim1D), Depth(0), Arrayed(0),
      MS(0), Sampled(0), Format(0){}
  SPIRVTypeImageDescriptor(SPIRVImageDimKind Dim, SPIRVWord Cont, SPIRVWord Arr,
      SPIRVWord Comp,  SPIRVWord Mult, SPIRVWord F):Dim(Dim), Depth(Cont),
          Arrayed(Arr), MS(Comp), Sampled(Mult), Format(F){}
};

template<> inline void
SPIRVMap<std::string, SPIRVTypeImageDescriptor>::init() {
#define _SPIRV_OP(x,...) {SPIRVTypeImageDescriptor S(__VA_ARGS__); \
  add(#x, S);}
_SPIRV_OP(image1d_t,                  Dim1D,      0, 0, 0, 0, 0)
_SPIRV_OP(image1d_buffer_t,           DimBuffer,  0, 0, 0, 0, 0)
_SPIRV_OP(image1d_array_t,            Dim1D,      0, 1, 0, 0, 0)
_SPIRV_OP(image2d_t,                  Dim2D,      0, 0, 0, 0, 0)
_SPIRV_OP(image2d_array_t,            Dim2D,      0, 1, 0, 0, 0)
_SPIRV_OP(image2d_depth_t,            Dim2D,      1, 0, 0, 0, 0)
_SPIRV_OP(image2d_array_depth_t,      Dim2D,      1, 1, 0, 0, 0)
_SPIRV_OP(image2d_msaa_t,             Dim2D,      0, 0, 1, 0, 0)
_SPIRV_OP(image2d_array_msaa_t,       Dim2D,      0, 1, 1, 0, 0)
_SPIRV_OP(image2d_msaa_depth_t,       Dim2D,      1, 0, 1, 0, 0)
_SPIRV_OP(image2d_array_msaa_depth_t, Dim2D,      1, 1, 1, 0, 0)
_SPIRV_OP(image3d_t,                  Dim3D,      0, 0, 0, 0, 0)
#undef _SPIRV_OP
}
typedef SPIRVMap<std::string, SPIRVTypeImageDescriptor>
  OCLSPIRVImageTypeMap;

// Comparision function required to use the struct as map key.
inline bool
operator<(const SPIRVTypeImageDescriptor &A,
    const SPIRVTypeImageDescriptor &B){
  return SPIRVTypeImageDescriptor::getAsTuple(A) <
      SPIRVTypeImageDescriptor::getAsTuple(B);
}

class SPIRVTypeImage:public SPIRVType {
public:
  const static Op OC = OpTypeImage;
  constexpr static SPIRVWord FixedWC = 9;
  SPIRVTypeImage(SPIRVModule *M, SPIRVId TheId, SPIRVId TheSampledType,
      const SPIRVTypeImageDescriptor &TheDesc)
    :SPIRVType(M, FixedWC, OC, TheId), SampledType(TheSampledType),
     Desc(TheDesc){
    validate();
  }
  SPIRVTypeImage(SPIRVModule *M, SPIRVId TheId, SPIRVId TheSampledType,
      const SPIRVTypeImageDescriptor &TheDesc, SPIRVAccessQualifierKind TheAcc)
    :SPIRVType(M, FixedWC + 1, OC, TheId), SampledType(TheSampledType),
     Desc(TheDesc){
    Acc.push_back(TheAcc);
    validate();
  }
  SPIRVTypeImage():SPIRVType(OC), SampledType(SPIRVID_INVALID),
    Desc(DimCount, SPIRVWORD_MAX, SPIRVWORD_MAX, SPIRVWORD_MAX,
        SPIRVWORD_MAX, SPIRVWORD_MAX){
  }
  const SPIRVTypeImageDescriptor &getDescriptor()const {
    return Desc;
  }
  bool isOCLImage() const {
    return Desc.Sampled == 0 /* Only known at runtime */ &&
           Desc.Format  == ImageFormatUnknown;
  }
  bool hasAccessQualifier() const { return !Acc.empty();}
  SPIRVAccessQualifierKind getAccessQualifier() const {
    // Play it safe, if unspecified assume it is read write.
    return hasAccessQualifier() ? Acc[0] : AccessQualifierReadWrite;
  }
  CapVec getRequiredCapability() const {
    CapVec CV;
    CV.push_back(SPIRVCapabilityKind::CapabilityImageBasic);
    if (Acc.size() > 0 && Acc[0] == AccessQualifierReadWrite)
      CV.push_back(SPIRVCapabilityKind::CapabilityImageReadWrite);
    if (Desc.MS)
      CV.push_back(SPIRVCapabilityKind::CapabilityImageMipmap);
    return CV;
  }
  SPIRVType* getSampledType() const { return get<SPIRVType>(SampledType); }
protected:
  _SPIRV_DEF_DEC9(Id, SampledType, Desc.Dim, Desc.Depth,
      Desc.Arrayed, Desc.MS, Desc.Sampled, Desc.Format, Acc)
  // The validation assumes OpenCL image or sampler type.
  void validate()const {
    IGC_ASSERT(OpCode == OC);
    IGC_ASSERT(WordCount == FixedWC + Acc.size());
    IGC_ASSERT(Desc.Dim <= 5);
    IGC_ASSERT(Desc.Depth <= 1);
    IGC_ASSERT(Desc.Arrayed <= 1);
    IGC_ASSERT(Desc.MS <= 1);
    IGC_ASSERT(Desc.Sampled == 0); // For OCL only
    IGC_ASSERT(Desc.Format == ImageFormatUnknown);  // For OCL only
    IGC_ASSERT(Acc.size() <= 1);
  }
  void setWordCount(SPIRVWord TheWC) {
    WordCount = TheWC;
    Acc.resize(WordCount - FixedWC);
  }
private:
  SPIRVId SampledType;
  SPIRVTypeImageDescriptor Desc;
  std::vector<SPIRVAccessQualifierKind> Acc;
};

class SPIRVTypeSampler:public SPIRVType {
public:
  const static Op OC = OpTypeSampler;
  const static SPIRVWord FixedWC = 2;
  SPIRVTypeSampler(SPIRVModule *M, SPIRVId TheId)
    :SPIRVType(M, FixedWC, OC, TheId){
    validate();
  }
  SPIRVTypeSampler():SPIRVType(OC){
  }
protected:
  _SPIRV_DEF_DEC1(Id)
  void validate()const {
    IGC_ASSERT(OpCode == OC);
    IGC_ASSERT(WordCount == FixedWC);
  }
};

class SPIRVTypeSampledImage:public SPIRVType {
public:
  const static Op OC = OpTypeSampledImage;
  const static SPIRVWord FixedWC = 3;
  SPIRVTypeSampledImage(SPIRVModule *M, SPIRVId TheId, SPIRVTypeImage *TheImgTy)
    :SPIRVType(M, FixedWC, OC, TheId), ImgTy(TheImgTy){
    validate();
  }
  SPIRVTypeSampledImage():SPIRVType(OC), ImgTy(nullptr){
  }

  const SPIRVTypeImage *getImageType() const {
    return ImgTy;
  }

  void setImageType(SPIRVTypeImage *TheImgTy) {
    ImgTy = TheImgTy;
  }

protected:
  SPIRVTypeImage *ImgTy;
  _SPIRV_DEF_DEC2(Id, ImgTy)
  void validate()const override {
    IGC_ASSERT(OpCode == OC);
    IGC_ASSERT(WordCount == FixedWC);
    IGC_ASSERT(ImgTy && ImgTy->isTypeImage());
  }
};

class SPIRVTypeStruct:public SPIRVType {
public:
    const static Op OC = OpTypeStruct;
    // There are always 2 words in this instruction except member types:
    // 1) WordCount + OpCode
    // 2) Result Id
    constexpr static SPIRVWord FixedWC = 2;
    using ContinuedInstType = typename InstToContinued<OC>::Type;
  // Complete constructor
  SPIRVTypeStruct(SPIRVModule *M, SPIRVId TheId,
      const std::vector<SPIRVType *> &TheMemberTypes, const std::string &TheName)
    :SPIRVType(M, FixedWC + TheMemberTypes.size(), OC, TheId),
     MemberTypeVec(TheMemberTypes){
    Name = TheName;
    validate();
  }
  // Incomplete constructor
  SPIRVTypeStruct():SPIRVType(OC){}

  SPIRVWord getMemberCount() const { return MemberTypeVec.size();}
  SPIRVType *getMemberType(size_t I) const { return MemberTypeVec[I];}
  void setMemberType(size_t I, SPIRVType* pTy)
  {
      if (I >= MemberTypeVec.size() && !ContinuedInstructions.empty()) {
          const size_t MaxNumElements = MaxWordCount - FixedWC;
          I -= MaxNumElements; // Remove operands that included into OpTypeStruct
          ContinuedInstructions[I / MaxNumElements]->setElementId(
              I % MaxNumElements, pTy->getId());
      }
      else {
          MemberTypeVec[I] = pTy;
      }
  }
  bool isPacked() const;
  void setPacked(bool Packed);

  void addContinuedInstruction(ContinuedInstType Inst) {
      ContinuedInstructions.push_back(Inst);
  }

  std::vector<ContinuedInstType> getContinuedInstructions() {
      return ContinuedInstructions;
  }

protected:
  void setWordCount(SPIRVWord WordCount) { MemberTypeVec.resize(WordCount - FixedWC);}

  _SPIRV_DCL_DEC

  void validate()const {
    SPIRVEntry::validate();
  }
private:
  std::vector<SPIRVType *> MemberTypeVec;      // Member Types
  std::vector<ContinuedInstType> ContinuedInstructions;
  const igc_spv::Op ContinuedOpCode = InstToContinued<OC>::OpCode;
};

class SPIRVTypeFunction:public SPIRVType {
public:
  // Complete constructor
  SPIRVTypeFunction(SPIRVModule *M, SPIRVId TheId, SPIRVType *TheReturnType,
      const std::vector<SPIRVType *> &TheParameterTypes)
    :SPIRVType(M, 3 + TheParameterTypes.size(), OpTypeFunction, TheId),
     ReturnType(TheReturnType) {
     for (const SPIRVType *T : TheParameterTypes) {
       ParamTypeIdVec.push_back(T->getId());
     }
     validate();
  }
  // Incomplete constructor
  SPIRVTypeFunction():SPIRVType(OpTypeFunction), ReturnType(NULL){}

  SPIRVType *getReturnType() const { return ReturnType;}
  SPIRVWord getNumParameters() const { return ParamTypeIdVec.size();}
  SPIRVType *getParameterType(unsigned I) const {
    return static_cast<SPIRVType *>(getEntry(ParamTypeIdVec[I]));
  }

protected:
  _SPIRV_DEF_DEC3(Id, ReturnType, ParamTypeIdVec)
  void setWordCount(SPIRVWord WordCount) {
    SPIRVType::setWordCount(WordCount);
    ParamTypeIdVec.resize(WordCount - 3);
  }
  void validate()const {
    SPIRVEntry::validate();
    ReturnType->validate();
    for (auto I : ParamTypeIdVec)
      getEntry(I)->validate();
  }
private:
  SPIRVType *ReturnType;                      // Return Type
  std::vector<SPIRVId> ParamTypeIdVec;        // Parameter Type Ids
};

class SPIRVTypeOpaqueGeneric:public SPIRVType {
public:
  // Complete constructor
  SPIRVTypeOpaqueGeneric(Op TheOpCode, SPIRVModule *M, SPIRVId TheId)
    :SPIRVType(M, 2, TheOpCode, TheId){
    validate();
  }

  // Incomplete constructor
  SPIRVTypeOpaqueGeneric(Op TheOpCode):SPIRVType(TheOpCode),
      Opn(SPIRVID_INVALID) {}

  SPIRVValue *getOperand() {
    return getValue(Opn);
  }
protected:
  _SPIRV_DEF_DEC1(Id)
  void validate()const {
    SPIRVEntry::validate();
    }
  SPIRVId Opn;
};

template<Op TheOpCode>
class SPIRVOpaqueGenericType:public SPIRVTypeOpaqueGeneric {
public:
  // Complete constructor
  SPIRVOpaqueGenericType(SPIRVModule *M, SPIRVId TheId)
    :SPIRVTypeOpaqueGeneric(TheOpCode, M, TheId){}
  // Incomplete constructor
  SPIRVOpaqueGenericType():SPIRVTypeOpaqueGeneric(TheOpCode){}
};

#define _SPIRV_OP(x) typedef SPIRVOpaqueGenericType<OpType##x> SPIRVType##x;
_SPIRV_OP(Event)
_SPIRV_OP(DeviceEvent)
_SPIRV_OP(ReserveId)
_SPIRV_OP(Queue)
#undef _SPIRV_OP

class SPIRVTypePipe :public SPIRVType {
public:
  // Complete constructor
  SPIRVTypePipe(SPIRVModule *M, SPIRVId TheId,
     SPIRVAccessQualifierKind AccessQual = AccessQualifierReadOnly)
    :SPIRVType(M, 3, OpTypePipe, TheId),
     AccessQualifier(AccessQual){
       validate();
     }

  // Incomplete constructor
  SPIRVTypePipe() :SPIRVType(OpTypePipe),
     AccessQualifier(SPIRVAccessQualifierKind::AccessQualifierReadOnly){}

  SPIRVAccessQualifierKind getAccessQualifier() const {
      return AccessQualifier;
  }
  void setPipeAcessQualifier(SPIRVAccessQualifierKind AccessQual) {
    AccessQualifier = AccessQual;
    IGC_ASSERT(isValid(AccessQualifier));
  }
  CapVec getRequiredCapability() const override {
     return getVec(SPIRVCapabilityKind::CapabilityPipes);
  }
protected:
  _SPIRV_DEF_DEC2(Id, AccessQualifier)
  void validate()const override {
    SPIRVEntry::validate();
  }
private:
  SPIRVAccessQualifierKind AccessQualifier;     // Access Qualifier
};

class SPIRVTypePipeStorage : public SPIRVType
{
public:
    // Incomplete constructor
    SPIRVTypePipeStorage() : SPIRVType(OpTypePipeStorage) {}
    CapVec getRequiredCapability() const
    {
        return getVec(SPIRVCapabilityKind::CapabilityPipeStorage);
    }
private:
    _SPIRV_DEF_DEC1(Id)
};


class SPIRVTypeNamedBarrier :public SPIRVType {
public:
    // Incomplete constructor
    SPIRVTypeNamedBarrier() :SPIRVType(OpTypeNamedBarrier){}

    CapVec getRequiredCapability() const {
        return getVec(SPIRVCapabilityKind::CapabilityNamedBarrier);
    }
protected:
    _SPIRV_DEF_DEC1(Id)
        void validate()const {
            SPIRVEntry::validate();
        }
};

class SPIRVTypeJointMatrixINTEL : public SPIRVType {
public:
    const static Op OC = OpTypeJointMatrixINTEL;
    const static SPIRVWord FixedWC = 7;
    // Complete constructor
    SPIRVTypeJointMatrixINTEL(SPIRVModule *M, SPIRVId TheId, SPIRVType *ElemType,
                              SPIRVId Rows, SPIRVId Columns,
                              SPIRVId Layout, SPIRVId Scope)
        : SPIRVType(M, FixedWC, OC, TheId), ElemType(ElemType),
          Rows(Rows), Columns(Columns), Layout(Layout), Scope(Scope) {
        validate();
    }

    // Incomplete constructor
    SPIRVTypeJointMatrixINTEL()
        : SPIRVType(OC), ElemType(0), Rows(0), Columns(0),
          Layout(0), Scope(0) {
    }

    CapVec getRequiredCapability() const override {
        return getVec(SPIRVCapabilityKind::CapabilityJointMatrixINTEL);
    }

    SPIRVType *getElemType() const { return ElemType; }

    unsigned getLayout() const;
    unsigned getRows() const;
    unsigned getColumns() const;
    unsigned getScope() const;
    std::string getMangledName() const;

    enum {
        LayoutColumnMajor = 0,
        LayoutRowMajor = 1,
        LayoutPackedA = 2,
        LayoutPackedB = 3,
        LayoutMAX
    };

protected:
    _SPIRV_DEF_DEC6(Id, ElemType, Rows, Columns, Layout, Scope)
    void validate() const override {
        SPIRVEntry::validate();
        ElemType->validate();
        IGC_ASSERT_EXIT_MESSAGE(getRows()    <= 64, "Unsupported rows size.");
        IGC_ASSERT_EXIT_MESSAGE(getColumns() <= 64, "Unsupported columns size.");
        IGC_ASSERT_EXIT_MESSAGE(getLayout() < LayoutMAX, "Unsupported layout.");
    }

private:
    SPIRVType *ElemType;
    SPIRVId Rows;
    SPIRVId Columns;
    SPIRVId Layout;
    SPIRVId Scope;
};


template<typename T2, typename T1>
bool
isType(const T1 *Ty, unsigned Bits = 0) {
  bool Is = Ty->getOpCode() == T2::OC;
  if (Bits == 0)
    return Is;
  return static_cast<const T2*>(Ty)->getBitWidth() == Bits;
}

// SPV_INTEL_device_side_avc_motion_estimation extension types
class SPIRVTypeVmeImageINTEL : public SPIRVType {
public:
    const static Op OC = OpTypeVmeImageINTEL;
    const static SPIRVWord FixedWC = 3;
    SPIRVTypeVmeImageINTEL(
        SPIRVModule *M, SPIRVId TheId, SPIRVTypeImage *TheImgTy)
        : SPIRVType(M, FixedWC, OC, TheId), ImgTy(TheImgTy) {
        validate();
    }

    SPIRVTypeVmeImageINTEL() : SPIRVType(OC), ImgTy(nullptr) {}

    const SPIRVTypeImage *getImageType() const { return ImgTy; }
    void setImageType(SPIRVTypeImage *TheImgTy) { ImgTy = TheImgTy; }

    virtual std::vector<SPIRVEntry*> getNonLiteralOperands() const {
        return std::vector<SPIRVEntry*>(1, ImgTy);
    }

    CapVec getRequiredCapability() const override {
        return getVec(CapabilitySubgroupAvcMotionEstimationINTEL);
    }

protected:
    SPIRVTypeImage * ImgTy;
    _SPIRV_DEF_DEC2(Id, ImgTy)

        void validate()const override{
        IGC_ASSERT(OpCode == OC);
        IGC_ASSERT(WordCount == FixedWC);
        IGC_ASSERT(ImgTy && ImgTy->isTypeImage());
    }
};

class SPIRVTypeSubgroupINTEL;
template<> inline void
SPIRVMap<std::string, Op, SPIRVTypeSubgroupINTEL>::init() {
#define _SPIRV_OP(x) \
  add("spirv.Avc"#x, OpTypeAvc##x##INTEL);
_SPIRV_OP(McePayload)
_SPIRV_OP(MceResult)
_SPIRV_OP(SicPayload)
_SPIRV_OP(SicResult)
_SPIRV_OP(ImeResultSingleReferenceStreamout)
_SPIRV_OP(ImeResultDualReferenceStreamout)
_SPIRV_OP(ImeSingleReferenceStreamin)
_SPIRV_OP(ImeDualReferenceStreamin)
_SPIRV_OP(ImePayload)
_SPIRV_OP(ImeResult)
_SPIRV_OP(RefPayload)
_SPIRV_OP(RefResult);
#undef _SPIRV_OP
}
typedef SPIRVMap<std::string, Op, SPIRVTypeSubgroupINTEL>
SPIRVSubgroupINTELTypeOpCodeMap;

class SPIRVTypeSubgroupAvcINTEL : public SPIRVType {
public:
    // Complete constructor
    SPIRVTypeSubgroupAvcINTEL(Op TheOpCode, SPIRVModule *M, SPIRVId TheId)
        : SPIRVType(M, 2, TheOpCode, TheId) {
        validate();
    }

    // Incomplete constructor
    SPIRVTypeSubgroupAvcINTEL(Op TheOpCode)
        : SPIRVType(TheOpCode), Opn(SPIRVID_INVALID) {}

    CapVec getRequiredCapability() const {
        return getVec(CapabilitySubgroupAvcMotionEstimationINTEL);
    }

    SPIRVValue *getOperand() { return getValue(Opn); }

protected:
    _SPIRV_DEF_DEC1(Id)
        void validate() const { SPIRVEntry::validate(); }
    SPIRVId Opn;
};

template<Op TheOpCode>
class SPIRVSubgroupAvcINTELType : public SPIRVTypeSubgroupAvcINTEL {
public:
    // Complete constructor
    SPIRVSubgroupAvcINTELType(SPIRVModule *M, SPIRVId TheId)
        : SPIRVTypeSubgroupAvcINTEL(TheOpCode, M, TheId) {}

    // Incomplete constructor
    SPIRVSubgroupAvcINTELType() : SPIRVTypeSubgroupAvcINTEL(TheOpCode) {}
};

class SPIRVTypeBufferSurfaceINTEL : public SPIRVType {
public:
  const static Op OC = OpTypeBufferSurfaceINTEL;
  const static SPIRVWord FixedWC = 2;
  SPIRVTypeBufferSurfaceINTEL(SPIRVModule *M, SPIRVId TheId,
                              SPIRVAccessQualifierKind TheAccess)
      : SPIRVType(M, FixedWC + 1, OC, TheId), AccessKind(1, TheAccess) {
    validate();
  }
  SPIRVTypeBufferSurfaceINTEL(SPIRVModule *M, SPIRVId TheId)
      : SPIRVType(M, FixedWC, OC, TheId) {
    validate();
  }
  SPIRVTypeBufferSurfaceINTEL() : SPIRVType(OC) {}

  bool hasAccessQualifier() const { return AccessKind.size() > 0; }
  SPIRVAccessQualifierKind getAccessQualifier() const { return AccessKind[0]; }

protected:
  _SPIRV_DEF_DEC2(Id, AccessKind)
  void setWordCount(SPIRVWord TheWC) override {
    if (TheWC > FixedWC)
      AccessKind.push_back(SPIRVAccessQualifierKind::AccessQualifierReadWrite);
    WordCount = TheWC;
  }

private:
  std::vector<SPIRVAccessQualifierKind> AccessKind;
};

#define _SPIRV_OP(x) \
  typedef SPIRVSubgroupAvcINTELType<OpType##x##INTEL> SPIRVType##x##INTEL;
_SPIRV_OP(AvcMcePayload)
_SPIRV_OP(AvcImePayload)
_SPIRV_OP(AvcRefPayload)
_SPIRV_OP(AvcSicPayload)
_SPIRV_OP(AvcMceResult)
_SPIRV_OP(AvcImeResult)
_SPIRV_OP(AvcImeResultSingleReferenceStreamout)
_SPIRV_OP(AvcImeResultDualReferenceStreamout)
_SPIRV_OP(AvcImeSingleReferenceStreamin)
_SPIRV_OP(AvcImeDualReferenceStreamin)
_SPIRV_OP(AvcRefResult)
_SPIRV_OP(AvcSicResult)
#undef _SPIRV_OP

class SPIRVTypeTokenINTEL : public SPIRVType {
public:
    // Complete constructor
    SPIRVTypeTokenINTEL(SPIRVModule* M, SPIRVId TheId)
        : SPIRVType(M, 2, OpTypeTokenINTEL, TheId) {}
    // Incomplete constructor
    SPIRVTypeTokenINTEL() : SPIRVType(OpTypeTokenINTEL) {}

    CapVec getRequiredCapability() const {
        return getVec(CapabilityTokenTypeINTEL);
    }

protected:
    _SPIRV_DEF_DEC1(Id)
};

}
#endif // SPIRVTYPE_HPP_
