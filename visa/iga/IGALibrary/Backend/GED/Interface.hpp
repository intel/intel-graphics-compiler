/*========================== begin_copyright_notice ============================

Copyright (c) 2019-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#ifndef _IGA_BACKEND_GED_INTERFACE_HPP_
#define _IGA_BACKEND_GED_INTERFACE_HPP_

////////////////////////////////////////////
// GED ENCODER USERS USE THIS INTERFACE
#include "../DecoderOpts.hpp"
#include "../EncoderOpts.hpp"
#include "../../ErrorHandler.hpp"
#include "../../IR/Kernel.hpp"

namespace iga {namespace ged
{
    bool IsEncodeSupported(
        const Model &m,
        const EncoderOpts &opts);
    void Encode(
        const Model &m,
        const EncoderOpts &opts,
        ErrorHandler &eh,
        Kernel &k,
        void *&bits,
        size_t &bitsLen);

    bool IsDecodeSupported(
        const Model &m,
        const DecoderOpts &opts);
    Kernel *Decode(
        const Model &m,
        const DecoderOpts &dopts,
        ErrorHandler &eh,
        const void *bits,
        size_t bitsLen);
}} // namespace

#endif // _IGA_BACKEND_GED_INTERFACE_HPP_
