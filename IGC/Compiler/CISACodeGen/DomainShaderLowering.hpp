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
#pragma once
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Argument.h>
#include "common/LLVMWarningsPop.hpp"


namespace IGC
{
    class DomainShaderProperties
    {
    public:
        bool m_hasClipDistance = false;
        QuadEltUnit  m_URBOutputLength = QuadEltUnit(0);
        llvm::Value* m_UArg;
        llvm::Value* m_VArg;
        llvm::Value* m_WArg;
        bool m_isVPAIndexDeclared = false;
        bool m_isRTAIndexDeclared = false;
    };

    class CollectDomainShaderProperties : public llvm::ImmutablePass
    {
    public:
        CollectDomainShaderProperties();
        static char ID;
        void DeclareClipDistance();
        void DeclareOutput(QuadEltUnit offset);
        void SetDomainPointUArgu(llvm::Value* Arg);
        void SetDomainPointVArgu(llvm::Value* Arg);
        void SetDomainPointWArgu(llvm::Value* Arg);
        llvm::Value* GetDomainPointUArgu();
        llvm::Value* GetDomainPointVArgu();
        llvm::Value* GetDomainPointWArgu();
        void SetRTAIndexDeclared(bool isRTAIndexDeclared);
        void SetVPAIndexDeclared(bool isVPAIndexDeclared);

        const DomainShaderProperties& GetProperties() { return m_dsProps; }
    protected:
        DomainShaderProperties m_dsProps;
    };
    /// Creates a function pass that lowers input/output intrinsics to URB read/write
    llvm::FunctionPass* createDomainShaderLoweringPass();

} // namespace IGC
