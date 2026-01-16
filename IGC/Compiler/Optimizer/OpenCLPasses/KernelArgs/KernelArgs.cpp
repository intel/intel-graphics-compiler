/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/KernelArgs/KernelArgs.hpp"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Argument.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Argument.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Metadata.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace IGC;
using namespace IGC::IGCMD;
using namespace llvm;

KernelArg::KernelArg(KernelArg::ArgType argType, KernelArg::AccessQual accessQual, unsigned int allocateSize,
                     unsigned int elemAllocateSize, size_t align, bool isConstantBuf, const llvm::Argument *arg,
                     unsigned int associatedArgNo)
    : m_implicitArgument(false), m_argType(argType), m_accessQual(accessQual), m_allocateSize(allocateSize), // in BYTES
      m_elemAllocateSize(elemAllocateSize), m_align(align), m_isConstantBuf(isConstantBuf), m_arg(arg),
      m_associatedArgNo(associatedArgNo), m_structArgOffset(-1), m_locationIndex(-1), m_locationCount(-1),
      m_needsAllocation(typeAlwaysNeedsAllocation()), m_isEmulationArgument(false), m_imageInfo({false, false}),
      m_isScalarAsPointer(false) {}

KernelArg::KernelArg(const Argument *arg, const DataLayout *DL, const StringRef typeStr, const StringRef qualStr,
                     int location_index, int location_count, bool needBindlessHandle, bool isEmulationArgument,
                     bool isScalarAsPointer)
    : m_implicitArgument(false), m_argType(calcArgType(arg, typeStr)), m_accessQual(calcAccessQual(arg, qualStr)),
      // Only explicit arguments that need allocation are part of the constant buffer
      m_isConstantBuf(needBindlessHandle || typeAlwaysNeedsAllocation()), m_arg(arg),
      m_associatedArgNo(arg->getArgNo()), m_structArgOffset(-1), m_locationIndex(location_index),
      m_locationCount(location_count), m_needsAllocation(needBindlessHandle || typeAlwaysNeedsAllocation()),
      m_isEmulationArgument(isEmulationArgument), m_imageInfo({false, false}), m_isScalarAsPointer(isScalarAsPointer) {
  m_allocateSize = calcAllocateSize(arg, DL);
  m_elemAllocateSize = calcElemAllocateSize(arg, DL);
  m_align = (size_t)calcAlignment(arg, DL);
}

KernelArg::KernelArg(const ImplicitArg &implicitArg, const DataLayout *DL, const Argument *arg,
                     unsigned int ExplicitArgNo, unsigned int structArgOffset, bool isScalarAsPointer,
                     unsigned int GRFSize)
    : m_implicitArgument(true), m_argType(calcArgType(implicitArg)), m_accessQual(AccessQual::NONE),
      m_allocateSize(implicitArg.getAllocateSize(*DL)), m_align(implicitArg.getAlignment(*DL)),
      m_isConstantBuf(implicitArg.isConstantBuf()), m_arg(arg),
      m_associatedArgNo(calcAssociatedArgNo(implicitArg, arg, ExplicitArgNo)), m_structArgOffset(structArgOffset),
      m_locationIndex(-1), m_locationCount(-1), m_needsAllocation(typeAlwaysNeedsAllocation()),
      m_isEmulationArgument(false), m_imageInfo({false, false}), m_isScalarAsPointer(isScalarAsPointer) {
  IGC_ASSERT(implicitArg.getNumberElements());

  m_elemAllocateSize = m_allocateSize / implicitArg.getNumberElements();
  if (implicitArg.isLocalIDs() && GRFSize == 64) {
    m_elemAllocateSize = m_allocateSize / (GRFSize / 2);
  }
}

unsigned int KernelArg::calcAllocateSize(const Argument *arg, const DataLayout *DL) const {
  if (!needsAllocation())
    return 0;

  return int_cast<unsigned int>(DL->getTypeAllocSize(arg->getType()));
}

alignment_t KernelArg::calcAlignment(const Argument *arg, const DataLayout *DL) const {
  // If we don't need to allocate, we certainly don't need alignment
  if (!needsAllocation())
    return 0;

  if (arg->hasAttribute(llvm::Attribute::Alignment)) {
    uint64_t align = arg->getParamAlign().valueOrOne().value();
    // Note that align 1 has no effect on non-byval, non-preallocated arguments.
    if (align != 1 || arg->hasPreallocatedAttr() || arg->hasByValAttr())
      return align;
  }

  Type *typeToAlign = arg->getType();
  // Usually, we return the alignment of the parameter type.
  // For local pointers, we need the alignment of the *contained* type.
  if (m_argType == ArgType::PTR_LOCAL) {
    typeToAlign = IGCLLVM::getArgAttrEltTy(arg);
    if (typeToAlign == nullptr && !IGCLLVM::isPointerTy(arg->getType()))
      typeToAlign = IGCLLVM::getNonOpaquePtrEltTy(arg->getType());
  }

  if (typeToAlign != nullptr)
    return DL->getABITypeAlign(typeToAlign).value();
  return /*MinimumAlignment*/ 1;
}

unsigned int KernelArg::calcElemAllocateSize(const Argument *arg, const DataLayout *DL) const {
  if (!needsAllocation())
    return 0;

  return int_cast<unsigned int>(DL->getTypeAllocSize(arg->getType()->getScalarType()));
}

// First member of pair is ArgType of buffer.
// When ArgType is SAMPLER, second member should be true.
// When ArgType is NOT_TO_ALLOCATE, second member should be false.
// ArgType enum uses same values for SAMPLER and NOT_TO_ALLOCATE.
// This function helps disambiguate between the two values.
KernelArg::BufferArgType KernelArg::getBufferType(const Argument *arg, const StringRef typeStr) {
  if (arg->getType()->getTypeID() != Type::PointerTyID)
    return {KernelArg::ArgType::SAMPLER, true};

  PointerType *ptrType = cast<PointerType>(arg->getType());

  int address_space = ptrType->getPointerAddressSpace();
  bool directIdx = false;
  unsigned int bufId = 0;
  BufferType bufType = DecodeAS4GFXResource(address_space, directIdx, bufId);

  // Check if this arg is an image
  if (bufType == BufferType::UAV) {
    ArgType imgArgType;
    // Check if argument is image
    if (isImage(arg, typeStr, imgArgType))
      return {imgArgType, false};
  } else if (bufType == BufferType::SAMPLER)
    return {KernelArg::ArgType::SAMPLER, true};

  return {KernelArg::ArgType::NOT_TO_ALLOCATE, false};
}

KernelArg::ArgType KernelArg::calcArgType(const Argument *arg, const StringRef typeStr) {
  switch (arg->getType()->getTypeID()) {

  case Type::PointerTyID: {
    PointerType *ptrType = cast<PointerType>(arg->getType());

    // Check for pointer address space
    switch (ptrType->getAddressSpace()) {
    case ADDRESS_SPACE_PRIVATE: {

      Type *type = arg->getType();
      if (typeStr.equals("queue_t") || typeStr.equals("spirv.Queue")) {
        return KernelArg::ArgType::PTR_DEVICE_QUEUE;
      } else if (arg->hasByValAttr() && type->isPointerTy() && arg->getParamByValType()->isStructTy()) {
        // Pass by value structs will show up as private pointer
        // arguments in the function signiture.
        return KernelArg::ArgType::STRUCT;
      } else {
        return KernelArg::ArgType::IMPLICIT_PRIVATE_BASE;
      }
    }

    case ADDRESS_SPACE_GLOBAL: {
      ArgType imgArgType;
      // Check if argument is image
      if (isImage(arg, typeStr, imgArgType))
        return imgArgType;
    }
      return KernelArg::ArgType::PTR_GLOBAL;

    case ADDRESS_SPACE_CONSTANT:
      // Bindless samplers are stored in addrspace(2)
      if (isSampler(arg, typeStr))
        return KernelArg::ArgType::SAMPLER;
      else if (isBindlessSampler(arg, typeStr))
        return KernelArg::ArgType::BINDLESS_SAMPLER;

      return KernelArg::ArgType::PTR_CONSTANT;
    case ADDRESS_SPACE_LOCAL:
      return KernelArg::ArgType::PTR_LOCAL;

    default:
#if 0
            // Need to disable this assertion for two-phase-inlining, i.e.
            // kernel arguments will be used for subroutines, which may
            // have arguments from other address spaces. It is unfortunate
            // that we cannot run ResourceAllocator only on kernels since
            // BuiltinsConverter checks caller's resource allocation info.
            //
            // For the final codegen, this allocation info is only queried
            // for kernels. This should not affect correctness, but a waste
            // on subroutines.
            //
            // FIXME: There is a chain of dependency.
            IGC_ASSERT_MESSAGE(0, "Unrecognized address space");
#endif
      // This is a buffer. Try to decode this
      return getBufferType(arg, typeStr).type;
    }
  }
  case Type::IntegerTyID:
    // Check if argument is sampler
    if (isSampler(arg, typeStr))
      return KernelArg::ArgType::SAMPLER;
    // Fall through to default

  default:
    // May reach here from Type::IntegerTyID
    return KernelArg::ArgType::CONSTANT_REG;
  }
}

KernelArg::ArgType KernelArg::calcArgType(const ImplicitArg &arg) const {
  switch (arg.getArgType()) {
  case ImplicitArg::R0:
    return KernelArg::ArgType::IMPLICIT_R0;
  case ImplicitArg::PAYLOAD_HEADER:
    return KernelArg::ArgType::IMPLICIT_PAYLOAD_HEADER;
  case ImplicitArg::GLOBAL_OFFSET:
    return KernelArg::ArgType::IMPLICIT_GLOBAL_OFFSET;
  case ImplicitArg::PRIVATE_BASE:
    return KernelArg::ArgType::IMPLICIT_PRIVATE_BASE;
  case ImplicitArg::CONSTANT_BASE:
    return KernelArg::ArgType::IMPLICIT_CONSTANT_BASE;
  case ImplicitArg::PRINTF_BUFFER:
    return KernelArg::ArgType::IMPLICIT_PRINTF_BUFFER;
  case ImplicitArg::SYNC_BUFFER:
    return KernelArg::ArgType::IMPLICIT_SYNC_BUFFER;
  case ImplicitArg::BUFFER_OFFSET:
    return KernelArg::ArgType::IMPLICIT_BUFFER_OFFSET;
  case ImplicitArg::GLOBAL_BASE:
    return KernelArg::ArgType::IMPLICIT_GLOBAL_BASE;
  case ImplicitArg::WORK_DIM:
    return KernelArg::ArgType::IMPLICIT_WORK_DIM;
  case ImplicitArg::NUM_GROUPS:
    return KernelArg::ArgType::IMPLICIT_NUM_GROUPS;
  case ImplicitArg::GLOBAL_SIZE:
    return KernelArg::ArgType::IMPLICIT_GLOBAL_SIZE;
  case ImplicitArg::LOCAL_SIZE:
    return KernelArg::ArgType::IMPLICIT_LOCAL_SIZE;
  case ImplicitArg::ENQUEUED_LOCAL_WORK_SIZE:
    return KernelArg::ArgType::IMPLICIT_ENQUEUED_LOCAL_WORK_SIZE;
  case ImplicitArg::LOCAL_ID_X:
    return KernelArg::ArgType::IMPLICIT_LOCAL_ID_X;
  case ImplicitArg::LOCAL_ID_Y:
    return KernelArg::ArgType::IMPLICIT_LOCAL_ID_Y;
  case ImplicitArg::LOCAL_ID_Z:
    return KernelArg::ArgType::IMPLICIT_LOCAL_ID_Z;
  case ImplicitArg::BUFFER_SIZE:
    return KernelArg::ArgType::IMPLICIT_BUFFER_SIZE;
  case ImplicitArg::STAGE_IN_GRID_ORIGIN:
    return KernelArg::ArgType::IMPLICIT_STAGE_IN_GRID_ORIGIN;
  case ImplicitArg::STAGE_IN_GRID_SIZE:
    return KernelArg::ArgType::IMPLICIT_STAGE_IN_GRID_SIZE;
  case ImplicitArg::CONSTANT_REG_FP32:
    return KernelArg::ArgType::CONSTANT_REG;
  case ImplicitArg::CONSTANT_REG_QWORD:
    return KernelArg::ArgType::CONSTANT_REG;
  case ImplicitArg::CONSTANT_REG_DWORD:
    return KernelArg::ArgType::CONSTANT_REG;
  case ImplicitArg::CONSTANT_REG_WORD:
    return KernelArg::ArgType::CONSTANT_REG;
  case ImplicitArg::CONSTANT_REG_BYTE:
    return KernelArg::ArgType::CONSTANT_REG;

  case ImplicitArg::IMAGE_HEIGHT:
    return KernelArg::ArgType::IMPLICIT_IMAGE_HEIGHT;
  case ImplicitArg::IMAGE_WIDTH:
    return KernelArg::ArgType::IMPLICIT_IMAGE_WIDTH;
  case ImplicitArg::IMAGE_DEPTH:
    return KernelArg::ArgType::IMPLICIT_IMAGE_DEPTH;
  case ImplicitArg::IMAGE_NUM_MIP_LEVELS:
    return KernelArg::ArgType::IMPLICIT_IMAGE_NUM_MIP_LEVELS;
  case ImplicitArg::IMAGE_CHANNEL_DATA_TYPE:
    return KernelArg::ArgType::IMPLICIT_IMAGE_CHANNEL_DATA_TYPE;
  case ImplicitArg::IMAGE_CHANNEL_ORDER:
    return KernelArg::ArgType::IMPLICIT_IMAGE_CHANNEL_ORDER;
  case ImplicitArg::IMAGE_SRGB_CHANNEL_ORDER:
    return KernelArg::ArgType::IMPLICIT_IMAGE_SRGB_CHANNEL_ORDER;
  case ImplicitArg::IMAGE_ARRAY_SIZE:
    return KernelArg::ArgType::IMPLICIT_IMAGE_ARRAY_SIZE;
  case ImplicitArg::IMAGE_NUM_SAMPLES:
    return KernelArg::ArgType::IMPLICIT_IMAGE_NUM_SAMPLES;
  case ImplicitArg::SAMPLER_ADDRESS:
    return KernelArg::ArgType::IMPLICIT_SAMPLER_ADDRESS;
  case ImplicitArg::SAMPLER_NORMALIZED:
    return KernelArg::ArgType::IMPLICIT_SAMPLER_NORMALIZED;
  case ImplicitArg::SAMPLER_SNAP_WA:
    return KernelArg::ArgType::IMPLICIT_SAMPLER_SNAP_WA;
  case ImplicitArg::INLINE_SAMPLER:
    return KernelArg::ArgType::IMPLICIT_INLINE_SAMPLER;

  case ImplicitArg::VME_MB_BLOCK_TYPE:
    return KernelArg::ArgType::IMPLICIT_VME_MB_BLOCK_TYPE;
  case ImplicitArg::VME_SUBPIXEL_MODE:
    return KernelArg::ArgType::IMPLICIT_VME_SUBPIXEL_MODE;
  case ImplicitArg::VME_SAD_ADJUST_MODE:
    return KernelArg::ArgType::IMPLICIT_VME_SAD_ADJUST_MODE;
  case ImplicitArg::VME_SEARCH_PATH_TYPE:
    return KernelArg::ArgType::IMPLICIT_VME_SEARCH_PATH_TYPE;

  case ImplicitArg::DEVICE_ENQUEUE_DEFAULT_DEVICE_QUEUE:
    return KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_DEFAULT_DEVICE_QUEUE;
  case ImplicitArg::DEVICE_ENQUEUE_EVENT_POOL:
    return KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_EVENT_POOL;
  case ImplicitArg::DEVICE_ENQUEUE_MAX_WORKGROUP_SIZE:
    return KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_MAX_WORKGROUP_SIZE;
  case ImplicitArg::DEVICE_ENQUEUE_PARENT_EVENT:
    return KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_PARENT_EVENT;
  case ImplicitArg::DEVICE_ENQUEUE_PREFERED_WORKGROUP_MULTIPLE:
    return KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_PREFERED_WORKGROUP_MULTIPLE;
  case ImplicitArg::GET_OBJECT_ID:
    return KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_DATA_PARAMETER_OBJECT_ID;
  case ImplicitArg::GET_BLOCK_SIMD_SIZE:
    return KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_DISPATCHER_SIMD_SIZE;

  case ImplicitArg::LOCAL_MEMORY_STATELESS_WINDOW_START_ADDRESS:
    return KernelArg::ArgType::IMPLICIT_LOCAL_MEMORY_STATELESS_WINDOW_START_ADDRESS;
  case ImplicitArg::LOCAL_MEMORY_STATELESS_WINDOW_SIZE:
    return KernelArg::ArgType::IMPLICIT_LOCAL_MEMORY_STATELESS_WINDOW_SIZE;
  case ImplicitArg::PRIVATE_MEMORY_STATELESS_SIZE:
    return KernelArg::ArgType::IMPLICIT_PRIVATE_MEMORY_STATELESS_SIZE;
  case ImplicitArg::RT_STACK_ID:
    return KernelArg::ArgType::RT_STACK_ID;
  case ImplicitArg::RT_GLOBAL_BUFFER_POINTER:
    return KernelArg::ArgType::IMPLICIT_RT_GLOBAL_BUFFER;
  case ImplicitArg::BINDLESS_OFFSET:
    return KernelArg::ArgType::IMPLICIT_BINDLESS_OFFSET;

  case ImplicitArg::IMPLICIT_ARG_BUFFER_PTR:
    return KernelArg::ArgType::IMPLICIT_ARG_BUFFER;

  case ImplicitArg::ASSERT_BUFFER_POINTER:
    return KernelArg::ArgType::IMPLICIT_ASSERT_BUFFER;
  case ImplicitArg::INDIRECT_DATA_POINTER:
    return KernelArg::ArgType::IMPLICIT_INDIRECT_DATA_POINTER;
  case ImplicitArg::SCRATCH_POINTER:
    return KernelArg::ArgType::IMPLICIT_SCRATCH_POINTER;
  case ImplicitArg::REGION_GROUP_SIZE:
    return KernelArg::ArgType::IMPLICIT_REGION_GROUP_SIZE;
  case ImplicitArg::REGION_GROUP_WG_COUNT:
    return KernelArg::ArgType::IMPLICIT_REGION_GROUP_WG_COUNT;
  case ImplicitArg::REGION_GROUP_BARRIER_BUFFER:
    return KernelArg::ArgType::IMPLICIT_REGION_GROUP_BARRIER_BUFFER;
  default:
    return KernelArg::ArgType::NOT_TO_ALLOCATE;
  }
}

KernelArg::AccessQual KernelArg::calcAccessQual(const Argument *arg, const StringRef qualStr) const {
  if (qualStr.equals("read_write"))
    return READ_WRITE;

  if (qualStr.startswith("read"))
    return READ_ONLY;

  if (qualStr.startswith("write"))
    return WRITE_ONLY;

  return NONE;
}

unsigned int KernelArg::calcAssociatedArgNo(const ImplicitArg &implicitArg, const Argument *arg,
                                            unsigned int ExplicitArgNo) const {
  ImplicitArg::ArgType argType = implicitArg.getArgType();
  if ((ImplicitArgs::isImplicitImage(argType)) || (ImplicitArgs::isImplicitStruct(argType)) ||
      (argType == ImplicitArg::GET_OBJECT_ID) || (argType == ImplicitArg::GET_BLOCK_SIMD_SIZE) ||
      (argType == ImplicitArg::BUFFER_OFFSET) || (argType == ImplicitArg::BINDLESS_OFFSET)) {
    // For implicit image and sampler and struct arguments and buffer offset,
    // the implicit arg's value represents the index of the associated
    // image/sampler/pointer argument
    return ExplicitArgNo;
  }
  return arg->getArgNo();
}

unsigned int KernelArg::getNumComponents() const {
  if (IGCLLVM::FixedVectorType *vecType = dyn_cast<IGCLLVM::FixedVectorType>(m_arg->getType())) {
    // Vector
    return int_cast<unsigned int>(vecType->getNumElements());
  }

  // Scalar
  return 1;
}

size_t KernelArg::getAlignment() const { return m_align; }

unsigned int KernelArg::getAllocateSize() const {
  return int_cast<unsigned int>(llvm::alignTo(m_allocateSize, iOpenCL::DATA_PARAMETER_DATA_SIZE));
}

unsigned int KernelArg::getSize() const { return m_allocateSize; }

unsigned int KernelArg::getElemAllocateSize() const { return m_elemAllocateSize; }

bool KernelArg::isConstantBuf() const { return m_isConstantBuf; }

bool KernelArg::typeAlwaysNeedsAllocation() const { return m_argType < KernelArg::ArgType::NOT_TO_ALLOCATE; }

bool KernelArg::needsAllocation() const { return m_needsAllocation; }

KernelArg::ArgType KernelArg::getArgType() const { return m_argType; }

KernelArg::AccessQual KernelArg::getAccessQual() const { return m_accessQual; }

const Argument *KernelArg::getArg() const { return m_arg; }

unsigned int KernelArg::getAssociatedArgNo() const { return m_associatedArgNo; }

unsigned int KernelArg::getStructArgOffset() const { return m_structArgOffset; }

unsigned int KernelArg::getLocationCount() const { return m_locationCount; }

unsigned int KernelArg::getLocationIndex() const { return m_locationIndex; }

bool KernelArg::isImage(const Argument *arg, const StringRef typeStr, ArgType &imageArgType) {
  if (!typeStr.startswith("image") && !typeStr.startswith("bindless"))
    return false;

  // Get the original OpenCL type from the metadata and check if it's an image
  // clang 3.8 introduced a new type mangling that includes the image access qualifier.
  // Accept those too.
  std::vector<std::string> accessQual{"_t", "_ro_t", "_wo_t", "_rw_t"};
  for (auto &postfix : accessQual) {
    if (typeStr.equals("image1d" + postfix)) {
      imageArgType = ArgType::IMAGE_1D;
      return true;
    }

    if (typeStr.equals("image1d_buffer" + postfix)) {
      imageArgType = ArgType::IMAGE_1D_BUFFER;
      return true;
    }

    if (typeStr.equals("image2d" + postfix)) {
      imageArgType = ArgType::IMAGE_2D;
      return true;
    }

    if (typeStr.equals("image2d_depth" + postfix)) {
      imageArgType = ArgType::IMAGE_2D_DEPTH;
      return true;
    }

    if (typeStr.equals("image2d_msaa" + postfix)) {
      imageArgType = ArgType::IMAGE_2D_MSAA;
      return true;
    }

    if (typeStr.equals("image2d_msaa_depth" + postfix)) {
      imageArgType = ArgType::IMAGE_2D_MSAA_DEPTH;
      return true;
    }

    if (typeStr.equals("image3d" + postfix)) {
      imageArgType = ArgType::IMAGE_3D;
      return true;
    }

    if (typeStr.equals("image1d_array" + postfix)) {
      imageArgType = ArgType::IMAGE_1D_ARRAY;
      return true;
    }

    if (typeStr.equals("image2d_array" + postfix)) {
      imageArgType = ArgType::IMAGE_2D_ARRAY;
      return true;
    }

    if (typeStr.equals("image2d_array_depth" + postfix)) {
      imageArgType = ArgType::IMAGE_2D_DEPTH_ARRAY;
      return true;
    }

    if (typeStr.equals("image2d_array_msaa" + postfix)) {
      imageArgType = ArgType::IMAGE_2D_MSAA_ARRAY;
      return true;
    }

    if (typeStr.equals("image2d_array_msaa_depth" + postfix)) {
      imageArgType = ArgType::IMAGE_2D_MSAA_DEPTH_ARRAY;
      return true;
    }
  }

  // See if these are address space decoded args.
  // Get the original OpenCL type from the metadata and check if it's an image
  if (typeStr.equals("bindless_image1d_t")) {
    imageArgType = ArgType::BINDLESS_IMAGE_1D;
    return true;
  }

  if (typeStr.equals("bindless_image1d_buffer_t")) {
    imageArgType = ArgType::BINDLESS_IMAGE_1D_BUFFER;
    return true;
  }

  if (typeStr.equals("bindless_image2d_t")) {
    imageArgType = ArgType::BINDLESS_IMAGE_2D;
    return true;
  }

  if (typeStr.equals("bindless_image2d_depth_t")) {
    imageArgType = ArgType::BINDLESS_IMAGE_2D_DEPTH;
    return true;
  }

  if (typeStr.equals("bindless_image2d_msaa_t")) {
    imageArgType = ArgType::BINDLESS_IMAGE_2D_MSAA;
    return true;
  }

  if (typeStr.equals("bindless_image2d_msaa_depth_t")) {
    imageArgType = ArgType::BINDLESS_IMAGE_2D_MSAA_DEPTH;
    return true;
  }

  if (typeStr.equals("bindless_image3d_t")) {
    imageArgType = ArgType::BINDLESS_IMAGE_3D;
    return true;
  }

  if (typeStr.equals("bindless_image_cube_array_t")) {
    imageArgType = ArgType::BINDLESS_IMAGE_CUBE_ARRAY;
    return true;
  }

  if (typeStr.equals("bindless_image_cube_t")) {
    imageArgType = ArgType::BINDLESS_IMAGE_CUBE;
    return true;
  }

  if (typeStr.equals("bindless_image1d_array_t")) {
    imageArgType = ArgType::BINDLESS_IMAGE_1D_ARRAY;
    return true;
  }

  if (typeStr.equals("bindless_image2d_array_t")) {
    imageArgType = ArgType::BINDLESS_IMAGE_2D_ARRAY;
    return true;
  }

  if (typeStr.equals("bindless_image2d_array_depth_t")) {
    imageArgType = ArgType::BINDLESS_IMAGE_2D_DEPTH_ARRAY;
    return true;
  }

  if (typeStr.equals("bindless_image2d_array_msaa_t")) {
    imageArgType = ArgType::BINDLESS_IMAGE_2D_MSAA_ARRAY;
    return true;
  }

  if (typeStr.equals("bindless_image2d_array_msaa_depth_t")) {
    imageArgType = ArgType::BINDLESS_IMAGE_2D_MSAA_DEPTH_ARRAY;
    return true;
  }

  if (typeStr.equals("bindless_image_cube_array_depth_t")) {
    imageArgType = ArgType::BINDLESS_IMAGE_CUBE_DEPTH_ARRAY;
    return true;
  }

  if (typeStr.equals("bindless_image_cube_depth_t")) {
    imageArgType = ArgType::BINDLESS_IMAGE_CUBE_DEPTH;
    return true;
  }

  return false;
}

bool KernelArg::isSampler(const Argument *arg, const StringRef typeStr) {
  // Get the original OpenCL type from the metadata and check if it's a sampler
  return (typeStr.equals("sampler_t"));
}

bool KernelArg::isBindlessSampler(const Argument *arg, const StringRef typeStr) {
  return (typeStr.equals("bindless_sampler_t"));
}

bool KernelArg::isArgPtrType() {
  return m_argType == ArgType::PTR_LOCAL || m_argType == ArgType::PTR_GLOBAL || m_argType == ArgType::PTR_CONSTANT ||
         m_argType == ArgType::PTR_DEVICE_QUEUE;
}

bool KernelArg::isImplicitLocalId() {
  return m_argType == ArgType::IMPLICIT_LOCAL_ID_X || m_argType == ArgType::IMPLICIT_LOCAL_ID_Y ||
         m_argType == ArgType::IMPLICIT_LOCAL_ID_Z;
}

bool KernelArgsOrder::VerifyOrder(std::array<KernelArg::ArgType, static_cast<int32_t>(KernelArg::ArgType::End)> &order,
                                  KernelArg::ArgType sent) {
  bool validOrder = false;
  // It's not safe to iterate over a random generated sentinel
  if (order[static_cast<uint>(KernelArg::ArgType::End) - 1] == sent) {
    order[static_cast<uint>(KernelArg::ArgType::End) - 1] = KernelArg::ArgType::Default;
    validOrder = true;
  } else {
    IGC_ASSERT(0);
  }

  return validOrder;
}

void KernelArgsOrder::TransposeGenerateOrder(
    std::array<KernelArg::ArgType, static_cast<int32_t>(KernelArg::ArgType::End)> &order) {
  int i = 0;

  for (const auto &j : order) {
    m_position[static_cast<uint32_t>(j)] = i++;
  }
}

KernelArgsOrder::KernelArgsOrder(InputType layout) {
  const KernelArg::ArgType SENTINEL = KernelArg::ArgType::End;

  switch (layout) {
  case InputType::INDEPENDENT:
  case InputType::CURBE: {
    std::array<KernelArg::ArgType, static_cast<int32_t>(KernelArg::ArgType::End)> CURBE = {
        KernelArg::ArgType::IMPLICIT_R0,

        KernelArg::ArgType::RUNTIME_VALUE,
        KernelArg::ArgType::IMPLICIT_INDIRECT_DATA_POINTER,
        KernelArg::ArgType::IMPLICIT_SCRATCH_POINTER,
        KernelArg::ArgType::IMPLICIT_PAYLOAD_HEADER,
        KernelArg::ArgType::IMPLICIT_GLOBAL_OFFSET,
        KernelArg::ArgType::IMPLICIT_ENQUEUED_LOCAL_WORK_SIZE,

        KernelArg::ArgType::PTR_LOCAL,
        KernelArg::ArgType::PTR_GLOBAL,
        KernelArg::ArgType::PTR_CONSTANT,
        KernelArg::ArgType::PTR_DEVICE_QUEUE,

        KernelArg::ArgType::CONSTANT_REG,

        KernelArg::ArgType::IMPLICIT_CONSTANT_BASE,
        KernelArg::ArgType::IMPLICIT_GLOBAL_BASE,
        KernelArg::ArgType::IMPLICIT_PRIVATE_BASE,
        KernelArg::ArgType::IMPLICIT_PRINTF_BUFFER,
        KernelArg::ArgType::IMPLICIT_SYNC_BUFFER,
        KernelArg::ArgType::IMPLICIT_RT_GLOBAL_BUFFER,
        KernelArg::ArgType::IMPLICIT_BUFFER_OFFSET,
        KernelArg::ArgType::IMPLICIT_WORK_DIM,
        KernelArg::ArgType::IMPLICIT_NUM_GROUPS,
        KernelArg::ArgType::IMPLICIT_GLOBAL_SIZE,
        KernelArg::ArgType::IMPLICIT_LOCAL_SIZE,
        KernelArg::ArgType::IMPLICIT_STAGE_IN_GRID_ORIGIN,
        KernelArg::ArgType::IMPLICIT_STAGE_IN_GRID_SIZE,
        KernelArg::ArgType::IMPLICIT_REGION_GROUP_SIZE,
        KernelArg::ArgType::IMPLICIT_REGION_GROUP_WG_COUNT,
        KernelArg::ArgType::IMPLICIT_REGION_GROUP_BARRIER_BUFFER,
        KernelArg::ArgType::IMPLICIT_BINDLESS_OFFSET,

        KernelArg::ArgType::IMPLICIT_IMAGE_HEIGHT,
        KernelArg::ArgType::IMPLICIT_IMAGE_WIDTH,
        KernelArg::ArgType::IMPLICIT_IMAGE_DEPTH,
        KernelArg::ArgType::IMPLICIT_IMAGE_NUM_MIP_LEVELS,
        KernelArg::ArgType::IMPLICIT_IMAGE_CHANNEL_DATA_TYPE,
        KernelArg::ArgType::IMPLICIT_IMAGE_CHANNEL_ORDER,
        KernelArg::ArgType::IMPLICIT_IMAGE_SRGB_CHANNEL_ORDER,
        KernelArg::ArgType::IMPLICIT_IMAGE_ARRAY_SIZE,
        KernelArg::ArgType::IMPLICIT_IMAGE_NUM_SAMPLES,
        KernelArg::ArgType::IMPLICIT_SAMPLER_ADDRESS,
        KernelArg::ArgType::IMPLICIT_SAMPLER_NORMALIZED,
        KernelArg::ArgType::IMPLICIT_SAMPLER_SNAP_WA,
        KernelArg::ArgType::IMPLICIT_INLINE_SAMPLER,

        KernelArg::ArgType::IMPLICIT_VME_MB_BLOCK_TYPE,
        KernelArg::ArgType::IMPLICIT_VME_SUBPIXEL_MODE,
        KernelArg::ArgType::IMPLICIT_VME_SAD_ADJUST_MODE,
        KernelArg::ArgType::IMPLICIT_VME_SEARCH_PATH_TYPE,

        KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_DEFAULT_DEVICE_QUEUE,
        KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_EVENT_POOL,
        KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_MAX_WORKGROUP_SIZE,
        KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_PARENT_EVENT,
        KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_PREFERED_WORKGROUP_MULTIPLE,
        KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_DATA_PARAMETER_OBJECT_ID,
        KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_DISPATCHER_SIMD_SIZE,

        KernelArg::ArgType::IMPLICIT_LOCAL_MEMORY_STATELESS_WINDOW_START_ADDRESS,
        KernelArg::ArgType::IMPLICIT_LOCAL_MEMORY_STATELESS_WINDOW_SIZE,
        KernelArg::ArgType::IMPLICIT_PRIVATE_MEMORY_STATELESS_SIZE,

        KernelArg::ArgType::R1,
        KernelArg::ArgType::RT_STACK_ID,
        KernelArg::ArgType::IMPLICIT_LOCAL_ID_X,
        KernelArg::ArgType::IMPLICIT_LOCAL_ID_Y,
        KernelArg::ArgType::IMPLICIT_LOCAL_ID_Z,

        KernelArg::ArgType::IMPLICIT_BUFFER_SIZE,

        KernelArg::ArgType::IMPLICIT_ARG_BUFFER,
        KernelArg::ArgType::IMPLICIT_ASSERT_BUFFER,

        KernelArg::ArgType::STRUCT,
        KernelArg::ArgType::SAMPLER,
        KernelArg::ArgType::IMAGE_1D,
        KernelArg::ArgType::IMAGE_1D_BUFFER,
        KernelArg::ArgType::IMAGE_2D,
        KernelArg::ArgType::IMAGE_2D_DEPTH,
        KernelArg::ArgType::IMAGE_2D_MSAA,
        KernelArg::ArgType::IMAGE_2D_MSAA_DEPTH,
        KernelArg::ArgType::IMAGE_3D,
        KernelArg::ArgType::IMAGE_CUBE,
        KernelArg::ArgType::IMAGE_CUBE_DEPTH,
        KernelArg::ArgType::IMAGE_1D_ARRAY,
        KernelArg::ArgType::IMAGE_2D_ARRAY,
        KernelArg::ArgType::IMAGE_2D_DEPTH_ARRAY,
        KernelArg::ArgType::IMAGE_2D_MSAA_ARRAY,
        KernelArg::ArgType::IMAGE_2D_MSAA_DEPTH_ARRAY,
        KernelArg::ArgType::IMAGE_CUBE_ARRAY,
        KernelArg::ArgType::IMAGE_CUBE_DEPTH_ARRAY,

        KernelArg::ArgType::BINDLESS_SAMPLER,
        KernelArg::ArgType::BINDLESS_IMAGE_1D,
        KernelArg::ArgType::BINDLESS_IMAGE_1D_BUFFER,
        KernelArg::ArgType::BINDLESS_IMAGE_2D,
        KernelArg::ArgType::BINDLESS_IMAGE_2D_DEPTH,
        KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA,
        KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_DEPTH,
        KernelArg::ArgType::BINDLESS_IMAGE_3D,
        KernelArg::ArgType::BINDLESS_IMAGE_CUBE,
        KernelArg::ArgType::BINDLESS_IMAGE_CUBE_DEPTH,
        KernelArg::ArgType::BINDLESS_IMAGE_1D_ARRAY,
        KernelArg::ArgType::BINDLESS_IMAGE_2D_ARRAY,
        KernelArg::ArgType::BINDLESS_IMAGE_2D_DEPTH_ARRAY,
        KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_ARRAY,
        KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_DEPTH_ARRAY,
        KernelArg::ArgType::BINDLESS_IMAGE_CUBE_ARRAY,
        KernelArg::ArgType::BINDLESS_IMAGE_CUBE_DEPTH_ARRAY,
        SENTINEL,
    };

    if (VerifyOrder(CURBE, SENTINEL)) {
      TransposeGenerateOrder(CURBE);
    }

  } break;
  case InputType::INDIRECT: {
    std::array<KernelArg::ArgType, static_cast<int32_t>(KernelArg::ArgType::End)> INDIRECT = {
        KernelArg::ArgType::IMPLICIT_R0,

        KernelArg::ArgType::R1,
        KernelArg::ArgType::RT_STACK_ID,
        KernelArg::ArgType::IMPLICIT_LOCAL_ID_X,
        KernelArg::ArgType::IMPLICIT_LOCAL_ID_Y,
        KernelArg::ArgType::IMPLICIT_LOCAL_ID_Z,

        KernelArg::ArgType::RUNTIME_VALUE,
        KernelArg::ArgType::IMPLICIT_INDIRECT_DATA_POINTER,
        KernelArg::ArgType::IMPLICIT_SCRATCH_POINTER,
        KernelArg::ArgType::IMPLICIT_PAYLOAD_HEADER,
        KernelArg::ArgType::IMPLICIT_GLOBAL_OFFSET,
        KernelArg::ArgType::IMPLICIT_ENQUEUED_LOCAL_WORK_SIZE,
        KernelArg::ArgType::PTR_LOCAL,
        KernelArg::ArgType::PTR_GLOBAL,
        KernelArg::ArgType::PTR_CONSTANT,
        KernelArg::ArgType::PTR_DEVICE_QUEUE,
        KernelArg::ArgType::CONSTANT_REG,

        KernelArg::ArgType::IMPLICIT_CONSTANT_BASE,
        KernelArg::ArgType::IMPLICIT_GLOBAL_BASE,
        KernelArg::ArgType::IMPLICIT_PRIVATE_BASE,
        KernelArg::ArgType::IMPLICIT_PRINTF_BUFFER,
        KernelArg::ArgType::IMPLICIT_SYNC_BUFFER,
        KernelArg::ArgType::IMPLICIT_RT_GLOBAL_BUFFER,
        KernelArg::ArgType::IMPLICIT_BUFFER_OFFSET,
        KernelArg::ArgType::IMPLICIT_WORK_DIM,
        KernelArg::ArgType::IMPLICIT_NUM_GROUPS,
        KernelArg::ArgType::IMPLICIT_GLOBAL_SIZE,
        KernelArg::ArgType::IMPLICIT_LOCAL_SIZE,
        KernelArg::ArgType::IMPLICIT_STAGE_IN_GRID_ORIGIN,
        KernelArg::ArgType::IMPLICIT_STAGE_IN_GRID_SIZE,
        KernelArg::ArgType::IMPLICIT_REGION_GROUP_SIZE,
        KernelArg::ArgType::IMPLICIT_REGION_GROUP_WG_COUNT,
        KernelArg::ArgType::IMPLICIT_REGION_GROUP_BARRIER_BUFFER,
        KernelArg::ArgType::IMPLICIT_BINDLESS_OFFSET,

        KernelArg::ArgType::IMPLICIT_ARG_BUFFER,
        KernelArg::ArgType::IMPLICIT_ASSERT_BUFFER,

        KernelArg::ArgType::IMPLICIT_IMAGE_HEIGHT,
        KernelArg::ArgType::IMPLICIT_IMAGE_WIDTH,
        KernelArg::ArgType::IMPLICIT_IMAGE_DEPTH,
        KernelArg::ArgType::IMPLICIT_IMAGE_NUM_MIP_LEVELS,
        KernelArg::ArgType::IMPLICIT_IMAGE_CHANNEL_DATA_TYPE,
        KernelArg::ArgType::IMPLICIT_IMAGE_CHANNEL_ORDER,
        KernelArg::ArgType::IMPLICIT_IMAGE_SRGB_CHANNEL_ORDER,
        KernelArg::ArgType::IMPLICIT_IMAGE_ARRAY_SIZE,
        KernelArg::ArgType::IMPLICIT_IMAGE_NUM_SAMPLES,
        KernelArg::ArgType::IMPLICIT_SAMPLER_ADDRESS,
        KernelArg::ArgType::IMPLICIT_SAMPLER_NORMALIZED,
        KernelArg::ArgType::IMPLICIT_SAMPLER_SNAP_WA,
        KernelArg::ArgType::IMPLICIT_INLINE_SAMPLER,

        KernelArg::ArgType::IMPLICIT_VME_MB_BLOCK_TYPE,
        KernelArg::ArgType::IMPLICIT_VME_SUBPIXEL_MODE,
        KernelArg::ArgType::IMPLICIT_VME_SAD_ADJUST_MODE,
        KernelArg::ArgType::IMPLICIT_VME_SEARCH_PATH_TYPE,

        KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_DEFAULT_DEVICE_QUEUE,
        KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_EVENT_POOL,
        KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_MAX_WORKGROUP_SIZE,
        KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_PARENT_EVENT,
        KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_PREFERED_WORKGROUP_MULTIPLE,
        KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_DATA_PARAMETER_OBJECT_ID,
        KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_DISPATCHER_SIMD_SIZE,

        KernelArg::ArgType::IMPLICIT_LOCAL_MEMORY_STATELESS_WINDOW_START_ADDRESS,
        KernelArg::ArgType::IMPLICIT_LOCAL_MEMORY_STATELESS_WINDOW_SIZE,
        KernelArg::ArgType::IMPLICIT_PRIVATE_MEMORY_STATELESS_SIZE,

        KernelArg::ArgType::IMPLICIT_BUFFER_SIZE,

        KernelArg::ArgType::STRUCT,
        KernelArg::ArgType::SAMPLER,
        KernelArg::ArgType::IMAGE_1D,
        KernelArg::ArgType::IMAGE_1D_BUFFER,
        KernelArg::ArgType::IMAGE_2D,
        KernelArg::ArgType::IMAGE_2D_DEPTH,
        KernelArg::ArgType::IMAGE_2D_MSAA,
        KernelArg::ArgType::IMAGE_2D_MSAA_DEPTH,
        KernelArg::ArgType::IMAGE_3D,
        KernelArg::ArgType::IMAGE_CUBE,
        KernelArg::ArgType::IMAGE_CUBE_DEPTH,
        KernelArg::ArgType::IMAGE_1D_ARRAY,
        KernelArg::ArgType::IMAGE_2D_ARRAY,
        KernelArg::ArgType::IMAGE_2D_DEPTH_ARRAY,
        KernelArg::ArgType::IMAGE_2D_MSAA_ARRAY,
        KernelArg::ArgType::IMAGE_2D_MSAA_DEPTH_ARRAY,
        KernelArg::ArgType::IMAGE_CUBE_ARRAY,
        KernelArg::ArgType::IMAGE_CUBE_DEPTH_ARRAY,

        KernelArg::ArgType::BINDLESS_SAMPLER,
        KernelArg::ArgType::BINDLESS_IMAGE_1D,
        KernelArg::ArgType::BINDLESS_IMAGE_1D_BUFFER,
        KernelArg::ArgType::BINDLESS_IMAGE_2D,
        KernelArg::ArgType::BINDLESS_IMAGE_2D_DEPTH,
        KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA,
        KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_DEPTH,
        KernelArg::ArgType::BINDLESS_IMAGE_3D,
        KernelArg::ArgType::BINDLESS_IMAGE_CUBE,
        KernelArg::ArgType::BINDLESS_IMAGE_CUBE_DEPTH,
        KernelArg::ArgType::BINDLESS_IMAGE_1D_ARRAY,
        KernelArg::ArgType::BINDLESS_IMAGE_2D_ARRAY,
        KernelArg::ArgType::BINDLESS_IMAGE_2D_DEPTH_ARRAY,
        KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_ARRAY,
        KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_DEPTH_ARRAY,
        KernelArg::ArgType::BINDLESS_IMAGE_CUBE_ARRAY,
        KernelArg::ArgType::BINDLESS_IMAGE_CUBE_DEPTH_ARRAY,
        SENTINEL,
    };

    if (VerifyOrder(INDIRECT, SENTINEL)) {
      TransposeGenerateOrder(INDIRECT);
    }
  } break;
  default:
    IGC_ASSERT(0);
    break;
  }
}

bool KernelArgsOrder::operator()(const KernelArg::ArgType &lhs, const KernelArg::ArgType &rhs) const {
  return m_position[static_cast<int32_t>(lhs)] < m_position[static_cast<int32_t>(rhs)];
}

KernelArgs::const_iterator::const_iterator(AllocationArgs &args, IterPos pos) {
  m_empty = args.empty();
  if (pos == IterPos::BEGIN) {
    m_major = args.begin();
    m_majorEnd = args.end();
    if (!m_empty)
      m_minor = (*args.begin()).second.begin();
  } else if (pos == IterPos::END) {
    m_major = args.end();
    m_majorEnd = args.end();
    if (!m_empty)
      m_minor = (*(--args.end())).second.end();
  }
}

KernelArgs::const_iterator &KernelArgs::const_iterator::operator++() {
  IGC_ASSERT(!m_empty);
  ++m_minor;

  if (m_minor == (*m_major).second.end()) {
    ++m_major;
    if (m_major != m_majorEnd) {
      m_minor = (*m_major).second.begin();
    }
  }

  return *this;
}

const KernelArg &KernelArgs::const_iterator::operator*() {
  IGC_ASSERT(!m_empty);
  return *m_minor;
}

bool KernelArgs::const_iterator::operator!=(const const_iterator &iterator) const {
  if (m_empty)
    return (m_major != iterator.m_major);
  else
    return (m_major != iterator.m_major) || (m_minor != iterator.m_minor);
}

bool KernelArgs::const_iterator::operator==(const const_iterator &iterator) const {
  if (m_empty)
    return (m_major == iterator.m_major);
  else
    return (m_major == iterator.m_major) && (m_minor == iterator.m_minor);
}

KernelArgs::KernelArgs(const Function &F, const DataLayout *DL, MetaDataUtils *pMdUtils, ModuleMetaData *moduleMD,
                       unsigned int GRFSize, KernelArgsOrder::InputType layout)
    : m_KernelArgsOrder(layout), m_args(m_KernelArgsOrder) {
  ImplicitArgs implicitArgs(F, pMdUtils);
  const unsigned int numImplicitArgs = implicitArgs.size();
  const unsigned int numRuntimeValue = moduleMD ? moduleMD->pushInfo.constantReg.size() : 0;
  IGC_ASSERT_MESSAGE(F.arg_size() >= (numImplicitArgs + numRuntimeValue),
                     "Function arg size does not match meta data args.");
  const unsigned int numExplicitArgs = F.arg_size() - numImplicitArgs - numRuntimeValue;
  llvm::Function::const_arg_iterator funcArg = F.arg_begin();

  FunctionMetaData *funcMD = nullptr;
  if (moduleMD) {
    auto it = moduleMD->FuncMD.find(const_cast<Function *>(&F));
    if (it != moduleMD->FuncMD.end()) {
      funcMD = &it->second;
    }
  }

  FunctionInfoMetaDataHandle funcInfoMD = pMdUtils->getFunctionsInfoItem(const_cast<llvm::Function *>(&F));
  // Explicit function args
  for (unsigned int i = 0, e = numExplicitArgs; i < e; ++i, ++funcArg) {
    bool needAllocation = false;
    if (moduleMD && moduleMD->UseBindlessImage) {
      // Check for bindless images which require allocation
      funcMD = &moduleMD->FuncMD[const_cast<llvm::Function *>(&F)];
      ResourceAllocMD *resAllocMD = &funcMD->resAllocMD;
      if (resAllocMD->argAllocMDList.size() > funcArg->getArgNo()) {
        ArgAllocMD *argAlloc = &resAllocMD->argAllocMDList[funcArg->getArgNo()];
        if (argAlloc->type == ResourceTypeEnum::BindlessUAVResourceType ||
            argAlloc->type == ResourceTypeEnum::BindlessSamplerResourceType) {
          needAllocation = !funcArg->use_empty();
        }
      }
    }

    int location_index = -1;
    int location_count = -1;
    bool is_emulation_argument = false;

    if (funcMD) {
      if (funcMD->funcArgs.size() > (unsigned)i) {
        location_index = funcMD->funcArgs[i].bufferLocationIndex;
        location_count = funcMD->funcArgs[i].bufferLocationCount;
        is_emulation_argument = funcMD->funcArgs[i].isEmulationArg;
      }
    }

    std::string argBaseType = "";
    std::string argAccessQualItem = "";

    if (funcMD) {
      if (funcMD->m_OpenCLArgBaseTypes.size() > (unsigned)i)
        argBaseType = funcMD->m_OpenCLArgBaseTypes[i];
      if (funcMD->m_OpenCLArgAccessQualifiers.size() > (unsigned)i)
        argAccessQualItem = funcMD->m_OpenCLArgAccessQualifiers[i];
    }

    bool isScalarAsPointer = funcMD && funcMD->m_OpenCLArgScalarAsPointers.count(funcArg->getArgNo()) > 0;

    KernelArg kernelArg = KernelArg(&(*funcArg), DL, argBaseType, argAccessQualItem, location_index, location_count,
                                    needAllocation, is_emulation_argument, isScalarAsPointer);

    if ((kernelArg.getArgType() == KernelArg::ArgType::IMAGE_3D ||
         kernelArg.getArgType() == KernelArg::ArgType::BINDLESS_IMAGE_3D) &&
        funcInfoMD->isArgInfoListHasValue()) {
      for (auto AI = funcInfoMD->begin_ArgInfoList(), AE = funcInfoMD->end_ArgInfoList(); AI != AE; ++AI) {
        ArgInfoMetaDataHandle argInfo = *AI;
        if (argInfo->getExplicitArgNum() == i) {
          if (argInfo->isImgAccessFloatCoordsHasValue() && argInfo->isImgAccessIntCoordsHasValue()) {
            kernelArg.setImgAccessedFloatCoords(argInfo->getImgAccessFloatCoords());
            kernelArg.setImgAccessedIntCoords(argInfo->getImgAccessIntCoords());
            break;
          }
        }
      }
    }

    addAllocationArg(kernelArg);
  }

  // Implicit function args
  for (unsigned int i = 0; i < numImplicitArgs; ++i, ++funcArg) {
    bool isScalarAsPointer = funcMD && funcMD->m_OpenCLArgScalarAsPointers.count(funcArg->getArgNo()) > 0;

    KernelArg kernelArg = KernelArg(implicitArgs[i], DL, &(*funcArg), implicitArgs.getExplicitArgNum(i),
                                    implicitArgs.getStructArgOffset(i), isScalarAsPointer, GRFSize);
    addAllocationArg(kernelArg);
  }

  // Need to add Runtime Values, so they can trigger NOSBuffer allocation in correct
  // order (especially needed when InputType::INDEPENDENT or InputType::CURBE is used).
  for (unsigned int i = 0; i < numRuntimeValue; ++i, ++funcArg) {
    KernelArg kernelArg = KernelArg(KernelArg::ArgType::RUNTIME_VALUE,      // argType
                                    KernelArg::AccessQual::NONE,            // accessQual
                                    4,                                      // allocateSize
                                    4,                                      // elemAllocateSize
                                    4,                                      // align
                                    true,                                   // isConstantBuf
                                    &(*funcArg),                            // arg
                                    numExplicitArgs + numImplicitArgs + 1); // associatedArgNo
    addAllocationArg(kernelArg);
  }
}

void KernelArgs::addAllocationArg(KernelArg &kernelArg) {
  KernelArg::ArgType argType = kernelArg.getArgType();

  // Add to the allocation arguments of this type
  m_args[argType].push_back(kernelArg);
}

KernelArgs::const_iterator KernelArgs::begin() {
  return const_iterator(m_args, KernelArgs::const_iterator::IterPos::BEGIN);
}

KernelArgs::const_iterator KernelArgs::end() {
  return const_iterator(m_args, KernelArgs::const_iterator::IterPos::END);
}

void KernelArgs::checkForZeroPerThreadData() {

  // On SKL, when we use Indirect thread payload, Spec says:
  // if Cross-Thread Constant Data Read Length for Indirect is greater than 0,
  // then Per thread data field must also be greater than 0.
  // In that case we allocate one blank payload grf for Per thread constant.

  // if PTD == 0 && CTCD > 0 then we would need to allocate a dummy argument to occupy a single GRF in a PTD
  // PTD 1 && CTCD > 0 is perfectly OK
  int PerThreadData = 0;
  bool HWWAForZeroLengthPTDRequired = true;
  for (AllocationArgs::const_iterator i = m_args.begin(), e = m_args.end(); i != e; ++i) {
    const KernelArg *arg = i->second.data();
    if (arg->needsAllocation() && !arg->isConstantBuf()) {
      if (++PerThreadData > 0 + 1 /* IMPLICIT_R0 */) {
        HWWAForZeroLengthPTDRequired = false;
        break;
      }
    }
  }
  if (HWWAForZeroLengthPTDRequired) {
    KernelArg kernelArg = KernelArg(KernelArg::ArgType::R1, KernelArg::AccessQual::NONE, 32, 4, 32, false, nullptr, 0);
    addAllocationArg(kernelArg);
  }
}

bool KernelArgs::empty() { return m_args.empty() ? true : begin() == end(); }
