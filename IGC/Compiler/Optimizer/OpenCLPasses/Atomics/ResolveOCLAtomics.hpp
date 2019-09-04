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

#include <map>
namespace IGC
{
    // Attributes for each __builtin_IB_atomic function.
    //
    //              bufType
    //                 |
    //       not used  V   op
    //       |-------|---|---|
    //    0 x 0 0 0 0 0 0 0 0 
    //
    typedef unsigned int OCLAtomicAttrs;

    // A pass that walks over call instructions and replaces all __builtin_IB_atomic calls
    // with corresponding GenISA intrinsics.
    //
    class ResolveOCLAtomics : public llvm::ModulePass, public llvm::InstVisitor<ResolveOCLAtomics>
    {
    public:
    // Pass identification, replacement for typeid
    static char ID;

    /// @brief  Constructor
    ResolveOCLAtomics();

    /// @brief  Destructor
    ~ResolveOCLAtomics() {}

    /// @brief  Provides name of pass
    virtual llvm::StringRef getPassName() const override
    {
        return "ResolveOCLAtomics";
    }

    // Entry point of the pass.
        virtual bool runOnModule(llvm::Module& M) override;

    // Call instructions visitor.
        void visitCallInst(llvm::CallInst& callInst);

    static const unsigned int ATTR_BUFFER_TYPE_SHIFT = 8;

    protected:
        llvm::Module* m_pModule;
        llvm::IntegerType* m_Int32Ty;
        bool               m_64bitPointer;

        // A map that keeps attributes for each "__builtin_IB_atomic_*" function name.
        std::map<llvm::StringRef, OCLAtomicAttrs>  m_AtomicDescMap;

        void initResolveOCLAtomics();

        /// @brief    Packs the given atomic operation and buffer type into "attributes" integer.
        /// @param    op        Atomic operation
        /// @param    bufType   Buffer Type
        /// @returns  Computed "attributes" integer.
        OCLAtomicAttrs genAtomicAttrs(AtomicOp op, BufferType bufType);

        /// @brief  Get the atomic operation for atomic function name.
        /// @param    name    The name of atomic intrinsic.
        /// @returns  The atomic operation.
        AtomicOp       getAtomicOp(llvm::StringRef name);

        /// @brief  Get the buffer type for atomic function name.
        /// @param    name    The name of atomic intrinsic.
        /// @returns  The buffer type.
        BufferType     getBufType(llvm::StringRef name);

        /// @brief  Initialize the "atomic name <--> attributes" map.
        void           initOCLAtomicsMap();

        /// @brief  Replace the "__builtin_IB_atomic_*" call with a call to GenISA atomic intrinsic.
        /// @param    callInst  call to "__builtin_IB_atomic_*" function.
        /// @param    op        atomic operation.
        /// @param    bufType   buffer type.
        void           processOCLAtomic(llvm::CallInst& callInst, AtomicOp op, BufferType bufType);

        /// @brief  Generates a call to "GenISA_GetBufferPtr" intrinsic.
        /// @param    callInst   call instruction to "__builtin_IB_atomic_*" built-in function.
        /// @param    bufType    corresponding buffer type.
        /// @returns  call instruction to generated GenISA_GetBufferPtr.
        llvm::CallInst* genGetBufferPtr(llvm::CallInst& callInst, BufferType bufType);
        
        /// @brief  Replace the "__builtin_IB_get_local_lock" call with a pointer to a local memory variable.
        /// @param    callInst  call to "__builtin_IB_get_local_lock*" function.
        void           processGetLocalLock(llvm::CallInst& callInst);

        /// @brief  Stores the value of local value used for spinlock for i64 local atomics emulation.
        llvm::GlobalVariable* m_localLock = nullptr;
        
        /// @brief  Indicates if the pass changed the processed function
        bool m_changed = false;
    };
}