/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _COLLECTGEOMETRYSHADERPROPERTIES_H_
#define _COLLECTGEOMETRYSHADERPROPERTIES_H_

#include "Compiler/CISACodeGen/GeometryShaderProperties.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/PassManager.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Intrinsics.h>
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"

namespace IGC {
    class CodeGenContextWrapper;
}

class CollectGeometryShaderProperties :
    public llvm::ImmutablePass,
    public llvm::InstVisitor<CollectGeometryShaderProperties>
{
public:
    CollectGeometryShaderProperties();

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
        AU.addRequired<IGC::CodeGenContextWrapper>();
    }

    /// read immutable information from the IR
    void gatherInformation(llvm::Function& kernel);

    /// Name of the pass
    virtual llvm::StringRef getPassName() const override { return "CollectGeometryShaderProperties"; }

    void visitCallInst(llvm::CallInst& I);

    /// Returns geometry shader properties object that stores information about
    /// the configuration of the Geometry Shader object.
    const IGC::GeometryShaderProperties& GetProperties() const {
        return m_gsProps;
    }

    static char ID;

private:
    void ExtractGlobalVariables(llvm::Function& F);
    void HandleSystemInput(llvm::GenIntrinsicInst& I);
    void HandleOutputWrite(llvm::GenIntrinsicInst& I);
    void HandleCutOrStreamHeader(llvm::GenIntrinsicInst& I);
    IGC::GeometryShaderProperties m_gsProps;
};

#endif // _COLLECTGEOMETRYSHADERPROPERTIES_H_
