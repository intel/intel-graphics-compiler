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

#pragma once

#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/StringRef.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    //===----------------------------------------------------------------------===//
    /// This is the class to track values that needs to be known at compile-time.
    /// This class can be used in three different scenarios, to track: images, samplers or constants.
    /// The algorithm is simply divided into two main steps:
    /// 1. trackValue function goes up through the tree to find an alloca which holds the value
    ///    passed as an parameter to call instruction.
    /// 2. alloca instruction found in point 1. is passed to findAllocaValue function to find
    ///    the source of value stored in that alloca. It can be either image, sampler or constant
    ///    value.
    class ValueTracker
    {
    private:
        ValueTracker(llvm::Function* F, const IGC::IGCMD::MetaDataUtils* pMdUtils, const IGC::ModuleMetaData* pModMD) :
            m_pMDUtils(pMdUtils), m_pModMD(pModMD), m_Function(F) {};
        // The first step of the algorithm
        llvm::Value* trackValue(llvm::CallInst* CI, const uint index);
        llvm::Value* handleGenIntrinsic(llvm::GenIntrinsicInst* I);
        llvm::Value* handleExtractElement(llvm::ExtractElementInst* E);
        llvm::Value* handleGlobalVariable(llvm::GlobalVariable* G);
        llvm::Value* handleConstExpr(llvm::ConstantExpr* CE);

        // The second step of the algorithm
        llvm::Value* findAllocaValue(llvm::Value* V, const uint depth);

        const IGC::IGCMD::MetaDataUtils* m_pMDUtils;
        const IGC::ModuleMetaData* m_pModMD;
        llvm::Function* m_Function;

        std::vector<llvm::ConstantInt*> gepIndices;
        llvm::SmallPtrSet<llvm::Value*, 10> visitedValues;
    public:
        static llvm::Value* track(
            llvm::CallInst* CI,
            const uint index,
            const IGC::IGCMD::MetaDataUtils* pMdUtils = nullptr,
            const IGC::ModuleMetaData* pModMD = nullptr);
    };
}