/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IGC/common/StringMacros.hpp"
#include "visa_igc_common_header.h"
#include "Common_ISA.h"
#include "Common_ISA_util.h"
#include "Common_ISA_framework.h"
#include "JitterDataStruct.h"
#include "VISAKernel.h"
#include "G4_IR.hpp"
#include "BuildIR.h"
#include "BinaryEncodingIGA.h"

#include <iomanip>

using namespace vISA;

static const char* const SrcModifierStr[Mod_src_undef] =
{
    "-",       // Mod_Minus
    "(abs)",   // Mod_Abs
    "-(abs)",  // Mod_Minus_Abs
    "-"        // Mod_Not (print as -)
};

static const G4_InstOptInfo InstOptInfo[] =
{
    {InstOpt_Align16, "Align16"},
    {InstOpt_M0, "M0"},
    {InstOpt_M4, "M4"},
    {InstOpt_M8, "M8"},
    {InstOpt_M12, "M12"},
    {InstOpt_M16, "M16"},
    {InstOpt_M20, "M20"},
    {InstOpt_M24, "M24"},
    {InstOpt_M28, "M28"},
    {InstOpt_Switch, "Switch"},
    {InstOpt_Atomic, "Atomic"},
    {InstOpt_NoDDChk, "NoDDChk"},
    {InstOpt_NoDDClr, "NoDDClr"},
    {InstOpt_WriteEnable, "NoMask"},
    {InstOpt_BreakPoint, "BreakPoint"},
    {InstOpt_EOT, "EOT"},
    {InstOpt_AccWrCtrl, "AccWrEn"},
    {InstOpt_Compacted, "Compacted"},
    {InstOpt_NoCompact, "NoCompact"},
    {InstOpt_NoSrcDepSet, "NoSrcDepSet"},
    {InstOpt_NoPreempt, "NoPreempt"},
    {InstOpt_Serialize, "Serialize"},
    {InstOpt_END, "END"}
};

#define HANDLE_INST(op, nsrc, ndst, type, plat, attr) \
    { G4_##op, #op, nsrc, ndst, type, plat, attr },


#define HANDLE_NAME_INST(op, name, nsrc, ndst, type, plat, attr) \
    { G4_##op, name, nsrc, ndst, type, plat, attr },

const G4_Inst_Info G4_Inst_Table[] = {
#include "G4Instruction.h"
};


static const char* getChannelEnableStr(ChannelEnable channel)
{
    switch (channel)
    {
    case NoChannelEnable:
        return "";
    case ChannelEnable_X:
        return "x";
    case ChannelEnable_Y:
        return "y";
    case ChannelEnable_XY:
        return "xy";
    case ChannelEnable_Z:
        return "z";
    case ChannelEnable_W:
        return "w";
    case ChannelEnable_ZW:
        return "zw";
    case ChannelEnable_XYZW:
        return "xyzw";
    default:
        MUST_BE_TRUE(false, "unsupported channel enable");
        return "";
    }
}

//global functions
uint8_t roundDownPow2(uint8_t n)
{
    uint8_t i = 1;
    while (n >= i) i <<= 1;
    return (i>>1);
}

/* Return the base rank for the input type ignoring the signed/unsigned
 * aspect of types.
 *    - Types of higher precision have higher ranks.
 *    - Floating types have higher precision than all integer types.
 */
static short Operand_Type_Base_Rank(G4_Type type)
{
    short type_size = (short)TypeSize(type);
    short type_rank = type_size * 2;

    if (type == Type_V || type == Type_UV)
    {
        type_rank = (short)TypeSize(Type_W);
    }
    else if (type == Type_VF)
    {
        type_rank = (short)TypeSize(Type_F);
    }
    else if (IS_TYPE_FLOAT_ALL(type))
    {
        type_rank += 2;
    }

    return type_rank;
}

/* Return the rank for the input type.
 *    - Types of higher precision have higher ranks.
 *    - Floating types have higher precision than all integer types.
 *    - Unsigned types have a higher rank than a signed type with the same
 *      precision.
 */
static short Operand_Type_Rank(G4_Type type)
{
    short type_rank = Operand_Type_Base_Rank(type);

    switch (type) {
    case Type_UB:
    case Type_UW:
    case Type_UD: {
        type_rank++;
        break;
    }
    default: {
        // No nothing.
        break;
    }
    }

    return type_rank;
}

// check if type1 can be represented by type2
static bool Is_Type_Included(G4_Type type1, G4_Type type2, const IR_Builder& builder)
{
    if (type1 == type2)
    {
        return true;
    }

    // Float and Int types are never subtype of each other
    if (IS_TYPE_FLOAT_ALL(type1) ^ IS_TYPE_FLOAT_ALL(type2))
    {
        return false;
    }
    if (type1 == Type_F && type2 == builder.getMixModeType() &&
        builder.getPlatform() > GENX_BDW && builder.getOption(vISA_enableUnsafeCP_DF))
    {
        return true;
    }

    if (Operand_Type_Rank(type1) < Operand_Type_Rank(type2))
    {
        if ((IS_UNSIGNED_INT(type1) || type1 == Type_UV) &&
            (IS_UNSIGNED_INT(type2) || type2 == Type_UV))
        {
            return true;
        }
        else if ((IS_SIGNED_INT(type1) || type1 == Type_V) &&
            (IS_SIGNED_INT(type2) || type2 == Type_V))
        {
            return true;
        }
        else if ((type1 == Type_UB || type1 == Type_UW || type1 == Type_UV) && IS_TYPE_INT(type2))
        {
            return true;
        }
        else if (builder.hasMixMode() && type1 == builder.getMixModeType() && type2 == Type_F)
        {
            return true;
        }
    }
    return false;
}

static void resetRightBound(G4_Operand* opnd)
{
    if (opnd) {
        opnd->unsetRightBound();
    }
}

static void associateOpndWithInst(G4_Operand* opnd, G4_INST* inst)
{
    if (opnd) {
        opnd->setInst(inst);
    }
}

G4_INST::G4_INST(
    const IR_Builder& irb,
    G4_Predicate* prd,
    G4_opcode o,
    G4_CondMod* m,
    G4_Sat s,
    G4_ExecSize size,
    G4_DstRegRegion* d,
    G4_Operand* s0,
    G4_Operand* s1,
    G4_Operand* s2,
    G4_Operand* s3,
    G4_InstOpts opt) :
    op(o), dst(d), predicate(prd), mod(m), option(opt),
    useInstList(irb.getAllocator()),
    defInstList(irb.getAllocator()),
    localId(0),
    srcCISAoff(UndefinedCisaOffset),
    sat(s ? 1 : 0),
    evenlySplitInst(false),
    execSize(size),
    bin(nullptr),
    builder(irb)
{
    srcs[0] = s0;
    srcs[1] = s1;
    srcs[2] = s2;
    srcs[3] = s3;

    dead = false;
    skipPostRA = false;
    doPostRA = false;
    implAccSrc = nullptr;
    implAccDst = nullptr;

    resetRightBound(dst);
    resetRightBound(s0);
    resetRightBound(s1);
    resetRightBound(s2);
    resetRightBound(s3);
    computeRightBound(predicate);
    computeRightBound(mod);

    associateOpndWithInst(dst, this);
    associateOpndWithInst(s0, this);
    associateOpndWithInst(s1, this);
    associateOpndWithInst(s2, this);
    associateOpndWithInst(s3, this);
    associateOpndWithInst(predicate, this);
    associateOpndWithInst(mod, this);
}

G4_InstSend::G4_InstSend(
    const IR_Builder& builder,
    G4_Predicate* prd,
    G4_opcode o,
    G4_ExecSize size,
    G4_DstRegRegion* dst,
    G4_SrcRegRegion* payload,
    G4_Operand* desc,
    G4_InstOpts opt,
    G4_SendDesc* md) :
    G4_INST(builder, prd, o, nullptr, g4::NOSAT, size, dst, payload, desc, opt),
    msgDesc(md)
{
    md->setExecSize(size);
}

G4_InstSend::G4_InstSend(
    const IR_Builder& builder,
    G4_Predicate* prd,
    G4_opcode o,
    G4_ExecSize size,
    G4_DstRegRegion* dst,
    G4_SrcRegRegion* payload,
    G4_SrcRegRegion* src1,
    G4_Operand* desc,
    G4_Operand* extDesc,
    G4_InstOpts opt,
    G4_SendDesc* md) :
    G4_INST(builder, prd, o, nullptr, g4::NOSAT, size, dst, payload, src1, desc, opt),
    msgDesc(md)
{
    setSrc(extDesc, 3);
    md->setExecSize(size);
}

void G4_INST::setOpcode(G4_opcode opcd)
{
    MUST_BE_TRUE(opcd < G4_NUM_OPCODE &&
        (G4_Inst_Table[op].instType == G4_Inst_Table[opcd].instType ||
        G4_Inst_Table[opcd].instType == InstTypeMov ||
        (
        (G4_Inst_Table[op].instType == InstTypeMov ||
        G4_Inst_Table[op].instType == InstTypeArith ||
        G4_Inst_Table[op].instType == InstTypeLogic ||
        G4_Inst_Table[op].instType == InstTypePseudoLogic ||
        G4_Inst_Table[op].instType == InstTypeVector) &&

        (G4_Inst_Table[opcd].instType == InstTypeMov ||
        G4_Inst_Table[opcd].instType == InstTypeArith ||
        G4_Inst_Table[opcd].instType == InstTypeLogic ||
        G4_Inst_Table[opcd].instType == InstTypeVector)
       ) ||
        opcd == G4_label),
        "setOpcode would change the intruction class, which is illegal.");

    bool resetBounds = false;

    if (op != opcd)
    {
        resetBounds = true;
    }

    op = opcd;

    if (resetBounds)
    {
        resetRightBound(dst);
        resetRightBound(srcs[0]);
        resetRightBound(srcs[1]);
        resetRightBound(srcs[2]);
        resetRightBound(predicate);
        resetRightBound(mod);
        resetRightBound(implAccDst);
        resetRightBound(implAccSrc);
    }
}

void G4_INST::setExecSize(G4_ExecSize s)
{
    bool resetBounds = false;

    if (execSize != s)
    {
        resetBounds = true;
    }

    execSize = s;

    if (resetBounds)
    {
        resetRightBound(dst);
        resetRightBound(srcs[0]);
        resetRightBound(srcs[1]);
        resetRightBound(srcs[2]);
        resetRightBound(predicate);
        resetRightBound(mod);
        resetRightBound(implAccDst);
        resetRightBound(implAccSrc);
    }
}

//
// We assume no mixed int and float source type, but mixed HF and F is ok
//
G4_Type G4_INST::getExecType() const
{
    G4_Type execType = Type_W;

    // special handling for int divide, as it supports D/UD sources only, while
    // vISA DIV allows B/W types
    // FIXME: if there are more instructions like this, we may need to reorder fixDstAlignment()
    // so that it happens after all sources are fixed and we can get the correct execution type
    if (isMath() && asMathInst()->isMathIntDiv())
    {
        return Type_D;
    }

    if (opcode() == G4_fcvt)
    {
        // fcvt : cvt b/w standard type and other special float type.
        //        execution type is the standard type.
        G4_Type srcTy = srcs[0]->getType();
        if (IS_TYPE_FLOAT_ALL(srcTy))
        {
            return srcTy;
        }
        // If src isn't standard float type, dst must be!
        return dst->getType();
    }
    if (opcode() == G4_srnd)
    {
        // srnd: src0 is either hf or f
        return srcs[0]->getType();
    }
    for (unsigned i = 0; i < G4_MAX_SRCS; i++)
    {
        G4_Operand* src = getSrc(i);
        if (src != NULL)
        {
            G4_Type srcType = src->getType();
            if (TypeSize(srcType) >= TypeSize(execType))
            {
                if (IS_DTYPE(srcType))
                {
                    execType = Type_D;
                }
                else if (IS_QTYPE(srcType))
                {
                    execType = Type_Q;
                }
                else if (IS_TYPE_FLOAT_ALL(srcType))
                {
                    execType = srcType;
                }
            }
        }
    }

    // int <-> HF conversion requires exec type to be dword
    // we don't consider Q<->HF since there are special checks in fixMov() for them
    if (dst)
    {
        G4_Type dstType = dst->getType();
        if (IS_HFTYPE(dstType) && (IS_TYPE_INT(execType) && !IS_QTYPE(execType)))
        {
            execType = Type_D;
        }
        else if (IS_HFTYPE(execType) && (IS_TYPE_INT(dstType) && !IS_QTYPE(dstType)))
        {
            execType = Type_F;
        }
    }

    return execType;
}

// V and VF are treated differently here from the above function
// FIXME: Why do we need two functions???
G4_Type G4_INST::getExecType2() const
{
    G4_Type execType = Type_W;

    // special handling for int divide, as it supports D/UD sources only, while
    // vISA DIV allows B/W types
    if (isMath() && asMathInst()->isMathIntDiv())
    {
        return Type_D;
    }

    for (unsigned i = 0; i < G4_MAX_SRCS; i++)
    {
        G4_Operand* src = getSrc(i);
        if (src == NULL)
        {
            continue;
        }
        G4_Type srcType = srcs[i]->getType();
        if (builder.hasBFMixMode() && srcType == Type_BF)
        {
            execType = Type_F;
        }
        else if (isLowPrecisionFloatTy(srcType) &&
            TypeSize(srcType) >= TypeSize(execType))
        {
            execType = srcType;
            break;
        }
        else if (srcType == Type_V)
        {
            execType = Type_V;
            break;
        }
        else if (srcType == Type_UV)
        {
            execType = Type_UV;
            break;
        }
        else if (IS_DFTYPE(srcType) && !IS_DFTYPE(execType))
        {
            execType = src->getType();
            break;
        }
        else if ((IS_FTYPE(srcType) || srcType == Type_VF) &&
            !IS_DFTYPE(execType) && !IS_FTYPE(execType))
        {
            execType = Type_F;
        }
        else if (IS_DTYPE(srcType) &&
            TypeSize(srcType) >= TypeSize(execType) &&
            !IS_DFTYPE(execType) && !IS_FTYPE(execType))
        {
            execType = Type_D;
        }
        else if (IS_QTYPE(srcType) &&
            TypeSize(srcType) >= TypeSize(execType) &&
            !IS_DFTYPE(execType) && !IS_FTYPE(execType))
        {
            execType = Type_Q;
        }
    }

    // int <-> HF conversion requires exec type to be dword
    // we don't consider Q<->HF since there are special checks in fixMov() for them
    if (dst)
    {
        G4_Type dstType = dst->getType();
        if (IS_HFTYPE(dstType) && (IS_TYPE_INT(execType) && !IS_QTYPE(execType)))
        {
            execType = Type_D;
        }
        else if (IS_HFTYPE(execType) && (IS_TYPE_INT(dstType) && !IS_QTYPE(dstType)))
        {
            execType = Type_F;
        }
    }

    return execType;
}

uint16_t G4_INST::getMaskOffset() const
{
    unsigned maskOption = (getOption() & InstOpt_QuarterMasks);

    if (!builder.hasNibCtrl())
    {
        assert(maskOption != InstOpt_M4 && maskOption != InstOpt_M12 && maskOption != InstOpt_M20 &&
            maskOption != InstOpt_M28 && "nibCtrl is not supported on this platform");
    }

    switch (maskOption)
    {
    case InstOpt_NoOpt:
        return 0;
    case InstOpt_M0:
        return 0;
    case InstOpt_M4:
        return 4;
    case InstOpt_M8:
        return 8;
    case InstOpt_M12:
        return 12;
    case InstOpt_M16:
        return 16;
    case InstOpt_M20:
        return 20;
    case InstOpt_M24:
        return 24;
    case InstOpt_M28:
        return 28;
    default:
        MUST_BE_TRUE(0, "Incorrect instruction execution mask");
        return 0;
    }
}

void G4_INST::setMetadata(const std::string& key, MDNode* value)
{
    if (!MD)
    {
        MD = const_cast<IR_Builder&>(builder).allocateMD();
    }
    MD->setMetadata(key, value);
}

void G4_INST::setComments(const std::string& str)
{
    // we create a new MDNode the assumption is that comment should be unique and there is no opportunity for sharing
    auto node = const_cast<IR_Builder&>(builder).allocateMDString(str);
    setMetadata(Metadata::InstComment, node);
}

void G4_INST::addComment(const std::string& comment) {
    std::string comments = getComments();
    if (!comments.empty()) { // add a separator
        comments += "; ";
    }
    comments += comment;
    setComments(comments);
}

void G4_INST::setTokenLoc(unsigned short token, unsigned globalID)
{
    if (!builder.getOption(vISA_SBIDDepLoc))
    {
        return;
    }
    auto tokenLoc = getMetadata(Metadata::TokenLoc);
    if (!tokenLoc)
    {
        auto node = const_cast<IR_Builder&>(builder).allocateMDTokenLocation(token, globalID);
        setMetadata(Metadata::TokenLoc, node);
    }
    else
    {
        MDTokenLocation* tokenL = tokenLoc->asMDTokenLocation();
        tokenL->addTokenLocation(token, globalID);
    }
}

//
// remove all references to this inst in other inst's use_list
// this is used when we want to delete this instruction
void G4_INST::removeAllDefs()
{
    for (auto&& item : defInstList)
    {
        G4_INST *def = item.first;
        def->useInstList.remove_if(
            [&](USE_DEF_NODE node) { return node.first == this; });
    }
    defInstList.clear();
}

void G4_INST::removeAllUses()
{
    for (auto&& item : useInstList)
    {
        G4_INST *user = item.first;
        user->defInstList.remove_if(
            [&](USE_DEF_NODE node) { return node.first == this; });
    }
    useInstList.clear();
}

//
// remove def/use for opndNum, which must be a source
// (i.e., not Opnd_dst/Opnd_condMod/Opnd_implAccDst)
void G4_INST::removeDefUse(Gen4_Operand_Number opndNum)
{
    DEF_EDGE_LIST_ITER iter = defInstList.begin();
    while (iter != defInstList.end())
    {
        if ((*iter).second == opndNum)
        {
            auto defInst = (*iter).first;
            defInst->useInstList.remove_if(
                [&](USE_DEF_NODE node) { return node.first == this && node.second == opndNum; });
            DEF_EDGE_LIST_ITER curr_iter = iter++;
            defInstList.erase(curr_iter);
        }
        else
        {
            ++iter;
        }
    }
}

const G4_Operand* G4_INST::getOperand(Gen4_Operand_Number opnd_num) const
{
    switch (opnd_num) {
    case Opnd_dst: return (G4_Operand*) dst;
    case Opnd_src0: return srcs[0];
    case Opnd_src1: return srcs[1];
    case Opnd_src2: return srcs[2];
    case Opnd_src3: return srcs[3];
    case Opnd_pred: return (G4_Operand*)predicate;
    case Opnd_condMod: return (G4_Operand*)mod;
    case Opnd_implAccSrc: return implAccSrc;
    case Opnd_implAccDst: return (G4_Operand*) implAccDst;
    default:
        MUST_BE_TRUE(0, "Operand number is out of range.");
        break;
    }
    return NULL;
}

USE_EDGE_LIST_ITER G4_INST::eraseUse(USE_EDGE_LIST_ITER iter)
{
    G4_INST *useInst = iter->first;
    useInst->defInstList.remove_if(
        [&](USE_DEF_NODE node) { return node.first == this && node.second == iter->second; });
    return useInstList.erase(iter);
}

// Transfer definitions used in this[opndNum1] to definitions used in
// inst2[opndNum2] and update definitions's def-use chain accordingly.
void G4_INST::transferDef(G4_INST *inst2, Gen4_Operand_Number opndNum1, Gen4_Operand_Number opndNum2)
{
    DEF_EDGE_LIST_ITER iter = defInstList.begin();
    while (iter != defInstList.end())
    {
        auto defInst = (*iter).first;
        if ((*iter).second == opndNum1)
        {
            // gcc 5.0 doesn't like emplace_back for some reason
            inst2->defInstList.push_back(USE_DEF_NODE(defInst, opndNum2));
            defInst->useInstList.remove_if(
                [&](USE_DEF_NODE node) { return node.second == opndNum1 && node.first == this; });
            defInst->useInstList.push_back(USE_DEF_NODE(inst2, opndNum2));
            DEF_EDGE_LIST_ITER curr_iter = iter++;
            defInstList.erase(curr_iter);

            //Remove the redundant d/u node.
            //Due to the instruction optimization, such as merge scalars, redundant d/u info may be generated.
            //Such as the case:
            //(W) shl (1) V3429(0,0)<1>:d V3380(0,0)<0;1,0>:d 0x17:w
            //(W) shl (1) V3430(0,0)<1>:d V3381(0,0)<0;1,0>:d 0x17:w
            //(W) add (1) V3432(0,0)<1>:d 0x43800000:d -V3429(0,0)<0;1,0>:d
            //(W) add (1) V3433(0,0)<1>:d 0x43800000:d -V3430(0,0)<0;1,0>:d
            //==>
            //(W) shl (2) Merged138(0,0)<1>:d Merged139(0,0)<1;1,0>:d 0x17:w
            //(W) add (2) Merged140(0,0)<1>:d 0x43800000:d -Merged138(0,0)<1;1,0>:d
            inst2->defInstList.sort();
            inst2->defInstList.unique();
            defInst->useInstList.sort();
            defInst->useInstList.unique();
        }
        else
        {
            ++iter;
        }
    }
}

// This copies, from this definition's source opndNum1, all of its defintions to
// inst2's source opndNum2. This is used for example by copy propagation to copy
// the def-use link of the move to the use instruction.
//
// If 'checked' is true, then this only copies those effective defs to inst2.
//
void G4_INST::copyDef(
    G4_INST *inst2,
    Gen4_Operand_Number opndNum1,
    Gen4_Operand_Number opndNum2,
    bool checked)
{
    for (auto I = def_begin(); I != def_end(); ++I)
    {
        if (I->second == opndNum1)
        {
            // If checked is enabled, then compare inst2[opndNum] with this
            // definition. Skip if this is not an effective use.
            if (checked)
            {
                G4_Operand *use = inst2->getOperand(opndNum2);
                ASSERT_USER(use, "null operand unexpected");
                G4_Operand *dst = I->first->getOperand(Opnd_dst);
                G4_Operand *condMod = I->first->getOperand(Opnd_condMod);
                if ((dst && use->compareOperand(dst, getBuilder()) != Rel_disjoint) ||
                    (condMod && use->compareOperand(condMod, getBuilder()) != Rel_disjoint))
                {
                    // OK
                }
                else
                {
                    // Skip to the next def.
                    continue;
                }
            }
            I->first->addDefUse(inst2, opndNum2);
        }
    }
    inst2->defInstList.unique();
}

/// Copy this instruction's defs to inst2.
void G4_INST::copyDefsTo(G4_INST *inst2, bool checked)
{
    if (this == inst2)
        return;

    for (auto I = def_begin(), E = def_end(); I != E; ++I)
    {
        G4_Operand *use = inst2->getOperand(I->second);
        // Copy when the corresponding use operand is not null.
        if (!use)
           continue;

        if (checked)
        {
            G4_Operand *dst = I->first->getOperand(Opnd_dst);
            G4_Operand *condMod = I->first->getOperand(Opnd_condMod);
            G4_Operand* implicitAccDef = I->first->getImplAccDst();
            if ((dst && use->compareOperand(dst, getBuilder()) != Rel_disjoint) ||
                (condMod && use->compareOperand(condMod, getBuilder()) != Rel_disjoint) ||
                (implicitAccDef && use->compareOperand(implicitAccDef, getBuilder()) != Rel_disjoint))
            {
                // OK
            }
            else
            {
                // Skip to the next def.
                continue;
            }
        }

        // inst2[I->second] is defined by I->first.
        I->first->addDefUse(inst2, I->second);
    }
}

/// Copy this instruction's uses to inst2.
void G4_INST::copyUsesTo(G4_INST *inst2, bool checked)
{
    if (this == inst2)
        return;

    for (auto I = use_begin(), E = use_end(); I != E; ++I)
    {
        if (checked)
        {
            G4_Operand *use = I->first->getOperand(I->second);
            ASSERT_USER(use, "null operand unexpected");

            G4_Operand *dst = inst2->getOperand(Opnd_dst);
            G4_Operand *condMod = inst2->getOperand(Opnd_condMod);
            G4_Operand *implicitAccDef = inst2->getImplAccDst();
            if ((dst && use->compareOperand(dst, getBuilder()) != Rel_disjoint) ||
                (condMod && use->compareOperand(condMod, getBuilder()) != Rel_disjoint) ||
                (implicitAccDef && use->compareOperand(implicitAccDef, getBuilder()) != Rel_disjoint))
            {
                // OK
            }
            else
            {
                // Skip to the next use.
                continue;
            }
        }

        // I->first[I->second] is defined by inst2.
        inst2->addDefUse(I->first, I->second);
    }
}

// This transfers this instructions' useInstList to inst2's,
// and update each use's defInstList to point to inst2.
// this instruction's use is destroyed in the process.
// if keepExisting is true, it will preserve inst2's existing uses.
void G4_INST::transferUse(G4_INST *inst2, bool keepExisting)
{
    if (this == inst2)
    {
        return;
    }

    if (!keepExisting)
    {
        inst2->removeAllUses();
    }

    copyUsesTo(inst2, false);
    removeAllUses();
}

//
// remove all references of this inst in other inst's def list
// this is used when we want to delete this instruction
void G4_INST::removeUseOfInst()
{
    for (auto&& node : defInstList)
    {
        auto defInst = node.first;
        defInst->useInstList.remove_if(
            [&](USE_DEF_NODE node) { return node.first == this;});
    }
}

// remove the faked def-instructions in def list, which is resulted from instruction spliting
void G4_INST::trimDefInstList()
{
    // trim def list
    DEF_EDGE_LIST_ITER iter = defInstList.begin();
    // since ACC is only exposed in ARCTAN intrinsic translation, there is no instruction split with ACC
    while (iter != defInstList.end())
    {
        G4_Operand *src = getOperand((*iter).second);

        if (src == nullptr)
        {
            // it's possible the source is entirely gone (e.g., predicate removed)
            iter = defInstList.erase(iter);
            continue;
        }
        G4_CmpRelation rel = Rel_undef;
        if (src->isFlag())
        {
            if ((*iter).first->getCondMod())
            {
                rel = src->compareOperand((*iter).first->getCondMod(), getBuilder());
            }
            else if ((*iter).first->getDst())
            {
                if ((*iter).first->hasNULLDst())
                {
                    rel = Rel_disjoint;
                }
                else
                {
                    rel = src->compareOperand((*iter).first->getDst(), getBuilder());
                }
            }
        }
        else
        {
            rel = src->compareOperand((*iter).first->getDst(), getBuilder());
        }

        if (rel == Rel_disjoint)
        {
            // remove this def-use
            // assumption: no duplicate def-use info
            USE_EDGE_LIST_ITER useIter = (*iter).first->useInstList.begin();
            while (useIter != (*iter).first->useInstList.end())
            {
                if ((*useIter).first == this && (*useIter).second == Opnd_src2)
                {
                    (*iter).first->useInstList.erase(useIter);
                    break;
                }
                useIter++;
            }
            DEF_EDGE_LIST_ITER tmpIter = iter;
            iter++;
            defInstList.erase(tmpIter);
            continue;
        }
        iter++;
    }
}

bool G4_INST::isDFInstruction() const
{
    G4_Operand* dst = getDst();
    if (dst && (dst->getType() == Type_DF))
    {
        return true;
    }
    for (int i = 0; i < getNumSrc(); i++)
    {
        G4_Operand* src = getSrc(i);
        if (src && (src->getType() == Type_DF))
        {
            return true;
        }
    }
    return false;
}

bool G4_INST::isMathPipeInst() const
{
    if (isMath())
    {
        return true;
    }


    return false;
}

bool G4_INST::distanceHonourInstruction() const
{
    if (isSend() || op == G4_nop || isWait() || isDpas())
    {
        return false;
    }
    if (isMathPipeInst())
    {
        if (builder.getPlatform() >= Xe_PVC)
        {
            return true;
        }
        return false;
    }
    return true;
}

bool G4_INST::tokenHonourInstruction() const
{
    if (isSend() || isDpas())
    {
        return true;
    }
    else
    {
        if (isMathPipeInst())
        {
            if (builder.getPlatform() >= Xe_PVC)
            {
                return false;
            }
            return true;
        }
        return false;
    }
}

bool G4_INST::hasNoPipe()
{
    if (op == G4_wait || op == G4_halt || op == G4_nop)
    {
        return true;
    }
    // PVC only
    if (op == G4_sync_fence)
    {
        return true;
    }
    return false;
}


bool G4_INST::isLongPipeType(G4_Type type) const
{
    if (builder.hasPartialInt64Support())
    {
        return type == Type_DF;
    }
    return IS_TYPE_LONG(type);
}

bool G4_INST::isIntegerPipeType(G4_Type type) const
{
    if (IS_TYPE_INTEGER(type))
    {
        return true;
    }

    if (builder.hasPartialInt64Support())
    {
        return type == Type_UQ || type == Type_Q;
    }

    return false;
}

bool G4_INST::isJEUPipeInstructionXe() const
{
    if (op == G4_jmpi ||
        op == G4_if ||
        op == G4_else ||
        op == G4_endif ||
        op == G4_break ||
        op == G4_join ||
        op == G4_cont ||
        op == G4_while ||
        op == G4_brc ||
        op == G4_brd ||
        op == G4_goto ||
        op == G4_call ||
        op == G4_return)
    {
        return true;
    }
    return false;
}


bool G4_INST::isLongPipeInstructionXe() const
{
    if (isJEUPipeInstructionXe())
    {
        return false;
    }

    if (!distanceHonourInstruction())
    {
        return false;
    }

    if (builder.hasFixedCycleMathPipeline() &&
        isMath())
    {
        return false;
    }


    const G4_Operand* dst = getDst();
    if (dst && isLongPipeType(dst->getType()))
    {
        return true;
    }

    if (!builder.hasPartialInt64Support())
    {
        for (int i = 0; i < G4_MAX_SRCS; i++)
        {
            const G4_Operand* src = getSrc(i);
            if (src && isLongPipeType(src->getType()))
            {
                return true;
            }
        }
    }

    return false;
}

bool G4_INST::isIntegerPipeInstructionXe() const
{
    if (isJEUPipeInstructionXe())
    {
        return true;
    }

    if (!distanceHonourInstruction())
    {
        return false;
    }

    if (isLongPipeInstructionXe())
    {
        return false;
    }


    if (builder.hasFixedCycleMathPipeline() &&
        isMath())
    {
        return false;
    }
    if (op == G4_fcvt)
    {
        return false;
    }
    if (op == G4_srnd)
    {
        return false;
    }

    G4_Operand* dst = getDst();
    if (dst && isIntegerPipeType(dst->getType()))
    {
        return true;
    }

    if (builder.hasQ2FInIntegerPipe() && dst->getType() == Type_F)
    {
        const G4_Operand* src = getSrc(0);
        if (src && (src->getType() == Type_Q || src->getType() == Type_UQ))
        {
            return true;
        }
    }

    if (!dst)
    {
        const G4_Operand* src = getSrc(0);
        if (src && isIntegerPipeType(src->getType()))
        {
            return true;
        }
    }

    return false;
}

bool G4_INST::isFloatPipeInstructionXe() const
{
    if (isJEUPipeInstructionXe())
    {
        return false;
    }

    if (!distanceHonourInstruction())
    {
        return false;
    }


    if (isLongPipeInstructionXe())
    {
        return false;
    }

    if (builder.hasFixedCycleMathPipeline() &&
        isMath())
    {
        return false;
    }
    if (opcode() == G4_fcvt)
    {
        return true;
    }
    if (opcode() == G4_srnd)
    {
        return true;
    }

    const G4_Operand* dst = getDst();
    if (dst &&
        (dst->getType() == Type_F ||
            dst->getType() == Type_HF ||
            dst->getType() == Type_BF))
    {
        if (builder.hasQ2FInIntegerPipe() && dst->getType() == Type_F)
        {
            const G4_Operand* src = getSrc(0);
            if (src && (src->getType() == Type_Q || src->getType() == Type_UQ))
            {
                return false;
            }
        }
        return true;
    }

    if (!dst)
    {
        const G4_Operand* src = getSrc(0);
        if (src &&
            (src->getType() == Type_F ||
                src->getType() == Type_HF ||
                src->getType() == Type_BF))
        {
            return true;
        }
    }

    return false;
}

SB_INST_PIPE G4_INST::getDataTypePipeXe(G4_Type type)
{
    switch (type)
    {
    case Type_UB:
    case Type_B:
    case Type_UW:
    case Type_W:
    case Type_UD:
    case Type_D:
    case Type_UV:
    case Type_V:
        return PIPE_INT;

    case Type_Q:
    case Type_UQ:
        if (builder.hasPartialInt64Support())
        {
            return PIPE_INT;
        }
        return PIPE_LONG;

    case Type_DF:
        return PIPE_LONG;

    case Type_HF:
    case Type_F:
    case Type_VF:
    case Type_NF:
    case Type_BF:
        return PIPE_FLOAT;

    default:
        return PIPE_NONE;
    }

    return PIPE_NONE;
}

SB_INST_PIPE G4_INST::getInstructionPipeXe()
{

    if (isLongPipeInstructionXe())
    {
        return PIPE_LONG;
    }

    if (isIntegerPipeInstructionXe())
    {
        return PIPE_INT;
    }

    if (isFloatPipeInstructionXe())
    {
        return PIPE_FLOAT;
    }

    if (builder.hasFixedCycleMathPipeline() &&
        isMath())
    {
        return PIPE_MATH;
    }

    if (tokenHonourInstruction())
    {
        if (isDpas())
        {
            return PIPE_DPAS;
        }
        if (isMathPipeInst())
        {
            return PIPE_MATH;
        }
        if (isSend())
        {
            return PIPE_SEND;
        }

        ASSERT_USER(0, "Wrong token pipe instruction!");
    }

    ASSERT_USER(hasNoPipe(), "No pipe instruction");
    return PIPE_NONE;
}

template <typename T>
static std::string fmtHexBody(T t, int cols = 0)
{
    std::stringstream ss;
    if (sizeof(t) == 1) // char/unsigned char to int
        ss << std::hex << std::setw(cols) << std::uppercase <<
            std::setfill('0') << (int)t;
    else
        ss << std::hex << std::setw(cols) << std::uppercase <<
            std::setfill('0') << t;
    return ss.str();
}

template <typename T>
static std::string fmtHex(T t, int cols = 0)
{
    std::stringstream ss;
    ss << "0x" << fmtHexBody(t, cols);
    return ss.str();
}


#ifdef _DEBUG
static void printDefUseImpl(
    std::ostream &os, G4_INST *def, G4_INST *use, Gen4_Operand_Number pos)
{
    os << "\n  def: ";
    def->emit(os);
    os << "\n user: ";
    use->emit(os);
    os << "\n opnd: ";
    use->getOperand(pos)->emit(os);
    os << "\n";
}
#endif

void G4_INST::dumpDefUse(std::ostream &os)
{
#if _DEBUG
    std::cerr << "\n------------ defs ------------\n";
    for (auto&& UD : defInstList)
    {
        printDefUseImpl(std::cerr, UD.first, this, UD.second);
    }
    std::cerr << "\n------------ uses ------------\n";
    for (auto&& DU : useInstList)
    {
        printDefUseImpl(std::cerr, this, DU.first, DU.second);
    }
#endif
}

namespace {
    // Customized def-use iterator comparison. Do not compare itself
    // but the content it is pointing to.
    struct def_less
    {
        bool operator()(DEF_EDGE_LIST_ITER a, DEF_EDGE_LIST_ITER b) const
        {
            if (a->first < b->first)
            {
                return true;
            }
            else if ((a->first == b->first) && (a->second < b->second))
            {
                return true;
            }
            return false;
        }
    };
}

G4_INST *G4_INST::getSingleDef(Gen4_Operand_Number opndNum, bool MakeUnique)
{
    if (MakeUnique)
    {
        std::set<DEF_EDGE_LIST_ITER, def_less> found;
        for (auto I = def_begin(); I != def_end(); /* empty */)
        {
            if (!found.insert(I).second)
            {
                I = defInstList.erase(I);
            }
            else
            {
                ++I;
            }
        }
    }

    G4_INST *def = 0;
    unsigned def_count = 0;
    for (auto I = def_begin(), E = def_end(); I != E; ++I)
    {
        if (I->second == opndNum)
        {
            if (++def_count > 1) return 0;
            def = I->first;
        }
    }

    return def;
}

// add def-use between this instruction <--> inst[srcPos]
// Note that this function does not check for duplicates
void G4_INST::addDefUse(G4_INST* inst, Gen4_Operand_Number srcPos)
{
    MUST_BE_TRUE(srcPos == Opnd_dst ||
        srcPos == Opnd_src0 || srcPos == Opnd_src1 ||
        srcPos == Opnd_src2 || srcPos == Opnd_src3 ||
        srcPos == Opnd_src4 || srcPos == Opnd_src5 ||
        srcPos == Opnd_src6 || srcPos == Opnd_src7 ||
        srcPos == Opnd_pred ||
        srcPos == Opnd_implAccSrc, "unexpected operand number");
    useInstList.emplace_back(inst, srcPos);
    inst->defInstList.emplace_back(this, srcPos);
}

// exchange def/use info of src0 and src1 after they are swapped.
void G4_INST::swapDefUse(Gen4_Operand_Number srcIxA, Gen4_Operand_Number srcIxB)
{
    DEF_EDGE_LIST_ITER iter = defInstList.begin();
    //To avoid redundant define and use items
    INST_LIST handledDefInst;

    // since ACC is only exposed in ARCTAN intrinsic translation, there is no instruction split with ACC
    while (iter != defInstList.end())
    {
        if ((*iter).second == srcIxB)
        {
            (*iter).second = srcIxA;
        }
        else if ((*iter).second == srcIxA)
        {
            (*iter).second = srcIxB;
        }
        else
        {
            iter++;
            continue;
        }
        if (std::find(handledDefInst.begin(), handledDefInst.end(), (*iter).first) != handledDefInst.end())
        {
            iter++;
            continue;
        }
        handledDefInst.push_back((*iter).first);
        // change uselist of def inst
        USE_EDGE_LIST_ITER useIter = (*iter).first->useInstList.begin();
        for (; useIter != (*iter).first->useInstList.end(); useIter++)
        {
            if ((*useIter).first == this)
            {
                if ((*useIter).second == srcIxB)
                {
                    (*useIter).second = srcIxA;
                }
                else if ((*useIter).second == srcIxA)
                {
                    (*useIter).second = srcIxB;
                }
            }
        }
        iter++;
    }
}

// returns true if inst is a commutable binary instruction and its two sources can be swapped
bool G4_INST::canSwapSource() const
{
    if (getNumSrc() != 2)
    {
        return false;
    }

    if (!INST_COMMUTATIVE(opcode()))
    {
        return false;
    }

    G4_Operand* src0 = getSrc(0);
    G4_Operand* src1 = getSrc(1);
    // src1 restrictions: no ARF, no VXH
    if (src0->isSrcRegRegion())
    {
        G4_SrcRegRegion* src0Region = src0->asSrcRegRegion();
        if (src0Region->isAreg() || src0Region->getRegion()->isRegionWH())
        {
            return false;
        }
    }

    // src0 restrictions: no Imm
    if (src1->isImm() || src1->isAddrExp())
    {
        return false;
    }

    // special check for mul: don't put DW on src1
    if (opcode() == G4_mul)
    {
        if (IS_DTYPE(src0->getType()) && !IS_DTYPE(src1->getType()))
        {
            return false;
        }
    }

    return true;
}
// fix src2 def/use to implicitSrc def/use
void G4_INST::fixMACSrc2DefUse()
{
    if (op != G4_mac)
    {
        return;
    }
    for (DEF_EDGE_LIST_ITER iter = defInstList.begin();
        iter != defInstList.end();
        iter++)
    {
        if ((*iter).second == Opnd_src2)
        {
            (*iter).second = Opnd_implAccSrc;
            G4_INST* defInst = (*iter).first;
            for (USE_EDGE_LIST_ITER useIter = defInst->useInstList.begin();
                useIter != defInst->useInstList.end();
                ++useIter)
            {
                if (((*useIter).first == this) &&
                    ((*useIter).second == Opnd_src2))
                {
                    (*useIter).second = Opnd_implAccSrc;
                    break;
                }
            }
            break;
        }
    }
}

// a raw move is a move with
// -- no saturation or src modifiers
// -- same dst and src type
// -- no conditional modifier (predicate is ok)
bool G4_INST::isRawMov() const
{
    return op == G4_mov && !sat && dst->getType() == srcs[0]->getType() &&
        getCondMod() == NULL &&
        (srcs[0]->isImm() ||
        (srcs[0]->isSrcRegRegion() && srcs[0]->asSrcRegRegion()->getModifier() == Mod_src_undef));
}

bool G4_INST::hasACCSrc() const
{
    if (implAccSrc ||
        (srcs[0] && srcs[0]->isSrcRegRegion() && srcs[0]->asSrcRegRegion()->isAccReg()))
    {
        return true;
    }
    return false;
}

// check if acc is possibly used by this instruction
bool G4_INST::hasACCOpnd() const
{
    return (isAccWrCtrlInst() ||
        implAccSrc ||
        implAccDst ||
        (op == G4_mulh &&
        IS_DTYPE(srcs[0]->getType()) && IS_DTYPE(srcs[1]->getType())) ||
        (dst && dst->isAccReg()) ||
        (srcs[0] && srcs[0]->isAccReg()) ||
        (srcs[1] && srcs[1]->isAccReg()) ||
        (srcs[2] && srcs[2]->isAccReg()) ||
        op == G4_madw);
}

G4_Type G4_INST::getOpExecType(int& extypesize)
{
    G4_Type extype;
    if (isRawMov())
    {
        extype = srcs[0]->getType();
    }
    else
    {
        extype = getExecType2();
    }
    if (IS_VINTTYPE(extype))
    {
        extypesize = getBuilder().numEltPerGRF<Type_UB>()/2;
    }
    else if (IS_VFTYPE(extype))
    {
        extypesize = getBuilder().numEltPerGRF<Type_UB>();
    }
    else
    {
        extypesize = TypeSize(extype);
    }

    return extype;
}

static G4_INST::MovType getMovType(
    const G4_INST* Inst, G4_Type dstTy, G4_Type srcTy, G4_SrcModifier srcMod)
{
    // COPY when dst & src types are the same.
    if (dstTy == srcTy)
        return G4_INST::Copy;

    bool dstIsFP = IS_TYPE_FLOAT_ALL(dstTy);
    bool srcIsFP = IS_TYPE_FLOAT_ALL(srcTy);

    // If dst & src are not both FPs or both Integers, that MOV must be
    // conversions from Integer to FP or vice versa.
    if (dstIsFP != srcIsFP) {
        if (dstIsFP)
            return G4_INST::IntToFP;

        ASSERT_USER(srcIsFP, "Unexpected source type!");
        return G4_INST::FPToInt;
    }

    // If they are both FPs, that MOV must be either up or down conversion.
    // Note it could not be a COPY as dst & src are different here.
    if (dstIsFP) {
        ASSERT_USER(srcIsFP, "Unexpected source type!");

        // TODO: Do we need to treat 'vf' differently?

        if (TypeSize(srcTy) < TypeSize(dstTy))
            return G4_INST::FPUpConv;

        ASSERT_USER(TypeSize(srcTy) > TypeSize(dstTy),
            "Unexpected FP source and destination type sizes!");
        return G4_INST::FPDownConv;
    }

    // They are both Integers. The destination signedness is ignored here to
    // detect the mov type as it really does not matter without saturation nor
    // condition modifier.

    ASSERT_USER(!IS_VINTTYPE(dstTy),
                "Unexpected immediate types are used as dst type!");

    // Always treat 'v' as SExt as they will always be extended even for
    // BYTE-sized types.
    if (srcTy == Type_V) {
        // If the sign bit is 0, then zext is the same as sext.
        // prefer zext as it allows more propagation.
        G4_Operand *Op0 = Inst->getSrc(0);
        if (Op0->isImm() && Op0->asImm()->isSignBitZero())
            return G4_INST::ZExt;
        return G4_INST::SExt;
    }

    // Always treat 'uv' as ZExt as they will always be extended even for
    // BYTE-sized types.
    if (srcTy == Type_UV)
        return G4_INST::ZExt;

    // Treat that mov as truncation.
    if (TypeSize(srcTy) > TypeSize(dstTy))
    {
        if (IS_SIGNED_INT(srcTy) &&
           srcMod != Mod_src_undef &&
           srcMod != Mod_Not)
        {
            return G4_INST::SuperMov;
        }
        else
        {
            return G4_INST::Trunc;
        }
    }

    // Treat that mov as sign extend or zero extend based on the signedness of
    // the source type only.
    if (TypeSize(srcTy) < TypeSize(dstTy)) {
        if (IS_SIGNED_INT(srcTy)) {
            // Treat ABS as zero-extenstion.
            if (srcMod == Mod_Abs)
                return G4_INST::ZExt;
            // If the sign bit is 0, then zext is the same as sext.
            // prefer zext as it allows more propagation.
            G4_Operand *Op0 = Inst->getSrc(0);
            if (Op0->isImm() && Op0->asImm()->isSignBitZero())
                return G4_INST::ZExt;

            return G4_INST::SExt;
        }
        else if (srcMod == Mod_Minus || srcMod == Mod_Minus_Abs)
        {   // SrcMod=negate means that number is signed
            return G4_INST::SExt;
        }
        return G4_INST::ZExt;
    }

    // Otherwise, treat it as COPY they are the same in bit size.
    // Treat ABS as zero-extenstion.
    if (IS_SIGNED_INT(srcTy) && srcMod == Mod_Abs)
        return G4_INST::ZExt;
    return G4_INST::Copy;
}

// check if this instruction can be propagated
G4_INST::MovType G4_INST::canPropagate() const
{
    G4_Declare* topDcl = NULL;

    if (dst == NULL)
    {
        return SuperMov;
    }

    topDcl = dst->getTopDcl();

    if (op != G4_mov
        // Do not eliminate if either sat or condMod is present.
        || getSaturate() || getCondMod()
        // Do not eliminate if there's no use (dead or side-effect code?)
        || useInstList.size() == 0
        // Do not eliminate stack call return value passing instructions.
        // Do not eliminate vars marked with Output attribute
        || (topDcl && topDcl->isOutput()))
    {
        return SuperMov;
    }

    // can't propagate stack call related variables (Arg, Retval, SP, FP)
    if (topDcl)
    {
        G4_Declare* rootDcl = topDcl->getRootDeclare();
        if (builder.isPreDefFEStackVar(rootDcl) || builder.isPreDefArg(rootDcl) ||
            builder.isPreDefRet(rootDcl))
        {
            return SuperMov;
        }
    }


    // Do not eliminate MOV/COPY to Acc/flag registers.
    if (dst->isAccReg() || dst->isFlag())
    {
        return SuperMov;
    }

    // Retain side effect of writing to debug register.
    if (dst->isDbgReg())
    {
        return SuperMov;
    }

    G4_Operand *src = srcs[0];

    if (src->isRelocImm())
    {
        return SuperMov;
    }

    // only support flag propagation for simd1 copy moves
    if (src->isFlag())
    {
        if (getExecSize() != g4::SIMD1 || src->getType() != dst->getType())
        {
            return SuperMov;
        }
    }

    // Do not propagate through copy of `acc0` if its execution size does not match the native size,
    // as some latest passes (e.g., fixAddCSubb) rely on the acc0 copy move for correctness
    if (src->isAccReg() && getExecSize() != builder.getNativeExecSize())
    {
        return SuperMov;
    }

    if (builder.kernel.fg.globalOpndHT.isOpndGlobal(dst))
    {
        return SuperMov;
    }

    G4_Type dstType = dst->getType();
    G4_Type srcType = src->getType();

    if (!builder.hasByteALU()
        && (TypeSize(dstType) == 1 || TypeSize(srcType) == 1))
    {
        return SuperMov;
    }

    G4_SrcModifier srcMod = Mod_src_undef;
    if (src->isSrcRegRegion()) {
        srcMod = src->asSrcRegRegion()->getModifier();
    }

    MovType MT = getMovType(this, dstType, srcType, srcMod);

    //Disabling mix mode copy propogation
    if (!builder.hasMixMode() &&
        ((IS_TYPE_F32_F64(srcType) && isLowPrecisionFloatTy(dstType)) ||
        (isLowPrecisionFloatTy(srcType) && IS_TYPE_F32_F64(dstType))))
    {
        return SuperMov;
    }

    // Selectively enable copy propagation on the detected mov type.
    switch (MT) {
    default:
        return SuperMov;
    case Copy:
    case ZExt:
    case SExt:
        // COPY and integer extending are allowed.
        break;
    case Trunc: {
        if (!src->isSrcRegRegion())
            return SuperMov;
        G4_SrcRegRegion *src0 = src->asSrcRegRegion();
        if (src0->getRegion()->isContiguous(getExecSize())) {
            unsigned newHS = TypeSize(srcType) / TypeSize(dstType);
            if (newHS > 4) {
                // Rule out Q -> B. WHY?
                return SuperMov;
            }
        } else if (!src0->isScalar()) {
            return SuperMov;
        }
        break;
    }
    case FPUpConv:
        // For FPUpConv, only HF -> F is allowed.
        if (!(srcType == builder.getMixModeType() && dstType == Type_F))
            return SuperMov;
        break;
    case FPDownConv:
    {
        if (IS_TYPE_F32_F64(srcType) &&
            builder.getMixModeType() == dstType &&
            builder.getOption(vISA_enableUnsafeCP_DF) &&
            useInstList.size() == 1)
            return FPDownConvSafe;
        break;
    }
    // TODO: Enable IntToFP or vice versa on constant.
    }

    return MT;
}

bool G4_INST::canPropagateBinaryToTernary() const
{
    if (opcode() != G4_add && opcode() != G4_mul)
        return false; // constrain just to a few ops for the moment
    else if (dst == nullptr)
        return false;
    else if (!dst->getBase()->isRegVar() && !dst->getBase()->isPhyGreg())
        return false; // must be GRF dst
    else if (dst->isIndirect())
        return false; // must not be indirect
    else if (dst->getHorzStride() != 1)
        return false; // must be <1>
    else if (
        dst->getType() != Type_D && dst->getType() != Type_UD &&
        dst->getType() != Type_Q && dst->getType() != Type_UQ)
        return false; // dst has to be :d or :ud (for now)
    else if (builder.kernel.fg.globalOpndHT.isOpndGlobal(dst))
        return false; // writes to globals must be visible
    else if (getNumSrc() != 2)
        return false; // must be binary
    else if (getPredicate())
        return false; // no predicates
    else if (getExecSize() != 1 && dst->getSubRegOff() != 0)
        return false; // must be dst.0 or SIMD1 to any subreg
    else if (getImplAccDst() || getImplAccSrc())
        return false; // no {AccWrEn}
    else if (getSaturate() || getCondMod())
        return false; // do not eliminate if either sat or condMod is present.
    else if (useInstList.size() == 0)
        return false; // do not eliminate if there's no use (dead or side-effect code?)

    G4_Declare* topDcl = dst->getTopDcl();
    if (topDcl) {
        // Do not eliminate stack call return value passing instructions.
        // Do not eliminate vars marked with Output attribute.
        if (topDcl->isOutput())
            return false;
        G4_Declare* rootDcl = topDcl->getRootDeclare();
        if (builder.isPreDefFEStackVar(rootDcl) || builder.isPreDefArg(rootDcl) ||
            builder.isPreDefRet(rootDcl))
        {
            // can't propagate stack call related variables (Arg, Retval, SP, FP)
            return false;
        }
    }

    for (int srcIx = 0; srcIx < getNumSrc(); srcIx++) {
        G4_Operand *src = srcs[srcIx];

        if (!src->isSrcRegRegion() && !src->isImm()) {
            return false; // only GRF
        } else if (src->isRelocImm()) {
            return false;
        }
        if (src->isSrcRegRegion()) {
            const G4_SrcRegRegion *srr = src->asSrcRegRegion();
            if (!srr->getBase()->isRegVar() && !srr->getBase()->isPhyGreg()) {
                return false; // has to be GRF
            } else if (srr->isIndirect()) {
                return false; // has to be direct
            }
        }
    }

    return true;
}

// Check to see whether the given type is supported by this opcode + operand. Mainly focus on integer ops
// This is used by copy propagation and def-hoisting to determine if the resulting instruction is legal
bool G4_INST::isLegalType(G4_Type type, Gen4_Operand_Number opndNum) const
{
    bool isSrc = (opndNum == Opnd_src0 || opndNum == Opnd_src1 || opndNum == Opnd_src2);
    switch (op)
    {
    default:
        // ToDo: Make this function more complete by adding more opcodes
        // keep alphabetical order when adding to make it easier to maintain
        return true;
    case G4_addc:
        return type == Type_UD;
    case G4_bfe:
    case G4_bfi1:
    case G4_bfi2:
        // additionally check src and dst have same type
        return (type == Type_D || type == Type_UD) &&
         (isSrc ? type == dst->getType() : type == getSrc(0)->getType());
    case G4_bfrev:
        return type == Type_UD;
    case G4_cbit:
        return type == Type_UB || type == Type_UW || type == Type_UD;
    case G4_fbh:
        return type == Type_D || type == Type_UD;
    case G4_fbl:
        return type == Type_UD;
    case G4_lzd:
        return type == Type_D || type == Type_UD;
    case G4_sad2:
    case G4_sada2:
        return type == Type_B || type == Type_UB;
    case G4_subb:
        return type == Type_UD;
    case G4_mov:
        // Avoid mov r7.0<1>:hf  0x76543210:v
        if (IS_VINTTYPE(type) &&
            (IS_FTYPE(dst->getType()) || IS_HFTYPE(dst->getType())))
        {
            return false;
        }
        return true;
    case G4_bfn:
        // do not allow copy propagation to change BFN operand type
        if (isSrc && type != getOperand(opndNum)->getType())
        {
            return false;
        }
        // fall through
    case G4_add3:
        return type == Type_W || type == Type_UW || type == Type_D || type == Type_UD;
    }
}

// returns true if inst supports only F type for both src and dst
bool G4_INST::isFloatOnly() const
{
    switch (op)
    {
    default:
        return false;
    case G4_dp2:
    case G4_dp3:
    case G4_dp4:
    case G4_dph:
    case G4_frc:
    case G4_line:
    case G4_lrp:
    case G4_pln:
    case G4_rndd:
    case G4_rnde:
    case G4_rndu:
    case G4_rndz:
        return true;
    }
}

/// isSignSensitive() - Check whether this instruction is sign sensitive on the
/// specified source operand.
bool G4_INST::isSignSensitive(Gen4_Operand_Number opndNum) const
{
    const G4_Operand *use = getOperand(opndNum);
    G4_Type useType = use->getType();
    G4_Type dstType = dst->getType();

    // If extending is required, most of insts are sign sensitive.
    if (TypeSize(dstType) > TypeSize(useType)) {
        return true;
    }

    switch (op) {
    case G4_asr:
        if (opndNum != Opnd_src0)
            break;
        // FALL THROUGH
    case G4_mach:
    case G4_fbh:
    case G4_mulh:
    case G4_sel:
    case G4_cmp:
    case G4_cmpn:
    case G4_madw:
        return true;
    case G4_mov:
        // inttofp is sign sensitive
        return IS_TYPE_INT(useType) && IS_TYPE_FLOAT_ALL(dstType);
    default:
        break;
    }
    // By default, inst is regarded as sign insensitive.
    return false;
}

G4_Type G4_INST::getPropType(
    Gen4_Operand_Number opndNum, MovType MT, const G4_INST *mov) const
{
    const G4_Operand *use = getOperand(opndNum);
    G4_Type useType = use->getType();
    G4_Type srcType = mov->getSrc(0)->getType();

    G4_SrcModifier srcMod = Mod_src_undef;
    if (mov->getSrc(0)->isSrcRegRegion()) {
        srcMod = mov->getSrc(0)->asSrcRegRegion()->getModifier();
    }
    G4_SrcModifier useMod = Mod_src_undef;
    if (use->isSrcRegRegion()) {
        useMod = use->asSrcRegRegion()->getModifier();
    }

    bool useIsFP = IS_TYPE_FLOAT_ALL(useType);
    bool srcIsFP = IS_TYPE_FLOAT_ALL(srcType);
    // Different numeric type.
    bool diffNumTy = useIsFP != srcIsFP;

    // TODO: Once we handle IntToFp, this condition should be checked
    // individually for each MovType.

    switch (MT) {
    case Copy:
        // Different numeric type with src mod cannot be propagated.
        if (diffNumTy && srcMod != Mod_src_undef)
            return Type_UNDEF;
        // Fp is simply to use useType.
        if (useIsFP)
            return useType;
        // Int needs to consider whether the use is sign-sensitive and the src
        // modifier.
        if (isSignSensitive(opndNum)) {
            switch (srcMod) {
            case Mod_Not:
            case Mod_Minus:
            case Mod_Minus_Abs:
                if (IS_UNSIGNED_INT(useType))
                    return Type_UNDEF;
                // Assume the combination of srcMod/srcType is valid.
                // FALL THROUGH
            case Mod_Abs:
                return srcType;
            default:
                break;
            }
        }
        else if (srcMod == Mod_Abs && IS_UNSIGNED_INT(useType) &&
                 IS_SIGNED_INT(srcType))
            return srcType;
        return useType;
    case ZExt:
        // Different numeric type with src zero-extended cannot be propagated.
        if (diffNumTy)
            return Type_UNDEF;
        // (sext (zext x)) is equal to (zext x)
        return srcType;
    case SExt:
        // Different numeric type with src sign-extended cannot be propagated.
        if (diffNumTy)
            return Type_UNDEF;
        // (zext (sext x)) is not equal to (sext x)
        if (IS_UNSIGNED_INT(useType))
            return Type_UNDEF;
        // Check if there's any modifier on the use.
        switch (useMod) {
        case Mod_Not:
        case Mod_Minus:
        case Mod_Minus_Abs:
            if (IS_QTYPE(useType) && IS_DTYPE(srcType)) {
                // (- (sext x)) is not equal to (sext (-x)) due to the corner case
                // where x is INT_MIN and -x is still INT_MIN without being
                // extended.
                return Type_UNDEF;
            }
            // FALL THROUGH
        default:
            break;
        }
        return srcType;
    case Trunc:
        if (diffNumTy)
            return Type_UNDEF;
        // Truncation always use the useType but the original source operand.
        // As a result, region needs changing to access the truncated bits
        // only.
        return useType;
    case FPUpConv:
        // Different numeric type with src up-converted cannot be propagated.
        if (diffNumTy)
            return Type_UNDEF;
        return srcType;
    case FPDownConvSafe:
        return srcType;
    default:
        break;
    }

    return Type_UNDEF;
}

static bool isLegalImmType(G4_Type type)
{
    return type != Type_BF;
    return true;
}

// cases that we do not propagate
// 0. use inst does not support the type of the operand being propagated
// 1. use inst is align16 instruction
// 2. first source of line
// 3. indirect source to compressed instructions or math instructions
// 4. byte src to if/while instructions
// 5. src with modifier to logic inst on BDW
// 6. When useinst is lifetime.end
// 7. use inst does not have dst
bool G4_INST::canPropagateTo(
    G4_INST *useInst, Gen4_Operand_Number opndNum, MovType MT, bool inSimdFlow, bool statelessAddr)
{
    G4_Operand *src = srcs[0];
    bool indirectSrc = src->isSrcRegRegion() &&
                       src->asSrcRegRegion()->getRegAccess() != Direct;
    bool hasModifier = src->isSrcRegRegion() &&
                       src->asSrcRegRegion()->getModifier() != Mod_src_undef;
    G4_Type dstType = dst->getType();
    G4_Type srcType = src->getType();

    G4_Operand *use = useInst->getOperand(opndNum);
    G4_Type useType = use->getType();

    //If the operand to be copied is acc register, need to check if the use operand can use acc register
    if (src->isAccReg())
    {
        if (!useInst->canSrcBeAccBeforeHWConform(opndNum))
        {
            return false;
        }
    }

    if (useInst->is2SrcAlign16())
    {
        // don't copy propagate for the legacy dp* instructions,
        // as we are missing some HW conformity checks for them
        return false;
    }

    // Skip lifetime.
    if (useInst->isLifeTimeEnd())
    {
        return false;
    }

    // Skip dpas as it has no region (maybe too conservative)
    if (useInst->isDpas())
    {
        return false;
    }

    // skip the instruction has no dst. e.g. G4_pseudo_fcall
    if (useInst->getDst() == nullptr)
        return false;

    // limit flag copy propagation to opcode known to work for now
    if (src->isFlag() && (useInst->opcode() != G4_not && useInst->opcode() != G4_and))
    {
        return false;
    }

    if (isMixedMode())
    {
        // FIXME: what's this for?
        if (execSize < g4::SIMD16 && MT == FPDownConvSafe && useInst->execSize == g4::SIMD16 &&
            !useInst->isMixedMode())
        {
            return false;
        }

        G4_opcode useOp = useInst->opcode();

        if (useOp != G4_mov &&
            useOp != G4_mul &&
            useOp != G4_pseudo_mad &&
            useOp != G4_add &&
            useOp != G4_sel &&
            useOp != G4_cmp)
        {
            return false;
        }
    }
    else if (srcType != useType && (useInst->opcode() == G4_mulh || useInst->opcode() == G4_madw))
    {
        // don't propagate widening ops into a mul/mach
        //   mov  T:d  SRC:w
        //   ...
        //   mach ... T:d ...
        // mach requires 32b types only
        return false;
    }


    // special checks for message desc/extended desc, which must be either a0 or imm
    if (useInst->isSend())
    {
        auto msgDescOpnd = useInst->isSplitSend() ? Opnd_src2 : Opnd_src1;
        if (opndNum == msgDescOpnd)
        {
            if (!src->isImm() && !src->isAddress())
            {
                return false;
            }
        }
        if (opndNum == Opnd_src3)
        {
            // there are some HW restrictions that prevent imm exdesc (e.g., on MRT write),
            // so we conservatively disable copy prop here
            return false;
        }
    }

    // The following are copied from local dataflow analysis.
    // TODO: re-examine..
    if (((opndNum == Opnd_src0 && useInst->isSend()) && !statelessAddr) ||
        (opndNum == Opnd_src1 && useInst->isSplitSend()))
    {
        return false;
    }

    auto isFloatPseudoMAD = [](G4_INST *inst)
    {
        return inst->opcode() == G4_pseudo_mad && IS_TYPE_FLOAT_ALL(inst->getDst()->getType());
    };

    //     mov (16|M0) r47.0 1:w
    // (W) add (16|M0) r49.0 r47.0 r45.0
    //
    // FIXME: remove this once DU/UD chain are computed correctly.
    //
    // Only skip when the defInst ('this') is defined in SIMD CF.
    if (useInst->isWriteEnableInst() && !isWriteEnableInst() && inSimdFlow)
    {
        return false;
    }

    if (useInst->opcode() == G4_fcvt)
    {
        // fcvt is not allowed to have immediate src.
        if (src->isImm() ||
            !src->isSrcRegRegion() ||
            !(src->asSrcRegRegion()->getRegion()->isContiguous(useInst->getExecSize())))
        {
            return false;
        }
    }
    if (useInst->opcode() == G4_srnd)
    {
        // srnd rZ.0<1>:ub  rX.0<1;1,0>:hf rY.0<1;1,0>:hf
        //   operands should be packed.
        if (useInst->getDst()->getType() == Type_UB &&
            src->isSrcRegRegion() &&
            !(src->asSrcRegRegion()->getRegion()->isContiguous(useInst->getExecSize())))
        {
            return false;
        }
    }

    if (src->isImm())
    {
        if (isFloatPseudoMAD(useInst) || useInst->opcode() == G4_math ||
            use->asSrcRegRegion()->hasModifier())
        {
            return false;
        }
    } else if (indirectSrc &&
               (isFloatPseudoMAD(useInst) || useInst->opcode() == G4_math))
    {
        return false;
    }
    if (getBuilder().getGRFSize() == 64 &&
        (useInst->opcode() == G4_dpas || useInst->opcode() == G4_dpasw) &&
        (opndNum == Opnd_src0 || opndNum == Opnd_src1))
    {
        uint32_t leftBoundInBytes = src->getLeftBound() * src->getTypeSize();
        // left bound should be 2grf aligned to propagate into dpas.
        if (leftBoundInBytes % (getBuilder().numEltPerGRF<Type_UB>()*2))
        {
            return false;
        }
    }

    // FIXME: to add specific checks for other instructions.
    G4_opcode useInst_op = useInst->opcode();

    if (useInst_op == G4_madm || (useInst->isMath() && useInst->asMathInst()->isIEEEMath()))
    {
        // do not propagate if useInst uses mme registers
        return false;
    }
    if ((useInst_op == G4_line && opndNum == Opnd_src0) ||
        (hasModifier && G4_Inst_Table[useInst_op].instType == InstTypeLogic))
    {
        return false;
    }

    bool isVxHSrc = indirectSrc && src->asSrcRegRegion()->getRegion()->isRegionWH();
    if (isVxHSrc && (useInst->getExecSize() != execSize || execSize >= g4::SIMD8))
    {
        // copy propagating VxH region may result in address spills later so it's usually a net loss
        return false;
    }

    if ((useInst_op == G4_asr || useInst_op == G4_shl || useInst_op == G4_shr) &&
        opndNum == Opnd_src0 && src->getTypeSize() < use->getTypeSize())
    {
        // Handle cases such as
        //     mov  A:q  B:d
        //     asr  r:d  A:q  C:q
        //  if C is immediate and its value is in 0:31 (for d), it is okay to prop;
        //  otherwise, no.
        G4_Operand* src1 = useInst->getOperand(Opnd_src1);
        if (src1->isImm())
        {
            // shiftAmt is LSB[0:useTypeBits - 1]
            int64_t v = src1->asImm()->getImm();
            uint32_t shiftAmt = (uint32_t)((uint64_t)v & (use->getTypeSize()*8 - 1));
            uint32_t nbits = 8 * src->getTypeSize();
            if (shiftAmt >= nbits)
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    // In general, to check whether that MOV could be propagated:
    //
    //  dst/T1 = src/T0;
    //  op(..., use/T2, ...);
    //
    // We need firstly check whether 'dst' and 'use' are exactly the same
    // variable regardless data type.

    // Check T1 and T2 has the same bit/byte size. Otherwise, it's not legal to
    // be propagated.
    // TODO: Revisit later if exection mask is guaranteed to be NoMask.
    if (TypeSize(dstType) != TypeSize(useType) && !statelessAddr) {
        return false;
    }

    // Do not propagate if def type is float and use type is int, or vice
    // versa.
    // NOTE: Such usage is possible from bitcast (not through this MOV but the
    // reference in the use insn) from one type to another.
    // TODO: Revisit this later to handle the case where this MOV insn is
    // exactly a COPY. The useType should be used instead.
    if (MT != Copy && ((IS_TYPE_FLOAT_ALL(dstType) && IS_TYPE_INT(useType)) ||
                       (IS_TYPE_INT(dstType) && IS_TYPE_FLOAT_ALL(useType))))
    {
        return false;
    }

    if (MT == Copy &&
        hasModifier &&
        dstType != useType)
    {
        return false;
    }

    if (hasModifier && !useInst->canSupportSrcModifier())
    {
        return false;
    }

    // Check 'dst' of MOV and 'use' are the same variable. Otherwise, it's not
    // legal to be propagated.
    G4_CmpRelation rel = dst->compareOperand(use, getBuilder());
    if (rel != Rel_eq)
    {
        return false;
    }

    // Type to be used after propagation. Use srcType by default.
    G4_Type propType = useInst->getPropType(opndNum, MT, this);

    if (propType == Type_UNDEF || (src->isImm() && !isLegalImmType(propType)))
    {
        return false;
    }

    // bfloat specific checks
    if (propType == Type_BF)
    {
        // If the useInst is G4_pseudo_mad and the use operand has source modifier, a invalid bf->bf mov with source modifier
        // may be inserted in fixMADInst(). So avoid propagating to G4_pseudo_mad source with source modifier.
        // TODO: a mov is not always inserted for G4_pseudo_mad source with source modifier since gen mad inst supports source
        // modifier. So for the no mov inserted case, avoid propagating may miss this opotimize. So, do we need to check if a mov
        // is really needed for G4_pseudo_mad source here? But the same check code in fixMADInst() seems very complicated?
        if (use->asSrcRegRegion()->hasModifier() && (useInst->isMov() || useInst->opcode() == G4_pseudo_mad))
        {
            // BF_CVT does not like source modifier
            return false;
        }
        if (src->isSrcRegRegion() && src->asSrcRegRegion()->isScalar() &&
            useInst->opcode() != G4_mov)
        {
            // HW has bug with scalar bfloat in mix mode instructions
            return false;
        }
        if (useInst->getDst()->getType() != Type_F)
        {
            // we currently don't handle BF->HF or BF->DF conversion
            return false;
        }
    }

    // Don't propagate unsupported propType.
    if (!useInst->isLegalType(propType, opndNum))
    {
        return false;
    }

    // TODO: Revisit this later as IntToFp could be folded on specific insts,
    // such as add, cmp, and mul, when types of all source operands could be
    // consistent.
    if (!(useInst->isRawMov() && dstType == useType) &&
        !(MT == Copy && propType == useType) &&
        ((IS_FTYPE(dstType) && (IS_TYPE_INT(propType) || IS_VINTTYPE(propType))) ||
         (IS_TYPE_INT(dstType) && (IS_FTYPE(propType) || IS_VFTYPE(propType)))))
    {
        return false;
    }

    if (useInst->getSingleDef(opndNum) == nullptr)
    {
        return false;
    }

    // Cannot generally safely propagate replicated vectors.
    unsigned dstElSize = TypeSize(dstType);
    unsigned srcElSize = TypeSize(propType);
    unsigned useElSize = TypeSize(useType);

    const RegionDesc *rd =
        src->isSrcRegRegion() ? src->asSrcRegRegion()->getRegion() : nullptr;
    G4_ExecSize newExecSize = useInst->getExecSize();
    if ((useElSize != dstElSize && !statelessAddr) &&
        (!src->isSrcRegRegion()
         || rd->isRepeatRegion(execSize)
         || !(rd->isFlatRegion() && rd->isPackedRegion())))
    {
        return false;
    }

    // Skip propagate scalar copies into the additive operand (src2) of integer
    // pseudo mad.
    if (!builder.hasAlign1Ternary())
    {
        if (opndNum == Opnd_src2 && useInst->opcode() == G4_pseudo_mad &&
            IS_TYPE_INT(useType) && rd && rd->isScalar())
            return false;
    }

    // Check repeat region
    bool sameDefUseELSize = (dstElSize == useElSize);
    bool sameExecSize = (execSize == newExecSize);
    const RegionDesc *useRd =
        use->isSrcRegRegion() ? use->asSrcRegRegion()->getRegion() : nullptr;
    bool repeatUseRegion = useRd && useRd->isRepeatRegion(newExecSize);
    bool scalarUse = useRd && useRd->isScalar();
    bool repeatSrcRegion = (rd && rd->isRepeatRegion(execSize));
    if (!sameExecSize && !statelessAddr &&
        !((sameDefUseELSize && scalarUse) ||
          (!repeatUseRegion && rd && rd->isFlatRegion() && rd->isPackedRegion()) ||
          (repeatUseRegion && sameDefUseELSize && (src->isImm() || !repeatSrcRegion))))
    {
        return false;
    }

    // Be conserversative, do not bother to do complicated region compositions.
    // There are three variables to compute the composition:
    // (1) the dst stride
    // (2) the source region
    // (3) the use source region

    // dStride, the dst stride
    // stride1, stride2 must be positive
    auto isComposable = [=](unsigned dStride, unsigned stride1,
                            unsigned stride2) -> bool
    {
        MUST_BE_TRUE(stride1 && stride2, "scalar region not expected");

        // composition is rd1 (or rd2).
        // If two variables are trivial, then the other variable could be
        // arbitrary. E.g.
        //
        // mov (8) V81(0,0)<1>:w V80(0,0)<1;1,0>:w
        // add (16) V82(0,0)<1>:w V81(0,0)<0;8,1>:w 0xa:w
        //
        // where rd1 has stride 1, dStride = 1, rd2 is non single strided.
        if ((stride1 == 1 && dStride == 1) || (stride2 == 1 && dStride == 1))
          return true;

        // If either stride1 or stride2 equals UndefVal, then there is no easy
        // formula to do the composition unless dStride == 1 and the other has
        // stride 1. This case is covered by the first check.
        //
        // To be composable, both regions need to be single strided (i.e. value
        // != UndefVal). This check is simplified by the value UndefVal (64).
        return stride1 * stride2 * dStride <= 32;
    };

    if (!sameExecSize && rd && useRd)
    {
        // the compoisition is also scalar.
        if (!rd->isScalar() && !useRd->isScalar())
        {
            G4_DstRegRegion *dstRegion = dst;
            uint16_t dstStride = dstRegion->getHorzStride();

            // A value to indicate this region is non-single strided.
            // Make it larger than 32 to simplify/unify the checking.
            const uint16_t UndefVal = 64;

            uint16_t stride1 = UndefVal;
            if (rd->isContiguous(execSize))
                stride1 = 1;
            else
                rd->isSingleNonUnitStride(execSize, stride1);

            uint16_t stride2 = UndefVal;
            if (useRd->isContiguous(newExecSize))
                stride2 = 1;
            else
                useRd->isSingleNonUnitStride(newExecSize, stride2);

            if (!isComposable(dstStride, stride1, stride2))
                return false;
        }
    }

    // check data type alignment
    if ((srcElSize < useElSize) &&
        (dstElSize == srcElSize) &&
        (execSize > g4::SIMD1) &&
        (!src->isImm()) &&
        ((src->getByteOffset() % useElSize) != 0))
    {
        return false;
    }

    if (src->isImm() && use->asSrcRegRegion()->hasModifier())
    {
        //FIXME: do we need to worry about signal bit in NaN being dropped?
        if (IS_TYPE_INT(srcType))
        {
            // we can't represent -(INT_MIN) or abs(INT_MIN)
            int64_t value = src->asImm()->getImm();
            switch (propType)
            {
            case Type_Q:
            case Type_UQ:
                return value != LLONG_MIN;
            default:
                return value != INT_MIN;
            }
        }
    }

    return true;
}

// check if this inst can be hoisted
// assume only MOV inst is checked
bool G4_INST::canHoist(bool simdBB, const Options *opt) const
{
    assert(op == G4_mov && "defHoisting only handles mov");
    if (dst == NULL)
    {
        return false;
    }

    G4_Operand *src = srcs[0];
    // check attributes of src and number of defs
    bool archRegSrc = (src->isFlag() || src->isAreg() || src->isAddress());
    bool indirectSrc = (src->getTopDcl() && src->getTopDcl()->getAddressed()) || src->getRegAccess() != Direct;
    bool noMultiDefOpt = ((defInstList.size() > 1) &&
        (predicate || (dst->getRegAccess() != Direct) || simdBB));
    if (src->isImm() ||
        archRegSrc ||
        indirectSrc ||
        (src->isSrcRegRegion() && src->asSrcRegRegion()->getModifier() != Mod_src_undef) ||
        (defInstList.size() == 0) ||
        noMultiDefOpt)
    {
        return false;
    }

    // check type
    G4_Type dstType, srcType;
    dstType = dst->getType();
    srcType = src->getType();

    // no dst type promotion after hoisting
    if (!Is_Type_Included(dstType, srcType, builder) ||
        // if multi def, src and dst should have the same type size
        (defInstList.size() > 1 &&
        (Operand_Type_Rank(srcType) != Operand_Type_Rank(dstType) ||
        // if multidef and used as a scalar, execution size should be one.
        (src->isSrcRegRegion() && src->asSrcRegRegion()->isScalar() && execSize > g4::SIMD1))))
    {
        return false;
    }

    // no opt repeat region
    unsigned short src_wd = src->asSrcRegRegion()->getRegion()->width;
    if ((src_wd != execSize &&
        (src->asSrcRegRegion()->getRegion()->vertStride < (src_wd * src->asSrcRegRegion()->getRegion()->horzStride))) ||
        // actually we can hoist if src is a scalar and target inst has no pred or cond mod.
        (execSize > g4::SIMD1 && src->asSrcRegRegion()->isScalar()))
    {
        return false;
    }

    if (src->getTopDcl() && src->getTopDcl()->isOutput())
    {
        return false;
    }

    return true;
}

// check if this instruction can be hoisted to defInst
bool G4_INST::canHoistTo(const G4_INST *defInst, bool simdBB) const
{
    assert(op == G4_mov && "defHoisting only handles mov");
    bool indirect_dst = (dst->getRegAccess() != Direct);

    auto def_dst = defInst->getDst();

    if (!def_dst)
    {
        // can this actually happen?
        return false;
    }
    G4_Type defDstType = def_dst->getType();
    G4_Type dstType = dst->getType(), srcType = srcs[0]->getType();
    unsigned int srcElSize = TypeSize(srcType);
    unsigned int dstElSize = TypeSize(dstType);
    unsigned int defDstElSize = TypeSize(defDstType);

    // cannot hoist an accumulator access into an instruction
    // that doesn't have a dst hz stride that matches source
    //   def (..) T<1> .. acc:d
    //   use (..) ...<2>:d  T<1>
    // ==>
    //   def2 (..) ...<2>:d ... acc
    //                 ^ dst stride mismatch means we mustn't hoist
    if (defInst->useAcc() && dst->getExecTypeSize() != srcElSize) {
        return false;
    }

    bool rawMovInst = isRawMov();
    bool cantHoistMAD =
        (defInst->opcode() == G4_pseudo_mad &&
            !(IS_TYPE_FLOAT_ALL(dstType) && IS_TYPE_FLOAT_ALL(defDstType)));
    if ((defInst->useInstList.size() != 1) ||
        (defInst->opcode() == G4_sad2) ||
        (defInst->opcode() == G4_sada2) ||
        (defInst->opcode() == G4_cbit && dstType != defDstType) ||
        (defInst->opcode() == G4_dp4a && dstType != defDstType) ||
        ((cantHoistMAD || (defInst->opcode() == G4_math)) &&
         (indirect_dst || (dstType != defDstType && !rawMovInst))))
    {
        return false;
    }

    if (!defInst->isLegalType(dstType, Opnd_dst))
    {
        return false;
    }

    if (isMixedMode())
    {
        G4_opcode defOp = defInst->opcode();

        if (defOp != G4_mov &&
            defOp != G4_mul &&
            defOp != G4_pseudo_mad &&
            defOp != G4_add &&
            defOp != G4_sel &&
            defOp != G4_cmp)
        {
            return false;
        }
        if (!builder.hasMixMode())
        {
            // normally we should disable the opt, but for the special case where
            // defInst is a move with integer source, we can still hoist since it
            // won't produce a mixed mode inst
            if (!(defInst->isMov() && IS_TYPE_INT(defInst->getSrc(0)->getType())))
            {
                return false;
            }
        }
        if (!builder.getOption(vISA_ignoreBFRounding) && dstType == Type_BF && defOp != G4_mov)
        {
            // F->BF move has RNE mode while mix mode BF uses RTZ due to HW bug
            // so we have to disallow the def-hoisting
            return false;
        }
    }

    if (dst->isAddress() && defInst->getNumSrc() == 3)
    {
        // no A0 dst for ternary instructions
        return false;
    }

    // compare boudaries and bitset
    if ((def_dst->getLeftBound() < srcs[0]->getLeftBound()) ||
        (def_dst->getRightBound() > srcs[0]->getRightBound()))
    {
        return false;
    }

    if (getSaturate() && !defInst->canSupportSaturate())
    {
        return false;
    }

    // check mixed type conversion
    // TODO: cleanup this part since mixed type check of the first half is already checked in canHoist.
    if ((!(defInst->isRawMov() && (defDstType == srcType)) &&
        ((IS_FTYPE(dstType) && (IS_TYPE_INT(srcType) || IS_VINTTYPE(srcType))) ||
        ((IS_FTYPE(srcType) || IS_VFTYPE(srcType)) && IS_TYPE_INT(dstType)))) ||
        (!rawMovInst &&
        ((IS_FTYPE(defDstType) && IS_TYPE_INT(defInst->getExecType())) ||
        (IS_FTYPE(defInst->getExecType())  && IS_TYPE_INT(defDstType)))))
    {
        return false;
    }

    if (!rawMovInst && (defInst->getSrc(0) &&
        (IS_DFTYPE(defInst->getSrc(0)->getType()) || IS_FTYPE(defInst->getSrc(0)->getType()))) &&
        (IS_SIGNED_INT(defDstType) || IS_UNSIGNED_INT(defDstType)))
    {
        // Sequence that should not be optimized:
        // mov V1:d    V2:df
        // mov V3:uw    V1:d
        //
        // This is *NOT* a candidate for:
        // mov V3:uw    V2:df
        //
        // In general, df/f->int performs saturation and unless value of
        // df/f is known, the result of mov may differ based on type
        // of dst.
        return false;
    }

    // no def hoisting for sends for now
    if (defInst->isSend())
    {
        return false;
    }

    if (defInst->opcode() == G4_mov && defInst->getSrc(0)->isFlag())
    {
        // TODO: check if use is a predicate, if not, can propagate?
        return false;
    }

    if (simdBB && (defInst->isWriteEnableInst() ^ isWriteEnableInst()))
    {
        // no opt if one isNoMask but the other is not
        return false;
    }

    if (defInst->getMaskOffset() != getMaskOffset() &&
        (simdBB || getPredicate() || getCondMod() ||
         defInst->getPredicate() || defInst->getCondMod()))
    {
        // no opt if their mask offset do not match,
        // and mov/defInst has flags
        return false;
    }

    if ((getPredicate() || getCondMod()) && (defInst->getPredicate() || defInst->getCondMod()))
    {
        // can't have both inst using flags
        return false;
    }

    bool same_type_size = def_dst->getTypeSize() == TypeSize(srcType);
    bool scalarSrc = srcs[0]->asSrcRegRegion()->isScalar();
    // handle predicated MOV and float def
    if ((getPredicate() && (execSize > g4::SIMD1) && !same_type_size) ||
        (IS_FTYPE(defDstType) && (defDstType != srcType) && (dstType != srcType)))
    {
        return false;
    }

    // if used as scalar and repeated region, dst should be packed
    // add(2) v2<1>:w v3 v4
    // mov(2) v5<2>:d  V2<0;1,0>:d
    if (scalarSrc && !same_type_size &&
        (execSize > g4::SIMD1) && (dst->getHorzStride() != 1))
    {
        return false;
    }

    // if indirect source is repeat region, or defhoisting will make it a repeat region,
    // no opt
    if (srcs[0]->asSrcRegRegion()->getRegion()->isRepeatRegion(execSize) &&
        !scalarSrc)
    {
        return false;
    }

    // check type conversion
    if (IS_SIGNED_INT(dstType) && (defInst->opcode() == G4_mov) &&
        (TypeSize(dstType) > srcElSize) &&
        ((IS_SIGNED_INT(defDstType) && IS_UNSIGNED_INT(defInst->getSrc(0)->getType())) ||
        (IS_UNSIGNED_INT(defDstType) && IS_SIGNED_INT(defInst->getSrc(0)->getType()))))
    {
        return false;
    }

    // check alignment and saturate
    if (((srcElSize > defDstElSize) || defInst->getSaturate()) && (srcType != dstType))
    {
        return false;
    }

    uint16_t dstHS = dst->getHorzStride();
    uint16_t srcHS = 0;
    const RegionDesc *srcRd = srcs[0]->asSrcRegRegion()->getRegion();
    if (!srcRd->isSingleStride(execSize, srcHS))
    {
        return false;
    }
    if ((srcElSize < defDstElSize) && ((dstHS > 1) || (srcHS > 1)))
    {
        return false;
    }
    if ((dstElSize != defDstElSize) && (srcElSize == dstElSize) &&
        (indirect_dst || ((dst->getByteOffset() % defDstElSize) != 0) ||
        (dstHS != srcHS)))
    {
        return false;
    }

    // dont hoist stack calls related variables (Arg, Retval, SP, FP)
    if (defInst->getDst() && defInst->getDst()->getTopDcl())
    {
        G4_Declare* defDstDcl = defInst->getDst()->getTopDcl()->getRootDeclare();
        if (builder.isPreDefFEStackVar(defDstDcl) || builder.isPreDefArg(defDstDcl) ||
            builder.isPreDefRet(defDstDcl))
        {
            return false;
        }
    }

    // For mov HF F, we have to check if the def Inst supports HF
    if (dstType != Type_F && defInst->isFloatOnly() && !isRawMov())
    {
        return false;
    }

    // Before:
    // or (8) V100(0,0)<1>:d ...
    // or (8) V100(1,0)<1>:d ...
    // mov (16) V101(0,0)<1>:b    V102(0,0)<16;16,1>:w <-- V102 is alias of V100
    // mov (16) V101(0,16)<1>:b   V102(1,0)<16;16,1>:w

    // After (invalid optimization):
    // or (8) V100(0,0)<1>:d ...
    // or (8) V100(0,4)<1>:d ...
    if (defDstType != srcType)
    {
        if (isRawMov() == false)
        {
            return false;
        }
    }

    // As dst's type of shl inst decides what shifting amt should be used,
    // make sure shifting amt would not be changed after doing hoisting.
    //    shl (1) V00(0,0)<1>:q V101(0,0):w  V102(0,0)<0;1,0>:q
    //    mov(1) V103(0, 0)<1>:b V100(0, 0)<0;1,0 >:q
    // Cannot do it for this case.
    if (defInst->opcode() == G4_shl || defInst->opcode() == G4_shr || defInst->opcode() == G4_asr)
    {
        uint32_t defSrc0Bytes = defInst->getSrc(0)->getTypeSize();
        bool QMode = (defDstElSize == 8 || defSrc0Bytes == 8);
        if ((QMode && defSrc0Bytes != 8 && dstElSize != 8) ||
            (!QMode && dstElSize == 8))
        {
            // Disable it; otherwise shift's mode is changed illegally!
            return false;
        }
    }

    // Cannot do hoisting if the use inst has src modifier.
    if (getSrc(0)->asSrcRegRegion()->hasModifier())
    {
        return false;
    }

    if (getBuilder().getGRFSize() == 64 &&
        (defInst->opcode() == G4_dpas || defInst->opcode() == G4_dpasw))
    {
        uint32_t leftBoundInBytes = dst->getLeftBound() * dst->getTypeSize();
        // left bound should be 2grf aligned to hoist dst into dpas.
        if (leftBoundInBytes % (getBuilder().numEltPerGRF<Type_UB>() * 2))
        {
            return false;
        }
    }
    if (defInst->opcode() == G4_fcvt)
    {
        return false;
    }
    if (defInst->opcode() == G4_srnd)
    {
        return false;
    }

    return true;
}

// check if the sources of an inst is commutative
// besides the property shown in inst table, some INT MUL instructions
// are not commutative due to HW restrictions
bool G4_INST::isCommutative() const
{
    //TODO: we can invert condMod of cmp to swap sources
    if (!(G4_Inst_Table[op].attributes & ATTR_COMMUTATIVE) || op == G4_cmp)
        return false;

    // for mul we can do D*W but not W*D
    if (op == G4_mul)
    {
        if (IS_DTYPE(srcs[0]->getType()))
        {
            return false;
        }
    }
    return true;
}

bool G4_INST::hasNULLDst() const
{
    if (dst && dst->isNullReg())
    {
        return true;
    }

    return false;
}

bool G4_INST::goodTwoGRFDst(bool& evenSplitDst)
{
    evenSplitDst = false;
    // The following applies to all platforms
    // The problem is , the first case is really an instruction with two destination registers.
    // in which case, hardware breaks into two operations. When this happens, hardware cannot update flag registers.
    // I.e., if execution size is 8 or less and the destination register is 2, flag updates are not supported.
    // -naveen

    if (!dst || hasNULLDst())
    {
        evenSplitDst = true;
        return true;
    }
    else
    {
        evenSplitDst = dst->evenlySplitCrossGRF(getBuilder(), execSize);
        // check if elements evenly split between two GRFs
        if (evenSplitDst)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}

// check if there is WAW, WAR, RAW dependency between the passing-in inst and this instruction
// there is no check for the case that two instructions are both send, since these checks are
// only used in def-joisting and copy propagation
bool G4_INST::isWARdep(G4_INST* inst)
{
    G4_Operand* msg0 = NULL;
    G4_Operand* src0_0 = inst->getSrc(0);
    G4_Operand* src0_1 = inst->getSrc(1);
    G4_Operand* src0_2 = inst->getSrc(2);
    G4_Operand* src0_3 = inst->getSrc(3);
    G4_Operand* implicitSrc0 = inst->getImplAccSrc();
    G4_Predicate* pred0 = inst->getPredicate();

    G4_Operand* dst1 = dst;

    if (dst1 && !hasNULLDst())
    {

        if (
            (src0_0 && src0_0->compareOperand(dst1, getBuilder()) != Rel_disjoint) ||
            (src0_1 && src0_1->compareOperand(dst1, getBuilder()) != Rel_disjoint) ||
            (src0_2 && src0_2->compareOperand(dst1, getBuilder()) != Rel_disjoint) ||
            (src0_3 && src0_3->compareOperand(dst1, getBuilder()) != Rel_disjoint) ||
            (msg0 && (msg0->compareOperand(dst1, getBuilder()) != Rel_disjoint)) ||
            (pred0 && (pred0->compareOperand(dst1, getBuilder()) != Rel_disjoint)) ||
            (implicitSrc0 && (implicitSrc0->compareOperand(dst1, getBuilder()) != Rel_disjoint)))
        {
            return true;
        }
    }

    if (mod)
    {
        if ((pred0 && pred0->compareOperand(mod, getBuilder()) != Rel_disjoint) ||
            (src0_0 && src0_0->isFlag() && src0_0->compareOperand(mod, getBuilder()) != Rel_disjoint) ||
            (src0_1 && src0_1->isFlag() && src0_1->compareOperand(mod, getBuilder()) != Rel_disjoint) ||
            (src0_2 && src0_2->isFlag() && src0_2->compareOperand(mod, getBuilder()) != Rel_disjoint))
        {
            return true;
        }
    }

    if (implAccDst)
    {
        if ((implicitSrc0 && implicitSrc0->compareOperand(implAccDst, getBuilder()) != Rel_disjoint) ||
            (src0_0 && src0_0->isAccReg() && src0_0->compareOperand(implAccDst, getBuilder()) != Rel_disjoint) ||
            (src0_1 && src0_1->isAccReg() && src0_1->compareOperand(implAccDst, getBuilder()) != Rel_disjoint) ||
            (src0_2 && src0_2->isAccReg() && src0_2->compareOperand(implAccDst, getBuilder()) != Rel_disjoint))
        {
            return true;
        }
    }
    return false;
}

bool G4_INST::isWAWdep(G4_INST *inst)
{
    G4_Operand *dst0 = inst->getDst();
    G4_Operand *dst1 = dst;
    G4_CondMod *cMod0 = inst->getCondMod();
    G4_CondMod *cMod1 = mod;
    G4_Operand *implicitDst0 = inst->getImplAccDst();
    G4_Operand *implicitDst1 = implAccDst;

    bool NULLDst1 = !dst1 || hasNULLDst();
    if (dst0 && !inst->hasNULLDst())
    {
        if ((!NULLDst1 && dst1->compareOperand(dst0, getBuilder()) != Rel_disjoint) ||
            (implicitDst1 && implicitDst1->compareOperand(dst0, getBuilder()) != Rel_disjoint) ||
            (cMod1 && cMod1->getBase() && cMod1->compareOperand(dst0, getBuilder()) != Rel_disjoint))
        {
            return true;
        }
    }

    if (implicitDst0)
    {
        if ((!NULLDst1 && dst1->compareOperand(implicitDst0, getBuilder()) != Rel_disjoint) ||
            (implicitDst1 && implicitDst1->compareOperand(implicitDst0, getBuilder()) != Rel_disjoint))
        {
            return true;
        }
    }

    if (cMod0 && cMod0->getBase())
    {
        if ((!NULLDst1 && dst1->compareOperand(cMod0, getBuilder()) != Rel_disjoint) ||
            (cMod1 && cMod1->getBase() && cMod1->compareOperand(cMod0, getBuilder()) != Rel_disjoint))
        {
            return true;
        }
    }

    return false;
}
bool G4_INST::isRAWdep(G4_INST *inst)
{
    G4_Operand *dst0 = inst->getDst();
    G4_CondMod *cMod0 = inst->getCondMod();
    G4_Operand *implicitDst0   = inst->getImplAccDst();
    G4_Operand *msg1 = NULL;
    G4_Predicate *pred1   = getPredicate();
    G4_Operand *src1_0 = getSrc(0);
    G4_Operand *src1_1 = getSrc(1);
    G4_Operand *src1_2 = getSrc(2);
    G4_Operand* src1_3 = getSrc(3);
    G4_Operand *implicitSrc1   = implAccSrc;

    bool NULLSrc1 = (opcode() == G4_math && src1_1->isNullReg());
    if (dst0 && !inst->hasNULLDst())
    {
        if ((src1_0 && src1_0->compareOperand(dst0, getBuilder()) != Rel_disjoint) ||
            (src1_1 && !NULLSrc1 && src1_1->compareOperand(dst0, getBuilder()) != Rel_disjoint) ||
            (src1_2 && src1_2->compareOperand(dst0, getBuilder()) != Rel_disjoint) ||
            (src1_3 && src1_3->compareOperand(dst0, getBuilder()) != Rel_disjoint) ||
            (msg1 && msg1->compareOperand(dst0, getBuilder()) != Rel_disjoint) ||
            (pred1 && pred1->compareOperand(dst0, getBuilder()) != Rel_disjoint) ||
            (implicitSrc1 && implicitSrc1->compareOperand(dst0, getBuilder()) != Rel_disjoint))
        {
            return true;
        }
    }

    if (cMod0 && cMod0->getBase())
    {
        if ((pred1 && pred1->compareOperand(cMod0, getBuilder()) != Rel_disjoint) ||
            (src1_0 && src1_0->isFlag() && src1_0->compareOperand(cMod0, getBuilder()) != Rel_disjoint) ||
            (src1_2 && src1_2->isFlag() && src1_2->compareOperand(cMod0, getBuilder()) != Rel_disjoint) ||
            (src1_1 && src1_1->isFlag() && src1_1->compareOperand(cMod0, getBuilder()) != Rel_disjoint))
        {
            return true;
        }
    }

    if (implicitDst0)
    {
        if ((implicitSrc1 && implicitSrc1->compareOperand(implicitDst0, getBuilder()) != Rel_disjoint) ||
            (src1_0 && src1_0->isAccReg() && src1_0->compareOperand(implicitDst0, getBuilder()) != Rel_disjoint) ||
            (src1_2 && src1_2->isAccReg() && src1_2->compareOperand(implicitDst0, getBuilder()) != Rel_disjoint) ||
            (src1_1 && src1_1->isAccReg() && src1_1->compareOperand(implicitDst0, getBuilder()) != Rel_disjoint))
        {
            return true;
        }
    }
    return false;
}

bool G4_INST::detectComprInst() const
{
    enum class ComprInstStates : unsigned char { U, T, F };

    G4_Type execType = getExecType();
    ComprInstStates comprInst = ComprInstStates::U;

    // Compressed instructions must have a minimum execution size of
    // at least 8.
    if (execSize < g4::SIMD8)
    {
        comprInst = ComprInstStates::F;
    }

    // Compressed instructions must have a minimum execution size of
    // at least 16 if the execution type is less than DF.
    else if (dst &&
             dst->getHorzStride() != UNDEFINED_SHORT &&
             dst->getType() != Type_UNDEF)
    {
        if ((unsigned)execSize * dst->getTypeSize() * dst->getHorzStride() >
            getBuilder().numEltPerGRF<Type_UB>())
        {
            comprInst = ComprInstStates::T;
        }
        else
        {
            comprInst = ComprInstStates::F;
        }
    }

    // Uncompressed instructions can only operate on a max of 4 DFs or
    // 8 DF4/F/DWs or 16 W/Bs (the only exception being packed byte
    // moves which always have destinations).
    else if ((unsigned)execSize * TypeSize(execType) > getBuilder().numEltPerGRF<Type_UB>())
    {
        comprInst = ComprInstStates::T;
    }

    else
    {
        comprInst = ComprInstStates::F;
    }

    return (comprInst == ComprInstStates::T);
}

/*
 * Check to see if the interpretation of the i/p src region is unaffected by
 * virtue of it making it a src of the compressed op, as opposed to (if
 * possible) it appearing within a regular uncompressed op with the same exec
 * size.
 * Register-indirect operands are NOT compression invariant. The following 4 rules
 * are used to determine compression invariant register-direct opnds:
 *    1. constants, scalars, and ARF regions/registers are always compression invariant
 *    2. if both the dst region and the i/p source region are native packed
 *       regions, and the GRF source region is additionally of type W/UW
 *    3. the src region covers (i.e. vs(region) * rows(region)) exactly two
 *       registers (strides allowed), except when the dst region is a native
 *       packed region and the GRF source has packed rows of type W/UW
 *    4. the first src of line op is always considered compression invariant
 *       (this is a special case quadruple region of <0;4,1>)
 * e.g.
 *   (Both srcs are compression invariant in the following examples)
 *      add (16) r10.0<1>:d  r12.0<0;1,0>:w  0x80:w {CC}
 *      add (16) r10.0<2>:w  r12.0<8;8,1>:d  r14.0<16;8,2>:w {CC}
 *      add (16) r10.0<1>:d  r12.0<16;8,2>:w r14.0<32;8,4>:b {CC}
 *      add (16) r10.0<1>:d  r12.0<8;8,1>:w  r14.0<8;8,1>:w {CC}
 *      add (16) r10.0<1>:d  r12.0<4;4,1>:w  r14.0<4;4,1>:d {CC}
 *      add (32) r10.0<1>:w  r12.0<8;8,1>:w  r14.0<16;8,2>:b {CC}
 *      add (8)  r10.0<1>:df r12.0<4;4,1>:df r14.0<4;4,1>:df {CC}
 *      mov (8)  r10.0<1>:df r12.0<4;4,1>:w {CC}
 *   (Only the first src is compression invariant in the following examples)
 *      add (16) r10.0<1>:d  r12.0<8;8,1>:w  r14.0<16;8,2>:b {CC}
 *      add (16) r10.0<2>:w  r14.0<32;8,1>:b r12.0<16;8,1>:w {CC}
 *      add (16) r10.0<2>:w  r12.0<4;4,1>:d  r14.0<8;8,1>:w {CC}
 *      add (32) r10.0<1>:w  r12.0<8;8,1>:w  r14.0<8;8,1>:b {CC}
 *   (Neither src is compression invariant in the following examples)
 *      add (16) r10.0<2>:w  r12.0<8;8,1>:w  r14.0<16;8,2>:b {CC}
 *      add (32) r10.0<1>:w  r12.0<8;8,1>:b  r14.0<8;8,1>:b {CC}
 *      mov (8)  r10.0<1>:df r12.0<4;4,1>:dw {CC}
 * Inputs:
 *      src - the i/p src operand region
 *      src_pos - the position that the src operand appears in the list
 *                of src operands
 * Assumptions:
 *    - this function is only valid for compressed ops and it is invalid
 *      to call it for uncompressed ops
 */
bool
G4_INST::isComprInvariantSrcRegion(G4_SrcRegRegion* src, int srcPos)
{
    if (src == NULL)
    {
        return true;
    }
    else if (src->isImm() || src->isAddrExp())
    {
        return true;
    }
    else if (src->getRegAccess() != Direct)
    {
        return false;
    }
    else if (src->getBase()->asRegVar()->getDeclare()->getRegFile() != G4_GRF &&
             src->getBase()->asRegVar()->getDeclare()->getRegFile() != G4_INPUT)
    {
         return true;
    }

    const RegionDesc* region = src->getRegion();

    if (opcode() == G4_line && srcPos == 0)
    {
        return true;
    }
    else if (region->isScalar())
    {
        return true;
    }
    else
    {
        int num_rows  = getExecSize() / src->getRegion()->width;
        int type_sz   = (int)src->getTypeSize();
        int byte_size = src->getRegion()->vertStride * type_sz * num_rows;

        if (getDst() && getDst()->isNativePackedRegion() &&
            IS_WTYPE(src->getType())) {
            if (src->isNativePackedRegion()) {
                return true;
            }
            else if (src->isNativePackedRowRegion()) {
                return false;
            }
        }
        if (byte_size == 2 * getBuilder().numEltPerGRF<Type_UB>()) {
            return true;
        }
        else {
            return false;
        }
    }
}

bool G4_INST::isPartialWrite() const
{
    G4_Predicate* aPred = predicate;
    if (aPred && aPred->isSameAsNoMask())
    {
        // equivalent to NoMask (W) without predicate
        aPred = nullptr;
    }

    return (aPred != NULL && op != G4_sel) || op == G4_smov;
}

bool G4_INST::isPartialWriteForSpill(bool inSIMDCF) const
{
    if (!getDst() || hasNULLDst())
    {
        // inst does not write to GRF
        return false;
    }

    if (isPartialWrite())
    {
        return true;
    }

    if (inSIMDCF && !isWriteEnableInst())
    {
        if (builder.usesStack() || !(builder.hasMaskForScratchMsg() && getDst()->getElemSize() == 4))
        {
            // scratch message only supports DWord mask
            // also we can't use the scratch message when under stack call
            return true;
        }
    }

    return false;
}


bool G4_INST::isAccSrcInst() const
{
    if (srcs[0] && srcs[0]->isSrcRegRegion() && srcs[0]->asSrcRegRegion()->getBase()->isAccReg())
    {
        return true;
    }
    else if (getNumSrc() == 3 && srcs[1] != nullptr)
    {
        if (srcs[1]->isSrcRegRegion() && srcs[1]->asSrcRegRegion()->getBase()->isAccReg())
        {
            return true;
        }
    }
    return false;
}

// Check if this instruction has an explicit acc destination
bool G4_INST::isAccDstInst() const
{
    if (dst != NULL && dst->getBase()->isAccReg())
    {
        return true;
    }
    return false;
}

bool G4_INST::isArithAddr() const
{
    if (srcs[1] != NULL)
        return isArithmetic() && srcs[1]->isAddrExp();
    else
        return false;
}

bool G4_INST::isMovAddr() const
{
    if (srcs[0] != NULL)
        return isMov() && srcs[0]->isAddrExp();
    return false;
}

//
// Check if the operands of send instruction obey the symbolic register rule
// ToDo: this is obsolete and should be removed
//
bool G4_INST::isValidSymbolOperand(bool &dst_valid, bool *srcs_valid) const
{
    MUST_BE_TRUE(srcs_valid, ERROR_INTERNAL_ARGUMENT);

    bool obeyRule = true;
    if (dst && dst->getBase()->isRegVar())
    {
        dst_valid = dst->obeySymbolRegRule();
        if (!dst_valid)
            obeyRule = false;
    }
    else
        dst_valid = false;                          // does not change obeyRule for non-register-variable

    for (unsigned i = 0; i < G4_MAX_SRCS; i++)
    {
        G4_Operand* src = getSrc(i);
        if (src && src->isSrcRegRegion() && src->asSrcRegRegion()->getBase()->isRegVar())
        {
            srcs_valid[i] = src->asSrcRegRegion()->obeySymbolRegRule();
            if (!srcs_valid[i])
                obeyRule = false;
        }
        else
            srcs_valid[i] = false;                  // does not change obeyRule for non-register-variable
    }

    return obeyRule;
}

const G4_VarBase* G4_INST::getCondModBase() const
{
    if (!getCondMod())
        return nullptr;

    return getCondMod()->getBase();
}

bool G4_INST::isOptBarrier() const
{
    if (op == G4_join)
    {
        return true;
    }

    if (isIntrinsic() && asIntrinsicInst()->getIntrinsicId() == Intrinsic::MemFence)
    {
        return true;
    }

    // any instructions that access special ARFs is considered a opt barrier
    // this includes any ARF that is not address/flag/acc
    if (dst != NULL)
    {
        if (dst->isAreg())
        {
            if (dst->isNReg() ||
                dst->isSrReg() ||
                dst->isCrReg() ||
                dst->isTmReg() ||
                dst->isTDRReg())
            {
                return true;
            }
        }
    }

    for (int i = 0; i < getNumSrc(); i++)
    {
        if (getSrc(i))
        {
            if (getSrc(i)->isAreg())
            {
                if (getSrc(i)->isNReg() ||
                    getSrc(i)->isSrReg() ||
                    getSrc(i)->isCrReg() ||
                    getSrc(i)->isTmReg() ||
                    getSrc(i)->isTDRReg())
                {
                    return true;
                }
            }
        }
    }
    return false;
}


static void emitPredWrEn(std::ostream& output, G4_INST &inst)
{
    G4_Predicate *pred = inst.getPredicate();
    bool isNoMask = (inst.getOption() & InstOpt_WriteEnable) != 0;

    if (pred) {
        output << "(";
        if (isNoMask)
            output << "W&";
        pred->emit_body(output, false);
        output << ") ";
    } else if (isNoMask) {
        output << "(W) ";
    } else {
        output << "    "; // align for predication (.....)
    }
}

static void emitExecSize(std::ostream& output, const G4_INST &inst)
{
    auto execSize = static_cast<int>(inst.getExecSize());
    if (inst.opcode() != G4_nop && inst.opcode() != G4_wait)
    {
        output << '(';
        if (execSize == UNDEFINED_EXEC_SIZE) {
            output << "??";
        } else {
            output << execSize;
        }
        if (int execOffset = inst.getMaskOffset()) {
            // non-zero channel offset
            output << "|M" << execOffset;
        }
        output << ") ";
    }
}

// the syntax column width of beinning instruction info
//  (P1.0) and (16)     ...
//         nop
//         and (16|M0)  ...
//                      ^ aligns operand start to same place here
static const int INST_START_COLUMN_WIDTH = 24;

// emits the first part of an instruction in an aligned column
static void emitInstructionStartColumn(std::ostream& output, G4_INST &inst)
{
    std::stringstream oupPfx;
    emitPredWrEn(oupPfx, inst);

    oupPfx << G4_Inst_Table[inst.opcode()].str;
    if (inst.isIntrinsic())
    {
        oupPfx << "." << inst.asIntrinsicInst()->getName();
        if (inst.isSpillIntrinsic())
        {
            oupPfx << "." << inst.asSpillIntrinsic()->getNumRows();
        }
        else if (inst.isFillIntrinsic())
        {
            oupPfx << "." << inst.asFillIntrinsic()->getNumRows();
        }
    }
    else if (inst.opcode() == G4_goto)
    {
        oupPfx << (inst.asCFInst()->isBackward() ? ".bwd" : ".fwd");
    }
    else if (inst.isBfn()) {
        oupPfx << "." << fmtHex(inst.asBfnInst()->getBooleanFuncCtrl(), 2);
    }
    else if (inst.isMath() && inst.asMathInst()->getMathCtrl() != MATH_RESERVED)
    {
        oupPfx << "." << MathOpNames[inst.asMathInst()->getMathCtrl()];
    }

    oupPfx << ' ';
    emitExecSize(oupPfx, inst);

    G4_CondMod *mod = inst.getCondMod();
    if (mod) {
        oupPfx << ' ';
        mod->emit(oupPfx);
    }

    std::string pfx = oupPfx.str();
    output << pfx;
    for (int i = 0; i < INST_START_COLUMN_WIDTH - (int)pfx.size(); i++)
        output << ' ';
}


void G4_INST::emit_inst(std::ostream& output, bool symbol_dst, bool *symbol_srcs)
{
    if (isLabel())
    {
        srcs[0]->emit(output);
        output << ":";
        if (((G4_Label*)srcs[0])->isStartLoopLabel())
            output << " // do";
    }
    else
    {
        // predication, opcode, execsize, condition, ...
        emitInstructionStartColumn(output, *this);

        if (isSpillIntrinsic())
        {
            output << ' ';
            output << "Scratch[" << asSpillIntrinsic()->getOffset() << "x" << getBuilder().numEltPerGRF<Type_UB>() << "]";
        }
        else if (dst)
        {
            output << ' ';
            if (sat)
                output << "(sat)";
            dst->emit(output, symbol_dst);
        } // else: may not have dst (e.g. branch)

        auto numSrcOpnds = getNumSrc();
        for (int i = 0; i < numSrcOpnds; i++)
        {
            if (getSrc(i))
            {
                output << "  ";
                if (symbol_srcs != NULL)
                {
                    getSrc(i)->emit(output, symbol_srcs[i]);  // emit symbolic/physical register depends on the flag
                }
                else
                {
                    getSrc(i)->emit(output, false);   // emit physical register
                }
            }
        }

        if (isFillIntrinsic())
        {
            output << "  ";
            output << "Scratch[" << asFillIntrinsic()->getOffset() << "x" << getBuilder().numEltPerGRF<Type_UB>() << "] ";
        }

        if (isFlowControl() && asCFInst()->getJip())
        {
            output << "  ";
            asCFInst()->getJip()->emit(output);
        }

        if (isFlowControl() && asCFInst()->getUip())
        {
            output << "  ";
            asCFInst()->getUip()->emit(output);
        }

        emit_options(output);
        if (getCISAOff() != -1) {
            output << " // ";
            emitInstIds(output);
        }
    } // end: non-label
} // G4_INST::emit_inst


void G4_INST::emitInstIds(std::ostream& output) const
{
    int srcLine = getLineNo();
    if (srcLine != 0) {
        output << "#" << srcLine << ":";
    }

    int vISAId = getCISAOff();
    if (vISAId != -1) {
        output << "$" << vISAId << ":";
    }

    uint32_t genId = getLexicalId();
    if (genId != -1) {
        output << "&" << genId << ":";
    }

    if (builder.hasSWSB())
    {
        unsigned tokenLocNum = getTokenLocationNum();
        for (unsigned i = 0; i < tokenLocNum; i++)
        {
            unsigned short token = 0;
            uint32_t depId = getTokenLoc(i, token);
            output << token << "." << depId << ":";
        }
    }

    int64_t pc = getGenOffset();
    if (pc != -1) {
        output << "[" << fmtHexBody(pc, 5) << "]";
    }
}


//
// Here we add a parameter symbolreg instead of use global option Options::symbolReg,
// because we should ouput non-symbolic register when dumping dot files
//
void G4_INST::emit(std::ostream& output, bool symbolreg, bool dotStyle)
{
    bool dst_valid = true;
    bool srcs_valid[G4_MAX_SRCS];

    if (symbolreg)
    {
        if (op==G4_nop || isLabel())
        {
            emit_inst(output, false, NULL);
            return;
        }

        //
        // Emit as comment if there is invalid operand, then emit instruction
        // based on the situation of operand
        //
        if (!isValidSymbolOperand(dst_valid, srcs_valid))
        {
            if (!dotStyle)
            {
                output << "//";
                bool srcs_valid1[G4_MAX_SRCS];
                for (unsigned i = 0; i < G4_MAX_SRCS; i++)
                    srcs_valid1[i] = true;
                emit_inst(output, true, srcs_valid1); // emit comments
                output << std::endl;
            }
        }
        emit_inst(output, dst_valid, srcs_valid); // emit instruction
    }
    else
        emit_inst(output, false, NULL); // emit instruction with physical register
}

std::ostream& operator<<(std::ostream& os, G4_INST& inst)
{
    inst.emit(os, false, false);
    return os;
}

// add instruction options; only wrap in braces {...}
// if there's at least one option
// instructions are assumed Align1 and only Align16 will be explicitly stated
void G4_INST::emit_options(std::ostream& output) const
{
    std::stringstream opts;
    bool first = true;
    auto emitOption = [&](const std::string &str) {
        if (first) {
            first = false;
        } else {
            opts << ",";
        }
        opts << str;
    };


    ////////////////////////////////////////////////////////////
    // SWSB options
    if (getDistance() != 0) {
        std::stringstream dists;
        switch (getDistanceTypeXe()) {
        case DistanceType::DIST:                    break;
        case DistanceType::DISTALL:   dists << 'A'; break;
        case DistanceType::DISTINT:   dists << 'I'; break;
        case DistanceType::DISTFLOAT: dists << 'F'; break;
        case DistanceType::DISTLONG:  dists << 'L'; break;
        case DistanceType::DISTMATH:  dists << 'M'; break;
        default:                      dists << "?"; break;
        }
        dists << '@' << (int)getDistance();
        emitOption(dists.str());
    }

    std::stringstream tks;
    std::string tks1;
    auto id = getToken();
    SWSBTokenType tkType = getTokenType();
    switch (tkType) {
    case TOKEN_NONE:
    case SB_SET:      break;
    case NoACCSBSet:  tks1 = "NoACC"; break;
    case AFTER_READ:  tks1 = ".R"; break;
    case AFTER_WRITE: tks1 = ".W"; break;
    case READ_ALL:    tks1 = ".R*"; break;
    case WRITE_ALL:   tks1 = ".W*"; break;
    default:          tks1 = ".??"; break;
    }
    if (tkType != TOKEN_NONE)
    {
        if (tkType != NoACCSBSet)
        {
            tks << '$' << (int)id << tks1;
        }

        if (tks1.size())
        {
            tks << tks1;
        }
        emitOption(tks.str());
    }

    ////////////////////////////////////////////////
    // bitset options
    G4_InstOpts currOpts = option;
    if (isEOT()) {
        currOpts |= InstOpt_EOT;
    }

    // strip out stuff we handle elsewhere
    currOpts &= ~(InstOpt_QuarterMasks | InstOpt_WriteEnable);
    unsigned short optIdx = 0;
    while (currOpts && 0xFFFFFFFF != InstOptInfo[optIdx].optMask)
    {
        if (currOpts & InstOptInfo[optIdx].optMask)
        {
            emitOption(InstOptInfo[optIdx].optStr);
            currOpts &= ~InstOptInfo[optIdx].optMask; // clear this bit
        }
        optIdx++;
    }

    ////////////////////////////////////////////////
    // for older Align16-supporting platforms
    // absense implies Align1
    if (isAligned16Inst()) {
        emitOption("Align16");
    }

    //////////////////////////////////////////////////
    // only include braces {...} if there's something
    auto optsStr = opts.str();
    if (!optsStr.empty())
        output << " {" << optsStr << "}";
}


static const char* const operandString[] =
{
    OPND_NUM_ENUM(STRINGIFY)
};

void G4_INST::emitDefUse(std::ostream& output) const
{
    output << "Def:\n";
    for (auto iter = defInstList.begin(), iterEnd = defInstList.end(); iter != iterEnd; ++iter)
    {
        G4_INST* inst = (*iter).first;
        inst->emit(output);
        output << "\t" << operandString[(*iter).second];
        output << "\n";
    }
    output << "Use:\n";
    for (auto iter = useInstList.begin(), iterEnd = useInstList.end(); iter != iterEnd; ++iter)
    {
        G4_INST* inst = (*iter).first;
        inst->emit(output);
        output << "\t" << operandString[(*iter).second];
        output << "\n";
    }
}

bool G4_INST::isMixedMode() const
{
    if (mayExceedTwoGRF() || !getDst())
    {
        return false;
    }
    for (int i = 0; i < getNumSrc(); ++i)
    {
        G4_Operand *tOpnd = getSrc(i);
        if (!tOpnd)
        {
            continue;
        }

        G4_Type srcType = tOpnd->getType();
        G4_Type dstType = getDst()->getType();

        if ((dstType == builder.getMixModeType() || srcType == builder.getMixModeType()) &&
            dstType != srcType)
        {
            // do not consider int<->float conversion as mixed type
            if (!IS_TYPE_INT(dstType) && !IS_TYPE_INT(srcType))
            {
                return true;
            }
        }
    }

    return false;
}

void G4_InstSend::setMsgDesc(G4_SendDesc *in)
{
    assert(in && "null descriptor not expected");
#if defined(_DEBUG)
    if (in && in->getExecSize() == g4::SIMD_UNDEFINED)
    {
        DEBUG_MSG("Msg Desc has execSize undefined!\n");
    }
#endif
    msgDesc = in;
    resetRightBound((G4_Operand*)dst);
    resetRightBound(srcs[0]);
}

bool G4_InstSend::isDirectSplittableSend()
{
    unsigned short elemSize = dst->getElemSize();
    SFID funcID = msgDesc->getSFID();
    const G4_SendDescRaw *desc = getMsgDescRaw();
    if (desc == nullptr) {
        // load/store messages are unsplittable for now
        return false;
    }
    switch (funcID)
    {
    case SFID::DP_DC1:
        switch (desc->getHdcMessageType())
        {
        case DC1_A64_SCATTERED_READ:   //emask need be vertically cut.
            return false;

        case DC1_A64_UNTYPED_SURFACE_READ:  //SVM gather 4: emask can be reused if the per-channel data is larger than 1 GRF
        case DC1_UNTYPED_SURFACE_READ:   //VISA gather 4
        case DC1_TYPED_SURFACE_READ:   //Gather 4 typed
            if (elemSize * execSize > (int)getBuilder().numEltPerGRF<Type_UB>() &&
                elemSize * execSize % getBuilder().numEltPerGRF<Type_UB>() == 0)
            {
                return true;
            }
            else
            {
                return false;
            }

        default: return false;
        }
    case SFID::DP_DC2:
        switch (desc->getHdcMessageType())
        {
        case DC2_UNTYPED_SURFACE_READ:   //gather 4 scaled :  emask can be reused if the per-channel data is larger than 1 GRF
        case DC2_A64_UNTYPED_SURFACE_READ: //SVM gather 4 scaled
            if (elemSize * execSize > (int)getBuilder().numEltPerGRF<Type_UB>() &&
                elemSize * execSize % getBuilder().numEltPerGRF<Type_UB>() == 0)
            {
                return true;
            }
            else
            {
                return false;
            }

        case DC2_BYTE_SCATTERED_READ:   //scaled byte scattered read: gather_scaled, handled as block read write, nomask
            return true;

        default: return false;
        }
    case SFID::DP_DC0:
        switch (desc->getHdcMessageType())
        {
        case DC_DWORD_SCATTERED_READ:   //dword scattered read: emask need be vertically cut according to splitting
        case DC_BYTE_SCATTERED_READ:       //byte scattered read
            return false;
        case DC_ALIGNED_OWORD_BLOCK_READ: //Nomask
        case DC_OWORD_BLOCK_READ:
            return true;
        default: return false;
        }
    case SFID::SAMPLER:
        return true;
    default: return false;
    }

    return false;
}


//
// emit send instruction with symbolic/physical register operand depending on the operand check
//
void G4_InstSend::emit_send(std::ostream& output, bool symbol_dst, bool *symbol_srcs)
{
    emitInstructionStartColumn(output, *this);

    output << ' ';
    dst->emit(output, symbol_dst);

    output << ' ';
    G4_Operand* currSrc = srcs[0];
    if (currSrc->isSrcRegRegion()) {
        // only output reg var & reg off; don't output region desc and type
        currSrc->asSrcRegRegion()->emitRegVarOff(output, false);
    } else {
        currSrc->emit(output, false); //emit CurrDst
    }
    output << ' ';

    if (isSplitSend())
    {
        // emit src1
        srcs[1]->asSrcRegRegion()->emitRegVarOff(output, false);
        output << ' ';
    }

    // emit exDesc if srcs[3] is not null.
    // It should always be a0.2 unless it was constant folded
    if (isSplitSend() && srcs[3])
    {
        srcs[3]->emit(output, false);
        output << ' ';
    }
    else
    {
        if (getMsgDescRaw()) {
            std::ios::fmtflags outFlags(output.flags());
            output << fmtHex(getMsgDescRaw()->getExtendedDesc());
            output << ' ';
            output.flags(outFlags);
        }
    }

    // emit msgDesc (2 for sends and 1 for send). Last operand shown in asm.
    int msgDescId = isSplitSend() ? 2 : 1;
    srcs[msgDescId]->emit(output, false);

    emit_options(output);
}

void G4_InstSend::emit_send(std::ostream& output, bool dotStyle)
{
    emit_send(output, false, NULL);
}

void G4_InstSend::emit_send_desc(std::ostream& output)
{
    const G4_INST* sendInst = this;

    // Emit a text description of the descriptor if it is available
    G4_SendDesc* msgDesc = sendInst->getMsgDesc();
    output << " // ";
    if (getCISAOff() != -1) {
        emitInstIds(output);
        output << "; ";
    }

    auto desc = msgDesc->getDescription();
    if (!desc.empty()) {
        output << msgDesc->getDescription();
    }
    if (const auto *rawDesc = sendInst->getMsgDescRaw()) {
    }

    output << ", resLen=" << msgDesc->getDstLenRegs();
    output << ", msgLen=" << msgDesc->getSrc0LenRegs();
    if (isSplitSend())
    {
        output << ", extMsgLen=" << msgDesc->getSrc1LenRegs();
    }

    if (msgDesc->isBarrier())
    {
        output << ", barrier";
    }
}


// print r#
void G4_Greg::emit(std::ostream& output, bool symbolreg)
{
    output << "r" << getRegNum();
}

void G4_Areg::emit(std::ostream& output, bool symbolreg)
{
    switch (getArchRegType())
    {
    case AREG_NULL:    output << "null";  break;
    case AREG_A0:      output << "a0";    break;
    case AREG_ACC0:    output << "acc0";  break;
    case AREG_ACC1:    output << "acc1";  break;
    case AREG_MASK0:   output << "ce0";   break;
    case AREG_MS0:     output << "ms0";   break;
    case AREG_DBG:     output << "dbg0";  break;
    case AREG_SR0:     output << "sr0";   break;
    case AREG_CR0:     output << "cr0";   break;
    case AREG_TM0:     output << "tm0";   break;
    case AREG_N0:      output << "n0";    break;
    case AREG_N1:      output << "n1";    break;
    case AREG_IP:      output << "ip";    break;
    case AREG_F0:      output << "f0";    break;
    case AREG_F1:      output << "f1";    break;
    case AREG_TDR0:    output << "tdr0";  break;
    case AREG_SP:      output << "sp";    break;
    case AREG_F2:      output << "f2";    break;
    case AREG_F3:      output << "f3";    break;
    default:
        output << "unknown architecture reg";
        MUST_BE_TRUE(false, ERROR_UNKNOWN);
    }
}

//
// initial all values idential to rgn's
//
G4_SrcRegRegion::G4_SrcRegRegion(G4_SrcRegRegion &rgn)
    : G4_Operand(G4_Operand::srcRegRegion), acc(rgn.acc), regOff(rgn.regOff), subRegOff(rgn.subRegOff)
{
    base = rgn.base;
    mod = rgn.mod;
    immAddrOff = rgn.immAddrOff;
    desc = rgn.desc;
    type = rgn.type;
    // copy swizzle value
    char *sw1 = swizzle, *sw2 = rgn.swizzle;
    while (*sw2) *sw1++ = *sw2++;
    *sw1 = *sw2;
    accRegSel = rgn.accRegSel;

    // FIXME: it's rather suspicious that we are copying internal fields this way
    bitVec[0] = rgn.bitVec[0];
    bitVec[1] = rgn.bitVec[1];

    top_dcl = rgn.top_dcl;
    left_bound = rgn.left_bound;
    right_bound = rgn.right_bound;
    byteOffset = rgn.byteOffset;
    rightBoundSet = rgn.rightBoundSet;
}

//
// return true if rng and this have the same reg region
//
bool G4_SrcRegRegion::sameSrcRegRegion(G4_SrcRegRegion& rgn)
{
    return base == rgn.base &&
           acc == rgn.acc &&
           mod == rgn.mod &&
           strcmp(swizzle,rgn.swizzle) == 0 &&
           desc == rgn.desc &&
           regOff == rgn.regOff &&
           subRegOff == rgn.subRegOff &&
           immAddrOff == rgn.immAddrOff &&
           type == rgn.type &&
           accRegSel == rgn.accRegSel;
}

// compute max execution size starting from the current pos.
// power of two. cross-GRF boundary is allowed if the region is evenly split.
// cross half-GRF should guaranttee evenly split
uint8_t G4_SrcRegRegion::getMaxExecSize(const IR_Builder& builder, int pos, uint8_t maxExSize, bool allowCrossGRF, uint16_t &vs, uint16_t &wd, bool &twoGRFsrc)
{
    if (isRightBoundSet() == false)
    {
        getInst()->computeRightBound(this);
    }

    twoGRFsrc = false;
    vs = 0;
    wd = 0;
    if (isScalar())
    {
        vs = 0;
        wd = 1;
        return maxExSize;
    }
    else if (acc != Direct)
    {
        // assume this operand is kosher (i.e., does not cross GRF) as the vISA spec requires it
        vs = desc->vertStride;
        wd = desc->width;
        return roundDownPow2(maxExSize);
    }

    // align16 operands
    if (desc->isRegionV())
    {
        vs = desc->vertStride;
        wd = desc->width;
        if (desc->horzStride == 0)
        {
            return roundDownPow2(maxExSize);
        }

        uint32_t elSize = getTypeSize();
        uint8_t maxSize = 0;

        uint32_t prevPos = pos * elSize;
        uint8_t numEleInFristGRF = 0, numEleInSecondGRF = 0;
        uint32_t newLB = getLeftBound() + prevPos;
        bool crossGRF = (newLB / builder.numEltPerGRF<Type_UB>() != getRightBound() / builder.numEltPerGRF<Type_UB>()),
            inFirstGRF = true;

        for (int i = pos + 4; i < (pos + maxExSize); i += 4)
        {
            uint32_t currPos = i * elSize;

            // check cross GRF boundary
            if (crossGRF && inFirstGRF)
            {
                uint32_t newRB = getLeftBound() + currPos - 1;
                uint32_t leftGRF = newLB / builder.numEltPerGRF<Type_UB>(), rightGRF = newRB / builder.numEltPerGRF<Type_UB>();
                if (leftGRF != rightGRF)
                {
                    inFirstGRF = false;
                    numEleInFristGRF = maxSize;
                    newLB = newRB;
                }
            }

            maxSize += 4;

            if (numEleInFristGRF)
            {
                numEleInSecondGRF += 4;
                if (numEleInSecondGRF == numEleInFristGRF)
                {
                    twoGRFsrc = true;
                    break;
                }
            }
        }
        if (numEleInSecondGRF < numEleInFristGRF)
        {
            twoGRFsrc = false;
            maxSize = numEleInFristGRF;
        }
        return maxSize;
    }

    // align1 direct
    uint32_t elSize = TypeSize(type);
    uint8_t maxSize = 1;

    bool alignToRow = pos % desc->width == 0;

    // region may not be contiguous/single stride depending on the start position
    bool contRegion = desc->isContiguous(maxExSize + (pos % desc->width));

    uint16_t vStride = 1;
    if (contRegion || desc->isSingleNonUnitStride(maxExSize + (pos % desc->width), vStride))
    {
        // apparently the old code actually allows GRF-crossing as long as it's evenly divided
        // (the function comment lied), so we have to try all exec sizes from the largest possible. sigh..
        vs = vStride;
        wd = 1;
        // we need to be careful with start byte here since maxExSize may not be same as inst exec size
        // e.g., say this is called on
        // mov (16) V44_m(2,0)<1>:f V43_in(1,19)<16;8,1>:ub
        // with pos 8 and maxExSize 8
        // the region is considered single stride in this case, but is not with the original exsize (16),
        // so we can't just multiply stride with type size to get starting offset
        uint32_t startByte = (getLeftBound() + getByteOffset(pos)) % builder.numEltPerGRF<Type_UB>();
        int retExecSize = 1;
        int execTypeSize = vStride * getElemSize();
        int exSizes[] = { 32, 16, 8, 4, 2 };

        for (auto size : exSizes)
        {
            if (maxExSize < size)
            {
                continue;
            }
            if (startByte + (size - 1) * execTypeSize + getElemSize() <= builder.numEltPerGRF<Type_UB>())
            {
                // no GRF crossing (we don't count the padding bytes after the last element)
                retExecSize = size;
                break;
            }
            else if (allowCrossGRF)
            {
                int numEltInFirstGRF = (builder.numEltPerGRF<Type_UB>() - startByte) / execTypeSize;
                // startByte may not be aligned to exec type size (e.g., r1.1<2;1,0>:b).  We need to increment by 1 in this case
                if ((builder.numEltPerGRF<Type_UB>() - startByte) % execTypeSize != 0)
                {
                    numEltInFirstGRF += 1;
                }
                if (numEltInFirstGRF == size - numEltInFirstGRF)
                {
                    twoGRFsrc = true;
                    retExecSize = size;
                    break;
                }
            }
        }

        return (uint8_t)retExecSize;
    }

    // conservative.
    // Here we assume that no cross width if row size is larger than width
    // mul (16) V112(0,0)<1>:f V111(0,0)<16;16,1>:f r1.0<1;4,0>:f
    if (!alignToRow && !contRegion && desc->vertStride != 0 && desc->horzStride != 0)
    {
        wd = vs = (uint16_t)roundDownPow2((pos/desc->width + 1) * desc->width - pos);

        // Need to check whether this subregion crosses grf or not.
        // E.g. the second half does cross a grf:
        // mov (8) V41(0, 9)<1> V58(2, 8)<32;8,4>
        //
        // Given a linearized index, compute its byte offset relative to the
        // first element (index 0).
        auto computeOffset = [=](unsigned index) -> unsigned {
            unsigned typeSize = TypeSize(type);
            unsigned offset = (index % desc->width) * desc->horzStride * typeSize;
            offset += (index / desc->width) * desc->vertStride * typeSize;
            return offset;
        };

        // Since a single element cannot cross a grf, checking the first byte of the
        // first and last element is sufficient.
        // FIXME: fix other places with this logic.
        unsigned firstPos = getLeftBound() + computeOffset((unsigned)pos);
        unsigned lastPos = getLeftBound() + computeOffset((unsigned)(pos + wd - 1));
        twoGRFsrc = firstPos / builder.numEltPerGRF<Type_UB>() != lastPos / builder.numEltPerGRF<Type_UB>();

        return (uint8_t)wd;
    }

    uint8_t posInFirstRow = pos%desc->width, eleInRow = 1, eleInFirstRow = desc->width - posInFirstRow;
    uint8_t pow2 = roundDownPow2(eleInFirstRow);

    if (eleInFirstRow != pow2 && !contRegion)
    {
        wd = pow2;
        vs = wd * desc->horzStride;
        return pow2;
    }

    uint32_t prevPos = (pos/desc->width * desc->vertStride + posInFirstRow * desc->horzStride) * elSize;
    uint8_t numEleInFristGRF = 0, numEleInSecondGRF = 0;
    bool crossRow = false;
    uint32_t newLB = getLeftBound() + prevPos;
    bool crossGRF = (newLB / builder.numEltPerGRF<Type_UB>() != getRightBound() / builder.numEltPerGRF<Type_UB>()),
        inFirstGRF = true;
    bool negVS = (desc->vertStride < desc->horzStride * desc->width);

    for (int i = pos + 1; i < (pos + maxExSize); i++)
    {
        uint8_t posInRow = i % desc->width;
        uint32_t currPos = ((i / desc->width) * desc->vertStride + posInRow * desc->horzStride) * elSize;

        // check cross row boundary
        if ((!contRegion || desc->vertStride == 0) && posInRow == 0)
        {
            uint8_t pow2Val = roundDownPow2(eleInRow);
            if (pow2Val != eleInRow  ||
                ((desc->vertStride == 0 || negVS) && !alignToRow))
            {
                // this happens in the first row
                wd = maxSize = pow2Val;
                vs = wd * desc->horzStride;
                break;
            }
            else if (wd == 0)
            {
                // <2;4,1>
                wd = eleInRow;
                if (alignToRow)
                {
                    vs= desc->vertStride;
                }
                else
                {
                    vs = (currPos - prevPos) / elSize;
                }
            }
            crossRow = true;
            eleInRow = 0;
        }

        // check cross GRF boundary
        if (crossGRF && inFirstGRF)
        {
            uint32_t newRB = getLeftBound() + currPos + elSize - 1;
            uint32_t leftGRF = newLB / builder.numEltPerGRF<Type_UB>(), rightGRF = newRB / builder.numEltPerGRF<Type_UB>();
            if (leftGRF != rightGRF)
            {
                inFirstGRF = false;
                uint8_t pow2Val = roundDownPow2(maxSize);

                // if number of element in first GRF is not power of 2, or
                // subregister offset of two GRFs are different and not contiguous(too conservative?)
                if (pow2Val != maxSize ||
                    (!contRegion && !(alignToRow && maxSize <= desc->width) && newLB % builder.numEltPerGRF<Type_UB>() != (getLeftBound() + currPos) % builder.numEltPerGRF<Type_UB>()))
                {
                    maxSize = pow2Val;
                    if (wd == 0)
                    {
                        wd = pow2Val;
                        vs = wd * desc->horzStride;
                    }
                    break;
                }
                else if (wd == 0)
                {
                    wd = maxSize < desc->width ? maxSize : desc->width;
                    vs = (currPos - prevPos) / elSize;
                }
                numEleInFristGRF = maxSize;
                newLB = newRB;
            }
        }

        maxSize++;
        eleInRow++;
        // make sure the number of elements in two rows are the same
        if (crossRow && eleInRow == eleInFirstRow && !alignToRow && !contRegion)
        {
            break;
        }

        if (numEleInFristGRF)
        {
            numEleInSecondGRF++;
            if (numEleInSecondGRF == numEleInFristGRF)
            {
                twoGRFsrc = true;
                break;
            }
        }
    }
    if (wd == 0)
    {
        // contiguous region
        wd = pow2;
        vs = wd * desc->horzStride;
    }
    if (numEleInSecondGRF < numEleInFristGRF)
    {
        maxSize = numEleInFristGRF;
    }
    return maxSize;
}

//
// output (Var+refOff).subRegOff
//
void printRegVarOff(std::ostream&  output,
                    G4_Operand*    opnd,
                    short          regOff,  // base+regOff is the starting register
                    short          subRegOff, // sub reg offset
                    short          immAddrOff, // imm addr offset
                    G4_Type        type,
                    bool symbolreg,
                    bool printSubReg)
//
// symbolreg == false,  output physcial register operand
// symbolreg == true,   output symbolic register operand
// Here we use symbolreg instead of Options::symbolReg, because sometimes we need to emit an instruction with invalid symbolic register as comment and
// then emit a workable instruction with physical register. In this case, we do not want to re-set the global option variable Options::symbolReg to switch
// between these two states, that may have potential side effects.
//
{
    short subRegOffset = (subRegOff != (short) UNDEFINED_SHORT) ? subRegOff : 0;

    G4_RegAccess acc = opnd->getRegAccess();
    G4_VarBase* base = opnd->getBase();
    if (acc == Direct)
    {
        MUST_BE_TRUE(regOff != (short) UNDEFINED_SHORT,
            ERROR_INTERNAL_ARGUMENT);

        if (base->isRegVar())
        {
            G4_RegVar* baseVar = static_cast<G4_RegVar*>(base);
            int declOpSize = baseVar->getDeclare()->getElemSize();
            uint16_t thisOpSize = TypeSize(type);

            if (baseVar->isPhyRegAssigned())
            {

                if (symbolreg && !base->isFlag())
                {
                    //
                    // No matter the type of register and if the allocation successed, we output format <symbol>(RegOff, SubRegOff)
                    // Note: we have check if the register allocation  successed when emit the declare!
                    //
                    output << base->asRegVar()->getName() << "(" << regOff << "," << subRegOff << ")";
                    return;
                }

                if (baseVar->getPhyReg()->isGreg())
                {
                    int regNum = 0, subRegNum = 0;
                    uint32_t byteAddress = opnd->getLinearizedStart();
                    const IR_Builder& builder = baseVar->getDeclare()->getBuilder();

                    if (baseVar->getDeclare()->getGRFBaseOffset() == 0)
                    {
                        // This is before RA and getLineariedStart() only contains the left bound
                        // we have to add the declare's phyreg
                        byteAddress += baseVar->getPhyReg()->asGreg()->getRegNum() * builder.getGRFSize() + baseVar->getPhyRegOff() * TypeSize(type);
                    }

                    regNum = byteAddress / builder.getGRFSize();
                    subRegNum = (byteAddress % builder.getGRFSize()) / TypeSize(type);


                     output << "r" << regNum;
                     if (printSubReg)
                     {
                         output << "." << subRegNum;
                     }
                }
                else if (baseVar->getPhyReg()->isAreg())
                {
                    (static_cast<G4_Areg*>(baseVar->getPhyReg()))->emit(output);
                    if (!baseVar->isNullReg())
                    {
                        unsigned ArfSubRegNum = baseVar->getPhyRegOff();

                        //ArfSubRegNum is in unit of declOpSize
                        //transform ArfSubRegNum to unit of thisOpSize
                        if (thisOpSize != declOpSize)
                        {
                            if (!opnd->getInst()->isPseudoKill())
                            {
                                MUST_BE_TRUE((ArfSubRegNum * declOpSize) % thisOpSize == 0,
                                    ERROR_DATA_RANGE("ARF sub-register number"));
                            }
                            ArfSubRegNum = (ArfSubRegNum * declOpSize) / thisOpSize;
                        }

                        unsigned subreg = ArfSubRegNum + subRegOffset;
                        output << '.' << subreg;
                    }
                }
                else
                    MUST_BE_TRUE(false, ERROR_UNKNOWN);
            }
            else        // physical register not allocated
            {
                baseVar->emit(output);
                output << '(' << regOff << ',' << subRegOff << ')';
            }
        }
        else //This is not a RegVar
        {
            if (base->isAccReg() && regOff != 0)
            {
                bool valid;
                int regNum = base->ExRegNum(valid);
                output << "acc" << regNum + regOff;
            }
            else
            {
                base->emit(output);
            }
            if (!base->isNullReg() && !base->isIpReg() && !base->isNReg() && subRegOff != (short) UNDEFINED_SHORT && printSubReg)
            {
                output << '.' << subRegOff;
            }
        }
    }
    else //This is an indirect access
    {
        if (acc == IndirGRF)
        {
            output << "r[";
        }
        else //Unknown access type
        {
            MUST_BE_TRUE(false, ERROR_UNKNOWN);
        }

        if (base->isRegVar())
        {
            MUST_BE_TRUE(regOff == 0, ERROR_INTERNAL_ARGUMENT);
            G4_RegVar* baseVar = static_cast<G4_RegVar*>(base);
            if (baseVar->isPhyRegAssigned())
            {
                MUST_BE_TRUE(baseVar->getPhyReg()->isAreg(), ERROR_UNKNOWN);

                if (symbolreg)
                {
                    output << baseVar->getName();
                    output << '(' << regOff << ',' << subRegOffset << ")," << immAddrOff << ']';
                }
                else
                {
                    (static_cast<G4_Areg*>(baseVar->getPhyReg()))->emit(output);
                    output << '.' << (baseVar->getPhyRegOff() + subRegOffset);
                    {
                        output << ", " << immAddrOff << ']';
                    }
                }
            }
            else //No register assigned yet
            {
                baseVar->emit(output);
                output << '(' << regOff << ',' << subRegOff << ')';
                output << ", " << immAddrOff << ']';
            }
        }
        else if (base->isAreg())
        {
            (static_cast<G4_Areg*>(base))->emit(output);
            output << '.' << subRegOffset;
            {
                output << ", " << immAddrOff << ']';
            }
        }
        else
        {
            MUST_BE_TRUE(false, "Unknown base variable type for indirect access");
        }
    }
}

//
// output <modifier>(Var+refOff).subRegOff<16;16,1>.xxyy
//
// symbolreg == false,  output <modifier>(Var+refOff).subRegOff<16;16,1>.xxyy
// symbolreg == true,   output <modifier><symbol>(RegOff, SubRegOff)<16;16,1> in symbolic register emit
// Here we use symbolreg instead of Options::symbolReg, because sometimes we need to emit an instruction with invalid symbolic register as comment and
// then emit a workable instruction with physical register. In this case, we do not want to re-set the global option variable Options::symbolReg to switch
// between these two states, that may have potential side effects.
//
void G4_SrcRegRegion::emit(std::ostream& output, bool symbolreg)
{
    if (mod != Mod_src_undef)
    {
        output << SrcModifierStr[mod];
    }

    //
    // output Var(refOff,subRegOff)
    //
    emitRegVarOff(output, symbolreg);
    //
    // output <vertStride;width,horzStride>
    //
    // do not emit region for null reg
    // do not emit region for macro madm
    if (desc && !base->isNullReg() && !base->isNReg() && !isAccRegValid())// rgn == NULL, the default region is used
    {
        bool align1ternary = inst && inst->getNumSrc() == 3 && inst->getPlatform() >= GENX_ICLLP &&
            !inst->isSend() && inst->isAligned1Inst();

        // RegionV is invalid for SRC operands
        if (desc->isRegionWH())
        {
            output << "<" << desc->width << "," << desc->horzStride << ">";
        }
        else if (desc->isRegionSW()) // support <0/4> for Src of Align16 instruction
        {
            output << "<" << desc->vertStride << ">";
        }
        else if (desc->vertStride == UNDEFINED_SHORT && desc->width == UNDEFINED_SHORT)
        {
            output << "<" << desc->horzStride << ">";
        }
        else
        {
            if (align1ternary)
            {
                // format is <V;H> with W derived from V and H
                output << "<" << desc->vertStride << ";" << desc->horzStride << ">";
            }
            else if (!isWithSwizzle())
            {
                // do not print region for align16 sources
                output << "<" << desc->vertStride << ";" << desc->width << "," << desc->horzStride << ">";
            }
        }
    }

    if (isAccRegValid())
    {
        // no vertical stride for 3-source instruction
        if (inst->getNumSrc() != 3 && desc)
        {
            output << "<" << desc->vertStride << ">";
        }

        // output acc2~acc9
        if (getAccRegSel() == NOACC)
        {
            output << ".noacc";
        }
        else
        {
            output <<".acc"<< (getAccRegSel()+2);
        }
    }
    else if (*swizzle)
    {
        output << "." << swizzle;
    }

    if (Type_UNDEF != type)
    {
        if (!symbolreg || acc != Direct)                // can output register data type for indirect addressing in any time
            output << ':' << TypeSymbol(type);
    }
}

//

// return true if this src is a scalar

// V82(1,0)<0>.xxxx:f

// V82(1,0)<0;1,0>:f  --- detect via

//

bool G4_SrcRegRegion::isScalar() const

{

    if (!isWithSwizzle())
    {

        return getRegion()->isScalar(); // check <0;1,0>
    }
    else
    {
        return swizzle[0] == 'r';
    }

}


//
// This function is used to check if the src operand obey the rule of symbolic register. We need this function to check the operand before we emit an instruction
//
bool G4_SrcRegRegion::obeySymbolRegRule() const
{
    if (!base->isRegVar())          // only for reg var
        return false;

    if (base->asRegVar()->getDeclare()->isSpilled())
    {
        return false;
    }

    //
    // Rule-3: No swizzle .xyzw
    //
    if (*swizzle)
    {
        return false;
    }
    //
    // Rule-4: do not support date type redefinition in direct addressing
    //
    if (Type_UNDEF != type)
    {
         if (base->isRegVar() && acc == Direct && base->asRegVar()->getDeclare()->getElemType() != type)        // check if the data type is the same as in declare
         {
             return false;
         }
    }

    return true;
}

//
// Here we use symbolreg instead of Options::symbolReg, because sometimes we need to emit an instruction with invalid symbolic register as comment and
// then emit a workable instruction with physical register. In this case, we do not want to re-set the global option variable Options::symbolReg to switch
// between these two states, that may have potential side effects.
//
void G4_SrcRegRegion::emitRegVarOff(std::ostream& output, bool symbolreg)
{
    bool printSubReg = true;
    if (inst && inst->isSend())
    {
        printSubReg = false;
    }
    printRegVarOff(output, this, regOff,subRegOff,immAddrOff,type, symbolreg, printSubReg);
}

//
// initial all values idential to rgn's
//
G4_DstRegRegion::G4_DstRegRegion(G4_DstRegRegion &rgn)
    : G4_Operand(G4_Operand::dstRegRegion)
{
    acc = rgn.acc;
    base = rgn.base;
    regOff = rgn.regOff;
    subRegOff = rgn.subRegOff;
    immAddrOff = rgn.immAddrOff;
    horzStride = rgn.horzStride;
    type = rgn.type;
    writeMask = rgn.writeMask;
    accRegSel = rgn.accRegSel;

    top_dcl = rgn.top_dcl;
    left_bound = rgn.left_bound;
    right_bound = rgn.right_bound;
    bitVec[0] = rgn.bitVec[0];
    bitVec[1] = rgn.bitVec[1];
    byteOffset = rgn.byteOffset;
    rightBoundSet = rgn.rightBoundSet;
}

void G4_DstRegRegion::computeLeftBound(const IR_Builder& builder)
{
    top_dcl = NULL;
    uint32_t newregoff = regOff, offset = 0;
    if (base && base->isRegVar())
    {
        top_dcl = base->asRegVar()->getDeclare();
        if (!top_dcl && base->asRegVar()->isGreg())
        {
            newregoff = base->asRegVar()->asGreg()->getRegNum();
        }
    }

    if (top_dcl)
    {
        while (top_dcl->getAliasDeclare())
        {
            offset += top_dcl->getAliasOffset();
            top_dcl = top_dcl->getAliasDeclare();
        }
    }

    if (base && base->isFlag())
    {
        if (base->isRegVar())
        {
            if (base->asRegVar()->getPhyReg())
            {
                left_bound = base->asRegVar()->getPhyRegOff() * 16;   // the bound of flag register is in unit of BIT
                left_bound += subRegOff * 16;
                left_bound += base->asRegVar()->getPhyReg()->asAreg()->getFlagNum() * 32;
            }
            else
            {
                left_bound = subRegOff * 16;
            }
        }
        else
        {
            left_bound = subRegOff * 16;
            left_bound += base->asAreg()->getFlagNum() * 32;
        }

        byteOffset = left_bound / 8;
    }
    else if (base != NULL && base->isAccReg())
    {
        left_bound = subRegOff * TypeSize(type);
        if (base->asAreg()->getArchRegType() == AREG_ACC1 || regOff == 1)
        {
            left_bound += builder.getGRFSize();
        }
        byteOffset = left_bound;
    } else if (top_dcl) {
        if (acc == Direct) {
            left_bound = offset + newregoff * builder.numEltPerGRF<Type_UB>() + subRegOff * TypeSize(type);
            if (top_dcl->getTotalElems() * top_dcl->getElemSize() >= (int)builder.numEltPerGRF<Type_UB>()) {
                byteOffset = left_bound;
            }
            else {
                unsigned alignOff = TypeSize(type) > TypeSize(Type_W) ?
                    TypeSize(type) : TypeSize(Type_W);

                if (top_dcl->getSubRegAlign() == Even_Word || top_dcl->getSubRegAlign() >= Four_Word) {
                    alignOff = top_dcl->getSubRegAlign() * 2;
                }

                byteOffset = left_bound + alignOff;
            }
        }
        else {
            left_bound = subRegOff * TypeSize(ADDR_REG_TYPE);
            byteOffset = TypeSize(type);
        }
    } else { // arch reg
        left_bound = 0;
        byteOffset = left_bound;
    }
}

//
// Initialize all values idential to rgn's, except for the base operand.
// Caller is responsible for allocating base operand and making sure it doesn't
// mess up the operands' hash table.
//
G4_DstRegRegion::G4_DstRegRegion(const IR_Builder& builder, G4_DstRegRegion &rgn, G4_VarBase *new_base)
    : G4_Operand(G4_Operand::dstRegRegion)
{
    acc = rgn.acc;
    regOff = rgn.regOff;
    subRegOff = rgn.subRegOff;
    immAddrOff = rgn.immAddrOff;
    horzStride = rgn.horzStride;
    type = rgn.type;
    writeMask = rgn.writeMask;
    base = new_base;

    computeLeftBound(builder);
    rightBoundSet = false;
}

bool G4_DstRegRegion::isCrossGRFDst(const IR_Builder& builder)
{
    if (isNullReg())
    {
        return inst != NULL &&
            (unsigned)inst->getExecSize() * getTypeSize() * horzStride > builder.numEltPerGRF<Type_UB>();
    }
    if (isRightBoundSet() == false)
    {
        // computeRightBound populates crossGRFDst field
        getInst()->computeRightBound(this);
    }

    return (left_bound / builder.numEltPerGRF<Type_UB>()) != right_bound / builder.numEltPerGRF<Type_UB>();
}

void G4_DstRegRegion::setDstBitVec(uint8_t exec_size)
{
    // byte level footprint computing bit vectors.
    uint64_t footprint0 = 0;
    uint64_t footprint1 = 0;

    unsigned short type_size = getTypeSize();
    unsigned short s_size = horzStride * type_size;

    // General cases.
    uint64_t bit_seq = TypeFootprint(type);
    for (uint8_t i = 0; i < exec_size; ++i)
    {
        int eltOffset = i * s_size;
        // no element can cross 64-byte boundary
        if (eltOffset >= 64)
        {
            footprint1 |= bit_seq << (eltOffset - 64);
        }
        else
        {
            footprint0 |= bit_seq << eltOffset;
        }
    }

    bitVec[0] = footprint0;
    bitVec[1] = footprint1;

    return;
}

unsigned G4_DstRegRegion::computeRightBound(uint8_t exec_size)
{
    bitVec[0] = 0;
    bitVec[1] = 0;

    if (base->isFlag()) {
        unsigned int totalBits = 0;
        if (G4_Inst_Table[inst->opcode()].instType != InstTypePseudoLogic)
        {
            // mov (1) f0.1<1>:uw ...
            // subreg is 1 if it's a 32 bit flag and we want to set the upper 16 bits
            left_bound = subRegOff * 16;
            totalBits = TypeBitSize(type);
        }
        else
        {
            /*
                we need to set leftBound for pseudo intruction
                so that it creates use/def links correctly in the control flow graph between
                cmp instruction and pseudo instruction.
                This matters when we break up SIMD32 instruction in to two SIMD16 with H1/H2 masks.
                The bound for compare for H2 will be [15,31], and this has to match.
                Without this no use/def link was created which caused issues in logic optimization.
                Also it produce incorrect behavior in any operation that relies on compareOperand.
            */
            left_bound = inst->getMaskOffset();
            totalBits = exec_size;
        }

        right_bound = left_bound + totalBits - 1;

        bitVec[0] = totalBits == 32 ? 0xFFFFFFFF : (1 << totalBits) - 1;
    }
    else
    {
        // For call, the return addr is always set as if simd2.
        if (inst->isCall() || inst->isFCall())
        {
            exec_size = 2;
        }

        if (acc == Direct)
        {
            setDstBitVec(exec_size);

            unsigned short type_size = TypeSize(type);
            unsigned short s_size = horzStride * type_size;
            unsigned totalBytes = (exec_size - 1) * s_size + type_size;

            // For wide dst instructions like madw opcode, the dst(SOA layout) size should be the sum of low result size and high
            // result size, and also both low and high results are GRF-aligned.
            if (INST_WIDE_DST(inst->opcode()))
            {
                const IR_Builder& builder = inst->getBuilder();
                unsigned totalBytesDstLow = (totalBytes + builder.getGRFSize() - 1) & (~(builder.getGRFSize() - 1)); // GRF-aligned
                totalBytes = totalBytesDstLow * 2;
            }

            right_bound = left_bound + totalBytes - 1;
        }
        else
        {
            // indirect
            bitVec[0] |= 0x3;
            right_bound = left_bound + TypeSize(ADDR_REG_TYPE) - 1;
        }
    }
    rightBoundSet = true;
    return right_bound;
}

/// compare regRegion to opnd
/// regRegion is either a SrcRegRegion or DstRegRegion, opnd can be any G4_operand
/// We put this in a separate function since G4_DstRegRegion and G4_SrcRegRegion
/// should have (nearly) identical code for compareOperand
static G4_CmpRelation compareRegRegionToOperand(G4_Operand* regRegion, G4_Operand* opnd, const IR_Builder& builder)
{
    assert((regRegion->isSrcRegRegion() || regRegion->isDstRegRegion()) && "expect either src or dst regRegion");
    bool legal_opnd = opnd->isSrcRegRegion() || opnd->isDstRegRegion() || opnd->isPredicate() || opnd->isCondMod() || opnd->isAddrExp();
    G4_VarBase* myBase = regRegion->getBase();
    G4_VarBase *opndBase = opnd->getBase();
    G4_RegAccess myAcc = regRegion->getRegAccess();
    G4_RegAccess opndAcc = opnd->getRegAccess();
    G4_Declare* myDcl = regRegion->getTopDcl();
    G4_Declare* opndDcl = opnd->getTopDcl();
    if (opnd->isAddrExp())
    {
        opndBase = opnd->asAddrExp()->getRegVar()->getBaseRegVar();
        opndDcl = opnd->asAddrExp()->getRegVar()->getDeclare();
    }

    if (regRegion->isAddrExp())
    {
        myBase = opnd->asAddrExp()->getRegVar()->getBaseRegVar();
        myDcl = opnd->asAddrExp()->getRegVar()->getDeclare();
    }

    if (!legal_opnd || myBase == nullptr || opndBase == nullptr)
    {
        // a null base operand can never interfere with anything
        return Rel_disjoint;
    }

    if (myDcl == opndDcl && opndDcl != nullptr)
    {
        // special checks for pseudo kills
        G4_INST* myInst = regRegion->getInst();
        G4_INST* opndInst = opnd->getInst();
        if (myInst && (myInst->isPseudoKill() || myInst->isLifeTimeEnd()))
        {
            return Rel_interfere;
        }

        if (opndInst && (opndInst->isPseudoKill() || opndInst->isLifeTimeEnd()))
        {
            return Rel_interfere;
        }

        if (opnd->isAddrExp() || regRegion->isAddrExp())
        {
            return Rel_interfere;
        }
    }

    if (opndAcc == myAcc && myAcc != Direct)
    {
        // two indirect are assumed to interfere in the absence of pointer analysis
        return Rel_interfere;
    }
    else if (opndAcc != myAcc)
    {
        // direct v. indirect
        // the two may inteferce if the direct operand is either an address-taken GRF or an address operand
        // we could make the check tighter by considering the offsets of the address operand,
        // but it won't much difference in practice
        auto mayInterfereWithIndirect = [](G4_Operand* direct, G4_Operand* indirect)
        {
            assert((direct->getRegAccess() == Direct && indirect->getRegAccess() == IndirGRF) &&
                "first opereand should be direct and second indirect");
            return (direct->getTopDcl() && direct->getTopDcl()->getAddressed()) ||
                (direct->isAddress() && direct->getTopDcl() == indirect->getTopDcl());
        };

        if ((opndAcc != Direct && mayInterfereWithIndirect(regRegion, opnd)) ||
            (myAcc != Direct && mayInterfereWithIndirect(opnd, regRegion)))
        {
            return Rel_interfere;
        }
        return Rel_disjoint;
    }

    // both are physically assigned.
    G4_VarBase *myPhyReg = myBase->isRegVar() ? myBase->asRegVar()->getPhyReg() : myBase;
    G4_VarBase *opndPhyReg = opndBase->isRegVar() ? opndBase->asRegVar()->getPhyReg() : opndBase;
    if (myPhyReg && opndPhyReg)
    {
        assert(myPhyReg->isPhyReg() && opndPhyReg->isPhyReg());
        if (myPhyReg->getKind() != opndPhyReg->getKind())
            return Rel_disjoint;

        if (myPhyReg->isPhyAreg())
        {
            if (myPhyReg->asAreg()->getArchRegType() == AREG_NULL)
            {
                //like NaN, a null ARF is disjoint to everyone including itself
                return Rel_disjoint;
            }

            // TODO: this is not accurate for flag/acc/address.
            return (myPhyReg->asAreg()->getArchRegType() ==
                  opndPhyReg->asAreg()->getArchRegType()) ? Rel_eq : Rel_disjoint;
        }

        // TODO: handle physically assigned GRF reg. Right now this should
        // not happen prior to RA.
    }

    if (myBase->getKind() != opndBase->getKind())
    {
        return Rel_disjoint;
    }

    if (myDcl != opndDcl)
    {
        return Rel_disjoint;
    }

    unsigned int left_bound2 = opnd->getLeftBound(), right_bound2 = opnd->getRightBound();
    uint32_t myLeftBound = regRegion->getLeftBound();
    uint32_t myRightBound = regRegion->getRightBound();

    {
        uint64_t opndBitVecL = opnd->getBitVecL(), opndBitVecH = opnd->getBitVecH(builder);
        uint64_t myBitVecL = regRegion->getBitVecL(), myBitVecH = regRegion->getBitVecH(builder);
        if (myRightBound < left_bound2 || right_bound2 < myLeftBound)
        {
            return Rel_disjoint;
        }
        else if (myLeftBound == left_bound2 &&
            myRightBound == right_bound2 &&
            myBitVecL == opndBitVecL && myBitVecH == opndBitVecH)
        {
            return Rel_eq;
        }
        else
        {
            // First consider if any operand is > two GRFs. If so we just compare the bound
            // as such operands are assumed to touch every element within the bound.
            bool meExceedTwoGRF = (myRightBound - myLeftBound) > 2u * builder.getGRFSize();
            bool opndExceedTwoGRF = (right_bound2 - left_bound2) > 2u * builder.getGRFSize();
            if (meExceedTwoGRF || opndExceedTwoGRF)
            {
                if (left_bound2 >= myLeftBound && right_bound2 <= myRightBound)
                {
                    return Rel_gt;
                }
                else if (myLeftBound >= left_bound2 && myRightBound <= right_bound2)
                {
                    return Rel_lt;
                }
                return Rel_interfere;
            }

            // Now both operands are within two GRFs, compare their footprint to get precise relations
            int maskSize = 2 * builder.getGRFSize();
            if (myDcl)
            {
                maskSize = myDcl->getRegVar()->isFlag() ? myDcl->getNumberFlagElements()
                    : myDcl->getByteSize();
            }
            BitSet myBitSet(maskSize, false);
            BitSet otherBitSet(maskSize, false);
            regRegion->updateFootPrint(myBitSet, true, builder);
            opnd->updateFootPrint(otherBitSet, true, builder);

            BitSet tmp = myBitSet;
            myBitSet &= otherBitSet;
            if (myBitSet.isEmpty())
            {
                return Rel_disjoint;
            }

            myBitSet = tmp;
            myBitSet -= otherBitSet;
            if (myBitSet.isEmpty())
            {
                return Rel_lt;
            }
            otherBitSet -= tmp;
            return otherBitSet.isEmpty() ? Rel_gt : Rel_interfere;
        }
    }
}

G4_CmpRelation G4_DstRegRegion::compareOperand(G4_Operand *opnd, const IR_Builder& builder)
{
    return compareRegRegionToOperand(this, opnd, builder);
}

bool G4_DstRegRegion::isNativeType() const
{
    G4_Type type = getType();

    if (IS_WTYPE(type) || IS_DTYPE(type) || IS_FTYPE(type) || type == Type_DF) {
        return true;
    }
    else {
        return false;
    }
}

bool G4_DstRegRegion::isNativePackedRowRegion() const
{
    if (isNativeType()) {
        return horzStride  == 1;
    }
    else {
        return false;
    }
}

bool G4_DstRegRegion::isNativePackedRegion() const
{
    return isNativePackedRowRegion();
}

bool G4_DstRegRegion::coverGRF(const IR_Builder& builder, uint16_t numGRF, uint8_t execSize)
{
    uint32_t size = builder.numEltPerGRF<Type_UB>() * numGRF;
    uint32_t range = getRightBound() - getLeftBound() + 1;
    if (acc == Direct)
    {
        if (range == size)
        {
            return true;
        }
        if (horzStride > 1)
        {
            if (size == execSize * horzStride * TypeSize(type))
            {
                return true;
            }
        }
    }
    else
    {
        if (size == execSize * horzStride * TypeSize(type))
        {
            return true;
        }
    }
    return false;
}

// Check if dst satisfies the following conditions(for platforms before BDW):
//The destination region is entirely contained in the lower OWord of a register.
//The destination region is entirely contained in the upper OWord of a register.
//The destination elements are evenly split between the two OWords of a register.

bool G4_DstRegRegion::goodOneGRFDst(const IR_Builder& builder, uint8_t execSize)
{
    if (acc != Direct)
    {
        return horzStride * TypeSize(type) * execSize == builder.numEltPerGRF<Type_UB>();
    }
    uint32_t halfSize = (getRightBound() - getLeftBound() + 1 + (horzStride - 1) * getTypeSize()) / 2;
    uint32_t middle = getLeftBound() + halfSize;
    if (getLeftBound() / (builder.numEltPerGRF<Type_UB>() / 2) == getRightBound() / (builder.numEltPerGRF<Type_UB>() / 2) ||
        (getLeftBound() / (builder.numEltPerGRF<Type_UB>() / 2) == (getRightBound() / (builder.numEltPerGRF<Type_UB>() / 2) - 1) &&
        getLeftBound() % (builder.numEltPerGRF<Type_UB>() / 2) == middle % (builder.numEltPerGRF<Type_UB>() / 2)))
    {
        return true;
    }
    return false;
}

bool G4_DstRegRegion::goodtwoGRFDst(const IR_Builder& builder, uint8_t execSize)
{
    return evenlySplitCrossGRF(builder, execSize);
}

// this is true if dst crosses GRF and has same number of elements in both GRFs
// (i.e, the middle element has same GRF offset as the start element)
bool G4_DstRegRegion::evenlySplitCrossGRF(const IR_Builder& builder, uint8_t execSize)
{
    // check number of elements in first GRF.
    MUST_BE_TRUE(acc == Direct, "Indirect operand can not cross GRF boundary.");

    if (execSize == 1)
    {
        return false;
    }

    int halfBytes = left_bound + horzStride * TypeSize(type) * (execSize / 2);
    int halfOffset = halfBytes % builder.numEltPerGRF<Type_UB>();
    int startOffset = left_bound % builder.numEltPerGRF<Type_UB>();
    return halfOffset == startOffset;
}

/*
 * check if the input opnd is align to GRF
 * if the first level dcl is not aligned to GRF or sub register offset of this opnd is not multiple GRFs, including 0,
 * return true.
 */
bool G4_DstRegRegion::checkGRFAlign(const IR_Builder& builder) const
{
    bool GRF_aligned = false;
    unsigned byte_subregoff = subRegOff * TypeSize(type);

    if (byte_subregoff % builder.numEltPerGRF<Type_UB>() != 0)
    {
        return false;
    }

    if (base)
    {
        if (base->isRegVar())
        {
            G4_Declare *dcl = base->asRegVar()->getDeclare();

            if (dcl)
            {
                G4_Declare *aliasdcl = dcl;

                unsigned aliasOffset = 0;
                while (aliasdcl->getAliasDeclare())
                {
                    aliasOffset += aliasdcl->getAliasOffset();
                    aliasdcl = aliasdcl->getAliasDeclare();
                }
                if (aliasOffset % builder.numEltPerGRF<Type_UB>() != 0)
                {
                    return false;
                }

                if (aliasdcl->getSubRegAlign() >= builder.getGRFAlign() ||
                    aliasdcl->getNumRows() * aliasdcl->getElemSize() * aliasdcl->getElemSize() >= (int)builder.numEltPerGRF<Type_UB>()) {
                    return true;
                }
            }
            else if (base->asRegVar()->isPhyRegAssigned() &&
                base->asRegVar()->getByteAddr(builder) % builder.numEltPerGRF<Type_UB>() == 0)
            {
                    return true;
            }
        }
    }

    return GRF_aligned;
}

//
// returns true if this operand (must be either Src or DstRegRegion) has a fixed subreg offset.
// This is true only if
// -- operand is direct,
// -- operand has assigned GRF (i.e., input), or
// -- base declare is a GRF variable that is GRF-aligned
// if true, the subreg offset is also returned via offset in bytes
// Note this always returns false for ARFs (flag, addr, etc.)
//
static bool regionHasFixedSubreg(const IR_Builder& builder, G4_Operand* opnd, uint32_t& offset)
{
    assert(opnd->isSrcRegRegion() || opnd->isDstRegRegion());
    short subRegOff = 0;
    if (opnd->isSrcRegRegion())
    {
        if (opnd->asSrcRegRegion()->getRegAccess() != Direct)
        {
            return false;
        }
        subRegOff = opnd->asSrcRegRegion()->getSubRegOff();
    }
    else if (opnd->isDstRegRegion())
    {
        if (opnd->asDstRegRegion()->getRegAccess() != Direct)
        {
            return false;
        }
        subRegOff = opnd->asDstRegRegion()->getSubRegOff();
    }

    G4_VarBase* base = opnd->getBase();

    if (base == NULL || !base->isRegVar() || !base->asRegVar()->getDeclare()->useGRF())
    {
        return false;
    }

    if (base->asRegVar()->isPhyRegAssigned())
    {
        offset = (subRegOff + base->asRegVar()->getPhyRegOff()) * TypeSize(opnd->getType());
        offset %= builder.getGRFSize();
        return true;
    }

    uint32_t subregByte = 0;
    G4_Declare *rootDcl = base->asRegVar()->getDeclare()->getRootDeclare(subregByte);
    subregByte += subRegOff * TypeSize(opnd->getType());

    if (rootDcl->getSubRegAlign() < builder.getGRFAlign())
    {
        return false;
    }
    offset = subregByte % builder.numEltPerGRF<Type_UB>();

    return true;
}


bool G4_DstRegRegion::hasFixedSubregOffset(const IR_Builder& builder, uint32_t& offset)
{
    return regionHasFixedSubreg(builder, this, offset);
}

// compute max execution size starting from the current pos.
// power of two. no cross GRF boundary is allowed now.
// TODO: cross GRF is allowed in BDW+.
// cross half-GRF should guaranttee evenly split
uint8_t G4_DstRegRegion::getMaxExecSize(const IR_Builder& builder, int pos, uint8_t maxExSize, bool twoGRFsrc)
{
    if (acc != Direct)
    {
        return roundDownPow2(maxExSize);
    }

    uint8_t elSize = (uint8_t)getTypeSize();
    uint8_t exTypeSize = horzStride * elSize;
    uint8_t maxSize = roundDownPow2(maxExSize);
    uint32_t newLB = getLeftBound() + pos * exTypeSize,
        newRB = newLB + (maxExSize - 1) * exTypeSize + elSize - 1;
    uint32_t leftGRF = newLB / builder.numEltPerGRF<Type_UB>(), rightGRF = newRB / builder.numEltPerGRF<Type_UB>();
    // pre-BDW does not allow cross GRF dst except full 2-GRF dst.
    // BDW+ allows if elements are evenly split between two GRFs
    bool crossGRF = false;
    if (isCrossGRFDst(builder))
    {
        // check cross GRF boundary
        uint8_t byteInFirstGRF = ((leftGRF + 1) * builder.numEltPerGRF<Type_UB>() - newLB);
        uint8_t eleInFirstGRF = byteInFirstGRF / exTypeSize +
            // v20(0,17)<2>:ub and simd size is 16
            ((byteInFirstGRF % exTypeSize != 0) && (byteInFirstGRF % exTypeSize >= elSize) ? 1 : 0);

        if (leftGRF != rightGRF)
        {
            uint8_t pow2 = roundDownPow2(eleInFirstGRF);
            if (pow2 != eleInFirstGRF)
            {
                maxSize = pow2;
                newRB = newLB + (maxSize - 1) * exTypeSize + elSize - 1;
            }
            else
            {
                // number of elements in first GRF is power of 2 and HS is not used to cross GRF
                // search into second GRF
                // if number of elements in second GRF >= numbr of elements in first GRF
                uint8_t byteInSecondGRF = (newRB + 1) % builder.numEltPerGRF<Type_UB>();
                uint8_t eleInSecondGRF = byteInSecondGRF / exTypeSize + (horzStride > 1 ? 1 : 0);
                if (eleInSecondGRF >= eleInFirstGRF)
                {
                    crossGRF = true;
                    maxSize = eleInFirstGRF * 2;
                }
            }
        }
    }
    // check if cross half-GRF boundary
    // FIXME: if we know that the new srcs are all in one GRF, we do not have to do the following check.
    if (!crossGRF && twoGRFsrc)
    {
        uint32_t halfGRFSize = builder.numEltPerGRF<Type_UB>() / 2;
        if (newLB / halfGRFSize != newRB / halfGRFSize)
        {
            uint32_t middlePoint = (newRB + (horzStride - 1) * elSize - newLB + 1) / 2;
            // check middle point
            if ((middlePoint + newLB) % halfGRFSize != 0)
            {
                // check size before half-GRF
                uint8_t sizeBeforeMidGRF = ((leftGRF * builder.numEltPerGRF<Type_UB>() + halfGRFSize) - newLB + exTypeSize - 1) / exTypeSize;
                uint8_t pow2Size = roundDownPow2(sizeBeforeMidGRF);
                // V36(0,1)<4>:ud is slipt into 2x2
                if (sizeBeforeMidGRF <= (maxSize >> 1) && pow2Size == sizeBeforeMidGRF)
                {
                    maxSize = 2 * pow2Size;
                }
                else
                {
                    maxSize = pow2Size;
                }
            }
        }
    }

    return maxSize;
}
//
// output (Var+refOff).subRegOff<1><WriteMask>
//
// symbolreg == false,  output <modifier>(Var+refOff).subRegOff<16;16,1>.xxyy
// symbolreg == true,   output <modifier><symbol>(RegOff, SubRegOff)<16;16,1> in symbolic register emit
// Here we use symbolreg instead of Options::symbolReg, because sometimes we need to emit an instruction with invalid symbolic register as comment and
// then emit a workable instruction with physical register. In this case, we do not want to re-set the global option variable Options::symbolReg to switch
// between these two states, that may have potential side effects.
//
void G4_DstRegRegion::emit(std::ostream& output, bool symbolreg)
{
    //
    // output Var(refOff,subRegOff)
    //
    emitRegVarOff(output, symbolreg);

    //
    // output <horzStride>
    //
    if (inst != NULL && inst->isSplitSend())
    {
        // do nothing for sends
    }
    else if (writeMask != NoChannelEnable)
    {
        // do nothing for align16 instructions
    }
    else if (isAccRegValid())
    {
        // do nothing for madm
    }
    else if (horzStride != UNDEFINED_SHORT)
    {
        output << '<' << horzStride << '>';
    }
    else if (base->isAreg())
    {
        output << "<1>";
    }
    else if (base->isNullReg())
    {
        // do not emit region for null reg
    }
    else if (base->isFlag())
    {
        output << "<1>";
    }
    else
    {
        MUST_BE_TRUE(false, "No default region specified");
    }

    if (isAccRegValid())
    {
        // output acc2~acc9
        if (getAccRegSel() == NOACC)
        {
            output << ".noacc";
        }
        else
        {
            output <<".acc"<< (getAccRegSel()+2);
        }
    }
    else if (writeMask != NoChannelEnable)
    {
        output << "." << getChannelEnableStr(writeMask);
    }

    if (Type_UNDEF != type)
    {
        if (!symbolreg || acc != Direct)                // can output register data type for indirect addressing in any time
            output << ':' << TypeSymbol(type);
    }
}

//
// This function is used to check if the src operand obey the rule of symbolic register. We need this function to check the operand before we emit an instruction
//
bool G4_DstRegRegion::obeySymbolRegRule() const
{
    if (!base->isRegVar())          // only for reg var
        return false;
    if (base->asRegVar()->getDeclare()->isSpilled())
    {
        return false;
    }
    //
    // For dst operand, we do not have Rule-2
    // Rule-2: must have register region or default register region
    //
    // Rule-3: No swizzle .xyzw
    //
    if (writeMask != NoChannelEnable)
    {
        return false;
    }
    //
    // Rule-4: do not support date type redefinition in direct addressing
    //
    if (Type_UNDEF != type)
    {
         if (base->isRegVar() && acc == Direct && base->asRegVar()->getDeclare()->getElemType() != type)        // check if the data type is the same as in declare
         {
             return false;
         }
    }

    return true;
}

//
// Here we use symbolreg instead of Options::symbolReg, because sometimes we need to emit an instruction with invalid symbolic register as comment and
// then emit a workable instruction with physical register. In this case, we do not want to re-set the global option variable Options::symbolReg to switch
// between these two states, that may have potential side effects.
//
void G4_DstRegRegion::emitRegVarOff(std::ostream& output, bool symbolreg)
{
    bool printSubReg = true;
    if (inst != NULL && inst->isSplitSend())
    {
        printSubReg = false;
    }
    printRegVarOff(output, this, regOff,subRegOff,immAddrOff,type, symbolreg, printSubReg);
}

//
// return true if prd and this are the same inst predicate
//
bool G4_Predicate::samePredicate(const G4_Predicate& prd) const
{
    return getBase() == prd.getBase() &&
           state == prd.state &&
           subRegOff == prd.subRegOff &&
           control == prd.control;
}
//
// return true if mod and this are the same condition modifier
//
bool G4_CondMod::sameCondMod(const G4_CondMod& m) const
{
    return getBase() == m.getBase() &&
           mod == m.mod &&
           subRegOff == m.subRegOff;
}

//
// create all physical register operands
//
PhyRegPool::PhyRegPool(Mem_Manager& m, unsigned int maxRegisterNumber)
{
    maxGRFNum = maxRegisterNumber;

    GRF_Table = (G4_Greg**)m.alloc(sizeof(G4_Greg*) * maxGRFNum);
    // create General Registers
    for (unsigned int i = 0; i < maxGRFNum; i++)
        GRF_Table[i] = new (m) G4_Greg(i);

    for (unsigned i = 0; i < AREG_LAST; i++)
    {
        ARF_Table[i] = nullptr;
    }

    // create Architecture Registers
    ARF_Table[AREG_NULL]     = new (m) G4_Areg(AREG_NULL);
    ARF_Table[AREG_A0]       = new (m) G4_Areg(AREG_A0);
    ARF_Table[AREG_ACC0]     = new (m) G4_Areg(AREG_ACC0);
    ARF_Table[AREG_ACC1]     = new (m) G4_Areg(AREG_ACC1);
    ARF_Table[AREG_MASK0]    = new (m) G4_Areg(AREG_MASK0);
    ARF_Table[AREG_MS0]      = new (m) G4_Areg(AREG_MS0);
    ARF_Table[AREG_DBG]      = new (m) G4_Areg(AREG_DBG);
    ARF_Table[AREG_SR0]      = new (m) G4_Areg(AREG_SR0);
    ARF_Table[AREG_CR0]      = new (m) G4_Areg(AREG_CR0);
    ARF_Table[AREG_TM0]      = new (m) G4_Areg(AREG_TM0);
    ARF_Table[AREG_N0]       = new (m) G4_Areg(AREG_N0);
    ARF_Table[AREG_N1]       = new (m) G4_Areg(AREG_N1);
    ARF_Table[AREG_IP]       = new (m) G4_Areg(AREG_IP);
    ARF_Table[AREG_F0]       = new (m) G4_Areg(AREG_F0);
    ARF_Table[AREG_F1]       = new (m) G4_Areg(AREG_F1);
    ARF_Table[AREG_TDR0]     = new (m) G4_Areg(AREG_TDR0);
    ARF_Table[AREG_SP]       = new (m)G4_Areg(AREG_SP);
    ARF_Table[AREG_F2]       = new (m) G4_Areg(AREG_F2);
    ARF_Table[AREG_F3]       = new (m) G4_Areg(AREG_F3);
}

void PhyRegPool::rebuildRegPool(Mem_Manager& m, unsigned int numRegisters)
{
    maxGRFNum = numRegisters;

    GRF_Table = (G4_Greg**)m.alloc(sizeof(G4_Greg*) * maxGRFNum);
    // create General Registers
    for (unsigned int i = 0; i < maxGRFNum; i++)
        GRF_Table[i] = new (m) G4_Greg(i);
}

void G4_Declare::setEvenAlign()
{
    regVar->setEvenAlign();
}

void G4_Declare::setSubRegAlign(G4_SubReg_Align subAl)
{
    regVar->setSubRegAlignment(subAl);
}

bool G4_Declare::isEvenAlign() const
{
    return regVar->isEvenAlign();
}

G4_SubReg_Align G4_Declare::getSubRegAlign() const
{
    return regVar->getSubRegAlignment();
}

void G4_Declare::copyAlign(G4_Declare* dcl)
{
    if (dcl->isEvenAlign())
    {
        setEvenAlign();
    }
    regVar->setSubRegAlignment(dcl->getSubRegAlign());
}

void G4_Declare::ElemInfo::reset(unsigned numElems, const IR_Builder& irb)
{
    numElements = numElems;
    numRows = (getByteSize() + (irb.numEltPerGRF<Type_UB>() - 1)) / irb.numEltPerGRF<Type_UB>();
    numElemsPerRow = numRows > 1 ? irb.numEltPerGRF<Type_UB>() / getElemSize() : numElements;
}

void G4_Declare::resizeNumRows(unsigned int numrows)
{
    unsigned byteSize = numrows * irb.numEltPerGRF<Type_UB>();
    setTotalElems(byteSize / getElemSize());
}

void G4_Declare::emit(std::ostream &output) const
{

    output << "//.declare " << name;
    output << " rf=";
    if (useGRF())
    {
        output << 'r';
    }
    else if (regFile == G4_ADDRESS)
    {
        output << 'a';
    }
    else if (regFile == G4_SCALAR)
    {
        output << 's';
    }
    else if (regFile == G4_FLAG)
    {
        output << 'f';
    }
    else
    {
        MUST_BE_TRUE(false, ERROR_UNKNOWN); //unhandled case
    }

    output << " size=" << getByteSize();
    if (Type_UNDEF != elemInfo.getType())
    {
        output << " type=" << TypeSymbol(elemInfo.getType());
    }
    if (AliasDCL)
    {
        output << " alias=" << AliasDCL->getName() << "+" << getAliasOffset();
    }
    output << " align=" << getSubRegAlign() << " words";
    if (regVar->isPhyRegAssigned())
    {
        G4_VarBase* phyreg = regVar->getPhyReg();
        if (phyreg->isGreg())
        {
            output << " (r" << phyreg->asGreg()->getRegNum() << "." << regVar->getPhyRegOff() << ")";
        }
        else if (phyreg->isAddress())
        {
            output << " (a0." << regVar->getPhyRegOff() << ")";
        }
        else if (phyreg->isFlag())
        {
            bool valid = false;
            output << " (f" << phyreg->asAreg()->ExRegNum(valid) << "." << regVar->getPhyRegOff() << ")";
        }
    }
    else if (isSpilled())
    {
        if (spillDCL)
        {
            // flag/addr spill
            output << " (spilled -> " << spillDCL->getName() << ")";
        }
        else
        {
            // GRF spill
            auto GRFOffset = getRegVar()->getDisp() / irb.getGRFSize();
            if (!AliasDCL)
            {
                output << " (spilled -> Scratch[" << GRFOffset << "x" << (int)irb.getGRFSize() << "])";
            }
            else
            {
                output << " (spilled)";
            }
        }
    }

    if (liveIn && liveOut)
    {
        output << " Input_Output";
    }
    else if (liveIn)
    {
        output << " Input";
    }
    else if (liveOut)
    {
        output << " Output";
    }

    output << "\n";
}

void G4_Predicate::emit(std::ostream& output, bool symbolreg)
{
    output << "(";
    emit_body(output, symbolreg);
    output << ") ";
}

void G4_Predicate::emit_body(std::ostream& output, bool symbolreg)
{
    static const char* align16ControlNames[] =
    {
        "",
        "xyzw",
        "x",
        "y",
        "z",
        "w"
        "any4h",
        "all4h"
    };

    if (state == PredState_Minus)
    {
        output << '!';
    }

    if (getBase()->asRegVar()->isPhyRegAssigned())
    {
        getBase()->asRegVar()->getPhyReg()->emit(output);
        output << "." << getBase()->asRegVar()->getPhyRegOff();
    }
    else
    {
        getBase()->emit(output);
        if (subRegOff != UNDEFINED_SHORT)
        {
            output << '.' << subRegOff;
        }
    }

    if (align16Control != PRED_ALIGN16_DEFAULT)
    {
        output << "." << align16ControlNames[align16Control];
    }
    else
    {
        if (control != PRED_DEFAULT)
        {
            output << '.';
            switch (control)
            {
            case PRED_ANY2H:
                output << "any2h";
                break;
            case PRED_ANY4H:
                output << "any4h";
                break;
            case PRED_ANY8H:
                output << "any8h";
                break;
            case PRED_ANY16H:
                output << "any16h";
                break;
            case PRED_ANY32H:
                output << "any32h";
                break;
            case PRED_ALL2H:
                output << "all2h";
                break;
            case PRED_ALL4H:
                output << "all4h";
                break;
            case PRED_ALL8H:
                output << "all8h";
                break;
            case PRED_ALL16H:
                output << "all16h";
                break;
            case PRED_ALL32H:
                output << "all32h";
                break;
            case PRED_ANYV:
                output << "anyv";
                break;
            case PRED_ALLV:
                output << "allv";
                break;
            default:
                // do nothing
                break;
            }
        }
    }
}

G4_Predicate::G4_Predicate(G4_Predicate &prd)
    : G4_Operand(G4_Operand::predicate, prd.getBase())
{
    state = prd.state;
    subRegOff = prd.subRegOff;
    control = prd.control;
    align16Control = prd.align16Control;

    top_dcl = prd.top_dcl;
    left_bound = prd.left_bound;
    right_bound = prd.right_bound;
    bitVec[0] = prd.bitVec[0];
    bitVec[1] = prd.bitVec[1];
    byteOffset = prd.byteOffset;
    rightBoundSet = prd.rightBoundSet;
    isPredicateSameAsNoMask = prd.isPredicateSameAsNoMask;
}

unsigned G4_Predicate::computeRightBound(uint8_t exec_size)
{
    rightBoundSet = true;
    bitVec[0] = 0;
    bitVec[1] = 0;

    uint16_t group_size = (uint16_t)getPredCtrlGroupSize();
    uint16_t totalBits = (exec_size > group_size) ? exec_size : group_size;

    if (inst)
        left_bound = inst->getMaskOffset();

    right_bound = left_bound + totalBits - 1;

    bitVec[0] = exec_size == 32 ? 0xFFFFFFFF : (1 << exec_size) - 1;

    return right_bound;
}

static G4_CmpRelation compareBound(uint32_t myLB, uint32_t myRB, uint32_t otherLB, uint32_t otherRB)
{
    if (myLB == otherLB && myRB == otherRB)
    {
        return Rel_eq;
    }
    else if (myRB < otherLB || otherRB < myLB)
    {
        return Rel_disjoint;
    }
    else if (myLB <= otherLB && myRB >= otherRB)
    {
        return Rel_gt;
    }
    else if (myLB >= otherLB && myRB <= otherRB)
    {
        return Rel_lt;
    }
    else
    {
        return Rel_interfere;
    }
}

/// compare flag to opnd
/// flag is either a G4_Predicate or G4_CondMod, opnd can be any G4_operand
/// We put this in a separate function since G4_Predicate and G4_CondMod
/// should have identical code for compareOperand
static G4_CmpRelation compareFlagToOperand(G4_Operand* flag, G4_Operand* opnd)
{
    assert((flag->isPredicate() || flag->isCondMod()) && "expect either predicate or conditional modifier");

    bool legalOpnd = opnd->isSrcRegRegion() || opnd->isDstRegRegion() || opnd->isPredicate() || opnd->isCondMod();
    G4_VarBase* myBase = flag->getBase();
    G4_VarBase *opndBase = opnd->getBase();

    if (!legalOpnd || myBase == nullptr || opndBase == nullptr || !opndBase->isFlag())
    {
        return Rel_disjoint;
    }

    // flags with different base declare definitely do not interfere (we do not consider physical flags here)
    if (flag->getTopDcl() != opnd->getTopDcl())
    {
        return Rel_disjoint;
    }

    // Do we generate pseudo kill on flags?
    G4_INST* opndInst = opnd->getInst();
    if (opndInst && (opndInst->isPseudoKill() || opndInst->isLifeTimeEnd()))
    {
        return Rel_interfere;
    }

    return compareBound(flag->getLeftBound(), flag->getRightBound(), opnd->getLeftBound(), opnd->getRightBound());
}

G4_CmpRelation G4_Predicate::compareOperand(G4_Operand *opnd, const IR_Builder& builder)
{
    return compareFlagToOperand(this, opnd);
}

// remove half of the bitvector and change right bound
void G4_Predicate::splitPred()
{
    uint16_t range = getRightBound() - getLeftBound() + 1;
    uint16_t shiftLen = range >> 2;
    right_bound = getLeftBound() + shiftLen - 1;

    bitVec[0] = ((uint32_t)getBitVecL()) >> shiftLen;
}

void G4_CondMod::emit(std::ostream& output, bool symbolreg)
{
    static const char* const CondModStr[Mod_cond_undef] =
    {
        "ze",  // zero
        "eq",  // equal
        "nz", // not zero
        "ne", // not equal
        "gt",  // greater
        "ge", // greater or equal
        "lt",  // less
        "le", // less or equal
        "ov",  // overflow
        "ri",  // round increment
        "un",  // unorder (NaN)
    };
    output << "(" <<  CondModStr[mod] << ")";
    if (getBase() == nullptr)
    {
        output << "f0.0";
    } else if (getBase()->asRegVar()->isPhyRegAssigned()) {
        getBase()->asRegVar()->getPhyReg()->emit(output);
        output << "." << getBase()->asRegVar()->getPhyRegOff();
    } else {
        getBase()->emit(output);
        if (subRegOff != UNDEFINED_SHORT)
        {
            output << '.' << subRegOff;
        }
    }
}
G4_CondMod::G4_CondMod(G4_CondMod &cMod)
    : G4_Operand(G4_Operand::condMod, cMod.getBase())
{
    mod = cMod.mod;
    subRegOff = cMod.subRegOff;

    top_dcl = cMod.top_dcl;
    left_bound = cMod.left_bound;
    right_bound = cMod.right_bound;
    bitVec[0] = cMod.bitVec[0];
    bitVec[1] = cMod.bitVec[1];
    byteOffset = cMod.byteOffset;
    rightBoundSet = cMod.rightBoundSet;
}

unsigned G4_CondMod::computeRightBound(uint8_t exec_size)
{
    bitVec[0] = 0;
    bitVec[1] = 0;
    rightBoundSet = true;

    if (inst)
        left_bound = inst->getMaskOffset();

    right_bound = left_bound + exec_size - 1;

    bitVec[0] = exec_size == 32 ? 0xFFFFFFFF : (1 << exec_size) - 1;

    return right_bound;
}

/// same as G4_Predicate::compareOperand
G4_CmpRelation G4_CondMod::compareOperand(G4_Operand *opnd, const IR_Builder& builder)
{
    return compareFlagToOperand(this, opnd);
}


// remove half of the bitvector and change right bound
void G4_CondMod::splitCondMod()
{
    uint16_t range = getRightBound() - getLeftBound() + 1;
    uint16_t shiftLen = range >> 2;
    right_bound = getLeftBound() + shiftLen - 1;

    bitVec[0] = ((uint32_t)getBitVecL()) >> shiftLen;
}
bool G4_Imm::isEqualTo(G4_Imm& imm1) const
{
    return (imm1.getType() == type) && (imm1.getImm() == imm.num);
}

// check if an immedate is in the range of type
bool G4_Imm::isInTypeRange(int64_t imm, G4_Type ty)
{
    switch (ty)
    {
    case Type_D:
        return imm >= (int)MIN_DWORD_VALUE && imm <= (int)MAX_DWORD_VALUE;
    case Type_Q:
        return true;
    case Type_UQ:
        return imm >= 0;
    case Type_UD:
        return (imm >= (unsigned)MIN_UDWORD_VALUE && imm <= (unsigned)MAX_UDWORD_VALUE);
    case Type_W:
        return (imm >= (int)MIN_WORD_VALUE && imm <= (int)MAX_WORD_VALUE);
    case Type_UW:
        return (imm >= (int)MIN_UWORD_VALUE && imm <= (int)MAX_UWORD_VALUE);
    case Type_B:
        return (imm >= (int)MIN_CHAR_VALUE && imm <= (int)MAX_CHAR_VALUE);
    case Type_UB:
        return (imm >= (int)MIN_UCHAR_VALUE && imm <= (int)MAX_UCHAR_VALUE);
    default:
        break;
    }

    return false;
}

bool G4_Imm::isZero() const
{
    if (IS_TYPE_F32_F64(type))
    {
        if (type == Type_F)
        {
            return (imm.fp32 == 0.0f);
        }
        return (imm.fp == 0.0);
    }
    return (imm.num == 0);
}

bool G4_Imm::isSignBitZero() const
{
    G4_Type Ty = getType();
    int64_t val = getInt();
    switch (Ty) {
    case Type_B:
    case Type_W:
    case Type_D:
    case Type_Q:
        return val > 0;
    case Type_V:
        return ((uint64_t)val & 0x88888888) == 0;
    default:
        break;
    }
    return false;
}

G4_CmpRelation G4_Imm::compareOperand(G4_Operand *opnd, const IR_Builder& builder)
{
    G4_CmpRelation rel = Rel_disjoint;
    if (opnd->isImm() && isEqualTo(opnd->asImm()))
    {
        return Rel_eq;
    }
    return rel;
}

void G4_Imm::emit(std::ostream& output, bool symbolreg)
{
    //
    // we only emit hex in this function
    //
    std::ios::fmtflags outFlags(output.flags());
    output.flags(std::ios_base::hex | std::ios_base::showbase);

    short word;
    if (type == Type_DF)
    {
        output << (uint64_t)imm.num;
    }
    else if (type == Type_F)
    {
        output << imm.num32;
    }
    else if (type == Type_W || type == Type_UW || type == Type_B || type == Type_UB)
    {
        word = (short)imm.num;
        output << word;
    }
    else if (type == Type_D || type == Type_UD)
    {
        // 32-bit int
        output << (int)imm.num;
    }
    else
    {
        // 64-bit int
        output << imm.num;
    }

    output.flags(outFlags);

    if (Type_UNDEF != type)
    {
        output << ':' << TypeSymbol(type);
    }
}

// emit number, automatically select the format according to its original format
void G4_Imm::emitAutoFmt(std::ostream& output)
{
    if (Type_F == type)
    {
        output << imm.fp32;
    }
    else if (Type_DF == type)
    {
       output << imm.fp;
    }
    else if (Type_W == type || Type_B == type)
    {
        output << (short)imm.num;
    }
    else if (Type_D == type)
    {
        output << imm.num;
    }
    else //unsigned value
    {
        output << (unsigned)imm.num;
    }

    if (Type_UNDEF != type)
    {
        output << ':' << TypeSymbol(type);
    }
}

int64_t G4_Imm::typecastVals(int64_t value, G4_Type type)
{
    int64_t retVal = 0;
    switch (type)
    {
    case Type_UD:
    case Type_UV:
    case Type_VF:
    {
        retVal = (int64_t)((unsigned)value);
        break;
    }
    case Type_D:
    case Type_V:
    {
        retVal = (int64_t)((int)value);
        break;
    }
    case Type_UW:
    {
        retVal = (int64_t)((uint16_t)value);
        break;
    }
    case Type_W:
    {
        retVal = (int64_t)((int16_t)value);
        break;
    }
    case Type_UB:
    {
        retVal = (int64_t)((uint8_t)value);
        break;
    }
    case Type_B:
    {
        retVal = (int64_t)((int8_t)value);
        break;
    }
    default:
    {
        // Dont do float conversions
        retVal = value;
    }
    }
    return retVal;
}

G4_RegVar *
G4_RegVarTransient::getNonTransientBaseRegVar ()
{
    G4_RegVar * base;
    for (base = getBaseRegVar (); base->isRegVarTransient (); base = base->getBaseRegVar ());
    return base;
}

G4_RegVar *
G4_RegVarTransient::getAbsBaseRegVar ()
{
    G4_RegVar * base;
    for (base = getBaseRegVar (); base->getBaseRegVar () != base; base = base->getBaseRegVar ());
    return base;
}

G4_RegVar *
G4_RegVarTmp::getAbsBaseRegVar ()
{
    G4_RegVar * base;
    for (base = getBaseRegVar (); base->getBaseRegVar () != base; base = base->getBaseRegVar ());
    return base;
}

void
G4_RegVar::emit(std::ostream& output, bool symbolreg)
{

    output << decl->getName();
    if (reg.phyReg != NULL)
    {
        output << "(";
        reg.phyReg->emit(output);
        output << '.' << reg.subRegOff << ':' <<
            TypeSymbol(getDeclare()->getElemType()) << ")";
    }
}
int G4_AddrExp::eval(const IR_Builder& builder)
{
    int byteAddr = 0;

    MUST_BE_TRUE(m_addressedReg->getPhyReg()  != NULL, "No addr takenregister found!");

    byteAddr = m_addressedReg->getByteAddr(builder); //let's assume the unsigned=>int won't overflow for now.
    byteAddr += m_offset;

    return byteAddr;
}
void G4_AddrExp::emit(std::ostream& output, bool symbolreg)
{
    output << '&';
    m_addressedReg->emit(output);
    output << '+' << m_offset;
}

void G4_SrcRegRegion::computeLeftBound(const IR_Builder& builder)
{
    top_dcl = NULL;
    unsigned newregoff = regOff, offset = 0;

    if (base)
    {
        if (base->isRegVar())
        {
            top_dcl = base->asRegVar()->getDeclare();
            if (!top_dcl && base->asRegVar()->isGreg())
            {
                newregoff = base->asRegVar()->asGreg()->getRegNum();
            }
        }
    }

    if (top_dcl)
    {
        while (top_dcl->getAliasDeclare())
        {
            offset += top_dcl->getAliasOffset();
            top_dcl = top_dcl->getAliasDeclare();
        }
    }

    if (base != NULL && base->isFlag())
    {
        if (base->isRegVar())
        {
            if (base->asRegVar()->getPhyReg())
            {
                left_bound = base->asRegVar()->getPhyRegOff() * 16;   // the bound of flag register is in unit of BIT
                left_bound += subRegOff * 16;
                left_bound += base->asRegVar()->getPhyReg()->asAreg()->getFlagNum() * 32;
            }
            else
            {
                left_bound = subRegOff * 16;
            }
        }
        else
        {
            left_bound = subRegOff * 16;
            left_bound += base->asAreg()->getFlagNum() * 32;
        }

        right_bound = 0;
    }
    else if (base != NULL && base->isAccReg())
    {
        left_bound = subRegOff * TypeSize(type);
        if (base->asAreg()->getArchRegType() == AREG_ACC1)
        {
            left_bound += 32;  // TODO: size of ACC is assumed to be 32 BYTEs.
        }
        byteOffset = left_bound;
    }
    else if (top_dcl)
    {
        if (acc == Direct)
        {
            left_bound = offset + newregoff * builder.numEltPerGRF<Type_UB>() + subRegOff * TypeSize(type);
            if (top_dcl->getTotalElems() * top_dcl->getElemSize() >= (int)builder.numEltPerGRF<Type_UB>())
            {
                byteOffset = left_bound;
            }
            else
            {
                unsigned alignOff = TypeSize(type) > TypeSize(Type_W) ?
                    TypeSize(type) : TypeSize(Type_W);
                if (top_dcl->getSubRegAlign() == Even_Word || top_dcl->getSubRegAlign() >= Four_Word)
                {
                    alignOff = top_dcl->getSubRegAlign() * 2;
                }
                byteOffset = left_bound + alignOff;
            }
        }
        else
        {
            left_bound = subRegOff * TypeSize(ADDR_REG_TYPE);
            byteOffset = TypeSize(type);
        }

        if (desc && desc->isScalar())
        {
            right_bound = left_bound + TypeSize(type) - 1;
        }
        else
        {
            right_bound = 0;
            // for other cases, we need execution size and instruction compression attr, so we just set
            // partial value here, which will be patched later
            // right_bound = desc->horzStride * TypeSize(type);
            // patch it with *exec_size + left_bound
            // if vertical stride == 0 and width < exec_size, divide it by 2
        }
    }
    else
    {  //arch reg
        left_bound = 0;
        byteOffset = left_bound;
    }
}

void G4_SrcRegRegion::setSrcBitVec(uint8_t exec_size, const IR_Builder& irb)
{
    uint64_t bit_seq = TypeFootprint(type);
    unsigned short typeSize = TypeSize(type);

    uint64_t footPrint0 = 0;
    uint64_t footPrint1 = 0;

    MUST_BE_TRUE(exec_size >= desc->width, "exec size must be >= width");
    if (desc->isScalar())
    {
        footPrint0 = bit_seq;
    }
    else if (desc->isContiguous(exec_size))
    {
        // fast path
        int totalBytes = exec_size * typeSize;
        MUST_BE_TRUE(totalBytes <= 2 * irb.getGRFSize(), "total bytes exceed 2 GRFs");

        footPrint0 = totalBytes < 64 ? (1ULL << totalBytes) - 1 : ULLONG_MAX;
        if (totalBytes > 64)
        {
            footPrint1 = totalBytes == 128 ? ULLONG_MAX : (1ULL << (totalBytes - 64)) - 1;
        }
    }
    else
    {
        for (int i = 0, numRows = exec_size / desc->width; i < numRows; ++i)
        {
            for (int j = 0; j < desc->width; ++j)
            {
                int eltOffset = i * desc->vertStride * typeSize + j * desc->horzStride * typeSize;
                // no element can cross 64-byte boundary
                if (eltOffset >= 64)
                {
                    footPrint1 |= bit_seq << (eltOffset - 64);
                }
                else
                {
                    footPrint0 |= bit_seq << eltOffset;
                }
            }
        }
    }

    bitVec[0] = footPrint0;
    bitVec[1] = footPrint1;
}

unsigned G4_SrcRegRegion::computeRightBound(uint8_t exec_size)
{
    unsigned short hs = desc->isScalar() ? 1 : desc->horzStride;
    unsigned short vs = desc->isScalar() ? 0 : desc->vertStride;
    rightBoundSet = true;
    unsigned short typeSize = TypeSize(type);

    bitVec[0] = 0;
    bitVec[1] = 0;
    if (base->isFlag())
    {
        unsigned int totalBits = 0;
        if (G4_Inst_Table[inst->opcode()].instType != InstTypePseudoLogic)
        {
            // mov (1) ... fx.1<0;1,0>:uw
            left_bound = subRegOff * 16;
            totalBits = base->asRegVar()->getDeclare()->getNumberFlagElements() < TypeBitSize(type) ?
                        base->asRegVar()->getDeclare()->getNumberFlagElements() : TypeBitSize(type);
        }
        else
        {
            /*
                we need to set leftBound for pseudo intruction
                so that it creates use/def links correctly in the control flow graph between
                cmp instruction and pseudo instruction.
                This matters when we break up SIMD32 instruction in to two SIMD16 with H1/H2 masks.
                The bound for compare for H2 will be [15,31], and this has to match.
                Without this no use/def link was created which caused issues in logic optimization.
                Also it produce incorrect behavior in any operation that relies on compareOperand.
            */
            left_bound = inst->getMaskOffset();
            totalBits = exec_size;
        }

        right_bound = left_bound + totalBits - 1;

        bitVec[0] = totalBits == 32 ? 0xFFFFFFFF : (1 << totalBits) - 1;
    }
    else
    {
        if (acc == Direct)
        {
            if (inst->isReturn() || inst->isFReturn())
            {
                exec_size = 2;
            }

            setSrcBitVec(exec_size, inst->getBuilder());

            if (desc->isScalar())
            {
                right_bound = left_bound + typeSize - 1;
            }
            else
            {
                int num_rows = exec_size / desc->width;
                if (num_rows > 0)
                {
                    right_bound =
                        left_bound +
                        (num_rows - 1) * vs * typeSize +
                        hs * (desc->width - 1) * typeSize +
                        typeSize - 1;
                }
                else
                {
                    // this fix applies to inplicit acc src
                    // usually when we compute new rb after inst splitting,
                    // the region is still the old one.
                    // exec_size may be smaller than width
                    right_bound =
                        left_bound +
                        hs * (exec_size - 1) * typeSize +
                        typeSize - 1;
                }
            }
        }
        else
        {
            unsigned short numAddrSubReg = 1;
            if (desc->isRegionWH())
            {
                numAddrSubReg = exec_size/desc->width;
            }
            for (uint16_t i = 0; i < numAddrSubReg; i++)
            {
                bitVec[0] |= ((uint64_t) 0x3) << (i * 2);
            }
            right_bound = left_bound + TypeSize(ADDR_REG_TYPE) * numAddrSubReg - 1;
        }
    }
    return right_bound;
}

G4_CmpRelation G4_SrcRegRegion::compareOperand(G4_Operand *opnd, const IR_Builder& builder)
{
    return compareRegRegionToOperand(this, opnd, builder);
}

bool G4_SrcRegRegion::isNativeType() const
{
    G4_Type type = getType();

    if (IS_WTYPE(type) || IS_DTYPE(type) || IS_FTYPE(type) || type == Type_DF) {
        return true;
    }
    else {
        return false;
    }
}

bool G4_SrcRegRegion::isNativePackedRowRegion() const
{
    if (isNativeType())
    {
        // A single element row is always packed.
        return (desc->horzStride == 1) ||
               (desc->width == 1 && desc->horzStride == 0);
    }

    return false;
}

bool G4_SrcRegRegion::isNativePackedRegion() const
{
    return isNativePackedRowRegion() && desc->vertStride == desc->width;
}

bool G4_SrcRegRegion::coverTwoGRF(const IR_Builder& builder)
{
    uint16_t range = getRightBound() - getLeftBound() + 1;
    if (range < builder.numEltPerGRF<Type_UB>())
        return false;
    if (desc->horzStride > 1)
    {
        range += (desc->horzStride - 1) * TypeSize(type);
    }
    if (range == builder.numEltPerGRF<Type_UB>() * 2 &&
        (desc->vertStride == desc->horzStride * desc->width ||
         desc->isContiguous(getInst()->getExecSize())))
    {
        return true;
    }
    return false;
}
// Assumption:
// operand crosses GRF boundary
bool G4_SrcRegRegion::evenlySplitCrossGRF(const IR_Builder& builder, uint8_t execSize, bool &sameSubRegOff,
    bool &vertCrossGRF, bool &contRegion, uint8_t &eleInFirstGRF)
{
    // always return true since all align16 instructions are generated by JIT
    // later on when we have other execution types for align16 instructions,
    // fix the following if to check src element distribution.
    // FIXME: do we need to check HS here?
    if (desc->isRegionV())
    {
        sameSubRegOff = true;
        vertCrossGRF = true;
        contRegion = true;
        return true;
    }
    vertCrossGRF = true;
    contRegion = desc->isSingleStride(getInst()->getExecSize());
    MUST_BE_TRUE(acc == Direct, "Indirect operand can not cross GRF boundary.");
    uint8_t firstSubRegOff = getLeftBound() % builder.numEltPerGRF<Type_UB>();
    uint8_t left = firstSubRegOff;
    uint8_t typeSize = (uint8_t)TypeSize(type);
    uint8_t execTySize = (desc->horzStride == 0 ? 1 : desc->horzStride) * typeSize;
    uint8_t lastEltEndByte = desc->horzStride * (desc->width - 1) * typeSize + typeSize;
    uint8_t realRowSize = lastEltEndByte;
    // check number of elements in first GRF.
    eleInFirstGRF = 0;
    while (left < builder.numEltPerGRF<Type_UB>())
    {
        if (left + realRowSize  <= (int)builder.numEltPerGRF<Type_UB>())
        {
            // realRowSize is used to handle V12(0,17)<32;8,2>:b
            eleInFirstGRF += desc->width;
            left += desc->vertStride * TypeSize(type);
        }
        else
        {
            vertCrossGRF = false;
            // V12(0,17)<32;8,2>:b is a good two GRF source
            eleInFirstGRF++;
            uint8_t newLeft = left + typeSize;
            newLeft += execTySize;
            while (newLeft < builder.numEltPerGRF<Type_UB>())
            {
                eleInFirstGRF++;
                newLeft += execTySize;
            }
            if (newLeft == builder.numEltPerGRF<Type_UB>())
            {
                eleInFirstGRF++;
                if (eleInFirstGRF % desc->width == 0)
                {
                    left += desc->vertStride * TypeSize(type);
                }
                else
                {
                    left = newLeft + (execTySize - typeSize);
                }
            }
            else if (eleInFirstGRF % desc->width == 0)
            {
                left += desc->vertStride * TypeSize(type);
            }
            else if (typeSize == execTySize)
            {
                left = newLeft;
            }
            else
            {
                left = newLeft - typeSize;
            }
        }
    }
    uint8_t secondSubRegOff = left % builder.numEltPerGRF<Type_UB>();

    sameSubRegOff = (firstSubRegOff == secondSubRegOff);
    // TODO: this guaranttees that there are equal number fo elements in each GRF, but not the distribution of elements in each of them.
    if (eleInFirstGRF * 2 == execSize)
    {
        return true;
    }
    return false;
}

bool G4_SrcRegRegion::evenlySplitCrossGRF(const IR_Builder& builder, uint8_t execSize)
{
    // check number of elements in first GRF.
    MUST_BE_TRUE(acc == Direct, "Indirect operand can not cross GRF boundary.");
    uint16_t sizeInFirstGRF = builder.numEltPerGRF<Type_UB>() - getLeftBound() % builder.numEltPerGRF<Type_UB>();
    uint16_t vertSize = desc->vertStride * getElemSize();
    uint16_t execTypeSize = desc->horzStride == 0 ? getElemSize() : desc->horzStride * getElemSize();
    uint16_t numEle = (sizeInFirstGRF + execTypeSize - 1)/ execTypeSize;
    uint16_t rowSize = desc->horzStride == 0 ? execTypeSize : desc->width * execTypeSize,
        numRows = desc->vertStride == 0 ? 1 : execSize/desc->width,
        numElePerRow = rowSize / execTypeSize,
        numExecEmePerRow = desc->horzStride == 0 ? 1 : desc->width;

    if (sizeInFirstGRF <= vertSize)
    {
        if (numEle >= desc->width)
        {
            numEle = desc->width;
        }
    }
    else if (desc->vertStride > desc->width)
    {
        numEle = sizeInFirstGRF/vertSize * numExecEmePerRow +
            ((sizeInFirstGRF%vertSize > rowSize) ? numExecEmePerRow : (sizeInFirstGRF%vertSize + execTypeSize - 1) / execTypeSize);
    }

    uint16_t totalNumEle = (desc->vertStride >= numElePerRow) ? (numRows * numExecEmePerRow) :
        (getRightBound() - getLeftBound() + 1) / execTypeSize;

    // TODO: this guarantees that there are equal number of elements in each GRF, but not the distribution of elements in each of them.
    if (numEle * 2 == totalNumEle)
    {
        return true;
    }
    return false;
}

/*
 * check if the input opnd is align to GRF
 * if the first level dcl is not aligned to GRF or sub register offset of this opnd is not multiple GRFs, including 0,
 * return true.
 */
bool G4_SrcRegRegion::checkGRFAlign(const IR_Builder& builder) {

    bool GRF_aligned = false;
    uint32_t byte_subregoff = subRegOff * getTypeSize();

    if (byte_subregoff  % builder.numEltPerGRF<Type_UB>() != 0) {
        return false;
    }

    if (base) {
        if (base->isRegVar()) {
            G4_Declare *dcl = base->asRegVar()->getDeclare();

            if (dcl) {
                G4_Declare *aliasdcl = dcl;

                unsigned aliasOffset = 0;
                while (aliasdcl->getAliasDeclare())
                {
                    aliasOffset += aliasdcl->getAliasOffset();
                    aliasdcl = aliasdcl->getAliasDeclare();
                }
                if (aliasOffset % builder.numEltPerGRF<Type_UB>() != 0)
                {
                    return false;
                }

                if (aliasdcl->getSubRegAlign() >= builder.getGRFAlign() ||
                    aliasdcl->getNumRows() * aliasdcl->getElemSize() * aliasdcl->getElemSize() >= (int)builder.numEltPerGRF<Type_UB>()) {
                        return true;
                }
            } else if (base->asRegVar()->isPhyRegAssigned() &&
                base->asRegVar()->getByteAddr(builder) % builder.numEltPerGRF<Type_UB>() == 0) {
                    return true;
            }
        }
    }

    return GRF_aligned;
}

//
// returns true if this SrcRegRegion has a fixed subreg offset (in bytes).
// This is true only if
// -- src is direct
// -- base declare is a GRF variable that is GRF-aligned
// if true, the subreg offset is also returned via offset
// Note this always returns false for ARFs (flag, addr, etc.)
//
bool G4_SrcRegRegion::hasFixedSubregOffset(const IR_Builder& builder, uint32_t& offset)
{
    return regionHasFixedSubreg(builder, this, offset);
}

/*
 * Return true if the src operand has a native type and has a packed (stride
 * of 1) region.
 */
bool G4_SrcRegRegion::isNativePackedSrcRegion()
{
    return isNativePackedRowRegion() &&
            (desc->vertStride == desc->width);
}

void RegionDesc::emit(std::ostream& output) const
{
    if (isRegionV())
    {
        output << '<' << horzStride << '>';
    }
    else if (isRegionWH())
    {
        output << '<' << width << ',' << horzStride << '>';
    }
    else
    {
        output << '<' << vertStride << ';' << width << ',' << horzStride << '>';
    }
}

void G4_Label::emit(std::ostream& output, bool symbolreg)
{
    output << label;
}

unsigned G4_RegVar::getByteAddr(const IR_Builder& builder) const
{
    MUST_BE_TRUE(reg.phyReg != NULL, ERROR_UNKNOWN);
    if (reg.phyReg->isGreg())
    {
        return reg.phyReg->asGreg()->getRegNum() * builder.numEltPerGRF<Type_UB>() +
            reg.subRegOff * decl->getElemSize();
    }
    if (reg.phyReg->isA0())
    {
        return reg.subRegOff * TypeSize(Type_UW);
    }

    MUST_BE_TRUE(false, ERROR_UNKNOWN);
    return 0;
}

void G4_RegVar::setSubRegAlignment(G4_SubReg_Align subAlg)
{
    // sub reg alignment can only be more restricted than prior setting
    MUST_BE_TRUE(subAlign == Any || subAlign == subAlg || subAlign % 2 == 0,
                 ERROR_UNKNOWN);
    if (subAlign > subAlg)
    {
        MUST_BE_TRUE(subAlign % subAlg == 0, "Sub reg alignment conflict");
        // do nothing; keep the original alignment (more restricted)
    }
    else
    {
        MUST_BE_TRUE(subAlg % subAlign == 0, "Sub reg alignment conflict");
        subAlign = subAlg;
    }
}

// For implicit Acc operands, left bound depends on
//    a) Inst execution type
//    b) Qtr control
//
// This function handles relevant cases, including hw intricacies
// and updates left bound only.
//
void G4_INST::computeLeftBoundForImplAcc(G4_Operand* opnd)
{
    if (opnd != NULL)
    {
        G4_Type extype;
        int extypesize;
        extype = getOpExecType(extypesize);

        if ((IS_WTYPE(extype) || IS_DTYPE(extype)))
        {
            // This condition is a result of HW Conformity requirement
            // that for exec type = D/DW, only acc0 is used even when
            // qtr control is set to Q2/H2
            opnd->setLeftBound(0);
        }
        else
        {
            if (opnd->isSrcRegRegion())
            {
                opnd->asSrcRegRegion()->computeLeftBound(getBuilder());
            }
            else if (opnd->isDstRegRegion())
            {
                opnd->asDstRegRegion()->computeLeftBound(getBuilder());
            }
        }
    }
}

bool G4_Operand::crossGRF(const IR_Builder& builder)
{
    return getRightBound() / builder.numEltPerGRF<Type_UB>() !=
           getLeftBound() / builder.numEltPerGRF<Type_UB>();
}

uint64_t G4_Operand::getBitVecH(const IR_Builder& builder)
{
    if (isRightBoundSet() == false && !isNullReg())
    {
        // computeRightBound also computes bitVec
        inst->computeRightBound(this);
    }
    if (builder.getGRFSize() == 32)
    {
        assert(bitVec[1] == 0 && "upper bits should be 0");
    }
    return bitVec[1];
}

//
// Normalize an operand's bitvec footprint based on its left bound
// and update the given bitset.
// If isSet is true, we set all bits that are covered by this operand.
// If isSet os false, we clear all bits that are covered by this operand.
//
void G4_Operand::updateFootPrint(BitSet& footprint, bool isSet, const IR_Builder& builder)
{
    unsigned N = NUM_BITS_PER_ELT;
    unsigned lb = getLeftBound();
    unsigned rb = getRightBound();
    const bool doFastPath = true; // for debugging

    if (doFastPath && lb % N == 0 && (rb + 1) % N == 0)
    {
        // lb is 32-byte aligned, set one dword at a time
        unsigned idx = lb / N;
        unsigned endIdx = rb / N;
        // get the precise footprint for the first two GRF
        for (int i = 0; i < 2 && idx <= endIdx; ++i, ++idx)
        {
            uint64_t bits = getBitVecL();
            uint32_t bitVal = (uint32_t)(i % 2 ? bits >> N : bits);
            if (isSet)
            {
                footprint.setElt(idx, bitVal);
            }
            else
            {
                footprint.resetElt(idx, bitVal);
            }
        }
        if (builder.getGRFSize() > 32)
        {
            for (int i = 0; i < 2 && idx <= endIdx; ++i, ++idx)
            {
                uint64_t bits = getBitVecH(builder);
                uint32_t bitVal = (uint32_t)(i % 2 ? bits >> N : bits);
                if (isSet)
                {
                    footprint.setElt(idx, bitVal);
                }
                else
                {
                    footprint.resetElt(idx, bitVal);
                }
            }
        }

        // beyond the first two GRF we assume every byte is touched
        while (idx <= endIdx)
        {
            if (isSet)
            {
                footprint.setElt(idx, 0xFFFFFFFF);
            }
            else
            {
                footprint.resetElt(idx, 0xFFFFFFFF);
            }
            idx++;
        }
    }
    else
    {
        // handle unaligned case
        uint64_t mask0 = getBitVecL();
        unsigned j = lb;
        for (unsigned i = 0; i < 64 && j <= rb; ++i, ++j)
        {
            if (mask0 & (1ULL << i))
                footprint.set(j, isSet);
        }
        if (builder.getGRFSize() > 32)
        {
            uint64_t mask1 = getBitVecH(builder);
            for (unsigned i = 0; i < 64 && j <= rb; ++i, ++j)
            {
                if (mask1 & (1ULL << i))
                    footprint.set(j, isSet);
            }
        }
        while (j++ <= rb)
            footprint.set(j, isSet);
    }
}

// update bit vector for this operand based on it size
// We assume all bytes are touched
void G4_Operand::setBitVecFromSize(uint32_t NBytes, const IR_Builder& builder)
{
    bitVec[0] = NBytes < 64 ? (1ULL << NBytes) - 1 : ULLONG_MAX;
    bitVec[1] = 0;
    if (builder.getGRFSize() > 32 && NBytes >= 64)
    {
        bitVec[1] = (NBytes < 64 * 2) ? (1ULL << (NBytes - 64)) - 1 : ULLONG_MAX;
    }
}

// Left and right bound for every operand is based off
// top most dcl.
// For flag register as dst/src/pred/cond mod, each bit of
// bitset represents corresponding bit of flag.
// For indirect access, right bound is set to sum of
// left bound and 15. The constant 15 is derived by the
// fact that address register is accessed as Type_UW which
// means 16 bits. right bound represents closed interval
// so 1 is subtracted.
// For direct access of GRF, each bit of bitset represents
// correcponding byte of operand.
void G4_INST::computeRightBound(G4_Operand* opnd)
{
    associateOpndWithInst(opnd, this);

    if (opnd &&
        opnd->isImm() == false &&
        opnd->isNullReg() == false)
    {
        bool done = false;

        if (done == false && op == G4_pln && opnd == srcs[1])
        {
            opnd->computeRightBound(execSize > g4::SIMD8 ? execSize : execSize * 2);
            if (execSize > g4::SIMD8)
            {
                opnd->setRightBound(opnd->right_bound * 2 - opnd->getLeftBound() + 1);
            }

            done = true;
        }
        else if (done == false && (isPseudoKill() || isPseudoUse()))
        {
            // pseudo kills/use write/read entire variable
            G4_Declare* topdcl = opnd->getBase()->asRegVar()->getDeclare()->getRootDeclare();
            opnd->setRightBound(topdcl->getByteSize() - 1);

            done = true;
        }
        else if (done == false && isFillIntrinsic())
        {
            asFillIntrinsic()->computeRightBound(opnd);
            done = true;
        }
        else if (done == false && isSpillIntrinsic())
        {
            asSpillIntrinsic()->computeRightBound(opnd);
            done = true;
        }

        if (done == false)
        {
            opnd->computeRightBound(execSize);

            if (getMaskOffset() > 0 &&
                ((opnd == getImplAccSrc()) ||
                (opnd == getImplAccDst())))
            {
                // for ARF (flag, acc) we have to adjust its bound based on the emask
                // We have to reset LB since the original instruction may have a non default emask
                opnd->setLeftBound(0);
                opnd->computeRightBound(execSize);
                unsigned int multiplicationFactor = 1;
                bool exceptionBoundsComputation = false;
                if (opnd->isAccReg())
                {
                    // Right bound granularity is in terms of
                    // bytes for Acc registers
                    multiplicationFactor = 4;
                }

                if (opnd == getImplAccDst() || opnd == getImplAccSrc())
                {
                    G4_Type extype;
                    int extypesize;
                    extype = getOpExecType(extypesize);

                    if ((IS_WTYPE(extype) || IS_DTYPE(extype)))
                    {
                        // This condition is a result of HW Conformity requirement
                        // that for exec type = D/DW, only acc0 is used even when
                        // qtr control is set to Q2/H2
                        opnd->setLeftBound(0);
                        opnd->setRightBound(31);
                        exceptionBoundsComputation = true;
                    }
                }

                if (exceptionBoundsComputation == false)
                {
                    // Update left/right bound as per inst mask offset
                    opnd->setLeftBound(opnd->left_bound + (getMaskOffset() * multiplicationFactor));
                    opnd->setRightBound(opnd->right_bound + (getMaskOffset () * multiplicationFactor));
                }
            }

            done = true;
        }
    }
}

void G4_InstSend::computeRightBound(G4_Operand* opnd)
{
    associateOpndWithInst(opnd, this);

    if (opnd && !opnd->isImm() && !opnd->isNullReg())
    {
        auto computeSendOperandBound = [this](G4_Operand* opnd, int numReg)
        {
            if (numReg == 0)
            {
                return;
            }

            // Sends read/write in units of GRF. With a narrower simd width,
            // the variable may have size smaller than one GRF, or smaller
            // the reponse or message length. In this case, limit the right
            // bound up to the variable size.
            unsigned LB = opnd->left_bound;
            unsigned RB = std::min(opnd->getTopDcl()->getByteSize(),
                LB + numReg * getBuilder().numEltPerGRF<Type_UB>()) - 1;

            unsigned NBytes = RB - LB + 1;
            opnd->setBitVecFromSize(NBytes, getBuilder());
            opnd->setRightBound(RB);
        };

        if (srcs[0] == opnd || (isSplitSend() && srcs[1] == opnd))
        {
            // For send instruction's msg operand rightbound depends
            // on msg descriptor
            uint16_t numReg = (srcs[0] == opnd) ?
                getMsgDesc()->getSrc0LenRegs() : getMsgDesc()->getSrc1LenRegs();
            computeSendOperandBound(opnd, numReg);
        }
        else if (dst == opnd)
        {
            // Compute right bound for dst operand
            const auto *desc = getMsgDesc();
            uint32_t dstBytes = desc->getDstLenBytes();
            if (dstBytes < getBuilder().getGRFSize()) {
                // e.g. OWord block read x1
                opnd->setBitVecL((1ULL << dstBytes) - 1);
                opnd->setRightBound(opnd->left_bound + dstBytes - 1);

            } else {
                uint16_t numReg = desc->getDstLenRegs();
                computeSendOperandBound(opnd, numReg);
            }
        }
        else
        {
            opnd->computeRightBound(execSize);
        }
    }

}

void G4_INST::computeARFRightBound()
{
    computeRightBound(predicate);
    computeRightBound(mod);
    computeRightBound(implAccSrc);
    computeRightBound(implAccDst);
}


// This function should only be invoked after computePReg() function
// has been invoked. The function computePReg() is invoked by computePhyReg()
// just before scheduling and post-RA.
// For GRF type variables this function returns linearized byte offset into
// GRF file. So if a variable is assigned r1 and its left bound is 0, this
// function will return (1 * 32) + 0 = 32.
// For non-GRF variables, GRF base offset value is 0 so value returned will
// be left bound.
// This function works for both, G4_SrcRegRegion as well as G4_DstRegRegion.
unsigned int G4_Operand::getLinearizedStart()
{
    unsigned linearizedStart = getLeftBound();
    G4_VarBase* base = getBase();

    if (base && base->isRegVar())
    {
        // LB is computed based on the root variable, so we have to go all the way up
        G4_Declare* dcl = base->asRegVar()->getDeclare();
        linearizedStart += dcl->getGRFBaseOffset();
        linearizedStart -= dcl->getOffsetFromBase();
    }

    return linearizedStart;
}

// Just like getLinearizedStart(), this function returns linearized byte
// offset of end of variable. For eg, if a variable is assigned r1 and
// region is type dword with inst exec size = 16, linearized end will be
// (63 - 0 + 32) = 95.
// Here, right bound is 63 since the region accesses 64 bytes,
// left bound is 0 since region access begins at byte 0,
// linearizedStart() will return 32 since r1 is allocated to the region.
// This function works for both, G4_SrcRegRegion as well as G4_DstRegRegion.
unsigned int G4_Operand::getLinearizedEnd()
{
    return (getRightBound() - getLeftBound() + getLinearizedStart());
}

void G4_Operand::dump() const
{
#if _DEBUG
    const_cast<G4_Operand *>(this)->emit(std::cerr, false);
#endif
}

void G4_INST::setPredicate(G4_Predicate* p)
{
    if (predicate && predicate->getInst() == this)
    {
        predicate->setInst(NULL);
    }

    predicate = p;

    associateOpndWithInst(p, this);
    computeRightBound(p);
}

void G4_INST::setSrc(G4_Operand* opnd, unsigned i)
{
    if (isPseudoAddrMovIntrinsic())
    {
        asIntrinsicInst()->setIntrinsicSrc(opnd, i);
        return;
    }

    MUST_BE_TRUE(i < G4_MAX_SRCS, ERROR_INTERNAL_ARGUMENT);

    if (srcs[i] != NULL)
    {
        if ((srcs[0] == srcs[i] && i != 0) ||
            (srcs[1] == srcs[i] && i != 1) ||
            (srcs[2] == srcs[i] && i != 2) ||
            (srcs[3] == srcs[i] && i != 3))
        {
            // opnd is present in some other
            // index of srcs so dont set its
            // inst to NULL
        }
        else
        {
            if (srcs[i]->getInst() == this)
            {
                srcs[i]->setInst(NULL);
            }
        }
    }

    srcs[i] = opnd;

    associateOpndWithInst(opnd, this);
    resetRightBound(opnd);
}

void G4_INST::setDest(G4_DstRegRegion* opnd)
{
    if (dst != NULL && dst->getInst() == this)
    {
        dst->setInst(NULL);
    }

    dst = opnd;

    associateOpndWithInst(opnd, this);
    resetRightBound(opnd);
}

void G4_INST::setCondMod(G4_CondMod* m)
{
    if (mod && mod->getInst() == this)
    {
        mod->setInst(NULL);
    }

    mod = m;

    associateOpndWithInst(m, this);
    computeRightBound(m);
}

void G4_INST::setImplAccSrc(G4_Operand* opnd)
{
    if (implAccSrc != NULL && implAccSrc->getInst() == this)
    {
        implAccSrc->setInst(NULL);
    }

    implAccSrc = opnd;

    associateOpndWithInst(opnd, this);
    computeRightBound(opnd);
}

void G4_INST::setImplAccDst(G4_DstRegRegion* opnd)
{
    if (implAccDst != NULL && implAccDst->getInst() == this)
    {
        implAccDst->setInst(NULL);
    }

    implAccDst = opnd;

    associateOpndWithInst(opnd, this);
    computeRightBound(opnd);
}

// get simd lane mask for this instruction. For example,
//      add  (8|M8) ...
// will have 0xFF00, which lane 8-15
unsigned G4_INST::getExecLaneMask() const
{
    unsigned maskbits = (unsigned)(((uint64_t)1 << getExecSize()) - 1);
    unsigned chanOffset = getMaskOffset();
    return (maskbits << chanOffset);
}

void G4_INST::print(std::ostream& OS) const
{
    G4_INST& inst = const_cast<G4_INST&>(*this);
    if (!inst.isLabel())
        OS << "\t";
    inst.emit(OS, false, false);
    OS << "\n";
}

void G4_INST::dump() const
{
    print(std::cerr);
}

bool G4_INST::canSupportSaturate() const
{
    if (op == G4_mul || op == G4_pseudo_mad)
    {
        for (int i = 0, numSrc = getNumSrc(); i < numSrc; ++i)
        {
            if (IS_DTYPE(getSrc(i)->getType()))
            {
                return false;
            }
        }
        return true;
    }

    if (isIntrinsic() || op == G4_mulh || op == G4_madw)
    {
        return false;
    }

    // note that IGA will return false for any opcode it does not recognize
    // If your psuedo opcode needs to support saturation you must add explicit check before this
    return InstSupportsSaturationIGA(getPlatform(), *this, builder);
}

bool G4_INST::canSupportCondMod() const
{
    if (!builder.hasCondModForTernary() && getNumSrc() == 3)
    {
        return false;
    }

    if (op == G4_mul)
    {
        // can't support conditional modifiers if source is DW and dst is not QW
        bool dwordSrc = false;
        for (int i = 0, numSrc = getNumSrc(); i < numSrc; ++i)
        {
            if (IS_DTYPE(getSrc(i)->getType()))
            {
                dwordSrc = true;
                break;
            }
        }
        if (dwordSrc && !IS_QTYPE(getDst()->getType()))
        {
            return false;
        }
        return true;
    }
    else if (op == G4_pseudo_mad)
    {
        // no cond mod for D * W
        G4_Operand* src0 = getSrc(0);
        G4_Operand* src1 = getSrc(1);
        if (IS_DTYPE(src0->getType()) || IS_DTYPE(src1->getType()))
        {
            return false;
        }
        return true;
    }

    if (op == G4_mov)
    {
        return dst->getType() != Type_BF && getSrc(0)->getType() != Type_BF;
    }

    // ToDo: replace with IGA model
    return ((op == G4_add) ||
        (op == G4_and) ||
        (op == G4_addc) ||
        (op == G4_asr) ||
        (op == G4_avg) ||
        (op == G4_dp2) ||
        (op == G4_dp3) ||
        (op == G4_dp4) ||
        (op == G4_dp4a) ||
        (op == G4_dph) ||
        (op == G4_dp4a) ||
        (op == G4_frc) ||
        (op == G4_line) ||
        (op == G4_lrp) ||
        (op == G4_lzd) ||
        (op == G4_mac) ||
        (op == G4_mach) ||
        (op == G4_mad) ||
        (op == G4_mov) ||
        (op == G4_mul) ||
        (op == G4_not) ||
        (op == G4_or) ||
        (op == G4_pln) ||
        (op == G4_rndd) ||
        (op == G4_rnde) ||
        (op == G4_rndu) ||
        (op == G4_rndz) ||
        (op == G4_sad2) ||
        (op == G4_sada2) ||
        (op == G4_shl) ||
        (op == G4_shr) ||
        (op == G4_subb) ||
        (op == G4_xor));
}

bool G4_INST::canSupportSrcModifier() const
{
    if (opcode() == G4_mov)
    {
        if (getDst()->getType() == Type_BF)
        {
            return false;
        }
    }

    if (opcode() == G4_pseudo_mad)
    {
        return true;
    }

    // note that IGA will return false for any opcode it does not recognize
    // If your psuedo opcode needs to support source modifier you must add
    // explicit check before this
    return InstSupportsSrcModifierIGA(getPlatform(), *this, builder);
}

// convert (execsize, offset) into emask option
// if no such mask option exists, return InstOpt_NoOpt
G4_InstOption G4_INST::offsetToMask(int execSize, int offset, bool nibOk)
{
    switch (execSize)
    {
    case 32:
        return InstOpt_M0;
    case 16:
        switch (offset)
        {
        case 0:
            return InstOpt_M0;
        case 16:
            return InstOpt_M16;
        default:
            return InstOpt_NoOpt;
        }
    case 8:
        switch (offset)
        {
        case 0:
            return InstOpt_M0;
        case 8:
            return InstOpt_M8;
        case 16:
            return InstOpt_M16;
        case 24:
            return InstOpt_M24;
        default:
            return InstOpt_NoOpt;
        }
    case 4:
        if (nibOk)
        {
            switch (offset)
            {
            case 0:
                return InstOpt_M0;
            case 4:
                return InstOpt_M4;
            case 8:
                return InstOpt_M8;
            case 12:
                return InstOpt_M12;
            case 16:
                return InstOpt_M16;
            case 20:
                return InstOpt_M20;
            case 24:
                return InstOpt_M24;
            case 28:
                return InstOpt_M28;
            default:
                return InstOpt_NoOpt;
            }
        }
        else
        {
            return InstOpt_NoOpt;
        }
    default:
        return InstOpt_NoOpt;
    }
}

void G4_DstRegRegion::setWriteMask(ChannelEnable wm)
{
    writeMask = wm;
}

void G4_SrcRegRegion::setSwizzle(const char* sw)
{
    MUST_BE_TRUE((int)strlen(sw) <  max_swizzle, ERROR_INTERNAL_ARGUMENT);
    strcpy_s(swizzle, max_swizzle, sw);
}

// convert contiguous regions to <N;N,1> form subject to the requirment
// that width is not used to cross GRF
// This is done because <1;1,0>/<2;2,1> require crossbar muxes and thus incur a performance penalty
// This should only be called after RA when we know the actual subreg offset
void G4_SrcRegRegion::rewriteContiguousRegion(IR_Builder& builder, uint16_t opNum)
{
    int execSize = inst->getExecSize();
    if (execSize == 1 || !desc->isContiguous(execSize))
    {
        return;
    }
    uint32_t eltSize = getTypeSize();
    uint32_t subRegOffset = getLinearizedStart() % builder.numEltPerGRF<Type_UB>();
    uint32_t endOffset = subRegOffset + inst->getExecSize() * eltSize;

    bool isAlign1Ternary = builder.hasAlign1Ternary() && inst->getNumSrc() == 3;

    if (builder.doNotRewriteContiguousRegion())
    {
        // 2-src and 3-src src0/1: normalize region to <1;1,0>
        // 3-src src2: normalize region to <2;2,1> since it only supports horz stride
        setRegion(builder, isAlign1Ternary && opNum == 2 ? builder.createRegionDesc(2, 2, 1) : builder.getRegionStride1(), true);
        return;
    }

    if (inst->getNumSrc() < 3)
    {
        // do <16;16,1> for HF/W if possible
        if (subRegOff == 0 && execSize == 16 && eltSize == 2)
        {
            setRegion(builder, builder.createRegionDesc(16, 16, 1), true);
            return;
        }
    }

    // Find a width that does not cross GRF from <8;8,1>, <4;4,1>, to <2;2,1>
    auto getWidth = [=, &builder](unsigned offset, unsigned eltSize) -> unsigned
    {
        unsigned Widths[] = { 8, 4, 2 };
        for (auto w : Widths)
        {
            if (w > inst->getExecSize())
                continue;

            if (w * eltSize > builder.numEltPerGRF<Type_UB>())
            {
                // <8;8,1> is not allowed for 64-bit type
                continue;
            }

            if (endOffset <= builder.numEltPerGRF<Type_UB>() ||
                subRegOffset % (w * eltSize) == 0)
            {
                return w;
            }
        }

        // width >= 2 crosses GRF
        return 0;
    };

    unsigned short w = (unsigned short)getWidth(subRegOffset, eltSize);

    if (builder.newTernaryStride() && isAlign1Ternary && (w == 2 || w == 0) && opNum != 2)
    {
        setRegion(builder, builder.getRegionStride1(), true);
        return;
    }

    if (w)
    {
        setRegion(builder, builder.createRegionDesc(w, w, 1), true);
    }
    else if (isAlign1Ternary)
    {
        // binary encoding asserts on <1;1,0> region for 3-src inst, so force change it to <2;2,1>
        setRegion(builder, builder.createRegionDesc(2, 2, 1), true);
    }
}

void LiveIntervalInfo::getLiveIntervals(std::vector<std::pair<uint32_t, uint32_t>>& intervals)
{
    for (auto&& it : liveIntervals)
    {
        intervals.push_back(it);
    }
}

void LiveIntervalInfo::addLiveInterval(uint32_t start, uint32_t end)
{
    if (liveIntervals.size() == 0)
    {
        liveIntervals.emplace_back(start, end);
    }
    else if (start - liveIntervals.back().second <= 1)
    {
        liveIntervals.back().second = end;
    }
    else if (liveIntervals.back().second < start)
    {
        liveIntervals.emplace_back(start, end);
    }
    else if (liveIntervals.front().first >= start && liveIntervals.back().second <= end)
    {
        liveIntervals.clear();
        liveIntervals.emplace_back(start, end);
    }
    else
    {
        bool inserted = false;
        uint32_t newEnd = end;
        for (auto liveIt = liveIntervals.begin(); liveIt != liveIntervals.end();)
        {
            auto& lr = (*liveIt);

            if (!inserted)
            {
                if (lr.first <= start && lr.second >= newEnd)
                {
                    inserted = true;
                    break;
                }
                else if (lr.first <= start && lr.second > start && lr.second <= newEnd)
                {
                    // Extend existing sub-interval
                    lr.second = newEnd;
                    inserted = true;
                    ++liveIt;
                    continue;
                }
                else if ((start - lr.second) <= 1u)
                {
                    lr.second = newEnd;
                    inserted = true;
                    ++liveIt;
                    continue;
                }
                else if (lr.first > start)
                {
                    // Insert new sub-interval
                    liveIntervals.insert(liveIt, std::make_pair(start, newEnd));
                    inserted = true;
                    continue;
                }
            }
            else
            {
                if (lr.first > newEnd)
                    break;
                else if (lr.first == newEnd)
                {
                    newEnd = lr.second;
                    auto newLRIt = liveIt;
                    --newLRIt;
                    (*newLRIt).second = newEnd;
                    liveIt = liveIntervals.erase(liveIt);
                    continue;
                }
                else if (lr.second <= newEnd)
                {
                    liveIt = liveIntervals.erase(liveIt);
                    continue;
                }
                else if(lr.first < newEnd && lr.second > newEnd)
                {
                    auto newLRIt = liveIt;
                    --newLRIt;
                    (*newLRIt).second = lr.second;
                    liveIntervals.erase(liveIt);
                    break;
                }
            }
            ++liveIt;
        }

        if (!inserted)
        {
            if (start - liveIntervals.back().second <= 1)
                liveIntervals.back().second = end;
            else
                liveIntervals.emplace_back(start, end);
        }
    }
}

void LiveIntervalInfo::liveAt(uint32_t cisaOff)
{
    if (cisaOff == UNMAPPABLE_VISA_INDEX)
        return;

    // Now iterate over all intervals and check which one should
    // be extended. If none, start a new one.
    bool added = false;
    auto prev = liveIntervals.begin();

    for (auto it = liveIntervals.begin(), itEnd = liveIntervals.end();
        it != itEnd;
        prev = it++)
    {
        auto& item = (*it);

        if (added)
        {
            // Check whether prev and current one can be merged
            if (((*prev).second == item.first) ||
                ((*prev).second == item.first - 1))
            {
                prev->second = item.second;
                it = liveIntervals.erase(it);
                break;
            }
            else
            {
                break;
            }
        }

        if (item.first == cisaOff + 1)
        {
            item.first = cisaOff;
            added = true;
            break;
        }

        if (item.second == cisaOff - 1)
        {
            item.second = cisaOff;
            added = true;
            continue;
        }

        if (!added &&
            item.first <= cisaOff &&
            item.second >= cisaOff)
        {
            added = true;
            break;
        }

        if (item.first > cisaOff)
        {
            liveIntervals.insert(it, std::make_pair(cisaOff, cisaOff));
            added = true;
            break;
        }
    }

    if (!added)
    {
        liveIntervals.push_back(std::make_pair(cisaOff, cisaOff));
    }
}

bool G4_INST::supportsNullDst() const
{
    if (isSend())
    {
        return true;
    }
    if (builder.getPlatform() >= Xe_PVC && dst->getTypeSize() == 1)
    {
        // null:b not supported
        return false;
    }
    return getNumSrc() != 3 && !(op == G4_pln && !builder.doPlane());
}

bool G4_INST::isAlign1Ternary() const
{
    return builder.hasAlign1Ternary() && getNumSrc() == 3 && !mayExceedTwoGRF();
}

// Detect packed low-precision instruction. This is used by the scheduler.
// - all src and dst are GRF and of :hf type and "packed".
// (src is also packed when it is replicated scalar).
// Two cases are possible:
// 1)   add (16)    r1.0<1>:hf   r2.0<8;8,1>:hf   r3.0<8;8,1>:hf    { Align1, H1 }
// 2)   add (16)    r1.0<1>:hf   r2.0<8;8,1>:hf   r3.0<0;1,0>:hf    { Align1, H1 }
bool G4_INST::isFastHFInstruction(void) const {
    if (getExecSize() < g4::SIMD16) {
        return false;
    }
    bool isHF = false;
    for (int op_i = 0, op_e = getNumSrc(); op_i < op_e; ++op_i) {
        G4_Operand *opnd = getSrc(op_i);
        if (! opnd) {
            continue;
        }
        if (!IS_HFTYPE(opnd->getType())) {
            return false;
        }
        if (opnd->isSrcRegRegion()) {
            G4_SrcRegRegion *srcRgn = opnd->asSrcRegRegion();
            if (! srcRgn->getRegion()->isContiguous(getExecSize())) {
                return false;
            }
        }
        isHF = true;
    }
    return isHF;
}


void G4_Declare::prepareForRealloc(G4_Kernel* kernel)
{
    // Reset allocated register if this dcl is not an input
    // or a pre-defined variable.
    auto& builder = kernel->fg.builder;

    setGRFBaseOffset(0);

    if (getRegFile() != G4_RegFileKind::G4_INPUT &&
        getRegVar()->isPhyRegAssigned() &&
        !getRegVar()->isAreg() &&
        this != builder->getBuiltinA0() &&
        this != builder->getBuiltinR0() &&
        this != builder->getBuiltinA0Dot2() &&
        this != builder->getBuiltinBindlessSampler() &&
        this != builder->getBuiltinHWTID() &&
        this != builder->getBuiltinT252() &&
        this != builder->getStackCallRet() &&
        this != builder->getStackCallArg() &&
        this != builder->getBEFP() &&
        this != builder->getBESP() &&
        this != kernel->fg.getScratchRegDcl() &&
        this != kernel->fg.getStackPtrDcl() &&
        this != kernel->fg.getFramePtrDcl())
    {
        getRegVar()->resetPhyReg();
        getRegVar()->setDisp(UINT_MAX);
    }
}

bool G4_INST::mayExpandToAccMacro() const
{
    auto isDMul = [](const G4_INST *Inst) {
        return Inst->opcode() == G4_mul && (IS_QTYPE(Inst->getDst()->getType()) ||
                                            (IS_DTYPE(Inst->getSrc(0)->getType()) &&
                                             IS_DTYPE(Inst->getSrc(1)->getType())));
    };

    auto mayBeMAC = [&](const G4_INST *Inst) {
       if (Inst->opcode() != G4_pseudo_mad)
           return false;
       if (IS_TYPE_FLOAT_ALL(Inst->getDst()->getType()) &&
           builder.getOption(vISA_forceFPMAD))
           return false;
       return true;
    };

    return opcode() == G4_mach ||
           opcode() == G4_mulh ||
           opcode() == G4_madw ||
           isDMul(this) ||
           mayBeMAC(this) ||
           (opcode() == G4_pln && !builder.doPlane());
}

bool G4_INST::canExecSizeBeAcc(Gen4_Operand_Number opndNum) const
{
    switch (dst->getType())
    {
    case Type_HF:
    case Type_BF:
        if (builder.relaxedACCRestrictions())
        {
            if (!((isMixedMode() && getExecSize() == g4::SIMD8) ||
                (getExecSize() == g4::SIMD16)))
            {
                return false;
            }
        }
        else
        {
            if (getExecSize() != G4_ExecSize(builder.getNativeExecSize() * 2))
            {
                return false;
            }
        }
        break;
    case Type_W:
    case Type_UW:
        if (getExecSize() != G4_ExecSize(builder.getNativeExecSize() * 2))
        {
            return false;
        }
        break;
    case Type_F:
        if (getExecSize() != G4_ExecSize(builder.getNativeExecSize() * 2) &&
            getExecSize() != builder.getNativeExecSize())
        {
            return false;
        }
        break;
    case Type_DF:
        if (!builder.useAccForDF())
        {
            return false;
        }
        if (getExecSize() != builder.getNativeExecSize() &&
            getExecSize() != G4_ExecSize(builder.getNativeExecSize() / 2))
        {
            return false;
        }
        break;
    case Type_D:
    case Type_UD:
        if (getExecSize() != builder.getNativeExecSize())
        {
            return false;
        }
        if (opndNum != Opnd_dst && isSignSensitive(opndNum))
        {
            return false;
        }
        break;
    default:
        return false;
    }

    return true;
}

// returns true if dst may be replaced by an explicit acc
// in addition to opcode-specific checks, we require
// -- dst must be GRF
// -- contiguous regions
// -- simd8 for D/UD, simd8/16 for F, simd16 for HF/W, other types not allowed
bool G4_INST::canDstBeAcc() const
{
    if (mayExpandToAccMacro())
    {
        // while this should not prevent dst from becoming acc (mul/plane macros use
        // acc as temp so should not affect final dst), later HW conformity is not equipped
        // to deal with such code so we disable the substitution
        return false;
    }

    if (dst == nullptr || dst->getTopDcl() == nullptr || dst->getHorzStride() != 1)
    {
        return false;
    }

    if (dst->getTopDcl()->getRegFile() != G4_GRF)
    {
        return false;
    }

    if (!builder.hasFP64Acc() && dst->getType() == Type_DF)
    {
        return false;
    }

    // src0 may not have indirect regioning
    if (!builder.accDstforIndirectSrc() && getSrc(0) && getSrc(0)->isSrcRegRegion())
    {
        auto src0Region = getSrc(0)->asSrcRegRegion();
        if (src0Region->getRegAccess() == IndirGRF)
        {
            return false;
        }
    }

    if (!canExecSizeBeAcc(Opnd_dst))
    {
        return false;
    }

    if (getSaturate() && IS_INT(dst->getType()))
    {
        return false;
    }

    if (!builder.relaxedACCRestrictions())
    {
        if (dst->getType() == builder.getMixModeType() && isMixedMode())
        {
            // acc can't be used as packed f16 for mix mode instruction as it doesn't support regioning
            return false;
        }
    }

    if (builder.avoidAccDstWithIndirectSource())
    {
        for (int i = 0, numSrc = getNumSrc(); i < numSrc; ++i)
        {
            bool indirectSrc = getSrc(i) && getSrc(i)->isSrcRegRegion() &&
                getSrc(i)->asSrcRegRegion()->getRegAccess() != Direct;
            if (indirectSrc)
            {
                return false;
            }
        }
    }

    if (isMath())
    {
        return builder.hasMathAcc();
    }
    switch (opcode())
    {
    case G4_add:
    case G4_and:
    case G4_asr:
    case G4_avg:
    case G4_frc:
    case G4_lzd:
    case G4_mul:
    case G4_not:
    case G4_or:
    case G4_rndd:
    case G4_rnde:
    case G4_rndu:
    case G4_rndz:
    case G4_shr:
    case G4_smov:
    case G4_xor:
    case G4_rol:
    case G4_ror:
        return true;
    case G4_sel:
        // sel seems to fail with int acc for some strange reason (sign extension?)
        return getCondMod() ? IS_TYPE_FLOAT_ALL(dst->getType()) : true;
    case G4_cmp:
    case G4_cmpn:
        // disable for now since it's causing some SKL tests to fail
        return false;
    case G4_mov:
        if (builder.hasFormatConversionACCRestrictions())
        {
            const bool allowedICombination = (IS_DTYPE(getSrc(0)->getType()) || getSrc(0)->getType() == Type_W || getSrc(0)->getType() == Type_UW) &&
                (IS_DTYPE(dst->getType()) || dst->getType() == Type_W || dst->getType() == Type_UW);
            const bool allowedFCombination = (getSrc(0)->getType() == Type_F || getSrc(0)->getType() == Type_HF) &&
                (dst->getType() == Type_F || dst->getType() == Type_HF);
            const bool allowedDFCombination = getSrc(0)->getType() == Type_DF &&
                dst->getType() == Type_DF;

            if (builder.restrictedACCRestrictions() && allowedFCombination)
            {
                uint16_t dstStride = dst->getHorzStride();
                uint16_t srcStride = 0;
                if (getSrc(0)->isSrcRegRegion())
                {
                    G4_SrcRegRegion* src = getSrc(0)->asSrcRegRegion();
                    const RegionDesc* region = src->getRegion();

                    if (!region->isSingleStride(execSize, srcStride))
                    {
                        return false;
                    }

                    //The bitmapping is model by the element size * element stride.
                    // No matter dst is float or half float.
                    // Pack and un-pack happen only in the destination register, so no matter dst is F or HF, it's not allowed to be replaced with ACC if bitmapping swizzles.
                    // If both dst and src are HF type, swizzle is not allowed as well.
                    //FIXME, mov (16|M0)   acc0.0<1>:f, r28<1;1,0>:hf can be passed in HW.
                    if ((dst->getType() != src->getType() || dst->getType() == Type_HF) &&
                        (dstStride * dst->getTypeSize() != srcStride * src->getTypeSize()))
                    {
                        return false;
                    }
                }
            }

            if (!allowedICombination && !allowedFCombination && !allowedDFCombination)
            {
                return false;
            }
        }
        return builder.relaxedACCRestrictions() || !getSrc(0)->isAccReg();
    case G4_pln:
        // we can't use acc if plane will be expanded
        return builder.doPlane();
    case G4_madm:
        return builder.useAccForMadm();
    case G4_mad:
    case G4_csel:
        return builder.canMadHaveAcc();
    case G4_dp4a:
        return builder.relaxedACCRestrictions2();
    case G4_bfn:
    case G4_add3:
        return true;
    default:
        return false;
    }
}

// returns true if src0 may be replaced by an explicit acc
// in addition to opcode-specific checks, we require
// -- contiguous regions
// -- simd8 for D/UD, simd8/16 for F, simd16 for HF/W, other types not allowed
bool G4_INST::canSrcBeAccBeforeHWConform(Gen4_Operand_Number opndNum) const
{
    int srcId = getSrcNum(opndNum);
    assert((srcId == 0 || srcId == 1 || srcId == 2) && "must be either src0, src1 or src2");

    if (!builder.relaxedACCRestrictions3() && srcId == 2)
    {
        return false;
    }

    if (getSrc(srcId) == nullptr || !getSrc(srcId)->isSrcRegRegion())
    {
        return false;
    }

    if (mayExpandToAccMacro())
    {
        return false;
    }

    G4_SrcRegRegion* src = getSrc(srcId)->asSrcRegRegion();
    if (srcId == 1 && src->hasModifier())
    {
        // some platforms allow float src1 acc modifiers,
        // while some don't allow src1 acc modifier at all.
        if (!IS_TYPE_FLOAT_ALL(src->getType()) || !builder.relaxedACCRestrictions())
        {
            return false;
        }
    }
    if (!src->getRegion()->isContiguous(getExecSize()))
    {
        return false;
    }

    if (builder.relaxedACCRestrictions() &&
        isMixedMode() &&
        isLowPrecisionFloatTy(src->getType()))
    {
        return false;
    }

    if (!canExecSizeBeAcc(opndNum))
    {
        return false;
    }

    if (opcode() == G4_mad && srcId == 0 &&
        !builder.canMadHaveSrc0Acc())
    {
        // mac's implicit acc gets its region from dst, so we have to check src and
        // dst have the same type
        if (src->getType() != dst->getType())
        {
            return false;
        }
    }

    if (IS_TYPE_FLOAT_ALL(src->getType()) ^ IS_TYPE_FLOAT_ALL(getDst()->getType()))
    {
        // no float <-> int conversion for acc source
        return false;
    }

    if (isMath())
    {
        return builder.hasMathAcc();
    }
    switch (opcode())
    {
    case G4_add:
    case G4_asr:
    case G4_avg:
    case G4_cmp:
    case G4_cmpn:
    case G4_frc:
    case G4_lzd:
    case G4_rndd:
    case G4_rnde:
    case G4_rndu:
    case G4_rndz:
    case G4_sel:
    case G4_shl:
    case G4_shr:
    case G4_smov:
    case G4_rol:
    case G4_ror:
        return true;
    case G4_mov:
        if (builder.hasFormatConversionACCRestrictions())
        {
            const bool allowedICombination = (IS_DTYPE(src->getType()) || src->getType() == Type_W || src->getType() == Type_UW) &&
                (IS_DTYPE(dst->getType()) || dst->getType() == Type_W || dst->getType() == Type_UW);
            const bool allowedFCombination = (src->getType() == Type_F || src->getType() == Type_HF) &&
                (dst->getType() == Type_F || dst->getType() == Type_HF);
            const bool allowedDFCombination = src->getType() == Type_DF &&
                dst->getType() == Type_DF;

            if (builder.restrictedACCRestrictions() && allowedFCombination)
            {
                uint16_t dstStride = dst->getHorzStride();
                uint16_t srcStride = 0;
                const RegionDesc* region = src->getRegion();

                if (!region->isSingleStride(execSize, srcStride))
                {
                    return false;
                }

                //The bitmapping is model by the element size * element stride.
                //When dst type is different with src type, or both are HF type.
                //FIXME, currently, r35 in following case cannot be replaced with acc
                // mov (16|M0) r35.0<1>:f r25.0<1;1,0>:f
                // mov (16|M0) r36.0<1>:hf r35.0<1;1,0>:f
                // the restriction may be relaxed after validation of HW team
                if ((dst->getType() != src->getType() || dst->getType() == Type_HF) &&
                    (dstStride * dst->getTypeSize() != srcStride * src->getTypeSize()))
                {
                    return false;
                }
            }

            if (!allowedICombination && !allowedFCombination && !allowedDFCombination)
            {
                return false;
            }
        }
        return builder.relaxedACCRestrictions() || !getDst()->isAccReg();
    case G4_madm:
        return builder.useAccForMadm();
    case G4_mad:
        // no int acc if it's used as mul operand
        return builder.canMadHaveAcc() &&
            ((srcId == 1 && (IS_FTYPE(src->getType()) || (src->getType() == Type_DF))) ||
                (srcId == 0 && src->getModifier() == Mod_src_undef) ||
                (srcId == 0 && builder.relaxedACCRestrictions_1()) ||
                (srcId == 2 && (IS_FTYPE(src->getType()) || (src->getType() == Type_DF))));
    case G4_csel:
        return builder.canMadHaveAcc();
    case G4_mul:
        return IS_TYPE_FLOAT_ALL(src->getType());
    case G4_and:
    case G4_not:
    case G4_or:
    case G4_xor:
        return src->getModifier() == Mod_src_undef;
    case G4_pln:
        return builder.doPlane() && src->getModifier() == Mod_src_undef;
    case G4_dp4a:
        if (builder.restrictedACCRestrictions())
        {
            return srcId == 0;
        }
        return builder.relaxedACCRestrictions2();
    case G4_bfn:
    case G4_add3:
        return true;
    default:
        return false;
    }
}

bool G4_INST::canSrcBeAccAfterHWConform(Gen4_Operand_Number opndNum) const
{
    int srcId = getSrcNum(opndNum);
    G4_SrcRegRegion* src = getSrc(srcId)->asSrcRegRegion();

    // dst must be GRF-aligned
    if ((getDst()->getLinearizedStart() % getBuilder().numEltPerGRF<Type_UB>()) != 0)
    {
        if (!(isMixedMode() && builder.getPlatform() == Xe_XeHPSDV))
            return false;
    }

    // check that src0 and dst have the same type/alignment
    auto dstEltSize = getDst()->getHorzStride() * getDst()->getTypeSize();
    if (dstEltSize > TypeSize(src->getType()))
    {
        return false;
    }
    else if (isLowPrecisionFloatTy(getDst()->getType()) && src->getType() == Type_F &&
        dstEltSize == 2)
    {
        if (builder.relaxedACCRestrictions())
        {
            //When source is float or half float from accumulator register and destination is half float with a stride of 1,
            //the source must register aligned. i.e., source must have offset zero.
            if ((src->getLinearizedStart() % getBuilder().numEltPerGRF<Type_UB>()) != 0)
            {
                return false;
            }
        }
        else
        {
            // no acc for mix mode inst with packed HF dst
            return false;
        }
    }

    return true;
}

bool G4_INST::canSrcBeAcc(Gen4_Operand_Number opndNum) const
{
    return canSrcBeAccBeforeHWConform(opndNum) && canSrcBeAccAfterHWConform(opndNum);
}

TARGET_PLATFORM G4_INST::getPlatform() const
{
    return builder.getPlatform();
}

G4_INST* G4_INST::cloneInst()
{
    // return nullptr if new derived class hasnt implemented
    // its own cloneInst()
    if (!isBaseInst() && !isCFInst())
        return nullptr;

    // Return a clone of current instruction.
    // This functionality is expected to be used by optimizations
    // such as rematerialization that require creating a copy
    // of instructions.
    auto nonConstBuilder = const_cast<IR_Builder*>(&builder);
    G4_INST* newInst = nullptr;
    auto prd = nonConstBuilder->duplicateOperand(getPredicate());
    auto condMod = nonConstBuilder->duplicateOperand(getCondMod());
    auto dst = nonConstBuilder->duplicateOperand(getDst());
    auto src0 = nonConstBuilder->duplicateOperand(getSrc(0));
    auto src1 = nonConstBuilder->duplicateOperand(getSrc(1));
    auto src2 = nonConstBuilder->duplicateOperand(getSrc(2));
    auto accSrc = nonConstBuilder->duplicateOperand(getImplAccSrc());
    auto accDst = nonConstBuilder->duplicateOperand(getImplAccDst());

    if (isSend())
    {
        MUST_BE_TRUE(false, "cloning send not yet supported");
    }
    else
    {
        newInst = nonConstBuilder->createInternalInst(prd, op, condMod, getSaturate(), getExecSize(),
            dst, src0, src1, option);

        if (src2)
            newInst->setSrc(src2, 2);

        if (accSrc)
            newInst->setImplAccSrc(accSrc);

        if (accDst)
            newInst->setImplAccDst(accDst);
    }

    return newInst;
}

G4_INST* G4_InstSend::cloneInst()
{
    auto nonConstBuilder = const_cast<IR_Builder*>(&builder);
    G4_INST* newInst = nullptr;
    auto prd = nonConstBuilder->duplicateOperand(getPredicate());
    auto dst = nonConstBuilder->duplicateOperand(getDst());
    auto src0 = nonConstBuilder->duplicateOperand(getSrc(0))->asSrcRegRegion();

    if (isSplitSend())
    {
        // desc -> src2, extDesc -> src3
        auto src1 = nonConstBuilder->duplicateOperand(getSrc(1))->asSrcRegRegion();
        auto desc = nonConstBuilder->duplicateOperand(getSrc(2));
        auto extDesc = nonConstBuilder->duplicateOperand(getSrc(3));
        newInst = nonConstBuilder->createInternalSplitSendInst(getExecSize(), dst, src0, src1, desc,
            getOption(), getMsgDescRaw(), extDesc);
        if (prd)
        {
            newInst->setPredicate(prd);
        }
    }
    else
    {
        auto desc = nonConstBuilder->duplicateOperand(getSrc(1));
        // desc -> src1, no extDesc (must be imm and stored in SendMsgDesc)
        newInst = nonConstBuilder->createInternalSendInst(prd, op, getExecSize(),
            dst, src0, desc, getOption(), getMsgDesc());
    }

    return newInst;
}

G4_InstIntrinsic::G4_InstIntrinsic(
    const IR_Builder& builder,
    G4_Predicate* prd,
    Intrinsic intrinId,
    G4_ExecSize execSize,
    G4_DstRegRegion* d,
    G4_Operand* s0,
    G4_Operand* s1,
    G4_Operand* s2,
    G4_Operand* s3,
    G4_Operand* s4,
    G4_Operand* s5,
    G4_Operand* s6,
    G4_Operand* s7,
    G4_InstOpts opt) :
    G4_INST(builder, prd, G4_intrinsic, nullptr, g4::NOSAT, execSize, d, nullptr, nullptr, nullptr, opt),
    intrinsicId(intrinId), tmpGRFStart(-1), tmpAddrStart(-1), tmpFlagStart(-1)
{
    srcs[0] = s0;
    srcs[1] = s1;
    srcs[2] = s2;
    srcs[3] = s3;
    srcs[4] = s4;
    srcs[5] = s5;
    srcs[6] = s6;
    srcs[7] = s7;

    resetRightBound(s0);
    resetRightBound(s1);
    resetRightBound(s2);
    resetRightBound(s3);
    resetRightBound(s4);
    resetRightBound(s5);
    resetRightBound(s6);
    resetRightBound(s7);

    associateOpndWithInst(s0, this);
    associateOpndWithInst(s1, this);
    associateOpndWithInst(s2, this);
    associateOpndWithInst(s3, this);
    associateOpndWithInst(s4, this);
    associateOpndWithInst(s5, this);
    associateOpndWithInst(s6, this);
    associateOpndWithInst(s7, this);
}

G4_Operand* G4_InstIntrinsic::getIntrinsicSrc(unsigned i) const
{
    MUST_BE_TRUE(i < G4_MAX_INTRINSIC_SRCS, ERROR_INTERNAL_ARGUMENT);
    return srcs[i];
}

G4_Operand* G4_InstIntrinsic::getOperand(Gen4_Operand_Number opnd_num) const
{
    switch (opnd_num) {
    case Opnd_src0: return srcs[0];
    case Opnd_src1: return srcs[1];
    case Opnd_src2: return srcs[2];
    case Opnd_src3: return srcs[3];
    case Opnd_src4: return srcs[4];
    case Opnd_src5: return srcs[5];
    case Opnd_src6: return srcs[6];
    case Opnd_src7: return srcs[7];
    default:
        MUST_BE_TRUE(0, "Operand number is out of range.");
        break;
    }
    return NULL;
}

void G4_InstIntrinsic::setIntrinsicSrc(G4_Operand* opnd, unsigned i)
{
    MUST_BE_TRUE(i < G4_MAX_INTRINSIC_SRCS, ERROR_INTERNAL_ARGUMENT);

    if (srcs[i] != NULL)
    {
        if (srcs[i]->getInst() == (G4_INST *)this)
        {
            srcs[i]->setInst(NULL);
        }
    }
    srcs[i] = opnd;

    associateOpndWithInst(opnd, (G4_INST*)this);
    resetRightBound(opnd);
}

G4_INST* G4_InstIntrinsic::cloneInst()
{
    auto nonConstBuilder = const_cast<IR_Builder*>(&builder);
    auto prd = nonConstBuilder->duplicateOperand(getPredicate());
    auto dst = nonConstBuilder->duplicateOperand(getDst());
    auto src0 = nonConstBuilder->duplicateOperand(getSrc(0));
    auto src1 = nonConstBuilder->duplicateOperand(getSrc(1));
    auto src2 = nonConstBuilder->duplicateOperand(getSrc(2));

    return nonConstBuilder->createInternalIntrinsicInst(prd, getIntrinsicId(), getExecSize(), dst,
        src0, src1, src2, option);
}

static void computeSpillFillOperandBound(G4_Operand* opnd, unsigned int LB, int numReg, const IR_Builder& builder)
{
    if (numReg == 0)
    {
        return;
    }

    // read/write in units of GRF.
    unsigned RB = std::min(opnd->getTopDcl()->getByteSize(),
        LB + numReg * builder.numEltPerGRF<Type_UB>()) - 1;

    unsigned NBytes = RB - LB + 1;
    opnd->setBitVecFromSize(NBytes, builder);
    opnd->setRightBound(RB);
}

uint32_t G4_SpillIntrinsic::getOffsetInBytes() const
{
    return offset * getBuilder().getGRFSize();
}

void G4_SpillIntrinsic::computeRightBound(G4_Operand* opnd)
{
    if (opnd) {
        uint16_t numReg = 0;
        if (opnd == getSrc(1))
        {
            numReg = asSpillIntrinsic()->getNumRows();
        }
        else if (opnd->isSrcRegRegion() && opnd == getSrc(0))
        {
            numReg = 1;
        }
        computeSpillFillOperandBound(opnd, opnd->left_bound, numReg, getBuilder());
    }
}

uint32_t G4_FillIntrinsic::getOffsetInBytes() const
{
    return offset * getBuilder().getGRFSize();
}

void G4_FillIntrinsic::computeRightBound(G4_Operand* opnd)
{
    if (opnd) {
        uint16_t numReg = 0;
        if (opnd == getDst()) {
            numReg = asFillIntrinsic()->getNumRows();
        } else if (opnd->isSrcRegRegion() &&
                   (opnd == getSrc(0) || opnd == getSrc(1))) {
            numReg = 1;
        }
        computeSpillFillOperandBound(opnd, opnd->left_bound, numReg, getBuilder());
    }
}

bool RegionDesc::isLegal(unsigned vs, unsigned w, unsigned hs)
{
    auto isPositiveAndLegal = [](unsigned val, unsigned high) {
        if (val == UNDEFINED_SHORT)
            return true;
        if (val > high || val == 0)
            return false;
        return ((val - 1) & val) == 0;
    };
    return isPositiveAndLegal(w, 16) &&
            (vs == 0 || isPositiveAndLegal(vs, 32)) &&
            (hs == 0 || isPositiveAndLegal(hs, 16));
}

RegionDesc::RegionDescKind RegionDesc::getRegionDescKind(
    uint16_t size, uint16_t vstride,
    uint16_t width, uint16_t hstride)
{
    // Skip special cases.
    if (vstride == UNDEFINED_SHORT || width == UNDEFINED_SHORT ||
        hstride == UNDEFINED_SHORT)
        return RK_Other;

    // <0;1,0>
    if (size == 1 || (vstride == 0 && hstride == 0) ||
        (vstride == 0 && width == 1))
        return RK_Stride0;

    // <1;1,0>
    if ((vstride == 1 && width == 1) || (size <= width && hstride == 1) ||
        (vstride == width && hstride == 1))
        return RK_Stride1;

    // <N;1,0>
    uint16_t stride = 0;
    if (vstride == width * hstride || width == size)
    {
        stride = hstride;
    }
    else if (width == 1 && hstride == 0)
    {
        stride = vstride;
    }

    return (stride == 2) ? RK_Stride2 : (stride == 4) ? RK_Stride4
                                                      : RK_Other;
}

bool RegionDesc::isContiguous(unsigned ExSize) const
{
    if (vertStride == 1 && width == 1)
        return true;
    if (vertStride == width && horzStride == 1)
        return true;

    return (ExSize == 1) ||
            (ExSize <= (unsigned)width && horzStride == 1);
}
bool RegionDesc::isSingleNonUnitStride(uint32_t execSize, uint16_t& stride) const
{
    if (isScalar() || isContiguous(execSize))
    {
        return false;
    }

    if (vertStride == width * horzStride || width == execSize)
    {
        stride = horzStride;
        return true;
    }

    if (horzStride == 0 && width == 1)
    {
        stride = vertStride;
        return true;
    }

    return false;
}

bool RegionDesc::isSingleStride(uint32_t execSize, uint16_t &stride) const
{
    if (isScalar())
    {
        stride = 0;
        return true;
    }
    if (isContiguous(execSize))
    {
        stride = 1;
        return true;
    }

    return isSingleNonUnitStride(execSize, stride);
}

G4_INST* G4_InstMath::cloneInst()
{
    auto nonConstBuilder = const_cast<IR_Builder*>(&builder);
    auto prd = nonConstBuilder->duplicateOperand(getPredicate());
    auto dst = nonConstBuilder->duplicateOperand(getDst());
    auto src0 = nonConstBuilder->duplicateOperand(getSrc(0));
    auto src1 = nonConstBuilder->duplicateOperand(getSrc(1));

    return nonConstBuilder->createInternalMathInst(
        prd, getSaturate(), getExecSize(),
        dst, src0, src1, getMathCtrl(), option);
}

G4_INST* G4_InstBfn::cloneInst()
{
    auto nonConstBuilder = const_cast<IR_Builder*>(&builder);
    auto prd = nonConstBuilder->duplicateOperand(getPredicate());
    auto condMod = nonConstBuilder->duplicateOperand(getCondMod());
    auto dst = nonConstBuilder->duplicateOperand(getDst());
    auto src0 = nonConstBuilder->duplicateOperand(getSrc(0));
    auto src1 = nonConstBuilder->duplicateOperand(getSrc(1));
    auto src2 = nonConstBuilder->duplicateOperand(getSrc(2));
    return nonConstBuilder->createInternalBfnInst(
        getBooleanFuncCtrl(), prd, condMod, getSaturate(), getExecSize(),
        dst, src0, src1, src2, option);
}

G4_INST* G4_InstDpas::cloneInst()
{
    auto nonConstBuilder = const_cast<IR_Builder*>(&builder);
    auto dst = nonConstBuilder->duplicateOperand(getDst());
    auto src0 = nonConstBuilder->duplicateOperand(getSrc(0));
    auto src1 = nonConstBuilder->duplicateOperand(getSrc(1));
    auto src2 = nonConstBuilder->duplicateOperand(getSrc(2));
    auto src3 = nonConstBuilder->duplicateOperand(getSrc(3));
    return nonConstBuilder->createInternalDpasInst(
        op, getExecSize(),
        dst, src0, src1, src2, src3, option,
        getSrc2Precision(), getSrc1Precision(), getSystolicDepth(), getRepeatCount());
}

bool G4_InstDpas::isInt() const
{
    // Check Src1 is enough.
    switch (Src1Precision)
    {
    case GenPrecision::S8:
    case GenPrecision::U8:
    case GenPrecision::S4:
    case GenPrecision::U4:
    case GenPrecision::S2:
    case GenPrecision::U2:
        return true;
    default:
        break;
    }
    return false;
}

bool G4_InstDpas::is2xInt8() const
{
    if ((Src1Precision == GenPrecision::S4 || Src1Precision == GenPrecision::U4 ||
         Src1Precision == GenPrecision::S2 || Src1Precision == GenPrecision::U2)
        &&
        (Src2Precision == GenPrecision::S4 || Src2Precision == GenPrecision::U4 ||
         Src2Precision == GenPrecision::S2 || Src2Precision == GenPrecision::U2))
    {
        return true;
    }
    return false;
}

uint8_t G4_InstDpas::getOpsPerChan() const
{
    if (isBF16() || isFP16())
        return OPS_PER_CHAN_2;
    else if (isTF32())
        return OPS_PER_CHAN_1;
    else if (is2xInt8())
        return OPS_PER_CHAN_8;
    // int8
    return OPS_PER_CHAN_4;
}

void G4_InstDpas::computeRightBound(G4_Operand* opnd)
{
    associateOpndWithInst(opnd, this);
    if (opnd && !opnd->isImm() && !opnd->isNullReg())
    {
        G4_InstDpas* dpasInst = asDpasInst();
        uint8_t D = dpasInst->getSystolicDepth();
        uint8_t C = dpasInst->getRepeatCount();

        auto computeDpasOperandBound = [this](G4_Operand* opnd, unsigned leftBound, unsigned rightBound)
        {
            unsigned NBytes = rightBound - leftBound + 1;
            opnd->setBitVecFromSize(NBytes, getBuilder());
            opnd->setRightBound(rightBound);
        };

        if (opnd == dst || (opnd == srcs[0] && !opnd->isNullReg()))
        {
            // dst and src0 are always packed, and RB is exec_size * type_size * rep_count
            auto opndSize = dpasInst->getExecSize() * opnd->getTypeSize() * C;
            computeDpasOperandBound(opnd, opnd->left_bound, opnd->left_bound + opndSize - 1);
        }
        else if (opnd == srcs[1])
        {
            uint32_t bytesPerLane = dpasInst->getSrc1SizePerLaneInByte();
            uint8_t src1_D = D;

            // Each lanes needs (src1_D * bytesPerLane) bytes, and it's multiple of DW!
            uint32_t bytesPerLaneForAllDepth = bytesPerLane * src1_D;
            bytesPerLaneForAllDepth = ((bytesPerLaneForAllDepth + 3) / 4) * 4;

            uint32_t bytes = bytesPerLaneForAllDepth * dpasInst->getExecSize();
            computeDpasOperandBound(opnd, opnd->left_bound, opnd->left_bound + bytes - 1);
        }
        else if (opnd == srcs[2])
        {
            // src2 is uniform.
            uint32_t bytesPerLane = dpasInst->getSrc2SizePerLaneInByte();
            uint32_t bytes = bytesPerLane * D * C;
            if (op == G4_dpasw) {
                bytes = bytesPerLane * D * ((C + 1) / 2);
            }
            computeDpasOperandBound(opnd, opnd->left_bound, opnd->left_bound + bytes - 1);
        }
    }
}

void G4_INST::inheritDIFrom(const G4_INST* inst)
{
    // Copy over debug info from inst
    setLocation(inst->getLocation());
    setCISAOff(getCISAOff() == UndefinedCisaOffset ? inst->getCISAOff() : getCISAOff());
}

void G4_INST::inheritSWSBFrom(const G4_INST* inst)
{
    // Copy the SWSB info
    setDistance(inst->getDistance());
    setLexicalId(inst->getLexicalId());

    setDistanceTypeXe(inst->getDistanceTypeXe());
    unsigned short token = inst->getToken();
    setToken(token);
    SWSBTokenType type = inst->getTokenType();
    setTokenType(type);
}

