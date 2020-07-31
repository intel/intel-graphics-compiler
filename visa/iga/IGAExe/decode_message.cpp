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
#include "IR/Messages.hpp"
#include "Frontend/IRToString.hpp"
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
    std::ostream &os,
    int chIx,
    const iga::MessageInfo &mi)
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
    const Opts &,
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
    const Opts &,
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

static char toLower(char c) {return (char)std::tolower(c);}

bool decodeSendDescriptor(const Opts &opts)
{
    std::ofstream ofs(opts.outputFile);
    std::ostream &os = opts.outputFile.empty() ? std::cout : ofs;

    ensurePlatformIsSet(opts);

    if (opts.inputFiles.size() != 3 && opts.inputFiles.size() != 2) {
        fatalExitWithMessage("-Xdsd: expects: SFID ex_desc desc");
    }
    auto parseSendDescArg =
        [&] (const char *which, std::string inp) {
            if (inp.find("a0.") == 0) {
                inp = inp.substr(3);
                iga::RegRef a0rr;
                try {
                    a0rr.subRegNum = (uint16_t)std::stoul(inp, nullptr, 10);
                } catch (...) {
                    fatalExitWithMessage(
                        "-Xdsd: %s: %s: invalid a0 subregister",
                        which, inp.c_str());
                }
                return iga::SendDesc(a0rr);
            }
            uint32_t val = 0;
            try {
                int base = 10;
                if (inp.find("0x") == 0 || inp.find("0X") == 0) {
                    inp = inp.substr(2);
                    base = 16;
                }
                val = std::stoul(inp, nullptr, base);
            } catch (std::invalid_argument i) {
                fatalExitWithMessage(
                    "-Xdsd: %s: %s: parse error", which, inp.c_str());
            } catch (std::out_of_range i) {
                fatalExitWithMessage(
                    "-Xdsd: %s: %s: value out of range", which, inp.c_str());
            }
            return iga::SendDesc(val);
        };
    int descArgOff = opts.inputFiles.size() == 2 ? 0 : 1;
    auto exDesc = parseSendDescArg("ex_desc", opts.inputFiles[descArgOff]);
    auto desc = parseSendDescArg("desc", opts.inputFiles[descArgOff+1]);

    iga::SFID sfid = iga::SFID::INVALID;
    if (opts.inputFiles.size() == 3) {
        std::string sfidSym = opts.inputFiles[0];
        std::transform(
            sfidSym.begin(), sfidSym.end(), sfidSym.begin(), toLower);
        sfid = iga::FromSyntax<iga::SFID>(sfidSym);
        if (sfid == iga::SFID::INVALID) {
            fatalExitWithMessage(
                "-Xdsd: %s: invalid or unsupported SFID for this platform",
                opts.inputFiles[0].c_str());
        }
    } else if (opts.platform <= IGA_GEN11) {
        // decode it from ex_desc[3:0]
        if (exDesc.isImm())
            sfid = iga::sfidFromEncoding(
                static_cast<iga::Platform>(opts.platform),
                exDesc.imm & 0xF);
        if (sfid == iga::SFID::INVALID) {
            fatalExitWithMessage(
              "-Xdsd: 0x%x: invalid or unsupported SFID for this platform",
              exDesc);
        }
    } else {
        fatalExitWithMessage("-Xdsd: expected three arguments");
    }

    auto emitDiagnostics =
        [&](const iga::DiagnosticList &ds, bool errors) {
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
                if (errors)
                  emitRedText(std::cerr, ss.str());
                else
                  emitYellowText(std::cerr, ss.str());
            }
        };

    // TODO: allow an extra parameter for indDesc
    iga::DecodedDescFields decodedFields;
    const auto dr = iga::tryDecode(
        static_cast<iga::Platform>(opts.platform), sfid,
        exDesc, desc, iga::REGREF_INVALID,
        &decodedFields);
    emitDiagnostics(dr.warnings, false);
    emitDiagnostics(dr.errors, true);
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

    emitGreenText(os, dr.info.symbol);
    if (!dr.info.description.empty()) {
        os << " (";
        emitYellowText(os, dr.info.description);
        os << ")";
    }
    os << "\n";
    if (dr.syntax.isValid()) {
        os << dr.syntax.str("..","D","A","X") << "\n";
    }
    auto emitDesc = [&](iga::SendDesc sda) {
        std::stringstream ss;
        if (sda.isImm()) {
            ss << "0x" << std::hex << std::uppercase << sda.imm;
        } else {
            ss << "a0." << (int)sda.reg.subRegNum;
        }
        return ss.str();
    };
    if (dr) {
        os << "  Op:                         ";
        emitYellowText(os, format(dr.info.op));
        os << "\n";
        os << "  Address Type:               ";
        emitYellowText(os, format(dr.info.addrType));
        os << "\n";
        if (dr.info.addrType == iga::AddrType::FLAT) {
            if (dr.info.hasAttr(iga::MessageInfo::SLM)) {
                os << "  Surface:                    ";
                emitYellowText(os, "SLM");
                os << "\n";
            }
        } else if (dr.info.addrType == iga::AddrType::BTI) {
            os << "  Surface:                    ";
            std::stringstream ss;
            ss << "surface index " << emitDesc(dr.info.surfaceId);
            emitYellowText(os,ss.str());
            os << "\n";
        }
        os << "  Address Size:               ";
        emitYellowText(os, dr.info.addrSizeBits);
        os << "b (per channel)\n";
        os << "  Data Size";
        if (dr.info.elemSizeBitsRegFile != dr.info.elemSizeBitsMemory) {
            os << " (RegFile):        ";
        } else {
            os << ":                  ";
        }
        emitYellowText(os, dr.info.elemSizeBitsRegFile);
        os << "b (per channel)\n";
        if (dr.info.elemSizeBitsRegFile != dr.info.elemSizeBitsMemory) {
            os << "  Data Size (Memory):         ";
            emitYellowText(os, dr.info.elemSizeBitsMemory);
            os << "b (per channel)\n";
        }
        os << "  Data Elements Per Address:  ";
        emitYellowText(os, dr.info.elemsPerAddr);
        os << " element" << (dr.info.elemsPerAddr != 1 ? "s" : "");
        if (iga::SendOpHasCmask(dr.info.op)) {
            os << "  (";
            emitYellowText(os, ".");
            if (dr.info.channelsEnabled & 0x1)
              emitYellowText(os,"X");
            if (dr.info.channelsEnabled & 0x2)
              emitYellowText(os,"Y");
            if (dr.info.channelsEnabled & 0x4)
              emitYellowText(os,"Z");
            if (dr.info.channelsEnabled & 0x8)
              emitYellowText(os,"W");
            os << " enabled)";
        }
        os << "\n";
        //
        os << "  Execution Width:            ";
        emitYellowText(os, dr.info.execWidth);
        os << " channel" << (dr.info.execWidth != 1 ? "s" : "") << "\n";
        bool emitCacheSettings =
            dr.info.isLoad() || dr.info.isStore() || dr.info.isAtomic();
        emitCacheSettings |= dr.info.op == iga::SendOp::READ_STATE;
        if (emitCacheSettings) {
            auto emitCaching =
                [&] (const char *name, iga::CacheOpt co) {
                    os << "  " << name << " Caching:                 ";
                    emitYellowText(os, format(co));
                    if (opts.verbosity > 0) {
                        if (co == iga::CacheOpt::DEFAULT)
                            os << " (uses state \"MOCS\" settings)";
                        else
                            os << " (overrides state \"MOCS\" settings)";
                    }
                    os << "\n";
                };
            emitCaching("L1", dr.info.cachingL1);
            emitCaching("L3", dr.info.cachingL3);
        }
        os << "\n";
        os << "  Immediate Offset:           ";
        emitYellowText(os, emitDesc(dr.info.immediateOffset));
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
                if (dr.info.hasAttr(attr)) {
                    emitAttr(attrDesc);
                }
            };
        checkAttr(iga::MessageInfo::ATOMIC_RETURNS, "atomic returns result");
        checkAttr(iga::MessageInfo::HAS_CHMASK,  "uses channel mask");
        checkAttr(iga::MessageInfo::SCRATCH,     "scratch");
        checkAttr(iga::MessageInfo::SLM,         "slm");
        checkAttr(iga::MessageInfo::TRANSPOSED,  "transposed");
        checkAttr(iga::MessageInfo::TYPED,       "typed");

        bool isZeroRlen = desc.isImm() && ((desc.imm >> 20) & 0x1F) == 0;
        if (dr.info.isLoad() && isZeroRlen) {
            emitAttr("prefetch (no data read into GRF; "
                "instruction only prefetches to cache)");
        }
        os << "\n";

        // don't pretend to understand the sampler
        const auto &mi = dr.info;
        bool showDataPayload =
              mi.op != iga::SendOp::READ_STATE &&
              mi.op != iga::SendOp::LOAD_STATUS &&
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

    return !dr;
}