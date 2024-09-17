/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "IGC/common/StringMacros.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CodeGenPublic.h"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "visa/include/visa_igc_common_header.h"

namespace llvm {
    class RTBuilder;
}

namespace IGC {

  class ResolveOCLRaytracingBuiltins : public llvm::ModulePass, public llvm::InstVisitor<ResolveOCLRaytracingBuiltins> {

  public:
    // Pass identification, replacement for typeid
    static char ID;

    ResolveOCLRaytracingBuiltins();

    virtual llvm::StringRef getPassName() const override {
      return "ResolveOCLRaytracingBuiltins";
    }

    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
      AU.addRequired<CodeGenContextWrapper>();
    }

    virtual bool runOnModule(llvm::Module& M) override;

    void visitCallInst(llvm::CallInst& callInst);

    void handleGetRtStack(llvm::CallInst& callInst);
    void handleGetThreadBTDStack(llvm::CallInst& callInst);
    void handleGetGlobalBTDStack(llvm::CallInst& callInst);
    void handleDispatchTraceRayQuery(llvm::CallInst& callInst);
    void handleRTSync(llvm::CallInst& callInst);
    void handleGetRTGlobalBuffer(llvm::CallInst& callInst);
    void handleInitRayQuery(llvm::CallInst& callInst);
    void handleUpdateRayQuery(llvm::CallInst& callInst);
    void handleQuery(llvm::CallInst& callInst);

  private:
    CodeGenContext* m_pCtx = nullptr;
    std::vector<llvm::CallInst*> m_callsToReplace;
    llvm::RTBuilder* m_builder = nullptr;

    void handleGetBTDStack(llvm::CallInst& callInst, const bool isGlobal);

    void defineOpaqueTypes();

    llvm::Value* getIntrinsicValue(llvm::GenISAIntrinsic::ID intrinsicId, llvm::ArrayRef<llvm::Value*> args = llvm::ArrayRef<llvm::Value*>());
  };
}
