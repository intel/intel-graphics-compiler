/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "PacketBuilder.h"
#include <cstdarg>
#include "Probe/Assertion.h"

namespace pktz
{
    void PacketBuilder::AssertMemoryUsageParams(Value* ptr, JIT_MEM_CLIENT usage)
    {
        IGC_ASSERT_MESSAGE(ptr->getType() != mInt64Ty,
            "Address appears to be GFX access.  Requires translation through BuilderGfxMem.");
    }

    Value* PacketBuilder::GEP(Value* Ptr, Value* Idx, Type* Ty, const Twine& Name)
    {
        return IRB()->CreateGEP(Ptr, Idx, Name);
    }

    Value* PacketBuilder::GEP(Type* Ty, Value* Ptr, Value* Idx, const Twine& Name)
    {
        return IRB()->CreateGEP(Ty, Ptr, Idx, Name);
    }

    Value* PacketBuilder::GEP(Value* ptr, const std::initializer_list<Value*>& indexList, Type* Ty)
    {
        std::vector<Value*> indices;
        for (auto i : indexList)
            indices.push_back(i);
        return GEPA(ptr, indices);
    }

    Value* PacketBuilder::GEP(Value* ptr, const std::initializer_list<uint32_t>& indexList, Type* Ty)
    {
        std::vector<Value*> indices;
        for (auto i : indexList)
            indices.push_back(C(i));
        return GEPA(ptr, indices);
    }

    Value* PacketBuilder::GEPA(Value* Ptr, ArrayRef<Value*> IdxList, const Twine& Name)
    {
        return IRB()->CreateGEP(Ptr, IdxList, Name);
    }

    Value* PacketBuilder::GEPA(Type* Ty, Value* Ptr, ArrayRef<Value*> IdxList, const Twine& Name)
    {
        return IRB()->CreateGEP(Ty, Ptr, IdxList, Name);
    }

    Value* PacketBuilder::IN_BOUNDS_GEP(Value* ptr, const std::initializer_list<Value*>& indexList)
    {
        std::vector<Value*> indices;
        for (auto i : indexList)
            indices.push_back(i);
        return IN_BOUNDS_GEP(ptr, indices);
    }

    Value* PacketBuilder::IN_BOUNDS_GEP(Value* ptr, const std::initializer_list<uint32_t>& indexList)
    {
        std::vector<Value*> indices;
        for (auto i : indexList)
            indices.push_back(C(i));
        return IN_BOUNDS_GEP(ptr, indices);
    }

    LoadInst* PacketBuilder::LOAD(Value* Ptr, const char* Name, Type* Ty, JIT_MEM_CLIENT usage)
    {
        AssertMemoryUsageParams(Ptr, usage);
        return IRB()->CreateLoad(Ptr, Name);
    }

    LoadInst* PacketBuilder::LOAD(Value* Ptr, const Twine& Name, Type* Ty, JIT_MEM_CLIENT usage)
    {
        AssertMemoryUsageParams(Ptr, usage);
        return IRB()->CreateLoad(Ptr, Name);
    }

    LoadInst* PacketBuilder::LOAD(Type* Ty, Value* Ptr, const Twine& Name, JIT_MEM_CLIENT usage)
    {
        AssertMemoryUsageParams(Ptr, usage);
        return IRB()->CreateLoad(Ty, Ptr, Name);
    }

    LoadInst*
    PacketBuilder::LOAD(Value* Ptr, bool isVolatile, const Twine& Name, Type* Ty, JIT_MEM_CLIENT usage)
    {
        AssertMemoryUsageParams(Ptr, usage);
        return IRB()->CreateLoad(Ptr, isVolatile, Name);
    }

    LoadInst* PacketBuilder::LOAD(Value*                                 basePtr,
                            const std::initializer_list<uint32_t>& indices,
                            const llvm::Twine&                     name,
                            Type*                                  Ty,
                            JIT_MEM_CLIENT                         usage)
    {
        std::vector<Value*> valIndices;
        for (auto i : indices)
            valIndices.push_back(C(i));
        return PacketBuilder::LOAD(GEPA(basePtr, valIndices), name);
    }

    LoadInst* PacketBuilder::LOADV(Value*                               basePtr,
                             const std::initializer_list<Value*>& indices,
                             const llvm::Twine&                   name)
    {
        std::vector<Value*> valIndices;
        for (auto i : indices)
            valIndices.push_back(i);
        return LOAD(GEPA(basePtr, valIndices), name);
    }

    StoreInst*
    PacketBuilder::STORE(Value* val, Value* basePtr, const std::initializer_list<uint32_t>& indices)
    {
        std::vector<Value*> valIndices;
        for (auto i : indices)
            valIndices.push_back(C(i));
        return STORE(val, GEPA(basePtr, valIndices));
    }

    StoreInst*
    PacketBuilder::STOREV(Value* val, Value* basePtr, const std::initializer_list<Value*>& indices)
    {
        std::vector<Value*> valIndices;
        for (auto i : indices)
            valIndices.push_back(i);
        return STORE(val, GEPA(basePtr, valIndices));
    }

    Value* PacketBuilder::OFFSET_TO_NEXT_COMPONENT(Value* base, Constant* offset)
    {
        return GEP(base, offset);
    }

    Value* PacketBuilder::MEM_ADD(Value*                                 i32Incr,
                            Value*                                 basePtr,
                            const std::initializer_list<uint32_t>& indices,
                            const llvm::Twine&                     name)
    {
        Value* i32Value  = LOAD(GEP(basePtr, indices), name);
        Value* i32Result = ADD(i32Value, i32Incr);
        return STORE(i32Result, GEP(basePtr, indices));
    }

}
