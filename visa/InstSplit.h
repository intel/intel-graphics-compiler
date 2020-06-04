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

#ifndef _INSTSPLIT_H_
#define _INSTSPLIT_H_

#include "BuildIR.h"

namespace vISA
{
    class InstSplitPass
    {
    public:
        InstSplitPass(IR_Builder* builder);
        void run();

    private:
        INST_LIST_ITER splitInstruction(INST_LIST_ITER it);
        G4_CmpRelation compareSrcDstRegRegion(G4_DstRegRegion* dstRegRegion, G4_Operand* opnd);
        void computeDstBounds(G4_DstRegRegion* dstRegion, uint32_t& leftBound, uint32_t& rightBound);
        void computeSrcBounds(G4_SrcRegRegion* srcRegion, uint32_t& leftBound, uint32_t& rightBound);
        void generateBitMask(G4_Operand* opnd, BitSet& footprint);

        IR_Builder* m_builder;
    };
}

#endif
