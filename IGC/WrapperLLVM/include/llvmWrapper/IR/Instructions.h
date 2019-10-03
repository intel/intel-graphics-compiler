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

#ifndef IGCLLVM_IR_INSTRUCTIONS_H
#define IGCLLVM_IR_INSTRUCTIONS_H

#include <llvm/IR/Instructions.h>

namespace IGCLLVM
{
    template<typename T>
    inline void CopySyncScopeID(T* LHS, T* RHS)
    {
#if LLVM_VERSION_MAJOR == 4
        LHS->setSynchScope(RHS->getSynchScope());
#elif LLVM_VERSION_MAJOR >= 7
        LHS->setSyncScopeID(RHS->getSyncScopeID());
#endif
    }

    template<class T = llvm::Instruction>
    inline
#if LLVM_VERSION_MAJOR == 4
        llvm::SynchronizationScope
#elif LLVM_VERSION_MAJOR >= 7
        llvm::SyncScope::ID
#endif
        getSyncScopeID(T* I)
    {
#if LLVM_VERSION_MAJOR == 4
        return I->getSynchScope();
#elif LLVM_VERSION_MAJOR >= 7
        return I->getSyncScopeID();
#endif
    }

    template<class T = llvm::Instruction>
    inline void DeleteInstruction(T*& I)
    {
#if LLVM_VERSION_MAJOR == 4
        delete I;
#elif LLVM_VERSION_MAJOR >= 7
        I->deleteValue();
#endif
    }

    template<class T = llvm::Instruction>
    inline unsigned getDestAlignment(T* I)
    {
#if LLVM_VERSION_MAJOR == 4
        return I->getAlignment();
#elif LLVM_VERSION_MAJOR >= 7
        return I->getDestAlignment();
#endif
    }
}

#endif
