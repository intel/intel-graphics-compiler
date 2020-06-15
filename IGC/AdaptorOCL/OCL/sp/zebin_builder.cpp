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

#include "zebin_builder.hpp"

#include "../../../Compiler/CodeGenPublic.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/BinaryFormat/ELF.h"
#include "Probe/Assertion.h"

using namespace IGC;
using namespace iOpenCL;
using namespace zebin;

ZEBinaryBuilder::ZEBinaryBuilder(
    const PLATFORM plat, bool is64BitPointer, const IGC::SOpenCLProgramInfo& programInfo)
    : mPlatform(plat), mBuilder(is64BitPointer)
{
    G6HWC::InitializeCapsGen8(&mHWCaps);

    // IGC only generates executable
    mBuilder.setFileType(ELF_TYPE_ZEBIN::ET_ZEBIN_EXE);

    mBuilder.setMachine(plat.eProductFamily);

    // FIXME: Most fields leaves as 0
    TargetFlags tf;
    tf.generatorSpecificFlags = TargetFlags::GeneratorSpecificFlags::NONE;
    tf.minHwRevisionId = plat.usRevId;
    tf.maxHwRevisionId = plat.usRevId;
    tf.generatorId = TargetFlags::GeneratorId::IGC;
    mBuilder.setTargetFlag(tf);

    addProgramScopeInfo(programInfo);
}

void ZEBinaryBuilder::createKernel(
    const char*  rawIsaBinary,
    unsigned int rawIsaBinarySize,
    const SOpenCLKernelInfo& annotations,
    const uint32_t grfSize)
{
    ZEELFObjectBuilder::SectionID textID =
        addKernelBinary(annotations.m_kernelName, rawIsaBinary, rawIsaBinarySize);
    addSymbols(textID, annotations);
    addKernelRelocations(textID, annotations);

    zeInfoKernel& zeKernel = mZEInfoBuilder.createKernel(annotations.m_kernelName);
    addKernelExecEnv(annotations, zeKernel);
    addLocalIds(annotations.m_executionEnivronment.CompiledSIMDSize,
        grfSize,
        annotations.m_threadPayload.HasLocalIDx,
        annotations.m_threadPayload.HasLocalIDy,
        annotations.m_threadPayload.HasLocalIDz,
        zeKernel);
    addPayloadArgsAndBTI(annotations, zeKernel);
    addMemoryBuffer(annotations, zeKernel);
}

void ZEBinaryBuilder::addProgramScopeInfo(const IGC::SOpenCLProgramInfo& programInfo)
{
    if (hasGlobalConstants(programInfo))
        mGlobalConstSectID = addGlobalConstants(programInfo);
    if (hasGlobals(programInfo))
        mGlobalSectID = addGlobals(programInfo);
}

bool ZEBinaryBuilder::hasGlobalConstants(const IGC::SOpenCLProgramInfo& annotations)
{
    if (!annotations.m_initConstantAnnotation.empty())
        return true;
    return false;
}

ZEELFObjectBuilder::SectionID
ZEBinaryBuilder::addGlobalConstants(const IGC::SOpenCLProgramInfo& annotations)
{
    // create a data section for global constant variables
    // This function should only be called when hasGlobalConstants return true
    // FIXME: not sure in what cases there will be more than one global constant buffer
    IGC_ASSERT(annotations.m_initConstantAnnotation.size() == 1);
    auto& ca = annotations.m_initConstantAnnotation.front();
    return mBuilder.addSectionData("global_const", (const uint8_t*)ca->InlineData.data(),
        ca->InlineData.size());
}

bool ZEBinaryBuilder::hasGlobals(const IGC::SOpenCLProgramInfo& annotations)
{
    if (!annotations.m_initGlobalAnnotation.empty())
        return true;
    return false;
}

ZEELFObjectBuilder::SectionID ZEBinaryBuilder::addGlobals(
    const IGC::SOpenCLProgramInfo& annotations)
{
    // create a data section for global variables
    // This function should only be called when hasGlobals return true
    // FIXME: not sure in what cases there will be more than one global buffer
    IGC_ASSERT(annotations.m_initGlobalAnnotation.size() == 1);
    auto& ca = annotations.m_initGlobalAnnotation.front();
    return mBuilder.addSectionData("global", (const uint8_t*)ca->InlineData.data(),
        ca->InlineData.size());
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
    zeinfoKernel.binding_table_indexes.insert(
        zeinfoKernel.binding_table_indexes.end(),
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
        ZEInfoBuilder::addPerThreadMemoryBuffer(zeinfoKernel.per_thread_memory_buffers,
            PreDefinedAttrGetter::MemBufferType::scratch,
            PreDefinedAttrGetter::MemBufferUsage::single_space,
            scratch0);

        return;
    }

    if (scratch0)
        ZEInfoBuilder::addPerThreadMemoryBuffer(zeinfoKernel.per_thread_memory_buffers,
            PreDefinedAttrGetter::MemBufferType::scratch,
            PreDefinedAttrGetter::MemBufferUsage::spill_fill_space,
            scratch0);
    if (scratch1)
        ZEInfoBuilder::addPerThreadMemoryBuffer(zeinfoKernel.per_thread_memory_buffers,
            PreDefinedAttrGetter::MemBufferType::scratch,
            PreDefinedAttrGetter::MemBufferUsage::private_space,
            scratch1);
    if (private_on_global)
        ZEInfoBuilder::addPerThreadMemoryBuffer(zeinfoKernel.per_thread_memory_buffers,
            PreDefinedAttrGetter::MemBufferType::global,
            PreDefinedAttrGetter::MemBufferUsage::private_space,
            private_on_global);
}

uint8_t ZEBinaryBuilder::getSymbolElfType(vISA::ZESymEntry& sym)
{
    switch (sym.s_type) {
    case vISA::GenSymType::S_NOTYPE:
        return llvm::ELF::STT_NOTYPE;
    case vISA::GenSymType::S_UNDEF:
        return llvm::ELF::STT_NOTYPE;
    case vISA::GenSymType::S_FUNC:
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

uint8_t ZEBinaryBuilder::getSymbolElfBinding(vISA::ZESymEntry& sym)
{
    // all symbols we have now that could be exposed must have
    // global binding
    switch (sym.s_type) {
    case vISA::GenSymType::S_NOTYPE:
    case vISA::GenSymType::S_UNDEF:
    case vISA::GenSymType::S_FUNC:
    case vISA::GenSymType::S_GLOBAL_VAR:
    case vISA::GenSymType::S_GLOBAL_VAR_CONST:
    case vISA::GenSymType::S_CONST_SAMPLER:
        return llvm::ELF::STB_GLOBAL;
    default:
        break;
    }
    IGC_ASSERT(0);
    return llvm::ELF::STB_GLOBAL;
}

void ZEBinaryBuilder::addSymbols(
    ZEELFObjectBuilder::SectionID kernelSectId,
    const IGC::SOpenCLKernelInfo& annotations)
{
    // get symbol list from the current process SKernelProgram
    auto symbols = [](int simdSize, const IGC::SKernelProgram& program) {
        if (simdSize == 8)
            return program.simd8.m_symbols;
        else if (simdSize == 16)
            return program.simd16.m_symbols;
        else
            return program.simd32.m_symbols;
    } (annotations.m_executionEnivronment.CompiledSIMDSize,
        annotations.m_kernelProgram);

    // If the symbol has UNDEF type, set its sectionId to -1
    // add function symbols defined in kernel text
    if (!symbols.function.empty())
        for (auto sym : symbols.function)
            mBuilder.addSymbol(sym.s_name, sym.s_offset, sym.s_size,
                getSymbolElfBinding(sym), getSymbolElfType(sym),
                (sym.s_type == vISA::GenSymType::S_UNDEF) ? -1 : kernelSectId);

    // add symbols defined in global constant section
    if (!symbols.globalConst.empty())
        for (auto sym : symbols.globalConst)
            mBuilder.addSymbol(sym.s_name, sym.s_offset, sym.s_size,
                getSymbolElfBinding(sym), getSymbolElfType(sym),
                (sym.s_type == vISA::GenSymType::S_UNDEF) ? -1 : mGlobalConstSectID);

    // add symbols defined in global section
    if (!symbols.global.empty())
        for (auto sym : symbols.global)
            mBuilder.addSymbol(sym.s_name, sym.s_offset, sym.s_size,
                getSymbolElfBinding(sym), getSymbolElfType(sym),
                (sym.s_type == vISA::GenSymType::S_UNDEF) ? -1 : mGlobalSectID);

    // we do not support sampler symbols now
    IGC_ASSERT(symbols.sampler.empty());
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
        else
            return program.simd32.m_relocs;
    } (annotations.m_executionEnivronment.CompiledSIMDSize, annotations.m_kernelProgram);

    // FIXME: For r_type, zebin::R_TYPE_ZEBIN should have the same enum value as visa::GenRelocType.
    // Take the value directly
    if (!relocs.empty())
        for (auto reloc : relocs)
            mBuilder.addRelocation(reloc.r_offset, reloc.r_symbol, (zebin::R_TYPE_ZEBIN)reloc.r_type, targetId);
}

void ZEBinaryBuilder::addKernelExecEnv(const SOpenCLKernelInfo& annotations,
    zeInfoKernel& zeinfoKernel)
{
    zeInfoExecutionEnvironment& env = zeinfoKernel.execution_env;

    // FIXME: compiler did not provide this information
    env.actual_kernel_start_offset = 0;

    // FIXME: is this barrier counts?
    env.barrier_count = annotations.m_executionEnivronment.HasBarriers;

    env.disable_mid_thread_preemption = annotations.m_executionEnivronment.DisableMidThreadPreemption;
    env.grf_count = annotations.m_executionEnivronment.NumGRFRequired;
    env.has_4gb_buffers = annotations.m_executionEnivronment.CompiledForGreaterThan4GBBuffers;
    env.has_device_enqueue = annotations.m_executionEnivronment.HasDeviceEnqueue;
    env.has_fence_for_image_access = annotations.m_executionEnivronment.HasReadWriteImages;
    env.has_global_atomics = annotations.m_executionEnivronment.HasGlobalAtomics;
    env.offset_to_skip_per_thread_data_load = annotations.m_threadPayload.OffsetToSkipPerThreadDataLoad;;
    env.offset_to_skip_set_ffid_gp = annotations.m_threadPayload.OffsetToSkipSetFFIDGP;;
    env.required_sub_group_size = annotations.m_executionEnivronment.CompiledSubGroupsNumber;
    if(annotations.m_executionEnivronment.HasFixedWorkGroupSize)
    {
        env.required_work_group_size.push_back(annotations.m_executionEnivronment.FixedWorkgroupSize[0]);
        env.required_work_group_size.push_back(annotations.m_executionEnivronment.FixedWorkgroupSize[1]);
        env.required_work_group_size.push_back(annotations.m_executionEnivronment.FixedWorkgroupSize[2]);
    }
    env.simd_size = annotations.m_executionEnivronment.CompiledSIMDSize;

    // FIXME: where is this information?
    env.slm_size = 0;

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
        // offset to 0 and for packed_local_ids, its size is 96 (int32*3) always
        mZEInfoBuilder.addPerThreadPayloadArgument(
            zeinfoKernel.per_thread_payload_arguments,
            PreDefinedAttrGetter::ArgType::packed_local_ids, 0, 96);
        return;
    }
    // otherwise, using arg_type::local_id format
    // byte size for one id, have to be grf align
    IGC_ASSERT(simdSize);
    IGC_ASSERT(grfSize);
    // each id takes 2 bytes
    int32_t per_id_size = 2 * simdSize;
    per_id_size = (per_id_size % grfSize) == 0 ?
        per_id_size : (per_id_size / grfSize) + 1;
    // total_size = num_of_ids * per_id_size
    int32_t total_size = per_id_size * ((has_local_id_x ? 1 : 0) +
        (has_local_id_y ? 1 : 0) + (has_local_id_z ? 1 : 0));
    mZEInfoBuilder.addPerThreadPayloadArgument(
        zeinfoKernel.per_thread_payload_arguments,
        PreDefinedAttrGetter::ArgType::local_id, 0, total_size);
}

void ZEBinaryBuilder::getBinaryObject(llvm::raw_pwrite_stream& os)
{
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