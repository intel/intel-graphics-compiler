/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CISACodeGen/CISACodeGen.h"
#include "Compiler/CISACodeGen/LivenessAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SparseBitVector.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

namespace IGC
{
    enum RegClass : uint8_t {
        REGISTER_CLASS_GRF = 0,
        REGISTER_CLASS_FLAG = 1,

        REGISTER_CLASS_TOTAL = 2   // GRF only for now
    };

    // various constants
    enum {
        GRF_TOTAL_NUM = 128,       // total number per thread
        GRF_NUM_THRESHOLD = 50,    // used to see if register pressure is high
        GRF_SIZE_IN_BYTE = 32,
        DWORD_SIZE_IN_BYTE = 4,
        FLAG_TOTAL_NUM = 4,
        FLAG_TOTAL_NUM_SIMD32 = 2
    };

    // Register Use info
    struct RegUse
    {
        RegClass rClass;
        //uint16_t nregs_simd8;
        uint16_t nregs_simd16;
        uint16_t uniformInBytes;

        RegUse() :
            rClass(REGISTER_CLASS_GRF),
            nregs_simd16(0), uniformInBytes(0)
        {}

        RegUse(const RegUse& rhs) :
            rClass(rhs.rClass),
            nregs_simd16(rhs.nregs_simd16),
            uniformInBytes(rhs.uniformInBytes)
        {}

        RegUse& operator += (const RegUse& rhs)
        {
            nregs_simd16 += rhs.nregs_simd16;
            uniformInBytes += rhs.uniformInBytes;
            return *this;
        }

        RegUse& operator -= (const RegUse& rhs)
        {
            if (nregs_simd16 > rhs.nregs_simd16)
            {
                nregs_simd16 -= rhs.nregs_simd16;
            }
            else
            {
                nregs_simd16 = 0;
            }
            if (uniformInBytes > rhs.uniformInBytes)
            {
                uniformInBytes -= rhs.uniformInBytes;
            }
            else
            {
                uniformInBytes = 0;
            }
            return *this;
        }

        RegUse& operator = (const RegUse& rhs) {
            if (this == &rhs) {
                return *this;
            }
            rClass = rhs.rClass;
            nregs_simd16 = rhs.nregs_simd16;
            uniformInBytes = rhs.uniformInBytes;
            return *this;
        }

        bool operator < (const RegUse& rhs) const {
            uint32_t n0 = (nregs_simd16 << 5) + uniformInBytes;
            uint32_t n1 = (rhs.nregs_simd16 << 5) + rhs.uniformInBytes;
            return n0 < n1;
        }

        void clear(RegClass rc = REGISTER_CLASS_GRF) {
            rClass = rc;
            nregs_simd16 = 0;
            uniformInBytes = 0;
        }
    };

    struct RegUsage {
        RegUse allUses[REGISTER_CLASS_TOTAL];

        RegUsage() { clear(); }
        ~RegUsage() = default;

        RegUsage(const RegUsage& copy_value)
        {
            copy(copy_value);
        }

        RegUsage& operator=(const RegUsage& rhs)
        {
            copy(rhs);
            return *this;
        }

        void copy(const RegUsage& copy_value)
        {
            std::copy(copy_value.allUses, copy_value.allUses + REGISTER_CLASS_TOTAL, allUses);
        }

        void clear()
        {
            for (int i = 0; i < REGISTER_CLASS_TOTAL; ++i) {
                allUses[(RegClass)i].clear((RegClass)i);

            }
        }
    };

    class RegisterEstimator : public llvm::FunctionPass
    {
        friend class RegPressureTracker;
    public:
        typedef llvm::SmallVector<RegUse, 32>  ValueToRegUseMap;
        typedef llvm::DenseMap<llvm::Instruction*, RegUsage>   InstToRegUsageMap;
        typedef llvm::DenseMap<llvm::BasicBlock*, RegUsage> BBToRegUsageMap;

        // Either GRF or flag. FLAG value has LLVM type of i1
        static RegClass getValueRegClass(llvm::Value* V) {
            bool isBool = V->getType()->getScalarType()->isIntegerTy(1);
            return isBool ? REGISTER_CLASS_FLAG : REGISTER_CLASS_GRF;
        }

        static char ID; // Pass identification, replacement for typeid

        RegisterEstimator() :
            llvm::FunctionPass(ID),
            m_DL(nullptr),
            m_LVA(nullptr),
            m_F(nullptr),
            m_WIA(nullptr)
        {
            initializeRegisterEstimatorPass(*llvm::PassRegistry::getPassRegistry());
        }

        bool runOnFunction(llvm::Function& F) override;

        void releaseMemory() override { clear(); }

        llvm::StringRef getPassName() const override { return "RegisterEstimator"; }

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<LivenessAnalysis>();
            AU.setPreservesAll();
        }

        LivenessAnalysis* getLivenessAnalysis() const { return m_LVA; }

        RegUse estimateNumOfRegs(llvm::Value* V) const;

        // This will compute Register pressure estimates. It also saves
        // register pressure estimate per instruction if "doRPPerInst"
        // is true.
        void calculate(bool doRPEPerInst = false);

        // Once MAX register estimate of a function is computed, check
        // if there is GRF pressure.  If the number of estimated registers
        // is larger than a given threshold (threshold is selected based on
        // the result of running SA PS shaders), it would think the register
        // pressure could be high.  This is used to turn on/off register
        // pressure tracking.
        bool isGRFPressureLow(uint16_t simdsize = 32) const
        {
            return isGRFPressureLow(simdsize, m_MaxRegs);
        }

        // Return true if this function has no GRF pressure at all.
        // A quick check to see if LivenessAnalysis is needed at all.
        bool hasNoGRFPressure() const { return m_noGRFPressure; }

        uint32_t getNumLiveGRFAtInst(llvm::Instruction* I, uint16_t simdsize = 16);

        // Return the max number of GRF needed for a BB
        uint32_t getMaxLiveGRFAtBB(llvm::BasicBlock* BB, uint16_t simdsize = 16) {
            RegUsage& ruse = m_BBMaxLiveVirtRegs[BB];
            RegUse& grfuse = ruse.allUses[REGISTER_CLASS_GRF];
            return getNumRegs(grfuse, simdsize);
        }

        // Return the number of GRF needed at entry to a BB
        uint32_t getNumLiveInGRFAtBB(llvm::BasicBlock* BB, uint16_t simdsize = 16) {
            RegUsage& ruse = m_BBLiveInVirtRegs[BB];
            RegUse& grfuse = ruse.allUses[REGISTER_CLASS_GRF];
            return getNumRegs(grfuse, simdsize);
        }

        uint32_t getNumValues() const {
            return (uint32_t)m_ValueRegUses.capacity();
        }

    private:

        bool m_RPEComputed{};
        const llvm::DataLayout* m_DL;
        LivenessAnalysis* m_LVA;
        llvm::Function* m_F;
        WIAnalysis* m_WIA;   // optional

        // The number of live registers needed at each instruction
        InstToRegUsageMap m_LiveVirtRegs;

        // Max live registers for each BB
        BBToRegUsageMap m_BBMaxLiveVirtRegs;

        // The number of live-in registers for each BB
        BBToRegUsageMap m_BBLiveInVirtRegs;

        // The max registers for this function, derived from RPE computation.
        RegUsage m_MaxRegs;

        // Set it to true when the GRF needed is low even we assume all values are
        // live from the entry to the end. This is used to skip RPE calucation entirely.
        bool m_noGRFPressure{};

        // Register needed for each value. Computed once for each value.
        // Used to avoid recomputing the same value again. It is also
        // used to check if Register Estimation for each BB/each Inst
        // can be skipped completedly.
        ValueToRegUseMap m_ValueRegUses;

        // Temporary use.
        llvm::DenseMap<llvm::BasicBlock*, int> m_pBB2ID;

        void addRegUsage(RegUsage& RUsage, SBitVector& BV);

        uint32_t getNumGRF(RegUsage& rusage, uint16_t simdsize = 16) {
            RegUse& grfuse = rusage.allUses[REGISTER_CLASS_GRF];
            return getNumRegs(grfuse, simdsize);
        }

        int getNUsesInBB(llvm::Value* V, llvm::BasicBlock* BB);

        void getLiveinRegsAtBB(RegUsage& RUsage, llvm::BasicBlock* BB)
        {
            RUsage = m_BBLiveInVirtRegs[BB];
            return;
        }
        void getMaxLiveinRegsAtBB(RegUsage& RUsage, llvm::BasicBlock* BB)
        {
            RUsage = m_BBMaxLiveVirtRegs[BB];
            return;
        }

        bool isGRFPressureLow(uint16_t simdsize, const RegUsage& Regs) const
        {
            const RegUse& ruse = Regs.allUses[REGISTER_CLASS_GRF];
            return (getNumRegs(ruse, simdsize) < (uint32_t)GRF_NUM_THRESHOLD);
        }

        const RegUse* getRegUse(uint32_t valId)
        {
            return &(m_ValueRegUses[valId]);
        }

        const RegUse* getRegUse(llvm::Value* V)
        {
            ValueToIntMap::iterator II = m_LVA->ValueIds.find(V);
            if (II == m_LVA->ValueIds.end())
            {
                IGC_ASSERT_MESSAGE(0, "Value is not part of LivenessAnalysis");
                return nullptr;
            }
            uint32_t valId = II->second;
            return getRegUse(valId);
        }

        uint32_t getNumRegs(const RegUse& RUse, uint16_t simdsize) const
        {
            uint32_t uniformRegs =
                (RUse.uniformInBytes + GRF_SIZE_IN_BYTE - 1) / GRF_SIZE_IN_BYTE;
            switch (simdsize) {
            case 16:
                return RUse.nregs_simd16 + uniformRegs;
            case 32:
                return 2 * RUse.nregs_simd16 + uniformRegs;
            default:
                return (RUse.nregs_simd16 + 1) / 2 + uniformRegs;
            }
        }

        void clear()
        {
            m_LiveVirtRegs.clear();
            m_MaxRegs.clear();
            m_BBMaxLiveVirtRegs.clear();
            m_BBLiveInVirtRegs.clear();
        }

    public:
        /// print - Convert to human readable form
        void print(llvm::raw_ostream& OS, int dumpLevel);
        void print(llvm::raw_ostream& OS, llvm::BasicBlock* BB, int dumpLevl);

#if defined( _DEBUG )
        /// dump - Dump RPE info to dbgs(), used in debugger.
        void dump();
        void dump(int dumpLevel);
        void dump(llvm::BasicBlock* BB);
        void dump(llvm::BasicBlock* BB, int dumpLevel);
#endif
    };

    // This is used to track the register pressure for a given BB or a
    // a stream of instructions. It requires RegisterEstimator to
    // provide the basic register estimation functionality and liveness
    // information.
    // To use it, an object of RegPressureTracker must be created first;
    // then follow these steps:
    //   1.  the object is initialized by init(), which sets up initial
    //       live-in sets, with an empty initial instruction stream.
    //   2.  advance() to add an instruction at a time sequentially to the
    //       head of the instruction stream.
    //   3.  getCurrNumGRF() returns the number of GRFs needed at the head
    //       of the stream.
    class RegPressureTracker {
    public:
        RegPressureTracker(RegisterEstimator* RPE);

        // init() will set up the initial live-in with BB's live-in.
        // If "doMaxRegInBB" is true, it will use the max live-in in this
        // BB instead of BB's live-in, which is more accurate, but also
        // more expensive.
        void init(llvm::BasicBlock* BB, bool doMaxRegInBB = false);
        void advance(llvm::Instruction* I);

        uint32_t getCurrNumGRF(uint16_t simdsize = 16)
        {
            return m_pRPE->getNumGRF(m_RUsage, simdsize);
        }

        bool isTrackingRegPressure() const { return m_TrackRegPressure; }

    private:
        llvm::BasicBlock* m_BB;
        RegisterEstimator* m_pRPE;
        SBitVector  m_LiveOutSet;
        bool m_TrackRegPressure;

        // register usage at the head of the current instruction stream.
        RegUsage m_RUsage;

        // For a value that will be dead at the end of BB, keep the
        // number of uses within the stream so we can have more
        // accurate live information.
        llvm::DenseMap<llvm::Value*, int> m_DeadValueNumUses;
    };
}
