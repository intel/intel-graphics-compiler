/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// PatchInfo dumper.
//

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <tuple>
#include <vector>

#include "visa/PatchInfo.h"

namespace {

struct Section {
  void *Start;
  std::size_t Size;
};

class PatchInfoDumper {
  char *Data;
  std::size_t Size;

  char *Binary;
  std::size_t BinarySize;

  cm::patch::PInfoHdr *Header;

  void *SectionHdrStart;
  unsigned SectionHdrEntries;

  std::FILE *OF;

  std::vector<Section> Sections;

public:
  PatchInfoDumper(char *D, std::size_t S, char *B, std::size_t BS,
                  std::FILE *fp)
      : Data(D), Size(S), Binary(B), BinarySize(BS), Header(nullptr),
        SectionHdrStart(nullptr), SectionHdrEntries(0), OF(fp) {}

  void dump();

protected:
  int dumpHeader();
  int dumpSectionHeaders();
  int dumpSections();

  int dumpDummySection(unsigned i);
  int dumpUnknownSection(unsigned i);
  int dumpBinary(unsigned i, char *Start, std::size_t Sz);
  int dumpRelocation(unsigned i, char *Start, std::size_t Sz,
                     cm::patch::PInfo_U16 Lnk, cm::patch::PInfo_U16 Lnk2);
  int dumpSymbolTable(unsigned i, char *Start, std::size_t Sz,
                      cm::patch::PInfo_U16 Lnk);
  int dumpStringTable(unsigned i, char *Start, std::size_t Sz);
  int dumpRegAccessTable(unsigned i, char *Start, std::size_t Sz,
                         cm::patch::PInfo_U16 Type, cm::patch::PInfo_U16 Lnk2);
  int dumpTokenTable(unsigned i, char *Start, std::size_t Sz,
                     cm::patch::PInfo_U16 Lnk2);

  int error(const char *msg) {
    std::cerr << msg << '\n';
    return 1;
  }
};

} // End anonymous namespace

void PatchInfoDumper::dump() {
  dumpHeader() || dumpSectionHeaders() || dumpSections();
}

int PatchInfoDumper::dumpHeader() {
  if (Size < sizeof(cm::patch::PInfoHdr))
    return error("Invalid patch info: header is too small!");

  cm::patch::PInfoHdr *H = reinterpret_cast<cm::patch::PInfoHdr *>(Data);

  if (!H->checkMagic())
    return error("Invalid patch info: invalid header!");
  if (H->Version != cm::patch::PV_0)
    return error("Invalid patch info: invalid header!");
  if (!H->isValidPlatform())
    return error("Invalid patch info: invalid header!");
  if (H->ShOffset >= Size)
    return error("Invalid patch info: invalid header!");
  if (H->ShOffset + H->ShNum * sizeof(cm::patch::PInfoSectionHdr) > Size)
    return error("Invalid patch info: invalid header!");

  union {
    char c[sizeof(cm::patch::PInfo_U32)];
    cm::patch::PInfo_U32 m;
  } u;

  u.m = H->Magic;
  std::fprintf(OF, "Patch Info Header:\n");
  std::fprintf(OF, "  Magic: %08x ('%c', '%c', '%c', '%c')\n", H->Magic, u.c[0],
               u.c[1], u.c[2], u.c[3]);
  std::fprintf(OF, "  Version: %u\n", H->Version);
  std::fprintf(OF, "  Platform: %u\n", H->Platform);
  std::fprintf(OF, "\n");

  Header = H;

  SectionHdrStart = Data + H->ShOffset;
  SectionHdrEntries = H->ShNum;

  return 0;
}

int PatchInfoDumper::dumpSectionHeaders() {
  if (!SectionHdrStart)
    return 1;

  auto getShTypeString = [](unsigned ShType) {
    static const char *ShTypeStrs[] = {
        "NONE",   "BINARY",     "RELOC",      "SYMTBL",
        "STRTBL", "INITREGTAB", "FINIREGTAB", "TOKTAB",
    };
    unsigned NumTys = sizeof(ShTypeStrs) / sizeof(ShTypeStrs[0]);
    if (ShType < NumTys)
      return ShTypeStrs[ShType];
    return "UNKNOWN";
  };

  std::fprintf(OF, "Section Headers:\n");
  cm::patch::PInfoSectionHdr *Sh =
      reinterpret_cast<cm::patch::PInfoSectionHdr *>(SectionHdrStart);
  for (unsigned i = 0, e = SectionHdrEntries; i != e; ++i) {
    std::fprintf(OF, "  [%3u] %04x(%10s) %4u %4u %08x %08x\n", i, Sh->ShType,
                 getShTypeString(Sh->ShType), Sh->ShLink, Sh->ShLink2,
                 Sh->ShOffset, Sh->ShSize);
    ++Sh;
  }
  std::fprintf(OF, "\n");

  return 0;
}

int PatchInfoDumper::dumpSections() {
  if (!SectionHdrStart)
    return 1;

  cm::patch::PInfoSectionHdr *Sh =
      reinterpret_cast<cm::patch::PInfoSectionHdr *>(SectionHdrStart);
  for (unsigned i = 0, e = SectionHdrEntries; i != e; ++i, ++Sh) {
    if (Sh->ShOffset >= Size) {
      error("Invalid section!");
      return 1;
    }
    if ((Sh->ShOffset + Sh->ShSize) > Size) {
      error("Invalid section!");
      return 1;
    }
    char *Start = Data + Sh->ShOffset;
    switch (Sh->ShType) {
    case cm::patch::PSHT_NONE:
      dumpDummySection(i);
      break;
    case cm::patch::PSHT_BINARY:
      if (Sh->ShSize == 0)
        dumpBinary(i, Binary, BinarySize);
      else
        dumpBinary(i, Start, Sh->ShSize);
      break;
    case cm::patch::PSHT_REL:
      dumpRelocation(i, Start, Sh->ShSize, Sh->ShLink, Sh->ShLink2);
      break;
    case cm::patch::PSHT_SYMTAB:
      dumpSymbolTable(i, Start, Sh->ShSize, Sh->ShLink);
      break;
    case cm::patch::PSHT_STRTAB:
      dumpStringTable(i, Start, Sh->ShSize);
      break;
    case cm::patch::PSHT_INITREGTAB:
    case cm::patch::PSHT_FINIREGTAB:
      dumpRegAccessTable(i, Start, Sh->ShSize, Sh->ShType, Sh->ShLink2);
      break;
    case cm::patch::PSHT_TOKTAB:
      dumpTokenTable(i, Start, Sh->ShSize, Sh->ShLink2);
      break;
    default:
      dumpUnknownSection(i);
      break;
    }
  }

  return 0;
}

int PatchInfoDumper::dumpDummySection(unsigned i) {
  std::fprintf(OF, "Dummy section [%u]:\n\n", i);
  return 0;
}

int PatchInfoDumper::dumpUnknownSection(unsigned i) {
  std::fprintf(OF, "Unknown section [%u]:\n", i);
  std::fprintf(OF, "  SKIPPED\n");
  std::fprintf(OF, "\n");
  return 0;
}

int PatchInfoDumper::dumpBinary(unsigned i, char *Start, std::size_t Sz) {
  std::fprintf(OF, "Binary section [%u]:\n", i);
  if (BinarySize == 0) {
    // If there's no binary specified, use the one from patch info.
    Binary = Start;
    BinarySize = Sz;
  }
  if (Sz > 0) {
    std::fprintf(OF, " %zd\n", Sz);
  } else
    std::fprintf(OF, "  dummy\n");
  std::fprintf(OF, "\n");
  return 0;
}

int PatchInfoDumper::dumpRelocation(unsigned i, char *Start, std::size_t Sz,
                                    cm::patch::PInfo_U16 Lnk,
                                    cm::patch::PInfo_U16 Lnk2) {
  std::fprintf(OF, "Relocation section [%u] for binary section [%u]:\n", i,
               Lnk2);

  cm::patch::PInfoSectionHdr *Sh =
      reinterpret_cast<cm::patch::PInfoSectionHdr *>(SectionHdrStart);
  char *StringTable = Data + Sh[Sh[Lnk].ShLink].ShOffset;
  cm::patch::PInfoSymbol *Sym =
      reinterpret_cast<cm::patch::PInfoSymbol *>(Data + Sh[Lnk].ShOffset);

  cm::patch::PInfoRelocation *Rel =
      reinterpret_cast<cm::patch::PInfoRelocation *>(Start);
  unsigned n = 0;
  for (; Sz > 0; ++n, ++Rel, Sz -= sizeof(cm::patch::PInfoRelocation)) {
    std::fprintf(OF, "  [%3u]:", n);
    std::fprintf(OF, "  %08x", Rel->RelAddr);
    std::fprintf(OF, "  %s", &StringTable[Sym[Rel->RelSym].SymName]);
    std::fprintf(OF, "\n");
  }
  std::fprintf(OF, "\n");

  return 0;
}

int PatchInfoDumper::dumpSymbolTable(unsigned i, char *Start, std::size_t Sz,
                                     cm::patch::PInfo_U16 Lnk) {
  std::fprintf(OF, "Symbol Table section [%u]:\n", i);

  cm::patch::PInfoSectionHdr *Sh =
      reinterpret_cast<cm::patch::PInfoSectionHdr *>(SectionHdrStart);
  char *StringTable = Data + Sh[Lnk].ShOffset;

  cm::patch::PInfoSymbol *Sym =
      reinterpret_cast<cm::patch::PInfoSymbol *>(Start);
  unsigned n = 0;
  for (; Sz > 0; ++n, ++Sym, Sz -= sizeof(cm::patch::PInfoSymbol)) {
    std::fprintf(OF, "  [%3u]: %08x", n, Sym->SymValue);
    if (Sym->SymShndx)
      std::fprintf(OF, " %5u", Sym->SymShndx);
    else
      std::fprintf(OF, " UNDEF");
    std::fprintf(OF, " %04x", Sym->SymExtra);
    std::fprintf(OF, " %s", &StringTable[Sym->SymName]);
    std::fprintf(OF, "\n");
  }
  std::fprintf(OF, "\n");
  return 0;
}

int PatchInfoDumper::dumpStringTable(unsigned i, char *Start, std::size_t Sz) {
  std::fprintf(OF, "String Table section [%u]:\n", i);

  for (unsigned n = 0, Off = 0; Off < Sz; ++n, ++Off) {
    char *Str = Start + Off;
    std::fprintf(OF, "  [%3u]: %s\n", n, Str);
    Off += unsigned(std::strlen(Str));
  }
  std::fprintf(OF, "\n");

  return 0;
}

int PatchInfoDumper::dumpRegAccessTable(unsigned i, char *Start, std::size_t Sz,
                                        cm::patch::PInfo_U16 Type,
                                        cm::patch::PInfo_U16 Lnk2) {
  std::fprintf(OF, "%s Register Table section [%u] for binary section [%u]:\n",
               (Type == cm::patch::PSHT_INITREGTAB) ? "Init" : "Fini", i, Lnk2);

  cm::patch::PInfoRegAccess *RegAcc =
      reinterpret_cast<cm::patch::PInfoRegAccess *>(Start);

  for (unsigned n = 0; Sz > 0;
       ++n, ++RegAcc, Sz -= sizeof(cm::patch::PInfoRegAccess)) {
    std::fprintf(OF, "  [%3u]:", n);
    std::fprintf(OF, "  %08x", RegAcc->RegAccAddr);
    std::fprintf(OF, "  r%-3u", RegAcc->RegAccRegNo);
    std::fprintf(OF, "  %04x", RegAcc->RegAccDUT);
    std::fprintf(OF, "\n");
  }
  std::fprintf(OF, "\n");

  return 0;
}

int PatchInfoDumper::dumpTokenTable(unsigned i, char *Start, std::size_t Sz,
                                    cm::patch::PInfo_U16 Lnk2) {
  std::fprintf(OF, "Token Table section [%u] for binary section [%u]:\n", i,
               Lnk2);
  std::fprintf(OF, "\n");

  cm::patch::PInfoToken *Tok = reinterpret_cast<cm::patch::PInfoToken *>(Start);

  bool First = true;
  for (unsigned n = 0; Sz > 0;
       ++n, ++Tok, Sz -= sizeof(cm::patch::PInfoToken)) {
    if (First)
      std::fprintf(OF, " {");
    else
      std::fprintf(OF, " ,");
    std::fprintf(OF, "$%u", Tok->TokenNo);
    First = false;
  }
  std::fprintf(OF, "}\n");
  return 0;
}

void dumpPatchInfo(void *Buf, std::size_t Size, void *Bin, std::size_t BinSize,
                   std::FILE *fp) {
  if (!fp)
    fp = stdout;
  PatchInfoDumper D(reinterpret_cast<char *>(Buf), Size,
                    reinterpret_cast<char *>(Bin), BinSize, fp);
  D.dump();
}
