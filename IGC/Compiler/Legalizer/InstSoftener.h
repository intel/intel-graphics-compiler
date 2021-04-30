/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef LEGALIZER_INSTSOFTENER_H
#define LEGALIZER_INSTSOFTENER_H

#include "TypeLegalizer.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/InstVisitor.h"
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

    namespace Legalizer {

        class InstSoftener : public InstVisitor<InstSoftener, bool> {
            friend class InstVisitor<InstSoftener, bool>;

            TypeLegalizer* TL;
            BuilderType* IRB;

            Value* Softened;

        public:
            InstSoftener(TypeLegalizer* L, BuilderType* B) : TL(L), IRB(B) {}

            bool soften(Instruction* I);

        private:
            /// Helpers
            const char* getSuffix() const { return TL->getSuffix(SoftenFloat); }
        };

    } // End Legalizer namespace

} // End IGC namespace

#endif // LEGALIZER_INSTSOFTENER_H
