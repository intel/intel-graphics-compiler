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

// dummy symbol to avoid LNK4221 warning
int inst_encoder_dummy;

using namespace iga;

#ifdef VALIDATE_BITS
void InstEncoder::checkFieldOverlaps(const Field &f)
{
    uint64_t fieldMask = getFieldMask(f);
    uint64_t &dirtyMask = f.offset < 64 ? state.dirty.qw0 : state.dirty.qw1;
    if (dirtyMask & fieldMask) {
        std::stringstream ss;
        ss << "instruction index " << state.instIndex <<
            ": " << f.name <<  " overlaps with ";
        for (auto &of : state.fieldsSet) {
            if (getFieldMask(*of) & fieldMask) {
                ss << of->name;
                auto str = ss.str();
                std::cerr << "checkFieldOverlaps: " << str << "\n";
                IGA_ASSERT_FALSE(str.c_str());
            }
        }
        ss << "another field (?)";
        auto str = ss.str();
        IGA_ASSERT_FALSE(str.c_str());
    }
    state.fieldsSet.push_back(&f);
    dirtyMask |= fieldMask;
}
#endif
