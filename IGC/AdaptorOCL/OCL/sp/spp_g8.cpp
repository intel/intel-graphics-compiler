/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "llvm/Config/llvm-config.h"
#include "spp_g8.h"
#include "Compiler/CodeGenPublic.h"
#include "program_debug_data.h"
#include "IGC/common/SystemThread.h"
#include "IGC/common/Types.hpp"
#include "IGC/common/shaderOverride.hpp"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"

#include <iomanip>
#include <iostream>
#include <fstream>
#include "Probe/Assertion.h"

#include "llvm/ADT/ArrayRef.h"
#include "lldWrapper/Common/Driver.h"
#include "lld/Common/Driver.h"

#include "llvm/Support/Path.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/JSON.h"

#if !defined(_MSC_VER) && !defined(__MINGW32__)
#include <unistd.h>
#else
#include <io.h>
#endif

namespace iOpenCL {

CGen8OpenCLProgramBase::CGen8OpenCLProgramBase(PLATFORM platform) : m_Platform(platform) {}

CGen8OpenCLProgramBase::~CGen8OpenCLProgramBase() {
  if (m_pSystemThreadKernelOutput) {
    SIP::CSystemThread::DeleteSystemThreadKernel(m_pSystemThreadKernelOutput);
  }
}

CGen8OpenCLProgram::CGen8OpenCLProgram(PLATFORM platform, const IGC::OpenCLProgramContext &context)
    : m_Context(context), CGen8OpenCLProgramBase(platform) {}

CGen8OpenCLProgram::~CGen8OpenCLProgram() { m_ShaderProgramList.clear(); }

void CGen8OpenCLProgram::clearBeforeRetry() {
  for (auto &P : m_ShaderProgramList) {
    P->clearBeforeRetry();
  }
}

static void dumpZEInfo(const IGC::CodeGenContext &Ctx, ZEBinaryBuilder &ZEBuilder, bool toConsole = false) {
  if (toConsole) {
    llvm::SmallVector<char, 1024> buf;
    llvm::raw_svector_ostream os(buf);
    ZEBuilder.printZEInfo(os);
    std::cout << os.str().str() << std::endl;
    return;
  }

  auto filename = IGC::Debug::DumpName(IGC::Debug::GetShaderOutputName())
                      .Hash(Ctx.hash)
                      .Type(ShaderType::OPENCL_SHADER)
                      .Extension("zeinfo");
  if (filename.allow()) {
    ZEBuilder.printZEInfo(filename.str());
  }
}

static std::string getKernelDumpName(const IGC::COpenCLKernel *kernel) {
  std::string kernelName(kernel->m_kernelInfo.m_kernelName);
  kernel->getShaderFileName(kernelName);

  auto *context = kernel->GetContext();
  auto name = IGC::Debug::DumpName(IGC::Debug::GetShaderOutputName())
                  .Hash(context->hash)
                  .Type(ShaderType::OPENCL_SHADER)
                  .PostFix(kernelName)
                  .SIMDSize(kernel->m_SIMDSize)
                  .Extension("elf");
  return name.RelativePath();
}

static std::string getTempFileName(const IGC::COpenCLKernel *kernel) {
  SmallString<128> FileName;
  std::error_code EC;

  EC = sys::fs::getPotentiallyUniqueTempFileName("ze", "elf", FileName);
  if (EC) {
    IGC::CodeGenContext *ctx = kernel->GetContext();
    if (ctx) {
      ctx->EmitError(
          (std::string("unable to create temporary file for kernel: ") + kernel->m_kernelInfo.m_kernelName).c_str(),
          nullptr);
    }
    return "";
  }
  return FileName.c_str();
}

void CGen8OpenCLProgramBase::addElfKernelMapping(const std::string &elfFileName, const std::string &kernelName) {
  if (!elfMapEntries) {
    elfMapEntries = std::make_unique<llvm::json::Array>();
  }
  llvm::json::Value elfKernelMapping = llvm::json::Object{{"filename", elfFileName}, {"kernel", kernelName}};
  elfMapEntries->push_back(elfKernelMapping);
}

bool CGen8OpenCLProgramBase::createElfKernelMapFile(const llvm::Twine &FilePath) {
  int writeFD = 0;
  sys::fs::CreationDisposition disp = sys::fs::CreationDisposition::CD_CreateAlways;
  sys::fs::OpenFlags flags = sys::fs::OpenFlags::OF_None;
  unsigned int mode = sys::fs::all_read | sys::fs::all_write;
  auto EC = sys::fs::openFileForReadWrite(FilePath, writeFD, disp, flags, mode);
  if (!EC) {
    raw_fd_ostream OS(writeFD, true, true); // shouldClose=true, unbuffered=true
    OS << llvm::formatv("{0:2}", llvm::json::Value(std::move(*elfMapEntries.release())));
    // close(writeFD) is not required due to shouldClose parameter in ostream
    return true;
  }
  return false;
}

bool CGen8OpenCLProgramBase::dumpElfKernelMapFile(IGC::CodeGenContext *Ctx) {
  if (IGC_IS_FLAG_DISABLED(ElfTempDumpEnable)) {
    // Silently skip JSON generation
    return true;
  }

  SmallString<64> elfMapPath;
  std::error_code EC;
  EC = sys::fs::getPotentiallyUniqueTempFileName("ze_map", "json", elfMapPath);
  if (EC) {
    if (Ctx) {
      Ctx->EmitError("unable to create temporary file for kernel mapping", nullptr);
    }
    return false;
  }
  return createElfKernelMapFile(elfMapPath);
}

bool CGen8OpenCLProgram::GetZEBinary(llvm::raw_pwrite_stream &programBinary, unsigned pointerSizeInBytes,
                                     const char *spv, uint32_t spvSize, const char *metrics, uint32_t metricsSize,
                                     const char *buildOptions, uint32_t buildOptionsSize) {
  std::vector<std::unique_ptr<llvm::MemoryBuffer>> elfStorage;
  bool elfTmpFilesError = false; // error creating temp files
  bool retValue = true;

  ZEBinaryBuilder zebuilder(m_Platform, pointerSizeInBytes == 8, m_Context.m_programInfo, (const uint8_t *)spv, spvSize,
                            (const uint8_t *)metrics, metricsSize, (const uint8_t *)buildOptions, buildOptionsSize);
  zebuilder.setProductFamily(m_Platform.eProductFamily);
  zebuilder.setGfxCoreFamily(m_Platform.eRenderCoreFamily);
  zebuilder.setVISAABIVersion(m_Context.platform.getVISAABIVersion());
  zebuilder.setGmdID(m_Platform.sRenderBlockID);

  //
  // Creating ZE binary requires linking individual ELF files,
  // containing kernel code and debug data (DWARF). Due to usual
  // naming convention of the kernels, we may exceed the size of
  // the file name (usually ~250 chars). So, each ELF file is
  // created using LLVM's TempFile functionality and a JSON file,
  // containing mapping between temp name and 'usual' kernel name.
  //
  // JSON file is created if IGC_ElfTempDumpEnable is enabled.
  std::list<string> elfVecNames;        // List of parameters for the linker, contains in/out ELF file names and params
  std::vector<const char *> elfVecPtrs; // Vector of pointers to the elfVecNames vector elements

  const unsigned int maxElfFileNameLength = 512;
  std::string elfLinkerLogName = "lldLinkLogName"; // First parameter for the linker, just a log name or a program name
                                                   // if this linker would be external.
  std::string linkedElfFileNameStr = "";           // Output file name created and filled by the linker
  int kernelID = 0;                                // Index of kernels; inserted into temporary ELF files names

  // If a single kernel in a program then neither temporary files are created nor the linker is in use,
  // hence ELF data is taken directly from the first found kernel's output (i.e. from m_debugData).
  IGC::SProgramOutput *pFirstKernelOutput = nullptr;
  bool isDebugInfo = true; // If a kernel does not contain debug info then this flag will be changed to false.
  IGC::CodeGenContext *ctx = nullptr;

  for (auto &pKernel : m_ShaderProgramList) {
    IGC::COpenCLKernel *simd8Shader = static_cast<IGC::COpenCLKernel *>(pKernel->GetShader(SIMDMode::SIMD8));
    IGC::COpenCLKernel *simd16Shader = static_cast<IGC::COpenCLKernel *>(pKernel->GetShader(SIMDMode::SIMD16));
    IGC::COpenCLKernel *simd32Shader = static_cast<IGC::COpenCLKernel *>(pKernel->GetShader(SIMDMode::SIMD32));

    // Determine how many simd modes we have per kernel
    // FIXME: We actually expect only one simd mode per kernel. There should not be multiple SIMD mode available
    // for one kernel (runtime cannot support that). So these check can be simplified
    std::vector<IGC::COpenCLKernel *> kernelVec;
    if ((m_Context.m_enableSimdVariantCompilation) && (m_Context.getModuleMetaData()->csInfo.forcedSIMDSize == 0)) {
      // For multiple SIMD modes, send SIMD modes in descending order
      if (IGC::COpenCLKernel::IsValidShader(simd32Shader))
        kernelVec.push_back(simd32Shader);
      if (IGC::COpenCLKernel::IsValidShader(simd16Shader))
        kernelVec.push_back(simd16Shader);
      if (IGC::COpenCLKernel::IsValidShader(simd8Shader))
        kernelVec.push_back(simd8Shader);

      if (IGC_IS_FLAG_ENABLED(ZeBinCompatibleDebugging)) {
        IGC_ASSERT_MESSAGE(false, "Missing ELF linking support for multiple SIMD modes");
      }
    } else if (m_Context.m_InternalOptions.EmitVisaOnly) {
      if (IGC::COpenCLKernel::IsVisaCompiledSuccessfullyForShader(simd32Shader))
        kernelVec.push_back(simd32Shader);
      else if (IGC::COpenCLKernel::IsVisaCompiledSuccessfullyForShader(simd16Shader))
        kernelVec.push_back(simd16Shader);
      else if (IGC::COpenCLKernel::IsVisaCompiledSuccessfullyForShader(simd8Shader))
        kernelVec.push_back(simd8Shader);
    } else {
      if (IGC::COpenCLKernel::IsValidShader(simd32Shader)) {
        kernelVec.push_back(simd32Shader);
      } else if (IGC::COpenCLKernel::IsValidShader(simd16Shader)) {
        kernelVec.push_back(simd16Shader);
      } else if (IGC::COpenCLKernel::IsValidShader(simd8Shader)) {
        kernelVec.push_back(simd8Shader);
      }
    }

    for (auto kernel : kernelVec) {
      IGC::SProgramOutput *pOutput = kernel->ProgramOutput();
      kernelID++; // 0 is reserved for an output linked file name

      zebuilder.createKernel((const char *)pOutput->m_programBin, pOutput->m_programSize, kernel->m_kernelInfo,
                             kernel->m_kernelCostexpInfo, kernel->getGRFSize(), pOutput->m_VISAAsm);

      // FIXME: Handle IGC_IS_FLAG_ENABLED(ShaderDumpEnable) and
      // IGC_IS_FLAG_ENABLED(ShaderOverride)

      // ... Create the debug data binary streams

      if (pOutput->m_debugDataSize == 0)
        isDebugInfo = false;
      if (IGC_IS_FLAG_ENABLED(ZeBinCompatibleDebugging) && isDebugInfo) {
        if (m_ShaderProgramList.size() == 1 && kernelVec.size() == 1) {
          // The first and only kernel found.
          pFirstKernelOutput = pOutput; // There is only one kernel in the program so no linking will be required
        } else {
          if (!elfVecNames.size()) {
            // The first of multiple kernels found:
            // - build for the linker a name of an output file, and
            // - add a log name, as a first element, to a vector of the linker parameters.

            ctx = kernel->GetContext(); // Remember context for future usage regarding warning emission (if needed).

            // Build a temporary output file name (with a path) for the linker
            std::string elfFileNameStr = getTempFileName(kernel);
            if (elfFileNameStr.empty()) {
              elfTmpFilesError = true; // Handle this error at the end of this function
              break;
            }
            linkedElfFileNameStr = elfFileNameStr;
            elfVecNames.push_back(elfLinkerLogName); // 1st element in this vector of names; required by the linker
            elfVecPtrs.push_back(elfVecNames.back().c_str());
          }

          // Build a temporary input file name (with a path) for the linker
          std::string elfFileNameStr = getTempFileName(kernel);
          if (elfFileNameStr.empty()) {
            elfTmpFilesError = true; // Handle this error at the end of this function
            break;
          }
          addElfKernelMapping(elfFileNameStr, getKernelDumpName(kernel));

          int writeFD = 0;
          sys::fs::CreationDisposition disp = sys::fs::CreationDisposition::CD_CreateAlways;
          sys::fs::OpenFlags flags = sys::fs::OpenFlags::OF_None;
          unsigned int mode = sys::fs::all_read | sys::fs::all_write;
          auto EC = sys::fs::openFileForReadWrite(Twine(elfFileNameStr), writeFD, disp, flags, mode);
          if (!EC) {
            raw_fd_ostream OS(writeFD, true, true); // shouldClose=true, unbuffered=true
            OS << StringRef((const char *)pOutput->m_debugData, pOutput->m_debugDataSize);
            // close(writeFD) is not required due to shouldClose parameter in ostream

            // A temporary input ELF file filled, so its name can be added to a vector of parameters for the linker
            elfVecNames.push_back(elfFileNameStr);
            elfVecPtrs.push_back(elfVecNames.back().c_str());
          } else {
            if (ctx) {
              ctx->EmitError((std::string("failed to open ELF file: ") + elfFileNameStr).c_str(), nullptr);
            }
            elfTmpFilesError = true; // Handle this error at the end of this function
            break;
          }
        }
      }
    }
  }

  if (IGC_IS_FLAG_ENABLED(ZeBinCompatibleDebugging) && isDebugInfo) {
    if (!elfTmpFilesError) {
      if (elfVecNames.size() == 0) {
        // if kernel was compiled only to visaasm, the output will be empty for gen binary.
        if (pFirstKernelOutput) {
          // Single kernel in a program, no ELF linking required
          // Copy sections one by one from ELF file to zeBinary with relocations adjusted.
          zebuilder.addElfSections(pFirstKernelOutput->m_debugData, pFirstKernelOutput->m_debugDataSize);
        }
      } else {
        // Multiple kernels in a program, ELF linking required before moving debuig info to zeBinary

        IGC_ASSERT_MESSAGE(elfVecNames.size() > 2,
                           "Unexpected number of parameters for the linker"); // 1st name is elfLinkerLog

        std::string linkedElfFileNameStrWithParam = "-o" + linkedElfFileNameStr;
        elfVecNames.push_back(linkedElfFileNameStrWithParam);
        elfVecPtrs.push_back(elfVecNames.back().c_str());

        std::string elfLinkerOpt2 = "--relocatable";
        elfVecNames.push_back(elfLinkerOpt2);
        elfVecPtrs.push_back(elfVecNames.back().c_str());

        if (IGC_IS_FLAG_DISABLED(UseMTInLLD)) {
          std::string elfLinkerOpt1 = "--threads=1";
          elfVecNames.push_back(elfLinkerOpt1);
          elfVecPtrs.push_back(elfVecNames.back().c_str());
        }

        // TODO: remove if not needed
        // std::string elfLinkerOpt1 = "--emit-relocs";  // "-q"
        // elfVecNames.push_back(elfLinkerOpt1);
        // elfVecPtrs.push_back((char*)(elfVecNames.at(elfVecNames.size() - 1).c_str()));

        ArrayRef<const char *> elfArrRef(elfVecPtrs);
        std::string linkErrStr = "";
        llvm::raw_string_ostream linkErr(linkErrStr);

        std::string linkOutStr = "";
        llvm::raw_string_ostream linkOut(linkOutStr);

        constexpr bool canExitEarly = false;
        bool linked = false;
        {
          // LLD is not assured to be thread-safe.
          // Mutex can be removed as soon as thread-safety is implemented in future versions of LLVM.
          static std::mutex linkerMtx;
          std::lock_guard<std::mutex> lck(linkerMtx);
          linked =
              IGCLLD::elf::link(elfArrRef, canExitEarly, linkOut, linkErr);
        }

        if (linked) {
          // Multiple ELF files linked.
          // Copy the data from the linked file to a memory, what will be a source location
          // for transmission debug info from the linked ELF to zeBinary

          // Check the size of the linked file to know how large memory space should be allocated
          uint64_t linkedElfSize = 0;
          auto ECfileSize = sys::fs::file_size(Twine(linkedElfFileNameStr), linkedElfSize);
          if (!ECfileSize && linkedElfSize > 0) {
            SmallString<maxElfFileNameLength> resultLinkedPath;
            Expected<sys::fs::file_t> FDOrErr =
                sys::fs::openNativeFileForRead(Twine(linkedElfFileNameStr), sys::fs::OF_UpdateAtime, &resultLinkedPath);
            std::error_code ECfileOpen;
            if (FDOrErr) {
              ErrorOr<std::unique_ptr<MemoryBuffer>> MBOrErr =
                  MemoryBuffer::getOpenFile(*FDOrErr, Twine(linkedElfFileNameStr), -1, false); // -1 of FileSize
              sys::fs::closeFile(*FDOrErr);
              if (MBOrErr) {
                // Copy sections one by one from ELF file to zeBinary with relocations adjusted.
                elfStorage.push_back(std::move(MBOrErr.get()));
                zebuilder.addElfSections((void *)elfStorage.back().get()->getBufferStart(), (size_t)linkedElfSize);
              } else {
                ECfileOpen = MBOrErr.getError();
                if (ctx) {
                  ctx->EmitError("ELF linked file cannot be buffered", nullptr);
                  ctx->EmitError(ECfileOpen.message().c_str(), nullptr);
                }
                elfTmpFilesError = true; // Handle this error also below
              }
            } else {
              ECfileOpen = errorToErrorCode(FDOrErr.takeError());
              if (ctx) {
                ctx->EmitError("ELF linked file cannot be opened", nullptr);
                ctx->EmitError(ECfileOpen.message().c_str(), nullptr);
              }
              elfTmpFilesError = true; // Handle this error also below
            }
          } else {
            if (ctx) {
              ctx->EmitError("ELF linked file size error", nullptr);
            }
            elfTmpFilesError = true; // Handle this error also below
          }
        } else {
          linkErr.str(); // Flush contents to the associated string
          linkOut.str(); // Flush contents to the associated string
          linkErrStr.append(linkOutStr);
          if (!linkErrStr.empty()) {
            if (ctx) {
              ctx->EmitError(linkErrStr.c_str(), nullptr);
            }
            elfTmpFilesError = true; // Handle this error also below
          }
        }
      }
    }

    if (elfTmpFilesError) {
      // Nothing to do with the linker when any error with temporary files occured.
      if (ctx) {
        ctx->EmitError("ZeBinary will not contain correct debug info due to an ELF temporary files error", nullptr);
      }
      retValue = false;
    }

    if (IGC_IS_FLAG_DISABLED(ElfTempDumpEnable)) {
      // Remove all temporary input ELF files
      for (const auto &elfFile : elfVecNames) {
        if (elfFile.compare(elfLinkerLogName.c_str())) {
          sys::fs::remove(Twine(elfFile), true); // true=ignore non-existing
          kernelID--;
          if (!kernelID) // elfVecNames may contain also options for the linker,
            break;       // hence finish input files removal when all input files removed.
        }
      }

      // Also remove a temporary linked output ELF file...
      sys::fs::remove(Twine(linkedElfFileNameStr), true); // true=ignore non-existing
    } else {
      retValue &= dumpElfKernelMapFile(ctx);
    }
  }

  zebuilder.getBinaryObject(programBinary);

  // dump .ze_info to a file
  if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable))
    dumpZEInfo(m_Context, zebuilder);
  if (IGC_IS_FLAG_ENABLED(DumpZEInfoToConsole))
    dumpZEInfo(m_Context, zebuilder, true);

  return retValue;
}

} // namespace iOpenCL
