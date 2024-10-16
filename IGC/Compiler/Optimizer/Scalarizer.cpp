/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/Scalarizer.h"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/Support/Alignment.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_os_ostream.h"
#include "common/LLVMWarningsPop.hpp"
#include "common/igc_regkeys.hpp"
#include "common/Types.hpp"
#include "Probe/Assertion.h"
#include <vector>

using namespace llvm;
using namespace IGC;

#define V_PRINT(a,b) \
    { \
        if (IGC_IS_FLAG_ENABLED(EnableScalarizerDebugLog)) \
        { \
            outs() << b; \
        } \
    }

namespace VectorizerUtils {
    static void SetDebugLocBy(Instruction* I, const Instruction* setBy) {
        if (!(I->getDebugLoc())) {
            I->setDebugLoc(setBy->getDebugLoc());
        }
    }
}

// Register pass to igc-opt
#define PASS_FLAG "igc-scalarize"
#define PASS_DESCRIPTION "Scalarize functions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ScalarizeFunction, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_END(ScalarizeFunction, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ScalarizeFunction::ID = 0;

ScalarizeFunction::ScalarizeFunction(IGC::SelectiveScalarizer selectiveMode) : FunctionPass(ID)
{
    initializeScalarizeFunctionPass(*PassRegistry::getPassRegistry());

    for (int i = 0; i < Instruction::OtherOpsEnd; i++) m_transposeCtr[i] = 0;

    V_PRINT(scalarizer, "ScalarizeFunction constructor\n");
    switch(selectiveMode) {
    case IGC::SelectiveScalarizer::Off:
        V_PRINT(scalarizer, "IGC_EnableSelectiveScalarizer forced off");
        m_SelectiveScalarization = false;
        break;
    case IGC::SelectiveScalarizer::On:
        V_PRINT(scalarizer, "IGC_EnableSelectiveScalarizer forced on");
        m_SelectiveScalarization = true;
        break;
    case IGC::SelectiveScalarizer::Auto:
        V_PRINT(scalarizer, "IGC_EnableSelectiveScalarizer = ");
        V_PRINT(scalarizer, IGC_IS_FLAG_ENABLED(EnableSelectiveScalarizer));
        m_SelectiveScalarization = IGC_IS_FLAG_ENABLED(EnableSelectiveScalarizer);
        break;
    }
    V_PRINT(scalarizer, "\n");

    // Initialize SCM buffers and allocation
    m_SCMAllocationArray = new SCMEntry[ESTIMATED_INST_NUM];
    m_SCMArrays.push_back(m_SCMAllocationArray);
    m_SCMArrayLocation = 0;
}

bool ScalarizeFunction::doFinalization(llvm::Module& M) {
    releaseAllSCMEntries();
    delete[] m_SCMAllocationArray;
    destroyDummyFunc();
    V_PRINT(scalarizer, "ScalarizeFunction doFinalization\n");
    return true;
}

bool ScalarizeFunction::runOnFunction(Function& F)
{
    CodeGenContext* pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    if (!IGC::ForceAlwaysInline(pCtx))
    {
        if (F.isDeclaration()) return false;
    }
    else
    {
        // Scalarization is done only on functions which return void (kernels)
        if (!F.getReturnType()->isVoidTy())
        {
            return false;
        }
    }

    m_currFunc = &F;
    m_moduleContext = &(m_currFunc->getContext());

    V_PRINT(scalarizer, "\nStart scalarizing function: " << m_currFunc->getName() << "\n");

    // obtain TagetData of the module
    m_pDL = &F.getParent()->getDataLayout();

    // Prepare data structures for scalarizing a new function
    m_usedVectors.clear();
    m_removedInsts.clear();
    m_SCM.clear();
    releaseAllSCMEntries();
    m_DRL.clear();
    m_Excludes.clear();

    // collecting instructions that we want to avoid scalarization
    if (m_SelectiveScalarization)
    {
        buildExclusiveSet();
    }

    // Scalarization. Iterate over all the instructions
    // Always hold the iterator at the instruction following the one being scalarized (so the
    // iterator will "skip" any instructions that are going to be added in the scalarization work)
    auto DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    for (auto dfi = df_begin(DT->getRootNode()), dfe = df_end(DT->getRootNode());
        dfi != dfe; ++dfi)
    {
        BasicBlock* bb = (*dfi)->getBlock();
        auto sI = bb->begin();
        auto sE = bb->end();
        while (sI != sE)
        {
            Instruction* currInst = &*sI;
            // Move iterator to next instruction BEFORE scalarizing current instruction
            ++sI;
            if (m_Excludes.count(currInst))
            {
                recoverNonScalarizableInst(currInst);
            }
            else
            {
                dispatchInstructionToScalarize(currInst);
            }
        }
    }

    resolveVectorValues();

    // Resolved DRL entries
    resolveDeferredInstructions();

    // Iterate over removed insts and delete them
    SmallDenseSet<Instruction*, ESTIMATED_INST_NUM>::iterator ri = m_removedInsts.begin();
    SmallDenseSet<Instruction*, ESTIMATED_INST_NUM>::iterator re = m_removedInsts.end();
    SmallDenseSet<Instruction*, ESTIMATED_INST_NUM>::iterator index = ri;

    for (; index != re; ++index)
    {
        // get rid of old users
        if (Value* val = dyn_cast<Value>(*index))
        {
            UndefValue* undefVal = UndefValue::get((*index)->getType());
            (val)->replaceAllUsesWith(undefVal);
        }
        IGC_ASSERT_MESSAGE((*index)->use_empty(), "Unable to remove used instruction");
        (*index)->eraseFromParent();
    }

    V_PRINT(scalarizer, "\nCompleted scalarizing function: " << m_currFunc->getName() << "\n");
    return true;
}

/// <summary>
/// @brief We want to avoid scalarization of vector instructions if the vector is used
/// as a whole entity somewhere in the program. This function tries to find
/// this kind of definition web that involves phi-node, insert-element etc,
/// then add them into the exclusion-set (excluded from scalarization).
/// </summary>
void ScalarizeFunction::buildExclusiveSet()
{

    auto isAddToWeb = [](Value* V) -> bool {
        return isa<PHINode>(V) || isa<BitCastInst>(V);
        };

    auto DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    for (auto dfi = df_begin(DT->getRootNode()), dfe = df_end(DT->getRootNode());
        dfi != dfe; ++dfi)
    {
        BasicBlock* bb = (*dfi)->getBlock();
        auto sI = bb->begin();
        auto sE = bb->end();
        while (sI != sE)
        {
            Instruction* currInst = &*sI;
            ++sI;
            // find the seed for the workset
            std::vector<Value*> workset;

            // Instructions that accept vectorial arguments can end legs of the web
            // i.e. the instructions that produce the vectorial arguments may be protected from scalarization
            if (GenIntrinsicInst* GII = dyn_cast<GenIntrinsicInst>(currInst))
            {
                unsigned numOperands = IGCLLVM::getNumArgOperands(GII);
                for (unsigned i = 0; i < numOperands; i++)
                {
                    Value* operand = GII->getArgOperand(i);
                    if (isa<VectorType>(operand->getType()))
                    {
                        workset.push_back(operand);
                    }
                }
            }
            else if (CallInst * CI = dyn_cast<CallInst>(currInst))
            {
                for (auto arg = CI->arg_begin(); arg != CI->arg_end(); ++arg)
                {
                    if (isa<VectorType>(arg->get()->getType()))
                    {
                        workset.push_back(arg->get());
                    }
                }
            }
            else if (auto IEI = dyn_cast<InsertElementInst>(currInst))
            {
                Value* scalarIndexVal = IEI->getOperand(2);
                // If the index is not a constant - we cannot statically remove this inst
                if (!isa<ConstantInt>(scalarIndexVal)) {
                    workset.push_back(IEI);
                }
            }
            else if (auto EEI = dyn_cast<ExtractElementInst>(currInst))
            {
                Value* scalarIndexVal = EEI->getOperand(1);
                // If the index is not a constant - we cannot statically remove this inst
                if (!isa<ConstantInt>(scalarIndexVal)) {
                    workset.push_back(EEI->getOperand(0));
                }
            }
            else if (BitCastInst* BCI = dyn_cast<BitCastInst>(currInst))
            {
              auto isBitcastSink = [](BitCastInst *BCI) -> bool {
                auto *SrcVTy = dyn_cast<IGCLLVM::FixedVectorType>(
                    BCI->getOperand(0)->getType());

                // If source is not a vector, we don't care about this bitcast
                if (!SrcVTy)
                  return false;

                // If destination is a vector then we scalarize if the number of
                // elements are the same (elementwise bitcast)
                if (auto *DestVTy =
                        dyn_cast<IGCLLVM::FixedVectorType>(BCI->getType()))
                  return DestVTy->getNumElements() != SrcVTy->getNumElements();

                // If destination is not a vector, we don't want to scalarize
                return true;
              };

              if (isBitcastSink(BCI)) {
                workset.push_back(BCI->getOperand(0));
              }
            }
            // try to find a web from the seed
            std::set<Value*> defweb;
            while (!workset.empty())
            {
                auto* Def = workset.back();
                workset.pop_back();
                if (m_Excludes.count(Def) || defweb.count(Def))
                {
                    continue;
                }

                // The web grows "up" (towards producers) through BitCasts and PHI nodes
                // but insert/extract elements and vector shuffles should be scalarized
                if (!isAddToWeb(Def)) continue;

                if (BitCastInst* BCI = dyn_cast<BitCastInst>(Def))
                {
                    defweb.insert(BCI);
                    if (!defweb.count(BCI->getOperand(0)) && isAddToWeb(BCI->getOperand(0)))
                    {
                        workset.push_back(BCI->getOperand(0));
                    }
                }
                else if (auto PHI = dyn_cast<PHINode>(Def))
                {
                    defweb.insert(PHI);
                    for (int i = 0, n = PHI->getNumOperands(); i < n; ++i)
                    {
                        if (!defweb.count(PHI->getOperand(i)) && isAddToWeb(PHI->getOperand(i)))
                        {
                            workset.push_back(PHI->getOperand(i));
                        }
                    }
                }
                else
                {
                    continue;
                }

                // The web grows "down" (towards users) through BitCasts and PHI nodes as well
                for (auto U : Def->users())
                {
                    if (!defweb.count(U) && isAddToWeb(U))
                    {
                        workset.push_back(U);
                    }
                }
            }
            m_Excludes.merge(defweb);
        }
    }
}

void ScalarizeFunction::dispatchInstructionToScalarize(Instruction* I)
{
    V_PRINT(scalarizer, "\tScalarizing Instruction: " << *I << "\n");

    if (m_removedInsts.count(I))
    {
        V_PRINT(scalarizer, "\tInstruction is already marked for removal. Being ignored..\n");
        return;
    }

    switch (I->getOpcode())
    {
    case Instruction::FNeg:
        scalarizeInstruction(dyn_cast<UnaryOperator>(I));
        break;
    case Instruction::Add:
    case Instruction::Sub:
    case Instruction::Mul:
    case Instruction::FAdd:
    case Instruction::FSub:
    case Instruction::FMul:
    case Instruction::UDiv:
    case Instruction::SDiv:
    case Instruction::FDiv:
    case Instruction::URem:
    case Instruction::SRem:
    case Instruction::FRem:
    case Instruction::Shl:
    case Instruction::LShr:
    case Instruction::AShr:
    case Instruction::And:
    case Instruction::Or:
    case Instruction::Xor:
        scalarizeInstruction(dyn_cast<BinaryOperator>(I));
        break;
    case Instruction::ICmp:
    case Instruction::FCmp:
        scalarizeInstruction(dyn_cast<CmpInst>(I));
        break;
    case Instruction::Trunc:
    case Instruction::ZExt:
    case Instruction::SExt:
    case Instruction::FPToUI:
    case Instruction::FPToSI:
    case Instruction::UIToFP:
    case Instruction::SIToFP:
    case Instruction::FPTrunc:
    case Instruction::FPExt:
    case Instruction::PtrToInt:
    case Instruction::IntToPtr:
    case Instruction::BitCast:
        scalarizeInstruction(dyn_cast<CastInst>(I));
        break;
    case Instruction::PHI:
        if (IGC_IS_FLAG_DISABLED(DisablePHIScalarization))
            scalarizeInstruction(dyn_cast<PHINode>(I));
        else
            recoverNonScalarizableInst(I);
        break;
    case Instruction::Select:
        scalarizeInstruction(dyn_cast<SelectInst>(I));
        break;
    case Instruction::ExtractElement:
        scalarizeInstruction(dyn_cast<ExtractElementInst>(I));
        break;
    case Instruction::InsertElement:
        scalarizeInstruction(dyn_cast<InsertElementInst>(I));
        break;
    case Instruction::ShuffleVector:
        scalarizeInstruction(dyn_cast<ShuffleVectorInst>(I));
        break;
        //case Instruction::Call :
        //  scalarizeInstruction(dyn_cast<CallInst>(I));
        //  break;
    case Instruction::Alloca:
        scalarizeInstruction(dyn_cast<AllocaInst>(I));
        break;
    case Instruction::GetElementPtr:
        scalarizeInstruction(dyn_cast<GetElementPtrInst>(I));
        break;
        // The remaining instructions are not supported for scalarization. Keep "as is"
    default:
        recoverNonScalarizableInst(I);
        break;
    }
}

void ScalarizeFunction::recoverNonScalarizableInst(Instruction* Inst)
{
    V_PRINT(scalarizer, "\t\tInstruction is not scalarizable.\n");

    // any vector value should have an SCM entry - even an empty one
    if (isa<VectorType>(Inst->getType())) getSCMEntry(Inst);

    // Iterate over all arguments. Check that they all exist (or rebuilt)
    if (CallInst* CI = dyn_cast<CallInst>(Inst))
    {
        unsigned numOperands = IGCLLVM::getNumArgOperands(CI);
        for (unsigned i = 0; i < numOperands; i++)
        {
            Value* operand = CI->getArgOperand(i);
            if (isa<VectorType>(operand->getType()))
            {
                // Recover value if needed (only needed for vector values)
                obtainVectorValueWhichMightBeScalarized(operand);
            }
        }
    }
    else
    {
        unsigned numOperands = Inst->getNumOperands();
        for (unsigned i = 0; i < numOperands; i++)
        {
            Value* operand = Inst->getOperand(i);
            if (isa<VectorType>(operand->getType()))
            {
                // Recover value if needed (only needed for vector values)
                obtainVectorValueWhichMightBeScalarized(operand);
            }
        }
    }
}

void ScalarizeFunction::scalarizeInstruction(UnaryOperator* UI)
{
    V_PRINT(scalarizer, "\t\tUnary instruction\n");
    IGC_ASSERT_MESSAGE(UI, "instruction type dynamic cast failed");
    IGCLLVM::FixedVectorType* instType = dyn_cast<IGCLLVM::FixedVectorType>(UI->getType());
    // Do not apply if optnone function
    if (UI->getFunction()->hasOptNone())
        return recoverNonScalarizableInst(UI);

    // Only need handling for vector binary ops
    if (!instType) return;

    // Prepare empty SCM entry for the instruction
    SCMEntry* newEntry = getSCMEntry(UI);

    // Get additional info from instruction
    unsigned numElements = int_cast<unsigned>(instType->getNumElements());

    // Obtain scalarized argument
    SmallVector<Value*, MAX_INPUT_VECTOR_WIDTH>operand0;
    bool op0IsConst;

    obtainScalarizedValues(operand0, &op0IsConst, UI->getOperand(0), UI);

    // If argument is constant, don't bother Scalarizing inst
    if (op0IsConst) return;

    // Generate new (scalar) instructions
    SmallVector<Value*, MAX_INPUT_VECTOR_WIDTH>newScalarizedInsts;
    newScalarizedInsts.resize(numElements);
    for (unsigned dup = 0; dup < numElements; dup++)
    {
        Value* Val = UnaryOperator::Create(
            UI->getOpcode(),
            operand0[dup],
            UI->getName(),
            UI
        );
        if (UnaryOperator* UO = dyn_cast<UnaryOperator>(Val)) {
            // Copy fast math flags if any.
            if (isa<FPMathOperator>(UO))
                UO->setFastMathFlags(UI->getFastMathFlags());
        }
        newScalarizedInsts[dup] = Val;
    }

    // Add new value/s to SCM
    updateSCMEntryWithValues(newEntry, &(newScalarizedInsts[0]), UI, true);

    // Remove original instruction
    m_removedInsts.insert(UI);
}

void ScalarizeFunction::scalarizeInstruction(BinaryOperator* BI)
{
    V_PRINT(scalarizer, "\t\tBinary instruction\n");
    IGC_ASSERT_MESSAGE(BI, "instruction type dynamic cast failed");
    IGCLLVM::FixedVectorType* instType = dyn_cast<IGCLLVM::FixedVectorType>(BI->getType());
    // Only need handling for vector binary ops
    if (!instType) return;

    // Prepare empty SCM entry for the instruction
    SCMEntry* newEntry = getSCMEntry(BI);

    // Get additional info from instruction
    unsigned numElements = int_cast<unsigned>(instType->getNumElements());

    // Obtain scalarized arguments
    SmallVector<Value*, MAX_INPUT_VECTOR_WIDTH>operand0;
    SmallVector<Value*, MAX_INPUT_VECTOR_WIDTH>operand1;
    bool op0IsConst, op1IsConst;

    obtainScalarizedValues(operand0, &op0IsConst, BI->getOperand(0), BI);
    obtainScalarizedValues(operand1, &op1IsConst, BI->getOperand(1), BI);

    // If both arguments are constants, don't bother Scalarizing inst
    if (op0IsConst && op1IsConst) return;

    // Generate new (scalar) instructions
    SmallVector<Value*, MAX_INPUT_VECTOR_WIDTH>newScalarizedInsts;
    newScalarizedInsts.resize(numElements);
    for (unsigned dup = 0; dup < numElements; dup++)
    {
        Value* Val = BinaryOperator::Create(
            BI->getOpcode(),
            operand0[dup],
            operand1[dup],
            BI->getName(),
            BI
        );
        if (BinaryOperator* BO = dyn_cast<BinaryOperator>(Val)) {
            // Copy overflow flags if any.
            if (isa<OverflowingBinaryOperator>(BO)) {
                BO->setHasNoSignedWrap(BI->hasNoSignedWrap());
                BO->setHasNoUnsignedWrap(BI->hasNoUnsignedWrap());
            }
            // Copy exact flag if any.
            if (isa<PossiblyExactOperator>(BO))
                BO->setIsExact(BI->isExact());
            // Copy fast math flags if any.
            if (isa<FPMathOperator>(BO))
                BO->setFastMathFlags(BI->getFastMathFlags());
        }
        newScalarizedInsts[dup] = Val;
    }

    // Add new value/s to SCM
    updateSCMEntryWithValues(newEntry, &(newScalarizedInsts[0]), BI, true);

    // Remove original instruction
    m_removedInsts.insert(BI);
}

void ScalarizeFunction::scalarizeInstruction(CmpInst* CI)
{
    V_PRINT(scalarizer, "\t\tCompare instruction\n");
    IGC_ASSERT_MESSAGE(CI, "instruction type dynamic cast failed");
    IGCLLVM::FixedVectorType* instType = dyn_cast<IGCLLVM::FixedVectorType>(CI->getType());
    // Only need handling for vector compares
    if (!instType) return;

    // Prepare empty SCM entry for the instruction
    SCMEntry* newEntry = getSCMEntry(CI);

    // Get additional info from instruction
    unsigned numElements = int_cast<unsigned>(instType->getNumElements());

    // Obtain scalarized arguments

    SmallVector<Value*, MAX_INPUT_VECTOR_WIDTH>operand0;
    SmallVector<Value*, MAX_INPUT_VECTOR_WIDTH>operand1;
    bool op0IsConst, op1IsConst;

    obtainScalarizedValues(operand0, &op0IsConst, CI->getOperand(0), CI);
    obtainScalarizedValues(operand1, &op1IsConst, CI->getOperand(1), CI);

    // If both arguments are constants, don't bother Scalarizing inst
    if (op0IsConst && op1IsConst) return;

    // Generate new (scalar) instructions
    SmallVector<Value*, MAX_INPUT_VECTOR_WIDTH>newScalarizedInsts;
    newScalarizedInsts.resize(numElements);
    for (unsigned dup = 0; dup < numElements; dup++)
    {
        newScalarizedInsts[dup] = CmpInst::Create(
            CI->getOpcode(),
            CI->getPredicate(),
            operand0[dup],
            operand1[dup],
            CI->getName(),
            CI
        );
    }

    // Add new value/s to SCM
    updateSCMEntryWithValues(newEntry, &(newScalarizedInsts[0]), CI, true);

    // Remove original instruction
    m_removedInsts.insert(CI);
}

void ScalarizeFunction::scalarizeInstruction(CastInst* CI)
{
    V_PRINT(scalarizer, "\t\tCast instruction\n");
    IGC_ASSERT_MESSAGE(CI, "instruction type dynamic cast failed");
    IGCLLVM::FixedVectorType* instType = dyn_cast<IGCLLVM::FixedVectorType>(CI->getType());

    // For BitCast - we only scalarize if src and dst types have same vector length
    if (isa<BitCastInst>(CI))
    {
        if (!instType) return recoverNonScalarizableInst(CI);
        IGCLLVM::FixedVectorType* srcType = dyn_cast<IGCLLVM::FixedVectorType>(CI->getOperand(0)->getType());
        if (!srcType || (instType->getNumElements() != srcType->getNumElements()))
        {
            return recoverNonScalarizableInst(CI);
        }
    }

    // Only need handling for vector cast
    if (!instType) return;

    // Prepare empty SCM entry for the instruction
    SCMEntry* newEntry = getSCMEntry(CI);

    // Get additional info from instruction
    unsigned numElements = int_cast<unsigned>(instType->getNumElements());
    IGC_ASSERT_MESSAGE(
        isa<IGCLLVM::FixedVectorType>(CI->getOperand(0)->getType()),
        "unexpected type!");
    IGC_ASSERT_MESSAGE(
        cast<IGCLLVM::FixedVectorType>(CI->getOperand(0)->getType())
        ->getNumElements() == numElements,
        "unexpected vector width");

    // Obtain scalarized argument
    SmallVector<Value*, MAX_INPUT_VECTOR_WIDTH>operand0;
    bool op0IsConst;

    obtainScalarizedValues(operand0, &op0IsConst, CI->getOperand(0), CI);

    // If argument is a constant, don't bother Scalarizing inst
    if (op0IsConst) return;

    // Obtain type, which ever scalar cast will cast-to
    Type* scalarDestType = instType->getElementType();

    // Generate new (scalar) instructions
    SmallVector<Value*, MAX_INPUT_VECTOR_WIDTH>newScalarizedInsts;
    newScalarizedInsts.resize(numElements);
    for (unsigned dup = 0; dup < numElements; dup++)
    {
        newScalarizedInsts[dup] = CastInst::Create(
            CI->getOpcode(),
            operand0[dup],
            scalarDestType,
            CI->getName(),
            CI
        );
    }

    // Add new value/s to SCM
    updateSCMEntryWithValues(newEntry, &(newScalarizedInsts[0]), CI, true);

    // Remove original instruction
    m_removedInsts.insert(CI);
}

void ScalarizeFunction::scalarizeInstruction(PHINode* PI)
{
    V_PRINT(scalarizer, "\t\tPHI instruction\n");
    IGC_ASSERT_MESSAGE(PI, "instruction type dynamic cast failed");
    IGCLLVM::FixedVectorType* instType = dyn_cast<IGCLLVM::FixedVectorType>(PI->getType());
    // Only need handling for vector PHI
    if (!instType) return;

    // Obtain number of incoming nodes \ PHI values
    unsigned numValues = PI->getNumIncomingValues();

    // Normally, a phi would be scalarized and a collection of
    // extractelements would be emitted for each value.  Since
    // VME payload CVariables don't necessarily match the size
    // of the llvm type, keep these phis vectorized here so we
    // can emit the appropriate movs in emitVectorCopy() when
    // emitting movs for phis.
    for (unsigned i = 0; i < numValues; i++)
    {
        auto* Op = PI->getIncomingValue(i);

        if (auto* GII = dyn_cast<GenIntrinsicInst>(Op))
        {
            switch (GII->getIntrinsicID())
            {
            case GenISAIntrinsic::GenISA_vmeSendIME2:
            case GenISAIntrinsic::GenISA_vmeSendFBR2:
            case GenISAIntrinsic::GenISA_vmeSendSIC2:
                recoverNonScalarizableInst(PI);
                return;

            default: break;
            }
        }
    }

    {
        // If PHI is used in insts that take vector as operands, keep this vector phi.
        // With the vector phi, variable alias can do a better job. Otherwise, more mov
        // insts could be generated.
        DenseMap<PHINode*, int> visited;
        SmallVector<PHINode*, 8> phis;
        phis.push_back(PI);
        while (!phis.empty())
        {
            PHINode* PN = phis.back();
            phis.pop_back();
            for (auto U : PN->users())
            {
                if (GenIntrinsicInst* GII = dyn_cast<GenIntrinsicInst>(U))
                {
                    switch (GII->getIntrinsicID())
                    {
                    default:
                        break;
                    case GenISAIntrinsic::GenISA_sub_group_dpas:
                    case GenISAIntrinsic::GenISA_dpas:
                    case GenISAIntrinsic::GenISA_simdBlockWrite:
                    case GenISAIntrinsic::GenISA_simdBlockWriteBindless:
                    case GenISAIntrinsic::GenISA_simdMediaBlockWrite:
                    case GenISAIntrinsic::GenISA_LSC2DBlockWrite:
                    case GenISAIntrinsic::GenISA_LSC2DBlockWriteAddrPayload:
                    case GenISAIntrinsic::GenISA_LSCStoreBlock:
                        recoverNonScalarizableInst(PI);
                        return;
                    }
                }
                else if (PHINode* N = dyn_cast<PHINode>(U))
                {
                    if (visited.count(N) == 0) {
                        visited[N] = 1;
                        phis.push_back(N);
                    }
                }
            }
        }
        visited.clear();
        phis.clear();
    }

    // Prepare empty SCM entry for the instruction
    SCMEntry* newEntry = getSCMEntry(PI);

    // Get additional info from instruction
    Type* scalarType = instType->getElementType();
    unsigned numElements = int_cast<unsigned>(instType->getNumElements());

    // Create new (empty) PHI nodes, and place them.
    SmallVector<Value*, MAX_INPUT_VECTOR_WIDTH>newScalarizedPHI;
    newScalarizedPHI.resize(numElements);
    for (unsigned i = 0; i < numElements; i++)
    {
        newScalarizedPHI[i] = PHINode::Create(scalarType, numValues, PI->getName(), PI);
    }

    // Iterate over incoming values in vector PHI, and fill scalar PHI's accordingly
    SmallVector<Value*, MAX_INPUT_VECTOR_WIDTH>operand;

    for (unsigned j = 0; j < numValues; j++)
    {
        // Obtain scalarized arguments
        obtainScalarizedValues(operand, NULL, PI->getIncomingValue(j), PI);

        // Fill all scalarized PHI nodes with scalar arguments
        for (unsigned i = 0; i < numElements; i++)
        {
            cast<PHINode>(newScalarizedPHI[i])->addIncoming(operand[i], PI->getIncomingBlock(j));
        }
    }

    // Add new value/s to SCM
    updateSCMEntryWithValues(newEntry, &(newScalarizedPHI[0]), PI, true);

    // Remove original instruction
    m_removedInsts.insert(PI);
}

void ScalarizeFunction::scalarizeInstruction(SelectInst* SI)
{
    V_PRINT(scalarizer, "\t\tSelect instruction\n");
    IGC_ASSERT_MESSAGE(SI, "instruction type dynamic cast failed");
    IGCLLVM::FixedVectorType* instType = dyn_cast<IGCLLVM::FixedVectorType>(SI->getType());
    // Only need handling for vector select
    if (!instType) return;

    // Prepare empty SCM entry for the instruction
    SCMEntry* newEntry = getSCMEntry(SI);

    // Get additional info from instruction
    unsigned numElements = int_cast<unsigned>(instType->getNumElements());

    // Obtain scalarized arguments (select inst has 3 arguments: Cond, TrueVal, FalseVal)
    SmallVector<Value*, MAX_INPUT_VECTOR_WIDTH>condOp;
    SmallVector<Value*, MAX_INPUT_VECTOR_WIDTH>trueValOp;
    SmallVector<Value*, MAX_INPUT_VECTOR_WIDTH>falseValOp;

    obtainScalarizedValues(trueValOp, NULL, SI->getTrueValue(), SI);
    obtainScalarizedValues(falseValOp, NULL, SI->getFalseValue(), SI);

    // Check if condition is a vector.
    Value* conditionVal = SI->getCondition();
    if (isa<VectorType>(conditionVal->getType()))
    {
        // Obtain scalarized breakdowns of condition
        obtainScalarizedValues(condOp, NULL, conditionVal, SI);
    }
    else
    {
        condOp.resize(numElements);
        // Broadcast the (scalar) condition, to be used by all the insruction breakdowns
        for (unsigned i = 0; i < numElements; i++) condOp[i] = conditionVal;
    }

    // Generate new (scalar) instructions
    SmallVector<Value*, MAX_INPUT_VECTOR_WIDTH>newScalarizedInsts;
    newScalarizedInsts.resize(numElements);
    for (unsigned dup = 0; dup < numElements; dup++)
    {
        // Small optimization: Some scalar selects may be redundant (trueVal == falseVal)
        if (trueValOp[dup] != falseValOp[dup])
        {
            newScalarizedInsts[dup] = SelectInst::Create(
                condOp[dup],
                trueValOp[dup],
                falseValOp[dup],
                SI->getName(),
                SI
            );
        }
        else
        {
            // just "connect" the destination value to the true value input
            newScalarizedInsts[dup] = trueValOp[dup];
        }
    }

    // Add new value/s to SCM
    updateSCMEntryWithValues(newEntry, &(newScalarizedInsts[0]), SI, true);

    // Remove original instruction
    m_removedInsts.insert(SI);
}

void ScalarizeFunction::scalarizeInstruction(ExtractElementInst* EI)
{
    V_PRINT(scalarizer, "\t\tExtractElement instruction\n");
    IGC_ASSERT_MESSAGE(EI, "instruction type dynamic cast failed");

    // Proper scalarization makes "extractElement" instructions redundant
    // Only need to "follow" the scalar element (as the input vector was
    // already scalarized)
    Value* vectorValue = EI->getOperand(0);
    Value* scalarIndexVal = EI->getOperand(1);

    // If the index is not a constant - we cannot statically remove this inst
    if (!isa<ConstantInt>(scalarIndexVal)) return recoverNonScalarizableInst(EI);

    // Obtain the scalarized operands
    SmallVector<Value*, MAX_INPUT_VECTOR_WIDTH>operand;
    obtainScalarizedValues(operand, NULL, vectorValue, EI);

    // Connect the "extracted" value to all its consumers
    uint64_t scalarIndex = cast<ConstantInt>(scalarIndexVal)->getZExtValue();
    auto valueVType = cast<IGCLLVM::FixedVectorType>(vectorValue->getType());
    if (static_cast<unsigned int>(scalarIndex) < (unsigned)valueVType->getNumElements())
    {
        IGC_ASSERT_MESSAGE(NULL != operand[static_cast<unsigned int>(scalarIndex)], "SCM error");
        // Replace all users of this inst, with the extracted scalar value
        EI->replaceAllUsesWith(operand[static_cast<unsigned int>(scalarIndex)]);
    }
    else
    {
        IGC_ASSERT_MESSAGE(0, "The instruction extractElement is out of bounds.");
        EI->replaceAllUsesWith(UndefValue::get(valueVType->getElementType()));
    }

    // Remove original instruction
    m_removedInsts.insert(EI);
}

void ScalarizeFunction::scalarizeInstruction(InsertElementInst* II)
{
    V_PRINT(scalarizer, "\t\tInsertElement instruction\n");
    IGC_ASSERT_MESSAGE(II, "instruction type dynamic cast failed");

    // Proper scalarization makes "InsertElement" instructions redundant.
    // Only need to "follow" the scalar elements and update in SCM
    Value* sourceVectorValue = II->getOperand(0);
    Value* sourceScalarValue = II->getOperand(1);
    Value* scalarIndexVal = II->getOperand(2);

    // If the index is not a constant - we cannot statically remove this inst
    if (!isa<ConstantInt>(scalarIndexVal)) return recoverNonScalarizableInst(II);

    // Obtain breakdown of input vector
    SmallVector<Value*, MAX_INPUT_VECTOR_WIDTH>scalarValues;
    if (isa<UndefValue>(sourceVectorValue))
    {
        // Scalarize the undef value (generate a scalar undef)
        IGCLLVM::FixedVectorType* inputVectorType = dyn_cast<IGCLLVM::FixedVectorType>(sourceVectorValue->getType());
        IGC_ASSERT_MESSAGE(inputVectorType, "expected vector argument");

        UndefValue* undefVal = UndefValue::get(inputVectorType->getElementType());

        // fill new SCM entry with UNDEFs and the new value
        scalarValues.resize(static_cast<unsigned int>(inputVectorType->getNumElements()));
        for (unsigned j = 0; j < inputVectorType->getNumElements(); j++)
        {
            scalarValues[j] = undefVal;
        }
    }
    else
    {
        // Obtain the scalar values of the input vector
        obtainScalarizedValues(scalarValues, NULL, sourceVectorValue, II);
    }

    IGC_ASSERT_MESSAGE(isa<ConstantInt>(scalarIndexVal), "inst arguments error");
    uint64_t scalarIndex = cast<ConstantInt>(scalarIndexVal)->getZExtValue();

    if (scalarIndex <
        dyn_cast<IGCLLVM::FixedVectorType>(II->getType())->getNumElements())
    {
        // Add the new element
        scalarValues[static_cast<unsigned int>(scalarIndex)] = sourceScalarValue;
    }

    // Prepare empty SCM entry for the instruction
    SCMEntry* newEntry = getSCMEntry(II);

    // Add new value/s to SCM
    updateSCMEntryWithValues(newEntry, &(scalarValues[0]), II, true, false);

    // Remove original instruction
    m_removedInsts.insert(II);
}

void ScalarizeFunction::scalarizeInstruction(ShuffleVectorInst* SI)
{
    V_PRINT(scalarizer, "\t\tShuffleVector instruction\n");
    IGC_ASSERT_MESSAGE(nullptr != SI, "instruction type dynamic cast failed");

    // Proper scalarization makes "ShuffleVector" instructions redundant.
    // Only need to "follow" the scalar elements and update in SCM

    // Grab input vectors types and width
    Value* sourceVector0Value = SI->getOperand(0);
    IGC_ASSERT(nullptr != sourceVector0Value);
    Value* sourceVector1Value = SI->getOperand(1);
    IGC_ASSERT(nullptr != sourceVector1Value);
    IGCLLVM::FixedVectorType* const inputType = dyn_cast<IGCLLVM::FixedVectorType>(sourceVector0Value->getType());
    IGC_ASSERT_MESSAGE(nullptr != inputType, "vector input error");
    IGC_ASSERT_MESSAGE(inputType == sourceVector1Value->getType(), "vector input error");
    unsigned sourceVectorWidth = int_cast<unsigned>(inputType->getNumElements());

    // generate an array of values (pre-shuffle), which concatenates both vectors
    SmallVector<Value*, MAX_INPUT_VECTOR_WIDTH>allValues;
    allValues.resize(2 * sourceVectorWidth);

    // Obtain scalarized input values (into concatenated array). if vector was Undef - keep NULL.
    if (!isa<UndefValue>(sourceVector0Value))
    {
        obtainScalarizedValues(allValues, NULL, sourceVector0Value, SI, 0);
    }
    if (!isa<UndefValue>(sourceVector1Value))
    {
        // Place values, starting in the middle of concatenated array
        obtainScalarizedValues(allValues, NULL, sourceVector1Value, SI, sourceVectorWidth);
    }

    // Generate array for shuffled scalar values
    SmallVector<Value*, MAX_INPUT_VECTOR_WIDTH>newVector;
    unsigned width = int_cast<unsigned>(cast<IGCLLVM::FixedVectorType>(SI->getType())->getNumElements());

    // Generate undef value, which may be needed as some scalar elements
    UndefValue* undef = UndefValue::get(inputType->getElementType());

    newVector.resize(width);
    // Go over shuffle order, and place scalar values in array
    for (unsigned i = 0; i < width; i++)
    {
        int maskValue = SI->getMaskValue(i);
        if (maskValue >= 0 && NULL != allValues[maskValue])
        {
            newVector[i] = allValues[maskValue];
        }
        else
        {
            newVector[i] = undef;
        }
    }

    // Create the new SCM entry
    SCMEntry* newEntry = getSCMEntry(SI);
    updateSCMEntryWithValues(newEntry, &(newVector[0]), SI, true, false);

    // Remove original instruction
    m_removedInsts.insert(SI);
}

void ScalarizeFunction::scalarizeInstruction(CallInst* CI)
{
    V_PRINT(scalarizer, "\t\tCall instruction\n");
    IGC_ASSERT_MESSAGE(CI, "instruction type dynamic cast failed");

    recoverNonScalarizableInst(CI);
}

void ScalarizeFunction::scalarizeInstruction(AllocaInst* AI)
{
    V_PRINT(scalarizer, "\t\tAlloca instruction\n");
    IGC_ASSERT_MESSAGE(AI, "instruction type dynamic cast failed");

    return recoverNonScalarizableInst(AI);
}

void ScalarizeFunction::scalarizeInstruction(GetElementPtrInst* GI)
{
    V_PRINT(scalarizer, "\t\tGEP instruction\n");
    IGC_ASSERT_MESSAGE(GI, "instruction type dynamic cast failed");

    // If it has more than one index, leave it as is.
    if (GI->getNumIndices() != 1)
    {
        return recoverNonScalarizableInst(GI);
    }
    Value* baseValue = GI->getOperand(0);
    Value* indexValue = GI->getOperand(1);

    // If it's not a vector instruction, leave it as is.
    if (!baseValue->getType()->isVectorTy() && !indexValue->getType()->isVectorTy())
    {
        return recoverNonScalarizableInst(GI);
    }
    SmallVector<Value*, MAX_INPUT_VECTOR_WIDTH>operand1;
    SmallVector<Value*, MAX_INPUT_VECTOR_WIDTH>operand2;
    Type* ptrTy;
    unsigned width = 1;

    if (baseValue->getType()->isVectorTy())
    {
        width = int_cast<unsigned>(dyn_cast<IGCLLVM::FixedVectorType>(baseValue->getType())->getNumElements());
        // Obtain the scalarized operands
        obtainScalarizedValues(operand1, NULL, baseValue, GI);
        ptrTy = cast<VectorType>(baseValue->getType())->getElementType();
    }
    else
    {
        ptrTy = baseValue->getType();
    }
    if (indexValue->getType()->isVectorTy())
    {
        width = int_cast<unsigned>(dyn_cast<IGCLLVM::FixedVectorType>(indexValue->getType())->getNumElements());
        // Obtain the scalarized operands
        obtainScalarizedValues(operand2, NULL, indexValue, GI);
    }
    IGC_ASSERT_MESSAGE(width > 1, "expected vector instruction");
    SmallVector<Value*, MAX_INPUT_VECTOR_WIDTH>scalarValues;
    scalarValues.resize(width);

    Value* assembledVector = UndefValue::get(IGCLLVM::FixedVectorType::get(ptrTy, width));
    for (unsigned i = 0; i < width; ++i)
    {
        auto op1 = baseValue->getType()->isVectorTy() ? operand1[i] : baseValue;
        auto op2 = indexValue->getType()->isVectorTy() ? operand2[i] : indexValue;

        Type* BaseTy = IGCLLVM::getNonOpaquePtrEltTy(op1->getType());
        Value* newGEP = GetElementPtrInst::Create(BaseTy, op1, op2,
            VALUE_NAME(GI->getName()), GI);
        Value* constIndex = ConstantInt::get(Type::getInt32Ty(context()), i);
        Instruction* insert = InsertElementInst::Create(assembledVector,
            newGEP, constIndex,
            VALUE_NAME(GI->getName() + ".assembled.vect"), GI);
        assembledVector = insert;
        scalarValues[i] = newGEP;

        V_PRINT(scalarizer,
            "\t\t\tCreated vector assembly inst:" << *assembledVector << "\n");
    }
    // Prepare empty SCM entry for the new instruction
    SCMEntry* newEntry = getSCMEntry(assembledVector);
    // Add new value/s to SCM
    updateSCMEntryWithValues(newEntry, &(scalarValues[0]), assembledVector, true);
    GI->replaceAllUsesWith(assembledVector);

    // Remove original instruction
    m_removedInsts.insert(GI);
}

void ScalarizeFunction::obtainScalarizedValues(SmallVectorImpl<Value*>& retValues, bool* retIsConstant,
    Value* origValue, Instruction* origInst, int destIdx)
{
    V_PRINT(scalarizer, "\t\t\tObtaining scalar value... " << *origValue << "\n");

    IGCLLVM::FixedVectorType* origType = dyn_cast<IGCLLVM::FixedVectorType>(origValue->getType());
    IGC_ASSERT_MESSAGE(origType, "Value must have a vector type!");
    unsigned width = int_cast<unsigned>(origType->getNumElements());

    if (destIdx == -1)
    {
        destIdx = 0;
        retValues.resize(width);
    }

    if (NULL != retIsConstant)
    {
        // Set retIsConstant (return value) to true, if the origValue is constant
        if (!isa<Constant>(origValue))
        {
            *retIsConstant = false;
        }
        else
        {
            *retIsConstant = true;
        }
    }

    // Lookup value in SCM
    SCMEntry* currEntry = getScalarizedValues(origValue);
    if (currEntry && (NULL != currEntry->scalarValues[0]))
    {
        // Value was found in SCM
        V_PRINT(scalarizer,
            "\t\t\tFound existing entry in lookup of " << origValue->getName() << "\n");
        for (unsigned i = 0; i < width; i++)
        {
            // Copy values to return array
            IGC_ASSERT_MESSAGE(NULL != currEntry->scalarValues[i], "SCM entry contains NULL value");
            retValues[i + destIdx] = currEntry->scalarValues[i];
        }
    }
    else if (isa<UndefValue>(origValue))
    {
        IGC_ASSERT_MESSAGE(origType, "original value must have a vector type!");
        // value is an undefVal. Break it to element-sized undefs
        V_PRINT(scalarizer, "\t\t\tUndefVal constant\n");
        Value* undefElement = UndefValue::get(origType->getElementType());
        for (unsigned i = 0; i < width; i++)
        {
            retValues[i + destIdx] = undefElement;
        }
    }
    else if (Constant* vectorConst = dyn_cast<Constant>(origValue))
    {
        V_PRINT(scalarizer, "\t\t\tProper constant: " << *vectorConst << "\n");
        // Value is a constant. Break it down to scalars by employing a constant expression
        for (unsigned i = 0; i < width; i++)
        {
            retValues[i + destIdx] = ConstantExpr::getExtractElement(vectorConst,
                ConstantInt::get(Type::getInt32Ty(context()), i));
        }
    }
    else if (isa<Instruction>(origValue) && !currEntry)
    {
        // Instruction not found in SCM. Means it will be defined in a following basic block.
        // Generate a DRL: dummy values, which will be resolved after all scalarization is complete.
        V_PRINT(scalarizer, "\t\t\t*** Not found. Setting DRL. \n");
        Type* dummyType = origType->getElementType();
        Function* dummy_function = getOrCreateDummyFunc(dummyType, origInst->getModule());
        DRLEntry newDRLEntry;
        newDRLEntry.unresolvedInst = origValue;
        newDRLEntry.dummyVals.resize(width);
        for (unsigned i = 0; i < width; i++)
        {
            // Generate dummy "call" instruction (but don't really place in function)
            retValues[i + destIdx] = CallInst::Create(dummy_function);
            newDRLEntry.dummyVals[i] = retValues[i + destIdx];
        }

        // Copy the data into DRL structure
        m_DRL.push_back(newDRLEntry);
    }
    else
    {
        V_PRINT(scalarizer,
            "\t\t\tCreating scalar conversion for " << origValue->getName() << "\n");
        // Value is an Instruction/global/function argument, and was not converted to scalars yet.
        // Create scalar values (break down the vector) and place in SCM:
        //   %scalar0 = extractelement <4 x Type> %vector, i32 0
        //   %scalar1 = extractelement <4 x Type> %vector, i32 1
        //   %scalar2 = extractelement <4 x Type> %vector, i32 2
        //   %scalar3 = extractelement <4 x Type> %vector, i32 3
        // The breaking instructions will be placed the the head of the function, or right
        // after the instruction (if it is an instruction)
        Instruction* locationInst = &*(inst_begin(m_currFunc));
        Instruction* origInstruction = dyn_cast<Instruction>(origValue);
        if (origInstruction)
        {
            BasicBlock::iterator insertLocation(origInstruction);
            ++insertLocation;
            locationInst = &(*insertLocation);
            // If the insert location is PHI, move the insert location to after all PHIs is the block
            if (isa<PHINode>(locationInst))
            {
                locationInst = locationInst->getParent()->getFirstNonPHI();
            }
        }

        // Generate extractElement instructions
        for (unsigned i = 0; i < width; ++i)
        {
            Value* constIndex = ConstantInt::get(Type::getInt32Ty(context()), i);
            retValues[i + destIdx] = ExtractElementInst::Create(origValue, constIndex,
                VALUE_NAME(origValue->getName() + ".scalar"), locationInst);
        }
        SCMEntry* newEntry = getSCMEntry(origValue);
        updateSCMEntryWithValues(newEntry, &(retValues[destIdx]), origValue, false);

    }
}

void ScalarizeFunction::obtainVectorValueWhichMightBeScalarized(Value* vectorVal)
{
    m_usedVectors.insert(vectorVal);
}

void ScalarizeFunction::resolveVectorValues()
{
    SmallSetVector<Value*, ESTIMATED_INST_NUM>::iterator it = m_usedVectors.begin();
    SmallSetVector<Value*, ESTIMATED_INST_NUM>::iterator e = m_usedVectors.end();
    for (; it != e; ++it) {
        obtainVectorValueWhichMightBeScalarizedImpl(*it);
    }
}

void ScalarizeFunction::obtainVectorValueWhichMightBeScalarizedImpl(Value* vectorVal)
{
    IGC_ASSERT_MESSAGE(isa<VectorType>(vectorVal->getType()), "Must be a vector type");
    if (isa<UndefValue>(vectorVal)) return;

    // ONLY IF the value appears in the SCM - there is a chance it was removed.
    if (!m_SCM.count(vectorVal)) return;
    SCMEntry* valueEntry = m_SCM[vectorVal];

    // Check in SCM entry, if value was really removed
    if (false == valueEntry->isOriginalVectorRemoved) return;

    V_PRINT(scalarizer, "\t\t\tTrying to use a removed value. Reassembling it...\n");
    // The vector value was removed. Need to reassemble it...
    //   %assembled.vect.0 = insertelement <4 x type> undef             , type %scalar.0, i32 0
    //   %assembled.vect.1 = insertelement <4 x type> %indx.vect.0, type %scalar.1, i32 1
    //   %assembled.vect.2 = insertelement <4 x type> %indx.vect.1, type %scalar.2, i32 2
    //   %assembled.vect.3 = insertelement <4 x type> %indx.vect.2, type %scalar.3, i32 3
    // Place the re-assembly in the location where the original instruction was
    Instruction* vectorInst = dyn_cast<Instruction>(vectorVal);
    IGC_ASSERT_MESSAGE(vectorInst, "SCM reports a non-instruction was removed. Should not happen");
    Instruction* insertLocation = vectorInst;
    // If the original instruction was PHI, place the re-assembly only after all PHIs is the block
    if (isa<PHINode>(vectorInst))
    {
        insertLocation = insertLocation->getParent()->getFirstNonPHI();
    }

    Value* assembledVector = UndefValue::get(vectorVal->getType());
    unsigned width = int_cast<unsigned>(dyn_cast<IGCLLVM::FixedVectorType>(vectorVal->getType())->getNumElements());
    for (unsigned i = 0; i < width; i++)
    {
        IGC_ASSERT_MESSAGE(NULL != valueEntry->scalarValues[i], "SCM entry has NULL value");
        Value* constIndex = ConstantInt::get(Type::getInt32Ty(context()), i);
        Instruction* insert = InsertElementInst::Create(assembledVector,
            valueEntry->scalarValues[i], constIndex,
            VALUE_NAME(vectorVal->getName() + ".assembled.vect"), insertLocation);
        VectorizerUtils::SetDebugLocBy(insert, vectorInst);
        assembledVector = insert;
        V_PRINT(scalarizer,
            "\t\t\tCreated vector assembly inst:" << *assembledVector << "\n");
    }
    // Replace the uses of "vectorVal" with the new vector
    vectorVal->replaceAllUsesWith(assembledVector);

    // create SCM entry to represent the new vector value..
    SCMEntry* newEntry = getSCMEntry(assembledVector);
    updateSCMEntryWithValues(newEntry, &(valueEntry->scalarValues[0]), assembledVector, false);
}

ScalarizeFunction::SCMEntry* ScalarizeFunction::getSCMEntry(Value* origValue)
{
    // origValue may be scalar or vector:
    // When the actual returned value of the CALL inst is different from the The "proper" retval
    // the original CALL inst value may be scalar (i.e. int2 is converted to double which is a scalar)
    IGC_ASSERT_MESSAGE(!isa<UndefValue>(origValue), "Trying to create SCM to undef value...");
    if (m_SCM.count(origValue)) return m_SCM[origValue];

    // If index of next free SCMEntry overflows the array size, create a new array
    if (m_SCMArrayLocation == ESTIMATED_INST_NUM)
    {
        // Create new SCMAllocationArray, push it to the vector of arrays, and set free index to 0
        m_SCMAllocationArray = new SCMEntry[ESTIMATED_INST_NUM];
        m_SCMArrays.push_back(m_SCMAllocationArray);
        m_SCMArrayLocation = 0;
    }
    // Allocate the new entry, and increment the free-element index
    SCMEntry* newEntry = &(m_SCMAllocationArray[m_SCMArrayLocation++]);

    // Set all primary data in entry
    if (newEntry->scalarValues.size())
        newEntry->scalarValues[0] = NULL;
    else
        newEntry->scalarValues.push_back(NULL);

    newEntry->isOriginalVectorRemoved = false;

    // Insert new entry to SCM map
    m_SCM.insert(std::pair<Value*, SCMEntry*>(origValue, newEntry));

    return newEntry;
}

void ScalarizeFunction::updateSCMEntryWithValues(ScalarizeFunction::SCMEntry* entry,
    Value* scalarValues[],
    const Value* origValue,
    bool isOrigValueRemoved,
    bool matchDbgLoc)
{
    IGC_ASSERT_MESSAGE((origValue->getType()->isArrayTy() || origValue->getType()->isVectorTy()), "only Vector values are supported");
    unsigned width = int_cast<unsigned>(dyn_cast<IGCLLVM::FixedVectorType>(origValue->getType())->getNumElements());

    entry->isOriginalVectorRemoved = isOrigValueRemoved;

    entry->scalarValues.resize(width);

    for (unsigned i = 0; i < width; ++i)
    {
        IGC_ASSERT_MESSAGE(NULL != scalarValues[i], "Trying to fill SCM with NULL value");
        entry->scalarValues[i] = scalarValues[i];
    }

    if (matchDbgLoc)
    {
        if (const Instruction* origInst = dyn_cast<Instruction>(origValue))
        {
            for (unsigned i = 0; i < width; ++i)
            {
                Instruction* scalarInst = dyn_cast<Instruction>(scalarValues[i]);
                if (scalarInst) VectorizerUtils::SetDebugLocBy(scalarInst, origInst);
            }
        }
    }
}

ScalarizeFunction::SCMEntry* ScalarizeFunction::getScalarizedValues(Value* origValue)
{
    if (m_SCM.count(origValue)) return m_SCM[origValue];
    return NULL;
}

void ScalarizeFunction::releaseAllSCMEntries()
{
    IGC_ASSERT_MESSAGE(m_SCMArrays.size() > 0, "At least one buffer is allocated at all times");
    while (m_SCMArrays.size() > 1)
    {
        // If there are additional allocated entry Arrays, release all of them (leave only the first)
        SCMEntry* popEntry = m_SCMArrays.pop_back_val();
        delete[] popEntry;
    }
    // set the "current" array pointer to the only remaining array
    m_SCMAllocationArray = m_SCMArrays[0];
    m_SCMArrayLocation = 0;
}

void ScalarizeFunction::resolveDeferredInstructions()
{
    llvm::MapVector<Value*, Value*> dummyToScalarMap;

    // lambda to check if a value is a dummy instruction
    auto isDummyValue = [this](Value* val)
        {
            auto* call = dyn_cast<CallInst>(val);
            if (!call) return false;
            // If the Value is one of the dummy functions that we created.
            for (const auto& function : createdDummyFunctions) {
                if (call->getCalledFunction() == function.second)
                    return true;
            }

            return false;
        };

    for (auto deferredEntry = m_DRL.begin(); m_DRL.size() > 0;)
    {
        DRLEntry current = *deferredEntry;
        V_PRINT(scalarizer,
            "\tDRL Going to fix value of orig inst: " << *current.unresolvedInst << "\n");
        Instruction* vectorInst = dyn_cast<Instruction>(current.unresolvedInst);
        IGC_ASSERT_MESSAGE(vectorInst, "DRL only handles unresolved instructions");

        IGCLLVM::FixedVectorType* currType = dyn_cast<IGCLLVM::FixedVectorType>(vectorInst->getType());
        IGC_ASSERT_MESSAGE(currType, "Cannot have DRL of non-vector value");
        unsigned width = int_cast<unsigned>(currType->getNumElements());

        SCMEntry* currentInstEntry = getSCMEntry(vectorInst);

        if (currentInstEntry->scalarValues[0] == NULL)
        {
            V_PRINT(scalarizer, "\t\tInst was not scalarized yet, Scalarizing now...\n");
            SmallVector<Value*, MAX_INPUT_VECTOR_WIDTH>newInsts;

            // This instruction was not scalarized. Create scalar values and place in SCM.
            //   %scalar0 = extractelement <4 x Type> %vector, i32 0
            //   %scalar1 = extractelement <4 x Type> %vector, i32 1
            //   %scalar2 = extractelement <4 x Type> %vector, i32 2
            //   %scalar3 = extractelement <4 x Type> %vector, i32 3
            // Place the vector break-down instructions right after the actual vector
            BasicBlock::iterator insertLocation(vectorInst);
            ++insertLocation;
            // If the insert location is PHI, move the insert location to after all PHIs is the block
            if (isa<PHINode>(insertLocation))
            {
                insertLocation = BasicBlock::iterator(insertLocation->getParent()->getFirstNonPHI());
            }

            newInsts.resize(width);
            for (unsigned i = 0; i < width; i++)
            {
                Value* constIndex = ConstantInt::get(Type::getInt32Ty(context()), i);
                Instruction* EE = ExtractElementInst::Create(vectorInst, constIndex,
                    VALUE_NAME(vectorInst->getName() + ".scalar"), &(*insertLocation));
                newInsts[i] = EE;
            }
            updateSCMEntryWithValues(currentInstEntry, &(newInsts[0]), vectorInst, false);
        }

        bool totallyResolved = true;

        // Connect the resolved values to their consumers
        for (unsigned i = 0; i < width; ++i)
        {
            Instruction* dummyInst = dyn_cast<Instruction>(current.dummyVals[i]);
            IGC_ASSERT_MESSAGE(dummyInst, "Dummy values are all instructions!");
            Value* scalarVal = currentInstEntry->scalarValues[i];

            if (isDummyValue(scalarVal))
            {
                // It's possible the scalar values are not resolved earlier and are themselves dummy instructions.
                // In order to find the real value, we look in the map to see which value replaced it.
                if (dummyToScalarMap.count(scalarVal))
                    scalarVal = dummyToScalarMap[scalarVal];
                else
                    totallyResolved = false;
            }

            // Save every dummy instruction with the scalar value its replaced with
            dummyToScalarMap[dummyInst] = scalarVal;
        }

        if (totallyResolved)
        {
            m_DRL.erase(deferredEntry);
        }
        else
        {
            deferredEntry++;
        }

        if (deferredEntry == m_DRL.end())
        {
            deferredEntry = m_DRL.begin();
        }
    }

    for (const auto& entry : dummyToScalarMap)
    {
        // Replace and erase all dummy instructions (don't use eraseFromParent as the dummy is not in the function)
        Instruction* dummyInst = cast<Instruction>(entry.first);
        dummyInst->replaceAllUsesWith(entry.second);
        dummyInst->deleteValue();
    }

    // clear DRL
    m_DRL.clear();
}

extern "C" FunctionPass * createScalarizerPass(IGC::SelectiveScalarizer selectiveMode)
{
    return new ScalarizeFunction(selectiveMode);
}

