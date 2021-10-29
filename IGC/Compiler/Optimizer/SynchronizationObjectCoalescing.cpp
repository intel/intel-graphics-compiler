/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/PassInfo.h"
#include "llvm/PassRegistry.h"
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "IGC/Compiler/CodeGenPublic.h"
#include "Probe/Assertion.h"
#include "Compiler/IGCPassSupport.h"
#include "SynchronizationObjectCoalescing.hpp"
#include <memory>
#include <utility>
#include <map>

namespace IGC
{

#if _DEBUG
////////////////////////////////////////////////////////////////////////
struct ExplanationEntry
{
    ////////////////////////////////////////////////////////////////////////
    enum Cause
    {
        GlobalMemoryRedundancy,
        L1CacheInvalidationRedundancy,
        StrictRedundancy,
        FastStrictRedundancy
    };

    Cause m_Cause;

    typedef std::tuple<const llvm::BasicBlock*, uint32_t/*index*/, bool/*isL1CacheInvalidationRedundancy*/, bool/*isGlobalMemoryRedundancy*/> SyncInstDescription;

    SyncInstDescription m_SynchronizationDescription;
    std::vector<SyncInstDescription> m_ForwardBoundaries;
    std::vector<SyncInstDescription> m_BackwardBoundaries;
    std::vector<SyncInstDescription> m_ThreadGroupBarriersBoundaries;
    std::vector<const llvm::Instruction*> m_ForwardMemoryInstructions;
    std::vector<const llvm::Instruction*> m_BackwardMemoryInstructions;
};
#endif // _DEBUG

////////////////////////////////////////////////////////////////////////
/// @brief Synchronization objects prevents from hazardous situations in kernels.
/// They cannot be removed if one of hazards is possible:
/// - Read-after-write (RAW) - "read too soon",
/// - Write-after-read(WAR) - "written too soon",
/// - Write-after-write(WAW) - "written out-of-order".
/// Synchronization objects are following:
/// - memory fences - they synchronize caches with destination memory to the defined
///   extent and ensures that memory accesses finish before the execution
///   of the synchronization,
/// - control barriers - they ensures not until all the threads of the defined scope
/// reached can any of them pass this instruction.
/// Atomic instructions can also be used for synchronizing or predicting the order of
/// execution. Such instructions introduces a complex way of synchronization which
/// can hide some potential hazards. In this context they must be handled as well.
/// This pass analyzes all paths of the shaders to find dependencies
/// between memory read and write instructions and synchronization instructions.
/// The result of this analysis indicates redundancies in the following synchronization
/// instructions:
/// - thread group barriers:  llvm.genx.GenISA.threadgroupbarrier;
///   (interacts with global, typed, shared memory buffers, outputs and atomics)
/// - memory fence: llvm.genx.GenISA.memoryfence;
///   depending on the HW platforms and intrinsic arguments the fence can synchronize
///   global(untyped and typed) memory and/or local memory(SLM)
///   (interacts with global, typed, shared memory buffers, outputs and atomics)
/// Effects of this are following:
/// 1) [a strict redundancy] removing unnecessary usage of the synchronization instructions can be
///    performed safely if none of such paths which meet at least one of the following order of visitation
///    (not crossed by a substitutive synchronization**):
///    - any write instruction, this synchronization instruction, any read instruction (RAW),
///    - any write instruction, this synchronization instruction, any write instruction (WAW),
///    - any write instruction, this synchronization instruction, any atomic instruction (RAW or WAW),
///    - any atomic instruction, this synchronization instruction, any read instruction (RAW),
///    - any atomic instruction, this synchronization instruction, any write instruction (WAR or WAW),
///    - (only for barriers or dependent fences****) any read instruction, this synchronization instruction,
///      any write instruction (WAR),
///    - any read instruction, this synchronization instruction, any atomic instruction (RAW),
///    - (only for barriers) any atomic instruction, this synchronization instruction, any atomic instruction (RAW, WAW or WAR).
///    * in this context read and write instructions contribute to the same type of memory.
///    ** [Only for the strict redundancy case] If a fence neighbors with barriers forwardly (there is no path between this fence and a barrier which
///      crosses a substitute of this fence), the area of searching is increased to next synchronization area which can be synchronized (there is
///      a substitute of this fence). This area can be depicted in this parenthesis: this fence -> ((barrier -> substitute*** -> barrier***))
///    *** this elements can be missing in the shader
///    **** dependent fences means that such fences neighbor with barriers forwardly (there is no path between these fences and a barrier which
///      crosses a substitute of these fences).
/// 2) [a partial redundancy] removing the global memory association with untyped memory fences (for platforms with dependent SLM fences)
///    can be performed safely if none of such paths which meet at least one of the following order of visitation
///    (not crossed by a substitutive synchronization):
///    - any buffer write instruction, this synchronization instruction, any buffer read instruction (RAW),
///    - any buffer write instruction, this synchronization instruction, any buffer write instruction (WAW),
///    - any buffer write instruction, this synchronization instruction, any atomic instruction (RAW or WAW),
///    - any atomic instruction, this synchronization instruction, any buffer read instruction (RAW).
/// 3) [a partial redundancy] disabling L1 cache invalidation (invalidation functionality) can be performed
///    safely if none of such paths which meet at least one of the following order of visitation (not crossed
///    by a substitutive synchronization):
///    - any buffer write instruction, this synchronization instruction, any buffer read instruction (RAW),
///    - any buffer write instruction, this synchronization instruction, any atomic instruction (RAW or WAW),
///    - any atomic instruction, this synchronization instruction, any buffer read instruction (WAR).
///    * L1 cache invalidation synchronizes read instructions.
/// Examples:
/// a) a required thread group barrier (write-barrier-read):
///      store float 5.000000e-01, float addrspace(3)* %store_address, align 4
///      call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false)
///      call void @llvm.genx.GenISA.threadgroupbarrier()
///      load float 5.000000e-01, float addrspace(3)* %load_address, align 4
/// b) a required thread group barrier (read-barrier-write):
///      load float 5.000000e-01, float addrspace(3)* %load_address, align 4
///      call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false)
///      call void @llvm.genx.GenISA.threadgroupbarrier()
///      store float 5.000000e-01, float addrspace(3)* %store_address, align 4
/// c) a required thread group barrier (write-barrier-write):
///      store float 5.000000e-01, float addrspace(3)* %store_address, align 4
///      call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false)
///      call void @llvm.genx.GenISA.threadgroupbarrier()
///      store float 5.000000e-01, float addrspace(3)* %store_address1, align 4
/// d) an unneeded thread group barrier (read-barrier-read):
///      load float 5.000000e-01, float addrspace(3)* %load_address, align 4
///      call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false) ; an unneeded instruction
///      call void @llvm.genx.GenISA.threadgroupbarrier()                                                             ; an unneeded instruction
///      load float 5.000000e-01, float addrspace(3)* %load_address1, align 4
/// e) an unneeded thread group barrier (write-barrier-no read/write):
///      store float 5.000000e-01, float addrspace(3)* %store_address, align 4
///      call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false) ; an unneeded instruction
///      call void @llvm.genx.GenISA.threadgroupbarrier()                                                             ; an unneeded instruction
/// f) an unneeded thread group barrier (no read/write-barrier-write):
///      call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false) ; an unneeded instruction
///      call void @llvm.genx.GenISA.threadgroupbarrier()                                                             ; an unneeded instruction
///      store float 5.000000e-01, float addrspace(3)* %store_address1, align 4
/// f) an unneeded thread group barrier (no read/write-barrier-no read/write):
///      call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false) ; an unneeded instruction
///      call void @llvm.genx.GenISA.threadgroupbarrier()                                                             ; an unneeded instruction
///
/// Note: The current implementation analyzes with low granularity. It means there is no detection if
/// there exist synchronization dependencies between the same area of memory.
/// Moreover, unsynchronized read instructions can be such instructions which are used only after thread
/// group barriers.
/// Additionally, the pass assumes all possible call instructions are inlined.
///
/// Dictionary for some terms:
/// 1) synchronization area - this is such an area where all threads in the same workgroup can be run independently. However the start point and
///    end point is the same for the bunch of threads. Such synchronization area can be drawn by an entry point, thread group barriers and
///    end of thread instructions.
/// 2) unsynchronized instruction - this is such a memory instruction which can finish after an imaginatively removed synchronization point. Read
///    instructions must be finished before the synchronization point or first usage. Writes instructions are only synchronized with the first
///    encountered synchronization point. The missing synchronization point for such an instruction can be destructive for the expected order
///    of memory operations.
/// 3) boundary instruction - this is such a synchronization instruction which draws a border for searching.
/// 4) substitute instruction - this is such a synchronization instruction which work at least so extensively as the reference instruction. It
///    means that the substitute instruction can replace the reference instruction without any regressive functional changes in the shader.
class SynchronizationObjectCoalescingAnalysis : public llvm::FunctionPass
{
public:
    static char ID; ///< ID used by the llvm PassManager (the value is not important)

    SynchronizationObjectCoalescingAnalysis();

    ////////////////////////////////////////////////////////////////////////
    virtual bool runOnFunction(llvm::Function& F);

    ////////////////////////////////////////////////////////////////////////
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const;

    ////////////////////////////////////////////////////////////////////////
    const llvm::DenseSet<llvm::Instruction*>& GetRedundantInstructions() const;

    ////////////////////////////////////////////////////////////////////////
    const llvm::DenseSet<llvm::Instruction*>& GetInvalidationFunctionalityRedundancies() const;

    ////////////////////////////////////////////////////////////////////////
    const llvm::DenseSet<llvm::Instruction*>& GetGlobalMemoryRedundancies() const;

#if _DEBUG
    ////////////////////////////////////////////////////////////////////////
    void print(llvm::raw_ostream& stream, bool onlyMemoryInstructionMask) const;

    ////////////////////////////////////////////////////////////////////////
    void dump(bool onlyMemoryInstructionMask = true) const;
#endif // _DEBUG
private:

    ////////////////////////////////////////////////////////////////////////
    enum InstructionMask : uint32_t
    {
        None = 0x0,
        AtomicOperation = 0x1,
        TypedReadOperation = 0x2,
        TypedWriteOperation = 0x4,
        OutputUrbReadOperation = 0x8,
        UrbWriteOperation = 0x10,
        BufferReadOperation = 0x20,
        BufferWriteOperation = 0x40,
        SharedMemoryReadOperation = 0x80,
        SharedMemoryWriteOperation = 0x100
    };

    static constexpr InstructionMask sc_MemoryWriteInstructionMask = static_cast<InstructionMask>(
        AtomicOperation |
        TypedWriteOperation |
        UrbWriteOperation |
        BufferWriteOperation |
        SharedMemoryWriteOperation);

    static constexpr InstructionMask sc_MemoryReadInstructionMask = static_cast<InstructionMask>(
        AtomicOperation |
        TypedReadOperation |
        OutputUrbReadOperation |
        BufferReadOperation |
        SharedMemoryReadOperation);

    ////////////////////////////////////////////////////////////////////////
    enum SynchronizationCaseMask : uint32_t
    {
        ReadSyncWrite = 0x1,
        WriteSyncWrite = 0x2,
        AtomicSyncRead = 0x4,
        AtomicSyncWrite = 0x8,
        WriteSyncAtomic = 0x10,
        ReadSyncAtomic = 0x20,
        WriteSyncRead = 0x40,
    };

    static constexpr SynchronizationCaseMask sc_FullSynchronizationCaseMask = static_cast<SynchronizationCaseMask>(
        WriteSyncRead |
        WriteSyncWrite |
        AtomicSyncRead |
        AtomicSyncWrite |
        WriteSyncAtomic |
        ReadSyncAtomic |
        ReadSyncWrite);

    ////////////////////////////////////////////////////////////////////////
    void Analyze();

    ////////////////////////////////////////////////////////////////////////
    void InvalidateMembers();

    ////////////////////////////////////////////////////////////////////////
    void GatherInstructions();

    ////////////////////////////////////////////////////////////////////////
    void FindRedundancies();

    ////////////////////////////////////////////////////////////////////////
    void GetVisibleMemoryInstructions(
        const llvm::Instruction* pSourceInst,
        bool forwardDirection,
        std::vector<const llvm::Instruction*>& boundaryInstructions,
        std::vector<const llvm::Instruction*>& memoryInstructions) const;

    ////////////////////////////////////////////////////////////////////////
    void GetAllUnsynchronizedMemoryInstructions(
        const llvm::Instruction* pSourceInst,
        std::vector<const llvm::Instruction*>& threadGroupBarriers,
        std::vector<const llvm::Instruction*>& memoryInstructions) const;

    ////////////////////////////////////////////////////////////////////////
    InstructionMask GetInstructionMask(
        const llvm::Instruction* pSourceInst,
        bool forwardDirection) const;

    ////////////////////////////////////////////////////////////////////////
    InstructionMask GetUnsynchronizedForwardInstructionMask(
        const llvm::Instruction* pSourceInst) const;

    ////////////////////////////////////////////////////////////////////////
    InstructionMask GetInstructionMask(
        const std::vector<const llvm::Instruction*>& input) const;

    ////////////////////////////////////////////////////////////////////////
    InstructionMask GetDefaultWriteMemoryInstructionMask(
        const llvm::Instruction* pSourceInst) const;

    ////////////////////////////////////////////////////////////////////////
    InstructionMask GetDefaultMemoryInstructionMask(
        const llvm::Instruction* pSourceInst) const;

    ////////////////////////////////////////////////////////////////////////
    llvm::DenseSet<llvm::Instruction*> GetAllSubsituteInstructions(
        const llvm::Instruction* pReferenceInst) const;

    ////////////////////////////////////////////////////////////////////////
    bool IsSubsituteInstruction(
        const llvm::Instruction* pEvaluatedInst,
        const llvm::Instruction* pReferenceInst) const;

    ////////////////////////////////////////////////////////////////////////
    InstructionMask GetInstructionMask(const llvm::Instruction* pInst) const;

    ////////////////////////////////////////////////////////////////////////
    static bool IsSyncInstruction(const llvm::Instruction* pInst);

    ////////////////////////////////////////////////////////////////////////
    static bool IsMemoryInstruction(const llvm::Instruction* pInst);

    ////////////////////////////////////////////////////////////////////////
    static bool IsReadMemoryInstruction(const llvm::Instruction* pInst);

    ////////////////////////////////////////////////////////////////////////
    static bool IsWriteMemoryInstruction(const llvm::Instruction* pInst);

    ////////////////////////////////////////////////////////////////////////
    static bool IsAtomicOperation(const llvm::Instruction* pInst);

    ////////////////////////////////////////////////////////////////////////
    static bool IsTypedReadOperation(const llvm::Instruction* pInst);

    ////////////////////////////////////////////////////////////////////////
    static bool IsTypedWriteOperation(const llvm::Instruction* pInst);

    ////////////////////////////////////////////////////////////////////////
    static bool IsOutputUrbReadOperation(const llvm::Instruction* pInst);

    ////////////////////////////////////////////////////////////////////////
    static bool IsUrbWriteOperation(const llvm::Instruction* pInst);

    ////////////////////////////////////////////////////////////////////////
    static bool IsBufferReadOperation(const llvm::Instruction* pInst);

    ////////////////////////////////////////////////////////////////////////
    static bool IsBufferWriteOperation(const llvm::Instruction* pInst);

    ////////////////////////////////////////////////////////////////////////
    static bool IsSharedMemoryReadOperation(const llvm::Instruction* pInst);

    ////////////////////////////////////////////////////////////////////////
    static bool IsSharedMemoryWriteOperation(const llvm::Instruction* pInst);

    ////////////////////////////////////////////////////////////////////////
    static bool IsThreadBarrierOperation(const llvm::Instruction* pInst);

    ////////////////////////////////////////////////////////////////////////
    static bool IsUntypedMemoryFenceOperation(const llvm::Instruction* pInst);

    ////////////////////////////////////////////////////////////////////////
    bool IsUntypedMemoryFenceOperationWithInvalidationFunctionality(const llvm::Instruction* pInst) const;

    ////////////////////////////////////////////////////////////////////////
    bool IsUntypedMemoryFenceOperationForSharedMemoryAccess(const llvm::Instruction* pInst) const;

    ////////////////////////////////////////////////////////////////////////
    bool IsUntypedMemoryFenceOperationForGlobalAccess(const llvm::Instruction* pInst) const;

    ////////////////////////////////////////////////////////////////////////
    static bool IsFenceOperation(const llvm::Instruction* pInst);

    ////////////////////////////////////////////////////////////////////////
    static bool IsGlobalResource(llvm::Type* pResourePointerType);

    ////////////////////////////////////////////////////////////////////////
    static bool IsSharedMemoryResource(llvm::Type* pResourePointerType);

    std::vector<llvm::Instruction*> m_UntypedMemoryFences;
    std::vector<llvm::Instruction*> m_ThreadGroupBarriers;

    llvm::DenseSet<llvm::Instruction*> m_RedundantInstructions;
    llvm::DenseSet<llvm::Instruction*> m_InvalidationFunctionalityRedundancies;
    llvm::DenseSet<llvm::Instruction*> m_GlobalMemoryRedundancies;

    llvm::Function* m_CurrentFunction = nullptr;
    bool m_HasIndependentSharedMemoryFenceFunctionality = false;
    InstructionMask m_GlobalMemoryInstructionMask = InstructionMask::None;
    ShaderType m_ShaderType = ShaderType::UNKNOWN;

#if _DEBUG
    std::vector<ExplanationEntry> m_ExplanationEntries;

    ////////////////////////////////////////////////////////////////////////
    void RegisterRedundancyExplanation(const llvm::Instruction* pInst, ExplanationEntry::Cause cause);
#endif // _DEBUG
};

char SynchronizationObjectCoalescingAnalysis::ID = 0;

////////////////////////////////////////////////////////////////////////////
SynchronizationObjectCoalescingAnalysis::SynchronizationObjectCoalescingAnalysis() :
    llvm::FunctionPass(ID)
{
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescingAnalysis::runOnFunction(llvm::Function& F)
{
    const bool isModified = false; // this is only an analysis
    m_CurrentFunction = &F;
    const CodeGenContext* const ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    m_HasIndependentSharedMemoryFenceFunctionality = ctx->platform.hasSLMFence();
    m_ShaderType = ctx->type;
    Analyze();
    return isModified;
}

////////////////////////////////////////////////////////////////////////
/// @brief Processes an analysis which results in pointing out redundancies
/// among synchronization instructions appearing in the analyzed function.
void SynchronizationObjectCoalescingAnalysis::Analyze()
{
    if (IGC_IS_FLAG_ENABLED(DisableSynchronizationObjectCoalescingPass))
    {
        return;
    }

    InvalidateMembers();
    GatherInstructions();
    FindRedundancies();
}

////////////////////////////////////////////////////////////////////////
/// @brief Identifies explicit redundancies among synchronization instructions.
void SynchronizationObjectCoalescingAnalysis::FindRedundancies()
{
    // This lambda function verifies if there is an obligation to preserve a synchronization instruction for the particular resource.
    // This stems from the forwardly and backwardly visible memory instructions and their interaction with the synchronization
    // instruction.
    auto IdentifyObligation = [this](
        InstructionMask forwardMemoryInstructionMask,
        InstructionMask backwardMemoryInstructionMask,
        InstructionMask readBit,
        InstructionMask writeBit,
        SynchronizationCaseMask synchronizationCaseMask) -> bool
    {
        // write -> barrier/fence -> read
        bool isObligatory = ((synchronizationCaseMask & SynchronizationCaseMask::WriteSyncRead) !=0) && ((backwardMemoryInstructionMask & writeBit) != 0 && (forwardMemoryInstructionMask & readBit) != 0);
        // write -> barrier/fence -> write
        isObligatory |= ((synchronizationCaseMask & SynchronizationCaseMask::WriteSyncWrite) != 0) && ((backwardMemoryInstructionMask & writeBit) != 0 && (forwardMemoryInstructionMask & writeBit) != 0);
        // atomic -> barrier/fence -> read
        isObligatory |= ((synchronizationCaseMask & SynchronizationCaseMask::AtomicSyncRead) != 0) && ((backwardMemoryInstructionMask & AtomicOperation) != 0 && (forwardMemoryInstructionMask & readBit) != 0);
        // atomic -> barrier/fence -> write
        isObligatory |= ((synchronizationCaseMask & SynchronizationCaseMask::AtomicSyncWrite) != 0) && ((backwardMemoryInstructionMask & AtomicOperation) != 0 && (forwardMemoryInstructionMask & writeBit) != 0);
        // write -> barrier/fence -> atomic
        isObligatory |= ((synchronizationCaseMask & SynchronizationCaseMask::WriteSyncAtomic) != 0) && ((backwardMemoryInstructionMask & writeBit) != 0 && (forwardMemoryInstructionMask & AtomicOperation) != 0);
        // read -> barrier -> atomic
        isObligatory |= ((synchronizationCaseMask & SynchronizationCaseMask::ReadSyncAtomic) != 0) && ((backwardMemoryInstructionMask & readBit) != 0 && (forwardMemoryInstructionMask & AtomicOperation) != 0);
        // read -> barrier -> write
        isObligatory |= ((synchronizationCaseMask & SynchronizationCaseMask::ReadSyncWrite) != 0) && ((backwardMemoryInstructionMask & readBit) != 0 && (forwardMemoryInstructionMask & writeBit) != 0);

        return isObligatory;
    };

    // This lambda function identifies partial and strict redundancies among synchronization operations
    auto GatherRedundancies = [this, &IdentifyObligation](std::vector<llvm::Instruction*>& synchronizationOperations)
    {
        constexpr bool forwardDirection = true;
        constexpr bool backwardDirection = false;

        for (llvm::Instruction* pInst : synchronizationOperations)
        {
            if ((m_GlobalMemoryInstructionMask & GetDefaultWriteMemoryInstructionMask(pInst)) == 0)
            {
#if _DEBUG
                RegisterRedundancyExplanation(pInst, ExplanationEntry::FastStrictRedundancy);
#endif // _DEBUG
                m_RedundantInstructions.insert(pInst);
            }
            InstructionMask localForwardMemoryInstructionMask = InstructionMask::None;
            InstructionMask localBackwardMemoryInstructionMask = InstructionMask::None;

            auto SetLocalMemoryInstructionMask = [this, pInst, forwardDirection, backwardDirection,
                &localForwardMemoryInstructionMask, &localBackwardMemoryInstructionMask]()
            {
                localForwardMemoryInstructionMask = GetInstructionMask(pInst, forwardDirection);
                localBackwardMemoryInstructionMask = GetInstructionMask(pInst, backwardDirection);

                // This assures the below checks are right
                IGC_ASSERT((localForwardMemoryInstructionMask & GetDefaultMemoryInstructionMask(pInst)) == localForwardMemoryInstructionMask);
                IGC_ASSERT((localBackwardMemoryInstructionMask & GetDefaultMemoryInstructionMask(pInst)) == localBackwardMemoryInstructionMask);
            };
            SetLocalMemoryInstructionMask();

            auto GetStrictObligation = [this, pInst, forwardDirection, backwardDirection, &localForwardMemoryInstructionMask,
                &localBackwardMemoryInstructionMask, &IdentifyObligation](
                InstructionMask readBit,
                InstructionMask writeBit,
                bool isThreadGroupBarrierDependent = false) -> bool
            {
                SynchronizationCaseMask synchronizationCaseMask = sc_FullSynchronizationCaseMask;

                if (IsFenceOperation(pInst))
                {
                    // Note: Please change the description in igc flags if the value is changed.
                    static_assert(SynchronizationCaseMask::ReadSyncWrite == 0x01);
                    bool disableReadFenceWriteCase = (IGC_GET_FLAG_VALUE(SynchronizationObjectCoalescingConfig) & SynchronizationCaseMask::ReadSyncWrite) != 0;
                    disableReadFenceWriteCase |= !isThreadGroupBarrierDependent;
                    if (disableReadFenceWriteCase)
                    {
                        synchronizationCaseMask = static_cast<SynchronizationCaseMask>((~SynchronizationCaseMask::ReadSyncWrite) & synchronizationCaseMask);
                    }
                }

                bool isObligatory = IdentifyObligation(localForwardMemoryInstructionMask, localBackwardMemoryInstructionMask, readBit, writeBit, synchronizationCaseMask);
                return isObligatory;
            };

            auto GetL1CacheInvalidationObligation = [this, forwardDirection, backwardDirection, &localForwardMemoryInstructionMask,
                &localBackwardMemoryInstructionMask, &IdentifyObligation](
                    InstructionMask readBit,
                    InstructionMask writeBit) -> bool
            {
                constexpr SynchronizationCaseMask L1CacheInvalidationCaseMask = static_cast<SynchronizationCaseMask>(
                    SynchronizationCaseMask::WriteSyncRead |
                    SynchronizationCaseMask::WriteSyncAtomic |
                    SynchronizationCaseMask::AtomicSyncRead);
                bool isObligatory = IdentifyObligation(localForwardMemoryInstructionMask, localBackwardMemoryInstructionMask, readBit, writeBit, L1CacheInvalidationCaseMask);
                return isObligatory;
            };

            auto GatherPartialRedundancy = [this, &pInst, &SetLocalMemoryInstructionMask]
                (llvm::DenseSet<llvm::Instruction*>& redundantCollection)
            {
                redundantCollection.insert(pInst);
                SetLocalMemoryInstructionMask();
            };

            // identify partial redundancies for untyped memory fences
            if (IsUntypedMemoryFenceOperationForGlobalAccess(pInst))
            {
                if (IsUntypedMemoryFenceOperationWithInvalidationFunctionality(pInst))
                {
                    // buffer access
                    bool isObligatory = GetL1CacheInvalidationObligation(BufferReadOperation, BufferWriteOperation);
                    {
                        isObligatory |= GetL1CacheInvalidationObligation(TypedReadOperation, TypedWriteOperation);
                    }
                    {
                        isObligatory |= GetL1CacheInvalidationObligation(OutputUrbReadOperation, UrbWriteOperation);
                    }
                    if (!isObligatory)
                    {
#if _DEBUG
                        RegisterRedundancyExplanation(pInst, ExplanationEntry::L1CacheInvalidationRedundancy);
#endif // _DEBUG
                        GatherPartialRedundancy(m_InvalidationFunctionalityRedundancies);
                    }
                }

                if (!m_HasIndependentSharedMemoryFenceFunctionality)
                {
                    // buffer access
                    bool isObligatory = GetStrictObligation(BufferReadOperation, BufferWriteOperation);

                    if (!isObligatory)
                    {
#if _DEBUG
                        RegisterRedundancyExplanation(pInst, ExplanationEntry::GlobalMemoryRedundancy);
#endif // _DEBUG
                        GatherPartialRedundancy(m_GlobalMemoryRedundancies);
                        IGC_ASSERT(!IsUntypedMemoryFenceOperationWithInvalidationFunctionality(pInst));
                    }
                }
            }

            // Strict Redundancies
            /// It means removing such a set of instructions safely if none of such paths which meet at least one of the
            /// following order of visitation (not crossed by a substitutive synchronization):
            /// - any write instruction, this synchronization instruction, any read instruction,
            /// - any write instruction, this synchronization instruction, any write instruction,
            /// - any write instruction, this synchronization instruction, any atomic instruction,
            /// - any atomic instruction, this synchronization instruction, any read instruction,
            /// - any atomic instruction, this synchronization instruction, any write instruction,
            /// - (only for barriers or dependent fences) any read instruction, this synchronization instruction, any write instruction,
            /// - (only for barriers) any atomic instruction, this synchronization instruction, any atomic instruction.
            /// * in this context read and write instructions contributes to the same type of memory.

            // identify strict redundancies

            // atomic -> barrier -> atomic
            bool isObligatory = false;
            if (IsThreadBarrierOperation(pInst))
            {
                isObligatory |= (localForwardMemoryInstructionMask & localBackwardMemoryInstructionMask & AtomicOperation) != 0;
            }

            auto CheckIfObligatory = [this, GetStrictObligation](
                bool isThreadGroupBarrierDependent = false) {

                // buffer access
                bool isObligatory = GetStrictObligation(BufferReadOperation, BufferWriteOperation, isThreadGroupBarrierDependent);
                // shared memory access
                isObligatory |= GetStrictObligation(SharedMemoryReadOperation, SharedMemoryWriteOperation, isThreadGroupBarrierDependent);
                // typed access
                isObligatory |= GetStrictObligation(TypedReadOperation, TypedWriteOperation, isThreadGroupBarrierDependent);
                // URB access
                isObligatory |= GetStrictObligation(OutputUrbReadOperation, UrbWriteOperation, isThreadGroupBarrierDependent);

                return isObligatory;
            };

            isObligatory |= CheckIfObligatory();

            if (!isObligatory && !IsThreadBarrierOperation(pInst))
            {
                InstructionMask unsynchronizedForwardMemoryInstructionMask = GetUnsynchronizedForwardInstructionMask(pInst);
                localForwardMemoryInstructionMask = unsynchronizedForwardMemoryInstructionMask;
                isObligatory |= CheckIfObligatory(true /*isThreadGroupBarrierDependent*/);
            }

            if (!isObligatory)
            {
#if _DEBUG
                RegisterRedundancyExplanation(pInst, ExplanationEntry::StrictRedundancy);
#endif // _DEBUG
                m_RedundantInstructions.insert(pInst);
            }
        }
    };

    GatherRedundancies(m_ThreadGroupBarriers);
    GatherRedundancies(m_UntypedMemoryFences);
}

////////////////////////////////////////////////////////////////////////
/// @brief Provides write memory instructions mask which are synchronized
/// by the instruction.
IGC::SynchronizationObjectCoalescingAnalysis::InstructionMask SynchronizationObjectCoalescingAnalysis::GetDefaultWriteMemoryInstructionMask(const llvm::Instruction* pSourceInst) const
{
    InstructionMask result = InstructionMask::None;
    if (IsUntypedMemoryFenceOperation(pSourceInst))
    {
        const bool hasInvalidationFunctionality = IsUntypedMemoryFenceOperationWithInvalidationFunctionality(pSourceInst);
        const bool hasSharedMemoryInfluence = IsUntypedMemoryFenceOperationForSharedMemoryAccess(pSourceInst);
        const bool hasGlobalInfluence = IsUntypedMemoryFenceOperationForGlobalAccess(pSourceInst);
        if (hasSharedMemoryInfluence)
        {
            result = static_cast<InstructionMask>(
                result |
                SharedMemoryWriteOperation);
            if (m_ShaderType == ShaderType::HULL_SHADER &&
                m_HasIndependentSharedMemoryFenceFunctionality)
            {
                // This is for ICL+ but should not harm on pre-ICL
                result = static_cast<InstructionMask>(
                    result |
                    UrbWriteOperation);
            }
        }
        if (hasGlobalInfluence)
        {
            result = static_cast<InstructionMask>(
                result |
                BufferWriteOperation);
            {
                result = static_cast<InstructionMask>(
                    result |
                    TypedWriteOperation);
            }
            {
                // This is for ICL+ but should not harm on pre-ICL
                result = static_cast<InstructionMask>(
                    result |
                    UrbWriteOperation);
            }
        }
    }
    else if (IsThreadBarrierOperation(pSourceInst))
    {
        result = static_cast<InstructionMask>(sc_MemoryWriteInstructionMask);
    }
    else
    {
        IGC_ASSERT(0);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////
/// @brief Provides default memory instruction mask which is used for
/// graph searching.
IGC::SynchronizationObjectCoalescingAnalysis::InstructionMask SynchronizationObjectCoalescingAnalysis::GetDefaultMemoryInstructionMask(
    const llvm::Instruction* pSourceInst) const
{
    // All the rules stems from assumptions in the main comment of this analysis (the paragraph about a strict redundancy).
    InstructionMask result = InstructionMask::None;
    if (IsUntypedMemoryFenceOperation(pSourceInst))
    {
        const bool hasSharedMemoryInfluence = IsUntypedMemoryFenceOperationForSharedMemoryAccess(pSourceInst);
        const bool hasGlobalInfluence = IsUntypedMemoryFenceOperationForGlobalAccess(pSourceInst);
        result = static_cast<InstructionMask>(
            result |
            AtomicOperation);

        if (hasSharedMemoryInfluence)
        {
            result = static_cast<InstructionMask>(
                result |
                SharedMemoryWriteOperation |
                SharedMemoryReadOperation);
            if (m_ShaderType == ShaderType::HULL_SHADER &&
                m_HasIndependentSharedMemoryFenceFunctionality)
            {
                // This is for ICL+ but should not harm on pre-ICL
                result = static_cast<InstructionMask>(
                    result |
                    UrbWriteOperation |
                    OutputUrbReadOperation);
            }
        }

        if (hasGlobalInfluence)
        {
            result = static_cast<InstructionMask>(
                result |
                BufferWriteOperation |
                BufferReadOperation);
            {
                result = static_cast<InstructionMask>(
                    result |
                    TypedWriteOperation |
                    TypedReadOperation);
            }
            {
                // This is for ICL+ but should not harm on pre-ICL
                result = static_cast<InstructionMask>(
                    result |
                    UrbWriteOperation |
                    OutputUrbReadOperation);
            }
        }
    }
    else if (IsThreadBarrierOperation(pSourceInst))
    {
        result = static_cast<InstructionMask>(sc_MemoryReadInstructionMask | sc_MemoryWriteInstructionMask);
    }
    else
    {
        IGC_ASSERT(0);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////
/// @brief Go through a basic block according to the iterator direction
/// until any boundary instruction is met. Meanwhile, this function collects
/// instructions which fulfill the predicate.
/// @param begin the begin iterator
/// @param end the end iterator
/// @param gatheredInstructions the collection of gathered instructions
/// @param GatheringPredicate determines which instructions can be collected
/// @param boundaryInstructions the collection of boundary instructions
/// @param BoundaryPredicate determines which instructions are boundary
template<typename BasicBlockterator>
static BasicBlockterator GatherInstructionsInBasicBlock(
    BasicBlockterator begin,
    BasicBlockterator end,
    std::vector<const llvm::Instruction*>& gatheredInstructions,
    std::function<bool(const llvm::Instruction*)>& GatheringPredicate,
    std::vector<const llvm::Instruction*>& boundaryInstructions,
    std::function<bool(const llvm::Instruction*)>& BoundaryPredicate)
{
    for (auto it = begin; it != end; it++)
    {
        const llvm::Instruction* pInst = &(*it);

        if (BoundaryPredicate(pInst))
        {
            boundaryInstructions.push_back(pInst);
            return it;
        }

        if (GatheringPredicate(pInst))
        {
            gatheredInstructions.push_back(pInst);
        }
    }
    return end;
}

////////////////////////////////////////////////////////////////////////
/// @brief Seeks available instructions which can be collected according to
/// the iterator direction. The searching ends up with meeting a boundary instruction.
/// The searching relies on DFS algorithm.
/// @param workList a collection with begin points of searching
/// @param visitedBasicBlocks holds all restricted basic blocks
/// @param gatheredInstructions the collection of gathered instructions
/// @param GatheringPredicate determines which instructions can be collected
/// @param boundaryInstructions the collection of boundary instructions
/// @param BoundaryPredicate determines which instructions are boundary
template<typename BasicBlockterator>
static void SearchInstructions(
    std::list<BasicBlockterator>& workList,
    llvm::DenseSet<const llvm::BasicBlock*>& visitedBasicBlocks,
    std::vector<const llvm::Instruction*>& gatheredInstructions,
    std::function<bool(const llvm::Instruction*)>& GatheringPredicate,
    std::vector<const llvm::Instruction*>& boundaryInstructions,
    std::function<bool(const llvm::Instruction*)>& BoundaryPredicate)
{
    constexpr bool isForwardDirection = std::is_same_v<BasicBlockterator, llvm::BasicBlock::const_iterator>;
    // handles a forward direction
    if constexpr (isForwardDirection)
    {
        for (auto it : workList)
        {
            const llvm::BasicBlock* pCurrentBasicBlock = it->getParent();

            // use the iterator only if it wasn't visited or restricted
            if (visitedBasicBlocks.find(pCurrentBasicBlock) != visitedBasicBlocks.end() &&
                (*visitedBasicBlocks.find(pCurrentBasicBlock))->begin() == it)
            {
                continue;
            }

            if (pCurrentBasicBlock->begin() == it)
            {
                visitedBasicBlocks.insert(pCurrentBasicBlock);
            }

            auto end = pCurrentBasicBlock->end();
            it = GatherInstructionsInBasicBlock(
                it,
                pCurrentBasicBlock->end(),
                gatheredInstructions,
                GatheringPredicate,
                boundaryInstructions,
                BoundaryPredicate);

            if (it == end)
            {
                for (const llvm::BasicBlock* pSuccessor : llvm::successors(pCurrentBasicBlock))
                {
                    workList.push_back(pSuccessor->begin());
                }
            }
        }
    }
    // handles a backward direction
    else
    {
        for (auto it : workList)
        {
            const llvm::BasicBlock* pCurrentBasicBlock = it->getParent();

            // use the iterator only if it wasn't visited or restricted
            if (visitedBasicBlocks.find(pCurrentBasicBlock) != visitedBasicBlocks.end() &&
                (*visitedBasicBlocks.find(pCurrentBasicBlock))->rbegin() == it)
            {
                continue;
            }

            if (pCurrentBasicBlock->rbegin() == it)
            {
                visitedBasicBlocks.insert(pCurrentBasicBlock);
            }

            auto end = pCurrentBasicBlock->rend();
            it = GatherInstructionsInBasicBlock(
                it,
                pCurrentBasicBlock->rend(),
                gatheredInstructions,
                GatheringPredicate,
                boundaryInstructions,
                BoundaryPredicate);

            if (it == end)
            {
                for (const llvm::BasicBlock* pPredecessor : llvm::predecessors(pCurrentBasicBlock))
                {
                    workList.push_back(pPredecessor->rbegin());
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////
/// @brief Provides reachable memory instructions from this source instruction.
/// The boundary for this searching are drawn by visible substitutes.
/// The visibility term means such a reachability which is determined by
/// the fact if any path between this source instruction and a substitute
/// is not crossed by another substitute.
/// The searching relies on DFS algorithm.
/// @param pSourceInst the source synchronization instruction
/// @param forwardDirection the direction of searching
/// @param memoryInstructionMask determines which memory instruction can be collected
/// @param output the output collection of reachable memory instructions
void SynchronizationObjectCoalescingAnalysis::GetVisibleMemoryInstructions(
    const llvm::Instruction* pSourceInst,
    bool forwardDirection,
    std::vector<const llvm::Instruction*>& boundaryInstructions,
    std::vector<const llvm::Instruction*>& memoryInstructions) const
{
    InstructionMask memoryInstructionMask = GetDefaultMemoryInstructionMask(pSourceInst);
    llvm::DenseSet<llvm::Instruction*> possibleBoundaryInstructions(GetAllSubsituteInstructions(pSourceInst));
    llvm::DenseSet<const llvm::BasicBlock*> visitedBasicBlocks;

    std::function<bool(const llvm::Instruction*)> BoundaryPredicate = [this, possibleBoundaryInstructions](const llvm::Instruction* pInst) -> bool
    {
        bool isBoundary = possibleBoundaryInstructions.find(pInst) != possibleBoundaryInstructions.end();
        return isBoundary;
    };

    std::function<bool(const llvm::Instruction*)> GatheringPredicate = [this, memoryInstructionMask](const llvm::Instruction* pInst) -> bool
    {
        bool gatherInstruction = (GetInstructionMask(pInst) & memoryInstructionMask) != 0;
        return gatherInstruction;
    };

    if (forwardDirection)
    {
        llvm::BasicBlock::const_iterator firstIt = ++pSourceInst->getIterator();
        std::list<llvm::BasicBlock::const_iterator> workList{ firstIt };
        SearchInstructions(workList, visitedBasicBlocks, memoryInstructions, GatheringPredicate, boundaryInstructions, BoundaryPredicate);
    }
    else
    {
        llvm::BasicBlock::const_reverse_iterator firstIt = ++pSourceInst->getReverseIterator();
        std::list<llvm::BasicBlock::const_reverse_iterator> workList;
        if (firstIt != pSourceInst->getParent()->rend())
        {
            workList.push_back(firstIt);
        }
        else
        {
            for (const llvm::BasicBlock* pPredecessor : llvm::predecessors(pSourceInst->getParent()))
            {
                workList.push_back(pPredecessor->rbegin());
            }
        }
        SearchInstructions(workList, visitedBasicBlocks, memoryInstructions, GatheringPredicate, boundaryInstructions, BoundaryPredicate);
    }
}

////////////////////////////////////////////////////////////////////////
/// @brief Provides reachable memory instructions in the next synchronization area
/// from this source instruction. Such searching is executed because fences
/// ensures that all started memory operations are finished together.
/// This all potentially unsynchronized instructions should be known to make decision
/// if the fence is redundant. This function search such patterns: this fence -> (barrier ->
/// substitute* -> barrier*). All unsynchronized instructions are inside the paranthesis.
/// * these instructions can be missed in the shader.
/// @param pSourceInst a collection with begin points of searching
/// @param threadGroupBarriers holds boundary thread group barrier instructions
/// @param memoryInstructions the collection of unsynchronized memory instructions
void SynchronizationObjectCoalescingAnalysis::GetAllUnsynchronizedMemoryInstructions(
    const llvm::Instruction* pSourceInst,
    std::vector<const llvm::Instruction*>& threadGroupBarriers,
    std::vector<const llvm::Instruction*>& memoryInstructions) const
{
    const llvm::DenseSet<llvm::Instruction*> possibleBoundaryInstructions(GetAllSubsituteInstructions(pSourceInst));

    std::function<bool(const llvm::Instruction*)> BoundaryPredicateFromSubstitutes = [this, &possibleBoundaryInstructions](const llvm::Instruction* pInst) -> bool
    {
        bool isBoundary = possibleBoundaryInstructions.find(pInst) != possibleBoundaryInstructions.end();
        return isBoundary;
    };

    std::function<bool(const llvm::Instruction*)> CheckIfBarrier = [this](const llvm::Instruction* pInst) -> bool
    {
        bool isThreadGroupBarrier = IsThreadBarrierOperation(pInst);
        return isThreadGroupBarrier;
    };

    // gather all visible thread group barriers
    {
        llvm::DenseSet<const llvm::BasicBlock*> visitedBasicBlocks;
        llvm::BasicBlock::const_iterator firstIt = ++pSourceInst->getIterator();
        std::list<llvm::BasicBlock::const_iterator> workList{ firstIt };
        std::vector<const llvm::Instruction*> boundaryInstructions;
        SearchInstructions(workList, visitedBasicBlocks, threadGroupBarriers, CheckIfBarrier, boundaryInstructions, BoundaryPredicateFromSubstitutes);
    }

    // gather all memory instructions between thread group barriers which goes through a substitute (thread group barrier -> fence -> thread group barrier)
    if (!threadGroupBarriers.empty())
    {
        InstructionMask memoryInstructionMask = GetDefaultMemoryInstructionMask(pSourceInst);
        std::function<bool(const llvm::Instruction*)> GatheringPredicate = [this, memoryInstructionMask](const llvm::Instruction* pInst) -> bool
        {
            bool gatherInstruction = (GetInstructionMask(pInst) & memoryInstructionMask) != 0;
            return gatherInstruction;
        };
        llvm::DenseSet<const llvm::BasicBlock*> visitedBasicBlocks;

        // find reachable substitutes from these thread group barriers
        std::list<llvm::BasicBlock::const_iterator> workList;
        for (const llvm::Instruction* pBarrier : threadGroupBarriers)
        {
            workList.push_back(pBarrier->getIterator());
        }
        std::vector<const llvm::Instruction*> visibleSubstitues;
        SearchInstructions(workList, visitedBasicBlocks, memoryInstructions, GatheringPredicate, visibleSubstitues, BoundaryPredicateFromSubstitutes);

        // find reachable thread group barriers from these substitutes
        workList.clear();
        for (const llvm::Instruction* pSubstitute : visibleSubstitues)
        {
            workList.push_back(pSubstitute->getIterator());
            if (visitedBasicBlocks.find(pSubstitute->getParent()) != visitedBasicBlocks.end() &&
                (*visitedBasicBlocks.find(pSubstitute->getParent()))->begin() == pSubstitute->getIterator())
            {
                visitedBasicBlocks.erase(pSubstitute->getParent());
            }
        }
        SearchInstructions(workList, visitedBasicBlocks, memoryInstructions, GatheringPredicate, threadGroupBarriers, CheckIfBarrier);
    }
}

////////////////////////////////////////////////////////////////////////
/// @brief Provides the memory instruction mask from reachable memory instructions
/// of this source instruction. The boundary for this searching are drawn
/// by visible substitutes. The visibility term means such a reachability which
/// is determined by the fact if any path between this source instruction and
/// a substitute is not crossed by another substitute.
/// @param pSourceInst the source synchronization instruction
/// @param forwardDirection the direction of searching
SynchronizationObjectCoalescingAnalysis::InstructionMask SynchronizationObjectCoalescingAnalysis::GetInstructionMask(
    const llvm::Instruction* pSourceInst,
    bool forwardDirection) const
{
    std::vector<const llvm::Instruction*> boundaryInstructions;
    std::vector<const llvm::Instruction*> memoryInstructions;
    GetVisibleMemoryInstructions(pSourceInst, forwardDirection, boundaryInstructions, memoryInstructions);
    return GetInstructionMask(memoryInstructions);
}

////////////////////////////////////////////////////////////////////////
/// @brief Provides the memory instruction mask from unsynchronized forward
/// memory instructions (in the next synchronization block delineated by
/// thread group barriers)
/// @param pSourceInst the source synchronization instruction
IGC::SynchronizationObjectCoalescingAnalysis::InstructionMask SynchronizationObjectCoalescingAnalysis::GetUnsynchronizedForwardInstructionMask(
    const llvm::Instruction* pSourceInst) const
{
    std::vector<const llvm::Instruction*> boundaryInstructions;
    std::vector<const llvm::Instruction*> memoryInstructions;
    GetAllUnsynchronizedMemoryInstructions(pSourceInst, boundaryInstructions, memoryInstructions);
    return GetInstructionMask(memoryInstructions);
}

////////////////////////////////////////////////////////////////////////
/// @brief Provides the memory instruction mask from reachable memory instructions
/// of this source instruction. The boundary for this searching are drawn
/// by visible substitutes. The visibility term means such a reachability which
/// is determined by the fact if any path between this source instruction and
/// a substitute is not crossed by another substitute.
/// @param pSourceInst the source synchronization instruction
/// @param forwardDirection the direction of searching
SynchronizationObjectCoalescingAnalysis::InstructionMask SynchronizationObjectCoalescingAnalysis::GetInstructionMask(
    const std::vector<const llvm::Instruction*>& input) const
{
    InstructionMask result = InstructionMask::None;
    for (const llvm::Instruction* pInst : input)
    {
        result = static_cast<InstructionMask>(result | GetInstructionMask(pInst));
    }
    return result;
}

////////////////////////////////////////////////////////////////////////
llvm::DenseSet<llvm::Instruction*> SynchronizationObjectCoalescingAnalysis::GetAllSubsituteInstructions(
    const llvm::Instruction* pReferenceInst) const
{
    llvm::DenseSet<llvm::Instruction*> result;
    auto FillWithSubstitues = [&result, pReferenceInst, this](const std::vector<llvm::Instruction*>& instructions)
    {
        for (llvm::Instruction* pEvaluatedInst : instructions)
        {
            if (m_RedundantInstructions.find(pEvaluatedInst) == m_RedundantInstructions.end() &&
                IsSubsituteInstruction(pEvaluatedInst, pReferenceInst))
            {
                result.insert(pEvaluatedInst);
            }
        }
    };
    if (IsUntypedMemoryFenceOperation(pReferenceInst))
    {
        FillWithSubstitues(m_UntypedMemoryFences);
    }
    else if (IsThreadBarrierOperation(pReferenceInst))
    {
        FillWithSubstitues(m_ThreadGroupBarriers);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////
/// @brief Checks if the reference instruction can be replaced by the evaluated instruction.
/// @param pEvaluatedInst the instruction which is evaluated if it can replace the reference one
/// @param pReferenceInst represents the reference instruction which is an object of the replacement
/// and it means that this instruction must be equal or weaker than the evaluated one.
bool SynchronizationObjectCoalescingAnalysis::IsSubsituteInstruction(
    const llvm::Instruction* pEvaluatedInst,
    const llvm::Instruction* pReferenceInst) const
{
    if (IsUntypedMemoryFenceOperation(pEvaluatedInst) &&
        IsUntypedMemoryFenceOperation(pReferenceInst))
    {
        const uint32_t commitEnableArg = 0;
        const uint32_t L3FlushRWDataArg = 1;
        const uint32_t L3FlushConstantDataArg = 2;
        const uint32_t L3FlushTextureDataArg = 3;
        const uint32_t L3FlushInstructionsArg = 4;
        bool isDuplicate = pEvaluatedInst->getOperand(commitEnableArg) ==
            pReferenceInst->getOperand(commitEnableArg);
        isDuplicate &= pEvaluatedInst->getOperand(L3FlushRWDataArg) ==
            pReferenceInst->getOperand(L3FlushRWDataArg);
        isDuplicate &= pEvaluatedInst->getOperand(L3FlushConstantDataArg) ==
            pReferenceInst->getOperand(L3FlushConstantDataArg);
        isDuplicate &= pEvaluatedInst->getOperand(L3FlushTextureDataArg) ==
            pReferenceInst->getOperand(L3FlushTextureDataArg);
        isDuplicate &= pEvaluatedInst->getOperand(L3FlushInstructionsArg) ==
            pReferenceInst->getOperand(L3FlushInstructionsArg);

        isDuplicate &= ((IsUntypedMemoryFenceOperationForGlobalAccess(pEvaluatedInst) && IsUntypedMemoryFenceOperationForGlobalAccess(pReferenceInst)) ||
            (m_HasIndependentSharedMemoryFenceFunctionality && (IsUntypedMemoryFenceOperationForSharedMemoryAccess(pEvaluatedInst) && IsUntypedMemoryFenceOperationForSharedMemoryAccess(pReferenceInst)))) ||
            (!m_HasIndependentSharedMemoryFenceFunctionality && (IsUntypedMemoryFenceOperationForGlobalAccess(pEvaluatedInst) || !IsUntypedMemoryFenceOperationForGlobalAccess(pReferenceInst)));

        if (IsUntypedMemoryFenceOperationForGlobalAccess(pReferenceInst))
        {
            isDuplicate &= (IsUntypedMemoryFenceOperationWithInvalidationFunctionality(pEvaluatedInst) ||
                !IsUntypedMemoryFenceOperationWithInvalidationFunctionality(pReferenceInst));
        }

        return isDuplicate;
    }
    else if (IsThreadBarrierOperation(pEvaluatedInst) &&
        IsThreadBarrierOperation(pReferenceInst))
    {
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////
const llvm::DenseSet<llvm::Instruction*>& SynchronizationObjectCoalescingAnalysis::GetRedundantInstructions() const
{
    return m_RedundantInstructions;
}

////////////////////////////////////////////////////////////////////////
const llvm::DenseSet<llvm::Instruction*>& SynchronizationObjectCoalescingAnalysis::GetInvalidationFunctionalityRedundancies() const
{
    return m_InvalidationFunctionalityRedundancies;
}

////////////////////////////////////////////////////////////////////////
const llvm::DenseSet<llvm::Instruction*>& SynchronizationObjectCoalescingAnalysis::GetGlobalMemoryRedundancies() const
{
    return m_GlobalMemoryRedundancies;
}

////////////////////////////////////////////////////////////////////////
void SynchronizationObjectCoalescingAnalysis::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    AU.setPreservesAll();
    AU.addRequired<CodeGenContextWrapper>();
}

////////////////////////////////////////////////////////////////////////
/// @brief Gathers synchronization instructions from the current
/// function.
void SynchronizationObjectCoalescingAnalysis::GatherInstructions()
{
    for (llvm::BasicBlock& basicBlock : *m_CurrentFunction)
    {
        for (llvm::Instruction& inst : basicBlock)
        {
            if (IsThreadBarrierOperation(&inst))
            {
                m_ThreadGroupBarriers.push_back(&inst);
            }
            else if (IsUntypedMemoryFenceOperation(&inst))
            {
                m_UntypedMemoryFences.push_back(&inst);
            }
            else if (InstructionMask memoryInstructionMask = GetInstructionMask(&inst);
                memoryInstructionMask != InstructionMask::None)
            {
                m_GlobalMemoryInstructionMask = static_cast<InstructionMask>(m_GlobalMemoryInstructionMask | memoryInstructionMask);
                if (memoryInstructionMask == InstructionMask::AtomicOperation)
                {
                    llvm::GenIntrinsicInst* pGenIntrinsicInst = llvm::cast<llvm::GenIntrinsicInst>(&inst);
                    if (pGenIntrinsicInst->getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_intatomictyped ||
                        pGenIntrinsicInst->getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_icmpxchgatomictyped)
                    {
                        m_GlobalMemoryInstructionMask = static_cast<InstructionMask>(m_GlobalMemoryInstructionMask | InstructionMask::TypedReadOperation);
                        m_GlobalMemoryInstructionMask = static_cast<InstructionMask>(m_GlobalMemoryInstructionMask | InstructionMask::TypedWriteOperation);
                    }
                    else
                    {
                        llvm::Type* pPointerType = inst.getOperand(0)->getType();
                        if (IsGlobalResource(pPointerType))
                        {
                            m_GlobalMemoryInstructionMask = static_cast<InstructionMask>(m_GlobalMemoryInstructionMask | InstructionMask::BufferReadOperation);
                            m_GlobalMemoryInstructionMask = static_cast<InstructionMask>(m_GlobalMemoryInstructionMask | InstructionMask::BufferWriteOperation);
                        }
                        else if (IsSharedMemoryResource(pPointerType))
                        {
                            m_GlobalMemoryInstructionMask = static_cast<InstructionMask>(m_GlobalMemoryInstructionMask | InstructionMask::SharedMemoryReadOperation);
                            m_GlobalMemoryInstructionMask = static_cast<InstructionMask>(m_GlobalMemoryInstructionMask | InstructionMask::SharedMemoryWriteOperation);
                        }
                    }
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////
void SynchronizationObjectCoalescingAnalysis::InvalidateMembers()
{
    m_UntypedMemoryFences.clear();
    m_ThreadGroupBarriers.clear();

    m_RedundantInstructions.clear();
    m_InvalidationFunctionalityRedundancies.clear();
    m_GlobalMemoryRedundancies.clear();
#if _DEBUG
    m_ExplanationEntries.clear();
#endif // _DEBUG
    m_GlobalMemoryInstructionMask = InstructionMask::None;
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescingAnalysis::IsSyncInstruction(const llvm::Instruction* pInst)
{
    return IsThreadBarrierOperation(pInst) ||
        IsUntypedMemoryFenceOperation(pInst);
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescingAnalysis::IsMemoryInstruction(const llvm::Instruction* pInst)
{
    return IsReadMemoryInstruction(pInst) ||
        IsWriteMemoryInstruction(pInst);
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescingAnalysis::IsReadMemoryInstruction(const llvm::Instruction* pInst)
{
    return IsAtomicOperation(pInst) ||
        IsBufferReadOperation(pInst) ||
        IsSharedMemoryReadOperation(pInst) ||
        IsTypedReadOperation(pInst) ||
        IsOutputUrbReadOperation(pInst);
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescingAnalysis::IsWriteMemoryInstruction(const llvm::Instruction* pInst)
{
    return IsAtomicOperation(pInst) ||
        IsBufferWriteOperation(pInst) ||
        IsSharedMemoryWriteOperation(pInst) ||
        IsTypedWriteOperation(pInst) ||
        IsUrbWriteOperation(pInst);
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescingAnalysis::IsAtomicOperation(const llvm::Instruction* pInst)
{
    if (llvm::isa<llvm::GenIntrinsicInst>(pInst))
    {
        const llvm::GenIntrinsicInst* pGenIntrinsicInst = llvm::cast<llvm::GenIntrinsicInst>(pInst);

        switch (pGenIntrinsicInst->getIntrinsicID())
        {
        case llvm::GenISAIntrinsic::GenISA_atomiccounterinc:
        case llvm::GenISAIntrinsic::GenISA_atomiccounterpredec:
        case llvm::GenISAIntrinsic::GenISA_icmpxchgatomicraw:
        case llvm::GenISAIntrinsic::GenISA_icmpxchgatomicrawA64:
        case llvm::GenISAIntrinsic::GenISA_cmpxchgatomicstructured:
        case llvm::GenISAIntrinsic::GenISA_icmpxchgatomictyped:
        case llvm::GenISAIntrinsic::GenISA_intatomicraw:
        case llvm::GenISAIntrinsic::GenISA_intatomicrawA64:
        case llvm::GenISAIntrinsic::GenISA_dwordatomicstructured:
        case llvm::GenISAIntrinsic::GenISA_intatomictyped:
        case llvm::GenISAIntrinsic::GenISA_fcmpxchgatomicraw:
        case llvm::GenISAIntrinsic::GenISA_fcmpxchgatomicrawA64:
        case llvm::GenISAIntrinsic::GenISA_fcmpxchgatomicstructured:
        case llvm::GenISAIntrinsic::GenISA_floatatomicraw:
        case llvm::GenISAIntrinsic::GenISA_floatatomicrawA64:
        case llvm::GenISAIntrinsic::GenISA_floatatomicstructured:
            return true;
        default:
            break;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescingAnalysis::IsTypedReadOperation(const llvm::Instruction* pInst)
{
    if (llvm::isa<llvm::GenIntrinsicInst>(pInst))
    {
        const llvm::GenIntrinsicInst* pGenIntrinsicInst = llvm::cast<llvm::GenIntrinsicInst>(pInst);

        switch (pGenIntrinsicInst->getIntrinsicID())
        {
        case llvm::GenISAIntrinsic::GenISA_typedread:
            return true;
        default:
            break;
        }
    }

    return false;
}

bool SynchronizationObjectCoalescingAnalysis::IsTypedWriteOperation(const llvm::Instruction* pInst)
{
    if (llvm::isa<llvm::GenIntrinsicInst>(pInst))
    {
        const llvm::GenIntrinsicInst* pGenIntrinsicInst = llvm::cast<llvm::GenIntrinsicInst>(pInst);

        switch (pGenIntrinsicInst->getIntrinsicID())
        {
        case llvm::GenISAIntrinsic::GenISA_typedwrite:
            return true;
        default:
            break;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescingAnalysis::IsOutputUrbReadOperation(const llvm::Instruction* pInst)
{
    if (llvm::isa<llvm::GenIntrinsicInst>(pInst))
    {
        const llvm::GenIntrinsicInst* pGenIntrinsicInst = llvm::cast<llvm::GenIntrinsicInst>(pInst);

        switch (pGenIntrinsicInst->getIntrinsicID())
        {
        case llvm::GenISAIntrinsic::GenISA_DCL_HSOutputCntrlPtInputVec:
        case llvm::GenISAIntrinsic::GenISA_DCL_HSPatchConstInputVec:
            return true;
        default:
            break;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescingAnalysis::IsUrbWriteOperation(const llvm::Instruction* pInst)
{
    if (llvm::isa<llvm::GenIntrinsicInst>(pInst))
    {
        const llvm::GenIntrinsicInst* pGenIntrinsicInst = llvm::cast<llvm::GenIntrinsicInst>(pInst);

        switch (pGenIntrinsicInst->getIntrinsicID())
        {
        case llvm::GenISAIntrinsic::GenISA_OUTPUT:
        case llvm::GenISAIntrinsic::GenISA_OutputTessControlPoint:
        case llvm::GenISAIntrinsic::GenISA_PatchConstantOutput:
            return true;
        default:
            break;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescingAnalysis::IsBufferReadOperation(const llvm::Instruction* pInst)
{
    if (llvm::isa<llvm::GenIntrinsicInst>(pInst))
    {
        const llvm::GenIntrinsicInst* pGenIntrinsicInst = llvm::cast<llvm::GenIntrinsicInst>(pInst);

        switch (pGenIntrinsicInst->getIntrinsicID())
        {
        case llvm::GenISAIntrinsic::GenISA_ldraw_indexed:
        case llvm::GenISAIntrinsic::GenISA_ldrawvector_indexed:
            return IsGlobalResource(pGenIntrinsicInst->getOperand(0)->getType());
        default:
            break;
        }
    }
    else if (llvm::isa<llvm::LoadInst>(pInst))
    {
        return IsGlobalResource(llvm::cast<llvm::LoadInst>(pInst)->getPointerOperandType());
    }

    return false;
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescingAnalysis::IsBufferWriteOperation(const llvm::Instruction* pInst)
{
    if (llvm::isa<llvm::GenIntrinsicInst>(pInst))
    {
        const llvm::GenIntrinsicInst* pGenIntrinsicInst = llvm::cast<llvm::GenIntrinsicInst>(pInst);

        switch (pGenIntrinsicInst->getIntrinsicID())
        {
        case llvm::GenISAIntrinsic::GenISA_storeraw_indexed:
        case llvm::GenISAIntrinsic::GenISA_storerawvector_indexed:
            return IsGlobalResource(pGenIntrinsicInst->getOperand(0)->getType());
        default:
            break;
        }
    }
    else if (llvm::isa<llvm::StoreInst>(pInst))
    {
        return IsGlobalResource(llvm::cast<llvm::StoreInst>(pInst)->getPointerOperandType());
    }

    return false;
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescingAnalysis::IsSharedMemoryReadOperation(const llvm::Instruction* pInst)
{
    if (llvm::isa<llvm::LoadInst>(pInst))
    {
        return IsSharedMemoryResource(llvm::cast<llvm::LoadInst>(pInst)->getPointerOperandType());
    }

    return false;
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescingAnalysis::IsSharedMemoryWriteOperation(const llvm::Instruction* pInst)
{
    if (llvm::isa<llvm::StoreInst>(pInst))
    {
        return IsSharedMemoryResource(llvm::cast<llvm::StoreInst>(pInst)->getPointerOperandType());
    }

    return false;
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescingAnalysis::IsThreadBarrierOperation(const llvm::Instruction* pInst)
{
    if (llvm::isa<llvm::GenIntrinsicInst>(pInst))
    {
        const llvm::GenIntrinsicInst* pGenIntrinsicInst = llvm::cast<llvm::GenIntrinsicInst>(pInst);

        switch (pGenIntrinsicInst->getIntrinsicID())
        {
        case llvm::GenISAIntrinsic::GenISA_threadgroupbarrier:
            return true;
        default:
            break;
        }
    }

    return false;
}
////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescingAnalysis::IsUntypedMemoryFenceOperation(const llvm::Instruction* pInst)
{
    if (llvm::isa<llvm::GenIntrinsicInst>(pInst))
    {
        const llvm::GenIntrinsicInst* pGenIntrinsicInst = llvm::cast<llvm::GenIntrinsicInst>(pInst);

        switch (pGenIntrinsicInst->getIntrinsicID())
        {
        case llvm::GenISAIntrinsic::GenISA_memoryfence:
            return true;
        default:
            break;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescingAnalysis::IsUntypedMemoryFenceOperationWithInvalidationFunctionality(const llvm::Instruction* pInst) const
{
    constexpr uint32_t L1CacheInvalidateArg = 6;
    return IsUntypedMemoryFenceOperation(pInst) &&
        llvm::cast<llvm::ConstantInt>(pInst->getOperand(L1CacheInvalidateArg))->getValue().getBoolValue() &&
        m_InvalidationFunctionalityRedundancies.find(pInst) == m_InvalidationFunctionalityRedundancies.end();
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescingAnalysis::IsUntypedMemoryFenceOperationForSharedMemoryAccess(const llvm::Instruction* pInst) const
{
    constexpr uint32_t globalMemFenceArg = 5;
    return IsUntypedMemoryFenceOperation(pInst) &&
        (!m_HasIndependentSharedMemoryFenceFunctionality || llvm::cast<llvm::ConstantInt>(pInst->getOperand(globalMemFenceArg))->getValue().getBoolValue() == false);
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescingAnalysis::IsUntypedMemoryFenceOperationForGlobalAccess(const llvm::Instruction* pInst) const
{
    constexpr uint32_t globalMemFenceArg = 5;
    return IsUntypedMemoryFenceOperation(pInst) &&
        llvm::cast<llvm::ConstantInt>(pInst->getOperand(globalMemFenceArg))->getValue().getBoolValue() &&
        m_GlobalMemoryRedundancies.find(pInst) == m_GlobalMemoryRedundancies.end();
}

////////////////////////////////////////////////////////////////////////
SynchronizationObjectCoalescingAnalysis::InstructionMask SynchronizationObjectCoalescingAnalysis::GetInstructionMask(const llvm::Instruction* pInst) const
{
    if (IsAtomicOperation(pInst))
    {
        return InstructionMask::AtomicOperation;
    }
    else if (IsTypedReadOperation(pInst))
    {
        return InstructionMask::TypedReadOperation;
    }
    else if (IsTypedWriteOperation(pInst))
    {
        return InstructionMask::TypedWriteOperation;
    }
    else if (IsOutputUrbReadOperation(pInst))
    {
        return InstructionMask::OutputUrbReadOperation;
    }
    else if (IsUrbWriteOperation(pInst))
    {
        return InstructionMask::UrbWriteOperation;
    }
    else if (IsBufferReadOperation(pInst))
    {
        return InstructionMask::BufferReadOperation;
    }
    else if (IsBufferWriteOperation(pInst))
    {
        return InstructionMask::BufferWriteOperation;
    }
    else if (IsSharedMemoryReadOperation(pInst))
    {
        return InstructionMask::SharedMemoryReadOperation;
    }
    else if (IsSharedMemoryWriteOperation(pInst))
    {
        return InstructionMask::SharedMemoryWriteOperation;
    }

    return InstructionMask::None;
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescingAnalysis::IsFenceOperation(const llvm::Instruction* pInst)
{
    return
        IsUntypedMemoryFenceOperation(pInst);
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescingAnalysis::IsGlobalResource(llvm::Type* pResourePointerType)
{
    uint as = pResourePointerType->getPointerAddressSpace();
    switch (as)
    {
    case ADDRESS_SPACE_LOCAL:
    case ADDRESS_SPACE_GENERIC:
        return false;
    case ADDRESS_SPACE_GLOBAL:
        return true;
    default:
    {
        BufferType bufferType = DecodeBufferType(as);
        switch (bufferType)
        {
        case IGC::UAV:
        case IGC::BINDLESS:
        case IGC::STATELESS:
        case IGC::SSH_BINDLESS:
            return true;
        case IGC::CONSTANT_BUFFER:
        case IGC::RESOURCE:
        case IGC::SLM:
        case IGC::POINTER:
        case IGC::BINDLESS_CONSTANT_BUFFER:
        case IGC::BINDLESS_TEXTURE:
        case IGC::SAMPLER:
        case IGC::BINDLESS_SAMPLER:
        case IGC::RENDER_TARGET:
        case IGC::STATELESS_READONLY:
        case IGC::STATELESS_A32:
        case IGC::SSH_BINDLESS_CONSTANT_BUFFER:
        case IGC::SSH_BINDLESS_TEXTURE:
        case IGC::BUFFER_TYPE_UNKNOWN:
            return false;
        }
    }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescingAnalysis::IsSharedMemoryResource(llvm::Type* pResourePointerType)
{
    uint as = pResourePointerType->getPointerAddressSpace();
    switch (as)
    {
    case ADDRESS_SPACE_LOCAL:
        return true;
    default:
        return false;
    }
}

#if _DEBUG
////////////////////////////////////////////////////////////////////////
/// @brief Registers information which allows understanding the decision
/// about identifying redundancy for this synchronization instruction.
void SynchronizationObjectCoalescingAnalysis::RegisterRedundancyExplanation(const llvm::Instruction* pInst, ExplanationEntry::Cause cause)
{
    constexpr bool forwardDirection = true;
    constexpr bool backwardDirection = false;

    auto GetIndex = [](const llvm::Instruction* pInst)
    {
        uint32_t i = 0;
        for (auto it = pInst->getParent()->begin(); it != pInst->getIterator(); it++, i++) {}
        return i;
    };

    auto GetBoundaryInst = [this, GetIndex](const llvm::Instruction* pInst) -> ExplanationEntry::SyncInstDescription
    {
        return {
            pInst->getParent(),
            GetIndex(pInst),
            m_InvalidationFunctionalityRedundancies.find(pInst) != m_InvalidationFunctionalityRedundancies.end(),
            m_GlobalMemoryRedundancies.find(pInst) != m_GlobalMemoryRedundancies.end()
        };
    };

    m_ExplanationEntries.emplace_back();
    ExplanationEntry& explanationEntry = m_ExplanationEntries.back();
    explanationEntry.m_SynchronizationDescription = GetBoundaryInst(pInst);
    explanationEntry.m_Cause = cause;

    if (explanationEntry.m_Cause == ExplanationEntry::Cause::FastStrictRedundancy)
    {
        return;
    }

    std::vector<const llvm::Instruction*> forwardBoundaryInstructions;
    std::vector<const llvm::Instruction*> backwardBoundaryInstructions;
    std::vector<const llvm::Instruction*> threadGroupBarriersBoundaries;
    GetVisibleMemoryInstructions(pInst, forwardDirection, forwardBoundaryInstructions, explanationEntry.m_ForwardMemoryInstructions);
    GetVisibleMemoryInstructions(pInst, backwardDirection, backwardBoundaryInstructions, explanationEntry.m_BackwardMemoryInstructions);
    if (!IsThreadBarrierOperation(pInst) && cause == ExplanationEntry::Cause::StrictRedundancy)
    {
        GetAllUnsynchronizedMemoryInstructions(pInst, threadGroupBarriersBoundaries, explanationEntry.m_ForwardMemoryInstructions);
    }

    for (const llvm::Instruction* pSubstituteInst : forwardBoundaryInstructions)
    {
        explanationEntry.m_ForwardBoundaries.push_back(GetBoundaryInst(pSubstituteInst));
    }

    for (const llvm::Instruction* pSubstituteInst : backwardBoundaryInstructions)
    {
        explanationEntry.m_BackwardBoundaries.push_back(GetBoundaryInst(pSubstituteInst));
    }

    for (const llvm::Instruction* pThreadGroupBarrier : threadGroupBarriersBoundaries)
    {
        explanationEntry.m_ThreadGroupBarriersBoundaries.push_back(GetBoundaryInst(pThreadGroupBarrier));
    }
}

////////////////////////////////////////////////////////////////////////
/// @brief Print explanation for all redundancies.
void SynchronizationObjectCoalescingAnalysis::print(llvm::raw_ostream& stream, bool onlyMemoryInstructionMask/* = true*/) const
{
    auto GetRedundancyCauseName = [](ExplanationEntry::Cause cause)
    {
        switch (cause)
        {
        case ExplanationEntry::GlobalMemoryRedundancy:
            return "global memory";
        case ExplanationEntry::L1CacheInvalidationRedundancy:
            return "L1 cache invalidation";
        case ExplanationEntry::StrictRedundancy:
            return "strict";
        case ExplanationEntry::FastStrictRedundancy:
            return "fast strict";
        default:
            return "unknown";
        }
    };

    auto GetSyncInstName = [this, GetRedundancyCauseName](const ExplanationEntry::SyncInstDescription& syncInstDesc)
    {
        std::stringstream syncDescName;
        syncDescName << std::get<0>(syncInstDesc)->getName().str() << ", "
            << std::get<1>(syncInstDesc);
        if (std::get<2>(syncInstDesc))
        {
            syncDescName << ", " << GetRedundancyCauseName(ExplanationEntry::L1CacheInvalidationRedundancy);
        }
        if (std::get<2>(syncInstDesc))
        {
            syncDescName << ", " << GetRedundancyCauseName(ExplanationEntry::GlobalMemoryRedundancy);
        }
        return syncDescName.str();
    };

    auto GetInstructionMaskBitName = [this](InstructionMask bit)
    {
        switch (bit)
        {
        case InstructionMask::None:
            return "None";
        case InstructionMask::AtomicOperation:
            return "AtomicOperation";
        case InstructionMask::TypedReadOperation:
            return "TypedReadOperation";
        case InstructionMask::TypedWriteOperation:
            return "TypedWriteOperation";
        case InstructionMask::OutputUrbReadOperation:
            return "OutputUrbReadOperation";
        case InstructionMask::UrbWriteOperation:
            return "UrbWriteOperation";
        case InstructionMask::BufferReadOperation:
            return "BufferReadOperation";
        case InstructionMask::BufferWriteOperation:
            return "BufferWriteOperation";
        case InstructionMask::SharedMemoryReadOperation:
            return "SharedMemoryReadOperation";
        case InstructionMask::SharedMemoryWriteOperation:
            return "SharedMemoryWriteOperation";
        default:
            return "Unknown";
        }
    };

    auto GetInstructionMaskName = [this, GetInstructionMaskBitName](InstructionMask mask)
    {
        std::stringstream name;
        if (mask != 0)
        {
            bool appendSeparator = false;
            for (uint32_t bit = 1; bit <= InstructionMask::SharedMemoryWriteOperation; bit <<= 1)
            {
                if ((bit & mask) != 0)
                {
                    if (appendSeparator)
                    {
                        name << " | ";
                    }
                    name << GetInstructionMaskBitName(static_cast<InstructionMask>(bit));
                    appendSeparator = true;
                }
            }
        }
        else
        {
            name << GetInstructionMaskBitName(InstructionMask::None);
        }
        return name.str();
    };


    auto GetInstructionDescription = [this, GetInstructionMaskBitName](const llvm::Instruction* pInst)
    {
        constexpr bool isForDebug = true;
        std::string result;
        llvm::raw_string_ostream stream(result);
        pInst->print(stream, isForDebug);
        stream << " (" << GetInstructionMaskBitName(GetInstructionMask(pInst)) << ")";
        stream.flush();
        return result;
    };

    uint32_t index = 0;
    std::string indentation = "  ";

    for (const ExplanationEntry& explanationEntry : m_ExplanationEntries)
    {
        stream << index << ". A " << GetRedundancyCauseName(explanationEntry.m_Cause) << " redundancy." << "\n";
        stream << indentation  << GetSyncInstName(explanationEntry.m_SynchronizationDescription) << "\n";
        if (explanationEntry.m_Cause == ExplanationEntry::Cause::FastStrictRedundancy)
        {
            stream << indentation << "The global instruction mask: " << GetInstructionMaskName(m_GlobalMemoryInstructionMask) << "\n";
            stream << "\n";
            continue;
        }
        stream << indentation << "The forward instruction mask: " << GetInstructionMaskName(GetInstructionMask(explanationEntry.m_ForwardMemoryInstructions)) << "\n";
        stream << indentation << "The backward instruction mask: " << GetInstructionMaskName(GetInstructionMask(explanationEntry.m_BackwardMemoryInstructions)) << "\n";
        stream << "\n";
        if (!onlyMemoryInstructionMask)
        {
            if (!explanationEntry.m_ForwardBoundaries.empty())
            {
                stream << indentation << "The forward boundary instructions: " << "\n";
                for (const ExplanationEntry::SyncInstDescription& syncDesc : explanationEntry.m_ForwardBoundaries)
                {
                    stream << indentation << indentation << GetSyncInstName(syncDesc) << "\n";
                }
                stream << "\n";
            }
            else
            {
                stream << indentation << "No forward boundary instructions." << "\n\n";
            }
            if (!explanationEntry.m_ThreadGroupBarriersBoundaries.empty())
            {
                stream << indentation << "The boundary thread group barrier instructions:" << "\n";
                for (const ExplanationEntry::SyncInstDescription& syncDesc : explanationEntry.m_ThreadGroupBarriersBoundaries)
                {
                    stream << indentation << indentation << GetSyncInstName(syncDesc) << "\n";
                }
                stream << "\n";
            }
            else
            {
                stream << indentation << "No boundary thread group barrier instructions." << "\n\n";
            }
            if (!explanationEntry.m_BackwardBoundaries.empty())
            {
                stream << indentation << "The backward boundary instructions:" << "\n";
                for (const ExplanationEntry::SyncInstDescription& syncDesc : explanationEntry.m_BackwardBoundaries)
                {
                    stream << indentation << indentation << GetSyncInstName(syncDesc) << "\n";
                }
                stream << "\n";
            }
            else
            {
                stream << indentation << "No backward boundary instructions." << "\n\n";
            }
            if (!explanationEntry.m_ForwardMemoryInstructions.empty())
            {
                stream << indentation << "The forward reachable memory instructions:" << "\n";
                for (const llvm::Instruction* pInst : explanationEntry.m_ForwardMemoryInstructions)
                {
                    stream << indentation << indentation << GetInstructionDescription(pInst) << "\n";
                }
                stream << "\n";
            }
            else
            {
                stream << indentation << "No forward reachable memory instructions." << "\n\n";
            }
            if (!explanationEntry.m_BackwardMemoryInstructions.empty())
            {
                stream << indentation << "The backward reachable memory instructions:" << "\n";
                for (const llvm::Instruction* pInst : explanationEntry.m_BackwardMemoryInstructions)
                {
                    stream << indentation << indentation << GetInstructionDescription(pInst) << "\n";
                }
                stream << "\n";
            }
            else
            {
                stream << indentation << "No backward reachable memory instructions." << "\n\n";
            }
        }

        index++;
    }
}

////////////////////////////////////////////////////////////////////////
/// @brief Dumps explanation for all redundancies.
void SynchronizationObjectCoalescingAnalysis::dump(bool onlyMemoryInstructionMask /*= true*/) const
{
    print(llvm::dbgs(), onlyMemoryInstructionMask);
}
#endif // _DEBUG

////////////////////////////////////////////////////////////////////////
/// @brief The pass is used for reducing either the number of synchronization
/// instructions or their scope of influence.
class SynchronizationObjectCoalescing : public llvm::FunctionPass
{
public:
    static char ID; ///< ID used by the llvm PassManager (the value is not important)

    SynchronizationObjectCoalescing();
    ~SynchronizationObjectCoalescing();

    ////////////////////////////////////////////////////////////////////////
    virtual bool runOnFunction(llvm::Function& F);

    ////////////////////////////////////////////////////////////////////////
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const;
};

char SynchronizationObjectCoalescing::ID = 0;

////////////////////////////////////////////////////////////////////////
SynchronizationObjectCoalescing::SynchronizationObjectCoalescing() :
    FunctionPass(ID)
{
    initializeSynchronizationObjectCoalescingPass(*llvm::PassRegistry::getPassRegistry());
}

////////////////////////////////////////////////////////////////////////
SynchronizationObjectCoalescing::~SynchronizationObjectCoalescing()
{
}

////////////////////////////////////////////////////////////////////////
/// @brief Either Removes redundant synchronization instructions or modifies
/// the scope of influence from synchronization instruction if safe.
bool SynchronizationObjectCoalescing::runOnFunction(llvm::Function& F)
{
    if (IGC_IS_FLAG_ENABLED(DisableSynchronizationObjectCoalescingPass))
    {
        return false;
    }
    const SynchronizationObjectCoalescingAnalysis& analysis = getAnalysis<SynchronizationObjectCoalescingAnalysis>();
    const llvm::DenseSet<llvm::Instruction*>& redundantInstructions = analysis.GetRedundantInstructions();

    // Removes redundant instructions
    for (llvm::Instruction* pInst : redundantInstructions)
    {
        pInst->eraseFromParent();
    }

    const llvm::DenseSet<llvm::Instruction*>& invalidationFunctionalityRedundancies = analysis.GetInvalidationFunctionalityRedundancies();

    // Disables invalidation of L1 cache for indicated memory fences
    for (llvm::Instruction* pInst : invalidationFunctionalityRedundancies)
    {
        if (redundantInstructions.find(pInst) != redundantInstructions.end())
        {
            continue;
        }

        llvm::GenIntrinsicInst* pGenIntrinsicInst = llvm::cast<llvm::GenIntrinsicInst>(pInst);

        switch (pGenIntrinsicInst->getIntrinsicID())
        {
        case llvm::GenISAIntrinsic::GenISA_memoryfence:
        {
            constexpr uint32_t L1CacheInvalidateArg = 6;
            pGenIntrinsicInst->setOperand(L1CacheInvalidateArg, llvm::ConstantInt::getFalse(pGenIntrinsicInst->getOperand(L1CacheInvalidateArg)->getType()));
            break;
        }
        case llvm::GenISAIntrinsic::GenISA_typedmemoryfence:
        {
            constexpr uint32_t L1CacheInvalidateArg = 0;
            pGenIntrinsicInst->setOperand(L1CacheInvalidateArg, llvm::ConstantInt::getFalse(pGenIntrinsicInst->getOperand(L1CacheInvalidateArg)->getType()));
            break;
        }
        default:
            IGC_ASSERT(0);
            break;
        }
    }

    const llvm::DenseSet<llvm::Instruction*>& globalMemoryRedundancies = analysis.GetGlobalMemoryRedundancies();

    // Diminishes the scope of indicated untyped memory fences
    for (llvm::Instruction* pInst : globalMemoryRedundancies)
    {
        if (redundantInstructions.find(pInst) != redundantInstructions.end())
        {
            continue;
        }

        llvm::GenIntrinsicInst* pGenIntrinsicInst = llvm::cast<llvm::GenIntrinsicInst>(pInst);

        switch (pGenIntrinsicInst->getIntrinsicID())
        {
        case llvm::GenISAIntrinsic::GenISA_memoryfence:
        {
            constexpr uint32_t globalMemFenceArg = 5;
            pGenIntrinsicInst->setOperand(globalMemFenceArg, llvm::ConstantInt::getFalse(pGenIntrinsicInst->getOperand(globalMemFenceArg)->getType()));
            break;
        }
        default:
            IGC_ASSERT(0);
            break;
        }
    }

    return redundantInstructions.size() > 0 ||
        invalidationFunctionalityRedundancies.size() > 0 ||
        globalMemoryRedundancies.size() > 0;
}

////////////////////////////////////////////////////////////////////////
void SynchronizationObjectCoalescing::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    AU.setPreservesCFG();
    AU.addRequired<SynchronizationObjectCoalescingAnalysis>();
}

////////////////////////////////////////////////////////////////////////
llvm::Pass* createSynchronizationObjectCoalescing()
{
    return new SynchronizationObjectCoalescing();
}

}

using namespace llvm;
using namespace IGC;
#define PASS_FLAG "igc-synchronization-object-coalescing-analysis"
#define PASS_DESCRIPTION "SynchronizationObjectCoalescingAnalysis"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(SynchronizationObjectCoalescingAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(SynchronizationObjectCoalescingAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
#undef PASS_FLAG
#undef PASS_DESCRIPTION
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS

#define PASS_FLAG "igc-synchronization-object-coalescing"
#define PASS_DESCRIPTION "SynchronizationObjectCoalescing"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SynchronizationObjectCoalescing, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(SynchronizationObjectCoalescingAnalysis)
IGC_INITIALIZE_PASS_END(SynchronizationObjectCoalescing, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
