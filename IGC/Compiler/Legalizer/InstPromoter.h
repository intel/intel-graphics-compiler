/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef LEGALIZER_INSTPROMOTER_H
#define LEGALIZER_INSTPROMOTER_H

#include "TypeLegalizer.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/InstVisitor.h"
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

    namespace Legalizer {

        class InstPromoter : public InstVisitor<InstPromoter, bool> {
            friend class InstVisitor<InstPromoter, bool>;

            TypeLegalizer* TL;
            BuilderType* IRB;

            Value* Promoted = nullptr;

        public:
            InstPromoter(TypeLegalizer* L, BuilderType* B) : TL(L), IRB(B) {}

            bool promote(Instruction* I);

        private:
            /// Helpers
            const char* getSuffix() const { return TL->getSuffix(Promote); }
            Value* getSinglePromotedValueIfExist(Value* OriginalValue);
            Type* getSinglePromotedTypeIfExist(Type* OriginalType);
            std::pair<Value*, Type*> preparePromotedIntrinsicInst(IntrinsicInst& I);

        private:
            // By default, capture all missing instructions!
            bool visitInstruction(Instruction& I);

            /// Terminator instructions
            ///

            bool visitTerminatorInst(IGCLLVM::TerminatorInst& I);

            /// Standard binary operators
            ///
            bool visitSelectInst(SelectInst& I);

            bool visitICmpInst(ICmpInst& I);

            bool visitBinaryOperator(BinaryOperator& I);

            /// Memory operators
            ///

            bool visitAllocaInst(AllocaInst& I);
            bool visitLoadInst(LoadInst& I);
            bool visitStoreInst(StoreInst& I);

            /// Cast operators

            bool visitTruncInst(TruncInst& I);
            bool visitSExtInst(SExtInst& I);
            bool visitZExtInst(ZExtInst& I);
            bool visitBitCastInst(BitCastInst& I);

            /// Other operators

            bool visitExtractElementInst(ExtractElementInst& I);
            bool visitInsertElementInst(InsertElementInst& I);
            bool visitGenIntrinsicInst(GenIntrinsicInst& I);
            bool visitLLVMIntrinsicInst(IntrinsicInst& I);
            bool visitCallInst(CallInst& I);
        };

    } // End Legalizer namespace

} // End IGC namespace

#endif // LEGALIZER_INSTPROMOTER_H
