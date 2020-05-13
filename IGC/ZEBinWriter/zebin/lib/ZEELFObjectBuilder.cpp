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
#include <ZEELFObjectBuilder.hpp>
#include <ZEInfo.hpp>
#include <ZEInfoYAML.hpp>

#include "common/LLVMWarningsPush.hpp"
#include "llvm/MC/StringTableBuilder.h"
#include "llvm/Support/EndianStream.h"
#include "llvm/Support/raw_ostream.h"
#include "common/LLVMWarningsPop.hpp"


#include <iostream>

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
        uint64_t offset  = 0;
        uint64_t size    = 0;
        uint32_t link    = 0;
        uint32_t info    = 0;
        uint32_t entsize = 0;

        const Section* section = nullptr;
    };
    typedef std::vector<SectionHdrEntry> SectionHdrListTy;

private:
    // set m_SectionHdrEntries and adjust the section index, also creaet
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
    // write relocation table section
    uint64_t writeRelocTab(const RelocationListTy& relocs);
    // write ze info section
    uint64_t writeZEInfo();
    // write string table
    uint64_t writeStrTab();
    // write section header
    void writeSectionHeader();
    // write the section header's offset into ELF header
    void writeSectionHdrOffset(uint64_t offset);
    // wirite number of zero bytes
    void writePadding(uint32_t size);

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
        const std::string& name, unsigned type, const Section* sect = nullptr);
    SectionHdrEntry& createNullSectionHdrEntry();

    uint32_t getSymTabEntSize();
    uint32_t getRelocTabEntSize();

    // name is the string table index of the symbol name
    void writeSymbol(uint32_t name, uint64_t value, uint64_t size,
        uint8_t binding, uint8_t type, uint8_t other, uint16_t shndx);

    void writeRelocation(uint64_t offset, uint64_t type, uint64_t symIdx);

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
    std::string name, std::string sectName, const uint8_t* data, uint64_t size,
    unsigned type, uint32_t padding, uint32_t align, StandardSectionListTy& sections)
{
    assert(type != ELF::SHT_NULL);
    // calcaulate the required padding to satisfy alignment requirement
    // The orignal data size is (size + padding)
    uint32_t need_padding_for_align = (align == 0) ?
        0 : align - ((size + padding) % align);
    if (need_padding_for_align == align)
        need_padding_for_align = 0;

    // total required padding is (padding + need_padding_for_align)
    sections.emplace_back(
        ZEELFObjectBuilder::StandardSection(name, sectName, data, size, type,
            (need_padding_for_align + padding), m_sectionId));
    ++m_sectionId;
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

    Section& sect = addStandardSection(name, sectName, data, size, ELF::SHT_PROGBITS,
        padding, align, m_textSections);

    return sect.id();
}

ZEELFObjectBuilder::SectionID
ZEELFObjectBuilder::addSectionData(
    std::string name, const uint8_t* data, uint64_t size, uint32_t padding, uint32_t align)
{
    // adjust the section name to be .data.givenSectName
    std::string sectName;
    if (name != "")
        sectName = m_DataName + "." + name;
    else
        sectName = m_DataName;

    Section& sect = addStandardSection(name, sectName, data, size, ELF::SHT_PROGBITS,
        padding, align, m_dataSections);
    return sect.id();
}

void
ZEELFObjectBuilder::addSectionSpirv(std::string name, const uint8_t* data, uint64_t size)
{
    if (name.empty())
        name = m_SpvName;
    addStandardSection(name, name, data, size, SHT_ZEBIN_SPIRV, 0, 0, m_otherStdSections);
}

ZEELFObjectBuilder::SectionID
ZEELFObjectBuilder::addSectionDebug(std::string name, const uint8_t* data, uint64_t size)
{
    if (name.empty())
        name = m_DebugName;
    Section& sect =
        addStandardSection(name, name, data, size, ELF::SHT_PROGBITS, 0, 0, m_otherStdSections);
    return sect.id();
}

void
ZEELFObjectBuilder::addSectionZEInfo(zeInfoContainer& zeInfo)
{
    // every object should have exactly one ze_info section
    assert(m_zeInfoSection == nullptr);
    m_zeInfoSection = new ZEInfoSection(zeInfo, m_sectionId);
    ++m_sectionId;
}

void ZEELFObjectBuilder::addSymbol(
    std::string name, uint64_t addr, uint64_t size, uint8_t binding,
    uint8_t type, ZEELFObjectBuilder::SectionID sectionId)
{
    m_symbols.emplace_back(
        ZEELFObjectBuilder::Symbol(name, addr, size, binding, type, sectionId));
}

ZEELFObjectBuilder::RelocSection&
ZEELFObjectBuilder::getOrCreateRelocSection(SectionID targetSectId)
{
    // linear search to see if there's existed reloc section with given target id
    // reversly iterate that the latest added might hit first
    for (RelocSectionListTy::reverse_iterator it = m_relocSections.rbegin();
         it != m_relocSections.rend(); ++it) {
        if ((*it).m_TargetID == targetSectId)
            return *it;
    }
    // if not found, create one
    // adjust the section name to be .rel.applyTergetName
    std::string sectName;
    std::string targetName = getSectionNameBySectionID(targetSectId);
    if (!targetName.empty())
        sectName = m_RelName + "." + targetName;
    else
        sectName = targetName;

    m_relocSections.emplace_back(m_sectionId, targetSectId, sectName);
    ++m_sectionId;
    return m_relocSections.back();
}

void ZEELFObjectBuilder::addRelocation(
    uint64_t offset, std::string symName, R_TYPE_ZEBIN type, SectionID sectionId)
{
    RelocSection& reloc_sect = getOrCreateRelocSection(sectionId);
    // create the relocation
    reloc_sect.m_Relocations.emplace_back(
        ZEELFObjectBuilder::Relocation(offset, symName, type));
}

uint64_t ZEELFObjectBuilder::finalize(llvm::raw_pwrite_stream& os)
{
    ELFWriter w(os, *this);
    return w.write();
}

std::string ZEELFObjectBuilder::getSectionNameBySectionID(SectionID id)
{
    // do linear search that we assume there won't be too many sections
    for (StandardSection& sect : m_textSections) {
        if (sect.id() == id)
            return sect.m_name;
    }
    for (StandardSection& sect : m_dataSections) {
        if (sect.id() == id)
            return sect.m_name;
    }
    for (StandardSection& sect : m_otherStdSections) {
        if (sect.id() == id)
            return sect.m_name;
    }
    assert(0 && "getSectionNameBySectionID: invalid SectionID");
    return "";
}

uint64_t ELFWriter::writeSectionData(const uint8_t* data, uint64_t size, uint32_t padding)
{
    uint64_t start_off = m_W.OS.tell();
    m_W.OS.write((const char*)data, size);

    writePadding(padding);

    assert((m_W.OS.tell() - start_off) == (size + padding));
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

uint32_t ELFWriter::getRelocTabEntSize()
{
    if (is64Bit())
        return sizeof(ELF::Elf64_Rel);
    else
        return sizeof(ELF::Elf32_Rel);
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

void ELFWriter::writeRelocation(uint64_t offset, uint64_t type, uint64_t symIdx)
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

uint64_t ELFWriter::writeRelocTab(const RelocationListTy& relocs)
{
    uint64_t start_off = m_W.OS.tell();

    for (const ZEELFObjectBuilder::Relocation& reloc : relocs) {
        // the target symbol's name must have been added into symbol table
        assert(m_SymNameIdxMap.find(reloc.symName()) != m_SymNameIdxMap.end());
        writeRelocation(
            reloc.offset(), reloc.type(), m_SymNameIdxMap[reloc.symName()]);
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

    for (ZEELFObjectBuilder::Symbol& sym : m_ObjBuilder.m_symbols) {
        // create symbol name entry in str table
        uint32_t nameoff = m_StrTabBuilder.add(StringRef(sym.name()));

        uint16_t sect_idx = 0;
        if (sym.sectionId() >= 0) {
            // the given section's index must have been adjusted in
            // createSectionHdrEntries
            assert(m_SectionIndex.find(sym.sectionId()) != m_SectionIndex.end());
            sect_idx = m_SectionIndex.at(sym.sectionId());
        } else {
            sect_idx = ELF::SHN_UNDEF;
        }

        writeSymbol(nameoff, sym.addr(), sym.size(), sym.binding(), sym.type(),
            0, sect_idx);
        // symbol name must be unique
        assert(m_SymNameIdxMap.find(sym.name()) == m_SymNameIdxMap.end());
        m_SymNameIdxMap.insert(std::make_pair(sym.name(), symidx));
        ++symidx;
    }

    return m_W.OS.tell() - start_off;
}

uint64_t ELFWriter::writeZEInfo()
{
    uint64_t start_off = m_W.OS.tell();
    // serialize ze_info contents
    llvm::yaml::Output yout(m_W.OS);
    yout << m_ObjBuilder.m_zeInfoSection->getZeInfo();

    return m_W.OS.tell() - start_off;
}

uint64_t ELFWriter::writeStrTab()
{
    uint64_t start_off = m_W.OS.tell();

    // at this point, all strings should be added. Finalized the string table
    // and write it to OS
    // must finlized it in order, that we take the offset of
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
            entry.name, entry.type, 0, 0, entry.offset, entry.size, entry.link,
            entry.info, 0, entry.entsize);
    }
}

void ELFWriter::writeSections()
{
    for (SectionHdrEntry& entry : m_SectionHdrEntries) {
        entry.offset = m_W.OS.tell();

        switch(entry.type) {
        case ELF::SHT_PROGBITS:
        case SHT_ZEBIN_SPIRV: {
            assert(entry.section != nullptr);
            assert(entry.section->getKind() == Section::STANDARD);
            const StandardSection* stdsect =
                static_cast<const StandardSection*>(entry.section);
            entry.size = writeSectionData(
                stdsect->m_data, stdsect->m_size, stdsect->m_padding);
            break;
        }
        case ELF::SHT_SYMTAB:
            entry.size = writeSymTab();
            entry.entsize = getSymTabEntSize();
            entry.link = m_StringTableIndex;
            // one greater than the last local symbol index. Currently we
            // should only have global symbols be exported. The only local
            // symbol is the default null symbol with index 0
            entry.info = 1;
            break;
        case ELF::SHT_REL: {
            assert(entry.section->getKind() == Section::RELOC);
            const RelocSection* relocSec =
                static_cast<const RelocSection*>(entry.section);
            entry.size = writeRelocTab(relocSec->m_Relocations);
            entry.entsize = getRelocTabEntSize();
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
        default:
            assert(0);
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
    m_W.OS << char(0);

    // e_ident padding
    m_W.OS.write_zeros(ELF::EI_NIDENT - ELF::EI_OSABI);

    // e_type
    m_W.write<uint16_t>(m_ObjBuilder.m_fileType);

    // e_machine
    m_W.write<uint16_t>(m_ObjBuilder.m_machineType);

    // e_version
    m_W.write<uint32_t>(0);

    // e_entry, no entry point
    writeWord(0);

    // e_phoff, no program header
    writeWord(0);

    // e_shoff, will write it later
    writeWord(0);

    // e_flags
    m_W.write<uint32_t>(m_ObjBuilder.m_flags.packed);

    // e_ehsize = ELF header size
    m_W.write<uint16_t>(is64Bit() ?
        sizeof(ELF::Elf64_Ehdr) : sizeof(ELF::Elf32_Ehdr));

    m_W.write<uint16_t>(0);          // e_phentsize = prog header entry size
    m_W.write<uint16_t>(0);          // e_phnum = # prog header entries = 0

    // e_shentsize
    m_W.write<uint16_t>(is64Bit() ?
        sizeof(ELF::Elf64_Shdr) : sizeof(ELF::Elf32_Shdr));

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
    const std::string& name, unsigned type, const Section* sect)
{
    m_SectionHdrEntries.emplace_back(SectionHdrEntry());
    SectionHdrEntry& entry = m_SectionHdrEntries.back();
    entry.type = type;
    entry.section = sect;
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
    // .rel
    // .ze_info
    // .strtab

    // first entry is NULL section
    createNullSectionHdrEntry();

    uint32_t index = 1;
    // .text
    for (StandardSection& sect : m_ObjBuilder.m_textSections) {
        m_SectionIndex.insert(std::make_pair(sect.id(), index));
        ++index;
        createSectionHdrEntry(sect.m_sectName, sect.m_type, &sect);
    }

    // .data
    for (StandardSection& sect : m_ObjBuilder.m_dataSections) {
        m_SectionIndex.insert(std::make_pair(sect.id(), index));
        ++index;
        createSectionHdrEntry(sect.m_sectName, sect.m_type, &sect);
    }

    // .symtab
    if (!m_ObjBuilder.m_symbols.empty()) {
        m_SymTabIndex = index;
        ++index;
        createSectionHdrEntry(m_ObjBuilder.m_SymTabName, ELF::SHT_SYMTAB);
    }

    // other sections
    for (StandardSection& sect : m_ObjBuilder.m_otherStdSections) {
        m_SectionIndex.insert(std::make_pair(sect.id(), index));
        ++index;
        createSectionHdrEntry(sect.m_sectName, sect.m_type, &sect);
    }

    // .rel
    if (!m_ObjBuilder.m_relocSections.empty()) {
        // go through relocation sections
        for (RelocSection& sect : m_ObjBuilder.m_relocSections) {
            SectionHdrEntry& entry =
                createSectionHdrEntry(sect.m_sectName, ELF::SHT_REL, &sect);
            // set apply target's section index
            // relocations could only apply to standard sections. At this point,
            // all standard section's section index should be adjusted
            assert(m_SectionIndex.find(sect.m_TargetID) != m_SectionIndex.end());
            entry.info = m_SectionIndex.at(sect.m_TargetID);
            entry.link = m_SymTabIndex;
            ++index;
        }
    }

    // .ze_info
    // every object must have exactly one ze_info section
    assert(m_ObjBuilder.m_zeInfoSection != nullptr);
    if (m_ObjBuilder.m_zeInfoSection != nullptr) {
        createSectionHdrEntry(m_ObjBuilder.m_ZEInfoName, SHT_ZEBIN_ZEINFO,
            m_ObjBuilder.m_zeInfoSection);
        ++index;
    }

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

// addPayloadArgumentByPointer - add explicit kernel argument with pointer
// type into given zeKernel
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

// addPayloadArgumentByValue - add explicit kernel argument with pass by
// value type into given zeKernel
zeInfoPayloadArgument& ZEInfoBuilder::addPayloadArgumentByValue(
    PayloadArgumentsTy& arg_list,
    int32_t offset,
    int32_t size,
    int32_t arg_index)
{
    arg_list.emplace_back();
    zeInfoPayloadArgument& arg = arg_list.back();
    arg.arg_type = PreDefinedAttrGetter::get(PreDefinedAttrGetter::ArgType::arg_byvalue);
    arg.offset = offset;
    arg.size = size;
    arg.arg_index = arg_index;
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
    BindingTableIndexesTy& bti_list,
    int32_t bti_value,
    int32_t arg_index)
{
    bti_list.emplace_back();
    zeInfoBindingTableIndex& bti = bti_list.back();
    bti.bti_value = bti_value;
    bti.arg_index = arg_index;
    return bti;
}

zePerThreadMemoryBuffer& ZEInfoBuilder::addPerThreadMemoryBuffer(
    PerThreadMemoryBuffersTy& mem_buff_list,
    PreDefinedAttrGetter::MemBufferType type,
    PreDefinedAttrGetter::MemBufferUsage usage,
    int32_t size)
{
    mem_buff_list.emplace_back();
    zePerThreadMemoryBuffer& info = mem_buff_list.back();
    info.type = PreDefinedAttrGetter::get(type);
    info.usage = PreDefinedAttrGetter::get(usage);
    info.size = size;
    return info;
}
