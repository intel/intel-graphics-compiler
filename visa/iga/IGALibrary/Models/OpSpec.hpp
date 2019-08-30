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
#ifndef IGA_OPSPEC_HPP
#define IGA_OPSPEC_HPP

#include "../asserts.hpp"
#include "../Backend/Native/Field.hpp"
#include "../IR/Types.hpp"

#include <cstddef>

namespace iga
{
    // Determines the for a given syntactic construct is explicit, implicit,
    // or default.  T will be something like a Type or Region or otherwise.
    //
    //   EXPLICIT:    means that the type must be given and it's a syntax error
    //                if the construct is absent; the `value` field is
    //                undefined; in formatted output, this construct must be
    //                present
    //
    //   DEFAULT:     means that the syntactic construct is optional; if it's
    //                absent in the syntax, then we silently use `value` in
    //                the IR; in formatted output this construct will be
    //                emitted.
    //
    //   IMPLICIT:    if present we use the given value with a warning,
    //                otherwise we use `value`
    //
    //
    // template <typename T>
    // struct Syntax {
    //     enum{EXPLICIT, DEFAULT IMPLICIT}  type;
    //    T                                 value;
    // };

    // An operation specification
    struct OpSpec {
        // The various syntaxes and formats that each instruction can support
        // The naming convention groups the instructions logically as
        //
        //   NULLARY                   => takes no operands
        //   BASIC_{UNARY,BINARY}_*    => takes one or two operands encodes
        //                                in basic format
        //   MATH_{UNARY,BINARY}       => much like BASIC, but supports MathFC
        //   MATH_MACRO*               => math.invm and math.sqrtm
        //   TERNARY{_MACRO}           => madm
        //   SYNC_{UNARY}              => wait, sync.*
        //
        // The special value of GROUP is used for operations that break into
        // multiple suboperations such as the math function.
        //
        // The suffixes also have important meaning.
        //   REG means it takes only a register as a source operand
        //   IMM means only a source immediate value
        //   REGIMM means either REG or IMM
        //
        // TODO: use a bit feature for REG, REGIMM, IMM etc...
        //        REG - a unary register operand
        //        IMM - a unary immediate operand
        //        REGIMM - a unary register operand or immediate
        //        REG_REG - a binary register, register
        //        REG_IMM - a binary register and immediate
        //        REG_REGIMM - binary reg  and then a reg or immediate
        //        REGIMM_REG_REGIMM - ternary
        //        ... other ideas
        //        IMPL_NULL_DST - implicit null DST
        enum Format {
            // feature bits for constructing ordinals
            // these can be masked out of the ordinals to determine various
            // features of the format
            BASIC   = 0x10000,
            MATH    = BASIC << 1, // MATH_* (includes MATH_MACRO)
            SEND    = MATH << 1, // SEND_*
            JUMP    = SEND << 1, // JUMP_*
            SPECIAL = JUMP << 1, // NULLARY, WAIT, WDEP

            MACRO   = SPECIAL << 1, // MATH_MACRO, TERNARY_MACRO

            HAS_DST = MACRO << 1, // has a destination
            UNARY   = HAS_DST << 1, // takes one operand
            BINARY  = UNARY << 1, // takes two operands
            TERNARY = BINARY << 1, // takes three operands
            // E.g. for math, wdep, etc...
            // indivudual ops should never reference this, but should
            // be retargeted to their specific format
            // both a feature bit and an entry
            GROUP = 0x80000000, // expands into subops

            /////////////////////////////////////////////////
            // THE ACTUAL VALID ENTRIES
            // These are all unique combinations of the above ordinals.
            //
            // for unpopulated entries
            INVALID = 0,

            // op (..) reg   reg
            // e.g. movi (PreCNL)
            BASIC_UNARY_REG = (BASIC|HAS_DST|UNARY) + 1,
            // op (..) reg  (reg|imm32|imm64)
            // e.g.  mov, not, ...
            BASIC_UNARY_REGIMM = BASIC_UNARY_REG + 1,

            // op (..) reg  imm32
            BASIC_BINARY_REG_IMM = (BASIC|HAS_DST|BINARY) + 1,
            // op (..) reg  reg
            BASIC_BINARY_REG_REG = BASIC_BINARY_REG_IMM + 1,
            // op (..) reg  (reg|imm32)
            BASIC_BINARY_REG_REGIMM = BASIC_BINARY_REG_REG + 1,

            // SPECIFCY: do we merge these cases with the math macro?
            // Then add an attribute .hasFunctionControl? to all basic binary
            // and unary?
            //
            // math[.unr_fc] (exec_size) reg  (reg|imm32)
            MATH_UNARY_REGIMM = (MATH|HAS_DST|UNARY) + 1, // inv, cos, ...
            // math[.bin_fc] (exec_size) reg  reg  (reg|imm32)
            MATH_BINARY_REG_REGIMM = (MATH|HAS_DST|BINARY) + 1, // pow, exp, fdiv
            // math[.unrm_fc] (exec_size) reg.acc  reg.acc
            MATH_MACRO_UNARY_REG = (MATH|MACRO|HAS_DST|UNARY) + 1, // rsqrtm
            // math[.binm_fc] (exec_size) reg.acc  reg.acc  reg.acc
            MATH_MACRO_BINARY_REG_REG = (MATH|MACRO|HAS_DST|BINARY) + 1, // invm

            // op (..) reg  reg   reg reg
            // op (..) reg  reg   reg imm16
            // op (..) reg  imm16 reg reg
            TERNARY_REGIMM_REG_REGIMM = (TERNARY|HAS_DST) + 1,
            // op (..) reg.acc   reg.acc  reg.acc  reg.acc
            TERNARY_MACRO_REG_REG_REG = (TERNARY|MACRO|HAS_DST) + 1,

            // BRANCHING OPS
            // brop (..)  label16                          [PreBDW]
            // brop (..)  label32                          [BDW+]
            // e.g. endif, else (PreBDW)
            JUMP_UNARY_IMM = (JUMP|UNARY) + 1,
            // br (..) reg            [implicit null in dst]
            // e.g. ret
            JUMP_UNARY_REG = JUMP_UNARY_IMM + 1,
            // brop (..)  [reg|label32]                    [PreBDW]
            // brop (..)  [reg|label16]                    [BDW+]
            // e.g. jmpi, brd
            // No destination
            JUMP_UNARY_REGIMM = JUMP_UNARY_REG + 1,
            // op (...)  reg   (lbl|reg)
            // e.g. call, calla
            // Has destination
            JUMP_UNARY_CALL_REGIMM = (JUMP|HAS_DST|UNARY) + 1,
            // brc (..) lbl16 lbl16    [PreBDW]
            // brc (..) lbl32 lbl32    [BDW+]
            // brc (..) reg32 null     [PreHSW]
            // brc (..) reg64 null     [HSW]
            // brc (..) reg64 null     [BDW+]
            JUMP_BINARY_BRC = (JUMP|BINARY) + 1,
            // (both sources)
            // brop (..)  label16   label16                [PreBDW]
            // brop (..)  label32   label32                [BDW+]
            // e.g. if, else (BDW+), goto, join
            JUMP_BINARY_IMM_IMM = JUMP_BINARY_BRC + 1,

            // send[c] (..)  reg  reg      ex_desc  desc
            SEND_UNARY = (SEND|HAS_DST|UNARY) + 1,
            // sends[c] (..) reg  reg  reg  ex_desc  desc
            // GEN12: send[c] (..) reg  reg  reg  ex_desc  desc

            SEND_BINARY = (SEND|HAS_DST|BINARY) + 1,

            // illegal or nop
            NULLARY = SPECIAL + 1,

            // SPECIFY: these two could be merged to SPECIAL_REG
            // since they both take a single register
            //
            //   sync.<syctrl> (1)   nreg
            // GEN12:
            //   sync.<syctrl> (..)  reg
            SYNC_UNARY = (SPECIAL|UNARY) + 2,
        };


        // various miscellaneous attributes about the operation
        enum Attr {
            NONE = 0,
            IS_BITWISE = 1, // and, asr, or, ror, rol, not, shl, shr, xor
            IS_SELECT = IS_BITWISE << 1, // sel (not csel)
            // TODO: SUPPORTS_ACCWREN (needs to be placed in BXML)
            SUPPORTS_BRCTL = IS_SELECT << 1,
            SUPPORTS_FLAGMODIFIER = SUPPORTS_BRCTL << 1,
            SUPPORTS_PREDICATION = SUPPORTS_FLAGMODIFIER << 1,
            SUPPORTS_SATURATION = SUPPORTS_PREDICATION << 1,
            SUPPORTS_SRCMODS = SUPPORTS_SATURATION << 1,
            // aliases for combinations of predication, flag modifier, ...
        };

        // for various type mappings
        struct TypeMapping {
            uint32_t dsts; // convert to BitSet<Type,uint32_t>
            uint32_t srcs; // convert to BitSet<Type,uint32_t>
        };

        // virtual opcode / opcode enum value (not the machine-encoded value):
        // will be the same value on all platforms
        Op             op;
        // the operation mnemonic
        const char    *mnemonic; // e.g. "addc"
        // the qualified mnmeonic name
        const char    *fullMnemonic; // e.g. "math.inv" or "addc"
        // the physical opcode value encoded
        int            code; // e.g. 0x1 for mov, 0x7E for nop
        // a high-level name for the operation (Name attr from BXML)
        const char    *name; // e.g. "Add With Carry"
        // a concatenation of the <Description> elements in BXML as
        // well as uncategorized <ProgrammingNote> elements
        const char    *description; // e.g. description from BXML
        // describes both the syntax and the encoding format for this
        // operation
        Format         format;
        // <EUOperandTypeMapping> from BXML.
        // (I think mov has the most ~22)
        TypeMapping    typeMappings[24];
        // Wrap in an EnumBitset<OpAttr,uint32_t> to extract OpAttr's
        int            attrs;

        // The parent op of pseudo op (grouped op); Op::INVALID otherwise
        Op             groupOp;
        // The first subop
        Op             subopStart;
        // The number of pseudoops under this op starting after this op
        // in the Model::ops table.  E.g. we'll have:
        // {Op::MATH, ...}
        // {Op::MATH_COS, ...} // pseudo op
        // {...
        // next op
        int            subopsLength;
        // For pseudo-ops this is the enum ordinal value for the function
        // control.  E.g. static_cast<MathFC>(...)
        // For regular ops it's -1
        int            functionControlValue;
        // For both the grouping op and it's children
        //
        // some ops have fragmented subfunction offset; we store these
        // from low subfunction bits to higher ones; invalid entries
        // (e.g. for ops with no subfunction or with only one, use
        //  {nullptr,0,0}
        // E.g.  {{"SyncFC[3:0]",92,4}} has trailing zero memory to indicate
        // only one is used
        //
        // should some op come along with more fragments change the size
        // to a larger value; everything should just work.
         Field         functionControlFields[2];

        // returns false for reserved opcodes
        bool isValid() const {
            return op != Op::INVALID;
        }
        bool hasAttrs(int a) const {
            return (a & attrs) != 0;
        }
        bool hasImpicitEm() const {
            if (groupOp == Op::SYNC)
                return true;
            switch (op) {
            case Op::NOP:
            case Op::ILLEGAL:
            case Op::JMPI:
            case Op::WAIT:
                return true;
            default:
                return false;
            }
        }

        //////////////////////////////////////////////////////////////////////
        // DESTINATION IMPLICIT VALUES FOR SYNTAX
        //////////////////////////////////////////////////////////////////////
        bool hasDstSubregister(Platform p) const {
            return !isMacro() && !isSendOrSendsFamily();
        }
        bool hasImplicitDstRegion() const {
            Region rgn;
            return implicitDstRegion(rgn);
        }
        Region implicitDstRegion() const {
            Region rgn;
            bool hasRgn = implicitDstRegion(rgn);
            IGA_ASSERT(hasRgn, "dst does not have an implicit region");
            return rgn;
        }
        bool implicitDstRegion(Region &rgn) const {
            // TODO: pull from BXML tables
            if (isSendOrSendsFamily()) {
                rgn = Region::DST1;
                return true;
            } else if (isTypedBranch()) {
                // call and ret have an implicit <1> as well
                rgn = Region::DST1;
                return true;
            } else if (isMacro()) {
                // e.g. madm and math.invm/sqrtm
                rgn = Region::DST1;
                return true;
            } else {
                rgn = Region::INVALID;
                return false;
            }
        }

        //
        Type defaultDstType(Platform p) const {
            Type type = Type::INVALID;
            if (isSendOrSendsFamily()) {
                type = Type::UD;
                if (p >= Platform::GEN12P1)
                    type = Type::UB;
            }
            return type;
        }
        Type defaultSrcType(Platform p) const {
            Type type = Type::INVALID;
            if (isSendOrSendsFamily()) {
                return defaultDstType(p);
            }
            return type;
        }
        bool hasImplicitDstType(Platform p) const {
            Type type;
            return implicitDstTypeVal(p, type);
        }
        Type implicitDstType(Platform p) const {
            Type type;
            (void)implicitDstTypeVal(p, type);
            return type;
        }
        bool implicitDstTypeVal(Platform p, Type &type) const {
            if (isSendFamily() && p >= Platform::GEN8) {
                type = Type::UD;
                if (p >= Platform::GEN12P1) {
                    type = Type::UB;
                }
                return true;
            }
            type = Type::INVALID;
            return false;
        }

        //////////////////////////////////////////////////////////////////////
        // SOURCE IMPLICIT VALUES FOR SYNTAX
        //////////////////////////////////////////////////////////////////////
        bool hasSrcSubregister(int srcOpIx, Platform p) const {
            // send instructions and math macros (including madm) don't emit
            // subregisters
            return !isSendOrSendsFamily() && !isMacro();
        }
        bool hasImplicitSrcRegionEq(
            int srcOpIx,
            Platform plt,
            ExecSize es,
            Region rgn) const
        {
            Region imRegion = implicitSrcRegion(srcOpIx, plt, es);
            if (imRegion == rgn)
                return true;
            return false;
        }
        bool hasImplicitSrcRegion(
            int srcOpIx,
            Platform plt,
            ExecSize es) const
        {
            return (implicitSrcRegion(srcOpIx, plt, es) != Region::INVALID);
        }

        // The source index here corresponds to syntactic position,
        // not encoding position
        Region implicitSrcRegion(
            int srcOpIx,
            Platform pltfm,
            ExecSize execSize) const
        {
            // TODO: fold this into implicitSrcRegionPtr and elide the macro hacking
            //
            // TODO: this needs to work off the table from BXML
            if (isSendFamily() && pltfm < Platform::GEN12P1) {
                return Region::SRC010;
            }
            else if (isMacro()) {
                if (isTernary()) {
                    // ternary macro: e.g. madm
                    if (srcOpIx == 2) {
                        return Region::SRCXX1;
                    }
                    else {
                        if (execSize == ExecSize::SIMD1) {
                            return Region::SRC0X0;
                        }
                        else if (pltfm >= Platform::GEN12P1) {
                            return Region::SRC1X0;
                        }
                        else {
                            return Region::SRC2X1;
                        }
                    }
                }
                else {
                    // basic macro: e.g. math.invm ...
                    if (execSize == ExecSize::SIMD1) {
                        return Region::SRC010;
                    }
                    else if (pltfm >= Platform::GEN12P1) {
                        return Region::SRC110;
                    }
                    else {
                        return Region::SRC221;
                    }
                }
            }
            else if (isSendFamily() || isSendsFamily()) {
                // no regions on send's
                return Region::SRC010;
            }
            else {
                if (pltfm >= Platform::GEN12P1 && isBranching())
                    return Region::SRC110;

                if (srcOpIx == 0) {
                    switch (op) {
                    case Op::JMPI:
                    case Op::CALL:
                    case Op::CALLA:
                    case Op::BRD:
                        return Region::SRC010;
                        // GED won't let us set 221
                    case Op::BRC:
                        return Region::SRC221;
                    case Op::RET: {
                        if (pltfm >= Platform::GEN12P1)
                            return Region::SRC010;
                        else
                            return Region::SRC221;
                    }
                    case Op::SYNC_NOP:
                    case Op::SYNC_ALLRD:
                    case Op::SYNC_ALLWR:
                    case Op::SYNC_BAR:
                    case Op::SYNC_FENCE:
                    case Op::SYNC_HOST:
                    case Op::SYNC:
                        return Region::SRC010;
                    default:
                        ; // fallthrough to return nullptr
                    }
                }
                else if (srcOpIx == 1) {
                    // Encoder encodes Src0 into Src1, we have to lie here;
                    // <2;2,1> gets manually set in Src0 explicitly by
                    // the encoder, this is just for Src1
                    switch (op) {
                    case Op::BRC:
                    case Op::CALL:
                    case Op::CALLA:
                        return Region::SRC010;
                    default:
                        ; // fallthrough to return nullptr
                    }
                }
                return Region::INVALID;
            }
        }

        // a "default source type" is a type that we optionally place on an
        // operand; an example would be on a jmpi or while operand
        // e.g. while (..) -16    means      while (..) -16:d
        // bool hasDefaultSrcType(int srcOpIx) const {
        //    Type type;
        //    return defaultSrcTypeVal(srcOpIx, type);
        // }
        // Type defaultSrcType(int srcOpIx) const {
        //    Type type;
        //    bool hasType = defaultSrcTypeVal(srcOpIx, type);
        //    IGA_ASSERT(hasType, "src doesn't have default type");
        //    return type;
        // }
        // bool defaultSrcTypeVal(int srcOpIx, Type& type) const {
        //    // TODO: pull from BXML tables
        //    if (isTypedBranch()) {
        //        // jmpi or call
        //        type = Type::D;
        //        return true;
        //    } else {
        //        type = Type::INVALID;
        //        return false;
        //    }
        // }

        bool isBranching() const {
            return (format & JUMP) != 0;
        }
        bool isMathSubFunc() const {
            return (format & MATH) != 0;
        }
        // currently branching ops that take an explicit type
        // in some cases we can elide this from syntax, in others we must
        // represent the type explicitly
        bool isTypedBranch() const {
            // TODO: reduce this set as we are able to normalize bits
            return op == Op::BRD ||
                 op == Op::BRC ||
                 op == Op::RET ||
                 op == Op::JMPI ||
                 op == Op::CALL ||
                 op == Op::CALLA;
        }
        bool isBitwise() const {
            return hasAttrs(Attr::IS_BITWISE);
        }
        bool isSendFamily() const {
            return op == Op::SEND || op == Op::SENDC ||
                groupOp == Op::SEND || groupOp == Op::SENDC;
        }
        bool isSendsFamily() const {
            return op == Op::SENDS || op == Op::SENDSC;
        }
        bool isSendOrSendsFamily() const {
            return (format & SEND) != 0;
        }
        bool isTernary() const {
            return (format & TERNARY) != 0;
        }
        bool isMacro() const {
            return (format & MACRO) != 0;
        }

        // indicates the operation corresponds to an OpSpec group with a
        // subfunction. e.g. math, etc...
        bool isGroup() const {
            return (format & GROUP) != 0;
        }
        // Indicates the op is a child op of a parent
        // e.g. math.inv is a child of math
        bool isSubop() const {
            return groupOp != Op::INVALID;
        }

        bool supportsQtrCtrl() const {
            return op != Op::JMPI;
        }

        bool isJipAbsolute() const {
            return op == Op::CALLA;
        }

        bool supportsAccWrEn(Platform pltf) const {
            return
                !supportsBranchCtrl() &&
                !isSendOrSendsFamily() &&
                !isBranching()  &&
                op != Op::NOP &&
                op != Op::ILLEGAL; //jmpi doesn't support branch control
        }
        bool supportsDebugCtrl() const {
            return op != Op::ILLEGAL;
        }

        bool supportsBranchCtrl() const {
            return hasAttrs(Attr::SUPPORTS_BRCTL);
        }
        bool supportsThreadCtrl() const {
            return
                op != Op::NOP &&
                op != Op::ILLEGAL;
        }
        bool supportsPredication() const {
            return hasAttrs(Attr::SUPPORTS_PREDICATION);
        }
        bool supportsFlagModifier() const {
            return hasAttrs(Attr::SUPPORTS_FLAGMODIFIER);
        }
        bool supportsSaturation() const {
            return hasAttrs(Attr::SUPPORTS_SATURATION);
        }
        bool supportsSourceModifiers() const {
            return hasAttrs(Attr::SUPPORTS_SRCMODS);
        }

        // whether there is a destination (in syntax)
        bool supportsDestination() const {
            return (format & HAS_DST) != 0;
        }
        unsigned getSourceCount() const {
            if (format & UNARY)
                return 1;
            else if (format & BINARY)
                return 2;
            else if (format & TERNARY)
                return 3;
            else
                return 0;
        }

        bool isSyncSubFunc() const { return groupOp == Op::SYNC; }

        bool isVariableLatency() const {
            return isSendOrSendsFamily() || isMathSubFunc()
                ;
        }
        bool isFixedLatency() const {
            // TODO: should subtract out special instructions?
            // nop, wait, illegal
            if (groupOp == Op::SYNC) {
                return false; // sync.* gets shot down after DepChk
            }
            switch (op) {
            case Op::ILLEGAL:
            case Op::NOP:
                // these get shot down after DepChk
                return false;
            default:
                return !isVariableLatency();
            }
        }

        bool implicitSrcTypeVal(
            int srcOpIx,
            bool isImmOrLbl,
            Platform pltfm,
            Type& type) const
        {
            // TODO: pull from BXML data (ideally somehow in the syntax)
            if (isTypedBranch()) {
                // branches no longer take types in GEN12
                if (pltfm >= Platform::GEN12P1) {
                    type = Type::INVALID;
                    return true;
                }
                // e.g. jmpi, call, or brc
                //   jmpi  r12.3:d
                //   brd   r12.3:d
                // we make the
                if (op == Op::BRC) {
                    //   brc   r12.3[:d]   null[:ud]
                    //   brc   LABEL[:d]   LABEL[:d]
                    type = srcOpIx == 0 || isImmOrLbl ? Type::D : Type::UD;
                }
                else {
                    type = Type::D;
                }
                return true;
            }
            else if (isBranching()) {
                // if, else, endif, while, break, cont, goto, join, ...
                // let GED pick the defaults
                type = Type::INVALID;
                return true;
            }
            else if (isSendFamily() && pltfm < Platform::GEN12P1) {
                // TRB: we don't print the type on send instructions unless it's
                // not :ud, this allows us to phase out types on send operands
                // while meaningless, apparently there is a requirement on SKL
                // requiring the sampler to read the type from the operand.
                //
                // Types on sends are totally gone in GEN12.
                type = Type::UD;
                return true;
            }
            else if (isSendOrSendsFamily()) {
                // for sends src0 is :ud, src1 has no type bits
                if (pltfm < Platform::GEN12P1) {
                    type = srcOpIx == 0 ? Type::UD : Type::INVALID;
                }
                else {
                    type = Type::UB;
                }
                return true;
            }
            else if (isSyncSubFunc()) {
                // sync imm32          has ud type
                // sync reg32          type is required
                // sync null           type is ommitted
                if (isImmOrLbl) {
                    type = Type::UD;
                    return true;
                }
                type = Type::INVALID;
                return false;
            }
            else {
                type = Type::INVALID;
                return false;
            }
        }

        // An "implicit source type" is a type that should be omitted in syntax
        // if present, we simply warn the user (it can still mismatch the default)
        bool hasImplicitSrcType(
            int srcOpIx, bool immOrLbl, Platform pltfm) const
        {
            Type type;
            return implicitSrcTypeVal(srcOpIx, immOrLbl, pltfm, type);
        }

        Type implicitSrcType(
            int srcOpIx, bool immOrLbl, Platform pltfm) const
        {
            Type type;
            (void)implicitSrcTypeVal(srcOpIx, immOrLbl, pltfm, type);
            return type;
        }

        bool supportsDepCtrl(Platform pltf) const {
            return
                !isSendOrSendsFamily() &&
                pltf < Platform::GEN12P1 &&
                op != Op::NOP &&
                op != Op::ILLEGAL;
        }
    }; // struct OpSpec
} // namespace iga::*
#endif // IGA_OPSPEC_HPP
