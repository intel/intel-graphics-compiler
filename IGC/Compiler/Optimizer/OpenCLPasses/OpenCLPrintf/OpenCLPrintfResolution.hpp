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
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"

#include <llvmWrapper/IR/Module.h>

#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    /// @brief  This structure contains information about a Printf argument.
    ///
    struct SPrintfArgDescriptor
    {
        SHADER_PRINTF_TYPE  argType;
        uint                vecSize;
        llvm::Value        *value;

        SPrintfArgDescriptor(SHADER_PRINTF_TYPE _argType, llvm::Value *_value, uint _vecSize = 0) :
            argType(_argType), value(_value), vecSize(_vecSize) {};

        SPrintfArgDescriptor() :
            argType(SHADER_PRINTF_INVALID), value(nullptr) {};
    };


    /// @brief  This pass expands all printf calls into a sequence of instructions
    ///         that writes printf data into the printf output buffer.
    ///         The format of printf output buffer is shown in OpenCLPrintfResolution.cpp
    ///         and in the "IGC Printf Implementation" whitepaper.
    class OpenCLPrintfResolution : public llvm::FunctionPass, public llvm::InstVisitor<OpenCLPrintfResolution>
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        OpenCLPrintfResolution();

        /// @brief  Destructor
        ~OpenCLPrintfResolution() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "OpenCLPrintfResolution";
        }

        void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        virtual bool doInitialization(llvm::Module &M) override;

        /// @brief  Pass entry point.
        virtual bool runOnFunction(llvm::Function &F) override;

        void visitCallInst(llvm::CallInst &callInst);

    private:
        // Construct a name for named metadata node that is used
        // to keep the kernel implicit arguments created for printf.
        std::string getPrintfStringsMDNodeName(llvm::Function &F);

        // Function that takes care of chararcters that are not able to be printed
        // like \n, \r, \t,.......
        std::string getEscapedString(const llvm::ConstantDataSequential* pCDS);

        // If printfArg is string, adds the string into metadata.
        // Returns the string index if the argument is string, and -1 otherwise.
        int  processPrintfString(llvm::Value* printfArg, llvm::Function &F);

        // Replaces a printf call with a sequence of IR instrictions.
        void expandPrintfCall(llvm::CallInst &printfCall, llvm::Function &F);
    
        // Walkes over printf arguments (including the format string) and adds them to printfArgs.
        // If an argument has vector type, adds the vector elements instead.
        void preprocessPrintfArgs(llvm::CallInst &printfCall);

        // Fix up an individual scalar argument.
        // If an argument is a double, convert it into a float, since Gen
        // doesn't support doubles.
        // Returns the fixed version of the argument.
        llvm::Value* fixupPrintfArg(llvm::CallInst &printfCall, llvm::Value* arg, SHADER_PRINTF_TYPE & argDataType);

//        llvm::Value* fixupVectorPrintfArg(llvm::CallInst &printfCall, llvm::Value* arg, SHADER_PRINTF_TYPE argDataType);

        // Returns true if a printf argument is string.
        bool argIsString(llvm::Value *printfArg);

        // Generates atomic_add function call:
        //   ret_val = atomic_add(output_buffer_ptr, data_size)
        // Returns the ret_val.
        llvm::CallInst* genAtomicAdd(llvm::Value *outputBufferPtr, llvm::Value *dataSize,
                                     llvm::CallInst &printfCall, llvm::StringRef name);

        // Computes the total size of output buffer space that is necessary
        // to keep the printf arguments.
        unsigned int getTotalDataSize();

        // Returns the size (in bytes) of printf argument type.
        unsigned int getArgTypeSize(SHADER_PRINTF_TYPE argType, uint vecSize);

        // Returns the SHADER_PRINTF_TYPE type of printf argument.
        SHADER_PRINTF_TYPE getPrintfArgDataType(llvm::Value *printfArg);

        // Creates Cast instruction that converts writeOffset to a pointer type
        // corresponding to the arg type.
        llvm::Instruction *generateCastToPtr(SPrintfArgDescriptor *argDesc,
                                             llvm::Value *writeOffset, llvm::BasicBlock *bblock);

    private:
        IGCLLVM::Module        *m_module;
        llvm::LLVMContext   *m_context;
        llvm::Function      *m_atomicAddFunc;
        unsigned int         m_stringIndex;
        llvm::IntegerType   *m_ptrSizeIntType;
        llvm::IntegerType   *m_int32Type;
        llvm::DebugLoc       m_DL;
        IGC::CodeGenContext *m_CGContext;
        bool                 m_fp64Supported;

        std::vector<llvm::CallInst*>        m_printfCalls;
        std::vector<SPrintfArgDescriptor>   m_argDescriptors;
    };

} // namespace IGC
