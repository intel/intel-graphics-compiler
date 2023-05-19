/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <string>
#include <type_traits>

#include <llvm/ADT/ArrayRef.h>
#include <llvm/Support/DynamicLibrary.h>

#include "Compiler/CodeGenPublic.h"

#include "vc/GenXCodeGen/GenXOCLRuntimeInfo.h"

#include "Probe/Assertion.h"
#include "common/VCPlatformSelector.hpp"

#include "AdaptorOCL/OCL/sp/spp_g8.h"

namespace llvm {
class Error;
class ToolOutputFile;
class raw_ostream;
} // namespace llvm

namespace vc {

struct CompileOptions;

// Interface to compile and package cm kernels into OpenCL binars.
class CMKernel {
public:
  using ArgKind = llvm::GenXOCLRuntimeInfo::KernelArgInfo::KindType;
  using ArgAccessKind = llvm::GenXOCLRuntimeInfo::KernelArgInfo::AccessKindType;

  explicit CMKernel(const PLATFORM &platform);
  ~CMKernel();

  PLATFORM m_platform;
  IGC::SOpenCLKernelInfo m_kernelInfo;
  IGC::COCLBTILayout m_btiLayout;
  uint32_t m_GRFSizeInBytes;
  bool m_SupportsDebugging = false;

  // getter for convenience
  const IGC::SProgramOutput &getProgramOutput() const {
    IGC_ASSERT_MESSAGE(m_kernelInfo.m_executionEnvironment.CompiledSIMDSize ==
                           1,
                       "SIMD size is expected to be 1 for CMKernel");
    return m_kernelInfo.m_kernelProgram.simd1;
  }

  // getter for convenience
  IGC::SProgramOutput &getProgramOutput() {
    return const_cast<IGC::SProgramOutput &>(
        static_cast<const CMKernel *>(this)->getProgramOutput());
  }

  // General argument
  void createConstArgumentAnnotation(unsigned argNo, unsigned sizeInBytes,
                                     unsigned payloadPosition,
                                     unsigned offsetInArg);

  // 1D/2D/3D Surface
  void
  createImageAnnotation(unsigned argNo, unsigned BTI,
                        llvm::GenXOCLRuntimeInfo::KernelArgInfo::KindType Kind,
                        ArgAccessKind Access);

  // add a pointer patch token.
  void createPointerGlobalAnnotation(unsigned index, unsigned offset,
                                     unsigned sizeInBytes, unsigned BTI,
                                     ArgAccessKind access, bool isBindless,
                                     bool isStateful);

  void createPointerLocalAnnotation(unsigned index, unsigned offset,
                                    unsigned sizeInBytes, unsigned alignment);

  void createPrivateBaseAnnotation(unsigned argNo, unsigned byteSize,
                                   unsigned payloadPosition, int BTI,
                                   unsigned statelessPrivateMemSize,
                                   bool isStateful);

  // add a stateful buffer patch token.
  void createBufferStatefulAnnotation(unsigned argNo, ArgAccessKind accessKind);

  // Local or global size
  void createSizeAnnotation(unsigned payloadPosition,
                            iOpenCL::DATA_PARAMETER_TOKEN type);

  // Global work offset/local work size
  void createImplicitArgumentsAnnotation(unsigned payloadPosition);

  // Sampler
  void createSamplerAnnotation(unsigned argNo, unsigned BTI);

  void createAssertBufferArgAnnotation(unsigned Index, unsigned BTI,
                                       unsigned Size, unsigned ArgOffset);
  void createPrintfBufferArgAnnotation(unsigned Index, unsigned BTI,
                                       unsigned Size, unsigned ArgOffset);

  void createImplArgsBufferAnnotation(unsigned Size, unsigned ArgOffset);

  void RecomputeBTLayout(int numUAVs, int numResources);
};

using ToolOutputHolder = std::unique_ptr<llvm::ToolOutputFile>;
using TmpFilesStorage = std::map<std::string, ToolOutputHolder>;

class CGen8CMProgram : public iOpenCL::CGen8OpenCLProgramBase {
public:
  class CMProgramCtxProvider
      : public iOpenCL::CGen8OpenCLStateProcessor::IProgramContext {
  public:
    CMProgramCtxProvider() {}

    ShaderHash getProgramHash() const override { return {}; }
    bool needsSystemKernel() const override { return false; }
    // TODO: VC Kernels should always allocate SIP surface (to allow debugging)
    bool isProgramDebuggable() const override {
      return KernelIsDebuggable;
    }
    bool hasProgrammableBorderColor() const override { return false; }
    bool useBindlessMode() const override { return false; }
    bool useBindlessLegacyMode() const override { return false; }

    bool KernelIsDebuggable = false;
  };

  explicit CGen8CMProgram(PLATFORM platform, const WA_TABLE &WATable,
                          llvm::ArrayRef<char> SPIRV = llvm::None,
                          llvm::Optional<llvm::StringRef> Opts = llvm::None);

  // Produce the final ELF binary with the given CM kernels
  // in OpenCL format.
  void CreateKernelBinaries(CompileOptions& Opts);
  void GetZEBinary(llvm::raw_pwrite_stream &programBinary,
                   unsigned pointerSizeInBytes) override;
  bool HasErrors() const { return !m_ErrorLog.empty(); };
  bool HasCrossThreadOffsetRelocations();
  llvm::Error GetError() const;

  // CM kernel list.
  using CMKernelsStorage = std::vector<std::unique_ptr<CMKernel>>;
  CMKernelsStorage m_kernels;

  // Data structure to create patch token based binaries.
  std::unique_ptr<IGC::SOpenCLProgramInfo> m_programInfo;

  CMProgramCtxProvider m_ContextProvider;
  std::string m_ErrorLog;

private:
  llvm::ArrayRef<char> m_spirv;
  llvm::Optional<llvm::StringRef> m_opts;

  TmpFilesStorage extractRawDebugInfo(llvm::raw_ostream &ErrStream);
  std::unique_ptr<llvm::MemoryBuffer> buildZeDebugInfo();
};

void createBinary(
    CGen8CMProgram &CMProgram,
    const llvm::GenXOCLRuntimeInfo::CompiledModuleT &CompiledModule);

} // namespace vc
