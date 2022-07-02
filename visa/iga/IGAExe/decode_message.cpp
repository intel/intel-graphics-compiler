/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
#include <sstream>
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
        (mi.elemSizeBitsRegFile - mi.elemSizeBitsMemory) / 8;
    if (mi.hasAttr(iga::MessageInfo::Attr::EXPAND_HIGH)) {
        emitSpan(os, CHANNELS[chIx], mi.elemSizeBitsMemory / 8);
        emitSpan(os, '-', zeroPadding);
    } else {
        emitSpan(os, '-', zeroPadding);
        emitSpan(os, CHANNELS[chIx], mi.elemSizeBitsMemory / 8);
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
        int regsPerVecElem = std::max<int>(1, bytesPerVecElem/grfSizeB);
        // 32B, float (4B), SIMD16 ==> 8
        // 32B/4B == 8
        int chsPerGrf = std::min<int>(grfSizeB/dataSize, mi.execWidth);
        int chIxBase = 0;
        for (int regIx = 0; regIx < regsPerVecElem; regIx++) {
            // walk the channels in reverse order
            emitSpan(os, '-', grfSizeB - mi.execWidth*dataSize);
            for (int chIxR = chsPerGrf - 1; chIxR >= 0; chIxR--) {
                emitDataElem(os, chIxBase + chIxR, mi);
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
        int totalRegs = std::max<int>(1, mi.elemsPerAddr/vecElemsPerReg);
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
            int elemsToPrint = std::min<int>(elemsLeft, vecElemsPerReg);
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

static bool tryParseInt(const char *which, std::string inp, uint64_t &val) {
    val = 0;
    try {
        int base = 10;
        if (inp.find("0x") == 0 || inp.find("0X") == 0) {
            inp = inp.substr(2);
            base = 16;
        }
        val = std::stoull(inp, nullptr, base);
        return true;
    } catch (std::invalid_argument i) {
        return false;
    } catch (std::out_of_range i) {
        fatalExitWithMessage(
            "-Xdsd: ", which, ": ", inp, ": value out of range");
    }
}
static uint64_t parseInt(const char *which, std::string inp) {
     uint64_t val = 0;
    if (!tryParseInt(which, inp, val)) {
        fatalExitWithMessage(
            "-Xdsd: ", which, ": ", inp, ": malformed value");
    }
    return val;
}

static iga::SFID tryParseSFID(std::string sfidSym)
{
    std::transform(
        sfidSym.begin(), sfidSym.end(), sfidSym.begin(), toLower);
    return iga::FromSyntax<iga::SFID>(sfidSym);
}
static iga::ExecSize tryParseExecSize(std::string str)
{
    if (str.empty() ||
        str.substr(0, 1) != "(" ||
        str.substr(str.size() - 1) != ")")
    {
        return iga::ExecSize::INVALID;
    }
    size_t off = 1;
    while (off < str.size() && std::isspace(str[off]))
        off++;
    size_t len = 0;
    int execSizeInt = 0;
    while (off + len < str.size() && std::isdigit(str[off + len])) {
        execSizeInt = 10 * execSizeInt + str[off + len] - '0';
        len++;
    }
    while (off + len < str.size() && std::isspace(str[off + len])) {
        len++;
    }
    if (len == 0 || off + len == str.size() || (str[off + len] != ')' && str[off + len] != '|'))
        return iga::ExecSize::INVALID;
    switch (execSizeInt) {
    case 1: return iga::ExecSize::SIMD1;
    case 2: return iga::ExecSize::SIMD2;
    case 4: return iga::ExecSize::SIMD4;
    case 8: return iga::ExecSize::SIMD8;
    case 16: return iga::ExecSize::SIMD16;
    case 32: return iga::ExecSize::SIMD32;
    default: return iga::ExecSize::INVALID;
    }
}

// returns ExDesc[...] or Desc[...] based on platform and offset and length
static std::string fmtDescField(iga::Platform platform, int off, int len)
{
    const char *which = "Desc";
    if (off >= 32) {
        off -= 32;
        which = "ExDesc";
    }
    std::stringstream ss;
    ss << which;
    ss << "[";
    if (len > 1) {
        ss << std::dec << off + len - 1 << ":";
    }
    ss << off;
    ss << "]";
    return ss.str();
}


static void emitDecodeOutput(
    const Opts &opts,
    std::ostream &os,
    iga::Platform p,
    const iga::DecodeResult &dr,
    const iga::DecodedDescFields &decodedFields)
{
    auto emitDiagnostics =
        [&](const iga::DiagnosticList &ds, bool errors) {
            for (const auto &d : ds) {
                std::stringstream ss;
                if (d.first.len != 0) {
                    ss << fmtDescField(
                        p, d.first.off, d.first.len) << ": ";
                }
                ss << d.second << "\n";
                if (errors)
                  emitRedText(std::cerr, ss.str());
                else
                  emitYellowText(std::cerr, ss.str());
            }
        };
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
            bool hasExDesc = true;
            if (hasExDesc && off >= 32) {
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
    auto emitDesc = [&](iga::SendDesc sda, bool treatSigned = false) {
        std::stringstream ss;
        if (sda.isImm()) {
            if (treatSigned && (int32_t)sda.imm < 0) {
                ss << "-0x" << std::hex << std::uppercase << -(int32_t)sda.imm;
            } else {
                ss << "0x" << std::hex << std::uppercase << sda.imm;
            }
        } else {
            ss << "a0." << (int)sda.reg.subRegNum;
        }
        return ss.str();
    };
    if (dr) {
        os << "\n";
        os << "  Op:                         ";
        emitYellowText(os, ToSyntax(dr.info.op));
        os << "\n";
        if (dr.info.addrType != iga::AddrType::INVALID) {
            os << "  Address Type:               ";
            emitYellowText(os, ToSymbol(dr.info.addrType));
            os << "\n";
        }
        if (dr.info.addrType == iga::AddrType::FLAT) {
            if (dr.info.hasAttr(iga::MessageInfo::Attr::SLM)) {
                os << "  Surface:                    ";
                emitYellowText(os, "SLM");
                os << "\n";
            }
        } else if (dr.info.addrType == iga::AddrType::BTI) {
            os << "  Surface:                    ";
            std::stringstream ss;
            ss << "surface binding table index " <<
                emitDesc(dr.info.surfaceId);
            emitYellowText(os,ss.str());
            os << "\n";
        }
        else if (
            dr.info.addrType == iga::AddrType::SS ||
            dr.info.addrType == iga::AddrType::BSS)
        {
            os << "  Surface:                    ";
            std::stringstream ss;
            ss << "surface object " << emitDesc(dr.info.surfaceId);
            emitYellowText(os, ss.str());
            os << "\n";
        }

        bool showAddrSize = dr.info.addrSizeBits != 0;
        if (showAddrSize) {
            os << "  Address Size:                   ";
            emitYellowText(os, dr.info.addrSizeBits);
            os << "b (per channel)\n";
        }

        if (dr.info.elemSizeBitsRegFile != dr.info.elemSizeBitsMemory ||
            dr.info.elemSizeBitsMemory != 0)
        {
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
                os << "b";
                if (!dr.info.isTransposed() && !dr.info.isBlock()) {
                    os << " (per channel)\n";
                }
                os << "\n";
            }
        }
        if (dr.info.elemsPerAddr != 0) {
            os << "  Data Elements Per Address:  ";
            emitYellowText(os, dr.info.elemsPerAddr);
            os << " element" << (dr.info.elemsPerAddr != 1 ? "s" : "");
        }
        const iga::SendOpDefinition &opInfo = iga::lookupSendOp(dr.info.op);
        if (opInfo.hasChMask()) {
            os << "  (";
            emitYellowText(os, ".");
            if (dr.info.channelsEnabled & 0x1)
              emitYellowText(os, "X");
            if (dr.info.channelsEnabled & 0x2)
              emitYellowText(os, "Y");
            if (dr.info.channelsEnabled & 0x4)
              emitYellowText(os, "Z");
            if (dr.info.channelsEnabled & 0x8)
              emitYellowText(os, "W");
            os << " enabled)";
        }
        os << "\n";
        //
        if (dr.info.execWidth > 0) {
            os << "  Execution Width:            ";
            emitYellowText(os, dr.info.execWidth);
            os << " channel" << (dr.info.execWidth != 1 ? "s" : "") << "\n";
        }
        //
        bool emitCacheSettings =
            dr.info.isLoad() || dr.info.isStore() || dr.info.isAtomic();
        emitCacheSettings |= dr.info.op == iga::SendOp::READ_STATE;
        if (emitCacheSettings) {
            auto emitCaching =
                [&] (const char *name, iga::CacheOpt co) {
                    os << "  " << name << " Caching:                 ";
                    emitYellowText(os, ToSymbol(co));
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
        if (dr.info.immediateOffset != 0) {
            os << "\n";
            os << "  Immediate Offset:           ";
            emitYellowText(os, emitDesc(dr.info.immediateOffset, true));
            os << " (in bytes)\n";
        }
        if (unsigned(dr.info.attributeSet) &
            ~unsigned(iga::MessageInfo::Attr::VALID))
        {
            os << "\n";
            os << "  Attributes:\n";
            auto emitAttr =
                [&] (const char *attrDesc) {
                    os << "    - ";
                    emitYellowText(os,attrDesc);
                    os << "\n";
                };
            auto checkAttr =
                [&] (iga::MessageInfo::Attr attr, const char *attrDesc) {
                    if (dr.info.hasAttr(attr)) {
                        emitAttr(attrDesc);
                    }
                };
            checkAttr(iga::MessageInfo::Attr::ATOMIC_RETURNS,
                "atomic returns result");
            checkAttr(iga::MessageInfo::Attr::HAS_CHMASK, "uses channel mask");
            checkAttr(iga::MessageInfo::Attr::SCRATCH, "scratch");
            checkAttr(iga::MessageInfo::Attr::SLM, "slm");
            checkAttr(iga::MessageInfo::Attr::TRANSPOSED, "transposed");
            checkAttr(iga::MessageInfo::Attr::TYPED, "typed");
        } // attrs


        // don't pretend to understand the sampler
        const auto &mi = dr.info;
        bool showDataPayload =
            mi.execWidth > 0 &&
            !mi.isSample() &&
            !mi.isOther() &&
            mi.op != iga::SendOp::READ_STATE &&
            mi.op != iga::SendOp::LOAD_STATUS &&
            mi.op != iga::SendOp::RENDER_READ &&
            mi.op != iga::SendOp::RENDER_WRITE &&
            mi.op != iga::SendOp::FENCE &&
            mi.op != iga::SendOp::CCS_PC &&
            mi.op != iga::SendOp::CCS_PU &&
            mi.op != iga::SendOp::CCS_SC &&
            mi.op != iga::SendOp::CCS_SU;
        if (showDataPayload) {
            os << "\n";
            os << "DATA PAYLOAD\n";
            int grfSize = opts.platform >= IGA_XE_HPC ? 64 : 32;
            if (mi.hasAttr(iga::MessageInfo::Attr::TRANSPOSED)) {
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
        } // showDataPayload


        if (dr.info.execWidth == 0) {
            os << "\n";
            os << "WARNING: include ExecSize as argument; e.g. "
                " -Xdsd  ugm \"(32)\" ...\n";
        }
    } // dr valid
}


bool decodeSendDescriptor(const Opts &opts)
{
    std::ofstream ofs(opts.outputFile);
    std::ostream &os = opts.outputFile.empty() ? std::cout : ofs;

    ensurePlatformIsSet(opts);

    const iga::Platform p = iga::Platform(opts.platform);

    size_t minArgs = opts.platform < IGA_XE ? 2 : 3;
    size_t maxArgs = minArgs + 1;

    if (opts.inputFiles.size() < minArgs &&
        opts.inputFiles.size() > maxArgs)
    {
        if (minArgs == 2)
            fatalExitWithMessage("-Xdsd: for this platform expects: "
                "ExecSize? ExDesc Desc");
        else if (minArgs == 3)
            fatalExitWithMessage("-Xdsd: for this platform expects: "
                "SFID ExecSize? ExDesc Desc");
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
                        "-Xdsd: ", which, ": ", inp,
                        ": invalid a0 subregister");
                }
                return iga::SendDesc(a0rr);
            }
            uint32_t val = (uint32_t)parseInt(which, inp);
            return iga::SendDesc(val);
        };


    int argOff = 0;
    if (argOff >= (int)opts.inputFiles.size()) {
        fatalExitWithMessage(
            "-Xdsd: expects arguments (see -h=Xdsd)");
    }

    iga::SFID sfid = tryParseSFID(opts.inputFiles[argOff]);
    if (sfid != iga::SFID::INVALID) {
        argOff++;
        if (opts.platform < IGA_XE)
            fatalExitWithMessage(
                "-Xdsd: ", opts.inputFiles[argOff],
                ": SFID is encoded in ExDesc[3:0] for this platform");
    } else if (sfid == iga::SFID::INVALID && opts.platform >= IGA_XE) {
        fatalExitWithMessage(
            "-Xdsd: ", opts.inputFiles[argOff],
            ": invalid SFID for this platform");
    }
    // Formats are:
    //   <=GEN11:      ExecSize? ExDesc  Desc
    //   >=XE:    SFID ExecSize? ExDesc  Desc
    //
    // If ExecSize is not given, then we deduce it from the platform.

    // ExecSize is '(' INT ('|' ('M0' | 'M4' | ...))? ')'
    // e.g. all the following produce ExecSize::SIMD8:
    //   "(8)", "( 8 )", "(8|M24)"
    //
    iga::ExecSize execSize = iga::ExecSize::INVALID;

    if (argOff >= (int)opts.inputFiles.size()) {
        fatalExitWithMessage("-Xdsd: expects ExecSize? ... Desc");
    }

    // ExecSize? (optional SIMD argument)
    execSize = tryParseExecSize(opts.inputFiles[argOff]);
    if (execSize != iga::ExecSize::INVALID)
        argOff++;


    if (argOff >= (int)opts.inputFiles.size()) {
        fatalExitWithMessage(
            "-Xdsd: expects ExDesc Desc");
    }
    std::string exDescStr = opts.inputFiles[argOff];

    // ExDesc
    const iga::SendDesc exDesc =
        parseSendDescArg("ExDesc", exDescStr);
    if (opts.platform < IGA_XE && sfid == iga::SFID::INVALID) {
        // decode it from ex_desc[3:0]
        if (exDesc.isImm())
            sfid = iga::sfidFromEncoding(
                static_cast<iga::Platform>(opts.platform),
                exDesc.imm & 0xF);
        if (sfid == iga::SFID::INVALID) {
            std::stringstream ss;
            ss << "0x" << std::hex << std::uppercase << (exDesc.imm & 0xF);
            fatalExitWithMessage(
                "-Xdsd: ", ss.str(),
                ": invalid or unsupported SFID for this platform");
        }
    }
    auto desc = parseSendDescArg("Desc", opts.inputFiles[argOff + 1]);

    if (execSize == iga::ExecSize::INVALID) {
        // Normally tryDecode() gets the ExecSize from IR or decoded from the
        // descriptor; since we don't have that here, cheat and guess with a
        // sort of pre-decode scam.  It's not ideal, but I can't think of
        // anything better right now.
        execSize =
            p >= iga::Platform::XE_HPC ?
                iga::ExecSize::SIMD32 : iga::ExecSize::SIMD16;
        if (sfid == iga::SFID::TGM) {
        // typed LSC messages default to half the SIMD size
            execSize =
            p >= iga::Platform::XE_HPC ?
                iga::ExecSize::SIMD16 : iga::ExecSize::SIMD8;
        }
    }
    //
    iga::DecodedDescFields decodedFields;
    const auto dr = iga::tryDecode(
        p, sfid, execSize,
        exDesc, desc,
        &decodedFields);
    emitDecodeOutput(opts, os, p, dr, decodedFields);

    return !dr;
}
