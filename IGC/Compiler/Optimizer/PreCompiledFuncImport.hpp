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

#include "common/debug/Debug.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/ADT/DenseMap.h>
#include "common/LLVMWarningsPop.hpp"


namespace IGC
{
    typedef struct {
        const char* FuncName;   // Name used in precompiled modules
        int LibModID;           // LibraryModules
    } PreCompiledFuncInfo;

    typedef struct {
        const unsigned char* Mod;   // Module binary in memory
        int ModSize;                // The number of bytes of this Module
    } LibraryModuleInfo;

    enum EmuKind : uint8_t {
        EMU_UNUSED = 0,
        EMU_I64DIVREM = 0x1,    // bit 0: original emulation lib, mostly i64 div/rem
        EMU_DP = 0x2,           // bit 1: IEEE-compliant double emulation (+-*/,cmp,convert,etc)
        EMU_SP_DIV = 0x4,       // bit 2: IEEE-complaint float div emulation (float)

    };

    class PreCompiledFuncImport : public llvm::ModulePass, public llvm::InstVisitor<PreCompiledFuncImport>
    {
    public:

        enum LibraryModules
        {
            LIBMOD_INT_DIV_REM,      // [u|s][div|rem], and their vector forms
            LIBMOD_DP_ADD_SUB,       // dp_add & dp_sub
            LIBMOD_DP_FMA_MUL,       // dp_mul & dp_fma
            LIBMOD_DP_DIV,           // dp_div
            LIBMOD_DP_CMP,           // dp_cmp
            LIBMOD_DP_CONV_I32,      // dp_to_[u]int32 & [u]int32_to_dp
            LIBMOD_DP_CONV_SP,       // dp_to_sp & sp_to_dp
            LIBMOD_DP_SQRT,          // dp_sqrt
            LIBMOD_SP_DIV,           // sp_div

            // This must be the last entry
            NUM_LIBMODS
        };

        // TODO: merge this with EmulatedFunctions
        // All this table should be auto-generated !
        enum FunctionIDs
        {
            FUNCTION_DP_ADD,
            FUNCTION_DP_SUB,
            FUNCTION_DP_FMA,
            FUNCTION_DP_MUL,
            FUNCTION_DP_DIV,
            FUNCTION_DP_CMP,
            FUNCTION_DP_TO_I32,
            FUNCTION_DP_TO_UI32,
            FUNCTION_I32_TO_DP,
            FUNCTION_UI32_TO_DP,
            FUNCTION_DP_TO_SP,
            FUNCTION_SP_TO_DP,
            FUNCTION_DP_SQRT,
            FUNCTION_SP_DIV,

            NUM_FUNCTION_IDS
        };

        enum EmulatedFunctions
        {
            FUNCTION_UDIV,
            FUNCTION_UREM,
            FUNCTION_SDIV,
            FUNCTION_SREM,
            NUM_FUNCTIONS
        };


        enum EmulatedFunctionTypes
        {
            TYPE_SCALAR,
            TYPE_VEC2,
            TYPE_VEC3,
            TYPE_VEC4,
            TYPE_VEC8,
            TYPE_VEC16,
            NUM_TYPES
        };

        // Pass identification, replacement for typeid
        static char ID;

        // For pass registration
        PreCompiledFuncImport() :
            ModulePass(ID),
            m_emuKind(EMU_UNUSED)
        {};

        PreCompiledFuncImport(CodeGenContext *CGCtx, uint32_t TheEmuKind);

        ~PreCompiledFuncImport() {}

        void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
        }

        llvm::StringRef getPassName() const override
        {
            return "PreCompiledFuncImport";
        }

        virtual bool runOnModule(llvm::Module &M) override;

        void visitBinaryOperator(llvm::BinaryOperator &I);
        void visitCastInst(llvm::CastInst& I);
        void visitFPTruncInst(llvm::FPTruncInst &inst);
        void visitFPExtInst(llvm::FPExtInst& I);
        void visitFCmpInst(llvm::FCmpInst& I);
        void visitCallInst(llvm::CallInst& I);

        static void checkAndSetEnableSubroutine(CodeGenContext *CGCtx);

    private:
        void processDivide(llvm::BinaryOperator &inst, EmulatedFunctions function);

        void processFPBinaryOperator(llvm::Instruction& I, FunctionIDs FID);
        llvm::Function* getOrCreateFunction(FunctionIDs FID);
        llvm::Value* createFlagValue(llvm::Function *F);
        uint32_t getFCmpMask(llvm::CmpInst::Predicate Pred);

        // Metadata & implicit args (IA)
        //   FuncNeedIA: original Functions that needs IA
        //   NewFuncWithIA: NewFuncWithIA[i] is the new function
        //                  of FuncNeedIA[i], with IA appended
        //                  to the new function's argument list.
        llvm::SmallVector<llvm::Function*, 8> FuncNeedIA;
        llvm::SmallVector<llvm::Function*, 8> NewFuncWithIA;
        llvm::DenseMap<llvm::Function *, ImplicitArgs *> FuncsImpArgs;
        void addMDFuncEntryForEmulationFunc(llvm::Function *F);
        bool usePrivateMemory(llvm::Function *F);
        void createFuncWithIA();
        void replaceFunc(llvm::Function* old_func, llvm::Function* new_func);
        llvm::FunctionType* getNewFuncType(
            llvm::Function* pFunc, const ImplicitArgs* pImplicitArgs);
        ImplicitArgs* getImplicitArgs(llvm::Function *F);

        bool preProcessDouble();
        void eraseFunction(llvm::Module* M, llvm::Function *F) {
            M->getFunctionList().remove(F);
            delete F;
        }

        bool shouldWeSubroutine(llvm::Function* F);

        // Check if subroutine call is needed and set it if so.
        void checkAndSetEnableSubroutine();
        CodeGenContext* m_pCtx;
        bool m_enableSubroutineCallForEmulation;

        IGCMD::MetaDataUtils *m_pMdUtils;
        llvm::Module *m_pModule;

        /// @brief  Indicates if the pass changed the processed function
        bool m_changed;

        const static char *m_sFunctionNames[NUM_FUNCTIONS][NUM_TYPES];

        /// @brief  Kind of emulations. Its bits are defined by EmuKind.
        const uint32_t m_emuKind;
        bool isDPEmu() const { return (m_emuKind & EmuKind::EMU_DP) > 0; }
        bool isI64DivRem() const { return (m_emuKind & EmuKind::EMU_I64DIVREM) > 0; }
        bool isSPDiv() const { return (m_emuKind & EmuKind::EMU_SP_DIV) > 0; }
        bool isDPConvFunc(llvm::Function *F) const;

        bool m_libModuleToBeImported[NUM_LIBMODS];
        bool m_libModuleAlreadyImported[NUM_LIBMODS];

        bool Int32DivRemEmuRemaining = true;

        static const PreCompiledFuncInfo m_functionInfos[NUM_FUNCTION_IDS];
        static const LibraryModuleInfo m_libModInfos[NUM_LIBMODS];
    };

} // namespace IGC
