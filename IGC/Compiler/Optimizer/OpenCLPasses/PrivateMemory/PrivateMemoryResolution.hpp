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

#include "Compiler/CISACodeGen/GenCodeGenModule.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;

namespace IGC
{
    /// Lower down alloca to private memory
    ModulePass* CreatePrivateMemoryResolution();

    /// \brief Analyze alloca instructions and determine the size and offset of
    /// each alloca and the total amount of private memory needed by each kernel.
    class ModuleAllocaInfo {
    public:
        ModuleAllocaInfo(Module* M, const DataLayout* DL,
            GenXFunctionGroupAnalysis* FGA = nullptr)
            : M(M), DL(DL), FGA(FGA) {
            analyze();
        }

        ~ModuleAllocaInfo() {
            for (auto I = InfoMap.begin(), E = InfoMap.end(); I != E; ++I)
                delete I->second;
        }

        ModuleAllocaInfo(const ModuleAllocaInfo&) = delete;
        ModuleAllocaInfo& operator=(const ModuleAllocaInfo&) = delete;

        /// \brief Return the offset of alloca instruction in private memory buffer.
        //  This function should not be called when AI is variable length alloca
        unsigned getConstBufferOffset(AllocaInst* AI) const {
            IGC_ASSERT(isa<ConstantInt>(AI->getArraySize()));
            Function* F = AI->getParent()->getParent();
            return getFuncAllocaInfo(F)->AllocaDesc[AI].first;
        }

        /// \brief Return the size of alloca instruction in private memory buffer.
        //  This function should not be called when AI is variable length alloca
        unsigned getConstBufferSize(AllocaInst* AI) const {
            IGC_ASSERT(isa<ConstantInt>(AI->getArraySize()));
            Function* F = AI->getParent()->getParent();
            return getFuncAllocaInfo(F)->AllocaDesc[AI].second;
        }

        /// \brief Return all alloca instructions of a given function.
        SmallVector<AllocaInst*, 8> & getAllocaInsts(Function * F) const {
            return getFuncAllocaInfo(F)->Allocas;
        }

        /// \brief Return the total private memory size per WI of a given function.
        unsigned getTotalPrivateMemPerWI(Function* F) const {
            auto FI = getFuncAllocaInfo(F);
            return FI ? FI->TotalSize : 0;
        }

    private:
        /// \brief The module being analyzed.
        Module* const M;

        /// \brief The DataLayout object.
        const DataLayout* const DL;

        /// \brief The optional function group analysis.
        GenXFunctionGroupAnalysis* const FGA;

        struct FunctionAllocaInfo {
            void setAllocaDesc(AllocaInst* AI, unsigned Offset, unsigned Size) {
                AllocaDesc[AI] = std::make_pair(Offset, Size);
            }

            /// \brief Total amount of private memory size per kernel. All functions in
            /// a kernel will have the same size.
            unsigned TotalSize = 0;

            /// \brief Alloca instructions for a function.
            SmallVector<AllocaInst*, 8> Allocas;

            /// \brief Alloca instruction, its offset and size in buffer.
            DenseMap<AllocaInst*, std::pair<unsigned, unsigned>> AllocaDesc;
        };

        FunctionAllocaInfo* getFuncAllocaInfo(Function* F) const {
            auto Iter = InfoMap.find(F);
            if (Iter != InfoMap.end())
                return Iter->second;
            return nullptr;
        }

        FunctionAllocaInfo* getOrCreateFuncAllocaInfo(Function* F) {
            auto Iter = InfoMap.find(F);
            if (Iter != InfoMap.end())
                return Iter->second;

            auto AllocaInfo = new FunctionAllocaInfo;
            InfoMap[F] = AllocaInfo;
            return AllocaInfo;
        }

        /// \brief Analyze the module and fill function alloca info map.
        void analyze();

        /// \brief When function group analysis is available, visit group by group.
        void analyze(FunctionGroup* FG);

        /// \brief Analyze individual functions.
        void analyze(Function* F, unsigned& gOffset, unsigned& gAlignment);

        /// \brief Each function has an entry that describes its private memory
        /// usage information.
        DenseMap<Function*, FunctionAllocaInfo*> InfoMap;
    };
}
