/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs/KernelArgs.hpp"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instruction.h>
#include <llvm/Analysis/AssumptionCache.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

namespace IGC
{
    // Too many stateful promotion will overwhelm the surface state
    // cache(32 entries per HDC), which will significantly impact the
    // performance. Simply disable stateful promotion after 32 args.
    constexpr uint maxPromotionCount = 32;

    enum class TargetAddressing {
        BINDFUL,
        BINDLESS
    };

    class StatelessToStateful : public llvm::FunctionPass, public llvm::InstVisitor<StatelessToStateful>
    {
    public:
        typedef llvm::DenseMap<const KernelArg*, int> ArgInfoMap;

        static char ID;

        StatelessToStateful();
        StatelessToStateful(TargetAddressing addressing);

        ~StatelessToStateful() {}

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<llvm::AssumptionCacheTracker>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        virtual llvm::StringRef getPassName() const override
        {
            return "StatelessToStateful";
        }

        virtual bool runOnFunction(llvm::Function& F) override;

        void visitLoadInst(llvm::LoadInst& I);
        void visitStoreInst(llvm::StoreInst& I);
        void visitCallInst(llvm::CallInst& I);

    private:
        struct InstructionInfo {
            InstructionInfo(llvm::Instruction* I, llvm::Value* ptr, llvm::Value* offset):
                statelessInst(I), ptr(ptr), offset(offset) {}
            InstructionInfo() = delete;

            void setStatefulAddrspace(unsigned addrspace) { statefulAddrSpace = addrspace; }
            unsigned getStatefulAddrSpace() {
                IGC_ASSERT(statefulAddrSpace);
                return *statefulAddrSpace;
            }
            void setBaseArgIndex(unsigned index) { baseArgIndex = index; }
            unsigned getBaseArgIndex() { return baseArgIndex; }
            llvm::Instruction* const statelessInst;
            llvm::Value* const ptr;
            llvm::Value* const offset;
        private:
            unsigned baseArgIndex = 0;
            std::optional<unsigned> statefulAddrSpace;
        };

        void findPromotableInstructions();
        void addToPromotionMap(llvm::Instruction& I, llvm::Value* Ptr);

        void promote();
        void promoteInstruction(InstructionInfo& InstInfo);
        void promoteLoad(InstructionInfo& InstInfo);
        void promoteStore(InstructionInfo& InstInfo);
        void promoteIntrinsic(InstructionInfo& InstInfo);

        bool doPromoteUntypedAtomics(const llvm::GenISAIntrinsic::ID intrinID, const llvm::GenIntrinsicInst* Inst);
        bool isUntypedAtomic(const llvm::GenISAIntrinsic::ID intrinID);

        // pointerIsPositiveOffsetFromKernelArgument - check if V can trace back to a kernel argument and
        // has positive offset from that argument.
        // ignoreSyncBuffer - when set to true, return false directly if V is from the implicit kernel
        // argument "sync buffer". sync buffer must be stateless access in ZEBinary path so cannot be promoted.
        bool pointerIsPositiveOffsetFromKernelArgument(llvm::Function *F,
                                                       llvm::Value *V,
                                                       llvm::Value *&offset,
                                                       unsigned int &argNumber,
                                                       bool ignoreSyncBuffer);

        // Check if the given pointer value can be traced back to any kernel argument.
        // return the kernel argument if found, otherwise return nullptr.
        const KernelArg* getKernelArgFromPtr(const llvm::PointerType& ptrType, llvm::Value* pVal);

        // check if the given pointer can be traced back to any kernel argument
        bool pointerIsFromKernelArgument(llvm::Value& ptr);

        bool getOffsetFromGEP(
            llvm::Function* F, const llvm::SmallVector<llvm::GetElementPtrInst*, 4>& GEPs,
            uint32_t argNumber, bool isImplicitArg, llvm::Value*& offset);
        llvm::Argument* getBufferOffsetArg(llvm::Function* F, uint32_t ArgNumber);
        void setPointerSizeTo32bit(int32_t AddrSpace, llvm::Module* M);

        // Encode uavIndex in addrspace. Note that uavIndex is not always the same as BTI.
        // Read only images are qualified as SRV resources and have separate indices space.
        // Writeable images and buffers are qualified as UAV resources and also have a
        // separate indices space. So if there is a read_only image and global buffer in the kernel,
        // they will both have `0` encoded in addrspace. The actual BTI will be computed based
        // on BTLayout in EmitVISAPass.
        unsigned encodeBindfulAddrspace(unsigned uavIndex);

        void updateArgInfo(const KernelArg* KA, bool IsPositive);
        void finalizeArgInitialValue(llvm::Function* F);

        const KernelArg* getKernelArg(llvm::Value* Arg)
        {
            IGC_ASSERT_MESSAGE(m_pKernelArgs, "Should initialize it before use!");
            for (const KernelArg& arg : *m_pKernelArgs) {
                if (arg.getArg() == Arg) {
                    return &arg;
                }
            }
            return nullptr;
        }

        const KernelArg* getBufferOffsetKernelArg(const KernelArg* KA)
        {
            IGC_ASSERT_MESSAGE(m_pKernelArgs, "KernelArgs: should initialize it before use!");
            int argno = KA->getAssociatedArgNo();
            for (const KernelArg& arg : *m_pKernelArgs) {
                if (arg.getArgType() == KernelArg::ArgType::IMPLICIT_BUFFER_OFFSET &&
                    arg.getAssociatedArgNo() == argno) {
                    return &arg;
                }
            }
            return nullptr;
        }

        // When true, runtime can generate surface with buffer's original base (creation base)
        bool m_hasBufferOffsetArg = false;

        // When m_hasBufferOffsetArg is true, optional buffer offset
        // can be on or off, which is indicated by this boolean flag.
        bool       m_hasOptionalBufferOffsetArg = false;

        // When true, every messages that are in ptrArg + offset will have offset >= 0.
        bool       m_hasPositivePointerOffset = false;

        // Handle non-gep pointer
        //   For historic reason (probably non-DW aligned arg), non-gep ptr isn't handled.
        //   If this field is true, non-gep ptr shall be handled.
        const bool m_supportNonGEPPtr = false;

        llvm::AssumptionCacheTracker* m_ACT = nullptr;
        llvm::AssumptionCache* getAC(llvm::Function* F)
        {
            return (m_ACT != nullptr ? &m_ACT->getAssumptionCache(*F)
                : nullptr);
        }

        TargetAddressing m_targetAddressing;
        OpenCLProgramContext* m_ctx = nullptr;
        ImplicitArgs* m_pImplicitArgs = nullptr;
        KernelArgs* m_pKernelArgs = nullptr;
        ArgInfoMap   m_argsInfo;
        bool m_changed = false;
        llvm::Function* m_F = nullptr;
        llvm::Module* m_Module = nullptr;

        // Map argument index to a vector of instructions that should be promoted to stateful.
        std::map<unsigned int, std::vector<InstructionInfo>> m_promotionMap;
    };

}
