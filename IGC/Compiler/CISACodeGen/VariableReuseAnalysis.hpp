/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/CISACodeGen/PatternMatchPass.hpp"
#include "Compiler/CISACodeGen/DeSSA.hpp"
#include "Compiler/CISACodeGen/CoalescingEngine.hpp"
#include "Compiler/CISACodeGen/BlockCoalescing.hpp"
#include "common/LLVMWarningsPush.hpp"
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
#include <map>
#include <algorithm>
#include "Probe/Assertion.h"

namespace IGC {

    struct SSubVecDesc
    {
        // Denote a subvector of BaseVector starting at StartElementOffset.
        // StartElementOffset is in the unit of BaseVector's element type.
        //
        // This can potentially denote subvector and basevector relationship
        // among vector values of different element sizes. For now, subvector
        // and basevector have the same element size (could be differnt types,
        // such as int32_t and float, etc). Here is the example showing the
        // relationship among them:
        //   Given the aliasing relation:
        //     Aliaser[0:n] -->  BaseVector[0:m]
        //   where (StartElementOffset + n) <= m. Then,
        //     Aliaser = BaseVector[StartElementOffset, StartElementOffset+n]

        // Aliaser
        //   It is a dessa node value; either scalar or subvector
        llvm::Value* Aliaser;

        // Aliasee:
        llvm::Value* BaseVector;

        // Valid only if this entry is for BaseVector, ie, Aliaser == BaseVector
        llvm::SmallVector<SSubVecDesc*, 16> Aliasers;

        // In the unit of BaseVector's element size
        short  StartElementOffset;  // in the unit of BaseVector's element type
        short  NumElts;             // the number of element of Aliaser

        SSubVecDesc(llvm::Value* V)
            : Aliaser(V), BaseVector(V), StartElementOffset(0)
        {
            IGCLLVM::FixedVectorType* VTy = llvm::dyn_cast<IGCLLVM::FixedVectorType>(V->getType());
            NumElts = VTy ? (short)VTy->getNumElements() : 1;
        }

        // Temporary : tobedeleted
        SSubVecDesc()
            : Aliaser(nullptr), BaseVector(nullptr),
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

    // A temporary struct for capturing vector/sub-vector relation.
    // For example:
    //   extElt:
    //     s0 = extElt From, 4
    //     s1 = extElt From, 5
    //     ...
    //
    //   cast:
    //     s0 = castinst s0
    //     s1 = castinst s1
    //     ...
    //
    //   insELt:
    //     V0 = insElt Undef,  s0, 0
    //     V1 = insElt V0,     s1, 1
    //     ......
    //     Vn = insElt Vn-1,   sn, n
    //
    // where s0-s1 are typically from extElt, but not necessary.
    // And they can from different vectors. Sometimes, castInst
    // are present between ins and ext. Here, two cases are
    // considered:
    //
    //   case 1: Insert to (x0 and x1 are inserted to y)
    //     case 1.1
    //        int4 x0, x1;
    //        int8 y = (x0, x1)
    //
    //     case 1.2
    //        int4 y = (s0, s1, s2, s3)
    //
    //   case 2: Extract from (y is extracted from x)
    //       int8 x
    //       int4 y = x.s0123 (use half of x)
    //
    //
    // A vector is used to keep this info. Each element of the
    // vector corresponds to a single IEI. So, for vector size
    // of N, there are N elements. The vector is defined as the
    // following inside the class:
    //    SmallVector<SVecInsExtInfo, 16> VecInsEltInfoTy
    //
    struct SVecInsEltInfo {
        llvm::InsertElementInst* IEI;
        llvm::Value* Elt;

        // If Elt is null, EEI must not be null, which
        // indicates that (FromVec, FromVec_eltIx) is
        // used as scalar operands in this IEI.
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
        typedef std::map<llvm::Value*, SSubVecDesc*> AliasMapTy;  // ordered map
        typedef llvm::SmallVector<llvm::Value*, 32> ValueVectorTy;
        typedef llvm::DenseMap<llvm::Value*, llvm::Value*> Val2ValMapTy;

        // following to be deleted
        typedef llvm::DenseMap<llvm::Value*, SSubVecDesc> ValueAliasMapTy;
        typedef llvm::DenseMap<llvm::Value*, llvm::TinyPtrVector<llvm::Value*> > AliasRootMapTy;
        typedef llvm::SmallVector<SVecElement, 32> VecEltTy;

        virtual bool runOnFunction(llvm::Function& F) override;

        // Need to perform this after WI/LiveVars/DeSSA/CoalescingEnging.
        // (todo: check if coalescing can be merged into dessa completely)
        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
            // AU.addRequired<RegisterEstimator>();
            AU.setPreservesAll();
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
        //      For mapping aliaser to aliasee:
        //         aliaser -> aliasee
        // where aliasee is a vector value and aliaser could be a scalar or
        // a vector values.
        //
        // Properties of the map:
        //   1. No chain aliases
        //       No following:
        //           Vec0 -> Vec1
        //           v0 -> Vec0
        //       Instead, they are represented as follows:
        //           Vec0 -> Vec1
        //           v0   -> Vec1
        //   2. Aliasee is an aliaser to itself (for convenience)
        //           Vec0 -> Vec0
        //       When this entry is seen, we know Vec0 is an aliasee,
        //       also called a alias root value.
        //   3. Liveness of aliaser and aliasee are not combined
        //      Unlike dessa alias, in which aliser's liveness is merged
        //      into aliasee's. Here, aliaser's liveness is nerver merged
        //      into aliasee's.
        //
        //  Note:
        //     notation:
        //         aliasCC(v) : all values that share the same alias root as v.
        //         dessaCC(v) : all values in the same dessa congruent class as v.
        //         subAlias(v, startIx, nelts) :
        //                      all v in aliasCC(v) whose elements overlap
        //                      v's baseVector[startIx : startIx+nelts-1].
        //  For example,  V is of int4, s0, s1, s2, s3 are scalars that are aliased
        //  to V's element at 0, 1, 2, and 2, respectively.
        //                         s0 --> v[0]
        //                         s1 --> v[1]
        //                         s2 --> v[2]
        //                         s3 --> v[3]
        //   aliasCC(s0) = aliasCC(s1) = aliasCC(s2) = aliasCC(s3) = aliasCC(v)
        //               = {v, s0, s1, s2, s3, s4}
        //   subAlias(v, 2, 2) = {s2, s3, v}  // only s2&s3 overlaps V[2:3]
        //   dessaCC(s0) = { values in the same dessa CC }
        //
        //   [todo] add Algo here.
        //
        AliasMapTy  m_aliasMap;

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
                if (llvm::Value * R = m_DeSSA->getRootValue(V)) {
                    auto IE = m_ArgDeSSARoot.end();
                    auto it = std::find(m_ArgDeSSARoot.begin(), IE, R);
                    return it != IE;
                }
            }
            return false;
        }

        // Add an entry V->itself if not existing yet.
        void addVecAlias(llvm::Value* Aliaser, llvm::Value* Aliasee, int Idx);
        SSubVecDesc* getOrCreateSubVecDesc(llvm::Value* V);
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
            VecInsEltInfoTy& AllIEIs);

        bool processExtractFrom(VecInsEltInfoTy& AllIEIs);
        bool processInsertTo(VecInsEltInfoTy& AllIEIs);

        // Check if sub can be aliased to Base[Base_ix:size(sub)-1]
        bool aliasInterfere(llvm::Value* Sub, llvm::Value* Base, int Base_ix);

        // DCC: DeSSA congruent class
        // If any of V's DCC is an aliaser, return true.
        bool hasAnyOfDCCAsAliaser(llvm::Value* V) const;
        bool hasAnotherInDCCAsAliasee(llvm::Value* V) const;
        bool isAliased(llvm::Value* V) const;

        // Returns true for the following pattern:
        //   a = extractElement <vectorType> EEI_Vec, <constant EEI_ix>
        //   b = insertElement  <vectorType> V1,  E,  <constant IEI_ix>
        // where EEI_ix and IEI_ix are constants; Return false otherwise.
        bool getVectorIndicesIfConstant(
            llvm::InsertElementInst* IEI,
            int& IEI_ix,
            llvm::Value*& EEI_Vec,
            int& EEI_ix);


        CodeGenContext* m_pCtx;
        WIAnalysis* m_WIA;
        LiveVars* m_LV;
        DeSSA* m_DeSSA;
        CodeGenPatternMatch* m_PatternMatch;
        CoalescingEngine* m_coalescingEngine;
        llvm::DominatorTree* m_DT;
        const llvm::DataLayout* m_DL;

        llvm::BumpPtrAllocator Allocator;

        /// Current Function; set on entry to runOnFunction
        /// and unset on exit to runOnFunction
        llvm::Function* m_F;

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

        // Temporaries under VATemp
        // if a value V is in a dessa CC and V is aliased,
        // add <V's root, V> into the map.  This is a quick
        // way to check if any of values in a dessa CC has
        // been aliased (either aliaser or aliasee)
        Val2ValMapTy m_root2AliasMap;
    };

    llvm::FunctionPass* createVariableReuseAnalysisPass();

} // namespace IGC
