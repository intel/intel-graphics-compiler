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

#include "FlowGraph.h"

using namespace vISA;

void G4_SrcRegRegion::computePReg()
{
    int thisOpSize(G4_Type_Table[type].byteSize);
    unsigned int regNum = 0, subRegNum = 0;
    if (base->isRegVar() && base->asRegVar()->isPhyRegAssigned())
    {
        G4_RegVar* baseVar = base->asRegVar();
        if (baseVar->getPhyReg()->isGreg())
        {
            G4_Declare* dcl = baseVar->getDeclare();

            regNum = (static_cast<G4_Greg*>(baseVar->getPhyReg()))->getRegNum();

            subRegNum = baseVar->getPhyRegOff();

            int declOpSize(G4_Type_Table[dcl->getElemType()].byteSize);

            if (thisOpSize != declOpSize)
            {
                subRegNum = (subRegNum * declOpSize) / thisOpSize;
            }

            unsigned int linearizedStart = (regNum * numEltPerGRF(Type_UB)) + (subRegNum * thisOpSize);

            dcl->setGRFBaseOffset(linearizedStart);
        }
    }
}

void G4_DstRegRegion::computePReg()
{
    unsigned int regNum = 0, subRegNum = 0;
    if (base->isRegVar() && base->asRegVar()->isPhyRegAssigned())
    {
        G4_RegVar* baseVar = base->asRegVar();
        if (baseVar->getPhyReg()->isGreg())
        {
            G4_Declare* dcl = baseVar->getDeclare();

            regNum = (static_cast<G4_Greg*>(baseVar->getPhyReg()))->getRegNum();

            subRegNum = baseVar->getPhyRegOff();

            int declOpSize(G4_Type_Table[dcl->getElemType()].byteSize);
            int thisOpSize(G4_Type_Table[type].byteSize);

            if (thisOpSize != declOpSize)
            {
                subRegNum = (subRegNum * declOpSize) / thisOpSize;
            }

            unsigned int linearizedStart = (regNum * numEltPerGRF(Type_UB)) + (subRegNum *  thisOpSize);

            dcl->setGRFBaseOffset(linearizedStart);
        }
    }
}
