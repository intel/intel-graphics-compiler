/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
        EMU_I64DIVREM = 0x1,      // bit 0: original emulation lib, mostly i64 div/rem
        EMU_DP = 0x2,             // bit 1: IEEE-compliant double emulation (+-*/,cmp,convert,etc)
        EMU_DP_DIV_SQRT = 0x4,    // bit 2: IEEE-compliant double emulation for div and sqrt (EMU_DP subset)
        EMU_DP_CONV = 0x8,        // bit 3: IEEE-compliant double emulation for conversions (EMU_DP subset)
        EMU_SP_DIV = 0x10,        // bit 4: IEEE-complaint float div emulation (float)
        EMU_I32DIVREM = 0x20,     // bit 5: emulation lib for i32 div/rem
        EMU_I32DIVREM_SP = 0x40,  // bit 6: emulation lib for i32 div/rem using fp32
        EMU_FP64_FP16_CONV = 0x80 // bit 7: fp64 to fp32 rte conversion
    };

    class PreCompiledFuncImport : public llvm::ModulePass, public llvm::InstVisitor<PreCompiledFuncImport>
    {
    public:

        enum LibraryModules
        {
            LIBMOD_INT_DIV_REM,      // [u|s][div|rem], and their vector forms
            LIBMOD_UINT32_DIV_REM,   // [u][div|rem], for 32 bit integers
            LIBMOD_SINT32_DIV_REM,   // [s][div|rem], for 32 bit integers
            LIBMOD_UINT32_DIV_REM_SP,// [u][div|rem], for 32 bit integers using fp32
            LIBMOD_SINT32_DIV_REM_SP,// [s][div|rem], for 32 bit integers using fp32
            LIBMOD_DP_ADD_SUB,       // dp_add & dp_sub
            LIBMOD_DP_FMA_MUL,       // dp_mul & dp_fma
            LIBMOD_DP_DIV,           // dp_div
            LIBMOD_DP_DIV_NOMADM,    // dp_div_nomadm_ieee & dp_div_nomadm_fast
            LIBMOD_DP_CMP,           // dp_cmp
            LIBMOD_DP_CONV_I32,      // dp_to_[u]int32 & [u]int32_to_dp
            LIBMOD_DP_CONV_SP,       // dp_to_sp & sp_to_dp
            LIBMOD_DP_SQRT,          // dp_sqrt
            LIBMOD_DP_SQRT_NOMADM,   // dp_sqrt_nomadm_ieee & dp_sqrt_nomadm_fast
            LIBMOD_SP_DIV,           // sp_div
            LIBMOD_DP_CONV_I64,      // dp_to_[u]int64 & [u]int64_to_dp
            LIBMOD_INT64_DIV_REM_SP, // [u|s][div|rem], for 64 bit integers using fp32
            LIBMOD_INT64_DIV_REM_DP, // [u|s][div|rem], for 64 bit integers using fp64

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
            FUNCTION_DP_DIV_NOMADM_IEEE,
            FUNCTION_DP_DIV_NOMADM_FAST,
            FUNCTION_DP_CMP,
            FUNCTION_DP_TO_I32,
            FUNCTION_DP_TO_UI32,
            FUNCTION_I32_TO_DP,
            FUNCTION_UI32_TO_DP,
            FUNCTION_DP_TO_SP,
            FUNCTION_SP_TO_DP,
            FUNCTION_DP_SQRT,
            FUNCTION_DP_SQRT_NOMADM_IEEE,
            FUNCTION_DP_SQRT_NOMADM_FAST,
            FUNCTION_SP_DIV,
            FUNCTION_DP_TO_I64,
            FUNCTION_DP_TO_UI64,
            FUNCTION_I64_TO_DP,
            FUNCTION_UI64_TO_DP,

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

        enum Int32EmulatedFunctions
        {
            FUNCTION_32_UDIVREM,
            FUNCTION_32_SDIVREM,
            FUNCTION_32_UDIVREM_SP,
            FUNCTION_32_SDIVREM_SP,
            NUM_INT32_EMU_FUNCTIONS
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

        enum EmuRoundingMode
        {
            ROUND_TO_NEAREST_EVEN,
            ROUND_TO_NEGATIVE,
            ROUND_TO_POSITIVE,
            ROUND_TO_ZERO,
            ROUND_TO_ANY
        };

        // Pass identification, replacement for typeid
        static char ID;

        // For pass registration
        PreCompiledFuncImport() :
            ModulePass(ID),
            m_emuKind(EMU_UNUSED)
        {};

        PreCompiledFuncImport(CodeGenContext* CGCtx, uint32_t TheEmuKind);

        ~PreCompiledFuncImport() {}

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
        }

        llvm::StringRef getPassName() const override
        {
            return "PreCompiledFuncImport";
        }

        void releaseMemory() override
        {
            FuncNeedIA.clear();
            NewFuncWithIA.clear();
            FuncsImpArgs.clear();
            m_DPEmuFlagTemp.clear();
            m_allNewCallInsts.clear();
        }

        virtual bool runOnModule(llvm::Module& M) override;

        void visitBinaryOperator(llvm::BinaryOperator& I);
        void visitCastInst(llvm::CastInst& I);
        void visitFPTruncInst(llvm::FPTruncInst& inst);
        void visitFPExtInst(llvm::FPExtInst& I);
        void visitFCmpInst(llvm::FCmpInst& I);
        void visitCallInst(llvm::CallInst& I);

        static void checkAndSetEnableSubroutine(CodeGenContext* CGCtx);

    private:

        // Data about imported function.
        struct ImportedFunction
        {
        public:
            enum EmuType
            {
                INT64,
                FASTDP, // faster DP div/rem for platforms without MADM instruction
                SLOWDP,
                OTHER
            };

            ImportedFunction(llvm::Function* F);

            bool isSlowDPEmuFunc() { return type == SLOWDP; }

            void updateUses();

            bool operator==(const llvm::Function* F) const {
                return this->F == F;
            }

            static ImportedFunction copy(ImportedFunction& other);
            static bool compare(ImportedFunction& L, ImportedFunction& R);

            llvm::Function* F;
            EmuType type;
            unsigned funcInstructions;
            unsigned totalInstructions;

        private:
            ImportedFunction(llvm::Function* F, EmuType type, unsigned funcInstructions, unsigned totalInstructions)
                : F(F), type(type), funcInstructions(funcInstructions), totalInstructions(totalInstructions) {}
        };

        void processDivide(llvm::BinaryOperator& inst, EmulatedFunctions function);
        void getInt64DivideEmuType(EmulatedFunctions function, unsigned int elementIndex, const char*& functionName, LibraryModules& module);
        llvm::BinaryOperator* upcastTo32Bit(llvm::BinaryOperator* I);
        void processInt32Divide(llvm::BinaryOperator& inst, Int32EmulatedFunctions function);

        void processFPBinaryOperator(llvm::Instruction& I, FunctionIDs FID);
        llvm::Function* getOrCreateFunction(FunctionIDs FID);
        llvm::Value* createFlagValue(llvm::Function* F);
        uint32_t getFCmpMask(llvm::CmpInst::Predicate Pred);

        // Metadata & implicit args (IA)
        //   FuncNeedIA: original Functions that needs IA
        //   NewFuncWithIA: NewFuncWithIA[i] is the new function
        //                  of FuncNeedIA[i], with IA appended
        //                  to the new function's argument list.
        llvm::SmallVector<llvm::Function*, 8> FuncNeedIA;
        llvm::SmallVector<llvm::Function*, 8> NewFuncWithIA;
        llvm::DenseMap<llvm::Function*, ImplicitArgs*> FuncsImpArgs;
        void addMDFuncEntryForEmulationFunc(llvm::Function* F);
        bool usePrivateMemory(llvm::Function* F);
        void createFuncWithIA();
        void replaceFunc(llvm::Function* old_func, llvm::Function* new_func);
        llvm::FunctionType* getNewFuncType(
            llvm::Function* pFunc, const ImplicitArgs* pImplicitArgs);
        ImplicitArgs* getImplicitArgs(llvm::Function* F);

        bool preProcessDouble();
        void handleInstrTypeChange(llvm::Instruction* oldInst, llvm::Value* newVal);
        void eraseFunction(llvm::Module* M, llvm::Function* F) {
            M->getFunctionList().remove(F);
            delete F;
        }

        // Remove llvm.module.flags metadata before linking
        void removeLLVMModuleFlag(llvm::Module* M);

        ImportedFunction createInlinedCopy(ImportedFunction& IF, unsigned calls);

        // Check if subroutine call is needed and set it if so.
        void checkAndSetEnableSubroutine();
        CodeGenContext* m_pCtx = nullptr;
        int m_enableCallForEmulation{}; // subroutine/stackcall

        IGCMD::MetaDataUtils* m_pMdUtils = nullptr;
        llvm::Module* m_pModule = nullptr;

        /// @brief  Indicates if the pass changed the processed function
        bool m_changed{};

        const static char* m_Int64DivRemFunctionNames[NUM_FUNCTIONS][NUM_TYPES];
        const static char* m_Int64SpDivRemFunctionNames[NUM_FUNCTIONS][NUM_TYPES];
        const static char* m_Int64DpDivRemFunctionNames[NUM_FUNCTIONS][NUM_TYPES];
        const static char* m_Int32EmuFunctionNames[NUM_INT32_EMU_FUNCTIONS];

        /// @brief  Kind of emulations. Its bits are defined by EmuKind.
        uint32_t m_emuKind;
        bool isDPEmu() const { return (m_emuKind & EmuKind::EMU_DP) > 0; }
        bool isDPDivSqrtEmu() const { return (m_emuKind & EmuKind::EMU_DP_DIV_SQRT) > 0; }
        bool isDPConvEmu() const { return (m_emuKind & EmuKind::EMU_DP_CONV) > 0; }
        bool isI64DivRem() const { return (m_emuKind & EmuKind::EMU_I64DIVREM) > 0; }
        bool isI32DivRem() const { return (m_emuKind & EmuKind::EMU_I32DIVREM) > 0; }
        bool isI32DivRemSP() const { return (m_emuKind & EmuKind::EMU_I32DIVREM_SP) > 0; }
        bool isSPDiv() const { return (m_emuKind & EmuKind::EMU_SP_DIV) > 0; }
        bool isRTEFP64toFP16() const { return (m_emuKind & EmuKind::EMU_FP64_FP16_CONV) > 0; }

        bool m_libModuleToBeImported[NUM_LIBMODS];
        bool m_libModuleAlreadyImported[NUM_LIBMODS];

        bool Int32DivRemEmuRemaining = true;

        static const PreCompiledFuncInfo m_functionInfos[NUM_FUNCTION_IDS];
        static const LibraryModuleInfo m_libModInfos[NUM_LIBMODS];
        // This is for creating a single flag temp for some of double emulation
        // As this is never used, we need to remove this flag from emulation
        // function completely.
        llvm::DenseMap<llvm::Function*, llvm::Value*> m_DPEmuFlagTemp;

        unsigned m_roundingMode = 0;
        unsigned m_flushDenorm = 0;
        unsigned m_flushToZero = 0;

        llvm::SmallVector<llvm::CallInst*, 16> m_CallRemDiv;

        // Calling convention:
        //    LLVM requires a call and its callee have the same calling convention;
        //    otherwise, the call would be deleted (for example, instcombine would
        //    delete it).
        //
        // The functions in lib could have a dfferent calling conventions than default
        // (for example, spirv_func or default). Before linking lib functions, we do
        // not know which calling convention to use. We first create a call with default
        // calling convention, then link libs (So far, LLVM linking does not check mismatch
        // of calling convention). To make sure a call and its callee have the same calling
        // convention, we will set the call's calling convention to its callee's once we
        // finish linking.
        //
        // This vector keeps all new calls whose calling convention will need to be set.
        llvm::SmallVector<llvm::CallInst*, 32> m_allNewCallInsts;
        void addCallInst(llvm::CallInst* CI) { m_allNewCallInsts.push_back(CI); }
        void eraseCallInst(llvm::CallInst* CI);
    };

} // namespace IGC
