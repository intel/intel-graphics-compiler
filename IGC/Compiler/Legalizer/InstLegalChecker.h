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

#ifndef LEGALIZER_INSTCHECKER_H
#define LEGALIZER_INSTCHECKER_H

#include "TypeLegalizer.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvmWrapper/IR/InstVisitor.h"
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

    namespace Legalizer {

        class TypeLegalizer;

        class InstLegalChecker : public InstVisitor<InstLegalChecker, LegalizeAction> {
            friend class InstVisitor<InstLegalChecker, LegalizeAction>;

            TypeLegalizer* TL;

        public:
            InstLegalChecker(TypeLegalizer* L) : TL(L) {}

            // Check whether that instruction is legal. Return true iff that
            // instruction is legal and false otherwise.
            LegalizeAction check(Instruction* I) { return visit(*I); }

        private:
            // By default, capture all missing instructions!
            LegalizeAction visitInstruction(Instruction& I);

            /// Terminator instructions
            ///

            LegalizeAction visitReturnInst(ReturnInst& I);
            LegalizeAction visitTerminatorInst(IGCLLVM::TerminatorInst&);

            /// Standard binary operators
            ///

            LegalizeAction visitBinaryOperator(BinaryOperator& I);

            /// Memory operators
            ///

            LegalizeAction visitAllocaInst(AllocaInst& I);
            LegalizeAction visitLoadInst(LoadInst& I);
            LegalizeAction visitStoreInst(StoreInst& I);
            LegalizeAction visitGetElementPtrInst(GetElementPtrInst& I);
            LegalizeAction visitFenceInst(FenceInst&);
            LegalizeAction visitAtomicCmpXchgInst(AtomicCmpXchgInst&);
            LegalizeAction visitAtomicRMWInst(AtomicRMWInst&);

            /// Cast operators
            ///

            LegalizeAction visitCastInst(CastInst& I);

            /// Other operators
            ///

            LegalizeAction visitCmpInst(CmpInst& I);
            LegalizeAction visitPHINode(PHINode& I);

            // Special intrinsics
            LegalizeAction visitDbgDeclareInst(DbgDeclareInst& I) { return Legal; }
            LegalizeAction visitDbgValueInst(DbgValueInst& I) { return Legal; }
            LegalizeAction visitDbgInfoIntrinsic(DbgInfoIntrinsic& I) { return Legal; }
            LegalizeAction visitMemTransferInst(MemTransferInst& I) { return Legal; }
            LegalizeAction visitMemIntrinsic(MemIntrinsic& I) { return Legal; }
            LegalizeAction visitVAStartInst(VAStartInst& I) { return Legal; }
            LegalizeAction visitVAEndInst(VAEndInst& I) { return Legal; }
            LegalizeAction visitVACopyInst(VACopyInst& I) { return Legal; }

            LegalizeAction visitIntrinsicInst(IntrinsicInst& I);
            LegalizeAction visitGenIntrinsicInst(GenIntrinsicInst& I);
            LegalizeAction visitCallInst(CallInst& I);
            LegalizeAction visitSelectInst(SelectInst& I);
            LegalizeAction visitVAArgInst(VAArgInst&);
            LegalizeAction visitExtractElementInst(ExtractElementInst& I);
            LegalizeAction visitInsertElementInst(InsertElementInst& I);
            LegalizeAction visitShuffleVectorInst(ShuffleVectorInst& I);
            LegalizeAction visitExtractValueInst(ExtractValueInst& I);
            LegalizeAction visitInsertValueInst(InsertValueInst& I);
            LegalizeAction visitLandingPadInst(LandingPadInst&);
#if LLVM_VERSION_MAJOR >= 10
            LegalizeAction visitFNeg(llvm::UnaryOperator& I);
#endif
        };

    } // End Legalizer namespace

} // End IGC namespace

#endif // LEGALIZER_INSTCHECKER_H
