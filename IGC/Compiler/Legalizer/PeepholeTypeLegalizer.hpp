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

// vim:ts=2:sw=2::et:

#ifndef LEGALIZER_PeepholeTypeLegalizer_H
#define LEGALIZER_PeepholeTypeLegalizer_H

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/TargetFolder.h"
#include "llvm/IR/InstVisitor.h"
#include "common/LLVMWarningsPop.hpp"
#include "common/Types.hpp"

namespace IGC {

    namespace Legalizer {

        using namespace llvm;

        class PeepholeTypeLegalizer : public FunctionPass, public InstVisitor<PeepholeTypeLegalizer> {
            llvm::IRBuilder<>* m_builder;
            Module* TheModule;
            Function* TheFunction;

        public:
            static char ID;

            PeepholeTypeLegalizer();

            bool runOnFunction(Function& F) override;

            void visitInstruction(Instruction& I);
            void legalizePhiInstruction(Instruction& I);
            void legalizeUnaryInstruction(Instruction& I);
            void legalizeBinaryOperator(Instruction& I);
            void legalizeExtractElement(Instruction& I);
            void cleanupZExtInst(Instruction& I);
            void cleanupBitCastInst(Instruction& I);

        private:
            bool NonBitcastInstructionsLegalized;
            bool CastInst_ZExtWithIntermediateIllegalsEliminated;
            bool Bitcast_BitcastWithIntermediateIllegalsEliminated;
            bool Changed;

            const DataLayout* DL;

            void getAnalysisUsage(AnalysisUsage& AU) const override;

            LLVMContext& getContext() const { return TheModule->getContext(); }
            Module* getModule() const { return TheModule; }
            Function* getFunction() const { return TheFunction; }
            bool isLegalInteger(unsigned int bitWidth) {
                if (bitWidth == 64)
                    return true;
                else
                    return DL->isLegalInteger(bitWidth);
            }

        };
    } // End Legalizer namespace

} // End IGC namespace

#endif // LEGALIZER_TYPELEGALIZER_H
