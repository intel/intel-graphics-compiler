/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IGC/common/StringMacros.hpp"
#include "Compiler/Optimizer/OpenCLPasses/LSCFuncs/LSCFuncsResolution.hpp"
#include "Compiler/Optimizer/OCLBIUtils.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"
#include "visa_igc_common_header.h"
#include <limits>
#include <string>
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

namespace {
    struct LscTypeInfo {
        LSC_DATA_SIZE dataSize;
        LSC_DATA_ELEMS vectorSize;
        int sizeOfType; // e.g. float4 => sizeof(float4) for D32 V4
    };

    /// @brief  LSCFuncsTranslation pass : tranlate lsc builtin (__builtin_IB_*lsc*) into igc intrinsic.
    ///
    /// This is not automated like the usual builtins because we have to do type
    /// inference and do extra sanity checking here on inputs.
    class LSCFuncsResolution : public FunctionPass, public InstVisitor<LSCFuncsResolution>
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        LSCFuncsResolution();

        /// @brief  Provides name of pass
        virtual StringRef getPassName() const override
        {
            return "LSCFuncsResolution";
        }

        void getAnalysisUsage(AnalysisUsage &AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
        }

        virtual bool runOnFunction(Function &F) override;

        void visitCallInst(CallInst& CI);

    private:
        /// LSC Load intrinsics call method
        Instruction* CreateLSCLoadIntrinsicCallInst(GenISAIntrinsic::ID op, bool isLocalMem);

        /// LSC Store intrinsics call method
        Instruction* CreateLSCStoreIntrinsicCallInst(GenISAIntrinsic::ID op, bool isLocalMem);

        /// LSC Prefetch and load status intrinsics
        Instruction* CreateLSCLoadStatusPreftchIntrinsicCallInst(
            GenISAIntrinsic::ID prefetchOp);

        /// LSC Fence intrinsics call method
        Instruction* CreateLSCFenceIntrinsicCallInst();

        /// LSC Atomic intrinsics call method
        Instruction* CreateLSCAtomicIntrinsicCallInst(bool isLocalMem);

        ///////////////////////////////////////////////////////////////////////
        /// Helpers
        ///////////////////////////////////////////////////////////////////////

        /// Decode the data size and vector size from the function name.
        /// Return true if sucessful; false otherwise.
        ///   Suffix's format:  <DS>_<VS>
        ///   DS - dataSize: uchar,ushort,uint,ulong
        ///   VS - vectorSize: <2|3|4|8|16|32|64>
        ///
        LscTypeInfo decodeTypeInfoFromName();

        /// Decode the SFID from the function name.
        /// Return true if sucessful; false otherwise.
        ///     Suffix's format:  <MP>
        ///     MP - memport: ugm,ugml,tgm,slm
        LSC_SFID decodeSfidFromName();

        /// Decode the atomic op from the function name.
        /// Return true if sucessful; false otherwise.
        ///     Suffix's format:  <AOP>
        ///     AOP - atomic operation: FP64 add, FP64 sub
        AtomicOp decodeAtomicOpFromName();

        /// obnoxious that we can't use std::pair or std::tuple and constexpr
        /// (something about compiler toolchain support made use elminate this
        /// in the past)
        struct SymbolMapping {
            const char *symbol;
            int value;
        };

        /// Searches a table of mappings
        template <typename T,int N>
        bool findFirstInfixMapping(
            StringRef FN, const SymbolMapping enums[N], T &value)
        {
            for (int i = 0; i < N && enums[i].symbol; i++)
                if (FN.find(enums[i].symbol) != StringRef::npos) {
                    value = static_cast<T>(enums[i].value);
                    return true;
                }
            return false;
        }

        //// Gets an i32 with a given value
        Constant *getConstantInt32(int value) {
            Type* i32 = Type::getInt32Ty(m_pCurrInst->getContext());
            return ConstantInt::get(i32, value, true);
        }

        /// E.g. for cache controls, fence options, etc
        Constant *getImmediateEnum(int i, int lo, int hi);

        ///
        /// Fetches and validates the immediate element offset.
        /// Ensures the element offset is immediate and fits in 32b
        // (after scaling by type)
        Constant *getImmediateElementOffset(int ix, LscTypeInfo ti);

        /// Gets an operand as cache control options and sanity checks it.
        /// Atomics have some special constraints.
        Constant *getCacheControlOpts(int i, bool isAtomic = false);

        /// Reports an error in translating the intrinsic
        void reportError(const char *what);

        /// Someone called reportError on the current instruction
        bool hasError() const {
            // ick: tellp is not const
            return m_ErrorMsg.rdbuf() && m_ErrorMsg.rdbuf()->in_avail() > 0;
        }

        /// Indicates if the pass changed the processed function
        bool m_changed;
        bool isHalfSimdMode;

        /// state valid under visitCallInst(...)
        std::stringstream m_ErrorMsg;
        CodeGenContext* m_pCtx;
        CallInst* m_pCurrInst;
        Function* m_pCurrInstFunc;

        static const StringRef PREFIX_LSC_STORE_local;
        static const StringRef PREFIX_LSC_STORE_global;
        static const StringRef PREFIX_LSC_STORE_BLOCK_global;

        static const StringRef PREFIX_LSC_LOAD_local;
        static const StringRef PREFIX_LSC_LOAD_global;
        static const StringRef PREFIX_LSC_LOAD_BLOCK_global;
        static const StringRef PREFIX_LSC_LOAD_status;

        static const StringRef PREFIX_LSC_FENCE;
        static const StringRef PREFIX_LSC_ATOMIC;
        static const StringRef PREFIX_LSC_PREFETCH;
    };
}

char LSCFuncsResolution::ID = 0;

const StringRef LSCFuncsResolution::PREFIX_LSC_STORE_local  = "__builtin_IB_lsc_store_local_";
const StringRef LSCFuncsResolution::PREFIX_LSC_STORE_global = "__builtin_IB_lsc_store_global_";
const StringRef LSCFuncsResolution::PREFIX_LSC_STORE_BLOCK_global = "__builtin_IB_lsc_store_block_global_";

const StringRef LSCFuncsResolution::PREFIX_LSC_LOAD_local  = "__builtin_IB_lsc_load_local_";
const StringRef LSCFuncsResolution::PREFIX_LSC_LOAD_global = "__builtin_IB_lsc_load_global_";
const StringRef LSCFuncsResolution::PREFIX_LSC_LOAD_BLOCK_global = "__builtin_IB_lsc_load_block_global_";
const StringRef LSCFuncsResolution::PREFIX_LSC_LOAD_status = "__builtin_IB_lsc_load_status_global_";

const StringRef LSCFuncsResolution::PREFIX_LSC_FENCE  = "__builtin_IB_lsc_fence_";
const StringRef LSCFuncsResolution::PREFIX_LSC_ATOMIC = "__builtin_IB_lsc_atomic_";
const StringRef LSCFuncsResolution::PREFIX_LSC_PREFETCH = "__builtin_IB_lsc_prefetch_global_";

// Register pass to igc-opt
#define PASS_FLAG "igc-lsc-funcs-translation"
#define PASS_DESCRIPTION "Translate lsc builtin functions into igc intrinsics"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LSCFuncsResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(LSCFuncsResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)



LSCFuncsResolution::LSCFuncsResolution() : FunctionPass(ID)
{
    initializeLSCFuncsResolutionPass(*PassRegistry::getPassRegistry());
}

bool LSCFuncsResolution::runOnFunction(Function &F)
{
    m_pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    int defaultSimdSize = 0;

    switch (m_pCtx->platform.getPlatformInfo().eProductFamily)
    {
    case IGFX_DG2:
        defaultSimdSize = 16;
        break;
    default:
        defaultSimdSize = 32;
        break;
    }

    auto m_pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    auto funcInfoMD = m_pMdUtils->getFunctionsInfoItem(&F);
    int actualSimdSize = funcInfoMD->getSubGroupSize()->getSIMD_size();
    isHalfSimdMode = defaultSimdSize != actualSimdSize; // SIMD8 on DG2, SIMD16 on PVC

    m_changed = false;

    visit(F);

    if (hasError()) {
        m_pCtx->EmitError(m_ErrorMsg.str().c_str(), &F);
        m_ErrorMsg.str(std::string()); // clear stringstream
    }
    return m_changed;
}

void LSCFuncsResolution::visitCallInst(CallInst &CI)
{
    /// Process LCS intrinsics
    m_pCurrInstFunc = CI.getCalledFunction();
    if (!m_pCurrInstFunc)
        return;
    m_pCurrInst = &CI;

    StringRef FN = m_pCurrInstFunc->getName();
    Instruction* lscCall = nullptr;

    //////////////
    // loads
    if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_LOAD_global)) {
        lscCall = CreateLSCLoadIntrinsicCallInst(
            GenISAIntrinsic::GenISA_LSCLoad, false);
    } else if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_LOAD_BLOCK_global)) {
        lscCall = CreateLSCLoadIntrinsicCallInst(
            GenISAIntrinsic::GenISA_LSCLoadBlock, false);
    } else if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_LOAD_local)) {
        lscCall = CreateLSCLoadIntrinsicCallInst(
            GenISAIntrinsic::GenISA_LSCLoad, true);
    //////////////
    // prefetches
    } else if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_LOAD_status)) {
        lscCall = CreateLSCLoadStatusPreftchIntrinsicCallInst(
            GenISAIntrinsic::GenISA_LSCLoadStatus);
    } else if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_PREFETCH)) {
        lscCall = CreateLSCLoadStatusPreftchIntrinsicCallInst(
            GenISAIntrinsic::GenISA_LSCPrefetch);
    //////////////
    // stores
    } else if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_STORE_global)) {
        lscCall = CreateLSCStoreIntrinsicCallInst(
            GenISAIntrinsic::GenISA_LSCStore, false);
    } else if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_STORE_BLOCK_global)) {
        lscCall = CreateLSCStoreIntrinsicCallInst(
            GenISAIntrinsic::GenISA_LSCStoreBlock, false);
    } else if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_STORE_local)) {
        lscCall = CreateLSCStoreIntrinsicCallInst(
            GenISAIntrinsic::GenISA_LSCStore, true);
    //////////////
    // atomics
    } else if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_ATOMIC)) {
        bool isLocalMem = FN.find("_local_") != StringRef::npos;
        lscCall = CreateLSCAtomicIntrinsicCallInst(isLocalMem);
    //////////////
    // misc stuff
    } else if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_FENCE)) {
        // LSC fence
        lscCall = CreateLSCFenceIntrinsicCallInst();
    } else {
        // not an LSC message, bail silently
        return;
    }

    // LSC is not supported/enabled
    if (!m_pCtx->platform.isProductChildOf(IGFX_DG2)) {
        IGC_ASSERT_MESSAGE(0, "LSC not supported on this platform");
        reportError("LSC not supported on this platform");
        return;
    }

    if (lscCall != nullptr) {
        lscCall->setDebugLoc(CI.getDebugLoc());
        CI.replaceAllUsesWith(lscCall);
        CI.eraseFromParent();

        m_changed = true;
    }
}


Instruction* LSCFuncsResolution::CreateLSCLoadIntrinsicCallInst(
    GenISAIntrinsic::ID op, bool isLocalMem)
{
    auto typeInfo = decodeTypeInfoFromName();
    if (hasError()) {
        return nullptr;
    }

    Value* args[5] {
        m_pCurrInst->getArgOperand(0),  // base address
        getImmediateElementOffset(1, typeInfo), // imm element offset
        getConstantInt32(typeInfo.dataSize),   // e.g. D32
        getConstantInt32(typeInfo.vectorSize), // e.g. V4
        isLocalMem ?  // cache options (default value for SLM)
            getConstantInt32(LSC_L1DEF_L3DEF) : getCacheControlOpts(2)
    };

    Type* OvldTys[2] {
        m_pCurrInstFunc->getReturnType(),
        args[0]->getType()
    };
    Function* lscFunc = GenISAIntrinsic::getDeclaration(
        m_pCurrInstFunc->getParent(), op, OvldTys);
    Instruction* lscCall = CallInst::Create(lscFunc, args, "", m_pCurrInst);
    return lscCall;
}


Instruction* LSCFuncsResolution::CreateLSCLoadStatusPreftchIntrinsicCallInst(
    GenISAIntrinsic::ID prefetchOp)
{
    auto typeInfo = decodeTypeInfoFromName();
    if (hasError()) {
        return nullptr;
    }

    // warning this is trusting the user's typing to be correct
    // we end up using args[i]->getType()
    Value* args[5] {
        m_pCurrInst->getArgOperand(0),  // base address
        getImmediateElementOffset(1, typeInfo),  // element offset
        getConstantInt32(typeInfo.dataSize),
        getConstantInt32(typeInfo.vectorSize),
        getCacheControlOpts(2) // cache options
    };

    Type* OvldTys[1] {
        args[0]->getType(), // only one overloaded type
    };
    Function* lscFunc = GenISAIntrinsic::getDeclaration(
        m_pCurrInstFunc->getParent(), prefetchOp, OvldTys);
    Instruction* lscCall = CallInst::Create(lscFunc, args, "", m_pCurrInst);
    if (prefetchOp == GenISAIntrinsic::GenISA_LSCLoadStatus) {
        // the intrinic treats bool as i1, but OCL treats bools as i8
        Type* i8 = Type::getInt8Ty(m_pCurrInst->getContext());
        lscCall =
            BitCastInst::CreateZExtOrBitCast(lscCall, i8, "", m_pCurrInst);
    }
    return lscCall;
}

Instruction* LSCFuncsResolution::CreateLSCStoreIntrinsicCallInst(
    GenISAIntrinsic::ID op, bool isLocalMem)
{
    auto typeInfo = decodeTypeInfoFromName();
    if (hasError()) {
        return nullptr;
    }

    Value* args[6] {
        m_pCurrInst->getArgOperand(0),      // memory address where the data is stored to
        getImmediateElementOffset(1, typeInfo),  // LSC immediate offset
        m_pCurrInst->getArgOperand(2),      // data to store
        getConstantInt32(typeInfo.dataSize),
        getConstantInt32(typeInfo.vectorSize),
        isLocalMem ?  // cache options (must be default for local)
            getConstantInt32(LSC_L1DEF_L3DEF) : getCacheControlOpts(3)
    };

    Type* OvldTys[2] {
        args[0]->getType(), // memory addr
        args[2]->getType(), // data to store
    };

    Function* lscFunc = GenISAIntrinsic::getDeclaration(
        m_pCurrInstFunc->getParent(), op, OvldTys);
    Instruction* lscCall = CallInst::Create(lscFunc, args, "", m_pCurrInst);
    return lscCall;
}


Instruction* LSCFuncsResolution::CreateLSCFenceIntrinsicCallInst() {
    LSC_SFID memPort = decodeSfidFromName();

    auto context = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    if (hasError()) {
        return nullptr;
    }

    Value* args[3] {
        getConstantInt32(memPort), // immediate sfid
        memPort == LSC_SLM ?
            getConstantInt32(LSC_SCOPE_GROUP) : // force SLM to use thread-group scope
            getImmediateEnum(0, LSC_SCOPE_GROUP, LSC_SCOPE_SYSACQ),  // immediate scope of the fence
        memPort == LSC_SLM ||
        (memPort == LSC_TGM &&
         context->platform.getPlatformInfo().eRenderCoreFamily == IGFX_XE_HPC_CORE) ?
            getConstantInt32(LSC_FENCE_OP_NONE) :
            getImmediateEnum(1, LSC_FENCE_OP_NONE, LSC_FENCE_OP_FLUSHL3)   // immediate flush type
    };

    auto scope = dyn_cast<ConstantInt>(args[1]);

    if (scope && (scope->getZExtValue() == LSC_SCOPE_SYSACQ || scope->getZExtValue() == LSC_SCOPE_SYSREL))
    {
        if (!context->platform.supportSystemFence())
        {
            reportError("platform does not support system fence");
        }
    }

    Function *lscFunc = GenISAIntrinsic::getDeclaration(
        m_pCurrInstFunc->getParent(), GenISAIntrinsic::GenISA_LSCFence, None);
    Instruction* lscCall = CallInst::Create(lscFunc, args, "", m_pCurrInst);
    return lscCall;
}

Instruction* LSCFuncsResolution::CreateLSCAtomicIntrinsicCallInst(
    bool isLocalMem)
{
    AtomicOp atomicOp = decodeAtomicOpFromName();
    if (hasError()) {
        return nullptr;
    }

    bool isFP64Atomic =
        atomicOp == EATOMIC_FADD64 || atomicOp == EATOMIC_FSUB64;
    bool isFP32Atomic =
        atomicOp == EATOMIC_FCMPWR ||
        atomicOp == EATOMIC_FADD || atomicOp == EATOMIC_FSUB ||
        atomicOp == EATOMIC_FMIN || atomicOp == EATOMIC_FMAX;
    bool hasSrc1 =
        atomicOp != EATOMIC_INC && atomicOp != EATOMIC_DEC &&
        atomicOp != EATOMIC_LOAD;
    bool hasSrc2 =
        atomicOp == EATOMIC_FCMPWR || atomicOp == EATOMIC_CMPXCHG;

    Type* retTy = m_pCurrInstFunc->getReturnType();

    //
    // For unary and binary atomics some the extra atomic operands need to
    // be set to some default value (we use zero); but we have to carefully
    // pick a value with a type that matches the function overload
    auto getZeroArg =
        [&]() -> Constant * {
            int bitSize = retTy->getScalarSizeInBits();
            if (isFP32Atomic) {
                return ConstantFP::get(
                    Type::getFloatTy(m_pCurrInst->getContext()), 0.0);
            } else if (isFP64Atomic) {
                return ConstantFP::get(
                    Type::getDoubleTy(m_pCurrInst->getContext()), 0.0);
            } else if (bitSize == 64) {
                return ConstantInt::get(
                    Type::getInt64Ty(m_pCurrInst->getContext()), 0, true);
            } else {
                return getConstantInt32(0);
            }
        };
    //
    Value *atomArg1 =
        hasSrc1 ? m_pCurrInst->getArgOperand(2) : getZeroArg();
    //
    Value *atomArg2 =
        hasSrc2 ? m_pCurrInst->getArgOperand(3) : getZeroArg();
    //
    const int ccOpndIx = hasSrc2 ? 4 : hasSrc1 ? 3 : 2;
    Value* args[6] {
        m_pCurrInst->getArgOperand(0), // memory ptr
        m_pCurrInst->getArgOperand(1), // immediate element offset
        atomArg1,                      // value or cmp [cmpxchg] or zero if unused
        atomArg2,                      // value [cmpxchg] or zero if unused
        getConstantInt32(atomicOp),    // atomic op
        isLocalMem ?                   // cache options (default for local)
            getConstantInt32(LSC_L1DEF_L3DEF) : getCacheControlOpts(ccOpndIx)
    };

    GenISAIntrinsic::ID id =
        isFP64Atomic ? GenISAIntrinsic::GenISA_LSCAtomicFP64 :
        isFP32Atomic ? GenISAIntrinsic::GenISA_LSCAtomicFP32 :
        GenISAIntrinsic::GenISA_LSCAtomicInts;

    Function *lscFunc = nullptr;
    if (!isFP32Atomic && !isFP64Atomic) {
        Type* IntTysOvld [4] {
            retTy,              // anyint (return type)
            args[0]->getType(), // anyptr
            retTy,              // [src1] anyint
            retTy,              // [src2] anyint
        };
        lscFunc = GenISAIntrinsic::getDeclaration(
            m_pCurrInstFunc->getParent(), id, IntTysOvld);
    } else {
        Type* FltTysOvld [1] {
            args[0]->getType(), // anyptr
        };
        lscFunc = GenISAIntrinsic::getDeclaration(
            m_pCurrInstFunc->getParent(), id, FltTysOvld);
    }

    Instruction* lscCall = CallInst::Create(lscFunc, args, "", m_pCurrInst);
    return lscCall;
}

LscTypeInfo LSCFuncsResolution::decodeTypeInfoFromName()
{
    StringRef FN = m_pCurrInstFunc->getName();
    LscTypeInfo ti{LSC_DATA_SIZE_8b, LSC_DATA_ELEMS_1, 1};

    // first match:
    //   ..load_{global,local,block_global}_uchar_to_uint(...)
    //   ..store_{global,local,block_global}_uchar_from_uint(...)
    // bail early if we get a hit:
    //  prefetch/load_status will show up as non-conversion types since
    //  they don't return data
    // everything else is suffixed by the type and maybe a vector integer

    if ((FN.endswith("uchar_to_uint")) ||
        (FN.endswith("uchar_from_uint")))
    {
        ti.dataSize = LSC_DATA_SIZE_8c32b;
        ti.sizeOfType = 1;
        return ti;
    }
    else if (
        FN.endswith("ushort_to_uint") ||
        FN.endswith("ushort_from_uint"))
    {
        ti.dataSize = LSC_DATA_SIZE_16c32b;
        ti.sizeOfType = 2;
        return ti;
    }

    // otherwise fall through and try the regular (non-conversion) types
    // returns true if we matched the string (even if error)
    //         false if mismatched
    auto matchTypeAndVector = [&] (
        const char *name,
        LSC_DATA_SIZE dsz,
        int sizeofType)
    {
        // error already reported
        if (hasError())
            return false;

        // Given "__builtin_IB_lsc_load_global_uint2", find "uint2"
        auto typePos = FN.find(name);
        if (typePos == StringRef::npos) {
            return false;
        }

        // data type matches
        ti.dataSize = dsz;
        ti.sizeOfType = sizeofType;

        // "...uchar16" -> "16"
        size_t vecOff = typePos + strlen(name);

        // if the function name suffix exactly matches (no string allocation)
        auto vectorSuffixMatches = [&](const char *pat) {
            if (vecOff + strlen(pat) != FN.size())
                return false; // suffix is not equal length
                              // equal length and prefix ==> exact match
            return FN.find(pat, vecOff) == vecOff;
        };

        // match the suffix exactly, reject garbage like
        // "uint27" (has prefix "uint2")
        if (vectorSuffixMatches("")) {
            ti.vectorSize = LSC_DATA_ELEMS_1;
        } else if (vectorSuffixMatches("2")) {
            ti.vectorSize = LSC_DATA_ELEMS_2;
            ti.sizeOfType *= 2;
        } else if (vectorSuffixMatches("3") || vectorSuffixMatches("4")) {
            if (vectorSuffixMatches("3")) {
                ti.vectorSize = LSC_DATA_ELEMS_3;
                ti.sizeOfType *= 3;
            } else {
                ti.vectorSize = LSC_DATA_ELEMS_4;
                ti.sizeOfType *= 4;
            }
        } else if (vectorSuffixMatches("8")) {
            ti.vectorSize = LSC_DATA_ELEMS_8;
            ti.sizeOfType *= 8;
        } else if (vectorSuffixMatches("16")) {
            ti.vectorSize = LSC_DATA_ELEMS_16;
            ti.sizeOfType *= 16;
            // we only support up to OpenCL vector length 8
            reportError("invalid vector size for data type");
            return true; // bail to avoid later confusing errors
        } else if (vectorSuffixMatches("32")) {
            ti.vectorSize = LSC_DATA_ELEMS_32;
            ti.sizeOfType *= 32;
            //
            // we only support up to OpenCL vector length 8
            reportError("invalid vector size for data type");
            return true; // bail to avoid later confusing errors
        } else if (vectorSuffixMatches("64")) {
            ti.vectorSize = LSC_DATA_ELEMS_64;
            ti.sizeOfType *= 64;
            //
            // we only support up to OpenCL vector length 8
            reportError("invalid vector size for data type");
            return true; // bail to avoid later confusing errors
        } else {
            // totally bogus vector size
            reportError("invalid vector size");
            return true; // bail to avoid later confusing errors
        }

        // Some sanity checking.
        // The legal prototypes provided in the builtin file constrain
        // most mischief, but remember anyone can write a prototype.
        if (ti.dataSize == LSC_DATA_SIZE_8b || ti.dataSize == LSC_DATA_SIZE_16b) {
            bool isPrefetchOrLoadStatus =
                FN.startswith(LSCFuncsResolution::PREFIX_LSC_LOAD_status) ||
                FN.startswith(LSCFuncsResolution::PREFIX_LSC_PREFETCH);
            if (!isPrefetchOrLoadStatus) {
                // D8 and D16 aren't supported yet in normal (non-prefetch)
                // loads and stores
                reportError("8b and 16b not supported");
                return true;
            } else {
                if (ti.vectorSize != LSC_DATA_ELEMS_1) {
                    // because we use widening types to make this work
                    reportError("8b and 16b with vector not supported");
                    return true;
                }
                // use widening message
                // no data will be returned for prefetch and status will
                // broadcast bits of a single DW
                ti.dataSize = ti.dataSize == LSC_DATA_SIZE_8b ?
                    LSC_DATA_SIZE_8c32b : LSC_DATA_SIZE_16c32b;
                ti.sizeOfType = 4;
            }
        }

        // even if errors were reported above, if we get here, it's a match
        // and we'll stop trying other types
        return true;
    };

    // N.b. certain data size and vector type may or may not exist on given
    // platforms, but we rely on the builtin proto-types to police that.
    // (We parse it successfully.)
    if (!matchTypeAndVector("uchar",  LSC_DATA_SIZE_8b, 1) &&
        !matchTypeAndVector("ushort", LSC_DATA_SIZE_16b, 2) &&
        !matchTypeAndVector("uint",   LSC_DATA_SIZE_32b, 4) &&
        !matchTypeAndVector("ulong",  LSC_DATA_SIZE_64b, 8))
    {
        reportError("invalid type for lsc operation");
    }
    return ti;
}

AtomicOp LSCFuncsResolution::decodeAtomicOpFromName()
{
    static const SymbolMapping symbols[42] {
        // FP 64 (local not suported)
        {"_add_global_double", EATOMIC_FADD64},
        {"_sub_global_double", EATOMIC_FSUB64},
        // FP 32
        {"_add_global_float", EATOMIC_FADD},
        {"_add_local_float", EATOMIC_FADD},
        {"_sub_global_float", EATOMIC_FSUB},
        {"_sub_local_float", EATOMIC_FSUB},
        {"_min_global_float", EATOMIC_FMIN},
        {"_min_local_float", EATOMIC_FMIN},
        {"_max_global_float", EATOMIC_FMAX},
        {"_max_local_float", EATOMIC_FMAX},
        {"_cmpxchg_global_float", EATOMIC_FCMPWR},
        {"_cmpxchg_local_float", EATOMIC_FCMPWR},
        /////////////////////////////////////////////////////
        // I16,I32,I64
        {"_add_", EATOMIC_IADD},
        {"_sub_", EATOMIC_SUB},
        // signed min/max
        {"_min_global_short", EATOMIC_MIN},
        {"_min_local_short", EATOMIC_MIN},
        {"_min_global_int", EATOMIC_MIN},
        {"_min_local_int", EATOMIC_MIN},
        {"_min_global_long", EATOMIC_MIN},
        // {"min_local_long", EATOMIC_MIN}, (global only)
        {"_max_global_short", EATOMIC_MAX},
        {"_max_local_short", EATOMIC_MAX},
        {"_max_global_int", EATOMIC_MAX},
        {"_max_local_int", EATOMIC_MAX},
        {"_max_global_long", EATOMIC_MAX},
        // {"max_local_long", EATOMIC_MAX}, (global only)

        // unsigned min/max
        {"_min_global_ushort", EATOMIC_UMIN},
        {"_min_local_ushort", EATOMIC_UMIN},
        {"_min_global_uint", EATOMIC_UMIN},
        {"_min_local_uint", EATOMIC_UMIN},
        {"_min_global_ulong", EATOMIC_UMIN},
        // {"min_local_ulong", EATOMIC_UMIN}, (global only)
        {"_max_global_ushort", EATOMIC_UMAX},
        {"_max_local_ushort", EATOMIC_UMAX},
        {"_max_global_uint", EATOMIC_UMAX},
        {"_max_local_uint", EATOMIC_UMAX},
        {"_max_global_ulong", EATOMIC_UMAX},
        // {"max_local_ulong", EATOMIC_UMAX}, (global only)
        //
        // integer compare and exchange
        {"_cmpxchg_", EATOMIC_CMPXCHG},
        // inc/dec
        {"_inc_", EATOMIC_INC},
        {"_dec_", EATOMIC_DEC},
        //  and/xor/or
        {"_and_", EATOMIC_AND},
        {"_xor_", EATOMIC_XOR},
        {"_or_", EATOMIC_OR},
        // load/store
        {"_load_", EATOMIC_LOAD},
        {"_store_", EATOMIC_STORE},
    };

    // maybe a better way to do this, but the compiler seems to need an
    // explicit size for inference below.
    static_assert(sizeof(symbols)/sizeof(symbols[0]) == 42);

    AtomicOp atomicOp = EATOMIC_IADD;
    StringRef FN = m_pCurrInstFunc->getName();
    if (!findFirstInfixMapping<AtomicOp,42>(FN, symbols, atomicOp)) {
        reportError("invalid lsc atomic operation");
    }
    return atomicOp;
}

LSC_SFID LSCFuncsResolution::decodeSfidFromName()
{
    static const SymbolMapping symbols[4] {
        {"_global_untyped_cross_tile", LSC_UGML},
        {"_global_untyped", LSC_UGM},
        {"_global_typed", LSC_TGM},
        {"_local", LSC_SLM},
    };

    // c.f. reasoning in decodeAtomicOpFromName
    static_assert(sizeof(symbols)/sizeof(symbols[0]) == 4);

    StringRef FN = m_pCurrInstFunc->getName();
    LSC_SFID memPort = LSC_UGM;
    if (!findFirstInfixMapping<LSC_SFID,4>(FN, symbols, memPort)) {
        reportError("invalid lsc SFID");
    }
    return memPort;
}

Constant *LSCFuncsResolution::getImmediateEnum(int i, int lo, int hi)
{
    Value *v = m_pCurrInst->getOperand(i);
    if (ConstantInt *ci = dyn_cast<ConstantInt>(v)) {
        return ci;
    } else {
        std::stringstream ss;
        ss << "operand " << i << " must be immediate";
        reportError(ss.str().c_str());
        return getConstantInt32(lo); // use lo for the error value
    }
}

Constant *LSCFuncsResolution::getImmediateElementOffset(
    int i, LscTypeInfo ti)
{
    Value *v = m_pCurrInst->getOperand(i);
    if (ConstantInt *ci = dyn_cast<ConstantInt>(v)) {
        int64_t scaledValue = ci->getSExtValue() * ti.sizeOfType;
        if (scaledValue < std::numeric_limits<int32_t>::min() ||
            scaledValue > std::numeric_limits<int32_t>::max())
        {
            // The vISA LSC API will emulate large offsets,
            // but is only int width
            reportError("scaled element offset too large");
            return getConstantInt32(0);
        }
        return getConstantInt32((int32_t)scaledValue);
    } else {
        reportError("element offset operand must be immediate");
        return getConstantInt32(0);
    }
}

Constant *LSCFuncsResolution::getCacheControlOpts(int i, bool)
{
    // TODO: error if atomic uses any sort of caching on L1
    return getImmediateEnum(i, LSC_L1DEF_L3DEF, LSC_L1IAR_WB_L3C_WB);
}

void LSCFuncsResolution::reportError(const char *what) {
    if (hasError())
        m_ErrorMsg << "\n";
    const DebugLoc &loc = m_pCurrInst->getDebugLoc();
    if (loc)
        m_ErrorMsg << "line " << loc.getLine() << ": ";
    m_ErrorMsg << m_pCurrInstFunc->getName().str() << ": " << what;
}

FunctionPass* IGC::createLSCFuncsResolutionPass()
{
    return new LSCFuncsResolution();
}
