/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenPublic.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include <llvm/Pass.h>
#include <llvm/Analysis/BasicAliasAnalysis.h>
#include "llvm/IR/Instruction.h"
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/Analysis/AliasAnalysis.h>
#include <llvmWrapper/Analysis/TargetLibraryInfo.h>
#include "llvmWrapper/Analysis/AliasAnalysis.h"

namespace IGC {
class RayTracingAddressSpaceAAResult : public IGCLLVM::AAResultBaseWrapper<RayTracingAddressSpaceAAResult> {
  using BaseT = IGCLLVM::AAResultBaseWrapper<RayTracingAddressSpaceAAResult>;
  friend BaseT;
  const llvm::TargetLibraryInfo &TLI;
  const CodeGenContext &CGC;

public:
  explicit RayTracingAddressSpaceAAResult(const llvm::TargetLibraryInfo &TLI, const CodeGenContext &ctx)
      : BaseT(), TLI(TLI), CGC(ctx), allStateful(checkStateful(ctx)) {}
  RayTracingAddressSpaceAAResult(RayTracingAddressSpaceAAResult &&Arg)
      : BaseT(std::move(Arg)), TLI(Arg.TLI), CGC(Arg.CGC), allStateful(checkStateful(Arg.CGC)) {}
  RayTracingAddressSpaceAAResult(const RayTracingAddressSpaceAAResult &) = delete;
  RayTracingAddressSpaceAAResult &operator=(const RayTracingAddressSpaceAAResult &) = delete;
  RayTracingAddressSpaceAAResult &operator=(RayTracingAddressSpaceAAResult &&) = delete;

  IGCLLVM::AliasResultEnum alias(const llvm::MemoryLocation &LocA, const llvm::MemoryLocation &LocB,
                                 llvm::AAQueryInfo &AAQI,
#if LLVM_VERSION_MAJOR < 16
                                 const llvm::Instruction *CtxI = nullptr
#else
                                 const llvm::Instruction *CtxI
#endif
  );

  llvm::ModRefInfo getModRefInfo(const llvm::CallBase *Call, const llvm::MemoryLocation &Loc, llvm::AAQueryInfo &AAQI);

  llvm::ModRefInfo getModRefInfo(const llvm::CallBase *Call1, const llvm::CallBase *Call2, llvm::AAQueryInfo &AAQI);

  static bool isRTAS(unsigned AS, const CodeGenContext &Ctx);

private:
  bool allStateful = false;

private:
  bool noRTASAlias(unsigned AS1, unsigned AS2) const;
  bool isRTAS(unsigned AS) const;
  static bool checkStateful(const CodeGenContext &Ctx);
};

class RayTracingAddressSpaceAAWrapperPass : public llvm::ImmutablePass {
  std::unique_ptr<RayTracingAddressSpaceAAResult> Result;

public:
  static char ID;

  RayTracingAddressSpaceAAWrapperPass();

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  bool doInitialization(llvm::Module &M) override;

  bool doFinalization(llvm::Module &M) override;

  RayTracingAddressSpaceAAResult &getResult();
  const RayTracingAddressSpaceAAResult &getResult() const;
};

llvm::ImmutablePass *createRayTracingAddressSpaceAAWrapperPass();
void addRayTracingAddressSpaceAAResult(llvm::Pass &, llvm::Function &, llvm::AAResults &);

} // namespace IGC
