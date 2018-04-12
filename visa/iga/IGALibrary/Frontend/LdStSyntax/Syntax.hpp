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
#include "Tables.hpp"

#include <ostream>

namespace iga {
    // only the first token, not the full sequence
    // e.g.s "a64c" or "surf[0x2]"
    const char *SymbolFor(MAddrModel am);
    // emits the full syntax
    void Format(std::ostream &os,MAddrModel am);
    //
    // makes a good faith attempt to compute the number of expected data
    // and address registers per message; headers are not inlcuded
    //
    // returns false if we can't figure it out
    bool ComputeMessageAddressRegisters(
        const MFormat *f,
        int simdMode,
        uint32_t desc,
        int &addrRegs);
    bool ComputeMessageDataRegisters(
        const MFormat *f,
        int simdMode,
        uint32_t desc,
        int &dataRegs);

    // returns the header included field (bit 19) if it exists in a format
    const MField *FindHeaderField(const MFormat &mf);
}