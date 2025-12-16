/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Constants.h>
#include "common/LLVMWarningsPop.hpp"
#include "common/igc_resourceDimTypes.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "common/MDFrameWork.h"

namespace IGC {
typedef union _gfxResourceAddrSpace {
  static_assert(BufferType::BUFFER_TYPE_UNKNOWN < 32, "Please also adjust bufType bit size");
  struct _bits {
    unsigned int bufId : 13;
    unsigned int resDimType : 3; // resource dimension type
    unsigned int bufType : 5;
    unsigned int indirect : 1;            // bool
    unsigned int nonDefaultCacheCtrl : 1; // bool
    unsigned int isSurfaceRes : 1;        // bool
    // llvm subclass data is 24 bit, using the field below will trigger assertion in LLVM/IR/Type.h.
    unsigned int _padding_do_not_use : 8;
  } bits;
  uint32_t u32Val;
} GFXResourceAddrSpace;

// If 'bufIdx' is a ConstantInt, 'uniqueIndAS' is irrelevant.
// Otherwise, you should set 'uniqueIndAS' if you want to identify
// this address space later on.  If not, the default can be used.
// resourceDimTypeId represents RESOURCE_DIMENSION_TYPE
inline unsigned EncodeAS4GFXResource(const llvm::Value &bufIdx, BufferType bufType,
                                     unsigned uniqueIndAS = IGC::DefaultIndirectIdx, bool isNonDefaultCacheCtrl = false,
                                     RESOURCE_DIMENSION_TYPE resourceDimTypeId = NUM_RESOURCE_DIMENSION_TYPES) {
  GFXResourceAddrSpace temp;
  static_assert(sizeof(temp) == 4, "Code below may need and update.");
  temp.u32Val = 0;
  IGC_ASSERT((bufType + 1) < BUFFER_TYPE_UNKNOWN + 1);
  temp.bits.bufType = bufType + 1;

  temp.bits.isSurfaceRes = (resourceDimTypeId != NUM_RESOURCE_DIMENSION_TYPES);
  if (temp.bits.isSurfaceRes) {
    temp.bits.resDimType = resourceDimTypeId;
  }

  if (bufType == SLM) {
    return ADDRESS_SPACE_LOCAL;
  } else if (bufType == STATELESS_READONLY) {
    return ADDRESS_SPACE_CONSTANT;
  } else if (bufType == STATELESS) {
    return ADDRESS_SPACE_GLOBAL;
  } else if (bufType == STATELESS_A32) {
    return ADDRESS_SPACE_THREAD_ARG;
  } else if (auto *CI = llvm::dyn_cast<llvm::ConstantInt>(&bufIdx)) {
    unsigned int bufId = static_cast<unsigned>(CI->getZExtValue());
    IGC_ASSERT((bufType == BINDLESS_SAMPLER) || (bufId < (1 << 13)));
    temp.bits.bufId = bufId;
    temp.bits.nonDefaultCacheCtrl = isNonDefaultCacheCtrl ? 1 : 0;
    return temp.u32Val;
  }

  // if it is indirect-buf, it is front-end's job to give a proper(unique) address-space per access
  temp.bits.bufId = uniqueIndAS;
  temp.bits.nonDefaultCacheCtrl = isNonDefaultCacheCtrl ? 1 : 0;
  temp.bits.indirect = 1;
  return temp.u32Val;
}

inline BufferType DecodeAS4GFXResource(unsigned addrSpace, bool &directIndexing, unsigned &bufId) {
  GFXResourceAddrSpace temp;
  temp.u32Val = addrSpace;

  directIndexing = (temp.bits.indirect == 0);
  bufId = temp.bits.bufId;

  if (addrSpace == ADDRESS_SPACE_LOCAL) {
    return SLM;
  } else if (addrSpace == ADDRESS_SPACE_THREAD_ARG) {
    return STATELESS_A32;
  }
  unsigned bufType = temp.bits.bufType - 1;
  if (bufType < BUFFER_TYPE_UNKNOWN) {
    return (BufferType)bufType;
  }

  return BUFFER_TYPE_UNKNOWN;
}
///
/// returns buffer type from addressspace
///
inline RESOURCE_DIMENSION_TYPE DecodeAS4GFXResourceType(unsigned addrSpace) {
  GFXResourceAddrSpace temp;
  temp.u32Val = addrSpace;
  RESOURCE_DIMENSION_TYPE resourceDimTypeId = NUM_RESOURCE_DIMENSION_TYPES;
  if (temp.bits.isSurfaceRes) {
    resourceDimTypeId = static_cast<RESOURCE_DIMENSION_TYPE>(temp.bits.resDimType);
    IGC_ASSERT(resourceDimTypeId < NUM_RESOURCE_DIMENSION_TYPES);
  }
  return resourceDimTypeId;
}
///
/// returns buffer type from addressspace
///
inline BufferType DecodeBufferType(unsigned addrSpace) {
  switch (addrSpace) {
  case ADDRESS_SPACE_CONSTANT:
    return STATELESS_READONLY;
  case ADDRESS_SPACE_LOCAL:
    return SLM;
  case ADDRESS_SPACE_GLOBAL:
    return STATELESS;
  case ADDRESS_SPACE_THREAD_ARG:
    return STATELESS_A32;
  default:
    break;
  }
  GFXResourceAddrSpace temp;
  temp.u32Val = addrSpace;
  BufferType type = BUFFER_TYPE_UNKNOWN;
  if (addrSpace > ADDRESS_SPACE_NUM_ADDRESSES && (temp.bits.bufType - 1) < BUFFER_TYPE_UNKNOWN) {
    type = static_cast<BufferType>(temp.bits.bufType - 1);
  }
  return type;
}

inline unsigned SetBufferAsBindless(unsigned addressSpaceOfPtr, BufferType bufferType) {
  GFXResourceAddrSpace temp = {};
  temp.u32Val = addressSpaceOfPtr;

  // Mark buffer as it is bindless for further processing
  switch (bufferType) {
  case BufferType::RESOURCE:
    temp.bits.bufType = IGC::BINDLESS_TEXTURE + 1;
    break;
  case BufferType::CONSTANT_BUFFER:
    temp.bits.bufType = IGC::BINDLESS_CONSTANT_BUFFER + 1;
    break;
  case BufferType::UAV:
    temp.bits.bufType = IGC::BINDLESS + 1;
    break;
  case BufferType::SAMPLER:
    temp.bits.bufType = IGC::BINDLESS_SAMPLER + 1;
    break;
  default:
    IGC_ASSERT_MESSAGE(0, "other types of buffers shouldn't reach this part");
    break;
  }
  return temp.u32Val;
}

///
/// returns info if direct addressing is used
///
inline bool IsDirectIdx(unsigned addrSpace) {
  GFXResourceAddrSpace temp;
  temp.u32Val = addrSpace;
  return (temp.bits.indirect == 0);
}

inline BufferType GetBufferType(unsigned addrSpace) {
  bool directIndexing = false;
  unsigned int bufId = 0;
  return DecodeAS4GFXResource(addrSpace, directIndexing, bufId);
}

// Return true if AS is for a stateful surface.
//    Stateful surface should have an encoded AS that is bigger than
//    ADDRESS_SPACE_NUM_ADDRESSES.
inline bool isStatefulAddrSpace(unsigned AS) { return AS > ADDRESS_SPACE_NUM_ADDRESSES; }

} // namespace IGC
