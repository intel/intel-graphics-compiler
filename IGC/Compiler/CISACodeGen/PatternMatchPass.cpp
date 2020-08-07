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

#include "Compiler/CISACodeGen/PatternMatchPass.hpp"
#include "Compiler/CISACodeGen/EmitVISAPass.hpp"
#include "Compiler/CISACodeGen/DeSSA.hpp"
#include "Compiler/MetaDataApi/IGCMetaDataHelper.h"
#include "common/igc_regkeys.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/PatternMatch.h>
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/InitializePasses.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

char CodeGenPatternMatch::ID = 0;
#define PASS_FLAG "CodeGenPatternMatch"
#define PASS_DESCRIPTION "Does pattern matching"
#define PASS_CFG_ONLY true
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(CodeGenPatternMatch, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(LiveVarsAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(PositionDepAnalysis)
IGC_INITIALIZE_PASS_END(CodeGenPatternMatch, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

namespace IGC
{

    CodeGenPatternMatch::CodeGenPatternMatch() : FunctionPass(ID),
        m_rootIsSubspanUse(false),
        m_blocks(nullptr),
        m_numBlocks(0),
        m_root(nullptr),
        m_currentPattern(nullptr),
        m_Platform(),
        m_AllowContractions(true),
        m_NeedVMask(false),
        m_samplertoRenderTargetEnable(false),
        m_ctx(nullptr),
        DT(nullptr),
        LI(nullptr),
        m_DL(0),
        m_WI(nullptr),
        m_LivenessInfo(nullptr)
    {
        initializeCodeGenPatternMatchPass(*PassRegistry::getPassRegistry());
    }

    CodeGenPatternMatch::~CodeGenPatternMatch()
    {
        delete[] m_blocks;
    }

    void CodeGenPatternMatch::CodeGenNode(llvm::DomTreeNode* node)
    {
        // Process blocks by processing the dominance tree depth first
        for (uint i = 0; i < node->getNumChildren(); i++)
        {
            CodeGenNode(node->getChildren()[i]);
        }
        llvm::BasicBlock* bb = node->getBlock();
        CodeGenBlock(bb);
    }

    bool CodeGenPatternMatch::runOnFunction(llvm::Function& F)
    {
        m_blockMap.clear();
        ConstantPlacement.clear();
        PairOutputMap.clear();
        UniformBools.clear();

        delete[] m_blocks;
        m_blocks = nullptr;

        m_ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

        MetaDataUtils* pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
        ModuleMetaData* modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
        if (pMdUtils->findFunctionsInfoItem(&F) == pMdUtils->end_FunctionsInfo())
        {
            return false;
        }

        m_AllowContractions = true;
        if (m_ctx->m_DriverInfo.NeedCheckContractionAllowed())
        {
            m_AllowContractions =
                modMD->compOpt.FastRelaxedMath ||
                modMD->compOpt.MadEnable;
        }
        m_Platform = m_ctx->platform;

        DT = &getAnalysis<llvm::DominatorTreeWrapperPass>().getDomTree();
        LI = &getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo();
        m_DL = &F.getParent()->getDataLayout();
        m_WI = &getAnalysis<WIAnalysis>();
        m_PosDep = &getAnalysis<PositionDepAnalysis>();
        // pattern match will update liveness held by LiveVar, which needs
        // WIAnalysis result for uniform variable
        m_LivenessInfo = &getAnalysis<LiveVarsAnalysis>().getLiveVars();
        CreateBasicBlocks(&F);
        CodeGenNode(DT->getRootNode());
        return false;
    }

    inline bool HasSideEffect(llvm::Instruction& inst)
    {
        if (inst.mayWriteToMemory() || inst.isTerminator())
        {
            return true;
        }
        return false;
    }


    inline bool HasPhiUse(llvm::Value& inst)
    {
        for (auto UI = inst.user_begin(), E = inst.user_end(); UI != E; ++UI)
        {
            llvm::User* U = *UI;
            if (llvm::isa<llvm::PHINode>(U))
            {
                return true;
            }
        }
        return false;
    }

    inline bool IsDbgInst(llvm::Instruction& inst)
    {
        if (llvm::isa<llvm::DbgInfoIntrinsic>(&inst))
        {
            return true;
        }
        return false;
    }

    bool CodeGenPatternMatch::IsConstOrSimdConstExpr(Value* C)
    {
        if (isa<ConstantInt>(C))
        {
            return true;
        }
        if (Instruction * inst = dyn_cast<Instruction>(C))
        {
            return SIMDConstExpr(inst);
        }
        return false;
    }

    // this function need to be in sync with CShader::EvaluateSIMDConstExpr on what can be supported
    bool CodeGenPatternMatch::SIMDConstExpr(Instruction* C)
    {
        auto it = m_IsSIMDConstExpr.find(C);
        if (it != m_IsSIMDConstExpr.end())
        {
            return it->second;
        }
        bool isConstExpr = false;
        if (BinaryOperator * op = dyn_cast<BinaryOperator>(C))
        {
            switch (op->getOpcode())
            {
            case Instruction::Add:
                isConstExpr = IsConstOrSimdConstExpr(op->getOperand(0)) && IsConstOrSimdConstExpr(op->getOperand(1));
                break;
            case Instruction::Mul:
                isConstExpr = IsConstOrSimdConstExpr(op->getOperand(0)) && IsConstOrSimdConstExpr(op->getOperand(1));
                break;
            case Instruction::Shl:
                isConstExpr = IsConstOrSimdConstExpr(op->getOperand(0)) && IsConstOrSimdConstExpr(op->getOperand(1));
                break;
            default:
                break;
            }
        }
        else if (llvm::GenIntrinsicInst * genInst = dyn_cast<GenIntrinsicInst>(C))
        {
            if (genInst->getIntrinsicID() == GenISAIntrinsic::GenISA_simdSize)
            {
                isConstExpr = true;
            }
        }
        m_IsSIMDConstExpr.insert(std::make_pair(C, isConstExpr));
        return isConstExpr;
    }

    bool CodeGenPatternMatch::NeedInstruction(llvm::Instruction& I)
    {
        if (SIMDConstExpr(&I))
        {
            return false;
        }
        if (HasPhiUse(I) || HasSideEffect(I) || IsDbgInst(I) ||
            (m_usedInstructions.find(&I) != m_usedInstructions.end()))
        {
            return true;
        }
        return false;
    }

    void CodeGenPatternMatch::AddToConstantPool(llvm::BasicBlock* UseBlock,
        llvm::Value* Val) {
        Constant* C = dyn_cast_or_null<Constant>(Val);
        if (!C)
            return;

        BasicBlock* LCA = UseBlock;
        // Determine where we put the constant initialization.
        // Choose loop pre-header as LICM.
        // XXX: Further investigation/tuning is needed to see whether
        // we need to hoist constant initialization out of the
        // top-level loop within a nested loop. So far, we only hoist
        // one level up.
        if (Loop * L = LI->getLoopFor(LCA)) {
            if (BasicBlock * Preheader = L->getLoopPreheader())
                LCA = Preheader;
        }
        // Find the common dominator as CSE.
        if (BasicBlock * BB = ConstantPlacement.lookup(C))
            LCA = DT->findNearestCommonDominator(LCA, BB);
        IGC_ASSERT_MESSAGE(LCA, "LCA always exists for reachable BBs within a function!");
        ConstantPlacement[C] = LCA;
    }

    // Check bool values that can be emitted as a single element predicate.
    void CodeGenPatternMatch::gatherUniformBools(Value* Val)
    {
        if (!isUniform(Val) || Val->getType()->getScalarType()->isIntegerTy(1))
            return;

        // Only starts from select instruction for now.
        // It is more complicate for uses in terminators.
        if (SelectInst * SI = dyn_cast<SelectInst>(Val)) {
            Value* Cond = SI->getCondition();
            if (Cond->getType()->isVectorTy() || !Cond->hasOneUse())
                return;

            // All users of bool values.
            DenseSet<Value*> Vals;
            Vals.insert(SI);

            // Grow the list of bool values to be checked.
            std::vector<Value*> ValList;
            ValList.push_back(Cond);

            bool IsLegal = true;
            while (!ValList.empty()) {
                Value* V = ValList.back();
                ValList.pop_back();
                IGC_ASSERT(nullptr != V);
                IGC_ASSERT(nullptr != V->getType());
                IGC_ASSERT(isUniform(V));
                IGC_ASSERT(V->getType()->isIntegerTy(1));

                // Check uses.
                for (auto UI = V->user_begin(), UE = V->user_end(); UI != UE; ++UI) {
                    Value* U = *UI;
                    if (!Vals.count(U))
                        goto FAIL;
                }

                // Check defs.
                Vals.insert(V);
                if (auto CI = dyn_cast<CmpInst>(V)) {
                    IGC_ASSERT(isUniform(CI->getOperand(0)));
                    IGC_ASSERT(isUniform(CI->getOperand(1)));
                    if (CI->getOperand(0)->getType()->getScalarSizeInBits() == 1)
                        goto FAIL;
                    continue;
                }
                else if (auto BI = dyn_cast<BinaryOperator>(V)) {
                    IGC_ASSERT(isUniform(BI->getOperand(0)));
                    IGC_ASSERT(isUniform(BI->getOperand(1)));
                    if (isa<Instruction>(BI->getOperand(0)))
                        ValList.push_back(BI->getOperand(0));
                    if (isa<Instruction>(BI->getOperand(1)))
                        ValList.push_back(BI->getOperand(1));
                    continue;
                }

            FAIL:
                IsLegal = false;
                break;
            }

            // Populate all boolean values if legal.
            if (IsLegal) {
                for (auto V : Vals) {
                    if (V->getType()->isIntegerTy(1))
                        UniformBools.insert(V);
                }
            }
        }
    }

    void CodeGenPatternMatch::CodeGenBlock(llvm::BasicBlock* bb)
    {
        llvm::BasicBlock::InstListType& instructionList = bb->getInstList();
        llvm::BasicBlock::InstListType::reverse_iterator I, E;
        auto it = m_blockMap.find(bb);
        IGC_ASSERT(it != m_blockMap.end());
        SBasicBlock* block = it->second;

        // loop through instructions bottom up
        for (I = instructionList.rbegin(), E = instructionList.rend(); I != E; ++I)
        {
            llvm::Instruction& inst = (*I);

            if (NeedInstruction(inst))
            {
                SetPatternRoot(inst);
                Pattern* pattern = Match(inst);
                if (pattern)
                {
                    block->m_dags.push_back(SDAG(pattern, m_root));
                    gatherUniformBools(m_root);
                }
            }
        }
    }

    void CodeGenPatternMatch::CreateBasicBlocks(llvm::Function* pLLVMFunc)
    {
        m_numBlocks = pLLVMFunc->size();
        m_blocks = new SBasicBlock[m_numBlocks];
        uint i = 0;
        for (BasicBlock& bb : *pLLVMFunc)
        {
            m_blocks[i].id = i;
            m_blocks[i].bb = &bb;
            m_blockMap.insert(std::pair<llvm::BasicBlock*, SBasicBlock*>(&bb, &m_blocks[i]));
            i++;
        }
    }
    Pattern* CodeGenPatternMatch::Match(llvm::Instruction& inst)
    {
        m_currentPattern = nullptr;
        visit(inst);
        return m_currentPattern;
    }

    void CodeGenPatternMatch::SetPatternRoot(llvm::Instruction& inst)
    {
        m_root = &inst;
        m_rootIsSubspanUse = IsSubspanUse(m_root);
    }

    template<typename Op_t, typename ConstTy>
    struct ClampWithConstants_match {
        typedef ConstTy* ConstPtrTy;

        Op_t Op;
        ConstPtrTy& CMin, & CMax;

        ClampWithConstants_match(const Op_t& OpMatch,
            ConstPtrTy& Min, ConstPtrTy& Max)
            : Op(OpMatch), CMin(Min), CMax(Max) {}

        template<typename OpTy>
        bool match(OpTy* V) {
            CallInst* GII = dyn_cast<CallInst>(V);
            if (!GII)
                return false;

            EOPCODE op = GetOpCode(GII);

            if (op != llvm_max && op != llvm_min)
                return false;

            Value* X = GII->getOperand(0);
            Value* C = GII->getOperand(1);
            if (isa<ConstTy>(X))
                std::swap(X, C);

            ConstPtrTy C0 = dyn_cast<ConstTy>(C);
            if (!C0)
                return false;

            CallInst* GII2 = dyn_cast<CallInst>(X);
            if (!GII2)
                return false;

            EOPCODE op2 = GetOpCode(GII2);
            if (!(op == llvm_min && op2 == llvm_max) &&
                !(op == llvm_max && op2 == llvm_min))
                return false;

            X = GII2->getOperand(0);
            C = GII2->getOperand(1);
            if (isa<ConstTy>(X))
                std::swap(X, C);

            ConstPtrTy C1 = dyn_cast<ConstTy>(C);
            if (!C1)
                return false;

            if (!Op.match(X))
                return false;

            CMin = (op2 == llvm_min) ? C0 : C1;
            CMax = (op2 == llvm_min) ? C1 : C0;
            return true;
        }
    };

    template<typename OpTy, typename ConstTy>
    inline ClampWithConstants_match<OpTy, ConstTy>
        m_ClampWithConstants(const OpTy& Op, ConstTy*& Min, ConstTy*& Max) {
        return ClampWithConstants_match<OpTy, ConstTy>(Op, Min, Max);
    }

    template<typename Op_t>
    struct IsNaN_match {
        Op_t Op;

        IsNaN_match(const Op_t& OpMatch) : Op(OpMatch) {}

        template<typename OpTy>
        bool match(OpTy* V) {
            using namespace llvm::PatternMatch;

            FCmpInst* FCI = dyn_cast<FCmpInst>(V);
            if (!FCI)
                return false;

            switch (FCI->getPredicate()) {
            case FCmpInst::FCMP_UNE:
                return FCI->getOperand(0) == FCI->getOperand(1) &&
                    Op.match(FCI->getOperand(0));
            case FCmpInst::FCMP_UNO:
                return m_Zero().match(FCI->getOperand(1)) &&
                    Op.match(FCI->getOperand(0));
            default:
                break;
            }

            return false;
        }
    };

    template<typename OpTy>
    inline IsNaN_match<OpTy> m_IsNaN(const OpTy& Op) {
        return IsNaN_match<OpTy>(Op);
    }

    std::tuple<Value*, unsigned, VISA_Type>
        CodeGenPatternMatch::isFPToIntegerSatWithExactConstant(llvm::CastInst* I) {
        using namespace llvm::PatternMatch; // Scoped using declaration.

        unsigned Opcode = I->getOpcode();
        IGC_ASSERT(Opcode == Instruction::FPToSI || Opcode == Instruction::FPToUI);

        unsigned BitWidth = I->getDestTy()->getIntegerBitWidth();
        APFloat FMin(I->getSrcTy()->getFltSemantics());
        APFloat FMax(I->getSrcTy()->getFltSemantics());
        if (Opcode == Instruction::FPToSI) {
            if (FMax.convertFromAPInt(APInt::getSignedMaxValue(BitWidth), true,
                APFloat::rmNearestTiesToEven) != APFloat::opOK)
                return std::make_tuple(nullptr, 0, ISA_TYPE_F);
            if (FMin.convertFromAPInt(APInt::getSignedMinValue(BitWidth), true,
                APFloat::rmNearestTiesToEven) != APFloat::opOK)
                return std::make_tuple(nullptr, 0, ISA_TYPE_F);
        }
        else {
            if (FMax.convertFromAPInt(APInt::getMaxValue(BitWidth), false,
                APFloat::rmNearestTiesToEven) != APFloat::opOK)
                return std::make_tuple(nullptr, 0, ISA_TYPE_F);
            if (FMin.convertFromAPInt(APInt::getMinValue(BitWidth), false,
                APFloat::rmNearestTiesToEven) != APFloat::opOK)
                return std::make_tuple(nullptr, 0, ISA_TYPE_F);
        }

        llvm::ConstantFP* CMin = nullptr;
        llvm::ConstantFP* CMax = nullptr;
        llvm::Value* X = nullptr;

        if (!match(I->getOperand(0), m_ClampWithConstants(m_Value(X), CMin, CMax)))
            return std::make_tuple(nullptr, 0, ISA_TYPE_F);

        if (!CMin->isExactlyValue(FMin) || !CMax->isExactlyValue(FMax))
            return std::make_tuple(nullptr, 0, ISA_TYPE_F);

        return std::make_tuple(X, Opcode, GetType(I->getType(), m_ctx));
    }

    // The following pattern matching is targeted to the conversion from FP values
    // to INTEGER values with saturation where the MAX and/or MIN INTEGER values
    // cannot be represented in FP values exactly. E.g., UINT_MAX (2**32-1) in
    // 'unsigned' cannot be represented in 'float', where only 23 significant bits
    // are available but UINT_MAX needs 32 significant bits. We cannot simply
    // express that conversion with saturation as
    //
    //  o := fptoui(clamp(x, float(UINT_MIN), float(UINT_MAX));
    //
    // as, in LLVM, fptoui is undefined when the 'unsigned' source cannot fit in
    // 'float', where clamp(x, MIN, MAX) is defined as max(min(x, MAX), MIN),
    //
    // Hence, OCL use the following sequence (over-simplified by excluding the NaN
    // case.)
    //
    //  o := select(fptoui(x), UINT_MIN, x < float(UINT_MIN));
    //  o := select(o,         UINT_MAX, x > float(UINT_MAX));
    //
    // (We SHOULD use 'o := select(o, UINTMAX, x >= float(UINT_MAX))' as
    // 'float(UINT_MAX)' will be rounded to UINT_MAX+1, i.e. 2 ** 32, and the next
    // smaller value than float(UINT_MAX) in 'float' is (2 ** 24 - 1) << 8. For
    // 'int', that's also true for INT_MIN.)

    std::tuple<Value*, unsigned, VISA_Type>
        CodeGenPatternMatch::isFPToSignedIntSatWithInexactConstant(llvm::SelectInst* SI) {
        using namespace llvm::PatternMatch; // Scoped using declaration.

        // TODO
        return std::make_tuple(nullptr, 0, ISA_TYPE_F);
    }

    std::tuple<Value*, unsigned, VISA_Type>
        CodeGenPatternMatch::isFPToUnsignedIntSatWithInexactConstant(llvm::SelectInst* SI)
    {
        using namespace llvm::PatternMatch; // Scoped using declaration.

        Constant* C0 = dyn_cast<Constant>(SI->getTrueValue());
        if (!C0)
            return std::make_tuple(nullptr, 0, ISA_TYPE_F);
        if (!isa<ConstantFP>(C0) && !isa<ConstantInt>(C0))
            return std::make_tuple(nullptr, 0, ISA_TYPE_F);
        Value* Cond = SI->getCondition();

        SelectInst* SI2 = dyn_cast<SelectInst>(SI->getFalseValue());
        if (!SI2)
            return std::make_tuple(nullptr, 0, ISA_TYPE_F);
        Constant* C1 = dyn_cast<Constant>(SI2->getTrueValue());
        if (!C1)
            return std::make_tuple(nullptr, 0, ISA_TYPE_F);
        if (!isa<ConstantFP>(C1) && !isa<ConstantInt>(C1))
            return std::make_tuple(nullptr, 0, ISA_TYPE_F);
        Value* Cond2 = SI2->getCondition();

        Value* X = SI2->getFalseValue();
        Type* Ty = X->getType();
        if (Ty->isFloatTy()) {
            BitCastInst* BC = dyn_cast<BitCastInst>(X);
            if (!BC)
                return std::make_tuple(nullptr, 0, ISA_TYPE_F);
            X = BC->getOperand(0);
            Ty = X->getType();
            C1 = ConstantExpr::getBitCast(C1, Ty);
            C0 = ConstantExpr::getBitCast(C0, Ty);
        }
        IntegerType* ITy = dyn_cast<IntegerType>(Ty);
        if (!ITy)
            return std::make_tuple(nullptr, 0, ISA_TYPE_F);
        unsigned BitWidth = ITy->getBitWidth();
        FPToUIInst* CI = dyn_cast<FPToUIInst>(X);
        if (!CI)
            return std::make_tuple(nullptr, 0, ISA_TYPE_F);
        Ty = CI->getSrcTy();
        if (!(Ty->isFloatTy() && BitWidth == 32) &&
            !(Ty->isDoubleTy() && BitWidth == 64))
            return std::make_tuple(nullptr, 0, ISA_TYPE_F);
        X = CI->getOperand(0);

        ConstantInt* CMin = dyn_cast<ConstantInt>(C0);
        ConstantInt* CMax = dyn_cast<ConstantInt>(C1);
        if (!CMax || !CMin || !CMax->isMaxValue(false) || !CMin->isMinValue(false))
            return std::make_tuple(nullptr, 0, ISA_TYPE_F);

        Constant* FMin = ConstantExpr::getUIToFP(CMin, Ty);
        Constant* FMax = ConstantExpr::getUIToFP(CMax, Ty);

        FCmpInst::Predicate Pred = FCmpInst::FCMP_FALSE;
        if (!match(Cond2, m_FCmp(Pred, m_Specific(X), m_Specific(FMax))))
            return std::make_tuple(nullptr, 0, ISA_TYPE_F);
        if (Pred != FCmpInst::FCMP_OGT) // FIXME: We should use OGE instead of OGT.
            return std::make_tuple(nullptr, 0, ISA_TYPE_F);

        FCmpInst::Predicate Pred2 = FCmpInst::FCMP_FALSE;
        if (!match(Cond,
            m_Or(m_FCmp(Pred, m_Specific(X), m_Specific(FMin)),
                m_FCmp(Pred2, m_Specific(X), m_Specific(X))))) {
            if (!match(Cond,
                m_Or(m_FCmp(Pred, m_Specific(X), m_Specific(FMin)),
                    m_Zero()))) {
                return std::make_tuple(nullptr, 0, ISA_TYPE_F);
            }
            // Special case where the staturatured result is bitcasted into float
            // again (due to typedwrite only accepts `float`. So the isNaN(X) is
            // reduced to `false`.
            Pred2 = FCmpInst::FCMP_UNE;
        }
        if (Pred != FCmpInst::FCMP_OLT || Pred2 != FCmpInst::FCMP_UNE)
            return std::make_tuple(nullptr, 0, ISA_TYPE_F);

        VISA_Type type = GetType(CI->getType(), m_ctx);

        // Fold extra clamp.
        Value* X2 = nullptr;
        ConstantFP* CMin2 = nullptr;
        ConstantFP* CMax2 = nullptr;
        if (match(X, m_ClampWithConstants(m_Value(X2), CMin2, CMax2))) {
            if (CMin2 == FMin) {
                if (CMax2->isExactlyValue(255.0)) {
                    X = X2;
                    type = ISA_TYPE_B;
                }
                else if (CMax2->isExactlyValue(65535.0)) {
                    X = X2;
                    type = ISA_TYPE_W;
                }
            }
        }

        return std::make_tuple(X, Instruction::FPToUI, type);
    }

    bool CodeGenPatternMatch::MatchFPToIntegerWithSaturation(llvm::Instruction& I) {
        Value* X = nullptr;
        unsigned Opcode = 0;
        VISA_Type type = ISA_TYPE_NUM;

        if (CastInst * CI = dyn_cast<CastInst>(&I)) {
            std::tie(X, Opcode, type) = isFPToIntegerSatWithExactConstant(CI);
            if (!X)
                return false;
        }
        else if (SelectInst * SI = dyn_cast<SelectInst>(&I)) {
            std::tie(X, Opcode, type) = isFPToSignedIntSatWithInexactConstant(SI);
            if (!X) {
                std::tie(X, Opcode, type) = isFPToUnsignedIntSatWithInexactConstant(SI);
                if (!X)
                    return false;
            }
        }
        else {
            return false;
        }

        // Match!
        IGC_ASSERT(Opcode == Instruction::FPToSI || Opcode == Instruction::FPToUI);

        struct FPToIntegerWithSaturationPattern : public Pattern {
            bool isUnsigned, needBitCast;
            VISA_Type type;
            SSource src;
            virtual void Emit(EmitPass* pass, const DstModifier& dstMod) {
                pass->EmitFPToIntWithSat(isUnsigned, needBitCast, type, src, dstMod);
            }
        };

        bool isUnsigned = (Opcode == Instruction::FPToUI);
        FPToIntegerWithSaturationPattern* pat
            = new (m_allocator) FPToIntegerWithSaturationPattern();
        pat->isUnsigned = isUnsigned;
        pat->needBitCast = !I.getType()->isIntegerTy();
        pat->type = type;
        pat->src = GetSource(X, !isUnsigned, false);
        AddPattern(pat);

        return true;
    }

    std::tuple<Value*, bool, bool>
        CodeGenPatternMatch::isIntegerSatTrunc(llvm::SelectInst* SI) {
        using namespace llvm::PatternMatch; // Scoped using declaration.

        ICmpInst* Cmp = dyn_cast<ICmpInst>(SI->getOperand(0));
        if (!Cmp)
            return std::make_tuple(nullptr, false, false);

        ICmpInst::Predicate Pred = Cmp->getPredicate();
        if (Pred != ICmpInst::ICMP_SGT && Pred != ICmpInst::ICMP_UGT)
            return std::make_tuple(nullptr, false, false);

        ConstantInt* CI = dyn_cast<ConstantInt>(Cmp->getOperand(1));
        if (!CI)
            return std::make_tuple(nullptr, false, false);

        // Truncate into unsigned integer by default.
        bool isSignedDst = false;
        unsigned DstBitWidth = SI->getType()->getIntegerBitWidth();
        unsigned SrcBitWidth = Cmp->getOperand(0)->getType()->getIntegerBitWidth();
        APInt UMax = APInt::getMaxValue(DstBitWidth);
        APInt UMin = APInt::getMinValue(DstBitWidth);
        APInt SMax = APInt::getSignedMaxValue(DstBitWidth);
        APInt SMin = APInt::getSignedMinValue(DstBitWidth);
        if (SrcBitWidth > DstBitWidth) {
            UMax = UMax.zext(SrcBitWidth);
            UMin = UMin.zext(SrcBitWidth);
            SMax = SMax.sext(SrcBitWidth);
            SMin = SMin.sext(SrcBitWidth);
        }
        else
        {
            // SrcBitwidth should be always wider than DstBitwidth,
            // since src is a source of a trunc instruction, and dst
            // have the same width as its destination.
            return std::make_tuple(nullptr, false, false);
        }

        if (CI->getValue() != UMax && CI->getValue() != SMax)
            return std::make_tuple(nullptr, false, false);
        if (CI->getValue() == SMax) // Truncate into signed integer.
            isSignedDst = true;

        APInt MinValue = isSignedDst ? SMin : UMin;
        CI = dyn_cast<ConstantInt>(SI->getOperand(1));
        if (!CI || !CI->isMaxValue(isSignedDst))
            return std::make_tuple(nullptr, false, false);

        TruncInst* TI = dyn_cast<TruncInst>(SI->getOperand(2));
        if (!TI)
            return std::make_tuple(nullptr, false, false);

        Value* Val = TI->getOperand(0);
        if (Val != Cmp->getOperand(0))
            return std::make_tuple(nullptr, false, false);

        // Truncate from unsigned integer.
        if (Pred == ICmpInst::ICMP_UGT)
            return std::make_tuple(Val, isSignedDst, false);

        // Truncate from signed integer. Need to check further for lower bound.
        Value* LHS, * RHS;
        if (!match(Val, m_SMax(m_Value(LHS), m_Value(RHS))))
            return std::make_tuple(nullptr, false, false);

        if (isa<ConstantInt>(LHS))
            std::swap(LHS, RHS);

        CI = dyn_cast<ConstantInt>(RHS);
        if (!CI || CI->getValue() != MinValue)
            return std::make_tuple(nullptr, false, false);

        return std::make_tuple(LHS, isSignedDst, true);
    }

    bool CodeGenPatternMatch::MatchIntegerTruncSatModifier(llvm::SelectInst& I) {
        // Only match BYTE or WORD.
        if (!I.getType()->isIntegerTy(8) && !I.getType()->isIntegerTy(16))
            return false;
        Value* Src = nullptr;
        bool isSignedDst = false, isSignedSrc = false;
        std::tie(Src, isSignedDst, isSignedSrc) = isIntegerSatTrunc(&I);
        if (!Src)
            return false;

        struct IntegerSatTruncPattern : public Pattern {
            SSource src;
            bool isSignedDst;
            bool isSignedSrc;
            virtual void Emit(EmitPass* pass, const DstModifier& dstMod) {
                pass->EmitIntegerTruncWithSat(isSignedDst, isSignedSrc, src, dstMod);
            }
        };

        IntegerSatTruncPattern* pat = new (m_allocator) IntegerSatTruncPattern();
        pat->src = GetSource(Src, isSignedSrc, false);
        pat->isSignedDst = isSignedDst;
        pat->isSignedSrc = isSignedSrc;
        AddPattern(pat);

        return true;
    }

    void CodeGenPatternMatch::visitFPToSIInst(llvm::FPToSIInst& I) {
        bool match = MatchFPToIntegerWithSaturation(I) || MatchModifier(I);
        IGC_ASSERT_MESSAGE(match, "Pattern match Failed");
    }

    void CodeGenPatternMatch::visitFPToUIInst(llvm::FPToUIInst& I) {
        bool match = MatchFPToIntegerWithSaturation(I) || MatchModifier(I);
        IGC_ASSERT_MESSAGE(match, "Pattern match Failed");
    }

    bool CodeGenPatternMatch::MatchSIToFPZExt(llvm::SIToFPInst* S2FI) {
        ZExtInst* ZEI = dyn_cast<ZExtInst>(S2FI->getOperand(0));
        if (!ZEI)
            return false;
        if (!ZEI->getSrcTy()->isIntegerTy(1))
            return false;

        struct SIToFPExtPattern : public Pattern {
            SSource src;
            virtual void Emit(EmitPass* pass, const DstModifier& dstMod) {
                pass->EmitSIToFPZExt(src, dstMod);
            }
        };

        SIToFPExtPattern* pat = new (m_allocator) SIToFPExtPattern();
        pat->src = GetSource(ZEI->getOperand(0), false, false);
        AddPattern(pat);

        return true;
    }

    void CodeGenPatternMatch::visitCastInst(llvm::CastInst& I)
    {
        bool match = 0;
        if (I.getOpcode() == Instruction::SExt)
        {
            match = MatchCmpSext(I) ||
                MatchModifier(I);
        }
        else if (I.getOpcode() == Instruction::SIToFP)
        {
            match = MatchSIToFPZExt(cast<SIToFPInst>(&I)) || MatchModifier(I);
        }
        else if (I.getOpcode() == Instruction::Trunc)
        {
            match =
                MatchModifier(I);
        }
        else
        {
            match = MatchModifier(I);
        }
    }

    bool CodeGenPatternMatch::NeedVMask()
    {
        return m_NeedVMask;
    }

    bool CodeGenPatternMatch::HasUseOutsideLoop(llvm::Value* v)
    {
        if (Instruction * inst = dyn_cast<Instruction>(v))
        {
            if (Loop * L = LI->getLoopFor(inst->getParent()))
            {
                for (auto UI = inst->user_begin(), E = inst->user_end(); UI != E; ++UI)
                {
                    if (!L->contains(cast<Instruction>(*UI)))
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    void CodeGenPatternMatch::HandleSubspanUse(llvm::Value* v)
    {
        IGC_ASSERT(m_root != nullptr);
        if (m_ctx->type != ShaderType::PIXEL_SHADER)
        {
            return;
        }
        if (!isa<Constant>(v) && m_WI->whichDepend(v) != WIAnalysis::UNIFORM)
        {
            if (isa<PHINode>(v) || HasUseOutsideLoop(v))
            {
                // If a phi is used in a subspan we cannot propagate the subspan use and need to use VMask
                m_NeedVMask = true;
            }
            else
            {
                m_subSpanUse.insert(v);
                if (LoadInst * load = dyn_cast<LoadInst>(v))
                {
                    if (load->getPointerAddressSpace() == ADDRESS_SPACE_PRIVATE)
                    {
                        m_NeedVMask = true;
                    }
                }
                if (HasPhiUse(*v) && m_WI->insideDivergentCF(m_root))
                {
                    // \todo, more accurate condition for force-isolation
                    ForceIsolate(v);
                }
            }
        }
    }

    bool CodeGenPatternMatch::MatchMinMax(llvm::SelectInst& SI) {
        // Pattern to emit.
        struct MinMaxPattern : public Pattern {
            SSource srcs[2];
            bool isMin, isUnsigned;
            virtual void Emit(EmitPass* pass, const DstModifier& dstMod) {
                // FIXME: We should tell umax/umin from max/min as integers in LLVM
                // have no sign!
                pass->EmitMinMax(isMin, isUnsigned, srcs, dstMod);
            }
        };

        // Skip min/max pattern matching on FP, which needs to either explicitly
        // use intrinsics or convert them into intrinsic in GenIRLower pass.
        if (SI.getType()->isFloatingPointTy())
            return false;

        bool isMin, isUnsigned;
        llvm::Value* LHS, * RHS;

        if (!isMinOrMax(&SI, LHS, RHS, isMin, isUnsigned))
            return false;

        MinMaxPattern* pat = new (m_allocator) MinMaxPattern();
        // FIXME: We leave unsigned operand without source modifier so far. When
        // its behavior is detailed and correcty modeled, consider to add source
        // modifier support.
        pat->srcs[0] = GetSource(LHS, !isUnsigned, false);
        pat->srcs[1] = GetSource(RHS, !isUnsigned, false);
        pat->isMin = isMin;
        pat->isUnsigned = isUnsigned;
        AddPattern(pat);

        return true;
    }

    void CodeGenPatternMatch::visitSelectInst(SelectInst& I)
    {
        bool match = MatchFloatingPointSatModifier(I) ||
            MatchIntegerTruncSatModifier(I) ||
            MatchAbsNeg(I) ||
            MatchFPToIntegerWithSaturation(I) ||
            MatchMinMax(I) ||
            /*MatchPredicate(I)   ||*/
            MatchSelectModifier(I);
        IGC_ASSERT_MESSAGE(match, "Pattern Match failed");
    }

    void CodeGenPatternMatch::visitBinaryOperator(llvm::BinaryOperator& I)
    {

        bool match = false;
        switch (I.getOpcode())
        {
        case Instruction::FSub:
            match = MatchFrc(I) ||
                MatchLrp(I) ||
                MatchPredAdd(I) ||
                MatchMad(I) ||
                MatchAbsNeg(I) ||
                MatchModifier(I);
            break;
        case Instruction::Sub:
            match = MatchMad(I) ||
                MatchAbsNeg(I) ||
                MatchMulAdd16(I) ||
                MatchModifier(I);
            break;
        case Instruction::Mul:
            match = MatchFullMul32(I) ||
                MatchMulAdd16(I) ||
                MatchModifier(I);
            break;
        case Instruction::Add:
            match = MatchMad(I) ||
                MatchMulAdd16(I) ||
                MatchModifier(I);
            break;
        case Instruction::UDiv:
        case Instruction::SDiv:
        case Instruction::AShr:
            match = MatchAvg(I) ||
                MatchModifier(I);
            break;
        case Instruction::FMul:
        case Instruction::URem:
        case Instruction::SRem:
        case Instruction::FRem:
        case Instruction::Shl:
            match = MatchModifier(I);
            break;
        case Instruction::LShr:
            match = MatchModifier(I, false);
            break;
        case Instruction::FDiv:
            match = MatchRsqrt(I) ||
                MatchModifier(I);
            break;
        case Instruction::FAdd:
            match = MatchLrp(I) ||
                MatchPredAdd(I) ||
                MatchMad(I) ||
                MatchSimpleAdd(I) ||
                MatchModifier(I);
            break;
        case Instruction::And:
            match =
                MatchBoolOp(I) ||
                MatchLogicAlu(I);
            break;
        case Instruction::Or:
            match =
                MatchBoolOp(I) ||
                MatchLogicAlu(I);
            break;
        case Instruction::Xor:
            match =
                MatchLogicAlu(I);
            break;
        default:
            IGC_ASSERT_MESSAGE(0, "unknown binary instruction");
            break;
        }
        IGC_ASSERT(match == true);
    }

    void CodeGenPatternMatch::visitCmpInst(llvm::CmpInst& I)
    {
        bool match = MatchCondModifier(I) ||
            MatchModifier(I);
        IGC_ASSERT(match);
    }

    void CodeGenPatternMatch::visitBranchInst(llvm::BranchInst& I)
    {
        MatchBranch(I);
    }

    void CodeGenPatternMatch::visitCallInst(CallInst& I)
    {
        bool match = false;
        using namespace GenISAIntrinsic;
        if (GenIntrinsicInst * CI = llvm::dyn_cast<GenIntrinsicInst>(&I))
        {
            switch (CI->getIntrinsicID())
            {
            case GenISAIntrinsic::GenISA_ROUNDNE:
            case GenISAIntrinsic::GenISA_imulH:
            case GenISAIntrinsic::GenISA_umulH:
            case GenISAIntrinsic::GenISA_uaddc:
            case GenISAIntrinsic::GenISA_usubb:
            case GenISAIntrinsic::GenISA_bfrev:
            case GenISAIntrinsic::GenISA_IEEE_Sqrt:
            case GenISAIntrinsic::GenISA_IEEE_Divide:
            case GenISAIntrinsic::GenISA_rsq:
                match = MatchModifier(I);
                break;
            case GenISAIntrinsic::GenISA_intatomicraw:
            case GenISAIntrinsic::GenISA_floatatomicraw:
            case GenISAIntrinsic::GenISA_intatomicrawA64:
            case GenISAIntrinsic::GenISA_floatatomicrawA64:
            case GenISAIntrinsic::GenISA_icmpxchgatomicraw:
            case GenISAIntrinsic::GenISA_fcmpxchgatomicraw:
            case GenISAIntrinsic::GenISA_icmpxchgatomicrawA64:
            case GenISAIntrinsic::GenISA_fcmpxchgatomicrawA64:
            case GenISAIntrinsic::GenISA_dwordatomicstructured:
            case GenISAIntrinsic::GenISA_floatatomicstructured:
            case GenISAIntrinsic::GenISA_cmpxchgatomicstructured:
            case GenISAIntrinsic::GenISA_fcmpxchgatomicstructured:
            case GenISAIntrinsic::GenISA_intatomictyped:
            case GenISAIntrinsic::GenISA_icmpxchgatomictyped:
            case GenISAIntrinsic::GenISA_typedread:
            case GenISAIntrinsic::GenISA_typedwrite:
            case GenISAIntrinsic::GenISA_ldstructured:
            case GenISAIntrinsic::GenISA_storestructured1:
            case GenISAIntrinsic::GenISA_storestructured2:
            case GenISAIntrinsic::GenISA_storestructured3:
            case GenISAIntrinsic::GenISA_storestructured4:
            case GenISAIntrinsic::GenISA_atomiccounterinc:
            case GenISAIntrinsic::GenISA_atomiccounterpredec:
            case GenISAIntrinsic::GenISA_ldptr:
            case GenISAIntrinsic::GenISA_ldrawvector_indexed:
            case GenISAIntrinsic::GenISA_ldraw_indexed:
            case GenISAIntrinsic::GenISA_storerawvector_indexed:
            case GenISAIntrinsic::GenISA_storeraw_indexed:
                match = MatchSingleInstruction(I);
                break;
            case GenISAIntrinsic::GenISA_GradientX:
            case GenISAIntrinsic::GenISA_GradientY:
            case GenISAIntrinsic::GenISA_GradientXfine:
            case GenISAIntrinsic::GenISA_GradientYfine:
                match = MatchGradient(*CI);
                break;
            case GenISAIntrinsic::GenISA_sampleptr:
            case GenISAIntrinsic::GenISA_sampleBptr:
            case GenISAIntrinsic::GenISA_sampleBCptr:
            case GenISAIntrinsic::GenISA_sampleCptr:
            case GenISAIntrinsic::GenISA_lodptr:
            case GenISAIntrinsic::GenISA_sampleKillPix:
                match = MatchSampleDerivative(*CI);
                break;
            case GenISAIntrinsic::GenISA_fsat:
                match = MatchFloatingPointSatModifier(I);
                break;
            case GenISAIntrinsic::GenISA_usat:
            case GenISAIntrinsic::GenISA_isat:
                match = MatchIntegerSatModifier(I);
                break;
            case GenISAIntrinsic::GenISA_WaveShuffleIndex:
                match = MatchRegisterRegion(*CI) ||
                    MatchShuffleBroadCast(*CI) ||
                    MatchWaveShuffleIndex(*CI);
                break;
            case GenISAIntrinsic::GenISA_simdBlockRead:
            case GenISAIntrinsic::GenISA_simdBlockWrite:
                match = MatchBlockReadWritePointer(*CI) ||
                    MatchSingleInstruction(*CI);
                break;
            case GenISAIntrinsic::GenISA_URBRead:
            case GenISAIntrinsic::GenISA_URBReadOutput:
                match = MatchURBRead(*CI) ||
                    MatchSingleInstruction(*CI);
                break;
            default:
                match = MatchSingleInstruction(I);
                // no pattern for the rest of the intrinsics
                break;
            }
            IGC_ASSERT_MESSAGE(match, "no pattern found for GenISA intrinsic");
        }
        else
        {
            Function* Callee = I.getCalledFunction();

            // Match inline asm
            if (I.isInlineAsm())
            {
                if (getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->m_DriverInfo.SupportInlineAssembly())
                {
                    match = MatchSingleInstruction(I);
                }
            }
            // Match indirect call, support declarations for indirect funcs
            else if (!Callee || Callee->hasFnAttribute("IndirectlyCalled"))
            {
                match = MatchSingleInstruction(I);
            }
            // Match direct call, skip declarations
            else if (!Callee->isDeclaration())
            {
                match = MatchSingleInstruction(I);
            }
        }
        IGC_ASSERT_MESSAGE(match, "no match for this call");
    }

    void CodeGenPatternMatch::visitUnaryInstruction(llvm::UnaryInstruction& I)
    {
        bool match = false;
        switch (I.getOpcode())
        {
        case Instruction::Alloca:
        case Instruction::Load:
        case Instruction::ExtractValue:
            match = MatchSingleInstruction(I);
            break;
        }
        IGC_ASSERT(match);
    }

    void CodeGenPatternMatch::visitIntrinsicInst(llvm::IntrinsicInst& I)
    {
        bool match = false;
        switch (I.getIntrinsicID())
        {
        case Intrinsic::sqrt:
        case Intrinsic::log2:
        case Intrinsic::cos:
        case Intrinsic::sin:
        case Intrinsic::pow:
        case Intrinsic::floor:
        case Intrinsic::ceil:
        case Intrinsic::trunc:
        case Intrinsic::ctpop:
        case Intrinsic::ctlz:
        case Intrinsic::cttz:
            match = MatchModifier(I);
            break;
        case Intrinsic::exp2:
            match = MatchPow(I) ||
                MatchModifier(I);
            break;
        case Intrinsic::fabs:
            match = MatchAbsNeg(I);
            break;
        case Intrinsic::fma:
            match = MatchFMA(I);
            break;
        case Intrinsic::maxnum:
        case Intrinsic::minnum:
            match = MatchFloatingPointSatModifier(I) ||
                MatchModifier(I);
            break;
        default:
            match = MatchSingleInstruction(I);
            // no pattern for the rest of the intrinsics
            break;
        }
        IGC_ASSERT_MESSAGE(match, "no pattern found");
    }

    void CodeGenPatternMatch::visitStoreInst(StoreInst& I)
    {
        bool match = false;
        // we try to fold some pointer values in GFX path, not OCL path
        if (m_ctx->m_DriverInfo.WALoadStorePatternMatch())
        {
            match = MatchSingleInstruction(I);
        }
        else
        {
            match = MatchLoadStorePointer(I, *(I.getPointerOperand())) ||
                MatchSingleInstruction(I);
        }
        IGC_ASSERT(match);
    }

    void CodeGenPatternMatch::visitLoadInst(LoadInst& I)
    {
        bool match = false;
        // we try to fold some pointer values in GFX path, not OCL path
        if (m_ctx->m_DriverInfo.WALoadStorePatternMatch())
        {
            match = MatchSingleInstruction(I);
        }
        else
        {
            match = MatchLoadStorePointer(I, *(I.getPointerOperand())) ||
                MatchSingleInstruction(I);
        }
        IGC_ASSERT(match);
    }

    void CodeGenPatternMatch::visitInstruction(llvm::Instruction& I)
    {
        // use default pattern
        MatchSingleInstruction(I);
    }

    void CodeGenPatternMatch::visitExtractElementInst(llvm::ExtractElementInst& I)
    {
        Value* VecOpnd = I.getVectorOperand();
        if (isa<Constant>(VecOpnd))
        {
            const Function* F = I.getParent()->getParent();
            unsigned NUse = 0;
            for (auto User : VecOpnd->users())
            {
                if (auto Inst = dyn_cast<Instruction>(User))
                {
                    NUse += (Inst->getParent()->getParent() == F);
                }
            }

            // Only add it to pool when there are multiple uses within this
            // function; otherwise no benefit but to hurt RP.
            if (NUse > 1)
                AddToConstantPool(I.getParent(), VecOpnd);
        }
        MatchSingleInstruction(I);
    }

    void CodeGenPatternMatch::visitPHINode(PHINode& I)
    {
        // nothing to do
    }

    void CodeGenPatternMatch::visitBitCastInst(BitCastInst& I)
    {
        // detect
        // %66 = insertelement <2 x i32> <i32 0, i32 undef>, i32 %xor19.i, i32 1
        // %67 = bitcast <2 x i32> % 66 to i64
        // and replace it with a shl 32
        struct Shl32Pattern : public Pattern
        {
            SSource sources[2];
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                pass->Binary(EOPCODE_SHL, sources, modifier);
            }
        };

        if (I.getType()->isIntegerTy(64) && I.getOperand(0)->getType()->isVectorTy() &&
            I.getOperand(0)->getType()->getVectorElementType()->isIntegerTy(32))
        {
            if (auto IEI = dyn_cast<InsertElementInst>(I.getOperand(0)))
            {
                auto vec = dyn_cast<ConstantVector>(IEI->getOperand(0));
                bool isCandidate = vec && vec->getNumOperands() == 2 && IsZero(vec->getOperand(0)) &&
                    isa<UndefValue>(vec->getOperand(1));
                auto index = dyn_cast<ConstantInt>(IEI->getOperand(2));
                isCandidate &= index && index->getZExtValue() == 1;
                if (isCandidate)
                {
                    Shl32Pattern* Pat = new (m_allocator) Shl32Pattern();
                    Pat->sources[0] = GetSource(IEI->getOperand(1), false, false);
                    Pat->sources[1] = GetSource(ConstantInt::get(Type::getInt32Ty(I.getContext()), 32), false, false);
                    AddPattern(Pat);
                    return;
                }
            }
        }

        MatchSingleInstruction(I);
    }

    void CodeGenPatternMatch::visitIntToPtrInst(IntToPtrInst& I) {
        MatchSingleInstruction(I);
    }

    void CodeGenPatternMatch::visitPtrToIntInst(PtrToIntInst& I) {
        MatchSingleInstruction(I);
    }

    void CodeGenPatternMatch::visitAddrSpaceCast(AddrSpaceCastInst& I)
    {
        MatchSingleInstruction(I);
    }

    void CodeGenPatternMatch::visitDbgInfoIntrinsic(DbgInfoIntrinsic& I)
    {
        MatchDbgInstruction(I);
    }

    void CodeGenPatternMatch::visitExtractValueInst(ExtractValueInst& I) {
        bool Match = false;

        // Ignore the extract value instruction. Handled in the call inst.
        if (CallInst * call = dyn_cast<CallInst>(I.getOperand(0)))
        {
            if (call->isInlineAsm() && call->getType()->isStructTy())
            {
                MarkAsSource(call);
                return;
            }
        }

        Match = matchAddPair(&I) ||
            matchSubPair(&I) ||
            matchMulPair(&I) ||
            matchPtrToPair(&I);

        IGC_ASSERT_MESSAGE(Match, "Unknown `extractvalue` instruction!");
    }

    bool CodeGenPatternMatch::matchAddPair(ExtractValueInst* Ex) {
        Value* V = Ex->getOperand(0);
        GenIntrinsicInst* GII = dyn_cast<GenIntrinsicInst>(V);
        if (!GII || GII->getIntrinsicID() != GenISAIntrinsic::GenISA_add_pair)
            return false;

        if (Ex->getNumIndices() != 1)
            return false;
        unsigned Idx = *Ex->idx_begin();
        if (Idx != 0 && Idx != 1)
            return false;

        struct AddPairPattern : public Pattern {
            GenIntrinsicInst* GII;
            SSource Sources[4]; // L0, H0, L1, H1
            virtual void Emit(EmitPass* Pass, const DstModifier& DstMod) {
                Pass->EmitAddPair(GII, Sources, DstMod);
            }
        };

        struct AddPairSubPattern : public Pattern {
            virtual void Emit(EmitPass* Pass, const DstModifier& Mod) {
                // DO NOTHING. Dummy pattern.
            }
        };

        PairOutputMapTy::iterator MI;
        bool New;
        std::tie(MI, New) = PairOutputMap.insert(std::make_pair(GII, PairOutputTy()));
        if (New) {
            AddPairPattern* Pat = new (m_allocator) AddPairPattern();
            Pat->GII = GII;
            Pat->Sources[0] = GetSource(GII->getOperand(0), false, false);
            Pat->Sources[1] = GetSource(GII->getOperand(1), false, false);
            Pat->Sources[2] = GetSource(GII->getOperand(2), false, false);
            Pat->Sources[3] = GetSource(GII->getOperand(3), false, false);
            AddPattern(Pat);
        }
        else {
            AddPairSubPattern* Pat = new (m_allocator) AddPairSubPattern();
            AddPattern(Pat);
        }
        if (Idx == 0)
            MI->second.first = Ex;
        else
            MI->second.second = Ex;

        return true;
    }

    bool CodeGenPatternMatch::matchSubPair(ExtractValueInst* Ex) {
        Value* V = Ex->getOperand(0);
        GenIntrinsicInst* GII = dyn_cast<GenIntrinsicInst>(V);
        if (!GII || GII->getIntrinsicID() != GenISAIntrinsic::GenISA_sub_pair)
            return false;

        if (Ex->getNumIndices() != 1)
            return false;
        unsigned Idx = *Ex->idx_begin();
        if (Idx != 0 && Idx != 1)
            return false;

        struct SubPairPattern : public Pattern {
            GenIntrinsicInst* GII;
            SSource Sources[4]; // L0, H0, L1, H1
            virtual void Emit(EmitPass* Pass, const DstModifier& DstMod) {
                Pass->EmitSubPair(GII, Sources, DstMod);
            }
        };

        struct SubPairSubPattern : public Pattern {
            virtual void Emit(EmitPass* Pass, const DstModifier& Mod) {
                // DO NOTHING. Dummy pattern.
            }
        };

        PairOutputMapTy::iterator MI;
        bool New;
        std::tie(MI, New) = PairOutputMap.insert(std::make_pair(GII, PairOutputTy()));
        if (New) {
            SubPairPattern* Pat = new (m_allocator) SubPairPattern();
            Pat->GII = GII;
            Pat->Sources[0] = GetSource(GII->getOperand(0), false, false);
            Pat->Sources[1] = GetSource(GII->getOperand(1), false, false);
            Pat->Sources[2] = GetSource(GII->getOperand(2), false, false);
            Pat->Sources[3] = GetSource(GII->getOperand(3), false, false);
            AddPattern(Pat);
        }
        else {
            SubPairSubPattern* Pat = new (m_allocator) SubPairSubPattern();
            AddPattern(Pat);
        }
        if (Idx == 0)
            MI->second.first = Ex;
        else
            MI->second.second = Ex;

        return true;
    }

    bool CodeGenPatternMatch::matchMulPair(ExtractValueInst* Ex) {
        Value* V = Ex->getOperand(0);
        GenIntrinsicInst* GII = dyn_cast<GenIntrinsicInst>(V);
        if (!GII || GII->getIntrinsicID() != GenISAIntrinsic::GenISA_mul_pair)
            return false;

        if (Ex->getNumIndices() != 1)
            return false;
        unsigned Idx = *Ex->idx_begin();
        if (Idx != 0 && Idx != 1)
            return false;

        struct MulPairPattern : public Pattern {
            GenIntrinsicInst* GII;
            SSource Sources[4]; // L0, H0, L1, H1
            virtual void Emit(EmitPass* Pass, const DstModifier& DstMod) {
                Pass->EmitMulPair(GII, Sources, DstMod);
            }
        };

        struct MulPairSubPattern : public Pattern {
            virtual void Emit(EmitPass* Pass, const DstModifier& Mod) {
                // DO NOTHING. Dummy pattern.
            }
        };

        PairOutputMapTy::iterator MI;
        bool New;
        std::tie(MI, New) = PairOutputMap.insert(std::make_pair(GII, PairOutputTy()));
        if (New) {
            MulPairPattern* Pat = new (m_allocator) MulPairPattern();
            Pat->GII = GII;
            Pat->Sources[0] = GetSource(GII->getOperand(0), false, false);
            Pat->Sources[1] = GetSource(GII->getOperand(1), false, false);
            Pat->Sources[2] = GetSource(GII->getOperand(2), false, false);
            Pat->Sources[3] = GetSource(GII->getOperand(3), false, false);
            AddPattern(Pat);
        }
        else {
            MulPairSubPattern* Pat = new (m_allocator) MulPairSubPattern();
            AddPattern(Pat);
        }
        if (Idx == 0)
            MI->second.first = Ex;
        else
            MI->second.second = Ex;

        return true;
    }

    bool CodeGenPatternMatch::matchPtrToPair(ExtractValueInst* Ex) {
        Value* V = Ex->getOperand(0);
        GenIntrinsicInst* GII = dyn_cast<GenIntrinsicInst>(V);
        if (!GII || GII->getIntrinsicID() != GenISAIntrinsic::GenISA_ptr_to_pair)
            return false;

        if (Ex->getNumIndices() != 1)
            return false;
        unsigned Idx = *Ex->idx_begin();
        if (Idx != 0 && Idx != 1)
            return false;

        struct PtrToPairPattern : public Pattern {
            GenIntrinsicInst* GII;
            SSource Sources[1]; // Ptr
            virtual void Emit(EmitPass* Pass, const DstModifier& DstMod) {
                Pass->EmitPtrToPair(GII, Sources, DstMod);
            }
        };

        struct PtrToPairSubPattern : public Pattern {
            virtual void Emit(EmitPass* Pass, const DstModifier& Mod) {
                // DO NOTHING. Dummy pattern.
            }
        };

        PairOutputMapTy::iterator MI;
        bool New;
        std::tie(MI, New) = PairOutputMap.insert(std::make_pair(GII, PairOutputTy()));
        if (New) {
            PtrToPairPattern* Pat = new (m_allocator) PtrToPairPattern();
            Pat->GII = GII;
            Pat->Sources[0] = GetSource(GII->getOperand(0), false, false);
            AddPattern(Pat);
        }
        else {
            PtrToPairSubPattern* Pat = new (m_allocator) PtrToPairSubPattern();
            AddPattern(Pat);
        }
        if (Idx == 0)
            MI->second.first = Ex;
        else
            MI->second.second = Ex;

        return true;
    }

    bool CodeGenPatternMatch::MatchAbsNeg(llvm::Instruction& I)
    {
        struct MovModifierPattern : public Pattern
        {
            SSource source;
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                pass->Mov(source, modifier);
            }
        };
        bool match = false;
        e_modifier mod;
        Value* source;
        if (GetModifier(I, mod, source))
        {
            MovModifierPattern* pattern = new (m_allocator) MovModifierPattern();
            pattern->source = GetSource(source, mod, false);
            match = true;
            AddPattern(pattern);
        }
        return match;
    }

    bool CodeGenPatternMatch::MatchFrc(llvm::BinaryOperator& I)
    {
        if (m_ctx->m_DriverInfo.DisableMatchFrcPatternMatch())
        {
            return false;
        }

        struct FrcPattern : public Pattern
        {
            SSource source;
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                pass->Frc(source, modifier);
            }
        };
        IGC_ASSERT(I.getOpcode() == Instruction::FSub);
        llvm::Value* source0 = I.getOperand(0);
        llvm::IntrinsicInst* source1 = llvm::dyn_cast<llvm::IntrinsicInst>(I.getOperand(1));
        bool found = false;
        if (source1 && source1->getIntrinsicID() == Intrinsic::floor)
        {
            if (source1->getOperand(0) == source0)
            {
                found = true;
            }
        }
        if (found)
        {
            FrcPattern* pattern = new (m_allocator) FrcPattern();
            pattern->source = GetSource(source0, true, false);
            AddPattern(pattern);
        }
        return found;
    }

    SSource CodeGenPatternMatch::GetSource(llvm::Value* value, bool modifier, bool regioning)
    {
        llvm::Value* sourceValue = value;
        e_modifier mod = EMOD_NONE;
        if (modifier)
        {
            GetModifier(*sourceValue, mod, sourceValue);
        }
        return GetSource(sourceValue, mod, regioning);
    }

    SSource CodeGenPatternMatch::GetSource(llvm::Value* value, e_modifier mod, bool regioning)
    {
        SSource source;
        GetRegionModifier(source, value, regioning);
        source.value = value;
        source.mod = mod;
        MarkAsSource(value);
        return source;
    }

    void CodeGenPatternMatch::MarkAsSource(llvm::Value* v)
    {
        // update liveness of the sources
        if (IsConstOrSimdConstExpr(v))
        {
            // skip constant
            return;
        }
        if (isa<Instruction>(v) || isa<Argument>(v))
        {
            m_LivenessInfo->HandleVirtRegUse(v, m_root->getParent(), m_root);
        }
        // mark the source as used so that we know we need to generate this value
        if (llvm::Instruction * inst = llvm::dyn_cast<Instruction>(v))
        {
            m_usedInstructions.insert(inst);
        }
        if (m_rootIsSubspanUse)
        {
            HandleSubspanUse(v);
        }
    }

    bool CodeGenPatternMatch::IsSubspanUse(llvm::Value* v)
    {
        return m_subSpanUse.find(v) != m_subSpanUse.end();
    }

    bool CodeGenPatternMatch::MatchFMA(llvm::IntrinsicInst& I)
    {
        struct FMAPattern : Pattern
        {
            SSource sources[3];
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                pass->Mad(sources, modifier);
            }
        };

        FMAPattern* pattern = new (m_allocator)FMAPattern();
        for (int i = 0; i < 3; i++)
        {
            llvm::Value* V = I.getOperand(i);
            pattern->sources[i] = GetSource(V, true, false);
            if (isa<Constant>(V) &&
                (!m_Platform.support16BitImmSrcForMad() ||
                    V->getType()->getTypeID() != llvm::Type::HalfTyID || i == 1))
            {
                //CNL+ mad instruction allows 16 bit immediate for src0 and src2
                AddToConstantPool(I.getParent(), V);
                pattern->sources[i].fromConstantPool = true;
            }
        }
        AddPattern(pattern);

        return true;
    }

    bool CodeGenPatternMatch::MatchPredAdd(llvm::BinaryOperator& I)
    {
        struct PredAddPattern : Pattern
        {
            SSource sources[2];
            SSource pred;
            e_predMode predMode;
            bool invertPred;
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                DstModifier modf = modifier;
                modf.predMode = predMode;
                pass->PredAdd(pred, invertPred, sources, modf);
            }
        };

        if (m_ctx->getModuleMetaData()->isPrecise)
        {
            return false;
        }

        if (m_ctx->type == ShaderType::VERTEX_SHADER ||
            !m_ctx->m_DriverInfo.SupportMatchPredAdd())
        {
            return false;
        }

        bool found = false;

        llvm::Value* sources[2], * pred;
        e_modifier src_mod[2], pred_mod;
        bool invertPred = false;
        if (m_AllowContractions == false || IGC_IS_FLAG_ENABLED(DisableMatchPredAdd))
        {
            return false;
        }

        // Skip the pattern match if FPTrunc/FPEXt is used right after fadd
        if (I.hasOneUse())
        {
            FPTruncInst* FPTrunc = llvm::dyn_cast<llvm::FPTruncInst>(*I.user_begin());
            FPExtInst* FPExt = llvm::dyn_cast<llvm::FPExtInst>  (*I.user_begin());

            if (FPTrunc || FPExt)
            {
                return false;
            }
        }

        IGC_ASSERT(I.getOpcode() == Instruction::FAdd || I.getOpcode() == Instruction::FSub);
        for (uint iAdd = 0; iAdd < 2 && !found; iAdd++)
        {
            Value* src = I.getOperand(iAdd);
            llvm::BinaryOperator* mul = llvm::dyn_cast<llvm::BinaryOperator>(src);
            if (mul && mul->getOpcode() == Instruction::FMul)
            {
                if (!mul->hasOneUse())
                {
                    continue;
                }

                for (uint iMul = 0; iMul < 2; iMul++)
                {
                    if (llvm::SelectInst * selInst = dyn_cast<SelectInst>(mul->getOperand(iMul)))
                    {
                        ConstantFP* C1 = dyn_cast<ConstantFP>(selInst->getOperand(1));
                        ConstantFP* C2 = dyn_cast<ConstantFP>(selInst->getOperand(2));
                        if (C1 && C2 && selInst->hasOneUse())
                        {
                            // select i1 %res_s48, float 1.000000e+00, float 0.000000e+00
                            if ((C2->isZero() && C1->isExactlyValue(1.f)))
                            {
                                invertPred = false;
                            }
                            // select i1 %res_s48, float 0.000000e+00, float 1.000000e+00
                            else if (C1->isZero() && C2->isExactlyValue(1.f))
                            {
                                invertPred = true;
                            }
                            else
                            {
                                continue;
                            }
                        } // if (C1 && C2 && selInst->hasOneUse())
                        else
                        {
                            continue;
                        }

                        //   % 97 = select i1 %res_s48, float 1.000000e+00, float 0.000000e+00
                        //   %102 = fmul fast float %97 %98

                        // case 1 (add)
                        // Before match
                        //   %105 = fadd %102 %103
                        // After match
                        //              %105 = %103
                        //   (%res_s48) %105 = fadd %105 %98

                        // case 2 (fsub match @ iAdd = 0)
                        // Before match
                        //   %105 = fsub %102 %103
                        // After match
                        //              %105 = -%103
                        //   (%res_s48) %105 = fadd %105 %98

                        // case 3 (fsub match @ iAdd = 1)
                        // Before match
                        //   %105 = fsub %103 %102
                        // After match
                        //              %105 = %103
                        //   (%res_s48) %98 = fadd %105 -%98

                        // sources[0]: store add operand (i.e. %103 above)
                        // sources[1]: store mul operand (i.e. %98 above)

                        sources[0] = I.getOperand(1 ^ iAdd);
                        sources[1] = mul->getOperand(1 ^ iMul);
                        pred = selInst->getOperand(0);

                        GetModifier(*sources[0], src_mod[0], sources[0]);
                        GetModifier(*sources[1], src_mod[1], sources[1]);
                        GetModifier(*pred, pred_mod, pred);

                        if (I.getOpcode() == Instruction::FSub)
                        {
                            src_mod[iAdd] = CombineModifier(EMOD_NEG, src_mod[iAdd]);
                        }

                        found = true;
                        break;
                    } //  if (llvm::SelectInst* selInst = dyn_cast<SelectInst>(mul->getOperand(iMul)))
                } // for (uint iMul = 0; iMul < 2; iMul++)
            } // if (mul && mul->getOpcode() == Instruction::FMul)
        } // for (uint iAdd = 0; iAdd < 2; iAdd++)

        if (found)
        {
            PredAddPattern* pattern = new (m_allocator) PredAddPattern();
            pattern->predMode = EPRED_NORMAL;
            pattern->sources[0] = GetSource(sources[0], src_mod[0], false);
            pattern->sources[1] = GetSource(sources[1], src_mod[1], false);
            pattern->pred = GetSource(pred, pred_mod, false);
            pattern->invertPred = invertPred;

            AddPattern(pattern);
        }
        return found;
    }

    // we match the following pattern
    // %c = fcmp %1 %2
    // %g = sext %c to i32
    // %h = and i32 %g 1065353216
    // %m = bitcast i32 %h to float
    // %p = fadd %m %n
    bool CodeGenPatternMatch::MatchSimpleAdd(llvm::BinaryOperator& I)
    {
        struct SimpleAddPattern : public Pattern
        {
            SSource sources[2];
            SSource pred;

            e_predMode predMode;
            bool invertPred;
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                DstModifier modf = modifier;
                modf.predMode = predMode;
                pass->PredAdd(pred, invertPred, sources, modf);
            }
        };

        IGC_ASSERT(I.getOpcode() == Instruction::FAdd);

        if (IGC_IS_FLAG_ENABLED(DisableMatchSimpleAdd))
        {
            return false;
        }

        unsigned int repAddOperand = 0;
        BitCastInst* bitCastInst0 = llvm::dyn_cast<llvm::BitCastInst>(I.getOperand(0));
        BitCastInst* bitCastInst1 = llvm::dyn_cast<llvm::BitCastInst>(I.getOperand(1));
        BitCastInst* bitCastInst = NULL;

        if (!bitCastInst0 && !bitCastInst1)
        {
            return false;
        }

        if (bitCastInst1)
        {
            bitCastInst = bitCastInst1;
            repAddOperand = 1;
        }
        else
        {
            bitCastInst = bitCastInst0;
            repAddOperand = 0;
        }

        if (!bitCastInst->hasOneUse())
        {
            return false;
        }

        Instruction* andInst = dyn_cast<Instruction>(bitCastInst->getOperand(0));
        if (!andInst || (andInst->getOpcode() != Instruction::And))
        {
            return false;
        }

        // check %h = and i32 %g 1065353216
        if (!andInst->getType()->isIntegerTy(32))
        {
            return false;
        }

        ConstantInt* CInt = dyn_cast<ConstantInt>(andInst->getOperand(1));
        if (!CInt || (CInt->getZExtValue() != 0x3f800000))
        {
            return false;
        }

        SExtInst* SExt = llvm::dyn_cast<llvm::SExtInst>(andInst->getOperand(0));
        if (!SExt)
        {
            return false;
        }

        CmpInst* cmp = llvm::dyn_cast<CmpInst>(SExt->getOperand(0));
        if (!cmp)
        {
            return false;
        }

        // match found
        SimpleAddPattern* pattern = new (m_allocator) SimpleAddPattern();
        llvm::Value* sources[2], * pred;
        e_modifier src_mod[2], pred_mod;

        sources[0] = I.getOperand(1 - repAddOperand);
        sources[1] = ConstantFP::get(I.getType(), 1.0);
        pred = cmp;

        GetModifier(*sources[0], src_mod[0], sources[0]);
        GetModifier(*sources[1], src_mod[1], sources[1]);
        GetModifier(*pred, pred_mod, pred);

        pattern->predMode = EPRED_NORMAL;
        pattern->sources[0] = GetSource(sources[0], src_mod[0], false);
        pattern->sources[1] = GetSource(sources[1], src_mod[1], false);
        pattern->pred = GetSource(pred, pred_mod, false);
        pattern->invertPred = false;
        AddPattern(pattern);

        return true;
    }

    bool CodeGenPatternMatch::MatchMad(llvm::BinaryOperator& I)
    {
        struct MadPattern : Pattern
        {
            SSource sources[3];
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                pass->Mad(sources, modifier);
            }
        };

        auto isFpMad = [](const Instruction& I)
        {
            return I.getType()->isFloatingPointTy();
        };

        if (isFpMad(I) && (m_ctx->getModuleMetaData()->isPrecise || m_ctx->getModuleMetaData()->compOpt.disableMathRefactoring))
        {
            return false;
        }
        if (m_ctx->type == ShaderType::VERTEX_SHADER &&
            m_ctx->m_DriverInfo.DisabeMatchMad())
        {
            return false;
        }
        if (IGC_IS_FLAG_ENABLED(DisableMatchMad))
        {
            return false;
        }
        if (isFpMad(I) && m_AllowContractions == false)
        {
            return false;
        }
        if (!isFpMad(I) && !(m_Platform.doIntegerMad() && m_ctx->m_DriverInfo.EnableIntegerMad()))
        {
            return false;
        }

        bool found = false;
        llvm::Value* sources[3];
        e_modifier src_mod[3];

        IGC_ASSERT(I.getOpcode() == Instruction::FAdd ||
            I.getOpcode() == Instruction::FSub ||
            I.getOpcode() == Instruction::Add ||
            I.getOpcode() == Instruction::Sub);
        if (I.getOperand(0) != I.getOperand(1))
        {
            for (uint i = 0; i < 2; i++)
            {
                Value* src = I.getOperand(i);
                if (FPExtInst * fpextInst = llvm::dyn_cast<llvm::FPExtInst>(src))
                {
                    if (!m_Platform.supportMixMode() && fpextInst->getSrcTy()->getTypeID() == llvm::Type::HalfTyID)
                    {
                        // no mix mode instructions
                    }
                    else if (fpextInst->getSrcTy()->getTypeID() != llvm::Type::DoubleTyID &&
                        fpextInst->getDestTy()->getTypeID() != llvm::Type::DoubleTyID)
                    {
                        src = fpextInst->getOperand(0);
                    }
                }
                llvm::BinaryOperator* mul = llvm::dyn_cast<llvm::BinaryOperator>(src);

                if (mul && (mul->getOpcode() == Instruction::FMul ||
                    mul->getOpcode() == Instruction::Mul))
                {
                    // in case we know we won't be able to remove the mul we don't merge it
                    if (!m_PosDep->PositionDependsOnInst(mul) && NeedInstruction(*mul))
                        continue;
                    sources[2] = I.getOperand(1 - i);
                    sources[1] = mul->getOperand(0);
                    sources[0] = mul->getOperand(1);
                    GetModifier(*sources[0], src_mod[0], sources[0]);
                    GetModifier(*sources[1], src_mod[1], sources[1]);
                    GetModifier(*sources[2], src_mod[2], sources[2]);
                    if (I.getOpcode() == Instruction::FSub ||
                        I.getOpcode() == Instruction::Sub)
                    {
                        if (i == 0)
                        {
                            src_mod[2] = CombineModifier(EMOD_NEG, src_mod[2]);
                        }
                        else
                        {
                            if (llvm::isa<llvm::Constant>(sources[0]))
                            {
                                src_mod[1] = CombineModifier(EMOD_NEG, src_mod[1]);
                            }
                            else
                            {
                                src_mod[0] = CombineModifier(EMOD_NEG, src_mod[0]);
                            }
                        }
                    }
                    found = true;
                    break;
                }
            }
        }

        // Check integer mad profitability.
        if (found && !isFpMad(I))
        {
            uint8_t numConstant = 0;
            for (int i = 0; i < 3; i++)
            {
                if (isa<Constant>(sources[i]))
                    numConstant++;

                // Only one immediate is supported
                if (numConstant > 1)
                    return false;
            }

            auto isByteOrWordValue = [](Value* V) -> bool {
                if (isa<ConstantInt>(V))
                {
                    // only 16-bit int immediate is supported
                    APInt val = dyn_cast<ConstantInt>(V)->getValue();
                    return val.sge(SHRT_MIN) && val.sle(SHRT_MAX);
                }
                // Trace the def-use chain and return the first non up-cast related value.
                while (isa<ZExtInst>(V) || isa<SExtInst>(V) || isa<BitCastInst>(V))
                    V = cast<Instruction>(V)->getOperand(0);
                const unsigned DWordSizeInBits = 32;
                return V->getType()->getScalarSizeInBits() < DWordSizeInBits;
            };

            // One multiplicant should be *W or *B.
            if (!isByteOrWordValue(sources[0]) && !isByteOrWordValue(sources[1]))
                return false;
        }

        if (found)
        {
            MadPattern* pattern = new (m_allocator) MadPattern();
            for (int i = 0; i < 3; i++)
            {
                pattern->sources[i] = GetSource(sources[i], src_mod[i], false);
                if (isa<Constant>(sources[i]) &&
                    (!m_Platform.support16BitImmSrcForMad() ||
                    (!sources[i]->getType()->isHalfTy() && !sources[i]->getType()->isIntegerTy()) || i == 1))
                {
                    //CNL+ mad instruction allows 16 bit immediate for src0 and src2
                    AddToConstantPool(I.getParent(), sources[i]);
                    pattern->sources[i].fromConstantPool = true;
                }
            }
            AddPattern(pattern);
        }
        return found;
    }

    // match simdblockRead/Write with preceding inttoptr if possible
    // to save a copy move
    bool CodeGenPatternMatch::MatchBlockReadWritePointer(llvm::GenIntrinsicInst& I)
    {
        struct BlockReadWritePointerPattern : public Pattern
        {
            GenIntrinsicInst* inst;
            Value* offset;
            explicit BlockReadWritePointerPattern(GenIntrinsicInst* I, Value* ofst) : inst(I), offset(ofst) {}
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                switch (inst->getIntrinsicID())
                {
                case GenISAIntrinsic::GenISA_simdBlockRead:
                    pass->emitSimdBlockRead(inst, offset);
                    break;
                case GenISAIntrinsic::GenISA_simdBlockWrite:
                    pass->emitSimdBlockWrite(inst, offset);
                    break;
                default:
                    IGC_ASSERT_MESSAGE(0, "unexpected intrinsic");
                    break;
                }
            }
        };

        if (I.getIntrinsicID() != GenISAIntrinsic::GenISA_simdBlockRead &&
            I.getIntrinsicID() != GenISAIntrinsic::GenISA_simdBlockWrite)
        {
            return false;
        }

        // check the address is inttoptr with same dst and src type width
        auto ptrVal = I.getOperand(0);
        auto I2P = dyn_cast<IntToPtrInst>(ptrVal);
        if (I2P &&
            I2P->getOperand(0)->getType()->getIntegerBitWidth() ==
            m_ctx->getRegisterPointerSizeInBits(I2P->getAddressSpace()))
        {
            auto addrBase = I2P->getOperand(0);
            BlockReadWritePointerPattern* pattern = new (m_allocator) BlockReadWritePointerPattern(&I, addrBase);
            MarkAsSource(addrBase);
            // for write mark data ptr as well
            if (I.getIntrinsicID() == GenISAIntrinsic::GenISA_simdBlockWrite)
            {
                MarkAsSource(I.getOperand(1));
            }

            AddPattern(pattern);
            return true;
        }
        return false;
    }

    // 1. Detect and handle immediate URB read offsets - these can be put in message descriptor.
    // 2. Detect offsets of the form "add dst, var, imm" - here we can remove the add, putting imm in message descriptor,
    // and var in message payload.
    bool CodeGenPatternMatch::MatchURBRead(llvm::GenIntrinsicInst& I)
    {
        struct URBReadPattern : public Pattern
        {
            explicit URBReadPattern(GenIntrinsicInst* I, QuadEltUnit globalOffset, llvm::Value* const perSlotOffset) :
                m_inst(I), m_globalOffset(globalOffset), m_perSlotOffset(perSlotOffset)
            {}

            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                IGC_ASSERT(m_inst->getIntrinsicID() == GenISAIntrinsic::GenISA_URBRead ||
                    m_inst->getIntrinsicID() == GenISAIntrinsic::GenISA_URBReadOutput);
                pass->emitURBReadCommon(m_inst, m_globalOffset, m_perSlotOffset);
            }

        private:
            GenIntrinsicInst* const m_inst;
            const QuadEltUnit m_globalOffset;
            llvm::Value* const m_perSlotOffset;
        };

        if (I.getIntrinsicID() != GenISAIntrinsic::GenISA_URBRead &&
            I.getIntrinsicID() != GenISAIntrinsic::GenISA_URBReadOutput)
        {
            return false;
        }

        const bool hasVertexIndexAsArg0 = I.getIntrinsicID() == GenISAIntrinsic::GenISA_URBRead;
        llvm::Value* const offset = I.getOperand(hasVertexIndexAsArg0 ? 1 : 0);
        if (const ConstantInt * const constOffset = dyn_cast<ConstantInt>(offset))
        {
            const QuadEltUnit globalOffset = QuadEltUnit(int_cast<unsigned>(constOffset->getZExtValue()));
            if (hasVertexIndexAsArg0)
            {
                MarkAsSource(I.getOperand(0));
            }
            URBReadPattern* pattern = new (m_allocator) URBReadPattern(&I, globalOffset, nullptr);
            AddPattern(pattern);
            return true;
        }
        else if (llvm::Instruction * const inst = llvm::dyn_cast<llvm::Instruction>(offset))
        {
            if (inst->getOpcode() == llvm::Instruction::Add)
            {
                const bool isConstant0 = llvm::isa<llvm::ConstantInt>(inst->getOperand(0));
                const bool isConstant1 = llvm::isa<llvm::ConstantInt>(inst->getOperand(1));
                if (isConstant0 || isConstant1)
                {
                    IGC_ASSERT_MESSAGE(!(isConstant0 && isConstant1), "Both operands are immediate - constants should be folded elsewhere.");

                    if (hasVertexIndexAsArg0)
                    {
                        MarkAsSource(I.getOperand(0));
                    }
                    const QuadEltUnit globalOffset = QuadEltUnit(int_cast<unsigned>(cast<ConstantInt>(
                        isConstant0 ? inst->getOperand(0) : inst->getOperand(1))->getZExtValue()));
                    llvm::Value* const perSlotOffset = isConstant0 ? inst->getOperand(1) : inst->getOperand(0);
                    MarkAsSource(perSlotOffset);
                    URBReadPattern* pattern = new (m_allocator) URBReadPattern(&I, globalOffset, perSlotOffset);
                    AddPattern(pattern);
                    return true;
                }
            }
        }

        return false;
    }


    bool CodeGenPatternMatch::MatchLoadStorePointer(llvm::Instruction& I, llvm::Value& ptrVal)
    {
        struct LoadStorePointerPattern : public Pattern
        {
            Instruction* inst;
            Value* offset;
            ConstantInt* immOffset;
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                if (isa<LoadInst>(inst))
                {
                    pass->emitLoad(cast<LoadInst>(inst), offset, immOffset);
                }
                else if (isa<StoreInst>(inst))
                {
                    pass->emitStore3D(cast<StoreInst>(inst), offset);
                }
            }
        };
        GenIntrinsicInst* ptr = dyn_cast<GenIntrinsicInst>(&ptrVal);
        IntToPtrInst* i2p = dyn_cast<IntToPtrInst>(&ptrVal);
        if (ptrVal.getType()->getPointerAddressSpace() == ADDRESS_SPACE_GLOBAL ||
            ptrVal.getType()->getPointerAddressSpace() == ADDRESS_SPACE_CONSTANT)
        {
            return false;
        }

        // Store3d supports only types equal or less than 128 bits.
        StoreInst* storeInst = dyn_cast<StoreInst>(&I);
        if (storeInst && storeInst->getValueOperand()->getType()->getPrimitiveSizeInBits() > 128)
        {
            return false;
        }

        if (i2p || (ptr && ptr->getIntrinsicID() == GenISAIntrinsic::GenISA_OWordPtr))
        {
            LoadStorePointerPattern* pattern = new (m_allocator) LoadStorePointerPattern();
            pattern->inst = &I;
            uint numSources = GetNbSources(I);
            for (uint i = 0; i < numSources; i++)
            {
                if (I.getOperand(i) != &ptrVal)
                {
                    MarkAsSource(I.getOperand(i));
                }
            }
            pattern->offset = cast<Instruction>(&ptrVal)->getOperand(0);
            pattern->immOffset = ConstantInt::get(Type::getInt32Ty(I.getContext()), 0);
            MarkAsSource(pattern->offset);
            AddPattern(pattern);
            return true;
        }
        return false;
    }

    bool CodeGenPatternMatch::MatchLrp(llvm::BinaryOperator& I)
    {
        struct LRPPattern : public Pattern
        {
            SSource sources[3];
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                pass->Lrp(sources, modifier);
            }
        };

        if (!I.getType()->isFloatTy())
            return false;
        if (!m_Platform.supportLRPInstruction())
            return false;

        if (m_ctx->getModuleMetaData()->isPrecise)
        {
            return false;
        }

        bool found = false;
        llvm::Value* sources[3];
        e_modifier   src_mod[3];

        if (m_AllowContractions == false)
        {
            return false;
        }

        IGC_ASSERT((I.getOpcode() == Instruction::FAdd) || (I.getOpcode() == Instruction::FSub));

        bool startPatternIsAdd = false;
        if (I.getOpcode() == Instruction::FAdd)
        {
            startPatternIsAdd = true;
        }

        // match the case: dst = src0 (src1 - src2)  + src2;
        for (uint i = 0; i < 2; i++)
        {
            llvm::BinaryOperator* mul = llvm::dyn_cast<llvm::BinaryOperator>(I.getOperand(i));
            if (mul && mul->getOpcode() == Instruction::FMul)
            {
                for (uint j = 0; j < 2; j++)
                {
                    llvm::BinaryOperator* sub = llvm::dyn_cast<llvm::BinaryOperator>(mul->getOperand(j));
                    if (sub)
                    {
                        llvm::ConstantFP* zero = llvm::dyn_cast<llvm::ConstantFP>(sub->getOperand(0));
                        if (zero && zero->isExactlyValue(0.f))
                        {
                            // in this case we can optimize the pattern into fmad and give better result
                            continue;
                        }

                        if ((startPatternIsAdd && sub->getOpcode() == Instruction::FSub) ||
                            (!startPatternIsAdd && i == 0 && sub->getOpcode() == Instruction::FAdd))
                        {
                            if (sub->getOperand(1) == I.getOperand(1 - i) &&
                                mul->getOperand(0) != mul->getOperand(1))
                            {
                                sources[0] = mul->getOperand(1 - j);
                                sources[1] = sub->getOperand(0);
                                sources[2] = sub->getOperand(1);
                                GetModifier(*sources[0], src_mod[0], sources[0]);
                                GetModifier(*sources[1], src_mod[1], sources[1]);
                                GetModifier(*sources[2], src_mod[2], sources[2]);

                                if (!startPatternIsAdd && i == 0)
                                {
                                    // handle patterns like this:
                                    // dst = src0 (src1 + src2) - src2;
                                    src_mod[2] = CombineModifier(EMOD_NEG, src_mod[2]);
                                }

                                found = true;
                                break;
                            }
                        }
                    }
                }
            }
            if (found)
            {
                break;
            }
        }

        // match the case: dst = src0 * src1 + src2 * (1.0 - src0);
        if (!found)
        {
            llvm::BinaryOperator* mul[2];
            mul[0] = llvm::dyn_cast<llvm::BinaryOperator>(I.getOperand(0));
            mul[1] = llvm::dyn_cast<llvm::BinaryOperator>(I.getOperand(1));
            if (mul[0] && mul[0]->getOpcode() == Instruction::FMul &&
                mul[1] && mul[1]->getOpcode() == Instruction::FMul &&
                !llvm::isa<llvm::ConstantFP>(mul[0]->getOperand(0)) &&
                !llvm::isa<llvm::ConstantFP>(mul[0]->getOperand(1)) &&
                !llvm::isa<llvm::ConstantFP>(mul[1]->getOperand(0)) &&
                !llvm::isa<llvm::ConstantFP>(mul[1]->getOperand(1)))
            {
                for (uint i = 0; i < 2; i++)
                {
                    for (uint j = 0; j < 2; j++)
                    {
                        llvm::BinaryOperator* sub = llvm::dyn_cast<llvm::BinaryOperator>(mul[i]->getOperand(j));
                        if (sub && sub->getOpcode() == Instruction::FSub)
                        {
                            llvm::ConstantFP* one = llvm::dyn_cast<llvm::ConstantFP>(sub->getOperand(0));
                            if (one && one->isExactlyValue(1.f))
                            {
                                for (uint k = 0; k < 2; k++)
                                {
                                    if (sub->getOperand(1) == mul[1 - i]->getOperand(k))
                                    {
                                        sources[0] = sub->getOperand(1);
                                        sources[1] = mul[1 - i]->getOperand(1 - k);
                                        sources[2] = mul[i]->getOperand(1 - j);
                                        GetModifier(*sources[0], src_mod[0], sources[0]);
                                        GetModifier(*sources[1], src_mod[1], sources[1]);
                                        GetModifier(*sources[2], src_mod[2], sources[2]);
                                        if (!startPatternIsAdd)
                                        {
                                            if (i == 1)
                                            {
                                                // handle patterns like this:
                                                // dst = (src1 * src0) - (src2 * (1.0 - src0))
                                                src_mod[2] = CombineModifier(EMOD_NEG, src_mod[2]);
                                            }
                                            else
                                            {
                                                // handle patterns like this:
                                                // dst = (src2 * (1.0 - src0)) - (src1 * src0)
                                                src_mod[1] = CombineModifier(EMOD_NEG, src_mod[1]);
                                            }
                                        }
                                        found = true;
                                        break;
                                    }
                                }
                            }
                        }
                        if (found)
                        {
                            break;
                        }
                    }
                    if (found)
                    {
                        break;
                    }
                }
            }
        }

        if (!found)
        {
            // match the case: dst = src2 - (src0 * src2) + (src0 * src1);
            // match the case: dst = (src0 * src1) + src2 - (src0 * src2);
            // match the case: dst = src2 + (src0 * src1) - (src0 * src2);
            if (I.getOpcode() == Instruction::FAdd || I.getOpcode() == Instruction::FSub)
            {
                // dst = op[0] +/- op[1] +/- op[2]
                llvm::Instruction* op[3];
                llvm::Instruction* addSub1 = llvm::dyn_cast<llvm::Instruction>(I.getOperand(0));
                if (addSub1 && (addSub1->getOpcode() == Instruction::FSub || addSub1->getOpcode() == Instruction::FAdd))
                {
                    op[0] = llvm::dyn_cast<llvm::Instruction>(addSub1->getOperand(0));
                    op[1] = llvm::dyn_cast<llvm::Instruction>(addSub1->getOperand(1));
                    op[2] = llvm::dyn_cast<llvm::Instruction>(I.getOperand(1));

                    if (op[0] && op[1] && op[2])
                    {
                        for (uint casei = 0; casei < 3; casei++)
                        {
                            // i, j, k are the index for op[]
                            uint i = (casei == 2 ? 1 : 0);
                            uint j = (casei == 0 ? 1 : 2);
                            uint k = 2 - casei;

                            //op[i] and op[j] should be fMul, and op[k] is src2
                            if (op[i]->getOpcode() == Instruction::FMul && op[j]->getOpcode() == Instruction::FMul)
                            {
                                for (uint srci = 0; srci < 2; srci++)
                                {
                                    for (uint srcj = 0; srcj < 2; srcj++)
                                    {
                                        // op[i] and op[j] needs to have one common source. this common source will be src0
                                        if (op[i]->getOperand(srci) == op[j]->getOperand(srcj))
                                        {
                                            // one of the non-common source from op[i] and op[j] needs to be the same as op[k], which is src2
                                            if (op[k] == op[i]->getOperand(1 - srci) ||
                                                op[k] == op[j]->getOperand(1 - srcj))
                                            {
                                                // disable if any of the sources is immediate
                                                if (llvm::isa<llvm::ConstantFP>(op[i]->getOperand(srci)) ||
                                                    llvm::isa<llvm::ConstantFP>(op[i]->getOperand(1 - srci)) ||
                                                    llvm::isa<llvm::ConstantFP>(op[j]->getOperand(srcj)) ||
                                                    llvm::isa<llvm::ConstantFP>(op[j]->getOperand(1 - srcj)) ||
                                                    llvm::isa<llvm::ConstantFP>(op[k]))
                                                {
                                                    break;
                                                }

                                                // check the add/sub cases and add negate to the sources when needed.
                                                /*
                                                ( src0src1, -src0src2, src2 )   okay
                                                ( src0src1, -src0src2, -src2 )  skip
                                                ( src0src1, src0src2, src2 )    skip
                                                ( src0src1, src0src2, -src2 )   negate src2
                                                ( -src0src1, -src0src2, src2 )  negate src1
                                                ( -src0src1, -src0src2, -src2 ) skip
                                                ( -src0src1, src0src2, src2 )   skip
                                                ( -src0src1, src0src2, -src2 )  negate src1 src2
                                                */

                                                bool SignPositiveOp[3];
                                                SignPositiveOp[0] = true;
                                                SignPositiveOp[1] = (addSub1->getOpcode() == Instruction::FAdd);
                                                SignPositiveOp[2] = (I.getOpcode() == Instruction::FAdd);

                                                uint mulSrc0Src1Index = op[k] == op[i]->getOperand(1 - srci) ? j : i;
                                                uint mulSrc0Src2Index = op[k] == op[i]->getOperand(1 - srci) ? i : j;

                                                if (SignPositiveOp[mulSrc0Src2Index] == SignPositiveOp[k])
                                                {
                                                    // abort the cases marked as "skip" in the comment above
                                                    break;
                                                }

                                                sources[0] = op[i]->getOperand(srci);
                                                sources[1] = op[k] == op[i]->getOperand(1 - srci) ? op[j]->getOperand(1 - srcj) : op[i]->getOperand(1 - srci);
                                                sources[2] = op[k];
                                                GetModifier(*sources[0], src_mod[0], sources[0]);
                                                GetModifier(*sources[1], src_mod[1], sources[1]);
                                                GetModifier(*sources[2], src_mod[2], sources[2]);

                                                if (SignPositiveOp[mulSrc0Src1Index] == false)
                                                {
                                                    src_mod[1] = CombineModifier(EMOD_NEG, src_mod[1]);
                                                }
                                                if (SignPositiveOp[k] == false)
                                                {
                                                    src_mod[2] = CombineModifier(EMOD_NEG, src_mod[2]);
                                                }

                                                found = true;
                                            }
                                        }
                                    }
                                    if (found)
                                    {
                                        break;
                                    }
                                }
                            }
                            if (found)
                            {
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (found)
        {
            LRPPattern* pattern = new (m_allocator) LRPPattern();
            for (int i = 0; i < 3; i++)
            {
                pattern->sources[i] = GetSource(sources[i], src_mod[i], false);
            }
            AddPattern(pattern);
        }
        return found;
    }

    bool CodeGenPatternMatch::MatchCmpSext(llvm::Instruction& I)
    {
        /*
            %res_s42 = icmp eq i32 %src1_s41, 0
            %17 = sext i1 %res_s42 to i32
                to
            %res_s42 (i32) = icmp eq i32 %src1_s41, 0


            %res_s73 = fcmp oge float %res_s61, %42
            %46 = sext i1 %res_s73 to i32
                to
            %res_s73 (i32) = fcmp oge float %res_s61, %42
        */

        struct CmpSextPattern : Pattern
        {
            llvm::CmpInst* inst;
            SSource sources[2];
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                pass->Cmp(inst->getPredicate(), sources, modifier);
            }
        };
        bool match = false;

        if (CmpInst * cmpInst = dyn_cast<CmpInst>(I.getOperand(0)))
        {
            if (cmpInst->getOperand(0)->getType()->getPrimitiveSizeInBits() == I.getType()->getPrimitiveSizeInBits())
            {
                CmpSextPattern* pattern = new (m_allocator) CmpSextPattern();
                bool supportModifer = SupportsModifier(cmpInst);

                pattern->inst = cmpInst;
                pattern->sources[0] = GetSource(cmpInst->getOperand(0), supportModifer, false);
                pattern->sources[1] = GetSource(cmpInst->getOperand(1), supportModifer, false);
                AddPattern(pattern);
                match = true;
            }
        }

        return match;
    }

    // Match the pattern of 32 x 32 = 64, a full 32-bit multiplication.
    bool CodeGenPatternMatch::MatchFullMul32(llvm::Instruction& I) {
        using namespace llvm::PatternMatch; // Scoped namespace using.

        struct FullMul32Pattern : public Pattern {
            SSource srcs[2];
            bool isUnsigned;
            virtual void Emit(EmitPass* pass, const DstModifier& dstMod)
            {
                pass->EmitFullMul32(isUnsigned, srcs, dstMod);
            }
        };

        IGC_ASSERT_MESSAGE(I.getOpcode() == llvm::Instruction::Mul, "Mul instruction is expected!");

        if (!I.getType()->isIntegerTy(64))
            return false;

        llvm::Value* LHS = I.getOperand(0);
        llvm::Value* RHS = I.getOperand(1);

        // Swap operand to ensure the constant is always RHS.
        if (isa<ConstantInt>(LHS))
            std::swap(LHS, RHS);

        bool IsUnsigned = false;
        llvm::Value* L = nullptr;
        llvm::Value* R = nullptr;

        // Check LHS
        if (match(LHS, m_SExt(m_Value(L)))) {
            // Bail out if there's non 32-bit integer.
            if (!L->getType()->isIntegerTy(32))
                return false;
        }
        else if (match(LHS, m_ZExt(m_Value(L)))) {
            // Bail out if there's non 32-bit integer.
            if (!L->getType()->isIntegerTy(32))
                return false;
            IsUnsigned = true;
        }
        else {
            // Bailout if it's unknown that LHS have less significant bits than the
            // product.
            // NOTE we don't assertion fail the case where LHS is an constant to prevent
            // the assertion in O0 mode. Otherwise, we expect there's at most 1
            // constant operand.
            return false;
        }

        // Check RHS
        if (match(RHS, m_SExt(m_Value(R)))) {
            // Bail out if there's signedness mismatch or non 32-bit integer.
            if (IsUnsigned || !R->getType()->isIntegerTy(32))
                return false;
        }
        else if (match(RHS, m_ZExt(m_Value(R)))) {
            // Bail out if there's signedness mismatch or non 32-bit integer.
            if (!IsUnsigned || !R->getType()->isIntegerTy(32))
                return false;
            IsUnsigned = true;
        }
        else if (ConstantInt * CI = dyn_cast<ConstantInt>(RHS)) {
            APInt Val = CI->getValue();
            // 31-bit unsigned integer could be used as either signed or
            // unsigned one. Otherwise, we need special check how MSB is used.
            if (!Val.isIntN(31)) {
                if (!(IsUnsigned && Val.isIntN(32)) &&
                    !(!IsUnsigned && Val.isSignedIntN(32))) {
                    return false;
                }
            }
            R = ConstantExpr::getTrunc(CI, L->getType());
        }
        else {
            // Bailout if it's unknown that RHS have less significant bits than the
            // product.
            return false;
        }

        FullMul32Pattern* Pat = new (m_allocator) FullMul32Pattern();
        Pat->srcs[0] = GetSource(L, !IsUnsigned, false);
        Pat->srcs[1] = GetSource(R, !IsUnsigned, false);
        Pat->isUnsigned = IsUnsigned;
        AddPattern(Pat);

        return true;
    }

    // For 32 bit integer mul/add/sub, use 16bit operands if possible. Thus,
    // This will match 16x16->32, 16x32->32, the same for add/sub.
    //
    // For example:
    //   1.  before:
    //        %9 = ashr i32 %8, 16
    //        %10 = mul nsw i32 %9, -1024
    //        ( asr (16|M0)  r19.0<1>:d  r19.0<8;8,1>:d  16:w
    //          mul (16|M0)  r19.0<1>:d  r19.0<8;8,1>:d  -1024:w )
    //
    //      after:
    //      --> %10:d = mul %9.1<16;8:2>:w -1024:w
    //          (  mul (16|M0)  r23.0<1>:d   r19.1<2;1,0>:w   -1024:w )
    //
    //  2. before:
    //        %9  = lshr i32 %8, 16
    //        %10 = and i32 %8, 65535
    //        %11 = mul nuw i32 %9, %10
    //        ( shr  (16|M0)   r14.0<1>:d  r12.0<8;8,1>:ud   16:w
    //          and(16 | M0)   r12.0<1>:d  r12.0<8;8,1>:d  65535:d
    //          mul(16 | M0)   r14.0<1>:d  r14.0<8;8,1>:d  r12.0<8;8,1>:d )
    //
    //     after:
    //     --> %11:d = mul %8.1<16;8,2>:uw %8.0<16;8,2>:uw
    //         ( mul (16|M0)  r14.0<1>:d   r12.1<2;1,0>:w   r12.0<2;1,0>:w )
    //
    bool CodeGenPatternMatch::MatchMulAdd16(Instruction& I) {
        using namespace llvm::PatternMatch;

        struct Oprd16Pattern : public Pattern {
            SSource srcs[2];
            Instruction* rootInst;
            virtual void Emit(EmitPass* pass, const DstModifier& dstMod)
            {
                pass->emitMulAdd16(rootInst, srcs, dstMod);
            }
        };

        // The code is under the control of registry key EnableMixIntOperands.
        if (IGC_IS_FLAG_DISABLED(EnableMixIntOperands))
        {
            return false;
        }

        unsigned opc = I.getOpcode();
        IGC_ASSERT_MESSAGE((opc == Instruction::Mul) || (opc == Instruction::Add) || (opc == Instruction::Sub), "Mul instruction is expected!");

        // Handle 32 bit integer mul/add/sub only.
        if (!I.getType()->isIntegerTy(32))
        {
            return false;
        }

        // Try to replace any source operands with ones of type short if any. As vISA
        // allows the mix of any integer type, each operand is considered separately.
        struct {
            Value* src;
            bool useLower;
            bool isSigned;
        } oprdInfo[2];
        bool isCandidate = false;

        for (int i = 0; i < 2; ++i)
        {
            Value* oprd = I.getOperand(i);
            Value* L;

            // oprdInfo[i].src == null --> no W operand replacement.
            oprdInfo[i].src = nullptr;
            if (ConstantInt * CI = dyn_cast<ConstantInt>(oprd))
            {
                int64_t val =
                    CI->isNegative() ? CI->getSExtValue() : CI->getZExtValue();
                // If src needs to be negated (y = x - a = x + (-a), as gen only
                // has add), need to check if the negated src fits into W/UW.
                bool isNegSrc = (opc == Instruction::Sub && i == 1);
                if (isNegSrc)
                {
                    val = -val;
                }
                if (INT16_MIN <= val && val <= INT16_MAX)
                {
                    oprdInfo[i].src = oprd;
                    oprdInfo[i].useLower = true; // does not matter for const
                    oprdInfo[i].isSigned = true;
                    isCandidate = true;
                }
                else if (0 <= val && val <= UINT16_MAX)
                {
                    oprdInfo[i].src = oprd;
                    oprdInfo[i].useLower = true; // does not matter for const
                    oprdInfo[i].isSigned = false;
                    isCandidate = true;
                }
            }
            else if (match(oprd, m_And(m_Value(L), m_SpecificInt(0xFFFF))))
            {
                oprdInfo[i].src = L;
                oprdInfo[i].useLower = true;
                oprdInfo[i].isSigned = false;
                isCandidate = true;
            }
            else if (match(oprd, m_LShr(m_Value(L), m_SpecificInt(16))))
            {
                oprdInfo[i].src = L;
                oprdInfo[i].useLower = false;
                oprdInfo[i].isSigned = false;
                isCandidate = true;
            }
            else if (match(oprd, m_AShr(m_Shl(m_Value(L), m_SpecificInt(16)),
                m_SpecificInt(16))))
            {
                oprdInfo[i].src = L;
                oprdInfo[i].useLower = true;
                oprdInfo[i].isSigned = true;
                isCandidate = true;
            }
            else if (match(oprd, m_AShr(m_Value(L), m_SpecificInt(16))))
            {
                oprdInfo[i].src = L;
                oprdInfo[i].useLower = false;
                oprdInfo[i].isSigned = true;
                isCandidate = true;
            }
        }

        if (!isCandidate) {
            return false;
        }

        Oprd16Pattern* Pat = new (m_allocator)Oprd16Pattern();
        for (int i = 0; i < 2; ++i)
        {
            if (oprdInfo[i].src)
            {
                Pat->srcs[i] = GetSource(oprdInfo[i].src, false, false);
                SSource& thisSrc = Pat->srcs[i];

                // for now, Use W/UW only if region_set is false or the src is scalar
                if (thisSrc.region_set &&
                    !(thisSrc.region[0] == 0 && thisSrc.region[1] == 1 && thisSrc.region[2] == 0))
                {
                    Pat->srcs[i] = GetSource(I.getOperand(i), true, false);
                }
                else
                {
                    // Note that SSource's type, if set by GetSource(), should be 32bit type. It's
                    // safe to override it with either UW or W. But for SSource's offset, need to
                    // re-calculate in term of 16bit, not 32bit.
                    thisSrc.type = oprdInfo[i].isSigned ? ISA_TYPE_W : ISA_TYPE_UW;
                    thisSrc.elementOffset = (2 * thisSrc.elementOffset) + (oprdInfo[i].useLower ? 0 : 1);
                }
            }
            else
            {
                Pat->srcs[i] = GetSource(I.getOperand(i), true, false);
            }
        }
        Pat->rootInst = &I;
        AddPattern(Pat);

        return true;
    }


    bool CodeGenPatternMatch::BitcastSearch(SSource& source, llvm::Value*& value, bool broadcast)
    {
        if (auto elemInst = dyn_cast<ExtractElementInst>(value))
        {
            if (auto bTInst = dyn_cast<BitCastInst>(elemInst->getOperand(0)))
            {
                // Pattern Matching (Instruction) + ExtractElem + (Vector)Bitcast
                //
                // In order to set the regioning for the ALU operand
                // I require three things:
                //      -The first is the source number of elements
                //      -The second is the destination number of elements
                //      -The third is the index from the extract element
                //
                // For example if I have <4 x i32> to <16 x i8> all I need is
                // the 4 (vstride) and the i8 (b) in this case the operand would look
                // like this -> r22.x <4;1,0>:b
                // x is calculated below and later on using the simdsize

                uint32_t index, srcNElts, dstNElts, nEltsRatio;
                llvm::Type* srcTy = bTInst->getOperand(0)->getType();
                llvm::Type* dstTy = bTInst->getType();

                srcNElts = (srcTy->isVectorTy()) ? srcTy->getVectorNumElements() : 1;
                dstNElts = (dstTy->isVectorTy()) ? dstTy->getVectorNumElements() : 1;

                if (srcNElts < dstNElts && srcTy->getScalarSizeInBits() < 64)
                {
                    if (isa<ConstantInt>(elemInst->getIndexOperand()))
                    {
                        index = int_cast<uint>(cast<ConstantInt>(elemInst->getIndexOperand())->getZExtValue());
                        nEltsRatio = dstNElts / srcNElts;
                        source.value = bTInst->getOperand(0);
                        source.SIMDOffset = iSTD::RoundDownNonPow2(index, nEltsRatio);
                        source.elementOffset = source.elementOffset * nEltsRatio + index % nEltsRatio;
                        value = source.value;
                        if (!broadcast)
                        {
                            source.region_set = true;
                            if (m_WI->whichDepend(value) == WIAnalysis::UNIFORM)
                                source.region[0] = 0;
                            else
                                source.region[0] = (unsigned char)nEltsRatio;
                            source.region[1] = 1;
                            source.region[2] = 0;
                        }
                        return true;
                    }
                }
            }
        }
        return false;
    }

    bool CodeGenPatternMatch::MatchModifier(llvm::Instruction& I, bool SupportSrc0Mod)
    {
        struct ModifierPattern : public Pattern
        {
            SSource sources[2];
            llvm::Instruction* instruction;
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                pass->BinaryUnary(instruction, sources, modifier);
            }
        };

        ModifierPattern* pattern = new (m_allocator)ModifierPattern();
        pattern->instruction = &I;
        uint nbSources = GetNbSources(I);

        bool supportModifer = SupportsModifier(&I);
        bool supportRegioning = SupportsRegioning(&I);
        pattern->sources[0] = GetSource(I.getOperand(0), supportModifer && SupportSrc0Mod, supportRegioning);
        if (nbSources > 1)
        {
            pattern->sources[1] = GetSource(I.getOperand(1), supportModifer, supportRegioning);
        }

        if (nbSources > 1)
        {
            // add df imm to constant pool for binary/ternary inst
            // we do 64-bit int imm bigger than 32 bits, since smaller may fit in D/W
            for (int i = 0, numSrc = (int)nbSources; i < numSrc; ++i)
            {
                Value* op = I.getOperand(i);
                if (isCandidateForConstantPool(op))
                {
                    AddToConstantPool(I.getParent(), op);
                    pattern->sources[i].fromConstantPool = true;
                }
            }
        }

        AddPattern(pattern);

        return true;
    }

    bool CodeGenPatternMatch::MatchSingleInstruction(llvm::Instruction& I)
    {
        struct SingleInstPattern : Pattern
        {
            llvm::Instruction* inst;
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                IGC_ASSERT(modifier.sat == false);
                IGC_ASSERT(modifier.flag == nullptr);
                pass->EmitNoModifier(inst);
            }
        };
        SingleInstPattern* pattern = new (m_allocator) SingleInstPattern();
        pattern->inst = &I;
        uint numSources = GetNbSources(I);
        for (uint i = 0; i < numSources; i++)
        {
            MarkAsSource(I.getOperand(i));
        }

        if (CallInst * callinst = dyn_cast<CallInst>(&I))
        {
            // Mark the function pointer in indirect calls as a source
            if (!callinst->getCalledFunction())
            {
                MarkAsSource(callinst->getCalledValue());
            }
        }
        AddPattern(pattern);
        return true;
    }

    bool CodeGenPatternMatch::MatchCanonicalizeInstruction(llvm::Instruction& I)
    {
        struct CanonicalizeInstPattern : Pattern
        {
            CanonicalizeInstPattern(llvm::Instruction* pInst, bool isNeeded) : m_pInst(pInst), m_IsNeeded(isNeeded) {}

            llvm::Instruction* m_pInst;
            bool m_IsNeeded;

            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                IGC_ASSERT(modifier.sat == false && modifier.flag == nullptr);
                if (m_IsNeeded)
                {
                    pass->emitCanonicalize(m_pInst);
                }
            }
        };

        IGC_ASSERT(I.getNumOperands() == 1);
        bool isNeeded = true;

        // FAdd, FSub, FMul, FDiv instructions flush subnormals to zero.
        // However, mix mode and math instructions preserve subnormals.
        // Other instructions also preserve subnormals.
        if (llvm::BinaryOperator * pBianaryOperator = llvm::dyn_cast<llvm::BinaryOperator>(I.getOperand(0)))
        {
            switch (pBianaryOperator->getOpcode())
            {
            case llvm::BinaryOperator::BinaryOps::FAdd:
            case llvm::BinaryOperator::BinaryOps::FMul:
            case llvm::BinaryOperator::BinaryOps::FSub:
            case llvm::BinaryOperator::BinaryOps::FDiv:
                isNeeded = false;
            default:
                break;
            }
        }

        CanonicalizeInstPattern* pattern = new (m_allocator) CanonicalizeInstPattern(&I, isNeeded);
        MarkAsSource(I.getOperand(0));

        AddPattern(pattern);
        return true;
    }

    bool CodeGenPatternMatch::MatchBranch(llvm::BranchInst& I)
    {
        struct CondBrInstPattern : Pattern
        {
            SSource cond;
            llvm::BranchInst* inst;
            e_predMode predMode = EPRED_NORMAL;
            bool isDiscardBranch = false;
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                if (isDiscardBranch)
                {
                    pass->emitDiscardBranch(inst, cond);
                }
                else
                {
                    pass->emitBranch(inst, cond, predMode);
                }
            }
        };
        CondBrInstPattern* pattern = new (m_allocator) CondBrInstPattern();
        pattern->inst = &I;

        if (!I.isUnconditional())
        {
            Value* cond = I.getCondition();
            if (dyn_cast<GenIntrinsicInst>(cond,
                GenISAIntrinsic::GenISA_UpdateDiscardMask))
            {
                pattern->isDiscardBranch = true;
            }
            pattern->cond = GetSource(I.getCondition(), false, false);
        }
        AddPattern(pattern);
        return true;
    }

    bool CodeGenPatternMatch::MatchFloatingPointSatModifier(llvm::Instruction& I)
    {
        struct SatPattern : Pattern
        {
            Pattern* pattern;
            SSource source;
            bool isUnsigned;
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                DstModifier mod = modifier;
                mod.sat = true;
                if (pattern)
                {
                    pattern->Emit(pass, mod);
                }
                else
                {
                    pass->Mov(source, mod);
                }
            }
        };
        bool match = false;
        llvm::Value* source = nullptr;
        bool isUnsigned = false;
        if (isSat(&I, source, isUnsigned))
        {
            SatPattern* satPattern = new (m_allocator) SatPattern();
            if (llvm::Instruction * inst = llvm::dyn_cast<Instruction>(source))
            {
                // As an heuristic we only match saturate if the instruction has one use
                // to avoid duplicating expensive instructions and increasing reg pressure
                // without improve code quality this may be refined in the future
                if (inst->hasOneUse() && SupportsSaturate(inst))
                {
                    satPattern->pattern = Match(*inst);
                    IGC_ASSERT_MESSAGE(satPattern->pattern, "Failed to match pattern");
                    match = true;
                }
            }
            if (!match)
            {
                satPattern->pattern = nullptr;
                satPattern->source = GetSource(source, true, false);
                match = true;
            }
            if (isUniform(&I) && source->hasOneUse())
            {
                gatherUniformBools(source);
            }
            AddPattern(satPattern);
        }
        return match;
    }

    bool CodeGenPatternMatch::MatchIntegerSatModifier(llvm::Instruction& I)
    {
        // a default pattern
        struct SatPattern : Pattern
        {
            Pattern* pattern;
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                DstModifier mod = modifier;
                mod.sat = true;
                pattern->Emit(pass, mod);
            }
        };

        // a special pattern is required because of the fact that the instruction works on unsigned values
        // whereas the default type is signed for arithmetic instructions
        struct UAddPattern : Pattern
        {
            BinaryOperator* inst;

            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                DstModifier mod = modifier;
                mod.sat = true;
                pass->EmitUAdd(inst, modifier);
            }
        };

        struct IntegerSatTruncPattern : public Pattern {
            SSource src;
            bool isSigned;
            virtual void Emit(EmitPass* pass, const DstModifier& dstMod)
            {
                pass->EmitIntegerTruncWithSat(isSigned, isSigned, src, dstMod);
            }
        };

        bool match = false;
        llvm::Value* source = nullptr;
        bool isUnsigned = false;
        if (isSat(&I, source, isUnsigned))
        {
            IGC_ASSERT(llvm::isa<Instruction>(source));

            // As an heuristic we only match saturate if the instruction has one use
            // to avoid duplicating expensive instructions and increasing reg pressure
            // without improve code quality this may be refined in the future
            if (llvm::Instruction* sourceInst = llvm::cast<llvm::Instruction>(source);
                sourceInst->hasOneUse() && SupportsSaturate(sourceInst))
            {
                if (llvm::BinaryOperator* binaryOpInst = llvm::dyn_cast<llvm::BinaryOperator>(source);
                    binaryOpInst && (binaryOpInst->getOpcode() == llvm::BinaryOperator::BinaryOps::Add) && isUnsigned)
                {
                    match = true;
                    UAddPattern* uAddPattern = new (m_allocator) UAddPattern();
                    uAddPattern->inst = binaryOpInst;
                    AddPattern(uAddPattern);
                }
                else if (binaryOpInst && (binaryOpInst->getOpcode() == llvm::BinaryOperator::BinaryOps::Add) && !isUnsigned)
                {
                    match = true;
                    SatPattern* satPattern = new (m_allocator) SatPattern();
                    satPattern->pattern = Match(*sourceInst);
                    AddPattern(satPattern);
                }
                else if (llvm::TruncInst* truncInst = llvm::dyn_cast<llvm::TruncInst>(source);
                    truncInst)
                {
                    match = true;
                    IntegerSatTruncPattern* satPattern = new (m_allocator) IntegerSatTruncPattern();
                    satPattern->isSigned = !isUnsigned;
                    satPattern->src = GetSource(truncInst->getOperand(0), !isUnsigned, false);
                    AddPattern(satPattern);
                }
                else if (llvm::GenIntrinsicInst * genIsaInst = llvm::dyn_cast<llvm::GenIntrinsicInst>(source);
                    genIsaInst &&
                    (genIsaInst->getIntrinsicID() == llvm::GenISAIntrinsic::ID::GenISA_dp4a_ss ||
                    genIsaInst->getIntrinsicID() == llvm::GenISAIntrinsic::ID::GenISA_dp4a_su ||
                    genIsaInst->getIntrinsicID() == llvm::GenISAIntrinsic::ID::GenISA_dp4a_uu ||
                    genIsaInst->getIntrinsicID() == llvm::GenISAIntrinsic::ID::GenISA_dp4a_us))
                {
                    match = true;
                    SatPattern* satPattern = new (m_allocator) SatPattern();
                    satPattern->pattern = Match(*sourceInst);
                    AddPattern(satPattern);
                }
                else
                {
                    IGC_ASSERT_MESSAGE(0, "An undefined pattern match");
                }
            }
        }
        return match;
    }

    bool CodeGenPatternMatch::MatchPredicate(llvm::SelectInst& I)
    {
        struct PredicatePattern : Pattern
        {
            bool invertFlag;
            Pattern* patternNotPredicated;
            Pattern* patternPredicated;
            SSource flag;
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                DstModifier mod = modifier;
                patternNotPredicated->Emit(pass, mod);
                mod.flag = &flag;
                mod.invertFlag = invertFlag;
                patternPredicated->Emit(pass, mod);
            }
        };
        bool match = false;
        bool invertFlag = false;
        llvm::Instruction* source0 = llvm::dyn_cast<llvm::Instruction>(I.getTrueValue());
        llvm::Instruction* source1 = llvm::dyn_cast<llvm::Instruction>(I.getFalseValue());
        if (source0 && source0->hasOneUse() && source1 && source1->hasOneUse())
        {
            if (SupportsPredicate(source0))
            {
                // temp fix until we find the best solution for this case
                if (!isa<ExtractElementInst>(source1))
                {
                    match = true;
                }
            }
            else if (SupportsPredicate(source1))
            {
                if (!isa<ExtractElementInst>(source0))
                {
                    std::swap(source0, source1);
                    invertFlag = true;
                    match = true;
                }
            }
        }
        if (match == true)
        {
            PredicatePattern* pattern = new (m_allocator) PredicatePattern();
            pattern->flag = GetSource(I.getCondition(), false, false);
            pattern->invertFlag = invertFlag;
            pattern->patternNotPredicated = Match(*source1);
            pattern->patternPredicated = Match(*source0);
            IGC_ASSERT_MESSAGE(pattern->patternNotPredicated, "Failed to match pattern");
            IGC_ASSERT_MESSAGE(pattern->patternPredicated, "Failed to match pattern");
            AddPattern(pattern);
        }
        return match;
    }

    bool CodeGenPatternMatch::MatchSelectModifier(llvm::SelectInst& I)
    {
        struct SelectPattern : Pattern
        {
            SSource sources[3];
            e_predMode predMode;
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                DstModifier modf = modifier;
                modf.predMode = predMode;
                pass->Select(sources, modf);
            }
        };
        SelectPattern* pattern = new (m_allocator) SelectPattern();
        pattern->predMode = EPRED_NORMAL;

        pattern->sources[0] = GetSource(I.getCondition(), false, false);
        pattern->sources[1] = GetSource(I.getTrueValue(), true, false);
        pattern->sources[2] = GetSource(I.getFalseValue(), true, false);

        // try to add to constant pool whatever possible.
        if (isCandidateForConstantPool(I.getTrueValue()))
        {
            AddToConstantPool(I.getParent(), I.getTrueValue());
            pattern->sources[1].fromConstantPool = true;
        }

        if (isCandidateForConstantPool(I.getFalseValue()))
        {
            AddToConstantPool(I.getParent(), I.getFalseValue());
            pattern->sources[2].fromConstantPool = true;
        }

        AddPattern(pattern);
        return true;
    }

    static bool IsPositiveFloat(Value* v, unsigned int depth = 0)
    {
        if (depth > 3)
        {
            // limit the depth of recursion to avoid compile time problem
            return false;
        }
        if (ConstantFP * cst = dyn_cast<ConstantFP>(v))
        {
            if (!cst->getValueAPF().isNegative())
            {
                return true;
            }
        }
        else if (Instruction * I = dyn_cast<Instruction>(v))
        {
            switch (I->getOpcode())
            {
            case Instruction::FMul:
            case Instruction::FAdd:
                return IsPositiveFloat(I->getOperand(0), depth + 1) && IsPositiveFloat(I->getOperand(1), depth + 1);
            case Instruction::Call:
                if (IntrinsicInst * intrinsicInst = dyn_cast<IntrinsicInst>(I))
                {
                    if (intrinsicInst->getIntrinsicID() == Intrinsic::fabs)
                    {
                        return true;
                    }
                }
                else if (isa<GenIntrinsicInst>(I, GenISAIntrinsic::GenISA_fsat))
                {
                    return true;
                }
                break;
            default:
                break;
            }
        }
        return false;
    }

    bool CodeGenPatternMatch::MatchPow(llvm::IntrinsicInst& I)
    {
        if (IGC_IS_FLAG_ENABLED(DisableMatchPow))
        {
            return false;
        }

        // For this pattern match exp(log(x) * y) = pow
        // if x < 0 and y is an integer (ex: 1.0)
        // with pattern match : pow(x, 1.0) = x
        // without pattern match : exp(log(x) * 1.0) = NaN because log(x) is NaN.
        //
        // Since pow is 2x slower than exp/log, disabling this optimization might not hurt much.
        // Keep the code and disable MatchPow to track any performance change for now.
        struct PowPattern : public Pattern
        {
            SSource sources[2];
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                pass->Pow(sources, modifier);
            }
        };
        bool found = false;
        llvm::Value* source0 = NULL;
        llvm::Value* source1 = NULL;
        if (I.getIntrinsicID() == Intrinsic::exp2)
        {
            llvm::BinaryOperator* mul = dyn_cast<BinaryOperator>(I.getOperand((0)));
            if (mul && mul->getOpcode() == Instruction::FMul)
            {
                for (uint j = 0; j < 2; j++)
                {
                    llvm::IntrinsicInst* log = dyn_cast<IntrinsicInst>(mul->getOperand(j));
                    if (log && log->getIntrinsicID() == Intrinsic::log2)
                    {
                        if (IsPositiveFloat(log->getOperand(0)))
                        {
                            source0 = log->getOperand(0);
                            source1 = mul->getOperand(1 - j);
                            found = true;
                            break;
                        }
                    }
                }
            }
        }
        if (found)
        {
            PowPattern* pattern = new (m_allocator) PowPattern();
            pattern->sources[0] = GetSource(source0, true, false);
            pattern->sources[1] = GetSource(source1, true, false);
            AddPattern(pattern);
        }
        return found;
    }

    // We match this pattern
    // %1 = add %2 %3
    // %b = %cmp %1 0
    // right now we don't match if the alu has more than 1 use has it could generate worse code
    bool CodeGenPatternMatch::MatchCondModifier(llvm::CmpInst& I)
    {
        struct CondModifierPattern : Pattern
        {
            Pattern* pattern;
            Instruction* alu;
            CmpInst* cmp;
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                IGC_ASSERT(modifier.flag == nullptr);
                IGC_ASSERT(modifier.sat == false);
                pass->emitAluConditionMod(pattern, alu, cmp);
            }
        };
        bool found = false;
        for (uint i = 0; i < 2; i++)
        {
            if (IsZero(I.getOperand(i)))
            {
                llvm::Instruction* alu = dyn_cast<Instruction>(I.getOperand(1 - i));
                if (alu && alu->hasOneUse() && SupportsCondModifier(alu))
                {
                    CondModifierPattern* pattern = new (m_allocator) CondModifierPattern();
                    pattern->pattern = Match(*alu);
                    IGC_ASSERT_MESSAGE(pattern->pattern, "Failed to match pattern");
                    pattern->alu = alu;
                    pattern->cmp = &I;
                    AddPattern(pattern);
                    found = true;
                    break;
                }
            }
        }
        return found;
    }

    // we match the following pattern
    // %f = cmp %1 %2
    // %o = or/and %f %g
    bool CodeGenPatternMatch::MatchBoolOp(llvm::BinaryOperator& I)
    {
        struct BoolOpPattern : public Pattern
        {
            llvm::BinaryOperator* boolOp;
            llvm::CmpInst::Predicate predicate;
            SSource cmpSource[2];
            SSource binarySource;
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                pass->CmpBoolOp(boolOp, predicate, cmpSource, binarySource, modifier);
            }
        };

        IGC_ASSERT(I.getOpcode() == Instruction::Or || I.getOpcode() == Instruction::And);
        bool found = false;
        if (I.getType()->isIntegerTy(1))
        {
            for (uint i = 0; i < 2; i++)
            {
                if (CmpInst * cmp = llvm::dyn_cast<CmpInst>(I.getOperand(i)))
                {
                    // only beneficial if the other operand only have one use
                    if (I.getOperand(1 - i)->hasOneUse())
                    {
                        BoolOpPattern* pattern = new (m_allocator) BoolOpPattern();
                        pattern->boolOp = &I;
                        pattern->predicate = cmp->getPredicate();
                        pattern->cmpSource[0] = GetSource(cmp->getOperand(0), true, false);
                        pattern->cmpSource[1] = GetSource(cmp->getOperand(1), true, false);
                        pattern->binarySource = GetSource(I.getOperand(1 - i), false, false);
                        AddPattern(pattern);
                        found = true;
                        break;
                    }
                }
            }
        }
        return found;
    }

    //
    //  Assume that V is of type T (integer) with N bits;
    //  and amt is of integer type too.
    //
    //  rol (V, amt) =  (V << amt) | ((unsigned(V) >> (N - amt))
    //    The function first finds the following generic pattern, note that
    //    [insts] denotes that "insts" are optional.
    //          [amt = and amt, N-1]
    //          high = shl V0, amt
    //          [amt0 = sub 0, amt  || amt0 = sub N, amt]
    //                ; if amt is constant && amt + amt0 == N, this is unneeded
    //          [amt0 = and amt0, N-1]
    //          low = lshr V1, amt0
    //          R = or high, low
    //
    //      case 0: [ likely, V is i32 or i64]
    //          V0 == V1 (V == V0 == V1)
    //
    //      case 1:  [ likely V is i16 or i8]
    //          V0 = sext V || zext V
    //          V1 = zext V
    //          Res = trunc R
    //
    //          Res's type == V's type
    //
    //  ror can be handled similarly. Note that
    //    ror (x, amt) = ((unsigned)x >> amt) | ( x << (N - amt))
    //                 = rol (x, N - amt);
    //
    bool CodeGenPatternMatch::MatchRotate(llvm::Instruction& I)
    {
        using namespace llvm::PatternMatch;

        struct RotatePattern : public Pattern
        {
            SSource sources[2];
            e_opcode rotateOPCode;
            llvm::Instruction* instruction;
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                pass->Binary(rotateOPCode, sources, modifier);
            }
        };

        if (!m_Platform.supportRotateInstruction())
        {
            return false;
        }

        // Sanity check:
        //   make sure that rotate is supported and "I" is a scalar "or" instruction
        IGC_ASSERT_MESSAGE(false == I.getType()->isVectorTy(), "Vector type not expected here");

        uint64_t typeWidth = I.getType()->getScalarSizeInBits();
        Instruction* OrInst = nullptr;
        if (I.getOpcode() == Instruction::Trunc)
        {
            if (BinaryOperator * tmp = dyn_cast<BinaryOperator>(I.getOperand(0)))
            {
                if (tmp->getOpcode() != Instruction::Or)
                {
                    return false;
                }
                OrInst = tmp;
            }
            else
            {
                return false;
            }
        }
        else if (I.getOpcode() == Instruction::Or)
        {
            OrInst = cast<BinaryOperator>(&I);
        }
        else
        {
            IGC_ASSERT_MESSAGE(0, "Should be invoked with Or/Trunc instruction");
        }

        // Do rotate only if
        //   1) type is W/DW (HW only supports W/DW); and
        //   2) both operands are instructions.
        Instruction* LHS = dyn_cast<Instruction>(OrInst->getOperand(0));
        Instruction* RHS = dyn_cast<Instruction>(OrInst->getOperand(1));
        bool typeWidthSupported = typeWidth == 16 || typeWidth == 32;

        if (!LHS || !RHS || !typeWidthSupported)
        {
            return false;
        }

        // Make adjustment so that LHS is shl.
        if (LHS->getOpcode() == Instruction::LShr)
        {
            Instruction* t = LHS;
            LHS = RHS;
            RHS = t;
        }
        if (LHS->getOpcode() != Instruction::Shl ||
            RHS->getOpcode() != Instruction::LShr)
        {
            return false;
        }

        // first: find V
        Value* V0 = LHS->getOperand(0);
        Value* V1 = RHS->getOperand(0);
        Value* V = nullptr;
        if (I.getOpcode() == Instruction::Or)
        {
            if (V0 == V1)
            {
                V = V0;
            }
        }
        else
        {
            Value* X0, * X1;
            if ((match(V0, m_ZExt(m_Value(X0))) || match(V0, m_SExt(m_Value(X0)))) &&
                match(V1, m_ZExt(m_Value(X1))))
            {
                if (X0 == X1 && X0->getType()->getScalarSizeInBits() == typeWidth)
                {
                    V = X0;
                }
            }
        }

        if (!V)
        {
            return false;
        }

        // Second: find amt
        uint64_t typeMask = typeWidth - 1;
        Value* LAmt = LHS->getOperand(1);
        Value* RAmt = RHS->getOperand(1);
        ConstantInt* C_LAmt = dyn_cast<ConstantInt>(LAmt);
        ConstantInt* C_RAmt = dyn_cast<ConstantInt>(RAmt);
        Value* X0, * X1;
        Value* Amt = nullptr;
        bool isROL = true;
        if (C_LAmt || C_RAmt)
        {
            // If only one of shift-amounts is constant, it cannot be rotate.
            if (C_LAmt && C_RAmt)
            {
                // For shift amount that is beyond the typewidth, the result is
                // undefined. Here, we just use the LSB.
                uint64_t c0 = C_LAmt->getZExtValue() & typeMask;
                uint64_t c1 = C_RAmt->getZExtValue() & typeMask;
                if ((c0 + c1) == typeWidth)
                {
                    Amt = LAmt;
                    isROL = true;
                }
            }
        }
        else
        {
            if (match(RAmt, m_And(m_Sub(m_Zero(), m_Value(X1)), m_SpecificInt(typeMask))) ||
                match(RAmt, m_And(m_Sub(m_SpecificInt(typeWidth), m_Value(X1)), m_SpecificInt(typeMask))) ||
                match(RAmt, m_Sub(m_Zero(), m_Value(X1))) ||
                match(RAmt, m_Sub(m_SpecificInt(typeWidth), m_Value(X1))))
            {
                if (LAmt == X1 ||
                    (match(LAmt, m_And(m_Value(X0), m_SpecificInt(typeMask))) && (X1 == X0)))
                {
                    Amt = X1;
                    isROL = true;
                }
            }
            if (!Amt &&
                (match(LAmt, m_And(m_Sub(m_Zero(), m_Value(X1)), m_SpecificInt(typeMask))) ||
                    match(LAmt, m_And(m_Sub(m_SpecificInt(typeWidth), m_Value(X1)), m_SpecificInt(typeMask))) ||
                    match(LAmt, m_Sub(m_Zero(), m_Value(X1))) ||
                    match(LAmt, m_Sub(m_SpecificInt(typeWidth), m_Value(X1)))))
            {
                if (RAmt == X1 ||
                    (match(RAmt, m_And(m_Value(X0), m_SpecificInt(typeMask))) && (X1 == X0)))
                {
                    Amt = X1;
                    isROL = false;
                }
            }

            if (Amt)
            {
                Value* X0, * X1, * X2;
                // 1) simple case: amt = 32 - X0;   use amt1 as shift amount.
                bool isReverse = match(Amt, m_Sub(m_SpecificInt(typeWidth), m_Value(X0)));

                // 2)   t = 16 - X0 | t = 0 - X0   ; for example,  t is i16/i8, etc
                //      t1 = t & 15
                //      amt = zext t1, i32
                isReverse = isReverse ||
                    (match(Amt, m_ZExt(m_Value(X1))) &&
                        match(X1, m_And(m_Value(X2), m_SpecificInt(typeMask))) &&
                        (match(X2, m_Sub(m_SpecificInt(typeWidth), m_Value(X0))) ||
                            match(X2, m_Sub(m_Zero(), m_Value(X0)))));

                if (isReverse)
                {
                    Amt = X0;
                    isROL = !isROL;
                }
            }
        }

        if (!Amt)
        {
            return false;
        }

        // Found the pattern.
        RotatePattern* pattern = new (m_allocator) RotatePattern();
        pattern->instruction = &I;
        pattern->sources[0] = GetSource(V, true, false);
        pattern->sources[1] = GetSource(Amt, true, false);
        pattern->rotateOPCode = isROL ? EOPCODE_ROL : EOPCODE_ROR;

        AddPattern(pattern);
        return true;
    }


    bool CodeGenPatternMatch::MatchLogicAlu(llvm::BinaryOperator& I)
    {
        struct LogicInstPattern : public Pattern
        {
            SSource sources[2];
            llvm::Instruction* instruction;
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                pass->BinaryUnary(instruction, sources, modifier);
            }
        };
        LogicInstPattern* pattern = new (m_allocator) LogicInstPattern();
        pattern->instruction = &I;
        for (unsigned int i = 0; i < 2; ++i)
        {
            e_modifier mod = EMOD_NONE;
            Value* src = I.getOperand(i);
            if (!I.getType()->isIntegerTy(1))
            {
                if (BinaryOperator * notInst = dyn_cast<BinaryOperator>(src))
                {
                    if (notInst->getOpcode() == Instruction::Xor)
                    {
                        if (ConstantInt * minusOne = dyn_cast<ConstantInt>(notInst->getOperand(1)))
                        {
                            if (minusOne->isMinusOne())
                            {
                                mod = EMOD_NOT;
                                src = notInst->getOperand(0);
                            }
                        }
                    }
                }
            }
            pattern->sources[i] = GetSource(src, mod, false);

            if (isCandidateForConstantPool(src))
            {
                AddToConstantPool(I.getParent(), src);
                pattern->sources[i].fromConstantPool = true;
            }


        }
        AddPattern(pattern);
        return true;
    }

    bool CodeGenPatternMatch::MatchRsqrt(llvm::BinaryOperator& I)
    {
        struct RsqrtPattern : public Pattern
        {
            SSource source;
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                pass->Rsqrt(source, modifier);
            }
        };

        bool found = false;
        llvm::Value* source = NULL;
        if (I.getOpcode() == Instruction::FDiv)
        {
            // by vISA document, rsqrt doesn't support double type
            if (isOne(I.getOperand(0)) && I.getType()->getTypeID() != Type::DoubleTyID)
            {
                if (llvm::IntrinsicInst * sqrt = dyn_cast<IntrinsicInst>(I.getOperand(1)))
                {
                    if (sqrt->getIntrinsicID() == Intrinsic::sqrt)
                    {
                        source = sqrt->getOperand(0);
                        found = true;
                    }
                }
            }
        }
        if (found)
        {
            RsqrtPattern* pattern = new (m_allocator) RsqrtPattern();
            pattern->source = GetSource(source, true, false);
            AddPattern(pattern);
        }
        return found;
    }

    bool CodeGenPatternMatch::MatchGradient(llvm::GenIntrinsicInst& I)
    {
        struct GradientPattern : public Pattern
        {
            SSource source;
            llvm::Instruction* instruction;
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                pass->BinaryUnary(instruction, &source, modifier);
            }
        };
        GradientPattern* pattern = new (m_allocator) GradientPattern();
        pattern->instruction = &I;
        pattern->source = GetSource(I.getOperand(0), true, false);
        AddPattern(pattern);
        // mark the source as subspan use
        HandleSubspanUse(pattern->source.value);
        return true;
    }

    bool CodeGenPatternMatch::MatchSampleDerivative(llvm::GenIntrinsicInst& I)
    {
        HandleSampleDerivative(I);
        return MatchSingleInstruction(I);
    }

    bool CodeGenPatternMatch::MatchDbgInstruction(llvm::DbgInfoIntrinsic& I)
    {
        struct DbgInstPattern : Pattern
        {
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                // Nothing to emit.
            }
        };
        DbgInstPattern* pattern = new (m_allocator) DbgInstPattern();
        if (DbgDeclareInst * pDbgDeclInst = dyn_cast<DbgDeclareInst>(&I))
        {
            if (pDbgDeclInst->getAddress())
            {
                MarkAsSource(pDbgDeclInst->getAddress());
            }
        }
        else if (DbgValueInst * pDbgValInst = dyn_cast<DbgValueInst>(&I))
        {
            if (pDbgValInst->getValue())
            {
                MarkAsSource(pDbgValInst->getValue());
            }
        }
        else
        {
            IGC_ASSERT_MESSAGE(0, "Unhandled Dbg intrinsic");
        }
        AddPattern(pattern);
        return true;
    }

    bool CodeGenPatternMatch::MatchAvg(llvm::Instruction& I)
    {
        // "Average value" pattern:
        // (x + y + 1) / 2  -->  avg(x, y)
        //
        // We're looking for patterns like this:
        //    % 14 = add nsw i32 % 10, % 13
        //    % 15 = add nsw i32 % 14, 1
        //    % 16 = ashr i32 % 15, 1

        struct AvgPattern : Pattern
        {
            SSource sources[2];
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                pass->Avg(sources, modifier);
            }
        };

        bool found = false;
        llvm::Value* sources[2];
        e_modifier   src_mod[2];

        IGC_ASSERT(I.getOpcode() == Instruction::SDiv || I.getOpcode() == Instruction::UDiv || I.getOpcode() == Instruction::AShr);

        // We expect 2 for "div" and 1 for "right shift".
        int  expectedVal = (I.getOpcode() == Instruction::SDiv ? 2 : 1);
        Value* opnd1 = I.getOperand(1);   // Divisor or shift factor.
        if (!isa<ConstantInt>(opnd1) || (cast<ConstantInt>(opnd1))->getZExtValue() != expectedVal)
        {
            return false;
        }

        if (Instruction * divSrc = dyn_cast<Instruction>(I.getOperand(0)))
        {
            if (divSrc->getOpcode() == Instruction::Add && !NeedInstruction(*divSrc))
            {
                Instruction* instAdd = cast<Instruction>(divSrc);
                for (int i = 0; i < 2; i++)
                {
                    if (ConstantInt * cnst = dyn_cast<ConstantInt>(instAdd->getOperand(i)))
                    {
                        // "otherArg" is the second argument of "instAdd" (which is not constant).
                        Value* otherArg = instAdd->getOperand(i == 0 ? 1 : 0);
                        if (cnst->getZExtValue() == 1 && isa<AddOperator>(otherArg) && !NeedInstruction(*cast<Instruction>(otherArg)))
                        {
                            Instruction* firstAdd = cast<Instruction>(otherArg);
                            sources[0] = firstAdd->getOperand(0);
                            sources[1] = firstAdd->getOperand(1);
                            GetModifier(*sources[0], src_mod[0], sources[0]);
                            GetModifier(*sources[1], src_mod[1], sources[1]);
                            found = true;
                            break;
                        }
                    }
                }
            }
        }

        if (found)
        {
            AvgPattern* pattern = new (m_allocator)AvgPattern();
            pattern->sources[0] = GetSource(sources[0], src_mod[0], false);
            pattern->sources[1] = GetSource(sources[1], src_mod[1], false);
            AddPattern(pattern);
        }
        return found;
    }

    bool CodeGenPatternMatch::MatchShuffleBroadCast(llvm::GenIntrinsicInst& I)
    {
        // Match cases like:
        //    %84 = bitcast <2 x i32> %vCastload to <4 x half>
        //    %scalar269 = extractelement <4 x half> % 84, i32 0
        //    %simdShuffle = call half @llvm.genx.GenISA.simdShuffle.f.f16(half %scalar269, i32 0)
        //
        // to mov with region and offset
        struct BroadCastPattern : public Pattern
        {
            SSource source;
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                pass->Mov(source, modifier);
            }
        };
        bool match = false;
        SSource source;
        Value* sourceV = &I;
        if (GetRegionModifier(source, sourceV, true))
        {
            BroadCastPattern* pattern = new (m_allocator) BroadCastPattern();
            GetModifier(*sourceV, source.mod, sourceV);
            source.value = sourceV;
            pattern->source = source;
            MarkAsSource(sourceV);
            match = true;
            AddPattern(pattern);
        }
        return match;
    }

    bool CodeGenPatternMatch::MatchWaveShuffleIndex(llvm::GenIntrinsicInst& I)
    {
        llvm::Value* helperLaneMode = I.getOperand(2);
        IGC_ASSERT(helperLaneMode);
        if (int_cast<int>(cast<ConstantInt>(helperLaneMode)->getSExtValue()) == 1)
        {
            //only if helperLaneMode==1, we enable helper lane under some shuffleindex cases (not for all cases).
            HandleSubspanUse(I.getArgOperand(0));
            HandleSubspanUse(&I);
        }
        return MatchSingleInstruction(I);
    }

    bool CodeGenPatternMatch::MatchRegisterRegion(llvm::GenIntrinsicInst& I)
    {
        struct MatchRegionPattern : public Pattern
        {
            SSource source;
            virtual void Emit(EmitPass* pass, const DstModifier& modifier)
            {
                pass->Mov(source, modifier);
            }
        };

        /*
        * Match case 1 - With SubReg Offset: Shuffle( data, (laneID << x) + y )
        *   %25 = call i16 @llvm.genx.GenISA.simdLaneId()
        *   %30 = zext i16 %25 to i32
        *   %31 = shl nuw nsw i32 %30, 1  - Current LaneID shifted by x
        *   %36 = add i32 %31, 1          - Current LaneID shifted by x + y  Shuffle( data, (laneID << x) + 1 )
        *   %37 = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %21, i32 %36)

        * Match case 2(Special case of Match Case 1) - No SubReg Offset: Shuffle( data, (laneID << x) + 0 )
        *    %25 = call i16 @llvm.genx.GenISA.simdLaneId()
        *    %30 = zext i16 %25 to i32
        *    %31 = shl nuw nsw i32 %30, 1 - Current LaneID shifted by x
        *    %32 = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %21, i32 %31)
        */

        Value* data = I.getOperand(0);
        Value* source = I.getOperand(1);
        uint typeByteSize = data->getType()->getScalarSizeInBits() / 8;
        bool isMatch = false;
        int subReg = 0;
        uint verticalStride = 1; //Default value for special case  Shuffle( data, (laneID << x) + y )  when x = 0

        if (auto binaryInst = dyn_cast<BinaryOperator>(source))
        {
            //Will be skipped for match case 2
            if (binaryInst->getOpcode() == Instruction::Add)
            {
                if (llvm::ConstantInt * simDOffSetInst = llvm::dyn_cast<llvm::ConstantInt>(binaryInst->getOperand(1)))
                {
                    subReg = int_cast<int>(cast<ConstantInt>(simDOffSetInst)->getSExtValue());

                    //Subregister must be a number between 0 and 15 for a valid region
                    // We could support up to 31 but we need to handle reading from different SIMD16 var chunks
                    if (subReg >= 0 && subReg < 16)
                    {
                        source = binaryInst->getOperand(0);
                    }
                }
            }
        }

        if (auto binaryInst = dyn_cast<BinaryOperator>(source))
        {
            if (binaryInst->getOpcode() == Instruction::Shl)
            {
                source = binaryInst->getOperand(0);

                if (llvm::ConstantInt * simDOffSetInst = llvm::dyn_cast<llvm::ConstantInt>(binaryInst->getOperand(1)))
                {
                    uint shiftFactor = int_cast<uint>(simDOffSetInst->getZExtValue());
                    //Check to make sure we dont end up with an invalid Vertical Stride.
                    //Only 1, 2, 4, 8, 16 are supported.
                    if (shiftFactor <= 4)
                        verticalStride = (1U << shiftFactor);
                    else
                        return false;
                }
            }
        }

        if (auto zExtInst = llvm::dyn_cast<llvm::ZExtInst>(source))
        {
            source = zExtInst->getOperand(0);
        }

        llvm::GenIntrinsicInst* intrin = llvm::dyn_cast<llvm::GenIntrinsicInst>(source);

        //Finally check for simLaneID intrisic
        if (intrin && (intrin->getIntrinsicID() == GenISAIntrinsic::GenISA_simdLaneId))
        {
            //To avoid compiler crash, pattern match with direct mov will be disable
            //Conservetively, we assum simd16 for 32 bytes GRF platforms and simd32 for 64 bytes GRF platforms
            bool cross2GRFs = typeByteSize * (subReg + verticalStride * (m_Platform.getGRFSize() > 32 ? 32 : 16)) > (2 * m_Platform.getGRFSize());
            if (!cross2GRFs)
            {
                MatchRegionPattern* pattern = new (m_allocator) MatchRegionPattern();
                pattern->source.elementOffset = subReg;

                //Set Region Parameters <VerString;Width,HorzString>
                pattern->source.region_set = true;
                pattern->source.region[0] = verticalStride;
                pattern->source.region[1] = 1;
                pattern->source.region[2] = 0;

                pattern->source.value = data;
                MarkAsSource(data);
                HandleSubspanUse(data);
                AddPattern(pattern);

                isMatch = true;
            }
        }

        return isMatch;
    }

    bool CodeGenPatternMatch::GetRegionModifier(SSource& sourceMod, llvm::Value*& source, bool regioning)
    {
        bool found = false;
        Value* OrignalSource = source;
        if (llvm::BitCastInst * bitCast = llvm::dyn_cast<BitCastInst>(source))
        {
            if (!bitCast->getType()->isVectorTy() && !bitCast->getOperand(0)->getType()->isVectorTy())
            {
                source = bitCast->getOperand(0);
                found = true;
            }
        }

        if (llvm::GenIntrinsicInst * intrin = llvm::dyn_cast<llvm::GenIntrinsicInst>(source))
        {
            GenISAIntrinsic::ID id = intrin->getIntrinsicID();
            if (id == GenISAIntrinsic::GenISA_WaveShuffleIndex)
            {
                if (llvm::ConstantInt * channelVal = llvm::dyn_cast<llvm::ConstantInt>(intrin->getOperand(1)))
                {
                    unsigned int offset = int_cast<unsigned int>(channelVal->getZExtValue());
                    if (offset < 16)
                    {
                        sourceMod.elementOffset = offset;
                        // SIMD shuffle force region <0,1;0>
                        sourceMod.region_set = true;
                        sourceMod.region[0] = 0;
                        sourceMod.region[1] = 1;
                        sourceMod.region[2] = 0;
                        sourceMod.instance = EINSTANCE_FIRST_HALF;
                        source = intrin->getOperand(0);
                        found = true;
                        BitcastSearch(sourceMod, source, true);
                    }
                }
            }
        }
        if (regioning && !sourceMod.region_set)
        {
            found |= BitcastSearch(sourceMod, source, false);
        }
        if (found && sourceMod.type == VISA_Type::ISA_TYPE_NUM)
        {
            // keep the original type
            sourceMod.type = GetType(OrignalSource->getType(), m_ctx);
        }
        return found;
    }

    void CodeGenPatternMatch::HandleSampleDerivative(llvm::GenIntrinsicInst& I)
    {
        switch (I.getIntrinsicID())
        {
        case GenISAIntrinsic::GenISA_sampleptr:
        case GenISAIntrinsic::GenISA_lodptr:
        case GenISAIntrinsic::GenISA_sampleKillPix:
            HandleSubspanUse(I.getOperand(0));
            HandleSubspanUse(I.getOperand(1));
            HandleSubspanUse(I.getOperand(2));
            break;
        case GenISAIntrinsic::GenISA_sampleBptr:
        case GenISAIntrinsic::GenISA_sampleCptr:
            HandleSubspanUse(I.getOperand(1));
            HandleSubspanUse(I.getOperand(2));
            HandleSubspanUse(I.getOperand(3));
            break;
        case GenISAIntrinsic::GenISA_sampleBCptr:
            HandleSubspanUse(I.getOperand(2));
            HandleSubspanUse(I.getOperand(3));
            HandleSubspanUse(I.getOperand(4));
            break;
        default:
            break;
        }
    }

    // helper function for pattern match
    static inline bool isLowerPredicate(llvm::CmpInst::Predicate pred)
    {
        switch (pred)
        {
        case llvm::CmpInst::FCMP_ULT:
        case llvm::CmpInst::FCMP_ULE:
        case llvm::CmpInst::FCMP_OLT:
        case llvm::CmpInst::FCMP_OLE:
        case llvm::CmpInst::ICMP_ULT:
        case llvm::CmpInst::ICMP_ULE:
        case llvm::CmpInst::ICMP_SLT:
        case llvm::CmpInst::ICMP_SLE:
            return true;
        default:
            break;
        }
        return false;
    }

    // helper function for pattern match
    static inline bool isGreaterOrLowerPredicate(llvm::CmpInst::Predicate pred)
    {
        switch (pred)
        {
        case llvm::CmpInst::FCMP_UGT:
        case llvm::CmpInst::FCMP_UGE:
        case llvm::CmpInst::FCMP_ULT:
        case llvm::CmpInst::FCMP_ULE:
        case llvm::CmpInst::FCMP_OGT:
        case llvm::CmpInst::FCMP_OGE:
        case llvm::CmpInst::FCMP_OLT:
        case llvm::CmpInst::FCMP_OLE:
        case llvm::CmpInst::ICMP_UGT:
        case llvm::CmpInst::ICMP_UGE:
        case llvm::CmpInst::ICMP_ULT:
        case llvm::CmpInst::ICMP_ULE:
        case llvm::CmpInst::ICMP_SGT:
        case llvm::CmpInst::ICMP_SGE:
        case llvm::CmpInst::ICMP_SLT:
        case llvm::CmpInst::ICMP_SLE:
            return true;
        default:
            break;
        }
        return false;
    }

    static bool isIntegerAbs(SelectInst* SI, e_modifier& mod, Value*& source) {
        using namespace llvm::PatternMatch; // Scoped using declaration.

        Value* Cond = SI->getOperand(0);
        Value* TVal = SI->getOperand(1);
        Value* FVal = SI->getOperand(2);

        ICmpInst::Predicate IPred = FCmpInst::FCMP_FALSE;
        Value* LHS = nullptr;
        Value* RHS = nullptr;

        if (!match(Cond, m_ICmp(IPred, m_Value(LHS), m_Value(RHS))))
            return false;

        if (!ICmpInst::isSigned(IPred))
            return false;

        if (match(LHS, m_Zero())) {
            IPred = ICmpInst::getSwappedPredicate(IPred);
            std::swap(LHS, RHS);
        }

        if (!match(RHS, m_Zero()))
            return false;

        if (match(TVal, m_Neg(m_Specific(FVal)))) {
            IPred = ICmpInst::getInversePredicate(IPred);
            std::swap(TVal, FVal);
        }

        if (!match(FVal, m_Neg(m_Specific(TVal))))
            return false;

        if (LHS != TVal)
            return false;

        source = TVal;
        mod = (IPred == ICmpInst::ICMP_SGT || IPred == ICmpInst::ICMP_SGE) ? EMOD_ABS : EMOD_NEGABS;

        return true;
    }

    bool isAbs(llvm::Value* abs, e_modifier& mod, llvm::Value*& source)
    {
        bool found = false;

        if (IntrinsicInst * intrinsicInst = dyn_cast<IntrinsicInst>(abs))
        {
            if (intrinsicInst->getIntrinsicID() == Intrinsic::fabs)
            {
                source = intrinsicInst->getOperand(0);
                mod = EMOD_ABS;
                return true;
            }
        }

        llvm::SelectInst* select = llvm::dyn_cast<llvm::SelectInst>(abs);
        if (!select)
            return false;

        // Try to find floating point abs first
        if (llvm::FCmpInst * cmp = llvm::dyn_cast<llvm::FCmpInst>(select->getOperand(0)))
        {
            llvm::CmpInst::Predicate pred = cmp->getPredicate();
            if (isGreaterOrLowerPredicate(pred))
            {
                for (int zeroIndex = 0; zeroIndex < 2; zeroIndex++)
                {
                    llvm::ConstantFP* zero = llvm::dyn_cast<llvm::ConstantFP>(cmp->getOperand(zeroIndex));
                    if (zero && zero->isZero())
                    {
                        llvm::Value* cmpSource = cmp->getOperand(1 - zeroIndex);
                        for (int sourceIndex = 0; sourceIndex < 2; sourceIndex++)
                        {
                            if (cmpSource == select->getOperand(1 + sourceIndex))
                            {
                                llvm::BinaryOperator* negate =
                                    llvm::dyn_cast<llvm::BinaryOperator>(select->getOperand(1 + (1 - sourceIndex)));
                                llvm::Value* negateSource = NULL;
                                if (negate && IsNegate(*negate, negateSource) && negateSource == cmpSource)
                                {
                                    found = true;
                                    source = cmpSource;
                                    // depending on the order source in cmp/select it can abs() or -abs()
                                    bool isNegateAbs = (zeroIndex == 0) ^ isLowerPredicate(pred) ^ (sourceIndex == 1);
                                    mod = isNegateAbs ? EMOD_NEGABS : EMOD_ABS;
                                }
                                break;
                            }
                        }
                        break;
                    }
                }
            }
        }

        // If not found, try integer abs
        return found || isIntegerAbs(select, mod, source);
    }

    // combine two modifiers, this function is *not* communtative
    e_modifier CombineModifier(e_modifier mod1, e_modifier mod2)
    {
        e_modifier mod = EMOD_NONE;
        switch (mod1)
        {
        case EMOD_ABS:
        case EMOD_NEGABS:
            mod = mod1;
            break;
        case EMOD_NEG:
            if (mod2 == EMOD_NEGABS)
            {
                mod = EMOD_ABS;
            }
            else if (mod2 == EMOD_ABS)
            {
                mod = EMOD_NEGABS;
            }
            else if (mod2 == EMOD_NEG)
            {
                mod = EMOD_NONE;
            }
            else
            {
                mod = EMOD_NEG;
            }
            break;
        default:
            mod = mod2;
        }
        return mod;
    }

    bool GetModifier(llvm::Value& modifier, e_modifier& mod, llvm::Value*& source)
    {
        mod = EMOD_NONE;
        if (llvm::Instruction * bin = llvm::dyn_cast<llvm::Instruction>(&modifier))
        {
            return GetModifier(*bin, mod, source);
        }
        return false;
    }

    bool GetModifier(llvm::Instruction& modifier, e_modifier& mod, llvm::Value*& source)
    {
        llvm::Value* modifierSource = NULL;
        mod = EMOD_NONE;
        BinaryOperator* bin = dyn_cast<BinaryOperator>(&modifier);
        if (bin && IsNegate(*bin, modifierSource))
        {
            e_modifier absModifier = EMOD_NONE;
            llvm::Value* absSource = NULL;
            if (isAbs(modifierSource, absModifier, absSource))
            {
                source = absSource;
                mod = IGC::CombineModifier(EMOD_NEG, absModifier);
            }
            else
            {
                source = modifierSource;
                mod = EMOD_NEG;
            }
            return true;
        }
        else if (isAbs(&modifier, mod, modifierSource))
        {
            source = modifierSource;
            return true;
        }
        return false;
    }

    bool IsNegate(llvm::BinaryOperator& sub, llvm::Value*& negateSource)
    {
        if (sub.getOpcode() == Instruction::FSub || sub.getOpcode() == Instruction::Sub)
        {
            if (IsZero(sub.getOperand(0)))
            {
                negateSource = sub.getOperand(1);
                return true;
            }
        }
        return false;
    }

    bool IsZero(llvm::Value* zero)
    {
        if (llvm::ConstantFP * FCst = llvm::dyn_cast<llvm::ConstantFP>(zero))
        {
            if (FCst->isZero())
            {
                return true;
            }
        }
        if (llvm::ConstantInt * ICst = llvm::dyn_cast<llvm::ConstantInt>(zero))
        {
            if (ICst->isZero())
            {
                return true;
            }
        }
        return false;
    }

    inline bool isMinOrMax(llvm::Value* inst, llvm::Value*& source0, llvm::Value*& source1, bool& isMin, bool& isUnsigned)
    {
        bool found = false;
        llvm::Instruction* max = llvm::dyn_cast<llvm::Instruction>(inst);
        if (!max)
            return false;

        EOPCODE op = GetOpCode(max);
        if (op == llvm_min || op == llvm_max)
        {
            source0 = max->getOperand(0);
            source1 = max->getOperand(1);
            isUnsigned = false;
            isMin = (op == llvm_min);
            return true;
        }
        else if (op == llvm_select)
        {
            if (llvm::CmpInst * cmp = llvm::dyn_cast<llvm::CmpInst>(max->getOperand(0)))
            {
                if (isGreaterOrLowerPredicate(cmp->getPredicate()))
                {
                    if ((cmp->getOperand(0) == max->getOperand(1) && cmp->getOperand(1) == max->getOperand(2)) ||
                        (cmp->getOperand(0) == max->getOperand(2) && cmp->getOperand(1) == max->getOperand(1)))
                    {
                        source0 = max->getOperand(1);
                        source1 = max->getOperand(2);
                        isMin = isLowerPredicate(cmp->getPredicate()) ^ (cmp->getOperand(0) == max->getOperand(2));
                        isUnsigned = IsUnsignedCmp(cmp->getPredicate());
                        found = true;
                    }
                }
            }
        }
        return found;
    }

    inline bool isMax(llvm::Value* max, llvm::Value*& source0, llvm::Value*& source1)
    {
        bool isMin, isUnsigned;
        llvm::Value* maxSource0;
        llvm::Value* maxSource1;
        if (isMinOrMax(max, maxSource0, maxSource1, isMin, isUnsigned))
        {
            if (!isMin)
            {
                source0 = maxSource0;
                source1 = maxSource1;
                return true;
            }
        }
        return false;
    }

    inline bool isMin(llvm::Value* min, llvm::Value*& source0, llvm::Value*& source1)
    {
        bool isMin, isUnsigned;
        llvm::Value* maxSource0;
        llvm::Value* maxSource1;
        if (isMinOrMax(min, maxSource0, maxSource1, isMin, isUnsigned))
        {
            if (isMin)
            {
                source0 = maxSource0;
                source1 = maxSource1;
                return true;
            }
        }
        return false;
    }


    bool isOne(llvm::Value* zero)
    {
        if (llvm::ConstantFP * FCst = llvm::dyn_cast<llvm::ConstantFP>(zero))
        {
            if (FCst->isExactlyValue(1.f))
            {
                return true;
            }
        }
        if (llvm::ConstantInt * ICst = llvm::dyn_cast<llvm::ConstantInt>(zero))
        {
            if (ICst->isOne())
            {
                return true;
            }
        }
        return false;
    }

    bool isSat(llvm::Instruction* sat, llvm::Value*& source, bool& isUnsigned)
    {
        bool found = false;
        llvm::Value* sources[2] = { 0 };
        bool floatMatch = sat->getType()->isFloatingPointTy();
        GenIntrinsicInst* intrin = dyn_cast<GenIntrinsicInst>(sat);
        if (intrin &&
            (intrin->getIntrinsicID() == GenISAIntrinsic::GenISA_fsat ||
            intrin->getIntrinsicID() == GenISAIntrinsic::GenISA_usat ||
            intrin->getIntrinsicID() == GenISAIntrinsic::GenISA_isat))
        {
            source = intrin->getOperand(0);
            found = true;
            isUnsigned = intrin->getIntrinsicID() == GenISAIntrinsic::GenISA_usat;
        }
        else if (floatMatch && isMax(sat, sources[0], sources[1]))
        {
            for (int i = 0; i < 2; i++)
            {
                if (IsZero(sources[i]))
                {
                    llvm::Value* maxSources[2] = { 0 };
                    if (isMin(sources[1 - i], maxSources[0], maxSources[1]))
                    {
                        for (int j = 0; j < 2; j++)
                        {
                            if (isOne(maxSources[j]))
                            {
                                found = true;
                                source = maxSources[1 - j];
                                isUnsigned = false;
                                break;
                            }
                        }
                    }
                    break;
                }
            }
        }
        else if (floatMatch && isMin(sat, sources[0], sources[1]))
        {
            for (int i = 0; i < 2; i++)
            {
                if (isOne(sources[i]))
                {
                    llvm::Value* maxSources[2] = { 0 };
                    if (isMax(sources[1 - i], maxSources[0], maxSources[1]))
                    {
                        for (int j = 0; j < 2; j++)
                        {
                            if (IsZero(maxSources[j]))
                            {
                                found = true;
                                source = maxSources[1 - j];
                                isUnsigned = false;
                                break;
                            }
                        }
                    }
                    break;
                }
            }
        }
        return found;
    }

    bool isCandidateForConstantPool(llvm::Value * val)
    {
        auto ci = dyn_cast<ConstantInt>(val);
        bool isBigQW = ci && !ci->getValue().isNullValue() && !ci->getValue().isSignedIntN(32);
        bool isDF = val->getType()->isDoubleTy();
        return (isBigQW || isDF);
    };

    uint CodeGenPatternMatch::GetBlockId(llvm::BasicBlock* block)
    {
        auto it = m_blockMap.find(block);
        IGC_ASSERT(it != m_blockMap.end());

        uint blockID = it->second->id;
        return blockID;
    }
}//namespace IGC
