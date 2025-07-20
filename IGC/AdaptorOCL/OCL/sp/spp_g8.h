/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef SPP_G8_H
#define SPP_G8_H

#pragma once

#include <memory>
#include "OCL/util/BinaryStream.h"
#include "usc.h"
#include "zebin_builder.hpp"
#include "CommonMacros.h"
#include "Compiler/CISACodeGen/CShaderProgram.hpp"

namespace llvm {
class Twine;
namespace json {
class Array;
}
} // namespace llvm

namespace IGC {
class OpenCLProgramContext;
class CodeGenContext;
class CShaderProgram;
class COCLBTILayout;
struct SOpenCLProgramInfo;
struct SProgramOutput;
}; // namespace IGC

namespace iOpenCL {

// This is the base class to create an OpenCL ELF binary.
class CGen8OpenCLProgramBase : DisallowCopy {
public:
  explicit CGen8OpenCLProgramBase(PLATFORM platform);
  virtual ~CGen8OpenCLProgramBase();
  CGen8OpenCLProgramBase(const CGen8OpenCLProgramBase &) = delete;
  CGen8OpenCLProgramBase &operator=(const CGen8OpenCLProgramBase &) = delete;

  USC::SSystemThreadKernelOutput *m_pSystemThreadKernelOutput = nullptr;

  PLATFORM getPlatform() const { return m_Platform; }

public:
  // GetZEBinary - get ZE binary object
  virtual void GetZEBinary(llvm::raw_pwrite_stream &programBinary, unsigned pointerSizeInBytes) {
    IGC_UNUSED(programBinary);
    IGC_UNUSED(pointerSizeInBytes);
  }

protected:
  PLATFORM m_Platform;
  std::unique_ptr<llvm::json::Array> elfMapEntries;

  void addElfKernelMapping(const std::string &elfFileName, const std::string &kernelName);
  bool createElfKernelMapFile(const llvm::Twine &FilePath);
  bool dumpElfKernelMapFile(IGC::CodeGenContext *Ctx = nullptr);
};

class CGen8OpenCLProgram : public CGen8OpenCLProgramBase {
public:
  CGen8OpenCLProgram(PLATFORM platform, const IGC::OpenCLProgramContext &context);

  ~CGen8OpenCLProgram();
  CGen8OpenCLProgram(const CGen8OpenCLProgram &) = delete;
  CGen8OpenCLProgram &operator=(const CGen8OpenCLProgram &) = delete;

  /// getZEBinary - create and get ZE Binary
  /// if spv and spvSize are given, a .spv section will be created in the output ZEBinary
  bool GetZEBinary(llvm::raw_pwrite_stream &programBinary, unsigned pointerSizeInBytes, const char *spv,
                   uint32_t spvSize, const char *metrics, uint32_t metricsSize, const char *buildOptions,
                   uint32_t buildOptionsSize);

  // Used to track the kernel info from CodeGen
  std::vector<IGC::CShaderProgram::UPtr> m_ShaderProgramList;

  // invoked when the current module is deleted to avoid dangling Function ptr.
  void clearBeforeRetry();

private:
  const IGC::OpenCLProgramContext &m_Context;
};

} // namespace iOpenCL

#endif // SPP_G8_H
