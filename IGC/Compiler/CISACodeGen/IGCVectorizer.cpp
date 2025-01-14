/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IGCVectorizer.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include <llvmWrapper/IR/BasicBlock.h>
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
#define PASS_DESCRIPTION2 "Vectorizes scalar path around igc vector intrinsics like dpas"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 false
IGC_INITIALIZE_PASS_BEGIN(IGCVectorizer, PASS_FLAG2, PASS_DESCRIPTION2,
                          PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(IGCVectorizer, PASS_FLAG2, PASS_DESCRIPTION2,
                        PASS_CFG_ONLY2, PASS_ANALYSIS2)

#define OutputLogStreamM OutputLogStream
#define DEBUG IGC_IS_FLAG_ENABLED(VectorizerLog)
#define PRINT_LOG(Str) if (DEBUG) { OutputLogStreamM << Str; writeLog(); }
#define PRINT_LOG_NL(Str) if (DEBUG) { OutputLogStreamM << Str << "\n"; writeLog(); }
#define PRINT_INST(I) if (DEBUG) { I->print(OutputLogStreamM, false); }
#define PRINT_INST_NL(I) if (DEBUG) { if (I) { I->print(OutputLogStreamM, false); } else { PRINT_LOG("NULL"); } OutputLogStreamM << "\n"; }
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

unsigned int getVectorSize(Value *I) {
    IGCLLVM::FixedVectorType *VecType =
        llvm::dyn_cast<IGCLLVM::FixedVectorType>(I->getType());
    if (!VecType)
        return 0;
    unsigned int NumElements = VecType->getNumElements();
    return NumElements;
}


bool isBinarySafe(Instruction *I) {

    bool Result = false;
    auto* Binary = llvm::dyn_cast<BinaryOperator>(I);
    if (Binary) {
        auto OpCode = Binary->getOpcode();
        Result  |=  OpCode == Instruction::FMul;
    }
    return Result;
}

bool isSafeToVectorize(Instruction *I) {
    // this is a very limited approach for vectorizing but it's safe
    bool Result =
        llvm::isa<PHINode>(I) ||
        llvm::isa<ExtractElementInst>(I) ||
        llvm::isa<InsertElementInst>(I) ||
        llvm::isa<FPTruncInst>(I) ||
        isBinarySafe(I);

    return Result;
}

bool IGCVectorizer::handlePHI(VecArr &Slice, Type *VectorType) {
    PHINode *ScalarPhi = static_cast<PHINode *>(Slice[0]);

    if (!checkPHI(ScalarPhi, Slice))
        return false;

    PHINode *Phi = PHINode::Create(VectorType, 2);
    Phi->setName("vectorized_phi");

    VecVal Operands;
    for (auto& BB : ScalarPhi->blocks()) {

        std::vector<Constant*> Elements;
        VecArr ForVector;
        bool IsConstOperand = true;
        bool IsInstOperand = true;
        bool IsVectorized = true;
        for (auto& El : Slice) {

            PHINode *Phi = static_cast<PHINode *>(El);
            Value *Val = Phi->getIncomingValueForBlock(BB);
            Value *ValCmp = ScalarPhi->getIncomingValueForBlock(BB);

            PRINT_INST(Val); PRINT_LOG("  &  "); PRINT_INST_NL(ValCmp);

            Constant *Const = llvm::dyn_cast<Constant>(Val);
            Constant *ConstCmp = llvm::dyn_cast<Constant>(ValCmp);
            IsConstOperand &= Const && ConstCmp;
            if (IsConstOperand) { Elements.push_back(Const); }

            Instruction* Inst = llvm::dyn_cast<Instruction>(Val);
            Instruction* InstCmp = llvm::dyn_cast<Instruction>(ValCmp);
            IsInstOperand &= Inst && InstCmp;
            if (IsInstOperand) {
                ForVector.push_back(Inst);
                IsVectorized &= ScalarToVector.count(Inst) && (ScalarToVector[Inst] == ScalarToVector[InstCmp]);
            }

        }

        if (IsConstOperand) {
            PRINT_LOG_NL("ConstOperand");
            auto ConstVec = ConstantVector::get(Elements);
            Operands.push_back(ConstVec);
        }
        else if (IsVectorized)   {
            PRINT_LOG_NL("Vectorized: ");
            auto Vectorized = ScalarToVector[ScalarPhi->getIncomingValueForBlock(BB)];
            PRINT_INST_NL(Vectorized);
            Operands.push_back(Vectorized);
        }
        else if (IsInstOperand)  {
            PRINT_LOG_NL("Created Vector: ");
            auto CreatedVec = createVector(ForVector, ForVector.back()->getNextNonDebugInstruction());
            PRINT_INST_NL(CreatedVec);
            Operands.push_back(CreatedVec);
        }
        else {
            PRINT_LOG_NL("Couldn't create operand array");
            return false;
        }

    }

    for (unsigned int i = 0; i < Operands.size(); ++i) {
        auto BB = ScalarPhi->getIncomingBlock(i);
        Phi->addIncoming(Operands[i], BB);
    }

    Phi->insertBefore(ScalarPhi);
    Phi->setDebugLoc(ScalarPhi->getDebugLoc());
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


InsertElementInst* IGCVectorizer::createVector(VecArr& Slice, Instruction* InsertPoint) {

    llvm::Type* elementType = Slice[0]->getType();
    llvm::VectorType* vectorType = llvm::FixedVectorType::get(elementType, Slice.size());
    llvm::Value* UndefVector = llvm::UndefValue::get(vectorType);
    InsertElementInst* CreatedInsert = nullptr;

    for (size_t i = 0; i < Slice.size(); i++) {
        llvm::Value* index = llvm::ConstantInt::get(llvm::Type::getInt32Ty(M->getContext()), i);
        // we start insert element with under value
        if (CreatedInsert) CreatedInsert = InsertElementInst::Create(CreatedInsert, Slice[i], index);
        else CreatedInsert = InsertElementInst::Create(UndefVector, Slice[i], index);
        CreatedInsert->setName("vector");
        CreatedInsert->setDebugLoc(Slice[i]->getDebugLoc());
        CreatedInsert->insertBefore(InsertPoint);
        CreatedVectorInstructions.push_back(CreatedInsert);
    }

    for (auto &El : Slice)
        ScalarToVector[El] = CreatedInsert;
    return CreatedInsert;
}

bool IGCVectorizer::handleBinaryInstruction(VecArr &Slice) {

    Value *PrevVectorization = nullptr;
    Instruction *First = Slice.front();
    if (ScalarToVector.count(First)) {
        auto Vectorized = ScalarToVector[First];
        if (llvm::isa<InsertElementInst>(Vectorized)) {
            PRINT_LOG_NL("Was sourced by other vector instruction, but wasn't vectorized");
            PrevVectorization = Vectorized;
        }
        else {
            PRINT_LOG_NL("Already was vectorized by other slice");
            return true;
        }
    }
    VecVal Operands;
    for (unsigned int OperNum = 0; OperNum < First->getNumOperands(); ++OperNum) {
        Value* Vectorized = checkOperandsToBeVectorized(First, OperNum, Slice);
        if (Vectorized)
            Operands.push_back(Vectorized);
        else {
            Value* VectorizedOperand = vectorizeSlice(Slice, OperNum);
            if (!VectorizedOperand) { PRINT_LOG_NL("Couldn't vectorize Slice"); return false; }
            Operands.push_back(VectorizedOperand);
        }
    }

    PRINT_DS("Operands: ", Operands);

    auto BinaryOpcode = llvm::cast<BinaryOperator>(First)->getOpcode();

    auto* CreatedInst = BinaryOperator::Create(BinaryOpcode, Operands[0], Operands[1]);
    CreatedInst->setName("vectorized_binary");

    CreatedInst->setDebugLoc(First->getDebugLoc());
    CreatedInst->insertAfter(Slice.back());
    CreatedVectorInstructions.push_back(CreatedInst);

    PRINT_LOG("Binary instruction created: ");
    PRINT_INST_NL(CreatedInst);

    for (auto &el : Slice) {
        if (ScalarToVector.count(el)) {
            PRINT_LOG_NL("Vectorized version already present");
            PRINT_INST(el); PRINT_LOG(" --> "); PRINT_INST_NL(ScalarToVector[el]);
        }
        ScalarToVector[el] = CreatedInst;
    }

    if (PrevVectorization) {
        PRINT_LOG_NL("Replaced with proper vector version");
        PrevVectorization->replaceAllUsesWith(CreatedInst);
    }

    return true;
}

bool IGCVectorizer::handleCastInstruction(VecArr &Slice) {

    Instruction *First = Slice.front();

    unsigned int OperNum = 0;
    Value* Vectorized = checkOperandsToBeVectorized(First, OperNum, Slice);
    if (!Vectorized) Vectorized = vectorizeSlice(Slice, OperNum);

    auto VectorSize = getVectorSize((Instruction* )Vectorized);
    auto Type = IGCLLVM::FixedVectorType::get(First->getType(), VectorSize);
    auto CastOpcode = llvm::cast<CastInst>(First)->getOpcode();

    CastInst* CreatedCast = CastInst::Create(CastOpcode, Vectorized, Type);
    CreatedCast->setName("vectorized_cast");

    CreatedCast->setDebugLoc(First->getDebugLoc());
    CreatedCast->insertBefore(First);
    CreatedVectorInstructions.push_back(CreatedCast);

    PRINT_LOG("Cast instruction created: ");
    PRINT_INST_NL(CreatedCast);

    for (auto &el : Slice)
        ScalarToVector[el] = CreatedCast;

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
    std::reverse(InSt.SlChain.begin(), InSt.SlChain.end());

    for (auto &SliceSt : InSt.SlChain) {
        PRINT_LOG_NL("");
        PRINT_LOG_NL("Process slice: ");
        VecArr& Slice = SliceSt.Vector;
        PRINT_DS("Slice: ", Slice);

        // this contains common checks for any slice
        if (!checkSlice(Slice, InSt))
            return false;

        Instruction *First = Slice[0];
        if (llvm::isa<PHINode>(First)) {
            if (!handlePHI(Slice, InSt.Final->getType())) return false;
        } else if (llvm::isa<CastInst>(First)) {
            if (!handleCastInstruction(Slice)) return false;
        } else if (llvm::isa<BinaryOperator>(First)) {
            if (!handleBinaryInstruction(Slice)) return false;
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

void IGCVectorizer::printSlice(Slice* S) {

    PRINT_LOG_NL("Slice: [ " << S << " ]");
    PRINT_LOG_NL("OpNum: " << S->OpNum);
    PRINT_LOG_NL("Parent: " << S->Parent);
    PRINT_DS("Slice: ", S->Vector);
}

void IGCVectorizer::buildTree(VecArr &V, VecOfSlices& Chain) {

    std::unordered_set<llvm::Instruction *> Explored;
    std::queue<Slice*> BFSQ;

    Chain.push_back({ 0, V, nullptr});
    Slice* Root = &Chain.back();
    BFSQ.push(Root);

    while (!BFSQ.empty()) {
        Slice *CurSlice = BFSQ.front();
        auto First = CurSlice->Vector.front();
        BFSQ.pop();

        PRINT_LOG_NL(""); PRINT_LOG("Start: "); PRINT_INST_NL(First);
        for (unsigned int OpNum = 0; OpNum < First->getNumOperands(); ++OpNum) {

            PRINT_LOG("Operand [" << OpNum << "]:  ");
            Instruction *Cmp = llvm::dyn_cast<Instruction>(First->getOperand(OpNum));
            bool IsSame = true;
            if (!Cmp) { IsSame = false; PRINT_LOG_NL("Not an instruction"); continue; }
            PRINT_LOG("First: "); PRINT_INST_NL(Cmp);
            if (!isSafeToVectorize(Cmp)) { PRINT_LOG_NL(" Not safe to vectorize "); IsSame = false; continue; }

            VecArr LocalVector;

            for (auto &El : CurSlice->Vector) {
                auto Operand = llvm::dyn_cast<Instruction>(El->getOperand(OpNum));

                if (!Operand) { IsSame = false; break; }

                bool IsExplored = Explored.count(Operand);
                if (IsExplored) { IsSame = false; break; }
                Explored.insert(Operand);

                IsSame &= Cmp->isSameOperationAs(Operand, false);
                if (!IsSame) break;
                LocalVector.push_back(Operand);
            }

            PRINT_DS("   check: ", LocalVector);
            if (IsSame) {
                PRINT_LOG_NL("Pushed");
                Chain.push_back({ OpNum, LocalVector, CurSlice});
                BFSQ.push(&Chain.back());
            }
        }
    }
}

bool IGCVectorizer::checkPHI(Instruction *Compare, VecArr &Slice) {
    PHINode *ComparePHI = static_cast<PHINode *>(Slice[0]);
    if (ComparePHI->getNumIncomingValues() != 2) {
        PRINT_LOG_NL("Only 2-way phi supported");
        return false;
    }
    BasicBlock* CmpBB = Compare->getParent();
    for (auto Phi : Slice) {
        if (CmpBB != Phi->getParent()) {
            PRINT_LOG_NL(" Only phi's from the same BB are supported");
            return false;
        }
    }
    return true;
}

Value* IGCVectorizer::vectorizeSlice(VecArr& Slice, unsigned int OperNum) {

    VecArr NotVectorizedInstruction;
    VecConst ConstNotVectorized;
    Value* NewVector = nullptr;

    for (auto &El : Slice) {
        Value *Val = El->getOperand(OperNum);
        PRINT_INST(El); PRINT_LOG(" --> " ); PRINT_INST_NL(Val);
        auto Inst = llvm::dyn_cast<Instruction>(Val);
        if (Inst) { NotVectorizedInstruction.push_back(Inst); continue; }
        auto Const = llvm::dyn_cast<Constant>(Val);
        if (Const) { ConstNotVectorized.push_back(Const); continue; }
    }

    if (ConstNotVectorized.size() == Slice.size()) {
        NewVector = ConstantVector::get(ConstNotVectorized);
        PRINT_LOG("New vector created: "); PRINT_INST_NL(NewVector);
    }

    if (NotVectorizedInstruction.size() == Slice.size()) {
        NewVector = createVector(NotVectorizedInstruction, NotVectorizedInstruction.back()->getNextNonDebugInstruction());
        PRINT_LOG("New vector created: "); PRINT_INST_NL(NewVector);
    }
    return NewVector;
}

Value* IGCVectorizer::checkOperandsToBeVectorized(Instruction *First, unsigned int OperNum, VecArr &Slice) {

    Value *Compare = ScalarToVector[First->getOperand(OperNum)];
    if (!Compare) {
        PRINT_LOG_NL(" Operand num: " << OperNum << " is not vectorized");
        return nullptr;
    }
    for (auto &El : Slice) {
        Value *Val = El->getOperand(OperNum);
        Value *ValCompare = ScalarToVector[Val];
        if (ValCompare != Compare) {
            PRINT_LOG("Compare: "); PRINT_INST_NL(Compare);
            PRINT_LOG("ValCompare: "); PRINT_INST_NL(ValCompare);
            PRINT_LOG_NL("Operands in slice do not converge");
            return nullptr;
        }
    }
    return Compare;
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
    if (!checkOperandsToBeVectorized(First, 1, Slice))
        return false;
    return true;
}

bool IGCVectorizer::checkExtractElement(Instruction *Compare, VecArr &Slice) {
    Value *CompareSource = Slice[0]->getOperand(0);

    if (getVectorSize(CompareSource) != Slice.size()) {
        PRINT_LOG_NL("Extract is wider than the slice, need additional handling, not implemented");
        return false;
    }

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
    M = F.getParent();
    CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    initializeLogFile(F);

    VecArr ToProcess;
    // we collect operands that seem promising for vectorization
    collectInstructionToProcess(ToProcess, F);
    PRINT_DS("Seed: ", ToProcess);
    PRINT_LOG_NL("\n\n");

    writeLog();

    for (unsigned int Ind = 0; Ind < ToProcess.size(); ++Ind) {

        unsigned int Index = IGC_GET_FLAG_VALUE(VectorizerList);
        PRINT_LOG_NL(" Index: " << Index << " Ind: " << Ind);
        if (Index != Ind && Index != -1) continue;

        auto& El = ToProcess[Ind];
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

            buildTree(InSt.Vec, InSt.SlChain);
            PRINT_LOG_NL("Print slices");
            for (auto& Slice : InSt.SlChain) { printSlice(&Slice); writeLog(); }


            CreatedVectorInstructions.clear();
            if (!processChain(InSt)) {
                writeLog();
                std::reverse(CreatedVectorInstructions.begin(), CreatedVectorInstructions.end());
                PRINT_DS("To Clean: ", CreatedVectorInstructions);
                for (auto& el : CreatedVectorInstructions) {
                    PRINT_LOG("Cleaned: "); PRINT_INST_NL(el); writeLog();
                    el->eraseFromParent();
                }
                ScalarToVector.clear();
            }
            else {
                for (auto& el : CreatedVectorInstructions) { PRINT_LOG("Created: "); PRINT_INST_NL(el); writeLog(); }
            }
            writeLog();
        }

        PRINT_LOG("\n\n");
    }

    writeLog();

    return true;
}
