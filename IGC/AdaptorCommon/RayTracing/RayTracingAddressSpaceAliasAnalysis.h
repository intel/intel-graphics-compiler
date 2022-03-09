/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenPublic.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include <llvm/Pass.h>
#include <llvmWrapper/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/BasicAliasAnalysis.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

class RayTracingAddressSpaceAAResult : public llvm::AAResultBase<RayTracingAddressSpaceAAResult>
{
    friend llvm::AAResultBase<RayTracingAddressSpaceAAResult>;
    const llvm::TargetLibraryInfo& TLI;
    const CodeGenContext& CGC;
public:
    explicit RayTracingAddressSpaceAAResult(
        const llvm::TargetLibraryInfo& TLI,
        const CodeGenContext& ctx)
        : llvm::AAResultBase<RayTracingAddressSpaceAAResult>(),
          TLI(TLI), CGC(ctx), allStateful(checkStateful(ctx)) {}
    RayTracingAddressSpaceAAResult(RayTracingAddressSpaceAAResult&& Arg)
        : llvm::AAResultBase<RayTracingAddressSpaceAAResult>(std::move(Arg)),
          TLI(Arg.TLI), CGC(Arg.CGC), allStateful(checkStateful(Arg.CGC)) {}

    IGCLLVM::AliasResultEnum alias(
        const llvm::MemoryLocation& LocA, const llvm::MemoryLocation& LocB
#if LLVM_VERSION_MAJOR >= 9
        , llvm::AAQueryInfo& AAQI
#endif
    );

    llvm::ModRefInfo getModRefInfo(
        const llvm::CallBase* Call, const llvm::MemoryLocation& Loc,
        llvm::AAQueryInfo& AAQI);

    llvm::ModRefInfo getModRefInfo(
        const llvm::CallBase* Call1, const llvm::CallBase* Call2,
        llvm::AAQueryInfo& AAQI);

    static bool isRTAS(unsigned AS, const CodeGenContext& Ctx);
private:
    bool allStateful = false;
private:
    bool noRTASAlias(unsigned AS1, unsigned AS2) const;
    bool isRTAS(unsigned AS) const;
    static bool checkStateful(const CodeGenContext& Ctx);
};

class RayTracingAddressSpaceAAWrapperPass : public llvm::ImmutablePass {
    std::unique_ptr<RayTracingAddressSpaceAAResult> Result;

public:
    static char ID;

    RayTracingAddressSpaceAAWrapperPass();

    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;

    bool doInitialization(llvm::Module& M) override;

    bool doFinalization(llvm::Module& M) override;

    RayTracingAddressSpaceAAResult& getResult();
    const RayTracingAddressSpaceAAResult& getResult() const;
};

llvm::ImmutablePass* createRayTracingAddressSpaceAAWrapperPass();
void addRayTracingAddressSpaceAAResult(llvm::Pass&, llvm::Function&, llvm::AAResults&);

} // End IGC namespace
