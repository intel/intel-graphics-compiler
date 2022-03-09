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
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/IGCPassSupport.h"
#include "SynchronizationObjectCoalescing.hpp"
#include "visa_igc_common_header.h"
#include <memory>
#include <utility>
#include <map>

using namespace llvm;
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

////////////////////////////////////////////////////////////////////////////////
enum InstructionMask : uint32_t
{
    None = 0x0,
    AtomicOperation            = (1 << 0),
    TypedReadOperation         = (1 << 1),
    TypedWriteOperation        = (1 << 2),
    OutputUrbReadOperation     = (1 << 3),
    UrbWriteOperation          = (1 << 4),
    BufferReadOperation        = (1 << 5),
    BufferWriteOperation       = (1 << 6),
    SharedMemoryReadOperation  = (1 << 7),
    SharedMemoryWriteOperation = (1 << 8),
};
constexpr InstructionMask AllNoAtomicMask =
    InstructionMask{ ((1 << 9) - 1) & ~InstructionMask::AtomicOperation };

inline constexpr InstructionMask operator|(InstructionMask a, InstructionMask b)
{
    return InstructionMask(uint32_t(a) | uint32_t(b));
}
inline constexpr InstructionMask& operator|=(InstructionMask& a, InstructionMask b)
{
    a = a | b;
    return a;
}

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
/// - typed memory fences:    llvm.genx.GenISA.typedmemoryfence;
///   (interacts with typed buffers and atomics)
/// - URB memory fences:      llvm.genx.GenISA.urbfence;
///   (interacts with outputs and atomics)
/// - LSC fences:      llvm.genx.GenISA.LSCfence;
///   Current implementation of the pass treats LSC fences separately from
///   legacy memoryfence intrinsics. The assumption is that frontends will
///   either use legacy memoryfence intrinsics or LSC fences and not a mix.
///   Note: a simple pass could be added to translate the legacy memoryfence
///   intrinsics to LSC intrinsics if needed.
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
        Empty = 0x0,
        ReadSyncWrite = 0x1,
        WriteSyncWrite = 0x2,
        AtomicSyncRead = 0x4,
        AtomicSyncWrite = 0x8,
        WriteSyncAtomic = 0x10,
        ReadSyncAtomic = 0x20,
        WriteSyncRead = 0x40,
        AtomicSyncAtomic = 0x80,
    };

    static constexpr SynchronizationCaseMask sc_FullSynchronizationCaseMask = static_cast<SynchronizationCaseMask>(
        WriteSyncRead |
        WriteSyncWrite |
        AtomicSyncRead |
        AtomicSyncWrite |
        WriteSyncAtomic |
        ReadSyncAtomic |
        ReadSyncWrite |
        AtomicSyncAtomic);

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
    InstructionMask GetAtomicInstructionMaskFromPointer(
        const llvm::Instruction* pSourceInst) const;

    ////////////////////////////////////////////////////////////////////////
    InstructionMask GetUnsynchronizedForwardInstructionMask(
        const llvm::Instruction* pSourceInst) const;

    ////////////////////////////////////////////////////////////////////////
    bool IsRequiredForAtomicOperationsOrdering(
        const llvm::Instruction* pSourceInst,
        bool onlyGlobalAtomics = false) const;

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
    SynchronizationCaseMask GetSynchronizationMask(
        InstructionMask localForwardMemoryInstructionMask,
        InstructionMask localBackwardMemoryInstructionMask,
        InstructionMask readBit,
        InstructionMask writeBit) const;

    ////////////////////////////////////////////////////////////////////////
    SynchronizationCaseMask GetStrictSynchronizationMask(
        llvm::Instruction* pInst) const;

    ////////////////////////////////////////////////////////////////////////
    SynchronizationCaseMask GetL1CacheInvalidatioSynchronizationMask() const;

    ////////////////////////////////////////////////////////////////////////
    SynchronizationCaseMask GetSynchronizationMaskForAllResources(
        InstructionMask localForwardMemoryInstructionMask,
        InstructionMask localBackwardMemoryInstructionMask) const;;

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
    static bool IsTypedMemoryFenceOperation(const llvm::Instruction* pInst);

    ////////////////////////////////////////////////////////////////////////
    bool IsTypedMemoryFenceOperationWithInvalidationFunctionality(const llvm::Instruction* pInst) const;

    ////////////////////////////////////////////////////////////////////////
    static bool IsUrbFenceOperation(const llvm::Instruction* pInst);

    ////////////////////////////////////////////////////////////////////////
    static bool IsFenceOperation(const llvm::Instruction* pInst);

    ////////////////////////////////////////////////////////////////////////
    static bool IsGlobalResource(llvm::Type* pResourePointerType);

    ////////////////////////////////////////////////////////////////////////
    static bool IsSharedMemoryResource(llvm::Type* pResourePointerType);

    std::vector<llvm::Instruction*> m_TypedMemoryFences;
    std::vector<llvm::Instruction*> m_UrbMemoryFences;
    std::vector<llvm::Instruction*> m_LscMemoryFences;
    std::vector<llvm::Instruction*> m_UntypedMemoryFences;
    std::vector<llvm::Instruction*> m_ThreadGroupBarriers;

    llvm::DenseSet<llvm::Instruction*> m_RedundantInstructions;
    llvm::DenseSet<llvm::Instruction*> m_InvalidationFunctionalityRedundancies;
    llvm::DenseSet<llvm::Instruction*> m_GlobalMemoryRedundancies;
    llvm::DenseSet<llvm::Instruction*> m_FencesBetweenReadAndWriteInstructions;

    llvm::Function* m_CurrentFunction = nullptr;
    bool m_HasIndependentSharedMemoryFenceFunctionality = false;
    bool m_HasTypedMemoryFenceFunctionality = false;
    bool m_HasUrbFenceFunctionality = false;
    InstructionMask m_GlobalMemoryInstructionMask = InstructionMask::None;
    ShaderType m_ShaderType = ShaderType::UNKNOWN;

#if _DEBUG
    std::vector<ExplanationEntry> m_ExplanationEntries;

    ////////////////////////////////////////////////////////////////////////
    void RegisterRedundancyExplanation(const llvm::Instruction* pInst, ExplanationEntry::Cause cause);
#endif // _DEBUG
};

static inline bool IsLscFenceOperation(const Instruction* pInst);
static inline LSC_SFID GetLscMem(const Instruction* pInst);
static inline LSC_SCOPE GetLscScope(const Instruction* pInst);
static inline LSC_FENCE_OP GetLscFenceOp(const Instruction* pInst);

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
    m_HasTypedMemoryFenceFunctionality = ctx->platform.hasLSC() && ctx->platform.LSCEnabled();
    m_HasUrbFenceFunctionality = ctx->platform.hasURBFence();
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
    // This lambda function identifies partial and strict redundancies among synchronization operations
    auto GatherRedundancies = [this](std::vector<llvm::Instruction*>& synchronizationOperations)
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
                IGC_ASSERT(
                    (localForwardMemoryInstructionMask == AllNoAtomicMask) ||
                    ((localForwardMemoryInstructionMask & GetDefaultMemoryInstructionMask(pInst)) == localForwardMemoryInstructionMask));
                IGC_ASSERT((localBackwardMemoryInstructionMask & GetDefaultMemoryInstructionMask(pInst)) == localBackwardMemoryInstructionMask);
            };
            SetLocalMemoryInstructionMask();

            auto GatherPartialRedundancy = [this, &pInst, &SetLocalMemoryInstructionMask]
                (llvm::DenseSet<llvm::Instruction*>& redundantCollection)
            {
                redundantCollection.insert(pInst);
                SetLocalMemoryInstructionMask();
            };

            // Partial redundancies:
            // 1) Applies to untyped and typed memory fences with invalidation
            //    functionality. Removing this functionality is possible if there
            //    is no read instruction which can depend on this synchronization
            //    instruction. Such a reduction can result in removing the whole
            //    instruction because of changing classification of the instruction.
            // 2) Applies to untyped memory fences interacting with global memory.
            //    Removing this association is possible if there is no global
            //    write instruction which can depend on this synchronization
            //    instruction. Such a reduction can result in removing the whole
            //    instruction because of changing classification of the instruction.

            // identify partial redundancies for untyped memory fences
            if (IsUntypedMemoryFenceOperationForGlobalAccess(pInst))
            {
                if (IsUntypedMemoryFenceOperationWithInvalidationFunctionality(pInst))
                {
                    // buffer access
                    SynchronizationCaseMask syncCaseMask = GetSynchronizationMask(localForwardMemoryInstructionMask, localBackwardMemoryInstructionMask, BufferReadOperation, BufferWriteOperation);
                    if (!m_HasTypedMemoryFenceFunctionality)
                    {
                        syncCaseMask = static_cast<SynchronizationCaseMask>(syncCaseMask | GetSynchronizationMask(localForwardMemoryInstructionMask, localBackwardMemoryInstructionMask, TypedReadOperation, TypedWriteOperation));
                    }
                    if (!m_HasUrbFenceFunctionality)
                    {
                        syncCaseMask = static_cast<SynchronizationCaseMask>(syncCaseMask | GetSynchronizationMask(localForwardMemoryInstructionMask, localBackwardMemoryInstructionMask, OutputUrbReadOperation, UrbWriteOperation));
                    }
                    syncCaseMask = static_cast<SynchronizationCaseMask>(syncCaseMask | GetSynchronizationMask(localForwardMemoryInstructionMask, localBackwardMemoryInstructionMask, OutputUrbReadOperation, UrbWriteOperation));

                    bool isObligatory = (syncCaseMask & GetL1CacheInvalidatioSynchronizationMask()) != 0;
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
                    SynchronizationCaseMask syncCaseMask =
                        GetSynchronizationMask(localForwardMemoryInstructionMask, localBackwardMemoryInstructionMask, BufferReadOperation, BufferWriteOperation);
                    if (!m_HasTypedMemoryFenceFunctionality)
                    {
                        syncCaseMask = static_cast<SynchronizationCaseMask>(syncCaseMask |
                            GetSynchronizationMask(localForwardMemoryInstructionMask, localBackwardMemoryInstructionMask, TypedReadOperation, TypedWriteOperation));
                    }
                    if (!m_HasUrbFenceFunctionality)
                    {
                        syncCaseMask = static_cast<SynchronizationCaseMask>(syncCaseMask |
                            GetSynchronizationMask(localForwardMemoryInstructionMask, localBackwardMemoryInstructionMask, OutputUrbReadOperation, UrbWriteOperation));
                    }
                    SynchronizationCaseMask referenceSyncCaseMask = GetStrictSynchronizationMask(pInst);
                    bool isObligatory = (syncCaseMask & referenceSyncCaseMask) != 0;
                    isObligatory |= IsRequiredForAtomicOperationsOrdering(pInst, true /*onlyGlobalAtomics*/);
                    bool verifyUnsynchronizedInstructions = IsFenceOperation(pInst);
                    verifyUnsynchronizedInstructions &= (!isObligatory || syncCaseMask == ReadSyncWrite);

                    if (verifyUnsynchronizedInstructions)
                    {
                        InstructionMask unsynchronizedForwardMemoryInstructionMask = GetUnsynchronizedForwardInstructionMask(pInst);
                        SynchronizationCaseMask syncCaseMaskForUnsynchronizedInstructions =
                            GetSynchronizationMask(unsynchronizedForwardMemoryInstructionMask, localBackwardMemoryInstructionMask, BufferReadOperation, BufferWriteOperation);
                        if (!m_HasTypedMemoryFenceFunctionality)
                        {
                            syncCaseMaskForUnsynchronizedInstructions = static_cast<SynchronizationCaseMask>(syncCaseMaskForUnsynchronizedInstructions |
                                GetSynchronizationMask(unsynchronizedForwardMemoryInstructionMask, localBackwardMemoryInstructionMask, TypedReadOperation, TypedWriteOperation));
                        }
                        if (!m_HasUrbFenceFunctionality)
                        {
                            syncCaseMaskForUnsynchronizedInstructions = static_cast<SynchronizationCaseMask>(syncCaseMaskForUnsynchronizedInstructions |
                                GetSynchronizationMask(unsynchronizedForwardMemoryInstructionMask, localBackwardMemoryInstructionMask, OutputUrbReadOperation, UrbWriteOperation));
                        }
                        syncCaseMask = syncCaseMaskForUnsynchronizedInstructions;
                        isObligatory = (syncCaseMask & referenceSyncCaseMask) != 0;
                    }

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

            // identify partial redundancies for typed memory fences
            if (IsTypedMemoryFenceOperationWithInvalidationFunctionality(pInst))
            {
                // typed access
                SynchronizationCaseMask syncCaseMask = GetSynchronizationMask(localForwardMemoryInstructionMask, localBackwardMemoryInstructionMask, TypedReadOperation, TypedWriteOperation);
                bool isObligatory = (syncCaseMask & GetL1CacheInvalidatioSynchronizationMask()) != 0;

                if (!isObligatory)
                {
#if _DEBUG
                    RegisterRedundancyExplanation(pInst, ExplanationEntry::L1CacheInvalidationRedundancy);
#endif // _DEBUG
                    GatherPartialRedundancy(m_InvalidationFunctionalityRedundancies);
                }
            }

            if (IsLscFenceOperation(pInst) &&
                m_InvalidationFunctionalityRedundancies.find(pInst) == m_InvalidationFunctionalityRedundancies.end())
            {
                LSC_SFID mem = GetLscMem(pInst);
                LSC_FENCE_OP op = GetLscFenceOp(pInst);
                if (op == LSC_FENCE_OP_INVALIDATE &&
                    (mem == LSC_UGM || mem == LSC_TGM))
                {
                    SynchronizationCaseMask syncCaseMask = GetSynchronizationMask(
                        localForwardMemoryInstructionMask,
                        localBackwardMemoryInstructionMask,
                        (mem == LSC_TGM ? TypedReadOperation : BufferReadOperation),
                        (mem == LSC_TGM ? TypedWriteOperation : BufferWriteOperation));
                    bool isObligatory = (syncCaseMask & GetL1CacheInvalidatioSynchronizationMask()) != 0;
                    if (!isObligatory)
                    {
#if _DEBUG
                        RegisterRedundancyExplanation(pInst, ExplanationEntry::L1CacheInvalidationRedundancy);
#endif // _DEBUG
                        GatherPartialRedundancy(m_InvalidationFunctionalityRedundancies);
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

            SynchronizationCaseMask syncCaseMask = GetSynchronizationMaskForAllResources(localForwardMemoryInstructionMask, localBackwardMemoryInstructionMask);
            SynchronizationCaseMask referenceSyncCaseMask = GetStrictSynchronizationMask(pInst);
            bool isObligatory = (syncCaseMask & referenceSyncCaseMask) != 0;
            isObligatory |= IsRequiredForAtomicOperationsOrdering(pInst);
            bool verifyUnsynchronizedInstructions = IsFenceOperation(pInst);
            verifyUnsynchronizedInstructions &= (!isObligatory || syncCaseMask == ReadSyncWrite);

            if (verifyUnsynchronizedInstructions)
            {
                InstructionMask unsynchronizedForwardMemoryInstructionMask = GetUnsynchronizedForwardInstructionMask(pInst);
                SynchronizationCaseMask syncCaseMaskForUnsynchronizedInstructions = GetSynchronizationMaskForAllResources(unsynchronizedForwardMemoryInstructionMask, localBackwardMemoryInstructionMask);
                syncCaseMask = syncCaseMaskForUnsynchronizedInstructions;
                isObligatory = (syncCaseMask & referenceSyncCaseMask) != 0;
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
    GatherRedundancies(m_TypedMemoryFences);
    GatherRedundancies(m_UrbMemoryFences);
    GatherRedundancies(m_LscMemoryFences);
}

////////////////////////////////////////////////////////////////////////
/// @brief Provides write memory instructions mask which are synchronized
/// by the instruction.
InstructionMask SynchronizationObjectCoalescingAnalysis::GetDefaultWriteMemoryInstructionMask(const llvm::Instruction* pSourceInst) const
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
                !m_HasUrbFenceFunctionality &&
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
            if (!m_HasTypedMemoryFenceFunctionality)
            {
                result = static_cast<InstructionMask>(
                    result |
                    TypedWriteOperation);
            }
            if (!m_HasUrbFenceFunctionality)
            {
                // This is for ICL+ but should not harm on pre-ICL
                result = static_cast<InstructionMask>(
                    result |
                    UrbWriteOperation);
            }
        }
    }
    else if (IsLscFenceOperation(pSourceInst))
    {
        IGC_ASSERT(m_HasUrbFenceFunctionality);
        IGC_ASSERT(m_HasIndependentSharedMemoryFenceFunctionality);
        switch (GetLscMem(pSourceInst))
        {
        case LSC_UGM: // .ugm
        case LSC_UGML: // .ugml
            result |= BufferWriteOperation;
            break;
        case LSC_TGM: // .tgm
            result |= TypedWriteOperation;
            break;
        case LSC_SLM: // .slm
            result |= SharedMemoryWriteOperation;
            break;
        }
    }
    else if (IsTypedMemoryFenceOperation(pSourceInst))
    {
        const bool hasInvalidationFunctionality = IsTypedMemoryFenceOperationWithInvalidationFunctionality(pSourceInst);
        result = static_cast<InstructionMask>(
            result |
            TypedWriteOperation);
    }
    else if (IsUrbFenceOperation(pSourceInst))
    {
        result = static_cast<InstructionMask>(
            result |
            UrbWriteOperation);
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
InstructionMask SynchronizationObjectCoalescingAnalysis::GetDefaultMemoryInstructionMask(
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
                !m_HasUrbFenceFunctionality &&
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
            if (!m_HasTypedMemoryFenceFunctionality)
            {
                result = static_cast<InstructionMask>(
                    result |
                    TypedWriteOperation |
                    TypedReadOperation);
            }
            if (!m_HasUrbFenceFunctionality)
            {
                // This is for ICL+ but should not harm on pre-ICL
                result = static_cast<InstructionMask>(
                    result |
                    UrbWriteOperation |
                    OutputUrbReadOperation);
            }
        }
    }
    else if (IsLscFenceOperation(pSourceInst))
    {
        IGC_ASSERT(m_HasUrbFenceFunctionality);
        IGC_ASSERT(m_HasIndependentSharedMemoryFenceFunctionality);
        switch (GetLscMem(pSourceInst))
        {
        case LSC_UGM: // .ugm
        case LSC_UGML: // .ugml
            result |= AtomicOperation | BufferWriteOperation | BufferReadOperation;
            break;
        case LSC_TGM: // .tgm
            result |= AtomicOperation | TypedWriteOperation | TypedReadOperation;
            break;
        case LSC_SLM: // .slm
            result |= AtomicOperation | SharedMemoryWriteOperation | SharedMemoryReadOperation;
            break;
        }
    }
    else if (IsTypedMemoryFenceOperation(pSourceInst))
    {
        const bool hasInvalidationFunctionality = IsTypedMemoryFenceOperationWithInvalidationFunctionality(pSourceInst);
        result = static_cast<InstructionMask>(
            result |
            AtomicOperation);

        result = static_cast<InstructionMask>(
            result |
            TypedWriteOperation |
            TypedReadOperation);
    }
    else if (IsUrbFenceOperation(pSourceInst))
    {
        result = static_cast<InstructionMask>(
            result |
            OutputUrbReadOperation |
            UrbWriteOperation |
            AtomicOperation);
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
InstructionMask SynchronizationObjectCoalescingAnalysis::GetInstructionMask(
    const llvm::Instruction* pSourceInst,
    bool forwardDirection) const
{
    std::vector<const llvm::Instruction*> boundaryInstructions;
    std::vector<const llvm::Instruction*> memoryInstructions;
    GetVisibleMemoryInstructions(pSourceInst, forwardDirection, boundaryInstructions, memoryInstructions);
    return GetInstructionMask(memoryInstructions);
}

////////////////////////////////////////////////////////////////////////
/// @brief Provides the memory instruction mask from the atomic operation
/// instruction based on the destination memory address.
/// @param pSourceInst the atomic operation operation instruction
IGC::InstructionMask SynchronizationObjectCoalescingAnalysis::GetAtomicInstructionMaskFromPointer(const llvm::Instruction* pSourceInst) const
{
    InstructionMask memoryInstructionMask = GetInstructionMask(pSourceInst);
    InstructionMask result{};
    if (memoryInstructionMask == InstructionMask::AtomicOperation)
    {
        const llvm::GenIntrinsicInst* pGenIntrinsicInst = llvm::cast<llvm::GenIntrinsicInst>(pSourceInst);
        if (pGenIntrinsicInst->getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_intatomictyped ||
            pGenIntrinsicInst->getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_icmpxchgatomictyped)
        {
            result = static_cast<InstructionMask>(result | InstructionMask::TypedReadOperation);
            result = static_cast<InstructionMask>(result | InstructionMask::TypedWriteOperation);
        }
        else
        {
            llvm::Type* pPointerType = pGenIntrinsicInst->getOperand(0)->getType();
            if (IsGlobalResource(pPointerType))
            {
                result = static_cast<InstructionMask>(result | InstructionMask::BufferReadOperation);
                result = static_cast<InstructionMask>(result | InstructionMask::BufferWriteOperation);
            }
            else if (IsSharedMemoryResource(pPointerType))
            {
                result = static_cast<InstructionMask>(result | InstructionMask::SharedMemoryReadOperation);
                result = static_cast<InstructionMask>(result | InstructionMask::SharedMemoryWriteOperation);
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////
/// @brief This function verifies which cases of synchronization can be
/// attributed to this synchronization instruction in the case of the particular resource.
/// This stems from the forwardly and backwardly visible memory instructions and
/// their interaction with the synchronization instruction.
/// @param localForwardMemoryInstructionMask the mask with forwardly visible memory instructions
/// @param localBackwardMemoryInstructionMask the mask with backwardly visible memory instructions
/// @param readBit the read instruction bit for the particular resource
/// @param writeBit the write instruction bit for the particular resource
SynchronizationObjectCoalescingAnalysis::SynchronizationCaseMask SynchronizationObjectCoalescingAnalysis::GetSynchronizationMask(
    InstructionMask localForwardMemoryInstructionMask,
    InstructionMask localBackwardMemoryInstructionMask,
    InstructionMask readBit,
    InstructionMask writeBit) const
{
    SynchronizationCaseMask result = SynchronizationCaseMask::Empty;
    // write -> barrier/fence -> read
    bool isWriteSyncReadCase = ((localBackwardMemoryInstructionMask & writeBit) != 0 && (localForwardMemoryInstructionMask & readBit) != 0);
    if (isWriteSyncReadCase)
    {
        result = static_cast<SynchronizationCaseMask>(result | SynchronizationCaseMask::WriteSyncRead);
    }

    // write -> barrier/fence -> write
    bool isWriteSyncWriteCase = ((localBackwardMemoryInstructionMask & writeBit) != 0 && (localForwardMemoryInstructionMask & writeBit) != 0);
    if (isWriteSyncWriteCase)
    {
        result = static_cast<SynchronizationCaseMask>(result | SynchronizationCaseMask::WriteSyncWrite);
    }

    // atomic -> barrier/fence -> read
    bool isAtomicSyncReadCase = ((localBackwardMemoryInstructionMask & AtomicOperation) != 0 && (localForwardMemoryInstructionMask & readBit) != 0);
    if (isAtomicSyncReadCase)
    {
        result = static_cast<SynchronizationCaseMask>(result | SynchronizationCaseMask::AtomicSyncRead);
    }

    // atomic -> barrier/fence -> write
    bool isAtomicSyncWriteCase = ((localBackwardMemoryInstructionMask & AtomicOperation) != 0 && (localForwardMemoryInstructionMask & writeBit) != 0);
    if (isAtomicSyncWriteCase)
    {
        result = static_cast<SynchronizationCaseMask>(result | SynchronizationCaseMask::AtomicSyncWrite);
    }

    // write -> barrier/fence -> atomic
    bool isWriteSyncAtomicCase = ((localBackwardMemoryInstructionMask & writeBit) != 0 && (localForwardMemoryInstructionMask & AtomicOperation) != 0);
    if (isWriteSyncAtomicCase)
    {
        result = static_cast<SynchronizationCaseMask>(result | SynchronizationCaseMask::WriteSyncAtomic);
    }

    // read -> barrier -> atomic
    bool isReadSyncAtomicCase = ((localBackwardMemoryInstructionMask & readBit) != 0 && (localForwardMemoryInstructionMask & AtomicOperation) != 0);
    if (isReadSyncAtomicCase)
    {
        result = static_cast<SynchronizationCaseMask>(result | SynchronizationCaseMask::ReadSyncAtomic);
    }

    // read -> barrier -> write
    bool isReadSyncWriteCase = ((localBackwardMemoryInstructionMask & readBit) != 0 && (localForwardMemoryInstructionMask & writeBit) != 0);
    if (isReadSyncWriteCase)
    {
        result = static_cast<SynchronizationCaseMask>(result | SynchronizationCaseMask::ReadSyncWrite);
    }

    // atomic -> barrier -> atomic
    bool isAtomicSyncAtomicCase = ((localBackwardMemoryInstructionMask & AtomicOperation) != 0 && (localForwardMemoryInstructionMask & AtomicOperation) != 0);
    if (isAtomicSyncAtomicCase)
    {
        result = static_cast<SynchronizationCaseMask>(result | SynchronizationCaseMask::AtomicSyncAtomic);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////
/// @brief Provides the synchronization case mask for determining strict
/// redundancy.
SynchronizationObjectCoalescingAnalysis::SynchronizationCaseMask SynchronizationObjectCoalescingAnalysis::GetStrictSynchronizationMask(llvm::Instruction* pInst) const
{
    SynchronizationCaseMask strictSynchronizationCaseMask = sc_FullSynchronizationCaseMask;

    if (IsFenceOperation(pInst))
    {
        // fences doesn't provide any guarantees for the order of instruction execution between threads
        strictSynchronizationCaseMask = static_cast<SynchronizationCaseMask>((~SynchronizationCaseMask::AtomicSyncAtomic) & strictSynchronizationCaseMask);

        // Note: Please change the description in igc flags if the value is changed.
        static_assert(SynchronizationCaseMask::ReadSyncWrite == 0x01);
        bool disableReadFenceWriteCase = (IGC_GET_FLAG_VALUE(SynchronizationObjectCoalescingConfig) & SynchronizationCaseMask::ReadSyncWrite) != 0;
        if (disableReadFenceWriteCase)
        {
            strictSynchronizationCaseMask = static_cast<SynchronizationCaseMask>((~SynchronizationCaseMask::ReadSyncWrite) & strictSynchronizationCaseMask);
        }
    }

    return strictSynchronizationCaseMask;
}

////////////////////////////////////////////////////////////////////////
/// @brief Provides the synchronization case mask for determining L1 Cache
/// invalidation redundancy.
IGC::SynchronizationObjectCoalescingAnalysis::SynchronizationCaseMask SynchronizationObjectCoalescingAnalysis::GetL1CacheInvalidatioSynchronizationMask() const
{
    constexpr SynchronizationCaseMask L1CacheInvalidationCaseMask = static_cast<SynchronizationCaseMask>(
        SynchronizationCaseMask::WriteSyncRead |
        SynchronizationCaseMask::WriteSyncAtomic |
        SynchronizationCaseMask::AtomicSyncRead);
    return L1CacheInvalidationCaseMask;
}

////////////////////////////////////////////////////////////////////////
/// @brief Provides the synchronization case mask for all resources.
SynchronizationObjectCoalescingAnalysis::SynchronizationCaseMask SynchronizationObjectCoalescingAnalysis::GetSynchronizationMaskForAllResources(
    InstructionMask localForwardMemoryInstructionMask,
    InstructionMask localBackwardMemoryInstructionMask) const
{
    // buffer access
    SynchronizationCaseMask syncCaseMask = GetSynchronizationMask(localForwardMemoryInstructionMask, localBackwardMemoryInstructionMask, BufferReadOperation, BufferWriteOperation);
    // shared memory access
    syncCaseMask = static_cast<SynchronizationCaseMask>(syncCaseMask | GetSynchronizationMask(localForwardMemoryInstructionMask, localBackwardMemoryInstructionMask, SharedMemoryReadOperation, SharedMemoryWriteOperation));
    // typed access
    syncCaseMask = static_cast<SynchronizationCaseMask>(syncCaseMask | GetSynchronizationMask(localForwardMemoryInstructionMask, localBackwardMemoryInstructionMask, TypedReadOperation, TypedWriteOperation));
    // URB access
    syncCaseMask = static_cast<SynchronizationCaseMask>(syncCaseMask | GetSynchronizationMask(localForwardMemoryInstructionMask, localBackwardMemoryInstructionMask, OutputUrbReadOperation, UrbWriteOperation));

    return syncCaseMask;
}

////////////////////////////////////////////////////////////////////////
/// @brief Provides the memory instruction mask from unsynchronized forward
/// memory instructions (in the next synchronization block delineated by
/// thread group barriers)
/// @param pSourceInst the source synchronization instruction
InstructionMask SynchronizationObjectCoalescingAnalysis::GetUnsynchronizedForwardInstructionMask(
    const llvm::Instruction* pSourceInst) const
{
    std::vector<const llvm::Instruction*> boundaryInstructions;
    std::vector<const llvm::Instruction*> memoryInstructions;
    GetAllUnsynchronizedMemoryInstructions(pSourceInst, boundaryInstructions, memoryInstructions);
    return GetInstructionMask(memoryInstructions);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief Checks if the fence is required to assure correct ordering of atomic
/// operations present before the fence (in program order)
/// @param pSourceInst the source synchronization instruction
/// @param onlyGlobalAtomics check only TGM and UGM atomic operations
bool SynchronizationObjectCoalescingAnalysis::IsRequiredForAtomicOperationsOrdering(
    const llvm::Instruction* pSourceInst,
    bool onlyGlobalAtomics /*= false*/) const
{
    if (!IsFenceOperation(pSourceInst))
    {
        // Not a fence, nothing to check
        return false;
    }
    InstructionMask defaultSourceInstructionMask = GetDefaultMemoryInstructionMask(pSourceInst);
    if (onlyGlobalAtomics)
    {
        // Change the mask to include only global memory operations
        InstructionMask globalOnlyMask = InstructionMask::None;
        InstructionMask typedReadOrWrite = static_cast<InstructionMask>(TypedReadOperation | TypedWriteOperation);
        InstructionMask globalReadOrWrite = static_cast<InstructionMask>(BufferReadOperation | BufferWriteOperation);
        if ((defaultSourceInstructionMask & typedReadOrWrite) != InstructionMask::None)
        {
            globalOnlyMask = typedReadOrWrite;
        }
        if ((defaultSourceInstructionMask & globalReadOrWrite) != InstructionMask::None)
        {
            globalOnlyMask |= globalReadOrWrite;
        }
        defaultSourceInstructionMask = globalOnlyMask;
    }
    if (defaultSourceInstructionMask == InstructionMask::None)
    {
        return false;
    }

    // Gather all instructions that the source fence operation affects.
    constexpr bool backwardDirection = false;
    std::vector<const llvm::Instruction*> boundaryInstructions;
    std::vector<const llvm::Instruction*> memoryInstructions;
    GetVisibleMemoryInstructions(pSourceInst, backwardDirection, boundaryInstructions, memoryInstructions);
    for (const llvm::Instruction* pInst : memoryInstructions)
    {
        InstructionMask memoryInstructionMask = GetInstructionMask(pInst);
        if (memoryInstructionMask != InstructionMask::AtomicOperation)
        {
            // Only atomic operations are checked.
            continue;
        }
        InstructionMask atomicPointerMemoryInstructionMask = GetAtomicInstructionMaskFromPointer(pInst);
        bool isPotentiallyUnsynchronizedAtomic = (atomicPointerMemoryInstructionMask & defaultSourceInstructionMask) != 0;
        if (!isPotentiallyUnsynchronizedAtomic)
        {
            // The source fence does not affect the atomic instruction.
            continue;
        }
        // Check if the atomic operation is not synchronized by a different
        // fence operation with different scope or memory operation.
        // e.g.:
        // %0 = call i32 @llvm.genx.GenISA.atomiccounterinc.p2490368i8(i8 addrspace(2490368)* %atomic_address)
        // call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 true, i1 false)
        // call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 true, i1 true)
        // store float %value, float addrspace(1)* %global_address
        {
            isPotentiallyUnsynchronizedAtomic = false;
            // Lambda that checks if a fence operation synchronizes the atomic operation.
            std::function<bool(const llvm::Instruction*)> BoundaryPredicate =
                [this,
                &atomicPointerMemoryInstructionMask,
                &isPotentiallyUnsynchronizedAtomic,
                pSourceInst](
                    const llvm::Instruction* pInst)
            {
                if (pInst == pSourceInst)
                {
                    // Instruction search reached the source instruction, there is
                    // no other fence instruction that assures ordering of the atomic.
                    isPotentiallyUnsynchronizedAtomic = true;
                }
                bool isFence = IsFenceOperation(pInst);
                if (isFence &&
                    m_RedundantInstructions.find(pInst) == m_RedundantInstructions.end())
                {
                    InstructionMask memoryInstructionMask = GetDefaultMemoryInstructionMask(pInst);
                    bool isBoundary = (atomicPointerMemoryInstructionMask & memoryInstructionMask) != 0;
                    return isBoundary;
                }
                return false;
            };
            // Dummy lambda to disable collection of instructions
            std::function<bool(const llvm::Instruction*)> CollectNone = [](
                const llvm::Instruction* pInst)
            {
                return false;
            };

            llvm::DenseSet<const llvm::BasicBlock*> visitedBasicBlocks;
            llvm::BasicBlock::const_iterator firstIt = ++pInst->getIterator();
            std::list<llvm::BasicBlock::const_iterator> workList{ firstIt };
            std::vector<const llvm::Instruction*> boundaryInstructions;;
            std::vector<const llvm::Instruction*> collectedInstructions;
            // Start from the atomic operation and check all instruction in the
            // forward direction.
            SearchInstructions(
                workList,
                visitedBasicBlocks,
                collectedInstructions,
                CollectNone,
                boundaryInstructions,
                BoundaryPredicate);
            IGC_ASSERT(boundaryInstructions.size() > 0);
        }

        // Check if any of the instructions that immediately follow the fence
        // instruction is a fence that assures ordering of current atomic
        // operation.
        // Note: this approach addresses the simplest cases like the example
        // below and can be improved in the future:
        // %0 = call i32 @llvm.genx.GenISA.atomiccounterinc.p2490368i8(i8 addrspace(2490368)* %atomic_address)
        // call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 true, i1 false)
        // call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 true, i1 true)
        // store float %value, float addrspace(1)* %global_address
        // %1 = load float, float addrspace(1)* %global_address
        if (isPotentiallyUnsynchronizedAtomic)
        {
            bool substituteFenceFound = false;
            for (llvm::BasicBlock::const_iterator it = ++pSourceInst->getIterator(); it != pSourceInst->getParent()->end(); ++it)
            {
                const llvm::Instruction* pCurrInst = &(*it);
                if (IsFenceOperation(pCurrInst) &&
                    IsSubsituteInstruction(pCurrInst, pSourceInst) &&
                    m_RedundantInstructions.find(pInst) == m_RedundantInstructions.end())
                {
                    substituteFenceFound = true;
                    break;
                }
            }
            if (!substituteFenceFound)
            {
                // Found an atomic operation that requires the source fence
                // instruction for correct memory ordering.
                return true;
            }
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////
/// @brief Provides the memory instruction mask from reachable memory instructions
/// of this source instruction. The boundary for this searching are drawn
/// by visible substitutes. The visibility term means such a reachability which
/// is determined by the fact if any path between this source instruction and
/// a substitute is not crossed by another substitute.
/// @param pSourceInst the source synchronization instruction
/// @param forwardDirection the direction of searching
InstructionMask SynchronizationObjectCoalescingAnalysis::GetInstructionMask(
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
    else if (IsTypedMemoryFenceOperation(pReferenceInst))
    {
        FillWithSubstitues(m_TypedMemoryFences);
    }
    else if (IsUrbFenceOperation(pReferenceInst))
    {
        FillWithSubstitues(m_UrbMemoryFences);
    }
    else if (IsLscFenceOperation(pReferenceInst))
    {
        FillWithSubstitues(m_LscMemoryFences);
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
    else if (IsLscFenceOperation(pEvaluatedInst) && IsLscFenceOperation(pReferenceInst))
    {
        IGC_ASSERT(m_HasUrbFenceFunctionality);
        IGC_ASSERT(m_HasIndependentSharedMemoryFenceFunctionality);
        bool isDuplicate = GetLscMem(pEvaluatedInst) == GetLscMem(pReferenceInst);
        isDuplicate &= GetLscScope(pEvaluatedInst) >= GetLscScope(pReferenceInst);
        LSC_FENCE_OP opEvaluated = GetLscFenceOp(pEvaluatedInst);
        LSC_FENCE_OP opReference = GetLscFenceOp(pReferenceInst);
        bool referenceIsOpNone =
            opReference == LSC_FENCE_OP_NONE ||
            m_InvalidationFunctionalityRedundancies.find(pReferenceInst) != m_InvalidationFunctionalityRedundancies.end();
        bool evaluatedIsOpNone =
            opEvaluated == LSC_FENCE_OP_NONE ||
            m_InvalidationFunctionalityRedundancies.find(pEvaluatedInst) != m_InvalidationFunctionalityRedundancies.end();
        // Current implementation allows replacing the reference LSC fence with
        // the evaluated LSC fence if any of the following conditions is true:
        // 1.) both fences have the same operation type
        // 2.) reference fence is a .none operation
        // 3.) reference fence is an .invalidate and evaluated fence is
        //     an .invalidate or an .evict
        bool cond1 = opReference == opEvaluated;
        bool cond2 = referenceIsOpNone;
        bool cond3 = !evaluatedIsOpNone &&
            opReference == LSC_FENCE_OP_INVALIDATE &&
            (opEvaluated == LSC_FENCE_OP_INVALIDATE || opEvaluated == LSC_FENCE_OP_EVICT);
        isDuplicate &= (cond1 || cond2 || cond3);

        return isDuplicate;
    }
    else if (IsTypedMemoryFenceOperation(pEvaluatedInst) &&
        IsTypedMemoryFenceOperation(pReferenceInst))
    {
        bool isDuplicate = IsTypedMemoryFenceOperationWithInvalidationFunctionality(pEvaluatedInst) ||
            !IsTypedMemoryFenceOperationWithInvalidationFunctionality(pReferenceInst);
        return isDuplicate;
    }
    else if (IsUrbFenceOperation(pEvaluatedInst) &&
        IsUrbFenceOperation(pReferenceInst))
    {
        return true;
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
            else if (IsTypedMemoryFenceOperation(&inst))
            {
                m_TypedMemoryFences.push_back(&inst);
            }
            else if (IsUrbFenceOperation(&inst))
            {
                m_UrbMemoryFences.push_back(&inst);
            }
            else if (IsLscFenceOperation(&inst))
            {
                m_LscMemoryFences.push_back(&inst);
            }
            else if (InstructionMask memoryInstructionMask = GetInstructionMask(&inst);
                memoryInstructionMask != InstructionMask::None)
            {
                m_GlobalMemoryInstructionMask = static_cast<InstructionMask>(m_GlobalMemoryInstructionMask | memoryInstructionMask);
                InstructionMask pointerMemoryInstructionMask = GetAtomicInstructionMaskFromPointer(&inst);
                m_GlobalMemoryInstructionMask = static_cast<InstructionMask>(m_GlobalMemoryInstructionMask | pointerMemoryInstructionMask);
            }
        }
    }
    if (!m_LscMemoryFences.empty())
    {
        // Current implementation does not expect a mix of legacy memory fences
        // and LSC fences.
        IGC_ASSERT(m_UntypedMemoryFences.empty());
        IGC_ASSERT(m_TypedMemoryFences.empty());
    }
}

////////////////////////////////////////////////////////////////////////
void SynchronizationObjectCoalescingAnalysis::InvalidateMembers()
{
    m_UntypedMemoryFences.clear();
    m_ThreadGroupBarriers.clear();
    m_TypedMemoryFences.clear();
    m_UrbMemoryFences.clear();
    m_LscMemoryFences.clear();

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
        IsTypedMemoryFenceOperation(pInst) ||
        IsUrbFenceOperation(pInst) ||
        IsLscFenceOperation(pInst) ||
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
        case llvm::GenISAIntrinsic::GenISA_OutputTaskDataInput:
        case llvm::GenISAIntrinsic::GenISA_OutputMeshPrimitiveDataInput:
        case llvm::GenISAIntrinsic::GenISA_OutputMeshVertexDataInput:
        case llvm::GenISAIntrinsic::GenISA_OutputMeshSivDataInput:
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
bool SynchronizationObjectCoalescingAnalysis::IsTypedMemoryFenceOperation(const llvm::Instruction* pInst)
{
    if (llvm::isa<llvm::GenIntrinsicInst>(pInst))
    {
        const llvm::GenIntrinsicInst* pGenIntrinsicInst = llvm::cast<llvm::GenIntrinsicInst>(pInst);

        switch (pGenIntrinsicInst->getIntrinsicID())
        {
        case llvm::GenISAIntrinsic::GenISA_typedmemoryfence:
            return true;
        default:
            break;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescingAnalysis::IsTypedMemoryFenceOperationWithInvalidationFunctionality(const llvm::Instruction* pInst) const
{
    constexpr uint32_t L1CacheInvalidateArg = 0;
    return IsTypedMemoryFenceOperation(pInst) &&
        llvm::cast<llvm::ConstantInt>(pInst->getOperand(L1CacheInvalidateArg))->getValue().getBoolValue() &&
        m_InvalidationFunctionalityRedundancies.find(pInst) == m_InvalidationFunctionalityRedundancies.end();
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescingAnalysis::IsUrbFenceOperation(const llvm::Instruction* pInst)
{
    if (llvm::isa<llvm::GenIntrinsicInst>(pInst))
    {
        const llvm::GenIntrinsicInst* pGenIntrinsicInst = llvm::cast<llvm::GenIntrinsicInst>(pInst);

        switch (pGenIntrinsicInst->getIntrinsicID())
        {
        case llvm::GenISAIntrinsic::GenISA_urbfence:
            return true;
        default:
            break;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
static inline bool IsLscFenceOperation(const Instruction* pInst)
{
    const GenIntrinsicInst* pIntr = dyn_cast<GenIntrinsicInst>(pInst);
    if (pIntr && pIntr->isGenIntrinsic(GenISAIntrinsic::GenISA_LSCFence))
    {
        return true;
    }
    return false;
}

static bool IsAsyncRaytracingOperation(const Instruction* pInst)
{
    if (auto *GII = dyn_cast<GenIntrinsicInst>(pInst))
    {
        switch (GII->getIntrinsicID())
        {
        case GenISAIntrinsic::GenISA_TraceRayAsync:
        case GenISAIntrinsic::GenISA_BindlessThreadDispatch:
            return true;
        default:
            break;
        }
    }

    return false;
}

static bool IsSyncRaytracingOperation(const Instruction* pInst)
{
    if (auto *GII = dyn_cast<GenIntrinsicInst>(pInst))
    {
        switch (GII->getIntrinsicID())
        {
        case GenISAIntrinsic::GenISA_TraceRaySync:
            return true;
        default:
            break;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
static inline LSC_SFID GetLscMem(const Instruction* pInst)
{
    IGC_ASSERT(IsLscFenceOperation(pInst));
    return getImmValueEnum<LSC_SFID>(pInst->getOperand(0));
}

////////////////////////////////////////////////////////////////////////////////
static inline LSC_SCOPE GetLscScope(const Instruction* pInst)
{
    IGC_ASSERT(IsLscFenceOperation(pInst));
    return getImmValueEnum<LSC_SCOPE>(pInst->getOperand(1));
}

////////////////////////////////////////////////////////////////////////////////
static inline LSC_FENCE_OP GetLscFenceOp(const Instruction* pInst)
{
    IGC_ASSERT(IsLscFenceOperation(pInst));
    return getImmValueEnum<LSC_FENCE_OP>(pInst->getOperand(2));
}
////////////////////////////////////////////////////////////////////////
InstructionMask SynchronizationObjectCoalescingAnalysis::GetInstructionMask(const llvm::Instruction* pInst) const
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
    else if (IsAsyncRaytracingOperation(pInst))
    {
        return AllNoAtomicMask;
    }
    else if (IsSyncRaytracingOperation(pInst))
    {
        return InstructionMask::BufferReadOperation |
               InstructionMask::BufferWriteOperation;
    }

    return InstructionMask::None;
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescingAnalysis::IsFenceOperation(const llvm::Instruction* pInst)
{
    return
        IsTypedMemoryFenceOperation(pInst) ||
        IsUrbFenceOperation(pInst) ||
        IsLscFenceOperation(pInst) ||
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
        case llvm::GenISAIntrinsic::GenISA_LSCFence:
        {
            LSC_SFID mem = GetLscMem(pGenIntrinsicInst);
            LSC_FENCE_OP op = GetLscFenceOp(pGenIntrinsicInst);
            IGC_ASSERT(mem == LSC_TGM || mem == LSC_UGM);
            IGC_ASSERT(op == LSC_FENCE_OP_INVALIDATE);
            constexpr uint32_t fenceOpArg = 2;
            llvm::Type* type = pGenIntrinsicInst->getOperand(fenceOpArg)->getType();
            pGenIntrinsicInst->setOperand(
                fenceOpArg,
                llvm::ConstantInt::get(type, LSC_FENCE_OP_NONE));
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
