/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _SENDFUSION_H_
#define _SENDFUSION_H_

namespace vISA
{
    class FlowGraph;
    class G4_BB;
    class Mem_Manager;

    bool doSendFusion(FlowGraph* CFG, vISA::Mem_Manager* MMgr);
}

#endif
