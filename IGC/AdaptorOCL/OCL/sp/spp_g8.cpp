/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "llvm/Config/llvm-config.h"
#include "spp_g8.h"
#include "../../../Compiler/CodeGenPublic.h"
#include "program_debug_data.h"
#include "../../../common/SystemThread.h"
#include "../../../common/Types.hpp"
#include "../../../common/shaderOverride.hpp"
#include "../../../Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"

#include <iomanip>
#include <fstream>
#include "Probe/Assertion.h"

#include "llvm/ADT/ArrayRef.h"
#include "lldWrapper/Common/Driver.h"
#include "lld/Common/Driver.h"

#include "llvm/Support/Path.h"
#include "llvm/Support/FileSystem.h"

#if !defined(_MSC_VER) && !defined(__MINGW32__)
#include <unistd.h>
#else
#include <io.h>
#endif

namespace iOpenCL
{

extern RETVAL g_cInitRetValue;

const char* uniqueElfFileNamePart = "_%%%%%%%%";

ShaderHash CGen8OpenCLProgram::CLProgramCtxProvider::getProgramHash() const {
    return m_Context.hash;
}
bool CGen8OpenCLProgram::CLProgramCtxProvider::needsSystemKernel() const {
    const auto& options = m_Context.m_InternalOptions;
    return options.IncludeSIPCSR ||
        options.IncludeSIPKernelDebug ||
        options.IncludeSIPKernelDebugWithLocalMemory;
}
bool CGen8OpenCLProgram::CLProgramCtxProvider::isProgramDebuggable() const {
    return m_Context.m_InternalOptions.KernelDebugEnable;
}
bool CGen8OpenCLProgram::CLProgramCtxProvider::hasProgrammableBorderColor() const {
    return m_Context.m_DriverInfo.ProgrammableBorderColorInCompute();
}

bool CGen8OpenCLProgram::CLProgramCtxProvider::useBindlessMode() const {
    return m_Context.m_InternalOptions.UseBindlessMode;
}
bool CGen8OpenCLProgram::CLProgramCtxProvider::useBindlessLegacyMode() const {
    return m_Context.m_InternalOptions.UseBindlessLegacyMode;
}


CGen8OpenCLProgramBase::CGen8OpenCLProgramBase(PLATFORM platform,
                                               const CGen8OpenCLStateProcessor::IProgramContext& Ctx, const WA_TABLE& WATable)
    : m_Platform(platform),
      m_StateProcessor(platform, Ctx, WATable)
{
    m_ProgramScopePatchStream = new Util::BinaryStream;
}

CGen8OpenCLProgramBase::~CGen8OpenCLProgramBase()
{
    delete m_ProgramScopePatchStream;

    if (m_pSystemThreadKernelOutput)
    {
        SIP::CSystemThread::DeleteSystemThreadKernel(m_pSystemThreadKernelOutput);
    }
}

RETVAL CGen8OpenCLProgramBase::GetProgramBinary(
    Util::BinaryStream& programBinary,
    unsigned pointerSizeInBytes )
{
    RETVAL retValue = g_cInitRetValue;

    iOpenCL::SProgramBinaryHeader   header;

    memset( &header, 0, sizeof( header ) );

    header.Magic = iOpenCL::MAGIC_CL;
    header.Version = iOpenCL::CURRENT_ICBE_VERSION;
    header.Device = m_Platform.eRenderCoreFamily;
    header.GPUPointerSizeInBytes = pointerSizeInBytes;
    header.NumberOfKernels = m_KernelBinaries.size();
    header.SteppingId = m_Platform.usRevId;
    header.PatchListSize = int_cast<DWORD>(m_ProgramScopePatchStream->Size());

    if (IGC_IS_FLAG_ENABLED(DumpOCLProgramInfo))
    {
        DebugProgramBinaryHeader(&header, m_StateProcessor.m_oclStateDebugMessagePrintOut);
    }

    programBinary.Write( header );

    programBinary.Write( *m_ProgramScopePatchStream );

    for( auto& data : m_KernelBinaries )
    {
        programBinary.Write( *(data.kernelBinary) );
    }

    return retValue;
}

void CGen8OpenCLProgramBase::CreateProgramScopePatchStream(const IGC::SOpenCLProgramInfo& annotations)
{
    m_StateProcessor.CreateProgramScopePatchStream(annotations, *m_ProgramScopePatchStream);
}

CGen8OpenCLProgram::CGen8OpenCLProgram(PLATFORM platform, const IGC::OpenCLProgramContext& context)
    : m_Context(context),
      m_ContextProvider(context),
      CGen8OpenCLProgramBase(platform, m_ContextProvider, context.platform.getWATable())
{
}

CGen8OpenCLProgram::~CGen8OpenCLProgram()
{
    for (auto p : m_ShaderProgramList)
    {
        delete p;
    }
    m_ShaderProgramList.clear();
}

RETVAL CGen8OpenCLProgramBase::GetProgramDebugData(Util::BinaryStream& programDebugData)
{
    // Used by VC only
    RETVAL retValue = g_cInitRetValue;

    unsigned numDebugBinaries = 0;
    for (const auto& data : m_KernelBinaries)
    {
        if (data.vcKernelDebugData && data.vcKernelDebugData->Size() > 0)
        {
            numDebugBinaries++;
        }
    }

    if (numDebugBinaries)
    {
        iOpenCL::SProgramDebugDataHeaderIGC header;

        memset(&header, 0, sizeof(header));

        header.Magic = iOpenCL::MAGIC_CL;
        header.Version = iOpenCL::CURRENT_ICBE_VERSION;
        header.Device = m_Platform.eRenderCoreFamily;
        header.NumberOfKernels = numDebugBinaries;
        header.SteppingId = m_Platform.usRevId;

        programDebugData.Write(header);

        for (const auto& data : m_KernelBinaries)
        {
            if (data.vcKernelDebugData && data.vcKernelDebugData->Size() > 0)
            {
                programDebugData.Write(*data.vcKernelDebugData.get());
            }
        }
    }

    return retValue;
}

RETVAL CGen8OpenCLProgramBase::GetProgramDebugDataSize(size_t& totalDbgInfoBufferSize)
{
    RETVAL retValue = g_cInitRetValue;

    unsigned numDebugBinaries = 0;
    for (auto& data : m_KernelBinaries)
    {
        if (data.dbgInfo.header &&
            data.dbgInfo.header->Size() > 0 &&
            data.dbgInfo.dbgInfoBufferSize > 0)
        {
            numDebugBinaries++;
        }
    }

    totalDbgInfoBufferSize = 0;
    if (numDebugBinaries)
    {
        totalDbgInfoBufferSize += sizeof(iOpenCL::SProgramDebugDataHeaderIGC);
        for (auto& data : m_KernelBinaries)
        {
            if (!data.dbgInfo.header)
                continue;
            totalDbgInfoBufferSize += (size_t)data.dbgInfo.header->Size() +
                (size_t)(data.dbgInfo.dbgInfoBufferSize +
                data.dbgInfo.extraAlignBytes);
        }
    }

    return retValue;
}

RETVAL CGen8OpenCLProgramBase::GetProgramDebugData(char* dstBuffer, size_t dstBufferSize)
{
    RETVAL retValue = g_cInitRetValue;
    size_t offset = 0;

    auto Append = [&offset, dstBuffer, dstBufferSize](const void* src, size_t srcSize)
    {
        memcpy_s(dstBuffer + offset, dstBufferSize - offset, src, srcSize);
        offset += srcSize;
    };

    unsigned numDebugBinaries = 0;
    for (auto& data : m_KernelBinaries)
    {
        if (data.dbgInfo.header &&
            data.dbgInfo.header->Size() > 0 &&
            data.dbgInfo.dbgInfoBufferSize > 0)
        {
            numDebugBinaries++;
        }
    }

    if( numDebugBinaries )
    {
        iOpenCL::SProgramDebugDataHeaderIGC header;

        memset( &header, 0, sizeof( header ) );

        header.Magic = iOpenCL::MAGIC_CL;
        header.Version = iOpenCL::CURRENT_ICBE_VERSION;
        header.Device = m_Platform.eRenderCoreFamily;
        header.NumberOfKernels = numDebugBinaries;
        header.SteppingId = m_Platform.usRevId;

        Append(&header, sizeof(header));

        const uint64_t zero = 0;
        for (auto& data : m_KernelBinaries)
        {
            if (data.dbgInfo.header &&
                data.dbgInfo.header->Size() > 0 &&
                data.dbgInfo.dbgInfoBufferSize > 0)
            {
                Append((void*)data.dbgInfo.header->GetLinearPointer(), (size_t)data.dbgInfo.header->Size());
                Append(data.dbgInfo.dbgInfoBuffer, data.dbgInfo.dbgInfoBufferSize);
                IGC_ASSERT(data.dbgInfo.extraAlignBytes <= sizeof(zero));
                Append((void*)&zero, data.dbgInfo.extraAlignBytes);
            }
        }
    }

    return retValue;
}

void dumpOCLKernelBinary(
    const IGC::COpenCLKernel *Kernel,
    const KernelData &data)
{
#if LLVM_VERSION_MAJOR >= 7
    using namespace IGC;
    using namespace IGC::Debug;

    auto *Ctx = Kernel->GetContext();

    auto &kernelName = Kernel->m_kernelInfo.m_kernelName;

    auto name = DumpName(IGC::Debug::GetShaderOutputName())
        .Hash(Ctx->hash)
        .Type(ShaderType::OPENCL_SHADER)
        .PostFix(kernelName)
        .Extension("kernbin");

    const auto &KernBin = data.kernelBinary;
    IGC_ASSERT(KernBin);

    std::error_code EC;
    llvm::raw_fd_ostream f(name.str(), EC);
    if (!EC)
        f.write(KernBin->GetLinearPointer(), (size_t)KernBin->Size());
#endif
}

void overrideOCLKernelBinary(
    const IGC::COpenCLKernel *Kernel,
    KernelData &data)
{
    using namespace IGC;
    using namespace IGC::Debug;

    auto *Ctx = Kernel->GetContext();

    auto &kernelName = Kernel->m_kernelInfo.m_kernelName;

    auto name = DumpName(IGC::Debug::GetShaderOutputName())
        .Hash(Ctx->hash)
        .Type(ShaderType::OPENCL_SHADER)
        .PostFix(kernelName)
        .Extension("kernbin");

    std::string Path = name.overridePath();

    std::ifstream f(Path, std::ios::binary);
    if (!f.is_open())
        return;

    appendToShaderOverrideLogFile(Path, "OVERRIDDEN: ");

    f.seekg(0, f.end);
    int newBinarySize = (int)f.tellg();
    f.seekg(0, f.beg);

    auto &KernBin = data.kernelBinary;
    KernBin.reset();

    KernBin = std::make_unique<Util::BinaryStream>();

    std::unique_ptr<char[]> Buf(new char[newBinarySize]);
    f.read(Buf.get(), newBinarySize);

    IGC_ASSERT_MESSAGE(f.good(), "Not fully read!");

    KernBin->Write(Buf.get(), newBinarySize);
}

void dumpOCLCos(const IGC::COpenCLKernel *Kernel, const std::string &stateDebugMsg) {
    IGC::CodeGenContext* context = Kernel->GetContext();

    auto dumpName =
        IGC::Debug::DumpName(IGC::Debug::GetShaderOutputName())
        .Type(ShaderType::OPENCL_SHADER)
        .Hash(context->hash)
        .StagedInfo(context);

    std::string kernelName = Kernel->m_kernelInfo.m_kernelName;
    const int MAX_KERNEL_NAME = 180;

    // Shorten kernel name to avoid issues with too long file name
    if (kernelName.size() > MAX_KERNEL_NAME)
    {
        kernelName.resize(MAX_KERNEL_NAME);
    }
    dumpName = dumpName.PostFix(kernelName);

    dumpName = dumpName.DispatchMode(Kernel->m_ShaderDispatchMode);
    dumpName = dumpName.SIMDSize(Kernel->m_dispatchSize).Retry(context->m_retryManager.GetRetryId()).Extension("cos");

    auto dump = IGC::Debug::Dump(dumpName, IGC::Debug::DumpType::COS_TEXT);

    IGC::Debug::DumpLock();
    dump.stream() << stateDebugMsg;
    IGC::Debug::DumpUnlock();
}

// Build a name for an ELF temporary file. If uniqueLockFileName contains any % characters then
// these characters are replaced with random characters 0-9 or a-f, and such a uniquely named
// file, where this name is returned in resultUniqueLockFileName, is created in a system temporary
// directory to reserve this name.
bool createElfFileName(std::string &name, unsigned int maxNameLen, SIMDMode simdMode, int id,
    SmallVectorImpl<char> &uniqueLockFileName, SmallVectorImpl<char> &resultUniqueLockFileName,
    std::string& resultFullElfFileNameStr)
{
    SmallString<64> tempDir;    // Do not worry about its size, because system_temp_directory() appends if needed.
    bool retValue = true;

    // Location of temporary input and output files required by the linker of ELF files
    // is a system temporary directory
    llvm::sys::path::system_temp_directory(false, tempDir);
#if defined(_WIN64) || defined(_WIN32)
    tempDir.append("\\");
#else
    tempDir.append("/");
#endif // defined(_WIN64) || defined(_WIN32)

    std::string pidStr =
#if LLVM_ON_UNIX
        std::to_string(getpid());
#elif defined(_WIN32)
        std::to_string(::GetCurrentProcessId());
#else
        "";
#endif

    resultFullElfFileNameStr.append(tempDir.c_str());
    resultFullElfFileNameStr.append(name.substr(0, maxNameLen - 1).c_str());
    resultFullElfFileNameStr.append("_simd");
    resultFullElfFileNameStr.append(to_string(simdMode == SIMDMode::SIMD8 ? 8 : simdMode == SIMDMode::SIMD16 ? 16 : 32));
    resultFullElfFileNameStr.append("_");
    resultFullElfFileNameStr.append(to_string(id));
    if (!pidStr.empty())
    {
        resultFullElfFileNameStr.append("_");
        resultFullElfFileNameStr.append(pidStr);
    }

    std::string uniqueLockFileNameStr = uniqueLockFileName.data();
    if (uniqueLockFileNameStr.find('%') < uniqueLockFileNameStr.size())
    {
        int uniqueLockFileID = 0;
        // Every '%' will be replaced with a random character (0-9 or a-f), taking care of multithreaded compilations
        if (std::error_code EC = sys::fs::createUniqueFile(
            uniqueLockFileName, uniqueLockFileID, resultUniqueLockFileName))
        {
            IGC_ASSERT_MESSAGE(false, "A uniquely named file not created");
            retValue = false;
        }
    }
    else
    {
        resultUniqueLockFileName = uniqueLockFileName;
    }

    resultFullElfFileNameStr.append(resultUniqueLockFileName.data());
    resultFullElfFileNameStr.append(".elf");

    return retValue;
}

void CGen8OpenCLProgram::GetZEBinary(
    llvm::raw_pwrite_stream& programBinary,
    unsigned pointerSizeInBytes,
    const char* spv, uint32_t spvSize,
    const char* metrics, uint32_t metricsSize,
    const char* buildOptions, uint32_t buildOptionsSize)
{
    std::vector<std::unique_ptr<llvm::MemoryBuffer>> elfStorage;

    ZEBinaryBuilder zebuilder(m_Platform, pointerSizeInBytes == 8,
        m_Context.m_programInfo,
        (const uint8_t*)spv, spvSize,
        (const uint8_t*)metrics, metricsSize,
        (const uint8_t*)buildOptions, buildOptionsSize);
    zebuilder.setProductFamily(m_Platform.eProductFamily);

    std::vector<string> elfVecNames;      // Vector of parameters for the linker, contains in/out ELF file names and params
    std::vector<char*> elfVecPtrs;        // Vector of pointers to the elfVecNames vector elements
    SIMDMode simdMode = SIMDMode::SIMD8;  // Currently processed kernel's SIMD

    SmallString<64> tempDir;  // Do not worry its size, because system_temp_directory() appends if needed.
    // Location of temporary input and output files required by the linker is a system temporary directory
    llvm::sys::path::system_temp_directory(false, tempDir);
#if defined(_WIN64) || defined(_WIN32)
    tempDir.append("\\");
#else
    tempDir.append("/");
#endif // defined(_WIN64) || defined(_WIN32)

    const unsigned int maxElfFileNameLength = 512;
    std::string elfLinkerLogName = "lldLinkLogName"; // First parameter for the linker, just a log name or a program name if this linker would be external.
    SmallString<16> uniqueLockFilePartStr(uniqueElfFileNamePart);
    std::string linkedElfFileNameStr = "";           // Output file name created and filled by the linker
    int kernelID = 0;                                // Index of kernels; inserted into temporary ELF files names
    bool elfTmpFilesError = false;                   // Set if something goes wrong with temporary files or the linker.

    // If a single kernel in a program then neither temporary files are created nor the linker is in use,
    // hence ELF data is taken directly from the first found kernel's output (i.e. from m_debugData).
    IGC::SProgramOutput* pFirstKernelOutput = nullptr;
    bool isDebugInfo = true;  // If a kernel does not contain debug info then this flag will be changed to false.
    IGC::CodeGenContext* ctx = nullptr;

    for (auto pKernel : m_ShaderProgramList)
    {
        IGC::COpenCLKernel* simd8Shader = static_cast<IGC::COpenCLKernel*>(pKernel->GetShader(SIMDMode::SIMD8));
        IGC::COpenCLKernel* simd16Shader = static_cast<IGC::COpenCLKernel*>(pKernel->GetShader(SIMDMode::SIMD16));
        IGC::COpenCLKernel* simd32Shader = static_cast<IGC::COpenCLKernel*>(pKernel->GetShader(SIMDMode::SIMD32));

        // Determine how many simd modes we have per kernel
        // FIXME: We actually expect only one simd mode per kernel. There should not be multiple SIMD mode available
        // for one kernel (runtime cannot support that). So these check can be simplified
        std::vector<IGC::COpenCLKernel*> kernelVec;
        if ((m_Context.m_DriverInfo.sendMultipleSIMDModes() || m_Context.m_enableSimdVariantCompilation)
            && (m_Context.getModuleMetaData()->csInfo.forcedSIMDSize == 0))
        {
            // For multiple SIMD modes, send SIMD modes in descending order
            if (IGC::COpenCLKernel::IsValidShader(simd32Shader))
                kernelVec.push_back(simd32Shader);
            if (IGC::COpenCLKernel::IsValidShader(simd16Shader))
                kernelVec.push_back(simd16Shader);
            if (IGC::COpenCLKernel::IsValidShader(simd8Shader))
                kernelVec.push_back(simd8Shader);

            if (IGC_IS_FLAG_ENABLED(ZeBinCompatibleDebugging))
            {
                IGC_ASSERT_MESSAGE(false, "Missing ELF linking support for multiple SIMD modes");
            }
        }
        else
        {
            if (IGC::COpenCLKernel::IsValidShader(simd32Shader))
            {
                kernelVec.push_back(simd32Shader);
                simdMode = SIMDMode::SIMD32;
            }
            else if (IGC::COpenCLKernel::IsValidShader(simd16Shader))
            {
                kernelVec.push_back(simd16Shader);
                simdMode = SIMDMode::SIMD16;
            }
            else if (IGC::COpenCLKernel::IsValidShader(simd8Shader))
            {
                kernelVec.push_back(simd8Shader);
                simdMode = SIMDMode::SIMD8;
            }
        }

        for (auto kernel : kernelVec)
        {
            IGC::SProgramOutput* pOutput = kernel->ProgramOutput();
            kernelID++;  // 0 is reserved for an output linked file name

            zebuilder.createKernel(
                (const char*)pOutput->m_programBin,
                pOutput->m_programSize,
                kernel->m_kernelInfo,
                kernel->getGRFSize(),
                m_Context.btiLayout,
                pOutput->m_VISAAsm,
                m_ContextProvider.isProgramDebuggable());

            // FIXME: Handle IGC_IS_FLAG_ENABLED(ShaderDumpEnable) and
            // IGC_IS_FLAG_ENABLED(ShaderOverride)

            // ... Create the debug data binary streams

            if (pOutput->m_debugDataSize == 0)
                isDebugInfo = false;
            if (IGC_IS_FLAG_ENABLED(ZeBinCompatibleDebugging) && isDebugInfo)
            {
                const unsigned int rsrvdForAllButFullName = 64;  // characters will be used for temporary ELF file names.
                unsigned int spaceAvailableForKernelName = maxElfFileNameLength - rsrvdForAllButFullName - tempDir.size();

                if (m_ShaderProgramList.size() == 1 && kernelVec.size() == 1)
                {
                    // The first and only kernel found.
                    pFirstKernelOutput = pOutput; // There is only one kernel in the program so no linking will be required
                }
                else
                {
                    if (!elfVecNames.size())
                    {
                        // The first of multiple kernels found:
                        // - build for the linker a name of an output file, and
                        // - add a log name, as a first element, to a vector of the linker parameters.

                        // Build a temporary output file name (with a path) for the linker

                        if (!createElfFileName(
                            kernel->m_kernelInfo.m_kernelName,
                            spaceAvailableForKernelName,  // maxNameLen
                            simdMode,
                            0,                      // kernel ID
                            uniqueLockFilePartStr,
                            uniqueLockFilePartStr,  // unique part only result, i.e. _%%%%%%%% replaced with unique sequence of characters
                            linkedElfFileNameStr))  // full name with a path result
                        {
                            IGC_ASSERT_MESSAGE(false, "A unique name for a linked ELF file not created");
                        }
                        elfVecNames.push_back(elfLinkerLogName);  // 1st element in this vector of names; required by the linker
                        elfVecPtrs.push_back((char*)(elfVecNames.at(elfVecNames.size() - 1).c_str()));

                        ctx = kernel->GetContext();  // Remember context for future usage regarding warning emission (if needed).
                    }

                    std::string elfFileNameStr = "";
                    // Build a temporary input file name (with a path) for the linker
                    if (!createElfFileName(
                        kernel->m_kernelInfo.m_kernelName,
                        spaceAvailableForKernelName,  // maxNameLen
                        simdMode,
                        kernelID,
                        uniqueLockFilePartStr,  // unique part is the same unique sequence of characters got earlier for a linked file
                        uniqueLockFilePartStr,  // unique part only result should remain the same as the previous parameter
                        elfFileNameStr))        // full name with a path result
                    {
                        IGC_ASSERT_MESSAGE(false, "A unique name for an input ELF file not created");
                    }

                    int writeFD = 0;
                    sys::fs::CreationDisposition disp = sys::fs::CreationDisposition::CD_CreateAlways;
                    sys::fs::OpenFlags flags = sys::fs::OpenFlags::OF_None;
                    unsigned int mode = sys::fs::all_read | sys::fs::all_write;
                    auto EC = sys::fs::openFileForReadWrite(Twine(elfFileNameStr), writeFD, disp, flags, mode);
                    if (!EC)
                    {
                        raw_fd_ostream OS(writeFD, true, true); // shouldClose=true, unbuffered=true
                        OS << StringRef((const char*)pOutput->m_debugData, pOutput->m_debugDataSize);
                        // close(writeFD) is not required due to shouldClose parameter in ostream

                        // A temporary input ELF file filled, so its name can be added to a vector of parameters for the linker
                        elfVecNames.push_back(elfFileNameStr);
                        elfVecPtrs.push_back((char*)(elfVecNames.at(elfVecNames.size() - 1).c_str()));
                    }
                    else
                    {
                        ctx->EmitError("ELF file opening error", nullptr);
                        elfTmpFilesError = true; // Handle this error at the end of this function
                        break;
                    }
                }
            }
        }
    }

    if (IGC_IS_FLAG_ENABLED(ZeBinCompatibleDebugging) && isDebugInfo)
    {
        if (!elfTmpFilesError)
        {
            if (elfVecNames.size() == 0)
            {
                // if kernel was compiled only to visaasm, the output will be empty for gen binary.
                if (pFirstKernelOutput) {
                    // Single kernel in a program, no ELF linking required
                    // Copy sections one by one from ELF file to zeBinary with relocations adjusted.
                    zebuilder.addElfSections(pFirstKernelOutput->m_debugData, pFirstKernelOutput->m_debugDataSize);
                }
            }
            else
            {
                // Multiple kernels in a program, ELF linking required before moving debuig info to zeBinary

                IGC_ASSERT_MESSAGE(elfVecNames.size() > 2, "Unexpected number of parameters for the linker"); // 1st name is elfLinkerLog

                std::string linkedElfFileNameStrWithParam = "-o" + linkedElfFileNameStr;
                elfVecNames.push_back(linkedElfFileNameStrWithParam);
                elfVecPtrs.push_back((char*)(elfVecNames.at(elfVecNames.size() - 1).c_str()));

                std::string elfLinkerOpt2 = "--relocatable";
                elfVecNames.push_back(elfLinkerOpt2);
                elfVecPtrs.push_back((char*)(elfVecNames.at(elfVecNames.size() - 1).c_str()));

                // TODO: remove if not needed
                //std::string elfLinkerOpt1 = "--emit-relocs";  // "-q"
                //elfVecNames.push_back(elfLinkerOpt1);
                //elfVecPtrs.push_back((char*)(elfVecNames.at(elfVecNames.size() - 1).c_str()));

                auto elfArrRef = makeArrayRef(elfVecPtrs);
                std::string linkErrStr = "";
                llvm::raw_string_ostream linkErr(linkErrStr);

                std::string linkOutStr = "";
                llvm::raw_string_ostream linkOut(linkOutStr);

                constexpr bool canExitEarly = false;
                if (IGCLLD::elf::link(elfArrRef, canExitEarly, linkOut, linkErr))
                {
                    // Multiple ELF files linked.
                    // Copy the data from the linked file to a memory, what will be a source location
                    // for transmission debug info from the linked ELF to zeBinary

                    // Check the size of the linked file to know how large memory space should be allocated
                    uint64_t linkedElfSize = 0;
                    auto ECfileSize = sys::fs::file_size(Twine(linkedElfFileNameStr), linkedElfSize);
                    if (!ECfileSize && linkedElfSize > 0)
                    {
                        SmallString<maxElfFileNameLength> resultLinkedPath;
                        Expected<sys::fs::file_t> FDOrErr = sys::fs::openNativeFileForRead(
                            Twine(linkedElfFileNameStr), sys::fs::OF_UpdateAtime, &resultLinkedPath);
                        std::error_code ECfileOpen;
                        if (FDOrErr)
                        {
                            ErrorOr<std::unique_ptr<MemoryBuffer>> MBOrErr =
                                MemoryBuffer::getOpenFile(*FDOrErr, Twine(linkedElfFileNameStr), -1, false); // -1 of FileSize
                            sys::fs::closeFile(*FDOrErr);
                            if (MBOrErr)
                            {
                                // Copy sections one by one from ELF file to zeBinary with relocations adjusted.
                                elfStorage.push_back(std::move(MBOrErr.get()));
                                zebuilder.addElfSections((void*)elfStorage.back().get()->getBufferStart(), (size_t)linkedElfSize);
                            }
                            else
                            {
                                ECfileOpen = MBOrErr.getError();
                                ctx->EmitError("ELF linked file cannot be buffered", nullptr);
                                ctx->EmitError(ECfileOpen.message().c_str(), nullptr);
                                elfTmpFilesError = true; // Handle this error also below
                            }
                        }
                        else
                        {
                            ECfileOpen = errorToErrorCode(FDOrErr.takeError());
                            ctx->EmitError("ELF linked file cannot be opened", nullptr);
                            ctx->EmitError(ECfileOpen.message().c_str(), nullptr);
                            elfTmpFilesError = true; // Handle this error also below

                        }
                    }
                    else
                    {
                        ctx->EmitError("ELF linked file size error", nullptr);
                        elfTmpFilesError = true; // Handle this error also below
                    }
                }
                else
                {
                    linkErr.str();  // Flush contents to the associated string
#if LLVM_VERSION_MAJOR >= 10
                    linkOut.str();  // Flush contents to the associated string
                    linkErrStr.append(linkOutStr);
#endif // LLVM_VERSION_MAJOR
                    if (!linkErrStr.empty())
                    {
                        ctx->EmitError(linkErrStr.c_str(), nullptr);
                        elfTmpFilesError = true; // Handle this error also below
                    }
                }
            }
        }

        if (elfTmpFilesError)
        {
            // Nothing to do with the linker when any error with temporary files occured.
            ctx->EmitError("ZeBinary will not contain correct debug info due to an ELF temporary files error", nullptr);
        }

        if (IGC_IS_FLAG_DISABLED(ElfTempDumpEnable))
        {
            // Remove all temporary input ELF files
            for (auto elfFile : elfVecNames)
            {
                if (elfFile.compare(elfLinkerLogName.c_str()))
                {
                    sys::fs::remove(Twine(elfFile), true); // true=ignore non-existing
                        kernelID--;
                        if (!kernelID)  // elfVecNames may contain also options for the linker,
                            break;      // hence finish input files removal when all input files removed.
                }
            }

            // Also remove a temporary linked output ELF file...
            sys::fs::remove(Twine(linkedElfFileNameStr), true); // true=ignore non-existing

            // ...and finally a unique locked file.
            sys::fs::remove(Twine(uniqueLockFilePartStr), true); // true=ignore non-existing
        }
    }

    zebuilder.getBinaryObject(programBinary);
}

void CGen8OpenCLProgram::CreateKernelBinaries()
{
    for (auto pKernel : m_ShaderProgramList)
    {
        IGC::COpenCLKernel* simd8Shader = static_cast<IGC::COpenCLKernel*>(pKernel->GetShader(SIMDMode::SIMD8));
        IGC::COpenCLKernel* simd16Shader = static_cast<IGC::COpenCLKernel*>(pKernel->GetShader(SIMDMode::SIMD16));
        IGC::COpenCLKernel* simd32Shader = static_cast<IGC::COpenCLKernel*>(pKernel->GetShader(SIMDMode::SIMD32));

        // Determine how many simd modes we have per kernel
        std::vector<IGC::COpenCLKernel*> kernelVec;
        if ((m_Context.m_DriverInfo.sendMultipleSIMDModes() || m_Context.m_enableSimdVariantCompilation)
            && (m_Context.getModuleMetaData()->csInfo.forcedSIMDSize == 0))
        {
            // For multiple SIMD modes, send SIMD modes in descending order
            if (IGC::COpenCLKernel::IsValidShader(simd32Shader))
                kernelVec.push_back(simd32Shader);
            if (IGC::COpenCLKernel::IsValidShader(simd16Shader))
                kernelVec.push_back(simd16Shader);
            if (IGC::COpenCLKernel::IsValidShader(simd8Shader))
                kernelVec.push_back(simd8Shader);
        }
        else
        {
            if (IGC::COpenCLKernel::IsValidShader(simd32Shader))
                kernelVec.push_back(simd32Shader);
            else if (IGC::COpenCLKernel::IsValidShader(simd16Shader))
                kernelVec.push_back(simd16Shader);
            else if (IGC::COpenCLKernel::IsValidShader(simd8Shader))
                kernelVec.push_back(simd8Shader);
        }

        for (const auto& kernel : kernelVec)
        {
            IGC::SProgramOutput* pOutput = kernel->ProgramOutput();

            // Create the kernel binary streams
            KernelData data;
            data.kernelBinary = std::make_unique<Util::BinaryStream>();

            m_StateProcessor.CreateKernelBinary(
                (const char*)pOutput->m_programBin,
                pOutput->m_programSize,
                kernel->m_kernelInfo,
                m_Context.m_programInfo,
                m_Context.btiLayout,
                *(data.kernelBinary),
                m_pSystemThreadKernelOutput,
                pOutput->m_unpaddedProgramSize);

            if (IGC_IS_FLAG_ENABLED(EnableCosDump))
                  dumpOCLCos(kernel,
                             m_StateProcessor.m_oclStateDebugMessagePrintOut);

            if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable))
                dumpOCLKernelBinary(kernel, data);

            if (IGC_IS_FLAG_ENABLED(ShaderOverride))
                overrideOCLKernelBinary(kernel, data);

            IGC_ASSERT(data.kernelBinary && data.kernelBinary->Size() > 0);

            // Create the debug data binary streams
            if (pOutput->m_debugDataSize > 0 || pOutput->m_debugDataGenISASize > 0)
            {
                data.dbgInfo.header = std::make_unique<Util::BinaryStream>();

                m_StateProcessor.CreateKernelDebugData(
                    (const char*)pOutput->m_debugData,
                    pOutput->m_debugDataSize,
                    (const char*)pOutput->m_debugDataGenISA,
                    pOutput->m_debugDataGenISASize,
                    kernel->m_kernelInfo.m_kernelName,
                    data.dbgInfo);
            }

            m_StateProcessor.m_oclStateDebugMessagePrintOut.clear();
            m_KernelBinaries.push_back(std::move(data));
        }
    }
}

} // namespace iOpenCL
