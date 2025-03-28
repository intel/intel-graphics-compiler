/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/CISACodeGen/PatternMatchPass.hpp"
#include "Compiler/CISACodeGen/DeSSA.hpp"
#include "Compiler/CISACodeGen/CoalescingEngine.hpp"
#include "Compiler/CISACodeGen/BlockCoalescing.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Dominators.h"
#include "llvm/ADT/TinyPtrVector.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/InstVisitor.h>
#include "llvm/Pass.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvm/Support/raw_ostream.h"
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/RegisterEstimator.hpp"
#include <list>
#include <unordered_map>
#include <algorithm>
#include "Probe/Assertion.h"

namespace IGC {
    // SBaseVecDesc and SSubVecDesc together describe subvec to baseVec aliasing
    // BaseVec is aka aliasee; subVec is aka aliaser.
    struct SSubVecDesc;

    struct SBaseVecDesc
    {
        // Minimum aligment required for BaseVector.
        //   For example,
        //     int2 a = ld p
        //     int4 b = {a, x, y}
        //   as 'a' is grf-aligned (ld's return payload), 'b' should be aligned
        //   at grf too in order to make 'a' the part of 'b'.
        e_alignment Align;
        llvm::Value* BaseVector;
        // Keep the original vector type as BaseVector is dessa node value,
        // which could be a different value with a differnt type. For vector
        // aliasing, both sub and base must have the same element size.
        llvm::VectorType* OrigType;
        // All BaseVector's aliasers (subVec)
        llvm::SmallVector<SSubVecDesc*, 16> Aliasers;

        SBaseVecDesc(llvm::Value* V, llvm::Value* OV, e_alignment A)
            : Align(A), BaseVector(V)
            , OrigType(llvm::dyn_cast<llvm::VectorType>(OV->getType()))
        {
            IGC_ASSERT(OrigType != nullptr);
        }
    };

    struct SSubVecDesc
    {
        // Denote a subvector of BaseVector starting at StartElementOffset.
        // StartElementOffset is in the unit of BaseVector's element type.
        //
        // Current implementation assumes that subvector and basevector have
        // the same element size (could be differnt types, such as int32_t
        // and float, etc). Here is the example showing the
        // relationship among them:
        //   Given the aliasing relation:
        //     Aliaser[0:n] -->  BaseVector[0:m]
        //   where (StartElementOffset + n) <= m. Then,
        //     Aliaser = BaseVector[StartElementOffset, StartElementOffset+n]

        // Aliaser and Aliasee
        //   They are dessa node values.
        llvm::Value* Aliaser;

        // Aliasee:
        //llvm::Value* BaseVector;

        // Keep all aliasers of BaseVecotr. Valid for the root entry only,
        // that is, Aliaser == BaseVector
        //llvm::SmallVector<SSubVecDesc*, 16> Aliasers;
        SBaseVecDesc* Aliasee;

        short  StartElementOffset;  // in the unit of BaseVector's element type
        short  NumElts;             // the number of elements of Aliaser

        SSubVecDesc(llvm::Value* V)
            : Aliaser(V), Aliasee(nullptr), StartElementOffset(0)
        {
            IGCLLVM::FixedVectorType* VTy = llvm::dyn_cast<IGCLLVM::FixedVectorType>(V->getType());
            NumElts = VTy ? (short)VTy->getNumElements() : 1;
        }

        // Temporary : tobedeleted
        SSubVecDesc()
            : Aliaser(nullptr), Aliasee(nullptr),
            StartElementOffset(0), NumElts(0)
        {}
    };

    //  Represent a Vector's element at index = EltIx.
    struct SVecElement {
        llvm::Value* Vec;
        llvm::Value* Elem;
        int          EltIx;

        SVecElement() : Vec(nullptr), Elem(nullptr), EltIx(-1) {}
    };

    // A struct for capturing one element aliasing b/w sub-vector/vector
    // Two cases are considered:
    //
    //   case 1: Insert to (x0 and x1 are inserted to y)
    //     case 1.1
    //        int4 x0, x1;
    //        int8 y = (x0, x1);
    //
    //     case 1.2
    //        int4 y = (s0, s1, s2, s3);
    //
    //   case 2: Extract from (y is extracted from x)
    //       int8 x;
    //       int4 y0 = x.s0123 (first half of x)
    //       int4 y1 = x.4567 (second half of x)
    //
    // Corresponding LLVM IRs are some extElt instructions followed by insElt.
    // For example, y0 in case 2 would be:
    //   s0 = extElt BVec, 0
    //   s1 = extElt BVec, 1
    //   s2 = extElt BVec, 2
    //   s3 = extElt BVec, 3
    //
    //   v0 = insElt undef, s0
    //   v1 = insElt v0,    s1
    //   v2 = insElt V1,    s2
    //   v3 = insElt V2,    s3
    //
    // Sometimes, type cast instrutions might be present for s0, s1, s2,and s3
    // before doing insElt.
    //
    struct SVecInsEltInfo {
        llvm::InsertElementInst* IEI;
        llvm::Value* Elt;

        // If Elt is null, EEI must not be null. EEI is used as scalar operand
        // in IEI and is the same as (FromVec, FromVec_eltIx).
        llvm::ExtractElementInst* EEI;
        llvm::Value* FromVec;
        int          FromVec_eltIx;

        SVecInsEltInfo()
            : IEI(nullptr), Elt(nullptr),
            EEI(nullptr), FromVec(nullptr), FromVec_eltIx(0)
        {}
    };

    /// RPE based analysis for querying variable reuse status.
    ///
    /// Let two instructions DInst and UInst be defined in the same basic block,
    ///
    /// DInst = ...
    /// UInst = DInst op Other
    ///
    /// and assume it is legal to use the same CVariable for DInst and UInst. This
    /// analysis determines if this reuse will be applied or not. When overall
    /// register pressure is low, this decision could be most aggressive. When DInst
    /// and UInst are acrossing a high pressure region (defined below), then the
    /// reuse will only be applied less aggressively.
    ///
    /// Denote by RPE(x) the estimated register pressure at point x. Let Threshold
    /// be a predefined threshold constant. We say pair (DInst, UInst) is crossing a
    /// high register pressure region if
    ///
    /// (1) RPE(x) >= Threshold for any x between DInst and UInst (inclusive), or
    /// (2) RPE(x) >= Threshold for any use x of UInst.
    ///
    class VariableReuseAnalysis : public llvm::FunctionPass,
        public llvm::InstVisitor<VariableReuseAnalysis>
    {
    public:
        static char ID;

        VariableReuseAnalysis();
        ~VariableReuseAnalysis() {}

        typedef llvm::SmallVector<SVecInsEltInfo, 32> VecInsEltInfoTy;
        typedef std::unordered_map<llvm::Value*, SSubVecDesc*> AliasMapTy;
        typedef std::unordered_map<llvm::Value*, SBaseVecDesc*> BaseVecMapTy;
        typedef llvm::SmallVector<llvm::Value*, 32> ValueVectorTy;
        typedef llvm::DenseMap<llvm::Value*, llvm::Value*> Val2ValMapTy;

        virtual bool runOnFunction(llvm::Function& F) override;

        // Need to perform this after WI/LiveVars/DeSSA/CoalescingEnging.
        // (todo: check if coalescing can be merged into dessa completely)
        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
            // AU.addRequired<RegisterEstimator>();
            AU.setPreservesAll();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<llvm::DominatorTreeWrapperPass>();
            AU.addRequired<WIAnalysis>();
            AU.addRequired<LiveVarsAnalysis>();
            AU.addRequired<CodeGenPatternMatch>();
            AU.addRequired<DeSSA>();
            AU.addRequired<CoalescingEngine>();
            AU.addRequired<BlockCoalescing>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        llvm::StringRef getPassName() const override {
            return "VariableReuseAnalysis";
        }

        /// Initialize per-function states. In particular, check if the entire function
        /// has a low pressure.
        void BeginFunction(llvm::Function* F, unsigned SimdSize) {
            m_SimdSize = (uint16_t)SimdSize;
            if (m_RPE) {
                if (m_RPE->isGRFPressureLow(m_SimdSize))
                    m_IsFunctionPressureLow = Status::True;
                else
                    m_IsFunctionPressureLow = Status::False;
            }
        }

        bool isCurFunctionPressureLow() const {
            return m_IsFunctionPressureLow == Status::True;
        }

        bool isCurBlockPressureLow() const {
            return m_IsBlockPressureLow == Status::True;
        }

        /// RAII class to initialize and cleanup basic block level cache.
        class EnterBlockRAII {
        public:
            explicit EnterBlockRAII(VariableReuseAnalysis* VRA, llvm::BasicBlock* BB)
                : VRA(VRA) {
                VRA->BeginBlock(BB);
            }
            ~EnterBlockRAII() { VRA->EndBlock(); }
            EnterBlockRAII(const EnterBlockRAII&) = delete;
            EnterBlockRAII& operator=(const EnterBlockRAII&) = delete;
            VariableReuseAnalysis* VRA;
        };
        friend class EnterBlockRAII;

        // Check use instruction's legality and its pressure impact.
        bool checkUseInst(llvm::Instruction* UInst, LiveVars* LV);

        // Check def instruction's legality and its pressure impact.
        bool checkDefInst(llvm::Instruction* DInst, llvm::Instruction* UInst,
            LiveVars* LV);

        // Visitor
        void visitExtractElementInst(llvm::ExtractElementInst& I);

        bool isAliasedValue(llvm::Value* V) {
            if (m_pCtx->getVectorCoalescingControl() > 0) {
                return isAliased(V);
            }
            return false;
        }

        // getRootValue():
        //   return dessa root value; if dessa root value
        //   is null, return itself.
        llvm::Value* getRootValue(llvm::Value* V);
        // getAliasRootValue()
        //   return alias root value if it exists, itself otherwise.
        llvm::Value* getAliasRootValue(llvm::Value* V);  // to be deleted

        /// printAlias - print value aliasing info in human readable form
        void printAlias(llvm::raw_ostream& OS, const llvm::Function* F = nullptr) const;
        /// dumpAalias - dump alias info to dbgs().
        void dumpAlias() const;

        //
        // m_aliasMap:
        //      For mapping aliaser to aliasee: aliaser -> aliasee
        // where aliasee is a vector and aliaser could be a scalar or a vector.
        //
        // Properties of the map:
        //   1. alias root value.
        //      A root value is denoted by a map entry from a value to itself.
        //           Vec0 -> Vec0
        //      Root value is always an aliasee, meaning the map has other
        //      entry like:
        //           Vec1 -> Vec0
        //   2. Any non-root value in this map is either an aliaser or
        //      an aliasee, but not both. For example,
        //       cannot have this:
        //           Vec0 -> Vec1
        //           v0 -> Vec0
        //       Instead, they are represented as follows:
        //           Vec0 -> Vec1
        //           v0   -> Vec1
        //   3. Liveness of aliaser and aliasee are not combined
        //      Unlike dessa alias, in which aliser's liveness is merged
        //      into aliasee's. Here, aliaser's liveness is nerver merged
        //      into aliasee's.
        //
        //  Notation:
        //    aliasCC(v) : all values that have the same alias root as v,
        //                 including alias root.
        //    dessaCC(v) : all values in the same dessa congruent class as v.
        //    subAlias(v, startIx, nelts) :
        //                 all v in aliasCC(v) that overlap v's elements in
        //                 range baseVector[startIx : startIx+nelts-1].
        //  For example,  V is of int4, s0, s1, s2, s3 are scalars that are aliased
        //  to V's element at 0, 1, 2, and 2, respectively.
        //                         s0 --> v[0]
        //                         s1 --> v[1]
        //                         s2 --> v[2]
        //                         s3 --> v[3]
        //   aliasCC(s0) = aliasCC(s1) = aliasCC(s2) = aliasCC(s3) = aliasCC(v)
        //               = {v, s0, s1, s2, s3}
        //   subAlias(v, 2, 2) = {s2, s3, v}  // only s2&s3 overlaps V[2:3]
        //   dessaCC(s0) = { values in the same dessa CC }
        //
        AliasMapTy  m_aliasMap;
        BaseVecMapTy m_baseVecMap;

        // sorted m_baseVecMap for creating cvar in derministic order
        llvm::SmallVector<SBaseVecDesc*, 16> m_sortedBaseVec;

        // Function argument cannot be made a sub-part of another bigger
        // value as it has been assigned a fixed physical GRF. The following
        // map is used for checking if a value is an arg or coalesced with
        // an argument by dessa.
        std::list<llvm::Value*> m_ArgDeSSARoot;
        bool isOrCoalescedWithArg(llvm::Value* V)
        {
            if (llvm::isa<llvm::Argument>(V))
                return true;
            if (m_DeSSA) {
                if (llvm::Value* R = m_DeSSA->getRootValue(V)) {
                    auto IE = m_ArgDeSSARoot.end();
                    auto it = std::find(m_ArgDeSSARoot.begin(), IE, R);
                    return it != IE;
                }
            }
            return false;
        }

        void addVecAlias(llvm::Value* Aliaser, llvm::Value* Aliasee,
            llvm::Value* OrigBaseVec, int Idx,
            e_alignment AliaseeAlign = EALIGN_AUTO);
        SSubVecDesc* getOrCreateSubVecDesc(llvm::Value* V);
        SBaseVecDesc* getOrCreateBaseVecDesc(
            llvm::Value* V, llvm::Value* OV, e_alignment A);
        void getAllAliasVals(
            ValueVectorTy& AliasVals,
            llvm::Value* Aliaser,
            llvm::Value* VecAliasee,
            int    Idx);

        // No need to emit code for instructions in this map due to aliasing
        llvm::DenseMap <llvm::Instruction*, int> m_HasBecomeNoopInsts;

        // For emitting livetime start to visa to assist liveness analysis
        //   1. m_LifetimeAt1stDefInBB :  aliasee -> BB
        //        Once a first def is encounted, add lifetime start and clear
        //        this map entry afterwards.
        //   2. m_LifetimeAtEndOfBB :  BB -> set of values
        //        Add lifetime start for all values in the set at the end of BB.
        llvm::DenseMap<llvm::Value*, llvm::BasicBlock*> m_LifetimeAt1stDefOfBB;
        llvm::DenseMap<llvm::BasicBlock*, llvm::TinyPtrVector<llvm::Value*> > m_LifetimeAtEndOfBB;

    private:
        void reset() {
            m_SimdSize = 0;
            m_IsFunctionPressureLow = Status::Undef;
            m_IsBlockPressureLow = Status::Undef;
            m_aliasMap.clear();
            m_baseVecMap.clear();
            m_sortedBaseVec.clear();
            m_root2AliasMap.clear();
            m_HasBecomeNoopInsts.clear();
            m_LifetimeAt1stDefOfBB.clear();
            m_LifetimeAtEndOfBB.clear();
        }

        // Initialize per-block states. In particular, check if the entire block has a
        // low pressure.
        void BeginBlock(llvm::BasicBlock* BB) {
            IGC_ASSERT(m_SimdSize != 0);
            if (m_RPE) {
                CodeGenContext* context = nullptr;
                context = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
                uint32_t BBPresure = m_RPE->getMaxLiveGRFAtBB(BB, m_SimdSize);
                if (BBPresure <= context->getNumGRFPerThread())
                    m_IsBlockPressureLow = Status::True;
                else
                    m_IsBlockPressureLow = Status::False;
            }
        }

        // Cleanup per-block states.
        void EndBlock() { m_IsBlockPressureLow = Status::Undef; }

        void visitLiveInstructions(llvm::Function* F);

        void setLifeTimeStartPos(
            llvm::Value* RootVal,
            ValueVectorTy& AllVals,
            BlockCoalescing* theBC);
        void postProcessing();

        void sortAliasResult();

        // Return true if this instruction can be converted to an alias
        bool canBeAlias(llvm::CastInst* I);

        // If V has been payload-coalesced, return true.
        bool hasBeenPayloadCoalesced(llvm::Value* V) {
            return (m_coalescingEngine->GetValueCCTupleMapping(V) != nullptr);
        }

        void mergeVariables(llvm::Function* F);
        void InsertElementAliasing(llvm::Function* F);

        llvm::Value* traceAliasValue(llvm::Value* V);
        bool getElementValue(
            llvm::InsertElementInst* IEI, int& IEI_ix,
            llvm::Value*& S,
            llvm::Value*& V, int& V_ix);
        bool getAllInsEltsIfAvailable(
            llvm::InsertElementInst* FirstIEI,
            VecInsEltInfoTy& AllIEIs,
            bool OnlySameBB = false);

        bool processExtractFrom(VecInsEltInfoTy& AllIEIs);
        bool processInsertTo(llvm::BasicBlock* BB, VecInsEltInfoTy& AllIEIs);

        // Check if sub can be aliased to Base[Base_ix:size(sub)-1]
        bool aliasInterfere(llvm::Value* Sub, llvm::Value* Base, int Base_ix);

        // DCC: DeSSA congruent class
        // If any value of V's DCC is an aliaser, return true.
        bool hasAnyDCCAsAliaser(llvm::Value* V) const;
        // If a value of V's DCC, which isn't V, is an aliasee, return true.
        bool hasAnotherDCCAsAliasee(llvm::Value* V) const;
        bool isAliased(llvm::Value* V) const;
        bool isAliaser(llvm::Value* V) const;

        // Returns true for the following pattern:
        //   a = extractElement <vectorType> EEI_Vec, <constant EEI_ix>
        //   b = insertElement  <vectorType> V1,  E,  <constant IEI_ix>
        // where EEI_ix and IEI_ix are constants; Return false otherwise.
        bool getVectorIndicesIfConstant(
            llvm::InsertElementInst* IEI,
            int& IEI_ix,
            llvm::Value*& EEI_Vec,
            int& EEI_ix);

        // For alias(S,B), each of S and B can be one of three states.
        enum class AState {
            SKIP,   // skip aliasing
            OK,     // aliasing okay if the other is target
            TARGET  // aliasing okay if no one is SKIP
        };
        bool isExtractMaskCandidate(llvm::Value* V) const;
        AState getCandidateStateUse(llvm::Value* V) const;
        AState getCandidateStateDef(llvm::Value* V) const;
        bool aliasOkay(AState A, AState B, AState C) const {
            if ((A == AState::TARGET || B == AState::TARGET
                || C == AState::TARGET) && A != AState::SKIP &&
                B != AState::SKIP && C != AState::SKIP) {
                return true;
            }
            return false;
        }
        bool checkSubAlign(e_alignment& BaseAlign,  llvm::Value* Subvec,
            llvm::Value* Basevec, int Base_ix);


        CodeGenContext* m_pCtx;
        WIAnalysis* m_WIA;
        LiveVars* m_LV;
        DeSSA* m_DeSSA;
        CodeGenPatternMatch* m_PatternMatch;
        CoalescingEngine* m_coalescingEngine;
        llvm::DominatorTree* m_DT = nullptr;
        const llvm::DataLayout* m_DL = nullptr;

        llvm::BumpPtrAllocator Allocator;

        /// Current Function; set on entry to runOnFunction
        /// and unset on exit to runOnFunction
        llvm::Function* m_F = nullptr;

        // The register pressure estimator (optional).
        RegisterEstimator* m_RPE;

        // Results may be cached at kernel level or basic block level. Use the
        // following enum to indicate cached flag status.
        enum class Status : int8_t {
            Undef = -1,
            False = 0,
            True = 1
        };

        // Per SIMD-compilation constant. Each compilation needs to initialize the
        // SIMD mode.
        uint16_t m_SimdSize;

        // When this function has low register pressure, reuse can be applied
        // aggressively without checking each individual def-use pair.
        Status m_IsFunctionPressureLow;

        // When this block has low register pressure, reuse can be applied
        // aggressively without checking each individual def-use pair.
        Status m_IsBlockPressureLow;

        // For vector alising on non-isolated values (under VectorAlias >= 2).
        // If a value V is in a dessa CC (not isolated) and V is aliased, add
        // <V's root, V> into the map.  This is a quick check to see if any
        // value in a dessa CC has been aliased (either aliaser or aliasee)
        Val2ValMapTy m_root2AliasMap;

        // Max size of BB for which scalar aliasing will apply.
        // Scalar aliasing will skip for BBs beyond this threshold
        const size_t m_BBSizeThreshold;

        // For vector aliasing heuristic to prevent possible high-reg pressure
        bool skipScalarAliaser(llvm::BasicBlock* BB, llvm::Value* V) const;
    };

    llvm::FunctionPass* createVariableReuseAnalysisPass();

} // namespace IGC
