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
#include "MessageInfo.hpp"

using namespace iga;

#include <sstream>


std::string iga::format(SendOp op)
{
// #define MK_CASE(X) case SendOp::X: return "SendOp::" #X
#define MK_CASE(X) case SendOp::X: return #X

    switch (op) {
    MK_CASE(LOAD);
    //
    MK_CASE(STORE);
    //
    MK_CASE(ATOMIC_LOAD);
    MK_CASE(ATOMIC_STORE);
    //
    MK_CASE(ATOMIC_AND);
    MK_CASE(ATOMIC_XOR);
    MK_CASE(ATOMIC_OR);
    //
    MK_CASE(ATOMIC_IINC);
    MK_CASE(ATOMIC_IDEC);
    MK_CASE(ATOMIC_IADD);
    MK_CASE(ATOMIC_ISUB);
    MK_CASE(ATOMIC_IRSUB);
    MK_CASE(ATOMIC_ICAS);
    //
    MK_CASE(ATOMIC_SMIN);
    MK_CASE(ATOMIC_SMAX);
    //
    MK_CASE(ATOMIC_UMIN);
    MK_CASE(ATOMIC_UMAX);
    //
    MK_CASE(ATOMIC_FADD);
    MK_CASE(ATOMIC_FSUB);
    MK_CASE(ATOMIC_FMIN);
    MK_CASE(ATOMIC_FMAX);
    MK_CASE(ATOMIC_FCAS);
    default:
        std::stringstream ss;
        ss << "0x" << std::hex << (int)op << "?";
        return ss.str();
    }
#undef MK_CASE
}

std::string iga::format(CacheOpt op)
{
// #define MK_CASE(X) case CacheOpt::X: return "CacheOpt::" #X
#define MK_CASE(X) case CacheOpt::X: return #X
    switch (op) {
    MK_CASE(DEFAULT);
    MK_CASE(READINVALIDATE);
    default:
        std::stringstream ss;
        ss << "0x" << std::hex << (int)op << "?";
        return ss.str();
    }
#undef MK_CASE
}

std::string iga::format(AddrType op)
{
// #define MK_CASE(X) case CacheOpt::X: return "CacheOpt::" #X
#define MK_CASE(X) case AddrType::X: return #X
    switch (op) {
    MK_CASE(FLAT);
    MK_CASE(BTI);
    default:
        std::stringstream ss;
        ss << "0x" << std::hex << (int)op << "?";
        return ss.str();
    }
#undef MK_CASE
}


static int getNumEnabledChannels(uint32_t chDisableBits)
{
    switch(chDisableBits & 0xF)
    {
    case 0x7:
    case 0xB:
    case 0xD:
    case 0xE: return 1;
    case 0x3:
    case 0x5:
    case 0x6:
    case 0x9:
    case 0xA:
    case 0xC: return 2;
    case 0x1:
    case 0x2:
    case 0x4:
    case 0x8: return 3;
    case 0x0: return 4;
    case 0xF: return 0;
    default:  break;
    }
    return 0;
}


iga::MessageInfo iga::MessageInfo::tryDecode(
    Platform platform,
    SFID sfid,
    uint32_t exDesc,
    uint32_t desc,
    std::string *error)
{
    std::string errDft;
    if (!error)
        error = &errDft;

    MessageInfo mi { }; // zero initialize
    mi.cachingL3 = CacheOpt::DEFAULT;

    // some constants to make some of the program more declarative
    const int ALWAYS_SIMD8 = 8;
    const int SLM_BTI = 0xFE;
    const int COHERENT_BTI = 0xFF;
    const int NONCOHERENT_BTI = 0xFD;

    bool decodeError = false;

    auto badOp =
        [&] (std::string why) {
            *error = why;
            decodeError = true;
            return mi;
        };
    auto badOp2 =
        [&] (std::string why1,std::string why2) {
            return badOp(why1+why2);
        };

    auto getDescBits = [&](int off, int len) {
            uint32_t mask = len == 32 ? 0xFFFFFFFF : ((1 << len) - 1);
            return (int)((desc >> off) & mask);
        };
    auto getDescBit = [&](int off) {
            return getDescBits(off,1) != 0;
        };

    auto decodeHwsBlocks = [&] (int off) {
            // 0 -> 1 block, 1 -> 2 blocks, 2, -> 4 blocks, ...
            return 1 << getDescBits(off,3);
        };
    auto decodeOwsBlocks = [&] (int off) {
            // MDC_DB_OW and MDC_64_DB_OW
            // 1L, 1H, 2, 4, 8, 16
            int bits = getDescBits(off,3);
            return bits <= 1 ? 1 : (1 << (bits-1));
        };
    auto decodeMDC_DS = [&] (int off) {
            int bits = getDescBits(off,2);
            if (bits == 3)
                badOp("invalid MDC_DS");
            return (1 << bits);
        };
    auto decodeMDC_DWS_DS = [&] (int off) {
            int bits = getDescBits(off,2);
            return (1 << bits);
        };
    auto decodeMDC_SM2 = [&] (int off) {
            int bits = getDescBits(off,1); // yeah SM2 is really 1 bit (2 means two values)
            return bits ? 16 : 8;
        };
    auto decodeMDC_SM2R = [&] (int off) {
            int bits = getDescBits(off,1);
            return bits ? 8 : 16;
        };
    auto decodeMDC_SM3 = [&] (int off) {
            // we assume SIMD4x2 doesn't exist anymore
            int bits = getDescBits(off,2);
            return bits == 1 ? 16 : 8;
        };

    // the most generic setter
    auto setScatterGatherOpX =
      [&] (std::string msgImpl,
            SendOp op,
            AddrType addrType,
            uint32_t surfaceId,
            CacheOpt l3,
            int addrSize,
            int bitsPerElemReg, int bitsPerElemMem,
            int elemsPerAddr, int simd, int extraAttrs = 0)
      {
          mi.messageImplementation = msgImpl;
          mi.op = op;
          mi.cachingL3 = l3;
          mi.addrType = addrType;
          mi.surface = surfaceId;
          if (!decodeError)
              mi.attributeSet |= MessageInfo::VALID;
          mi.attributeSet |= extraAttrs;
          mi.addrSizeBits = addrSize;
          mi.elemSizeBitsRegFile = bitsPerElemReg;
          mi.elemSizeBitsMemory = bitsPerElemMem;
          mi.elemsPerAddr = elemsPerAddr;
          mi.execWidth = simd;
      };
    auto setScatterGatherOp =
      [&] (std::string msgImpl,
          SendOp op,
          AddrType addrType,
          uint32_t surfaceId,
          int addrSize,
          int bitsPerElem,
          int elemsPerAddr,
          int simd,
          int extraAttrs = 0)
      {
          setScatterGatherOpX(
              msgImpl,
              op,
              addrType,
              surfaceId,
              CacheOpt::DEFAULT,
              addrSize,
              bitsPerElem, bitsPerElem,
              elemsPerAddr,
              simd,
              extraAttrs);
      };
    // allows different data sizes in mem and reg
    auto setHdcMessageX =
        [&] (std::string msgImpl,
            SendOp op,
            int addrSize,
            int bitsPerElemReg,
            int bitsPerElemMem,
            int elemsPerAddr,
            int simd,
            int extraAttrs)
        {
            auto bti = getDescBits(0,8);
            AddrType addrType = AddrType::BTI;
            uint32_t surfaceId = 0;
            if (addrSize == 32) {
                if (bti == SLM_BTI) {
                    addrType = AddrType::FLAT;
                    surfaceId = 0;
                    extraAttrs |= MessageInfo::SLM;
                } else if (bti == COHERENT_BTI || bti == NONCOHERENT_BTI) {
                    extraAttrs |= bti == COHERENT_BTI ?
                        MessageInfo::COHERENT : 0;
                    addrType = AddrType::FLAT;
                } else {
                    addrType = AddrType::BTI;
                    surfaceId = bti;
                }
            } else if (addrSize == 64) {
                addrType = AddrType::FLAT;
                if (bti == COHERENT_BTI)
                    extraAttrs |= MessageInfo::COHERENT;
                else if (bti != NONCOHERENT_BTI)
                    badOp2(msgImpl, " must have 0xFF or 0xFD BTI");
            }
            setScatterGatherOpX(
                msgImpl,
                op,
                addrType,
                surfaceId,
                CacheOpt::DEFAULT,
                addrSize,
                bitsPerElemReg, bitsPerElemMem,
                elemsPerAddr,
                simd,
                extraAttrs);
        };
    // data size is same in mem and reg (typical case)
    auto setHdcMessage =
        [&] (std::string msgImpl,
            SendOp op,
            int addrSize,
            int bitsPerElem,
            int elemsPerAddr,
            int execSize,
            int extraAttrs)
        {
            setHdcMessageX(
                msgImpl,
                op,
                addrSize,
                bitsPerElem, bitsPerElem,
                elemsPerAddr,
                execSize,
                extraAttrs);
        };
    auto setHdcOwBlock =
        [&] (std::string msgImpl,
            SendOp op,
            int addrSize,
            int extraAttrs)
        {
            auto owBits = getDescBits(8,3);
            // if there's only one OW, we pad up to 1 GRF (since it's LO or HI)
            // otherwise, we'll have 2, 4, ... all GRF multiples
            int regBlockSize = owBits < 2 ? 256 : 128;
            extraAttrs |= MessageInfo::TRANSPOSED;
            if (owBits == 1)
                extraAttrs |= MessageInfo::EXPAND_HIGH;
            setHdcMessageX(
                msgImpl,
                op,
                addrSize,
                regBlockSize, 128,
                decodeOwsBlocks(8),
                1, // SIMD
                extraAttrs);
        };
    auto setHdcHwBlock =
        [&] (std::string msgImpl,
            SendOp op,
            int addrSize,
            int blocksCountOffset,
            int extraAttrs)
        {
            extraAttrs |= MessageInfo::TRANSPOSED;
            setHdcMessageX(
                msgImpl,
                op,
                addrSize,
                256, 256,
                decodeHwsBlocks(blocksCountOffset),
                1, // SIMD
                extraAttrs);
        };

    auto setHdcUntypeSurfaceMessage =
        [&] (const char *msgImpl,
            bool isRead,
            int addrSizeBits,
            int extraAttrs)
        {
            extraAttrs |= MessageInfo::HAS_CHMASK;
            setHdcMessage(
                msgImpl,
                isRead ? SendOp::LOAD : SendOp::STORE,
                addrSizeBits,
                32,
                getNumEnabledChannels(getDescBits(8,4)),
                decodeMDC_SM3(12),
                extraAttrs);
        };
    auto setHdcFloatAtomicMessage =
        [&] (const char *msgImpl, int addrSize, int dataSize) {
            std::stringstream mss(msgImpl);
            SendOp op = SendOp::INVALID;
            switch (getDescBits(8,3)) {
            case 0x1: op = SendOp::ATOMIC_FMAX; break;
            case 0x2: op = SendOp::ATOMIC_FMIN; break;
            case 0x3: op = SendOp::ATOMIC_FCAS; break;
            default: badOp2(msgImpl, " (unknown float op)"); return;
            }
            int extraAttrs = MessageInfo::HAS_CHMASK;
            extraAttrs |= getDescBit(13) ? MessageInfo::ATOMIC_LOAD : 0;
            if (op != SendOp::INVALID) {
                setHdcMessage(
                    msgImpl,
                    op,
                    addrSize,
                    dataSize,
                    1,
                    decodeMDC_SM2R(12),
                    extraAttrs);
            }
        };
    auto setHdcIntAtomicMessage =
      [&] (const char *msgImpl, int addrSize, int dataSize, int simdSize) {
          SendOp op = SendOp::INVALID;
          switch (getDescBits(8,2)) {
          // again with case 0x0 they wedged in a 64b CAS as part of the
          // 32b message (note there's also a QW atomic message)
          case 0x0: op = SendOp::ATOMIC_ICAS; dataSize = 64; break;
          // The rest are 32b (or 16b)
          case 0x1: op = SendOp::ATOMIC_AND;   break;
          case 0x2: op = SendOp::ATOMIC_OR;    break;
          case 0x3: op = SendOp::ATOMIC_XOR;   break;
          case 0x4: op = SendOp::ATOMIC_STORE; break;
          case 0x7: op = SendOp::ATOMIC_IADD;  break;
          case 0x8: op = SendOp::ATOMIC_ISUB;  break; // x = x - y
          case 0x9: op = SendOp::ATOMIC_IRSUB; break; // x = y - x
          case 0xA: op = SendOp::ATOMIC_SMAX;  break;
          case 0xB: op = SendOp::ATOMIC_SMIN;  break;
          case 0xC: op = SendOp::ATOMIC_UMAX;  break;
          case 0xD: op = SendOp::ATOMIC_UMIN;  break;
          case 0xE: op = SendOp::ATOMIC_ICAS;  break;
          default: badOp2(msgImpl, " (unknown op)"); return;
          }
          int extraAttrs = MessageInfo::HAS_CHMASK;
          extraAttrs |= getDescBit(13) ? MessageInfo::ATOMIC_LOAD : 0;
          if (op != SendOp::INVALID) {
              setHdcMessage(
                msgImpl,
                op,
                addrSize,
                dataSize,
                1,
                simdSize,
                extraAttrs);
          }
      };


    switch (sfid) {
    case SFID::DCRO:
    {
        const int msgType = getDescBits(14,5);
        switch (msgType) {
        case 0x00: // constant           oword block read
        case 0x01: // constant unaligned oword block read
            setHdcOwBlock(
                msgType == 0x00 ?
                  "constant oword block read" :
                  "constant unaligned oword block read",
                SendOp::LOAD,
                32, // 32b address
                0);
            mi.cachingL3 = getDescBits(13,1) ?
                CacheOpt::READINVALIDATE : CacheOpt::DEFAULT;
            break;
        case 0x03: // constant dword gathering read
            setHdcMessage(
                "constant dword gathering read",
                SendOp::LOAD,
                32,
                32,
                decodeMDC_DWS_DS(10),
                decodeMDC_SM2(8),
                0);
            mi.cachingL3 = getDescBits(13,1) ?
                CacheOpt::READINVALIDATE : CacheOpt::DEFAULT;
            break;
        default:
            return badOp("unsupported constant op");
        }
        break;
    } // end CC
    //////////////////////////////////////////////////////////////
    case SFID::DC0:
    {
        const int msgType = getDescBits(14,5);
        if (getDescBit(18) == 0) {
            // non-scratch
            switch (msgType) {
            case 0x0: // oword block read
                setHdcOwBlock(
                    "oword block read",
                    SendOp::LOAD,
                    32, // all 32b addresses
                    0);
                break;
            case 0x1: // aligned hword/oword block read
                setHdcOwBlock(
                    "aligned oword block read",
                    SendOp::LOAD,
                    32, // all 32b addresses
                    0);
                break;
            case 0x8: // hword/oword block write
                setHdcOwBlock(
                    "oword block write",
                    SendOp::STORE,
                    32, // all 32b addresses
                    0);
                break;
            case 0x9: // hword aligned block write
                setHdcOwBlock(
                    "aligned oword block write",
                    SendOp::STORE,
                    32, // all 32b addresses
                    0);
                break;
            //
            case 0x2: // oword dual block read
            case 0xA: // oword dual block write
                return badOp("oword dual block read/write not supported");
            //
            case 0x3: // dword scattered read
            case 0xB: // dword scattered write
            {
                setHdcMessage(
                    msgType == 0x3 ?
                        "dword gathering read" : "dword scattered write",
                    msgType == 0x3 ? SendOp::LOAD : SendOp::STORE,
                    32,
                    32,
                    decodeMDC_DWS_DS(10),
                    decodeMDC_SM2(8),
                    0
                  );
                mi.cachingL3 = getDescBits(13,1) ?
                    CacheOpt::READINVALIDATE : CacheOpt::DEFAULT;
                break;
            }
            case 0x4: // byte scattered read
            case 0xC: // byte scattered write
            {
                // "byte" scattered always consumes a DW of GRF per channel,
                // but DWS_DS controls how many bytes are loaded per address
                // that might be 1, 2, 4 all packed into one DW.
                // So think of:
                //     DWS_DS == 0 (byte) as u8 zext to u32
                //     DWS_DS == 1 (word) as u16 zext to u32
                //     DWS_DS == 2 (dword) as u32 zext to u32
                setHdcMessageX(
                    msgType == 0x4 ?
                        "byte gathering read" : "byte scattered write",
                    msgType == 0x4 ? SendOp::LOAD : SendOp::STORE,
                    32, // 32b addrs
                    32, // each channel occupies a DW in the reg file
                    8*decodeMDC_DS(10), // in memory it can be 1, 2, or 4 bytes
                    1, // vector size always 1
                    decodeMDC_SM2(8),
                    0);
                mi.cachingL3 = getDescBits(13,1) ?
                    CacheOpt::READINVALIDATE : CacheOpt::DEFAULT;
                break;
            }
            case 0x7: // memory fence
                return badOp("memory fence is unsupported");
            default:
                return badOp("unsupported dc0 op");
            } // end switch legacy DC0
            break;
        } else {
            // scratch
            bool isRead = getDescBit(17);
            setHdcHwBlock(
                isRead ?
                    "scratch hword block read" : "scratch hword block write",
                isRead ? SendOp::LOAD : SendOp::STORE,
                32, // r0.5
                12, // [13:12] num HWs
                MessageInfo::SCRATCH);
            // scratch offset [11:0] (reg aligned)
            mi.immediateOffset = 32*getDescBits(0,12);
            break;
        }
        break;
    } // end case DC0
    //////////////////////////////////////////////////////////////
    case SFID::DC1:
    {
        const int msgType = getDescBits(14,5);
        switch (msgType)
        {
        case 0x1: // untyped surface read
        case 0x9: // untyped surface write
        {
            setHdcUntypeSurfaceMessage(
                msgType == 0x1 ?
                    "untyped surface read" : "untyped surface write",
                msgType == 0x1,
                32,
                0);
            break;
        }
        case 0x11: // a64 untype surface read
        case 0x19: // a64 untype surface write
        {
            setHdcUntypeSurfaceMessage(
                msgType == 0x1 ?
                    "a64 untyped surface read" : "a64 untyped surface write",
                msgType == 0x11,
                64, // 8B addrs
                0);
            break;
        }
        case 0x10: // a64 gathering read (byte/dw/qw)
        case 0x1A: // a64 scattered write (byte/dw/qw)
        {
            // 0 is byte-scattered, the others (DW/QW) are true SIMT
            int subType = getDescBits(8,2);
            if (subType == 0) {
                // c.f. handling above with non-A64 version of BSR
                setHdcMessageX(
                    msgType == 0x10 ?
                        "a64 byte gathering read" : "a64 byte scattered write",
                    msgType == 0x10 ? SendOp::LOAD : SendOp::STORE,
                    64, // A64
                    32, // widens to DW (similar to non-A64 version)
                    8*decodeMDC_DWS_DS(10), // bits from memory
                    1, //
                    decodeMDC_SM2(12),
                    0);
            } else {
                // unlike non-A64 version, this variant supports DW and QW
                // in the same message type, the MDC_A64_DS is treated as a
                // vector length
                setHdcMessage(
                    msgType == 0x10 ?
                        "a64 gathering read" : "a64 scattered write",
                    msgType == 0x10 ? SendOp::LOAD : SendOp::STORE,
                    64,
                    subType == 1 ? 32 : 64,
                    decodeMDC_DWS_DS(10), // true vector
                    decodeMDC_SM2(12),
                    0);
            }
            mi.cachingL3 = getDescBits(13,1) ?
                CacheOpt::READINVALIDATE : CacheOpt::DEFAULT;
            break;
        }
        case 0x14: // a64 (unaligned,hword|oword) block read
        case 0x15: // a64 (unaligned,hword|oword) block write
        {
            int subType = getDescBits(11,2);
            bool isHword = (subType == 3);
            bool isUnaligned = subType == 1;
            std::string msgImpl = "a64";
            if (isUnaligned)
                msgImpl += " unaligned";
            if (isHword) {
                msgImpl += " hword";
            } else {
                msgImpl += " oword";
            }
            msgImpl += " block";
            if (msgType == 0x14) {
                msgImpl += " read";
            } else {
                msgImpl += " write";
            }
            if (isHword) {
                setHdcHwBlock(
                    msgImpl,
                    msgType == 0x14 ? SendOp::LOAD : SendOp::STORE,
                    64, // 64b addr
                    8, // offset of HWs
                    0);
            } else {
                setHdcOwBlock(
                    msgImpl,
                    msgType == 0x14 ? SendOp::LOAD : SendOp::STORE,
                    64, // 64b addr
                    0);
            }
            mi.cachingL3 = getDescBits(13,1) ?
                CacheOpt::READINVALIDATE : CacheOpt::DEFAULT;
            break;
        }
        case 0x1D: // a64 fp32 atomic
        case 0x1B: // fp32 atomic
            setHdcFloatAtomicMessage(
                msgType == 0x1D ? "a64 float atomic" : "float atomic",
                msgType == 0x1D ? 64 : 32,
                32);
            break;
        case 0x12: { // a64 atomic int{32,64}
            // they repurpose the SIMD size as the data size and the message
            // is always SIMD8
            int simd = ALWAYS_SIMD8;
            setHdcIntAtomicMessage(
                "a64 untyped atomic int32",
                64,
                getDescBit(12) ? 64 : 32,
                simd);
            break;
        }
        case 0x02: // atomic int32
            setHdcIntAtomicMessage(
                "untyped atomic int32",
                32,
                32,
                decodeMDC_SM2R(12));
            break;
        default:
            return badOp("unsupported dc1 op");
        } // DC1 switch
        break;
    } // DC1
    //////////////////////////////////////////////////////////////
    // DC2 shouldn't be used after SKL
    // case SFID::DP_DC2:
    default:
        return badOp("unsupported sfid");
    }

    return mi;
}