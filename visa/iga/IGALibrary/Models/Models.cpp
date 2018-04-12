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
    const OpSpec *os = &lookupOpSpecByCode((unsigned)mi->getField(FOPCODE));
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
        }
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

const RegInfo* Model::lookupRegInfoByName(RegName name) const
{
    for (const RegInfo &ri : registers) {
        if (ri.reg == name && ri.supportedOn(platform)) {
            return &ri;
        }
    }
    IGA_ASSERT_FALSE("unreachable");
    return nullptr;
}

const RegInfo* Model::lookupArfRegInfoByCode(const uint8_t encoding) const
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
