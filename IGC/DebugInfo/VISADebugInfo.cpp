/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// clang-format off
#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/raw_ostream.h"
#include "common/LLVMWarningsPop.hpp"
// clang-format on

#include "VISADebugDecoder.hpp"
#include "VISADebugInfo.hpp"
#include "VISAModule.hpp"
#include "Utils.hpp"

#include <algorithm>
#include <string>
#include <map>

using namespace llvm;

namespace IGC {

VISAObjectDebugInfo::VISAObjectDebugInfo(const CompiledObjectInfo &COIn)
    : CO(COIn) {
  // build GenIsaToVisa LUT
  std::transform(CO.CISAIndexMap.begin(), CO.CISAIndexMap.end(),
                 std::back_inserter(GenISAToVISAIndex),
                 [](const auto &CisaIndex) {
                   return IDX_Gen2Visa{CisaIndex.second /*GenOffset*/,
                                       CisaIndex.first /*VisaOffset*/};
                 });
  // build GenIsaInstSize LUT
  // TODO: we don't really need individual arrays for that, a single
  // storage should work just fine
  // Compute all Gen ISA offsets corresponding to each VISA index
  for (auto &Gen2VisaIdx : GenISAToVISAIndex) {
    auto FoundRecord = VISAIndexToAllGenISAOff.find(Gen2VisaIdx.VisaOffset);
    if (FoundRecord != VISAIndexToAllGenISAOff.end()) {
      FoundRecord->second.push_back(Gen2VisaIdx.GenOffset);
    } else {
      VISAIndexToAllGenISAOff.emplace(
          std::make_pair(Gen2VisaIdx.VisaOffset,
                         std::vector<unsigned>({Gen2VisaIdx.GenOffset})));
    }
  }
  // build VisaIndexToGen LUT
  if (!GenISAToVISAIndex.empty()) {
    auto CurRange = llvm::make_range(GenISAToVISAIndex.begin(),
                                     std::prev(GenISAToVISAIndex.end()));
    auto NextRange = llvm::make_range(std::next(GenISAToVISAIndex.begin()),
                                      GenISAToVISAIndex.end());
    for (const auto &IndexPair : llvm::zip(CurRange, NextRange)) {
      IDX_Gen2Visa &CurGenIdx = std::get<0>(IndexPair);
      IDX_Gen2Visa &NextGenIdx = std::get<1>(IndexPair);
      IGC_ASSERT(NextGenIdx.GenOffset >= CurGenIdx.GenOffset);
      auto Size = NextGenIdx.GenOffset - CurGenIdx.GenOffset;
      GenISAInstSizeBytes.insert(std::make_pair(CurGenIdx.GenOffset, Size));
    }
    GenISAInstSizeBytes.insert(
        std::make_pair(GenISAToVISAIndex.back().GenOffset, 16u));
  }
}

void VISAObjectDebugInfo::dump() const { print(llvm::dbgs()); }

void VISAObjectDebugInfo::print(llvm::raw_ostream &OS) const {
  OS << "LUT for <" << CO.kernelName << "> {\n";

  OS << "  --- VISAIndexToAllGenISAOff Dump (\n";
  OrderedTraversal(VISAIndexToAllGenISAOff, [&OS](const auto &VisaIdx,
                                                  const auto &GenOffsets) {
    OS << "    VI2Gen: " << VisaIdx << " => [";
    std::vector<std::string> HexStrings;
    std::transform(
        GenOffsets.begin(), GenOffsets.end(), std::back_inserter(HexStrings),
        [](const auto &Offset) {
          return (llvm::Twine("0x") + llvm::Twine::utohexstr(Offset)).str();
        });
    OS << llvm::join(HexStrings, ", ");
    OS << "]\n";
  });
  OS << "  )___\n";

  OS << "  --- GenISAToVISAIndex Dump (\n";
  llvm::for_each(GenISAToVISAIndex, [&OS](const auto &GenToVisaIdx) {
    OS << "    G2V: 0x" << llvm::Twine::utohexstr(GenToVisaIdx.GenOffset)
       << " => " << GenToVisaIdx.VisaOffset << "\n";
  });
  OS << "  )___\n";

  OS << "  --- GenISAInstSizeBytes Dump (\n";
  OrderedTraversal(
      GenISAInstSizeBytes, [&OS](const auto &GenOffset, const auto &Size) {
        OS << "    GI2Size: 0x" << llvm::Twine::utohexstr(GenOffset) << " => ";
        OS << Size << "\n";
      });
  OS << "  )___\n";

  OS << "}\n";
}

VISADebugInfo::VISADebugInfo(const void *RawDbgDataPtr)
    : DecodedDebugStorage(RawDbgDataPtr) {
  for (const auto &CO : DecodedDebugStorage.compiledObjs) {
    DebugInfoMap.emplace(std::make_pair(&CO, VISAObjectDebugInfo(CO)));
  }
}

const VISAObjectDebugInfo &
VISADebugInfo::getVisaObjectDI(const VISAModule &VM) const {

  auto EntryFuncName = VM.GetVISAFuncName();

  IGC_ASSERT(!DebugInfoMap.empty());
  if (DebugInfoMap.size() == 1)
    return DebugInfoMap.begin()->second;

  for (const auto &CO : DecodedDebugStorage.compiledObjs) {
    auto VisaDebugInfoIt = DebugInfoMap.find(&CO);
    IGC_ASSERT(VisaDebugInfoIt != DebugInfoMap.end());

    if (CO.kernelName.compare(EntryFuncName.str()) == 0) {
      return VisaDebugInfoIt->second;
    }

    if (VM.GetType() == VISAModule::ObjectType::SUBROUTINE) {
      // Subroutine bounds are stored inside corresponding kernel struct
      // in VISA debug info.
      bool SubroutineMatched = std::any_of(
          CO.subs.begin(), CO.subs.end(), [&EntryFuncName](const auto &Sub) {
            return Sub.name.compare(EntryFuncName.str()) == 0;
          });
      if (SubroutineMatched) {
        return VisaDebugInfoIt->second;
      }
    }
  }

  IGC_ASSERT_MESSAGE(0, "could not get debug info object!");
  return DebugInfoMap.begin()->second;
}

const VISAObjectDebugInfo &VISADebugInfo::getVisaObjectByCompliledObjectName(
    llvm::StringRef CompiledObjectName) const {
  auto FoundIt = std::find_if(DecodedDebugStorage.compiledObjs.begin(),
                              DecodedDebugStorage.compiledObjs.end(),
                              [&CompiledObjectName](const auto &CO) {
                                return (CO.kernelName == CompiledObjectName);
                              });
  IGC_ASSERT(FoundIt != DecodedDebugStorage.compiledObjs.end());

  auto VisaDebugInfoIt = DebugInfoMap.find(&*FoundIt);
  IGC_ASSERT(VisaDebugInfoIt != DebugInfoMap.end());
  return VisaDebugInfoIt->second;
}

void VISADebugInfo::dump() const { print(llvm::dbgs()); }

void VISADebugInfo::print(llvm::raw_ostream &OS) const {

  DecodedDebugStorage.print(OS);
  OS << "--- [DBG] VISADebugInfo LUTS [DBG] ---\n";

  OrderedTraversal(DebugInfoMap,
                   [&OS](const auto *CompiledObjDI, const auto &VoDI) {
                     (void)CompiledObjDI;
                     VoDI.print(OS);
                   },
                   [](const auto *CompiledObjL, const auto *CompiledObjR) {
                     return CompiledObjL->kernelName < CompiledObjR->kernelName;
                   });
}

} // namespace IGC
