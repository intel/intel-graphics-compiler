/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef LEGALIZER_INSTELEMENTIZER_H
#define LEGALIZER_INSTELEMENTIZER_H

#include "TypeLegalizer.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/InstVisitor.h"
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

    namespace Legalizer {

        class InstElementizer : public InstVisitor<InstElementizer, bool> {
            friend class InstVisitor<InstElementizer, bool>;

            TypeLegalizer* TL;
            BuilderType* IRB;

            ValueSeq Elementized;

        public:
            InstElementizer(TypeLegalizer* L, BuilderType* B) : TL(L), IRB(B) {}

            bool elementize(Instruction* I);

        private:
            /// Helpers
            const char* getSuffix() const { return TL->getSuffix(Elementize); }
        };

    } // End Legalizer namespace

} // End IGC namespace

#endif // LEGALIZER_INSTELEMENTIZER_H
