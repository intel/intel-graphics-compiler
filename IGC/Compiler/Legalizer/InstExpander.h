/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef LEGALIZER_INSTEXPANDER_H
#define LEGALIZER_INSTEXPANDER_H

#include "TypeLegalizer.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/InstVisitor.h"
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

    namespace Legalizer {

        class InstExpander : public InstVisitor<InstExpander, bool> {
            friend class InstVisitor<InstExpander, bool>;

            TypeLegalizer* TL;
            BuilderType* IRB;

            ValueSeq Expanded;

        public:
            InstExpander(TypeLegalizer* L, BuilderType* B) : TL(L), IRB(B) {}

            bool expand(Instruction* I);

        private:
            /// Helpers
            const char* getSuffix() const { return TL->getSuffix(Expand); }

        private:
            // By default, capture all missing instructions!
            bool visitInstruction(Instruction& I);

            /// Terminator instructions
            ///

            bool visitTerminatorInst(IGCLLVM::TerminatorInst& I);

            /// Standard binary operators
            ///

            bool visitAdd(BinaryOperator& I);
            bool visitSub(BinaryOperator& I);
            bool visitMul(BinaryOperator& I);
            bool visitUDiv(BinaryOperator& I);
            bool visitSDiv(BinaryOperator& I);
            bool visitURem(BinaryOperator& I);
            bool visitSRem(BinaryOperator& I);
            bool visitShl(BinaryOperator& I);
            bool visitLShr(BinaryOperator& I);
            bool visitAShr(BinaryOperator& I);
            bool visitBinaryOperator(BinaryOperator& I);

            /// Memory operators
            ///

            bool visitLoadInst(LoadInst& I);
            bool visitStoreInst(StoreInst& I);

            /// Cast operators
            ///

            bool visitTruncInst(TruncInst& I);
            bool visitZExtInst(ZExtInst& I);
            bool visitBitCastInst(BitCastInst& I);
        };

    } // End Legalizer namespace

} // End IGC namespace
#endif // LEGALIZER_INSTEXPANDER_H
