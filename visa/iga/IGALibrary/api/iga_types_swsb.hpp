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
#ifndef _IGA_TYPES_SWSB_HPP
#define _IGA_TYPES_SWSB_HPP

#include <stdint.h>

namespace iga
{
    enum class SWSB_STATUS
    {
        SUCCESS,
        ERROR_SET_ON_VARIABLE_LENGTH_ONLY,
        ERROR_INVALID_SBID_VALUE,
        ERROR_ENCODE_MODE,
        UNKNOWN_INST_TYPE,
        ERROR_DECODE,
        NONE
    };

    enum SWSB_ENCODE_MODE : uint8_t
    {
        SWSBInvalidMode = 0,
        SingleDistPipe  = 1,     // 1 distance pipe  : TGL
    };

    struct SWSB
    {
    public:
        enum DistType {
            NO_DIST,
            REG_DIST,
        };

        enum TokenType {
            NOTOKEN,
            SET,
            SRC,
            DST
        };

        enum InstType {
            UNKNOWN,
            MATH,
            SEND,
            OTHERS
        };

    public:
        DistType  distType;
        TokenType tokenType;
        uint32_t  minDist; // distance to nearest register dependency
        uint32_t  sbid;    // swsb id. the barrier identifier (sbid) applies to all wait's

        SWSB()
            : distType(DistType::NO_DIST), tokenType(TokenType::NOTOKEN), minDist(0), sbid(0) {}
        SWSB(DistType dt, TokenType tt, uint32_t dis, uint32_t id)
            : distType(dt), tokenType(tt), minDist(dis), sbid(id) {}

        bool operator==(const SWSB& rhs) const
        {
            return distType == rhs.distType && tokenType == rhs.tokenType &&
                sbid == rhs.sbid && minDist == rhs.minDist;
        }

        bool operator!=(const SWSB& rhs) const
        {
            return !(*this == rhs);
        }

        bool hasSWSB() const
        {
            return (distType != DistType::NO_DIST) || (tokenType != TokenType::NOTOKEN);
        }

        bool hasBothDistAndToken() const
        {
            return (distType != DistType::NO_DIST) && (tokenType != TokenType::NOTOKEN);
        }

        bool hasDist() const
        {
            return distType != DistType::NO_DIST;
        }

        bool hasToken() const
        {
            return tokenType != TokenType::NOTOKEN;
        }


    private:
        template<SWSB_ENCODE_MODE M>
        SWSB_STATUS decode(uint32_t swsbBits, InstType instTy);

        template<SWSB_ENCODE_MODE M>
        uint32_t getSWSBBinary(InstType instTy) const;

        template<SWSB_ENCODE_MODE M>
        bool verify(InstType instTy) const;

    public:
        /// createSWSB - decode swsb info to SWSB
        SWSB_STATUS decode(
            uint32_t swsbBits, SWSB_ENCODE_MODE enMode, InstType instTy);

        /// getSWSBBinary - encode swsb to bianry
        uint32_t getSWSBBinary(SWSB_ENCODE_MODE enMode, InstType instTy) const;

        /// verify - verify if the SWSB is in the correct combination according to given
        /// instruction type and encoding mode
        bool verify(SWSB_ENCODE_MODE enMode, InstType instTy) const;
    };
} // namespace iga
#endif //#ifndef _IGA_TYPES_SWSB_HPP
