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
#include "Interface.hpp"

#include "Decoder.hpp"
#include "Encoder.hpp"

using namespace iga;

bool iga::ged::IsEncodeSupported(const Model &, const EncoderOpts &)
{
    return true;
}
void iga::ged::Encode(
    const Model &m,
    const EncoderOpts &eopts,
    ErrorHandler &eh,
    Kernel &k,
    void *&bits,
    size_t &bitsLen)
{
    Encoder enc(m, eh, eopts);
    uint32_t bitsLen32 = 0;
    try {
        enc.encodeKernel(
            k,
            k.getMemManager(),
            bits,
            bitsLen32);
    } catch (FatalError) {
        // error already reported
    }
    bitsLen = bitsLen32;
}

bool iga::ged::IsDecodeSupported(
    const Model &,
    const DecoderOpts &)
{
    return true;
}
Kernel *iga::ged::Decode(
    const Model &m,
    const DecoderOpts &dopts,
    ErrorHandler &eh,
    const void *bits,
    size_t bitsLen)
{
    Kernel *k = nullptr;
    try {
        iga::Decoder decoder(m, eh);
        k = dopts.useNumericLabels ?
            decoder.decodeKernelNumeric(bits, bitsLen) :
            decoder.decodeKernelBlocks(bits, bitsLen);
    } catch (FatalError) {
        // error already reported
    }
    return k;
}