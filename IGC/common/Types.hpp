/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGC_COMMON_TYPES_H
#define IGC_COMMON_TYPES_H

#include "IGC/common/StringMacros.hpp"
#include "3d/common/iStdLib/types.h"

#include "AdaptorCommon/API/igc.h"
#include "common/debug/Debug.hpp"

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/Support/TypeSize.h"
#include "IGC/common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"
#include "IGC/common/shaderHash.hpp"
#include "EmUtils.h"

namespace USC
{
    struct ShaderD3D;
}

namespace IGC
{
    enum PrecisionType : uint8_t
    {
        PRECISION_UNUSED, U8, U4, U2, S8, S4, S2,
        BF8,
        HF8,
        TF32 = 10,
        BF16, FP16
    };

    inline uint32_t getPrecisionInBits(PrecisionType P)
    {
        switch (P)
        {
        default:
            break;
        case BF16:
        case FP16:
            return 16;
        case U8:
        case S8:
            return 8;
        case U4:
        case S4:
            return 4;
        case U2:
        case S2:
            return 2;
        // PVC
        case TF32:
            return 32;
        }
        return 0;
    }
}

enum class SIMDMode : unsigned char
{
    UNKNOWN,
    SIMD1,
    SIMD2,
    SIMD4,
    SIMD8,
    SIMD16,
    SIMD32,
    END,
    BEGIN = 0
};

enum class SIMDStatus : unsigned char
{
    SIMD_BEGIN = 0,
    SIMD_PASS,
    SIMD_FUNC_FAIL,
    SIMD_PERF_FAIL,
    SIMD_END
};

inline uint16_t numLanes(SIMDMode width)
{
    switch(width)
    {
    case SIMDMode::SIMD1   : return 1;
    case SIMDMode::SIMD2   : return 2;
    case SIMDMode::SIMD4   : return 4;
    case SIMDMode::SIMD8   : return 8;
    case SIMDMode::SIMD16  : return 16;
    case SIMDMode::SIMD32  : return 32;
    case SIMDMode::UNKNOWN :
    default:
        IGC_ASSERT_MESSAGE(0, "unreachable");
        return 1;
    }
}

inline SIMDMode lanesToSIMDMode(unsigned lanes) {
    switch (lanes) {
    case  1: return SIMDMode::SIMD1;
    case  2: return SIMDMode::SIMD2;
    case  4: return SIMDMode::SIMD4;
    case  8: return SIMDMode::SIMD8;
    case 16: return SIMDMode::SIMD16;
    case 32: return SIMDMode::SIMD32;
    default:
        IGC_ASSERT_MESSAGE(0, "Unexpected number of lanes!");
        return SIMDMode::UNKNOWN;
    }
}

enum class ShaderType
{
#include "ShaderTypesIncl.h"
};

enum class ShaderDispatchMode
{
    NOT_APPLICABLE,
    SINGLE_PATCH,
    DUAL_PATCH,
    EIGHT_PATCH,
    DUAL_SIMD8,
    QUAD_SIMD8_DYNAMIC, // 3DTATE_PS_BODY::PolyPackingPolicy::POLY_PACK8_DYNAMIC
    END,
    BEGIN = 0
};

static const char *ShaderTypeString[] = {
    "ERROR",
    "VS",
    "HS",
    "DS",
    "GS",
    "TASK",
    "MESH",
    "PS",
    "CS",
    "OCL",
    "RAYDISPATCH",
    "ERROR"
};

static_assert(sizeof(ShaderTypeString) / sizeof(*ShaderTypeString) == static_cast<size_t>(ShaderType::END) + 1,
    "Update the array");

template <typename TIter>
class RangeWrapper
{
public:
    TIter& begin() { return m_first; }
    TIter& end() { return m_last; }

private:
    RangeWrapper( TIter first, TIter last )
        : m_first(first)
        , m_last(last)
    { }

    TIter m_first;
    TIter m_last;

    template <typename T>
    friend inline RangeWrapper<T> range( const T& first, const T& last );
};

/**
 * \brief Create a proxy to iterate over a container's nonstandard iterator pairs via c++11 range-for
 *
 * This is to be used for containers that provide multiple sets of iterators
 * such as llvm::Function, which has begin()/end() and arg_begin()/arg_end().
 * In the latter case, range-for cannot be used without such a proxy.
 *
 * \example
 *     for ( auto arg : range( pFunc->arg_begin(), pFunc->arg_end() ) )
 *     {
 *         ...
 *     }
 */
template <typename TIter>
inline RangeWrapper<TIter> range( const TIter& first, const TIter& last )
{
    return RangeWrapper<TIter>(first, last);
}

template <typename TEnum>
class EnumIter
{
public:
    EnumIter operator++(/* prefix */)
    {
        increment_me();
        return *this;
    }
    EnumIter operator++(int /* postfix */)
    {
        TEnum old(m_val);
        increment_me();
        return old;
    }
    TEnum operator*() const
    {
        return m_val;
    }
    bool operator!=(EnumIter const& other) const
    {
        return !is_equal(other);
    }
    bool operator==(EnumIter const& other) const
    {
        return is_equal(other);
    }
private:
    EnumIter(TEnum init)
        : m_val(init)
    {}
    bool is_equal(EnumIter const& other) const
    {
        return m_val == other.m_val;
    }
    void increment_me()
    {
         m_val = static_cast<TEnum>( static_cast<unsigned int>(m_val) + 1 );
    }

    template <typename T>
    friend inline RangeWrapper<EnumIter<T>> eRange(T, T);

    TEnum m_val;
};

/**
 * \brief Create a proxy to iterate over every element of an enumeration
 *
 * Assumes that TEnum follows this pattern, with no gaps between the enumerated values:
 * enum class EFoo {
 *     E1,
 *     E2,
 *     END,
 *     BEGIN = 0
 * };
 */
template <typename TEnum>
inline RangeWrapper<EnumIter<TEnum>> eRange( TEnum begin = TEnum::BEGIN, TEnum end = TEnum::END)
{
    return range( EnumIter<TEnum>(begin), EnumIter<TEnum>(end) );
}

/// Template that should be used when a static_cast of a larger integer
/// type to a smaller one is required.
/// In debug, this will check if the source argument doesn't exceed the target type range.
/// In release, it is just static_cast with no check.
///
/// Note that the cases where one needs to typecast integer types should be infrequent.
/// Could be necessary when dealing with other interfaces, otherwise think twice before
/// using it - maybe changing types or modifying the code design would be better.
template <typename TDst, typename TSrc>
inline typename std::enable_if<
    std::is_signed<TDst>::value && std::is_signed<TSrc>::value,
    TDst>::type int_cast(TSrc value)
{
    static_assert(std::is_integral<TDst>::value && std::is_integral<TSrc>::value,
        "int_cast<>() should be used only for conversions between integer types.");

    IGC_ASSERT(std::numeric_limits<TDst>::min() <= value);
    IGC_ASSERT(value <= std::numeric_limits<TDst>::max());

    return static_cast<TDst>(value);
}

template <typename TDst, typename TSrc>
inline typename std::enable_if<
    std::is_signed<TDst>::value && std::is_unsigned<TSrc>::value,
    TDst>::type int_cast(TSrc value)
{
    static_assert(std::is_integral<TDst>::value && std::is_integral<TSrc>::value,
        "int_cast<>() should be used only for conversions between integer types.");

    IGC_ASSERT(value <= static_cast<typename std::make_unsigned<TDst>::type>(std::numeric_limits<TDst>::max()));

    return static_cast<TDst>(value);
}

template <typename TDst, typename TSrc>
inline typename std::enable_if<
    std::is_unsigned<TDst>::value && std::is_signed<TSrc>::value,
    TDst>::type int_cast(TSrc value)
{
    static_assert(std::is_integral<TDst>::value && std::is_integral<TSrc>::value,
        "int_cast<>() should be used only for conversions between integer types.");

    IGC_ASSERT(0 <= value);
    IGC_ASSERT(static_cast<typename std::make_unsigned<TSrc>::type>(value) <= std::numeric_limits<TDst>::max());

    return static_cast<TDst>(value);
}

template <typename TDst, typename TSrc>
inline typename std::enable_if<
    std::is_unsigned<TDst>::value && std::is_unsigned<TSrc>::value,
    TDst>::type int_cast(TSrc value)
{
    static_assert(std::is_integral<TDst>::value && std::is_integral<TSrc>::value,
        "int_cast<>() should be used only for conversions between integer types.");

    IGC_ASSERT(value <= std::numeric_limits<TDst>::max());
    return static_cast<TDst>(value);
}

template <typename TDst>
inline typename std::enable_if<
    std::is_unsigned<TDst>::value,
    TDst>::type int_cast(llvm::TypeSize value)
{
    static_assert(std::is_integral<TDst>::value,
        "int_cast<>() should be used only for conversions between integer types.");

    IGC_ASSERT(value.getFixedValue() <= std::numeric_limits<TDst>::max());
    return static_cast<TDst>(value.getFixedValue());
}

template <typename TDst>
inline typename std::enable_if<
    std::is_signed<TDst>::value,
    TDst>::type int_cast(llvm::TypeSize value)
{
    static_assert(std::is_integral<TDst>::value,
        "int_cast<>() should be used only for conversions between integer types.");

    IGC_ASSERT(value.getFixedValue() <= static_cast<typename std::make_unsigned<TDst>::type>(std::numeric_limits<TDst>::max()));
    return static_cast<TDst>(value.getFixedValue());
}

#endif //IGC_COMMON_TYPES_H
