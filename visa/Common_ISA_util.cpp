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

#include <string.h>
#include <sstream>
#include "Common_ISA_util.h"
#include "Common_ISA_framework.h"
#include "G4_Opcode.h"
#include "PreDefinedVars.h"


vISAPreDefinedSurface vISAPreDefSurf[COMMON_ISA_NUM_PREDEFINED_SURF_VER_3_1] =
{
    { 0, PREDEF_SURF_0, "T0" },
    { 1, PREDEF_SURF_1, "T1" },
    { 2, PREDEF_SURF_2, "T2" },
    { 3, PREDEF_SURF_3, "TSS" },
    { 4, PREDEF_SURF_252, "T252" },
    { 5, PREDEF_SURF_255, "T255" },
};


const char* Common_ISA_Get_Align_Name(VISA_Align align)
{
    static const char* common_ISA_align_name[] = {
        " ",
        "word",
        "dword",
        "qword",
        "oword",
        "GRF",
        "GRFx2", // "2GRF"
        "byte",
    };

    return common_ISA_align_name[align];
}

const char* Common_ISA_Get_Modifier_Name(VISA_Modifier modifier)
{
    switch(modifier)
    {
        case MODIFIER_NONE    : return "";
        case MODIFIER_ABS     : return "(abs)";
        case MODIFIER_NEG     : return "(-)";
        case MODIFIER_NEG_ABS : return "(-abs)";
        case MODIFIER_SAT     : return "sat";
        case MODIFIER_NOT     : return "(~)";
        default:
            MUST_BE_TRUE(false, "Invalid modifier.");
            return "invalid_modifier";
    }
}

short Common_ISA_Get_Region_Value( Common_ISA_Region_Val val )
{
    switch( val ) {
        case REGION_NULL:
            return -1;
        case REGION_0:
            return 0;
        case REGION_1:
            return 1;
        case REGION_2:
            return 2;
        case REGION_4:
            return 4;
        case REGION_8:
            return 8;
        case REGION_16:
            return 16;
        case REGION_32:
            return 32;
        default:
            MUST_BE_TRUE(false, " illegal region value: " << (int)val);
            return -1;
    }
}

G4_opcode GetGenOpcodeFromVISAOpcode( ISA_Opcode opcode )
{
    switch(opcode)
    {
        case ISA_RESERVED_0:
            return G4_illegal;
        case ISA_ADD:
            return G4_add;
        case ISA_AVG:
            return G4_avg;
        case ISA_DIV:
            return G4_math;
        case ISA_DP2:
            return G4_dp2;
        case ISA_DP3:
            return G4_dp3;
        case ISA_DP4:
            return G4_dp4;
        case ISA_DPH:
            return G4_dph;
        case ISA_DP4A:
            return G4_dp4a;
        case ISA_EXP:
            return G4_math;
        case ISA_FRC:
            return G4_frc;
        case ISA_LINE:
            return G4_line;
        case ISA_LOG:
            return G4_math;
        case ISA_LRP:
            return G4_lrp;
        case ISA_MAD:
            return G4_pseudo_mad;
        case ISA_MOD:
            return G4_math;
        case ISA_MUL:
            return G4_mul;
        case ISA_PLANE:
            return G4_pln;
        case ISA_POW:
            return G4_math;
        case ISA_RNDD:
            return G4_rndd;
        case ISA_RNDE:
            return G4_rnde;
        case ISA_RNDU:
            return G4_rndu;
        case ISA_RNDZ:
            return G4_rndz;
        case ISA_SAD2:
            return G4_sad2;
        case ISA_SAD2ADD:
            return G4_pseudo_sada2;
        case ISA_SIN:
            return G4_math;
        case ISA_COS:
            return G4_math;
        case ISA_SQRT:
            return G4_math;
        case ISA_RSQRT:
            return G4_math;
        case ISA_INV:
            return G4_math;
        case ISA_LZD:
            return G4_lzd;
        case ISA_AND:
            return G4_and;
        case ISA_OR:
            return G4_or;
        case ISA_XOR:
            return G4_xor;
        case ISA_NOT:
            return G4_not;
        case ISA_SHL:
            return G4_shl;
        case ISA_SHR:
            return G4_shr;
        case ISA_ASR:
            return G4_asr;
        case ISA_ROL:
            return G4_rol;
        case ISA_ROR:
            return G4_ror;
        case ISA_BFE:
            return G4_bfe;
        case ISA_BFI:
            return G4_bfi1;
        case ISA_BFREV:
            return G4_bfrev;
        case ISA_CBIT:
            return G4_cbit;
        case ISA_FBL:
            return G4_fbl;
        case ISA_FBH:
            return G4_fbh;
        case ISA_ADDR_ADD:
            return G4_add;
        case ISA_MOV:
            return G4_mov;
        case ISA_SEL:
        case ISA_FMINMAX:
            return G4_sel;
        case ISA_SETP:
            break;
        case ISA_CMP:
            return G4_cmp;
        case ISA_SUBROUTINE:
            break;
        case ISA_LABEL:
            return G4_label;
        case ISA_JMP:
            return G4_jmpi;
        case ISA_CALL:
            return G4_call;
        case ISA_RET:
            return G4_return;
        case ISA_MULH:
            return G4_mulh;
        case ISA_ADDC:
            return G4_addc;
        case ISA_SUBB:
            return G4_subb;
        case ISA_OWORD_LD:
        case ISA_OWORD_ST:
        case ISA_MEDIA_LD:
        case ISA_MEDIA_ST:
        case ISA_GATHER:
        case ISA_SCATTER:
        case ISA_OWORD_LD_UNALIGNED:
        case ISA_SAMPLE:
        case ISA_SAMPLE_UNORM:
        case ISA_FILE:
        case ISA_LOC:
        case ISA_DWORD_ATOMIC:
            break;
        case ISA_GOTO:
            return G4_goto;
        default:
            MUST_BE_TRUE(0, "Invalid opcode in common ISA.");
            break;
    }
    return G4_illegal;
}

G4_Type GetGenTypeFromVISAType( VISA_Type type )
{
    switch(type)
    {
    case ISA_TYPE_UD:
        return Type_UD;
    case ISA_TYPE_D:
        return Type_D;
    case ISA_TYPE_UW:
        return Type_UW;
    case ISA_TYPE_W:
        return Type_W;
    case ISA_TYPE_UB:
        return Type_UB;
    case ISA_TYPE_B:
        return Type_B;
    case ISA_TYPE_DF:
        return Type_DF;
    case ISA_TYPE_F:
        return Type_F;
    case ISA_TYPE_VF:
        return Type_VF;
    case ISA_TYPE_V:
        return Type_V;
    case ISA_TYPE_BOOL:
        return Type_BOOL;
    case ISA_TYPE_UV:
        return Type_UV;
    case ISA_TYPE_Q:
        return Type_Q;
    case ISA_TYPE_UQ:
        return Type_UQ;
    case ISA_TYPE_HF:
        return Type_HF;
    default:
        return Type_UNDEF;
    }
}

VISA_Type Get_Common_ISA_Type_From_G4_Type( G4_Type type )
{
    switch(type)
    {
    case Type_UD:
        return ISA_TYPE_UD;
    case Type_D:
        return ISA_TYPE_D;
    case Type_UW:
        return ISA_TYPE_UW;
    case Type_W:
        return ISA_TYPE_W;
    case Type_UB:
        return ISA_TYPE_UB;
    case Type_B:
        return ISA_TYPE_B;
    case Type_DF:
        return ISA_TYPE_DF;
    case Type_F:
        return ISA_TYPE_F;
    case Type_VF:
        return ISA_TYPE_VF;
    case Type_V:
        return ISA_TYPE_V;
    case Type_BOOL:
        return ISA_TYPE_BOOL;
    case Type_UV:
        return ISA_TYPE_UV;
    case Type_Q:
        return ISA_TYPE_Q;
    case Type_UQ:
        return ISA_TYPE_UQ;
    case Type_HF:
        return ISA_TYPE_HF;
    default:
        return ISA_TYPE_NUM;
    }
}

G4_SubReg_Align Get_G4_SubRegAlign_From_Type( G4_Type ty )
{
    switch(ty)
    {
    case Type_B:
    case Type_UB:
    case Type_W:
    case Type_UW:
        return Any;
    case Type_UD:
    case Type_D:
    case Type_F:
        return Even_Word;
    case Type_DF:
        return Four_Word;
    case Type_V:
    case Type_VF:
    case Type_UV:
        return Eight_Word;
    case Type_Q:
    case Type_UQ:
        return Four_Word;
    default:
        return Any;
    }
}

// size is the number of byte
G4_SubReg_Align Get_G4_SubRegAlign_From_Size( uint16_t size )
{
    switch( size )
    {
    case 1:
    case 2:
        return Any;
    case 4:
        return Even_Word;
    case 8:
        if (getGenxPlatform() != GENX_BXT)
          return Four_Word;
        // FALL THROUGH
        // WA: It's a workaround where a potential HW issue needs
        // identifying.
    case 16:
        return Eight_Word;
    case 32:
        return Sixteen_Word;
    default:
        return GRFALIGN;
    }
}

G4_SrcModifier GetGenSrcModFromVISAMod( VISA_Modifier mod )
{
    switch( mod )
    {
    case MODIFIER_NONE:
        return Mod_src_undef;
    case MODIFIER_ABS:
        return Mod_Abs;
    case MODIFIER_NEG:
        return Mod_Minus;
    case MODIFIER_NEG_ABS:
        return Mod_Minus_Abs;
    case MODIFIER_NOT:
        return Mod_Not;
    default:
        MUST_BE_TRUE(0, "Wrong src modifier");
        return Mod_src_undef;
    }
}

G4_CondModifier Get_G4_CondModifier_From_Common_ISA_CondModifier( VISA_Cond_Mod cmod )
{
    switch(cmod){
        case ISA_CMP_E:
            return Mod_e;
        case ISA_CMP_NE:
            return Mod_ne;
        case ISA_CMP_G:
            return Mod_g;
        case ISA_CMP_GE:
            return Mod_ge;
        case ISA_CMP_L:
            return Mod_l;
        case ISA_CMP_LE:
            return Mod_le;
        case ISA_CMP_UNDEF:
            return Mod_cond_undef;
        default:
            MUST_BE_TRUE( 0, "Invalid CISA Conditional Modifier." );
            return Mod_cond_undef;
    }
}

bool hasPredicate(ISA_Opcode op)
{
    switch(ISA_Inst_Table[op].type)
    {
    case ISA_Inst_Mov:
        return !(op == ISA_SETP || op == ISA_MOVS || op == ISA_FMINMAX);
    case ISA_Inst_Arith:
    case ISA_Inst_Logic:
        return true;
    case ISA_Inst_Compare:
    case ISA_Inst_Address:
    case ISA_Inst_Data_Port:
    case ISA_Inst_Sampler:
    case ISA_Inst_Misc:
        return (op == ISA_DWORD_ATOMIC || op == ISA_GATHER_SCALED ||
                op == ISA_GATHER4_SCALED || op == ISA_GATHER4_TYPED ||
                op == ISA_SCATTER_SCALED || op == ISA_SCATTER4_SCALED ||
                op == ISA_SCATTER4_TYPED || op == ISA_RAW_SEND ||
                op == ISA_RAW_SENDS || op == ISA_3D_SAMPLE ||
                op == ISA_3D_LOAD || op == ISA_3D_GATHER4 ||
                op == ISA_3D_RT_WRITE || op == ISA_3D_URB_WRITE ||
                op == ISA_3D_TYPED_ATOMIC
                );
    case ISA_Inst_Flow:
        return !(op == ISA_SUBROUTINE || op == ISA_LABEL || op == ISA_SWITCHJMP);
    case ISA_Inst_SIMD_Flow:
        return op == ISA_GOTO;
    default:
        return false;
    }
}

bool hasExecSize(ISA_Opcode op, uint8_t subOp)
{
    switch(ISA_Inst_Table[op].type)
    {
        case ISA_Inst_Mov:
        case ISA_Inst_Arith:
        case ISA_Inst_Logic:
        case ISA_Inst_Compare:
        case ISA_Inst_Address:
            return true;
        case ISA_Inst_Data_Port:
            if( op == ISA_MEDIA_LD || op == ISA_MEDIA_ST ){
                return false;
            }else
                return true;
        case ISA_Inst_SVM:
            if (subOp == SVM_BLOCK_LD || subOp == SVM_BLOCK_ST || subOp == 0) {
                return false;
            } else
                return true;
        case ISA_Inst_Sampler:
        case ISA_Inst_Misc:
            if( op == ISA_RAW_SEND || op == ISA_RAW_SENDS || op == ISA_3D_SAMPLE ||
                op == ISA_3D_LOAD || op == ISA_3D_GATHER4 || op == ISA_3D_URB_WRITE ||
                op == ISA_3D_INFO)
            {
                return true;
            }
            else
                return false;
        case ISA_Inst_Flow:
            if( op == ISA_SUBROUTINE || op == ISA_LABEL || op == ISA_FADDR)
            {
                return false;
            }
            else
                return true;
        case ISA_Inst_SIMD_Flow:
            return true;
        default:
            return false;
    }
}


bool hasLabelSrc(ISA_Opcode op)
{
    if( ISA_Inst_Table[op].type == ISA_Inst_Flow)
    {
        if (op == ISA_RET || op == ISA_FRET || op == ISA_IFCALL || op == ISA_FADDR)
            return false;
        else //( op == ISA_SUBROUTINE || op == ISA_LABEL || op == ISA_JMP || op == ISA_CALL || op == ISA_FCALL )
            return true;
    }
    else if( op == ISA_GOTO )
        return true;
    else
        return false;
}

unsigned Get_Common_ISA_SVM_Block_Num(VISA_SVM_Block_Num num)
{
    switch (num)
    {
        case SVM_BLOCK_NUM_1: return 1;
        case SVM_BLOCK_NUM_2: return 2;
        case SVM_BLOCK_NUM_4: return 4;
        case SVM_BLOCK_NUM_8: return 8;
        default: MUST_BE_TRUE(false, "Illegal SVM block number (should be 1, 2, 4, or 8).");
    }
    return 0;
}

VISA_SVM_Block_Num valueToVISASVMBlockNum(unsigned int value)
{
    switch (value)
    {
    case 1:
        return SVM_BLOCK_NUM_1;
    case 2:
        return SVM_BLOCK_NUM_2;
    case 4:
        return SVM_BLOCK_NUM_4;
    case 8:
        return SVM_BLOCK_NUM_8;
    default:
        MUST_BE_TRUE(false, "invalid SVM block number");
        return SVM_BLOCK_NUM_1;
    }
}

VISA_SVM_Block_Type valueToVISASVMBlockType(unsigned int value)
{
    switch (value)
    {
    case 1:
        return SVM_BLOCK_TYPE_BYTE;
    case 4:
        return SVM_BLOCK_TYPE_DWORD;
    case 8:
        return SVM_BLOCK_TYPE_QWORD;

    default:
        MUST_BE_TRUE(false, "invalid SVM block number");
        return SVM_BLOCK_TYPE_BYTE;
    }

}

unsigned Get_Common_ISA_SVM_Block_Size(VISA_SVM_Block_Type size)
{
    switch (size)
    {
        case SVM_BLOCK_TYPE_BYTE: return 1;
        case SVM_BLOCK_TYPE_DWORD: return 4;
        case SVM_BLOCK_TYPE_QWORD: return 8;
        default: MUST_BE_TRUE(false, "Illegal SVM block size (should be 1, 4, or 8).");
    }
    return 0;
}

unsigned Get_VISA_Oword_Num( VISA_Oword_Num num )
{
    switch(num){
        case OWORD_NUM_1:
            return 1;
        case OWORD_NUM_2:
            return 2;
        case OWORD_NUM_4:
            return 4;
        case OWORD_NUM_8:
            return 8;
        case OWORD_NUM_16:
            return 16;
        default:
            MUST_BE_TRUE( false, "illegal Oword number (should be 0..3)." );
            return 0;
    }
}

unsigned Get_VISA_Exec_Size( VISA_Exec_Size size )
{
    switch(size){
        case EXEC_SIZE_1:
            return 1;
        case EXEC_SIZE_2:
            return 2;
        case EXEC_SIZE_4:
            return 4;
        case EXEC_SIZE_8:
            return 8;
        case EXEC_SIZE_16:
            return 16;
        case EXEC_SIZE_32:
            return 32;
        default:
            MUST_BE_TRUE( false, "illegal common ISA execsize (should be 0..5)." );
            return 0;
    }
}

bool IsMathInst(ISA_Opcode op)
{
    switch(op){
        case ISA_INV:
        case ISA_DIV:
        case ISA_MOD:
        case ISA_LOG:
        case ISA_EXP:
        case ISA_SQRT:
        case ISA_RSQRT:
        case ISA_SIN:
        case ISA_COS:
        case ISA_POW:
            return true;
        default:
            return false;
    }
}

bool IsIntType(VISA_Type type)
{
    switch(type){
        case ISA_TYPE_UD:
        case ISA_TYPE_D:
        case ISA_TYPE_UW:
        case ISA_TYPE_W:
        case ISA_TYPE_UB:
        case ISA_TYPE_B:
        case ISA_TYPE_Q:
        case ISA_TYPE_UQ:
            return true;
        default:
            return false;
    }
}

bool IsIntOrIntVecType(VISA_Type type)
{
    return type == ISA_TYPE_V || type == ISA_TYPE_UV || IsIntType(type);
}

bool IsSingedIntType(VISA_Type type)
{
    switch (type) {
        case ISA_TYPE_D:
        case ISA_TYPE_W:
        case ISA_TYPE_B:
        case ISA_TYPE_Q:
            return true;
        default:
            return false;
    }
}

bool IsUnsignedIntType(VISA_Type type)
{
    switch (type) {
        case ISA_TYPE_UD:
        case ISA_TYPE_UW:
        case ISA_TYPE_UB:
        case ISA_TYPE_UQ:
            return true;
        default:
            return false;
    }
}

unsigned short Get_Common_ISA_Region_Value(Common_ISA_Region_Val val)
{
    switch( val ){
    case REGION_0:
        return 0;
    case REGION_1:
        return 1;
    case REGION_2:
        return 2;
    case REGION_4:
        return 4;
    case REGION_8:
        return 8;
    case REGION_16:
        return 16;
    case REGION_32:
        return 32;
    default:
        return UNDEFINED_SHORT;  //???
    }
}

Common_ISA_Region_Val Get_CISA_Region_Val( short val )
{
    if( val == (short)0x8000 ){
        return REGION_NULL;
    }else{
        switch(val){
            case 0: return REGION_0;
            case 1: return REGION_1;
            case 2: return REGION_2;
            case 4: return REGION_4;
            case 8: return REGION_8;
            case 16: return REGION_16;
            case 32: return REGION_32;
            case -1:
                return REGION_NULL;
            default:
                MUST_BE_TRUE( 0, "Invalid Region value." );
                return REGION_NULL;
        }
    }
}

unsigned short Create_CISA_Region(
            unsigned short vstride,
            unsigned short width,
            unsigned short hstride)
{
    unsigned short region = 0;
    region |= (unsigned short)Get_CISA_Region_Val(vstride) & 0xF;
    region |= ((unsigned short)Get_CISA_Region_Val(width) & 0xF) << 4;
    region |= ((unsigned short)Get_CISA_Region_Val(hstride) & 0xF) << 8;
    return region;
}

unsigned Round_Down_Pow2(unsigned n)
{
    unsigned int i = 1;
    while (n >= i) i <<= 1;
    return (i>>1);
}
unsigned Round_Up_Pow2(unsigned n)
{
    unsigned int i = 1;
    if (n == 0)
        return 0;
    while (n > i) i <<= 1;
    return i;
}

G4_opcode Get_Pseudo_Opcode(ISA_Opcode op)
{
    switch( op ){
        case ISA_AND:
            return G4_pseudo_and;
        case ISA_OR:
            return G4_pseudo_or;
        case ISA_XOR:
            return G4_pseudo_xor;
        case ISA_NOT:
            return G4_pseudo_not;
        default:
            return G4_illegal;
    }
    return G4_illegal;
}

VISA_EMask_Ctrl Get_Next_EMask(VISA_EMask_Ctrl currEMask, int execSize)
{
    switch (execSize) {
    default: // Next eMask is only valid for SIMD4, SIMD8, and SIMD16.
        break;
    case 16:
        switch (currEMask) {
        case vISA_EMASK_M1:     return vISA_EMASK_M5;
        case vISA_EMASK_M1_NM:  return vISA_EMASK_M5_NM;
        default: break;
        }
        break;
    case 8:
        switch (currEMask) {
        case vISA_EMASK_M1:     return vISA_EMASK_M3;
        case vISA_EMASK_M1_NM:  return vISA_EMASK_M3_NM;
        case vISA_EMASK_M3:     return vISA_EMASK_M5;
        case vISA_EMASK_M3_NM:  return vISA_EMASK_M5_NM;
        case vISA_EMASK_M5:     return vISA_EMASK_M7;
        case vISA_EMASK_M5_NM:  return vISA_EMASK_M7_NM;
        default: break;
        }
        break;
    case 4:
        switch (currEMask) {
        case vISA_EMASK_M1:     return vISA_EMASK_M2;
        case vISA_EMASK_M1_NM:  return vISA_EMASK_M2_NM;
        case vISA_EMASK_M2:     return vISA_EMASK_M3;
        case vISA_EMASK_M2_NM:  return vISA_EMASK_M3_NM;
        case vISA_EMASK_M3:     return vISA_EMASK_M4;
        case vISA_EMASK_M3_NM:  return vISA_EMASK_M4_NM;
        case vISA_EMASK_M4:     return vISA_EMASK_M5;
        case vISA_EMASK_M4_NM:  return vISA_EMASK_M5_NM;
        case vISA_EMASK_M5:     return vISA_EMASK_M6;
        case vISA_EMASK_M5_NM:  return vISA_EMASK_M6_NM;
        case vISA_EMASK_M6:     return vISA_EMASK_M7;
        case vISA_EMASK_M6_NM:  return vISA_EMASK_M7_NM;
        case vISA_EMASK_M7:     return vISA_EMASK_M8;
        case vISA_EMASK_M7_NM:  return vISA_EMASK_M8_NM;
        default: break;
        }
        break;
    }

    return vISA_NUM_EMASK;
}

unsigned int Get_Gen4_Emask( VISA_EMask_Ctrl cisa_emask, int exec_size )
{

    switch( exec_size )
    {
    case 32:
        switch (cisa_emask)
        {
        case vISA_EMASK_M1:
            return InstOpt_NoOpt;
        case vISA_EMASK_M5:
            return InstOpt_M16;
        case vISA_EMASK_M1_NM:
            return InstOpt_WriteEnable;
        case vISA_EMASK_M5_NM:
            return InstOpt_M16 | InstOpt_WriteEnable;
        default:
            ASSERT_USER(false, "Invalid emask for SIMD32 inst");
            return InstOpt_NoOpt;
        }
        break;
    case 16:
        {
            switch( cisa_emask )
            {
                case vISA_EMASK_M1:
                    return InstOpt_M0;
                case vISA_EMASK_M5:
                    return InstOpt_M16;
                case vISA_EMASK_M1_NM:
                    return InstOpt_M0 | InstOpt_WriteEnable;
                case vISA_EMASK_M5_NM:
                    return InstOpt_M16 | InstOpt_WriteEnable;
                default:
                    MUST_BE_TRUE( false, "Invalid emask for SIMD16 inst" );
                    return InstOpt_NoOpt;
            }
        }
        break;
    case 8:
        {
            switch( cisa_emask )
            {
                case vISA_EMASK_M1:
                    return InstOpt_M0;
                case vISA_EMASK_M3:
                    return InstOpt_M8;
                case vISA_EMASK_M5:
                    return InstOpt_M16;
                case vISA_EMASK_M7:
                    return InstOpt_M24;
                case vISA_EMASK_M1_NM:
                    return InstOpt_M0 | InstOpt_WriteEnable;
                case vISA_EMASK_M3_NM:
                    return InstOpt_M8 | InstOpt_WriteEnable;
                case vISA_EMASK_M5_NM:
                    return InstOpt_M16 | InstOpt_WriteEnable;
                case vISA_EMASK_M7_NM:
                    return InstOpt_M24 | InstOpt_WriteEnable;
                default:
                    MUST_BE_TRUE( false, "Invalid emask for SIMD8 inst" );
                    return InstOpt_NoOpt;
            }
        }
    default:
        // size 4, 2, 1
        {
            switch (cisa_emask)
            {
                case vISA_EMASK_M1:
                    return InstOpt_M0;
                case vISA_EMASK_M2:
                    return InstOpt_M4;
                case vISA_EMASK_M3:
                    return InstOpt_M8;
                case vISA_EMASK_M4:
                    return InstOpt_M12;
                case vISA_EMASK_M5:
                    return InstOpt_M16;
                case vISA_EMASK_M6:
                    return InstOpt_M20;
                case vISA_EMASK_M7:
                    return InstOpt_M24;
                case vISA_EMASK_M8:
                    return InstOpt_M28;
                case vISA_EMASK_M1_NM:
                    return InstOpt_M0 | InstOpt_WriteEnable;
                case vISA_EMASK_M2_NM:
                    return InstOpt_M4 | InstOpt_WriteEnable;
                case vISA_EMASK_M3_NM:
                    return InstOpt_M8 | InstOpt_WriteEnable;
                case vISA_EMASK_M4_NM:
                    return InstOpt_M12 | InstOpt_WriteEnable;
                case vISA_EMASK_M5_NM:
                    return InstOpt_M16 | InstOpt_WriteEnable;
                case vISA_EMASK_M6_NM:
                    return InstOpt_M20 | InstOpt_WriteEnable;
                case vISA_EMASK_M7_NM:
                    return InstOpt_M24 | InstOpt_WriteEnable;
                case vISA_EMASK_M8_NM:
                    return InstOpt_M28 | InstOpt_WriteEnable;
                default:
                    MUST_BE_TRUE( false, "Invalid emask for SIMD4 inst." );
                    return InstOpt_NoOpt;
            }
        }
    }
}

Common_ISA_Operand_Class CISA_Opnd_Class( vector_opnd opnd )
{
    return (Common_ISA_Operand_Class)(opnd.tag & 0x7);
}

unsigned Get_Atomic_Op(VISAAtomicOps op) {

    switch (op) {
    default:
        ASSERT_USER(false, "CISA error: Invalid vISA atomic op for DWord atomic write.");
        break;
    case ATOMIC_ADD:        return GEN_ATOMIC_ADD;
    case ATOMIC_SUB:        return GEN_ATOMIC_SUB;
    case ATOMIC_INC:        return GEN_ATOMIC_INC;
    case ATOMIC_DEC:        return GEN_ATOMIC_DEC;
    case ATOMIC_MIN:        return GEN_ATOMIC_UMIN;
    case ATOMIC_MAX:        return GEN_ATOMIC_UMAX;
    case ATOMIC_XCHG:       return GEN_ATOMIC_MOV;
    case ATOMIC_CMPXCHG:    return GEN_ATOMIC_CMPWR;
    case ATOMIC_AND:        return GEN_ATOMIC_AND;
    case ATOMIC_OR:         return GEN_ATOMIC_OR;
    case ATOMIC_XOR:        return GEN_ATOMIC_XOR;
    case ATOMIC_IMIN:       return GEN_ATOMIC_IMIN;
    case ATOMIC_IMAX:       return GEN_ATOMIC_IMAX;
    case ATOMIC_PREDEC:     return GEN_ATOMIC_PREDEC;
    case ATOMIC_FMIN:       return GEN_ATOMIC_FMIN;
    case ATOMIC_FMAX:       return GEN_ATOMIC_FMAX;
    case ATOMIC_FCMPWR:     return GEN_ATOMIC_FCMPWR;
    }
    return ~0U;
}

uint16_t Get_VISA_Type_Size(VISA_Type type)
{
    switch(type)
    {
    case ISA_TYPE_UD:
    case ISA_TYPE_D:
    case ISA_TYPE_F:
    case ISA_TYPE_V:
    case ISA_TYPE_VF:
    case ISA_TYPE_UV:
        return 4;
    case ISA_TYPE_UW:
    case ISA_TYPE_W:
    case ISA_TYPE_HF:
        return 2;
    case ISA_TYPE_UB:
    case ISA_TYPE_B:
    case ISA_TYPE_BOOL:
        return 1;
    case ISA_TYPE_DF:
    case ISA_TYPE_Q:
    case ISA_TYPE_UQ:
        return 8;
    default:
        MUST_BE_TRUE( 0, "Invalid data type: size unknown." );
        return 0;


    }
}

int Get_Size_Attribute_Info(attribute_info_t * attr)
{
    return sizeof(attr->nameIndex) + sizeof(attr->size) + attr->size;
}

int Get_Size_Label_Info(label_info_t * lbl)
{
    int size = sizeof(lbl->name_index) + sizeof(lbl->kind) + sizeof(lbl->attribute_count);

    for(int i = 0; i< lbl->attribute_count; i++)
    {
        size += Get_Size_Attribute_Info(&lbl->attributes[i]);
    }
    return size;
}

int Get_Size_Var_Info_CISA3(var_info_t * t)
{
    /*
        var_info {
    ud name_index;
    ub bit_properties;
    uw num_elements;
    ud alias_index;
    uw alias_offset;
    ub attribute_count;
    attribute_info[attribute_count];
    }
    */
    int size = sizeof(t->name_index) + sizeof(t->bit_properties) + sizeof(t->num_elements) +
        sizeof(t->alias_index) + sizeof(t->alias_offset) + sizeof(t->alias_scope_specifier) +sizeof(t->attribute_count);

    for(int i = 0; i< t->attribute_count; i++)
    {
        size += Get_Size_Attribute_Info(&t->attributes[i]);
    }
    return size;
}

int Get_Size_Addr_Info(addr_info_t * addr)
{
    /*
    address_info {
    ud name_index;
    uw num_elements;
    ub attribute_count;
    attribute_info[attribute_count];
    }
    */
    int size = sizeof(addr->name_index) + sizeof(addr->num_elements) + sizeof(addr->attribute_count);
    for(int i = 0; i< addr->attribute_count; i++)
    {
        size += Get_Size_Attribute_Info(&addr->attributes[i]);
    }
    return size;
}

int Get_Size_Pred_Info(pred_info_t * pred)
{
    /*
    predicate_info {
    ud name_index;
    uw num_elements;
    ub attribute_count;
    attribute_info[attribute_count];
    }
    */
    int size = sizeof(pred->name_index) + sizeof(pred->num_elements) + sizeof(pred->attribute_count);
    for(int i = 0; i< pred->attribute_count; i++)
    {
        size += Get_Size_Attribute_Info(&pred->attributes[i]);
    }
    return size;
}

int Get_Size_Input_Info(input_info_t * input)
{
    /*
    input_info {
    b kind;
    ud id;
    w offset;
    uw size;
    }
    */
    return sizeof(input->kind) + sizeof(input->index) + sizeof(input->offset) + sizeof(input->size);
}

int Get_Size_Vector_Operand(vector_opnd * cisa_opnd)
{
    int size = 0;

    switch(cisa_opnd->tag & 0x7)
    {
    case OPERAND_GENERAL:
        {
            size = sizeof(cisa_opnd->opnd_val.gen_opnd.index) + sizeof(cisa_opnd->opnd_val.gen_opnd.col_offset) +
                sizeof(cisa_opnd->opnd_val.gen_opnd.row_offset) + sizeof(cisa_opnd->opnd_val.gen_opnd.region);
            break;
        }
    case OPERAND_ADDRESS:
        {
            size = sizeof(cisa_opnd->opnd_val.addr_opnd.index) + sizeof(cisa_opnd->opnd_val.addr_opnd.offset) +
                sizeof(cisa_opnd->opnd_val.addr_opnd.width);
            break;
        }
    case OPERAND_INDIRECT:
        {
            size = sizeof(cisa_opnd->opnd_val.indirect_opnd.index) + sizeof(cisa_opnd->opnd_val.indirect_opnd.addr_offset) +
                sizeof(cisa_opnd->opnd_val.indirect_opnd.indirect_offset) + sizeof(cisa_opnd->opnd_val.indirect_opnd.bit_property) +
                sizeof(cisa_opnd->opnd_val.indirect_opnd.region);
            break;
        }
    case OPERAND_PREDICATE:
        {
            size = sizeof(cisa_opnd->opnd_val.pred_opnd.index);
            break;
        }
    case OPERAND_IMMEDIATE:
        {
            switch (cisa_opnd->opnd_val.const_opnd.type) {
            default:
                size = sizeof(unsigned int);
                break;
            case ISA_TYPE_Q:
            case ISA_TYPE_UQ:
            case ISA_TYPE_DF:
                size = sizeof(unsigned long long);
                break;
            }

            size += sizeof(cisa_opnd->opnd_val.const_opnd.type);
            break;
        }
    case OPERAND_STATE:
        {
            size = sizeof(cisa_opnd->opnd_val.state_opnd.index) + sizeof(cisa_opnd->opnd_val.state_opnd.offset) +
                sizeof(cisa_opnd->opnd_val.state_opnd.opnd_class);
            break;
        }
    default:
        {
            MUST_BE_TRUE( 0, "Invalid Vector Operand Class. Size cannot be determined." );
            break;
        }
    }

    size += sizeof(cisa_opnd->tag);

    return size;
}


/*
cisa3.0
function_info {
    ub linkage; // MBZ
    ub name_len;
    ub name[name_len];
    ud offset;
    ud size;
    uw num_syms_variable; // MBZ
    uw num_syms_function; // MBZ
}
*/
//for cisa 3.0
unsigned long getSizeFunctionInfo(kernel_info_t * kernel_info)
{
    unsigned int size = sizeof(kernel_info->linkage) + sizeof(kernel_info->name_len) +
        kernel_info->name_len + sizeof(kernel_info->offset) + sizeof(kernel_info->size);

    size += sizeof(kernel_info->variable_reloc_symtab.num_syms);
    size += sizeof(kernel_info->function_reloc_symtab.num_syms);

    return size;
}
/*
    kernel_info {
    ub name_len;
    ub name[name_len];
    ud offset;
    ud size;
    ud input_offset;
    uw num_syms_variable; // MBZ
    uw num_syms_function; // MBZ
    ub num_gen_binaries;
    gen_binary_info gen_binaries[num_gen_binaries];
}
*/
unsigned long get_Size_Kernel_Info(kernel_info_t * kernel_info, int major_version, int minor_version)
{
    unsigned long size = sizeof(kernel_info->name_len) + kernel_info->name_len
        + sizeof(kernel_info->offset) + sizeof(kernel_info->size)
        + sizeof(kernel_info->input_offset);

    size += sizeof(kernel_info->variable_reloc_symtab.num_syms);
    size += sizeof(kernel_info->function_reloc_symtab.num_syms);

    size += sizeof(kernel_info->num_gen_binaries);

    for (int i = 0; i < kernel_info->num_gen_binaries; i++)
    {
        size += sizeof(kernel_info->gen_binaries->platform);
        size += sizeof(kernel_info->gen_binaries->binary_offset);
        size += sizeof(kernel_info->gen_binaries->binary_size);
    }

    return size;
}

unsigned long get_Size_Isa_Header( common_isa_header * m_header, int major_version, int minor_version )
{
    unsigned long size = sizeof(m_header->magic_number) + sizeof(m_header->major_version)
        + sizeof(m_header->minor_version) + sizeof(m_header->num_kernels);

    for(int i = 0; i < m_header->num_kernels; i++)
    {
        size += get_Size_Kernel_Info(&m_header->kernels[i], major_version, minor_version);
    }
    /*
    common_isa_header {
    ud magic_number;
    ub major_version;
    ub minor_version;
    uw num_kernels;
    kernel_info kernels[num_kernels];
    uw num_variables;
    file_scope_var_info variables[num_variables];
    uw num_functions;
    function_info functions[num_functions];
    }

    */

    // file-scope variables are no longer supported
    size += sizeof(uint16_t);

    size += sizeof(m_header->num_functions);

    for (int i = 0; i < m_header->num_functions; i++)
    {
        size += getSizeFunctionInfo(&m_header->functions[i]);
    }

    return size;
}

VISA_Cond_Mod Get_Common_ISA_CondModifier_From_G4_CondModifier(G4_CondModifier  cmod )
{
    switch(cmod){
        //case ISA_CMP_NONE:
        //    return Mod_z;
        case Mod_e:
            return ISA_CMP_E;
        case Mod_ne:
            return ISA_CMP_NE;
        case Mod_g:
            return ISA_CMP_G;
        case Mod_ge:
            return ISA_CMP_GE;
        case Mod_l:
            return ISA_CMP_L;
        case Mod_le:
            return ISA_CMP_LE;
        //case ISA_CMP_R:
        //    return Mod_r;
        //case ISA_CMP_O:
        //    return Mod_o;
        //case ISA_CMP_U:
        //    return Mod_u;
        case Mod_cond_undef:
            return ISA_CMP_UNDEF;
        default:
            MUST_BE_TRUE( 0, "Invalid G4 Conditional Modifier." );
            return ISA_CMP_UNDEF;
    }
}

VISA_Exec_Size Get_VISA_Exec_Size_From_Raw_Size( unsigned int size )
{
    switch(size){
        case 1:
            return EXEC_SIZE_1;
        case 2:
            return EXEC_SIZE_2;
        case 4:
            return EXEC_SIZE_4;
        case 8:
            return EXEC_SIZE_8;
        case 16:
            return EXEC_SIZE_16;
        case 32:
            return EXEC_SIZE_32;
        default:
            MUST_BE_TRUE( false, "illegal common ISA execsize (should be 1, 2, 4, 8, 16, 32)." );
            return EXEC_SIZE_ILLEGAL;
    }
}
int Get_Size_State_Info(state_info_t * t)
{
    int size = sizeof(t->name_index) + sizeof(t->num_elements) + sizeof(t->attribute_count);

    for(int i = 0; i< t->attribute_count; i++)
    {
        size += Get_Size_Attribute_Info(&t->attributes[i]);
    }
    return size;
}

VISA_Oword_Num Get_VISA_Oword_Num_From_Number( unsigned num )
{
    switch(num){
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
            MUST_BE_TRUE( false, "illegal Oword number." );
            return OWORD_NUM_ILLEGAL;
    }
}

VISA_Modifier Get_Common_ISA_SrcMod_From_G4_Mod(G4_SrcModifier mod )
{
    switch( mod )
    {
    case Mod_src_undef:
        return MODIFIER_NONE ;
    case Mod_Abs:
        return MODIFIER_ABS ;
    case Mod_Minus:
        return MODIFIER_NEG ;
    case Mod_Minus_Abs:
        return MODIFIER_NEG_ABS ;
    case Mod_Not:
        return MODIFIER_NOT;
    default:
        MUST_BE_TRUE(0, "Wrong src modifier");
        return MODIFIER_NONE;
    }
}


VISA_Type getVectorOperandType(const common_isa_header& isaHeader, const print_format_provider_t* header, const vector_opnd& opnd)
{
    unsigned numPreDefinedVars = Get_CISA_PreDefined_Var_Count();
    switch (opnd.getOperandClass())
    {
        case OPERAND_GENERAL:
            if (opnd.opnd_val.gen_opnd.index < numPreDefinedVars)
            {
                // One of the pre-defined variables
                return getPredefinedVarType(mapExternalToInternalPreDefVar(opnd.getOperandIndex()));
            }
            else
            {
                const var_info_t& var = *header->getVar(opnd.getOperandIndex()-numPreDefinedVars);
                return var.getType();
            }
        case OPERAND_ADDRESS:
            return ISA_TYPE_UW;
        case OPERAND_PREDICATE:
            return ISA_TYPE_BOOL;
        case OPERAND_INDIRECT:
            return (VISA_Type)(opnd.opnd_val.indirect_opnd.bit_property & 0xF);
        case OPERAND_ADDRESSOF:
            return ISA_TYPE_UW;
        case OPERAND_IMMEDIATE:
            return (VISA_Type)(opnd.opnd_val.const_opnd.type & 0xF);
        case OPERAND_STATE:
            return ISA_TYPE_UD;
        default:
            return ISA_TYPE_UD;
    }
}

const raw_opnd& getRawOperand(const CISA_INST* inst, unsigned i)
{
    MUST_BE_TRUE(inst, "Argument Exception: argument inst is NULL.");
    MUST_BE_TRUE(inst->opnd_count > i, "No such operand, i, for instruction inst.");
    return inst->opnd_array[i]->_opnd.r_opnd;
}

const vector_opnd& getVectorOperand(const CISA_INST* inst, unsigned i)
{
    MUST_BE_TRUE(inst, "Argument Exception: argument inst is NULL.");
    MUST_BE_TRUE(inst->opnd_count > i, "No such operand, i, for instruction inst.");
    return inst->opnd_array[i]->_opnd.v_opnd;
}

CISA_opnd_type getOperandType(const CISA_INST* inst, unsigned i)
{
    MUST_BE_TRUE(inst, "Argument Exception: argument inst is NULL.");
    MUST_BE_TRUE(inst->opnd_count > i, "No such operand, i, for instruction inst.");
    return inst->opnd_array[i]->opnd_type;
}

int64_t typecastVals(const void* value, VISA_Type isaType)
{
    int64_t retVal = 0;
    switch (isaType)
    {
    case ISA_TYPE_UD:
    case ISA_TYPE_UV:
    case ISA_TYPE_VF:
    {
        retVal = (int64_t)(*((unsigned int*)value));
        break;
    }
    case ISA_TYPE_D:
    case ISA_TYPE_V:
    {
        retVal = (int64_t)(*((int*)value));
        break;
    }
    case ISA_TYPE_UW:
    {
        retVal = (int64_t)(*((uint16_t*)value));
        break;
    }
    case ISA_TYPE_W:
    {
        retVal = (int64_t)(*((int16_t*)value));
        break;
    }
    case ISA_TYPE_UB:
    {
        retVal = (int64_t)(*((uint8_t*)value));
        break;
    }
    case ISA_TYPE_B:
    {
        retVal = (int64_t)(*((int8_t*)value));
        break;
    }
    case ISA_TYPE_HF:
    {
        // clear higher bits
        retVal = (int64_t)(*((uint16_t*)value));
        break;
    }

    default:
    {
        assert(0);
        return -1;
    }
    }
    return retVal;
}

// convert binary vISA surface id to GEN surface index
int Get_PreDefined_Surf_Index(int index)
{
    if (getGenxPlatform() < GENX_SKL)
    {
        switch (index)
        {
        case 1:
            return PREDEF_SURF_1_OLD;
        case 2:
            return PREDEF_SURF_2_OLD;
        case 3:
            return PREDEF_SURF_3_OLD;
        default:
            ;
            // fallthrough
        }
    }

    return vISAPreDefSurf[index].genId;

}

const char* createStringCopy(const char* name, vISA::Mem_Manager &m_mem)
{
    if (strlen(name) == 0)
    {
        return "";
    }
    size_t size = strlen(name) + 1;
    if (size > 255)
    {
        size = 255;
    }
    char* str = (char*) m_mem.alloc(size);
    strncpy_s(str, size, name, size);
    return str;
}

std::string sanitizeLabelString(std::string str)
{
    auto isReservedChar = [](char c)
    {
        return !isalnum(c) && c != '_' && c != '$';
    };
    std::replace_if(str.begin(), str.end(), isReservedChar, '_');
    return str;
}

// This function scrubs out illegal file path characters
//
// NOTE: we must permit directory separators though since the string is a path
std::string sanitizePathString(std::string str)
{
#ifdef _WIN32
    // better cross platform behavior ./foo/bar.asm => to backslashes
    auto isFwdSlash = [](char c) {return c == '/';};
    std::replace_if(str.begin(), str.end(), isFwdSlash, '\\');
#endif

    auto isReservedChar = [](char c)
    {
#ifdef _WIN32
        // c.f. https://docs.microsoft.com/en-us/windows/desktop/fileio/naming-a-file
        switch (c)
        {
        // we need these because we have a full path
        // case '\\': path separator
        case ':': // can be a drive suffix, but we handle this manually
        case '"':
        case '*':
        case '|':
        case '?':
        case '<':
        case '>':
            return true;
        default: return !isprint(c) && !isspace(c);
        }
        return false;
#else
        return c == ':' || (!isprint(c) && !isspace(c));
#endif
    };

#ifdef _WIN32
    if (str.length() > 2 && isalnum(str[0]) && str[1] == ':') {
        // drive prefix: D:... or D:
        std::replace_if(str.begin()+2, str.end(), isReservedChar, '_');
    } else {
        std::replace_if(str.begin(), str.end(), isReservedChar, '_');
    }
#else
    std::replace_if(str.begin(), str.end(), isReservedChar, '_');
#endif
    return str;
}


