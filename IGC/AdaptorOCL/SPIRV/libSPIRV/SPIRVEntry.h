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

// This file defines the base class for SPIRV entities.

#ifndef SPIRVENTRY_HPP_
#define SPIRVENTRY_HPP_

#include "SPIRVEnum.h"
#include "SPIRVError.h"

#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "Probe/Assertion.h"

namespace igc_spv{

class SPIRVModule;
class SPIRVDecoder;
class SPIRVType;
class SPIRVValue;
class SPIRVDecorate;
class SPIRVDecorateId;
class SPIRVForward;
class SPIRVMemberDecorate;
class SPIRVLine;
class SPIRVString;
class SPIRVExtInst;

// Add declaration of decode functions to a class.
// Used inside class definition.
#define _SPIRV_DCL_DEC \
    void decode(std::istream &I);

#define _SPIRV_DCL_DEC_OVERRIDE \
    void decode(std::istream &I) override;

// Add implementation of decode functions to a class.
// Used out side of class definition.
#define _SPIRV_IMP_DEC0(Ty)                                                              \
    void Ty::decode(std::istream &I) {}
#define _SPIRV_IMP_DEC1(Ty,x)                                                            \
    void Ty::decode(std::istream &I) { getDecoder(I) >> x;}
#define _SPIRV_IMP_DEC2(Ty,x,y)                                                          \
    void Ty::decode(std::istream &I) { getDecoder(I) >> x >> y;}
#define _SPIRV_IMP_DEC3(Ty,x,y,z)                                                        \
    void Ty::decode(std::istream &I) { getDecoder(I) >> x >> y >> z;}
#define _SPIRV_IMP_DEC4(Ty,x,y,z,u)                                                      \
    void Ty::decode(std::istream &I) { getDecoder(I) >> x >> y >> z >> u;}
#define _SPIRV_IMP_DEC5(Ty,x,y,z,u,v)                                                    \
    void Ty::decode(std::istream &I) { getDecoder(I) >> x >> y >> z >> u >> v;}
#define _SPIRV_IMP_DEC6(Ty,x,y,z,u,v,w)                                                  \
    void Ty::decode(std::istream &I) { getDecoder(I) >> x >> y >> z >> u >> v >> w;}
#define _SPIRV_IMP_DEC7(Ty,x,y,z,u,v,w,r)                                                \
    void Ty::decode(std::istream &I) { getDecoder(I) >> x >> y >> z >> u >> v >> w >> r;}
#define _SPIRV_IMP_DEC8(Ty,x,y,z,u,v,w,r,s)                                              \
    void Ty::decode(std::istream &I) { getDecoder(I) >> x >> y >> z >> u >>              \
      v >> w >> r >> s;}
#define _SPIRV_IMP_DEC9(Ty,x,y,z,u,v,w,r,s,t)                                            \
    void Ty::decode(std::istream &I) { getDecoder(I) >> x >> y >> z >> u >>              \
      v >> w >> r >> s >> t;}

// Add definition of decode functions to a class.
// Used inside class definition.
#define _SPIRV_DEF_DEC0                                                                  \
    void decode(std::istream &I) {}
#define _SPIRV_DEF_DEC1(x)                                                               \
    void decode(std::istream &I) { getDecoder(I) >> x;}
#define _SPIRV_DEF_DEC1_OVERRIDE(x)                                                      \
    void decode(std::istream &I) override { getDecoder(I) >> x;}
#define _SPIRV_DEF_DEC2(x,y)                                                             \
    void decode(std::istream &I) override { getDecoder(I) >> x >> y;}
#define _SPIRV_DEF_DEC3(x,y,z)                                                           \
    void decode(std::istream &I) { getDecoder(I) >> x >> y >> z;}
#define _SPIRV_DEF_DEC3_OVERRIDE(x,y,z)                                                  \
    void decode(std::istream &I) override { getDecoder(I) >> x >> y >> z;}
#define _SPIRV_DEF_DEC4(x,y,z,u)                                                         \
    void decode(std::istream &I) { getDecoder(I) >> x >> y >> z >> u;}
#define _SPIRV_DEF_DEC4_OVERRIDE(x,y,z,u)                                                \
    void decode(std::istream &I) override { getDecoder(I) >> x >> y >> z >> u;}
#define _SPIRV_DEF_DEC5(x,y,z,u,v)                                                       \
    void decode(std::istream &I) { getDecoder(I) >> x >> y >> z >> u >> v;}
#define _SPIRV_DEF_DEC6(x,y,z,u,v,w)                                                     \
    void decode(std::istream &I) override { getDecoder(I) >> x >> y >> z >> u >> v >> w;}
#define _SPIRV_DEF_DEC7(x,y,z,u,v,w,r)                                                   \
    void decode(std::istream &I) { getDecoder(I) >> x >> y >> z >> u >> v >> w >> r;}
#define _SPIRV_DEF_DEC8(x,y,z,u,v,w,r,s)                                                 \
    void decode(std::istream &I) { getDecoder(I) >> x >> y >> z >> u >> v >>             \
      w >> r >> s;}
#define _SPIRV_DEF_DEC9(x,y,z,u,v,w,r,s,t)                                               \
    void decode(std::istream &I) { getDecoder(I) >> x >> y >> z >> u >> v >>             \
      w >> r >> s >> t;}

/// All SPIR-V in-memory-representation entities inherits from SPIRVEntry.
/// Usually there are two flavors of constructors of SPIRV objects:
///
/// 1. complete constructor: It requires all the parameters needed to create a
///    SPIRV entity with complete information which can be validated. It is
///    usually used by LLVM/SPIR-V translator to create SPIRV object
///    corresponding to LLVM object. Such constructor calls validate() at
///    the end of the construction.
///
/// 2. incomplete constructor: For leaf classes, it has no parameters.
///    It is usually called by SPIRVEntry::make(opcode) to create an incomplete
///    object which should not be validated. Then setWordCount(count) is
///    called to fix the size of the object if it is variable, and then the
///    information is filled by the virtual function decode(istream).
///    After that the object can be validated.
///
/// To add a new SPIRV class:
///
/// 1. It is recommended to name the class as SPIRVXXX if it has a fixed op code
///    OpXXX. Although it is not mandatory, doing this facilitates adding it to
///    the table of the factory function SPIRVEntry::create().
/// 2. Inherit from proper SPIRV class such as SPIRVType, SPIRVValue,
///    SPIRVInstruction, etc.
/// 3. Implement virtual function decode(), validate().
/// 4. If the object has variable size, implement virtual function
///    setWordCount().
/// 5. If the class has special attributes, e.g. having no id, or having no
///    type as a value, set them in the constructors.
/// 6. Add the class to the Table of SPIRVEntry::create().
/// 7. Add the class to SPIRVToLLVM and LLVMToSPIRV.

class SPIRVEntry {
public:
  typedef std::vector<SPIRVCapabilityKind> CapVec;
  enum SPIRVEntryAttrib {
    SPIRVEA_DEFAULT     = 0,
    SPIRVEA_NOID        = 1,      // Entry has no valid id
    SPIRVEA_NOTYPE      = 2,      // Value has no type
  };

  // Complete constructor for objects with id
  SPIRVEntry(SPIRVModule *M, unsigned TheWordCount, Op TheOpCode,
      SPIRVId TheId)
    :Module(M), OpCode(TheOpCode), Id(TheId), Attrib(SPIRVEA_DEFAULT),
     WordCount(TheWordCount), Line(nullptr){
    validate();
  }

  // Complete constructor for objects without id
  SPIRVEntry(SPIRVModule *M, unsigned TheWordCount, Op TheOpCode)
    :Module(M), OpCode(TheOpCode), Id(SPIRVID_INVALID), Attrib(SPIRVEA_NOID),
     WordCount(TheWordCount), Line(nullptr){
    validate();
  }

  // Incomplete constructor
  SPIRVEntry(Op TheOpCode)
    :Module(NULL), OpCode(TheOpCode), Id(SPIRVID_INVALID),
     Attrib(SPIRVEA_DEFAULT), WordCount(0), Line(nullptr){}

  SPIRVEntry()
    :Module(NULL), OpCode(OpNop), Id(SPIRVID_INVALID),
     Attrib(SPIRVEA_DEFAULT), WordCount(0), Line(nullptr){}


  virtual ~SPIRVEntry(){}

  bool exist(SPIRVId)const;
  template<class T>
  T* get(SPIRVId TheId)const { return static_cast<T*>(getEntry(TheId));}
  SPIRVEntry *getEntry(SPIRVId) const;
  SPIRVEntry *getOrCreate(SPIRVId TheId) const;
  SPIRVValue *getValue(SPIRVId TheId)const;
  std::vector<SPIRVValue *> getValues(const std::vector<SPIRVId>&)const;
  std::vector<SPIRVId> getIds(const std::vector<SPIRVValue *>)const;
  SPIRVType *getValueType(SPIRVId TheId)const;
  std::vector<SPIRVType *> getValueTypes(const std::vector<SPIRVId>&)const;

  virtual SPIRVDecoder getDecoder(std::istream &);
  SPIRVErrorLog &getErrorLog()const;
  SPIRVId getId() const { IGC_ASSERT(hasId()); return Id;}
  SPIRVLine *getLine() const { return Line;}
  SPIRVLinkageTypeKind getLinkageType() const;
  Op getOpCode() const { return OpCode;}
  SPIRVModule *getModule() const { return Module;}
  virtual CapVec getRequiredCapability() const { return CapVec();}
  const std::string& getName() const { return Name;}
  bool hasDecorate(Decoration Kind, size_t Index = 0,
      SPIRVWord *Result=0)const;
  bool hasDecorateId(Decoration Kind, size_t Index = 0,
      SPIRVId* Result = 0) const;
  std::set<SPIRVWord> getDecorate(Decoration Kind, size_t Index = 0)const;
  std::vector<SPIRVDecorate const*> getDecorations(Decoration Kind) const;
  std::set<SPIRVId> getDecorateId(Decoration Kind, size_t Index = 0) const;
  std::vector<SPIRVDecorateId const*> getDecorationIds(Decoration Kind) const;
  std::vector<std::string> getDecorationStringLiteral(Decoration Kind) const;
  std::vector<SPIRVId> getDecorationIdLiterals(Decoration Kind) const;
  bool hasId() const { return !(Attrib & SPIRVEA_NOID);}
  bool hasLine() const { return Line != nullptr;}
  bool hasLinkageType() const;
  bool isAtomic() const { return isAtomicOpCode(OpCode);}
  bool isBasicBlock() const { return isLabel();}
  bool isBuiltinCall() const { return OpCode == OpExtInst;}
  bool isDecorate()const { return OpCode == OpDecorate;}
  bool isDecorateId() const { return OpCode == OpDecorateId; }
  bool isMemberDecorate()const { return OpCode == OpMemberDecorate;}
  bool isForward() const { return OpCode == OpForward;}
  bool isLabel() const { return OpCode == OpLabel;}
  bool isUndef() const { return OpCode == OpUndef;}
  bool isControlBarrier() const { return OpCode == OpControlBarrier;}
  bool isMemoryBarrier() const { return OpCode == OpMemoryBarrier;}
  bool isVariable() const { return OpCode == OpVariable;}
  virtual bool isInst() const { return false;}

  void addDecorate(const SPIRVDecorate *);
  void addDecorate(SPIRVDecorateId*);
  void addDecorate(Decoration Kind);
  void addDecorate(Decoration Kind, SPIRVWord Literal);
  void eraseDecorate(Decoration);
  void eraseDecorateId(Decoration);
  void addMemberDecorate(const SPIRVMemberDecorate *);
  void addMemberDecorate(SPIRVWord MemberNumber, Decoration Kind);
  void addMemberDecorate(SPIRVWord MemberNumber, Decoration Kind,
      SPIRVWord Literal);
  void eraseMemberDecorate(SPIRVWord MemberNumber, Decoration Kind);
  void setHasNoId() { Attrib |= SPIRVEA_NOID;}
  void setId(SPIRVId TheId) { Id = TheId;}
  void setLine(SPIRVLine*);
  void setDIScope(SPIRVExtInst*);
  SPIRVExtInst* getDIScope();
  void setLinkageType(SPIRVLinkageTypeKind);
  void setModule(SPIRVModule *TheModule);
  void setName(const std::string& TheName);
  virtual void setScope(SPIRVEntry *Scope){};
  void takeAnnotations(SPIRVForward *);
  void takeDecorates(SPIRVEntry *);
  void takeDecorateIds(SPIRVEntry*);
  void takeMemberDecorates(SPIRVEntry *);
  void takeLine(SPIRVEntry *);

  /// After a SPIRV entry is created during reading SPIRV binary by default
  /// constructor, this function is called to allow the SPIRV entry to resize
  /// its variable sized member before decoding the remaining words.
  virtual void setWordCount(SPIRVWord TheWordCount);

  /// Create an empty SPIRV object by op code, e.g. OpTypeInt creates
  /// SPIRVTypeInt.
  static SPIRVEntry *create(Op);

  friend std::istream &operator>>(std::istream &I, SPIRVEntry &E);
  virtual void decode(std::istream &I);

  friend class SPIRVDecoder;

  /// Checks the integrity of the object.
  virtual void validate()const {
    IGC_ASSERT_MESSAGE(Module, "Invalid module");
    IGC_ASSERT_MESSAGE(OpCode != OpNop, "Invalid op code");
    IGC_ASSERT_MESSAGE((!hasId() || isValid(Id)), "Invalid Id");
  }
  void validateFunctionControlMask(SPIRVWord FCtlMask)const;
  void validateValues(const std::vector<SPIRVId> &)const;
  void validateBuiltin(SPIRVWord, SPIRVWord)const;

  virtual bool hasNoScope() { return false; }
  virtual bool isOpLine() { return false; }
  virtual bool isOpNoLine() { return false; }
  virtual bool isScope() { return false; }
  virtual bool startsScope() { return false; }
  virtual bool endsScope() { return false; }
  virtual bool isString() { return false; }
  virtual bool isConstant() { return false; }

protected:
  /// An entry may have multiple FuncParamAttr decorations.
  typedef std::multimap<Decoration, const SPIRVDecorate*> DecorateMapType;
  typedef std::multimap<Decoration, const SPIRVDecorateId*> DecorateIdMapType;
  typedef std::map<std::pair<SPIRVWord, Decoration>,
      const SPIRVMemberDecorate*> MemberDecorateMapType;

  bool canHaveMemberDecorates() const {
    return OpCode == OpTypeStruct ||
        OpCode == OpForward;
  }
  MemberDecorateMapType& getMemberDecorates() {
    IGC_ASSERT(canHaveMemberDecorates());
    return MemberDecorates;
  }

  SPIRVModule *Module;
  Op OpCode;
  SPIRVId Id;
  std::string Name;
  unsigned Attrib;
  SPIRVWord WordCount;

  DecorateMapType Decorates;
  DecorateIdMapType DecorateIds;
  MemberDecorateMapType MemberDecorates;
  SPIRVLine *Line;
  SPIRVExtInst* diScope = nullptr;
};

class SPIRVEntryNoIdGeneric:public SPIRVEntry {
public:
  SPIRVEntryNoIdGeneric(SPIRVModule *M, unsigned TheWordCount, Op OC)
    :SPIRVEntry(M, TheWordCount, OC){
    setAttr();
  }
  SPIRVEntryNoIdGeneric(Op OC):SPIRVEntry(OC){
    setAttr();
  }
protected:
  void setAttr() {
    setHasNoId();
  }
};

template<Op OC>
class SPIRVEntryNoId:public SPIRVEntryNoIdGeneric {
public:
  SPIRVEntryNoId(SPIRVModule *M, unsigned TheWordCount)
    :SPIRVEntryNoIdGeneric(M, TheWordCount, OC){}
  SPIRVEntryNoId():SPIRVEntryNoIdGeneric(OC){}
};

template<Op TheOpCode>
class SPIRVEntryOpCodeOnly:public SPIRVEntryNoId<TheOpCode> {
public:
  SPIRVEntryOpCodeOnly(){
    SPIRVEntry::WordCount = 1;
    validate();
  }
protected:
  _SPIRV_DEF_DEC0
  void validate()const {
    IGC_ASSERT(isValid(SPIRVEntry::OpCode));
  }
};

class SPIRVAnnotationGeneric:public SPIRVEntryNoIdGeneric {
public:
  // Complete constructor
  SPIRVAnnotationGeneric(const SPIRVEntry *TheTarget, unsigned TheWordCount,
      Op OC)
      :SPIRVEntryNoIdGeneric((TheTarget ? TheTarget->getModule() : NULL), TheWordCount, OC),
     Target(TheTarget ? TheTarget->getId() : SPIRVID_INVALID){}
  // Incomplete constructor
  SPIRVAnnotationGeneric(Op OC):SPIRVEntryNoIdGeneric(OC),
      Target(SPIRVID_INVALID){}

  SPIRVId getTargetId()const { return Target;}
  SPIRVForward *getOrCreateTarget()const;
  void setTargetId(SPIRVId T) { Target = T;}
protected:
  SPIRVId Target;
};

template<Op OC>
class SPIRVAnnotation:public SPIRVAnnotationGeneric {
public:
  // Complete constructor
  SPIRVAnnotation(const SPIRVEntry *TheTarget, unsigned TheWordCount)
    :SPIRVAnnotationGeneric(TheTarget, TheWordCount, OC){}
  // Incomplete constructor
  SPIRVAnnotation():SPIRVAnnotationGeneric(OC){}
};

class SPIRVEntryPoint:public SPIRVAnnotation<OpEntryPoint> {
public:
  SPIRVEntryPoint(SPIRVModule *TheModule, SPIRVExecutionModelKind,
      SPIRVId TheId, const std::string &TheName);
  SPIRVEntryPoint():ExecModel(ExecutionModelKernel){}
  _SPIRV_DCL_DEC
protected:
  SPIRVExecutionModelKind ExecModel;
  std::string Name;
  CapVec getRequiredCapability() const {
    return getVec(getCapability(ExecModel));
  }
};


class SPIRVName:public SPIRVAnnotation<OpName> {
public:
  // Complete constructor
  SPIRVName(const SPIRVEntry *TheTarget, const std::string& TheStr);
  // Incomplete constructor
  SPIRVName(){}
protected:
  _SPIRV_DCL_DEC
  void validate() const;

  std::string Str;
};

class SPIRVMemberName:public SPIRVAnnotation<OpName> {
public:
  static const SPIRVWord FixedWC = 3;
  // Complete constructor
  SPIRVMemberName(const SPIRVEntry *TheTarget, SPIRVWord TheMemberNumber,
      const std::string& TheStr)
    :SPIRVAnnotation(TheTarget, FixedWC + getSizeInWords(TheStr)),
     MemberNumber(TheMemberNumber), Str(TheStr){
    validate();
  }
  // Incomplete constructor
  SPIRVMemberName():MemberNumber(SPIRVWORD_MAX){}
protected:
  _SPIRV_DCL_DEC
  void validate() const;
  SPIRVWord MemberNumber;
  std::string Str;
};

class SPIRVString:public SPIRVEntry {
  static const Op OC = OpString;
  static const SPIRVWord FixedWC = 2;
public:
  SPIRVString(SPIRVModule *M, SPIRVId TheId, const std::string &TheStr)
    :SPIRVEntry(M, FixedWC + getSizeInWords(TheStr), OC, TheId), Str(TheStr){}
  SPIRVString():SPIRVEntry(OC){}
  _SPIRV_DCL_DEC
  const std::string &getStr()const { return Str;}
  bool isString() { return true; }
protected:
  std::string Str;
};

class SPIRVLine:public SPIRVEntryNoIdGeneric {
public:
  static const SPIRVWord WC = 5;
  // Complete constructor
  SPIRVLine(SPIRVModule* M, SPIRVId TheFileName, SPIRVWord TheLine,
      SPIRVWord TheColumn)
    :SPIRVEntryNoIdGeneric(M, WC, OpLine), FileName(TheFileName), Line(TheLine),
     Column(TheColumn){
    validate();
  }
  // Incomplete constructor
  SPIRVLine(): SPIRVEntryNoIdGeneric(OpLine), FileName(SPIRVID_INVALID),
    Line(SPIRVWORD_MAX), Column(SPIRVWORD_MAX) {}

  SPIRVWord getColumn() const {
    return Column;
  }

  void setColumn(SPIRVWord column) {
    Column = column;
  }

  SPIRVId getFileName() const {
    return FileName;
  }

  const std::string &getFileNameStr() const {
    return get<SPIRVString>(FileName)->getStr();
  }

  void setFileName(SPIRVId fileName) {
    FileName = fileName;
  }

  SPIRVWord getLine() const {
    return Line;
  }

  void setLine(SPIRVWord line) {
    Line = line;
  }

  bool isOpLine() { return true; }

protected:
  _SPIRV_DCL_DEC
  void validate() const;
  SPIRVId FileName;
  SPIRVWord Line;
  SPIRVWord Column;
};

class SPIRVNoLine :public SPIRVEntryNoIdGeneric {
public:
    static const SPIRVWord WC = 1;
    // Complete constructor
    SPIRVNoLine(SPIRVModule* M)
        :SPIRVEntryNoIdGeneric(M, WC, OpNoLine)
    {
        validate();
    }
    // Incomplete constructor
    SPIRVNoLine() : SPIRVEntryNoIdGeneric(OpNoLine) {}

    bool isOpLine() { return true; }

protected:
    _SPIRV_DCL_DEC
        void validate() const;
};

class SPIRVExecutionMode:public SPIRVAnnotation<OpExecutionMode> {
public:
  // Complete constructor for LocalSize, LocalSizeHint
  SPIRVExecutionMode(SPIRVEntry *TheTarget, SPIRVExecutionModeKind TheExecMode,
      SPIRVWord x, SPIRVWord y, SPIRVWord z)
  :SPIRVAnnotation(TheTarget, 6), ExecMode(TheExecMode){
    WordLiterals.push_back(x);
    WordLiterals.push_back(y);
    WordLiterals.push_back(z);
  }
  // Complete constructor for VecTypeHint
  SPIRVExecutionMode(SPIRVEntry *TheTarget, SPIRVExecutionModeKind TheExecMode,
      SPIRVWord code)
  :SPIRVAnnotation(TheTarget, 4),
   ExecMode(TheExecMode) {
    WordLiterals.push_back(code);
  }
  // Complete constructor for ContractionOff
  SPIRVExecutionMode(SPIRVEntry *TheTarget, SPIRVExecutionModeKind TheExecMode)
  :SPIRVAnnotation(TheTarget, 3), ExecMode(TheExecMode){}
  // Incomplete constructor
  SPIRVExecutionMode():ExecMode(ExecutionModeCount){}
  SPIRVExecutionModeKind getExecutionMode()const { return ExecMode;}
  const std::vector<SPIRVWord>& getLiterals()const { return WordLiterals;}
  CapVec getRequiredCapability() const {
    return getVec(getCapability(ExecMode));
  }
protected:
  _SPIRV_DCL_DEC
  SPIRVExecutionModeKind ExecMode;
  std::vector<SPIRVWord> WordLiterals;
};


class SPIRVComponentExecutionModes {
  typedef std::map<SPIRVExecutionModeKind, SPIRVExecutionMode*>
    SPIRVExecutionModeMap;
public:
  void addExecutionMode(SPIRVExecutionMode *ExecMode) {
    ExecModes[ExecMode->getExecutionMode()] = ExecMode;
  }
  SPIRVExecutionMode *getExecutionMode(SPIRVExecutionModeKind EMK)const {
    auto Loc = ExecModes.find(EMK);
    if (Loc == ExecModes.end())
      return nullptr;
    return Loc->second;
  }
protected:
  SPIRVExecutionModeMap ExecModes;
};

class SPIRVExtInstImport:public SPIRVEntry {
public:
  const static Op OC = OpExtInstImport;
  // Complete constructor
  SPIRVExtInstImport(SPIRVModule *TheModule, SPIRVId TheId,
      const std::string& TheStr);
  // Incomplete constructor
  SPIRVExtInstImport():SPIRVEntry(OC){}
protected:
  _SPIRV_DCL_DEC
  void validate() const;

  std::string Str;
};

class SPIRVMemoryModel:public SPIRVEntryNoId<OpMemoryModel> {
public:
  SPIRVMemoryModel(SPIRVModule *M):SPIRVEntryNoId(M, 3){}
  SPIRVMemoryModel(){}
  _SPIRV_DCL_DEC
  void validate() const;
};

class SPIRVSource:public SPIRVEntryNoId<OpSource> {
public:
  SPIRVSource(SPIRVModule *M):SPIRVEntryNoId(M, 3){}
  SPIRVSource(){}
  _SPIRV_DCL_DEC
};

class SPIRVSourceExtension:public SPIRVEntryNoId<OpSourceExtension> {
public:
  SPIRVSourceExtension(SPIRVModule *M, const std::string &SS);
  SPIRVSourceExtension(){}
  _SPIRV_DCL_DEC
private:
  std::string S;
};

class SPIRVExtension:public SPIRVEntryNoId<OpExtension> {
public:
  SPIRVExtension(SPIRVModule *M, const std::string &SS);
  SPIRVExtension(){}
  _SPIRV_DCL_DEC
private:
  std::string S;
};

class SPIRVCapability:public SPIRVEntryNoId<OpCapability> {
public:
  SPIRVCapability(SPIRVModule *M, SPIRVCapabilityKind K);
  SPIRVCapability():Kind(CapabilityNone){}
  _SPIRV_DCL_DEC
private:
  SPIRVCapabilityKind Kind;
};

class SPIRVModuleProcessed: public SPIRVEntryNoId<OpModuleProcessed> {
public:
  SPIRVModuleProcessed(SPIRVModule *M):SPIRVEntryNoId(M, 2){}
  SPIRVModuleProcessed(){}
  _SPIRV_DCL_DEC
private:
  std::string S;
};

template<class T>
T* bcast(SPIRVEntry *E) {
  return static_cast<T*>(E);
}

template <igc_spv::Op OC> bool isa(SPIRVEntry *E) {
  return E ? E->getOpCode() == OC : false;
}

template <igc_spv::Op OC>
class SPIRVContinuedInstINTELBase : public SPIRVEntryNoId<OC> {
public:
    template <igc_spv::Op _OC, class T = void>
    using EnableIfCompositeConst =
        typename std::enable_if_t<_OC == OpConstantCompositeContinuedINTEL ||
        _OC ==
        OpSpecConstantCompositeContinuedINTEL,
        T>;
    // Complete constructor
    SPIRVContinuedInstINTELBase(SPIRVModule* M,
        const std::vector<SPIRVValue*>& TheElements)
        : SPIRVEntryNoId<OC>(M, TheElements.size() + 1) {

        Elements = SPIRVEntry::getIds(TheElements);
        validate();
    }

    SPIRVContinuedInstINTELBase(SPIRVModule* M, unsigned NumOfElements)
        : SPIRVEntryNoId<OC>(M, NumOfElements + 1) {
        Elements.resize(NumOfElements, SPIRVID_INVALID);
        validate();
    }

    // Incomplete constructor
    SPIRVContinuedInstINTELBase() : SPIRVEntryNoId<OC>() {}

    template <igc_spv::Op OPC = OC>
    EnableIfCompositeConst<OPC, std::vector<SPIRVValue*>> getElements() const {
        return SPIRVEntry::getValues(Elements);
    }

    SPIRVWord getNumElements() const { return Elements.size(); }

protected:
    void validate() const override;

    void setWordCount(SPIRVWord WordCount) override {
        SPIRVEntry::setWordCount(WordCount);
        Elements.resize(WordCount - 1);
    }
    _SPIRV_DCL_DEC_OVERRIDE

    std::vector<SPIRVId> Elements;
};

class SPIRVTypeStructContinuedINTEL
    : public SPIRVContinuedInstINTELBase<OpTypeStructContinuedINTEL> {
public:
    constexpr static Op OC = OpTypeStructContinuedINTEL;
    // Complete constructor
    SPIRVTypeStructContinuedINTEL(SPIRVModule * M, unsigned NumOfElements)
        : SPIRVContinuedInstINTELBase<OC>(M, NumOfElements) {}

    // Incomplete constructor
    SPIRVTypeStructContinuedINTEL() : SPIRVContinuedInstINTELBase<OC>() {}

    void setElementId(size_t I, SPIRVId Id) { Elements[I] = Id; }

    SPIRVType* getMemberType(size_t I) const;
};
using SPIRVConstantCompositeContinuedINTEL =
SPIRVContinuedInstINTELBase<OpConstantCompositeContinuedINTEL>;
using SPIRVSpecConstantCompositeContinuedINTEL =
SPIRVContinuedInstINTELBase<OpSpecConstantCompositeContinuedINTEL>;

template <igc_spv::Op OpCode> struct InstToContinued;

template <> struct InstToContinued<OpTypeStruct> {
    using Type = SPIRVTypeStructContinuedINTEL *;
    constexpr static igc_spv::Op OpCode = OpTypeStructContinuedINTEL;
};

template <> struct InstToContinued<OpConstantComposite> {
    using Type = SPIRVConstantCompositeContinuedINTEL *;
    constexpr static igc_spv::Op OpCode = OpConstantCompositeContinuedINTEL;
};

template <> struct InstToContinued<OpSpecConstantComposite> {
    using Type = SPIRVSpecConstantCompositeContinuedINTEL *;
    constexpr static igc_spv::Op OpCode = OpSpecConstantCompositeContinuedINTEL;
};


// ToDo: The following typedef's are place holders for SPIRV entity classes
// to be implemented.
// Each time a new class is implemented, remove the corresponding typedef.
// This is also an indication of how much work is left.
#define _SPIRV_OP(x, ...) typedef SPIRVEntryOpCodeOnly<Op##x> SPIRV##x;
_SPIRV_OP(Nop)
_SPIRV_OP(SourceContinued)
_SPIRV_OP(TypeMatrix)
_SPIRV_OP(TypeRuntimeArray)
_SPIRV_OP(ImageTexelPointer)
_SPIRV_OP(ImageSampleDrefImplicitLod)
_SPIRV_OP(ImageSampleDrefExplicitLod)
_SPIRV_OP(ImageSampleProjImplicitLod)
_SPIRV_OP(ImageSampleProjExplicitLod)
_SPIRV_OP(ImageSampleProjDrefImplicitLod)
_SPIRV_OP(ImageSampleProjDrefExplicitLod)
_SPIRV_OP(ImageFetch)
_SPIRV_OP(ImageGather)
_SPIRV_OP(ImageDrefGather)
_SPIRV_OP(QuantizeToF16)
_SPIRV_OP(Transpose)
_SPIRV_OP(ArrayLength)
_SPIRV_OP(MatrixTimesScalar)
_SPIRV_OP(VectorTimesMatrix)
_SPIRV_OP(MatrixTimesVector)
_SPIRV_OP(MatrixTimesMatrix)
_SPIRV_OP(OuterProduct)
_SPIRV_OP(IAddCarry)
_SPIRV_OP(ISubBorrow)
_SPIRV_OP(BitFieldInsert)
_SPIRV_OP(BitFieldSExtract)
_SPIRV_OP(BitFieldUExtract)
_SPIRV_OP(DPdx)
_SPIRV_OP(DPdy)
_SPIRV_OP(Fwidth)
_SPIRV_OP(DPdxFine)
_SPIRV_OP(DPdyFine)
_SPIRV_OP(FwidthFine)
_SPIRV_OP(DPdxCoarse)
_SPIRV_OP(DPdyCoarse)
_SPIRV_OP(FwidthCoarse)
_SPIRV_OP(EmitVertex)
_SPIRV_OP(EndPrimitive)
_SPIRV_OP(EmitStreamVertex)
_SPIRV_OP(EndStreamPrimitive)
_SPIRV_OP(Kill)
_SPIRV_OP(ImageSparseSampleImplicitLod)
_SPIRV_OP(ImageSparseSampleExplicitLod)
_SPIRV_OP(ImageSparseSampleDrefImplicitLod)
_SPIRV_OP(ImageSparseSampleDrefExplicitLod)
_SPIRV_OP(ImageSparseSampleProjImplicitLod)
_SPIRV_OP(ImageSparseSampleProjExplicitLod)
_SPIRV_OP(ImageSparseSampleProjDrefImplicitLod)
_SPIRV_OP(ImageSparseSampleProjDrefExplicitLod)
_SPIRV_OP(ImageSparseFetch)
_SPIRV_OP(ImageSparseGather)
_SPIRV_OP(ImageSparseDrefGather)
_SPIRV_OP(ImageSparseTexelsResident)
#undef _SPIRV_OP

}
#endif /* SPIRVENTRY_HPP_ */
