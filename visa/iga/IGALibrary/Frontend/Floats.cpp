/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
#include "Floats.hpp"
#include "../strings.hpp"

#include <cmath> // needed for android build!
#include <cstdlib>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

using namespace iga;

// Big Theory Statement (BTS): on NaN's in IGA
//
// NaN is an equivalence class of floating point values.  Different
// libraries, runtimes, and hardware use different elements of the
// class as the characteristic value when they want a NaN value.
// For instance, the sign bit can be anything and the mantissa payload
// bits can be anything except for the leading mantissa bit (which
// distinguishes a qnan from an snan).
//
// NOTE: we distinguish between 'qnan' and 'snan' instead of just using 'nan'
//       with the whole payload since we parse as 64b and only narrow once
//       we see the type; we considered 'nan' '(' HEXLIT ')' and just letting
//       the user sort the meaning of the mantissa out, but this gets messy
//       when widening literals during parse or narrowing them during
//       formatting.
//
// Without loss of generality (this generalizes to all IEEE sizes),
// the 32-bit IEEE 754 (C.f. section 6.2.1 of the spec) we have
//   s eeeeeeee mmm`mmmm`mmmm`mmmm`mmmm`mmmm
// (where the s and the m's can be anything such that at least one 'm' is
// non-zero; all 0's would imply infinity)
//
// SNAN: "snan" (signaling NaN) is:
//   s 11111111 0xx`xxxx`xxxx`xxxx`xxxx`xxxx
//              ^ leading bit of mantissa is 0; at least one bit
//                of the rest of the payload (x's must be non-zero)
//
// QNAN: IEEE754 says that qnan (quiet NaN) is
//   s 11111111 1xx`xxxx`xxxx`xxxx`xxxx`xxxx
//              ^ leading bit of mantiss is 1; the rest of the payload
//                can be zeros since the top mantissa bit is set
//
// IGA supports the following syntax (from examples)
//     NaNLit ::= '-'? ('snan'|'qnan') '(' HEXLIT ')'
//
// Examples:
//   * "-snan(0xA):f" parses as
//         s eeeeeeee mmm`mmmm`mmmm`mmmm`mmmm`mmmm
//         1 11111111 000`0000`0000`0000`0000`1010
//         ^ negative ^ snan                  ^^^^ 0xA payload
//           sign bit is respected
//
//   * "qnan(0x1B):f" parses as
//         s eeeeeeee mmm`mmmm`mmmm`mmmm`mmmm`mmmm
//         1 11111111 100`0000`0000`0000`0001`1011
//         ^ positive ^ qnan                ^^^^^^ 0x1B
//
//   * "snan(0x0):f" would be a parse error since the mantissa must not be 0
//         s eeeeeeee mmm`mmmm`mmmm`mmmm`mmmm`mmmm
//         1 11111111 000`0000`0000`0000`0000`0000
//         ^ positive ^ snan^^^^^^^^^^^^^^^^^^^^^^^ at least one bit must be 0
//
//   * "snan(0x00800000):f" illegal: payload value too large for bitfield
//         s eeeeeeee mmm`mmmm`mmmm`mmmm`mmmm`mmmm
//         1 11111111 000`0000`0000`0000`0000`0000
//                  ^ high bit of payload overflows to here
//
//   * "snan(0x00800000):df" payload fits since df has 53 bits
//         s eeeeeeeeeee m`mmmm`mmmm`mmmm`mmmm`mmmm`mmmm`mmmm`mmmm`mmmm`mmmm`mmmm`mmmm
//         1 11111111111 0`0000`0000`0000`0000`0000`0000`1000`0000`0000`0000`0000`0000
//                                                       ^ 0x800000
//
//   * "snan(0x400000):f" is legal, but just means qnan(0).  This is because
//      the parser parses the value as 64b and then converts it to 32b when it
//      sees the :f.  The payload fits while parsing, but conversion cannot
//      tell that we were dealing with qnan

template <typename F> int FloatMantissaBits();
template <typename F> int FloatExponentBits();
template <> int FloatMantissaBits<double>() {return 52;}
template <> int FloatExponentBits<double>() {return 11;}
template <> int FloatMantissaBits<float>() {return 23;}
template <> int FloatExponentBits<float>() {return 8;}
template <> int FloatMantissaBits<uint16_t>() {return 10;}
template <> int FloatExponentBits<uint16_t>() {return 5;}

// E.g. FloatBias<float>() == 127
//
template <typename F> static int FloatBias() {
    return (1 << (FloatExponentBits<F>() - 1)) - 1;
}

// FloatBiasDiff<double,float> => 1023 - 127
// FloatBiasDiff<float,half>   => 127 - 15 (0x70)
template <typename FBIG,typename FSML> static int FloatBiasDiff() {
    return FloatBias<FBIG>() - FloatBias<FSML>();
}

template <typename F, typename I> static
void FormatFloatImplNaN(std::ostream &os, I bits)
{
    const int MANT_LEN = FloatMantissaBits<F>();
    const int EXPN_LEN = FloatExponentBits<F>();
    const I SIGN_BIT = ((I)1 << (MANT_LEN + EXPN_LEN));
    if (bits & SIGN_BIT) {
        os << '-';
    }
    // split the payload into the quiet/signaling bit
    // (high bit of the mantissa) and the lower bits
    const I QNAN_BIT = (I)1 << (MANT_LEN - 1);
    if (bits & QNAN_BIT) {
        os << "qnan";
    } else {
        os << "snan";
    }
    os << "(";
    I lowerPayload = bits & (QNAN_BIT - 1); // lower bits of mantissa
    fmtHex(os, (uint64_t)lowerPayload);
    os << ")";
}

// Checks if a floating point number will reparse exactly as formatted.
//
// We parse floats as 64b literals and cast down.  So we use strtod for all
// floating point types and cast down.
template <typename T>
static bool willReparseExactly(T x, std::string str)
{
    // we always parse as a double since the parser will do the same
    double y = strtod(str.c_str(), nullptr);
    return ((T)y == x);
}

// Formats a float in decimal or scientific format if it will not lose
// precision during reparse
//
// Returns false if we were able to format it as something representable.
template <typename F, typename I>
static bool TryFormatFloatImplNonHex(std::ostream &os, F x)
{
    // first check special values (+-inf or nan)
    if (IS_INF(x)) {
        if (x < 0) {
            os << '-';
        }
        os << "inf";
        return true;
    } else if (IS_NAN(x)) {
        FormatFloatImplNaN<F,I>(os, iga::FloatToBits(x));
        return true;
    } else {
        // else it's not a special value (could be denorm)
        // we try and render it several different ways until we find something
        // that we can parse back bit-exact.
        // We try:
        //    - decimal
        //    - exponential
        //    - and then fall back on hex
        // static_assert(sizeof(F) == sizeof(I));

        // try as default, this lets STL pick the format.
        // it sometimes gives nice terse output
        // e.g. "3" for "3.0" instead of "3.0000000000..." (when possible)
        std::stringstream ss;
        ss.unsetf(std::ios_base::floatfield);
        ss << x;
        if (willReparseExactly(x, ss.str())) {
            auto str = ss.str();
            os << str;
            if (str.find('.') == std::string::npos &&
                str.find('e') == std::string::npos &&
                str.find('E') == std::string::npos)
            {
                // floats need a ".0" suffixing them if not in scientific form
                // STL default float sometimes drops the .
                //
                //  e.g. given "-0.0f", if we were to format that
                // as "-0:f" (which MSVCRT does), then it parses as
                // "0" since this is the negation the S64 value 0 (during parse)
                //
                // NOTE: we also have to ensure that we aren't using an
                // exponential (scientific) form already.
                //
                // e.g. 1e-007 should not convert to 1e-007.0
                os << ".0";
            }
            return true;
        }

        // try as scientific
        ss.str(std::string()); // reset
        ss << std::scientific << x;
        if (willReparseExactly(x, ss.str())) {
            os << ss.str();
            return true;
        }

        // TODO: IFDEF this given a new enough compiler
        // (must parse too)
        // e.g. 0x1.47ae147ae147bp-7 (need to parse first)
        //   NOTE: parsing should use >>
        //   float f;
        //   e.g. std::istringstream(""0x1P-1022") >> std::hexfloat >> f;

        // fallback to hex integral
        // FormatFloatImplHex<F,I>(os, x);
        return false;
    } // else
}

template <typename F>
static void FormatFloatAsHex(std::ostream &os, F f)
{
    fmtHex(os, (uint64_t)iga::FloatToBits(f));
}

void iga::FormatFloat(std::ostream &os, float x)
{
    if (!TryFormatFloatImplNonHex<float,uint32_t>(os, x)) {
        FormatFloatAsHex<float>(os, x);
    }
}

void iga::FormatFloat(std::ostream &os, double x)
{
    if (!TryFormatFloatImplNonHex<double,uint64_t>(os, x)) {
        FormatFloatAsHex<double>(os, x);
    }
}

void iga::FormatFloat(std::ostream &os, uint16_t w16)
{
#if 0
    fmtHex(os, (uint64_t)w16);
#else
    float f32 = ConvertHalfToFloat(w16);
    if (IS_NAN(f32)) {
        // So we get the correct payload size for NaNs
        FormatFloatImplNaN<uint16_t,uint16_t>(os, w16);
    } else if (ConvertFloatToHalf(f32) != w16 ||
        !TryFormatFloatImplNonHex<float,uint32_t>(os, f32))
    {
        FormatFloatAsHex<uint16_t>(os, w16);
    } // else: FormatFloatImplNonHex worked
#endif
}


void iga::FormatFloat(std::ostream &os, uint8_t x)
{
    FormatFloat(os, ConvertQuarterToFloatGEN(x));
}

uint32_t iga::ConvertDoubleToFloatBits(double f)
{
    uint64_t f64 = FloatToBits(f);

    uint64_t m64 = f64 & IGA_F64_MANT_MASK;
    uint64_t e64 = (f64 & IGA_F64_EXP_MASK) >> FloatMantissaBits<double>();
    if (e64 == (IGA_F64_EXP_MASK >> FloatMantissaBits<double>()) && m64 != 0) {
        // f64 NaN
        uint32_t m32 = (uint32_t)m64 & IGA_F32_MANT_MASK;
        m32 |= (uint32_t)((m64 & IGA_F64_QNAN_BIT) >>
            (FloatMantissaBits<double>() - FloatMantissaBits<float>())); // preserve snan
        if (m32 == 0) {
            // The payload was only in the high bits which we dropped;
            // make it non-zero so we retain NaN'ness
            m32 = 1;
        }
        uint32_t s32 = (uint32_t)(f64 >> 32) & IGA_F32_SIGN_BIT;
        return (s32 | IGA_F32_EXP_MASK | m32);
    } else {
        // regular conversion can deal with all the other special cases
        return FloatToBits((float)f);
    }
}

float iga::ConvertDoubleToFloat(double f)
{
    return FloatFromBits(ConvertDoubleToFloatBits(f));
}

// TODO: generalize this to all IEEE sizes once we're confident it works and
// the rounding behavior is good
uint16_t  iga::ConvertFloatToHalf(float f)
{
    uint32_t f32 = FloatToBits(f);

    uint32_t m32 = IGA_F32_MANT_MASK & f32;
    uint32_t e32 = (IGA_F32_EXP_MASK & f32) >> FloatMantissaBits<float>();

    uint32_t m16;
    uint32_t e16;

    if (e32 == (IGA_F32_EXP_MASK >> FloatMantissaBits<float>())) {
        // NaN or Infinity
        e16 = IGA_F16_EXP_MASK;
        m16 = (IGA_F16_MANT_MASK >> 1) & f32;
        if (m32 != 0) {
            // preserve the bottom 9 bits of the NaN payload and
            // shift the signaling bit (high bit) down as bit 10
            m16 |= (IGA_F32_QNAN_BIT & f32) >>
                (FloatMantissaBits<float>() - FloatMantissaBits<uint16_t>());
            // s eeeeeeee mmmmmmmmmmmmmmmmmmmmmm
            //            |            |||||||||
            //            |            vvvvvvvvv
            //            +---------->mmmmmmmmmm
            if (m16 == 0) {
                // if the nonzero payload is in the high bits and and gets
                // dropped and the signal bit is non-zero, then m16 is 0,
                // to maintain it as a qnan, we must set at least one bit
                m16 = 0x1;
            }
        }
    } else if (e32 > (127 - 15) + 0x1E) { // e16 overflows 5 bits after bias fix
        // Too large for f16 => infinity
        e16 = IGA_F16_EXP_MASK;
        m16 = 0;
    } else if (e32 <= (127 - 15) && e32 >= 0x66) {
        // Denorm/subnorm float
        //
        // Code taken from simulator ThreadContext::IE_F32TO16
        // Normal floats are:
        //   (1 + sum{m[i]^(23-i)*2^(-i)}) * 2^(e - bias)
        //   (each mantissa bit is a fractional power of 2)
        // Denorms are:
        //   (0 + ...)
        // This is a zero exponent, but non-zero mantissa
        //
        // set leading bit past leading mantissa bit (low exponent bit)
        // (hidden one)
        m32 |= (IGA_F32_QNAN_BIT << 1);
        // repeatedly increment the f32 exponent and divide the denorm
        // mantissa until the exponent reachs a non-zero value
        for (; e32 <= 127 - 15; m32 >>= 1, e32++)
            ;
        e16 = 0;
        m16 = m32 >> (FloatMantissaBits<float>() - FloatMantissaBits<uint16_t>());
    } else if (e32 < 0x66) {
        // Too small: rounds to +/-0.0
        e16 = 0;
        m16 = 0;
    } else {
        // Normalized float
        e16 = (e32 - (127 - 15)) << FloatMantissaBits<uint16_t>();
        m16 = m32 >> (FloatMantissaBits<float>() - FloatMantissaBits<uint16_t>());
        // TODO: rounding voodoo?
    }

    uint32_t s16 = (f32 >> 16) & IGA_F16_SIGN_BIT;

    return (uint16_t)(s16 | e16 | m16);
}


// GEN's 8-bit restricted float ("quarter float")
//   s eee mmmm (bias 2^(3-1) - 1 == 3)
//  s gfe dcba
// mantissa shifted to the top of the float
//     dcb`a000`0000`0000`0000`0000
// exponenent is unpacked by expanding
//    gfe to the following:
//
//    dgGGGGfe where G = ~g
// negative and positive 0 bypass this logic
//
// => NaN and infinities are illegal in this format
// => Denorms are not supported
//     QUOTE:
//      Specifically, when the exponent field is zero and the fraction
//      field is not zero, an implied one is still present instead of
//      taking a denormalized form (without an implied one).  This results
//      in a simple implementation but with a smaller dynamic range -
//      the magnitude of the smallest non-zero number is 0.1328125.
float iga::ConvertQuarterToFloatGEN(uint8_t u8)
{
    if (u8 == 0x00) {
        return 0.0f;
    } else if (u8 == 0x80) {
        return -0.0f;
    } else {
        uint32_t f32;
        f32 = ((uint32_t)u8 & 0x80) << (32 - 8); // d: sign
        f32 |= ((0x30 & (uint32_t)u8) << (23 - 4)); // fe: low bits of exp
        if ((0x40 & u8) == 0) { // g = 0
            // exp=011111fe
            f32 |= 0x1F << (23 + 2);
        } else {
            // exp=100000fe
            f32 |= 1ul << (23 + 7); // f32 high exp bit
        }
        f32 |= ((uint32_t)u8 & 0xF) << (23 - 4); // dcba: mantissa
        return FloatFromBits(f32);
    }
}

float iga::ConvertHalfToFloat(uint16_t u16)
{
    static const int MANTISSA_DIFFERENCE = // 23 - 10
        FloatMantissaBits<float>() - FloatMantissaBits<uint16_t>();

    uint32_t s32 = ((uint32_t)u16 & IGA_F16_SIGN_BIT) << 16;
    uint32_t e16 = (u16 & IGA_F16_EXP_MASK) >> FloatMantissaBits<uint16_t>();
    uint32_t m16 = u16 & IGA_F16_MANT_MASK;

    uint32_t m32, e32;
    if (e16 != 0 && e16 < 0x1F) {
        //  normal number
        e32 = e16 + FloatBiasDiff<float,uint16_t>(); // (127 - 15); // 0x70
        m32 = m16 << (23 - 10);
    } else if (e16 == 0 && m16 != 0) {
        // denorm/subnorm number (e16 == 0)
        // shift the mantissa left until the hidden one gets set
        for (e32 = (127 - 15 + 1);
            (m16 & (IGA_F16_MANT_MASK + 1)) == 0;
            m16 <<= 1, e32--)
            ;
        m32 = (m16 << MANTISSA_DIFFERENCE) & IGA_F32_MANT_MASK;
    } else if (e16 == 0) { // +/- 0.0
        e32 = 0;
        m32 = 0;
    } else {
        e32 = IGA_F16_EXP_MASK >> FloatMantissaBits<float>();
        if (m16 == 0) { // Infinity
            m32 = 0;
        } else { // NaN:  m16!=0 && e16==0x1F
            m32 = (u16 & IGA_F16_QNAN_BIT) << MANTISSA_DIFFERENCE; // preserve sNaN bit
            m32 |= (IGA_F16_MANT_MASK >> 1) & m16;
            if (m32 == 0) {
                m32 = 1; // ensure still NaN
            }
        }
    }

    return FloatFromBits(s32 | (e32 << 23) | m32);
}

double iga::ConvertFloatToDouble(float f)
{
    if (IS_NAN(f)) {
        uint32_t f32 = FloatToBits(f);

        uint64_t m64;
        m64 = (uint64_t)(f32 & IGA_F32_QNAN_BIT) <<
            (FloatMantissaBits<double>() - FloatMantissaBits<float>()); // keep the sNaN bit
        m64 |= (IGA_F32_MANT_MASK >> 1) & f32; // keep the non sNaN part
                                               // lower part of the payload
        uint64_t bits =
            (((uint64_t)f32 & IGA_F32_SIGN_BIT) << 32) | // sign
            IGA_F64_EXP_MASK |                           // exp
            m64;                                         // new mantissa
        return FloatFromBits(bits);
    } else {
        // not NaN: use default value
        return (double)f;
    }
}


