/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_ATTRIBUTES_H
#define IGCLLVM_IR_ATTRIBUTES_H


#include "llvm/Config/llvm-config.h"

#include <llvm/IR/Attributes.h>
#include <llvm/IR/InstrTypes.h>

namespace IGCLLVM {
    /// Return the attribute object that exists at the given index.
    inline llvm::Attribute getAttribute(llvm::AttributeList Attrs, unsigned Index, llvm::Attribute::AttrKind Kind) {
#if LLVM_VERSION_MAJOR >= 14
        return Attrs.getAttributeAtIndex(Index, Kind);
#else
        return Attrs.getAttribute(Index, Kind);
#endif
    }

    inline llvm::Attribute getWithStructRetType(llvm::LLVMContext &Context, llvm::Type *Ty) {
#if LLVM_VERSION_MAJOR <= 11
        return llvm::Attribute::get(Context, llvm::Attribute::StructRet);
#else
        return llvm::Attribute::getWithStructRetType(Context, Ty);
#endif
    }

    inline void addFnAttr(llvm::CallBase& CB, llvm::Attribute Attr) {
#if LLVM_VERSION_MAJOR >= 14
        CB.addFnAttr(Attr);
#else
        CB.addAttribute(llvm::AttributeList::FunctionIndex, Attr);
#endif
    }

    inline void removeFnAttr(llvm::CallBase& CB, llvm::Attribute::AttrKind Kind) {
#if LLVM_VERSION_MAJOR >= 14
        CB.removeFnAttr(Kind);
#else
        CB.removeAttribute(llvm::AttributeList::FunctionIndex, Kind);
#endif
    }

    /// Return the attribute object that exists at the given index.
    inline llvm::Attribute getAttribute(llvm::AttributeList Attrs, unsigned Index, llvm::StringRef Kind) {
#if LLVM_VERSION_MAJOR >= 14
        return Attrs.getAttributeAtIndex(Index, Kind);
#else
        return Attrs.getAttribute(Index, Kind);
#endif
    }

    inline bool hasParentContext(llvm::Attribute &Attribute, llvm::LLVMContext &Ctx) {
#if LLVM_VERSION_MAJOR >= 13
        return Attribute.hasParentContext(Ctx);
#else
        // before llvm 13 there was no functionality for checking attribute validity
        // nor did llvm::Verifier contain the nested loops checking said validity in
        // verifyFunctionAttrs() by invoking Attribute::hasParentContext() for each.
        // Since it wasn't an issue, this wrapper will also treat it as a non-issue.
        return true;
#endif
    }

    inline llvm::AttributeSet getParamAttrs(const llvm::AttributeList &AL, unsigned opNum) {
#if LLVM_VERSION_MAJOR >= 14
        return AL.getParamAttrs(opNum);
#else
        return AL.getParamAttributes(opNum);
#endif
    }

    inline bool hasParamAttr(const llvm::AttributeList &AL, unsigned ArgNo, llvm::Attribute::AttrKind Kind) {
#if LLVM_VERSION_MAJOR >= 14
        return AL.hasParamAttr(ArgNo, Kind);
#else
        return AL.hasParamAttribute(ArgNo, Kind);
#endif
    }

    inline llvm::AttributeList addAttributesAtIndex(llvm::AttributeList &Attrs, llvm::LLVMContext &C, unsigned Index, const llvm::AttrBuilder &B) {
#if LLVM_VERSION_MAJOR >= 14
        return Attrs.addAttributesAtIndex(C, Index, B);
#else
        return Attrs.addAttributes(C, Index, B);
#endif
    }

    inline llvm::AttributeSet getFnAttrs(const llvm::AttributeList &AL) {
#if LLVM_VERSION_MAJOR >= 14
        return AL.getFnAttrs();
#else
        return AL.getFnAttributes();
#endif
    }

    inline llvm::AttributeSet getRetAttrs(const llvm::AttributeList &AL) {
#if LLVM_VERSION_MAJOR >= 14
        return AL.getRetAttrs();
#else
        return AL.getRetAttributes();
#endif
    }

    inline llvm::AttributeList addFnAttributes(llvm::AttributeList &Attrs, llvm::LLVMContext& C, llvm::AttrBuilder& B) {
#if LLVM_VERSION_MAJOR >= 14
        return Attrs.addFnAttributes(C, B);
#else
        return Attrs.addAttributes(C, llvm::AttributeList::FunctionIndex, B);
#endif
    }

    inline llvm::AttributeList addRetAttributes(llvm::AttributeList &Attrs, llvm::LLVMContext& C, llvm::AttrBuilder& B) {
#if LLVM_VERSION_MAJOR >= 14
        return Attrs.addRetAttributes(C, B);
#else
        return Attrs.addAttributes(C, llvm::AttributeList::ReturnIndex, B);
#endif
    }

    inline llvm::AttrBuilder makeAttrBuilder(llvm::LLVMContext &Ctx) {
        return llvm::AttrBuilder(
#if LLVM_VERSION_MAJOR >= 14
            Ctx
#endif // LLVM_VERSION_MAJOR
            );
    }

    inline llvm::AttrBuilder makeAttrBuilder(llvm::LLVMContext &Ctx, llvm::AttributeSet AS) {
        return llvm::AttrBuilder(
#if LLVM_VERSION_MAJOR >= 14
            Ctx,
#endif // LLVM_VERSION_MAJOR
            AS);
    }

    inline llvm::AttrBuilder &addStructRetAttr(llvm::AttrBuilder &AB, llvm::Type *Ty) {
#if LLVM_VERSION_MAJOR >= 12
        return AB.addStructRetAttr(Ty);
#else // LLVM_VERSION_MAJOR
        (void)Ty;
        return AB.addAttribute(llvm::Attribute::StructRet);
#endif // LLVM_VERSION_MAJOR
    }
}

#endif
