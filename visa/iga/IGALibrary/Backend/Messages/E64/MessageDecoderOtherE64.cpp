/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#include "MessageDecoderE64.hpp"

using namespace iga;

struct MessageDecoderOtherE64 : MessageDecoderE64 {
  MessageDecoderOtherE64(Platform _platform, SFID _sfid, ExecSize _execSize,
                         int _src0Len, int _src1Len, uint64_t _desc,
                         RegRef id0, RegRef id1, DecodeResult &_result)
      : MessageDecoderE64(_platform, _sfid, _execSize, _src0Len, _src1Len,
                          _desc, id0, id1, _result) {}

  void decodeGTWY();
  void decodeRTA();
  void decodeBTD();
  void decodeRC();

private:
  const SendOpDefinition &addOpCode(SendOp op);
}; // MessageDecoderOtherE64

const SendOpDefinition &MessageDecoderOtherE64::addOpCode(SendOp op) {
  auto opBits = getDescBits(0, 6);
  const auto &sOpInfo = lookupSendOp(op);
  result.info.op = op;
  result.info.description = sOpInfo.description;
  result.info.symbol = sOpInfo.mnemonic;
  addField("Opcode", 0, 6, opBits, sOpInfo.description);
  return sOpInfo;
}

void MessageDecoderOtherE64::decodeGTWY() {
  enum GW_OPCODE {
    SIG_EOT = 0x0,
    SIG_BAR = 0x4,
    SIG_NAMED_BAR = 0x5,
    SAVE_BAR = 0x8,
    RESTORE_BAR = 0x9,
    SIG_EOTR = 0xA,
    RESTORE_BTD_STACK = 0xB,
    SIG_SIPBAR = 0xC,
  };
  result.info.execWidth = int(execSize);

  const auto opBits = getDescBits(0, 6);

  auto decodeBarrierSigDesc = [&](SendOp op) {
    addOpCode(op);

    uint32_t actvThrds = decodeField<uint32_t>(
        "ActiveThreadsOnly", 7, 1, [&](uint32_t enc, std::stringstream &mss) {
          if (enc) {
            mss << "EOT'd threads are not expected to "
                   "arrive at barrier";
          } else {
            mss << "All threads are expected to arrive at barrier";
            result.syntax.controls += ".all";
            result.info.description += " (include exited)";
          }
          return enc;
        });

    if (actvThrds && op == SendOp::SIGNAL_NAMED_BARRIER)
      error(7, 1, "ActiveThreadsOnly forbidden on named barrier");
    addReserved(6, 1);
    addReserved(8, 46 - 8 + 1);
  }; // decodeBarrierSigDesc

  auto decodeEotDesc = [&](SendOp op)
  {
    addOpCode(op);
    addReserved(6, 1);
    decodeField("EotSubfunc", 7, 2,
                [&](uint32_t enc, std::stringstream &mss) -> void {
                  switch (enc) {
                  case 0x0:
                    mss << "normal";
                    break;
                  case 0x1:
                    mss << "replay saved EOT";
                    result.info.description += " (replay saved)";
                    break;
                  case 0x2:
                    mss << "replay not run";
                    result.info.description += " (replay not run)";
                    break;
                  case 0x3:
                    mss << "empty EOT (no valid thread state)";
                    result.info.description += " (empty state)";
                    break;
                  default:
                    mss << "?";
                    break;
                  }
                });
    addReserved(9, 46 - 9 + 1);
  }; // decodeEotDesc

  auto decodeSimpleDescriptor = [&](SendOp op) {
    addOpCode(op);
    addReserved(6, 41);
  };

  result.info.dstLenBytes = 0;
  result.info.src0LenBytes = 0;
  result.info.src1LenBytes = 0; // all gateway messages lack src1

  switch (opBits) {
  case SIG_BAR:
    decodeBarrierSigDesc(SendOp::SIGNAL_BARRIER);
    addDocs(
      DocRef::Kind::MSGOP, "GW_SIG_BARRIER", "72051",
      DocRef::Kind::DESC, "GW_SIGBAR_DESC", "72052",
      DocRef::Kind::SRC0, "GW_SIGBAR_PAYLOAD", "72057");
    result.info.src0LenBytes = 12; // r0.2[31:24] ... call it r0.{0,1,2}
    break;
  case SIG_NAMED_BAR:
    decodeBarrierSigDesc(SendOp::SIGNAL_NAMED_BARRIER);
    addDocs(
      DocRef::Kind::MSGOP, "GW_SIG_BARRIER", "72135",
      DocRef::Kind::DESC, "GW_SIGBAR_DESC", "72052",
      DocRef::Kind::SRC0, "GW_NAMEDBAR_PAYLOAD", "72064");
    result.info.src0LenBytes = 12; // r0.2[31:0] ... call it r0.{0,1,2}
    break;
  case SIG_SIPBAR:
    decodeBarrierSigDesc(SendOp::SIGNAL_SYSTEM_ROUTINE_BARRIER);
    addDocs(
      DocRef::Kind::MSGOP, "GW_SIG_SIPBARRIER", "72058",
      DocRef::Kind::DESC, "GW_SIGBAR_DESC", "72052",
      DocRef::Kind::SRC0, "GW_SIGBAR_PAYLOAD", "72057",
      DocRef::Kind::DST, "MDP_SIP_BARRIER_RESPONSE", "70576");
    result.info.dstLenBytes = 4; // DST.0[31:0] = "First Thread"
    result.info.src0LenBytes = 12; // r0.2[31:24] ... call it r0.{0,1,2}
    break;
  case SIG_EOT:
    decodeEotDesc(SendOp::EOT);
    addDocs(
      DocRef::Kind::MSGOP, "GW_EOT", "72054",
      DocRef::Kind::DESC, "GW_EOT_DESC", "72055",
      DocRef::Kind::SRC0, "GW_EOT_PAYLOAD", "70051");
    result.info.dstLenBytes = 0;
    result.info.src0LenBytes = 12; // r0.2[31:24] ... call it r0.{0,1,2}
    break;
  case SIG_EOTR:
    decodeSimpleDescriptor(SendOp::EOTR);
    addDocs(
      DocRef::Kind::MSGOP, "GW_SIG_EOTR", "72060",
      DocRef::Kind::DESC, "GW_EOTR_DESC", "72092",
      DocRef::Kind::SRC0, "EOR_PAYLOAD", "67738");
    result.info.src0LenBytes = 1; // reg[7:0] walker id
    break;
  case SAVE_BAR:
    decodeSimpleDescriptor(SendOp::SAVE_BARRIER);
    addDocs(
      DocRef::Kind::MSGOP, "GW_SAVE_BAR", "72068",
      DocRef::Kind::DESC, "GW_SAVE_RESTORE_BAR_DESC", "72067",
      DocRef::Kind::DST, "SAVE_RESTORE_BAR_PAYLOAD", "67043",
      DocRef::Kind::SRC0, "MDP_SAVE_BARRIER", "70584");
    result.info.src0LenBytes = 12; // named barrier ID src0.2[7:0]
    result.info.dstLenBytes = BYTES_PER_REGISTER;
    break;
  case RESTORE_BAR:
    decodeSimpleDescriptor(SendOp::RESTORE_BARRIER);
    addDocs(
      DocRef::Kind::MSGOP, "GW_RESTORE_BAR", "72059",
      DocRef::Kind::DESC, "GW_SAVE_RESTORE_BAR_DESC", "72067",
      DocRef::Kind::SRC0, "SAVE_RESTORE_BAR_PAYLOAD", "67043");
    result.info.src0LenBytes = BYTES_PER_REGISTER;
    break;
  case RESTORE_BTD_STACK:
    decodeSimpleDescriptor(SendOp::RESTORE_STACK);
    addDocs(
      DocRef::Kind::MSGOP, "RESTORE_BTD_STACK", "72108",
      DocRef::Kind::DESC, "GW_RESTORE_STACK_DESC", "72113",
      DocRef::Kind::SRC0, "STK_RSTR_PAYLOAD", "68749");
    break;
  default:
    error(0, 6, "inavlid gateway op");
  } // switch
  if (execSize != ExecSize::SIMD1 && execSize != ExecSize::INVALID) {
    error(0, 6, "exec size must be SIMD1");
  }
  if (indDesc0 != REGREF_INVALID) {
    error(0, 6, "IND0 should not be set");
  }
  if (indDesc1 != REGREF_INVALID) {
    error(0, 6, "IND1 should not be set");
  }
}

void MessageDecoderOtherE64::decodeRTA() {
  addDocs(DocRef::Kind::DESC, "TRACERAY_MSD_SIMPLIFIED", "72132",
          DocRef::Kind::DST, "RAYQUERY_WRBK", "70491",
          DocRef::Kind::SRC0, "TRACE_RAY_GRP_PAYLOAD", "73256",
          DocRef::Kind::IND0, "TraceRayIndDesc0", "73321",
          DocRef::Kind::IND1, "TraceRayIndDesc1", "74327");
  enum {
    OP_TRACE_RAY_ASYNC = 0x0,
    OP_TRACE_RAY_SYNC = 0x1,
    OP_TRACE_BOX_SYNC = 0x2,
    OP_RAYQUERY_CHECK = 0x3,
    OP_RAYQUERY_RELEASE = 0x4
  };

  const auto famBits = getDescBits(4, 2);
  if (famBits != 0) {
    error(4, 2, "op family must be 0");
    return;
  }
  addField("OpFamily", 4, 2, famBits, "");

  const auto &sOpInfo = lookupSendOp(SendOp::TRACE_RAY);
  result.info.description = sOpInfo.description;
  result.info.symbol = sOpInfo.mnemonic;
  result.info.op = SendOp::TRACE_RAY;

  const auto opBits = getDescBits(0, 4);
  switch (opBits) {
  case OP_TRACE_RAY_ASYNC: result.info.symbol += ".async"; break;
  case OP_TRACE_RAY_SYNC: result.info.symbol += ".sync"; break;
  case OP_TRACE_BOX_SYNC: result.info.symbol += ".box_sync"; break;
  case OP_RAYQUERY_CHECK: result.info.symbol += ".ray_query_check"; break;
  case OP_RAYQUERY_RELEASE: result.info.symbol += ".ray_query_release"; break;
  default: result.info.symbol += ".???";
  }
  addField("Opcode", 0, 4, opBits, result.info.description);
  addReserved(6, 6);
  decodeField("Spawn Type", 12, 2, [](uint32_t enc, std::stringstream &ss) {
    if (enc == 0)
      ss << "Ordinary Spawn";
    else
      ss << "?";
  });
  addReserved(14, 46 - 14 + 1);

  bool isSimd32 = execSize == ExecSize::INVALID || execSize == ExecSize::SIMD32;
  result.info.dstLenBytes = BYTES_PER_REGISTER;
  result.info.src0LenBytes = (isSimd32 ? 3 : 2) * BYTES_PER_REGISTER;
}

void MessageDecoderOtherE64::decodeBTD() {
  addDocs(DocRef::Kind::MSGOP, "BTD_OPCODE", "72483",
          DocRef::Kind::DESC, "BTD_SPAWN_MSD_SIMPLE", "72133",
          DocRef::Kind::SRC0, "BTD_SPAWN_PAYLOAD", "73257",
          DocRef::Kind::IND0, "BTDInd0Desc", "73359",
          DocRef::Kind::IND1, "BTDInd1Desc", "72112");
  enum {
    BTD_OPCODE_NORMAL = 0x1,
    BTD_OPCODE_OVERDISPATCH = 0x2,
  };

  const auto opBits = getDescBits(0, 6);
  switch (opBits) {
  case BTD_OPCODE_NORMAL:
  case BTD_OPCODE_OVERDISPATCH: {
    const auto &sOpInfo = lookupSendOp(SendOp::SPAWN);
    result.info.description = sOpInfo.description;
    result.info.symbol = sOpInfo.mnemonic;
    result.info.op = SendOp::SPAWN;
    if (opBits == BTD_OPCODE_OVERDISPATCH) {
      result.info.description += " (with overdispatch)";
    }
    break;
  }
  default:
    result.info.description = "?";
    break;
  }
  addField("Opcode", 0, 6, opBits, result.info.description);
  addReserved(6, 8);
  addReserved(14, 46 - 14 + 1);

  bool isSimd32 = execSize == ExecSize::INVALID || execSize == ExecSize::SIMD32;
  result.info.dstLenBytes = 0;
  result.info.src0LenBytes = (isSimd32 ? 5 : 3) * BYTES_PER_REGISTER;
}

void MessageDecoderOtherE64::decodeRC() {
  enum {
    RT_READ = 0x2,
    RT_WRITE_DUAL = 0x5,
    RT_WRITE = 0x6,
  };
  bool isSimd32 = execSize == ExecSize::INVALID || execSize == ExecSize::SIMD32;
  addDoc(DocRef::Kind::DESC, "DESCRIPTOR_RENDER_TARGET_MESSAGE", "71862");

  const auto opBits = getDescBits(0, 6);
  switch (opBits) {
  case RT_READ: {
    const auto &sOpInfo = lookupSendOp(SendOp::RENDER_READ);
    result.info.op = sOpInfo.op;
    result.info.description = sOpInfo.description;
    result.info.symbol = sOpInfo.mnemonic;
    // result.info.dstLen = ??;
    break;
  }
  case RT_WRITE:
  case RT_WRITE_DUAL: {
    if (opBits == RT_WRITE) {
      if (isSimd32) {
        addDoc(DocRef::Kind::SRC0, "MDP_RTW_SIMD32", "65198");
      } else {
        addDoc(DocRef::Kind::SRC0, "MDP_RTW", "63902");
      }
    } else {
      addDoc(DocRef::Kind::SRC0, "MDP_RTW_DS", "63914");

    }
    const auto &sOpInfo = lookupSendOp(SendOp::RENDER_WRITE);
    result.info.op = sOpInfo.op;
    result.info.description = sOpInfo.description;
    result.info.symbol = sOpInfo.mnemonic;
    if (opBits == 0x5) {
      result.info.description += " (dual source write)";
    }
    result.info.dstLenBytes = 0;
    break;
  }
  default:
    result.info.description = "?";
    break;
  }

  addField("Opcode", 0, 6, opBits, result.info.description);
  addReserved(6, 1);
  auto cm = decodeVectorSizeCmask("xyzw");
  result.info.symbol += cm;
  result.info.description += " of " + cm;
  addReserved(11, 3);
  decodeField("Last Render Target Select", 14, 1,
              [&](uint32_t enc, std::stringstream &ss) {
                if (enc) {
                  result.info.description += " with last render target select";
                  ss << "last render target select";
                }
              });
  addReserved(15, 6);
  decodeField("Null Render Target", 21, 1,
              [&](uint32_t enc, std::stringstream &ss) {
                if (enc) {
                  result.info.description += " with null target";
                  ss << "null render target";
                }
              });
  addReserved(22, 33 - 22 + 1);
  static const char *HDR_REDUCTION_FIELDS[] = {
      "Output Mask to Render Target", "Source Depth Present", "Stencil Present",
      "Src0 Alpha Present"};
  static const char *HDR_REDUCTION_FIELDS_SHORT[] = {"output mask", "depth",
                                                     "stencil", "alpha"};
  bool first = true;
  int extraCoords = 0;
  std::stringstream sso;
  for (int i = 0;
       i < sizeof(HDR_REDUCTION_FIELDS) / sizeof(HDR_REDUCTION_FIELDS[0]);
       i++) {
    extraCoords += decodeField<int>(HDR_REDUCTION_FIELDS[i], 34 + i, 1,
                                    [&](uint32_t enc, std::stringstream &ss) {
                                      if (enc) {
                                        if (first) {
                                          result.info.description += " with ";
                                          first = false;
                                        } else {
                                          result.info.description += ", ";
                                        }
                                        ss << HDR_REDUCTION_FIELDS_SHORT[i]
                                           << " present in message";
                                        result.info.description +=
                                            HDR_REDUCTION_FIELDS_SHORT[i];
                                      }
                                      return enc ? 1 : 0;
                                    });
  }
  // (could be split between src0 and src1....) ick... (no idea how to solve)
  if (opBits == RT_READ) {
    int regsPerCoord = execSize == ExecSize::SIMD32 ? 2 : 1;
    // R?, G?, B?, A?
    result.info.dstLenBytes = BYTES_PER_REGISTER * regsPerCoord *
                              (result.info.elemsPerAddr + extraCoords);
    result.info.src0LenBytes = BYTES_PER_REGISTER;
  }

  addReserved(38, 46 - 38 + 1);
}

void iga::DecodeMessageOtherE64(Platform p, SFID sfid, ExecSize execSize,
                                int src0Len, int src1Len, uint64_t desc,
                                RegRef id0, RegRef id1, DecodeResult &result) {
  MessageDecoderOtherE64 mduo(p, sfid, execSize, src0Len, src1Len, desc,
                                  id0, id1, result);

  if (sfid == SFID::GTWY) {
    mduo.decodeGTWY();
  } else if (sfid == SFID::RTA) {
    mduo.decodeRTA();
  } else if (sfid == SFID::RC) {
    mduo.decodeRC();
  } else if (sfid == SFID::BTD) {
    mduo.decodeBTD();
  } else {
    mduo.error(0, 0, "unsupported SFID");
  }

  if (result.errors.empty()) {
    mduo.checkBytesLen("src0", result.info.src0LenBytes, src0Len);
    mduo.checkBytesLen("src1", result.info.src1LenBytes, src1Len);
  }
}
