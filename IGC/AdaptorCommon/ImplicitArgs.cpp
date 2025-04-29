/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ImplicitArgs.hpp"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs/KernelArgs.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvmWrapper/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

static const std::vector<ImplicitArg> IMPLICIT_ARGS = {
    ImplicitArg(ImplicitArg::R0, "r0", ImplicitArg::INT, WIAnalysis::UNIFORM_THREAD, 8, ImplicitArg::ALIGN_GRF, false, GenISAIntrinsic::GenISA_getR0),

    ImplicitArg(ImplicitArg::PAYLOAD_HEADER, "payloadHeader", ImplicitArg::INT, WIAnalysis::UNIFORM_WORKGROUP, 8, ImplicitArg::ALIGN_GRF, true, GenISAIntrinsic::GenISA_getPayloadHeader),
    ImplicitArg(ImplicitArg::GLOBAL_OFFSET, "globalOffset", ImplicitArg::INT, WIAnalysis::UNIFORM_WORKGROUP, 3, ImplicitArg::ALIGN_DWORD, true, GenISAIntrinsic::GenISA_getGlobalOffset),
    ImplicitArg(ImplicitArg::WORK_DIM, "workDim", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true, GenISAIntrinsic::GenISA_getWorkDim),

    ImplicitArg(ImplicitArg::NUM_GROUPS, "numWorkGroups", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 3, ImplicitArg::ALIGN_DWORD, true, GenISAIntrinsic::GenISA_getNumWorkGroups),
    ImplicitArg(ImplicitArg::GLOBAL_SIZE, "globalSize", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 3, ImplicitArg::ALIGN_DWORD, true, GenISAIntrinsic::GenISA_getGlobalSize),
    ImplicitArg(ImplicitArg::LOCAL_SIZE, "localSize", ImplicitArg::INT, WIAnalysis::UNIFORM_WORKGROUP, 3, ImplicitArg::ALIGN_DWORD, true, GenISAIntrinsic::GenISA_getLocalSize),
    ImplicitArg(ImplicitArg::ENQUEUED_LOCAL_WORK_SIZE, "enqueuedLocalSize", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 3, ImplicitArg::ALIGN_DWORD, true, GenISAIntrinsic::GenISA_getEnqueuedLocalSize),

    ImplicitArg(ImplicitArg::LOCAL_ID_X, "localIdX", ImplicitArg::SHORT, WIAnalysis::RANDOM, 16, ImplicitArg::ALIGN_GRF, false, GenISAIntrinsic::GenISA_getLocalID_X),
    ImplicitArg(ImplicitArg::LOCAL_ID_Y, "localIdY", ImplicitArg::SHORT, WIAnalysis::RANDOM, 16, ImplicitArg::ALIGN_GRF, false, GenISAIntrinsic::GenISA_getLocalID_Y),
    ImplicitArg(ImplicitArg::LOCAL_ID_Z, "localIdZ", ImplicitArg::SHORT, WIAnalysis::RANDOM, 16, ImplicitArg::ALIGN_GRF, false, GenISAIntrinsic::GenISA_getLocalID_Z),

    ImplicitArg(ImplicitArg::CONSTANT_BASE, "constBase", ImplicitArg::CONSTPTR, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_PTR, true),
    ImplicitArg(ImplicitArg::GLOBAL_BASE, "globalBase", ImplicitArg::GLOBALPTR, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_PTR, true),
    ImplicitArg(ImplicitArg::PRIVATE_BASE, "privateBase", ImplicitArg::PRIVATEPTR, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_PTR, true, GenISAIntrinsic::GenISA_getPrivateBase),
    ImplicitArg(ImplicitArg::PRINTF_BUFFER, "printfBuffer", ImplicitArg::GLOBALPTR, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_PTR, true, GenISAIntrinsic::GenISA_getPrintfBuffer),

    ImplicitArg(ImplicitArg::BUFFER_OFFSET, "bufferOffset", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),

    ImplicitArg(ImplicitArg::CONSTANT_REG_FP32, "const_reg_fp32", ImplicitArg::FP32, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),
    ImplicitArg(ImplicitArg::CONSTANT_REG_QWORD, "const_reg_qword", ImplicitArg::LONG, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_QWORD, true),
    ImplicitArg(ImplicitArg::CONSTANT_REG_DWORD, "const_reg_dword", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),
    ImplicitArg(ImplicitArg::CONSTANT_REG_WORD, "const_reg_word", ImplicitArg::SHORT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),
    ImplicitArg(ImplicitArg::CONSTANT_REG_BYTE, "const_reg_byte", ImplicitArg::BYTE, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),

    ImplicitArg(ImplicitArg::IMAGE_HEIGHT, "imageHeigt", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),
    ImplicitArg(ImplicitArg::IMAGE_WIDTH, "imageWidth", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),
    ImplicitArg(ImplicitArg::IMAGE_DEPTH, "imageDepth", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),
    ImplicitArg(ImplicitArg::IMAGE_NUM_MIP_LEVELS, "imageNumMipLevels", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),
    ImplicitArg(ImplicitArg::IMAGE_CHANNEL_DATA_TYPE, "imageDataType", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),
    ImplicitArg(ImplicitArg::IMAGE_CHANNEL_ORDER, "imageOrder", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),
    ImplicitArg(ImplicitArg::IMAGE_SRGB_CHANNEL_ORDER, "imageSrgbOrder", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),
    ImplicitArg(ImplicitArg::IMAGE_ARRAY_SIZE, "imageArrSize", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),
    ImplicitArg(ImplicitArg::IMAGE_NUM_SAMPLES, "imageNumSamples", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),

    ImplicitArg(ImplicitArg::SAMPLER_ADDRESS, "smpAddress", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),
    ImplicitArg(ImplicitArg::SAMPLER_NORMALIZED, "smpNormalized", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),
    ImplicitArg(ImplicitArg::SAMPLER_SNAP_WA, "smpSnapWA", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),
    ImplicitArg(ImplicitArg::INLINE_SAMPLER, "inlineSampler", ImplicitArg::LONG, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_QWORD, true),
    ImplicitArg(ImplicitArg::FLAT_IMAGE_BASEOFFSET, "flatImageBaseoffset", ImplicitArg::LONG, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_QWORD, true),
    ImplicitArg(ImplicitArg::FLAT_IMAGE_HEIGHT, "flatImageHeight", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),
    ImplicitArg(ImplicitArg::FLAT_IMAGE_WIDTH, "flatImageWidth", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),
    ImplicitArg(ImplicitArg::FLAT_IMAGE_PITCH, "flatImagePitch", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),

    ImplicitArg(ImplicitArg::VME_MB_BLOCK_TYPE, "vmeMbBlockType", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),
    ImplicitArg(ImplicitArg::VME_SUBPIXEL_MODE, "vmeSubpixelMode", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),
    ImplicitArg(ImplicitArg::VME_SAD_ADJUST_MODE, "vmeSadAdjustMode", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),
    ImplicitArg(ImplicitArg::VME_SEARCH_PATH_TYPE, "vmeSearchPathType", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),

    ImplicitArg(ImplicitArg::DEVICE_ENQUEUE_DEFAULT_DEVICE_QUEUE, "deviceEnqueueDefaultDeviceQueue", ImplicitArg::GLOBALPTR, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_PTR, true),
    ImplicitArg(ImplicitArg::DEVICE_ENQUEUE_EVENT_POOL, "deviceEnqueueEventPool", ImplicitArg::GLOBALPTR, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_PTR, true),
    ImplicitArg(ImplicitArg::DEVICE_ENQUEUE_MAX_WORKGROUP_SIZE, "deviceEnqueueMaxWorkgroupSize", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),
    ImplicitArg(ImplicitArg::DEVICE_ENQUEUE_PARENT_EVENT, "deviceEnqueueParentEvent", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),
    ImplicitArg(ImplicitArg::DEVICE_ENQUEUE_PREFERED_WORKGROUP_MULTIPLE, "deviceEnqueuePreferedWorkgroupMultiple", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),
    ImplicitArg(ImplicitArg::GET_OBJECT_ID, "deviceEnqueueGetObjectId", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),
    ImplicitArg(ImplicitArg::GET_BLOCK_SIMD_SIZE, "deviceEnqueueGetBlockSimdSize", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),

    ImplicitArg(ImplicitArg::LOCAL_MEMORY_STATELESS_WINDOW_START_ADDRESS, "localMemStatelessWindowStartAddr", ImplicitArg::GLOBALPTR, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_PTR, true),
    ImplicitArg(ImplicitArg::LOCAL_MEMORY_STATELESS_WINDOW_SIZE, "localMemStatelessWindowSize", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),
    ImplicitArg(ImplicitArg::PRIVATE_MEMORY_STATELESS_SIZE, "PrivateMemStatelessSize", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),

    ImplicitArg(ImplicitArg::STAGE_IN_GRID_ORIGIN, "stageInGridOrigin", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 3, ImplicitArg::ALIGN_GRF, true, GenISAIntrinsic::GenISA_getStageInGridOrigin),
    ImplicitArg(ImplicitArg::STAGE_IN_GRID_SIZE, "stageInGridSize", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 3, ImplicitArg::ALIGN_GRF, true, GenISAIntrinsic::GenISA_getStageInGridSize),

    ImplicitArg(ImplicitArg::SYNC_BUFFER, "syncBuffer", ImplicitArg::GLOBALPTR, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_PTR, false, GenISAIntrinsic::GenISA_getSyncBuffer),

    // raytracing
    ImplicitArg(ImplicitArg::RT_GLOBAL_BUFFER_POINTER, "globalPointer", ImplicitArg::GLOBALPTR, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_GRF, false, GenISAIntrinsic::GenISA_getRtGlobalBufferPtr),
    ImplicitArg(ImplicitArg::RT_LOCAL_BUFFER_POINTER, "localPointer", ImplicitArg::GLOBALPTR, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_QWORD, false),
    ImplicitArg(ImplicitArg::RT_INLINED_DATA, "inlinedData", ImplicitArg::GLOBALPTR, WIAnalysis::UNIFORM_GLOBAL, 2, ImplicitArg::ALIGN_GRF, false),
    ImplicitArg(ImplicitArg::RT_STACK_ID, "stackID", ImplicitArg::SHORT, WIAnalysis::RANDOM, 16, ImplicitArg::ALIGN_GRF, false),

    ImplicitArg(ImplicitArg::BINDLESS_OFFSET, "bindlessOffset", ImplicitArg::INT, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_DWORD, true),

    ImplicitArg(ImplicitArg::IMPLICIT_ARG_BUFFER_PTR, "implicitArgBuffer", ImplicitArg::GLOBALPTR, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_PTR, true),

    ImplicitArg(ImplicitArg::ASSERT_BUFFER_POINTER, "assertBufferPointer", ImplicitArg::GLOBALPTR, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_GRF, false, GenISAIntrinsic::GenISA_getAssertBufferPtr),

    // BufferBoundsChecking
    ImplicitArg(ImplicitArg::BUFFER_SIZE, "bufferSize", ImplicitArg::LONG, WIAnalysis::UNIFORM_GLOBAL, 1, ImplicitArg::ALIGN_QWORD, true),
};

ImplicitArg::ImplicitArg(
        const ArgType&                  argType,
        const std::string&              name,
        const ValType                   valType,
        WIAnalysis::WIDependancy        dependency,
        unsigned int                    nbElement,
        ValAlign                        align,
        bool                            isConstantBuf)
        :   m_argType(argType),
            m_name(name),
            m_valType(valType),
            m_dependency(dependency),
            m_nbElement(nbElement),
            m_align(align),
            m_isConstantBuf(isConstantBuf),
            m_GenIntrinsicID(GenISAIntrinsic::ID::no_intrinsic)
{
}

ImplicitArg::ImplicitArg(
    const ArgType& argType,
    const std::string& name,
    const ValType                   valType,
    WIAnalysis::WIDependancy        dependency,
    unsigned int                    nbElement,
    ValAlign                        align,
    bool                            isConstantBuf,
    GenISAIntrinsic::ID             GenIntrinsicID)
    : m_argType(argType),
    m_name(name),
    m_valType(valType),
    m_dependency(dependency),
    m_nbElement(nbElement),
    m_align(align),
    m_isConstantBuf(isConstantBuf),
    m_GenIntrinsicID(GenIntrinsicID)
{
}

ImplicitArg::ArgType ImplicitArg::getArgType() const
{
    return m_argType;
}

const std::string& ImplicitArg::getName() const
{
    return m_name;
}

Type* ImplicitArg::getLLVMType(LLVMContext& context) const
{
    // Return the appropriate LLVM type, based on the argument value type.
    // This has two quirks:
    // 1) Pointer size depends on the data layout.
    // 2) For non-uniform arguments, m_nbElement should be used to determine
    // the allocation size, but not to determine the LLVM type, because every
    // work-item sees a scalar.
    Type* baseType = nullptr;
    switch (m_valType)
    {
    case BYTE:
        baseType = Type::getInt8Ty(context);
        break;
    case SHORT:
        baseType = Type::getInt16Ty(context);
        break;
    case INT:
        baseType = Type::getInt32Ty(context);
        break;
    case LONG:
        baseType = Type::getInt64Ty(context);
        break;
    case FP32:
        baseType = Type::getFloatTy(context);
        break;
    case CONSTPTR:
        baseType = Type::getInt8Ty(context)->getPointerTo(ADDRESS_SPACE_CONSTANT);
        break;
    case PRIVATEPTR:
        baseType = Type::getInt8Ty(context)->getPointerTo(ADDRESS_SPACE_PRIVATE);
        break;
    case GLOBALPTR:
        baseType = Type::getInt8Ty(context)->getPointerTo(ADDRESS_SPACE_GLOBAL);
        break;
    default:
        IGC_ASSERT_MESSAGE(0, "Unrecognized implicit argument type");
        return nullptr;
    }

    if (m_nbElement == 1 || !WIAnalysis::isDepUniform(m_dependency))
    {
        return baseType;
    }
    else
    {
        return IGCLLVM::FixedVectorType::get(baseType, m_nbElement);
    }
}

unsigned int ImplicitArg::getNumberElements() const
{
    return m_nbElement;
}

VISA_Type ImplicitArg::getVISAType(const DataLayout& DL) const
{
    switch (m_valType)
    {
    case BYTE:
        return VISA_Type::ISA_TYPE_B;
    case SHORT:
        return VISA_Type::ISA_TYPE_W;
    case INT:
        return VISA_Type::ISA_TYPE_D;
    case LONG:
        return VISA_Type::ISA_TYPE_Q;
    case FP32:
        return VISA_Type::ISA_TYPE_F;
    case CONSTPTR:
    case GLOBALPTR:
    case PRIVATEPTR:
        return getPointerSize(DL) == 4 ? VISA_Type::ISA_TYPE_UD : VISA_Type::ISA_TYPE_UQ;
    default:
        IGC_ASSERT_MESSAGE(0, "Unrecognized implicit argument type");
        break;
    }

    return VISA_Type::ISA_TYPE_UD;
}

unsigned int ImplicitArg::getPointerSize(const DataLayout& DL) const {
    switch (m_valType) {
    case CONSTPTR:    return DL.getPointerSize(ADDRESS_SPACE_CONSTANT);
    case PRIVATEPTR:  return DL.getPointerSize(ADDRESS_SPACE_PRIVATE);
    case GLOBALPTR:   return DL.getPointerSize(ADDRESS_SPACE_GLOBAL);
    default:
        IGC_ASSERT_MESSAGE(0, "Unrecognized pointer type");
        break;
    }
    return 0;
}

IGC::e_alignment ImplicitArg::getAlignType(const DataLayout& DL) const
{
    switch (m_align)
    {
    case ALIGN_DWORD:
       return IGC::EALIGN_DWORD;
    case ALIGN_QWORD:
        return IGC::EALIGN_QWORD;
    case ALIGN_GRF: //According to old implementation, EALIGN_GRF = EALIGN_HWORD, the corresponding alignmentSize is 32, so EALIGN_HWORD will not change the old define.
       return IGC::EALIGN_HWORD;  //FIXME: But, the ALIGN_GRF is really GRF aligned? If so, there is bug here.
    case ALIGN_PTR:
      return getPointerSize(DL) == 4 ? IGC::EALIGN_DWORD : IGC::EALIGN_QWORD;
    default:
        IGC_ASSERT_MESSAGE(0, "Uknown alignment");
        break;
    }

    return IGC::EALIGN_DWORD;
}

size_t ImplicitArg::getAlignment(const DataLayout& DL) const
{
    return (size_t) 1 << getAlignType(DL);
}

WIAnalysis::WIDependancy ImplicitArg::getDependency() const {
    return m_dependency;
}

unsigned int ImplicitArg::getAllocateSize(const DataLayout& DL) const
{
    unsigned int elemSize = 0;

    switch (m_valType)
    {
    case BYTE:
        elemSize = 1;
        break;
    case SHORT:
        elemSize = 2;
        break;
    case INT:
        elemSize = 4;
        break;
    case LONG:
        elemSize = 8;
        break;
    case FP32:
        elemSize = 4;
        break;
    case CONSTPTR:
    case GLOBALPTR:
    case PRIVATEPTR:
        elemSize = getPointerSize(DL);
        break;
    default:
        IGC_ASSERT_MESSAGE(0, "Unrecognized implicit argument type");
        break;
    }

    return m_nbElement *  elemSize;
}

bool ImplicitArg::isConstantBuf() const {
    return m_isConstantBuf;
}

bool ImplicitArg::isLocalIDs() const {
    return (m_argType == ImplicitArg::LOCAL_ID_X ||
                 m_argType == ImplicitArg::LOCAL_ID_Y ||
                 m_argType == ImplicitArg::LOCAL_ID_Z);
}

GenISAIntrinsic::ID ImplicitArg::getGenIntrinsicID() const {
    return m_GenIntrinsicID;
}

ImplicitArgs::ImplicitArgs(const llvm::Function& func , const MetaDataUtils* pMdUtils)
{
    IGC_ASSERT_MESSAGE((IMPLICIT_ARGS.size() == ImplicitArg::NUM_IMPLICIT_ARGS), "Mismatch in NUM_IMPLICIT_ARGS and IMPLICIT_ARGS vector");
    {
        // Note: the order that implicit args are added here must match the
        // order of the ImplicitArg::Argtype enum.  Let's check that they match:
        uint32_t CurArgId = ImplicitArg::START_ID;
        for (auto& Arg : IMPLICIT_ARGS)
        {
            IGC_ASSERT_MESSAGE((Arg.getArgType() == CurArgId++), "enum and vector out of sync!");
        }
    }
    m_funcInfoMD = pMdUtils->getFunctionsInfoItem(const_cast<llvm::Function*>(&func));
}

const ImplicitArg& ImplicitArgs::operator[](unsigned int i) const {
    IGC_ASSERT_MESSAGE((IMPLICIT_ARGS.size() == ImplicitArg::NUM_IMPLICIT_ARGS), "Mismatch in NUM_IMPLICIT_ARGS and IMPLICIT_ARGS vector");
    return IMPLICIT_ARGS[getArgType(i)];
}

unsigned int ImplicitArgs::getArgIndex(ImplicitArg::ArgType argType) const {
    IGC_ASSERT_MESSAGE((this->size() > 0), "There are no implicit arguments!");

    // Find the first appearance of the given implicit arg type
    unsigned int implicitArgIndex = 0;
    for (implicitArgIndex = 0; implicitArgIndex < this->size(); ++implicitArgIndex)
    {
        ImplicitArg::ArgType type = getArgType(implicitArgIndex);
        if (type == argType)
        {
            break;
        }
    }

    IGC_ASSERT_MESSAGE((implicitArgIndex < this->size()), "Implicit argument not found!");

    return implicitArgIndex;
}

bool ImplicitArgs::isImplicitArgExist(
    const IGCMD::FunctionInfoMetaDataHandle& funcInfo,
    ImplicitArg::ArgType argType)
{
    bool res = false;

    // Find the first appearance of the given implicit arg type
    for (unsigned int implicitArgIndex = 0; implicitArgIndex < funcInfo->size_ImplicitArgInfoList(); ++implicitArgIndex)
    {
        ImplicitArg::ArgType type = (ImplicitArg::ArgType)
            funcInfo->getImplicitArgInfoListItem(implicitArgIndex)->getArgId();
        if (type == argType)
        {
            res = true;
            break;
        }
    }

    return res;
}

bool ImplicitArgs::isImplicitArgExist(ImplicitArg::ArgType argType) const
{
    return isImplicitArgExist(m_funcInfoMD, argType);
}

bool ImplicitArgs::isImplicitArgExist(
    llvm::Function& F,
    ImplicitArg::ArgType argType,
    const IGCMD::MetaDataUtils* pMdUtils)
{
    return isImplicitArgExist(pMdUtils->getFunctionsInfoItem(&F), argType);
}

bool ImplicitArgs::isImplicitArgExistForNumberedArg(ImplicitArg::ArgType argType, int argNum) const {
    IGC_ASSERT_MESSAGE((argNum >= 0), "objectNum cannot be less than 0");

    for (int i = 0, e = m_funcInfoMD->size_ImplicitArgInfoList(); i < e; ++i)
    {
        ArgInfoMetaDataHandle argInfo = m_funcInfoMD->getImplicitArgInfoListItem(i);
        if (argInfo->getArgId() == argType && argInfo->getExplicitArgNum() == argNum)
        {
            return true;
        }
    }
    return false;
}

unsigned int ImplicitArgs::getImageArgIndex(ImplicitArg::ArgType argType, const Argument* image) const {
    IGC_ASSERT_MESSAGE(isImplicitImage(argType), "Non image/sampler implicit arg!");
    return getNumberedArgIndex(argType, image->getArgNo());
}

unsigned int ImplicitArgs::getNumberedArgIndex(ImplicitArg::ArgType argType, int argNum) const {
    IGC_ASSERT_MESSAGE((argNum >= 0), "objectNum cannot be less than 0");

    for (int i = 0, e = m_funcInfoMD->size_ImplicitArgInfoList() ; i < e; ++i)
    {
        ArgInfoMetaDataHandle argInfo = m_funcInfoMD->getImplicitArgInfoListItem(i);
        if (argInfo->getArgId() == argType && argInfo->getExplicitArgNum() == argNum)
        {
            return i;
        }
    }

    IGC_ASSERT_MESSAGE(0, "No implicit argument for the given type & argNum");
    return m_funcInfoMD->size_ImplicitArgInfoList();
}

void ImplicitArgs::addImplicitArgs(llvm::Function& F, const SmallVectorImpl<ImplicitArg::ArgType>& implicitArgs, const MetaDataUtils* pMdUtils) {
    // Add implicit args metadata for the given function
    FunctionInfoMetaDataHandle funcInfo = pMdUtils->getFunctionsInfoItem(&F);
    for (auto arg : implicitArgs)
    {
        if (!isImplicitArgExist(F, arg, pMdUtils))
        {
            ArgInfoMetaDataHandle argMD = ArgInfoMetaDataHandle(new ArgInfoMetaData());
            argMD->setArgId(arg);
            funcInfo->addImplicitArgInfoListItem(argMD);
        }
    }
}

void ImplicitArgs::addImplicitArgsTotally(llvm::Function& F, const SmallVectorImpl<ImplicitArg::ArgType>& implicitArgs, const MetaDataUtils* pMdUtils) {
    // Add implicit args metadata for the given function
    FunctionInfoMetaDataHandle funcInfo = pMdUtils->getFunctionsInfoItem(&F);
    bool argAdded = false;
    for (auto arg : implicitArgs)
    {
        if (!isImplicitArgExist(F, arg, pMdUtils))
        {
            ArgInfoMetaDataHandle argMD = ArgInfoMetaDataHandle(new ArgInfoMetaData());
            argMD->setArgId(arg);
            funcInfo->addImplicitArgInfoListItem(argMD);
            argAdded = true;
        }
    }
    if (!argAdded) return;

    // Add implicit args metadata for callers
    std::vector<Value*> functionUserList(F.user_begin(), F.user_end());
    for (auto U : functionUserList)
    {
        CallInst* cInst = dyn_cast<CallInst>(U);
        IGC_ASSERT_MESSAGE(cInst, "Caller must not be nullptr");
        Function* parent_func = cInst->getParent()->getParent();
        addImplicitArgsTotally(*parent_func, implicitArgs, pMdUtils);
    }
}

void ImplicitArgs::addImageArgs(llvm::Function& F, const ImplicitArg::ArgMap& argMap, const MetaDataUtils* pMdUtils)
{
    FunctionInfoMetaDataHandle funcInfo = pMdUtils->getFunctionsInfoItem(&F);
    for (ImplicitArg::ArgType argType = ImplicitArg::IMAGES_START; argType <= ImplicitArg::IMAGES_END; argType = static_cast<ImplicitArg::ArgType>(argType + 1))
    {
        auto argMapIter = argMap.find(argType);
        if (argMapIter != argMap.end())
        {
            for (const auto& argI : argMapIter->second)
            {
                ArgInfoMetaDataHandle argMD = ArgInfoMetaDataHandle(new ArgInfoMetaData());
                argMD->setArgId(argType);
                argMD->setExplicitArgNum(argI);
                funcInfo->addImplicitArgInfoListItem(argMD);
            }
        }
    }
}

void ImplicitArgs::addStructArgs(llvm::Function& F, const Argument* A, const ImplicitArg::StructArgList& S, const MetaDataUtils* pMdUtils)
{
    FunctionInfoMetaDataHandle funcInfo = pMdUtils->getFunctionsInfoItem(&F);

    for (const auto& argI : S)
    {
        unsigned int id = argI.first;
        unsigned int offset = argI.second;

        ArgInfoMetaDataHandle argMD = ArgInfoMetaDataHandle(new ArgInfoMetaData());
        argMD->setExplicitArgNum(A->getArgNo());
        argMD->setArgId(id);
        argMD->setStructArgOffset(offset);
        funcInfo->addImplicitArgInfoListItem(argMD);
    }
}

void ImplicitArgs::addNumberedArgs(llvm::Function& F, const ImplicitArg::ArgMap& argMap, const IGCMD::MetaDataUtils* pMdUtils)
{
    FunctionInfoMetaDataHandle funcInfo = pMdUtils->getFunctionsInfoItem(&F);
    for (const auto& argPair : argMap)
    {
        ImplicitArg::ArgType argId = argPair.first;
        for (const auto& argNum : argPair.second)
        {
            ArgInfoMetaDataHandle argMD(new ArgInfoMetaData());
            argMD->setArgId(argId);
            argMD->setExplicitArgNum(argNum);
            funcInfo->addImplicitArgInfoListItem(argMD);
        }
    }
}

// Add one implicit argument for each pointer argument to global or constant buffer.
// Note that F is the original input function (ie, without implicit arguments).
void ImplicitArgs::addBufferOffsetArgs(llvm::Function& F, const IGCMD::MetaDataUtils* pMdUtils, IGC::ModuleMetaData *modMD)
{
    ImplicitArg::ArgMap OffsetArgs;
    FunctionInfoMetaDataHandle funcInfoMD = pMdUtils->getFunctionsInfoItem(&F);

    IGC_ASSERT(modMD->FuncMD.find(&F) != modMD->FuncMD.end());

    // StatelessToStateful optimization is not applied on non-kernel functions.
    if (!isEntryFunc(pMdUtils, &F))
        return;

    FunctionMetaData* funcMD = &modMD->FuncMD.find(&F)->second;
    for (const auto& Arg : F.args())
    {
        PointerType* PTy = dyn_cast<PointerType>(Arg.getType());
        if (!PTy ||
            (PTy->getPointerAddressSpace() != ADDRESS_SPACE_CONSTANT &&
             PTy->getPointerAddressSpace() != ADDRESS_SPACE_GLOBAL))
        {
            continue;
        }

        int argNo = Arg.getArgNo();

        std::string argbaseType = "";
        if (funcMD->m_OpenCLArgBaseTypes.size() > (unsigned)argNo)
            argbaseType = funcMD->m_OpenCLArgBaseTypes[argNo];

        // Do not generate implicit arg for any image arguments
        KernelArg::ArgType ImgArgType;
        if (KernelArg::isImage(
            &Arg, argbaseType, ImgArgType) ||
            KernelArg::isSampler(&Arg, argbaseType))
        {
            continue;
        }

        OffsetArgs[ImplicitArg::BUFFER_OFFSET].insert(argNo);
    }
    if (OffsetArgs.size() > 0)
    {
        ImplicitArgs::addNumberedArgs(F, OffsetArgs, pMdUtils);
    }
}

// Add one implicit argument for each pointer argument to global or constant buffer.
// Note that F is the original input function (ie, without implicit arguments).
void ImplicitArgs::addBindlessOffsetArgs(llvm::Function& F, const IGCMD::MetaDataUtils* pMdUtils, IGC::ModuleMetaData* modMD)
{
    ImplicitArg::ArgMap OffsetArgs;
    FunctionInfoMetaDataHandle funcInfoMD = pMdUtils->getFunctionsInfoItem(&F);

    IGC_ASSERT(modMD->FuncMD.find(&F) != modMD->FuncMD.end());

    // StatelessToStateful optimization is not applied on non-kernel functions.
    if (!isEntryFunc(pMdUtils, &F))
        return;

    FunctionMetaData* funcMD = &modMD->FuncMD.find(&F)->second;
    for (const auto& Arg : F.args())
    {
        PointerType* PTy = dyn_cast<PointerType>(Arg.getType());
        if (!PTy ||
            (PTy->getPointerAddressSpace() != ADDRESS_SPACE_CONSTANT &&
                PTy->getPointerAddressSpace() != ADDRESS_SPACE_GLOBAL))
        {
            continue;
        }

        int argNo = Arg.getArgNo();

        std::string argbaseType = "";
        if (funcMD->m_OpenCLArgBaseTypes.size() > (unsigned)argNo)
            argbaseType = funcMD->m_OpenCLArgBaseTypes[argNo];

        // Do not generate implicit arg for any image arguments
        KernelArg::ArgType ImgArgType;
        if (KernelArg::isImage(
            &Arg, argbaseType, ImgArgType) ||
            KernelArg::isSampler(&Arg, argbaseType))
        {
            continue;
        }

        OffsetArgs[ImplicitArg::BINDLESS_OFFSET].insert(argNo);
    }

    // Create a bindless offset for the implicit args CONST_BASE and GLOBAL_BASE as well.
    // At this step, they are not yet added to the function signature, but since implicit args
    // are added sequentially, we can calculate the associated argNo here.
    unsigned argNo = F.arg_size();
    for (auto AI = funcInfoMD->begin_ImplicitArgInfoList(), AE = funcInfoMD->end_ImplicitArgInfoList(); AI != AE; AI++, argNo++)
    {
        ArgInfoMetaDataHandle argInfo = *AI;
        ImplicitArg::ArgType argId = static_cast<ImplicitArg::ArgType>(argInfo->getArgId());
        if (argId == ImplicitArg::CONSTANT_BASE || argId == ImplicitArg::GLOBAL_BASE)
        {
            OffsetArgs[ImplicitArg::BINDLESS_OFFSET].insert(argNo);
        }
    }

    if (OffsetArgs.size() > 0)
    {
        ImplicitArgs::addNumberedArgs(F, OffsetArgs, pMdUtils);
    }
}

unsigned int ImplicitArgs::size() const {
    return m_funcInfoMD->size_ImplicitArgInfoList();
}

bool ImplicitArgs::isImplicitImage(ImplicitArg::ArgType argType)
{
    return (argType >= ImplicitArg::IMAGES_START) && (argType <= ImplicitArg::IMAGES_END);
}

bool ImplicitArgs::isImplicitStruct(ImplicitArg::ArgType argType)
{
    return (argType >= ImplicitArg::STRUCT_START) && (argType <= ImplicitArg::STRUCT_END);
}

bool ImplicitArgs::isImplicitArg(Argument *arg) const
{
    unsigned argSize = arg->getParent()->arg_size();
    unsigned numImplicitArgs = size();
    IGC_ASSERT_MESSAGE(argSize >= numImplicitArgs, "Function arg size does not match meta data args.");
    unsigned argNo = arg->getArgNo();
    return argNo >= (argSize - numImplicitArgs);
}

int ImplicitArgs::getExplicitArgNumForArg(Argument *implicitArg) const
{
    unsigned argSize = implicitArg->getParent()->arg_size();
    unsigned numImplicitArgs = size();
    IGC_ASSERT_MESSAGE(argSize >= numImplicitArgs, "Function arg size does not match meta data args.");
    unsigned argNo = implicitArg->getArgNo();
    IGC_ASSERT_MESSAGE(argNo >= (argSize - numImplicitArgs), "The arg should be implicit arg");
    unsigned implicitArgIndex = argNo - (argSize - numImplicitArgs);
    ArgInfoMetaDataHandle argInfo = m_funcInfoMD->getImplicitArgInfoListItem(implicitArgIndex);
    return argInfo->getExplicitArgNum();
}

ImplicitArg::ArgType ImplicitArgs::getArgType(unsigned int index) const {
    IGC_ASSERT_MESSAGE((index < size()), "Index out of range");
    ArgInfoMetaDataHandle argInfo = m_funcInfoMD->getImplicitArgInfoListItem(index);
    return static_cast<ImplicitArg::ArgType>(argInfo->getArgId());
}

ImplicitArg::ArgType ImplicitArgs::getArgType(GenISAIntrinsic::ID id) {
    IGC_ASSERT(id != GenISAIntrinsic::ID::no_intrinsic);
    for (auto &arg : IMPLICIT_ARGS)
    {
        if (arg.getGenIntrinsicID() == id)
        {
            return arg.getArgType();
        }
    }
    return ImplicitArg::ArgType::NUM_IMPLICIT_ARGS;
}

IGC::WIAnalysis::WIDependancy ImplicitArgs::getArgDep(GenISAIntrinsic::ID id) {
    IGC_ASSERT(id != GenISAIntrinsic::ID::no_intrinsic);
    for (auto& arg : IMPLICIT_ARGS)
    {
        if (arg.getGenIntrinsicID() == id)
        {
            return arg.getDependency();
        }
    }
    return IGC::WIAnalysis::WIDependancy::RANDOM;
}

bool ImplicitArgs::hasIntrinsicSupport(ImplicitArg::ArgType i) {
    return IMPLICIT_ARGS[i].getGenIntrinsicID() != GenISAIntrinsic::no_intrinsic;
}

int32_t ImplicitArgs::getExplicitArgNum(unsigned int index) const
{
    ArgInfoMetaDataHandle argInfo = m_funcInfoMD->getImplicitArgInfoListItem(index);

    if (argInfo->isExplicitArgNumHasValue())
    {
        return argInfo->getExplicitArgNum();
    }

    return -1;
}

int32_t ImplicitArgs::getStructArgOffset(unsigned int index) const
{
    ArgInfoMetaDataHandle argInfo = m_funcInfoMD->getImplicitArgInfoListItem(index);

    if (argInfo->isStructArgOffsetHasValue())
    {
        return argInfo->getStructArgOffset();
    }

    return -1;
}

Argument* ImplicitArgs::getImplicitArg(llvm::Function& F, ImplicitArg::ArgType argType) const
{
    if (!isImplicitArgExist(argType))
        return nullptr;
    unsigned int numImplicitArgs = this->size();
    unsigned int implicitArgIndex = this->getArgIndex(argType);
    IGC_ASSERT_MESSAGE(F.arg_size() >= numImplicitArgs, "Function arg size does not match meta data args.");
    unsigned int implicitArgIndexInFunc = F.arg_size() - numImplicitArgs + implicitArgIndex;

    return F.arg_begin() + implicitArgIndexInFunc;
}

Value* ImplicitArgs::getImplicitArgValue(llvm::Function& F, ImplicitArg::ArgType argType, const IGCMD::MetaDataUtils* pMdUtils)
{
    Value* funcArg = getImplicitArg(F, argType);

    if (funcArg)
    {
        // If the function argument already exists, just return it
        return funcArg;
    }

    if (!isEntryFunc(pMdUtils, &F))
    {
        ImplicitArg iArg = IMPLICIT_ARGS[argType];
        GenISAIntrinsic::ID genID = iArg.getGenIntrinsicID();
        if (genID != GenISAIntrinsic::ID::no_intrinsic)
        {
            // Look for already existing intrinsic
            for (auto II = inst_begin(F), IE = inst_end(F); II != IE; II++)
            {
                if (GenIntrinsicInst* inst = dyn_cast<GenIntrinsicInst>(&*II))
                {
                    if (inst->getIntrinsicID() == genID)
                    {
                        // Make sure that intrinsic that we use is called before we use it's result
                        auto parentFunction = inst->getParent()->getParent();
                        auto firstFunc = parentFunction->getEntryBlock().getFirstNonPHI();
                        inst->moveBefore(firstFunc);
                        return inst;
                    }
                }
            }

            // Does not exist, create the intrinsic at function entry
            llvm::IRBuilder<> Builder(&*F.getEntryBlock().begin());
            Type* argTy = iArg.getLLVMType(F.getParent()->getContext());
            Function* intrinsicDecl = GenISAIntrinsic::getDeclaration(F.getParent(), genID, argTy);
            CallInst* inst = Builder.CreateCall(intrinsicDecl);
            return inst;
        }
    }
    return nullptr;
}

Argument* ImplicitArgs::getNumberedImplicitArg(llvm::Function& F, ImplicitArg::ArgType argType, int argNum) const
{
    IGC_ASSERT_MESSAGE((F.arg_size() >= size()), "Invalid number of arguments in the function!");

    unsigned int numImplicitArgs = size();
    unsigned int implicitArgIndex = this->getNumberedArgIndex(argType, argNum);
    if (implicitArgIndex == numImplicitArgs)
      return nullptr;
    IGC_ASSERT_MESSAGE(F.arg_size() >= numImplicitArgs, "Function arg size does not match meta data args.");
    unsigned int implicitArgIndexInFunc = F.arg_size() - numImplicitArgs + implicitArgIndex;

    return F.arg_begin() + implicitArgIndexInFunc;
}

TODO("The allocation size and alignment of constBase should change according to target bitness...")

