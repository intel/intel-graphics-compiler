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
