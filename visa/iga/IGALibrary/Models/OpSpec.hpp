/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGA_OPSPEC_HPP
#define IGA_OPSPEC_HPP

#include "../IR/Types.hpp"
#include "../strings.hpp"

#include <cstddef>
#include <cstdint>

namespace iga
{
    // An operation specification (OpSpec).
    //
    // Each model contains an array of these built-in
    struct OpSpec {
        // The various syntaxes and formats that each instruction can support
        // The naming convention groups the instructions logically as
        //
        //   NULLARY                   => takes no operands
        //   BASIC_{UNARY,BINARY}_*    => takes one or two operands encodes
        //                                in basic format
        //   MATH_{UNARY,BINARY}       => much like BASIC, but supports MathFC
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

            HAS_DST = SPECIAL << 1, // has a destination
            UNARY   = HAS_DST << 1, // takes one operand
            BINARY  = UNARY << 1, // takes two operands
            TERNARY = BINARY << 1, // takes three operands

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

            // Math operations.
            // We determine the number of sources by subfunction.
            //
            // math[.unr_fc] (exec_size) reg  (reg|imm32)
            // math[.bin_fc] (exec_size) reg  reg  (reg|imm32)
            // math[.unrm_fc] (exec_size) reg.acc  reg.acc
            // math[.binm_fc] (exec_size) reg.acc  reg.acc  reg.acc
            MATH_BINARY_REG_REGIMM = (MATH|HAS_DST|BINARY) + 1, // pow, exp, fdiv

            // op (..) reg  reg   reg reg
            // op (..) reg  reg   reg imm16
            // op (..) reg  imm16 reg reg
            // op (..) reg.acc   reg.acc  reg.acc  reg.acc
            TERNARY_REGIMM_REG_REGIMM = (TERNARY|HAS_DST) + 1,

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
            // XE: send[c] (..) reg  reg  reg  ex_desc  desc

            SEND_BINARY = (SEND|HAS_DST|BINARY) + 1,

            // illegal or nop
            NULLARY = SPECIAL + 1,

            // SPECIFY: these two could be merged to SPECIAL_REG
            // since they both take a single register
            //
            //   sync.<syctrl> (1)   nreg
            // XE:
            //   sync.<syctrl> (..)  reg
            SYNC_UNARY = (SPECIAL|UNARY) + 2,
        };
        //
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
        //
        // for various type mappings
        struct TypeMapping {
            uint32_t dsts; // convert to BitSet<Type,uint32_t>
            uint32_t srcs; // convert to BitSet<Type,uint32_t>
        };
        //
        // virtual opcode / opcode enum value (not the machine-encoded value):
        // will be the same value on all platforms
        Op             op;
        //
        // The platform for this operation; this is mainly for internal
        // methods.
        Platform       platform;
        //
        // the physical opcode value encoded
        uint32_t       opcode; // e.g. 0x1 for mov, 0x7E for nop
        //
        // the operation mnemonic; e.g. "addc"
        ModelString    mnemonic;
        //
        // a high-level name for the operation (Name attr from BXML)
        // e.g. "Add With Carry"
        ModelString    name;
        //
        // describes both the syntax and the encoding format for this
        // operation
        Format         format;
        //
        // <EUOperandTypeMapping> from BXML.
        // (I think mov has the most ~22; expand if needed)
        TypeMapping    typeMappings[24];
        //
        // Wrap in an EnumBitset<OpAttr,uint32_t> to extract OpAttr's
        int            attrs;
        //
#if 0
        constexpr OpSpec(
            Op _op,
            Platform _platform,
            uint32_t _opcode,
            const char *_mnemonic,
            const char *_name,
            Format _format,
            const TypeMapping _typeMappings[24],
            int _attrs)
          : op(_op), platform(_platform), opcode(_opcode),
            , mnemonic(_mnemonic), name(_name),
            format(_format),
            typeMappings(_typeMappings), attrs(_attrs)
        {
        }
        OpSpec(const OpSpec &) = delete;
#endif

        // returns false for reserved opcodes
        bool isValid() const {return !is(Op::INVALID);}
        // generic check of attributes the bits are 'Attr'
        bool hasAttrs(int a) const {return (a & attrs) != 0;}

        // compare the opcode more in a more readable manner than comparison
        bool is(Op _op) const {return op == _op;}
        // also disjunctive comparison
        bool isOneOf(Op op1, Op op2) const {return is(op1) || is(op2);}
        bool isOneOf(Op op1, Op op2, Op op3) const {
            return is(op1) || is(op2) || is(op3);
        }

        //////////////////////////////////////////////////////////////////////
        // INSTRUCTION LEVEL IMPLICIT SYNTAX
        // e.g. 'nop' lacks an execution size and offset
        bool hasImpicitEm() const;

        //////////////////////////////////////////////////////////////////////
        // DESTINATION IMPLICIT VALUES FOR SYNTAX
        //////////////////////////////////////////////////////////////////////
        bool hasDstSubregister(bool isMacro) const;
        bool hasImplicitDstRegion(bool isMacro) const {
            Region rgn; return implicitDstRegion(rgn, isMacro);
        }
        Region implicitDstRegion(bool isMacro) const;
        bool implicitDstRegion(Region &rgn, bool isMacro) const;


        bool hasImplicitDstType() const {
            Type type;
            return implicitDstTypeVal(type);
        }
        Type implicitDstType() const {
            Type type = Type::INVALID;
            (void)implicitDstTypeVal(type);
            return type;
        }
        bool implicitDstTypeVal(Type &type) const;

        //////////////////////////////////////////////////////////////////////
        // SOURCE IMPLICIT VALUES FOR SYNTAX
        //////////////////////////////////////////////////////////////////////
        bool hasSrcSubregister(int srcOpIx, bool isMacro) const;

        // isMacro: pesky Op::MATH can sometimes be a macro (e.g. math.invm),
        // but typically is not (e.g. math.sqt, math.inv)
        bool hasImplicitSrcRegion(
            int srcOpIx, ExecSize es, bool isMacro) const;

        // The source index here corresponds to syntactic position,
        // not encoding position
        Region implicitSrcRegion(
            int srcOpIx, ExecSize es, bool isMacro) const;


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
        bool isBitwise() const {return hasAttrs(Attr::IS_BITWISE);}

        // currently branching ops that take an explicit type
        // in some cases we can elide this from syntax, in others we must
        // represent the type explicitly
        bool isTypedBranch() const;
        bool isSendFamily() const {return isOneOf(Op::SEND, Op::SENDC);}
        bool isSendsFamily() const {return isOneOf(Op::SENDS, Op::SENDSC);}
        bool isSendOrSendsFamily() const {return (format & SEND) != 0;}
        bool isTernary() const {return (format & TERNARY) != 0;}

        // GED doesn't permit us to set execution offset for jmpi
        bool supportsQtrCtrl() const {return !is(Op::JMPI);}

        bool isJipAbsolute() const {return is(Op::CALLA);}

        bool supportsAccWrEn() const;

        // all ops exception illegal accept {Breakpoint}
        bool supportsDebugCtrl() const {return !is(Op::ILLEGAL);}

        bool supportsBranchCtrl() const {
            return hasAttrs(Attr::SUPPORTS_BRCTL);
        }
        bool supportsThreadCtrl() const {
            return op != Op::NOP && op != Op::ILLEGAL;
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

        // returns true if this instruction supports a subfunction of
        // some sorts; e.g. Op:MATH, Op::SEND, ...
        bool supportsSubfunction() const;

        // whether there is a destination (in syntax)
        bool supportsDestination() const {return (format & HAS_DST) != 0;}

        // we need to pass in the math function if the op is math
        // (use MathFC::INVALID if not applicable)
        unsigned getSourceCount(Subfunction sf) const;


        bool isVariableLatency() const;
        bool isFixedLatency() const;

        // this method determines if a source operand supports a default type
        // and returns it if so
        bool implicitSrcTypeVal(
            int srcOpIx, bool isImmOrLbl, Type& type) const;

        // An "implicit source type" is a type that should be omitted in syntax
        // if present, we simply warn the user (it can still mismatch the default)
        bool hasImplicitSrcType(int srcOpIx, bool immOrLbl) const {
            Type type = Type::INVALID;
            return implicitSrcTypeVal(srcOpIx, immOrLbl, type);
        }

        Type implicitSrcType(int srcOpIx, bool immOrLbl) const {
            Type type = Type::INVALID;
            (void)implicitSrcTypeVal(srcOpIx, immOrLbl, type);
            return type;
        }

        // if an instruction supports {NoDDClr,NoDDChk}
        bool supportsDepCtrl() const {
            return
                !isSendOrSendsFamily() &&
                platform < Platform::XE &&
                !is(Op::NOP) &&
                !is(Op::ILLEGAL);
        }


        bool isDpasFamily() const {
            return isOneOf(Op::DPAS, Op::DPASW
            );
        }
    }; // struct OpSpec
} // namespace iga::*
#endif // IGA_OPSPEC_HPP
