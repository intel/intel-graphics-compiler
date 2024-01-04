/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

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

// This file implements Module class for SPIR-V.

#include "SPIRVModule.h"
#include "SPIRVDebug.h"
#include "SPIRVDecorate.h"
#include "SPIRVEntry.h"
#include "SPIRVValue.h"
#include "SPIRVFunction.h"
#include "SPIRVInstruction.h"
#include "SPIRVMemAliasingINTEL.h"
#include "SPIRVAsm.h"
#include "Probe/Assertion.h"

namespace igc_spv{

SPIRVModule::SPIRVModule()
{}

SPIRVModule::~SPIRVModule()
{}

class SPIRVModuleImpl : public SPIRVModule {
public:
  SPIRVModuleImpl():SPIRVModule(), NextId(0),
    SPIRVVersion(SPIRVVersionSupported::fullyCompliant),
    SPIRVGenerator(SPIRVGEN_AMDOpenSourceLLVMSPIRVTranslator),
    InstSchema(SPIRVISCH_Default),
    SrcLang(SpvSourceLanguageOpenCL_C),
    SrcLangVer(12),
    MemoryModel(SPIRVMemoryModelKind::MemoryModelOpenCL),
    SCMap(nullptr) {
    AddrModel = sizeof(size_t) == 32 ? AddressingModelPhysical32 : AddressingModelPhysical64;
  };
  virtual ~SPIRVModuleImpl();
  SPIRVModuleImpl(const SPIRVModuleImpl&) = delete;
  SPIRVModuleImpl& operator=(const SPIRVModuleImpl&) = delete;

  // Object query functions
  bool exist(SPIRVId) const override;
  bool exist(SPIRVId, SPIRVEntry **) const override;
  SPIRVId getId(SPIRVId Id = SPIRVID_INVALID, unsigned Increment = 1);
  virtual SPIRVEntry *getEntry(SPIRVId Id) const override;
  virtual void addUnknownStructField(
    SPIRVTypeStruct*, unsigned idx, SPIRVId id) override;
  virtual void resolveUnknownStructFields() override;
  bool hasDebugInfo() const override
  {
      return getCompilationUnit() != nullptr;
  }

  // Error handling functions
  SPIRVErrorLog &getErrorLog() override { return ErrLog;}
  SPIRVErrorCode getError(std::string &ErrMsg) override { return ErrLog.getError(ErrMsg);}

  // Module query functions
  SPIRVAddressingModelKind getAddressingModel() override { return AddrModel;}
  SPIRVExtInstSetKind getBuiltinSet(SPIRVId SetId) const override;
  const SPIRVCapSet &getCapability() const override { return CapSet;}
  bool isSpecConstantSpecialized(SPIRVWord spec_id) const override {
    if(SCMap)
      return SCMap->find(spec_id) != SCMap->end();
    else
      return false;
  }
  uint64_t getSpecConstant(SPIRVWord spec_id) override {
    IGC_ASSERT_EXIT_MESSAGE(isSpecConstantSpecialized(spec_id), "Specialization constant was not specialized!");
    return SCMap->at(spec_id);
  }
  void setSpecConstantMap(SPIRVSpecConstantMap *specConstants) override { SCMap = specConstants; }
  std::set<std::string> &getExtension() override { return SPIRVExt;}
  SPIRVFunction *getFunction(unsigned I) const override { return FuncVec[I];}
  SPIRVAsmINTEL *getAsm(unsigned I) const override { return AsmVec[I]; }
  SPIRVVariable *getVariable(unsigned I) const override { return VariableVec[I];}
  virtual SPIRVValue *getValue(SPIRVId TheId) const override;
  virtual std::vector<SPIRVValue *> getValues(const std::vector<SPIRVId>&)const override;
  virtual std::vector<SPIRVId> getIds(const std::vector<SPIRVEntry *>&)const override;
  virtual std::vector<SPIRVId> getIds(const std::vector<SPIRVValue *>&)const override;
  virtual SPIRVType *getValueType(SPIRVId TheId)const override;
  virtual std::vector<SPIRVType *> getValueTypes(const std::vector<SPIRVId>&)
      const override;
  SPIRVMemoryModelKind getMemoryModel() override { return MemoryModel;}
  virtual SPIRVConstant* getLiteralAsConstant(unsigned Literal) override;
  unsigned getNumFunctions() const override { return FuncVec.size();}
  unsigned getNumVariables() const override { return VariableVec.size();}
  SpvSourceLanguage getSourceLanguage(SPIRVWord * Ver = nullptr) const override {
    if (Ver)
      *Ver = SrcLangVer;
    return SrcLang;
  }
  std::set<std::string> &getSourceExtension() override { return SrcExtension;}
  bool isEntryPoint(SPIRVExecutionModelKind, SPIRVId EP) const override;
  const std::string &getModuleProcessed() const { return ModuleProcessed; }
  const std::vector<SPIRVString *> &getStringVec() const override { return StringVec; }

  // Module changing functions
  bool importBuiltinSet(const std::string &, SPIRVId *) override;
  bool importBuiltinSetWithId(const std::string &, SPIRVId) override;
  void setAddressingModel(SPIRVAddressingModelKind AM) override { AddrModel = AM;}
  void setAlignment(SPIRVValue *, SPIRVWord) override;
  void setMemoryModel(SPIRVMemoryModelKind MM) override { MemoryModel = MM;}
  void setName(SPIRVEntry *E, const std::string &Name) override;
  void setSourceLanguage(SpvSourceLanguage Lang, SPIRVWord Ver) override {
    SrcLang = Lang;
    SrcLangVer = Ver;
  }
  void setModuleProcessed(const std::string& MP) override {
    ModuleProcessed = MP;
  }

  // Object creation functions
  template<class T> void addTo(std::vector<T *> &V, SPIRVEntry *E);
  virtual SPIRVEntry *addEntry(SPIRVEntry *E) override;
  virtual SPIRVString *getString(const std::string &Str) override;
  virtual SPIRVMemberName *addMemberName(SPIRVTypeStruct *ST,
      SPIRVWord MemberNumber, const std::string &Name) override;
  virtual SPIRVLine *addLine(SPIRVString *FileName, SPIRVWord Line,
      SPIRVWord Column) override;
  virtual void addCapability(SPIRVCapabilityKind) override;
  virtual SPIRVEntry* addModuleProcessed(const std::string&) override;
  virtual std::vector<SPIRVModuleProcessed*> getModuleProcessedVec() override;
  virtual const SPIRVDecorateGeneric *addDecorate(const SPIRVDecorateGeneric *) override;
  virtual SPIRVDecorationGroup *addDecorationGroup() override;
  virtual SPIRVDecorationGroup *addDecorationGroup(SPIRVDecorationGroup *Group) override;
  virtual SPIRVGroupDecorate *addGroupDecorate(SPIRVDecorationGroup *Group,
      const std::vector<SPIRVEntry *> &Targets) override;
  virtual SPIRVGroupDecorateGeneric *addGroupDecorateGeneric(
      SPIRVGroupDecorateGeneric *GDec) override;
  virtual SPIRVGroupMemberDecorate *addGroupMemberDecorate(
      SPIRVDecorationGroup *Group, const std::vector<SPIRVEntry *> &Targets) override;
  virtual void addEntryPoint(SPIRVExecutionModelKind ExecModel, SPIRVId EntryPoint,
                             const std::string& Name,
                             const std::vector<SPIRVId>& Variables) override;
  virtual SPIRVForward *addForward(SPIRVType *Ty) override;
  virtual SPIRVForward *addForward(SPIRVId, SPIRVType *Ty) override;
  virtual SPIRVFunction *addFunction(SPIRVFunction *) override;
  virtual SPIRVFunction *addFunction(SPIRVTypeFunction *, SPIRVId) override;
  virtual SPIRVEntry *replaceForward(SPIRVForward *, SPIRVEntry *) override;

  // Type creation functions
  template<class T> T * addType(T *Ty);
  virtual SPIRVTypeInt *addIntegerType(unsigned BitWidth) override;
  virtual SPIRVTypeBufferSurfaceINTEL *
  addBufferSurfaceINTELType(SPIRVAccessQualifierKind Access) override;

  // Constant creation functions
  virtual SPIRVValue *addConstant(SPIRVValue *) override;
  virtual SPIRVValue *addConstant(SPIRVType *, uint64_t) override;

  // Instruction creation functions
  virtual SPIRVInstruction *addLoopMergeInst(
      SPIRVId MergeBlock, SPIRVId ContinueTarget,
      SPIRVWord LoopControl,
      const std::vector<SPIRVWord>& LoopControlParameters,
      SPIRVBasicBlock *BB) override;
  virtual SPIRVInstruction *
  addInstruction(SPIRVInstruction *Inst, SPIRVBasicBlock *BB,
                 SPIRVInstruction *InsertBefore = nullptr);
  SPIRVEntry *addAsmTargetINTEL(const std::string &) override;
  SPIRVValue *addAsmINTEL(SPIRVTypeFunction *, SPIRVAsmTargetINTEL *,
                          const std::string &, const std::string &) override;
  SPIRVInstruction *addAsmCallINTELInst(SPIRVAsmINTEL *, const std::vector<SPIRVWord> &,
                                   SPIRVBasicBlock *) override;

  virtual SPIRVExtInst* getCompilationUnit() const override
  {
      for (auto& item : IdEntryMap)
      {
          if (item.second->getOpCode() == igc_spv::Op::OpExtInst)
          {
              auto extInst = static_cast<SPIRVExtInst*>(item.second);
              if ((extInst->getExtSetKind() == SPIRVExtInstSetKind::SPIRVEIS_DebugInfo ||
                  extInst->getExtSetKind() == SPIRVExtInstSetKind::SPIRVEIS_OpenCL_DebugInfo_100) &&
                  extInst->getExtOp() == OCLExtOpDbgKind::CompileUnit)
                  return extInst;
          }
      }

      return nullptr;
  }

  virtual std::vector<SPIRVExtInst*> getGlobalVars() override
  {
      std::vector<SPIRVExtInst*> globalVars;

      for (auto& item : IdEntryMap)
      {
          if (item.second->getOpCode() == igc_spv::Op::OpExtInst)
          {
              auto extInst = static_cast<SPIRVExtInst*>(item.second);
              if ((extInst->getExtSetKind() == SPIRVExtInstSetKind::SPIRVEIS_DebugInfo ||
                  extInst->getExtSetKind() == SPIRVExtInstSetKind::SPIRVEIS_OpenCL_DebugInfo_100) &&
                  extInst->getExtOp() == OCLExtOpDbgKind::GlobalVariable)
                  globalVars.push_back(extInst);
          }
      }

      return globalVars;
  }

  virtual std::vector<SPIRVExtInst*> getImportedEntities() override
  {
      std::vector<SPIRVExtInst*> importedEntities;

      for (auto& item : IdEntryMap)
      {
          if (item.second->getOpCode() == igc_spv::Op::OpExtInst)
          {
              auto extInst = static_cast<SPIRVExtInst*>(item.second);
              if ((extInst->getExtSetKind() == SPIRVExtInstSetKind::SPIRVEIS_DebugInfo ||
                  extInst->getExtSetKind() == SPIRVExtInstSetKind::SPIRVEIS_OpenCL_DebugInfo_100) &&
                  extInst->getExtOp() == OCLExtOpDbgKind::ImportedEntity)
              {
                  importedEntities.push_back(extInst);
              }
          }
      }

      return importedEntities;
  }

  virtual std::vector<SPIRVValue*> parseSpecConstants() override
  {
      std::vector<SPIRVValue*> specConstants;

      for (auto& item : IdEntryMap)
      {
          Op opcode = item.second->getOpCode();
          if (opcode == igc_spv::Op::OpSpecConstant ||
              opcode == igc_spv::Op::OpSpecConstantTrue ||
              opcode == igc_spv::Op::OpSpecConstantFalse)
          {
              auto specConstant = static_cast<SPIRVValue*>(item.second);
              specConstants.push_back(specConstant);
          }
      }

      return specConstants;
  }

  // I/O functions
  friend std::istream & operator>>(std::istream &I, SPIRVModule& M);

private:
  SPIRVErrorLog ErrLog;
  SPIRVId NextId;
  SPIRVWord SPIRVVersion;
  SPIRVGeneratorKind SPIRVGenerator;
  SPIRVInstructionSchemaKind InstSchema;
  SpvSourceLanguage SrcLang;
  SPIRVWord SrcLangVer;
  std::set<std::string> SrcExtension;
  std::set<std::string> SPIRVExt;
  SPIRVAddressingModelKind AddrModel;
  SPIRVMemoryModelKind MemoryModel;
  std::string ModuleProcessed;

  typedef std::map<SPIRVId, SPIRVEntry *> SPIRVIdToEntryMap;
  typedef std::map<SPIRVTypeStruct*,
      std::vector<std::pair<unsigned, SPIRVId> > > SPIRVUnknownStructFieldMap;
  typedef std::vector<SPIRVEntry*> SPIRVAliasInstMDVec;
  typedef std::unordered_map<llvm::MDNode*, SPIRVEntry*> SPIRVAliasInstMDMap;
  typedef std::unordered_set<SPIRVEntry *> SPIRVEntrySet;
  typedef std::set<SPIRVId> SPIRVIdSet;
  typedef std::vector<SPIRVId> SPIRVIdVec;
  typedef std::vector<SPIRVFunction *> SPIRVFunctionVector;
  typedef std::vector<SPIRVVariable *> SPIRVVariableVec;
  typedef std::vector<SPIRVString *> SPIRVStringVec;
  typedef std::vector<SPIRVLine *> SPIRVLineVec;
  typedef std::vector<SPIRVDecorationGroup *> SPIRVDecGroupVec;
  typedef std::vector<SPIRVGroupDecorateGeneric *> SPIRVGroupDecVec;
  typedef std::map<SPIRVId, SPIRVExtInstSetKind> SPIRVIdToBuiltinSetMap;
  typedef std::map<SPIRVExecutionModelKind, SPIRVIdSet> SPIRVExecModelIdSetMap;
  typedef std::map<SPIRVExecutionModelKind, SPIRVIdVec> SPIRVExecModelIdVecMap;
  typedef std::unordered_map<std::string, SPIRVString*> SPIRVStringMap;
  typedef std::vector<SPIRVAsmINTEL *> SPIRVAsmVector;
  typedef std::vector<SPIRVEntryPoint*> SPIRVEntryPointVec;

  SPIRVAsmVector AsmVec;
  SPIRVIdToEntryMap IdEntryMap;
  SPIRVIdToEntryMap IdTypeForwardMap; // Forward declared IDs
  SPIRVUnknownStructFieldMap UnknownStructFieldMap;
  SPIRVFunctionVector FuncVec;
  SPIRVVariableVec VariableVec;
  SPIRVEntrySet EntryNoId;         // Entries without id
  SPIRVIdToBuiltinSetMap IdBuiltinMap;
  SPIRVIdSet NamedId;
  SPIRVStringVec StringVec;
  SPIRVLineVec LineVec;
  SPIRVDecorateSet DecorateSet;
  SPIRVDecGroupVec DecGroupVec;
  SPIRVGroupDecVec GroupDecVec;
  SPIRVExecModelIdSetMap EntryPointSet;
  SPIRVEntryPointVec EntryPointVec;
  SPIRVStringMap StrMap;
  SPIRVCapSet CapSet;
  SPIRVSpecConstantMap *SCMap;
  std::map<unsigned, SPIRVTypeInt*> IntTypeMap;
  std::map<unsigned, SPIRVConstant*> LiteralMap;
  SPIRVAliasInstMDVec AliasInstMDVec;
  SPIRVAliasInstMDMap AliasInstMDMap;
  std::vector<SPIRVModuleProcessed*> ModuleProcessedVec;

  void layoutEntry(SPIRVEntry* Entry);
};

SPIRVModuleImpl::~SPIRVModuleImpl() {
    for (const auto &I : IdEntryMap)
        delete I.second;

    for (auto I : EntryNoId)
        delete I;
}

SPIRVLine*
SPIRVModuleImpl::addLine(SPIRVString* FileName,
    SPIRVWord Line, SPIRVWord Column) {
  auto L = add(new SPIRVLine(this, FileName->getId(), Line, Column));
  return L;
}

void
SPIRVModuleImpl::addCapability(SPIRVCapabilityKind Cap) {
  CapSet.insert(Cap);
}

SPIRVEntry* SPIRVModuleImpl::addModuleProcessed(const std::string& Process) {
    ModuleProcessedVec.push_back(new SPIRVModuleProcessed(this, Process));
    return ModuleProcessedVec.back();
}

std::vector<SPIRVModuleProcessed*> SPIRVModuleImpl::getModuleProcessedVec() {
    return ModuleProcessedVec;
}

SPIRVConstant*
SPIRVModuleImpl::getLiteralAsConstant(unsigned Literal) {
  auto Loc = LiteralMap.find(Literal);
  if (Loc != LiteralMap.end())
    return Loc->second;
  auto Ty = addIntegerType(32);
  auto V = new SPIRVConstant(this, Ty, getId(), static_cast<uint64_t>(Literal));
  LiteralMap[Literal] = V;
  addConstant(V);
  return V;
}

void
SPIRVModuleImpl::layoutEntry(SPIRVEntry* E) {
  auto OC = E->getOpCode();
  int IntOC = static_cast<int>(OC);
  switch (IntOC) {
  case OpString:
    addTo(StringVec, E);
    break;
  case OpLine:
    addTo(LineVec, E);
    break;
  case OpVariable: {
    auto BV = static_cast<SPIRVVariable*>(E);
    if (!BV->getParent())
      addTo(VariableVec, E);
    break;
   }
  case OpAliasDomainDeclINTEL:
  case OpAliasScopeDeclINTEL:
  case OpAliasScopeListDeclINTEL: {
      addTo(AliasInstMDVec, E);
      break;
  }
  default:
    break;
  }
}

// Add an entry to the id to entry map.
// Assertion tests if the id is mapped to a different entry.
// Certain entries need to be add to specific collectors to maintain
// logic layout of SPIRV.
SPIRVEntry *
SPIRVModuleImpl::addEntry(SPIRVEntry *Entry) {
    IGC_ASSERT_MESSAGE(Entry, "Invalid entry");
    if (Entry->hasId())
    {
        SPIRVId Id = Entry->getId();
        IGC_ASSERT_MESSAGE(Entry->getId() != SPIRVID_INVALID, "Invalid id");
        SPIRVEntry *Mapped = nullptr;
        if (exist(Id, &Mapped))
        {
            if (Mapped->getOpCode() == OpForward)
            {
                replaceForward(static_cast<SPIRVForward *>(Mapped), Entry);
            }
            else
            {
                IGC_ASSERT_MESSAGE(Mapped == Entry, "Id used twice");
            }
        }
        else
        {
            IdEntryMap[Id] = Entry;
        }
    }
    else
    {
        EntryNoId.insert(Entry);

        // Store the known ID of pointer type that would be declared later.
        if (Entry->getOpCode() == OpTypeForwardPointer)
          IdTypeForwardMap[static_cast<SPIRVTypeForwardPointer*>(Entry)
            ->getPointerId()] = Entry;
    }

    Entry->setModule(this);

    layoutEntry(Entry);
    return Entry;
}

bool
SPIRVModuleImpl::exist(SPIRVId Id) const {
  return exist(Id, nullptr);
}

bool
SPIRVModuleImpl::exist(SPIRVId Id, SPIRVEntry **Entry) const {
  IGC_ASSERT_MESSAGE(Id != SPIRVID_INVALID, "Invalid Id");
  SPIRVIdToEntryMap::const_iterator Loc = IdEntryMap.find(Id);
  if (Loc == IdEntryMap.end())
    return false;
  if (Entry)
    *Entry = Loc->second;
  return true;
}

// If Id is invalid, returns the next available id.
// Otherwise returns the given id and adjust the next available id by increment.
SPIRVId
SPIRVModuleImpl::getId(SPIRVId Id, unsigned increment) {
  if (!isValid(Id))
    Id = NextId;
  else
    NextId = std::max(Id, NextId);
  NextId += increment;
  return Id;
}

SPIRVEntry *
SPIRVModuleImpl::getEntry(SPIRVId Id) const {
  IGC_ASSERT_MESSAGE(Id != SPIRVID_INVALID, "Invalid Id");
  SPIRVIdToEntryMap::const_iterator Loc = IdEntryMap.find(Id);
  if (Loc != IdEntryMap.end()) {
    return Loc->second;
  }
  SPIRVIdToEntryMap::const_iterator LocFwd = IdTypeForwardMap.find(Id);
  if (LocFwd != IdTypeForwardMap.end()) {
    return LocFwd->second;
  }
  IGC_ASSERT_EXIT_MESSAGE(false, "Id is not in map");
  return nullptr;
}

void
SPIRVModuleImpl::addUnknownStructField(
    SPIRVTypeStruct* pStruct, unsigned idx, SPIRVId id)
{
    UnknownStructFieldMap[pStruct].push_back(std::make_pair(idx, id));
}

void SPIRVModuleImpl::resolveUnknownStructFields()
{
    for (auto &kv : UnknownStructFieldMap)
    {
        auto *pStruct = kv.first;
        for (auto &indices : kv.second)
        {
            unsigned idx = indices.first;
            SPIRVId id   = indices.second;

            auto *pTy = static_cast<SPIRVType*>(getEntry(id));
            pStruct->setMemberType(idx, pTy);
        }
    }

    UnknownStructFieldMap.clear();
}

SPIRVExtInstSetKind
SPIRVModuleImpl::getBuiltinSet(SPIRVId SetId) const {
  auto Loc = IdBuiltinMap.find(SetId);
  IGC_ASSERT_EXIT_MESSAGE(Loc != IdBuiltinMap.end(), "Invalid builtin set id");
  return Loc->second;
}

bool
SPIRVModuleImpl::isEntryPoint(SPIRVExecutionModelKind ExecModel, SPIRVId EP)
  const {
  IGC_ASSERT_MESSAGE(isValid(ExecModel), "Invalid execution model");
  IGC_ASSERT_MESSAGE(EP != SPIRVID_INVALID, "Invalid function id");
  auto Loc = EntryPointSet.find(ExecModel);
  if (Loc == EntryPointSet.end())
    return false;
  return (Loc->second.count(EP) > 0);
}

// Module change functions
bool
SPIRVModuleImpl::importBuiltinSet(const std::string& BuiltinSetName,
    SPIRVId *BuiltinSetId) {
  SPIRVId TmpBuiltinSetId = getId();
  if (!importBuiltinSetWithId(BuiltinSetName, TmpBuiltinSetId))
    return false;
  if (BuiltinSetId)
    *BuiltinSetId = TmpBuiltinSetId;
  return true;
}

bool
SPIRVModuleImpl::importBuiltinSetWithId(const std::string& BuiltinSetName,
    SPIRVId BuiltinSetId) {
  SPIRVExtInstSetKind BuiltinSet = SPIRVEIS_Count;
  SPIRVCKRT(SPIRVBuiltinSetNameMap::rfind(BuiltinSetName, &BuiltinSet),
      InvalidBuiltinSetName, "Actual is " + BuiltinSetName);
  IdBuiltinMap[BuiltinSetId] = BuiltinSet;
  return true;
}

void
SPIRVModuleImpl::setAlignment(SPIRVValue *V, SPIRVWord A) {
  V->setAlignment(A);
}

void
SPIRVModuleImpl::setName(SPIRVEntry *E, const std::string &Name) {
  E->setName(Name);
  if (!E->hasId())
    return;
  if (!Name.empty())
    NamedId.insert(E->getId());
  else
    NamedId.erase(E->getId());
}

// Type creation functions
template<class T>
T *
SPIRVModuleImpl::addType(T *Ty) {
  add(Ty);
  if (!Ty->getName().empty())
    setName(Ty, Ty->getName());
  return Ty;
}

SPIRVTypeInt *
SPIRVModuleImpl::addIntegerType(unsigned BitWidth) {
  auto Loc = IntTypeMap.find(BitWidth);
  if (Loc != IntTypeMap.end())
    return Loc->second;
  auto Ty = new SPIRVTypeInt(this, getId(), BitWidth, false);
  IntTypeMap[BitWidth] = Ty;
  return addType(Ty);
}

SPIRVTypeBufferSurfaceINTEL *
SPIRVModuleImpl::addBufferSurfaceINTELType(SPIRVAccessQualifierKind Access) {
  return addType(new SPIRVTypeBufferSurfaceINTEL(this, getId(), Access));
}

SPIRVFunction *
SPIRVModuleImpl::addFunction(SPIRVFunction *Func) {
  FuncVec.push_back(add(Func));
  return Func;
}

SPIRVFunction *
SPIRVModuleImpl::addFunction(SPIRVTypeFunction *FuncType, SPIRVId Id) {
  return addFunction(new SPIRVFunction(this, FuncType,
      getId(Id, FuncType->getNumParameters() + 1)));
}

const SPIRVDecorateGeneric *
SPIRVModuleImpl::addDecorate(const SPIRVDecorateGeneric *Dec) {
  SPIRVId Id = Dec->getTargetId();
  SPIRVEntry *Target = nullptr;
  bool Found = exist(Id, &Target);
  IGC_ASSERT_MESSAGE(Found, "Decorate target does not exist");
  if (!Dec->getOwner())
    DecorateSet.push_back(Dec);
  return Dec;
}

void
SPIRVModuleImpl::addEntryPoint(SPIRVExecutionModelKind ExecModel,
                               SPIRVId EntryPoint, const std::string& Name,
                               const std::vector<SPIRVId>& Variables) {
  IGC_ASSERT_MESSAGE(isValid(ExecModel), "Invalid execution model");
  IGC_ASSERT_MESSAGE(EntryPoint != SPIRVID_INVALID, "Invalid entry point");
  auto* EP =
      add(new SPIRVEntryPoint(this, ExecModel, EntryPoint, Name, Variables));
  EntryPointVec.push_back(EP);
  EntryPointSet[ExecModel].insert(EntryPoint);
}

SPIRVForward *
SPIRVModuleImpl::addForward(SPIRVType *Ty) {
  return add(new SPIRVForward(this, Ty, getId()));
}

SPIRVForward *
SPIRVModuleImpl::addForward(SPIRVId Id, SPIRVType *Ty) {
  return add(new SPIRVForward(this, Ty, Id));
}

SPIRVEntry *
SPIRVModuleImpl::replaceForward(SPIRVForward *Forward, SPIRVEntry *Entry) {
  SPIRVId Id = Entry->getId();
  SPIRVId ForwardId = Forward->getId();
  if (ForwardId == Id)
    IdEntryMap[Id] = Entry;
  else {
    auto Loc = IdEntryMap.find(Id);
    IGC_ASSERT_EXIT(Loc != IdEntryMap.end());
    IdEntryMap.erase(Loc);
    Entry->setId(ForwardId);
    IdEntryMap[ForwardId] = Entry;
  }
  // Annotations include name, decorations, execution modes
  Entry->takeAnnotations(Forward);
  delete Forward;
  return Entry;
}

SPIRVValue *
SPIRVModuleImpl::addConstant(SPIRVValue *C) {
  return add(C);
}

SPIRVValue *
SPIRVModuleImpl::addConstant(SPIRVType *Ty, uint64_t V) {
  if (Ty->isTypeBool()) {
    if (V)
      return new SPIRVConstantTrue(this, Ty, getId());
    else
      return new SPIRVConstantFalse(this, Ty, getId());
  }
  return addConstant(new SPIRVConstant(this, Ty, getId(), V));
}

// Instruction creation functions

SPIRVInstruction *
SPIRVModuleImpl::addInstruction(SPIRVInstruction *Inst, SPIRVBasicBlock *BB,
                                SPIRVInstruction *InsertBefore) {
  if (BB)
    return BB->addInstruction(Inst, InsertBefore);
  if (Inst->getOpCode() != OpSpecConstantOp)
    Inst = createSpecConstantOpInst(Inst);
  return static_cast<SPIRVInstruction *>(addConstant(Inst));
}

SPIRVEntry *SPIRVModuleImpl::addAsmTargetINTEL(const std::string &TheTarget) {
  auto Asm = new SPIRVAsmTargetINTEL(this, getId(), TheTarget);
  return add(Asm);
}

SPIRVValue *SPIRVModuleImpl::addAsmINTEL(SPIRVTypeFunction *TheType,
                                         SPIRVAsmTargetINTEL *TheTarget,
                                         const std::string &TheInstructions,
                                         const std::string &TheConstraints) {
  auto Asm = new SPIRVAsmINTEL(this, TheType, getId(), TheTarget,
                               TheInstructions, TheConstraints);
  AsmVec.push_back(Asm);
  return add(Asm);
}

SPIRVInstruction *
SPIRVModuleImpl::addAsmCallINTELInst(SPIRVAsmINTEL *TheAsm,
                                     const std::vector<SPIRVWord> &TheArguments,
                                     SPIRVBasicBlock *BB) {
  return addInstruction(
      new SPIRVAsmCallINTEL(getId(), TheAsm, TheArguments, BB), BB);
}

SPIRVInstruction *SPIRVModuleImpl::addLoopMergeInst(
  SPIRVId MergeBlock, SPIRVId ContinueTarget, SPIRVWord LoopControl,
  const std::vector<SPIRVWord>& LoopControlParameters, SPIRVBasicBlock *BB) {
   return addInstruction(
     new SPIRVLoopMerge(MergeBlock, ContinueTarget, LoopControl,
       LoopControlParameters, BB),
     BB, const_cast<SPIRVInstruction *>(BB->getTerminateInstr()));
}

template<class T>
void SPIRVModuleImpl::addTo(std::vector<T*>& V, SPIRVEntry* E) {
  V.push_back(static_cast<T *>(E));
}

// The first decoration group includes all the previously defined decorates.
// The second decoration group includes all the decorates defined between the
// first and second decoration group. So long so forth.
SPIRVDecorationGroup*
SPIRVModuleImpl::addDecorationGroup() {
  return addDecorationGroup(new SPIRVDecorationGroup(this, getId()));
}

SPIRVDecorationGroup*
SPIRVModuleImpl::addDecorationGroup(SPIRVDecorationGroup* Group) {
  add(Group);
  Group->takeDecorates(DecorateSet);
  DecGroupVec.push_back(Group);
  IGC_ASSERT(DecorateSet.empty());
  return Group;
}

SPIRVGroupDecorateGeneric*
SPIRVModuleImpl::addGroupDecorateGeneric(SPIRVGroupDecorateGeneric *GDec) {
  add(GDec);
  GDec->decorateTargets();
  GroupDecVec.push_back(GDec);
  return GDec;
}
SPIRVGroupDecorate*
SPIRVModuleImpl::addGroupDecorate(
    SPIRVDecorationGroup* Group, const std::vector<SPIRVEntry*>& Targets) {
  auto GD = new SPIRVGroupDecorate(Group, getIds(Targets));
  addGroupDecorateGeneric(GD);
  return GD;
}

SPIRVGroupMemberDecorate*
SPIRVModuleImpl::addGroupMemberDecorate(
    SPIRVDecorationGroup* Group, const std::vector<SPIRVEntry*>& Targets) {
  auto GMD = new SPIRVGroupMemberDecorate(Group, getIds(Targets));
  addGroupDecorateGeneric(GMD);
  return GMD;
}

SPIRVString*
SPIRVModuleImpl::getString(const std::string& Str) {
  auto Loc = StrMap.find(Str);
  if (Loc != StrMap.end())
    return Loc->second;
  auto S = add(new SPIRVString(this, getId(), Str));
  StrMap[Str] = S;
  return S;
}

SPIRVMemberName*
SPIRVModuleImpl::addMemberName(SPIRVTypeStruct* ST,
    SPIRVWord MemberNumber, const std::string& Name) {
  return add(new SPIRVMemberName(ST, MemberNumber, Name));
}

std::istream &
operator>> (std::istream &I, SPIRVModule &M) {
  SPIRVDecoder Decoder(I, M);
  SPIRVModuleImpl &MI = *static_cast<SPIRVModuleImpl*>(&M);

  SPIRVWord Magic;
  Decoder >> Magic;

  IGC_ASSERT_EXIT_MESSAGE((MagicNumber == Magic), "Invalid magic number");

  Decoder >> MI.SPIRVVersion;

  bool supportVersion =
      MI.SPIRVVersion <= SPIRVVersionSupported::fullyCompliant;

  IGC_ASSERT_EXIT_MESSAGE(supportVersion, "Unsupported SPIRV version number");

  Decoder >> MI.SPIRVGenerator;

  // Bound for Id
  Decoder >> MI.NextId;

  Decoder >> MI.InstSchema;
  IGC_ASSERT_MESSAGE(MI.InstSchema == SPIRVISCH_Default, "Unsupported instruction schema");

  while (Decoder.getWordCountAndOpCode()) {
    if (!Decoder.getEntry()){
      if(M.getErrorLog().getErrorCode() == SPIRVEC_UnsupportedSPIRVOpcode)
        break;
    }
  }

  return I;
}

SPIRVModule *
SPIRVModule::createSPIRVModule() {
  return new SPIRVModuleImpl;
}

SPIRVValue *
SPIRVModuleImpl::getValue(SPIRVId TheId)const {
  return get<SPIRVValue>(TheId);
}

SPIRVType *
SPIRVModuleImpl::getValueType(SPIRVId TheId)const {
  return get<SPIRVValue>(TheId)->getType();
}

std::vector<SPIRVValue *>
SPIRVModuleImpl::getValues(const std::vector<SPIRVId>& IdVec)const {
  std::vector<SPIRVValue *> ValueVec;
  for (auto i:IdVec)
    ValueVec.push_back(getValue(i));
  return ValueVec;
}

std::vector<SPIRVType *>
SPIRVModuleImpl::getValueTypes(const std::vector<SPIRVId>& IdVec)const {
  std::vector<SPIRVType *> TypeVec;
  for (auto i:IdVec)
    TypeVec.push_back(getValue(i)->getType());
  return TypeVec;
}

std::vector<SPIRVId>
SPIRVModuleImpl::getIds(const std::vector<SPIRVEntry *> &ValueVec)const {
  std::vector<SPIRVId> IdVec;
  for (auto i:ValueVec)
    IdVec.push_back(i->getId());
  return IdVec;
}

std::vector<SPIRVId>
SPIRVModuleImpl::getIds(const std::vector<SPIRVValue *> &ValueVec)const {
  std::vector<SPIRVId> IdVec;
  for (auto i:ValueVec)
    IdVec.push_back(i->getId());
  return IdVec;
}

SPIRVDbgInfo::SPIRVDbgInfo(SPIRVModule *TM)
:M(TM){
}

std::string
SPIRVDbgInfo::getFunctionFileStr(SPIRVFunction *F) {
  if (F->hasLine())
    return F->getLine()->getFileNameStr();
  return "";
}

unsigned
SPIRVDbgInfo::getFunctionLineNo(SPIRVFunction *F) {
  if (F->hasLine())
    return F->getLine()->getLine();
  return 0;
}

bool IsSPIRVBinary(const std::string &Img) {
  if (Img.size() < sizeof(unsigned))
    return false;
  auto Magic = reinterpret_cast<const unsigned*>(Img.data());
  return *Magic == MagicNumber;
}

}

