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

#ifndef _ACCSUBSTITUION_H
#define _ACCSUBSTITUION_H

#include "FlowGraph.h"
#include "BuildIR.h"

// Replace local GRF variables with accumulator. This offers a number of benefits:
// -- avoid bank conflict
// -- reduce back to back dependency
// -- lower power consumption
// Note that this does not lower GRF pressure since we run this pass post-RA; this is because we only have a few accumulators available,
// and doing this before RA may introduce anti-dependencies and make scheduling less effective.

struct AccInterval;

namespace vISA
{

class AccSubPass
{

    IR_Builder& builder;
    G4_Kernel& kernel;

    int numAccSubDef = 0;
    int numAccSubUse = 0;

    bool replaceDstWithAcc(G4_INST* inst, int accNum);


public:

    AccSubPass(IR_Builder& B, G4_Kernel& K) : builder(B), kernel(K) {}

    AccSubPass(const AccSubPass&) = delete;

    virtual ~AccSubPass() = default;

    void run()
    {
        for (auto bb : kernel.fg)
        {
            accSub(bb);
        }
    }
    void accSub(G4_BB* bb);
    void multiAccSub(G4_BB* bb);

    bool isAccCandidate(G4_INST* inst, int& lastUse, bool& mustBeAcc0);

    int getNumAccSubDef() const { return numAccSubDef; }
    int getNumAccSubUse() const { return numAccSubUse; }
};

}

#endif // _ACCSUBSTITUION_H
