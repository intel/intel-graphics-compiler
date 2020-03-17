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
#include "InstEncoder.hpp"
#include "../BitProcessor.hpp"

#include <sstream>

// dummy symbol to avoid LNK4221 warning

using namespace iga;

#ifdef IGA_VALIDATE_BITS
void InstEncoder::reportFieldOverlap(const Fragment &fr)
{
    std::stringstream ss;
    ss << "INTERNAL ERROR: ";
    ss << "instruction #" << state.instIndex <<
        " [PC 0x" << std::hex << state.inst->getPC() << "]: " <<
        fr.name << " overlaps with ";
    // iterate the old fragments and find one that overlapped
    for (const Fragment *of : state.fragmentsSet) {
        if (of->overlaps(fr)) {
            ss << of->name;
            auto str = ss.str();
            // std::cerr << "checkFieldOverlaps: " << str << "\n";
            // IGA_ASSERT_FALSE(str.c_str());
            error("%s", str.c_str());
            return;
        }
    }
    ss << "another field (?)";
    auto str = ss.str();
    error("%s", str.c_str());
    // IGA_ASSERT_FALSE(str.c_str());
}
#endif

void InstEncoder::encodeFieldBits(const Field &f, uint64_t val0)
{
#ifndef IGA_VALIDATE_BITS
    // bit-validation can't cut this corner
    if (val0 == 0) {
        // short circuit since we start with zeros
        return;
    }
#endif
    uint64_t val = val0;
    for (const Fragment &fr : f.fragments) {
        if (fr.kind == Fragment::Kind::INVALID)
            break;
        const auto MASK = fr.getMask();
        const auto fragValue = val & MASK;
        switch (fr.kind) {
        case Fragment::Kind::ENCODED:
            bits->setBits(fr.offset, fr.length, fragValue);
#ifdef IGA_VALIDATE_BITS
            if (state.dirty.getBits(fr.offset, fr.length)) {
                // some of these bits have already been set by some
                // other field; this demonstrates an internal error
                // in our encoding logic
                reportFieldOverlap(fr);
            }
            state.fragmentsSet.push_back(&fr);
            state.dirty.setBits(fr.offset, fr.length, MASK);
#endif
            break;
        case Fragment::Kind::ZERO_WIRES:
        case Fragment::Kind::ZERO_FILL:
            if (fragValue != 0) {
                errorAt(state.inst->getLoc(),
                    "%s: 0x%X: field fragment must be zero",
                    fr.name, val0);
            }
            break;
        default:
            IGA_ASSERT_FALSE("unreachable");
        }
        val >>= fr.length;
    }

    if (val != 0) {
        // value overflows the virtual encoding
        errorAt(state.inst->getLoc(),
            "%s: 0x%X: value is too large for field", f.name, val);
    }
}
