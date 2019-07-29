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
#include "MessageFormatting.hpp"
#include "Syntax.hpp"
#include "Tables.hpp"
#include "../Formatter.hpp"
#include "../IRToString.hpp"
#include "../../bits.hpp"

#include <sstream>
#include <string>

using namespace iga;

static const MFormat *findFormat(
    const Model &m,
    SFID sfid,
    uint32_t desc)
{
    auto matches = [&] (const MFormat &mf) {
        if (mf.platform > m.platform) {
            return false; // this message is too new
        }
        return
            sfid == mf.sfid &&
            (desc & mf.opcodeMask) == mf.opcodeValue;
    };
    size_t mLen = 0;
    const MFormat *mArr = GetMFormatTable(mLen);
    const MFormat *format = nullptr;
    for (size_t mIx = 0; mIx < mLen; mIx++) {
        if (matches(mArr[mIx])) {
            if (format == nullptr || mArr[mIx].platform >= format->platform) {
                // first match OR newer; matches(..) enforces >=
                //
                // debug builds ensure that only one message format matches
                // a given descriptor
                IGA_ASSERT(
                    format == nullptr || mArr[mIx].platform > format->platform,
                    "descriptor matches two or more formats");
                format = mArr + mIx;
#ifndef _DEBUG
                break; // release version bails at first match
#endif
            }
        }
    } // end table loop
    return format;
}

FormatResult iga::FormatLdStInstruction(
    const Model &m,
    const Instruction &i)
{
    FormatResult r;
    const OpSpec &os = i.getOpSpec();
    if (!os.isSendOrSendsFamily()) {
        return r.error("not a send");
    }

    auto exDescT = i.getExtMsgDescriptor();
    auto descT = i.getMsgDescriptor();
    if (exDescT.type == SendDescArg::REG32A ||
        descT.type == SendDescArg::REG32A)
    {
        return r.error("has reg descs");
    }
    if (i.getSource(0).getDirRegName() != RegName::GRF_R) {
        return r.error("src0 must be GRF");
    }
    uint32_t desc = descT.imm;
    uint32_t exDesc = exDescT.imm;

    // bool isGroupSend = os.groupOp == Op::SEND || os.groupOp == Op::SENDC;
    SFID sfid;
    if (os.groupOp != Op::SEND && os.groupOp != Op::SENDC) {
        // SFID is ExDesc[3:0]
        sfid = static_cast<SFID>(exDescT.imm & 0xF);
    } else {
        // SFID is the subop
        sfid = static_cast<SFID>(os.functionControlValue);
    }

    /////////////////////////////////////////////////
    // resolve message and encode descriptors
    const MFormat *format = findFormat(m, sfid, desc);
    if (format == nullptr) {
        return r.error("no mapping for descriptors");
    }

    // bool hdr = testBit(desc,19) != 0;
    int mlen = (int)getBits(desc, 25, 4);
    int xlen = (int)getBits(exDesc, 6, 4);
    int rlen = (int)getBits(desc, 20, 5);

    std::stringstream op;
    if (format->kind == MKind::LOAD) {
        if (i.getDestination().getDirRegName() != RegName::GRF_R) {
            return r.error("dst must be GRF for load");
        }
        op << "ld";
    } else if (format->kind == MKind::STORE) {
        if (i.getDestination().getDirRegName() != RegName::ARF_NULL) {
            return r.error("dst must be null for store");
        }
        op << "st";
    } else if (format->kind == MKind::ATOMIC) {
        op << "at";
    } else {
        return r.error("unsupported format kind (for now)");
    }

    bool isConditionalSend =
        os.op == Op::SENDC || os.groupOp == Op::SENDC ||
        os.op == Op::SENDSC || os.groupOp == Op::SENDSC;
    if (isConditionalSend) {
        op << "c";
    }
    bool isUnarySend = os.getSourceCount() == 1;
    if (isUnarySend) { // old-style unary send or sendc
        op << "p";
    }
    op << ".";
    op << format->mnemonic;
    // we have "ld.sc8" so far; append the arguments: e.g. ".x4"
    // we have something like "ld.typed" so far, add the arguments,
    // e.g. ".sg8l" and ".rgba"
    for (size_t aIx = 0;
        aIx < sizeof(format->argIndices)/sizeof(format->argIndices[0]);
        aIx++)
    {
        int ix = format->argIndices[aIx];
        if (ix == -1) { // end of the array
            break;
        }
        // extra the field value from the descriptor and format it
        op << ".";
        const MField &mf = format->fields[ix];
        uint32_t val = getBits(desc,mf.off,mf.type->length);
        mf.type->subopFormat(op, val);
    }
    r.opcode = op.str();

    int32_t addrModelArg = 0; // scratch[...] or surf[...]
    int32_t addrOffsetArg = 0; // [... + 0x320]
    int simdSize = 1;
    // run through the rest of the fields
    for (const MField &mf : format->fields) {
        if (mf.type == nullptr) {
            break; // end of the field array
        }
        uint32_t val = getBits(desc,mf.off,mf.type->length);
        if (mf.type->mapping == MFieldMapping::MFieldMappingInstOpt) {
            if (val) {
                r.instOpts.push_back(mf.type->instOptSymbol);
            } // 0 implies absense
        } else if (mf.type->mapping == MFieldMapping::MFieldMappingExecSize) {
            ExecSize es = ExecSize::INVALID;
            if (!mf.type->execSizeDecode(val,es)) {
                return r.error("error decoding SIMD size from descriptor");
            }
            ExecSize instExecSize = i.getExecSize();
            switch (instExecSize) {
            case ExecSize::SIMD1:
            case ExecSize::SIMD2:
            case ExecSize::SIMD4:
                // we emulate SIMD1, SIMD2, and SIMD4 vector scatter/gather
                // IO via SIMD8 and relying on the emask to strip stuff off
                instExecSize = ExecSize::SIMD8;
                break;
            default:
                break;
            }
            if (instExecSize != es) {
                return r.error("descriptor SIMD size mismatches instruction ExecSize");
            }
            simdSize = static_cast<int>(instExecSize);
            // otherwise, nothing else to do; the SIMD size will be implied
            // by the ExecSize during parse (see MessageParsing.cpp)
        } else if (mf.type->mapping == MFieldMapping::MFieldMappingAddrModel) {
            // scratch offset or surface index
            addrModelArg = (int32_t)val;
        } // else something else...
    } // for format->fields


    MAddrModel formatAddrModel = format->addrModel;
    if (formatAddrModel == MAddrModel::BTS_SLM_A32) {
        // The format might be something that includes multiple cases like
        // e.g. BTS_SLM_A32 needs to determine what it's dealing with
        if (isSLMBTI(addrModelArg)) {
            formatAddrModel = MAddrModel::SLM_FAMILY;
        } else if (isStatelessBTI(addrModelArg)) {
            formatAddrModel = MAddrModel::A32_FAMILY;
        } else {
            formatAddrModel = MAddrModel::BTS_FAMILY;
        }
    } // else Scratch or A64
    MAddrModel addrModel =
        static_cast<MAddrModel>(formatAddrModel | addrModelArg);
    if (addrModel == MAddrModel::A32_SO || addrModel == MAddrModel::A64_SO) {
        // this could be part of the format we define in the tables
        addrOffsetArg = (exDesc >> 16);
    }

    auto emitRegRange = [] (
        std::stringstream &ss,
        int reg,
        int len)
    {
        ss << "r" << std::dec << reg;
        if (len == 0) {
            ss << "-<ERR>";
        } else if (len != 1) {
            ss << "-" << (reg + len - 1);
        }
    };
    auto emitOptTypeSuffix = [] (
        std::stringstream &ss,
        Type t)
    {
        if (t != Type::UD && t != Type::INVALID) {
            ss << ToSyntax(t);
        }
    };

    const Operand &dst = i.getDestination();
    const Operand &src0 = i.getSource(0);
    const Operand &src1 = i.getSource(1);
    int numSrcs = i.getSourceCount();
    if (dst.getKind() != Operand::Kind::DIRECT ||
        src0.getKind() != Operand::Kind::DIRECT ||
        (numSrcs > 1 && src1.getKind() != Operand::Kind::DIRECT))
    {
        return r.error("an operand is indirect");
    }

    auto emitAddrOffsetOpt = [] (std::stringstream &ss, int32_t addOffset) {
        if (addOffset != 0) {
            ss <<  std::hex;
            if (addOffset > 0) {
                ss << "+0x" << addOffset;
            } else {
                ss << "-0x" << -addOffset;
            }
            ss << std::dec;
        }
    };

    // construct the destination operand
    std::stringstream dstSs;
    if (format->kind == MKind::LOAD) {
        // flat register range
        // e.g. something like "r12-13:f"
        emitRegRange(dstSs, dst.getDirRegRef().regNum, rlen);
        emitOptTypeSuffix(dstSs, dst.getType());
    } else if (format->kind == MKind::STORE) {
        // an address [r12-13]
        //
        // sends (8) null  addr  data
        //   <==>
        // st.... (8) [addr] data
        //
        // send (8) null  (addr:data)
        //   <==>
        // st... (8) [addr] (addr+mlen)
        if (dst.getDirRegName() != RegName::ARF_NULL)
            return r.error("store as send must have a null dst");
        Format(dstSs, addrModel);
        dstSs << '[';
        emitRegRange(dstSs, src0.getDirRegRef().regNum, mlen);
        emitAddrOffsetOpt(dstSs, addrOffsetArg);
        dstSs << ']';
    } else {
        IGA_ASSERT_FALSE("only LD and ST supported");
    }
    r.dst = dstSs.str();

    // construct the source operand
    std::stringstream srcSs;
    if (format->kind == MKind::LOAD) {
        // ld... (8)  dst  a64[r20,r41-44]  // with header and extra addrs
        //  => sends (8) ... dst r20 r41
        //
        // ld... (8)  dst   surf[0x4][r0] // e.g. block load, no extra addrs
        // ld... (8)  dst   scratch[0x100][r0] // e.g. scratch load, no extra addrs
        //  sends (8) ... dst r0 null
        //
        // ld... (8)  dst  a64[r21-24]  // no header
        //  sends (8) ... dst r21 null
        // or
        //  send  (8) ... dst r21
        //
        Format(srcSs, addrModel); // e.g. "a64c"
        srcSs << '[';
        emitRegRange(srcSs, (int)src0.getDirRegRef().regNum, mlen);
        if (src0.getDirRegName() != RegName::GRF_R) {
            return r.error("src0 is null (doesn't fit operand mapping)");
        }
        if (os.getSourceCount() == 2 &&
            xlen > 0 &&
            src1.getDirRegName() == RegName::GRF_R)
        {
            srcSs << ",";
            emitRegRange(srcSs, (int)src1.getDirRegRef().regNum, xlen);
        }
        emitAddrOffsetOpt(srcSs, addrOffsetArg);
        srcSs << ']';
        emitOptTypeSuffix(srcSs, src0.getType()); // e.g. :f
    } else if (format->kind == MKind::STORE) {
        int addrRegs = 1;
        if (numSrcs == 1 &&
            !ComputeMessageAddressRegisters(format, simdSize, desc, addrRegs))
        {
            return r.error("store needs binary send");
        } else if (numSrcs == 1 || src1.getDirRegName() == RegName::ARF_NULL) {
            return r.error("store needs binary send with non-null src1");
        }
        // TODO: verify that xlen is the full data payload return error if not
        // TODO: srcReg = numSrcs > 1 ?
        // USE ComputeMessageLengths(format,simdMode,numAddrRegs,numDataRegs)
        emitRegRange(srcSs, (int)src1.getDirRegRef().regNum, xlen);
        // type comes from dst since sources in binary send don't have types
        emitOptTypeSuffix(srcSs, dst.getType()); // e.g. :f
    } else {
        // a flat value
        IGA_ASSERT_FALSE("only LD and ST supported");
    }
    r.src = srcSs.str();

    return r;
}
