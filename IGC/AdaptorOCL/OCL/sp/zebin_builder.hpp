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

#pragma once

#include <vector>
#include <ZEELFObjectBuilder.hpp>
#include "sp_g8.h"

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

    // Set the given GfxCoreFamily value to elfFileHeader::e_machine. Also set the flag
    // TargetFlags::machineEntryUsesGfxCoreInsteadOfProductFamily.
    // The default value of e_machine is ProductFamily. This API will set e_machine to
    // given value which is supposed to be GfxCoreFamily.
    void setGfxCoreFamilyToELFMachine(uint32_t value);

    /// add kernel information. Also create kernel metadata inforamtion for .ze_info
    /// This function can be called several times for adding different kernel information
    /// into this ZEObject
    /// The given rawIsaBinary must be lived through the entire ZEBinaryBuilder life
    void createKernel(
        const char*  rawIsaBinary,
        unsigned int rawIsaBinarySize,
        const IGC::SOpenCLKernelInfo& annotations,
        const uint32_t grfSize);

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
    bool hasGlobalConstants(const IGC::SOpenCLProgramInfo& annotations);

    /// add data section for globals
    zebin::ZEELFObjectBuilder::SectionID addGlobals(
        const IGC::SOpenCLProgramInfo& annotations);
    bool hasGlobals(const IGC::SOpenCLProgramInfo& annotations);

    /// add spir-v section
    void addSPIRV(const uint8_t* data, uint32_t size);

    /// ------------ kernel scope helper functions ------------
    /// add gen binary
    zebin::ZEELFObjectBuilder::SectionID addKernelBinary(
        const std::string& kernelName, const char* kernelBinary,
        unsigned int kernelBinarySize);

    /// add execution environment
    void addKernelExecEnv(const IGC::SOpenCLKernelInfo& annotations,
                          zebin::zeInfoKernel& zeinfoKernel);

    /// add symbols of this kernel corresponding to kernek binary
    /// added by addKernelBinary
    void addSymbols(
        zebin::ZEELFObjectBuilder::SectionID kernelSectId,
        const IGC::SOpenCLKernelInfo& annotations);

    /// get symbol type and binding
    /// FIXME: this should be decided when symbol being created
    uint8_t getSymbolElfType(vISA::ZESymEntry& sym);
    uint8_t getSymbolElfBinding(vISA::ZESymEntry& sym);

    /// add relocations of this kernel corresponding to binary added by
    /// addKernelBinary. Assume that all relocations' target section is
    /// the added text section
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
    zebin::ZEELFObjectBuilder::SectionID mGlobalConstSectID;
    zebin::ZEELFObjectBuilder::SectionID mConstStringSectID;
    zebin::ZEELFObjectBuilder::SectionID mGlobalSectID;
};

} //namespace iOpenCL