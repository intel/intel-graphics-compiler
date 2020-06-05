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
#ifndef _IGA_INSTRUCTION_HPP_
#define _IGA_INSTRUCTION_HPP_

// WARNING: the IR is subject to change without any notice.  External tools
// should use the official interfaces in the external API.  Those interfaces
// are tested between releases and maintained even with changes to the IR.

#include "../MemManager/MemManager.hpp"

#include "../Models/Models.hpp"
#include "Operand.hpp"
#include "Loc.hpp"
#include "Types.hpp"
#include "../EnumBitset.hpp"
#include "../asserts.hpp"

#include <string>

namespace iga
{
    enum class SourceIndex {
        SRC0,
        SRC1,
        SRC2,
    };

    // represents a GEN instruction
    class Instruction
    {
    public:
        Instruction(
            int id,
            PC pc,
            Loc loc,
            const OpSpec &os)
            : Instruction(
                os, ExecSize::INVALID, ChannelOffset::M0, MaskCtrl::NORMAL)
        {
            setID(id);
            setLoc(loc);
            setPC(pc);
        }

        // TODO: phase this constructor out
        Instruction(
            const OpSpec &os,
            ExecSize execSize,
            ChannelOffset chOff,
            MaskCtrl mc)
            : m_opSpec(os)
            , m_sf(InvalidFC::INVALID)
            , m_maskCtrl(mc)
            , m_pred(PredCtrl::NONE,false)
            , m_flagReg(REGREF_ZERO_ZERO)
            , m_execSize(execSize)
            , m_chOff(chOff)
            , m_flagModifier(FlagModifier::NONE)
            , m_instLoc(Loc::INVALID)
            , m_instId(0xFFFFFFFF)
            , m_pc(0)
            , m_sendDstLength(-1)
            , m_sendSrc0Len(-1)
            , m_sendSrc1Length(-1)
        {
        }


        // for placement allocation
        void operator delete(void *, MemManager*) { }
        void *operator new(size_t sz, MemManager* m) {return m->alloc(sz);}

        ///////////////////////////////////////////////////////////////////////
        // operations that set instruction state
        // (these are kept roughly in syntax order;
        //  e.g. predication before instruction options)
        //
        void setLoc(const Loc &loc) {m_instLoc = loc;}
        void setPC(int32_t pc) {m_pc = pc;}
        void setID(int id) {m_instId = id;}
        void setMaskCtrl(MaskCtrl mc) {m_maskCtrl = mc;}
        void setExecSize(ExecSize es) {m_execSize = es;}
        void setChannelOffset(ChannelOffset co) {m_chOff = co;}
        void setFlagModifier(FlagModifier flagModifier) {m_flagModifier = flagModifier;}
        void setFlagReg(RegRef reg) {m_flagReg = reg;}
        void setPredication(const Predication &predOpnd) {m_pred = predOpnd;}

        void setSubfunction(Subfunction sf);

        void setDirectDestination(
            DstModifier dstMod,
            RegName r,
            RegRef reg,
            Region::Horz rgn,
            Type type);
        void setMacroDestination(
            DstModifier dstMod,
            RegName r,
            RegRef reg,
            MathMacroExt mme,
            Region::Horz rgn,
            Type type);
        void setInidirectDestination(
            DstModifier dstMod,
            RegRef addrReg,
            int16_t addrImmOff,
            Region::Horz rgnH,
            Type type);
        void setDirectSource(
            SourceIndex srcIx,
            SrcModifier srcMod,
            RegName rType,
            RegRef reg,
            Region rgn,
            Type type);
        void setInidirectSource(
            SourceIndex srcIx,
            SrcModifier srcMod,
            RegRef reg,
            int16_t m_immOffset,
            Region rgn,
            Type type);
        void setMacroSource(
            SourceIndex srcIx,
            SrcModifier srcMod,
            RegName r,
            RegRef reg,
            MathMacroExt acc,
            Region rgn,
            Type type);
        void setImmediateSource(
            SourceIndex srcIx,
            const ImmVal &val,
            Type type);
        void setLabelSource(
            SourceIndex srcIx,
            Block *jip,
            Type type = Type::INVALID);
        void setLabelSource(
            SourceIndex srcIx,
            int32_t pc,
            Type type = Type::INVALID);
        void setSource(
            SourceIndex srcIx,
            const Operand &op); // e.g. pass Operand::NULL_UD_SRC

        void setMsgDesc(const SendDesc &msg) {m_desc = msg;}
        void setExtMsgDesc(const SendDesc &msg) {m_exDesc = msg;}
        void setDstLength(int dstLength) {m_sendDstLength = dstLength;}
        void setSrc0Length(int src0Length) {m_sendSrc0Len = src0Length;}
        void setSrc1Length(int src1Length) {m_sendSrc1Length = src1Length;}

        void setSWSB(SWSB swsb) {m_depInfo = swsb;}

        void addInstOpt(const InstOpt &opt) {m_instOpts.add(opt);}
        void addInstOpts(const InstOptSet &opts) {m_instOpts.add(opts);}

        // associates an optional comment with instruction
        void setComment(std::string comment) {m_comment = comment;}

        ///////////////////////////////////////////////////////////////////////
        // operations that get instruction state
        //
        // the source or binary location
        const Loc        &getLoc() const {return m_instLoc;}
        int               getID() const {return m_instId;}
        PC                getPC() const {return m_pc;}

        // returns the instruction op specification
        const OpSpec      &getOpSpec()         const {return m_opSpec;}
        // opcode, math function control, and branch control
        Op                 getOp()             const {return m_opSpec.op;}
        bool               is(Op op)           const {return m_opSpec.is(op);}

        // WrEn (NoMask)
        MaskCtrl           getMaskCtrl()       const {return m_maskCtrl;}
        // predication
        bool               hasPredication()    const {return m_pred.function != PredCtrl::NONE;}
        const Predication& getPredication()    const {return m_pred;}

        ///////////////////////////////////////////////////////////
        // subfunction accessors
        Subfunction        getSubfunction()    const {return m_sf;}
        // specific subfunction accessors
        // TODO: tantrum (assert) if they call one on the wrong op?
        BranchCntrl        getBranchCtrl()     const {return m_sf.branch;}
        MathFC             getMathFc()         const {return m_sf.math;}
        SFID               getSendFc()         const {return m_sf.send;}
        SyncFC             getSyncFc()         const {return m_sf.sync;}

        // true for madm or math.invm and math.rsqrtm
        bool               isMacro()           const;
        // execution width info
        ExecSize           getExecSize()       const {return m_execSize;}
        ChannelOffset      getChannelOffset()  const {return m_chOff;}

        // returns if the instruction has a flag (condition) modifier
        bool               hasFlagModifier()   const {return m_flagModifier != FlagModifier::NONE;}
        FlagModifier       getFlagModifier()   const {return m_flagModifier;}

        // flag register is shared by predication and flag modfier
        const RegRef&      getFlagReg()        const {return m_flagReg;}

        const Operand&     getDestination()    const {return m_dst;}
              Operand&     getDestination()          {return m_dst;}
        const Operand&     getSource(size_t srcNum) const {return m_srcs[srcNum];}
              Operand&     getSource(size_t srcNum)       {return m_srcs[srcNum];}
        const Operand&     getSource(SourceIndex srcNum) const {return m_srcs[(int)srcNum];}
              Operand&     getSource(SourceIndex srcNum)       {return m_srcs[(int)srcNum];}

        unsigned           getSourceCount() const;

        SendDesc           getExtMsgDescriptor() const {return m_exDesc;}
        SendDesc           getMsgDescriptor()    const {return m_desc;}
        int                getDstLength() const {return m_sendDstLength;}
        int                getSrc0Length() const {return m_sendSrc0Len;}
        int                getSrc1Length() const {return m_sendSrc1Length;}

        const InstOptSet&  getInstOpts() const {return m_instOpts;}
        bool               hasInstOpt(InstOpt opt) const {return m_instOpts.contains(opt);}
        void               removeInstOpt(InstOpt opt) {m_instOpts.remove(opt);}

        const Block       *getJIP() const {return m_srcs[0].getTargetBlock();}
        const Block       *getUIP() const {return m_srcs[1].getTargetBlock();}
        const std::string &getComment() const {return m_comment;}
        SWSB               getSWSB() const {return m_depInfo;}
        bool               isBranching() const {return getOpSpec().isBranching();}

        bool               isMovWithLabel() const;

        void               validate() const; // asserts on malformed IR
        std::string        str(Platform p) const; // returns syntax of this instruction
    private:
        const OpSpec&    m_opSpec; // information about the specific inst op
        Subfunction      m_sf; // e.g. MathFC::INV for math.inv, SFID::DC0 for send.dc0

        MaskCtrl         m_maskCtrl; // (W) WrEn (write enable / NoMask)
        Predication      m_pred; // predication function (and logical sign)
        RegRef           m_flagReg; // shared by m_pred and m_flagModifier

        ExecSize         m_execSize;
        ChannelOffset    m_chOff;
        Operand          m_dst;
        Operand          m_srcs[3];

        FlagModifier     m_flagModifier; // conditional-modifier function

        SendDesc         m_exDesc;
        SendDesc         m_desc;
        int              m_sendDstLength; // -1 if unknown
        int              m_sendSrc0Len; // -1 if unknown
        int              m_sendSrc1Length; // -1 if unknown

        InstOptSet       m_instOpts; // miscellaneous instruction attributes
        SWSB             m_depInfo;

        int              m_instId; // unique id for this instruction (unique in the kernel)

        int32_t          m_pc; // the encode/decode PC for this instruction
        Loc              m_instLoc; // source location info we keep this
                                    // separate from the PC since we
                                    // use both during assembly

        // e.g. for illegal instruction extended info (e.g. decode errors)
        // enables us to emit things like
        //        illegal {Compacted} // failed to uncompact ....
        //                               ^^^^^^^^^^^^^^^^^^^
        std::string      m_comment;
    }; // class Instruction
} // namespace iga
#endif //_IGA_INSTRUCTION_HPP_
