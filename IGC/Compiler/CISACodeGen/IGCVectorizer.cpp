/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IGCVectorizer.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include <algorithm>

//
// IGCVectorizer pass currently looks for insert elements instructions
// that are going inside LSC2DBlockWrite & sub_group_dpas
// intrinsics and vectorizes phi nodes and eliminates
// unnecessary insert/extract element operations
//
// BEFORE:
// %phi_a = phi %extr_a
// %phi_b = phi %extr_b
// %dpas_vec = insert element %phi_a
// %dpas_vec = insert element %phi_b
// %dpas_res = dpas (%dpas_vec ...)
// %extr_a = extract element %dpas_res
// %extr_b = extrat elelment %dpas_res
// end of BB
//
// %a = phi %extr_a
// %b = phi %extr_b
// %vec = insert element %a
// %vec = insert element %b
// lsc_block_write (%vec ...)
// end of BB
//
// AFTER:
// %phi_vec  = phi 2xfloat %dpas_res
// %dpas_res = dpas (%phi_vec ...)
// end of BB
//
// %phi_vec_2 = phi 2xfloat %dpas_res
// lsc_block_write (%phi_vec_2 ...)
// end of BB
//
// we vectorize PHI & scatter/gather pairs to eliminate scalar path between
// inherently vector intrinsics
//
// the backbone of the optimization is a vector_slice_tree (VectorSliceChain):
// each slice is a vector with index matching position of a scalar value
// inside the final vector:
// using strict ordering we can check that data inside final vector matches
// the data of the original vector element
//
//  example 4 elements for compactness:
//  [ 0       1       2        3     ]
//  [ tmp104  tmp105  tmp106  tmp107 ]
//  [ tmp90   tmp91   tmp92   tmp93  ]
//  [ tmp114  tmp115  tmp116  tmp117 ]
//
// Slice:
// -->   %tmp104 = insertelement <8 x float> zeroinitializer, float %tmp90, i64 0
// -->   %tmp105 = insertelement <8 x float> %tmp104, float %tmp91, i64 1
// -->   %tmp106 = insertelement <8 x float> %tmp105, float %tmp92, i64 2
// -->   %tmp107 = insertelement <8 x float> %tmp106, float %tmp93, i64 3
// Slice:
// -->   %tmp90 = phi float [ 0.000000e+00, %bb60 ], [ %tmp114, %bb88 ]
// -->   %tmp91 = phi float [ 0.000000e+00, %bb60 ], [ %tmp115, %bb88 ]
// -->   %tmp92 = phi float [ 0.000000e+00, %bb60 ], [ %tmp116, %bb88 ]
// -->   %tmp93 = phi float [ 0.000000e+00, %bb60 ], [ %tmp117, %bb88 ]
// Slice:
// -->   %tmp114 = extractelement <8 x float> %tmp113, i64 0
// -->   %tmp115 = extractelement <8 x float> %tmp113, i64 1
// -->   %tmp116 = extractelement <8 x float> %tmp113, i64 2
// -->   %tmp117 = extractelement <8 x float> %tmp113, i64 3
//
// if you have to make sense of what is happening I RECOMMEND YOU
// to check the logs: IGC_DumpToCustomDir=Dump IGC_ShaderDumpEnable=1 IGC_VectorizerLog=1
// they will be present inside the Dump folder


char IGCVectorizer::ID = 0;

#define PASS_FLAG2 "igc-vectorizer"
#define PASS_DESCRIPTION2 "prints register pressure estimation"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 false
IGC_INITIALIZE_PASS_BEGIN(IGCVectorizer, PASS_FLAG2, PASS_DESCRIPTION2,
                          PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(IGCVectorizer, PASS_FLAG2, PASS_DESCRIPTION2,
                        PASS_CFG_ONLY2, PASS_ANALYSIS2)

#define DEBUG IGC_IS_FLAG_ENABLED(VectorizerLog)
#define PRINT_LOG(Str) if (DEBUG) OutputLogStream << Str;
#define PRINT_LOG_NL(Str) if (DEBUG) OutputLogStream << Str << "\n";
#define PRINT_INST(I) if (DEBUG) { I->print(OutputLogStream, false); }
#define PRINT_INST_NL(I) if (DEBUG) { I->print(OutputLogStream, false); OutputLogStream << "\n"; }
#define PRINT_DS(Str, DS) if (DEBUG) { for (auto DS_EL : DS) { { PRINT_LOG(Str); } { PRINT_INST_NL(DS_EL); } } }

IGCVectorizer::IGCVectorizer() : FunctionPass(ID) {
    initializeIGCVectorizerPass(*PassRegistry::getPassRegistry());
};

void IGCVectorizer::writeLog() {
    if (IGC_IS_FLAG_ENABLED(VectorizerLog) && OutputLogFile->is_open())
        *OutputLogFile << OutputLogStream.str();

    OutputLogStream.str().clear();
}

void IGCVectorizer::initializeLogFile(Function &F) {
    if (!IGC_IS_FLAG_ENABLED(VectorizerLog))
        return;

    std::stringstream ss;
    ss << F.getName().str() << "_"
       << "Vectorizer";
    auto Name = Debug::DumpName(IGC::Debug::GetShaderOutputName())
                    .Hash(CGCtx->hash)
                    .Type(CGCtx->type)
                    .Retry(CGCtx->m_retryManager.GetRetryId())
                    .Pass(ss.str().c_str())
                    .Extension("ll");

    OutputLogFile = std::make_unique<std::ofstream>(Name.str());
}

void IGCVectorizer::findInsertElementsInDataFlow(llvm::Instruction *I,
                                                 VecArr &Chain) {
    std::queue<llvm::Instruction *> BFSQ;
    BFSQ.push(I);
    std::unordered_set<llvm::Instruction *> Explored;

    Chain.push_back(I);
    if (llvm::isa<InsertElementInst>(I))
        return;

    while (!BFSQ.empty()) {
        llvm::Instruction *CurrI = BFSQ.front();
        BFSQ.pop();
        for (unsigned int i = 0; i < CurrI->getNumOperands(); ++i) {
            Instruction *Op = llvm::dyn_cast<Instruction>(CurrI->getOperand(i));
            if (!Op)
                continue;

            bool IsConstant = llvm::isa<llvm::Constant>(Op);
            bool IsExplored = Explored.count(Op);
            bool IsInsertElement = llvm::isa<InsertElementInst>(Op);

            if (IsInsertElement)
                Chain.push_back(Op);

            bool Skip = IsConstant || IsExplored || IsInsertElement;
            if (Skip)
                continue;

            Chain.push_back(Op);
            Explored.insert(Op);
            BFSQ.push(Op);
        }
    }
}

unsigned int getConstantValueAsInt(Value *I) {
    ConstantInt *Value = static_cast<ConstantInt *>(I);
    unsigned int Result = Value->getSExtValue();
    return Result;
}

unsigned int getVectorSize(Instruction *I) {
    IGCLLVM::FixedVectorType *VecType =
        llvm::dyn_cast<IGCLLVM::FixedVectorType>(I->getType());
    if (!VecType)
        return 0;
    unsigned int NumElements = VecType->getNumElements();
    return NumElements;
}

bool isSafeToVectorize(Instruction *I) {
    // this is a very limited approach for vectorizing
    // but it's safe
    bool Result = llvm::isa<PHINode>(I) || llvm::isa<ExtractElementInst>(I) ||
                  llvm::isa<InsertElementInst>(I);

    return Result;
}

bool IGCVectorizer::compareOperands(Value *A, Value *B) {
    Constant *ConstA = llvm::dyn_cast<Constant>(A);
    Constant *ConstB = llvm::dyn_cast<Constant>(B);

    Instruction *InstA = llvm::dyn_cast<Instruction>(A);
    Instruction *InstB = llvm::dyn_cast<Instruction>(B);

    if (ConstA && ConstB) {
        bool BothZero = ConstA->isZeroValue() && ConstB->isZeroValue();
        BothZero &= !(ConstA->isNegativeZeroValue() || ConstB->isNegativeZeroValue());
        return BothZero;
    } else if (InstA && InstB) {
        if (!ScalarToVector.count(InstA)) {
            PRINT_LOG_NL("some elements weren't even vectorized");
            return false;
        }
        bool Same = ScalarToVector[InstA] == ScalarToVector[InstB];
        return Same;
    }
    return false;
}

bool IGCVectorizer::handlePHI(VecArr &Slice, Type *VectorType) {
    PHINode *ScalarPhi = static_cast<PHINode *>(Slice[0]);

    if (!checkPHI(ScalarPhi, Slice))
        return false;

    llvm::Constant *zeroInitializer =
        llvm::ConstantAggregateZero::get(VectorType);
    PHINode *Phi = PHINode::Create(VectorType, 2);
    Phi->setName("vectorized_phi");

    for (auto& BB : ScalarPhi->blocks()) {
        Value *Val = ScalarPhi->getIncomingValueForBlock(BB);
        Constant *Const = llvm::dyn_cast<Constant>(Val);
        Instruction *Inst = llvm::dyn_cast<Instruction>(Val);

        if (Const && Const->isZeroValue())
            Phi->addIncoming(zeroInitializer, BB);
        else if (Inst)
            Phi->addIncoming(ScalarToVector[Inst], BB);
        else {
            PRINT_LOG_NL("malformed PHI, no vectorization");
            return false;
        }
    }

    auto BB = ScalarPhi->getParent();
    Phi->setDebugLoc(ScalarPhi->getDebugLoc());
    BB->getInstList().insert(BB->begin(), Phi);
    CreatedVectorInstructions.push_back(Phi);

    for (auto &El : Slice)
        ScalarToVector[El] = Phi;
    PRINT_LOG("PHI created: ");
    PRINT_INST_NL(Phi);
    return true;
}

bool IGCVectorizer::handleInsertElement(VecArr &Slice, Instruction* Final) {
    Instruction *First = Slice.front();
    if (!checkInsertElement(First, Slice))
        return false;

    // we can handle case with more than 1 value
    // but it wasn't tested
    if (!Final->hasOneUse())
        return false;

    PRINT_LOG_NL("InsertElement substituted with vectorized instruction");
    Value *Compare = ScalarToVector[First->getOperand(1)];
    *(Final->use_begin()) = Compare;
    return true;
}

// this basicaly seeds the chain
bool IGCVectorizer::handleExtractElement(VecArr &Slice) {
    Instruction *First = Slice.front();
    if (!checkExtractElement(First, Slice))
        return false;

    Value *Source = First->getOperand(0);
    for (auto &el : Slice)
        ScalarToVector[el] = Source;
    return true;
}

bool IGCVectorizer::processChain(InsertStruct &InSt) {
    std::reverse(InSt.Chain.begin(), InSt.Chain.end());

    for (auto &Slice : InSt.Chain) {
        PRINT_LOG_NL("Process slice: ");
        PRINT_DS("Slice: ", Slice);

        // this contains common checks for any slice
        if (!checkSlice(Slice, InSt))
            return false;

        Instruction *First = Slice[0];
        if (llvm::isa<PHINode>(First)) {
            if (!handlePHI(Slice, InSt.Final->getType())) return false;
        } else if (llvm::isa<ExtractElementInst>(First)) {
            if (!handleExtractElement(Slice)) return false;
        } else if (llvm::isa<InsertElementInst>(First)) {
            if (!handleInsertElement(Slice, InSt.Final)) return false;
        } else {
            IGC_ASSERT("we should not be here");
        }
    }
    return true;
}

void IGCVectorizer::clusterInsertElement(
    InsertElementInst* HeadInsertEl, InsertStruct &InSt) {
    InSt.Final = HeadInsertEl;
    Instruction *Head = HeadInsertEl;

    while (true) {
        InSt.Vec.push_back(Head);
        Head = llvm::dyn_cast<Instruction>(Head->getOperand(0));
        if (!Head)
            break;
        if (!llvm::isa<InsertElementInst>(Head))
            break;
    }

    // purely convenience feature I want first insert to be at 0 index in
    // array
    std::reverse(InSt.Vec.begin(), InSt.Vec.end());

    PRINT_LOG("fin: ");
    PRINT_INST_NL(InSt.Final);
    PRINT_DS("vec: ", InSt.Vec);
    PRINT_LOG_NL("--------------------------");
}

void IGCVectorizer::collectScalarPath(VecArr &V, VectorSliceChain &Chain) {
    typedef std::pair<Instruction *, unsigned int> Pair;
    std::queue<Pair> BFSQ;

    Chain.push_back({});
    for (auto &Insert : V) {
        BFSQ.push({Insert, 0});
        Chain[0].push_back(Insert);
    }
    std::unordered_set<llvm::Instruction *> Explored;

    while (!BFSQ.empty()) {
        llvm::Instruction *CurrI = BFSQ.front().first;
        unsigned int Level = BFSQ.front().second;
        BFSQ.pop();

        for (unsigned int i = 0; i < CurrI->getNumOperands(); ++i) {
            Instruction *Op = llvm::dyn_cast<Instruction>(CurrI->getOperand(i));
            if (!Op)
                continue;

            bool IsConstant = llvm::isa<llvm::Constant>(Op);
            bool IsExplored = Explored.count(Op);
            bool IsVector = Op->getType()->isVectorTy();
            bool IsNotSafeToVectorize = !isSafeToVectorize(Op);
            bool IsExtractEl = llvm::isa<llvm::ExtractElementInst>(Op);

            if (IsExtractEl) {
                if (Chain.size() <= (Level + 1))
                    Chain.push_back({});
                Chain[Level + 1].push_back(Op);
                InstructionToSlice[Op] = &Chain[Level + 1];
            }

            bool Skip = IsConstant || IsExplored || IsVector || IsExtractEl || IsNotSafeToVectorize;
            if (Skip)
                continue;

            if (Chain.size() <= (Level + 1))
                Chain.push_back({});
            Chain[Level + 1].push_back(Op);
            InstructionToSlice[Op] = &Chain[Level + 1];

            Explored.insert(Op);
            BFSQ.push({Op, Level + 1});
        }
    }
}


bool IGCVectorizer::checkPHI(Instruction *Compare, VecArr &Slice) {
    PHINode *ComparePHI = static_cast<PHINode *>(Slice[0]);
    if (ComparePHI->getNumIncomingValues() != 2) {
        PRINT_LOG_NL("Only 2-way phi supported");
        return false;
    }

    for (auto BB : ComparePHI->blocks()) {
        for (auto &El : Slice) {
            PHINode *ElPHI = static_cast<PHINode *>(El);
            Value *Val = ElPHI->getIncomingValueForBlock(BB);
            Value *CompareVal = ComparePHI->getIncomingValueForBlock(BB);
            if (!compareOperands(CompareVal, Val)) {
                PRINT_LOG_NL("couldn't vectorize PHI, operands do not converge");
                return false;
            }
        }
    }

    for (auto &BB : ComparePHI->blocks()) {
        Value *Val = ComparePHI->getIncomingValueForBlock(BB);
        Instruction *Inst = llvm::dyn_cast<Instruction>(Val);
        if (Inst) {
            if (!ScalarToVector.count(Inst)) {
                PRINT_LOG_NL("can't vectorize, operand hasn't been vectorized");
                return false;
            }
        }
    }
    return true;
}

bool IGCVectorizer::checkInsertElement(Instruction *First, VecArr &Slice) {
    for (unsigned int i = 0; i < Slice.size(); ++i) {
        auto *InsertionIndex = Slice[i]->getOperand(2);
        unsigned int Index = getConstantValueAsInt(InsertionIndex);
        // elements are stored so index of the array
        // corresponds with the way how final data should be laid out
        if (Index != i) {
            PRINT_LOG_NL("Not supported index swizzle");
            return false;
        }
    }

    // we check that all the scalar elements in the slice are
    // already present inside generated vector element
    if (!ScalarToVector.count(First->getOperand(1))) {
        PRINT_LOG_NL("some elements weren't even vectorized");
        return false;
    }
    Value *Compare = ScalarToVector[First->getOperand(1)];
    for (auto &El : Slice) {
        Value *Val = El->getOperand(1);
        Value *ValCompare = ScalarToVector[Val];
        if (ValCompare != Compare) {
            PRINT_LOG("InsertCompare: "); PRINT_INST_NL(Compare);
            PRINT_LOG("InsertVal: "); PRINT_INST_NL(ValCompare);
            PRINT_LOG_NL("Insert Element, operands do not converge");
            return false;
        }
    }
    return true;
}

bool IGCVectorizer::checkExtractElement(Instruction *Compare, VecArr &Slice) {
    Value *CompareSource = Slice[0]->getOperand(0);

    if (!llvm::isa<Instruction>(CompareSource)) {
        PRINT_LOG_NL("Source is not an instruction");
        return false;
    }

    for (unsigned int i = 0; i < Slice.size(); ++i) {
        if (CompareSource != Slice[i]->getOperand(0)) {
            PRINT_LOG_NL("Source operand differ between extract elements");
            return false;
        }
        unsigned int Index = getConstantValueAsInt(Slice[i]->getOperand(1));
        // elements are stored so index of the array
        // corresponds with the way how final data should be laid out
        if (Index != i) {
            PRINT_LOG_NL("Not supported index swizzle");
            return false;
        }
    }
    return true;
}

bool IGCVectorizer::checkSlice(VecArr &Slice, InsertStruct &InSt) {
    if (Slice.size() != getVectorSize(InSt.Final)) {
        PRINT_LOG_NL("vector size isn't equal to the width of the vector tree");
        return false;
    }

    Instruction *Compare = Slice[0];
    if (!isSafeToVectorize(Compare)) {
        PRINT_LOG("instruction in a chain is not supported: ");
        PRINT_INST_NL(Compare);
        return false;
    }

    for (unsigned int i = 1; i < Slice.size(); ++i) {
        if (!Compare->isSameOperationAs(Slice[i])) {
            PRINT_LOG_NL("Not all operations in the slice are identical");
            return false;
        }
    }

    return true;
}

bool filterInstruction(GenIntrinsicInst *I) {
    if (!I)
        return false;

    GenISAIntrinsic::ID ID = I->getIntrinsicID();
    bool Pass = (ID == GenISAIntrinsic::GenISA_LSC2DBlockWrite) ||
                (ID == GenISAIntrinsic::GenISA_sub_group_dpas);

    return Pass;
}

bool hasPotentialToBeVectorized(Instruction *I) {
    bool Result = llvm::isa<InsertElementInst>(I) || llvm::isa<CastInst>(I) || llvm::isa<PHINode>(I);
    return Result;
}

void IGCVectorizer::collectInstructionToProcess(VecArr &ToProcess,
                                                Function &F) {
    for (BasicBlock &BB : F) {
        for (auto &I : BB) {

            GenIntrinsicInst *GenI = llvm::dyn_cast<GenIntrinsicInst>(&I);
            bool Pass = filterInstruction(GenI);
            if (!Pass)
                continue;

            for (unsigned int I = 0; I < GenI->getNumOperands(); ++I) {
                Instruction *Op =
                    llvm::dyn_cast<Instruction>(GenI->getOperand(I));
                if (!Op)
                    continue;
                if (!Op->getType()->isVectorTy())
                    continue;
                if (!hasPotentialToBeVectorized(Op))
                    continue;
                // we collect only vector type arguments to check
                // maybe they were combined from scalar values
                // and could be vectorized
                ToProcess.push_back(Op);
            }
        }
    }
}

bool IGCVectorizer::runOnFunction(llvm::Function &F) {
    CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    initializeLogFile(F);

    VecArr ToProcess;
    // we collect operands that seem promising for vectorization
    collectInstructionToProcess(ToProcess, F);
    PRINT_DS("Seed: ", ToProcess);
    PRINT_LOG_NL("\n\n");

    writeLog();

    for (auto &El : ToProcess) {
        PRINT_LOG("Candidate: ");
        PRINT_INST_NL(El);

        VecArr Chain;
        // we take the collected operands and
        // check if they have insert elements in their
        // data flow, in case they do, we collect those
        findInsertElementsInDataFlow(El, Chain);

        PRINT_DS("Chain: ", Chain);
        PRINT_LOG_NL("--------------------------");

        VecArr VecOfInsert;
        for (auto &El : Chain)
            if (llvm::isa<InsertElementInst>(El))
                VecOfInsert.push_back(El);

        // multiple clusters are supported but not tested hence disabled for now
        // #TODO write a test for multiple clusters
        if (VecOfInsert.empty() || VecOfInsert.size() != 1) {
            PRINT_LOG("Currently we support only 1 insert cluster\n\n");
            continue;
        }

        PRINT_DS("Insert: ", VecOfInsert);
        writeLog();

        // we process collected insert elements into a specific data structure
        // for convenience
        for (auto el : VecOfInsert) {
            InsertStruct InSt;
            clusterInsertElement(static_cast<InsertElementInst*>(el), InSt);
            if (InSt.Vec.size() != getVectorSize(InSt.Final)) {
                PRINT_LOG_NL("partial insert -> rejected");
                continue;
            }
            writeLog();

            collectScalarPath(InSt.Vec, InSt.Chain);
            for (auto &Slice : InSt.Chain) {
                PRINT_LOG_NL("Slice:");
                PRINT_DS("--> ", Slice);
            }
            writeLog();

            CreatedVectorInstructions.clear();
            if (!processChain(InSt)) {
                for (auto& el : CreatedVectorInstructions) {
                    PRINT_LOG("Cleaned: "); PRINT_INST_NL(el);
                    el->eraseFromParent();
                }
            }
            writeLog();
        }

        PRINT_LOG("\n\n");
    }

    writeLog();

    return true;
}
