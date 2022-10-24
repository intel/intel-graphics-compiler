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

namespace IGC {

  class ResolveOCLRaytracingBuiltins : public llvm::ModulePass, public llvm::InstVisitor<ResolveOCLRaytracingBuiltins> {

  public:
    // Pass identification, replacement for typeid
    static char ID;

    ResolveOCLRaytracingBuiltins();

    ~ResolveOCLRaytracingBuiltins() {};

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
    void handleGetImplicitDG(llvm::CallInst& callInst);

  private:
    CodeGenContext* m_pCtx;
    std::vector<llvm::CallInst*> m_callsToReplace;
    IGCLLVM::IRBuilder<>* m_builder;

    llvm::Instruction* loadFromOffset(llvm::Value* basePtr, const size_t offset, const size_t typeSizeInBytes, llvm::StringRef valName);
    void handleGetBTDStack(llvm::CallInst& callInst, const bool isGlobal);

    llvm::CallInst* CreateLSCFence(
        llvm::IRBuilder<>* IRB,
        LSC_SFID SFID,
        LSC_SCOPE Scope,
        LSC_FENCE_OP FenceOp);

#define RT_DISPATCH_GETTER_DECL(FieldName) \
    llvm::Instruction* FieldName##Getter(llvm::Value* rtDispatchGlobalsValue);

    RT_DISPATCH_GETTER_DECL(rtMemBasePtr)
    RT_DISPATCH_GETTER_DECL(maxBVHLevels)
    RT_DISPATCH_GETTER_DECL(stackSizePerRay)
    RT_DISPATCH_GETTER_DECL(numDSSRTStacks)

#undef RT_DISPATCH_GETTER_DECL

    llvm::Value* getIntrinsicValue(llvm::GenISAIntrinsic::ID intrinsicId, llvm::ArrayRef<llvm::Value*> args = llvm::None);
  };
}
