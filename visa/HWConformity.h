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

#ifndef _HWCONFORMITY_H_
#define _HWCONFORMITY_H_

#include "Gen4_IR.hpp"
#include "BuildIR.h"
#include "FlowGraph.h"
#include "Common_ISA_util.h"

#include <map>
namespace vISA
{
    class HWConformity
    {
        IR_Builder& builder;
        G4_Kernel& kernel;
        vISA::Mem_Manager& mem;
        int numAccSubDef = 0;
        int numAccSubUse = 0;


        // This is added for data layout optimization.
        // Currently it only targets packed-byte pattern.
        // Can be extended later for other patterns.
        enum AccessPattern {
            ACCESS_PATTERN_UNDEF = 0,
            ACCESS_PATTERN_PACKED_BYTE = 1,
            ACCESS_PATTERN_INVALID = 2
        };
        std::map<G4_Declare*, AccessPattern> dclAccessPattern;

        AccessPattern getAccessPattern(G4_Declare* dcl)
        {
            auto iter = dclAccessPattern.find(dcl);
            if (iter == dclAccessPattern.end())
            {
                return ACCESS_PATTERN_UNDEF;
            }
            return (*iter).second;
        }
        void setAccessPattern(G4_Declare* dcl, AccessPattern ap)
        {
            dclAccessPattern[dcl] = ap;
        }
        bool markPackedByteReference(G4_Kernel& kernel, G4_Operand* opnd, G4_INST* inst);
        G4_Operand* fixPackedByteReference(IR_Builder& builder, G4_Operand* opnd);

        // helper functions
        bool hasBadRegion(G4_INST *inst);
        bool canSplitInst(G4_INST *inst, G4_INST *use_op);
        bool splitInstListForByteDst(INST_LIST_ITER it, G4_BB *bb, uint16_t extypesize);

        G4_DstRegRegion* insertMovAfter(INST_LIST_ITER& it, G4_DstRegRegion* dst, G4_Type type, G4_BB *bb, G4_SubReg_Align dstAlign = Any);
        G4_Operand* insertMovBefore(INST_LIST_ITER it, uint32_t srcNum, G4_Type type, G4_BB *bb,
            G4_SubReg_Align tmpAlign = Any);
        G4_SrcRegRegion* insertCopyBefore(INST_LIST_ITER it, uint32_t srcNum, G4_SubReg_Align tmpAlign, G4_BB *bb);
        void broadcast(G4_BB* bb, INST_LIST_ITER it, int srcPos, G4_SubReg_Align subAlign);

        G4_INST *splitInstWithByteDst(G4_INST *expand_op);
        G4_Align getDclAlignment(int opndBytes, G4_INST *inst, bool isScalar, G4_SubReg_Align &subAlign);
        bool isPackedWord(G4_Operand *src);

        // HW conformity check functions
        void fixPackedSource(INST_LIST_ITER it, G4_BB *bb, G4_Type extype);
        bool fixMathInst(INST_LIST_ITER it, G4_BB *bb);
        bool fixMULInst(INST_LIST_ITER &it, G4_BB *bb);
        void fixMULHInst(INST_LIST_ITER &i, G4_BB *bb);
        void fixMulSrc1(INST_LIST_ITER i, G4_BB* bb);
        void splitDWMULInst(INST_LIST_ITER &start, INST_LIST_ITER &end, G4_BB *bb);
        void fixOpnds(INST_LIST_ITER it, G4_BB *bb, G4_Type& exType);
        bool fixLine(INST_LIST_ITER it, G4_BB *bb);
        bool fixOpndType(INST_LIST_ITER it, G4_BB *bb);
        void fixPackedHFConversions(INST_LIST_ITER it, G4_BB* bb);

        bool fixIndirectOpnd(INST_LIST_ITER i, G4_BB *bb);
        G4_Type getNonVectorType(G4_Type srcType);
        void fix3SrcInst(INST_LIST_ITER i, G4_BB* bb);
        void fixAlign13SrcInst(INST_LIST_ITER i, G4_BB* bb);
        void fixCompareInst(INST_LIST_ITER i, G4_BB *bb, G4_Type exType, int dst_elsize);
        bool fixDstAlignment(INST_LIST_ITER i, G4_BB *bb, G4_Type extype, unsigned int dst_elsize);
        bool fixDstAlignmentWithVectorImm(INST_LIST_ITER i, G4_BB *bb);
        bool fixDstType(INST_LIST_ITER i, G4_BB *bb, G4_Type extype);
        bool fixAccDst(INST_LIST_ITER i, G4_BB* bb);
        bool fixAccSrc(INST_LIST_ITER i, G4_BB* bb);
        bool fixImplicitAcc(INST_LIST_ITER i, G4_BB* bb);
        void fixDstHstride(INST_LIST_ITER i, int extypesize);
        void fixMADInst(BB_LIST_ITER it);
        void fixSrcRegion(G4_INST *inst);
        void conformBB(BB_LIST_ITER it);
        void fixSADA2Inst(BB_LIST_ITER it);
        void fixMixedHFInst(BB_LIST_ITER it);
        void fixSendInst(BB_LIST_ITER it);
        bool canSplitByteDst(G4_opcode op);
        bool fixInstOpndTypeAlign(INST_LIST_ITER i, G4_BB* bb);
        void fixOpndTypeAlign(G4_BB* bb);
        void fixInstExecSize(BB_LIST_ITER it);
        bool reduceExecSize(INST_LIST_ITER iter, G4_BB* bb);
        bool reduceExecSizeForMath(INST_LIST_ITER iter, G4_BB* bb);
        void checkSrcDstOverlap(INST_LIST_ITER iter, G4_BB* bb, bool compOpt);
        void splitInstruction(INST_LIST_ITER iter, G4_BB* bb, bool compOpt, uint8_t numInFirstMov, bool rule4_11, bool allowSrcCrossGRF);
        void splitSIMD32Inst(INST_LIST_ITER iter, G4_BB* bb);
        void evenlySplitInst(INST_LIST_ITER iter, G4_BB* bb, bool checkOverlap = true);
        void moveSrcToGRF(INST_LIST_ITER it, uint32_t srcNum, uint16_t numGRF, G4_BB *bb);
        void saveDst(INST_LIST_ITER& it, uint8_t stride, G4_BB *bb);
        void restoreDst(INST_LIST_ITER& it, G4_DstRegRegion *origDst, G4_BB *bb);
        void insertMovAfter(INST_LIST_ITER& it, uint16_t stride, G4_BB* bb);
        void removeBadSrc(INST_LIST_ITER& it, G4_BB *bb, bool crossGRFDst, bool oneGRFSrc[3], bool badTwoGRFSrc[3]);
        G4_DstRegRegion* createSubDstOperand(G4_DstRegRegion* dst, uint16_t start, uint8_t size);
        G4_SrcRegRegion* createSubSrcOperand(G4_SrcRegRegion* src, uint16_t start, uint8_t size, uint16_t newVs, uint16_t newWd);
        uint8_t checkMinExecSize(G4_opcode op);
        // Create a new instrution for instruction splitting.
        G4_INST *makeSplittingInst(G4_INST *inst, uint8_t ExSize);
        bool checkInstRegions(G4_INST *inst, G4_BB *bb, bool &evenSplit);
        bool convertMAD2MAC(INST_LIST_ITER it, std::vector<G4_INST*> &madList, G4_BB *bb);
        void convertMAD2MulAdd(INST_LIST_ITER iter, G4_BB *bb);
        G4_Type getAccType(G4_Type ty);
        bool findHoistLocation(INST_LIST_ITER start, INST_LIST_ITER &end, uint16_t &movDist, G4_INST *boundary);
        void convertComprInstSrcRegion(G4_INST *inst);
        void addACCOpnd(G4_INST *inst, bool needACCSrc, int stride, G4_Type accTy);
        void maintainDU4TempMov(G4_INST *inst, G4_INST *movInst);
        void fixImm64(INST_LIST_ITER i, G4_BB* bb);
        bool checkSrcCrossGRF(INST_LIST_ITER &i, G4_BB* bb);
        G4_INST* checkSrcDefInst(G4_INST *inst, G4_INST *def_inst, uint32_t srcNum);
        void fix64bInst(INST_LIST_ITER i, G4_BB* bb);
        bool fixPlaneInst(INST_LIST_ITER i, G4_BB* bb);
        void expandPlaneInst(INST_LIST_ITER i, G4_BB* bb);
        bool fixAddcSubb(G4_BB* bb);
        void fixDataLayout();
        bool fixMov(INST_LIST_ITER i, G4_BB* bb);


        void helperGenerateTempDst(
            G4_BB *bb,
            INST_LIST_ITER instIter,
            G4_INST *inst,
            uint8_t hStride,
            G4_Type tempDstType,
            G4_SubReg_Align subAlign = Any);

        bool checkMixMode(INST_LIST_ITER &i, G4_BB* bb);

        bool isGoodAlign16Src(G4_INST* inst, int srcPos);
        bool isGoodAlign1TernarySrc(G4_INST* inst, int srcPos, bool canBeImm);
        bool isGoodAlign1TernaryDst(G4_INST* inst) const;
        void copyDwords(G4_Declare* dst,
            int dstOffset,
            G4_Declare* src,
            int srcOffset,
            int numDwords,
            G4_BB* bb,
            INST_LIST_ITER iter);

        void copyDwordsIndirect(G4_Declare* dst,
            G4_SrcRegRegion* src,
            int numDwords,
            G4_BB* bb,
            INST_LIST_ITER iter);

        void copyRegs(G4_Declare* dst,
            int dstOffset,
            G4_Declare* src,
            int srcOffset,
            int numRegs,
            G4_BB* bb,
            INST_LIST_ITER iter);

        bool isFpMadPreferred(G4_BB *bb, INST_LIST_ITER iter);
        bool generateFPMad(G4_BB* bb, INST_LIST_ITER iter);
        bool generateAlign1Mad(G4_BB* bb, INST_LIST_ITER iter);
        bool hasSameSubregOffset(G4_INST* inst) const;

        void fixImmAndARFSrc(INST_LIST_ITER it, G4_BB *bb);
        void generateMacl(INST_LIST_ITER it, G4_BB *bb);
        void doGenerateMacl(INST_LIST_ITER it, G4_BB *bb);

        void fixSelCsel(INST_LIST_ITER it, G4_BB *bb);

        void* operator new(size_t sz, vISA::Mem_Manager& m) { return m.alloc(sz); }
        void multiAccSubstitution(G4_BB* bb);

        bool checkSrcMod(INST_LIST_ITER it, G4_BB* bb, int srcPos);

    public:
        HWConformity(IR_Builder& b, G4_Kernel &k, vISA::Mem_Manager& m) :
            builder(b), kernel(k), mem(m)
        {
        }
        void chkHWConformity();
        static void tryEliminateMadSrcModifier(IR_Builder &builder, G4_INST *inst);
        void accSubstitution(G4_BB* bb);
        int getNumAccSubDef() const { return numAccSubDef; }
        int getNumAccSubUse() const { return numAccSubUse; }
    };
}
//single entry point for HW conformity checks
extern void HWConformityChk(vISA::IR_Builder& builder, vISA::G4_Kernel& kernel, vISA::Mem_Manager& mem);

extern CM_INLINE void checkHFMixModeRule4_11(vISA::G4_INST *inst, bool & rule4, bool & rule11) {
    /*
     * Also need to split for HF restrictions:
     *
     * 4. No simd16 in mixed mode when destination is packed f16 for
     *    both Align1 and Align16.  mad(8) r3.xyzw:hf r4.xyzw:f
     *    r6.xyzw:hf r7.xyzw:hf add(8) r20.0<1>:hf r3<8;8,1>:f
     *    r6.0<8;8,1>:hf {Q1}
     *
     * 11. [DevCHV, DevSKL, DevBXT]: No simd16 in mixed mode when
     *     destination is f32. Instruction Execution size must be no more
     *     than 8.
     */
    rule4 = (inst->getExecSize() > 8
             && inst->getDst()
             && inst->getDst()->getType() == Type_HF
             && getGenxPlatform() >= GENX_CHV ) ? true : false ;

    rule11 = (inst->getExecSize() > 8 && inst->opcode() != G4_mov &&
              inst->getDst() && inst->getDst()->getType() == Type_F &&
              (getGenxPlatform() == GENX_CHV || getGenxPlatform() == GENX_SKL ||
               getGenxPlatform() == GENX_BXT))
                 ? true
                 : false;
}


#endif /* _HWCONFORMITY_H_ */
