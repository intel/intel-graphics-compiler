/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "cif/builtins/memory/buffer/buffer.h"
#include "cif/builtins/memory/buffer/impl/buffer_impl.h"

#include "cif/macros/enable.h"

namespace CIF{

namespace Builtins{

void CIF_GET_INTERFACE_CLASS(Buffer, 1)::SetAllocator(AllocatorT allocator, DeallocatorT deallocator, ReallocatorT reallocator){
    CIF_GET_PIMPL()->SetAllocator(allocator, deallocator, reallocator);
}

void CIF_GET_INTERFACE_CLASS(Buffer, 1)::SetUnderlyingStorage(void *memory, size_t size, DeallocatorT deallocator){
    CIF_GET_PIMPL()->SetUnderlyingStorage(memory, size, deallocator);
}

void CIF_GET_INTERFACE_CLASS(Buffer, 1)::SetUnderlyingStorage(const void *memory, size_t size){
    CIF_GET_PIMPL()->SetUnderlyingStorage(memory, size);
}

void *CIF_GET_INTERFACE_CLASS(Buffer, 1)::DetachAllocation(){
    return CIF_GET_PIMPL()->DetachAllocation();
}

const void *CIF_GET_INTERFACE_CLASS(Buffer, 1)::GetMemoryRaw() const{
    return CIF_GET_PIMPL()->GetMemoryRaw();
}

void *CIF_GET_INTERFACE_CLASS(Buffer, 1)::GetMemoryRawWriteable(){
    return CIF_GET_PIMPL()->GetMemoryRawWriteable();
}

size_t CIF_GET_INTERFACE_CLASS(Buffer, 1)::GetSizeRaw() const{
    return CIF_GET_PIMPL()->GetSizeRaw();
}

size_t CIF_GET_INTERFACE_CLASS(Buffer, 1)::GetCapacityRaw() const{
    return CIF_GET_PIMPL()->GetCapacityRaw();
}

bool CIF_GET_INTERFACE_CLASS(Buffer, 1)::Resize(size_t newSize){
    return CIF_GET_PIMPL()->Resize(newSize);
}

bool CIF_GET_INTERFACE_CLASS(Buffer, 1)::Reserve(size_t newCapacity){
    return CIF_GET_PIMPL()->Reserve(newCapacity);
}

void CIF_GET_INTERFACE_CLASS(Buffer, 1)::Clear(){
    CIF_GET_PIMPL()->Clear();
}

void CIF_GET_INTERFACE_CLASS(Buffer, 1)::Deallocate(){
    CIF_GET_PIMPL()->Deallocate();
}

bool CIF_GET_INTERFACE_CLASS(Buffer, 1)::AlignUp(uint32_t alignment){
    return CIF_GET_PIMPL()->AlignUp(alignment);
}

bool CIF_GET_INTERFACE_CLASS(Buffer, 1)::PushBackRawBytes(const void *newData, size_t size){
    return CIF_GET_PIMPL()->PushBackRawBytes(newData, size);
}

bool CIF_GET_INTERFACE_CLASS(Buffer, 1)::IsConst()const{
    return CIF_GET_PIMPL()->IsConst();
}

}

}

#include "cif/macros/disable.h"
