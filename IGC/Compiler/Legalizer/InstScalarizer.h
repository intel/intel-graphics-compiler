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

// vim:ts=2:sw=2:et:

#ifndef LEGALIZER_INSTSCALARIZER_H
#define LEGALIZER_INSTSCALARIZER_H

#include "TypeLegalizer.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/InstVisitor.h"
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
