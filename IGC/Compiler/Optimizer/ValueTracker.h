/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
        llvm::Value* trackValue(llvm::Value* I);
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
        std::vector<llvm::CallInst*> callInsts;
        llvm::SmallPtrSet<llvm::Value*, 10> visitedValues;
        std::vector<llvm::Value*> workList;
        std::map<llvm::Value*, llvm::Value*> phiVisited;
    public:
        static llvm::Value* track(
            llvm::CallInst* CI,
            const uint index,
            const IGC::IGCMD::MetaDataUtils* pMdUtils = nullptr,
            const IGC::ModuleMetaData* pModMD = nullptr);
    };
}
