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

/* G4_SrcRegRegion */

unsigned short G4_SrcRegRegion::ExRegNum(bool &valid) const {
    short normRegNum = 0;
    short registerOffset = (regOff == (short)UNDEFINED_SHORT)? 0: regOff;
    short subRegisterOffset = (subRegOff == (short)UNDEFINED_SHORT)? 0: subRegOff;
    if (base->isRegVar())
    {
        G4_RegVar* baseVar = static_cast<G4_RegVar*>(base);
        if(baseVar->isPhyRegAssigned() && baseVar->getPhyReg()->isGreg())    // Greg
        {
            valid = true;
            unsigned RegNum = (static_cast<G4_Greg*>(baseVar->getPhyReg()))->getRegNum();
            unsigned SubRegNum = baseVar->getPhyRegOff();

            int declOpSize(G4_Type_Table[baseVar->getDeclare()->getElemType()].byteSize);
            int thisOpSize(G4_Type_Table[type].byteSize);

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

unsigned short G4_SrcRegRegion::ExSubRegNum(bool &valid){
    valid = true;
    unsigned short subRegNum = 0;
    short normSubRegNum = 0;
    short subRegisterOffset = (subRegOff == (short)UNDEFINED_SHORT)? 0: subRegOff;
    if(base->isRegVar())
    {
        G4_RegVar* baseVar = static_cast<G4_RegVar*>(base);
        if(baseVar->isPhyRegAssigned() && baseVar->getPhyReg()->isAreg())
        {
            normSubRegNum = baseVar->getPhyRegOff() + subRegisterOffset;
            MUST_BE_TRUE(normSubRegNum>=0,
                ERROR_DATA_RANGE("sub-register number"));
            subRegNum = (unsigned short) normSubRegNum;
            if(acc == Direct)
            {
                MUST_BE_TRUE(regOff == 0,
                    ERROR_DATA_RANGE("register offset"));
                int thisOpSize(G4_Type_Table[this->type].byteSize);
                int declOpSize(G4_Type_Table[baseVar->getDeclare()->getElemType()].byteSize);
                if(thisOpSize > declOpSize)
                {
                    MUST_BE_TRUE((thisOpSize/declOpSize) == 2 || (thisOpSize/declOpSize) == 4,
                        ERROR_DATA_RANGE("operand size"));
                    unsigned shiftVal = ((thisOpSize/declOpSize) == 2) ? 1 : 2;
                    subRegNum >>= shiftVal;
                }
                else if(thisOpSize < declOpSize)
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

            int thisOpSize(G4_Type_Table[this->type].byteSize);
            int declOpSize(G4_Type_Table[baseVar->getDeclare()->getElemType()].byteSize);

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
    if(subRegOff == (short)UNDEFINED_SHORT)
        valid = false;
    return subRegNum;
}

unsigned short G4_SrcRegRegion::ExIndRegNum(bool &valid){
    return base->ExIndRegNum(valid);
}

unsigned short G4_SrcRegRegion::ExIndSubRegNum(bool &valid){
    if(base->isRegVar())
    {
        short subRegisterOffset = (subRegOff == (short)UNDEFINED_SHORT)? 0: subRegOff;
        short normSubRegNum = (static_cast<G4_RegVar*>(base)->getPhyRegOff() + subRegisterOffset);
        MUST_BE_TRUE(normSubRegNum>=0,
            ERROR_DATA_RANGE("sub-register number"));
        return ((unsigned short) normSubRegNum);
    }
    return ExSubRegNum(valid);
}

short G4_SrcRegRegion::ExIndImmVal(void){
    return immAddrOff;
}

bool G4_SrcRegRegion::ExNegMod(bool &valid){
    bool negMod = false;
    valid = false;
    switch(mod)
    {
    case Mod_Minus:
    case Mod_Minus_Abs:
        valid  = true;
        negMod = true;
        break;
    default:
        break; // Prevent gcc warning
    }
    return negMod;
}

/* G4_DstRegRegion */

unsigned short G4_DstRegRegion::ExRegNum(bool &valid){
    short normRegNum = 0;
    short registerOffset = (regOff == (short)UNDEFINED_SHORT)? 0: regOff;
    short subRegisterOffset = (subRegOff == (short)UNDEFINED_SHORT)? 0: subRegOff;
    if (base->isRegVar())
    {
        G4_RegVar* baseVar = static_cast<G4_RegVar*>(base);
        if(baseVar->isPhyRegAssigned() && baseVar->getPhyReg()->isGreg())
        {
            valid = true;
            unsigned RegNum = (static_cast<G4_Greg*>(baseVar->getPhyReg()))->getRegNum();
            unsigned SubRegNum = baseVar->getPhyRegOff();

            int declOpSize(G4_Type_Table[baseVar->getDeclare()->getElemType()].byteSize);
            int thisOpSize(G4_Type_Table[type].byteSize);

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

unsigned short G4_DstRegRegion::ExSubRegNum(bool &valid){
    valid = true;
    unsigned short subRegNum = 0;
    short normSubRegNum = 0;
    short subRegisterOffset = (subRegOff == (short)UNDEFINED_SHORT)? 0: subRegOff;
    if(base->isRegVar())
    {
        G4_RegVar* baseVar = static_cast<G4_RegVar*>(base);
        if(baseVar->isPhyRegAssigned() && baseVar->getPhyReg()->isAreg())
        {
            normSubRegNum = baseVar->getPhyRegOff() + subRegisterOffset;
            MUST_BE_TRUE(normSubRegNum>=0,
                ERROR_DATA_RANGE("sub-register number"));
            subRegNum = (unsigned short) normSubRegNum;
            if(acc == Direct)
            {
                MUST_BE_TRUE(regOff == 0,
                    ERROR_DATA_RANGE("register offset"));
                int thisOpSize(G4_Type_Table[this->type].byteSize);
                int declOpSize(G4_Type_Table[baseVar->getDeclare()->getElemType()].byteSize);
                if(thisOpSize > declOpSize)
                {
                    MUST_BE_TRUE((thisOpSize/declOpSize) == 2 || (thisOpSize/declOpSize) == 4,
                        ERROR_DATA_RANGE("operand size"));
                    unsigned shiftVal = ((thisOpSize/declOpSize) == 2) ? 1 : 2;
                    subRegNum >>= shiftVal;
                }
                else if(thisOpSize < declOpSize)
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

            int thisOpSize(G4_Type_Table[this->type].byteSize);
            int declOpSize(G4_Type_Table[baseVar->getDeclare()->getElemType()].byteSize);

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
    if(subRegOff == (short)UNDEFINED_SHORT)
        valid = false;
    return subRegNum;
}

unsigned short G4_DstRegRegion::ExIndRegNum(bool &valid){
    return base->ExIndRegNum(valid);
}

unsigned short G4_DstRegRegion::ExIndSubRegNum(bool &valid){
    if(base->isRegVar())
    {
        short subRegisterOffset = (subRegOff == (short)UNDEFINED_SHORT)? 0: subRegOff;
        short normSubRegNum = (static_cast<G4_RegVar*>(base)->getPhyRegOff() + subRegisterOffset);
        MUST_BE_TRUE(normSubRegNum>=0,
            ERROR_DATA_RANGE("sub-register number"));
        return ((unsigned short) normSubRegNum);
    }
    return ExSubRegNum(valid);
}

short G4_DstRegRegion::ExIndImmVal(void){
    return immAddrOff;
}
