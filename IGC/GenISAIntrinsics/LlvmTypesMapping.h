/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <type_traits>
#include <stdint.h>

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/ArrayRef.h"
#include "common/LLVMWarningsPop.hpp"

namespace llvm
{
class Type;
class LLVMContext;
class TypeDescription;
} // namespace llvm

namespace IGC
{

enum class TypeID : uint32_t
{
    Void,
    Integer,
    Float,
    Vector,
    Struct,
    Pointer,
    Any,
    ArgumentReference,
    Count
};

template<TypeID id>
struct TypeDescriptionTraits;

struct TypeDescription;

constexpr bool IsOverloadable(const TypeDescription& type);

template<>
struct TypeDescriptionTraits<TypeID::Any>
{
    static constexpr TypeID scTypeID = TypeID::Any;
    static constexpr bool scIsOverloadable = true;
    static constexpr bool scHasDefaultType = true;
    static constexpr bool scHasTypeDefinition = false;
};

struct AnyType
{
    using Traits = TypeDescriptionTraits<TypeID::Any>;

    constexpr AnyType(const TypeDescription* pDefaultType = nullptr) :
        m_pDefaultType(pDefaultType)
    {

    }

    constexpr bool IsOverloadable() const { return true; }
    constexpr const TypeDescription* GetDefaultType() const { return m_pDefaultType; }

    const TypeDescription* m_pDefaultType;
};

template<>
struct TypeDescriptionTraits<TypeID::Void>
{
    static constexpr TypeID scTypeID = TypeID::Void;
    static constexpr bool scIsOverloadable = false;
    static constexpr bool scHasDefaultType = false;
    static constexpr bool scHasTypeDefinition = true;
};

struct VoidType
{
    using Traits = TypeDescriptionTraits<TypeID::Void>;

    static llvm::Type* GetType(llvm::LLVMContext& ctx);
    bool VerifyType(llvm::Type* pType) const;
};

template<>
struct TypeDescriptionTraits<TypeID::Integer>
{
    static constexpr TypeID scTypeID = TypeID::Integer;
    static constexpr bool scIsOverloadable = true;
    static constexpr bool scHasDefaultType = false;
    static constexpr bool scHasTypeDefinition = true;
};

struct IntegerType
{
    using Traits = TypeDescriptionTraits<TypeID::Integer>;

    constexpr IntegerType(uint8_t bitWidth = 0) :
        m_BitWidth(bitWidth)
    {

    }

    static llvm::Type* GetType(llvm::LLVMContext& ctx, uint8_t bitWidth);
    llvm::Type* GetType(llvm::LLVMContext& ctx) const;
    bool VerifyType(llvm::Type* pType) const;
    constexpr bool IsOverloadable() const { return m_BitWidth == 0; }

    uint8_t m_BitWidth;
};

template<>
struct TypeDescriptionTraits<TypeID::Float>
{
    static constexpr TypeID scTypeID = TypeID::Float;
    static constexpr bool scIsOverloadable = true;
    static constexpr bool scHasDefaultType = false;
    static constexpr bool scHasTypeDefinition = true;
};

struct FloatType
{
    using Traits = TypeDescriptionTraits<TypeID::Float>;
    constexpr FloatType(uint8_t bitWidth = 0) :
        m_BitWidth(bitWidth)
    {

    }

    static llvm::Type* GetType(llvm::LLVMContext& ctx, uint8_t bitWidth);
    llvm::Type* GetType(llvm::LLVMContext& ctx) const;
    bool VerifyType(llvm::Type* pType) const;
    constexpr bool IsOverloadable() const { return m_BitWidth == 0; }

    uint8_t m_BitWidth;
};

template<>
struct TypeDescriptionTraits<TypeID::Vector>
{
    static constexpr TypeID scTypeID = TypeID::Vector;
    static constexpr bool scIsOverloadable = true;
    static constexpr bool scHasDefaultType = false;
    static constexpr bool scHasTypeDefinition = true;
};

struct VectorType
{
    using Traits = TypeDescriptionTraits<TypeID::Vector>;
    constexpr VectorType(const TypeDescription& type, uint32_t numElements = 0) :
        m_NumElements(numElements),
        m_Type(type)
    {
    }

    static llvm::Type* GetType(llvm::LLVMContext& ctx, llvm::Type* pType, uint32_t numElements);
    llvm::Type* GetType(llvm::LLVMContext& ctx) const;
    bool VerifyType(llvm::Type* pType) const;
    constexpr bool IsOverloadable() const { return m_NumElements == 0 || IGC::IsOverloadable(m_Type); }

    uint32_t m_NumElements;
    const TypeDescription& m_Type;
};

template<>
struct TypeDescriptionTraits<TypeID::Struct>
{
    static constexpr TypeID scTypeID = TypeID::Struct;
    static constexpr bool scIsOverloadable = true;
    static constexpr bool scHasDefaultType = false;
    static constexpr bool scHasTypeDefinition = true;
};

struct MemberTypeListNode
{
    constexpr MemberTypeListNode(const TypeDescription& type, const MemberTypeListNode* pNext) :
        m_Type(type),
        m_pNext(pNext)
    {
    }

    const TypeDescription& m_Type;
    const MemberTypeListNode* m_pNext;
};

struct StructType
{
    using Traits = TypeDescriptionTraits<TypeID::Struct>;
    constexpr StructType(const MemberTypeListNode* types) :
        m_pMemberTypeList(types)
    {
    }

    static llvm::Type* GetType(llvm::LLVMContext& ctx, llvm::ArrayRef<llvm::Type*> types);
    llvm::Type* GetType(llvm::LLVMContext& ctx) const;
    bool VerifyType(llvm::Type* pType) const;

    constexpr bool IsOverloadable() const
    {
        if (m_pMemberTypeList == nullptr)
        {
            return true;
        }
        const MemberTypeListNode* pCurr = m_pMemberTypeList;
        while (pCurr != nullptr)
        {
            if (IGC::IsOverloadable(pCurr->m_Type))
            {
                return true;
            }
            pCurr = pCurr->m_pNext;
        }
        return false;
    }

    const MemberTypeListNode* const m_pMemberTypeList;
};

template<>
struct TypeDescriptionTraits<TypeID::Pointer>
{
    static constexpr TypeID scTypeID = TypeID::Pointer;
    static constexpr bool scIsOverloadable = true;
    static constexpr bool scHasDefaultType = false;
    static constexpr bool scHasTypeDefinition = true;
};

struct PointerType
{
    using Traits = TypeDescriptionTraits<TypeID::Pointer>;
    constexpr PointerType(const TypeDescription& type, uint32_t addressSpace = UINT32_MAX) :
        m_AddressSpace(addressSpace),
        m_Type(type)
    {
    }

    static llvm::Type* GetType(llvm::LLVMContext& ctx, llvm::Type* pType, uint32_t addressSpace);
    llvm::Type* GetType(llvm::LLVMContext& ctx) const;
    bool VerifyType(llvm::Type* pType) const;
    constexpr bool IsOverloadable() const { return m_AddressSpace == UINT32_MAX || IGC::IsOverloadable(m_Type); }

    uint32_t m_AddressSpace;
    const TypeDescription& m_Type;
};

template<>
struct TypeDescriptionTraits<TypeID::ArgumentReference>
{
    static constexpr TypeID scTypeID = TypeID::ArgumentReference;
    static constexpr bool scIsOverloadable = true;
    static constexpr bool scHasDefaultType = false;
    static constexpr bool scHasTypeDefinition = false;
};


struct ArgumentReferenceType
{
    using Traits = TypeDescriptionTraits<TypeID::ArgumentReference>;

    constexpr ArgumentReferenceType(uint8_t index = 0) :
        m_Index(index)
    {

    }

    constexpr bool IsOverloadable() const { return true; }

    uint8_t m_Index;
};

struct TypeDescription
{
    constexpr TypeDescription() :
        m_ID(TypeID::Void),
        m_Void{ }
    {

    }
    constexpr TypeDescription(const VoidType& def) :
        m_ID(TypeID::Void),
        m_Void{ def }
    {

    }

    constexpr TypeDescription(const IntegerType& def) :
        m_ID(std::decay_t<decltype(def)>::Traits::scTypeID),
        m_Integer{ def }
    {
    }

    constexpr TypeDescription(const FloatType& def) :
        m_ID(std::decay_t<decltype(def)>::Traits::scTypeID),
        m_Float{ def }
    {
    }

    constexpr TypeDescription(const VectorType& def) :
        m_ID(std::decay_t<decltype(def)>::Traits::scTypeID),
        m_Vector{ def }
    {
    }

    constexpr TypeDescription(const PointerType& def) :
        m_ID(std::decay_t<decltype(def)>::Traits::scTypeID),
        m_Pointer{ def }
    {
    }

    constexpr TypeDescription(const StructType& def) :
        m_ID(std::decay_t<decltype(def)>::Traits::scTypeID),
        m_Struct{ def }
    {
    }

    constexpr TypeDescription(const AnyType& def) :
        m_ID(std::decay_t<decltype(def)>::Traits::scTypeID),
        m_Any{ def }
    {
    }

    constexpr TypeDescription(const ArgumentReferenceType& def) :
        m_ID(std::decay_t<decltype(def)>::Traits::scTypeID),
        m_ArgumentReference{ def }
    {
    }

    template<typename LamdbaT>
    constexpr void Visit(LamdbaT&& func) const
    {
        switch (m_ID)
        {
        case TypeID::Void:
            func(m_Void);
            break;
        case TypeID::Integer:
            func(m_Integer);
            break;
        case TypeID::Float:
            func(m_Float);
            break;
        case TypeID::Vector:
            func(m_Vector);
            break;
        case TypeID::Any:
            func(m_Any);
            break;
        case TypeID::Pointer:
            func(m_Pointer);
            break;
        case TypeID::Struct:
            func(m_Struct);
            break;
        case TypeID::ArgumentReference:
            func(m_ArgumentReference);
            break;
        default:
            break;
        }
    }

    llvm::Type* GetType(llvm::LLVMContext& ctx) const;
    bool VerifyType(llvm::Type* pType) const;

    constexpr const TypeDescription* GetDefaultType(llvm::LLVMContext& ctx) const
    {
        const TypeDescription* pResult = nullptr;
        Visit([&ctx, &pResult](const auto& currType) {
            using T = std::decay_t<decltype(currType)>;
            using Traits = typename T::Traits;
            if constexpr (Traits::scHasDefaultType)
            {
                pResult = currType.GetDefaultType();
            }
            });
        return pResult;
    }

    constexpr bool IsOverloadable() const
    {
        bool isOverloadable = false;
        Visit([&isOverloadable](const auto& currType)
            {
                using T = std::decay_t<decltype(currType)>;
                using Traits = typename T::Traits;
                if constexpr (Traits::scIsOverloadable)
                {
                    isOverloadable = currType.IsOverloadable();
                }
            });
        return isOverloadable;
    }

    const union
    {
        VoidType m_Void;
        IntegerType m_Integer;
        FloatType m_Float;
        VectorType m_Vector;
        AnyType m_Any;
        PointerType m_Pointer;
        StructType m_Struct;
        ArgumentReferenceType m_ArgumentReference;
    };

    const TypeID m_ID;
};

constexpr bool IsOverloadable(const TypeDescription& type)
{
    return type.IsOverloadable();
}

struct EmptyTypeHolderT
{
    static constexpr TypeDescription scType = VoidType();
};

template<uint8_t index>
struct ArgumentReferenceTypeHolderT
{
    static constexpr TypeDescription scType = ArgumentReferenceType(index);
};

template<typename TypeHolderT = EmptyTypeHolderT>
struct AnyTypeHolderT
{
    static constexpr TypeDescription scType =
        AnyType(std::is_same_v<TypeHolderT, EmptyTypeHolderT> ? nullptr : &TypeHolderT::scType);
};

struct VoidTypeHolderT
{
    static constexpr TypeDescription scType = VoidType();
};

template<uint8_t numBits>
struct IntegerTypeHolderT
{
    static_assert(numBits == 0 || numBits == 1 || numBits == 8 || numBits == 16 || numBits == 32 || numBits == 64);
    static constexpr TypeDescription scType = IntegerType(numBits);
};

template<uint8_t numBits>
struct FloatTypeHolderT
{
    static_assert(numBits == 0 || numBits == 16 || numBits == 32 || numBits == 64);
    static constexpr TypeDescription scType = FloatType(numBits);
};

template<typename TypeHolderT, uint32_t numElements = 0>
struct VectorTypeHolderT
{
    static_assert(numElements == 0 || numElements == 2 || numElements == 4 || numElements == 8 || numElements == 16);
    static constexpr const TypeDescription& scElementType = TypeHolderT::scType;
    static constexpr TypeDescription scType = VectorType(scElementType, numElements);
};

template<typename TypeHolderT, uint32_t addressSpace = UINT32_MAX>
struct PointerTypeHolderT
{
    static constexpr const TypeDescription& scElementType = TypeHolderT::scType;
    static constexpr TypeDescription scType = PointerType(scElementType, addressSpace);
};

template<typename ...RestT>
struct MemberTypeListHolderT
{
    static constexpr MemberTypeListNode* scpMemberListNode = nullptr;
};

template<typename TypeHolderT, typename ...RestT>
struct MemberTypeListHolderT<TypeHolderT, RestT...>
{
    static constexpr const TypeDescription& scMemberType = TypeHolderT::scType;
    static constexpr const MemberTypeListNode* scNextNode = MemberTypeListHolderT<RestT...>::scpMemberListNode;
    static constexpr MemberTypeListNode scMemberListNode = MemberTypeListNode(scMemberType, scNextNode);
    static constexpr const MemberTypeListNode* scpMemberListNode = &scMemberListNode;
};

template<typename MemberTypeListT>
struct StructTypeHolderT
{
    static constexpr const MemberTypeListNode* scpMemberListNode = MemberTypeListT::scpMemberListNode;
    static constexpr TypeDescription scType = StructType(scpMemberListNode);
};

} // namespace IGC