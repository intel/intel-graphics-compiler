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
#include "Syntax.hpp"
#include "../../bits.hpp"

#include <algorithm>
#include <bitset>

using namespace iga;

const char *iga::SymbolFor(MAddrModel am)
{
    switch (am) {
    case MAddrModel::A32_CO:  return "a32c";
    case MAddrModel::A32_NC:  return "a32";
    case MAddrModel::A32_SO:  return "a32o";
    case MAddrModel::A64_CO:  return "a64c";
    case MAddrModel::A64_NC:  return "a64";
    case MAddrModel::A64_SO:  return "a64o";
    case MAddrModel::SLM:     return "slm";
    default:
        if ((am & MAddrModel::BTS_FAMILY) == MAddrModel::BTS_FAMILY) {
            return "surf";
        } else if ((am & MAddrModel::SCRATCH_FAMILY) == MAddrModel::SCRATCH_FAMILY) {
            return "scratch";
        } else {
            return nullptr;
        }
    }
}

void iga::Format(std::ostream &os, MAddrModel am)
{
    os << SymbolFor(am) << std::uppercase;
    if ((am & MAddrModel::BTS_FAMILY) == MAddrModel::BTS_FAMILY) {
        os << "[0x" << std::hex << (0xFF & (int)am) << "]";
    } else if ((am & MAddrModel::SCRATCH_FAMILY) == MAddrModel::SCRATCH_FAMILY) {
        os << "[0x" << std::hex << (0xFFF & (int)am) << "]";
    }
}


static const int DW_PER_GRF = 8;

bool iga::ComputeMessageAddressRegisters(
    const MFormat *f,
    int simdMode,
    uint32_t desc,
    int &addrRegs)
{
    int addrDws = (f->addrModel == MAddrModel::A64_FAMILY) ? 2 : 1;
    if (simdMode == 1) {
        addrRegs = 0; // block op (only a header)
    } else {
        addrRegs = (simdMode * addrDws) / DW_PER_GRF; // vector op
    }
    if (testBit(desc,29) && addrRegs > 1) {
        // FIXME: this doesn't work for sampler
        addrRegs /= 2;
    }
    return true;
}

static int getElemsPerAddrDEs(uint32_t desc)
{
    switch (getBits<uint32_t>(desc, 10, 2)) {
    case 0: return 1;
    case 1: return 2;
    case 2: return 4;
    default: return 8;
    }
}
static int getElemsPerAddrCMask(uint32_t desc)
{
    std::bitset<4> bs(0xF & ~getBits<uint32_t>(desc, 8, 4));
    return (int)bs.count();
}
static int getBlockHWsA64(uint32_t desc)
{
    auto bs = getBits<uint32_t>(desc, 8, 3);
    return (1 << (bs - 1));
}
static int getBlockHWsScratch(uint32_t desc)
{
    auto bs = getBits<uint32_t>(desc, 12, 2);
    return (1 << bs);
}
static int getBlockOWsRegs(uint32_t desc)
{
    auto bs = getBits<uint32_t>(desc, 8, 3);
    if (bs <= 2)
        return 1;
    return (1 << (bs/2));
}
static int getSimdSM3(uint32_t desc)
{
    auto bs = getBits<uint32_t>(desc, 12, 2);
    return bs == 1 ? 16 : 8; // SIMD16 is 1, SIMD8 is 0
}

static const int BYTES_PER_GRF = 32;

static bool computeMessageDataRegisters(
    const MFormat *f,
    int simdMode,
    uint32_t desc, // partial descriptor
    int &dataRegs)
{
    // TODO: figure some way of doing this without hardcoding
    // these enums and whatnot
    switch (f->type) {
    case MType::MF_GR_VEC_A64_D8:
    case MType::MF_SW_VEC_A64_D8:
    case MType::MF_GR_VEC_A32_D8:
    case MType::MF_SW_VEC_A32_D8:
    {
        // e.g. x1
        //   r0: 0...1...2...3...4...5...6...7...
        //   r1: 8...9...A...B...C...D...E...F...
        // only x8 expands to multiple registers
        //   r0: 00000000111111112222222233333333
        //   r1: 44444444555555556666666677777777
        //   r2: 88888888...
        //   r3: ...
        int bpa = std::max(getElemsPerAddrDEs(desc),4);
        dataRegs =  bpa * simdMode / BYTES_PER_GRF;
        return true;
    }
    case MType::MF_GR_VEC_A64_D32:
    case MType::MF_SW_VEC_A64_D32:
    case MType::MF_GR_VEC_A32_D32:
    case MType::MF_SW_VEC_A32_D32:
    case MType::MF_CGR_VEC_A32_D32:
        dataRegs = (4 * getElemsPerAddrDEs(desc) * simdMode)/BYTES_PER_GRF;
        return true;
    case MType::MF_GR_VEC_A64_D64:
    case MType::MF_SW_VEC_A64_D64:
        dataRegs = (8 * getElemsPerAddrDEs(desc) * simdMode)/BYTES_PER_GRF;
        return true;
    case MType::MF_BR_A64_D256:
    case MType::MF_BW_A64_D256:
        dataRegs = getBlockHWsA64(desc);
        return true;
    case MType::MF_BR_A64_D128:
    case MType::MF_BW_A64_D128:
    case MType::MF_BR_A32_D128:
    case MType::MF_BW_A32_D128:
    case MType::MF_ABR_A32_D128:
    case MType::MF_CBR_A32_D128:
    case MType::MF_CUBR_A32_D128:
        dataRegs = getBlockOWsRegs(desc);
        return true;
    case MType::MF_SBR_A32_D256:
    case MType::MF_SBW_A32_D256:
        dataRegs = getBlockHWsScratch(desc);
        return true;
    case MType::MF_USR_A64_D32:
    case MType::MF_USW_A64_D32:
    case MType::MF_USR_A32_D32:
    case MType::MF_USW_A32_D32:
        // e.g. ld.untyped.rgb (16) ...
        dataRegs = getSimdSM3(desc) *
            (4 * getElemsPerAddrCMask(desc))/BYTES_PER_GRF;
        return true;
    case MType::MF_TSR_A32_D32:
    case MType::MF_TSW_A32_D32:
        // 8 DWORDS(*4) elements SG8L or SG8H
        // times number of channels enabled (RGBA)
        dataRegs = 8 * 4 * getElemsPerAddrCMask(desc) / BYTES_PER_GRF;
        return true;
    default:
        break;
    }

    return false;
}

bool iga::ComputeMessageDataRegisters(
    const MFormat *f,
    int simdMode,
    uint32_t desc, // partial descriptor
    int &dataRegs)
{
    if (!computeMessageDataRegisters(f, simdMode, desc, dataRegs)) {
        return false;
    }
    if (testBit(desc,30) && dataRegs > 1) {
        // FIXME: only if data packing supported (e.g. sampler)
        // e.g. 4 => 2
        //      5 => 3
        dataRegs = (dataRegs + 1) / 2;
    }
    return true;
}

const MField *iga::FindHeaderField(const MFormat &mf)
{
    for (const MField &f : mf.fields) {
        if (f.type == nullptr) {
            break; // end of the field array
        }
        if (f.type->mapping == MFieldMapping::MFieldMappingHeader) {
            return &f;
        }
    }
    return nullptr;
}
