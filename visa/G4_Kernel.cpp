/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "BuildIR.h"
#include "DebugInfo.h"
#include "G4_Kernel.hpp"
#include "G4_BB.hpp"
#include "VarSplit.h"
// #include "iga/IGALibrary/api/igaEncoderWrapper.hpp"
#include "iga/IGALibrary/api/kv.hpp"
#include "BinaryEncodingIGA.h"
#include "Common_ISA_framework.h"
#include "VISAKernel.h"

#include <list>
#include <fstream>
#include <functional>
#include <iomanip>
#include <utility>

using namespace vISA;

void gtPinData::markInsts()
{
    // Take a snapshot of instructions in kernel.
    for (auto bb : kernel.fg)
    {
        for (auto inst : *bb)
        {
            markedInsts.insert(inst);
        }
    }
}

void gtPinData::removeUnmarkedInsts()
{
    if (!kernel.fg.getIsStackCallFunc() &&
        !kernel.fg.getHasStackCalls())
    {
        // Marked instructions correspond to caller/callee save
        // and FP/SP manipulation instructions.
        return;
    }

    MUST_BE_TRUE(whichRAPass == ReRAPass,
        "Unexpectedly removing unmarked instructions in first RA pass");
    // Instructions not seen in "marked" snapshot will be removed by this function.
    for (auto bb : kernel.fg)
    {
        for (auto it = bb->begin(), itEnd = bb->end();
            it != itEnd;)
        {
            auto inst = (*it);

            if (markedInsts.find(inst) == markedInsts.end())
            {
                it = bb->erase(it);
                continue;
            }
            it++;
        }
    }
}

void* gtPinData::getFreeGRFInfo(unsigned& size)
{
    // Here is agreed upon format for reporting free GRFs:
    //struct freeBytes
    //{
    //    unsigned short startByte;
    //    unsigned short numConsecutiveBytes;
    //};

    // Added magic 0xDEADD00D at start and
    // magic 0xDEADBEEF at the end of buffer
    // on request of gtpin team.
    //
    //struct freeGRFInfo
    //{
    //    unsigned short numItems;
    //
    //    freeBytes data[numItems];
    //};
    struct freeBytes
    {
        unsigned short startByte;
        unsigned short numConsecutiveBytes;
    };

    struct freeGRFInfo
    {
        unsigned int magicStart;
        unsigned int numItems;
    };

    // Compute free register information using vector for efficiency,
    // then convert to POS for passing back to gtpin.
    std::vector<std::pair<unsigned short, unsigned short>> vecFreeBytes;

    for (auto byte : globalFreeRegs)
    {
        if (vecFreeBytes.size() > 0)
        {
            auto& lastFree = vecFreeBytes.back();
            if (byte == (lastFree.first + lastFree.second))
            {
                lastFree.second += 1;
            }
            else
            {
                vecFreeBytes.push_back(std::make_pair(byte, 1));
            }
        }
        else
        {
            vecFreeBytes.push_back(std::make_pair(byte, 1));
        }
    }

    // Now convert vector to POS
    unsigned int numItems = (unsigned int)vecFreeBytes.size();
    freeGRFInfo* buffer = (freeGRFInfo*)malloc(numItems * sizeof(freeBytes) + sizeof(unsigned int)
        + sizeof(unsigned int) + sizeof(unsigned int));
    if (buffer)
    {
        buffer->numItems = numItems;
        buffer->magicStart = 0xDEADD00D;
        memcpy_s((unsigned char*)buffer + sizeof(unsigned int) + sizeof(unsigned int),
            numItems * sizeof(freeBytes), vecFreeBytes.data(), numItems * sizeof(freeBytes));
        unsigned int magicEnd = 0xDEADBEEF;
        memcpy_s((unsigned char*)buffer + sizeof(unsigned int) + sizeof(unsigned int) + (numItems * sizeof(freeBytes)),
            sizeof(magicEnd), &magicEnd, sizeof(magicEnd));

        // numItems - unsigned int
        // magicStart - unsigned int
        // magicEnd - unsigned int
        // data - numItems * sizeof(freeBytes)
        size = sizeof(unsigned int) + sizeof(unsigned int) + sizeof(unsigned int) + (numItems * sizeof(freeBytes));
    }

    return buffer;
}

void gtPinData::setGTPinInit(void* buffer)
{
    MUST_BE_TRUE(sizeof(gtpin::igc::igc_init_t) <= 200, "Check size of igc_init_t");
    gtpin_init = (gtpin::igc::igc_init_t*)buffer;

    if (gtpin_init->re_ra)
        kernel.getOptions()->setOption(vISA_ReRAPostSchedule, true);
    if (gtpin_init->grf_info)
        kernel.getOptions()->setOption(vISA_GetFreeGRFInfo, true);
}

template<class T>
void write(void* buffer, const T& data, unsigned int& offset)
{
    memcpy_s((char*)buffer+offset, sizeof(T), &data, sizeof(T));
    offset += sizeof(T);
}

void* gtPinData::getIndirRefs(unsigned int& size)
{
    // Store indirect access per %ip
    // %ip -> vector[start byte, size]
    std::map<unsigned int, std::vector<std::pair<unsigned int, unsigned int>>> indirRefMap;

    // return %ip of first executable instruction in kernel
    auto getIpOfFirstInst = [&]()
    {
        unsigned int startIp = 0;
        if (kernel.fg.getIsStackCallFunc())
        {
            for (auto bb : kernel.fg.getBBList())
            {
                if (startIp > 0)
                    break;
                for (auto inst : bb->getInstList())
                {
                    startIp = (unsigned int)inst->getGenOffset();

                    // verify truncation is still legal
                    MUST_BE_TRUE(inst->getGenOffset() == (uint32_t)inst->getGenOffset(),
                        "%ip out of bounds");

                    if (startIp > 0)
                        break;
                }
            }
        }
        return startIp;
    };

    unsigned int startIp = getIpOfFirstInst();

    auto getIndirRefData = [&](G4_Declare* addr)
    {
        // for given addr, return std::vector<std::pair<start byte, size>>
        std::vector<std::pair<unsigned int, unsigned int>> indirs;

        auto it = indirRefs.find(addr);
        if (it == indirRefs.end())
            return indirs;

        for (auto target : (*it).second)
        {
            auto start = target->getGRFBaseOffset();
            auto size = target->getByteSize();
            indirs.push_back(std::make_pair(start, size));
        }
        return std::move(indirs);
    };

    for (auto bb : kernel.fg.getBBList())
    {
        for (auto inst : bb->getInstList())
        {
            auto dst = inst->getDst();
            if (dst && dst->isIndirect())
            {
                // encode dst indirect reference
                auto indirs = getIndirRefData(dst->getTopDcl());
                auto& mapEntry = indirRefMap[(uint32_t)-inst->getGenOffset() - startIp];
                mapEntry.insert(mapEntry.end(),
                    indirs.begin(), indirs.end());
            }

            for (unsigned int i = 0; i != inst->getNumSrc(); ++i)
            {
                auto src = inst->getSrc(i);
                if (src && src->isSrcRegRegion() &&
                    src->asSrcRegRegion()->isIndirect())
                {
                    // encode src indirect reference
                    auto indirs = getIndirRefData(src->asSrcRegRegion()->getTopDcl());
                    auto& mapEntry = indirRefMap[(uint32_t)inst->getGenOffset() - startIp];
                    mapEntry.insert(mapEntry.end(),
                        indirs.begin(), indirs.end());
                }
            }
        }
    }

    unsigned int numRanges = 0;
    for (auto& item : indirRefMap)
    {
        numRanges += item.second.size();
    }

    // see gtpin_IGC_interface.h for format of igc_token_indirect_access_info_t
    size = sizeof(gtpin::igc::igc_token_indirect_access_info_t::num_ranges) + numRanges * sizeof(gtpin::igc::ins_reg_range_t);
    auto buffer = malloc(size);
    unsigned int offset = 0;
    write<uint32_t>(buffer, numRanges, offset);
    for (auto& item : indirRefMap)
    {
        MUST_BE_TRUE(offset < size, "Out of bounds");
        write<uint32_t>(buffer, item.first, offset);
        for (const auto& arg : item.second)
        {
            MUST_BE_TRUE(offset < size, "Out of bounds");
            write<uint16_t>(buffer, arg.first, offset);
            MUST_BE_TRUE(offset < size, "Out of bounds");
            write<uint16_t>(buffer, arg.second, offset);
        }
    }

    MUST_BE_TRUE(offset == size, "Unexpected bounds");

    return buffer;
}

template<typename T>
static void writeBuffer(
    std::vector<unsigned char>& buffer,
    unsigned& bufferSize,
    const T* t,
    unsigned numBytes)
{
    const unsigned char* data = (const unsigned char*)t;
    for (unsigned i = 0; i != numBytes; i++)
    {
        buffer.push_back(data[i]);
    }
    bufferSize += numBytes;
}

void* gtPinData::getGTPinInfoBuffer(unsigned &bufferSize)
{
    if (!gtpin_init && !gtpinInitFromL0)
    {
        bufferSize = 0;
        return nullptr;
    }
    gtpin::igc::igc_init_t t;
    std::vector<unsigned char> buffer;
    unsigned numTokens = 0;
    auto stackABI = kernel.fg.getIsStackCallFunc() || kernel.fg.getHasStackCalls();
    bufferSize = 0;

    memset(&t, 0, sizeof(t));

    t.version = gtpin::igc::GTPIN_IGC_INTERFACE_VERSION;
    t.igc_init_size = sizeof(t);
    if (gtpinInitFromL0)
    {
        if (!stackABI)
        {
            if (kernel.getOption(vISA_GetFreeGRFInfo))
            {
                t.grf_info = 1;
                numTokens++;
                // indirect info
                numTokens++;
            }

            if (kernel.getOption(vISA_GTPinReRA))
            {
                t.re_ra = 1;
            }
        }
        else
        {
            // provide only indirect info for stack calls
            if (kernel.getOption(vISA_GetFreeGRFInfo))
            {
                t.grf_info = 1;
                numTokens++;
            }
        }

        if (kernel.getOptions()->getOption(vISA_GenerateDebugInfo))
            t.srcline_mapping = 1;

        if (kernel.getOptions()->getuInt32Option(vISA_GTPinScratchAreaSize) > 0)
        {
            t.scratch_area_size = getNumBytesScratchUse();
            numTokens++;
        }

        if (!t.grf_info &&
            kernel.getOptions()->getOption(vISA_GetFreeGRFInfo))
        {
            // this check is to report out indir references, irrespective of
            // whether stack call is present.
            t.grf_info = 1;
            numTokens++;
        }
    }
    else
    {
        t.version = std::min(gtpin_init->version, gtpin::igc::GTPIN_IGC_INTERFACE_VERSION);
        if (!stackABI)
        {
            if (gtpin_init->grf_info)
            {
                t.grf_info = 1;
                numTokens++;
                // indirect info
                numTokens++;
            }

            if (gtpin_init->re_ra)
            {
                t.re_ra = 1;
            }
        }
        else
        {
            // provide only indirect info for stack calls
            if (gtpin_init->grf_info)
            {
                t.grf_info = 1;
                numTokens++;
            }
        }

        if (gtpin_init->srcline_mapping && kernel.getOptions()->getOption(vISA_GenerateDebugInfo))
            t.srcline_mapping = 1;

        if (gtpin_init->scratch_area_size > 0)
        {
            t.scratch_area_size = gtpin_init->scratch_area_size;
            numTokens++;
        }

        if (!t.grf_info &&
            gtpin_init->grf_info)
        {
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

    if (t.grf_info)
    {
        if (!stackABI)
        {
            // create token
            void* rerabuffer = nullptr;
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
        void* indirRefs = nullptr;
        unsigned int indirRefsSize = 0;

        indirRefs = getIndirRefs(indirRefsSize);

        gtpin::igc::igc_token_header_t th;
        th.token = gtpin::igc::GTPIN_IGC_TOKEN::GTPIN_IGC_TOKEN_INDIRECT_ACCESS_INFO;
        th.token_size = sizeof(gtpin::igc::igc_token_header_t) + indirRefsSize;

        // write token and data to buffer
        writeBuffer(buffer, bufferSize, &th, sizeof(th));
        writeBuffer(buffer, bufferSize, indirRefs, indirRefsSize);

        free(indirRefs);
    }

    if (t.scratch_area_size)
    {
        gtpin::igc::igc_token_scratch_area_info_t scratchSlotData;
        scratchSlotData.scratch_area_size = t.scratch_area_size;
        scratchSlotData.scratch_area_offset = nextScratchFree;

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
        offsets.cross_thread_prolog_size = kernel.getCrossThreadNextOff() - offsets.per_thread_prolog_size;
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

    void* gtpinBuffer = allocCodeBlock(bufferSize);

    memcpy_s(gtpinBuffer, bufferSize, buffer.data(), bufferSize);

    // Dump buffer with shader dumps
    if (kernel.getOption(vISA_outputToFile))
    {
        std::string asmName = kernel.getOptions()->getOptionCstr(VISA_AsmFileName);
        if (!asmName.empty())
        {
            const VISAKernelImpl* vKernel = kernel.fg.builder->getParent()->getKernel(kernel.getName());
            if (vKernel && vKernel->getIsFunction())
            {
                unsigned funcID = -1;
                vKernel->GetFunctionId(funcID);
                asmName += "_f" + std::to_string(funcID);
            }

            std::ofstream ofInit;
            std::stringstream ssInit;
            ssInit << asmName << ".gtpin_igc_init";
            ofInit.open(ssInit.str(), std::ofstream::binary);
            if (gtpin_init)
            {
                ofInit.write((const char*)gtpin_init, sizeof(*gtpin_init));
            }
            ofInit.close();

            std::ofstream ofInfo;
            std::stringstream ssInfo;
            ssInfo << std::string(asmName) << ".gtpin_igc_info";
            ofInfo.open(ssInfo.str(), std::ofstream::binary);
            if (gtpinBuffer)
            {
                ofInfo.write((const char*)gtpinBuffer, bufferSize);
            }
            ofInfo.close();
        }
    }

    return gtpinBuffer;
}

void gtPinData::setScratchNextFree(unsigned next) {
    nextScratchFree = ((next + kernel.numEltPerGRF<Type_UB>() - 1) / kernel.numEltPerGRF<Type_UB>()) * kernel.numEltPerGRF<Type_UB>();
}

unsigned int gtPinData::getScratchNextFree() const
{
    return nextScratchFree;
}

uint32_t gtPinData::getNumBytesScratchUse() const
{
    if (gtpin_init)
    {
        return gtpin_init->scratch_area_size;
    }
    else if (isGTPinInitFromL0())
    {
        return kernel.getOptions()->getuInt32Option(vISA_GTPinScratchAreaSize);
    }
    return 0;
}


G4_Kernel::G4_Kernel(const PlatformInfo& pInfo, INST_LIST_NODE_ALLOCATOR& alloc,
    Mem_Manager& m, Options* options, Attributes* anAttr, uint32_t funcId,
    unsigned char major, unsigned char minor)
    : platformInfo(pInfo), m_options(options), m_kernelAttrs(anAttr),
    m_function_id(funcId), RAType(RA_Type::UNKNOWN_RA),
    asmInstCount(0), kernelID(0), fg(alloc, this, m),
    major_version(major), minor_version(minor)
{
    ASSERT_USER(
        major < COMMON_ISA_MAJOR_VER ||
        (major == COMMON_ISA_MAJOR_VER && minor <= COMMON_ISA_MINOR_VER),
        "CISA version not supported by this JIT-compiler");

    name = NULL;
    numThreads = 0;
    hasAddrTaken = false;
    kernelDbgInfo = nullptr;
    sharedDebugInfo = false;
    sharedGTPinInfo = false;
    if (options->getOption(vISAOptions::vISA_ReRAPostSchedule) ||
        options->getOption(vISAOptions::vISA_GetFreeGRFInfo) ||
        options->getuInt32Option(vISAOptions::vISA_GTPinScratchAreaSize))
    {
        allocGTPinData();
    } else {
        gtPinInfo = nullptr;
    }

    // NoMask WA
    m_EUFusionNoMaskWAInfo = nullptr;

    setKernelParameters();
}

G4_Kernel::~G4_Kernel()
{
    if (kernelDbgInfo && !sharedDebugInfo)
    {
        kernelDbgInfo->~KernelDebugInfo();
    }

    if (gtPinInfo && !sharedGTPinInfo)
    {
        gtPinInfo->~gtPinData();
    }

    if (varSplitPass)
    {
        delete varSplitPass;
        varSplitPass = nullptr;
    }

    Declares.clear();
}

void G4_Kernel::setKernelDebugInfo(KernelDebugInfo* k)
{
    assert(k);
    if (kernelDbgInfo)
    {
        kernelDbgInfo->~KernelDebugInfo();
    }
    kernelDbgInfo = k;
    sharedDebugInfo = true;
}

void G4_Kernel::setGTPinData(gtPinData* p) {
    assert(p);
    if (gtPinInfo == nullptr)
    {
        gtPinInfo->~gtPinData();
    }
    gtPinInfo = p;
    sharedGTPinInfo = true;
}

void G4_Kernel::computeChannelSlicing()
{
    G4_ExecSize simdSize = getSimdSize();
    channelSliced = true;

    if (simdSize == g4::SIMD8 || simdSize == g4::SIMD16)
    {
        // SIMD8/16 kernels are not sliced
        channelSliced = false;
        return;
    }

    if (simdSize == g4::SIMD32 && numEltPerGRF<Type_UB>() >= 64)
    {
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
    std::unordered_map<G4_Declare*, std::bitset<32>> emaskRef;
    for (auto bb : fg)
    {
        for (auto inst : *bb)
        {
            if (inst->isSend())
                continue;

            auto dst = inst->getDst();
            if (!dst || !dst->getTopDcl() ||
                dst->getHorzStride() != 1)
                continue;

            if (inst->isWriteEnableInst())
                continue;

            auto regFileKind = dst->getTopDcl()->getRegFile();
            if (regFileKind != G4_RegFileKind::G4_GRF && regFileKind != G4_RegFileKind::G4_INPUT)
                continue;

            if (dst->getTopDcl()->getByteSize() <= dst->getTypeSize() * (unsigned)simdSize)
                continue;

            auto emaskOffStart = inst->getMaskOffset();

            // Reset all bits on first encounter of dcl
            if (emaskRef.find(dst->getTopDcl()) == emaskRef.end())
                emaskRef[dst->getTopDcl()].reset();

            // Set bits based on which EM bits are used in the def
            for (unsigned i = emaskOffStart; i != (emaskOffStart + inst->getExecSize()); i++)
            {
                emaskRef[dst->getTopDcl()].set(i);
            }
        }
    }

    // Check whether any variable's emask usage straddles across lower and upper 16 bits
    for (auto& emRefs : emaskRef)
    {
        auto& bits = emRefs.second;
        auto num = bits.to_ulong();

        // Check whether any lower 16 and upper 16 bits are set
        if (((num & 0xffff) != 0) && ((num & 0xffff0000) != 0))
        {
            channelSliced = false;
            return;
        }
    }

    return;
}

void G4_Kernel::calculateSimdSize()
{
    // Iterate over all instructions in kernel to check
    // whether default execution size of kernel is
    // SIMD8/16. This is required for knowing alignment
    // to use for GRF candidates.

    // only do it once per kernel, as we should not introduce inst with larger simd size than in the input
    if (simdSize.value != 0)
    {
        return;
    }

    // First, get simdsize from attribute (0 : not given)
    // If not 0|8|16|32, wrong value from attribute.
    simdSize = G4_ExecSize((unsigned)m_kernelAttrs->getInt32KernelAttr(Attributes::ATTR_SimdSize));
    if (simdSize != g4::SIMD8 && simdSize != g4::SIMD16 && simdSize != g4::SIMD32)
    {
        assert(simdSize.value == 0 && "vISA: wrong value for SimdSize attribute");
        simdSize = g4::SIMD8;

        for (auto bb : fg)
        {
            for (auto inst : *bb)
            {
                // do not consider send since for certain messages we have to set its execution size
                // to 16 even in simd8 shaders
                if (!inst->isLabel() && !inst->isSend())
                {
                    uint32_t size = inst->getMaskOffset() + inst->getExecSize();
                    if (size > 16)
                    {
                        simdSize = g4::SIMD32;
                        break;
                    }
                    else if (size > 8)
                    {
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
// Updates kernel's related structures based on number of threads.
//
void G4_Kernel::updateKernelByNumThreads(int nThreads)
{
    if (numThreads == nThreads)
        return;

    numThreads = nThreads;

    // Scale number of GRFs, Acc, SWSB tokens.
    setKernelParameters();

    // Update physical register pool
    fg.builder->rebuildPhyRegPool(getNumRegTotal());
}

//
// Evaluate AddrExp/AddrExpList to Imm
//
void G4_Kernel::evalAddrExp()
{
    for (std::list<G4_BB*>::iterator it = fg.begin(), itEnd = fg.end();
        it != itEnd; ++it)
    {
        G4_BB* bb = (*it);

        for (INST_LIST_ITER i = bb->begin(), iEnd = bb->end(); i != iEnd; i++)
        {
            G4_INST* inst = (*i);

            //
            // process each source operand
            //
            for (unsigned j = 0; j < G4_MAX_SRCS; j++)
            {
                G4_Operand* opnd = inst->getSrc(j);

                if (opnd == NULL) continue;

                if (opnd->isAddrExp())
                {
                    int val = opnd->asAddrExp()->eval(*fg.builder);
                    G4_Type ty = opnd->asAddrExp()->getType();

                    G4_Imm* imm = fg.builder->createImm(val, ty);
                    inst->setSrc(imm, j);
                }
            }
        }
    }
}

// FIX: this needs to here because of the above static thread-local variable
extern _THREAD const char* g4_prevFilename;
extern _THREAD int g4_prevSrcLineNo;

static std::vector<std::string> split(
    const std::string & str, const char * delimiter)
{
    std::vector<std::string> v;
    std::string::size_type start = 0;

    for (auto pos = str.find_first_of(delimiter, start);
        pos != std::string::npos;
        start = pos + 1, pos = str.find_first_of(delimiter, start))
    {
        if (pos != start)
        {
            v.emplace_back(str, start, pos - start);
        }
    }

    if (start < str.length())
        v.emplace_back(str, start, str.length() - start);
    return v;
}

static iga_gen_t getIGAPlatform(TARGET_PLATFORM genPlatform)
{
    iga_gen_t platform = IGA_GEN_INVALID;
    switch (genPlatform)
    {
    case GENX_BDW: platform = IGA_GEN8; break;
    case GENX_CHV: platform = IGA_GEN8lp; break;
    case GENX_SKL: platform = IGA_GEN9; break;
    case GENX_BXT: platform = IGA_GEN9lp; break;
    case GENX_ICLLP: platform = IGA_GEN11; break;
    case GENX_TGLLP:platform = IGA_GEN12p1; break;
    case Xe_XeHPSDV: platform = IGA_XE_HP; break;
    case Xe_DG2:
    case Xe_MTL:
        platform = IGA_XE_HPG;
        break;
    case Xe_PVC:
    case Xe_PVCXT:
        platform = IGA_XE_HPC;
        break;
    default:
        break;
    }

    return platform;
}

KernelDebugInfo* G4_Kernel::getKernelDebugInfo()
{
    if (kernelDbgInfo == nullptr)
    {
        kernelDbgInfo = new(fg.mem)KernelDebugInfo();
    }

    return kernelDbgInfo;
}

unsigned G4_Kernel::getStackCallStartReg() const
{
    // Last 3 (or 2) GRFs reserved for stack call purpose
    unsigned totalGRFs = getNumRegTotal();
    unsigned startReg = totalGRFs - numReservedABIGRF();
    return startReg;
}
unsigned G4_Kernel::calleeSaveStart() const
{
    return getCallerSaveLastGRF() + 1;
}
unsigned G4_Kernel::getNumCalleeSaveRegs() const
{
    unsigned totalGRFs = getNumRegTotal();
    return totalGRFs - calleeSaveStart() - numReservedABIGRF();
}

//
// rename non-root declares to their root decl name to make
// it easier to read IR dump
//
void G4_Kernel::renameAliasDeclares()
{
#if _DEBUG
    for (auto dcl : Declares)
    {
        if (dcl->getAliasDeclare())
        {
            uint32_t offset = 0;
            G4_Declare* rootDcl = dcl->getRootDeclare(offset);
            std::string newName(rootDcl->getName());
            if (rootDcl->getElemType() != dcl->getElemType())
            {
                newName += "_";
                newName += TypeSymbol(dcl->getElemType());
            }
            if (offset != 0)
            {
                newName += "_" + std::to_string(offset);
            }
            dcl->setName(fg.builder->getNameString(fg.mem, 64, "%s", newName.c_str()));
        }
    }
#endif
}

//
// perform relocation for every entry in the allocation table
//
void G4_Kernel::doRelocation(void* binary, uint32_t binarySize)
{
    for (auto&& entry : relocationTable)
    {
        entry.doRelocation(*this, binary, binarySize);
    }
}

G4_INST* G4_Kernel::getFirstNonLabelInst() const
{
    for (auto I = fg.cbegin(), E = fg.cend(); I != E; ++I)
    {
        auto bb = *I;
        G4_INST* firstInst = bb->getFirstInst();
        if (firstInst)
        {
            return firstInst;
        }
    }
    // empty kernel
    return nullptr;
}

std::string G4_Kernel::getDebugSrcLine(const std::string& fileName, int srcLine)
{
    auto iter = debugSrcLineMap.find(fileName);
    if (iter == debugSrcLineMap.end())
    {
        std::ifstream ifs(fileName);
        if (!ifs)
        {
            // file doesn't exist
            debugSrcLineMap[fileName] = std::make_pair<bool, std::vector<std::string>>(false, {});
            return "";
        }
        std::string line;
        std::vector<std::string> srcLines;
        while (std::getline(ifs, line))
        {
            srcLines.push_back(line);
        }
        debugSrcLineMap[fileName] = std::make_pair(true, std::move(srcLines));
    }
    iter = debugSrcLineMap.find(fileName);
    if (iter == debugSrcLineMap.end() ||
        !iter->second.first)
    {
        return "";
    }
    auto& lines = iter->second.second;
    if (srcLine > (int) lines.size() || srcLine <= 0)
    {
        return "invalid line number";
    }
    return lines[srcLine - 1];
}

VarSplitPass* G4_Kernel::getVarSplitPass()
{
    if (varSplitPass)
        return varSplitPass;

    varSplitPass = new VarSplitPass(*this);

    return varSplitPass;
}

void G4_Kernel::setKernelParameters()
{
    unsigned overrideGRFNum = 0;
    unsigned overrideNumThreads = 0;

    TARGET_PLATFORM platform = getPlatform();
    overrideGRFNum = m_options->getuInt32Option(vISA_TotalGRFNum);

    overrideNumThreads = m_options->getuInt32Option(vISA_HWThreadNumberPerEU);

    //
    // Number of threads/GRF can currently be set by:
    // 1.- IGC flag (reg key)
    // 2.- Compiler option entered by user for
    //      2.1 entire module
    //      2.2 kernel function
    // 3.- Compiler heuristics
    //
    if (m_options->getuInt32Option(vISA_ForceHWThreadNumberPerEU))
    {
        numThreads = m_options->getuInt32Option(vISA_ForceHWThreadNumberPerEU);
    }
    regSharingHeuristics = m_options->getOption(vISA_RegSharingHeuristics);
    if (overrideNumThreads || regSharingHeuristics)
    {
        overrideGRFNum = 0;
        if (numThreads > 0)
        {
            overrideNumThreads = numThreads;
        }
    }

    // Set the number of GRFs
    if (overrideGRFNum > 0)
    {
        // User-provided number of GRFs
        unsigned Val = m_options->getuInt32Option(vISA_GRFNumToUse);
        if (Val > 0)
        {
            numRegTotal = std::min(Val, overrideGRFNum);
        }
        else
        {
            numRegTotal = overrideGRFNum;
        }
        callerSaveLastGRF = ((overrideGRFNum - 8) / 2) - 1;
    }
    else if (overrideNumThreads > 0)
    {
        switch (platform)
        {
        case Xe_XeHPSDV:
        case Xe_DG2:
        case Xe_MTL:
            switch (overrideNumThreads)
            {
            case 4:
                numRegTotal = 256;
                break;
            default:
                numRegTotal = 128;
            }
            break;
        case Xe_PVC:
        case Xe_PVCXT:
            switch (overrideNumThreads)
            {
            case 4:
                numRegTotal = 256;
                break;
            case 5:
                numRegTotal = 192;
                break;
            case 6:
                numRegTotal = 160;
                break;
            case 8:
                numRegTotal = 128;
                break;
            case 10:
                numRegTotal = 96;
                break;
            case 12:
                numRegTotal = 64;
                break;
            default:
                numRegTotal = 128;
            }
            break;
        default:
            numRegTotal = 128;
        }
        callerSaveLastGRF = ((numRegTotal - 8) / 2) - 1;
    }
    else
    {
        // Default value for all other platforms
        unsigned Val = m_options->getuInt32Option(vISA_GRFNumToUse);
        numRegTotal = Val ? Val : 128;
        callerSaveLastGRF = ((numRegTotal - 8) / 2) - 1;
    }
    // For safety update TotalGRFNum, there may be some uses for this vISA option
    m_options->setOption(vISA_TotalGRFNum, numRegTotal);

    // Set the number of SWSB tokens
    unsigned overrideNumSWSB = m_options->getuInt32Option(vISA_SWSBTokenNum);
    if (overrideNumSWSB > 0)
    {
        // User-provided number of SWSB tokens
        numSWSBTokens = overrideNumSWSB;
    }
    else if (overrideNumThreads > 0)
    {
        switch (platform)
        {
        case Xe_PVC:
        case Xe_PVCXT:
            switch (overrideNumThreads)
            {
            case 4:
                numSWSBTokens = 32;
                break;
            case 5:
                numSWSBTokens = 24;
                break;
            case 6:
                numSWSBTokens = 20;
                break;
            case 8:
                numSWSBTokens = 16;
                break;
            case 10:
                numSWSBTokens = 12;
                break;
            case 12:
                numSWSBTokens = 8;
                break;
            default:
                numSWSBTokens = 16;
            }
            break;
        default:
            numSWSBTokens = 16;
        }
    }
    else
    {
        // Default value based on platform
        switch (platform)
        {
        case Xe_PVC:
        case Xe_PVCXT:
            numSWSBTokens = 16;
            if (numRegTotal == 256)
            {
                numSWSBTokens *= 2;
            }
            break;
        default:
            numSWSBTokens = 16;
        }
    }


    // Set the number of Acc. They are in the unit of GRFs (i.e., 1 accumulator is the same size as 1 GRF)
    unsigned overrideNumAcc = m_options->getuInt32Option(vISA_numGeneralAcc);
    if (overrideNumAcc > 0)
    {
        // User-provided number of Acc
        numAcc = overrideNumAcc;
    }
    else if (overrideNumThreads > 0)
    {
        switch (platform)
        {
        case Xe_XeHPSDV:
        case Xe_DG2:
        case Xe_MTL:
            switch (overrideNumThreads)
            {
            case 4:
                numAcc = 8;
                break;
            default:
                numAcc = 4;
            }
            break;
        case Xe_PVC:
        case Xe_PVCXT:
            switch (overrideNumThreads)
            {
            case 4:
                numAcc = 8;
                break;
            case 5:
                numAcc = 6;
                break;
            case 6:
            case 8:
                numAcc = 4;
                break;
            case 10:
            case 12:
                numAcc = 2;
                break;
            default:
                numAcc = 8;
            }
            break;
        default:
            numAcc = 4;
        }
    }
    else
    {
        // Default value based on platform
        switch (platform)
        {
        case Xe_XeHPSDV:
        case Xe_DG2:
        case Xe_MTL:
        case Xe_PVC:
        case Xe_PVCXT:
            numAcc = 4;
            if (numRegTotal == 256)
            {
                numAcc *= 2;
            }
            break;
        default:
            numAcc = 2;
        }
    }

    // Set number of threads if it was not defined before
    if (numThreads == 0)
    {
        if (overrideNumThreads > 0)
        {
            numThreads = overrideNumThreads;
        }
        else
        {
            switch (platform)
            {
            case Xe_XeHPSDV:
            case Xe_DG2:
            case Xe_MTL:
                switch (numRegTotal)
                {
                case 256:
                    numThreads = 4;
                    break;
                default:
                    numThreads = 8;
                }
                break;
            case Xe_PVC:
            case Xe_PVCXT:
                switch (numRegTotal)
                {
                case 256:
                    numThreads = 4;
                    break;
                case 192:
                    numThreads = 5;
                    break;
                case 160:
                    numThreads = 6;
                    break;
                case 128:
                    numThreads = 8;
                    break;
                case 96:
                    numThreads = 10;
                    break;
                case 64:
                    numThreads = 12;
                    break;
                default:
                    numThreads = 8;
                }
                break;
            default:
                numThreads = 7;
            }
        }
    }

    if (m_options->getOption(vISA_hasDoubleAcc))
    {
        numAcc = 16;
    }
}

void G4_Kernel::dump(std::ostream &os) const
{
    fg.print(os);
}

void G4_Kernel::dumpToFile(const std::string &suffixIn)
{
    bool dumpDot = m_options->getOption(vISA_DumpDot);
    bool dumpG4 =
        m_options->getOption(vISA_DumpPasses) ||
        m_options->getuInt32Option(vISA_DumpPassesSubset) >= 1;
    if (!dumpDot && !dumpG4)
        return;

    // calls to this will produce a sequence of dumps
    // [kernel-name].000.[suffix].{dot,g4}
    // [kernel-name].001.[suffix].{dot,g4}
    // ...
    // If vISA_DumpPassesSubset == 1 then we omit any files that don't change
    // the string representation of the kernel (i.e. skip passes that don't do anything).
    std::stringstream ss;
    ss << (name ? name : "UnknownKernel");
    ss << "." << std::setfill('0') << std::setw(3) << nextDumpIndex++ << "." << suffixIn;
    std::string baseName = sanitizePathString(ss.str());

    if (dumpDot)
        dumpDotFileInternal(baseName);

    if (dumpG4)
        dumpG4Internal(baseName);
}

void G4_Kernel::emitDeviceAsm(
    std::ostream& os, const void * binary, uint32_t binarySize)
{
    //
    // for GTGPU lib release, don't dump out asm
    //
#ifdef NDEBUG
#ifdef GTGPU_LIB
    return;
#endif
#endif
    const bool newAsm =
        m_options->getOption(vISA_dumpNewSyntax) && !(binary == NULL || binarySize == 0);

    if (!m_options->getOption(vISA_StripComments)) {
        emitDeviceAsmHeaderComment(os);
    }

    // Set this to NULL to always print filename for each kernel
    g4_prevFilename = nullptr;
    g4_prevSrcLineNo = 0;

    if (!newAsm) {
        emitDeviceAsmInstructionsOldAsm(os);
        return;
    }

    emitDeviceAsmInstructionsIga(os, binary, binarySize);

    if (getPlatformGeneration() >= PlatformGen::XE) {
        os << "\n\n";
        os << "//.BankConflicts: " <<  fg.XeBCStats.BCNum << "\n";
        os << "//.BankConflicts.SameBank: " <<  fg.XeBCStats.sameBankConflicts << "\n";
        os << "//.BankConflicts.TwoSrc: " <<  fg.XeBCStats.twoSrcBC << "\n";
        int nativeSimdSize = 8;
        if (getPlatform() >= Xe_PVC)
            nativeSimdSize = 16;
        os << "//.SIMD" << 2*nativeSimdSize << "ReadSuppressions: " <<  fg.XeBCStats.simd16ReadSuppression << "\n";
        os << "//.SIMD" << nativeSimdSize << "s: " <<  fg.XeBCStats.simd8 << "\n//\n";
        os << "//.RMWs: " << fg.numRMWs << "\n//\n";
    }
    else
    {
        os << "// Bank Conflict Statistics: \n";
        os << "// -- GOOD: " << fg.BCStats.NumOfGoodInsts << "\n";
        os << "// --  BAD: " << fg.BCStats.NumOfBadInsts << "\n";
        os << "// --   OK: " << fg.BCStats.NumOfOKInsts << "\n";
    }

    os << "//.accSubDef: " << fg.XeBCStats.accSubDef << "\n";
    os << "//.accSubUse: " << fg.XeBCStats.accSubUse << "\n";
    os << "//.accSubCandidateDef: " << fg.XeBCStats.accSubCandidateDef << "\n";
    os << "//.accSubCandidateUse: " << fg.XeBCStats.accSubCandidateUse << "\n";
}

void G4_Kernel::emitRegInfo()
{
    const char* asmName = nullptr;
    getOptions()->getOption(VISA_AsmFileName, asmName);
    const char* asmNameEmpty = "";
    if (!asmName)
    {
        asmName = asmNameEmpty;
    }

    std::string dumpFileName = std::string(asmName) + ".reginfo";
    std::fstream ofile(dumpFileName, std::ios::out);

    emitRegInfoKernel(ofile);

    ofile.close();
}

void G4_Kernel::emitRegInfoKernel(std::ostream& output)
{
    output << "//.platform " << getGenxPlatformString();
    output << "\n" << "//.kernel ID 0x" << std::hex << getKernelID() << "\n";
    output << std::dec << "\n";
    int instOffset = 0;

    for (BB_LIST_ITER itBB = fg.begin(); itBB != fg.end(); ++itBB)
    {
        for (INST_LIST_ITER itInst = (*itBB)->begin(); itInst != (*itBB)->end(); ++itInst)
        {
            G4_INST* inst = (*itInst);
            if (inst->isLabel())
            {
                continue;
            }
            if (inst->getLexicalId() == -1)
            {
                continue;
            }

            (*itBB)->emitRegInfo(output, inst, instOffset);
            instOffset += inst->isCompactedInst() ? 8 : 16;
        }
    }
    return;
}

//
// This routine dumps out the dot file of the control flow graph along with instructions.
// dot is drawing graph tool from AT&T.
//
void G4_Kernel::dumpDotFileInternal(const std::string &baseName)
{
    std::fstream ofile(baseName + ".dot", std::ios::out);
    assert(ofile);
    //
    // write digraph KernelName {"
    //          size = "8, 10";
    //
    const char* asmFileName = NULL;
    m_options->getOption(VISA_AsmFileName, asmFileName);
    if (asmFileName == NULL)
        ofile << "digraph UnknownKernel" << " {" << std::endl;
    else
        ofile << "digraph " << asmFileName << " {" << std::endl;
    //
    // keep the graph width 8, estimate a reasonable graph height
    //
    const unsigned itemPerPage = 64;                                        // 60 instructions per Letter page
    unsigned totalItem = (unsigned)Declares.size();
    for (std::list<G4_BB*>::iterator it = fg.begin(); it != fg.end(); ++it)
        totalItem += ((unsigned)(*it)->size());
    totalItem += (unsigned)fg.size();
    float graphHeight = (float)totalItem / itemPerPage;
    graphHeight = graphHeight < 100.0f ? 100.0f : graphHeight;    // minimal size: Letter
    ofile << "\n\t// Setup\n";
    ofile << "\tsize = \"80.0, " << graphHeight << "\";\n";
    ofile << "\tpage= \"80.5, 110\";\n";
    ofile << "\tpagedir=\"TL\";\n";
    //
    // dump out declare information
    //     Declare [label="
    //
    //if (name == NULL)
    //  ofile << "\tDeclares [shape=record, label=\"{kernel:UnknownKernel" << " | ";
    //else
    //  ofile << "\tDeclares [shape=record, label=\"{kernel:" << name << " | ";
    //for (std::list<G4_Declare*>::iterator it = Declares.begin(); it != Declares.end(); ++it)
    //{
    //  (*it)->emit(ofile, true, Options::symbolReg);   // Solve the DumpDot error on representing <>
    //
    //  ofile << "\\l";  // left adjusted
    //}
    //ofile << "}\"];" << std::endl;
    //
    // dump out flow graph
    //
    for (std::list<G4_BB*>::iterator it = fg.begin(); it != fg.end(); ++it)
    {
        G4_BB* bb = (*it);
        //
        // write:   BB0 [shape=plaintext, label=<
        //                      <TABLE BORDER="0" CELLBORDER="1" CELLSPACING="0">
        //                          <TR><TD ALIGN="CENTER">BB0: TestRA_Dot</TD></TR>
        //                          <TR><TD>
        //                              <TABLE BORDER="0" CELLBORDER="0" CELLSPACING="0">
        //                                  <TR><TD ALIGN="LEFT">TestRA_Dot:</TD></TR>
        //                                  <TR><TD ALIGN="LEFT"><FONT color="red">add (8) Region(0,0)[1] Region(0,0)[8;8,1] PAYLOAD(0,0)[8;8,1] [NoMask]</FONT></TD></TR>
        //                              </TABLE>
        //                          </TD></TR>
        //                      </TABLE>>];
        // print out label if the first inst is a label inst
        //
        ofile << "\t";
        bb->writeBBId(ofile);
        ofile << " [shape=plaintext, label=<" << std::endl;
        ofile << "\t\t\t    <TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\">" << std::endl;
        ofile << "\t\t\t\t<TR><TD ALIGN=\"CENTER\">";
        bb->writeBBId(ofile);
        ofile << ": ";

        if (!bb->empty() && bb->front()->isLabel())
        {
            bb->front()->getSrc(0)->emit(ofile);
        }
        ofile << "</TD></TR>" << std::endl;
        //emit all instructions within basic block
        ofile << "\t\t\t\t<TR><TD>" << std::endl;

        if (!bb->empty())
        {
            ofile << "\t\t\t\t\t    <TABLE BORDER=\"0\" CELLBORDER=\"0\" CELLSPACING=\"0\">" << std::endl;
            for (INST_LIST_ITER i = bb->begin(); i != bb->end(); i++)
            {
                //
                // detect if there is spill code first, set different color for it
                //
                std::string fontColor = "black";
                //
                // emit the instruction
                //
                ofile << "\t\t\t\t\t\t<TR><TD ALIGN=\"LEFT\"><FONT color=\"" << fontColor << "\">";
                std::ostringstream os;
                (*i)->emit(os, m_options->getOption(vISA_SymbolReg), true);
                std::string dotStr(os.str());
                //TODO: dot doesn't like '<', '>', '{', or '}' (and '&') this code below is a hack. need to replace with delimiters.
                //std::replace_if(dotStr.begin(), dotStr.end(), bind2nd(equal_to<char>(), '<'), '[');
                std::replace_if(dotStr.begin(), dotStr.end(), std::bind(std::equal_to<char>(), std::placeholders::_1, '<'), '[');
                std::replace_if(dotStr.begin(), dotStr.end(), std::bind(std::equal_to<char>(), std::placeholders::_1, '>'), ']');
                std::replace_if(dotStr.begin(), dotStr.end(), std::bind(std::equal_to<char>(), std::placeholders::_1, '{'), '[');
                std::replace_if(dotStr.begin(), dotStr.end(), std::bind(std::equal_to<char>(), std::placeholders::_1, '}'), ']');
                std::replace_if(dotStr.begin(), dotStr.end(), std::bind(std::equal_to<char>(), std::placeholders::_1, '&'), '$');
                ofile << dotStr;

                ofile << "</FONT></TD></TR>" << std::endl;
                //ofile << "\\l"; // left adjusted
            }
            ofile << "\t\t\t\t\t    </TABLE>" << std::endl;
        }

        ofile << "\t\t\t\t</TD></TR>" << std::endl;
        ofile << "\t\t\t    </TABLE>>];" << std::endl;
        //
        // dump out succ edges
        // BB12 -> BB10
        //
        for (std::list<G4_BB*>::iterator sit = bb->Succs.begin();
            sit != bb->Succs.end(); ++sit)
        {
            bb->writeBBId(ofile);
            ofile << " -> ";
            (*sit)->writeBBId(ofile);
            ofile << std::endl;
        }
    }
    //
    // write "}" to end digraph
    //
    ofile << std::endl << " }" << std::endl;
    //
    // close dot file
    //
    ofile.close();
}

// Dump the instructions into a .g4 file
void G4_Kernel::dumpG4Internal(const std::string &file)
{
    std::stringstream g4asm;
    dumpG4InternalTo(g4asm);
    std::string g4asms = g4asm.str();
    if (m_options->getuInt32Option(vISA_DumpPassesSubset) == 1 && g4asms == lastG4Asm) {
        return;
    }
    lastG4Asm = std::move(g4asms);

    std::fstream ofile(file + ".g4", std::ios::out);
    assert(ofile);
    dumpG4InternalTo(ofile);
}

void G4_Kernel::dumpG4InternalTo(std::ostream &os)
{
    const char* asmFileName = nullptr;
    m_options->getOption(VISA_AsmFileName, asmFileName);
    os << ".kernel " << name << "\n";

    for (const G4_Declare *d : Declares) {
        static const int MIN_DECL = 34; // skip the built-in decls
        if (d->getDeclId() > MIN_DECL) {
            // os << d->getDeclId() << "\n";
            d->emit(os);
        }
    }

    for (std::list<G4_BB*>::iterator it = fg.begin();
        it != fg.end(); ++it)
    {
        // Emit BB number
        G4_BB* bb = (*it);
        bb->writeBBId(os);

        // Emit BB type
        if (bb->getBBType())
        {
            os << " [" << bb->getBBTypeStr() << "] ";
        }

        os << "\tPreds: ";
        for (auto pred : bb->Preds)
        {
            pred->writeBBId(os);
            os << " ";
        }
        os << "\tSuccs: ";
        for (auto succ : bb->Succs)
        {
            succ->writeBBId(os);
            os << " ";
        }
        os << "\n";

        bb->emit(os);
        os << "\n\n";
    } // bbs
}

void G4_Kernel::emitDeviceAsmHeaderComment(std::ostream& os)
{
    os << "//.kernel ";
    if (name != NULL)
    {
        // some 3D kernels do not have a name
        os << name;
    }

    os << "\n" << "//.platform " << getGenxPlatformString();
    os << "\n" << "//.thread_config " << "numGRF=" << numRegTotal << ", numAcc=" << numAcc;
    if (fg.builder->hasSWSB())
    {
        os << ", numSWSB=" << numSWSBTokens;
    }
    os << "\n" << "//.options_string \"" << m_options->getUserArgString().str() << "\"";
    os << "\n" << "//.full_options \"" << m_options->getFullArgString() << "\"";
    os << "\n" << "//.instCount " << asmInstCount;
    static const char* const RATypeString[] {
        RA_TYPE(STRINGIFY)
    };
    os << "\n//.RA type\t" << RATypeString[RAType];

    if (auto jitInfo = fg.builder->getJitInfo())
    {
        if (jitInfo->numGRFUsed != 0)
        {
            os << "\n" << "//.GRF count " << jitInfo->numGRFUsed;
        }
        if (jitInfo->spillMemUsed > 0)
        {
            os << "\n" << "//.spill size " << jitInfo->spillMemUsed;
        }
        if (jitInfo->numGRFSpillFill > 0)
        {
            os << "\n" << "//.spill GRF est. ref count " << jitInfo->numGRFSpillFill;
        }
        if (jitInfo->numFlagSpillStore > 0)
        {
            os << "\n//.spill flag store " << jitInfo->numFlagSpillStore;
            os << "\n//.spill flag load " << jitInfo->numFlagSpillLoad;
        }
    }

    auto privateMemSize = getInt32KernelAttr(Attributes::ATTR_SpillMemOffset);
    if (privateMemSize != 0)
    {
        os << "\n//.private memory size " << privateMemSize;
    }
    os << "\n\n";

    //Step2: emit declares (as needed)
    //
    // firstly, emit RA declare as comments or code depends on Options::symbolReg
    // we check if the register allocation is successful here
    //

    for (auto dcl : Declares)
    {
        dcl->emit(os);
    }
    os << "\n";

    auto fmtHex = [](int i) {
        std::stringstream ss;
        ss << "0x" << std::hex << std::uppercase << i;
        return ss.str();
    };

    const unsigned inputCount = fg.builder->getInputCount();
    std::vector<std::string> argNames;
    size_t maxNameLen = 8;
    for (unsigned id = 0; id < inputCount; id++)
    {
        const input_info_t* ii = fg.builder->getInputArg(id);
        std::stringstream ss;
        if (ii->dcl && ii->dcl->getName()) {
            ss << ii->dcl->getName();
        } else {
            ss << "__unnamed" << (id + 1);
        }
        argNames.push_back(ss.str());
        maxNameLen = std::max(maxNameLen, argNames.back().size());
    }

    // emit input location and size
    os << "// .inputs\n";
    const size_t COLW_IDENT = maxNameLen;
    static const size_t COLW_TYPE = 8;
    static const size_t COLW_SIZE = 6;
    static const size_t COLW_AT = 8;
    static const size_t COLW_CLASS = 10;

    std::stringstream bordss;
    bordss << "// ";
    bordss << '+'; bordss << std::setfill('-') << std::setw(COLW_IDENT + 2) << "";
    bordss << '+'; bordss << std::setfill('-') << std::setw(COLW_TYPE + 2) << "";
    bordss << '+'; bordss << std::setfill('-') << std::setw(COLW_SIZE + 2) << "";
    bordss << '+'; bordss << std::setfill('-') << std::setw(COLW_AT + 2) << "";
    bordss << '+'; bordss << std::setfill('-') << std::setw(COLW_CLASS + 2) << "";
    bordss << '+' << "\n";
    std::string border = bordss.str();

    os << border;
    os <<
        "//" <<
        " | " << std::left << std::setw(COLW_IDENT) << "id" <<
        " | " << std::left << std::setw(COLW_TYPE) << "type" <<
        " | " << std::right << std::setw(COLW_SIZE) << "bytes" <<
        " | " << std::left << std::setw(COLW_AT) << "at" <<
        " | " << std::left << std::setw(COLW_CLASS) << "class" <<
        " |" << "\n";
    os << border;

    const unsigned grfSize = getGRFSize();
    for (unsigned id = 0; id < inputCount; id++)
    {
        const input_info_t* input_info = fg.builder->getInputArg(id);
        //
        os << "//";
        //
        // id
        os <<
            " | " << std::left << std::setw(COLW_IDENT) << argNames[id];
        //
        // type and length
        //   e.g. :uq x 16
        const G4_Declare *dcl = input_info->dcl;
        std::stringstream sstype;
        if (dcl) {
            switch (dcl->getElemType()) {
            case Type_B: sstype << ":b"; break;
            case Type_W: sstype << ":w"; break;
            case Type_D: sstype << ":d"; break;
            case Type_Q: sstype << ":q"; break;
            case Type_V: sstype << ":v"; break;
            case Type_UB: sstype << ":ub"; break;
            case Type_UW: sstype << ":uw"; break;
            case Type_UD: sstype << ":ud"; break;
            case Type_UQ: sstype << ":uq"; break;
            case Type_UV: sstype << ":uv"; break;
                //
            case Type_F:  sstype << ":f"; break;
            case Type_HF: sstype << ":hf"; break;
            case Type_DF: sstype << ":df"; break;
            case Type_NF: sstype << ":nf"; break;
            case Type_BF: sstype << ":bf"; break;
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
        os << " | " << std::right << std::setw(COLW_SIZE) << std::dec << input_info->size;

        // location
        unsigned reg = input_info->offset / grfSize,
            subRegBytes = input_info->offset % grfSize;
        std::stringstream ssloc;
        ssloc << "r" << reg;
        if (subRegBytes != 0)
            ssloc << "+" << subRegBytes;
        os << " | " << std::left << std::setw(COLW_AT) << ssloc.str();

        // class
        std::string inpcls;
        switch (input_info->getInputClass()) {
        case INPUT_GENERAL: inpcls = "general"; break;
        case INPUT_SAMPLER: inpcls = "sampler"; break;
        case INPUT_SURFACE: inpcls = "surface"; break;
        default: inpcls = fmtHex((int)input_info->getInputClass()); break;
        }
        os << " | " << std::left << std::setw(COLW_CLASS) << inpcls;
        //
        os << " |\n";
    }
    os << border << "\n";

    if (getPlatformGeneration() < PlatformGen::XE)
    {
        fg.BCStats.clear();
    }
    else
    {
        fg.XeBCStats.clear();
    }
    fg.numRMWs = 0;
}


static std::map<int, std::string> parseDecodeErrors(
    KernelView &kView, const char *errBuf, size_t errBufSize)
{
    // FIXME: IGA KernelView should be refactored to just return PC's
    // paired with diagnostic strings for each
    // (automatically allocate in IGA and cleanup when KV is deleted)
    bool dissasemblyFailed = !kView.decodeSucceeded();
    std::string igaErrMsgs;
    std::vector<std::string> igaErrMsgsVector;
    std::map<int, std::string> errorToStringMap;
    if (dissasemblyFailed)
    {
        std::cerr << "failed to decode binary for asm output";
        igaErrMsgs = errBuf;
        igaErrMsgsVector = split(igaErrMsgs, "\n");
        for (auto msg : igaErrMsgsVector)
        {
            auto pos = msg.find("ERROR");
            if (pos != std::string::npos)
            {
                std::cerr << msg << "\n";
                std::vector<std::string> aString = split(msg, " ");
                for (auto token : aString)
                {
                    if (token.find_first_of("0123456789") != std::string::npos)
                    {
                        int errorPC = std::atoi(token.c_str());
                        errorToStringMap[errorPC] = msg;
                        break;
                    }
                }
            }
        }
    }

    return errorToStringMap;
}

using BlockOffsets = std::map<int32_t,std::vector<std::string>>;

static BlockOffsets precomputeBlockOffsets(
    std::ostream& os, G4_Kernel &g4k, const KernelView &kv)
{
    // pre-compute the PCs of each basic block
    int32_t currPc = 0, lastInstSize = -1;
    std::map<int32_t,std::vector<std::string>> blockOffsets;
    for (BB_LIST_ITER itBB = g4k.fg.begin(); itBB != g4k.fg.end(); ++itBB) {
        for (INST_LIST_ITER itInst = (*itBB)->begin(); itInst != (*itBB)->end(); ++itInst) {
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
void G4_Kernel::emitDeviceAsmInstructionsIga(
    std::ostream& os, const void * binary, uint32_t binarySize)
{
    os << "\n";

    const size_t ERROR_STRING_MAX_LENGTH = 16 * 1024;
    char* errBuf = new char[ERROR_STRING_MAX_LENGTH];
    assert(errBuf);
    if (!errBuf)
        return;
    TARGET_PLATFORM p = getPlatform();
    KernelView kv(
        getIGAPlatform(p), binary, binarySize,
        GetIGASWSBEncodeMode(*fg.builder),
        errBuf, ERROR_STRING_MAX_LENGTH);
    const auto errorMap =
        parseDecodeErrors(kv, errBuf, ERROR_STRING_MAX_LENGTH);
    delete [] errBuf;

    const auto blockOffsets = precomputeBlockOffsets(os, *this, kv);

    //
    // Generate a label with uniqueLabel as prefix (required by some tools).
    // We do so by using labeler callback.  If uniqueLabels is not present, use iga's
    // default label.  For example,
    //   Without option -uniqueLabels:
    //      generating default label,   L1234
    //   With option -uniqueLabels <sth>:
    //      generating label with <sth> as prefix, <sth>_L1234
    //
    std::string labelPrefix;
    if (m_options->getOption(vISA_UniqueLabels))
    {
        const char* labelPrefixC = nullptr;
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
        LabelerState(
            const KernelView *_kv,
            const BlockOffsets &offs,
            const std::string &lblPfx)
            : kv(_kv), blockOffsets(offs), labelPrefix(lblPfx)
        {
        }
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
    for (int i = 0; i < 3; i++)
    {
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

    // printedLabels - tracked the labels those have been printed to the pc to avoid
    // printing the same label twice at the same pc. This can happen when there's an empty
    // BB contains only labels. The BB and the following BB will both print those labels.
    // The pair is the pc to label name pair.
    std::set<std::pair<int32_t, std::string>> printedLabels;
    // tryPrintLable - check if the given label is already printed with the given pc. Print it
    // if not, and skip it if yes.
    auto tryPrintLabel = [&os, &printedLabels](int32_t label_pc, std::string label_name) {
        auto label_pair = std::make_pair(label_pc, label_name);
        // skip if the same label in the set
        if (printedLabels.find(label_pair) != printedLabels.end())
            return;
        os << label_name << ":\n";
        printedLabels.insert(label_pair);
    };

    for (BB_LIST_ITER itBB = fg.begin(); itBB != fg.end(); ++itBB) {
        os << "// "; (*itBB)->emitBbInfo(os); os << "\n";
        for (INST_LIST_ITER itInst = (*itBB)->begin();
            itInst != (*itBB)->end(); ++itInst)
        {
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

            auto eitr = errorMap.find(pc);
            if (eitr != errorMap.end()) {
                os << "// " << eitr->second << "\n";
                os << "// text representation might not be correct";
            }

            static const uint32_t IGA_FMT_OPTS =
                getOption(vISA_PrintHexFloatInAsm) ? IGA_FORMATTING_OPT_PRINT_HEX_FLOATS : IGA_FORMATTING_OPTS_DEFAULT
                | IGA_FORMATTING_OPT_PRINT_LDST
                | IGA_FORMATTING_OPT_PRINT_BFNEXPRS;
            while (true) {
                size_t nw = kv.getInstSyntax(
                    pc,
                    igaStringBuffer.data(), igaStringBuffer.size(),
                    IGA_FMT_OPTS,
                    labeler, &ls);
                if (nw == 0) {
                    os << "<<error formatting instruction at PC " << pc << ">>\n";
                    break;
                } else if (nw <= igaStringBuffer.size()) {
                    // print it (pad it out so comments line up on most instructions)
                    std::string line =igaStringBuffer.data();
                    while (line.size() < 100)
                        line += ' ';
                    os << line;
                    break;
                } else {
                    igaStringBuffer.resize(igaStringBuffer.size() + 512);
                    // try again
                }
            }

            (*itBB)->emitBasicInstructionComment(os, itInst, suppressRegs, lastRegs);
            os << "\n";

            pc += kv.getInstSize(pc);
        } // for insts in block
    } // for blocks
} // emitDeviceAsmInstructionsIga


// Should be removed once we can confirm no one uses it
// the output comes from G4_INST::... and almost certainly won't be
// parsable by IGA
void G4_Kernel::emitDeviceAsmInstructionsOldAsm(std::ostream& os)
{
    os << std::endl << ".code";
    for (BB_LIST_ITER it = fg.begin(); it != fg.end(); ++it)
    {
        os << "\n";
        (*it)->emit(os);
    }
    //Step4: emit clean-up.
    os << std::endl;
    os << ".end_code" << std::endl;
    os << ".end_kernel" << std::endl;
    os << std::endl;
}

G4_BB* G4_Kernel::getNextBB(G4_BB* bb) const
{
    if (!bb)
        return nullptr;

    // Return the lexically following bb.
    G4_BB* nextBB = nullptr;
    for (auto it = fg.cbegin(), ie = fg.cend(); it != ie; it++)
    {
        auto curBB = (*it);
        if (curBB == bb)
        {
            if (it != ie)
            {
                it++;
                nextBB = (*it);
            }
            break;
        }
    }

    return nextBB;
}

unsigned G4_Kernel::getBinOffsetOfBB(G4_BB* bb) const {
    if (!bb)
        return 0;

    // Given a bb, return the binary offset of first non-label of instruction.
    auto it = std::find_if(bb->begin(), bb->end(), [](G4_INST* inst) { return !inst->isLabel(); });
    assert(it != bb->end() && "expect at least one non-label inst in second BB");
    return (unsigned)(*it)->getGenOffset();
}

unsigned G4_Kernel::getPerThreadNextOff() const
{
    if (!hasPerThreadPayloadBB())
        return 0;
    G4_BB* next = getNextBB(perThreadPayloadBB);
    return getBinOffsetOfBB(next);
}

unsigned G4_Kernel::getCrossThreadNextOff() const
{
    if (!hasCrossThreadPayloadBB())
        return 0;
    G4_BB* next = getNextBB(crossThreadPayloadBB);
    return getBinOffsetOfBB(next);
}

unsigned G4_Kernel::getComputeFFIDGPNextOff() const
{
    if (!hasComputeFFIDProlog())
        return 0;
    // return the offset of the second entry (GP1)
    // the first instruction in the second BB is the start of the second entry
    assert(fg.getNumBB() > 1 && "expect at least one prolog BB");
    assert(!computeFFIDGP1->empty() && !computeFFIDGP1->front()->isLabel());
    return getBinOffsetOfBB(computeFFIDGP1);
}

unsigned G4_Kernel::getComputeFFIDGP1NextOff() const
{
    if (!hasComputeFFIDProlog())
        return 0;
    // return the offset of the BB next to GP1
    // the first instruction in the second BB is the start of the second entry
    assert(fg.getNumBB() > 1 && "expect at least one prolog BB");
    G4_BB* next = getNextBB(computeFFIDGP1);
    return getBinOffsetOfBB(next);
}
