/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _COMMON_BINARYENCODING_H_
#define _COMMON_BINARYENCODING_H_

#include "Assertions.h"
#include "FlowGraph.h"
#include "G4_Kernel.hpp"
#include "Timer.h"

#include <optional>
#include <unordered_map>

///////////////////////////////////////////////////////////////////////////////
// Constants
///////////////////////////////////////////////////////////////////////////////
const uint32_t INST_SIZE = 16; // bytes
const uint32_t JUMP_INST_COUNT_SIZE = INST_SIZE / 2;

#define BYTES_PER_INST 16
#define BYTES_PER_OWORD 16
#define DWORDS_PER_INST 4
#define BITS_PER_DWORD 32

const uint32_t NUM_REGISTER_BYTES = 1 << 5;

const uint32_t SCRATCH_BINDING_TABLE_INDEX = 255;

#define ES_1_CHANNEL 0
#define ES_2_CHANNELS 1
#define ES_4_CHANNELS 2
#define ES_8_CHANNELS 3
#define ES_16_CHANNELS 4
#define ES_32_CHANNELS 5

typedef enum _RegFile_ {
  REG_FILE_A,
  REG_FILE_R,
  REG_FILE_M,
  REG_FILE_I
} RegFile;

typedef enum _ArchRegFile_ {      // (ARF Registers -- Overview):
  ARCH_REG_FILE_NULL = 0x00,      // 0000 null    Null register
  ARCH_REG_FILE_A = 0x01,         // 0001 a0.#    Address register
  ARCH_REG_FILE_ACC = 0x02,       // 0010 acc#    Accumulator register
  ARCH_REG_FILE_F = 0x03,         // 0011 f#.#    Flag register
  ARCH_REG_FILE_CE_REG = 0x04,    // 0100 ce#     Channel Enable register
  ARCH_REG_FILE_MSG_REG = 0x05,   // 0101 msg+    Message Control Register
  ARCH_REG_FILE_SP_REG = 0x06,    // 0110 sp      Stack Pointer Register
  ARCH_REG_FILE_STATE_REG = 0x07, // 0111 sr0.#   State register
  ARCH_REG_FILE_CNTL_REG = 0x08,  // 1000 cr0.#   Control register
  ARCH_REG_FILE_NCNT_REG = 0x09,  // 1001 n#      Notification count register
  ARCH_REG_FILE_IP = 0x0A,        // 1010 ip      Instruction pointer register
  ARCH_REG_FILE_TDR_REG = 0x0B,   // 1011 tdr     Thread dependency register
  ARCH_REG_FILE_TM_REG = 0x0C,    // 1100 tm0     TimeStamp register
  ARCH_REG_FILE_FC_REG = 0x0D,    // 1101 fc#.#   Flow Control register
  ARCH_REG_FILE_DBG_REG = 0x0F    // 1111 dbg0    Debug only
} ArchRegFile;

enum class Align1PredCtrl {
  NONE,
  SEQUENTIAL,
  ANYV,
  ALLV,
  ANY2H,
  ALL2H,
  ANY4H,
  ALL4H,
  ANY8H,
  ALL8H,
  ANY16H,
  ALL16H,
  ANY32H,
  ALL32H
};

typedef enum _AddrMode_ { ADDR_MODE_IMMED, ADDR_MODE_INDIR } AddrMode;

typedef enum _ChanSel_ {
  CHAN_SEL_X,
  CHAN_SEL_Y,
  CHAN_SEL_Z,
  CHAN_SEL_W,
  CHAN_SEL_UNDEF
} ChanSel;

// Source operand swizzles for Align16 instructions. While ISA supports
// all permutations of .xyzw, compiler only ever generates "xyzw" (default),
// "r" (scalar), or "xyxy" and "zwzw" (for 64-bit type).
enum class SrcSwizzle {
  XYZW,
  R,
  XYXY,
  ZWZW,
};

#define NUM_REGISTER_CHANNELS 4

namespace vISA {
class ForwardJmpOffset {
public:
  G4_INST *inst;
  int32_t offset;
  ForwardJmpOffset(G4_INST *_inst, int32_t _offset)
      : inst(_inst), offset(_offset){};
};

//===----------------------------------------------------------------------===//
/// \brief Binary instruction wrapper
///
class BinInst {
private:
  bool is3Src;

public:
  // constructor: initializing to be 0
  union {
    uint32_t DWords[DWORDS_PER_INST];
    char Bytes[BYTES_PER_INST];
  };
  uint32_t localInstNumber; // Instruction's position within the BB
  uint32_t instNumber;      // Global instruction number
  bool compacted = false;

  uint64_t genOffset = 0;

  BinInst() {
    DWords[0] = 0;
    DWords[1] = 0;
    DWords[2] = 0;
    DWords[3] = 0;
    is3Src = false;
    localInstNumber = 0;
    instNumber = 0;
  }

  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }

  inline uint32_t GetBits(const int HighBit, const int LowBit) {
    vISA_ASSERT(HighBit >= LowBit, "high bit must be >= low bit");

    int retValue;
    int HighDword = HighBit / BITS_PER_DWORD;
    int LowDword = LowBit / BITS_PER_DWORD;
    if (HighDword == LowDword) {
      uint32_t Dword = HighDword;
      int mask = (int)(0xffffffff >> (32 - (HighBit - LowBit + 1)));
      uint32_t shift = LowBit - (Dword * BITS_PER_DWORD);

      retValue = DWords[Dword] >> shift;
      retValue &= mask;
    } else {
      // only allow reading from at most 2 dwords
      vISA_ASSERT(HighDword == LowDword + 1, "can't return > 32 bits");
      uint32_t shift = LowBit - (LowDword * BITS_PER_DWORD);
      retValue = DWords[LowDword] >> shift;
      retValue |= GetBits(HighBit, (LowDword + 1) * BITS_PER_DWORD)
                  << (32 - shift);
    }

    return retValue;
  }

  inline void SetBits(const uint32_t HighBit, const uint32_t LowBit,
                      const uint32_t value) {
    vISA_ASSERT(HighBit >= LowBit, "high bit must be >= low bit");
    vISA_ASSERT(HighBit / BITS_PER_DWORD == LowBit / BITS_PER_DWORD,
                 "function doesn't handle bits crossing dword");

    uint32_t maxvalue =
        ((1 << (HighBit - LowBit)) - 1) | (1 << (HighBit - LowBit));
    uint32_t newvalue = value;
    newvalue &= maxvalue;
    uint32_t Dword = HighBit / BITS_PER_DWORD;

    int mask = (int)(0xffffffff >> (32 - (HighBit - LowBit + 1)));
    uint32_t shift = LowBit - (Dword * BITS_PER_DWORD);
    mask <<= shift;
    DWords[Dword] &= ~mask;
    DWords[Dword] |= (newvalue << shift);
  }

  void SetIs3Src(bool _is3src) { is3Src = _is3src; };
  bool GetIs3Src() { return is3Src; };

private:
  bool dontCompact = false;
  bool mustCompact = false;

public:
  void SetDontCompactFlag(bool _d) { dontCompact = _d; };
  bool GetDontCompactFlag() { return dontCompact; };

  void SetMustCompactFlag(bool _m) { mustCompact = _m; };
  bool GetMustCompactFlag() { return mustCompact; };

  void SetInstNumber(uint32_t _n) { instNumber = _n; };
  uint32_t GetInstNumber() { return instNumber; };

  void SetGenOffset(uint64_t o) { genOffset = o; }
  uint64_t GetGenOffset() const { return genOffset; }

  bool isInitialized() { // not all 0s
    return (!(DWords[0] == 0 && DWords[1] == 0 && DWords[2] == 0 &&
              DWords[3] == 0));
  }
};
} // namespace vISA

struct DebugFormatHeader {
  uint32_t magic;
  uint16_t stringCount;
  std::vector<std::string> strings;
  uint16_t offset;
};

namespace vISA {
class DebugInfoFormat {
private:
  std::string m_kernelName;
  mutable int m_kernelNameIntern;
  int m_cisaOffset;
  uint64_t m_genOffset;

public:
  DebugInfoFormat(std::string kernelName, int cisaOffset, uint64_t genOffset)
      : m_kernelName(kernelName), m_kernelNameIntern(-1),
        m_cisaOffset(cisaOffset), m_genOffset(genOffset) {}

  virtual ~DebugInfoFormat() {}

  DebugInfoFormat(DebugInfoFormat const &d) {
    m_kernelName = d.getKernelName();
    m_kernelNameIntern = d.getKernelNameIntern();
    m_cisaOffset = d.getCisaOffset();
    m_genOffset = d.getGenOffset();
  }

  DebugInfoFormat &operator=(const DebugInfoFormat &rhs) {
    m_kernelName = rhs.getKernelName();
    m_kernelNameIntern = rhs.getKernelNameIntern();
    m_cisaOffset = rhs.getCisaOffset();
    m_genOffset = rhs.getGenOffset();

    return *this;
  }

  std::string getKernelName() const { return m_kernelName; }
  int getKernelNameIntern() const { return m_kernelNameIntern; }
  int getCisaOffset() const { return m_cisaOffset; }
  uint64_t getGenOffset() const { return m_genOffset; }

  void setKernelNameIntern(int intern) const { m_kernelNameIntern = intern; };
};
} // namespace vISA

/* A list of binary instructions */
/* FIX ME: delete each binary instruction inside BinInstList at the end*/
namespace vISA {
class BinInstList : public std::vector<BinInst *> {};
} // namespace vISA

//===----------------------------------------------------------------------===//
/// \brief Common base class for binary encoders
///

//===----------------------------------------------------------------------===//
/// \brief common IVB compaction source table
///

const uint32_t COMPACT_TABLE_SIZE = 32;
const uint32_t COMPACT_TABLE_SIZE_3SRC = 4;
[[maybe_unused]]
static uint32_t IVBCompactControlTable[COMPACT_TABLE_SIZE] = {
    0x00000002, // 000,0000,0000,0000,0010
    0x00004000, // 000,0100,0000,0000,0000
    0x00004001, // 000,0100,0000,0000,0001
    0x00004002, // 000,0100,0000,0000,0010
    0x00004003, // 000,0100,0000,0000,0011
    0x00004004, // 000,0100,0000,0000,0100
    0x00004005, // 000,0100,0000,0000,0101
    0x00004007, // 000,0100,0000,0000,0111
    0x00004008, // 000,0100,0000,0000,1000
    0x00004009, // 000,0100,0000,0000,1001
    0x0000400D, // 000,0100,0000,0000,1101
    0x00006000, // 000,0110,0000,0000,0000
    0x00006001, // 000,0110,0000,0000,0001
    0x00006002, // 000,0110,0000,0000,0010
    0x00006003, // 000,0110,0000,0000,0011
    0x00006004, // 000,0110,0000,0000,0100
    0x00006005, // 000,0110,0000,0000,0101
    0x00006007, // 000,0110,0000,0000,0111
    0x00006009, // 000,0110,0000,0000,1001
    0x0000600D, // 000,0110,0000,0000,1101
    0x00006010, // 000,0110,0000,0001,0000
    0x00006100, // 000,0110,0001,0000,0000
    0x00008000, // 000,1000,0000,0000,0000
    0x00008002, // 000,1000,0000,0000,0010
    0x00008004, // 000,1000,0000,0000,0100
    0x00008100, // 000,1000,0001,0000,0000
    0x00016000, // 001,0110,0000,0000,0000
    0x00016010, // 001,0110,0000,0001,0000
    0x00018000, // 001,1000,0000,0000,0000
    0x00018100, // 001,1000,0001,0000,0000
    0x00028000, // 010,1000,0000,0000,0000
    0x00028100  // 010,1000,0001,0000,0000
};
[[maybe_unused]]
static uint32_t IVBCompactSourceTable[COMPACT_TABLE_SIZE] = {
    0x00000000, // 000000000000
    0x00000002, // 000000000010
    0x00000010, // 000000010000
    0x00000012, // 000000010010
    0x00000018, // 000000011000
    0x00000020, // 000000100000
    0x00000028, // 000000101000
    0x00000048, // 000001001000
    0x00000050, // 000001010000
    0x00000070, // 000001110000
    0x00000078, // 000001111000
    0x00000300, // 001100000000
    0x00000302, // 001100000010
    0x00000308, // 001100001000
    0x00000310, // 001100010000
    0x00000312, // 001100010010
    0x00000320, // 001100100000
    0x00000328, // 001100101000
    0x00000338, // 001100111000
    0x00000340, // 001101000000
    0x00000342, // 001101000010
    0x00000348, // 001101001000
    0x00000350, // 001101010000
    0x00000360, // 001101100000
    0x00000368, // 001101101000
    0x00000370, // 001101110000
    0x00000371, // 001101110001
    0x00000378, // 001101111000
    0x00000468, // 010001101000
    0x00000469, // 010001101001
    0x0000046A, // 010001101010
    0x00000588  // 010110001000
};
[[maybe_unused]]
static uint32_t IVBCompactSubRegTable[COMPACT_TABLE_SIZE] = {
    0x00000000, // 000,0000,0000,0000
    0x00000001, // 000,0000,0000,0001
    0x00000008, // 000,0000,0000,1000
    0x0000000F, // 000,0000,0000,1111
    0x00000010, // 000,0000,0001,0000
    0x00000080, // 000,0000,1000,0000
    0x00000100, // 000,0001,0000,0000
    0x00000180, // 000,0001,1000,0000
    0x00000200, // 000,0010,0000,0000
    0x00000210, // 000,0010,0001,0000
    0x00000280, // 000,0010,1000,0000
    0x00001000, // 001,0000,0000,0000
    0x00001001, // 001,0000,0000,0001
    0x00001081, // 001,0000,1000,0001
    0x00001082, // 001,0000,1000,0010
    0x00001083, // 001,0000,1000,0011
    0x00001084, // 001,0000,1000,0100
    0x00001087, // 001,0000,1000,0111
    0x00001088, // 001,0000,1000,1000
    0x0000108E, // 001,0000,1000,1110
    0x0000108F, // 001,0000,1000,1111
    0x00001180, // 001,0001,1000,0000
    0x000011E8, // 001,0001,1110,1000
    0x00002000, // 010,0000,0000,0000
    0x00002180, // 010,0001,1000,0000
    0x00003000, // 011,0000,0000,0000
    0x00003C87, // 011,1100,1000,0111
    0x00004000, // 100,0000,0000,0000
    0x00005000, // 101,0000,0000,0000
    0x00006000, // 110,0000,0000,0000
    0x00007000, // 111,0000,0000,0000
    0x0000701C  // 111,0000,0001,1100
};
[[maybe_unused]]
static uint32_t IVBCompactDataTypeTable[COMPACT_TABLE_SIZE] = {
    0x00008001, // 00,1000,0000,0000,0001
    0x00008020, // 00,1000,0000,0010,0000
    0x00008021, // 00,1000,0000,0010,0001
    0x00008061, // 00,1000,0000,0110,0001
    0x000080BD, // 00,1000,0000,1011,1101
    0x000082FD, // 00,1000,0010,1111,1101
    0x000083A1, // 00,1000,0011,1010,0001
    0x000083A5, // 00,1000,0011,1010,0101
    0x000083BD, // 00,1000,0011,1011,1101
    0x00008421, // 00,1000,0100,0010,0001
    0x00008C20, // 00,1000,1100,0010,0000
    0x00008C21, // 00,1000,1100,0010,0001
    0x000094A5, // 00,1001,0100,1010,0101
    0x00009CA4, // 00,1001,1100,1010,0100
    0x00009CA5, // 00,1001,1100,1010,0101
    0x0000F3BD, // 00,1111,0011,1011,1101
    0x0000F79D, // 00,1111,0111,1001,1101
    0x0000F7BC, // 00,1111,0111,1011,1100
    0x0000F7BD, // 00,1111,0111,1011,1101
    0x0000FFBC, // 00,1111,1111,1011,1100
    0x0000020C, // 00,0000,0010,0000,1100
    0x0000803D, // 00,1000,0000,0011,1101
    0x000080A5, // 00,1000,0000,1010,0101
    0x00008420, // 00,1000,0100,0010,0000
    0x000094A4, // 00,1001,0100,1010,0100
    0x00009C84, // 00,1001,1100,1000,0100
    0x0000A509, // 00,1010,0101,0000,1001
    0x0000DFBD, // 00,1101,1111,1011,1101
    0x0000FFBD, // 00,1111,1111,1011,1101
    0x0000BDAC, // 00,1011,1101,1010,1100
    0x0000A528, // 00,1010,0101,0010,1000
    0x0000AD28  // 00,1010,1101,0010,100
};

// DataTypeIndex Compact Instruction Field Mappings 1/2 Source Operands DevBDW
// DataTypeIndex 21-Bit Mapping Mapped Meaning
[[maybe_unused]]
static uint32_t BDWCompactDataTypeTable[COMPACT_TABLE_SIZE] = {
    0x00040001, // 001000000000000000001
    0x00040040, // 001000000000001000000
    0x00040041, // 001000000000001000001
    0x000400C1, // 001000000000011000001
    0x0004015D, // 001000000000101011101
    0x000405DD, // 001000000010111011101
    0x00040741, // 001000000011101000001
    0x00040745, // 001000000011101000101
    0x0004075D, // 001000000011101011101
    0x00041041, // 001000001000001000001
    0x00043040, // 001000011000001000000
    0x00043041, // 001000011000001000001
    0x00045145, // 001000101000101000101
    0x00047144, // 001000111000101000100
    0x00047145, // 001000111000101000101
    0x0005C75D, // 001011100011101011101
    0x0005D71D, // 001011101011100011101
    0x0005D75C, // 001011101011101011100
    0x0005D75D, // 001011101011101011101
    0x0005F75C, // 001011111011101011100
    0x0000040C, // 000000000010000001100
    0x0004005D, // 001000000000001011101
    0x00040145, // 001000000000101000101
    0x00041040, // 001000001000001000000
    0x00045144, // 001000101000101000100
    0x00047104, // 001000111000100000100
    0x00049209, // 001001001001000001001
    0x0005775D, // 001010111011101011101
    0x0005F75D, // 001011111011101011101
    0x0004F34C, // 001001111001101001100
    0x00049248, // 001001001001001001000
    0x0004B248, // 001001011001001001000
};

[[maybe_unused]]
static uint32_t ICLCompactDataTypeTable[COMPACT_TABLE_SIZE] = {
    0x40001, // 001000000000000000001
    0x40040, // 001000000000001000000
    0x40041, // 001000000000001000001
    0x400C1, // 001000000000011000001
    0x40165, // 001000000000101100101
    0x40BE5, // 001000000101111100101
    0x40941, // 001000000100101000001
    0x40945, // 001000000100101000101
    0x40965, // 001000000100101100101
    0x41041, // 001000001000001000001
    0x43040, // 001000011000001000000
    0x43041, // 001000011000001000001
    0x45145, // 001000101000101000101
    0x47144, // 001000111000101000100
    0x47145, // 001000111000101000101
    0x64965, // 001100100100101100101
    0x65925, // 001100101100100100101
    0x65964, // 001100101100101100100
    0x65965, // 001100101100101100101
    0x67964, // 001100111100101100100
    0x0040C, // 000000000010000001100
    0x40065, // 001000000000001100101
    0x40145, // 001000000000101000101
    0x41040, // 001000001000001000000
    0x45144, // 001000101000101000100
    0x47104, // 001000111000100000100
    0x49209, // 001001001001000001001
    0x6F965, // 001101111100101100101
    0x67965, // 001100111100101100101
    0x4F34C, // 001001111001101001100
    0x49248, // 001001001001001001000
    0x4B248, // 001001011001001001000
};

// ControlIndex Compact Instruction Field Mappings 3 Source Operands BDW/CHV
[[maybe_unused]]
static uint32_t BDWCompactControlTable3Src[COMPACT_TABLE_SIZE_3SRC] = {
    0x00806001, // 100000000110000000000001
    0x00006001, // 000000000110000000000001
    0x00008001, // 000000001000000000000001
    0x00008021, // 000000001000000000100001
};

// SourceIndex Compact Instruction Field Mappings 3 Source Operands BDW/CHV
[[maybe_unused]]
static uint64_t BDWCompactSourceTable3Src[COMPACT_TABLE_SIZE_3SRC] = {
    0x07272720F000, // 0001110010011100100111001000001111000000000000
    0x07272720F002, // 0001110010011100100111001000001111000000000010
    0x07272720F008, // 0001110010011100100111001000001111000000001000
    0x07272720F020, // 0001110010011100100111001000001111000000100000
};

static struct _CompactDataTypeTable_ {
  union Data {
    struct {
      uint32_t Bits_046_032 : 15;
      uint32_t Bits_063_061 : 3;
      uint32_t Reserved : 14;
    } sData;
    uint32_t ulData;
  };

  uint32_t GetBits_063_061(uint32_t index) {
    vISA_ASSERT(index < COMPACT_TABLE_SIZE,
                 "Out of Control Bit Datatype Table range.");
    Data data;
    data.ulData = Values[index];
    return data.sData.Bits_063_061;
  }

  uint32_t GetBits_046_032(uint32_t index) {
    vISA_ASSERT(index < COMPACT_TABLE_SIZE,
                 "Out of Control Bit Datatype Table range.");
    Data data;
    data.ulData = Values[index];
    return data.sData.Bits_046_032;
  }

  bool FindIndex(uint32_t &index, uint32_t bits_063_061,
                 uint32_t bits_046_032) {
    for (index = 0; index < COMPACT_TABLE_SIZE; ++index) {
      Data data;
      data.ulData = Values[index];
      if (data.sData.Bits_063_061 == bits_063_061 &&
          data.sData.Bits_046_032 == bits_046_032) {
        return true;
      }
    }
    return false;
  }

  uint32_t Values[COMPACT_TABLE_SIZE];
} CompactDataTypeTable;

static struct _CompactSubRegTable_ {
  union Data {
    struct {
      uint32_t Bits_052_048 : 5;
      uint32_t Bits_068_064 : 5;
      uint32_t Bits_100_096 : 5;
      uint32_t Reserved : 17;
    } sData;
    uint32_t ulData;
  };

  uint32_t GetBits_100_096(uint32_t index) {
    vISA_ASSERT(index < COMPACT_TABLE_SIZE,
                 "Out of Control Bit Subreg Table range.");
    Data data;
    data.ulData = Values[index];
    return data.sData.Bits_100_096;
  }

  uint32_t GetBits_068_064(uint32_t index) {
    vISA_ASSERT(index < COMPACT_TABLE_SIZE,
                 "Out of Control Bit Subreg Table range.");
    Data data;
    data.ulData = Values[index];
    return data.sData.Bits_068_064;
  }

  uint32_t GetBits_052_048(uint32_t index) {
    vISA_ASSERT(index < COMPACT_TABLE_SIZE,
                 "Out of Control Bit Subreg Table range.");
    Data data;
    data.ulData = Values[index];
    return data.sData.Bits_052_048;
  }

  bool FindIndex(uint32_t &index, uint32_t bits_100_096, uint32_t bits_068_064,
                 uint32_t bits_052_048) {
    for (index = 0; index < COMPACT_TABLE_SIZE; ++index) {
      Data data;
      data.ulData = Values[index];
      if (data.sData.Bits_100_096 == bits_100_096 &&
          data.sData.Bits_068_064 == bits_068_064 &&
          data.sData.Bits_052_048 == bits_052_048) {
        return true;
      }
    }
    return false;
  }

  bool HasMatch(uint32_t &index, uint32_t bits_100_096, uint32_t bits_068_064,
                uint32_t bits_052_048, unsigned int match_mask) {
    bool match[3];
    match[0] = (match_mask & 0x1) == 0x1;
    match[1] = (match_mask & 0x2) == 0x2;
    match[2] = (match_mask & 0x4) == 0x4;

    for (index = 0; index < COMPACT_TABLE_SIZE; ++index) {
      Data data;
      data.ulData = Values[index];
      bool found[3] = {false, false, false};

      for (int i = 0; i < 3; i++) {
        if (!match[i])
          found[i] = true;
      }

      if (match[0] && (data.sData.Bits_052_048 == bits_052_048)) {
        found[0] = true;
      }

      if (match[1] && (data.sData.Bits_068_064 == bits_068_064)) {
        found[1] = true;
      }

      if (match[2] && (data.sData.Bits_100_096 == bits_100_096)) {
        found[2] = true;
      }

      if (found[0] && found[1] && found[2])
        return true;
    }

    return false;
  }

  uint32_t Values[COMPACT_TABLE_SIZE];
} CompactSubRegTable;

static struct _CompactSourceTable_ {
  uint32_t GetBits_120_109(uint32_t index) {
    vISA_ASSERT(index < COMPACT_TABLE_SIZE,
                 "Out of Control Bit Source Table range.");
    return Values[index];
  }

  uint32_t GetBits_088_077(uint32_t index) {
    vISA_ASSERT(index < COMPACT_TABLE_SIZE,
                 "Out of Control Bit Source Table range.");
    return Values[index];
  }

  bool FindIndex(uint32_t &index, uint32_t bits) {
    for (index = 0; index < COMPACT_TABLE_SIZE; ++index) {
      if (Values[index] == bits) {
        return true;
      }
    }
    return false;
  }

  uint32_t Values[COMPACT_TABLE_SIZE];
} CompactSourceTable;

namespace vISA {
class _BDWCompactControlTable_ {
  const static unsigned maxEntry = 111;
  Mem_Manager &mem;

  struct HashNode {
    uint32_t key;
    uint8_t idx;
    HashNode *next;

    HashNode(uint32_t k, uint8_t i, HashNode *nxt)
        : key(k), idx(i), next(nxt) {}
    void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }
  };

  HashNode *table[maxEntry];

  unsigned FindEntry(uint32_t key) {
    return key % maxEntry;
    // return (key & 0xF) | (key >> 9);
  }

public:
  _BDWCompactControlTable_(Mem_Manager &m) : mem(m) {
    for (unsigned i = 0; i < maxEntry; i++)
      table[i] = NULL;
  }

  bool FindIndex(uint32_t &index, uint32_t bits_033_032, uint32_t bits_031_031,
                 uint32_t bits_023_012, uint32_t bits_010_009,
                 uint32_t bits_034_034, uint32_t bits_008_008) {
    uint32_t i = bits_008_008 | (bits_034_034 << 1) | (bits_010_009 << 2) |
                 (bits_023_012 << 4) | (bits_031_031 << 16) |
                 (bits_033_032 << 17);
    for (HashNode *n = table[FindEntry(i)]; n != NULL; n = n->next) {
      if (n->key == i) {
        index = n->idx;
        return true;
      }
    }
    return false;
  }

  void AddIndex(uint32_t key, uint8_t idx) {
    int entry = FindEntry(key);
    table[entry] = new (mem) HashNode(key, idx, table[entry]);
  }
};

class _BDWCompactSourceTable_ {
  const static unsigned maxEntry = 61;
  Mem_Manager &mem;

  struct HashNode {
    uint32_t key;
    uint8_t idx;
    HashNode *next;

    HashNode(uint32_t k, uint8_t i, HashNode *nxt)
        : key(k), idx(i), next(nxt) {}
    void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }
  };

  HashNode *table[maxEntry];

  unsigned FindEntry(uint32_t key) {
    return key % maxEntry;
    // return (key & 0x3) | ((key & 0x3f8) >> 1);
  }

public:
  _BDWCompactSourceTable_(Mem_Manager &m) : mem(m) {
    for (unsigned i = 0; i < maxEntry; i++)
      table[i] = NULL;
  }

  bool FindIndex(uint32_t &index, uint32_t bits) {
    for (HashNode *n = table[FindEntry(bits)]; n != NULL; n = n->next) {
      if (n->key == bits) {
        index = n->idx;
        return true;
      }
    }
    return false;
  }

  void AddIndex(uint32_t key, uint8_t idx) {
    int entry = FindEntry(key);
    table[entry] = new (mem) HashNode(key, idx, table[entry]);
  }

  uint32_t GetBits_120_109(uint32_t index) {
    return IVBCompactSourceTable[index];
  }

  uint32_t GetBits_088_077(uint32_t index) {
    return IVBCompactSourceTable[index];
  }
};

class _BDWCompactSubRegTable_ {
  const static unsigned maxEntry = 37;
  const static unsigned maxEntry1 = 37;
  const static unsigned maxEntry2 = 37;

  Mem_Manager &mem;

  struct HashNode {
    uint32_t key;
    uint8_t idx;
    HashNode *next;

    HashNode(uint32_t k, uint8_t i, HashNode *nxt)
        : key(k), idx(i), next(nxt) {}
    void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }
  };

  HashNode *table[maxEntry];
  HashNode *table1[maxEntry1];
  HashNode *table2[maxEntry2];

  unsigned FindEntry(uint32_t key) {
    return key % maxEntry;
    // return (key >> 12) | ((key & 0xF) << 3) | (key & 0x380) ;
  }

  unsigned FindEntry1(uint32_t key) { return key % maxEntry1; }

  unsigned FindEntry2(uint32_t key) { return key % maxEntry2; }

public:
  _BDWCompactSubRegTable_(Mem_Manager &m) : mem(m) {
    for (unsigned i = 0; i < maxEntry; i++) {
      table[i] = table1[i] = table2[i] = NULL;
    }
  }

  bool FindIndex(uint32_t &index, uint32_t bits_100_096, uint32_t bits_068_064,
                 uint32_t bits_052_048) {
    uint32_t i = bits_052_048 | (bits_068_064 << 5) | (bits_100_096 << 10);
    for (HashNode *n = table[FindEntry(i)]; n != NULL; n = n->next) {
      if (n->key == i) {
        index = n->idx;
        return true;
      }
    }
    return false;
  }

  bool FindIndex1(uint32_t &index, uint32_t bits_052_048) {
    for (HashNode *n = table1[FindEntry1(bits_052_048)]; n != NULL;
         n = n->next) {
      if (n->key == bits_052_048) {
        index = n->idx;
        return true;
      }
    }
    return false;
  }

  bool FindIndex2(uint32_t &index, uint32_t bits_068_064,
                  uint32_t bits_052_048) {
    uint32_t i = bits_052_048 | (bits_068_064 << 5);
    for (HashNode *n = table2[FindEntry2(i)]; n != NULL; n = n->next) {
      if (n->key == i) {
        index = n->idx;
        return true;
      }
    }
    return false;
  }

  void AddIndex(uint32_t key, uint8_t idx) {
    int entry = FindEntry(key);
    table[entry] = new (mem) HashNode(key, idx, table[entry]);
  }

  void AddIndex1(uint32_t key, uint8_t idx) {
    int entry = FindEntry1(key);
    HashNode *n = table1[entry];
    for (; n != NULL; n = n->next) {
      if (n->key == key) {
        break;
      }
    }
    if (n == NULL) {
      table1[entry] = new (mem) HashNode(key, idx, table1[entry]);
    }
  }

  void AddIndex2(uint32_t key, uint8_t idx) {
    int entry = FindEntry2(key);
    HashNode *n = table2[entry];
    for (; n != NULL; n = n->next) {
      if (n->key == key) {
        break;
      }
    }
    if (n == NULL) {
      table2[entry] = new (mem) HashNode(key, idx, table2[entry]);
    }
  }

  uint32_t GetBits_100_096(uint32_t index) {
    return IVBCompactSubRegTable[index] >> 10;
  }

  uint32_t GetBits_068_064(uint32_t index) {
    return (IVBCompactSubRegTable[index] & 0x000003E0) >> 5;
  }

  uint32_t GetBits_052_048(uint32_t index) {
    return IVBCompactSubRegTable[index] & 0x0000001F;
  }
};

// add Str in below struct to differentiate its loop up table
class _BDWCompactDataTypeTableStr_ {
  const static unsigned maxEntry = 111;
  Mem_Manager &mem;

  struct HashNode {
    uint32_t key;
    uint8_t idx;
    HashNode *next;

    HashNode(uint32_t k, uint8_t i, HashNode *nxt)
        : key(k), idx(i), next(nxt) {}
    void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }
  };

  HashNode *table[maxEntry];

  unsigned FindEntry(uint32_t key) {
    return key % maxEntry;
    // return ((key & 0x3FC) >> 2) | ((key & 0x3000) >> 4);
  }

public:
  _BDWCompactDataTypeTableStr_(Mem_Manager &m) : mem(m) {
    for (unsigned i = 0; i < maxEntry; i++)
      table[i] = NULL;
  }

  bool FindIndex(uint32_t &index, uint32_t bits_063_061, uint32_t bits_094_089,
                 uint32_t bits_046_035) {
    uint32_t i = 0;
    i = bits_046_035 | (bits_094_089 << 12) | (bits_063_061 << 18);
    for (HashNode *n = table[FindEntry(i)]; n != NULL; n = n->next) {
      if (n->key == i) {
        index = n->idx;
        return true;
      }
    }
    return false;
  }

  void AddIndex(uint32_t key, uint8_t idx) {
    int entry = FindEntry(key);
    table[entry] = new (mem) HashNode(key, idx, table[entry]);
  }
};
} // namespace vISA
extern unsigned long bitsSrcRegFile[4];     // = {128, 128, 128, 128};
extern unsigned long bits3SrcFlagRegNum[2]; // = {128, 128};
extern unsigned long bitsFlagRegNum[2];     // = {128, 128};

#define SET_BIT_RANGE(field, high, low)                                        \
  (field)[0] = high;                                                           \
  (field)[1] = low;

#define SET_BIT_RANGES(field, high1, low1, high2, low2)                        \
  (field)[0] = high1;                                                          \
  (field)[1] = low1;                                                           \
  (field)[2] = high2;                                                          \
  (field)[3] = low2;

// below are extended for BDW/CHV compaction
namespace vISA {
class _CompactControl3Src_ {
  uint32_t Values[COMPACT_TABLE_SIZE_3SRC];
  union Data {
    struct {
      uint32_t Bits_028_008 : 21;
      uint32_t Bits_034_032 : 3;
      // bits 36-35 are only for CHV, reserved for BDW
      uint32_t Bits_036_035 : 2;
      uint32_t Reserved : 6;
    } sData;
    uint32_t ulData;
  };

  uint32_t GetBit_028_008(uint32_t index) {
    vISA_ASSERT(index < COMPACT_TABLE_SIZE_3SRC,
                 "Out of Control Bit Compact Table range.");
    Data data;
    data.ulData = Values[index];
    return data.sData.Bits_028_008;
  }

  uint32_t GetBits_034_032(uint32_t index) {
    vISA_ASSERT(index < COMPACT_TABLE_SIZE_3SRC,
                 "Out of Control Bit Compact Table range.");
    Data data;
    data.ulData = Values[index];
    return data.sData.Bits_034_032;
  }

  uint32_t GetBits_036_035(uint32_t index) {
    vISA_ASSERT(index < COMPACT_TABLE_SIZE_3SRC,
                 "Out of Control Bit Compact Table range.");
    Data data;
    data.ulData = Values[index];
    return data.sData.Bits_036_035;
  }

public:
  bool FindBDWIndex(uint32_t &index, uint32_t bits_034_032,
                    uint32_t bits_028_008) {
    for (index = 0; index < COMPACT_TABLE_SIZE_3SRC; ++index) {
      Data data;
      data.ulData = Values[index];
      if (data.sData.Bits_034_032 == bits_034_032 &&
          data.sData.Bits_028_008 == bits_028_008) {
        return true;
      }
    }
    return false;
  }

  bool FindCHVIndex(uint32_t &index, uint32_t bits_036_035,
                    uint32_t bits_034_032, uint32_t bits_028_008) {
    for (index = 0; index < COMPACT_TABLE_SIZE_3SRC; ++index) {
      Data data;
      data.ulData = Values[index];
      if (data.sData.Bits_036_035 == bits_036_035 &&
          data.sData.Bits_034_032 == bits_034_032 &&
          data.sData.Bits_028_008 == bits_028_008) {
        return true;
      }
    }
    return false;
  }

  _CompactControl3Src_(TARGET_PLATFORM platform) {
    if (platform == GENX_BDW) {
      for (int i = 0; i < (int)COMPACT_TABLE_SIZE_3SRC; i++) {
        Values[i] = BDWCompactControlTable3Src[i];
      }
    } else if (platform >= GENX_CHV) {
      // CHV is the same as BDW, except for:
      // -- 2 extra leading bits (00) for the control table (26 v. 24 bits)
      // -- 3 extra leading bits (000) for the source table (49 v. 46 bits)
      // as such, we use the same tables from BDW
      // initialization of 3src compaction table for both CHV and SKL
      for (int i = 0; i < (int)COMPACT_TABLE_SIZE_3SRC; i++) {
        Values[i] = BDWCompactControlTable3Src[i];
      }
    }
  }
};

class _CompactSourceTable3Src_ {
  uint64_t Values[COMPACT_TABLE_SIZE_3SRC];
  union Data {
    struct {
      // we have to use uint64_t since Bits_093_086 straddles dword
      uint64_t Bits_055_037 : 19;
      uint64_t Bits_072_065 : 8;
      uint64_t Bits_093_086 : 8;
      uint64_t Bits_114_107 : 8;
      uint64_t Bits_083_083 : 1;
      uint64_t Bits_104_104 : 1;
      uint64_t Bits_125_125 : 1;
      uint64_t Reserved : 18;
    } sData;
    uint64_t ulData;
  };

public:
  bool FindIndex(uint32_t &index, uint32_t bits_125_125, uint32_t bits_104_104,
                 uint32_t bits_083_083, uint32_t bits_114_107,
                 uint32_t bits_093_086, uint32_t bits_072_065,
                 uint32_t bits_055_037) {

    for (index = 0; index < COMPACT_TABLE_SIZE_3SRC; ++index) {
      Data data;
      data.ulData = Values[index];
      if (data.sData.Bits_125_125 == bits_125_125 &&
          data.sData.Bits_104_104 == bits_104_104 &&
          data.sData.Bits_083_083 == bits_083_083 &&
          data.sData.Bits_114_107 == bits_114_107 &&
          data.sData.Bits_093_086 == bits_093_086 &&
          data.sData.Bits_072_065 == bits_072_065 &&
          data.sData.Bits_055_037 == bits_055_037) {
        return true;
      }
    }
    return false;
  }

  _CompactSourceTable3Src_(TARGET_PLATFORM platform) {
    if (platform == GENX_BDW) {
      for (int i = 0; i < (int)COMPACT_TABLE_SIZE_3SRC; i++) {
        Values[i] = BDWCompactSourceTable3Src[i];
      }
    }
  }
};

class _CompactSourceTable3SrcCHV_ {
public:
  union Data {
    struct {
      uint64_t Bits_055_037 : 19;
      uint64_t Bits_072_065 : 8;
      uint64_t Bits_093_086 : 8;
      uint64_t Bits_114_107 : 8;
      uint64_t Bits_084_083 : 2;
      uint64_t Bits_105_104 : 2;
      uint64_t Bits_126_125 : 2;
      uint64_t Reserved : 17;
    } sData;
    uint64_t ulData;
  };

  bool FindIndex(uint32_t &index, uint32_t bits_126_125, uint32_t bits_105_104,
                 uint32_t bits_084_083, uint32_t bits_114_107,
                 uint32_t bits_093_086, uint32_t bits_072_065,
                 uint32_t bits_055_037) {

    for (index = 0; index < COMPACT_TABLE_SIZE_3SRC; ++index) {
      Data data;
      data.ulData = Values[index];
      if (data.sData.Bits_126_125 == bits_126_125 &&
          data.sData.Bits_105_104 == bits_105_104 &&
          data.sData.Bits_084_083 == bits_084_083 &&
          data.sData.Bits_114_107 == bits_114_107 &&
          data.sData.Bits_093_086 == bits_093_086 &&
          data.sData.Bits_072_065 == bits_072_065 &&
          data.sData.Bits_055_037 == bits_055_037) {
        return true;
      }
    }
    return false;
  }

  _CompactSourceTable3SrcCHV_(TARGET_PLATFORM platform) {
    if (platform >= GENX_CHV) {
      // CHV is the same as BDW, except for:
      // -- 2 extra leading bits (00) for the control table (26 v. 24 bits)
      // -- 3 extra leading bits (000) for the source table (49 v. 46 bits)
      // as such, we use the same tables from BDW
      // initialization of 3src compaction table for both CHV and SKL
      for (int i = 0; i < (int)COMPACT_TABLE_SIZE_3SRC; i++) {
        Values[i] = BDWCompactSourceTable3Src[i];
      }
    }
  }
  uint64_t Values[COMPACT_TABLE_SIZE_3SRC];
};
} // namespace vISA

namespace vISA {

typedef enum {
  PRED_ALIGN16_DEFAULT = 1,
  PRED_ALIGN16_X = 2,
  PRED_ALIGN16_Y = 3,
  PRED_ALIGN16_Z = 4,
  PRED_ALIGN16_W = 5,
  PRED_ALIGN16_ANY4H = 6,
  PRED_ALIGN16_ALL4H = 7
} G4_Align16_Predicate_Control;

enum ChannelEnable {
  NoChannelEnable = 0,
  ChannelEnable_X = 1,
  ChannelEnable_Y = 2,
  ChannelEnable_XY = 3,
  ChannelEnable_Z = 4,
  ChannelEnable_W = 8,
  ChannelEnable_ZW = 0xC,
  ChannelEnable_XYZW = 0xF
};

class BinaryEncodingBase {
public:
  _BDWCompactControlTable_ BDWCompactControlTable;
  _BDWCompactSourceTable_ BDWCompactSourceTable;
  _BDWCompactSubRegTable_ BDWCompactSubRegTable;
  _BDWCompactDataTypeTableStr_ BDWCompactDataTypeTableStr;
  _CompactControl3Src_ CompactControlTable3Src;
  _CompactSourceTable3Src_ CompactSourceTable3Src;
  _CompactSourceTable3SrcCHV_ CompactSourceTable3SrcCHV;

  BinaryEncodingBase(Mem_Manager &m, G4_Kernel &k, const std::string& fname)
      : BDWCompactControlTable(m), BDWCompactSourceTable(m),
        BDWCompactSubRegTable(m), BDWCompactDataTypeTableStr(m),
        CompactControlTable3Src(k.getPlatform()),
        CompactSourceTable3Src(k.getPlatform()),
        CompactSourceTable3SrcCHV(k.getPlatform()), mem(m), fileName(fname),
        kernel(k), instCounts(0) {}

  typedef enum { SUCCESS, FAILURE } Status;

  BinInstList &getBinInstList() { return binInstList; }

  virtual ~BinaryEncodingBase() {}

  bool isBBBinInstEmpty(G4_BB *bb);
  G4_INST *getFirstNonLabelInst(G4_BB *bb);

  void BuildLabelMap(G4_INST *, int &, int &, int &, int &);
  virtual void SetCompactCtrl(BinInst *mybin, uint32_t value) = 0;
  virtual uint32_t GetCompactCtrl(BinInst *mybin) = 0;

  void FixInst();
  void FixAlign16Inst(G4_INST *inst);
  void FixMathInst(G4_INST *inst);

  void ProduceBinaryBuf(void *&);
  void *EmitBinary(uint32_t &);
  virtual Status WriteToDatFile();

  virtual void DoAll() = 0;

  uint32_t GetInstCounts() { return instCounts; };
  void SetInstCounts(uint32_t _i) { instCounts = _i; };

  void computeBinaryOffsets();

  bool compactOneInstruction(G4_INST *);
  bool BDWcompactOneInstruction(G4_INST *);
  bool BDWcompactOneInstruction3Src(G4_INST *);
  bool CHVcompactOneInstruction3Src(G4_INST *);
  bool uncompactOneInstruction(G4_INST *);

  bool doCompaction() const;

protected:
  // returns the offset for label in # of half instructions (kernel entry is 0),
  // or -1 if the label is not present
  uint32_t GetLabelInfo(G4_Label *label) {
    auto iter = LabelMap.find(label);
    if (iter == LabelMap.end()) {
      return -1;
    }
    return iter->second;
  }

  Mem_Manager &mem;     ///< Reference to the memory manager
  std::string fileName; ///< Name of the binary file
  G4_Kernel &kernel;
  BinInstList binInstList; ///< Reference to the binary instructions
  std::map<G4_Label *, uint32_t> LabelMap;

  uint32_t instCounts;

  // Maps for Align16 operands' swizzle (source) and write mask (dst).
  std::map<G4_SrcRegRegion *, SrcSwizzle> align16SrcSwizzle;
  std::map<G4_DstRegRegion *, ChannelEnable> align16DstWriteMask;

  // Map for Align16 predicate control. If a predicate is not in the map it has
  // default control.
  std::unordered_map<G4_Predicate *, G4_Align16_Predicate_Control>
      align16PredCtrl;

  // Map an instruction to its binary encoding.
  std::unordered_map<G4_INST *, BinInst *> instToBinInstMap;

public:
  // all platform specific bit locations are initialized here
  static void InitPlatform() {
    // BDW+ encoding
    SET_BIT_RANGE(bitsFlagRegNum, 33, 33);
    SET_BIT_RANGE(bits3SrcFlagRegNum, 33, 33);
    SET_BIT_RANGES(bitsSrcRegFile, 42, 41, 90, 89);
  }

  inline uint32_t GetSrc0RegFile(BinInst *mybin) {
    if (mybin->GetIs3Src())
      return REG_FILE_R;
    else
      return mybin->GetBits(bitsSrcRegFile[0], bitsSrcRegFile[1]);
  }

  inline uint32_t GetSrc1RegFile(BinInst *mybin) {
    if (mybin->GetIs3Src())
      return REG_FILE_R;
    else
      return mybin->GetBits(bitsSrcRegFile[2], bitsSrcRegFile[3]);
  }
  inline uint32_t GetFlagRegNum(BinInst *mybin) {
    if (mybin->GetIs3Src())
      return mybin->GetBits(bits3SrcFlagRegNum[0], bits3SrcFlagRegNum[1]);
    else
      return mybin->GetBits(bitsFlagRegNum[0], bitsFlagRegNum[1]);
  }
  inline void SetFlagRegNum(BinInst *mybin, uint32_t value) {
    if (mybin->GetIs3Src())
      mybin->SetBits(bits3SrcFlagRegNum[0], bits3SrcFlagRegNum[1], value);
    else
      mybin->SetBits(bitsFlagRegNum[0], bitsFlagRegNum[1], value);
  }

  void setSwizzle(G4_SrcRegRegion *src, SrcSwizzle swizzle) {
    align16SrcSwizzle[src] = swizzle;
  }
  void setWriteMask(G4_DstRegRegion *dst, ChannelEnable writeMask) {
    align16DstWriteMask[dst] = writeMask;
  }

  std::optional<SrcSwizzle> getSwizzle(G4_SrcRegRegion *src) {
    auto iter = align16SrcSwizzle.find(src);
    return iter == align16SrcSwizzle.end() ? std::nullopt
                                           : std::optional(iter->second);
  }
  ChannelEnable getWriteMask(G4_DstRegRegion *dst) {
    auto iter = align16DstWriteMask.find(dst);
    return iter == align16DstWriteMask.end() ? NoChannelEnable : iter->second;
  }

  void setAlign16PredCtrl(G4_Predicate *pred,
                          G4_Align16_Predicate_Control ctrl) {
    align16PredCtrl[pred] = ctrl;
  }
  G4_Align16_Predicate_Control getAlign16PredCtrl(G4_Predicate *pred) {
    auto iter = align16PredCtrl.find(pred);
    return iter == align16PredCtrl.end() ? PRED_ALIGN16_DEFAULT : iter->second;
  }

  void setBinInst(G4_INST *inst, BinInst *binInst) {
    instToBinInstMap[inst] = binInst;
  }
  BinInst *getBinInst(G4_INST *inst) const {
    auto iter = instToBinInstMap.find(inst);
    return iter == instToBinInstMap.end() ? nullptr : iter->second;
  }

  // Should use G9HDL::EU_OPCODE as return type. But Forward declaration of enum
  // fails for linux, so use uint32_t as WA for now.
  uint32_t getEUOpcode(G4_opcode g4opc); // defined in BinaryEncodingCNL.cpp
};                                       // BinaryEncodingBase
} // namespace vISA

namespace vISA {
class EncodingHelper {
public:
  static inline RegFile GetDstRegFile(G4_DstRegRegion *dst);
  static inline RegFile GetSrcRegFile(G4_Operand *src);
  static inline uint32_t GetArchRegType(G4_VarBase *opnd);
  static inline uint32_t GetDstArchRegType(G4_DstRegRegion *opnd);
  static inline unsigned short GetElementSizeValue(G4_Operand *opnd);
  static inline AddrMode GetDstAddrMode(G4_DstRegRegion *dst);
  static inline AddrMode GetSrcAddrMode(G4_Operand *src);
  static inline void mark3Src(G4_INST *inst, BinaryEncodingBase &encoder);
  static inline bool hasLabelString(G4_INST *inst);

  static inline bool isSrcSubRegNumValid(G4_Operand *src) {
    bool valid = false;
    if (EncodingHelper::GetSrcRegFile(src) != REG_FILE_A ||
        EncodingHelper::GetSrcArchRegType(src) != ARCH_REG_FILE_NULL) {
      if (EncodingHelper::GetSrcAddrMode(src) == ADDR_MODE_IMMED) {
        if (!src->isSrcRegRegion() ||
            src->asSrcRegRegion()->getSubRegOff() != (short)UNDEFINED_SHORT) {
          valid = true;
        }
      }
    }

    return valid;
  }

  static inline uint32_t GetSrcArchRegType(G4_Operand *opnd) {
    if (opnd->isSrcRegRegion()) {
      G4_VarBase *base = opnd->asSrcRegRegion()->getBase();

      if (base->isRegVar()) {
        G4_VarBase *preg = base->asRegVar()->getPhyReg();
        return EncodingHelper::GetArchRegType(preg);
      } else {
        return EncodingHelper::GetArchRegType(base);
      }
    }

    return ARCH_REG_FILE_NULL;
  }
  static inline ChanSel GetSrcChannelSelectValue(G4_SrcRegRegion *srcRegion,
                                                 int i,
                                                 BinaryEncodingBase &encoder) {
    ChanSel ChanSelectValue = CHAN_SEL_UNDEF;

    auto getSrcSwizzleStr = [](SrcSwizzle sw) {
      switch (sw) {
      case SrcSwizzle::R:
        return "r";
      case SrcSwizzle::XYZW:
        return "xyzw";
      case SrcSwizzle::XYXY:
        return "xyxy";
      case SrcSwizzle::ZWZW:
        return "zwzw";
      }
      return "";
    };

    auto maybeSwizzle = encoder.getSwizzle(srcRegion);
    if (!maybeSwizzle)
      return ChanSelectValue;
    // Note that below is not legal for "r", caller should check.
    const char *swizzleStr = getSrcSwizzleStr(*maybeSwizzle);
    if (i < NUM_REGISTER_CHANNELS) {
      switch (swizzleStr[i]) {
      case 'x':
        ChanSelectValue = CHAN_SEL_X;
        break;
      case 'y':
        ChanSelectValue = CHAN_SEL_Y;
        break;
      case 'z':
        ChanSelectValue = CHAN_SEL_Z;
        break;
      case 'w':
        ChanSelectValue = CHAN_SEL_W;
        break;
      }
    }
    return ChanSelectValue;
  }

  static inline bool GetRepControl(G4_Operand *src,
                                   BinaryEncodingBase &encoder) {
    if (src->isSrcRegRegion()) {
      auto maybeSwizzle = encoder.getSwizzle(src->asSrcRegRegion());
      if (maybeSwizzle && *maybeSwizzle == SrcSwizzle::R)
        return true;
    }
    return false;
  }
};
} // namespace vISA

namespace vISA {
inline void BinaryEncodingBase::BuildLabelMap(G4_INST *inst,
                                              int &localHalfInstNum,
                                              int &localInstNum,
                                              int &globalHalfInstNum,
                                              int &globalInstNum) {
  if (inst->isLabel()) {
    this->LabelMap[inst->getLabel()] = globalHalfInstNum;
  } else {
    BinInst *bin = getBinInst(inst);

    localInstNum++;
    globalInstNum++;
    if (GetCompactCtrl(bin)) {
      localHalfInstNum += 1;
      globalHalfInstNum += 1;
    } else {
      localHalfInstNum += 2;
      globalHalfInstNum += 2;
    }
  }
}

/**
 * labels will be skipped in encoding stage and calculated in computeOffset()
 */
inline bool EncodingHelper::hasLabelString(G4_INST *inst) {
  G4_opcode op = inst->opcode();
  if (op == G4_label || op == G4_break || op == G4_cont || op == G4_halt ||
      op == G4_endif)
    return true;
  else if (op == G4_call && inst->getSrc(0) && inst->getSrc(0)->isLabel()) {
    return true;
  } else if (op == G4_jmpi && inst->getSrc(0) && inst->getSrc(0)->isLabel()) {
    return true;
  }

  return false;
}

//////////////////////////////////////////////////////////////////////////
inline void EncodingHelper::mark3Src(G4_INST *inst,
                                     BinaryEncodingBase &encoder) {
  BinInst *mybin = encoder.getBinInst(inst);

  if (inst->getNumSrc() == 3 && !inst->isSend()) {
    mybin->SetIs3Src(true);
  } else {
    mybin->SetIs3Src(false);
  }
}

/// Returns the register file of source operands.
inline RegFile EncodingHelper::GetSrcRegFile(G4_Operand *src) {
  if (src->isImm())
    return REG_FILE_I;

  G4_SrcRegRegion *srcRegRegion = src->asSrcRegRegion();

  if (srcRegRegion->isIndirect()) {
    return REG_FILE_R;
  }

  G4_VarBase *base = srcRegRegion->getBase();

  if (base->isRegVar()) {
    G4_VarBase *opnd = base->asRegVar()->getPhyReg();
    if (opnd->isAreg())
      return REG_FILE_A;
    else if (opnd->isGreg())
      return REG_FILE_R;
  } else {
    if (base->isAreg())
      return REG_FILE_A;
    else if (base->isGreg())
      return REG_FILE_R;
  }

  vISA_ASSERT_UNREACHABLE("invalid src regfile");
  return REG_FILE_R;
}

//////////////////////////////////////////////////////////////////////////
inline AddrMode EncodingHelper::GetDstAddrMode(G4_DstRegRegion *dst) {
  if (dst->getRegAccess() == Direct)
    return ADDR_MODE_IMMED;
  else
    return ADDR_MODE_INDIR;
}

//////////////////////////////////////////////////////////////////////////
inline AddrMode EncodingHelper::GetSrcAddrMode(G4_Operand *src) {
  if (src->isSrcRegRegion()) {
    if (src->asSrcRegRegion()->getRegAccess() == Direct)
      return ADDR_MODE_IMMED;
    else
      return ADDR_MODE_INDIR;
  }

  return ADDR_MODE_IMMED;
}

//////////////////////////////////////////////////////////////////////////
inline unsigned short EncodingHelper::GetElementSizeValue(G4_Operand *opnd) {
  unsigned short ElementSizeValue = 0;
  switch (opnd->getType()) {
  case Type_B:
  case Type_UB:
    ElementSizeValue = 1;
    break;
  case Type_UW:
  case Type_W:
  case Type_HF:
    ElementSizeValue = 2;
    break;
  case Type_UD:
  case Type_D:
  case Type_F:
    ElementSizeValue = 4;
    break;
  case Type_DF:
    ElementSizeValue = 8;
    break;
  case Type_Q:
  case Type_UQ:
    ElementSizeValue = 8;
    break;
  default: // error here
    break;
  }
  return ElementSizeValue;
}

//////////////////////////////////////////////////////////////////////////
inline uint32_t EncodingHelper::GetDstArchRegType(G4_DstRegRegion *opnd) {
  G4_VarBase *base = opnd->getBase();

  if (base->isRegVar()) {
    G4_VarBase *preg = base->asRegVar()->getPhyReg();
    return GetArchRegType(preg);
  } else {
    return GetArchRegType(base);
  }
}

//////////////////////////////////////////////////////////////////////////
inline uint32_t EncodingHelper::GetArchRegType(G4_VarBase *opnd) {
  uint32_t kind = ARCH_REG_FILE_DBG_REG;
  if (opnd->isAreg()) {
    switch (((G4_Areg *)opnd)->getArchRegType()) {
    case AREG_NULL:
      kind = ARCH_REG_FILE_NULL;
      break;
    case AREG_A0:
      kind = ARCH_REG_FILE_A;
      break;
    case AREG_ACC0:
    case AREG_ACC1:
      kind = ARCH_REG_FILE_ACC;
      break;
    case AREG_MASK0:
      kind = ARCH_REG_FILE_CE_REG;
      break;
    case AREG_MSG0:
      kind = ARCH_REG_FILE_MSG_REG;
      break;
    case AREG_DBG:
      kind = ARCH_REG_FILE_DBG_REG;
      break;
    case AREG_SR0:
      kind = ARCH_REG_FILE_STATE_REG;
      break;
    case AREG_CR0:
      kind = ARCH_REG_FILE_CNTL_REG;
      break;
    case AREG_TM0:
      kind = ARCH_REG_FILE_TM_REG;
      break;
    case AREG_N0:
    case AREG_N1:
      kind = ARCH_REG_FILE_NCNT_REG;
      break;
    case AREG_IP:
      kind = ARCH_REG_FILE_IP;
      break;
    case AREG_F0:
    case AREG_F1:
      kind = ARCH_REG_FILE_F;
      break;
    case AREG_TDR0:
      kind = ARCH_REG_FILE_TDR_REG;
      break;
    case AREG_SP:
      kind = ARCH_REG_FILE_SP_REG;
      break;
    default:
      kind = ARCH_REG_FILE_DBG_REG;
      // TODO: err message here except from send dst
    }
  }
  return kind;
}

//////////////////////////////////////////////////////////////////////////
inline RegFile EncodingHelper::GetDstRegFile(G4_DstRegRegion *dst) {

  if (dst->isIndirect()) {
    return REG_FILE_R;
  }

  G4_VarBase *base = dst->getBase();

  if (base->isRegVar()) {
    G4_VarBase *opnd = base->asRegVar()->getPhyReg();
    if (opnd->isAreg())
      return REG_FILE_A;
    else if (opnd->isGreg())
      return REG_FILE_R;
  } else {
    if (base->isAreg())
      return REG_FILE_A;
    else if (base->isGreg())
      return REG_FILE_R;
  }
  vISA_ASSERT_UNREACHABLE("invalid dst regfile");
  return REG_FILE_R;
}

//////////////////////////////////////////////////////////////////////////
inline uint32_t GetEncodeExecSize(G4_INST *inst) {
  unsigned char execSize;
  G4_opcode op = inst->opcode();

  if (op == G4_nop || op == G4_wait || op == G4_jmpi) {
    return ES_1_CHANNEL;
  }

  execSize = inst->getExecSize();
  uint32_t exSz = 0;
  switch (execSize) {
  case 1:
    exSz = ES_1_CHANNEL;
    break;
  case 2:
    exSz = ES_2_CHANNELS;
    break;
  case 4:
    exSz = ES_4_CHANNELS;
    break;
  case 8:
    exSz = ES_8_CHANNELS;
    break;
  case 16:
    exSz = ES_16_CHANNELS;
    break;
  case 32:
    exSz = ES_32_CHANNELS;
    break;
  default:
    vISA_ASSERT_UNREACHABLE("Invalid execution size: %d", (short)execSize);
  }
  return exSz;
}
} // namespace vISA
#define bitsSrcImm32_0 127
#define bitsSrcImm32_1 96
#define bitsSrcImm32_2 127
#define bitsSrcImm32_3 96
#define bitsDebugCtrl_0 30
#define bitsDebugCtrl_1 30
#define bitsAccWrCtrl_0 28
#define bitsAccWrCtrl_1 28
#define bitsCondModifier_0 27
#define bitsCondModifier_1 24
#define bits3SrcDstRegNumHWord_0 63
#define bits3SrcDstRegNumHWord_1 56
#define bitsDstRegNumHWord_0 60
#define bitsDstRegNumHWord_1 53
#define bits64DebugCtrl_0 7
#define bits64DebugCtrl_1 7
#define bits64ControlIndex_0 12
#define bits64ControlIndex_1 8
#define bits64DataTypeIndex_0 17
#define bits64DataTypeIndex_1 13
#define bits64SubRegIndex_0 22
#define bits64SubRegIndex_1 18

// CondModifier [27:24] same as 128 bit
#define bits64FlagSubRegNum_0 28
#define bits64FlagSubRegNum_1 28

// change from reserve to AccWrCtrl
#define bits64AccWrCtrl_0 23
#define bits64AccWrCtrl_1 23

// CompactCtrl [29:29] same as 128 bit
#define bits64Src0Index_0 34
#define bits64Src0Index_1 32
#define bits64Src0Index_2 31
#define bits64Src0Index_3 30
#define bits64Src1Index_0 39
#define bits64Src1Index_1 35
#define bits64DstRegNum_0 47
#define bits64DstRegNum_1 40
#define bits64Src0RegNum_0 55
#define bits64Src0RegNum_1 48
#define bits64Src1RegNum_0 63
#define bits64Src1RegNum_1 56
#define bits3SrcSrcRegNumHWord_0 83
#define bits3SrcSrcRegNumHWord_1 76
#define bits3SrcSrcRegNumHWord_2 104
#define bits3SrcSrcRegNumHWord_3 97
#define bitsSrcRegNumHWord_0 76
#define bitsSrcRegNumHWord_1 69
#define bitsSrcRegNumHWord_2 108
#define bitsSrcRegNumHWord_3 101

static const uint32_t UNDEFINED_VALUE = 0x00000000;

namespace vISA {
inline uint32_t GetSrc0Imm32(BinInst *mybin) {
  if (mybin->GetIs3Src())
    return UNDEFINED_VALUE;
  else
    return mybin->GetBits(bitsSrcImm32_0, bitsSrcImm32_1);
}

inline uint32_t GetSrc1Imm32(BinInst *mybin) {
  if (mybin->GetIs3Src())
    return UNDEFINED_VALUE;
  else
    return mybin->GetBits(bitsSrcImm32_2, bitsSrcImm32_3);
}

inline bool CompactableImmediate(uint32_t Immediate) {
  // Each word needs to be of the format:
  // Old rule, not valid 000 0xxxx yyyyyyyy or 111 1xxxx yyyyyyyy

  // new rule, the 32 bit has to be
  // zzzzzzzz zzzzzzzz zzz zxxxx yyyyyyyy; z could be either 0 or 1

  // Separate immediate into 2 words
  uint32_t Words[2];
  Words[0] = Immediate & 0xffff;
  Words[1] = (Immediate >> 16) & 0xffff;

  // Words must be identical
  // if (Words[0] != Words[1])
  //     return false;

  // Make sure upper 4 bits for word 0 and all bit for word 1 are identical
  // for (int word = 0; word < WORDS_PER_DWORD; word++)
  {
    int UpperFour = (Words[0] >> 12) & 0xf;
    if (!(((UpperFour == 0x0) && (Words[1] == 0x0)) ||
          ((UpperFour == 0xf) && (Words[1] == 0xffff))))
      return false;
  }

  // If it passes, compact able
  return true;
}

inline uint32_t GetDebugCtrl(BinInst *mybin) {
  return mybin->GetBits(bitsDebugCtrl_0, bitsDebugCtrl_1);
}

inline uint32_t GetAccWrCtrl(BinInst *mybin) {
  return mybin->GetBits(bitsAccWrCtrl_0, bitsAccWrCtrl_1);
} // GT

inline uint32_t GetCondModifier(BinInst *mybin) {
  return mybin->GetBits(bitsCondModifier_0, bitsCondModifier_1);
}

inline uint32_t GetDstRegNumHWord(BinInst *mybin) {
  if (mybin->GetIs3Src())
    return mybin->GetBits(bits3SrcDstRegNumHWord_0, bits3SrcDstRegNumHWord_1);
  else
    return mybin->GetBits(bitsDstRegNumHWord_0, bitsDstRegNumHWord_1);
}

inline uint32_t GetCmpDebugCtrl(BinInst *mybin) {
  return mybin->GetBits(bits64DebugCtrl_0, bits64DebugCtrl_1);
}

inline void SetCmpDebugCtrl(BinInst *mybin, uint32_t value) {
  mybin->SetBits(bits64DebugCtrl_0, bits64DebugCtrl_1, value);
}

inline void SetAccWrCtrl(BinInst *mybin, uint32_t value) {
  mybin->SetBits(bitsAccWrCtrl_0, bitsAccWrCtrl_1, value);
}

inline void SetDebugCtrl(BinInst *mybin, uint32_t value) {
  mybin->SetBits(bitsDebugCtrl_0, bitsDebugCtrl_1, value);
}

inline uint32_t GetCmpControlIndex(BinInst *mybin) {
  return mybin->GetBits(bits64ControlIndex_0, bits64ControlIndex_1);
}

inline void SetCmpControlIndex(BinInst *mybin, uint32_t value) {
  mybin->SetBits(bits64ControlIndex_0, bits64ControlIndex_1, value);
}

inline uint32_t GetCmpDataTypeIndex(BinInst *mybin) {
  return mybin->GetBits(bits64DataTypeIndex_0, bits64DataTypeIndex_1);
}

inline void SetCmpDataTypeIndex(BinInst *mybin, uint32_t value) {
  mybin->SetBits(bits64DataTypeIndex_0, bits64DataTypeIndex_1, value);
}

inline uint32_t GetCmpSubRegIndex(BinInst *mybin) {
  return mybin->GetBits(bits64SubRegIndex_0, bits64SubRegIndex_1);
}

inline void SetCmpSubRegIndex(BinInst *mybin, uint32_t value) {
  mybin->SetBits(bits64SubRegIndex_0, bits64SubRegIndex_1, value);
}

inline uint32_t GetCmpFlagSubRegNum(BinInst *mybin) {
  return mybin->GetBits(bits64FlagSubRegNum_0, bits64FlagSubRegNum_1);
}

inline void SetCmpFlagSubRegNum(BinInst *mybin, uint32_t value) {
  mybin->SetBits(bits64FlagSubRegNum_0, bits64FlagSubRegNum_1, value);
}

inline uint32_t GetCmpAccWrCtrl(BinInst *mybin) {
  return mybin->GetBits(bits64AccWrCtrl_0, bits64AccWrCtrl_1);
}

inline void SetCmpAccWrCtrl(BinInst *mybin, uint32_t value) {
  mybin->SetBits(bits64AccWrCtrl_0, bits64AccWrCtrl_1, value);
}

inline void SetCondModifier(BinInst *mybin, uint32_t value) {
  mybin->SetBits(bitsCondModifier_0, bitsCondModifier_1, value);
}

inline uint32_t GetCmpSrc0Index(BinInst *mybin) {
  // This one is ugly.  GetBits can't cross 32 bit boundary.

  // Get upper 3 bits...
  uint32_t upper3 = mybin->GetBits(bits64Src0Index_0, bits64Src0Index_1);
  // Lower 2 bits...
  uint32_t lower2 = mybin->GetBits(bits64Src0Index_2, bits64Src0Index_3);

  return (upper3 << 2) | lower2;
}

inline void SetCmpSrc0Index(BinInst *mybin, uint32_t value) {
  // This one is ugly.  SetBits can't cross 32 bit boundary.

  // Upper 3 bits...
  mybin->SetBits(bits64Src0Index_0, bits64Src0Index_1, value >> 2);
  // Lower 2 bits...
  mybin->SetBits(bits64Src0Index_2, bits64Src0Index_3, value);
}

inline uint32_t GetCmpSrc1Index(BinInst *mybin) {
  return mybin->GetBits(bits64Src1Index_0, bits64Src1Index_1);
}

inline void SetCmpSrc1Index(BinInst *mybin, uint32_t value) {
  mybin->SetBits(bits64Src1Index_0, bits64Src1Index_1, value);
}

inline uint32_t GetCmpDstRegNum(BinInst *mybin) {
  return mybin->GetBits(bits64DstRegNum_0, bits64DstRegNum_1);
}

inline void SetCmpDstRegNum(BinInst *mybin, uint32_t value) {
  mybin->SetBits(bits64DstRegNum_0, bits64DstRegNum_1, value);
}

inline uint32_t GetCmpSrc1RegNum(BinInst *mybin) {
  return mybin->GetBits(bits64Src1RegNum_0, bits64Src1RegNum_1);
}

inline void SetCmpSrc1RegNum(BinInst *mybin, uint32_t value) {
  mybin->SetBits(bits64Src1RegNum_0, bits64Src1RegNum_1, value);
}

inline uint32_t GetCmpSrc0RegNum(BinInst *mybin) {
  return mybin->GetBits(bits64Src0RegNum_0, bits64Src0RegNum_1);
}

inline void SetCmpSrc0RegNum(BinInst *mybin, uint32_t value) {
  mybin->SetBits(bits64Src0RegNum_0, bits64Src0RegNum_1, value);
}

inline uint32_t GetSrc0RegNumHWord(BinInst *mybin) {
  if (mybin->GetIs3Src())
    return mybin->GetBits(bits3SrcSrcRegNumHWord_0, bits3SrcSrcRegNumHWord_1);
  else
    return mybin->GetBits(bitsSrcRegNumHWord_0, bitsSrcRegNumHWord_1);
}

inline uint32_t GetSrc1RegNumHWord(BinInst *mybin) {
  if (mybin->GetIs3Src())
    return mybin->GetBits(bits3SrcSrcRegNumHWord_2, bits3SrcSrcRegNumHWord_3);
  else
    return mybin->GetBits(bitsSrcRegNumHWord_2, bitsSrcRegNumHWord_3);
}

inline void SetDstRegNumHWord(BinInst *mybin, uint32_t value) {
  if (mybin->GetIs3Src())
    mybin->SetBits(bits3SrcDstRegNumHWord_0, bits3SrcDstRegNumHWord_1, value);
  else
    mybin->SetBits(bitsDstRegNumHWord_0, bitsDstRegNumHWord_1, value);
}

inline void SetSrc0RegNumHWord(BinInst *mybin, uint32_t value) {
  if (mybin->GetIs3Src())
    mybin->SetBits(bits3SrcSrcRegNumHWord_0, bits3SrcSrcRegNumHWord_1, value);
  else
    mybin->SetBits(bitsSrcRegNumHWord_0, bitsSrcRegNumHWord_1, value);
}

inline void SetSrc1RegNumHWord(BinInst *mybin, uint32_t value) {
  if (mybin->GetIs3Src())
    mybin->SetBits(bits3SrcSrcRegNumHWord_2, bits3SrcSrcRegNumHWord_3, value);
  else
    mybin->SetBits(bitsSrcRegNumHWord_2, bitsSrcRegNumHWord_3, value);
}

inline Align1PredCtrl GetAlign1PredCtrl(G4_Predicate_Control ctrl) {
  Align1PredCtrl pCntrl = Align1PredCtrl::NONE;

  switch (ctrl) {
  case PRED_DEFAULT:
    pCntrl = Align1PredCtrl::SEQUENTIAL;
    break;
  case PRED_ANY2H:
    pCntrl = Align1PredCtrl::ANY2H;
    break;
  case PRED_ANY4H:
    pCntrl = Align1PredCtrl::ANY4H;
    break;
  case PRED_ANY8H:
    pCntrl = Align1PredCtrl::ANY8H;
    break;
  case PRED_ANY16H:
    pCntrl = Align1PredCtrl::ANY16H;
    break;
  case PRED_ANY32H:
    pCntrl = Align1PredCtrl::ANY32H;
    break;
  case PRED_ALL2H:
    pCntrl = Align1PredCtrl::ALL2H;
    break;
  case PRED_ALL4H:
    pCntrl = Align1PredCtrl::ALL4H;
    break;
  case PRED_ALL8H:
    pCntrl = Align1PredCtrl::ALL8H;
    break;
  case PRED_ALL16H:
    pCntrl = Align1PredCtrl::ALL16H;
    break;
  case PRED_ALL32H:
    pCntrl = Align1PredCtrl::ALL32H;
    break;
  case PRED_ANYV:
    pCntrl = Align1PredCtrl::ANYV;
    break;
  case PRED_ALLV:
    pCntrl = Align1PredCtrl::ALLV;
    break;
  default:
    break;
  }
  return pCntrl;
}

} // namespace vISA
static struct _CompactControlTable_ {
  union Data {
    struct {
      uint32_t Bits_023_008 : 16;
      uint32_t Bits_031_031 : 1;
      uint32_t Bits_090_089 : 2;
      uint32_t Reserved : 13;
    } sData;
    uint32_t ulData;
  };

  uint32_t GetBit_031(uint32_t index) {
    vISA_ASSERT(index < COMPACT_TABLE_SIZE,
                 "Out of Control Bit Compact Table range.");
    Data data;
    data.ulData = Values[index];
    return data.sData.Bits_031_031;
  }

  uint32_t GetBits_023_008(uint32_t index) {
    vISA_ASSERT(index < COMPACT_TABLE_SIZE,
                 "Out of Control Bit Compact Table range.");
    Data data;
    data.ulData = Values[index];
    return data.sData.Bits_023_008;
  }

  uint32_t GetBits_090_089(uint32_t index) {
    vISA_ASSERT(index < COMPACT_TABLE_SIZE,
                 "Out of Control Bit Compact Table range.");
    Data data;
    data.ulData = Values[index];
    return data.sData.Bits_090_089;
  }

  uint32_t Values[COMPACT_TABLE_SIZE];
} CompactControlTable;

namespace vISA {
inline bool BinaryEncodingBase::compactOneInstruction(G4_INST *inst) {
  G4_opcode op = inst->opcode();
  BinInst *mybin = getBinInst(inst);
  if (op == G4_if || op == G4_else || op == G4_endif || op == G4_while ||
      op == G4_halt || op == G4_break || op == G4_cont ||
      /* GetComprCtrl(mybin) == COMPR_CTRL_COMPRESSED  || */
      mybin->GetDontCompactFlag()) {
    // do not compact conditional branches
    return false;
  }

  if (op == G4_nop) {
    return false;
  }

  // temporary WA, to be removed later
  // we disable compacting nop/return
  // until it is clear that we can compact them
  if (op == G4_call || op == G4_return) {
    return false;
  }

  bool result = BDWcompactOneInstruction(inst);

  return result;
}

inline bool BinaryEncodingBase::BDWcompactOneInstruction3Src(G4_INST *inst) {
  BinInst *mybin = getBinInst(inst);

  // Check control table...
  uint32_t controlIndex;
  uint32_t bits_034_032 = mybin->GetBits(34, 32);
  uint32_t bits_028_008 = mybin->GetBits(28, 8);
  [[maybe_unused]] bool mustCompact = !(mybin->GetMustCompactFlag());
  if (!CompactControlTable3Src.FindBDWIndex(controlIndex, bits_034_032,
                                            bits_028_008)) {
    vISA_ASSERT(mustCompact, "Compaction failure for control table");
    // Can't compact...
    return false;
  }

  // Check source index table...
  uint32_t srcIndex;
  uint32_t bits_125_125 = mybin->GetBits(125, 125);
  uint32_t bits_104_104 = mybin->GetBits(104, 104);
  uint32_t bits_083_083 = mybin->GetBits(83, 83);
  uint32_t bits_114_107 = mybin->GetBits(114, 107);
  uint32_t bits_093_086 = mybin->GetBits(93, 86);
  uint32_t bits_072_065 = mybin->GetBits(72, 65);
  uint32_t bits_055_037 = mybin->GetBits(55, 37);

  if (!CompactSourceTable3Src.FindIndex(
          srcIndex, bits_125_125, bits_104_104, bits_083_083, bits_114_107,
          bits_093_086, bits_072_065, bits_055_037)) {
    vISA_ASSERT(mustCompact, "Compaction failure for source table");
    // Can't compact...
    return false;
  }

  // We have valid indices at this point.  Make a compacted instruction...
  // The field of opcode 6:0 and reserved 7 remain the same

  mybin->SetBits(9, 8, controlIndex);
  mybin->SetBits(11, 10, srcIndex);
  mybin->SetBits(18, 12, mybin->GetBits(63, 56));
  mybin->SetBits(27, 19, 0); // 27 to 19: reverved
  mybin->SetBits(28, 28, mybin->GetBits(64, 64));
  SetCompactCtrl(mybin, 1); // 29
  // 30, 31: the same
  mybin->SetBits(32, 32, mybin->GetBits(85, 85));
  mybin->SetBits(33, 33, mybin->GetBits(106, 106));
  mybin->SetBits(36, 34, mybin->GetBits(75, 73));
  mybin->SetBits(39, 37, mybin->GetBits(96, 94));
  mybin->SetBits(42, 40, mybin->GetBits(117, 115));
  mybin->SetBits(49, 43, mybin->GetBits(82, 76));
  mybin->SetBits(56, 50, mybin->GetBits(103, 97));
  mybin->SetBits(63, 57, mybin->GetBits(124, 118));

  // SetMustCompact(true);

  // Copy on top of ourselves...
  return true;
}

inline bool BinaryEncodingBase::CHVcompactOneInstruction3Src(G4_INST *inst) {
  BinInst *mybin = getBinInst(inst);

  // Check control table...
  uint32_t controlIndex;
  uint32_t bits_036_035 = mybin->GetBits(36, 35);
  uint32_t bits_034_032 = mybin->GetBits(34, 32);
  uint32_t bits_028_008 = mybin->GetBits(28, 8);
  [[maybe_unused]] bool mustCompact = !(mybin->GetMustCompactFlag());
  if (!CompactControlTable3Src.FindCHVIndex(controlIndex, bits_036_035,
                                            bits_034_032, bits_028_008)) {
    vISA_ASSERT(mustCompact, "Compaction failure for control table");
    // Can't compact...
    return false;
  }

  // Check source index table...
  uint32_t srcIndex;
  uint32_t bits_126_125 = mybin->GetBits(126, 125);
  uint32_t bits_105_104 = mybin->GetBits(105, 104);
  uint32_t bits_084_083 = mybin->GetBits(84, 83);
  uint32_t bits_114_107 = mybin->GetBits(114, 107);
  uint32_t bits_093_086 = mybin->GetBits(93, 86);
  uint32_t bits_072_065 = mybin->GetBits(72, 65);
  uint32_t bits_055_037 = mybin->GetBits(55, 37);

  if (!CompactSourceTable3SrcCHV.FindIndex(
          srcIndex, bits_126_125, bits_105_104, bits_084_083, bits_114_107,
          bits_093_086, bits_072_065, bits_055_037)) {
    vISA_ASSERT(mustCompact, "Compaction failure for source table");
    // Can't compact...
    return false;
  }

  // We have valid indices at this point.  Make a compacted instruction...
  // The field of opcode 6:0 and reserved 7 remain the same

  mybin->SetBits(9, 8, controlIndex);
  mybin->SetBits(11, 10, srcIndex);
  mybin->SetBits(18, 12, mybin->GetBits(63, 56));
  mybin->SetBits(27, 19, 0); // 27 to 19: reverved
  mybin->SetBits(28, 28, mybin->GetBits(64, 64));
  SetCompactCtrl(mybin, 1); // 29
  // 30, 31: the same
  mybin->SetBits(32, 32, mybin->GetBits(85, 85));
  mybin->SetBits(33, 33, mybin->GetBits(106, 106));
  mybin->SetBits(36, 34, mybin->GetBits(75, 73));
  mybin->SetBits(39, 37, mybin->GetBits(96, 94));
  mybin->SetBits(42, 40, mybin->GetBits(117, 115));
  mybin->SetBits(49, 43, mybin->GetBits(82, 76));
  mybin->SetBits(56, 50, mybin->GetBits(103, 97));
  mybin->SetBits(63, 57, mybin->GetBits(124, 118));

  return true;
}

inline bool BinaryEncodingBase::BDWcompactOneInstruction(G4_INST *inst) {
  BinInst *mybin = getBinInst(inst);

  if (mybin->GetIs3Src()) {
    if (inst->getPlatform() == GENX_BDW) {
      return BDWcompactOneInstruction3Src(inst);
    } else if (inst->getPlatform() >= GENX_CHV) {
      // CHV and SKL are using the same compaction table for 3src
      return CHVcompactOneInstruction3Src(inst);
    } else {
      // other platforms not handled yet
      return false;
    }
  }

  if (inst->getPlatform() >= GENX_CHV && inst->isSend()) {
    return false;
  }

  bool source_immediate[2];
  source_immediate[0] = (RegFile(GetSrc0RegFile(mybin)) == REG_FILE_I);
  source_immediate[1] = (RegFile(GetSrc1RegFile(mybin)) == REG_FILE_I);

  // Check control table...
  uint32_t controlIndex;
  uint32_t bits_033_032 = mybin->GetBits(33, 32);
  uint32_t bits_031_031 = mybin->GetBits(31, 31);
  uint32_t bits_023_012 = mybin->GetBits(23, 12);
  uint32_t bits_010_009 = mybin->GetBits(10, 9);
  uint32_t bits_034_034 = mybin->GetBits(34, 34);
  uint32_t bits_008_008 = mybin->GetBits(8, 8);
  [[maybe_unused]] bool mustCompact = !(mybin->GetMustCompactFlag());
  if (!BDWCompactControlTable.FindIndex(
          controlIndex, bits_033_032, bits_031_031, bits_023_012, bits_010_009,
          bits_034_034, bits_008_008)) {
    vISA_ASSERT(mustCompact, "Compaction failure for control table");
    // Can't compact...
    return false;
  }

  // Check data type table
  uint32_t dataTypeIndex;
  uint32_t bits_063_061 = mybin->GetBits(63, 61);
  uint32_t bits_094_089 = mybin->GetBits(94, 89);
  uint32_t bits_046_035 = mybin->GetBits(46, 35);
  if (!BDWCompactDataTypeTableStr.FindIndex(dataTypeIndex, bits_063_061,
                                            bits_094_089, bits_046_035)) {
    vISA_ASSERT(mustCompact, "Compaction failure for data type table");
    // Can't compact...
    return false;
  }

  // Check sub-register table...
  uint32_t subRegIndex;
  uint32_t bits_100_096 = mybin->GetBits(100, 96);
  uint32_t bits_068_064 = mybin->GetBits(68, 64);
  uint32_t bits_052_048 = mybin->GetBits(52, 48);

  // If source 0 is an immediate, we only check destination
  // sub-register info for compaction restrictions.
  if (source_immediate[0]) {
    if (!BDWCompactSubRegTable.FindIndex1(subRegIndex, bits_052_048)) {
      vISA_ASSERT(mustCompact, "Compaction failure for sub reg table");
      // Can't compact...
      return false;
    }
  }
  // If source 1 is an immediate, we need to check sub-register info
  // for source 0 in addition to sub-register info for destination
  else if (source_immediate[1]) {
    if (!BDWCompactSubRegTable.FindIndex2(subRegIndex, bits_068_064,
                                          bits_052_048)) {
      vISA_ASSERT(mustCompact, "Compaction failure for sub reg table");
      // Can't compact...
      return false;
    }
  }
  // Otherwise we check everything
  else if (!BDWCompactSubRegTable.FindIndex(subRegIndex, bits_100_096,
                                            bits_068_064, bits_052_048)) {
    vISA_ASSERT(mustCompact, "Compaction failure for sub reg table");
    // Can't compact...
    return false;
  }

  // Check source 0 table...
  uint32_t src0Index;
  uint32_t bits_088_077 = mybin->GetBits(88, 77);

  // If source 0 is not immediate data, we need to check source 0 info
  if (!source_immediate[0]) {
    if (!BDWCompactSourceTable.FindIndex(src0Index, bits_088_077)) {
      vISA_ASSERT(mustCompact, "Compaction failure for source table");
      // Can't compact...
      return false;
    }
  } else {
    // Default to index0 of tables
    src0Index = 0;
  }

  // Check source 1 table...
  uint32_t src1Index;
  uint32_t bits_120_109 = mybin->GetBits(120, 109);

  // If both source 0 and source 1 are not immediate data,
  // we need to cehck source 1 information
  if (!source_immediate[0] && !source_immediate[1]) {
    if (!BDWCompactSourceTable.FindIndex(src1Index, bits_120_109)) {
      vISA_ASSERT(mustCompact, "Compact should be set to false");
      // Can't compact...
      return false;
    }
  } else {
    src1Index = mybin->GetBits(127, 104);
  }

  uint32_t immediateData = 0;
  // If we have an immediate, overwrite bits [39:35] [63:56] with immediate
  if (source_immediate[0] || source_immediate[1]) {
    if (source_immediate[0]) {
      immediateData = GetSrc0Imm32(mybin);
    } else {
      immediateData = GetSrc1Imm32(mybin);
    }

    if (!CompactableImmediate(immediateData)) {
      vISA_ASSERT(mustCompact, "Compact should be set to false");
      // Can't compact...
      return false;
    }
  }

  // We have valid indices at this point.  Make a compacted instruction...
  // The field of opcode/debugCtrl, Bits 6:0 and 7 remain the same

  uint32_t accWrCtrl = GetAccWrCtrl(mybin);
  uint32_t dstRegNum = GetDstRegNumHWord(mybin);
  uint32_t src0RegNum = GetSrc0RegNumHWord(mybin);
  uint32_t src1RegNum = GetSrc1RegNumHWord(mybin);
  uint32_t debugCtrl = GetDebugCtrl(mybin);

  SetCmpDebugCtrl(mybin, debugCtrl);
  SetCmpControlIndex(mybin, controlIndex);   // 12:8
  SetCmpDataTypeIndex(mybin, dataTypeIndex); // 17:13
  SetCmpSubRegIndex(mybin, subRegIndex);     // 22:18
  SetCmpAccWrCtrl(mybin, accWrCtrl);         // 23
  // 27:24 cond modifier remain the same;
  mybin->SetBits(28, 28, 0);         // 28 reserved
  SetCompactCtrl(mybin, 1);          // 29
  SetCmpSrc0Index(mybin, src0Index); // 34:30
  SetCmpSrc1Index(mybin, src1Index); // 39:35
  SetCmpDstRegNum(mybin, dstRegNum); // 47:40
  if (source_immediate[0])
    SetCmpSrc0RegNum(mybin, 0);
  else
    SetCmpSrc0RegNum(mybin, src0RegNum); // 55:48
  if (source_immediate[1])
    SetCmpSrc1RegNum(mybin, 0);
  else
    SetCmpSrc1RegNum(mybin, src1RegNum); // 63:56

  // If we have an immediate, overwrite bits [39:35] [63:56] with immediate
  if (source_immediate[0] || source_immediate[1]) {
    SetCmpSrc1RegNum(mybin, immediateData & 0xff); // 63:56
    // 39:35 change from (immediateData & 0x1f)>>8
    // to (immediateData >> 8)& 0x1f)
    SetCmpSrc1Index(mybin, (immediateData >> 8) & 0x1f);
  }

  // SetMustCompact(true);

  // Copy on top of ourselves...
  return true;
}

inline bool BinaryEncodingBase::uncompactOneInstruction(G4_INST *inst) {
  BinInst *mybin = getBinInst(inst);
  // Validate control index...
  unsigned long controlIndex = GetCmpControlIndex(mybin);
  if (COMPACT_TABLE_SIZE <= controlIndex) {
    return false;
  }

  // Validate data type index...
  uint32_t dataTypeIndex = GetCmpDataTypeIndex(mybin);
  if (COMPACT_TABLE_SIZE <= dataTypeIndex) {
    return false;
  }

  // Validate sub-register index...
  uint32_t subRegIndex = GetCmpSubRegIndex(mybin);
  if (COMPACT_TABLE_SIZE <= subRegIndex) {
    return false;
  }

  // Validate source 0 index...
  uint32_t src0Index = GetCmpSrc0Index(mybin);
  if (COMPACT_TABLE_SIZE <= src0Index) {
    return false;
  }

  // Validate source 1 index...
  uint32_t src1Index = GetCmpSrc1Index(mybin);
  if (COMPACT_TABLE_SIZE <= src1Index) {
    return false;
  }

  // Pull out compatced immediate source
  // Get bits [39] [39] [39] [39:35]
  uint32_t src1IndexSignExtend = GetCmpSrc1Index(mybin);
  if ((src1IndexSignExtend & 0x10) != 0) {
    src1IndexSignExtend |= 0xe0;
  }
  // Get bits [63:56]
  uint32_t src1RegNum = GetCmpSrc1RegNum(mybin);
  // Put together [39] [39] [39] [39:35] [63:56] [39] [39] [39] [39:35] [63:56]
  uint32_t immediateSource = 0;
  immediateSource =
      (src1RegNum |
       (src1IndexSignExtend
        << 8)); // | (src1RegNum << 16) | (src1IndexSignExtend << 24));
  // do the sign extension upto 32 bit
  if ((immediateSource & 0x8000) == 0x8000) // if the sign is 1
    immediateSource = immediateSource | 0xffff0000;

  // Start clear...
  // Uncompact...
  // ins.SetOpCode(GetOpCode());

  uint32_t comDebugCtrl = GetCmpDebugCtrl(mybin);
  uint32_t controlIndex31 = CompactControlTable.GetBit_031(controlIndex);
  uint32_t controlIndex23 = CompactControlTable.GetBits_023_008(controlIndex);
  uint32_t dataTypeIndex63 =
      CompactDataTypeTable.GetBits_063_061(dataTypeIndex);
  uint32_t dataTypeIndex46 =
      CompactDataTypeTable.GetBits_046_032(dataTypeIndex);
  uint32_t subRegIndex100 = CompactSubRegTable.GetBits_100_096(subRegIndex);
  uint32_t subRegIndex68 = CompactSubRegTable.GetBits_068_064(subRegIndex);
  uint32_t subRegIndex52 = CompactSubRegTable.GetBits_052_048(subRegIndex);
  uint32_t condModifier = GetCondModifier(mybin);
  uint32_t accWrCtrl = GetCmpAccWrCtrl(mybin);
  uint32_t flagSubRegNum = GetCmpFlagSubRegNum(mybin);
  uint32_t bits88 = CompactSourceTable.GetBits_088_077(src0Index);
  uint32_t bits120 = CompactSourceTable.GetBits_120_109(src1Index);
  uint32_t dstRegNum = GetCmpDstRegNum(mybin);
  uint32_t src0RegNum = GetCmpSrc0RegNum(mybin);
  uint32_t src0RegFile = GetSrc0RegFile(mybin);
  uint32_t src1RegFile = GetSrc1RegFile(mybin);

  SetDebugCtrl(mybin, comDebugCtrl);
  mybin->SetBits(31, 31, controlIndex31);
  mybin->SetBits(23, 8, controlIndex23);
  mybin->SetBits(63, 61, dataTypeIndex63);
  mybin->SetBits(46, 32, dataTypeIndex46);
  mybin->SetBits(100, 96, subRegIndex100);
  mybin->SetBits(68, 64, subRegIndex68);
  mybin->SetBits(52, 48, subRegIndex52);
  SetCondModifier(mybin, condModifier);
  SetAccWrCtrl(mybin, accWrCtrl);
  SetFlagRegNum(mybin, flagSubRegNum);
  SetCompactCtrl(mybin, 0); // uncompaction
  mybin->SetBits(88, 77, bits88);
  //  mybin->SetBits(100, 96, CompactSubRegTable.GetBits_100_096(subRegIndex));
  mybin->SetBits(120, 109, bits120);
  SetDstRegNumHWord(mybin, dstRegNum);
  SetSrc0RegNumHWord(mybin, src0RegNum);
  SetSrc1RegNumHWord(mybin, src1RegNum);

  // set all MBZ bits in DW0 & DW1 to 0
  mybin->SetBits(7, 7, 0);

  // If either source is immediate, fill in w/ Source Immediate above
  if ((RegFile(src0RegFile) == REG_FILE_I) ||
      (RegFile(src1RegFile) == REG_FILE_I)) {
    mybin->SetBits(127, 96, immediateSource);
  }

  return true;
}
} // namespace vISA
#endif
