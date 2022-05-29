/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CISACodeGen/CISACodeGen.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/LiveVars.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/CISACodeGen/PositionDepAnalysis.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstVisitor.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/MapVector.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/DataLayout.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/Instructions.h"

namespace llvm
{
    class Value;
    class PHINode;
    class Function;
    class BasicBlock;
    class GenIntrinsicInst;
}

namespace IGC
{

    struct SSource
    {
        llvm::Value* value;
        VISA_Type type;      // Set if source needs to be bit-casted to different type.
        e_modifier mod;
        int elementOffset;
        int SIMDOffset;
        unsigned char region[3];
        bool region_set;
        e_instance instance;
        bool fromConstantPool;

        SSource() : value(nullptr), type(VISA_Type::ISA_TYPE_NUM), mod(EMOD_NONE), elementOffset(0), SIMDOffset(0), region_set(false), instance(EINSTANCE_UNSPECIFIED), fromConstantPool(false)
        {
        }
    };

    struct DstModifier
    {
        SSource* flag;
        bool invertFlag;
        bool sat;
        e_predMode predMode;
        DstModifier() :
            flag(nullptr), invertFlag(false)
            , sat(false), predMode(EPRED_NORMAL)
        {
        }
    };
    class EmitPass;
    class CShader;

    struct Pattern
    {
        virtual void Emit(EmitPass* pass, const DstModifier& modifier) = 0;
        virtual ~Pattern() {}

        // Does the pattern allow destination saturation?
        virtual bool supportsSaturate() { return true; }
    };

    struct SDAG
    {
        SDAG(Pattern* pattern, llvm::Instruction* root) : m_pattern(pattern), m_root(root)
        {

        }
        Pattern* m_pattern;
        llvm::Instruction* m_root;
    };

    struct SDestination
    {
        llvm::Instruction* inst;
        llvm::Value* predicate;
        e_modifier mod;
        bool inversePredicate;
        bool uniform;
    };

    struct SBasicBlock
    {
        using reverse_iterator = std::vector<SDAG>::reverse_iterator;

        ~SBasicBlock() {}
        uint id;
        llvm::BasicBlock* bb;
        std::vector<SDAG> m_dags;
        // caches the active lane mask (a flag variable) for this BB
        // this is currently set only when we enable the A64 WA
        CVariable* m_activeMask = nullptr;
    };

    class CodeGenPatternMatch : public llvm::FunctionPass, public llvm::InstVisitor<CodeGenPatternMatch>
    {
    private:
        CodeGenPatternMatch(CodeGenPatternMatch&) = delete;
        CodeGenPatternMatch& operator =(CodeGenPatternMatch&) = delete;

    public:

        CodeGenPatternMatch();

        ~CodeGenPatternMatch();

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<llvm::DominatorTreeWrapperPass>();
            AU.addRequired<llvm::LoopInfoWrapperPass>();
            AU.addRequired<WIAnalysis>();
            AU.addRequired<LiveVarsAnalysis>();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<PositionDepAnalysis>();
            AU.addRequired<CodeGenContextWrapper>();
            AU.setPreservesAll();
        }

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual llvm::StringRef getPassName() const override {
            return "CodeGenPatternMatchPass";
        }

        void visitCastInst(llvm::CastInst& I);
        void visitBinaryOperator(llvm::BinaryOperator& I);
        void visitCmpInst(llvm::CmpInst& I);
        void visitPHINode(llvm::PHINode& I);
        void visitUnaryInstruction(llvm::UnaryInstruction& I);
        void visitCallInst(llvm::CallInst& I);
        void visitIntrinsicInst(llvm::IntrinsicInst& I);
        void visitSelectInst(llvm::SelectInst& I);
        void visitBitCastInst(llvm::BitCastInst& I);
        void visitIntToPtrInst(llvm::IntToPtrInst& I);
        void visitPtrToIntInst(llvm::PtrToIntInst& I);
        void visitAddrSpaceCast(llvm::AddrSpaceCastInst& I);
        void visitInstruction(llvm::Instruction& I);
        void visitExtractElementInst(llvm::ExtractElementInst& I);
        void visitLoadInst(llvm::LoadInst& I);
        void visitStoreInst(llvm::StoreInst& I);
        void visitFPToSIInst(llvm::FPToSIInst& I);
        void visitFPToUIInst(llvm::FPToUIInst& I);
        void visitDbgInfoIntrinsic(llvm::DbgInfoIntrinsic& I);
        void visitExtractValueInst(llvm::ExtractValueInst& I);
        void visitInsertValueInst(llvm::InsertValueInst& I);
        void visitBranchInst(llvm::BranchInst& I);

    public:
        void CodeGenBlock(llvm::BasicBlock* bb);
        void CodeGenNode(llvm::DomTreeNode* node);

        void ForceIsolate(llvm::Value* val) { m_forceIsolates.insert(val); }
        bool IsForceIsolated(llvm::Value* val) { return (m_forceIsolates.count(val) > 0) ? true : false; }

        SSource GetSource(llvm::Value* v, bool modifier, bool regioning);
        SSource GetSource(llvm::Value* value, e_modifier mod, bool regioning);
        bool GetRegionModifier(SSource& mod, llvm::Value*& source, bool regioning);
        bool BitcastSearch(SSource& mod, llvm::Value*& source, bool broadcast);

        void MarkAsSource(llvm::Value* v);

        bool MatchFMA(llvm::IntrinsicInst& I);
        bool MatchFrc(llvm::BinaryOperator& I);
        bool MatchFloor(llvm::BinaryOperator& I);
        bool MatchPredAdd(llvm::BinaryOperator& I);
        bool MatchSimpleAdd(llvm::BinaryOperator& I);
        bool MatchMad(llvm::BinaryOperator& I);
        bool MatchLrp(llvm::BinaryOperator& I);
        bool MatchCmpSext(llvm::Instruction& I);
        bool MatchModifier(llvm::Instruction& I, bool SupportSrc0Mod = true);
        bool MatchSingleInstruction(llvm::Instruction& I);
        bool MatchCanonicalizeInstruction(llvm::Instruction& I);
        bool MatchBranch(llvm::BranchInst& I);
        bool MatchShuffleBroadCast(llvm::GenIntrinsicInst& I);
        bool MatchWaveShuffleIndex(llvm::GenIntrinsicInst& I);
        bool MatchWaveInstruction(llvm::GenIntrinsicInst& I);
        bool MatchRegisterRegion(llvm::GenIntrinsicInst& I);

        Pattern* Match(llvm::Instruction& inst);
        bool MatchAbsNeg(llvm::Instruction& I);
        bool MatchFloatingPointSatModifier(llvm::Instruction& I);
        bool MatchIntegerSatModifier(llvm::Instruction& I);
        bool MatchPredicate(llvm::SelectInst& I);
        bool MatchSelectModifier(llvm::SelectInst& I);
        bool MatchPow(llvm::IntrinsicInst& I);
        bool MatchCondModifier(llvm::CmpInst& I);
        bool MatchBoolOp(llvm::BinaryOperator& I);
        bool MatchFunnelShiftRotate(llvm::IntrinsicInst& I);
        bool MatchImmOffsetLSC(llvm::Instruction& I);
        bool MatchBfn(llvm::Instruction& I);
        bool MatchCmpSelect(llvm::SelectInst& I);
        bool MatchAdd3(llvm::Instruction& I);
        bool MatchDpas(llvm::GenIntrinsicInst& I);
        bool MatchDp4a(llvm::GenIntrinsicInst& I);
        bool MatchLogicAlu(llvm::BinaryOperator& I);
        bool MatchRsqrt(llvm::BinaryOperator& I);
        bool MatchLoadStorePointer(llvm::Instruction& I, llvm::Value& ptrVal);
        bool MatchBlockReadWritePointer(llvm::GenIntrinsicInst& I);
        bool MatchURBRead(llvm::GenIntrinsicInst& I);
        bool MatchGradient(llvm::GenIntrinsicInst& I);
        bool MatchSampleDerivative(llvm::GenIntrinsicInst& I);
        bool MatchDbgInstruction(llvm::DbgInfoIntrinsic& I);
        bool MatchAvg(llvm::Instruction& I);

        bool MatchMinMax(llvm::SelectInst& I);
        bool MatchFullMul32(llvm::Instruction& I);
        bool MatchMulAdd16(llvm::Instruction& I);
        bool MatchFPToIntegerWithSaturation(llvm::Instruction& I);
        std::tuple<llvm::Value*, unsigned, VISA_Type> isFPToIntegerSatWithExactConstant(llvm::CastInst* I);
        std::tuple<llvm::Value*, unsigned, VISA_Type> isFPToSignedIntSatWithInexactConstant(llvm::SelectInst* I);
        std::tuple<llvm::Value*, unsigned, VISA_Type> isFPToUnsignedIntSatWithInexactConstant(llvm::SelectInst* I);
        bool MatchIntegerTruncSatModifier(llvm::SelectInst& I);
        std::tuple<llvm::Value*, bool, bool> isIntegerSatTrunc(llvm::SelectInst*);

        bool MatchSIToFPZExt(llvm::SIToFPInst* S2FI);

        bool matchAddPair(llvm::ExtractValueInst*);
        bool matchSubPair(llvm::ExtractValueInst*);
        bool matchMulPair(llvm::ExtractValueInst*);
        bool matchPtrToPair(llvm::ExtractValueInst*);

        bool MatchUnmaskedRegionBoundary(llvm::Instruction& I, bool start);

        bool MatchInsertToStruct(llvm::InsertValueInst*);
        bool MatchExtractFromStruct(llvm::ExtractValueInst*);

        void AddPattern(Pattern* P)
        {
            m_currentPattern = P;
        }

        void SetSrcModifier(unsigned int sourceIndex, e_modifier mod);
        void SetPatternRoot(llvm::Instruction& inst);
        void CreateBasicBlocks(llvm::Function* pLLVMFunc);
        uint GetBlockId(llvm::BasicBlock* bb);
        void HandleSubspanUse(llvm::Value* v);
        void HandleSampleDerivative(llvm::GenIntrinsicInst& I);
        bool IsSubspanUse(llvm::Value* v);
        bool HasUseOutsideLoop(llvm::Value* v);
        bool NeedVMask();

        //helper function
        bool NeedInstruction(llvm::Instruction& I);
        bool SIMDConstExpr(llvm::Instruction* v);
        bool IsConstOrSimdConstExpr(llvm::Value* C);
        bool FlushesDenormsOnOutput(llvm::Instruction& I);
        bool FlushesDenormsOnInput(llvm::Instruction& I);

        // Place a constant Val into the constant pool. This constant should be
        // available in basic block UseBlock.
        void AddToConstantPool(llvm::BasicBlock* UseBlock, llvm::Value* Val);

        bool canEmitAsUniformBool(const llvm::Value* Val) const
        {
            return UniformBools.count(Val) > 0;
        }

        bool isUniform(const llvm::Value* V) const
        {
            return m_WI && (m_WI->isUniform(V));
        };

    public:
        llvm::DenseSet<llvm::Instruction*>       m_usedInstructions;
        bool                  m_rootIsSubspanUse;
        llvm::DenseSet<llvm::Value*>             m_subSpanUse;
        llvm::SmallPtrSet<llvm::Value*, 8>       m_forceIsolates;
        std::map<llvm::BasicBlock*, SBasicBlock*> m_blockMap;
        SBasicBlock* m_blocks;
        uint                  m_numBlocks;
        llvm::DenseMap<llvm::Instruction*, bool> m_IsSIMDConstExpr;

        // Where we put the constant initialization.
        llvm::MapVector<llvm::Constant*, llvm::BasicBlock*> ConstantPlacement;

        typedef std::pair<llvm::ExtractValueInst*, llvm::ExtractValueInst*> PairOutputTy;
        typedef llvm::DenseMap<llvm::Value*, PairOutputTy> PairOutputMapTy;
        PairOutputMapTy PairOutputMap;

        llvm::Instruction* m_root;
        Pattern* m_currentPattern;

        CPlatform             m_Platform;
        bool                  m_AllowContractions;
        bool                  m_NeedVMask;
        bool                  m_samplertoRenderTargetEnable;
    public:
        static char ID;

    private:
        CodeGenContext* m_ctx; // Caching codegen context, valid only within runOnFunction().
        llvm::DominatorTree* DT;
        llvm::LoopInfo* LI;
        const llvm::DataLayout* m_DL;
        WIAnalysis* m_WI;
        LiveVars* m_LivenessInfo;
        PositionDepAnalysis* m_PosDep;
        llvm::BumpPtrAllocator m_allocator;
        // The set of boolean values stored as predicates of a single element.
        // Otherwise, they are expanded to the SIMD size.
        llvm::DenseSet<const llvm::Value*> UniformBools;

        // Find bool values that will be emitted as uniform variables.
        // Otherwise they will be expanded to the SIMD size, by default.
        void gatherUniformBools(llvm::Value* Val);

        // Return true if it is dbg related instruction
        bool IsDbgInst(llvm::Instruction& inst) const;
    };

    //helper
    bool GetModifier(llvm::Instruction& modifier, e_modifier& mod, llvm::Value*& source);
    bool GetModifier(llvm::Value& modifier, e_modifier& mod, llvm::Value*& source);
    bool IsNegate(llvm::Instruction* modifier, llvm::Value*& negateSource);
    bool IsZero(llvm::Value* v);
    bool isAbs(llvm::Value* abs, e_modifier& mod, llvm::Value*& source);
    bool isSat(llvm::Instruction* sat, llvm::Value*& source, bool& isUnsigned);
    bool isOne(llvm::Value* zero);
    bool isMinOrMax(llvm::Value* inst, llvm::Value*& source0, llvm::Value*& source1, bool& isMin, bool& isUnsigned);
    bool isCandidateForConstantPool(llvm::Value * val);
    e_modifier CombineModifier(e_modifier mod1, e_modifier mod2);

    static uint GetNbSources(llvm::Instruction& v)
    {
        if (llvm::CallInst * intrin = llvm::dyn_cast<llvm::CallInst>(&v))
        {
            return IGCLLVM::getNumArgOperands(intrin);
        }
        return v.getNumOperands();
    }

} // namespace IGC
