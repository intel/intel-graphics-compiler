/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef LIB_GENXCODEGEN_DEBUG_INFO_H
#define LIB_GENXCODEGEN_DEBUG_INFO_H

#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/Error.h>

#include "Probe/Assertion.h"

#include <unordered_map>
#include <map>

class VISAKernel;
class VISABuilder;
class ModuleToVisaTransformInfo;
struct ProgramInfo;

namespace IGC {
  struct DebugEmitterOpts;
} // namespace IGC

namespace llvm {

class Function;
class Instruction;
class FunctionGroup;
class GenXModule;

namespace genx {
namespace di {

struct VisaMapping {
  struct Mapping {
    unsigned VisaIdx = 0;
    const Instruction *Inst = nullptr;
    // Count of VISA instruction for this llvm Inst
    unsigned VisaCount = 0;
    bool IsDbgInst = false;
  };
  std::vector<Mapping> V2I;
};
} // namespace di
} // namespace genx

//--------------------------------------------------------------------
// Builds and holds the debug information for the current module
struct GenXDebugInfo : public ModulePass {

  using ElfBin = std::vector<char>;
  using DbgInfoStorage = std::unordered_map<const Function *, ElfBin>;

private:
  DbgInfoStorage ElfOutputs;

  void cleanup();
  void processKernel(const IGC::DebugEmitterOpts &Opts, const ProgramInfo &PD);
  void processPrimaryFunction(const IGC::DebugEmitterOpts &Opts,
                              const ModuleToVisaTransformInfo &MVTI,
                              const GenXModule &GM, VISABuilder &VB,
                              const Function &PF);

public:
  static char ID;

  explicit GenXDebugInfo() : ModulePass(ID) {}
  ~GenXDebugInfo() { cleanup(); }

  StringRef getPassName() const override { return "GenX Debug Info"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnModule(Module &M) override;
  void releaseMemory() override { cleanup(); }

  const DbgInfoStorage &getModuleDebug() const { return ElfOutputs; }
};

void initializeGenXDebugInfoPass(PassRegistry &);

} // namespace llvm

#endif // LIB_GENXCODEGEN_DEBUG_INFO_H
