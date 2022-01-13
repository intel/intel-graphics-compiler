/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_OPERATOR_H
#define IGCLLVM_IR_OPERATOR_H

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Operator.h>

namespace IGCLLVM {
#if LLVM_VERSION_MAJOR < 11
    class AddrSpaceCastOperator
            : public llvm::ConcreteOperator<llvm::Operator, llvm::Instruction::AddrSpaceCast> {
        friend class llvm::AddrSpaceCastInst;
        friend class llvm::ConstantExpr;

    public:
        llvm::Value *getPointerOperand() { return getOperand(0); }

        const llvm::Value *getPointerOperand() const { return getOperand(0); }

        unsigned getSrcAddressSpace() const {
            return getPointerOperand()->getType()->getPointerAddressSpace();
        }

        unsigned getDestAddressSpace() const {
            return getType()->getPointerAddressSpace();
        }
    };
#else
    using llvm::AddrSpaceCastOperator;
#endif
} // namespace IGCLLVM

#endif // IGCLLVM_IR_OPERATOR_H
