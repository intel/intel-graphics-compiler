/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_DIBUILDER_H
#define IGCLLVM_IR_DIBUILDER_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/DebugInfoMetadata.h"

namespace IGCLLVM
{
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

        // SysRoot Argument only for LLVM <= 10 backwards compatibility
        llvm::DIModule *createModule(llvm::DIScope *Scope, llvm::StringRef Name,
                        llvm::StringRef ConfigurationMacros, llvm::StringRef IncludePath,
                        llvm::StringRef APINotesFile = {}, llvm::DIFile *File = nullptr,
                        unsigned LineNo = 0, bool IsDecl = false, llvm::StringRef SysRoot = {})
        {

#if LLVM_VERSION_MAJOR >= 12
// Simply proxy the call

            return llvm::DIBuilder::createModule(Scope, Name, ConfigurationMacros, IncludePath,
                        APINotesFile, File, LineNo, IsDecl);

#elif LLVM_VERSION_MAJOR == 11
// LLVM 12 introduced IsDecl argument.
//
//     Differential Revision: https://reviews.llvm.org/D93462

            return llvm::DIBuilder::createModule(Scope, Name, ConfigurationMacros, IncludePath,
                        APINotesFile, File, LineNo);

#elif LLVM_VERSION_MAJOR <= 10
// LLVM 11 introduced major changes in arguments for Fortran support.
// Deleted SysRoot argument had moved to DICompileUnit
//
//     Differential Revision #1: https://reviews.llvm.org/D79484
//     Differential Revision #2: https://reviews.llvm.org/D71732

            return llvm::DIBuilder::createModule(Scope, Name, ConfigurationMacros, IncludePath,
                        SysRoot);
#endif
        }

        inline llvm::DITemplateTypeParameter* createTemplateTypeParameter(
            llvm::DIScope* Scope,
            llvm::StringRef Name,
            llvm::DIType* Ty)
        {
#if LLVM_VERSION_MAJOR <= 10
            return llvm::DIBuilder::createTemplateTypeParameter(Scope, Name, Ty);
#else
            return llvm::DIBuilder::createTemplateTypeParameter(Scope, Name, Ty, true);
#endif
        }

        inline llvm::DITemplateValueParameter* createTemplateValueParameter(
            llvm::DIScope* Scope,
            llvm::StringRef Name,
            llvm::DIType* Ty,
            llvm::Constant* Val)
        {
#if LLVM_VERSION_MAJOR <= 10
            return llvm::DIBuilder::createTemplateValueParameter(Scope, Name, Ty, Val);
#else
            return llvm::DIBuilder::createTemplateValueParameter(Scope, Name, Ty, true, Val);
#endif
        }
    };
}

#endif
