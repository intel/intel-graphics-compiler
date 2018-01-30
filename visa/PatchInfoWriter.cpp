/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

// PatchInfo Writer

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <vector>

#include "PatchInfo.h"

namespace {

class PatchInfoWriter {
  std::ostream &OS;
  unsigned Platform;
  uint64_t ShStart; // Start of section header.

public:
  PatchInfoWriter(std::ostream &S, unsigned P) : OS(S), Platform(P) {}

  void writePatchInfo(const char *FuncName, bool IsCaller, bool IsCallee,
                    const std::vector<std::tuple<unsigned, const char *>> &Rels
                    );

  unsigned tell() { return unsigned(OS.tellp()); }

  template<typename T> void write(T Val) {
    OS.write(reinterpret_cast<char *>(&Val), sizeof(Val));
  }

  void write(const char *Buf, size_t Len) {
    OS.write(Buf, Len);
  }

  void align(unsigned Align) {
    unsigned A = (1U << Align);
    unsigned Origin = tell();
    unsigned Padding = ((Origin + A - 1) / A) * A;
    writeZeros(Padding - Origin);
  }

protected:
  void writeHeader();
  void fixHeader(cm::patch::PInfo_U16, cm::patch::PInfo_U16,
                 cm::patch::PInfo_Offset, cm::patch::PInfo_Offset);

  void writeZeros(unsigned N) {
    uint64_t Z64 = 0;
    for (unsigned i = 0, e = N / sizeof(Z64); i != e; ++i)
      OS.write(reinterpret_cast<char *>(&Z64), sizeof(Z64));
    OS.write(reinterpret_cast<char *>(&Z64), N % sizeof(Z64));
  }
};

class SectionWriter {
  unsigned Align;

protected:
  PatchInfoWriter &PW;
  cm::patch::PInfo_U16    Index;
  cm::patch::PInfo_U16    Type;
  cm::patch::PInfo_Offset Start;
  cm::patch::PInfo_U32    Size;

public:
  SectionWriter(PatchInfoWriter &W, cm::patch::PInfo_U16 T)
    : Align(0), PW(W), Index(0), Start(0), Size(0), Type(T) {}

  void writeSectionData(std::function<void()> PreWrite = nullptr,
                        std::function<void()> PostWrite = nullptr) {
    PW.align(Align);
    if (PreWrite) PreWrite();
    Start = PW.tell();
    writeSectionDataImpl();
    Size = PW.tell() - Start;
    if (PostWrite) PostWrite();
  }

  void writeSectionHeader() {
    writeSectionHeaderImpl();
  }

  cm::patch::PInfo_U16 getIdx() const { return Index; }
  void setIdx(cm::patch::PInfo_U16 Idx) { Index = Idx; }

  unsigned getAlign() const { return Align; }
  void setAlign(unsigned Alignment) { Align = Alignment; }

protected:
  virtual void writeSectionDataImpl() = 0;
  virtual void writeSectionHeaderImpl() {
    cm::patch::PInfoSectionHdr H{Type, 0, 0, 0, Start, Size};
    PW.write(H);
  }
};

class NullSectionWriter : public SectionWriter {
public:
  NullSectionWriter(PatchInfoWriter &W)
    : SectionWriter(W, cm::patch::PSHT_NONE) {}

protected:
  void writeSectionDataImpl() override {
    // DO NOTHING
  }
};

class RelocationWriter;
class SymbolTableWriter;
class StringTableWriter;

class DummyBinaryWriter : public SectionWriter {
  SymbolTableWriter &SymbolTable;
  RelocationWriter &Rel;

  const char *FuncName;
  bool IsCaller, IsCallee;
  const std::vector<std::tuple<unsigned, const char *>> *Relocs;

public:
  DummyBinaryWriter(PatchInfoWriter &W,
                    SymbolTableWriter &S,
                    RelocationWriter &R,
                    const char *FN, bool Caller, bool Callee,
                    const std::vector<std::tuple<unsigned, const char *>> &Rels
                    )
    : SectionWriter(W, cm::patch::PSHT_BINARY), SymbolTable(S), Rel(R),
      FuncName(FN), IsCaller(Caller), IsCallee(Callee), Relocs(&Rels)
  {}

protected:
  void writeSectionDataImpl() override;
};

struct Symbol {
  cm::patch::PInfo_U32  Index;        ///< Index to the symbol table.
  cm::patch::PInfo_U32  StringIndex;  ///< Index to the string table.
  SectionWriter         *Section;     ///< Section where the symbol points to.
  cm::patch::PInfo_Addr Value;        ///< Offset where to symbol points to.
  cm::patch::PInfo_U16  Extra;        ///< Extra info.
};

struct Relocation {
  cm::patch::PInfo_Addr   Addr;
  Symbol                  *Sym;
};

class RelocationWriter : public SectionWriter {
  SymbolTableWriter &SymbolTable;
  SectionWriter *Section;
  std::vector<Relocation> Relocs;

public:
  RelocationWriter(PatchInfoWriter &W, SymbolTableWriter &S)
    : SectionWriter(W, cm::patch::PSHT_REL), Section(nullptr), SymbolTable(S) {}

  void addReloc(cm::patch::PInfo_Addr Addr, Symbol *Sym);

  void setSection(SectionWriter *S) { Section = S; }

protected:
  void writeSectionDataImpl() override;
  void writeSectionHeaderImpl() override;
};

struct cstring_less {
  bool operator()(const char *s0, const char *s1) const {
    return std::strcmp(s0, s1) < 0;
  }
};

class SymbolTableWriter : public SectionWriter {
  std::map<const char *, Symbol *, cstring_less> SymbolMap;
  std::vector<Symbol> SymbolPool;
  StringTableWriter &StringTable;

public:
  SymbolTableWriter(PatchInfoWriter &W, StringTableWriter &S)
    : SectionWriter(W, cm::patch::PSHT_SYMTAB), StringTable(S) {
    SymbolPool.push_back(Symbol{0, 0, nullptr, 0, 0});
  }

  Symbol *addSymbol(const char *Name, SectionWriter *Section,
                    cm::patch::PInfo_Addr Value, cm::patch::PInfo_U16 Extra);

protected:
  void writeSectionDataImpl() override;
  void writeSectionHeaderImpl() override;
};

class StringTableWriter : public SectionWriter {
  std::map<const char *, cm::patch::PInfo_Offset, cstring_less> StringMap;
  std::string StringPool;

public:
  StringTableWriter(PatchInfoWriter &W)
    : SectionWriter(W, cm::patch::PSHT_STRTAB) {
    // Append the null character.
    StringPool.push_back(0);
  }

  cm::patch::PInfo_U32 addString(const char *Str);

protected:
  void writeSectionDataImpl() override;
};


class SecHdrWriter {
  PatchInfoWriter &PW;
  std::vector<SectionWriter *> Sections;
  cm::patch::PInfo_Offset Start;

public:
  SecHdrWriter(PatchInfoWriter &W) : PW(W), Start(0) {}

  void addSection(SectionWriter &S) {
    S.setIdx(cm::patch::PInfo_U16(Sections.size()));
    Sections.push_back(&S);
  }

  void writeSectionHeader() {
    Start = PW.tell();
    for (auto I : Sections)
      I->writeSectionHeader();
  }

  cm::patch::PInfo_Offset getStart() const { return Start; }
  cm::patch::PInfo_U16 getNumOfEntries() const {
    return cm::patch::PInfo_U16(Sections.size());
  }
};

} // End namespace anonymous

void
writePatchInfo(std::ostream &OS, unsigned Platform, const char *FuncName,
               bool IsCaller, bool IsCallee,
               const std::vector<std::tuple<unsigned, const char *>> &Rels
               ) {
  PatchInfoWriter W(OS, Platform);
  W.writePatchInfo(FuncName, IsCaller, IsCallee, Rels
                   );
}

void
PatchInfoWriter::writePatchInfo(
  const char *FuncName, bool IsCaller, bool IsCallee,
  const std::vector<std::tuple<unsigned, const char *>> &Rels
  ) {
  writeHeader();

  NullSectionWriter Null(*this);
  StringTableWriter StrTbl(*this);
  SymbolTableWriter SymTbl(*this, StrTbl);
  RelocationWriter  Reloc(*this, SymTbl);
  DummyBinaryWriter Binary(*this, SymTbl, Reloc
                           , FuncName, IsCaller, IsCallee, Rels
                           );

  StrTbl.setAlign(2);
  SymTbl.setAlign(2);
  Reloc.setAlign(2);

  SecHdrWriter SH(*this);
  SH.addSection(Null);
  SH.addSection(Binary);
  SH.addSection(Reloc);
  SH.addSection(SymTbl);
  SH.addSection(StrTbl);

  Null.writeSectionData();
  Binary.writeSectionData();
  SymTbl.writeSectionData();
  Reloc.writeSectionData();
  StrTbl.writeSectionData();

  align(2); // Align to 4B.
  SH.writeSectionHeader();
  align(4); // Ensure patch info has size aligned to 16B.

  fixHeader(SH.getNumOfEntries(), 0, SH.getStart(), 0);
}

void PatchInfoWriter::writeHeader() {
  cm::patch::PInfoHdr PIH;
  PIH.Magic = cm::patch::MAGIC;
  PIH.Version = cm::patch::PV_0;
  PIH.Platform = (cm::patch::PInfo_U16)Platform;
  PIH.ShNum = 0;
  PIH.PgNum = 0;
  PIH.ShOffset = 0;
  PIH.PgOffset = 0;
  OS.write(reinterpret_cast<char *>(&PIH), sizeof(PIH));
}

void PatchInfoWriter::fixHeader(cm::patch::PInfo_U16 ShNum,
                                cm::patch::PInfo_U16 PgNum,
                                cm::patch::PInfo_Offset ShOff,
                                cm::patch::PInfo_Offset PgOff) {
  // Fill 'ShNum', 'PgNum', 'ShOffset', and 'PgOffset'.
  auto OldPos = OS.tellp();
  // Re-write 'ShNum', 'PgNum', 'ShOffset', and 'PgOffset'.
  OS.seekp(offsetof(cm::patch::PInfoHdr, ShNum), std::ios_base::beg);
  write(ShNum);
  write(PgNum);
  write(ShOff);
  write(PgOff);
  // Restore the output position.
  OS.seekp(OldPos);
}

void DummyBinaryWriter::writeSectionDataImpl() {
  // As a dummy binary writer, only add symbols and relocations from this
  // binary section.
  cm::patch::PInfo_U16 LT = 0;
  if (IsCaller && !Relocs->empty())
    LT = 1;
  else if (IsCallee)
    LT = 2;
  SymbolTable.addSymbol(FuncName, this, 0, LT);
  // Set the section which relocations applies.
  Rel.setSection(this);
  // Add relocations.
  for (auto &R : *Relocs)
    Rel.addReloc(std::get<0>(R),
                 SymbolTable.addSymbol(std::get<1>(R), nullptr, 0, 0));
}

void RelocationWriter::addReloc(cm::patch::PInfo_Addr Addr, Symbol *Sym) {
  Relocs.push_back(Relocation{Addr, Sym});
}

void RelocationWriter::writeSectionDataImpl() {
  for (auto &I : Relocs) {
    cm::patch::PInfoRelocation Rel{I.Addr, I.Sym->Index};
    PW.write(Rel);
  }
}

void RelocationWriter::writeSectionHeaderImpl() {
  cm::patch::PInfoSectionHdr
    H{Type, SymbolTable.getIdx(), Section->getIdx(), 0, Start, Size};
  PW.write(H);
}

Symbol *SymbolTableWriter::addSymbol(const char *Name,
                                     SectionWriter *Section,
                                     cm::patch::PInfo_Addr Value,
                                     cm::patch::PInfo_U16 Extra) {
  auto I = SymbolMap.find(Name);
  if (I != SymbolMap.end())
    return I->second;
  cm::patch::PInfo_U32 StringIndex = StringTable.addString(Name);
  cm::patch::PInfo_U32 Index = cm::patch::PInfo_U32(SymbolPool.size());
  SymbolPool.push_back(Symbol{Index, StringIndex, Section, Value, Extra});
  Symbol *NewSym = &SymbolPool.back();
  SymbolMap.insert(std::make_pair(Name, NewSym));
  return NewSym;
}

void SymbolTableWriter::writeSectionDataImpl() {
  for (auto &I : SymbolPool) {
    cm::patch::PInfo_U16 Shndx = cm::patch::PSHN_UNDEF;
    if (I.Section)
      Shndx = I.Section->getIdx();
    cm::patch::PInfoSymbol S{I.StringIndex, I.Value, Shndx, I.Extra};
    PW.write(S);
  }
}

void SymbolTableWriter::writeSectionHeaderImpl() {
  cm::patch::PInfoSectionHdr H{Type, StringTable.getIdx(), 0, 0, Start, Size};
  PW.write(H);
}

cm::patch::PInfo_U32 StringTableWriter::addString(const char *Str) {
  auto I = StringMap.find(Str);
  if (I != StringMap.end())
    return I->second;
  cm::patch::PInfo_U32 Index = cm::patch::PInfo_U32(StringPool.size());
  StringPool.append(Str, std::strlen(Str) + 1); // Including the null character.
  StringMap.insert(std::make_pair(Str, Index));
  return Index;
}

void StringTableWriter::writeSectionDataImpl() {
  const char *S = StringPool.data();
  PW.write(S, StringPool.size());
}

