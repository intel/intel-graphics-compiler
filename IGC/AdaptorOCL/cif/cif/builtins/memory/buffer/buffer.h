/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <type_traits>

#include "cif/common/id.h"
#include "cif/common/cif.h"
#include "cif/common/cif_main.h"

#include "cif/macros/enable.h"

namespace CIF {

namespace Builtins {
/// Custom allocator function
using AllocatorT = void *(CIF_CALLING_CONV *)(size_t size);

/// Custom allocator function
using ReallocatorT = void *(CIF_CALLING_CONV *)(void *oldMemory, size_t oldSize, size_t newSize);

/// Custom deallocator function
using DeallocatorT = void (CIF_CALLING_CONV *)(void *memory);

/// Builtin interface that can be used as generic buffer
CIF_DECLARE_COMMON_INTERFACE(Buffer, "CIF_BUFFER");

/// v #1
CIF_DEFINE_INTERFACE_VER(Buffer, 1){
  CIF_INHERIT_CONSTRUCTOR();

  /// Get address to underlying buffer with ElementT as pointer element type
  template<typename ElementT>
  ElementT *GetMemoryWriteable(){
      return reinterpret_cast<ElementT*>(GetMemoryRawWriteable());
  }

  /// Get address to underlying const buffer with ElementT as pointer element type
  template<typename ElementT>
  const ElementT *GetMemory() const{
      return reinterpret_cast<const ElementT*>(GetMemoryRaw());
  }

  /// Get size in units of ElementT (ElementT == void version)
  template<typename ElementT>
  const typename std::enable_if<std::is_void<ElementT>::value, size_t>::type GetSize() const{
      return GetSizeRaw();
  }

  /// Get size in units of ElementT (ElementT != void version)
  template<typename ElementT>
  const typename std::enable_if<false == std::is_void<ElementT>::value, size_t>::type GetSize() const{
      return GetSizeRaw() / sizeof(ElementT);
  }

  /// Copies given element to the end of the buffer
  /// Note : If (packed == false), then this function will automatically align current underlying buffer
  ///        pointer to alignof(ElementT)
  template<typename ElementT>
  bool PushBackRawCopy(const ElementT &newEl, bool packed = true){
      static_assert(std::is_pod<ElementT>::value, "Supporting only POD types");
      if(packed == false){
          size_t alignment = alignof(ElementT);
          bool success = AlignUp(static_cast<uint32_t>(alignment));
          if(success == false){
              return false;
          }
      }

      return PushBackRawBytes(&newEl, sizeof(ElementT));
  }

  /// Sets custom allocator and deallocator functions to be used by this buffer interface
  /// Note : will reallocate using new allocators only when additional space will be needed
  /// Note : reallocator is optional and can be nullptr
  virtual void SetAllocator(AllocatorT allocator, DeallocatorT deallocator, ReallocatorT reallocator);

  /// Sets underlying buffer storage and its deallocator
  /// Note : will destroy current underlying buffer (if any)
  /// Note : if size of given memory becomes too small, will fallback to allocating new memory
  virtual void SetUnderlyingStorage(void *memory, size_t size, DeallocatorT deallocator);

  /// Sets underlying constant buffer storage and its deallocator
  /// Note : will destroy current underlying buffer (if any)
  /// Note : will allocate new memory if this const memory will be accessed in non-const manners
  /// Note : since given pointer is treated as const, memory pointed ty by it will never be freed
  ///        by this buffer instance
  virtual void SetUnderlyingStorage(const void *memory, size_t size);

  /// Detaches and returns current allocation leaving this buffer's underlying buffer empty
  virtual void *DetachAllocation();

  /// Returns const raw access to underlying buffer
  virtual const void *GetMemoryRaw() const;

  /// Returns writeable access to underlyng buffer
  /// Note : for constant buffer this will cause reallocation (copy!) to writeable memory
  virtual void *GetMemoryRawWriteable();

  /// Returns size in bytes of underlying buffer
  virtual size_t GetSizeRaw() const;

  /// Returns capacity in bytes of underlying buffer
  virtual size_t GetCapacityRaw() const;

  /// Resizes (may reallocate) underlying buffer's size
  virtual bool Resize(size_t newSize);

  /// Resizes (may reallocate) the underlying buffer's capacity
  virtual bool Reserve(size_t newCapacity);

  /// Sets the size to 0
  virtual void Clear();

  /// Sets the size to 0 and deallocates underlying buffer
  virtual void Deallocate();

  /// Aligns up current size to meet required alignment
  virtual bool AlignUp(uint32_t alignment);

  /// Pushes new raw element to buffer
  virtual bool PushBackRawBytes(const void *newData, size_t size);

  /// Can be true if SetUnderlyingStorage(const void *, size_t) was used, please refer to SetUnderlyingStorage
  virtual bool IsConst() const;
};

CIF_GENERATE_VERSIONS_LIST(Buffer);
CIF_MARK_LATEST_VERSION(BufferLatest, Buffer);
using BufferSimple = Buffer<1>; /// tag the most common version

template<typename BufferInterface = BufferLatest>
CIF::RAII::UPtr_t<BufferInterface> CreateConstBuffer(CIF::CIFMain *provider, const void *data, size_t size){
    if(provider == nullptr){
        return CIF::RAII::UPtr<BufferInterface>(nullptr);
    }
    auto buff = provider->CreateBuiltin<BufferInterface>();
    if(buff == nullptr){
        return CIF::RAII::UPtr<BufferInterface>(nullptr);
    }
    if((data != nullptr) && (size != 0)){
        buff->SetUnderlyingStorage(data, size);
    }
    return buff;
}

template<typename BufferInterface = BufferLatest>
CIF::RAII::UPtr_t<BufferInterface> CreateWriteableBuffer(CIF::CIFMain *provider, const void *initialData, size_t initialSize){
    auto buff = CreateConstBuffer<BufferInterface>(provider, initialData, initialSize);
    if(buff == nullptr){
        return CIF::RAII::UPtr<BufferInterface>(nullptr);
    }
    auto writeableMem = buff->GetMemoryRawWriteable();
    if(writeableMem == nullptr && (initialData != nullptr && initialSize != 0)){
        // failed to allocate new memory in writeable memory
        return CIF::RAII::UPtr<BufferInterface>(nullptr);
    }
    return buff;
}

template<typename BufferInterface = BufferLatest>
CIF::RAII::UPtr_t<BufferInterface> CreateBufferFromPtr(CIF::CIFMain *provider, void *ptr, size_t size, DeallocatorT ptrDeallocator){
    auto buff = CreateConstBuffer<BufferInterface>(provider, nullptr, 0);
    if(buff == nullptr){
        return CIF::RAII::UPtr<BufferInterface>(nullptr);
    }

    buff->SetUnderlyingStorage(ptr, size, ptrDeallocator);

    return buff;
}

}

}

#include "cif/macros/disable.h"
