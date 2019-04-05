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

#ifndef COMMON_ISA_UTIL_INCLUDED
#define COMMON_ISA_UTIL_INCLUDED

/*  Utility functions for common ISA binary emission
 *
 */
#include "visa_igc_common_header.h"
#include "Common_ISA.h"
#include "Gen4_IR.hpp"

extern const char* Common_ISA_Get_Align_Name( VISA_Align );
extern const char* Common_ISA_Get_Modifier_Name( VISA_Modifier );
extern short Common_ISA_Get_Region_Name( Common_ISA_Region_Val );
extern G4_opcode Get_G4_Opcode_From_Common_ISA_Opcode( ISA_Opcode );
extern VISA_Type Get_Common_ISA_Type_From_G4_Type( G4_Type );
extern G4_Type Get_G4_Type_From_Common_ISA_Type( VISA_Type );
extern G4_SubReg_Align Get_G4_SubRegAlign_From_Type( G4_Type ty );
extern G4_SubReg_Align Get_G4_SubRegAlign_From_Size( uint16_t size );
extern int Get_G4_ExecSize_From_Common_ISA_ExecSize( Common_ISA_Exec_Size );
extern G4_SrcModifier Get_G4_SrcMod_From_Common_ISA_Mod( VISA_Modifier );
extern G4_CondModifier Get_G4_CondModifier_From_Common_ISA_CondModifier( Common_ISA_Cond_Mod );
extern bool hasPredicate(ISA_Opcode op);
extern bool hasExecSize(ISA_Opcode op, uint8_t subOp = 0);
extern bool hasLabelSrc(ISA_Opcode op);
extern unsigned Get_Common_ISA_SVM_Block_Num (Common_ISA_SVM_Block_Num );
extern Common_ISA_SVM_Block_Num valueToVISASVMBlockNum(unsigned int);
extern unsigned Get_Common_ISA_SVM_Block_Size(Common_ISA_SVM_Block_Type);
extern Common_ISA_SVM_Block_Type valueToVISASVMBlockType(unsigned int);
extern unsigned Get_Common_ISA_Oword_Num( Common_ISA_Oword_Num );
extern unsigned Get_Common_ISA_Exec_Size( Common_ISA_Exec_Size );
extern bool IsMathInst(ISA_Opcode op);
extern bool IsIntType(VISA_Type);
extern bool IsIntOrIntVecType(VISA_Type);
extern bool IsSingedIntType(VISA_Type);
extern bool IsUnsignedIntType(VISA_Type);
extern unsigned short Get_Common_ISA_Region_Value(Common_ISA_Region_Val);
extern unsigned short Create_CISA_Region(unsigned short vstride, unsigned short width, unsigned short hstride);
extern unsigned Round_Up_Pow2(unsigned n);
extern unsigned Round_Down_Pow2(unsigned n);
extern G4_Predicate_Control Get_Pred_Ctrl( unsigned short predicate, int size);
extern G4_Predicate_Control vISAPredicateToG4Predicate( VISA_PREDICATE_CONTROL control, int size);
extern G4_opcode Get_Pseudo_Opcode(ISA_Opcode op);
extern Common_VISA_EMask_Ctrl Get_Next_EMask(Common_VISA_EMask_Ctrl currEMask, int execSize);
extern unsigned int Get_Gen4_Emask( Common_VISA_EMask_Ctrl cisa_emask, int exec_size );
extern Common_ISA_Operand_Class CISA_Opnd_Class( vector_opnd opnd );
extern unsigned Get_Atomic_Op(VISAAtomicOps op);
extern uint16_t Get_Common_ISA_Type_Size(VISA_Type type);

extern int Get_Size_Attribute_Info(attribute_info_t * attr);
extern int Get_Size_Label_Info(label_info_t * lbl);
extern int Get_Size_Var_Info_CISA3(var_info_t * t);
extern int Get_Size_State_Info(state_info_t * t);
extern int Get_Size_Addr_Info(addr_info_t * addr);
extern int Get_Size_Pred_Info(pred_info_t * pred);
extern int Get_Size_Input_Info(input_info_t * input);
extern int Get_Size_Vector_Operand(vector_opnd * cisa_opnd);
extern unsigned long get_Size_Isa_Header( common_isa_header * m_header, int major_version, int minor_version );
extern Common_ISA_Region_Val Get_CISA_Region_Val( short val );
extern short Common_ISA_Get_Region_Value( Common_ISA_Region_Val val );
extern unsigned long get_Size_Kernel_Info(kernel_info_t * kernel_info, int major_version, int minor_version );
extern unsigned long getSizeFunctionInfo(kernel_info_t * kernel_info);
extern Common_ISA_Cond_Mod Get_Common_ISA_CondModifier_From_G4_CondModifier(G4_CondModifier  cmod );
extern Common_ISA_Exec_Size Get_Common_ISA_Exec_Size_From_Raw_Size( unsigned int size );
extern Common_ISA_Oword_Num Get_Common_ISA_Oword_Num_From_Number( unsigned num );
extern VISA_Modifier Get_Common_ISA_SrcMod_From_G4_Mod(G4_SrcModifier mod );

inline uint32_t getVersionAsInt(uint32_t major, uint32_t minor)
{
    return major * 100 + minor;
}

inline unsigned int Get_CISA_PreDefined_Var_Count()
{
    return COMMON_ISA_NUM_PREDEFINED_VAR_VER_3;
}

extern const char* createStringCopy(const char* name, vISA::Mem_Manager &m_mem);

extern std::string sanitizeString(std::string& str);

inline unsigned int Get_CISA_PreDefined_Surf_Count()
{
    return COMMON_ISA_NUM_PREDEFINED_SURF_VER_3_1;

}

inline unsigned
getSendRspLengthBitOffset ()
{
    return SEND_GT_RSP_LENGTH_BIT_OFFSET;
}

// Send message information query

inline unsigned
getSendMsgLengthBitOffset ()
{
    return SEND_GT_MSG_LENGTH_BIT_OFFSET;
}

inline unsigned
getSendHeaderPresentBitOffset ()
{
    return SEND_GT_MSG_HEADER_PRESENT_BIT_OFFSET;
}

VISA_Type getVectorOperandType(const common_isa_header& isaHeader, const print_format_provider_t* header, const vector_opnd& opnd);

template <typename T> T getPrimitiveOperand(const CISA_INST* inst, unsigned i)
{
    MUST_BE_TRUE(inst, "Argument Exception: argument inst is NULL.");
    MUST_BE_TRUE(inst->opnd_count > i, "No such operand, i, for instruction inst.");
    return (T)inst->opnd_array[i]->_opnd.other_opnd;
}

const raw_opnd& getRawOperand(const CISA_INST* inst, unsigned i);

const vector_opnd& getVectorOperand(const CISA_INST* inst, unsigned i);

CISA_opnd_type getOperandType(const CISA_INST* inst, unsigned i);

int64_t typecastVals(const void *value, VISA_Type isaType);

int Get_PreDefined_Surf_Index( int index );

inline bool isShiftOp(ISA_Opcode op)
{
    return op == ISA_SHL || op == ISA_SHR || op == ISA_ASR || op == ISA_ROL || op == ISA_ROR;
}


#endif  /* COMMON_ISA_UTIL_INCLUDED */

