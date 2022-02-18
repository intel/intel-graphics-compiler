/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <ZEELFObjectBuilder.hpp>
#include <ZEInfo.hpp>
#include <ZEInfoYAML.hpp>

#ifndef ZEBinStandAloneBuild
#include "common/LLVMWarningsPush.hpp"
#endif

#include "llvm/MC/StringTableBuilder.h"
#include "llvm/Support/EndianStream.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"

#ifndef ZEBinStandAloneBuild
#include "common/LLVMWarningsPop.hpp"
#endif

#include <iostream>
#include <tuple>
#include "Probe/Assertion.h"

namespace zebin {

/// ELFWriter - A helper class to write ELF contents into given raw_pwrite_stream,
///             according to the given ZEELFObjectBuilder. This object should
///             only be used by ZEELFObjectBuilder
class ELFWriter {
public:
    ELFWriter(llvm::raw_pwrite_stream& OS,
        ZEELFObjectBuilder& objBuilder);

    // write the ELF file into OS, return the number of written bytes
    uint64_t write();

private:
    typedef ZEELFObjectBuilder::Section Section;
    typedef ZEELFObjectBuilder::StandardSection StandardSection;
    typedef ZEELFObjectBuilder::RelocSection RelocSection;
    typedef ZEELFObjectBuilder::ZEInfoSection ZEInfoSection;
    typedef ZEELFObjectBuilder::RelocationListTy RelocationListTy;
    typedef std::map<ZEELFObjectBuilder::SectionID, uint32_t> SectionIndexMapTy;
    typedef std::map<std::string, uint64_t> SymNameIndexMapTy;

    struct SectionHdrEntry {
        uint32_t name    = 0;
        unsigned type    = 0;
        uint64_t flags   = 0;
        uint64_t offset  = 0;
        uint64_t size    = 0;
        uint32_t link    = 0;
        uint32_t info    = 0;
        uint32_t entsize = 0;

        const Section* section = nullptr;
        // Cache the section name for writer's reference
        llvm::StringRef sectName;
    };
    typedef std::vector<SectionHdrEntry> SectionHdrListTy;

private:
    // set m_SectionHdrEntries and adjust the section index, also create
    // strings for sections' name in StringTableBuilder
    void createSectionHdrEntries();
    // write elf header
    void writeHeader();
    // write sections and set the attributes in SectionHdrEntry
    void writeSections();
    // write a raw section
    uint64_t writeSectionData(const uint8_t* data, uint64_t size, uint32_t padding);
    // write symbol table section, return section size
    uint64_t writeSymTab();
    // write rel or rela relocation table section
    uint64_t writeRelocTab(const RelocationListTy& relocs, bool isRelFormat);
    // write ze info section
    uint64_t writeZEInfo();
    // write .note.intelgt.compat section
    std::pair<uint64_t, uint64_t> writeCompatibilityNote();
    // write string table
    uint64_t writeStrTab();
    // write section header
    void writeSectionHeader();
    // write the section header's offset into ELF header
    void writeSectionHdrOffset(uint64_t offset);
    // write number of zero bytes
    void writePadding(uint32_t size);

    // The name writeWord seems confusing. Both ELF32 and ELF64 words are
    // uint32_t.
    void writeWord(uint64_t Word) {
        if (is64Bit())
            m_W.write<uint64_t>(Word);
        else
            m_W.write<uint32_t>(static_cast<uint32_t>(Word));
    }

    bool is64Bit() { return m_ObjBuilder.m_is64Bit; }

    uint16_t numOfSections();

    // name is the string table index of the section name
    SectionHdrEntry& createSectionHdrEntry(
        const std::string& name, unsigned type, unsigned flags = 0, const Section* sect = nullptr);
    SectionHdrEntry& createNullSectionHdrEntry();

    uint32_t getSymTabEntSize();
    uint32_t getRelocTabEntSize(bool isRelFormat);

    // name is the string table index of the symbol name
    void writeSymbol(uint32_t name, uint64_t value, uint64_t size,
    uint8_t binding, uint8_t type, uint8_t other, uint16_t shndx);

    void writeRelRelocation(uint64_t offset, uint64_t type, uint64_t symIdx);
    void writeRelaRelocation(uint64_t offset, uint64_t type, uint64_t symIdx, uint64_t addend);

    void writeSecHdrEntry(uint32_t name, uint32_t type, uint64_t flags,
        uint64_t address, uint64_t offset,
        uint64_t size, uint32_t link, uint32_t info,
        uint64_t addralign, uint64_t entsize);

private:
    llvm::support::endian::Writer m_W;
    llvm::StringTableBuilder m_StrTabBuilder{llvm::StringTableBuilder::ELF};
    ZEELFObjectBuilder& m_ObjBuilder;

    // Map Section::m_id to ELF section index, used for creating symbol table
    SectionIndexMapTy m_SectionIndex;
    uint32_t m_SymTabIndex = 0;
    // string table index, it'll be the last section in this ELF file
    uint32_t m_StringTableIndex = 0;

    // symbol name to symbol index mapping, for creating relocations
    SymNameIndexMapTy m_SymNameIdxMap;

    // section information for constructing section header
    SectionHdrListTy m_SectionHdrEntries;

};

} // namespace zebin

using namespace zebin;
using namespace llvm;

ZEELFObjectBuilder::Section&
ZEELFObjectBuilder::addStandardSection(
    std::string sectName, const uint8_t* data, uint64_t size, unsigned type,
    unsigned flags, uint32_t padding, uint32_t align, StandardSectionListTy& sections)
{
    IGC_ASSERT(type != ELF::SHT_NULL);
    // calculate the required padding to satisfy alignment requirement
    // The original data size is (size + padding)
    uint32_t need_padding_for_align = (align == 0) ?
        0 : align - ((size + padding) % align);
    if (need_padding_for_align == align)
        need_padding_for_align = 0;

    // total required padding is (padding + need_padding_for_align)
    sections.emplace_back(
        ZEELFObjectBuilder::StandardSection(sectName, data, size, type,
            flags, (need_padding_for_align + padding), m_sectionIdCount));
    ++m_sectionIdCount;
    return sections.back();
}

ZEELFObjectBuilder::SectionID
ZEELFObjectBuilder::addSectionText(
    std::string name, const uint8_t* data, uint64_t size, uint32_t padding,
    uint32_t align)
{
    // adjust the section name to be .text.givenSectName
    std::string sectName;
    if (name != "")
        sectName = m_TextName + "." + name;
    else
        sectName = m_TextName;

    Section& sect = addStandardSection(sectName, data, size, ELF::SHT_PROGBITS,
        ELF::SHF_ALLOC | ELF::SHF_EXECINSTR, padding, align, m_textSections);

    return sect.id();
}

ZEELFObjectBuilder::SectionID
ZEELFObjectBuilder::addSectionData(
    std::string name, const uint8_t* data, uint64_t size, uint32_t padding, uint32_t align, bool rodata)
{
    // adjust the section name to be .data.givenSectName
    std::string sectName;
    if (name != "")
        sectName = m_DataName + "." + name;
    else
        sectName = m_DataName;

    unsigned flags = ELF::SHF_ALLOC;
    if (!rodata)
        flags |= ELF::SHF_WRITE;

    Section& sect = addStandardSection(sectName, data, size, ELF::SHT_PROGBITS,
        flags, padding, align, m_dataAndbssSections);
    return sect.id();
}

ZEELFObjectBuilder::SectionID
ZEELFObjectBuilder::addSectionBss(
    std::string name, uint64_t size, uint32_t padding, uint32_t align)
{
    // adjust the section name to be .bss.givenSectName
    std::string sectName;
    if (name != "")
        sectName = m_BssName + "." + name;
    else
        sectName = m_BssName;

    Section& sect = addStandardSection(sectName, nullptr, size, ELF::SHT_NOBITS,
        ELF::SHF_ALLOC | ELF::SHF_WRITE, padding, align, m_dataAndbssSections);
    return sect.id();
}

void
ZEELFObjectBuilder::addSectionGTPinInfo(std::string name, const uint8_t* data, uint64_t size)
{
    // adjust the section name
    std::string sectName;
    if (name != "")
        sectName = m_GTPinInfoName + "." + name;
    else
        sectName = m_GTPinInfoName;

    addStandardSection(sectName,
        data, size, SHT_ZEBIN_GTPIN_INFO, 0, 0, 0, m_otherStdSections);
}

void
ZEELFObjectBuilder::addSectionVISAAsm(std::string name, const uint8_t* data, uint64_t size)
{
    // adjust the section name
    std::string sectName;
    if (name != "")
        sectName = m_VISAAsmName + "." + name;
    else
        sectName = m_VISAAsmName;

    addStandardSection(sectName,
        data, size, SHT_ZEBIN_VISAASM, 0, 0, 0, m_otherStdSections);
}

void
ZEELFObjectBuilder::addSectionMisc(std::string name, const uint8_t* data, uint64_t size)
{
    // adjust the section name
    std::string sectName;
    if (name != "")
        sectName = m_MiscName + "." + name;
    else
        sectName = m_MiscName;

    addStandardSection(sectName,
        data, size, SHT_ZEBIN_MISC, 0, 0, 0, m_otherStdSections);
}

void
ZEELFObjectBuilder::addSectionSpirv(std::string name, const uint8_t* data, uint64_t size)
{
    if (name.empty())
        name = m_SpvName;
    addStandardSection(name, data, size, SHT_ZEBIN_SPIRV, 0, 0, 0, m_otherStdSections);
}

ZEELFObjectBuilder::SectionID
ZEELFObjectBuilder::addSectionDebug(std::string name, const uint8_t* data, uint64_t size)
{
    if (name.empty())
        name = m_DebugName;
    Section& sect =
        addStandardSection(name, data, size, ELF::SHT_PROGBITS, 0, 0, 0, m_otherStdSections);
    return sect.id();
}

void
ZEELFObjectBuilder::addSectionZEInfo(zeInfoContainer& zeInfo)
{
    // every object should have at most one ze_info section
    IGC_ASSERT(!m_zeInfoSection);
    m_zeInfoSection.reset(new ZEInfoSection(zeInfo, m_sectionIdCount));
    ++m_sectionIdCount;
}

void ZEELFObjectBuilder::addSymbol(
    std::string name, uint64_t addr, uint64_t size, uint8_t binding,
    uint8_t type, ZEELFObjectBuilder::SectionID sectionId)
{
    if (binding == llvm::ELF::STB_LOCAL)
        m_localSymbols.emplace_back(
            ZEELFObjectBuilder::Symbol(name, addr, size, binding, type, sectionId));
    else
        m_globalSymbols.emplace_back(
            ZEELFObjectBuilder::Symbol(name, addr, size, binding, type, sectionId));
}

ZEELFObjectBuilder::RelocSection&
ZEELFObjectBuilder::getOrCreateRelocSection(SectionID targetSectId, bool isRelFormat)
{
    // linear search to see if there's existed reloc section with given target id and rel format
    // reversely iterate that the latest added might hit first
    for (RelocSectionListTy::reverse_iterator it = m_relocSections.rbegin();
         it != m_relocSections.rend(); ++it) {
        if ((*it).m_TargetID == targetSectId && (*it).isRelFormat() == isRelFormat)
            return *it;
    }
    // if not found, create one
    // adjust the section name to be .rel.applyTargetName or .rela.applyTargetName
    // If the targt name is empty, we use the defualt name .rel/.rela as the section name
    // though in our case this should not happen
    std::string sectName;
    std::string targetName = getSectionNameBySectionID(targetSectId);
    if (!targetName.empty())
        sectName = (isRelFormat? m_RelName : m_RelaName) + targetName;
    else
        sectName = isRelFormat? m_RelName : m_RelaName;

    m_relocSections.emplace_back(RelocSection(m_sectionIdCount, targetSectId, sectName, isRelFormat));
    ++m_sectionIdCount;
    return m_relocSections.back();
}

void ZEELFObjectBuilder::addRelRelocation(
    uint64_t offset, std::string symName, R_TYPE_ZEBIN type, SectionID sectionId)
{
    RelocSection& reloc_sect = getOrCreateRelocSection(sectionId, true);
    // create the relocation
    reloc_sect.m_Relocations.emplace_back(
        ZEELFObjectBuilder::Relocation(offset, symName, type));
}

void ZEELFObjectBuilder::addRelaRelocation(
    uint64_t offset, std::string symName, R_TYPE_ZEBIN type, uint64_t addend, SectionID sectionId)
{
    RelocSection& reloc_sect = getOrCreateRelocSection(sectionId, false);
    // create the relocation
    reloc_sect.m_Relocations.emplace_back(
        ZEELFObjectBuilder::Relocation(offset, symName, type, addend));
}

uint64_t ZEELFObjectBuilder::finalize(llvm::raw_pwrite_stream& os)
{
    ELFWriter w(os, *this);
    return w.write();
}

ZEELFObjectBuilder::SectionID
ZEELFObjectBuilder::getSectionIDBySectionName(const char* name)
{
    for (StandardSection& sect : m_textSections) {
        if (strcmp(name, sect.m_sectName.c_str()) == 0)
            return sect.id();
    }
    for (StandardSection& sect : m_dataAndbssSections) {
        if (strcmp(name, sect.m_sectName.c_str()) == 0)
            return sect.id();
    }
    for (StandardSection& sect : m_otherStdSections) {
        if (strcmp(name, sect.m_sectName.c_str()) == 0)
            return sect.id();
    }

    IGC_ASSERT_MESSAGE(0, "getSectionIDBySectionName: section not found");
    return 0;
}

std::string ZEELFObjectBuilder::getSectionNameBySectionID(SectionID id)
{
    // do linear search that we assume there won't be too many sections
    for (StandardSection& sect : m_textSections) {
        if (sect.id() == id)
            return sect.m_sectName;
    }
    for (StandardSection& sect : m_dataAndbssSections) {
        if (sect.id() == id)
            return sect.m_sectName;
    }
    for (StandardSection& sect : m_otherStdSections) {
        if (sect.id() == id)
            return sect.m_sectName;
    }
    IGC_ASSERT_MESSAGE(0, "getSectionNameBySectionID: invalid SectionID");
    return "";
}

uint64_t ELFWriter::writeSectionData(const uint8_t* data, uint64_t size, uint32_t padding)
{
    uint64_t start_off = m_W.OS.tell();

    // it's possible that a section has only pading but no data
    if (data != nullptr)
        m_W.OS.write((const char*)data, size);

    writePadding(padding);

    IGC_ASSERT((m_W.OS.tell() - start_off) == (size + padding));
    return m_W.OS.tell() - start_off;
}

void ELFWriter::writePadding(uint32_t size)
{
    for (uint32_t i = 0; i < size; ++i)
        m_W.write<uint8_t>(0x0);
}

uint32_t ELFWriter::getSymTabEntSize()
{
    if (is64Bit())
        return sizeof(ELF::Elf64_Sym);
    else
        return sizeof(ELF::Elf32_Sym);
}

uint32_t ELFWriter::getRelocTabEntSize(bool isRelFormat)
{
    if (is64Bit())
        return isRelFormat ? sizeof(ELF::Elf64_Rel) : sizeof(ELF::Elf64_Rela);
    else
        return isRelFormat ? sizeof(ELF::Elf32_Rel) : sizeof(ELF::Elf32_Rela);
}

void ELFWriter::writeSymbol(uint32_t name, uint64_t value, uint64_t size,
    uint8_t binding, uint8_t type, uint8_t other, uint16_t shndx)
{
    uint8_t info = (binding << 4) | (type & 0xf);
    if (is64Bit()) {
        m_W.write(name);  // st_name
        m_W.write(info);  // st_info
        m_W.write(other); // st_other
        m_W.write(shndx); // st_shndx
        writeWord(value); // st_value
        writeWord(size);  // st_size
    } else {
        m_W.write(name);  // st_name
        writeWord(value); // st_value
        writeWord(size);  // st_size
        m_W.write(info);  // st_info
        m_W.write(other); // st_other
        m_W.write(shndx); // st_shndx
    }
}

void ELFWriter::writeRelRelocation(uint64_t offset, uint64_t type, uint64_t symIdx)
{
    if (is64Bit()) {
        uint64_t info = (symIdx << 32) | (type & 0xffffffffL);
        m_W.write(offset);
        m_W.write(info);
    } else {
        uint32_t info = ((uint32_t)symIdx << 8) | ((unsigned char)type);
        m_W.write(uint32_t(offset));
        m_W.write(info);
    }
}

void ELFWriter::writeRelaRelocation(uint64_t offset, uint64_t type, uint64_t symIdx, uint64_t addend)
{
    writeRelRelocation(offset, type, symIdx);
    if (is64Bit()) {
        m_W.write(addend);
    } else {
        m_W.write(uint32_t(addend));
    }
}

uint64_t ELFWriter::writeRelocTab(const RelocationListTy& relocs, bool isRelFormat)
{
    uint64_t start_off = m_W.OS.tell();

    for (const ZEELFObjectBuilder::Relocation& reloc : relocs) {
        // the target symbol's name must have been added into symbol table
        IGC_ASSERT(m_SymNameIdxMap.find(reloc.symName()) != m_SymNameIdxMap.end());

        if (isRelFormat)
            writeRelRelocation(
                reloc.offset(), reloc.type(), m_SymNameIdxMap[reloc.symName()]);
        else
            writeRelaRelocation(
                reloc.offset(), reloc.type(), m_SymNameIdxMap[reloc.symName()], reloc.addend());
    }

    return m_W.OS.tell() - start_off;
}

uint64_t ELFWriter::writeSymTab()
{
    uint64_t start_off = m_W.OS.tell();

    uint64_t symidx = 0;

    // index 0 is the null symbol
    writeSymbol(0, 0, 0, 0, 0, 0, ELF::SHN_UNDEF);
    ++symidx;

    auto writeOneSym = [&](ZEELFObjectBuilder::Symbol& sym) {
        // create symbol name entry in str table
        uint32_t nameoff = m_StrTabBuilder.add(StringRef(sym.name()));

        uint16_t sect_idx = 0;
        if (sym.sectionId() >= 0) {
            // the given section's index must have been adjusted in
            // createSectionHdrEntries
            IGC_ASSERT(m_SectionIndex.find(sym.sectionId()) != m_SectionIndex.end());
            sect_idx = m_SectionIndex.at(sym.sectionId());
        }
        else {
            sect_idx = ELF::SHN_UNDEF;
        }

        writeSymbol(nameoff, sym.addr(), sym.size(), sym.binding(), sym.type(),
            0, sect_idx);
        // global symbol name must be unique
        IGC_ASSERT(sym.binding() != llvm::ELF::STB_GLOBAL || m_SymNameIdxMap.find(sym.name()) == m_SymNameIdxMap.end());
        // FIXME: This may not set the symidx correctly when there're multiple
        // same-name local symbols.
        m_SymNameIdxMap.insert(std::make_pair(sym.name(), symidx));
        ++symidx;
    };

    // Write the local symbols first
    for (ZEELFObjectBuilder::Symbol& sym : m_ObjBuilder.m_localSymbols) {
        writeOneSym(sym);
    }

    // And then global symbols
    for (ZEELFObjectBuilder::Symbol& sym : m_ObjBuilder.m_globalSymbols) {
        writeOneSym(sym);
    }

    return m_W.OS.tell() - start_off;
}

uint64_t ELFWriter::writeZEInfo()
{
    uint64_t start_off = m_W.OS.tell();
    // serialize ze_info contents
    llvm::yaml::Output yout(m_W.OS);
    IGC_ASSERT(m_ObjBuilder.m_zeInfoSection);
    yout << m_ObjBuilder.m_zeInfoSection->getZeInfo();

    return m_W.OS.tell() - start_off;
}

std::pair<uint64_t, uint64_t> ELFWriter::writeCompatibilityNote() {
    auto padToRequiredAlign = [&]() {
        // The alignment of the Elf word, name and descriptor is 4.
        // Implementations differ from the specification here: in practice all
        // variants align both the name and descriptor to 4-bytes.
        uint64_t cur = m_W.OS.tell();
        uint64_t next = llvm::alignTo(cur, 4);
        writePadding(next - cur);
    };

    auto writeOneNote = [&](StringRef owner, auto desc, uint32_t type) {
        // It's easier to use uint32_t directly now because both Elf32_Word and
        // Elf64_Word are uint32_t.
        // TODO: Use template implementation to handle ELF32 and ELF64 cases.
        m_W.write<uint32_t>(owner.size() + 1);
        m_W.write<uint32_t>(sizeof(desc));
        m_W.write<uint32_t>(type);
        m_W.OS << owner << '\0';
        padToRequiredAlign();
        m_W.write(desc);
        padToRequiredAlign();
    };

    // Align the section offset to the required alignment first.
    // TODO: Handle the section alignment in a more generic place..
    padToRequiredAlign();
    uint64_t start_off = m_W.OS.tell();
    // write NT_INTELGT_PRODUCT_FAMILY
    writeOneNote("IntelGT",
                 static_cast<uint32_t>(m_ObjBuilder.m_productFamily),
                 NT_INTELGT_PRODUCT_FAMILY);

    // write NT_INTELGT_GFXCORE_FAMILY_
    writeOneNote("IntelGT",
                 static_cast<uint32_t>(m_ObjBuilder.m_gfxCoreFamily),
                 NT_INTELGT_GFXCORE_FAMILY);

    // write NT_INTELGT_TARGET_METADATA
    writeOneNote("IntelGT",
                 m_ObjBuilder.m_metadata.packed,
                 NT_INTELGT_TARGET_METADATA);
    return std::make_pair(start_off, m_W.OS.tell() - start_off);
}

uint64_t ELFWriter::writeStrTab()
{
    uint64_t start_off = m_W.OS.tell();

    // at this point, all strings should be added. Finalized the string table
    // and write it to OS
    // must finalize it in order, that we take the offset of
    // section and symbols' name when added
    m_StrTabBuilder.finalizeInOrder();
    m_StrTabBuilder.write(m_W.OS);

    return m_W.OS.tell() - start_off;
}

void ELFWriter::writeSecHdrEntry(uint32_t name, uint32_t type, uint64_t flags,
    uint64_t address, uint64_t offset,
    uint64_t size, uint32_t link, uint32_t info,
    uint64_t addralign, uint64_t entsize)
{
    m_W.write(name);      // sh_name
    m_W.write(type);      // sh_type
    writeWord(flags);     // sh_flags
    writeWord(address);   // sh_addr
    writeWord(offset);    // sh_offset
    writeWord(size);      // sh_size
    m_W.write(link);      // sh_link
    m_W.write(info);      // sh_info
    writeWord(addralign); // sh_addralign
    writeWord(entsize);   // sh_entsize
}

void ELFWriter::writeSectionHeader()
{
    // all SectionHdrEntry fields should be fill-up in either
    // createSectionHdrEntries or writeSections
    for (SectionHdrEntry& entry : m_SectionHdrEntries) {
        writeSecHdrEntry(
            entry.name, entry.type, entry.flags, 0, entry.offset, entry.size, entry.link,
            entry.info, 0, entry.entsize);
    }
}

void ELFWriter::writeSections()
{
    for (SectionHdrEntry& entry : m_SectionHdrEntries) {
        entry.offset = m_W.OS.tell();

        switch(entry.type) {
        case SHT_ZEBIN_GTPIN_INFO: {
            // Encode the sh_info field with the index of the corresponding
            // kernel or function symbol in .symtab so that gtpin can do a fast
            // lookup.
            llvm::StringRef symName = entry.sectName;
            auto res = symName.consume_front(m_ObjBuilder.m_GTPinInfoName);
            IGC_ASSERT(res);
            if (symName.consume_front(".")) {
                auto it = m_SymNameIdxMap.find(symName.str());
                IGC_ASSERT(it != m_SymNameIdxMap.end());
                entry.info = it->second;
            }
            /* Fall-through */
        }
        case ELF::SHT_PROGBITS:
        case SHT_ZEBIN_VISAASM:
        case SHT_ZEBIN_SPIRV: {
            IGC_ASSERT(nullptr != entry.section);
            IGC_ASSERT(entry.section->getKind() == Section::STANDARD);
            const StandardSection* const stdsect =
                static_cast<const StandardSection*>(entry.section);
            IGC_ASSERT(nullptr != stdsect);
            IGC_ASSERT(stdsect->m_size + stdsect->m_padding);
            entry.size = writeSectionData(
                stdsect->m_data, stdsect->m_size, stdsect->m_padding);
            break;
        }
        case ELF::SHT_NOBITS: {
            const StandardSection* const stdsect =
                static_cast<const StandardSection*>(entry.section);
            IGC_ASSERT(nullptr != stdsect);
            entry.size = stdsect->m_size;
            break;
        }
        case ELF::SHT_SYMTAB:
            entry.size = writeSymTab();
            entry.entsize = getSymTabEntSize();
            entry.link = m_StringTableIndex;
            // one greater than the last local symbol index, including the
            // first null symbol
            entry.info = m_ObjBuilder.m_localSymbols.size() + 1;
            break;

        case ELF::SHT_REL:
        case ELF::SHT_RELA: {
            IGC_ASSERT(nullptr != entry.section);
            IGC_ASSERT(entry.section->getKind() == Section::RELOC);
            const RelocSection* const relocSec =
                static_cast<const RelocSection*>(entry.section);
            IGC_ASSERT(nullptr != relocSec);
            entry.size = writeRelocTab(relocSec->m_Relocations, relocSec->isRelFormat());
            entry.entsize = getRelocTabEntSize(relocSec->isRelFormat());
            break;
        }
        case SHT_ZEBIN_ZEINFO:
            entry.size = writeZEInfo();
            break;

        case ELF::SHT_STRTAB:
            entry.size = writeStrTab();
            break;

        case ELF::SHT_NULL:
            // the first entry
            entry.size =
                (m_SectionHdrEntries.size() + 1) >= ELF::SHN_LORESERVE ?
                (m_SectionHdrEntries.size() + 1) : 0;
            break;

        case ELF::SHT_NOTE: {
            if (entry.sectName == m_ObjBuilder.m_CompatNoteName)
                std::tie(entry.offset, entry.size) = writeCompatibilityNote();
            break;
        }

        default:
            IGC_ASSERT(0);
            break;
        }
    }
}

void ELFWriter::writeSectionHdrOffset(uint64_t offset)
{
    auto &stream = static_cast<raw_pwrite_stream &>(m_W.OS);

    if (is64Bit()) {
        uint64_t Val =
            support::endian::byte_swap<uint64_t>(offset, m_W.Endian);
        stream.pwrite(reinterpret_cast<char *>(&Val), sizeof(Val),
            offsetof(ELF::Elf64_Ehdr, e_shoff));

    } else {
        uint32_t Val =
            support::endian::byte_swap<uint32_t>(
                static_cast<uint32_t>(offset), m_W.Endian);
        stream.pwrite(reinterpret_cast<char *>(&Val), sizeof(Val),
            offsetof(ELF::Elf32_Ehdr, e_shoff));
    }
}

void ELFWriter::writeHeader()
{
    // e_ident[EI_MAG0] to e_ident[EI_MAG3]
    m_W.OS << ELF::ElfMagic;

    // e_ident[EI_CLASS]
    m_W.OS << char(m_ObjBuilder.m_is64Bit ? ELF::ELFCLASS64 : ELF::ELFCLASS32);

    // e_ident[EI_DATA]
    m_W.OS << char(ELF::ELFDATA2LSB);

    // e_ident[EI_VERSION]
    m_W.OS << char(ELF::EV_CURRENT);

    // e_ident padding
    m_W.OS.write_zeros(ELF::EI_NIDENT - ELF::EI_OSABI);

    // e_type: Currently IGC always emits a relocatable file
    m_W.write<uint16_t>(ELF::ET_REL);

    // e_machine
    m_W.write<uint16_t>(EM_INTELGT);

    // e_version
    m_W.write<uint32_t>(ELF::EV_CURRENT);

    // e_entry, no entry point
    writeWord(0);

    // e_phoff, no program header
    writeWord(0);

    // e_shoff, will write it later
    writeWord(0);

    // e_flags
    m_W.write<uint32_t>(0);

    // e_ehsize = ELF header size
    m_W.write<uint16_t>(is64Bit() ?
        sizeof(ELF::Elf64_Ehdr) : sizeof(ELF::Elf32_Ehdr));

    m_W.write<uint16_t>(0);          // e_phentsize = prog header entry size
    m_W.write<uint16_t>(0);          // e_phnum = # prog header entries = 0

    // e_shentsize
    m_W.write<uint16_t>(is64Bit() ?
        sizeof(ELF::Elf64_Shdr) : sizeof(ELF::Elf32_Shdr));

    // TODO: We do not support the case that the number of sections is greater
    // than or equal to SHN_LORESERVE now.
    IGC_ASSERT_MESSAGE(numOfSections() < ELF::SHN_LORESERVE,
        "# of sections should be less than SHN_LORESERVE (oxff00)");
    // e_shnum
    m_W.write<uint16_t>(numOfSections());

    // e_shstrndx  = .strtab index
    m_W.write<uint16_t>(m_StringTableIndex);
}

uint16_t ELFWriter::numOfSections()
{
    // string table is the last section in this file
    return m_StringTableIndex + 1;
}

ELFWriter::ELFWriter(llvm::raw_pwrite_stream& OS,
                     ZEELFObjectBuilder& objBuilder)
    : m_W(OS, llvm::support::little), m_ObjBuilder(objBuilder)
{
}

uint64_t ELFWriter::write()
{
    uint64_t start = m_W.OS.tell();
    createSectionHdrEntries();
    writeHeader();
    writeSections();
    uint64_t sectHdrOff = m_W.OS.tell();
    writeSectionHeader();
    writeSectionHdrOffset(sectHdrOff);
    return m_W.OS.tell() - start;
}

ELFWriter::SectionHdrEntry& ELFWriter::createNullSectionHdrEntry()
{
    m_SectionHdrEntries.emplace_back(SectionHdrEntry());
    SectionHdrEntry& entry = m_SectionHdrEntries.back();
    entry.type = ELF::SHT_NULL;
    entry.name = 0;
    return entry;
}

ELFWriter::SectionHdrEntry& ELFWriter::createSectionHdrEntry(
    const std::string& name, unsigned type, unsigned flags, const Section* sect)
{
    m_SectionHdrEntries.emplace_back(SectionHdrEntry());
    SectionHdrEntry& entry = m_SectionHdrEntries.back();
    entry.type = type;
    entry.flags = flags;
    entry.section = sect;
    entry.sectName = name;
    uint32_t nameoff = m_StrTabBuilder.add(StringRef(name));
    entry.name = nameoff;
    return entry;
}

void ELFWriter::createSectionHdrEntries()
{
    // adjust the sections' order in ELF and set m_SectionIndex, m_SymTabIndex,
    // m_RelTabIndex and m_StringTableIndex
    // Sections will be layout as
    // .text
    // .data
    // .symtab
    // all other standard sections follow the order of being added (spv, debug)
    // .rel and .rela
    // .ze_info
    // .strtab

    // first entry is NULL section
    createNullSectionHdrEntry();

    uint32_t index = 1;
    // .text
    for (StandardSection& sect : m_ObjBuilder.m_textSections) {
        m_SectionIndex.insert(std::make_pair(sect.id(), index));
        ++index;
        createSectionHdrEntry(sect.m_sectName, sect.m_type, sect.m_flags, &sect);
    }

    // .data
    for (StandardSection& sect : m_ObjBuilder.m_dataAndbssSections) {
        m_SectionIndex.insert(std::make_pair(sect.id(), index));
        ++index;
        createSectionHdrEntry(sect.m_sectName, sect.m_type, sect.m_flags, &sect);
    }

    // .symtab
    if (!m_ObjBuilder.m_localSymbols.empty() ||
        !m_ObjBuilder.m_globalSymbols.empty()) {
        m_SymTabIndex = index;
        ++index;
        createSectionHdrEntry(m_ObjBuilder.m_SymTabName, ELF::SHT_SYMTAB);
    }

    // other sections
    for (StandardSection& sect : m_ObjBuilder.m_otherStdSections) {
        m_SectionIndex.insert(std::make_pair(sect.id(), index));
        ++index;
        createSectionHdrEntry(sect.m_sectName, sect.m_type, sect.m_flags, &sect);
    }

    // .rel and .rela
    if (!m_ObjBuilder.m_relocSections.empty()) {
        // go through relocation sections
        for (RelocSection& sect : m_ObjBuilder.m_relocSections) {
            SectionHdrEntry& entry =
                sect.isRelFormat() ?
                  createSectionHdrEntry(sect.m_sectName, ELF::SHT_REL, 0, &sect) :
                  createSectionHdrEntry(sect.m_sectName, ELF::SHT_RELA, 0, &sect);
            // set apply target's section index
            // relocations could only apply to standard sections. At this point,
            // all standard section's section index should be adjusted
            IGC_ASSERT(m_SectionIndex.find(sect.m_TargetID) != m_SectionIndex.end());
            entry.info = m_SectionIndex.at(sect.m_TargetID);
            entry.link = m_SymTabIndex;
            ++index;
        }
    }

    // .ze_info
    if (m_ObjBuilder.m_zeInfoSection) {
        createSectionHdrEntry(m_ObjBuilder.m_ZEInfoName, SHT_ZEBIN_ZEINFO, 0,
            m_ObjBuilder.m_zeInfoSection.get());
        ++index;
    }

    // .note.intelgt.compat
    // Create the compatibility note section
    createSectionHdrEntry(m_ObjBuilder.m_CompatNoteName, ELF::SHT_NOTE);
    ++index;

    // .strtab
    m_StringTableIndex = index;
    createSectionHdrEntry(m_ObjBuilder.m_StrTabName, ELF::SHT_STRTAB);
}

// createKernel - create a zeInfoKernel and add it into zeInfoContainer
zeInfoKernel& ZEInfoBuilder::createKernel(const std::string& name)
{
    mContainer.kernels.emplace_back();
    zeInfoKernel& k = mContainer.kernels.back();
    k.name = name;
    return k;
}

bool ZEInfoBuilder::empty() const
{
    return mContainer.kernels.empty();
}

// addPayloadArgumentByPointer - add explicit kernel argument with pointer
// type into given arg_list
zeInfoPayloadArgument& ZEInfoBuilder::addPayloadArgumentByPointer(
    PayloadArgumentsTy& arg_list,
    int32_t offset,
    int32_t size,
    int32_t arg_index,
    PreDefinedAttrGetter::ArgAddrMode addrmode,
    PreDefinedAttrGetter::ArgAddrSpace addrspace,
    PreDefinedAttrGetter::ArgAccessType access_type)
{
    arg_list.emplace_back();
    zeInfoPayloadArgument& arg = arg_list.back();
    arg.arg_type = PreDefinedAttrGetter::get(PreDefinedAttrGetter::ArgType::arg_bypointer);
    arg.offset = offset;
    arg.size = size;
    arg.arg_index = arg_index;
    arg.addrmode = PreDefinedAttrGetter::get(addrmode);
    arg.addrspace = PreDefinedAttrGetter::get(addrspace);
    arg.access_type = PreDefinedAttrGetter::get(access_type);
    return arg;
}

// addPayloadArgumentSampler - add explicit kernel argument for sampler
// into given arg_list
// The argument type will be set to by_pointer, and addr_space will be set to sampler
zeInfoPayloadArgument& ZEInfoBuilder::addPayloadArgumentSampler(
    PayloadArgumentsTy& arg_list,
    int32_t offset,
    int32_t size,
    int32_t arg_index,
    int32_t sampler_index,
    PreDefinedAttrGetter::ArgAddrMode addrmode,
    PreDefinedAttrGetter::ArgAccessType access_type)
{
    zeInfoPayloadArgument& arg = addPayloadArgumentByPointer(arg_list, offset, size, arg_index, addrmode,
        PreDefinedAttrGetter::ArgAddrSpace::sampler, access_type);
    arg.sampler_index = sampler_index;
    return arg;
}

// addPayloadArgumentByValue - add explicit kernel argument with pass by
// value type into given arg_list
zeInfoPayloadArgument& ZEInfoBuilder::addPayloadArgumentByValue(
    PayloadArgumentsTy& arg_list,
    int32_t offset,
    int32_t size,
    int32_t arg_index,
    int32_t source_offset)
{
    // Here we merge the specified payload argument from the flattened byval
    // aggregate elements into the previous contiguous zeinfo payload argument
    // when the host data layout is same as the payload layout, so that there
    // will be less zeinfo payload arguments and it'll be easier for runtime to
    // manage.
    bool mergeable = false;
    if (!arg_list.empty()) {
        zeInfoPayloadArgument& prev = arg_list.back();
        // merge-able elements must be contiguous in payload and in the host data layout
        // FIXME: It's possible that an element is contiguous in host data but is not in
        // the payload. Cases seen in by-val nested struct argument.
        mergeable = prev.arg_index == arg_index &&
                    (prev.source_offset + prev.size == source_offset) &&
                    (prev.offset + prev.size == offset);
    }

    if (!mergeable)
        arg_list.emplace_back();

    zeInfoPayloadArgument& arg = arg_list.back();

    if (mergeable) {
        arg.size += size;
    } else  {
        arg.arg_type = PreDefinedAttrGetter::get(PreDefinedAttrGetter::ArgType::arg_byvalue);
        arg.offset = offset;
        arg.size = size;
        arg.arg_index = arg_index;
        arg.source_offset = source_offset;
    }
    return arg;
}

// addPayloadArgumentImplicit - add non-user argument (implicit argument)
// into given zeKernel. The type must be local_size, group_size,
// global_id_offset or private_base_stateless
zeInfoPayloadArgument& ZEInfoBuilder::addPayloadArgumentImplicit(
    PayloadArgumentsTy& arg_list,
    PreDefinedAttrGetter::ArgType type,
    int32_t offset,
    int32_t size)
{
    arg_list.emplace_back();
    zeInfoPayloadArgument& arg = arg_list.back();
    arg.arg_type = PreDefinedAttrGetter::get(type);
    arg.offset = offset;
    arg.size = size;
    return arg;
}

// addPerThreadPayloadArgument - add a per-thread payload argument into
// given kernel. Currently we only support local id as per-thread argument.
// The given type must be packed_local_ids or local_id
zeInfoPerThreadPayloadArgument& ZEInfoBuilder::addPerThreadPayloadArgument(
    PerThreadPayloadArgumentsTy& arg_list,
    PreDefinedAttrGetter::ArgType type,
    int32_t offset,
    int32_t size)
{
    arg_list.emplace_back();
    zeInfoPerThreadPayloadArgument& arg = arg_list.back();
    arg.arg_type = PreDefinedAttrGetter::get(type);
    arg.offset = offset;
    arg.size = size;
    return arg;
}

// addBindingTableIndex - add a binding table index into kernel, with
// corresponding kernel argument index
zeInfoBindingTableIndex& ZEInfoBuilder::addBindingTableIndex(
    BindingTableIndicesTy& bti_list,
    int32_t bti_value,
    int32_t arg_index)
{
    bti_list.emplace_back();
    zeInfoBindingTableIndex& bti = bti_list.back();
    bti.bti_value = bti_value;
    bti.arg_index = arg_index;
    return bti;
}

zeInfoPerThreadMemoryBuffer& ZEInfoBuilder::addPerThreadMemoryBuffer(
    PerThreadMemoryBuffersTy& mem_buff_list,
    PreDefinedAttrGetter::MemBufferType type,
    PreDefinedAttrGetter::MemBufferUsage usage,
    int32_t size)
{
    // use addScratchPerThreadMemoryBuffer API to add scratch buffer
    IGC_ASSERT(type != PreDefinedAttrGetter::MemBufferType::scratch);
    mem_buff_list.emplace_back();
    zeInfoPerThreadMemoryBuffer& info = mem_buff_list.back();
    info.type = PreDefinedAttrGetter::get(type);
    info.usage = PreDefinedAttrGetter::get(usage);
    info.size = size;
    return info;
}

zeInfoPerThreadMemoryBuffer& ZEInfoBuilder::addScratchPerThreadMemoryBuffer(
    PerThreadMemoryBuffersTy& mem_buff_list,
    PreDefinedAttrGetter::MemBufferUsage usage,
    int32_t slot_id,
    int32_t size)
{
    mem_buff_list.emplace_back();
    zeInfoPerThreadMemoryBuffer& info = mem_buff_list.back();
    info.type = PreDefinedAttrGetter::get(PreDefinedAttrGetter::MemBufferType::scratch);
    info.usage = PreDefinedAttrGetter::get(usage);
    info.size = size;
    info.slot = slot_id;
    return info;
}

zeInfoPerThreadMemoryBuffer& ZEInfoBuilder::addPerSIMTThreadGlobalMemoryBuffer(
    PerThreadMemoryBuffersTy& mem_buff_list,
    PreDefinedAttrGetter::MemBufferUsage usage,
    int32_t size)
{
    mem_buff_list.emplace_back();
    zeInfoPerThreadMemoryBuffer& info = mem_buff_list.back();
    info.type = PreDefinedAttrGetter::get(PreDefinedAttrGetter::MemBufferType::global);
    info.usage = PreDefinedAttrGetter::get(usage);
    info.size = size;
    info.is_simt_thread = true;
    return info;
}

void ZEInfoBuilder::addExpPropertiesHasNonKernelArgLdSt(zeInfoKernel& zekernel,
    bool hasNonKernelArgLoad, bool hasNonKernelArgStore, bool hasNonKernelArgAtomic)
{
    zeInfoExperimentalProperties& ep = zekernel.experimental_properties;
    ep.has_non_kernel_arg_load = hasNonKernelArgLoad;
    ep.has_non_kernel_arg_store = hasNonKernelArgStore;
    ep.has_non_kernel_arg_atomic = hasNonKernelArgAtomic;
}

void ZEInfoBuilder::addGlobalHostAccessSymbol(const std::string& device_name, const std::string& host_name)
{
    mContainer.global_host_access_table.push_back(zeInfoHostAccess{ device_name, host_name });
}
