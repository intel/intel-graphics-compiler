/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <vector>
#include <ZEELFObjectBuilder.hpp>
#include "sp_g8.h"
#include "llvm/BinaryFormat/ELF.h"
#include "CLElfLib/ElfReader.h"

namespace IGC
{
    struct SOpenCLKernelInfo;
    struct SOpenCLProgramInfo;
    class CBTILayout;
    class OpenCLProgramContext;
}

namespace vISA
{
    struct ZESymEntry;
}

namespace iOpenCL
{

/// ZEBinaryBuilder - Provides services to create a ZE Binary from given
/// SProgramOutput information
class ZEBinaryBuilder : DisallowCopy
{
public:
    // Setup ZEBin platform, and ELF header information. The program scope information
    // is also be parsed from SOpenCLProgramInfo in the constructor
    ZEBinaryBuilder(const PLATFORM plat, bool is64BitPointer,
        const IGC::SOpenCLProgramInfo& programInfo, const uint8_t* spvData, uint32_t spvSize);

    // Set the ProductFamily as the specified value.
    void setProductFamily(PRODUCT_FAMILY value);

    // Set the GfxCoreFamily as the specified value.
    void setGfxCoreFamily(GFXCORE_FAMILY value);

    /// add kernel information. Also create kernel metadata information for .ze_info
    /// This function can be called several times for adding different kernel information
    /// into this ZEObject
    /// The given rawIsaBinary must be lived through the entire ZEBinaryBuilder life
    void createKernel(
        const char*  rawIsaBinary,
        unsigned int rawIsaBinarySize,
        const IGC::SOpenCLKernelInfo& annotations,
        const uint32_t grfSize,
        const IGC::CBTILayout& layout,
        const std::string& visaasm,
        bool isProgramDebuggable);

    // getElfSymbol - find a symbol name in ELF binary and return a symbol entry
    // that will later be transformed to ZE binary format
    void getElfSymbol(CLElfLib::CElfReader* elfReader, const unsigned int symtabIdx, llvm::ELF::Elf64_Sym& symtabEntry,
        char*& symName);

    /// addElfSections - copy every section of ELF file (a buffer in memory) to zeBinary
    void addElfSections(void* elfBin, size_t elfSize);

    /// getBinaryObject - get the final ze object
    void getBinaryObject(llvm::raw_pwrite_stream& os);

    // getBinaryObject - write the final object into given Util::BinaryStream
    // Avoid using this function, which has extra buffer copy
    void getBinaryObject(Util::BinaryStream& outputStream);

    void printBinaryObject(const std::string& filename);

private:
    /// ------------ program scope helper functions ------------
    /// add program scope information. This function will be called in the ctor.
    /// The program scope information include global buffers (data sections for
    /// globals and global constants)
    /// ProgramScopeInfo must be prepared before kernel information. For example,
    /// Symbols are per-kernel information but they could possible refering to
    /// program-scope sections such as global buffer.
    void addProgramScopeInfo(const IGC::SOpenCLProgramInfo& programInfo);

    /// add data section for global constants
    void addGlobalConstants(const IGC::SOpenCLProgramInfo& annotations);

    /// add data section for globals
    void addGlobals(const IGC::SOpenCLProgramInfo& annotations);

    /// add spir-v section
    void addSPIRV(const uint8_t* data, uint32_t size);

    /// add runtime symbols
    void addRuntimeSymbols();

    /// add program scope symbols (e.g. symbols defined in global/const buffer)
    void addProgramSymbols(const IGC::SOpenCLProgramInfo& annotations);

    /// add program scope relocations (e.g. relocations for global/const buffer)
    void addProgramRelocations(const IGC::SOpenCLProgramInfo& annotations);

    /// ------------ kernel scope helper functions ------------
    /// add gen binary
    zebin::ZEELFObjectBuilder::SectionID addKernelBinary(
        const std::string& kernelName, const char* kernelBinary,
        unsigned int kernelBinarySize);

    /// add execution environment
    void addKernelExecEnv(const IGC::SOpenCLKernelInfo& annotations,
                          zebin::zeInfoKernel& zeinfoKernel);

    /// add experimental properties
    void addKernelExperimentalProperties(const IGC::SOpenCLKernelInfo& annotations,
        zebin::zeInfoKernel& zeinfoKernel);

    /// add symbols of this kernel corresponding to kernel binary
    /// added by addKernelBinary
    void addKernelSymbols(
        zebin::ZEELFObjectBuilder::SectionID kernelSectId,
        const IGC::SOpenCLKernelInfo& annotations);

    /// get symbol type
    /// FIXME: this should be decided when symbol being created
    uint8_t getSymbolElfType(const vISA::ZESymEntry& sym);

    /// addSymbol - a helper function to add a symbol which is defined in targetSect
    void addSymbol(const vISA::ZESymEntry& sym, uint8_t binding,
        zebin::ZEELFObjectBuilder::SectionID targetSect);

    /// add relocations of this kernel corresponding to binary added by
    /// addKernelBinary.
    void addKernelRelocations(
        zebin::ZEELFObjectBuilder::SectionID targetId,
        const IGC::SOpenCLKernelInfo& annotations);

    /// add local ids as per-thread payload argument
    void addLocalIds(uint32_t simdSize, uint32_t grfSize,
        bool has_local_id_x, bool has_local_id_y, bool has_local_id_z,
        zebin::zeInfoKernel& zeinfoKernel);

    /// add payload arguments and BTI info from IGC::SOpenCLKernelInfo
    /// payload arguments and BTI info have been added at
    /// COpenCLKernel::CreateZEPayloadArguments
    void addPayloadArgsAndBTI(
        const IGC::SOpenCLKernelInfo& annotations,
        zebin::zeInfoKernel& zeinfoKernel);

    /// add Memory buffer information
    void addMemoryBuffer(
        const IGC::SOpenCLKernelInfo& annotations,
        zebin::zeInfoKernel& zeinfoKernel);

    /// add gtpin_info section
    /// Add everything used to be in patch token iOpenCL::PATCH_TOKEN_GTPIN_INFO
    /// into gtpin_info section
    void addGTPinInfo(const IGC::SOpenCLKernelInfo& annotations);

    /// ------------ Verifier sub-functions ------------
    bool hasSystemKernel(
        const IGC::OpenCLProgramContext* clContext,
        const USC::SSystemThreadKernelOutput* pSystemThreadKernelOutput);

    bool hasSystemThreadSurface(const IGC::OpenCLProgramContext* clContext);

    /// Calculate correct (pure) size of ELF binary, because m_debugDataSize in kernel output
    /// contains something else.
    size_t calcElfSize(void* elfBin, size_t elfSize);

    /// add debug environment
    void addKernelDebugEnv(const IGC::SOpenCLKernelInfo& annotations,
                           const IGC::CBTILayout& layout,
                           zebin::zeInfoKernel& zeinfoKernel);

    /// add visasm of the kernel
    void addKernelVISAAsm(const std::string& kernel, const std::string& visaasm);

    /// add global_host_access_table section to .ze_info
    void addGlobalHostAccessInfo(const IGC::SOpenCLProgramInfo& annotations);

private:
    // mBuilder - Builder of a ZE ELF object
    zebin::ZEELFObjectBuilder mBuilder;

    // mZEInfoBuilder - Builder and holder of a zeInfoContainer, which will
    // be added into ZEELFObjectBuilder as .ze_info section
    zebin::ZEInfoBuilder mZEInfoBuilder;

    const PLATFORM mPlatform;
    G6HWC::SMediaHardwareCapabilities mHWCaps;

    /// sectionID holder for program scope sections
    /// There should be only one global, global constant buffer per program
    zebin::ZEELFObjectBuilder::SectionID mGlobalConstSectID = -1;
    zebin::ZEELFObjectBuilder::SectionID mConstStringSectID = -1;
    zebin::ZEELFObjectBuilder::SectionID mGlobalSectID = -1;
};

} //namespace iOpenCL
