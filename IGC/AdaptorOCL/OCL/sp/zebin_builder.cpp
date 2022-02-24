/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "zebin_builder.hpp"

#include "../../../Compiler/CodeGenPublic.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

#include <string>

using namespace IGC;
using namespace iOpenCL;
using namespace zebin;
using namespace CLElfLib;   // ElfReader related typedefs
using namespace llvm;

ZEBinaryBuilder::ZEBinaryBuilder(
    const PLATFORM plat, bool is64BitPointer, const IGC::SOpenCLProgramInfo& programInfo,
    const uint8_t* spvData, uint32_t spvSize)
    : mPlatform(plat), mBuilder(is64BitPointer)
{
    G6HWC::InitializeCapsGen8(&mHWCaps);

    // FIXME: Most fields leaves as 0
    TargetMetadata metadata;
    metadata.generatorSpecificFlags = TargetMetadata::GeneratorSpecificFlags::NONE;
    metadata.minHwRevisionId = plat.usRevId;
    metadata.maxHwRevisionId = plat.usRevId;
    metadata.generatorId = TargetMetadata::GeneratorId::IGC;
    mBuilder.setTargetMetadata(metadata);

    addProgramScopeInfo(programInfo);

    if (spvData != nullptr)
        addSPIRV(spvData, spvSize);
}

void ZEBinaryBuilder::setProductFamily(PRODUCT_FAMILY value)
{
    mBuilder.setProductFamily(value);
}

void ZEBinaryBuilder::setGfxCoreFamily(GFXCORE_FAMILY value)
{
    mBuilder.setGfxCoreFamily(value);
}

void ZEBinaryBuilder::createKernel(
    const char*  rawIsaBinary,
    unsigned int rawIsaBinarySize,
    const SOpenCLKernelInfo& annotations,
    const uint32_t grfSize,
    const CBTILayout& layout,
    const std::string& visaasm,
    bool isProgramDebuggable)
{
    ZEELFObjectBuilder::SectionID textID =
        addKernelBinary(annotations.m_kernelName, rawIsaBinary, rawIsaBinarySize);
    addKernelSymbols(textID, annotations);
    addKernelRelocations(textID, annotations);

    zeInfoKernel& zeKernel = mZEInfoBuilder.createKernel(annotations.m_kernelName);
    addKernelExecEnv(annotations, zeKernel);
    addKernelExperimentalProperties(annotations, zeKernel);
    if (annotations.m_threadPayload.HasLocalIDx ||
        annotations.m_threadPayload.HasLocalIDy ||
        annotations.m_threadPayload.HasLocalIDz) {
        addLocalIds(annotations.m_executionEnivronment.CompiledSIMDSize,
            grfSize,
            annotations.m_threadPayload.HasLocalIDx,
            annotations.m_threadPayload.HasLocalIDy,
            annotations.m_threadPayload.HasLocalIDz,
            zeKernel);
    }
    addPayloadArgsAndBTI(annotations, zeKernel);
    addMemoryBuffer(annotations, zeKernel);
    addGTPinInfo(annotations);
    if (!visaasm.empty())
        addKernelVISAAsm(annotations.m_kernelName, visaasm);
    if (isProgramDebuggable)
        addKernelDebugEnv(annotations, layout, zeKernel);
}

void ZEBinaryBuilder::addGlobalHostAccessInfo(const SOpenCLProgramInfo& annotations)
{
    for (auto& info : annotations.m_zebinGlobalHostAccessTable)
    {
        mZEInfoBuilder.addGlobalHostAccessSymbol(info.device_name, info.host_name);
    }
}

void ZEBinaryBuilder::addGTPinInfo(const IGC::SOpenCLKernelInfo& annotations)
{
    const IGC::SKernelProgram* program = &(annotations.m_kernelProgram);
    const SProgramOutput* output = nullptr;
    switch (annotations.m_executionEnivronment.CompiledSIMDSize) {
    case 1:  output = &(program->simd1); break;
    case 8:  output = &(program->simd8); break;
    case 16: output = &(program->simd16); break;
    case 32: output = &(program->simd32); break;
    default: IGC_ASSERT(output != nullptr); break;
    }

    uint8_t* buffer = (uint8_t*)output->m_gtpinBuffer;
    uint32_t size = output->m_gtpinBufferSize;
    if (buffer != nullptr && size)
        mBuilder.addSectionGTPinInfo(annotations.m_kernelName, buffer, size);
    for (auto& funcGTPin : output->m_FuncGTPinInfoList) {
        buffer = (uint8_t*)funcGTPin.buffer;
        size = funcGTPin.bufferSize;
        if (buffer != nullptr && size)
            mBuilder.addSectionGTPinInfo(funcGTPin.name, buffer, size);
    }
}

void ZEBinaryBuilder::addProgramScopeInfo(const IGC::SOpenCLProgramInfo& programInfo)
{
    addGlobalConstants(programInfo);
    addGlobals(programInfo);
    addRuntimeSymbols();
    addProgramSymbols(programInfo);
    addProgramRelocations(programInfo);
    addGlobalHostAccessInfo(programInfo);
}

void ZEBinaryBuilder::addGlobalConstants(const IGC::SOpenCLProgramInfo& annotations)
{
    // General constants: .data.const and .bss.const
    // create a data section for global constant variables
    if (annotations.m_initConstantAnnotation && annotations.m_initConstantAnnotation->AllocSize) {
        auto& ca = annotations.m_initConstantAnnotation;
        // the normal .data.const size
        uint32_t dataSize = ca->InlineData.size();
        // the zero-initialize variables size, the .bss.const size
        uint32_t bssSize = ca->AllocSize - dataSize;
        uint32_t alignment = ca->Alignment;

        if (IGC_IS_FLAG_ENABLED(AllocateZeroInitializedVarsInBss)) {
            zebin::ZEELFObjectBuilder::SectionID normal_id = -1, bss_id = -1;
            if (dataSize) {
                // if the bss section existed, we leave the alignment in bss section.
                // that in our design the entire global buffer is the size of normal section (.const) plus bss section
                // we do not want to add the alignment twice on the both sections
                // Alos set the padding size to 0 that we always put the padding into bss section
                uint32_t normal_alignment = bssSize ? 0 : alignment;
                normal_id = mBuilder.addSectionData("const", (const uint8_t*)ca->InlineData.data(),
                    dataSize, 0, normal_alignment, /*rodata*/true);
            }
            if (bssSize) {
                bss_id = mBuilder.addSectionBss("const", bssSize, alignment);
            }

            // set mGlobalConstSectID to normal_id if existed, and bss_id if not.
            // mGlobalConstSectID will be used for symbol section reference. We always refer to normal_id section
            // even if the the symbol is defeind in bss section when normal_id section exists
            mGlobalConstSectID = dataSize ? normal_id : bss_id;
        } else {
            // before runtime can support bss section, we create all 0s in .const.data section by adding
            // bssSize of padding
            mGlobalConstSectID = mBuilder.addSectionData("const", (const uint8_t*)ca->InlineData.data(),
                dataSize, bssSize, alignment, /*rodata*/true);
        }
    }

    // String literals for printf: .data.const.string
    if (annotations.m_initConstantStringAnnotation &&
        annotations.m_initConstantStringAnnotation->AllocSize) {
        auto& caString = annotations.m_initConstantStringAnnotation;
        uint32_t dataSize = caString->InlineData.size();
        uint32_t paddingSize = caString->AllocSize - dataSize;
        uint32_t alignment = caString->Alignment;
        mConstStringSectID = mBuilder.addSectionData("const.string", (const uint8_t*)caString->InlineData.data(),
            dataSize, paddingSize, alignment, /*rodata*/true);
    }
}

void ZEBinaryBuilder::addGlobals(const IGC::SOpenCLProgramInfo& annotations)
{
    if (annotations.m_initGlobalAnnotation == nullptr)
        return;

    // create a data section for global variables
    auto& ca = annotations.m_initGlobalAnnotation;

    if (!ca->AllocSize)
        return;

    uint32_t dataSize = ca->InlineData.size();
    uint32_t bssSize = ca->AllocSize - dataSize;
    uint32_t alignment = ca->Alignment;

    if (IGC_IS_FLAG_ENABLED(AllocateZeroInitializedVarsInBss)) {
        // The .bss.global section size is the bssSize (ca->AllocSize - ca->InlineData.size()),
        // and the normal .data.global size is dataSize (ca->InlineData.size())
        zebin::ZEELFObjectBuilder::SectionID normal_id = -1, bss_id = -1;
        if (dataSize) {
            uint32_t normal_alignment = bssSize ? 0 : alignment;
            normal_id = mBuilder.addSectionData("global", (const uint8_t*)ca->InlineData.data(),
                dataSize, 0, normal_alignment, /*rodata*/false);
        }
        if (bssSize) {
            bss_id = mBuilder.addSectionBss("global", bssSize, alignment);
        }
        // mGlobalSectID is the section id that will be referenced by global symbols.
        // It should be .data.global if existed. If there's only .bss.global section, then all global
        // symbols reference to .bss.global section, so set the mGlobalConstSectID to it
        mGlobalSectID = dataSize ? normal_id : bss_id;
    } else {
        // before runtime can support bss section, we create all 0s in .global.data section by adding
        // bssSize of padding
        mGlobalSectID = mBuilder.addSectionData("global", (const uint8_t*)ca->InlineData.data(),
            dataSize, bssSize, alignment, /*rodata*/false);
    }
}

void ZEBinaryBuilder::addSPIRV(const uint8_t* data, uint32_t size)
{
    mBuilder.addSectionSpirv("", data, size);
}

ZEELFObjectBuilder::SectionID ZEBinaryBuilder::addKernelBinary(const std::string& kernelName,
    const char* kernelBinary, unsigned int kernelBinarySize)
{
    return mBuilder.addSectionText(kernelName, (const uint8_t*)kernelBinary,
        kernelBinarySize, mHWCaps.InstructionCachePrefetchSize, sizeof(DWORD));
}

void ZEBinaryBuilder::addPayloadArgsAndBTI(
    const SOpenCLKernelInfo& annotations,
    zeInfoKernel& zeinfoKernel)
{
    // copy the payload arguments into zeinfoKernel
    zeinfoKernel.payload_arguments.insert(
        zeinfoKernel.payload_arguments.end(),
        annotations.m_zePayloadArgs.begin(),
        annotations.m_zePayloadArgs.end());

    // copy the bit table into zeinfoKernel
    zeinfoKernel.binding_table_indices.insert(
        zeinfoKernel.binding_table_indices.end(),
        annotations.m_zeBTIArgs.begin(),
        annotations.m_zeBTIArgs.end());
}

void ZEBinaryBuilder::addMemoryBuffer(
    const IGC::SOpenCLKernelInfo& annotations,
    zebin::zeInfoKernel& zeinfoKernel)
{
    // scracth0 is either
    //  - contains privates and both igc and vISA stack, or
    //  - contains only vISA stack
    uint32_t scratch0 =
        annotations.m_executionEnivronment.PerThreadScratchSpace;
    // scratch1 is privates on stack
    uint32_t scratch1 =
        annotations.m_executionEnivronment.PerThreadScratchSpaceSlot1;
    // private_on_global: privates and IGC stack on stateless
    uint32_t private_on_global =
        annotations.m_executionEnivronment.PerThreadPrivateOnStatelessSize;

    //  single scratch space have everything
    if (scratch0 && !scratch1 && !private_on_global) {
        ZEInfoBuilder::addScratchPerThreadMemoryBuffer(zeinfoKernel.per_thread_memory_buffers,
            PreDefinedAttrGetter::MemBufferUsage::single_space,
            0,
            scratch0
        );
        return;
    }

    if (scratch0)
        ZEInfoBuilder::addScratchPerThreadMemoryBuffer(zeinfoKernel.per_thread_memory_buffers,
            PreDefinedAttrGetter::MemBufferUsage::spill_fill_space,
            0,
            scratch0);
    if (scratch1)
        ZEInfoBuilder::addScratchPerThreadMemoryBuffer(zeinfoKernel.per_thread_memory_buffers,
            PreDefinedAttrGetter::MemBufferUsage::private_space,
            1,
            scratch1);
    if (private_on_global) {
        ZEInfoBuilder::addPerSIMTThreadGlobalMemoryBuffer(zeinfoKernel.per_thread_memory_buffers,
            PreDefinedAttrGetter::MemBufferUsage::private_space,
            private_on_global);
        // FIXME: IGC currently generate global buffer with size assume to be per-simt-thread
        // ZEInfoBuilder::addPerThreadMemoryBuffer(zeinfoKernel.per_thread_memory_buffers,
        //    PreDefinedAttrGetter::MemBufferType::global,
        //    PreDefinedAttrGetter::MemBufferUsage::private_space,
        //    private_on_global);
    }
}

uint8_t ZEBinaryBuilder::getSymbolElfType(const vISA::ZESymEntry& sym)
{
    switch (sym.s_type) {
    case vISA::GenSymType::S_NOTYPE:
        return llvm::ELF::STT_NOTYPE;

    case vISA::GenSymType::S_UNDEF:
        return llvm::ELF::STT_NOTYPE;

    case vISA::GenSymType::S_FUNC:
    case vISA::GenSymType::S_KERNEL:
        return llvm::ELF::STT_FUNC;

    case vISA::GenSymType::S_GLOBAL_VAR:
    case vISA::GenSymType::S_GLOBAL_VAR_CONST:
    case vISA::GenSymType::S_CONST_SAMPLER:
        return llvm::ELF::STT_OBJECT;
    default:
        break;
    }
    return llvm::ELF::STT_NOTYPE;
}

void ZEBinaryBuilder::addSymbol(const vISA::ZESymEntry& sym, uint8_t binding,
    ZEELFObjectBuilder::SectionID targetSect)
{
    if (sym.s_type == vISA::GenSymType::S_UNDEF)
        targetSect = -1;
    mBuilder.addSymbol(sym.s_name, sym.s_offset, sym.s_size, binding,
        getSymbolElfType(sym), targetSect);
}

void ZEBinaryBuilder::addRuntimeSymbols()
{
    if (IGC_IS_FLAG_ENABLED(EnableGlobalStateBuffer))
        mBuilder.addSymbol("INTEL_PATCH_CROSS_THREAD_OFFSET_OFF_R0", /*addr*/0, /*size*/0,
            llvm::ELF::STB_GLOBAL, llvm::ELF::STT_NOTYPE, /*sectionId*/-1);
}

void ZEBinaryBuilder::addProgramSymbols(const IGC::SOpenCLProgramInfo& annotations)
{
    const IGC::SOpenCLProgramInfo::ZEBinProgramSymbolTable& symbols = annotations.m_zebinSymbolTable;

    // add symbols defined in global constant section
    IGC_ASSERT(symbols.globalConst.empty() || mGlobalConstSectID != -1);
    for (auto sym : symbols.globalConst)
        addSymbol(sym, llvm::ELF::STB_GLOBAL, mGlobalConstSectID);

    // add symbols defined in global string constant section
    IGC_ASSERT(symbols.globalStringConst.empty() || mConstStringSectID != -1);
    for (auto sym : symbols.globalStringConst)
        addSymbol(sym, llvm::ELF::STB_GLOBAL, mConstStringSectID);

    // add symbols defined in global section
    IGC_ASSERT(symbols.global.empty() || mGlobalSectID != -1);
    for (auto sym : symbols.global)
        addSymbol(sym, llvm::ELF::STB_GLOBAL, mGlobalSectID);

}

void ZEBinaryBuilder::addKernelSymbols(
    ZEELFObjectBuilder::SectionID kernelSectId,
    const IGC::SOpenCLKernelInfo& annotations)
{
    // get symbol list from the current process SKernelProgram
    auto symbols = [](int simdSize, const IGC::SKernelProgram& program) {
        if (simdSize == 8)
            return program.simd8.m_symbols;
        else if (simdSize == 16)
            return program.simd16.m_symbols;
        else if (simdSize == 32)
            return program.simd32.m_symbols;
        else
            return program.simd1.m_symbols;
    } (annotations.m_executionEnivronment.CompiledSIMDSize,
        annotations.m_kernelProgram);

    // add local symbols of this kernel binary
    for (auto sym : symbols.local) {
        IGC_ASSERT(sym.s_type != vISA::GenSymType::S_UNDEF);
        addSymbol(sym, llvm::ELF::STB_LOCAL, kernelSectId);
    }

    // add function symbols defined in kernel text
    for (auto sym : symbols.function)
        addSymbol(sym, llvm::ELF::STB_GLOBAL, kernelSectId);

    // we do not support sampler symbols now
    IGC_ASSERT(symbols.sampler.empty());
}

void ZEBinaryBuilder::addProgramRelocations(const IGC::SOpenCLProgramInfo& annotations)
{
    const IGC::SOpenCLProgramInfo::ZEBinRelocTable& relocs = annotations.m_GlobalPointerAddressRelocAnnotation;

    // FIXME: For r_type, zebin::R_TYPE_ZEBIN should have the same enum value as visa::GenRelocType.
    // Take the value directly
    IGC_ASSERT(relocs.globalConstReloc.empty() || mGlobalConstSectID != -1);
    for (auto reloc : relocs.globalConstReloc)
        mBuilder.addRelRelocation(reloc.r_offset, reloc.r_symbol, static_cast<zebin::R_TYPE_ZEBIN>(reloc.r_type), mGlobalConstSectID);

    IGC_ASSERT(relocs.globalReloc.empty() || mGlobalSectID != -1);
    for (auto reloc : relocs.globalReloc)
        mBuilder.addRelRelocation(reloc.r_offset, reloc.r_symbol, static_cast<zebin::R_TYPE_ZEBIN>(reloc.r_type), mGlobalSectID);
}

void ZEBinaryBuilder::addKernelRelocations(
    ZEELFObjectBuilder::SectionID targetId,
    const IGC::SOpenCLKernelInfo& annotations)
{
    // get relocation list from the current process SKernelProgram
    auto relocs = [](int simdSize, const IGC::SKernelProgram& program) {
        if (simdSize == 8)
            return program.simd8.m_relocs;
        else if (simdSize == 16)
            return program.simd16.m_relocs;
        else if (simdSize == 32)
            return program.simd32.m_relocs;
        else
            return program.simd1.m_relocs;
    } (annotations.m_executionEnivronment.CompiledSIMDSize, annotations.m_kernelProgram);

    // FIXME: For r_type, zebin::R_TYPE_ZEBIN should have the same enum value as visa::GenRelocType.
    // Take the value directly
    if (!relocs.empty())
        for (auto reloc : relocs)
            mBuilder.addRelRelocation(reloc.r_offset, reloc.r_symbol, (zebin::R_TYPE_ZEBIN)reloc.r_type, targetId);
}

void ZEBinaryBuilder::addKernelExperimentalProperties(const SOpenCLKernelInfo& annotations,
    zeInfoKernel& zeinfoKernel)
{
    // Write to zeinfoKernel only when the attribute is enabled
    if (IGC_IS_FLAG_ENABLED(DumpHasNonKernelArgLdSt)) {
        ZEInfoBuilder::addExpPropertiesHasNonKernelArgLdSt(zeinfoKernel,
            annotations.m_hasNonKernelArgLoad,
            annotations.m_hasNonKernelArgStore,
            annotations.m_hasNonKernelArgAtomic);
    }
}

void ZEBinaryBuilder::addKernelExecEnv(const SOpenCLKernelInfo& annotations,
    zeInfoKernel& zeinfoKernel)
{
    zeInfoExecutionEnv& env = zeinfoKernel.execution_env;

    env.barrier_count = annotations.m_executionEnivronment.HasBarriers;
    env.disable_mid_thread_preemption = annotations.m_executionEnivronment.DisableMidThreadPreemption;
    env.grf_count = annotations.m_executionEnivronment.NumGRFRequired;
    env.has_4gb_buffers = annotations.m_executionEnivronment.CompiledForGreaterThan4GBBuffers;
    env.has_device_enqueue = annotations.m_executionEnivronment.HasDeviceEnqueue;
    env.has_dpas = annotations.m_executionEnivronment.HasDPAS;
    env.has_fence_for_image_access = annotations.m_executionEnivronment.HasReadWriteImages;
    env.has_global_atomics = annotations.m_executionEnivronment.HasGlobalAtomics;
    env.has_multi_scratch_spaces = CPlatform(mPlatform).hasScratchSurface() && IGC_IS_FLAG_ENABLED(SeparateSpillPvtScratchSpace);
    env.has_no_stateless_write = (annotations.m_executionEnivronment.StatelessWritesCount == 0);
    env.has_stack_calls = annotations.m_executionEnivronment.HasStackCalls;
    env.require_disable_eufusion = annotations.m_executionEnivronment.RequireDisableEUFusion;
    env.inline_data_payload_size = annotations.m_threadPayload.PassInlineDataSize;
    env.offset_to_skip_per_thread_data_load = annotations.m_threadPayload.OffsetToSkipPerThreadDataLoad;;
    env.offset_to_skip_set_ffid_gp = annotations.m_threadPayload.OffsetToSkipSetFFIDGP;

    // extract required_sub_group_size from kernel attribute list
    // it will be in the format of "intel_reqd_sub_group_size(16)"
    const std::string pat = "intel_reqd_sub_group_size(";
    const std::string& attrs = annotations.m_kernelAttributeInfo;
    size_t p1 = attrs.find(pat);
    if (p1 != std::string::npos) {
        p1 += pat.size();
        size_t p2 = attrs.find(')', p1);
        IGC_ASSERT(p2 != std::string::npos && p1 < p2);
        env.required_sub_group_size = std::stoul(attrs.substr(p1, p2 - p1));
    }

    if(annotations.m_executionEnivronment.HasFixedWorkGroupSize)
    {
        env.required_work_group_size.push_back(annotations.m_executionEnivronment.FixedWorkgroupSize[0]);
        env.required_work_group_size.push_back(annotations.m_executionEnivronment.FixedWorkgroupSize[1]);
        env.required_work_group_size.push_back(annotations.m_executionEnivronment.FixedWorkgroupSize[2]);
    }
    env.simd_size = annotations.m_executionEnivronment.CompiledSIMDSize;
    // set slm size to inline local size
    env.slm_size = annotations.m_executionEnivronment.SumFixedTGSMSizes ;
    env.subgroup_independent_forward_progress = annotations.m_executionEnivronment.SubgroupIndependentForwardProgressRequired;
    if (annotations.m_executionEnivronment.WorkgroupWalkOrder[0] ||
        annotations.m_executionEnivronment.WorkgroupWalkOrder[1] ||
        annotations.m_executionEnivronment.WorkgroupWalkOrder[2]) {
        env.work_group_walk_order_dimensions.push_back(annotations.m_executionEnivronment.WorkgroupWalkOrder[0]);
        env.work_group_walk_order_dimensions.push_back(annotations.m_executionEnivronment.WorkgroupWalkOrder[1]);
        env.work_group_walk_order_dimensions.push_back(annotations.m_executionEnivronment.WorkgroupWalkOrder[2]);
    }
}

void ZEBinaryBuilder::addLocalIds(uint32_t simdSize, uint32_t grfSize,
    bool has_local_id_x, bool has_local_id_y, bool has_local_id_z,
    zebin::zeInfoKernel& zeinfoKernel)
{
    // simdSize 1 is CM kernel, using arg_type::packed_local_ids format
    if (simdSize == 1) {
        // Currently there's only one kind of per-thread argument, hard-coded the
        // offset to 0 and for packed_local_ids, its size is 6 bytes (int16*3) always
        mZEInfoBuilder.addPerThreadPayloadArgument(
            zeinfoKernel.per_thread_payload_arguments,
            PreDefinedAttrGetter::ArgType::packed_local_ids, 0, 6);
        return;
    }
    // otherwise, using arg_type::local_id format
    IGC_ASSERT(simdSize);
    IGC_ASSERT(grfSize);
    // each id takes 2 bytes
    int32_t per_id_size = 2 * simdSize;
    // byte size for one id have to be grf align
    per_id_size = (per_id_size % grfSize) == 0 ?
        per_id_size : ((per_id_size / grfSize) + 1) * grfSize;
    // total_size = num_of_ids * per_id_size
    int32_t total_size = per_id_size * ((has_local_id_x ? 1 : 0) +
        (has_local_id_y ? 1 : 0) + (has_local_id_z ? 1 : 0));
    mZEInfoBuilder.addPerThreadPayloadArgument(
        zeinfoKernel.per_thread_payload_arguments,
        PreDefinedAttrGetter::ArgType::local_id, 0, total_size);
}

// Calculate correct (pure) size of ELF binary, because debugDataSize taken from pOutput->m_debugDataVISASize
// contains something else.
// If ELF is validated successfully then return a calculated size. Othwerwise, return 0.
size_t ZEBinaryBuilder::calcElfSize(void* elfBin, size_t elfSize)
{
    SElf64Header* elf64Header = (SElf64Header*)elfBin;
    size_t elfBinSize = 0; // Correct (pure) size of ELF binary to be calculated

    if (elfSize == 0)
    {
        IGC_ASSERT_MESSAGE(false, "Empty ELF file - nothing to be transfered to zeBinary");
        return 0; // ELF binary incorrect
    }

    if ((elfSize < ID_IDX_NUM_BYTES) ||
        (elf64Header->Identity[ID_IDX_MAGIC0] != ELF_MAG0) || (elf64Header->Identity[ID_IDX_MAGIC1] != ELF_MAG1) ||
        (elf64Header->Identity[ID_IDX_MAGIC2] != ELF_MAG2) || (elf64Header->Identity[ID_IDX_MAGIC3] != ELF_MAG3) ||
        (elf64Header->Identity[ID_IDX_CLASS] != EH_CLASS_64))
    {
        IGC_ASSERT_MESSAGE(false, "ELF file header incorrect - nothing to be transfered to zeBinary");
        return 0; // ELF binary incorrect
    }

    size_t idxSectionHdrOffset = 0; // Indexed section header offset
    SElf64SectionHeader* sectionHeader = NULL;

    // Calculate correct (pure) size of ELF binary, because debugDataSize i.e. pOutput->m_debugDataVISASize
    // contains something else.
    elfBinSize += elf64Header->ElfHeaderSize;

    // ELF binary scanning to calculate a size of elf binary w/o alignment and additional data overhead.
    for (unsigned int i = 0; i < elf64Header->NumSectionHeaderEntries; i++)
    {
        idxSectionHdrOffset = (size_t)elf64Header->SectionHeadersOffset + (i * elf64Header->SectionHeaderEntrySize);
        sectionHeader = (SElf64SectionHeader*)((char*)elf64Header + idxSectionHdrOffset);

        // Tally up the sizes
        elfBinSize += (size_t)sectionHeader->DataSize;
        elfBinSize += (size_t)elf64Header->SectionHeaderEntrySize;
    }

    return elfBinSize;
}

// Finds a symbol name in ELF binary and returns a symbol entry
// that will later be transformed to ZE binary format
void ZEBinaryBuilder::getElfSymbol(CElfReader* elfReader, const unsigned int symtabIdx, ELF::Elf64_Sym &symtabEntry,
    char* &symName)
{
    IGC_ASSERT_MESSAGE(elfReader->GetElfHeader()->SectionHeaderEntrySize == 64, "ELF entry size 64 supported only");

    // To find a symbol name for example for relocation first we have to do
    // a lookup into .symtab (to find an index of the string in the .strtab)
    // then we have to find this name in .strtab.

    // Get data of .symtab and .strtab sections in ELF binary.
    char* symtabData = NULL;
    size_t symtabDataSize = 0;
    elfReader->GetSectionData(".symtab", symtabData, symtabDataSize);
    char* strtabData = NULL;
    size_t strtabDataSize = 0;
    elfReader->GetSectionData(".strtab", strtabData, strtabDataSize);
    if (strtabDataSize <= 1)
        elfReader->GetSectionData(".shstrtab", strtabData, strtabDataSize);

    if (!symtabData || !strtabData)
    {
        return;
    }

    // Perform lookup into .symtab.
    unsigned int symtabEntrySize = sizeof(llvm::ELF::Elf64_Sym);
    symtabEntry = *(llvm::ELF::Elf64_Sym*)(symtabData + symtabIdx * symtabEntrySize);

    // Then find the name in .strtab (String Table), where data may look as showed below:
    //  .debug_abbrev .text.stackcall .debug_ranges .debug_str .debug_info
    // ^NULL         ^NULL           ^NULL         ^NULL      ^NULL       ^NULL
    //
    // Each symtab entry contains 'st_shndx' filed, which is an index of a name (not a byte offset)
    // located in the String Table. To find for example a symbol name indexed as 3, the 3rd NULL
    // character must be found in the String Table, which is followed by the name of this symbol
    // ('.debug_ranges' in the example above).

    unsigned int ndx = symtabEntry.st_shndx; // No. of NULL characters to be skipped in .strtab
    while (ndx--)              // Iterate thru names/strings from the beginning of .strtab data
    {
        while (*strtabData++); // Find \0 terminator at the end of a given name
        strtabData++;          // Move a pointer to the first character of the next name
    }
    strtabData--;              // When a symbol name found, location of the \0 terminator is returned
                               // (not location of a name following this)
    symName = strtabData;
}

// Copy every section of ELF file (a buffer in memory) to zeBinary
void ZEBinaryBuilder::addElfSections(void* elfBin, size_t elfSize)
{
    // Correct (pure) size of ELF binary to be calculated
    size_t pureElfBinSize = calcElfSize(elfBin, elfSize);
    if (!pureElfBinSize)
    {
        return; // ELF file incorrect
    }

    SElf64Header* elf64Header = (SElf64Header*)elfBin;
    size_t entrySize = elf64Header->SectionHeaderEntrySize;  // Get the section header entry size

    CElfReader* elfReader = CElfReader::Create((char*)elfBin, pureElfBinSize);
    RAIIElf ElfObj(elfReader);

    if (!elfReader || !elfReader->IsValidElf64(elfBin, pureElfBinSize))
    {
        IGC_ASSERT_MESSAGE(false, "ELF file invalid - nothing to be transfered to zeBinary");
        return;
    }

    // Find .symtab and .strtab (or shstrtab) sections in ELF binary.
    const SElf64SectionHeader* symtabSectionHeader = elfReader->GetSectionHeader(".symtab");
    const SElf64SectionHeader* strtabSectionHeader = elfReader->GetSectionHeader(".strtab");
    if (strtabSectionHeader->DataSize <= 1)
    {
        strtabSectionHeader = elfReader->GetSectionHeader(".shstrtab");
    }

    if (!strtabSectionHeader || !symtabSectionHeader)
    {
        IGC_ASSERT_MESSAGE(false, "Some ELF file sections not found - nothing to be transfered to zeBinary");
        return;
    }

    ZEELFObjectBuilder::SectionID zeBinSectionID = 0;

    char* secData = NULL;
    size_t secDataSize = 0;
    std::vector<std::string> zeBinSymbols;      // ELF symbols added to zeBinary for a given section; to avoid duplicated symbols.

    // ELF binary scanning sections with copying whole sections one by one to zeBinary, except:
    // - empty sections
    // - Text section
    // - relocation sections
    // Also adjusting relocations found in relocation (.rela) sections.
    // Note:
    // - 64-bit ELF supported only
    // - .rel sections not supported

    for (unsigned int elfSectionIdx = 1; elfSectionIdx < elf64Header->NumSectionHeaderEntries; elfSectionIdx++)
    {
        if (elfReader->GetSectionData(elfSectionIdx, secData, secDataSize) != SUCCESS)
        {
            IGC_ASSERT_MESSAGE(false, "ELF file section data not found");
            continue;
        }

        if (secDataSize > 0) //pSectionHeader->DataSize > 0)
        {
            // Get section header to filter some section types.
            const SElf64SectionHeader* sectionHeader = elfReader->GetSectionHeader(elfSectionIdx);
            if (sectionHeader != nullptr)
            {
                if (sectionHeader->Type == ELF::SHT_REL)
                {
                    IGC_ASSERT_MESSAGE(false, "ELF file relocation sections w/o addend not supported");
                    continue;
                }
                else if (sectionHeader->Type == ELF::SHT_RELA)
                {
                    int relocEntrySize = (entrySize == 64) ? sizeof(struct ELF::Elf64_Rela) : sizeof(struct ELF::Elf32_Rela);
                    IGC_ASSERT_MESSAGE((secDataSize % relocEntrySize) == 0, "Incorrect relocation section size");
                    IGC_ASSERT_MESSAGE((entrySize == 64) || (entrySize == 32), "Incorrect relocation entry size");

                    // If .rela.foo is being processed then find zeBinary section ID of previously added .foo section
                    ZEELFObjectBuilder::SectionID nonRelaSectionID =
                        mBuilder.getSectionIDBySectionName(elfReader->GetSectionName(elfSectionIdx) + sizeof(".rela") - 1);
                    // Local symbols with the same name are allowed in zebinary if defined in different sections.
                    zeBinSymbols.clear();

                    if (entrySize == 64)
                    {
                        uint64_t relocEntryNum = secDataSize / relocEntrySize;
                        struct ELF::Elf64_Rela relocEntry;

                        for (uint64_t i = 0; i < relocEntryNum; i++)
                        {
                            relocEntry = *(struct ELF::Elf64_Rela*)(secData + i * relocEntrySize);
                            const uint32_t symtabEntrySize = sizeof(ELF::Elf64_Sym);
                            uint64_t symtabEntryNum = symtabSectionHeader->DataSize / symtabEntrySize;

                            if ((relocEntry.r_info >> 32) < symtabEntryNum)  // index
                            {
                                ELF::Elf64_Sym symtabEntry;
                                char* symName = NULL;
                                // To find a symbol name of relocation for adding to zeBinary, first we have to do
                                // a lookup into .symtab then we have to find this name in .strtab.
                                getElfSymbol(elfReader, relocEntry.r_info >> 32 /*index*/, symtabEntry, symName);

                                vISA::ZESymEntry zeSym(
                                    (vISA::GenSymType)symtabEntry.st_info,
                                    (uint32_t)symtabEntry.st_value,
                                    (uint32_t)symtabEntry.st_size,
                                    symName);  // Symbol's name

                                // Avoid symbol duplications - check whether a current symbol has been previously added.
                                bool isSymbolAdded = false;
                                for (auto zeBinSym : zeBinSymbols)
                                {
                                    if (!zeBinSym.compare(zeSym.s_name))
                                    {
                                        isSymbolAdded = true;  // A current symbol has been previously added.
                                        break;
                                    }
                                }

                                // Add either a non-global symbol, or a global symbol which is not duplicated.
                                if (!isSymbolAdded)
                                {
                                    // A current symbol has not been previously added so do it now.
                                    // Note: All symbols in ELF are local.
                                    mBuilder.addSymbol(
                                        zeSym.s_name, zeSym.s_offset, zeSym.s_size, ELF::STB_LOCAL, getSymbolElfType(zeSym), nonRelaSectionID);
                                    zeBinSymbols.push_back(zeSym.s_name);
                                }

                                unsigned int relocType = relocEntry.r_info & 0xF;
                                zebin::R_TYPE_ZEBIN zebinType = R_ZE_NONE;

                                if (relocType == ELF::R_X86_64_64)
                                    zebinType = R_ZE_SYM_ADDR;
                                else if (relocType == ELF::R_X86_64_32)
                                    zebinType = R_ZE_SYM_ADDR_32;
                                else
                                    IGC_ASSERT_MESSAGE(false, "Unsupported ELF relocation type");

                                mBuilder.addRelaRelocation(
                                    relocEntry.r_offset, zeSym.s_name, zebinType, relocEntry.r_addend, nonRelaSectionID);
                            }
                        }
                    }
                    else // entrySize == 32
                    {
                        IGC_ASSERT_MESSAGE(false, "ELF 64-bit entry size supported only");
                    }
                }
                else if (const char* sectionName = elfReader->GetSectionName(elfSectionIdx))
                {
                    if (!memcmp(sectionName, ".debug", sizeof(".debug") - 1))
                    {
                        // Non-empty, non-relocation and non-text debug section to be copied from ELF to zeBinary.
                        zeBinSectionID = mBuilder.addSectionDebug(sectionName, (uint8_t*)secData, secDataSize); // no padding, no alignment
                    }
                }
            }
        }
    }
}

void ZEBinaryBuilder::getBinaryObject(llvm::raw_pwrite_stream& os)
{
    if (!mZEInfoBuilder.empty())
        mBuilder.addSectionZEInfo(mZEInfoBuilder.getZEInfoContainer());
    mBuilder.finalize(os);
}

void ZEBinaryBuilder::getBinaryObject(Util::BinaryStream& outputStream)
{
    llvm::SmallVector<char, 64> buf;
    llvm::raw_svector_ostream llvm_os(buf);
    getBinaryObject(llvm_os);
    outputStream.Write(buf.data(), buf.size());
}

void ZEBinaryBuilder::printBinaryObject(const std::string& filename)
{
    std::error_code EC;
    llvm::raw_fd_ostream os(filename, EC);
    mBuilder.finalize(os);
    os.close();
}

void ZEBinaryBuilder::addKernelDebugEnv(const SOpenCLKernelInfo& annotations,
                                        const CBTILayout& layout,
                                        zeInfoKernel& zeinfoKernel)
{
    zeInfoDebugEnv& env = zeinfoKernel.debug_env;
    env.sip_surface_bti = layout.GetSystemThreadBindingTableIndex();
    // Now set the sip surface offset to 0 directly. Currently the surface offset
    // is computed locally when creating patch tokens.
    env.sip_surface_offset = 0;
}

void ZEBinaryBuilder::addKernelVISAAsm(const std::string& kernel,
                                       const std::string& visaasm)
{
    IGC_ASSERT(!visaasm.empty());
    mBuilder.addSectionVISAAsm(
        kernel,
        reinterpret_cast<const uint8_t*>(visaasm.data()),
        visaasm.size());
}
