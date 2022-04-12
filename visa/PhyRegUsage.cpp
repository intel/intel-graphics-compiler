/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "PhyRegUsage.h"
#include "FlowGraph.h"
#include "GraphColor.h"

using namespace vISA;

PhyRegUsage::PhyRegUsage(PhyRegUsageParms& p) :
    gra(p.gra),
    lrs(p.lrs),
    availableGregs(p.availableGregs),
    availableSubRegs(p.availableSubRegs),
    availableAddrs(p.availableAddrs),
    availableFlags(p.availableFlags),
    colorHeuristic(FIRST_FIT),
    startARFReg(p.startARFReg),
    startFLAGReg(p.startFlagReg),
    startGRFReg(p.startGRFReg),
    bank1_start(p.bank1_start),
    bank2_start(p.bank2_start),
    bank1_end(p.bank1_end),
    bank2_end(p.bank2_end),
    totalGRFNum(p.totalGRF),
    honorBankBias(p.doBankConflict),
    builder(*p.gra.kernel.fg.builder),
    regPool(p.gra.regPool)
{
    maxGRFCanBeUsed = p.maxGRFCanBeUsed;
    regFile = p.rFile;

    weakEdgeUsage = p.weakEdgeUsage;
    overlapTest = false;

    if (regFile == G4_GRF)
    {
        memset(availableGregs, true, sizeof(bool)* totalGRFNum);
        memset(availableSubRegs, 0xffffffff, sizeof(uint32_t)*totalGRFNum);
        if (weakEdgeUsage)
        {
            memset(weakEdgeUsage, 0, sizeof(uint8_t)* totalGRFNum);
        }
    }
    else if (regFile == G4_ADDRESS)
    {
        auto numAddrRegs = getNumAddrRegisters();
        for (unsigned i = 0; i < numAddrRegs; i++)
            availableAddrs[i] = true;
    }
    else if (regFile == G4_FLAG)
    {
        auto numFlags = builder.getNumFlagRegisters();
        for (unsigned i = 0; i < numFlags; i++)
            availableFlags[i] = true;
    }
}

void PhyRegUsage::markBusyForDclSplit(G4_RegFileKind kind,
    unsigned regNum,
    unsigned regOff,
    unsigned nunits,  //word units
    unsigned numRows)
{
    MUST_BE_TRUE(numRows > 0 && nunits > 0, ERROR_INTERNAL_ARGUMENT);
    MUST_BE_TRUE(regNum + numRows <= maxGRFCanBeUsed, ERROR_UNKNOWN);

    unsigned uwordsPerGRF = builder.numEltPerGRF<Type_UW>();
    unsigned start_GRF = (regNum * uwordsPerGRF + regOff) / uwordsPerGRF;
    unsigned end_GRF = (regNum * uwordsPerGRF + regOff + nunits) / uwordsPerGRF;

    unsigned start_sub_GRF = (regNum * uwordsPerGRF + regOff) % uwordsPerGRF;
    unsigned end_sub_GRF = (regNum * uwordsPerGRF + regOff + nunits) % uwordsPerGRF;

    for (unsigned i = start_GRF; i < end_GRF; i++)
    {
        availableGregs[i] = false;
        if (builder.getGRFSize() == 64)
            availableSubRegs[i] = 0;
        else
            availableSubRegs[i] = 0xffff0000;
    }

    if (end_sub_GRF)
    {
        availableGregs[end_GRF] = false;
        if (start_GRF == end_GRF)
        {
            auto subregMask = getSubregBitMask(start_sub_GRF, nunits);
            availableSubRegs[end_GRF] &= ~subregMask;
        }
        else
        {
            auto subregMask = getSubregBitMask(0, end_sub_GRF);
            availableSubRegs[end_GRF] &= ~subregMask;
        }
    }
}
//
// mark availRegs[start ... start+num-1] free again
//
void PhyRegUsage::freeContiguous(bool availRegs[],
    unsigned start,
    unsigned num,
    unsigned maxRegs)
{
    for (unsigned i = start; i < start + num; i++)
    {
        MUST_BE_TRUE(i < maxRegs && availRegs[i] == false,
            ERROR_UNKNOWN);
        availRegs[i] = true;
    }
}
//
// return true if all entries are true
//
bool PhyRegUsage::allFree(bool availRegs[], unsigned maxRegs)
{
    for (unsigned i = 0; i < maxRegs; i++)
    {
        if (availRegs[i] == false)
            return false;
    }
    return true;
}

//
// mark sub reg [regOff .. regOff + nbytes -1] of the reg regNum free
//
void PhyRegUsage::freeGRFSubReg(unsigned regNum,
    unsigned regOff,
    unsigned nwords,
    G4_Type  ty)
{
    //
    // adjust regOff to its corresponding word position
    //

    int startWord = regOff * TypeSize(ty) / G4_WSIZE;
    auto subregMask = getSubregBitMask(startWord, nwords);
    availableSubRegs[regNum] |= subregMask;

    //
    // if all sub regs of regNum are free, then unlink the reg
    //
    if (availableSubRegs[regNum] == 0xFFFFFFFF)
    {
        MUST_BE_TRUE(!availableGregs[regNum],
            ERROR_UNKNOWN);
        availableGregs[regNum] = true;
    }
}

//
// free registers that are held by intv
//
void PhyRegUsage::freeRegs(LiveRange* varBasis)
{
    G4_Declare* decl = varBasis->getDcl();
    G4_RegFileKind kind = decl->getRegFile();
    MUST_BE_TRUE(varBasis->getPhyReg(),
        ERROR_UNKNOWN);
    if (decl->useGRF())
    {
        MUST_BE_TRUE(varBasis->getPhyReg()->isGreg(), ERROR_UNKNOWN);
        if (canGRFSubRegAlloc(decl))
        {
            freeGRFSubReg(((G4_Greg*)varBasis->getPhyReg())->getRegNum(), varBasis->getPhyRegOff(),
                numAllocUnit(decl->getNumElems(), decl->getElemType()), decl->getElemType());
        }
        else
        {
            freeContiguous(availableGregs, ((G4_Greg*)varBasis->getPhyReg())->getRegNum(),
                decl->getNumRows(), totalGRFNum);
        }
    }
    else if (kind == G4_ADDRESS)
    {
        MUST_BE_TRUE(varBasis->getPhyReg()->isAreg(), ERROR_UNKNOWN);
        freeContiguous(availableAddrs, varBasis->getPhyRegOff(),
            numAllocUnit(decl->getNumElems(), decl->getElemType()), getNumAddrRegisters());
    }
    else if (kind == G4_FLAG)
    {
        MUST_BE_TRUE(varBasis->getPhyReg()->isFlag(), ERROR_UNKNOWN);
        freeContiguous(availableFlags, varBasis->getPhyRegOff(),
            numAllocUnit(decl->getNumElems(), decl->getElemType()), builder.getNumFlagRegisters());
    }
    else // not yet handled
        MUST_BE_TRUE(false, ERROR_UNKNOWN);
}

static int getSubAlignInWords(G4_SubReg_Align subAlign)
{
    return static_cast<int>(subAlign);
}

unsigned short PhyRegUsage::getOccupiedBundle(const G4_Declare* dcl) const
{
    unsigned short occupiedBundles = 0;
    unsigned bundleNum = 0;
    if (!builder.getOption(vISA_enableBundleCR))
    {
        return occupiedBundles;
    }

    if (!builder.hasDPAS() || !builder.getOption(vISA_EnableDPASBundleConflictReduction))
    {
        return 0;
    }

    for (const BundleConflict& conflict : gra.getBundleConflicts(dcl))
    {
        unsigned reg = -1;
        int offset = 0;

        offset = conflict.offset;
        const G4_RegVar* regVar = conflict.dcl->getRegVar();
        if (regVar->isPhyRegAssigned())
        {
            reg = regVar->getPhyReg()->asGreg()->getRegNum();
        }
        else
        {
            LiveRange* lr = lrs[regVar->getId()];
            if (lr && lr->getPhyReg())
            {
                reg = lr->getPhyReg()->asGreg()->getRegNum();
            }
        }

        if (reg != -1)
        {
            unsigned bundle = gra.get_bundle(reg, offset);
            unsigned bundle1 = gra.get_bundle(reg, offset + 1);
            if (!(occupiedBundles & ((unsigned short)1 << bundle)))
            {
                bundleNum++;
            }
            occupiedBundles |= (unsigned short)1 << bundle;
            occupiedBundles |= (unsigned short)1 << bundle1;
        }
    }
    if (bundleNum > 12)
    {
        occupiedBundles = 0;
    }

    return occupiedBundles;
}

// returns the starting word index if we find enough free contiguous words satisfying alignment,
// -1 otherwise
int PhyRegUsage::findContiguousWords(
    uint32_t words,
    G4_SubReg_Align subAlign,
    int numWords) const
{
    // early exit in (false?) hope of saving compile time
    if (words == 0)
    {
        return -1;
    }

    int step = getSubAlignInWords(subAlign);
    int startWord = 0;
    int uwordsPerGRF = builder.numEltPerGRF<Type_UW>();

    for (int i = startWord; i + numWords <= uwordsPerGRF; i += step)
    {
        uint32_t bitMask = getSubregBitMask(i, numWords);
        if ((bitMask & words) == bitMask)
        {
            return i;
        }
    }

    return -1;
}

//
// look for contiguous available regs starting from startPos
//
bool PhyRegUsage::findContiguousGRF(bool availRegs[],
    const bool forbidden[],
    unsigned occupiedBundles,
    BankAlign align,
    unsigned numRegNeeded,
    unsigned maxRegs,
    unsigned& startPos,
    unsigned& idx,
    bool forceCalleeSaveOnly,
    bool isEOTSrc)
{
    unsigned startPosRunOne = startPos;
    unsigned endPosRunOne = maxRegs;

    if (isEOTSrc && (startPosRunOne >= maxRegs))
    {
        return false;
    }
    else
    {
        MUST_BE_TRUE(startPosRunOne < maxRegs, ERROR_UNKNOWN);
    }
    bool found =
        findContiguousNoWrapGRF(
        availRegs, forbidden, occupiedBundles, align, numRegNeeded, startPosRunOne, endPosRunOne, idx);

    if (startPosRunOne > 0 && found == false && !isEOTSrc && !forceCalleeSaveOnly)
    {
        unsigned startPosRunTwo = 0;
        unsigned endPosRunTwo = startPos + numRegNeeded;
        endPosRunTwo = std::min(endPosRunTwo, maxRegs);
        MUST_BE_TRUE(endPosRunTwo > 0 && endPosRunTwo <= maxRegs, ERROR_UNKNOWN);
        found =
            findContiguousNoWrapGRF(
            availRegs, forbidden, occupiedBundles, align, numRegNeeded, startPosRunTwo, endPosRunTwo, idx);
    }

    if (found)
    {
        MUST_BE_TRUE(idx < maxRegs && idx + numRegNeeded <= maxRegs, ERROR_UNKNOWN);

        if (colorHeuristic == ROUND_ROBIN) {
            startPos = (idx + numRegNeeded) % maxRegs;
        }
    }

    return found;
}

//
// look for contiguous available regs starting from startPos
//
bool PhyRegUsage::findContiguousAddrFlag(bool availRegs[],
    const bool forbidden[],
    G4_SubReg_Align subAlign,
    unsigned numRegNeeded,
    unsigned maxRegs,
    unsigned& startPos,
    unsigned& idx,
    bool isCalleeSaveBias,
    bool isEOTSrc)
{
    unsigned startPosRunOne = startPos;
    unsigned endPosRunOne = maxRegs;

    if (isEOTSrc && (startPosRunOne >= maxRegs))
    {
        return false;
    }
    else
    {
        MUST_BE_TRUE(startPosRunOne < maxRegs, ERROR_UNKNOWN);
    }
    bool found =
        findContiguousNoWrapAddrFlag(
        availRegs, forbidden, subAlign, numRegNeeded, startPosRunOne, endPosRunOne, idx);

    if (startPosRunOne > 0 && found == false && !isEOTSrc && !isCalleeSaveBias)
    {
        unsigned startPosRunTwo = 0;
        unsigned endPosRunTwo = startPos + numRegNeeded;
        endPosRunTwo = std::min(endPosRunTwo, maxRegs);
        MUST_BE_TRUE(endPosRunTwo > 0 && endPosRunTwo <= maxRegs, ERROR_UNKNOWN);
        found =
            findContiguousNoWrapAddrFlag(
            availRegs, forbidden, subAlign, numRegNeeded, startPosRunTwo, endPosRunTwo, idx);
    }

    if (found)
    {
        MUST_BE_TRUE(idx < maxRegs && idx + numRegNeeded <= maxRegs, ERROR_UNKNOWN);

        if (colorHeuristic == ROUND_ROBIN) {
            startPos = (idx + numRegNeeded) % maxRegs;
        }
    }

    return found;
}

bool PhyRegUsage::findContiguousGRFFromBanks(G4_Declare *dcl,
    bool availRegs[],
    const bool forbidden[],
    BankAlign origAlign,
    unsigned& idx,
    bool oneGRFBankDivision)
{   // EOT is not handled in this function
    bool found = false;
    unsigned numRegNeeded = dcl->getNumRows();
    auto dclBC = gra.getBankConflict(dcl);
    bool gotoSecondBank = (dclBC == BANK_CONFLICT_SECOND_HALF_EVEN ||
        dclBC == BANK_CONFLICT_SECOND_HALF_ODD) && (dcl->getNumRows() > 1);

    BankAlign align = origAlign;
    if (dclBC != BANK_CONFLICT_NONE && align == BankAlign::Either && dcl->getNumRows() <= 1)
    {
        align = gra.getBankAlign(dcl);
    }

    ASSERT_USER(bank1_end < totalGRFNum && bank1_start < totalGRFNum && bank2_start < totalGRFNum && bank2_end < totalGRFNum,
        "Wrong bank boundaries value");

    if (colorHeuristic == ROUND_ROBIN)
    {
        //For round robin, bank1_end and bank2_end are fixed.
        if (gotoSecondBank)  //For odd aligned varaibe, we put them to a specific sections.
        {
            //From maxGRFCanBeUsed - 1 to bank2_end
            ASSERT_USER(bank2_start >= bank2_end, "Second bank's start can not less than end\n");

            if ((bank2_start - bank2_end + 1) >= numRegNeeded) //3 - 2 + 1 >= 2
            {
                found = findFreeRegs(
                    availRegs, forbidden, align, numRegNeeded, bank2_start, bank2_end, idx, gotoSecondBank, oneGRFBankDivision);
            }

            if (!found)
            {
                if (maxGRFCanBeUsed - 1 >= bank2_start + numRegNeeded)
                {
                    found = findFreeRegs(
                        availRegs, forbidden, align, numRegNeeded, maxGRFCanBeUsed - 1, bank2_start + 1, idx, gotoSecondBank, oneGRFBankDivision);
                }
                else
                {
                    return false;
                }
            }

            if (found)
            {
                bank2_start = idx - 1;
                if (bank2_start < bank2_end)
                {
                    bank2_start = maxGRFCanBeUsed - 1;
                }
            }
        }
        else
        {   //From 0 to bank1_end
            if (bank1_end - bank1_start + 1 >= numRegNeeded)
            {
                found = findFreeRegs(
                    availRegs, forbidden, BankAlign::Even, numRegNeeded, bank1_start, bank1_end, idx, gotoSecondBank, oneGRFBankDivision);
            }

            if (!found)
            {
                if (bank1_start >= numRegNeeded)
                {
                    found = findFreeRegs(
                        availRegs, forbidden, BankAlign::Even, numRegNeeded, 0, bank1_start - 2 + numRegNeeded, idx, gotoSecondBank, oneGRFBankDivision);
                }
            }

            if (found)
            {
                bank1_start = idx + numRegNeeded;
                if (bank1_start > bank1_end)
                {
                    bank1_start = 0;
                }
            }
        }
    }
    else
    {
        //For first fit, the bank1_start and bank2_start are fixed. bank2_end and bank1_end are dynamically decided, but can not change in one direction (MIN or MAX).
        if (gotoSecondBank)  //For odd aligned varaibe, we put them to a specific sections.
        {
            found = findFreeRegs(
                availRegs, forbidden, align, numRegNeeded, maxGRFCanBeUsed - 1, 0, idx, gotoSecondBank, oneGRFBankDivision);

            if (found)
            {
                bank2_end = std::min(idx, bank2_end);
            }
        }
        else
        {
            found = findFreeRegs(
                availRegs, forbidden, align, numRegNeeded, 0, maxGRFCanBeUsed - 1, idx, gotoSecondBank, oneGRFBankDivision);
            if (found)
            {
                bank1_end = std::max(idx + numRegNeeded - 1, bank1_end);
            }
        }

        if (bank2_end <= bank1_end)
        {
            found = false;
        }

    }

    return found;
}

bool PhyRegUsage::isOverlapValid(unsigned int reg, unsigned int numRegs)
{
    for (unsigned int i = reg; i < (reg + numRegs); i++)
    {
        auto k = getWeakEdgeUse(i);
        if (!(k == 0 ||
            k == (i - reg + 1)))
        {
            // This condition will be taken when there is a partial
            // overlap.
            return false;
        }
    }

    return true;
}

//
// look for contiguous available regs from startPos to maxRegs
//
bool PhyRegUsage::findContiguousNoWrapGRF(bool availRegs[],
    const bool forbidden[],
    unsigned short occupiedBundles,
    BankAlign align,
    unsigned numRegNeeded,
    unsigned startPos,
    unsigned endPos,
    unsigned& idx)
{
    unsigned i = startPos;
    while (i < endPos)
    {
        if (((i & 0x1) && align == BankAlign::Even) || // i is odd but intv needs to be even aligned
            ((i & 0x1) == 0 && align == BankAlign::Odd)) // i is even but intv needs to be odd aligned
        {
            i++;
        }
        else
        {
            if (align == BankAlign::Even2GRF)
            {
                while ((i % 4 >= 2) || ((numRegNeeded >= 2) && (i % 2 != 0)))
                {
                    i++;
                }
            }
            else if (align == BankAlign::Odd2GRF)
            {
                while ((i % 4 < 2) || ((numRegNeeded >= 2) && (i % 2 != 0)))
                {
                    i++;
                }
            }

            if (numRegNeeded == 0 ||
                i + numRegNeeded > endPos)
                return false; // no available regs
            //
            // find contiguous numRegNeeded registers
            // forbidden != NULL then check forbidden
            //
            unsigned j = i;
            if (overlapTest &&
                !isOverlapValid(i, numRegNeeded))
            {
                i++;
            }
            else if (occupiedBundles & (1 << gra.get_bundle(i, 0)) ||
                     occupiedBundles & (1 << gra.get_bundle(i, 1)))
            {
                i++;
            }
            else
            {
                for (; j < i + numRegNeeded && availRegs[j] && (forbidden == NULL || !forbidden[j]); j++);
                if (j == i + numRegNeeded)
                {
                    for (unsigned k = i; k < j; k++) availRegs[k] = false;
                    idx = i;
                    return true;
                }
                else
                    i = j + 1;
            }
        }
    }
    return false; // no available regs
}

//
// look for contiguous available regs from startPos to maxRegs
//
bool PhyRegUsage::findContiguousNoWrapAddrFlag(bool availRegs[],
    const bool forbidden[],
    G4_SubReg_Align subAlign, //Sub align is used only for Flag and Address registers
    unsigned numRegNeeded,
    unsigned startPos,
    unsigned endPos,
    unsigned& idx)
{
    unsigned i = startPos;
    while (i < endPos)
    {
        //
        // some register assignments need special alignment, we check
        // whether the alignment criteria is met.
        //
        if (subAlign == Sixteen_Word && i != 0)
        {    // Sixteen_Word sub-align should have i=0
            return false;
        } else if ((subAlign == Eight_Word && i % 8 != 0) ||    // 8_Word align, i must be divided by 8
            (i & 0x1 && subAlign == Even_Word) || // i is odd but intv needs to be even aligned
            (subAlign == Four_Word && (i % 4 != 0))) // 4_word alignment
            i++;
        else
        {
            if (numRegNeeded == 0 ||
                i + numRegNeeded > endPos)
            {
                return false; // no available regs
            }
            //
            // find contiguous numRegNeeded registers
            // forbidden != NULL then check forbidden
            //
            unsigned j = i;
            for (; j < i + numRegNeeded && availRegs[j] && (forbidden == NULL || !forbidden[j]); j++);
            if (j == i + numRegNeeded)
            {
                for (unsigned k = i; k < j; k++) availRegs[k] = false;
                idx = i;
                return true;
            } else {
                i = j + 1;
            }
        }
    }
    return false; // no available regs
}

bool PhyRegUsage::findFreeRegs(bool availRegs[],
    const bool forbidden[],
    BankAlign align,
    unsigned numRegNeeded,
    unsigned startRegNum,  //inclusive
    unsigned endRegNum, //inclusive: less and equal when startRegNum <= endRegNum, larger and equal when startRegNum > endRegNum
    unsigned& idx,
    bool gotoSecondBank,
    bool oneGRFBankDivision)
{
    bool forward = startRegNum <= endRegNum ? true : false;
    int startReg = forward ? startRegNum : startRegNum - numRegNeeded + 1;
    int endReg = forward ? endRegNum - numRegNeeded + 1 : endRegNum;
    int i = startReg;

    while (1)
    {
        if (forward)
        {
            if (i > endReg)
                break;
        }
        else
        {
            if (i < endReg)
                break;
        }

        if ((align == BankAlign::Even2GRF) && (i % 2 != 0 ||  i % 4 == 3))
        {
            i += forward ? 1 : -1;
            continue;
        }
        else if ((align == BankAlign::Odd2GRF) && (i % 2 != 0 || i % 4 == 1))
        {
            i += forward ? 1 : -1;
            continue;
        }
        else if ((((i & 0x1) && align == BankAlign::Even) || // i is odd but intv needs to be even aligned
            (((i & 0x1) == 0) && (align == BankAlign::Odd)))) // i is even but intv needs to be odd aligned
        {
            i += forward ? 1 : -1;
            continue;
        }
        else
        {
            if ((forward && (i > endReg)) ||
                (!forward && (i < endReg)))
            {
                return false; // no available regs
            }

            if (regFile == G4_GRF &&
                overlapTest &&
                !isOverlapValid(i, numRegNeeded))
            {
                i += forward ? 1 : -1;
            }
            else
            {
                // find contiguous numRegNeeded registers
                // forbidden != NULL then check forbidden
                //
                unsigned j = i;
                for (; j < i + numRegNeeded && availRegs[j] && (forbidden == NULL || !forbidden[j]); j++);
                if (j == i + numRegNeeded)
                {
                    for (unsigned k = i; k < j; k++) availRegs[k] = false;
                    idx = i;
                    return true;
                }
                else
                {   //Jump over the register region which a poke in the end
                    if (forward)
                    {
                        i = j + 1;
                    }
                    else
                    {
                        if (j > numRegNeeded)
                        {
                            i = j - numRegNeeded;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }
        }
    }

    return false;
}

//
// return true, if the var can be allocated using sub reg
//
bool PhyRegUsage::canGRFSubRegAlloc(G4_Declare* decl)
{
    if (decl->getNumRows() != 1) // more than 1 row
        return false;
    if (numAllocUnit(decl->getNumElems(), decl->getElemType()) < builder.numEltPerGRF<Type_UW>())
        return true;
    return false;
}

void PhyRegUsage::findGRFSubRegFromRegs(int startReg,
    int endReg,
    int step,
    PhyReg *phyReg,
    G4_SubReg_Align subAlign,
    unsigned nwords,
    const bool forbidden[],
    bool fromPartialOccupiedReg)
{
    int idx = startReg;
    while (1)
    {
        if (step > 0)
        {
            if (idx > endReg)
            {
                break;
            }
        }
        else
        {
            if (idx < endReg)
            {
                break;
            }
        }

        if (forbidden && forbidden[idx])
        {
            idx += step;
            continue;
        }

        if (fromPartialOccupiedReg && availableSubRegs[idx] == 0xFFFFFFFF)
        {
            // favor partially allocated GRF first
            idx += step;
            continue;
        }

        int subreg = findContiguousWords(availableSubRegs[idx], subAlign, nwords);
        if (subreg != -1)
        {
            phyReg->reg = idx;
            phyReg->subreg = subreg;
            return;
        }

        idx += step;
    }

    return;
}

PhyRegUsage::PhyReg PhyRegUsage::findGRFSubRegFromBanks(G4_Declare *dcl,
    const bool forbidden[],
    bool oneGRFBankDivision)
{
    int startReg = 0, endReg = totalGRFNum;
    int step = 0;
    G4_SubReg_Align subAlign = gra.getSubRegAlign(dcl);
    unsigned nwords = numAllocUnit(dcl->getNumElems(), dcl->getElemType());
    auto dclBC = gra.getBankConflict(dcl);
    bool gotoSecondBank = dclBC == BANK_CONFLICT_SECOND_HALF_EVEN ||
        dclBC == BANK_CONFLICT_SECOND_HALF_EVEN;

    if (gotoSecondBank && oneGRFBankDivision)
    {
        startReg = (maxGRFCanBeUsed - 1);
        startReg = startReg % 2 ? startReg : startReg - 1;
        if (colorHeuristic == ROUND_ROBIN)
        {
            endReg = bank2_end;
        }
        else
        {
            endReg = 0;
        }
        step = -2;
    }
    else if (gotoSecondBank && !oneGRFBankDivision)  //We will depends on low high, treated as even align
    {
        startReg = (maxGRFCanBeUsed - 1);
        startReg = startReg % 2 ? startReg - 1 : startReg;
        if (colorHeuristic == ROUND_ROBIN)
        {
            endReg = bank2_end;
        }
        else
        {
            endReg = 0;
        }
        step = -1;
    }
    else
    {
        if (colorHeuristic == ROUND_ROBIN)
        {
            startReg = 0;
            endReg = bank1_end;
        }
        else
        {
            startReg = 0;
            endReg = maxGRFCanBeUsed - 1;
        }
        if (oneGRFBankDivision)
        {
            step = 2;
        }
        else
        {
            step = 1;
        }
    }

    PhyReg phyReg = { -1, -1 };

    //Try to find sub register from the registers which are partially occupied already.
    findGRFSubRegFromRegs(startReg, endReg, step, &phyReg, subAlign, nwords, forbidden, true);

    //If failed or across the boundary of specified bank, try again and find from the registers which are totally free
    if (phyReg.reg == -1 || (gotoSecondBank && ((unsigned)phyReg.reg <= bank1_end)) || (!gotoSecondBank && ((unsigned)phyReg.reg >= bank2_end)))
    {
        findGRFSubRegFromRegs(startReg, endReg, step, &phyReg, subAlign, nwords, forbidden, false);
    }

    if (phyReg.reg != -1 && colorHeuristic == FIRST_FIT)
    {
        if (gotoSecondBank)
        {
            bank2_end = std::min((unsigned)phyReg.reg, bank2_end);
        }
        else
        {
            bank1_end = std::max((unsigned)phyReg.reg, bank1_end);
        }
        if (bank1_end >= bank2_end)
        {
            phyReg.reg = -1;
        }
    }

    return phyReg;
}

//
// return reg and subRegOff (subr)
// To support sub-reg alignment
//
PhyRegUsage::PhyReg PhyRegUsage::findGRFSubReg(const bool forbidden[],
    bool calleeSaveBias,
    bool callerSaveBias,
    BankAlign align,
    G4_SubReg_Align subAlign,
    unsigned nwords)
{
    int startReg = 0, endReg = totalGRFNum;
    PhyReg phyReg = { -1, -1 };
    if (calleeSaveBias)
    {
        startReg = builder.kernel.calleeSaveStart();
    }
    else if (callerSaveBias)
    {
        endReg = builder.kernel.calleeSaveStart();
    }

    int step = align == BankAlign::Even ? 2 : 1;

    auto findSubGRFAlloc = [step, forbidden, this, subAlign, nwords](unsigned int startReg, unsigned int endReg)
    {
        PhyReg phyReg = { -1, -1 };
        for (auto idx = startReg; idx < endReg; idx += step)
        {
            // forbidden GRF is not an allocation candidate
            if (forbidden && forbidden[idx])
            {
                continue;
            }

            // check if entire GRF is available
            if (availableSubRegs[idx] == 0xFFFFFFFF)
            {
                if (phyReg.reg == -1)
                {
                    // favor partially allocated GRF first so dont
                    // return this assignment yet
                    phyReg.reg = idx;
                    phyReg.subreg = 0;
                }
                continue;
            }

            int subreg = findContiguousWords(availableSubRegs[idx], subAlign, nwords);
            if (subreg != -1)
            {
                phyReg.reg = idx;
                phyReg.subreg = subreg;
                return phyReg;
            }
        }

        // either return {-1, -1} or an allocation where entire GRF is available
        return phyReg;
    };

    if (callerSaveBias || calleeSaveBias)
    {
        // attempt bias based assignment first
        phyReg = findSubGRFAlloc(startReg, endReg);
        if (phyReg.subreg != -1)
            return phyReg;
    }

    // Find sub-GRF allocation throughout GRF file
    phyReg = findSubGRFAlloc(0, totalGRFNum);

    return phyReg;
}

bool PhyRegUsage::assignGRFRegsFromBanks(LiveRange*     varBasis,
    BankAlign  align,
    const bool*     forbidden,
    ColorHeuristic  heuristic,
    bool oneGRFBankDivision)
{
    colorHeuristic = heuristic;
    G4_Declare* decl = varBasis->getDcl();

    //
    // if regs are allocated to intv, i is the reg number and off is the reg
    // offset for sub reg allocation
    //
    unsigned i = 0;   // avail reg number

    //
    // determine if we need to do sub reg allcoation
    //
    if (canGRFSubRegAlloc(decl))
    {
        bool retVal = false;

        PhyRegUsage::PhyReg phyReg = findGRFSubRegFromBanks(decl, forbidden, oneGRFBankDivision);
        if (phyReg.reg != -1)
        {
            // based on type, adjust sub reg off accordingly
            // word: stay the same, dword: *2, byte: /2
            // assign r_i.off
            varBasis->setPhyReg(regPool.getGreg(phyReg.reg),
                phyReg.subreg*G4_WSIZE / decl->getElemSize());
            retVal = true;
        }

        return retVal;
    }
    else
    {
        bool success = false;
        if (varBasis->getEOTSrc() && builder.hasEOTGRFBinding())
        {
            bool forceCalleeSaveAlloc = builder.kernel.fg.isPseudoVCEDcl(decl);
            startGRFReg = totalGRFNum - 16;
            success = findContiguousGRF(availableGregs, forbidden, 0, align, decl->getNumRows(), maxGRFCanBeUsed,
                startGRFReg, i, forceCalleeSaveAlloc, true);
        }
        else
        {
            success = findContiguousGRFFromBanks(decl, availableGregs, forbidden, align, i, oneGRFBankDivision);
        }

        if (success)
        {
            varBasis->setPhyReg(regPool.getGreg(i), 0);
        }

        return success;
    }

    return false;
}

//
// find registers for intv
// To support sub-reg alignment
//
bool PhyRegUsage::assignRegs(bool  highInternalConflict,
    LiveRange*         varBasis,
    const bool*     forbidden,
    BankAlign        align,
    G4_SubReg_Align subAlign,
    ColorHeuristic  heuristic,
    float             spillCost,
    bool hintSet)
{
    colorHeuristic = heuristic;

    G4_Declare* decl = varBasis->getDcl();
    G4_RegFileKind kind = decl->getRegFile();
    BankAlign bankAlign = align;

    //
    // if regs are allocated to intv, i is the reg number and off is the reg
    // offset for sub reg allocation
    //
    unsigned i = 0;   // avail reg number

    auto getAlignToUse = [this](BankAlign align, BankAlign bankAlign)
    {
        if (GlobalRA::useGenericAugAlign(builder.getPlatformGeneration()))
            return (align != BankAlign::Either ? align : bankAlign);
        else
            return (bankAlign != BankAlign::Either ? bankAlign : align);
    };

    if (kind == G4_GRF) // general register file
    {
        //
        // determine if we need to do sub reg allcoation
        //
        if (canGRFSubRegAlloc(decl))
        {
            bool retVal = false;
            int oldStartGRFReg = startGRFReg;
            BankConflict varBasisBC = gra.getBankConflict(varBasis->getVar()->asRegVar()->getDeclare());

            if (!builder.getOptions()->getuInt32Option(vISA_ReservedGRFNum) &&
                honorBankBias &&
                varBasisBC != BANK_CONFLICT_NONE)
            {
                if (highInternalConflict)
                {
                    switch (varBasisBC)
                    {
                    case BANK_CONFLICT_FIRST_HALF_EVEN:
                    case BANK_CONFLICT_FIRST_HALF_ODD:
                        startGRFReg = 0;
                        break;
                    case BANK_CONFLICT_SECOND_HALF_EVEN:
                    case BANK_CONFLICT_SECOND_HALF_ODD:
                        startGRFReg = 64;
                        break;
                    default: break;
                    }
                }
                else
                {
                    bankAlign = gra.getBankAlign(varBasis->getVar()->asRegVar()->getDeclare());
                }
            }

            // If the var is biased to receive a callee-bias, start at r60 and wrap around.
            // NOTE: We are assuming a first-fit strategy when a callee-bias is present.
            if (varBasis->getCalleeSaveBias())
            {
                startGRFReg = 60;
            }

            PhyRegUsage::PhyReg phyReg = findGRFSubReg(forbidden, varBasis->getCalleeSaveBias(),
                varBasis->getCallerSaveBias(), getAlignToUse(align, bankAlign), subAlign,
                numAllocUnit(decl->getNumElems(), decl->getElemType()));
            if (phyReg.reg != -1)
            {
                // based on type, adjust sub reg off accordingly
                // word: stay the same, dword: *2, byte: /2
                // assign r_i.off
                varBasis->setPhyReg(regPool.getGreg(phyReg.reg),
                    phyReg.subreg * G4_WSIZE / decl->getElemSize());
                retVal = true;
            }

            if (varBasis->getCalleeSaveBias())
            {
                startGRFReg = oldStartGRFReg;
            }

            return retVal;
        }
        else
        {
            int oldStartGRFReg = startGRFReg;
            unsigned endGRFReg = maxGRFCanBeUsed; // round-robin reg  start bias
            BankConflict varBasisBC = gra.getBankConflict(varBasis->getVar()->asRegVar()->getDeclare());

            if (!builder.getOptions()->getuInt32Option(vISA_ReservedGRFNum) &&
                honorBankBias &&
                varBasisBC != BANK_CONFLICT_NONE &&
                !hintSet)
            {
                if (highInternalConflict)
                {
                    switch (varBasisBC)
                    {
                    case BANK_CONFLICT_FIRST_HALF_EVEN:
                    case BANK_CONFLICT_FIRST_HALF_ODD:
                        startGRFReg = 0;
                        break;
                    case BANK_CONFLICT_SECOND_HALF_EVEN:
                    case BANK_CONFLICT_SECOND_HALF_ODD:
                        startGRFReg = 64;
                        break;
                    default: break;
                    }
                }
                else
                {
                    bankAlign = gra.getBankAlign(varBasis->getVar()->asRegVar()->getDeclare());
                }
            }

            // If the var is biased to receive a callee-bias, start at r60 and wrap around.
            // NOTE: We are assuming a first-fit strategy when a callee-bias is present.
            if (varBasis->getCalleeSaveBias())
            {
                startGRFReg = builder.kernel.calleeSaveStart();
            }

            if (varBasis->getEOTSrc() && builder.hasEOTGRFBinding())
            {
                startGRFReg = totalGRFNum - 16;
            }

            bool forceCalleeSaveAlloc = builder.kernel.fg.isPseudoVCEDcl(decl);
            unsigned short occupiedBundles = getOccupiedBundle(decl);
            bool success = findContiguousGRF(availableGregs, forbidden, occupiedBundles,
                getAlignToUse(align, bankAlign), decl->getNumRows(), endGRFReg,
                startGRFReg, i, forceCalleeSaveAlloc, varBasis->getEOTSrc());
            if (success) {
                varBasis->setPhyReg(regPool.getGreg(i), 0);
            }

            if (varBasis->getEOTSrc())
            {
                startGRFReg = oldStartGRFReg;
            }

            if (varBasis->getCalleeSaveBias())
            {
                startGRFReg = oldStartGRFReg;
            }

            return success;
        }
    }
    else if (kind == G4_ADDRESS) // address register
    {
        MUST_BE_TRUE(decl->getNumRows() == 1, ERROR_UNKNOWN);
        //
        // determine alignment
        // if the number of reg needed is more than 1, then we go ahead
        //
        unsigned regNeeded = numAllocUnit(decl->getNumElems(), decl->getElemType());
        if (findContiguousAddrFlag(availableAddrs, forbidden, subAlign, regNeeded, getNumAddrRegisters(), startARFReg, i))
        {
            // subregoffset should consider the declare data type
            varBasis->setPhyReg(regPool.getAddrReg(), i*G4_WSIZE / decl->getElemSize());
            return true;
        }
        return false;
    }
    else if (kind == G4_FLAG) // Flag register
    {
        MUST_BE_TRUE(decl->getNumRows() == 1, ERROR_UNKNOWN);
        //
        // determine alignment
        // if the number of reg needed is more than 1, then we go ahead
        //
        unsigned regNeeded = numAllocUnit(decl->getNumElems(), decl->getElemType());
        if (findContiguousAddrFlag(availableFlags, forbidden, subAlign, regNeeded, builder.getNumFlagRegisters(), startFLAGReg, i))
        {
            // subregoffset should consider the declare data type
            varBasis->setPhyReg(regPool.getFlagAreg(i / 2), i & 1);
            return true;
        }
        return false;
    }
    else // not handled yet
    {
        MUST_BE_TRUE(false, ERROR_UNKNOWN);
        return false;
    }
}

//
// allocate forbidden vectors
//
unsigned LiveRange::getForbiddenVectorSize() const
{
    switch (regKind)
    {
    case G4_GRF:
    case G4_INPUT:
        return gra.kernel.getNumRegTotal();
    case G4_ADDRESS:
        return getNumAddrRegisters();
    case G4_FLAG:
        return gra.builder.getNumFlagRegisters();
    default:
        assert(false && "illegal reg file");
        return 0;
    }
}

//
// allocate forbidden vectors
//
void LiveRange::allocForbiddenVector(Mem_Manager& mem)
{
    unsigned size = getForbiddenVectorSize();

    if (size > 0)
    {
        forbidden = (bool*)mem.alloc(sizeof(bool)*size);
        memset(forbidden, false, size);
    }
}

void getForbiddenGRFs(
    std::vector<unsigned int>& regNum, G4_Kernel &kernel,
    unsigned stackCallRegSize, unsigned reserveSpillSize, unsigned rerservedRegNum)
{
    // Push forbidden register numbers to vector regNum
    //
    // r0 - Forbidden when platform is not 3d
    // rMax, rMax-1, rMax-2 - Forbidden in presence of stack call sites
    unsigned totalGRFNum = kernel.getNumRegTotal();

    if (kernel.getKernelType() != VISA_3D ||
        kernel.getOption(vISA_enablePreemption) ||
        reserveSpillSize > 0 ||
        kernel.getOption(vISA_ReserveR0) ||
        kernel.getOption(vISA_PreserveR0InR0))
    {
        regNum.push_back(0);
    }

    if (kernel.getOption(vISA_enablePreemption))
    {
        // r1 is reserved for SIP kernel
        regNum.push_back(1);
    }

    unsigned reservedRegSize = stackCallRegSize + reserveSpillSize;
    for (unsigned int i = 0; i < reservedRegSize; i++)
    {
        regNum.push_back(totalGRFNum - 1 - i);
    }

    unsigned largestNoneReservedReg = totalGRFNum - reservedRegSize - 1;
    if (totalGRFNum - reservedRegSize >= totalGRFNum - 16)
    {
        largestNoneReservedReg = totalGRFNum - 16 - 1;
    }


    if (totalGRFNum - reservedRegSize < rerservedRegNum)
    {
        MUST_BE_TRUE(false, "After reservation, there is not enough regiser!");
    }

    for (unsigned int i = 0; i < rerservedRegNum; i++)
    {
        regNum.push_back(largestNoneReservedReg - i);
    }
}

void getCallerSaveGRF(std::vector<unsigned int>& regNum, G4_Kernel* kernel)
{
    unsigned int startCalleeSave = kernel->calleeSaveStart();
    unsigned int endCalleeSave = startCalleeSave + kernel->getNumCalleeSaveRegs();
    // r60-r124 are caller save regs for SKL
    for (unsigned int i = startCalleeSave; i < endCalleeSave; i++)
    {
        regNum.push_back(i);
    }
}

void getCalleeSaveGRF(std::vector<unsigned int>& regNum, G4_Kernel* kernel)
{
    // r1-r59 are callee save regs for SKL
    unsigned int numCallerSaveGRFs = kernel->getCallerSaveLastGRF() + 1;
    for (unsigned int i = 1; i < numCallerSaveGRFs; i++)
    {
        regNum.push_back(i);
    }
}

//
// mark forbidden vectors
//
void LiveRange::allocForbidden(Mem_Manager& mem, bool reserveStackCallRegs, unsigned reserveSpillSize, unsigned rerservedRegNum)
{
    if (forbidden == NULL)
    {
        allocForbiddenVector(mem);
    }


    if (regKind == G4_GRF)
    {
        std::vector<unsigned int> forbiddenGRFs;
        unsigned int stackCallRegSize = reserveStackCallRegs ? gra.kernel.numReservedABIGRF() : 0;
        getForbiddenGRFs(forbiddenGRFs, gra.kernel, stackCallRegSize, reserveSpillSize, rerservedRegNum);

        for (unsigned int i = 0; i < forbiddenGRFs.size(); i++)
        {
            unsigned int regNum = forbiddenGRFs[i];
            forbidden[regNum] = true;
        }
    }
}

//
// mark forbidden registers for caller-save pseudo var
//
void LiveRange::allocForbiddenCallerSave(Mem_Manager& mem, G4_Kernel* kernel)
{
    if (forbidden == NULL)
    {
        allocForbiddenVector(mem);
    }

    MUST_BE_TRUE(regKind == G4_GRF, ERROR_UNKNOWN);

    std::vector<unsigned int> callerSaveRegs;
    getCallerSaveGRF(callerSaveRegs, kernel);
    for (unsigned int i = 0; i < callerSaveRegs.size(); i++)
    {
        unsigned int callerSaveReg = callerSaveRegs[i];
        forbidden[callerSaveReg] = true;
    }
}

//
// mark forbidden registers for callee-save pseudo var
//
void LiveRange::allocForbiddenCalleeSave(Mem_Manager& mem, G4_Kernel* kernel)
{
    if (forbidden == NULL)
    {
        allocForbiddenVector(mem);
    }

    MUST_BE_TRUE(regKind == G4_GRF, ERROR_UNKNOWN);

    std::vector<unsigned int> calleeSaveRegs;
    getCalleeSaveGRF(calleeSaveRegs, kernel);
    for (unsigned int i = 0; i < calleeSaveRegs.size(); i++)
    {
        unsigned int calleeSaveReg = calleeSaveRegs[i];
        forbidden[calleeSaveReg] = true;
    }
}

//
// print assigned reg info
//
void LiveRange::dump() const
{
    G4_Declare* decl = var->getDeclare();
    this->emit(std::cout);
    std::cout << " : ";
    //
    // print alignment
    //
    std::cout << "\t";
    if (gra.getSubRegAlign(decl) == Any)
    {
        std::cout << "\t";
    }
    else {
        std::cout  << gra.getSubRegAlign(decl) << "_words SubReg_Align";
    }
    //
    // dump number of registers that are needed
    //
    if (decl->getRegFile() == G4_ADDRESS)
    {
        std::cout << " + " << (IS_DTYPE(decl->getElemType()) ? 2 * decl->getNumElems() : decl->getNumElems()) << " regs";
    }
    else
    {
        std::cout << "\t(" << decl->getNumRows() << "x" << decl->getNumElems() << "):"
            << TypeSymbol(decl->getElemType());
    }
}

PhyRegUsageParms::PhyRegUsageParms(
    GlobalRA& g, LiveRange* l[], G4_RegFileKind r, unsigned int m,
    unsigned int& startARF, unsigned int& startFlag, unsigned int& startGRF,
    unsigned int& bank1_s, unsigned int& bank1_e, unsigned int& bank2_s, unsigned int& bank2_e,
    bool doBC, bool* avaGReg, uint32_t* avaSubReg,
    bool* avaAddrs, bool* avaFlags, uint8_t* weakEdges)
    : gra(g), startARFReg(startARF), startFlagReg(startFlag), startGRFReg(startGRF),
    bank1_start(bank1_s), bank1_end(bank1_e), bank2_start(bank2_s), bank2_end(bank2_e)
{
    doBankConflict = doBC;
    availableGregs = avaGReg;
    availableSubRegs = avaSubReg;
    availableAddrs = avaAddrs;
    availableFlags = avaFlags;
    weakEdgeUsage = weakEdges;
    maxGRFCanBeUsed = m;
    rFile = r;
    totalGRF = gra.kernel.getNumRegTotal();
    lrs = l;
}
