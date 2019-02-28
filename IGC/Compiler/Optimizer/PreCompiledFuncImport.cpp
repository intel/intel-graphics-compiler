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
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/ScaledNumber.h>
#include "llvm/ADT/SmallSet.h"
#include <llvm/IR/Module.h>
#include <llvmWrapper/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/GenericDomTree.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/IRReader/IRReader.h>
#include "common/LLVMWarningsPop.hpp"

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/Optimizer/PreCompiledFuncImport.hpp"
#include "Compiler/Optimizer/PreCompiledFuncLibrary.cpp"

// No Support to double emulation.
const unsigned char igcbuiltin_emu_dp_add_sub[] = {0};
const unsigned char igcbuiltin_emu_dp_fma_mul[] = {0};
const unsigned char igcbuiltin_emu_dp_cmp[] = {0};
const unsigned char igcbuiltin_emu_dp_conv_i32[] = {0};
const unsigned char igcbuiltin_emu_dp_conv_sp[] = {0};
const unsigned char igcbuiltin_emu_dp_div[] = {0};
const unsigned char igcbuiltin_emu_dp_sqrt[] = {0};
// No Support to IEEE compliant emulation
const unsigned char igcbuiltin_emu_sp_div[] = {0};
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "common/LLVMUtils.h"
#include "AdaptorOCL/OCL/BuiltinResource.h"
#include "AdaptorOCL/OCL/LoadBuffer.h"
#include "AdaptorOCL/Upgrader/Upgrader.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

    // Register pass to igc-opt
#define PASS_FLAG "igc-precompiled-import"
#define PASS_DESCRIPTION "PreCompiledFuncImport"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PreCompiledFuncImport, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(PreCompiledFuncImport, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char PreCompiledFuncImport::ID = 0;

PreCompiledFuncImport::PreCompiledFuncImport(
	CodeGenContext *CGCtx, uint32_t TheEmuKind) :
    ModulePass(ID),
    m_emuKind(TheEmuKind),
	m_pCtx (CGCtx),
	m_enableSubroutineCallForEmulation(false)
{
    initializePreCompiledFuncImportPass(*PassRegistry::getPassRegistry());

    if (isDPEmu() &&
		(IGC_IS_FLAG_ENABLED(EnableSubroutineForEmulation) ||
		 IGC_IS_FLAG_ENABLED(ForceSubroutineForEmulation)))
	{
		checkAndSetEnableSubroutine();
	}
}

const char* PreCompiledFuncImport::m_sFunctionNames[NUM_FUNCTIONS][NUM_TYPES] =
{
    // udiv
    {
        "__precompiled_udiv",
        "__precompiled_udiv2",
        "__precompiled_udiv3",
        "__precompiled_udiv4",
        "__precompiled_udiv8",
        "__precompiled_udiv16"
    },
    // urem
    {
        "__precompiled_umod",
        "__precompiled_umod2",
        "__precompiled_umod3",
        "__precompiled_umod4",
        "__precompiled_umod8",
        "__precompiled_umod16"
    },
    // sdiv
    {
        "__precompiled_sdiv",
        "__precompiled_sdiv2",
        "__precompiled_sdiv3",
        "__precompiled_sdiv4",
        "__precompiled_sdiv8",
        "__precompiled_sdiv16"
    },
    // srem
    {
        "__precompiled_smod",
        "__precompiled_smod2",
        "__precompiled_smod3",
        "__precompiled_smod4",
        "__precompiled_smod8",
        "__precompiled_smod16"
    }
};


const PreCompiledFuncInfo PreCompiledFuncImport::m_functionInfos[NUM_FUNCTION_IDS] =
{
    { "__igcbuiltin_dp_add",        LIBMOD_DP_ADD_SUB },
    { "__igcbuiltin_dp_sub",        LIBMOD_DP_ADD_SUB },
    { "__igcbuiltin_dp_fma",        LIBMOD_DP_FMA_MUL },
    { "__igcbuiltin_dp_mul",        LIBMOD_DP_FMA_MUL },
    { "__igcbuiltin_dp_div",        LIBMOD_DP_DIV },
    { "__igcbuiltin_dp_cmp",        LIBMOD_DP_CMP },
    { "__igcbuiltin_dp_to_int32",   LIBMOD_DP_CONV_I32 },
    { "__igcbuiltin_dp_to_uint32",  LIBMOD_DP_CONV_I32 },
    { "__igcbuiltin_int32_to_dp",   LIBMOD_DP_CONV_I32 },
    { "__igcbuiltin_uint32_to_dp",  LIBMOD_DP_CONV_I32 },
    { "__igcbuiltin_dp_to_sp",      LIBMOD_DP_CONV_SP },  
    { "__igcbuiltin_sp_to_dp",      LIBMOD_DP_CONV_SP },
    { "__igcbuiltin_dp_sqrt",       LIBMOD_DP_SQRT },
	{ "__igcbuiltin_sp_div",        LIBMOD_SP_DIV }
};

const LibraryModuleInfo PreCompiledFuncImport::m_libModInfos[NUM_LIBMODS] =
{
    /* LIBMOD_INT_DIV_REM */   { preCompiledFunctionLibrary, sizeof(preCompiledFunctionLibrary) },
    /* LIBMOD_INT_ADD_SUB */   { igcbuiltin_emu_dp_add_sub, sizeof(igcbuiltin_emu_dp_add_sub) },
    /* LIBMOD_DP_FMA_MUL  */   { igcbuiltin_emu_dp_fma_mul, sizeof(igcbuiltin_emu_dp_fma_mul) },
    /* LIBMOD_DP_DIV      */   { igcbuiltin_emu_dp_div, sizeof(igcbuiltin_emu_dp_div) },
    /* LIBMOD_DP_CMP      */   { igcbuiltin_emu_dp_cmp, sizeof(igcbuiltin_emu_dp_cmp) },
    /* LIBMOD_DP_CONV_I32 */   { igcbuiltin_emu_dp_conv_i32, sizeof(igcbuiltin_emu_dp_conv_i32) },
    /* LIBMOD_DP_CONV_SP  */   { igcbuiltin_emu_dp_conv_sp, sizeof(igcbuiltin_emu_dp_conv_sp) },
    /* LIBMOD_DP_SQRT     */   { igcbuiltin_emu_dp_sqrt, sizeof(igcbuiltin_emu_dp_sqrt) },
	/* LIBMOD_SP_DIV      */   { igcbuiltin_emu_sp_div, sizeof(igcbuiltin_emu_sp_div) }
};

// This function scans intructions before emulation. It converts double-related
// operations (intrinsics, instructions) into ones that can be emulated. It has:
//   1. Intrinsics
//      Replaced some intrinsics of double operands with a known sequence that can be emulated.
//      For example, max() does not have its corresponding emulation function. And it is replaced
//      here with cmp and select. And,
//   2. instructions
//      Convert something like
//         1:   y = sext i32 x to i64; z = sitofp i64 y to double
//              --> z = sitofp i32 x to double
//         2:   y = zext i32 x to i64; z = uitofp i64 y to double
//              --> z = uitofp i32 x to double
//         3:   y = fptoui double x to i64;  z = trunc i64 y to i32
//              --> z = fptoui double to i32           
//              Note that this is just a "WA", which isn't correct in general
//              (It works for OCL so far).
//         Once we have convertion functions b/w double and i64, the "3" shall
//         be removed.
bool PreCompiledFuncImport::preProcessDouble()
{
    CodeGenContext* pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    bool hasNoNaN = pCtx->getModuleMetaData()->compOpt.FiniteMathOnly;
    SmallVector<Instruction *, 8> toBeDeleted;
    for (auto II = m_pModule->begin(), IE = m_pModule->end(); II != IE; ++II)
    {
        Function *Func = &(*II);
        for (inst_iterator i = inst_begin(Func), e = inst_end(Func);
             i != e; ++i)
        {
            Instruction* Inst = &*i;
            if (TruncInst *TI = dyn_cast<TruncInst>(Inst))
            {
                Value* oprd = TI->getOperand(0);
                Type* dstTy = TI->getType();
                uint32_t dstBits = dstTy->getIntegerBitWidth();
                uint32_t srcBits = oprd->getType()->getIntegerBitWidth();
                Instruction* fptoui = dyn_cast<FPToUIInst>(oprd);
                if (fptoui && srcBits == 64 && dstBits <= 32 &&
                    fptoui->getOperand(0)->getType()->isDoubleTy())
                {
                    Instruction* newinst = CastInst::Create(
                        Instruction::FPToUI, fptoui->getOperand(0), dstTy, "", TI);
                    TI->replaceAllUsesWith(newinst);
                    toBeDeleted.push_back(TI);
                }
            }          
            else if (UIToFPInst *UFI = dyn_cast<UIToFPInst>(Inst))
            {
                Value* oprd = UFI->getOperand(0);
                Type* dstTy = UFI->getType();
                uint32_t srcBits = oprd->getType()->getIntegerBitWidth();
                Instruction* zext = dyn_cast<ZExtInst>(oprd);
                if (zext && srcBits == 64 && dstTy->isDoubleTy() &&
                    zext->getOperand(0)->getType()->getIntegerBitWidth() <= 32)
                {
                    Instruction* newinst = CastInst::Create(
                        Instruction::UIToFP, zext->getOperand(0), dstTy, "", UFI);
                    UFI->replaceAllUsesWith(newinst);
                    toBeDeleted.push_back(UFI);
                }
            }
            else if (SIToFPInst *SFI = dyn_cast<SIToFPInst>(Inst))
            {
                Value* oprd = SFI->getOperand(0);
                Type* dstTy = SFI->getType();
                uint32_t srcBits = oprd->getType()->getIntegerBitWidth();
                Instruction* sext = dyn_cast<SExtInst>(oprd);
                if (sext && srcBits == 64 && dstTy->isDoubleTy() &&
                    sext->getOperand(0)->getType()->getIntegerBitWidth() <= 32)
                {
                    Instruction* newinst = CastInst::Create(
                        Instruction::SIToFP, sext->getOperand(0), dstTy, "", SFI);
                    SFI->replaceAllUsesWith(newinst);
                    toBeDeleted.push_back(SFI);
                }
            }
            else if (CallInst *CallI = dyn_cast<CallInst>(Inst))
            {
                Type * resTy = CallI->getType();
                IntrinsicInst *II = dyn_cast<IntrinsicInst>(CallI);
                GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(CallI);
                if (resTy->isDoubleTy() &&
                    ((II && (II->getIntrinsicID() == Intrinsic::maxnum ||
                             II->getIntrinsicID() == Intrinsic::minnum))))
                {
                    // max and min on double operands
                    Value* Oprd0 = CallI->getOperand(0);
                    Value* Oprd1 = CallI->getOperand(1);
                    bool isMax = (II && II->getIntrinsicID() == Intrinsic::maxnum);
                    Instruction* res = nullptr;
                    if (hasNoNaN)
                    {
                        Instruction* cond = FCmpInst::Create(
                            Instruction::FCmp,
                            isMax ? FCmpInst::FCMP_OGE : FCmpInst::FCMP_OLT,
                            Oprd0, Oprd1, "", CallI);
                        cond->setDebugLoc(CallI->getDebugLoc());

                        // Note that select will be splitted in legalization
                        res = SelectInst::Create(cond, Oprd0, Oprd1, "", CallI);
                        res->setDebugLoc(CallI->getDebugLoc());
                    }
                    else
                    {
                        // z = max/min(x,y) sementics:
                        //     If either operand is NaN, return the other one (both are NaN,
                        //     return NaN); otherwise, return normal max/min.
                        // 
                        // Convert it to:
                        //   cond =  x >= y [x < y]  || x&y is unordered   (fcmp_uge|fcmp_ult)
                        //   t = cond ? x : y
                        //   op0_isnan = (x != x)    (fcmp_one)
                        //   res = op0_isnan ? y : t
                        //
                        Instruction* cond = FCmpInst::Create(
                            Instruction::FCmp,
                            isMax ? FCmpInst::FCMP_UGE : FCmpInst::FCMP_ULT,
                            Oprd0, Oprd1, "", CallI);
                        cond->setDebugLoc(CallI->getDebugLoc());
                        Instruction* sel = SelectInst::Create(cond, Oprd0, Oprd1, "", CallI);
                        sel->setDebugLoc(CallI->getDebugLoc());

                        Instruction* isnan = FCmpInst::Create(
                            Instruction::FCmp, FCmpInst::FCMP_ONE, Oprd0, Oprd0, "", CallI);
                        res = SelectInst::Create(isnan, Oprd1, sel, "", CallI);
                        res->setDebugLoc(CallI->getDebugLoc());
                    }
                    CallI->replaceAllUsesWith(res);
                    toBeDeleted.push_back(CallI);
                }
                else  if (resTy->isDoubleTy() &&                  
                          GII && (GII->getIntrinsicID() == GenISAIntrinsic::GenISA_fsat))
                {
                    // y = fsat(x) :  1. y = 0.0 if x is NaN or x < 0.0;
                    //                2. y = 1.0 if x > 1.0;
                    //                3. y = y  otherwise.
                    Value* Oprd0 = CallI->getOperand(0);
                    Constant* FC0 = ConstantFP::get(resTy, 0.0);
                    Constant* FC1 = ConstantFP::get(resTy, 1.0);
                    Instruction* cond = FCmpInst::Create(
                        Instruction::FCmp, 
                        hasNoNaN ? FCmpInst::FCMP_OLT : FCmpInst::FCMP_ULT,
                        Oprd0, FC0, "", CallI);
                    cond->setDebugLoc(CallI->getDebugLoc());
                    Instruction* sel = SelectInst::Create(cond, FC0, Oprd0, "", CallI);
                    sel->setDebugLoc(CallI->getDebugLoc());

                    Instruction* cond1 = FCmpInst::Create(
                        Instruction::FCmp, FCmpInst::FCMP_OGT,
                        Oprd0, FC1, "", CallI);
                    cond1->setDebugLoc(CallI->getDebugLoc());
                    Instruction* res = SelectInst::Create(cond1, FC1, sel, "", CallI);
                    res->setDebugLoc(CallI->getDebugLoc());

                    CallI->replaceAllUsesWith(res);
                    toBeDeleted.push_back(CallI);
                }
            }
        }  
    }

    for(int i=0, e = (int)toBeDeleted.size(); i < e; ++i)
    {
        Instruction* Inst = toBeDeleted[i];
        Inst->eraseFromParent();
    }

    return (toBeDeleted.size() > 0);
}

inline bool isPrecompiledEmulationFunction(Function* func)
{
    return func->getName().contains("precompiled_s32divrem") ||
    func->getName().contains("precompiled_u32divrem") ||
    func->getName().contains("__igcbuiltin_sp_div");
}

bool PreCompiledFuncImport::runOnModule(Module &M)
{
	// sanity check
	if (m_emuKind == 0) {
		// Nothing to emulate
		return false;
	}

	m_pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
	m_pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    m_pModule = &M;
    m_changed = false;

    for (int i=0; i < NUM_LIBMODS; ++i) {
        m_libModuleToBeImported[i] = false;
        m_libModuleAlreadyImported[i] = false;
    }

    SmallSet<Function*, 32> origFunctions;
    for (auto II = M.begin(), IE = M.end(); II != IE; ++II)
    {
        Function *Func = &(*II);
        if (Func->isDeclaration()) {
            continue;
        }
        origFunctions.insert(Func);
    }

    if (isDPEmu() && preProcessDouble())
    {
        m_changed = true;
    }
 
    unsigned int count = 0;
    while (count < 2)
    {
        count++;
        visit(M);

        if (m_changed)
        {
            llvm::Linker ld(M);
            for (int i = 0; i < NUM_LIBMODS; ++i)
            {
                if (!m_libModuleToBeImported[i]) {
                    continue;
                }

                if (m_libModuleAlreadyImported[i])
                    continue;

                const char* pLibraryModule = (const char*)m_libModInfos[i].Mod;
                uint32_t libSize = m_libModInfos[i].ModSize;

                // Load the module we want to compile and link it to existing module
                //StringRef BitRef((char*)preCompiledFunctionLibrary, preCompiledFunctionLibrarySize);
                StringRef BitRef(pLibraryModule, libSize);
                //llvm::Expected<std::unique_ptr<llvm::Module>> ModuleOrErr =
                //    llvm::getLazyBitcodeModule(MemoryBufferRef(BitRef.str(), ""), M.getContext());
                llvm::Expected<std::unique_ptr<llvm::Module>> ModuleOrErr =
                    llvm::parseBitcodeFile(MemoryBufferRef(BitRef.str(), ""), M.getContext());
                if (llvm::Error EC = ModuleOrErr.takeError())
                {
                    assert(0 && "llvm getLazyBitcodeModule - FAILED to parse bitcode");
                }
                std::unique_ptr<llvm::Module> m_pBuiltinModule = std::move(*ModuleOrErr);
                assert(m_pBuiltinModule && "llvm version mismatch - could not load llvm module");

                // Set target triple and datalayout to the original module (emulation func
                // works for both 64 & 32 bit applications).
                m_pBuiltinModule->setDataLayout(M.getDataLayout());
                m_pBuiltinModule->setTargetTriple(M.getTargetTriple());

                // Linking the two modules
                llvm::Linker ld(M);

                if (ld.linkInModule(std::move(m_pBuiltinModule)))
                {
                    assert(0 && "Error linking the two modules");
                }
                m_libModuleAlreadyImported[i] = true;
                m_pBuiltinModule = nullptr;
            }
        }
        for (int i = 0; i < NUM_LIBMODS; ++i) 
        {
            m_libModuleToBeImported[i] = false;
        }
    }

	bool hasNewMDEntry = false;
	FuncNeedIA.clear();
	NewFuncWithIA.clear();


    // Post processing, set those imported functions as internal linkage
    // and alwaysinline.
    for (auto II = M.begin(), IE = M.end(); II != IE; )
    {
        Function* Func = &(*II);
        ++II;
        if (!Func || Func->isDeclaration())
        {
            continue;
        }

        if (!origFunctions.count(Func))
        {
			// Make it internal linkage.
			Func->setLinkage(GlobalValue::InternalLinkage);

			// Need to check if it is dead after setting linkage.
			if (Func->isDefTriviallyDead())
			{
				eraseFunction(&M, Func);
				continue;
			}

            // Remove noinline/AlwaysInline attr if present.
            Func->removeFnAttr(llvm::Attribute::NoInline);
			Func->removeFnAttr(llvm::Attribute::AlwaysInline);

			// Inline all conv functions. And for others, inline
			// only those that have less than 4 uses. If ForceSubroutine
            // is set, do not inline any of them.
			if (m_enableSubroutineCallForEmulation &&
                (IGC_IS_FLAG_ENABLED(ForceSubroutineForEmulation) ||
				 (Func->hasNUsesOrMore(4) && !isDPConvFunc(Func) && !isPrecompiledEmulationFunction(Func))))
			{
				Func->addFnAttr(llvm::Attribute::NoInline);
			}
			else
			{
				// Add AlwaysInline attribute to force inlining all calls.
				Func->addFnAttr(llvm::Attribute::AlwaysInline);
			}

			if (m_enableSubroutineCallForEmulation)
			{
				// Create an entry in metadata for this function and add
				// implicit args for private base if needed (note that the
				// entry func should have private base as implicit arg
				// already. It has been added in PrivateMemoryUsageAnalysis.
				// And emulation functions may need implicit args for private
				// memory only, not any other implicit args). Once implicit
				// args are added, function signature and its calls are changed
				// accordingly.
				addMDFuncEntryForEmulationFunc(Func);
				hasNewMDEntry = true;
			}
        }
    }

	int sz = (int)FuncNeedIA.size();
	if (sz > 0)
	{
		createFuncWithIA();
		for (int i = 0; i < sz; ++i) {
			replaceFunc(FuncNeedIA[i], NewFuncWithIA[i]);
		}

		// free FuncsImpArgs
		for (auto I : FuncsImpArgs) {
			ImplicitArgs *IA = I.second;
			delete IA;
		}
		FuncsImpArgs.clear();
	}

	if (hasNewMDEntry)
		m_pMdUtils->save(M.getContext());

#if 0
    {
        CodeGenContext* pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
        DumpLLVMIR(pCtx, "PrecompiledLib_after");
    }
#endif

    return m_changed;
}

void PreCompiledFuncImport::visitBinaryOperator(BinaryOperator &I)
{
    if (I.getOperand(0)->getType()->isIntOrIntVectorTy())
    {
        unsigned int integerBitWidth = I.getOperand(0)->getType()->getScalarType()->getIntegerBitWidth();

        if (integerBitWidth == 64)
        {
            if (isI64DivRem())
            {
                switch (I.getOpcode())
                {
                case Instruction::UDiv:
                    processDivide(I, FUNCTION_UDIV);
                    break;
                case Instruction::URem:
                    processDivide(I, FUNCTION_UREM);
                    break;
                case Instruction::SDiv:
                    processDivide(I, FUNCTION_SDIV);
                    break;
                case Instruction::SRem:
                    processDivide(I, FUNCTION_SREM);
                    break;
                default:
                    // nothing
                    break;
                };
            }
        }
    }
    else if (isDPEmu() && I.getOperand(0)->getType()->isDoubleTy())
    {
        switch (I.getOpcode())
        {
        case Instruction::FMul:
            processFPBinaryOperator(I, FUNCTION_DP_MUL);
            break;
        case Instruction::FAdd:
            processFPBinaryOperator(I, FUNCTION_DP_ADD);
            break;
        case Instruction::FSub:
            processFPBinaryOperator(I, FUNCTION_DP_SUB);
            break;
        case Instruction::FDiv:
            processFPBinaryOperator(I, FUNCTION_DP_DIV);
            break;

        default:
            // nothing
            break;
        };
    }
}


//64 bit emulation for divide 
void PreCompiledFuncImport::processDivide(BinaryOperator &inst, EmulatedFunctions function)
{
    unsigned int numElements = 1;
    unsigned int elementIndex = 0;

    Type* argumentType = inst.getOperand(0)->getType();

    if (argumentType->isVectorTy())
    {
        numElements = argumentType->getVectorNumElements();
    }

    switch (numElements)
    {
    case 1:
        elementIndex = 0;
        break;
    case 2:
        elementIndex = 1;
        break;
    case 3:
        elementIndex = 2;
        break;
    case 4:
        elementIndex = 3;
        break;
    case 8:
        elementIndex = 4;
        break;
    case 16:
        elementIndex = 5;
        break;
    default:
        assert(0 && "Unexpected vector size");
        return;
    }

    StringRef funcName = m_sFunctionNames[function][elementIndex];

    Function* func = m_pModule->getFunction(funcName);

    // Try to look up the function in the module's symbol
    // table first, else add it.
    if (func == NULL)
    {
        Type* types[2];

        types[0] = inst.getOperand(0)->getType();
        types[1] = inst.getOperand(1)->getType();

        FunctionType* FuncIntrType = FunctionType::get(
            inst.getType(),
            types,
            false);

        func = Function::Create(
            FuncIntrType,
            GlobalValue::ExternalLinkage,
            funcName,
            m_pModule);
    }

    Value* args[2];

    args[0] = inst.getOperand(0);
    args[1] = inst.getOperand(1);

    CallInst *funcCall = CallInst::Create(func, args, inst.getName(), &inst);
    funcCall->setDebugLoc(inst.getDebugLoc());

    inst.replaceAllUsesWith(funcCall);
    inst.eraseFromParent();

    m_libModuleToBeImported[LIBMOD_INT_DIV_REM] = true;
    m_changed = true;
}

void PreCompiledFuncImport::visitFPTruncInst(llvm::FPTruncInst &inst)
{
    if ((isI64DivRem() || isDPEmu()) &&
		inst.getDestTy()->isHalfTy() && inst.getSrcTy()->isDoubleTy())
    {
        if (inst.getDestTy()->isVectorTy())
        {
            assert(0 && "Unexpected vector size");
            return;
        }

        const StringRef funcName = "__precompiled_convert_f64_to_f16";
        Function* func = m_pModule->getFunction(funcName);

        // Try to look up the function in the module's symbol
        // table first, else add it.
        if (func == NULL)
        {
            FunctionType* FuncIntrType = FunctionType::get(
                inst.getDestTy(),
                inst.getSrcTy(),
                false);

            func = Function::Create(
                FuncIntrType,
                GlobalValue::ExternalLinkage,
                funcName,
                m_pModule);
        }

        CallInst* funcCall = CallInst::Create(func, inst.getOperand(0), inst.getName(), &inst);
        funcCall->setDebugLoc(inst.getDebugLoc());

        inst.replaceAllUsesWith(funcCall);
        inst.eraseFromParent();

        m_libModuleToBeImported[LIBMOD_INT_DIV_REM] = true;
        m_changed = true;
        return;
    }

    if (isDPEmu() && inst.getDestTy()->isFloatTy() && inst.getSrcTy()->isDoubleTy())
    {
        Function *newFunc = getOrCreateFunction(FUNCTION_DP_TO_SP);
        Value* args[5];

        Type* intTy = Type::getInt32Ty(m_pModule->getContext());
        Function *CurrFunc = inst.getParent()->getParent();
        args[0] = inst.getOperand(0);
        args[1] = ConstantInt::get(intTy, 0);  // RN
        args[2] = ConstantInt::get(intTy, 1);  // flush to zero
        args[3] = args[2];                     // flush denorm
        args[4] = createFlagValue(CurrFunc);   // ignored

        CallInst *funcCall = CallInst::Create(newFunc, args, inst.getName(), &inst);
        funcCall->setDebugLoc(inst.getDebugLoc());

        inst.replaceAllUsesWith(funcCall);
        inst.eraseFromParent();

        m_changed = true;
        return;
    }
}


// Common code to process fadd/fsub/fmul/fdiv whose emu function prototype is
//   double (double, double, int, int, int, int*);
void PreCompiledFuncImport::processFPBinaryOperator(Instruction& I, FunctionIDs FID)
{
    Function *newFunc = getOrCreateFunction(FID);
    Value* args[6];

    Type* intTy = Type::getInt32Ty(m_pModule->getContext());
    Function *CurrFunc = I.getParent()->getParent();
    args[0] = I.getOperand(0);
    args[1] = I.getOperand(1);
    args[2] = ConstantInt::get(intTy, 0);  // RN
    args[3] = ConstantInt::get(intTy, 1);  // flush to zero
    args[4] = args[3];                     // flush denorm
    args[5] = createFlagValue(CurrFunc);   // FP flag, ignored

    CallInst *funcCall = CallInst::Create(newFunc, args, I.getName(), &I);
    funcCall->setDebugLoc(I.getDebugLoc());

    I.replaceAllUsesWith(funcCall);
    I.eraseFromParent();

    m_changed = true;
}

Function* PreCompiledFuncImport::getOrCreateFunction(FunctionIDs FID)
{
    // Common arguments for DP emulation functions
    //
    // Rounding Modes
    //   #define RN 0
    //   #define RD 1
    //   #define RU 2
    //   #define RZ 3
    //
    // Return value for CMP function
    //   #define CMP_EQ 0
    //   #define CMP_LT 1
    //   #define CMP_GT 2
    //   #define CMP_UNORD 3
    //
    // ftz
    //   0            : not "flush to zero"
    //   1 (non-zero) : flush to zero   (non-zero)
    //
    // daz
    //   0            : retain denorm
    //   1 (non-zero) : denorm as zero.
    //
    // flag bits
    //   #define INEXACT   0x20
    //   #define UNDERFLOW 0x10
    //   #define OVERFLOW  0x8
    //   #define ZERODIV   0x4
    //   #define DENORMAL  0x2
    //   #define INVALID   0x1
    //

    const PreCompiledFuncInfo& finfo = m_functionInfos[FID];
    StringRef funcName = finfo.FuncName;

    Function* newFunc = m_pModule->getFunction(funcName);
    if (newFunc) {
        // Already created, just return it.
        return newFunc;
    }

    SmallVector<Type*, 8> argTypes;
    Type* intTy = Type::getInt32Ty(m_pModule->getContext());
    Type* dpTy = Type::getDoubleTy(m_pModule->getContext());
    Type* spTy = Type::getFloatTy(m_pModule->getContext());
    Type* intPtrTy = intTy->getPointerTo(ADDRESS_SPACE_PRIVATE);
    Type* retTy;
    switch (FID)
    {
    case FUNCTION_DP_FMA:
        // double dp_fma (double xin, double yin, double zin,
        //                int rmode, int ftz, int daz,
        //                int* pflags);
        argTypes.push_back(dpTy);
        // fall-through
    case FUNCTION_DP_MUL:
    case FUNCTION_DP_ADD:
    case FUNCTION_DP_SUB:
    case FUNCTION_DP_DIV:
        // double dp_add/sub/mul/div (
        //     double xin, double yin,
        //     int rmode, int ftz, int daz,
        //     int* pflags)
        argTypes.push_back(dpTy);
        argTypes.push_back(dpTy);
        argTypes.push_back(intTy);
        argTypes.push_back(intTy);
        argTypes.push_back(intTy);
        argTypes.push_back(intPtrTy);

        retTy = dpTy;
        break;

    case FUNCTION_DP_CMP:
        // int dp_cmp (double xin, double yin, int daz)
        argTypes.push_back(dpTy);
        argTypes.push_back(dpTy);
        argTypes.push_back(intTy);

        retTy = intTy;
        break;

    case FUNCTION_DP_TO_I32:
    case FUNCTION_DP_TO_UI32:
        // [u]int32 dp_to_[u]int32
        //     (double xin, int rmode, int daz, int* pflags)
        argTypes.push_back(dpTy);
        argTypes.push_back(intTy);
        argTypes.push_back(intTy);
        argTypes.push_back(intPtrTy);

        retTy = intTy;
        break;

    case FUNCTION_I32_TO_DP:
    case FUNCTION_UI32_TO_DP:
        // double int32_to_dp (int32 in)
        // double uint32_to_dp (uint32 in)
        argTypes.push_back(intTy);

        retTy = dpTy;
        break;

    case FUNCTION_DP_TO_SP:
        // float dp_to_sp(double xin, int rmode, int ftz, int daz, int* pflags)
        argTypes.push_back(dpTy);
        argTypes.push_back(intTy);
        argTypes.push_back(intTy);
        argTypes.push_back(intTy);
        argTypes.push_back(intPtrTy);

        retTy = spTy;
        break;

    case FUNCTION_SP_TO_DP:
        // double sp_to_dp (float xin, int daz, int* pflags)
        argTypes.push_back(spTy);
        argTypes.push_back(intTy);
        argTypes.push_back(intPtrTy);

        retTy = dpTy;
        break;

    case FUNCTION_DP_SQRT:
        // double dp_sqrt(double xin, int rmode, int ftz, int daz, int* pflags)
        argTypes.push_back(dpTy);
        argTypes.push_back(intTy);
        argTypes.push_back(intTy);
        argTypes.push_back(intTy);
        argTypes.push_back(intPtrTy);

        retTy = dpTy;
        break;

	case FUNCTION_SP_DIV:
		argTypes.push_back(spTy);
		argTypes.push_back(spTy);
        argTypes.push_back(intTy);
        argTypes.push_back(intTy);
		retTy = spTy;
		break;

    default:
        llvm_unreachable("Undefined FunctionIDs");
    }

    FunctionType* funcType = FunctionType::get(retTy, argTypes, false);
    newFunc = Function::Create(
        funcType,
        GlobalValue::ExternalLinkage,
        funcName,
        m_pModule);

    // keep track of what libraries will be imported.
    m_libModuleToBeImported[finfo.LibModID] = true;
    return newFunc;
}

// Alloca an int and return address Value to that.
Value* PreCompiledFuncImport::createFlagValue(Function *F)
{
    LLVMContext& Ctx = F->getContext();
    BasicBlock *EntryBB = &(F->getEntryBlock());
    Instruction* insert_before = &(*EntryBB->getFirstInsertionPt());
    Type *intTy = Type::getInt32Ty(Ctx);
    Value *flagPtrValue = new AllocaInst(intTy, 0, "DPEmuFlag", insert_before);
    return flagPtrValue;
}

void PreCompiledFuncImport::visitFPExtInst(llvm::FPExtInst& I)
{
    if (isDPEmu() && I.getDestTy()->isDoubleTy() &&
        (I.getSrcTy()->isFloatTy() || I.getSrcTy()->isHalfTy()))
    {
        Function *newFunc = getOrCreateFunction(FUNCTION_SP_TO_DP);
        Type* intTy = Type::getInt32Ty(m_pModule->getContext());
        Function *CurrFunc = I.getParent()->getParent();
        Value* args[3];
        if (I.getSrcTy()->isHalfTy())
        {
            Instruction *newI = new FPExtInst(
                I.getOperand(0),
                Type::getFloatTy(m_pModule->getContext()),
                "DPEmufp16tofp32",
                &I);
            newI->setDebugLoc(I.getDebugLoc());
            args[0] = newI;
        }
        else
        {
            args[0] = I.getOperand(0);
        }
        args[1] = ConstantInt::get(intTy, 1);  // flush denorm
        args[2] = createFlagValue(CurrFunc);   // FP flags, ignored
        CallInst *funcCall = CallInst::Create(newFunc, args, I.getName(), &I);
        funcCall->setDebugLoc(I.getDebugLoc());

        I.replaceAllUsesWith(funcCall);
        I.eraseFromParent();

        m_changed = true;
    }
}

void PreCompiledFuncImport::visitCastInst(llvm::CastInst& I)
{
    if (!isDPEmu()) {
        return;
    }

    // Do not expect vector type for emulation yet.
    if (I.getType()->isVectorTy() || I.getOperand(0)->getType()->isVectorTy())
    {
        return;
    }

    FunctionIDs FID;
    Type* intTy = Type::getInt32Ty(m_pModule->getContext());
    Type* dstTy = I.getType();
    Type* srcTy = I.getOperand(0)->getType();
    uint32_t opc = I.getOpcode();
    uint32_t intBits;
    Value* oprd0 = I.getOperand(0);
    switch (opc)
    {
    case Instruction::FPToSI:
    case Instruction::FPToUI:
        intBits = dstTy->getIntegerBitWidth();
        if (!srcTy->isDoubleTy() || intBits > 32) {
            return;
        }

        FID = ((opc == Instruction::FPToSI) ? FUNCTION_DP_TO_I32
            : FUNCTION_DP_TO_UI32);
        break;

    case Instruction::SIToFP:
    case Instruction::UIToFP:
        intBits = srcTy->getIntegerBitWidth();
        if (!dstTy->isDoubleTy() || intBits > 32) {
            return;
        }
        if (intBits < 32)
        {
            Instruction* newInst;
            if (opc == Instruction::SIToFP)
            {
                newInst = new SExtInst(oprd0, intTy, "DPEmusext", &I);
            }
            else
            {
                newInst = new ZExtInst(oprd0, intTy, "DPEmuzext", &I);
            }
            newInst->setDebugLoc(I.getDebugLoc());
            oprd0 = newInst;
        }

        FID = ((opc == Instruction::SIToFP) ? FUNCTION_I32_TO_DP
            : FUNCTION_UI32_TO_DP);
        break;

    default:
        return;
    }

    Function *newFunc = getOrCreateFunction(FID);
    Function *CurrFunc = I.getParent()->getParent();
    SmallVector<Value*, 4> args;
    args.push_back(oprd0);
    if (opc == Instruction::FPToSI || opc == Instruction::FPToUI)
    {
        Constant* COne = ConstantInt::get(intTy, 1);
		Constant* RZ = ConstantInt::get(intTy, 3);
        args.push_back(RZ);     // round mode = RZ
        args.push_back(COne);   // flush denorm
        args.push_back(createFlagValue(CurrFunc));  // FP flags, ignored
    }

    Instruction* newVal = CallInst::Create(newFunc, args, I.getName(), &I);
    newVal->setDebugLoc(I.getDebugLoc());

    if ((intBits < 32) &&
        (opc == Instruction::FPToSI || opc == Instruction::FPToUI))
    {
        newVal = new TruncInst(newVal, I.getType(), "DPEmuTrunc", &I);
        newVal->setDebugLoc(I.getDebugLoc());
    }

    I.replaceAllUsesWith(newVal);
    I.eraseFromParent();

    m_changed = true;
    return;
}

uint32_t PreCompiledFuncImport::getFCmpMask(CmpInst::Predicate Pred)
{
    // Return value from dp_cmp
    enum {
        DP_EMU_CMP_EQ = 0,     // bit 0 in mask
        DP_EMU_CMP_LT = 1,     // bit 1 in mask
        DP_EMU_CMP_GT = 2,     // bit 2 in mask
        DP_EMU_CMP_UNORD = 3   // bit 3 in mask
    };
#define SETMASKBIT(n)  ( 0x1 << (n))

    uint32_t mask = 0;
    switch (Pred) {
    case CmpInst::FCMP_OEQ :
    case CmpInst::FCMP_UEQ :
        mask = SETMASKBIT(DP_EMU_CMP_EQ);
        break;
    case CmpInst::FCMP_OLT :
    case CmpInst::FCMP_ULT :
        mask = SETMASKBIT(DP_EMU_CMP_LT);
        break;

    case CmpInst::FCMP_OLE :
    case CmpInst::FCMP_ULE :
        mask = (SETMASKBIT(DP_EMU_CMP_EQ) | SETMASKBIT(DP_EMU_CMP_LT));
        break;

    case CmpInst::FCMP_OGT :
    case CmpInst::FCMP_UGT :
        mask = SETMASKBIT(DP_EMU_CMP_GT);
        break;

    case CmpInst::FCMP_OGE :
    case CmpInst::FCMP_UGE :
        mask = (SETMASKBIT(DP_EMU_CMP_EQ) | SETMASKBIT(DP_EMU_CMP_GT));
        break;

    case CmpInst::FCMP_ONE :
    case CmpInst::FCMP_UNE :
        mask = (SETMASKBIT(DP_EMU_CMP_LT) | SETMASKBIT(DP_EMU_CMP_GT));
        break;

    case CmpInst::FCMP_ORD:
        mask |= (SETMASKBIT(DP_EMU_CMP_EQ) | SETMASKBIT(DP_EMU_CMP_GT) |
            SETMASKBIT(DP_EMU_CMP_LT));
        break;

    case CmpInst::FCMP_UNO :
        //  All Unordered is set later.
        break;

    default:
        llvm_unreachable("Wrong fcmp flag");
    }

    switch (Pred) {
    case CmpInst::FCMP_UEQ:
    case CmpInst::FCMP_ULT:
    case CmpInst::FCMP_ULE:
    case CmpInst::FCMP_UGT:
    case CmpInst::FCMP_UGE:
    case CmpInst::FCMP_UNE:
    case CmpInst::FCMP_UNO:
        mask |= SETMASKBIT(DP_EMU_CMP_UNORD);
        break;
    default:
        break;
    }
    return mask;
}

void PreCompiledFuncImport::visitFCmpInst(FCmpInst& I)
{
    if (!isDPEmu() || !I.getOperand(0)->getType()->isDoubleTy()) {
        return;
    }

    CmpInst::Predicate pred = I.getPredicate();
    if (pred == CmpInst::FCMP_FALSE || pred == CmpInst::FCMP_TRUE)
    {
        return;
    }

    Function *newFunc = getOrCreateFunction(FUNCTION_DP_CMP);
    Type* intTy = Type::getInt32Ty(m_pModule->getContext());
    Value* args[3];
    args[0] = I.getOperand(0);
    args[1] = I.getOperand(1);
    args[2] = ConstantInt::get(intTy, 1);  // flush denorm.

    CallInst *funcCall = CallInst::Create(newFunc, args, I.getName(), &I);
    funcCall->setDebugLoc(I.getDebugLoc());

    //
    // 'mask' indicates that if any of bits is set, the condition is true.
    // 'bitVal' is the bit that is set by this emulation function, which is
    // calculated by  1 << funcCall.
    //   Given the fcmp 
    //       i1 cond = fcmp (Pred) x,  y
    //   it is translated into
    //       c1 = dp_cmp(x, y, 1)
    //       c2 = 1 << c1;
    //       if ( (c2 & mask(Pred)) != 0) cond = 1 else cond = 0;
    //
    uint32_t mask = getFCmpMask(pred);
    Constant* maskVal = ConstantInt::get(intTy, mask);
    Constant* COne = ConstantInt::get(intTy, 1);
    Constant* CZero = ConstantInt::get(intTy, 0);
    Instruction* bitVal =
        BinaryOperator::CreateShl(COne, funcCall, "", &I);
    Instruction* anyBitSet =
        BinaryOperator::CreateAnd(maskVal, bitVal, "", &I);
    Instruction* intcmp = CmpInst::Create(
        Instruction::ICmp, CmpInst::ICMP_NE, anyBitSet, CZero, "DPEmuCmp", &I);

    I.replaceAllUsesWith(intcmp);
    I.eraseFromParent();

    m_changed = true;
}

void PreCompiledFuncImport::visitCallInst(llvm::CallInst& I)
{
    if (IGC_IS_FLAG_ENABLED(EnableTestIGCBuiltin))
    {
        // This is to test if an emulated function is the same
        // as the hardware instruction. It requires the platform
        // that has hw instructions, such ask SKL etc.
        // Currently, it is available for OCL only.  For example,
        // the following ocl kernel will test if double-to-int32
        // is the same as its emulated function.
        //
        // extern int __igcbuildin_dp_to_int32(double, int, int, int*);
        // 
        // void kernel test_db2si32(global int* dst, global double *src)
        // {
        //    int flag = 0;
        //    int ix = get_global_id(0);
        //    double din = src[ix];
        //    int hwRes = (int)din;
        //    int emuRes = __igcbuildin_dp_to_int32(din, 0, 0, &flag);
        //    int r0 = 0, r1 = 0;
        //    if (hwRes != emuRes)
        //    {
        //        r0 = hwRes;
        //        r1 = emuRes;
        //    }
        //    dst[2 * ix] = r0;
        //    dst[2 * ix + 1] = r1;
        // }
        // 
        // With the host application invoking this kernel, it will check if the emulated
        // function matches the hardware instruction.
        // 
        // Note that when using this functionality, ForceDPEmulation must be 0 AND it must
        // be on the platform that supports the hardware instrutions for those emulated
        // operations.
        // 
        Function* func = I.getCalledFunction();
        if (func)
        {
            StringRef fName = func->getName();
            for (int FID = 0; FID < NUM_FUNCTION_IDS; ++FID)
            {
                const PreCompiledFuncInfo& finfo = m_functionInfos[FID];
                if (fName.equals(finfo.FuncName))
                {
                    m_libModuleToBeImported[finfo.LibModID] = true;
                    m_changed = true;

                    // Make sure to use the default calling Convention
                    // as emulation functions uses the default!
                    I.setCallingConv(0);
                    break;
                }
            }
        }
    }

    Type * resTy = I.getType();
    Type* intTy = Type::getInt32Ty(m_pModule->getContext());
    IntrinsicInst *II = dyn_cast<IntrinsicInst>(&I);
    GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(&I);
	if (isSPDiv() && resTy->isFloatTy() &&
		GII && GII->getIntrinsicID() == GenISAIntrinsic::GenISA_IEEE_Divide)
	{
		Function *newFunc = getOrCreateFunction(FUNCTION_SP_DIV);
		Value* args[4];
		args[0] = I.getOperand(0);
		args[1] = I.getOperand(1);

        auto pMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
        int ftz = (m_pCtx->m_floatDenormMode32 == FLOAT_DENORM_FLUSH_TO_ZERO) ? 1 : 0;
        int daz = (pMD->compOpt.DenormsAreZero) ? 1 : 0;
        args[2] = ConstantInt::get(Type::getInt32Ty(I.getContext()), ftz);
        args[3] = ConstantInt::get(Type::getInt32Ty(I.getContext()), daz);

		Instruction* newVal = CallInst::Create(newFunc, args, I.getName(), &I);
		newVal->setDebugLoc(I.getDebugLoc());

		I.replaceAllUsesWith(newVal);
		I.eraseFromParent();

		m_changed = true;
		return;
	}

	if (!isDPEmu()) {
		return;
	}

    if (resTy->isDoubleTy() &&
        (II && II->getIntrinsicID() == Intrinsic::sqrt))
    {
        Function *newFunc = getOrCreateFunction(FUNCTION_DP_SQRT);
        Function *CurrFunc = I.getParent()->getParent();
        Value* args[5];
        args[0] = I.getOperand(0);
        args[1] = ConstantInt::get(intTy, 0); // RN
        args[2] = ConstantInt::get(intTy, 1); // flush to zero
        args[3] = args[2];                    // flush denorm
        args[4] = createFlagValue(CurrFunc);  // FP Flag, ignored

        Instruction* newVal = CallInst::Create(newFunc, args, I.getName(), &I);
        newVal->setDebugLoc(I.getDebugLoc());

        I.replaceAllUsesWith(newVal);
        I.eraseFromParent();

        m_changed = true;
        return;
    }

    // llvm.fma.f64
    if (resTy->isDoubleTy() && II && II->getIntrinsicID() == Intrinsic::fma)
    {
        Function *newFunc = getOrCreateFunction(FUNCTION_DP_FMA);
        Function *CurrFunc = I.getParent()->getParent();
        Value* args[7];
        args[0] = I.getOperand(0);
        args[1] = I.getOperand(1);
        args[2] = I.getOperand(2);
        args[3] = ConstantInt::get(intTy, 0); // RN
        args[4] = ConstantInt::get(intTy, 1); // flush to zero
        args[5] = args[4];                    // flush denorm
        args[6] = createFlagValue(CurrFunc);  // FP Flag, ignored

        Instruction* newVal = CallInst::Create(newFunc, args, I.getName(), &I);
        newVal->setDebugLoc(I.getDebugLoc());

        I.replaceAllUsesWith(newVal);
        I.eraseFromParent();

        m_changed = true;
        return;
    }

    // llvm.fabs.f64
    if (resTy->isDoubleTy() && II && II->getIntrinsicID() == Intrinsic::fabs)
    {
        // bit 63 is sign bit, set it to zero. Don't use int64.
        VectorType* vec2Ty = VectorType::get(intTy, 2);
        Instruction* twoI32 =  CastInst::Create(
            Instruction::BitCast, I.getOperand(0), vec2Ty, "", &I);
        twoI32->setDebugLoc(I.getDebugLoc());
        Instruction* topI32 = ExtractElementInst::Create(
            twoI32,  ConstantInt::get(intTy, 1), "", &I);
        topI32->setDebugLoc(I.getDebugLoc());
        Instruction* newTopI32 = BinaryOperator::CreateAnd(
            topI32, ConstantInt::get(intTy, 0x7fffffff), "", &I);
        newTopI32->setDebugLoc(I.getDebugLoc());
        Instruction* val = InsertElementInst::Create(
            twoI32, newTopI32, ConstantInt::get(intTy, 1), "", &I);
        val->setDebugLoc(I.getDebugLoc());
        Instruction* fabsVal = CastInst::Create(
            Instruction::BitCast, val, resTy, "DPEmuFabs", &I);
        fabsVal->setDebugLoc(I.getDebugLoc());
 
        I.replaceAllUsesWith(fabsVal);
        I.eraseFromParent();

        m_changed = true;
        return;
    }
    return;
}

bool PreCompiledFuncImport::isDPConvFunc(Function *F) const
{
	StringRef FN = F->getName();
	for (int i = 0; i < NUM_FUNCTION_IDS; ++i) {
		if (FN.equals(m_functionInfos[i].FuncName))
		{
			FunctionIDs FID = (FunctionIDs)i;
			switch (FID)
			{
			default:
				break;
			case FUNCTION_DP_TO_I32:
			case FUNCTION_DP_TO_UI32:
			case FUNCTION_I32_TO_DP:
			case FUNCTION_UI32_TO_DP:
			case FUNCTION_DP_TO_SP:
			case FUNCTION_SP_TO_DP:
				return true;
			}
			break;
		}
	}
	return false;
}

bool PreCompiledFuncImport::usePrivateMemory(Function *F)
{
	// Assume alloca is in entry BB
	BasicBlock *entryBB = &F->getEntryBlock();
	for (auto II = entryBB->begin(), IE = entryBB->end(); II != IE; ++II)
	{
		Instruction *I = &*II;
		if (isa<AllocaInst>(I))
			return true;
	}
	return false;
}

void PreCompiledFuncImport::addMDFuncEntryForEmulationFunc(Function *F)
{
    FunctionInfoMetaDataHandle FH = FunctionInfoMetaDataHandle(FunctionInfoMetaData::get());
	FH->setType(FunctionTypeMD::UserFunction);
	for (auto arg = F->arg_begin(); arg != F->arg_end(); ++arg)
	{
		ArgInfoMetaDataHandle AH = ArgInfoMetaDataHandle(ArgInfoMetaData::get());
		AH->setExplicitArgNum(arg->getArgNo());
		FH->addArgInfoListItem(AH);
	}
	m_pMdUtils->setFunctionsInfoItem(F, FH);

	if (!usePrivateMemory(F))
		return;

	// Add private base
	ArgInfoMetaDataHandle arg_R0 = ArgInfoMetaDataHandle(ArgInfoMetaData::get());
	arg_R0->setArgId(ImplicitArg::R0);
	FH->addImplicitArgInfoListItem(arg_R0);
	ArgInfoMetaDataHandle arg_PBase = ArgInfoMetaDataHandle(ArgInfoMetaData::get());
	arg_PBase->setArgId(ImplicitArg::PRIVATE_BASE);
	FH->addImplicitArgInfoListItem(arg_PBase);

	FuncNeedIA.push_back(F);
}

FunctionType* PreCompiledFuncImport::getNewFuncType(
	Function* pFunc, const ImplicitArgs* pImplicitArgs)
{
	// Add all explicit parameters
	FunctionType* pFuncType = pFunc->getFunctionType();
	std::vector<Type *> newParamTypes(pFuncType->param_begin(), pFuncType->param_end());

	// Add implicit arguments parameter types
	for (unsigned int i = 0; i < pImplicitArgs->size(); ++i)
	{
		newParamTypes.push_back((*pImplicitArgs)[i].getLLVMType(pFunc->getContext()));
	}

	// Create new function type with explicit and implicit parameter types
	return FunctionType::get(pFunc->getReturnType(), newParamTypes, pFunc->isVarArg());
}

// Once the new function is created, the old function's metadata is
// re-assigned to the new one. And thus, the old function has no
// metadata entry anymore.
void PreCompiledFuncImport::createFuncWithIA()
{
	NewFuncWithIA.clear();
	for (int i = 0, sz = (int)FuncNeedIA.size(); i < sz; ++i)
	{
		Function* pFunc = FuncNeedIA[i];
		ImplicitArgs implicitArgs(*pFunc, m_pMdUtils);
		FunctionType *pNewFTy = getNewFuncType(pFunc, &implicitArgs);
		Function* pNewFunc = Function::Create(pNewFTy, pFunc->getLinkage());
		pNewFunc->copyAttributesFrom(pFunc);
		pNewFunc->setSubprogram(pFunc->getSubprogram());
		pFunc->setSubprogram(nullptr);
		m_pModule->getFunctionList().insert(pFunc->getIterator(), pNewFunc);
		pNewFunc->takeName(pFunc);

		// Since we have now created the new function, splice the body of the old
		// function right into the new function, leaving the old body of the function empty.
		pNewFunc->getBasicBlockList().splice(pNewFunc->begin(), pFunc->getBasicBlockList());

		// Loop over the argument list, transferring uses of the old arguments over to
		// the new arguments
		// updateNewFuncArgs(pFunc, pNewFunc, &implicitArgs);
		Function::arg_iterator currArg = pNewFunc->arg_begin();
		for (Function::arg_iterator I = pFunc->arg_begin(), E = pFunc->arg_end();
			I != E; ++I, ++currArg)
		{
			llvm::Value* newArg = &(*currArg);
			// Move the name and users over to the new version.
			I->replaceAllUsesWith(newArg);
			currArg->takeName(&(*I));
		}

		// Set name for implicit args
		int iArgIx = 0;
		for (Function::arg_iterator AE = pNewFunc->arg_end(); currArg != AE; ++currArg, ++iArgIx)
		{
			currArg->setName(implicitArgs[iArgIx].getName());
		}

		// Assign metadata for old_func to new_func
		auto oldFuncIter = m_pMdUtils->findFunctionsInfoItem(pFunc);
		m_pMdUtils->setFunctionsInfoItem(pNewFunc, oldFuncIter->second);
		m_pMdUtils->eraseFunctionsInfoItem(oldFuncIter);

        if (m_pCtx->getModuleMetaData()->FuncMD.find(pFunc) != m_pCtx->getModuleMetaData()->FuncMD.end())
        {
            m_pCtx->getModuleMetaData()->FuncMD[pNewFunc] = m_pCtx->getModuleMetaData()->FuncMD[pFunc];
            m_pCtx->getModuleMetaData()->FuncMD.erase(pFunc);
        }

		// Map old func to new func
		NewFuncWithIA.push_back(pNewFunc);
	}
}

void PreCompiledFuncImport::replaceFunc(Function* old_func, Function* new_func)
{
	FunctionInfoMetaDataHandle newFH = m_pMdUtils->getFunctionsInfoItem(new_func);
	std::vector<Instruction*> list_delete;
	for (auto U = old_func->user_begin(), UE = old_func->user_end(); U != UE; ++U)
	{
		std::vector<Value*> new_args;
		CallInst *cInst = dyn_cast<CallInst>(*U);
		if (!cInst)
		{
			m_pCtx->EmitError(" undefined reference to `jmp()' ");
			return;
		}
		Function *parent_func = cInst->getParent()->getParent();
		size_t numArgOperands = cInst->getNumArgOperands();

		// let's prepare argument list on new call function
		llvm::Function::arg_iterator new_arg_iter = new_func->arg_begin();
		llvm::Function::arg_iterator new_arg_end = new_func->arg_end();

		assert(IGCLLVM::GetFuncArgSize(new_func) >= numArgOperands);

		// basic arguments
		for (unsigned int i = 0; i < numArgOperands; ++i, ++new_arg_iter)
		{
			llvm::Value* arg = cInst->getOperand(i);
			new_args.push_back(arg);
		}

		// implicit arguments
		ImplicitArgs *parentIA = getImplicitArgs(parent_func);
		int cImpCount = 0;
		while (new_arg_iter != new_arg_end)
		{
			ArgInfoMetaDataHandle argInfo = newFH->getImplicitArgInfoListItem(cImpCount);
			ImplicitArg::ArgType argId = (ImplicitArg::ArgType)argInfo->getArgId();
			Argument *iArgVal = parentIA->getArgInFunc(*parent_func, argId);

			new_args.push_back(iArgVal);
			++new_arg_iter;
			++cImpCount;
		}

		// insert new call instruction before old one
		CallInst *inst;
		if (new_func->getReturnType()->isVoidTy())
		{
			inst = CallInst::Create(new_func, new_args, "", cInst);
		}
		else
		{
			inst = CallInst::Create(new_func, new_args, new_func->getName(), cInst);
		}
		inst->setCallingConv(new_func->getCallingConv());
		inst->setDebugLoc(cInst->getDebugLoc());
		cInst->replaceAllUsesWith(inst);
		list_delete.push_back(cInst);
	}

	for (auto i : list_delete)
	{
		i->eraseFromParent();
	}

	assert(old_func->use_empty() && "old_func should have no use at this point!");
	// Now, after changing funciton signature,
	// and validate there are no calls to the old function we can erase it.
	//
	// Note that old_func's metadata has been deleted already
	old_func->eraseFromParent();
}

ImplicitArgs* PreCompiledFuncImport::getImplicitArgs(Function *F)
{
	if (FuncsImpArgs.count(F) == 0)
	{
		ImplicitArgs *IA = new ImplicitArgs(*F, m_pMdUtils);
		FuncsImpArgs[F] = IA;
	}
	return FuncsImpArgs[F];
}


void PreCompiledFuncImport::checkAndSetEnableSubroutine()
{
	// Skip if subroutine is already on.
	if (m_pCtx->m_enableSubroutine)
	{
		m_enableSubroutineCallForEmulation = true;
		return;
	}
	else if (IGC_IS_FLAG_ENABLED(ForceSubroutineForEmulation))
	{
		m_enableSubroutineCallForEmulation = true;
		m_pCtx->m_enableSubroutine = true;
		return;
	}

	// Only simd8 is supported for subroutine. Make sure simd8 is allowed!
	if (m_pCtx->type == ShaderType::COMPUTE_SHADER)
	{
		ComputeShaderContext* ctx = static_cast<ComputeShaderContext*>(m_pCtx);
		SIMDMode simdMode = ctx->GetLeastSIMDModeAllowed();
		if (simdMode != SIMDMode::SIMD8)
			return;
	}

	Module* M = m_pCtx->getModule();
	for (auto FI = M->begin(), FE = M->end(); FI != FE; ++FI)
	{
		Function *F = &*FI;
		for (auto II = inst_begin(F), IE = inst_end(F); II != IE; ++II)
		{
			Instruction *I = &*II;
			switch (I->getOpcode()) {
			default:
				break;
			case Instruction::FDiv:
			case Instruction::FMul:
			case Instruction::FAdd:
			case Instruction::FSub:
			case Instruction::FCmp:
			  {
				if (I->getOperand(0)->getType()->isDoubleTy())
				{
					m_enableSubroutineCallForEmulation = true;
				}
				break;
			  }
		    case Instruction::FPToSI:
			case Instruction::FPToUI:
			case Instruction::FPTrunc:
				if (I->getOperand(0)->getType()->isDoubleTy())
				{
					m_enableSubroutineCallForEmulation = true;
				}
				break;
			case Instruction::SIToFP:
			case Instruction::UIToFP:
			case Instruction::FPExt:
				if (I->getType()->isDoubleTy())
				{
					m_enableSubroutineCallForEmulation = true;
				}
				break;
			}

			IntrinsicInst *IInst = dyn_cast<IntrinsicInst>(I);
			// Handle intrinsic calls
			if (IInst && IInst->getIntrinsicID() == Intrinsic::sqrt)
			{
                if (I->getType()->isDoubleTy())
                {
                    m_enableSubroutineCallForEmulation = true;
                }
			}
            else if (IInst && IInst->getIntrinsicID() == Intrinsic::fma)
            {
                if (I->getType()->isDoubleTy())
                {
                    m_enableSubroutineCallForEmulation = true;
                }
            }

			if (m_enableSubroutineCallForEmulation) {
				m_pCtx->m_enableSubroutine = true;
				return;
			}
		}
	}
}
