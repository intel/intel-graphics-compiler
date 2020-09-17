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

#include "GenXBackendConfig.h"

#include "llvm/Support/CommandLine.h"

#include <string>

#define DEBUG_TYPE "GenXBackendConfig"

using namespace llvm;

//===----------------------------------------------------------------------===//
//
// All options that can control backend behavior should be here.
//
//===----------------------------------------------------------------------===//
static cl::opt<bool> DumpRegAllocOpt(
    "genx-dump-regalloc", cl::init(false), cl::Hidden,
    cl::desc(
        "Enable dumping of GenX liveness and register allocation to a file."));

static cl::opt<unsigned> StackMemSizeOpt("stack-mem-size",
                                         cl::desc("Available space for stack"),
                                         cl::init(8 * 1024));

static cl::opt<bool>
    EnableAsmDumpsOpt("genx-enable-asm-dumps",
                      cl::desc("Enable finalizer assembly dumps"),
                      cl::init(false));
static cl::opt<bool>
    EnableDebugInfoDumpOpt("genx-enable-dbginfo-dumps",
                           cl::desc("Enable debug information-related dumps"),
                           cl::init(false));
static cl::opt<std::string> DebugInfoDumpNameOverride(
    "genx-dbginfo-dumps-name-override",
    cl::desc("Override for 'suffix' part of debug info dump name"));

//===----------------------------------------------------------------------===//
//
// Backend config related stuff.
//
//===----------------------------------------------------------------------===//
char GenXBackendConfig::ID = 0;

GenXBackendOptions::GenXBackendOptions()
    : DumpRegAlloc(DumpRegAllocOpt), StackSurfaceMaxSize(StackMemSizeOpt),
      EnableAsmDumps(EnableAsmDumpsOpt),
      EnableDebugInfoDumps(EnableDebugInfoDumpOpt),
      DebugInfoDumpsNameOverride(DebugInfoDumpNameOverride) {}

GenXBackendConfig::GenXBackendConfig() : ImmutablePass(ID) {
  initializeGenXBackendConfigPass(*PassRegistry::getPassRegistry());
}

GenXBackendConfig::GenXBackendConfig(GenXBackendOptions OptionsIn,
                                     GenXBackendData DataIn)
    : ImmutablePass(ID), Options(std::move(OptionsIn)),
      Data(std::move(DataIn)) {
  initializeGenXBackendConfigPass(*PassRegistry::getPassRegistry());
}

INITIALIZE_PASS(GenXBackendConfig, DEBUG_TYPE, DEBUG_TYPE, false, true)
