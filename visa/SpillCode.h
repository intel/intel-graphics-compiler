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

#ifndef __SPILLCODE_H__
#define __SPILLCODE_H__
#include "visa_igc_common_header.h"
#include "common.h"
#include "GraphColor.h"
#include <vector>

namespace vISA
{
class SpillManager
{
    GlobalRA& gra;
    G4_Kernel& kernel;
    PointsToAnalysis& pointsToAnalysis;

    //
    // for creating insts
    //
    IR_Builder& builder;
    //
    // the current block
    //
    unsigned bbId;
    //
    // spilled live ranges
    //
    const LIVERANGE_LIST & spilledLRs;

    // id for newly created address or flag variables
    uint32_t origTempDclId;
    uint32_t tempDclId;

    // The number of flag spill store inserted.
    unsigned numFlagSpillStore;

    // The number of flag spill load inserted.
    unsigned numFlagSpillLoad;

    unsigned int currCISAOffset;

    void genRegMov(G4_BB* bb,
                   INST_LIST_ITER it,
                   G4_VarBase*    src,
                   unsigned short sSubRegOff,
                   G4_VarBase*    dst,
                   unsigned       nRegs,
                   bool           useNoMask);
    G4_Declare* createNewSpillLocDeclare(G4_Declare* dcl);
    G4_Declare* createNewTempAddrDeclare(G4_Declare* dcl);
    G4_Declare* createNewTempFlagDeclare(G4_Declare* dcl);
    G4_Declare* createNewTempAddrDeclare(G4_Declare* dcl, uint16_t num_reg);
    void replaceSpilledDst(G4_BB* bb,
                           INST_LIST_ITER it, // where new insts will be inserted
                           G4_INST*       inst,
                           PointsToAnalysis& pointsToAnalysis,
                           G4_Operand ** operands_analyzed,
                           G4_Declare ** declares_created);
    void replaceSpilledSrc(G4_BB* bb,
                           INST_LIST_ITER it, // where new insts will be inserted
                           G4_INST*       inst,
                           unsigned       i,
                           PointsToAnalysis& pointsToAnalysis,
                           G4_Operand ** operands_analyzed,
                           G4_Declare ** declares_created);
    void replaceSpilledPredicate(G4_BB* bb,
                           INST_LIST_ITER it,
                           G4_INST*       inst);
    void replaceSpilledFlagDst(G4_BB*     bb,
                           INST_LIST_ITER it,
                           G4_INST*       inst);

    void createSpillLocations(G4_Kernel& kernel);

public:


    SpillManager(GlobalRA& g, const LIVERANGE_LIST & splrs, uint32_t startTempDclId) :
        gra(g), kernel(g.kernel), pointsToAnalysis(g.pointsToAnalysis), builder(*g.kernel.fg.builder),
        bbId(UINT_MAX), spilledLRs(splrs), origTempDclId(startTempDclId)
    {
        tempDclId = startTempDclId;
        numFlagSpillStore = numFlagSpillLoad = 0;
    }
    void insertSpillCode();
    bool isAnyNewTempCreated() const {return getNumTempCreated() != 0;}
    uint32_t getNumTempCreated() const { return tempDclId - origTempDclId; }
    uint32_t getNextTempDclId() const { return tempDclId; }

    unsigned getNumFlagSpillStore() const { return numFlagSpillStore; }
    unsigned getNumFlagSpillLoad() const { return numFlagSpillLoad; }
};
}
#endif // __SPILLCODE_H__
