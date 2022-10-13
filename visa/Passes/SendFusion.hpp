/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VISA_PASSES_SENDFUSION_HPP
#define VISA_PASSES_SENDFUSION_HPP

namespace vISA {
class FlowGraph;
class G4_BB;
class Mem_Manager;

bool doSendFusion(FlowGraph *CFG, vISA::Mem_Manager *MMgr);
} // namespace vISA

#endif
