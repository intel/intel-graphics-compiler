/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
        IGC::SHADER_PRINTF_TYPE  argType;
        uint                vecSize;
        llvm::Value* value;

        SPrintfArgDescriptor(IGC::SHADER_PRINTF_TYPE _argType, llvm::Value* _value, uint _vecSize = 0) :
            argType(_argType), vecSize(_vecSize), value(_value) {};

        SPrintfArgDescriptor() :
            argType(IGC::SHADER_PRINTF_INVALID), value(nullptr) {};
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

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        virtual bool doInitialization(llvm::Module& M) override;

        /// @brief  Pass entry point.
        virtual bool runOnFunction(llvm::Function& F) override;

        void visitCallInst(llvm::CallInst& callInst);

    private:
        // Construct a name for named metadata node that is used
        // to keep the kernel implicit arguments created for printf.
        std::string getPrintfStringsMDNodeName(llvm::Function& F);

        // If printfArg is string, adds the string into metadata.
        // Returns the string index if the argument is string, and -1 otherwise.
        llvm::Value* processPrintfString(llvm::Value* arg, llvm::Function& F);

        // Replaces a printf call with a sequence of IR instrictions.
        void expandPrintfCall(llvm::CallInst& printfCall, llvm::Function& F);

        // Removes excess arguments from m_argDescriptors vector. Excess arguments
        // are arguments that do not have a corresponding format specifier in a
        // format string.
        void removeExcessArgs();

        // Returns number of format specifiers in a format string.
        unsigned getNumFormatSpecifiers(const llvm::ConstantDataArray* formatString);

        // Walkes over printf arguments (including the format string) and adds them to printfArgs.
        // If an argument has vector type, adds the vector elements instead.
        void preprocessPrintfArgs(llvm::CallInst& printfCall);

        // Fix up an individual scalar argument.
        // If an argument is a double, convert it into a float, since Gen
        // doesn't support doubles.
        // Returns the fixed version of the argument.
        llvm::Value* fixupPrintfArg(llvm::CallInst& printfCall, llvm::Value* arg, IGC::SHADER_PRINTF_TYPE& argDataType);

        //        llvm::Value* fixupVectorPrintfArg(llvm::CallInst &printfCall, llvm::Value* arg, IGC::SHADER_PRINTF_TYPE argDataType);

                // Returns true if a printf argument is string.
        bool argIsString(llvm::Value* printfArg);

        // Generates atomic_add function call:
        //   ret_val = atomic_add(output_buffer_ptr, data_size)
        // Returns the ret_val.
        llvm::CallInst* genAtomicAdd(llvm::Value* outputBufferPtr, llvm::Value* dataSize,
            llvm::CallInst& printfCall, llvm::StringRef name);

        // Computes the total size of output buffer space that is necessary
        // to keep the printf arguments.
        unsigned int getTotalDataSize();

        // Returns the size (in bytes) of printf argument type.
        unsigned int getArgTypeSize(IGC::SHADER_PRINTF_TYPE argType, uint vecSize);

        // Returns the IGC::SHADER_PRINTF_TYPE type of printf argument.
        IGC::SHADER_PRINTF_TYPE getPrintfArgDataType(llvm::Value* printfArg);

        // Creates Cast instruction that converts writeOffset to a pointer type
        // corresponding to the arg type.
        llvm::Instruction* generateCastToPtr(SPrintfArgDescriptor* argDesc,
            llvm::Value* writeOffset, llvm::BasicBlock* bblock);

    private:
        IGCLLVM::Module* m_module = nullptr;
        llvm::LLVMContext* m_context = nullptr;
        llvm::Function* m_atomicAddFunc = nullptr;
        unsigned int         m_stringIndex{};
        std::map<std::string, unsigned int> m_MapStringStringIndex{};
        llvm::IntegerType* m_ptrSizeIntType = nullptr;
        llvm::IntegerType* m_int32Type = nullptr;
        llvm::DebugLoc       m_DL{};
        IGC::CodeGenContext* m_CGContext = nullptr;
        bool                 m_fp64Supported{};

        std::vector<llvm::CallInst*>        m_printfCalls;
        std::vector<SPrintfArgDescriptor>   m_argDescriptors;
    };

} // namespace IGC
