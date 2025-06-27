/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Formatter.hpp"
#include "../ErrorHandler.hpp"
#include "../IR/Instruction.hpp"
#include "../IR/Messages.hpp"
#include "../IR/Types.hpp"
#include "../Models/Models.hpp"
#include "../strings.hpp"
#include "Floats.hpp"
#include "FormatterJSON.hpp"
#include "IRToString.hpp"
#include "SendDescriptorDecoding.hpp"
#include "ged.h"
#ifndef IGA_DISABLE_ENCODER_EXCEPTIONS
#include "../Backend/GED/Interface.hpp"
#include "../Backend/Native/Interface.hpp"
#endif
#include "../Backend/GED/IGAToGEDTranslation.hpp"
#include "../Backend/Native/MInst.hpp"
#include "../api/iga.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <vector>

namespace iga {

class Formatter : public BasicFormatter {
protected:
  ErrorHandler &errorHandler;
  const Model &model;
  const FormatOpts &opts;
  struct ColumnPreferences cols;
  const Instruction *currInst;
  const uint8_t *currInstBits = nullptr; // optional to render encoding inline

  ansi_esc ANSI_FADED;
  ansi_esc ANSI_REGISTER(RegName rnm) const {
    return rnm == RegName::GRF_R ? ANSI_REGISTER_GRF : ANSI_REGISTER_ARF;
  }
  ansi_esc ANSI_REGISTER_GRF;
  ansi_esc ANSI_REGISTER_ARF;
  ansi_esc ANSI_FLAGMODIFIER;
  ansi_esc ANSI_IMMEDIATE;
  ansi_esc ANSI_MNEMONIC;
  ansi_esc ANSI_SUBFUNCTION;
  ansi_esc ANSI_COMMENT;

  void formatDstOp(const Instruction &i);
  // prior platform used regular fmtDstOp with "implicit types"...
  void formatSendDstOp(const Instruction &i);

  void formatSrcOp(SourceIndex ix, const Instruction &i);

  void formatSyncAllSrc0(const Instruction &i);
  void formatBareRegisterUnescaped(RegName regName, int regNum);
  void formatRegister(RegName rnm, RegRef reg, bool emitSubReg = true,
                      bool isSIMT = false);
  void formatSendSrcWithLength(const Operand &src, int srcLen);
  void formatDstType(const OpSpec &os, Type type);
  void formatSrcBare(const Operand &src);

public:
  Formatter(ErrorHandler &err, std::ostream &out, const FormatOpts &fopts,
            const ColumnPreferences &colPrefs = ColumnPreferences())
      : BasicFormatter(fopts.printAnsi, out), errorHandler(err),
        model(fopts.model), opts(fopts), cols(colPrefs), currInst(nullptr) {
    // TODO: could make these mappable via environment variable
    //
    // export IGA_FormatAnsiRegisterArf="\033[38;2;138;43;211m"
    // export IGA_FormatAnsiMnemonic=...
    // export IGA_FormatAnsiComment=...
    //
    // (Put this in system.cpp; iga::LookupEnvVar(...))
    if (fopts.printAnsi) {
      ANSI_FADED = "\033[38;2;128;128;128m";
      ANSI_REGISTER_GRF = "\033[38;2;255;192;203m";
      ANSI_REGISTER_ARF = "\033[38;2;255;218;185m";
      ANSI_IMMEDIATE = "\033[38;2;221;160;221m";
      ANSI_FLAGMODIFIER = "\033[38;2;221;160;221m";
      ANSI_MNEMONIC = "\033[38;2;10;153;215m"; // Intel Blue
      ANSI_SUBFUNCTION = "\033[38;2;176;224;230m";
      ANSI_COMMENT = "\033[38;2;128;128;128m";
    }
  }

  Platform platform() const { return model.platform; }

  // labels are of the form "L" + off, but we prefix a
  // "_N" for negative.
  // E.g. "L64" is PC 64.
  // E.g. "L_N16" is -16 ("(N)egative")
  static void getDefaultLabelDefinition(std::ostream &o, int32_t pc) {
    o << "L";
    if (pc < 0) {
      o << "_N";
      pc = -pc;
    }
    o << pc;
  }

  void formatLabel(int32_t pc) {
    bool labeled = false;
    if (opts.labeler) {
      const char *userLabel = (*opts.labeler)(pc, opts.labelerContext);
      if (userLabel) {
        emit(userLabel);
        labeled = true;
      }
    }

    // use a default label if there isn't a labeler or the user callback
    // returned nullptr
    if (!labeled) {
      if (opts.numericLabels) {
        // Raw numbers for labels
        emitDecimal(pc);
      } else {
        Formatter::getDefaultLabelDefinition(o, pc);
      }
    }
  }

  void formatKernel(const Kernel &k, const void *vbits) {
    currInstBits = (const uint8_t *)vbits;
    if (opts.printInstDefs && opts.liveAnalysis) {
      std::stringstream ss;
      ss << "// itrs: " << opts.liveAnalysis->iterations << "\n";
      ss << "// live-in:\n";
      std::vector<const Dep *> liveIn;
      for (const Dep &d : opts.liveAnalysis->liveIn) {
        liveIn.push_back(&d);
      }
      std::sort(
          liveIn.begin(), liveIn.end(), [&](const Dep *d1, const Dep *d2) {
            if (d1->use->getID() < d2->use->getID())
              return true;
            else if (d1->def && d2->def && d1->def->getID() < d2->def->getID())
              return true;
            else
              return false;
          });
      for (const Dep *d : liveIn) {
        ss << "//   ";
        d->str(ss);
        ss << "\n";
      }
      emitAnsi(ANSI_FADED, ss.str());
    }

    for (const Block *b : k.getBlockList()) {
      if (!opts.numericLabels) {
        formatLabel(b->getPC());
        emit(':');
        newline();
      }

      formatBlockContents(*b);
    }
  }

  void formatBlockContents(const Block &b) {
    for (const auto &i : b.getInstList()) {
      formatInstruction(*i);
      newline();

      if (currInstBits) {
        currInstBits += i->hasInstOpt(InstOpt::COMPACTED) ? 8 : 16;
      }
    }
  }

  void formatPrefixComment(const Instruction &i, const void *vbits) {
    bool printBits = opts.printInstBits && currInstBits != nullptr;
    bool printInstId = opts.printInstDefs;
    bool printPc = opts.printInstPc;
    if (!printBits && !printInstId && !printPc) {
      return; // nothing to print
    }

    const uint32_t *bits = (const uint32_t *)vbits;
    emit(ANSI_COMMENT);
    emit("/* ");
    bool first = true;
    if (printInstId) {
      std::stringstream ss;
      ss << '#' << i.getID();
      o << std::right << std::setw(4) << ss.str();
      first = false;
    }

    if (printPc) {
      if (first) {
        first = false;
      } else {
        emit(" ");
      }
      emit("[");
      // emit PC. if basePCOffset is specified, PC is printed after adding
      // the offset specified in command line. By default, basePCOffset
      // is 0
      emitHexDigits<PC>(i.getPC() + opts.basePCOffset, 4);
      emit("] ");
    }

    if (printBits) {
      if (first) {
        first = false;
      } else {
        emit(" ");
      }
      if (i.hasInstOpt(InstOpt::COMPACTED)) {
        emit("        ");
        emit(' ');
        emit("        ");
        emit(' ');
      } else {
        emitHexDigits<uint32_t>(bits[3], 8);
        emit('`');
        emitHexDigits<uint32_t>(bits[2], 8);
        emit('`');
      }
      emitHexDigits<uint32_t>(bits[1], 8);
      emit('`');
      emitHexDigits<uint32_t>(bits[0], 8);
    }
    emit(" */ ");
    o << std::setfill(' ');
    o << std::dec;
    emit(ANSI_RESET);
  }

  void formatInlineBinaryInstruction(const Instruction &i) {
    emit(".inline_inst ");
    for (auto val : i.getInlineBinary()) {
      emitHex(val);
      emit(" ");
    }
  }

  // for single instruction
  void formatInstruction(const Instruction &i, const void *vbits) {
    currInstBits = (const uint8_t *)vbits;
    formatInstruction(i);
  }

  void formatInstruction(const Instruction &i) {
    currInst = &i;
    if (i.isInlineBinaryInstruction()) {
      formatInlineBinaryInstruction(i);
      currInst = nullptr;
      return;
    }

    if (opts.printInstDeps) {
      formatInstructionDeps(i);
    }
    formatPrefixComment(i, currInstBits);

    const bool isSend = i.is(Op::SEND); // not for sendc
    if (isSend) {
      bool sendPrinted = false;
      if (platform() >= Platform::XE_HPG && opts.printLdSt) {
        sendPrinted = formatLoadStoreSyntax(i);
      }
      if (!sendPrinted) {
        formatNormalInstructionBody(i, "");
      }
    } else {
      formatNormalInstructionBody(i, "");
    }

    currInst = nullptr;
  }

  void formatInstructionDeps(const Instruction &i) {
    bool first = true;
    auto emitSet = [&](const std::string& what, const RegSet &rs) {
      if (!rs.empty()) {
        if (first) {
          first = false;
          emitAnsi(ANSI_FADED, "// ");
        } else
          emitAnsi(ANSI_FADED, ", ");
        emitAnsi(ANSI_FADED, what, ":", rs.str());
      }
    };
    RegSet rsDst(model);
    rsDst.addExplicitDestinationOutputs(i);
    emitSet("d", rsDst);

    RegSet rsDfl(model);
    rsDfl.addFlagModifierOutputs(i);
    emitSet("d-fl", rsDfl);

    RegSet rsAcc(model);
    rsAcc.addDestinationImplicit(i);
    emitSet("d-impl", rsAcc);

    if (!first) {
      newline();
      first = true;
    }
    for (int ix = 0; ix < (int)i.getSourceCount(); ix++) {
      RegSet rs(model);
      rs.addSourceOperandInput(i, ix);
      std::stringstream ss;
      ss << "s" << ix;
      emitSet(ss.str(), rs);
    }

    RegSet rsPrS(model);
    rsPrS.addPredicationInputs(i);
    emitSet("s-pr", rsPrS);

    RegSet rsAccS(model);
    rsAccS.addSourceImplicit(i);
    emitSet("s-impl", rsAccS);

    RegSet rsDescS(model);
    rsDescS.addSourceDescriptorInputs(i);
    emitSet("s-desc", rsDescS);

    if (!first)
      newline();
  }

  bool formatLoadStoreSyntax(const Instruction &i);

  static bool useSyncAllSetSyntax(const Instruction &i) {
    return i.is(Op::SYNC) && (i.getSubfunction().sync == SyncFC::ALLRD ||
                              i.getSubfunction().sync == SyncFC::ALLWR);
  }

  void formatNormalInstructionBody(const Instruction &i,
                                   const std::string &debugSendDecode) {
    if (platform() == Platform::XE2 && i.getOpSpec().isAnySendFormat()) {
      formatInstructionBodySendXe2(i);
      return;
    }
    if (platform() >= Platform::XE3 && i.getOpSpec().isAnySendFormat()) {
      // Xe3 backwards compatible (non-ld/st/raw) send syntax
      //   send (..) dst  src0   src1:src1len  [exDescOff:]exDesc desc
      // Gather send
      //   send (..) dst  r[src0.x]            [exDescOff:]exDesc desc
      formatInstructionBodySendXe3(i);
      return;
    }

    formatInstructionPrefix(i);

    int nSrcs = i.getSourceCount();
    bool hasNonEmptyInstOpts = hasInstOptTokens(i);
    const OpSpec &os = i.getOpSpec();

    if (os.supportsDestination()) {
      // destination ops
      emit("  ");
      formatDstOp(i);
    } else {
      if (nSrcs > 0 || hasNonEmptyInstOpts) {
        // We only print this if there is something on the line following
        // this.  Checking this prevents useless spaces at the end of line.
        emitSpaces(cols.dstOp);
      }
    }

    // source ops
    if (nSrcs == 0) {
      // no source ops, but add spacing if we have instruction options
      // thus to prevent EOL spaces
      if (hasNonEmptyInstOpts) {
        emitSpaces(cols.srcOp);
      }
    } else {
      if (nSrcs >= 1) {
        emit("  ");
        if (useSyncAllSetSyntax(i)) {
          formatSyncAllSrc0(i);
        } else {
          formatSrcOp(SourceIndex::SRC0, i);
        }
      }
      if (nSrcs >= 2) {
        emit("  ");
        formatSrcOp(SourceIndex::SRC1, i);
      } else if (!os.isSendFormat()) {
        // ensure at least two columns worth of sources, with
        // an exception for send.  We poach those spaces for the
        // descriptors.
        startColumn(cols.srcOp);
        finishColumn();
      }
      if (nSrcs >= 3) {
        emit("  ");
        formatSrcOp(SourceIndex::SRC2, i);
      }
    }

    // send descriptors
    if (os.isAnySendFormat()) {
      emit("  ");
      formatSendDesc(i.getExtMsgDescriptor());
      emit("  ");
      formatSendDesc(i.getMsgDescriptor(), 8);
    }

    // instruction options
    std::vector<const char *> emptyExtraOpts;
    formatInstOpts(i, emptyExtraOpts);

    // EOL comments
    formatEolComments(i, debugSendDecode);
  }

private:
  // Xe2 send is same as earlier send + funny ExDescImm business
  void formatInstructionBodySendXe2(const Instruction &i) {
    formatInstructionPrefix(i);

    emit("  ");
    formatSendDstOp(i);

    emit("  ");
    if (i.isGatherSend()) {
      formatRegIndRef(i.getSource(SourceIndex::SRC0));
      emit(ANSI_RESET);
    } else
      formatSrcBare(i.getSource(SourceIndex::SRC0));

    emit("  ");

    auto src1NeedsLenSuffix = i.getExtMsgDescriptor().isImm() ||
                              i.getSendFc() == SFID::UGM ||
                              i.hasInstOpt(InstOpt::EXBSO);
    if (src1NeedsLenSuffix)
      formatSendSrcWithLength(i.getSource(SourceIndex::SRC1),
                              i.getSrc1Length());
    else
      formatSrcBare(i.getSource(SourceIndex::SRC1));

    emit("  ");

    auto immOff = i.getExtImmOffDescriptor();
    if (immOff != 0) {
      emitHex(immOff);
      emit(":");
    }
    formatSendExDesc(i.getExtMsgDescriptor());
    emit("  ");
    formatSendDesc(i.getMsgDescriptor(), 8);

    // instruction options
    formatInstOpts(i);

    // EOL comments
    formatEolComments(i);
  }

  // xe3 backwards compatible send with Gather Send support
  void formatInstructionBodySendXe3(const Instruction &i) {
    formatInstructionPrefix(i);

    emit("  ");
    formatSendDstOp(i);

    emit("  ");
    if (i.isGatherSend()) {
      formatRegIndRef(i.getSource(SourceIndex::SRC0));
      emit(ANSI_RESET);
    } else {
      formatSrcBare(i.getSource(SourceIndex::SRC0));
    }
    emit("  ");

    // Gather Send lacks src1 (implicitly null)
    if (!i.isGatherSend()) {
      auto src1NeedsLenSuffix = i.getExtMsgDescriptor().isImm() ||
                                i.getSendFc() == SFID::UGM ||
                                i.hasInstOpt(InstOpt::EXBSO);
      if (src1NeedsLenSuffix)
        formatSendSrcWithLength(i.getSource(SourceIndex::SRC1),
                                i.getSrc1Length());
      else
        formatSrcBare(i.getSource(SourceIndex::SRC1));

      emit("  ");
    }

    auto immOff = i.getExtImmOffDescriptor();
    if (immOff != 0) {
      emitHex(immOff);
      emit(":");
    }
    formatSendExDesc(i.getExtMsgDescriptor());
    emit("  ");
    formatSendDesc(i.getMsgDescriptor(), 8);

    // instruction options
    formatInstOpts(i);

    // EOL comments
    formatEolComments(i);
  }


  void formatInstructionPrefix(const Instruction &i) {
    formatMaskAndPredication(i);
    emit(' ');
    formatOpMnemonicExecInfo(i);
    emit(' ');
    formatFlagModifier(i);
  }

  void formatMaskAndPredication(const Instruction &i) {
    formatMaskAndPredication(i.getMaskCtrl(), i.getPredication(),
                             i.getFlagReg());
  }

  void formatMaskAndPredication(const MaskCtrl &mc, const Predication &p,
                                const RegRef &f) {
    startColumn(cols.predication);
    bool hasPred = p.function != PredCtrl::NONE;
    if (mc == MaskCtrl::NOMASK || hasPred) {
      emit('(');
      if (mc == MaskCtrl::NOMASK) {
        emit(ToSyntax(mc));
        if (hasPred) {
          emit('&');
        }
      }
      if (hasPred) {
        if (p.inverse) {
          emit('~');
        }
        formatScalarRegRead(RegName::ARF_F, f);
        emitAnsi(ANSI_FLAGMODIFIER, ToSyntax(p.function));
      }
      emit(')');
    }

    finishColumn();
  }

  // even if they ask for short names, some ops may be ambiguous that way
  // e.g. "sync.nop" may not be shortened to "nop" because then it would
  // parse back as the wrong thing
  bool shortOpWouldBeAmbiguous(const OpSpec &us) const {
    for (const OpSpec *them : model.ops()) {
      if (us.op != them->op && us.mnemonic.str() == them->mnemonic.str()) {
        return true;
      }
    }
    return false;
  }

  void formatExecInfo(const Instruction &i) {
    const OpSpec &os = i.getOpSpec();
    if (!os.hasImpicitEm()) {
      auto es = i.getExecSize();
      auto coff = i.getChannelOffset();
      emit('(');
      emit(ToSyntax(es));
      emit('|');
      emitAnsi(coff == ChannelOffset::M0, ANSI_FADED, ToSyntax(coff));
      emit(')');
    }
  }

  void formatOpMnemonicExecInfo(const Instruction &i) {
    startColumn(cols.opCodeExecInfo);

    const OpSpec &os = i.getOpSpec();

    emitAnsi(ANSI_MNEMONIC, os.mnemonic.str());

    std::string subfunc;
    switch (os.op) {
    // case Op::SEL:
    // TODO: with syntax extensions we could treat
    //    sel (lt)f1.1 ...
    //  as
    //    sel.lt ... f1.1
    //
    //  and
    //
    //    (f0.1) sel ...
    //    (~f1.1) sel ...
    //    (~f1.1.any*) sel ...
    //  as
    //    sel.esc   ... f0.1
    //    sel.any*  ... f0.1
    case Op::MATH:
      subfunc = ToSyntax(i.getMathFc());
      break;
    case Op::SEND:
    case Op::SENDC:
      if (platform() >= Platform::XE) {
        subfunc = ToSyntax(i.getSendFc());
      } // else part of ex_desc
      break;
    case Op::SYNC:
      subfunc = ToSyntax(i.getSyncFc());
      break;
    case Op::BFN:
      if (opts.syntaxBfnSymbolicFunctions) {
        // decode binary function names to symbolic
        // bfn.(s0&s1|~s2)
        subfunc += "(";
        subfunc += i.getBfnFc().c_str();
        subfunc += ")";
      } else {
        // use a raw value
        // emit<uint32_t>(i.getBfnFc().value);
        std::stringstream ss;
        ss << "0x" << std::hex << std::setw(2) << std::setfill('0')
           << std::uppercase << i.getBfnFc().value;
        subfunc = ss.str();
      }
      break;
    case Op::DPAS:
    case Op::DPASW:
      subfunc = ToSyntax(i.getDpasFc());
      break;
    default:
      if (os.supportsBranchCtrl() && i.getBranchCtrl() == BranchCntrl::ON) {
        subfunc += "b";
      }
      break;
    }

    if (!subfunc.empty()) {
      emit('.');
      emitAnsi(ANSI_SUBFUNCTION, std::move(subfunc));
    }

    emit(' ');
    formatExecInfo(i);

    finishColumn();
  }

  void formatFlagModifier(const Instruction &i) {
    startColumn(cols.flagMod);
    // should be check for suportsFlagModifier, but looks like our
    // models are wrong on math instruction.
    /*&& i.getOpSpec().supportsFlagModifier()*/
    if (i.hasFlagModifier() && !i.getOpSpec().isAnySendFormat()) {
      emitAnsi(ANSI_FLAGMODIFIER, '(', ToSyntax(i.getFlagModifier()), ')');

      if (i.is(Op::SEL)) {
        // select actually uses the conditional modifier as a read
        // source, not a write: insanity
        formatScalarRegRead(RegName::ARF_F, i.getFlagReg());
      } else {
        formatScalarRegWrite(RegName::ARF_F, i.getFlagReg());
      }
    }

    finishColumn();
  }

  void formatDstRegion(const Region &rgn) {
    bool isVector = rgn.getHz() == Region::Horz::HZ_1;
    emitAnsi(isVector, ANSI_FADED, ToSyntax(rgn));
  }

  void formatSourceModifier(const OpSpec &os, SrcModifier sm) {
    if (sm != SrcModifier::NONE) {
      emit(ToSyntax(os.op, sm));
    }
  }

  void formatSourceRegion(SourceIndex srcIx, const Instruction &i) {
    int srcIxVal = static_cast<int>(srcIx);
    const OpSpec &os = i.getOpSpec();
    const Operand &src = i.getSource(srcIx);
    // breaks reversibiliy with:
    //  movi (8|M0)    r37.0<1>:uw   r[a0.0,64]<8;8,1>:uw  null<0;1,0>:ud
    // writes null out as null:ud and parses it as <1;1,0>
    // either need to complicate the default regioning rules by defaulting
    //  null:ud to <0;1,0>
    // but this makes default region a function of the register used
    // (additional complexity to those already complex rules)
    //
    // if (src.getDirRegName() == RegName::ARF_NULL) {
    //  // drop the region on all null operands
    // return;
    // }
    const Region &rgn = src.getRegion();
    if (os.hasImplicitSrcRegion(srcIxVal, i.getExecSize(), i.isMacro()) ||
        os.isBranching()) {
      // e.g. on some platforms certain branches have an implicit <0;1,0>
      // meaning we can omit it if the IR matches
      return;
    }

    // region e.g. <8;8,1> or <1;0> ternary src0/src1 <1> for tern src2
    bool isVector = false, isScalar = false;
    if (os.isTernary()) {
      // align1 ternary has form
      // op (...) dst<H>  src0<v;h>  src1<v;h>  src2<h>
      if (srcIxVal < 2) {
        isVector = rgn == Region::SRC1X0;
        isScalar = rgn == Region::SRC0X0;
      } else {
        isVector = rgn == Region::SRCXX1;
        isScalar = rgn == Region::SRCXX0;
      }
    } else {
      // all other operands use the full <v;w,h>
      // this also handles <w,h> (VxH or Vx1 mode)
      isVector = rgn == Region::SRC110;
      isScalar = rgn == Region::SRC010;
    }
    bool fadeRegion =
        (i.getExecSize() == ExecSize::SIMD1 && isScalar) || isVector;
    emitAnsi(fadeRegion, ANSI_FADED, ToSyntax(rgn));
  }

  void formatSourceType(int srcIx, const OpSpec &os, const Operand &op) {
    if ((os.isBranching() && model.supportsSimplifiedBranches())) {
      // doesn't support types
      return;
    }
    if (os.is(Op::SYNC) && op.isNull())
      return;
    const Type &type = op.getType();
    bool lblArg = op.getKind() == Operand::Kind::LABEL ||
                  op.getKind() == Operand::Kind::IMMEDIATE;
    if (os.hasImplicitSrcType(srcIx, lblArg)) {
      auto expectedType = os.implicitSrcType(srcIx, lblArg);
      if (expectedType == type) {
        return;
      }
    }

    // fade the type if it's uniform
    bool isUniformType =
        currInst && currInst->getDestination().getType() == type;
    emitAnsi(isUniformType, ANSI_FADED, ToSyntax(type));
  }

  void formatSendDesc(const SendDesc &sd, int cw = 0) {
    startColumn(cols.sendDesc);

    emit(ANSI_SUBFUNCTION);
    if (sd.isImm()) {
      emitHex(sd.imm, cw);
    } else {
      emit('a');
      formatRegRef(sd.reg);
    }
    emit(ANSI_RESET);

    finishColumn();
  }
  void formatSendExDesc(const SendDesc &sd, int cw = 0) {
    startColumn(cols.sendDesc);
    if (sd.isImm()) {
      emit(ANSI_IMMEDIATE);
      emitHex(sd.imm, cw);
    } else {
      emit(ANSI_REGISTER_ARF);
      emit('a');
      formatRegRef(sd.reg);
    }
    emit(ANSI_RESET);

    finishColumn();
  }

  // e.g. op (8) ... {Compacted}
  //                 ^^^^^^^^^^^
  void formatInstOpts(const Instruction &i,
                      const std::vector<const char *> &instOpts);
  void formatInstOpts(const Instruction &i) {
    std::vector<const char *> emptyExtraOpts;
    formatInstOpts(i, emptyExtraOpts);
  }

  bool hasInstOptTokens(const Instruction &i) const {
    bool hasDepInfo = platform() >= Platform::XE && i.getSWSB().hasSWSB();
    return !i.getInstOpts().empty() || hasDepInfo;
  }

  void formatEolComments(const Instruction &i,
                         const std::string &debugSendDecode = "",
                         bool decodeSendDesc = true) {
    std::stringstream ss;

    // separate all comments with a semicolon
    Intercalator semiColon(ss, "; ");

    if (opts.printInstDefs && opts.liveAnalysis) {
      // -Xprint-defs
      // emit all live ranges where this instruction is the use
      // this will indicate the source instruction
      std::stringstream ss2;
      intercalate(
          ss2, ",", opts.liveAnalysis->deps,
          [&](const Dep &d) { return d.use == &i; },
          [&](const Dep &d) {
            if (d.def) {
              ss2 << " #" << d.def->getID();
            } else {
              ss2 << " IN";
            }
            d.values.str(ss2);
          });
      if (ss2.tellp() > 0) {
        semiColon.insert();
        ss << ss2.str();
      }

#if 0
      // for debugging (normally used by JSON only)
      const auto &live = opts.liveAnalysis->sums[i.getID()];
      semiColon.insert();
      ss << "r=" << live.grfBytes;
      ss << ",acc=" << live.accBytes;
      ss << ",f=" << live.flagBytes;
      ss << ",a=" << live.indexBytes;
      ss << ",s=" << live.scalarBytes;
#endif
    }

    const std::string &comment = i.getComment();
    if (!comment.empty()) {
      // custom comment attached to the instruction by some processor
      semiColon.insert();
      //
      // if the comment contains newlines, change them to ; so that
      // the EOL comment won't be broken
      for (size_t i = 0; i < comment.size(); i++) {
        if (comment[i] == '\r' || comment[i] == '\n')
          ss << "; ";
        else
          ss << comment[i];
      }
    }

      if (i.getOpSpec().isAnySendFormat()) {
        // send with immediate descriptors, we can decode this to
        // something more sane in comments
        const SendDesc exDesc = i.getExtMsgDescriptor(),
                       desc = i.getMsgDescriptor();
        //
        if (decodeSendDesc) {
          // ld/st syntax not enabled
          if (ss.tellp() != 0) {
            semiColon.insert();
          }
          EmitSendDescriptorInfo(platform(), i.getSendFc(), i.getExecSize(),
                                 !i.getDestination().isNull(), i.getDstLength(),
                                 i.getSrc0Length(), i.getSrc1Length(),
                                 i.getExtImmOffDescriptor(),
                                 exDesc, desc, ss);
        } else if (opts.printLdSt) {
          // tried to format with ld/st syntax and ...
          if (ss.tellp() != 0) {
            semiColon.insert();
          }
          // success, show the descriptors for debugging
          if (debugSendDecode.empty()) {
            if (exDesc.isImm())
              fmtHex(ss, exDesc.imm);
            else
              ss << "???";
            ss << "  ";
            if (desc.isImm())
              fmtHex(ss, desc.imm);
            else
              ss << "???";
          }
        }
        if (!debugSendDecode.empty()) {
          semiColon.insert();
          ss << debugSendDecode;
        }
      } else if (i.is(Op::BFN) && !opts.syntaxBfnSymbolicFunctions) {
        semiColon.insert();
        ss << i.getBfnFc().c_str();
      }

    if (ss.tellp() > 0) {
      // only add the comment if we emitted something
      emitAnsi(ANSI_COMMENT, " // ", ss.str());
    }
  }

  void formatRegIndRef(const Operand &op) {
    emitAnsi(ANSI_REGISTER_GRF, "r");
    emit("[");
    if (op.getDirRegName() == RegName::ARF_S)
      formatScalarRegRead(op.getDirRegName(), op.getDirRegRef());
    else
      formatScalarRegRead(RegName::ARF_A, op.getIndAddrReg());

    if (op.getIndImmAddr() != 0) {
      int16_t val = op.getIndImmAddr();
      if (!opts.syntaxExtensions) {
        emit(',');
      } else if (op.getIndImmAddr() > 0) {
        emit("+");
      } else {
        emit("-");
        val = -val;
      }
      emitAnsi(ANSI_IMMEDIATE, val);
    }
    emit(']');
  }

  void formatScalarRegWrite(RegName rnm, RegRef rr) {
    // we may distinguish in the future
    formatScalarRegRead(rnm, rr);
  }
  void formatScalarRegRead(RegName rnm, RegRef rr) {
    const auto *rinfo = model.lookupRegInfoByRegName(rnm);
    if (rinfo) {
      emit(ANSI_REGISTER(rnm));
      emit(rinfo->syntax);
    } else {
      emit("???");
    }
    formatRegRef(rr);
    emit(ANSI_RESET);
  }

  void formatRegRef(const RegRef &ref) {
    emit((int)ref.regNum, '.', (int)ref.subRegNum);
  }
}; // end: class Formatter

void Formatter::formatSrcBare(const Operand &src) {
  emit(ANSI_REGISTER(src.getDirRegName()));

  const RegInfo *ri = model.lookupRegInfoByRegName(src.getDirRegName());
  if (ri == nullptr) {
    emit("???");
    return;
  }
  emit(ri->syntax);
  int regNum = (int)src.getDirRegRef().regNum;
  if (regNum != 0 || ri->hasRegNum()) {
    emit(regNum);
  }
  emit(ANSI_RESET);
}

void Formatter::formatDstType(const OpSpec &os, Type type) {
  if (os.isBranching() && model.supportsSimplifiedBranches()) {
    return; // doesn't support types
  }
  if (os.hasImplicitDstType()) {
    if (os.implicitDstType() != type) {
      emit(ToSyntax(type));
    } // else: it's already in in binary normal form
  } else {
    // type
    emit(ToSyntax(type));
  }
}

void Formatter::formatSendSrcWithLength(const Operand &src, int srcLen) {
  emit(ANSI_REGISTER(src.getDirRegName()));

  const RegInfo *ri = model.lookupRegInfoByRegName(src.getDirRegName());
  if (ri == nullptr) {
    emit("???");
    return;
  }
  emit(ri->syntax);
  int regNum = (int)src.getDirRegRef().regNum;
  if (regNum != 0 || ri->hasRegNum()) {
    emit(regNum);
  }

  // src register format: r10:4 means r10-r13 (used to be r10#4)
  emit(":");
  emit(srcLen);

  emit(ANSI_RESET);
}

void Formatter::formatBareRegisterUnescaped(RegName regName, int regNum) {
  const RegInfo *ri = model.lookupRegInfoByRegName(regName);
  if (ri == nullptr) {
    emit("???");
    return;
  }

  emit(ri->syntax);

  if (regNum != 0 || ri->hasRegNum()) {
    // some registers don't have numbers
    // e.g. null or ce
    emit(regNum);
  }
}

void Formatter::formatRegister(RegName regName, RegRef reg, bool emitSubReg,
                               bool isSIMT) {
  emit(ANSI_REGISTER(regName));

  // show the subreg if:
  //  - caller demands it (e.g. it's a nonsend) AND
  //       the register chosen has subregisters (e.g. not ce and null)
  //  - OR it's non-zero (either bad IR or something's there)
  const RegInfo *ri = model.lookupRegInfoByRegName(regName);
  if (ri == nullptr) {
    emit("RegName::???");
    return;
  }
  formatBareRegisterUnescaped(regName, (int)reg.regNum);

  if ((emitSubReg && ri->hasSubregs()) || reg.subRegNum != 0) {
    if (isSIMT)
      emit(ANSI_FADED); // a boring SIMT access => fade the text
    emit('.');
    emit((int)reg.subRegNum);
  }

  emit(ANSI_RESET);
}

void Formatter::formatDstOp(const Instruction &i) {
  const OpSpec &os = i.getOpSpec();
  const Operand &dst = i.getDestination();

  startColumn(os.isAnySendFormat() ? cols.sendDstOp : cols.dstOp);

  if (dst.getDstModifier() == DstModifier::SAT) {
    emit("(sat)");
  }

  switch (dst.getKind()) {
  case Operand::Kind::DIRECT: {
    bool isSIMT =
        i.getExecSize() > ExecSize::SIMD1 && // non-scalar op
        dst.getRegion() == Region::DST1 &&   // packed (non-strided) write
        dst.getDirRegRef().subRegNum == 0;   // writing to the mid. of reg.
    formatRegister(dst.getDirRegName(), dst.getDirRegRef(),
                   os.hasDstSubregister(i.isMacro()), isSIMT);
    break;
  }
  case Operand::Kind::MACRO:
    formatRegister(dst.getDirRegName(), dst.getDirRegRef(),
                   os.hasDstSubregister(true));
    emitAnsi(ANSI_REGISTER(RegName::ARF_MME), ToSyntax(dst.getMathMacroExt()));
    break;
  case Operand::Kind::INDIRECT:
    formatRegIndRef(dst);
    break;
  default:
    emit("Operand::Kind::?");
  }

  Region dstRgn = dst.getRegion();
  if (!os.hasImplicitDstRegion(i.isMacro()) ||
      (dstRgn != os.implicitDstRegion(i.isMacro()) &&
       dstRgn != Region::INVALID)) {
    // some instructions don't have dst regions
    formatDstRegion(dst.getRegion());
  }
  formatDstType(os, dst.getType());

  finishColumn();
}

// Normal backwards compatible send or newer generalized
void Formatter::formatSendDstOp(const Instruction &i) {
  const Operand &dst = i.getDestination();

  startColumn(cols.sendDstOp);

  formatRegister(dst.getDirRegName(), dst.getDirRegRef(), false, true);

  finishColumn();
}

static bool sendSrc1NeedsLengthSuffix(const Formatter &f,
                                      const Instruction &i) {
  // If Src1.Length is not encoded in the ExDesc (but rather in EU ISA)
  // then we need to emit Src1.Length somewhere else.  We've chosen
  // to the suffix the source register.
  //   e.g. r1:4
  // This holds for XeHP with ExBSO
  // and for imm descs in XeHPG+ as well
  auto exDesc = i.getExtMsgDescriptor();
  return i.hasInstOpt(InstOpt::EXBSO) || // includes the XeHPG+ imm cases
         (exDesc.isImm() && f.platform() >= Platform::XE_HPG);
}

void Formatter::formatSrcOp(SourceIndex srcIx, const Instruction &i) {
  const Operand &src = i.getSource(srcIx);
  const OpSpec &os = i.getOpSpec();

  startColumn(os.isAnySendFormat() ? cols.sendSrcOp : cols.srcOp);

  switch (src.getKind()) {
  case Operand::Kind::DIRECT: {
    // NOTE: this path is not called for send messages after a given platform;
    // in other words:
    IGA_ASSERT(!os.isAnySendFormat() || platform() < Platform::XE2,
               "unexpected send formatting");
    if (os.isAnySendFormat() && srcIx == SourceIndex::SRC1 &&
        sendSrc1NeedsLengthSuffix(*this, i)) {
      formatSendSrcWithLength(src, i.getSrc1Length());
      break;
    }
    bool hasSubreg = os.hasSrcSubregister(int(srcIx), i.isMacro());
    bool isSimt = i.getExecSize() > ExecSize::SIMD1 &&
                  (src.getRegion() != Region::SRC110 ||
                   src.getRegion() != Region::SRC1X0 ||
                   src.getRegion() != Region::SRCXX1) &&
                  src.getDirRegRef().subRegNum == 0;
    formatSourceModifier(os, src.getSrcModifier());
    formatRegister(src.getDirRegName(), src.getDirRegRef(), hasSubreg, isSimt);
    formatSourceRegion(srcIx, i);
    break;
  }
  case Operand::Kind::MACRO: {
    formatSourceModifier(os, src.getSrcModifier());
    formatRegister(src.getDirRegName(), src.getDirRegRef(), false);
    emit(ToSyntax(src.getMathMacroExt()));
    formatSourceRegion(srcIx, i);
    ;
    break;
  }
  case Operand::Kind::INDIRECT:
    formatSourceModifier(os, src.getSrcModifier());
    formatRegIndRef(src);
    formatSourceRegion(srcIx, i);
    break;
  case Operand::Kind::IMMEDIATE:
    emit(ANSI_IMMEDIATE);
    switch (src.getType()) {
    case Type::B:
      emitDecimal(src.getImmediateValue().s8);
      break;
    case Type::W:
      emitDecimal(src.getImmediateValue().s16);
      break;
    case Type::D:
      emitDecimal(src.getImmediateValue().s32);
      break;
    case Type::Q:
      emitDecimal(src.getImmediateValue().s64);
      break;
    case Type::UB:
      emitHex(src.getImmediateValue().u8);
      break;
    case Type::UW:
      emitHex(src.getImmediateValue().u16);
      break;
    case Type::UD:
      emitHex(src.getImmediateValue().u32);
      break;
    case Type::UQ:
      emitHex(src.getImmediateValue().u64);
      break;
    case Type::BF:
      emitHex(src.getImmediateValue().u16);
      break;
    case Type::HF:
      if (opts.hexFloats) {
        emitHex(src.getImmediateValue().u16);
      } else {
        FormatFloat(o, src.getImmediateValue().u16);
      }
      break;
    case Type::F:
    case Type::TF32:
      if (opts.hexFloats) {
        emitHex(src.getImmediateValue().u32);
      } else {
        emitFloat(src.getImmediateValue().f32);
      }
      break;
    case Type::DF:
      if (opts.hexFloats) {
        emitHex(src.getImmediateValue().u64);
      } else {
        emitFloat(src.getImmediateValue().f64);
      }
      break;
    case Type::BF8:
    case Type::HF8:
      emitHex(src.getImmediateValue().u8);
      break;
    case Type::V:
      emitHex(src.getImmediateValue().u32);
      break;
    case Type::UV:
      emitHex(src.getImmediateValue().u32);
      break;
    case Type::VF:
      // packed float format
      //  each of four 8 - bit immediate vector elements has 1 sign bit,
      //  3 bits for the biased exponent (bias of 3), and 4 bits
      //  for the significand
      emitHex(src.getImmediateValue().u32);
      break;
    default:
      emit("???");
    };
    emit(ANSI_RESET);
    break;
  case Operand::Kind::LABEL: {
    emit(ANSI_IMMEDIATE);
    const Block *b = src.getTargetBlock();
    formatLabel(b ? b->getPC() : src.getImmediateValue().s32);
    emit(ANSI_RESET);
    break;
  }
  default:
    emit("???");
  }

  if (!model.supportsSimplifiedBranches() ||
      src.getKind() != Operand::Kind::LABEL) {
    formatSourceType(int(srcIx), os, src);
  }
  finishColumn();
} // end formatSrcOp()

// e.g. formats: sync.allrd ($1,$3,$7)
void Formatter::formatSyncAllSrc0(const Instruction &i) {
  const auto &src0 = i.getSource(0);
  if (src0.getKind() != Operand::Kind::IMMEDIATE) {
    // e.g.  sync.allrd  null
    formatSrcOp(SourceIndex::SRC0, i);
  } else {
    // e.g.  sync.allrd  ($1,$3,$7,$13)
    emit(ANSI_IMMEDIATE);
    auto sbids = src0.getImmediateValue().u32;
    bool first = true;
    emit("(");
    for (int i = 0; i < 32; i++) {
      if ((1 << i) & sbids) {
        if (first)
          first = false;
        else
          emit(",");
        emit("$", i);
      }
    }
    emit(")");
    emit(ANSI_RESET);
  }
}

void Formatter::formatInstOpts(const Instruction &i,
                               const std::vector<const char *> &extraInstOpts) {
  const auto &iopts = i.getInstOpts();

  const auto &di = i.getSWSB();
  bool hasDepInfo = platform() >= Platform::XE && di.hasSWSB();
  if (iopts.empty() && !hasDepInfo && extraInstOpts.empty()) {
    return; // early out (no braces)
  }

  emit(" {");
  ToSyntaxNoBraces(o, iopts);
  // extra options germane to such ld/st syntax
  for (size_t opIx = 0; opIx < extraInstOpts.size(); opIx++) {
    if (opIx > 0) {
      emit(",");
    }
    emit(extraInstOpts[opIx]);
  }
  if (!iopts.empty() && (hasDepInfo || !extraInstOpts.empty())) {
    // something was output, prepend a , for the depinfo
    emit(",");
  }

  // special token and dist/token won't co-exist in the same swsb
  // no need to insert "," after this since there must not be
  // dist/token swsb
  if (di.hasSpecialToken()) {
    assert(!di.hasDist() && !di.hasToken());
    if (di.spToken == SWSB::SpecialToken::NOACCSBSET)
      emit("NoAccSBSet");
  }

  if (di.hasDist() || di.hasToken()) {
    switch (di.distType) {
    case SWSB::DistType::REG_DIST:
      emit("@");
      emit((int)di.minDist);
      break;
    case SWSB::DistType::REG_DIST_ALL:
      emit("A@");
      emit((int)di.minDist);
      break;
    case SWSB::DistType::REG_DIST_FLOAT:
      emit("F@");
      emit((int)di.minDist);
      break;
    case SWSB::DistType::REG_DIST_INT:
      emit("I@");
      emit((int)di.minDist);
      break;
    case SWSB::DistType::REG_DIST_LONG:
      emit("L@");
      emit((int)di.minDist);
      break;
    case SWSB::DistType::REG_DIST_MATH:
      emit("M@");
      emit((int)di.minDist);
      break;
    case SWSB::DistType::REG_DIST_SCALAR:
      emit("S@");
      emit((int)di.minDist);
      break;
    default:
      break;
    }

    if (di.hasBothDistAndToken())
      emit(",");

    switch (di.tokenType) {
    case SWSB::TokenType::DST:
      emit("$");
      emit((int)di.sbid);
      emit(".dst");
      break;
    case SWSB::TokenType::SRC:
      emit("$");
      emit((int)di.sbid);
      emit(".src");
      break;
    case SWSB::TokenType::SET:
      emit("$");
      emit((int)di.sbid);
      break;
    default:
      break;
    }
  }
  emit('}');
} // end formatInstOpts

bool Formatter::formatLoadStoreSyntax(const Instruction &i) {
  // TODO: maybe re-enable this if we can find a way to make this non-ambiguous
  if (platform() >= Platform::XE3) {
    return false;
  }
  if (platform() < Platform::XE_HPG) {
    // We will not even try on <=XeHPG
    return false;
  }
  const auto desc = i.getMsgDescriptor();
  if (desc.isReg()) {
    // given a register descriptor, we've no hope of decoding the op
    return false;
  }
  const auto exDesc = i.getExtMsgDescriptor();

  const auto sfid = i.getSendFc();
  const auto di = tryDecode(platform(), sfid, i.getExecSize(),
                            i.getExtImmOffDescriptor(),
                            exDesc, desc, nullptr);
  if (!di) {
    // if decode failed fallback to the canonical send syntax
    return false;
  } else if (!sendOpSupportsSyntax(platform(), di.info.op, sfid)) {
    // if decode succeeded, but the syntax isn't supported, then back off
    return false;
  }

  formatMaskAndPredication(i);

  const auto syntax = di.syntax;
  startColumn(cols.opCodeExecInfo + 12);
  emit(' ');
  emit(ANSI_MNEMONIC);
  emit(syntax.mnemonic); // e.g. load
  emit(ANSI_SUBFUNCTION);
  emit(syntax.controls); // e.g. .ugm.d32.a64.uc.ca
  emit(ANSI_RESET);
  emit(' ');
  formatExecInfo(i);
  finishColumn();

  auto emitPayload = [&](const Operand &op, int len) {
    emit(ANSI_REGISTER(op.getDirRegName()));
    //
    formatBareRegisterUnescaped(op.getDirRegName(),
                                (int)op.getDirRegRef().regNum);
    if (len >= 0) {
      emit(ANSI_FADED);
      emit(':');
      emitDecimal(len);
    }
    emit(ANSI_RESET);
    //
  };

  // emits the destination data register part (for load and atomic)
  auto emitDstDataReg = [&]() {
    startColumn(cols.dstOp);
    //
    int len = i.getDstLength();
    if (i.getDestination().getDirRegName() == RegName::ARF_NULL) {
    }
    emitPayload(i.getDestination(), len);
    //
    finishColumn();
  };

  // emits the [...]:2:a64 part
  auto emitAddrReg = [&](int cols) {
    startColumn(cols);
    emit(syntax.surface);
    emit('[');
    emit(syntax.scale);
    //
    emitPayload(i.getSource(0), i.getSrc0Length());
    //
    emit(syntax.immOffset);
    emit(']');
    finishColumn();
  };

  emit("  ");
  if (syntax.isLoad()) {
    emitDstDataReg();
    emit(' ');
    emitAddrReg(cols.srcOp);
    //
  } else if (syntax.isStore()) {
    emitAddrReg(cols.dstOp);
    emit(' ');
    startColumn(cols.srcOp);
    emitPayload(i.getSource(1), i.getSrc1Length());
    finishColumn();
    //
  } else if (syntax.isAtomic()) {
    // atomic_fadd....d32.a64 ... null  [r10:2]   r12:2
    emitDstDataReg();
    emit(' ');
    emitAddrReg(cols.srcOp);
    emit(' ');
    startColumn(cols.srcOp);
    emitPayload(i.getSource(1), i.getSrc1Length());
    finishColumn();
    //
  } else {
    const auto &dst = i.getDestination();
    if (dst.getDirRegName() != RegName::ARF_NULL) {
      emitDstDataReg();
      emit(' ');
    }
    emitAddrReg(cols.srcOp);
  }

  emit(' ');

  // instruction options
  formatInstOpts(i);

  // EOL comments
  //
  // provide the descriptor for debugging
  std::stringstream ss;
  if (exDesc.isImm()) {
    ss << "ex_desc:" << fmtHex(exDesc.imm);
  } else { // exDesc.isReg()
    ss << "ex_desc:a0." << (int)exDesc.reg.subRegNum;
  }
  // desc has to be imm or we wouldn't be decoding the message in
  // ld/st syntax
  ss << "; desc:" << fmtHex(desc.imm);
  // provide the rlen=... (Dst.Length)

  formatEolComments(i, ss.str(), false);

  return true;
}

///////////////////////////////////////////////////////////////////////////////
//
// Public interfaces into the kernel
void FormatOpts::addApiOpts(uint32_t fmtOpts, uint32_t pcOff) {
  numericLabels = (fmtOpts & IGA_FORMATTING_OPT_NUMERIC_LABELS) != 0;
  syntaxExtensions = (fmtOpts & IGA_FORMATTING_OPT_SYNTAX_EXTS) != 0;
  hexFloats = (fmtOpts & IGA_FORMATTING_OPT_PRINT_HEX_FLOATS) != 0;
  printInstPc = (fmtOpts & IGA_FORMATTING_OPT_PRINT_PC) != 0;
  printInstBits = (fmtOpts & IGA_FORMATTING_OPT_PRINT_BITS) != 0;
  printInstDefs = (fmtOpts & IGA_FORMATTING_OPT_PRINT_DEFS) != 0;
  printInstDeps = (fmtOpts & IGA_FORMATTING_OPT_PRINT_DEPS) != 0;
  printLdSt = (fmtOpts & IGA_FORMATTING_OPT_PRINT_LDST) != 0;
  syntaxBfnSymbolicFunctions =
      (fmtOpts & IGA_FORMATTING_OPT_PRINT_BFNEXPRS) != 0;
  printAnsi = (fmtOpts & IGA_FORMATTING_OPT_PRINT_ANSI) != 0;
  const auto jsonOpts =
    IGA_FORMATTING_OPT_PRINT_JSON | IGA_FORMATTING_OPT_PRINT_JSON_V1;
  printJson = (fmtOpts & jsonOpts) != 0;
  printJsonVersion = (fmtOpts & IGA_FORMATTING_OPT_PRINT_JSON_V1) ? 1 : 2;
  basePCOffset = pcOff;
}

void FormatKernel(ErrorHandler &e, std::ostream &o, const FormatOpts &opts,
                  const Kernel &k, const void *bits) {
  IGA_ASSERT(k.getModel().platform == opts.model.platform,
             "kernel and options must have same platform");
  if (opts.printInstDefs && opts.liveAnalysis == nullptr) {
    DepAnalysis la = ComputeDepAnalysis(&k);
    FormatOpts optsCopy = opts;
    optsCopy.liveAnalysis = &la;
    FormatKernel(e, o, optsCopy, k, bits);
    return;
  }
  if (!opts.printJson) {
    Formatter f(e, o, opts);
    f.formatKernel(k, (const uint8_t *)bits);
  } else {
    FormatJSON(o, opts, k, bits);
  }
}

void FormatInstruction(ErrorHandler &e, std::ostream &o, const FormatOpts &opts,
                       const Instruction &i, const void *bits) {
  if (opts.printJson) {
    if (opts.printJsonVersion == 1)
      FormatInstructionJSON1(o, opts, i, bits);
    else
      FormatInstructionJSON2(o, opts, i, bits);
  } else {
    Formatter f(e, o, opts);
    f.formatInstruction(i, (const uint8_t *)bits);
  }
}

#ifndef IGA_DISABLE_ENCODER_EXCEPTIONS
void FormatInstruction(ErrorHandler &e, std::ostream &o, const FormatOpts &opts,
                       const void *bits) {
  FormatInstruction(e, o, opts, 0, bits);
}
void FormatInstruction(ErrorHandler &e, std::ostream &o, const FormatOpts &opts,
                       size_t startPc, const void *bits,
                       bool useNativeDecoder) {
  size_t instLen = ((const MInst *)bits)->isCompact() ? 8 : 16;

  DecoderOpts dopts;
  dopts.useNumericLabels = true;
  iga::Kernel *k = nullptr;
  if (useNativeDecoder && iga::native::IsDecodeSupported(opts.model, dopts)) {
    k = iga::native::Decode(opts.model, dopts, e, bits, instLen);
  } else {
    k = iga::ged::Decode(opts.model, dopts, e, bits, instLen);
  }
  if (e.hasErrors()) {
    for (const auto &err : e.getErrors()) {
      o << err.message << "\n";
    }
  }
  if (k) {
    const auto &bl = k->getBlockList();
    if (bl.size() > 0) {
      const auto &il = bl.front()->getInstList();
      if (il.size() > 0) {
        Instruction &i = *il.front();
        i.setPC((int32_t)startPc);
        FormatInstruction(e, o, opts, i, bits);
      }
    }
    delete k;
  }
}
#endif

void GetDefaultLabelName(std::ostream &o, int32_t pc) {
  Formatter::getDefaultLabelDefinition(o, pc);
}

///////////////////////////////////////////////////////////////////////////////
// Following functions generally used for testing, debugging, or
// producing friendly diagnostics

///////////////////////////////////////////////////////////////////////////////
// The opname should be resolved
std::string FormatOpName(const iga::Model &model, const void *bits) {
  std::string s;
  iga::OpSpecMissInfo missInfo = {0};
  const iga::OpSpec &os = model.lookupOpSpecFromBits(bits, missInfo);

  if (!os.isValid()) {
    std::stringstream ss;
    ss << "OP[" << std::hex << "]" << missInfo.opcode << "?";
    s = ss.str();
  } else {
    s = os.mnemonic;
  }

  s += ':';
  while (s.length() < 8)
    s += ' ';
  return s;
}
std::string FormatOpBits(const iga::Model &model, const void *bitsV) {
  const uint8_t *bits = (const uint8_t *)bitsV;
  bool compact = ((const iga::MInst *)bits)->isCompact();
  int iLen = compact ? 8 : 16;

  std::stringstream ss;
  ss << iga::PadR(12, FormatOpName(model, bitsV));
  for (int i = 0; i < iLen; i++) {
    if (i > 0)
      ss << ' ';
    ss << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)bits[i];
    if (i == 7) {
      ss << ' ';
    }
  }
  return ss.str();
}

} // namespace iga
