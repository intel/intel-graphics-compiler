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
/*
* The IGA - KernelView API
*/
#include "kv.h"

#include "../IR/Block.hpp"
#include "../Backend/GED/Decoder.hpp"
#include "../Backend/GED/GEDUtil.hpp"
#include "../Frontend/Formatter.hpp"
#include "../strings.hpp"

#include <sstream>


///////////////////////////////////////////////////////////////////////////////
//
// KernelView binary interface
//
///////////////////////////////////////////////////////////////////////////////
using namespace iga;

class KernelViewImpl {
private:
    KernelViewImpl(const KernelViewImpl& k)
        : m_model(*iga::Model::LookupModel(k.m_model.platform)) { }
    KernelViewImpl& operator =(const KernelViewImpl &k) { return *this; }
public:
    const iga::Model                       &m_model;
    iga::Kernel                            *m_kernel;
    iga::ErrorHandler                       m_errHandler;
    std::map<uint32_t,iga::Instruction*>    m_instsByPc;
    std::map<uint32_t, Block*>              m_blockToPcMap;

    KernelViewImpl(
        iga::Platform platf,
        const void *bytes,
        size_t bytesLength
    )
        : m_model(*iga::Model::LookupModel(platf))
        , m_kernel(nullptr)

    {
        iga::Decoder decoder(*Model::LookupModel(platf), m_errHandler);
        IGA_ASSERT(Model::LookupModel(platf) != nullptr, "Unsupported platform");
        m_kernel = decoder.decodeKernelBlocks(bytes, bytesLength);

        int32_t pc = 0;
        for (iga::Block *b : m_kernel->getBlockList()) {
            m_blockToPcMap[b->getPC()] = b;
            for (iga::Instruction *inst : b->getInstList()) {
                pc = inst->getPC();
                m_instsByPc[pc] = inst;
            }
        }
    }

    ~KernelViewImpl() {
        if (m_kernel) {
            delete m_kernel;
        }
    }


    const iga::Instruction *getInstruction(int32_t pc) const {
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

// iga.cpp
extern iga::Platform ToPlatform(iga_gen_t gen);

kv_t *kv_create(
    iga_gen_t gen_platf,
    const void *bytes,
    size_t bytes_len,
    iga_status_t *status,
    char *errbuf,
    size_t errbuf_cap
)
{
    if (errbuf && errbuf_cap > 0)
        *errbuf = 0;

    iga::Platform p = ToPlatform(gen_platf);
    if (p == iga::Platform::INVALID) {
        if (status)
            *status = IGA_UNSUPPORTED_PLATFORM;
        if (errbuf) {
            formatTo(errbuf, errbuf_cap, "%s", "iga api: unsupported platform");
        }
        return nullptr;
    }

    KernelViewImpl *kvImpl = nullptr;
    try {
        kvImpl = new (std::nothrow)KernelViewImpl(p, bytes, bytes_len
        );
        if (!kvImpl) {
            if (errbuf)
                formatTo(errbuf, errbuf_cap, "%s", "failed to allocate");
            if (status)
                *status = IGA_OUT_OF_MEM;
            return nullptr;
        }
    } catch (const iga::FatalError &fe) {
        if (errbuf) {
            const char *msg = fe.what();
            formatTo(errbuf, errbuf_cap, "decoding error: %s", msg);
        }
        if (status)
            *status = IGA_DECODE_ERROR;
        if (kvImpl)
        {
            delete kvImpl;
        }
        return nullptr;
    }

    // copy out the errors and warnings
    if (kvImpl) {
        std::stringstream ss;
        if (kvImpl->m_errHandler.hasErrors()) {
            for (auto d : kvImpl->m_errHandler.getErrors()) {
                ss << "ERROR: " << d.at.offset << ". " << d.message << "\n";
            }
        }
        for (auto d : kvImpl->m_errHandler.getWarnings()) {
            ss << "WARNING: " << d.at.offset << ". " << d.message << "\n";
        }
        (void)copyOut(errbuf, errbuf_cap, ss);
    }

    if (kvImpl->m_errHandler.hasErrors()) {
        /*
        if (kvImpl) {
        // free the KernelViewImpl since we are failing
        delete kvImpl;
        kvImpl = nullptr;
        }
        */
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


void kv_delete(kv_t *kv)
{
    if (kv)
        delete ((KernelViewImpl *)kv);
}


int32_t kv_get_inst_size(const kv_t *kv, int32_t pc)
{
    if (!kv)
        return 0;

    const iga::Instruction *inst = ((KernelViewImpl *)kv)->getInstruction(pc);
    if (!inst) {
        return 0;
    }
    return inst->hasInstOpt(iga::InstOpt::COMPACTED) ? 8 : 16;
}


bool kv_has_inst_opt(const kv_t *kv, int32_t pc, uint32_t opt)
{
    KernelViewImpl *kvImpl = (KernelViewImpl *)kv;
    const Instruction *inst = kvImpl->getInstruction(pc);
    return inst->hasInstOpt((iga::InstOpt)opt);
}

uint32_t kv_get_inst_targets(
    const kv_t *kv,
    int32_t pc,
    int32_t *pcs)
{
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


size_t kv_get_inst_syntax(
    const kv_t *kv,
    int32_t pc,
    char *sbuf,
    size_t sbuf_cap,
    const char *(*labeler)(int32_t, void *),
    void *labeler_env)
{
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
    FormatOpts fopts(kvImpl->m_model.platform, labeler, labeler_env);
    FormatInstruction(
        kvImpl->m_errHandler,
        ss,
        fopts,
        *inst);

    return copyOut(sbuf, sbuf_cap, ss);
}


size_t kv_get_default_label_name(
    int32_t pc,
    char *sbuf,
    size_t sbuf_cap)
{
    if (!sbuf || sbuf_cap == 0)
    {
        return 0;
    }
    std::stringstream strm;
    GetDefaultLabelName(strm, pc);
    return copyOut(sbuf, sbuf_cap, strm);
}


uint32_t kv_is_inst_target(const kv_t *kv, int32_t pc)
{
    if (!kv)
        return 0;
    return ((KernelViewImpl *)kv)->getBlock(pc) == nullptr ? 0 : 1;
}


int32_t kv_get_opgroup(const kv_t *kv, int32_t pc)
{
    if (!kv)
        return (int32_t)kv_opgroup_t::KV_OPGROUP_INVALID;

    KernelViewImpl *kvImpl = (KernelViewImpl *)kv;
    const Instruction *inst = kvImpl->getInstruction(pc);
    if (!inst) {
        return (int32_t)kv_opgroup_t::KV_OPGROUP_INVALID;
    }
    switch (inst->getOp()) {
    case Op::IF:    return (int32_t)kv_opgroup_t::KV_OPGROUP_IF;
    case Op::ENDIF: return (int32_t)kv_opgroup_t::KV_OPGROUP_ENDIF;
    case Op::ELSE:  return (int32_t)kv_opgroup_t::KV_OPGROUP_ELSE;
    case Op::WHILE: return (int32_t)kv_opgroup_t::KV_OPGROUP_WHILE;
    default:
        if (inst->getOpSpec().isSendOrSendsFamily() &&
            inst->hasInstOpt(InstOpt::EOT))
        {
            return (int32_t)kv_opgroup_t::KV_OPGROUP_SEND_EOT;
        } else {
            return (int32_t)kv_opgroup_t::KV_OPGROUP_OTHER;
        }
    }
}


uint32_t kv_get_send_descs(
    const kv_t *kv, int32_t pc, uint32_t *ex_desc, uint32_t *desc)
{
    if (!kv || !ex_desc || !desc)
        return 0;
    const Instruction *inst = ((KernelViewImpl *)kv)->getInstruction(pc);
    if (!inst || !inst->getOpSpec().isSendOrSendsFamily()) {
        *ex_desc = *desc = KV_INVALID_SEND_DESC;
        return 0;
    }

    uint32_t n = 0;
    if (inst->getExtMsgDescriptor().type == SendDescArg::IMM) {
        n++;
        *ex_desc = inst->getExtMsgDescriptor().imm;
    } else {
        *ex_desc = KV_INVALID_SEND_DESC;
    }
    if (inst->getMsgDescriptor().type == SendDescArg::IMM) {
        n++;
        *desc = inst->getMsgDescriptor().imm;
    } else {
        *desc = KV_INVALID_SEND_DESC;
    }
    return n;
}

void kv_get_send_indirect_descs(
    const kv_t *kv, int32_t pc, uint8_t *ex_desc_reg, uint8_t *ex_desc_subreg,
    uint8_t *desc_reg, uint8_t *desc_subreg)
{
    if (!kv || !ex_desc_reg || !ex_desc_subreg || !desc_reg || !desc_subreg)
        return;

    const Instruction *inst = ((KernelViewImpl *)kv)->getInstruction(pc);
    if (!inst || !inst->getOpSpec().isSendOrSendsFamily()) {
        *ex_desc_reg = *ex_desc_subreg = *desc_reg = *desc_subreg = KV_INVALID_REG;
        return;
    }

    if (inst->getExtMsgDescriptor().type == SendDescArg::REG32A) {
        *ex_desc_reg = inst->getExtMsgDescriptor().reg.regNum;
        *ex_desc_subreg = inst->getExtMsgDescriptor().reg.subRegNum;
    }
    else {
        *ex_desc_reg = *ex_desc_subreg = KV_INVALID_REG;
    }

    if (inst->getMsgDescriptor().type == SendDescArg::REG32A) {
        *desc_reg = inst->getMsgDescriptor().reg.regNum;
        *desc_subreg = inst->getMsgDescriptor().reg.subRegNum;
    }
    else {
        *desc_reg = *desc_subreg = KV_INVALID_REG;
    }
}

/******************** KernelView analysis APIs *******************************/
const Instruction *getInstruction(const kv_t *kv, int32_t pc)
{
    if (!kv) {
        return nullptr;
    }
    const Instruction *inst = ((KernelViewImpl *)kv)->getInstruction(pc);
    return inst;
}

kv_status_t kv_get_message_type(
    const kv_t *kv, int32_t pc, int32_t *message_type_enum)
{
    if (!kv || !message_type_enum) {
        return kv_status_t::KV_INVALID_ARGUMENT;
    }

    const Instruction *inst = getInstruction(kv, pc);
    if (!inst) {
        return kv_status_t::KV_INVALID_PC;
    } else if (!inst || !inst->getOpSpec().isSendOrSendsFamily()) {
        return kv_status_t::KV_NON_SEND_INSTRUCTION;
    }

    auto exDesc = inst->getExtMsgDescriptor();
    auto desc = inst->getMsgDescriptor();

    // NOTE: we could probably get the message just from desc
    if (desc.type != SendDescArg::IMM)
        return kv_status_t::KV_DESCRIPTOR_INDIRECT;

    Platform p = ((KernelViewImpl *)kv)->m_model.platform;
    SFMessageType msgType =
        getMessageType(p, inst->getOpSpec(), exDesc.imm, desc.imm);
    *message_type_enum = static_cast<int32_t>(msgType);

    if (msgType == SFMessageType::INVALID)
        return kv_status_t::KV_DESCRIPTOR_INVALID;

    return kv_status_t::KV_SUCCESS;
}

kv_status_t kv_get_message_sfid(const kv_t *kv, int32_t pc, int32_t *sfid_enum)
{
    if (!kv || !sfid_enum) {
        return kv_status_t::KV_INVALID_ARGUMENT;
    }

    const Instruction *inst = getInstruction(kv, pc);
    if (!inst) {
        return kv_status_t::KV_INVALID_PC;
    } else if (!inst || !inst->getOpSpec().isSendOrSendsFamily()) {
        return kv_status_t::KV_NON_SEND_INSTRUCTION;
    }

    auto exDesc = inst->getExtMsgDescriptor();
    if (exDesc.type != SendDescArg::IMM)
        return kv_status_t::KV_DESCRIPTOR_INDIRECT;

    Platform p = ((KernelViewImpl *)kv)->m_model.platform;
    SFID sfid = getSFID(p, inst->getOpSpec(), exDesc.imm, 0);
    *sfid_enum = static_cast<int32_t>(sfid);

    if (sfid == SFID::INVALID)
        return kv_status_t::KV_DESCRIPTOR_INVALID;

    return kv_status_t::KV_SUCCESS;
}

uint32_t kv_get_message_len(
    const kv_t *kv, int32_t pc, uint32_t* mLen, uint32_t* emLen, uint32_t* rLen)
{
    if (!mLen || !emLen || !rLen)
        return 0;

    const Instruction *inst = getInstruction(kv, pc);
    if (!inst) {
        return 0;
    }

    auto exDesc = inst->getExtMsgDescriptor();
    auto desc = inst->getMsgDescriptor();

    // TODO: do I check for immediates?
    //if (exDesc.type != SendDescArg::IMM || desc.type != SendDescArg::IMM)
    //    return 0;

    Platform p = ((KernelViewImpl *)kv)->m_model.platform;

    return getMessageLengths(p, inst->getOpSpec(), exDesc.imm, desc.imm, mLen, emLen, rLen);
}

uint32_t kv_get_execution_size(const kv_t *kv, int32_t pc)
{
    if (!kv) {
        return static_cast<uint32_t>(ExecSize::INVALID);
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst) {
        return static_cast<uint32_t>(ExecSize::INVALID);
    }

    return static_cast<uint32_t>(inst->getExecSize());
}


int32_t kv_get_number_sources(const kv_t *kv, int32_t pc)
{
    if (!kv) {
        return -1;
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst) {
        return -1;
    }

    return inst->getSourceCount();
}

uint32_t kv_get_opcode(const kv_t *kv, int32_t pc)
{
    if (!kv) {
        return static_cast<uint32_t>(Op::INVALID);
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst) {
        return static_cast<uint32_t>(Op::INVALID);
    }
    return static_cast<uint32_t>(inst->getOpSpec().op);
    //    return static_cast<uint32_t>(inst->getOpSpec().code);
}

int32_t kv_get_has_destination(const kv_t *kv, int32_t pc)
{
    if (!kv) {
        return -1;
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || inst->getOp() == Op::ILLEGAL) {
        return -1;
    }

    const OpSpec& instSpec = inst->getOpSpec();
    return instSpec.supportsDestination() ? 1 : 0;
}

int32_t kv_get_destination_register(const kv_t *kv, int32_t pc)
{
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
    if (dst.getKind() == Operand::Kind::DIRECT) {
        return dst.getDirRegRef().regNum;
    }

    if (dst.getKind() == Operand::Kind::INDIRECT) {
        return dst.getIndAddrReg().regNum;
    }
    return -1;
}

int32_t kv_get_destination_sub_register(const kv_t *kv, int32_t pc)
{
    if (!kv) {
        return -1;
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || inst->getOp() == Op::ILLEGAL) {
        return -1;
    }

    const OpSpec& instSpec = inst->getOpSpec();
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

uint32_t kv_get_destination_data_type(const kv_t *kv, int32_t pc)
{
    if (!kv) {
        return static_cast<uint32_t>(Type::INVALID);
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || inst->getOp() == Op::ILLEGAL) {
        return static_cast<uint32_t>(Type::INVALID);
    }
    return (int32_t)inst->getDestination().getType();
}

uint32_t kv_get_destination_register_type(const kv_t *kv, int32_t pc)
{
    if (!kv) {
        return static_cast<uint32_t>(RegName::INVALID);
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || inst->getOp() == Op::ILLEGAL) {
        return static_cast<uint32_t>(RegName::INVALID);
    }
    return static_cast<uint32_t>(inst->getDestination().getDirRegName());
}

uint32_t kv_get_destination_register_kind(const kv_t *kv, int32_t pc)
{
    if (!kv) {
        return static_cast<uint32_t>(Kind::INVALID);
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || inst->getOp() == Op::ILLEGAL) {
        return static_cast<uint32_t>(Kind::INVALID);
    }
    return (uint32_t)inst->getDestination().getKind();
}

int32_t kv_get_source_register(const kv_t *kv, int32_t pc, uint32_t sourceNumber)
{
    if (!kv) {
        return -1;
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || sourceNumber >= inst->getSourceCount()) {
        return -1;
    }
    const auto &src = inst->getSource((size_t)sourceNumber);
    if (src.getKind() == Operand::Kind::DIRECT) {
        return (int32_t)src.getDirRegRef().regNum;
    }

    if (src.getKind() == Operand::Kind::INDIRECT) {
        return (int32_t)src.getIndAddrReg().regNum;
    }

    return -1;
}

int32_t kv_get_source_sub_register(const kv_t *kv, int32_t pc, uint32_t sourceNumber)
{
    if (!kv) {
        return -1;
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || inst->getOp() == Op::ILLEGAL ||
        sourceNumber >= inst->getSourceCount())
    {
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

uint32_t kv_get_source_data_type(const kv_t *kv, int32_t pc, uint32_t sourceNumber)
{
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

uint32_t kv_get_source_register_type(const kv_t *kv, int32_t pc, uint32_t sourceNumber)
{
    if (!kv) {
        return static_cast<uint32_t>(RegName::INVALID);
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || sourceNumber >= inst->getSourceCount()) {
        return static_cast<uint32_t>(RegName::INVALID);
    }
    const auto &src = inst->getSource((size_t)sourceNumber);
    if (src.getKind() == Operand::Kind::INVALID) {
        return -1;
    }
    return static_cast<uint32_t>(src.getDirRegName());
}

uint32_t kv_get_source_register_kind(const kv_t *kv, int32_t pc, uint32_t sourceNumber)
{
    if (!kv) {
        return static_cast<uint32_t>(Kind::INVALID);
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || sourceNumber >= inst->getSourceCount()) {
        return static_cast<uint32_t>(Kind::INVALID);
    }
    return static_cast<uint32_t>(inst->getSource((uint8_t)sourceNumber).getKind());
}

int32_t kv_is_source_vector(const kv_t *kv, int32_t pc, uint32_t sourceNumber)
{
    if (!kv) {
        return -1;
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || sourceNumber >= inst->getSourceCount()) {
        return -1;
    }

    auto src = inst->getSource((uint8_t)sourceNumber);
    if (src.getKind() != Operand::Kind::DIRECT ||
        src.getKind() != Operand::Kind::INDIRECT)
    {
        return -1;
    }

    auto rgn = src.getRegion();
    if (rgn == Region::SRC010 || rgn == Region::SRC0X0 || rgn == Region::SRCXX0) {
        return 0;
    }

    return 1;
}

uint32_t kv_get_channel_offset(const kv_t *kv, int32_t pc)
{
    if (!kv) {
        return static_cast<uint32_t>(ChannelOffset::M0);
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || inst->getOp() == Op::ILLEGAL) {
        return static_cast<uint32_t>(ChannelOffset::M0);
    }
    return (uint32_t)inst->getChannelOffset();
}

uint32_t kv_get_mask_control(const kv_t *kv, int32_t pc)
{
    if (!kv) {
        return static_cast<uint32_t>(MaskCtrl::NORMAL);
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || inst->getOp() == Op::ILLEGAL) {
        return static_cast<uint32_t>(MaskCtrl::NORMAL);
    }
    return (uint32_t)inst->getMaskCtrl();
}

int32_t kv_get_destination_region(const kv_t *kv, int32_t pc, uint32_t *hz)
{
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

int32_t kv_get_source_region(const kv_t *kv, int32_t pc, uint32_t src_op, uint32_t *vt, uint32_t *wi, uint32_t *hz)
{
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
    if (!inst ||
        !(inst->getOpSpec().getSourceCount() > src_op)) {
        *vt = SrcRgnVt;
        *wi = SrcRgnWi;
        *hz = SrcRgnHz;
        return -1;
    }
    const Operand &src = inst->getSource(src_op);
    if(!(src.getKind() == Operand::Kind::DIRECT || src.getKind() == Operand::Kind::INDIRECT) ||
        !(src.getDirRegName() == RegName::GRF_R)
        )
    {
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

int32_t kv_get_source_immediate(const kv_t *kv, int32_t pc, uint32_t src_op, uint64_t *imm)
{
    if(!kv) {
        return -1;
    }
    const Instruction *inst = getInstruction(kv, pc);
    if(!inst || src_op >= inst->getSourceCount()) {
        return -1;
    }
    const auto &src = inst->getSource((size_t)src_op);
    if(src.getKind() != Operand::Kind::IMMEDIATE) {
        return -1;
    }
    *imm = src.getImmediateValue().u64;
    return 0;
}

/*
* This function returns flag modifier for the instruction
* EQ, NE, etc...
*/
uint32_t kv_get_flag_modifier(const kv_t *kv, int32_t pc)
{
    if(!kv) {
        return static_cast<uint32_t>(FlagModifier::NONE);
    }
    const Instruction *inst = getInstruction(kv, pc);
    if(!inst || inst->getOp() == Op::ILLEGAL) {
        return static_cast<uint32_t>(FlagModifier::NONE);
    }
    return (uint32_t)inst->getFlagModifier();
}

/*
* This function returns source modifier for the operand
* can be ABS, NEG, NEG_ABS, NONE
*/
uint32_t kv_get_source_modifier(const kv_t *kv, int32_t pc, uint32_t src_op)
{
    if(!kv) {
        return static_cast<uint32_t>(SrcModifier::NONE);
    }
    const Instruction *inst = getInstruction(kv, pc);

    if(!inst || inst->getOp() == Op::ILLEGAL) {
        return static_cast<uint32_t>(SrcModifier::NONE);
    }
    if(!inst || src_op >= inst->getSourceCount()) {
        return static_cast<uint32_t>(SrcModifier::NONE);
    }
    const auto &src = inst->getSource((size_t)src_op);
    if(src.getKind() == Operand::Kind::IMMEDIATE){
        return static_cast<uint32_t>(SrcModifier::NONE);
    }

    return (uint32_t)src.getSrcModifier();
}

/*
* This function returns the destination modifier for a given instruction
* can be NONE or SAT
*/
uint32_t kv_get_destination_modifier(const kv_t *kv, int32_t pc)
{
    if(!kv) {
        return static_cast<uint32_t>(DstModifier::NONE);
    }
    const Instruction *inst = getInstruction(kv, pc);

    if(!inst || inst->getOp() == Op::ILLEGAL) {
        return static_cast<uint32_t>(DstModifier::NONE);
    }

    const Operand &dst = inst->getDestination();

    return (uint32_t)dst.getDstModifier();
}

/*
* This function returns the flag register used by instruction
*/
int32_t kv_get_flag_register(const kv_t *kv, int32_t pc)
{
    if(!kv) {
        return -1;
    }
    const Instruction *inst = getInstruction(kv, pc);

    if(!inst || inst->getOp() == Op::ILLEGAL) {
        return -1;
    }

    return (int32_t)inst->getFlagReg().regNum;
}

/*
* This function returns the flag sub-register used by instruction
*/
int32_t kv_get_flag_sub_register(const kv_t *kv, int32_t pc)
{
    if(!kv) {
        return -1;
    }
    const Instruction *inst = getInstruction(kv, pc);
    if(!inst || inst->getOp() == Op::ILLEGAL)
    {
        return -1;
    }

    return (int32_t)inst->getFlagReg().subRegNum;
}

/*
* This function returns the predicate function for a given instruction
*/
uint32_t kv_get_predicate(const kv_t *kv, int32_t pc)
{
    if(!kv) {
        return static_cast<uint32_t>(PredCtrl::NONE);
    }
    const Instruction *inst = getInstruction(kv, pc);
    if(!inst || inst->getOp() == Op::ILLEGAL)
    {
        return static_cast<uint32_t>(PredCtrl::NONE);
    }
    return (uint32_t)inst->getPredication().function;
}

/*
* This function returns if inverse predicate is set
*/
uint32_t kv_get_is_inverse_predicate(const kv_t *kv, int32_t pc)
{
    if(!kv) {
        return 0;
    }
    const Instruction *inst = getInstruction(kv, pc);
    if(!inst || inst->getOp() == Op::ILLEGAL)
    {
        return 0;
    }
    return (uint32_t)inst->getPredication().inverse;
}
