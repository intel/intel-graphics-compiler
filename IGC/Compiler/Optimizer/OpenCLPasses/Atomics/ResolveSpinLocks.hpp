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

#include "Compiler/CodeGenPublic.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/ADT/StringRef.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class ResolveSpinLocks : public llvm::ModulePass, public llvm::InstVisitor<ResolveSpinLocks>
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        ResolveSpinLocks();

        /// @brief  Destructor
        ~ResolveSpinLocks() {}

        // Entry point of the pass.
        virtual bool runOnModule(llvm::Module& M) override;

        // Call instructions visitor.
        void visitCallInst(llvm::CallInst& callInst);

    protected:
        llvm::Module* m_pModule;

        /// @brief  Replace the "__builtin_IB_get_local_lock" call with a pointer to a local memory variable.
        /// @param    callInst  call to "__builtin_IB_get_local_lock*" function.
        void           processGetLocalLock(llvm::CallInst& callInst);

        /// @brief  Replace the "__builtin_IB_get_global_lock" call with a pointer to a global memory variable.
        /// @param    callInst  call to "__builtin_IB_get_global_lock*" function.
        void           processGetGlobalLock(llvm::CallInst& callInst);

        /// @brief  Stores the value of local value used for spinlock for i64 local atomics emulation.
        llvm::GlobalVariable* m_localLock = nullptr;

        /// @brief  Stores the value of global value used for spinlock for i64 global atomics emulation.
        llvm::GlobalVariable* m_globalLock = nullptr;

        /// @brief  Indicates if the pass changed the processed function
        bool m_changed = false;
    };
}