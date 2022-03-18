/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __OPTIMIZER_H__
#define __OPTIMIZER_H__


#include "BuildIR.h"
#include "RegAlloc.h"
#include "HWConformity.h"
#include "LocalScheduler/LocalScheduler_G4IR.h"
#include "LocalScheduler/SWSB_G4IR.h"
#include <unordered_set>
#include <optional>

typedef struct _AddrSubReg_Node{
    short immAddrOff = 0;
    int subReg = 0;
    INST_LIST_ITER iter;
    bool canRemoveInst = false;
    bool canUseImmed = false;
    bool usedImmed = false;
} AddrSubReg_Node;

/**
 *  Below data structures are used for message header optimization
 */
typedef enum _HEADER_ORDER_
{
    HEADER_UNDEF         = 0,
    HEADER_FULL_REGISTER = 1,
    HEADER_X             = 2,
    HEADER_Y             = 3,
    HEADER_SIZE          = 4
} HEADER_ORDER;

namespace vISA
{
class DPASSrc2RSCache
{
public:
    std::vector<int> GRFCache;
    unsigned latestID;
    bool firstDpas;

    DPASSrc2RSCache()
    {
        latestID = 0;
        firstDpas = true;
        GRFCache.resize(16, -1);
    }
    ~DPASSrc2RSCache()
    {
    }
};
class MSGTable
{
public:
    //below instructions are to be compared against the current send
    G4_INST * send;     // the reference send
    G4_INST * a0Dot0;   // def of a0.0
    G4_INST * m;        // def of m
    G4_INST * mDot0;    // def of m.0: X
    G4_INST * mDot1;    // def of m.1: Y
    G4_INST * mDot2;    // def of m.2: size

    INST_LIST_ITER a0Dot0_it;
    INST_LIST_ITER m_it;
    INST_LIST_ITER mDot0_it;
    INST_LIST_ITER mDot1_it;
    INST_LIST_ITER mDot2_it;

    bool invalid = false;
    bool opt;             // if the catched send is used

    //below shows whether there are new defs to determine reuse or remove
    //if redundant, then remove; otherwise, reuse the catched header
    bool isXRedef;      // X is used to define m.0
    bool isYRedef;      // Y is used to define m.1
    bool isSizeRedef;   // Size is used to define m.2
    bool isR0Dot0Redef; // r0.0 is used to define m
    HEADER_ORDER first;

    void insertHeaderMovInst(G4_INST *, IR_Builder&, G4_BB *);
    void reusePreviousHeader(G4_INST *, G4_INST *, G4_INST *, IR_Builder&);

    ~MSGTable() {};

};  // to remove redundant message headers
}
typedef std::list<vISA::MSGTable*> MSGTableList;
typedef std::list<vISA::MSGTable*>::iterator MSGTable_ITER;

#define MESSAGE_HEADER_THRESHOLD 1

typedef struct _DEFA0
{
    vISA::G4_INST *pred = nullptr;
    vISA::G4_INST *curr = nullptr;
    INST_LIST_ITER predIt;
    INST_LIST_ITER currIt;
    bool isA0Redef = false;
} DEFA0;

/**
 *  end of data structures for message header optimization
 */

// auxiliary structure for inserting save and restore instructions
namespace vISA
{

class Optimizer
{
    IR_Builder& builder;
    G4_Kernel&  kernel;
    FlowGraph&  fg;

    vISA::Mem_Manager& mem;
    //
    // optimization phases
    //
    G4_SrcModifier mergeModifier(G4_Operand *def, G4_Operand *use);
    void cleanMessageHeader();
    void sendFusion();
    void renameRegister();
    void localDefHoisting();
    void reassociateConst();
    void removePartialMovs();
    void localCopyPropagation();
    void localInstCombine();
    void optimizeLogicOperation();
    void cselPeepHoleOpt();
    void regAlloc();
    void insertFallThroughJump();
    void reverseOffsetProp(
            AddrSubReg_Node addrRegInfo[8],
            int subReg,
            unsigned int srcNum,
            INST_LIST_ITER lastIter,
            INST_LIST_ITER iend
       );
    void FoldAddrImmediate();
    bool foldCmpSel(G4_BB *BB, G4_INST *selInst, INST_LIST_ITER &selInst_II);
    bool foldPseudoNot(G4_BB *bb, INST_LIST_ITER& iter);
    bool createSmov(G4_BB *bb, G4_INST* flagMove, G4_INST* nextInst);
    bool foldCmpToCondMod(G4_BB* BB, INST_LIST_ITER& iter);
    void HWWorkaround();
    void preRA_HWWorkaround();
    void postRA_HWWorkaround();
    G4_INST* evenlySplitDPASInst(INST_LIST_ITER iter, G4_BB* bb);
    bool hasDPASSourceTwoReuse(DPASSrc2RSCache* src2GRFCache, G4_INST* inst);
    void DPASWA(G4_BB* bb, INST_LIST_ITER ii, DPASSrc2RSCache* src2GRFCache);
    void normalizeRegion();
    void initializePayload();
    void dumpPayload();
    void collectStats();
    void createR0Copy();

    void fixEndIfWhileLabels();
    void mergeScalarInst();
    void HWConformityChk() { ::HWConformityChk(builder, kernel, mem); }
    void removeRedundMov() { fg.removeRedundMov(); }
    void removeEmptyBlocks() { fg.removeEmptyBlocks(); }
    void reassignBlockIDs() { fg.reassignBlockIDs(); }
    void evalAddrExp() { kernel.evalAddrExp(); }
    void preRA_Schedule()
    {
        if (kernel.useRegSharingHeuristics())
        {
            preRA_RegSharing Sched(kernel, mem, /*rpe*/ nullptr);
            Sched.run();
        }
        else
        {
            preRA_Scheduler Sched(kernel, mem, /*rpe*/ nullptr);
            Sched.run();
        }
    }
    void localSchedule()
    {
        LocalScheduler lSched(kernel.fg, mem);
        lSched.localScheduling();
    }

    void adjustIndirectCallOffsetAfterSWSBSet();

    // For HW debugging
    void zeroSomeARF();

    void addSWSBInfo();

    void lowerMadSequence();

    void LVN();

    void ifCvt();

    void ifCvtFCCall();

    void reRAPostSchedule();

    void dce();

    void accSubPostSchedule();

    void accSubBeforeRA();


    // return true if BuiltInR0 gets a different allocation than r0
    bool R0CopyNeeded();

private:
    /* below member functions are used for message header opt */
    bool isHeaderOptCandidate(G4_INST *, G4_INST *);
    bool isHeaderOptReuse(G4_INST *, G4_INST *);
    bool headerOptValidityCheck(MSGTable *, MSGTable *);
    bool isHeaderCachingCandidate(G4_INST *);
    void messageHeaderReport(size_t, size_t, G4_Kernel&);
    void optMessageHeaders(MSGTableList &, G4_BB* , DEFA0&);
    void addEntryToMessageTable(G4_INST *, MSGTableList &, G4_BB*, INST_LIST_ITER, DEFA0 &);
    bool chkNewDefBetweenSends(G4_INST *, MSGTableList& , DEFA0&);
    /* below member functions are used for barrier header opt */
    void removeRedundantBarrierHeaders(G4_INST *, G4_SrcRegRegion*, bool);
    bool isBarrierPattern(G4_INST *, G4_SrcRegRegion* &);
    void hoistBarrierHeaderToTop(G4_SrcRegRegion*);
    /* end of member functions for message header opt */
    void cleanupBindless();
    G4_Operand* updateSendsHeaderReuse(std::vector<std::vector<G4_INST*>> &, std::vector<G4_INST*> &, INST_LIST_ITER);
    void countGRFUsage();
    void changeMoveType();
    void split4GRFVars();
    void legalizeType();
    void analyzeMove();

    void removeInstrinsics();

    void countBankConflicts();
    unsigned int numBankConflicts;

    bool chkFwdOutputHazard(INST_LIST_ITER &, INST_LIST_ITER&);
    bool chkFwdOutputHazard(G4_INST*, INST_LIST_ITER);
    bool chkBwdOutputHazard(INST_LIST_ITER &, INST_LIST_ITER&);
    bool chkBwdOutputHazard(G4_INST *, INST_LIST_ITER&);
    bool chkBwdOutputHazard(G4_INST *, INST_LIST_ITER&, G4_INST *);
    bool chkBwdWARdep(G4_INST*, INST_LIST_ITER);
    bool chkBwdWAWdep(G4_INST*, INST_LIST_ITER);

    // various HW WA
    void addSwitchOptionToBB(G4_BB*, bool isSubroutine = false);
    void linePlaneWA(G4_INST* inst);
    void fixSendSrcRegion(G4_INST* inst);
    void clearARFDependencies();
    void clearSendDependencies();
    void loadThreadPayload();
    void addFFIDProlog();
    void insertFenceBeforeEOT();
    void insertScratchReadBeforeEOT();
    void resetA0();
    void setA0toTdrForSendc();
    void replaceRetWithJmpi();
    void doNoMaskWA();
    void newDoNoMaskWA();
    void applyFusedCallWA();
    void setDMaskFusedCallWA();
    void finishFusedCallWA();
    void doNoMaskWA_postRA();
    void newDoNoMaskWA_postRA();
    bool NoMaskWAUseRAList() const { return true; }
    void insertFenceAtEntry();
    void expandMulPostSchedule();
    void expandMadwPostSchedule();
    void fixReadSuppressioninFPU0();

    typedef std::vector<vISA::G4_INST*> InstListType;
    // create instruction sequence to calculate call offset from ip
    void expandIndirectCallWithRegTarget();
    // a helper function to create instruction sequence to replace indirect call with jmpi
    void createInstForJmpiSequence(InstListType& insts, G4_INST* fcall);
    // a hlper function to create the instructions to calculate the jump target offset,
    // return G4_Declare of the new created jmp target
    G4_Declare* createInstsForCallTargetOffset(
        InstListType& insts, G4_INST* fcall, int64_t adjust_off);
    // a helper function to create the instructions to get ip from call's dst
    // This is a WA for platforms can't support ip register
    // The give add_with_ip must be an add instruction with ip register as its src0
    void replaceIPWithCall(InstListType& insts, G4_INST* add_with_ip);

    void insertDummyMad(G4_BB* bb, INST_LIST_ITER inst_it);

    void insertDummyCsel(G4_BB* bb, INST_LIST_ITER inst_it, bool newBB);

    void insertDummyMov(G4_BB* bb, INST_LIST_ITER inst_it, G4_Operand* opnd);
    void insertDummyMovForHWRSWADPAS(G4_BB* bb);
    void insertDummyMovForHWRSWA();
    void insertHashMovs();
    void insertDummyCompactInst();
    void removeLifetimeOps();
    void recomputeBound(std::unordered_set<G4_Declare*>& declares);

    void mapOrphans();
    void varSplit();
    void cloneSampleInst();

    // helper function to find the size of cross thread data which needs to be loaded
    // loadStartOffset - in this parameter we put the offsset of first input which gets loaded.
    uint32_t findLoadedInputSize(uint32_t& loadStartOffset);

    /// Each optimization should be a member function of this class.
    /// This defines a pass type as a pointer to member function.
    typedef void (Optimizer::*PassType)();

    /// Data structure that collects information about passes.
    struct PassInfo {
        /// The pass to be executed for this kernel.
        PassType Pass;

        /// The member function name as a pass.
        const char *Name;

        /// The option that controls this pass. This might not be a one-to-one
        /// relation between pass and option. For example, multiple passes
        /// could be mapped to a single option, like vISA_EnableAlways.
        vISAOptions Option;

        /// Corresponding timer for this pass. When it is not a concrete
        /// timer i.e. TIMER_NUM_TIMERS, then no time will be recorded.
        TimerID Timer;

        PassInfo(PassType P, const char *N, vISAOptions O,
                 TimerID T = TimerID::NUM_TIMERS)
            : Pass(P), Name(N), Option(O), Timer(T) {}

        PassInfo() : Pass(0), Name(0), Option(vISA_EnableAlways),
            Timer(TimerID::NUM_TIMERS) {}
    };

    bool foldPseudoAndOr(G4_BB* bb, INST_LIST_ITER& iter);

public:
    /// Index enum for each pass in the pass array.
    enum PassIndex {
        PI_cleanMessageHeader = 0,
        PI_sendFusion,
        PI_renameRegister,
        PI_localDefHoisting,
        PI_localCopyPropagation,
        PI_localInstCombine,
        PI_removePartialMovs,
        PI_cselPeepHoleOpt,
        PI_optimizeLogicOperation,
        PI_HWConformityChk,            // always
        PI_preRA_HWWorkaround,         // always, each WA under specific control
        PI_postRA_HWWorkaround,        // always, each WA under specific control
        PI_preRA_Schedule,
        PI_regAlloc,                   // always
        PI_removeLifetimeOps,          // always
        PI_countBankConflicts,
        PI_removeRedundMov,            // always
        PI_removeEmptyBlocks,          // always
        PI_insertFallThroughJump,      // always
        PI_reassignBlockIDs,           // always
        PI_evalAddrExp,                // always
        PI_FoldAddrImmediate,
        PI_localSchedule,
        PI_HWWorkaround,               // always
        PI_fixEndIfWhileLabels,           // always
        PI_insertHashMovs,
        PI_insertDummyMovForHWRSWA,
        PI_insertDummyCompactInst,
        PI_mergeScalarInst,
        PI_lowerMadSequence,
        PI_LVN,
        PI_ifCvt,
        PI_normalizeRegion,            // always
        PI_dumpPayload,
        PI_collectStats,          // always
        PI_createR0Copy,
        PI_initializePayload,
        PI_cleanupBindless,
        PI_countGRFUsage,
        PI_changeMoveType,
        PI_reRAPostSchedule,
        PI_accSubBeforeRA,
        PI_accSubPostSchedule,
        PI_dce,
        PI_reassociateConst,
        PI_split4GRFVars,
        PI_loadThreadPayload,
        PI_addFFIDProlog,
        PI_insertFenceBeforeEOT,
        PI_insertScratchReadBeforeEOT,
        PI_mapOrphans,
        PI_varSplit,
        PI_legalizeType,
        PI_analyzeMove,
        PI_removeInstrinsics,
        PI_expandMulPostSchedule,
        PI_zeroSomeARF,
        PI_addSWSBInfo,
        PI_expandMadwPostSchedule,
        PI_NUM_PASSES
    };

private:
    /// Array of passes registered.
    PassInfo Passes[PI_NUM_PASSES];

    // indicates whether RA has failed
    bool RAFail;

    /// Initialize all passes during the construction.
    void initOptimizations();

    /// Common interface to execute a pass.
    void runPass(PassIndex Index);

    bool isCopyPropProfitable(G4_INST* movInst) const;

    std::optional<INST_LIST_ITER> findFenceCommitPos(INST_LIST_ITER fence, G4_BB* bb) const;

    bool addFenceCommit(INST_LIST_ITER iter, G4_BB* bb, bool scheduleFenceCommit);

public:
    Optimizer(vISA::Mem_Manager& m, IR_Builder& b, G4_Kernel& k, FlowGraph& f) :
        builder(b), kernel(k), fg(f), mem(m), RAFail(false)
    {
        numBankConflicts = 0;
        initOptimizations();
    }
    int optimization();

};

}

#endif // __OPTIMIZER_H__
