/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "G4_Kernel.hpp"
#include "BinaryEncodingIGA.h"
#include "BuildIR.h"
#include "Common_ISA_framework.h"
#include "DebugInfo.h"
#include "G4_BB.hpp"
#include "KernelCost.hpp"
#include "VISAKernel.h"
#include "VarSplit.h"
#include "iga/IGALibrary/Models/Models.hpp"
#include "iga/IGALibrary/api/kv.hpp"
#include "visa_wa.h"

#include <fstream>
#include <functional>
#include <iomanip>
#include <list>
#include <utility>

using namespace vISA;

void *gtPinData::getFreeGRFInfo(unsigned &size) {
  // Here is agreed upon format for reporting free GRFs:
  // struct freeBytes
  //{
  //    unsigned short startByte;
  //    unsigned short numConsecutiveBytes;
  //};

  // Added magic 0xDEADD00D at start and
  // magic 0xDEADBEEF at the end of buffer
  // on request of gtpin team.
  //
  // struct freeGRFInfo
  //{
  //    unsigned short numItems;
  //
  //    freeBytes data[numItems];
  //};
  struct freeBytes {
    unsigned short startByte;
    unsigned short numConsecutiveBytes;
  };

  struct freeGRFInfo {
    unsigned int magicStart;
    unsigned int numItems;
  };

  // Compute free register information using vector for efficiency,
  // then convert to POS for passing back to gtpin.
  std::vector<std::pair<unsigned short, unsigned short>> vecFreeBytes;

  for (auto byte : globalFreeRegs) {
    if (vecFreeBytes.size() > 0) {
      auto &lastFree = vecFreeBytes.back();
      if (byte == (lastFree.first + lastFree.second)) {
        lastFree.second += 1;
      } else {
        vecFreeBytes.push_back(std::make_pair(byte, 1));
      }
    } else {
      vecFreeBytes.push_back(std::make_pair(byte, 1));
    }
  }

  // Now convert vector to POS
  unsigned int numItems = (unsigned int)vecFreeBytes.size();
  freeGRFInfo *buffer = (freeGRFInfo *)malloc(
      numItems * sizeof(freeBytes) + sizeof(unsigned int) +
      sizeof(unsigned int) + sizeof(unsigned int));
  if (buffer) {
    buffer->numItems = numItems;
    buffer->magicStart = 0xDEADD00D;
    memcpy_s((unsigned char *)buffer + sizeof(unsigned int) +
                 sizeof(unsigned int),
             numItems * sizeof(freeBytes), vecFreeBytes.data(),
             numItems * sizeof(freeBytes));
    unsigned int magicEnd = 0xDEADBEEF;
    memcpy_s((unsigned char *)buffer + sizeof(unsigned int) +
                 sizeof(unsigned int) + (numItems * sizeof(freeBytes)),
             sizeof(magicEnd), &magicEnd, sizeof(magicEnd));

    // numItems - unsigned int
    // magicStart - unsigned int
    // magicEnd - unsigned int
    // data - numItems * sizeof(freeBytes)
    size = sizeof(unsigned int) + sizeof(unsigned int) + sizeof(unsigned int) +
           (numItems * sizeof(freeBytes));
  }

  return buffer;
}

void gtPinData::setGTPinInit(void *buffer) {
  vISA_ASSERT(sizeof(gtpin::igc::igc_init_t) <= 200,
              "Check size of igc_init_t");
  gtpin_init = (gtpin::igc::igc_init_t *)buffer;

  // reRA pass is no longer supported.
  // FIXME: should we assert here?
  // if (gtpin_init->re_ra)
  if (gtpin_init->grf_info)
    kernel.getOptions()->setOption(vISA_GetFreeGRFInfo, true);
}

template <class T>
void write(void *buffer, const T &data, unsigned int &offset) {
  memcpy_s((char *)buffer + offset, sizeof(T), &data, sizeof(T));
  offset += sizeof(T);
}

void *gtPinData::getIndirRefs(unsigned int &size) {
  // Store indirect access per %ip
  // %ip -> vector[start byte, size]
  std::map<unsigned int, std::vector<std::pair<unsigned int, unsigned int>>>
      indirRefMap;

  // return %ip of first executable instruction in kernel
  auto getIpOfFirstInst = [&]() {
    unsigned int startIp = 0;
    if (kernel.fg.getIsStackCallFunc()) {
      for (auto bb : kernel.fg.getBBList()) {
        if (startIp > 0)
          break;
        for (auto inst : bb->getInstList()) {
          startIp = (unsigned int)inst->getGenOffset();

          if (inst->isLabel())
            continue;

          // verify truncation is still legal
          vISA_ASSERT(inst->getGenOffset() == (uint32_t)inst->getGenOffset(),
                      "%ip out of bounds");

          if (startIp > 0)
            break;
        }
      }
    }
    return startIp;
  };

  unsigned int startIp = getIpOfFirstInst();

  auto getIndirRefData = [&](G4_Declare *addr) {
    // for given addr, return std::vector<std::pair<start byte, size>>
    std::vector<std::pair<unsigned int, unsigned int>> indirs;

    auto it = indirRefs.find(addr);
    if (it == indirRefs.end())
      return indirs;

    for (auto target : (*it).second) {
      if (target->isSpilled())
        continue;
      auto start = target->getGRFOffsetFromR0();
      auto size = target->getByteSize();
      indirs.push_back(std::make_pair(start, size));
    }
    return indirs;
  };

  for (auto bb : kernel.fg.getBBList()) {
    // Kernel's CFG may be stitched together
    // with that of its callees. We want to
    // iterate over only those BBs that belong
    // to current CFG.
    if (&bb->getParent() != &kernel.fg)
      break;
    for (auto inst : bb->getInstList()) {
      auto dst = inst->getDst();
      if (dst && dst->isIndirect()) {
        // encode dst indirect reference
        auto indirs = getIndirRefData(dst->getTopDcl());
        auto &mapEntry = indirRefMap[(uint32_t)inst->getGenOffset() - startIp];
        mapEntry.insert(mapEntry.end(), indirs.begin(), indirs.end());
      }

      for (unsigned int i = 0; i != inst->getNumSrc(); ++i) {
        auto src = inst->getSrc(i);
        if (src && src->isSrcRegRegion() &&
            src->asSrcRegRegion()->isIndirect()) {
          // encode src indirect reference
          auto indirs = getIndirRefData(src->asSrcRegRegion()->getTopDcl());
          auto &mapEntry =
              indirRefMap[(uint32_t)inst->getGenOffset() - startIp];
          mapEntry.insert(mapEntry.end(), indirs.begin(), indirs.end());
        }
      }
    }
  }

  unsigned int numRanges = 0;
  for (auto &item : indirRefMap) {
    numRanges += item.second.size();
  }

  // see gtpin_IGC_interface.h for format of igc_token_indirect_access_info_t
  size = sizeof(gtpin::igc::igc_token_indirect_access_info_t::num_ranges) +
         numRanges * sizeof(gtpin::igc::ins_reg_range_t);
  auto buffer = malloc(size);
  unsigned int offset = 0;
  write<uint32_t>(buffer, numRanges, offset);
  for (auto &item : indirRefMap) {
    for (const auto &arg : item.second) {
      vISA_ASSERT(offset < size, "Out of bounds");
      write<uint32_t>(buffer, item.first, offset);
      vISA_ASSERT(offset < size, "Out of bounds");
      write<uint16_t>(buffer, arg.first, offset);
      vISA_ASSERT(offset < size, "Out of bounds");
      write<uint16_t>(buffer, arg.second, offset);
    }
  }

  vISA_ASSERT(offset == size, "Unexpected bounds");

  return buffer;
}

template <typename T>
static void writeBuffer(std::vector<unsigned char> &buffer,
                        unsigned &bufferSize, const T *t, unsigned numBytes) {
  const unsigned char *data = (const unsigned char *)t;
  for (unsigned i = 0; i != numBytes; i++) {
    buffer.push_back(data[i]);
  }
  bufferSize += numBytes;
}

void *gtPinData::getGTPinInfoBuffer(unsigned &bufferSize,
                                    unsigned int scratchOffset) {
  if (!gtpin_init && !gtpinInitFromL0) {
    bufferSize = 0;
    return nullptr;
  }
  gtpin::igc::igc_init_t t;
  std::vector<unsigned char> buffer;
  unsigned numTokens = 0;
  auto stackABI =
      kernel.fg.getIsStackCallFunc() || kernel.fg.getHasStackCalls();
  bufferSize = 0;

  memset(&t, 0, sizeof(t));

  t.version = gtpin::igc::GTPIN_IGC_INTERFACE_VERSION;
  t.igc_init_size = sizeof(t);
  if (gtpinInitFromL0) {
    if (!stackABI) {
      if (kernel.getOption(vISA_GetFreeGRFInfo)) {
        t.grf_info = 1;
        numTokens++;
        // indirect info
        numTokens++;
      }

      if (kernel.getOption(vISA_GTPinReRA)) {
        t.re_ra = 1;
      }
    } else {
      // provide only indirect info for stack calls
      if (kernel.getOption(vISA_GetFreeGRFInfo)) {
        t.grf_info = 1;
        numTokens++;
      }
    }

    if (kernel.getOptions()->getOption(vISA_GenerateDebugInfo))
      t.srcline_mapping = 1;

    if (kernel.getOptions()->getuInt32Option(vISA_GTPinScratchAreaSize) > 0) {
      t.scratch_area_size = getNumBytesScratchUse();
      numTokens++;
    }

    if (!t.grf_info && kernel.getOptions()->getOption(vISA_GetFreeGRFInfo)) {
      // this check is to report out indir references, irrespective of
      // whether stack call is present.
      t.grf_info = 1;
      numTokens++;
    }
  } else {
    t.version =
        std::min(gtpin_init->version, gtpin::igc::GTPIN_IGC_INTERFACE_VERSION);
    if (!stackABI) {
      if (gtpin_init->grf_info) {
        t.grf_info = 1;
        numTokens++;
        // indirect info
        numTokens++;
      }

      if (gtpin_init->re_ra) {
        t.re_ra = 1;
      }
    } else {
      // provide only indirect info for stack calls
      if (gtpin_init->grf_info) {
        t.grf_info = 1;
        numTokens++;
      }
    }

    if (gtpin_init->srcline_mapping &&
        kernel.getOptions()->getOption(vISA_GenerateDebugInfo))
      t.srcline_mapping = 1;

    if (gtpin_init->scratch_area_size > 0) {
      t.scratch_area_size = gtpin_init->scratch_area_size;
      numTokens++;
    }

    if (!t.grf_info && gtpin_init->grf_info) {
      t.grf_info = 1;
      numTokens++;
    }
  }

  // For payload offsets
  numTokens++;

  // Report #GRFs
  numTokens++;

  writeBuffer(buffer, bufferSize, &t, sizeof(t));
  writeBuffer(buffer, bufferSize, &numTokens, sizeof(uint32_t));

  if (t.grf_info) {
    if (!stackABI) {
      // create token
      void *rerabuffer = nullptr;
      unsigned rerasize = 0;

      rerabuffer = getFreeGRFInfo(rerasize);

      gtpin::igc::igc_token_header_t th;
      th.token = gtpin::igc::GTPIN_IGC_TOKEN::GTPIN_IGC_TOKEN_GRF_INFO;
      th.token_size = sizeof(gtpin::igc::igc_token_header_t) + rerasize;

      // write token and data to buffer
      writeBuffer(buffer, bufferSize, &th, sizeof(th));
      writeBuffer(buffer, bufferSize, rerabuffer, rerasize);

      free(rerabuffer);
    }
    // report indir refs
    void *indirRefs = nullptr;
    unsigned int indirRefsSize = 0;

    indirRefs = getIndirRefs(indirRefsSize);

    gtpin::igc::igc_token_header_t th;
    th.token =
        gtpin::igc::GTPIN_IGC_TOKEN::GTPIN_IGC_TOKEN_INDIRECT_ACCESS_INFO;
    th.token_size = sizeof(gtpin::igc::igc_token_header_t) + indirRefsSize;

    // write token and data to buffer
    writeBuffer(buffer, bufferSize, &th, sizeof(th));
    writeBuffer(buffer, bufferSize, indirRefs, indirRefsSize);

    free(indirRefs);
  }

  if (t.scratch_area_size) {
    gtpin::igc::igc_token_scratch_area_info_t scratchSlotData;
    scratchSlotData.scratch_area_size = t.scratch_area_size;
    vISA_ASSERT(scratchOffset >= nextScratchFree, "scratch offset mismatch");
    scratchSlotData.scratch_area_offset = scratchOffset;

    // gtpin scratch slots are beyond spill memory
    scratchSlotData.token = gtpin::igc::GTPIN_IGC_TOKEN_SCRATCH_AREA_INFO;
    scratchSlotData.token_size = sizeof(scratchSlotData);

    writeBuffer(buffer, bufferSize, &scratchSlotData, sizeof(scratchSlotData));
  }

  {
    // Write payload offsets
    gtpin::igc::igc_token_kernel_start_info_t offsets;
    offsets.token = gtpin::igc::GTPIN_IGC_TOKEN_KERNEL_START_INFO;
    offsets.per_thread_prolog_size = kernel.getPerThreadNextOff();
    offsets.cross_thread_prolog_size =
        kernel.getCrossThreadNextOff() - offsets.per_thread_prolog_size;
    offsets.token_size = sizeof(offsets);
    writeBuffer(buffer, bufferSize, &offsets, sizeof(offsets));
  }

  {
    // Report num GRFs
    gtpin::igc::igc_token_num_grf_regs_t numGRFs;
    numGRFs.token = gtpin::igc::GTPIN_IGC_TOKEN_NUM_GRF_REGS;
    numGRFs.token_size = sizeof(numGRFs);
    numGRFs.num_grf_regs = kernel.getNumRegTotal();
    writeBuffer(buffer, bufferSize, &numGRFs, sizeof(numGRFs));
  }

  void *gtpinBuffer = allocCodeBlock(bufferSize);

  memcpy_s(gtpinBuffer, bufferSize, buffer.data(), bufferSize);

  // Dump buffer with shader dumps
  if (kernel.getOption(vISA_outputToFile)) {
    std::string asmName = kernel.getOptions()->getOptionCstr(VISA_AsmFileName);
    if (!asmName.empty()) {
      const VISAKernelImpl *vKernel =
          kernel.fg.builder->getParent()->getKernel(kernel.getName());
      if (vKernel && vKernel->getIsFunction()) {
        unsigned funcID = -1;
        vKernel->GetFunctionId(funcID);
        asmName += "_f" + std::to_string(funcID);
      }

      std::ofstream ofInit;
      std::stringstream ssInit;
      ssInit << asmName << ".gtpin_igc_init";
      ofInit.open(ssInit.str(), std::ofstream::binary);
      if (gtpin_init) {
        ofInit.write((const char *)gtpin_init, sizeof(*gtpin_init));
      }
      ofInit.close();

      std::ofstream ofInfo;
      std::stringstream ssInfo;
      ssInfo << asmName << ".gtpin_igc_info";
      ofInfo.open(ssInfo.str(), std::ofstream::binary);
      if (gtpinBuffer) {
        ofInfo.write((const char *)gtpinBuffer, bufferSize);
      }
      ofInfo.close();
    }
  }

  return gtpinBuffer;
}

void gtPinData::setScratchNextFree(unsigned next) {
  nextScratchFree = ((next + kernel.numEltPerGRF<Type_UB>() - 1) /
                     kernel.numEltPerGRF<Type_UB>()) *
                    kernel.numEltPerGRF<Type_UB>();
}

unsigned int gtPinData::getScratchNextFree() const { return nextScratchFree; }

uint32_t gtPinData::getNumBytesScratchUse() const {
  if (gtpin_init) {
    return gtpin_init->scratch_area_size;
  } else if (isGTPinInitFromL0()) {
    return kernel.getOptions()->getuInt32Option(vISA_GTPinScratchAreaSize);
  }
  return 0;
}

G4_Kernel::G4_Kernel(const PlatformInfo &pInfo, INST_LIST_NODE_ALLOCATOR &alloc,
                     Mem_Manager &m, Options *options, Attributes *anAttr,
                     uint32_t funcId, unsigned char major, unsigned char minor)
    : platformInfo(pInfo), m_options(options), m_kernelAttrs(anAttr),
      m_function_id(funcId), RAType(RA_Type::UNKNOWN_RA), asmInstCount(0),
      kernelID(0), fg(alloc, this, m), major_version(major),
      minor_version(minor), grfMode(pInfo.platform, pInfo.grfSize, options) {
  vISA_ASSERT(major < COMMON_ISA_MAJOR_VER || (major == COMMON_ISA_MAJOR_VER &&
                                               minor <= COMMON_ISA_MINOR_VER),
              "CISA version not supported by this JIT-compiler");

  name = NULL;
  hasAddrTaken = false;
  kernelDbgInfo = nullptr;
  if (options->getOption(vISAOptions::vISA_GetFreeGRFInfo) ||
      options->getuInt32Option(vISAOptions::vISA_GTPinScratchAreaSize)) {
    allocGTPinData();
  } else {
    gtPinInfo = nullptr;
  }
  autoGRFSelection = m_options->getOption(vISA_AutoGRFSelection);
  // NoMask WA
  m_EUFusionNoMaskWAInfo = nullptr;

  setKernelParameters();
}

G4_Kernel::~G4_Kernel() {
  if (kernelDbgInfo) {
    kernelDbgInfo.reset();
  }

  if (gtPinInfo) {
    gtPinInfo.reset();
  }

  Declares.clear();
}

void G4_Kernel::computeChannelSlicing() {
  G4_ExecSize simdSize = getSimdSize();
  channelSliced = true;

  if (simdSize == g4::SIMD8 || simdSize == g4::SIMD16) {
    // SIMD8/16 kernels are not sliced
    channelSliced = false;
    return;
  }

  if (simdSize == g4::SIMD32 && numEltPerGRF<Type_UB>() >= 64) {
    // For 64 bytes GRF, simd32 kernel, there is no slicing
    channelSliced = false;
    return;
  }
  // .dcl V1 size = 128 bytes
  // op (16|M0) V1(0,0)     ..
  // op (16|M16) V1(2,0)    ..
  // For above sequence, return 32. Instruction
  // is broken in to 2 only due to hw restriction.
  // Allocation of dcl is still as if it were a
  // SIMD32 kernel.

  // Store emask bits that are ever used to define a variable
  std::unordered_map<G4_Declare *, std::bitset<32>> emaskRef;
  for (auto bb : fg) {
    for (auto inst : *bb) {
      if (inst->isSend())
        continue;

      auto dst = inst->getDst();
      if (!dst || !dst->getTopDcl() || dst->getHorzStride() != 1)
        continue;

      if (inst->isWriteEnableInst())
        continue;

      auto regFileKind = dst->getTopDcl()->getRegFile();
      if (regFileKind != G4_RegFileKind::G4_GRF &&
          regFileKind != G4_RegFileKind::G4_INPUT)
        continue;

      if (dst->getTopDcl()->getByteSize() <=
          dst->getTypeSize() * (unsigned)simdSize)
        continue;

      auto emaskOffStart = inst->getMaskOffset();

      // Reset all bits on first encounter of dcl
      if (emaskRef.find(dst->getTopDcl()) == emaskRef.end())
        emaskRef[dst->getTopDcl()].reset();

      // Set bits based on which EM bits are used in the def
      for (unsigned i = emaskOffStart;
           i != (emaskOffStart + inst->getExecSize()); i++) {
        emaskRef[dst->getTopDcl()].set(i);
      }
    }
  }

  // Check whether any variable's emask usage straddles across lower and upper
  // 16 bits
  for (auto &emRefs : emaskRef) {
    auto &bits = emRefs.second;
    auto num = bits.to_ulong();

    // Check whether any lower 16 and upper 16 bits are set
    if (((num & 0xffff) != 0) && ((num & 0xffff0000) != 0)) {
      channelSliced = false;
      return;
    }
  }

  return;
}

void G4_Kernel::calculateSimdSize() {
  // Iterate over all instructions in kernel to check
  // whether default execution size of kernel is
  // SIMD8/16. This is required for knowing alignment
  // to use for GRF candidates.

  // only do it once per kernel, as we should not introduce inst with larger
  // simd size than in the input
  if (simdSize.value != 0) {
    return;
  }

  // First, get simdsize from attribute (0 : not given)
  // If not 0|8|16|32, wrong value from attribute.
  simdSize = G4_ExecSize(
      (unsigned)m_kernelAttrs->getInt32KernelAttr(Attributes::ATTR_SimdSize));
  if (simdSize != g4::SIMD8 && simdSize != g4::SIMD16 &&
      simdSize != g4::SIMD32) {
    vISA_ASSERT(simdSize.value == 0,
                "vISA: wrong value for SimdSize attribute");
    // pvc+: simd16; simd8 otherwise
    simdSize = fg.builder->getNativeExecSize();

    for (auto bb : fg) {
      for (auto inst : *bb) {
        // do not consider send since for certain messages we have to set its
        // execution size to 16 even in simd8 shaders
        // Also skip noMask inst
        if (!inst->isLabel() && !inst->isSend() && !inst->isWriteEnableInst()) {
          uint32_t size = inst->getMaskOffset() + inst->getExecSize();
          if (size > 16) {
            simdSize = g4::SIMD32;
            break;
          } else if (size > 8) {
            simdSize = g4::SIMD16;
          }
        }
      }
      if (simdSize == g4::SIMD32)
        break;
    }
  }

  if (GlobalRA::useGenericAugAlign(getPlatformGeneration()))
    computeChannelSlicing();
}

//
// Updates kernel's related structures to large GRF
//
bool G4_Kernel::updateKernelToLargerGRF() {
  if (numRegTotal == grfMode.getMaxGRF())
    return false;

  // Scale number of GRFs, Acc, SWSB tokens.
  setKernelParameters(grfMode.moveToLargerGRF());
  fg.builder->rebuildPhyRegPool(getNumRegTotal());
  return true;
}

//
// Updates kernel's related structures based on register pressure
//
void G4_Kernel::updateKernelByRegPressure(unsigned regPressure,
                                          bool forceGRFModeUp) {
  unsigned largestInputReg = getLargestInputRegister();
  if (m_kernelAttrs->isKernelAttrSet(Attributes::ATTR_MaxRegThreadDispatch)) {
    unsigned maxRegPayloadDispatch = m_kernelAttrs->getInt32KernelAttr(
        Attributes::ATTR_MaxRegThreadDispatch);
    largestInputReg = std::max(largestInputReg, maxRegPayloadDispatch);
  }

  unsigned newGRF = grfMode.setModeByRegPressure(regPressure, largestInputReg,
                                                 forceGRFModeUp);

  if (newGRF == numRegTotal)
    return;

  // Scale number of threads, Acc, SWSB tokens.
  setKernelParameters(newGRF);

  // Update physical register pool
  fg.builder->rebuildPhyRegPool(getNumRegTotal());
}

//
// Updates kernel's related structures based on NumGRF attribute
//
bool G4_Kernel::updateKernelFromNumGRFAttr() {
  unsigned attrNumGRF =
      m_kernelAttrs->getInt32KernelAttr(Attributes::ATTR_NumGRF);
  if (attrNumGRF != 0 && !grfMode.isValidNumGRFs(attrNumGRF))
    return false;
  if (numRegTotal == attrNumGRF)
    return true;

  autoGRFSelection = (attrNumGRF == 0);
  // Scale number of GRFs, Acc, SWSB tokens.
  setKernelParameters(attrNumGRF);
  fg.builder->rebuildPhyRegPool(getNumRegTotal());
  return true;
}

//
// Evaluate AddrExp/AddrExpList to Imm
//
void G4_Kernel::evalAddrExp() {
  for (std::list<G4_BB *>::iterator it = fg.begin(), itEnd = fg.end();
       it != itEnd; ++it) {
    G4_BB *bb = (*it);

    for (INST_LIST_ITER i = bb->begin(), iEnd = bb->end(); i != iEnd; i++) {
      G4_INST *inst = (*i);

      //
      // process each source operand
      //
      for (unsigned j = 0, numSrc = inst->getNumSrc(); j < numSrc; j++) {
        G4_Operand *opnd = inst->getSrc(j);
        if (!opnd)
          continue;
        if (opnd->isAddrExp()) {
          int val = opnd->asAddrExp()->eval(*fg.builder);
          G4_Type ty = opnd->asAddrExp()->getType();

          G4_Imm *imm = fg.builder->createImm(val, ty);
          inst->setSrc(imm, j);
        }
      }
    }
  }
}

[[maybe_unused]] static std::vector<std::string> split(const std::string &str,
                                                       const char *delimiter) {
  std::vector<std::string> v;
  std::string::size_type start = 0;

  for (auto pos = str.find_first_of(delimiter, start); pos != std::string::npos;
       start = pos + 1, pos = str.find_first_of(delimiter, start)) {
    if (pos != start) {
      v.emplace_back(str, start, pos - start);
    }
  }

  if (start < str.length())
    v.emplace_back(str, start, str.length() - start);
  return v;
}

static iga_gen_t getIGAPlatform(TARGET_PLATFORM genPlatform) {
  iga_gen_t platform = IGA_GEN_INVALID;
  switch (genPlatform) {
  case GENX_BDW:
    platform = IGA_GEN8;
    break;
  case GENX_CHV:
    platform = IGA_GEN8lp;
    break;
  case GENX_SKL:
    platform = IGA_GEN9;
    break;
  case GENX_BXT:
    platform = IGA_GEN9lp;
    break;
  case GENX_ICLLP:
    platform = IGA_GEN11;
    break;
  case GENX_TGLLP:
    platform = IGA_GEN12p1;
    break;
  case Xe_XeHPSDV:
    platform = IGA_XE_HP;
    break;
  case Xe_DG2:
  case Xe_MTL:
  case Xe_ARL:
    platform = IGA_XE_HPG;
    break;
  case Xe_PVC:
  case Xe_PVCXT:
    platform = IGA_XE_HPC;
    break;
  case Xe2:
    platform = IGA_XE2;
    break;
  case Xe3:
    platform = IGA_XE3;
    break;
  default:
    break;
  }

  return platform;
}

KernelDebugInfo *G4_Kernel::getKernelDebugInfo() {
  if (kernelDbgInfo == nullptr) {
    kernelDbgInfo = std::make_shared<KernelDebugInfo>();
  }

  return kernelDbgInfo.get();
}

void G4_Kernel::createKernelCostInfo(KernelCost *KCA) {
  //
  // copy data from FuncCost of KernelCostAnalysis to G4_Kernel's kernelCost
  // (LoopCost is src type, LoopCostInfo is dst type)
  //
  m_kernelCost = std::make_unique<KernelCostInfo>();
  FuncCost &FC = KCA->getKernelCost();

  int sz = FC.m_allLoopsInProgramOrder.size();
  m_kernelCost.get()->allLoopCosts.resize(sz);
  m_kernelCost.get()->kernelCost.C = FC.m_funcCost.C.getCostMetrics();

  for (int i = 0; i < sz; ++i) {
    const Loop *L = FC.m_allLoopsInProgramOrder[i];
    LoopCost &LC = KCA->getLoopCost(L);
    LoopCostInfo &LCI = m_kernelCost.get()->allLoopCosts[i];

    LCI.loopId = i;
    vISA_ASSERT(i == LC.m_loopId, "Kernel Cost Analysis: incorrect loop id");

    LCI.backedge_visaId = LC.m_backedge_visaId;
    const CostMetrics &cm = LC.m_loopBodyCost.C.getCostMetrics();
    LCI.loopBodyCost.C = cm;
    LCI.LCE = nullptr;
    LCI.numChildLoops = L->getNumImmChildLoops();
    vISA_ASSERT(LCI.numChildLoops == LC.m_loopBodyCost.LoopCosts.size(),
                "Kernel Cost Analysis: incorrect number of child loops!");
    LCI.nestingLevel = L->getNestingLevel();

    for (LoopCost *immLC : LC.m_loopBodyCost.LoopCosts) {
      int loop_id = immLC->m_loopId;
      LoopCostInfo &immLCI = m_kernelCost.get()->allLoopCosts[loop_id];
      LCI.loopBodyCost.loopCosts.push_back(&immLCI);
    }
  }
}

void StackCallABI::setVersion() {
  // VISA ABI version 1 is deprecated so default version to use is version 2
  version = StackCallABIVersion::VER_2;
}

void StackCallABI::init(G4_Kernel *k) {
  vISA_ASSERT(!kernel, "init called multiple times");
  kernel = k;
  setVersion();
  if (version == StackCallABIVersion::VER_3) {
    vISA_ASSERT(kernel->getGRFSize() == 64, "require 64-byte GRF for ABI v3");
    vISA_ASSERT(kernel->getPlatform() >= TARGET_PLATFORM::Xe3,
                "ABI v3 supported only for Xe3+");
  }

  switch (version) {
  case StackCallABIVersion::VER_1:
  case StackCallABIVersion::VER_2:
    subRegs.Ret_IP = SubRegs_Stackcall_v1_v2_Ret_IP;
    subRegs.Ret_EM = SubRegs_Stackcall_v1_v2_Ret_EM;
    subRegs.BE_SP = SubRegs_Stackcall_v1_v2_BE_SP;
    subRegs.BE_FP = SubRegs_Stackcall_v1_v2_BE_FP;
    subRegs.FE_FP = SubRegs_Stackcall_v1_v2_FE_FP;
    subRegs.FE_SP = SubRegs_Stackcall_v1_v2_FE_SP;

    offsets.Ret_IP = FrameDescriptorOfsets_v1_v2_Ret_IP;
    offsets.Ret_EM = FrameDescriptorOfsets_v1_v2_Ret_EM;
    offsets.BE_SP = FrameDescriptorOfsets_v1_v2_BE_SP;
    offsets.BE_FP = FrameDescriptorOfsets_v1_v2_BE_FP;
    offsets.FE_FP = FrameDescriptorOfsets_v1_v2_FE_FP;
    offsets.FE_SP = FrameDescriptorOfsets_v1_v2_FE_SP;
    break;
  case StackCallABIVersion::VER_3:
    subRegs.Ret_IP = SubRegs_Stackcall_v3_Ret_IP;
    subRegs.Ret_EM = SubRegs_Stackcall_v3_Ret_EM;
    subRegs.BE_SP = SubRegs_Stackcall_v3_BE_SP;
    subRegs.BE_FP = SubRegs_Stackcall_v3_BE_FP;
    subRegs.FE_FP = SubRegs_Stackcall_v3_FE_FP;
    subRegs.FE_SP = SubRegs_Stackcall_v3_FE_SP;

    offsets.Ret_IP = FrameDescriptorOfsets_v3_Ret_IP;
    offsets.Ret_EM = FrameDescriptorOfsets_v3_Ret_EM;
    offsets.BE_SP = FrameDescriptorOfsets_v3_BE_SP;
    offsets.BE_FP = FrameDescriptorOfsets_v3_BE_FP;
    offsets.FE_FP = FrameDescriptorOfsets_v3_FE_FP;
    offsets.FE_SP = FrameDescriptorOfsets_v3_FE_SP;
    break;
  default:
    vISA_ASSERT(false, "unknown ABI");
  }
  argReg = ArgRet_Stackcall_Arg;
  retReg = ArgRet_Stackcall_Ret;
}

unsigned StackCallABI::getStackCallStartReg() const {
  // Last 3 (or 2) GRFs reserved for stack call purpose
  unsigned totalGRFs = kernel->getNumRegTotal();
  unsigned startReg = totalGRFs - numReservedABIGRF();
  return startReg;
}
unsigned StackCallABI::calleeSaveStart() const {
  return getCallerSaveLastGRF() + 1;
}
unsigned StackCallABI::getNumCalleeSaveRegs() const {
  unsigned totalGRFs = kernel->getNumRegTotal();
  return totalGRFs - calleeSaveStart() - numReservedABIGRF();
}

uint32_t StackCallABI::numReservedABIGRF() const {
  if (version == StackCallABIVersion::VER_1)
    return 3;
  else if (version == StackCallABIVersion::VER_2) {
    if (kernel->getOption(vISA_PreserveR0InR0))
      return 2;
    return 3;
  } else {
    // for ABI version > 2
    return 1;
  }
}

uint32_t StackCallABI::getFPSPGRF() const {
  // For ABI V1, return (numRegTotal - 3), i.e. 125.
  // For ABI V2, return (numRegTotal - 1), i.e. 127, 255.
  // For ABI V3, return (numRegTotal - 1), i.e. 127, 255.

  if (version == StackCallABIVersion::VER_1) {
    return getStackCallStartReg() + FPSPGRF;
  } else if (version == StackCallABIVersion::VER_2) {
    return (kernel->getNumRegTotal() - 1) - FPSPGRF;
  } else {

    return (kernel->getNumRegTotal() - 1) - FPSPGRF;
  }
}

uint32_t StackCallABI::getSpillHeaderGRF() const {
  // For ABI V1 return r126.
  // For ABI V2 return r126.
  // For ABI V3 return r127.
  if (version == StackCallABIVersion::VER_1)
    return getStackCallStartReg() + SpillHeaderGRF;
  else if (version == StackCallABIVersion::VER_2)
    return (kernel->getNumRegTotal() - 1) - SpillHeaderGRF;
  else
    return kernel->stackCall.getFPSPGRF();
}

uint32_t StackCallABI::getThreadHeaderGRF() const {
  // For ABI V1 return r127.
  // For ABI V2 return r125.
  vISA_ASSERT(
      kernel->getOption(vISA_PreserveR0InR0) == false,
      "r0 is preserved in r0 itself. no special stack call header needed");
  if (version == StackCallABIVersion::VER_1)
    return getStackCallStartReg() + ThreadHeaderGRF;
  else
    return (kernel->getNumRegTotal() - 1) - ThreadHeaderGRF;
}

//
// perform relocation for every entry in the allocation table
//
void G4_Kernel::doRelocation(void *binary, uint32_t binarySize) {
  for (auto &&entry : relocationTable) {
    entry.doRelocation(*this, binary, binarySize);
  }
}

G4_INST *G4_Kernel::getFirstNonLabelInst() const {
  for (auto I = fg.cbegin(), E = fg.cend(); I != E; ++I) {
    auto bb = *I;
    G4_INST *firstInst = bb->getFirstInst();
    if (firstInst) {
      return firstInst;
    }
  }
  // empty kernel
  return nullptr;
}

std::string G4_Kernel::getDebugSrcLine(const std::string &fileName,
                                       int srcLine) {
  auto iter = debugSrcLineMap.find(fileName);
  if (iter == debugSrcLineMap.end()) {
    std::ifstream ifs(fileName);
    if (!ifs) {
      // file doesn't exist
      debugSrcLineMap[fileName] =
          std::make_pair<bool, std::vector<std::string>>(false, {});
      return "";
    }
    std::string line;
    std::vector<std::string> srcLines;
    while (std::getline(ifs, line)) {
      srcLines.push_back(line);
    }
    debugSrcLineMap[fileName] = std::make_pair(true, std::move(srcLines));
  }
  iter = debugSrcLineMap.find(fileName);
  if (iter == debugSrcLineMap.end() || !iter->second.first) {
    return "";
  }
  auto &lines = iter->second.second;
  if (srcLine > (int)lines.size() || srcLine <= 0) {
    return "invalid line number";
  }
  return lines[srcLine - 1];
}

unsigned G4_Kernel::getLargestInputRegister() {
  const unsigned inputCount = fg.builder->getInputCount();
  unsigned regNum = 0;
  if (inputCount) {
    const input_info_t *ii = fg.builder->getInputArg(inputCount - 1);
    regNum = (ii->offset + ii->dcl->getByteSize()) /
             fg.builder->numEltPerGRF<Type_UB>();
  }

  return regNum;
}

void G4_Kernel::setKernelParameters(unsigned newGRF) {
  unsigned overrideGRFNum = 0, overrideNumThreads = 0, overrideNumSWSB = 0,
           overrideNumAcc = 0;

  overrideGRFNum = m_options->getuInt32Option(vISA_TotalGRFNum);
  overrideNumThreads = m_options->getuInt32Option(vISA_HWThreadNumberPerEU);
  overrideNumSWSB = m_options->getuInt32Option(vISA_SWSBTokenNum);
  overrideNumAcc = m_options->getuInt32Option(vISA_numGeneralAcc);

  //
  // Number of threads/GRF can currently be set by:
  // 1.- Per kernel attribute
  // 2.- IGC flag (reg key)
  // 3.- Compiler option entered by user for
  //      2.1 entire module
  //      2.2 kernel function
  // 4.- Compiler heuristics
  //
  // 1 is set via kernel attribute. 2 and 3 via vISA option.
  // If none of them are set, compiler selects the best option (4).
  //

  if (newGRF > 0) {
    // per kernel attribute or GRF change during compilation
    grfMode.setModeByNumGRFs(newGRF);
    overrideGRFNum = 0;
  } else if (overrideNumThreads > 0) {
    // Forcing a specific number of threads
    grfMode.setModeByNumThreads(overrideNumThreads);
    overrideGRFNum = 0;
    autoGRFSelection = false;
  } else if (overrideGRFNum > 0) {
    // Forcing a specific number of GRFs
    grfMode.setModeByNumGRFs(overrideGRFNum);
    autoGRFSelection = false;
  } else {
    // Use default value
    grfMode.setDefaultGRF();
    overrideGRFNum = 0;
  }

  // Set number of GRFs
  numRegTotal = overrideGRFNum ? overrideGRFNum : grfMode.getNumGRF();
  auto lastCallerSavedGRF =
      getOptions()->getuInt32Option(vISA_LastCallerSavedGRF);
  // When vISA_LastCallerSavedGRF is set, it's an ABI breaking change.
  // Kernel and entire callee nest must be compiled with same
  // value of vISA_LastCallerSavedGRF for correctness.
  if (lastCallerSavedGRF)
    stackCall.setCallerSaveLastGRF(lastCallerSavedGRF);
  else
    stackCall.setCallerSaveLastGRF(((numRegTotal - 8) / 2) - 1);

  // Set number of threads
  numThreads = grfMode.getNumThreads();

  // Set the number of SWSB tokens
  numSWSBTokens =
      overrideNumSWSB ? overrideNumSWSB : grfMode.getNumSWSBTokens();

  // Set the number of Acc
  numAcc = overrideNumAcc ? overrideNumAcc : grfMode.getNumAcc();

  // Special configurations go here
  if (m_options->getOption(vISA_hasDoubleAcc)) {
    numAcc = 16;
  }
}

bool G4_Kernel::hasInlineData() const {
  const IR_Builder &b = *fg.builder;
  return
      b.getOption(vISA_useInlineData);
}

std::vector<ArgLayout> G4_Kernel::getArgumentLayout() {
  const uint32_t startGRF =
      getOptions()->getuInt32Option(vISA_loadThreadPayloadStartReg);
  const uint32_t inputsStart = startGRF * getGRFSize();
  const uint32_t inputCount = fg.builder->getInputCount();

  const int PTIS = AlignUp(
      getInt32KernelAttr(Attributes::ATTR_PerThreadInputSize), getGRFSize());

  // Checks if input_info is cross-thread-input
  auto isInCrossThreadData = [&](const input_info_t *input_info) {
    return (uint32_t)input_info->offset >= inputsStart + PTIS;
  };

  const uint32_t inlineDataSize = fg.builder->getInlineDataSize();
  const bool useInlineData = hasInlineData();
  // Checks if input_info fits in inlineData
  auto isInInlineData = [&](const input_info_t *const input_info) {
    if (!useInlineData) {
      return false;
    }
    uint32_t inputEnd = input_info->offset + input_info->size;
    bool fitsInInlineData = inputEnd <= inputsStart + PTIS + inlineDataSize;
    return isInCrossThreadData(input_info) && fitsInInlineData;
  };

  const uint32_t startGrfAddr =
      getOptions()->getuInt32Option(vISA_loadThreadPayloadStartReg) *
      getGRFSize();

  std::vector<ArgLayout> args;
  for (unsigned ix = 0; ix < inputCount; ix++) {
    const input_info_t *input = fg.builder->getInputArg(ix);
    if (input->isPseudoInput()) {
      continue;
    } else if (fg.builder->getFCPatchInfo()->getIsEntryKernel()) {
      const vISA::G4_Declare *dcl = input->dcl;
      if (INPUT_GENERAL == input->getInputClass() && !dcl->isLiveIn()) {
        break;
      }
    }
    int dstGrfAddr = input->offset;
    auto memSrc = ArgLayout::MemSrc::INVALID;
    int memOff = input->offset - startGrfAddr; // subtract off r0
    if (isInInlineData(input)) {
      memSrc = ArgLayout::MemSrc::INLINE;
      memOff %= getGRFSize();
      vISA_ASSERT(memOff < (int)inlineDataSize, "inline reg arg OOB");
      vISA_ASSERT(memOff + (int)input->size <= (int)inlineDataSize,
                  "inline reg arg overflows");
    } else if (isInCrossThreadData(input)) {
      memSrc = ArgLayout::MemSrc::CTI;
      memOff -= PTIS + (useInlineData ? inlineDataSize : 0);
    } else {
      memSrc = ArgLayout::MemSrc::PTI;
    }
    args.emplace_back(input->dcl, dstGrfAddr, memSrc, memOff, input->size);
  }
  std::sort(args.begin(), args.end(),
            [&](const ArgLayout &a1, const ArgLayout &a2) {
              return a1.dstGrfAddr < a2.dstGrfAddr;
            });
  return args;
}

void G4_Kernel::dump(std::ostream &os) const { fg.print(os); }

void G4_Kernel::dumpToFile(const std::string &suffixIn, bool forceG4Dump) {
  bool dumpDot = m_options->getOption(vISA_DumpDot);
  bool dumpG4 = forceG4Dump || m_options->getOption(vISA_DumpPasses) ||
                m_options->getuInt32Option(vISA_DumpPassesSubset) >= 1;
  if (!dumpDot && !dumpG4)
    return;

  // todo: remove else branch as it is not reached at all.
  std::stringstream ss;
  const char *prefix = nullptr;
  getOptions()->getOption(VISA_AsmFileName, prefix);
  if (prefix != nullptr) {
    // Use AsmFileName as prefix for g4/dot dumps
    if (fg.builder->getIsKernel()) {
      // entry
      ss << prefix << "." << std::setfill('0') << std::setw(3)
         << nextDumpIndex++ << "." << suffixIn;
    } else {
      // callee
      ss << prefix << "_f" << getFunctionId() << "." << std::setfill('0')
         << std::setw(3) << nextDumpIndex++ << "." << suffixIn;
    }
  } else {
    // calls to this will produce a sequence of dumps
    // [kernel-name].000.[suffix].{dot,g4}
    // [kernel-name].001.[suffix].{dot,g4}
    // ...
    // If vISA_DumpPassesSubset == 1 then we omit any files that don't change
    // the string representation of the kernel (i.e. skip passes that don't do
    // anything).
    if (m_options->getOption(vISA_DumpUseInternalName) || name == nullptr) {
      if (fg.builder->getIsKernel()) {
        ss << "k" << getKernelID();
      } else {
        ss << "f" << getFunctionId();
      }
    } else {
      ss << name;
    }
    ss << "." << std::setfill('0') << std::setw(3) << nextDumpIndex++ << "."
       << suffixIn;
  }
  std::string baseName = sanitizePathString(ss.str());

  if (dumpDot)
    dumpDotFileInternal(baseName);

  if (dumpG4)
    dumpG4Internal(baseName);
}

void G4_Kernel::dumpToConsole() { dumpG4InternalTo(std::cout); }

void G4_Kernel::emitDeviceAsm(std::ostream &os, const void *binary,
                              uint32_t binarySize) {
  //
  // for GTGPU lib release, don't dump out asm
  //
#ifdef NDEBUG
#ifdef GTGPU_LIB
  return;
#endif
#endif
  const bool newAsm = m_options->getOption(vISA_dumpNewSyntax) &&
                      !(binary == NULL || binarySize == 0);

  if (!m_options->getOption(vISA_StripComments)) {
    emitDeviceAsmHeaderComment(os);
  }

  if (!newAsm) {
    emitDeviceAsmInstructionsOldAsm(os);
    return;
  }

  emitDeviceAsmInstructionsIga(os, binary, binarySize);

  if (getPlatformGeneration() >= PlatformGen::XE) {
    os << "\n\n";
    auto jitInfo = fg.builder->getJitInfo();
    os << "//.BankConflicts: " << jitInfo->statsVerbose.BCNum << "\n";
    os << "//.ByteRMWs: " << jitInfo->statsVerbose.numByteRMWs << "\n//\n";
  } else {
    os << "// Bank Conflict Statistics: \n";
    os << "// -- GOOD: " << fg.BCStats.NumOfGoodInsts << "\n";
    os << "// --  BAD: " << fg.BCStats.NumOfBadInsts << "\n";
    os << "// --   OK: " << fg.BCStats.NumOfOKInsts << "\n";
  }
}

void G4_Kernel::emitRegInfo() {
  const char *asmName = nullptr;
  getOptions()->getOption(VISA_AsmFileName, asmName);
  const char *asmNameEmpty = "";
  if (!asmName) {
    asmName = asmNameEmpty;
  }

  std::string dumpFileName = std::string(asmName) + ".reginfo";
  std::fstream ofile(dumpFileName, std::ios::out);

  emitRegInfoKernel(ofile);

  ofile.close();
}

void G4_Kernel::emitRegInfoKernel(std::ostream &output) {
  output << "//.platform " << getGenxPlatformString();
  output << "\n"
         << "//.kernel ID 0x" << std::hex << getKernelID() << "\n";
  output << std::dec << "\n";
  int instOffset = 0;

  for (BB_LIST_ITER itBB = fg.begin(); itBB != fg.end(); ++itBB) {
    for (INST_LIST_ITER itInst = (*itBB)->begin(); itInst != (*itBB)->end();
         ++itInst) {
      G4_INST *inst = (*itInst);
      if (inst->isLabel()) {
        continue;
      }
      if (inst->getLexicalId() == -1) {
        continue;
      }

      (*itBB)->emitRegInfo(output, inst, instOffset);
      instOffset += inst->isCompactedInst() ? 8 : 16;
    }
  }
  return;
}

//
// This routine dumps out the dot file of the control flow graph along with
// instructions. dot is drawing graph tool from AT&T.
//
void G4_Kernel::dumpDotFileInternal(const std::string &baseName) {
  std::fstream ofile(baseName + ".dot", std::ios::out);
  vASSERT(!ofile.fail());
  //
  // write digraph KernelName {"
  //          size = "8, 10";
  //
  const char *asmFileName = NULL;
  m_options->getOption(VISA_AsmFileName, asmFileName);
  if (asmFileName == NULL)
    ofile << "digraph UnknownKernel"
          << " {"
          << "\n";
  else
    ofile << "digraph " << asmFileName << " {"
          << "\n";
  //
  // keep the graph width 8, estimate a reasonable graph height
  //
  const unsigned itemPerPage = 64; // 60 instructions per Letter page
  unsigned totalItem = (unsigned)Declares.size();
  for (std::list<G4_BB *>::iterator it = fg.begin(); it != fg.end(); ++it)
    totalItem += ((unsigned)(*it)->size());
  totalItem += (unsigned)fg.size();
  float graphHeight = (float)totalItem / itemPerPage;
  graphHeight =
      graphHeight < 100.0f ? 100.0f : graphHeight; // minimal size: Letter
  ofile << "\n\t// Setup\n";
  ofile << "\tsize = \"80.0, " << graphHeight << "\";\n";
  ofile << "\tpage= \"80.5, 110\";\n";
  ofile << "\tpagedir=\"TL\";\n";
  // dump out flow graph
  for (std::list<G4_BB *>::iterator it = fg.begin(); it != fg.end(); ++it) {
    G4_BB *bb = (*it);
    //
    // write:   BB0 [shape=plaintext, label=<
    //                      <TABLE BORDER="0" CELLBORDER="1" CELLSPACING="0">
    //                          <TR><TD ALIGN="CENTER">BB0: TestRA_Dot</TD></TR>
    //                          <TR><TD>
    //                              <TABLE BORDER="0" CELLBORDER="0"
    //                              CELLSPACING="0">
    //                                  <TR><TD
    //                                  ALIGN="LEFT">TestRA_Dot:</TD></TR>
    //                                  <TR><TD ALIGN="LEFT"><FONT
    //                                  color="red">add (8) Region(0,0)[1]
    //                                  Region(0,0)[8;8,1] PAYLOAD(0,0)[8;8,1]
    //                                  [NoMask]</FONT></TD></TR>
    //                              </TABLE>
    //                          </TD></TR>
    //                      </TABLE>>];
    // print out label if the first inst is a label inst
    //
    ofile << "\t";
    bb->writeBBId(ofile);
    ofile << " [shape=plaintext, label=<"
          << "\n";
    ofile << "\t\t\t    <TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\">"
          << "\n";
    ofile << "\t\t\t\t<TR><TD ALIGN=\"CENTER\">";
    bb->writeBBId(ofile);
    ofile << ": ";

    if (!bb->empty() && bb->front()->isLabel()) {
      bb->front()->getSrc(0)->emit(ofile);
    }
    ofile << "</TD></TR>"
          << "\n";
    // emit all instructions within basic block
    ofile << "\t\t\t\t<TR><TD>"
          << "\n";

    if (!bb->empty()) {
      ofile << "\t\t\t\t\t    <TABLE BORDER=\"0\" CELLBORDER=\"0\" "
               "CELLSPACING=\"0\">"
            << "\n";
      for (INST_LIST_ITER i = bb->begin(); i != bb->end(); i++) {
        //
        // detect if there is spill code first, set different color for it
        //
        std::string fontColor = "black";
        //
        // emit the instruction
        //
        ofile << "\t\t\t\t\t\t<TR><TD ALIGN=\"LEFT\"><FONT color=\""
              << fontColor << "\">";
        std::ostringstream os;
        (*i)->emit(os);
        std::string dotStr(os.str());
        // TODO: dot doesn't like '<', '>', '{', or '}' (and '&') this code
        // below is a hack. need to replace with delimiters.
        // std::replace_if(dotStr.begin(), dotStr.end(),
        // bind2nd(equal_to<char>(), '<'), '[');
        std::replace_if(
            dotStr.begin(), dotStr.end(),
            std::bind(std::equal_to<char>(), std::placeholders::_1, '<'), '[');
        std::replace_if(
            dotStr.begin(), dotStr.end(),
            std::bind(std::equal_to<char>(), std::placeholders::_1, '>'), ']');
        std::replace_if(
            dotStr.begin(), dotStr.end(),
            std::bind(std::equal_to<char>(), std::placeholders::_1, '{'), '[');
        std::replace_if(
            dotStr.begin(), dotStr.end(),
            std::bind(std::equal_to<char>(), std::placeholders::_1, '}'), ']');
        std::replace_if(
            dotStr.begin(), dotStr.end(),
            std::bind(std::equal_to<char>(), std::placeholders::_1, '&'), '$');
        ofile << dotStr;

        ofile << "</FONT></TD></TR>"
              << "\n";
        // ofile << "\\l"; // left adjusted
      }
      ofile << "\t\t\t\t\t    </TABLE>"
            << "\n";
    }

    ofile << "\t\t\t\t</TD></TR>"
          << "\n";
    ofile << "\t\t\t    </TABLE>>];"
          << "\n";
    //
    // dump out succ edges
    // BB12 -> BB10
    //
    for (std::list<G4_BB *>::iterator sit = bb->Succs.begin();
         sit != bb->Succs.end(); ++sit) {
      bb->writeBBId(ofile);
      ofile << " -> ";
      (*sit)->writeBBId(ofile);
      ofile << "\n";
    }
  }
  //
  // write "}" to end digraph
  //
  ofile << "\n"
        << " }"
        << "\n";
  //
  // close dot file
  //
  ofile.close();
}

// Dump the instructions into a .g4 file
void G4_Kernel::dumpG4Internal(const std::string &file) {
  std::stringstream g4asm;
  dumpG4InternalTo(g4asm);
  std::string g4asms = g4asm.str();
  if (m_options->getuInt32Option(vISA_DumpPassesSubset) == 1 &&
      g4asms == lastG4Asm) {
    return;
  }
  lastG4Asm = std::move(g4asms);

  std::fstream ofile(file + ".g4", std::ios::out);
  vASSERT(!ofile.fail());
  dumpG4InternalTo(ofile);
}

void G4_Kernel::dumpG4InternalTo(std::ostream &os) {
  if (name)
    os << ".kernel " << name << "\n";
  else
    os << ".kernel\n";

  for (const G4_Declare *d : Declares) {
    static const int MIN_DECL = 34; // skip the built-in decls
    if (d->getDeclId() > MIN_DECL) {
      // os << d->getDeclId() << "\n";
      d->emit(os);
    }
  }
  os << "\n";

  // Additional dumps for lit testing
  os << "// simdSize = " << (int)simdSize.value << "\n";

  os << "\n";
  for (std::list<G4_BB *>::iterator it = fg.begin(); it != fg.end(); ++it) {
    // Emit BB number
    G4_BB *bb = (*it);
    bb->writeBBId(os);

    // Emit BB type
    if (bb->getBBType()) {
      os << " [" << bb->getBBTypeStr() << "] ";
    }

    os << "\tPreds: ";
    for (auto pred : bb->Preds) {
      pred->writeBBId(os);
      os << " ";
    }
    os << "\tSuccs: ";
    for (auto succ : bb->Succs) {
      succ->writeBBId(os);
      os << " ";
    }
    os << "\n";

    bb->emit(os);
    os << "\n\n";
  } // bbs
}

void G4_Kernel::emitDeviceAsmHeaderComment(std::ostream &os) {
  os << "//.kernel ";
  if (name != NULL) {
    // some 3D kernels do not have a name
    os << name;
  }

#if !Release
  os << "\n"
     << "//.platform " << getGenxPlatformString();
  os << "\n"
     << "//.thread_config "
     << "numGRF=" << numRegTotal << ", numAcc=" << numAcc;
#endif

  if (fg.builder->hasSWSB()) {
    os << ", numSWSB=" << numSWSBTokens;
  }
  os << "\n"
     << "//.options_string \"" << m_options->getUserArgString().str() << "\"";
  os << "\n"
     << "//.full_options \"" << m_options->getFullArgString() << "\"";
  os << "\n"
     << "//.instCount " << asmInstCount;
  static const char *const RATypeString[]{RA_TYPE(STRINGIFY)};
  os << "\n//.RA type\t" << RATypeString[RAType];
  if (!m_options->getOption(vISA_skipGitHash))
    os << "\n//.git-hash " << GIT_COMMIT_HASH;

  if (auto jitInfo = fg.builder->getJitInfo()) {
    if (jitInfo->stats.numGRFUsed != 0) {
      os << "\n"
         << "//.GRF count " << jitInfo->stats.numGRFUsed;
    }
    if (jitInfo->stats.spillMemUsed > 0) {
      os << "\n"
         << "//.spill size " << jitInfo->stats.spillMemUsed;
    }
    if (jitInfo->stats.numGRFSpillFillWeighted > 0) {
      os << "\n"
         << "//.spill GRF est. ref count "
         << jitInfo->stats.numGRFSpillFillWeighted;
    }
    if (jitInfo->stats.numFlagSpillStore > 0) {
      os << "\n//.spill flag store " << jitInfo->stats.numFlagSpillStore;
      os << "\n//.spill flag load " << jitInfo->stats.numFlagSpillLoad;
    }
  }

  auto privateMemSize = getInt32KernelAttr(Attributes::ATTR_SpillMemOffset);
  if (privateMemSize != 0) {
    os << "\n//.private memory size " << privateMemSize;
  }
  os << "\n\n";

  // Step2: emit declares (as needed)
  for (auto dcl : Declares) {
    dcl->emit(os);
  }
  os << "\n";

  auto fmtHex = [](int i) {
    std::stringstream ss;
    ss << "0x" << std::hex << std::uppercase << i;
    return ss.str();
  };

  auto args = getArgumentLayout();
  const unsigned inputCount = (unsigned)args.size();
  std::vector<std::string> argNames;
  size_t maxNameLen = 8;
  for (unsigned ix = 0; ix < inputCount; ix++) {
    const ArgLayout &a = args[ix];
    std::stringstream ss;
    if (a.decl && a.decl->getName()) {
      ss << a.decl->getName();
    } else {
      ss << "__unnamed" << (ix + 1);
    }
    argNames.push_back(ss.str());
    maxNameLen = std::max(maxNameLen, argNames.back().size());
  }

  // emit input location and size
  os << "// .inputs\n";
  const size_t COLW_IDENT = maxNameLen;
  static const size_t COLW_TYPE = 8;
  static const size_t COLW_SIZE = 6;
  static const size_t COLW_AT = 8;    // e.g. "r16+0x20"
  static const size_t COLW_FROM = 16; // e.g. "inline+0x20"

  std::stringstream bordss;
  bordss << "// ";
  bordss << '+';
  bordss << std::setfill('-') << std::setw(COLW_IDENT + 2) << "";
  bordss << '+';
  bordss << std::setfill('-') << std::setw(COLW_TYPE + 2) << "";
  bordss << '+';
  bordss << std::setfill('-') << std::setw(COLW_SIZE + 2) << "";
  bordss << '+';
  bordss << std::setfill('-') << std::setw(COLW_AT + 2) << "";
  bordss << '+';
  bordss << std::setfill('-') << std::setw(COLW_FROM + 2) << "";
  bordss << '+' << "\n";
  std::string border = bordss.str();

  os << border;
  os << "//"
     << " | " << std::left << std::setw(COLW_IDENT) << "id"
     << " | " << std::left << std::setw(COLW_TYPE) << "type"
     << " | " << std::right << std::setw(COLW_SIZE) << "bytes"
     << " | " << std::left << std::setw(COLW_AT) << "at"
     << " | " << std::left << std::setw(COLW_FROM) << "from"
     << " |"
     << "\n";
  os << border;

  const unsigned grfSize = getGRFSize();
  for (unsigned ix = 0; ix < inputCount; ix++) {
    const ArgLayout &a = args[ix];
    //
    os << "//";
    //
    // id
    os << " | " << std::left << std::setw(COLW_IDENT) << argNames[ix];
    //
    // type and length
    //   e.g. :uq x 16
    const G4_Declare *dcl = a.decl;
    std::stringstream sstype;
    if (dcl) {
      switch (dcl->getElemType()) {
      case Type_B:
        sstype << ":b";
        break;
      case Type_W:
        sstype << ":w";
        break;
      case Type_D:
        sstype << ":d";
        break;
      case Type_Q:
        sstype << ":q";
        break;
      case Type_V:
        sstype << ":v";
        break;
      case Type_UB:
        sstype << ":ub";
        break;
      case Type_UW:
        sstype << ":uw";
        break;
      case Type_UD:
        sstype << ":ud";
        break;
      case Type_UQ:
        sstype << ":uq";
        break;
      case Type_UV:
        sstype << ":uv";
        break;
        //
      case Type_F:
        sstype << ":f";
        break;
      case Type_HF:
        sstype << ":hf";
        break;
      case Type_DF:
        sstype << ":df";
        break;
      case Type_NF:
        sstype << ":nf";
        break;
      case Type_BF:
        sstype << ":bf";
        break;
      default:
        sstype << fmtHex((int)dcl->getElemType()) << "?";
        break;
      }
      if (dcl->getTotalElems() != 1)
        sstype << " x " << dcl->getTotalElems();
    } else {
      sstype << "?";
    }
    os << " | " << std::left << std::setw(COLW_TYPE) << sstype.str();
    //
    // size
    os << " | " << std::right << std::setw(COLW_SIZE) << fmtHex(a.size);

    // location
    unsigned reg = a.dstGrfAddr / grfSize, subRegBytes = a.dstGrfAddr % grfSize;
    std::stringstream ssloc;
    ssloc << "r" << reg;
    if (subRegBytes != 0)
      ssloc << "+" << fmtHex(subRegBytes);
    os << " | " << std::left << std::setw(COLW_AT) << ssloc.str();

    // from
    std::string from;
    switch (a.memSource) {
    case ArgLayout::MemSrc::CTI:
      from = "cti";
      break;
    case ArgLayout::MemSrc::PTI:
      from = "pti[tid]";
      break;
    case ArgLayout::MemSrc::INLINE:
      from = "inline";
      break;
    default:
      from = fmtHex(int(a.memSource)) + "?";
      break;
    }
    std::stringstream ssf;
    ssf << from;
    ssf << "+" << fmtHex(a.memOffset);

    os << " | " << std::left << std::setw(COLW_FROM) << ssf.str();
    //
    os << " |\n";
  }
  os << border << "\n";

  if (getPlatformGeneration() < PlatformGen::XE) {
    fg.BCStats.clear();
  }
}

using BlockOffsets = std::map<int32_t, std::vector<std::string>>;

static BlockOffsets precomputeBlockOffsets(std::ostream &os, G4_Kernel &g4k,
                                           const KernelView &kv) {
  // pre-compute the PCs of each basic block
  int32_t currPc = 0, lastInstSize = -1;
  BlockOffsets blockOffsets;
  for (BB_LIST_ITER itBB = g4k.fg.begin(); itBB != g4k.fg.end(); ++itBB) {
    for (INST_LIST_ITER itInst = (*itBB)->begin(); itInst != (*itBB)->end();
         ++itInst) {
      if ((*itInst)->isLabel()) {
        // G4 treats labels as special instructions
        const char *lbl = (*itInst)->getLabelStr();
        if (lbl && *lbl) {
          blockOffsets[currPc].emplace_back(lbl);
        }
      } else {
        // we are looking at the next G4 instruction,
        // but reached the end of the decode stream
        if (lastInstSize == 0) {
          os << "// ERROR: deducing G4 block PCs "
                "(IGA decoded stream ends early); falling back to IGA labels\n";
          blockOffsets.clear(); // fallback to IGA default labels
          return blockOffsets;
        }
        lastInstSize = kv.getInstSize(currPc);

        G4_INST *inst = (*itInst);

       // For HW WA.
       // In which, vISA may ask IGA to emit some additional instructions.
       // For example, sync is used to make instruction aligned, and nop is
       // used to support stepping in debugger.
       // However, due to compaction, we might not know the exact location of
       // the instruction, the sync instruction insertion has to happen during
       // encoding, which is unknown for the instruction size of kernel in the
       // decoding. That's the issue we have to make these changes.
        if (inst->isCachelineAligned()) {
          iga::Op opcode = kv.getOpcode(currPc);
          // There could be multiple sync.nop instructions emitted by IGA to
          // make the instruction aligned. Here we continue to advance PC when
          // seeing sync.nop so that vISA inst and IGA inst could match again.
          while (opcode == iga::Op::SYNC) {
            currPc += lastInstSize;
            opcode = kv.getOpcode(currPc);
            lastInstSize = kv.getInstSize(currPc);
          }
        }

        // When the inst requires an additional nop after it, again we need to
        // advance PC to consume NOP to make vISA inst and IGA inst match later.
        if (inst->requireNopAfter()) {
          currPc += lastInstSize;
          lastInstSize = kv.getInstSize(currPc);
          vASSERT(kv.getOpcode(currPc) == iga::Op::NOP);
        }

        currPc += lastInstSize;
      }
    }
  }
  if (kv.getInstSize(currPc) != 0) {
    // we are looking at the next G4 instruction,
    // but reached the end of the decode stream
    os << "// ERROR: deducing G4 block PCs "
          "(G4_INST stream ends early); falling back to IGA labels\n";
    blockOffsets.clear(); // fallback to IGA default labels
  }
  return blockOffsets;
}

// needs further cleanup (confirm label prefixes are gone, newAsm == true)
void G4_Kernel::emitDeviceAsmInstructionsIga(std::ostream &os,
                                             const void *binary,
                                             uint32_t binarySize) {
  os << "\n";

  const size_t ERROR_STRING_MAX_LENGTH = 16 * 1024;
  char *errBuf = new char[ERROR_STRING_MAX_LENGTH];
  vASSERT(errBuf);
  if (!errBuf)
    return;

  iga_gen_t igaPlatform = getIGAPlatform(getPlatform());

  const iga::Model *igaModel =
      iga::Model::LookupModel(iga::ToPlatform(igaPlatform));
  iga::SWSB_ENCODE_MODE swsbEncodeMode = igaModel->getSWSBEncodeMode();


  KernelView kv(igaPlatform, binary, binarySize, swsbEncodeMode, errBuf,
                ERROR_STRING_MAX_LENGTH
  );

  if (!kv.decodeSucceeded()) {
    const char *MSG =
        "vISA asm emission: failed to re-decode binary for asm output\n";
    // trb: do we really need to clobber std::cerr from a driver?
    // Shader dump output will have the message.
    std::cerr << MSG;
    std::cerr << errBuf << "\n";
    os << MSG;
    os << errBuf << "\n";
    // still continue since parital output might be present
  }
  delete[] errBuf;

  const auto blockOffsets = precomputeBlockOffsets(os, *this, kv);

  //
  // Generate a label with uniqueLabel as prefix (required by some tools).
  // We do so by using labeler callback.  If uniqueLabels is not present, use
  // iga's default label.  For example,
  //   Without option -uniqueLabels:
  //      generating default label,   L1234
  //   With option -uniqueLabels <sth>:
  //      generating label with <sth> as prefix, <sth>_L1234
  //
  std::string labelPrefix;
  if (m_options->getOption(vISA_UniqueLabels)) {
    const char *labelPrefixC = nullptr;
    m_options->getOption(vISA_LabelStr, labelPrefixC);
    labelPrefix = labelPrefixC;
    if (!labelPrefix.empty())
      labelPrefix += '_';
  }

  struct LabelerState {
    const KernelView *kv;
    const BlockOffsets &blockOffsets;
    const std::string labelPrefix;
    std::string labelStorage;
    LabelerState(const KernelView *_kv, const BlockOffsets &offs,
                 const std::string &lblPfx)
        : kv(_kv), blockOffsets(offs), labelPrefix(lblPfx) {}
  };
  LabelerState ls(&kv, blockOffsets, labelPrefix);

  // storage for the IGA labeler
  auto labeler = [](int32_t pc, void *data) -> const char * {
    LabelerState &ls = *(LabelerState *)data;
    ls.labelStorage = ls.labelPrefix;
    auto itr = ls.blockOffsets.find(pc);
    if (itr == ls.blockOffsets.end()) {
      // let IGA choose the label name, but we still have to prefix
      // our user provided prefix
      char igaDefaultLabel[128];
      ls.kv->getDefaultLabelName(pc, igaDefaultLabel, sizeof(igaDefaultLabel));
      ls.labelStorage += igaDefaultLabel;
      return ls.labelStorage.c_str();
    }
    std::string g4Label = itr->second.front().c_str();
    ls.labelStorage += g4Label;
    return ls.labelStorage.c_str();
  };

  // initialize register suppression info
  int suppressRegs[5] = {};
  int lastRegs[3] = {};
  for (int i = 0; i < 3; i++) {
    suppressRegs[i] = -1;
    lastRegs[i] = -1;
  }

  ////////////////////////////////////////
  // emit the program text (instructions) iteratively
  // this is a little tricky because G4 treats labels as instructions
  // thus we need to do a little checking to keep the two streams in sync
  int32_t pc = 0;
  std::vector<char> igaStringBuffer;
  igaStringBuffer.resize(512); // TODO: expand default after testing

  // printedLabels - tracked the labels those have been printed to the pc to
  // avoid printing the same label twice at the same pc. This can happen when
  // there's an empty BB contains only labels. The BB and the following BB will
  // both print those labels. The pair is the pc to label name pair.
  std::set<std::pair<int32_t, std::string>> printedLabels;
  // tryPrintLable - check if the given label is already printed with the given
  // pc. Print it if not, and skip it if yes.
  auto tryPrintLabel = [&os, &printedLabels](int32_t label_pc,
                                             const std::string &label_name) {
    auto label_pair = std::make_pair(label_pc, label_name);
    // skip if the same label in the set
    if (printedLabels.find(label_pair) != printedLabels.end())
      return;
    os << label_name << ":\n";
    printedLabels.insert(label_pair);
  };

  for (BB_LIST_ITER itBB = fg.begin(); itBB != fg.end(); ++itBB) {
    os << "// ";
    (*itBB)->emitBbInfo(os);
    os << "\n";
    for (INST_LIST_ITER itInst = (*itBB)->begin(); itInst != (*itBB)->end();
         ++itInst) {
      G4_INST *i = (*itInst);

      // walk to next non-label in this block;
      // return true if we find one, else fails if at end of block
      auto findNextNonLabel = [&](bool print) {
        while ((*itInst)->isLabel()) {
          if (print)
            os << "// " << (*itInst)->getLabelStr() << ":\n";
          itInst++;
          if (itInst == (*itBB)->end())
            break;
        }
        if (itInst == (*itBB)->end())
          return false;
        i = (*itInst);
        return true;
      };

      bool isInstTarget = kv.isInstTarget(pc);
      if (isInstTarget) {
        auto itr = ls.blockOffsets.find(pc);
        if (itr == ls.blockOffsets.end()) {
          std::string labelname(labeler(pc, &ls));
          tryPrintLabel(pc, labelname);
        } else {
          // there can be multiple labels per PC
          for (const std::string &lbl : itr->second) {
            std::string labelname(ls.labelPrefix + lbl);
            tryPrintLabel(pc, labelname);
          }
        }
        if (!findNextNonLabel(false)) {
          break; // at end of block
        }
      } else if (i->isLabel()) {
        // IGA doesn't consider this PC to be a label but G4 does
        //
        // move forward until we find the next non-label
        if (!findNextNonLabel(true)) {
          break; // at end of block
        }
      }

      ///////////////////////////////////////////////////////////////////
      // we are looking at a non-label G4_INST at the next valid IGA PC
      // (same instruction)
      if (!getOptions()->getOption(vISA_disableInstDebugInfo)) {
        (*itBB)->emitInstructionSourceLineMapping(os, itInst);
      }

      uint32_t fmtOpts =
          IGA_FORMATTING_OPTS_DEFAULT | IGA_FORMATTING_OPT_PRINT_BFNEXPRS;
      if (getOption(vISA_PrintHexFloatInAsm))
        fmtOpts |= IGA_FORMATTING_OPT_PRINT_HEX_FLOATS;
      if (!getOption(vISA_noLdStAsmSyntax))
        fmtOpts |= IGA_FORMATTING_OPT_PRINT_LDST;

      auto formatToInstToStream = [&](int32_t pc, std::ostream &os) {
        // multiple calls to getInstSyntax since we may have to
        // dynamically resize buffer
        while (true) {
          size_t nw =
              kv.getInstSyntax(pc, igaStringBuffer.data(),
                               igaStringBuffer.size(), fmtOpts, labeler, &ls);
          if (nw == 0) {
            os << "<<error formatting instruction at "
                  "PC 0x"
               << std::uppercase << std::hex << pc << ">>\n";
            break;
          } else if (nw <= igaStringBuffer.size()) {
            // print it (pad it out so comments line up on most instructions)
            std::string line = igaStringBuffer.data();
            while (line.size() < 100)
              line += ' ';
            os << line;
            break;
          } else {
            igaStringBuffer.resize(igaStringBuffer.size() + 512);
            // try again
          }
        }
      };

      // Advance PC when the vISA instruction needs to be cacheline-aligned or
      // requires a Nop after. See comments in precomputeBlockOffsets for
      // details.
      if (i->isCachelineAligned()) {
        iga::Op opcode = kv.getOpcode(pc);
        while (opcode == iga::Op::SYNC) {
          formatToInstToStream(pc, os);
          os << "\n";
          pc += kv.getInstSize(pc);
          opcode = kv.getOpcode(pc);
        }
      }
      if (i->requireNopAfter()) {
        formatToInstToStream(pc, os);
        os << "\n";
        pc += kv.getInstSize(pc);
        vASSERT(kv.getOpcode(pc) == iga::Op::NOP);
      }

      formatToInstToStream(pc, os);

      (*itBB)->emitBasicInstructionComment(os, itInst, suppressRegs, lastRegs,
                                           pc);
      os << "\n";

      pc += kv.getInstSize(pc);
    } // for insts in block
  }   // for blocks
} // emitDeviceAsmInstructionsIga

// Should be removed once we can confirm no one uses it
// the output comes from G4_INST::... and almost certainly won't be
// parsable by IGA
void G4_Kernel::emitDeviceAsmInstructionsOldAsm(std::ostream &os) {
  os << "\n"
     << ".code";
  for (BB_LIST_ITER it = fg.begin(); it != fg.end(); ++it) {
    os << "\n";
    (*it)->emit(os);
  }
  // Step4: emit clean-up.
  os << "\n";
  os << ".end_code"
     << "\n";
  os << ".end_kernel"
     << "\n";
  os << "\n";
}

G4_BB *G4_Kernel::getNextBB(G4_BB *bb) const {
  if (!bb)
    return nullptr;

  // Return the lexically following bb.
  G4_BB *nextBB = nullptr;
  for (auto it = fg.cbegin(), ie = fg.cend(); it != ie; it++) {
    auto curBB = (*it);
    if (curBB == bb) {
      it++;
      if (it != ie) {
        nextBB = (*it);
      }
      break;
    }
  }

  return nextBB;
}

unsigned G4_Kernel::getBinOffsetOfBB(G4_BB *bb) const {
  G4_INST *succInst = bb ? bb->getFirstInst() : nullptr;

  if (succInst != nullptr) {
    return (unsigned)succInst->getGenOffset();
  } else {
    G4_BB *succBB = bb ? getNextBB(bb) : nullptr;

    while ((succBB != nullptr) && (succInst == nullptr)) {
      succInst = succBB->getFirstInst();
      succBB = getNextBB(succBB);
    }

    if (succInst != nullptr) {
      return (unsigned)succInst->getGenOffset();
    } else {
      return 0;
    }
  }
}

unsigned G4_Kernel::getPerThreadNextOff() const {
  if (!hasPerThreadPayloadBB())
    return 0;
  G4_BB *next = getNextBB(perThreadPayloadBB);
  return getBinOffsetOfBB(next);
}

unsigned G4_Kernel::getCrossThreadNextOff() const {
  if (!hasCrossThreadPayloadBB())
    return 0;
  G4_BB *next = getNextBB(crossThreadPayloadBB);
  return getBinOffsetOfBB(next);
}

unsigned G4_Kernel::getComputeFFIDGPNextOff() const {
  if (!hasComputeFFIDProlog())
    return 0;
  // return the offset of the second entry (GP1)
  // the first instruction in the second BB is the start of the second entry
  vISA_ASSERT(fg.getNumBB() > 1, "expect at least one prolog BB");
  vASSERT(!computeFFIDGP1->empty() && !computeFFIDGP1->front()->isLabel());
  return getBinOffsetOfBB(computeFFIDGP1);
}

unsigned G4_Kernel::getComputeFFIDGP1NextOff() const {
  if (!hasComputeFFIDProlog())
    return 0;
  // return the offset of the BB next to GP1
  // the first instruction in the second BB is the start of the second entry
  vISA_ASSERT(fg.getNumBB() > 1, "expect at least one prolog BB");
  G4_BB *next = getNextBB(computeFFIDGP1);
  return getBinOffsetOfBB(next);
}

unsigned G4_Kernel::getSRFInWords() {
  return (fg.builder->getNumScalarRegisters() *
          fg.builder->getScalarRegisterSizeInBytes()) /
         2;
}

// GRF modes supported by HW
// There must be at least one Config that is VRTEnable for each platform
GRFMode::GRFMode(const TARGET_PLATFORM plat, unsigned regSize, Options *op)
    : platform(plat), grfSize(regSize), options(op) {
  switch (platform) {
  case Xe_XeHPSDV:
  case Xe_DG2:
  case Xe_MTL:
  case Xe_ARL:
    configs.resize(2);
    // Configurations with <numGRF, numThreads, SWSBTokens, numAcc>
    configs[0] = Config(128, 8, 16, 4);
    configs[1] = Config(256, 4, 16, 8);
    defaultMode = 0;
    break;
  case Xe_PVC:
  case Xe_PVCXT:
  case Xe2:
    configs.resize(2);
    // Configurations with <numGRF, numThreads, SWSBTokens, numAcc>
    configs[0] = Config(128, 8, 16, 4);
    configs[1] = Config(256, 4, 32, 8);
    defaultMode = 0;
    break;
  case Xe3:
    configs.resize(7);
    // Configurations with <numGRF, numThreads, SWSBTokens, numAcc>
    configs[0] = Config(32, 10, 32, 4);
    configs[1] = Config(64, 10, 32, 4);
    configs[2] = Config(96, 10, 32, 4);
    configs[3] = Config(128, 8, 32, 4);
    configs[4] = Config(160, 6, 32, 4);
    configs[5] = Config(192, 5, 32, 4);
    configs[6] = Config(256, 4, 32, 8);
    defaultMode = 3;
    break;
  default:
    // platforms <= TGL
    configs.resize(1);
    // Configurations with <numGRF, numThreads, SWSBTokens, numAcc>
    configs[0] = {128, 7, 16, 2};
    defaultMode = 0;
  }
  currentMode = defaultMode;

  // Set lower bound GRF
  unsigned minGRF = op->getuInt32Option(vISA_MinGRFNum);
  lowerBoundGRF = minGRF > 0 ? minGRF : configs.front().numGRF;
  vISA_ASSERT(isValidNumGRFs(lowerBoundGRF),
              "Invalid lower bound for GRF number");

  // Set upper bound GRF
  unsigned maxGRF = op->getuInt32Option(vISA_MaxGRFNum);
  upperBoundGRF = maxGRF > 0 ? maxGRF : configs.back().numGRF;
  vISA_ASSERT(isValidNumGRFs(upperBoundGRF),
              "Invalid upper bound for GRF number");

  // Select higher GRF
  GRFModeUpValue = op->getuInt32Option(vISA_ForceGRFModeUp);
  vISA_ASSERT(GRFModeUpValue >= 0 && GRFModeUpValue <= configs.size(),
              "Invalid value for selecting a higher GRF mode");
}

unsigned GRFMode::setModeByRegPressure(unsigned maxRP, unsigned largestInputReg,
                                       bool forceGRFModeUp) {
  unsigned size = configs.size(), i = 0;
  bool spillAllowed = getSpillThreshold() > 0;
  unsigned spillThresholdInRegs = getSpillThreshold() / grfSize;
  // find appropiate GRF based on reg pressure
  for (; i < size; i++) {
    if (configs[i].VRTEnable && configs[i].numGRF >= lowerBoundGRF &&
        configs[i].numGRF <= upperBoundGRF) {
      currentMode = i;
      if (maxRP <= configs[i].numGRF &&
          // Check that we've at least 8 GRFs over and above
          // those blocked for kernel input. This helps cases
          // where an 8 GRF variable shows up in entry BB.
          (largestInputReg + 8) <= configs[i].numGRF) {
        if (forceGRFModeUp && GRFModeUpValue > 0) {
          // Check if user is force a higher GRF mode
          unsigned newGRFMode = currentMode + GRFModeUpValue;
          unsigned maxGRFMode = getMaxGRFMode();
          currentMode = newGRFMode < maxGRFMode ? newGRFMode : maxGRFMode;
        }

        if (spillAllowed && !hasSmallerGRFSameThreads() && currentMode > 0) {
          unsigned lowerGRFNum = getSmallerGRF();
          // Select a lower GRF number in PreRA in case the register
          // pressure computed is a bit higher (e.g. 4%) than the lower GRF
          // config. If spills are detected, RA will still bump up the GRF
          // number to avoid them.
          // For example, if reg pressure is 165, we select 160GRF since
          // we have spill threshold enabled and the diff between 165 and 160
          // is less than 4%.
          if ((lowerGRFNum * 1.04 >= maxRP ||
               configs[currentMode].numGRF == getMaxGRF()) &&
              lowerGRFNum >= (largestInputReg + 8) &&
              lowerGRFNum >= lowerBoundGRF)
            setModeByNumGRFs(lowerGRFNum);
        }
        return configs[currentMode].numGRF;
      } else if (spillAllowed &&
                 maxRP <= configs[i].numGRF + spillThresholdInRegs &&
                 (largestInputReg + 8) <= configs[i].numGRF) {
        return configs[currentMode].numGRF;
      }
    }
  }
  // RP is greater than the maximum GRF available, so set the largest GRF
  // available
  return configs[currentMode].numGRF;
}

// Check if next larger GRF has the same number of threads per EU
bool GRFMode::hasLargerGRFSameThreads() const {
  unsigned largerGrfIdx = currentMode + 1;
  if (largerGrfIdx == configs.size() || !configs[largerGrfIdx].VRTEnable)
    return false;

  return configs[currentMode].numThreads == configs[largerGrfIdx].numThreads;
}

// Check if next smaller GRF has the same number of threads per EU
bool GRFMode::hasSmallerGRFSameThreads() const {
  int smallerGrfIdx = currentMode - 1;
  if (smallerGrfIdx < 0 || !configs[smallerGrfIdx].VRTEnable)
    return false;
  return configs[currentMode].numThreads == configs[smallerGrfIdx].numThreads;
}

// Get spill threshold for current GRF mode
unsigned GRFMode::getSpillThreshold() const {
  if (platform < Xe3)
    return 0;
  // FIXME: currently spill thresholds for <96GRF are
  // causing some performance regressions. We need more
  // study to define proper thresholds for this range.
  if (configs[currentMode].numGRF < 96)
    return 0;
  if (configs[currentMode].numGRF == 256 &&
      options->getuInt32Option(vISA_SpillAllowed256GRF) > 0)
    return options->getuInt32Option(vISA_SpillAllowed256GRF);

  return options->getuInt32Option(vISA_SpillAllowed);
}
