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
#include "iga_main.hpp"
#include "io.hpp"

// internal headers
#include "Backend/MessageInfo.hpp"
#include "ColoredIO.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>


static void emitSpan(std::ostream &os, char chr, int len) {
    for (int i = 0; i < len; i++) {
        if (chr != '-')
            emitYellowText(os, chr);
        else
            os << chr;
    }
}
static void emitDataElem(
    std::ostream &os, int chIx, const iga::MessageInfo &mi)
{
    static const char *CHANNELS = "0123456789ABCDEFGHIJKLMNOPQRSTUV";
    int zeroPadding =
        (mi.elemSizeBitsRegFile - mi.elemSizeBitsMemory)/8;
    if (mi.hasAttr(iga::MessageInfo::EXPAND_HIGH)) {
        emitSpan(os, CHANNELS[chIx], mi.elemSizeBitsMemory/8);
        emitSpan(os, '-',zeroPadding);
    } else {
        emitSpan(os, '-',zeroPadding);
        emitSpan(os, CHANNELS[chIx], mi.elemSizeBitsMemory/8);
    }
}
static void emitRegName(std::ostream &os, int grfNum) {
    os << "  reg[" << std::setw(2) << std::right << grfNum << "]";
}

static void formatSIMT(
    const Opts &opts,
    std::ostream &os,
    const iga::MessageInfo &mi,
    int grfSizeB)
{
    if (mi.elemSizeBitsRegFile == 0)
        return;
    int currGrf = 0;
    for (int vecIx = 0; vecIx < mi.elemsPerAddr; vecIx++) {
        int dataSize = mi.elemSizeBitsRegFile/8;
        int bytesPerVecElem = mi.execWidth*dataSize;
        int regsPerVecElem = std::max<int>(1,bytesPerVecElem/grfSizeB);
        // 32B, float (4B), SIMD16 ==> 8
        // 32B/4B == 8
        int chsPerGrf = std::min<int>(grfSizeB/dataSize,mi.execWidth);
        int chIxBase = 0;
        for (int regIx = 0; regIx < regsPerVecElem; regIx++) {
            // walk the channels in reverse order
            emitSpan(os, '-', grfSizeB - mi.execWidth*dataSize);
            for (int chIxR = chsPerGrf - 1; chIxR >= 0; chIxR--) {
                emitDataElem(os, chIxBase+chIxR, mi);
            }
            emitRegName(os, currGrf++);
            if (regIx == 0 && mi.elemsPerAddr > 1)
                os << " start of SIMT vector element [" << vecIx << "]";
            os << "\n";

            chIxBase += chsPerGrf;
        }
    }
}

static void formatSIMD(
    const Opts &opts,
    std::ostream &os,
    const iga::MessageInfo &mi,
    int grfSizeB)
{
    int currGrf = 0;
    int dataSize = mi.elemSizeBitsRegFile/8;
    int vecElemsPerReg = grfSizeB/dataSize;
    // outer loop: these should all be SIMD1, but the logical extension
    // exists and we can make this algorithm aware of that possible
    // extension
    for (int chIx = 0; chIx < mi.execWidth; chIx++) {
        int totalRegs = std::max<int>(1,mi.elemsPerAddr/vecElemsPerReg);
        int elemsLeft = mi.elemsPerAddr;
        int vecIx = 0;
        for (int regIx = 0; regIx < totalRegs; regIx++) {
            // at vecIx, walk in reverse order
            if (elemsLeft < vecElemsPerReg) {
                // pad out the last register
                // this can happen if the message payload doesn't fill
                // a register
                emitSpan(os, '-', (vecElemsPerReg - elemsLeft)*dataSize);
            }
            int elemsToPrint = std::min<int>(elemsLeft,vecElemsPerReg);
            for (int vi = elemsToPrint - 1; vi >= 0; --vi) {
                emitDataElem(os, chIx, mi);
            }
            emitRegName(os, currGrf++);
            if (vecElemsPerReg == 1) {
              os << " vector elem " << vecIx;
            } else {
              os << " vector elems " << vecIx << ".." <<
                vecIx + elemsToPrint - 1;
            }
            os << "\n";
            vecIx += vecElemsPerReg;
            elemsLeft -= vecElemsPerReg;
        }
    }
}

bool decodeSendDescriptor(const Opts &opts)
{
    std::ofstream ofs(opts.outputFile);
    std::ostream &os = opts.outputFile.empty() ? std::cout : ofs;

    ensurePlatformIsSet(opts);

    if (opts.inputFiles.size() != 3 && opts.inputFiles.size() != 2) {
        fatalExitWithMessage("-Xdsd: expects: ex_desc desc");
    }
    auto parseInt32 =
        [&] (const char *which, std::string inp) {
            uint32_t val = 0;
            try {
                int base = 10;
                if (inp.find("0x") == 0 || inp.find("0X") == 0) {
                    inp = inp.substr(2);
                    base = 16;
                }
                val = std::stoul(inp,nullptr,base);
            } catch (std::invalid_argument i) {
                fatalExitWithMessage(
                    "-Xdsd: %s: %s: parse error", which, inp.c_str());
            } catch (std::out_of_range i) {
                fatalExitWithMessage(
                    "-Xdsd: %s: %s: value out of range", which, inp.c_str());
            }
            return val;
        };
    int descArgOff = opts.inputFiles.size() == 2 ? 0 : 1;
    uint32_t ex_desc = parseInt32("ex_desc",opts.inputFiles[descArgOff]);
    uint32_t desc = parseInt32("desc",opts.inputFiles[descArgOff+1]);

    iga::SFID sfid = iga::SFID::INVALID;
    if (opts.inputFiles.size() == 3) {
        std::string sfidStr = opts.inputFiles[0];
        std::transform(
            sfidStr.begin(), sfidStr.end(), sfidStr.begin(), ::toupper);
        if (sfidStr == "DC0") {
            sfid = iga::SFID::DC0;
        } else if (sfidStr == "DC1") {
            sfid = iga::SFID::DC1;
        } else if (sfidStr == "DC2") {
            sfid = iga::SFID::DC2;
        } else if (sfidStr == "GTWY") {
            sfid = iga::SFID::GTWY;
        } else if (sfidStr == "RC") {
            sfid = iga::SFID::RC;
        } else if (sfidStr == "URB") {
            sfid = iga::SFID::URB;
        } else if (sfidStr == "TS") {
            sfid = iga::SFID::TS;
        } else if (sfidStr == "VME") {
            sfid = iga::SFID::VME;
        } else if (sfidStr == "DCRO") {
            sfid = iga::SFID::DCRO;
        } else if (sfidStr == "SMPL") {
            sfid = iga::SFID::SMPL;
        } else {
            fatalExitWithMessage(
                "-Xdsd: %s: invalid or unsupported SFID for this platform",
                opts.inputFiles[0].c_str());
        }
    } else {
        // decode it from ex_desc[3:0]
        sfid = iga::MessageInfo::sfidFromEncoding(
            static_cast<iga::Platform>(opts.platform), ex_desc & 0xF);
        if (sfid == iga::SFID::INVALID) {
            fatalExitWithMessage(
              "-Xdsd: 0x%x: invalid or unsupported SFID for this platform",
              ex_desc);
        }
    }

    iga::DiagnosticList es, ws;
    auto emitDiagnostics = [&](const iga::DiagnosticList &ds) {
        for (const auto &d : ds) {
            std::stringstream ss;
            if (d.first.len != 0) {
                int off = d.first.off;
                const char *which = "Desc";
                if (off >= 32) {
                    off -= 32;
                    which = "ExDesc";
                }
                ss << which << "[";
                if (d.first.len > 1) {
                    ss << off + d.first.len - 1 << ":";
                }
                ss << off << "]: ";
            }
            ss << d.second << "\n";
            if (&ds == &es)
              emitRedText(std::cerr,ss.str());
            else
              emitYellowText(std::cerr,ss.str());
        }
    };

    iga::DecodedDescFields decodedFields;
    auto mi = iga::MessageInfo::tryDecode(
        static_cast<iga::Platform>(opts.platform),
        sfid,
        ex_desc,
        desc,
        es,
        ws,
        &decodedFields);
    emitDiagnostics(ws);
    emitDiagnostics(es);
    if (!decodedFields.empty()) {
        os << "DESCRIPTOR FIELDS:\n";
        for (const auto &df : decodedFields) {
            std::stringstream ss;
            const auto &f = std::get<0>(df);
            const auto &val = std::get<1>(df);
            const auto &meaning = std::get<2>(df);
            int off = f.offset;
            ss << "  ";
            if (off >= 32) {
                off -= 32;
                ss << "ExDesc";
            } else {
                ss << "Desc";
            }
            ss << "[";
            if (f.length > 1) {
                ss << std::dec << off + f.length - 1 << ":";
            }
            ss << off;
            ss << "]";
            while (ss.tellp() < 16) {
                ss << ' ';
            }
            ss << "  " << std::setw(32) << std::left << f.name;
            ss << "  ";
            std::stringstream ssh;
            int fw = (f.length + 4 - 1)/4;
            ssh << "0x" << std::hex << std::uppercase <<  std::setw(fw) <<
                std::setfill('0') << val;
            ss << std::setw(12) << std::right << ssh.str();
            if (!meaning.empty())
                ss << "  " << meaning;
            ss << "\n";

            os << ss.str();
        }
        os << "\n";
    }

    emitGreenText(os,mi.symbol);
    if (!mi.description.empty()) {
        os << " (";
        emitYellowText(os,mi.description);
        os << ")";
    }
    os << "\n";
    if (mi)
    {
        os << "  Op:                         ";
        emitYellowText(os, format(mi.op));
        os << "\n";
        os << "  Address Type:               ";
        emitYellowText(os, format(mi.addrType));
        os << "\n";
        if (mi.addrType == iga::AddrType::FLAT) {
            if (mi.hasAttr(iga::MessageInfo::SLM)) {
                os << "  Surface:                    ";
                emitYellowText(os, "SLM");
                os << "\n";
            }
        } else if (mi.addrType == iga::AddrType::BTI) {
            os << "  Surface:                    ";
            std::stringstream ss;
            ss << "surface index " << mi.surface;
            emitYellowText(os,ss.str());
            os << "\n";
        }
        os << "  Address Size:               ";
        emitYellowText(os, mi.addrSizeBits);
        os << "b (per channel)\n";
        os << "  Data Size";
        if (mi.elemSizeBitsRegFile != mi.elemSizeBitsMemory) {
            os << " (RegFile):        ";
        } else {
            os << ":                  ";
        }
        emitYellowText(os, mi.elemSizeBitsRegFile);
        os << "b (per channel)\n";
        if (mi.elemSizeBitsRegFile != mi.elemSizeBitsMemory) {
            os << "  Data Size (Memory):         ";
            emitYellowText(os, mi.elemSizeBitsMemory);
            os << "b (per channel)\n";
        }
        os << "  Data Elements Per Address:  ";
        emitYellowText(os, mi.elemsPerAddr);
        os << " element" << (mi.elemsPerAddr != 1 ? "s" : "");
        if (mi.op == iga::SendOp::LOAD_QUAD ||
            mi.op == iga::SendOp::STORE_QUAD)
        {
            os << "  (";
            emitYellowText(os, ".");
            if (mi.channelsEnabled & 0x1)
              emitYellowText(os,"X");
            if (mi.channelsEnabled & 0x2)
              emitYellowText(os,"Y");
            if (mi.channelsEnabled & 0x4)
              emitYellowText(os,"Z");
            if (mi.channelsEnabled & 0x8)
              emitYellowText(os,"W");
            os << " enabled)";
        }
        os << "\n";
        //
        os << "  Execution Width:            ";
        emitYellowText(os,mi.execWidth);
        os << " channel" << (mi.execWidth != 1 ? "s" : "") << "\n";
        bool emitCacheSettings = mi.isLoad() || mi.isStore() || mi.isAtomic();
        if (emitCacheSettings) {
            auto emitCaching =
                [&] (const char *name, iga::CacheOpt co) {
                    os << "  " << name << " Caching:                 ";
                    emitYellowText(os,format(co));
                    if (opts.verbosity > 0) {
                        if (co == iga::CacheOpt::DEFAULT)
                            os << " (uses state \"MOCS\" settings)";
                        else
                            os << " (overrides state \"MOCS\" settings)";
                    }
                    os << "\n";
                };
            emitCaching("L1", mi.cachingL1);
            emitCaching("L3", mi.cachingL3);
        }
        os << "\n";
        os << "  Immediate Offset:           ";
        emitYellowText(os,mi.immediateOffset);
        os << "\n";
        os << "\n";
        os << "  Attributes:\n";
        auto emitAttr =
            [&] (const char *attrDesc) {
                os << "    - ";
                emitYellowText(os,attrDesc);
                os << "\n";
            };
        auto checkAttr =
            [&] (int attr, const char *attrDesc) {
                if (mi.hasAttr(attr)) {
                    emitAttr(attrDesc);
                }
            };
        checkAttr(iga::MessageInfo::ATOMIC_RETURNS, "atomic returns result");
        checkAttr(iga::MessageInfo::COHERENT,
            "coherent access (for stateless)");
        checkAttr(iga::MessageInfo::HAS_CHMASK,  "uses channel mask");
        checkAttr(iga::MessageInfo::SCRATCH,     "scratch");
        checkAttr(iga::MessageInfo::SLM,         "slm");
        checkAttr(iga::MessageInfo::TRANSPOSED,  "transposed");
        checkAttr(iga::MessageInfo::TYPED,       "typed");
        if (mi.isLoad() && ((desc >> 20) & 0x1F) == 0) {
            emitAttr("prefetch (no data read into GRF; "
                "instruction only prefetches to cache)");
        }
        os << "\n";

        // don't pretend to understand the sampler
        bool showDataPayload =
              mi.op != iga::SendOp::SAMPLER_LOAD &&
              mi.op != iga::SendOp::RENDER_READ &&
              mi.op != iga::SendOp::RENDER_WRITE &&
              mi.op != iga::SendOp::FENCE;
        if (showDataPayload) {
            os << "DATA PAYLOAD\n";
            int grfSize = 32;
            if (mi.hasAttr(iga::MessageInfo::TRANSPOSED)) {
                // formatSIMD(opts, os, msgInfo, grfSize);
                formatSIMD(opts, os, mi, grfSize);
            } else {
                formatSIMT(opts, os, mi, grfSize);
            }
            if (opts.verbosity > 0) {
                os <<
                  "\n"
                  "   legend:\n"
                  "     * 0, 1, ... A, ... V  indicates the SIMD channel (base 32)\n"
                  "       each character represents one byte in the register file\n"
                  "     * '-' means undefined or zero\n";
            }
        }
    }

    return !mi;
}