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

// This file implements base class for SPIR-V entities.

#include "SPIRVEntry.h"
#include "SPIRVFunction.h"
#include "SPIRVInstruction.h"
#include "SPIRVMemAliasingINTEL.h"
#include "SPIRVAsm.h"
#include "Probe/Assertion.h"

namespace igc_spv{

template<typename T>
SPIRVEntry* create() {
  return new T();
}

SPIRVEntry *
SPIRVEntry::create(Op OpCode) {
  switch (OpCode) {
#define _SPIRV_OP(x,...) case Op##x: return igc_spv::create<SPIRV##x>();
#include "SPIRVOpCodeEnum.h"
#undef _SPIRV_OP
  default:
    break;
  }
  return 0;
}

SPIRVErrorLog &
SPIRVEntry::getErrorLog()const {
  return Module->getErrorLog();
}

bool
SPIRVEntry::exist(SPIRVId TheId)const {
  return Module->exist(TheId);
}

SPIRVEntry *
SPIRVEntry::getOrCreate(SPIRVId TheId)const {
  SPIRVEntry *Entry = nullptr;
  bool Found = Module->exist(TheId, &Entry);
  if (!Found)
    return Module->addForward(TheId, nullptr);
  return Entry;
}

SPIRVValue *
SPIRVEntry::getValue(SPIRVId TheId)const {
  return get<SPIRVValue>(TheId);
}

SPIRVType *
SPIRVEntry::getValueType(SPIRVId TheId)const {
  return get<SPIRVValue>(TheId)->getType();
}

SPIRVDecoder
SPIRVEntry::getDecoder(std::istream& I){
  return SPIRVDecoder(I, *Module);
}

void
SPIRVEntry::setWordCount(SPIRVWord TheWordCount){
  WordCount = TheWordCount;
}

void
SPIRVEntry::setName(const std::string& TheName) {
  Name = TheName;
}

void
SPIRVEntry::setModule(SPIRVModule *TheModule) {
  IGC_ASSERT_MESSAGE(TheModule, "Invalid module");
  if (TheModule == Module)
    return;
  IGC_ASSERT_MESSAGE(Module == NULL, "Cannot change owner of entry");
  Module = TheModule;
}

// Read words from SPIRV binary and create members for SPIRVEntry.
// The word count and op code has already been read before calling this
// function for creating the SPIRVEntry. Therefore the input stream only
// contains the remaining part of the words for the SPIRVEntry.
void
SPIRVEntry::decode(std::istream &I) {
  IGC_ASSERT_EXIT_MESSAGE(0, "Not implemented");
}

std::vector<SPIRVValue *>
SPIRVEntry::getValues(const std::vector<SPIRVId>& IdVec)const {
  std::vector<SPIRVValue *> ValueVec;
  for (auto i:IdVec)
    ValueVec.push_back(getValue(i));
  return ValueVec;
}

std::vector<SPIRVType *>
SPIRVEntry::getValueTypes(const std::vector<SPIRVId>& IdVec)const {
  std::vector<SPIRVType *> TypeVec;
  for (auto i:IdVec)
    TypeVec.push_back(getValue(i)->getType());
  return TypeVec;
}

std::vector<SPIRVId>
SPIRVEntry::getIds(const std::vector<SPIRVValue *> ValueVec)const {
  std::vector<SPIRVId> IdVec;
  for (auto i:ValueVec)
    IdVec.push_back(i->getId());
  return IdVec;
}

SPIRVEntry *
SPIRVEntry::getEntry(SPIRVId TheId) const {
  return Module->getEntry(TheId);
}

void
SPIRVEntry::validateFunctionControlMask(SPIRVWord TheFCtlMask)
  const {
   SPIRVCK(TheFCtlMask <= (unsigned)SPIRVFunctionControlMaskKind::FunctionControlMaskMax,
      InvalidFunctionControlMask, "");
}

void
SPIRVEntry::validateValues(const std::vector<SPIRVId> &Ids)const {
  for (auto I:Ids)
    getValue(I)->validate();
}

void
SPIRVEntry::validateBuiltin(SPIRVWord TheSet, SPIRVWord Index)const {
  IGC_ASSERT_MESSAGE(TheSet != SPIRVWORD_MAX, "Invalid builtin");
  IGC_ASSERT_MESSAGE(Index != SPIRVWORD_MAX, "Invalid builtin");
}

void
SPIRVEntry::addDecorate(const SPIRVDecorate *Dec){
  Decorates.insert(std::make_pair(Dec->getDecorateKind(), Dec));
  Module->addDecorate(Dec);
}

void
SPIRVEntry::addDecorate(Decoration Kind) {
  addDecorate(new SPIRVDecorate(Kind, this));
}

void
SPIRVEntry::addDecorate(Decoration Kind, SPIRVWord Literal) {
    switch (static_cast<int>(Kind)) {
    case DecorationAliasScopeINTEL:
    case DecorationNoAliasINTEL:
        addDecorate(new SPIRVDecorateId(Kind, this, Literal));
        return;
    default:
        addDecorate(new SPIRVDecorate(Kind, this, Literal));
    }
}

void
SPIRVEntry::addDecorate(SPIRVDecorateId* Dec) {
    DecorateIds.insert(std::make_pair(Dec->getDecorateKind(), Dec));
    Module->addDecorate(Dec);
    SPIRVDBG(spvdbgs() << "[addDecorateId] " << *Dec << '\n';)
}

void
SPIRVEntry::eraseDecorate(Decoration Dec){
  Decorates.erase(Dec);
}

void
SPIRVEntry::takeDecorates(SPIRVEntry *E){
  Decorates = std::move(E->Decorates);
}

void
SPIRVEntry::takeDecorateIds(SPIRVEntry* E) {
    DecorateIds = std::move(E->DecorateIds);
    SPIRVDBG(spvdbgs() << "[takeDecorateIds] " << Id << '\n';)
}

void
SPIRVEntry::setLine(SPIRVLine *L){
  Line = L;
}

void SPIRVEntry::setDIScope(SPIRVExtInst* I) {
    diScope = I;
}

SPIRVExtInst* SPIRVEntry::getDIScope() {
    return diScope;
}

void
SPIRVEntry::takeLine(SPIRVEntry *E){
  Line = E->Line;
  if (Line == nullptr)
    return;
  E->Line = nullptr;
}

void
SPIRVEntry::addMemberDecorate(const SPIRVMemberDecorate *Dec){
  IGC_ASSERT(canHaveMemberDecorates() && MemberDecorates.find(Dec->getPair()) ==
      MemberDecorates.end());
  MemberDecorates[Dec->getPair()] = Dec;
  Module->addDecorate(Dec);
}

void
SPIRVEntry::addMemberDecorate(SPIRVWord MemberNumber, Decoration Kind) {
  addMemberDecorate(new SPIRVMemberDecorate(Kind, MemberNumber, this));
}

void
SPIRVEntry::addMemberDecorate(SPIRVWord MemberNumber, Decoration Kind,
    SPIRVWord Literal) {
  addMemberDecorate(new SPIRVMemberDecorate(Kind, MemberNumber, this, Literal));
}

void
SPIRVEntry::eraseMemberDecorate(SPIRVWord MemberNumber, Decoration Dec){
  MemberDecorates.erase(std::make_pair(MemberNumber, Dec));
}

void
SPIRVEntry::takeMemberDecorates(SPIRVEntry *E){
  MemberDecorates = std::move(E->MemberDecorates);
}

void
SPIRVEntry::takeAnnotations(SPIRVForward *E){
  Module->setName(this, E->getName());
  takeDecorates(E);
  takeDecorateIds(E);
  takeMemberDecorates(E);
  takeLine(E);
  if (OpCode == OpFunction)
    static_cast<SPIRVFunction *>(this)->takeExecutionModes(E);
}

// Check if an entry has Kind of decoration and get the literal of the
// first decoration of such kind at Index.
bool
SPIRVEntry::hasDecorate(Decoration Kind, size_t Index, SPIRVWord *Result)const {
  DecorateMapType::const_iterator Loc = Decorates.find(Kind);
  if (Loc == Decorates.end())
    return false;

  if (Result)
  {
    *Result = Loc->second->getLiteral(Index);
  }

  return true;
}

bool SPIRVEntry::hasDecorateId(Decoration Kind, size_t Index,
    SPIRVId* Result) const {
    auto Loc = DecorateIds.find(Kind);
    if (Loc == DecorateIds.end())
        return false;
    if (Result)
        *Result = Loc->second->getLiteral(Index);
    return true;
}

// Get literals of all decorations of Kind at Index.
std::set<SPIRVWord>
SPIRVEntry::getDecorate(Decoration Kind, size_t Index) const {
  auto Range = Decorates.equal_range(Kind);
  std::set<SPIRVWord> Value;
  for (auto I = Range.first, E = Range.second; I != E; ++I) {
    IGC_ASSERT_MESSAGE(Index < I->second->getLiteralCount(), "Invalid index");
    Value.insert(I->second->getLiteral(Index));
  }
  return Value;
}

std::vector<SPIRVDecorate const *>
SPIRVEntry::getDecorations(Decoration Kind) const {
  auto Range = Decorates.equal_range(Kind);
  std::vector<SPIRVDecorate const *> Decors;
  Decors.reserve(Decorates.count(Kind));
  for (auto I = Range.first, E = Range.second; I != E; ++I) {
    Decors.push_back(I->second);
  }
  return Decors;
}

std::vector<SPIRVId>
SPIRVEntry::getDecorationIdLiterals(Decoration Kind) const {
    auto Loc = DecorateIds.find(Kind);
    if (Loc == DecorateIds.end())
        return {};

    return (Loc->second->getVecLiteral());
}

std::set<SPIRVId> SPIRVEntry::getDecorateId(Decoration Kind,
    size_t Index) const {
    auto Range = DecorateIds.equal_range(Kind);
    std::set<SPIRVId> Value;
    for (auto I = Range.first, E = Range.second; I != E; ++I) {
        assert(Index < I->second->getLiteralCount() && "Invalid index");
        Value.insert(I->second->getLiteral(Index));
    }
    return Value;
}

std::vector<SPIRVDecorateId const*>
SPIRVEntry::getDecorationIds(Decoration Kind) const {
    auto Range = DecorateIds.equal_range(Kind);
    std::vector<SPIRVDecorateId const*> Decors;
    Decors.reserve(DecorateIds.count(Kind));
    for (auto I = Range.first, E = Range.second; I != E; ++I) {
        Decors.push_back(I->second);
    }
    return Decors;
}

std::vector<std::string>
SPIRVEntry::getDecorationStringLiteral(Decoration Kind) const {
  auto Loc = Decorates.equal_range(Kind);
  if (Loc.first == Decorates.end())
    return {};

  std::vector<std::string> DecorationStrings;
  for (auto it = Loc.first; it != Loc.second; ++it) {
    auto v = getVecString(it->second->getVecLiteral());
    DecorationStrings.insert(DecorationStrings.end(), v.begin(), v.end());
  }
  return DecorationStrings;
}

bool
SPIRVEntry::hasLinkageType() const {
  return OpCode == OpFunction || OpCode == OpVariable;
}

SPIRVLinkageTypeKind
SPIRVEntry::getLinkageType() const {
  auto hasLinkageAttr = [&](SPIRVWord *Result)
  {
      auto Loc = Decorates.find(DecorationLinkageAttributes);
      if (Loc == Decorates.end())
          return false;

      if (Result)
      {
          auto *Dec = Loc->second;
          // Linkage Attributes has an arbitrary width string to start.  The
          // last Word is the linkage type.
          *Result = Dec->getLiteral(Dec->getLiteralCount() - 1);
      }

      return true;
  };

  IGC_ASSERT(hasLinkageType());
  SPIRVWord LT = SPIRVLinkageTypeKind::LinkageTypeCount;
  if (!hasLinkageAttr(&LT))
     return SPIRVLinkageTypeKind::LinkageTypeInternal;
  return static_cast<SPIRVLinkageTypeKind>(LT);
}

void
SPIRVEntry::setLinkageType(SPIRVLinkageTypeKind LT) {
  IGC_ASSERT(isValid(LT));
  IGC_ASSERT(hasLinkageType());
  addDecorate(new SPIRVDecorate(DecorationLinkageAttributes, this, LT));
}

std::istream &
operator>>(std::istream &I, SPIRVEntry &E) {
  E.decode(I);
  return I;
}

SPIRVEntryPoint::SPIRVEntryPoint(SPIRVModule *TheModule,
  SPIRVExecutionModelKind TheExecModel, SPIRVId TheId,
  const std::string &TheName)
  :SPIRVAnnotation(TheModule->get<SPIRVFunction>(TheId),
   getSizeInWords(TheName) + 3), ExecModel(TheExecModel), Name(TheName){
}

void
SPIRVEntryPoint::decode(std::istream &I) {
  getDecoder(I) >> ExecModel >> Target >> Name;
  Module->setName(getOrCreateTarget(), Name);
  Module->addEntryPoint(ExecModel, Target);
}

void
SPIRVExecutionMode::decode(std::istream &I) {
  getDecoder(I) >> Target >> ExecMode;
  switch(ExecMode) {
  case SPIRVExecutionModeKind::ExecutionModeLocalSize:
  case SPIRVExecutionModeKind::ExecutionModeLocalSizeHint:
    WordLiterals.resize(3);
    break;
  case SPIRVExecutionModeKind::ExecutionModeInvocations:
  case SPIRVExecutionModeKind::ExecutionModeOutputVertices:
  case SPIRVExecutionModeKind::ExecutionModeVecTypeHint:
  case SPIRVExecutionModeKind::ExecutionModeSubgroupSize:
  case SPIRVExecutionModeKind::ExecutionModeSubgroupsPerWorkgroup:
    WordLiterals.resize(1);
    break;
  default:
    // Do nothing. Keep this to avoid VS2013 warning.
    break;
  }
  getDecoder(I) >> WordLiterals;
  getOrCreateTarget()->addExecutionMode(this);
}

SPIRVForward *
SPIRVAnnotationGeneric::getOrCreateTarget()const {
  SPIRVEntry *Entry = nullptr;
  bool Found = Module->exist(Target, &Entry);
  IGC_ASSERT_MESSAGE((!Found || Entry->getOpCode() == OpForward), "Annotations only allowed on forward");
  if (!Found)
    Entry = Module->addForward(Target, nullptr);
  return static_cast<SPIRVForward *>(Entry);
}

SPIRVName::SPIRVName(const SPIRVEntry *TheTarget, const std::string& TheStr)
  :SPIRVAnnotation(TheTarget, getSizeInWords(TheStr) + 2), Str(TheStr){
}

void
SPIRVName::decode(std::istream &I) {
  getDecoder(I) >> Target >> Str;
  Module->setName(getOrCreateTarget(), Str);
}

void
SPIRVName::validate() const {
  IGC_ASSERT_MESSAGE(WordCount == getSizeInWords(Str) + 2, "Incorrect word count");
}

_SPIRV_IMP_DEC2(SPIRVString, Id, Str)
_SPIRV_IMP_DEC3(SPIRVMemberName, Target, MemberNumber, Str)

void
SPIRVLine::decode(std::istream &I) {
  getDecoder(I) >> FileName >> Line >> Column;
}

void
SPIRVLine::validate() const {
  IGC_ASSERT(OpCode == OpLine);
  IGC_ASSERT(WordCount == 5);
  IGC_ASSERT(get<SPIRVEntry>(FileName)->getOpCode() == OpString);
  IGC_ASSERT(Line != SPIRVWORD_MAX);
  IGC_ASSERT(Column != SPIRVWORD_MAX);
}

void
SPIRVNoLine::decode(std::istream &I) {
}

void
SPIRVNoLine::validate() const {
    IGC_ASSERT(OpCode == OpNoLine);
    IGC_ASSERT(WordCount == 1);
}

void
SPIRVMemberName::validate() const {
  IGC_ASSERT(OpCode == OpMemberName);
  IGC_ASSERT(WordCount == getSizeInWords(Str) + FixedWC);
  IGC_ASSERT(get<SPIRVEntry>(Target)->getOpCode() == OpTypeStruct);
  IGC_ASSERT(MemberNumber < get<SPIRVTypeStruct>(Target)->getStructMemberCount());
}

SPIRVExtInstImport::SPIRVExtInstImport(SPIRVModule *TheModule, SPIRVId TheId,
    const std::string &TheStr):
  SPIRVEntry(TheModule, 2 + getSizeInWords(TheStr), OC, TheId), Str(TheStr){
  validate();
}

void
SPIRVExtInstImport::decode(std::istream &I) {
  getDecoder(I) >> Id >> Str;
  Module->importBuiltinSetWithId(Str, Id);
}

void
SPIRVExtInstImport::validate() const {
  SPIRVEntry::validate();
  IGC_ASSERT_MESSAGE(!Str.empty(), "Invalid builtin set");
}

void
SPIRVMemoryModel::decode(std::istream &I) {
  SPIRVAddressingModelKind AddrModel;
  SPIRVMemoryModelKind MemModel;
  getDecoder(I) >> AddrModel >> MemModel;
  Module->setAddressingModel(AddrModel);
  Module->setMemoryModel(MemModel);
}

void
SPIRVMemoryModel::validate() const {
  unsigned AM = Module->getAddressingModel();
  unsigned MM = Module->getMemoryModel();
  SPIRVCK(AM < SPIRVAddressingModelKind::AddressingModelCount, InvalidAddressingModel, "Actual is " + AM);
  SPIRVCK(MM < SPIRVMemoryModelKind::MemoryModelCount, InvalidMemoryModel, "Actual is " + MM);
}

void
SPIRVSource::decode(std::istream &I) {
  SpvSourceLanguage Lang = SpvSourceLanguageUnknown;
  SPIRVWord Ver = SPIRVWORD_MAX;
  getDecoder(I) >> Lang >> Ver;
  Module->setSourceLanguage(Lang, Ver);
}

SPIRVSourceExtension::SPIRVSourceExtension(SPIRVModule *M,
    const std::string &SS) : SPIRVEntryNoId(M, 1 + getSizeInWords(SS)), S(SS){}

void
SPIRVSourceExtension::decode(std::istream &I) {
  getDecoder(I) >> S;
  Module->getSourceExtension().insert(S);
}

SPIRVExtension::SPIRVExtension(SPIRVModule *M, const std::string &SS)
  :SPIRVEntryNoId(M, 1 + getSizeInWords(SS)), S(SS){}

void
SPIRVExtension::decode(std::istream &I) {
  getDecoder(I) >> S;
  Module->getExtension().insert(S);
}

SPIRVCapability::SPIRVCapability(SPIRVModule *M, SPIRVCapabilityKind K)
  :SPIRVEntryNoId(M, 2), Kind(K){
}

void
SPIRVCapability::decode(std::istream &I) {
  getDecoder(I) >> Kind;
  Module->addCapability(Kind);
}

void
SPIRVModuleProcessed::decode(std::istream &I) {
    getDecoder(I) >> S;
    Module->setModuleProcessed(S);
}

template <igc_spv::Op OC> void SPIRVContinuedInstINTELBase<OC>::validate() const {
    SPIRVEntry::validate();
}

template <igc_spv::Op OC>
void SPIRVContinuedInstINTELBase<OC>::decode(std::istream& I) {
    SPIRVEntry::getDecoder(I) >> (Elements);
}

SPIRVType* SPIRVTypeStructContinuedINTEL::getMemberType(size_t I) const {
    return static_cast<SPIRVType*>(SPIRVEntry::getEntry(Elements[I]));
}

} // namespace igc_spv

