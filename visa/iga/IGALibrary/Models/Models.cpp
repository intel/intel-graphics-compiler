#include "Models.hpp"

// for bxml/Model operand type mappings
#define TYPE(T) \
    ENUM_BITSET_VALUE(T,uint32_t)

#include "bxml/Model7P5.hpp"
#include "bxml/Model8.hpp"
#include "bxml/Model9.hpp"
#include "bxml/Model10.hpp"
#include "bxml/Model11.hpp"
#include "../bits.hpp"
#include "../bits.hpp"
#include "../Backend/Native/MInst.hpp"

#include <sstream>
#include <iostream>

using namespace iga;



// full "constructor"
#define UNWRAP_TUPLE(...) {__VA_ARGS__}
#define IGA_REGISTER_SPEC(\
    PLAT_LO,PLAT_HI,\
    REGNAME,SYNTAX,DESCRIPTION,\
    REGNUM7_4,REGNUM_BASE,\
    ACC_GRAN,\
    NUM_REGS,NUM_BYTE_PER_REG) \
    {REGNAME,SYNTAX,DESCRIPTION,REGNUM7_4,REGNUM_BASE,PLAT_LO,PLAT_HI,ACC_GRAN,NUM_REGS,UNWRAP_TUPLE NUM_BYTE_PER_REG}
// for <= some platform (whatever our lowest platform is
#define IGA_REGISTER_SPEC_LE(PLAT_HI,REGNAME,SYNTAX,DESCRIPTION,REGNUM7_4,REGNUM_BASE,ACC_GRAN,NUM_REGS,NUM_BYTE_PER_REG) \
    IGA_REGISTER_SPEC(Platform::GEN6,PLAT_HI,REGNAME,SYNTAX,DESCRIPTION,REGNUM7_4,REGNUM_BASE,ACC_GRAN,NUM_REGS,NUM_BYTE_PER_REG)
// for >= some platform (up to the highest)
#define IGA_REGISTER_SPEC_GE(PLAT_LO,REGNAME,SYNTAX,DESCRIPTION,REGNUM7_4,REGNUM_BASE,ACC_GRAN,NUM_REGS,NUM_BYTE_PER_REG) \
    IGA_REGISTER_SPEC(PLAT_LO,Platform::GENNEXT,REGNAME,SYNTAX,DESCRIPTION,REGNUM7_4,REGNUM_BASE,ACC_GRAN,NUM_REGS,NUM_BYTE_PER_REG)
// a specification valid on all platforms
#define IGA_REGISTER_SPEC_UNIFORM(REGNAME,SYNTAX,DESCRIPTION,REGNUM7_4,REGNUM_BASE,ACC_GRAN,NUM_REGS,NUM_BYTE_PER_REG) \
    IGA_REGISTER_SPEC(Platform::GEN6,Platform::GENNEXT,REGNAME,SYNTAX,DESCRIPTION,REGNUM7_4,REGNUM_BASE,ACC_GRAN,NUM_REGS,NUM_BYTE_PER_REG)


// ordered by encoding of RegNum[7:4]
// newest platforms first
static const struct RegInfo REGISTER_SPECIFICATIONS[] = {
    IGA_REGISTER_SPEC_UNIFORM(
        RegName::GRF_R,"r","General",
        0,0,
        1,
        128,(0)),
    IGA_REGISTER_SPEC_UNIFORM(
        RegName::ARF_NULL, "null", "Null",
        0x0, 0,
        0,
        0, (32)),
    IGA_REGISTER_SPEC_UNIFORM(RegName::ARF_A, "a", "Index",
        0x1, 0,
        2,
        1, (32)),

    IGA_REGISTER_SPEC_UNIFORM(
        RegName::ARF_ACC, "acc", "Accumulator",
        0x2, 0,
        1,
        2, (32,32)),
    IGA_REGISTER_SPEC_UNIFORM( // acc2-9 are really mme0-7
        RegName::ARF_MME, "mme", "Math Macro",
        0x2, 2,
        4,
        8, (32,32,32,32,32,32,32,32)),

    IGA_REGISTER_SPEC_UNIFORM(
        RegName::ARF_F, "f", "Flag Register",
        0x3, 0,
        2,
        2, (4,4)),

    IGA_REGISTER_SPEC_GE(
        Platform::GEN7P5,
        RegName::ARF_CE, "ce", "Channel Enable",
        0x4, 0,
        4,
        0, (4)),

    IGA_REGISTER_SPEC_GE(
        Platform::GEN8,
        RegName::ARF_MSG, "msg", "Message Control",
        0x5, 0,
        4,
        8, (4,4,4,4,4,4,4,4)),

    IGA_REGISTER_SPEC_GE(
        Platform::GEN8,
        RegName::ARF_SP, "sp", "Stack Pointer",
        0x6, 0,
        4,
        0, (2*8)), // two subregisters of 8 bytes each
    IGA_REGISTER_SPEC(
        Platform::GEN7P5, Platform::GEN7P5,
        RegName::ARF_SP, "sp", "Stack Pointer",
        0x6, 0,
        4,
        0, (2*4)), // two subregisters of 4 bytes each


    IGA_REGISTER_SPEC_UNIFORM(
        RegName::ARF_SR, "sr", "State Register",
        0x7, 0,
        1,
        2, (16,16)), // sr{0,1}.{0..3}:d
    IGA_REGISTER_SPEC_UNIFORM(
        RegName::ARF_CR, "cr", "Control Register",
        0x8, 0,
        4,
        1, (3*4)), // cr0.{0..2}:d
    IGA_REGISTER_SPEC_UNIFORM(
        RegName::ARF_N, "n", "Notification Register",
        0x9, 0,
        4,
        1, (3*4)), // n0.{0..2}:d

    IGA_REGISTER_SPEC_UNIFORM(
        RegName::ARF_IP, "ip", "Instruction Pointer",
        0xA, 0,
        4,
        0, (4)), // ip
    IGA_REGISTER_SPEC_UNIFORM(
        RegName::ARF_TDR, "tdr", "Thread Dependency Register",
        0xB, 0,
        2,
        1, (16)), // tdr0.*
    IGA_REGISTER_SPEC_GE(
        Platform::GEN10,
        RegName::ARF_TM, "tm", "Timestamp Register",
        0xC, 0,
        4,
        1, (5*4)), // tm0.{0..4}:d
    IGA_REGISTER_SPEC_LE(
        Platform::GEN9,
        RegName::ARF_TM, "tm", "Timestamp Register",
        0xC, 0,
        4,
        1, (4*4)), // tm0.{0..3}:d
    IGA_REGISTER_SPEC_GE(
        Platform::GEN7P5,
        RegName::ARF_FC, "fc", "Flow Control",
        0xD, 0,
        4,
        5, (4*32,4*1,4*1,4*4,4*1)),
    IGA_REGISTER_SPEC(Platform::GEN7, Platform::GEN7P5,
        RegName::ARF_DBG, "dbg", "Debug",
        0xF, 0,
        4,
        1, (4)), // dbg0.0:ud
    IGA_REGISTER_SPEC_GE(Platform::GEN8,
        RegName::ARF_DBG, "dbg", "Debug",
        0xF, 0,
        4,
        1, (2*4)), // dbg0.{0,1}:ud
};


const OpSpec& Model::lookupOpSpec(Op op) const
{
    if (op < Op::FIRST_OP || op > Op::LAST_OP) {
        // external opspec API can reach this
        // IGA_ASSERT_FALSE("op out of bounds");
        return opsArray[(int)Op::INVALID]; // return invalid if assertions are off
    }
    return opsArray[(int)op];
}

const OpSpec& Model::lookupOpSpecByCode(unsigned opcode) const
{
    // if (!opsByCodeValid) {
    //     for (int i = (int)Op::FIRST_OP; i <= (int)Op::LAST_OP; i++) {
    //         const OpSpec &os = lookupOpSpec((Op)i);
    //         if (!os.isSubop()) {
    //             opsByCode[os.code] = &os;
    //         }
    //     }
    //     opsByCodeValid = true;
    // }
    for (int i = (int)Op::FIRST_OP; i <= (int)Op::LAST_OP; i++) {
        // FIXME: BXML needs to default invalid fields to -1
        if (opsArray[i].op != Op::INVALID &&
            opsArray[i].code == opcode)
        {
            return opsArray[i];
        }
    }
    return opsArray[static_cast<int>(Op::INVALID)];
}

template <int N>
static unsigned getBitsFromFragments(const uint64_t *qws, const Field ff[N])
{
    unsigned bits = 0;

    int off = 0;
    for (int i = 0; i < N; i++) {
        if (ff[i].length == 0) {
            break;
        }
        auto frag = (unsigned)getBits(qws, ff[i].offset, ff[i].length);
        bits |= frag << off;
        off += ff[i].length;
    }

    return bits;
}

// determines if a given group op has ternary subops
// this assumes all subops are the same format (all ternary or not)
static bool groupOpsAreTernaryFamily(
    const Model &model,
    const OpSpec *osGroup)
{
    for (int i = 0; osGroup->subopsLength; i++) {
        // find the first valid subop on this platform (model)
        const OpSpec &child = model.lookupOpSpec((Op)((int)osGroup->subopStart + i));
        if (child.isGroup()) {
            return groupOpsAreTernaryFamily(model, &child);
        } else if (child.isValid()) {
            return child.isTernary();
        }
    }
    // all are invalid: something is probably wrong
    return false;
}


const OpSpec& LookupOpSpecByBitsNative(
    const Model &model,
    const MInst *mi,
    OpSpecMissInfo &missInfo,
    const OpSpec *parentOp)
{
    while (parentOp->isGroup()) {
        auto sfBits = mi->getFields<2>(parentOp->functionControlFields);
        const OpSpec &subOp = model.lookupGroupSubOp(parentOp->op, (int)sfBits);
        if (!subOp.isValid()) {
            missInfo.parent = parentOp;
            missInfo.opcode = sfBits;
            return subOp;
        }
        parentOp = &subOp;
    }
    return *parentOp;
}

const OpSpec& LookupOpSpecSubfunctionByBitsCompacted(
    const Model &model,
    const MInst *miCmp,
    OpSpecMissInfo &missInfo,
    const CompactedField **compactedFields,
    size_t numCompactedFields,
    const OpSpec *parentOp)
{
    // HARD CASE: we have to extract the subop from one or more
    // compacted fields. E.g. we have Op::MATH and need to
    // resolve this to Op::MATH_INV.  But we need to get the
    // subfunction (MathFC) from the FlagMod bits.
    //
    // The generalized form is a a bit harder and we describe it in
    // detail.
    //
    // NOTE: below {X,Y} means simple bit concatenation
    //       (X<<(8*sizeof(Y)) | Y)
    // NOTE: native means an uncompacted 128b encoding
    //
    // However, the OpSpec::functionControlFields, which tells us where
    // the native encoding we can find subfunction field fragments
    // may contain multiple fragments.  Moreover, they don't have to map
    // exactly onto existing native fields.  For instance, they might be
    // a subset or even superset of native fields.  The spec just lists
    // the pieces that exist. E.g.
    //    <Property Name="Subfunctions" Value="MathFC[27:24]" ... />
    //                                       ^^^^ native field
    // Moreover, this can be multiple fragments (made up example)
    //    <Property Name="Subfunctions" Value="Foo[102,89:86,77:74]" ... />
    // would mean fragments {102,89:64,27:24}
    // And 102 could just be a subset of some compacted field.
    //
    // Visually:
    //  So in general a subfunction value might consist of bits
    //    {TTTT,SSSS}
    // where these are drawn from the uncompacted format
    //   ......TTTT.........SSSS..................128b
    // and the uncompacted fields that we compact might be A, B, and C
    //   ......CCCC.........BBAA..................128b
    //         TTTT         SSSS < uses two fields
    //
    // *********************************************************************
    // The easiest way to solve this is to just partially uncompact enough
    // of the instruction parts we need and then pull the subfunction fields
    // from from that dummy instruction.
    MInst partialUncompact;
    partialUncompact.qw0 = partialUncompact.qw1 = 0;

    // checks if a native field is used by our subfunction bits
    auto fieldOverlapsSubfunction = [&] (const Field &nf)
    {
        for (size_t sfFragIx = 0;
            sfFragIx < sizeof(parentOp->functionControlFields)/sizeof(parentOp->functionControlFields[0]);
            sfFragIx++)
        {
            const Field &sfFrag = parentOp->functionControlFields[sfFragIx];
            if (sfFrag.length == 0) // end of the subfunction fragments
                break;
            if (sfFrag.overlaps(nf))
                return true;
        }
        return false;
    };
    for (size_t cfIx = 0; cfIx < numCompactedFields; cfIx++ ) {
        const auto *cf = compactedFields[cfIx];
        uint64_t cmpIndex = miCmp->getField(cf->index);
        uint64_t mappedValue = cf->values[cmpIndex];

        int offsetWithinCompactedValue = 0;
        for (int k = (int)cf->numMappings - 1; k >= 0; --k) {
            // iterate all the packed native fields within compacted value
            // starting from low bits in the mapping and iterate on up to
            // the higher ones
            //
            // e.g. CMP_SRIX_3SRC has mappings
            // { Src2.SubRegNum[4:0], Src1.SubRegNum[4:0],
            //   Src0.SubRegNum[4:0], Dst.SubRegNum[4:0] }
            // we go fro Dst.SubReg[4:0] upwards
            const Field *nf = cf->mappings[k];
            if (fieldOverlapsSubfunction(*nf)) {
                uint64_t nativeVal = getBits(mappedValue, offsetWithinCompactedValue, nf->length);
                partialUncompact.setField(*nf, nativeVal);
            }

            offsetWithinCompactedValue += nf->length;
        }
    }

    // extract from the paritally uncompacted native instruction
    // the bits in parentOp->functionControlFields will be unpacked
    //
    // A static test should be run to ensure model consistency
    // I.e. subfunction bits may not come from unpacked fields (e.g. Src0.Reg)
    // since we won't look there (you could enable this if needed with
    // modifications)
    auto sfBits = partialUncompact.getFields<2>(parentOp->functionControlFields);

    // lookup the subfunction bits
    const OpSpec *subOp = &model.lookupGroupSubOp(parentOp->op, (unsigned)sfBits);
    if (!subOp->isValid()) {
        missInfo.parent = parentOp;
        missInfo.opcode = sfBits;
        return *subOp;
    } else if (subOp->isGroup()) {
        // subop of a subop (not used currently)
        return LookupOpSpecSubfunctionByBitsCompacted(
            model,
            miCmp,
            missInfo,
            compactedFields,
            numCompactedFields,
            subOp);
    } else {
        // valid lookup
        return *subOp;
    }
}

const OpSpec& Model::lookupOpSpecFromBits(
    const void *bits,
    OpSpecMissInfo &missInfo) const
{
    const static Field FOPCODE{nullptr,0,7};
    const MInst *mi = (const MInst *)bits;
    auto opc = mi->getField(FOPCODE);
    missInfo.parent = nullptr;
    missInfo.opcode = opc;
    const OpSpec *os = &lookupOpSpecByCode((unsigned)opc);
    if (mi->isCompact()) {
        if (!os->isGroup()) {
            return *os;
        } else {
            const CompactedField **compactedFields = nullptr;
            size_t numCompactedFields = 0;
            (void)compactedFields;
            (void)numCompactedFields;
            if (compactedFields)
              return LookupOpSpecSubfunctionByBitsCompacted(
                  *this, mi, missInfo, compactedFields, numCompactedFields, os);

            compactedFields = nullptr;
                  IGA_ASSERT_FALSE("no compaction tables on this platform (yet)");
                  return *os;
        } // else(!os->isGroup())
    } else {
        return LookupOpSpecByBitsNative(*this, mi, missInfo, os);
    }
}

const OpSpec &Model::lookupGroupSubOp(Op op, unsigned fcBits) const
{
    const OpSpec& os = opsArray[(int)op];
    IGA_ASSERT(os.format == OpSpec::GROUP, "must be group op");
    for (int i = 0; i < os.subopsLength; i++) {
        const OpSpec &sos = opsArray[(int)os.subopStart + i];
        if (sos.functionControlValue == fcBits) {
            return sos;
        }
    }
    // return invalid without asserting
    //
    // std::stringstream ss;
    // ss << "unable to find subop of ";
    // auto mne = lookupOpSpec(op).mnemonic;
    // mne = mne ? mne : "?";
    // ss << std::hex << "0x" << (int)op << " (" << mne << ")";
    // IGA_ASSERT_FALSE(ss.str().c_str());
    return opsArray[static_cast<int>(Op::INVALID)];
}

const OpSpec* Model::lookupSubOpParent(const OpSpec &os) const
{
    if (os.groupOp == Op::INVALID)
        return nullptr;
    return &opsArray[static_cast<int>(os.groupOp)];
}

const RegInfo *Model::lookupRegInfoByRegName(RegName name) const
{
    // static tester should check this
    for (const RegInfo &ri : REGISTER_SPECIFICATIONS) {
        if (ri.regName == name && ri.supportedOn(platform)) {
            return &ri;
        }
    }
    return nullptr;
}

const RegInfo *iga::GetRegisterSpecificationTable(int &len)
{
    len = sizeof(REGISTER_SPECIFICATIONS)/sizeof(REGISTER_SPECIFICATIONS[0]);
    return REGISTER_SPECIFICATIONS;
}

const RegInfo* Model::lookupArfRegInfoByRegNum(uint8_t regNum7_0) const
{
    const RegInfo *arfAcc = nullptr;
    int regNum = (int)(regNum7_0 & 0xF);
    for (const RegInfo &ri : REGISTER_SPECIFICATIONS) {
        if (ri.regName == RegName::GRF_R) {
            continue; // GRF will be in the table as 0000b
        } else if (ri.regNum7_4 == (regNum7_0 >> 4) && // RegNum[7:4] matches AND
            ri.supportedOn(platform))           // platform matches
        {
            int shiftedRegNum = regNum - ri.regNumBase;
            if (ri.regName == RegName::ARF_MME &&
                !ri.isRegNumberValid(shiftedRegNum) &&
                arfAcc != nullptr)
            {
                // they picked an invalid register in the acc# space
                // (which is shared with mme#)
                // since acc# is far more likely, favor that so the error
                // message about the register number being out of range
                // refers to acc# instead of mme#
                return arfAcc;
            } else if (ri.regName == RegName::ARF_ACC &&
                !ri.isRegNumberValid(shiftedRegNum))
            {
                // not really acc#, but mme#, continue the loop until we find
                // that one, but at least save acc# for the error case above
                arfAcc = &ri;
            } else {
                // - it's acc# (value reg)
                // - its mme#
                // - it's some other ARF
                return &ri;
            }
        }
    }
    // if we get here, we didn't find a matching register for this platform
    // it is possible we found and rejected acc# because the reg num was out
    // of bounds and we were hoping it was an mme# register, so we return
    // that reg specification so that the error message will favor
    // acc# over mme# (since the latter is far less likely)
    return arfAcc;
}

bool RegInfo::encode(int reg, uint8_t &regNumBits) const
{
    if (!isRegNumberValid(reg)) {
        return false;
    }

    if (regName == RegName::GRF_R) {
        regNumBits = reg;
    } else {
        // ARF
        // RegNum[7:4] = high bits from the spec
        reg += regNumBase;
        // this assert would suggest that something is busted in
        // the RegInfo table
        IGA_ASSERT(reg <= 0xF, "ARF encoding overflowed");
        regNumBits = regNum7_4 << 4;
        regNumBits |= (uint8_t)reg;
    }
    return true;
}

bool RegInfo::decode(uint8_t regNumBits, int &reg) const
{
    if (regName == RegName::GRF_R) {
        reg = (int)regNumBits;
    } else {
        reg = (int)(regNumBits & 0xF) - regNumBase; // acc2 -> mme0
    }
    return isRegNumberValid(reg);
}


const Model *Model::LookupModel(Platform p)
{
    switch (p) {
    case Platform::GEN7P5:
        return &MODEL_GEN7P5;
    case Platform::GEN8:
    case Platform::GEN8LP:
        return &MODEL_GEN8;
    case Platform::GEN9:
    case Platform::GEN9LP:
    case Platform::GEN9P5:
        return &MODEL_GEN9;
    case Platform::GEN10:
        return &MODEL_GEN10;
    case Platform::GEN11:
        return &MODEL_GEN11;
    default:
        return nullptr;
    }
}
