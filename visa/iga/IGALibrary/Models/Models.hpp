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
#ifndef IGA_MODELS_HPP
#define IGA_MODELS_HPP

#include "OpSpec.hpp"
#include "../IR/Types.hpp"
#include "../asserts.hpp"

#include <cstddef>
#include <cstdint>

namespace iga
{
    struct RegInfo {
        const char *name; // TODO: rename to "syntax";
        RegName     reg; // should be "name"
        // const char *desc; // e.g. "State Register"
        uint32_t    encoding; // bits [7:4] in register number, reg num is [3:0] except for sp, which uses only bit[3]
        Platform    plat_intrd; // platforms where this is valid
        Platform    plat_last;  // platforms where this is valid
        int         acc_gran; // access granularity (in bytes)
        int         num_regs;   // number of registers (0 means no reg. number)
        int         num_bytes[16]; // number of bytes in each reg


        // TODO: add attributes
        //   (readable/writeable)
        bool isRegNumberValid(int reg) const {
            return num_regs == 0 || reg < num_regs;
        }
        bool isSubRegByteOffsetValid(int regNum, int subregByte) const {
            int regBytes = reg == RegName::GRF_R ? 32 : num_bytes[regNum];
            return subregByte < regBytes;
        }

        // int accessGranularity(int reg) const {
        // }
        bool supportsRegioning() const {
            // needs to be more liberal than before
            // context save and restore seems to region some of the
            // non-regionable registers
            switch (reg) {
            case RegName::ARF_IP:
            case RegName::ARF_CE:
                return false;
            default:
                return true;
            }
        }

        bool supportedOn(Platform p) const {
            return plat_intrd <= p && p <= plat_last;
        }
    };
    // See IRChecker.cpp / checkDst (these ARFs need {Switch})
    static bool arfNeedsSwitch(RegName rn) {

        // ARFs without a scoreboard need {Switch} before the write
        //  Registers with scoreboard and no switch required -
        //    Accumulator/address register/flag register/notify register
        //  Registers without scoreboard and switch required -
        //    Control Register/State Register/Stack Pointer/Timestamp/Pause/IP register
        //  CE is read only.
        //  FC does not require switch. It is read/write only in CSR SIP Routine.
        //  TDR does not require switch. It is read/write only in CSR SIP Routine.

        switch (rn) {
        case RegName::ARF_CR:
        case RegName::ARF_DBG:
        case RegName::ARF_IP:
        case RegName::ARF_SP:
        case RegName::ARF_SR:
        case RegName::ARF_TM:
            return true;
        default:
            return false;
        }
    }


    // TODO: define PlatformSet and move into Models
    static const struct RegInfo registers[] = {
        {"r", RegName::GRF_R, 0xFFFFFFFF, Platform::GEN6, Platform::GENNEXT,
        1, 128, {0}} // handle subreg components specially
      , {"null", RegName::ARF_NULL, 0x0, Platform::GEN6, Platform::GENNEXT,
          0,  0, {32}}

      , {"a",   RegName::ARF_A, 0x1,    Platform::GEN6, Platform::GEN7P5,
          2,  1, {16}}
      , {"a",   RegName::ARF_A, 0x1,    Platform::GEN8, Platform::GENNEXT,
          2,  1, {32}}

      , {"acc",  RegName::ARF_ACC, 0x2,  Platform::GEN6, Platform::GEN7P5,
          4,  2, {32,32}}
      , {"acc",  RegName::ARF_ACC, 0x2,  Platform::GEN8, Platform::GENNEXT,
          2, 10, {32,32,32,32,32,32,32,32,32,32}}

      , {"f",    RegName::ARF_F, 0x3,    Platform::GEN6, Platform::GEN6,
          2, 1, {4}}
      , {"f",    RegName::ARF_F, 0x3,    Platform::GEN7, Platform::GENNEXT,
          2, 2, {4,4}}

      , {"ce",  RegName::ARF_CE, 0x4,   Platform::GEN7P5, Platform::GENNEXT,
          4, 0, {4}}

      , {"msg",  RegName::ARF_MSG, 0x5,  Platform::GEN8, Platform::GENNEXT,
          4, 8, {4,4,4,4,4,4,4,4}}

      , {"sp",   RegName::ARF_SP, 0x6,   Platform::GEN7P5, Platform::GEN7P5,
          4, 0, {2*4}}
      , {"sp",   RegName::ARF_SP, 0x6,   Platform::GEN8, Platform::GENNEXT,
          4, 0, {2*8}}

      , {"sr",   RegName::ARF_SR, 0x7,   Platform::GEN6, Platform::GENNEXT,
          1, 2, {16,16}}
      , {"cr",   RegName::ARF_CR, 0x8,   Platform::GEN6, Platform::GENNEXT,
          4, 1, {4*3}}
      , {"n",   RegName::ARF_N, 0x9,     Platform::GEN6, Platform::GENNEXT,
          4, 1, {4*3}}
      , {"ip",  RegName::ARF_IP, 0xA,    Platform::GEN6, Platform::GENNEXT,
          4, 0, {4}}
      , {"tdr", RegName::ARF_TDR, 0xB,   Platform::GEN6, Platform::GENNEXT,
          2, 1, {16}}

      , {"tm", RegName::ARF_TM, 0xC,    Platform::GEN7, Platform::GEN8LP,
          4, 1, {4*4}}
      , {"tm", RegName::ARF_TM, 0xC,    Platform::GEN9, Platform::GEN9P5,
          4, 1, {4*4}}
      , {"tm", RegName::ARF_TM, 0xC,    Platform::GEN10, Platform::GENNEXT,
          4, 1, {5*4}}
      , {"fc",  RegName::ARF_FC, 0xD,   Platform::GEN7P5, Platform::GENNEXT,
          4, 5, {4*32,4*1,4*1,4*4,4*1}}
      , {"dbg", RegName::ARF_DBG, 0xF,   Platform::GEN7, Platform::GEN7P5,
          4, 1, {4}}
      , {"dbg", RegName::ARF_DBG, 0xF,   Platform::GEN8, Platform::GENNEXT,
          4, 1, {8}}
    };


    static uint8_t IsRegisterScaled(RegName regName)
    {
        switch (regName)
        {
        case RegName::GRF_R:
        case RegName::ARF_NULL:
        case RegName::ARF_A:
        case RegName::ARF_ACC:
        case RegName::ARF_TM:
        case RegName::ARF_CR:
        case RegName::ARF_SP:
        case RegName::ARF_F:
        case RegName::ARF_N:
        case RegName::ARF_DBG:
        case RegName::ARF_SR:
        case RegName::ARF_TDR:
            return true;
        // based on IsaAsm's implemenatation fc isn't scaled
        case RegName::ARF_FC:
        default:
            return false;
        }
    }
    static uint32_t SubRegToBytesOffset(
        int subRegNum, RegName regName, Type type)
    {
        return IsRegisterScaled(regName) ?
            subRegNum*TypeSize(type) : subRegNum;
    }
    static uint8_t BytesOffsetToSubReg(
        uint32_t offset, RegName regName, Type type)
    {
        return IsRegisterScaled(regName) ? offset/TypeSize(type) : offset;
    }

    // TODO: make sublinear (numeric map over RegName)
    static const RegInfo &LookupRegInfo(Platform p, RegName rt)
    {
        for (size_t k = 0; k < sizeof(registers)/sizeof(registers[0]); k++) {
            const RegInfo &ri = registers[k];
            if (p >= ri.plat_intrd && p <= ri.plat_last && ri.reg == rt ) {
                return ri;
            }
        }
        return registers[(int)RegName::ARF_NULL];
    }


    // enables abstract iteration of all OpSpecs in the Model
    // see Model::ops()
    class OpSpecTableIterator {
        Op curr;
        const OpSpec *const opsArray;
        void advanceToNextValid() {
            advance(); // advance at least one
            while (curr <= Op::LAST_OP && !currValid()) {
                advance();
            }
        }
        bool currValid() const { return opsArray[(int)curr].isValid(); }
        void advance() {
            if (curr <= Op::LAST_OP) {
                curr = (Op)((int)curr + 1);
            }
        }
    public:
        OpSpecTableIterator(const OpSpec *const ops, Op from)
            : curr(from), opsArray(ops)
        {
            if (!currValid()) { // if FIRST_OP is bogus, go to next
                advanceToNextValid();
            }
        }
        bool operator==(const OpSpecTableIterator &rhs) const {
            return curr == rhs.curr;
        }
        bool operator!=(const OpSpecTableIterator &rhs) const {
            return !(*this == rhs);
        }
        // OpSpecTableIterator  operator++(int); // post-increment
        OpSpecTableIterator&  operator++() { // pre-increment
            advanceToNextValid();
            return *this;
        }
        const OpSpec *operator *() const {
            return opsArray + (int)curr;
        }
    };
    class OpSpecTableWalker {
    private:
        const OpSpec *const opsArray;
        OpSpecTableIterator end_itr;
    public:
        OpSpecTableWalker(const OpSpec *const ops)
            : opsArray(ops)
            , end_itr(ops, (Op)((int)Op::LAST_OP + 1))
        {
        }
        OpSpecTableIterator begin() const {
            return OpSpecTableIterator(opsArray, Op::FIRST_OP);
        }
        const OpSpecTableIterator &end() const { return end_itr; }
    };


    // error info if we fail to resolve the OpSpec
    // only valid if `decoodeOpSpec` returns nullptr
    struct OpSpecMissInfo
    {
        const OpSpec *parent; // nullptr if at the root  (a top-level op)
                              // otherwise if it's a group op (e.g. math.*,
                              // this'll be that parent
        uint64_t     opcode;  // the opcode bits we tried to lookup
                              // for subfunctions (e.g. math.*), this'll be
                              // the subfunction bits we were looking for
    };


    // Corresponds to a platform model (e.g. GEN9)
    // Has methods to lookup the various operations (OpSpec's) by name
    // opcode value (7 bit encoding), and enumeration value (Op).
    struct Model {
        Platform             platform;
        const char          *ext;  // e.g. "9p5"
        const char          *name; // e.g. "KBL"

        const OpSpec        *const opsArray; // the table of supported ops
        const OpSpec        *opsByCode[128];
        bool                 opsByCodeValid;
        /*
         * Enables iteration of all valid ops in the table in a for all loop
         * E.g. one would write:
         *   iga::Model model = ...
         *   for (const OpSpec *os : model.op()) {
         *      IGA_ASSERT(os->isValid(),"all ops walked will be valid");
         *   }
         */
        OpSpecTableWalker ops() const { return OpSpecTableWalker(opsArray); }
        bool encodeReg(RegName rn, int reg, uint8_t &outputBits) const;

        const OpSpec&        lookupOpSpec(Op op) const;
        const OpSpec&        lookupOpSpecByCode(unsigned opcode) const;
        const OpSpec&        lookupOpSpecFromBits(const void *bits, OpSpecMissInfo &missInfo) const;
        const OpSpec&        lookupGroupSubOp(Op op, unsigned fc_bits) const;
        const OpSpec*        lookupSubOpParent(const OpSpec &os) const;
        const RegInfo*       lookupArfRegInfoByCode(const uint8_t encoding) const; // given the high 4 bits (shift down)
        const RegInfo*       lookupRegInfoByName(RegName name) const;


        static const Model  *LookupModel(Platform platform);


        bool supportsHwDeps() const
        {
            return false;
        }
        // send is unary (sends is binary)
        bool supportsUnarySend() const {
            return supportsHwDeps();
        }
        // sends merged with send (send is binary)
        bool supportsUnifiedSend() const {
            return !supportsHwDeps();
        }
        // registers in control flow is stored in src1 for
        // certain instructions
        bool supportsSrc1CtrlFlow() const {
            return supportsUnarySend();
        }

        // the wait instruction exists
        bool supportsWait() const { return supportsHwDeps(); }

        // ImplAcc must be Align16
        bool supportsAlign16ImplicitAcc() const {
            return platform <= Platform::GEN10;
        }
        // If the GED_ACCESS_MODE is supported
        bool supportsAccessMode() const { return supportsAlign16ImplicitAcc(); }

        // {NoSrcDepSet} allowed
        bool supportNoSrcDepSet() const {
            return platform >= Platform::GEN9 && !supportsUnifiedSend();
        }
        // {NoPreempt} allowed
        bool supportsNoPreempt() const {
            return platform >= Platform::GEN10 && !supportsUnifiedSend();
        }
        // implies that:
        //  - branches don't have types
        //  - the pc is always relative to pre-inc (even jmpi)
        bool supportsSimplifiedBranches() const {
            return false;
        }

        bool supportsAlign16() const { return platform <= Platform::GEN10; }
        bool supportsAlign16MacroOnly() const { return platform == Platform::GEN10; }
        bool supportsNrmAlgn16AccDst()  const { return platform > Platform::GEN7P5 && platform < Platform::GEN10; }
        bool supportsAlign16Ternary() const { return platform < Platform::GEN10; }
        bool supportsAlign16MacroInst() const { return platform <= Platform::GEN10; }
    }; // class model
} // namespace iga::*

#endif // MODELS_HPP
