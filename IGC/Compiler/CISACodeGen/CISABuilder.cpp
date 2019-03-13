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
#include "Compiler/CISACodeGen/CISABuilder.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/PixelShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/ComputeShaderCodeGen.hpp"
#include "Compiler/MetaDataApi/IGCMetaDataDefs.h"
#include "common/allocator.h"
#include "common/Types.hpp"
#include "common/Stats.hpp"
#include "common/MemStats.h"
#include "common/debug/Dump.hpp"
#include "common/igc_regkeys.hpp"
#include "common/secure_mem.h"
#include "common/secure_string.h"
#include "common/shaderOverride.hpp"
#include "common/CompilerStatsUtils.hpp"
#include "inc/common/sku_wa.h"
#include "inc/common/RelocationInfo.h"
#include <iStdLib/utility.h>

#if !defined(_WIN32)
#   define _strdup strdup
#   define _snprintf snprintf
#endif

/***********************************************************************************
This file defines the CEncoder class which is used to generate CISA instructions
************************************************************************************/

// macro to check the result of VISA API calls
#define V(x) { int result = x; assert(result==0 && "call to VISA API failed"); }

static const unsigned  int g_cScratchSpaceMsglimit = (128 * 1024);
using namespace llvm;

namespace IGC
{

unsigned int getGRFSize()
{
    unsigned int byteSize = 32;

    return byteSize;
}

Common_ISA_Exec_Size getExecSize(SIMDMode width)
{
    switch(width)
    {
    case SIMDMode::SIMD1   : return EXEC_SIZE_1;
    case SIMDMode::SIMD4   : return EXEC_SIZE_4;
    case SIMDMode::SIMD8   : return EXEC_SIZE_8;
    case SIMDMode::SIMD16  : return EXEC_SIZE_16;
    case SIMDMode::SIMD32  : return EXEC_SIZE_32;
    case SIMDMode::UNKNOWN :
    default                : assert(0 && "unreachable"); break;
    }
    return EXEC_SIZE_ILLEGAL;
}

VISAAtomicOps convertAtomicOpEnumToVisa(AtomicOp op)
{
    switch(op)
    {
    case EATOMIC_AND:
    case EATOMIC_AND64:
        return ATOMIC_AND;
    case EATOMIC_DEC:
    case EATOMIC_DEC64:
        return ATOMIC_DEC;
    case EATOMIC_IADD:
    case EATOMIC_IADD64:
        return ATOMIC_ADD;
    case EATOMIC_IMAX:
    case EATOMIC_IMAX64:
        return ATOMIC_IMAX;
    case EATOMIC_IMIN:
    case EATOMIC_IMIN64:
        return ATOMIC_IMIN;
    case EATOMIC_INC:
    case EATOMIC_INC64:
        return ATOMIC_INC;
    case EATOMIC_MAX:
    case EATOMIC_MAX64:
        return ATOMIC_MAX;
    case EATOMIC_MIN:
    case EATOMIC_MIN64:
        return ATOMIC_MIN;
    case EATOMIC_OR:
    case EATOMIC_OR64:
        return ATOMIC_OR;
    case EATOMIC_SUB:
    case EATOMIC_SUB64:
        return ATOMIC_SUB;
    case EATOMIC_UMAX:
    case EATOMIC_UMAX64:
        return ATOMIC_MAX;
    case EATOMIC_UMIN:
    case EATOMIC_UMIN64:
        return ATOMIC_MIN;
    case EATOMIC_XOR:
    case EATOMIC_XOR64:
        return ATOMIC_XOR;
    case EATOMIC_XCHG:
    case EATOMIC_XCHG64:
        return ATOMIC_XCHG;
    case EATOMIC_CMPXCHG:
    case EATOMIC_CMPXCHG64:
        return ATOMIC_CMPXCHG;
    case EATOMIC_PREDEC:
    case EATOMIC_PREDEC64:
        return ATOMIC_PREDEC;
    case EATOMIC_FMAX:
        return ATOMIC_FMAX;
    case EATOMIC_FMIN:
        return ATOMIC_FMIN;
    case EATOMIC_FCMPWR:
         return ATOMIC_FCMPWR;
    default:
        assert(0 && "Atomic Op not implemented");
    }

    return ATOMIC_AND;
}

inline Common_ISA_Exec_Size visaExecSize(SIMDMode width)
{
    switch(width)
    {
    case SIMDMode::SIMD1   : return EXEC_SIZE_1;
    case SIMDMode::SIMD2   : return EXEC_SIZE_2;
    case SIMDMode::SIMD4   : return EXEC_SIZE_4;
    case SIMDMode::SIMD8   : return EXEC_SIZE_8;
    case SIMDMode::SIMD16  : return EXEC_SIZE_16;
    case SIMDMode::SIMD32  : return EXEC_SIZE_32;
    case SIMDMode::UNKNOWN :
    default                : assert(0 && "unreachable"); break;
    }
    return EXEC_SIZE_ILLEGAL;
}

inline GATHER_SCATTER_ELEMENT_SIZE visaElementSize(unsigned int m_elt_size)
{
    GATHER_SCATTER_ELEMENT_SIZE elementSize = GATHER_SCATTER_BYTE_UNDEF;
    if(m_elt_size == 1)
    {
        elementSize = GATHER_SCATTER_BYTE;
    }
    else if( m_elt_size == 2)
    {
        elementSize = GATHER_SCATTER_WORD;
    }
    else if(m_elt_size == 4)
    {
        elementSize = GATHER_SCATTER_DWORD;
    }
    else
    {
        assert(0 && "unreachable");
    }
    return elementSize;
}

static inline Common_ISA_SVM_Block_Type
visaBlockType(unsigned elemSize) {
    switch (elemSize) {
    case 8:  return SVM_BLOCK_TYPE_BYTE;
    case 32: return SVM_BLOCK_TYPE_DWORD;
    case 64: return SVM_BLOCK_TYPE_QWORD;
    }

    assert(false && "Unknown block/element size. Expect 8-/32-/64-bit only!");
    return static_cast<Common_ISA_SVM_Block_Type>(~0U);
}

static inline Common_ISA_SVM_Block_Num
visaBlockNum(unsigned numElems) {
    switch (numElems) {
    case 1: return SVM_BLOCK_NUM_1;
    case 2: return SVM_BLOCK_NUM_2;
    case 4: return SVM_BLOCK_NUM_4;
    case 8: return SVM_BLOCK_NUM_8;
    }

    assert(false &&
           "Unknown number of blocks/elements. Expect 1, 2, 4, or 8 only!");
    return static_cast<Common_ISA_SVM_Block_Num>(~0U);
}

static unsigned
GetRawOpndSplitOffset(Common_ISA_Exec_Size fromExecSize,
                      Common_ISA_Exec_Size toExecSize,
                      unsigned thePart, CVariable *var) {
    if (!var || var->IsUniform())
        return 0;

    assert(fromExecSize == EXEC_SIZE_16 && toExecSize == EXEC_SIZE_8 &&
        "Only support splitting from exec-size 16 to exec-size 8!");
    assert((thePart == 0 || thePart == 1) &&
        "Splitting from exec-size-16 to exec-size-8 only breaks into 2 "
        "parts!");

    unsigned elemSize = var->GetElemSize();

    switch (elemSize) {
    case 4:
        return thePart * SIZE_GRF * 1;
    case 8:
        return thePart * SIZE_GRF * 2;
    }

    assert(false && "Unknown data type to split!");
    return ~0U;
}

size_t URBChannelMask::size() const
{
    return m_bitmask == 0 ? 0 : iSTD::bsr(m_bitmask) + 1;
}

unsigned int URBChannelMask::asVISAMask() const
{
    // if all bits in the mask are set we need to return 0xFF which means 'no channel mask'
    // if all bits are set -> adding one creates a power of two, so x and x+1 has no common bits.
    if (((m_bitmask + 1) & m_bitmask) == 0)
    {
        return (uint32_t) -1;
    }
    else
    {
        return (uint16_t) m_bitmask;
    }
}

void CEncoder::Init()
{
    m_encoderState.m_srcOperand[0].init();
    m_encoderState.m_srcOperand[1].init();
    m_encoderState.m_srcOperand[2].init();
    m_encoderState.m_srcOperand[3].init();
    m_encoderState.m_dstOperand.init();
    m_encoderState.m_flag.init();
    m_encoderState.m_mask = EMASK_Q1;
    m_encoderState.m_noMask = false;
    m_encoderState.m_simdSize = m_program->m_SIMDSize;
    m_encoderState.m_uniformSIMDSize = SIMDMode::SIMD1;
}

CEncoder::CEncoder()
{
    m_program = nullptr;
    vbuilder = nullptr;
}

CEncoder::~CEncoder()
{
}

void CEncoder::SetProgram(CShader* program)
{
    m_program = program;
    Init();
}

void CEncoder::SubroutineCall(CVariable* flag, llvm::Function *F)
{
    VISA_LabelOpnd* visaLabel = GetFuncLabel(F);
    m_encoderState.m_flag.var = flag;
    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
    // control flow instructions cannot be broken down into lower SIMD
    Common_VISA_EMask_Ctrl emask = m_encoderState.m_noMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1;
    Common_ISA_Exec_Size execSize = visaExecSize(m_program->m_dispatchSize);
    V(vKernel->AppendVISACFCallInst(predOpnd, emask, execSize, visaLabel));
}

void CEncoder::StackCall(CVariable* flag, llvm::Function *F, unsigned char argSize, unsigned char retSize)
{
    VISAFunction* visaFunc = GetStackFunction(F);
    m_encoderState.m_flag.var = flag;
    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
    // control flow instructions cannot be broken down into lower SIMD
    Common_VISA_EMask_Ctrl emask = m_encoderState.m_noMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1;
    Common_ISA_Exec_Size execSize = visaExecSize(m_program->m_dispatchSize);
    unsigned int funcId = 0;
    V(visaFunc->GetFunctionId(funcId));
    V(vKernel->AppendVISACFFunctionCallInst(predOpnd, emask, execSize, (unsigned short)funcId, argSize, retSize));
}

void CEncoder::IndirectStackCall(CVariable* flag, CVariable* funcPtr, unsigned char argSize, unsigned char retSize)
{
    m_encoderState.m_flag.var = flag;
    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
    // control flow instructions cannot be broken down into lower SIMD
    Common_VISA_EMask_Ctrl emask = m_encoderState.m_noMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1;
    Common_ISA_Exec_Size execSize = visaExecSize(m_program->m_dispatchSize);
    VISA_VectorOpnd* funcAddrOpnd = GetSourceOperandNoModifier(funcPtr);
    V(vKernel->AppendVISACFIndirectFuncCallInst(predOpnd, emask, execSize, funcAddrOpnd, argSize, retSize));
}

void CEncoder::SubroutineRet(CVariable* flag)
{
    m_encoderState.m_flag.var = flag;
    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
    // control flow instructions cannot be broken down into lower SIMD
    Common_VISA_EMask_Ctrl emask = m_encoderState.m_noMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1;
    Common_ISA_Exec_Size execSize = visaExecSize(m_program->m_dispatchSize);
    V(vKernel->AppendVISACFRetInst(predOpnd, emask, execSize));
}

void CEncoder::StackRet(CVariable* flag)
{
    m_encoderState.m_flag.var = flag;
    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
    // control flow instructions cannot be broken down into lower SIMD
    Common_VISA_EMask_Ctrl emask = m_encoderState.m_noMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1;
    Common_ISA_Exec_Size execSize = visaExecSize(m_program->m_dispatchSize);
    V(vKernel->AppendVISACFFunctionRetInst(predOpnd, emask, execSize));
}

void CEncoder::Jump(CVariable* flag, uint label)
{
    VISA_LabelOpnd* visaLabel = GetLabel(label);
    m_encoderState.m_flag.var = flag;
    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
    // control flow instructions cannot be broken down into lower SIMD
    Common_VISA_EMask_Ctrl emask = m_encoderState.m_noMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1;
    Common_ISA_Exec_Size execSize = visaExecSize(m_program->m_dispatchSize);
	if (flag == nullptr || flag->IsUniform())
    {
        execSize = EXEC_SIZE_1;
    }
    V(vKernel->AppendVISACFSIMDInst(ISA_GOTO, predOpnd, emask, execSize, visaLabel));
}

void CEncoder::Label(uint label)
{
    VISA_LabelOpnd* visaLabel = GetLabel(label);
    V(vKernel->AppendVISACFLabelInst(visaLabel));
}

uint CEncoder::GetNewLabelID()
{
    uint id = labelMap.size();
    labelMap.push_back(nullptr);
    return (id);
}

void CEncoder::DwordAtomicRaw(
    AtomicOp atomic_op,
    const ResourceDescriptor& resource,
    CVariable* dst,
    CVariable* elem_offset,
    CVariable* src0,
    CVariable* src1,
    bool is16Bit)
{
    assert(m_encoderState.m_flag.var == nullptr && "predicate not supported");
    VISA_StateOpndHandle* pSurfStateOpndHandle = GetVISASurfaceOpnd(resource);
    VISA_PredOpnd *predOpnd = GetFlagOperand(m_encoderState.m_flag);
    VISA_RawOpnd* pDst = GetRawDestination(dst);
    VISA_RawOpnd* pElemOffset = GetRawSource(elem_offset);
    VISA_RawOpnd* pSrc0 = GetRawSource(src0);
    VISA_RawOpnd* pSrc1 = GetRawSource(src1);

    /*
    So the problem is this - the message was added for SNB, and at the time it was implemented as
    CMPXCHG : new = (old==src1) ? src0 : old

    In IVB this becomes untyped atomic, and it's implemented as
    AOP_CMPWR (src0 == old_dst) ? src1 : old_dst old_dst

    Note that the source is swapped.  Since we define CMPXCHG as the former in vISA, internally we
    perform a swap for it.  So I guess for now you'll need to swap the two source to follow the vISA
    semantics.  We may want to add a new vISA message to fix this issue.
    */
    if (atomic_op == EATOMIC_CMPXCHG) {
        std::swap(pSrc0, pSrc1);
    }

    V(vKernel->AppendVISASurfAccessDwordAtomicInst(
        predOpnd,
        convertAtomicOpEnumToVisa(atomic_op),
        is16Bit,
        ConvertMaskToVisaType(m_encoderState.m_mask, m_encoderState.m_noMask),
        visaExecSize(m_encoderState.m_simdSize),
        pSurfStateOpndHandle,
        pElemOffset,
        pSrc0,
        pSrc1,
        pDst));
}

void CEncoder::Cmp(e_predicate p, CVariable* dst, CVariable* src0, CVariable* src1)
{
    Common_ISA_Cond_Mod subOp = ConvertCondModToVisaType(p);

    bool flagDst = 0;
    if( dst->GetType() == ISA_TYPE_BOOL )
    {
        flagDst = true;
    }

    unsigned numParts = 0;

    // Due to a simulator quirk, we need to split the instruction even if the
    // dst operand of the compare is null, if it "looks" too large,
    // that is, if the execution size is 16 and the comparison type
    // is QW.
    bool bNeedSplitting = false;
    if (flagDst &&
        (GetAluExecSize(dst) == EXEC_SIZE_16) &&
        (src0->GetElemSize() > 4 || src1->GetElemSize() > 4))
    {
        bNeedSplitting = true;
        numParts = 2;
    }

    bNeedSplitting = bNeedSplitting ||
                     NeedSplitting(src0, m_encoderState.m_srcOperand[0], numParts, true) ||
                     NeedSplitting(src1, m_encoderState.m_srcOperand[1], numParts, true);

    if (bNeedSplitting)
    {
        Common_VISA_EMask_Ctrl execMask = GetAluEMask(dst);
        Common_ISA_Exec_Size fromExecSize = GetAluExecSize(dst);
        Common_ISA_Exec_Size toExecSize = SplitExecSize(fromExecSize, numParts);

        for (unsigned thePart = 0; thePart != numParts; ++thePart) {
            SModifier newSrc0Mod = SplitVariable(fromExecSize, toExecSize, thePart, src0, m_encoderState.m_srcOperand[0], true);
            SModifier newSrc1Mod = SplitVariable(fromExecSize, toExecSize, thePart, src1, m_encoderState.m_srcOperand[1], true);
            VISA_VectorOpnd* srcOpnd0 = GetSourceOperand(src0, newSrc0Mod);
            VISA_VectorOpnd* srcOpnd1 = GetSourceOperand(src1, newSrc1Mod);
            if( flagDst )
            {
                V(vKernel->AppendVISAComparisonInst(subOp,
                    SplitEMask(fromExecSize, toExecSize, thePart, execMask),
                    toExecSize,
                    dst->visaPredVariable,
                    srcOpnd0, srcOpnd1));
            }
            else
            {
                SModifier newDstMod = SplitVariable(fromExecSize, toExecSize, thePart, dst, m_encoderState.m_dstOperand);
                VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dst, newDstMod);
                V(vKernel->AppendVISAComparisonInst(subOp,
                    SplitEMask(fromExecSize, toExecSize, thePart, execMask),
                    toExecSize,
                    dstOpnd,
                    srcOpnd0, srcOpnd1));
            }
        }
    } else {
        VISA_VectorOpnd* opnd0 = GetSourceOperand(src0, m_encoderState.m_srcOperand[0]);
        VISA_VectorOpnd* opnd1 = GetSourceOperand(src1, m_encoderState.m_srcOperand[1]);

        if( flagDst )
        {
            V(vKernel->AppendVISAComparisonInst(
                subOp,
                GetAluEMask(dst),
                GetAluExecSize(dst),
                dst->visaPredVariable,
                opnd0,
                opnd1));
        }
        else
        {
            V(vKernel->AppendVISAComparisonInst(
                subOp,
                GetAluEMask(dst),
                GetAluExecSize(dst),
                GetDestinationOperand(dst, m_encoderState.m_dstOperand),
                opnd0,
                opnd1));
        }
    }
}

void CEncoder::Select(CVariable* flag, CVariable* dst, CVariable* src0, CVariable* src1)
{
    m_encoderState.m_flag.var = flag;

    unsigned numParts = 0;
    if (NeedSplitting(dst, m_encoderState.m_dstOperand, numParts) ||
        NeedSplitting(src0, m_encoderState.m_srcOperand[0], numParts, true) ||
        NeedSplitting(src1, m_encoderState.m_srcOperand[1], numParts, true)) {

        Common_VISA_EMask_Ctrl execMask = GetAluEMask(dst);
        Common_ISA_Exec_Size fromExecSize = GetAluExecSize(dst);
        Common_ISA_Exec_Size toExecSize = SplitExecSize(fromExecSize, numParts);

        for (unsigned thePart = 0; thePart != numParts; ++thePart) {
            SModifier newDstMod = SplitVariable(fromExecSize, toExecSize, thePart, dst, m_encoderState.m_dstOperand);
            SModifier newSrc0Mod = SplitVariable(fromExecSize, toExecSize, thePart, src0, m_encoderState.m_srcOperand[0], true);
            SModifier newSrc1Mod = SplitVariable(fromExecSize, toExecSize, thePart, src1, m_encoderState.m_srcOperand[1], true);
            VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dst, newDstMod);
            VISA_VectorOpnd* srcOpnd0 = GetSourceOperand(src0, newSrc0Mod);
            VISA_VectorOpnd* srcOpnd1 = GetSourceOperand(src1, newSrc1Mod);
            VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
            V(vKernel->AppendVISADataMovementInst(ISA_SEL, predOpnd, IsSat(),
                SplitEMask(fromExecSize, toExecSize, thePart, execMask),
                toExecSize,
                dstOpnd, srcOpnd0, srcOpnd1));
        }
    } else {
        VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dst, m_encoderState.m_dstOperand);
        VISA_VectorOpnd* src0Opnd = GetSourceOperand(src0, m_encoderState.m_srcOperand[0]);
        VISA_VectorOpnd* src1Opnd = GetSourceOperand(src1, m_encoderState.m_srcOperand[1]);
        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);

        V(vKernel->AppendVISADataMovementInst(
            ISA_SEL,
            predOpnd,
            IsSat(),
            GetAluEMask(dst),
            GetAluExecSize(dst),
            dstOpnd,
            src0Opnd,
            src1Opnd));
    }
}

void CEncoder::SetDstSubVar(uint subVar)
{
    m_encoderState.m_dstOperand.subVar = int_cast<uint8_t>(subVar);
}

void CEncoder::SetDstSubReg(uint subReg)
{
    m_encoderState.m_dstOperand.subReg = int_cast<uint16_t>(subReg);
}

void CEncoder::SetSrcSubVar(uint srcNum, uint subVar)
{
    assert(srcNum<4);
    m_encoderState.m_srcOperand[srcNum].subVar = int_cast<uint8_t>(subVar);
}

void CEncoder::SetSrcSubReg(uint srcNum, uint subReg)
{
    assert(srcNum<4);
    m_encoderState.m_srcOperand[srcNum].subReg = int_cast<uint16_t>(subReg);
}

void CEncoder::SetDstModifier(e_modifier mod)
{
    assert(mod==EMOD_SAT || mod==EMOD_NONE);
    m_encoderState.m_dstOperand.mod = mod;
}

void CEncoder::SetSrcModifier(uint srcNum, e_modifier mod)
{
    assert(mod!=EMOD_SAT);
    assert(srcNum<3);
    m_encoderState.m_srcOperand[srcNum].mod = mod;
}

void CEncoder::SetPredicate(CVariable* flag)
{
    assert(flag==NULL || flag->GetVarType()==EVARTYPE_PREDICATE);
    m_encoderState.m_flag.var = flag;
}

void CEncoder::SetInversePredicate(bool inv)
{
    m_encoderState.m_flag.invertFlag = inv;
}

void CEncoder::SetPredicateMode(e_predMode mode)
{
    m_encoderState.m_flag.mode = mode;
}

void CEncoder::SetDstModifier(const DstModifier& modifier)
{
    if(modifier.sat)
    {
        SetDstModifier(EMOD_SAT);
    }
    if(modifier.flag)
    {
        SetPredicate(m_program->GetSymbol(modifier.flag->value));
        SetInversePredicate(modifier.invertFlag);
    }
}

void CEncoder::SetSrcRegion(uint srcNum, uint vStride, uint width, uint hStride)
{
    m_encoderState.m_srcOperand[srcNum].region[0] = int_cast<uint8_t>(vStride);
    m_encoderState.m_srcOperand[srcNum].region[1] = int_cast<uint8_t>(width);
    m_encoderState.m_srcOperand[srcNum].region[2] = int_cast<uint8_t>(hStride);
    m_encoderState.m_srcOperand[srcNum].specialRegion = true;
}

void CEncoder::SetDstRegion(uint hStride)
{
    m_encoderState.m_dstOperand.region[2] = int_cast<uint8_t>(hStride);
    m_encoderState.m_dstOperand.specialRegion = (hStride != 1);
}

uint64_t GetSignBit(VISA_Type type)
{
    switch(type)
    {
    case ISA_TYPE_Q:
    case ISA_TYPE_DF:
        return 63;
    case ISA_TYPE_D:
    case ISA_TYPE_F:
        return 31;
    case ISA_TYPE_W:
    case ISA_TYPE_HF:
        return 15;
    case ISA_TYPE_B:
        return 7;
    default:
        assert(0 && "type doesn't support modifier");
        break;
    }
    return 63;
}

bool IsFloat(VISA_Type type)
{
    return type == ISA_TYPE_DF || type == ISA_TYPE_F || type == ISA_TYPE_HF;
}

uint64_t CalculateImmediateValue(CVariable* var, e_modifier mod)
{
    uint64_t immediate = var->GetImmediateValue();
    assert(mod == EMOD_ABS || mod == EMOD_NEG || mod == EMOD_NEGABS || mod == EMOD_NONE);
    // handle modifiers for immediates.
    // Change the sign bit for floats and do logic operations for integers
    if(IsFloat(var->GetType()))
    {
        if(mod == EMOD_ABS)
        {
            immediate &= ~((uint64_t)(1)<<GetSignBit(var->GetType()));
        }
        else if(mod == EMOD_NEG)
        {
            immediate ^= (uint64_t)(1)<<GetSignBit(var->GetType());
        }
        else if(mod == EMOD_NEGABS)
        {
             immediate |= ((uint64_t)(1)<<GetSignBit(var->GetType()));
        }
    }
    else
    {
        if(mod == EMOD_ABS || mod == EMOD_NEGABS)
        {
            uint64_t mask = (immediate >> GetSignBit(var->GetType())) & (uint64_t)0x01;
            immediate = (immediate + mask) ^ mask;
        }
        if(mod == EMOD_NEG || mod == EMOD_NEGABS)
        {
            immediate = ~immediate + 1;
        }
    }
    return immediate;
}

VISA_VectorOpnd* CEncoder::GetSourceOperandNoModifier(CVariable* var)
{
    SModifier nullMod;
    nullMod.init();
    return GetSourceOperand(var, nullMod);
}

VISA_VectorOpnd* CEncoder::GetSourceOperand(CVariable* var, const SModifier& mod)
{
    if(var == nullptr)
    {
        return nullptr;
    }
    VISA_VectorOpnd* operand = nullptr;
    if(var->IsImmediate())
    {
        uint64_t immediate = CalculateImmediateValue(var, mod.mod);
        V(vKernel->CreateVISAImmediate(operand, &immediate, var->GetType()));
    }
    else
    {
        if(var->GetVarType() == EVARTYPE_GENERAL)
        {
            unsigned short vStride = 1;
            unsigned short width   = 1;
            unsigned short hStride = 0;

            if(mod.specialRegion)
            {
                vStride = int_cast<unsigned short>(mod.region[0]);
                width   = int_cast<unsigned short>(mod.region[1]);
                hStride = int_cast<unsigned short>(mod.region[2]);
            }
            else if(var->IsUniform())
            {
                //Scalar regioning
                vStride = 0;
                width   = 1;
                hStride = 0;
            }
            unsigned char rowOffset = 0;
            unsigned char colOffset = 0;
            GetRowAndColOffset(var, mod.subVar, mod.subReg, rowOffset, colOffset);
            V(vKernel->CreateVISASrcOperand(
                operand,
                GetVISAVariable(var),
                ConvertModifierToVisaType(mod.mod),
                vStride,
                width,
                hStride,
                rowOffset,
                colOffset));
        }
        else if (var->GetVarType() == EVARTYPE_ADDRESS)
        {
            if (var->IsUniform())
            {
                // uniform addressing uses 1x1 indirect addressing mode
                unsigned short vStride = 8;
                unsigned short width   = 8;
                unsigned short hStride = 1;

                //if vector is also uniform
                if(var->IsVectorUniform())
                {
                    vStride = 0;
                    width   = 1;
                    hStride = 0;
                }
                unsigned short immOffset = (unsigned short)
                    mod.subReg * GetCISADataTypeSize(var->GetType());
                V(vKernel->CreateVISAIndirectSrcOperand(
                    operand,
                    var->visaAddrVariable,
                    MODIFIER_NONE,
                    0,
                    immOffset,
                    vStride,
                    width,
                    hStride,
                    var->GetType()));
            }
            else
            {
                // non-uniform addressing uses VxH indirect addressing mode
                // NB: this requires that all subregisters of a0 are properly
                // set up, including per-lane subreg offsets.
                V(vKernel->CreateVISAIndirectOperandVxH(
                    operand,
                    var->visaAddrVariable,
                    mod.subReg,
                    0,
                    var->GetType()));
            }
        }
    }
    return operand;
}

VISA_VectorOpnd* CEncoder::GetDestinationOperand(CVariable* var, const SModifier& mod)
{
    VISA_VectorOpnd* operand = NULL;
    //Create Dst operand
    if(var->GetVarType() == EVARTYPE_GENERAL)
    {
        unsigned short hStride  = 1;
        unsigned char rowOffset = 0;
        unsigned char colOffset = 0;
        GetRowAndColOffset(var, mod.subVar, mod.subReg, rowOffset, colOffset);
        if(mod.specialRegion)
        {
            hStride = (unsigned short)mod.region[2];
        }

        V(vKernel->CreateVISADstOperand(
            operand,
            GetVISAVariable(var),
            hStride,
            rowOffset,
            colOffset));
    }
    else if (var->GetVarType() == EVARTYPE_ADDRESS)
    {
        const unsigned short hStride  = 1;
        unsigned char  addrOffset = int_cast<unsigned char>(mod.subReg);
        unsigned short immOffset = 0;
        if (var->IsUniform())
        {
            // We are using 1x1 destination region, we must use a0.0.
            // Use subReg to compute immOffset.
            immOffset = (unsigned short)
                mod.subReg * GetCISADataTypeSize(var->GetType());
            addrOffset = 0;
        }
        V(vKernel->CreateVISAIndirectDstOperand(
            operand,
            var->visaAddrVariable,
            addrOffset,
            immOffset,
            hStride,
            var->GetType()));
    }
    return operand;
}

VISA_PredOpnd* CEncoder::GetFlagOperand(const SFlag& flag)
{
    if(flag.var == nullptr)
    {
        return nullptr;
    }
    VISA_PredOpnd* operand = nullptr;
    VISA_PREDICATE_STATE predState = (flag.invertFlag)
        ? PredState_INVERSE : PredState_NO_INVERSE;
    VISA_PREDICATE_CONTROL predCtrl = PRED_CTRL_NON;

    switch (flag.mode)
    {
    case EPRED_ALL:     predCtrl = PRED_CTRL_ALL;   break;
    case EPRED_ANY:     predCtrl = PRED_CTRL_ANY;   break;
    default: break;
    }

    V(vKernel->CreateVISAPredicateOperand(
        operand,
        flag.var->visaPredVariable,
        predState,
        predCtrl));
    return operand;
}

Common_ISA_Exec_Size CEncoder::GetAluExecSize(CVariable* dst) const
{
    SIMDMode simdSize = m_encoderState.m_simdSize;

    if (dst && dst->GetVarType() == EVARTYPE_ADDRESS)
    {
        if (dst->IsVectorUniform() && dst->IsUniform())
        {
            simdSize = m_encoderState.m_uniformSIMDSize;
        }
    }
    else if (dst && dst->IsUniform())
    {
        if (dst->GetVarType() == EVARTYPE_PREDICATE)
        {
            if (dst->GetNumberElement() == 1)
            {
                simdSize = m_encoderState.m_uniformSIMDSize;
            }
        }
        else
        {
            simdSize = m_encoderState.m_uniformSIMDSize;
        }
    }

    return visaExecSize(simdSize);
}

Common_VISA_EMask_Ctrl CEncoder::GetAluEMask(CVariable* dst)
{
    e_mask mask = m_encoderState.m_mask;
    bool noMask = m_encoderState.m_noMask;
    if (dst)
    {
        if (m_encoderState.m_SubSpanDestination)
        {
            noMask = true;
        }
        else
        {
            if (dst->GetVarType() == EVARTYPE_ADDRESS)
            {
                if (dst->IsVectorUniform() && dst->IsUniform())
                {
                    noMask = true;
                }
            }
            else if (dst->IsUniform())
            {
                noMask = true;
            }
        }
    }

    return ConvertMaskToVisaType(mask, noMask);
}

bool CEncoder::IsSat()
{
    return (m_encoderState.m_dstOperand.mod == EMOD_SAT)? true: false;
}

void CEncoder::MinMax(CISA_MIN_MAX_SUB_OPCODE subopcode, CVariable* dst, CVariable* src0, CVariable* src1)
{
    assert(m_encoderState.m_flag.var == nullptr && "min/max doesn't support predication");

    unsigned numParts = 0;
    if (NeedSplitting(dst, m_encoderState.m_dstOperand, numParts) ||
        NeedSplitting(src0, m_encoderState.m_srcOperand[0], numParts, true) ||
        NeedSplitting(src1, m_encoderState.m_srcOperand[1], numParts, true)) {

        Common_VISA_EMask_Ctrl execMask = GetAluEMask(dst);
        Common_ISA_Exec_Size fromExecSize = GetAluExecSize(dst);
        Common_ISA_Exec_Size toExecSize = SplitExecSize(fromExecSize, numParts);

        for (unsigned thePart = 0; thePart != numParts; ++thePart) {
            SModifier newDstMod = SplitVariable(fromExecSize, toExecSize, thePart, dst, m_encoderState.m_dstOperand);
            SModifier newSrc0Mod = SplitVariable(fromExecSize, toExecSize, thePart, src0, m_encoderState.m_srcOperand[0], true);
            SModifier newSrc1Mod = SplitVariable(fromExecSize, toExecSize, thePart, src1, m_encoderState.m_srcOperand[1], true);
            VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dst, newDstMod);
            VISA_VectorOpnd* srcOpnd0 = GetSourceOperand(src0, newSrc0Mod);
            VISA_VectorOpnd* srcOpnd1 = GetSourceOperand(src1, newSrc1Mod);
            V(vKernel->AppendVISAMinMaxInst(subopcode, IsSat(),
                SplitEMask(fromExecSize, toExecSize, thePart, execMask),
                toExecSize,
                dstOpnd, srcOpnd0, srcOpnd1));
        }
    } else {
        VISA_VectorOpnd* opnd0 = GetSourceOperand(src0, m_encoderState.m_srcOperand[0]);
        VISA_VectorOpnd* opnd1 = GetSourceOperand(src1, m_encoderState.m_srcOperand[1]);
        VISA_VectorOpnd* dstopnd = GetDestinationOperand(dst, m_encoderState.m_dstOperand);

        V(vKernel->AppendVISAMinMaxInst(
            subopcode,
            IsSat(),
            GetAluEMask(dst),
            GetAluExecSize(dst),
            dstopnd,
            opnd0,
            opnd1));
    }
}

// NeedSplitting - Check whether a variable needs splitting due to the
// violation of the hardware rule of no more than 2 GRFs should be accessed.
// So far, only the following cases are covered
// - SIMD16
//      note that SIMD32 is supported differently.
// - data types of 4+ bytes or 32+ bits
// - for source, we only handle limited regions.
//
// numParts - return the total parts to be split, e.g. if the region spans 4
// GRFs, it needs splitting into 2 parts at least.
bool CEncoder::NeedSplitting(CVariable *var, const SModifier &mod,
                             unsigned &numParts,
                             bool isSource) const {
    // If nothing is specified, don't split.
    if (!var)
        return false;
    // Only handle SIMD16 now! We assume all data movements in SIMD8 will honor
    // the region rules.
    Common_ISA_Exec_Size simdSize = GetAluExecSize(var);
    switch (simdSize) {
    default:
        return false;
    case EXEC_SIZE_16:
        break;
    // NOTE that SIMD32 will be supported differently based on the current
    // implementation!
    }

    // Only general variables need splitting so far.
    if (var->GetVarType() != EVARTYPE_GENERAL)
        return false;

    // Only varying variable need splitting so far.
    // NOTE: uniform variable is assumed to take less than 2 GRF+.
    if (var->IsUniform())
        return false;

    unsigned elemSize = var->GetElemSize();
    // We assume there is no 2 GRF crossing when element size is smaller than
    // 4 bytes (or 32 bits), e.g. 16-bit WORD.
    if (elemSize < 4)
        return false;
    // If the data type has more than 4 bytes, i.e. 32 bits, it already crosses
    // 2+ GRFs by itself. There's no need to check further.
    if (elemSize > 4) {
        assert(elemSize == 8 && "Only QWORD is supported so far!");
        assert((isSource || !mod.specialRegion) &&
               "It's expected that there's no special region associated with "
               "QWORD type destination!");
        if (isSource && mod.specialRegion) {
            if (mod.region[1] == 1 && mod.region[0] == 0) {
                // src region is <0;1,x>, can't cross 2 GRF.  No need to split.
                return false;
            }
            assert(false && "Unhandled special source region on QWORD type!");
        }
        numParts = std::max(numParts, 2U);
        return true;
    }

    // For 32-bit data types, without special region, they won't cross 2+ GRFs.
    if (!mod.specialRegion)
        return false;

    // Check regioning.
    if (isSource) {
        // FIXME: Need better support for region with non-1 width.
        if (mod.region[1] != 1)
            return false;
        if (mod.region[0] < 2)
            return false;
        // For src with width set to 1, region with > 1 vstride needs
        // splitting.
        numParts = std::max(numParts, unsigned(mod.region[0]));
        return true;
    }
    if (mod.region[2] < 2)
        return false;
    numParts = std::max(numParts, unsigned(mod.region[2]));
    // For dst, region with > 1 hstride needs splitting.
    return true;
}

// SplitVariable - Split the variable to prevent accessing 2+ GRFs.
SModifier CEncoder::SplitVariable(Common_ISA_Exec_Size fromExecSize,
                                  Common_ISA_Exec_Size toExecSize,
                                  unsigned thePart,
                                  CVariable *var, const SModifier &mod,
                                  bool isSource) const {
    // Splitting uniform or source scalar variables is unnecessary!
    bool isAddrVar = var && var->GetVarType() == EVARTYPE_ADDRESS;
    if (!var || (var->IsUniform() && (!isAddrVar || var->IsVectorUniform())) ||
        (isSource && mod.specialRegion &&
         mod.region[1] == 1 && mod.region[0] == 0 && mod.region[2] == 0))
        return mod;

    assert(fromExecSize == EXEC_SIZE_16 && toExecSize == EXEC_SIZE_8 &&
           "Only support splitting from exec-size 16 to exec-size 8!");
    assert((thePart == 0 || thePart == 1) &&
           "Splitting from exec-size-16 to exec-size-8 only breaks into 2 "
           "parts!");

    // Copy the original modifier first.
    SModifier newMod = mod;
    unsigned elemSize = var->GetElemSize();

    if (isAddrVar)
    {
        // Note that for address var, subReg has two meanings:
        //   1. if var is uniform (so using 1x1 addressing mode),
        //         subReg * (size of var's type) is a0.0's immOffset;
        //   2. otherwise (using VxH addressing mode),
        //         subReg is indeed an sub register number of a0.
        newMod.subReg += thePart * 8;
        return newMod;
    }

    if (!mod.specialRegion) {
        // Without special regioning, split the given variable based on type.
        switch (elemSize) {
        default:
            assert(false && "Unknown data type to split!");
            break;
        case 1:
        case 2:
            newMod.subReg += thePart * 8; // 8, i.e. half elements
            break;
        case 4:
            newMod.subVar += thePart * 1; // 1 GRF
            break;
        case 8:
            newMod.subVar += thePart * 2; // 2 GRFs
            break;
        }
        return newMod;
    }

    unsigned theStride = 0;
    if (isSource) {
        assert(mod.region[1] == 1 &&
               "Don't know how to split region with non-1 width!");
        theStride = mod.region[0];
    } else {
        theStride = mod.region[2];
    }

    switch (elemSize) {
    default:
        assert(false && "Unknown data type to split!");
        break;
    case 1:
    case 2:
        newMod.subReg += thePart * 8 * theStride; // 8, i.e. half elements
        break;
    case 4:
        newMod.subVar += thePart * 1 * theStride; // 1 GRF
        break;
    case 8:
        newMod.subVar += thePart * 2 * theStride; // 2 GRFs
        break;
    }

    return newMod;
}

Common_ISA_Exec_Size
CEncoder::SplitExecSize(Common_ISA_Exec_Size fromExecSize, unsigned numParts) const {
    assert(fromExecSize == EXEC_SIZE_16 && "Only know how to split SIMD16!");
    assert(numParts == 2 && "Only know splitting SIMD16 into SIMD8!");

    switch (fromExecSize) {
    default:
        break;
    case EXEC_SIZE_16:
        return EXEC_SIZE_8;
    }
    assert(false && "Unknown execution size to be split!");
    return static_cast<Common_ISA_Exec_Size>(~0);
}

Common_VISA_EMask_Ctrl
CEncoder::SplitEMask(Common_ISA_Exec_Size fromExecSize,
                     Common_ISA_Exec_Size toExecSize,
                     unsigned thePart, Common_VISA_EMask_Ctrl execMask) const {
    assert(fromExecSize == EXEC_SIZE_16 && toExecSize == EXEC_SIZE_8 &&
           "Only support splitting from exec-size 16 to exec-size 8!");
    assert((thePart == 0 || thePart == 1) &&
           "Splitting from exec-size-16 to exec-size-8 only breaks into 2 "
           "parts!");

    // FIXME: Better to generate a table!

    switch (fromExecSize) {
    default:
        break;
    case EXEC_SIZE_16:
        switch (toExecSize) {
        default:
            break;
        case EXEC_SIZE_8:
            switch (execMask) {
            default:
                break;
            case vISA_EMASK_M1:     return thePart ? vISA_EMASK_M3    : vISA_EMASK_M1;
            case vISA_EMASK_M1_NM:  return thePart ? vISA_EMASK_M3_NM : vISA_EMASK_M1_NM;
            case vISA_EMASK_M5:     return thePart ? vISA_EMASK_M7    : vISA_EMASK_M5;
            case vISA_EMASK_M5_NM:  return thePart ? vISA_EMASK_M7_NM : vISA_EMASK_M5_NM;
            }
            break;
        }
        break;
    }
    assert(false && "Unknown execution mask to be split into low part!");
    return static_cast<Common_VISA_EMask_Ctrl>(~0);
}

// Splitting SIMD16 Message Data Payload (MDP at offset = MDPOfst) for A64
// scatter/untyped write messages to two SIMD8 MDPs (V0 and V1).
void CEncoder::SplitMDP16To8(CVariable* MDP, uint32_t MDPOfst, uint32_t NumBlks, CVariable* V0, CVariable* V1)
{
    VISA_GenVar* GV = GetVISAVariable(MDP);
    VISA_GenVar* v0GV = GetVISAVariable(V0);
    VISA_GenVar* v1GV = GetVISAVariable(V1);
    VISA_VectorOpnd *movDst0, *movDst1, *srcOpnd;
    const Common_ISA_Exec_Size fromESize = EXEC_SIZE_16;
    const Common_ISA_Exec_Size toESize = EXEC_SIZE_8;
    const uint32_t eltBytes = MDP->GetElemSize();
    assert(eltBytes == V0->GetElemSize() && eltBytes == V1->GetElemSize() &&
        "Element size should be the same among SIMD16 MDP and SIMD8 MDP!");
    // Number of elements per GRF

    if (eltBytes > 0)
    {
        const uint32_t GRFElts = SIZE_GRF / eltBytes;

        if (GRFElts > 0)
        {
            Common_VISA_EMask_Ctrl execNM = ConvertMaskToVisaType(m_encoderState.m_mask, m_encoderState.m_noMask);
            uint32_t MDPStart = MDPOfst / eltBytes;
            for (uint32_t i = 0; i < NumBlks; ++i)
            {
                uint32_t dstOfst = i * 8;
                uint32_t srcOfst = i * 16 + MDPStart;
                V(vKernel->CreateVISADstOperand(movDst0, v0GV, 1, dstOfst / GRFElts, dstOfst % GRFElts));
                V(vKernel->CreateVISADstOperand(movDst1, v1GV, 1, dstOfst / GRFElts, dstOfst % GRFElts));

                V(vKernel->CreateVISASrcOperand(srcOpnd, GV, MODIFIER_NONE,
                    1, 1, 0, srcOfst / GRFElts, srcOfst % GRFElts));

                V(vKernel->AppendVISADataMovementInst(
                    ISA_MOV, nullptr, false,
                    SplitEMask(fromESize, toESize, 0, execNM),
                    toESize, movDst0, srcOpnd));

                srcOfst += 8;
                V(vKernel->CreateVISASrcOperand(srcOpnd, GV, MODIFIER_NONE,
                    1, 1, 0, srcOfst / GRFElts, srcOfst % GRFElts));

                V(vKernel->AppendVISADataMovementInst(
                    ISA_MOV, nullptr, false,
                    SplitEMask(fromESize, toESize, 1, execNM),
                    toESize, movDst1, srcOpnd));
            }
        }
    }
}

// Merge two SIMD8 MDP (V0 and V1) into a single SIMD16 MDP (MDP at offset = MDPOfst)
void CEncoder::MergeMDP8To16(CVariable* V0, CVariable* V1, uint32_t NumBlks, CVariable* MDP, uint32_t MDPOfst)
{
    VISA_GenVar* GV = GetVISAVariable(MDP);
    VISA_GenVar* v0GV = GetVISAVariable(V0);
    VISA_GenVar* v1GV = GetVISAVariable(V1);
    VISA_VectorOpnd *movDst, *movSrc0, *movSrc1;
    const Common_ISA_Exec_Size fromESize = EXEC_SIZE_16;
    const Common_ISA_Exec_Size toESize = EXEC_SIZE_8;
    const uint32_t eltBytes = MDP->GetElemSize();
    assert(eltBytes == V0->GetElemSize() && eltBytes == V1->GetElemSize() &&
           "Element size should be the same among SIMD16 MDP and SIMD8 MDP!");

    if (eltBytes > 0)
    {
        // Number of elements per GRF
        const uint32_t GRFElts = SIZE_GRF / eltBytes;

        if (GRFElts > 0)
        {
            Common_VISA_EMask_Ctrl execNM = ConvertMaskToVisaType(m_encoderState.m_mask, m_encoderState.m_noMask);
            uint32_t MDPStart = MDPOfst / eltBytes;
            for (uint32_t i = 0; i < NumBlks; ++i)
            {
                uint32_t dstOfst = i * 16 + MDPStart;
                uint32_t srcOfst = i * 8;
                V(vKernel->CreateVISADstOperand(movDst, GV, 1, dstOfst / GRFElts, dstOfst % GRFElts));
                V(vKernel->CreateVISASrcOperand(movSrc0, v0GV, MODIFIER_NONE,
                    1, 1, 0, srcOfst / GRFElts, srcOfst % GRFElts));
                V(vKernel->CreateVISASrcOperand(movSrc1, v1GV, MODIFIER_NONE,
                    1, 1, 0, srcOfst / GRFElts, srcOfst % GRFElts));

                V(vKernel->AppendVISADataMovementInst(
                    ISA_MOV, nullptr, false,
                    SplitEMask(fromESize, toESize, 0, execNM),
                    toESize, movDst, movSrc0));

                dstOfst += 8;
                V(vKernel->CreateVISADstOperand(movDst, GV, 1, dstOfst / GRFElts, dstOfst % GRFElts));
                V(vKernel->AppendVISADataMovementInst(
                    ISA_MOV, nullptr, false,
                    SplitEMask(fromESize, toESize, 1, execNM),
                    toESize, movDst, movSrc1));
            }
        }
    }
}

static SModifier
EmulateVariable(CVariable *Var, SModifier Mod, bool IsHiPart, bool IsSource) {
    if (Mod.specialRegion) {
        if (IsSource) {
            Mod.region[0] *= 2;
            Mod.region[2] *= 2;
        } else
            Mod.region[2] *= 2;
    } else {
        if (IsSource) {
            if (!Var->IsUniform()) {
                Mod.region[0] = 2;
                Mod.region[1] = 1;
                Mod.region[2] = 0;
                Mod.specialRegion = true;
            }
        } else {
            Mod.region[2] = 2;
            Mod.specialRegion = true;
        }
    }
    Mod.subReg *= 2;
    if (IsHiPart)
        Mod.subReg += 1;
    return Mod;
}

void CEncoder::DataMov(ISA_Opcode opcode, CVariable* dst, CVariable* src)
{
    if(opcode == ISA_SETP)
    {
        assert(dst->GetVarType() == EVARTYPE_PREDICATE);
        V(vKernel->AppendVISASetP(
            GetAluEMask(dst),
            IsSecondHalf() ? GetAluExecSize(dst) : visaExecSize(m_program->m_dispatchSize),
            dst->visaPredVariable,
            GetSourceOperand(src, m_encoderState.m_srcOperand[0])));
    }
    else if(opcode == ISA_MOV && src->GetVarType() == EVARTYPE_PREDICATE)
    {
        V(vKernel->AppendVISAPredicateMove(
            GetDestinationOperand(dst, m_encoderState.m_dstOperand),
            src->visaPredVariable));
    }
    else
    {
        VISA_Type dstT = dst->GetType();
        VISA_Type srcT = src->GetType();
        bool Is64BitDst = (dstT == ISA_TYPE_Q || dstT == ISA_TYPE_UQ);
        bool Is64BitSrc = (srcT == ISA_TYPE_Q || srcT == ISA_TYPE_UQ);
        bool Need64BitEmu =
            m_program->GetContext()->platform.hasNo64BitInst() &&
            (Is64BitDst || Is64BitSrc);
        // If DP is not supported, need to split mov as well.
        if (IGC_IS_FLAG_ENABLED(ForceDPEmulation) ||
            !m_program->GetContext()->platform.supportFP64())
        {
            if (dstT == ISA_TYPE_DF && srcT == ISA_TYPE_DF)
            {
                Need64BitEmu = true;
                Is64BitDst = true;
                Is64BitSrc = true;
            }
            else
            {
                // No Double type expected here.
                assert((dstT != ISA_TYPE_DF && srcT != ISA_TYPE_DF) &&
                    "Double type unexpected here!");
            }
        }

        CVariable *dstAlias = nullptr;
        CVariable *srcAlias = nullptr;
        VISA_VectorOpnd *srcImmLo = nullptr;
        VISA_VectorOpnd *srcImmHi = nullptr;
        if (Need64BitEmu) {
            if (Is64BitDst)
                dstAlias = m_program->GetNewAlias(dst, ISA_TYPE_UD, 0, 0);
            else
                dstAlias = dst;
            if (src->IsImmediate()) {
                uint64_t Imm = src->GetImmediateValue();
                unsigned ImmLo = Imm & 0xFFFFFFFFULL;
                unsigned ImmHi = Imm >> 32;
                V(vKernel->CreateVISAImmediate(srcImmLo, &ImmLo, ISA_TYPE_UD));
                V(vKernel->CreateVISAImmediate(srcImmHi, &ImmHi, ISA_TYPE_UD));
            } else {
                if (Is64BitSrc)
                    srcAlias = m_program->GetNewAlias(src, ISA_TYPE_UD, 0, 0);
                else
                    srcAlias = src;
            }
        }
        unsigned numParts = 0;
        if (NeedSplitting(dst, m_encoderState.m_dstOperand, numParts) ||
            NeedSplitting(src, m_encoderState.m_srcOperand[0], numParts, true)) {

            Common_VISA_EMask_Ctrl execMask = GetAluEMask(dst);
            Common_ISA_Exec_Size fromExecSize = GetAluExecSize(dst);
            Common_ISA_Exec_Size toExecSize = SplitExecSize(fromExecSize, numParts);

            for (unsigned thePart = 0; thePart != numParts; ++thePart) {
                SModifier newDstMod = SplitVariable(fromExecSize, toExecSize, thePart, dst, m_encoderState.m_dstOperand);
                SModifier newSrcMod = SplitVariable(fromExecSize, toExecSize, thePart, src, m_encoderState.m_srcOperand[0], true);
                if (Need64BitEmu) {
                    if (Is64BitSrc && Is64BitDst) {
                        // Generate data movement on Lo part.
                        SModifier LoDstMod = EmulateVariable(dst, newDstMod, false, false);
                        SModifier LoSrcMod = EmulateVariable(src, newSrcMod, false, true);
                        VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dstAlias, LoDstMod);
                        VISA_VectorOpnd* srcOpnd = srcImmLo ? srcImmLo : GetSourceOperand(srcAlias, LoSrcMod);
                        VISA_PredOpnd* predOpnd  = GetFlagOperand(m_encoderState.m_flag);
                        V(vKernel->AppendVISADataMovementInst(opcode, predOpnd, IsSat(),
                            SplitEMask(fromExecSize, toExecSize, thePart, execMask),
                            toExecSize,
                            dstOpnd, srcOpnd));
                        // Generate data movement on Hi part.
                        SModifier HiDstMod = EmulateVariable(dst, newDstMod, true, false);
                        SModifier HiSrcMod = EmulateVariable(src, newSrcMod, true, true);
                        dstOpnd = GetDestinationOperand(dstAlias, HiDstMod);
                        srcOpnd = srcImmHi ? srcImmHi : GetSourceOperand(srcAlias, HiSrcMod);
                        predOpnd  = GetFlagOperand(m_encoderState.m_flag);
                        V(vKernel->AppendVISADataMovementInst(opcode, predOpnd, IsSat(),
                            SplitEMask(fromExecSize, toExecSize, thePart, execMask),
                            toExecSize,
                            dstOpnd, srcOpnd));
                    } else if (Is64BitSrc) {
                        assert(!Is64BitDst && "Expect non 64-bit dst!");
                        // Generate data movement on Lo part only.
                        SModifier LoSrcMod = EmulateVariable(src, newSrcMod, false, true);
                        VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dstAlias, newDstMod);
                        VISA_VectorOpnd* srcOpnd = srcImmLo ? srcImmLo : GetSourceOperand(srcAlias, LoSrcMod);
                        VISA_PredOpnd* predOpnd  = GetFlagOperand(m_encoderState.m_flag);
                        V(vKernel->AppendVISADataMovementInst(opcode, predOpnd, IsSat(),
                            SplitEMask(fromExecSize, toExecSize, thePart, execMask),
                            toExecSize,
                            dstOpnd, srcOpnd));
                    } else {
                        assert(Is64BitDst && !Is64BitSrc && "Expect non 64-bit src and 64-bit dst!");
                        // Generate data movement on Lo part.
                        SModifier LoDstMod = EmulateVariable(dst, newDstMod, false, false);
                        VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dstAlias, LoDstMod);
                        VISA_VectorOpnd* srcOpnd = srcImmLo ? srcImmLo : GetSourceOperand(srcAlias, newSrcMod);
                        VISA_PredOpnd* predOpnd  = GetFlagOperand(m_encoderState.m_flag);
                        V(vKernel->AppendVISADataMovementInst(opcode, predOpnd, IsSat(),
                            SplitEMask(fromExecSize, toExecSize, thePart, execMask),
                            toExecSize,
                            dstOpnd, srcOpnd));
                        // Generate data movement on Hi part.
                        unsigned ImmHi = 0U;
                        V(vKernel->CreateVISAImmediate(srcImmHi, &ImmHi, ISA_TYPE_UD));
                        SModifier HiDstMod = EmulateVariable(dst, newDstMod, true, false);
                        dstOpnd = GetDestinationOperand(dstAlias, HiDstMod);
                        srcOpnd = srcImmHi;
                        predOpnd  = GetFlagOperand(m_encoderState.m_flag);
                        V(vKernel->AppendVISADataMovementInst(opcode, predOpnd, IsSat(),
                            SplitEMask(fromExecSize, toExecSize, thePart, execMask),
                            toExecSize,
                            dstOpnd, srcOpnd));
                    }
                } else {
                    VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dst, newDstMod);
                    VISA_VectorOpnd* srcOpnd = GetSourceOperand(src, newSrcMod);
                    VISA_PredOpnd* predOpnd  = GetFlagOperand(m_encoderState.m_flag);
                    V(vKernel->AppendVISADataMovementInst(opcode, predOpnd, IsSat(),
                        SplitEMask(fromExecSize, toExecSize, thePart, execMask),
                        toExecSize,
                        dstOpnd, srcOpnd));
                }
            }
        } else {
            if (Need64BitEmu) {
                if (Is64BitSrc && Is64BitDst) {
                    // Generate data movement on Lo part.
                    SModifier LoDstMod = EmulateVariable(dst, m_encoderState.m_dstOperand, false, false);
                    SModifier LoSrcMod = EmulateVariable(src, m_encoderState.m_srcOperand[0], false, true);
                    VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dstAlias, LoDstMod);
                    VISA_VectorOpnd* srcOpnd = srcImmLo ? srcImmLo : GetSourceOperand(srcAlias, LoSrcMod);
                    VISA_PredOpnd* predOpnd  = GetFlagOperand(m_encoderState.m_flag);
                    V(vKernel->AppendVISADataMovementInst(opcode, predOpnd, IsSat(),
                        GetAluEMask(dst),
                        GetAluExecSize(dst),
                        dstOpnd, srcOpnd));
                    // Generate data movement on Hi part.
                    SModifier HiDstMod = EmulateVariable(dst, m_encoderState.m_dstOperand, true, false);
                    SModifier HiSrcMod = EmulateVariable(src, m_encoderState.m_srcOperand[0], true, true);
                    dstOpnd = GetDestinationOperand(dstAlias, HiDstMod);
                    srcOpnd = srcImmHi ? srcImmHi : GetSourceOperand(srcAlias, HiSrcMod);
                    predOpnd  = GetFlagOperand(m_encoderState.m_flag);
                    V(vKernel->AppendVISADataMovementInst(opcode, predOpnd, IsSat(),
                        GetAluEMask(dst),
                        GetAluExecSize(dst),
                        dstOpnd, srcOpnd));
                } else if (Is64BitSrc) {
                    assert(!Is64BitDst && "Expect non 64-bit dst!");
                    // Generate data movement on Lo part only.
                    SModifier LoSrcMod = EmulateVariable(src, m_encoderState.m_srcOperand[0], false, true);
                    VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dstAlias, m_encoderState.m_dstOperand);
                    VISA_VectorOpnd* srcOpnd = srcImmLo ? srcImmLo : GetSourceOperand(srcAlias, LoSrcMod);
                    VISA_PredOpnd* predOpnd  = GetFlagOperand(m_encoderState.m_flag);
                    V(vKernel->AppendVISADataMovementInst(opcode, predOpnd, IsSat(),
                        GetAluEMask(dst),
                        GetAluExecSize(dst),
                        dstOpnd, srcOpnd));
                } else {
                    assert(Is64BitDst && !Is64BitSrc && "Expect non 64-bit src and 64-bit dst!");
                    // Generate data movement on Lo part.
                    SModifier LoDstMod = EmulateVariable(dst, m_encoderState.m_dstOperand, false, false);
                    VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dstAlias, LoDstMod);
                    VISA_VectorOpnd* srcOpnd = srcImmLo ? srcImmLo : GetSourceOperand(srcAlias, m_encoderState.m_srcOperand[0]);
                    VISA_PredOpnd* predOpnd  = GetFlagOperand(m_encoderState.m_flag);
                    V(vKernel->AppendVISADataMovementInst(opcode, predOpnd, IsSat(),
                        GetAluEMask(dst),
                        GetAluExecSize(dst),
                        dstOpnd, srcOpnd));
                    // Generate data movement on Hi part.
                    unsigned ImmHi = 0U;
                    V(vKernel->CreateVISAImmediate(srcImmHi, &ImmHi, ISA_TYPE_UD));
                    SModifier HiDstMod = EmulateVariable(dst, m_encoderState.m_dstOperand, true, false);
                    dstOpnd = GetDestinationOperand(dstAlias, HiDstMod);
                    srcOpnd = srcImmHi;
                    predOpnd  = GetFlagOperand(m_encoderState.m_flag);
                    V(vKernel->AppendVISADataMovementInst(opcode, predOpnd, IsSat(),
                        GetAluEMask(dst),
                        GetAluExecSize(dst),
                        dstOpnd, srcOpnd));
                }
            } else {
                VISA_VectorOpnd* srcOpnd = GetSourceOperand(src, m_encoderState.m_srcOperand[0]);
                VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dst, m_encoderState.m_dstOperand);
                VISA_PredOpnd* predOpnd  = GetFlagOperand(m_encoderState.m_flag);
                V(vKernel->AppendVISADataMovementInst(
                    opcode,
                    predOpnd,
                    IsSat(),
                    GetAluEMask(dst),
                    GetAluExecSize(dst),
                    dstOpnd,
                    srcOpnd));
            }
        }
    }
}

void CEncoder::LogicOp(
    ISA_Opcode opcode,
    CVariable* dst,
    CVariable* src0,
    CVariable* src1,
    CVariable* src2,
    CVariable* src3)
{
    if(dst->GetVarType() == EVARTYPE_PREDICATE ||
        src0->GetVarType() == EVARTYPE_PREDICATE ||
        (src1 != nullptr && src1->GetVarType() == EVARTYPE_PREDICATE))
    {
        VISA_PredVar* src1Dcl = NULL;
        if(src1 != NULL)
            src1Dcl = src1->visaPredVariable;

        // Try to use NOT instruction for predicate, we won't have phi on
        // predicate since Legalization pass convert i1 phi to i32.
        if (opcode == ISA_NOT)
            SetNoMask();

        V(vKernel->AppendVISALogicOrShiftInst(
            opcode,
            GetAluEMask(dst),
            GetAluExecSize(dst),
            dst->visaPredVariable,
            src0->visaPredVariable,
            src1Dcl));
    }
    else
    {
        unsigned numParts = 0;
        if (NeedSplitting(dst, m_encoderState.m_dstOperand, numParts) ||
            NeedSplitting(src0, m_encoderState.m_srcOperand[0], numParts, true) ||
            NeedSplitting(src1, m_encoderState. m_srcOperand[1], numParts, true) ||
            NeedSplitting(src2, m_encoderState.m_srcOperand[2], numParts, true) ||
            NeedSplitting(src3, m_encoderState.m_srcOperand[3], numParts, true)) {

            Common_VISA_EMask_Ctrl execMask = GetAluEMask(dst);
            Common_ISA_Exec_Size fromExecSize = GetAluExecSize(dst);
            Common_ISA_Exec_Size toExecSize = SplitExecSize(fromExecSize, numParts);

            for (unsigned thePart = 0; thePart != numParts; ++thePart) {
                SModifier newDstMod = SplitVariable(fromExecSize, toExecSize, thePart, dst, m_encoderState.m_dstOperand);
                SModifier newSrc0Mod = SplitVariable(fromExecSize, toExecSize, thePart, src0, m_encoderState.m_srcOperand[0], true);
                SModifier newSrc1Mod = SplitVariable(fromExecSize, toExecSize, thePart, src1, m_encoderState.m_srcOperand[1], true);
                SModifier newSrc2Mod = SplitVariable(fromExecSize, toExecSize, thePart, src2, m_encoderState.m_srcOperand[2], true);
                SModifier newSrc3Mod = SplitVariable(fromExecSize, toExecSize, thePart, src3, m_encoderState.m_srcOperand[3], true);
                VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dst, newDstMod);
                VISA_VectorOpnd* srcOpnd0 = GetSourceOperand(src0, newSrc0Mod);
                VISA_VectorOpnd* srcOpnd1 = GetSourceOperand(src1, newSrc1Mod);
                VISA_VectorOpnd* srcOpnd2 = GetSourceOperand(src2, newSrc2Mod);
                VISA_VectorOpnd* srcOpnd3 = GetSourceOperand(src3, newSrc3Mod);
                VISA_PredOpnd* predOpnd  = GetFlagOperand(m_encoderState.m_flag);
                V(vKernel->AppendVISALogicOrShiftInst(opcode, predOpnd, IsSat(),
                    SplitEMask(fromExecSize, toExecSize, thePart, execMask),
                    toExecSize,
                    dstOpnd, srcOpnd0, srcOpnd1, srcOpnd2, srcOpnd3));
            }
        } else {
            VISA_VectorOpnd* srcOpnd0 = GetSourceOperand(src0, m_encoderState.m_srcOperand[0]);
            VISA_VectorOpnd* srcOpnd1 = GetSourceOperand(src1, m_encoderState.m_srcOperand[1]);
            VISA_VectorOpnd* srcOpnd2 = GetSourceOperand(src2, m_encoderState.m_srcOperand[2]);
            VISA_VectorOpnd* srcOpnd3 = GetSourceOperand(src3, m_encoderState.m_srcOperand[3]);
            VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dst, m_encoderState.m_dstOperand);
            VISA_PredOpnd* predOpnd  = GetFlagOperand(m_encoderState.m_flag);

            V(vKernel->AppendVISALogicOrShiftInst(
                opcode,
                predOpnd,
                IsSat(),
                GetAluEMask(dst),
                GetAluExecSize(dst),
                dstOpnd,
                srcOpnd0,
                srcOpnd1,
                srcOpnd2,
                srcOpnd3));
        }
    }
}

void CEncoder::Arithmetic(ISA_Opcode opcode, CVariable* dst, CVariable* src0, CVariable*src1, CVariable* src2)
{
    unsigned numParts = 0;
    if (NeedSplitting(dst, m_encoderState.m_dstOperand, numParts) ||
        NeedSplitting(src0, m_encoderState.m_srcOperand[0], numParts, true) ||
        NeedSplitting(src1, m_encoderState.m_srcOperand[1], numParts, true) ||
        NeedSplitting(src2, m_encoderState.m_srcOperand[2], numParts, true)) {

        Common_VISA_EMask_Ctrl execMask = GetAluEMask(dst);
        Common_ISA_Exec_Size fromExecSize = GetAluExecSize(dst);
        Common_ISA_Exec_Size toExecSize = SplitExecSize(fromExecSize, numParts);

        for (unsigned thePart = 0; thePart != numParts; ++thePart) {
            SModifier newDstMod = SplitVariable(fromExecSize, toExecSize, thePart, dst, m_encoderState.m_dstOperand);
            SModifier newSrc0Mod = SplitVariable(fromExecSize, toExecSize, thePart, src0, m_encoderState.m_srcOperand[0], true);
            SModifier newSrc1Mod = SplitVariable(fromExecSize, toExecSize, thePart, src1, m_encoderState.m_srcOperand[1], true);
            SModifier newSrc2Mod = SplitVariable(fromExecSize, toExecSize, thePart, src2, m_encoderState.m_srcOperand[2], true);
            VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dst, newDstMod);
            VISA_VectorOpnd* srcOpnd0 = GetSourceOperand(src0, newSrc0Mod);
            VISA_VectorOpnd* srcOpnd1 = GetSourceOperand(src1, newSrc1Mod);
            VISA_VectorOpnd* srcOpnd2 = GetSourceOperand(src2, newSrc2Mod);
            VISA_PredOpnd* predOpnd  = GetFlagOperand(m_encoderState.m_flag);
            V(vKernel->AppendVISAArithmeticInst(opcode, predOpnd, IsSat(),
                SplitEMask(fromExecSize, toExecSize, thePart, execMask),
                toExecSize,
                dstOpnd, srcOpnd0, srcOpnd1, srcOpnd2));
        }
    } else {
        VISA_VectorOpnd* srcOpnd0 = GetSourceOperand(src0, m_encoderState.m_srcOperand[0]);
        VISA_VectorOpnd* srcOpnd1 = GetSourceOperand(src1, m_encoderState.m_srcOperand[1]);
        VISA_VectorOpnd* srcOpnd2 = GetSourceOperand(src2, m_encoderState.m_srcOperand[2]);
        VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dst, m_encoderState.m_dstOperand);
        VISA_PredOpnd* predOpnd  = GetFlagOperand(m_encoderState.m_flag);
        V(vKernel->AppendVISAArithmeticInst(
            opcode,
            predOpnd,
            IsSat(),
            GetAluEMask(dst),
            GetAluExecSize(dst),
            dstOpnd,
            srcOpnd0,
            srcOpnd1,
            srcOpnd2));
    }
}


void CEncoder::AddPair(CVariable *Lo, CVariable *Hi, CVariable *L0, CVariable *H0, CVariable *L1, CVariable *H1) {
    assert(m_encoderState.m_dstOperand.mod == EMOD_NONE && "addPair doesn't support saturate");

    if (Hi == nullptr) {
        // When Hi part is ignored, reduce 64-bit subtraction into 32-bit.
        GenericAlu(EOPCODE_ADD, Lo, L0, L1);
        return;
    }

    if (Lo == nullptr) {
        // We cannot reduce the strength if only Lo is ignored.
        Lo = m_program->GetNewVariable(Hi->GetNumberElement(), Hi->GetType(), Hi->GetAlign(), Hi->IsUniform());
    }

    // Use `UD` only.
    if (Lo->GetType() != ISA_TYPE_UD && Lo->GetType() != ISA_TYPE_UV) Lo = m_program->BitCast(Lo, ISA_TYPE_UD);
    if (Hi->GetType() != ISA_TYPE_UD && Hi->GetType() != ISA_TYPE_UV) Hi = m_program->BitCast(Hi, ISA_TYPE_UD);
    if (L0->GetType() != ISA_TYPE_UD && L0->GetType() != ISA_TYPE_UV) L0 = m_program->BitCast(L0, ISA_TYPE_UD);
    if (H0->GetType() != ISA_TYPE_UD && H0->GetType() != ISA_TYPE_UV) H0 = m_program->BitCast(H0, ISA_TYPE_UD);
    if (L1->GetType() != ISA_TYPE_UD && L1->GetType() != ISA_TYPE_UV) L1 = m_program->BitCast(L1, ISA_TYPE_UD);
    if (H1->GetType() != ISA_TYPE_UD && H1->GetType() != ISA_TYPE_UV) H1 = m_program->BitCast(H1, ISA_TYPE_UD);

    Common_ISA_Exec_Size ExecSize = GetAluExecSize(Lo);
    assert(ExecSize == EXEC_SIZE_16 || ExecSize == EXEC_SIZE_8 ||
           ExecSize == EXEC_SIZE_4 || ExecSize == EXEC_SIZE_2 ||
           ExecSize == EXEC_SIZE_1);

    if (ExecSize == EXEC_SIZE_16) {
        // Have to split it because `acc0` has only 8 elements for 32-bit
        // integer types.
        unsigned NumParts = 2;
        Common_VISA_EMask_Ctrl ExecMask = GetAluEMask(Lo);
        Common_ISA_Exec_Size FromExecSize = GetAluExecSize(Lo);
        Common_ISA_Exec_Size ToExecSize = SplitExecSize(FromExecSize, NumParts);

        VISA_PredOpnd *Pred  = GetFlagOperand(m_encoderState.m_flag);
        for (unsigned ThePart = 0; ThePart != NumParts; ++ThePart) {
            SModifier NewDstMod = SplitVariable(FromExecSize, ToExecSize, ThePart, Lo, m_encoderState.m_dstOperand);
            SModifier NewS0LMod = SplitVariable(FromExecSize, ToExecSize, ThePart, L0, m_encoderState.m_srcOperand[0], true);
            SModifier NewS0HMod = SplitVariable(FromExecSize, ToExecSize, ThePart, H0, m_encoderState.m_srcOperand[1], true);
            SModifier NewS1LMod = SplitVariable(FromExecSize, ToExecSize, ThePart, L1, m_encoderState.m_srcOperand[2], true);
            SModifier NewS1HMod = SplitVariable(FromExecSize, ToExecSize, ThePart, H1, m_encoderState.m_srcOperand[3], true);
            VISA_VectorOpnd *S0L = GetSourceOperand(L0, NewS0LMod);
            VISA_VectorOpnd *S0H = GetSourceOperand(H0, NewS0HMod);
            VISA_VectorOpnd *S1L = GetSourceOperand(L1, NewS1LMod);
            VISA_VectorOpnd *S1H = GetSourceOperand(H1, NewS1HMod);
            VISA_VectorOpnd *L = GetDestinationOperand(Lo, NewDstMod);
            VISA_VectorOpnd *H = GetDestinationOperand(Hi, NewDstMod);
            VISA_VectorOpnd *HIn = GetSourceOperand(Hi, NewDstMod);

            unsigned NumElems = 8;
            CVariable *Carry = m_program->GetNewVariable((uint16_t)NumElems, Lo->GetType(), Lo->GetAlign(), Lo->IsUniform());
            VISA_VectorOpnd *AccOut = GetDestinationOperand(Carry, m_encoderState.m_dstOperand);
            VISA_VectorOpnd *AccIn = GetSourceOperand(Carry, m_encoderState.m_dstOperand);

            Common_VISA_EMask_Ctrl EMask = SplitEMask(FromExecSize, ToExecSize, ThePart, ExecMask);
            V(vKernel->AppendVISAArithmeticInst(
                ISA_ADDC, Pred, EMask, ToExecSize,
                L, AccOut, S0L, S1L));
            V(vKernel->AppendVISAArithmeticInst(
                ISA_ADD, Pred, false, EMask, ToExecSize,
                H, S0H, S1H));
            H = GetDestinationOperand(Hi, NewDstMod);
            V(vKernel->AppendVISAArithmeticInst(
                ISA_ADD, Pred, false, EMask, ToExecSize,
                H, AccIn, HIn));
        }
    } else {
        VISA_VectorOpnd *S0L = GetSourceOperand(L0, m_encoderState.m_srcOperand[0]);
        VISA_VectorOpnd *S0H = GetSourceOperand(H0, m_encoderState.m_srcOperand[1]);
        VISA_VectorOpnd *S1L = GetSourceOperand(L1, m_encoderState.m_srcOperand[2]);
        VISA_VectorOpnd *S1H = GetSourceOperand(H1, m_encoderState.m_srcOperand[3]);
        VISA_VectorOpnd *L = GetDestinationOperand(Lo, m_encoderState.m_dstOperand);
        VISA_VectorOpnd *H = GetDestinationOperand(Hi, m_encoderState.m_dstOperand);
        VISA_PredOpnd *Pred  = GetFlagOperand(m_encoderState.m_flag);

        unsigned short NumElems = (ExecSize == EXEC_SIZE_1) ? 1 :
                            (ExecSize == EXEC_SIZE_2) ? 2 :
                            (ExecSize == EXEC_SIZE_4) ? 4 : 8;
        CVariable *Carry = m_program->GetNewVariable(NumElems, Lo->GetType(), Lo->GetAlign(), Lo->IsUniform());
        VISA_VectorOpnd *AccOut = GetDestinationOperand(Carry, m_encoderState.m_dstOperand);

        SModifier MidMod = m_encoderState.m_dstOperand;
        if (Lo->IsUniform() && NumElems != 1) {
            MidMod.region[0] = 1;
            MidMod.region[1] = 1;
            MidMod.region[2] = 0;
            MidMod.specialRegion = true;
        }
        VISA_VectorOpnd *HIn = GetSourceOperand(Hi, MidMod);
        VISA_VectorOpnd *AccIn = GetSourceOperand(Carry, MidMod);

        Common_VISA_EMask_Ctrl ExecMask = GetAluEMask(Lo);
        V(vKernel->AppendVISAArithmeticInst(
            ISA_ADDC, Pred, ExecMask, ExecSize,
            L, AccOut, S0L, S1L));
        V(vKernel->AppendVISAArithmeticInst(
            ISA_ADD, Pred, false, ExecMask, ExecSize,
            H, S0H, S1H));
        H = GetDestinationOperand(Hi, m_encoderState.m_dstOperand);
        V(vKernel->AppendVISAArithmeticInst(
            ISA_ADD, Pred, false, ExecMask, ExecSize,
            H, AccIn, HIn));
    }
}

void CEncoder::SubPair(CVariable *Lo, CVariable *Hi, CVariable *L0, CVariable *H0, CVariable *L1, CVariable *H1) {
    assert(m_encoderState.m_dstOperand.mod == EMOD_NONE && "subPair doesn't support saturate");

    Common_ISA_Exec_Size ExecSize = GetAluExecSize(Lo);
    assert(ExecSize == EXEC_SIZE_16 || ExecSize == EXEC_SIZE_8 || ExecSize == EXEC_SIZE_1);

    if (Hi == nullptr) {
        // When Hi part is ignored, reduce 64-bit subtraction into 32-bit.
        SetSrcModifier(1, EMOD_NEG);
        GenericAlu(EOPCODE_ADD, Lo, L0, L1);
        return;
    }

    if (Lo == nullptr) {
        // We cannot reduce the strength if only Lo is ignored.
        Lo = m_program->GetNewVariable(Hi->GetNumberElement(), Hi->GetType(), Hi->GetAlign(), Hi->IsUniform());
    }

    // Use `UD` only.
    if (Lo->GetType() != ISA_TYPE_UD && Lo->GetType() != ISA_TYPE_UV) Lo = m_program->BitCast(Lo, ISA_TYPE_UD);
    if (Hi->GetType() != ISA_TYPE_UD && Hi->GetType() != ISA_TYPE_UV) Hi = m_program->BitCast(Hi, ISA_TYPE_UD);
    if (L0->GetType() != ISA_TYPE_UD && L0->GetType() != ISA_TYPE_UV) L0 = m_program->BitCast(L0, ISA_TYPE_UD);
    if (H0->GetType() != ISA_TYPE_UD && H0->GetType() != ISA_TYPE_UV) H0 = m_program->BitCast(H0, ISA_TYPE_UD);
    if (L1->GetType() != ISA_TYPE_UD && L1->GetType() != ISA_TYPE_UV) L1 = m_program->BitCast(L1, ISA_TYPE_UD);
    if (H1->GetType() != ISA_TYPE_UD && H1->GetType() != ISA_TYPE_UV) H1 = m_program->BitCast(H1, ISA_TYPE_UD);

    if (ExecSize == EXEC_SIZE_16) {
        // Have to split it because `acc0` has only 8 elements for 32-bit
        // integer types.
        unsigned NumParts = 2;
        Common_VISA_EMask_Ctrl ExecMask = GetAluEMask(Lo);
        Common_ISA_Exec_Size FromExecSize = GetAluExecSize(Lo);
        Common_ISA_Exec_Size ToExecSize = SplitExecSize(FromExecSize, NumParts);

        // Negative `S1H`
        SModifier S1HMod = m_encoderState.m_srcOperand[1];
        assert(S1HMod.mod == EMOD_NONE);
        S1HMod.mod = EMOD_NEG;
        VISA_PredOpnd *Pred  = GetFlagOperand(m_encoderState.m_flag);
        for (unsigned ThePart = 0; ThePart != NumParts; ++ThePart) {
            SModifier NewDstMod = SplitVariable(FromExecSize, ToExecSize, ThePart, Lo, m_encoderState.m_dstOperand);
            SModifier NewS0LMod = SplitVariable(FromExecSize, ToExecSize, ThePart, L0, m_encoderState.m_srcOperand[0], true);
            SModifier NewS0HMod = SplitVariable(FromExecSize, ToExecSize, ThePart, H0, m_encoderState.m_srcOperand[1], true);
            SModifier NewS1LMod = SplitVariable(FromExecSize, ToExecSize, ThePart, L1, m_encoderState.m_srcOperand[2], true);
            SModifier NewS1HMod = SplitVariable(FromExecSize, ToExecSize, ThePart, H1, S1HMod, true);
            VISA_VectorOpnd *S0L = GetSourceOperand(L0, NewS0LMod);
            VISA_VectorOpnd *S0H = GetSourceOperand(H0, NewS0HMod);
            VISA_VectorOpnd *S1L = GetSourceOperand(L1, NewS1LMod);
            VISA_VectorOpnd *S1H = GetSourceOperand(H1, NewS1HMod);
            VISA_VectorOpnd *L = GetDestinationOperand(Lo, NewDstMod);
            VISA_VectorOpnd *H = GetDestinationOperand(Hi, NewDstMod);
            VISA_VectorOpnd *HIn = GetSourceOperand(Hi, NewDstMod);

            unsigned short NumElems = 8;
            CVariable *Carry = m_program->GetNewVariable(NumElems, Lo->GetType(), Lo->GetAlign(), Lo->IsUniform());
            VISA_VectorOpnd *AccOut = GetDestinationOperand(Carry, m_encoderState.m_dstOperand);
            // Negative `Acc0`
            SModifier AccMod = m_encoderState.m_dstOperand;
            assert(AccMod.mod == EMOD_NONE);
            AccMod.mod = EMOD_NEG;
            VISA_VectorOpnd *AccIn = GetSourceOperand(Carry, AccMod);

            Common_VISA_EMask_Ctrl EMask = SplitEMask(FromExecSize, ToExecSize, ThePart, ExecMask);
            V(vKernel->AppendVISAArithmeticInst(
                ISA_SUBB, Pred, EMask, ToExecSize,
                L, AccOut, S0L, S1L));
            V(vKernel->AppendVISAArithmeticInst(
                ISA_ADD, Pred, false, EMask, ToExecSize,
                H, S0H, S1H));
            H = GetDestinationOperand(Hi, NewDstMod);
            V(vKernel->AppendVISAArithmeticInst(
                ISA_ADD, Pred, false, EMask, ToExecSize,
                H, AccIn, HIn));
        }
    } else {
        VISA_VectorOpnd *S0L = GetSourceOperand(L0, m_encoderState.m_srcOperand[0]);
        VISA_VectorOpnd *S0H = GetSourceOperand(H0, m_encoderState.m_srcOperand[1]);
        VISA_VectorOpnd *S1L = GetSourceOperand(L1, m_encoderState.m_srcOperand[2]);
        // Negative `S0H`
        SModifier S1HMod = m_encoderState.m_srcOperand[1];
        assert(S1HMod.mod == EMOD_NONE);
        S1HMod.mod = EMOD_NEG;
        VISA_VectorOpnd *S1H = GetSourceOperand(H1, S1HMod);
        VISA_VectorOpnd *L = GetDestinationOperand(Lo, m_encoderState.m_dstOperand);
        VISA_VectorOpnd *H = GetDestinationOperand(Hi, m_encoderState.m_dstOperand);
        VISA_PredOpnd *Pred  = GetFlagOperand(m_encoderState.m_flag);

        unsigned short NumElems = (ExecSize == 1) ? 1 : 8;
        CVariable *Carry = m_program->GetNewVariable(NumElems, Lo->GetType(), Lo->GetAlign(), Lo->IsUniform());
        VISA_VectorOpnd *AccOut = GetDestinationOperand(Carry, m_encoderState.m_dstOperand);

        SModifier MidMod = m_encoderState.m_dstOperand;
        if (Lo->IsUniform() && NumElems != 1) {
            MidMod.region[0] = 1;
            MidMod.region[1] = 1;
            MidMod.region[2] = 0;
            MidMod.specialRegion = true;
        }
        VISA_VectorOpnd *HIn = GetSourceOperand(Hi, MidMod);
        // Negative `Acc0`
        SModifier AccMod = MidMod;
        assert(AccMod.mod == EMOD_NONE);
        AccMod.mod = EMOD_NEG;
        VISA_VectorOpnd *AccIn = GetSourceOperand(Carry, AccMod);

        Common_VISA_EMask_Ctrl ExecMask = GetAluEMask(Lo);
        V(vKernel->AppendVISAArithmeticInst(
            ISA_SUBB, Pred, ExecMask, ExecSize,
            L, AccOut, S0L, S1L));
        V(vKernel->AppendVISAArithmeticInst(
            ISA_ADD, Pred, false, ExecMask, ExecSize,
            H, S0H, S1H));
        H = GetDestinationOperand(Hi, m_encoderState.m_dstOperand);
        V(vKernel->AppendVISAArithmeticInst(
            ISA_ADD, Pred, false, ExecMask, ExecSize,
            H, AccIn, HIn));
    }
}

void CEncoder::CarryBorrowArith(ISA_Opcode opcode, CVariable* dst, CVariable* src0, CVariable*src1)
{
    VISA_VectorOpnd* srcOpnd0 = GetSourceOperand(src0, m_encoderState.m_srcOperand[0]);
    VISA_VectorOpnd* srcOpnd1 = GetSourceOperand(src1, m_encoderState.m_srcOperand[1]);
    VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dst, m_encoderState.m_dstOperand);
    VISA_PredOpnd* predOpnd  = GetFlagOperand(m_encoderState.m_flag);
    SModifier carryOperand = m_encoderState.m_dstOperand;
    Common_ISA_Exec_Size execSize = GetAluExecSize(dst);

    switch (execSize) {
    default:
        assert(false && "Unknown execution size on carry-borrow-arith!");
        break;
    case EXEC_SIZE_1:
        carryOperand.subReg += 1;
        break;
    case EXEC_SIZE_8:
        carryOperand.subVar += 1;
        break;
    case EXEC_SIZE_16:
        carryOperand.subVar += 2;
        break;
    }
    VISA_VectorOpnd* carryBorrowOpnd = GetDestinationOperand(dst, carryOperand);
    assert(m_encoderState.m_dstOperand.mod == EMOD_NONE && "addc/subb doesn't support saturate");

    V(vKernel->AppendVISAArithmeticInst(
        opcode,
        predOpnd,
        GetAluEMask(dst),
        GetAluExecSize(dst),
        dstOpnd,
        carryBorrowOpnd,
        srcOpnd0,
        srcOpnd1));
}

void CEncoder::URBWrite(
    CVariable* src,
    const int payloadElementOffset,
    CVariable* offset,
    CVariable* urbHandle,
    CVariable* mask )
{
    Common_VISA_EMask_Ctrl emask = ConvertMaskToVisaType(m_encoderState.m_mask, m_encoderState.m_noMask);
    Common_ISA_Exec_Size execSize = visaExecSize(m_encoderState.m_simdSize);
    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
    VISA_RawOpnd* urbhandle = GetRawSource(urbHandle);
    // Two possible cases: offset may be constant (immediate) or runtime value.
    unsigned short immOffset = 0;
    VISA_RawOpnd* perSlotOffset = nullptr;
    if (offset->IsImmediate())
    {
        immOffset = int_cast<unsigned short>(offset->GetImmediateValue());
        V(vKernel->CreateVISANullRawOperand(perSlotOffset, false));
    }
    else
    {
        // per-slot offset cannot be a uniform variable even if the value is since
        // we need the data in each lane
        assert(!offset->IsUniform() && "Per slot offset cannot be a uniform variable");
        perSlotOffset = GetRawSource(offset);
    }

    // Three possible cases:
    // 1. Channel Mask is immediate value with 0xFF, so not needed to send
    // 2. Channel Mask is immediate value other than 0xFF, so needed to send, but as immediate value
    // 3. Channel Mask is not immediate value, so needed to send, but as not immediate value
    VISA_RawOpnd *channelMask = nullptr;
    unsigned char payloadSize = 0;
    if (!mask->IsImmediate())
    {
        channelMask = GetRawSource(mask);
        // All 4 elements will be send - we don't know which are masked out.
        payloadSize = 4;
    }
    else
    {
        unsigned int immChannelMask = int_cast<unsigned int>(mask->GetImmediateValue());
        URBChannelMask immMask(immChannelMask);
        if (immMask.isAllSet())
        {
            V(vKernel->CreateVISANullRawOperand(channelMask, false));
        }
        else
        {
            CVariable* tmpDst = m_program->GetNewVariable(8, ISA_TYPE_UD, EALIGN_GRF, true);
            VISA_VectorOpnd* movDst = nullptr;
            V(vKernel->CreateVISADstOperand(movDst, GetVISAVariable(tmpDst), 1, 0, 0));

            VISA_VectorOpnd* immSrc = nullptr;
            V(vKernel->CreateVISAImmediate(immSrc, &immChannelMask, ISA_TYPE_UW));

            V(vKernel->AppendVISADataMovementInst(
                ISA_MOV, nullptr, false, vISA_EMASK_M1,
                EXEC_SIZE_8, movDst, immSrc));
            V(vKernel->CreateVISARawOperand(channelMask, GetVISAVariable(tmpDst), 0));
        }

        payloadSize = int_cast<unsigned char>(immMask.size());
    }

    VISA_RawOpnd *vertexData = GetRawSource(src, payloadElementOffset);

    V(vKernel->AppendVISA3dURBWrite(
        predOpnd,
        emask,
        execSize,
        payloadSize,
        channelMask,
        immOffset,
        urbhandle,
        perSlotOffset,
        vertexData));
}

VISA_RawOpnd* CEncoder::GetRawSource(CVariable* var, uint offset)
{
    VISA_RawOpnd *srcOpnd = nullptr;
    if(var)
    {
        if(var->IsImmediate())
        {
            VISA_VectorOpnd* vecOpnd = nullptr;
            uint immediate = int_cast<uint>(var->GetImmediateValue());
            V(vKernel->CreateVISAImmediate(vecOpnd, &immediate, ISA_TYPE_UD));
            srcOpnd = (VISA_RawOpnd*)vecOpnd;
        }
        else
        {
            V(vKernel->CreateVISARawOperand(
                srcOpnd,
                GetVISAVariable(var),
                int_cast<unsigned short>(offset + var->GetAliasOffset())));
        }
    }
    else
    {
        V(vKernel->CreateVISANullRawOperand(srcOpnd, false));
    }
    return srcOpnd;
}

VISA_RawOpnd* CEncoder::GetRawDestination(CVariable* var, unsigned offset)
{
    VISA_RawOpnd *dstOpnd = nullptr;
    if(var)
    {
        V(vKernel->CreateVISARawOperand(
            dstOpnd, GetVISAVariable(var),
            m_encoderState.m_dstOperand.subVar*SIZE_GRF + offset + var->GetAliasOffset()));
    }
    else
    {
         V(vKernel->CreateVISANullRawOperand(dstOpnd, true));
    }
    return dstOpnd;
}

void CEncoder::Send(CVariable* dst, CVariable* src, uint exDesc, CVariable* messDescriptor, bool isSendc)
{
    if(dst && dst->IsUniform())
    {
        m_encoderState.m_simdSize = m_encoderState.m_uniformSIMDSize;
    }
    unsigned char sendc = isSendc ? 1 : 0;
    unsigned char srcSize = src->GetSize()/SIZE_GRF;
    unsigned char dstSize = dst ? dst->GetSize()/SIZE_GRF : 0;
    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
    VISA_RawOpnd *srcOpnd0    = GetRawSource(src);
    VISA_RawOpnd *dstOpnd     = GetRawDestination(dst);
    VISA_VectorOpnd *desc = GetUniformSource(messDescriptor);

    V(vKernel->AppendVISAMiscRawSend(
        predOpnd,
        GetAluEMask(dst),
        visaExecSize(m_encoderState.m_simdSize),
        sendc,
        exDesc,
        srcSize,
        dstSize,
        desc,
        srcOpnd0,
        dstOpnd));
}

void CEncoder::Send(CVariable* dst, CVariable* src, uint ffid, CVariable* exDesc, CVariable* messDescriptor, bool isSendc)
{
    Sends(dst, src, nullptr, ffid, exDesc, messDescriptor, isSendc);
}

void CEncoder::Sends(CVariable* dst, CVariable* src0, CVariable* src1, uint ffid, CVariable* exDesc, CVariable* messDescriptor,  bool isSendc)
{
    if(exDesc->IsImmediate() && src1 == nullptr)
    {
        Send(dst, src0, (uint)exDesc->GetImmediateValue(), messDescriptor, isSendc);
        return;
    }
    if(dst && dst->IsUniform())
    {
        m_encoderState.m_simdSize = m_encoderState.m_uniformSIMDSize;
    }
    unsigned char sendc = isSendc ? 1 : 0;
    unsigned char src0Size = src0->GetSize()/SIZE_GRF;
    unsigned char src1Size = src1 ? src1->GetSize() / SIZE_GRF : 0;
    unsigned char dstSize = dst ? dst->GetSize()/SIZE_GRF : 0;
    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
    VISA_RawOpnd *srcOpnd0 = GetRawSource(src0);
    VISA_RawOpnd *srcOpnd1 = GetRawSource(src1);
    VISA_RawOpnd *dstOpnd     = GetRawDestination(dst);
    VISA_VectorOpnd *exMessDesc = GetUniformSource(exDesc);
    VISA_VectorOpnd *desc = GetUniformSource(messDescriptor);

    V(vKernel->AppendVISAMiscRawSends(
        predOpnd,
        GetAluEMask(dst),
        visaExecSize(m_encoderState.m_simdSize),
        sendc,
        ffid,
        exMessDesc,
        src0Size,
        src1Size, // right now only one source
        dstSize,
        desc,
        srcOpnd0,
        srcOpnd1,
        dstOpnd));
}

VISA_StateOpndHandle* CEncoder::GetBTIOperand(uint bindingTableIndex)
{
    IGC::e_predefSurface predDefSurface = ESURFACE_NORMAL;
    if(bindingTableIndex == 255)
        predDefSurface = ESURFACE_STATELESS;
    else if(bindingTableIndex == 254)
        predDefSurface = ESURFACE_SLM;
    CVariable tempImm(bindingTableIndex, ISA_TYPE_UD);
    return GetVISASurfaceOpnd(predDefSurface, &tempImm );
}

void CEncoder::RenderTargetWrite(CVariable* var[],
                                 bool isUndefined[],
                                 bool lastRenderTarget,
                                 bool perSample,
                                 bool coarseMode,
                                 bool headerMaskFromCe0,
                                 CVariable* bindingTableIndex,
                                 CVariable* RTIndex,
                                 CVariable* source0Alpha,
                                 CVariable* oMask,
                                 CVariable* depth,
                                 CVariable *stencil,
                                 CVariable *CPSCounter,
                                 CVariable *sampleIndex,
                                 CVariable *r1Reg)
{
    Common_VISA_EMask_Ctrl emask = ConvertMaskToVisaType(m_encoderState.m_mask, m_encoderState.m_noMask);
    Common_ISA_Exec_Size execSize = visaExecSize(m_encoderState.m_simdSize);
    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
    VISA_StateOpndHandle* surfOpnd = GetVISASurfaceOpnd(ESURFACE_NORMAL, bindingTableIndex);

    vISA_RT_CONTROLS cntrls;
    uint8_t numMsgSpecificOpnds = 0;
    VISA_RawOpnd* srcOpnd[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

    cntrls.isPerSample = perSample;
    cntrls.isCoarseMode = coarseMode;
    cntrls.isHeaderMaskfromCe0 = headerMaskFromCe0;
    assert(!(predOpnd != nullptr && cntrls.isHeaderMaskfromCe0));

    if(source0Alpha)
    {
        cntrls.s0aPresent = true;
        srcOpnd[numMsgSpecificOpnds++] = GetRawSource(source0Alpha);
    }else
        cntrls.s0aPresent = false;

    if(oMask)
    {
        cntrls.oMPresent = true;
        srcOpnd[numMsgSpecificOpnds++] = GetRawSource(oMask);
    }else
        cntrls.oMPresent = false;

    for(int i = 0; i < 4; i++)
    {
        if(isUndefined[i])
        {
            V(vKernel->CreateVISANullRawOperand(srcOpnd[numMsgSpecificOpnds++], false));
        }else
        {
            srcOpnd[numMsgSpecificOpnds++] = GetRawSource(var[i]);
        }
    }

    if(depth)
    {
        cntrls.zPresent = true;
        srcOpnd[numMsgSpecificOpnds++] = GetRawSource(depth);
    }else
        cntrls.zPresent = false;

    if(stencil)
    {
        cntrls.isStencil = true;
        srcOpnd[numMsgSpecificOpnds++] = GetRawSource(stencil);
    }else
        cntrls.isStencil = false;

    cntrls.isSampleIndex = false;
    VISA_VectorOpnd *sampleIndexOpnd = NULL;
    if (sampleIndex)
    {
        sampleIndexOpnd = GetSourceOperandNoModifier(sampleIndex);
        cntrls.isSampleIndex = true;
    }
    VISA_VectorOpnd *cpsCounterOpnd = GetSourceOperandNoModifier(CPSCounter);

    VISA_VectorOpnd *RTIndexOpnd = nullptr;
    cntrls.RTIndexPresent = false;
    // if RTIndex is 0, then no need to prepare the header for send
    if (!RTIndex->IsImmediate() || RTIndex->GetImmediateValue() != 0)
    {
        RTIndexOpnd = GetSourceOperandNoModifier(RTIndex);
        cntrls.RTIndexPresent = true;
    }

    //controls last render target select bit
    cntrls.isLastWrite = lastRenderTarget;

    //r1Reg should always be populated
    //vISA will decide whether to use it or not.
    VISA_RawOpnd *r1RegOpnd = GetRawSource(r1Reg);


    if(CPSCounter)
    {
        V(vKernel->AppendVISA3dRTWriteCPS(
            predOpnd,
            emask,
            execSize,
            RTIndexOpnd,
            cntrls,
            surfOpnd,
            r1RegOpnd,
            sampleIndexOpnd,
            cpsCounterOpnd,
            numMsgSpecificOpnds,
            srcOpnd));
    }
    else
    {
        V(vKernel->AppendVISA3dRTWrite(
            predOpnd,
            emask,
            execSize,
            RTIndexOpnd,
            cntrls,
            surfOpnd,
            r1RegOpnd,
            sampleIndexOpnd,
            numMsgSpecificOpnds,
            srcOpnd));
    }
}

VISA_StateOpndHandle* CEncoder::GetSamplerOperand(
    const SamplerDescriptor& sampler,
    bool& isIdxLT16)
{
    //Sampler index
    VISA_VectorOpnd* dstOpnd = nullptr;
    VISA_SamplerVar* samplerVar = nullptr;

    if (sampler.m_samplerType == ESAMPLER_NORMAL)
    {
        samplerVar = samplervar;

        if (sampler.m_sampler->IsImmediate())
        {
            uint immediate = int_cast<uint>(sampler.m_sampler->GetImmediateValue());
            if (immediate < 16)
            {
                isIdxLT16 = true;
            }
            else
            {
                isIdxLT16 = false;
            }
        }
        else
        {
            // for dynamic index, avoid generate additional code for APIs only supporting 16 samplers
            if(m_program->GetContext()->m_DriverInfo.SupportMoreThan16Samplers())
            {
                isIdxLT16 = false;
            }
            else
            {
                isIdxLT16 = true;
            }
        }
    }
    else
    {
        V(vKernel->GetBindlessSampler(samplerVar));
        isIdxLT16 = true;
    }

    V(vKernel->CreateVISAStateOperand(dstOpnd, samplerVar, 0, true));

    VISA_VectorOpnd* sourecOpnd = nullptr;
    assert(sampler.m_sampler->IsUniform());
    sourecOpnd = GetUniformSource(sampler.m_sampler);

    //Add the mov special instruction for sampler
    V(vKernel->AppendVISADataMovementInst(
        ISA_MOVS,
        nullptr,
        false,
        vISA_EMASK_M1_NM,
        EXEC_SIZE_1,
        dstOpnd,
        sourecOpnd,
        nullptr));

    VISA_StateOpndHandle* samplerOpnd = nullptr;
    V(vKernel->CreateVISAStateOperandHandle(samplerOpnd, samplerVar));
    return samplerOpnd;
}

VISA_StateOpndHandle* CEncoder::GetSamplerOperand(CVariable* samplerIndex)
{
    SamplerDescriptor sampler;
    bool isIdxLT16;
    sampler.m_sampler = samplerIndex;
    return GetSamplerOperand(sampler, isIdxLT16);
}

void CEncoder::Sample(
    EOPCODE subOpcode,
    uint writeMask,
    CVariable* offset,
    const ResourceDescriptor& resource,
    const SamplerDescriptor& sampler,
    uint numSources,
    CVariable* dst,
    SmallVector<CVariable*, 4>& payload,
    bool zeroLOD,
    bool cpsEnable,
    bool feedbackEnable,
    bool nonUniformState)
{
    int numMsgSpecificOpnds = numSources;
    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
    bool isIdxLT16;
    VISA_StateOpndHandle* samplerOpnd = GetSamplerOperand(sampler, isIdxLT16);
    VISA_StateOpndHandle* btiOpnd = GetVISASurfaceOpnd(resource);
    VISA_RawOpnd* dstVar = GetRawDestination(dst);
    VISA_RawOpnd * opndArray[11];
    for(int i = 0; i< numMsgSpecificOpnds; i++)
    {
        opndArray[i] = GetRawSource(payload[i]);
    }

    VISA_VectorOpnd* aoffimmi = GetSourceOperandNoModifier(offset);
    // Use bit 15 of aoffimmi to tell VISA the sample index could be greater
    // than 15.  In this case, we need to use msg header, and setup M0.3
    // to point to next 16 sampler state.
    if(!isIdxLT16)
    {
        uint16_t aoffimmiVal = (uint16_t)offset->GetImmediateValue() | BIT(15);
        V(vKernel->CreateVISAImmediate(aoffimmi, &aoffimmiVal, ISA_TYPE_UW));
    }

    V(vKernel->AppendVISA3dSampler(
        ConvertSubOpcode(subOpcode, zeroLOD),
        feedbackEnable, // pixel null mask
        cpsEnable,
        !nonUniformState,
        predOpnd,
        GetAluEMask(dst),
        visaExecSize(m_encoderState.m_simdSize),
        ConvertChannelMaskToVisaType(writeMask),
        aoffimmi,
        samplerOpnd,
        btiOpnd,
        dstVar,
        numSources,
        opndArray));
}

void CEncoder::Load(
    EOPCODE subOpcode,
    uint writeMask,
    CVariable* offset,
    const ResourceDescriptor& resource,
    uint numSources,
    CVariable* dst,
    SmallVector<CVariable*, 4>& payload,
    bool zeroLOD,
    bool feedbackEnable)
{
    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
    VISA_StateOpndHandle* surfOpnd = GetVISASurfaceOpnd(resource);
    VISA_RawOpnd* dstVar = GetRawDestination(dst);

    VISA_RawOpnd * opndArray[11];
    for(unsigned int i = 0; i <numSources; i++)
    {
        opndArray[i] = GetRawSource(payload[i]);
    }

    VISA_VectorOpnd* aoffimmi = GetSourceOperandNoModifier(offset);

    V(vKernel->AppendVISA3dLoad(
        ConvertSubOpcode(subOpcode, zeroLOD),
        feedbackEnable, // pixel null mask
        predOpnd,
        GetAluEMask(dst),
        GetAluExecSize(dst),
        ConvertChannelMaskToVisaType(writeMask),
        aoffimmi,
        surfOpnd,
        dstVar,
        numSources,
        opndArray));
}

void CEncoder::Info(EOPCODE subOpcode, uint writeMask, const ResourceDescriptor& resource, CVariable* lod, CVariable* dst)
{
    VISA_StateOpndHandle* surfOpnd = GetVISASurfaceOpnd(resource);
    VISA_RawOpnd* dstVar = GetRawDestination(dst);
    VISA_RawOpnd* lodVar = GetRawSource(lod);

    V(vKernel->AppendVISA3dInfo(
        ConvertSubOpcode(subOpcode, false),
        GetAluEMask(dst),
        GetAluExecSize(dst),
        ConvertChannelMaskToVisaType(writeMask),
        surfOpnd,
        lodVar,
        dstVar));
}

void CEncoder::Gather4Inst(
    EOPCODE subOpcode,
    CVariable* offset,
    const ResourceDescriptor& resource,
    const SamplerDescriptor& sampler,
    uint numSources,
    CVariable* dst,
    SmallVector<CVariable*, 4>& payload,
    uint channel,
    bool feedbackEnable)
{
    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
    bool isIdxLT16;
    VISA_StateOpndHandle* samplerOpnd = GetSamplerOperand(sampler, isIdxLT16);
    VISA_StateOpndHandle* surfOpnd = GetVISASurfaceOpnd(resource);
    VISA_RawOpnd* dstVar = GetRawDestination(dst);
    VISA_RawOpnd * opndArray[11];
    for(unsigned int i = 0; i < numSources; i++)
    {
        opndArray[i] = GetRawSource(payload[i]);
    }

    VISA_VectorOpnd* aoffimmi = GetSourceOperandNoModifier(offset);
    if (!isIdxLT16)
    {
        uint16_t aoffimmiVal = (uint16_t)offset->GetImmediateValue() | BIT(15);
        V(vKernel->CreateVISAImmediate(aoffimmi, &aoffimmiVal, ISA_TYPE_UW));
    }

    V(vKernel->AppendVISA3dGather4(
        ConvertSubOpcode(subOpcode, false),
        feedbackEnable, // pixel null mask
        predOpnd,
        GetAluEMask(dst),
        visaExecSize(m_encoderState.m_simdSize),
        ConvertSingleSourceChannel(channel),
        aoffimmi,
        samplerOpnd,
        surfOpnd,
        dstVar,
        numSources,
        opndArray));
}

void CEncoder::AddrAdd(CVariable* dst, CVariable* src0, CVariable* src1)
{
    if(dst->IsUniform())
    {
        m_encoderState.m_simdSize = SIMDMode::SIMD1;
        m_encoderState.m_noMask = true;
    }
    VISA_VectorOpnd* pSrc1Opnd = GetSourceOperand(src1, m_encoderState.m_srcOperand[1]);
    VISA_VectorOpnd* pSrc0Addr = nullptr;
    V(vKernel->CreateVISAAddressOfOperand(pSrc0Addr, GetVISAVariable(src0), src0->GetAliasOffset()));
    VISA_VectorOpnd* pVectorOpnd = nullptr;
    V(vKernel->CreateVISAAddressDstOperand(pVectorOpnd, dst->visaAddrVariable, 0));

    V(vKernel->AppendVISAAddrAddInst(
        GetAluEMask(dst),
        visaExecSize(m_encoderState.m_simdSize),
        pVectorOpnd,
        pSrc0Addr,
        pSrc1Opnd));
}

void CEncoder::Barrier(e_barrierKind BarrierKind)
{
    if (BarrierKind == EBARRIER_SIGNAL) {
        // signal only
        V(vKernel->AppendVISASplitBarrierInst(true));
        return;
    }
    if (BarrierKind == EBARRIER_WAIT) {
        // wait only
        V(vKernel->AppendVISASplitBarrierInst(false));
        return;
    }
    V(vKernel->AppendVISASyncInst(ISA_BARRIER));
}

void CEncoder::Fence(bool CommitEnable,
    bool L3_Flush_RW_Data,
    bool L3_Flush_Constant_Data,
    bool L3_Flush_Texture_Data,
    bool L3_Flush_Instructions,
    bool Global_Mem_Fence,
    bool L1_Flush_Constant_Data,
    bool SWFence) // if true no ISA is emitted and the instruction is a pure code barrier
{
    // Only a single bit set here is a valid configuration
    assert( L3_Flush_Instructions +
        L3_Flush_Texture_Data +
        L3_Flush_Constant_Data +
        L3_Flush_RW_Data <= 1 );

    uint fenceFlags =  ( L3_Flush_Instructions << 1 ) |
        ( L3_Flush_Texture_Data << 2 ) |
        ( L3_Flush_Constant_Data << 3 ) |
        ( L3_Flush_RW_Data << 4 ) |
        ( (!Global_Mem_Fence) << 5 ) | // bit 5: 1 -- local, 0 -- global
        ( L1_Flush_Constant_Data << 6 ) |
        ( SWFence << 7 ) |
        ( CommitEnable << 0 );

    V(vKernel->AppendVISASyncInst(ISA_FENCE, int_cast<unsigned char>(fenceFlags)));
}

void CEncoder::FlushSamplerCache()
{
    V(vKernel->AppendVISASyncInst(ISA_SAMPLR_CACHE_FLUSH));
}

void CEncoder::EndOfThread()
{
    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
    V(vKernel->AppendVISACFRetInst(predOpnd, vISA_EMASK_M1, EXEC_SIZE_1));
}

void CEncoder::SetFloatDenormMode(VISAKernel* vKernel, Float_DenormMode mode16,
                                  Float_DenormMode mode32, Float_DenormMode mode64)
{
    VISA_VectorOpnd* src0_Opnd = nullptr;
    VISA_VectorOpnd* src1_Opnd = nullptr;
    VISA_VectorOpnd* dst_Opnd = nullptr;
    VISA_GenVar* cr0_var = nullptr;
    uint imm_data = 0;
    if (mode16 == FLOAT_DENORM_RETAIN)
        imm_data |= 0x400;
    if (mode32 == FLOAT_DENORM_RETAIN)
        imm_data |= 0x80;
    if (mode64 == FLOAT_DENORM_RETAIN)
        imm_data |= 0x40;
    // If we are in the default mode no need to set the CR
    if(imm_data != 0)
    {
        V(vKernel->GetPredefinedVar(cr0_var, PREDEFINED_CR0));
        V(vKernel->CreateVISASrcOperand(src0_Opnd, cr0_var, MODIFIER_NONE, 0, 1, 0, 0, 0));
        V(vKernel->CreateVISAImmediate(src1_Opnd, &imm_data, ISA_TYPE_UD));
        V(vKernel->CreateVISADstOperand(dst_Opnd, cr0_var, 1, 0, 0));
        V(vKernel->AppendVISAArithmeticInst(
            ISA_OR,
            nullptr,
            false,
            vISA_EMASK_M1_NM,
            EXEC_SIZE_1,
            dst_Opnd,
            src0_Opnd,
            src1_Opnd));
    }
}

void CEncoder::SetVectorMask(bool VMask)
{
    VISA_VectorOpnd* src0_Opnd = nullptr;
    VISA_VectorOpnd* src1_Opnd = nullptr;
    VISA_VectorOpnd* dst_Opnd = nullptr;
    VISA_GenVar* cr0_var;
    uint bitmaskImm = 1 << 3;
    if(!VMask)
    {
        bitmaskImm = ~bitmaskImm;
    }
    V(vKernel->GetPredefinedVar(cr0_var, PREDEFINED_CR0));
    V(vKernel->CreateVISASrcOperand(src0_Opnd, cr0_var, MODIFIER_NONE, 0, 1, 0, 0, 0));
    V(vKernel->CreateVISAImmediate(src1_Opnd, &bitmaskImm, ISA_TYPE_UD));
    V(vKernel->CreateVISADstOperand(dst_Opnd, cr0_var, 1, 0, 0));
    V(vKernel->AppendVISAArithmeticInst(
        VMask ? ISA_OR : ISA_AND,
        nullptr,
        false,
        vISA_EMASK_M1_NM,
        EXEC_SIZE_1,
        dst_Opnd,
        src0_Opnd,
        src1_Opnd));
}

void CEncoder::SetFloatRoundingMode(RoundingMode actualMode, RoundingMode newMode)
{
    if (actualMode != newMode)
    {
        VISA_VectorOpnd* src0_Opnd = nullptr;
        VISA_VectorOpnd* src1_Opnd = nullptr;
        VISA_VectorOpnd* dst_Opnd = nullptr;
        VISA_GenVar* cr0_var;
        uint roundingMode = actualMode ^ newMode;
        V(vKernel->GetPredefinedVar(cr0_var, PREDEFINED_CR0));
        V(vKernel->CreateVISASrcOperand(src0_Opnd, cr0_var, MODIFIER_NONE, 0, 1, 0, 0, 0));
        V(vKernel->CreateVISAImmediate(src1_Opnd, &roundingMode, ISA_TYPE_UD));
        V(vKernel->CreateVISADstOperand(dst_Opnd, cr0_var, 1, 0, 0));
        V(vKernel->AppendVISAArithmeticInst(
            ISA_XOR,
            nullptr,
            false,
            vISA_EMASK_M1_NM,
            EXEC_SIZE_1,
            dst_Opnd,
            src0_Opnd,
            src1_Opnd));
    }
}

void CEncoder::SetFloatRoundingModeDefault(RoundingMode actualMode)
{
    const RoundingMode defaultRoundingMode = getEncoderRoundingMode(static_cast<Float_RoundingMode>(
        m_program->GetContext()->getModuleMetaData()->compOpt.FloatRoundingMode));

    SetFloatRoundingMode(actualMode, defaultRoundingMode);
}

CEncoder::RoundingMode CEncoder::getEncoderRoundingMode(Float_RoundingMode FP_RM)
{
    switch (FP_RM) {
    default:
        break;
    case FLOAT_ROUND_TO_POSITIVE:
        return RoundingMode::RoundToPositive;
    case FLOAT_ROUND_TO_NEGATIVE:
        return RoundingMode::RoundToNegative;
    case FLOAT_ROUND_TO_ZERO:
        return RoundingMode::RoundToZero;
    }
    return RoundToNearestEven;
}

VISA_LabelOpnd* CEncoder::GetLabel(uint label)
{
    VISA_LabelOpnd *visaLabel = labelMap[label];
    if(visaLabel == nullptr)
    {
        VISA_Label_Kind kind = LABEL_BLOCK;
        char labelname[128]="";
        sprintf_s(labelname, sizeof(labelname), "label%d", labelCounter++);
        V(vKernel->CreateVISALabelVar(visaLabel, labelname, kind));
        labelMap[label] = visaLabel;
    }
    return visaLabel;
}

VISAFunction* CEncoder::GetStackFunction(llvm::Function *F)
{
    auto Iter = stackFuncMap.find(F);
    if (Iter != stackFuncMap.end())
    {
        return Iter->second;
    }
    VISAFunction *visaFunc = nullptr;
    V(vbuilder->AddFunction(visaFunc, F->getName().data()));
    stackFuncMap[F] = visaFunc;
    return visaFunc;
}

VISA_LabelOpnd* CEncoder::GetFuncLabel(llvm::Function *F)
{
    auto Iter = funcLabelMap.find(F);
    if (Iter != funcLabelMap.end())
    {
        return Iter->second;
    }

    // Create a new function label.
    VISA_LabelOpnd *visaLabel = nullptr;
    V(vKernel->CreateVISALabelVar(visaLabel, F->getName().data(), LABEL_SUBROUTINE));
    funcLabelMap[F] = visaLabel;

    return visaLabel;
}

void CEncoder::Push()
{
    Init();
}

VISA_VectorOpnd* CEncoder::GetUniformSource(CVariable* var)
{
    VISA_VectorOpnd* srcOperand = nullptr;
    if(var == nullptr)
    {
        return nullptr;
    }
    if(var->IsImmediate())
    {
        // TODO: need support for 64 bits immediate
        uint immediate = int_cast<uint>(var->GetImmediateValue());
        V(vKernel->CreateVISAImmediate(srcOperand, &immediate, ISA_TYPE_UD));
    }
    else
    {
        unsigned char rowOffset = 0;
        unsigned char colOffset = 0;
        GetRowAndColOffset(var, 0, 0, rowOffset, colOffset);
        V(vKernel->CreateVISASrcOperand(srcOperand, GetVISAVariable(var), MODIFIER_NONE, 0, 1, 0, rowOffset, colOffset));
    }
    return srcOperand;
}

TARGET_PLATFORM GetVISAPlatform(const CPlatform* platform)
{
    switch(platform->GetPlatformFamily())
    {
    case IGFX_GEN8_CORE:
        if (platform->getPlatformInfo().eProductFamily == IGFX_CHERRYVIEW )
        {
            return GENX_CHV;
        }
        else
        {
        return GENX_BDW;
        }
    case IGFX_GEN9_CORE:
    case IGFX_GENNEXT_CORE:
        if (platform->getPlatformInfo().eProductFamily == IGFX_BROXTON ||
            platform->getPlatformInfo().eProductFamily == IGFX_GEMINILAKE)
        {
            return GENX_BXT;
        }
        else
        {
            return GENX_SKL;
        }
    case IGFX_GEN10_CORE:
        return GENX_CNL;
    case IGFX_GEN11_CORE:
        if (platform->getPlatformInfo().eProductFamily == IGFX_ICELAKE_LP ||
            platform->getPlatformInfo().eProductFamily == IGFX_LAKEFIELD
           )
        {
            return GENX_ICLLP;
        }
        else
        {
            return GENX_ICL;
        }
    default:
        assert(0 && "unsupported platform");
        break;
    }
    return GENX_SKL;
}

void CEncoder::OWLoad( CVariable* dst, const ResourceDescriptor& resource, CVariable* src0, bool owordAligned, uint bytesToBeRead, uint dstOffset )
{
    VISA_StateOpndHandle* surfOpnd = GetVISASurfaceOpnd(resource);
    VISA_VectorOpnd* offset = GetUniformSource( src0 );
    VISA_RawOpnd* dstVar = GetRawDestination( dst, dstOffset );
    uint size = ( bytesToBeRead / SIZE_OWORD );

    V( vKernel->AppendVISASurfAccessOwordLoadStoreInst(
        owordAligned ? ISA_OWORD_LD : ISA_OWORD_LD_UNALIGNED,
        vISA_EMASK_M1_NM,  // OWord load is always nomask
        surfOpnd,
        ConvertSizeToVisaType( size ),
        offset,
        dstVar ) );
}

void CEncoder::OWStore( CVariable* data, e_predefSurface surfaceType, CVariable* bufId, CVariable* src0, uint bytesToBeRead, uint srcOffset )
{
    VISA_StateOpndHandle* surfOpnd = GetVISASurfaceOpnd( surfaceType, bufId );
    VISA_VectorOpnd* offset  = GetUniformSource( src0 );
    VISA_RawOpnd*    dataVar = GetRawSource( data, srcOffset );
    uint size = ( bytesToBeRead / SIZE_OWORD );

    V( vKernel->AppendVISASurfAccessOwordLoadStoreInst(
        ISA_OWORD_ST,
        vISA_EMASK_M1_NM,
        surfOpnd,
        ConvertSizeToVisaType( size ),
        offset,
        dataVar ) );
}

void CEncoder::OWStoreA64( CVariable* data, CVariable* src0, uint bytesToBeRead, uint srcOffset )
{
    VISA_VectorOpnd* offset  = GetUniformSource( src0 );
    VISA_RawOpnd*    dataVar = GetRawDestination( data, srcOffset );
    uint size                = ( bytesToBeRead / SIZE_OWORD );

    V( vKernel->AppendVISASvmBlockStoreInst(
        ConvertSizeToVisaType( size ),
        true,   // always unaligned for now
        offset,
        dataVar ) );
}

void CEncoder::OWLoadA64( CVariable* dst, CVariable* src0, uint bytesToBeRead, uint dstOffset )
{
    VISA_VectorOpnd* offset = GetUniformSource( src0 );
    VISA_RawOpnd* dstVar    = GetRawDestination( dst, dstOffset );
    uint size               = ( bytesToBeRead / SIZE_OWORD );

    V( vKernel->AppendVISASvmBlockLoadInst(
        ConvertSizeToVisaType( size ),
        true,   // always unaligned for now
        offset,
        dstVar ) );
}

void CEncoder::MediaBlockMessage(
    ISA_Opcode subOpcode,
    CVariable* dst,
    e_predefSurface surfaceType,
    CVariable* bufId,
    CVariable* xOffset,
    CVariable* yOffset,
    uint modifier,
    unsigned char blockWidth,
    unsigned char blockHeight,
    uint plane )
{
    VISA_StateOpndHandle* surfOpnd = GetVISASurfaceOpnd( surfaceType, bufId );
    VISA_VectorOpnd* xVar  = GetUniformSource( xOffset );
    VISA_VectorOpnd* yVar  = GetUniformSource( yOffset );
    VISA_RawOpnd* tempVar  = nullptr;
    if ( subOpcode == ISA_MEDIA_LD )
    {
        tempVar = GetRawDestination( dst );
    }
    else if ( subOpcode == ISA_MEDIA_ST )
    {
        tempVar = GetRawSource( dst );
    }

    MEDIA_LD_mod  modi     = ( MEDIA_LD_mod )modifier;
    CISA_PLANE_ID planeVar = ( CISA_PLANE_ID )plane;

    V( vKernel->AppendVISASurfAccessMediaLoadStoreInst(
        subOpcode,
        modi,
        surfOpnd,
        blockWidth,
        blockHeight,
        xVar,
        yVar,
        tempVar,
        planeVar ) );
}

void CEncoder::TypedReadWrite(
    ISA_Opcode opcode,
    const ResourceDescriptor& resource,
    CVariable* pU,
    CVariable* pV,
    CVariable* pR,
    CVariable* pLOD,
    CVariable* pSrcDst,
    uint writeMask)
{
    // only SIMD 8 reads & writes are supported.
    VISAChannelMask channelMask = CHANNEL_MASK_RGBA;//for typed write leaving this as before
    if (writeMask != 0)
    {
        channelMask = ConvertChannelMaskToVisaType(writeMask);
    }
    VISA_StateOpndHandle* pSurfStateOpndHandle = GetVISASurfaceOpnd(resource);

    // TODO unify the way we calculate offset for raw sources, maybe we shouldn't use offset at all
    VISA_RawOpnd* pUOffset = GetRawSource(pU, m_encoderState.m_srcOperand[0].subVar*SIZE_GRF);
    VISA_RawOpnd* pVOffset = GetRawSource(pV, m_encoderState.m_srcOperand[1].subVar*SIZE_GRF);
    VISA_RawOpnd* pROffset = GetRawSource(pR, m_encoderState.m_srcOperand[2].subVar*SIZE_GRF);
    VISA_RawOpnd* pLODOffset = GetRawSource(pLOD, m_encoderState.m_srcOperand[3].subVar*SIZE_GRF);
    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
    assert(m_encoderState.m_dstOperand.subVar == 0);

    VISA_RawOpnd* pDstVar = nullptr;
    Common_VISA_EMask_Ctrl mask;
    if (opcode == ISA_SCATTER4_TYPED)
    {
        pDstVar = GetRawSource(pSrcDst, 0);
        mask = ConvertMaskToVisaType(m_encoderState.m_mask, m_encoderState.m_noMask);
    }
    else
    {
        pDstVar = GetRawDestination(pSrcDst);
        mask = GetAluEMask(pSrcDst);
    }

    V(vKernel->AppendVISASurfAccessGather4Scatter4TypedInst(
        opcode,
        predOpnd,
        channelMask,
        mask,
        visaExecSize(m_encoderState.m_simdSize),
        pSurfStateOpndHandle,
        pUOffset,
        pVOffset,
        pROffset,
        pLODOffset,
        pDstVar));
}

void CEncoder::ScatterGather(ISA_Opcode opcode, CVariable* srcdst, CVariable* bufId, CVariable* offset, CVariable* gOffset, e_predefSurface surface, int elementSize)
{
    VISA_VectorOpnd* globalOffsetOpnd = nullptr;
    VISA_StateOpndHandle* surfOpnd = GetVISASurfaceOpnd(surface, bufId);
    if(gOffset)
    {
        globalOffsetOpnd = GetUniformSource(gOffset);
    }
    else
    {
        int value = 0;
        V(vKernel->CreateVISAImmediate(globalOffsetOpnd, &value ,ISA_TYPE_UD));
    }
    VISA_RawOpnd* elementOffset = GetRawSource(offset);

    VISA_RawOpnd* dstVar = NULL;

    Common_VISA_EMask_Ctrl mask;
    if(opcode == ISA_GATHER)
    {
        dstVar = GetRawDestination(srcdst);
        mask = GetAluEMask(srcdst);
    }
    else
    {
        dstVar = GetRawSource(srcdst);
        mask = ConvertMaskToVisaType(m_encoderState.m_mask, m_encoderState.m_noMask);
    }

    V(vKernel->AppendVISASurfAccessGatherScatterInst(
        opcode,
        mask,
        visaElementSize(elementSize),
        getExecSize(m_encoderState.m_simdSize),
        surfOpnd,
        globalOffsetOpnd,
        elementOffset,
        dstVar));
}

void CEncoder::ScatterGather4(ISA_Opcode opcode, CVariable* srcdst, CVariable* bufId, CVariable* offset, CVariable* gOffset, e_predefSurface surface)
{
    VISA_VectorOpnd* globalOffsetOpnd = nullptr;
    if(gOffset)
    {
        globalOffsetOpnd = GetUniformSource(gOffset);
    }
    else
    {
        int value = 0;
        V(vKernel->CreateVISAImmediate(globalOffsetOpnd, &value ,ISA_TYPE_UD));
    }
    VISA_RawOpnd* elementOffset = GetRawSource(offset);
    VISA_StateOpndHandle* surfOpnd = GetVISASurfaceOpnd(surface, bufId);

    VISA_RawOpnd* dstVar;
    Common_VISA_EMask_Ctrl visaMask;
    if(opcode == ISA_GATHER4)
    {
        dstVar = GetRawDestination(srcdst);
        visaMask = GetAluEMask(srcdst);
    }
    else
    {
        dstVar = GetRawSource(srcdst);
        visaMask = ConvertMaskToVisaType(m_encoderState.m_mask, m_encoderState.m_noMask);
    }
    uint nd = srcdst->GetSize();
    if (m_encoderState.m_simdSize == SIMDMode::SIMD8)
        nd = nd / SIZE_GRF;
    else if (m_encoderState.m_simdSize == SIMDMode::SIMD16)
        nd = nd / (SIZE_GRF * 2);
    else
        assert(0);

    uint mask = BIT(nd)-1;

    V(vKernel->AppendVISASurfAccessGather4Scatter4Inst(
        opcode,
        ConvertChannelMaskToVisaType(mask),
        visaMask,
        getExecSize(m_encoderState.m_simdSize),
        surfOpnd,
        globalOffsetOpnd,
        elementOffset,
        dstVar));
}

void CEncoder::GenericAlu(e_opcode opcode, CVariable* dst, CVariable* src0, CVariable* src1, CVariable* src2)
{
    ISA_Opcode visaOpcode = ConvertOpcode[opcode];
    switch(visaOpcode)
    {
    case ISA_MOV:
    case ISA_MOVS:
    case ISA_SETP:
        DataMov(visaOpcode, dst, src0);
        break;
    case ISA_FMINMAX:
        MinMax(opcode == EOPCODE_MIN ? CISA_DM_FMIN : CISA_DM_FMAX, dst, src0, src1);
        break;
    case ISA_AND:
    case ISA_ASR:
    case ISA_CBIT:
    case ISA_FBH:
    case ISA_FBL:
    case ISA_NOT:
    case ISA_OR:
    case ISA_SHL:
    case ISA_SHR:
    case ISA_ROL:
    case ISA_ROR:
    case ISA_XOR:
        LogicOp(visaOpcode, dst, src0, src1, src2);
        break;
    default:
        Arithmetic(visaOpcode, dst, src0, src1, src2);
        break;
    }
}

VISA_StateOpndHandle* CEncoder::GetVISASurfaceOpnd(const ResourceDescriptor& resource)
{
    return GetVISASurfaceOpnd(resource.m_surfaceType, resource.m_resource);
}

VISA_StateOpndHandle* CEncoder::GetVISASurfaceOpnd(e_predefSurface surfaceType, CVariable* bti)
{
    VISA_StateOpndHandle* surfOpnd = nullptr;
    if(surfaceType == ESURFACE_NORMAL || surfaceType == ESURFACE_BINDLESS)
    {
        VISA_SurfaceVar* surfacevar = nullptr;
        if(surfaceType == ESURFACE_BINDLESS)
        {
            V(vKernel->GetPredefinedSurface(surfacevar, PREDEFINED_SURFACE_T252));
        }
        else
        {
            surfacevar = dummySurface;
        }
        VISA_VectorOpnd* sourecOpnd = GetUniformSource(bti);
        VISA_VectorOpnd* dstOpnd = nullptr;
        V(vKernel->CreateVISAStateOperand(dstOpnd, surfacevar, 0, true));

        //Add the mov special instruction
        V(vKernel->AppendVISADataMovementInst(
            ISA_MOVS,
            nullptr,
            false,
            vISA_EMASK_M1_NM,
            EXEC_SIZE_1,
            dstOpnd,
            sourecOpnd,
            nullptr));

        V(vKernel->CreateVISAStateOperandHandle(surfOpnd, surfacevar));
    }
    else
    {
        VISA_SurfaceVar* surfacevar = NULL;
        switch (surfaceType)
        {
        case ESURFACE_SLM:
            V(vKernel->GetPredefinedSurface(surfacevar, PREDEFINED_SURFACE_SLM));
            break;
        case ESURFACE_STATELESS:
            V(vKernel->GetPredefinedSurface(surfacevar, PREDEFINED_SURFACE_T255));
            break;
        default:
            assert("Invalid surface" && 0);
            break;
        }
        V(vKernel->CreateVISAStateOperandHandle(surfOpnd, surfacevar));
    }
    return surfOpnd;
}

Common_VISA_EMask_Ctrl CEncoder::ConvertMaskToVisaType(e_mask mask, bool noMask)
{
    switch(mask)
    {
    case EMASK_Q1:
        if(m_encoderState.m_secondHalf)
        {
            return noMask ? vISA_EMASK_M5_NM : vISA_EMASK_M5;
        }
        else
        {
            return noMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1;
        }
    case EMASK_Q2:
        if(m_encoderState.m_secondHalf)
        {
            return noMask ? vISA_EMASK_M7_NM : vISA_EMASK_M7;
        }
        else
        {
            return noMask ? vISA_EMASK_M3_NM : vISA_EMASK_M3;
        }
    case EMASK_Q3:
        return noMask ? vISA_EMASK_M5_NM : vISA_EMASK_M5;
    case EMASK_Q4:
        return noMask ? vISA_EMASK_M7_NM : vISA_EMASK_M7;
    case EMASK_H1:
        return noMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1;
    case EMASK_H2:
        return noMask ? vISA_EMASK_M5_NM : vISA_EMASK_M5;
    default:
        assert( 0 && "unreachable" );
        return vISA_EMASK_M1_NM;
    }
}

VISA_Modifier ConvertModifierToVisaType(e_modifier modifier)
{
    switch(modifier)
    {
    case EMOD_NONE:
        return MODIFIER_NONE;
    case EMOD_SAT:
        return MODIFIER_SAT;
    case EMOD_ABS:
        return MODIFIER_ABS;
    case EMOD_NEG:
        return MODIFIER_NEG;
    case EMOD_NEGABS:
        return MODIFIER_NEG_ABS;
    case EMOD_NOT:
        return MODIFIER_NOT;
    default:
        assert( 0 && "unreachable" );
        return MODIFIER_NONE;
    }
}

Common_ISA_Cond_Mod ConvertCondModToVisaType(e_predicate condMod)
{
    switch(condMod)
    {
    case EPREDICATE_EQ:
        return ISA_CMP_E;
    case EPREDICATE_NE:
        return ISA_CMP_NE;
    case EPREDICATE_GT:
        return ISA_CMP_G;
    case EPREDICATE_GE:
        return ISA_CMP_GE;
    case EPREDICATE_LT:
        return ISA_CMP_L;
    case EPREDICATE_LE:
        return ISA_CMP_LE;
    default:
        assert( 0 && "unreachable" );
        return ISA_CMP_UNDEF;
    }
}

Common_ISA_Oword_Num  ConvertSizeToVisaType(uint size)
{
    switch(size)
    {
    case 1:
        return OWORD_NUM_1;
    case 2:
        return OWORD_NUM_2;
    case 4:
        return OWORD_NUM_4;
    case 8:
        return OWORD_NUM_8;
    case 16:
        return OWORD_NUM_16;
    default:
        assert( 0 && "unreachable" );
        return OWORD_NUM_ILLEGAL;
    }
}

VISAChannelMask ConvertChannelMaskToVisaType(uint mask)
{
    switch (mask & 0xf)
    {
    case 1:   return CHANNEL_MASK_R;
    case 2:   return CHANNEL_MASK_G;
    case 3:   return CHANNEL_MASK_RG;
    case 4:   return CHANNEL_MASK_B;
    case 5:   return CHANNEL_MASK_RB;
    case 6:   return CHANNEL_MASK_GB;
    case 7:   return CHANNEL_MASK_RGB;
    case 8:   return CHANNEL_MASK_A;
    case 9:   return CHANNEL_MASK_RA;
    case 0xa: return CHANNEL_MASK_GA;
    case 0xb: return CHANNEL_MASK_RGA;
    case 0xc: return CHANNEL_MASK_BA;
    case 0xd: return CHANNEL_MASK_RBA;
    case 0xe: return CHANNEL_MASK_GBA;
    case 0xf: return CHANNEL_MASK_RGBA;
    default:
        {
            assert(0 && "Wrong mask");
            return CHANNEL_MASK_NOMASK;
        }
    }
}

uint CEncoder::GetCISADataTypeSize(VISA_Type type)
{
    switch (type)
    {
    case ISA_TYPE_UD:
        return 4;
    case ISA_TYPE_D:
        return 4;
    case ISA_TYPE_UW:
        return 2;
    case ISA_TYPE_W:
        return 2;
    case ISA_TYPE_UB:
        return 1;
    case ISA_TYPE_B:
        return 1;
    case ISA_TYPE_DF:
        return 8;
    case ISA_TYPE_F:
        return 4;
    case ISA_TYPE_V:
        return 4;
    case ISA_TYPE_VF:
        return 4;
    case ISA_TYPE_BOOL:
        return 1;
    case ISA_TYPE_UV:
        return 4;
    case ISA_TYPE_Q:
        return 8;
    case ISA_TYPE_UQ:
        return 8;
    case ISA_TYPE_HF:
        return 2;
    default:
        assert(0 && "Unimplemented CISA Data Type");
        break;
    }

    return 0;
}

e_alignment CEncoder::GetCISADataTypeAlignment(VISA_Type type)
{
    switch (type)
    {
    case ISA_TYPE_UD:
        return EALIGN_DWORD;
    case ISA_TYPE_D:
        return EALIGN_DWORD;
    case ISA_TYPE_UW:
        return EALIGN_WORD;
    case ISA_TYPE_W:
        return EALIGN_WORD;
    case ISA_TYPE_UB:
        return EALIGN_BYTE;
    case ISA_TYPE_B:
        return EALIGN_BYTE;
    case ISA_TYPE_DF:
        return EALIGN_QWORD;
    case ISA_TYPE_F:
        return EALIGN_DWORD;
    case ISA_TYPE_V:
        return EALIGN_DWORD;
    case ISA_TYPE_VF:
        return EALIGN_DWORD;
    case ISA_TYPE_BOOL:
        return EALIGN_BYTE;
    case ISA_TYPE_UV:
        return EALIGN_BYTE;
    case ISA_TYPE_Q:
        return EALIGN_QWORD;
    case ISA_TYPE_UQ:
        return EALIGN_QWORD;
    case ISA_TYPE_HF:
        return EALIGN_WORD;
    default:
        assert(0 && "Unimplemented CISA Data Type");
        break;
    }

    return EALIGN_BYTE;
}

VISASampler3DSubOpCode CEncoder::ConvertSubOpcode(EOPCODE subOpcode, bool zeroLOD)
{
    switch(subOpcode)
    {
    case llvm_sampleptr:
        return VISA_3D_SAMPLE;
    case llvm_sample_bptr:
        return VISA_3D_SAMPLE_B;
    case llvm_sample_cptr:
        return VISA_3D_SAMPLE_C;
    case llvm_sample_dptr:
        return VISA_3D_SAMPLE_D;
    case llvm_sample_dcptr:
        return VISA_3D_SAMPLE_D_C;
    case llvm_sample_lptr:
        return zeroLOD? VISA_3D_SAMPLE_LZ : VISA_3D_SAMPLE_L;
    case llvm_sample_lcptr:
        return zeroLOD? VISA_3D_SAMPLE_C_LZ : VISA_3D_SAMPLE_L_C;
    case llvm_sample_bcptr:
        return VISA_3D_SAMPLE_B_C;
    case llvm_ld_ptr:
        return zeroLOD ? VISA_3D_LD_LZ : VISA_3D_LD;
    case llvm_resinfoptr:
        return VISA_3D_RESINFO;
    case llvm_gather4ptr:
        return VISA_3D_GATHER4;
    case llvm_gather4Cptr:
        return VISA_3D_GATHER4_C;
    case llvm_gather4POptr:
        return VISA_3D_GATHER4_PO;
    case llvm_gather4POCptr:
        return VISA_3D_GATHER4_PO_C;
    case llvm_sampleinfoptr:
        return VISA_3D_SAMPLEINFO;
    case llvm_ldmsptr:
    case llvm_ldmsptr16bit:
        return VISA_3D_LD2DMS_W;
    case llvm_ldmcsptr:
        return VISA_3D_LD_MSC;
    case llvm_lodptr:
        return VISA_3D_LOD;
    case llvm_sample_killpix:
        return VISA_3D_SAMPLE_KILLPIX;
    default:
        assert(0 && "wrong sampler subopcode");
        return VISA_3D_SAMPLE;
    }
}

bool CEncoder::IsIntegerType(VISA_Type type)
{
    return (type == ISA_TYPE_B  ||
            type == ISA_TYPE_UB ||
            type == ISA_TYPE_W  ||
            type == ISA_TYPE_UW ||
            type == ISA_TYPE_D  ||
            type == ISA_TYPE_UD ||
            type == ISA_TYPE_Q  ||
            type == ISA_TYPE_UQ ||
            0);
}

bool CEncoder::IsFloatType(VISA_Type type)
{
    return (type == ISA_TYPE_F ||
            type == ISA_TYPE_DF ||
            0);
}

VISASourceSingleChannel ConvertSingleSourceChannel(uint srcChannel)
{
    switch(srcChannel)
    {
    case 0:
        return VISA_3D_GATHER4_CHANNEL_R;
    case 1:
        return VISA_3D_GATHER4_CHANNEL_G;
    case 2:
        return VISA_3D_GATHER4_CHANNEL_B;
    case 3:
        return VISA_3D_GATHER4_CHANNEL_A;
    }
    assert(0 && "Wrong channel");
    return VISA_3D_GATHER4_CHANNEL_R;
}

void CEncoder::BeginSubroutine(llvm::Function *F)
{
    labelMap.clear();
    labelMap.resize(F->size(), nullptr);
    V(vKernel->AppendVISACFLabelInst(GetFuncLabel(F)));
}

void CEncoder::BeginStackFunction(llvm::Function *F)
{
    labelMap.clear();
    labelMap.resize(F->size(), nullptr);
    // At this place, the vISA object is changed!
    vKernel = GetStackFunction(F);
    VISA_LabelOpnd *visaLabel = nullptr;
    V(vKernel->CreateVISALabelVar(visaLabel, F->getName().data(), LABEL_SUBROUTINE));
    V(vKernel->AppendVISACFLabelInst(visaLabel));
}

void CEncoder::AddFunctionSymbol(llvm::Function* F, CVariable* fvar)
{
    SModifier mod;
    mod.init();
    VISA_VectorOpnd* visaFuncAddr = GetDestinationOperand(fvar, mod);
    V(vKernel->AppendVISACFSymbolInst(F->getName(), visaFuncAddr));
}

void CEncoder::InitEncoder( bool canAbortOnSpill, bool hasStackCall )
{
    m_aliasesMap.clear();
    m_encoderState.m_SubSpanDestination = false;
    CodeGenContext* context = m_program->GetContext();
    m_encoderState.m_secondHalf = false;
    m_enableVISAdump = false;
    labelMap.clear();
    labelMap.resize(m_program->entry->size(), nullptr);
    labelCounter = 0;

    vbuilder = nullptr;
    TARGET_PLATFORM VISAPlatform = GetVISAPlatform(&(context->platform));


    bool KernelDebugEnable = false;
    bool ForceNonCoherentStatelessBti = false;
    auto gtpin_init = context->gtpin_init;
    if (context->type == ShaderType::OPENCL_SHADER)
    {
        auto ClContext = static_cast<OpenCLProgramContext*>(context);
        KernelDebugEnable = ClContext->m_InternalOptions.KernelDebugEnable;
        ForceNonCoherentStatelessBti = ClContext->m_ShouldUseNonCoherentStatelessBTI;
    }

    bool EnableBarrierInstCounterBits = false;
    if (context->type == ShaderType::HULL_SHADER)
    {
        EnableBarrierInstCounterBits = true;
    }
    bool preserveR0 = false;
    if(context->type == ShaderType::PIXEL_SHADER)
    {
        preserveR0 = !static_cast<CPixelShader*>(m_program)->IsLastPhase();
    }
    bool isOptDisabled = context->getModuleMetaData()->compOpt.OptDisable;

    // create vbuilder->Compile() params
    llvm::SmallVector<const char*, 10> params;
    if (IGC_IS_FLAG_ENABLED(EnableVISADotAll))
    {
        params.push_back("-dotAll");
    }
    if (IGC_IS_FLAG_ENABLED(EnableVISADebug) || isOptDisabled)
    {
        params.push_back("-debug");
    }
    // Ensure VISA_Opts has the same scope as CreateVISABuilder so that valid
    // strings are checked by vISA and freed out of this function.
    if (IGC_IS_FLAG_ENABLED(VISAOptions))
    {
        std::vector<std::string> VISA_Opts;
        const char *DELIMITERS = " \t\n\v\f\r,"; // isspace(c), and comma for igcstandalone
        std::string line(IGC_GET_REGKEYSTRING(VISAOptions));
        std::size_t pos = 0;
        std::size_t found;
        for (; (found = line.find_first_of(DELIMITERS, pos)) != std::string::npos; ++pos) {
            // Skip consecutive whitespaces.
            if (found == pos)
                continue;
            VISA_Opts.push_back(line.substr(pos, found - pos));
            pos = found;
        }
        if (pos < line.length())
            VISA_Opts.push_back(line.substr(pos));
        for (auto &opt : VISA_Opts) {
            // note that the memory should be freed once
            // params has been read, but since this is only for
            // debugging, do not bother freeing memory.
            params.push_back(_strdup(opt.c_str()));
            if (opt == "-output" || opt == "-binary" || opt == "-dumpvisa" || opt == "-dumpcommonisa")
            {
                m_enableVISAdump = true;
            }
        }
    }

    if (IGC_IS_FLAG_ENABLED(ShaderDebugHashCodeInKernel))
    {
        QWORD AssemblyHash = { 0 };
        char Low[20], High[20];
        AssemblyHash = context->hash.getAsmHash();
        params.push_back("-hashmovs");
        sprintf_s(Low, sizeof(Low), "%d", (DWORD)AssemblyHash);
        params.push_back(Low);
        sprintf_s(High, sizeof(High), "%d", (DWORD)(AssemblyHash >> 32));
        params.push_back(High);
    }

    SetVISAWaTable(m_program->m_Platform->getWATable());

    bool enableVISADump = IGC_IS_FLAG_ENABLED(EnableVISASlowpath) || IGC_IS_FLAG_ENABLED(ShaderDumpEnable);
    V(CreateVISABuilder(vbuilder, vISA_3D, enableVISADump ? CM_CISA_BUILDER_BOTH : CM_CISA_BUILDER_GEN,
        VISAPlatform, params.size(), params.data(), &m_WaTable));

    // Set up options. This must be done before creating any variable/instructions
    // since some of the options affect IR building.

    if (IGC_IS_FLAG_ENABLED(ForceNoFP64bRegioning))
    {
        vbuilder->SetOption(vISA_forceNoFP64bRegioning, true);
    }

    if (IGC_IS_FLAG_ENABLED(DumpCompilerStats))
    {
        vbuilder->SetOption(vISA_DumpCompilerStats, true);
    }

    if (context->type == ShaderType::OPENCL_SHADER && context->m_floatDenormMode32 == FLOAT_DENORM_RETAIN &&
        context->m_floatDenormMode64 == FLOAT_DENORM_RETAIN)
    {
        vbuilder->SetOption(vISA_hasRNEandDenorm, true);
    }

    // need to fold ret into the previous RTWrite/URBWrite/etc
    if (context->type != ShaderType::OPENCL_SHADER && context->type != ShaderType::COMPUTE_SHADER)
    {
        {
            vbuilder->SetOption(vISA_foldEOTtoPrevSend, true);
        }
    }

    if (m_program->m_DriverInfo->clearScratchWriteBeforeEOT() &&
        (context->type == ShaderType::PIXEL_SHADER || context->type == ShaderType::OPENCL_SHADER))
    {
        vbuilder->SetOption(vISA_clearScratchWritesBeforeEOT, true);
    }

    if (context->type == ShaderType::PIXEL_SHADER)
    {
        vbuilder->SetOption(vISA_clearHDCWritesBeforeEOT, true);
    }

    // Disable multi-threaded latencies in the vISA scheduler when not in 3D
    if (context->type == ShaderType::OPENCL_SHADER)
    {
        if (m_program->m_Platform->singleThreadBasedInstScheduling())

        {
            vbuilder->SetOption(vISA_useMultiThreadedLatencies, false);
        }
    }

    auto enableScheduler = [=]() {
        // Check if preRA scheduler is disabled from input.
        if (isOptDisabled)
           return false;
        if (context->type == ShaderType::OPENCL_SHADER) {
            auto ClContext = static_cast<OpenCLProgramContext*>(context);
            if (!ClContext->m_InternalOptions.IntelEnablePreRAScheduling)
                return false;
        }

        // Check reg-key or compiler input
        if (IGC_IS_FLAG_ENABLED(ForceVISAPreSched) || context->getModuleMetaData()->csInfo.forcedVISAPreRAScheduler)
            return true;

        // API check.
        if (IGC_IS_FLAG_ENABLED(EnableVISAPreSched) &&
            m_program->m_DriverInfo->enableVISAPreRAScheduler())
            return true;

        return false;
    };

    if (enableScheduler())
    {
        vbuilder->SetOption(vISA_preRA_Schedule, true);
        if (uint32_t Val = IGC_GET_FLAG_VALUE(VISAPreSchedCtrl))
        {
            vbuilder->SetOption(vISA_preRA_ScheduleCtrl, Val);
        }
        else
        {
            uint32_t V = m_program->m_DriverInfo->getVISAPreRASchedulerCtrl();
            vbuilder->SetOption(vISA_preRA_ScheduleCtrl, V);
        }

        if (uint32_t Val = IGC_GET_FLAG_VALUE(VISAPreSchedRPThreshold))
        {
            vbuilder->SetOption(vISA_preRA_ScheduleRPThreshold, Val);
        }
    }
    else
    {
        vbuilder->SetOption(vISA_preRA_Schedule, false);
    }

    if (IGC_IS_FLAG_ENABLED(FastSpill))
    {
        vbuilder->SetOption(vISA_FastSpill, true);
    }

    vbuilder->SetOption(vISA_NoVerifyvISA, true);

    if (context->m_instrTypes.hasDebugInfo)
    {
        vbuilder->SetOption(vISA_GenerateDebugInfo, true);
    }

    if (canAbortOnSpill)
    {
        vbuilder->SetOption(vISA_AbortOnSpill, true);
        if (AvoidRetryOnSmallSpill())
        {
            // 2 means #spill/fill is roughly 1% of #inst
            // ToDo: tune the threshold
            vbuilder->SetOption(vISA_AbortOnSpillThreshold, 2u);
        }
    }

    if(context->m_retryManager.GetLastSpillSize() > 0)
    {
        if(context->m_retryManager.GetLastSpillSize() > g_cScratchSpaceMsglimit)
        {
            vbuilder->SetOption(vISA_UseScratchMsgForSpills, false);
        }
    }

    if ((context->type == ShaderType::OPENCL_SHADER || context->type == ShaderType::COMPUTE_SHADER) &&
        VISAPlatform >= GENX_SKL && IGC_IS_FLAG_ENABLED(EnablePreemption) && !hasStackCall)
    {
        vbuilder->SetOption(vISA_enablePreemption, true);
    }

    if (m_program->m_ScratchSpaceSize > 0)
    {
        vbuilder->SetOption(vISA_SpillMemOffset, m_program->m_ScratchSpaceSize);
    }

    if (IGC_IS_FLAG_ENABLED(forceGlobalRA))
    {
        vbuilder->SetOption(vISA_LocalRA, false);
        vbuilder->SetOption(vISA_LocalBankConflictReduction, false);
    }

    if (IGC_IS_FLAG_ENABLED(disableVarSplit))
    {
        vbuilder->SetOption(vISA_LocalDeclareSplitInGlobalRA, false);
    }

    if (IGC_IS_FLAG_ENABLED(disableRemat))
    {
        vbuilder->SetOption(vISA_NoRemat, true);
    }

    if (ForceNonCoherentStatelessBti || IGC_IS_FLAG_ENABLED(ForceNonCoherentStatelessBTI))
    {
        vbuilder->SetOption(vISA_noncoherentStateless, true);
    }

    if (IGC_IS_FLAG_ENABLED(DisableIfCvt))
    {
        vbuilder->SetOption(vISA_ifCvt, false);
    }

    if (IGC_IS_FLAG_DISABLED(EnableVISAStructurizer) ||
        m_program->m_Platform->getWATable().Wa_1407528679 != 0)
    {
        vbuilder->SetOption(vISA_EnableStructurizer, false);
    }
    else if (IGC_GET_FLAG_VALUE(EnableVISAStructurizer) == FLAG_SCF_UCFOnly)
    {
        vbuilder->SetOption(vISA_StructurizeCF, false);
    }

    if (IGC_IS_FLAG_DISABLED(EnableVISAJmpi))
    {
        vbuilder->SetOption(vISA_EnableScalarJmp, false);
    }

    if (IGC_IS_FLAG_ENABLED(DisableCSEL))
    {
        vbuilder->SetOption(vISA_enableCSEL, false);
    }
    if (IGC_IS_FLAG_ENABLED(DisableFlagOpt))
    {
        vbuilder->SetOption(vISA_LocalFlagOpt, false);
    }

    if (IGC_IS_FLAG_ENABLED(EnableVISAOutput))
    {
        vbuilder->SetOption(vISA_outputToFile, true);
        m_enableVISAdump = true;
    }
    if (IGC_IS_FLAG_ENABLED(EnableVISABinary))
    {
        vbuilder->SetOption(vISA_GenerateBinary, true);
        m_enableVISAdump = true;
    }
    if (IGC_IS_FLAG_ENABLED(EnableVISADumpCommonISA))
    {
        vbuilder->SetOption(vISA_DumpvISA, true);
        vbuilder->SetOption(vISA_GenerateISAASM, true);
        m_enableVISAdump = true;
    }
    if (IGC_IS_FLAG_ENABLED(EnableVISANoSchedule))
    {
        vbuilder->SetOption(vISA_LocalScheduling, false);
    }
    if (IGC_IS_FLAG_ENABLED(EnableVISANoBXMLEncoder))
    {
        vbuilder->SetOption(vISA_BXMLEncoder, false);
    }
    if (IGC_IS_FLAG_ENABLED(DisableMixMode))
    {
        vbuilder->SetOption(vISA_DisableMixMode, true);
    }
    if (IGC_IS_FLAG_ENABLED(ForceMixMode))
    {
        vbuilder->SetOption(vISA_ForceMixMode, true);
    }
    if (IGC_IS_FLAG_ENABLED(DisableHFMath))
    {
        vbuilder->SetOption(vISA_DisableHFMath, true);
    }

    if (IGC_IS_FLAG_ENABLED(disableIGASyntax))
    {
        vbuilder->SetOption(vISA_dumpNewSyntax, false);
    }
    if (IGC_IS_FLAG_ENABLED(disableCompaction))
    {
        vbuilder->SetOption(vISA_Compaction, false);
    }

    // In Vulkan and OGL buffer variable memory reads and writes within
    // a single shader invocation must be processed in order.
    if (m_program->m_DriverInfo->DisableDpSendReordering())
    {
        vbuilder->SetOption(vISA_ReorderDPSendToDifferentBti, false);
    }

    if (m_program->m_DriverInfo->UseALTMode())
    {
        vbuilder->SetOption(vISA_ChangeMoveType, false);
    }

    if (IGC_IS_FLAG_ENABLED(DisableSendS))
    {
        vbuilder->SetOption(vISA_UseSends, false);
    }
    if (m_program->m_DriverInfo->AllowUnsafeHalf())
    {
        vbuilder->SetOption(vISA_enableUnsafeCP_DF, true);
    }

    if (IGC_GET_FLAG_VALUE(UnifiedSendCycle) != 0)
    {
        vbuilder->SetOption(vISA_UnifiedSendCycle, IGC_GET_FLAG_VALUE(UnifiedSendCycle));
    }

    if (IGC_GET_FLAG_VALUE(ReservedRegisterNum) != 0 && (IGC_GET_FLAG_VALUE(TotalGRFNum) != 0))
    {
        assert(0 && "ReservedRegisterNum and TotalGRFNum registry keys cannot be used at the same time");
    }

    if (IGC_GET_FLAG_VALUE(ReservedRegisterNum) != 0)
    {
        vbuilder->SetOption(vISA_ReservedGRFNum, IGC_GET_FLAG_VALUE(ReservedRegisterNum));
    }

    vbuilder->SetOption(vISA_TotalGRFNum, context->getNumGRFPerThread());

    if (IGC_IS_FLAG_ENABLED(SystemThreadEnable))
    {
        /* Some tools only use 32bits hash, to maintain compatibility
        across lot of unknown tool chains doing Compare for only LowerPart
        */
        if (IGC_GET_FLAG_VALUE(ShaderDebugHashCode) == (DWORD)context->hash.getAsmHash())
        {
            vbuilder->SetOption(vISA_setStartBreakPoint, true);
        }
    }
    else if (KernelDebugEnable)
    {
        vbuilder->SetOption(vISA_AddKernelID, true);
        vbuilder->SetOption(vISA_setStartBreakPoint, true);
    }

    if (EnableBarrierInstCounterBits)
    {
        vbuilder->SetOption(VISA_EnableBarrierInstCounterBits, true);
    }
    if (preserveR0)
    {
        vbuilder->SetOption(vISA_ReserveR0, true);
    }
    if (IGC_IS_FLAG_ENABLED(InitializeRegistersEnable))
    {
        vbuilder->SetOption(vISA_InitPayload, true);
    }
    if (IGC_IS_FLAG_ENABLED(DumpPayloadToScratch))
    {
        vbuilder->SetOption(vISA_dumpPayload, true);
    }
    if (IGC_IS_FLAG_ENABLED(ExpandPlane))
    {
        vbuilder->SetOption(vISA_expandPlane, true);
    }
    if (IGC_IS_FLAG_ENABLED(EnableBCR))
    {
        vbuilder->SetOption(vISA_enableBCR, true);
    }
    if (IGC_IS_FLAG_ENABLED(forceSamplerHeader))
    {
        vbuilder->SetOption(vISA_forceSamplerHeader, true);
    }
    if (IGC_IS_FLAG_ENABLED(EnableIGAEncoder))
    {
        vbuilder->SetOption(vISA_IGAEncoder, true);
    }
    else
    {
        vbuilder->SetOption(vISA_IGAEncoder, false);
    }


    if (IGC_IS_FLAG_ENABLED(EnableAccSub))
    {
        vbuilder->SetOption(vISA_accSubstitution, true);
        uint32_t numAcc = IGC_GET_FLAG_VALUE(NumGeneralAcc);
        assert(numAcc >= 0 && numAcc <= 8 && "number of general acc should be [1-8] if set");
        if (numAcc > 0)
        {
            vbuilder->SetOption(vISA_numGeneralAcc, numAcc);
        }

        if (IGC_IS_FLAG_ENABLED(EnableAccSubDF))
        {
            vbuilder->SetOption(vISA_accSubDF, true);
        }
        if (IGC_IS_FLAG_ENABLED(EnableAccSubMadm))
        {
            vbuilder->SetOption(vISA_accSubMadm, true);
        }
    }
    else
    {
        vbuilder->SetOption(vISA_accSubstitution, false);
    }

    if (IGC_IS_FLAG_ENABLED(EnableNoDD))
    {
        vbuilder->SetOption(vISA_EnableNoDD, true);
    }

    if (IGC_IS_FLAG_ENABLED(GlobalSendVarSplit))
    {
        vbuilder->SetOption(vISA_GlobalSendVarSplit, true);
    }

    if (IGC_IS_FLAG_ENABLED(FuseTypedWrite))
    {
        vbuilder->SetOption(vISA_FuseTypedWrites, true);
    }

    if(IGC_IS_FLAG_ENABLED(ShaderDumpEnable) && IGC_IS_FLAG_ENABLED(InterleaveSourceShader))
    {
        vbuilder->SetOption(vISA_EmitLocation, true);
    }

    // Enable SendFusion for SIMD8
    if (IGC_IS_FLAG_ENABLED(EnableSendFusion) &&
        m_program->GetContext()->platform.supportSplitSend() &&
        m_program->m_dispatchSize == SIMDMode::SIMD8 &&
        (IGC_GET_FLAG_VALUE(EnableSendFusion) == FLAG_LEVEL_2 ||   // 2: force send fusion
         context->m_DriverInfo.AllowSendFusion()))
    {
        vbuilder->SetOption(vISA_EnableSendFusion, true);
        if (IGC_IS_FLAG_ENABLED(EnableAtomicFusion) &&
            context->type == ShaderType::OPENCL_SHADER)
        {
            vbuilder->SetOption(vISA_EnableAtomicFusion, true);
        }
    }

    if (context->getModuleMetaData()->compOpt.FastRelaxedMath ||
        context->getModuleMetaData()->compOpt.UnsafeMathOptimizations)
    {
        vbuilder->SetOption(vISA_unsafeMath, true);
    }

    // With statelessToStatefull on, it is possible that two different BTI messages
    // (two kernel arguments) might refer to the same memory. To be safe, turn off
    // visa DPSend reordering.
    if (IGC_IS_FLAG_ENABLED(EnableStatelessToStatefull) &&
        context->type == ShaderType::OPENCL_SHADER)
    {
        vbuilder->SetOption(vISA_ReorderDPSendToDifferentBti, false);
    }

    if (m_program->m_Platform->alignBindlessSampler())
    {
        vbuilder->SetOption(vISA_alignBindlessSampler, true);
    }

    vKernel = nullptr;

    const char* kernelName = m_program->entry->getName().data();
    if (context->m_instrTypes.hasDebugInfo)
    {
        // This metadata node is added by TransformBlocks pass for device side
        // enqueue feature of OCL2.0+.
        // The problem is that for device side enqueue, kernel name used in
        // IGC differs the one used to create JIT kernel. This leads to different
        // kernel names in .elf file and .dbg file. So dbgmerge tool cannot
        // merge the two together. With this metadata node we create a mapping
        // between the two names and when debug info is enabled, make JIT use
        // same name as IGC.
        // Names earlier -
        // ParentKernel_dispatch_0 in dbg and
        // __ParentKernel_block_invoke in elf
        // when kernel name is ParentKernel
        //
        auto md = m_program->entry->getParent()->getNamedMetadata("igc.device.enqueue");
        if (md)
        {
            for (unsigned int i = 0; i < md->getNumOperands(); i++)
            {
                auto mdOpnd = md->getOperand(i);
                auto first = dyn_cast_or_null<MDString>(mdOpnd->getOperand(1));
                if (first &&
                    first->getString().equals(kernelName))
                {
                    auto second = dyn_cast_or_null<MDString>(mdOpnd->getOperand(0));
                    if (second)
                    {
                        kernelName = second->getString().data();
                    }
                }
            }
        }
    }

    if (m_enableVISAdump || context->m_instrTypes.hasDebugInfo)
    {
        // vISA does not support string of length >= 255. Truncate if this exceeds
        // the limit. Note that vISA may append an extension, so relax it to a
        // random number 240 here.
        const int MAX_VISA_STRING_LENGTH = 240;
        if (m_program->entry->getName().size() >= MAX_VISA_STRING_LENGTH)
        {
            std::string shortName = m_program->entry->getName();
            shortName.resize(MAX_VISA_STRING_LENGTH);
            V(vbuilder->AddKernel(vKernel, shortName.c_str()));
        }
        else
        {
            V(vbuilder->AddKernel(vKernel, kernelName));
        }

        std::string asmName = IGC::Debug::GetDumpName(m_program, "asm");
        std::replace_if(asmName.begin(), asmName.end(),
            [](const char& c) {return c == '>' || c == '<'; }, '_');
        if (asmName.length() >= MAX_VISA_STRING_LENGTH)
        {
            asmName.resize(MAX_VISA_STRING_LENGTH);
        }
        V(vKernel->AddKernelAttribute("AsmName", asmName.length(), asmName.c_str()));
    }
    else
    {
        V(vbuilder->AddKernel(vKernel, "kernel"));
        V(vKernel->AddKernelAttribute("AsmName", std::strlen("0.asm") , "0.asm"));
    }

    vMainKernel = vKernel;

    if (gtpin_init)
    {
        vKernel->SetGTPinInit(gtpin_init);
    }

    // Right now only 1 main function in the kernel
    VISA_LabelOpnd *functionLabel = nullptr;
    V(vKernel->CreateVISALabelVar(functionLabel, "main", LABEL_SUBROUTINE));
    V(vKernel->AppendVISACFLabelInst(functionLabel));

    V(vKernel->CreateVISASurfaceVar(dummySurface, "", 1));

    V(vKernel->CreateVISASamplerVar(samplervar, "", 1));

    CEncoder::SetFloatDenormMode(vKernel, context->m_floatDenormMode16,
                                          context->m_floatDenormMode32,
                                          context->m_floatDenormMode64);

    // The instruction is generated only if mode != FLOAT_ROUND_TO_NEAREST_EVEN
    CEncoder::SetFloatRoundingMode(
        getEncoderRoundingMode(FLOAT_ROUND_TO_NEAREST_EVEN),
        getEncoderRoundingMode(static_cast<Float_RoundingMode>(
            context->getModuleMetaData()->compOpt.FloatRoundingMode)));
}

void CEncoder::SetKernelStackPointer64()
{
    assert(vKernel);
    int spSize = 64;
    V(vKernel->AddKernelAttribute("FESPSize", sizeof(spSize), &spSize));
}

void CEncoder::SetStackFunctionArgSize(uint size)
{
    assert(vKernel);
    V(vKernel->AddKernelAttribute("ArgSize", sizeof(size), &size));
}

void CEncoder::SetStackFunctionRetSize(uint size)
{
    assert(vKernel);
    V(vKernel->AddKernelAttribute("RetValSize", sizeof(size), &size));
}

SEncoderState CEncoder::CopyEncoderState()
{
    return m_encoderState;
}

void CEncoder::SetEncoderState(SEncoderState &newState)
{
    m_encoderState = newState;
}

static inline VISA_Align GetVISAAlign(CVariable* var)
{
    VISA_Align align;
    switch(var->GetAlign())
    {
    case EALIGN_BYTE: align = ALIGN_BYTE;
        break;
    case EALIGN_WORD: align = ALIGN_WORD;
        break;
    case EALIGN_DWORD: align = ALIGN_DWORD;
        break;
    case EALIGN_QWORD: align = ALIGN_QWORD;
        break;
    case EALIGN_OWORD: align = ALIGN_OWORD;
        break;
    case EALIGN_GRF: align = ALIGN_GRF;
        break;
    case EALIGN_2GRF: align = ALIGN_2_GRF;
        break;
    default:
        align = ALIGN_UNDEF;
        assert(0);
    }
    return align;
}

VISA_GenVar* CEncoder::GetVISAVariable(CVariable* var)
{
    if(m_encoderState.m_secondHalf)
    {
        if(var->GetNumberInstance() == 2)
        {
            return var->visaGenVariable[1];
        }
    }
    return var->visaGenVariable[0];
}

void CEncoder::GetVISAPredefinedVar(CVariable* pVar, PreDefined_Vars var)
{
    vKernel->GetPredefinedVar(pVar->visaGenVariable[0], var);
    switch (var) {
    case PREDEFINED_TSC:
    case PREDEFINED_SR0:
    case PREDEFINED_CR0:
    case PREDEFINED_CE0:
    case PREDEFINED_DBG:
        // Creating alias to ARF is not allowed.
        return;
    default:
        break;
    }

    VISA_GenVar* pAliasGenVar = nullptr;

    // Create alias to the specified pre-defined variable to match the
    // requested types and elements..
    vKernel->CreateVISAGenVar(
        pAliasGenVar,
        "",
        pVar->GetNumberElement(),
        pVar->GetType(),
        ALIGN_GRF,
        pVar->visaGenVariable[0],
        pVar->GetAliasOffset());

    pVar->visaGenVariable[0] = pAliasGenVar;
}

void CEncoder::CreateVISAVar(CVariable* var)
{
    if(var->GetAlias()!=NULL)
    {
        var->ResolveAlias();
        // In case the alias is an exact copy or just a sub variable just re-use the variable
        if(var->GetAlias()->GetType() == var->GetType())
        {
            for(uint i = 0; i<var->GetNumberInstance();i++)
            {
                var->visaGenVariable[i] = var->GetAlias()->visaGenVariable[i];
            }
        }
        else
        {
            SAlias alias(var->GetAlias(), var->GetType());
            auto aliasPair = m_aliasesMap.insert(std::pair<SAlias, CVariable*>(alias, var));
            if(aliasPair.second == false)
            {
                for(uint i = 0; i < var->GetNumberInstance(); i++)
                {
                    var->visaGenVariable[i] = aliasPair.first->second->visaGenVariable[i];
                }
            }
            else
            {
                assert(var->GetType() != ISA_TYPE_BOOL && "boolean cannot have alias");
                for(uint i = 0; i < var->GetNumberInstance(); i++)
                {
                    // Since we no longer use the built-in alias offset mechanism,
                    // we have to create the aliases to be of at least the size of the
                    // original variable (in bytes)
                    // Otherwise, we may end up a situation where we have an alias with
                    // an offset (m_aliasOffset, that we don't notify vISA about),
                    // and make an out-of-bounds access.
                    // This is the opposite of the calculation that happens in
                    // CVariable::CVariable.

                    uint16_t nbElement =
                        var->GetAlias()->GetNumberElement() *
                        CEncoder::GetCISADataTypeSize(var->GetAlias()->GetType()) /
                        CEncoder::GetCISADataTypeSize(var->GetType());

                    V(vKernel->CreateVISAGenVar(
                        var->visaGenVariable[i],
                        "",
                        nbElement,
                        var->GetType(),
                        GetVISAAlign(var->GetAlias()), // Use parent's align as we create an alias of the parent.
                        var->GetAlias()->visaGenVariable[i],
                        0));
                }
            }
        }
    }
    else
    {
        uint num_elts = var->GetNumberElement();
        if( var->GetVarType() == EVARTYPE_GENERAL )
        {
            var->visaGenVariable[0] = nullptr;
            var->visaGenVariable[1] = nullptr;
            assert(var->GetType() != ISA_TYPE_BOOL && "boolean cannot be general var");
            for(uint i = 0; i<var->GetNumberInstance();i++)
            {
                V(vKernel->CreateVISAGenVar(
                    var->visaGenVariable[i],
                    "",
                    num_elts,
                    var->GetType(),
                    GetVISAAlign(var)));
            }
        }
        else if(var->GetVarType() == EVARTYPE_PREDICATE)
        {
            unsigned short nb = int_cast<unsigned short>(num_elts)*var->GetNumberInstance();
            V(vKernel->CreateVISAPredVar(
                var->visaPredVariable,
                "",
                nb));
        }
        else
        {
            // when both array and index are uniform so is the destination address variable
            uint nb =  (var->IsUniform() && var->IsVectorUniform()) ? 1 : var->GetNumberElement();
            V(vKernel->CreateVISAAddrVar(var->visaAddrVariable, "", nb));
        }
    }
}

void CEncoder::DeclareInput(CVariable* var, uint offset, uint instance)
{
    V(vKernel->CreateVISAInputVar(
        var->visaGenVariable[instance],
        int_cast<unsigned short>(offset),
        int_cast<unsigned short>(var->GetSize())));
}

void CEncoder::MarkAsOutput(CVariable* var)
{
    for(unsigned int i = 0; i < var->GetNumberInstance(); i++)
    {
        V(vKernel->AddAttributeToVar(var->visaGenVariable[i], "Output", 0, nullptr));
    }
}

bool CEncoder::AvoidRetryOnSmallSpill() const
{
    CodeGenContext* context = m_program->GetContext();
    return context->type == ShaderType::PIXEL_SHADER &&
        m_program->m_dispatchSize == SIMDMode::SIMD8 &&
        context->m_retryManager.IsFirstTry();
}

void CEncoder::CreateFunctionSymbolTable(void*& buffer, unsigned& bufferSize, unsigned& tableEntries)
{
    buffer = nullptr;
    bufferSize = 0;
    tableEntries = 0;

    if (IGC_IS_FLAG_ENABLED(EnableFunctionPointer))
    {
        Module* pModule = m_program->GetContext()->getModule();
        std::vector<Function*> funcsToExport;
        for (auto &F : pModule->getFunctionList())
        {
            // Find all functions in the module we need to export as symbols
            if (F.hasFnAttribute("AsFunctionPointer"))
            {
                if (!F.isDeclaration() || F.getNumUses() > 0)
                    funcsToExport.push_back(&F);
            }
        }

        if (funcsToExport.empty())
            return;

        // Allocate buffer to store symbol table entries
        tableEntries = funcsToExport.size();
        bufferSize = sizeof(IGC::GenSymEntry) * tableEntries;
        buffer = (void*) malloc(bufferSize);
        assert(buffer && "Function Symbol Table not allocated");
        IGC::GenSymEntry* entry_ptr = (IGC::GenSymEntry*) buffer;

        for (auto pFunc : funcsToExport)
        {
            assert(pFunc->getName().size() <= IGC::MAX_SYMBOL_NAME_LENGTH);
            strcpy(entry_ptr->s_name, pFunc->getName().str().c_str());

            if (pFunc->isDeclaration())
            {
                // If the function is only declared, set as undefined type
                entry_ptr->s_type = IGC::GenSymType::S_UNDEF;
                entry_ptr->s_offset = 0;
            }
            else
            {
                auto Iter = stackFuncMap.find(pFunc);
                assert(Iter != stackFuncMap.end() && "vISA function not found");

                // Query vISA for the function's byte offset within the compiled module
                VISAFunction* visaFunc = Iter->second;
                entry_ptr->s_type = IGC::GenSymType::S_FUNC;
                entry_ptr->s_offset = (uint32_t) visaFunc->getGenOffset();
            }
            entry_ptr++;
        }
    }
}
void CEncoder::CreateFunctionRelocationTable(void*& buffer, unsigned& bufferSize, unsigned& tableEntries)
{
    buffer = nullptr;
    bufferSize = 0;
    tableEntries = 0;

    if (IGC_IS_FLAG_ENABLED(EnableFunctionPointer))
    {
        // vISA will directly return the buffer with GenRelocEntry layout
        V(vMainKernel->GetGenRelocEntryBuffer(buffer, bufferSize, tableEntries));
        assert(sizeof(IGC::GenRelocEntry) * tableEntries == bufferSize);
    }
}

void CEncoder::Compile()
{
    COMPILER_TIME_START(m_program->GetContext(), TIME_CG_vISAEmitPass);

    CodeGenContext* context = m_program->GetContext();
    SProgramOutput* pOutput = m_program->ProgramOutput();

    if( m_program->m_dispatchSize == SIMDMode::SIMD8 )
    {
        MEM_SNAPSHOT( IGC::SMS_AFTER_CISACreateDestroy_SIMD8 );
    }
    else if( m_program->m_dispatchSize == SIMDMode::SIMD16 )
    {
        MEM_SNAPSHOT( IGC::SMS_AFTER_CISACreateDestroy_SIMD16 );
    }
    else if( m_program->m_dispatchSize == SIMDMode::SIMD32 )
    {
        MEM_SNAPSHOT( IGC::SMS_AFTER_CISACreateDestroy_SIMD32 );
    }

    COMPILER_TIME_END(m_program->GetContext(), TIME_CG_vISAEmitPass);

    COMPILER_TIME_START(m_program->GetContext(), TIME_CG_vISACompile);

    //Compile to generate the V-ISA binary
    //TARGET_PLATFORM VISAPlatform = GetVISAPlatform(m_Platform);
    int vIsaCompile = 0;
    if( m_enableVISAdump )
    {
        std::string isaName = IGC::Debug::GetDumpName(m_program, "isa");
        std::replace_if(isaName.begin(), isaName.end(),
            [](const char& c) {return c == '>' || c == '<'; }, '_');
        // vISA does not support string of length >= 255. Truncate if this exceeds
        // the limit. Note that vISA may append an extension, so relax it to a
        // random number 240 here.
        const int MAX_VISA_STRING_LENGTH = 240;
        if (isaName.length() >= MAX_VISA_STRING_LENGTH)
        {
            isaName.resize(MAX_VISA_STRING_LENGTH);
        }
        vIsaCompile = vbuilder->Compile(const_cast<char*>(isaName.c_str()));
    }
    else
    {
        vIsaCompile = vbuilder->Compile("");
    }
    FINALIZER_INFO *jitInfo;
    vMainKernel->GetJitInfo(jitInfo);
    if(jitInfo->isSpill)
    {
        context->m_retryManager.SetSpillSize(jitInfo->numGRFSpillFill);
        m_program->m_spillSize = jitInfo->numGRFSpillFill;
        m_program->m_spillCost =
            float(jitInfo->numGRFSpillFill) / jitInfo->numAsmCount;

        context->m_retryManager.numInstructions = jitInfo->numAsmCount;
    }
    COMPILER_TIME_END(m_program->GetContext(), TIME_CG_vISACompile);

#if GET_TIME_STATS
    // handle the vISA time counters differently here
    if (context->m_compilerTimeStats)
    {
        context->m_compilerTimeStats->recordVISATimers();
    }
#endif

    if (IGC_IS_FLAG_ENABLED(DumpCompilerStats))
    {
        CompilerStats CompilerStats;
        vMainKernel->GetCompilerStats(CompilerStats);
        CompilerStatsUtils::RecordCodeGenCompilerStats(context, m_program->m_dispatchSize, CompilerStats, jitInfo);
    }

    if( vIsaCompile == -1 )
    {
        assert(0 && "CM failure in vbuilder->Compile()");
    }
    else if( vIsaCompile == -2 )
    {
        assert(0 && "CM user error in vbuilder->Compile()");
    }
    else if( vIsaCompile == -3 ) // CM early terminates on spill
    {
#if (GET_SHADER_STATS)
        if (m_program->m_dispatchSize == SIMDMode::SIMD16)
        {
            COMPILER_SHADER_STATS_SET(m_program->m_shaderStats, STATS_ISA_EARLYEXIT16, 1);
        }
        else if (m_program->m_dispatchSize == SIMDMode::SIMD32)
        {
            COMPILER_SHADER_STATS_SET(m_program->m_shaderStats, STATS_ISA_EARLYEXIT32, 1);
        }
#endif
        return;
    }

    COMPILER_TIME_START(m_program->GetContext(), TIME_CG_vISAEmitPass);

    if(m_program->m_dispatchSize == SIMDMode::SIMD8 )
    {
        MEM_SNAPSHOT( IGC::SMS_AFTER_vISACompile_SIMD8 );
    }
    else if( m_program->m_dispatchSize == SIMDMode::SIMD16 )
    {
        MEM_SNAPSHOT( IGC::SMS_AFTER_vISACompile_SIMD16 );
    }
    else if( m_program->m_dispatchSize == SIMDMode::SIMD32 )
    {
        MEM_SNAPSHOT( IGC::SMS_AFTER_vISACompile_SIMD32 );
    }

    if (m_program->m_dispatchSize == SIMDMode::SIMD16)
    {
        uint sendStallCycle = 0;
        uint staticCycle = 0;
        for (uint i = 0; i < jitInfo->BBNum; i++)
        {
            sendStallCycle += jitInfo->BBInfo[i].sendStallCycle;
            staticCycle += jitInfo->BBInfo[i].staticCycle;
        }
        m_program->m_sendStallCycle = sendStallCycle;
        m_program->m_staticCycle = staticCycle;
    }

    if (jitInfo->isSpill && AvoidRetryOnSmallSpill())
    {
        context->m_retryManager.Disable();
    }

#if (GET_SHADER_STATS && !PRINT_DETAIL_SHADER_STATS)
    if( m_program->m_dispatchSize == SIMDMode::SIMD8 )
    {
        COMPILER_SHADER_STATS_SET(m_program->m_shaderStats, STATS_ISA_INST_COUNT, jitInfo->numAsmCount);
        COMPILER_SHADER_STATS_SET(m_program->m_shaderStats, STATS_ISA_SPILL8, (int)jitInfo->isSpill);
    }
    else if( m_program->m_dispatchSize == SIMDMode::SIMD16 )
    {
        COMPILER_SHADER_STATS_SET(m_program->m_shaderStats, STATS_ISA_INST_COUNT_SIMD16, jitInfo->numAsmCount);
        COMPILER_SHADER_STATS_SET(m_program->m_shaderStats, STATS_ISA_SPILL16, (int)jitInfo->isSpill);
    }
    else if( m_program->m_dispatchSize == SIMDMode::SIMD32 )
    {
        COMPILER_SHADER_STATS_SET(m_program->m_shaderStats, STATS_ISA_INST_COUNT_SIMD32, jitInfo->numAsmCount);
        COMPILER_SHADER_STATS_SET(m_program->m_shaderStats, STATS_ISA_SPILL32, (int)jitInfo->isSpill);
    }
#endif

    void* genxbin = nullptr;
    int size = 0, binSize = 0;
    bool binOverride = false;

    V(vMainKernel->GetGenxBinary(genxbin, binSize));
    if (IGC_IS_FLAG_ENABLED(ShaderOverride))
    {
            Debug::DumpName name = IGC::Debug::GetDumpNameObj(m_program, "asm");
            std::string binFileName = name.overridePath();

            overrideShaderIGA(context, genxbin, binSize, binFileName, binOverride);

            if (!binOverride)
            {
                name = IGC::Debug::GetDumpNameObj(m_program, "dat");
                binFileName = name.overridePath();
                overrideShaderBinary(genxbin, binSize, binFileName, binOverride);
            }

    }

    assert(genxbin!=nullptr);
    size = binSize;

    // the kernel has to be padded to have a size aligned on 64 bytes
    size_t padding = iSTD::GetAlignmentOffset( size, 64);//m_program->m_Platform->getKernelPointerAlignSize() );
    void *kernel = IGC::aligned_malloc(size + padding, 16 /* sizeof(DQWORD) */);
    memcpy_s(kernel, size + padding, genxbin, binSize);
    // pad out the rest with 0s
    memset( static_cast<char*>(kernel)+size, 0, padding );

    if (binOverride)
    {
        free(genxbin);
    }
    else
    {
        freeBlock(genxbin);
    }

    void *dbgInfo = nullptr;
    unsigned int dbgSize = 0;
    if (context->m_instrTypes.hasDebugInfo)
    {
        void* genxdbgInfo = nullptr;
        void* VISAMap = nullptr;
        unsigned int numElems = 0;
        V(vMainKernel->GetGenxDebugInfo(genxdbgInfo, dbgSize, VISAMap, numElems));
        if (m_enableVISAdump)
        {
            std::string debugFileNameStr = IGC::Debug::GetDumpName(m_program, "dbg");
            FILE* dbgFile = fopen(debugFileNameStr.c_str(), "wb+");
            if (dbgFile != NULL)
            {
                fwrite(genxdbgInfo, dbgSize, 1, dbgFile);
                fclose(dbgFile);
            }
        }

        dbgInfo = IGC::aligned_malloc(dbgSize, sizeof(void*));

        memcpy_s(dbgInfo, dbgSize, genxdbgInfo, dbgSize);

        freeBlock(genxdbgInfo);

        if (numElems > 0 && VISAMap)
        {
            for (unsigned int i = 0; i < numElems*2; i+=2)
            {
                auto GenISAOffset = ((unsigned int*)VISAMap)[i];
                auto VISAIndex = ((unsigned int*)VISAMap)[i+1];
                m_program->m_VISAIndexToGenISAOff.push_back(std::make_pair(VISAIndex, GenISAOffset));
            }

            freeBlock(VISAMap);
        }
    }

    pOutput->m_programBin   = kernel;
    pOutput->m_programSize  = size + padding;
    pOutput->m_unpaddedProgramSize = size;
    pOutput->m_scratchSpaceUsedBySpills = 0; // initializing
    pOutput->m_debugDataGenISA = dbgInfo;
    pOutput->m_debugDataGenISASize = dbgSize;
    pOutput->m_InstructionCount = jitInfo->numAsmCount;

    vMainKernel->GetGTPinBuffer(pOutput->m_gtpinBuffer, pOutput->m_gtpinBufferSize);

    CreateFunctionSymbolTable(pOutput->m_funcSymbolTable,
                              pOutput->m_funcSymbolTableSize,
                              pOutput->m_funcSymbolTableEntries);
    CreateFunctionRelocationTable(pOutput->m_funcRelocationTable,
                                  pOutput->m_funcRelocationTableSize,
                                  pOutput->m_funcRelocationTableEntries);

    if (jitInfo->isSpill == true)
    {
        pOutput->m_scratchSpaceUsedBySpills = jitInfo->spillMemUsed;
    }

    pOutput->m_scratchSpaceUsedByShader = m_program->m_ScratchSpaceSize;

    pOutput->m_scratchSpaceUsedByGtpin = jitInfo->numBytesScratchGtpin;

    COMPILER_TIME_END(m_program->GetContext(), TIME_CG_vISAEmitPass);
}

void CEncoder::DestroyVISABuilder()
{
    V(::DestroyVISABuilder(vbuilder));
    vbuilder = nullptr;
}

void CEncoder::Copy(CVariable* dst, CVariable* src)
{
    // undef value are not copied
    if(!src->IsUndef() || IGC_IS_FLAG_ENABLED(InitializeUndefValueEnable))
    {
        CVariable* rawDst = dst;
        assert(GetCISADataTypeSize(src->GetType()) == GetCISADataTypeSize(dst->GetType()));
        bool isVecImm = src->IsImmediate() && (src->GetType() == ISA_TYPE_UV ||
                                               src->GetType() == ISA_TYPE_V ||
                                               src->GetType() == ISA_TYPE_VF);
        if(src->GetType() != dst->GetType() && !isVecImm)
        {
            rawDst = m_program->BitCast(dst, src->GetType());
        }
        DataMov(ISA_MOV, rawDst, src);
    }
}

void CEncoder::BoolToInt(CVariable* dst, CVariable* src)
{
    assert(src->GetType() == ISA_TYPE_BOOL);
    assert((dst->GetType() == ISA_TYPE_UD) || (dst->GetType() == ISA_TYPE_D));

    // undef value are not copied
    if(!src->IsUndef() || IGC_IS_FLAG_ENABLED(InitializeUndefValueEnable)) {
        // Casting 'dst' to BOOL is unnecessary.
        DataMov(ISA_MOV, dst, src);
    }
}

void CEncoder::GatherA64(CVariable *dst,
                         CVariable *offset,
                         unsigned elemSize,
                         unsigned numElems) {
    assert((elemSize == 8 || elemSize == 32 || elemSize == 64) &&
           "Only B/DW/QW-sized elements are supported!");
    assert((numElems == 1 || numElems == 2 || numElems == 4 ||
           (numElems == 8 && (elemSize == 32 || m_program->m_Platform->has8ByteA64ByteScatteredMessage()))) &&
           "Only 1/2/4/8 elements are supported!");

    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
    VISA_RawOpnd *addressOpnd = GetRawSource(offset);
    VISA_RawOpnd *dstOpnd = GetRawDestination(dst);

    SIMDMode thisSM = offset->IsUniform() ? lanesToSIMDMode(offset->GetNumberElement()) : m_encoderState.m_simdSize;
    if (m_program->m_Platform->GetPlatformFamily() == IGFX_GEN8_CORE && thisSM == SIMDMode::SIMD16)
    {
        // BDW A64 gather does not support SIMD16, split it into 2 SIMD8
        Common_VISA_EMask_Ctrl execMask = GetAluEMask(offset);
        Common_ISA_Exec_Size fromExecSize = EXEC_SIZE_16;
        Common_ISA_Exec_Size toExecSize = EXEC_SIZE_8;

        if (numElems == 1 || elemSize == 8)
        {   // No mov instructions (for packing) are needed.
            for (unsigned p = 0; p < 2; ++p)
            {
                addressOpnd = GetRawSource(offset, GetRawOpndSplitOffset(fromExecSize, toExecSize, p, offset));
                dstOpnd = GetRawDestination(dst, GetRawOpndSplitOffset(fromExecSize, toExecSize, p, dst));

                V(vKernel->AppendVISASvmGatherInst(
                    predOpnd,
                    SplitEMask(fromExecSize, toExecSize, p, execMask),
                    toExecSize,
                    visaBlockType(elemSize),
                    visaBlockNum(numElems),
                    addressOpnd, dstOpnd));
            }
        }
        else
        {
            // Do two SIMD8 gather and then merge (pack) the two simd8 results
            // to form the single simd16 payload.
            CVariable* V0, *V1;
            uint16_t newNumElems = (uint16_t) 8 * numElems;
            V0 = m_program->GetNewVariable(newNumElems,
                                           dst->GetType(),
                                           dst->GetAlign(),
                                           dst->IsUniform());
            V1 = m_program->GetNewVariable(newNumElems,
                                           dst->GetType(),
                                           dst->GetAlign(),
                                           dst->IsUniform());

            for (unsigned p = 0; p < 2; ++p)
            {
                addressOpnd = GetRawSource(offset, GetRawOpndSplitOffset(fromExecSize, toExecSize, p, offset));
                dstOpnd = GetRawDestination(p == 0 ? V0 : V1);

                V(vKernel->AppendVISASvmGatherInst(
                    predOpnd,
                    SplitEMask(fromExecSize, toExecSize, p, execMask),
                    toExecSize,
                    visaBlockType(elemSize),
                    visaBlockNum(numElems),
                    addressOpnd, dstOpnd));
            }

            uint32_t dstOfstBytes = dst->GetAliasOffset() + m_encoderState.m_dstOperand.subVar * SIZE_GRF;
            MergeMDP8To16(V0, V1, numElems, dst, dstOfstBytes);
        }
        return;
    }

    V(vKernel->AppendVISASvmGatherInst(predOpnd, GetAluEMask(offset),
                                       visaExecSize(offset->IsUniform() ? lanesToSIMDMode(offset->GetNumberElement()) : m_encoderState.m_simdSize),
                                       visaBlockType(elemSize),
                                       visaBlockNum(numElems),
                                       addressOpnd, dstOpnd));
}

void CEncoder::ScatterA64(CVariable *src,
                          CVariable *offset,
                          unsigned elemSize,
                          unsigned numElems) {
    assert((elemSize == 8 || elemSize == 32 || elemSize == 64) &&
           "Only B/DW/QW-sized elements are supported!");
    assert((numElems == 1 || numElems == 2 || numElems == 4 ||
           (numElems == 8 && (elemSize == 32 || m_program->m_Platform->has8ByteA64ByteScatteredMessage()))) &&
           "Only 1/2/4/8 elements are supported!");

    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
    VISA_RawOpnd *addressOpnd = GetRawSource(offset);
    VISA_RawOpnd *srcOpnd = GetRawSource(src);

    SIMDMode thisSM = offset->IsUniform() ? lanesToSIMDMode(offset->GetNumberElement()) : m_encoderState.m_simdSize;
    if( m_program->m_Platform->GetPlatformFamily() == IGFX_GEN8_CORE && thisSM == SIMDMode::SIMD16 )
    {
        // BDW A64 scatter does not support SIMD16, split it into 2 SIMD8
        Common_VISA_EMask_Ctrl execMask = GetAluEMask(offset);
        Common_ISA_Exec_Size fromExecSize = EXEC_SIZE_16;
        Common_ISA_Exec_Size toExecSize = EXEC_SIZE_8;

        if (numElems == 1 || elemSize == 8)
        {   // No unpacking (mov instructions) are needed.
            for (unsigned p = 0; p < 2; ++p)
            {
                addressOpnd = GetRawSource(offset, GetRawOpndSplitOffset(fromExecSize, toExecSize, p, offset));
                srcOpnd = GetRawSource(src, GetRawOpndSplitOffset(fromExecSize, toExecSize, p, src));
                V(vKernel->AppendVISASvmScatterInst(
                    predOpnd,
                    SplitEMask(fromExecSize, toExecSize, p, execMask),
                    toExecSize,
                    visaBlockType(elemSize),
                    visaBlockNum(numElems),
                    addressOpnd, srcOpnd));
            }
        }
        else
        {
            // Unpacking the original simd16 data payload to form the two simd8
            // data payload by splitting the original simd16 data payload.
            CVariable *V0, *V1;
            uint16_t newNumElems = (uint16_t) 8 * numElems;
            V0 = m_program->GetNewVariable(newNumElems,
                                           src->GetType(),
                                           src->GetAlign(),
                                           src->IsUniform());
            V1 = m_program->GetNewVariable(newNumElems,
                                           src->GetType(),
                                           src->GetAlign(),
                                           src->IsUniform());
            // Starting offset is calculated from AliasOffset only (subVar not used).
            uint32_t srcOfstBytes = src->GetAliasOffset();
            SplitMDP16To8(src, srcOfstBytes, numElems, V0, V1);

            for (unsigned p = 0; p < 2; ++p)
            {
                addressOpnd = GetRawSource(offset, GetRawOpndSplitOffset(fromExecSize, toExecSize, p, offset));
                srcOpnd = GetRawSource(p == 0 ? V0 : V1);

                V(vKernel->AppendVISASvmScatterInst(
                    predOpnd,
                    SplitEMask(fromExecSize, toExecSize, p, execMask),
                    toExecSize,
                    visaBlockType(elemSize),
                    visaBlockNum(numElems),
                    addressOpnd, srcOpnd));
            }
        }
        return;
    }

    V(vKernel->AppendVISASvmScatterInst(predOpnd, GetAluEMask(offset),
                                        visaExecSize(offset->IsUniform() ? lanesToSIMDMode(offset->GetNumberElement()) : m_encoderState.m_simdSize),
                                        visaBlockType(elemSize),
                                        visaBlockNum(numElems),
                                        addressOpnd, srcOpnd));
}

void CEncoder::ByteGather(CVariable *dst,
                          const ResourceDescriptor& resource,
                          CVariable *offset,
                          unsigned elemSize,
                          unsigned numElems) {
    assert(elemSize == 8 && "Only BYTE element is supported!");
    assert((numElems == 1 || numElems == 2 || numElems == 4) &&
           "Only 1/2/4 elements are supported!");

    // Extend the offset to 64bits and use the A64 gather message if needed
    if((resource.m_surfaceType == ESURFACE_STATELESS) &&
        (m_program->m_DriverInfo->NeedWAToTransformA32MessagesToA64()) &&
        (m_program->m_Platform->getWATable().WaNoA32ByteScatteredStatelessMessages != 0))
    {
        SEncoderState gatherState = CopyEncoderState();
        Push();

        CVariable* offset64 = m_program->GetNewVariable(offset->GetNumberElement(),
            ISA_TYPE_UQ,
            EALIGN_GRF,
            offset->IsUniform(),
            offset->GetNumberInstance());

        CVariable *offset32UD = m_program->BitCast(offset, ISA_TYPE_UD);

        if(offset->IsUniform())
        {
            uint elements = offset->GetNumberElement();
            SetUniformSIMDSize(lanesToSIMDMode(elements));
            SetNoMask();
            SetSrcRegion(0, elements, elements, 1);
        }

        Cast(offset64, offset32UD);
        Push();

        SetEncoderState(gatherState);
        GatherA64(dst, offset64, elemSize, numElems);
        return;

    }

    VISA_StateOpndHandle* surfaceOpnd = GetVISASurfaceOpnd(resource);
    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
    VISA_RawOpnd *addressOpnd = GetRawSource(offset);
    VISA_RawOpnd *dstOpnd = GetRawDestination(dst);

    VISA_VectorOpnd *globalOffsetOpnd = 0;
    int val = 0;
    V(vKernel->CreateVISAImmediate(globalOffsetOpnd, &val, ISA_TYPE_UD));

    V(vKernel->AppendVISASurfAccessScatterScaledInst(ISA_GATHER_SCALED,
                                                     predOpnd,
                                                     GetAluEMask(offset),
                                                     visaExecSize(offset->IsUniform() ? lanesToSIMDMode(offset->GetNumberElement()) :
                                                                                        m_encoderState.m_simdSize),
                                                     visaBlockNum(numElems),
                                                     surfaceOpnd,
                                                     globalOffsetOpnd,
                                                     addressOpnd, dstOpnd));
}

void CEncoder::ByteScatter(CVariable *src,
                           const ResourceDescriptor& resource,
                           CVariable *offset,
                           unsigned elemSize,
                           unsigned numElems) {
    assert(elemSize == 8 && "Only BYTE element is supported!");
    assert((numElems == 1 || numElems == 2 || numElems == 4) &&
           "Only 1/2/4 elements are supported!");

    // Extend the offset to 64bits and use the A64 gather message if needed
    if((resource.m_surfaceType == ESURFACE_STATELESS) &&
       (m_program->m_DriverInfo->NeedWAToTransformA32MessagesToA64()) &&
       (m_program->m_Platform->getWATable().WaNoA32ByteScatteredStatelessMessages != 0))
    {
        SEncoderState gatherState = CopyEncoderState();
        Push();

        CVariable* offset64 = m_program->GetNewVariable(offset->GetNumberElement(),
            ISA_TYPE_UQ,
            EALIGN_GRF,
            offset->IsUniform(),
            offset->GetNumberInstance());

        CVariable *offset32UD = m_program->BitCast(offset, ISA_TYPE_UD);

        if(offset->IsUniform())
        {
            uint elements = offset->GetNumberElement();
            SetUniformSIMDSize(lanesToSIMDMode(elements));
            SetNoMask();
            SetSrcRegion(0, elements, elements, 1);
        }

        Cast(offset64, offset32UD);
        Push();

        SetEncoderState(gatherState);
        ScatterA64(src, offset64, elemSize, numElems);
        return;

    }

    VISA_StateOpndHandle* surfaceOpnd = GetVISASurfaceOpnd(resource);
    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
    VISA_RawOpnd *addressOpnd = GetRawSource(offset);
    VISA_RawOpnd *srcOpnd = GetRawSource(src);

    VISA_VectorOpnd *globalOffsetOpnd = 0;
    int val = 0;
    V(vKernel->CreateVISAImmediate(globalOffsetOpnd, &val, ISA_TYPE_UD));

    V(vKernel->AppendVISASurfAccessScatterScaledInst(ISA_SCATTER_SCALED,
                                                     predOpnd,
                                                     GetAluEMask(offset),
                                                     visaExecSize(offset->IsUniform() ? lanesToSIMDMode(offset->GetNumberElement()) :
                                                                                        m_encoderState.m_simdSize),
                                                     visaBlockNum(numElems),
                                                     surfaceOpnd,
                                                     globalOffsetOpnd,
                                                     addressOpnd, srcOpnd));
}

void CEncoder::Gather4ScaledNd(CVariable *dst,
    const ResourceDescriptor& resource,
    CVariable *offset,
    unsigned nd ) {

    VISA_StateOpndHandle* surfaceOpnd = GetVISASurfaceOpnd(resource);
    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
    VISA_RawOpnd *addressOpnd = GetRawSource(offset);
    VISA_RawOpnd *dstOpnd = GetRawDestination(dst);

    VISA_VectorOpnd *globalOffsetOpnd = 0;
    int val = 0;
    V(vKernel->CreateVISAImmediate(globalOffsetOpnd, &val, ISA_TYPE_UD));

    V(vKernel->AppendVISASurfAccessGather4Scatter4ScaledInst(
        ISA_GATHER4_SCALED,
        predOpnd,
        GetAluEMask(dst),
        visaExecSize(offset->IsUniform() ? lanesToSIMDMode(offset->GetNumberElement()) :
            m_encoderState.m_simdSize),
        ConvertChannelMaskToVisaType(BIT(nd) - 1),
        surfaceOpnd,
        globalOffsetOpnd,
        addressOpnd, dstOpnd));
}


void CEncoder::Gather4Scaled(CVariable *dst,
                       const ResourceDescriptor& resource,
                       CVariable *offset) {
    unsigned nd = dst->GetSize();
    if (dst->IsUniform())
    {
        if (nd > SIZE_GRF)
        {
            assert(false && "Unknown DstSize!");
            return;
        }
        nd = 1;
    }
    else
    {
        switch (m_encoderState.m_simdSize) {
        default: assert(false && "Unknown SIMD size!"); return;
        case SIMDMode::SIMD8:
            nd = nd / (SIZE_GRF * 1);
            break;
        case SIMDMode::SIMD16:
            nd = nd / (SIZE_GRF * 2);
            break;
        }
    }
    Gather4ScaledNd(dst, resource, offset, nd);
}

void CEncoder::Scatter4Scaled(CVariable *src,
                        const ResourceDescriptor& resource,
                        CVariable *offset) {
    unsigned nd = src->GetSize();
    if (src->IsUniform())
    {
        if (nd > SIZE_GRF)
        {
            assert(false && "Unknown SrcSize!");
            return;
        }
        nd = 1;
    }
    else
    {
        switch (m_encoderState.m_simdSize) {
        default: assert(false && "Unknown SIMD size!"); return;
        case SIMDMode::SIMD8:
            nd = nd / (SIZE_GRF * 1);
            break;
        case SIMDMode::SIMD16:
            nd = nd / (SIZE_GRF * 2);
            break;
        }
    }

    VISA_StateOpndHandle* surfaceOpnd = GetVISASurfaceOpnd(resource);
    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
    VISA_RawOpnd *addressOpnd = GetRawSource(offset);
    VISA_RawOpnd *srcOpnd = GetRawSource(src);

    VISA_VectorOpnd *globalOffsetOpnd = 0;
    int val = 0;
    V(vKernel->CreateVISAImmediate(globalOffsetOpnd, &val, ISA_TYPE_UD));

    V(vKernel->AppendVISASurfAccessGather4Scatter4ScaledInst(
        ISA_SCATTER4_SCALED,
        predOpnd,
        GetAluEMask(src),
        visaExecSize(offset->IsUniform() ? lanesToSIMDMode(offset->GetNumberElement()) :
            m_encoderState.m_simdSize),
        ConvertChannelMaskToVisaType(BIT(nd) - 1),
        surfaceOpnd,
        globalOffsetOpnd,
        addressOpnd, srcOpnd));
}

void CEncoder::Gather4A64(CVariable *dst, CVariable *offset) {
    assert(dst->GetElemSize() == 4 && "Gather4 must have 4-byte element");

    uint32_t dstOfstBytes = m_encoderState.m_dstOperand.subVar * SIZE_GRF + dst->GetAliasOffset();
    unsigned nd = dst->GetSize();
    switch (m_encoderState.m_simdSize) {
    default: assert(false && "Unknown SIMD size!"); return;
    case SIMDMode::SIMD8:
        nd = nd / (SIZE_GRF * 1);
        break;
    case SIMDMode::SIMD16:
        nd = nd / (SIZE_GRF * 2);
        break;
    }

    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
    VISA_RawOpnd *addressOpnd = GetRawSource(offset);
    VISA_RawOpnd *dstOpnd = GetRawDestination(dst);

    VISA_VectorOpnd *globalOffsetOpnd = 0;
    int val = 0;
    V(vKernel->CreateVISAImmediate(globalOffsetOpnd, &val, ISA_TYPE_UD));

    if (m_program->m_Platform->GetPlatformFamily() == IGFX_GEN8_CORE && m_encoderState.m_simdSize == SIMDMode::SIMD16)
    {
        // BDW A64 untyped does not support SIMD16, split it into 2 SIMD8
        Common_VISA_EMask_Ctrl execMask = GetAluEMask(offset);
        Common_ISA_Exec_Size fromExecSize = EXEC_SIZE_16;
        Common_ISA_Exec_Size toExecSize = EXEC_SIZE_8;

        if (nd == 1)
        {
            // No packing is needed.
            for (unsigned p = 0; p < 2; ++p)
            {
                addressOpnd = GetRawSource(offset, GetRawOpndSplitOffset(fromExecSize, toExecSize, p, offset));
                dstOpnd = GetRawDestination(dst, GetRawOpndSplitOffset(fromExecSize, toExecSize, p, dst));

                V(vKernel->AppendVISASvmGather4ScaledInst(
                    predOpnd,
                    SplitEMask(fromExecSize, toExecSize, p, execMask),
                    toExecSize,
                    ConvertChannelMaskToVisaType(BIT(nd) - 1),
                    globalOffsetOpnd,
                    addressOpnd, dstOpnd));
            }
        }
        else
        {
            // Packing the two SIMD8 data payload to form the SIMD16 data payload
            // by merging the two simd8 data payload.
            CVariable* V0, *V1;
            uint16_t newNumElems = (uint16_t) 8 * nd;
            V0 = m_program->GetNewVariable(newNumElems,
                                           dst->GetType(),
                                           dst->GetAlign(),
                                           dst->IsUniform());
            V1 = m_program->GetNewVariable(newNumElems,
                                           dst->GetType(),
                                           dst->GetAlign(),
                                           dst->IsUniform());

            for (unsigned p = 0; p < 2; ++p)
            {
                addressOpnd = GetRawSource(offset, GetRawOpndSplitOffset(fromExecSize, toExecSize, p, offset));
                dstOpnd = GetRawDestination(p == 0 ? V0 : V1);

                V(vKernel->AppendVISASvmGather4ScaledInst(
                    predOpnd,
                    SplitEMask(fromExecSize, toExecSize, p, execMask),
                    toExecSize,
                    ConvertChannelMaskToVisaType(BIT(nd) - 1),
                    globalOffsetOpnd,
                    addressOpnd, dstOpnd));
            }

            MergeMDP8To16(V0, V1, nd, dst, dstOfstBytes);
        }
        return;
    }

    V(vKernel->AppendVISASvmGather4ScaledInst(
        predOpnd,
        GetAluEMask(dst),
        visaExecSize(m_encoderState.m_simdSize),
        ConvertChannelMaskToVisaType(BIT(nd) - 1),
        globalOffsetOpnd,
        addressOpnd, dstOpnd));
}

void CEncoder::Scatter4A64(CVariable *src, CVariable *offset) {
    assert(src->GetElemSize() == 4 && "scatter4 must have 4-byte element");

    uint32_t srcOfstBytes = src->GetAliasOffset();
    unsigned nd = src->GetSize();
    switch (m_encoderState.m_simdSize) {
    default: assert(false && "Unknown SIMD size!"); return;
    case SIMDMode::SIMD8:
        nd = nd / (SIZE_GRF * 1);
        break;
    case SIMDMode::SIMD16:
        nd = nd / (SIZE_GRF * 2);
        break;
    }

    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
    VISA_RawOpnd *addressOpnd = GetRawSource(offset);
    VISA_RawOpnd *srcOpnd = GetRawSource(src);

    VISA_VectorOpnd *globalOffsetOpnd = 0;
    int val = 0;
    V(vKernel->CreateVISAImmediate(globalOffsetOpnd, &val, ISA_TYPE_UD));

    if (m_program->m_Platform->GetPlatformFamily() == IGFX_GEN8_CORE && m_encoderState.m_simdSize == SIMDMode::SIMD16)
    {
        // BDW A64 untyped does not support SIMD16, split it into 2 SIMD8
        Common_VISA_EMask_Ctrl execMask = GetAluEMask(src);
        Common_ISA_Exec_Size fromExecSize = EXEC_SIZE_16;
        Common_ISA_Exec_Size toExecSize = EXEC_SIZE_8;

        if (nd == 1)
        {
            // No need to do unpacking
            for (unsigned p = 0; p < 2; ++p)
            {
                addressOpnd = GetRawSource(offset, GetRawOpndSplitOffset(fromExecSize, toExecSize, p, offset));
                srcOpnd = GetRawSource(src, GetRawOpndSplitOffset(fromExecSize, toExecSize, p, src));

                V(vKernel->AppendVISASvmScatter4ScaledInst(
                    predOpnd,
                    SplitEMask(fromExecSize, toExecSize, p, execMask),
                    toExecSize,
                    ConvertChannelMaskToVisaType(BIT(nd) - 1),
                    globalOffsetOpnd,
                    addressOpnd, srcOpnd));
            }
        }
        else
        {
            // Unpacking is needed from the original SIMD16 data payload to form
            // two SIMD8 data payload by spliting the original simd16 data payload.
            CVariable *V0, *V1;
            uint16_t newNumElems = (uint16_t) 8 * nd;
            V0 = m_program->GetNewVariable(newNumElems,
                                           src->GetType(),
                                           src->GetAlign(),
                                           src->IsUniform());
            V1 = m_program->GetNewVariable(newNumElems,
                                           src->GetType(),
                                           src->GetAlign(),
                                           src->IsUniform());

            SplitMDP16To8(src, srcOfstBytes, nd, V0, V1);

            for (unsigned p = 0; p < 2; ++p)
            {
                addressOpnd = GetRawSource(offset, GetRawOpndSplitOffset(fromExecSize, toExecSize, p, offset));
                srcOpnd = GetRawSource(p == 0 ? V0 : V1);

                V(vKernel->AppendVISASvmScatter4ScaledInst(
                    predOpnd,
                    SplitEMask(fromExecSize, toExecSize, p, execMask),
                    toExecSize,
                    ConvertChannelMaskToVisaType(BIT(nd) - 1),
                    globalOffsetOpnd,
                    addressOpnd, srcOpnd));
            }
        }
        return;
    }

    V(vKernel->AppendVISASvmScatter4ScaledInst(
        predOpnd,
        GetAluEMask(src),
        visaExecSize(m_encoderState.m_simdSize),
        ConvertChannelMaskToVisaType(BIT(nd) - 1),
        globalOffsetOpnd,
        addressOpnd, srcOpnd));
}

void CEncoder::AtomicRawA64(AtomicOp atomic_op,
                            CVariable* dst,
                            CVariable* offset,
                            CVariable* src0,
                            CVariable* src1,
                            unsigned short bitwidth)
{
    // For cmpxchg, we have to change the order of arguments.
    if (atomic_op == EATOMIC_CMPXCHG) {
        std::swap(src0, src1);
    }

    VISAAtomicOps atomicOpcode = convertAtomicOpEnumToVisa(atomic_op);

    if (m_encoderState.m_simdSize == SIMDMode::SIMD16) {
        // Split SIMD16 atomic ops into two SIMD8 ones.
        Common_VISA_EMask_Ctrl execMask = ConvertMaskToVisaType(m_encoderState.m_mask, m_encoderState.m_noMask);
        Common_ISA_Exec_Size fromExecSize = visaExecSize(m_encoderState.m_simdSize);
        Common_ISA_Exec_Size toExecSize = SplitExecSize(fromExecSize, 2);

        for (unsigned thePart = 0; thePart != 2; ++thePart) {
            VISA_RawOpnd *dstOpnd  = GetRawDestination(dst, GetRawOpndSplitOffset(fromExecSize, toExecSize, thePart, dst));
            VISA_RawOpnd *addressOpnd = GetRawSource(offset, GetRawOpndSplitOffset(fromExecSize, toExecSize, thePart, offset));
            VISA_RawOpnd *src0Opnd = GetRawSource(src0, GetRawOpndSplitOffset(fromExecSize, toExecSize, thePart, src0));
            VISA_RawOpnd *src1Opnd = GetRawSource(src1, GetRawOpndSplitOffset(fromExecSize, toExecSize, thePart, src1));

            V(vKernel->AppendVISASvmAtomicInst(GetFlagOperand(m_encoderState.m_flag),
                                               SplitEMask(fromExecSize, toExecSize, thePart, execMask),
                                               toExecSize, atomicOpcode, bitwidth,
                                               addressOpnd, src0Opnd, src1Opnd, dstOpnd));
        }

        return;
    }

    VISA_RawOpnd *addressOpnd = GetRawSource(offset);
    VISA_RawOpnd *src0Opnd = GetRawSource(src0);
    VISA_RawOpnd *src1Opnd = GetRawSource(src1);
    VISA_RawOpnd *dstOpnd  = GetRawDestination(dst);

    V(vKernel->AppendVISASvmAtomicInst(GetFlagOperand(m_encoderState.m_flag),
                                       ConvertMaskToVisaType(m_encoderState.m_mask, m_encoderState.m_noMask),
                                       visaExecSize(m_encoderState.m_simdSize),
                                       atomicOpcode,
                                       bitwidth,
                                       addressOpnd,
                                       src0Opnd,
                                       src1Opnd,
                                       dstOpnd));
}

void CEncoder::Wait()
{
    V(vKernel->AppendVISAWaitInst(nullptr));
}

void CEncoder::SendVmeIme(CVariable* bindingTableIndex,
                            unsigned char streamMode,
                            unsigned char searchControlMode,
                            CVariable* uniInputVar,
                            CVariable* imeInputVar,
                            CVariable* ref0Var,
                            CVariable* ref1Var,
                            CVariable* costCenterVar,
                            CVariable* outputVar) {

    VISA_StateOpndHandle* surface = GetVISASurfaceOpnd(ESURFACE_NORMAL, bindingTableIndex);
    VISA_RawOpnd* uniInput = GetRawSource(uniInputVar);
    VISA_RawOpnd* imeInput = GetRawSource(imeInputVar);
    VISA_RawOpnd* ref0 = GetRawSource(ref0Var);
    VISA_RawOpnd* ref1 = GetRawSource(ref1Var);
    VISA_RawOpnd* costCenter = GetRawSource(costCenterVar);
    VISA_RawOpnd* output = GetRawDestination(outputVar);
    V(vKernel->AppendVISAMiscVME_IME(surface, streamMode, searchControlMode, uniInput, imeInput, ref0, ref1, costCenter, output));
}

void CEncoder::SendVmeFbr(CVariable* bindingTableIndex,
                            CVariable* uniInputVar,
                            CVariable* fbrInputVar,
                            CVariable* FBRMbModeVar,
                            CVariable* FBRSubMbShapeVar,
                            CVariable* FBRSubPredModeVar,
                            CVariable* outputVar) {
    VISA_StateOpndHandle *surface = GetVISASurfaceOpnd(ESURFACE_NORMAL, bindingTableIndex);
    VISA_RawOpnd *UNIInput = GetRawSource(uniInputVar);
    VISA_RawOpnd *FBRInput = GetRawSource(fbrInputVar);
    VISA_VectorOpnd* FBRMbMode = GetSourceOperand(FBRMbModeVar, m_encoderState.m_srcOperand[0]);
    VISA_VectorOpnd *FBRSubMbShape = GetSourceOperand(FBRSubMbShapeVar, m_encoderState.m_srcOperand[1]);
    VISA_VectorOpnd *FBRSubPredMode = GetSourceOperand(FBRSubPredModeVar, m_encoderState.m_srcOperand[2]);
    VISA_RawOpnd *output = GetRawDestination(outputVar);

    V(vKernel->AppendVISAMiscVME_FBR(surface, UNIInput, FBRInput, FBRMbMode, FBRSubMbShape, FBRSubPredMode, output));
}

void CEncoder::SendVmeSic(
    CVariable* bindingTableIndex,
    CVariable* uniInputVar,
    CVariable* sicInputVar,
    CVariable* outputVar )
{
    VISA_StateOpndHandle *surface = GetVISASurfaceOpnd(ESURFACE_NORMAL, bindingTableIndex);
    VISA_RawOpnd *UNIInput = GetRawSource(uniInputVar);
    VISA_RawOpnd *SICInput = GetRawSource(sicInputVar);
    VISA_RawOpnd *output = GetRawDestination(outputVar);

    V(vKernel->AppendVISAMiscVME_SIC(surface, UNIInput, SICInput, output));
}

void CEncoder::SendVideoAnalytic(
    llvm::GenIntrinsicInst* inst,
    CVariable* vaResult,
    CVariable* coords,
    CVariable* size,
    CVariable* srcImg,
    CVariable* sampler )
{
    VISA_RawOpnd *vaOutput = GetRawDestination( vaResult );

    SModifier mod0 = m_encoderState.m_srcOperand[0];
    SModifier mod1 = m_encoderState.m_srcOperand[1];

    mod0.specialRegion = mod1.specialRegion = true;
    mod0.region[0] = mod1.region[0] = 0;
    mod0.region[1] = mod1.region[1] = 1;
    mod0.region[2] = mod1.region[2] = 0;
    mod0.subReg = 0;
    mod0.subVar = 0;

    if( coords->IsUniform() )
    {
        mod1.subReg = 1;
        mod1.subVar = 0;
    }
    else
    {
        mod1.subReg = 0;
        mod1.subVar = 2;
    }

    VISA_VectorOpnd *uOffset = GetSourceOperand( coords, mod0 );
    VISA_VectorOpnd *vOffset = GetSourceOperand( coords, mod1 );

    if( size && size->IsUniform() )
    {
        mod1.subReg = 1;
        mod1.subVar = 0;
    }
    else
    {
        mod1.subReg = 0;
        mod1.subVar = 2;
    }

    VISA_VectorOpnd *wSize = ( size ? GetSourceOperand( size, mod0 ) : NULL );
    VISA_VectorOpnd *hSize = ( size ? GetSourceOperand( size, mod1 ) : NULL );

    // So far we support only one VA function per kernel, and other sample
    // messages are not supported when there is VA function within the kernel.
    // So, for now it should be fine to always use bti 0 for VA functions.
    DWORD btiIndex = 0;
    DWORD mmfMode = 0;

    VISA_StateOpndHandle *surfaceOpnd = GetBTIOperand( btiIndex );
    VISA_StateOpndHandle* samplerHnd = GetSamplerOperand( sampler );
    VISA_VectorOpnd* mmModeOpnd = NULL;

    EDMode erodeDilateMode = CM_DILATE;
    EDExecMode execMode = CM_ED_64x4;
    bool isBigKernel = true;

    if( m_program->m_Platform->GetPlatformFamily() == IGFX_GEN8_CORE )
    {
        isBigKernel = false;
    }

    switch( inst->getIntrinsicID() )
    {
    case GenISAIntrinsic::GenISA_vaErode:
        erodeDilateMode = CM_ERODE;
    case GenISAIntrinsic::GenISA_vaDilate:
        V( vKernel->AppendVISAVAErodeDilate( erodeDilateMode, samplerHnd, surfaceOpnd, uOffset, vOffset, execMode, vaOutput ) );
        break;
    case GenISAIntrinsic::GenISA_vaMinMaxFilter:
        V( vKernel->CreateVISAImmediate( mmModeOpnd, &mmfMode, ISA_TYPE_UD ) );
        V( vKernel->AppendVISAVAMinMaxFilter( samplerHnd, surfaceOpnd, uOffset, vOffset, CM_16_FULL, CM_MMF_16x4, mmModeOpnd, vaOutput ) );
        break;
    case GenISAIntrinsic::GenISA_vaConvolveGRF_16x1:
        V(vKernel->AppendVISAVAConvolve(samplerHnd, surfaceOpnd, uOffset, vOffset, CM_CONV_16x1, isBigKernel, vaOutput));
        break;
    case GenISAIntrinsic::GenISA_vaConvolve:
    case GenISAIntrinsic::GenISA_vaConvolveGRF_16x4:
        V(vKernel->AppendVISAVAConvolve(samplerHnd, surfaceOpnd, uOffset, vOffset, CM_CONV_16x4, isBigKernel, vaOutput));
        break;
    case GenISAIntrinsic::GenISA_vaMinMax:
        V( vKernel->CreateVISAImmediate( mmModeOpnd, &mmfMode, ISA_TYPE_UD ) );
        V( vKernel->AppendVISAVAMinMax( surfaceOpnd, uOffset, vOffset, mmModeOpnd, vaOutput ) );
        break;
    case GenISAIntrinsic::GenISA_vaCentroid:
        V( vKernel->AppendVISAVACentroid( surfaceOpnd, uOffset, vOffset, wSize, vaOutput ) );
        break;
    case GenISAIntrinsic::GenISA_vaBoolCentroid:
    case GenISAIntrinsic::GenISA_vaBoolSum:
        V( vKernel->AppendVISAVABooleanCentroid( surfaceOpnd, uOffset, vOffset, wSize, hSize, vaOutput ) );
        break;
    default:
        assert( 0 && "Trying to emit unrecognized video analytic instruction (listed above)" );
        break;
    };
}

void CEncoder::SetVISAWaTable(WA_TABLE const& waTable)
{
    //Copy from driver WA table to VISA WA table

    m_WaTable.WaHeaderRequiredOnSimd16Sample16bit = waTable.WaHeaderRequiredOnSimd16Sample16bit;
    m_WaTable.WaSendsSrc1SizeLimitWhenEOT = waTable.WaSendsSrc1SizeLimitWhenEOT;
    m_WaTable.WaDisallow64BitImmMov = waTable.WaDisallow64BitImmMov;
    m_WaTable.WaThreadSwitchAfterCall = waTable.WaThreadSwitchAfterCall;
    m_WaTable.WaSrc1ImmHfNotAllowed = waTable.WaSrc1ImmHfNotAllowed;
    m_WaTable.WaDstSubRegNumNotAllowedWithLowPrecPacked = waTable.WaDstSubRegNumNotAllowedWithLowPrecPacked;
    m_WaTable.WaDisableMixedModeLog = waTable.WaDisableMixedModeLog;
    m_WaTable.WaDisableMixedModeFdiv = waTable.WaDisableMixedModeFdiv;
    m_WaTable.WaDisableMixedModePow = waTable.WaDisableMixedModePow;
    m_WaTable.WaFloatMixedModeSelNotAllowedWithPackedDestination = waTable.WaFloatMixedModeSelNotAllowedWithPackedDestination;
    m_WaTable.WADisableWriteCommitForPageFault = waTable.WADisableWriteCommitForPageFault;
    //To be enabled after VISA changes the name for this to be consistent
    //m_WaTable.WAInsertNOPBetweenMathPOWDIVAnd2RegInstr  = waTable.WAInsertNOPBetweenMathPOWDIVAnd2RegInstr;
    m_WaTable.WaClearArfDependenciesBeforeEot = waTable.WaClearArfDependenciesBeforeEot;
    m_WaTable.WaMixModeSelInstDstNotPacked = waTable.WaMixModeSelInstDstNotPacked;
    m_WaTable.WaDisableSendsPreemption = waTable.WaDisableSendsPreemption;
    m_WaTable.WaResetN0BeforeGatewayMessage = waTable.WaResetN0BeforeGatewayMessage;

    if (m_program->GetShaderType() != ShaderType::PIXEL_SHADER &&
        m_program->GetShaderType() != ShaderType::COMPUTE_SHADER &&
        m_program->GetShaderType() != ShaderType::OPENCL_SHADER )
    {
        m_WaTable.WaClearTDRRegBeforeEOTForNonPS = waTable.WaClearTDRRegBeforeEOTForNonPS;
    }

    if (IGC_IS_FLAG_DISABLED(ForceSendsSupportOnSKLA0))
    {
        m_WaTable.WaDisableSendsSrc0DstOverlap = waTable.WaDisableSendsSrc0DstOverlap;
    }


    TODO("Limit this C0 WA as required to only Compute , as it causes hangs in some  3D Workloads");
    if (m_program->GetShaderType() == ShaderType::COMPUTE_SHADER || m_program->GetShaderType() == ShaderType::OPENCL_SHADER)
    {
        if (IGC_IS_FLAG_DISABLED(DisableWaSendSEnableIndirectMsgDesc))
        {
            m_WaTable.WaSendSEnableIndirectMsgDesc = waTable.WaSendSEnableIndirectMsgDesc;
        }
    }

    if (IGC_IS_FLAG_DISABLED(DisableWaDisableSIMD16On3SrcInstr))
    {
        m_WaTable.WaDisableSIMD16On3SrcInstr = waTable.WaDisableSIMD16On3SrcInstr;
    }

    // no send src/dst overlap when page fault is enabled
    m_WaTable.WaDisableSendSrcDstOverlap = m_program->m_Platform->WaDisableSendSrcDstOverlap();

    m_WaTable.WaNoSimd16TernarySrc0Imm = waTable.WaNoSimd16TernarySrc0Imm;
    m_WaTable.Wa_1406306137 = waTable.Wa_1406306137;
    m_WaTable.Wa_2201674230 = waTable.Wa_2201674230;
	m_WaTable.Wa_1406950495 = waTable.Wa_1406950495;
}

void CEncoder::GetRowAndColOffset(CVariable* var, unsigned int subVar, unsigned int subReg, unsigned char& rowOff, unsigned char& colOff)
{
    unsigned int varTypeSize = GetCISADataTypeSize(var->GetType());
    unsigned int offset = var->GetAliasOffset() + subVar * SIZE_GRF + subReg * varTypeSize;
    assert((offset%SIZE_GRF) % varTypeSize == 0 && "offset has to be aligned on element size");
    rowOff = int_cast<unsigned char>(offset / SIZE_GRF);
    assert(varTypeSize != 0);
    colOff = int_cast<unsigned char>((offset%SIZE_GRF) / varTypeSize);
}

void CEncoder::Loc(unsigned int line)
{
    V(vKernel->AppendVISAMiscLOC(line));
}

void CEncoder::File(std::string& s)
{
    V(vKernel->AppendVISAMiscFileInst(s.c_str()));
}

void CEncoder::Lifetime(VISAVarLifetime StartOrEnd, CVariable* dst)
{
    SModifier noMod; // Default is no mod.
    noMod.init();
    VISA_VectorOpnd* srcOpnd = GetSourceOperand(dst, noMod);
    V(vKernel->AppendVISALifetime(StartOrEnd, srcOpnd));
}

}
