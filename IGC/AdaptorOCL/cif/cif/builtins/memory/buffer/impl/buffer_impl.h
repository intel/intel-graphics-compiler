/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cif/builtins/memory/buffer/buffer.h"

#include "cif/export/pimpl_base.h"

#include "cif/macros/enable.h"

#include "cif/helpers/memory.h"

namespace CIF {

namespace Builtins {

CIF_DECLARE_INTERFACE_PIMPL(Buffer) : CIF::PimplBase {
    CIF_PIMPL_DECLARE_CONSTRUCTOR(Version_t version, CIF::ICIF *parentInterface) {
    }

    CIF_PIMPL_DECLARE_CONSTRUCTOR(Version_t version) {
    }

    CIF_PIMPL_DECLARE_CONSTRUCTOR() {
    }

    CIF_PIMPL_DECLARE_DESTRUCTOR(){
        Deallocate();
    }

  /// Copies given element to the end of the buffer
  /// Note : If (packed == false), then this function will automatically align current underlying buffer
  ///        pointer to alignof(ElementT)
  template<typename ElementT>
  bool PushBackRawCopy(const ElementT &newEl, bool packed = true){
      static_assert(std::is_standard_layout_v<ElementT> && std::is_trivial_v<ElementT>, "Supporting only POD types");
      if(packed == false){
          size_t alignment = alignof(ElementT);
          bool success = AlignUp(alignment);
          if(success == false){
              return false;
          }
      }

      return PushBackRawBytes(&newEl, sizeof(ElementT));
  }

    /// Sets custom allocator and deallocator functions to be used by this buffer interface
    /// Note : will reallocate using new allocators only when additional space will be needed
    /// Note : reallocator is optional and can be nullptr
    void SetAllocator(AllocatorT allocator, DeallocatorT deallocator, ReallocatorT reallocator){
        if(allocator == nullptr){
            allocator = EmptyAllocator;
        }

        if((this->memory) && (this->tempDeallocator == nullptr)){
            this->tempDeallocator = this->deallocator;
        }

        this->allocator = allocator;
        this->deallocator = deallocator;
        this->reallocator = reallocator;
    }

    /// Sets underlying buffer storage and its deallocator
    /// Note : will destroy current underlying buffer (if any)
    void SetUnderlyingStorage(void *memory, size_t size, DeallocatorT deallocator){
        Deallocate();

        this->memory = memory;
        this->size = size;
        this->capacity = size;
        this->tempDeallocator = deallocator;
    }

    /// Sets underlying constant buffer storage and its deallocator
    /// Note : will destroy current underlying buffer (if any)
    /// Note : will allocate new memory if this const memory will be accessed in non-const manners
    void SetUnderlyingStorage(const void *memory, size_t size){
        Deallocate();

        // casting-away constness, but marking as const
        this->memory = const_cast<void*>(memory);
        this->isConst = true;

        this->size = size;
        this->capacity = size;
        this->tempDeallocator = nullptr;
    }

    /// Detaches and returns current allocation leaving this buffer's underlying buffer empty
    void *DetachAllocation(){
        void *mem = memory;
        this->memory = nullptr;
        this->size = 0;
        this->capacity = 0;
        this->isConst = false;
        this->tempDeallocator = nullptr;
        return mem;
    }

    /// Returns const raw access to underlying buffer
    const void *GetMemoryRaw() const{
        return memory;
    }

    /// Returns writeable access to underlyng buffer
    /// Note : for constant buffer this will cause reallocation (copy!) to writeable memory
    void *GetMemoryRawWriteable(){
        if(this->isConst && this->capacity != 0){
            if(false == Reallocate(this->size, this->capacity)){
                return nullptr;
            }
        }
        return memory;
    }

    /// Returns size in bytes of underlying buffer
    size_t GetSizeRaw() const{
        return size;
    }

    /// Returns capacity in bytes of underlying buffer
    size_t GetCapacityRaw() const{
        return capacity;
    }

    /// Resizes (may reallocate) underlying buffer's size
    bool Resize(size_t newSize) {
        return this->Reallocate(newSize, std::max(this->capacity, newSize));
    }

    /// Resizes (may reallocate) the underlying buffer's capacity
    bool Reserve(size_t newCapacity){
        return this->Reallocate(this->size, newCapacity);
    }

    /// Sets the size to 0
    void Clear(){
        this->size = 0;
    }

    /// Sets the size to 0 and deallocates underlying buffer
    void Deallocate(){
        if(isConst){
            this->memory = nullptr;
            this->size = 0;
            this->capacity = 0;
            this->isConst = false;
            this->tempDeallocator = nullptr;
        }else if(tempDeallocator){
            tempDeallocator(this->memory);
            this->memory = nullptr;
            this->size = 0;
            this->capacity = 0;
            this->tempDeallocator = nullptr;
        }else{
            if(deallocator != nullptr){
                deallocator(this->memory);
            }
            this->memory = nullptr;
            this->size = 0;
            this->capacity = 0;
        }
    }

    /// Aligns up current size to meet required alignment
    bool AlignUp(uint32_t alignment){
        size_t misalligned = alignment - (this->size % alignment);
        size_t newSize = this->size + misalligned;
        return Reallocate(newSize, newSize);
    }

    /// Pushes new raw element to buffer
    bool PushBackRawBytes(const void *newData, size_t size){
        if(size == 0){
            // nothing to do
            return true;
        }

        size_t oldSize = this->size;
        if(false == Reallocate(this->size + size, this->size + size)){
            return false;
        }

        CIF::SafeCopy(CIF::OffsetedPtr(GetMemoryRawWriteable(), oldSize), this->capacity - oldSize, newData, size);
        return true;
    }

    /// Can be true if SetUnderlyingStorage(const void *, size_t) was used, please refer to SetUnderlyingStorage
    bool IsConst() const{
        return isConst;
    }

protected:
    static void * CIF_CALLING_CONV EmptyAllocator(size_t size){
        return nullptr;
    }

    static void * CIF_CALLING_CONV DefaultAllocator(size_t size){
        return new(std::nothrow) char[size];
    }

    static void CIF_CALLING_CONV DefaultDeallocator(void * memory){
        if(memory != nullptr){
            delete []reinterpret_cast<char*>(memory);
        }
    }

    bool Reallocate(size_t newSize, size_t newCapacity){
        if(isConst){
            void * newMem = this->allocator(newCapacity);
            if(newMem == nullptr){
                return false;
            }

            size_t bytesToCopy = std::min(newSize, this->size);
            CIF::SafeCopy(newMem, newCapacity, this->memory, bytesToCopy);
            this->memory = newMem;
            this->isConst = false;
            this->tempDeallocator = nullptr;
            this->capacity = newCapacity;
            this->size = newSize;
            return true;
        }

        if(tempDeallocator != nullptr){
            if(newCapacity <= this->capacity){
                this->size = newSize;
                return true;
            }

            void * newMem = this->allocator(newCapacity);
            if(newMem == nullptr){
                return false;
            }
            size_t bytesToCopy = std::min(newSize, this->size);
            CIF::SafeCopy(newMem, newCapacity, this->memory, bytesToCopy);
            void *oldMemory = this->memory;
            this->memory = newMem;
            this->capacity = newCapacity;
            this->size = newSize;
            this->tempDeallocator(oldMemory);
            this->tempDeallocator = nullptr;
            return true;
        }

        if(newCapacity <= this->capacity){
            this->size = newSize;
            return true;
        }

        if(reallocator != nullptr){
            void *newMem = reallocator(this->memory, this->capacity, newCapacity);
            if(newMem == nullptr){
                return false;
            }

            this->memory = newMem;
            this->capacity = newCapacity;
            this->size = newSize;
            return true;
        }

        void *newMem = allocator(newCapacity);
        if(newMem == nullptr){
            return false;
        }

        size_t bytesToCopy = std::min(newSize, this->size);
        CIF::SafeCopy(newMem, newCapacity, this->memory, bytesToCopy);
        if(deallocator != nullptr){
            deallocator(this->memory);
        }
        this->memory = newMem;
        this->capacity = newCapacity;
        this->size = newSize;

        return true;
    }

    void *memory = nullptr;
    size_t size = 0;
    size_t capacity = 0;
    bool isConst = false;
    DeallocatorT tempDeallocator = nullptr;
    AllocatorT allocator = DefaultAllocator;
    DeallocatorT deallocator = DefaultDeallocator;
    ReallocatorT reallocator = nullptr;
};

CIF_DEFINE_INTERFACE_TO_PIMPL_FORWARDING_CTOR_DTOR(Buffer);

}

}

#include "cif/macros/disable.h"
