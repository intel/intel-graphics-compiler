/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

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
                                     ArgAccessKind access, bool isBindless);

  void createPrivateBaseAnnotation(unsigned argNo, unsigned byteSize,
                                   unsigned payloadPosition, int BTI,
                                   unsigned statelessPrivateMemSize);

  // add a stateful buffer patch token.
  void createBufferStatefulAnnotation(unsigned argNo, ArgAccessKind accessKind);

  // Local or global size
  void createSizeAnnotation(unsigned payloadPosition,
                            iOpenCL::DATA_PARAMETER_TOKEN type);

  // Global work offset/local work size
  void createImplicitArgumentsAnnotation(unsigned payloadPosition);

  // Sampler
  void createSamplerAnnotation(unsigned argNo, unsigned BTI);

  void createPrintfBufferArgAnnotation(unsigned Index, unsigned BTI,
                                       unsigned Size, unsigned ArgOffset);

  void createImplArgsBufferAnnotation(unsigned Size, unsigned ArgOffset);

  void RecomputeBTLayout(int numUAVs, int numResources);
};

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

  explicit CGen8CMProgram(PLATFORM platform, const WA_TABLE& WATable);

  // Produce the final ELF binary with the given CM kernels
  // in OpenCL format.
  void CreateKernelBinaries(CompileOptions& Opts);
  void GetZEBinary(llvm::raw_pwrite_stream &programBinary,
                   unsigned pointerSizeInBytes) override;
  bool HasErrors() const { return !m_ErrorLog.empty(); };
  llvm::Error GetError() const;

  // CM kernel list.
  using CMKernelsStorage = std::vector<std::unique_ptr<CMKernel>>;
  CMKernelsStorage m_kernels;

  // Data structure to create patch token based binaries.
  std::unique_ptr<IGC::SOpenCLProgramInfo> m_programInfo;

  CMProgramCtxProvider m_ContextProvider;
  std::string m_ErrorLog;
};

void createBinary(
    CGen8CMProgram &CMProgram,
    const llvm::GenXOCLRuntimeInfo::CompiledModuleT &CompiledModule);

} // namespace vc
