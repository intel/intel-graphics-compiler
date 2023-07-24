/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef LEGALIZER_INSTSCALARIZER_H
#define LEGALIZER_INSTSCALARIZER_H

#include "TypeLegalizer.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/InstVisitor.h"
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

    namespace Legalizer {

        class InstScalarizer : public InstVisitor<InstScalarizer, bool> {
            friend class InstVisitor<InstScalarizer, bool>;

            TypeLegalizer* TL;
            BuilderType* IRB;

            ValueSeq Scalarized;

        public:
            InstScalarizer(TypeLegalizer* L, BuilderType* B) : TL(L), IRB(B) {}

            bool scalarize(Instruction* I);

        private:
            /// Helpers
            const char* getSuffix() const { return TL->getSuffix(Scalarize); }

        private:
            // By default, capture all missing instructions!
            bool visitInstruction(Instruction& I);

            /// Terminator instructions
            ///

            bool visitTerminatorInst(IGCLLVM::TerminatorInst& I);

            /// Standard binary operators
            ///

            bool visitBinaryOperator(BinaryOperator& I);

            /// Memory operators
            ///

            bool visitLoadInst(LoadInst& I);
            bool visitStoreInst(StoreInst& I);
            bool visitGetElementPtrInst(GetElementPtrInst& I);

            /// Cast operators
            ///

            bool visitBitCastInst(BitCastInst& I);

            /// Special instructions
            ///

            bool visitExtractElementInst(ExtractElementInst& I);
            bool visitInsertElementInst(InsertElementInst& I);
        };

    } // End Legalizer namespace

} // End IGC namespace

#endif // LEGALIZER_INSTSCALARIZER_H
