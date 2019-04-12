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

#ifndef LEGALIZER_INSTPROMOTER_H
#define LEGALIZER_INSTPROMOTER_H

#include "TypeLegalizer.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/InstVisitor.h"
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

namespace Legalizer {

  class InstPromoter : public InstVisitor<InstPromoter, bool> {
    friend class InstVisitor<InstPromoter, bool>;

    TypeLegalizer *TL;
    BuilderType *IRB;

    Value *Promoted;

  public:
    InstPromoter(TypeLegalizer *L, BuilderType *B) : TL(L), IRB(B) {}

    bool promote(Instruction *I);

  private:
    /// Helpers
    const char *getSuffix() const { return TL->getSuffix(Promote); }

  private:
    // By default, capture all missing instructions!
    bool visitInstruction(Instruction &I);

    /// Terminator instructions
    ///

    bool visitTerminatorInst(IGCLLVM::TerminatorInst &I);

    /// Standard binary operators
    ///
    bool visitSelectInst(SelectInst &I);

    bool visitICmpInst(ICmpInst &I);

    bool visitBinaryOperator(BinaryOperator &I);

    /// Memory operators
    ///

    bool visitLoadInst(LoadInst &I);
    bool visitStoreInst(StoreInst &I);

    /// Cast operators

    bool visitTruncInst(TruncInst &I);
    bool visitZExtInst(ZExtInst &I);
    bool visitBitCastInst(BitCastInst &I);

    /// Other operators

    bool visitGenIntrinsicInst(GenIntrinsicInst &I);
    bool visitCallInst(CallInst &I);
  };

} // End Legalizer namespace

} // End IGC namespace

#endif // LEGALIZER_INSTPROMOTER_H
