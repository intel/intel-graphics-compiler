/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "visa_igc_common_header.h"

#include "Common_ISA.h"
#include "Mem_Manager.h"

#include "IGC/common/StringMacros.hpp"

const char *implictKindStrings[IMPLICIT_INPUT_COUNT] = {
    "EXPLICIT", "LOCAL_SIZE", "GROUP_COUNT", "LOCAL_ID", "PSEUDO_INPUT"};

const int implictKindValues[IMPLICIT_INPUT_COUNT] = {
    INPUT_EXPLICIT, LOCAL_SIZE, GROUP_COUNT, LOCAL_ID, PSEUDO_INPUT};

const char *Rel_op_str[ISA_CMP_UNDEF + 1] = {"eq", // equal
                                             "ne", // not equal
                                             "gt", // greater
                                             "ge", // greater or equal
                                             "lt", // less
                                             "le", // less or equal
                                             " "};

const char *media_ld_mod_str[MEDIA_LD_Mod_NUM] = {
    "nomod", "modified", "top", "bottom", "top_mod", "bottom_mod"};

const char *sampler_channel_output_str[4] = {"16-full", "16-downsampled",
                                             "8-full", "8-downsampled"};

const char *vme_op_mode_str[VME_OP_MODE_NUM] = {"inter", "intra", "both"};

const char *emask_str[vISA_NUM_EMASK + 1] = {
    "M1",    "M2",    "M3",    "M4",    "M5",    "M6",
    "M7",    "M8",    "M1_NM", "M2_NM", "M3_NM", "M4_NM",
    "M5_NM", "M6_NM", "M7_NM", "M8_NM", "NoMask"};

static const char *getSampleOp3DNameOrNull(VISASampler3DSubOpCode opcode,
                                           TARGET_PLATFORM platform) {
  switch (opcode) {
  case VISA_3D_SAMPLE: // 0x0
    return "sample_3d";
  case VISA_3D_SAMPLE_B: // 0x1
    return "sample_b";
  case VISA_3D_SAMPLE_L: // 0x2
    return "sample_l";
  case VISA_3D_SAMPLE_C: // 0x3
    return "sample_c";
  case VISA_3D_SAMPLE_D: // 0x4
    return "sample_d";
  case VISA_3D_SAMPLE_B_C: // 0x5
    return "sample_b_c";
  case VISA_3D_SAMPLE_L_C: // 0x6
    return "sample_l_c";
  case VISA_3D_LD: // 0x7
    return "load_3d";
  case VISA_3D_GATHER4: // 0x8
    return "sample4";
  case VISA_3D_LOD: // 0x9
    return "lod";
  case VISA_3D_RESINFO: // 0xA
    return "resinfo";
  case VISA_3D_SAMPLEINFO: // 0xB
    return "sampleinfo";
  case VISA_3D_SAMPLE_KILLPIX: // 0xC
    return "sample+killpix";
  case VISA_3D_GATHER4_L: // 0xD
    return "sample4_l";
  case VISA_3D_GATHER4_B: // 0xE
    return "sample4_b";
  case VISA_3D_GATHER4_I: // 0xF
    return "sample4_i";
  case VISA_3D_GATHER4_C: // 0x10
    return "sample4_c";
  case VISA_3D_GATHER4_PO: // 0x11
                           // Xe2 reuses GATHER4_PO* opcode values, Xe2 doesn't
                           // have GATHER4_PO*.
    vISA_ASSERT(VISA_3D_SAMPLE_D_C_MLOD == VISA_3D_GATHER4_PO,
                "Code below needs update");
    if (platform >= Xe2) {
      return "sample_d_c_mlod";
    }
    return "sample4_po";
  case VISA_3D_GATHER4_PO_C: // 0x12
    vISA_ASSERT(VISA_3D_SAMPLE_MLOD == VISA_3D_GATHER4_PO_C,
                "Code below needs update");
    if (platform >= Xe2) {
      return "sample_mlod";
    }
    return "sample4_po_c";
  case VISA_3D_SAMPLE_C_MLOD: // 0x13
    return "sample_c_mlod";
  case VISA_3D_SAMPLE_D_C: // 0x14
    return "sample_d_c";
  case VISA_3D_GATHER4_I_C: // 0x15
    return "sample4_i_c";
  case VISA_3D_GATHER4_L_C: // 0x17
    return "sample4_l_c";
  case VISA_3D_SAMPLE_LZ: // 0x18
    return "sample_lz";
  case VISA_3D_SAMPLE_C_LZ: // 0x19
    return "sample_c_lz";
  case VISA_3D_LD_LZ: // 0x1A
    return "load_lz";
  case VISA_3D_LD_L: // 0x1B
    return "load_l";
  case VISA_3D_LD2DMS_W: // 0x1C
    return "load_2dms_w";
  case VISA_3D_LD_MCS: // 0x1D
    return "load_mcs";
  case VISA_3D_SAMPLE_PO:
    return "sample_po";
  case VISA_3D_SAMPLE_PO_B:
    return "sample_po_b";
  case VISA_3D_SAMPLE_PO_L:
    return "sample_po_l";
  case VISA_3D_SAMPLE_PO_C:
    return "sample_po_c";
  case VISA_3D_SAMPLE_PO_D:
    return "sample_po_d";
  case VISA_3D_SAMPLE_PO_L_C:
    return "sample_po_l_c";
  case VISA_3D_GATHER4_PO_PACKED:
    return "sample4_po";
  case VISA_3D_GATHER4_PO_PACKED_L:
    return "sample4_po_l";
  case VISA_3D_GATHER4_PO_PACKED_B:
    return "sample4_po_b";
  case VISA_3D_GATHER4_PO_PACKED_I:
    return "sample4_po_i";
  case VISA_3D_GATHER4_PO_PACKED_C:
    return "sample4_po_c";
  case VISA_3D_GATHER4_PO_PACKED_I_C:
    return "sample4_po_i_c";
  case VISA_3D_GATHER4_PO_PACKED_L_C:
    return "sample4_po_l_c";
  case VISA_3D_SAMPLE_PO_LZ:
    return "sample_po_lz";
  case VISA_3D_SAMPLE_PO_C_LZ:
    return "sample_po_c_lz";
  default:
    return nullptr;
  }
}
const char *getSampleOp3DName(VISASampler3DSubOpCode opcode,
                              TARGET_PLATFORM platform) {
  const char *name = getSampleOp3DNameOrNull(opcode, platform);
  vISA_ASSERT(name, "invalid sampler opcode");
  if (!name)
    return "sampler_unknown";
  return name;
}
VISASampler3DSubOpCode getSampleOpFromName(const char *str,
                                           TARGET_PLATFORM platform) {
  for (int i = 0; i < ISA_NUM_OPCODE; i++) {
    const char *symI =
        getSampleOp3DNameOrNull((VISASampler3DSubOpCode)i, platform);
    if (symI && strcmp(symI, str) == 0)
      return (VISASampler3DSubOpCode)i;
  }
  return (VISASampler3DSubOpCode)-1;
}

const char *avs_control_str[4] = {"AVS_16_FULL", "AVS_16_DOWN_SAMPLE",
                                  "AVS_8_FULL", "AVS_8_DOWN_SAMPLE"};

const char *avs_exec_mode[3] = {"AVS_16x4", "AVS_INVALID", "AVS_16x8"};

// NOTE: keep the order consistent with CMAtomicOperations
const char *CISAAtomicOpNames[] = {
    "add",     // ATOMIC_ADD       = 0x0,
    "sub",     // ATOMIC_SUB       = 0x1,
    "inc",     // ATOMIC_INC       = 0x2,
    "dec",     // ATOMIC_DEC       = 0x3,
    "min",     // ATOMIC_MIN       = 0x4,
    "max",     // ATOMIC_MAX       = 0x5,
    "xchg",    // ATOMIC_XCHG      = 0x6,
    "cmpxchg", // ATOMIC_CMPXCHG   = 0x7,
    "and",     // ATOMIC_AND       = 0x8,
    "or",      // ATOMIC_OR        = 0x9,
    "xor",     // ATOMIC_XOR       = 0xa,
    "minsint", // ATOMIC_IMIN      = 0xb,
    "maxsint", // ATOMIC_IMAX      = 0xc,
    "",        //              [SKIP 0xd]
    "",        //              [SKIP 0xe]
    "",        //              [SKIP 0xf]
    "fmax",    // ATOMIC_FMAX      = 0x10,
    "fmin",    // ATOMIC_FMIN      = 0x11,
    "fcmpwr",  // ATOMIC_FCMPWR    = 0x12,
    "fadd",    // ATOMIC_FADD      = 0x13,
    "fsub",    // ATOMIC_FSUB      = 0x14
};

CISATypeInfo CISATypeTable[ISA_TYPE_NUM] = {
    {ISA_TYPE_UD, "ud", 4}, {ISA_TYPE_D, "d", 4},       {ISA_TYPE_UW, "uw", 2},
    {ISA_TYPE_W, "w", 2},   {ISA_TYPE_UB, "ub", 1},     {ISA_TYPE_B, "b", 1},
    {ISA_TYPE_DF, "df", 8}, {ISA_TYPE_F, "f", 4},       {ISA_TYPE_V, "v", 4},
    {ISA_TYPE_VF, "vf", 4}, {ISA_TYPE_BOOL, "bool", 1}, {ISA_TYPE_UQ, "uq", 8},
    {ISA_TYPE_UV, "uv", 4}, {ISA_TYPE_Q, "q", 8},       {ISA_TYPE_HF, "hf", 2},
    {ISA_TYPE_BF, "bf", 2}};

const char *ChannelMask::Names[] = {
    "0000", // ABGR
    "R",    // 0001
    "G",    // 0010
    "RG",   // 0011
    "B",    // 0100
    "RB",   // 0101
    "GB",   // 0110
    "RGB",  // 0111
    "A",    // 1000
    "RA",   // 1001
    "GA",   // 1010
    "RGA",  // 1011
    "BA",   // 1100
    "RBA",  // 1101
    "GBA",  // 1110
    "RGBA"  // 1111
};
