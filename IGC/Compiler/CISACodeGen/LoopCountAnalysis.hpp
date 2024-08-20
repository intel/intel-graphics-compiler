/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/ADT/StringRef.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    llvm::FunctionPass* createLoopCountAnalysisPass();

    class CollectLoopCount : public llvm::ImmutablePass
    {
        public:
            CollectLoopCount();

            virtual llvm::StringRef getPassName() const override
            {
                return "collectLoopCount";
            }

            struct ArgSym {
                int argNo;
                int byteOffset;
                int sizeInBytes;
                bool isInDirect;
                ArgSym(int num, int offset, int bytes, bool direct) :argNo(num), byteOffset(offset), sizeInBytes(bytes), isInDirect(direct) {}

                bool operator==(const ArgSym& argsym) const {
                    return argNo == argsym.argNo && byteOffset == argsym.byteOffset && sizeInBytes == argsym.sizeInBytes && isInDirect == argsym.isInDirect;
                }
            };

            struct LCE {
                float factor;
                int argIndex;
                float C;
                LCE(float loopFactor, int argInd, float constant) : factor(loopFactor), argIndex(argInd), C(constant) {}
            };

            static char ID;
            void addLCE(int argNo, int byteOffset, int sizeInBytes, bool isInDirect, float factor, float C);
            std::vector<LCE>& getLCE();
            std::vector<ArgSym>& getloopArgs();

        private:
            std::vector<ArgSym> loopArgs;
            std::vector<LCE> loopCountExpressions;

    };
}