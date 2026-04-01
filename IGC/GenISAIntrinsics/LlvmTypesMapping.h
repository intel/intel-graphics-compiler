/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <type_traits>
#include <stdint.h>
#include <array>
#include <algorithm>

#include "llvmWrapper/Support/ModRef.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Attributes.h"
#include "common/LLVMWarningsPop.hpp"

namespace llvm {
class Type;
class LLVMContext;
class TypeDescription;
} // namespace llvm

namespace IGC {

enum class TypeID : uint32_t { Void, Integer, Float, Vector, Struct, Pointer, Any, ArgumentReference, TypeList, Count };

template <TypeID id> struct TypeDescriptionTraits;

struct TypeDescription;

constexpr bool IsOverloadable(const TypeDescription &type);

// ---------------------------------------------------------------------------
// ValueBound / ValueConstraint - generic building blocks reused by any type
// property that is either a literal constant or extracted from another
// argument (e.g. vector element count, integer bit-width).
/// Which scalar property to extract from a referenced argument's type.
enum class PropertyExtractionKind : uint8_t {
  VectorNumElements = 0, ///< Number of elements of a FixedVectorType.
  IntegerBitWidth = 1,   ///< Bit-width of an IntegerType.
};

/// A single bound value: either a literal uint32_t or a property extracted
/// from the type of another argument.
///   - Literal 0 means "unbounded" when used as a range bound.
struct ValueBound {
  enum class Kind : uint8_t { Literal = 0, PropertyRef = 1 };

  Kind m_Kind;
  union {
    uint32_t m_Value; ///< Literal constant (0 = unbounded).
    struct {
      uint8_t m_Index;                     ///< Index into the intrinsic's type array (0 = return type, 1+ = arguments).
      PropertyExtractionKind m_Extraction; ///< Which property to extract.
    } m_Ref;
  };

  constexpr ValueBound() : m_Kind(Kind::Literal), m_Value(0) {}
  constexpr explicit ValueBound(uint32_t value) : m_Kind(Kind::Literal), m_Value(value) {}
  constexpr ValueBound(uint8_t index, PropertyExtractionKind extraction)
      : m_Kind(Kind::PropertyRef), m_Ref{index, extraction} {}

  constexpr bool IsLiteral() const { return m_Kind == Kind::Literal; }
  constexpr bool IsRef() const { return m_Kind == Kind::PropertyRef; }
  constexpr bool IsUnbounded() const { return IsLiteral() && m_Value == 0; }
};

/// Describes an accepted range for a type property.
///   - Both bounds literal-0           -> unconstrained (any value).
///   - low == high (both literal, !=0) -> exact match (preferred short form).
///   - Otherwise                       -> range [low, high]  (0 = open on that side).
struct ValueConstraint {
  ValueBound m_Low;
  ValueBound m_High;

  /// Unconstrained.
  constexpr ValueConstraint() = default;

  /// Exact value (literal or ref).
  constexpr explicit ValueConstraint(ValueBound exact) : m_Low(exact), m_High(exact) {}

  /// Range [low, high].
  constexpr ValueConstraint(ValueBound low, ValueBound high) : m_Low(low), m_High(high) {}

  constexpr bool IsExact() const {
    return m_Low.IsLiteral() && m_High.IsLiteral() && m_Low.m_Value != 0 && m_Low.m_Value == m_High.m_Value;
  }
  constexpr bool IsAny() const { return m_Low.IsUnbounded() && m_High.IsUnbounded(); }
  constexpr bool IsRange() const { return !IsExact() && !IsAny(); }

  /// Returns true when both bounds are literals but the constraint is not
  /// an exact single value -- i.e. the value cannot be determined without an
  /// externally supplied (overloaded) type.  PropertyRef bounds are excluded
  /// because they are resolved from other arguments.
  constexpr bool IsLiteralUndetermined() const { return m_Low.IsLiteral() && m_High.IsLiteral() && !IsExact(); }

  /// Returns the exact literal value. Only valid when IsExact() is true.
  constexpr uint32_t GetExact() const { return m_Low.m_Value; }
  /// Returns the low bound. Only valid for literal bounds.
  constexpr const ValueBound &GetLow() const { return m_Low; }
  /// Returns the high bound. Only valid for literal bounds.
  constexpr const ValueBound &GetHigh() const { return m_High; }
};

/// Context for resolving types during intrinsic function type construction.
/// Holds already-resolved types (for ArgumentReference/PropertyRef) and
/// overloaded types (for Any/TypeList/overloadable types), with a mutable
/// index that advances each time an overloaded type is consumed.
struct TypeResolutionContext {
  llvm::ArrayRef<llvm::Type *> m_OverloadedTypes; ///< Caller-supplied overloaded types.
  uint8_t m_OverloadedTypeIndex = 0;              ///< Mutable index into m_OverloadedTypes.

  TypeResolutionContext() = default;
  TypeResolutionContext(llvm::ArrayRef<llvm::Type *> overloadedTypes) : m_OverloadedTypes(overloadedTypes) {}

  /// Consume and return the next overloaded type, or nullptr if exhausted.
  llvm::Type *ConsumeOverloadedType() {
    if (m_OverloadedTypeIndex < m_OverloadedTypes.size())
      return m_OverloadedTypes[m_OverloadedTypeIndex++];
    return nullptr;
  }
};

template <> struct TypeDescriptionTraits<TypeID::Any> {
  static constexpr TypeID scTypeID = TypeID::Any;
  static constexpr bool scIsOverloadable = true;
  static constexpr bool scHasDefaultType = true;
  static constexpr bool scHasTypeDefinition = false;
};

struct AnyType {
  using Traits = TypeDescriptionTraits<TypeID::Any>;

  constexpr AnyType(const TypeDescription *pDefaultType = nullptr) : m_pDefaultType(pDefaultType) {}

  constexpr bool IsOverloadable() const { return true; }
  constexpr const TypeDescription *GetDefaultType() const { return m_pDefaultType; }

  const TypeDescription *m_pDefaultType;
};

template <> struct TypeDescriptionTraits<TypeID::Void> {
  static constexpr TypeID scTypeID = TypeID::Void;
  static constexpr bool scIsOverloadable = false;
  static constexpr bool scHasDefaultType = false;
  static constexpr bool scHasTypeDefinition = true;
};

struct VoidType {
  using Traits = TypeDescriptionTraits<TypeID::Void>;

  static llvm::Type *GetType(llvm::LLVMContext &ctx);
  bool VerifyType(llvm::Type *pType) const;
};

template <> struct TypeDescriptionTraits<TypeID::Integer> {
  static constexpr TypeID scTypeID = TypeID::Integer;
  static constexpr bool scIsOverloadable = true;
  static constexpr bool scHasDefaultType = false;
  static constexpr bool scHasTypeDefinition = true;
};

struct IntegerType {
  using Traits = TypeDescriptionTraits<TypeID::Integer>;

  /// Exact literal bit-width (0 = any).
  constexpr IntegerType(uint8_t bitWidth) : m_BitWidth(ValueConstraint(ValueBound(static_cast<uint32_t>(bitWidth)))) {}

  constexpr IntegerType(uint8_t bitWidthLo, uint8_t bitWidthHi)
      : m_BitWidth(ValueConstraint(ValueBound(static_cast<uint32_t>(bitWidthLo)),
                                   ValueBound(static_cast<uint32_t>(bitWidthHi)))) {}

  /// General constraint.
  constexpr explicit IntegerType(ValueConstraint bitWidth) : m_BitWidth(bitWidth) {}

  static llvm::Type *GetType(llvm::LLVMContext &ctx, uint8_t bitWidth);
  llvm::Type *GetType(llvm::LLVMContext &ctx, TypeResolutionContext &resCtx) const;
  llvm::Type *GetType(llvm::LLVMContext &ctx) const;
  bool VerifyType(llvm::Type *pType) const;
  constexpr bool IsOverloadable() const { return m_BitWidth.IsLiteralUndetermined(); }

  ValueConstraint m_BitWidth;
};

template <> struct TypeDescriptionTraits<TypeID::Float> {
  static constexpr TypeID scTypeID = TypeID::Float;
  static constexpr bool scIsOverloadable = true;
  static constexpr bool scHasDefaultType = false;
  static constexpr bool scHasTypeDefinition = true;
};

struct FloatType {
  using Traits = TypeDescriptionTraits<TypeID::Float>;
  constexpr FloatType(uint8_t bitWidth = 0) : m_BitWidth(bitWidth) {}

  static llvm::Type *GetType(llvm::LLVMContext &ctx, uint8_t bitWidth);
  llvm::Type *GetType(llvm::LLVMContext &ctx) const;
  bool VerifyType(llvm::Type *pType) const;
  constexpr bool IsOverloadable() const { return m_BitWidth == 0; }

  uint8_t m_BitWidth;
};

template <> struct TypeDescriptionTraits<TypeID::Vector> {
  static constexpr TypeID scTypeID = TypeID::Vector;
  static constexpr bool scIsOverloadable = true;
  static constexpr bool scHasDefaultType = false;
  static constexpr bool scHasTypeDefinition = true;
};

struct VectorType {
  using Traits = TypeDescriptionTraits<TypeID::Vector>;

  /// General constraint on element count.
  constexpr VectorType(const TypeDescription &type, ValueConstraint numElements = {})
      : m_NumElements(numElements), m_Type(type) {}

  /// Convenience: exact literal element count (0 = any).
  constexpr VectorType(const TypeDescription &type, uint32_t numElements)
      : m_NumElements(ValueConstraint(ValueBound(numElements))), m_Type(type) {}

  /// Convenience: literal range [low, high].
  constexpr VectorType(const TypeDescription &type, uint32_t low, uint32_t high)
      : m_NumElements(ValueConstraint(ValueBound(low), ValueBound(high))), m_Type(type) {}

  static llvm::Type *GetType(llvm::LLVMContext &ctx, llvm::Type *pType, uint32_t numElements);
  llvm::Type *GetType(llvm::LLVMContext &ctx) const;
  llvm::Type *GetType(llvm::LLVMContext &ctx, TypeResolutionContext &resCtx) const;
  bool VerifyType(llvm::Type *pType) const;
  constexpr bool IsOverloadable() const { return m_NumElements.IsLiteralUndetermined() || IGC::IsOverloadable(m_Type); }

  ValueConstraint m_NumElements;
  const TypeDescription &m_Type;
};

template <> struct TypeDescriptionTraits<TypeID::Struct> {
  static constexpr TypeID scTypeID = TypeID::Struct;
  static constexpr bool scIsOverloadable = true;
  static constexpr bool scHasDefaultType = false;
  static constexpr bool scHasTypeDefinition = true;
};

struct StructType {
  using Traits = TypeDescriptionTraits<TypeID::Struct>;
  constexpr StructType(const TypeDescription *pMemberTypes = nullptr, uint32_t numMembers = 0)
      : m_pMemberTypes(pMemberTypes), m_NumMembers(numMembers) {}

  static llvm::Type *GetType(llvm::LLVMContext &ctx, llvm::ArrayRef<llvm::Type *> types);
  llvm::Type *GetType(llvm::LLVMContext &ctx) const;
  llvm::Type *GetType(llvm::LLVMContext &ctx, TypeResolutionContext &resCtx) const;
  bool VerifyType(llvm::Type *pType) const;

  constexpr bool IsOverloadable() const;

  const TypeDescription *m_pMemberTypes;
  uint32_t m_NumMembers;
};

template <> struct TypeDescriptionTraits<TypeID::Pointer> {
  static constexpr TypeID scTypeID = TypeID::Pointer;
  static constexpr bool scIsOverloadable = true;
  static constexpr bool scHasDefaultType = false;
  static constexpr bool scHasTypeDefinition = true;
};

struct PointerType {
  using Traits = TypeDescriptionTraits<TypeID::Pointer>;
  constexpr PointerType(const TypeDescription &type, uint32_t addressSpace = UINT32_MAX)
      : m_AddressSpace(addressSpace), m_Type(type) {}

  static llvm::Type *GetType(llvm::LLVMContext &ctx, llvm::Type *pType, uint32_t addressSpace);
  llvm::Type *GetType(llvm::LLVMContext &ctx) const;
  llvm::Type *GetType(llvm::LLVMContext &ctx, TypeResolutionContext &resCtx) const;
  bool VerifyType(llvm::Type *pType) const;
  constexpr bool IsOverloadable() const { return m_AddressSpace == UINT32_MAX || IGC::IsOverloadable(m_Type); }

  uint32_t m_AddressSpace;
  const TypeDescription &m_Type;
};

template <> struct TypeDescriptionTraits<TypeID::ArgumentReference> {
  static constexpr TypeID scTypeID = TypeID::ArgumentReference;
  static constexpr bool scIsOverloadable = true;
  static constexpr bool scHasDefaultType = false;
  static constexpr bool scHasTypeDefinition = false;
};

enum class ReferenceExtractionType : uint8_t {
  None = 0,
  VectorElement = 1,
};

struct ReferenceType {
  using Traits = TypeDescriptionTraits<TypeID::ArgumentReference>;

  constexpr ReferenceType(uint8_t index = 0, ReferenceExtractionType extraction = ReferenceExtractionType::None)
      : m_Index(index), m_Extraction(extraction) {}

  constexpr bool IsOverloadable() const { return false; }

  uint8_t m_Index;
  ReferenceExtractionType m_Extraction;
};

template <> struct TypeDescriptionTraits<TypeID::TypeList> {
  static constexpr TypeID scTypeID = TypeID::TypeList;
  static constexpr bool scIsOverloadable = true;
  static constexpr bool scHasDefaultType = false;
  static constexpr bool scHasTypeDefinition = false;
};

struct TypeListType {
  using Traits = TypeDescriptionTraits<TypeID::TypeList>;
  constexpr TypeListType(const TypeDescription *pTypes = nullptr, uint32_t numTypes = 0)
      : m_pTypes(pTypes), m_NumTypes(numTypes) {}

  constexpr bool IsOverloadable() const { return true; }
  bool VerifyType(llvm::Type *pType) const;

  const TypeDescription *m_pTypes;
  uint32_t m_NumTypes;
};

struct TypeDescription {
  constexpr TypeDescription() : m_ID(TypeID::Void), m_Void{} {}
  constexpr TypeDescription(const VoidType &def) : m_ID(TypeID::Void), m_Void{def} {}

  constexpr TypeDescription(const IntegerType &def)
      : m_ID(std::decay_t<decltype(def)>::Traits::scTypeID), m_Integer{def} {}

  constexpr TypeDescription(const FloatType &def) : m_ID(std::decay_t<decltype(def)>::Traits::scTypeID), m_Float{def} {}

  constexpr TypeDescription(const VectorType &def)
      : m_ID(std::decay_t<decltype(def)>::Traits::scTypeID), m_Vector{def} {}

  constexpr TypeDescription(const PointerType &def)
      : m_ID(std::decay_t<decltype(def)>::Traits::scTypeID), m_Pointer{def} {}

  constexpr TypeDescription(const StructType &def)
      : m_ID(std::decay_t<decltype(def)>::Traits::scTypeID), m_Struct{def} {}

  constexpr TypeDescription(const AnyType &def) : m_ID(std::decay_t<decltype(def)>::Traits::scTypeID), m_Any{def} {}

  constexpr TypeDescription(const ReferenceType &def)
      : m_ID(std::decay_t<decltype(def)>::Traits::scTypeID), m_Reference{def} {}

  constexpr TypeDescription(const TypeListType &def)
      : m_ID(std::decay_t<decltype(def)>::Traits::scTypeID), m_TypeList{def} {}

  template <typename LamdbaT> constexpr void Visit(LamdbaT &&func) const {
    switch (m_ID) {
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
      func(m_Reference);
      break;
    case TypeID::TypeList:
      func(m_TypeList);
      break;
    default:
      break;
    }
  }

  llvm::Type *GetType(llvm::LLVMContext &ctx) const;
  llvm::Type *GetType(llvm::LLVMContext &ctx, TypeResolutionContext &resCtx) const;
  bool VerifyType(llvm::Type *pType) const;

  constexpr const TypeDescription *GetDefaultType(llvm::LLVMContext &ctx) const {
    const TypeDescription *pResult = nullptr;
    Visit([&ctx, &pResult](const auto &currType) {
      using T = std::decay_t<decltype(currType)>;
      using Traits = typename T::Traits;
      if constexpr (Traits::scHasDefaultType) {
        pResult = currType.GetDefaultType();
      }
    });
    return pResult;
  }

  constexpr bool IsOverloadable() const {
    bool isOverloadable = false;
    Visit([&isOverloadable](const auto &currType) {
      using T = std::decay_t<decltype(currType)>;
      using Traits = typename T::Traits;
      if constexpr (Traits::scIsOverloadable) {
        isOverloadable = currType.IsOverloadable();
      }
    });
    return isOverloadable;
  }

  const union {
    VoidType m_Void;
    IntegerType m_Integer;
    FloatType m_Float;
    VectorType m_Vector;
    AnyType m_Any;
    PointerType m_Pointer;
    StructType m_Struct;
    ReferenceType m_Reference;
    TypeListType m_TypeList;
  };

  const TypeID m_ID;
};

constexpr bool StructType::IsOverloadable() const {
  if (m_pMemberTypes == nullptr || m_NumMembers == 0)
    return true;
  return std::any_of(m_pMemberTypes, m_pMemberTypes + m_NumMembers,
                     [](const TypeDescription &type) { return IGC::IsOverloadable(type); });
}

struct ArgumentDescription {
  constexpr ArgumentDescription(const TypeDescription &type)
      : m_Type(type), m_AttrKind(llvm::Attribute::None), m_Capture(std::nullopt) {}

  constexpr ArgumentDescription(const TypeDescription &type, llvm::Attribute::AttrKind attrKind)
      : m_Type(type), m_AttrKind(attrKind), m_Capture(std::nullopt) {}

  constexpr ArgumentDescription(const TypeDescription &type, std::optional<IGCLLVM::CaptureComponents> captureComponent)
      : m_Type(type), m_AttrKind(llvm::Attribute::None), m_Capture(captureComponent) {}

  constexpr ArgumentDescription(const TypeDescription &type, llvm::Attribute::AttrKind attrKind,
                                std::optional<IGCLLVM::CaptureComponents> captureComponent)
      : m_Type(type), m_AttrKind(attrKind), m_Capture(captureComponent) {}

  const TypeDescription &m_Type;
  llvm::Attribute::AttrKind m_AttrKind;
  std::optional<IGCLLVM::CaptureComponents> m_Capture;
};

constexpr bool IsOverloadable(const TypeDescription &type) { return type.IsOverloadable(); }

struct EmptyTypeHolderT {
  static constexpr TypeDescription scType = VoidType();
};

template <uint8_t index, ReferenceExtractionType extraction = ReferenceExtractionType::None>
struct ReferenceTypeHolderT {
  static constexpr TypeDescription scType = ReferenceType(index, extraction);
};

template <typename TypeHolderT = EmptyTypeHolderT> struct AnyTypeHolderT {
  static constexpr TypeDescription scType =
      AnyType(std::is_same_v<TypeHolderT, EmptyTypeHolderT> ? nullptr : &TypeHolderT::scType);
};

struct VoidTypeHolderT {
  static constexpr TypeDescription scType = VoidType();
};

/// Literal bound (value 0 = unbounded).
template <uint32_t value> struct LiteralBoundHolderT {
  static constexpr ValueBound scBound = ValueBound(value);
};

/// Bound extracted from another argument's type property.
template <uint8_t index, PropertyExtractionKind extraction> struct PropertyRefBoundHolderT {
  static constexpr ValueBound scBound = ValueBound(index, extraction);
};

template <uint8_t numBitsLo, uint8_t numBitsHi = numBitsLo> struct IntegerTypeHolderT {
  static_assert(numBitsLo == 0 || numBitsLo == 1 || numBitsLo == 8 || numBitsLo == 16 || numBitsLo == 32 ||
                numBitsLo == 64);
  static_assert(numBitsHi == 0 || numBitsHi == 1 || numBitsHi == 8 || numBitsHi == 16 || numBitsHi == 32 ||
                numBitsHi == 64);
  static_assert(numBitsHi >= numBitsLo || numBitsHi == 0);
  static constexpr TypeDescription scType = IntegerType(numBitsLo, numBitsHi);
};

/// Integer whose bit-width is described by an arbitrary ValueConstraint.
template <typename ConstraintHolderT> struct IntegerTypeConstrainedHolderT {
  static constexpr TypeDescription scType = IntegerType(ConstraintHolderT::scConstraint);
};

template <uint8_t numBits> struct FloatTypeHolderT {
  static_assert(numBits == 0 || numBits == 16 || numBits == 32 || numBits == 64);
  static constexpr TypeDescription scType = FloatType(numBits);
};

/// Exact literal element count (0 = any), or literal range.
template <typename TypeHolderT, uint32_t numElementsLow = 0, uint32_t numElementsHigh = numElementsLow>
struct VectorTypeHolderT {
  static_assert(numElementsHigh >= numElementsLow || numElementsHigh == 0);
  static constexpr const TypeDescription &scElementType = TypeHolderT::scType;
  static constexpr TypeDescription scType = VectorType(scElementType, numElementsLow, numElementsHigh);
};

/// Vector whose element count is described by an arbitrary ValueConstraint.
template <typename TypeHolderT, typename ConstraintHolderT> struct VectorTypeConstrainedHolderT {
  static constexpr const TypeDescription &scElementType = TypeHolderT::scType;
  static constexpr TypeDescription scType = VectorType(scElementType, ConstraintHolderT::scConstraint);
};

/// Exact-match constraint from a single ValueBound holder.
template <typename BoundHolderT> struct ExactConstraintHolderT {
  static constexpr ValueConstraint scConstraint = ValueConstraint(BoundHolderT::scBound);
};

/// Range constraint from two ValueBound holders.
template <typename LowBoundHolderT, typename HighBoundHolderT> struct RangeConstraintHolderT {
  static constexpr ValueConstraint scConstraint = ValueConstraint(LowBoundHolderT::scBound, HighBoundHolderT::scBound);
};

template <typename TypeHolderT, uint32_t addressSpace = UINT32_MAX> struct PointerTypeHolderT {
  static constexpr const TypeDescription &scElementType = TypeHolderT::scType;
  static constexpr TypeDescription scType = PointerType(scElementType, addressSpace);
};

template <typename... TypeHolderTs> struct StructTypeHolderT {
  static constexpr std::array<TypeDescription, sizeof...(TypeHolderTs)> scMemberTypes = {TypeHolderTs::scType...};
  static constexpr TypeDescription scType = StructType(scMemberTypes.data(), sizeof...(TypeHolderTs));
};

template <typename... TypeHolderTs> struct TypeListTypeHolderT {
  static constexpr std::array<TypeDescription, sizeof...(TypeHolderTs)> scTypes = {TypeHolderTs::scType...};
  static constexpr TypeDescription scType = TypeListType(scTypes.data(), sizeof...(TypeHolderTs));
};

} // namespace IGC