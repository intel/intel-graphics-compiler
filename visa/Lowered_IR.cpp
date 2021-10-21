/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "FlowGraph.h"
using namespace vISA;

/* G4_SrcRegRegion */

unsigned short G4_SrcRegRegion::ExRegNum(bool &valid) const {
    short normRegNum = 0;
    short registerOffset = (regOff == (short)UNDEFINED_SHORT)? 0: regOff;
    short subRegisterOffset = (subRegOff == (short)UNDEFINED_SHORT)? 0: subRegOff;
    if (base->isRegVar())
    {
        G4_RegVar* baseVar = static_cast<G4_RegVar*>(base);
        if (baseVar->isPhyRegAssigned() && baseVar->getPhyReg()->isGreg())    // Greg
        {
            valid = true;
            unsigned RegNum = (static_cast<G4_Greg*>(baseVar->getPhyReg()))->getRegNum();
            unsigned SubRegNum = baseVar->getPhyRegOff();

            int declOpSize = TypeSize(baseVar->getDeclare()->getElemType());
            int thisOpSize = TypeSize(type);

            if (thisOpSize != declOpSize)
            {
                MUST_BE_TRUE((SubRegNum * declOpSize) % thisOpSize == 0,
                    ERROR_DATA_RANGE("sub-register number"));
                SubRegNum = (SubRegNum * declOpSize) / thisOpSize;
            }
            short regnum = (SubRegNum + subRegisterOffset) / (32 / thisOpSize);
            normRegNum = RegNum + registerOffset + regnum;
            MUST_BE_TRUE(normRegNum>=0,
                ERROR_DATA_RANGE("register number"));
            return ((unsigned short)normRegNum);
        }
    }
    normRegNum = base->ExRegNum(valid)+registerOffset;
    MUST_BE_TRUE(normRegNum>=0,
        ERROR_DATA_RANGE("register number"));
    return ((unsigned short)normRegNum);
}

unsigned short G4_SrcRegRegion::ExSubRegNum(bool &valid) {
    valid = true;
    unsigned short subRegNum = 0;
    short normSubRegNum = 0;
    short subRegisterOffset = (subRegOff == (short)UNDEFINED_SHORT)? 0: subRegOff;
    if (base->isRegVar())
    {
        G4_RegVar* baseVar = static_cast<G4_RegVar*>(base);
        if (baseVar->isPhyRegAssigned() && baseVar->getPhyReg()->isAreg())
        {
            normSubRegNum = baseVar->getPhyRegOff() + subRegisterOffset;
            MUST_BE_TRUE(normSubRegNum>=0,
                ERROR_DATA_RANGE("sub-register number"));
            subRegNum = (unsigned short) normSubRegNum;
            if (acc == Direct)
            {
                MUST_BE_TRUE(regOff == 0,
                    ERROR_DATA_RANGE("register offset"));
                int thisOpSize = getTypeSize();
                int declOpSize = TypeSize(baseVar->getDeclare()->getElemType());
                if (thisOpSize > declOpSize)
                {
                    MUST_BE_TRUE((thisOpSize/declOpSize) == 2 || (thisOpSize/declOpSize) == 4,
                        ERROR_DATA_RANGE("operand size"));
                    unsigned shiftVal = ((thisOpSize/declOpSize) == 2) ? 1 : 2;
                    subRegNum >>= shiftVal;
                }
                else if (thisOpSize < declOpSize)
                {
                    MUST_BE_TRUE((declOpSize/thisOpSize) == 2 || (declOpSize/thisOpSize) == 4,
                        ERROR_DATA_RANGE("operand size"));
                    unsigned shiftVal = ((declOpSize/thisOpSize) == 2) ? 1 : 2;
                    subRegNum <<= shiftVal;
                }
                return subRegNum;
            }
        }
        else if (baseVar->isPhyRegAssigned() &&
            (baseVar->getPhyReg()->isGreg()))
        {
            normSubRegNum = (short)baseVar->getPhyRegOff();

            int thisOpSize = getTypeSize();
            int declOpSize = TypeSize(baseVar->getDeclare()->getElemType());

            if (thisOpSize != declOpSize)
            {
                MUST_BE_TRUE((normSubRegNum * declOpSize) % thisOpSize == 0,
                    ERROR_DATA_RANGE("sub-register number"));
                normSubRegNum = (normSubRegNum * declOpSize) / thisOpSize;
            }
            normSubRegNum = (normSubRegNum + subRegisterOffset) % (32 / thisOpSize);
            MUST_BE_TRUE(normSubRegNum>=0,
                ERROR_DATA_RANGE("sub-register number"));
            subRegNum = (unsigned short) normSubRegNum;
            return subRegNum;
        }
    }
    normSubRegNum = subRegisterOffset;
    MUST_BE_TRUE(normSubRegNum>=0,
        ERROR_DATA_RANGE("sub-register number"));
    subRegNum = (unsigned short) normSubRegNum;
    if (subRegOff == (short)UNDEFINED_SHORT)
        valid = false;
    return subRegNum;
}

unsigned short G4_SrcRegRegion::ExIndSubRegNum(bool &valid) {
    if (base->isRegVar())
    {
        short subRegisterOffset = (subRegOff == (short)UNDEFINED_SHORT)? 0: subRegOff;
        short normSubRegNum = (static_cast<G4_RegVar*>(base)->getPhyRegOff() + subRegisterOffset);
        MUST_BE_TRUE(normSubRegNum>=0,
            ERROR_DATA_RANGE("sub-register number"));
        return ((unsigned short) normSubRegNum);
    }
    return ExSubRegNum(valid);
}

short G4_SrcRegRegion::ExIndImmVal(void) {
    return immAddrOff;
}

/* G4_DstRegRegion */

unsigned short G4_DstRegRegion::ExRegNum(bool &valid) {
    short normRegNum = 0;
    short registerOffset = (regOff == (short)UNDEFINED_SHORT)? 0: regOff;
    short subRegisterOffset = (subRegOff == (short)UNDEFINED_SHORT)? 0: subRegOff;
    if (base->isRegVar())
    {
        G4_RegVar* baseVar = static_cast<G4_RegVar*>(base);
        if (baseVar->isPhyRegAssigned() && baseVar->getPhyReg()->isGreg())
        {
            valid = true;
            unsigned RegNum = (static_cast<G4_Greg*>(baseVar->getPhyReg()))->getRegNum();
            unsigned SubRegNum = baseVar->getPhyRegOff();

            int declOpSize = TypeSize(baseVar->getDeclare()->getElemType());
            int thisOpSize = TypeSize(type);

            if (thisOpSize != declOpSize)
            {
                MUST_BE_TRUE((SubRegNum * declOpSize) % thisOpSize == 0,
                    ERROR_DATA_RANGE("operand size"));
                SubRegNum = (SubRegNum * declOpSize) / thisOpSize;
            }
            short regnum = (SubRegNum + subRegisterOffset) / (32 / thisOpSize);
            normRegNum = RegNum + registerOffset + regnum;
            MUST_BE_TRUE(normRegNum>=0,
                ERROR_DATA_RANGE("register number"));
            return ((unsigned short)normRegNum);
        }
    }
    normRegNum = base->ExRegNum(valid)+registerOffset;
    MUST_BE_TRUE(normRegNum>=0,
        ERROR_DATA_RANGE("register number"));
    return ((unsigned short)normRegNum);
}

unsigned short G4_DstRegRegion::ExSubRegNum(bool &valid) {
    valid = true;
    unsigned short subRegNum = 0;
    short normSubRegNum = 0;
    short subRegisterOffset = (subRegOff == (short)UNDEFINED_SHORT)? 0: subRegOff;
    if (base->isRegVar())
    {
        G4_RegVar* baseVar = static_cast<G4_RegVar*>(base);
        if (baseVar->isPhyRegAssigned() && baseVar->getPhyReg()->isAreg())
        {
            normSubRegNum = baseVar->getPhyRegOff() + subRegisterOffset;
            MUST_BE_TRUE(normSubRegNum>=0,
                ERROR_DATA_RANGE("sub-register number"));
            subRegNum = (unsigned short) normSubRegNum;
            if (acc == Direct)
            {
                MUST_BE_TRUE(regOff == 0,
                    ERROR_DATA_RANGE("register offset"));
                int thisOpSize = getTypeSize();
                int declOpSize = TypeSize(baseVar->getDeclare()->getElemType());
                if (thisOpSize > declOpSize)
                {
                    MUST_BE_TRUE((thisOpSize/declOpSize) == 2 || (thisOpSize/declOpSize) == 4 || (thisOpSize / declOpSize) == 8,
                        ERROR_DATA_RANGE("operand size"));
                    unsigned shiftVal = ((thisOpSize/declOpSize) == 2) ? 1 : 2;
                    subRegNum >>= shiftVal;
                }
                else if (thisOpSize < declOpSize)
                {
                    MUST_BE_TRUE((declOpSize/thisOpSize) == 2 || (declOpSize/thisOpSize) == 4,
                        ERROR_DATA_RANGE("operand size"));
                    unsigned shiftVal = ((declOpSize/thisOpSize) == 2) ? 1 : 2;
                    subRegNum <<= shiftVal;
                }
                return subRegNum;
            }
        }
        else if (baseVar->isPhyRegAssigned() &&
            (baseVar->getPhyReg()->isGreg()))
        {
            normSubRegNum = (short)baseVar->getPhyRegOff();

            int thisOpSize = getTypeSize();
            int declOpSize = TypeSize(baseVar->getDeclare()->getElemType());

            if (thisOpSize != declOpSize)
            {
                MUST_BE_TRUE((normSubRegNum * declOpSize) % thisOpSize == 0,
                    ERROR_DATA_RANGE("operand size"));
                normSubRegNum = (normSubRegNum * declOpSize) / thisOpSize;
            }
            normSubRegNum = (normSubRegNum + subRegisterOffset) % (32 / thisOpSize);
            MUST_BE_TRUE(normSubRegNum>=0,
                ERROR_DATA_RANGE("sub-register number"));
            subRegNum = (unsigned short) normSubRegNum;
            return subRegNum;
        }
    }
    normSubRegNum = subRegisterOffset;
    MUST_BE_TRUE(normSubRegNum>=0,
        ERROR_DATA_RANGE("sub-register number"));
    subRegNum = (unsigned short) normSubRegNum;
    if (subRegOff == (short)UNDEFINED_SHORT)
        valid = false;
    return subRegNum;
}

unsigned short G4_DstRegRegion::ExIndSubRegNum(bool &valid) {
    if (base->isRegVar())
    {
        short subRegisterOffset = (subRegOff == (short)UNDEFINED_SHORT)? 0: subRegOff;
        short normSubRegNum = (static_cast<G4_RegVar*>(base)->getPhyRegOff() + subRegisterOffset);
        MUST_BE_TRUE(normSubRegNum>=0,
            ERROR_DATA_RANGE("sub-register number"));
        return ((unsigned short) normSubRegNum);
    }
    return ExSubRegNum(valid);
}

short G4_DstRegRegion::ExIndImmVal(void) {
    return immAddrOff;
}
