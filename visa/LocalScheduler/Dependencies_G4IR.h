/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _DEPENDENCIES_H_
#define _DEPENDENCIES_H_

#include "../G4_Opcode.h"

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

DepType getDepForOpnd(Gen4_Operand_Number cur, Gen4_Operand_Number liv);
} // namespace vISA

#endif
