/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_ATTRIBUTES_H
#define IGCLLVM_IR_ATTRIBUTES_H

#include "llvm/Config/llvm-config.h"

#include <llvm/IR/Attributes.h>

namespace IGCLLVM {
    /// Return the attribute object that exists at the given index.
    inline llvm::Attribute getAttribute(llvm::AttributeList Attrs, unsigned Index, llvm::Attribute::AttrKind Kind) {
#if LLVM_VERSION_MAJOR >= 14
        return Attrs.getAttributeAtIndex(llvm::AttributeList::FunctionIndex, Kind);
#else
        return Attrs.getAttribute(Index, Kind);
#endif
    }

    /// Return the attribute object that exists at the given index.
    inline llvm::Attribute getAttribute(llvm::AttributeList Attrs, unsigned Index, llvm::StringRef Kind) {
#if LLVM_VERSION_MAJOR >= 14
        return Attrs.getAttributeAtIndex(llvm::AttributeList::FunctionIndex, Kind);
#else
        return Attrs.getAttribute(Index, Kind);
#endif
    }

    inline llvm::AttributeSet getParamAttrs(const llvm::AttributeList &AL, unsigned opNum) {
#if LLVM_VERSION_MAJOR >= 14
        return AL.getParamAttrs(opNum);
#else
        return AL.getParamAttributes(opNum);
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

    class AttrBuilder : public llvm::AttrBuilder {
    public:
        AttrBuilder() = delete;
        AttrBuilder(llvm::LLVMContext &Ctx)
        #if LLVM_VERSION_MAJOR >= 14
        : llvm::AttrBuilder(Ctx) {}
        #else
        : llvm::AttrBuilder() {}
        #endif

        AttrBuilder(llvm::LLVMContext &Ctx, llvm::AttributeSet AS)
        #if LLVM_VERSION_MAJOR >= 14
        : llvm::AttrBuilder(Ctx, AS) {}
        #else
        : llvm::AttrBuilder(AS) {}
        #endif
    };
}

#endif
