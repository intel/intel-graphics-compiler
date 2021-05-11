/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cassert>

#include "iga_types_swsb.hpp"

// SingleDistPipe encoding
const static uint32_t SWSB_FLAG_SBID_DST          = 0x20;
const static uint32_t SWSB_FLAG_SBID_SRC          = 0x30;
const static uint32_t SWSB_FLAG_SBID_SET          = 0x40;
const static uint32_t SWSB_FLAG_SBID_SET_REG_DIST = 0x80;
const static uint32_t SWSB_SHIFT_SBID_SET         = 4;
const static uint32_t SWSB_FOOTPRINT_SBID         = 0xF;
const static uint32_t SWSB_FOOTPRINT_DIST         = 0x7;

// ThreeDistPipe encoding
const static uint32_t SWSB_3DIST_FLAG_REG_DIST          = 0x00;
const static uint32_t SWSB_3DIST_FLAG_REG_DIST_ALL      = 0x08;
const static uint32_t SWSB_3DIST_FLAG_REG_DIST_FLOAT    = 0x10;
const static uint32_t SWSB_3DIST_FLAG_REG_DIST_INT      = 0x18;
const static uint32_t SWSB_3DIST_FLAG_SBID_DST          = 0x20;
const static uint32_t SWSB_3DIST_FLAG_SBID_SRC          = 0x30;
const static uint32_t SWSB_3DIST_FLAG_SBID_SET          = 0x40;
const static uint32_t SWSB_3DIST_FLAG_REG_DIST_LONG     = 0x50;
const static uint32_t SWSB_3DIST_FLAG_SBID_SET_REG_DIST = 0x80;

const static uint32_t SWSB_3DIST_SHIFT_SBID_SET = 4;
const static uint32_t SWSB_3DIST_FOOTPRINT_SBID = 0xF;
const static uint32_t SWSB_3DIST_FOOTPRINT_DIST = 0x7;



namespace iga {

    template<>
    SWSB_STATUS SWSB::decode<SWSB_ENCODE_MODE::SingleDistPipe>(
        uint32_t swsbBits, InstType instTy)
    {
        SWSB_STATUS stat = SWSB_STATUS::SUCCESS;
        if ((0xF0 & swsbBits) == 0) {
            // distance/nop; low 4b tell us the distance and 0 means nop
            minDist = swsbBits & 0xF;
            distType = minDist == 0 ? DistType::NO_DIST : DistType::REG_DIST;
            tokenType = TokenType::NOTOKEN;
        }
        else if ((SWSB_FLAG_SBID_SET_REG_DIST & swsbBits) != 0) {
            // either WaitDst+DepDist (in-order instruction)
            // or     WaitSet+DepDist (out-of-order instruction)
            if (instTy == InstType::UNKNOWN)
                return SWSB_STATUS::UNKNOWN_INST_TYPE;
            distType = DistType::REG_DIST;
            minDist = SWSB_FOOTPRINT_DIST & (swsbBits >> SWSB_SHIFT_SBID_SET);

            if (instTy == InstType::MATH || instTy == InstType::SEND)
                tokenType = TokenType::SET;
            else
                tokenType = TokenType::DST;
            sbid = swsbBits & SWSB_FOOTPRINT_SBID;
        }
        else {
            distType = DistType::NO_DIST;
            // one of the other cases
            switch (0xF0 & swsbBits) {

            case SWSB_FLAG_SBID_SET:
                tokenType = TokenType::SET;
                break;
            case SWSB_FLAG_SBID_SRC:
                tokenType = TokenType::SRC;
                break;
            case SWSB_FLAG_SBID_DST:
                tokenType = TokenType::DST;
                break;
            default:
                stat = SWSB_STATUS::ERROR_INVALID_SBID_VALUE;
                // rest are reserved
            }
            sbid = (SWSB_FOOTPRINT_SBID & swsbBits);
        }
        return stat;
    }

    template<>
    uint32_t SWSB::encode<SWSB_ENCODE_MODE::SingleDistPipe>(InstType) const
    {
        uint32_t swsb = 0; //all 0's means no dependency

        if (distType != DistType::NO_DIST && tokenType != TokenType::NOTOKEN) {
            swsb = sbid;
            swsb |= minDist << SWSB_SHIFT_SBID_SET;
            swsb |= SWSB_FLAG_SBID_SET_REG_DIST;
        }
        else if (distType != DistType::NO_DIST) { // Dist only
            assert(distType == DistType::REG_DIST);
            assert((minDist & (~SWSB_FOOTPRINT_DIST)) == 0);
            swsb = minDist;
        }
        else if (tokenType != TokenType::NOTOKEN) { // Token only
            swsb = sbid;
            switch (tokenType) {
            case TokenType::DST:
                swsb |= SWSB_FLAG_SBID_DST;
                break;
            case TokenType::SRC:
                swsb |= SWSB_FLAG_SBID_SRC;
                break;
            case TokenType::SET:
                swsb |= SWSB_FLAG_SBID_SET;
                break;
            default:
                assert(0);
                break;
            }
        }
        // otherwise swsb is 0
        return swsb;
    }

    template<>
    bool SWSB::verify<SWSB_ENCODE_MODE::SingleDistPipe>(InstType instTy) const
    {
        if (distType != DistType::NO_DIST && tokenType != TokenType::NOTOKEN) {
            switch (instTy) {
            case InstType::DPAS:
            case InstType::MATH:
            case InstType::SEND:
                if (distType == DistType::REG_DIST && tokenType == TokenType::SET)
                    return true;
                break;
            default:
                if (distType == DistType::REG_DIST && tokenType == TokenType::DST)
                    return true;
                break;
            }
            return false;
        }
        else if (distType != DistType::NO_DIST) { // Dist only
            if (distType == DistType::REG_DIST)
                return true;
            else
                return false;
        }
        // others, either token only or no swsb, are valid cases
        return true;
    }


    template<>
    SWSB_STATUS SWSB::decode<SWSB_ENCODE_MODE::ThreeDistPipe>(uint32_t swsbBits, InstType instTy)
    {
        SWSB_STATUS stat = SWSB_STATUS::SUCCESS;
        distType = DistType::NO_DIST;
        tokenType = TokenType::NOTOKEN;
        // distance
        if ((swsbBits & ~SWSB_3DIST_FOOTPRINT_DIST) == SWSB_3DIST_FLAG_REG_DIST) {
            minDist = (swsbBits & SWSB_3DIST_FOOTPRINT_DIST);
            distType = minDist == 0 ? DistType::NO_DIST : DistType::REG_DIST;
        }
        else if ((swsbBits & ~SWSB_3DIST_FOOTPRINT_DIST) == SWSB_3DIST_FLAG_REG_DIST_ALL) {
            minDist = (swsbBits & SWSB_3DIST_FOOTPRINT_DIST);
            distType = minDist == 0 ? DistType::NO_DIST : DistType::REG_DIST_ALL;
        }
        else if ((swsbBits & ~SWSB_3DIST_FOOTPRINT_DIST) == SWSB_3DIST_FLAG_REG_DIST_FLOAT) {
            minDist = (swsbBits & SWSB_3DIST_FOOTPRINT_DIST);
            distType = minDist == 0 ? DistType::NO_DIST : DistType::REG_DIST_FLOAT;
        }
        else if ((swsbBits & ~SWSB_3DIST_FOOTPRINT_DIST) == SWSB_3DIST_FLAG_REG_DIST_INT) {
            minDist = (swsbBits & SWSB_3DIST_FOOTPRINT_DIST);
            distType = minDist == 0 ? DistType::NO_DIST : DistType::REG_DIST_INT;
        }
        else if ((swsbBits & ~SWSB_3DIST_FOOTPRINT_DIST) == SWSB_3DIST_FLAG_REG_DIST_LONG) {
            minDist = (swsbBits & SWSB_3DIST_FOOTPRINT_DIST);
            distType = minDist == 0 ? DistType::NO_DIST : DistType::REG_DIST_LONG;
        }
        // dual mode: regDist + SBID
        else if ((SWSB_3DIST_FLAG_SBID_SET_REG_DIST & swsbBits) != 0) {
            // either WaitDst+DepDist (fixed)
            // or     WaitSet+DepDist (var)
            if (instTy == InstType::UNKNOWN)
                return SWSB_STATUS::UNKNOWN_INST_TYPE;
            switch(instTy) {
            case InstType::DPAS:
            case InstType::MATH:
                distType = DistType::REG_DIST;
                tokenType = TokenType::SET;
                break;
            case InstType::SEND:
                distType = DistType::REG_DIST_ALL;
                tokenType = TokenType::SET;
                break;
            default:
                distType = DistType::REG_DIST;
                tokenType = TokenType::DST;
                break;
            }
            sbid = (swsbBits & SWSB_3DIST_FOOTPRINT_SBID);
            minDist = (SWSB_3DIST_FOOTPRINT_DIST & (swsbBits >> SWSB_3DIST_SHIFT_SBID_SET));
        }
        // SWSB ID dst/src/set
        else {
            // one of the other cases
            switch (0xF0 & swsbBits) {
            case SWSB_3DIST_FLAG_SBID_SET:
                tokenType = TokenType::SET;
                break;
            case SWSB_3DIST_FLAG_SBID_SRC:
                tokenType = TokenType::SRC;
                break;
            case SWSB_3DIST_FLAG_SBID_DST:
                tokenType = TokenType::DST;
                break;
            default:
                stat = SWSB_STATUS::ERROR_INVALID_SBID_VALUE;
                // rest are reserved
            }
            sbid = (SWSB_3DIST_FOOTPRINT_SBID & swsbBits);
        }
        return stat;
    }

    template<>
    uint32_t SWSB::encode<SWSB_ENCODE_MODE::ThreeDistPipe>(InstType) const
    {
        uint32_t swsb = 0; //all 0's means no dependency

        if (distType != DistType::NO_DIST && tokenType != TokenType::NOTOKEN) {
            swsb = sbid;
            swsb |= (minDist << SWSB_SHIFT_SBID_SET);
            swsb |= SWSB_FLAG_SBID_SET_REG_DIST;
        } else if (distType != DistType::NO_DIST) { // Dist only
            assert((minDist & (~SWSB_3DIST_FOOTPRINT_DIST)) == 0);
            switch (distType)
            {
            case DistType::REG_DIST:
                swsb = SWSB_3DIST_FLAG_REG_DIST | minDist;
                break;
            case DistType::REG_DIST_ALL:
                swsb = SWSB_3DIST_FLAG_REG_DIST_ALL | minDist;
                break;
            case DistType::REG_DIST_FLOAT:
                swsb = SWSB_3DIST_FLAG_REG_DIST_FLOAT | minDist;
                break;
            case DistType::REG_DIST_INT:
                swsb = SWSB_3DIST_FLAG_REG_DIST_INT | minDist;
                break;
            case DistType::REG_DIST_LONG:
                swsb = SWSB_3DIST_FLAG_REG_DIST_LONG | minDist;
                break;
            default:
                assert(0);
                break;
            }
        } else if (tokenType != TokenType::NOTOKEN) { // Token only
            switch(tokenType) {
            case TokenType::DST:
                swsb = SWSB_3DIST_FLAG_SBID_DST | sbid;
                break;
            case TokenType::SRC:
                swsb = SWSB_3DIST_FLAG_SBID_SRC | sbid;
                break;
            case TokenType::SET:
                swsb = SWSB_3DIST_FLAG_SBID_SET | sbid;
                break;
            default:
                assert(0);
                break;
            }
        }
        return swsb;
    }
    template<>
    bool SWSB::verify<SWSB_ENCODE_MODE::ThreeDistPipe>(InstType instTy) const
    {
        if (distType != DistType::NO_DIST && tokenType != TokenType::NOTOKEN) {
            switch (instTy) {
            case InstType::DPAS:
            case InstType::MATH:
                if (distType == DistType::REG_DIST && tokenType == TokenType::SET)
                    return true;
                break;
            case InstType::SEND:
                if (distType == DistType::REG_DIST_ALL && tokenType == TokenType::SET)
                    return true;
                break;
            default:
                if (distType == DistType::REG_DIST && tokenType == TokenType::DST)
                    return true;
                break;
            }
            return false;
        } else if (distType != DistType::NO_DIST) { // Dist only
            switch (distType)
            {
            case DistType::REG_DIST:
            case DistType::REG_DIST_ALL:
            case DistType::REG_DIST_FLOAT:
            case DistType::REG_DIST_INT:
            case DistType::REG_DIST_LONG:
                return true;
            default:
                return false;
            }
        }
        // others, either token only or no swsb, are valid cases
        return true;
    }







    template<>
    SWSB_STATUS SWSB::decode<SWSB_ENCODE_MODE::SWSBInvalidMode>(uint32_t, InstType)
    {
        return SWSB_STATUS::ERROR_ENCODE_MODE;
    }

    template<SWSB_ENCODE_MODE M>
    uint32_t SWSB::encode(InstType instTy) const
    {
        return 0;
    }

    template<SWSB_ENCODE_MODE M>
    bool SWSB::verify(InstType instTy) const
    {
        return false;
    }
} // end namespace iga

using namespace iga;

/// createSWSB - decode swsb info to SWSB
SWSB_STATUS SWSB::decode(
    uint32_t swsbBits, SWSB_ENCODE_MODE enMode, InstType instTy)
{
    switch(enMode) {
    case SWSB_ENCODE_MODE::SingleDistPipe:
        return decode<SWSB_ENCODE_MODE::SingleDistPipe>(swsbBits, instTy);

    case SWSB_ENCODE_MODE::ThreeDistPipe:
        return decode<SWSB_ENCODE_MODE::ThreeDistPipe>(swsbBits, instTy);

    case SWSB_ENCODE_MODE::SWSBInvalidMode:
        return decode<SWSB_ENCODE_MODE::SWSBInvalidMode>(swsbBits, instTy);

    default:
        break;
    }
    return SWSB_STATUS::ERROR_ENCODE_MODE;
}

/// encode - encode swsb to bianry
uint32_t SWSB::encode(SWSB_ENCODE_MODE enMode, InstType instTy) const
{
    switch (enMode) {
    case SWSB_ENCODE_MODE::SingleDistPipe:
        return encode<SWSB_ENCODE_MODE::SingleDistPipe>(instTy);
    case SWSB_ENCODE_MODE::ThreeDistPipe:
        return encode<SWSB_ENCODE_MODE::ThreeDistPipe>(instTy);
    default:
        break;
    }
    return 0;
}

void SWSB::clear()
{
    distType = DistType::NO_DIST;
    tokenType = TokenType::NOTOKEN;
    minDist = 0;
    sbid = 0;
    spToken = SpecialToken::NONE;
}

bool SWSB::verifySpecialToken(SWSB_ENCODE_MODE enMode) const
{
    assert(hasSpecialToken());
    return true;
}

bool SWSB::verify(SWSB_ENCODE_MODE enMode, InstType instTy) const
{
    if (!hasSWSB())
        return true;

    if (hasSpecialToken())
        return verifySpecialToken(enMode);

    switch (enMode) {
    case SWSB_ENCODE_MODE::SingleDistPipe:
        return verify<SWSB_ENCODE_MODE::SingleDistPipe>(instTy);

    case SWSB_ENCODE_MODE::ThreeDistPipe:
        return verify<SWSB_ENCODE_MODE::ThreeDistPipe>(instTy);
    default:
        break;
    }
    return false;
}
