#include "Models.hpp"

// for bxml/Model operand type mappings
#define TYPE(T) \
    ENUM_BITSET_VALUE(T,uint32_t)

#include "bxml/Model7P5.hpp"
#include "bxml/Model8.hpp"
#include "bxml/Model9.hpp"
#include "bxml/Model10.hpp"
#include "../bits.hpp"
#include "../bits.hpp"
#include "../Backend/Native/MInst.hpp"

#include <sstream>
#include <iostream>

using namespace iga;

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

const OpSpec& Model::lookupOpSpecByBits(const void *bits) const
{
    const uint64_t *qws = (const uint64_t *)bits;
    const OpSpec *os = &lookupOpSpecByCode((unsigned)getBits(qws, 0,7));
    while (os->isGroup()) {
        // TODO: must map the compaction table index field,
        // the table, and the relvant fragments from entries
        const MInst *mi = (const MInst *)bits;
        if (!mi->isCompact()) {
            auto sfBits = getBitsFromFragments<2>(qws, os->functionControlFields);
            os = &lookupGroupSubOp(os->op, sfBits);
        } else {
        }

    }
    return *os;
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

const RegInfo* Model::lookupRegInfoByCode(const uint8_t encoding) const
{
    for (const RegInfo &ri : registers) {
        if (ri.encoding == encoding && ri.supportedOn(platform)) {
            return &ri;
        }
    }
    return nullptr;
}

bool Model::encodeReg(RegName rn, int reg, uint8_t &outputBits) const
{
    for (const RegInfo &ri : registers) {
        if (ri.reg == rn && ri.supportedOn(platform)) {
            if (reg > ri.num_regs) {
                return false;
            }
            outputBits = ri.encoding << 4;
            if (rn == RegName::ARF_SP && reg != 0) {
                outputBits |= 0xF;
            } else {
                outputBits |= (uint8_t)reg;
            }
            return true;
        }
    }
    return false;
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
    default:
        return nullptr;
    }
}
