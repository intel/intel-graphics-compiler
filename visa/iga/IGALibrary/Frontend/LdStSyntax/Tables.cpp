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
/*
 * NOTA BENE: THIS FILE WAS AUTOMATICALLY GENERATED FROM BXML VIA SCRIPTING
 * TOOLS IN THE DIRECTORY: $IGA_ROOT/bxml/send

 * If there's a bug in BXML, fix it there and then regenerate these files.
 */
#include "Tables.hpp"
#include "../../bits.hpp"

using namespace iga;

//////////////////////////////////////////////////
// MFieldType Structure Definitions

///////////////////////////////////////////////////////////////////////////////
// A64  (A64 Stateless Binding Table Entry)
static const MFieldType MFT_A64 {
    8,                       // length (in bits)
    "A64",                   // name
    MFieldMappingAddrModel,  // mapping
    nullptr,                 // instOptSymbol
    nullptr,                 // subopParse
    nullptr,                 // subopFormat
    nullptr,                 // execSizeDecode
    nullptr,                 // execSizeEncode
};

///////////////////////////////////////////////////////////////////////////////
// BTS  (Binding Table Entry)
static const MFieldType MFT_BTS {
    8,                       // length (in bits)
    "BTS",                   // name
    MFieldMappingAddrModel,  // mapping
    nullptr,                 // instOptSymbol
    nullptr,                 // subopParse
    nullptr,                 // subopFormat
    nullptr,                 // execSizeDecode
    nullptr,                 // execSizeEncode
};

///////////////////////////////////////////////////////////////////////////////
// BTS_SLM_A32  ((BTI|SLM|A32))
static const MFieldType MFT_BTS_SLM_A32 {
    8,                       // length (in bits)
    "BTS_SLM_A32",           // name
    MFieldMappingAddrModel,  // mapping
    nullptr,                 // instOptSymbol
    nullptr,                 // subopParse
    nullptr,                 // subopFormat
    nullptr,                 // execSizeDecode
    nullptr,                 // execSizeEncode
};

///////////////////////////////////////////////////////////////////////////////
// CMask  (Channel Mask Message Descriptor Control Field)
static bool SubopParse_MFT_CMask(const std::string &token, uint32_t &val) {
    if (token == "rgba") {
        val = 0x0;
    } else if (token == "gba") {
        val = 0x1;
    } else if (token == "rba") {
        val = 0x2;
    } else if (token == "ba") {
        val = 0x3;
    } else if (token == "rga") {
        val = 0x4;
    } else if (token == "ga") {
        val = 0x5;
    } else if (token == "ra") {
        val = 0x6;
    } else if (token == "a") {
        val = 0x7;
    } else if (token == "rgb") {
        val = 0x8;
    } else if (token == "gb") {
        val = 0x9;
    } else if (token == "rb") {
        val = 0xA;
    } else if (token == "b") {
        val = 0xB;
    } else if (token == "rg") {
        val = 0xC;
    } else if (token == "g") {
        val = 0xD;
    } else if (token == "r") {
        val = 0xE;
    } else {
      return false;
    }
    return true;
}
static bool SubopFormat_MFT_CMask(std::ostream &os, uint32_t val) {
    if (val == 0x0) {
        os << "rgba";
    } else if (val == 0x1) {
        os << "gba";
    } else if (val == 0x2) {
        os << "rba";
    } else if (val == 0x3) {
        os << "ba";
    } else if (val == 0x4) {
        os << "rga";
    } else if (val == 0x5) {
        os << "ga";
    } else if (val == 0x6) {
        os << "ra";
    } else if (val == 0x7) {
        os << "a";
    } else if (val == 0x8) {
        os << "rgb";
    } else if (val == 0x9) {
        os << "gb";
    } else if (val == 0xA) {
        os << "rb";
    } else if (val == 0xB) {
        os << "b";
    } else if (val == 0xC) {
        os << "rg";
    } else if (val == 0xD) {
        os << "g";
    } else if (val == 0xE) {
        os << "r";
    } else {
      os << "CMask(0x" << std::hex << val << ")";
        return false;
    }
    return true;
}
static const MFieldType MFT_CMask {
    4,                       // length (in bits)
    "CMask",                 // name
    MFieldMappingSubOp,      // mapping
    nullptr,                 // instOptSymbol
    &SubopParse_MFT_CMask,   // subopParse
    &SubopFormat_MFT_CMask,  // subopFormat
    nullptr,                 // execSizeDecode
    nullptr,                 // execSizeEncode
};

///////////////////////////////////////////////////////////////////////////////
// CMaskUW  (Untyped Write Channel Mask Message Descriptor Control Field)
static bool SubopParse_MFT_CMaskUW(const std::string &token, uint32_t &val) {
    if (token == "rgba") {
        val = 0x0;
    } else if (token == "rgb") {
        val = 0x8;
    } else if (token == "rg") {
        val = 0xC;
    } else if (token == "r") {
        val = 0xE;
    } else {
      return false;
    }
    return true;
}
static bool SubopFormat_MFT_CMaskUW(std::ostream &os, uint32_t val) {
    if (val == 0x0) {
        os << "rgba";
    } else if (val == 0x8) {
        os << "rgb";
    } else if (val == 0xC) {
        os << "rg";
    } else if (val == 0xE) {
        os << "r";
    } else {
      os << "CMaskUW(0x" << std::hex << val << ")";
        return false;
    }
    return true;
}
static const MFieldType MFT_CMaskUW {
    4,                       // length (in bits)
    "CMaskUW",               // name
    MFieldMappingSubOp,      // mapping
    nullptr,                 // instOptSymbol
    &SubopParse_MFT_CMaskUW, // subopParse
    &SubopFormat_MFT_CMaskUW, // subopFormat
    nullptr,                 // execSizeDecode
    nullptr,                 // execSizeEncode
};

///////////////////////////////////////////////////////////////////////////////
// DE  (Data Elements)
static bool SubopParse_MFT_DE(const std::string &token, uint32_t &val) {
    if (token == "x1") {
        val = 0x0;
    } else if (token == "x2") {
        val = 0x1;
    } else if (token == "x4") {
        val = 0x2;
    } else if (token == "x8") {
        val = 0x3;
    } else {
      return false;
    }
    return true;
}
static bool SubopFormat_MFT_DE(std::ostream &os, uint32_t val) {
    if (val == 0x0) {
        os << "x1";
    } else if (val == 0x1) {
        os << "x2";
    } else if (val == 0x2) {
        os << "x4";
    } else if (val == 0x3) {
        os << "x8";
    } else {
      os << "DE(0x" << std::hex << val << ")";
        return false;
    }
    return true;
}
static const MFieldType MFT_DE {
    2,                       // length (in bits)
    "DE",                    // name
    MFieldMappingSubOp,      // mapping
    nullptr,                 // instOptSymbol
    &SubopParse_MFT_DE,      // subopParse
    &SubopFormat_MFT_DE,     // subopFormat
    nullptr,                 // execSizeDecode
    nullptr,                 // execSizeEncode
};

///////////////////////////////////////////////////////////////////////////////
// H  (Header Optional)
static const MFieldType MFT_H {
    1,                       // length (in bits)
    "H",                     // name
    MFieldMappingHeader,     // mapping
    nullptr,                 // instOptSymbol
    nullptr,                 // subopParse
    nullptr,                 // subopFormat
    nullptr,                 // execSizeDecode
    nullptr,                 // execSizeEncode
};

///////////////////////////////////////////////////////////////////////////////
// HW64  (Data Blocks Hex Words)
static bool SubopParse_MFT_HW64(const std::string &token, uint32_t &val) {
    if (token == "x1") {
        val = 0x1;
    } else if (token == "x2") {
        val = 0x2;
    } else if (token == "x4") {
        val = 0x3;
    } else if (token == "x8") {
        val = 0x4;
    } else {
      return false;
    }
    return true;
}
static bool SubopFormat_MFT_HW64(std::ostream &os, uint32_t val) {
    if (val == 0x1) {
        os << "x1";
    } else if (val == 0x2) {
        os << "x2";
    } else if (val == 0x3) {
        os << "x4";
    } else if (val == 0x4) {
        os << "x8";
    } else {
      os << "HW64(0x" << std::hex << val << ")";
        return false;
    }
    return true;
}
static const MFieldType MFT_HW64 {
    3,                       // length (in bits)
    "HW64",                  // name
    MFieldMappingSubOp,      // mapping
    nullptr,                 // instOptSymbol
    &SubopParse_MFT_HW64,    // subopParse
    &SubopFormat_MFT_HW64,   // subopFormat
    nullptr,                 // execSizeDecode
    nullptr,                 // execSizeEncode
};

///////////////////////////////////////////////////////////////////////////////
// HWSB  (Data Blocks Hex Words)
static bool SubopParse_MFT_HWSB(const std::string &token, uint32_t &val) {
    if (token == "x1") {
        val = 0x0;
    } else if (token == "x2") {
        val = 0x1;
    } else if (token == "x4") {
        val = 0x2;
    } else if (token == "x8") {
        val = 0x3;
    } else {
      return false;
    }
    return true;
}
static bool SubopFormat_MFT_HWSB(std::ostream &os, uint32_t val) {
    if (val == 0x0) {
        os << "x1";
    } else if (val == 0x1) {
        os << "x2";
    } else if (val == 0x2) {
        os << "x4";
    } else if (val == 0x3) {
        os << "x8";
    } else {
      os << "HWSB(0x" << std::hex << val << ")";
        return false;
    }
    return true;
}
static const MFieldType MFT_HWSB {
    3,                       // length (in bits)
    "HWSB",                  // name
    MFieldMappingSubOp,      // mapping
    nullptr,                 // instOptSymbol
    &SubopParse_MFT_HWSB,    // subopParse
    &SubopFormat_MFT_HWSB,   // subopFormat
    nullptr,                 // execSizeDecode
    nullptr,                 // execSizeEncode
};

///////////////////////////////////////////////////////////////////////////////
// OWs  (Data Blocks Oct Words)
static bool SubopParse_MFT_OWs(const std::string &token, uint32_t &val) {
    if (token == "x1L") {
        val = 0x0;
    } else if (token == "x1H") {
        val = 0x1;
    } else if (token == "x2") {
        val = 0x2;
    } else if (token == "x4") {
        val = 0x3;
    } else if (token == "x8") {
        val = 0x4;
    } else {
      return false;
    }
    return true;
}
static bool SubopFormat_MFT_OWs(std::ostream &os, uint32_t val) {
    if (val == 0x0) {
        os << "x1L";
    } else if (val == 0x1) {
        os << "x1H";
    } else if (val == 0x2) {
        os << "x2";
    } else if (val == 0x3) {
        os << "x4";
    } else if (val == 0x4) {
        os << "x8";
    } else {
      os << "OWs(0x" << std::hex << val << ")";
        return false;
    }
    return true;
}
static const MFieldType MFT_OWs {
    3,                       // length (in bits)
    "OWs",                   // name
    MFieldMappingSubOp,      // mapping
    nullptr,                 // instOptSymbol
    &SubopParse_MFT_OWs,     // subopParse
    &SubopFormat_MFT_OWs,    // subopFormat
    nullptr,                 // execSizeDecode
    nullptr,                 // execSizeEncode
};

///////////////////////////////////////////////////////////////////////////////
// PD  (Packed Data)
static const MFieldType MFT_PD {
    1,                       // length (in bits)
    "PD",                    // name
    MFieldMappingInstOpt,    // mapping
    "PackedData",            // instOptSymbol
    nullptr,                 // subopParse
    nullptr,                 // subopFormat
    nullptr,                 // execSizeDecode
    nullptr,                 // execSizeEncode
};

///////////////////////////////////////////////////////////////////////////////
// SG3  (Slot Group 3)
static bool SubopParse_MFT_SG3(const std::string &token, uint32_t &val) {
    if (token == "sg4x2") {
        val = 0x0;
    } else if (token == "sg8l") {
        val = 0x1;
    } else if (token == "sg8h") {
        val = 0x2;
    } else {
      return false;
    }
    return true;
}
static bool SubopFormat_MFT_SG3(std::ostream &os, uint32_t val) {
    if (val == 0x0) {
        os << "sg4x2";
    } else if (val == 0x1) {
        os << "sg8l";
    } else if (val == 0x2) {
        os << "sg8h";
    } else {
      os << "SG3(0x" << std::hex << val << ")";
        return false;
    }
    return true;
}
static const MFieldType MFT_SG3 {
    2,                       // length (in bits)
    "SG3",                   // name
    MFieldMappingSubOp,      // mapping
    nullptr,                 // instOptSymbol
    &SubopParse_MFT_SG3,     // subopParse
    &SubopFormat_MFT_SG3,    // subopFormat
    nullptr,                 // execSizeDecode
    nullptr,                 // execSizeEncode
};

///////////////////////////////////////////////////////////////////////////////
// SM2  (SIMD Mode 2)
static bool ExecSizeDecode_MFT_SM2(uint32_t val, ExecSize &execSize) {
    if (val == 0x0) {
        execSize = ExecSize::SIMD8;
    } else if (val == 0x1) {
        execSize = ExecSize::SIMD16;
    } else {
        execSize = ExecSize::INVALID;
        return false;
    }
    return true;
}
static bool ExecSizeEncode_MFT_SM2(ExecSize execSize, uint32_t &val) {
    switch (execSize) {
    case ExecSize::SIMD1:
    case ExecSize::SIMD2:
    case ExecSize::SIMD4: // <8 uses SIMD8
    case ExecSize::SIMD8:
        val = 0x0;
        break;
    case ExecSize::SIMD16:
        val = 0x1;
        break;
    default:
        return false;
    }
    return true;
}
static const MFieldType MFT_SM2 {
    1,                       // length (in bits)
    "SM2",                   // name
    MFieldMappingExecSize,   // mapping
    nullptr,                 // instOptSymbol
    nullptr,                 // subopParse
    nullptr,                 // subopFormat
    &ExecSizeDecode_MFT_SM2, // execSizeDecode
    &ExecSizeEncode_MFT_SM2, // execSizeEncode
};

///////////////////////////////////////////////////////////////////////////////
// SM3  (SIMD Mode 3)
static bool ExecSizeDecode_MFT_SM3(uint32_t val, ExecSize &execSize) {
    if (val == 0x1) {
        execSize = ExecSize::SIMD16;
    } else if (val == 0x2) {
        execSize = ExecSize::SIMD8;
    } else {
        execSize = ExecSize::INVALID;
        return false;
    }
    return true;
}
static bool ExecSizeEncode_MFT_SM3(ExecSize execSize, uint32_t &val) {
    switch (execSize) {
    case ExecSize::SIMD1:
    case ExecSize::SIMD2:
    case ExecSize::SIMD4: // <8 uses SIMD8
    case ExecSize::SIMD8:
        val = 0x2;
        break;
    case ExecSize::SIMD16:
        val = 0x1;
        break;
    default:
        return false;
    }
    return true;
}
static const MFieldType MFT_SM3 {
    2,                       // length (in bits)
    "SM3",                   // name
    MFieldMappingExecSize,   // mapping
    nullptr,                 // instOptSymbol
    nullptr,                 // subopParse
    nullptr,                 // subopFormat
    &ExecSizeDecode_MFT_SM3, // execSizeDecode
    &ExecSizeEncode_MFT_SM3, // execSizeEncode
};

///////////////////////////////////////////////////////////////////////////////
// ScratchOffset  (Scratch Offset)
static const MFieldType MFT_ScratchOffset {
    12,                      // length (in bits)
    "ScratchOffset",         // name
    MFieldMappingAddrModel,  // mapping
    nullptr,                 // instOptSymbol
    nullptr,                 // subopParse
    nullptr,                 // subopFormat
    nullptr,                 // execSizeDecode
    nullptr,                 // execSizeEncode
};

#define MFORMAT_TABLE_LENGTH 29
static const MFormat MFORMAT_TABLE[MFORMAT_TABLE_LENGTH] {
//////////////////////////////////////////////////////////////////////////////
// Vector
    {
        // #0:  MSD1R_A64_BS  A64 Byte Gathering Read
        Platform::GEN8, // platform
        MType::MF_GR_VEC_A64_D8, // type
        MKind::LOAD, // kind
        "xga8", // mnemonic
        {{30,&MFT_PD},{12,&MFT_SM2},{10,&MFT_DE},{0,&MFT_A64}}, // fields
        {2, -1}, // argIndices
        MAddrModel::A64_FAMILY, // addrModel
        SFID::DC1, // sfid
        BITFIELD_MASK32(29,1)|BITFIELD_MASK32(19,1)|BITFIELD_MASK32(14,5)|BITFIELD_MASK32(8,2), // formatMask
        (0x0<<29)|(0x0<<19)|(0x10<<14)|(0x0<<8) // formatValue
    },
    {
        // #2:  MSD1W_A64_BS  A64 Byte Scattering Write
        Platform::GEN8, // platform
        MType::MF_SW_VEC_A64_D8, // type
        MKind::STORE, // kind
        "xsc8", // mnemonic
        {{30,&MFT_PD},{12,&MFT_SM2},{10,&MFT_DE},{0,&MFT_A64}}, // fields
        {2, -1}, // argIndices
        MAddrModel::A64_FAMILY, // addrModel
        SFID::DC1, // sfid
        BITFIELD_MASK32(29,1)|BITFIELD_MASK32(19,1)|BITFIELD_MASK32(14,5)|BITFIELD_MASK32(8,2), // formatMask
        (0x0<<29)|(0x0<<19)|(0x1A<<14)|(0x0<<8) // formatValue
    },
    {
        // #3:  MSD1R_A64_DWS  A64 DWord Gathering Read
        Platform::GEN8, // platform
        MType::MF_GR_VEC_A64_D32, // type
        MKind::LOAD, // kind
        "xga32", // mnemonic
        {{30,&MFT_PD},{12,&MFT_SM2},{10,&MFT_DE},{0,&MFT_A64}}, // fields
        {2, -1}, // argIndices
        MAddrModel::A64_FAMILY, // addrModel
        SFID::DC1, // sfid
        BITFIELD_MASK32(29,1)|BITFIELD_MASK32(19,1)|BITFIELD_MASK32(14,5)|BITFIELD_MASK32(8,2), // formatMask
        (0x0<<29)|(0x0<<19)|(0x10<<14)|(0x1<<8) // formatValue
    },
    {
        // #4:  MSD1W_A64_DWS  A64 DWord Scattering Write
        Platform::GEN8, // platform
        MType::MF_SW_VEC_A64_D32, // type
        MKind::STORE, // kind
        "xsc32", // mnemonic
        {{30,&MFT_PD},{12,&MFT_SM2},{10,&MFT_DE},{0,&MFT_A64}}, // fields
        {2, -1}, // argIndices
        MAddrModel::A64_FAMILY, // addrModel
        SFID::DC1, // sfid
        BITFIELD_MASK32(29,1)|BITFIELD_MASK32(19,1)|BITFIELD_MASK32(14,5)|BITFIELD_MASK32(8,2), // formatMask
        (0x0<<29)|(0x0<<19)|(0x1A<<14)|(0x1<<8) // formatValue
    },
    {
        // #5:  MSD1R_A64_QWS  A64 QWord Gathering Read
        Platform::GEN8, // platform
        MType::MF_GR_VEC_A64_D64, // type
        MKind::LOAD, // kind
        "xga64", // mnemonic
        {{30,&MFT_PD},{12,&MFT_SM2},{10,&MFT_DE},{0,&MFT_A64}}, // fields
        {2, -1}, // argIndices
        MAddrModel::A64_FAMILY, // addrModel
        SFID::DC1, // sfid
        BITFIELD_MASK32(29,1)|BITFIELD_MASK32(19,1)|BITFIELD_MASK32(14,5)|BITFIELD_MASK32(8,2), // formatMask
        (0x0<<29)|(0x0<<19)|(0x10<<14)|(0x2<<8) // formatValue
    },
    {
        // #6:  MSD1W_A64_QWS  A64 QWord Scattering Write
        Platform::GEN8, // platform
        MType::MF_SW_VEC_A64_D64, // type
        MKind::STORE, // kind
        "xsc64", // mnemonic
        {{30,&MFT_PD},{12,&MFT_SM2},{10,&MFT_DE},{0,&MFT_A64}}, // fields
        {2, -1}, // argIndices
        MAddrModel::A64_FAMILY, // addrModel
        SFID::DC1, // sfid
        BITFIELD_MASK32(29,1)|BITFIELD_MASK32(19,1)|BITFIELD_MASK32(14,5)|BITFIELD_MASK32(8,2), // formatMask
        (0x0<<29)|(0x0<<19)|(0x1A<<14)|(0x2<<8) // formatValue
    },
    {
        // #7:  MSD0R_BS  Byte Gathering Read
        Platform::GEN8, // platform
        MType::MF_GR_VEC_A32_D8, // type
        MKind::LOAD, // kind
        "ga8", // mnemonic
        {{30,&MFT_PD},{19,&MFT_H},{10,&MFT_DE},{8,&MFT_SM2},{0,&MFT_BTS_SLM_A32}}, // fields
        {2, -1}, // argIndices
        MAddrModel::BTS_SLM_A32, // addrModel
        SFID::DC0, // sfid
        BITFIELD_MASK32(29,1)|BITFIELD_MASK32(14,5), // formatMask
        (0x0<<29)|(0x4<<14) // formatValue
    },
    {
        // #9:  MSD0W_BS  Byte Scattering Write
        Platform::GEN8, // platform
        MType::MF_SW_VEC_A32_D8, // type
        MKind::STORE, // kind
        "sc8", // mnemonic
        {{30,&MFT_PD},{19,&MFT_H},{10,&MFT_DE},{8,&MFT_SM2},{0,&MFT_BTS_SLM_A32}}, // fields
        {2, -1}, // argIndices
        MAddrModel::BTS_SLM_A32, // addrModel
        SFID::DC0, // sfid
        BITFIELD_MASK32(29,1)|BITFIELD_MASK32(14,5), // formatMask
        (0x0<<29)|(0xC<<14) // formatValue
    },
    {
        // #10:  MSD0R_DWS  DWord Gathering Read
        Platform::GEN8, // platform
        MType::MF_GR_VEC_A32_D32, // type
        MKind::LOAD, // kind
        "ga32", // mnemonic
        {{30,&MFT_PD},{19,&MFT_H},{10,&MFT_DE},{8,&MFT_SM2},{0,&MFT_BTS_SLM_A32}}, // fields
        {2, -1}, // argIndices
        MAddrModel::BTS_SLM_A32, // addrModel
        SFID::DC0, // sfid
        BITFIELD_MASK32(29,1)|BITFIELD_MASK32(14,5)|BITFIELD_MASK32(9,1), // formatMask
        (0x0<<29)|(0x3<<14)|(0x1<<9) // formatValue
    },
    {
        // #11:  MSD0W_DWS  DWord Scattering Write
        Platform::GEN8, // platform
        MType::MF_SW_VEC_A32_D32, // type
        MKind::STORE, // kind
        "sc32", // mnemonic
        {{30,&MFT_PD},{19,&MFT_H},{10,&MFT_DE},{8,&MFT_SM2},{0,&MFT_BTS_SLM_A32}}, // fields
        {2, -1}, // argIndices
        MAddrModel::BTS_SLM_A32, // addrModel
        SFID::DC0, // sfid
        BITFIELD_MASK32(29,1)|BITFIELD_MASK32(14,5)|BITFIELD_MASK32(9,1), // formatMask
        (0x0<<29)|(0xB<<14)|(0x1<<9) // formatValue
    },
    {
        // #12:  MSDCR_DWS  Constant DWord Gathering Read
        Platform::GEN8, // platform
        MType::MF_CGR_VEC_A32_D32, // type
        MKind::LOAD, // kind
        "ga32c", // mnemonic
        {{30,&MFT_PD},{19,&MFT_H},{10,&MFT_DE},{8,&MFT_SM2},{0,&MFT_BTS}}, // fields
        {2, -1}, // argIndices
        MAddrModel::BTS_FAMILY, // addrModel
        SFID::DCRO, // sfid
        BITFIELD_MASK32(29,1)|BITFIELD_MASK32(14,5)|BITFIELD_MASK32(9,1), // formatMask
        (0x0<<29)|(0x3<<14)|(0x1<<9) // formatValue
    },

//////////////////////////////////////////////////////////////////////////////
// Block
    {
        // #19:  MSD1R_A64_HWB  A64 HWord Block Read
        Platform::GEN8, // platform
        MType::MF_BR_A64_D256, // type
        MKind::LOAD, // kind
        "xbl256", // mnemonic
        {{19,&MFT_H},{8,&MFT_HW64},{0,&MFT_A64}}, // fields
        {1, -1}, // argIndices
        MAddrModel::A64_FAMILY, // addrModel
        SFID::DC1, // sfid
        BITFIELD_MASK32(14,5)|BITFIELD_MASK32(11,2), // formatMask
        (0x14<<14)|(0x3<<11) // formatValue
    },
    {
        // #20:  MSD1W_A64_HWB  A64 HWord Block Write
        Platform::GEN8, // platform
        MType::MF_BW_A64_D256, // type
        MKind::STORE, // kind
        "xbl256", // mnemonic
        {{19,&MFT_H},{8,&MFT_HW64},{0,&MFT_A64}}, // fields
        {1, -1}, // argIndices
        MAddrModel::A64_FAMILY, // addrModel
        SFID::DC1, // sfid
        BITFIELD_MASK32(14,5)|BITFIELD_MASK32(11,2), // formatMask
        (0x15<<14)|(0x3<<11) // formatValue
    },
    {
        // #21:  MSD1R_A64_OWB  A64 OWord Block Read
        Platform::GEN8, // platform
        MType::MF_BR_A64_D128, // type
        MKind::LOAD, // kind
        "xbl128", // mnemonic
        {{19,&MFT_H},{8,&MFT_OWs},{0,&MFT_A64}}, // fields
        {1, -1}, // argIndices
        MAddrModel::A64_FAMILY, // addrModel
        SFID::DC1, // sfid
        BITFIELD_MASK32(14,5)|BITFIELD_MASK32(11,2), // formatMask
        (0x14<<14)|(0x0<<11) // formatValue
    },
    {
        // #22:  MSD1W_A64_OWB  A64 OWord Block Write
        Platform::GEN8, // platform
        MType::MF_BW_A64_D128, // type
        MKind::STORE, // kind
        "xbl128", // mnemonic
        {{19,&MFT_H},{8,&MFT_OWs},{0,&MFT_A64}}, // fields
        {1, -1}, // argIndices
        MAddrModel::A64_FAMILY, // addrModel
        SFID::DC1, // sfid
        BITFIELD_MASK32(14,5)|BITFIELD_MASK32(11,2), // formatMask
        (0x15<<14)|(0x0<<11) // formatValue
    },
    {
        // #23:  MSD1R_A64_OWUB  Unaligned A64 OWord Block Read
        Platform::GEN8, // platform
        MType::MF_UBR_A64_D128, // type
        MKind::LOAD, // kind
        "xubl128", // mnemonic
        {{19,&MFT_H},{8,&MFT_OWs},{0,&MFT_A64}}, // fields
        {1, -1}, // argIndices
        MAddrModel::A64_FAMILY, // addrModel
        SFID::DC1, // sfid
        BITFIELD_MASK32(14,5)|BITFIELD_MASK32(11,2), // formatMask
        (0x15<<14)|(0x1<<11) // formatValue
    },
    {
        // #24:  MSD0R_OWB  OWord Block Read
        Platform::GEN8, // platform
        MType::MF_BR_A32_D128, // type
        MKind::LOAD, // kind
        "bl128", // mnemonic
        {{19,&MFT_H},{8,&MFT_OWs},{0,&MFT_BTS_SLM_A32}}, // fields
        {1, -1}, // argIndices
        MAddrModel::BTS_SLM_A32, // addrModel
        SFID::DC0, // sfid
        BITFIELD_MASK32(14,5), // formatMask
        (0x0<<14) // formatValue
    },
    {
        // #25:  MSD0W_OWB  OWord Block Write
        Platform::GEN8, // platform
        MType::MF_BW_A32_D128, // type
        MKind::STORE, // kind
        "bl128", // mnemonic
        {{19,&MFT_H},{8,&MFT_OWs},{0,&MFT_BTS_SLM_A32}}, // fields
        {1, -1}, // argIndices
        MAddrModel::BTS_SLM_A32, // addrModel
        SFID::DC0, // sfid
        BITFIELD_MASK32(14,5), // formatMask
        (0x8<<14) // formatValue
    },
    {
        // #26:  MSD0R_OWAB  Aligned OWord Block Read
        Platform::GEN8, // platform
        MType::MF_ABR_A32_D128, // type
        MKind::LOAD, // kind
        "abl128", // mnemonic
        {{19,&MFT_H},{8,&MFT_OWs},{0,&MFT_BTS_SLM_A32}}, // fields
        {1, -1}, // argIndices
        MAddrModel::BTS_SLM_A32, // addrModel
        SFID::DC0, // sfid
        BITFIELD_MASK32(14,5), // formatMask
        (0x1<<14) // formatValue
    },
    {
        // #27:  MSDCR_OWB  Constant OWord Block Read
        Platform::GEN8, // platform
        MType::MF_CBR_A32_D128, // type
        MKind::LOAD, // kind
        "cbl128", // mnemonic
        {{19,&MFT_H},{8,&MFT_OWs},{0,&MFT_BTS}}, // fields
        {1, -1}, // argIndices
        MAddrModel::BTS_FAMILY, // addrModel
        SFID::DCRO, // sfid
        BITFIELD_MASK32(14,5), // formatMask
        (0x0<<14) // formatValue
    },
    {
        // #28:  MSDCR_OWUB  Constant Unaligned OWord Block Read
        Platform::GEN8, // platform
        MType::MF_CUBR_A32_D128, // type
        MKind::LOAD, // kind
        "cubl128", // mnemonic
        {{19,&MFT_H},{8,&MFT_OWs},{0,&MFT_BTS}}, // fields
        {1, -1}, // argIndices
        MAddrModel::BTS_FAMILY, // addrModel
        SFID::DCRO, // sfid
        BITFIELD_MASK32(14,5), // formatMask
        (0x1<<14) // formatValue
    },
    {
        // #29:  MSD0R_SB  Scratch HWord Block Read
        Platform::GEN8, // platform
        MType::MF_SBR_A32_D256, // type
        MKind::LOAD, // kind
        "sbl256", // mnemonic
        {{19,&MFT_H},{12,&MFT_HWSB},{0,&MFT_ScratchOffset}}, // fields
        {1, -1}, // argIndices
        MAddrModel::SCRATCH_FAMILY, // addrModel
        SFID::DC0, // sfid
        BITFIELD_MASK32(14,5), // formatMask
        (0x10<<14) // formatValue
    },
    {
        // #30:  MSD0W_SB  Scratch HWord Block Write
        Platform::GEN8, // platform
        MType::MF_SBW_A32_D256, // type
        MKind::STORE, // kind
        "sbl256", // mnemonic
        {{19,&MFT_H},{12,&MFT_HWSB},{0,&MFT_ScratchOffset}}, // fields
        {1, -1}, // argIndices
        MAddrModel::SCRATCH_FAMILY, // addrModel
        SFID::DC0, // sfid
        BITFIELD_MASK32(14,5), // formatMask
        (0x18<<14) // formatValue
    },

//////////////////////////////////////////////////////////////////////////////
// Surface
    {
        // #31:  MSD1R_A64_US  A64 Untyped Surface Read
        Platform::GEN8, // platform
        MType::MF_USR_A64_D32, // type
        MKind::LOAD, // kind
        "xus", // mnemonic
        {{12,&MFT_SM3},{8,&MFT_CMask},{0,&MFT_A64}}, // fields
        {1, -1}, // argIndices
        MAddrModel::A64_FAMILY, // addrModel
        SFID::DC1, // sfid
        BITFIELD_MASK32(29,1)|BITFIELD_MASK32(19,1)|BITFIELD_MASK32(14,5), // formatMask
        (0x0<<29)|(0x0<<19)|(0x11<<14) // formatValue
    },
    {
        // #33:  MSD1W_A64_US  A64 Untyped Surface Write
        Platform::GEN8, // platform
        MType::MF_USW_A64_D32, // type
        MKind::STORE, // kind
        "xus", // mnemonic
        {{12,&MFT_SM3},{8,&MFT_CMaskUW},{0,&MFT_A64}}, // fields
        {1, -1}, // argIndices
        MAddrModel::A64_FAMILY, // addrModel
        SFID::DC1, // sfid
        BITFIELD_MASK32(29,1)|BITFIELD_MASK32(19,1)|BITFIELD_MASK32(14,5), // formatMask
        (0x0<<29)|(0x0<<19)|(0x19<<14) // formatValue
    },
    {
        // #34:  MSD1R_US  A32 Untyped Surface Read
        Platform::GEN8, // platform
        MType::MF_USR_A32_D32, // type
        MKind::LOAD, // kind
        "us", // mnemonic
        {{19,&MFT_H},{12,&MFT_SM3},{8,&MFT_CMask},{0,&MFT_BTS_SLM_A32}}, // fields
        {2, -1}, // argIndices
        MAddrModel::BTS_SLM_A32, // addrModel
        SFID::DC1, // sfid
        BITFIELD_MASK32(29,1)|BITFIELD_MASK32(14,5), // formatMask
        (0x0<<29)|(0x1<<14) // formatValue
    },
    {
        // #36:  MSD1W_US  A32 Untyped Surface Write
        Platform::GEN8, // platform
        MType::MF_USW_A32_D32, // type
        MKind::STORE, // kind
        "us", // mnemonic
        {{19,&MFT_H},{12,&MFT_SM3},{8,&MFT_CMaskUW},{0,&MFT_BTS_SLM_A32}}, // fields
        {2, -1}, // argIndices
        MAddrModel::BTS_SLM_A32, // addrModel
        SFID::DC1, // sfid
        BITFIELD_MASK32(29,1)|BITFIELD_MASK32(14,5), // formatMask
        (0x0<<29)|(0x9<<14) // formatValue
    },
    {
        // #37:  MSD1R_TS  A32 Typed Surface Read
        Platform::GEN8, // platform
        MType::MF_TSR_A32_D32, // type
        MKind::LOAD, // kind
        "ts", // mnemonic
        {{19,&MFT_H},{12,&MFT_SG3},{8,&MFT_CMask},{0,&MFT_BTS}}, // fields
        {1,2, -1}, // argIndices
        MAddrModel::BTS_FAMILY, // addrModel
        SFID::DC1, // sfid
        BITFIELD_MASK32(29,1)|BITFIELD_MASK32(14,5), // formatMask
        (0x0<<29)|(0x5<<14) // formatValue
    },
    {
        // #38:  MSD1W_TS  A32 Typed Surface Write
        Platform::GEN8, // platform
        MType::MF_TSW_A32_D32, // type
        MKind::STORE, // kind
        "ts", // mnemonic
        {{19,&MFT_H},{12,&MFT_SG3},{8,&MFT_CMaskUW},{0,&MFT_BTS}}, // fields
        {1,2, -1}, // argIndices
        MAddrModel::BTS_FAMILY, // addrModel
        SFID::DC1, // sfid
        BITFIELD_MASK32(29,1)|BITFIELD_MASK32(14,5), // formatMask
        (0x0<<29)|(0xD<<14) // formatValue
    },

};

const MFormat *iga::GetMFormatTable(size_t &len) {
    len = sizeof(MFORMAT_TABLE)/sizeof(MFORMAT_TABLE[0]);
    return MFORMAT_TABLE;
}
