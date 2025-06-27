/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "FormatterJSON.hpp"
#include "../IR/Messages.hpp"
#include "IRToString.hpp"

#include <unordered_map>
#include <set>

using namespace iga;

class JSONFormatterV2 : public BasicFormatter {
protected:
  const Model &model;
  const FormatOpts &opts;
  const uint8_t *bits;
  int currIndent = 2;

  // map instruction ID to all def/use pairs
  using DUMap = std::unordered_map<int, std::vector<const Dep *>>;
  DUMap depDefs, depUses;

  const RegSet EMPTY_SET;

public:
  JSONFormatterV2(std::ostream &o, const FormatOpts &os, const void *bs)
      : BasicFormatter(false, o), model(os.model), opts(os),
        bits((const uint8_t *)bs), EMPTY_SET(os.model) {
    o << std::boolalpha;
    if (opts.liveAnalysis) {
      for (const Dep &d : opts.liveAnalysis->deps) {
        if (d.def != nullptr) {
          depDefs[d.def->getID()].emplace_back(&d);
        }
        if (d.use != nullptr) {
          depUses[d.use->getID()].emplace_back(&d);
        }
      }
    }
  }
  Platform platform() const { return model.platform; }

  // emits an error value that'll blow up on JSON load
  void emitIrError(const char *what) { emit("IR.Error(\"", what, "\")"); }

  void emitEscaped(const std::string &s) {
    for (size_t i = 0; i < s.size(); i++) {
      switch (s[i]) {
      case '\n':
        emit("\\n");
        break;
      case '\t':
        emit("\\t");
        break;
      case '\v':
        emit("\\v");
        break;
      case '\f':
        emit("\\f");
        break;
      case '\a':
        emit("\\a");
        break;
      case '\"':
        emit("\\\"");
        break;
      case '\\':
        emit("\\\\");
        break;
      default:
        emit(s[i]);
        break;
      }
    }
  }

  void emitIndent() {
    for (int i = 0; i < currIndent; i++)
      emit(' ');
  }
  void withIndent(const std::function<void()> &f) {
    currIndent += 2;
    f();
    currIndent -= 2;
  }

  void emitLabel(int32_t pc) {
    bool printed = false;
    if (opts.labeler) {
      const char *usrL = opts.labeler(pc, opts.labelerContext);
      if (usrL) {
        printed = true;
        emit(usrL);
      }
    }
    if (!printed) {
      std::stringstream ss;
      ss << std::setw(4) << std::dec << std::setfill('0') << pc;
      emit("L", ss.str());
    }
  }

  void emitKernel(const Kernel &k) {
    precomputeBlockGraph(k);

    emit("{\n");
    emit("  \"version\":\"2.0\",\n");
    emit("  \"platform\":\"", model.names[0].str(), "\",\n");
    emit("  \"elems\":[\n");
    currIndent += 2;

    for (const Block *b : k.getBlockList()) {
      emitIndent();
      emit("{\"kind\":\"L\"");
      emit(", \"id\":", b->getID());
      if (opts.printInstPc) {
        emit(", \"pc\":", b->getPC());
      }
      emit(", \"symbol\":\"");
      emitLabel(b->getPC());
      emit("\"");

      auto emitSet = [&](const char *which, const std::set<int> &ids) {
        if (ids.empty()) {
          return;
        }
        emit(", ", which, ":[");
        bool first = true;
        for (int id : ids) {
          if (first)
            first = false;
          else
            emit(",");
          emit(id);
        }
        emit("]");
      }; // emitSet
      emitSet("\"preds\"", preds[b]);
      emitSet("\"succs\"", succs[b]);

      emit("}");
      if (b != k.getBlockList().back() || !b->getInstList().empty())
        emit(",");
      emit("\n");

      for (const Instruction *iP : b->getInstList()) {
        emitInst(*iP);
        if (iP != b->getInstList().back() || b != k.getBlockList().back())
          emit(",");
        emit("\n");
      }
    }

    emit("  ]\n");
    currIndent -= 2;
    emit("}\n");
    emit("\n");
  }

  std::unordered_map<const Block *,std::set<int>> preds, succs;

  void precomputeBlockGraph(const Kernel &k) {
    succs.clear();
    preds.clear();

    const auto &bl = k.getBlockList();
    auto itr = bl.begin();

    auto addEdge =
      [&](const Block *p, const Block *s) {
        preds[s].insert(p->getID());
        succs[p].insert(s->getID());
      }; // addEdge

    while (itr != bl.end()) {
      const Block *b = *itr++;
      auto addBranchTarget =
        [&](const Operand &op) {
          if (op.getKind() != Operand::Kind::LABEL) {
            return;
          }
          const Block *bt = op.getTargetBlock();
          if (bt) {
            addEdge(b, bt);
          }
        }; // addBranchTarget

      const auto &bInsts = b->getInstList();
      if (bInsts.empty()) {
        // empty block: fallthrough
        if (itr != bl.end()) {
          addEdge(b, *itr);
        }
      } else {
        // non-empty block: look at last instruction
        const Instruction &li = *bInsts.back();
        if (!li.isBranching()) {
          // non-branching: fallthrough (unless EOT which kills program)
          bool unpredicatedEot =
            !li.hasInstOpt(InstOpt::EOT) && !li.hasPredication();
          if (itr != bl.end() && !unpredicatedEot) {
            addEdge(b, *itr);
          }
        } else {
          // edges to all operands
          for (unsigned i = 0; i < li.getSourceCount(); i++) {
            addBranchTarget(li.getSource(i));
          }
          // fallthrough only if not unpredicated jmpi
          if (!li.is(Op::JMPI) || li.hasPredication()) {
            if (itr != bl.end())
              addEdge(b, *itr);
          }
        }
      }
    } // while
  }

  std::vector<std::string> comments;

  void emitInst(const Instruction &i) {
    comments.clear();

    // const bool isSend = i.getOpSpec().isAnySendFormat();
    emitIndent();
    emit("{");
    emit("\"kind\":\"I\"");
    emit(", \"id\":", i.getID());
    if (opts.printInstPc) {
      emit(", \"pc\":", i.getPC());
    }

    if (opts.printLdSt) {
      if (!emitLdStInst(i)) {
        emitNormalInst(i);
      }
    } else {
      emitNormalInst(i);
    }

    if (platform() >= Platform::XE) {
      emitRegDist(i.getSWSB());
      emitSbid(i.getSWSB());
    }
    emitInstOpts(i);

    bool printBits = opts.printInstBits && bits != nullptr;
    if (printBits) {
      emit(", \"encoding\":");
      const uint32_t *instBits = (const uint32_t *)(bits + i.getPC());
      emit('\"');
      if (!i.hasInstOpt(InstOpt::COMPACTED)) {
        emitHex(instBits[3], 8);
        emit('`');
        emitHex(instBits[2], 8);
        emit('`');
      }
      emitHex(instBits[1], 8);
      emit('`');
      emitHex(instBits[0], 8);
      emit('\"');
    }

    if (opts.printInstDefs && opts.liveAnalysis &&
        !opts.liveAnalysis->sums.empty()) {
      emit(", \"liveTotals\":{");
      const auto &totals = opts.liveAnalysis->sums[i.getID()];
      emit("\"r\":", totals.grfBytes, ",");
      emit("\"acc\":", totals.accBytes, ",");
      emit("\"s\":", totals.scalarBytes, ",");
      emit("\"f\":", totals.flagBytes, ",");
      emit("\"a\":", totals.indexBytes);
      emit("}");
    }

    if (!comments.empty()) {
      emit(", \"comment\":\"");
      for (size_t i = 0; i < comments.size(); i++) {
        if (i > 0)
          emit("; ");
        emitEscaped(comments[i]);
      }
      emit("\"");
    }
    emit("}");
  }

  void emitNormalInst(const Instruction &i) {
    std::string subfunc;
    switch (i.getOpSpec().op) {
    case Op::MATH:
      subfunc = ToSyntax(i.getSubfunction().math);
      break;
    case Op::SEND:
    case Op::SENDC:
      subfunc = ToSyntax(i.getSubfunction().send);
      break;
    case Op::SYNC:
      subfunc = ToSyntax(i.getSubfunction().sync);
      break;
    case Op::BFN:
      if (opts.syntaxBfnSymbolicFunctions) {
        // decode binary function names to symbolic
        // bfn.(s0&s1|~s2)
        subfunc += format("(", i.getBfnFc().c_str(), ")");
      } else {
        // use a raw value
        // emit<uint32_t>(i.getBfnFc().value);
        subfunc = fmtHex((int)i.getBfnFc().value, 2);
      }
      break;
    case Op::DPASW:
    case Op::DPAS:
      subfunc = ToSyntax(i.getDpasFc());
      break;
    default:
      break;
    }

    emitPredOpSubfuncExecInfoFlagReg(i, i.getOpSpec().mnemonic.str(), subfunc);
    // implicit accumulator def/uses
    if (opts.liveAnalysis) {
      RegSet accIn {model};
      accIn.addSourceImplicit(i);
      RegSet accOu {model};
      accOu.addDestinationImplicit(i);
      if (!accIn.empty() || !accOu.empty()) {
        emit(", \"implicit\":{");
        bool emittedInputs = emitDepInputs(i, accIn, false);
        emitDepOutputs(i, accOu, emittedInputs);
        emit("}");
      }
    }

    withIndent([&] {
      emit(",\n");
      emitDstCommaNewline(i);
      emitIndent();
      if (i.getSourceCount() > 0) {
        emit("\"srcs\":[\n");
        withIndent([&] {
          for (unsigned srcIx = 0; srcIx < i.getSourceCount(); srcIx++) {
            emitIndent();
            emitSrc(i, srcIx);
            if (srcIx != i.getSourceCount() - 1)
              emit(",\n");
          }
          // if it's a send, create fake extra send parameters
          bool hasSendDescs = i.getOpSpec().isAnySendFormat();
          if (hasSendDescs) {
            auto emitDesc = [&](SendDesc sd) {
              emit(",\n");
              emitIndent();
              emit("{");
              if (sd.type == SendDesc::Kind::REG32A) {
                RegSet rs {model};
                emitKindField(Operand::Kind::DIRECT);
                emit(", \"reg\":"); emitRegValue(RegName::ARF_A, sd.reg);
                rs.setSrcRegion(RegName::ARF_A, sd.reg, Region::SRC110, 2, 16);
                emitDepInputs(i, rs);
              } else {
                emitKindField(Operand::Kind::IMMEDIATE);
                emit(", \"value\":\"0x", hex(sd.imm), "\"");
              }
              emit("}");
            };
            emitDesc(i.getExtMsgDescriptor());
            emitDesc(i.getMsgDescriptor());
          }
        });
        emit("\n");
        emitIndent();
        emit("]");
      } else {
        emit("\"srcs\":[]"); // so we print *something* (for next comma)
      }
    });
  } // emitNormalInst

  /////////////////////////////////////////////////////////////////////////
  bool emitLdStInst(const Instruction &i) {
    const auto desc = i.getMsgDescriptor();
    if (desc.isReg()) {
      // given a register descriptor, we've no hope of decoding the op
      // fallback to the regular canonical send path
      return false;
    }
    const auto exDesc = i.getExtMsgDescriptor();
    const auto sfid = i.getSendFc();
    DecodeResult di;
    bool defaultDecode = i.getOpSpec().isAnySendFormat();
    if (defaultDecode) {
      di = tryDecode(platform(), sfid, i.getExecSize(),
                                i.getExtImmOffDescriptor(),
                                exDesc, desc, nullptr);
    }
    if (!di) {
      // if message decode failed fallback to the canonical send syntax
      return false;
    } else if (!sendOpSupportsSyntax(platform(), di.info.op, sfid)) {
      // if decode succeeded but the syntax isn't supported, then bail
      return false;
    }

    emitPredOpSubfuncExecInfoFlagReg(i, di.syntax.mnemonic, di.syntax.controls);

    emit(",\n");
    withIndent([&] {
      emitIndent();
      if (!di.syntax.isStore()) {
        // there is an explicit destination
        const Operand &dst = i.getDestination();
        emit("\"dst\":{");
        emitSendDataRegDst(i, dst.getDirRegName(), dst.getDirRegRef().regNum,
                           i.getDstLength());
        emit("},\n");
      }

      /////////////////////////////////////////////
      // sources
      emitIndent();
      emit("\"srcs\":[\n");
      withIndent([&] {
        // everyone has src0 as an address
        emitIndent();
        emit("{\"kind\":\"AD\", ");
        emitSendSrc0AddressFields(i);
        emitSendSrc0AddrSurfFields(i, di.info);
        emit("},\n");

        //////////////////////////////////////
        // src1 (if not the old unary send)
        if (i.getSourceCount() > 1) {
          emitIndent();
          emitSendPayloadSrc(i, 1, "DA");
          emit("\n");
        }
      });

      emitIndent();
      emit("]"); // end of srcs:[..]
    });

    return true;
  }

  /////////////////////////////////////////////////////////////////////////
  // op+subfunc+exec size and exec offset
  void emitPredOpSubfuncExecInfoFlagReg(const Instruction &i,
                                        const std::string& mnemonic,
                                        const std::string& subop) {
    emitPred(i);

    emit(", \"op\":\"", mnemonic, "\"");
    if (!subop.empty()) {
      const auto& sf = subop[0] == '.' ? subop.substr(1) : subop;
      emit(", \"subop\":\"", sf, "\"");
    }

    emit(", \"es\":", ToSyntax(i.getExecSize()));
    if (i.getChannelOffset() != ChannelOffset::M0)
      emit(", \"eo\":", 4 * (int)(i.getChannelOffset()));

    if (i.hasFlagModifier()) {
      emit(", \"fm\":{\"cond\":\"", ToSyntax(i.getFlagModifier()), "\"");
      RegSet rs {model};
      rs.addFlagModifierOutputs(i);
      emitDepOutputs(i, rs);
      emit("}");
    }

    emitFlagReg(i);
  }


  // pred:{func:[null|""|".any"|".all"|...],inv:[true|false]"",wren:[true|false]}
  void emitPred(const Instruction &i) {
    Predication p = i.getPredication();
    if (i.hasPredication()) {
      emit(", \"pred\":");
      emit("{");
      if (p.inverse) {
        emit("\"inv\":true, ");
      }
      emit("\"func\":\"", ToSyntax(p.function), "\"");
      RegSet rs {model};
      rs.addPredicationInputs(i);
      emitDepInputs(i, rs);
      emit("}");
    }
    //
    if (i.getMaskCtrl() == MaskCtrl::NOMASK) {
      emit(", \"wren\":true");
    }
  }

  // {flag:{reg:...}}
  void emitFlagReg(const Instruction &i) {
    if (i.hasPredication() || (i.hasFlagModifier() && !i.is(Op::SEL))) {
      emit(", \"freg\":");
      emitRegValue(RegName::ARF_F, i.getFlagReg());
    }
  }

  // dst:{kind=..., REST, sat:T|F, uses:...}
  void emitDstCommaNewline(const Instruction &i) {
    const OpSpec &os = i.getOpSpec();
    if (os.isAnySendFormat()) {
      emitIndent();
      emit("\"dst\":");
      emitSendDstValue(i);
      emit(",\n");
      return;
    }
    const Operand &dst = i.getDestination();
    if (!i.getOpSpec().supportsDestination()) {
      return;
    }
    emitIndent();
    emit("\"dst\":");
    emit("{");
    emitKindField(dst.getKind());
    switch (dst.getKind()) {
    case Operand::Kind::DIRECT:
      emit(", \"reg\":");
      emitRegValue(dst.getDirRegName(), dst.getDirRegRef());
      break;
    case Operand::Kind::MACRO:
      emit(", \"reg\":");
      emitRegValue(dst.getDirRegName(), dst.getDirRegRef());
      emit(", ");
      emitMathMacroExtField(dst.getMathMacroExt());
      break;
    case Operand::Kind::INDIRECT:
      emit(", \"areg\":");
      emitRegValue(RegName::ARF_A, dst.getIndAddrReg());
      emit(", \"aoff\":", dst.getIndImmAddr());
      break;
    default:
      break;
    }
    if (dst.getDstModifier() == DstModifier::SAT) {
      emit(", \"sat\":true");
    }
    if (!os.hasImplicitDstRegion(i.isMacro())) {
      emitRgnField(i, dst.getRegion(), true);
    }
    emitTypeField(dst.getType());

    // uses
    RegSet rs {model};
    rs.addDestinationOutputs(i);
    emitDepOutputs(i, rs);
    emit("},\n");
  }

  void emitMathMacroExtField(MathMacroExt mme) {
    emit("\"mme\":\"", ToSyntax(mme).substr(1), "\"");
  }

  void emitSrc(const Instruction &i, int srcIx) {
    const OpSpec &os = i.getOpSpec();
    if (os.isAnySendFormat()) {
      emitSendSrc(i, srcIx);
      return;
    }
    const Operand &src = i.getSource(srcIx);
    emit("{");
    emitKindField(src.getKind());
    bool immOrLbl = false;
    switch (src.getKind()) {
    case Operand::Kind::DIRECT:
      emitSrcModifierField(src.getSrcModifier());
      emit(", \"reg\":");
      emitRegValue(src.getDirRegName(), src.getDirRegRef());
      emitSrcRgn(i, srcIx);
      break;
    case Operand::Kind::MACRO:
      emitSrcModifierField(src.getSrcModifier());
      emit(", \"reg\":");
      emitRegValue(src.getDirRegName(), src.getDirRegRef());
      emit(", ");
      emitMathMacroExtField(src.getMathMacroExt());
      emitSrcRgn(i, srcIx);
      break;
    case Operand::Kind::INDIRECT:
      emitSrcModifierField(src.getSrcModifier());
      emit(", \"areg\":");
      emitRegValue(RegName::ARF_A, src.getIndAddrReg());
      emit(", \"aoff\":", src.getIndImmAddr());
      emitSrcRgn(i, srcIx);
      break;
    case Operand::Kind::IMMEDIATE: {
      emit(", \"value\":\"");
      auto imm = src.getImmediateValue();
      switch (src.getType()) {
      case Type::UB:
        emitHex(imm.u8);
        break;
      case Type::UW:
        emitHex(imm.u16);
        break;
      case Type::UD:
        emitHex(imm.u32);
        break;
      case Type::UQ:
        emitHex(imm.u64);
        break;
      case Type::B:
        emitDecimal(imm.s8);
        break;
      case Type::W:
        emitDecimal(imm.s16);
        break;
      case Type::D:
        emitDecimal(imm.s32);
        break;
      case Type::Q:
        emitDecimal(imm.s64);
        break;
      case Type::BF:
        emitHex(imm.u16);
        break;
      case Type::HF:
        if (opts.hexFloats) {
          emitHex(imm.u16);
        } else {
          FormatFloat(o, imm.u16);
        }
        break;
      case Type::F:
      case Type::TF32:
        if (opts.hexFloats) {
          emitHex(imm.u32);
        } else {
          emitFloat(imm.f32);
        }
        break;
      case Type::DF:
        if (opts.hexFloats) {
          emitHex(imm.u64);
        } else {
          emitFloat(imm.f64);
        }
        break;
      case Type::BF8:
      case Type::HF8:
        emitHex(imm.u8);
        break;
      case Type::V:
        emitHex(imm.u32);
        break;
      case Type::UV:
        emitHex(imm.u32);
        break;
      case Type::VF:
        emitHex(imm.u32);
        break;
      default:
        emitIrError("invalid type for imm");
      }
      emit("\"");
      immOrLbl = true;
      break;
    }
    case Operand::Kind::LABEL: {
      const Block *b = src.getTargetBlock();
      emit(", \"target\":\"");
      emitLabel(b ? b->getPC() : src.getImmediateValue().s32);
      emit("\"");
      immOrLbl = true;
      break;
    }
    default:
      break;
    }

    if (!os.hasImplicitSrcType(srcIx, immOrLbl)) {
      emitTypeField(src.getType());
    }

    // even immediate fields will have this; it'll just be empty
    RegSet rs {model};
    rs.addSourceOperandInput(i, srcIx);
    if (!rs.empty())
      emitDepInputs(i, rs);

    emit("}");
  }

  void emitSendDstValue(const Instruction &i) {
    const Operand &dst = i.getDestination();
    emit("{");
    if (i.getDstLength() >= 0) {
      // new style send (r13:4) (take bits from desc)
      emitSendDataRegDst(i, dst.getDirRegName(), dst.getDirRegRef().regNum,
                         i.getDstLength());
    } else {
      emit("\"kind\":\"RD\"");
      emit(", \"reg\":");
      emitRegValue(dst.getDirRegName(), dst.getDirRegRef());
    }
    emit("}");
  }

  void emitSrcModifierField(SrcModifier sm) {
    if (sm == SrcModifier::NONE)
      return;
    emit(", \"mods\":", sm == SrcModifier::NEG       ? "\"n\""
                        : sm == SrcModifier::ABS     ? "\"a\""
                        : sm == SrcModifier::NEG_ABS ? "\"na\""
                                                     : "\"\"");
  }

  void emitKindField(Operand::Kind k) {
    emit("\"kind\":");
    switch (k) {
    case Operand::Kind::DIRECT:
      emit("\"RD\"");
      break;
    case Operand::Kind::MACRO:
      emit("\"RM\"");
      break;
    case Operand::Kind::INDIRECT:
      emit("\"RI\"");
      break;
    case Operand::Kind::LABEL:
      emit("\"LB\"");
      break;
    case Operand::Kind::IMMEDIATE:
      emit("\"IM\"");
      break;
    default:
      emitIrError("bad operand kind");
    }
  }

  void emitSrcRgn(const Instruction &i, int srcIx) {
    const Operand &src = i.getSource(srcIx);
    if (!i.getOpSpec().hasImplicitSrcRegion(srcIx, i.getExecSize(),
                                           i.isMacro())) {
      emitRgnField(i, src.getRegion(), false);
    }
  }

  void emitSendSrc(const Instruction &i, int srcIx) {
    const Operand &src = i.getSource(srcIx);
    if (srcIx == 0 && i.getSrc0Length() >= 0) {
      emit("{");
      emit("\"kind\":\"AD\", ");
      emitSendSrc0AddressFields(i);
      const auto di = tryDecode(i, nullptr);
      if (di) {
        comments.push_back(di.info.description);
        emitSendSrc0AddrSurfFields(i, di.info);
      }
      emit("}");
    } else if (srcIx == 1 && i.getSrc1Length() >= 0) {
      emitSendPayloadSrc(i, 1, "DA");
    } else {
      // old send operand (treat as raw direct register access)
      // this is because we lack a known payload length
      // so the user is on their own
      emit("{");
      emit("\"kind\":\"RD\"");
      emit(", \"reg\":");
      emitRegValue(src.getDirRegName(), src.getDirRegRef());
      // the best we can do is guess it's 1 without decoding the message
      RegSet rs {model};
      rs.addSourceOperandInput(i, srcIx);
      emitSendPayloadDeps(i, rs, src.getDirRegName(), src.getDirRegRef().regNum,
                          1, true);
      emit("}");
    }
  }

  void emitSendSrc0AddressFields(const Instruction &i) {
    emit("\"areg\":");
    const auto &src0 = i.getSource(0);
    if (src0.getKind() == Operand::Kind::DIRECT) {
      emitRegValue(src0.getDirRegName(), src0.getDirRegRef().regNum);
    } else if (src0.getKind() == Operand::Kind::INDIRECT) { // indirect send
      emitRegValue(RegName::ARF_S, src0.getIndAddrReg());
    } else {
      emit("\"???\"");
    }
    emit(", \"alen\":", i.getSrc0Length());
    // payload dependencies
    RegSet rs {model};
    rs.addSourceOperandInput(i, 0);
    emitDepInputs("adefs", i, rs);
  }

  void emitSendSrc0AddrSurfFields(const Instruction &i, const MessageInfo &mi) {
    if (mi.immediateOffsetBlock2dX || mi.immediateOffsetBlock2dY) {
      emit(", \"aoff\":[",
        mi.immediateOffsetBlock2dX, ",", mi.immediateOffsetBlock2dY, "]");
    } else if (mi.immediateOffset) {
      emit(", \"aoff\":", mi.immediateOffset);
    }

    emit(", \"stype\":");

    auto emitStateful = [&] (const char *addrType) {
      emit(addrType);
      if (mi.surfaceId.isReg()) {
        emit(", \"soff\":");
        RegSet surfOffDeps {model};
        surfOffDeps.setSrcRegion(RegName::ARF_A, mi.surfaceId.reg,
                                 Region::SRC110, 2, 16);
        emitRegValue(RegName::ARF_A, mi.surfaceId.reg);
        emitDepInputs("sdefs", i, surfOffDeps);
      } else {
        emit(", \"soff\":");
        emitDecimal(mi.surfaceId.imm);
      }
    };


    switch (mi.addrType) {
    case AddrType::INVALID:
      emit("\"invalid\"");
      break;
    case AddrType::BTI:
      emitStateful("\"bti\"");
      break;
    case AddrType::BSS:
      emitStateful("\"bss\"");
      break;
    case AddrType::SS:
      emitStateful("\"ss\"");
      break;
    case AddrType::FLAT: {
      emit("\"flat\"");
      break;
    }
    default:
      emitIrError("invalid surface type");
    }
  }

  void emitSendPayloadSrc(const Instruction &i, int srcIx,
                          const char *kind = nullptr) {
    const Operand &src = i.getSource(srcIx);
    RegName regName = src.getDirRegName();
    int regCount = srcIx == 0 ? i.getSrc0Length() : i.getSrc1Length();
    emit("{");
    if (kind) {
      emit("\"kind\":\"", kind, "\", ");
    }
    emit("\"reg\":");
    emitRegValue(regName, src.getDirRegRef().regNum);
    emit(", \"len\":", regCount);
    RegSet rs {model};
    rs.addSourceOperandInput(i, srcIx);
    emitSendPayloadDeps(i, rs, regName, regCount, regCount, true);
    emit("}");
  }

  void emitSendDataRegDst(const Instruction &i, RegName dataReg, int regNum,
                          int regLen) {
    emit("\"kind\":\"DA\"");
    emitSendPayloadFields(dataReg, regNum, regLen);
    emitSendPayloadDeps(i, EMPTY_SET, dataReg, regNum, regLen, false);
  }

  void emitSendPayloadFields(RegName reg, int regNum, int numRegs) {
    emit(", \"reg\":");
    emitRegValue(reg, regNum);
    emit(", \"len\":", numRegs);
  }
  void emitSendPayloadDeps(const Instruction &i, const RegSet &regStats,
                           RegName reg, int regNum, int numRegs, bool isRead) {
    if (isRead) {
      emitDepInputs(i, regStats);
    }
  }

  void emitRgnField(const Instruction &i, Region r, bool dst = false) {
    if (!dst) {
      // fix ternary regions
      if (r.getVt() == Region::Vert::VT_INVALID &&
          r.getWi() == Region::Width::WI_INVALID) {
        // ternary src2
        //    officially it was: w = 0 if h is 0 or E; v = w * h
        // use normalized regions here to simplify
        //   <0> ==> <0;1,0>
        //   <K> ==> <K;1,0>
        auto hz = r.getHz();
        // auto es = i.getExecSize();
        if (hz == Region::Horz::HZ_0) {
          r = Region::SRC010;
        } else {
          r = Region::SRC110;
          r.set(Region::Vert(hz));
        }
      } else if (r.getWi() == Region::Width::WI_INVALID) {
        // ternary src0/src1
        //  w = 1 when v & h are 0
        //    = v when h = 0
        //    = v/h otherwise
        auto vt = r.getVt();
        auto hz = r.getHz();
        if (vt == Region::Vert::VT_0 && hz == Region::Horz::HZ_0) {
          r.set(Region::Width::WI_1);
        } else if (hz == Region::Horz::HZ_0) {
          r.set(Region::Width(int(vt)));
        } else {
          r.set(Region::Width(int(vt) / int(hz)));
        }
      }
    }
    emitRgnField(r, dst);
  } // emitRgnField(Instruction)

  void emitRgnField(Region r, bool dst = false) {
    if (r == Region::INVALID)
      return;
    emit(", \"rgn\":");
    if (dst) {
      emit("{\"h\":", int(r.getHz()), "}");
    } else {
      emit("{");
      if (r.getVt() != Region::Vert::VT_VxH) {
        emit("\"v\":", int(r.getVt()),",");
      }
      emit("\"w\":", int(r.getWi()), ",\"h\":", int(r.getHz()));
      emit("}");
    }
  } // emitRgnField(Region)

  void emitTypeField(Type t) {
    if (t == Type::INVALID) {
      return;
    }
    emit(", \"type\":\"", ToSyntax(t).substr(1), "\""); // Type::UQ => "uq"
  }

  void emitRegDist(const SWSB &di) {
    if (di.distType == SWSB::DistType::NO_DIST)
      return;
    emit(", \"regDist\":");
    auto emitPipeDist = [&](const char *pfx) {
      emit("\"", pfx, "@", (int)di.minDist, "\"");
    };
    switch (di.distType) {
    case SWSB::DistType::REG_DIST:
      emitPipeDist("");
      break;
    case SWSB::DistType::REG_DIST_ALL:
      emitPipeDist("A");
      break;
    case SWSB::DistType::REG_DIST_FLOAT:
      emitPipeDist("F");
      break;
    case SWSB::DistType::REG_DIST_INT:
      emitPipeDist("I");
      break;
    case SWSB::DistType::REG_DIST_LONG:
      emitPipeDist("L");
      break;
    case SWSB::DistType::REG_DIST_MATH:
      emitPipeDist("M");
      break;
    case SWSB::DistType::REG_DIST_SCALAR:
      emitPipeDist("S");
      break;
    default:
      emit("\"?\"");
      break;
    }
  }

  void emitSbid(const SWSB &di) {
    if (di.tokenType == SWSB::TokenType::NOTOKEN)
      return;

    emit(", \"sbid\":{");
    auto emitSwsbEvent = [&](const char *function) {
      emit("\"id\":", (int)di.sbid);
      emit(", \"func\":\"", function, "\"");
    };
    switch (di.tokenType) {
    case SWSB::TokenType::DST:
      emitSwsbEvent(".dst");
      break;
    case SWSB::TokenType::SRC:
      emitSwsbEvent(".src");
      break;
    case SWSB::TokenType::SET:
      emitSwsbEvent("");
      break;
    default:
      emit("\"?\"");
      break;
    }
    emit("}");
  }

  void emitInstOpts(const Instruction &i) {
    static const InstOpt ALL_INST_OPTS[]{
        InstOpt::ACCWREN,     InstOpt::ATOMIC,  InstOpt::BREAKPOINT,
        InstOpt::COMPACTED,   InstOpt::EOT,     InstOpt::NOCOMPACT,
        InstOpt::NODDCHK,     InstOpt::NODDCLR, InstOpt::NOPREEMPT,
        InstOpt::NOSRCDEPSET, InstOpt::SWITCH,  InstOpt::SERIALIZE,
        InstOpt::EXBSO,       InstOpt::CPS,
    };

    InstOptSet ios = i.getInstOpts();
    bool first = true;
    std::stringstream ss;
    auto emitSeparator = [&]() {
      if (first)
        first = false;
      else
        ss << ",";
    };
    for (size_t i = 0;
         i < sizeof(ALL_INST_OPTS) / sizeof(ALL_INST_OPTS[0]) && !ios.empty();
         i++) {
      if (ios.contains(ALL_INST_OPTS[i])) {
        emitSeparator();
        ios.remove(ALL_INST_OPTS[i]);
        ss << "\"" << ToSyntax(ALL_INST_OPTS[i]) << "\"";
      }
    }

    const auto &di = i.getSWSB();
    // special token and dist/token won't co-exist in the same swsb
    // no need to insert "," after this since there must not be
    // dist/token swsb
    if (di.hasSpecialToken()) {
      IGA_ASSERT(!di.hasDist() && !di.hasToken(), "malformed SWSB IR");
      if (di.spToken == SWSB::SpecialToken::NOACCSBSET) {
        emitSeparator();
        ss << "NoAccSBSet";
      }
    }
    auto s = ss.str();
    if (!s.empty())
      emit(", \"opts\":[", s ,"]");
  }

  void emitRegValue(RegName rn, RegRef rr) {
    emit("{""\"rn\":\"", ToSyntax(rn), "\"");
    if (rr.regNum) {
      emit(",\"r\":", rr.regNum);
    }
    if (rr.subRegNum) {
      emit(",\"sr\":", rr.subRegNum);
    }
    emit("}");
  }
  void emitRegValue(RegName rn, int regNum, int sr = 0) {
    emitRegValue(rn, RegRef((int16_t)regNum, (int16_t)sr));
  }

  /////////////////////////////////////////////////////////////////////////
  // call this for operands that are instruction outputs
  bool emitDepOutputs(
    const Instruction &i, const RegSet &rs, bool preComm = true)
  {
    // 'i' is writing 'rs' emit all pairs where this is the
    // def and report them as uess
    if (opts.liveAnalysis == nullptr) {
      return false;
    }
    const auto &ds = depDefs[i.getID()];
    if (ds.empty())
      return false;

    std::set<int> ids; // make unique and in ascending order
    for (const Dep *d : ds) {
      if (d->use != nullptr && rs.intersects(d->values)) {
        ids.insert(d->use->getID());
      }
    }

    return emitSetIfNonEmpty("uses", ids, preComm);
  }

  // call this for operands that are instruction inputs
  bool emitDepInputs(
    const Instruction &i, const RegSet &rs, bool preComm = true)
  {
    return emitDepInputs("defs", i, rs, preComm);
  }
  bool emitDepInputs(const char *which,
    const Instruction &i, const RegSet &rs, bool preComm = true)
  {
    // 'i' is writing 'rs' emit all pairs where this is the
    // def and report them as uess
    if (opts.liveAnalysis == nullptr) {
      return false;
    }
    // the users of 'rs' are all those that use this set
    if (rs.empty())
      return false;
    const auto &c = depUses[i.getID()];
    if (c.empty())
      return false;

    std::set<int> ids; // make unique and in ascending order
    for (const Dep *d : c) {
      if (d->def != nullptr && rs.intersects(d->values)) {
        ids.insert(d->def->getID());
      }
    }

    return emitSetIfNonEmpty(which, ids, preComm);
  }

  bool emitSetIfNonEmpty(
    const char *which, const std::set<int> &ids, bool preComm)
  {
    if (ids.empty()) // no intersection with this set
      return false;
    if (preComm)
      emit(", ");
    emit("\"", which, "\":[");
    bool first = true;
    for (auto id : ids) {
      if (first)
        first = false;
      else
        emit(",");
      emit(id);
    }
    emit("]");
    return true;
  }
}; // JSONFormatterV2

namespace iga {
void FormatJSON2(std::ostream &o, const FormatOpts &opts, const Kernel &k,
                     const void *bits);
} // iga::

void iga::FormatJSON2(std::ostream &o, const FormatOpts &opts, const Kernel &k,
                     const void *bits) {
  JSONFormatterV2(o, opts, bits).emitKernel(k);
}

void iga::FormatInstructionJSON2(std::ostream &o, const FormatOpts &opts,
                                const Instruction &i, const void *bits) {
  JSONFormatterV2(o, opts, bits).emitInst(i);
}
