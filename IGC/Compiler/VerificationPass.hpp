/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

///=======================================================================================
/// This file contains types, enumerations, classes and other declarations used by
/// the IGC IR Verification Pass.
#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/ArrayRef.h>
#include "llvm/Support/raw_ostream.h"
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"

#include <set>
#include <vector>

namespace IGC
{
    enum class IRSpecObjectClass
    {
        Instruction,
        GenISAIntrinsic,
        Intrinsic
    };

    class VerificationPass : public llvm::ModulePass
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        VerificationPass();

        /// Destructor
        ~VerificationPass() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "VerificationPass";
        }

        /// Entry point of the pass.
        virtual bool runOnModule(llvm::Module& M) override;

    protected:
        // Module
        llvm::Module* m_Module;

        // String for collecting verification error messages.
        std::string   m_str;

        // Stream interface for m_str
        llvm::raw_string_ostream m_messagesToDump;

        /// This structure contains sets of valid IGC IR objects
        /// such as types, intrinsic functions, etc.
        struct
        {
            std::set<unsigned int>               FPTypeSizes;
            std::set<unsigned int>               vectorTypeSizes;
            std::set<unsigned int>               Instructions;
            std::set<llvm::Intrinsic::ID>        IGCLLVMIntrinsics;  // LLVM intrinsics supported by IGC
        } m_IGC_IR_spec;

        /// Initialization of the pass data
        void initVerificationPass();

        void addIRSpecObject(IRSpecObjectClass objClass, unsigned int ID);

        /// Verification of the function and its content.
        bool verifyFunction(llvm::Function& F);

        /// Verification of instruction.
        bool verifyInstruction(llvm::Instruction& inst);

        /// Common verification function that is executed
        /// for all types of instructions.
        bool verifyInstCommon(llvm::Instruction& inst);

        ///------------------------------------------------------
        /// Instruction verification functions for specific
        /// types of instructions should be declared here.
        ///------------------------------------------------------

        /// Specific verification function for Call instructions.
        bool verifyInstCall(llvm::Instruction& inst);
        /// Specific verification function for insert/extract instructions.
        bool verifyVectorInst(llvm::Instruction& inst);

        ///------------------------------------------------------

        /// Verification of values
        bool verifyValue(llvm::Value* val);

        /// Verification of type
        bool verifyType(llvm::Type* type, llvm::Value* val);

        /// Dumps the verification errors.
        void dumpMessages(llvm::Module& M);

        /// Prints the LLVM value into m_messagesToDump text stream.
        void printValue(llvm::Value* value);
    };
} // namespace IGC
