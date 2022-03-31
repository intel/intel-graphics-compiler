/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Instructions.h>
#include <llvm/ADT/DenseMap.h>
#include "llvm/ADT/MapVector.h"
#include <llvm/ADT/ilist.h>
#include <llvm/ADT/SetVector.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/Support/Debug.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include "common/LLVMWarningsPop.hpp"
#include <llvm/ADT/DenseSet.h>

#include <string>
#include <sstream>
#include <set>
#include <map>

namespace IGC
{

    class CodeGenContextWrapper;

    // Maximum width supported as input
#define MAX_INPUT_VECTOR_WIDTH 16

// Define estimated amount of instructions in function
#define ESTIMATED_INST_NUM 128

/// @brief Scalarization pass used for converting code in functions
///  which operate on vector types, to work on scalar types (by breaking
///  data elements to scalars, and breaking each vector operation
///  to several scalar operations).
///  Functions are also replaced (similar to instructions), according
///  to data received from RuntimeServices.

    class ScalarizeFunction : public llvm::FunctionPass
    {
    public:
        static char ID; // Pass identification, replacement for typeid

        ScalarizeFunction(bool selectiveScalarization = false);

        ~ScalarizeFunction();

        /// @brief Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "ScalarizeFunction";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
            AU.setPreservesCFG();
        }

        virtual bool runOnFunction(llvm::Function& F) override;

    private:

        /// @brief select an exclusive set that would not be scalarized
        void buildExclusiveSet();
        /// @brief main Method for dispatching instructions (according to inst type) for scalarization
        /// @param I instruction to dispatch
        void dispatchInstructionToScalarize(llvm::Instruction* I);

        /// @brief Instructions which cannot be Scalarized reach this function.
        ///  They may have a vector value, so create empty SCM entries for them if needed,
        ///  and also re-create any vector input which may have been scalarized
        /// @param Inst instruction to work on
        void recoverNonScalarizableInst(llvm::Instruction* Inst);

        /*! \name Scalarizarion Functions
         *  \{ */
         /// @brief Scalarize an instruction
         /// @param I Instruction to scalarize
        void scalarizeInstruction(llvm::BinaryOperator* BI);
        void scalarizeInstruction(llvm::CmpInst* CI);
        void scalarizeInstruction(llvm::CastInst* CI);
        void scalarizeInstruction(llvm::PHINode* CI);
        void scalarizeInstruction(llvm::SelectInst* SI);
        void scalarizeInstruction(llvm::ExtractElementInst* SI);
        void scalarizeInstruction(llvm::InsertElementInst* II);
        void scalarizeInstruction(llvm::ShuffleVectorInst* SI);
        void scalarizeInstruction(llvm::CallInst* CI);
        void scalarizeInstruction(llvm::AllocaInst* CI);
        void scalarizeInstruction(llvm::GetElementPtrInst* CI);

        /*! \name Scalarizarion Utility Functions
         *  \{ */

         /// @brief Takes a vector value, and returns the scalarized "breakdown" of that value
         /// @param retValues Array for returning scalar elements in
         /// @param retIsConstant Return (by reference) if the given value is a constant
         /// @param origValue Vector value to obtain elements from
         /// @param origInst Instruction for which service is requested (may be used as insertion point)
        void obtainScalarizedValues(llvm::SmallVectorImpl<llvm::Value*>& retValues, bool* retIsConstant,
            llvm::Value* origValue, llvm::Instruction* origInst, int dstIdx = -1);


        /// @brief a set contains vector from original kernel that need to be used after sclarization
        llvm::SmallSetVector<llvm::Value*, ESTIMATED_INST_NUM> m_usedVectors;

        /// @brief update museVectors set with the vectori value to be obtained at when scalarization finish
        /// @param vectorVal Vector being added to set
        void obtainVectorValueWhichMightBeScalarized(llvm::Value* vectorVal);

        /// @brief Given a vector value, check if still exists, or rebuild from scalar elements
        ///  this funciton assumes the SCM map is updated and thus should be run after sclarization is finished
        /// @param vectorVal Vector being checked
        void obtainVectorValueWhichMightBeScalarizedImpl(llvm::Value* vectorVal);

        /// @brief obtaining vector values that are needed after scalarizaion by invoking
        ///  obtainVectorValueWhichMightBeScalarizedImpl over m_usedVectors
        void resolveVectorValues();

        /// @brief Resolve deferred insts (Values which were scalarized with dummies)
        void resolveDeferredInstructions();

        /*! \} */

        /// @brief Pointer to current function's context
        llvm::LLVMContext* m_moduleContext;
        /// @brief Accessor to current function's context
        llvm::LLVMContext& context() { return *m_moduleContext; }
        /// @brief Pointer to current function
        llvm::Function* m_currFunc;

        /// @brief Set containing all the removed instructions in the function.
        llvm::SmallDenseSet<llvm::Instruction*, ESTIMATED_INST_NUM> m_removedInsts;
        /// @brief Counters for "transpose" statistics
        int m_transposeCtr[llvm::Instruction::OtherOpsEnd];

        /// <summary>
        /// @brief The instructions we do not want to scalarize
        /// </summary>
        std::set<llvm::Value*> m_Excludes;

        /// @brief The SCM (scalar conversions map). Per each value - map of its scalar elements
        struct SCMEntry
        {
            llvm::SmallVector<llvm::Value*, MAX_INPUT_VECTOR_WIDTH>scalarValues;
            bool isOriginalVectorRemoved;
        };
        llvm::DenseMap<llvm::Value*, SCMEntry*> m_SCM;

        /// @brief called to create a new SCM entry. If entry already exists - return it instead
        /// @param origValue Value pointer to search in SCM
        /// @return pointer to found or created SCM entry
        SCMEntry* getSCMEntry(llvm::Value* origValue);

        /// @brief called to update values in SCM entry
        /// @param entry SCM entry to update
        /// @param scalarValues array of values to place in SCMEntry
        /// @param origValue Value which is the key of the SCMEntry
        /// @param isOrigValueRemoved True if original (vector) value was erased during scalarization
        /// @param matchDbgLoc True if we want to match debug loc of the scalar value to orig Value.
        void updateSCMEntryWithValues(SCMEntry* entry, llvm::Value* scalarValues[],
            const llvm::Value* origValue, bool isOrigValueRemoved,
            bool matchDbgLoc = true);

        /// @brief returns an SCM entry if it exists. otherwise return NULL.
        /// @param origValue Value used as key in SCM
        /// @return SCMEntry if found, NULL otherwise
        SCMEntry* getScalarizedValues(llvm::Value* origValue);

        /// @brief release all allocations of SCM entries
        void releaseAllSCMEntries();

        /// @brief create the dummy function which is called to signify a dummy value
        inline llvm::Function* getOrCreateDummyFunc(llvm::Type* dummyType, llvm::Module* module) {
            if (createdDummyFunctions.find(dummyType) == createdDummyFunctions.end()) {
                llvm::FunctionType* funcType = llvm::FunctionType::get(dummyType, false);
                llvm::Function* function = llvm::Function::Create(funcType, llvm::Function::InternalLinkage, "", module);
                createdDummyFunctions[dummyType] = function;
                return function;
            }
            else
                return createdDummyFunctions[dummyType];
        }

        /// @brief deletes the memory associated with all the created dynamic Function objects in the map
        inline void destroyDummyFunc() {
            for (auto& function : createdDummyFunctions) {
                if (function.second) {
                    function.second->eraseFromParent();
                    function.second = nullptr;
                }
            }
        }

        /// @brief An array of available SCMEntry's
        SCMEntry* m_SCMAllocationArray;

        /// @brief Index, in "SCMAllocationArray", of next free SCMEntry
        unsigned m_SCMArrayLocation;

        /// @brief Vector containing all the "SCMAllocationArray" arrays which were allocated
        llvm::SmallVector<SCMEntry*, 4> m_SCMArrays;

        /// @brief The DRL (Deferred resolution list).
        typedef struct DRLEntry
        {
            llvm::Value* unresolvedInst;
            llvm::SmallVector<llvm::Value*, MAX_INPUT_VECTOR_WIDTH>dummyVals;
        } DRLEntry;
        llvm::SmallVector<DRLEntry, 4> m_DRL;

        /*! \name Pre-Scalarization function arguments scan
         *  \{ */

         /// @brief Data structure for holding "real" inputs/output of function call
         //   first - arguments of function
         //   second - retuns of function. There may be more than one return value, e.g. sincos
        typedef std::pair< llvm::SmallVector<llvm::Value*, 4>, llvm::SmallVector<llvm::Value*, 4>    > funcRootsVect;
        /// @brief Some getters which access funcRootsVect and make the code more readable
        static llvm::SmallVectorImpl<llvm::Value*>& getReturns(funcRootsVect& FRV) { return FRV.second; }
        static const llvm::SmallVectorImpl<llvm::Value*>& getReturns(const funcRootsVect& FRV) { return FRV.second; }
        static llvm::SmallVectorImpl<llvm::Value*>& getArgs(funcRootsVect& FRV) { return FRV.first; }
        static const llvm::SmallVectorImpl<llvm::Value*>& getArgs(const funcRootsVect& FRV) { return FRV.first; }

        /// @brief flag for selective scalarization
        bool m_SelectiveScalarization;

        /// @brief This holds DataLayout of processed module
        const llvm::DataLayout* m_pDL;

        /// @brief This holds all the created dummy functions throughout the lifetime of the pass, and manages their memory
        llvm::MapVector<llvm::Type*, llvm::Function*> createdDummyFunctions;
    };

} // namespace IGC

/// By default (no argument given to this function), vector load/store are kept as is.
extern "C" llvm::FunctionPass* createScalarizerPass(bool selectiveScalarization = false);
