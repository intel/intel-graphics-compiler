/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===- ZEELFObjectBuilder.hpp -----------------------------------*- C++ -*-===//
// ZE Binary Utilities
//
// \file
// This file declares ZEELFObjectBuilder for building an ZE Binary object
//===----------------------------------------------------------------------===//

#ifndef ZE_ELF_OBJECT_BUILDER_HPP
#define ZE_ELF_OBJECT_BUILDER_HPP

#include <ZEELF.h>
#include <ZEInfo.hpp>
#include "inc/common/igfxfmid.h"

#ifndef ZEBinStandAloneBuild
#include "common/LLVMWarningsPush.hpp"
#endif

#include "llvm/BinaryFormat/ELF.h"

#ifndef ZEBinStandAloneBuild
#include "common/LLVMWarningsPop.hpp"
#endif

#include <map>
#include <memory>
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
    // The valid SectionID must be 0 or positive value
    typedef int32_t SectionID;

public:
    ZEELFObjectBuilder(bool is64Bit) : m_is64Bit(is64Bit)
    {
        m_metadata.packed = 0;
    }

    ~ZEELFObjectBuilder() {}

    void setProductFamily(PRODUCT_FAMILY family) { m_productFamily = family; }
    PRODUCT_FAMILY getProductFamily() const { return m_productFamily; }

    void setGfxCoreFamily(GFXCORE_FAMILY family) { m_gfxCoreFamily = family; }
    GFXCORE_FAMILY getGfxCoreFamily() const { return m_gfxCoreFamily; }

    void setTargetMetadata(TargetMetadata metadata) { m_metadata = metadata; }
    TargetMetadata getTargetMetadata() const { return m_metadata; }

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
    SectionID addSectionData(
        std::string name, const uint8_t* data, uint64_t size, uint32_t padding = 0, uint32_t align = 0, bool rodata = false);

    // add a bss section which occupies no space in the ELF, but with size and other section information
    // The bss sections could be used for zero-initialized variables.
    // - name: section name. Do not includes leading .bss in given name.
    //         For example, giving "const", the section name will be .bss.const
    // - size: input buffer size in byte
    // - padding: required padding at the end
    // - align: alignment requirement in byte
    // - return a unique id for referencing in addSymbol
    SectionID addSectionBss(
        std::string name, uint64_t size, uint32_t padding = 0, uint32_t align = 0);

    // add a spv section contains the spv source. This section will be set to SHT_ZEBIN_SPIRV type
    // - name: section name. The default name is .spv
    // - size in byte
    // - Note that the alignment requirement of the section should be satisfied
    //   by the given data and size
    // - Note that the given data buffer have to be alive through
    //   ZEELFObjectBuilder
    void addSectionSpirv(std::string name, const uint8_t* data, uint64_t size);

    // add .gtpin_info section
    // - name: section name. Do not includes leading .gtpin_info in the given
    //         name. For example, giving "func", the section name will be
    //         ".gtpin_info.func". The default name is .gtpin_fino. It'll be apply
    //         if the given name is empty.
    // - size in byte
    // - Note that the alignment requirement of the section should be satisfied
    //   by the given data and size
    // - Note that the given data buffer have to be alive through
    //   ZEELFObjectBuilder
    void addSectionGTPinInfo(std::string name, const uint8_t* data, uint64_t size);

    // add .visaasm section
    // - name: section name. Do not includes leading .visaasm in the given
    //         name. For example, giving "func", the section name will be
    //         ".visaasm.func". The default name is .visaasm. It'll be apply
    //         if the given name is empty.
    // - size in byte
    // - Note that the alignment requirement of the section should be satisfied
    //   by the given data and size
    // - Note that the given data buffer have to be alive through ZEELFObjectBuilder
    void addSectionVISAAsm(std::string name, const uint8_t* data, uint64_t size);

    // add .misc section
    // - name: section name. Do not includes leading .misc in the given
    //         name. For example, giving "func", the section name will be
    //         ".misc.func". The default name is .misc. It'll be apply
    //         if the given name is empty.
    // - size in byte
    // - Note that the alignment requirement of the section should be satisfied
    //   by the given data and size
    // - Note that the given data buffer have to be alive through ZEELFObjectBuilder
    void addSectionMisc(std::string name, const uint8_t* data, uint64_t size);

    // add .note.intelgt.metrics section
    // - name: .note.intelgt.metrics. Do not includes leading .note.intelgt.metrics
    //         in the given name. For example, giving "func", the section name will be
    //         ".note.intelgt.metrics.func". The default name is .note.intelgt.metrics.
    //         It'll be applied if the given name is empty.
    // - size in byte
    // - Note that the alignment requirement of the section should be satisfied
    //   by the given data and size
    // - Note that the given data buffer have to be alive through ZEELFObjectBuilder
    void addSectionMetrics(std::string name, const uint8_t* data, uint64_t size);

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

    // add a relocation with rel format
    // This function will create a corresponding .rel.{targetSectionName} section if
    // not exist
    // - offset    : the binary offset of the section where the relocation
    //               will apply to. The section is denoted by sectionId
    // - symName   : the target symbol's name
    // - type      : the relocation name
    // - sectionId : the section id where the relocation is apply to
    void addRelRelocation(
        uint64_t offset, std::string symName, R_TYPE_ZEBIN type, SectionID sectionId);

    // add a relocation with rela format
    // This function will create a corresponding .rela.{targetSectionName} section if
    // not exist
    // - offset    : the binary offset of the section where the relocation
    //               will apply to. The section is denoted by sectionId
    // - symName   : the target symbol's name
    // - type      : the relocation name
    // - addend    : the addend value
    // - sectionId : the section id where the relocation is apply to
    void addRelaRelocation(
        uint64_t offset, std::string symName, R_TYPE_ZEBIN type, uint64_t addend, SectionID sectionId);

    // finalize - Finalize the ELF Object, write ELF file into given os
    // return number of written bytes
    uint64_t finalize(llvm::raw_pwrite_stream& os);

    // get an ID of a section
    // - name  : section name
    SectionID getSectionIDBySectionName(const char* name);

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
        Section& operator=(const Section&) = default;

    protected:
        SectionID m_id;
    };

    class StandardSection : public Section {
    public:
        StandardSection(std::string sectName, const uint8_t* data, uint64_t size,
            unsigned type, unsigned flags, uint32_t padding, uint32_t id)
            : Section(id), m_sectName(sectName), m_data(data), m_size(size), m_type(type),
              m_flags(flags), m_padding(padding)
        {}

        Kind getKind() const { return STANDARD; }

        // m_sectName - the final name presented in ELF section header
        // This field is required as the place holder for StringTable construction
        std::string m_sectName;
        const uint8_t* m_data;
        uint64_t m_size;
        // section type
        unsigned m_type;
        unsigned m_flags = 0;
        uint32_t m_padding;
    };

    class ZEInfoSection : public Section {
    public:
        ZEInfoSection(zeInfoContainer& zeinfo, uint32_t id)
            : Section(id), m_zeinfo(zeinfo)
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
            uint8_t type, SectionID sectionId)
            : m_name(name), m_addr(addr), m_size(size), m_binding(binding),
            m_type(type), m_sectionId(sectionId)
        {}

        std::string& name()            { return m_name;      }
        uint64_t     addr()      const { return m_addr;      }
        uint64_t     size()      const { return m_size;      }
        uint8_t      binding()   const { return m_binding;   }
        uint8_t      type()      const { return m_type;      }
        SectionID    sectionId() const { return m_sectionId; }

    private:
        std::string m_name;
        uint64_t m_addr;
        uint64_t m_size;
        uint8_t m_binding;
        uint8_t m_type;
        SectionID m_sectionId;
    };

    /// Relocation - present the relocation information of each entry.
    /// The relocation itself doesn't know if it's in rel or rela format.
    /// It's rel or rela depends on it's in RelocSection or RelaRelocSection
    class Relocation {
    public:
        Relocation(uint64_t offset, std::string symName, R_TYPE_ZEBIN type, uint64_t addend = 0)
            : m_offset(offset), m_symName(symName), m_type(type), m_addend(addend)
        {}

        uint64_t            offset()  const { return m_offset;  }
        const std::string&  symName() const { return m_symName; }
        R_TYPE_ZEBIN        type()    const { return m_type;    }
        uint64_t            addend()  const { return m_addend;  }

    private:
        uint64_t m_offset;
        std::string m_symName;
        R_TYPE_ZEBIN m_type;
        uint64_t m_addend;
    };

    typedef std::vector<StandardSection> StandardSectionListTy;
    typedef std::vector<Symbol> SymbolListTy;
    typedef std::vector<Relocation> RelocationListTy;

    class RelocSection : public Section {
    public:
        RelocSection(SectionID myID, SectionID targetID, std::string sectName, bool isRelFormat) :
            Section(myID), m_TargetID(targetID), m_sectName(sectName), m_isRelFormat (isRelFormat)
        {}

        Kind getKind() const { return RELOC; }
        bool isRelFormat() const { return m_isRelFormat; }

    public:
        // target section's id that this relocation section apply to
        SectionID m_TargetID;
        std::string m_sectName;
        RelocationListTy m_Relocations;

        // This is a rel or rela relocation format
        bool m_isRelFormat;
    };
    typedef std::vector<RelocSection> RelocSectionListTy;

private:
    Section& addStandardSection(
        std::string sectName, const uint8_t* data, uint64_t size, unsigned type,
        unsigned flags, uint32_t padding, uint32_t align, StandardSectionListTy& sections);

    // isRelFormat - rel or rela relocation format
    RelocSection& getOrCreateRelocSection(SectionID targetSectId, bool isRelFormat);

    std::string getSectionNameBySectionID(SectionID id);

private:
    // place holder for section default name
    const std::string m_TextName        = ".text";
    const std::string m_DataName        = ".data";
    const std::string m_BssName         = ".bss";
    const std::string m_SymTabName      = ".symtab";
    const std::string m_RelName         = ".rel";
    const std::string m_RelaName        = ".rela";
    const std::string m_SpvName         = ".spv";
    const std::string m_VISAAsmName     = ".visaasm";
    const std::string m_DebugName       = ".debug_info";
    const std::string m_ZEInfoName      = ".ze_info";
    const std::string m_GTPinInfoName   = ".gtpin_info";
    const std::string m_MiscName        = ".misc";
    const std::string m_CompatNoteName  = ".note.intelgt.compat";
    const std::string m_MetricsNoteName = ".note.intelgt.metrics";
    const std::string m_StrTabName      = ".strtab";

private:
    // 32 or 64 bit object
    bool m_is64Bit;

    // information used to generate .note.intelgt.compat
    PRODUCT_FAMILY m_productFamily = IGFX_UNKNOWN;
    GFXCORE_FAMILY m_gfxCoreFamily = IGFX_UNKNOWN_CORE;
    TargetMetadata m_metadata;

    StandardSectionListTy m_textSections;
    StandardSectionListTy m_dataAndbssSections; // data and bss sections
    StandardSectionListTy m_otherStdSections;
    RelocSectionListTy    m_relocSections; // rel and rela reloc sections

    // current section id
    SectionID m_sectionIdCount = 0;

    // every ze object contains at most one ze_info section
    std::unique_ptr<ZEInfoSection> m_zeInfoSection;
    SymbolListTy m_localSymbols;
    SymbolListTy m_globalSymbols;

};

/// ZEInfoBuilder - Build a zeInfoContainer for .ze_info section
class ZEInfoBuilder {
public:
    ZEInfoBuilder()
    {
        mContainer.version = PreDefinedAttrGetter::getVersionNumber();
    }

    zeInfoContainer& getZEInfoContainer()             { return mContainer; }
    const zeInfoContainer& getZEInfoContainer() const { return mContainer; }

    // empty - return true if there is no kernel/function info in it
    bool empty() const;

    /// --------- Helper functions for setup zeinfo contents -------------- ///
    // createKernel - create a zeInfoKernel and add it into zeInfoContainer
    zeInfoKernel& createKernel(const std::string& name);

    // createFunction - create a zeInfoFunction and add it into zeInfoContainer
    zeInfoFunction& createFunction(const std::string& name);

    // addGlobalHostAccessSymbol - create a zeInfo global_host_access_table section
    // which is used by Runtime to identify a global variable based on host name
    void addGlobalHostAccessSymbol(const std::string& device_name, const std::string& host_name);

    // addPayloadArgumentByPointer - add explicit kernel argument with pointer
    // type into given arg_list
    static zeInfoPayloadArgument& addPayloadArgumentByPointer(
        PayloadArgumentsTy& arg_list,
        int32_t offset,
        int32_t size,
        int32_t arg_index,
        PreDefinedAttrGetter::ArgAddrMode addrmode,
        PreDefinedAttrGetter::ArgAddrSpace addrspace,
        PreDefinedAttrGetter::ArgAccessType access_type,
        int32_t alignment = 0);

    // addPayloadArgumentByValue - add explicit kernel argument with pass by
    // value type into given arg_list
    static zeInfoPayloadArgument& addPayloadArgumentByValue(
        PayloadArgumentsTy& arg_list,
        int32_t offset,
        int32_t size,
        int32_t arg_index,
        int32_t source_offset);

    // addPayloadArgumentSampler - add explicit kernel argument for sampler
    // into given arg_list
    // The argument type will be set to by_pointer, and addr_space will be set to sampler
    static zeInfoPayloadArgument& addPayloadArgumentSampler(
        PayloadArgumentsTy& arg_list,
        int32_t offset,
        int32_t size,
        int32_t arg_index,
        int32_t sampler_index,
        PreDefinedAttrGetter::ArgAddrMode addrmode,
        PreDefinedAttrGetter::ArgAccessType access_type);

    // addPayloadArgumentImplicit - add non-user argument (implicit argument)
    // into given arg_list. The type must be local_size, group_size,
    // global_id_offset or private_base_stateless
    static zeInfoPayloadArgument& addPayloadArgumentImplicit(
        PayloadArgumentsTy& arg_list,
        PreDefinedAttrGetter::ArgType type,
        int32_t offset,
        int32_t size);

    // addPerThreadPayloadArgument - add a per-thread payload argument into
    // arg_list. Currently we only support local id as per-thread argument.
    // The given type must be packed_local_ids or local_id
    static zeInfoPerThreadPayloadArgument& addPerThreadPayloadArgument(
        PerThreadPayloadArgumentsTy& arg_list,
        PreDefinedAttrGetter::ArgType type,
        int32_t offset,
        int32_t size);

    // addBindingTableIndex - add a binding table index into given bti_list, with
    // corresponding kernel argument index
    static zeInfoBindingTableIndex& addBindingTableIndex(
        BindingTableIndicesTy& bti_list,
        int32_t bti_value,
        int32_t arg_index);

    // addPerThreadMemoryBuffer - add a memory buffer info with global or slm type
    // If adding buffer with "global" type, this API assume it is allocated per-hardware-thread
    // Use below addPerSIMTThreadGlobalMemoryBuffer API if attempting to add per-simt-thread global buffer
    static zeInfoPerThreadMemoryBuffer& addPerThreadMemoryBuffer(
        PerThreadMemoryBuffersTy& mem_buff_list,
        PreDefinedAttrGetter::MemBufferType type,
        PreDefinedAttrGetter::MemBufferUsage usage,
        int32_t size);

    // addScratchPerThreadMemoryBuffer - add a memory buffer info for scratch buffer
    // per_thread_memory_buffers::type set to scratch
    static zeInfoPerThreadMemoryBuffer& addScratchPerThreadMemoryBuffer(
        PerThreadMemoryBuffersTy& mem_buff_list,
        PreDefinedAttrGetter::MemBufferUsage usage,
        int32_t slot_id,
        int32_t size);

    // addPerSIMTThreadGlobalMemoryBuffer - add a memory buffer info
    // for global memory buffer with
    // per_thread_memory_buffers::type set to global
    // per_thread_memory_buffers::is_simt_thread set to true
    // Use addPerThreadMemoryBuffer if adding per-hardware-thread global memory buffer
    static zeInfoPerThreadMemoryBuffer& addPerSIMTThreadGlobalMemoryBuffer(
        PerThreadMemoryBuffersTy& mem_buff_list,
        PreDefinedAttrGetter::MemBufferUsage usage,
        int32_t size);

    // addExpPropertiesHasNonKernelArgLdSt - add experimental_properties for has-non-kernel-arg-load-store analysis
    // add experimental_properties::has_non_kernel_arg_load, experimental_properties::has_non_kernel_arg_store
    // and experimental_properties::has_non_kernel_arg_atomic
    // Note that zeInfoExperimentalProperties is made as a vector under kernel in the spec, because we want it only
    // present when needed. If it's not a vector, the attribute name will always present in final output even if
    // all of its sub-attributes are default and are not shown.
    static void addExpPropertiesHasNonKernelArgLdSt(zeInfoKernel& zekernel,
        bool hasNonKernelArgLoad, bool hasNonKernelArgStore, bool hasNonKernelArgAtomic);

private:
    zeInfoContainer mContainer;
};

} // end namespace zebin

#endif // ZE_ELF_OBJECT_BUILDER_HPP
