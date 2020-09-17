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
//
// Definition of backend configuration options and immutable wrapper pass.
//
// This pass should be used to query all options that can affect backend
// behavior. Pass will always be available at least with default options.
// Default values are set using LLVM command line options that can be
// overridden, for example, in plugin mode.
//
// Online mode wrapper will provide its custom values for all options that
// should not be defaulted.
//
// Proposed usage in passes: just use "getAnalysis<GenXBackendConfig>()" and
// query all needed information.
//
//===----------------------------------------------------------------------===//

#ifndef LIB_GENXCODEGEN_GENXBACKENDCONFIG_H
#define LIB_GENXCODEGEN_GENXBACKENDCONFIG_H

#include "vc/Support/ShaderDump.h"

#include "llvmWrapper/Support/MemoryBuffer.h"

#include "Probe/Assertion.h"

#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>

namespace llvm {

void initializeGenXBackendConfigPass(PassRegistry &PR);

// Plain structure to be filled by users who want to create backend
// configuration. Some values are default-initialized from cl options.
struct GenXBackendOptions {
  // Enable/disable regalloc dump.
  bool DumpRegAlloc;

  // Maximum available memory for stack (in bytes).
  unsigned StackSurfaceMaxSize;

  // Non-owning pointer to abstract shader dumper for debug dumps.
  vc::ShaderDumper *Dumper = nullptr;

  // Whether to enable finalizer dumps.
  bool EnableAsmDumps;
  // Whether to enable dumps of kernel debug information
  bool EnableDebugInfoDumps;
  std::string DebugInfoDumpsNameOverride;

  GenXBackendOptions();
};

struct GenXBackendData {
  MemoryBufferRef OCLGenericBiFModule;

  GenXBackendData() = default;
  GenXBackendData(const MemoryBuffer &OCLGenericBiFModuleBuffer)
      : OCLGenericBiFModule{
            IGCLLVM::makeMemoryBufferRef(OCLGenericBiFModuleBuffer)} {}
};

class GenXBackendConfig : public ImmutablePass {
public:
  static char ID;

private:
  GenXBackendOptions Options;
  GenXBackendData Data;

public:
  GenXBackendConfig();
  explicit GenXBackendConfig(GenXBackendOptions OptionsIn,
                             GenXBackendData DataIn);

  // Return whether regalloc results should be printed.
  bool enableRegAllocDump() const { return Options.DumpRegAlloc; }

  // Return maximum available space in bytes for stack purposes.
  unsigned getStackSurfaceMaxSize() const {
    return Options.StackSurfaceMaxSize;
  }

  MemoryBufferRef getOCLGenericBiFModule() const {
    return Data.OCLGenericBiFModule;
  }

  // Return whether shader dumper is installed.
  bool hasShaderDumper() const { return Options.Dumper; }

  // Get reference to currently installed dumper.
  // Precondition: hasShaderDumper() == true.
  vc::ShaderDumper &getShaderDumper() const {
    IGC_ASSERT_MESSAGE(hasShaderDumper(),
                       "Attempt to query not installed dumper");
    return *Options.Dumper;
  }

  bool asmDumpsEnabled() const { return Options.EnableAsmDumps; }
  bool dbgInfoDumpsEnabled() const { return Options.EnableDebugInfoDumps; }
  const std::string &dbgInfoDumpsNameOverride() const {
    return Options.DebugInfoDumpsNameOverride;
  }
};
} // namespace llvm

#endif
