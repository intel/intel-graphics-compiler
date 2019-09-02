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

#ifndef IGCLLVM_IR_DIBUILDER_H
#define IGCLLVM_IR_DIBUILDER_H

#include <llvm/IR/DIBuilder.h>
#include <llvmWrapper/IR/DebugInfoMetadata.h>

namespace IGCLLVM
{
#if LLVM_VERSION_MAJOR == 4
    using llvm::DIBuilder;
#elif LLVM_VERSION_MAJOR >= 7
    class DIBuilder : public llvm::DIBuilder
    {
    public:

        DIBuilder(llvm::Module &M, bool AllowUnresolved = true,
            llvm::DICompileUnit *CU = nullptr) : llvm::DIBuilder(M, AllowUnresolved, CU)
        {}

        inline llvm::DIDerivedType* createInheritance(llvm::DIType *Ty, llvm::DIType *BaseTy,
            uint64_t BaseOffset,
            llvm::DINode::DIFlags Flags) {
            return llvm::DIBuilder::createInheritance(Ty, BaseTy, BaseOffset, 0, Flags);
        }
        inline llvm::Instruction *insertDbgValueIntrinsic(llvm::Value *V, uint64_t Offset,
            llvm::DILocalVariable *VarInfo,
            llvm::DIExpression *Expr,
            const llvm::DILocation *DL,
            llvm::BasicBlock *InsertAtEnd) {
            return llvm::DIBuilder::insertDbgValueIntrinsic(V, VarInfo, Expr, DL, InsertAtEnd);
        }
        inline llvm::Instruction *insertDbgValueIntrinsic(llvm::Value *V, uint64_t Offset,
            llvm::DILocalVariable *VarInfo,
            llvm::DIExpression *Expr,
            const llvm::DILocation *DL,
            llvm::Instruction *InsertBefore) {
            return llvm::DIBuilder::insertDbgValueIntrinsic(V, VarInfo, Expr, DL, InsertBefore);
        }
        inline llvm::Instruction* insertDbgValueIntrinsic(llvm::Value* V,
            llvm::DILocalVariable* VarInfo,
            llvm::DIExpression* Expr,
            const llvm::DILocation* DL,
            llvm::Instruction* InsertBefore) {
            return llvm::DIBuilder::insertDbgValueIntrinsic(V, VarInfo, Expr, DL, InsertBefore);
        }
        inline llvm::DINamespace* createNameSpace(llvm::DIScope *Scope, llvm::StringRef Name, llvm::DIFile *File,
            unsigned LineNo, bool ExportSymbols)
        {
            return llvm::DIBuilder::createNameSpace(Scope, Name, ExportSymbols);
        }

        inline IGCLLVM::DISubprogram* createFunction(
            llvm::DIScope *Scope, llvm::StringRef Name, llvm::StringRef LinkageName, llvm::DIFile *File,
            unsigned LineNo, llvm::DISubroutineType *Ty, bool isLocalToUnit,
            bool isDefinition, unsigned ScopeLine,
            llvm::DINode::DIFlags Flags = llvm::DINode::FlagZero, bool isOptimized = false,
            llvm::DITemplateParameterArray TParams = nullptr,
            llvm::DISubprogram *Decl = nullptr, llvm::DITypeArray ThrownTypes = nullptr)
        {
            return (IGCLLVM::DISubprogram*)llvm::DIBuilder::createFunction(
#if LLVM_VERSION_MAJOR == 7
                Scope, Name, LinkageName, File, LineNo, Ty, isLocalToUnit, isDefinition,
                ScopeLine, Flags, isOptimized, TParams, Decl, ThrownTypes);
#elif LLVM_VERSION_MAJOR >= 8
                Scope, Name, LinkageName, File, LineNo, Ty, isLocalToUnit, Flags, 
                llvm::DISubprogram::SPFlagDefinition, TParams, Decl, ThrownTypes);
#endif
        }

        inline llvm::DISubprogram *createMethod(
            llvm::DIScope *Scope, llvm::StringRef Name, llvm::StringRef LinkageName, llvm::DIFile *File,
            unsigned LineNo, llvm::DISubroutineType *Ty, bool isLocalToUnit,
            bool isDefinition, unsigned Virtuality = 0, unsigned VTableIndex = 0,
            int ThisAdjustment = 0, llvm::DIType *VTableHolder = nullptr,
            llvm::DINode::DIFlags Flags = llvm::DINode::FlagZero, bool isOptimized = false,
            llvm::DITemplateParameterArray TParams = nullptr,
            llvm::DITypeArray ThrownTypes = nullptr)
        {
            return llvm::DIBuilder::createMethod(
#if LLVM_VERSION_MAJOR == 7
                Scope, Name, LinkageName, File, LineNo, Ty, isLocalToUnit, isDefinition,
                Virtuality, VTableIndex, ThisAdjustment, VTableHolder, Flags, isOptimized, TParams, ThrownTypes);
#elif LLVM_VERSION_MAJOR >= 8
                Scope, Name, LinkageName, File, LineNo, Ty, Virtuality, ThisAdjustment,
                VTableHolder, Flags, llvm::DISubprogram::SPFlagDefinition, TParams, ThrownTypes);
#endif
        }


        inline llvm::DISubprogram *createTempFunctionFwdDecl(
            llvm::DIScope *Scope, llvm::StringRef Name, llvm::StringRef LinkageName, llvm::DIFile *File,
            unsigned LineNo, llvm::DISubroutineType *Ty, bool isLocalToUnit,
            bool isDefinition, unsigned ScopeLine,
            llvm::DINode::DIFlags Flags = llvm::DINode::FlagZero, bool isOptimized = false,
            llvm::DITemplateParameterArray TParams = nullptr,
            llvm::DISubprogram *Decl = nullptr, llvm::DITypeArray ThrownTypes = nullptr)
        {
            return llvm::DIBuilder::createTempFunctionFwdDecl(
#if LLVM_VERSION_MAJOR == 7
                Scope, Name, LinkageName, File, LineNo, Ty, isLocalToUnit, isDefinition,
                ScopeLine, Flags, isOptimized, TParams, Decl, ThrownTypes);
#elif LLVM_VERSION_MAJOR >= 8
                Scope, Name, LinkageName, File, LineNo, Ty, ScopeLine,
                Flags, llvm::DISubprogram::SPFlagDefinition, TParams, Decl, ThrownTypes);
#endif
        }
    };
#endif
}

#endif
