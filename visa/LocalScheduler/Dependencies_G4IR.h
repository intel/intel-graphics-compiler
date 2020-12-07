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

#ifndef _DEPENDENCIES_H_
#define _DEPENDENCIES_H_

class Options;

namespace vISA {

class G4_INST;

enum DepType
{
    NODEP = 0,
    RAW, RAW_MEMORY,
    WAR, WAR_MEMORY,
    WAW, WAW_MEMORY,
    CONTROL_FLOW_BARRIER,
    SEND_BARRIER,
    INDIRECT_ADDR_BARRIER,
    MSG_BARRIER,
    DEP_LABEL,
    OPT_BARRIER,
    DEPTYPE_MAX
};

DepType getDepSend(G4_INST *curInst, G4_INST *liveInst, bool BTIIsRestrict);

DepType getDepScratchSend(G4_INST *curInst, G4_INST *liveInst);

DepType CheckBarrier(G4_INST *inst);

} // namespace vISA

#endif
