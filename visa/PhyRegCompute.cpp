/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "BuildIR.h"
#include "FlowGraph.h"

using namespace vISA;

void G4_SrcRegRegion::computePReg(const IR_Builder& builder)
{
    int thisOpSize = TypeSize(type);
    unsigned int regNum = 0, subRegNum = 0;
    if (base->isRegVar() && base->asRegVar()->isPhyRegAssigned())
    {
        G4_RegVar* baseVar = base->asRegVar();
        if (baseVar->getPhyReg()->isGreg())
        {
            G4_Declare* dcl = baseVar->getDeclare();

            regNum = (static_cast<G4_Greg*>(baseVar->getPhyReg()))->getRegNum();

            subRegNum = baseVar->getPhyRegOff();

            int declOpSize = dcl->getElemSize();

            if (thisOpSize != declOpSize)
            {
                subRegNum = (subRegNum * declOpSize) / thisOpSize;
            }

            unsigned int linearizedStart = (regNum * builder.numEltPerGRF<Type_UB>()) + (subRegNum * thisOpSize);

            dcl->setGRFBaseOffset(linearizedStart);
        }
#if 0
        if (baseVar->getPhyReg()->isA0())
        {
            G4_Declare* dcl = baseVar->getDeclare();

            subRegNum = baseVar->getPhyRegOff();

            unsigned int linearizedStart = subRegNum * TypeSize(ADDR_REG_TYPE);

            dcl->setGRFBaseOffset(linearizedStart);
        }
#endif
    }
}

void G4_DstRegRegion::computePReg(const IR_Builder& builder)
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

            int declOpSize = dcl->getElemSize();
            int thisOpSize = TypeSize(type);

            if (thisOpSize != declOpSize)
            {
                subRegNum = (subRegNum * declOpSize) / thisOpSize;
            }

            unsigned int linearizedStart = (regNum * builder.numEltPerGRF<Type_UB>()) + (subRegNum *  thisOpSize);

            dcl->setGRFBaseOffset(linearizedStart);
        }

#if 0
        if (baseVar->getPhyReg()->isA0())
        {
            G4_Declare* dcl = baseVar->getDeclare();

            subRegNum = baseVar->getPhyRegOff();

            unsigned int linearizedStart = subRegNum * TypeSize(ADDR_REG_TYPE);

            dcl->setGRFBaseOffset(linearizedStart);
        }
#endif
    }
}
