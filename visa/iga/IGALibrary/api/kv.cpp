/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*
 * The IGA - KernelView API
 */
#include "kv.h"

#include "../Backend/GED/Decoder.hpp"
#include "../Backend/GED/GEDUtil.hpp"
#include "../Frontend/Formatter.hpp"
#include "../IR/Block.hpp"
#include "../IR/Messages.hpp"
#include "../strings.hpp"

#include <mutex>
#include <sstream>

///////////////////////////////////////////////////////////////////////////////
//
// KernelView binary interface
//
///////////////////////////////////////////////////////////////////////////////
using namespace iga;

class KernelViewImpl {
private:
  KernelViewImpl(const KernelViewImpl &k)
      : m_model(*Model::LookupModel(k.m_model.platform)) {}
  KernelViewImpl& operator=(const KernelViewImpl&) = delete;

public:
  const Model &m_model;
  Kernel *m_kernel = nullptr;
  ErrorHandler m_errHandler;
  std::map<uint32_t, const Instruction *> m_instsByPc;
  std::map<uint32_t, const Block *> m_blockToPcMap;
  DepAnalysis *m_liveAnalysis = nullptr;

  KernelViewImpl(const Model &model, const void *bytes, size_t bytesLength,
                 SWSB_ENCODE_MODE swsbOverride)
      : m_model(model)

  {
    Decoder decoder(model, m_errHandler);
    if (swsbOverride != SWSB_ENCODE_MODE::SWSBInvalidMode) {
      decoder.setSWSBEncodingMode(swsbOverride);
    }
    m_kernel = decoder.decodeKernelBlocks(bytes, bytesLength);

    for (const Block *b : m_kernel->getBlockList()) {
      m_blockToPcMap[b->getPC()] = b;
      for (const Instruction *inst : b->getInstList()) {
        m_instsByPc[inst->getPC()] = inst;
      }
    }
  }

  ~KernelViewImpl() {
    if (m_liveAnalysis) {
      delete m_liveAnalysis;
    }
    if (m_kernel) {
      delete m_kernel;
    }
  }

  const Instruction *getInstruction(int32_t pc) const {
    auto itr = m_instsByPc.find(pc);
    if (itr == m_instsByPc.end()) {
      return nullptr;
    }
    return itr->second;
  }

  const Block *getBlock(int32_t pc) const {
    auto itr = m_blockToPcMap.find(pc);
    if (itr == m_blockToPcMap.end()) {
      return nullptr;
    }
    return itr->second;
  }
};

kv_t *kv_create(iga_gen_t gen_platf, const void *bytes, size_t bytes_len,
                iga_status_t *status, char *errbuf, size_t errbuf_cap,
                uint32_t swsbMode) {
  if (errbuf && errbuf_cap > 0)
    *errbuf = 0;

  Platform p = ToPlatform(gen_platf);
  const Model *model = Model::LookupModel(p);
  if (model == nullptr) {
    if (status)
      *status = IGA_UNSUPPORTED_PLATFORM;
    if (errbuf) {
      iga::copyOutString(errbuf, errbuf_cap, nullptr, "unsupported platform");
    }
    return nullptr;
  }

  KernelViewImpl *kvImpl = nullptr;
  try {
    kvImpl =
        new (std::nothrow) KernelViewImpl(*model, bytes, bytes_len, static_cast<iga::SWSB_ENCODE_MODE>(swsbMode));
    if (!kvImpl) {
      if (errbuf) {
        iga::copyOutString(errbuf, errbuf_cap, nullptr, "failed to allocate");
      }
      if (status)
        *status = IGA_OUT_OF_MEM;
      return nullptr;
    }
  } catch (const FatalError &fe) {
    if (errbuf) {
      std::string msgS = iga::format("decoding error: ", fe.what());
      iga::copyOutString(errbuf, errbuf_cap, nullptr, msgS.c_str());
    }
    if (status)
      *status = IGA_DECODE_ERROR;
    if (kvImpl) {
      delete kvImpl;
    }
    return nullptr;
  }

  // copy out the errors and warnings
  if (kvImpl) {
    std::stringstream ss;
    auto fmtDiag = [&](const char *what, const Diagnostic &d) {
      ss << what << ": PC[0x" << iga::hex(d.at.offset) << "] " << d.message
         << "\n";
    };
    if (kvImpl->m_errHandler.hasErrors()) {
      for (const auto& d : kvImpl->m_errHandler.getErrors()) {
        fmtDiag("ERROR", d);
      }
    }
    for (const auto& d : kvImpl->m_errHandler.getWarnings()) {
      fmtDiag("WARNING", d);
    }
    (void)copyOut(errbuf, errbuf_cap, ss);
  }

  if (kvImpl->m_errHandler.hasErrors()) {
    // copy out the error status
    if (status)
      *status = IGA_DECODE_ERROR;
  } else {
    // copy out the error status
    if (status)
      *status = IGA_SUCCESS;
  }

  return (kv_t *)kvImpl;
}

void kv_delete(kv_t *kv) {
  if (kv)
    delete ((KernelViewImpl *)kv);
}

int32_t kv_get_inst_size(const kv_t *kv, int32_t pc) {
  if (!kv)
    return 0;

  const Instruction *inst = ((KernelViewImpl *)kv)->getInstruction(pc);
  if (!inst) {
    return 0;
  }
  return inst->hasInstOpt(InstOpt::COMPACTED) ? 8 : 16;
}

kv_status_t kv_get_inst_msg_info(const kv_t *kv, int32_t pc, bool *isAtomic,
                                 bool *isSlm, bool *isScratch) {
    if (!kv) {
        return KV_ERROR;
    }

    const Instruction* inst = ((KernelViewImpl*)kv)->getInstruction(pc);
    if (!inst) {
        return KV_INVALID_PC;
    }
    const DecodeResult di = tryDecode(*inst, nullptr);
    if (!di) {
        return KV_DECODE_ERROR;
    }

    *isSlm = di.info.hasAttr(iga::MessageInfo::Attr::SLM);
    *isScratch = di.info.hasAttr(iga::MessageInfo::Attr::SCRATCH);
    *isAtomic = di.info.isAtomic();

    return KV_SUCCESS;
}

bool kv_has_inst_opt(const kv_t *kv, int32_t pc, uint32_t opt) {
  KernelViewImpl *kvImpl = (KernelViewImpl *)kv;
  const Instruction *inst = kvImpl->getInstruction(pc);
  if (!inst)
      return false;
  return inst->hasInstOpt((InstOpt)opt);
}

uint32_t kv_get_inst_targets(const kv_t *kv, int32_t pc, int32_t *pcs) {
  if (!kv)
    return 0;

  const Instruction *inst = ((KernelViewImpl *)kv)->getInstruction(pc);
  if (!inst || inst->getOp() == Op::ILLEGAL) {
    return 0;
  }

  if (!inst->getOpSpec().isBranching()) {
    return 0;
  }

  uint32_t nSrcs = 0;

  if (inst->getSourceCount() > 0) {
    const Operand &op = inst->getSource(SourceIndex::SRC0);
    if (op.getKind() == Operand::Kind::LABEL) {
      if (pcs)
        pcs[nSrcs] = inst->getJIP()->getPC();
      nSrcs++;
    }
  }

  if (inst->getSourceCount() > 1) {
    const Operand &op = inst->getSource(SourceIndex::SRC1);
    if (op.getKind() == Operand::Kind::LABEL) {
      if (pcs)
        pcs[nSrcs] = inst->getUIP()->getPC();
      nSrcs++;
    }
  }

  return nSrcs;
}

size_t kv_get_inst_syntax(const kv_t *kv, int32_t pc, char *sbuf,
                          size_t sbuf_cap, uint32_t fmt_opts,
                          const char *(*labeler)(int32_t, void *),
                          void *labeler_env) {
  if (!kv) {
    if (sbuf && sbuf_cap > 0)
      *sbuf = 0;
    return 0;
  }

  KernelViewImpl *kvImpl = (KernelViewImpl *)kv;
  const Instruction *inst = kvImpl->getInstruction(pc);
  if (!inst) {
    if (sbuf && sbuf_cap > 0)
      *sbuf = 0;
    return 0;
  }

  std::stringstream ss;
  FormatOpts fopts(kvImpl->m_model, labeler, labeler_env);
  if (fopts.printInstDefs) {
    static std::mutex m;
    const std::lock_guard<std::mutex> g(m);
    if (kvImpl->m_liveAnalysis == nullptr) {
      kvImpl->m_liveAnalysis = new (std::nothrow) DepAnalysis();
      *kvImpl->m_liveAnalysis = ComputeDepAnalysis(kvImpl->m_kernel);
    }
    fopts.liveAnalysis = kvImpl->m_liveAnalysis;
  }
  fopts.addApiOpts(fmt_opts);
  fopts.setSWSBEncodingMode(kvImpl->m_model.getSWSBEncodeMode());
  FormatInstruction(kvImpl->m_errHandler, ss, fopts, *inst);

  return copyOut(sbuf, sbuf_cap, ss);
}

size_t kv_get_default_label_name(int32_t pc, char *sbuf, size_t sbuf_cap) {
  if (!sbuf || sbuf_cap == 0) {
    return 0;
  }
  std::stringstream strm;
  GetDefaultLabelName(strm, pc);
  return copyOut(sbuf, sbuf_cap, strm);
}

uint32_t kv_is_inst_target(const kv_t *kv, int32_t pc) {
  if (!kv)
    return 0;
  return ((KernelViewImpl *)kv)->getBlock(pc) == nullptr ? 0 : 1;
}

int32_t kv_get_opgroup(const kv_t *kv, int32_t pc) {
  if (!kv)
    return (int32_t)kv_opgroup_t::KV_OPGROUP_INVALID;

  KernelViewImpl *kvImpl = (KernelViewImpl *)kv;
  const Instruction *inst = kvImpl->getInstruction(pc);
  if (!inst) {
    return (int32_t)kv_opgroup_t::KV_OPGROUP_INVALID;
  }
  switch (inst->getOp()) {
  case Op::IF:
    return (int32_t)kv_opgroup_t::KV_OPGROUP_IF;
  case Op::ENDIF:
    return (int32_t)kv_opgroup_t::KV_OPGROUP_ENDIF;
  case Op::ELSE:
    return (int32_t)kv_opgroup_t::KV_OPGROUP_ELSE;
  case Op::WHILE:
    return (int32_t)kv_opgroup_t::KV_OPGROUP_WHILE;
  default:
    if (inst->getOpSpec().isAnySendFormat() && inst->hasInstOpt(InstOpt::EOT)) {
      return (int32_t)kv_opgroup_t::KV_OPGROUP_SEND_EOT;
    } else {
      return (int32_t)kv_opgroup_t::KV_OPGROUP_OTHER;
    }
  }
}

uint32_t kv_get_send_descs(const kv_t *kv, int32_t pc, uint32_t *ex_desc,
                           uint32_t *desc) {
  if (!kv || !ex_desc || !desc)
    return 0;
  const Instruction *inst = ((KernelViewImpl *)kv)->getInstruction(pc);
  if (!inst || !inst->getOpSpec().isAnySendFormat()) {
    *ex_desc = *desc = KV_INVALID_SEND_DESC;
    return 0;
  }

  uint32_t n = 0;
      if (inst->getExtMsgDescriptor().isImm()) {
          n++;
          *ex_desc = inst->getExtMsgDescriptor().imm;
      }
      else {
          *ex_desc = KV_INVALID_SEND_DESC;
      }
      if (inst->getMsgDescriptor().isImm()) {
          n++;
          *desc = inst->getMsgDescriptor().imm;
      }
      else {
          *desc = KV_INVALID_SEND_DESC;
      }

  return n;
}

kv_status_t kv_get_send_exdesc_immoff(const kv_t *kv, int32_t pc,
                                      uint32_t *exdesc_immoff) {
  if (!kv || !exdesc_immoff)
    return KV_INVALID_ARGUMENT;
  const Instruction *inst = ((KernelViewImpl *)kv)->getInstruction(pc);
  if (!inst) {
    return KV_INVALID_ARGUMENT;
  } else if (!inst->getOpSpec().isAnySendFormat()) {
    return KV_NON_SEND_INSTRUCTION;
  } else if (inst->getSendFc() != SFID::UGM) {
    return KV_DESCRIPTOR_INVALID;
  }

  const auto desc = inst->getMsgDescriptor();

  auto addrType = ((desc.imm >> 29) & 0x3); // Desc[30:29]
  // when desc is reg, we don't know the addrType so are not able to do this
  // check
  if (desc.isImm() && addrType != 1 && addrType != 2)  // BSS/SS
    return KV_DESCRIPTOR_INVALID;

  *exdesc_immoff = inst->getExtImmOffDescriptor();
  return KV_SUCCESS;
}

kv_status_t kv_get_send_indirect_descs(const kv_t *kv, int32_t pc,
                                       uint8_t *ex_desc_reg,
                                       uint8_t *ex_desc_subreg,
                                       uint8_t *desc_reg,
                                       uint8_t *desc_subreg) {
  if (!kv || !ex_desc_reg || !ex_desc_subreg || !desc_reg || !desc_subreg)
    return KV_INVALID_ARGUMENT;

  const Instruction *inst = ((const KernelViewImpl *)kv)->getInstruction(pc);
  if (!inst) {
    return KV_INVALID_PC;
  } else if (!inst->getOpSpec().isAnySendFormat()) {
    return KV_NON_SEND_INSTRUCTION;
  }
  if (inst->getExtMsgDescriptor().isReg()) {
    *ex_desc_reg = (uint8_t)inst->getExtMsgDescriptor().reg.regNum;
    *ex_desc_subreg = (uint8_t)inst->getExtMsgDescriptor().reg.subRegNum;
  } else {
    *ex_desc_reg = *ex_desc_subreg = KV_INVALID_REG;
  }

  if (inst->getMsgDescriptor().isReg()) {
    *desc_reg = (uint8_t)inst->getMsgDescriptor().reg.regNum;
    *desc_subreg = (uint8_t)inst->getMsgDescriptor().reg.subRegNum;
  } else {
    *desc_reg = *desc_subreg = KV_INVALID_REG;
  }
  return KV_SUCCESS;
}


/******************** KernelView analysis APIs *******************************/
static const Instruction *getInstruction(const kv_t *kv, int32_t pc) {
  if (!kv) {
    return nullptr;
  }
  const Instruction *inst = ((KernelViewImpl *)kv)->getInstruction(pc);
  return inst;
}

kv_status_t kv_get_send_exbso(const kv_t *kv, int32_t pc, int32_t *exbso) {
  if (!kv || !exbso) {
    return kv_status_t::KV_INVALID_ARGUMENT;
  }

  *exbso = -1;

  Platform p = ((KernelViewImpl *)kv)->m_model.platform;
  if (p < Platform::XE_HP)
    return kv_status_t::KV_INCAPABLE_PLATFORM;

  const Instruction *inst = getInstruction(kv, pc);
  if (!inst) {
    return kv_status_t::KV_INVALID_PC;
  } else if (!inst || !inst->getOpSpec().isAnySendFormat()) {
    return kv_status_t::KV_NON_SEND_INSTRUCTION;
  }

  if (inst->hasInstOpt(InstOpt::EXBSO))
    *exbso = 1;
  else
    *exbso = 0;
  return kv_status_t::KV_SUCCESS;
}

kv_status_t kv_get_message_type(const kv_t *kv, int32_t pc,
                                int32_t *message_type_enum) {
  if (!kv || !message_type_enum) {
    return kv_status_t::KV_INVALID_ARGUMENT;
  }

  const Instruction *inst = getInstruction(kv, pc);
  if (!inst) {
    return kv_status_t::KV_INVALID_PC;
  } else if (!inst || !inst->getOpSpec().isAnySendFormat()) {
    return kv_status_t::KV_NON_SEND_INSTRUCTION;
  }

  auto desc = inst->getMsgDescriptor();

  // NOTE: we could probably get the message just from desc
  if (desc.isReg())
    return kv_status_t::KV_DESCRIPTOR_INDIRECT;

  Platform p = ((KernelViewImpl *)kv)->m_model.platform;
  SFMessageType msgType = getMessageType(p, inst->getSendFc(), desc.imm);
  *message_type_enum = static_cast<int32_t>(msgType);

  if (msgType == SFMessageType::INVALID)
    return kv_status_t::KV_DESCRIPTOR_INVALID;

  return kv_status_t::KV_SUCCESS;
}

kv_status_t kv_get_message_type_ext(const kv_t *kv, int32_t pc, uint32_t desc,
                                    int32_t sfid, int32_t *message_type_enum) {
  if (!kv || !message_type_enum) {
    return kv_status_t::KV_INVALID_ARGUMENT;
  }

  const Instruction *inst = getInstruction(kv, pc);
  if (!inst) {
    return kv_status_t::KV_INVALID_PC;
  } else if (!inst || !inst->getOpSpec().isAnySendFormat()) {
    return kv_status_t::KV_NON_SEND_INSTRUCTION;
  }

  Platform p = ((KernelViewImpl *)kv)->m_model.platform;
  SFMessageType msgType = getMessageType(p, (iga::SFID)sfid, desc);

  *message_type_enum = static_cast<int32_t>(msgType);

  if (msgType == SFMessageType::INVALID)
    return kv_status_t::KV_DESCRIPTOR_INVALID;

  return kv_status_t::KV_SUCCESS;
}

kv_status_t kv_get_message_sfid(const kv_t *kv, int32_t pc,
                                int32_t *sfid_enum) {
  if (!kv || !sfid_enum) {
    return kv_status_t::KV_INVALID_ARGUMENT;
  }

  const Instruction *inst = getInstruction(kv, pc);
  if (!inst) {
    return kv_status_t::KV_INVALID_PC;
  } else if (!inst || !inst->getOpSpec().isAnySendFormat()) {
    return kv_status_t::KV_NON_SEND_INSTRUCTION;
  }

  Platform p = ((KernelViewImpl *)kv)->m_model.platform;
  if (p < Platform::XE) {
    const auto exDesc = inst->getExtMsgDescriptor();

    // <TGL: SFID is ExDesc[3:0]; if it's in a0, we're sunk
    if (exDesc.isReg() && p < Platform::XE)
      return kv_status_t::KV_DESCRIPTOR_INDIRECT;
  }

  SFID sfid = inst->getSendFc();
  *sfid_enum = static_cast<int32_t>(sfid);

  if (sfid == SFID::INVALID)
    return kv_status_t::KV_DESCRIPTOR_INVALID;

  return kv_status_t::KV_SUCCESS;
}

uint32_t kv_get_message_len(const kv_t *kv, int32_t pc, uint32_t *mLen,
                            uint32_t *emLen, uint32_t *rLen) {
  if (!kv || !mLen || !emLen || !rLen)
    return 0;

  const Instruction *inst = getInstruction(kv, pc);
  if (!inst)
    return 0;
  else if (!inst->getOpSpec().isAnySendFormat())
    return 0;

  // set the values and count how many are valid
  uint32_t numSet = 0;
  //
  auto setOne = [&numSet](int n) {
    if (n < 0)
      return KV_INVALID_LEN;
    numSet++;
    return (uint32_t)n;
  };

  auto dstLen = inst->getDstLength();
  *rLen = setOne(dstLen);
  *mLen = setOne(inst->getSrc0Length());
  *emLen = setOne(inst->getSrc1Length());

  return numSet;
}

uint32_t kv_get_message_len_ext(const kv_t *kv, int32_t pc, uint32_t desc,
                                uint32_t exDesc, uint32_t *mLen,
                                uint32_t *emLen, uint32_t *rLen) {
  if (!mLen || !emLen || !rLen)
    return 0;

  const Instruction *inst = getInstruction(kv, pc);
  if (!inst) {
    return 0;
  }

  Platform p = ((KernelViewImpl *)kv)->m_model.platform;

  return getMessageLengths(p, inst->getOpSpec(), exDesc, desc, mLen, emLen,
                           rLen);
}

uint32_t kv_get_execution_size(const kv_t *kv, int32_t pc) {
  if (!kv) {
    return static_cast<uint32_t>(ExecSize::INVALID);
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst) {
    return static_cast<uint32_t>(ExecSize::INVALID);
  }

  return static_cast<uint32_t>(inst->getExecSize());
}

bool kv_get_swsb_info(const kv_t *kv, int32_t pc, SWSB_ENCODE_MODE,
                      SWSB &swsb) {
  if (!kv) {
    return false;
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst) {
    return false;
  }
  swsb = inst->getSWSB();
  return true;
}

int32_t kv_get_number_sources(const kv_t *kv, int32_t pc) {
  if (!kv) {
    return -1;
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst) {
    return -1;
  }

  return inst->getSourceCount();
}

uint32_t kv_get_opcode(const kv_t *kv, int32_t pc) {
  if (!kv) {
    return static_cast<uint32_t>(Op::INVALID);
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst) {
    return static_cast<uint32_t>(Op::INVALID);
  }
  return static_cast<uint32_t>(inst->getOpSpec().op);
}

kv_status_t kv_get_subfunction(const kv_t *kv, int32_t pc, uint32_t *subfunc) {
  *subfunc = static_cast<uint32_t>(InvalidFC::INVALID);

  if (!kv) {
    return KV_INVALID_ARGUMENT;
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst) {
    return KV_INVALID_PC;
  }

  // for send, get_message_sfid to support decoding SFID from exDesc
  if (inst->getOpSpec().isAnySendFormat()) {
    int32_t sfid = -1;
    kv_status_t st = kv_get_message_sfid(kv, pc, &sfid);
    *subfunc = static_cast<uint32_t>(sfid);
    return st;
  }

  *subfunc = inst->getSubfunction().bits;

  if (!inst->getSubfunction().isValid())
    return KV_NO_SUBFUNCTION;

  return KV_SUCCESS;
}

int32_t kv_get_has_destination(const kv_t *kv, int32_t pc) {
  if (!kv) {
    return -1;
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst || inst->getOp() == Op::ILLEGAL) {
    return -1;
  }

  const OpSpec &instSpec = inst->getOpSpec();
  return instSpec.supportsDestination() ? 1 : 0;
}

int32_t kv_get_destination_register(const kv_t *kv, int32_t pc) {
  if (!kv) {
    return -1;
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst) {
    return -1;
  }
  if (!inst->getOpSpec().supportsDestination()) {
    return -1;
  }
  const Operand &dst = inst->getDestination();
  if (dst.getKind() == Operand::Kind::DIRECT ||
      dst.getKind() == Operand::Kind::MACRO) {
    return dst.getDirRegRef().regNum;
  }

  if (dst.getKind() == Operand::Kind::INDIRECT) {
    return dst.getIndAddrReg().regNum;
  }
  return -1;
}

int32_t kv_get_destination_sub_register(const kv_t *kv, int32_t pc) {
  if (!kv) {
    return -1;
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst || inst->getOp() == Op::ILLEGAL) {
    return -1;
  }

  const OpSpec &instSpec = inst->getOpSpec();
  if (!instSpec.supportsDestination()) {
    return -1;
  }
  const Operand &dst = inst->getDestination();
  if (dst.getKind() == Operand::Kind::DIRECT) {
    return dst.getDirRegRef().subRegNum;
  }

  if (dst.getKind() == Operand::Kind::INDIRECT) {
    return dst.getIndAddrReg().subRegNum;
  }

  return -1;
}

uint32_t kv_get_destination_data_type(const kv_t *kv, int32_t pc) {
  if (!kv) {
    return static_cast<uint32_t>(Type::INVALID);
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst || inst->getOp() == Op::ILLEGAL) {
    return static_cast<uint32_t>(Type::INVALID);
  }
  return (int32_t)inst->getDestination().getType();
}

uint32_t kv_get_destination_register_type(const kv_t *kv, int32_t pc) {
  if (!kv) {
    return static_cast<uint32_t>(RegName::INVALID);
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst || inst->getOp() == Op::ILLEGAL) {
    return static_cast<uint32_t>(RegName::INVALID);
  }
  return static_cast<uint32_t>(inst->getDestination().getDirRegName());
}

uint32_t kv_get_destination_register_kind(const kv_t *kv, int32_t pc) {
  if (!kv) {
    return static_cast<uint32_t>(Kind::INVALID);
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst || inst->getOp() == Op::ILLEGAL) {
    return static_cast<uint32_t>(Kind::INVALID);
  }
  return (uint32_t)inst->getDestination().getKind();
}

int32_t kv_get_source_register(const kv_t *kv, int32_t pc,
                               uint32_t sourceNumber) {
  if (!kv) {
    return -1;
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst || sourceNumber >= inst->getSourceCount()) {
    return -1;
  }
  const auto &src = inst->getSource((size_t)sourceNumber);
  if (src.getKind() == Operand::Kind::DIRECT ||
      src.getKind() == Operand::Kind::MACRO) {
    return (int32_t)src.getDirRegRef().regNum;
  }

  if (src.getKind() == Operand::Kind::INDIRECT) {
    return (int32_t)src.getIndAddrReg().regNum;
  }

  return -1;
}

int32_t kv_get_source_sub_register(const kv_t *kv, int32_t pc,
                                   uint32_t sourceNumber) {
  if (!kv) {
    return -1;
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst || inst->getOp() == Op::ILLEGAL ||
      sourceNumber >= inst->getSourceCount()) {
    return -1;
  }
  const auto &src = inst->getSource((size_t)sourceNumber);
  if (src.getKind() == Operand::Kind::DIRECT) {
    return (int32_t)src.getDirRegRef().subRegNum;
  }

  if (src.getKind() == Operand::Kind::INDIRECT) {
    return (int32_t)src.getIndAddrReg().subRegNum;
  }
  return -1;
}

uint32_t kv_get_source_data_type(const kv_t *kv, int32_t pc,
                                 uint32_t sourceNumber) {
  if (!kv) {
    return static_cast<uint32_t>(Type::INVALID);
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst || sourceNumber >= inst->getSourceCount()) {
    return static_cast<uint32_t>(Type::INVALID);
  }
  const auto &src = inst->getSource((size_t)sourceNumber);
  if (src.getKind() == Operand::Kind::INVALID) {
    return static_cast<uint32_t>(Type::INVALID);
  }
  return static_cast<uint32_t>(src.getType());
}

uint32_t kv_get_source_register_type(const kv_t *kv, int32_t pc,
                                     uint32_t sourceNumber) {
  if (!kv) {
    return static_cast<uint32_t>(RegName::INVALID);
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst || sourceNumber >= inst->getSourceCount()) {
    return static_cast<uint32_t>(RegName::INVALID);
  }
  const auto &src = inst->getSource((size_t)sourceNumber);
  if (src.getKind() == Operand::Kind::INVALID) {
    return (uint32_t)-1;
  }
  return static_cast<uint32_t>(src.getDirRegName());
}

uint32_t kv_get_source_register_kind(const kv_t *kv, int32_t pc,
                                     uint32_t sourceNumber) {
  if (!kv) {
    return static_cast<uint32_t>(Kind::INVALID);
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst || sourceNumber >= inst->getSourceCount()) {
    return static_cast<uint32_t>(Kind::INVALID);
  }
  return static_cast<uint32_t>(
      inst->getSource((uint8_t)sourceNumber).getKind());
}

int32_t kv_is_source_vector(const kv_t *kv, int32_t pc, uint32_t sourceNumber) {
  if (!kv) {
    return -1;
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst || sourceNumber >= inst->getSourceCount()) {
    return -1;
  }

  const auto &src = inst->getSource((uint8_t)sourceNumber);
  if (src.getKind() != Operand::Kind::DIRECT &&
      src.getKind() != Operand::Kind::INDIRECT) {
    return -1;
  }

  auto rgn = src.getRegion();
  if (rgn == Region::SRC010 || rgn == Region::SRC0X0 || rgn == Region::SRCXX0) {
    return 0;
  }

  return 1;
}

uint32_t kv_get_channel_offset(const kv_t *kv, int32_t pc) {
  if (!kv) {
    return static_cast<uint32_t>(ChannelOffset::M0);
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst || inst->getOp() == Op::ILLEGAL) {
    return static_cast<uint32_t>(ChannelOffset::M0);
  }
  return (uint32_t)inst->getChannelOffset();
}

uint32_t kv_get_mask_control(const kv_t *kv, int32_t pc) {
  if (!kv) {
    return static_cast<uint32_t>(MaskCtrl::NORMAL);
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst || inst->getOp() == Op::ILLEGAL) {
    return static_cast<uint32_t>(MaskCtrl::NORMAL);
  }
  return (uint32_t)inst->getMaskCtrl();
}

int32_t kv_get_destination_region(const kv_t *kv, int32_t pc, uint32_t *hz) {
  uint32_t DstRgnHz = static_cast<uint32_t>(Region::Horz::HZ_INVALID);
  if (!kv) {
    *hz = DstRgnHz;
    return -1;
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst || !inst->getOpSpec().supportsDestination()) {
    *hz = DstRgnHz;
    return -1;
  }
  const Operand &dst = inst->getDestination();
  DstRgnHz = static_cast<uint32_t>(dst.getRegion().getHz());
  *hz = DstRgnHz;
  return 0;
}

int32_t kv_get_source_region(const kv_t *kv, int32_t pc, uint32_t src_op,
                             uint32_t *vt, uint32_t *wi, uint32_t *hz) {
  uint32_t SrcRgnVt = static_cast<uint32_t>(Region::Vert::VT_INVALID);
  uint32_t SrcRgnWi = static_cast<uint32_t>(Region::Width::WI_INVALID);
  uint32_t SrcRgnHz = static_cast<uint32_t>(Region::Horz::HZ_INVALID);
  const uint32_t c_maxSrcOperands = 3;
  if (!kv && src_op < c_maxSrcOperands) {
    *vt = SrcRgnVt;
    *wi = SrcRgnWi;
    *hz = SrcRgnHz;
    return -1;
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst || inst->getSourceCount() <= src_op) {
    *vt = SrcRgnVt;
    *wi = SrcRgnWi;
    *hz = SrcRgnHz;
    return -1;
  }
  const Operand &src = inst->getSource(src_op);
  if (!(src.getKind() == Operand::Kind::DIRECT ||
        src.getKind() == Operand::Kind::INDIRECT) ||
      !(src.getDirRegName() == RegName::GRF_R ||
        src.getDirRegName() == RegName::ARF_SR ||
        src.getDirRegName() == RegName::ARF_ACC)) {
    *vt = SrcRgnVt;
    *wi = SrcRgnWi;
    *hz = SrcRgnHz;
    return -1;
  }
  SrcRgnVt = static_cast<uint32_t>(src.getRegion().getVt());
  SrcRgnWi = static_cast<uint32_t>(src.getRegion().getWi());
  SrcRgnHz = static_cast<uint32_t>(src.getRegion().getHz());
  *vt = SrcRgnVt;
  *wi = SrcRgnWi;
  *hz = SrcRgnHz;
  return 0;
}

int32_t kv_get_source_immediate(const kv_t *kv, int32_t pc, uint32_t src_op,
                                uint64_t *imm) {
  if (!kv) {
    return -1;
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst || src_op >= inst->getSourceCount()) {
    return -1;
  }
  const auto &src = inst->getSource((size_t)src_op);
  if (src.getKind() != Operand::Kind::IMMEDIATE) {
    return -1;
  }
  *imm = src.getImmediateValue().u64;
  return 0;
}

int32_t kv_get_source_indirect_imm_off(const kv_t *kv, int32_t pc,
                                       uint32_t src_op, int16_t *immoff) {
  if (!kv) {
    return -1;
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst || src_op >= inst->getSourceCount()) {
    return -1;
  }
  const auto &src = inst->getSource((size_t)src_op);
  if (src.getKind() != Operand::Kind::INDIRECT) {
    return -1;
  }
  *immoff = src.getIndImmAddr();
  return 0;
}

int32_t kv_get_destination_indirect_imm_off(const kv_t *kv, int32_t pc,
                                            int16_t *immoff) {
  if (!kv) {
    return -1;
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst)
    return -1;

  if (!inst->getOpSpec().supportsDestination()) {
    return -1;
  }
  const Operand &dst = inst->getDestination();
  if (dst.getKind() != Operand::Kind::INDIRECT) {
    return -1;
  }

  *immoff = dst.getIndImmAddr();
  return 0;
}

static int16_t getMathMacroNum(MathMacroExt mme) {
  switch (mme) {
  case MathMacroExt::MME0:
    return 0;
  case MathMacroExt::MME1:
    return 1;
  case MathMacroExt::MME2:
    return 2;
  case MathMacroExt::MME3:
    return 3;
  case MathMacroExt::MME4:
    return 4;
  case MathMacroExt::MME5:
    return 5;
  case MathMacroExt::MME6:
    return 6;
  case MathMacroExt::MME7:
    return 7;
  case MathMacroExt::NOMME: // follow the encoding value, set NOMME to 8
    return 8;
  case MathMacroExt::INVALID:
    break;
  }
  return -1;
}

int32_t kv_get_source_mme_number(const kv_t *kv, int32_t pc, uint32_t src_op,
                                 int16_t *mme) {
  *mme = -1;
  if (!kv) {
    return -1;
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst || src_op >= inst->getSourceCount()) {
    return -1;
  }
  const auto &src = inst->getSource((size_t)src_op);

  *mme = getMathMacroNum(src.getMathMacroExt());
  if (*mme == -1)
    return -1;
  return 0;
}

int32_t kv_get_destination_mme_number(const kv_t *kv, int32_t pc,
                                      int16_t *mme) {
  *mme = -1;
  if (!kv)
    return -1;

  const Instruction *inst = getInstruction(kv, pc);
  if (!inst)
    return -1;

  if (!inst->getOpSpec().supportsDestination()) {
    return -1;
  }
  const Operand &dst = inst->getDestination();
  *mme = getMathMacroNum(dst.getMathMacroExt());
  if (*mme == -1)
    return -1;
  return 0;
}

uint32_t kv_get_flag_modifier(const kv_t *kv, int32_t pc) {
  if (!kv) {
    return static_cast<uint32_t>(FlagModifier::NONE);
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst || inst->getOp() == Op::ILLEGAL) {
    return static_cast<uint32_t>(FlagModifier::NONE);
  }
  return (uint32_t)inst->getFlagModifier();
}

uint32_t kv_get_source_modifier(const kv_t *kv, int32_t pc, uint32_t src_op) {
  if (!kv) {
    return static_cast<uint32_t>(SrcModifier::NONE);
  }
  const Instruction *inst = getInstruction(kv, pc);

  if (!inst || inst->getOp() == Op::ILLEGAL) {
    return static_cast<uint32_t>(SrcModifier::NONE);
  }
  if (!inst || src_op >= inst->getSourceCount()) {
    return static_cast<uint32_t>(SrcModifier::NONE);
  }
  const auto &src = inst->getSource((size_t)src_op);
  if (src.getKind() == Operand::Kind::IMMEDIATE) {
    return static_cast<uint32_t>(SrcModifier::NONE);
  }

  return (uint32_t)src.getSrcModifier();
}

uint32_t kv_get_destination_modifier(const kv_t *kv, int32_t pc) {
  if (!kv) {
    return static_cast<uint32_t>(DstModifier::NONE);
  }
  const Instruction *inst = getInstruction(kv, pc);

  if (!inst || inst->getOp() == Op::ILLEGAL) {
    return static_cast<uint32_t>(DstModifier::NONE);
  }

  const Operand &dst = inst->getDestination();

  return (uint32_t)dst.getDstModifier();
}

int32_t kv_get_flag_register(const kv_t *kv, int32_t pc) {
  if (!kv) {
    return -1;
  }
  const Instruction *inst = getInstruction(kv, pc);

  if (!inst || inst->getOp() == Op::ILLEGAL) {
    return -1;
  }

  return (int32_t)inst->getFlagReg().regNum;
}

int32_t kv_get_flag_sub_register(const kv_t *kv, int32_t pc) {
  if (!kv) {
    return -1;
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst || inst->getOp() == Op::ILLEGAL) {
    return -1;
  }

  return (int32_t)inst->getFlagReg().subRegNum;
}

uint32_t kv_get_predicate(const kv_t *kv, int32_t pc) {
  if (!kv) {
    return static_cast<uint32_t>(PredCtrl::NONE);
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst || inst->getOp() == Op::ILLEGAL) {
    return static_cast<uint32_t>(PredCtrl::NONE);
  }
  return (uint32_t)inst->getPredication().function;
}

uint32_t kv_get_is_inverse_predicate(const kv_t *kv, int32_t pc) {
  if (!kv) {
    return 0;
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst || inst->getOp() == Op::ILLEGAL) {
    return 0;
  }
  return (uint32_t)inst->getPredication().inverse;
}

kv_status_t kv_get_cache_opt(const kv_t *kv, int32_t pc, int32_t cache_level,
                             int32_t *cacheopt_enum) {
  if (!kv) {
    return kv_status_t::KV_INVALID_ARGUMENT;
  }

  const Instruction *inst = getInstruction(kv, pc);
  if (!inst) {
    return kv_status_t::KV_INVALID_PC;
  } else if (!inst->getOpSpec().isAnySendFormat()) {
    return kv_status_t::KV_NON_SEND_INSTRUCTION;
  }

  const DecodeResult di = tryDecode(*inst, nullptr);
  if (!di) {
    return kv_status_t::KV_DECODE_ERROR;
  }

  switch (static_cast<CacheLevel>(cache_level)) {
  case CacheLevel::L1:
    *cacheopt_enum = static_cast<int32_t>(di.info.cachingL1);
    break;
  case CacheLevel::L3:
    *cacheopt_enum = static_cast<int32_t>(di.info.cachingL3);
    break;
  default:
    return kv_status_t::KV_INVALID_ARGUMENT;
  }

  return kv_status_t::KV_SUCCESS;
}

int32_t kv_get_syncfc(const kv_t *kv, int32_t pc) {
  if (!kv) {
    return static_cast<int32_t>(iga::SyncFC::INVALID);
  }
  const Instruction *inst = getInstruction(kv, pc);
  if (!inst || inst->getOp() != iga::Op::SYNC) {
    return static_cast<int32_t>(iga::SyncFC::INVALID);
  }

  // for send, get_message_sfid to support decoding SFID from exDesc
  return static_cast<int32_t>(inst->getSyncFc());
}
