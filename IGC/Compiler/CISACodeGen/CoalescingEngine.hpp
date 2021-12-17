/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/CISACodeGen/PatternMatchPass.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CISACodeGen/PayloadMapping.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Dominators.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/Allocator.h>
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Probe/Assertion.h"

namespace IGC {

    class CEncoder;
    class CVariable;
    class CodeGenContextWrapper;
    class DeSSA;


    class CoalescingEngine : public llvm::FunctionPass, public llvm::InstVisitor<CoalescingEngine>
    {
        //TODO: this is fixed for now, but once we have pressure heuristic, could be relaxed
        static const int MaxTupleSize = 12;

    public:
        static char ID; // Pass identification, replacement for typeid
        CoalescingEngine();

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;

        virtual void releaseMemory() override {
            Allocator.Reset();

            for (auto itr = m_CCTupleList.begin(),
                iend = m_CCTupleList.end();
                itr != iend; ++itr)
            {
                CCTuple* ccTuple = *itr;
                delete ccTuple;
            }
            m_CCTupleList.clear();
            //Nodes need not be deallocated since they are
            //owned by bump allocator (Allocator), and are destroyed
            //once bump allocator goes out the the scope.
            NodeCCTupleMap.clear();
            ValueNodeMap.clear();
            BBProcessingDefs.clear();
            NodeOffsetMap.clear();
        }

        bool runOnFunction(llvm::Function&) override;

        virtual llvm::StringRef getPassName() const override {
            return "CoalescingEngine";
        }

        /// print - print partitions in human readable form
        void print(llvm::raw_ostream& OS, const llvm::Module* = 0) const override;

        /// dump - Dump the partitions to dbgs().
        void dump() const;

        void ProcessBlock(llvm::BasicBlock* bb);

        //
        bool MatchSingleInstruction(llvm::GenIntrinsicInst* I);


        int GetSingleElementWidth(
            SIMDMode simdMode,
            const llvm::DataLayout* pDL,
            llvm::Value* val)
        {
            int result = 0;
            int mult = 1;
            if (val->getType()->isHalfTy() && simdMode == SIMDMode::SIMD8)
            {
                mult = 2;
            }

            result = int_cast<int>(mult *
                numLanes(simdMode) *
                pDL->getTypeAllocSize(val->getType()));

            return result;
        }

        //
        CVariable* PrepareExplicitPayload(
            CShader* outProgram,
            CEncoder* encoder,
            SIMDMode simdMode,
            const llvm::DataLayout* pDL,
            llvm::Instruction* inst,
            int& payloadOffset);

        CVariable* PrepareUniformUrbWritePayload(
            CShader* shader,
            CEncoder* encoder,
            llvm::GenIntrinsicInst* inst);

        CVariable* PrepareSplitUrbWritePayload(
            CShader* outProgram,
            CEncoder* encoder,
            SIMDMode simdMode,
            uint32_t splitPartNo,
            llvm::Instruction* inst);

        void visitCastInst(llvm::CastInst& I);
        void visitBinaryOperator(llvm::BinaryOperator& I);
        void visitCmpInst(llvm::CmpInst& I);
        void visitPHINode(llvm::PHINode& I);
        void visitUnaryInstruction(llvm::UnaryInstruction& I);
        void visitCallInst(llvm::CallInst& I);
        void visitSelectInst(llvm::SelectInst& I);
        void visitBitCastInst(llvm::BitCastInst& I);
        void visitInstruction(llvm::Instruction& I);
        void visitLoadInst(llvm::LoadInst& I);
        void visitStoreInst(llvm::StoreInst& I);


        //////////////////////////////////////////////////////////////////////////
        struct ElementNode
        {
            enum Flags {
                kRegisterIsolatedFlag = 1,
                kPHIIsolatedFlag = 2
            };
            ElementNode(llvm::Value* _value) :
                value(_value), rank(0)
            {
                parent.setPointer(this);
            }

            ~ElementNode()
            {
            }

            ElementNode* getLeader();

            // Fields:
            llvm::PointerIntPair<ElementNode*, 2> parent;
            llvm::Value* value;
            unsigned rank;
        };

        //////////////////////////////////////////////////////////////////////////
        struct CCTuple
        {
            CCTuple() :
                leftBound(0),
                rightBound(0),
                hasNonHomogeneousElements(false),
                rootInst(nullptr)
            {
            }

            int leftBound;
            int rightBound;


            int GetLeftBound() const
            {
                return leftBound;
            }

            int GetRightBound() const
            {
                return rightBound;
            }


            //cannot be safely extended left/right (used for non-homogeneous payload modeling)
            bool hasNonHomogeneousElements;

            llvm::Instruction* rootInst;

            inline bool HasNonHomogeneousElements() const
            {
                return hasNonHomogeneousElements;
            }

            inline void SetHasNonHomogeneousElements(llvm::Instruction* _rootInst)
            {
                IGC_ASSERT(rootInst == nullptr);
                rootInst = _rootInst;
                hasNonHomogeneousElements = true;
            }

            inline llvm::Instruction* GetRoot() const
            {
                return rootInst;
            }


            inline uint GetNumElements() const
            {
                IGC_ASSERT(rightBound >= leftBound);
                return rightBound - leftBound + 1;
            }

            inline void InitializeIndexWithCCRoot(int index, ElementNode* elNode)
            {
                OffsetToCCMap[index] = elNode;

                ResizeBounds(index);
            }

            inline void ResizeBounds(int index)
            {
                if (index < leftBound)
                {
                    leftBound = index;
                }

                if (index > rightBound)
                {
                    rightBound = index;
                }
            }

            inline void CheckIndexForNonInitializedSlot(int index)
            {
                //if (OffsetToCCMap.count(index) == 0)
                {
                    ResizeBounds(index);
                }
            }


            ElementNode* GetCCNodeWithIndex(int index)
            {
                return OffsetToCCMap[index];
            }

            void AttachNodeAtIndex(int index, ElementNode* node, CoalescingEngine& CE)
            {
                if (OffsetToCCMap.count(index) == 0 || OffsetToCCMap[index] == NULL)
                {
                    IGC_ASSERT(node->value == CE.getRegRoot(node->value));

                    CE.NodeCCTupleMap[node] = this;
                    CE.NodeOffsetMap[node] = index;

                    InitializeIndexWithCCRoot(index, node);
                    CE.CurrentDominatingParent[node->value] = node->value;
                    CE.ImmediateDominatingParent[node->value] = NULL;
                }
                else
                {
                    if (OffsetToCCMap[index] == node)
                    {
                        //Do nothing, it is already there.
                    }
                    else
                    {
                        //Here comes a logic that will attach a new node to CC.
                        IGC_ASSERT(OffsetToCCMap.count(index));
                        IGC_ASSERT(OffsetToCCMap[index]);

                        llvm::Value* ccRootValue = OffsetToCCMap[index]->value;

                        CE.unionRegs(ccRootValue, node->value);

                        llvm::Value* NewParent = CE.GetActualDominatingParent(
                            ccRootValue,
                            llvm::dyn_cast<llvm::Instruction>(node->value));

                        CE.ImmediateDominatingParent[node->value] = NewParent;
                        CE.CurrentDominatingParent[ccRootValue] = node->value;
                    }
                }
            }

            //print in human readable form
            void print(llvm::raw_ostream& OS, const llvm::Module* = 0) const;

            void dump() const;

            //FIXME: not sure whether this is the best choice
        protected:
            llvm::DenseMap<int, ElementNode*> OffsetToCCMap;

            friend class CoalescingEngine;
        };

        friend struct CCTuple;

        ///returns non-null CCtuple pointer if there is at least
        // one value that is 'coalesced' into the tuple.
        // It returns one of those values (e.g. for type insepction)
        // if the condition is true.
        CCTuple* IsAnyValueCoalescedInCCTuple(
            llvm::Instruction* inst,
            const uint numOperands,
            int& zeroBasedPayloadElementOffset,
            llvm::Value*& val);

        ///
        bool IsPayloadCovered(
            llvm::Instruction* inst,
            CCTuple* ccTuple,
            const uint numOperands,
            const int payloadToCCTupleRelativeOffset);


        //FIXME: do a filter over relevant (TGI) instructions
        uint DetermineWeight(llvm::Value* val)
        {
            if (ValueWeightMap.count(val)) {
                return ValueWeightMap[val];
            }
            else {
                uint numUsers = 0;
                {
                    llvm::SmallPtrSet<llvm::User*, 8> touchedUsers;
                    for (llvm::Value::user_iterator i = val->user_begin(), e = val->user_end(); i != e; ++i) {
                        llvm::User* user = *i;
                        if (llvm::isa<llvm::Instruction>(user)) {
                            if (!touchedUsers.count(user))
                            {
                                numUsers++;
                                touchedUsers.insert(user);
                            }
                        }
                    }
                }
                ValueWeightMap[val] = numUsers;
                return numUsers;
            }
        }

        //FIXME: this might not be the most effective way if called multiple times
        bool IsolationFilter(llvm::Value* val) const
        {
            if (isCoalescedByDeSSA(val)) {
                return true;
            }
            if (llvm::dyn_cast<llvm::PHINode>(val)) {
                return true;
            }
            else if (llvm::dyn_cast<llvm::ExtractElementInst>(val)) {
                return true;
            }
            else {
                for (llvm::Value::user_iterator i = val->user_begin(), e = val->user_end(); i != e; ++i)
                {
                    if (llvm::dyn_cast<llvm::PHINode>(*i)) {
                        return true;
                    }
                }
            }

            return false;
        }

        bool IsValIsolated(llvm::Value* val)
        {
            return IsolationFilter(val) ||
                isUniform(val) ||
                llvm::isa<llvm::Argument>(val);
        }

        inline bool IsValConstOrIsolated(llvm::Value* val) const
        {
            return llvm::isa<llvm::Constant>(val) ||
                IsolationFilter(val) ||
                isUniform(val) ||
                llvm::isa<llvm::Argument>(val);
        }


        void IncrementalCoalesce(llvm::BasicBlock*);
        void PrepareTuple(
            const uint numOperands,
            llvm::Instruction* tupleGeneratingInstruction,
            llvm::SmallPtrSet<CCTuple*, 8> & touchedTuplesSet,
            llvm::SmallVector<CCTuple*, 8> & touchedTuples,
            bool& isAnyNodeAnchored,
            bool& isAnyNodeCoalescable);

        void DecideSplit(llvm::Instruction* tupleGeneratingInstruction);

        void CreateTuple(
            const uint numOperands,
            llvm::Instruction* tupleGeneratingInstruction);

        void DetermineAnchor(
            const uint numOperands,
            const llvm::Instruction* tupleGeneratingInstruction,
            CCTuple*& ccTuple,
            int& rootTupleStartOffset,
            int& thisTupleStartOffset);

        //Return true if it is ok to continue, false if interference is detected
        bool EvictOrStop(
            CCTuple* ccTuple,
            const int index,
            llvm::Instruction* tupleGeneratingInstruction,
            bool const forceEviction,
            //out:
            bool& interferes)
        {
            if (!IsInsertionSlotAvailable(ccTuple, index, tupleGeneratingInstruction)) {
                if (forceEviction) {
                    PrepareInsertionSlot(ccTuple, index, tupleGeneratingInstruction);
                    return true; //we can continue
                }
                else {
                    interferes = true;
                    return false; //abort whole 'transaction'
                }
            }
            return true;
        }

        class ProcessInterferencesElementFunctor;

        bool InterferenceCheck(
            const uint numOperands,
            llvm::Instruction* tupleGeneratingInstruction,
            const int offsetDiff,
            CCTuple* ccTuple,
            //out:
            ProcessInterferencesElementFunctor* interferencesFunctor);

        /// \brief return true if element slot is safe for copying-in
        /// canExtend - if true, it is assumed that ccTuple can be 'still' extended
        ///
        ///
        bool IsInsertionSlotAvailable(
            CCTuple* ccTuple,
            const int index,
            llvm::Instruction* tupleGeneratingInstruction,
            const bool canExtend = true);

        void PrepareInsertionSlot(
            CCTuple* ccTuple,
            const int index,
            llvm::Instruction* tupleGeneratingInstruction,
            const bool evictFullCongruenceClass = false);

        bool CheckIntersectionForNonHomogeneous(
            const uint numOperands,
            llvm::Instruction* tupleGeneratingInstruction,
            const int offsetDiff,
            CCTuple* ccTuple);

        void ProcessTuple(llvm::Instruction* tupleGeneratingInstruction);
        void GreedyHeuristic(llvm::Instruction* tupleGeneratingInstruction);

        std::vector<CCTuple*>& GetCCTupleList()
        {
            return m_CCTupleList;
        }

        /// Get the union-root of a register. The root is 0 if the register has been
        /// isolated.
        llvm::Value* getRegRoot(llvm::Value*) const;

        ///Given the currentInstruction and root identifier of CC, find the element
        ///that belongs to CC and is a proper dominator of the currentInstruction.
        ///In straight-line codes and in separate payload and de-ssa phases, this
        ///actually should always give the results in one iteration.
        // Pop registers from the stack represented by ImmediateDominatingParent
        // until we find a parent that dominates the current instruction.
        inline llvm::Value* GetActualDominatingParent(llvm::Value* RootV, llvm::Instruction* currentInstruction)
        {
            llvm::Value* NewParent = CurrentDominatingParent[RootV];

            while (NewParent) {
                if (getRegRoot(NewParent)) {
                    //Fixme: here it does not apply.
                    // we have added the another condition because the domination-test
                    // does not work between two phi-node. See the following comments
                    // from the DT::dominates:
                    // " It is not possible to determine dominance between two PHI nodes
                    //   based on their ordering
                    //  if (isa<PHINode>(A) && isa<PHINode>(B))
                    //    return false;"
                    if (llvm::isa<llvm::Argument>(NewParent)) {
                        break;
                    }
                    else if (DT->dominates(llvm::cast<llvm::Instruction>(NewParent), currentInstruction)) {
                        break;
                    } /* else if (cast<llvm::Instruction>(NewParent)->getParent() == MBB &&
                        isa<PHINode>(DefMI) && isa<PHINode>(NewParent)) {
                            break;
                    } */
                }
                NewParent = ImmediateDominatingParent[NewParent];
            }

            return NewParent;
        }

        //Here we test the interference between the
        //Algorithm:
        //Starting from the currentDominatingParent, walk the congruence class
        //dominance tree upward, until an element that dominates the currentInstruction
        //is found.
        //Since this method could be called on the non-dominance traversal (e.g.
        //currentInstruction dominates some elements in the tree)
        // Say we have a dominance tree that is already constructed (lifetime goes
        // downwards):
        // CC dominance tree
        //    v67
        //    ^
        //    |
        //   v121 (dominating)
        //    ^
        //    |    -----   v181
        //    |
        //   v190 <- (dominated)
        //
        // Want to check the interference with v181, which dominates v190, but is dominated
        // by v121. if we would just look for a dominating element for v181, we would find
        // out that v121 is dominator of v181 and (say) it is not interfering with v181, thus
        // leading to conclusion that there is no interference between v181 and CC dominance tree.

        inline void SymmetricInterferenceTest(
            llvm::Value* RootV,
            llvm::Instruction* currentInstruction,
            llvm::Value*& dominating, //in-out
            llvm::Value*& dominated)
        {
            llvm::Value* NewParent = CurrentDominatingParent[RootV];
            dominating = nullptr;
            dominated = nullptr;

            while (NewParent)
            {
                if (getRegRoot(NewParent)) //not isolated
                {
                    if (llvm::isa<llvm::Argument>(NewParent))
                    {
                        dominating = NewParent;
                        break;
                    }
                    else if (DT->dominates(llvm::cast<llvm::Instruction>(NewParent), currentInstruction))
                    {
                        dominating = NewParent;
                        break;
                    }
                    dominated = NewParent;
                }
                NewParent = ImmediateDominatingParent[NewParent];
            }
        }

        inline CCTuple* GetValueCCTupleMapping(llvm::Value* val)
        {
            llvm::Value* RootV = getRegRoot(val);
            if (!RootV) {
                return NULL;
            }

            IGC_ASSERT(ValueNodeMap.count(RootV));
            auto RI = ValueNodeMap.find(RootV);
            ElementNode* Node = RI->second;

            auto CCI = NodeCCTupleMap.find(Node);
            if (CCI != NodeCCTupleMap.end()) {
                return CCI->second;
            }
            else {
                return NULL;
            }
        }

        /// Caller is responsible for assuring that value is not isolated
        // e.g. by calling GetCalueCCTupleMapping previously.
        inline int GetValueOffsetInCCTuple(llvm::Value* val)
        {
            llvm::Value* RootV = getRegRoot(val);
            IGC_ASSERT(nullptr != RootV);
            IGC_ASSERT(ValueNodeMap.count(RootV));

            auto RI = ValueNodeMap.find(RootV);
            ElementNode* Node = RI->second;

            auto CCI = NodeOffsetMap.find(Node);
            IGC_ASSERT(CCI != NodeOffsetMap.end());
            return CCI->second;
        }

        //////////////////////////////////////////////////////////////////////////
        class ElementFunctor {
        public:
            virtual void SetIndex(int index) = 0;
            virtual bool visitCopy() = 0;
            virtual bool visitConstant() = 0;
            virtual bool visitArgument() = 0;
            virtual bool visitIsolated() = 0;
            virtual bool visitAnchored() = 0;
            virtual bool visitInterfering(llvm::Value* val, const bool evictFullCongruenceClass) = 0;
            virtual bool visitPackedNonInterfering(llvm::Value* val) = 0;
            virtual ~ElementFunctor() {}
        };

        //////////////////////////////////////////////////////////////////////////
        class GatherWeightElementFunctor : public ElementFunctor
        {
        public:
            GatherWeightElementFunctor() :
                nAlignedAnchors(0),
                nInsertionSlotRequired(0),
                nNeedsDisplacement(0)
            {
            }

            virtual void SetIndex(int index)
            {
            }

            virtual bool visitCopy()
            {
                nInsertionSlotRequired++;
                return true;
            }

            virtual bool visitConstant()
            {
                nInsertionSlotRequired++;
                return true;
            }

            ///Not used yet, but is here in an interface
            ///for completeness.
            virtual bool visitArgument()
            {
                return true;
            }

            virtual bool visitIsolated()
            {
                nInsertionSlotRequired++;
                return true;
            }

            ///Visits constrained (anchored) element.
            virtual bool visitAnchored()
            {
                nAlignedAnchors++;
                return true;
            }

            ///Visits a value in a tuple that interferes with some (non-isolated)
            ///element in a CC class that occupies the same slot.
            virtual bool visitInterfering(
                llvm::Value* val,
                const bool evictFullCongruenceClass)
            {
                nNeedsDisplacement++;
                return true;
            }

            virtual bool visitPackedNonInterfering(llvm::Value* val)
            {
                return true;
            }

            ///Gets the number of 'anchored' elements in a tuple.
            inline int GetNumAlignedAnchors() const
            {
                return nAlignedAnchors;
            }

            inline int GetNumInsertionSlotsRequired() const
            {
                return nInsertionSlotRequired;
            }

            ///Gets the number of values in a tuple
            inline int GetNumNeedsDisplacement() const
            {
                return nNeedsDisplacement;
            }

        private:
            int nAlignedAnchors;
            int nInsertionSlotRequired;
            int nNeedsDisplacement;
        };

        //////////////////////////////////////////////////////////////////////////
        class ProcessInterferencesElementFunctor : public ElementFunctor
        {
        private:
            bool m_forceEviction;
            bool m_interferes;
            llvm::Instruction* m_tupleInst;
            const int m_offsetDiff;
            CCTuple* m_ccTuple;
            CoalescingEngine* m_CE;
            int m_index;
            llvm::SmallPtrSet<llvm::Value*, 8> m_valuesForIsolation;

        public:
            ProcessInterferencesElementFunctor(
                bool forceEviction,
                llvm::Instruction* inst,
                const int offsetDiff,
                CCTuple* ccTuple,
                CoalescingEngine* CE) :
                m_forceEviction(forceEviction),
                m_interferes(false),
                m_tupleInst(inst),
                m_offsetDiff(offsetDiff),
                m_ccTuple(ccTuple),
                m_CE(CE),
                m_index(0)
            {

            }

            llvm::SmallPtrSet<llvm::Value*, 8> & GetComputedValuesForIsolation()
            {
                return m_valuesForIsolation;
            }

            inline void SetForceEviction(bool force)
            {
                m_forceEviction = force;
            }

            inline bool IsInterfering() const
            {
                return m_interferes;
            }

            virtual void SetIndex(int index)
            {
                m_index = index;
            }

            virtual bool visitCopy()
            {
                return m_CE->EvictOrStop(
                    m_ccTuple,
                    m_index + m_offsetDiff,
                    m_tupleInst,
                    m_forceEviction,
                    m_interferes);
            }
            virtual bool visitConstant()
            {
                return m_CE->EvictOrStop(
                    m_ccTuple,
                    m_index + m_offsetDiff,
                    m_tupleInst,
                    m_forceEviction,
                    m_interferes);
            }
            virtual bool visitIsolated()
            {
                return m_CE->EvictOrStop(
                    m_ccTuple,
                    m_index + m_offsetDiff,
                    m_tupleInst,
                    m_forceEviction,
                    m_interferes);
            }

            virtual bool visitArgument()
            {
                return m_CE->EvictOrStop(
                    m_ccTuple,
                    m_index + m_offsetDiff,
                    m_tupleInst,
                    m_forceEviction,
                    m_interferes);
            }

            virtual bool visitAnchored()
            {
                return true;
            }

            virtual bool visitInterfering(llvm::Value* val, const bool evictFullCongruenceClass)
            {
                if (m_forceEviction) {

                    // HEURISTIC:
                    if (m_CE->DetermineWeight(val) < 2) {

                        m_CE->PrepareInsertionSlot(
                            m_ccTuple,
                            m_index + m_offsetDiff,
                            m_tupleInst,
                            false //evictFullCongruenceClass
                        );

                        m_valuesForIsolation.insert(val);
                    }
                    else
                    {
                        m_CE->PrepareInsertionSlot(
                            m_ccTuple,
                            m_index + m_offsetDiff,
                            m_tupleInst,
                            true //evictFullCongruenceClass
                        );

                    }
                    return true;
                }
                else {
                    m_interferes = true;
                    return false;
                }

                return true;
            }

            virtual bool visitPackedNonInterfering(llvm::Value* val)
            {
                if (m_CE->DetermineWeight(val) < 2) {
                    m_valuesForIsolation.insert(val);
                }

                return true;
            }
        };

        void ProcessElements(
            const uint numOperands,
            llvm::Instruction* tupleInst,
            const int offsetDiff,
            CCTuple* ccTuple,
            ElementFunctor* functor);

    public:

        uint GetNumPayloadElements(llvm::Instruction* inst)
        {
            if (currentPart_ == 0)
            {
                auto iter = splitPoint_.find(inst);
                if (iter == splitPoint_.end())
                {
                    //means we have no split point, can
                    return m_PayloadMapping.GetNumPayloadElements(inst);
                }
                else
                {
                    IGC_ASSERT((*iter).second > 0);
                    return (*iter).second;
                }
                //return splitPoint_
            }
            else
            {
                IGC_ASSERT(currentPart_ == 1);
                auto iter = splitPoint_.find(inst);
                IGC_ASSERT(iter != splitPoint_.end());
                return m_PayloadMapping.GetNumPayloadElements(inst) - (*iter).second;
            }
        }
        //uint GetNonAdjustedNumPayloadElements_Sample(const SampleIntrinsic *inst)
        //{
        //    return m_PayloadMapping.GetNonAdjustedNumPayloadElements_Sample(inst);
        //}

        int GetNumPayloadElements_Sample(const llvm::SampleIntrinsic* inst)
        {
            return m_PayloadMapping.GetNumPayloadElements_Sample(inst);
        }

        /// Get the mapping from the payload element (at position index)
        /// to intrinsic argument value. Indexing is zero based.
        llvm::Value* GetPayloadElementToValueMapping(const llvm::Instruction* inst, uint index)
        {
            return m_PayloadMapping.GetPayloadElementToValueMapping(inst, currentLowBound_ + index);
        }
        llvm::Value* GetPayloadElementToValueMapping_sample(const llvm::SampleIntrinsic* inst, const uint index)
        {
            return m_PayloadMapping.GetPayloadElementToValueMapping_sample(inst, index);
        }
        llvm::Value* GetNonAdjustedPayloadElementToValueMapping_sample(const llvm::SampleIntrinsic* inst, const uint index)
        {
            return m_PayloadMapping.GetNonAdjustedPayloadElementToValueMapping_sample(inst, index);
        }

        bool HasNonHomogeneousPayloadElements(const llvm::Instruction* inst)
        {
            return m_PayloadMapping.HasNonHomogeneousPayloadElements(inst);
        }
        int GetLeftReservedOffset(const llvm::Instruction* inst, SIMDMode simdMode)
        {
            return m_PayloadMapping.GetLeftReservedOffset(inst, simdMode);
        }

        int GetRightReservedOffset(const llvm::Instruction* inst, SIMDMode simdMode)
        {
            return m_PayloadMapping.GetRightReservedOffset(inst, simdMode);
        }
        const llvm::Instruction* GetSupremumOfNonHomogeneousPart(
            const llvm::Instruction* inst1,
            const llvm::Instruction* inst2)
        {
            return m_PayloadMapping.GetSupremumOfNonHomogeneousPart(inst1, inst2);
        }

        uint GetNumSplitParts(llvm::Instruction* inst)
        {
            auto iter = splitPoint_.find(inst);
            if (iter != splitPoint_.end())
            {
                uint splitIndex = (*iter).second;
                return splitIndex > 0 ? 2 : 1;
            }
            else
            {
                return 1;
            }
        }

        void SetCurrentPart(llvm::Instruction* inst, unsigned int partNum)
        {
            if (partNum == 0)
            {
                currentLowBound_ = 0;
                currentPart_ = partNum;
                //            currentUpperBound_ = splitPoint_[inst];
            }
            else
            {
                IGC_ASSERT(partNum == 1);
                currentLowBound_ = splitPoint_[inst];
                IGC_ASSERT(currentLowBound_ > 0);
                currentPart_ = partNum;
            }
        }

    private:

        CPlatform m_Platform;
        PayloadMapping m_PayloadMapping;
        std::vector<CCTuple*> m_CCTupleList;
        llvm::DominatorTree* DT;
        LiveVars* LV;
        WIAnalysis* WIA;
        DeSSA* m_DeSSA;
        CodeGenPatternMatch* CG;
        llvm::DenseMap<llvm::Value*, ElementNode*> ValueNodeMap;
        llvm::DenseMap<ElementNode*, CCTuple*> NodeCCTupleMap;
        //Mapping root element node to its offset in cc tuple:
        llvm::DenseMap<ElementNode*, int> NodeOffsetMap;
        llvm::DenseMap<llvm::Value*, uint> ValueWeightMap;
        ModuleMetaData* m_ModuleMetadata;
        CodeGenContext* m_CodeGenContext;
        //Maps a basic block to a list of instruction defs to be processed for coalescing (in dominance order)
        llvm::DenseMap<llvm::BasicBlock*, std::vector<llvm::Instruction*> > BBProcessingDefs;


        /* Taken from strong DE SSA */
        // Perform a depth-first traversal of the dominator tree, splitting
        // interferences amongst PHI-congruence classes.

        llvm::BumpPtrAllocator Allocator;

        llvm::DenseMap<llvm::Value*, llvm::Value*> CurrentDominatingParent;
        llvm::DenseMap<llvm::Value*, llvm::Value*> ImmediateDominatingParent;

        llvm::DenseMap<llvm::Instruction*, uint> splitPoint_;
        unsigned currentLowBound_;
        //unsigned currentUpperBound_;
        unsigned currentPart_;

        /// Get the union-root of a PHI. The root of a PHI is 0 if the PHI has been
        /// isolated. Otherwise, it is the original root of its destination and
        /// all of its operands (before they were isolated, if they were).
        llvm::Value* getPHIRoot(llvm::Instruction*) const;

        void unionRegs(llvm::Value*, llvm::Value*);

        bool isUniform(llvm::Value* v) const {
            return (WIA->isUniform(v));
        }

        // Isolate a register.
        void isolateReg(llvm::Value* Val) {
            ElementNode* Node = ValueNodeMap[Val];
            Node->parent.setInt(Node->parent.getInt() | ElementNode::kRegisterIsolatedFlag);
        }
        /* Taken from strong DE SSA */



        struct MIIndexCompare {
            MIIndexCompare(LiveVars* _lv) : LV(_lv) { }

            bool operator()(const llvm::Instruction* LHS, const llvm::Instruction* RHS) const {
                return LV->getDistance(LHS) < LV->getDistance(RHS);
            }

            LiveVars* LV;
        };

        bool isCoalescedByDeSSA(llvm::Value* V) const;
    };

} //namespace IGC
