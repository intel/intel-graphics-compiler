/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "MessageDecoderE64.hpp"

using namespace iga;

#define LSC_LOOKUP_TABLE(FIELD, OFF, LEN, TBL)                                 \
  lookupFromTable(FIELD, OFF, LEN, TBL, sizeof(TBL) / sizeof(TBL[0]))

struct MessageDecoderSamplerE64 : MessageDecoderE64 {
  MessageDecoderSamplerE64(Platform _platform, SFID _sfid, ExecSize _execSize,
                           int _src0Len, int _src1Len, uint64_t _desc,
                           RegRef id0, RegRef id1, DecodeResult &_result)
      : MessageDecoderE64(_platform, _sfid, _execSize, _src0Len, _src1Len,
                          _desc, id0, id1, _result) {}

  void decode();

private:
  std::string decodeChannels(bool);
}; // MessageDecoderSamplerE64

void MessageDecoderSamplerE64::decode() {
  addDocs(
    DocRef::MSGOP, "SAMPLER_MESSAGE_TYPE", "64985",
    DocRef::DESC, "MESSAGE_DESCRIPTOR_SAMPLING_ENGINE_Efficient64bit", "57021",
    DocRef::SRC0, "SAMPLER_MSG_HEADER_64bit_Eff", "57024");

  // Desc[5:0] SamplerMessageType
  const auto enc = getDescBits(0, 6);
  const SamplerMessageDescription smpOpInfo =
      LookupSamplerMessageDescription(enc, 32, false);
  addField("SamplerMessageType", 0, 6, enc, smpOpInfo.mnemonic);

  const auto &opInfo = lookupSendOp(smpOpInfo.op);
  result.info.op = smpOpInfo.op;
  result.info.symbol = opInfo.mnemonic;
  result.info.addrType = AddrType::SURF;
  result.info.execWidth = execSize == ExecSize::INVALID ? 32 : int(execSize);
  //
  result.info.surfaceState = indDesc0;
  result.info.samplerState = indDesc1;
  if (!opInfo.isValid()) {
    error(0, 6, "invalid SamplerMessageType");
    return;
  }

  // description string
  std::stringstream dss;
  dss << opInfo.mnemonic;
  //
  // Desc[6] EnableLSCBacking
  decodeField("EnableLscBacking", 6, 1,
    [&] (uint32_t val, std::stringstream &ss) {
      ss << (val == 0 ? "Disabled" : "Enabled");
    });
  //
  // Desc[10:7] varies based on sampler message type
  bool gather4 = opInfo.hasAttr(SendOpDefinition::Attr::GATHER4);
  dss << decodeChannels(gather4);
  //
  // Desc[13:11] SURFACE_TYPE_HINT
  dss << decodeSurfaceTypeHint();

  // Desc[14] AddressInputFormat
  decodeField("AddressInputFormat", 14, 1,
    [&] (uint32_t val, std::stringstream &ss) {
      result.info.addrSizeBits = (val == 0 ? 32 : 16);
      ss << (val == 0 ? "AIF32" : "AIF16");
      if (result.info.addrSizeBits == 16) {
        dss << ".a16";
      }
    });
  // Desc[15] DataReturnFormat
  decodeField("DataReturnFormat", 15, 1,
    [&] (uint32_t val, std::stringstream &ss) {
      result.info.elemSizeBitsRegFile = (val == 0 ? 32 : 16);
      result.info.elemSizeBitsMemory = result.info.elemSizeBitsRegFile;
      ss << (val == 0 ? "DRF32" : "DRF16");
      if (result.info.elemSizeBitsRegFile == 16) {
        dss << ".d16";
      }
    });

  decodeCacheControlValue(opInfo.op);

  auto trtt = getDescBits(20, 1);
  if (trtt) {
    result.info.description += " w/ trtt";
    dss << ".trtt";
  }
  addField("TRTT_NULL", 20, 1, trtt,
           trtt ? "include status register" : "no status return");
  //
  auto feedback = getDescBits(21, 1);
  if (feedback) {
    dss << ".feedback";
  }
  addField("IncludeFeedbackSurface", 21, 1, feedback,
           feedback ? "Feedback surface included in header"
                    : "No feedback header");

  decodeSmsIdx();
  if (result.info.samplerStateIndex > 0 && result.info.samplerStateIndex <= 6)
    dss << ".smsidx=" << result.info.samplerStateIndex;

  decodeSsIdx();
  if (result.info.surfaceStateIndex != 0)
    dss << ".ssidx=" << result.info.surfaceStateIndex;

  decodeUvrOffsets();
  if (!result.syntax.immOffset.empty())
    dss << ".uvroff" << result.syntax.immOffset;

  addReserved(42, 5);


  if (src0Len >= 0 && src1Len >= 0) {
    int srcPayloads = src0Len + src1Len;
    if (feedback) // one of the registers is the header
      srcPayloads--;
    int coords = srcPayloads / 2;
    auto s = smpOpInfo.describeParams(coords);
    if (!s.empty())
      dss << ": " << s;
  }
  result.info.description = dss.str();

  int dstCmp =
      std::max<int>(result.info.execWidth * result.info.elemSizeBitsRegFile / 8,
                    BYTES_PER_REGISTER);
  result.info.dstLenBytes = dstCmp * result.info.elemsPerAddr;
  if (trtt) { // includes an extra phase with the status bits
    result.info.dstLenBytes += BYTES_PER_REGISTER;
    if (execSize == ExecSize::SIMD32) { //in SIMD32 mode, 2 trailing regsiters
      result.info.dstLenBytes += BYTES_PER_REGISTER;
    }
  }
  if (src0Len >= 0)
    result.info.src0LenBytes = src0Len * BYTES_PER_REGISTER;
  if (src1Len >= 0)
    result.info.src1LenBytes = src1Len * BYTES_PER_REGISTER;
}

std::string MessageDecoderSamplerE64::decodeChannels(bool gather4) {
  // If gather4 message:
  //   Desc[8:7]  RESERVED
  //   Desc[10:9] Gather4SourceChannelSelect
  // ELSE
  //   Desc[10:7] WriteChannelMask
  std::string syn;
  if (gather4) {
    // https://gfxspecs.intel.com/Predator/Home/Index/71638
    addReserved(7, 2);
    auto g4 = getDescBits(9, 2);
    std::string meaning;
    switch (g4) {
    case 0:
      meaning = "select R channel";
      syn = ".r";
      break;
    case 1:
      meaning = "select G channel";
      syn = ".g";
      break;
    case 2:
      meaning = "select B channel";
      syn = ".b";
      break;
    case 3:
      meaning = "select A channel";
      syn = ".a";
      break;
    }
    addField("Gather4SourceChannelSelect", 9, 2, g4, meaning);
    result.info.elemsPerAddr = 4;
  } else {
    result.info.addAttr(MessageInfo::Attr::HAS_CHMASK);
    syn = decodeVectorSizeCmask("rgba");
  }
  return syn;
}

void iga::DecodeMessageSamplerE64(Platform platform, SFID sfid,
                                  ExecSize execSize, int src0Len, int src1Len,
                                  uint64_t desc, RegRef id0, RegRef id1,
                                  DecodeResult &result) {
  MessageDecoderSamplerE64 mdec {platform, sfid, execSize, src0Len, src1Len,
                                 desc, id0, id1, result};
  mdec.decode();
  if (result.errors.empty()) {
    mdec.checkBytesLen("src0", result.info.src0LenBytes, src0Len);
    mdec.checkBytesLen("src1", result.info.src1LenBytes, src1Len);
  }
}
