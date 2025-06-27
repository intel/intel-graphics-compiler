/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGA_INSTRUCTION_HPP_
#define _IGA_INSTRUCTION_HPP_

// WARNING: the IR is subject to change without any notice.  External tools
// should use the official interfaces in the external API.  Those interfaces
// are tested between releases and maintained even with changes to the IR.

#include "../MemManager/MemManager.hpp"

#include "../EnumBitset.hpp"
#include "../Models/Models.hpp"
#include "../asserts.hpp"
#include "Loc.hpp"
#include "Operand.hpp"
#include "Types.hpp"

#include <string>

namespace iga {
enum class SourceIndex {
  SRC0 = 0,
  SRC1 = 1,
  SRC2 = 2,
};

// represents a GEN instruction
class Instruction {
public:
  // inlineBinaryType - Inline binary value container
  // inlineBinaryType[0]: bit[127:96]
  // inlineBinaryType[1]: bit[95:64]
  // inlineBinaryType[2]: bit[63:32]
  // inlineBinaryType[3]: bit[31:0]
  typedef std::array<uint32_t, 4> InlineBinaryType;

public:
  Instruction(int id, PC pc, Loc loc, const OpSpec &os)
      : Instruction(os, ExecSize::INVALID, ChannelOffset::M0,
                    MaskCtrl::NORMAL) {
    setID(id);
    setLoc(loc);
    setPC(pc);
  }

  // TODO: phase this constructor out
  Instruction(const OpSpec &os, ExecSize execSize, ChannelOffset chOff,
              MaskCtrl mc)
      : m_opSpec(os), m_maskCtrl(mc), m_execSize(execSize), m_chOff(chOff) {}
  Instruction(const Instruction &) = delete;
  Instruction& operator=(const Instruction&) = delete;

  // for placement allocation
  void operator delete(void *, MemManager *) {}
  void *operator new(size_t sz, MemManager *m) { return m->alloc(sz); }

  ///////////////////////////////////////////////////////////////////////
  // operations that set instruction state
  // (these are kept roughly in syntax order;
  //  e.g. predication before instruction options)
  //
  void setLoc(const Loc &loc) { m_instLoc = loc; }
  void setPC(int32_t pc) { m_pc = pc; }
  void setID(int id) { m_instId = id; }
  void setMaskCtrl(MaskCtrl mc) { m_maskCtrl = mc; }
  void setExecSize(ExecSize es) { m_execSize = es; }
  void setChannelOffset(ChannelOffset co) { m_chOff = co; }
  void setFlagModifier(FlagModifier flagModifier) {
    m_flagModifier = flagModifier;
  }
  void setFlagReg(RegRef reg) { m_flagReg = reg; }
  void setPredication(const Predication &predOpnd) { m_pred = predOpnd; }

  void setSubfunction(Subfunction sf);

  void setDirectDestination(DstModifier dstMod, RegName r, RegRef reg,
                            Region::Horz rgn, Type type);
  void setMacroDestination(DstModifier dstMod, RegName r, RegRef reg,
                           MathMacroExt mme, Region::Horz rgn, Type type);
  void setIndirectDestination(DstModifier dstMod, RegRef addrReg,
                              int16_t addrImmOff, Region::Horz rgnH,
                              Type type);
  void setDirectSource(SourceIndex srcIx, SrcModifier srcMod, RegName rType,
                       RegRef reg, Region rgn, Type type);
  void setIndirectSource(SourceIndex srcIx, SrcModifier srcMod,
                         RegName regName, RegRef reg, int16_t m_immOffset,
                         Region rgn, Type type);
  void setMacroSource(SourceIndex srcIx, SrcModifier srcMod, RegName r,
                      RegRef reg, MathMacroExt acc, Region rgn, Type type);
  void setImmediateSource(SourceIndex srcIx, const ImmVal &val, Type type);
  void setLabelSource(SourceIndex srcIx, Block *jip, Type type = Type::INVALID);
  void setLabelSource(SourceIndex srcIx, int32_t pc, Type type = Type::INVALID);
  // e.g. pass Operand::NULL_UD_SRC
  void setSource(SourceIndex srcIx, const Operand &op);

  void setExtImmOffDesc(uint32_t imm);
  void setExtMsgDesc(const SendDesc &msg);
  void setMsgDesc(const SendDesc &msg);


  void setDstLength(int dstLength) { m_sendDstLength = dstLength; }
  void setSrc0Length(int src0Length) { m_sendSrc0Length = src0Length; }
  void setSrc1Length(int src1Length) { m_sendSrc1Length = src1Length; }

  void setSWSB(SWSB swsb) { m_depInfo = swsb; }

  void addInstOpt(const InstOpt &opt) { m_instOpts.add(opt); }
  void addInstOpts(const InstOptSet &opts) { m_instOpts.add(opts); }

  // associates an optional comment with instruction
  void setComment(const std::string& comment) { m_comment = comment; }

  ///////////////////////////////////////////////////////////////////////
  // name lacks get*** for consistency with other classes that all use
  // just "platform()"
  Platform platform() const { return m_opSpec.platform; }
  const Model &model() const;

  ///////////////////////////////////////////////////////////////////////
  // operations that get instruction state
  //
  // the source or binary location
  const Loc &getLoc() const { return m_instLoc; }
  int getID() const { return m_instId; }
  PC getPC() const { return m_pc; }

  // returns the instruction op specification
  const OpSpec &getOpSpec() const { return m_opSpec; }
  // opcode, math function control, and branch control
  Op getOp() const { return m_opSpec.op; }
  bool is(Op op) const { return m_opSpec.is(op); }

  // WrEn (NoMask)
  MaskCtrl getMaskCtrl() const { return m_maskCtrl; }
  // predication
  bool hasPredication() const { return m_pred.function != PredCtrl::NONE; }
  const Predication &getPredication() const { return m_pred; }

  ///////////////////////////////////////////////////////////
  // subfunction accessors
  Subfunction getSubfunction() const { return m_sf; }
  // specific subfunction accessors
  // TODO: tantrum (assert) if they call one on the wrong op?
  BranchCntrl getBranchCtrl() const { return m_sf.branch; }
  MathFC getMathFc() const { return m_sf.math; }
  SFID getSendFc() const { return m_sf.send; }
  SyncFC getSyncFc() const { return m_sf.sync; }
  BfnFC getBfnFc() const { return m_sf.bfn; }
  DpasFC getDpasFc() const { return m_sf.dpas; }

  // true for madm or math.invm and math.rsqrtm
  bool isMacro() const;
  // true if any of dst or src uses DF type
  bool isDF() const;
  // execution width info
  ExecSize getExecSize() const { return m_execSize; }
  ChannelOffset getChannelOffset() const { return m_chOff; }

  // returns if the instruction has a flag (condition) modifier
  bool hasFlagModifier() const { return m_flagModifier != FlagModifier::NONE; }
  FlagModifier getFlagModifier() const { return m_flagModifier; }

  // flag register is shared by predication and flag modfier
  const RegRef &getFlagReg() const { return m_flagReg; }

  const Operand &getDestination() const { return m_dst; }
  Operand &getDestination() { return m_dst; }
  const Operand &getSource(size_t srcNum) const { return m_srcs[srcNum]; }
  Operand &getSource(size_t srcNum) { return m_srcs[srcNum]; }
  const Operand &getSource(SourceIndex srcNum) const {
    return m_srcs[(int)srcNum];
  }
  Operand &getSource(SourceIndex srcNum) { return m_srcs[(int)srcNum]; }

  unsigned getSourceCount() const;

  uint32_t getExtImmOffDescriptor() const;
  SendDesc getExtMsgDescriptor() const;
  SendDesc getMsgDescriptor() const;


  // (For send messages) this returns the dst payload length in registers
  // as encoded by the descriptors (if known).
  int getDstLength() const { return m_sendDstLength; }
  // (For send messages) this returns the src0 payload length in registers
  // as encoded by the descriptors (if known).
  int getSrc0Length() const { return m_sendSrc0Length; }
  // (For send messages) this returns the src1 payload length in registers
  // as encoded by the descriptors (if known).
  int getSrc1Length() const { return m_sendSrc1Length; }

  const InstOptSet &getInstOpts() const { return m_instOpts; }
  bool hasInstOpt(InstOpt opt) const { return m_instOpts.contains(opt); }
  void removeInstOpt(InstOpt opt) { m_instOpts.remove(opt); }

  const Block *getJIP() const { return m_srcs[0].getTargetBlock(); }
  const Block *getUIP() const { return m_srcs[1].getTargetBlock(); }
  const std::string &getComment() const { return m_comment; }
  SWSB getSWSB() const { return m_depInfo; }
  bool isBranching() const { return getOpSpec().isBranching(); }

  bool isMovWithLabel() const;
  // check if an instruction is an indirect send
  bool isGatherSend() const;

  void setInlineBinary(const InlineBinaryType &binary);
  bool isInlineBinaryInstruction() const { return m_isInlineBinaryInst; }
  const InlineBinaryType &getInlineBinary() const { return m_inlineBinary; }

  SWSB::InstType getSWSBInstType(SWSB_ENCODE_MODE mode) const;

  void validate() const;   // asserts on malformed IR
  std::string str() const; // returns syntax of this instruction

private:
  const OpSpec &m_opSpec; // information about the specific inst op
  // SFID, e.g. MathFC::INV for math.inv, SFID::DC0 for send.dc0
  Subfunction m_sf = InvalidFC::INVALID;

  MaskCtrl m_maskCtrl; // (W) WrEn (write enable / NoMask)
  Predication m_pred;  // predication function (and logical sign)
  RegRef m_flagReg = REGREF_ZERO_ZERO; // shared by m_pred and m_flagModifier

  ExecSize m_execSize;
  ChannelOffset m_chOff;
  Operand m_dst;
  Operand m_srcs[3];

  // conditional-modifier function
  FlagModifier m_flagModifier = FlagModifier::NONE;

  // Xe2: given an reg ExDesc, this holds the extra bits for the
  // immediate offset part.  In the case of an immediate ExDesc
  // (below); this should be 0 (the immediate offset is part of m_desc
  // as in eralier platforms for that case).
  uint32_t m_exImmOffDesc = 0;
  SendDesc m_exDesc;
  SendDesc m_desc;


  ///////////////////////////////////////////////////////////////////////
  // Regarding Src1.Length
  //
  // For XeHP to XeHPC, we encode Src1.Length and CPS
  // sometimes we use the ExDesc a0.# reg for these fields and
  // sometimes they are encoded in the EU ISA bits (as immediate)
  //
  // +-----------+------------+-------------------------------------------
  // | Platform  | ExDesc ... | Src1.Length/CPS
  // +-----------+------------+-------------------------------------------
  // | XE        | ExDesc[10:6] always holds Src1.Length
  // |           | Imm.       | EU[103:99,35] (this::imm[11:6])
  // |           |      send ... (SRC1LEN<<6) ...
  // |           | Reg.       | ExDesc[11:6] (this::reg a0.#[11:6])
  // |           |      send ... a0.2 ...
  // +-----------+------------+-------------------------------------------
  // | XE_HP     | Sampler bindless accesses need more surface offset bits;
  // |           | thus ExDesc[11:6] are now low surface offest bits.
  // |           | Thus in that case, Src1.Length [10:6] and CPS [11]
  // |           | must be placed in the EU encoding.
  // |           +------------------------------------------------
  // |           | Imm.       | EU[103:99,35] (as this::imm[11:6])
  // |           |      send ... src1           (SRC1LEN<<6) ...
  // |           +------------------------------------------------
  // |           | Reg.       | Reg[11:6]
  // |           |      send ...  src1          a0.#   ...
  // |           +------------------------------------------------
  // |           | Reg.+ExBSO | EU[103:99,35] (as this::imm[11:6])
  // |           |      send ...  src1:SRC1LEN  a0.#   ... {[CPS]}
  // +-----------+------------+-------------------------------------------
  // | XE_HPG    | Src1.Length always comes from the EU ISA, regardless
  // |           | if a0.# is used.
  // |           | Imm.       | EU[103:99,35] (as this::src1Length,this::cps)
  // |           |      send ...  src1:SRC1LEN  imm  ... {[CPS]}
  // |           | Reg.+ExBSO | EU[103:99,35] (as this::imm[11:6])
  // |           |      send ...  src1:SRC1LEN  a0.#  ... {[CPS]}
  // +-----------+------------+-------------------------------------------
  // | XE_HPC    | Same as XE_HPG, but since the sampler is absent
  // |           | there won't be a use of ExBSO and CPS.
  // +-----------+------------+-------------------------------------------
  // | XE2       | Similar to XE_HPC/XE_HPG except
  // |           | CPS is gone (now ExDescImm[11])
  // |           | ExBSO is hardwired to 1, but the field persists.
  // |           | There is an ExDescImm available even with reg ExDesc
  // +-----------+--------------------------------------------------------
  // | XE3 Send  | Same as Xe2 regarding descriptors
  // +-----------+--------------------------------------------------------
  //

  // These fields are best-effort decodes of the message lengths
  // E.g. if the fields are buried in an immediate send descriptor,
  // extract them here.  In some cases (e.g. reg descriptors), we can't
  // help you.  Oppose architects that propose instruction definitions
  // that are taken form registers.
  int m_sendDstLength = -1;  // -1 if unknown
  int m_sendSrc0Length = -1; // -1 if unknown
  int m_sendSrc1Length = -1; // -1 if unknown

  InstOptSet m_instOpts; // miscellaneous instruction attributes
  SWSB m_depInfo;

  // Binary for inline binary instruction.
  InlineBinaryType m_inlineBinary = {0};
  bool m_isInlineBinaryInst = false;

  int m_instId = 0xFFFFFFFF; // unique id for this instruction
                             // (unique in the kernel)

  int32_t m_pc = 0; // the encode/decode PC for this instruction

  // source location info we keep this
  // separate from the PC since we
  // use both during assembly
  Loc m_instLoc = Loc::INVALID;

  // e.g. for illegal instruction extended info (e.g. decode errors)
  // enables us to emit things like
  //        illegal {Compacted} // failed to uncompact ....
  //                               ^^^^^^^^^^^^^^^^^^^
  std::string m_comment;
}; // class Instruction
} // namespace iga
#endif //_IGA_INSTRUCTION_HPP_
