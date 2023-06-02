/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//        ENUM                                               DESCRIPTION
DEF_TIMER(TOTAL, "Total")
DEF_TIMER(BUILDER, "IR_Build")
DEF_TIMER(CFG, "CFG")
DEF_TIMER(OPTIMIZER, "Optimizer")
DEF_TIMER(HW_CONFORMITY, "HW_Conformity")
DEF_TIMER(MISC_OPTS, "Misc_opts")
DEF_TIMER(SWSB, "\tSWSB")
DEF_TIMER(TOTAL_RA, "Total_RA")
DEF_TIMER(ADDR_FLAG_RA, "\tAddr_Flag_RA")
DEF_TIMER(LOCAL_RA, "\tGRF_Local_RA")
DEF_TIMER(HYBRID_RA, "\tGRF_Hybrid_RA")
DEF_TIMER(LINEARSCAN_RA, "\tGRF_LinearScan_RA")
DEF_TIMER(GRF_GLOBAL_RA, "\tGRF_Global_RA")
DEF_TIMER(INTERFERENCE, "\t  Interference")
DEF_TIMER(COLORING, "\t  Graph Coloring")
DEF_TIMER(SPILL, "\t  spill")
DEF_TIMER(PRERA_SCHEDULING, "preRA_Scheduling")
DEF_TIMER(SCHEDULING, "Scheduling")
DEF_TIMER(ENCODE_AND_EMIT, "Encode+Emit")
DEF_TIMER(ENCODE_COMPACTION, "\tCompaction")
DEF_TIMER(IGA_ENCODER, "\tIGA_Encoding")
DEF_TIMER(VISA_BUILDER_APPEND_INST, "VB_Append_Instruction")
DEF_TIMER(VISA_BUILDER_CREATE_VAR, "VB_Create_Var")
DEF_TIMER(VISA_BUILDER_CREATE_OPND, "VB_Create_Operand")
DEF_TIMER(VISA_BUILDER_IR_CONSTRUCTION, "VB_IR_Construction")
DEF_TIMER(LIVENESS, "liveness")
DEF_TIMER(RPE, "Reg Pressure Estimate")
