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

#ifndef _IR_BUILDER_HANDLER_HPP_
#define _IR_BUILDER_HANDLER_HPP_

#include "Kernel.hpp"
#include "../asserts.hpp"
#include "../ErrorHandler.hpp"
#include "../Frontend/IRToString.hpp"
#include "../Models/Models.hpp"

#include <map>
#include <set>
#include <vector>

namespace iga {
// The parser handler is called by the parser when various productions are
// encountered.  This allows us to separate IR construction from the actual
// syntax processing.
//
// This is needed since we have to save information about the operands
// during the parse.  We can't create operands as we see them since we don't
// have an existing Instruction * until endInstruction().   E.g. createSend
// requires us having seen exDesc and desc.  Since these follow the operands;
// hence, we must save operand info as we go.
struct OperandInfo {
    Loc                      loc;
    Operand::Kind            kind;

    union // operand register/immediate value info
    {
        struct // a register (direct or indirect)
        {
            union // optional modifier (e.g. -r12, ~r12, (abs) (sat))
            {
                SrcModifier  regOpSrcMod;
                DstModifier  regOpDstMod;
            };
            struct // the actual register (GRF for Operand::Kind::INDIRECT)
            {
                RegName      regOpName;    // e.g. r#, a#, null, ...
                Region       regOpRgn;     // e.g. <1>, <8;8,1>
                ImplAcc      regOpImplAcc; // e.g. implicit accumulator
            };
            union // direct vs. indirect
            {
                RegRef       regOpReg; // direct operands
                struct // indirect operands
                {
                    RegRef   regOpIndReg; // a0.4
                    int16_t  regOpIndOff; // e.g. "16" in "r[a0.4,16]"
                };
            };
        };
        struct // immediate operand
        {
            ImmVal           immValue;
            Block           *immBlock;
        };
    };
    std::string              immLabel; // has constructor
    Type                     type;
};


typedef std::map<std::string,Block*> BlockMap;
typedef std::vector<Block*>          BlockArr;
typedef std::set<const Block*>       BlockSet;


// Larger constructs, such as blocks, instructions, and whatnot
// generally consist of a ***Start() and ***End() method to denote the
// start and end of each feature.  Upon parse or semantic error, the
// parser may terminate early, and corresponding ***End() functions may
// not be called in those cases.
//
// The methods are roughly called in syntax order, but this isn't a strict
// requirement and the handler should not make any assumptions about that.
class InstBuilder {
    const Model&                m_model;
    ErrorHandler&               m_errorHandler;
    Kernel                     *m_kernel;

    // per-instruction state
    Loc                         m_loc;
    Predication                 m_predication;
    const OpSpec               *m_opSpec;
    BranchCntrl                 m_brnchCtrl;

    RegRef                      m_flagReg; // shared by predication / condition modifier

    ExecSize                    m_execSize;
    ChannelOffset               m_chOff;
    MaskCtrl                    m_maskCtrl;

    FlagModifier                m_flagModifier;

    DstModifier                 m_dstModifier;
    OperandInfo                 m_dst;
    OperandInfo                 m_srcs[3];
    int                         m_nSrcs;

    SendDescArg                 m_exDesc;
    SendDescArg                 m_desc;
    InstOptSet                  m_instOpts;

    Block                      *m_currBlock;

    BlockMap                    m_blocks;
    std::map<Block*,Loc>        m_blocksRefed;
    BlockSet                    m_blocksDefd; // all defined
    BlockArr                    m_blockDefinitionOrder; // same as above, but in def order
    std::map<Block*,Loc>        m_blocksNumericTargets;

    uint32_t                    m_pc; // current PC
    uint32_t                    m_nextId; // next instruction id



    void clearInstState() {
        m_predication.function = PredCtrl::NONE;
        m_predication.inverse = false;

        m_flagReg = REGREF_ZERO_ZERO;

        m_opSpec = nullptr;

        m_execSize = ExecSize::SIMD1;
        m_chOff = ChannelOffset::M0;
        m_brnchCtrl = BranchCntrl::OFF;
        m_maskCtrl = MaskCtrl::NORMAL;

        m_flagModifier = FlagModifier::NONE;

        m_dstModifier = DstModifier::NONE;

        m_dst.kind = Operand::Kind::INVALID;
        m_dst.immBlock = nullptr;
        m_dst.immLabel.clear();
        for (auto &m_src : m_srcs) {
            m_src.regOpSrcMod = SrcModifier::NONE;
            m_src.kind = Operand::Kind::INVALID;
            m_src.immBlock = nullptr;
            m_src.immLabel.clear();
        }
        m_nSrcs = 0;

        m_exDesc.imm = 0;
        m_desc.imm = 0;

        m_instOpts.clear();
    }

    Block *lookupBlock(const std::string &label) {
        Block *b;
        auto itr = m_blocks.find(label);
        if (itr == m_blocks.end()) {
            b = m_kernel->createBlock();
            m_blocks[label] = b;
        } else {
            b = itr->second;
        }
        return b;
    }
    Block *lookupBlockNumeric(const Loc &loc, const int64_t absPc) {
        std::stringstream ss;
        ss << "." << absPc;
        Block *b = lookupBlock(ss.str());
        if (m_blocksNumericTargets.find(b) == m_blocksNumericTargets.end()) {
            // this constitutes a "definition"
            m_blocksNumericTargets[b] = loc;
            b->setOffset((int32_t)absPc);
            m_blocksDefd.insert(b);
        }
        return b;
    }

    // splits block b at x
    //   lbl is one of the numeric labels targeting this numeric block
    //   (for error reporting)
    // x must be within b
    // => if x extends past the end of the block, it's an error
    // => if x doesn't land on an instruction boundary, it's an error
    void splitBlock(const Loc &xReferrerLoc, Block *x, Block *b) {
        InstList &xis = x->getInstList();
        IGA_ASSERT(xis.empty(), "numeric target should be an empty block");

        InstList &bis = b->getInstList();
        int32_t pc = b->getOffset();

        for (auto bisIter = bis.begin(); bisIter != bis.end(); ++bisIter) {
            if (x->getOffset() == pc) {
                // found the split location
                // copy the tail
                xis.insert(xis.begin(), bisIter, bis.end());
                // erase the old tail
                bis.erase(bisIter, bis.end());
                // Now we have block b followed by x
                // set the source location for this block to be the
                // same location as the first instruction, if there is one
                if (xis.size() > 0) {
                    x->setLoc(xis.front()->getLoc());
                }
                return;
            } else if (x->getOffset() < pc) {
                // we passed over it (doesn't land on an instruction boundary)
                m_errorHandler.reportError(xReferrerLoc,
                    "numeric label targets the middle of an instruction");
                return;
            } else {
                // else, step to the next instruction
                pc += (*bisIter)->hasInstOpt(InstOpt::COMPACTED) ? 8 : 16;
            }
        } // for block instructions
        if (x->getOffset() > pc) {
            // out of bounds (past end of kernel)
            m_errorHandler.reportError(xReferrerLoc,
                "numeric label targets past the end of the kernel");
        } // else: targets end of program exactly
    }

public:
    InstBuilder(Kernel *kernel, ErrorHandler &e)
        : m_model(kernel->getModel())
        , m_errorHandler(e)
        , m_kernel(kernel)
    {
    }

    ErrorHandler &errorHandler() {return m_errorHandler;}

    // Called at the beginning of the program
    void ProgramStart() {
        m_pc = 0;
        m_nextId = 1;
    }


    // Called at the end of the program
    void ProgramEnd() {
        // Ensure all label references were defined
        // 1.a. Append the block to the kernel
        // 1.b. Create an efficient lookup for those blocks defined
        //      now done incrementally by m_blocksDefd
        // 2. Iterate all references to blocks
        //    Ensure that each was defined
        for (auto itr = m_blocksRefed.begin();
            itr != m_blocksRefed.end();
            itr++)
        {
            Block *b = itr->first;
            bool notNumericLabel = m_blocksNumericTargets.find(b) == m_blocksNumericTargets.end();
            if (notNumericLabel && m_blocksDefd.find(b) == m_blocksDefd.end()) {
                // not a numeric label and never defined
                m_errorHandler.reportError(itr->second, "undefined label");
            }
        }

        // now we have to insert all the numeric targets in block definition
        // order and possibly split other blocks into multiple pieces
        // E.g.
        // L0:
        //     nop
        //     add ... {Compacted}  // need label ".16" here
        //     (f0) while -8
        //     jmpi  16
        // L1:                      // need label ".56" here for jmpi
        //     nop
        //
        // The while -8 induces a new block at offset 16
        // This expands the CFG into
        //
        // L0:
        //     nop
        // .16:
        //     add ... {Compacted}  // need label ".16" here
        //     (f0) while -8
        //     jmpi  16
        // .56:
        // L1:
        //     nop
        // Note, we could reuse L1, but that would entail searching for all
        // instructions that target .56 and replacing their operands.  This
        // is sloppier, but get's the job done.
        //
        // Don't try and assemble with numeric labels....
        for (auto itr = m_blocksNumericTargets.begin();
            itr != m_blocksNumericTargets.end();
            itr++)
        {
            Block *x = itr->first;
            if (x->getOffset() < 0) {
                m_errorHandler.reportError(itr->second,
                    "points to before beginning of kernel listing");
                continue;
            }

            // walk through the blocks and find the insertion point for x
            // invariant: x->getOffset() < b->getOffset()
            //   base case: from above
            //   inductive case: from below (see the 'continue' path)
            for (size_t i = 0; i < m_blockDefinitionOrder.size(); i++) {
                Block *b = m_blockDefinitionOrder[i];
                if (x->getOffset() == b->getOffset()) {
                    // can insert this block just above current block
                    // without any splitting needed
                    m_blockDefinitionOrder.insert(
                        m_blockDefinitionOrder.begin() + i, x);
                } else { // x->getOffset() > b->getOffset()
                    if (i < m_blockDefinitionOrder.size() - 1) {
                        // not at the end, look at successor
                        Block *c = m_blockDefinitionOrder[i + 1];
                        if (x->getOffset() >= c->getOffset()) {
                            // need to iterate to next block
                            continue;
                        }
                        // split block b
                        splitBlock(itr->second, x, b);
                    } else {
                        // this is the last block
                        // better be within this block or it's out of bounds
                        splitBlock(itr->second, x, b);
                    }
                    // insert the block *after* b (since it splits b)
                    m_blockDefinitionOrder.insert(
                        m_blockDefinitionOrder.begin() + i + 1, x);
                }
                break; // on to the next numeric label
            }
        }

        // after this mess is done we can insert all the blocks in
        // their natural offset order
        for (auto b : m_blockDefinitionOrder) {
            m_kernel->appendBlock(b);
        }
    }


    void BlockStart(const Loc &lblLoc, const std::string &label) {
        auto itr = m_blocks.find(label);
        if (itr != m_blocks.end()) {
            // This label does not correspond to a block yet
            // (It might be a forward reference though)
            const Block *b = itr->second;
            if (m_blocksDefd.find(b) != m_blocksDefd.end()) {
                std::stringstream ss;
                ss << "redefinition of label (previously defined on line " <<
                    b->getLoc().line << ")";
                m_errorHandler.reportError(lblLoc, ss.str());
            }
        }
        m_currBlock = lookupBlock(label);
        m_currBlock->setLoc(lblLoc);
        m_currBlock->setOffset(m_pc);

        m_blocksDefd.insert(m_currBlock);
        m_blockDefinitionOrder.push_back(m_currBlock);
    }


    void BlockEnd(uint32_t extent) {
        Loc newLoc = m_currBlock->getLoc();
        newLoc.extent = extent;
        m_currBlock->setLoc(newLoc);
        m_currBlock = nullptr;
    }


    void InstStart(const Loc &loc) {
        clearInstState();
        m_loc = loc;
    }


    void InstEnd(uint32_t extent) {
        // set the full instruction length (in chars)
        m_loc.extent = extent;

        Instruction *inst = nullptr;
        if (m_opSpec->isMathSubFunc()) {
            inst =
                m_kernel->createBasicInstruction(
                    *m_opSpec,
                    m_predication,
                    m_flagReg,
                    m_execSize,
                    m_chOff,
                    m_maskCtrl,
                    m_flagModifier);
        } else if (m_opSpec->format == OpSpec::Format::SYNC_UNARY) {
            inst =
                m_kernel->createBasicInstruction(
                    *m_opSpec,
                    m_predication,
                    m_flagReg,
                    m_execSize,
                    m_chOff,
                    m_maskCtrl,
                    FlagModifier::NONE);
        } else if (m_opSpec->isBranching()) {
            inst =
                m_kernel->createBranchInstruction(
                    *m_opSpec,
                    m_predication,
                    m_flagReg,
                    m_execSize,
                    m_chOff,
                    m_maskCtrl,
                    m_brnchCtrl);
        } else if (m_opSpec->isSendOrSendsFamily()) {
            inst =
                m_kernel->createSendInstruction(
                    *m_opSpec,
                    m_predication,
                    m_flagReg,
                    m_execSize,
                    m_chOff,
                    m_maskCtrl,
                    m_exDesc,
                    m_desc);
        } else if (m_opSpec->op == Op::NOP) {
            inst = m_kernel->createNopInstruction();
        } else if (m_opSpec->op == Op::ILLEGAL) {
            inst = m_kernel->createIllegalInstruction();
        } else {
            inst =
                m_kernel->createBasicInstruction(
                    *m_opSpec,
                    m_predication,
                    m_flagReg,
                    m_execSize,
                    m_chOff,
                    m_maskCtrl,
                    m_flagModifier);
        }
        inst->setLoc(m_loc);
        m_currBlock->appendInstruction(inst);

        if (m_opSpec->supportsDestination()) {
            if (m_dst.kind == Operand::Kind::DIRECT) {
                inst->setDirectDestination(
                    m_dstModifier,
                    m_dst.regOpName,
                    m_dst.regOpReg,
                    m_dst.regOpRgn.getHz(),
                    m_dst.type);
            } else if (m_dst.kind == Operand::Kind::MACRO) {
                inst->setMacroDestination(
                    m_dstModifier,
                    m_dst.regOpName,
                    m_dst.regOpReg,
                    m_dst.regOpImplAcc,
                    m_dst.type);
            } else { // Operand::Kind::INDIRECT
                inst->setInidirectDestination(
                    m_dstModifier,
                    m_dst.regOpIndReg,
                    m_dst.regOpIndOff,
                    m_dst.regOpRgn.getHz(),
                    m_dst.type);
            }
        } // end setting destinations

        // set source operands
        for (int i = 0; i < m_nSrcs; i++) {
            const OperandInfo &src = m_srcs[i];
            SourceIndex opIx = (SourceIndex)((int)SourceIndex::SRC0 + i);
            if (src.kind == Operand::Kind::DIRECT) {
                inst->setDirectSource(
                    opIx,
                    src.regOpSrcMod,
                    src.regOpName,
                    src.regOpReg,
                    src.regOpRgn,
                    src.type);
            } else if (src.kind == Operand::Kind::MACRO) {
                inst->setMacroSource(
                    opIx,
                    src.regOpSrcMod,
                    src.regOpName,
                    src.regOpReg,
                    src.regOpImplAcc,
                    src.type);
            } else if (src.kind == Operand::Kind::INDIRECT) {
                inst->setInidirectSource(
                    opIx,
                    src.regOpSrcMod,
                    src.regOpIndReg,
                    src.regOpIndOff,
                    src.regOpRgn,
                    src.type);
            } else if (src.kind == Operand::Kind::LABEL) {
                inst->setLabelSource(opIx, src.immBlock, src.type);
            } else if (src.kind == Operand::Kind::IMMEDIATE) {
                inst->setImmediateSource(opIx, src.immValue, src.type);
            } else {
                IGA_ASSERT_FALSE("unexpected src kind");
            }
        } // for
        inst->addInstOpts(m_instOpts);
        inst->setID(m_nextId++);
        inst->setDecodePC(m_pc);
        m_pc += inst->hasInstOpt(InstOpt::COMPACTED) ? 8 : 16;
    }


    void InstPredication(
        const Loc &loc,
        bool inv,
        const RegRef &flagReg,
        PredCtrl predCtrl)
    {
        m_predication.inverse = inv;
        m_predication.function = predCtrl;
        m_flagReg = flagReg;
    }


    void InstOp(const OpSpec *spec) {
        m_opSpec = spec;
    }


    // The absense of this implies branch control is either off or not
    // present (e.g. HSW or an instruction without that feature).
    //
    // E.g.   if.b   (16) 12 16
    void InstBrCtl(BranchCntrl bc) {
        m_brnchCtrl = bc;
    }


    void InstExecInfo(
        const Loc &execSizeLoc, ExecSize execSize,
        const Loc &execOffLoc, ChannelOffset execOff)
    {
        m_execSize = execSize;
        m_chOff = execOff;
    }


    void InstNoMask(const Loc &loc) {
        m_maskCtrl = MaskCtrl::NOMASK;
    }


    // The flag modifier (condition modifier)
    void InstFlagModifier(const RegRef &flagReg, FlagModifier flmodf) {
        m_flagModifier = flmodf;
        m_flagReg = flagReg;
    }


    /////////////////////////////////////////////
    // destination operand callbacks

    // (sat) applied to the destination operand
    void InstDstOpSaturate() {
        m_dstModifier = DstModifier::SAT;
    }

    // direct access
    //
    // e.g. r13.4<2>:t
    void InstDstOpRegDirect(
        const Loc &loc,
        const RegInfo &ri,
        const RegRef &reg,
        const Region::Horz &rgnHorz,
        const Type &ty)
    {
        m_dst.kind = Operand::Kind::DIRECT;

        m_dst.regOpDstMod = m_dstModifier;
        m_dst.regOpName = ri.reg;
        m_dst.regOpReg = reg;
        m_dst.regOpRgn.setDstHz(rgnHorz);
        m_dst.type = ty;
    }
    // implicit accumulator access
    //
    // e.g. r13.acc4:t
    void InstDstOpRegImplAcc(
        const Loc &loc,
        const RegInfo &ri,
        int regNum,
        ImplAcc implAcc,
        const Type &ty)
    {
        m_dst.kind = Operand::Kind::MACRO;

        m_dst.regOpDstMod = m_dstModifier;
        m_dst.regOpName = ri.reg;
        m_dst.regOpReg = {(uint8_t)regNum, 0};
        m_dst.regOpRgn = Region::DST1;
        m_dst.regOpImplAcc = implAcc;
        m_dst.type = ty;
    }

    // e.g. r[a0.4,16]<2>:t
    void InstDstOpRegIndirect(
        const Loc &loc,
        const RegRef &addrReg,
        int addrOff,
        const Region::Horz &rgnHorz,
        const Type &ty)
    {
        m_dst.kind = Operand::Kind::INDIRECT;

        m_dst.regOpDstMod = m_dstModifier;
        m_dst.regOpName = RegName::GRF_R;
        m_dst.regOpIndReg = addrReg;
        m_dst.regOpIndOff = (uint16_t)addrOff;
        m_dst.regOpRgn.setDstHz(rgnHorz);
        m_dst.type = ty;
    }

    // a more generic setter
    void InstDstOp(OperandInfo opInfo) {
        m_dst = opInfo;
    }

    /////////////////////////////////////////////
    // source operand callbacks

    // Direct register source operand
    //
    // e.g. r13.4<2>:t
    void InstSrcOpRegDirect(
        int srcOpIx, // index of the current source operand
        const Loc &loc,
        SrcModifier srcMod, // source modifiers on this operand
        const RegInfo &ri, // the type of register
        const RegRef &rr, // register/subregister
        const Region &rgn, // region parameters
        const Type &ty)
    {
        m_nSrcs = m_nSrcs < srcOpIx + 1 ? srcOpIx + 1 : m_nSrcs;
        OperandInfo &src = m_srcs[srcOpIx];
        src.kind = Operand::Kind::DIRECT;
        src.regOpSrcMod = srcMod;
        src.regOpName = ri.reg;
        src.regOpReg = rr;
        src.regOpRgn = rgn;
        src.type = ty;
    }
    // Implicit accumulator
    // e.g. r13.acc4:t
    void InstSrcOpRegImplAcc(
        int srcOpIx, // index of the current source operand
        const Loc &loc,
        SrcModifier srcMod, // source modifiers on this operand
        const RegInfo &ri, // the type of register
        int regNum,
        ImplAcc implAcc,
        const Type &ty)
    {
        m_nSrcs = m_nSrcs < srcOpIx + 1 ? srcOpIx + 1 : m_nSrcs;
        OperandInfo &src = m_srcs[srcOpIx];
        src.kind = Operand::Kind::MACRO;
        src.regOpSrcMod = srcMod;
        src.regOpName = ri.reg;
        src.regOpReg = {(uint8_t)regNum, 0};
        src.regOpRgn = Region::INVALID;
        src.regOpImplAcc = implAcc;
        src.type = ty;
    }
    // Parsed a source indirect operand
    //
    // e.g. "r[a0.4,16]<1,0>:f"
    void InstSrcOpRegIndirect(
        int srcOpIx, // index of the current source operand
        const Loc &loc,
        const SrcModifier &srcMod, // source modifiers on this operand
        const RegRef &addrReg,
        int addrOff, // e.g. 16 in r[a0.3,16] (0 if absent)
        const Region &rgn,
        const Type &ty)
    {
        m_nSrcs = m_nSrcs < srcOpIx + 1 ? srcOpIx + 1 : m_nSrcs;
        OperandInfo &src = m_srcs[srcOpIx];
        src.kind = Operand::Kind::INDIRECT;
        src.regOpSrcMod = srcMod;
        src.regOpName = RegName::GRF_R;
        src.regOpIndReg = addrReg;
        src.regOpIndOff = (uint16_t)addrOff;
        src.regOpRgn = rgn;
        src.type = ty;
    }


    // Called on a source immediate operand.  Some immedidate operands
    // will not have explicit types.  E.g. send descriptors.
    //
    // e.g. 14:d
    //      0x21424
    void InstSrcOpImmValue(
        int srcOpIx,
        const Loc &loc,
        const ImmVal &val,
        const Type &ty)
    {
        m_nSrcs = m_nSrcs < srcOpIx + 1 ? srcOpIx + 1 : m_nSrcs;
        OperandInfo &src = m_srcs[srcOpIx];
        src.kind = Operand::Kind::IMMEDIATE;
        src.immValue = val;
        src.type = ty;
    }


    // Called when an immediate label is encountered (e.g. on branches)
    void InstSrcOpImmLabel(
        int srcOpIx,
        const Loc &loc,
        const std::string &sym,
        Type type)
    {
        m_nSrcs = m_nSrcs < srcOpIx + 1 ? srcOpIx + 1 : m_nSrcs;
        OperandInfo &src = m_srcs[srcOpIx];
        src.kind = Operand::Kind::LABEL;
        src.immBlock = lookupBlock(sym);
        src.type = type;

        m_blocksRefed[src.immBlock] = loc;
    }


    // almost all cases
    void InstSrcOpImmLabelRelative(
      int srcOpIx,
      const Loc &loc,
      const int64_t relPc, // the actual PC relative to program start
      Type type)
    {
      InstSrcOpImmLabelAbsolute(srcOpIx, loc, relPc + (int64_t)m_pc, type);
    }
    // calla directly calls this, everything else goes through relative
    // (see above)
    void InstSrcOpImmLabelAbsolute(
      int srcOpIx,
      const Loc &loc,
      const int64_t absPc, // the actual PC relative to program start
      Type type)
    {
      m_nSrcs = m_nSrcs < srcOpIx + 1 ? srcOpIx + 1 : m_nSrcs;
      OperandInfo &src = m_srcs[srcOpIx];
      src.kind = Operand::Kind::LABEL;
      src.immBlock = lookupBlockNumeric(loc, (int64_t)absPc);
      src.type = type;

      m_blocksRefed[src.immBlock] = loc;
    }

    // a more generic setter
    void InstSrcOp(int srcOpIx, OperandInfo opInfo) {
        m_srcs[srcOpIx] = opInfo;
    }

    // send descriptors
    // E.g. "send ... 0xC  a0.0"
    // (we translate  this to:  a0.0<0;1,0>:ud)
    void InstSendDescs(
        const Loc &locDesc,
        const SendDescArg &exDesc,
        const Loc &locExDesc,
        const SendDescArg &desc)
    {
        m_exDesc = exDesc;
        m_desc = desc;
    }


    /////////////////////////////////////////////
    // instruction option callbacks
    void InstOpts(const InstOptSet &instOpts) {
        m_instOpts = instOpts;
    }


}; // class ParseHandler

} // namespace

#endif //_IR_BUILDER_HANDLER_HPP_
