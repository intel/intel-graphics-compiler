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


#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <list>

#include "visa_igc_common_header.h"
#include "Common_ISA_util.h"
#include "Common_ISA_framework.h"
#include "JitterDataStruct.h"
#include "BuildIR.h"
#include "common.h"
#include "Timer.h"

using namespace vISA;
//
// look up an imm operand
//
G4_Imm* OperandHashTable::lookupImm(int64_t imm, G4_Type ty)
{
    ImmKey key(imm, ty);
    auto iter = immTable.find(key);
    return iter != immTable.end() ? iter->second : nullptr;
}

//
// look up label operand
//
G4_Label* OperandHashTable::lookupLabel(const char* lab)
{
    auto iter = labelTable.find(lab);
    return iter != labelTable.end() ? iter->second : nullptr;
}

//
// create a dst reg region
//
G4_Imm* OperandHashTable::createImm(int64_t imm, G4_Type ty)
{
    G4_Imm* i = new (mem)G4_Imm(imm, ty);
    ImmKey key(imm, ty);
    immTable[key] = i;
    return i;
}

//
// create a label operand
//
G4_Label* OperandHashTable::createLabel(const char* lab)
{
    //
    // create a new string (new_str) and copy lab to new_str
    //
    size_t len = strlen(lab) + 1;
    char* new_str = (char*)mem.alloc(len);  // +1 for null that ends the string
    strcpy_s(new_str, len, lab);
    G4_Label* l = new (mem) G4_Label(new_str);
    labelTable[new_str] = l;
    return l;
}

//
// create the region <vstride; width, hstride> if not yet created
//
const RegionDesc* RegionPool::createRegion(
    uint16_t vstride, uint16_t width, uint16_t hstride)
{

    for (unsigned i = 0, size = (unsigned)rgnlist.size(); i < size; i++)
    {
        RegionDesc* region = rgnlist[i];
        if (region->vertStride == vstride &&
            region->width == width &&
            region->horzStride == hstride)
        {
            return region; // exist
        }
    }
    //
    // create one
    //
    RegionDesc* rd = new (mem) RegionDesc(vstride, width, hstride);
    rgnlist.push_back(rd);
    return rd;
}

/*
    Used in IR_Builder::translateVISARawSendInst. All the bits in des and extDesc are already set.
*/
G4_SendMsgDescriptor* IR_Builder::createGeneralMsgDesc(
    uint32_t desc,
    uint32_t extDesc,
    SendAccess access,
    G4_Operand* bti,
    G4_Operand* sti)
{
    return new (mem) G4_SendMsgDescriptor(desc, extDesc, access, bti, sti);
}

G4_SendMsgDescriptor* IR_Builder::createSendMsgDesc(
    SFID sfid,
    uint32_t desc,
    uint32_t extDesc,
    int src1Len,
    SendAccess access,
    G4_Operand *bti,
    bool isValidFuncCtrl)
{
    return new (mem) G4_SendMsgDescriptor(sfid, desc, extDesc, src1Len, access, bti, isValidFuncCtrl);
}

G4_SendMsgDescriptor* IR_Builder::createSendMsgDesc(
    unsigned funcCtrl,
    unsigned regs2rcv,
    unsigned regs2snd,
    SFID funcID,
    unsigned extMsgLength,
    uint16_t extFuncCtrl,
    SendAccess access,
    G4_Operand *bti,
    G4_Operand *sti)
{
    G4_SendMsgDescriptor* msgDesc = new (mem) G4_SendMsgDescriptor(
        funcCtrl, regs2rcv, regs2snd, funcID, (uint16_t) extMsgLength,
        extFuncCtrl, access, bti, sti, *this);
    return msgDesc;
}

// shorthand for read msg desc. Note that extDesc still needs to be explicitly created,
// SendMsgDesc ctor does not program all the bits
G4_SendMsgDescriptor* IR_Builder::createReadMsgDesc(SFID sfid,
    uint32_t desc,
    G4_Operand* bti)
{
    //ToDo: move extDesc into SendMsgDesc ctor
    uint32_t extDesc = G4_SendMsgDescriptor::createExtDesc(sfid);
    return new (mem) G4_SendMsgDescriptor(sfid, desc, extDesc, 0, SendAccess::READ_ONLY, bti, true);
}

G4_SendMsgDescriptor* IR_Builder::createWriteMsgDesc(SFID sfid,
    uint32_t desc,
    int src1Len,
    G4_Operand* bti)
{
    //ToDo: move extDesc into SendMsgDesc ctor
    uint32_t extDesc = G4_SendMsgDescriptor::createExtDesc(sfid, false, src1Len);
    return new (mem) G4_SendMsgDescriptor(sfid, desc, extDesc, src1Len, SendAccess::WRITE_ONLY, bti, true);
}

G4_SendMsgDescriptor* IR_Builder::createSyncMsgDesc(SFID sfid, uint32_t desc)
{
    //ToDo: move extDesc into SendMsgDesc ctor
    uint32_t extDesc = G4_SendMsgDescriptor::createExtDesc(sfid);
    return new (mem) G4_SendMsgDescriptor(sfid, desc, extDesc, 0, SendAccess::READ_WRITE, nullptr, true);
}

G4_SendMsgDescriptor* IR_Builder::createSampleMsgDesc(
    uint32_t desc,
    bool cps,
    int src1Len,
    G4_Operand* bti,
    G4_Operand* sti)
{
#define CPS_LOD_COMPENSATION_ENABLE 11

    uint32_t extDesc = G4_SendMsgDescriptor::createExtDesc(SFID::SAMPLER, false, src1Len);
    if (cps)
    {
        extDesc |= 1 << CPS_LOD_COMPENSATION_ENABLE;
    }
    return new (mem) G4_SendMsgDescriptor(desc, extDesc, SendAccess::READ_ONLY, bti, sti);
}

G4_Operand* IR_Builder::emitSampleIndexGE16(
    G4_Operand* sampler,
    G4_Declare* headerDecl)
{
    G4_Operand* samplerIdx;

    G4_Declare* t0
        = createTempVar(1, Type_UD, Any);
    G4_DstRegRegion* t0Dst
        = Create_Dst_Opnd_From_Dcl(t0, 1);
    G4_SrcRegRegion* t0Src
        = Create_Src_Opnd_From_Dcl(t0, getRegionScalar());

    G4_Declare* baseAdj
        = createTempVar(1, Type_UD, Any);
    G4_DstRegRegion* baseAdjDst
        = Create_Dst_Opnd_From_Dcl(baseAdj, 1);
    G4_SrcRegRegion* baseAdjSrc
        = Create_Src_Opnd_From_Dcl(baseAdj, getRegionScalar());

    G4_Declare* idxLow
        = createTempVar(1, Type_UD, Any);
    G4_DstRegRegion* idxLowDst
        = Create_Dst_Opnd_From_Dcl(idxLow, 1);
    G4_SrcRegRegion* idxLowSrc
        = Create_Src_Opnd_From_Dcl(idxLow, getRegionScalar());

    // calculate the sampler state base pointer offset based on
    // sample index, for putting to msg header M0.3
    createBinOp(G4_shr, 1,
        t0Dst, sampler, createImm(4, Type_UD),
        InstOpt_WriteEnable, true);
    createBinOp(G4_shl, 1,
        baseAdjDst, t0Src, createImm(8, Type_UD),
        InstOpt_WriteEnable, true);

    // get low 4 bits of sample index for putting into msg descriptor
    G4_SrcRegRegion* sampler2Src
        = createSrcRegRegion(Mod_src_undef, Direct,
        sampler->getTopDcl()->getRegVar(), 0, 0, getRegionScalar(), Type_UD);
    createBinOp(G4_and, 1,
        idxLowDst, sampler2Src, createImm(0xf, Type_UD),
        InstOpt_WriteEnable, true);
    samplerIdx = idxLowSrc;

    // add the base pointer offset with r0.3 and put to M0.3
    G4_DstRegRegion* stateBaseRgn
        = createDst(headerDecl->getRegVar(),
            0, 3, 1, Type_UD);
    G4_SrcRegRegion* src0
        = createSrcRegRegion(Mod_src_undef, Direct,
            builtinR0->getRegVar(), 0, 3, getRegionScalar(), Type_UD);
    createBinOp(G4_add, 1, stateBaseRgn,
        src0, baseAdjSrc, InstOpt_WriteEnable, true);

    return samplerIdx;
}

G4_INST* IR_Builder::createInst(G4_Predicate* prd,
                                G4_opcode op,
                                G4_CondMod* mod,
                                bool sat,
                                unsigned char size,
                                G4_DstRegRegion* dst,
                                G4_Operand* src0,
                                G4_Operand* src1,
                                unsigned int option,
                                bool addToInstList)
{
    return createInst(prd, op, mod, sat, size, dst, src0, src1, option, 0, addToInstList);
}
G4_INST* IR_Builder::createInst(G4_Predicate* prd,
                                G4_opcode op,
                                G4_CondMod* mod,
                                bool sat,
                                unsigned char size,
                                G4_DstRegRegion* dst,
                                G4_Operand* src0,
                                G4_Operand* src1,
                                unsigned int option,
                                int lineno,
                                bool addToInstList)
{
    MUST_BE_TRUE(op != G4_math, "IR_Builder::createInst should not be used to create math instructions");
    G4_INST* i = NULL;

    // ToDo: have separate functions to create call/jmp/ret
    if (G4_Inst_Table[op].instType == InstTypeFlow)
    {
        i = new (mem)G4_InstCF(*this, prd, op, mod, sat, size, dst, src0, option);
    }
    else
    {
        i = new (mem)G4_INST(*this, prd, op, mod, sat, size, dst, src0, src1, option);
    }

    if (addToInstList)
    {
        i->setCISAOff(curCISAOffset);

        if (m_options->getOption(vISA_EmitLocation))
        {
            i->setLocation(new (mem) MDLocation(lineno == 0 ? curLine : lineno, curFile));
        }

        instList.push_back(i);
    }

    instAllocList.push_back(i);

    return i;
}

// same as above, except we don't add it to the Builder's instList
G4_INST* IR_Builder::createInternalInst(G4_Predicate* prd,
                                        G4_opcode op,
                                        G4_CondMod* mod,
                                        bool sat,
                                        unsigned char size,
                                        G4_DstRegRegion* dst,
                                        G4_Operand* src0,
                                        G4_Operand* src1,
                                        unsigned int option)
{
    return createInternalInst(prd, op, mod, sat, size, dst, src0, src1, option, 0, UNMAPPABLE_VISA_INDEX, NULL);
}

G4_INST* IR_Builder::createInternalInst(G4_Predicate* prd,
                                        G4_opcode op,
                                        G4_CondMod* mod,
                                        bool sat,
                                        unsigned char size,
                                        G4_DstRegRegion* dst,
                                        G4_Operand* src0,
                                        G4_Operand* src1,
                                        unsigned int option,
                                        int lineno, int CISAoff,
                                        const char* srcFilename)
{
    MUST_BE_TRUE(op != G4_math, "IR_Builder::createInternalInst should not be used to create math instructions");

    auto ii = createInst(prd, op, mod, sat, size, dst, src0, src1, option, false);

    ii->setCISAOff(CISAoff);

    if (m_options->getOption(vISA_EmitLocation))
    {
        ii->setLocation(new (mem) MDLocation(lineno, curFile));
    }

    return ii;
}

G4_INST* IR_Builder::createNop(uint32_t instOpt)
{
    return createInternalInst(nullptr, G4_nop, nullptr, false, 1, nullptr, nullptr, nullptr, instOpt);
}

// sync inst are always internal, so no option to append it to instList. Also currently don't take any InstOpt
G4_INST* IR_Builder::createSync(G4_opcode syncOp, G4_Operand* src)
{
    assert(G4_INST::isSyncOpcode(syncOp) && "expect a sync op");
    return createInternalInst(nullptr, syncOp, nullptr, false, 1, nullptr, src, nullptr, InstOpt_NoOpt);
}

G4_INST* IR_Builder::createMov(uint8_t execSize, G4_DstRegRegion* dst,
    G4_Operand* src0, uint32_t option, bool appendToInstList,
    int lineno, int CISAoff, const char* srcFilename)
{
    G4_INST* newInst = nullptr;
    if (appendToInstList)
    {
        newInst = createInst(nullptr, G4_mov, nullptr, false, execSize, dst, src0, nullptr, option);
    }
    else
    {
        newInst = createInternalInst(nullptr, G4_mov, nullptr, false, execSize, dst, src0, nullptr, option);
    }
    newInst->setLineNo(lineno);
    newInst->setCISAOff(CISAoff);
    newInst->setSrcFilename(srcFilename);
    return newInst;
}

G4_INST* IR_Builder::createBinOp(G4_opcode op, uint8_t execSize, G4_DstRegRegion* dst,
    G4_Operand* src0, G4_Operand* src1, uint32_t option, bool appendToInstList)
{
    if (appendToInstList)
    {
        return createInst(nullptr, op, nullptr, false, execSize, dst, src0, src1, option);
    }
    else
    {
        return createInternalInst(nullptr, op, nullptr, false, execSize, dst, src0, src1, option);
    }
}

G4_INST* IR_Builder::createIf(G4_Predicate* prd, uint8_t size, uint32_t option)
{
    auto inst = createCFInst(prd, G4_if, size, nullptr, nullptr, option);
    return inst;
}

G4_INST* IR_Builder::createElse(uint8_t size, uint32_t option)
{
    auto inst = createCFInst(nullptr, G4_else, size, nullptr, nullptr, option);
    return inst;
}

G4_INST* IR_Builder::createEndif(uint8_t size, uint32_t option)
{
    auto inst = createCFInst(nullptr, G4_endif, size, nullptr, nullptr, option);
    return inst;
}

G4_INST* IR_Builder::createLabelInst(G4_Label* label, bool appendToInstList)
{
    if (appendToInstList)
    {
        return createInst(nullptr, G4_label, nullptr, false, UNDEFINED_EXEC_SIZE, nullptr, label, nullptr, 0, 0);
    }
    else
    {
        return createInternalInst(nullptr, G4_label, nullptr, false, UNDEFINED_EXEC_SIZE, nullptr, label, nullptr, 0, 0);
    }
}

// jmpTarget may be either a label (direct jmp) or scalar operand (indirect jmp)
G4_INST* IR_Builder::createJmp(G4_Predicate* pred, G4_Operand* jmpTarget, uint32_t option, bool appendToInstList)
{
    if (appendToInstList)
    {
        return createInst(pred, G4_jmpi, nullptr, false, 1, nullptr, jmpTarget, nullptr, option);
    }
    else
    {
        return createInternalInst(pred, G4_jmpi, nullptr, false, 1, nullptr, jmpTarget, nullptr, option);
    }
}

G4_INST* IR_Builder::createInternalCFInst(
    G4_Predicate* prd,
    G4_opcode op,
    unsigned char size,
    G4_Label* jip,
    G4_Label* uip,
    unsigned int option,
    int lineno,
    int CISAoff,
    const char* srcFilename)
{
    MUST_BE_TRUE(G4_Inst_Table[op].instType == InstTypeFlow,
        "IR_Builder::createInternalCFInst must be used with InstTypeFlow instruction class");

    auto ii = createCFInst(prd, op, size, jip, uip, option, lineno, false);

    ii->setCISAOff(CISAoff);

    if (m_options->getOption(vISA_EmitLocation))
    {
        ii->setLocation(new (mem) MDLocation(lineno, srcFilename));
    }

    return ii;
}

G4_INST* IR_Builder::createCFInst(
    G4_Predicate* prd,
    G4_opcode op,
    unsigned char size,
    G4_Label* jip,
    G4_Label* uip,
    unsigned int option,
    int lineno,
    bool addToInstList)
{
    MUST_BE_TRUE(G4_Inst_Table[op].instType == InstTypeFlow,
        "IR_Builder::createCFInst must be used with InstTypeFlow instruction class");

    G4_InstCF* ii = new (mem)G4_InstCF(*this, prd, op, size, jip, uip, option);

    if (addToInstList)
    {
        ii->setCISAOff(curCISAOffset);

        if (m_options->getOption(vISA_EmitLocation))
        {
            ii->setLocation(new (mem) MDLocation(lineno == 0 ? curLine : lineno, curFile));
        }
        instList.push_back(ii);
    }

    instAllocList.push_back(ii);

    return ii;
}


G4_INST* IR_Builder::createInst(G4_Predicate* prd,
                                G4_opcode op,
                                G4_CondMod* mod,
                                bool sat,
                                unsigned char size,
                                G4_DstRegRegion* dst,
                                G4_Operand* src0,
                                G4_Operand* src1,
                                G4_Operand* src2,
                                unsigned int option,
                                bool addToInstList)
{
    return createInst(prd, op, mod, sat, size, dst, src0, src1, src2, option, 0);
}
G4_INST* IR_Builder::createInst(G4_Predicate* prd,
                                G4_opcode op,
                                G4_CondMod* mod,
                                bool sat,
                                unsigned char size,
                                G4_DstRegRegion* dst,
                                G4_Operand* src0,
                                G4_Operand* src1,
                                G4_Operand* src2,
                                unsigned int option,
                                int lineno,
                                bool addToInstList)
{
    MUST_BE_TRUE(op != G4_math && G4_Inst_Table[op].instType != InstTypeFlow,
        "IR_Builder::createInst should not be used to create math/CF instructions");

    G4_INST* i = NULL;

    i = new (mem)G4_INST(*this, prd, op, mod, sat, size, dst, src0, src1, src2, option);

    if (addToInstList)
    {
        i->setCISAOff(curCISAOffset);

        if (m_options->getOption(vISA_EmitLocation))
        {
            i->setLocation(new (mem) MDLocation(lineno == 0 ? curLine : lineno, curFile));
        }

        instList.push_back(i);
    }

    instAllocList.push_back(i);

    return i;
}

// same as above, except we don't add it to the Builder's instList
G4_INST* IR_Builder::createInternalInst(G4_Predicate* prd,
                                        G4_opcode op,
                                        G4_CondMod* mod,
                                        bool sat,
                                        unsigned char size,
                                        G4_DstRegRegion* dst,
                                        G4_Operand* src0,
                                        G4_Operand* src1,
                                        G4_Operand* src2,
                                        unsigned int option)
{
    return createInternalInst(prd, op, mod, sat, size, dst, src0, src1, src2, option, 0, UNMAPPABLE_VISA_INDEX, NULL);
}
G4_INST* IR_Builder::createInternalInst(G4_Predicate* prd,
                                        G4_opcode op,
                                        G4_CondMod* mod,
                                        bool sat,
                                        unsigned char size,
                                        G4_DstRegRegion* dst,
                                        G4_Operand* src0,
                                        G4_Operand* src1,
                                        G4_Operand* src2,
                                        unsigned int option,
                                        int lineno, int CISAoff,
                                        const char* srcFilename)
{
    auto ii = createInst(prd, op, mod, sat, size, dst, src0, src1, src2, option,
        lineno, false);

    ii->setCISAOff(CISAoff);

    if (m_options->getOption(vISA_EmitLocation))
    {
        ii->setLocation(new (mem) MDLocation(lineno == 0 ? curLine : lineno, srcFilename));
    }

    return ii;
}

G4_InstSend* IR_Builder::createSendInst(G4_Predicate* prd,
    G4_opcode op,
    unsigned char size,
    G4_DstRegRegion* postDst,
    G4_SrcRegRegion* currSrc,
    G4_Operand* msg,
    unsigned int option,
    G4_SendMsgDescriptor *msgDesc,
    int lineno,
    bool addToInstList)
{

    assert (msgDesc && "msgDesc must not be null");
    G4_InstSend* m = new (mem)G4_InstSend(*this, prd, op, size, postDst, currSrc, msg, option, msgDesc);

    if (addToInstList)
    {
        m->setCISAOff(curCISAOffset);

        if (m_options->getOption(vISA_EmitLocation))
        {
            m->setLocation(new (mem) MDLocation(lineno == 0 ? curLine : lineno, curFile));
        }

        instList.push_back(m);
    }

    instAllocList.push_back(m);

    return m;
}

G4_InstSend* IR_Builder::createInternalSendInst(G4_Predicate* prd,
    G4_opcode op,
    unsigned char size,
    G4_DstRegRegion* postDst,
    G4_SrcRegRegion* currSrc,
    G4_Operand* msg,
    unsigned int option,
    G4_SendMsgDescriptor *msgDesc,
    int lineno,
    int CISAoff,
    const char* srcFilename)
{
    auto ii = createSendInst(prd, op, size, postDst, currSrc,
        msg, option, msgDesc, lineno, false);

    ii->setCISAOff(CISAoff);

    if (m_options->getOption(vISA_EmitLocation))
    {
        ii->setLocation(new (mem) MDLocation(lineno == 0 ? curLine : lineno, srcFilename));
    }

    return ii;
}

//
// Create a split send (sends) instruction
// sends (size) dst src0 src1 exDesc msgDesc
//

G4_InstSend* IR_Builder::createSplitSendInst(G4_Predicate* prd,
                                         G4_opcode op,
                                         unsigned char size,
                                         G4_DstRegRegion* dst,
                                         G4_SrcRegRegion* src0, // can be header
                                         G4_SrcRegRegion* src1,
                                         G4_Operand* msg,       // msg descriptor: imm or vec
                                         unsigned int option,
                                         G4_SendMsgDescriptor *msgDesc,
                                         G4_Operand* src3,      // ext msg desciptor: imm or vec
                                         int lineno,
                                         bool addToInstList)
{

    if (!src1)
    {
        // src1 may be null if we need to force generate split send (e.g., for bindless surfaces)
        MUST_BE_TRUE(msgDesc->extMessageLength() == 0, "src1 length must be 0 if it is null");
        src1 = createNullSrc(Type_UD);
    }
    if (!src3)
    {
        src3 = createImm(msgDesc->getExtendedDesc(), Type_UD);
    }
    G4_InstSend* m = new (mem) G4_InstSend(*this, prd, op, size, dst, src0, src1, msg, src3, option, msgDesc);

    if (addToInstList)
    {
        m->setCISAOff(curCISAOffset);

        if (m_options->getOption(vISA_EmitLocation))
        {
            m->setLocation(new (mem) MDLocation(lineno == 0 ? curLine : lineno, curFile));
        }
        instList.push_back(m);
    }

    instAllocList.push_back(m);

    return m;
}

G4_InstSend* IR_Builder::createInternalSplitSendInst(G4_Predicate* prd,
    G4_opcode op,
    unsigned char size,
    G4_DstRegRegion* dst,
    G4_SrcRegRegion* src0, // can be header
    G4_SrcRegRegion* src1,
    G4_Operand* msg,       // msg descriptor: imm or vec
    unsigned int option,
    G4_SendMsgDescriptor *msgDesc,
    G4_Operand* src3,      // ext msg desciptor: imm or vec
    int lineno,
    int CISAoff,
    const char* srcFilename)
{
    auto ii = createSplitSendInst(prd, op, size, dst, src0, src1, msg, option,
        msgDesc, src3, lineno, false);

    ii->setCISAOff(CISAoff);

    if (m_options->getOption(vISA_EmitLocation))
    {
        ii->setLocation(new (mem) MDLocation(lineno == 0 ? curLine : lineno, srcFilename));
    }

    return ii;
}

//
// Math instruction is like a generic one except:
// -- it takes a G4_MathOp to specify the function control
// -- conditional modifier is not allowed
// -- there are additional restrictions on dst/src regions that will be checked in HW conformity
//
G4_INST* IR_Builder::createMathInst(G4_Predicate* prd,
                                    bool sat,
                                    unsigned char size,
                                    G4_DstRegRegion* dst,
                                    G4_Operand* src0,
                                    G4_Operand* src1,
                                    G4_MathOp mathOp,
                                    unsigned int option,
                                    int lineno,
                                    bool addToInstList)
{
    G4_INST* i = new (mem)G4_InstMath(*this, prd, G4_math, NULL, sat, size, dst, src0, src1, option, mathOp);

    if (addToInstList)
    {
        i->setCISAOff(curCISAOffset);

        if (m_options->getOption(vISA_EmitLocation))
        {
            i->setLocation(new (mem) MDLocation(lineno == 0 ? curLine : lineno, curFile));
        }
        instList.push_back(i);
    }

    instAllocList.push_back(i);

    return i;
}

G4_INST* IR_Builder::createInternalMathInst(G4_Predicate* prd,
    bool sat,
    unsigned char size,
    G4_DstRegRegion* dst,
    G4_Operand* src0,
    G4_Operand* src1,
    G4_MathOp mathOp,
    unsigned int option,
    int lineno,
    int CISAoff,
    const char* srcFilename)
{
    auto ii = createMathInst(prd, sat, size, dst, src0, src1, mathOp, option, lineno, false);

    ii->setCISAOff(CISAoff);

    if (m_options->getOption(vISA_EmitLocation))
    {
        ii->setLocation(new (mem) MDLocation(lineno == 0 ? curLine : lineno, srcFilename));
    }

    return ii;
}

G4_INST* IR_Builder::createIntrinsicInst(
    G4_Predicate* prd, Intrinsic intrinId,
    uint8_t size, G4_DstRegRegion* dst,
    G4_Operand* src0, G4_Operand* src1, G4_Operand* src2,
    unsigned int option, int lineno, bool addToInstList)
{
    G4_INST* i = nullptr;

    if(intrinId == Intrinsic::Spill)
        i = new (mem) G4_SpillIntrinsic(*this, prd, intrinId, size, dst, src0, src1, src2, option);
    else if(intrinId == Intrinsic::Fill)
        i = new (mem) G4_FillIntrinsic(*this, prd, intrinId, size, dst, src0, src1, src2, option);
    else
        i = new (mem) G4_InstIntrinsic(*this, prd, intrinId, size, dst, src0, src1, src2, option);

    if (addToInstList)
    {
        i->setCISAOff(curCISAOffset);

        if (m_options->getOption(vISA_EmitLocation))
        {
            i->setLocation(new (mem) MDLocation(lineno == 0 ? curLine : lineno, curFile));
        }

        instList.push_back(i);
    }

    instAllocList.push_back(i);

    return i;
}

G4_INST* IR_Builder::createInternalIntrinsicInst(G4_Predicate* prd, Intrinsic intrinId,
    uint8_t size, G4_DstRegRegion* dst,
    G4_Operand* src0, G4_Operand* src1, G4_Operand* src2,
    unsigned int option, int lineno, int CISAoff, const char* srcFilename)
{
    auto ii = createIntrinsicInst(prd, intrinId, size, dst, src0, src1, src2, option,
        lineno, false);

    ii->setCISAOff(CISAoff);

    if (m_options->getOption(vISA_EmitLocation))
    {
        ii->setLocation(new (mem) MDLocation(lineno == 0 ? curLine : lineno, srcFilename));
    }

    return ii;
}

G4_MathOp IR_Builder::Get_MathFuncCtrl(ISA_Opcode op, G4_Type type)
{
    switch(op)
    {
    case ISA_LOG:
        return MATH_LOG;
    case ISA_MOD:   // remainder of IDIV
        return MATH_INT_DIV_REM;
    case ISA_POW:
        return MATH_POW;
    case ISA_SIN:
        return MATH_SIN;
    case ISA_COS:
        return MATH_COS;
    case ISA_SQRT:
        return MATH_SQRT;
    case ISA_RSQRT:
        return MATH_RSQ;
    case ISA_INV:
        return MATH_INV;
    case ISA_DIV:
        return IS_FTYPE(type) || IS_HFTYPE(type) ? MATH_FDIV : MATH_INT_DIV_QUOT;
    case ISA_EXP:
        return MATH_EXP;
    default:
        ASSERT_USER(0, "Illegal math opcode." );
        return MATH_RESERVED;
    }
}

// After building IR total number number of rows required
// for arg and retvar become known, so resize the pre-defined
// vars here to the max required in current compilation unit.
void IR_Builder::resizePredefinedStackVars()
{
    getStackCallArg()->resizeNumRows(this->getArgSize());
    getStackCallRet()->resizeNumRows(this->getRetVarSize());
}

/*
* Create send instruction for specified GenX architecture.
* bti: surface id
* sti: sampler id
*/
G4_InstSend* IR_Builder::Create_Send_Inst_For_CISA(
    G4_Predicate* pred,
    G4_DstRegRegion *postDst,
    G4_SrcRegRegion *payload,
    unsigned regs2snd,
    unsigned regs2rcv,
    unsigned execsize,
    unsigned fc,
    SFID tf_id,
    bool header_present,
    SendAccess access,
    G4_Operand* bti,
    G4_Operand* sti,
    unsigned int option,
    bool is_sendc)
{
    G4_SendMsgDescriptor* msgDesc =
        createSendMsgDesc(fc, regs2rcv, regs2snd, tf_id,
                          0, 0, access, bti, sti);

    msgDesc->setHeaderPresent(header_present);

    return Create_Send_Inst_For_CISA(pred, postDst, payload, execsize, msgDesc, option, is_sendc);
}

//bindless surfaces, write the content of T252 to extended message descriptor
//exdesc holds the value of the extended message descriptor for bit [0:11]
//add (1) a0.2<1>:ud T252<1>:ud exDesc:ud {NoMask}
// returns a0.2<0;1,0>:ud
G4_SrcRegRegion* IR_Builder::createBindlessExDesc(uint32_t exdesc)
{
    // virtual var for each exdesc
    G4_SrcRegRegion* T252 = Create_Src_Opnd_From_Dcl(builtinT252, getRegionScalar());
    const char* buf = getNameString(mem, 20, "ExDesc%d", num_temp_dcl++);
    G4_Declare* exDescDecl = createDeclareNoLookup(buf, G4_ADDRESS, 1, 1, Type_UD);
    exDescDecl->setSubRegAlign(Four_Word);
    G4_DstRegRegion* dst = Create_Dst_Opnd_From_Dcl(exDescDecl, 1);
    if (useNewExtDescFormat())
    {
        createMov(1, dst, T252, InstOpt_WriteEnable, true);
    }
    else
    {
        createBinOp(G4_add, 1, dst, T252, createImm(exdesc, Type_UD), InstOpt_WriteEnable, true);
    }
    return Create_Src_Opnd_From_Dcl(exDescDecl, getRegionScalar());
}

G4_InstSend *IR_Builder::Create_Send_Inst_For_CISA(G4_Predicate *pred,
                                               G4_DstRegRegion *postDst,
                                               G4_SrcRegRegion *payload,
                                               unsigned execsize,
                                               G4_SendMsgDescriptor *msgDesc,
                                               unsigned option,
                                               bool is_sendc)
{
    G4_opcode send_opcode= is_sendc ? G4_sendc : G4_send;

    fixSendDstType(postDst, (uint8_t) execsize);

    uint32_t desc = msgDesc->getDesc();
    G4_Operand *bti = msgDesc->getBti();
    G4_Operand *sti = msgDesc->getSti();
    G4_Operand *descOpnd = NULL;

    bool needSamplerMove = sti && !sti->isImm() && !isBindlessSampler(sti);

    if ((bti && !bti->isImm()) || needSamplerMove)
    {
        // use a0.0 directly
        G4_DstRegRegion* addr_dst_opnd = Create_Dst_Opnd_From_Dcl(builtinA0, 1);

        if( bti && !bti->isImm() )
        {
            //add (1) a0.0:ud bti:ud desc:ud
            // create source for bti
            createBinOp(
                G4_add,
                1,
                addr_dst_opnd,
                bti,
                createImm( desc, Type_UD ),
                InstOpt_WriteEnable,
                true);
        }

        if (needSamplerMove)
        {
            G4_Declare *dcl1 = createTempVar(1, Type_UD, Any );
            G4_DstRegRegion* tmp_dst_opnd = Create_Dst_Opnd_From_Dcl(dcl1, 1);

            createBinOp(
                G4_shl,
                1,
                tmp_dst_opnd,
                sti,
                createImm( 8, Type_UD ),
                InstOpt_WriteEnable,
                true );

            G4_SrcRegRegion* tmp_src_opnd = Create_Src_Opnd_From_Dcl(dcl1, getRegionScalar());

            if( !bti || bti->isImm() )
            {
                createBinOp(
                    G4_add,
                    1,
                    addr_dst_opnd,
                    tmp_src_opnd,
                    createImm( desc, Type_UD ),
                    InstOpt_WriteEnable,
                    true);
            }
            else
            {
                G4_SrcRegRegion* addr_src_opnd = Create_Src_Opnd_From_Dcl(builtinA0, getRegionScalar());

                createBinOp(
                    G4_add,
                    1,
                    duplicateOperand( addr_dst_opnd ),
                    addr_src_opnd,
                    tmp_src_opnd,
                    InstOpt_WriteEnable,
                    true );
            }
        }

        descOpnd = Create_Src_Opnd_From_Dcl(builtinA0, getRegionScalar());
    }
    else
    {
        descOpnd = createImm(desc, Type_UD);
    }

    return createSendInst(
        pred,
        send_opcode,
        (uint8_t)execsize,
        postDst,
        payload,
        descOpnd,
        option,
        msgDesc,
        0);
}

/*
* Create split send instruction for specified GenX architecture.
* bti: surface id
* sti: sampler id
* Gen9: sends (execsize)     dst,  src1,  src2,  ex_desc,  desc
*/
G4_InstSend* IR_Builder::Create_SplitSend_Inst_For_CISA(
    G4_Predicate* pred,
    G4_DstRegRegion *dst,
    G4_SrcRegRegion *src1,
    unsigned regs2snd1,
    G4_SrcRegRegion *src2,
    unsigned regs2snd2,
    unsigned regs2rcv,
    unsigned execsize,
    unsigned fc,
    SFID tf_id,
    bool header_present,
    SendAccess access,
    G4_Operand* bti,
    G4_Operand* sti,
    unsigned int option,
    bool is_sendc)
{
    G4_SendMsgDescriptor *msgDesc =
        createSendMsgDesc(fc, regs2rcv, regs2snd1, tf_id, regs2snd2,
                          0, access, bti, sti);

    msgDesc->setHeaderPresent(header_present);

    return Create_SplitSend_Inst(pred, dst, src1, src2, execsize,
        msgDesc, option, is_sendc);
}

// desc, if indirect, is constructed from the BTI/STI values in msgDesc and is always a0.0
G4_InstSend *IR_Builder::Create_SplitSend_Inst(G4_Predicate *pred,
    G4_DstRegRegion *dst,
    G4_SrcRegRegion *src1,
    G4_SrcRegRegion *src2,
    unsigned execsize,
    G4_SendMsgDescriptor *msgDesc,
    unsigned option,
    bool is_sendc)
{
    G4_opcode send_opcode = is_sendc ? G4_sendsc : G4_sends;

    fixSendDstType(dst, (uint8_t) execsize);

    uint32_t desc = msgDesc->getDesc();
    uint32_t exdesc = msgDesc->getExtendedDesc();
    G4_Operand *bti = msgDesc->getBti();
    G4_Operand *sti = msgDesc->getSti();

    G4_Operand* descOpnd = NULL;
    G4_SrcRegRegion* extDescOpnd = nullptr;

    bool doAlignBindlessSampler = alignBindlessSampler() && sti && isBindlessSampler(sti);
    bool needsSamplerMove = (sti && !sti->isImm() && !isBindlessSampler(sti)) || doAlignBindlessSampler;

    bool needsSurfaceMove = false;
    bool needsA0ExDesc = false;

    if (bti && bti->isSrcRegRegion())
    {
        if (isBindlessSurface(bti))
        {
            needsA0ExDesc = true;
            // set T252 as BTI
            if ((desc & 0xFF) != PREDEF_SURF_252)
            {
                desc = (desc & ~0xFF) | PREDEF_SURF_252;
            }
        }
        else
        {
            needsSurfaceMove = true;
        }
    }

    if (needsSurfaceMove)
    {
        //add (1) a0.0:ud bti:ud desc:ud
        G4_DstRegRegion* addrDstOpnd = Create_Dst_Opnd_From_Dcl(builtinA0, 1);

        createBinOp(G4_add, 1, addrDstOpnd, bti,
            createImm(desc, Type_UD), InstOpt_WriteEnable, true);
    }

    if (needsSamplerMove)
    {
        G4_Declare *dcl1 = createTempVar(1, Type_UD, Any);

        if (doAlignBindlessSampler)
        {
            // check if address is 32-byte aligned
            // use STI = 0 for 32-byte aligned address, STI = 1 otherwise
            // (W) and (1) (nz)f0.0 null S31 0x10:uw
            G4_Declare* tmpFlag = createTempFlag(1);
            G4_CondMod* condMod = createCondMod(Mod_nz, tmpFlag->getRegVar(), 0);
            createInst(nullptr, G4_and, condMod, false, 1, createNullDst(Type_UD),
                createSrcRegRegion(*(sti->asSrcRegRegion())), createImm(0x10, Type_UW), InstOpt_WriteEnable);
            // (W) (f0.0) sel (1) tmp:ud 0x100 0x0
            G4_Predicate* pred = createPredicate(PredState_Plus, tmpFlag->getRegVar(), 0);
            createInst(pred, G4_sel, nullptr, false, 1, Create_Dst_Opnd_From_Dcl(dcl1, 1),
                createImm(0x100, Type_UW), createImm(0x0, Type_UW), InstOpt_WriteEnable);
        }
        else
        {
            // shl (1) tmp:ud sti:ud 0x8:uw
            G4_DstRegRegion* tmpDstOpnd = Create_Dst_Opnd_From_Dcl(dcl1, 1);
            createBinOp(G4_shl, 1, tmpDstOpnd, sti,
                createImm(8, Type_UD), InstOpt_WriteEnable, true);
        }

        G4_SrcRegRegion* tmpSrcOpnd = Create_Src_Opnd_From_Dcl(dcl1, getRegionScalar());
        G4_DstRegRegion* addrDstOpnd = Create_Dst_Opnd_From_Dcl(builtinA0, 1);
        if (!needsSurfaceMove)
        {
            // add (1) a0.0 tmp:ud desc:ud
            createBinOp(G4_add, 1, addrDstOpnd, tmpSrcOpnd,
                createImm(desc, Type_UD),
                InstOpt_WriteEnable,
                true);
        }
        else
        {
            // add (1) a0.0 a0.0:ud tmp:ud
            G4_SrcRegRegion* addrSrcOpnd = Create_Src_Opnd_From_Dcl(builtinA0, getRegionScalar());
            createBinOp(G4_add, 1, addrDstOpnd, addrSrcOpnd,
                tmpSrcOpnd, InstOpt_WriteEnable, true);
        }
    }

    if (needsSurfaceMove || needsSamplerMove)
    {
        descOpnd = Create_Src_Opnd_From_Dcl(builtinA0, getRegionScalar());
    }
    else
    {
        descOpnd = createImm(desc, Type_UD);
    }

    if (needsA0ExDesc)
    {
        extDescOpnd = createBindlessExDesc(exdesc);
    }
    else
    {
        // do nothing as the extended msg desc will just be a null operand
    }

    return createSplitSendInst(pred, send_opcode, (uint8_t)execsize,
        dst, src1, src2,
        descOpnd,
        option, msgDesc, extDescOpnd, 0);
}



// for RTWrite,
// desc has a constant BTI value (i.e., no bindless) and no STI
// extDesc may be indirect (MRT and other bits) and is passed in
G4_InstSend *IR_Builder::Create_SplitSend_Inst_For_RTWrite(G4_Predicate *pred,
    G4_DstRegRegion *dst,
    G4_SrcRegRegion *src1,
    G4_SrcRegRegion *src2,
    G4_SrcRegRegion *extDescOpnd,
    unsigned execsize,
    G4_SendMsgDescriptor *msgDesc,
    unsigned option)
{
    G4_opcode send_opcode = G4_sendsc;

    fixSendDstType(dst, (uint8_t)execsize);

    uint32_t desc = msgDesc->getDesc();
    G4_Operand* descOpnd = nullptr;
    G4_Operand *bti = msgDesc->getBti();

    if (bti && bti->isSrcRegRegion())
    {
        //add (1) a0.0:ud bti:ud desc:ud
        G4_DstRegRegion* addrDstOpnd = Create_Dst_Opnd_From_Dcl(builtinA0, 1);
        createBinOp(G4_add, 1, addrDstOpnd, bti,
            createImm(desc, Type_UD), InstOpt_WriteEnable, true);
        descOpnd = Create_Src_Opnd_From_Dcl(builtinA0, getRegionScalar());
    }
    else
    {
        descOpnd = createImm(desc, Type_UD);
    }

    return createSplitSendInst(pred, send_opcode, (uint8_t)execsize,
        dst, src1, src2, descOpnd,
        option, msgDesc, extDescOpnd);
}

// create a declare for send payload
G4_Declare* IR_Builder::createSendPayloadDcl( unsigned num_elt, G4_Type type )
{
    const char* name = getNameString(mem, 16, "M%u", ++num_temp_dcl);
    unsigned short numRow = ( num_elt * G4_Type_Table[type].byteSize - 1 ) / GENX_GRF_REG_SIZ + 1;
    unsigned short numElt = ( numRow == 1 ) ? num_elt : (GENX_GRF_REG_SIZ/G4_Type_Table[type].byteSize);
    G4_Declare *dcl = createDeclareNoLookup(
        name,
        G4_GRF,
        numElt,
        numRow,
        type);
    return dcl;
}

void IR_Builder::Create_MOVR0_Inst( G4_Declare* dcl, short regOff, short subregOff, bool use_nomask )
{
    G4_DstRegRegion* dst1_opnd = createDst(
        dcl->getRegVar(),
        regOff,
        subregOff,
        1,
        dcl->getElemType());

    // create r0 src
    G4_SrcRegRegion* r0_src_opnd = Create_Src_Opnd_From_Dcl(builtinR0, getRegionStride1());
    // create inst
    createMov(
        GENX_DATAPORT_IO_SZ,
        dst1_opnd,
        r0_src_opnd,
        ( use_nomask ? InstOpt_WriteEnable : 0 ),
        true );
}

void IR_Builder::Create_ADD_Inst(G4_Declare* dcl, short regOff, short subregOff, uint8_t execsize,
    G4_Predicate* pred, G4_CondMod* condMod, G4_Operand* src0_opnd, G4_Operand* src1_opnd, G4_InstOption options)
{
    auto dst = createDst(dcl->getRegVar(), regOff, subregOff, 1, dcl->getElemType());

    if (src0_opnd->isImm() && src0_opnd->asImm()->isZero())
    {
        createInst(pred, G4_mov, condMod, false, execsize, dst, src1_opnd, NULL, options);
    }
    else if (src1_opnd->isImm() && src1_opnd->asImm()->isZero())
    {
        createInst(pred, G4_mov, condMod, false, execsize, dst, src0_opnd, NULL, options);
    }
    else if (src0_opnd->isImm() && !src1_opnd->isImm())
    {
        createInst(pred, G4_add, condMod, false, execsize, dst, src1_opnd, src0_opnd, options);
    }
    else
    {
        createInst(pred, G4_add, condMod, false, execsize, dst, src0_opnd, src1_opnd, options);
    }
}

// curently this function is only called in dataport intrinsic translation functions.
// if it is used in some other places, the Qtrctrl should be changed if needed. currently it is NOMASK by default.
// TODO!!! change inst_opt if QtrCtrl is allowed for GATHER later.
void IR_Builder::Create_MOV_Inst(
    G4_Declare* dcl,
    short regOff,
    short subregOff,
    unsigned execsize,
    G4_Predicate* pred,
    G4_CondMod* condMod,
    G4_Operand* src_opnd,
    bool use_nomask )
{
    G4_DstRegRegion* dst2_opnd = createDst(
        dcl->getRegVar(),
        regOff,
        subregOff,
        1,
        dcl->getElemType());

    createInst(
        pred,
        G4_mov,
        condMod,
        false,
        (uint8_t) execsize,
        dst2_opnd,
        src_opnd,
        NULL,
        ( use_nomask ? InstOpt_WriteEnable : 0 ),
        0 );
}

// send payload preparation.
// dcl: decl for send payload
// num_dword: number of DW to send
// src_opnd: send src, its size may be several GRFs
void IR_Builder::Create_MOV_Send_Src_Inst(
    G4_Declare* dcl,
    short regoff,
    short subregoff,
    unsigned num_dword,
    G4_Operand* src_opnd,
    unsigned int option )
{
    // since src_opnd is raw source in CISA, it is aligned to GRF, so there is no subRegOff.
    unsigned remained_dword = num_dword;
    // if data type of src_opnd is not UD, change it to UD
    // assumption: size of src_opnd is multiple of UD
    short dst_regoff = regoff, dst_subregoff = subregoff;
    unsigned char execsize = 1;
    G4_DstRegRegion* dst = NULL;
    //G4_SrcRegRegion* src = NULL;
    G4_Operand* src = NULL;
    const RegionDesc *rd = NULL;
    G4_Declare *dst_dcl = dcl;
    short src_regoff = 0, src_subregoff = 0;
    bool non_ud_scalar = false;
    bool scalar_src = ( src_opnd->isImm() || num_dword == 1 );

    if( scalar_src && src_opnd->getType() != Type_UD ){
        // change the type of dst dcl to src type
        remained_dword = num_dword * ( G4_Type_Table[Type_UD].byteSize/G4_Type_Table[src_opnd->getType()].byteSize );
        dst_dcl = createSendPayloadDcl(remained_dword, src_opnd->getType());
        dst_dcl->setAliasDeclare( dcl, regoff * G4_GRF_REG_NBYTES + subregoff * G4_Type_Table[Type_UD].byteSize );
        dst_regoff = 0;
        dst_subregoff = 0;
        non_ud_scalar = true;
    }

    src_regoff = src_opnd->asSrcRegRegion()->getRegOff();
    src_subregoff = src_opnd->asSrcRegRegion()->getSubRegOff();
    src_subregoff = (src_subregoff * G4_Type_Table[src_opnd->getType()].byteSize) / G4_Type_Table[dst_dcl->getElemType()].byteSize;


    auto getMaxEsize = [](uint32_t opt)
    {
        unsigned maskOption = (opt & InstOpt_QuarterMasks);
        switch (maskOption)
        {
        case InstOpt_M4:
        case InstOpt_M12:
        case InstOpt_M20:
        case InstOpt_M28:
            return 4;
        case InstOpt_M8:
        case InstOpt_M24:
            return 8;
        case InstOpt_M16:
            return 16;
        default:
            return 32;
        }
    };
    int maxEsize = getMaxEsize(option);

    // here remained_dword is not the number of DW, but the number of dst data type.
    while( remained_dword )
    {
        if( non_ud_scalar && G4_Type_Table[src_opnd->getType()].byteSize != G4_Type_Table[Type_UD].byteSize )
        {
            if( remained_dword >= 32 )
            {
                execsize = 32;
            }
            else if( remained_dword >= 16 )
            {
                execsize = 16;
            }
            else
            {
                execsize = (uint8_t) Round_Down_Pow2(remained_dword);
            }

            execsize = (execsize > maxEsize) ? maxEsize :  execsize;
            if( execsize == 1 )
            {
                rd = getRegionScalar();
            }
            else
            {
                rd = getRegionStride1();
            }
        }
        else
        {
            if( remained_dword >= 16 )
            {
                execsize = 16;
            }
            else if( remained_dword >= 8 )
            {
                execsize = 8;
            }
            else
            {
                execsize = (uint8_t) Round_Down_Pow2(remained_dword);
            }
            execsize = (execsize > maxEsize) ? maxEsize :  execsize;
            if ( execsize == 1 )
            {
                rd = getRegionScalar();
            }
            else
            {
                rd = getRegionStride1();
            }
        }

        dst = createDst(
            dst_dcl->getRegVar(),
            dst_regoff,
            dst_subregoff,
            1,
            dst_dcl->getElemType());

        if (scalar_src && src_opnd->isImm())
        {
            src = src_opnd->asImm();
        }
        else
        {

            src = createSrcRegRegion(
                Mod_src_undef,
                Direct,
                src_opnd->asSrcRegRegion()->getBase(),
                src_regoff,
                src_subregoff,
                rd,
                dst_dcl->getElemType());
        }

        createMov(
            execsize,
            dst,
            src,
            option,
            true );

        // update offset in decl
        if (remained_dword >= execsize) {
            remained_dword -= execsize;
            if (execsize * dst_dcl->getElemSize() == 2 * G4_GRF_REG_NBYTES) {
                dst_regoff += 2;
                if (!scalar_src) {
                    src_regoff += 2;
                }
            }
            else if (execsize * dst_dcl->getElemSize() == G4_GRF_REG_NBYTES) {
                dst_regoff += 1;
                if (!scalar_src) {
                    src_regoff += 1;
                }
            }
            else {
                dst_subregoff += execsize;
                if (dst_subregoff > (G4_GRF_REG_NBYTES / dst_dcl->getElemSize())) {
                    dst_regoff++;
                    dst_subregoff -= G4_GRF_REG_NBYTES / dst_dcl->getElemSize();
                }
                if (!scalar_src) {
                    src_subregoff += execsize;
                    if (src_subregoff > (short)(G4_GRF_REG_NBYTES / G4_Type_Table[Type_UD].byteSize)) {
                        src_regoff++;
                        src_subregoff -= G4_GRF_REG_NBYTES / G4_Type_Table[Type_UD].byteSize;
                    }
                }
            }
        }
    }
}
// create an opnd without regpoff and subregoff
G4_DstRegRegion* IR_Builder::Create_Dst_Opnd_From_Dcl( G4_Declare* dcl, unsigned short hstride )
{
    return createDst(
        dcl->getRegVar(),
        0,
        0,
        hstride,
        dcl->getElemType());
}
// create an opnd without regpoff and subregoff
G4_SrcRegRegion* IR_Builder::Create_Src_Opnd_From_Dcl(
    G4_Declare* dcl, const RegionDesc* rd)
{
    return createSrcRegRegion(
        Mod_src_undef,
        Direct,
        dcl->getRegVar(),
        0,
        0,
        rd,
        dcl->getElemType());
}

G4_DstRegRegion* IR_Builder::createNullDst(G4_Type dstType)
{
    return createDst(
        phyregpool.getNullReg(),
        0,
        0,
        1,
        dstType );
}

G4_SrcRegRegion* IR_Builder::createNullSrc( G4_Type srcType )
{
    return createSrcRegRegion( Mod_src_undef,
                               Direct,
                               phyregpool.getNullReg(),
                               0,
                               0,
                               getRegionScalar(),
                               srcType);
}

/*check if the dst opnd align to GRF.
* if it is not aligned to GRF
1. change align of var dcl to GRF if the dst size is smaller than GRF size, no alias or alias offset is 0.
2. otherwise, create a temp operand and return it.
*/
G4_DstRegRegion* IR_Builder::Check_Send_Dst( G4_DstRegRegion *dst_opnd )
{
    //FIXME: This function seems to be bogus
    G4_DstRegRegion* d;
    // check if dst is align to GRF

    if( G4_Type_Table[dst_opnd->getType()].byteSize > G4_Type_Table[Type_B].byteSize )
    {
        d = dst_opnd;
    }
    else
    {
        // change type of dcl and offset in it
        short new_SubRegOff = dst_opnd->getSubRegOff();
        if( dst_opnd->getRegAccess() == Direct )
        {
            new_SubRegOff = dst_opnd->getSubRegOff() * G4_Type_Table[Type_B].byteSize / G4_Type_Table[Type_UD].byteSize;
        }
        G4_DstRegRegion new_dst(
            dst_opnd->getRegAccess(),
            dst_opnd->getBase(),
            dst_opnd->getRegOff(),
            new_SubRegOff,
            1,
            Type_UD);
        d = createDstRegRegion( new_dst );
    }

    return d;
}

void IR_Builder::addInputArg(input_info_t * inpt)
{
    m_inputVect.push_back(inpt);
}

input_info_t * IR_Builder::getInputArg(unsigned int index)
{
    return m_inputVect[index];
}

unsigned int IR_Builder::getInputCount()
{
    return (uint32_t) m_inputVect.size();
}

input_info_t *IR_Builder::getRetIPArg() {
    // TODO: So far, we assume the last argument of caller of callable kernel
    // or callable kernel is the RetIP argument. If required, extra attribute
    // will be added to specify which QWORD argument is used as RetIP argument
    // and the code will traverse all argument to find that one.
    input_info_t *RetIP = getInputArg(getInputCount() - 1);
    // More sanity check on the argument.
    ASSERT_USER(IS_QTYPE(RetIP->dcl->getElemType()), "RetIP needs to be QWORD!");
    ASSERT_USER(RetIP->dcl->getNumElems() == 1, "RetIP needs to be QWORD!");
    return RetIP;
}

// check if an operand is aligned to <align_byte>
bool IR_Builder::isOpndAligned( G4_Operand *opnd, unsigned short &offset, int align_byte )
{
    offset = 0;
    bool isAligned = true;

    switch ( opnd->getKind() )
    {
    case G4_Operand::immediate:
    case G4_Operand::addrExp:
    case G4_Operand::label:
    case G4_Operand::condMod:
    case G4_Operand::predicate:
    {
        isAligned = true;
        break;
    }
    case G4_Operand::srcRegRegion:
    case G4_Operand::dstRegRegion:
    {
        int type_size = G4_Type_Table[opnd->getType()].byteSize;
        G4_Declare *dcl = NULL;
        if (opnd->getBase()->isRegVar())
        {
            dcl = opnd->getBase()->asRegVar()->getDeclare();
            while (dcl && dcl->getAliasDeclare())
            {
                if( dcl->getSubRegAlign() != Any &&
                    ( ( ( dcl->getSubRegAlign() * 2 ) >= align_byte && ( dcl->getSubRegAlign() * 2 ) % align_byte != 0 ) ||
                    ( ( dcl->getSubRegAlign() * 2 ) < align_byte && align_byte % ( dcl->getSubRegAlign() * 2 ) != 0 ) ) )
                {
                        isAligned = false;
                        break;
                }
                offset += (unsigned short) dcl->getAliasOffset();
                dcl = dcl->getAliasDeclare();
            }

            if (dcl && dcl->getRegVar() && dcl->getRegVar()->isPhyRegAssigned())
            {
                offset += static_cast<unsigned short>(dcl->getRegVar()->getByteAddr());
            }
        }
        if( !isAligned )
        {
            return isAligned;
        }

        if( opnd->isDstRegRegion() )
        {
            if( opnd->asDstRegRegion()->getRegAccess() != Direct )
            {
                isAligned = false;
            }
            offset += opnd->asDstRegRegion()->getRegOff() * G4_GRF_REG_NBYTES + opnd->asDstRegRegion()->getSubRegOff() * type_size;
        }
        else if( opnd->isSrcRegRegion() )
        {
            if( opnd->asSrcRegRegion()->getRegAccess() != Direct )
            {
                isAligned = false;
            }
            offset += opnd->asSrcRegRegion()->getRegOff() * G4_GRF_REG_NBYTES + opnd->asSrcRegRegion()->getSubRegOff() * type_size;
        }
        if( offset % align_byte != 0 )
        {
            return false;
        }
        // Only alignment of the top dcl can be changed.
        if (dcl && dcl->getRegFile() == G4_GRF)
        {
            if (dcl->getSubRegAlign() == Any ||
                ((dcl->getSubRegAlign() * 2) < align_byte && align_byte % (dcl->getSubRegAlign() * 2) == 0))
            {
                dcl->setSubRegAlign(G4_SubReg_Align(align_byte / 2));
            }
            else if( ( dcl->getSubRegAlign() * 2 ) < align_byte || ( dcl->getSubRegAlign() * 2 ) % align_byte != 0 )
            {
                    isAligned = false;
            }
        }
        else if (opnd->getKind() == G4_Operand::dstRegRegion &&
            // Only care about GRF or half-GRF alignment.
            (align_byte == G4_GRF_REG_NBYTES || align_byte == G4_GRF_REG_NBYTES / 2) &&
            dcl && dcl->getRegFile() == G4_ADDRESS)
        {

            // Get the single definition of the specified operand from the use
            // inst.
            auto getSingleDefInst = [](G4_INST *UI, Gen4_Operand_Number OpndNum)
                -> G4_INST * {
                G4_INST *Def = nullptr;
                for (DEF_EDGE_LIST_ITER I = UI->defInstList.begin(),
                                        E = UI->defInstList.end();
                                        I != E; ++I) {
                    if (I->second != OpndNum)
                        continue;
                    if (Def) {
                        // Not single defined, bail out
                        Def = nullptr;
                        break;
                    }
                    Def = I->first;
                }
                return Def;
            };

            G4_INST *inst = opnd->getInst();
            if (inst) {
                // Check address calculation like:
                //
                //    shl (1) V1  V0          imm
                //    add (1) a0  $V2 + off   V1
                //    ...
                //    (use)... r[a0, disp] ...
                //
                // need to check both disp, off, and V1 are aligned.
                //
                // Check acc_use_op's def-list.
                G4_INST *LEA = getSingleDefInst(inst, Opnd_dst);
                if (LEA && LEA->opcode() == G4_add && LEA->getExecSize() == 1) {
                    isAligned = true;
                    G4_Operand *Op0 = LEA->getSrc(0);
                    G4_Operand *Op1 = LEA->getSrc(1);
                    if (Op0->isSrcRegRegion()) {
                        // TODO: Consider MUL as well.
                        G4_INST *Def = getSingleDefInst(LEA, Opnd_src0);
                        if (Def && Def->opcode() == G4_shl &&
                            Def->getSrc(1)->isImm()) {
                            G4_Imm *Imm = Def->getSrc(1)->asImm();
                            unsigned Factor = (1U << Imm->getInt());
                            // TODO: We only perform alignment checking on
                            // component wise and may need to consider checking
                            // the accumulated result.
                            if (Factor % align_byte != 0)
                                isAligned = false;
                        } else if (Def && Def->opcode() == G4_and &&
                                   Def->getSrc(1)->isImm()) {
                            G4_Imm *Imm = Def->getSrc(1)->asImm();
                            uint64_t Mask = uint64_t(Imm->getInt());
                            // align_byte could be 32 or 16 guarded previsouly.
                            uint64_t AlignMask = align_byte - 1;
                            if ((Mask & AlignMask) != 0)
                                isAligned = false;
                        } else
                            isAligned = false;
                    }
                    if (isAligned && Op1->isAddrExp()) {
                        G4_AddrExp *AE = Op1->asAddrExp();
                        G4_Declare *Dcl = AE->getRegVar()->getDeclare();
                        unsigned AliasOffset = 0;
                        while (Dcl && Dcl->getAliasDeclare()) {
                            AliasOffset += Dcl->getAliasOffset();
                            Dcl = Dcl->getAliasDeclare();
                        }
                        // TODO: We only perform alignment checking on
                        // component wise and may need to consider checking
                        // the accumulated result.
                        if ((AliasOffset % align_byte) != 0 ||
                            (Dcl && Dcl->getSubRegAlign() != GRFALIGN &&
                             Dcl->getSubRegAlign() != Sixteen_Word &&
                             Dcl->getSubRegAlign() != Eight_Word) ||
                            AE->getOffset() % align_byte != 0) {
                            isAligned = false;
                        }
                    } else
                        isAligned = false;
                    if (isAligned) {
                        // TODO: We only perform alignment checking on
                        // component wise and may need to consider checking
                        // the accumulated result.
                        if (opnd->asDstRegRegion()->getAddrImm() % align_byte != 0)
                            isAligned = false;
                    }
                }
            }
        }
        else if (dcl && dcl->getRegFile() == G4_FLAG)
        {
            // need to make flag even-word aligned if it's used in a setp with dword source
            // ToDo: should we fix input to use 16-bit value instead
            if (align_byte == 4)
            {
                dcl->setSubRegAlign(Even_Word);
            }
        }
        break;
    }
    default:
        break;
    }
    return isAligned;
}

G4_Predicate_Control IR_Builder::vISAPredicateToG4Predicate(VISA_PREDICATE_CONTROL control, int size)
{
    switch (control)
    {
    case PRED_CTRL_NON:
        return PRED_DEFAULT;
    case PRED_CTRL_ANY:
    {
        if (!predCtrlHasWidth())
        {
            return PRED_ANY_WHOLE;
        }
        switch (size)
        {
        case 1:
            return PRED_DEFAULT;
        case 2:
            return PRED_ANY2H;
        case 4:
            return PRED_ANY4H;
        case 8:
            return PRED_ANY8H;
        case 16:
            return PRED_ANY16H;
        case 32:
            return PRED_ANY32H;
        default:
            MUST_BE_TRUE(0, "Invalid predicate control group size.");
            return PRED_DEFAULT;
        }
    }
    case PRED_CTRL_ALL:
    {
        if (!predCtrlHasWidth())
        {
            return PRED_ALL_WHOLE;
        }
        switch (size)
        {
        case 1:
            return PRED_DEFAULT;
        case 2:
            return PRED_ALL2H;
        case 4:
            return PRED_ALL4H;
        case 8:
            return PRED_ALL8H;
        case 16:
            return PRED_ALL16H;
        case 32:
            return PRED_ALL32H;
        default:
            MUST_BE_TRUE(0, "Invalid predicate control group size.");
            return PRED_DEFAULT;
        }
    }
    default:
        MUST_BE_TRUE(0, "Invalid predicate control.");
        return PRED_DEFAULT;
    }
}

// bind a vISA input variable <dcl> to the GRF byte offset <offset>
void IR_Builder::bindInputDecl(G4_Declare* dcl, int offset)
{    // decide the physical register number and sub register number
    unsigned int regNum = offset / getGRFSize();
    unsigned int subRegNum = (offset % getGRFSize()) / dcl->getElemSize();
    dcl->getRegVar()->setPhyReg(phyregpool.getGreg(regNum), subRegNum);
    dcl->setRegFile(G4_INPUT);
    unsigned int reservedGRFNum = m_options->getuInt32Option(vISA_ReservedGRFNum);
    if (regNum + dcl->getNumRows() > kernel.getNumRegTotal() - reservedGRFNum) {
        MUST_BE_TRUE(false, "INPUT payload execeeds the regsiter number");
    }
}


