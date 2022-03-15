/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "VISADebugDecoder.hpp"

#include <llvm/ADT/Twine.h>

template <typename T>
static void PrintItems(llvm::raw_ostream &OS, const T &Items,
                       const char *Separator = " ") {
  bool First = true;
  std::for_each(Items.begin(), Items.end(),
                [&First, &OS, &Separator](const auto &Item) {
                  if (!First)
                    OS << Separator;
                  else
                    First = false;

                  OS << "(";
                  Item.print(OS);
                  OS << ")";
                });
}

void IGC::DbgDecoder::Mapping::Register::print(llvm::raw_ostream &OS) const {
  OS << "RegMap<R#: " << regNum << ", Sub#:" << subRegNum << ">";
}

void IGC::DbgDecoder::Mapping::Register::dump() const { print(llvm::dbgs()); }

void IGC::DbgDecoder::Mapping::Memory::print(llvm::raw_ostream &OS) const {
  OS << "MemMap<" << ((isBaseOffBEFP == 1) ? "AbsBase(" : "BE_FP(")
     << memoryOffset << ")>";
}

void IGC::DbgDecoder::Mapping::Memory::dump() const { print(llvm::dbgs()); }

void IGC::DbgDecoder::VarAlloc::print(llvm::raw_ostream &OS) const {
  switch (virtualType) {
  case VarAlloc::VirTypeAddress:
    OS << "v:A->";
    break;
  case VarAlloc::VirTypeFlag:
    OS << "v:F->";
    break;
  case VarAlloc::VirTypeGRF:
    OS << "v:G->";
    break;
  };
  switch (physicalType) {
  case VarAlloc::PhyTypeAddress:
    OS << "p:A(!GRF) ";
    break;
  case VarAlloc::PhyTypeFlag:
    OS << "p:F(!GRF) ";
    break;
  case VarAlloc::PhyTypeGRF:
    OS << "p:G ";
    mapping.r.print(OS);
    break;
  case VarAlloc::PhyTypeMemory:
    OS << "p:M(!GRF) ";
    mapping.m.print(OS);
    break;
  };
}

void IGC::DbgDecoder::VarAlloc::dump() const { print(llvm::dbgs()); }

void IGC::DbgDecoder::LiveIntervalsVISA::print(llvm::raw_ostream &OS) const {
  OS << "LInt-V[" << start << ";" << end << "] ";
  var.print(OS);
}

void IGC::DbgDecoder::LiveIntervalsVISA::dump() const { print(llvm::dbgs()); }

void IGC::DbgDecoder::VarInfo::print(llvm::raw_ostream &OS) const {
  OS << "{ " << name << " - ";
  PrintItems(OS, lrs, ", ");
  OS << " }";
}

void IGC::DbgDecoder::VarInfo::dump() const { print(llvm::dbgs()); }

void IGC::DbgDecoder::LiveIntervalGenISA::print(llvm::raw_ostream &OS) const {
  OS << "LInt-G[" << start << ";" << end << "] ";
  var.print(OS);
}

void IGC::DbgDecoder::LiveIntervalGenISA::dump() const { print(llvm::dbgs()); }

void IGC::DbgDecoder::SubroutineInfo::print(llvm::raw_ostream &OS) const {
  OS << "Name=" << name << " [" << startVISAIndex << ";" << endVISAIndex
     << "], retvals: ";
  PrintItems(OS, retval, ", ");
}

void IGC::DbgDecoder::SubroutineInfo::dump() const { print(llvm::dbgs()); }

void IGC::DbgDecoder::RegInfoMapping::print(llvm::raw_ostream &OS) const {
  OS << "srcRegOff: " << srcRegOff << ", " << numBytes << " bytes; ";
  if (dstInReg)
    dst.r.print(OS);
  else
    dst.m.print(OS);
}

void IGC::DbgDecoder::RegInfoMapping::dump() const { print(llvm::dbgs()); }

void IGC::DbgDecoder::PhyRegSaveInfoPerIP::print(llvm::raw_ostream &OS) const {
  OS << "PhyR_SaveInfo: "
     << "IPOffset " << genIPOffset << ", numEntries " << numEntries << "\n";
  OS << "   >RegInfoMapping: [";

  PrintItems(OS, data, ", ");
  OS << "   ]";
}

void IGC::DbgDecoder::PhyRegSaveInfoPerIP::dump() const { print(llvm::dbgs()); }

void IGC::DbgDecoder::CallFrameInfo::print(llvm::raw_ostream &OS) const {
  OS << "    frameSize: " << frameSize << "\n";
  OS << "    befpValid: " << befpValid << "\n";
  OS << "    callerbefpValid: " << callerbefpValid << "\n";
  OS << "    retAddrValid: " << retAddrValid << "\n";

  OS << "    befp list: [\n    ";
  PrintItems(OS, befp, "\n        ");
  OS << "    ]\n";

  OS << "    callerbefp list: [\n    ";
  PrintItems(OS, callerbefp, "\n        ");
  OS << "    ]\n";

  OS << "    retaddr list: [\n    ";
  PrintItems(OS, retAddr, "\n        ");
  OS << "    ]\n";

  OS << "    callee save entry list: [\n    ";
  PrintItems(OS, calleeSaveEntry, "\n        ");
  OS << "    ]\n";

  OS << "    caller save entry list: [\n    ";
  PrintItems(OS, callerSaveEntry, "\n        ");
  OS << "    ]\n";
}

void IGC::DbgDecoder::CallFrameInfo::dump() const { print(llvm::dbgs()); }

void IGC::DbgDecoder::DbgInfoFormat::print(llvm::raw_ostream &OS) const {
  OS << "<VISADebugInfo>\n";
  OS << "  Kernel: " << kernelName << "\n";
  OS << "  RelocOffset: " << relocOffset << "\n";
  OS << "  NumSubroutines: " << subs.size() << "\n";

  OS << "  Subroutines: [\n    ";
  PrintItems(OS, subs, "\n    ");
  OS << "  ]\n";
  OS << "  CFI: {\n";
  cfi.print(OS);
  OS << "  }\n";

  OS << "  Vars:\n  ";
  PrintItems(OS, Vars, "\n  ");
  OS << "\n  CisaIndex:\n";
  std::for_each(CISAIndexMap.begin(), CISAIndexMap.end(), [&OS](const auto &V) {
    auto VisaIndex = V.first;
    auto GenOff = V.second;
    OS << "  GI: 0x" << llvm::Twine::utohexstr(GenOff)
       << " -> VI: " << VisaIndex << "\n";
  });
  OS << "</VISADebugInfo>";
}

void IGC::DbgDecoder::DbgInfoFormat::dump() const { print(llvm::dbgs()); }

void IGC::DbgDecoder::print(llvm::raw_ostream &OS) const {
  OS << "=====================================\n";
  OS << "***Compiled Kernel Debug Info Dump***\n";
  OS << "=====================================\n";
  size_t i = 0;
  for (const auto &k : compiledObjs) {
    OS << "CO[" << i << "] = " << k.kernelName << "\n";
    OS << "    ";
    if (k.subs.empty()) {
      OS << "- no subroutines\n";
    } else {
      for (const auto &Sub : k.subs)
        OS << "    " << Sub.name << "\n";
    }
    ++i;
  }
  for (const auto &k : compiledObjs) {
    k.print(OS);
    OS << "\n";
  }
  OS << "-------------------------------------\n";
}

void IGC::DbgDecoder::dump() const { print(llvm::dbgs()); }
