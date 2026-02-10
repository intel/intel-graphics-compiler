/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

// clang-format off
#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "common/LLVMWarningsPop.hpp"
// clang-format on

#include "VISAModule.hpp"
#include "LexicalScopes.hpp"
#include "Utils.hpp"
#include "VISADebugInfo.hpp"

#include <algorithm>
#include <cstddef>
#include <unordered_map>
#include <vector>

#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

void VISAVariableLocation::dump() const { print(llvm::dbgs()); }

void VISAVariableLocation::print(raw_ostream &OS) const {
  OS << "   - VarLoc = { ";
  if (IsImmediate()) {
    OS << "Imm: ";
    m_pConstVal->print(OS, true);
  } else if (HasSurface()) {
    auto PrintExtraSurfaceType = [this](raw_ostream &OS) {
      if (IsSLM())
        OS << "SLM";
      if (IsTexture())
        OS << "Texture";
      if (IsSampler())
        OS << "Sampler";
      if (!IsSLM() && !IsTexture() && !IsSampler())
        OS << "Unknown";
    };
    if (!HasLocation()) {
      // Simple surface entry
      OS << "Type: SimpleSurface, "
         << "SurfaceReg: " << m_surfaceReg << ", Extra: ";
      PrintExtraSurfaceType(OS);
    } else {
      // Surface entry + offset
      OS << "Type: Surface, "
         << "SurfaceReg: " << m_surfaceReg;
      // Simple surface entry
      OS << "Type: SimpleSurface, "
         << "SurfaceReg: " << m_surfaceReg;
      if (m_isRegister) {
        OS << ", Offset: [VReg=" << m_locationReg << "]";
        if (HasLocationSecondReg()) {
          OS << ", [VReg2=" << m_locationSecondReg << "]";
        }
      } else {
        OS << ", Offset: " << m_locationOffset;
      }
      OS << ", IsMem: " << m_isInMemory;
      OS << ", Vectorized: ";
      if (m_isVectorized)
        OS << "v" << m_vectorNumElements;
      else
        OS << "false";
      OS << ", Extra: ";
      PrintExtraSurfaceType(OS);
    }
  } else if (HasLocation()) {
    // Address/Register location
    if (m_isInMemory) {
      OS << "Type: InMem";
      OS << ", Loc:";
      if (m_isRegister)
        OS << "[VReg=" << m_locationReg << "]";
      else
        OS << "[Offset=" << m_locationOffset << "]";
      OS << ", GlobalASID: " << m_isGlobalAddrSpace;
    } else {
      OS << "Type: Value";
      OS << ", VReg: " << m_locationReg;
    }
    OS << ", Vectorized: ";
    if (m_isVectorized)
      OS << "v" << m_vectorNumElements;
    else
      OS << "false";
  } else {
    std::array<std::pair<const char *, bool>, 7> Props = {{{"IsImmediate:", IsImmediate()},
                                                           {"HasSurface:", HasSurface()},
                                                           {"HasLocation:", HasLocation()},
                                                           {"IsInMemory:", IsInMemory()},
                                                           {"IsRegister:", IsRegister()},
                                                           {"IsVectorized:", IsVectorized()},
                                                           {"IsInGlobalAddressSpace:", IsInGlobalAddrSpace()}}};
    if (std::all_of(Props.begin(), Props.end(), [](const auto &Item) { return Item.second == false; })) {
      OS << "empty";
    } else {
      OS << "UNEXPECTED_FORMAT: true, ";
      OS << "FORMAT: { ";
      for (const auto &Prop : Props) {
        OS << Prop.first << ": " << Prop.second << ", ";
      }
      OS << " }";
    }
  }
  OS << " }\n";
}

void VISAModule::BeginInstruction(Instruction *pInst) {
  IGC_ASSERT_MESSAGE(!m_instInfoMap.count(pInst), "Instruction emitted twice!");
  // Assume VISA Id was updated by this point, validate that.
  ValidateVisaId();
  unsigned int nextVISAInstId = m_currentVisaId + 1;
  m_instInfoMap[pInst] = InstructionInfo(INVALID_SIZE, nextVISAInstId);
  m_instList.push_back(pInst);

  if (IsCatchAllIntrinsic(pInst)) {
    m_catchAllVisaId = nextVISAInstId;
  }
}

void VISAModule::EndInstruction(Instruction *pInst) {
  IGC_ASSERT_MESSAGE(m_instList.size() > 0, "Trying to end Instruction other than the last one called with begin!");
  IGC_ASSERT_MESSAGE(m_instList.back() == pInst,
                     "Trying to end Instruction other than the last one called with begin!");
  IGC_ASSERT_MESSAGE(m_instInfoMap.count(pInst), "Trying to end instruction more than once!");
  IGC_ASSERT_MESSAGE(m_instInfoMap[pInst].m_size == INVALID_SIZE, "Trying to end instruction more than once!");

  // Assume VISA Id was updated by this point, validate that.
  ValidateVisaId();

  unsigned currInstOffset = m_instInfoMap[pInst].m_offset;
  unsigned nextInstOffset = m_currentVisaId + 1;
  m_instInfoMap[m_instList.back()].m_size = nextInstOffset - currInstOffset;
}

void VISAModule::BeginEncodingMark() { ValidateVisaId(); }

void VISAModule::EndEncodingMark() { UpdateVisaId(); }

bool VISAModule::HasVisaOffset(const llvm::Instruction *pInst) const {
  return m_instInfoMap.find(pInst) != m_instInfoMap.end();
}

unsigned int VISAModule::GetVisaOffset(const llvm::Instruction *pInst) const {
  InstInfoMap::const_iterator itr = m_instInfoMap.find(pInst);
  IGC_ASSERT_MESSAGE(itr != m_instInfoMap.end(), "Invalid Instruction");
  return itr->second.m_offset;
}

const Module *VISAModule::GetModule() const { return m_Func->getParent(); }

const Function *VISAModule::GetEntryFunction() const { return m_Func; }

const LLVMContext &VISAModule::GetContext() const { return GetModule()->getContext(); }

const std::string VISAModule::GetDataLayout() const { return GetModule()->getDataLayout().getStringRepresentation(); }

const std::string &VISAModule::GetTargetTriple() const { return m_triple; }

bool VISAModule::IsExecutableInst(const llvm::Instruction &inst) {
  // Return false if inst is dbg info intrinsic or if it is
  // catch all intrinsic. In both of these cases, we dont want
  // to emit associated debug loc since there is no machine
  // code generated for them.
  if (IsCatchAllIntrinsic(&inst))
    return false;

  if (llvm::isa<DbgInfoIntrinsic>(inst))
    return false;

  return true;
}

void VISAModule::rebuildVISAIndexes() {

  VisaIndexToInst.clear();
  VisaIndexToVisaSizeIndex.clear();
  for (VISAModule::const_iterator II = begin(), IE = end(); II != IE; ++II) {
    const Instruction *pInst = *II;

    // store VISA mapping only if pInst generates Gen code
    if (!IsExecutableInst(*pInst))
      continue;

    InstInfoMap::const_iterator itr = m_instInfoMap.find(pInst);
    if (itr == m_instInfoMap.end())
      continue;

    // No VISA instruction emitted corresponding to this llvm IR instruction.
    // Typically happens with cast instructions.
    if (itr->second.m_size == 0)
      continue;

    unsigned int currOffset = itr->second.m_offset;
    VisaIndexToInst.insert(std::make_pair(currOffset, pInst));
    unsigned int currSize = itr->second.m_size;
    for (auto VI = currOffset, E = (currOffset + currSize); VI != E; ++VI)
      VisaIndexToVisaSizeIndex[VI] = VisaSizeIndex{currOffset, currSize};
  }
}

// This function returns a vector of tuples. Each tuple corresponds to a call
// site where physical register startRegNum is saved. Tuple format: <start IP,
// end IP, stack offset>
//
// startIP - %ip where startRegNum is available on BE stack,
// endIP - %ip where startRegNum is available in original location (GRF),
// stack offset - location on BE stack between [startIP - endIP)
//
// This function is called to compute caller save of 1 sub-interval genIsaRange.
// The sub-interval genIsaRange could pass over 0 or more stack call functions.
// A tuple is created for every stack call site that requires save/restore of
// startRegNum.
//
// It is assumed that if startRegNum is within caller save area then entire
// variable is in caller save area.
std::vector<std::tuple<uint64_t, uint64_t, unsigned int>>
VISAModule::getAllCallerSave(const VISAObjectDebugInfo &VDI, uint64_t startRange, uint64_t endRange,
                             DbgDecoder::LiveIntervalsVISA &genIsaRange) const {
  std::vector<std::tuple<uint64_t, uint64_t, unsigned int>> callerSaveIPs;

  if (VDI.getCFI().callerSaveEntry.empty())
    return callerSaveIPs;

  if (!genIsaRange.isGRF())
    return callerSaveIPs;

  auto startRegNum = genIsaRange.getGRF().regNum;

  // There are valid entries in caller save data structure
  unsigned int prevSize = 0;
  bool inCallerSaveSection = false;
  std::vector<DbgDecoder::PhyRegSaveInfoPerIP> saves;
  const auto &CFI = VDI.getCFI();
  auto callerSaveStartIt = CFI.callerSaveEntry.end();

  for (auto callerSaveIt = CFI.callerSaveEntry.begin(); callerSaveIt != CFI.callerSaveEntry.end(); ++callerSaveIt) {
    auto &callerSave = (*callerSaveIt);
    if (prevSize > 0 && prevSize > callerSave.numEntries && !inCallerSaveSection) {
      // It means previous there was a call instruction
      // between prev and current instruction.
      callerSaveStartIt = callerSaveIt;
      --callerSaveStartIt;
      inCallerSaveSection = true;
    }

    if ((*callerSaveIt).numEntries == 0 && inCallerSaveSection) {
      uint64_t callerSaveIp = (*callerSaveStartIt).genIPOffset + VDI.getRelocOffset();
      uint64_t callerRestoreIp = (*callerSaveIt).genIPOffset + VDI.getRelocOffset();
      // End of current caller save section
      if (startRange < callerSaveIp) {
        callerRestoreIp = std::min<uint64_t>(endRange, callerRestoreIp);
        // Variable is live over stack call function.
        for (const auto &callerSaveReg : (*callerSaveStartIt).data) {
          // startRegNum is saved to caller save area around the stack call.
          if ((callerSaveReg.srcRegOff / getGRFSizeInBytes()) == startRegNum) {
            // Emit caller save/restore only if %ip is within range
            callerSaveIPs.emplace_back(
                std::make_tuple(callerSaveIp, callerRestoreIp, (unsigned int)callerSaveReg.dst.m.memoryOffset));
            inCallerSaveSection = false;
            break;
          }
        }
      }
    }

    prevSize = callerSave.numEntries;
  }

  return callerSaveIPs;
}

void VISAModule::coalesceRanges(GenISARange &GenISARange) {
  // Treat 2 sub-intervals as coalesceable as long %ip end of first interval
  // and %ip start of second interval is within a threshold.
  // 0x10 is equivalent to 1 asm instruction.
  const unsigned int CoalescingThreshold = 0x0;

  class Comp {
  public:
    bool operator()(const std::pair<unsigned int, unsigned int> &a, const std::pair<unsigned int, unsigned int> &b) {
      return a.first < b.first;
    }
  } Comp;

  if (GenISARange.size() == 0)
    return;

  std::sort(GenISARange.begin(), GenISARange.end(), Comp);

  for (unsigned int i = 0; i != GenISARange.size() - 1; i++) {
    if (GenISARange[i].first == (unsigned int)-1 && GenISARange[i].second == (unsigned int)-1)
      continue;

    for (unsigned int j = i + 1; j != GenISARange.size(); j++) {
      if (GenISARange[j].first == (unsigned int)-1 && GenISARange[j].second == (unsigned int)-1)
        continue;

      if (GenISARange[j].first >= GenISARange[i].second &&
          GenISARange[j].first <= (CoalescingThreshold + GenISARange[i].second)) {
        GenISARange[i].second = GenISARange[j].second;
        GenISARange[j].first = (unsigned int)-1;
        GenISARange[j].second = (unsigned int)-1;
      }
    }
  }

  GenISARange.erase(std::remove_if(GenISARange.begin(), GenISARange.end(),
                                   [](const auto &Range) { return Range.first == -1 && Range.second == -1; }),
                    GenISARange.end());
}

void VISAModule::print(raw_ostream &OS) const {

  OS << "[DBG] VisaModule\n";

  OS << "  --- VisaIndexToInst Dump\n";
  OrderedTraversal(VisaIndexToInst, [&OS](const auto &VisaIdx, const auto &Inst) {
    OS << "    VI2Inst: " << VisaIdx << " ->  inst: " << *Inst << "\n";
  });
  OS << "  ___\n";

  OS << "  --- VISAIndexToSize Dump\n";
  OrderedTraversal(VisaIndexToVisaSizeIndex, [&OS](const auto &VisaIdx, const auto &VisaInterval) {
    OS << "    VI2Size: " << VisaIdx << " -> {offset: " << VisaInterval.VisaOffset
       << ", size: " << VisaInterval.VisaInstrNum << "}\n";
  });
  OS << "  ___\n";
}

const llvm::Instruction *getNextInst(const llvm::Instruction *start) {
  // Return consecutive instruction in llvm IR.
  // Iterate to next BB if required.
  if (const auto *next = start->getNextNode()) {
    return next;
  }
  if (const auto *nextNode = start->getParent()->getNextNode()) {
    return &(nextNode->front());
  }
  return (const llvm::Instruction *)nullptr;
}

const VISAModule::GenISARanges &VISAModule::getAllGenISARanges(const VISAObjectDebugInfo &VDI) {
  // Return vector of GenISARange elements for each instruction in module.
  // Corresponding index of instruction in resulting vector can be retrieved
  // from m_genISARangeIndex.
  if (!m_genISARanges.empty()) {
    return m_genISARanges;
  }

  const auto &VisaToGenMapping = VDI.getVisaToGenLUT();
  const auto &GenToSizeInBytes = VDI.getGenToSizeInBytesLUT();
  std::size_t currentRangeIndex = 0;

  llvm::for_each(m_instList, [&](const auto *entry) {
    GenISARange GenISARange;
    m_genISARangeIndex[entry] = currentRangeIndex;
    currentRangeIndex++;
    auto entryInfoIt = m_instInfoMap.find(entry);
    if (entryInfoIt == m_instInfoMap.end()) {
      m_genISARanges.emplace_back(std::move(GenISARange));
      return;
    }

    const auto startVISAOffset = entryInfoIt->second.m_offset;
    // VISASize indicated # of VISA insts emitted for this
    // LLVM IR inst
    const auto VISASize = entryInfoIt->second.m_size;

    if (VISASize == INVALID_SIZE) {
      m_genISARanges.emplace_back(std::move(GenISARange));
      return;
    }
    // Range consists of a sequence of LLVM IR instructions. This function needs
    // to return a range of corresponding Gen ISA instructions. Instruction
    // scheduling in Gen ISA means several independent sub-ranges will be
    // present.
    for (auto VISAIndex = startVISAOffset; VISAIndex != (startVISAOffset + VISASize); VISAIndex++) {
      auto it = VisaToGenMapping.find(VISAIndex);
      if (it == VisaToGenMapping.end())
        continue;
      int lastEnd = -1;
      for (const auto &genInst : it->second) {
        const auto sizeGenInst = GenToSizeInBytes.lookup(genInst);

        if (!GenISARange.empty())
          lastEnd = GenISARange.back().second;

        if (lastEnd == genInst) {
          GenISARange.back().second += sizeGenInst;
        } else {
          GenISARange.emplace_back(genInst, genInst + sizeGenInst);
        }
        lastEnd = GenISARange.back().second;
      }
    }
    m_genISARanges.emplace_back(std::move(GenISARange));
  });
  return m_genISARanges;
}

VISAModule::GenISARange VISAModule::getGenISARange(const VISAObjectDebugInfo &VDI, const InsnRange &Range) {
  // Given a range, return vector of start-end range for corresponding Gen ISA
  // instructions
  const auto *start = Range.first;
  const auto *end = Range.second;

  const auto &VisaToGenMapping = VDI.getVisaToGenLUT();
  const auto &GenToSizeInBytes = VDI.getGenToSizeInBytesLUT();
  const VISAModule::GenISARanges &GenISARanges = getAllGenISARanges(VDI);

  // Range consists of a sequence of LLVM IR instructions. This function needs
  // to return a range of corresponding Gen ISA instructions. Instruction
  // scheduling in Gen ISA means several independent sub-ranges will be present.
  GenISARange GenISARange;

  if (!end || !start || !HasVisaOffset(start) || !HasVisaOffset(end)) {
    return GenISARange;
  }

  auto startIndex = m_genISARangeIndex.find(start)->second;
  const auto endIndex = m_genISARangeIndex.find(end)->second;
  while (startIndex != (endIndex + 1)) {
    if (!GenISARanges[startIndex].empty()) {
      GenISARange.insert(GenISARange.end(), GenISARanges[startIndex].begin(), GenISARanges[startIndex].end());
    }
    ++startIndex;
  }

  if (GenISARange.empty()) {
    return GenISARange;
  }

  if (m_catchAllVisaId != 0) {
    llvm::DenseMap<unsigned, unsigned> unassignedGenOffset;
    auto it = VisaToGenMapping.find(m_catchAllVisaId);
    if (it != VisaToGenMapping.end()) {
      for (const auto &genInst : it->second) {
        unsigned int sizeGenInst = GenToSizeInBytes.lookup(genInst);
        unassignedGenOffset[genInst] = sizeGenInst;
      }
    }

    // Check whether holes can be filled up using catch all attributed Gen
    // instructions
    for (unsigned int i = 0; i != GenISARange.size(); i++) {
      auto rangeEnd = GenISARange[i].second;
      auto it = unassignedGenOffset.find(rangeEnd);
      if (it != unassignedGenOffset.end()) {
        GenISARange[i].second += (*it).second;
      }
    }
  }

  coalesceRanges(GenISARange);

  return GenISARange;
}

const DbgDecoder::VarInfo *VISAModule::getVarInfo(const VISAObjectDebugInfo &VDI, unsigned int vreg) const {
  auto &Cache = *VICache.get();
  if (Cache.empty()) {
    for (const auto &VarInfo : VDI.getVISAVariables()) {
      StringRef Name = VarInfo.name;
      // TODO: what to do with variables starting with "T"?
      if (Name.startswith("V")) {
        Name = Name.drop_front();
        unsigned RegNum = 0;
        if (!Name.getAsInteger(10, RegNum))
          Cache.insert(std::make_pair(RegNum, &VarInfo));
      }
    }
  }

  auto FoundIt = Cache.find(vreg);
  if (FoundIt == Cache.end())
    return nullptr;
  if (FoundIt->second->lrs.empty())
    return nullptr;
  return FoundIt->second;
}

bool VISAModule::hasOrIsStackCall(const VISAObjectDebugInfo &VDI) const {
  const auto &CFI = VDI.getCFI();
  if (CFI.befpValid || CFI.frameSize > 0 || CFI.retAddr.size() > 0)
    return true;
  return IsIntelSymbolTableVoidProgram();
}

const std::vector<DbgDecoder::SubroutineInfo> *VISAModule::getSubroutines(const VISAObjectDebugInfo &VDI) const {
  return &VDI.getSubroutines();
}

const VISAObjectDebugInfo &VISAModule::getVisaObjectDI(const VISADebugInfo &VD) const {
  return VD.getVisaObjectDI(*this);
}

bool VISAVariableLocation::IsSampler() const {
  if (!HasSurface())
    return false;

  auto surface = GetSurface();
  if (surface >= VISAModule::SAMPLER_REGISTER_BEGIN &&
      surface < VISAModule::SAMPLER_REGISTER_BEGIN + VISAModule::SAMPLER_REGISTER_NUM)
    return true;
  return false;
}

bool VISAVariableLocation::IsTexture() const {
  if (!HasSurface())
    return false;

  auto surface = GetSurface();
  if (surface >= VISAModule::TEXTURE_REGISTER_BEGIN &&
      surface < VISAModule::TEXTURE_REGISTER_BEGIN + VISAModule::TEXTURE_REGISTER_NUM)
    return true;
  return false;
}
