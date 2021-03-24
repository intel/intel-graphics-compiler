/*========================== begin_copyright_notice ============================

Copyright (c) 2020-2021 Intel Corporation

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

#ifndef EMITTEROPTIONS_HPP_0NAXOILP
#define EMITTEROPTIONS_HPP_0NAXOILP

namespace IGC
{
  struct DebugEmitterOpts {
    bool DebugEnabled = false;
    bool isDirectElf = false;
    bool UseNewRegisterEncoding = true;
    bool EnableSIMDLaneDebugging = true;
    bool EnableGTLocationDebugging = false;
    bool UseOffsetInLocation = false;
    bool EmitDebugRanges = false;
    bool EmitDebugLoc = false;
    bool EmitOffsetInDbgLoc = false;
    bool EnableRelocation = false;
    bool EmitPrologueEnd = true;
    bool ScratchOffsetInOW = true;
  };
}

#endif /* end of include guard: EMITTEROPTIONS_HPP_0NAXOILP */

