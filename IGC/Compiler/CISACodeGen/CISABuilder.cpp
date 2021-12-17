/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/CISABuilder.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/PixelShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/ComputeShaderCodeGen.hpp"
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
#include <llvm/Support/Path.h>
#include <llvm/ADT/Statistic.h>
#include <iStdLib/utility.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <fstream>
#include "Probe/Assertion.h"

#if !defined(_WIN32)
#   define _strdup strdup
#endif

/***********************************************************************************
This file defines the CEncoder class which is used to generate CISA instructions
************************************************************************************/

// macro to check the result of VISA API calls
#define V(x) do { int result = (x); IGC_ASSERT_MESSAGE((0 == result), "call to VISA API failed"); } while(0)

static const unsigned  int g_cScratchSpaceMsglimit = (128 * 1024);
using namespace llvm;

#define DEBUG_TYPE "cisa-builder"

STATISTIC(SimdSize8, "Number of shader(s) with SIMD8");
STATISTIC(SimdSize16, "Number of shader(s) with SIMD16");
STATISTIC(SimdSize32, "Number of shader(s) with SIMD32");

namespace IGC
{
    inline VISA_Exec_Size visaExecSize(SIMDMode width)
    {
        switch (width)
        {
        case SIMDMode::SIMD1: return EXEC_SIZE_1;
        case SIMDMode::SIMD2: return EXEC_SIZE_2;
        case SIMDMode::SIMD4: return EXEC_SIZE_4;
        case SIMDMode::SIMD8: return EXEC_SIZE_8;
        case SIMDMode::SIMD16: return EXEC_SIZE_16;
        case SIMDMode::SIMD32: return EXEC_SIZE_32;
        case SIMDMode::UNKNOWN:
        default: IGC_ASSERT_MESSAGE(0, "unreachable"); break;
        }
        return EXEC_SIZE_ILLEGAL;
    }

    VISAAtomicOps convertAtomicOpEnumToVisa(AtomicOp op)
    {
        switch (op)
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
        case EATOMIC_FADD:
        case EATOMIC_FADD64:
            return ATOMIC_FADD;
        case EATOMIC_FSUB:
            return ATOMIC_FSUB;
        default:
            IGC_ASSERT_MESSAGE(0, "Atomic Op not implemented");
            return ATOMIC_AND;
        }
    }

    inline GATHER_SCATTER_ELEMENT_SIZE visaElementSize(unsigned int m_elt_size)
    {
        GATHER_SCATTER_ELEMENT_SIZE elementSize = GATHER_SCATTER_BYTE_UNDEF;
        if (m_elt_size == 1)
        {
            elementSize = GATHER_SCATTER_BYTE;
        }
        else if (m_elt_size == 2)
        {
            elementSize = GATHER_SCATTER_WORD;
        }
        else if (m_elt_size == 4)
        {
            elementSize = GATHER_SCATTER_DWORD;
        }
        else
        {
            IGC_ASSERT_MESSAGE(0, "unreachable");
        }
        return elementSize;
    }

    static inline VISA_SVM_Block_Type
        visaBlockType(unsigned elemSize) {
        switch (elemSize) {
        case 8:  return SVM_BLOCK_TYPE_BYTE;
        case 32: return SVM_BLOCK_TYPE_DWORD;
        case 64: return SVM_BLOCK_TYPE_QWORD;
        }

        IGC_ASSERT_MESSAGE(0, "Unknown block/element size. Expect 8-/32-/64-bit only!");
        return static_cast<VISA_SVM_Block_Type>(~0U);
    }

    static inline VISA_SVM_Block_Num
        visaBlockNum(unsigned numElems) {
        switch (numElems) {
        case 1: return SVM_BLOCK_NUM_1;
        case 2: return SVM_BLOCK_NUM_2;
        case 4: return SVM_BLOCK_NUM_4;
        case 8: return SVM_BLOCK_NUM_8;
        }

        IGC_ASSERT_MESSAGE(0, "Unknown number of blocks/elements. Expect 1, 2, 4, or 8 only!");
        return static_cast<VISA_SVM_Block_Num>(~0U);
    }

    constexpr unsigned visaNumLanes(VISA_Exec_Size execSize)
    {
        unsigned lanes = 0;
        switch (execSize)
        {
        case EXEC_SIZE_1:  lanes = 1; break;
        case EXEC_SIZE_2:  lanes = 2; break;
        case EXEC_SIZE_4:  lanes = 4; break;
        case EXEC_SIZE_8:  lanes = 8; break;
        case EXEC_SIZE_16: lanes = 16; break;
        case EXEC_SIZE_32: lanes = 32; break;
        default: IGC_ASSERT(0); break;
        }
        return lanes;
    }

    // Take certain attributes of either src or dst instruction operand and return the size
    // of the associated grf region, accessed during instruction's execution, in bytes.
    // If aligned==true, the size includes length of data block starting at the beginning of grf
    // and ending at the subReg; this is useful to check if the region crosses 2 grf boundary.
    // If special region attribute is not set, the regioning is <1; 1, 0> for src and <1> for dst.
    // Note that the assertions may hit in certain cases, which should be handled separately,
    // like uniform vars with operand with special region set.
    constexpr unsigned GrfRegionSize(VISA_Exec_Size execSize, unsigned elementSize,
        const SModifier& mod, bool isSource, bool aligned = true)
    {
        constexpr unsigned grfSize = 32; // in bytes
        // If subReg is big enough to cross grf boundary, adjust it.
        const unsigned base = (mod.subReg * elementSize) % grfSize;
        unsigned lastInRegion = aligned ? base : 0;
        if (isSource)
        {
            // Formula based on algorithm provided in the spec (see Region Parameters)
            const unsigned vstride = mod.specialRegion ? mod.region[0] : 1;
            const unsigned width = mod.specialRegion ? mod.region[1] : 1;
            const unsigned hstride = mod.specialRegion ? mod.region[2] : 0;
            if (0 == width)
            {
                return unsigned(-1);
            }
            const unsigned height = visaNumLanes(execSize) / width;
            if (0 == height)
            {
                return unsigned(-1);
            }
            lastInRegion += (height - 1) * vstride * elementSize +
                (width - 1) * hstride * elementSize;
        }
        else
        {
            const unsigned hstride = mod.specialRegion ? mod.region[2] : 1;
            lastInRegion += (visaNumLanes(execSize) - 1) * hstride * elementSize;
        }
        return lastInRegion + elementSize;
    };
    // Compile-time ULTs for GrfRegionSize()
    static_assert(GrfRegionSize(EXEC_SIZE_16, 4, SModifier{}, false) == 64 &&
        GrfRegionSize(EXEC_SIZE_16, 4, SModifier{ 16, {}, {0,0,2}, {}, {}, true }, false) == 124 &&
        GrfRegionSize(EXEC_SIZE_16, 4, SModifier{ 15, {}, {0,0,2}, {}, {}, true }, false) == 124 + 7 * 4 &&
        GrfRegionSize(EXEC_SIZE_8, 8, SModifier{ 1, {}, {0,0,2}, {}, {}, true }, false) == 128,
        "GrfRegionSize compile-time test failed - dst.");
    static_assert(GrfRegionSize(EXEC_SIZE_16, 4, SModifier{}, true) == 64 &&
        GrfRegionSize(EXEC_SIZE_16, 4, SModifier{ {}, {}, {4,4,0}, {}, {}, true }, true) == 52 &&
        GrfRegionSize(EXEC_SIZE_8, 8, SModifier{ 8, {}, {2,1,0}, {}, {}, true }, true) == 120 &&
        GrfRegionSize(EXEC_SIZE_8, 8, SModifier{ 10, {}, {2,1,0}, {}, {}, true }, true) == 120 + 2 * 8,
        "GrfRegionSize compile-time test failed - src.");

    // split a SIMD16 variable into two SIMD8 while satisfying vISA's raw operand alignment
    // return a tuple representing the vISA raw operand (var + offset) after split
    std::tuple<CVariable*, uint32_t> CEncoder::splitRawOperand(CVariable* var, bool isFirstHalf,
        VISA_EMask_Ctrl execMask)
    {

        if (!var || var->IsUniform() || isFirstHalf)
        {
            // simply return the original variable
            return std::make_tuple(var, 0);
        }

        uint32_t offset = 8 * var->GetElemSize();
        if ((offset % getGRFSize()) == 0)
        {
            return std::make_tuple(var, offset);
        }
        else
        {
            // create a copy to make the CVariable aligned
            auto tmpVar = m_program->GetNewVariable(
                8, var->GetType(), CVariable::getAlignment(getGRFSize()),
                CName::NONE);
            SModifier mod;
            mod.init();
            auto dstOpnd = GetDestinationOperand(tmpVar, mod);
            mod.subReg = 8;
            auto srcOpnd = GetSourceOperand(var, mod);

            V(vKernel->AppendVISADataMovementInst(
                ISA_MOV, nullptr, false,
                SplitEMask(EXEC_SIZE_16, EXEC_SIZE_8, 1, execMask),
                EXEC_SIZE_8, dstOpnd, srcOpnd));

            return std::make_tuple(tmpVar, 0);
        }
    }

    unsigned  CEncoder::GetRawOpndSplitOffset(
        VISA_Exec_Size fromExecSize,
        VISA_Exec_Size toExecSize,
        unsigned thePart, CVariable* var) const
    {
        if (!var || var->IsUniform())
            return 0;

        IGC_ASSERT_MESSAGE(fromExecSize == EXEC_SIZE_16, "Only support splitting from exec-size 16 to exec-size 8");
        IGC_ASSERT_MESSAGE(toExecSize == EXEC_SIZE_8, "Only support splitting from exec-size 16 to exec-size 8");
        IGC_ASSERT_MESSAGE((thePart == 0) || (thePart == 1), "Splitting from exec-size-16 to exec-size-8 only breaks into 2 parts");

        unsigned elemSize = var->GetElemSize();

        switch (elemSize)
        {
        case 4:
            return thePart * getGRFSize() * 1;
        case 8:
            return thePart * getGRFSize() * 2;
        }

        IGC_ASSERT_MESSAGE(0, "Unknown data type to split!");
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
            return (uint32_t)-1;
        }
        else
        {
            return (uint16_t)m_bitmask;
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

        if (m_nestLevelForcedNoMaskRegion > 0) {
          m_encoderState.m_noMask = true;
        }
    }

    CEncoder::CEncoder()
    {
        m_program = nullptr;
        vbuilder = nullptr;
        vAsmTextBuilder = nullptr;
    }

    CEncoder::~CEncoder()
    {
    }

    uint32_t CEncoder::getGRFSize() const { return m_program->getGRFSize(); }


    std::string CEncoder::GetShaderName() {
        return IGC::Debug::GetDumpNameObj(m_program, "").str();
    }

    void CEncoder::SetProgram(CShader* program)
    {
        m_program = program;
        Init();
    }

    void CEncoder::SubroutineCall(CVariable* flag, llvm::Function* F)
    {
        VISA_LabelOpnd* visaLabel = GetFuncLabel(F);
        m_encoderState.m_flag.var = flag;
        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
        // control flow instructions cannot be broken down into lower SIMD
        VISA_EMask_Ctrl emask = m_encoderState.m_noMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1;
        VISA_Exec_Size execSize = visaExecSize(m_program->m_dispatchSize);
        if (F->hasFnAttribute("KMPLOCK"))
        {
            emask = vISA_EMASK_M1_NM;
            execSize = EXEC_SIZE_1;
        }
        V(vKernel->AppendVISACFCallInst(predOpnd, emask, execSize, visaLabel));
    }

    void CEncoder::StackCall(CVariable* flag, llvm::Function* F, unsigned char argSize, unsigned char retSize)
    {

        m_encoderState.m_flag.var = flag;
        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
        // control flow instructions cannot be broken down into lower SIMD
        VISA_EMask_Ctrl emask = m_encoderState.m_noMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1;
        VISA_Exec_Size execSize = visaExecSize(m_program->m_dispatchSize);
        V(vKernel->AppendVISACFFunctionCallInst(predOpnd, emask, execSize, F->getName().data(), argSize, retSize));
    }

    void CEncoder::IndirectStackCall(CVariable* flag, CVariable* funcPtr, unsigned char argSize, unsigned char retSize)
    {
        m_encoderState.m_flag.var = flag;
        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
        // control flow instructions cannot be broken down into lower SIMD
        VISA_EMask_Ctrl emask = m_encoderState.m_noMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1;
        VISA_Exec_Size execSize = visaExecSize(m_program->m_dispatchSize);
        VISA_VectorOpnd* funcAddrOpnd = GetSourceOperandNoModifier(funcPtr);
        V(vKernel->AppendVISACFIndirectFuncCallInst(predOpnd, emask, execSize, funcAddrOpnd, argSize, retSize));
    }

    void CEncoder::SubroutineRet(CVariable* flag, llvm::Function* F)
    {
        m_encoderState.m_flag.var = flag;
        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
        // control flow instructions cannot be broken down into lower SIMD
        VISA_EMask_Ctrl emask = m_encoderState.m_noMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1;
        VISA_Exec_Size execSize = visaExecSize(m_program->m_dispatchSize);
        if (F->hasFnAttribute("KMPLOCK"))
        {
            emask = vISA_EMASK_M1_NM;
            execSize = EXEC_SIZE_1;
        }
        V(vKernel->AppendVISACFRetInst(predOpnd, emask, execSize));
    }

    void CEncoder::StackRet(CVariable* flag)
    {
        m_encoderState.m_flag.var = flag;
        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
        // control flow instructions cannot be broken down into lower SIMD
        VISA_EMask_Ctrl emask = m_encoderState.m_noMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1;
        VISA_Exec_Size execSize = visaExecSize(m_program->m_dispatchSize);
        V(vKernel->AppendVISACFFunctionRetInst(predOpnd, emask, execSize));
    }

    void CEncoder::Jump(CVariable* flag, uint label)
    {
        VISA_LabelOpnd* visaLabel = GetLabel(label);
        m_encoderState.m_flag.var = flag;
        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
        // control flow instructions cannot be broken down into lower SIMD
        VISA_EMask_Ctrl emask = m_encoderState.m_noMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1;
        VISA_Exec_Size execSize = visaExecSize(m_program->m_dispatchSize);

        // visa and igc agreement.
        //    goto (1) is used to tell visa the goto is uniform.
        // Goto(1) is generated if
        //   1. jump is unconditional, or
        //   2. jump is uniform (thread uniform or above) and no EU fusion, or
        //   3. jump is either workgroup or global uniform under EU fusion
        //      (it is temporarily under key control for ease of debugging)
        if (flag == nullptr ||
            (!m_program->m_Platform->hasFusedEU() && flag->IsUniform()) ||
            (IGC_IS_FLAG_ENABLED(EnableWorkGroupUniformGoto) &&
             m_program->m_Platform->hasFusedEU() && flag->IsWorkGroupOrGlobalUniform()))
        {
            execSize = EXEC_SIZE_1;
        }
        V(vKernel->AppendVISACFGotoInst(predOpnd, emask, execSize, visaLabel));
    }

    void CEncoder::Label(uint label)
    {
        VISA_LabelOpnd* visaLabel = GetLabel(label);
        V(vKernel->AppendVISACFLabelInst(visaLabel));
    }

    uint CEncoder::GetNewLabelID(const CName &name)
    {
        uint id = labelMap.size();
        labelMap.push_back(nullptr);
        labelNameMap.push_back(
            CreateVisaLabelName(llvm::StringRef(name.getCString())));
        return id;
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

        // Fix types for dword atomics
        VISA_Type type = ISA_TYPE_UD;
        if (atomic_op == EATOMIC_IMAX || atomic_op == EATOMIC_IMIN)
        {
            type = ISA_TYPE_D;
        }
        else if (atomic_op == EATOMIC_FMAX ||
            atomic_op == EATOMIC_FMIN ||
            atomic_op == EATOMIC_FADD ||
            atomic_op == EATOMIC_FSUB ||
            atomic_op == EATOMIC_FCMPWR)
        {
            type = ISA_TYPE_F;
        }
        if (src0 && src0->GetType() != type)
            src0 = m_program->BitCast(src0, type);
        if (src1 && src1->GetType() != type)
            src1 = m_program->BitCast(src1, type);
        if (dst && dst->GetType() != type)
            dst = m_program->BitCast(dst, type);
        if (elem_offset->GetType() != ISA_TYPE_UD)
            elem_offset = m_program->BitCast(elem_offset, ISA_TYPE_UD);

        IGC_ASSERT_MESSAGE(nullptr == m_encoderState.m_flag.var, "not supported predicate");

        VISA_StateOpndHandle* pSurfStateOpndHandle = GetVISASurfaceOpnd(resource);
        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
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
        if (ESURFACE_STATELESS == resource.m_surfaceType)
        {
            this->m_program->IncStatelessWritesCount();
        }
    }

    void CEncoder::Cmp(e_predicate p, CVariable* dst, CVariable* src0, CVariable* src1)
    {
        VISA_Cond_Mod subOp = ConvertCondModToVisaType(p);

        bool flagDst = 0;
        if (dst->GetType() == ISA_TYPE_BOOL)
        {
            flagDst = true;
        }

        VISA_VectorOpnd* opnd0 = GetSourceOperand(src0, m_encoderState.m_srcOperand[0]);
        VISA_VectorOpnd* opnd1 = GetSourceOperand(src1, m_encoderState.m_srcOperand[1]);

        if (flagDst)
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

    void CEncoder::Select(CVariable* flag, CVariable* dst, CVariable* src0, CVariable* src1)
    {
        m_encoderState.m_flag.var = flag;

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

    void CEncoder::PredAdd(CVariable* flag, CVariable* dst, CVariable* src0, CVariable* src1)
    {
        m_encoderState.m_flag.var = flag;

        Arithmetic(ISA_ADD, dst, src0, src1);
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
        IGC_ASSERT(srcNum < 4);
        m_encoderState.m_srcOperand[srcNum].subVar = int_cast<uint8_t>(subVar);
    }

    void CEncoder::SetSrcSubReg(uint srcNum, uint subReg)
    {
        IGC_ASSERT(srcNum < 4);
        m_encoderState.m_srcOperand[srcNum].subReg = int_cast<uint16_t>(subReg);
    }

    void CEncoder::SetDstModifier(e_modifier mod)
    {
        IGC_ASSERT((mod == EMOD_SAT) || (mod == EMOD_NONE));
        m_encoderState.m_dstOperand.mod = mod;
    }

    void CEncoder::SetSrcModifier(uint srcNum, e_modifier mod)
    {
        IGC_ASSERT(mod != EMOD_SAT);
        IGC_ASSERT(srcNum < 3);
        m_encoderState.m_srcOperand[srcNum].mod = mod;
    }

    void CEncoder::SetPredicate(CVariable* flag)
    {
        IGC_ASSERT((nullptr == flag) || (flag->GetVarType() == EVARTYPE_PREDICATE));
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
        if (modifier.sat)
        {
            SetDstModifier(EMOD_SAT);
        }
        if (modifier.flag)
        {
            SetPredicate(m_program->GetSymbol(modifier.flag->value));
            SetInversePredicate(modifier.invertFlag);
        }
    }

    void CEncoder::SetSrcRegion(uint srcNum, uint vStride, uint width, uint hStride, e_instance instance)
    {
        m_encoderState.m_srcOperand[srcNum].region[0] = int_cast<uint8_t>(vStride);
        m_encoderState.m_srcOperand[srcNum].region[1] = int_cast<uint8_t>(width);
        m_encoderState.m_srcOperand[srcNum].region[2] = int_cast<uint8_t>(hStride);
        m_encoderState.m_srcOperand[srcNum].instance = instance;
        m_encoderState.m_srcOperand[srcNum].specialRegion = true;
    }

    void CEncoder::SetDstRegion(uint hStride)
    {
        m_encoderState.m_dstOperand.region[2] = int_cast<uint8_t>(hStride);
        m_encoderState.m_dstOperand.specialRegion = (hStride != 1);
    }

    uint64_t GetSignBit(VISA_Type type)
    {
        switch (type)
        {
        case ISA_TYPE_Q:
        case ISA_TYPE_DF:
            return 63;
        case ISA_TYPE_D:
        case ISA_TYPE_F:
            return 31;
        case ISA_TYPE_W:
        case ISA_TYPE_HF:
        case ISA_TYPE_BF:
            return 15;
        case ISA_TYPE_B:
            return 7;
        default:
            IGC_ASSERT_MESSAGE(0, "type doesn't support modifier");
            break;
        }
        return 63;
    }

    bool IsFloat(VISA_Type type)
    {
        return type == ISA_TYPE_DF || type == ISA_TYPE_F || type == ISA_TYPE_HF || type == ISA_TYPE_BF;
    }

    uint64_t CalculateImmediateValue(CVariable* var, e_modifier mod)
    {
        IGC_ASSERT(nullptr != var);
        uint64_t immediate = var->GetImmediateValue();
        IGC_ASSERT((mod == EMOD_ABS) || (mod == EMOD_NEG) || (mod == EMOD_NEGABS) || (mod == EMOD_NONE));
        // handle modifiers for immediates.
        // Change the sign bit for floats and do logic operations for integers
        if (IsFloat(var->GetType()))
        {
            if (mod == EMOD_ABS)
            {
                immediate &= ~((uint64_t)(1) << GetSignBit(var->GetType()));
            }
            else if (mod == EMOD_NEG)
            {
                immediate ^= (uint64_t)(1) << GetSignBit(var->GetType());
            }
            else if (mod == EMOD_NEGABS)
            {
                immediate |= ((uint64_t)(1) << GetSignBit(var->GetType()));
            }
        }
        else
        {
            if (mod == EMOD_ABS || mod == EMOD_NEGABS)
            {
                uint64_t mask = (immediate >> GetSignBit(var->GetType()))& (uint64_t)0x01;
                immediate = (immediate + mask) ^ mask;
            }
            if (mod == EMOD_NEG || mod == EMOD_NEGABS)
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
        if (var == nullptr)
        {
            return nullptr;
        }
        VISA_VectorOpnd* operand = nullptr;
        if (var->IsImmediate())
        {
            uint64_t immediate = CalculateImmediateValue(var, mod.mod);
            V(vKernel->CreateVISAImmediate(operand, &immediate, var->GetType()));
        }
        else
        {
            if (var->GetVarType() == EVARTYPE_GENERAL)
            {
                unsigned short vStride = 1;
                unsigned short width = 1;
                unsigned short hStride = 0;

                if (mod.specialRegion)
                {
                    vStride = int_cast<unsigned short>(mod.region[0]);
                    width = int_cast<unsigned short>(mod.region[1]);
                    hStride = int_cast<unsigned short>(mod.region[2]);
                }
                else if (var->IsUniform())
                {
                    //Scalar regioning
                    vStride = 0;
                    width = 1;
                    hStride = 0;
                }
                unsigned char rowOffset = 0;
                unsigned char colOffset = 0;
                GetRowAndColOffset(var, mod.subVar, mod.subReg, rowOffset, colOffset);
                V(vKernel->CreateVISASrcOperand(
                    operand,
                    GetVISAVariable(var, mod.instance),
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
                    unsigned short width = 8;
                    unsigned short hStride = 1;

                    //if vector is also uniform
                    if (var->IsVectorUniform())
                    {
                        vStride = 0;
                        width = 1;
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
        if (var->GetVarType() == EVARTYPE_GENERAL)
        {
            unsigned short hStride = 1;
            unsigned char rowOffset = 0;
            unsigned char colOffset = 0;
            GetRowAndColOffset(var, mod.subVar, mod.subReg, rowOffset, colOffset);
            if (mod.specialRegion)
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
            const unsigned short hStride = 1;
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
        if (flag.var == nullptr)
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

    VISA_Exec_Size CEncoder::GetAluExecSize(CVariable* dst) const
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

    VISA_EMask_Ctrl CEncoder::GetAluEMask(CVariable* dst)
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
        return (m_encoderState.m_dstOperand.mod == EMOD_SAT) ? true : false;
    }

    void CEncoder::MinMax(CISA_MIN_MAX_SUB_OPCODE subopcode, CVariable* dst, CVariable* src0, CVariable* src1)
    {
        IGC_ASSERT_MESSAGE(nullptr == m_encoderState.m_flag.var, "min/max doesn't support predication");

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
    bool CEncoder::NeedSplitting(CVariable* var, const SModifier& mod,
        unsigned& numParts, bool isSource) const
    {
        // If nothing is specified, don't split.
        if (!var)
        {
            return false;
        }

        // Only handle SIMD16 now! We assume all data movements in SIMD8 will honor
    // the region rules.
        VISA_Exec_Size simdSize = GetAluExecSize(var);
        const unsigned elemSize = var->GetElemSize();

        switch (simdSize)
        {
        case EXEC_SIZE_16:
            break;
        default:
        {
            // Checks for some rare cases that are not handled by the splitter, but should be detected and reported.
            // Example: mov (8|M0)    r4.0<1>:q     r31.0<2;1,0>:q
            unsigned maxBlockSize = getGRFSize() * 2; // size of 2 GRFs in bytes
            // For uniform variables (which implies simdSize==1) the emitter may set regions with width>1.
            // As it may happen in various places, we detect it here.
            IGC_ASSERT(var->IsUniform() || (GrfRegionSize(simdSize, elemSize, mod, isSource) <= maxBlockSize));
            return false;
        }
        }

        // Only general variables need splitting so far.
        if (var->GetVarType() != EVARTYPE_GENERAL)
        {
            return false;
        }

        // Only varying variable need splitting so far.
        // NOTE: uniform variable is assumed to take less than 2 GRF+.
        if (var->IsUniform())
        {
            return false;
        }

        // We assume there is no 2 GRF crossing when element size is smaller than
        // 4 bytes (or 32 bits), e.g. 16-bit WORD.
        if (elemSize < 4)
        {
            return false;
        }

        // If the data type has more than 4 bytes, i.e. 32 bits, it already crosses
        // 2+ GRFs by itself. There's no need to check further.
        if (elemSize > 4)
        {
            IGC_ASSERT_MESSAGE(8 == elemSize, "Only QWORD is supported so far");
            IGC_ASSERT_MESSAGE(isSource || !mod.specialRegion,
                "It's expected that there's no special region associated with QWORD type destination!");
            if (isSource && mod.specialRegion)
            {
                if (mod.region[1] == 1 && mod.region[0] == 0)
                {
                    // src region is <0;1,x>, can't cross 2 GRF.  No need to split.
                    return false;
                }
                IGC_ASSERT_MESSAGE(0, "Unhandled special source region on QWORD type!");
            }

            numParts = std::max(numParts, 2U);
            return true;

        }


        // For 32-bit data types, without special region, they won't cross 2+ GRFs.
        if (!mod.specialRegion)
        {
            return false;
        }

        // Check regioning.
        if (isSource)
        {
            // FIXME: Need better support for region with non-1 width.
            if (mod.region[1] != 1)
            {
                return false;
            }

            if (mod.region[0] < 2)
            {
                return false;
            }

            // For src with width set to 1, region with > 1 vstride needs
            // splitting.
            numParts = std::max(numParts, unsigned(mod.region[0]));
            return true;
        }

        if (mod.region[2] < 2)
        {
            return false;
        }

        // For dst, region with > 1 hstride needs splitting.
        numParts = std::max(numParts, unsigned(mod.region[2]));
        return true;
    }

    // SplitVariable - Split the variable to prevent accessing 2+ GRFs.
    SModifier CEncoder::SplitVariable(VISA_Exec_Size fromExecSize,
        VISA_Exec_Size toExecSize,
        unsigned thePart,
        CVariable* var, const SModifier& mod,
        bool isSource) const {
        // Splitting uniform or source scalar variables is unnecessary!
        bool isAddrVar = var && var->GetVarType() == EVARTYPE_ADDRESS;
        if (!var || (var->IsUniform() && (!isAddrVar || var->IsVectorUniform())) ||
            (isSource && mod.specialRegion &&
                mod.region[1] == 1 && mod.region[0] == 0 && mod.region[2] == 0))
            return mod;

        IGC_ASSERT_MESSAGE(((fromExecSize == EXEC_SIZE_16) && (toExecSize == EXEC_SIZE_8)) || ((fromExecSize == EXEC_SIZE_32) && (toExecSize == EXEC_SIZE_16)),
            "Only support splitting from exec-size 16 to exec-size 8, or 32 to 16!");
        IGC_ASSERT_MESSAGE((thePart == 0) || (thePart == 1),
            "Splitting from exec-size-16 to exec-size-8 only breaks into 2 parts!");

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
            newMod.subReg += thePart * visaNumLanes(toExecSize);
            return newMod;
        }

        if (!mod.specialRegion) {
            // Without special regioning, split the given variable based on type.
            switch (elemSize) {
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
            default:
                IGC_ASSERT_MESSAGE(0, "Unknown data type to split!");
                break;
            }
            return newMod;
        }

        unsigned theStride = 0;
        if (isSource) {
            IGC_ASSERT_MESSAGE((mod.region[1] == 1),
                "Don't know how to split region with non-1 width!");
            theStride = mod.region[0];
        }
        else {
            theStride = mod.region[2];
        }

        switch (elemSize) {
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
        default:
            IGC_ASSERT_MESSAGE(0, "Unknown data type to split!");
            break;
        }

        return newMod;
    }

    VISA_Exec_Size  CEncoder::SplitExecSize(VISA_Exec_Size fromExecSize, unsigned numParts) const
    {
        IGC_ASSERT_MESSAGE(2 == numParts, "Only know splitting SIMD16 into SIMD8!");

        switch (fromExecSize) {
        default:
            break;
        case EXEC_SIZE_32:
            return EXEC_SIZE_16;
        case EXEC_SIZE_16:
            return EXEC_SIZE_8;
        }
        IGC_ASSERT_MESSAGE(0, "Unknown execution size to be split!");
        return static_cast<VISA_Exec_Size>(~0);
    }

    VISA_EMask_Ctrl
        CEncoder::SplitEMask(VISA_Exec_Size fromExecSize,
            VISA_Exec_Size toExecSize,
            unsigned thePart, VISA_EMask_Ctrl execMask) const {
        IGC_ASSERT_MESSAGE(((fromExecSize == EXEC_SIZE_16) && (toExecSize == EXEC_SIZE_8)) || ((fromExecSize == EXEC_SIZE_32) && (toExecSize == EXEC_SIZE_16)),
            "Only support splitting from exec-size 16 to exec-size 8, or from 32 to 16!");
        IGC_ASSERT_MESSAGE((thePart == 0) || (thePart == 1),
            "Splitting from exec-size-16 to exec-size-8 only breaks into 2 parts!");

        // FIXME: Better to generate a table!

        switch (fromExecSize) {
        default:
            break;
        case EXEC_SIZE_32:
            switch (toExecSize) {
            default:
                break;
            case EXEC_SIZE_16:
                switch (execMask) {
                default:
                    break;
                case vISA_EMASK_M1:     return thePart ? vISA_EMASK_M5 : vISA_EMASK_M1;
                case vISA_EMASK_M1_NM:  return thePart ? vISA_EMASK_M5_NM : vISA_EMASK_M1_NM;
                case vISA_EMASK_M3:     return thePart ? vISA_EMASK_M7 : vISA_EMASK_M3;
                case vISA_EMASK_M3_NM:  return thePart ? vISA_EMASK_M7_NM : vISA_EMASK_M3_NM;
                case vISA_EMASK_M5:     return thePart ? vISA_EMASK_M1 : vISA_EMASK_M5;
                case vISA_EMASK_M5_NM:  return thePart ? vISA_EMASK_M1_NM : vISA_EMASK_M5_NM;
                case vISA_EMASK_M7:     return thePart ? vISA_EMASK_M3 : vISA_EMASK_M7;
                case vISA_EMASK_M7_NM:  return thePart ? vISA_EMASK_M3_NM : vISA_EMASK_M7_NM;
                }
                break;
            }
            break;

        case EXEC_SIZE_16:
            switch (toExecSize) {
            default:
                break;
            case EXEC_SIZE_8:
                switch (execMask) {
                default:
                    break;
                case vISA_EMASK_M1:     return thePart ? vISA_EMASK_M3 : vISA_EMASK_M1;
                case vISA_EMASK_M1_NM:  return thePart ? vISA_EMASK_M3_NM : vISA_EMASK_M1_NM;
                case vISA_EMASK_M5:     return thePart ? vISA_EMASK_M7 : vISA_EMASK_M5;
                case vISA_EMASK_M5_NM:  return thePart ? vISA_EMASK_M7_NM : vISA_EMASK_M5_NM;
                }
                break;
            }
            break;
        }
        IGC_ASSERT_MESSAGE(0, "Unknown execution mask to be split into low part!");
        return static_cast<VISA_EMask_Ctrl>(~0);
    }

    // Splitting SIMD16 Message Data Payload (MDP at offset = MDPOfst) for A64
    // scatter/untyped write messages to two SIMD8 MDPs (V0 and V1).
    void CEncoder::SplitPayloadToLowerSIMD(CVariable* MDP, uint32_t MDPOfst, uint32_t NumBlks, CVariable* V0, CVariable* V1, uint32_t fromSize)
    {
        IGC_ASSERT(nullptr != MDP);
        IGC_ASSERT(nullptr != V0);
        IGC_ASSERT(nullptr != V1);

        VISA_GenVar* GV = GetVISAVariable(MDP);
        VISA_GenVar* v0GV = GetVISAVariable(V0);
        VISA_GenVar* v1GV = GetVISAVariable(V1);
        VISA_VectorOpnd* movDst0 = nullptr;
        VISA_VectorOpnd* movDst1 = nullptr;
        VISA_VectorOpnd* srcOpnd = nullptr;
        const uint32_t toSize = fromSize / 2;
        const VISA_Exec_Size fromESize = visaExecSize(lanesToSIMDMode(fromSize));
        const VISA_Exec_Size toESize = visaExecSize(lanesToSIMDMode(toSize));
        const uint32_t eltBytes = MDP->GetElemSize();

        IGC_ASSERT_MESSAGE(V0->GetElemSize() == eltBytes, "Element size should be the same among SIMD16 MDP and SIMD8 MDP.");
        IGC_ASSERT_MESSAGE(V1->GetElemSize() == eltBytes, "Element size should be the same among SIMD16 MDP and SIMD8 MDP.");

        // Number of elements per GRF

        if (eltBytes > 0)
        {
            uint32_t GRFElts = getGRFSize() / eltBytes;

            if (GRFElts > 0)
            {
                VISA_EMask_Ctrl execNM = ConvertMaskToVisaType(m_encoderState.m_mask, m_encoderState.m_noMask);
                uint32_t MDPStart = MDPOfst / eltBytes;
                for (uint32_t i = 0; i < NumBlks; ++i)
                {
                    uint32_t dstOfst = i * toSize;
                    uint32_t srcOfst = i * fromSize + MDPStart;
                    V(vKernel->CreateVISADstOperand(movDst0, v0GV, 1, dstOfst / GRFElts, dstOfst % GRFElts));
                    V(vKernel->CreateVISADstOperand(movDst1, v1GV, 1, dstOfst / GRFElts, dstOfst % GRFElts));

                    V(vKernel->CreateVISASrcOperand(srcOpnd, GV, MODIFIER_NONE,
                        1, 1, 0, srcOfst / GRFElts, srcOfst % GRFElts));

                    V(vKernel->AppendVISADataMovementInst(
                        ISA_MOV, nullptr, false,
                        SplitEMask(fromESize, toESize, 0, execNM),
                        toESize, movDst0, srcOpnd));

                    srcOfst += toSize;
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
    void CEncoder::MergePayloadToHigherSIMD(CVariable* V0, CVariable* V1, uint32_t NumBlks, CVariable* MDP, uint32_t MDPOfst, uint32_t toSize)
    {
        VISA_GenVar* GV = GetVISAVariable(MDP);
        VISA_GenVar* v0GV = GetVISAVariable(V0);
        VISA_GenVar* v1GV = GetVISAVariable(V1);
        VISA_VectorOpnd* movDst = nullptr;
        VISA_VectorOpnd* movSrc0 = nullptr;
        VISA_VectorOpnd* movSrc1 = nullptr;
        const uint32_t fromSize = toSize / 2;
        const VISA_Exec_Size fromESize = visaExecSize(lanesToSIMDMode(toSize));
        const VISA_Exec_Size toESize = visaExecSize(lanesToSIMDMode(fromSize));
        const uint32_t eltBytes = MDP->GetElemSize();
        IGC_ASSERT_MESSAGE(V0->GetElemSize() == eltBytes, "Element size should be the same among SIMD16 MDP and SIMD8 MDP!");
        IGC_ASSERT_MESSAGE(V1->GetElemSize() == eltBytes, "Element size should be the same among SIMD16 MDP and SIMD8 MDP!");

        if (eltBytes > 0)
        {
            // Number of elements per GRF
            const uint32_t GRFElts = getGRFSize() / eltBytes;

            if (GRFElts > 0)
            {
                VISA_EMask_Ctrl execNM = ConvertMaskToVisaType(m_encoderState.m_mask, m_encoderState.m_noMask);
                uint32_t MDPStart = MDPOfst / eltBytes;
                for (uint32_t i = 0; i < NumBlks; ++i)
                {
                    uint32_t dstOfst = i * toSize + MDPStart;
                    uint32_t srcOfst = i * fromSize;
                    V(vKernel->CreateVISADstOperand(movDst, GV, 1, dstOfst / GRFElts, dstOfst % GRFElts));
                    V(vKernel->CreateVISASrcOperand(movSrc0, v0GV, MODIFIER_NONE,
                        1, 1, 0, srcOfst / GRFElts, srcOfst % GRFElts));
                    V(vKernel->CreateVISASrcOperand(movSrc1, v1GV, MODIFIER_NONE,
                        1, 1, 0, srcOfst / GRFElts, srcOfst % GRFElts));

                    V(vKernel->AppendVISADataMovementInst(
                        ISA_MOV, nullptr, false,
                        SplitEMask(fromESize, toESize, 0, execNM),
                        toESize, movDst, movSrc0));

                    dstOfst += fromSize;
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
        EmulateVariable(CVariable* Var, SModifier Mod, bool IsHiPart, bool IsSource) {
        if (Mod.specialRegion) {
            if (IsSource) {
                Mod.region[0] *= 2;
                Mod.region[2] *= 2;
            }
            else
                Mod.region[2] *= 2;
        }
        else {
            if (IsSource) {
                if (!Var->IsUniform()) {
                    Mod.region[0] = 2;
                    Mod.region[1] = 1;
                    Mod.region[2] = 0;
                    Mod.specialRegion = true;
                }
            }
            else {
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
        if (opcode == ISA_SETP)
        {
            IGC_ASSERT(nullptr != dst);
            IGC_ASSERT(dst->GetVarType() == EVARTYPE_PREDICATE);
            V(vKernel->AppendVISASetP(
                GetAluEMask(dst),
                IsSecondHalf() ? GetAluExecSize(dst) : visaExecSize(m_program->m_dispatchSize),
                dst->visaPredVariable,
                GetSourceOperand(src, m_encoderState.m_srcOperand[0])));
        }
        else if (opcode == ISA_MOV && src->GetVarType() == EVARTYPE_PREDICATE)
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
                m_program->GetContext()->platform.hasNoInt64Inst() &&
                (Is64BitDst || Is64BitSrc);

            // If DP is not supported, need to split mov as well.
            if (IGC_IS_FLAG_ENABLED(ForceDPEmulation) ||
                m_program->GetContext()->platform.hasNoFP64Inst())
            {
                if (dstT == ISA_TYPE_DF && srcT == ISA_TYPE_DF)
                {
                    Need64BitEmu = true;
                    Is64BitDst = true;
                    Is64BitSrc = true;
                }
                else
                {
                    IGC_ASSERT_MESSAGE(dstT != ISA_TYPE_DF, "double type is not expected here");
                    IGC_ASSERT_MESSAGE(srcT != ISA_TYPE_DF, "double type is not expected here");
                }
            }
            if (dst->GetVarType() != EVARTYPE_GENERAL || src->GetVarType() != EVARTYPE_GENERAL)
            {
                // code can't handle indirect operands, let vISA do it
                // ToDo: disable int64b copy emu entirely?
                Need64BitEmu = false;
            }

            CVariable* dstAlias = nullptr;
            CVariable* srcAlias = nullptr;
            VISA_VectorOpnd* srcImmLo = nullptr;
            VISA_VectorOpnd* srcImmHi = nullptr;
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
                }
                else {
                    if (Is64BitSrc)
                        srcAlias = m_program->GetNewAlias(src, ISA_TYPE_UD, 0, 0);
                    else
                        srcAlias = src;
                }
            }

            if (Need64BitEmu)
            {
                if (Is64BitSrc && Is64BitDst)
                {
                    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
                    if (!predOpnd && !IsSat() && dst->IsUniform() && src->IsUniform() && !src->IsImmediate() && m_encoderState.m_uniformSIMDSize == SIMDMode::SIMD1)
                    {
                        // special handling for uniform 64b copy by generating SIMD2 move instead of 2xSIMD1
                        // technically we need to check for src modifier and whether dst/src are indirect operand as well,
                        // but it doesn't look like the original code below is doing it anyway..
                        SModifier dstAsUDMod = m_encoderState.m_dstOperand;
                        dstAsUDMod.subReg *= 2;
                        SModifier srcAsUDMod = m_encoderState.m_srcOperand[0];
                        srcAsUDMod.region[0] = 1;
                        srcAsUDMod.region[1] = 1;
                        srcAsUDMod.region[2] = 0;
                        srcAsUDMod.specialRegion = true;
                        srcAsUDMod.subReg *= 2;
                        auto dstOpnd = GetDestinationOperand(dstAlias, dstAsUDMod);
                        auto SIMDSize = lanesToSIMDMode(numLanes(m_encoderState.m_uniformSIMDSize) * 2);
                        auto srcOpnd = GetSourceOperand(srcAlias, srcAsUDMod);
                        V(vKernel->AppendVISADataMovementInst(opcode, nullptr, false, vISA_EMASK_M1_NM, visaExecSize(SIMDSize),
                            dstOpnd, srcOpnd));
                    }
                    else
                    {
                        // Generate data movement on Lo part.
                        SModifier LoDstMod = EmulateVariable(dst, m_encoderState.m_dstOperand, false, false);
                        SModifier LoSrcMod = EmulateVariable(src, m_encoderState.m_srcOperand[0], false, true);
                        VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dstAlias, LoDstMod);
                        VISA_VectorOpnd* srcOpnd = srcImmLo ? srcImmLo : GetSourceOperand(srcAlias, LoSrcMod);

                        V(vKernel->AppendVISADataMovementInst(opcode, predOpnd, IsSat(),
                            GetAluEMask(dst),
                            GetAluExecSize(dst),
                            dstOpnd, srcOpnd));
                        // Generate data movement on Hi part.
                        SModifier HiDstMod = EmulateVariable(dst, m_encoderState.m_dstOperand, true, false);
                        SModifier HiSrcMod = EmulateVariable(src, m_encoderState.m_srcOperand[0], true, true);
                        dstOpnd = GetDestinationOperand(dstAlias, HiDstMod);
                        srcOpnd = srcImmHi ? srcImmHi : GetSourceOperand(srcAlias, HiSrcMod);
                        predOpnd = GetFlagOperand(m_encoderState.m_flag);
                        V(vKernel->AppendVISADataMovementInst(opcode, predOpnd, IsSat(),
                            GetAluEMask(dst),
                            GetAluExecSize(dst),
                            dstOpnd, srcOpnd));
                    }
                }
                else if (Is64BitSrc)
                {
                    IGC_ASSERT_MESSAGE(!Is64BitDst, "Expect non 64-bit dst!");
                    // Generate data movement on Lo part only.
                    SModifier LoSrcMod = EmulateVariable(src, m_encoderState.m_srcOperand[0], false, true);
                    VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dstAlias, m_encoderState.m_dstOperand);
                    VISA_VectorOpnd* srcOpnd = srcImmLo ? srcImmLo : GetSourceOperand(srcAlias, LoSrcMod);
                    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
                    V(vKernel->AppendVISADataMovementInst(opcode, predOpnd, IsSat(),
                        GetAluEMask(dst),
                        GetAluExecSize(dst),
                        dstOpnd, srcOpnd));
                }
                else
                {
                    IGC_ASSERT_MESSAGE(Is64BitDst, "Expect 64-bit dst!");
                    IGC_ASSERT_MESSAGE(!Is64BitSrc, "Expect non 64-bit src");

                    // Generate data movement on Lo part.
                    SModifier LoDstMod = EmulateVariable(dst, m_encoderState.m_dstOperand, false, false);
                    VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dstAlias, LoDstMod);
                    VISA_VectorOpnd* srcOpnd = srcImmLo ? srcImmLo : GetSourceOperand(srcAlias, m_encoderState.m_srcOperand[0]);
                    VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
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
                    predOpnd = GetFlagOperand(m_encoderState.m_flag);
                    V(vKernel->AppendVISADataMovementInst(opcode, predOpnd, IsSat(),
                        GetAluEMask(dst),
                        GetAluExecSize(dst),
                        dstOpnd, srcOpnd));
                }
            }
            else
            {
                VISA_VectorOpnd* srcOpnd = GetSourceOperand(src, m_encoderState.m_srcOperand[0]);
                VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dst, m_encoderState.m_dstOperand);
                VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
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

    void CEncoder::LogicOp(
        ISA_Opcode opcode,
        CVariable* dst,
        CVariable* src0,
        CVariable* src1,
        CVariable* src2,
        CVariable* src3)
    {
        if (dst->GetVarType() == EVARTYPE_PREDICATE ||
            src0->GetVarType() == EVARTYPE_PREDICATE ||
            (src1 != nullptr && src1->GetVarType() == EVARTYPE_PREDICATE))
        {
            VISA_PredVar* src1Dcl = NULL;
            if (src1 != NULL)
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
            VISA_VectorOpnd* srcOpnd0 = GetSourceOperand(src0, m_encoderState.m_srcOperand[0]);
            VISA_VectorOpnd* srcOpnd1 = GetSourceOperand(src1, m_encoderState.m_srcOperand[1]);
            VISA_VectorOpnd* srcOpnd2 = GetSourceOperand(src2, m_encoderState.m_srcOperand[2]);
            VISA_VectorOpnd* srcOpnd3 = GetSourceOperand(src3, m_encoderState.m_srcOperand[3]);
            VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dst, m_encoderState.m_dstOperand);
            VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);

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

    void CEncoder::Arithmetic(ISA_Opcode opcode, CVariable* dst, CVariable* src0, CVariable* src1, CVariable* src2)
    {
        VISA_VectorOpnd* srcOpnd0 = GetSourceOperand(src0, m_encoderState.m_srcOperand[0]);
        VISA_VectorOpnd* srcOpnd1 = GetSourceOperand(src1, m_encoderState.m_srcOperand[1]);
        VISA_VectorOpnd* srcOpnd2 = GetSourceOperand(src2, m_encoderState.m_srcOperand[2]);
        VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dst, m_encoderState.m_dstOperand);
        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
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

    void CEncoder::Bfn(uint8_t booleanFuncCtrl, CVariable* dst, CVariable* src0, CVariable* src1, CVariable* src2)
    {
        VISA_VectorOpnd* srcOpnd0 = GetSourceOperand(src0, m_encoderState.m_srcOperand[0]);
        VISA_VectorOpnd* srcOpnd1 = GetSourceOperand(src1, m_encoderState.m_srcOperand[1]);
        VISA_VectorOpnd* srcOpnd2 = GetSourceOperand(src2, m_encoderState.m_srcOperand[2]);
        VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dst, m_encoderState.m_dstOperand);
        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);

        V(vKernel->AppendVISABfnInst(
            booleanFuncCtrl,
            predOpnd,
            IsSat(),
            GetAluEMask(dst),
            GetAluExecSize(dst),
            dstOpnd,
            srcOpnd0,
            srcOpnd1,
            srcOpnd2));
    }

    // We allow H1 to be nullptr for the common case of adding 64-bit variable with 32-bit imm
    void CEncoder::AddPair(CVariable* Lo, CVariable* Hi, CVariable* L0, CVariable* H0, CVariable* L1, CVariable* H1) {
        IGC_ASSERT_MESSAGE(m_encoderState.m_dstOperand.mod == EMOD_NONE, "addPair doesn't support saturate");

        if (Hi == nullptr) {
            // When Hi part is ignored, reduce 64-bit subtraction into 32-bit.
            GenericAlu(EOPCODE_ADD, Lo, L0, L1);
            return;
        }

        if (Lo == nullptr) {
            // We cannot reduce the strength if only Lo is ignored.
            Lo = m_program->GetNewVariable(
                Hi->GetNumberElement(), Hi->GetType(), Hi->GetAlign(), Hi->IsUniform(), Hi->getName());
        }

        // Use `UD` only.
        if (Lo->GetType() != ISA_TYPE_UD && Lo->GetType() != ISA_TYPE_UV) Lo = m_program->BitCast(Lo, ISA_TYPE_UD);
        if (Hi->GetType() != ISA_TYPE_UD && Hi->GetType() != ISA_TYPE_UV) Hi = m_program->BitCast(Hi, ISA_TYPE_UD);
        if (L0->GetType() != ISA_TYPE_UD && L0->GetType() != ISA_TYPE_UV) L0 = m_program->BitCast(L0, ISA_TYPE_UD);
        if (H0->GetType() != ISA_TYPE_UD && H0->GetType() != ISA_TYPE_UV) H0 = m_program->BitCast(H0, ISA_TYPE_UD);
        if (L1->GetType() != ISA_TYPE_UD && L1->GetType() != ISA_TYPE_UV) L1 = m_program->BitCast(L1, ISA_TYPE_UD);
        if (H1 && H1->GetType() != ISA_TYPE_UD && H1->GetType() != ISA_TYPE_UV) H1 = m_program->BitCast(H1, ISA_TYPE_UD);

        VISA_Exec_Size ExecSize = GetAluExecSize(Lo);
        IGC_ASSERT((ExecSize == EXEC_SIZE_32) || (ExecSize == EXEC_SIZE_16) || (ExecSize == EXEC_SIZE_8) || (ExecSize == EXEC_SIZE_4) || (ExecSize == EXEC_SIZE_2) || (ExecSize == EXEC_SIZE_1));

        if (needsSplitting(ExecSize))
        {
            // Have to split it because `acc0` has only 8 elements for 32-bit
            // integer types.
            unsigned NumParts = 2;
            VISA_EMask_Ctrl ExecMask = GetAluEMask(Lo);
            VISA_Exec_Size FromExecSize = GetAluExecSize(Lo);
            VISA_Exec_Size ToExecSize = SplitExecSize(FromExecSize, NumParts);

            VISA_PredOpnd* Pred = GetFlagOperand(m_encoderState.m_flag);
            for (unsigned ThePart = 0; ThePart != NumParts; ++ThePart) {
                SModifier NewDstMod = SplitVariable(FromExecSize, ToExecSize, ThePart, Lo, m_encoderState.m_dstOperand);
                SModifier NewS0LMod = SplitVariable(FromExecSize, ToExecSize, ThePart, L0, m_encoderState.m_srcOperand[0], true);
                SModifier NewS0HMod = SplitVariable(FromExecSize, ToExecSize, ThePart, H0, m_encoderState.m_srcOperand[1], true);
                SModifier NewS1LMod = SplitVariable(FromExecSize, ToExecSize, ThePart, L1, m_encoderState.m_srcOperand[2], true);

                VISA_VectorOpnd* S0L = GetSourceOperand(L0, NewS0LMod);
                VISA_VectorOpnd* S0H = GetSourceOperand(H0, NewS0HMod);
                VISA_VectorOpnd* S1L = GetSourceOperand(L1, NewS1LMod);
                VISA_VectorOpnd* L = GetDestinationOperand(Lo, NewDstMod);
                VISA_VectorOpnd* H = GetDestinationOperand(Hi, NewDstMod);
                VISA_VectorOpnd* HIn = GetSourceOperand(Hi, NewDstMod);

                unsigned NumElems = m_program->m_Platform->getAccChNumUD();
                CVariable* Carry = m_program->GetNewVariable(
                    (uint16_t)NumElems, Lo->GetType(), Lo->GetAlign(), Lo->IsUniform(),
                    CName(Lo->getName(), "Carry"));
                VISA_VectorOpnd* AccOut = GetDestinationOperand(Carry, m_encoderState.m_dstOperand);
                VISA_VectorOpnd* AccIn = GetSourceOperand(Carry, m_encoderState.m_dstOperand);

                VISA_EMask_Ctrl EMask = SplitEMask(FromExecSize, ToExecSize, ThePart, ExecMask);
                V(vKernel->AppendVISATwoDstArithmeticInst(
                    ISA_ADDC, Pred, EMask, ToExecSize,
                    L, AccOut, S0L, S1L));

                if (H1 && !(H1->IsImmediate() && H1->GetImmediateValue() == 0))
                {
                    SModifier NewS1HMod = SplitVariable(FromExecSize, ToExecSize, ThePart, H1, m_encoderState.m_srcOperand[3], true);
                    VISA_VectorOpnd* S1H = GetSourceOperand(H1, NewS1HMod);
                    if (m_program->m_Platform->supportAdd3Instruction())
                    {
                        H = GetDestinationOperand(Hi, NewDstMod);
                        V(vKernel->AppendVISAArithmeticInst(
                            ISA_ADD3, Pred, false, EMask, ToExecSize,
                            H, AccIn, S0H, S1H));
                    }
                    else
                    {
                        V(vKernel->AppendVISAArithmeticInst(
                            ISA_ADD, Pred, false, EMask, ToExecSize,
                            H, S0H, S1H));
                        H = GetDestinationOperand(Hi, NewDstMod);
                        V(vKernel->AppendVISAArithmeticInst(
                            ISA_ADD, Pred, false, EMask, ToExecSize,
                            H, AccIn, HIn));
                    }
                }
                else
                {
                    V(vKernel->AppendVISAArithmeticInst(
                        ISA_ADD, Pred, false, EMask, ToExecSize,
                        H, AccIn, S0H));
                }
            }
        }
        else {
            VISA_VectorOpnd* S0L = GetSourceOperand(L0, m_encoderState.m_srcOperand[0]);
            VISA_VectorOpnd* S0H = GetSourceOperand(H0, m_encoderState.m_srcOperand[1]);
            VISA_VectorOpnd* S1L = GetSourceOperand(L1, m_encoderState.m_srcOperand[2]);
            VISA_VectorOpnd* L = GetDestinationOperand(Lo, m_encoderState.m_dstOperand);
            VISA_VectorOpnd* H = GetDestinationOperand(Hi, m_encoderState.m_dstOperand);
            VISA_PredOpnd* Pred = GetFlagOperand(m_encoderState.m_flag);

            unsigned short NumElems = (ExecSize == EXEC_SIZE_1) ? 1 :
                (ExecSize == EXEC_SIZE_2) ? 2 :
                (ExecSize == EXEC_SIZE_4) ? 4 : m_program->m_Platform->getAccChNumUD();
            CVariable* Carry = m_program->GetNewVariable(
                NumElems, Lo->GetType(), Lo->GetAlign(), Lo->IsUniform(), CName(Lo->getName(), "Carry"));
            VISA_VectorOpnd* AccOut = GetDestinationOperand(Carry, m_encoderState.m_dstOperand);

            SModifier MidMod = m_encoderState.m_dstOperand;
            if (Lo->IsUniform() && NumElems != 1) {
                MidMod.region[0] = 1;
                MidMod.region[1] = 1;
                MidMod.region[2] = 0;
                MidMod.specialRegion = true;
            }
            VISA_VectorOpnd* HIn = GetSourceOperand(Hi, MidMod);
            VISA_VectorOpnd* AccIn = GetSourceOperand(Carry, MidMod);

            VISA_EMask_Ctrl ExecMask = GetAluEMask(Lo);
            V(vKernel->AppendVISATwoDstArithmeticInst(
                ISA_ADDC, Pred, ExecMask, ExecSize,
                L, AccOut, S0L, S1L));

            if (H1 && !(H1->IsImmediate() && H1->GetImmediateValue() == 0))
            {
                VISA_VectorOpnd* S1H = GetSourceOperand(H1, m_encoderState.m_srcOperand[3]);
                if (m_program->m_Platform->supportAdd3Instruction())
                {
                    H = GetDestinationOperand(Hi, m_encoderState.m_dstOperand);
                    V(vKernel->AppendVISAArithmeticInst(
                        ISA_ADD3, Pred, false, ExecMask, ExecSize,
                        H, AccIn, S0H, S1H));
                }
                else
                {
                    V(vKernel->AppendVISAArithmeticInst(
                        ISA_ADD, Pred, false, ExecMask, ExecSize,
                        H, S0H, S1H));
                    H = GetDestinationOperand(Hi, m_encoderState.m_dstOperand);
                    V(vKernel->AppendVISAArithmeticInst(
                        ISA_ADD, Pred, false, ExecMask, ExecSize,
                        H, AccIn, HIn));
                }
            }
            else
            {
                V(vKernel->AppendVISAArithmeticInst(
                    ISA_ADD, Pred, false, ExecMask, ExecSize,
                    H, AccIn, S0H));
            }
        }
    }

    void CEncoder::SubPair(CVariable* Lo, CVariable* Hi, CVariable* L0, CVariable* H0, CVariable* L1, CVariable* H1) {
        IGC_ASSERT_MESSAGE(m_encoderState.m_dstOperand.mod == EMOD_NONE, "subPair doesn't support saturate");

        IGC_ASSERT(Lo || Hi);  // At least one is used
        if (Hi == nullptr) {
            // When Hi part is ignored, reduce 64-bit subtraction into 32-bit.
            SetSrcModifier(1, EMOD_NEG);
            GenericAlu(EOPCODE_ADD, Lo, L0, L1);
            return;
        }

        if (Lo == nullptr) {
            // We cannot reduce the strength if only Lo is ignored.
            Lo = m_program->GetNewVariable(
                Hi->GetNumberElement(), Hi->GetType(), Hi->GetAlign(), Hi->IsUniform(), CName(Hi->getName(), "Carry"));
        }

        VISA_Exec_Size ExecSize = GetAluExecSize(Lo);
        IGC_ASSERT((ExecSize == EXEC_SIZE_32) || (ExecSize == EXEC_SIZE_16) || (ExecSize == EXEC_SIZE_8) || (ExecSize == EXEC_SIZE_1));

        // Use `UD` only.
        if (Lo->GetType() != ISA_TYPE_UD && Lo->GetType() != ISA_TYPE_UV) Lo = m_program->BitCast(Lo, ISA_TYPE_UD);
        if (Hi->GetType() != ISA_TYPE_UD && Hi->GetType() != ISA_TYPE_UV) Hi = m_program->BitCast(Hi, ISA_TYPE_UD);
        if (L0->GetType() != ISA_TYPE_UD && L0->GetType() != ISA_TYPE_UV) L0 = m_program->BitCast(L0, ISA_TYPE_UD);
        if (H0->GetType() != ISA_TYPE_UD && H0->GetType() != ISA_TYPE_UV) H0 = m_program->BitCast(H0, ISA_TYPE_UD);
        if (L1->GetType() != ISA_TYPE_UD && L1->GetType() != ISA_TYPE_UV) L1 = m_program->BitCast(L1, ISA_TYPE_UD);
        if (H1->GetType() != ISA_TYPE_UD && H1->GetType() != ISA_TYPE_UV) H1 = m_program->BitCast(H1, ISA_TYPE_UD);

        if (needsSplitting(ExecSize))
        {
            // Have to split it because `acc0` has only 8 elements for 32-bit
            // integer types.
            unsigned NumParts = 2;
            VISA_EMask_Ctrl ExecMask = GetAluEMask(Lo);
            VISA_Exec_Size FromExecSize = GetAluExecSize(Lo);
            VISA_Exec_Size ToExecSize = SplitExecSize(FromExecSize, NumParts);

            // Negative `S1H`
            SModifier S1HMod = m_encoderState.m_srcOperand[1];
            IGC_ASSERT(S1HMod.mod == EMOD_NONE);
            S1HMod.mod = EMOD_NEG;
            VISA_PredOpnd* Pred = GetFlagOperand(m_encoderState.m_flag);
            for (unsigned ThePart = 0; ThePart != NumParts; ++ThePart) {
                SModifier NewDstMod = SplitVariable(FromExecSize, ToExecSize, ThePart, Lo, m_encoderState.m_dstOperand);
                SModifier NewS0LMod = SplitVariable(FromExecSize, ToExecSize, ThePart, L0, m_encoderState.m_srcOperand[0], true);
                SModifier NewS0HMod = SplitVariable(FromExecSize, ToExecSize, ThePart, H0, m_encoderState.m_srcOperand[1], true);
                SModifier NewS1LMod = SplitVariable(FromExecSize, ToExecSize, ThePart, L1, m_encoderState.m_srcOperand[2], true);
                SModifier NewS1HMod = SplitVariable(FromExecSize, ToExecSize, ThePart, H1, S1HMod, true);
                VISA_VectorOpnd* S0L = GetSourceOperand(L0, NewS0LMod);
                VISA_VectorOpnd* S0H = GetSourceOperand(H0, NewS0HMod);
                VISA_VectorOpnd* S1L = GetSourceOperand(L1, NewS1LMod);
                VISA_VectorOpnd* S1H = GetSourceOperand(H1, NewS1HMod);
                VISA_VectorOpnd* L = GetDestinationOperand(Lo, NewDstMod);
                VISA_VectorOpnd* H = GetDestinationOperand(Hi, NewDstMod);
                VISA_VectorOpnd* HIn = GetSourceOperand(Hi, NewDstMod);

                unsigned short NumElems = m_program->m_Platform->getAccChNumUD();
                CVariable* Carry =
                    m_program->GetNewVariable(NumElems, Lo->GetType(), Lo->GetAlign(), Lo->IsUniform(), CName(Lo->getName(), "Carry"));
                VISA_VectorOpnd* AccOut = GetDestinationOperand(Carry, m_encoderState.m_dstOperand);
                // Negative `Acc0`
                SModifier AccMod = m_encoderState.m_dstOperand;
                IGC_ASSERT(AccMod.mod == EMOD_NONE);
                AccMod.mod = EMOD_NEG;
                VISA_VectorOpnd* AccIn = GetSourceOperand(Carry, AccMod);

                VISA_EMask_Ctrl EMask = SplitEMask(FromExecSize, ToExecSize, ThePart, ExecMask);
                V(vKernel->AppendVISATwoDstArithmeticInst(
                    ISA_SUBB, Pred, EMask, ToExecSize,
                    L, AccOut, S0L, S1L));
                if (m_program->m_Platform->supportAdd3Instruction())
                {
                    H = GetDestinationOperand(Hi, NewDstMod);
                    V(vKernel->AppendVISAArithmeticInst(
                        ISA_ADD3, Pred, false, EMask, ToExecSize,
                        H, AccIn, S0H, S1H));
                }
                else
                {
                    V(vKernel->AppendVISAArithmeticInst(
                        ISA_ADD, Pred, false, EMask, ToExecSize,
                        H, S0H, S1H));
                    H = GetDestinationOperand(Hi, NewDstMod);
                    V(vKernel->AppendVISAArithmeticInst(
                        ISA_ADD, Pred, false, EMask, ToExecSize,
                        H, AccIn, HIn));
                }
            }
        }
        else {
            VISA_VectorOpnd* S0L = GetSourceOperand(L0, m_encoderState.m_srcOperand[0]);
            VISA_VectorOpnd* S0H = GetSourceOperand(H0, m_encoderState.m_srcOperand[1]);
            VISA_VectorOpnd* S1L = GetSourceOperand(L1, m_encoderState.m_srcOperand[2]);
            // Negative `S0H`
            SModifier S1HMod = m_encoderState.m_srcOperand[1];
            IGC_ASSERT(S1HMod.mod == EMOD_NONE);
            S1HMod.mod = EMOD_NEG;
            VISA_VectorOpnd* S1H = GetSourceOperand(H1, S1HMod);
            VISA_VectorOpnd* L = GetDestinationOperand(Lo, m_encoderState.m_dstOperand);
            VISA_VectorOpnd* H = GetDestinationOperand(Hi, m_encoderState.m_dstOperand);
            VISA_PredOpnd* Pred = GetFlagOperand(m_encoderState.m_flag);

            unsigned short NumElems = (ExecSize == 1) ? 1 : m_program->m_Platform->getAccChNumUD();
            CVariable* Carry = m_program->GetNewVariable(
                NumElems, Lo->GetType(), Lo->GetAlign(), Lo->IsUniform(), CName(Lo->getName(), "Carry"));
            VISA_VectorOpnd* AccOut = GetDestinationOperand(Carry, m_encoderState.m_dstOperand);

            SModifier MidMod = m_encoderState.m_dstOperand;
            if (Lo->IsUniform() && NumElems != 1) {
                MidMod.region[0] = 1;
                MidMod.region[1] = 1;
                MidMod.region[2] = 0;
                MidMod.specialRegion = true;
            }
            VISA_VectorOpnd* HIn = GetSourceOperand(Hi, MidMod);
            // Negative `Acc0`
            SModifier AccMod = MidMod;
            IGC_ASSERT(AccMod.mod == EMOD_NONE);
            AccMod.mod = EMOD_NEG;
            VISA_VectorOpnd* AccIn = GetSourceOperand(Carry, AccMod);

            VISA_EMask_Ctrl ExecMask = GetAluEMask(Lo);
            V(vKernel->AppendVISATwoDstArithmeticInst(
                ISA_SUBB, Pred, ExecMask, ExecSize,
                L, AccOut, S0L, S1L));
            if (m_program->m_Platform->supportAdd3Instruction())
            {
                H = GetDestinationOperand(Hi, m_encoderState.m_dstOperand);
                V(vKernel->AppendVISAArithmeticInst(
                    ISA_ADD3, Pred, false, ExecMask, ExecSize,
                    H, AccIn, S0H, S1H));
            }
            else
            {
                V(vKernel->AppendVISAArithmeticInst(
                    ISA_ADD, Pred, false, ExecMask, ExecSize,
                    H, S0H, S1H));
                H = GetDestinationOperand(Hi, m_encoderState.m_dstOperand);
                V(vKernel->AppendVISAArithmeticInst(
                    ISA_ADD, Pred, false, ExecMask, ExecSize,
                    H, AccIn, HIn));
            }
        }
    }

    void CEncoder::CarryBorrowArith(ISA_Opcode opcode, CVariable* dst, CVariable* src0, CVariable* src1)
    {
        VISA_VectorOpnd* srcOpnd0 = GetSourceOperand(src0, m_encoderState.m_srcOperand[0]);
        VISA_VectorOpnd* srcOpnd1 = GetSourceOperand(src1, m_encoderState.m_srcOperand[1]);
        VISA_VectorOpnd* dstOpnd = GetDestinationOperand(dst, m_encoderState.m_dstOperand);
        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
        SModifier carryOperand = m_encoderState.m_dstOperand;
        VISA_Exec_Size execSize = GetAluExecSize(dst);

        switch (execSize)
        {
        case EXEC_SIZE_1:
            carryOperand.subReg += 1;
            break;
        case EXEC_SIZE_8:
            carryOperand.subVar += 1;
            break;
        case EXEC_SIZE_16:
            carryOperand.subVar += 2;
            break;
        default:
            IGC_ASSERT_MESSAGE(0, "Unknown execution size on carry-borrow-arith!");
            break;
        }
        VISA_VectorOpnd* carryBorrowOpnd = GetDestinationOperand(dst, carryOperand);
        IGC_ASSERT_MESSAGE(m_encoderState.m_dstOperand.mod == EMOD_NONE, "addc/subb doesn't support saturate");

        V(vKernel->AppendVISATwoDstArithmeticInst(
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
        CVariable* mask)
    {

        IGC_ASSERT(nullptr != offset);

        VISA_EMask_Ctrl emask = ConvertMaskToVisaType(m_encoderState.m_mask, m_encoderState.m_noMask);
        VISA_Exec_Size execSize = visaExecSize(m_encoderState.m_simdSize);
        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
        VISA_RawOpnd* handle = GetRawSource(urbHandle);
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
            perSlotOffset = GetRawSource(offset);
        }

        // Three possible cases:
        // 1. Channel Mask is immediate value with 0xFF, so not needed to send
        // 2. Channel Mask is immediate value other than 0xFF, so needed to send, but as immediate value
        // 3. Channel Mask is not immediate value, so needed to send, but as not immediate value
        VISA_RawOpnd* channelMask = nullptr;
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
                CVariable* tmpDst = m_program->GetNewVariable(8, ISA_TYPE_UD, EALIGN_GRF, true, CName::NONE);
                VISA_VectorOpnd* movDst = nullptr;
                V(vKernel->CreateVISADstOperand(movDst, GetVISAVariable(tmpDst), 1, 0, 0));

                VISA_VectorOpnd* immSrc = nullptr;
                V(vKernel->CreateVISAImmediate(immSrc, &immChannelMask, ISA_TYPE_UW));

                V(vKernel->AppendVISADataMovementInst(
                    ISA_MOV, nullptr, false, emask,
                    EXEC_SIZE_8, movDst, immSrc));
                V(vKernel->CreateVISARawOperand(channelMask, GetVISAVariable(tmpDst), 0));
            }

            payloadSize = int_cast<unsigned char>(immMask.size());
        }

        VISA_RawOpnd* vertexData = GetRawSource(src, payloadElementOffset);

        V(vKernel->AppendVISA3dURBWrite(
            predOpnd,
            emask,
            execSize,
            payloadSize,
            channelMask,
            immOffset,
            handle,
            perSlotOffset,
            vertexData));
    }


    VISA_RawOpnd* CEncoder::GetRawSource(CVariable* var, uint offset)
    {
        VISA_RawOpnd* srcOpnd = nullptr;
        if (var)
        {
            if (var->IsImmediate())
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
        VISA_RawOpnd* dstOpnd = nullptr;
        if (var)
        {
            V(vKernel->CreateVISARawOperand(
                dstOpnd, GetVISAVariable(var),
                m_encoderState.m_dstOperand.subVar * getGRFSize() + offset + var->GetAliasOffset()));
        }
        else
        {
            V(vKernel->CreateVISANullRawOperand(dstOpnd, true));
        }
        return dstOpnd;
    }

    void CEncoder::Send(CVariable* dst, CVariable* src, uint exDesc, CVariable* messDescriptor, bool isSendc)
    {
        if (dst && dst->IsUniform())
        {
            m_encoderState.m_simdSize = m_encoderState.m_uniformSIMDSize;
        }
        unsigned char sendc = isSendc ? 1 : 0;
        unsigned char srcSize = src->GetSize() / getGRFSize();
        unsigned char dstSize = dst ? dst->GetSize() / getGRFSize() : 0;
        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
        VISA_RawOpnd* srcOpnd0 = GetRawSource(src);
        VISA_RawOpnd* dstOpnd = GetRawDestination(dst);
        VISA_VectorOpnd* desc = GetUniformSource(messDescriptor);

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

    void CEncoder::Sends(CVariable* dst, CVariable* src0, CVariable* src1, uint ffid, CVariable* exDesc, CVariable* messDescriptor, bool isSendc, bool hasEOT)
    {
        if (exDesc->IsImmediate() && src1 == nullptr)
        {
            Send(dst, src0, (uint)exDesc->GetImmediateValue(), messDescriptor, isSendc);
            return;
        }
        if (dst && dst->IsUniform())
        {
            m_encoderState.m_simdSize = m_encoderState.m_uniformSIMDSize;
        }
        unsigned char sendc = isSendc ? 1 : 0;
        unsigned char src0Size = src0->GetSize() / getGRFSize();
        unsigned char src1Size = src1 ? src1->GetSize() / getGRFSize() : 0;
        unsigned char dstSize = dst ? dst->GetSize() / getGRFSize() : 0;
        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
        VISA_RawOpnd* srcOpnd0 = GetRawSource(src0);
        VISA_RawOpnd* srcOpnd1 = GetRawSource(src1);
        VISA_RawOpnd* dstOpnd = GetRawDestination(dst);
        VISA_VectorOpnd* exMessDesc = GetUniformSource(exDesc);
        VISA_VectorOpnd* desc = GetUniformSource(messDescriptor);

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
            dstOpnd,
            hasEOT));
    }

    VISA_StateOpndHandle* CEncoder::GetBTIOperand(uint bindingTableIndex)
    {
        IGC::e_predefSurface predDefSurface = ESURFACE_NORMAL;
        if (bindingTableIndex == 255)
            predDefSurface = ESURFACE_STATELESS;
        else if (bindingTableIndex == 254)
            predDefSurface = ESURFACE_SLM;
        CVariable tempImm(bindingTableIndex, ISA_TYPE_UD);
        return GetVISASurfaceOpnd(predDefSurface, &tempImm);
    }

    void CEncoder::RenderTargetWrite(CVariable* var[],
        bool isUndefined[],
        bool lastRenderTarget,
        bool isNullRT,
        bool perSample,
        bool coarseMode,
        bool headerMaskFromCe0,
        CVariable* bindingTableIndex,
        CVariable* RTIndex,
        CVariable* source0Alpha,
        CVariable* oMask,
        CVariable* depth,
        CVariable* stencil,
        CVariable* CPSCounter,
        CVariable* sampleIndex,
        CVariable* r1Reg)
    {
        VISA_EMask_Ctrl emask = ConvertMaskToVisaType(m_encoderState.m_mask, m_encoderState.m_noMask);
        VISA_Exec_Size execSize = visaExecSize(m_encoderState.m_simdSize);
        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
        VISA_StateOpndHandle* surfOpnd = GetVISASurfaceOpnd(ESURFACE_NORMAL, bindingTableIndex);

        vISA_RT_CONTROLS cntrls;
        uint8_t numMsgSpecificOpnds = 0;
        VISA_RawOpnd* srcOpnd[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

        cntrls.isPerSample = perSample;
        cntrls.isCoarseMode = coarseMode;
        cntrls.isHeaderMaskfromCe0 = headerMaskFromCe0;
        IGC_ASSERT(!((predOpnd != nullptr) && cntrls.isHeaderMaskfromCe0));

        if (source0Alpha)
        {
            cntrls.s0aPresent = true;
            srcOpnd[numMsgSpecificOpnds++] = GetRawSource(source0Alpha);
        }
        else
            cntrls.s0aPresent = false;

        if (oMask)
        {
            cntrls.oMPresent = true;
            srcOpnd[numMsgSpecificOpnds++] = GetRawSource(oMask);
        }
        else
            cntrls.oMPresent = false;

        for (int i = 0; i < 4; i++)
        {
            if (isUndefined[i])
            {
                V(vKernel->CreateVISANullRawOperand(srcOpnd[numMsgSpecificOpnds++], false));
            }
            else
            {
                srcOpnd[numMsgSpecificOpnds++] = GetRawSource(var[i]);
            }
        }

        if (depth)
        {
            cntrls.zPresent = true;
            srcOpnd[numMsgSpecificOpnds++] = GetRawSource(depth);
        }
        else
            cntrls.zPresent = false;

        if (stencil)
        {
            cntrls.isStencil = true;
            srcOpnd[numMsgSpecificOpnds++] = GetRawSource(stencil);
        }
        else
            cntrls.isStencil = false;

        cntrls.isSampleIndex = false;
        VISA_VectorOpnd* sampleIndexOpnd = NULL;
        if (sampleIndex)
        {
            sampleIndexOpnd = GetSourceOperandNoModifier(sampleIndex);
            cntrls.isSampleIndex = true;
        }
        VISA_VectorOpnd* cpsCounterOpnd = GetSourceOperandNoModifier(CPSCounter);

        VISA_VectorOpnd* RTIndexOpnd = nullptr;
        cntrls.RTIndexPresent = false;
        // if RTIndex is 0, then no need to prepare the header for send
        if (!RTIndex->IsImmediate() || RTIndex->GetImmediateValue() != 0)
        {
            RTIndexOpnd = GetSourceOperandNoModifier(RTIndex);
            cntrls.RTIndexPresent = true;
        }

        //controls last render target select bit
        cntrls.isLastWrite = lastRenderTarget;

        // controls NULL render target enbale bit
        cntrls.isNullRT = isNullRT;

        //r1Reg should always be populated
        //vISA will decide whether to use it or not.
        VISA_RawOpnd* r1RegOpnd = GetRawSource(r1Reg);


        if (CPSCounter)
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
                if (m_program->GetContext()->m_DriverInfo.SupportMoreThan16Samplers())
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

        IGC_ASSERT(nullptr != sampler.m_sampler);
        IGC_ASSERT(sampler.m_sampler->IsUniform());
        VISA_VectorOpnd* sourecOpnd = GetUniformSource(sampler.m_sampler);

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

        if (!m_program->m_Platform->hasSamplerSupport())
            return;

        int numMsgSpecificOpnds = numSources;
        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
        bool isIdxLT16;
        VISA_StateOpndHandle* samplerOpnd = GetSamplerOperand(sampler, isIdxLT16);
        VISA_StateOpndHandle* btiOpnd = GetVISASurfaceOpnd(resource);
        VISA_RawOpnd* dstVar = GetRawDestination(dst);
        VISA_RawOpnd* opndArray[11];
        for (int i = 0; i < numMsgSpecificOpnds; i++)
        {
            opndArray[i] = GetRawSource(payload[i]);
        }

        VISA_VectorOpnd* aoffimmi = GetSourceOperandNoModifier(offset);
        // Use bit 15 of aoffimmi to tell VISA the sample index could be greater
        // than 15.  In this case, we need to use msg header, and setup M0.3
        // to point to next 16 sampler state.
        if (!isIdxLT16)
        {
            uint16_t aoffimmiVal = (uint16_t)offset->GetImmediateValue() | BIT(15);
            V(vKernel->CreateVISAImmediate(aoffimmi, &aoffimmiVal, ISA_TYPE_UW));
        }

        {
            int status = vKernel->AppendVISA3dSampler(
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
                opndArray);

            V(status);
        }
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

        if (!m_program->m_Platform->hasSamplerSupport())
            return;

        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
        VISA_StateOpndHandle* surfOpnd = GetVISASurfaceOpnd(resource);
        VISA_RawOpnd* dstVar = GetRawDestination(dst);

        VISA_RawOpnd* opndArray[11];
        for (unsigned int i = 0; i < numSources; i++)
        {
            opndArray[i] = GetRawSource(payload[i]);
        }

        VISA_VectorOpnd* aoffimmi = GetSourceOperandNoModifier(offset);

        {
            int status = vKernel->AppendVISA3dLoad(
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
                opndArray);

            V(status);
        }
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

        if (!m_program->m_Platform->hasSamplerSupport())
            return;

        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
        bool isIdxLT16;
        VISA_StateOpndHandle* samplerOpnd = GetSamplerOperand(sampler, isIdxLT16);
        VISA_StateOpndHandle* surfOpnd = GetVISASurfaceOpnd(resource);
        VISA_RawOpnd* dstVar = GetRawDestination(dst);
        VISA_RawOpnd* opndArray[11];
        for (unsigned int i = 0; i < numSources; i++)
        {
            opndArray[i] = GetRawSource(payload[i]);
        }

        VISA_VectorOpnd* aoffimmi = GetSourceOperandNoModifier(offset);
        if (!isIdxLT16)
        {
            uint16_t aoffimmiVal = (uint16_t)offset->GetImmediateValue() | BIT(15);
            V(vKernel->CreateVISAImmediate(aoffimmi, &aoffimmiVal, ISA_TYPE_UW));
        }

        {
            int status = vKernel->AppendVISA3dGather4(
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
                opndArray);

            V(status);
        }
    }

    void CEncoder::AddrAdd(CVariable* dst, CVariable* src0, CVariable* src1)
    {
        // On ICL+ platforms address register must be initialized if it is used
        // in VxH indirect addressing to avoid out-of-bounds access on inactive
        // lanes. VISA initializes address register at the beginning of the
        // shader which is sufficient for shaders that use address register only
        // for indirect addressing but is not sufficient if shader also uses
        // address register in send descriptors. The latter case is handled by
        // the initialization below.
        // see VISA Optimizer::resetA0()
        const bool mayUseA0InSendDesc =
            m_program->GetContext()->m_instrTypes.mayHaveIndirectResources;
        const bool needsA0Reset =
            m_program->m_Platform->NeedResetA0forVxHA0();

        if (((mayUseA0InSendDesc && needsA0Reset) ||
            IGC_IS_FLAG_ENABLED(InitializeAddressRegistersBeforeUse)) &&
            !dst->IsUniform() &&
            !m_encoderState.m_noMask)
        {
            m_encoderState.m_noMask = true;
            VISA_VectorOpnd* srcOpnd = nullptr;
            VISA_VectorOpnd* dstOpnd = nullptr;
            const DWORD zero = 0;
            V(vKernel->CreateVISAImmediate(srcOpnd, &zero, ISA_TYPE_UW));
            V(vKernel->CreateVISAAddressDstOperand(dstOpnd, dst->visaAddrVariable, 0));
            V(vKernel->AppendVISADataMovementInst(
                ISA_MOV,
                nullptr,
                false,
                GetAluEMask(dst),
                visaExecSize(m_encoderState.m_simdSize),
                dstOpnd,
                srcOpnd));
            m_encoderState.m_noMask = false;
        }

        if (dst->IsUniform())
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
        IGC_ASSERT((L3_Flush_Instructions + L3_Flush_Texture_Data + L3_Flush_Constant_Data + L3_Flush_RW_Data) <= 1);

        uint fenceFlags = (L3_Flush_Instructions << 1) |
            (L3_Flush_Texture_Data << 2) |
            (L3_Flush_Constant_Data << 3) |
            (L3_Flush_RW_Data << 4) |
            ((!Global_Mem_Fence) << 5) | // bit 5: 1 -- local, 0 -- global
            (L1_Flush_Constant_Data << 6) |
            (SWFence << 7) |
            (CommitEnable << 0);

        V(vKernel->AppendVISASyncInst(ISA_FENCE, int_cast<unsigned char>(fenceFlags)));
    }

    void CEncoder::FlushSamplerCache()
    {
        V(vKernel->AppendVISASyncInst(ISA_SAMPLR_CACHE_FLUSH));
    }

    void CEncoder::EOT()
    {
        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
        V(vKernel->AppendVISACFRetInst(predOpnd, vISA_EMASK_M1, EXEC_SIZE_1));
    }

    // Init Control register for denorm modes, rounding modes, etc.
    void CEncoder::initCR(VISAKernel* vKernel)
    {
        // Those bits must be zero'ed on entry to kernel/shader.
        // (If not, this function needs to be changed accordingly.)
        VISA_VectorOpnd* src0_Opnd = nullptr;
        VISA_VectorOpnd* src1_Opnd = nullptr;
        VISA_VectorOpnd* dst_Opnd = nullptr;
        VISA_GenVar* cr0_var = nullptr;
        uint imm_data = 0;

        CodeGenContext* pCtx = m_program->GetContext();
        if (pCtx->m_floatDenormMode16 == FLOAT_DENORM_RETAIN)
            imm_data |= 0x400;
        if (pCtx->m_floatDenormMode32 == FLOAT_DENORM_RETAIN)
            imm_data |= 0x80;
        if (pCtx->m_floatDenormMode64 == FLOAT_DENORM_RETAIN)
            imm_data |= 0x40;

        uint RM_bits = 0;
        ERoundingMode RM_FPCvtInt = static_cast<ERoundingMode>(pCtx->getModuleMetaData()->compOpt.FloatCvtIntRoundingMode);
        ERoundingMode RM_FP = static_cast<ERoundingMode>(pCtx->getModuleMetaData()->compOpt.FloatRoundingMode);
        if (RM_FPCvtInt == ERoundingMode::ROUND_TO_ZERO) {
            // No need to set FPCvtInt, just need to set FP RM.
            RM_bits = getEncoderRoundingMode_FP(RM_FP);
        }
        else if (RM_FPCvtInt == RM_FP) {
            // Setting FPCvtInt will set both FPCvtInt and FP
            RM_bits = getEncoderRoundingMode_FPCvtInt(RM_FPCvtInt);
        }
        else {
            IGC_ASSERT_MESSAGE(0, "Unsupport combination of default rounding mode (FP and FPCvtInt)!");
        }
        imm_data |= RM_bits;

        // If we are in the default mode no need to set the CR
        if (imm_data != 0)
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
        VISA_GenVar* cr0_var = nullptr;
        uint bitmaskImm = 1 << 3;
        if (!VMask)
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

    void CEncoder::SetRoundingMode_FP(ERoundingMode actualRM, ERoundingMode newRM)
    {
        IGC_ASSERT_MESSAGE(newRM != ERoundingMode::ROUND_TO_ANY, "Invalid rounding mode");
        if (actualRM != newRM)
        {
            RMEncoding actualRM_en = getEncoderRoundingMode_FP(actualRM);
            RMEncoding newRM_en = getEncoderRoundingMode_FP(newRM);
            SetRoundingMode(actualRM_en, newRM_en);
        }
    }

    void CEncoder::SetRoundingMode_FPCvtInt(ERoundingMode actualRM, ERoundingMode newRM)
    {
        IGC_ASSERT_MESSAGE(newRM != ERoundingMode::ROUND_TO_ANY, "Invalid rounding mode");
        if (actualRM != newRM)
        {
            RMEncoding actualRM_en = getEncoderRoundingMode_FPCvtInt(actualRM);
            RMEncoding newRM_en = getEncoderRoundingMode_FPCvtInt(newRM);
            SetRoundingMode(actualRM_en, newRM_en);
        }
    }

    // Set rounding mode based on given encoding.
    void CEncoder::SetRoundingMode(RMEncoding actualRM, RMEncoding newRM)
    {
        IGC_ASSERT_MESSAGE((actualRM != newRM), "Only setting RM if the new RM is different from the current RM!");

        VISA_VectorOpnd* src0_Opnd = nullptr;
        VISA_VectorOpnd* src1_Opnd = nullptr;
        VISA_VectorOpnd* dst_Opnd = nullptr;
        VISA_GenVar* cr0_var = nullptr;
        uint roundingMode = actualRM ^ newRM;
        IGC_ASSERT(nullptr != vKernel);
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

    CEncoder::RMEncoding CEncoder::getEncoderRoundingMode_FP(ERoundingMode FP_RM)
    {
        switch (FP_RM) {
        default:
            break;
        case ROUND_TO_POSITIVE:
            return RMEncoding::RoundToPositive;
        case ROUND_TO_NEGATIVE:
            return RMEncoding::RoundToNegative;
        case ROUND_TO_ZERO:
            return RMEncoding::RoundToZero;
        }
        return RMEncoding::RoundToNearestEven;
    }

    CEncoder::RMEncoding CEncoder::getEncoderRoundingMode_FPCvtInt(ERoundingMode FCvtI_RM)
    {
        switch (FCvtI_RM) {
        default:
            break;
        case ROUND_TO_NEAREST_EVEN:
            return RMEncoding::RoundToNearestEven_int;
        case ROUND_TO_POSITIVE:
            return RMEncoding::RoundToPositive_int;
        case ROUND_TO_NEGATIVE:
            return RMEncoding::RoundToNegative_int;
        }
        return RMEncoding::RoundToZero_int;
    }

    VISA_LabelOpnd* CEncoder::GetLabel(uint label)
    {
        VISA_LabelOpnd* visaLabel = labelMap[label];
        if (visaLabel == nullptr)
        {
            // all blocks should have labels; but new blocks inserted during
            // encoding might not
            VISA_Label_Kind kind = LABEL_BLOCK;

            std::stringstream lbl;
            if (labelNameMap[label].empty()) {
                lbl << CreateShortLabel(labelCounter++);
            } else {
                lbl << labelNameMap[label].getVisaCString();
            }
            V(vKernel->CreateVISALabelVar(visaLabel, lbl.str().c_str(), kind));
            labelMap[label] = visaLabel;
        }
        return visaLabel;
    }

    VISAFunction* CEncoder::GetStackFunction(llvm::Function* F)
    {
        auto Iter = stackFuncMap.find(F);
        if (Iter != stackFuncMap.end())
        {
            return Iter->second;
        }
        VISAFunction* visaFunc = nullptr;
        V(vbuilder->AddFunction(visaFunc, F->getName().data()));
        stackFuncMap[F] = visaFunc;
        return visaFunc;
    }

    VISA_LabelOpnd* CEncoder::GetFuncLabel(llvm::Function* F)
    {
        auto Iter = funcLabelMap.find(F);
        if (Iter != funcLabelMap.end())
        {
            return Iter->second;
        }

        // Create a new function label.
        VISA_LabelOpnd* visaLabel = nullptr;
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
        if (var == nullptr)
        {
            return nullptr;
        }
        if (var->IsImmediate())
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
        switch (platform->GetPlatformFamily())
        {
        case IGFX_GEN8_CORE:
            if (platform->getPlatformInfo().eProductFamily == IGFX_CHERRYVIEW)
            {
                return GENX_CHV;
            }
            else
            {
                return GENX_BDW;
            }
            // fall-through
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
            // fall-through
        case IGFX_GEN11_CORE:
            return GENX_ICLLP;
        case IGFX_GEN12_CORE:
        case IGFX_XE_HP_CORE:
        case IGFX_GEN12LP_CORE:
            if (   platform->getPlatformInfo().eProductFamily == IGFX_TIGERLAKE_LP
                || platform->getPlatformInfo().eProductFamily == IGFX_DG1
                || platform->getPlatformInfo().eProductFamily == IGFX_ROCKETLAKE
                || platform->getPlatformInfo().eProductFamily == IGFX_ALDERLAKE_S
                || platform->getPlatformInfo().eProductFamily == IGFX_ALDERLAKE_P
               )
            {
                return GENX_TGLLP;
            }
            else if (platform->getPlatformInfo().eProductFamily == IGFX_XE_HP_SDV)
            {
                return Xe_XeHPSDV;
            }
            // fall-through
        default:
            IGC_ASSERT_MESSAGE(0, "unsupported platform");
            break;
        }
        return GENX_SKL;
    }

    void CEncoder::OWLoad(CVariable* dst, const ResourceDescriptor& resource, CVariable* src0, bool owordAligned, uint bytesToBeRead, uint dstOffset)
    {
        VISA_StateOpndHandle* surfOpnd = GetVISASurfaceOpnd(resource);
        VISA_VectorOpnd* offset = GetUniformSource(src0);
        VISA_RawOpnd* dstVar = GetRawDestination(dst, dstOffset);
        uint size = (bytesToBeRead / SIZE_OWORD);

        V(vKernel->AppendVISASurfAccessOwordLoadStoreInst(
            owordAligned ? ISA_OWORD_LD : ISA_OWORD_LD_UNALIGNED,
            vISA_EMASK_M1_NM,  // OWord load is always nomask
            surfOpnd,
            ConvertSizeToVisaType(size),
            offset,
            dstVar));
    }

    void CEncoder::OWStore(CVariable* data, e_predefSurface surfaceType, CVariable* bufId, CVariable* src0, uint bytesToBeRead, uint srcOffset)
    {
        VISA_StateOpndHandle* surfOpnd = GetVISASurfaceOpnd(surfaceType, bufId);
        VISA_VectorOpnd* offset = GetUniformSource(src0);
        VISA_RawOpnd* dataVar = GetRawSource(data, srcOffset);
        uint size = (bytesToBeRead / SIZE_OWORD);

        V(vKernel->AppendVISASurfAccessOwordLoadStoreInst(
            ISA_OWORD_ST,
            vISA_EMASK_M1_NM,
            surfOpnd,
            ConvertSizeToVisaType(size),
            offset,
            dataVar));
        if (ESURFACE_STATELESS == surfaceType)
        {
            this->m_program->IncStatelessWritesCount();
        }
    }

    void CEncoder::OWStoreA64(CVariable* data, CVariable* src0, uint bytesToBeRead, uint srcOffset)
    {
        VISA_VectorOpnd* offset = GetUniformSource(src0);
        VISA_RawOpnd* dataVar = GetRawDestination(data, srcOffset);
        uint size = (bytesToBeRead / SIZE_OWORD);

        V(vKernel->AppendVISASvmBlockStoreInst(
            ConvertSizeToVisaType(size),
            true,   // always unaligned for now
            offset,
            dataVar));
    }

    void CEncoder::OWLoadA64(CVariable* dst, CVariable* src0, uint bytesToBeRead, uint dstOffset)
    {
        VISA_VectorOpnd* offset = GetUniformSource(src0);
        VISA_RawOpnd* dstVar = GetRawDestination(dst, dstOffset);
        uint size = (bytesToBeRead / SIZE_OWORD);

        V(vKernel->AppendVISASvmBlockLoadInst(
            ConvertSizeToVisaType(size),
            true,   // always unaligned for now
            offset,
            dstVar));
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
        uint plane)
    {
        VISA_StateOpndHandle* surfOpnd = GetVISASurfaceOpnd(surfaceType, bufId);
        VISA_VectorOpnd* xVar = GetUniformSource(xOffset);
        VISA_VectorOpnd* yVar = GetUniformSource(yOffset);
        VISA_RawOpnd* tempVar = nullptr;
        if (subOpcode == ISA_MEDIA_LD)
        {
            tempVar = GetRawDestination(dst);
        }
        else if (subOpcode == ISA_MEDIA_ST)
        {
            tempVar = GetRawSource(dst);
        }

        MEDIA_LD_mod  modi = (MEDIA_LD_mod)modifier;
        CISA_PLANE_ID planeVar = (CISA_PLANE_ID)plane;

        V(vKernel->AppendVISASurfAccessMediaLoadStoreInst(
            subOpcode,
            modi,
            surfOpnd,
            blockWidth,
            blockHeight,
            xVar,
            yVar,
            tempVar,
            planeVar));
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
        VISA_RawOpnd* pUOffset = GetRawSource(pU, m_encoderState.m_srcOperand[0].subVar * getGRFSize());
        VISA_RawOpnd* pVOffset = GetRawSource(pV, m_encoderState.m_srcOperand[1].subVar * getGRFSize());
        VISA_RawOpnd* pROffset = GetRawSource(pR, m_encoderState.m_srcOperand[2].subVar * getGRFSize());
        VISA_RawOpnd* pLODOffset = GetRawSource(pLOD, m_encoderState.m_srcOperand[3].subVar * getGRFSize());
        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
        IGC_ASSERT(0 == m_encoderState.m_dstOperand.subVar);

        VISA_RawOpnd* pDstVar = nullptr;
        VISA_EMask_Ctrl mask;
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
        if (gOffset)
        {
            globalOffsetOpnd = GetUniformSource(gOffset);
        }
        else
        {
            int value = 0;
            V(vKernel->CreateVISAImmediate(globalOffsetOpnd, &value, ISA_TYPE_UD));
        }
        VISA_RawOpnd* elementOffset = GetRawSource(offset);

        VISA_RawOpnd* dstVar = NULL;

        VISA_EMask_Ctrl mask;
        if (opcode == ISA_GATHER)
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
            visaExecSize(m_encoderState.m_simdSize),
            surfOpnd,
            globalOffsetOpnd,
            elementOffset,
            dstVar));
        if (ISA_SCATTER == opcode && ESURFACE_STATELESS == surface)
        {
            this->m_program->IncStatelessWritesCount();
        }
    }

    void CEncoder::GenericAlu(e_opcode opcode, CVariable* dst, CVariable* src0, CVariable* src1, CVariable* src2)
    {
        ISA_Opcode visaOpcode = ConvertOpcode[opcode];
        switch (visaOpcode)
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
        if (surfaceType == ESURFACE_NORMAL || surfaceType == ESURFACE_BINDLESS || surfaceType == ESURFACE_SSHBINDLESS)
        {
            VISA_SurfaceVar* surfacevar = nullptr;
            if (surfaceType == ESURFACE_BINDLESS)
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
            case ESURFACE_SCRATCH:
                // NOTE: For scratch surface, we need to shr the surface state offset coming in R0.5 by 4.
                //       This shr operation is generated by vISA in HDC path
                V(vKernel->GetPredefinedSurface(surfacevar, PREDEFINED_SURFACE_SCRATCH));
                break;
            default:
                IGC_ASSERT_MESSAGE(0, "Invalid surface");
                break;
            }
            V(vKernel->CreateVISAStateOperandHandle(surfOpnd, surfacevar));
        }
        return surfOpnd;
    }

    VISA_EMask_Ctrl CEncoder::ConvertMaskToVisaType(e_mask mask, bool noMask)
    {
        VISA_EMask_Ctrl emaskRet = vISA_EMASK_M1_NM;
        switch (mask)
        {
        case EMASK_Q1:
            if (m_encoderState.m_secondHalf)
            {
                emaskRet = noMask ? vISA_EMASK_M5_NM : vISA_EMASK_M5;
            }
            else
            {
                emaskRet = noMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1;
            }
            break;
        case EMASK_Q2:
            if (m_encoderState.m_secondHalf)
            {
                emaskRet = noMask ? vISA_EMASK_M7_NM : vISA_EMASK_M7;
            }
            else
            {
                emaskRet = noMask ? vISA_EMASK_M3_NM : vISA_EMASK_M3;
            }
            break;
        case EMASK_Q3:
            emaskRet = noMask ? vISA_EMASK_M5_NM : vISA_EMASK_M5;
            break;
        case EMASK_Q4:
            emaskRet = noMask ? vISA_EMASK_M7_NM : vISA_EMASK_M7;
            break;
        case EMASK_H1:
            emaskRet = noMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1;
            break;
        case EMASK_H2:
            emaskRet = noMask ? vISA_EMASK_M5_NM : vISA_EMASK_M5;
            break;
        default:
            IGC_ASSERT_MESSAGE(0, "unreachable");
            emaskRet = vISA_EMASK_M1_NM;
        }

        if (!m_encoderState.m_secondNibble)
            return emaskRet;

        switch (emaskRet) {
        case vISA_EMASK_M1:     return vISA_EMASK_M2;
        case vISA_EMASK_M1_NM:  return vISA_EMASK_M2_NM;
        case vISA_EMASK_M3:     return vISA_EMASK_M4;
        case vISA_EMASK_M3_NM:  return vISA_EMASK_M4_NM;
        case vISA_EMASK_M5:     return vISA_EMASK_M6;
        case vISA_EMASK_M5_NM:  return vISA_EMASK_M6_NM;
        case vISA_EMASK_M7:     return vISA_EMASK_M8;
        case vISA_EMASK_M7_NM:  return vISA_EMASK_M8_NM;
        default:
            IGC_ASSERT_MESSAGE(0, "unreachable");
            return vISA_EMASK_M1_NM;
        }
        return vISA_EMASK_M1_NM;
    }

    VISA_Modifier ConvertModifierToVisaType(e_modifier modifier)
    {
        switch (modifier)
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
            IGC_ASSERT_MESSAGE(0, "unreachable");
            return MODIFIER_NONE;
        }
    }

    VISA_Cond_Mod ConvertCondModToVisaType(e_predicate condMod)
    {
        switch (condMod)
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
            IGC_ASSERT_MESSAGE(0, "unreachable");
            return ISA_CMP_UNDEF;
        }
    }

    VISA_Oword_Num  ConvertSizeToVisaType(uint size)
    {
        switch (size)
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
            IGC_ASSERT_MESSAGE(0, "unreachable");
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
            IGC_ASSERT_MESSAGE(0, "Wrong mask");
            return CHANNEL_MASK_NOMASK;
        }
        }
    }

    VISASampler3DSubOpCode CEncoder::ConvertSubOpcode(EOPCODE subOpcode, bool zeroLOD)
    {
        switch (subOpcode)
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
            return zeroLOD ? VISA_3D_SAMPLE_LZ : VISA_3D_SAMPLE_L;
        case llvm_sample_lcptr:
            return zeroLOD ? VISA_3D_SAMPLE_C_LZ : VISA_3D_SAMPLE_L_C;
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
            return VISA_3D_LD_MCS;
        case llvm_lodptr:
            return VISA_3D_LOD;
        case llvm_sample_killpix:
            return VISA_3D_SAMPLE_KILLPIX;
        default:
            IGC_ASSERT_MESSAGE(0, "wrong sampler subopcode");
            return VISA_3D_SAMPLE;
        }
    }

    bool CEncoder::IsIntegerType(VISA_Type type)
    {
        return (type == ISA_TYPE_B ||
            type == ISA_TYPE_UB ||
            type == ISA_TYPE_W ||
            type == ISA_TYPE_UW ||
            type == ISA_TYPE_D ||
            type == ISA_TYPE_UD ||
            type == ISA_TYPE_Q ||
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
        switch (srcChannel)
        {
        case 0:
            return VISA_3D_GATHER4_CHANNEL_R;
        case 1:
            return VISA_3D_GATHER4_CHANNEL_G;
        case 2:
            return VISA_3D_GATHER4_CHANNEL_B;
        case 3:
            return VISA_3D_GATHER4_CHANNEL_A;
        default:
            IGC_ASSERT_MESSAGE(0, "Wrong channel");
            return VISA_3D_GATHER4_CHANNEL_R;
        }
    }

    void CEncoder::BeginSubroutine(llvm::Function* F)
    {
        InitLabelMap(F);
        V(vKernel->AppendVISACFLabelInst(GetFuncLabel(F)));
    }

    void CEncoder::BeginStackFunction(llvm::Function* F)
    {
        InitLabelMap(F);
        // At this place, the vISA object is changed!
        vKernel = GetStackFunction(F);
        VISA_LabelOpnd* visaLabel = nullptr;
        V(vKernel->CreateVISALabelVar(visaLabel, F->getName().data(), LABEL_SUBROUTINE));
        V(vKernel->AppendVISACFLabelInst(visaLabel));
    }

    void CEncoder::BeginPayloadSection()
    {
        // Payload Section is created as a function and compiled separately
        // from the shader body
        VISAFunction* visaFunc = nullptr;
        V(vbuilder->AddPayloadSection(visaFunc, "PayloadSection"));
        vPayloadSection = visaFunc;
        CodeGenContext* context = m_program->GetContext();
        std::string asmName;
        if (m_enableVISAdump || context->m_instrTypes.hasDebugInfo)
        {
            asmName = GetDumpFileName("asm");
        }
        else
        {
            asmName = "kernel.asm";
        }
        V(vPayloadSection->AddKernelAttribute("OutputAsmPath", asmName.length(), asmName.c_str()));

        VISA_LabelOpnd* functionLabel = nullptr;
        V(vPayloadSection->CreateVISALabelVar(functionLabel, "payload", LABEL_SUBROUTINE));
        V(vPayloadSection->AppendVISACFLabelInst(functionLabel));
        vMainKernel = vPayloadSection;
    }

    void CEncoder::AddVISASymbol(std::string& symName, CVariable* cvar)
    {
        SModifier mod;
        mod.init();
        VISA_VectorOpnd* visaSymAddr = GetDestinationOperand(cvar, mod);
        V(vKernel->AppendVISACFSymbolInst(symName, visaSymAddr));
    }

    void CEncoder::SaveOption(vISAOptions option, bool val)
    {
        OptionValue entry;
        entry.type = OpType::ET_BOOL;
        entry.vBool = val;
        m_visaUserOptions.push_back(std::make_pair(option, entry));
    }
    void CEncoder::SaveOption(vISAOptions option, uint32_t val)
    {
        OptionValue entry;
        entry.type = OpType::ET_INT32;
        entry.vInt32 = val;
        m_visaUserOptions.push_back(std::make_pair(option, entry));
    }
    void CEncoder::SaveOption(vISAOptions option, const char* val)
    {
        OptionValue entry;
        entry.type = OpType::ET_CSTR;
        entry.vCstr = val;
        m_visaUserOptions.push_back(std::make_pair(option, entry));
    }
    void CEncoder::SetBuilderOptions(VISABuilder* pbuilder)
    {
        for (auto OP : m_visaUserOptions)
        {
            switch (OP.second.type)
            {
            case OpType::ET_BOOL:
                pbuilder->SetOption(OP.first, OP.second.vBool);
                break;
            case OpType::ET_INT32:
                pbuilder->SetOption(OP.first, OP.second.vInt32);
                break;
            case OpType::ET_CSTR:
                pbuilder->SetOption(OP.first, OP.second.vCstr);
                break;
            default:
                IGC_ASSERT_MESSAGE(0, "Undefined user option type");
                break;
            }
        }
    }

    void CEncoder::InitBuildParams(llvm::SmallVector<std::unique_ptr< char, std::function<void(char*)>>, 10>& params)
    {
        CodeGenContext* context = m_program->GetContext();
        bool isOptDisabled = context->getModuleMetaData()->compOpt.OptDisable;
        using param_uptr = std::unique_ptr<char, std::function<void(char*)>>;
        auto literal_deleter = [](char* val) {};
        auto dup_deleter = [](char* val) {free(val); };
        // create vbuilder->Compile() params
        if (IGC_IS_FLAG_ENABLED(EnableVISADotAll))
        {
            params.push_back(param_uptr("-dotAll", literal_deleter));
        }
        if (IGC_IS_FLAG_ENABLED(EnableVISADebug) || isOptDisabled)
        {
            params.push_back(param_uptr("-debug", literal_deleter));
        }

        if (context->getModuleMetaData()->compOpt.FastVISACompile)
        {
            params.push_back(param_uptr("-fasterRA", literal_deleter));
            params.push_back(param_uptr("-noLocalSplit", literal_deleter));
        }
        if (IGC_IS_FLAG_ENABLED(EnableGlobalStateBuffer))
        {
            params.push_back(param_uptr("-emitCrossThreadOffR0Reloc", literal_deleter));
        }
        // Ensure VISA_Opts has the same scope as CreateVISABuilder so that valid
        // strings are checked by vISA and freed out of this function.
        if (IGC_IS_FLAG_ENABLED(VISAOptions))
        {
            std::vector<std::string> VISA_Opts;
            const char* DELIMITERS = " \t\n\v\f\r,"; // isspace(c), and comma for igcstandalone
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
            for (auto& opt : VISA_Opts) {
                // note that the memory should be freed once
                // params has been read, but since this is only for
                // debugging, do not bother freeing memory.
                params.push_back(param_uptr(_strdup(opt.c_str()), dup_deleter));
                if (opt == "-output" || opt == "-binary" || opt == "-dumpvisa" || opt == "-dumpcommonisa")
                {
                    m_enableVISAdump = true;
                }
            }
        }
        if (IGC_IS_FLAG_DISABLED(ForceDisableShaderDebugHashCodeInKernel) &&
            (context->m_DriverInfo.EnableShaderDebugHashCodeInKernel() ||
             IGC_IS_FLAG_ENABLED(ShaderDebugHashCodeInKernel)))
        {
            auto addHash = [&](char* OptName, QWORD Hash)
            {
                params.push_back(param_uptr(OptName, literal_deleter));
                std::string Low = std::to_string((DWORD)Hash);
                std::string High = std::to_string((DWORD)(Hash >> 32));
                params.push_back(param_uptr(_strdup(Low.c_str()), dup_deleter));
                params.push_back(param_uptr(_strdup(High.c_str()), dup_deleter));
            };

            QWORD AssemblyHash = context->hash.getAsmHash();
            addHash("-hashmovs", AssemblyHash);

            QWORD NosHash = context->hash.getNosHash();
            QWORD PsoHash = context->hash.getPsoHash();
            QWORD hashToUse = NosHash != 0 ? NosHash : PsoHash;
            if (hashToUse)
                addHash("-hashmovs1", hashToUse);
            else if (context->hash.getPerShaderPsoHash() != 0)
                addHash("-hashmovs1", context->hash.getPerShaderPsoHash());
        }
    }
    void CEncoder::InitVISABuilderOptions(TARGET_PLATFORM VISAPlatform, bool canAbortOnSpill, bool hasStackCall, bool enableVISA_IR)
    {
        CodeGenContext* context = m_program->GetContext();
        bool KernelDebugEnable = false;
        bool ForceNonCoherentStatelessBti = false;
        bool AllowSpill = true;

        SaveOption(vISA_Linker, IGC_GET_FLAG_VALUE(VISALTO));
        if (context->type == ShaderType::OPENCL_SHADER)
        {
            auto ClContext = static_cast<OpenCLProgramContext*>(context);
            KernelDebugEnable = ClContext->m_InternalOptions.KernelDebugEnable;
            ForceNonCoherentStatelessBti = ClContext->m_ShouldUseNonCoherentStatelessBTI;
            AllowSpill = !ClContext->m_InternalOptions.NoSpill;

            if (ClContext->m_InternalOptions.GTPinReRA)
            {
                SaveOption(vISA_GTPinReRA, true);
                SaveOption(vISA_ReRAPostSchedule, true);
            }
            if (ClContext->m_InternalOptions.GTPinGRFInfo)
            {
                SaveOption(vISA_GetFreeGRFInfo, true);
            }
            if (ClContext->m_InternalOptions.GTPinScratchAreaSize)
            {
                SaveOption(vISA_GTPinScratchAreaSize, ClContext->m_InternalOptions.GTPinScratchAreaSizeValue);
            }
        }

        bool EnableBarrierInstCounterBits = false;
        if (context->type == ShaderType::HULL_SHADER)
        {
            EnableBarrierInstCounterBits = true;
        }
        bool preserveR0 = false;
        if (context->type == ShaderType::PIXEL_SHADER)
        {
            preserveR0 = !static_cast<CPixelShader*>(m_program)->IsLastPhase();
        }
        bool isOptDisabled = context->getModuleMetaData()->compOpt.OptDisable;

        // Set up options. This must be done before creating any variable/instructions
        // since some of the options affect IR building.
        if (IGC_IS_FLAG_ENABLED(ForceNoFP64bRegioning))
        {
            SaveOption(vISA_forceNoFP64bRegioning, true);
        }

        if (IGC_IS_FLAG_ENABLED(DumpCompilerStats) || context->getModuleMetaData()->compOpt.CaptureCompilerStats)
        {
            SaveOption(vISA_EnableCompilerStats, true);
        }

        if (IGC_IS_FLAG_ENABLED(EnableSamplerSplit))
        {
            SaveOption(vISA_enableCloneSampleInst, true);
        }


        if (m_program->m_Platform->getWATable().Wa_14012760189 && IGC_IS_FLAG_ENABLED(EnableEvaluateSamplerSplit))
        {
            SaveOption(vISA_cloneEvaluateSampleInst, true);
        }

        if (IGC_IS_FLAG_ENABLED(ForceFFIDOverwrite)/*|| m_program->m_Platform->WaOverwriteFFID()*/)
        {
            unsigned int ffid[unsigned(ShaderType::END)] = {
                0,
                static_cast<unsigned>(context->isPOSH() ? FFID_VSR : FFID_VS),
                FFID_HS,
                FFID_DS,
                FFID_GS,
                FFID_PS,
                FFID_GP,
                FFID_GP
            };
            SaveOption(vISA_setFFID, ffid[unsigned(context->type)]);
        }

        SaveOption(vISA_hasRNEandDenorm, true);

        // need to fold ret into the previous RTWrite/URBWrite/etc
        if (context->type != ShaderType::OPENCL_SHADER && context->type != ShaderType::COMPUTE_SHADER)
        {
            {
                SaveOption(vISA_foldEOTtoPrevSend, true);
            }
        }

        if (m_program->m_DriverInfo->clearScratchWriteBeforeEOT() &&
            (context->type == ShaderType::PIXEL_SHADER || context->type == ShaderType::OPENCL_SHADER))
        {
            SaveOption(vISA_clearScratchWritesBeforeEOT, true);
        }

        bool clearHDCWritesBeforeEOT = m_program->m_DriverInfo->UsesSparseAliasedResidency() &&
            context->platform.WaInsertHDCFenceBeforeEOTWhenSparseAliasedResources();
        clearHDCWritesBeforeEOT |= ((context->type == ShaderType::PIXEL_SHADER) ||
            (context->type == ShaderType::COMPUTE_SHADER) ||
            (context->type == ShaderType::OPENCL_SHADER)) &&
            context->platform.NeedsHDCFenceBeforeEOTInPixelShader();
        clearHDCWritesBeforeEOT |= IGC_IS_FLAG_ENABLED(ForceMemoryFenceBeforeEOT);

        if (clearHDCWritesBeforeEOT)
        {
            SaveOption(vISA_clearHDCWritesBeforeEOT, true);
        }


        // Disable multi-threaded latencies in the vISA scheduler when not in 3D
        if (context->type == ShaderType::OPENCL_SHADER)
        {
            if (m_program->m_Platform->singleThreadBasedInstScheduling())
            {
                SaveOption(vISA_useMultiThreadedLatencies, false);
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
            bool enableForRetey = m_program->m_DriverInfo->enableVISAPreRASchedulerForRetry() ||
                context->m_retryManager.AllowVISAPreRAScheduler();

            if (IGC_IS_FLAG_ENABLED(EnableVISAPreSched) &&
                m_program->m_DriverInfo->enableVISAPreRAScheduler() &&
                enableForRetey)
                return true;

            return false;
        };

        if (enableScheduler())
        {
            SaveOption(vISA_preRA_Schedule, true);
            if (uint32_t Val = IGC_GET_FLAG_VALUE(VISAPreSchedCtrl))
            {
                SaveOption(vISA_preRA_ScheduleCtrl, Val);
            }
            else
            {
                uint32_t V = m_program->m_DriverInfo->getVISAPreRASchedulerCtrl();
                if (m_program->GetHasDPAS())
                {
                    V = 4; // register pressure only
                }
                SaveOption(vISA_preRA_ScheduleCtrl, V);
            }

            uint32_t VISAPreSchedVal = 0;
            if (context->type == ShaderType::COMPUTE_SHADER)
                VISAPreSchedVal = context->getModuleMetaData()->csInfo.VISAPreSchedRPThreshold;
            else if (context->type == ShaderType::PIXEL_SHADER)
                VISAPreSchedVal = context->getModuleMetaData()->compOpt.VISAPreSchedRPThreshold;
            // registry key setting has higher priority
            if (uint32_t Val = IGC_GET_FLAG_VALUE(VISAPreSchedRPThreshold))
            {
                SaveOption(vISA_preRA_ScheduleRPThreshold, Val);
            }
            else if (VISAPreSchedVal)
            {
                SaveOption(vISA_preRA_ScheduleRPThreshold, VISAPreSchedVal);
            }

            if (uint32_t Val = IGC_GET_FLAG_VALUE(VISAScheduleStartBBID))
            {
                SaveOption(vISA_ScheduleStartBBID, Val);
            }

            if (uint32_t Val = IGC_GET_FLAG_VALUE(VISAScheduleEndBBID))
            {
                SaveOption(vISA_ScheduleEndBBID, Val);
            }
        }
        else
        {
            SaveOption(vISA_preRA_Schedule, false);
        }

        if (IGC_IS_FLAG_ENABLED(ReplaceIndirectCallWithJmpi))
        {
            SaveOption(vISA_replaceIndirectCallWithJmpi, true);
        }

        if (IGC_IS_FLAG_ENABLED(FastSpill))
        {
            SaveOption(vISA_FastSpill, true);
        }

#ifdef _DEBUG
        // enable vISA verifier if we are generating vISA IR
        SaveOption(vISA_NoVerifyvISA, !enableVISA_IR);
#else
        SaveOption(vISA_NoVerifyvISA, true);
#endif

        if (context->m_instrTypes.hasDebugInfo)
        {
            SaveOption(vISA_GenerateDebugInfo, true);

            if (context->metrics.Enable())
            {
                SaveOption(vISA_GenerateKernelInfo, true);
                SaveOption(vISA_EmitLocation, true);
            }
        }

        if (canAbortOnSpill)
        {
            SaveOption(vISA_AbortOnSpill, true);
            if (AvoidRetryOnSmallSpill())
            {
                // 2 means #spill/fill is roughly 1% of #inst
                // ToDo: tune the threshold
                if (m_program->m_dispatchSize == SIMDMode::SIMD8)
                    SaveOption(vISA_AbortOnSpillThreshold, IGC_GET_FLAG_VALUE(SIMD8_SpillThreshold) * 2);

                else if (m_program->m_dispatchSize == SIMDMode::SIMD16)
                    SaveOption(vISA_AbortOnSpillThreshold, IGC_GET_FLAG_VALUE(SIMD16_SpillThreshold) * 2);
            }
        }

        if (context->type == ShaderType::OPENCL_SHADER && m_program->m_dispatchSize == SIMDMode::SIMD8)
        {
            // AllowSpill is set to false if -cl-intel-no-spill internal option was passed from OpenCL Runtime.
            // It has been implemented to avoid scratch space usage for scheduler kernel.
            if (AllowSpill)
            {
                SaveOption(vISA_AbortOnSpillThreshold, IGC_GET_FLAG_VALUE(SIMD8_SpillThreshold) * 2);
            }
        }

        if ((context->type == ShaderType::OPENCL_SHADER || context->type == ShaderType::COMPUTE_SHADER) &&
            m_program->m_Platform->preemptionSupported() && IGC_IS_FLAG_ENABLED(EnablePreemption))
        {
            SaveOption(vISA_enablePreemption, true);
        }

        if (IGC_IS_FLAG_ENABLED(forceGlobalRA))
        {
            SaveOption(vISA_LocalRA, false);
            SaveOption(vISA_LocalBankConflictReduction, false);
        }

        if (IGC_IS_FLAG_ENABLED(disableVarSplit))
        {
            SaveOption(vISA_LocalDeclareSplitInGlobalRA, false);
        }

        if (IGC_IS_FLAG_ENABLED(disableRemat))
        {
            SaveOption(vISA_NoRemat, true);
        }

        if (ForceNonCoherentStatelessBti || IGC_IS_FLAG_ENABLED(ForceNonCoherentStatelessBTI))
        {
            SaveOption(vISA_noncoherentStateless, true);
        }

        if (IGC_IS_FLAG_ENABLED(DisableIfCvt))
        {
            SaveOption(vISA_ifCvt, false);
        }

        if (IGC_IS_FLAG_ENABLED(EnableVISAStructurizer) &&
            (m_program->m_Platform->hasSCF() || IGC_IS_FLAG_ENABLED(ForceVISAStructurizer)))
        {
            SaveOption(vISA_EnableStructurizer, true);

            if (IGC_GET_FLAG_VALUE(EnableVISAStructurizer) == FLAG_SCF_UCFOnly)
            {
                // visa structurizer will generate UCF only.
                SaveOption(vISA_StructurizerCF, false);
            }
        }

        if (IGC_IS_FLAG_DISABLED(EnableVISAJmpi))
        {
            SaveOption(vISA_EnableScalarJmp, false);
        }

        if (IGC_IS_FLAG_ENABLED(ForceNoMaskWA)) {
            SaveOption(vISA_forceNoMaskWA, true);
            // Turn off jmpi as there is no wa for jmpi
            SaveOption(vISA_EnableScalarJmp, false);
        }

        if (m_program->m_Platform->getWATable().Wa_1808850743 ||
            m_program->m_Platform->getWATable().Wa_1409909237)
        {
            uint32_t nomask_val = IGC_GET_FLAG_VALUE(NoMaskWA);
            if (context->type == ShaderType::OPENCL_SHADER)
            {
                auto ClContext = static_cast<OpenCLProgramContext*>(context);
                if (ClContext->m_InternalOptions.DisableNoMaskWA)
                {
                    nomask_val = 0;
                }
            }
            SaveOption(vISA_noMaskWA, nomask_val);
            if (nomask_val > 0)
            {
                // Turn off jmpi as there is no wa for jmpi
                SaveOption(vISA_EnableScalarJmp, false);
            }
        }

        if (m_program->m_Platform->hasFusedEU()
            && IGC_IS_FLAG_ENABLED(EnableCallWA)
            && (m_program->HasStackCalls() || m_program->IsIntelSymbolTableVoidProgram()))
        {
            SaveOption(vISA_fusedCallWA, true);
        }

        if (context->type == ShaderType::OPENCL_SHADER)
        {
            auto ClContext = static_cast<OpenCLProgramContext*>(context);
            if (ClContext->m_InternalOptions.IgnoreBFRounding)
            {
                SaveOption(vISA_ignoreBFRounding, true);
            }
        }

        if (IGC_IS_FLAG_ENABLED(DisableCSEL))
        {
            SaveOption(vISA_enableCSEL, false);
        }
        if (IGC_IS_FLAG_ENABLED(DisableFlagOpt))
        {
            SaveOption(vISA_LocalFlagOpt, false);
        }

        if (IGC_IS_FLAG_ENABLED(EnableVISAOutput))
        {
            SaveOption(vISA_outputToFile, true);
            m_enableVISAdump = true;
        }
        if (IGC_IS_FLAG_ENABLED(EnableVISABinary))
        {
            SaveOption(vISA_GenerateBinary, true);
            m_enableVISAdump = true;
        }
        if (IGC_IS_FLAG_ENABLED(EnableVISADumpCommonISA))
        {
            SaveOption(vISA_DumpvISA, true);
            SaveOption(vISA_GenerateISAASM, true);
            m_enableVISAdump = true;
        }
        if (IGC_IS_FLAG_ENABLED(EnableVISANoSchedule))
        {
            SaveOption(vISA_LocalScheduling, false);
        }
        if (IGC_IS_FLAG_ENABLED(EnableVISANoBXMLEncoder))
        {
            SaveOption(vISA_BXMLEncoder, false);
        }
        if (IGC_IS_FLAG_ENABLED(DisableMixMode) ||
            context->getModuleMetaData()->disableMixMode)
        {
            SaveOption(vISA_DisableMixMode, true);
        }
        if (IGC_IS_FLAG_ENABLED(ForceMixMode))
        {
            SaveOption(vISA_ForceMixMode, true);
        }
        if (IGC_IS_FLAG_ENABLED(DisableHFMath))
        {
            SaveOption(vISA_DisableHFMath, true);
        }

        if (IGC_IS_FLAG_ENABLED(disableIGASyntax))
        {
            SaveOption(vISA_dumpNewSyntax, false);
        }
        if (IGC_IS_FLAG_ENABLED(disableCompaction))
        {
            SaveOption(vISA_Compaction, false);
        }

        if (auto *regex = IGC_GET_REGKEYSTRING(ShaderDumpFilter))
        {
            SaveOption(vISA_ShaderDumpFilter, regex);
        }

        // In Vulkan and OGL buffer variable memory reads and writes within
        // a single shader invocation must be processed in order.
        if (m_program->m_DriverInfo->DisableDpSendReordering())
        {
            SaveOption(vISA_ReorderDPSendToDifferentBti, false);
        }

        if (m_program->m_DriverInfo->UseALTMode())
        {
            SaveOption(vISA_ChangeMoveType, false);
            SaveOption(vISA_ALTMode, true);
        }

        if (IGC_IS_FLAG_ENABLED(DisableSendS))
        {
            SaveOption(vISA_UseSends, false);
        }
        if (m_program->m_DriverInfo->AllowUnsafeHalf())
        {
            SaveOption(vISA_enableUnsafeCP_DF, true);
        }

        if (IGC_GET_FLAG_VALUE(ReservedRegisterNum) != 0 && (IGC_GET_FLAG_VALUE(TotalGRFNum) != 0))
        {
            IGC_ASSERT_MESSAGE(0, "ReservedRegisterNum and TotalGRFNum registry keys cannot be used at the same time");
        }

        if (IGC_GET_FLAG_VALUE(ReservedRegisterNum) != 0)
        {
            SaveOption(vISA_ReservedGRFNum, IGC_GET_FLAG_VALUE(ReservedRegisterNum));
        }
        if (IGC_GET_FLAG_VALUE(GRFNumToUse) > 0)
        {
            SaveOption(vISA_GRFNumToUse, IGC_GET_FLAG_VALUE(GRFNumToUse));
        }

        if (IGC_GET_FLAG_VALUE(TotalGRFNum) != 0)
        {
            SaveOption(vISA_TotalGRFNum, IGC_GET_FLAG_VALUE(TotalGRFNum));
        }
        else if (context->type == ShaderType::COMPUTE_SHADER && IGC_GET_FLAG_VALUE(TotalGRFNum4CS) != 0)
        {
            SaveOption(vISA_TotalGRFNum, IGC_GET_FLAG_VALUE(TotalGRFNum4CS));
        }
        else
        {
            SaveOption(vISA_TotalGRFNum, context->getNumGRFPerThread());
        }

        //
        // Setting number of GRF and threads per EU is restricted to OCL only
        // Number of threads can be set by:
        //  1. User input through
        //    1.1 compiler option for entire module
        //    1.2 kernel annotation for a specific kernel function
        //  2. Compiler heuristics
        //
        if (context->type == ShaderType::OPENCL_SHADER)
        {
            auto ClContext = static_cast<OpenCLProgramContext*>(context);
            if (m_program->m_Platform->supportsStaticRegSharing())
            {
                if (ClContext->getNumThreadsPerEU() > 0)
                {
                    // Number of threads per EU is set per module (by compiler option)
                    SaveOption(vISA_HWThreadNumberPerEU, ClContext->getNumThreadsPerEU());
                }
                else if (m_program->getAnnotatedNumThreads() > 0)
                {
                    // Number of threads per EU is set per kernel function (by user annotation)
                    SaveOption(vISA_HWThreadNumberPerEU, m_program->getAnnotatedNumThreads());
                }
                else if (m_program->m_Platform->supportsAutoGRFSelection() &&
                    context->m_DriverInfo.supportsAutoGRFSelection() &&
                    !IGC_IS_FLAG_ENABLED(DisableRegSharingHeuristics) &&
                    !ClContext->m_InternalOptions.Intel128GRFPerThread &&
                    !ClContext->m_InternalOptions.Intel256GRFPerThread)
                {
                    // When user hasn't specified number of threads, we can rely on compiler heuristics
                    SaveOption(vISA_RegSharingHeuristics, true);
                }
            }
        }
        if (IGC_GET_FLAG_VALUE(ForceHWThreadNumberPerEU) != 0)
        {
            SaveOption(vISA_ForceHWThreadNumberPerEU, IGC_GET_FLAG_VALUE(ForceHWThreadNumberPerEU));
        }

        if (IGC_IS_FLAG_ENABLED(EnableHashMovsAtPrologue))
        {
            SaveOption(vISA_HashMovsAtPrologue, true);
        }

        if (IGC_IS_FLAG_ENABLED(SystemThreadEnable))
        {
            /* Some tools only use 32bits hash, to maintain compatibility
            across lot of unknown tool chains doing Compare for only LowerPart
            */
            if (IGC_GET_FLAG_VALUE(ShaderDebugHashCode) == (DWORD)context->hash.getAsmHash())
            {
                SaveOption(vISA_setStartBreakPoint, true);
            }
        }
        else if (KernelDebugEnable)
        {
            SaveOption(vISA_AddKernelID, true);
            SaveOption(vISA_setStartBreakPoint, true);
        }

        auto g4Subset = (uint32_t)IGC_GET_FLAG_VALUE(ShaderDumpEnableG4);
        if (g4Subset != 0)
            SaveOption(vISA_DumpPassesSubset, g4Subset);

        if (EnableBarrierInstCounterBits)
        {
            SaveOption(VISA_EnableBarrierInstCounterBits, true);
        }
        if (preserveR0)
        {
            SaveOption(vISA_ReserveR0, true);
        }
        if (IGC_IS_FLAG_ENABLED(InitializeRegistersEnable))
        {
            SaveOption(vISA_InitPayload, true);
        }
        if (IGC_IS_FLAG_ENABLED(UseOldSubRoutineAugIntf))
        {
            SaveOption(vISA_UseOldSubRoutineAugIntf, true);
        }
        if (IGC_IS_FLAG_ENABLED(FastCompileRA) && !hasStackCall)
        {
            SaveOption(vISA_FastCompileRA, true);
        }
        if (IGC_IS_FLAG_ENABLED(HybridRAWithSpill) && !hasStackCall)
        {
            SaveOption(vISA_HybridRAWithSpill, true);
        }
        if (IGC_IS_FLAG_ENABLED(DumpPayloadToScratch))
        {
            SaveOption(vISA_dumpPayload, true);
        }
        if (IGC_IS_FLAG_ENABLED(ExpandPlane))
        {
            SaveOption(vISA_expandPlane, true);
        }
        if (IGC_IS_FLAG_ENABLED(EnableBCR))
        {
            SaveOption(vISA_enableBCR, true);
        }
        if (IGC_IS_FLAG_ENABLED(ForceBCR))
        {
            SaveOption(vISA_forceBCR, true);
        }
        if (IGC_IS_FLAG_ENABLED(forceSamplerHeader))
        {
            SaveOption(vISA_forceSamplerHeader, true);
        }
        if (IGC_IS_FLAG_ENABLED(EnableIGAEncoder))
        {
            SaveOption(vISA_IGAEncoder, true);
        }
        else
        {
            SaveOption(vISA_IGAEncoder, false);
        }

        if (IGC_IS_FLAG_ENABLED(SetA0toTdrForSendc))
        {
            SaveOption(vISA_setA0toTdrForSendc, true);
        }

        if (IGC_IS_FLAG_ENABLED(AvoidDstSrcGRFOverlap))
        {
            SaveOption(vISA_DstSrcOverlapWA, true);
        }

        if (IGC_IS_FLAG_ENABLED(AvoidSrc1Src2Overlap))
        {
            SaveOption(vISA_Src1Src2OverlapWA, true);
        }

        if (IGC_IS_FLAG_ENABLED(UseLinearScanRA))
        {
            SaveOption(vISA_LinearScan, true);
        }

        if (IGC_IS_FLAG_ENABLED(EnableIGASWSB))
        {
            SaveOption(vISA_EnableIGASWSB, true);
        }

        if (IGC_IS_FLAG_ENABLED(EnableQuickTokenAlloc))
        {
            SaveOption(vISA_QuickTokenAllocation, true);
        }

        if (IGC_IS_FLAG_ENABLED(EnableSWSBStitch) ||
            (context->type == ShaderType::PIXEL_SHADER &&
             static_cast<CPixelShader*>(m_program)->GetPhase() == PSPHASE_PIXEL))
        {
            SaveOption(vISA_SWSBStitch, true);
        }

        if (IGC_IS_FLAG_ENABLED(DisableRegDistDep))
        {
            SaveOption(vISA_disableRegDistDep, true);
        }

        if (IGC_IS_FLAG_ENABLED(EnableForceDebugSWSB) ||
            IGC_IS_FLAG_ENABLED(EnableSWSBInstStall) ||
            IGC_IS_FLAG_ENABLED(EnableSWSBTokenBarrier))
        {
            if (IGC_IS_FLAG_ENABLED(EnableSWSBInstStall))
            {
                SaveOption(vISA_SWSBInstStall, IGC_GET_FLAG_VALUE(EnableSWSBInstStall));
                SaveOption(vISA_SWSBInstStallEnd, IGC_GET_FLAG_VALUE(EnableSWSBInstStallEnd));
            }

            if (IGC_IS_FLAG_ENABLED(EnableSWSBTokenBarrier))
            {
                SaveOption(vISA_SWSBTokenBarrier, IGC_GET_FLAG_VALUE(EnableSWSBTokenBarrier));
            }

            if (IGC_IS_FLAG_ENABLED(EnableForceDebugSWSB))
            {
                SaveOption(vISA_forceDebugSWSB, true);
            }
            SaveOption(vISA_Compaction, false);
        }

        if (IGC_IS_FLAG_ENABLED(EnableGatherWithImm))
        {
            SaveOption(vISA_EnableGatherWithImm, true);
        }

        if (IGC_IS_FLAG_ENABLED(EnableGroupScheduleForBC))
        {
            SaveOption(vISA_EnableGroupScheduleForBC, true);
        }

        if (VISAPlatform == Xe_XeHPSDV && IGC_IS_FLAG_ENABLED(DPASTokenReduction))
        {
            SaveOption(vISA_EnableDPASTokenReduction, true);
        }

        if (IGC_IS_FLAG_ENABLED(DisableThreeALUPipes))
        {
            SaveOption(vISA_EnableALUThreePipes, false);
        }

        SaveOption(vISA_useInlineData, m_program->passNOSInlineData());

        if (m_program->m_Platform->supportLoadThreadPayloadForCompute())
        {
            SaveOption(vISA_loadThreadPayload, m_program->loadThreadPayload());
        }
        else
        {
            SaveOption(vISA_loadThreadPayload, false);
        }



        if (IGC_IS_FLAG_ENABLED(EnablerReadSuppressionWA) &&
            VISAPlatform >= GENX_TGLLP)
        {
            SaveOption(vISA_InsertDummyMovForHWRSWA, true);
            if (IGC_IS_FLAG_ENABLED(DPASReadSuppressionWA))
            {
                SaveOption(vISA_InsertDummyMovForDPASRSWA, true);
            }
            if (IGC_GET_FLAG_VALUE(RSWARegNum) != 0)
            {
                SaveOption(vISA_registerHWRSWA, IGC_GET_FLAG_VALUE(RSWARegNum));
            }
        }

        if (IGC_GET_FLAG_VALUE(SWSBTokenNum) != 0)
        {
            SaveOption(vISA_SWSBTokenNum, IGC_GET_FLAG_VALUE(SWSBTokenNum));
        }

        if (IGC_IS_FLAG_ENABLED(EnableAccSub))
        {
            SaveOption(vISA_accSubstitution, true);
            uint32_t numAcc = IGC_GET_FLAG_VALUE(NumGeneralAcc);

            IGC_ASSERT_MESSAGE(0 <= numAcc, "number of general acc should be [1-16] if set");
            IGC_ASSERT_MESSAGE(numAcc <= 16, "number of general acc should be [1-16] if set");

            if (numAcc > 0)
            {
                SaveOption(vISA_numGeneralAcc, numAcc);
            }

            if (IGC_IS_FLAG_ENABLED(HasDoubleAcc))
            {
                SaveOption(vISA_hasDoubleAcc, true);
            }
        }
        else
        {
            SaveOption(vISA_accSubstitution, false);
        }

        if (IGC_IS_FLAG_ENABLED(GlobalSendVarSplit))
        {
            SaveOption(vISA_GlobalSendVarSplit, true);
        }

        if (m_program->m_Platform->canFuseTypedWrite())
        {
            SaveOption(vISA_FuseTypedWrites, true);
        }

        if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable) && IGC_IS_FLAG_ENABLED(InterleaveSourceShader))
        {
            SaveOption(vISA_EmitLocation, true);
        }

        if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable))
        {
            SaveOption(vISA_SBIDDepLoc, true);
        }

        // Enable SendFusion for SIMD8
        // TODO: Re-enable SendFusion when VMask is enabled. The hardware should support this, but
        //  more investigation needs to be done on whether simply replacing sr0.2 with sr0.3 is enough.
        if (IGC_IS_FLAG_ENABLED(EnableSendFusion) &&
            !(context->type == ShaderType::PIXEL_SHADER && static_cast<CPixelShader*>(m_program)->NeedVMask()) &&
            m_program->GetContext()->platform.supportSplitSend() &&
            m_program->m_dispatchSize == SIMDMode::SIMD8 &&
            (IGC_GET_FLAG_VALUE(EnableSendFusion) == FLAG_LEVEL_2 ||   // 2: force send fusion
                context->m_DriverInfo.AllowSendFusion()))
        {
            SaveOption(vISA_EnableSendFusion, true);
            if (IGC_IS_FLAG_ENABLED(EnableAtomicFusion) &&
                context->type == ShaderType::OPENCL_SHADER)
            {
                SaveOption(vISA_EnableAtomicFusion, true);
            }
        }

        // With StatelessToStateful on, it is possible that two different BTI messages
        // (two kernel arguments) might refer to the same memory. To be safe, turn off
        // visa DPSend reordering.
        if (IGC_IS_FLAG_ENABLED(EnableStatelessToStateful) &&
            context->type == ShaderType::OPENCL_SHADER)
        {
            SaveOption(vISA_ReorderDPSendToDifferentBti, false);
        }

        if (m_program->m_Platform->WaDisableSendSrcDstOverlap())
        {
            SaveOption(vISA_noSendSrcDstOverlap, true);
        }

        if (m_program->m_Platform->WaDisableSendSrcDstOverlap())
        {
            SaveOption(vISA_noSendSrcDstOverlap, true);
        }

        // Set to stitch all functions to all kernels in a VISABuidler
        SaveOption(vISA_noStitchExternFunc, false);

        // Turning off optimizations as much as possible to have the fastest compilation
        if ((IsStage1FastestCompile(context->m_CgFlag, context->m_StagingCtx) ||
             IGC_GET_FLAG_VALUE(ForceFastestSIMD)) &&
            (m_program->m_DriverInfo->SupportFastestStage1() ||
             IGC_IS_FLAG_ENABLED(EnableFastestForVulkan)))
        {
            if (IGC_GET_FLAG_VALUE(FastestS1Experiments) == FCEXP_NO_EXPRIMENT)
            {
                SaveOption(vISA_LocalScheduling, false);
                SaveOption(vISA_preRA_Schedule, false);
                SaveOption(vISA_SpillSpaceCompression, false);
                SaveOption(vISA_LVN, false);
                SaveOption(vISA_QuickTokenAllocation, true);
                if (IGC_IS_FLAG_DISABLED(FastestWALinearScanForCS) ||
                    context->type != ShaderType::COMPUTE_SHADER)
                {
                    SaveOption(vISA_LinearScan, true);
                }
            }
            else
            {
                if (IGC_GET_FLAG_VALUE(FastestS1Experiments) & FCEXP_FASTSPILL)
                    SaveOption(vISA_FastSpill, true);

                if (IGC_GET_FLAG_VALUE(FastestS1Experiments) & FCEXP_LOCAL_SCHEDULING)
                    SaveOption(vISA_LocalScheduling, false);

                if (IGC_GET_FLAG_VALUE(FastestS1Experiments) & FCEXP_PRERA_SCHEDULING)
                    SaveOption(vISA_preRA_Schedule, false);

                if (IGC_GET_FLAG_VALUE(FastestS1Experiments) & FCEXP_NO_REMAT)
                    SaveOption(vISA_NoRemat, true);

                if (IGC_GET_FLAG_VALUE(FastestS1Experiments) & FCEXP_SPILL_COMPRESSION)
                    SaveOption(vISA_SpillSpaceCompression, false);

                if (IGC_GET_FLAG_VALUE(FastestS1Experiments) & FCEXP_LOCAL_DECL_SPLIT_GLOBAL_RA)
                    SaveOption(vISA_LocalDeclareSplitInGlobalRA, false);

                if (IGC_GET_FLAG_VALUE(FastestS1Experiments) & FCEXP_DISABLE_LVN)
                    SaveOption(vISA_LVN, false);
                if (IGC_GET_FLAG_VALUE(FastestS1Experiments) & FCEXP_QUICKTOKEN_ALLOC)
                    SaveOption(vISA_QuickTokenAllocation, true);
                if (((IGC_IS_FLAG_DISABLED(FastestWALinearScanForCS) ||
                     context->type != ShaderType::COMPUTE_SHADER)) &&
                    (IGC_GET_FLAG_VALUE(FastestS1Experiments) & FCEXP_LINEARSCAN))
                    SaveOption(vISA_LinearScan, true); // use linearScan

                if (IGC_GET_FLAG_VALUE(FastestS1Experiments) & FCEXP_1PASSRA)
                    SaveOption(vISA_FastCompileRA, true); // use 1 iteration RA
            }
        }

    } // InitVISABuilderOptions

    // Get a unqiue label for inline asm instruction blocks at the module level.
    // For each call to asm("..."), user can input the "%=" string format to generate a unique label for that call.
    // In this case we would generate "__4_000" for the 1st usage of "%=" in an asm block in the 5th function of the module.
    std::string CEncoder::GetUniqueInlineAsmLabel()
    {
        std::stringstream ss;
        ss << GetCompilerLabelPrefix() << labelFunctionIndex << "_" <<
            std::setw(3) << std::setfill('0') << labelInlineAsmCounter++;
        return ss.str();
    }

    // Creates a module/program-unique label prefix.
    // E.g. the 3rd label of the 5th function would be
    // "__4_002".  Ugly, yes, but you shouldn't see it as this is the
    // fallback case.  Short, unique, and debuggable....
    // Release-internal/debug will have better names.
    std::string CEncoder::CreateShortLabel(unsigned labelIndex) const
    {
        std::stringstream ss;
        ss << GetCompilerLabelPrefix() << labelFunctionIndex << "_" <<
            std::setw(3) << std::setfill('0') << labelIndex;
        return ss.str();
    }

    // Converts an LLVM label L into a name appropriate for vISA's label rules
    //  * remove illegal chracters for vISA
    //  * contrains the length while maintaining uniqueness
    // The format is something that contains both function index and the
    // label name passed in.
    //
    // If enabled the output will be:
    //  _[FUNCTION-INDEX]_[LABEL-INDEX](_[LLVM-NAME])?
    // i.e. if the LLVM name is empty we omit that whole suffix
    CName CEncoder::CreateVisaLabelName(const llvm::StringRef &L)
    {
#ifndef IGC_MAP_LLVM_NAMES_TO_VISA
        return CreateShortLabel(labelCounter++);
#else // IGC_MAP_LLVM_NAMES_TO_VISA
        static const size_t MAX_LLVM_NAME = 250;

        auto sanitizeChar = [](char c) {
            return isalnum(c) || c == '_' ? c : '_';
        };

        // The vISA backend constrains this to around 256 characters.
        // (1) Function names can be extremely long (currFunctionName).
        //     DPC++ with template gunk can be hundreds of characters.
        //     If the names are too long, punt and use a function index.
        //     Functions cannot be integers, thus the function part cannot
        //     collide if we use this replacement.
        // (2) LLVM labels (L) can be extremely long. E.g. LLVM chains
        //     together names synthetically and can get to >900 chars.
        //     In this case, we prefix a label index and suffix as much of
        //     the LLVM label on as possible.
        std::stringstream lbl;
        lbl << GetCompilerLabelPrefix();
        if (!currFunctionName.empty() && currFunctionName.size() < 128) {
            const char *s = currFunctionName.getVisaCString();
            while (*s)
                lbl << sanitizeChar(*s++);
        } else {
            lbl << std::setw(2) << std::setfill('0') << labelFunctionIndex;
        }
        // since the label name could be the empty string, and to keep things
        // simple, we unconditionally use the label counter (and increment it)
        lbl << "_" << std::setw(3) << std::setfill('0') <<
            labelCounter++;

        size_t charsLeft = MAX_LLVM_NAME - (size_t)lbl.tellp();
        size_t nLeft = std::min(charsLeft, L.size());
        if (L.size() > 0 && nLeft > 0) {
            // if not the empty string then add a separator
            lbl << "_";
            nLeft--;
        }
        // suffix as many characters of the label as we can
        for (size_t i = 0; i < nLeft; i++) {
            lbl << sanitizeChar(L[i]);
        }

        return lbl.str();
#endif // IGC_MAP_LLVM_NAMES_TO_VISA
    }

    void CEncoder::InitLabelMap(const llvm::Function* F)
    {
        labelMap.clear();
        labelMap.resize(F->size(), nullptr);
        labelCounter = 0;
        labelInlineAsmCounter = 0;
        labelFunctionIndex++;
        currFunctionName = F->getName();
        labelNameMap.clear();
        labelNameMap.reserve(F->size());
        for (auto BI = F->begin(), BE = F->end(); BI != BE; BI++)
        {
            labelNameMap.emplace_back(CreateVisaLabelName(BI->getName()));
        }
    }

    void CEncoder::InitEncoder(bool canAbortOnSpill, bool hasStackCall, bool hasInlineAsmCall, VISAKernel* prevKernel)
    {
        m_aliasesMap.clear();
        m_encoderState.m_SubSpanDestination = false;
        CodeGenContext* context = m_program->GetContext();
        m_encoderState.m_secondHalf = false;
        m_encoderState.m_secondNibble = false;
        m_enableVISAdump = false;
        m_nestLevelForcedNoMaskRegion = 0;
        m_hasInlineAsm = hasInlineAsmCall;

        InitLabelMap(m_program->entry);

        vbuilder = nullptr;
        vAsmTextBuilder = nullptr;
        TARGET_PLATFORM VISAPlatform = GetVISAPlatform(&(context->platform));

        SetVISAWaTable(m_program->m_Platform->getWATable());

        llvm::SmallVector<const char*, 10> params;
        llvm::SmallVector<std::unique_ptr< char, std::function<void(char*)>>, 10> params2;
        if (!m_hasInlineAsm)
        {
            // Asm text writer mode doesnt need dump params
            InitBuildParams(params2);
            for (size_t i = 0; i < params2.size(); i++)
            {
                params.push_back((params2[i].get()));
            }
        }

        COMPILER_TIME_START(m_program->GetContext(), TIME_CG_vISACompile);
        bool enableVISADump = IGC_IS_FLAG_ENABLED(EnableVISASlowpath) || IGC_IS_FLAG_ENABLED(ShaderDumpEnable);
        auto builderMode = m_hasInlineAsm ? vISA_ASM_WRITER : vISA_DEFAULT;
        auto builderOpt = (enableVISADump || m_hasInlineAsm) ? VISA_BUILDER_BOTH : VISA_BUILDER_GEN;
        V(CreateVISABuilder(vbuilder, builderMode, builderOpt, VISAPlatform, params.size(), params.data(),
            &m_vISAWaTable));

        if (IsCodePatchCandidate())
        {
            SetHasPrevKernel(prevKernel != nullptr);
        }
        InitVISABuilderOptions(VISAPlatform, canAbortOnSpill, hasStackCall, builderOpt == VISA_BUILDER_BOTH);

        // Pass all build options to builder
        SetBuilderOptions(vbuilder);

        vKernel = nullptr;

        std::string kernelName = std::string(m_program->entry->getName());
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
                            kernelName = second->getString().str();
                        }
                    }
                }
            }
        }

        std::string asmName;
        if (m_enableVISAdump || context->m_instrTypes.hasDebugInfo)
        {
            asmName = GetDumpFileName("asm");
        }
        else
        {
            kernelName = "kernel";
            asmName = "kernel.asm";
        }

        V(vbuilder->AddKernel(vKernel, kernelName.c_str()));
        V(vbuilder->SetPrevKernel(prevKernel));
        V(vKernel->AddKernelAttribute("OutputAsmPath", asmName.length(), asmName.c_str()));

        SetDispatchSimdSize();
        SetSpillMemOffset();

        vMainKernel = vKernel;

        auto gtpin_init = context->gtpin_init;
        if (gtpin_init)
        {
            vKernel->SetGTPinInit(gtpin_init);
        }

        // Right now only 1 main function in the kernel
        VISA_LabelOpnd* functionLabel = nullptr;
        V(vKernel->CreateVISALabelVar(functionLabel, "_main", LABEL_SUBROUTINE));
        V(vKernel->AppendVISACFLabelInst(functionLabel));

        V(vKernel->CreateVISASurfaceVar(dummySurface, "", 1));

        V(vKernel->CreateVISASamplerVar(samplervar, "", 1));

        // Set float denorm modes and rounding modes as default
        initCR(vKernel);
    }

    void CEncoder::SetDispatchSimdSize()
    {
        IGC_ASSERT(nullptr != vKernel);
        uint8_t dispatchSIMD = (uint8_t)numLanes(m_program->m_dispatchSize);
        V(vKernel->AddKernelAttribute("SimdSize", 1, &dispatchSIMD));
    }

    void CEncoder::SetSpillMemOffset()
    {
        IGC_ASSERT(nullptr != vKernel);
        uint scratchSpaceSizeTemp = m_program->m_ScratchSpaceSize;


        if (scratchSpaceSizeTemp > 0) {
            V(vKernel->AddKernelAttribute("SpillMemOffset", 4, &scratchSpaceSizeTemp));
        }
    }

    void CEncoder::SetStackFunctionArgSize(uint size)
    {
        uint8_t sz = (uint8_t)size;
        IGC_ASSERT(nullptr != vKernel);
        V(vKernel->AddKernelAttribute("ArgSize", 1, &sz));
    }

    void CEncoder::SetStackFunctionRetSize(uint size)
    {
        uint8_t sz = (uint8_t)size;
        IGC_ASSERT(nullptr != vKernel);
        V(vKernel->AddKernelAttribute("RetValSize", 1, &sz));
    }

    void CEncoder::SetExternFunctionFlag()
    {
        IGC_ASSERT(nullptr != vKernel);
        V(vKernel->AddKernelAttribute("Extern", 0, nullptr));
    }

    SEncoderState CEncoder::CopyEncoderState()
    {
        return m_encoderState;
    }

    void CEncoder::SetEncoderState(SEncoderState& newState)
    {
        m_encoderState = newState;
    }

    VISA_Align CEncoder:: GetVISAAlign(CVariable* var)
    {
        VISA_Align align;
        switch (var->GetAlign())
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
        case EALIGN_HWORD: align = ALIGN_HWORD;
            break;
        case EALIGN_32WORD: align = ALIGN_32WORD;
            break;
        case EALIGN_64WORD: align = ALIGN_64WORD;
            break;
        default:
            align = ALIGN_UNDEF;
            IGC_ASSERT(0);
            break;
        }
        return align;
    }

    VISA_GenVar* CEncoder::GetVISAVariable(CVariable* var)
    {
        if (m_encoderState.m_secondHalf)
        {
            if (var->GetNumberInstance() == 2)
            {
                return var->visaGenVariable[1];
            }
        }
        return var->visaGenVariable[0];
    }

    VISA_GenVar* CEncoder::GetVISAVariable(CVariable* var, e_instance instance)
    {
        VISA_GenVar* result = GetVISAVariable(var);

        if (instance != EINSTANCE_UNSPECIFIED &&
            var->GetNumberInstance() == 2)
        {
            if (instance == EINSTANCE_FIRST_HALF)
            {
                result = var->visaGenVariable[0];
            }
            else
            {
                result = var->visaGenVariable[1];
            }
        }
        return result;
    }

    void CEncoder::GetVISAPredefinedVar(CVariable* pVar, PreDefined_Vars var)
    {
        vKernel->GetPredefinedVar(pVar->visaGenVariable[0], var);
        switch (var) {
        case PREDEFINED_NULL:
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
            pVar->getVisaCString(),
            pVar->GetNumberElement(),
            pVar->GetType(),
            ALIGN_HWORD,
            pVar->visaGenVariable[0],
            pVar->GetAliasOffset());

        pVar->visaGenVariable[0] = pAliasGenVar;
    }

    void CEncoder::CreateVISAVar(CVariable* var)
    {
        IGC_ASSERT(nullptr != var);

        if (var->GetAlias() != NULL)
        {
            var->ResolveAlias();
            // In case the alias is an exact copy or just a sub variable just re-use the variable
            if (var->GetAlias()->GetType() == var->GetType())
            {
                for (uint i = 0; i < var->GetNumberInstance(); i++)
                {
                    var->visaGenVariable[i] = var->GetAlias()->visaGenVariable[i];
                }
            }
            else
            {
                SAlias alias(var->GetAlias(), var->GetType());
                auto aliasPair = m_aliasesMap.insert(std::pair<SAlias, CVariable*>(alias, var));
                if (aliasPair.second == false)
                {
                    for (uint i = 0; i < var->GetNumberInstance(); i++)
                    {
                        var->visaGenVariable[i] = aliasPair.first->second->visaGenVariable[i];
                    }
                }
                else
                {
                    IGC_ASSERT_MESSAGE(var->GetType() != ISA_TYPE_BOOL, "boolean cannot have alias");
                    for (uint i = 0; i < var->GetNumberInstance(); i++)
                    {
                        // Since we no longer use the built-in alias offset mechanism,
                        // we have to create the aliases to be of at least the size of the
                        // original variable (in bytes)
                        // Otherwise, we may end up a situation where we have an alias with
                        // an offset (m_aliasOffset, that we don't notify vISA about),
                        // and make an out-of-bounds access.
                        // This is the opposite of the calculation that happens in
                        // CVariable::CVariable.

                        const unsigned int denominator = CEncoder::GetCISADataTypeSize(var->GetType());
                        IGC_ASSERT(denominator);
                        uint16_t nbElement =
                            var->GetAlias()->GetNumberElement() *
                            CEncoder::GetCISADataTypeSize(var->GetAlias()->GetType()) /
                            denominator;

                        V(vKernel->CreateVISAGenVar(
                            var->visaGenVariable[i],
                            var->getVisaCString(),
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
            if (var->GetVarType() == EVARTYPE_GENERAL)
            {
                var->visaGenVariable[0] = nullptr;
                var->visaGenVariable[1] = nullptr;
                IGC_ASSERT_MESSAGE(var->GetType() != ISA_TYPE_BOOL, "boolean cannot be general var");
                for (uint i = 0; i < var->GetNumberInstance(); i++)
                {
                    V(vKernel->CreateVISAGenVar(
                        var->visaGenVariable[i],
                        var->getVisaCString(),
                        num_elts,
                        var->GetType(),
                        GetVISAAlign(var)));
                }
            }
            else if (var->GetVarType() == EVARTYPE_PREDICATE)
            {
                unsigned short nb = int_cast<unsigned short>(num_elts) * var->GetNumberInstance();
                V(vKernel->CreateVISAPredVar(
                    var->visaPredVariable,
                    "",
                    nb));
            }
            else
            {
                // when both array and index are uniform so is the destination address variable
                uint nb = (var->IsUniform() && var->IsVectorUniform()) ? 1 : var->GetNumberElement();
                V(vKernel->CreateVISAAddrVar(var->visaAddrVariable, "", nb));
            }
        }
    }

    void CEncoder::DeclareInput(CVariable* var, uint offset, uint instance)
    {
        // Avoid declaring more inputs/outputs than available registers
        if (offset + var->GetSize() >= vKernel->getNumRegTotal() * getGRFSize())
            return;
        V(vKernel->CreateVISAInputVar(
            var->visaGenVariable[instance],
            int_cast<unsigned short>(offset),
            int_cast<unsigned short>(var->GetSize())));
    }

    void CEncoder::MarkAsOutput(CVariable* var)
    {
        for (unsigned int i = 0; i < var->GetNumberInstance(); i++)
        {
            V(vKernel->AddAttributeToVar(var->visaGenVariable[i], "Output", 0, nullptr));
        }
    }

    void CEncoder::MarkAsPayloadLiveOut(CVariable* var)
    {
        for (unsigned int i = 0; i < var->GetNumberInstance(); i++)
        {
            V(vKernel->AddAttributeToVar(var->visaGenVariable[i], "PayloadLiveOut", 0, nullptr));
        }
    }

    bool CEncoder::AvoidRetryOnSmallSpill() const
    {
        CodeGenContext* context = m_program->GetContext();
        return context->type == ShaderType::PIXEL_SHADER &&
            (m_program->m_dispatchSize == SIMDMode::SIMD8 || m_program->m_dispatchSize == SIMDMode::SIMD16) &&
            context->m_retryManager.IsFirstTry();
    }

    void CEncoder::CreateKernelSymbol(const std::string& kernelName, unsigned offset,
        unsigned size, SProgramOutput::ZEBinFuncSymbolTable& symbols)
    {
        // kernel symbols are local symbols
        symbols.local.emplace_back(vISA::GenSymType::S_KERNEL, offset, size, kernelName);
    }

    void CEncoder::CreateSymbolTable(ValueToSymbolList& symbolTableList)
    {
        Module* pModule = m_program->GetContext()->getModule();
        ModuleMetaData* modMD = m_program->GetContext()->getModuleMetaData();

        for (auto& F : pModule->getFunctionList())
        {
            // Find all variant function declarations
            if (F.isDeclaration() && F.hasFnAttribute("variant-function-decl"))
            {
                // Parse the function name string
                auto [symStr, fName, vecLen] = IGC::ParseVectorVariantFunctionString(F.getName());

                Function* VFDef = pModule->getFunction(fName);
                if (VFDef && numLanes(m_program->m_dispatchSize) == vecLen)
                {
                    auto Iter = stackFuncMap.find(VFDef);
                    IGC_ASSERT_MESSAGE(Iter != stackFuncMap.end(), "vISA function not found");

                    vISA::GenSymEntry fEntry;
                    IGC_ASSERT(F.getName().size() <= vISA::MAX_SYMBOL_NAME_LENGTH);
                    strcpy_s(fEntry.s_name, vISA::MAX_SYMBOL_NAME_LENGTH, F.getName().str().c_str());

                    // Query vISA for the function's byte offset within the compiled module
                    // The actual binary offset data should point to the function definition
                    VISAFunction* visaFunc = Iter->second;
                    fEntry.s_type = vISA::GenSymType::S_FUNC;
                    fEntry.s_offset = (uint32_t)visaFunc->getGenOffset();
                    fEntry.s_size = (uint32_t)visaFunc->getGenSize();

                    symbolTableList.push_back(std::make_pair(&F, fEntry));
                }
            }
            // Ignore variant function definitions
            else if (F.hasFnAttribute("variant-function-def"))
            {
                IGC_ASSERT_MESSAGE(F.use_empty(), "This function should never be accessed directly");
                continue;
            }
            // Find all functions in the module we need to export as symbols
            else if (F.hasFnAttribute("referenced-indirectly") && (!F.isDeclaration() || !F.use_empty()))
            {
                vISA::GenSymEntry fEntry;
                IGC_ASSERT(F.getName().size() <= vISA::MAX_SYMBOL_NAME_LENGTH);
                strcpy_s(fEntry.s_name, vISA::MAX_SYMBOL_NAME_LENGTH, F.getName().str().c_str());

                bool isTrue = false;
                if (F.isDeclaration() || isTrue)
                {
                    // If the function is only declared, set as undefined type
                    fEntry.s_type = vISA::GenSymType::S_UNDEF;
                    fEntry.s_offset = 0;
                    fEntry.s_size = 0;
                }
                else
                {
                    auto Iter = stackFuncMap.find(&F);
                    IGC_ASSERT_MESSAGE(Iter != stackFuncMap.end(), "vISA function not found");

                    // Query vISA for the function's byte offset within the compiled module
                    VISAFunction* visaFunc = Iter->second;
                    fEntry.s_type = vISA::GenSymType::S_FUNC;
                    fEntry.s_offset = (uint32_t)visaFunc->getGenOffset();
                    fEntry.s_size = (uint32_t)visaFunc->getGenSize();
                }
                symbolTableList.push_back(std::make_pair(&F, fEntry));
            }
        }

        // Export global symbols
        for (auto global : modMD->inlineProgramScopeOffsets)
        {
            GlobalVariable* pGlobal = global.first;

            // Export the symbol if global is external/common linkage, or has uses in the module
            bool needSymbol = pGlobal->use_empty()
                ? (modMD->compOpt.EnableTakeGlobalAddress && (pGlobal->hasCommonLinkage() || pGlobal->hasExternalLinkage()))
                : true;

            if (needSymbol)
            {
                StringRef name = pGlobal->getName();
                unsigned addrSpace = pGlobal->getType()->getAddressSpace();
                IGC_ASSERT(name.size() <= vISA::MAX_SYMBOL_NAME_LENGTH);

                vISA::GenSymEntry sEntry;
                memset(sEntry.s_name, '0', vISA::MAX_SYMBOL_NAME_LENGTH);
                strcpy_s(sEntry.s_name, vISA::MAX_SYMBOL_NAME_LENGTH, name.str().c_str());
                MDNode* md = pGlobal->getMetadata("ConstSampler");
                if (md)
                {
                    // Constant Sampler: s_offset contains the sampler ID
                    sEntry.s_type = vISA::GenSymType::S_CONST_SAMPLER;
                    sEntry.s_size = 0;
                    sEntry.s_offset = static_cast<uint32_t>(global.second);
                }
                else
                {
                    sEntry.s_type = (addrSpace == ADDRESS_SPACE_GLOBAL) ? vISA::GenSymType::S_GLOBAL_VAR : vISA::GenSymType::S_GLOBAL_VAR_CONST;
                    sEntry.s_size = int_cast<uint32_t>(pModule->getDataLayout().getTypeAllocSize(pGlobal->getType()->getPointerElementType()));
                    sEntry.s_offset = static_cast<uint32_t>(global.second);
                }
                symbolTableList.push_back(std::make_pair(pGlobal, sEntry));
            }
        }
    }

    void CEncoder::CreateSymbolTable(void*& buffer, unsigned& bufferSize, unsigned& tableEntries)
    {
        buffer = nullptr;
        bufferSize = 0;
        tableEntries = 0;

        ValueToSymbolList symbolTableList;
        CreateSymbolTable(symbolTableList);

        // Get the data for patch token
        if (!symbolTableList.empty())
        {
            std::vector<vISA::GenSymEntry> tempBufferData;
            // Collect the data just for the symbol table entries
            for (auto I : symbolTableList)
            {
                auto symbolEntry = I.second;
                tempBufferData.push_back(symbolEntry);
            }

            tableEntries = tempBufferData.size();
            bufferSize = tableEntries * sizeof(vISA::GenSymEntry);
            buffer = malloc(bufferSize);
            IGC_ASSERT_MESSAGE(nullptr != buffer, "Symbol table cannot be allocated");
            memcpy_s(buffer, bufferSize, tempBufferData.data(), bufferSize);
        }
    }

    void CEncoder::CreateSymbolTable(SProgramOutput::ZEBinFuncSymbolTable& funcSyms,
        SOpenCLProgramInfo::ZEBinProgramSymbolTable& programSyms)
    {
        ValueToSymbolList symbolTableList;
        CreateSymbolTable(symbolTableList);

        // Get the data for zebin
        for (auto I : symbolTableList)
        {
            Value* symbolValue = I.first;
            auto symbolEntry = I.second;

            if (Function* F = dyn_cast<Function>(symbolValue))
            {
                funcSyms.function.emplace_back((vISA::GenSymType)symbolEntry.s_type, symbolEntry.s_offset, symbolEntry.s_size, F->getName().str());
            }
            else if (GlobalVariable* G = dyn_cast<GlobalVariable>(symbolValue))
            {
                // const sampler
                if (symbolEntry.s_type == vISA::GenSymType::S_CONST_SAMPLER) {
                    funcSyms.sampler.emplace_back((vISA::GenSymType)symbolEntry.s_type, symbolEntry.s_offset, symbolEntry.s_size, G->getName().str());
                }
                // global variables
                else if (symbolEntry.s_type == vISA::GenSymType::S_GLOBAL_VAR) {
                    programSyms.global.emplace_back((vISA::GenSymType)symbolEntry.s_type, symbolEntry.s_offset, symbolEntry.s_size, G->getName().str());
                }
                // global constants and string literals
                else {
                    Constant* initializer = G->getInitializer();
                    ConstantDataSequential* cds = dyn_cast<ConstantDataSequential>(initializer);
                    if (cds && (cds->isCString() || cds->isString()))
                        programSyms.globalStringConst.emplace_back((vISA::GenSymType)symbolEntry.s_type, symbolEntry.s_offset, symbolEntry.s_size, G->getName().str());
                    else
                        programSyms.globalConst.emplace_back((vISA::GenSymType)symbolEntry.s_type, symbolEntry.s_offset, symbolEntry.s_size, G->getName().str());
                }
            }
            else
            {
                IGC_ASSERT(0);
            }
        }
    }

    void CEncoder::CreateRelocationTable(void*& buffer, unsigned& bufferSize, unsigned& tableEntries)
    {
        // for patch-token-based binary format
        buffer = nullptr;
        bufferSize = 0;
        tableEntries = 0;

        // vISA will directly return the buffer with GenRelocEntry layout
        IGC_ASSERT(nullptr != vMainKernel);
        V(vMainKernel->GetGenRelocEntryBuffer(buffer, bufferSize, tableEntries));
        IGC_ASSERT((sizeof(vISA::GenRelocEntry) * tableEntries) == bufferSize);
    }

    void CEncoder::CreateRelocationTable(SProgramOutput::RelocListTy& relocations)
    {
        // for ZEBinary format
        IGC_ASSERT(nullptr != vMainKernel);
        V(vMainKernel->GetRelocations(relocations));
    }

    void CEncoder::CreateFuncAttributeTable(void*& buffer, unsigned& bufferSize,
        unsigned& tableEntries, SProgramOutput::FuncAttrListTy& attrs)
    {
        buffer = nullptr;
        bufferSize = 0;
        tableEntries = 0;

        std::vector<vISA::GenFuncAttribEntry> attribTable;
        for (auto it : funcAttributeMap)
        {
            vISA::GenFuncAttribEntry entry;
            Function* F = it.first;
            IGC_ASSERT(nullptr != F);
            IGC_ASSERT(F->getName().size() <= vISA::MAX_SYMBOL_NAME_LENGTH);
            strcpy_s(entry.f_name, vISA::MAX_SYMBOL_NAME_LENGTH, F->getName().str().c_str());
            entry.f_isKernel = it.second.isKernel ? 1 : 0;
            entry.f_hasBarrier = it.second.hasBarrier ? 1 : 0;
            entry.f_privateMemPerThread = (uint32_t) (it.second.argumentStackSize + it.second.allocaStackSize);

            // Get spill mem usage from visa
            VISAKernel* visaFunc = nullptr;
            if (it.second.isKernel)
            {
                visaFunc = vMainKernel;
            }
            else
            {
                auto Iter = stackFuncMap.find(F);
                IGC_ASSERT_MESSAGE(Iter != stackFuncMap.end(), "vISA function not found");
                visaFunc = Iter->second;
            }
            FINALIZER_INFO* jitInfo;
            visaFunc->GetJitInfo(jitInfo);
            entry.f_spillMemPerThread = jitInfo->spillMemUsed;

            attrs.emplace_back(entry.f_isKernel, entry.f_hasBarrier, entry.f_privateMemPerThread,
                entry.f_spillMemPerThread, F->getName().str());
            attribTable.push_back(entry);
        }

        if (!attribTable.empty())
        {
            tableEntries = attribTable.size();
            bufferSize = tableEntries * sizeof(vISA::GenFuncAttribEntry);
            buffer = malloc(bufferSize);
            IGC_ASSERT_MESSAGE(nullptr != buffer, "Table cannot be allocated");
            memcpy_s(buffer, bufferSize, attribTable.data(), bufferSize);
        }
    }

    void CEncoder::Compile(bool hasSymbolTable)
    {
        IGC_ASSERT(nullptr != m_program);
        CodeGenContext* const context = m_program->GetContext();
        SProgramOutput* const pOutput = m_program->ProgramOutput();
        const std::vector<const char*>* additionalVISAAsmToLink = nullptr;
        if(context->type == ShaderType::OPENCL_SHADER) {
            auto cl_context = static_cast<OpenCLProgramContext*>(context);
            if(!cl_context->m_VISAAsmToLink.empty()) {
                additionalVISAAsmToLink = &cl_context->m_VISAAsmToLink;
            }
        }

        if (m_program->m_dispatchSize == SIMDMode::SIMD8)
        {
            MEM_SNAPSHOT(IGC::SMS_AFTER_CISACreateDestroy_SIMD8);
        }
        else if (m_program->m_dispatchSize == SIMDMode::SIMD16)
        {
            MEM_SNAPSHOT(IGC::SMS_AFTER_CISACreateDestroy_SIMD16);
        }
        else if (m_program->m_dispatchSize == SIMDMode::SIMD32)
        {
            MEM_SNAPSHOT(IGC::SMS_AFTER_CISACreateDestroy_SIMD32);
        }

        int vIsaCompile = 0;
        VISAKernel* pMainKernel = nullptr;

        // ShaderOverride for .visaasm files
        std::vector<std::string> visaOverrideFiles;
        bool visaAsmOverride = false;
        std::string kernelName;
        if (IGC_IS_FLAG_ENABLED(ShaderOverride))
        {
            // Kernel count is one per visaBuilder
            // Function count depends on stackFuncMap size
            int kernelCount = 1;
            int functionCount = stackFuncMap.size();
            int count = kernelCount + functionCount;
            IGC::Debug::OutputFolderName folder = IGC::Debug::GetShaderOverridePath();
            Debug::DumpName name = IGC::Debug::GetDumpNameObj(m_program, "visaasm");
            kernelName = name.GetKernelName();

            visaOverrideFiles.push_back(name.AbsolutePath(folder));

            for (int i = 0; i < functionCount; i++)
            {
                std::string tmpVisaFile = name.AbsolutePath(folder);
                std::string::size_type asmNameEnd = tmpVisaFile.find_last_of('.');
                tmpVisaFile = tmpVisaFile.substr(0, asmNameEnd);
                std::stringstream asmName;
                asmName << tmpVisaFile;
                asmName << "_f";
                asmName << i;
                asmName << ".visaasm";
                visaOverrideFiles.push_back(asmName.str());
            }

            if (visaOverrideFiles.size() == count)
            {
                for (const std::string& file : visaOverrideFiles)
                {
                    FILE*  tempFile = fopen(file.c_str(), "r");
                    if (tempFile)
                    {
                        visaAsmOverride = true;
                        fclose(tempFile);
                    }
                    else
                    {
                        visaAsmOverride = false;
                        if (functionCount > 0)
                        {
                            std::string message = "Cannot open overridden file! Put all .visaasm files in ShaderOverride dir.";
                            appendToShaderOverrideLogFile(message, "WARNING: ");
                        }
                        break;

                    }
                }
            }
        }

        // Compile generated VISA text string for inlineAsm
        if (m_hasInlineAsm || visaAsmOverride || additionalVISAAsmToLink)
        {
            llvm::SmallVector<const char*, 10> params;
            llvm::SmallVector<std::unique_ptr< char, std::function<void(char*)>>, 10> params2;
            InitBuildParams(params2);
            for (const auto &ptr : params2)
            {
                params.push_back(ptr.get());
            }

            // Create a new builder for parsing the visaasm
            TARGET_PLATFORM VISAPlatform = GetVISAPlatform(&(context->platform));
            V(CreateVISABuilder(vAsmTextBuilder, vISA_ASM_READER, VISA_BUILDER_BOTH, VISAPlatform,
                params.size(), params.data(), &m_vISAWaTable));
            // Use the same build options as before, except that we enable vISA verifier to catch
            // potential errors in user inline assembly
            SetBuilderOptions(vAsmTextBuilder);
            vAsmTextBuilder->SetOption(vISA_NoVerifyvISA, false);

            bool vISAAsmParseError = false;
            // Parse the generated VISA text
            if (visaAsmOverride)
            {
                for (const std::string& tmpVisaFile : visaOverrideFiles)
                {
                    llvm::SmallVector<char, 1024> visaAsmNameVector;
                    std::string visaAsmName = GetDumpFileName("");

                    StringRef visaAsmNameRef(visaAsmName.c_str());
                    StringRef tmpVisaFileRef(tmpVisaFile.c_str());
                    StringRef directory = llvm::sys::path::parent_path(visaAsmNameRef);
                    StringRef filename = llvm::sys::path::filename(tmpVisaFileRef);

                    llvm::sys::path::append(visaAsmNameVector, directory, filename);
                    visaAsmName = std::string(visaAsmNameVector.begin(), visaAsmNameVector.end());

                    auto result = vAsmTextBuilder->ParseVISAText(tmpVisaFile.c_str());
                    appendToShaderOverrideLogFile(visaAsmName, "OVERRIDEN: ");
                    vISAAsmParseError = (result != 0);
                    if (vISAAsmParseError) {
                        IGC_ASSERT_MESSAGE(0, "visaasm file parse error!");
                        break;
                    }
                }

                // We need to update stackFuncMap for the symbol table for the overridden object,
                // because stackFuncMap contains information about functions for original object.
                // Only the IndirectlyCalled functions should be updated,
                // because these functions can be used in CreateSymbolTable.
                // Other normal stack call functions aren't used in CreateSymbolTable.
                if (hasSymbolTable && stackFuncMap.size() > 0)
                {
                    Module* pModule = m_program->GetContext()->getModule();
                    for (auto& F : pModule->getFunctionList())
                    {
                        if (F.hasFnAttribute("referenced-indirectly") && (!F.isDeclaration() || !F.use_empty()))
                        {
                            auto Iter = stackFuncMap.find(&F);
                            IGC_ASSERT_MESSAGE(Iter != stackFuncMap.end(), "vISA function not found");

                            VISAFunction* original = Iter->second;
                            stackFuncMap[&F] = static_cast<VISAFunction*>(vAsmTextBuilder->GetVISAKernel(original->getFunctionName()));
                        }
                    }
                }
            }
            else
            {
                int result = 0;
                if (m_hasInlineAsm) {
                    std::string parseTextFile = GetDumpFileName("inline.visaasm");
                    result = vAsmTextBuilder->ParseVISAText(vbuilder->GetAsmTextStream().str(), parseTextFile);
                }

                if (result == 0 && additionalVISAAsmToLink) {
                    std::stringstream ss;
                    result = vbuilder->Compile(
                        m_enableVISAdump ? GetDumpFileName("isa").c_str() : "",
                        &ss, true);
                    result = (result == 0) ? vAsmTextBuilder->ParseVISAText(vMainKernel->getVISAAsm(), "") : result;
                    for(auto visaAsm : *additionalVISAAsmToLink) {
                        result = (result == 0) ? vAsmTextBuilder->ParseVISAText(visaAsm, "") : result;
                    }

                    // Mark invoke_simd targets with LTO_InvokeOptTarget attribute.
                    IGC_ASSERT(m_program && m_program->GetContext() && m_program->GetContext()->getModule());
                    for (auto& F : m_program->GetContext()->getModule()->getFunctionList())
                    {
                        if (F.hasFnAttribute("invoke_simd_target")) {
                            auto vFunc = vAsmTextBuilder->GetVISAKernel(F.getName().data());
                            IGC_ASSERT(vFunc);
                            bool enabled = true;
                            vFunc->AddKernelAttribute("LTO_InvokeOptTarget", 1, &enabled);
                        }
                    }
                }

                if (result != 0)
                {
                    std::string output;
                    raw_string_ostream S(output);
                    S << "parsing vISA inline assembly failed:\n" << vAsmTextBuilder->GetCriticalMsg();
                    S.flush();
                    context->EmitError(output.c_str(), nullptr);
                    vISAAsmParseError = true;
                }
            }

            if (vISAAsmParseError)
            {
                COMPILER_TIME_END(m_program->GetContext(), TIME_CG_vISACompile);
                return;
            }
            else
            {
                if (!visaAsmOverride)
                {
                    // vISA verifier is already invoked in ParseVISAText earlier
                    vAsmTextBuilder->SetOption(vISA_NoVerifyvISA, true);
                }

                if (visaAsmOverride || additionalVISAAsmToLink) {
                    // After call to ParseVISAText, we have new VISAKernel, which don't have asm path set.
                    // So we need to set the OutputAsmPath attribute of overridden kernel,
                    // otherwise, we will not get .visaasm dump and .asm file dump
                    auto kernelName = IGC::Debug::GetDumpNameObj(m_program, "").GetKernelName();
                    std::string asmName = GetDumpFileName("asm");
                    auto vKernel = vAsmTextBuilder->GetVISAKernel(kernelName);
                    vKernel->AddKernelAttribute("OutputAsmPath", asmName.length(), asmName.c_str());
                }

                pMainKernel = vAsmTextBuilder->GetVISAKernel(kernelName);
                vIsaCompile = vAsmTextBuilder->Compile(
                    m_enableVISAdump ? GetDumpFileName("isa").c_str() : "");
            }
        }
        //Compile to generate the V-ISA binary
        else
        {
            pMainKernel = vMainKernel;
            vIsaCompile = vbuilder->Compile(
                m_enableVISAdump ? GetDumpFileName("isa").c_str() : "");
        }

        COMPILER_TIME_END(m_program->GetContext(), TIME_CG_vISACompile);

#if GET_TIME_STATS
        // handle the vISA time counters differently here
        if (context->m_compilerTimeStats)
        {
            context->m_compilerTimeStats->recordVISATimers();
        }
#endif
        KERNEL_INFO* vISAstats;
        pMainKernel->GetKernelInfo(vISAstats);
        // Collect metrics from vISA
        context->metrics.CollectRegStats(vISAstats);

        FINALIZER_INFO* jitInfo = nullptr;
        pMainKernel->GetJitInfo(jitInfo);

        // Depend on vISA information about barriers presence to make sure that it's
        // always set properly, even if a barrier is used as a part of Inline vISA code only.
        if (jitInfo->usesBarrier)
        {
            m_program->SetHasBarrier();
        }

        if (jitInfo->isSpill)
        {
            context->m_retryManager.SetSpillSize(jitInfo->numGRFSpillFill);
            m_program->m_spillSize = jitInfo->numGRFSpillFill;
            m_program->m_spillCost =
                float(jitInfo->numGRFSpillFill) / jitInfo->numAsmCount;

            context->m_retryManager.numInstructions = jitInfo->numAsmCount;
        }

        if (IGC_IS_FLAG_ENABLED(DumpCompilerStats))
        {
            CompilerStats CompilerStats;
            pMainKernel->GetCompilerStats(CompilerStats);
            CompilerStatsUtils::RecordCodeGenCompilerStats(context, m_program->m_dispatchSize, CompilerStats, jitInfo);
        }

        if (vIsaCompile == -1)
        {
            IGC_ASSERT_MESSAGE(0, "CM failure in vbuilder->Compile()");
        }
        else if (vIsaCompile == -2)
        {
            IGC_ASSERT_MESSAGE(0, "CM user error in vbuilder->Compile()");
        }
        else if (vIsaCompile == -3) // CM early terminates on spill
        {
#if (GET_SHADER_STATS)
            if (m_program->m_dispatchSize == SIMDMode::SIMD8)
            {
                COMPILER_SHADER_STATS_SET(m_program->m_shaderStats, STATS_ISA_EARLYEXIT8, 1);
            }
            else if (m_program->m_dispatchSize == SIMDMode::SIMD16)
            {
                COMPILER_SHADER_STATS_SET(m_program->m_shaderStats, STATS_ISA_EARLYEXIT16, 1);
            }
            else if (m_program->m_dispatchSize == SIMDMode::SIMD32)
            {
                COMPILER_SHADER_STATS_SET(m_program->m_shaderStats, STATS_ISA_EARLYEXIT32, 1);
            }
#endif
            context->SetSIMDInfo(SIMD_SKIP_SPILL, m_program->m_dispatchSize, m_program->m_ShaderDispatchMode);
            return;
        }

        if (m_program->m_dispatchSize == SIMDMode::SIMD8)
        {
            MEM_SNAPSHOT(IGC::SMS_AFTER_vISACompile_SIMD8);
            SimdSize8++;
        }
        else if (m_program->m_dispatchSize == SIMDMode::SIMD16)
        {
            MEM_SNAPSHOT(IGC::SMS_AFTER_vISACompile_SIMD16);
            SimdSize16++;
        }
        else if (m_program->m_dispatchSize == SIMDMode::SIMD32)
        {
            MEM_SNAPSHOT(IGC::SMS_AFTER_vISACompile_SIMD32);
            SimdSize32++;
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

        if (jitInfo->isSpill && (AvoidRetryOnSmallSpill() || jitInfo->avoidRetry))
        {
            context->m_retryManager.Disable();
        }

#if (GET_SHADER_STATS && !PRINT_DETAIL_SHADER_STATS)
        if (m_program->m_dispatchSize == SIMDMode::SIMD8)
        {
            COMPILER_SHADER_STATS_SET(m_program->m_shaderStats, STATS_ISA_INST_COUNT, jitInfo->numAsmCount);
            COMPILER_SHADER_STATS_SET(m_program->m_shaderStats, STATS_ISA_SPILL8, (int)jitInfo->isSpill);
        }
        else if (m_program->m_dispatchSize == SIMDMode::SIMD16)
        {
            COMPILER_SHADER_STATS_SET(m_program->m_shaderStats, STATS_ISA_INST_COUNT_SIMD16, jitInfo->numAsmCount);
            COMPILER_SHADER_STATS_SET(m_program->m_shaderStats, STATS_ISA_SPILL16, (int)jitInfo->isSpill);
        }
        else if (m_program->m_dispatchSize == SIMDMode::SIMD32)
        {
            COMPILER_SHADER_STATS_SET(m_program->m_shaderStats, STATS_ISA_INST_COUNT_SIMD32, jitInfo->numAsmCount);
            COMPILER_SHADER_STATS_SET(m_program->m_shaderStats, STATS_ISA_SPILL32, (int)jitInfo->isSpill);
        }
#endif
        void* genxbin = nullptr;
        int size = 0, binSize = 0;
        bool binOverride = false;

        V(pMainKernel->GetGenxBinary(genxbin, binSize));
        if (IGC_IS_FLAG_ENABLED(ShaderOverride))
        {
            Debug::DumpName name = IGC::Debug::GetDumpNameObj(m_program, "asm");
            std::string binFileName = name.overridePath();

            overrideShaderIGA(context->platform.getPlatformInfo(), genxbin, binSize, binFileName, binOverride);

            if (!binOverride)
            {
                name = IGC::Debug::GetDumpNameObj(m_program, "dat");
                binFileName = name.overridePath();
                overrideShaderBinary(genxbin, binSize, binFileName, binOverride);
            }

        }

        IGC_ASSERT(genxbin != nullptr);
        size = binSize;

        // the kernel has to be padded to have a size aligned on 64 bytes
        size_t padding = iSTD::GetAlignmentOffset(size, 64);//m_program->m_Platform->getKernelPointerAlignSize());
        void* kernel = nullptr;
        if (size!=0)
        {
            kernel = IGC::aligned_malloc(size + padding, 16 /* sizeof(DQWORD) */);
            memcpy_s(kernel, size + padding, genxbin, binSize);
            // pad out the rest with 0s
            memset(static_cast<char*>(kernel) + size, 0, padding);
        }
        if (binOverride)
        {
            free(genxbin);
        }
        else
        {
            freeBlock(genxbin);
        }

        void* dbgInfo = nullptr;
        unsigned int dbgSize = 0;
        if (context->m_instrTypes.hasDebugInfo || m_enableVISAdump)
        {
            void* genxdbgInfo = nullptr;
            V(pMainKernel->GetGenxDebugInfo(genxdbgInfo, dbgSize));
            if (m_enableVISAdump)
            {
                // passing VISAOptions: -generateDebugInfo should
                // cause dbg file to be generated, even when
                // hasDebugInfo = false.
                if (context->m_instrTypes.hasDebugInfo)
                {
                    // assertion check makes sense only if debug info
                    // is present in input.
                    IGC_ASSERT(nullptr != genxdbgInfo);
                    IGC_ASSERT(0 < dbgSize);
                }
                if (dbgSize > 0)
                {
                    // dump dbg file only if it not empty
                    std::string debugFileNameStr = IGC::Debug::GetDumpName(m_program, "dbg");
                    FILE* const dbgFile = fopen(debugFileNameStr.c_str(), "wb+");
                    if (nullptr != dbgFile)
                    {
                        fwrite(genxdbgInfo, dbgSize, 1, dbgFile);
                        fclose(dbgFile);
                    }
                }
            }

            dbgInfo = IGC::aligned_malloc(dbgSize, sizeof(void*));

            memcpy_s(dbgInfo, dbgSize, genxdbgInfo, dbgSize);

            freeBlock(genxdbgInfo);
        }

        pOutput->m_programBin = kernel;
        pOutput->m_programSize = size + padding;
        pOutput->m_unpaddedProgramSize = size;
        pOutput->m_scratchSpaceUsedBySpills = 0; // initializing
        pOutput->m_debugDataGenISA = dbgInfo;
        pOutput->m_debugDataGenISASize = dbgSize;
        pOutput->m_InstructionCount = jitInfo->numAsmCount;
        pOutput->m_BasicBlockCount = jitInfo->BBNum;
        if (context->getModuleMetaData()->compOpt.CaptureCompilerStats)
        {
            ReportCompilerStatistics(pMainKernel, pOutput);
        }

        pMainKernel->GetGTPinBuffer(pOutput->m_gtpinBuffer, pOutput->m_gtpinBufferSize);

        bool ZEBinEnabled = IGC_IS_FLAG_ENABLED(EnableZEBinary) || context->getCompilerOption().EnableZEBinary;

        if (hasSymbolTable)
        {
            if (ZEBinEnabled)
            {
                // we can only support zebin symbols for OPENCL_SHADER for now
                IGC_ASSERT(context->type == ShaderType::OPENCL_SHADER);
                auto cl_context = static_cast<OpenCLProgramContext*>(context);
                CreateSymbolTable(pOutput->m_symbols,
                    cl_context->m_programInfo.m_zebinSymbolTable);
            }
            else
            {
                CreateSymbolTable(pOutput->m_funcSymbolTable,
                    pOutput->m_funcSymbolTableSize,
                    pOutput->m_funcSymbolTableEntries);
            }
        }

        if (ZEBinEnabled)
        {
            // create symbols for kernel.
            // The kernel Symbol has the same name as the kernel, and offset
            // pointed to 0.
            CreateKernelSymbol(m_program->entry->getName().str(), 0,
                (unsigned)pMainKernel->getGenSize(), pOutput->m_symbols);

            // Emit symbol "_entry' as the actual kernel start. Maybe we can
            // consider to use the value of the _main label in this case. Now
            // set the symbol value as the max offset next to the per-thread
            // prolog, the cross-thread prolog, or the compute-FFID prolog.
            unsigned actual_kernel_start_off =
                std::max(std::max(jitInfo->offsetToSkipPerThreadDataLoad,
                                  jitInfo->offsetToSkipCrossThreadDataLoad),
                         jitInfo->offsetToSkipSetFFIDGP1);
            CreateKernelSymbol("_entry", actual_kernel_start_off,
                (unsigned)pMainKernel->getGenSize() - actual_kernel_start_off, pOutput->m_symbols);
        }

        if (ZEBinEnabled)
        {
            CreateRelocationTable(pOutput->m_relocs);
        }
        else
        {
            CreateRelocationTable(pOutput->m_funcRelocationTable,
                pOutput->m_funcRelocationTableSize,
                pOutput->m_funcRelocationTableEntries);
        }

        if (IGC_IS_FLAG_ENABLED(EnableRuntimeFuncAttributePatching))
        {
            CreateFuncAttributeTable(pOutput->m_funcAttributeTable,
                pOutput->m_funcAttributeTableSize,
                pOutput->m_funcAttributeTableEntries,
                pOutput->m_funcAttrs);
        }

        if (jitInfo->isSpill == true)
        {
            pOutput->m_scratchSpaceUsedBySpills = jitInfo->spillMemUsed;
        }

        pOutput->setScratchSpaceUsedByShader(m_program->m_ScratchSpaceSize);

        pOutput->m_scratchSpaceUsedByGtpin = jitInfo->numBytesScratchGtpin;

        pOutput->m_offsetToSkipPerThreadDataLoad = jitInfo->offsetToSkipPerThreadDataLoad;

        pOutput->m_offsetToSkipSetFFIDGP = jitInfo->offsetToSkipSetFFIDGP;

        pOutput->m_numGRFTotal = jitInfo->numGRFTotal;
    }

    void CEncoder::DestroyVISABuilder()
    {
        if (vAsmTextBuilder != nullptr)
        {
            V(::DestroyVISABuilder(vAsmTextBuilder));
            vAsmTextBuilder = nullptr;
        }
        V(::DestroyVISABuilder(vbuilder));
        vbuilder = nullptr;
    }

    void CEncoder::Copy(CVariable* dst, CVariable* src)
    {
        IGC_ASSERT(nullptr != dst);
        IGC_ASSERT(nullptr != src);
        // undef value are not copied
        if (!src->IsUndef() || IGC_IS_FLAG_ENABLED(InitializeUndefValueEnable))
        {
            CVariable* rawDst = dst;
            IGC_ASSERT(GetCISADataTypeSize(src->GetType()) == GetCISADataTypeSize(dst->GetType()));
            bool isVecImm = src->IsImmediate() && (src->GetType() == ISA_TYPE_UV ||
                src->GetType() == ISA_TYPE_V ||
                src->GetType() == ISA_TYPE_VF);
            if (src->GetType() != dst->GetType() && !isVecImm)
            {
                rawDst = m_program->BitCast(dst, src->GetType());
            }
            DataMov(ISA_MOV, rawDst, src);
        }
    }

    void CEncoder::BoolToInt(CVariable* dst, CVariable* src)
    {
        IGC_ASSERT(nullptr != dst);
        IGC_ASSERT(nullptr != src);
        IGC_ASSERT(src->GetType() == ISA_TYPE_BOOL);

        VISA_Type dstType = dst->GetType();
        IGC_ASSERT((dstType == ISA_TYPE_UD) || (dstType == ISA_TYPE_D) || (dstType == ISA_TYPE_UB) || (dstType == ISA_TYPE_B) || (dstType == ISA_TYPE_UW) || (dstType == ISA_TYPE_W));

        // undef value are not copied
        if (!src->IsUndef() || IGC_IS_FLAG_ENABLED(InitializeUndefValueEnable)) {
            // Casting 'dst' to BOOL is unnecessary.
            DataMov(ISA_MOV, dst, src);
        }
    }

    void CEncoder::GatherA64(
        CVariable* dst,
        CVariable* offset,
        unsigned elemSize,
        unsigned numElems)
    {
        IGC_ASSERT_MESSAGE((elemSize == 8) || (elemSize == 32) || (elemSize == 64),
            "Only B/DW/QW-sized elements are supported!");
        IGC_ASSERT_MESSAGE((numElems == 1) || (numElems == 2) || (numElems == 4) || ((numElems == 8) && ((elemSize == 32) || m_program->m_Platform->has8ByteA64ByteScatteredMessage())),
            "Only 1/2/4/8 elements are supported!");

        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
        VISA_RawOpnd* addressOpnd = GetRawSource(offset);
        VISA_RawOpnd* dstOpnd = GetRawDestination(dst);

        SIMDMode thisSM = offset->IsUniform() ? lanesToSIMDMode(offset->GetNumberElement()) : m_encoderState.m_simdSize;
        if (m_program->m_Platform->GetPlatformFamily() == IGFX_GEN8_CORE && thisSM == SIMDMode::SIMD16)
        {
            // BDW A64 gather does not support SIMD16, split it into 2 SIMD8
            VISA_EMask_Ctrl execMask = GetAluEMask(offset);
            VISA_Exec_Size fromExecSize = EXEC_SIZE_16;
            VISA_Exec_Size toExecSize = EXEC_SIZE_8;

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
                CVariable* V0, * V1;
                uint16_t newNumElems = (uint16_t)8 * numElems;
                V0 = m_program->GetNewVariable(
                    newNumElems,
                    dst->GetType(),
                    dst->GetAlign(),
                    dst->IsUniform(),
                    dst->getName());
                V1 = m_program->GetNewVariable(
                    newNumElems,
                    dst->GetType(),
                    dst->GetAlign(),
                    dst->IsUniform(),
                    dst->getName());

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

                uint32_t dstOfstBytes = dst->GetAliasOffset() + m_encoderState.m_dstOperand.subVar * getGRFSize();
                MergePayloadToHigherSIMD(V0, V1, numElems, dst, dstOfstBytes, 16);
            }
            return;
        }

        V(vKernel->AppendVISASvmGatherInst(predOpnd, GetAluEMask(offset),
            visaExecSize(offset->IsUniform() ? lanesToSIMDMode(offset->GetNumberElement()) : m_encoderState.m_simdSize),
            visaBlockType(elemSize),
            visaBlockNum(numElems),
            addressOpnd, dstOpnd));
    }

    void CEncoder::ScatterA64(CVariable* src,
        CVariable* offset,
        unsigned elemSize,
        unsigned numElems) {
        IGC_ASSERT_MESSAGE((elemSize == 8) || (elemSize == 32) || (elemSize == 64),
            "Only B/DW/QW-sized elements are supported!");
        IGC_ASSERT_MESSAGE((numElems == 1) || (numElems == 2) || (numElems == 4) || ((numElems == 8) && ((elemSize == 32) || m_program->m_Platform->has8ByteA64ByteScatteredMessage())),
            "Only 1/2/4/8 elements are supported!");

        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
        VISA_RawOpnd* addressOpnd = GetRawSource(offset);
        VISA_RawOpnd* srcOpnd = GetRawSource(src);

        SIMDMode thisSM = offset->IsUniform() ? lanesToSIMDMode(offset->GetNumberElement()) : m_encoderState.m_simdSize;
        if (m_program->m_Platform->GetPlatformFamily() == IGFX_GEN8_CORE && thisSM == SIMDMode::SIMD16)
        {
            // BDW A64 scatter does not support SIMD16, split it into 2 SIMD8
            VISA_EMask_Ctrl execMask = GetAluEMask(offset);
            VISA_Exec_Size fromExecSize = EXEC_SIZE_16;
            VISA_Exec_Size toExecSize = EXEC_SIZE_8;

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
                CVariable* V0, * V1;
                uint16_t newNumElems = (uint16_t)8 * numElems;
                V0 = m_program->GetNewVariable(
                    newNumElems,
                    src->GetType(),
                    src->GetAlign(),
                    src->IsUniform(),
                    CName::NONE);
                V1 = m_program->GetNewVariable(
                    newNumElems,
                    src->GetType(),
                    src->GetAlign(),
                    src->IsUniform(),
                    CName::NONE);
                // Starting offset is calculated from AliasOffset only (subVar not used).
                uint32_t srcOfstBytes = src->GetAliasOffset();
                SplitPayloadToLowerSIMD(src, srcOfstBytes, numElems, V0, V1, 16);

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
        this->m_program->IncStatelessWritesCount();
    }

    void CEncoder::ByteGather(
        CVariable* dst,
        const ResourceDescriptor& resource,
        CVariable* offset,
        unsigned elemSize,
        unsigned numElems) {
        IGC_ASSERT_MESSAGE(elemSize == 8, "Only BYTE element is supported!");
        IGC_ASSERT_MESSAGE((numElems == 1) || (numElems == 2) || (numElems == 4),
            "Only 1/2/4 elements are supported!");

        // Extend the offset to 64bits and use the A64 gather message if needed
        if ((resource.m_surfaceType == ESURFACE_STATELESS) &&
            (m_program->m_DriverInfo->NeedWAToTransformA32MessagesToA64()) &&
            (m_program->m_Platform->getWATable().WaNoA32ByteScatteredStatelessMessages != 0))
        {
            SEncoderState gatherState = CopyEncoderState();
            Push();

            CVariable* offset64 = m_program->GetNewVariable(
                offset->GetNumberElement(),
                ISA_TYPE_UQ,
                EALIGN_GRF,
                offset->IsUniform(),
                offset->GetNumberInstance(),
                CName(offset->getName(), "_64b"));

            CVariable* offset32UD = m_program->BitCast(offset, ISA_TYPE_UD);

            if (offset->IsUniform())
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
        VISA_RawOpnd* addressOpnd = GetRawSource(offset);
        VISA_RawOpnd* dstOpnd = GetRawDestination(dst);

        VISA_VectorOpnd* globalOffsetOpnd = 0;
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

    void CEncoder::ByteScatter(
        CVariable* src,
        const ResourceDescriptor& resource,
        CVariable* offset,
        unsigned elemSize,
        unsigned numElems)
    {
        IGC_ASSERT_MESSAGE(elemSize == 8, "Only BYTE element is supported!");
        IGC_ASSERT_MESSAGE((numElems == 1) || (numElems == 2) || (numElems == 4),
            "Only 1/2/4 elements are supported!");

        // Extend the offset to 64bits and use the A64 gather message if needed
        if ((resource.m_surfaceType == ESURFACE_STATELESS) &&
            (m_program->m_DriverInfo->NeedWAToTransformA32MessagesToA64()) &&
            (m_program->m_Platform->getWATable().WaNoA32ByteScatteredStatelessMessages != 0))
        {
            SEncoderState gatherState = CopyEncoderState();
            Push();

            CVariable* offset64 = m_program->GetNewVariable(
                offset->GetNumberElement(),
                ISA_TYPE_UQ,
                EALIGN_GRF,
                offset->IsUniform(),
                offset->GetNumberInstance(),
                CName(offset->getName(), "_64b"));

            CVariable* offset32UD = m_program->BitCast(offset, ISA_TYPE_UD);

            if (offset->IsUniform())
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
        VISA_RawOpnd* addressOpnd = GetRawSource(offset);
        VISA_RawOpnd* srcOpnd = GetRawSource(src);

        VISA_VectorOpnd* globalOffsetOpnd = 0;
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

    void CEncoder::Gather4ScaledNd(CVariable* dst,
        const ResourceDescriptor& resource,
        CVariable* offset,
        unsigned nd) {

        VISA_StateOpndHandle* surfaceOpnd = GetVISASurfaceOpnd(resource);
        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
        VISA_RawOpnd* addressOpnd = GetRawSource(offset);
        VISA_RawOpnd* dstOpnd = GetRawDestination(dst);

        VISA_VectorOpnd* globalOffsetOpnd = 0;
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

    uint32_t CEncoder::getNumChannels(CVariable* var) const
    {
        IGC_ASSERT(nullptr != var);
        unsigned nd = var->GetSize();
        if (var->IsUniform())
        {
            IGC_ASSERT_MESSAGE(nd <= getGRFSize(), "Unknown Variable Size!");
            return 1;
        }
        else
        {
            static_assert(0 < SIZE_DWORD);

            switch (m_encoderState.m_simdSize)
            {
            case SIMDMode::SIMD8:
                return nd / (8 * SIZE_DWORD);
            case SIMDMode::SIMD16:
                return nd / (16 * SIZE_DWORD);
            case SIMDMode::SIMD32:
                return nd / (32 * SIZE_DWORD);
            default:
                IGC_ASSERT_MESSAGE(0, "Unknown SIMD size!");
                return 1;
            }
        }
        return 1;
    }

    void CEncoder::Gather4Scaled(CVariable* dst,
        const ResourceDescriptor& resource,
        CVariable* offset)
    {
        unsigned nd = getNumChannels(dst);
        Gather4ScaledNd(dst, resource, offset, nd);
    }

    void CEncoder::Scatter4Scaled(CVariable* src,
        const ResourceDescriptor& resource,
        CVariable* offset) {
        unsigned nd = getNumChannels(src);

        VISA_StateOpndHandle* surfaceOpnd = GetVISASurfaceOpnd(resource);
        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
        VISA_RawOpnd* addressOpnd = GetRawSource(offset);
        VISA_RawOpnd* srcOpnd = GetRawSource(src);

        VISA_VectorOpnd* globalOffsetOpnd = 0;
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
        if (ESURFACE_STATELESS == resource.m_surfaceType)
        {
            this->m_program->IncStatelessWritesCount();
        }
    }

    void CEncoder::Gather4A64(CVariable* dst, CVariable* offset) {
        IGC_ASSERT(nullptr != dst);
        IGC_ASSERT_MESSAGE(dst->GetElemSize() == 4, "Gather4 must have 4-byte element");

        uint32_t dstOfstBytes = m_encoderState.m_dstOperand.subVar * getGRFSize() + dst->GetAliasOffset();
        unsigned nd = dst->GetSize();
        switch (m_encoderState.m_simdSize) {
        case SIMDMode::SIMD8:
            nd = nd / (8 * SIZE_DWORD);
            break;
        case SIMDMode::SIMD16:
            nd = nd / (16 * SIZE_DWORD);
            break;
        case SIMDMode::SIMD32:
            nd = nd / (32 * SIZE_DWORD);
            break;
        default:
            IGC_ASSERT_MESSAGE(0, "Unknown SIMD size!");
            return;
        }

        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
        VISA_RawOpnd* addressOpnd = GetRawSource(offset);
        VISA_RawOpnd* dstOpnd = GetRawDestination(dst);

        VISA_VectorOpnd* globalOffsetOpnd = 0;
        int val = 0;
        V(vKernel->CreateVISAImmediate(globalOffsetOpnd, &val, ISA_TYPE_UD));

        if (m_program->m_Platform->GetPlatformFamily() == IGFX_GEN8_CORE && m_encoderState.m_simdSize == SIMDMode::SIMD16)
        {
            // BDW A64 untyped does not support SIMD16, split it into 2 SIMD8
            VISA_EMask_Ctrl execMask = GetAluEMask(offset);
            VISA_Exec_Size fromExecSize = EXEC_SIZE_16;
            VISA_Exec_Size toExecSize = EXEC_SIZE_8;

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
                CVariable* V0, * V1;
                uint16_t newNumElems = (uint16_t)8 * nd;
                V0 = m_program->GetNewVariable(
                    newNumElems,
                    dst->GetType(),
                    dst->GetAlign(),
                    dst->IsUniform(),
                    CName(dst->getName(),"_M0"));
                V1 = m_program->GetNewVariable(
                    newNumElems,
                    dst->GetType(),
                    dst->GetAlign(),
                    dst->IsUniform(),
                    CName(dst->getName(),"_M8"));

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

                MergePayloadToHigherSIMD(V0, V1, nd, dst, dstOfstBytes, 16);
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

    void CEncoder::Scatter4A64(CVariable* src, CVariable* offset) {
        IGC_ASSERT(nullptr != src);
        IGC_ASSERT_MESSAGE(src->GetElemSize() == 4, "scatter4 must have 4-byte element");

        uint32_t srcOfstBytes = src->GetAliasOffset();
        unsigned nd = src->GetSize();
        switch (m_encoderState.m_simdSize) {
        case SIMDMode::SIMD8:
            nd = nd / (8 * SIZE_DWORD);
            break;
        case SIMDMode::SIMD16:
            nd = nd / (16 * SIZE_DWORD);
            break;
        case SIMDMode::SIMD32:
            nd = nd / (32 * SIZE_DWORD);
            break;
        default:
            IGC_ASSERT_MESSAGE(0, "unknown SIMD size");
            return;
        }

        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
        VISA_RawOpnd* addressOpnd = GetRawSource(offset);
        VISA_RawOpnd* srcOpnd = GetRawSource(src);

        VISA_VectorOpnd* globalOffsetOpnd = 0;
        int val = 0;
        V(vKernel->CreateVISAImmediate(globalOffsetOpnd, &val, ISA_TYPE_UD));

        if (m_program->m_Platform->GetPlatformFamily() == IGFX_GEN8_CORE && m_encoderState.m_simdSize == SIMDMode::SIMD16)
        {
            // BDW A64 untyped does not support SIMD16, split it into 2 SIMD8
            VISA_EMask_Ctrl execMask = GetAluEMask(src);
            VISA_Exec_Size fromExecSize = EXEC_SIZE_16;
            VISA_Exec_Size toExecSize = EXEC_SIZE_8;

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
                CVariable* V0, * V1;
                uint16_t newNumElems = (uint16_t)8 * nd;
                V0 = m_program->GetNewVariable(
                    newNumElems,
                    src->GetType(),
                    src->GetAlign(),
                    src->IsUniform(),
                    CName(src->getName(),"_M0"));
                V1 = m_program->GetNewVariable(
                    newNumElems,
                    src->GetType(),
                    src->GetAlign(),
                    src->IsUniform(),
                    CName(src->getName(),"_M8"));

                SplitPayloadToLowerSIMD(src, srcOfstBytes, nd, V0, V1, 16);

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

    void CEncoder::AtomicRawA64(
        AtomicOp atomic_op,
        const ResourceDescriptor& resource,
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

        if (m_encoderState.m_simdSize == SIMDMode::SIMD16)
        {
            // Split SIMD16 atomic ops into two SIMD8 ones.
            VISA_EMask_Ctrl execMask = ConvertMaskToVisaType(m_encoderState.m_mask, m_encoderState.m_noMask);
            VISA_Exec_Size fromExecSize = visaExecSize(m_encoderState.m_simdSize);
            VISA_Exec_Size toExecSize = SplitExecSize(fromExecSize, 2);

            for (unsigned thePart = 0; thePart != 2; ++thePart)
            {
                CVariable* rawOpndVar = nullptr;
                uint32_t rawOpndOffset = 0;
                bool isFirstHalf = thePart == 0;

                std::tie(rawOpndVar, rawOpndOffset) = splitRawOperand(offset, isFirstHalf, execMask);
                VISA_RawOpnd* addressOpnd = GetRawSource(rawOpndVar, rawOpndOffset);
                std::tie(rawOpndVar, rawOpndOffset) = splitRawOperand(src0, isFirstHalf, execMask);
                VISA_RawOpnd* src0Opnd = GetRawSource(rawOpndVar, rawOpndOffset);
                std::tie(rawOpndVar, rawOpndOffset) = splitRawOperand(src1, isFirstHalf, execMask);
                VISA_RawOpnd* src1Opnd = GetRawSource(rawOpndVar, rawOpndOffset);

                // dst needs special handling since its move has to come after the send
                VISA_RawOpnd* dstOpnd = nullptr;
                bool needsTmpDst = !isFirstHalf && dst && (dst->GetElemSize() * 8) % getGRFSize() != 0;
                if (!needsTmpDst)
                {
                    std::tie(rawOpndVar, rawOpndOffset) = splitRawOperand(dst, isFirstHalf, execMask);
                    dstOpnd = GetRawDestination(rawOpndVar, rawOpndOffset);
                }
                else
                {
                    rawOpndVar = m_program->GetNewVariable(
                        8,
                        dst->GetType(),
                        CVariable::getAlignment(getGRFSize()),
                        CName(dst->getName(), "_RET"));
                    dstOpnd = GetRawDestination(rawOpndVar, 0);
                }

                V(vKernel->AppendVISASvmAtomicInst(GetFlagOperand(m_encoderState.m_flag),
                    SplitEMask(fromExecSize, toExecSize, thePart, execMask),
                    toExecSize, atomicOpcode, bitwidth,
                    addressOpnd, src0Opnd, src1Opnd, dstOpnd));
                this->m_program->IncStatelessWritesCount();

                if (needsTmpDst)
                {
                    SModifier mod;
                    mod.init();
                    mod.subReg = 8;
                    auto dstOpnd = GetDestinationOperand(dst, mod);

                    mod.init();
                    auto srcOpnd = GetSourceOperand(rawOpndVar, mod);

                    V(vKernel->AppendVISADataMovementInst(
                        ISA_MOV, nullptr, false,
                        SplitEMask(EXEC_SIZE_16, EXEC_SIZE_8, 1, execMask),
                        EXEC_SIZE_8, dstOpnd, srcOpnd));
                }
            }

            return;
        }

        VISA_RawOpnd* addressOpnd = GetRawSource(offset);
        VISA_RawOpnd* src0Opnd = GetRawSource(src0);
        VISA_RawOpnd* src1Opnd = GetRawSource(src1);
        VISA_RawOpnd* dstOpnd = GetRawDestination(dst);

        V(vKernel->AppendVISASvmAtomicInst(GetFlagOperand(m_encoderState.m_flag),
            ConvertMaskToVisaType(m_encoderState.m_mask, m_encoderState.m_noMask),
            visaExecSize(m_encoderState.m_simdSize),
            atomicOpcode,
            bitwidth,
            addressOpnd,
            src0Opnd,
            src1Opnd,
            dstOpnd));
        this->m_program->IncStatelessWritesCount();
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
        VISA_StateOpndHandle* surface = GetVISASurfaceOpnd(ESURFACE_NORMAL, bindingTableIndex);
        VISA_RawOpnd* UNIInput = GetRawSource(uniInputVar);
        VISA_RawOpnd* FBRInput = GetRawSource(fbrInputVar);
        VISA_VectorOpnd* FBRMbMode = GetSourceOperand(FBRMbModeVar, m_encoderState.m_srcOperand[0]);
        VISA_VectorOpnd* FBRSubMbShape = GetSourceOperand(FBRSubMbShapeVar, m_encoderState.m_srcOperand[1]);
        VISA_VectorOpnd* FBRSubPredMode = GetSourceOperand(FBRSubPredModeVar, m_encoderState.m_srcOperand[2]);
        VISA_RawOpnd* output = GetRawDestination(outputVar);

        V(vKernel->AppendVISAMiscVME_FBR(surface, UNIInput, FBRInput, FBRMbMode, FBRSubMbShape, FBRSubPredMode, output));
    }

    void CEncoder::SendVmeSic(
        CVariable* bindingTableIndex,
        CVariable* uniInputVar,
        CVariable* sicInputVar,
        CVariable* outputVar)
    {
        VISA_StateOpndHandle* surface = GetVISASurfaceOpnd(ESURFACE_NORMAL, bindingTableIndex);
        VISA_RawOpnd* UNIInput = GetRawSource(uniInputVar);
        VISA_RawOpnd* SICInput = GetRawSource(sicInputVar);
        VISA_RawOpnd* output = GetRawDestination(outputVar);

        V(vKernel->AppendVISAMiscVME_SIC(surface, UNIInput, SICInput, output));
    }

    void CEncoder::SendVideoAnalytic(
        llvm::GenIntrinsicInst* inst,
        CVariable* vaResult,
        CVariable* coords,
        CVariable* size,
        CVariable* srcImg,
        CVariable* sampler)
    {
        VISA_RawOpnd* vaOutput = GetRawDestination(vaResult);

        SModifier mod0 = m_encoderState.m_srcOperand[0];
        SModifier mod1 = m_encoderState.m_srcOperand[1];

        mod0.specialRegion = mod1.specialRegion = true;
        mod0.region[0] = mod1.region[0] = 0;
        mod0.region[1] = mod1.region[1] = 1;
        mod0.region[2] = mod1.region[2] = 0;
        mod0.subReg = 0;
        mod0.subVar = 0;

        if (coords->IsUniform())
        {
            mod1.subReg = 1;
            mod1.subVar = 0;
        }
        else
        {
            mod1.subReg = 0;
            mod1.subVar = 2;
        }

        VISA_VectorOpnd* uOffset = GetSourceOperand(coords, mod0);
        VISA_VectorOpnd* vOffset = GetSourceOperand(coords, mod1);

        if (size && size->IsUniform())
        {
            mod1.subReg = 1;
            mod1.subVar = 0;
        }
        else
        {
            mod1.subReg = 0;
            mod1.subVar = 2;
        }

        VISA_VectorOpnd* wSize = (size ? GetSourceOperand(size, mod0) : NULL);
        VISA_VectorOpnd* hSize = (size ? GetSourceOperand(size, mod1) : NULL);

        // So far we support only one VA function per kernel, and other sample
        // messages are not supported when there is VA function within the kernel.
        // So, for now it should be fine to always use bti 0 for VA functions.
        DWORD btiIndex = 0;
        DWORD mmfMode = 0;

        VISA_StateOpndHandle* surfaceOpnd = GetBTIOperand(btiIndex);
        VISA_StateOpndHandle* samplerHnd = GetSamplerOperand(sampler);
        VISA_VectorOpnd* mmModeOpnd = NULL;

        EDMode erodeDilateMode = VA_DILATE;
        EDExecMode execMode = VA_ED_64x4;
        bool isBigKernel = true;

        if (m_program->m_Platform->GetPlatformFamily() == IGFX_GEN8_CORE)
        {
            isBigKernel = false;
        }

        switch (inst->getIntrinsicID())
        {
        case GenISAIntrinsic::GenISA_vaErode:
            erodeDilateMode = VA_ERODE;
        case GenISAIntrinsic::GenISA_vaDilate:
            V(vKernel->AppendVISAVAErodeDilate(erodeDilateMode, samplerHnd, surfaceOpnd, uOffset, vOffset, execMode, vaOutput));
            break;
        case GenISAIntrinsic::GenISA_vaMinMaxFilter:
            V(vKernel->CreateVISAImmediate(mmModeOpnd, &mmfMode, ISA_TYPE_UD));
            V(vKernel->AppendVISAVAMinMaxFilter(samplerHnd, surfaceOpnd, uOffset, vOffset, AVS_16_FULL, VA_MMF_16x4, mmModeOpnd, vaOutput));
            break;
        case GenISAIntrinsic::GenISA_vaConvolveGRF_16x1:
            V(vKernel->AppendVISAVAConvolve(samplerHnd, surfaceOpnd, uOffset, vOffset, VA_CONV_16x1, isBigKernel, vaOutput));
            break;
        case GenISAIntrinsic::GenISA_vaConvolve:
        case GenISAIntrinsic::GenISA_vaConvolveGRF_16x4:
            V(vKernel->AppendVISAVAConvolve(samplerHnd, surfaceOpnd, uOffset, vOffset, VA_CONV_16x4, isBigKernel, vaOutput));
            break;
        case GenISAIntrinsic::GenISA_vaMinMax:
            V(vKernel->CreateVISAImmediate(mmModeOpnd, &mmfMode, ISA_TYPE_UD));
            V(vKernel->AppendVISAVAMinMax(surfaceOpnd, uOffset, vOffset, mmModeOpnd, vaOutput));
            break;
        case GenISAIntrinsic::GenISA_vaCentroid:
            V(vKernel->AppendVISAVACentroid(surfaceOpnd, uOffset, vOffset, wSize, vaOutput));
            break;
        case GenISAIntrinsic::GenISA_vaBoolCentroid:
        case GenISAIntrinsic::GenISA_vaBoolSum:
            V(vKernel->AppendVISAVABooleanCentroid(surfaceOpnd, uOffset, vOffset, wSize, hSize, vaOutput));
            break;
        default:
            IGC_ASSERT_MESSAGE(0, "Trying to emit unrecognized video analytic instruction (listed above)");
            break;
        };
    }

    void CEncoder::SetVISAWaTable(WA_TABLE const& waTable)
    {
        // Copy from driver WA table to VISA WA table,
        // then update the conditional W/A
        m_vISAWaTable = waTable;

        if (m_program->GetShaderType() != ShaderType::PIXEL_SHADER &&
            m_program->GetShaderType() != ShaderType::COMPUTE_SHADER &&
            m_program->GetShaderType() != ShaderType::OPENCL_SHADER)
        {
            m_vISAWaTable.WaClearTDRRegBeforeEOTForNonPS = waTable.WaClearTDRRegBeforeEOTForNonPS;
        }
        else
        {
            m_vISAWaTable.WaClearTDRRegBeforeEOTForNonPS = false;
        }

        if (IGC_IS_FLAG_DISABLED(ForceSendsSupportOnSKLA0))
        {
            m_vISAWaTable.WaDisableSendsSrc0DstOverlap = waTable.WaDisableSendsSrc0DstOverlap;
        }
        else
        {
            m_vISAWaTable.WaDisableSendsSrc0DstOverlap = false;
        }

        TODO("Limit this C0 WA as required to only Compute , as it causes hangs in some 3D Workloads");
        if (IGC_IS_FLAG_DISABLED(DisableWaSendSEnableIndirectMsgDesc) &&
            (m_program->GetShaderType() == ShaderType::COMPUTE_SHADER ||
                m_program->GetShaderType() == ShaderType::OPENCL_SHADER))
        {
            m_vISAWaTable.WaSendSEnableIndirectMsgDesc = waTable.WaSendSEnableIndirectMsgDesc;
        }
        else
        {
            m_vISAWaTable.WaSendSEnableIndirectMsgDesc = false;
        }

        if (IGC_IS_FLAG_DISABLED(DisableWaDisableSIMD16On3SrcInstr))
        {
            m_vISAWaTable.WaDisableSIMD16On3SrcInstr = waTable.WaDisableSIMD16On3SrcInstr;
        }
        else
        {
            m_vISAWaTable.WaDisableSIMD16On3SrcInstr = false;
        }
    }

    void CEncoder::GetRowAndColOffset(CVariable* var, unsigned int subVar, unsigned int subReg, unsigned char& rowOff, unsigned char& colOff)
    {
        IGC_ASSERT(nullptr != var);
        unsigned int varTypeSize = GetCISADataTypeSize(var->GetType());
        unsigned int offset = var->GetAliasOffset() + subVar * getGRFSize() + subReg * varTypeSize;
        IGC_ASSERT(0 < getGRFSize());
        IGC_ASSERT(0 < varTypeSize);
        IGC_ASSERT_MESSAGE((offset % getGRFSize()) % varTypeSize == 0, "offset has to be aligned on element size");
        rowOff = int_cast<unsigned char>(offset / getGRFSize());
        colOff = int_cast<unsigned char>((offset % getGRFSize()) / varTypeSize);
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

    void CEncoder::DebugLinePlaceholder()
    {
        V(vKernel->AppendVISADebugLinePlaceholder());
    }

    GenPrecision ConvertPrecisionToVisaType(PrecisionType P)
    {
        switch (P) {
        default: break;
        case PrecisionType::S2: return GenPrecision::S2;
        case PrecisionType::S4: return GenPrecision::S4;
        case PrecisionType::S8: return GenPrecision::S8;
        case PrecisionType::U2: return GenPrecision::U2;
        case PrecisionType::U4: return GenPrecision::U4;
        case PrecisionType::U8: return GenPrecision::U8;
        case PrecisionType::BF16: return GenPrecision::BF16;
        case PrecisionType::FP16: return GenPrecision::FP16;
        }

        return GenPrecision::INVALID;
    }


    void CEncoder::dpas(
        CVariable* dst, CVariable* input,
        CVariable* weight, PrecisionType weight_precision,
        CVariable* activation, PrecisionType activation_precision,
        uint8_t systolicDepth, uint8_t repeatCount,
        bool IsDpasw)
    {
        SModifier noMod; // Default is no mod.
        noMod.init();
        // PrecisionType to GenPrecision
        GenPrecision src1Precision = ConvertPrecisionToVisaType(weight_precision);
        GenPrecision src2Precision = ConvertPrecisionToVisaType(activation_precision);

        VISA_EMask_Ctrl execMask = GetAluEMask(dst);
        VISA_Exec_Size execSize = EXEC_SIZE_8;
        {
            VISA_RawOpnd* dstOpnd = GetRawDestination(dst);
            VISA_RawOpnd* srcOpnd0 = GetRawSource(input);
            VISA_RawOpnd* srcOpnd1 = GetRawSource(weight);
            VISA_VectorOpnd* srcOpnd2 = GetSourceOperand(activation, noMod);
            V(vKernel->AppendVISADpasInst(
                IsDpasw ? ISA_DPASW : ISA_DPAS,
                execMask,
                execSize,
                dstOpnd,
                srcOpnd0,
                srcOpnd1,
                srcOpnd2,
                src2Precision,
                src1Precision,
                systolicDepth,
                repeatCount));
        }
    }

    void CEncoder::QWGather(CVariable* dst,
        const ResourceDescriptor& resource,
        CVariable* offset,
        unsigned elemSize,
        unsigned numElems)
    {
        IGC_ASSERT_MESSAGE(elemSize == 64, "Only QWord element is supported!");
        IGC_ASSERT_MESSAGE((numElems == 1) || (numElems == 2) || (numElems == 4),
            "Only 1/2/4 elements are supported!");

        VISA_StateOpndHandle* surfaceOpnd = GetVISASurfaceOpnd(resource);
        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
        VISA_RawOpnd* addressOpnd = GetRawSource(offset);
        VISA_RawOpnd* dstOpnd = GetRawDestination(dst);

        V(vKernel->AppendVISAQwordGatherInst(
            predOpnd,
            GetAluEMask(offset),
            visaExecSize(offset->IsUniform() ? lanesToSIMDMode(offset->GetNumberElement()) :
                m_encoderState.m_simdSize),
            visaBlockNum(numElems),
            surfaceOpnd,
            addressOpnd, dstOpnd));
    }

    void CEncoder::QWScatter(CVariable* src,
        const ResourceDescriptor& resource,
        CVariable* offset,
        unsigned elemSize,
        unsigned numElems)
    {
        IGC_ASSERT_MESSAGE(elemSize == 64, "Only QWord element is supported");
        IGC_ASSERT_MESSAGE((numElems == 1) || (numElems == 2) || (numElems == 4),
            "Only 1/2/4 elements are supported!");

        VISA_StateOpndHandle* surfaceOpnd = GetVISASurfaceOpnd(resource);
        VISA_PredOpnd* predOpnd = GetFlagOperand(m_encoderState.m_flag);
        VISA_RawOpnd* addressOpnd = GetRawSource(offset);
        VISA_RawOpnd* srcOpnd = GetRawSource(src);

        V(vKernel->AppendVISAQwordScatterInst(
            predOpnd,
            GetAluEMask(offset),
            visaExecSize(offset->IsUniform() ? lanesToSIMDMode(offset->GetNumberElement()) :
                m_encoderState.m_simdSize),
            visaBlockNum(numElems),
            surfaceOpnd,
            addressOpnd, srcOpnd));
    }


    std::string CEncoder::GetVariableName(CVariable* var)
    {
        IGC_ASSERT(nullptr != var);
        if (var->IsImmediate())
        {
            std::stringstream temp;
            temp << "0x" << std::hex << var->GetImmediateValue() << ":" << CISATypeTable[var->GetType()].typeName;
            return temp.str();
        }
        switch (var->GetVarType())
        {
        case EVARTYPE_GENERAL:
            return vKernel->getVarName(GetVISAVariable(var));
        case EVARTYPE_PREDICATE:
            return vKernel->getVarName(var->visaPredVariable);
        case EVARTYPE_ADDRESS:
            return vKernel->getVarName(var->visaAddrVariable);
        case EVARTYPE_SURFACE:
            return vKernel->getVarName(var->visaSurfVariable);
        case EVARTYPE_SAMPLER:
            return vKernel->getVarName(var->visaSamplerVariable);
        default:
            IGC_ASSERT_MESSAGE(0, "Unknown var type");
            return "";
        }
    }

    std::string CEncoder::GetDumpFileName(std::string extension)
    {
        std::string filename = IGC::Debug::GetDumpName(m_program, extension.c_str());
        return filename;
    }


    void CEncoder::ReportCompilerStatistics(VISAKernel* pMainKernel, SProgramOutput* pOutput)
    {
        CompilerStats compilerStats;
        pMainKernel->GetCompilerStats(compilerStats);
        int simdsize = GetThreadCount(m_program->m_dispatchSize);


        // set optional statistics
        if (compilerStats.Find(CompilerStats::numCyclesStr()))
        {
            pOutput->m_NumCycles.emplace(compilerStats.GetI64(CompilerStats::numCyclesStr(), simdsize));
        }

        if (compilerStats.Find(CompilerStats::numGRFFillStr()))
        {
            pOutput->m_NumGRFFill.emplace(compilerStats.GetI64(CompilerStats::numGRFFillStr(), simdsize));
        }

        if (compilerStats.Find(CompilerStats::numGRFSpillStr()))
        {
            pOutput->m_NumGRFSpill.emplace(compilerStats.GetI64(CompilerStats::numGRFSpillStr(), simdsize));
        }

        if (compilerStats.Find(CompilerStats::numSendStr()))
        {
            pOutput->m_NumSends.emplace(compilerStats.GetI64(CompilerStats::numSendStr(), simdsize));
        }
        FINALIZER_INFO* jitInfo = nullptr;
        if (0 == pMainKernel->GetJitInfo(jitInfo))
        {
            uint sendStallCycle = 0;
            for (uint i = 0; i < jitInfo->BBNum; i++)
            {
                sendStallCycle += jitInfo->BBInfo[i].sendStallCycle;
            }
            pOutput->m_NumSendStallCycles.emplace(sendStallCycle);
        }
    }

    int CEncoder::GetThreadCount(SIMDMode simdMode)
    {
        int simdsize = 0;
        switch (m_program->m_dispatchSize)
        {
        case SIMDMode::SIMD8:
            simdsize = 8;
            break;
        case SIMDMode::SIMD16:
            simdsize = 16;
            break;
        case SIMDMode::SIMD32:
            simdsize = 32;
            break;
        default:
            break;
        }
        return simdsize;
    }

}
