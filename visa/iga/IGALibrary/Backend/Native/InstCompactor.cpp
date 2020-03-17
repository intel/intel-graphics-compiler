/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

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
    case Platform::GENNEXT:
    default:
        compactionMissed = true;
        IGA_ASSERT_FALSE("compaction not supported on this platform");
        return CompactionResult::CR_NO_COMPACT;
    }
}


// NOTE: has to be above encodeForPlatform<GEN12> or we get C2910 in MSVC
CompactionResult InstCompactor::tryToCompactImplFamilyGen12()
{
    compactionResult = CompactionResult::CR_NO_FORMAT;
    return compactionResult;
}
