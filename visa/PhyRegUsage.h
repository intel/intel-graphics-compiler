/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __PHYREGUSAGE_H__
#define __PHYREGUSAGE_H__

#include "BuildIR.h"
#include "G4_Opcode.h"
#include "G4_IR.hpp"

enum ColorHeuristic {FIRST_FIT, ROUND_ROBIN};

// forward declares
namespace vISA
{
class LiveRange;
class GlobalRA;

class PhyRegUsageParms
{
public:
    GlobalRA& gra;
    G4_RegFileKind rFile;
    unsigned int maxGRFCanBeUsed;
    unsigned int& startARFReg;
    unsigned int& startFlagReg;
    unsigned int& startGRFReg;
    unsigned int& bank1_start;
    unsigned int& bank1_end;
    unsigned int& bank2_start;
    unsigned int& bank2_end;
    bool doBankConflict;
    bool* availableGregs;
    uint32_t* availableSubRegs;
    bool* availableAddrs;
    bool* availableFlags;
    uint8_t* weakEdgeUsage;
    unsigned int totalGRF;
    LiveRange** lrs;

    PhyRegUsageParms(GlobalRA& g, LiveRange* l[], G4_RegFileKind r, unsigned int m, unsigned int& startARF, unsigned int& startFlag, unsigned int& startGRF,
        unsigned int& bank1_s, unsigned int& bank1_e, unsigned int& bank2_s, unsigned int& bank2_e, bool doBC, bool* avaGReg,
        uint32_t* avaSubReg, bool* avaAddrs, bool* avaFlags, uint8_t* weakEdges);
};

//
// track which registers are currently in use (cannot be assigned to other variables)
// For sub reg allocation, the granularity is UW/W (2 bytes). Doing so, we only need to
// handle even and odd alignment.
//
class PhyRegUsage
{
    GlobalRA& gra;
    LiveRange** lrs;
    unsigned maxGRFCanBeUsed;
    bool *availableGregs;             // true if the reg is available for allocation
    uint32_t *availableSubRegs;     // each entry is a 16-bit value of the words that are free in a GRF
    bool *availableAddrs;                        // true if the reg is available for allocation
    bool *availableFlags;
    uint8_t* weakEdgeUsage;

    ColorHeuristic colorHeuristic;   // perform register assignment in first-fit/round-robin for GRFs
    G4_RegFileKind regFile;

    unsigned& startARFReg;                        // round-robin reg  start bias
    unsigned& startFLAGReg;
    unsigned& startGRFReg;                        // round-robin reg  start bias


    unsigned &bank1_start;
    unsigned &bank2_start;
    unsigned &bank1_end;
    unsigned &bank2_end;

    unsigned totalGRFNum;

    bool honorBankBias;       // whether we honor the bank bias assigned by the bank conflict avoidance heuristic
    bool overlapTest; // set to true only when current dcl has compatible ranges marked by augmentation

    struct PhyReg
    {
        int reg;
        int subreg; // in unit of words (0-15)
    }; // return type for findGRFSubReg

    PhyReg findGRFSubReg(const bool forbidden[],
        bool callerSaveBias,
        bool callerSaverBias,
        BankAlign align,
        G4_SubReg_Align subAlign,
        unsigned nwords);

    void findGRFSubRegFromRegs(int startReg,
        int endReg,
        int step,
        PhyReg *phyReg,
        G4_SubReg_Align subAlign,
        unsigned nwords,
        const bool forbidden[],
        bool fromPartialOccupiedReg);

    PhyReg findGRFSubRegFromBanks(G4_Declare *dcl,
        const bool forbidden[],
        bool oneGRFBankDivision);

    void freeGRFSubReg(unsigned regNum, unsigned regOff, unsigned nwords, G4_Type ty);
    void freeContiguous(bool availRegs[], unsigned start, unsigned numReg, unsigned maxRegs);
    bool canGRFSubRegAlloc(G4_Declare* decl);
    bool findContiguousNoWrapGRF(bool availRegs[], const bool forbidden[], unsigned short occupiedBundles, BankAlign align, unsigned numRegNeeded, unsigned startPos, unsigned endPos, unsigned & idx);

    bool findContiguousNoWrapAddrFlag(bool availRegs[],
                                   const bool forbidden[],
                                 G4_SubReg_Align subAlign,
                                 unsigned numRegNeeded,
                                 unsigned startPos,
                                 unsigned endPos,
                                 unsigned& idx);

    bool allFree(bool availRegs[], unsigned maxRegs);

    bool findFreeRegs(bool availRegs[],
        const bool forbidden[],
        BankAlign align,
        unsigned numRegNeeded,
        unsigned startRegNum,
        unsigned endRegNum,
        unsigned& idx,
        bool gotoSecondBank,
        bool oneGRFBankDivision);

public:
    IR_Builder& builder;

    PhyRegPool& regPool; // all Physical Reg Operands

    PhyRegUsage(PhyRegUsageParms&);

    bool isOverlapValid(unsigned int, unsigned int);

    void setWeakEdgeUse(unsigned int reg, uint8_t index)
    {
        // Consider V1 is allocated to r10, r11, r12, r13.
        // Then following will be set eventually to model
        // compatible ranges:
        //  weakEdgeUsage[10] = 1;
        //  weakEdgeUsage[11] = 2;
        //  weakEdgeUsage[12] = 3;
        //  weakEdgeUsage[13] = 4;
        // This means some other compatible range cannot start
        // at r7, r8, r9, r11, r12, r13. Another compatible range
        // can either have no overlap at all with this range (strong
        // edge), or it can start at r10 to have full
        // overlap (weak edge).
        weakEdgeUsage[reg] = index;
    }

    uint8_t getWeakEdgeUse(unsigned int reg) const
    {
        return weakEdgeUsage[reg];
    }

    void runOverlapTest(bool t)
    {
        overlapTest = t;
    }

    ~PhyRegUsage()
    {

    }

    bool assignRegs(bool  isSIMD16,
                    LiveRange* var,
                    const bool* forbidden,
                    BankAlign  align,
                    G4_SubReg_Align subAlign,
                    ColorHeuristic colorHeuristic,
                    float spillCost,
                    bool hintSet);

    bool assignGRFRegsFromBanks(LiveRange*     varBasis,
                             BankAlign  align,
                             const bool*     forbidden,
                             ColorHeuristic  heuristic,
                             bool oneGRFBankDivision);

    void markBusyForDclSplit(G4_RegFileKind kind,
        unsigned regNum,
        unsigned regOff,
        unsigned nunits,
        unsigned numRows);

    void markBusyGRF(unsigned regNum,
        unsigned regOff,
        unsigned nunits,
        unsigned numRows,
        bool isPreDefinedVar)
    {
        MUST_BE_TRUE(numRows > 0 && nunits > 0, ERROR_INTERNAL_ARGUMENT);

        MUST_BE_TRUE((regNum + numRows <= maxGRFCanBeUsed) || isPreDefinedVar,
            ERROR_UNKNOWN);

        //
        // sub reg allocation (allocation unit is word)
        //
        if (numRows == 1 && regOff + nunits < builder.numEltPerGRF<Type_UW>())
        {
            availableGregs[regNum] = false;
            auto subregMask = getSubregBitMask(regOff, nunits);
            availableSubRegs[regNum] &= ~subregMask;
        }
        else // allocate whole registers
        {
            for (unsigned i = 0; i < numRows; i++)
            {
                availableGregs[regNum + i] = false;
                if (builder.getGRFSize() == 64)
                    availableSubRegs[regNum + i] = 0;
                else
                    availableSubRegs[regNum + i] = 0xffff0000;
            }
        }
    }

    void markBusyAddress(unsigned regNum,
                  unsigned regOff,
                  unsigned nunits,
        unsigned numRows)
    {
        MUST_BE_TRUE(regNum == 0 && regOff + nunits <= getNumAddrRegisters(),
            ERROR_UNKNOWN);
        for (unsigned i = regOff; i < regOff + nunits; i++)
            availableAddrs[i] = false;
    }

    void markBusyFlag(unsigned regNum,
        unsigned regOff,
        unsigned nunits,
        unsigned numRows)
    {
        for (unsigned i = regOff; i < regOff + nunits; i++)
            availableFlags[i] = false;
    }

    static unsigned numAllocUnit(unsigned nelems, G4_Type ty)
    {
        //
        // we allocate sub reg in 2-byte granularity
        //
        unsigned nbytes = nelems* TypeSize(ty);
        return nbytes/G4_WSIZE + nbytes%G4_WSIZE;
    }

    // translate offset to allocUnit
    static unsigned offsetAllocUnit(unsigned nelems, G4_Type ty)
    {

        unsigned nbytes = nelems * TypeSize(ty);
        //RA allocate register in unit of G4_WSIZE bytes
        //pre-assigned register may start from nbytes%G4_WSIZE != 0, i.e, within an allocUnit
        return nbytes/G4_WSIZE;
    }

    void updateRegUsage(LiveRange* lr);

    uint32_t getSubregBitMask(uint32_t start, uint32_t num) const
    {
        MUST_BE_TRUE(num > 0 && start+num <= builder.numEltPerGRF<Type_UW>(), "illegal number of words");
        uint32_t mask = ((1 << num) - 1) << start;

        return (uint32_t) mask;
    }

    void emit(std::ostream& output)
    {
        output << "available GRFs: ";
        for (unsigned int i = 0; i < totalGRFNum; i++)
        {
            if (availableGregs[i])
            {
                output << i << " ";
            }
        }
        output << "\n";
    }

private:

    void freeRegs(LiveRange* var);

    bool findContiguousAddrFlag(bool availRegs[],
                            const bool forbidden[],
                           G4_SubReg_Align subAlign,
                           unsigned numRegNeeded,
                           unsigned maxRegs,
                           unsigned& startReg, // inout
                           unsigned& idx,      // output
                           bool isCalleeSaveBias = false,
                           bool isEOTSrc = false);

    bool findContiguousGRFFromBanks(G4_Declare *dcl, bool availRegs[],
                                 const bool forbidden[],
                                 BankAlign align,
                                 unsigned& idx,
                                 bool oneGRFBankDivision);

    unsigned short getOccupiedBundle(const G4_Declare* dcl) const;

    // find contiguous free words in a registers
    int findContiguousWords(uint32_t words, G4_SubReg_Align alignment, int numWord) const;
    bool findContiguousGRF(bool availRegs[], const bool forbidden[], unsigned occupiedBundles, BankAlign align,
        unsigned numRegNeeded, unsigned maxRegs, unsigned & startPos, unsigned & idx, bool isCalleeSaveBias, bool isEOTSrc);
};
}
#endif // __PHYREGUSAGE_H__
