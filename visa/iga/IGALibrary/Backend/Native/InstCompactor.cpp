/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "InstCompactor.hpp"
#include "../../bits.hpp"

using namespace iga;


bool InstCompactor::compactIndex(
    const CompactionMapping &cm, int immLo, int immHi)
{
    // fragments are ordered from high bit down to 0
    // hence, we have to walk them in reverse order to build the
    // word in reverse order
    //
    // we build up the don't-care mask as we assemble the
    // desired mapping
    uint64_t relevantBits = 0xFFFFFFFFFFFFFFFFull;
    int indexOffset = 0; // offset into the compaction index value
    uint64_t mappedValue = 0;
    for (int i = (int)cm.numMappings - 1; i >= 0; i--) {
        // assume atomic field
        const Fragment &mappedFragment = *cm.mappings[i];
        if (rangeContains(immLo, immHi, mappedFragment.offset) ||
            rangeContains(immLo, immHi,
                mappedFragment.offset + mappedFragment.length - 1))
        {
            // this is a don't-care field since the imm value expands
            // into this field; strip those bits out
            relevantBits &= ~getFieldMask<uint64_t>(
                indexOffset, mappedFragment.length);
        } else {
            // we omit the random bits in don't care fields to give us
            // a normalized lookup value (for debugging)
            // technically we can match any table entry for don't-care
            // bits
            uint64_t uncompactedValue =
                uncompactedBits.getFragment(mappedFragment);
            iga::setBits(
                mappedValue,
                indexOffset,
                mappedFragment.length,
                uncompactedValue);
        }
        indexOffset += mappedFragment.length;
    }

    // TODO: make lookup constant (could use a prefix tree or just a simple hash)
    for (size_t i = 0; i < cm.numValues; i++) {
        if ((cm.values[i] & relevantBits) == mappedValue) {
            if (!compactedBits.setField(cm.index, (uint64_t)i)) {
                IGA_ASSERT_FALSE("compaction index overruns field");
            }
            return true; // hit
        }
    }

    // compaction miss
    fail(cm, mappedValue);
    return compactionDebugInfo != nullptr;
}


CompactionResult InstCompactor::tryToCompactImpl() {
    switch (model.platform) {
    case Platform::XE:
    case Platform::XE_HP:
    case Platform::XE_HPG:
    case Platform::XE_HPC:
        return tryToCompactImplFamilyXE();
    case Platform::FUTURE:
    default:
        compactionMissed = true;
        IGA_ASSERT_FALSE("compaction not supported on this platform");
        return CompactionResult::CR_NO_COMPACT;
    }
}


CompactionResult InstCompactor::tryToCompactImplFamilyXE()
{
    compactionResult = CompactionResult::CR_NO_FORMAT;
    return compactionResult;
}
