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

#include "ImplicitArgs.hpp"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/DerivedTypes.h>
#include <llvmWrapper/IR/Function.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

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
            m_isConstantBuf(isConstantBuf)
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
    switch(m_valType)
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

    if (m_nbElement == 1 || m_dependency != WIAnalysis::UNIFORM)
    {
        return baseType;
    }
    else
    {
        return VectorType::get(baseType, m_nbElement);
    }
}

unsigned int ImplicitArg::getNumberElements() const
{
    return m_nbElement;
}

VISA_Type ImplicitArg::getVISAType(const DataLayout& DL) const
{
    switch(m_valType)
    {
    case BYTE:
        return VISA_Type::ISA_TYPE_B;
        break;
    case SHORT:
        return VISA_Type::ISA_TYPE_W;
        break;
    case INT:
        return VISA_Type::ISA_TYPE_D;
        break;
    case LONG:
        return VISA_Type::ISA_TYPE_Q;
        break;
    case FP32:
        return VISA_Type::ISA_TYPE_F;
        break;
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
        case ALIGN_GRF:
           return IGC::EALIGN_GRF;
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
    calignmentSize as;
    return as[getAlignType(DL)];
}

WIAnalysis::WIDependancy ImplicitArg::getDependency() const {
    return m_dependency;
}

unsigned int ImplicitArg::getAllocateSize(const DataLayout& DL) const
{
    unsigned int elemSize = 0;

    switch(m_valType)
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

ImplicitArgs::ImplicitArgs(const llvm::Function& func , const MetaDataUtils* pMdUtils)
{
    if (IMPLICIT_ARGS.size() == 0)
    {
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::R0, "r0", ImplicitArg::INT, WIAnalysis::UNIFORM, 8, ImplicitArg::ALIGN_GRF, false));

        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::PAYLOAD_HEADER, "payloadHeader", ImplicitArg::INT, WIAnalysis::UNIFORM, 8, ImplicitArg::ALIGN_GRF, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::WORK_DIM, "workDim", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));

        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::NUM_GROUPS, "numWorkGroups", ImplicitArg::INT, WIAnalysis::UNIFORM, 3, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::GLOBAL_SIZE, "globalSize", ImplicitArg::INT, WIAnalysis::UNIFORM, 3, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::LOCAL_SIZE, "localSize", ImplicitArg::INT, WIAnalysis::UNIFORM, 3, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::ENQUEUED_LOCAL_WORK_SIZE, "enqueuedLocalSize", ImplicitArg::INT, WIAnalysis::UNIFORM, 3, ImplicitArg::ALIGN_DWORD, true));

        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::LOCAL_ID_X, "localIdX", ImplicitArg::SHORT, WIAnalysis::RANDOM, 16, ImplicitArg::ALIGN_GRF, false));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::LOCAL_ID_Y, "localIdY", ImplicitArg::SHORT, WIAnalysis::RANDOM, 16, ImplicitArg::ALIGN_GRF, false));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::LOCAL_ID_Z, "localIdZ", ImplicitArg::SHORT, WIAnalysis::RANDOM, 16, ImplicitArg::ALIGN_GRF, false));

        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::CONSTANT_BASE, "constBase", ImplicitArg::CONSTPTR, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_PTR, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::GLOBAL_BASE, "globalBase", ImplicitArg::GLOBALPTR, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_PTR, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::PRIVATE_BASE, "privateBase", ImplicitArg::PRIVATEPTR, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_PTR, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::PRINTF_BUFFER, "printfBuffer", ImplicitArg::GLOBALPTR, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_PTR, true));

        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::BUFFER_OFFSET, "bufferOffset", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));

        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::CONSTANT_REG_FP32, "const_reg_fp32", ImplicitArg::FP32, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::CONSTANT_REG_QWORD, "const_reg_qword", ImplicitArg::LONG, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_QWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::CONSTANT_REG_DWORD, "const_reg_dword", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::CONSTANT_REG_WORD, "const_reg_word", ImplicitArg::SHORT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::CONSTANT_REG_BYTE, "const_reg_byte", ImplicitArg::BYTE, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));

        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::IMAGE_HEIGHT, "imageHeigt", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::IMAGE_WIDTH, "imageWidth", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::IMAGE_DEPTH, "imageDepth", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::IMAGE_NUM_MIP_LEVELS, "imageNumMipLevels", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::IMAGE_CHANNEL_DATA_TYPE, "imageDataType", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::IMAGE_CHANNEL_ORDER, "imageOrder", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::IMAGE_SRGB_CHANNEL_ORDER, "imageSrgbOrder", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::IMAGE_ARRAY_SIZE, "imageArrSize", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::IMAGE_NUM_SAMPLES, "imageNumSamples", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));

        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::SAMPLER_ADDRESS, "smpAddress", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::SAMPLER_NORMALIZED, "smpNormalized", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::SAMPLER_SNAP_WA, "smpSnapWA", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::FLAT_IMAGE_BASEOFFSET, "flatImageBaseoffset", ImplicitArg::LONG, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_QWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::FLAT_IMAGE_HEIGHT, "flatImageHeight", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::FLAT_IMAGE_WIDTH, "flatImageWidth", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::FLAT_IMAGE_PITCH, "flatImagePitch", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));

        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::VME_MB_BLOCK_TYPE, "vmeMbBlockType", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::VME_SUBPIXEL_MODE, "vmeSubpixelMode", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::VME_SAD_ADJUST_MODE, "vmeSadAdjustMode", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::VME_SEARCH_PATH_TYPE, "vmeSearchPathType", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));

        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::DEVICE_ENQUEUE_DEFAULT_DEVICE_QUEUE, "deviceEnqueueDefaultDeviceQueue", ImplicitArg::GLOBALPTR, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_PTR, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::DEVICE_ENQUEUE_EVENT_POOL, "deviceEnqueueEventPool", ImplicitArg::GLOBALPTR, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_PTR, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::DEVICE_ENQUEUE_MAX_WORKGROUP_SIZE, "deviceEnqueueMaxWorkgroupSize", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::DEVICE_ENQUEUE_PARENT_EVENT, "deviceEnqueueParentEvent", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::DEVICE_ENQUEUE_PREFERED_WORKGROUP_MULTIPLE, "deviceEnqueuePreferedWorkgroupMultiple", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::GET_OBJECT_ID, "deviceEnqueueGetObjectId", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::GET_BLOCK_SIMD_SIZE, "deviceEnqueueGetBlockSimdSize", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));

        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::LOCAL_MEMORY_STATELESS_WINDOW_START_ADDRESS, "localMemStatelessWindowStartAddr", ImplicitArg::GLOBALPTR, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_PTR, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::LOCAL_MEMORY_STATELESS_WINDOW_SIZE, "localMemStatelessWindowSize", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::PRIVATE_MEMORY_STATELESS_SIZE, "PrivateMemStatelessSize", ImplicitArg::INT, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_DWORD, true));

        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::STAGE_IN_GRID_ORIGIN, "stageInGridOrigin", ImplicitArg::INT, WIAnalysis::UNIFORM, 3, ImplicitArg::ALIGN_GRF, true));
        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::STAGE_IN_GRID_SIZE, "stageInGridSize", ImplicitArg::INT, WIAnalysis::UNIFORM, 3, ImplicitArg::ALIGN_GRF, true));

        IMPLICIT_ARGS.push_back(ImplicitArg(ImplicitArg::SYNC_BUFFER, "syncBuffer", ImplicitArg::GLOBALPTR, WIAnalysis::UNIFORM, 1, ImplicitArg::ALIGN_PTR, false));

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
    }

    m_funcInfoMD = pMdUtils->getFunctionsInfoItem(const_cast<llvm::Function*>(&func));
}

const ImplicitArg& ImplicitArgs::operator[](unsigned int i) const {
    IGC_ASSERT_MESSAGE((IMPLICIT_ARGS.size() == ImplicitArg::NUM_IMPLICIT_ARGS), "Mismatch in NUM_IMPLICIT_ARGS and IMPLICIT_ARGS vector");
    return IMPLICIT_ARGS[getArgType(i)];
}

unsigned int ImplicitArgs::getArgIndex(ImplicitArg::ArgType argType) {
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

    if (funcInfo->size_ImplicitArgInfoList() <= 0) //There are no implicit arguments!
        return res;

    // Find the first appearance of the given implicit arg type
    unsigned int implicitArgIndex = 0;
    for (implicitArgIndex = 0; implicitArgIndex < funcInfo->size_ImplicitArgInfoList(); ++implicitArgIndex)
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

bool ImplicitArgs::isImplicitArgExist(ImplicitArg::ArgType argType)
{
    return isImplicitArgExist(m_funcInfoMD, argType);
}

bool ImplicitArgs::isImplicitArgExist(
    llvm::Function& F,
    ImplicitArg::ArgType argType,
    IGCMD::MetaDataUtils* pMdUtils)
{
    return isImplicitArgExist(pMdUtils->getFunctionsInfoItem(&F), argType);
}

unsigned int ImplicitArgs::getImageArgIndex(ImplicitArg::ArgType argType, const Argument* image) {
    IGC_ASSERT_MESSAGE(isImplicitImage(argType), "Non image/sampler implicit arg!");
    return getNumberedArgIndex(argType, image->getArgNo());
}

unsigned int ImplicitArgs::getNumberedArgIndex(ImplicitArg::ArgType argType, int argNum) {
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

void ImplicitArgs::addImplicitArgs(llvm::Function& F, SmallVectorImpl<ImplicitArg::ArgType>& implicitArgs, MetaDataUtils* pMdUtils) {
    // Indirect calls does not support implicit arguments!
    // Just return for now. TODO: Each pass should check if it's inserting implicit arg to indirect function
    if (F.hasFnAttribute("IndirectlyCalled"))
        return;

    // Add implicit args metadata for the given function
    FunctionInfoMetaDataHandle funcInfo = pMdUtils->getFunctionsInfoItem(&F);
    for (auto arg : implicitArgs)
    {
        if (!isImplicitArgExist(F, arg, pMdUtils))
        {
            ArgInfoMetaDataHandle argMD = ArgInfoMetaDataHandle(ArgInfoMetaData::get());
            argMD->setArgId(arg);
            funcInfo->addImplicitArgInfoListItem(argMD);
        }
    }
    pMdUtils->save(F.getParent()->getContext());
}

void ImplicitArgs::addImageArgs(llvm::Function& F, ImplicitArg::ArgMap& argMap, MetaDataUtils* pMdUtils)
{
    FunctionInfoMetaDataHandle funcInfo = pMdUtils->getFunctionsInfoItem(&F);
    for (int i = 0; i < numImageArgTypes; ++i)
    {
        ImplicitArg::ArgValSet& argSet = argMap[IndexToArgType.at(i)];
        for (const auto& argI : argSet)
        {
            ArgInfoMetaDataHandle argMD = ArgInfoMetaDataHandle(ArgInfoMetaData::get());
            argMD->setArgId(IndexToArgType.at(i));
            argMD->setExplicitArgNum(argI);
            funcInfo->addImplicitArgInfoListItem(argMD);
        }
    }
    pMdUtils->save(F.getParent()->getContext());
}

void ImplicitArgs::addStructArgs(llvm::Function& F, const Argument* A, const ImplicitArg::StructArgList& S, MetaDataUtils* pMdUtils)
{
    FunctionInfoMetaDataHandle funcInfo = pMdUtils->getFunctionsInfoItem(&F);

    for (auto argI = S.begin(), end = S.end(); argI != end; ++argI) {
        unsigned int id = argI->first;
        unsigned int offset = argI->second;

        ArgInfoMetaDataHandle argMD = ArgInfoMetaDataHandle(ArgInfoMetaData::get());
        argMD->setExplicitArgNum(A->getArgNo());
        argMD->setArgId(id);
        argMD->setStructArgOffset(offset);
        funcInfo->addImplicitArgInfoListItem(argMD);
    }

    pMdUtils->save(F.getParent()->getContext());
}

void ImplicitArgs::addNumberedArgs(llvm::Function& F, ImplicitArg::ArgMap& argMap, IGCMD::MetaDataUtils* pMdUtils)
{
  FunctionInfoMetaDataHandle funcInfo = pMdUtils->getFunctionsInfoItem(&F);
  for (const auto& argPair : argMap)
  {
    ImplicitArg::ArgType argId = argPair.first;
    ImplicitArg::ArgValSet argSet = argPair.second;
    for (const auto& argNum : argSet)
    {
      ArgInfoMetaDataHandle argMD = ArgInfoMetaDataHandle(ArgInfoMetaData::get());
      argMD->setArgId(argId);
      argMD->setExplicitArgNum(argNum);
      funcInfo->addImplicitArgInfoListItem(argMD);
    }
  }
  pMdUtils->save(F.getParent()->getContext());
}

// Add one implicit argument for each pointer argument to global or constant buffer.
// Note that F is the original input function (ie, without implicit arguments).
void ImplicitArgs::addBufferOffsetArgs(llvm::Function& F, IGCMD::MetaDataUtils* pMdUtils, IGC::ModuleMetaData *modMD)
{
    ImplicitArg::ArgMap OffsetArgs;
    FunctionInfoMetaDataHandle funcInfoMD =
        pMdUtils->getFunctionsInfoItem(const_cast<Function*>(&F));

    IGC_ASSERT(modMD->FuncMD.find(&F) != modMD->FuncMD.end());

    // StatelessToStatefull optimization is not applied on non-kernel functions.
    if (!isEntryFunc(pMdUtils, &F))
        return;

    FunctionMetaData* funcMD = &modMD->FuncMD.find(&F)->second;
    for (auto& Arg : F.args() )
    {
        Value* AV = &Arg;
        PointerType* PTy = dyn_cast<PointerType>(AV->getType());
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

ImplicitArg::ArgType ImplicitArgs::getArgType(unsigned int index) const {
    IGC_ASSERT_MESSAGE((index < size()), "Index out of range");
    ArgInfoMetaDataHandle argInfo = m_funcInfoMD->getImplicitArgInfoListItem(index);
    return (ImplicitArg::ArgType)argInfo->getArgId();
}

int32_t ImplicitArgs::getExplicitArgNum(unsigned int index) const
{
    ArgInfoMetaDataHandle argInfo = m_funcInfoMD->getImplicitArgInfoListItem(index);

    if(argInfo->isExplicitArgNumHasValue())
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


TODO("Refactor code to avoid code triplication for getArgInFunc(), getImplicitArg() and WIFuncResolution::getImplicitArg()")
Argument* ImplicitArgs::getArgInFunc(llvm::Function& F, ImplicitArg::ArgType argType) {
    IGC_ASSERT_MESSAGE((IGCLLVM::GetFuncArgSize(F) >= size()), "Invalid number of argumnents in the function!");

    unsigned int argIndex       =  getArgIndex(argType);
    unsigned int argIndexInFunc = IGCLLVM::GetFuncArgSize(F) - size() + argIndex;
    Function::arg_iterator arg  = F.arg_begin();
    for (unsigned int i = 0; i < argIndexInFunc; ++i,  ++arg);

    return &(*arg);
}

Argument* ImplicitArgs::getImplicitArg(llvm::Function& F, ImplicitArg::ArgType argType)
{
    unsigned int numImplicitArgs = this->size();
    if (!isImplicitArgExist(argType))
        return nullptr;
    unsigned int implicitArgIndex = this->getArgIndex(argType);

    unsigned int implicitArgIndexInFunc = IGCLLVM::GetFuncArgSize(F) - numImplicitArgs + implicitArgIndex;

    Function::arg_iterator arg = F.arg_begin();
    for (unsigned int i = 0; i < implicitArgIndexInFunc; ++i, ++arg);

    return &(*arg);
}

Argument* ImplicitArgs::getNumberedImplicitArg(llvm::Function& F, ImplicitArg::ArgType argType, int argNum)
{
    IGC_ASSERT_MESSAGE((IGCLLVM::GetFuncArgSize(F) >= size()), "Invalid number of arguments in the function!");

    unsigned int numImplicitArgs = size();
    unsigned int implicitArgIndex = this->getNumberedArgIndex(argType, argNum);
    if (implicitArgIndex == numImplicitArgs)
      return nullptr;

    unsigned int implicitArgIndexInFunc = IGCLLVM::GetFuncArgSize(F) - numImplicitArgs + implicitArgIndex;

    Function::arg_iterator arg = F.arg_begin();
    for (unsigned int i = 0; i < implicitArgIndexInFunc; ++i, ++arg);

    return &(*arg);
}

TODO("The allocation size and alignment of constBase should change according to target bitness...")

const int ImplicitArgs::numImageArgTypes = ImplicitArg::IMAGES_END - ImplicitArg::IMAGES_START + 1;

std::map<int, ImplicitArg::ArgType> ImplicitArgs::initIndexToArgMap()
{
    std::map<int, ImplicitArg::ArgType> map;
    for (int i = 0; i < ImplicitArgs::numImageArgTypes; ++i)
    {
        map[i] = (ImplicitArg::ArgType)(ImplicitArg::IMAGES_START + i);
    }
    return map;
}
const std::map<int, ImplicitArg::ArgType> ImplicitArgs::IndexToArgType = initIndexToArgMap();
