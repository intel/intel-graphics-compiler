/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Models.hpp"

// this must precede model*.hpp inclusion below
#include "../strings.hpp"


// for bxml/Model operand type mappings
#define TYPE(T) \
    ENUM_BITSET_VALUE(T, uint32_t)

#include "bxml/ModelGen7p5.hpp"
#include "bxml/ModelGen8.hpp"
#include "bxml/ModelGen9.hpp"
#include "bxml/ModelGen10.hpp"
#include "bxml/ModelGen11.hpp"
#include "bxml/ModelXe.hpp"
#include "bxml/ModelXeHP.hpp"
#include "bxml/ModelXeHPG.hpp"
#include "bxml/ModelXeHPC.hpp"
#include "../asserts.hpp"
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
    IGA_REGISTER_SPEC(PLAT_LO,Platform::FUTURE,REGNAME,SYNTAX,DESCRIPTION,REGNUM7_4,REGNUM_BASE,ACC_GRAN,NUM_REGS,NUM_BYTE_PER_REG)
// a specification valid on all platforms
#define IGA_REGISTER_SPEC_UNIFORM(REGNAME,SYNTAX,DESCRIPTION,REGNUM7_4,REGNUM_BASE,ACC_GRAN,NUM_REGS,NUM_BYTE_PER_REG) \
    IGA_REGISTER_SPEC(Platform::GEN6,Platform::FUTURE,REGNAME,SYNTAX,DESCRIPTION,REGNUM7_4,REGNUM_BASE,ACC_GRAN,NUM_REGS,NUM_BYTE_PER_REG)


// ordered by encoding of RegNum[7:4]
// newest platforms first
static const struct RegInfo REGISTER_SPECIFICATIONS[] = {
    IGA_REGISTER_SPEC_LE(
        Platform::GEN11,
        RegName::GRF_R, "r", "General",
        0, 0,
        1,
        128,(0)),

    IGA_REGISTER_SPEC_GE(
        Platform::XE,
        RegName::GRF_R, "r", "General",
        0,0, // regNum7_4, regNumBase
        1,   // accGran
        256,(0)),

    IGA_REGISTER_SPEC_UNIFORM(
        RegName::ARF_NULL, "null", "Null",
        0x0, 0,
        0,
        0, (32)),
    IGA_REGISTER_SPEC_UNIFORM(RegName::ARF_A, "a", "Index",
        0x1, 0,
        2,
        1, (32)),

    // acc and mme share same RegNum[7:4], mme gets the high registers
    IGA_REGISTER_SPEC_LE(
        Platform::GEN11,
        RegName::ARF_ACC, "acc", "Accumulator",
        0x2, 0,
        1,
        2, (32,32)),
    IGA_REGISTER_SPEC(Platform::XE, Platform::XE,
        RegName::ARF_ACC, "acc", "Accumulator",
        0x2, 0,
        1,
        8, (32,32,32,32,32,32,32,32)),
    IGA_REGISTER_SPEC(Platform::XE_HP, Platform::XE_HP,
        RegName::ARF_ACC, "acc", "Accumulator",
        0x2, 0,
        1,
        8, (32,32,32,32,32,32,32,32)),
    IGA_REGISTER_SPEC(Platform::XE_HPG, Platform::XE_HPG,
        RegName::ARF_ACC, "acc", "Accumulator",
        0x2, 0,
        1,
        8, (32,32,32,32,32,32,32,32)),
    IGA_REGISTER_SPEC_GE(
        Platform::XE_HPC,
        RegName::ARF_ACC, "acc", "Accumulator",
        0x2, 0,
        1,
        16, (64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64)),
    IGA_REGISTER_SPEC_LE(
        Platform::GEN11,
        RegName::ARF_MME, "mme", "Math Macro",
        0x2, 2, // offset by 2 "acc2-9"
        4,
        8, (32,32,32,32,32,32,32,32)),
    IGA_REGISTER_SPEC(Platform::XE, Platform::XE,
        RegName::ARF_MME, "mme", "Math Macro",
        0x2, 8, // offset by 8 "acc8-15"
        4,
        8, (32,32,32,32,32,32,32,32)),
    IGA_REGISTER_SPEC(Platform::XE_HP, Platform::XE_HP,
        RegName::ARF_MME, "mme", "Math Macro",
        0x2, 8, // offset by 8 "acc8-15"
        4,
        8, (32,32,32,32,32,32,32,32)),
    IGA_REGISTER_SPEC(Platform::XE_HPG, Platform::XE_HPG,
        RegName::ARF_MME, "mme", "Math Macro",
        0x2, 8, // offset by 8 "acc8-15"
        4,
        8, (32,32,32,32,32,32,32,32)),
    IGA_REGISTER_SPEC_GE(
        Platform::XE_HPC,
        RegName::ARF_MME, "mme", "Math Macro",
        0x2, 8, // offset by 8 "acc8-15"
        4,
        8, (64,64,64,64,64,64,64,64)),

    IGA_REGISTER_SPEC_LE(
        Platform::XE_HPG,
        RegName::ARF_F, "f", "Flag Register",
        0x3, 0,
        2,
        2, (4,4)),
    IGA_REGISTER_SPEC_GE(
        Platform::XE_HPC,
        RegName::ARF_F, "f", "Flag Register",
        0x3, 0,
        2,
        4, (4,4,4,4)),

    IGA_REGISTER_SPEC_GE(
        Platform::GEN7P5,
        RegName::ARF_CE, "ce", "Channel Enable",
        0x4, 0,
        4,
        0, (4)),

    IGA_REGISTER_SPEC_LE(
        Platform::XE_HPC,
        RegName::ARF_MSG, "msg", "Message Control",
        0x5, 0,
        4,
        8, (4,4,4,4,4,4,4,4)),
    IGA_REGISTER_SPEC(
        Platform::GEN7P5, Platform::GEN7P5,
        RegName::ARF_SP, "sp", "Stack Pointer",
        0x6, 0,
        4,
        0, (2*4)), // two subregisters of 4 bytes each
    IGA_REGISTER_SPEC(
        Platform::GEN8, Platform::XE_HPC,
        RegName::ARF_SP, "sp", "Stack Pointer",
        0x6, 0,
        4,
        0, (2*8)), // two subregisters of 8 bytes each


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

        // with SWSB wait n{0,1} replaced by sync.{bar,host}, which
        // implicitly reference notification registers;
        // not sure if these are needed in CSR though, so leaving for now

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

    // fc0.0-31  stack-entry 0-31
    // fc1.0     global counts
    // fc2.0     top of stack pointers
    // fc3.0-3   per channel counts
    // fc4.0     call mask
    IGA_REGISTER_SPEC(Platform::GEN7P5, Platform::GEN11,
        RegName::ARF_FC, "fc", "Flow Control",
        0xD, 0,
        4,
        5, (4*32,4*1,4*1,4*4,4*1)),
    //  EU GOTO/JOIN instruction latency improvement HAS397165 removes two flow control registers
    // fc0.0-31  per-channel IP
    // fc1.0     channel enables
    // fc2       call mask
    // fc3       JEU fused mask
    IGA_REGISTER_SPEC(Platform::XE, Platform::XE_HPC,
        RegName::ARF_FC, "fc", "Flow Control",
        0xD, 0,
        4,
        4, (4*32,4*1,4*1,4*1)),
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
        if (opsArray[i].op != Op::INVALID &&
            opsArray[i].opcode == opcode)
        {
            return opsArray[i];
        }
    }
    return opsArray[static_cast<int>(Op::INVALID)];
}

template <int N>
static unsigned getBitsFromFragments(const uint64_t *qws, const Fragment ff[N])
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


const OpSpec& Model::lookupOpSpecFromBits(
    const void *bits,
    OpSpecMissInfo &missInfo) const
{
    constexpr static Fragment F_OPCODE("Opcode", 0, 7);
    //
    const MInst *mi = (const MInst *)bits;
    auto opc = mi->getFragment(F_OPCODE);
    missInfo.opcode = opc;
    const OpSpec *os = &lookupOpSpecByCode((unsigned)opc);
    return *os;
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

uint32_t Model::getNumGRF() const
{
    return getRegCount(RegName::GRF_R);
}

uint32_t Model::getNumFlagReg() const
{
    return getRegCount(RegName::ARF_F);
}

uint32_t Model::getGRFByteSize() const
{
    return platform >= Platform::XE_HPC ? 64 : 32;
}

uint32_t Model::getRegCount(RegName rn) const {
    const RegInfo* ri = lookupRegInfoByRegName(rn);
    IGA_ASSERT(ri, "invalid register for platform");
    // for getNumReg 0 means single register (like ce); bump to 1
    int n = std::max(ri->getNumReg(), 1);
    return n;
}

uint32_t Model::getBytesPerReg(RegName rn) const {
    const RegInfo* ri = lookupRegInfoByRegName(rn);
    IGA_ASSERT(ri, "invalid register for platform");
    if (rn == RegName::GRF_R) {
        // GRF has 0's in numBytesPerReg[..]
        if (platform >= Platform::XE_HPC)
            return 64;
        return 32;
    }
    // we assume they are all equal length
    return ri->numBytesPerReg[0];
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
        } else if (ri.regNum7_4 == ((uint32_t)regNum7_0 >> 4) && // RegNum[7:4] matches AND
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
        regNumBits = (uint8_t)reg;
    } else {
        // ARF
        // RegNum[7:4] = high bits from the spec
        reg += regNumBase;
        // this assert would suggest that something is busted in
        // the RegInfo table
        IGA_ASSERT(reg <= 0xF, "ARF encoding overflowed");
        regNumBits = (uint8_t)(regNum7_4 << 4);
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


// static const iga::Model MODEL_GEN7(
//    Platform::GEN7P5, &MODEL_GEN7_OPSPECS[0], "7", "ivb");
static constexpr Model MODEL_GEN7P5(
    Platform::GEN7P5, &MODEL_GEN7P5_OPSPECS[0], "7p5", "hsw");
static constexpr Model MODEL_GEN8(
    Platform::GEN8, &MODEL_GEN8_OPSPECS[0], "8", "bdw");
static constexpr Model MODEL_GEN9(
    Platform::GEN9, &MODEL_GEN9_OPSPECS[0], "9", "skl");
static constexpr Model MODEL_GEN10(
    Platform::GEN10, &MODEL_GEN10_OPSPECS[0], "10", "cnl");
static constexpr Model MODEL_GEN11(
    Platform::GEN11, &MODEL_GEN11_OPSPECS[0], "11", "icl");
static constexpr Model MODEL_XE(
    Platform::XE, &MODEL_XE_OPSPECS[0], "12p1",
    "xe", "xelp", "tgl", "tgllp", "dg1");
static constexpr Model MODEL_XE_HP(
    Platform::XE_HP, &MODEL_XE_HP_OPSPECS[0], "12p5", "xehp"
    );
static constexpr Model MODEL_XE_HPG(
    Platform::XE_HPG, &MODEL_XE_HPG_OPSPECS[0],
    "12p71", // default file extension
    "xehpg"
    );
static constexpr Model MODEL_XE_HPC(
    Platform::XE_HPC, &MODEL_XE_HPC_OPSPECS[0],
    "12p72",
    "xehpc"
    );

const Model * const iga::ALL_MODELS[] {
    &MODEL_GEN7P5,
    &MODEL_GEN8,
    &MODEL_GEN9,
    &MODEL_GEN10,
    &MODEL_GEN11,
    &MODEL_XE,
    &MODEL_XE_HP,
    &MODEL_XE_HPG,
    &MODEL_XE_HPC,
};
const size_t iga::ALL_MODELS_LEN = sizeof(ALL_MODELS)/sizeof(ALL_MODELS[0]);

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
    case Platform::XE:
        return &MODEL_XE;
    case Platform::XE_HP:
        return &MODEL_XE_HP;
    case Platform::XE_HPG:
        return &MODEL_XE_HPG;
    case Platform::XE_HPC:
        return &MODEL_XE_HPC;
    default:
        return nullptr;
    }
}

const Model &Model::LookupModelRef(Platform platform)
{
    const Model *m = Model::LookupModel(platform);
    IGA_ASSERT(m, "invalid platform");
    return *m;
}

