/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

// clang-format off
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "common/LLVMWarningsPop.hpp"
// clang-format on

#include "Probe/Assertion.h"

#include <algorithm>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace IGC {
template <typename T> T read(const void *&dbg) {
  static_assert(std::is_standard_layout<T>::value);

  T data = 0;
  const char *SrcP = (const char *)dbg;
  const char *EndP = SrcP + sizeof(T);
  char *DstP = (char *)&data;
  std::copy(SrcP, EndP, DstP);
  dbg = EndP;
  return data;
}

// Decode Gen debug info data structure
class DbgDecoder {
public:
  class Mapping {
  public:
    class Register {
    public:
      uint16_t regNum;
      uint16_t subRegNum; // for GRF, in byte offset

      bool operator==(const Register &rhs) const {
        return (regNum == rhs.regNum && subRegNum == rhs.subRegNum);
      }

      void print(llvm::raw_ostream &OS) const;
      void dump() const;
    };
    class Memory {
    public:
      uint32_t isBaseOffBEFP : 1; // MSB of 32-bit field denotes whether base if
                                  // off BE_FP (0) or absolute (1)
      int32_t memoryOffset : 31;  // memory offset

      bool operator==(const Memory &rhs) const {
        return (isBaseOffBEFP == rhs.isBaseOffBEFP &&
                memoryOffset == rhs.memoryOffset);
      }

      void print(llvm::raw_ostream &OS) const;
      void dump() const;
    };
    union {
      Register r;
      Memory m;
    };
  };
  class VarAlloc {
  public:
    enum VirtualVarType { VirTypeAddress = 0, VirTypeFlag = 1, VirTypeGRF = 2 };
    enum PhysicalVarType {
      PhyTypeAddress = 0,
      PhyTypeFlag = 1,
      PhyTypeGRF = 2,
      PhyTypeMemory = 3
    };
    VirtualVarType virtualType;
    PhysicalVarType physicalType;
    Mapping mapping;

    void print(llvm::raw_ostream &OS) const;
    void dump() const;
  };
  class LiveIntervalsVISA {
  public:
    uint16_t start = 0;
    uint16_t end = 0;
    VarAlloc var;

    bool isGRF() const {
      if (var.physicalType == DbgDecoder::VarAlloc::PhysicalVarType::PhyTypeGRF)
        return true;

      return false;
    }

    bool isSpill() const {
      if (var.physicalType ==
          DbgDecoder::VarAlloc::PhysicalVarType::PhyTypeMemory)
        return true;

      return false;
    }

    bool isSpillOnStack() const {
      if (!isSpill())
        return false;

      if (var.mapping.m.isBaseOffBEFP == 0)
        return true;

      return false;
    }

    Mapping::Register getGRF() const { return var.mapping.r; }

    Mapping::Memory getSpillOffset() const { return var.mapping.m; }

    void print(llvm::raw_ostream &OS) const;
    void dump() const;
  };
  class LiveIntervalGenISA {
  public:
    uint32_t start = 0;
    uint32_t end = 0;
    VarAlloc var;

    void print(llvm::raw_ostream &OS) const;
    void dump() const;
  };

  class VarInfo {
  public:
    std::string name;
    std::vector<LiveIntervalsVISA> lrs;

    void print(llvm::raw_ostream &OS) const;
    void dump() const;
  };
  class SubroutineInfo {
  public:
    std::string name;
    uint32_t startVISAIndex;
    uint32_t endVISAIndex;
    std::vector<LiveIntervalsVISA> retval;

    void print(llvm::raw_ostream &OS) const;
    void dump() const;
  };
  class RegInfoMapping {
  public:
    uint16_t srcRegOff = 0;
    uint16_t numBytes = 0;
    bool dstInReg = false;
    Mapping dst;

    void print(llvm::raw_ostream &OS) const;
    void dump() const;
  };
  class PhyRegSaveInfoPerIP {
  public:
    uint32_t genIPOffset = 0;
    uint16_t numEntries = 0;
    std::vector<RegInfoMapping> data;

    void print(llvm::raw_ostream &OS) const;
    void dump() const;
  };
  class CallFrameInfo {
  public:
    uint16_t frameSize = 0;
    bool befpValid = false;
    std::vector<LiveIntervalGenISA> befp;
    bool callerbefpValid = false;
    std::vector<LiveIntervalGenISA> callerbefp;
    bool retAddrValid = false;
    std::vector<LiveIntervalGenISA> retAddr;
    uint16_t CEOffsetFromFPOff;
    uint16_t CEStoreIP;
    uint16_t numCalleeSaveEntries = 0;
    std::vector<PhyRegSaveInfoPerIP> calleeSaveEntry;
    uint32_t numCallerSaveEntries = 0;
    std::vector<PhyRegSaveInfoPerIP> callerSaveEntry;

    bool getBEFPRegNum(uint32_t &regNum, uint32_t &subRegNum) const {
      if (befp.size() == 0)
        return false;

      // Assume that be_fp is available throughout the function
      // and at the same location.
      IGC_ASSERT_MESSAGE(befp.front().var.physicalType == VarAlloc::PhyTypeGRF,
                         "BE_FP not in register");

      regNum = befp.front().var.mapping.r.regNum;
      subRegNum = befp.front().var.mapping.r.subRegNum;

      return true;
    }

    void print(llvm::raw_ostream &OS) const;
    void dump() const;
  };
  class DbgInfoFormat {
  public:
    std::string kernelName;
    uint32_t relocOffset = 0;
    std::vector<std::pair<unsigned int, unsigned int>> CISAOffsetMap;
    std::vector<std::pair<unsigned int, unsigned int>> CISAIndexMap;
    std::vector<VarInfo> Vars;

    std::vector<SubroutineInfo> subs;
    CallFrameInfo cfi;

    void print(llvm::raw_ostream &OS) const;
    void dump() const;
  };

  std::vector<DbgInfoFormat> compiledObjs;

private:
  void readMappingReg(DbgDecoder::Mapping &mapping) {
    mapping.r.regNum = read<uint16_t>(dbg);
    mapping.r.subRegNum = read<uint16_t>(dbg);
  }

  void readMappingMem(DbgDecoder::Mapping &mapping) {
    uint32_t temp = read<uint32_t>(dbg);
    mapping.m.memoryOffset = (temp & 0x7fffffff);
    mapping.m.isBaseOffBEFP = (temp & 0x80000000);
  }

  LiveIntervalsVISA readLiveIntervalsVISA() {
    DbgDecoder::LiveIntervalsVISA lv;
    lv.start = read<uint16_t>(dbg);
    lv.end = read<uint16_t>(dbg);
    lv.var = readVarAlloc();
    return lv;
  }

  LiveIntervalGenISA readLiveIntervalGenISA() {
    DbgDecoder::LiveIntervalGenISA lr;
    lr.start = read<uint32_t>(dbg);
    lr.end = read<uint32_t>(dbg);
    lr.var = readVarAlloc();
    return lr;
  }

  RegInfoMapping readRegInfoMapping() {
    DbgDecoder::RegInfoMapping info;
    info.srcRegOff = read<uint16_t>(dbg);
    info.numBytes = read<uint16_t>(dbg);
    info.dstInReg = (bool)read<uint8_t>(dbg);
    if (info.dstInReg) {
      readMappingReg(info.dst);
    } else {
      readMappingMem(info.dst);
    }
    return info;
  }

  VarAlloc readVarAlloc() {
    DbgDecoder::VarAlloc data;

    data.virtualType = (DbgDecoder::VarAlloc::VirtualVarType)read<uint8_t>(dbg);
    data.physicalType =
        (DbgDecoder::VarAlloc::PhysicalVarType)read<uint8_t>(dbg);

    enum class PhyType : unsigned { Address = 0, Flag = 1, GRF = 2, Mem = 3 };
    if (data.physicalType == (unsigned)PhyType::Address ||
        data.physicalType == (unsigned)PhyType::Flag ||
        data.physicalType == (unsigned)PhyType::GRF) {
      readMappingReg(data.mapping);
    } else if (data.physicalType == (unsigned)PhyType::Mem) {
      readMappingMem(data.mapping);
    }
    return data;
  }

  const void *dbg;
  uint16_t numCompiledObj = 0;
  uint32_t magic = 0;

  void decode() {
    magic = read<uint32_t>(dbg);
    numCompiledObj = read<uint16_t>(dbg);

    for (unsigned int i = 0; i != numCompiledObj; i++) {
      DbgInfoFormat f;
      uint16_t nameLen = read<uint16_t>(dbg);
      for (unsigned int j = 0; j != nameLen; j++)
        f.kernelName += read<char>(dbg);
      f.relocOffset = read<uint32_t>(dbg);

      // cisa offsets map
      uint32_t count = read<uint32_t>(dbg);
      for (unsigned int j = 0; j != count; j++) {
        uint32_t cisaOffset = read<uint32_t>(dbg);
        uint32_t genOffset = read<uint32_t>(dbg);
        f.CISAOffsetMap.push_back(
            std::make_pair(cisaOffset, f.relocOffset + genOffset));
      }

      // cisa index map
      count = read<uint32_t>(dbg);
      for (unsigned int j = 0; j != count; j++) {
        uint32_t cisaIndex = read<uint32_t>(dbg);
        uint32_t genOffset = read<uint32_t>(dbg);
        f.CISAIndexMap.push_back(
            std::make_pair(cisaIndex, f.relocOffset + genOffset));
      }

      // var info
      count = read<uint32_t>(dbg);
      for (unsigned int j = 0; j != count; j++) {
        VarInfo v;

        nameLen = read<uint16_t>(dbg);
        for (unsigned int k = 0; k != nameLen; k++)
          v.name += read<char>(dbg);

        auto countLRs = read<uint16_t>(dbg);
        for (unsigned int k = 0; k != countLRs; k++) {
          LiveIntervalsVISA lv = readLiveIntervalsVISA();
          v.lrs.push_back(lv);
        }

        f.Vars.push_back(v);
      }

      // subroutines
      count = read<uint16_t>(dbg);
      for (unsigned int j = 0; j != count; j++) {
        SubroutineInfo sub;
        nameLen = read<uint16_t>(dbg);
        for (unsigned int k = 0; k != nameLen; k++)
          sub.name += read<char>(dbg);

        sub.startVISAIndex = read<uint32_t>(dbg);
        sub.endVISAIndex = read<uint32_t>(dbg);
        auto countLRs = read<uint16_t>(dbg);
        for (unsigned int k = 0; k != countLRs; k++) {
          LiveIntervalsVISA lv = readLiveIntervalsVISA();
          sub.retval.push_back(lv);
        }
        f.subs.push_back(sub);
      }

      // call frame information
      f.cfi.frameSize = read<uint16_t>(dbg);
      f.cfi.befpValid = (bool)read<uint8_t>(dbg);
      if (f.cfi.befpValid) {
        count = read<uint16_t>(dbg);
        for (unsigned int j = 0; j != count; j++) {
          LiveIntervalGenISA lv = readLiveIntervalGenISA();
          ;
          f.cfi.befp.push_back(lv);
        }
      }
      f.cfi.callerbefpValid = (bool)read<uint8_t>(dbg);
      if (f.cfi.callerbefpValid) {
        count = read<uint16_t>(dbg);
        for (unsigned int j = 0; j != count; j++) {
          LiveIntervalGenISA lv = readLiveIntervalGenISA();
          f.cfi.callerbefp.push_back(lv);
        }
      }
      f.cfi.retAddrValid = (bool)read<uint8_t>(dbg);
      if (f.cfi.retAddrValid) {
        count = read<uint16_t>(dbg);
        for (unsigned int j = 0; j != count; j++) {
          LiveIntervalGenISA lv = readLiveIntervalGenISA();
          f.cfi.retAddr.push_back(lv);
        }
      }
      f.cfi.CEOffsetFromFPOff = read<uint16_t>(dbg);
      f.cfi.CEStoreIP = read<uint16_t>(dbg);
      f.cfi.numCalleeSaveEntries = read<uint16_t>(dbg);
      for (unsigned int j = 0; j != f.cfi.numCalleeSaveEntries; j++) {
        PhyRegSaveInfoPerIP phyRegSave;
        phyRegSave.genIPOffset = read<uint32_t>(dbg);
        phyRegSave.numEntries = read<uint16_t>(dbg);
        for (unsigned int k = 0; k != phyRegSave.numEntries; k++)
          phyRegSave.data.push_back(readRegInfoMapping());
        f.cfi.calleeSaveEntry.push_back(phyRegSave);
      }

      f.cfi.numCallerSaveEntries = read<uint32_t>(dbg);
      for (unsigned int j = 0; j != f.cfi.numCallerSaveEntries; j++) {
        PhyRegSaveInfoPerIP phyRegSave;
        phyRegSave.genIPOffset = read<uint32_t>(dbg);
        phyRegSave.numEntries = read<uint16_t>(dbg);
        for (unsigned int k = 0; k != phyRegSave.numEntries; k++)
          phyRegSave.data.push_back(readRegInfoMapping());
        f.cfi.callerSaveEntry.push_back(phyRegSave);
      }

      compiledObjs.push_back(f);
    }
  }

public:
  // TODO: we should pass the size too
  DbgDecoder(const void *buf) : dbg(buf) {
    if (buf)
      decode();
  }

  void print(llvm::raw_ostream &OS) const;
  void dump() const;
};
} // namespace IGC
