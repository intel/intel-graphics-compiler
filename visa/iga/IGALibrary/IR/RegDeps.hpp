/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGA_REGDEPS_HPP
#define _IGA_REGDEPS_HPP

#include "BitSet.hpp"
#include "Instruction.hpp"
#include "Kernel.hpp" // BlockList TODO: move to Blocks.hpp (elide Encoder def)

#include <ostream>
#include <vector>
#include <map>
#include <memory>
#include <set>

namespace iga
{

enum class DEP_TYPE
{
    NONE,
    READ,
    WRITE,
    WRITE_ALWAYS_INTERFERE,
    READ_ALWAYS_INTERFERE
};

enum class DEP_PIPE
{
    NONE,
    // TGL
    SHORT,
    LONG,
    CONTROL_FLOW, // TGL only
    SEND,
    MATH,
    // XeHP+
    FLOAT,
    INTEGER,
    LONG64,
    DPAS,
    SEND_SLM,     // XeHPG LSC SLM message
    SEND_UNKNOWN, // XeHPG LSC desc is indirect, not sure if it's SLM
    MATH_INORDER // XeHPC
};

enum class DEP_CLASS
{
    NONE,
    IN_ORDER,
    OUT_OF_ORDER,
    OTHER
};
struct SBID
{
    uint32_t sbid = 0;
    DEP_TYPE dType = DEP_TYPE::NONE;
    bool isFree = true;

    SBID() {}

    SBID(uint32_t id, bool isfree, DEP_TYPE type)
        : sbid(id), dType(type), isFree(isfree) { }

    void reset()
    {
        sbid = 0;
        dType = DEP_TYPE::NONE;
        isFree = true;
    }
};

class DepSetBuilder;

// A sort of bit set for representing
// BITs per 32-byte GRF register
//    1 would be register granularity only
//    8 would be DWORD granularity
class DepSet {
    friend DepSetBuilder;

  public:
    struct InstIDs {
    public:
        InstIDs() {}
        InstIDs(uint32_t global_id,
                uint32_t in_order_id)
            : global(global_id),
              inOrder(in_order_id)
        {}

        InstIDs(uint32_t global_id,
                uint32_t in_order_id,
                uint32_t float_pipe_id,
                uint32_t int_pipe_id,
                uint32_t long_pipe_id,
                uint32_t math_pipe_id)
          : global(global_id),
            inOrder(in_order_id),
            floatPipe(float_pipe_id),
            intPipe(int_pipe_id),
            longPipe(long_pipe_id),
            mathPipe(math_pipe_id)
        { }

        // unique id for all instructions
        uint32_t global = 0;
        // id counter for all in-order instructions
        uint32_t inOrder = 0;
        // id coutner for float pipe
        uint32_t floatPipe = 0;
        // id counter for short pipe
        uint32_t intPipe = 0;
        // id counter for Long pipe
        uint32_t longPipe = 0;
        // id counter for in-order math pipe
        uint32_t mathPipe = 0;
    };

    typedef std::pair<uint32_t, uint32_t> RegRangeType;
    typedef std::vector<RegRangeType> RegRangeListType;

  private:
    DepSet(const InstIDs& inst_id_counter, const DepSetBuilder& dsb);
    ~DepSet()
    {
        delete bits;
    }
    DepSet(const DepSet& ds) = delete;
    DepSet& operator=(DepSet const&) = delete;

public:
    // FIXME: shold be moved to DepSetBuilder
    uint32_t addressOf(RegName rn, const RegRef &rr, uint32_t typeSizeBytes) const;
    bool isRegTracked(RegName rn) const;

    void setDstRegion(
        RegName rn,
        RegRef rr,
        Region r,
        uint32_t execSize,
        uint32_t typeSizeBits);
    void setSrcRegion(
        RegName rn,
        RegRef rr,
        Region r,
        uint32_t execSize,
        uint32_t typeSizeBits);

    void setDepType(DEP_TYPE type){ m_dType = type; }
    void setHasIndirect() { m_hasIndirect = true; }
    void setHasSR() { m_hasSR = true; }

    void addGrf(size_t reg);
    void addGrfBytes(size_t reg, size_t subRegBytes, size_t bytes);
    void addA_W(RegRef rr) { addABytes(rr.regNum, 2* (size_t)rr.subRegNum, 2); }
    void addA_D(RegRef rr) { addABytes(rr.regNum, 4* (size_t)rr.subRegNum, 4); }
    void addABytes(size_t reg, size_t subregBytes, size_t bytes);
    void addFBytes(size_t fByteOff, size_t bytes);
    void addToBucket(uint32_t regNum){ m_bucketList.push_back(regNum); }
    void setDepPipe(DEP_PIPE pipe) { m_dPipe = pipe; }
    void setDepClass(DEP_CLASS cls) { m_dClass = cls; }
    void setSBID(SBID &sw) { m_sbid = sw; }

    bool                    empty()                         const { return bits->empty(); }
    void                    reset() { bits->reset(); }
    bool                    intersects(const DepSet &rhs)   const { return bits->intersects(*rhs.bits); }
    DEP_TYPE                getDepType()                    const { return m_dType; }
    bool                    hasIndirect()                   const { return m_hasIndirect; }
    bool                    hasSR()                         const { return m_hasSR; }
    const Instruction*      getInstruction()                const { return m_instruction; }
    const InstIDs&          getInstIDs()                    const { return m_InstIDs; }
    size_t                  getInstGlobalID()               const { return m_InstIDs.global; }

    DEP_PIPE                getDepPipe()                    const { return m_dPipe; }
    DEP_CLASS               getDepClass()                   const { return m_dClass; }
    SBID                    getSBID()                       const { return m_sbid; }
    void                    str(std::ostream &os) const;
    std::string             str() const;
    bool                    destructiveSubtract(const DepSet &rhs);

    const std::vector<size_t>&  getBuckets() const { return m_bucketList; }
    const BitSet<>&   getBitSet() const { return *bits; }
    BitSet<>&         getBitSetVol() { return *bits; }

    const DepSet* getCompanion() const { return m_companion; }
    DepSet* getCompanion() { return m_companion; }

    void setCompanion(DepSet* companion) { m_companion = companion; }

private:
    void setInputsFlagDep();
    void setInputsSrcDep();

    void setOutputsFlagDep();
    void setOutputsDstDep();
    // set the register footpring on full registers for all register
    // the given dst has touched. For example, consider the entire r2 as
    // its footpring for a dst register "r2.2:d"
    // This is a helper setter for some Workarounds
    void setOutputsDstDepFullGrf();

    // Set the bits to this DepSet with the given reg_range
    void addDependency(const RegRangeType& reg_range);
    void addDependency(const RegRangeListType& reg_range);

    // get the used src registers of given dpas inst
    // Return: in reg_range each pair denotes start and upper grf reg number of each src
    // extra_regs - the extra register footprint required by HW workaround: treat Src2 as dpas.8x8
    //              when calculating register footpring. extra_regs only affect external dependency
    //              and will not apply to calculating dpas/macro internal dependency
    void getDpasSrcDependency(const Instruction &inst, RegRangeListType& reg_range,
        RegRangeListType& extra_regs, const Model& model);

    // get the used dst registers of given dpas inst
    // Return: in reg_range denotes the start and upper grf reg number
    void getDpasDstDependency(const Instruction &inst, RegRangeType& reg_range);

    // helper function to get dpas src register footpring upper bound
    uint32_t getDPASSrcDepUpBound(unsigned idx, Type srcType, uint32_t execSize, uint32_t lowBound,
        uint32_t systolicDepth, uint32_t repeatCount, uint32_t opsPerChan);
    // helper function to get dpas OPS_PER_CHAN
    uint32_t getDPASOpsPerChan(Type src1_ty, Type src2_ty);
private:
    const Instruction* m_instruction;

    // track the inst id counters when it reach to this instruction
    const InstIDs m_InstIDs;

    const DepSetBuilder& m_DB;

    DEP_TYPE m_dType;
    DEP_PIPE m_dPipe;
    DEP_CLASS m_dClass;
    BitSet<>* bits;
    std::vector<size_t> m_bucketList;
    SBID m_sbid;
    bool m_hasIndirect;
    // set true if the instruction has access the special registers: CR, CE, SR
    // In this case we cannot be sure which register it is actually affect,
    // will need to sync all pipes
    bool m_hasSR;
    void formatShortReg(
        std::ostream &os,
        bool &first,
        const char *reg_name,
        size_t reg_num,
        size_t reg_start,
        size_t reg_len) const;

    // There are always two DepSet (input and output) for an instruction. Here
    // record the compnion DepSet that create for the same instruction with this
    // DepSet. This is for the use of when we clear an in-order instruction's
    // dependency, we'd like to clear its both input and output DepSets
    DepSet* m_companion = nullptr;
};

/// DepSetBuilder - create the DepSet, also keep track of Model dependend register info
class DepSetBuilder {
public:
    typedef DepSet::InstIDs InstIDs;

    DepSetBuilder(const Model& model)
        : GRF_REGS(model.getNumGRF()),
          GRF_BYTES_PER_REG(model.getGRFByteSize()),
          ARF_F_REGS(model.getNumFlagReg()),
          mPlatformModel(model)
    {}

    ~DepSetBuilder()
    {
        for (auto ds : mAllDepSet)
            delete ds;
    }

public:
    // DepSet creater
    /// createSrcDepSet - create DepSet for src operands of instruction i
    DepSet* createSrcDepSet(const Instruction &i, const InstIDs& inst_id_counter,
        SWSB_ENCODE_MODE enc_mode);
    /// createDstDepSet - create DepSet for dst operands of instruction i
    DepSet* createDstDepSet(const Instruction &i, const InstIDs& inst_id_counter,
        SWSB_ENCODE_MODE enc_mode);

    /// createSrcDstDepSetForDpas - Find the DPAS macro and set the dependency for input and output
    /// return the created DepSets for input(src) and output(dst)
    std::pair<DepSet*, DepSet*> createDPASSrcDstDepSet(
        const InstList& insList, InstListIterator instIt, const InstIDs& inst_id_counter,
        size_t& dpasCnt, SWSB_ENCODE_MODE enc_mode);

    /// mathDstWA - this will return the DepSet with math's dst region, and force to occupy the
    /// entire registers no matter what the region and channel are. e.g. if dst is r1.3, it'll
    /// occupy the entire r1
    /// @input setFlagDep - True if this is a full DepSet creation and would like to set the dep
    ///                     for flag registers. False if the DepSet is an additional dependecny for
    ///                     an instruction which already has DepSet created (e.g. for MathWA)
    /// This is the WA to fix the HW read suppression issue
    DepSet* createDstDepSetFullGrf(const Instruction &i, const InstIDs& inst_id_counter,
        SWSB_ENCODE_MODE enc_mode, bool setFlagDep);


    // Register File Size Info
    uint32_t getGRF_REGS()                  const { return GRF_REGS; }
    uint32_t getGRF_BYTES_PER_REG()         const { return GRF_BYTES_PER_REG; }
    uint32_t getGRF_LEN()                   const { return GRF_REGS * GRF_BYTES_PER_REG; }

    uint32_t getARF_A_BYTES_PER_REG()       const { return ARF_A_BYTES_PER_REG; }
    uint32_t getARF_A_REGS()                const { return ARF_A_REGS; }
    uint32_t getARF_A_LEN()                 const { return ARF_A_REGS * ARF_A_BYTES_PER_REG; }

    uint32_t getARF_ACC_REGS()              const { return ARF_ACC_REGS; }
    uint32_t getARF_ACC_BYTES_PER_REG()     const { return ARF_ACC_BYTES_PER_REG; }
    uint32_t getARF_ACC_LEN()               const { return ARF_ACC_REGS * ARF_ACC_BYTES_PER_REG; }

    uint32_t getARF_F_REGS()                const { return ARF_F_REGS; }
    uint32_t getARF_F_BYTES_PER_REG()       const { return ARF_F_BYTES_PER_REG; }
    uint32_t getARF_F_LEN()                 const { return ARF_F_REGS * ARF_F_BYTES_PER_REG; }

    uint32_t getARF_SPECIAL_REGS()          const { return ARF_SPECIAL_REGS; }
    uint32_t getARF_SPECIAL_BYTES_PER_REG() const { return ARF_SPECIAL_BYTES_PER_REG; }
    uint32_t getARF_SPECIAL_LEN()           const { return ARF_SPECIAL_REGS * ARF_SPECIAL_BYTES_PER_REG; }

    uint32_t getGRF_START()                 const { return 0; }
    uint32_t getARF_A_START()               const { return ALIGN_UP_TO(32, getGRF_START() + getGRF_LEN()); }
    uint32_t getARF_ACC_START()             const { return ALIGN_UP_TO(32, getARF_A_START() + getARF_A_LEN()); }
    uint32_t getARF_F_START()               const { return ALIGN_UP_TO(32, getARF_ACC_START() + getARF_ACC_LEN()); }
    uint32_t getARF_SPECIAL_START()         const { return ALIGN_UP_TO(32, getARF_F_START() + getARF_F_LEN()); }
    uint32_t getTOTAL_END()                 const { return ALIGN_UP_TO(32, getARF_SPECIAL_START() + getARF_SPECIAL_LEN()); }

    uint32_t getTOTAL_BITS()                const { return getTOTAL_END(); }
    uint32_t getBYTES_PER_BUCKET()          const { return getGRF_BYTES_PER_REG(); }
    uint32_t getTOTAL_BUCKETS()             const { return (getTOTAL_BITS() / getBYTES_PER_BUCKET()) + 1; }

    uint32_t getBucketStart(RegName regname) const
    {
        uint32_t bucket = 0;
        switch (regname)
        {
        case iga::RegName::GRF_R:
            bucket = getGRF_START() / getBYTES_PER_BUCKET();
            break;
        case iga::RegName::ARF_A:
            bucket = getARF_A_START() / getBYTES_PER_BUCKET();
            break;
        case iga::RegName::ARF_ACC:
            bucket = getARF_ACC_START() / getBYTES_PER_BUCKET();
            break;
        case iga::RegName::ARF_F:
            bucket = getARF_F_START() / getBYTES_PER_BUCKET();
            break;
        case RegName::ARF_CR:
        case RegName::ARF_SR:
            bucket = getARF_SPECIAL_START() / getBYTES_PER_BUCKET();
            break;
        default:
            //putting rest of archtecture registers in to same bucket
            bucket = getARF_F_START() / 32;
            break;
        }
        return bucket;
    }

private:
    // ASSUMES: byte-level tracking for all elements

    // FIXME: Some info taken from model, some are hardcoded
    const uint32_t GRF_REGS;
    const uint32_t GRF_BYTES_PER_REG;

    const uint32_t ARF_A_BYTES_PER_REG = 32;
    const uint32_t ARF_A_REGS = 1;

    const uint32_t ARF_ACC_REGS = 12;
    const uint32_t ARF_ACC_BYTES_PER_REG = 32;

    const uint32_t ARF_F_REGS;
    const uint32_t ARF_F_BYTES_PER_REG = 4;

    //for registers that migh thave indirect dependence like CR and SR
    const uint32_t ARF_SPECIAL_REGS = 2;
    const uint32_t ARF_SPECIAL_BYTES_PER_REG = 4;

private:
    // DpasMacroBuilder - a helper class for building dpas macro

    // DPAS sequence can form a macro by setting {Atomic} to all DPAS in the same macro, except the last one
    // DPAS can form a macro when:
    // 1. consecutive DPAS instructions of the same opcode
    // 2. same datatype of the same operand across all instructions
    // 3. same execution mask across all instructions
    // 4. same depth
    // 5. has no internal dependency within the instruction
    //    with an exception that for depth 8 dpas, src0and dst dependency is allowed if they are completely the same
    // 6. One of below conditions is met:
    //    a) src1 read suppression is fulfilled
    //    b) src2 read suppression is fulfilled
    class DpasMacroBuilder {
    public:
        DpasMacroBuilder(
            const DepSetBuilder &dsBuilder,
            const Model &model,
            const InstList &instList,
            InstListIterator dpasIt,
            DepSet& in,
            DepSet& out)
            : m_dsBuilder(dsBuilder), m_model(model), m_instList(instList), m_firstDpasIt(dpasIt),
              m_inps(in), m_oups(out)
        {}

        // form a macro start from dpasIt, and set the register footprint of all
        // instruction in the macro into the given input and output DepSet
        // - dpasCnt is the output of number of instructions in the macro
        // - return the last intruction in the formed macro
        const Instruction& formMacro(size_t &dpasCnt);

    private:
        typedef DepSet::RegRangeListType RegRangeListType;
        typedef RegRangeListType SrcRegRangeType;
        typedef DepSet::RegRangeType RegRangeType;
        typedef RegRangeType DstRegRangeType;


        // check if the given next_inst can be the candidate within current macro
        // return true if it can't
        bool nextIsNotMacroCandidate(const Instruction &dpas, const Instruction &next_inst) const;

        // set register fange from start_reg to upper_reg into bit_set
        void setBits(BitSet<> &bit_set, uint32_t start_reg, uint32_t upper_reg) const;

        // set dst_range to dst_bits and src_range to src_bits
        void setDstSrcBits(
            const SrcRegRangeType &src_range,
            const DstRegRangeType &dst_range,
            BitSet<> &src_bits,
            BitSet<> &dst_bits) const;

        // check if the given register ranges having intersection
        bool hasIntersect(const DepSet::RegRangeType &rr1, const DepSet::RegRangeType &rr2) const;

        // If rr1 and rr2 footprint are all the same, return true.
        // If rr1 and rr2 has intersect but not entirely the same, then return
        // false. If no dependency, return true
        bool hasEntireOverlapOrNoOverlap(const DepSet::RegRangeType &rr1,
                                         const DepSet::RegRangeType &rr2) const;

        // check if the instruction having internal dependency
        // Instruction having internal dependency on dst to src is not allowed
        // to be in a macro. Only for dpas8x8, insternal dep on dst and src0 is
        // allowed, but only when src0 and dst memory footprint is entirely the
        // same
        bool hasInternalDep(const DstRegRangeType &dst_range,
                            const SrcRegRangeType &src_range,
                            bool isDepth8) const;

        // check if the givem src/dst RegRange has producer-consumer dependency
        // (WAW/WAR/RWA) to the given src/dst BitSet
        bool hasProducerConsumerDep(const DstRegRangeType& dst_range,
                                    const SrcRegRangeType& src_range,
                                    const BitSet<>& target_dst_bits,
                                    const BitSet<>& target_src_bits) const;


        void updateRegFootprintsToDepSets(
            SrcRegRangeType& src_range, SrcRegRangeType& extra_src_range, DstRegRangeType& dst_range);
        void updateRegFootprintsToDepSets(
            RegRangeListType& src_range, RegRangeListType& extra_src_range, RegRangeListType& dst_range);

    private:
        struct SuppressBlock {
        public:
            // Maximum number of groups allowed. This dpeneds on platform and the src index
            const size_t maxNumGroup;
            const size_t groupSize;

            // Groups in this block: each group is represented by the first register of the group
            std::vector<uint16_t> groups;

            // Keep tarck of the src and dst register footprints of all instructions those
            // are in this suppressBlcok. This will be used to set to DepSet when the block
            // is decided to be added into the macro, for avoiding re-calculating the dependency
            RegRangeListType allSrcRange, allExtraSrcRange;
            RegRangeListType allDstRange;

        public:
            SuppressBlock(size_t maxNumGroup, size_t groupSize)
                : maxNumGroup(maxNumGroup), groupSize(groupSize)
            {}

            bool isFull() const
            {
                return groups.size() >= maxNumGroup;
            }

            bool contains(uint16_t startRegNum) const
            {
                return std::find(groups.begin(), groups.end(), startRegNum) != groups.end();
            }

            // check if the given register range has partially overlapped with existed ones
            // return true when there is partially overlapped
            // retur false if there is completely overlapped or no overlapped
            bool partialOverlapped(uint16_t startRegNum) const
            {
                if (contains(startRegNum))
                    return false;
                for (auto& group : groups) {
                    // number of registers in a group must be all the same across the groups
                    // in the same block
                    uint16_t diff = startRegNum > group ?
                                        startRegNum - group : group - startRegNum;
                    if (diff < groupSize)
                        return true;
                }
                return false;
            }

            size_t size () const { return groups.size(); }

            // add the registers into this blcok,
            void addRegs(uint16_t startRegNum)
            {
                assert(!isFull());
                assert(!contains(startRegNum));
                groups.push_back(startRegNum);
            }

            void addRegRanges(SrcRegRangeType& srcRange,
                              SrcRegRangeType& extraSrcRange,
                              DstRegRangeType& dstRange)
            {
                allSrcRange.insert(allSrcRange.end(), srcRange.begin(), srcRange.end());
                allExtraSrcRange.insert(allExtraSrcRange.end(), extraSrcRange.begin(), extraSrcRange.end());
                allDstRange.push_back(dstRange);
            }

        }; // SuppressionBlock
        typedef std::unique_ptr<SuppressBlock> SuppressBlockPtrTy;

        // get the max number of suppression groups according to srcIdx and platform
        size_t getNumberOfSuppresionGroups(uint32_t srcIdx) const;

        // check from startIt, find the number of consecutive dpas those can be grouped in a macro due
        // to srcIdx suppression. Return number of instructions found
        size_t formSrcSuppressionBlock(InstListIterator startIt, uint32_t srcIdx);

        // return the candidate SuppressBlock that is found fulfilling read suppression requirement
        // of given src index, start from the give instruction. This block is the first candidate block
        // of instructions register those can be suppressed. Will need to check if the following instructions
        // having the same registers so that they can actually being suppressed.
        // return nullptr if there is no chance to suppress the given src
        // allDstBits, allSrcBits - all used grf bits in the return suppressBlock
        // allDstNoLastBits, allSrcNoLastBits - all used grf in the return suppressBlock except the
        // last instruction's
        SuppressBlockPtrTy getSuppressionBlockCandidate(
            InstListIterator startIt, uint32_t srcIdx,
            BitSet<>& allDstBits, BitSet<>& allSrcBits,
            BitSet<>& allDstNoLastBits, BitSet<>& allSrcNoLastBits) const;

        bool srcIsSuppressCandidate(const Instruction& inst, uint32_t srcIdx) const;

    private:
        const DepSetBuilder &m_dsBuilder;
        const Model &m_model;

        const InstList &m_instList;
        InstListIterator m_firstDpasIt;

        DepSet& m_inps;
        DepSet& m_oups;
    }; // DpasMacroBuilder

private:
    // Track all the created DepSet for deletion
    std::vector<DepSet*> mAllDepSet;
    const Model &mPlatformModel;
};

}
#endif // _IGA_REGDEPS_HPP
