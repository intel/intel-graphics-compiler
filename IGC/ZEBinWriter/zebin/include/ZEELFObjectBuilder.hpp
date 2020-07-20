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
//===- ZEELFObjectBuilder.hpp -----------------------------------*- C++ -*-===//
// ZE Binary Utilitis
//
// \file
// This file declares ZEELFObjectBuilder for building an ZE Binary object
//===----------------------------------------------------------------------===//

#ifndef ZE_ELF_OBJECT_BUILDER_HPP
#define ZE_ELF_OBJECT_BUILDER_HPP

#include <ZEELF.h>
#include <ZEInfo.hpp>

#ifndef ZEBinStandAloneBuild
#include "common/LLVMWarningsPush.hpp"
#endif

#include "llvm/BinaryFormat/ELF.h"

#ifndef ZEBinStandAloneBuild
#include "common/LLVMWarningsPop.hpp"
#endif

#include <map>
#include <string>
#include <vector>

namespace llvm {
    class raw_pwrite_stream;
}

namespace zebin {

/// ZEELFObjectBuilder - Build an ELF Object for ZE binary format
class ZEELFObjectBuilder {
    friend class ELFWriter;
public:
    typedef uint32_t SectionID;

public:
    // Each ZEELFObjectBuilder creates one ELF Object
    ZEELFObjectBuilder(bool is64Bit,
                       ELF_TYPE_ZEBIN fileType,
                       uint32_t machine,
                       TargetFlags flags)
        : m_is64Bit(is64Bit),
          m_fileType(fileType),
          m_machineType(machine),
          m_flags(flags) {}

    ZEELFObjectBuilder(bool is64Bit)
        : m_is64Bit(is64Bit)
    {
        m_fileType = ET_ZEBIN_NONE;
        m_machineType = 0;
        m_flags.packed = 0;
    }

    ~ZEELFObjectBuilder() {
        if (m_zeInfoSection != nullptr)
            delete m_zeInfoSection;
    }

    /// Set elfFileHeader::e_type value
    void setFileType(ELF_TYPE_ZEBIN fileType)
    { m_fileType = fileType; }

    /// Set elfFileHeader::e_machine value
    void setMachine(uint32_t machine)
    { m_machineType = machine; }

    /// Set elfFileHeader::e_flags
    void setTargetFlag(TargetFlags flags)
    { m_flags = flags; }

    // add a text section contains gen binary
    // - name: section name. This is usually the kernel or function name of
    //         this text section. Do not includes leading .text in given
    //         name. For example, giving "kernel", the section name will be
    //         .text.kernel
    // - data: memory buffer of the section contents. This buffer must be live
    //         through this ZEELFObjectBuilder
    // - size: input buffer size in byte
    // - padding: required padding at the end
    // - align: alignment requirement in byte
    // - return a unique id for referencing in addSymbol
    // Currently we assume there is only one text section that'll be added
    SectionID addSectionText(
        std::string name, const uint8_t* data, uint64_t size, uint32_t padding, uint32_t align);

    // add a data section contains raw data, such as constant or global buffer.
    // - name: section name. Do not includes leading .data in given
    //         name. For example, giving "const", the section name will be
    //         .data.const
    // - data: memory buffer of the section contents. This buffer must be live through
    //         this ZEELFObjectBuilder
    // - size: input buffer size in byte
    // - padding: required padding at the end
    // - align: alignment requirement in byte
    // - return a unique id for referencing in addSymbol
    // - return a unique id for referencing in addSymbol
    SectionID addSectionData(
        std::string name, const uint8_t* data, uint64_t size, uint32_t padding = 0, uint32_t align = 0);

    // add a data section contains raw data, such as constant or global buffer.
    // - name: section name. The default name is .spv
    // - size in byte
    // - Note that the alignment requirement of the section should be satisfied
    //   by the given data and size
    // - Note that the given data buffer have to be alive through
    //   ZEELFObjectBuilder
    void addSectionSpirv(std::string name, const uint8_t* data, uint64_t size);

    // .debug_info section in DWARF format
    // - name: section name. The default name is .debug_info
    // - size in byte
    // - Note that the alignment requirement of the section should be satisfied
    //   by the given data and size
    // - Note that the given data buffer have to be alive through
    //   ZEELFObjectBuilder
    // - return a unique id for referencing in addSymbol
    SectionID addSectionDebug(std::string name, const uint8_t* data, uint64_t size);

    // add ze_info section
    void addSectionZEInfo(zeInfoContainer& zeInfo);

    // add a symbol
    // - name    : symbol's name
    // - addr    : symbol's address. The binary offset of where this symbol is
    //             defined in the section
    // - size    : symbol size
    // - binding : symbol binding. The value is defined in ELF standard ST_BIND
    // - type    : symbol type. The value is defined in ELF standard ST_TYPE
    // - sectionId : the section id of which this symbol is defined in. Giving
    //               -1 if this is an UNDEFINED symbol
    void addSymbol(std::string name, uint64_t addr, uint64_t size,
        uint8_t binding, uint8_t type, SectionID sectionId);

    // add a relocation
    // - offset    : the binary offset of the section where the relocation
    //               will apply to. The section is denoted by sectionId
    // - symName   : the target symbol's name
    // - type      : the relocation name
    // - sectionId : the section id where the relocation is apply to
    // Note that currently we assume there's only one text section. So the
    // target section will be the one added by addSectionText
    void addRelocation(
        uint64_t offset, std::string symName, R_TYPE_ZEBIN type, SectionID sectionId);

    // finalize - Finalize the ELF Object, write ELF file into given os
    // return number of written bytes
    uint64_t finalize(llvm::raw_pwrite_stream& os);

private:
    class Section {
    public:
        enum Kind {STANDARD, RELOC, ZEINFO};
        virtual Kind getKind() const = 0;

        SectionID id() const { return m_id; }

    protected:
        Section(uint32_t id) : m_id(id) {}
        virtual ~Section() {}
        Section(const Section&) = default;
        Section& operator=(const Section &) = default;

    protected:
        SectionID m_id;
    };

    class StandardSection : public Section {
    public:
        StandardSection(std::string name, std::string sectName, const uint8_t* data, uint64_t size,
            unsigned type, uint32_t padding, uint32_t id)
            : m_name(name), m_sectName(sectName), m_data(data), m_size(size), m_type(type),
              m_padding(padding), Section(id)
        {}

        Kind getKind() const { return STANDARD; }

        // m_name - given name from addSectionData/addSectionText/...
        // we need the name when there is relocations for this section, its reloc
        // section name will be .rel.{m_name}
        std::string m_name;
        // m_sectName - the final name presented in ELF section header
        // For example, text section for kernel with "myKernel", the given
        // name could be "myKernel", so m_name = "myKernel". And the
        // m_sectName is ".text.myKernel". This field is required as the
        // place holder for StringTable construction
        std::string m_sectName;
        const uint8_t* m_data;
        uint64_t m_size;
        // section type
        unsigned m_type;
        uint32_t m_padding;
    };

    class ZEInfoSection : public Section {
    public:
        ZEInfoSection(zeInfoContainer& zeinfo, uint32_t id)
            : m_zeinfo(zeinfo), Section(id)
        {}

        Kind getKind() const { return ZEINFO; }

        zeInfoContainer& getZeInfo()
        { return m_zeinfo; }

    private:
        zeInfoContainer& m_zeinfo;
    };

    class Symbol {
    public:
        Symbol(std::string name, uint64_t addr, uint64_t size, uint8_t binding,
            uint8_t type, int32_t sectionId)
            : m_name(name), m_addr(addr), m_size(size), m_binding(binding),
            m_type(type), m_sectionId(sectionId)
        {}

        std::string&   name()            { return m_name;      }
        uint64_t       addr()      const { return m_addr;      }
        uint64_t       size()      const { return m_size;      }
        uint8_t        binding()   const { return m_binding;   }
        uint8_t        type()      const { return m_type;      }
        int32_t        sectionId() const { return m_sectionId; }

    private:
        std::string m_name;
        uint64_t m_addr;
        uint64_t m_size;
        uint8_t m_binding;
        uint8_t m_type;
        int32_t m_sectionId;
    };

    class Relocation {
    public:
        Relocation(uint64_t offset, std::string symName, R_TYPE_ZEBIN type)
            : m_offset(offset), m_symName(symName), m_type(type)
        {}

        uint64_t            offset()  const { return m_offset;  }
        const std::string&  symName() const { return m_symName; }
        R_TYPE_ZEBIN        type()    const { return m_type;    }

    private:
        uint64_t m_offset;
        std::string m_symName;
        R_TYPE_ZEBIN m_type;
    };

    typedef std::vector<StandardSection> StandardSectionListTy;
    typedef std::vector<Symbol> SymbolListTy;
    typedef std::vector<Relocation> RelocationListTy;

    class RelocSection : public Section {
    public:
        RelocSection(SectionID myID, SectionID targetID, std::string sectName)
            : Section(myID), m_TargetID(targetID), m_sectName(sectName)
        {}

        Kind getKind() const { return RELOC; }

        // target section's id that this relocation section apply to
        SectionID m_TargetID;
        std::string m_sectName;
        RelocationListTy m_Relocations;
    };
    typedef std::vector<RelocSection> RelocSectionListTy;

private:
    Section& addStandardSection(
        std::string name, std::string sectName, const uint8_t* data, uint64_t size,
        unsigned type, uint32_t padding, uint32_t align, StandardSectionListTy& sections);

    RelocSection& getOrCreateRelocSection(SectionID targetSectId);

    std::string getSectionNameBySectionID(SectionID id);

private:
    // place holder for section default name
    const std::string m_TextName   = ".text";
    const std::string m_DataName   = ".data";
    const std::string m_SymTabName = ".symtab";
    const std::string m_RelName    = ".rel";
    const std::string m_SpvName    = ".spv";
    const std::string m_DebugName  = ".debug_info";
    const std::string m_ZEInfoName = ".ze_info";
    const std::string m_StrTabName = ".strtab";

private:
    // 32 or 64 bit object
    bool m_is64Bit;

    // information for ELF header
    ELF_TYPE_ZEBIN m_fileType; // e_type
    uint32_t m_machineType;    // e_machine
    TargetFlags m_flags;       // e_flags

    StandardSectionListTy m_textSections;
    StandardSectionListTy m_dataSections;
    StandardSectionListTy m_otherStdSections;
    RelocSectionListTy    m_relocSections;

    // current section id
    uint32_t m_sectionId = 0;

    // every ze object contains only one ze_info section
    ZEInfoSection* m_zeInfoSection = nullptr;
    SymbolListTy m_localSymbols;
    SymbolListTy m_globalSymbols;

};

/// ZEInfoBuilder - Build a zeInfoContainer for .ze_info section
class ZEInfoBuilder {
public:
    zeInfoContainer& getZEInfoContainer()             { return mContainer; }
    const zeInfoContainer& getZEInfoContainer() const { return mContainer; }

    /// --------- Helper functions for setup zeinfo contents -------------- ///
    // createKernel - create a zeInfoKernel and add it into zeInfoContainer
    zeInfoKernel& createKernel(const std::string& name);

    // addPayloadArgumentByPointer - add explicit kernel argument with pointer
    // type into given zeKernel
    static zeInfoPayloadArgument& addPayloadArgumentByPointer(
        PayloadArgumentsTy& arg_list,
        int32_t offset,
        int32_t size,
        int32_t arg_index,
        PreDefinedAttrGetter::ArgAddrMode addrmode,
        PreDefinedAttrGetter::ArgAddrSpace addrspace,
        PreDefinedAttrGetter::ArgAccessType access_type);

    // addPayloadArgumentByValue - add explicit kernel argument with pass by
    // value type into given zeKernel
    static zeInfoPayloadArgument& addPayloadArgumentByValue(
        PayloadArgumentsTy& arg_list,
        int32_t offset,
        int32_t size,
        int32_t arg_index);

    // addPayloadArgumentImplicit - add non-user argument (implicit argument)
    // into given zeKernel. The type must be local_size, group_size,
    // global_id_offset or private_base_stateless
    static zeInfoPayloadArgument& addPayloadArgumentImplicit(
        PayloadArgumentsTy& arg_list,
        PreDefinedAttrGetter::ArgType type,
        int32_t offset,
        int32_t size);

    // addPerThreadPayloadArgument - add a per-thread payload argument into
    // given kernel. Currently we only support local id as per-thread argument.
    // The given type must be packed_local_ids or local_id
    static zeInfoPerThreadPayloadArgument& addPerThreadPayloadArgument(
        PerThreadPayloadArgumentsTy& arg_list,
        PreDefinedAttrGetter::ArgType type,
        int32_t offset,
        int32_t size);

    // addBindingTableIndex - add a binding table index into kernel, with
    // corresponding kernel argument index
    static zeInfoBindingTableIndex& addBindingTableIndex(
        BindingTableIndexesTy& bti_list,
        int32_t bti_value,
        int32_t arg_index);

    // addPerThreadMemoryBuffer - add a memory buffer info
    static zePerThreadMemoryBuffer& addPerThreadMemoryBuffer(
        PerThreadMemoryBuffersTy& mem_buff_list,
        PreDefinedAttrGetter::MemBufferType type,
        PreDefinedAttrGetter::MemBufferUsage usage,
        int32_t size);

private:
    zeInfoContainer mContainer;
};

} // end namespace zebin

#endif // ZE_ELF_OBJECT_BUILDER_HPP
