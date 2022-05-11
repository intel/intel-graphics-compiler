/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/debug/Debug.hpp"
#include <limits>
#include <type_traits>
#include "Probe/Assertion.h"

template<typename T, typename TUnder = unsigned int, int TRatio = T::ratio>
class Unit
{
private:
    static_assert(std::is_integral<TUnder>::value, "TUnder must be of integral type");
    static_assert(
        TRatio == T::ratio,
        "Third template parameter gives the ratio information to the vs2012 natvis debugger."
        "For debugging sanity, this must be the same as the ratio itself." );

    typedef T       myT;
    typedef TUnder  myTUnder;

public:

    /// Default constructor initializes the unit to value zero.
    Unit()
        : m_value( 0 * T::ratio)
    {
    }

    /// Constructor initializing with the unit value.
    explicit Unit(TUnder value)
        : m_value(value * T::ratio)
    {
    }

    // Copy constructor
    template<typename R>
    Unit(Unit<R, TUnder> const& other)
    {
        // TODO: copy assertion from inside this operator to here to make things more clear
        this->operator=(other);
    }

    /// Assignment operator.
    template<typename R>
    Unit<T, TUnder> operator =(Unit<R, TUnder> other)
    {
        static_assert(T::ratio);
        IGC_ASSERT_MESSAGE(other.m_value % T::ratio == 0,
            "Invalid assignment: resulting count is not a whole number");
        this->m_value = other.m_value;
        return *this;
    }

    /// Multiplication by a scalar from the right hand side.
    Unit<T, TUnder> operator * (TUnder multiplier) const
    {
        Unit<T, TUnder> res;
        res.m_value = this->m_value * multiplier;
        return res;
    }

    /// Increment operator increases the value of the unit by one.
    Unit<T, TUnder> operator ++()
    {
        this->m_value += T::ratio;
        return *this;
    }

    /// Increment operator increases the value of the unit by one.
    Unit<T, TUnder> operator ++(int)
    {
        Unit<T, TUnder> copy(*this);
        this->operator++();
        return copy;
    }

    /// Decrement operator decreases the value of the unit by one.
    Unit<T, TUnder> operator --()
    {
        this->m_value -= T::ratio;
        return *this;
    }

    /// Decrement operator decreases the value of the unit by one.
    Unit<T, TUnder> operator --(int)
    {
        Unit<T, TUnder> copy(*this);
        this->operator--();
        return copy;
    }

    /// Operator equals returns true when the values of both units are the same.
    template<typename R>
    bool operator ==(Unit<R, TUnder> other) const
    {
        return this->m_value == other.m_value;
    }

    template<typename R>
    bool operator !=(Unit<R, TUnder> other) const
    {
        return !(this->operator==(other));
    }

    template<typename R>
    bool operator <(Unit<R, TUnder> other) const
    {
        return this->m_value < other.m_value;
    }

    template<typename R>
    bool operator >(Unit<R, TUnder> other) const
    {
        return !(*this == other) && !(*this < other);
    }

    /// Returns the underlying value of the unit.
    TUnder Count() const
    {
        static_assert(T::ratio);

        if ( m_value % T::ratio == 0)
        {
            return m_value / T::ratio;
        }
        else
        {
            IGC_ASSERT_MESSAGE(0, "Corrupted value of a unit, should be a multiple of the ratio.");
            return 0;
        }
    }

    /// Addition operator creates a new value that corresponds to the sum of arguments.
    ///
    /// When adding two object of different units, we want to return a new
    /// object representing the sum that has the type of the smaller argument unit.
    /// For example, if we have
    /// Unit<ByteUnitDescriptor> a;
    /// Unit<WordUnit> b;
    /// then a + b and b + a should have type Unit<ByteUnitDescriptor> since only then
    /// all values can be represented when converting between units.
    /// To achieve this variability of the return type, enable_if is employed
    /// in the two template methods below. Thanks to SFINAE, one of them will be
    /// instantiated, precisely the one with the smaller return type.
    /// If (T::ratio < R::ratio) is true, type T is a smaller unit and we want it
    /// to be the return type. Otherwise, Unit<R> will be the return type.
    template <typename R>
    Unit< typename std::enable_if<(T::ratio < R::ratio), T>::type, TUnder>
        operator +(const Unit<R,TUnder> & other) const
    {
        return add(other);
    }

    /// Addition operator creates a new value that corresponds to the sum of arguments.
    ///
    /// This is the complementary variant that will get instantiated when
    /// unit size of R is larger than or equal to T.
    template <typename R>
    Unit< typename std::enable_if<!(T::ratio < R::ratio), R>::type, TUnder>
        operator +(const Unit<R,TUnder> & other) const
    {
        // Switch the order of arguments so that the larger is on rhs and call add.
        return other.add(*this);
    }

    /// Subtraction operator creates a new value that corresponds to the difference of arguments.
    ///
    /// This template will be instantiated when unit size of T is smaller than of R.
    template <typename R>
    Unit< typename std::enable_if<(T::ratio < R::ratio), T>::type, TUnder>
        operator -(const Unit<R,TUnder> & other) const
    {
        return subtract(other);
    }

    /// Subtraction operator creates a new value that corresponds to the difference of arguments.
    ///
    /// This is the complementary variant that will get instantiated when
    /// unit size of R is larger than or equal to T.
    template <typename R>
    Unit< typename std::enable_if<!(T::ratio < R::ratio), R>::type, TUnder>
        operator -(const Unit<R,TUnder> & other) const
    {
        // Switch the order of arguments so that the larger is on rhs and call subtract.
        return other.subtractFrom(*this);
    }


private:

    /// Implements addition of variables with different unit sizes.
    /// This method requires that the larger unit is the right-hand side argument
    /// and larger unit ratio is a multiple of the smaller unit's ratio.
    template<typename R>
    Unit<T, TUnder> add(const Unit<R, TUnder> & other) const
    {
        static_assert(T::ratio);
        static_assert(T::ratio <= R::ratio);
        static_assert(R::ratio % T::ratio == 0,
            "Size of the unit on the right hand side of operator + must be equal to"
            "or a multiple of the left-hand unit size.");

        IGC_ASSERT_MESSAGE(other.m_value % T::ratio == 0,
            "Invalid addition: resulting count is not a whole number");
        Unit<T, TUnder> res;
        res.m_value = this->m_value + other.m_value;
        return res;
    }

    /// Implements subtraction of variables with different unit sizes.
    /// This method requires that the larger unit is the right-hand side argument.
    /// Implements a-b as a call to a.subtract(b)
    template<typename R>
    Unit<T, TUnder> subtract(const Unit<R, TUnder> & other) const
    {
        static_assert(T::ratio);
        static_assert(T::ratio <= R::ratio);
        static_assert(R::ratio % T::ratio == 0,
            "Size of the unit on the right hand side of operator- must be equal to"
            "or a multiple of the left-hand unit size.");

        IGC_ASSERT_MESSAGE(other.m_value % T::ratio == 0,
            "Invalid subtraction: resulting count is not a whole number");
        Unit<T, TUnder> res;
        res.m_value = this->m_value - other.m_value;
        return res;
    }

    /// Implements subtraction of variables with different unit sizes.
    /// This method requires that the larger unit is the right-hand side argument.
    /// Implements a-b as a call to b.subtractMeFrom(a)
    template<typename R>
    Unit<T, TUnder> subtractFrom(const Unit<R, TUnder> & other) const
    {
        static_assert(T::ratio);
        static_assert(T::ratio <= R::ratio);
        static_assert(R::ratio % T::ratio == 0,
            "Size of the unit on the right hand side of operator- must be equal to"
            "or a multiple of the left-hand unit size.");

        IGC_ASSERT_MESSAGE(other.m_value % T::ratio == 0,
            "Invalid subtraction: resulting count is not a whole number");
        Unit<T, TUnder> res;
        res.m_value = other.m_value - this->m_value;
        return res;
    }

    template<typename R_, typename RUnder_, int TRatio_>
    friend class Unit;

    // Declarations of friendship for free function templates operating on units.
    template<typename TDst_, typename TSrcUnit>
    friend Unit<TDst_, typename TSrcUnit::myTUnder, TDst_::ratio> round_down(TSrcUnit other);

    template<typename TDst_, typename TSrcUnit>
    friend Unit<TDst_, typename TSrcUnit::myTUnder, TDst_::ratio> round_up(TSrcUnit other);

    /// Stores the value of the unit.
    /// It is already multiplied by unit ratio.
    TUnder    m_value;
};

/// Converts the unit from the smaller source unit to a larger destination unit.
/// If the smaller unit value is not an integral multiple of the larger unit size,
/// the value gets rounded down.
/// For example, if source unit = m, destination unit = km then
/// round_down 1700 m = 1 km
template<typename TDst, typename TSrcUnit>
Unit<TDst, typename TSrcUnit::myTUnder, TDst::ratio> round_down(TSrcUnit other)
{
    static_assert(TDst::ratio >= TSrcUnit::myT::ratio,
        "Invalid rounding down: destination unit size must be larger than the source unit size.");
    Unit<TDst, typename TSrcUnit::myTUnder> res;
    res.m_value = other.m_value - (other.m_value % TDst::ratio);
    return res;
}

/// Converts the unit from the smaller source unit to a larger destination unit.
/// If the smaller unit value is not an integral multiple of the larger unit size,
/// the value gets rounded up.
/// For example, if source unit = m, destination unit = km then
/// round_up 1700 m = 2 km
template<typename TDst, typename TSrcUnit>
Unit<TDst, typename TSrcUnit::myTUnder, TDst::ratio> round_up(TSrcUnit other)
{
    static_assert(TDst::ratio >= TSrcUnit::myT::ratio,
        "Invalid rounding up: destination unit size must be larger than the source unit size.");
    Unit<TDst, typename TSrcUnit::myTUnder> res;
    typename TSrcUnit::myTUnder remainder = other.m_value % TDst::ratio;
    res.m_value = (remainder == 0) ? other.m_value : other.m_value - remainder + TDst::ratio;
    return res;
}

// If you need printing out units for some reason, feel free to uncomment.
// But then you need to do #include <llvm/Support/raw_ostream.h>
//template<typename T, typename TUnder>
//llvm::raw_ostream & operator<<(llvm::raw_ostream &os, Unit<T, TUnder> const& u)
//{
//    TUnder count = u.count();
//    return os << count << " " << T::name(u.count() > 1);
//}
