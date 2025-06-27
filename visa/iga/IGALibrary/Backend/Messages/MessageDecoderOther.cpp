/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "MessageDecoder.hpp"

#include <tuple>

using namespace iga;

struct MessageDecoderOther : MessageDecoderLegacy {
  MessageDecoderOther(Platform _platform, SFID _sfid, ExecSize _execSize,
                      uint32_t exImmOffDesc, SendDesc _exDesc, SendDesc _desc,
                      DecodeResult &_result)
      : MessageDecoderLegacy(_platform, _sfid, _execSize, exImmOffDesc, _exDesc,
                             _desc, _result) {}

  void tryDecodeOther() {
    switch (sfid) {
    case SFID::GTWY:
      tryDecodeGTWY();
      break;
    case SFID::RC:
      if (platform() >= Platform::XE2) {
        tryDecodeRC_XE2();
      } else {
        tryDecodeRC();
      }
      break;
    case SFID::RTA:
      tryDecodeRTA();
      break;
    case SFID::BTD:
      tryDecodeBTD();
      break;
    case SFID::SMPL:
      tryDecodeSMPL();
      break;
    case SFID::TS:
      tryDecodeTS();
      break;
    case SFID::URB:
      tryDecodeURB();
      break;
    default:
      error(0, 0, "invalid sfid");
    }
  }

  void tryDecodeGTWY();
  void tryDecodeRC();
  void tryDecodeRC_XE2();
  void tryDecodeRTA();
  void tryDecodeBTD();
  void tryDecodeSMPL();
  void tryDecodeTS();
  void tryDecodeURB();

}; // MessageDecodeOther

void MessageDecoderOther::tryDecodeGTWY() {

  struct GatewayElement {
    uint32_t encoding;
    SendOp op;
    const char *type;
    const char *preXeDoc, *pstXeDoc, *pstXe2Doc;
    constexpr GatewayElement(uint32_t enc, SendOp snd, const char *ty,
                             const char *preXe, const char *pstXe,
                             const char *pstXe2)
        : encoding(enc), op(snd), type(ty), preXeDoc(preXe), pstXeDoc(pstXe),
          pstXe2Doc(pstXe2) {}
  };
  //
  static constexpr GatewayElement GATEWAY_MESSAGES[]{
      {0x0, SendOp::EOT, "MSD_EOT", nullptr, "54044", "57488"},
      {0x1, SendOp::SIGNAL, "MSD_SIGNAL_EVENT", "33508", "47927", "57492"},
      {0x2, SendOp::MONITOR, "MSD_MONITOR_EVENT", "33512", "47925", "57490"},
      {0x3, SendOp::UNMONITOR, "MSD_MONITOR_NO_EVENT", "33513", "47926",
       "57491"},
      // no 0x5
      {0x4, SendOp::SIGNAL_BARRIER, "MSD_BARRIER", "33524", "47924", "57489"},
      {0x6, SendOp::WAIT, "MSD_WAIT_FOR_EVENT", "33514", "47928", "57493"},
      {0x8, SendOp::SAVE_BARRIER, "MSD_SAVE_BARRIER", nullptr, nullptr,
       "67045"},
      {0x9, SendOp::RESTORE_BARRIER, "MSD_RESTORE_BARRIER", nullptr, nullptr,
       "67049"},
      {0xA, SendOp::EOTR, "MSD_END_OF_RESTORE", nullptr, nullptr, "67091"},
      {0xB, SendOp::RESTORE_STACK, "MSD_RESTORE_BTD_STACK", nullptr, nullptr,
       "68748"},
      {0xC, SendOp::SIGNAL_SYSTEM_ROUTINE_BARRIER, "MSD_SIP_BARRIER", nullptr,
       nullptr, "70574"},
  };

  const GatewayElement *ge = nullptr;
  // Bit [3] is defined on later platforms to extend gateway encodings
  // on prior platforms it's always MBZ at least
  // (or stuff that didn't make it to production)
  // So we can blindly pretend [3:0] is the opcode for all platforms.
  uint32_t opBits = getDescBits(0, 4); // [3:0]
  for (const auto &g : GATEWAY_MESSAGES) {
    if (g.encoding == opBits) {
      ge = &g;
      break;
    }
  }
  if (ge == nullptr) {
    error(0, 4, "unsupported GTWY op");
    return;
  }
  std::stringstream sym, desc;
  const SendOpDefinition &sod = lookupSendOp(ge->op);
  if (!sod.isValid()) {
    // should be in the table
    error(0, 4, "INTERNAL ERROR: cannot find GTWY op");
    return;
  }

  sym << sod.mnemonic;
  desc << sod.description;
  int expectMlen = 0, expectRlen = 0;
  if (sod.op == SendOp::EOT) {
    if (platform() < Platform::XE_HPG) {
      error(0, 4, "Gateway EOT not available on this platform");
    }
    expectMlen = 1;
  }
  const char *link = platform() >= Platform::XE2  ? ge->pstXe2Doc
                     : platform() >= Platform::XE ? ge->pstXeDoc
                                                  : ge->preXeDoc;
  addDoc(DocRef::DESC, ge->type, link);

  switch (sod.op) {
  case SendOp::EOT:
    addDoc(DocRef::SRC0, "GW_EOT_PAYLOAD",
           platform() >= Platform::XE2 ? "70051" : nullptr);
    expectMlen = 1;
    break;
  case SendOp::SIGNAL:
  case SendOp::MONITOR:   // C.f. MDP_EVENT
  case SendOp::UNMONITOR: // C.f. MDP_NO_EVENT
  case SendOp::WAIT:      // C.f. MDP_Timeout
    expectMlen = 1;
    break;
  case SendOp::SIGNAL_BARRIER: // C.f. MDP_Barrier
    // XeHP+ no register is returned
    expectRlen = platform() >= Platform::XE_HP ? 0 : 1;
    if (platform() >= Platform::XE2) {
      decodeDescBitField("ActiveThreadsOnly", 11,
                         "only active threads participate");
    }
    addDoc(DocRef::SRC0, "MDP_Barrier",
           platform() >= Platform::XE2      ? "57499"
           : platform() >= Platform::XE_HPG ? "54006"
           : platform() >= Platform::XE     ? "47931"
                                            : "33523");
    break;
  case SendOp::SAVE_BARRIER:
    addDoc(DocRef::DST, "MDP_SAVE_BARRIER", "70584");
    addDoc(DocRef::SRC0, "SAVE_RESTORE_BAR_PAYLOAD", "67043");
    expectRlen = 1;
    expectMlen = 1;
    break;
  case SendOp::RESTORE_BARRIER:
    addDoc(DocRef::SRC0, "SAVE_RESTORE_BAR_PAYLOAD", "67043");
    expectMlen = 1;
    break;
  case SendOp::RESTORE_STACK:
    addDoc(DocRef::SRC0, "STK_RSTR_PAYLOAD", "68749");
    expectMlen = 1;
    break;
  case SendOp::EOTR:
    addDoc(DocRef::SRC0, "EOR_PAYLOAD", "67738");
    expectMlen = 1;
    break;
  case SendOp::SIGNAL_SYSTEM_ROUTINE_BARRIER:
    addDoc(DocRef::DST, "MDP_SIP_BARRIER_RESPONSE", "70576");
    addDoc(DocRef::SRC0, "GW_SIP_BARRIER_PAYLOAD", "71500");
    expectRlen = 1;
    expectMlen = 1;
    break;
  default:
    break;
  }


  addField("GatewayOpcode", 0, 4, opBits, desc.str());
  setSpecialOpX(sym.str(), desc.str(), sod.op, AddrType::INVALID, SendDesc(0),
                expectMlen, // mlen
                expectRlen, // rlen
                MessageInfo::Attr::NONE);
  decodeMDC_HF(); // all gateway messages forbid a header
}

void MessageDecoderOther::tryDecodeRC() {
  // PSEUDO syntax
  //   rtw.{f16,f32}.{simd8,simd16,rep16,lo8ds,hi8ds}{.c}{.ps}{.lrts}{.sgh}
  //   rtr.{f32}.{simd8,simd16}{.ps}{.sgh}
  std::stringstream sym;

  static const uint32_t RT_READ = 0xD;
  static const uint32_t RT_WRITE = 0xC;

  std::string descSfx;
  sym << "rt";
  auto mt = getDescBits(14, 4);
  switch (mt) {
  case RT_WRITE:
    sym << "w";
    descSfx = "render target write";
    break;
  case RT_READ:
    sym << "r";
    descSfx = "render target read";
    break;
  default:
    descSfx = "unknown render target op";
    error(14, 4, "unsupported RC op");
  }
  addField("MessageTypeRC", 14, 4, mt, descSfx);

  std::stringstream descs;
  int bitSize =
      decodeDescBitField("DataSize", 30, "FP32", "FP16") == 0 ? 32 : 16;
  if (bitSize == 32) {
    descs << "full-precision";
    sym << ".f32";
  } else {
    descs << "half-precision";
    sym << ".f16";
    if (mt == RT_READ)
      warning(30, 1, "half-precision not supported on render target read");
  }
  descs << " " << descSfx;

  std::stringstream subopSymSs;
  auto subOpBits = getDescBits(8, 3);
  int execSize = 0;
  switch (mt) {
  case RT_WRITE:
    switch (subOpBits) {
    case 0x0:
      execSize = DEFAULT_EXEC_SIZE;
      subopSymSs << ".simd" << execSize;
      descs << " SIMD" << execSize;
      break;
    case 0x1:
      execSize = DEFAULT_EXEC_SIZE;
      subopSymSs << ".rep" << execSize;
      descs << " replicated SIMD" << execSize;
      break;
    case 0x2:
      execSize = DEFAULT_EXEC_SIZE / 2;
      subopSymSs << ".lo" << execSize << "ds";
      descs << " of low SIMD" << execSize;
      break;
    case 0x3:
      execSize = DEFAULT_EXEC_SIZE / 2;
      subopSymSs << ".hi" << execSize << "ds";
      descs << " of high SIMD" << execSize;
      break;
    case 0x4:
      execSize = DEFAULT_EXEC_SIZE / 2;
      subopSymSs << ".simd" << execSize;
      descs << " SIMD" << execSize;
      break;
    default:
      sym << ".???";
      descs << "unknown write subop";
      error(8, 3, "unknown write subop");
      break;
    }
    break;
  case RT_READ:
    switch (subOpBits) {
    case 0x0:
      execSize = DEFAULT_EXEC_SIZE;
      subopSymSs << ".simd" << execSize;
      descs << " SIMD" << execSize;
      break;
    case 0x1:
      execSize = DEFAULT_EXEC_SIZE / 2;
      subopSymSs << ".simd" << execSize;
      descs << " SIMD" << execSize;
      break;
    default:
      sym << ".???";
      descs << "unknown read subop";
      error(8, 3, "unknown read subop");
      break;
    }
    break;
  default:
    break;
  }
  addField("Subop", 8, 3, subOpBits, subopSymSs.str());
  sym << subopSymSs.str();

  if (mt == RT_WRITE) {
    auto pc = decodeDescBitField("PerCoarsePixelPSOutputs", 18, "disabled",
                                 "enabled");
    if (pc) {
      descs << " with Per-Coarse Pixel PS outputs enable";
      sym << ".cpo";
    }
  }
  auto ps = decodeDescBitField("PerSamplePS", 13, "disabled", "enabled");
  if (ps) {
    descs << " with Per-Sample Pixel PS outputs enable";
    sym << ".psp";
  }

  if (mt == RT_WRITE) {
    auto lrts =
        decodeDescBitField("LastRenderTargetSelect", 12, "false", "true");
    if (lrts) {
      descs << "; last render target";
      sym << ".lrts";
    }
  }

  auto sgs =
      decodeDescBitField("SlotGroupSelect", 11, "SLOTGRP_LO", "SLOTGRP_HI");
  if (sgs) {
    descs << " slot group high";
    sym << ".sgh";
  }

  uint32_t surfaceIndex = 0;
  (void)decodeDescField("BTS", 0, 8,
                        [&](std::stringstream &ss, uint32_t value) {
                          surfaceIndex = value;
                          ss << "surface " << value;
                          descs << " to surface " << value;
                          sym << ".bti[" << value << "]";
                        });

  MessageInfo &mi = result.info;
  mi.symbol = sym.str();
  mi.description = descs.str();
  mi.op = mt == RT_READ ? SendOp::RENDER_READ : SendOp::RENDER_WRITE;
  mi.execWidth = execSize;
  mi.elemSizeBitsMemory = mi.elemSizeBitsRegFile = bitSize;
  mi.addrSizeBits = 0;
  mi.surfaceId = surfaceIndex;
  mi.attributeSet = MessageInfo::Attr::NONE;

  decodeMDC_H2(); // all render target messages permit a dual-header
}
void MessageDecoderOther::tryDecodeRC_XE2() {
  // PSEUDO syntax:
  //   {rtw,rtdw}{.mrcps},{.hdr,}{.psps,}{.lo16,hi16,}
  //   rtr{.hdr,}{.psps,}{.lo16,hi16,}
  std::stringstream sym;

  // message type
  enum {
    RT_READ = 0xD,
    RT_WRITE = 0xC
  };

  // message subtype
  enum {
    ST_SIMD32_WRITE = 0x0,
    ST_SIMD16_DWRITE = 0x2 // no name in BXML
  };

  // pick default SIMD size if it's not set
  if (instExecSize == ExecSize::INVALID)
    instExecSize = ExecSize::SIMD16;

  std::string descSfx, stSfx;
  sym << "rt";
  const auto mt = getDescBits(14, 4);
  const auto st = getDescBits(8, 2);
  const auto isWr = mt == RT_WRITE;
  const auto isDuWr = isWr && st == ST_SIMD16_DWRITE;
  switch (mt) {
  case RT_WRITE:
    if (st == ST_SIMD16_DWRITE) {
      stSfx = "SIMD16_DWRITE";
      sym << "dw";
      descSfx = "render write dual";
      addDocs(DocRef::DESC, "MSD_RTW_SIMD16DS", "63908",
              DocRef::EXDESC, nullptr, "57391",
              DocRef::SRC0, "MH_RT", "57476",
              DocRef::SRC1, "MDP_RTW_DS", "63914");
      if (instExecSize == ExecSize::SIMD32)
        error(14, 4, "dual write must be SIMD16");
    } else if (st == ST_SIMD32_WRITE) {
      stSfx = "SIMD32_WRITE";
      sym << "w";
      descSfx = "render write";
      if (instExecSize == ExecSize::SIMD32)
        addDocs(DocRef::DESC, "MSD_RTW_SIMD32", "65209",
                DocRef::EXDESC, nullptr, "57391",
                DocRef::SRC0, "MH_RT", "57476",
                DocRef::SRC1, "MDP_RTW_SIMD32", "65198");
      else
        addDocs(DocRef::DESC, "MSD_RTW_SIMD16", "57384",
                DocRef::EXDESC, nullptr, "57391",
                DocRef::SRC0, "MH_RT", "57476",
                DocRef::SRC1, "MDP_RTW", "63902");
    }
    break;
  case RT_READ:
    sym << "r";
    descSfx = "render read";
    if (instExecSize == ExecSize::SIMD32)
      addDocs(DocRef::DESC, "MSD_RTR_SIMD32", "65211",
              DocRef::EXDESC, nullptr, "57391",
              DocRef::SRC0, "MH_RT", "57476");
    else
      addDocs(DocRef::DESC, "MSD_RTR_SIMD16", "57374",
              DocRef::EXDESC, nullptr, "57391",
              DocRef::SRC0, "MH_RT", "57476");
    break;
  default:
    descSfx = "unknown render target op";
    error(14, 4, "unsupported RC op");
  }

  addField("MessageTypeRC", 14, 4, mt, descSfx);
  addField("RenderTargetMessageSubtype", 8, 2, st, stSfx);

  std::stringstream descs;

  int bitSize =
      decodeDescBitField("DataSize", 30, "FP32", "FP16") == 0 ? 32 : 16;
  if (bitSize != 32) {
    warning(30, 1, "half-precision not supported");
  }
  descs << descSfx;

  auto decodeBit =
      [&](int off, const char *descField, const char *descSfx,
          const char *symSfx) {
        auto val = decodeDescBitField(descField, off, "Disabled", "Enabled");
        if (val) {
          descs << "  " << descSfx;
          sym << "." << symSfx;
        }
        return val;
      };

  // Desc[29] .mrcps - multirate coarse pixel shading
  if (mt == RT_READ) {
    decodeReserved(29, 1);
  } else {
    decodeBit(29, "MultiRateCPS", "multirate coarse pixel shading", "mrcps");
  }

  // Desc[19] .hdr - render target messages permit a header
  if (decodeMDC_H()) {
    sym << ".hdr";
  }

  // Desc[18] .pscps - per sample coarse pixel shading
  if (mt == RT_READ) {
    decodeReserved(18, 1);
  } else {
    decodeBit(18, "PerSampleOutCPS",
                  "per-sample outputs coarse pixel shading", "psocps");
  }

  // Desc[13] .psps - per sample pixel shading
  decodeBit(13, "PerSamplePSEnable", "per-sample pixel shading", "psps");

  // Desc[12] .lrts - last render target select
  decodeBit(12, "LastRenderTargetSelect", "last render target select", "lrts");

  // Desc[11] slot group select (.lo16|.hi16)
  if (instExecSize != ExecSize::SIMD32) {
    auto sgs = decodeDescBitField("SlotGroup", 11, "SG_LO", "SG_HI");
    if (sgs) {
      descs << " slot group " << (sgs ? "high16" : "low16");
      sym << (sgs ?  ".hi16" : ".lo16");
    }
  } else {
    decodeReserved(11, 1);
  }


  uint32_t surfaceIndex = 0;
  (void)decodeDescField("BTS", 0, 8,
                        [&](std::stringstream &ss, uint32_t value) {
                          surfaceIndex = value;
                          ss << "surface " << value;
                          descs << " to bti[" << value << "]";
                          sym << ".bti[" << value << "]";
                        });

  // ExDesc[23:21] - render target index
  if (mt == RT_READ) {
    decodeExDescReserved(21, 3);
  } else {
    decodeExDescField("RenderTargetIndex", 21, 3,
                      [&](std::stringstream &ss, uint32_t val) {
                        sym << ".rti[" << val << "]";
                        descs << ".rti[" <<val << "]";
                      });
  }


  // ExDesc[20] - null render target
  decodeExDescField("NullRenderTarget", 20, 1,
                    [&](std::stringstream &ss, uint32_t val) {
                      if (val)
                        sym << ".nullrt";
                    });

  // message layout is
  //  COLORS_SIMD32 = R,R,G,G,B,B,A,A
  //  COLORS_SIMD16 = R,G,B,A
  // RTW_SIMD16: [s0A][oM]{COLORS_SIMD16}[,SZ][,STENCIL]
  //   (4 to 8 total registers not counting possible header)
  // RTW_DSIMD16: [oM,]{COLORS_SIMD16},{COLORS_SIMD16}[,SZ][,STENCIL]
  //   (8 to 11 total registers not counting possible header)
  // RTW SIMD32: [s0A,s0A][,oM]{COLORS_SIMD32}[,SZ,SZ][,STENCIL,STENCIL]
  //   (8 to 15 total registers not counting possible header)
  const auto phasePerParam =
      (instExecSize == ExecSize::SIMD32 || isDuWr) ? 2 : 1;
  int dataPhases = 4 * phasePerParam;

  auto decodeExDescParam = [&](const char *field, int off, const char *sy,
                               int deltaPhases = 1) {
    return decodeExDescField(field, off, 1,
                             [&](std::stringstream &ss, uint32_t val) {
                               if (val) {
                                 sym << "." << sy;
                                 ss << "Present";
                                 dataPhases += deltaPhases;
                                 descs << "." << sy;
                               } else {
                                 ss << "Absent";
                               }
                             });
  };

  if (exDesc.isImm()) {
    // decode bits [15:12] in parameter order
    if (getDescBits(32 + 12, 4)) {
      descs << " with ex params ";
    }

    // ExDesc[15] - src0 alpha present
    auto s0a = decodeExDescParam("Src0AlphaPresent", 15, "s0a", phasePerParam);
    if (s0a && isDuWr) {
      error(15, 1, "Src0Alpha not permitted on render dual write");
    }
    // ExDesc[14] - stencil present
    decodeExDescParam("StencilPresent", 14, "stncl", 1);
    // ExDesc[13] - source depth
    decodeExDescParam("SourceDepth", 13, "sz", phasePerParam);
    // ExDesc[12] - omask
    decodeExDescParam("OMask", 12, "om", 1);

    // *** Comes directly from Src1.Len in EU bits ***
    //
    // ExDesc[10:6] - xlen
    // decodeExDescField("ExtendedMessageLength", 6, 4,
    //                   [&](std::stringstream &ss, uint32_t val) {
    //                     ss << "src1.length is " << val << " registers";
    //                   });
  } // else reg exDesc probably shouldn't happen, but we can't see it if so

  MessageInfo &mi = result.info;
  mi.symbol = sym.str();
  mi.description = descs.str();
  mi.op = mt == RT_READ            ? SendOp::RENDER_READ
          : st == ST_SIMD16_DWRITE ? SendOp::RENDER_DWRITE
                                   : SendOp::RENDER_WRITE;
  mi.execWidth = int(instExecSize);
  mi.elemSizeBitsMemory = mi.elemSizeBitsRegFile = 32;
  mi.addrSizeBits = 0; // the addresses of render target are implicit
  mi.surfaceId = surfaceIndex;
  mi.attributeSet = MessageInfo::Attr::NONE;
  // mi.dstLenBytes = 0;
  // if (header)
  //  result.info.src0LenBytes = BITS_PER_REGISTER / 8;
  // mi.src1LenBytes = dataPhases * BITS_PER_REGISTER / 8;
  (void)dataPhases;
}

void MessageDecoderOther::tryDecodeRTA() {
  const int TRACERAY_MSD = 0x00;

  std::stringstream sym, descs;
  int opBits = getDescBits(14, 4); // [17:14]
  if (opBits == TRACERAY_MSD) {
    int simd = decodeMDC_SM2(8);
    if (platform() >= Platform::XE2 && simd != 16)
      error(8, 1, "message must be SIMD16 on this platform");

    addDoc(DocRef::DESC, "TRACERAY_MSD",
           platform() >= Platform::XE2 ? "57495" : "47929");
    if (instExecSize == ExecSize::SIMD8 && platform() <= Platform::XE_HPG) {
      addDoc(DocRef::SRC0, "TRACE_RAY_SIMD8_PAYLOAD", "47938");
    } else if (instExecSize == ExecSize::SIMD16) {
      addDoc(DocRef::SRC0, "TRACE_RAY_SIMD16_PAYLOAD",
        platform() >= Platform::XE2 ? "57508" : "47937");
    }

    // int mlen = getDescBits(25, 4); // [28:25]
    sym << "trace_ray" << simd;
    descs << "simd" << simd << " trace ray";

    MessageInfo &mi = result.info;
    mi.symbol = sym.str();
    mi.description = descs.str();
    mi.op = SendOp::TRACE_RAY;
    // payload is 2 or 3 registers (SIMD8 vs SIMD16)
    // #47937
    //   - "addr" the first with a uniform pointer to global data
    //   - "data" the second being ray payloads[7:0] (each 32b)
    //   - "data" if SIMD16, rays[15:6]
    // we treat this as a SIMD1 operation (logically)
    // containing SIMD data 32b elements
    mi.execWidth = simd;
    mi.elemSizeBitsRegFile = mi.elemSizeBitsMemory = 32;
    mi.elemsPerAddr = 1;
    //
    mi.addrSizeBits = 64;
    if (platform() >= Platform::XE_HPC) {
      // XeHPC has a bit set at Msg[128] as well
      mi.addrSizeBits = 129;
    }
    mi.addrType = AddrType::FLAT;
    mi.surfaceId = 0;
    mi.attributeSet = MessageInfo::Attr::NONE;
    decodeMDC_HF();
  } else {
    error(14, 4, "unsupported RTA op");
  }
}

void MessageDecoderOther::tryDecodeBTD() {
  enum {
    BTD_SPAWN = 0x1,
    BTD_SPAWN_OVERDISPATCH = 0x2,
  };

  int simd = decodeMDC_SM2(8);
  if (platform() >= Platform::XE_HPC && simd != 16)
    error(8, 1, "message must be SIMD16 on this platform");
  if (instExecSize == ExecSize::INVALID) {
    instExecSize = ExecSize(simd);
  }

  std::stringstream sym, descs;
  uint32_t opBits = getDescBits(14, 4); // [17:14]
  addDoc(DocRef::DESC, "BTD_SPAWN_MSD", chooseDoc(nullptr, "47923", "57487"));
  if (instExecSize == ExecSize::SIMD8 && platform() <= Platform::XE_HPG) {
    addDoc(DocRef::SRC0, "SIMD8_BTD_SPAWN_PAYLOAD", "47936");
  } else if (instExecSize == ExecSize::SIMD16) {
    addDoc(DocRef::SRC0, "BTD_SIMD16_SPAWN_PAYLOAD",
      platform() >= Platform::XE2 ? "57501" : "47930");
  } else {
    error(14, 4, "invalid execution size");
  }

  if (opBits == BTD_SPAWN) {
    sym << "spawn" << simd;
    descs << "bindless simd" << simd << " spawn thread";
  } else if (opBits == BTD_SPAWN_OVERDISPATCH) {
    sym << "spawn_overdispatch" << simd;
    descs << "bindless simd" << simd << " spawn thread overdispatched";
  } else {
    error(14, 4, "invalid BTD op");
  }

  MessageInfo &mi = result.info;
  mi.op = SendOp::SPAWN;
  mi.description = descs.str();
  mi.symbol = sym.str();
  mi.execWidth = simd;

  addField("MessageType", 14, 4, opBits, descs.str());
}

///////////////////////////////////////////////////////////////////////////////
enum class SamplerSIMD {
  INVALID = 0,
  SIMD8,
  SIMD16,
  SIMD32,
  SIMD32_64,
  SIMD8H, // half precision parameters
  SIMD16H, // half precision parameters
  SIMD32H, // half precision parameters
  SIMD8_INTRET,  // with integer return XeHPG+
  SIMD16_INTRET, // with integer return XeHPG+
};

///////////////////////////////////////////////
// TGL ...
// 000 Reserved
// 001 SIMD8
// 010 SIMD16
// 011 SIMD32/64
// 100 Reserved
// 101 SIMD8H
// 110 SIMD16H
// 111 Reserved
//
///////////////////////////////////////////////
// XeHPG+
// 000 SIMD8 + Integer Return
// 001 SIMD8
// 010 SIMD16
// 011 Reserved
// 100 SIMD16 + Integer Return
// 101 SIMD8H
// 110 SIMD16H
// 111 Reserved
//
///////////////////////////////////////////////
// Xe2+
// 000 SIMD16 + Integer Return (not supported on XE2)
// 001 SIMD16
// 010 SIMD32
// 011 Reserved
// 100 SIMD16 + Integer Return (not supported on XE2)
// 101 SIMD16H
// 110 SIMD32H
// 111 Reserved

static SamplerSIMD decodeSimdSize(Platform p, uint32_t simdBits, int &simdSize,
                                  std::stringstream &syms,
                                  std::stringstream &descs) {
  SamplerSIMD simdMode = SamplerSIMD::INVALID;
  switch (simdBits) {
  case 0:
    if (p >= Platform::XE2) {
      simdMode = SamplerSIMD::SIMD16_INTRET;
      syms << "simd16_iret";
      descs << "simd16 with integer return";
      simdSize = 16;
      break;
    }
    if (p >= Platform::XE_HPG) {
      simdMode = SamplerSIMD::SIMD8_INTRET;
      syms << "simd8_iret";
      descs << "simd8 with integer return";
      simdSize = 8;
      break;
    }
    simdMode = SamplerSIMD::INVALID;
    syms << "???";
    descs << "unknown simd mode";
    simdSize = 1;
    return simdMode;
  case 1:
    if (p >= Platform::XE2) {
      simdMode = SamplerSIMD::SIMD16;
      simdSize = 16;
      descs << "simd16";
      syms << "simd16";
      break;
    }
    simdMode = SamplerSIMD::SIMD8;
    simdSize = 8;
    descs << "simd8";
    syms << "simd8";
    break;
  case 2:
    if (p >= Platform::XE2) {
      simdMode = SamplerSIMD::SIMD32;
      simdSize = 32;
      descs << "simd32";
      syms << "simd32";
      break;
    }
    simdMode = SamplerSIMD::SIMD16;
    simdSize = 16;
    descs << "simd16";
    syms << "simd16";
    break;
  case 3:
    if (p >= Platform::XE_HPG) {
      simdMode = SamplerSIMD::INVALID;
      syms << "???";
      descs << "unknown simd mode";
      simdSize = 1;
      break;
    }
    simdMode = SamplerSIMD::SIMD32_64;
    descs << "simd32/64";
    syms << "simd32";
    simdSize = 32;
    break;
  case 4:
    if (p >= Platform::XE2) {
      simdMode = SamplerSIMD::SIMD16_INTRET;
      syms << "simd32_iret";
      descs << "simd32 with integer return";
      simdSize = 32;
      break;
    }
    if (p >= Platform::XE_HPG) {
      syms << "simd16_iret";
      descs << "simd16 with integer return";
      simdMode = SamplerSIMD::SIMD16_INTRET;
      simdSize = 16;
      break;
    }
    simdMode = SamplerSIMD::INVALID;
    syms << "???";
    descs << "unknown simd mode";
    simdSize = 1;
    break;
  case 5:
    if (p >= Platform::XE2) {
      simdMode = SamplerSIMD::SIMD16H;
      simdSize = 16;
      syms << "simd16h";
      descs << "simd16 with half precision parameters";
      break;
    }
    simdMode = SamplerSIMD::SIMD8H;
    simdSize = 8;
    syms << "simd8h";
    descs << "simd8 half with precision parameters";
    break;
  case 6:
    if (p >= Platform::XE2) {
      simdMode = SamplerSIMD::SIMD32H;
      simdSize = 32;
      syms << "simd32h";
      descs << "simd32 with half precision parameters";
      break;
    }
    simdMode = SamplerSIMD::SIMD16H;
    syms << "simd16h";
    descs << "simd16 with half precision parameters";
    simdSize = 16;
    break;
  default:
    break;
  } // switch

  return simdMode;
}

int SamplerMessageDescription::countParams() const {
  int n = 0;
  for (SamplerParam sp : params) {
    if (sp == SamplerParam::NONE)
      break;
  }
  return n;
}

std::string SamplerMessageDescription::describe(int srcsLen) const {
  std::stringstream ss;
  ss << mnemonic << ":" << describeParams(srcsLen);
  return ss.str();
}
std::string SamplerMessageDescription::describeParams(int srcsLen) const {
  std::stringstream ss;
  bool first = true;
  int n = 0;
  for (SamplerParam sp : params) {
    if (sp == SamplerParam::NONE)
      break;
    if (first) {
      first = false;
    } else {
      ss << ",";
    }
    ss << ToSymbol(sp);
    // for optional parameters
    if (srcsLen >= 0 && ++n >= srcsLen)
      break;
  }
  return ss.str();
}

std::string iga::ToSymbol(SamplerParam sp) {
  switch (sp) {
  case SamplerParam::AI:
    return "ai";
  case SamplerParam::BIAS:
    return "bias";
  case SamplerParam::BIAS_AI:
    return "bias_ai";
  case SamplerParam::DUDX:
    return "dudx";
  case SamplerParam::DUDY:
    return "dudy";
  case SamplerParam::DUMMY:
    return "dummy";
  case SamplerParam::DVDX:
    return "dvdx";
  case SamplerParam::DVDY:
    return "dvdy";
  case SamplerParam::LOD:
    return "lod";
  case SamplerParam::LOD_AI:
    return "lod_ai";
  case SamplerParam::MCS0:
    return "mcs0";
  case SamplerParam::MCS1:
    return "mcs1";
  case SamplerParam::MCS2:
    return "mcs2";
  case SamplerParam::MCS3:
    return "mcs3";
  case SamplerParam::MLOD:
    return "mlod";
  case SamplerParam::MLOD_R:
    return "mlod_r";
  case SamplerParam::R:
    return "r";
  case SamplerParam::REF:
    return "ref";
  case SamplerParam::SI:
    return "si";
  case SamplerParam::U:
    return "u";
  case SamplerParam::V:
    return "v";
  case SamplerParam::OFFUV_R:
    return "offuv_r";
  case SamplerParam::OFFUVR:
    return "offuvr";
  case SamplerParam::OFFUV:
    return "offuv";
  case SamplerParam::OFFUVR_R:
    return "offuvr_r";
  case SamplerParam::BIAS_OFFUVR:
    return "bias_offuvr";
  case SamplerParam::BIAS_OFFUV:
    return "bias_offuv";
  case SamplerParam::LOD_OFFUV:
    return "lod_offuv";
  default:
    return "?";
  }
}

static SamplerMessageDescription
lookupSamplerMessageInfoXe2(uint32_t samplerMessageType, SamplerSIMD simd) {
  static constexpr SamplerMessageDescription INVALID(SendOp::INVALID,
                                                     "<<invalid>>", 0);

  // if the message is fp16 (apparently they bake that into the "SIMD" size)
  bool isFp16 = simd == SamplerSIMD::SIMD8H || simd == SamplerSIMD::SIMD16H ||
                simd == SamplerSIMD::SIMD32H;
  auto chooseBasedOnDataSize = [&](SamplerParam fp16, SamplerParam fp32) {
    return isFp16 ? fp16 : fp32;
  };
  switch (samplerMessageType) {
  case 0x00:
    return SamplerMessageDescription(
        SendOp::SAMPLE, "sample", 1, SamplerParam::U, SamplerParam::V,
        SamplerParam::R, SamplerParam::AI, SamplerParam::MLOD);
  case 0x01:
    return SamplerMessageDescription(
        SendOp::SAMPLE_B, "sample_b", 2,
        chooseBasedOnDataSize(SamplerParam::BIAS, SamplerParam::BIAS_AI),
        SamplerParam::U, SamplerParam::V, SamplerParam::R,
        chooseBasedOnDataSize(SamplerParam::AI, SamplerParam::MLOD),
        chooseBasedOnDataSize(SamplerParam::MLOD, SamplerParam::NONE));
  case 0x02:
    return SamplerMessageDescription(
        SendOp::SAMPLE_L, "sample_l", 2,
        chooseBasedOnDataSize(SamplerParam::LOD, SamplerParam::LOD_AI),
        SamplerParam::U, SamplerParam::V, SamplerParam::R,
        chooseBasedOnDataSize(SamplerParam::AI, SamplerParam::NONE));
  case 0x03:
    return SamplerMessageDescription(
        SendOp::SAMPLE_C, "sample_c", 2, SamplerParam::REF, SamplerParam::U,
        SamplerParam::V, SamplerParam::R, SamplerParam::AI, SamplerParam::MLOD);
  case 0x04:
    return SamplerMessageDescription(
        SendOp::SAMPLE_D, "sample_d", 2, SamplerParam::U, SamplerParam::DUDX,
        SamplerParam::DUDY, SamplerParam::V, SamplerParam::DVDX,
        SamplerParam::DVDY, SamplerParam::R, SamplerParam::MLOD);
  case 0x05:
    return SamplerMessageDescription(
        SendOp::SAMPLE_B_C, "sample_b_c", 3, SamplerParam::REF,
        chooseBasedOnDataSize(SamplerParam::BIAS, SamplerParam::BIAS_AI),
        SamplerParam::U, SamplerParam::V, SamplerParam::R,
        chooseBasedOnDataSize(SamplerParam::AI, SamplerParam::MLOD),
        chooseBasedOnDataSize(SamplerParam::MLOD, SamplerParam::NONE));
  case 0x06:
    return SamplerMessageDescription(
        SendOp::SAMPLE_L_C, "sample_l_c", 3, SamplerParam::REF,
        chooseBasedOnDataSize(SamplerParam::LOD, SamplerParam::LOD_AI),
        SamplerParam::U, SamplerParam::V, SamplerParam::R,
        chooseBasedOnDataSize(SamplerParam::AI, SamplerParam::NONE));
  case 0x07:
    return SamplerMessageDescription(SendOp::LD, "sampler_ld", 1,
                                     SamplerParam::U, SamplerParam::V,
                                     SamplerParam::LOD, SamplerParam::R);
  case 0x08:
  case 0x09:
    return SamplerMessageDescription(
        samplerMessageType == 0x08 ? SendOp::GATHER4 : SendOp::SAMPLE_LOD,
        samplerMessageType == 0x08 ? "gather4" : "sampler_lod", 1,
        SamplerParam::U, SamplerParam::V, SamplerParam::R, SamplerParam::AI);
  case 0x0A:
    return SamplerMessageDescription(SendOp::SAMPLE_RESINFO, "sampler_resinfo",
                                     1, SamplerParam::LOD);
  case 0x0B:
    return SamplerMessageDescription(SendOp::SAMPLE_INFO, "sample_info", 0,
                                     SamplerParam::DUMMY);
  case 0x0C:
    return SamplerMessageDescription(SendOp::SAMPLE_KILLPIX, "sampler_killpix",
                                     1, SamplerParam::U, SamplerParam::V,
                                     SamplerParam::R);
  case 0x0D:
    return SamplerMessageDescription(
        SendOp::GATHER4_L, "gather4_l", 2,
        chooseBasedOnDataSize(SamplerParam::LOD, SamplerParam::LOD_AI),
        SamplerParam::U, SamplerParam::V, SamplerParam::R,
        chooseBasedOnDataSize(SamplerParam::AI, SamplerParam::NONE));
  case 0x0E:
    return SamplerMessageDescription(
        SendOp::GATHER4_B, "gather4_b", 2,
        chooseBasedOnDataSize(SamplerParam::BIAS, SamplerParam::BIAS_AI),
        SamplerParam::U, SamplerParam::V, SamplerParam::R,
        chooseBasedOnDataSize(SamplerParam::AI, SamplerParam::NONE));
  case 0x0F:
    return SamplerMessageDescription(SendOp::GATHER4_I, "gather_i", 1,
                                     SamplerParam::U, SamplerParam::V,
                                     SamplerParam::R, SamplerParam::AI);
  case 0x10:
    return SamplerMessageDescription(
        SendOp::GATHER4_C, "gather4_c", 2, SamplerParam::REF, SamplerParam::U,
        SamplerParam::V, SamplerParam::R, SamplerParam::AI);
  case 0x11:
      return SamplerMessageDescription(SendOp::SAMPLE_D_C_MLOD, "sample_d_c_mlod",
          4, SamplerParam::REF, SamplerParam::U,
          SamplerParam::DUDX, SamplerParam::DUDY,
          SamplerParam::V, SamplerParam::DVDX,
          SamplerParam::DVDY, SamplerParam::MLOD_R);
  case 0x12:
    return SamplerMessageDescription(
        SendOp::SAMPLE_MLOD, "sample_mlod", 1, SamplerParam::MLOD,
        SamplerParam::U, SamplerParam::V, SamplerParam::R, SamplerParam::AI);
  case 0x13:
    return SamplerMessageDescription(SendOp::SAMPLE_C_MLOD, "sample_c_mlod", 3,
                                     SamplerParam::MLOD, SamplerParam::REF,
                                     SamplerParam::U, SamplerParam::V,
                                     SamplerParam::R, SamplerParam::AI);
  case 0x14:
      return SamplerMessageDescription(
          SendOp::SAMPLE_D_C, "sample_d_c", 4, SamplerParam::REF, SamplerParam::U,
          SamplerParam::DUDX, SamplerParam::DUDY, SamplerParam::V,
          SamplerParam::DVDX, SamplerParam::DVDY, SamplerParam::R);
  case 0x15:
    return SamplerMessageDescription(
        SendOp::GATHER4_I_C, "gather4_i_c", 2, SamplerParam::REF,
        SamplerParam::U, SamplerParam::V, SamplerParam::R, SamplerParam::AI);
  //////////////////////////////////
  // case 0x16: gap
  case 0x17:
    return SamplerMessageDescription(
        SendOp::GATHER4_L_C, "gather4_l_c", 3, SamplerParam::REF,
        chooseBasedOnDataSize(SamplerParam::LOD, SamplerParam::LOD_AI),
        SamplerParam::U, SamplerParam::V, SamplerParam::R,
        chooseBasedOnDataSize(SamplerParam::AI, SamplerParam::NONE));
  case 0x18:
    return SamplerMessageDescription(SendOp::SAMPLE_LZ, "sample_lz", 1,
                                     SamplerParam::U, SamplerParam::V,
                                     SamplerParam::R, SamplerParam::AI);
  case 0x19:
    return SamplerMessageDescription(
        SendOp::SAMPLE_C_LZ, "sample_c_lz", 2, SamplerParam::REF,
        SamplerParam::U, SamplerParam::V, SamplerParam::R, SamplerParam::AI);
  case 0x1A:
    return SamplerMessageDescription(SendOp::LD_LZ, "ld_lz", 1,
                                     SamplerParam::U, SamplerParam::V,
                                     SamplerParam::R);
  case 0x1B:
    return SamplerMessageDescription(SendOp::SAMPLE_LD_L, "ld_l", 1,
                                     SamplerParam::U, SamplerParam::V,
                                     SamplerParam::R, SamplerParam::LOD);
  case 0x1C:
    return SamplerMessageDescription(
        SendOp::LD_2DMS_W, "ld2dms_w", 1, SamplerParam::SI,
        SamplerParam::MCS0, SamplerParam::MCS1, SamplerParam::MCS2,
        SamplerParam::MCS3, SamplerParam::U, SamplerParam::V, SamplerParam::R);
  case 0x1D:
    return SamplerMessageDescription(SendOp::LD_MCS, "ld_mcs", 1,
                                     SamplerParam::U, SamplerParam::V,
                                     SamplerParam::R);
  case 0x1F:
    return SamplerMessageDescription(SendOp::SAMPLE_FLUSH, "cache_flush", 0,
                                     SamplerParam::DUMMY);

  case 0x20:
    return SamplerMessageDescription(
        SendOp::SAMPLE_PO, "sample_po", 3, SamplerParam::U, SamplerParam::V,
        SamplerParam::R, SamplerParam::OFFUVR, SamplerParam::MLOD);
  case 0x21:
    return SamplerMessageDescription(
        SendOp::SAMPLE_PO_B, "sample_po_b", 2, SamplerParam::BIAS_OFFUVR,
        SamplerParam::U, SamplerParam::V, SamplerParam::R, SamplerParam::MLOD);
  case 0x22:
    return SamplerMessageDescription(SendOp::SAMPLE_PO_L, "sample_po_l", 2,
                                     SamplerParam::LOD_OFFUV, SamplerParam::U,
                                     SamplerParam::R, SamplerParam::V);
  case 0x23:
    return SamplerMessageDescription(SendOp::SAMPLE_PO_C, "sample_po_c", 4,
                                     SamplerParam::REF, SamplerParam::U,
                                     SamplerParam::V, SamplerParam::OFFUV_R,
                                     SamplerParam::MLOD);
  case 0x24:
    return SamplerMessageDescription(
        SendOp::SAMPLE_PO_D, "sample_po_d", 7, SamplerParam::U,
        SamplerParam::DUDX, SamplerParam::DUDY, SamplerParam::V,
        SamplerParam::DVDX, SamplerParam::DVDY, SamplerParam::OFFUVR_R,
        SamplerParam::MLOD);
  // case 0x25: missing
  case 0x26:
    return SamplerMessageDescription(SendOp::SAMPLE_PO_L_C, "sample_po_l_c", 3,
                                     SamplerParam::REF, SamplerParam::LOD_OFFUV,
                                     SamplerParam::U, SamplerParam::V,
                                     SamplerParam::R);
  // case 0x27: missing
  case 0x28:
    return SamplerMessageDescription(SendOp::GATHER4_PO, "gather4_po", 4,
                                     SamplerParam::U, SamplerParam::V,
                                     SamplerParam::R, SamplerParam::OFFUV);
  // case 0x29..0x2C: missing
  case 0x2D:
    return SamplerMessageDescription(SendOp::GATHER4_PO_L, "gather4_po_l", 2,
                                     SamplerParam::LOD_OFFUV, SamplerParam::U,
                                     SamplerParam::V, SamplerParam::R);
  case 0x2E:
    return SamplerMessageDescription(SendOp::GATHER4_PO_B, "gather4_po_b", 2,
                                     SamplerParam::BIAS_OFFUV, SamplerParam::V,
                                     SamplerParam::R);
  case 0x2F:
    return SamplerMessageDescription(SendOp::GATHER4_PO_I, "gather4_po_i", 4,
                                     SamplerParam::U, SamplerParam::V,
                                     SamplerParam::R, SamplerParam::OFFUV);
  case 0x30:
    return SamplerMessageDescription(SendOp::GATHER4_PO_I_C, "gather4_po_c",
                                     4, SamplerParam::REF, SamplerParam::U,
                                     SamplerParam::V, SamplerParam::OFFUV_R);
  // case 0x31...0x34: missing
  case 0x35:
    return SamplerMessageDescription(SendOp::GATHER4_PO_I_C, "gather4_po_i_c",
                                     4, SamplerParam::REF, SamplerParam::U,
                                     SamplerParam::V, SamplerParam::OFFUV_R);
  // case 0x36: missing
  case 0x37:
    return SamplerMessageDescription(SendOp::GATHER4_PO_L_C, "gather4_po_l_c",
                                     3, SamplerParam::REF,
                                     SamplerParam::LOD_OFFUV, SamplerParam::U,
                                     SamplerParam::V, SamplerParam::R);
  case 0x38:
    return SamplerMessageDescription(SendOp::SAMPLE_PO_LZ, "sample_po_lz", 4,
                                     SamplerParam::U, SamplerParam::V,
                                     SamplerParam::R, SamplerParam::OFFUVR);
  case 0x39:
    return SamplerMessageDescription(SendOp::SAMPLE_PO_C_LZ, "sample_po_c_lz",
                                     4, SamplerParam::REF, SamplerParam::U,
                                     SamplerParam::V, SamplerParam::OFFUV_R);
  default:
    break;
  }
  return INVALID;
}


static void decodeSendMessage(uint32_t opBits, SamplerSIMD simdMode,
                              SendOp &sendOp, std::string &symbol,
                              std::string &desc, int &params) {
  if (simdMode != SamplerSIMD::SIMD32_64) {
    switch (opBits) {
    case 0x00:
      symbol = "sample";
      desc = "sample";
      params = 4;
      break;
    case 0x01:
      symbol = "sample_b";
      desc = "sample+LOD bias";
      params = 5;
      break;
    case 0x02:
      symbol = "sample_l";
      desc = "sample override LOD";
      params = 5;
      break;
    case 0x03:
      symbol = "sample_c";
      desc = "sample compare";
      params = 5;
      break;
    case 0x04:
      symbol = "sample_d";
      desc = "sample gradient";
      params = 10;
      break;
    case 0x05:
      symbol = "sample_b_c";
      desc = "sample compare+LOD bias";
      params = 6;
      break;
    case 0x06:
      symbol = "sample_l_c";
      desc = "sample compare+override LOD";
      params = 6;
      break;
    case 0x07:
      symbol = "sample_ld";
      desc = "sample load";
      params = 4;
      break;
    case 0x08:
      symbol = "sample_gather4";
      desc = "sample gather4";
      params = 4;
      break;
    case 0x09:
      symbol = "sample_lod";
      desc = "sample override lod";
      params = 4;
      break;
    case 0x0A:
      symbol = "sample_resinfo";
      desc = "sample res info";
      params = 1;
      break;
    case 0x0B:
      symbol = "sample_info";
      desc = "sample info";
      params = 0;
      break;
    case 0x0C:
      symbol = "sample_killpix";
      desc = "sample+killpix";
      params = 3;
      break;
    case 0x10:
      symbol = "sample_gather4_c";
      desc = "sample gather4+compare";
      params = 5;
      break;
    case 0x11:
      symbol = "sample_gather4_po";
      desc = "sample gather4+pixel offset";
      params = 5;
      break;
    case 0x12:
      symbol = "sample_gather4_po_c";
      desc = "sample gather4 pixel offset+compare";
      params = 6;
      break;
      // case 0x13: //skipped
    case 0x14:
      symbol = "sample_d_c";
      desc = "sample derivatives+compare";
      params = 11;
      break;
    case 0x16:
      symbol = "sample_min";
      desc = "sample min";
      params = 2;
      break;
    case 0x17:
      symbol = "sample_max";
      desc = "sample max";
      params = 2;
      break;
    case 0x18:
      symbol = "sample_lz";
      desc = "sample with lod forced to 0";
      params = 4;
      break;
    case 0x19:
      symbol = "sample_c_lz";
      desc = "sample compare+with lod forced to 0";
      params = 5;
      break;
    case 0x1A:
      symbol = "sample_ld_lz";
      desc = "sample load with lod forced to 0";
      params = 3;
      break;
      // case 0x1B:
    case 0x1C:
      symbol = "sample_ld2dms_w";
      desc = "sample ld2 multi-sample wide";
      params = 7;
      break;
    case 0x1D:
      symbol = "sample_ld_mcs";
      desc = "sample load mcs auxiliary data";
      params = 4;
      break;
    case 0x1E:
      symbol = "sample_ld2dms";
      desc = "sample load multi-sample";
      params = 6;
      break;
    case 0x1F:
      symbol = "sample_ld2ds";
      desc = "sample multi-sample without mcs";
      params = 6;
      break;
    default:
      symbol = std::to_string(opBits) + "?";
      desc = "?";
      break;
    }
  } else {
    switch (opBits) {
    case 0x00:
      symbol = "sample_unorm";
      desc = "sample unorm";
      params = 4; // relatively sure
      break;
    case 0x02:
      symbol = "sample_unorm_killpix";
      desc = "sample unorm+killpix";
      params = 4; // no idea???
      break;
    case 0x08:
      symbol = "sample_deinterlace";
      desc = "sample deinterlace";
      params = 4; // no idea???
      break;
    case 0x0C:
      // yes: this appears to be replicated
      symbol = "sample_unorm_media";
      desc = "sample unorm for media";
      params = 4; // no idea???
      break;
    case 0x0A:
      // yes: this appears to be replicated
      symbol = "sample_unorm_killpix_media";
      desc = "sample unorm+killpix for media";
      params = 4; // no idea???
      break;
    case 0x0B:
      symbol = "sample_8x8";
      desc = "sample 8x8";
      params = 4; // no idea???
      break;
    case 0x1F:
      symbol = "sample_flush";
      desc = "sampler cache flush";
      params = 0; // certain
      break;
    default:
      symbol = std::to_string(opBits) + "?";
      desc = "?";
      sendOp = SendOp::INVALID;
      break;
    }
  }
}

void MessageDecoderOther::tryDecodeSMPL() {
  setDoc("12484", "43860", "57022");
  std::stringstream syms, descs;

  auto simd2 = decodeDescBitField("SIMD[2]", 29, "", "");
  auto simd01 = getDescBits(17, 2);
  uint32_t simdBits = simd01 | (simd2 << 2);
  int simdSize = 0;
  const SamplerSIMD simdMode =
      decodeSimdSize(platform(), simdBits, simdSize, syms, descs);
  if (simdMode == SamplerSIMD::INVALID) {
    error(17, 2, "invalid SIMD mode");
  }
  addField("SIMD[1:0]", 17, 2, simd01, descs.str());

  bool is16bData = decodeDescBitField("ReturnFormat", 30, "32b", "16b") != 0;
  if (is16bData) {
    syms << "_16";
    descs << " 16b";
  }
  syms << "_";
  descs << " ";

  SendOp sendOp = SendOp::SAMPLE;
  int params = 0;
  std::string symbol, desc;
  auto opBits = getDescBits(12, 5);
  // check if bit 31 is set for sampler messages with positional offsets
  opBits |= getDescBits(31, 1) << 5;
  if (platform() >= Platform::XE2) {
    auto msg = lookupSamplerMessageInfoXe2(opBits, simdMode);
    (void)addField("SamplerMessageType", 12, 5, opBits, msg.mnemonic);
    desc = msg.describe(-1);
    params = msg.countParams();
  } else {
    decodeSendMessage(opBits, simdMode, sendOp, symbol, desc, params);
  }
  syms << symbol;

  (void)addField("SamplerMessageType", 12, 5, opBits, desc);
  descs << desc;

  auto six = decodeDescField(
      "SamplerIndex", 8, 4,
      [&](std::stringstream &ss, uint32_t six) { ss << "sampler " << six; });
  descs << " using sampler index " << six;
  syms << "[" << six << "," << decodeBTI(32) << "]";

  setScatterGatherOp(syms.str(), descs.str(), sendOp, AddrType::INVALID,
                     getDescBits(0, 8), 32, is16bData ? 16 : 32, params,
                     simdSize, MessageInfo::Attr::NONE);
  decodeMDC_H(); // header is optional in the sampler
}

void MessageDecoderOther::tryDecodeTS() {
  if (getDescBits(0, 3) == 0) {
    setSpecialOpX("eot", "end of thread", SendOp::EOT, AddrType::FLAT, 0,
                  1, // mlen
                  0, // rlen
                  MessageInfo::Attr::NONE);
  } else {
    error(0, 32, "unsupported TS op");
  }
}

void MessageDecoderOther::tryDecodeURB() {
  std::stringstream sym, descs;
  SendOp op;
  int simd = DEFAULT_EXEC_SIZE / 2;
  int addrSize = 32;
  int dataSize = 32; // MH_URB_HANDLE has this an array of MHC_URB_HANDLE
  auto opBits = getDescBits(0, 4);          // [3:0]
  auto chMaskPresent = getDescBit(15) != 0; // [15]
  auto decodeGUO = [&]() {
    return decodeDescField("GlobalUrbOffset", 4, 11,
                           [&](std::stringstream &ss, uint32_t val) {
                             ss << val << " (in owords)";
                           }); // [14:4]
  };
  auto decodePSO = [&]() {
    return decodeDescBitField("PerSlotOffsetPresent", 17,
                              "per-slot offset in payload") != 0;
  };
  int rlen = getDescBits(20, 5);
  // int mlen = getDescBits(25,4);
  int xlen = getDescBits(32 + 6, 5);
  int elemsPerAddr = 1;
  int off = 0;
  switch (opBits) {
  case 7:
    op = SendOp::STORE;
    off = 8 * decodeGUO();
    addField("ChannelMaskEnable", 15, 1, getDescBit(15),
             chMaskPresent ? "enabled" : "disabled");
    sym << "MSDUW_DWS";
    descs << "urb dword ";
    if (chMaskPresent)
      descs << "masked ";
    descs << "write";
    if (decodePSO())
      descs << " with per-slot offset enabled";
    // "SIMD8 URB Dword Read message. Reads 1..8 Dwords, based on RLEN."
    elemsPerAddr = xlen != 0 ? xlen : 1;
    setDoc(chMaskPresent ? "44779" : "44778");
    decodeMDC_HR();
    break;
  case 8:
    op = SendOp::LOAD;
    off = 8 * decodeGUO();
    sym << "MSDUR_DWS";
    descs << "urb dword read";
    if (decodePSO())
      descs << " with per-slot offset enabled";
    elemsPerAddr = rlen;
    setDoc("44777");
    decodeMDC_HR();
    break;
  case 9:
    op = SendOp::FENCE;
    sym << "MSD_URBFENCE";
    descs << "urb fence";
    simd = 1;
    dataSize = 0;
    setDoc("53422");
    break;
  default:
    error(0, 4, "unsupported URB op");
    return;
  }
  addField("URBOpcode", 0, 4, opBits, sym.str() + " (" + descs.str() + ")");

  setScatterGatherOp(sym.str(), descs.str(), op, AddrType::FLAT, 0, addrSize,
                     dataSize, elemsPerAddr, simd, MessageInfo::Attr::NONE);
  if (opBits == 7 || opBits == 8)
    result.info.immediateOffset = off;
}

///////////////////////////////////////////////////////////////////////////////
void iga::decodeDescriptorsOther(Platform platform, SFID sfid,
                                 ExecSize _execSize, uint32_t exImmOffDesc,
                                 SendDesc exDesc, SendDesc desc,
                                 DecodeResult &result) {
  MessageDecoderOther mdo(platform, sfid, _execSize, exImmOffDesc, exDesc, desc,
                          result);
  mdo.tryDecodeOther();
}
