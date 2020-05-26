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

%{
#include <stdio.h>
#include <math.h>
#include <string>
#include <vector>

#include "visa_igc_common_header.h"
#include "Common_ISA.h"
#include "Common_ISA_util.h"
#include "Common_ISA_framework.h"
#include "JitterDataStruct.h"
#include "VISAKernel.h"


//VISA_Type variable_declaration_and_type_check(char *var, Common_ISA_Var_Class type);
void CISAerror(CISA_IR_Builder* builder, char const* msg);
int  yylex();
extern int CISAlineno;

static bool ParseAlign(CISA_IR_Builder* pCisaBuilder, const char *sym, VISA_Align &value);
static bool ParseEMask(CISA_IR_Builder* pCisaBuilder, const char* sym, VISA_EMask_Ctrl &emask);

/*
 * check if the cond is true.
 * if cond is false, then print errorMessage (syntax error) and YYABORT
 */
#define MUST_HOLD(cond, errorMessage) \
  {if (!(cond)) {pCisaBuilder->RecordParseError(CISAlineno, errorMessage); YYABORT;}}
#define PARSE_ERROR(...)\
  {pCisaBuilder->RecordParseError(CISAlineno, __VA_ARGS__); YYABORT;}
#ifdef _DEBUG
#define TRACE(str) fprintf(CISAout, str)
#else
#define TRACE(str)
#endif

char * switch_label_array[32];
std::vector<VISA_opnd*> RTWriteOperands;
VISA_opnd *opndRTWriteArray[32];
int num_parameters;

VISA_RawOpnd* rawOperandArray[16];

#ifndef PRId64
# ifdef _WIN32
#   define PRId64 "I64d"
# else
#   if __WORDSIZE == 64
#     define PRId64 "ld"
#   else
#     define PRId64 "lld"
#   endif
# endif
#endif

#ifdef _MSC_VER
#pragma warning(disable:4065; disable:4267)
#endif

%}

%parse-param {CISA_IR_Builder* pCisaBuilder}

%error-verbose

%union
{
    int64_t                number;
    double                 fp;

    struct strlitbuf_struct {
        char      decoded[4096];
        size_t    len;
    } strlit;
    char *                 string;
    char *                 asm_name;
    char *                 var_name;

    VISA_Type              type;
    ISA_Opcode             opcode;
    VISA_Cond_Mod    mod;
    //G4_Type              g4_type;
    //G4_Operand*          opnd;

    bool                   sat;
    short                  shortnum;
    unsigned short         ushortnum;

    struct {
        //G4_CondModifier        mod;
        VISA_Cond_Mod cisa_mod;
    } cond_mod;
    struct {
        VISA_opnd * cisa_gen_opnd;
    } cisa_pred;

    struct {
        //G4_PredState           state;
        VISA_PREDICATE_STATE cisa_state;
    } pred_state;

    struct {
        //G4_SrcModifier         srcMod;
        VISA_Modifier mod;
    } src_mod;

    struct {
        unsigned int v_stride;
        unsigned int h_stride;
        unsigned int width;
        //RegionDesc*              rgn;
    } cisa_region;

    struct  {
        int                row;
        int                elem;
    } offset;

    struct {
        //G4_Operand*        opnd;
        Common_ISA_Operand_Class type;
       // VISA_opnd * cisa_gen_opnd;
    } dstOpnd;

    struct {
        //G4_Operand*        opnd;
        Common_ISA_Operand_Class type;
       // VISA_opnd * cisa_gen_opnd;
    } srcOpnd;

    struct {
        //G4_Operand*        opnd;
        Common_ISA_Operand_Class type;
        VISA_opnd * cisa_gen_opnd;
    } genOperand;

    struct {
        //G4_Operand*        opnd;
        Common_ISA_Operand_Class type;
        Common_ISA_Function_Parameters_Kind kind;
        char * var_name; //for Surface, VME, Sampler
        //for raw operand
        //G4_Declare *dcl;
        unsigned short offset;
    } srcFuncOpnd;

    struct {
        char * var_name;
        //G4_Operand*        opnd;
        VISA_opnd * cisa_gen_opnd;
        unsigned char streamMode;
        unsigned char searchCtrl;
    } vmeOpndIvb;

    struct {
        //G4_Operand* fbrMbMode;
        //G4_Operand* fbrSubMbShape;
        //G4_Operand* fbrSubPredMode;

        VISA_opnd * cisa_fbrMbMode_opnd;
        VISA_opnd * cisa_fbrSubMbShape_opnd;
        VISA_opnd * cisa_fbrSubPredMode_opnd;
    } vmeOpndFbr;

    struct {
        //G4_Operand*        opnd;
        int                row;
        int                elem;
        int                immOff;
        //G4_RegAccess       acc;
        VISA_opnd * cisa_gen_opnd;
        CISA_GEN_VAR * cisa_decl;
    } regAccess;

    struct{
        char *             aliasname;
        int                offset;
    } alias;

    struct {
        //G4_Declare *dcl;
        unsigned short offset;
        VISA_opnd * cisa_gen_opnd;
    } RawVar;

    struct {
        //G4_Declare *dcl;
        Common_ISA_State_Opnd type;
        unsigned char offset;
        VISA_opnd * cisa_gen_opnd;
    } StateVar;

    struct {
        VISA_EMask_Ctrl emask;
        int exec_size;
    } emask_exec_size;

    /*
        MUST MATCH DEFINITION IN Common_ISA_framework.h!!!!!!!!!!
    */
    struct attr_gen_struct {
        char *             name;
        bool               isInt;
        int                value;
        char *             string_val;
        bool               attr_set;
    } attr_gen;


    /* Align Support in Declaration*/
    VISA_Align             CISA_align;
    VISA_Align             align;

    VISAAtomicOps          atomic_op;
    VISASampler3DSubOpCode sample3DOp;

    MEDIA_LD_mod           media_mode;
    bool                   oword_mod;
    VISAChannelMask        s_channel; // Cannot use ChannelMask here as it's a member of union where non-trivial constructor is not allowed.
    CHANNEL_OUTPUT_FORMAT  s_channel_output;
    COMMON_ISA_VME_OP_MODE VME_type;
    VISA_EMask_Ctrl emask;
    OutputFormatControl    cntrl;
    AVSExecMode            execMode;
    bool                   file_end;
    unsigned char          fence_options;

    // Pixel null mask for sampler instructions.
    bool                   pixel_null_mask;

    // CPS LOD compensation enable for 3d sample.
    bool                   cps;
    bool                   non_uniform_sampler;
    bool                   flag;
}

%start CISAStmt

%token DIRECTIVE_KERNEL     /* .kernel */
%token DIRECTIVE_VERSION    /* .verions */
%token DIRECTIVE_ENTRY      /* .entry */
%token DIRECTIVE_DECL       /* .decl */
%token FUNC_DIRECTIVE_DECL  /* .funcdecl */
%token DIRECTIVE_ATTR       /* .attr */
%token DIRECTIVE_KERNEL_ATTR /* .kernel_attr */
%token DIRECTIVE_INPUT      /* .input */
%token DIRECTIVE_PARAMETER  /* .parameter */
%token DIRECTIVE_LOC        /* .loc */
%token DIRECTIVE_FUNC       /* .function */
%token DIRECTIVE_GLOBAL_FUNC /* .global_function */
%token SRCMOD_NEG           /* (-) */
%token SRCMOD_ABS           /* (abs) */
%token SRCMOD_NEGABS        /* (-abs) */
%token SRCMOD_NOT           /* (~) */
%token SAT                  /* .sat */
%token PIXEL_NULL_MASK      /* .pixel_null_mask */
%token CPS                  /* .cps */
%token NON_UNIFORM_SAMPLER  /* .divS */
%token ALIAS
%token ALIGN
%token RAW_SEND_STRING      /* raw_send */
%token RAW_SENDC_STRING     /* raw_sendc */
%token RAW_SENDS_STRING     /* raw_sends */
%token RAW_SENDS_EOT_STRING /* raw_sends_eot */
%token RAW_SENDSC_STRING    /* raw_sendsc */
%token RAW_SENDSC_EOT_STRING /* raw_sendsc_eot */

%token <atomic_op> ATOMIC_SUB_OP
%token <var_name> PHYSICAL_REGISTER
%token <type> ITYPE
%token <type> DECL_DATA_TYPE
%token <type> DFTYPE         /* :df */
%token <type> FTYPE          /* :f */
%token <type> HFTYPE         /* :hf */
%token <type> TYPE           /* :w, :uw, :ud, :d, ... */
%token <type> VTYPE          /* :v and vf */
%token <fp> FLOATINGPOINT    /* fp value */
%token <fp> DOUBLEFLOAT      /* double fp value */
%token <number> NUMBER       /* integer number */
%token <number> HEX_NUMBER   /* 0x123 */
%token <mod> COND_MOD        /* .ne .ge ... */


%token <string> LANGLE          /* < */
%token <string> RANGLE          /* > */
%token <string> LBRACK          /* [ */
%token <string> RBRACK          /* ] */
%token <string> IND_LBRACK      /* r[ */
%token <string> LPAREN           /* ( */
%token <string> RPAREN           /* ) */
%token <string> LBRACE           /* { */
%token <string> RBRACE           /* } */

%token <string> DOT              /* . */
%token <string> COMMA            /* , */
%token <string> SEMI             /* ; */
%token <string> COLON            /* : */
%token <string> SLASH            /* / */

%token <string> EQUALS           /* = */
%token <string> PLUS             /* + */
%token <string> MINUS            /* - */
%token <string> TIMES            /* * */
%token <string> AMP              /* & */
%token <string> TILDE            /* ~ */
%token <string> BANG             /* ! */

%token <string> LABEL
%token <string> IMPLICIT_INPUT
%token <string> OFFSET
%token <string> SLM_SIZE
%token <string> PRED_CNTL     /* .any2h, .any4h, ... */
%token <string> CHAN4_MASK    /* RGBA */
%token <string> VAR           /* variable */
%token <string> NULL_VAR      /* variable (V0) */
%token <string> COMMENT_LINE  /* comment line text */
%token <string> STMT_DELIM    /* statement delimited - \n */
%token <string> INPUT_VAR
%token <string> NUM_ELTS
%token <string> V_NAME_TOKEN
%token <string> SIZE
%token <string> FLAG_REG_NAME
%token <string> SURF_USE_NAME
%token <string> F_CLASS
%token <string> G_CLASS
%token <string> P_CLASS
%token <string> A_CLASS
%token <string> S_CLASS
%token <string> T_CLASS
%token <string> RTWRITE_OPTION

%token <string>            STRING_LITERAL
%token <CISA_align>        ALIGNTYPE_2GRF
%token <media_mode>        MEDIA_MODE
%token <oword_mod>         OWORD_MODIFIER
%token <s_channel>         SAMPLER_CHANNEL
%token <s_channel>         SLM_CHANNEL
%token <s_channel_output>  CHANNEL_OUTPUT
%token <VME_type>          VME_TYPE
%token <file_end>          FILE_EOF
%token <execMode>          EXECMODE
%token <cntrl>             CNTRL
%token <fence_options>     FENCE_OPTIONS;

//Instruction opcode tokens
%token <opcode> THREE_OPERAND_OP
%token <opcode> TWO_OPERAND_OP

%token <opcode> MOD_OP
%token <opcode> ADDR_ADD_OP
%token <opcode> UNARY_LOGIC_OP
%token <opcode> BINARY_LOGIC_OP
%token <opcode> TERNARY_LOGIC_OP
%token <opcode> QUATERNARY_LOGIC_OP

%token <opcode> SEL_OP
%token <opcode> MIN_OP
%token <opcode> MAX_OP
%token <opcode> ANTI_TRIG_OP
%token <opcode> MATH2_OP
%token <opcode> MATH3_OP
%token <opcode> ARITH2_OP
%token <opcode> ARITH3_OP
%token <opcode> ARITH4_OP
%token <opcode> ARITH4_OP2
%token <opcode> CMP_OP
%token <opcode> SVM_OP
%token <opcode> SVM_SCATTER_OP
%token <opcode> SVM_GATHER4SCALED_OP
%token <opcode> SVM_SCATTER4SCALED_OP
%token <opcode> SVM_ATOMIC_OP
%token <opcode> OWORD_OP
%token <opcode> MEDIA_OP
%token <opcode> SCATTER_OP
%token <opcode> SCATTER_TYPED_OP
%token <opcode> SCATTER_SCALED_OP
%token <opcode> SCATTER4_SCALED_OP
%token <opcode> BARRIER_OP
%token <opcode> SBARRIER_SIGNAL
%token <opcode> SBARRIER_WAIT
%token <opcode> ATOMIC_OP
%token <opcode> DWORD_ATOMIC_OP
%token <opcode> TYPED_ATOMIC_OP
%token <opcode> SAMPLE_OP
%token <opcode> SAMPLE_UNORM_OP
%token <opcode> SURFACE_OP
%token <opcode> VME_IME_OP
%token <opcode> VME_SIC_OP
%token <opcode> VME_FBR_OP
%token <opcode> BRANCH_OP
%token <opcode> IFCALL
%token <opcode> FCALL
%token <opcode> FADDR
%token <opcode> SWITCHJMP_OP
%token <opcode> MOVS_OP
%token <opcode> SETP_OP
%token <opcode> MOV_OP
%token <opcode> FILE_OP
%token <opcode> LOC_OP
%token <opcode> CACHE_FLUSH_OP
%token <opcode> WAIT_OP
%token <opcode> FENCE_GLOBAL_OP
%token <opcode> FENCE_LOCAL_OP
%token <opcode> FENCE_SW_OP
%token <opcode> YIELD_OP
%token <sample3DOp> SAMPLE_3D_OP
%token <sample3DOp> LOAD_3D_OP
%token <sample3DOp> SAMPLE4_3D_OP
%token <opcode> RESINFO_OP_3D
%token <opcode> SAMPLEINFO_OP_3D
%token <opcode> RTWRITE_OP_3D
%token <opcode> URBWRITE_OP_3D
%token <opcode> LIFETIME_START_OP
%token <opcode> LIFETIME_END_OP
%token <opcode> AVS_OP

%type <string> TargetLabel
%type <number> SwitchLabels
%type <number> RTWriteOperandParse
%type <number> RawOperandArray
//%type <string> KERNEL_NAME
%type <string> PredCntrl
%type <string> FUNCTION_NAME
%type <string> SymbolName
%type <string> StrLitOrVar

%type <number> PlaneID
%type <number> SIMDMode
%type <number> DstRegion
%type <number> ImmAddrOffset
%type <number> AbstractNum
%type <number> Exp
// %type <number> Exp32
%type <number> ElemNum
%type <number> OFFSET_NUM
%type <number> SIZE_NUM

%type <cond_mod> ConditionalModifier

%type <genOperand> SrcGeneralOperand
%type <genOperand> SrcGeneralOperand_1
%type <genOperand> SrcImmOperand
%type <genOperand> SrcIndirectOperand
%type <genOperand> SrcIndirectOperand_1
%type <genOperand> SrcAddrOperand
%type <genOperand> AddrOfOperand
%type <genOperand> Imm
%type <genOperand> FpImm
%type <genOperand> DFImm
%type <genOperand> HFImm

%type <genOperand> VecSrcOperand_G_I_IMM_A
%type <genOperand> VecSrcOperand_G_I_IMM
%type <genOperand> VecSrcOperand_G_IMM
%type <genOperand> VecSrcOperand_A_G
%type <genOperand> VecSrcOpndSimple
%type <genOperand> VecDstOperand_A
%type <genOperand> VecDstOperand_G
%type <genOperand> VecDstOperand_G_I
%type <genOperand> DstGeneralOperand
%type <genOperand> DstAddrOperand
%type <genOperand> DstIndirectOperand

%type <cisa_region> Region
%type <cisa_region> RegionWH
%type <cisa_region> RegionV
%type <cisa_region> IndirectRegion
%type <cisa_region> SrcRegion
%type <vmeOpndIvb> VMEOpndIME
%type <vmeOpndFbr> VMEOpndFBR
%type <regAccess> IndirectVar
%type <regAccess> AddrParam
%type <regAccess> AddrVar
%type <regAccess> AddressableVar

%type <fp>  FloatPoint
%type <fp>  DoubleFloat

%type <CISA_align>          AlignType
%type <emask_exec_size>     ExecSize
%type <ushortnum>           ExecSizeInt
%type <offset>              TwoDimOffset
%type <type>                DataType
%type <src_mod>             SrcModifier
%type <pred_state>          PredState
%type <sat>                 InstModifier
%type <cisa_pred>           Predicate
%type <alias>               AliasInfo
%type <oword_mod>           OwordModifier
%type <RawVar>              RawOperand
%type <StateVar>            DstStateOperand
%type <StateVar>            SrcStateOperand
%type <pixel_null_mask>     PIXEL_NULL_MASK_ENABLE
%type <cps>                 CPS_ENABLE
%type <non_uniform_sampler> NON_UNIFORM_SAMPLER_ENABLE
%type <attr_gen>            GEN_ATTR
%type <flag>                IS_ATOMIC16
%type <ushortnum>           ATOMIC_BITWIDTH



%type <string> RTWRITE_MODE

%type <string> V_NAME
/* the declaration order implies operator precedence */
%left MINUS PLUS TIMES SLASH
%left NEG

%%

CISAStmt : /* empty */
       | CISAStmt STMT_DELIM
       | CISAStmt EndOfFile
       | CISAStmt CommentLine                             STMT_DELIM
       | CISAStmt ScopeOp                 TrailingComment STMT_DELIM
       | CISAStmt DirectiveKernel         TrailingComment STMT_DELIM
       | CISAStmt DirectiveGlobalFunction TrailingComment STMT_DELIM
       | CISAStmt DirectiveVersion        TrailingComment STMT_DELIM
       | CISAStmt DirectiveDecl           TrailingComment STMT_DELIM
       | CISAStmt DirectiveInput          TrailingComment STMT_DELIM
       | CISAStmt DirectiveImplicitInput  TrailingComment STMT_DELIM
       | CISAStmt DirectiveParameter      TrailingComment STMT_DELIM
       | CISAStmt DirectiveFunc           TrailingComment STMT_DELIM
       | CISAStmt DirectiveAttr           TrailingComment STMT_DELIM
       | CISAStmt CISAInst                TrailingComment STMT_DELIM

/* ----- Handle trailing comments ---- */
TrailingComment : /* empty */
              {
                    //TODO add comments
                  //pBuilder->addTrailingComment("No comment");
              }
             | COMMENT_LINE
              {
                    //TODO add comments
                  //pBuilder->addTrailingComment($1);
              };

EndOfFile : FILE_EOF
             {
                 pCisaBuilder->CISA_post_file_parse();
             }

StrLitOrVar : STRING_LITERAL | VAR

/* --------------------------------------------------------------------- */
/* ------------------------- directives -------------------------------- */
/* --------------------------------------------------------------------- */

/* ----- .kernel ------ */
DirectiveKernel : DIRECTIVE_KERNEL StrLitOrVar
              {
                  VISAKernel *cisa_kernel = NULL;
                  pCisaBuilder->AddKernel(cisa_kernel, $2);
              };


/* ----- .global_function ------ */
DirectiveGlobalFunction : DIRECTIVE_GLOBAL_FUNC StrLitOrVar
              {
                  VISAFunction *cisa_kernel = NULL;
                  pCisaBuilder->AddFunction(cisa_kernel, $2);
              };

 V_NAME :
                          {$$ = "";};
        | V_NAME_TOKEN VAR{$$ = $2;};


/* ----- .version ------ */
DirectiveVersion : DIRECTIVE_VERSION NUMBER DOT NUMBER
   {
       pCisaBuilder->CISA_IR_setVersion((unsigned char)$2, (unsigned char)$4);
   }

/* ----- .decl ----- */
DirectiveDecl : DeclVariable
                | DeclAddress
                | DeclPredicate
                | DeclSampler
                | DeclSurface
                | DeclFunctions

DeclFunctions: FUNC_DIRECTIVE_DECL STRING_LITERAL
    {
        // do nothing as it's informational only
    }

               //     1       2      3          4          5       6       7          8          9
DeclVariable: DIRECTIVE_DECL VAR G_CLASS DECL_DATA_TYPE NUM_ELTS NUMBER AlignType AliasInfo GEN_ATTR
               {
                   attr_gen_struct temp_struct;
                   temp_struct.value = $9.value;
                   temp_struct.name = $9.name;
                   temp_struct.string_val = $9.string_val;
                   temp_struct.isInt = $9.isInt;
                   temp_struct.attr_set = $9.attr_set;
                   if (!pCisaBuilder->CISA_general_variable_decl($2, (unsigned int)$6, $4, $7, $8.aliasname, $8.offset, temp_struct, CISAlineno)) {
                       YYABORT; // error already reported
                   }
               };

               //     1      2     3       4       5      6
DeclAddress: DIRECTIVE_DECL VAR A_CLASS NUM_ELTS NUMBER GEN_ATTR
               {
                   attr_gen_struct temp_struct;
                   temp_struct.value = $6.value;
                   temp_struct.name = $6.name;
                   temp_struct.string_val = $6.string_val;
                   temp_struct.isInt = $6.isInt;
                   temp_struct.attr_set = $6.attr_set;
                   pCisaBuilder->CISA_addr_variable_decl($2, (unsigned int)$5, ISA_TYPE_UW, temp_struct, CISAlineno);
               };

               //     1        2      3      4       5       6
DeclPredicate: DIRECTIVE_DECL VAR P_CLASS NUM_ELTS NUMBER GEN_ATTR
               {
                   attr_gen_struct temp_struct;
                   temp_struct.value = $6.value;
                   temp_struct.name = $6.name;
                   temp_struct.string_val = $6.string_val;
                   temp_struct.isInt = $6.isInt;
                   temp_struct.attr_set = $6.attr_set;
                   if (!pCisaBuilder->CISA_predicate_variable_decl($2, (unsigned int)$5, temp_struct, CISAlineno)) {
                       YYABORT; // error already reported
                   }
               };

               //     1      2     3       4       5       6         7
DeclSampler: DIRECTIVE_DECL VAR S_CLASS NUM_ELTS NUMBER  V_NAME GEN_ATTR
               {
                   if (!pCisaBuilder->CISA_sampler_variable_decl($2, (int)$5, $6, CISAlineno)) {
                       YYABORT; // error already reported
                   }
               };

               //     1      2     3       4       5       6        7
DeclSurface: DIRECTIVE_DECL VAR T_CLASS NUM_ELTS NUMBER  V_NAME GEN_ATTR
               {
                   attr_gen_struct temp_struct;
                   temp_struct.value = $7.value;
                   temp_struct.name = $7.name;
                   temp_struct.string_val = $7.string_val;
                   temp_struct.isInt = $7.isInt;
                   temp_struct.attr_set = $7.attr_set;
                   if (!pCisaBuilder->CISA_surface_variable_decl($2, (int)$5, $6, temp_struct, CISAlineno)) {
                       YYABORT; // error already reported
                   }
               };

/* ----- .input ------ */
               //     1          2       3        4        5
DirectiveInput: DIRECTIVE_INPUT VAR OFFSET_NUM SIZE_NUM GEN_ATTR
               {
                   pCisaBuilder->CISA_input_directive($2, (short)$3, (unsigned short)$4, CISAlineno);
               };

/* ----- .implicit inputs ------ */
               //              1        2       3        4        5
DirectiveImplicitInput: IMPLICIT_INPUT VAR OFFSET_NUM SIZE_NUM GEN_ATTR
               {
                   pCisaBuilder->CISA_implicit_input_directive($1, $2, (short)$3, (unsigned short)$4, CISAlineno);
               };

/* ----- .parameter ------ */
               //            1           2       3        4
DirectiveParameter: DIRECTIVE_PARAMETER VAR  SIZE_NUM GEN_ATTR
               {
                   pCisaBuilder->CISA_input_directive($2, 0, (unsigned short)$3, CISAlineno);
               };
/* ----- .attribute ------ */
               //     1               2     3         4
DirectiveAttr: DIRECTIVE_KERNEL_ATTR VAR EQUALS STRING_LITERAL {
                   pCisaBuilder->CISA_attr_directive($2, $4, CISAlineno);
               } |
               DIRECTIVE_KERNEL_ATTR VAR EQUALS NUMBER {
                   pCisaBuilder->CISA_attr_directiveNum($2, (uint32_t)$4, CISAlineno);
               } |
               DIRECTIVE_KERNEL_ATTR VAR EQUALS {
                   pCisaBuilder->CISA_attr_directive($2, nullptr, CISAlineno);
               };

/* ----- .function ----- */
               //     1           2
DirectiveFunc: DIRECTIVE_FUNC FUNCTION_NAME
               {
                   pCisaBuilder->CISA_function_directive($2);
               }

FUNCTION_NAME : VAR {$$ = $1;}
                | STRING_LITERAL {$$ = $1;};

SIZE_NUM : /* empty */
                {$$ = 1;}
              | SIZE NUMBER
                {$$ = $2;};

OFFSET_NUM : /* empty */
                {$$ = 0;}
              | OFFSET NUMBER
                {$$ = $2;};

AlignType : /* empty */
               {
                   $$ = ALIGN_BYTE;
               }
              | ALIGN ALIGNTYPE_2GRF {
                   $$ = ALIGN_BYTE;
              }
              | ALIGN VAR /* e.g. byte, word, dword, qword, GRF, GRFx2 */
               {
                   if (!ParseAlign(pCisaBuilder, $2, $$)) {
                       PARSE_ERROR($2, ": invalid ALIGN");
                   }
               }
              | ALIGN error
               {
                   PARSE_ERROR("syntax error in align attribute");
               };

AliasInfo : /* empty */
               {
                   $$.aliasname = NULL;
                   $$.offset = 0;
               }
              | ALIAS LANGLE VAR COMMA Exp RANGLE
               {
                   $$.aliasname = $3;
                   $$.offset = (int)$5;
               }
              | ALIAS LANGLE error
               {
                   PARSE_ERROR("syntax error in alias attribute");
               }
              | ALIAS LANGLE VAR error RANGLE
               {
                   PARSE_ERROR("syntax error in alias attribute");
                   YYABORT; // bail out
               }
              | error
               {
                   PARSE_ERROR("syntax error in alias attribute");
                   YYABORT; // bail out
               };

GEN_ATTR : /* Empty */
            {
                $$.name = "";
                $$.value = 0;
                $$.attr_set = false;
            }
            | DIRECTIVE_ATTR VAR EQUALS NUMBER
            {
              $$.name = $2;
              $$.isInt = true;
              $$.value = (int)$4;
              $$.attr_set = true;
            };
            | DIRECTIVE_ATTR VAR EQUALS
            {
              $$.name = $2;
              $$.isInt = false;
              $$.string_val = "";
              $$.attr_set = true;
            };
            //        1       2   3        4
            | DIRECTIVE_ATTR VAR EQUALS STRING_LITERAL
            {
              $$.name = $2;
              $$.isInt = false;
              $$.string_val = $4;
              $$.attr_set = true;
            };

/* ---------------------------------------------------------- */
/* --------------- Instructions ----------------------------- */
/* ---------------------------------------------------------- */

CISAInst: LogicInstruction
        | SvmInstruction
        | UnaryLogicInstruction
        | MathInstruction_2OPND
        | MathInstruction_3OPND
        | ArithInstruction_2OPND
        | ArithInstruction_3OPND
        | ArithInstruction_4OPND
        | AntiTrigInstruction
        | AddrAddInstruction
        | CmpInstruction
        | MediaInstruction
        | OwordInstruction
        | ScatterInstruction
        | ScatterTypedInstruction
        | ScatterScaledInstruction
        | Scatter4ScaledInstruction
        | SynchronizationInstruction
        | BranchInstruction
        | DwordAtomicInstruction
        | TypedAtomicInstruction
        | SampleInstruction
        | SampleUnormInstruction
        | VMEInstruction
        | AVSInstruction
        | MovsInstruction
        | MovInstruction
        | SelInstruction
        | MinInstruction
        | MaxInstruction
        | SetpInstruction
        | FILE
        | LOC
        | RawSendInstruction
        | RawSendsInstruction
        | Sample3dInstruction
        | Load3dInstruction
        | Gather43dInstruction
        | ResInfo3dInstruction
        | SampleInfo3dInstruction
        | RTWriteInstruction
        | URBWriteInstruction
        | LifetimeStartInst
        | LifetimeEndInst
        | NO_OPND_INST
        | LABEL
         {
             pCisaBuilder->CISA_create_label($1, CISAlineno);
         };


                        //   1       2                3             4           5                       6                      7
LogicInstruction : Predicate BINARY_LOGIC_OP InstModifier  ExecSize VecDstOperand_G_I VecSrcOperand_G_I_IMM VecSrcOperand_G_I_IMM
         {
             pCisaBuilder->CISA_create_logic_instruction($1.cisa_gen_opnd, $2, $3, $4.emask, $4.exec_size, $5.cisa_gen_opnd, $6.cisa_gen_opnd, $7.cisa_gen_opnd, NULL, NULL, CISAlineno);
         };
         | Predicate BINARY_LOGIC_OP InstModifier  ExecSize VAR VAR VAR
         {
             pCisaBuilder->CISA_create_logic_instruction($2, $4.emask, $4.exec_size, $5, $6, $7, CISAlineno);
         };
         | Predicate TERNARY_LOGIC_OP InstModifier  ExecSize VecDstOperand_G_I VecSrcOperand_G_I_IMM VecSrcOperand_G_I_IMM VecSrcOperand_G_I_IMM
         {
             pCisaBuilder->CISA_create_logic_instruction($1.cisa_gen_opnd, $2, $3, $4.emask, $4.exec_size, $5.cisa_gen_opnd, $6.cisa_gen_opnd, $7.cisa_gen_opnd, $8.cisa_gen_opnd, NULL, CISAlineno);
         };
         | Predicate QUATERNARY_LOGIC_OP InstModifier  ExecSize VecDstOperand_G_I VecSrcOperand_G_I_IMM VecSrcOperand_G_I_IMM VecSrcOperand_G_I_IMM VecSrcOperand_G_I_IMM
         {
             pCisaBuilder->CISA_create_logic_instruction($1.cisa_gen_opnd, $2, $3, $4.emask, $4.exec_size, $5.cisa_gen_opnd, $6.cisa_gen_opnd, $7.cisa_gen_opnd, $8.cisa_gen_opnd, $9.cisa_gen_opnd, CISAlineno);
         };

                       //   1       2                3            4             5                    6
UnaryLogicInstruction : Predicate UNARY_LOGIC_OP InstModifier  ExecSize VecDstOperand_G_I VecSrcOperand_G_I_IMM
         {
             pCisaBuilder->CISA_create_logic_instruction($1.cisa_gen_opnd, $2, $3, $4.emask, $4.exec_size, $5.cisa_gen_opnd, $6.cisa_gen_opnd, NULL, NULL, NULL, CISAlineno);
         }
         | Predicate UNARY_LOGIC_OP InstModifier  ExecSize VAR VAR
         {
             pCisaBuilder->CISA_create_logic_instruction($2, $4.emask, $4.exec_size, $5, $6, NULL, CISAlineno);
         };

                    //  1         2        3            4        5                 6
MathInstruction_2OPND : Predicate MATH2_OP InstModifier ExecSize VecDstOperand_G_I VecSrcOperand_G_I_IMM
         {
             pCisaBuilder->CISA_create_math_instruction($1.cisa_gen_opnd, $2, $3, $4.emask, $4.exec_size, $5.cisa_gen_opnd, $6.cisa_gen_opnd, NULL, CISAlineno);
         };

                    //      1         2        3            4        5                 6                     7
MathInstruction_3OPND : Predicate MATH3_OP InstModifier ExecSize VecDstOperand_G_I VecSrcOperand_G_I_IMM VecSrcOperand_G_I_IMM
         {
             pCisaBuilder->CISA_create_math_instruction($1.cisa_gen_opnd, $2, $3, $4.emask, $4.exec_size, $5.cisa_gen_opnd, $6.cisa_gen_opnd, $7.cisa_gen_opnd, CISAlineno);
         };

                         //  1        2        3            4              5             6
ArithInstruction_2OPND : Predicate ARITH2_OP InstModifier ExecSize VecDstOperand_G_I VecSrcOperand_G_I_IMM
         {
             //pBuilder->CISA_create_arith_instruction($1, $2, $3, $4.emask, $4.exec_size, $5.opnd, $6.opnd, NULL, NULL, CISAlineno);
             pCisaBuilder->CISA_create_arith_instruction($1.cisa_gen_opnd, $2, $3, $4.emask, $4.exec_size, $5.cisa_gen_opnd, $6.cisa_gen_opnd, NULL, NULL, CISAlineno);
         };

                         //  1        2           3          4            5                 6                7
ArithInstruction_3OPND : Predicate ARITH3_OP InstModifier ExecSize VecDstOperand_G_I VecSrcOperand_G_I_IMM VecSrcOperand_G_I_IMM
         {
             MUST_BE_TRUE1(!(($2 == ISA_LINE) && ($6.type == OPERAND_IMMEDIATE || $6.type == OPERAND_INDIRECT)), CISAlineno, "Wrong type of first src operand for LINE instruction");
             //pBuilder->CISA_create_arith_instruction($1, $2, $3, $4.emask, $4.exec_size, $5.opnd, $6.opnd, $7.opnd, NULL, CISAlineno);
             pCisaBuilder->CISA_create_arith_instruction($1.cisa_gen_opnd, $2, $3, $4.emask, $4.exec_size, $5.cisa_gen_opnd, $6.cisa_gen_opnd, $7.cisa_gen_opnd, NULL, CISAlineno);
         };

                         //  1        2         3           4             5                   6              7                         8
ArithInstruction_4OPND : Predicate ARITH4_OP InstModifier ExecSize VecDstOperand_G_I VecSrcOperand_G_I_IMM VecSrcOperand_G_I_IMM  VecSrcOperand_G_I_IMM
         {
             //pBuilder->CISA_create_arith_instruction($1, $2, $3, $4.emask, $4.exec_size, $5.opnd, $6.opnd, $7.opnd, $8.opnd, CISAlineno);
             pCisaBuilder->CISA_create_arith_instruction($1.cisa_gen_opnd, $2, $3, $4.emask, $4.exec_size, $5.cisa_gen_opnd, $6.cisa_gen_opnd, $7.cisa_gen_opnd, $8.cisa_gen_opnd, CISAlineno);
         };
         //  1          2         3           4                   5                   6                   7
         |Predicate ARITH4_OP2 ExecSize VecDstOperand_G_I VecDstOperand_G_I VecSrcOperand_G_I_IMM  VecSrcOperand_G_I_IMM
         {
            pCisaBuilder->CISA_create_arith_instruction2($1.cisa_gen_opnd, $2, $3.emask, $3.exec_size, $4.cisa_gen_opnd, $5.cisa_gen_opnd, $6.cisa_gen_opnd, $7.cisa_gen_opnd, CISAlineno);
         }


                     //  1            2           3            4             5                6
AntiTrigInstruction : Predicate ANTI_TRIG_OP InstModifier ExecSize VecDstOperand_G_I VecSrcOperand_G_I_IMM
         {
             pCisaBuilder->CISA_create_invtri_inst($1.cisa_gen_opnd, $2, $3, $4.emask, $4.exec_size, $5.cisa_gen_opnd, $6.cisa_gen_opnd, CISAlineno);
         };

                     //  1            2           3              4                     5
AddrAddInstruction : ADDR_ADD_OP ExecSize VecDstOperand_A VecSrcOperand_A_G  VecSrcOperand_G_IMM
         {
             pCisaBuilder->CISA_create_address_instruction($1, $2.emask, $2.exec_size, $3.cisa_gen_opnd, $4.cisa_gen_opnd, $5.cisa_gen_opnd, CISAlineno);
         };

                //   1       2        3                  4
SetpInstruction : SETP_OP ExecSize   VAR  VecSrcOperand_G_I_IMM
         {
             pCisaBuilder->CISA_create_setp_instruction($1, $2.emask, $2.exec_size, $3, $4.cisa_gen_opnd, CISAlineno);
         };

                //   1       2       3            4            5                   6                   7
SelInstruction : Predicate SEL_OP InstModifier ExecSize VecDstOperand_G_I VecSrcOperand_G_I_IMM VecSrcOperand_G_I_IMM
         {
             pCisaBuilder->CISA_create_sel_instruction($2, $3, $1.cisa_gen_opnd, $4.emask, $4.exec_size, $5.cisa_gen_opnd, $6.cisa_gen_opnd, $7.cisa_gen_opnd, CISAlineno);
         };

                //   1      2           3           4      5                   6                   7           8
MinInstruction : Predicate MIN_OP InstModifier ExecSize VecDstOperand_G_I VecSrcOperand_G_I_IMM VecSrcOperand_G_I_IMM
         {
             pCisaBuilder->CISA_create_fminmax_instruction(0, ISA_FMINMAX, $3, $1.cisa_gen_opnd, $4.emask, $4.exec_size, $5.cisa_gen_opnd, $6.cisa_gen_opnd, $7.cisa_gen_opnd, CISAlineno);
         };

                //   1      2           3           4      5                   6                   7           8
MaxInstruction : Predicate MAX_OP InstModifier ExecSize VecDstOperand_G_I VecSrcOperand_G_I_IMM VecSrcOperand_G_I_IMM
         {
             pCisaBuilder->CISA_create_fminmax_instruction(1, ISA_FMINMAX, $3, $1.cisa_gen_opnd, $4.emask, $4.exec_size, $5.cisa_gen_opnd, $6.cisa_gen_opnd, $7.cisa_gen_opnd, CISAlineno);
         };

                //   1       2         3          4              5               6
MovInstruction : Predicate MOV_OP InstModifier ExecSize VecDstOperand_G_I VecSrcOperand_G_I_IMM_A
         {
             pCisaBuilder->CISA_create_mov_instruction($1.cisa_gen_opnd, $2, $4.emask, $4.exec_size, $3, $5.cisa_gen_opnd, $6.cisa_gen_opnd, CISAlineno);
         };
         | Predicate MOV_OP InstModifier ExecSize VecDstOperand_G_I VAR
         {
             pCisaBuilder->CISA_create_mov_instruction($5.cisa_gen_opnd, $6, CISAlineno);
         };

                //   1       2            3            4
MovsInstruction : MOVS_OP ExecSize DstStateOperand SrcStateOperand
         {
             pCisaBuilder->CISA_create_movs_instruction($2.emask, ISA_MOVS, $2.exec_size, $3.cisa_gen_opnd, $4.cisa_gen_opnd, CISAlineno);
         };
         | MOVS_OP ExecSize VecDstOperand_G SrcStateOperand
         {
           pCisaBuilder->CISA_create_movs_instruction($2.emask, ISA_MOVS, $2.exec_size, $3.cisa_gen_opnd, $4.cisa_gen_opnd, CISAlineno);
         };
         | MOVS_OP ExecSize DstStateOperand VecSrcOperand_G_I_IMM
         {
           pCisaBuilder->CISA_create_movs_instruction($2.emask, ISA_MOVS, $2.exec_size, $3.cisa_gen_opnd, $4.cisa_gen_opnd, CISAlineno);
         };

                 //   1          2            3        4                  5                6
CmpInstruction :  CMP_OP ConditionalModifier ExecSize VAR VecSrcOperand_G_I_IMM VecSrcOperand_G_I_IMM
         {
             pCisaBuilder->CISA_create_cmp_instruction($2.cisa_mod, ISA_CMP, $3.emask, $3.exec_size, $4, $5.cisa_gen_opnd, $6.cisa_gen_opnd, CISAlineno);
         };
         //    1        2                    3        4                    5                        6
         |  CMP_OP ConditionalModifier ExecSize VecDstOperand_G_I VecSrcOperand_G_I_IMM VecSrcOperand_G_I_IMM
         {
             pCisaBuilder->CISA_create_cmp_instruction($2.cisa_mod, ISA_CMP, $3.emask, $3.exec_size, $4.cisa_gen_opnd, $5.cisa_gen_opnd, $6.cisa_gen_opnd, CISAlineno);
         };
                 //    1       2          3           4     5               6                    7                8
MediaInstruction : MEDIA_OP MEDIA_MODE TwoDimOffset VAR PlaneID VecSrcOperand_G_I_IMM VecSrcOperand_G_I_IMM  RawOperand
         {
             pCisaBuilder->CISA_create_media_instruction($1, $2, $3.row, $3.elem, (int)$5, $4, $6.cisa_gen_opnd, $7.cisa_gen_opnd, $8.cisa_gen_opnd, CISAlineno);
         };

                   //    1          2        3        4       5         6                  7           8
ScatterInstruction : SCATTER_OP ElemNum ExecSize OwordModifier VAR VecSrcOperand_G_I_IMM RawOperand RawOperand
         {
             pCisaBuilder->CISA_create_scatter_instruction($1, (int) $2, $3.emask, $3.exec_size, $4, $5, $6.cisa_gen_opnd, $7.cisa_gen_opnd, $8.cisa_gen_opnd, CISAlineno);
        };

                //              1             2                 3            4       5         6           7             8           9            10
ScatterTypedInstruction :  Predicate   SCATTER_TYPED_OP  SAMPLER_CHANNEL  ExecSize  VAR    RawOperand   RawOperand   RawOperand  RawOperand    RawOperand
        {
            pCisaBuilder->CISA_create_scatter4_typed_instruction($2, $1.cisa_gen_opnd, ChannelMask::createFromAPI($3), $4.emask, $4.exec_size, $5, $6.cisa_gen_opnd, $7.cisa_gen_opnd, $8.cisa_gen_opnd, $9.cisa_gen_opnd, $10.cisa_gen_opnd, CISAlineno);
        };

//                              1           2               3               4      5        6                   7            8
Scatter4ScaledInstruction : Predicate SCATTER4_SCALED_OP SAMPLER_CHANNEL  ExecSize VAR VecSrcOperand_G_I_IMM RawOperand RawOperand
        {
            pCisaBuilder->CISA_create_scatter4_scaled_instruction($2, $1.cisa_gen_opnd, $4.emask, $4.exec_size, ChannelMask::createFromAPI($3), $5, $6.cisa_gen_opnd, $7.cisa_gen_opnd, $8.cisa_gen_opnd, CISAlineno);
        };

//                                 1                 2   3      4   5      6   7                        8        9
ScatterScaledInstruction : Predicate SCATTER_SCALED_OP DOT NUMBER ExecSize VAR VecSrcOperand_G_I_IMM RawOperand RawOperand
        {
            pCisaBuilder->CISA_create_scatter_scaled_instruction($2, $1.cisa_gen_opnd, $5.emask, $5.exec_size, (uint32_t) $4, $6, $7.cisa_gen_opnd, $8.cisa_gen_opnd, $9.cisa_gen_opnd, CISAlineno);
        };

SynchronizationInstruction:
              BARRIER_OP
            {
                pCisaBuilder->CISA_create_sync_instruction($1);
            };
            | SBARRIER_SIGNAL
            {
                pCisaBuilder->CISA_create_sbarrier_instruction(true);
            };
            | SBARRIER_WAIT
            {
                pCisaBuilder->CISA_create_sbarrier_instruction(false);
            };

//                      1         2               3             4           5        6   7          8          9          10
DwordAtomicInstruction: Predicate DWORD_ATOMIC_OP ATOMIC_SUB_OP IS_ATOMIC16 ExecSize VAR RawOperand RawOperand RawOperand RawOperand
    {
        pCisaBuilder->CISA_create_dword_atomic_instruction($1.cisa_gen_opnd, $3, $4, $5.emask, $5.exec_size, $6, $7.cisa_gen_opnd, $8.cisa_gen_opnd, $9.cisa_gen_opnd, $10.cisa_gen_opnd, CISAlineno);
    }

//                      1         2               3             4           5        6   7          8          9          10         11         12         13
TypedAtomicInstruction: Predicate TYPED_ATOMIC_OP ATOMIC_SUB_OP IS_ATOMIC16 ExecSize VAR RawOperand RawOperand RawOperand RawOperand RawOperand RawOperand RawOperand
    {
        pCisaBuilder->CISA_create_typed_atomic_instruction($1.cisa_gen_opnd, $3, $4, $5.emask, $5.exec_size, $6,
        $7.cisa_gen_opnd, $8.cisa_gen_opnd, $9.cisa_gen_opnd, $10.cisa_gen_opnd, $11.cisa_gen_opnd, $12.cisa_gen_opnd, $13.cisa_gen_opnd, CISAlineno);
    }

                 //            1               2               3        4   5               6                   7                      8                    9              10
SampleUnormInstruction: SAMPLE_UNORM_OP SAMPLER_CHANNEL CHANNEL_OUTPUT VAR VAR VecSrcOperand_G_I_IMM VecSrcOperand_G_I_IMM VecSrcOperand_G_I_IMM VecSrcOperand_G_I_IMM RawOperand
           {
              pCisaBuilder->CISA_create_sampleunorm_instruction($1, ChannelMask::createFromAPI($2), $3, $4, $5, $6.cisa_gen_opnd, $7.cisa_gen_opnd, $8.cisa_gen_opnd, $9.cisa_gen_opnd, $10.cisa_gen_opnd, CISAlineno);
           };

                 //    1          2               3    4   5        6       7          8          9
SampleInstruction: SAMPLE_OP SAMPLER_CHANNEL SIMDMode VAR VAR RawOperand RawOperand RawOperand RawOperand
           {
               pCisaBuilder->CISA_create_sample_instruction($1, ChannelMask::createFromAPI($2), (int)$3, $4, $5, $6.cisa_gen_opnd, $7.cisa_gen_opnd, $8.cisa_gen_opnd, $9.cisa_gen_opnd, CISAlineno);
           };
           |
           // 1             2          3       4     5           6         7           8
           SAMPLE_OP SAMPLER_CHANNEL SIMDMode VAR RawOperand RawOperand RawOperand RawOperand
           {
               pCisaBuilder->CISA_create_sample_instruction($1, ChannelMask::createFromAPI($2), (int)$3, "", $4, $5.cisa_gen_opnd, $6.cisa_gen_opnd, $7.cisa_gen_opnd, $8.cisa_gen_opnd, CISAlineno);
           };

           //        1         2            3                      4          5                             6                 7          8                        9   10  11            12
Sample3dInstruction: Predicate SAMPLE_3D_OP PIXEL_NULL_MASK_ENABLE CPS_ENABLE NON_UNIFORM_SAMPLER_ENABLE SAMPLER_CHANNEL ExecSize VecSrcOperand_G_I_IMM VAR VAR RawOperand RawOperandArray
           {
               pCisaBuilder->create3DSampleInstruction( $1.cisa_gen_opnd, $2, $3, $4, $5, ChannelMask::createFromAPI($6), $7.emask, $7.exec_size, $8.cisa_gen_opnd, $9, $10, $11.cisa_gen_opnd, (unsigned int) $12, rawOperandArray, CISAlineno);
           };

           //      1         2          3                      4               5        6                        7   8          9
Load3dInstruction: Predicate LOAD_3D_OP PIXEL_NULL_MASK_ENABLE SAMPLER_CHANNEL ExecSize VecSrcOperand_G_I_IMM VAR RawOperand RawOperandArray
           {
               pCisaBuilder->create3DLoadInstruction( $1.cisa_gen_opnd, $2, $3, ChannelMask::createFromAPI($4), $5.emask, $5.exec_size, $6.cisa_gen_opnd, $7, $8.cisa_gen_opnd, (unsigned int) $9, rawOperandArray, CISAlineno);
           };

           //         1         2             3                      4               5        6                        7   8   9          10
Gather43dInstruction: Predicate SAMPLE4_3D_OP PIXEL_NULL_MASK_ENABLE SAMPLER_CHANNEL ExecSize VecSrcOperand_G_I_IMM VAR VAR RawOperand RawOperandArray
           {
              pCisaBuilder->createSample4Instruction( $1.cisa_gen_opnd, $2, $3, ChannelMask::createFromAPI($4), $5.emask, $5.exec_size, $6.cisa_gen_opnd, $7, $8, $9.cisa_gen_opnd, (unsigned int) $10, rawOperandArray, CISAlineno );
           };

            //          1                   2              3           4           5              6
ResInfo3dInstruction: RESINFO_OP_3D     ExecSize   SAMPLER_CHANNEL    VAR     RawOperand      RawOperand
           {
                pCisaBuilder->CISA_create_info_3d_instruction( VISA_3D_RESINFO, $2.emask, $2.exec_size, ChannelMask::createFromAPI($3), $4, $5.cisa_gen_opnd, $6.cisa_gen_opnd, CISAlineno );
           };

           //               1                   2           3           4          5
SampleInfo3dInstruction: SAMPLEINFO_OP_3D   ExecSize  SAMPLER_CHANNEL  VAR     RawOperand
           {
                pCisaBuilder->CISA_create_info_3d_instruction( VISA_3D_SAMPLEINFO, $2.emask, $2.exec_size, ChannelMask::createFromAPI($3), $4, NULL, $5.cisa_gen_opnd, CISAlineno );
           };

RTWriteOperandParse: /* empty */
            {
            }
            | RTWriteOperandParse VecSrcOperand_G_IMM
            {
                RTWriteOperands.push_back($2.cisa_gen_opnd);
            }
            | RTWriteOperandParse RawOperand
            {
                RTWriteOperands.push_back($2.cisa_gen_opnd);
            }
            //          1           2               3               4         5     6
RTWriteInstruction: Predicate    RTWRITE_OP_3D    RTWRITE_MODE    ExecSize    VAR   RTWriteOperandParse
           {
               pCisaBuilder->CISA_create_rtwrite_3d_instruction( $1.cisa_gen_opnd, $3, $4.emask, (unsigned int)$4.exec_size, $5,
                                                                  RTWriteOperands, CISAlineno );
               RTWriteOperands.clear();
           };

            //          1           2               3           4        5          6         7             8               9
URBWriteInstruction: Predicate  URBWRITE_OP_3D    ExecSize    NUMBER    NUMBER    RawOperand RawOperand    RawOperand    RawOperand
           {
               pCisaBuilder->CISA_create_urb_write_3d_instruction( $1.cisa_gen_opnd, $3.emask, (unsigned int)$3.exec_size, (unsigned int)$4, (unsigned int)$5, $6.cisa_gen_opnd, $7.cisa_gen_opnd, $8.cisa_gen_opnd, $9.cisa_gen_opnd, CISAlineno );
           };

            //          1         2   3     4                       5                       6                   7                       8                   9                   10                  11  12                    13 14  15
AVSInstruction : AVS_OP SAMPLER_CHANNEL VAR VAR VecSrcOperand_G_I_IMM VecSrcOperand_G_I_IMM VecSrcOperand_G_I_IMM VecSrcOperand_G_I_IMM VecSrcOperand_G_I_IMM VecSrcOperand_G_I_IMM VecSrcOperand_G_I_IMM CNTRL VecSrcOperand_G_I_IMM EXECMODE VecSrcOperand_G_I_IMM RawOperand
           {
               pCisaBuilder->CISA_create_avs_instruction(ChannelMask::createFromAPI($2), $3, $4, $5.cisa_gen_opnd, $6.cisa_gen_opnd, $7.cisa_gen_opnd, $8.cisa_gen_opnd, $9.cisa_gen_opnd, $10.cisa_gen_opnd, $11.cisa_gen_opnd, $12, $13.cisa_gen_opnd, $14, $15.cisa_gen_opnd, $16.cisa_gen_opnd, CISAlineno);
           };


VMEInstruction :
              //     1          2     3       4         5           6         7           8         9
               VME_IME_OP VMEOpndIME VAR RawOperand RawOperand  RawOperand RawOperand RawOperand RawOperand
           {
           /*
                1 - OP
                2 - StreamMode, SearchCtrl
                3 - Surface
                4 - UNIInput
                5 - IMEInput
                6 - ref0
                7 - ref1
                8 - CostCenter
                9 - Output
           */
                pCisaBuilder->CISA_create_vme_ime_instruction($1, $2.streamMode, $2.searchCtrl, $4.cisa_gen_opnd, $5.cisa_gen_opnd, $3,
                    $6.cisa_gen_opnd,$7.cisa_gen_opnd, $8.cisa_gen_opnd, $9.cisa_gen_opnd, CISAlineno);
           };
           |
            //    1      2      3           4          5
             VME_SIC_OP VAR RawOperand RawOperand  RawOperand
           {
                pCisaBuilder->CISA_create_vme_sic_instruction($1, $3.cisa_gen_opnd, $4.cisa_gen_opnd, $2, $5.cisa_gen_opnd, CISAlineno);
           };
           |
           //    1          2       3      4          5         6
             VME_FBR_OP VMEOpndFBR VAR RawOperand RawOperand RawOperand
             {
                /*
                    1 - OP
                    2 - FBRMdMode, FBRSubMbShape, FBRSubPredMode
                    3 - surface
                    4 - UNIInput
                    5 - FBRInput
                    6 - output
                */
                pCisaBuilder->CISA_create_vme_fbr_instruction($1, $4.cisa_gen_opnd, $5.cisa_gen_opnd, $3,
                    $2.cisa_fbrMbMode_opnd, $2.cisa_fbrSubMbShape_opnd, $2.cisa_fbrSubPredMode_opnd, $6.cisa_gen_opnd, CISAlineno);
             };

                 //    1         2          3       4            5               6
OwordInstruction : OWORD_OP OwordModifier ExecSize VAR VecSrcOperand_G_I_IMM RawOperand
         {
             pCisaBuilder->CISA_create_oword_instruction($1, $2, $3.exec_size, $4, $5.cisa_gen_opnd, $6.cisa_gen_opnd, CISAlineno);
         }

SvmInstruction:
//     2        3                     4
SVM_OP ExecSize VecSrcOperand_G_I_IMM RawOperand
{
    pCisaBuilder->CISA_create_svm_block_instruction((SVMSubOpcode)$1, $2.exec_size, false/*unaligned*/, $3.cisa_gen_opnd, $4.cisa_gen_opnd, CISAlineno);
}
//          2              3   4      5   6      7        8          9
| Predicate SVM_SCATTER_OP DOT NUMBER DOT NUMBER ExecSize RawOperand RawOperand
{
    pCisaBuilder->CISA_create_svm_scatter_instruction($1.cisa_gen_opnd, (SVMSubOpcode)$2, $7.emask, $7.exec_size, (unsigned int)$4, (unsigned int)$6, $8.cisa_gen_opnd, $9.cisa_gen_opnd, CISAlineno);
}
//          2             3             4               5        6          7          8          9
| Predicate SVM_ATOMIC_OP ATOMIC_SUB_OP ATOMIC_BITWIDTH ExecSize RawOperand RawOperand RawOperand RawOperand
{
    pCisaBuilder->CISA_create_svm_atomic_instruction($1.cisa_gen_opnd, $5.emask, $5.exec_size, $3, $4, $6.cisa_gen_opnd, $8.cisa_gen_opnd, $9.cisa_gen_opnd, $7.cisa_gen_opnd, CISAlineno);
}
//        1                    2               3   4      5                         6          7
| Predicate SVM_GATHER4SCALED_OP SAMPLER_CHANNEL ExecSize VecSrcOperand_G_I_IMM RawOperand RawOperand
{
    pCisaBuilder->CISA_create_svm_gather4_scaled($1.cisa_gen_opnd, $4.emask, $4.exec_size, ChannelMask::createFromAPI($3), $5.cisa_gen_opnd, $6.cisa_gen_opnd, $7.cisa_gen_opnd, CISAlineno);
}
//        1                     2               3   4      5                        6          7
| Predicate SVM_SCATTER4SCALED_OP SAMPLER_CHANNEL ExecSize VecSrcOperand_G_I_IMM RawOperand RawOperand
{
    pCisaBuilder->CISA_create_svm_scatter4_scaled($1.cisa_gen_opnd, $4.emask, $4.exec_size, ChannelMask::createFromAPI($3), $5.cisa_gen_opnd, $6.cisa_gen_opnd, $7.cisa_gen_opnd, CISAlineno);
}


SwitchLabels: /* empty */
               {
                $$ = 0;
               }
               | COMMA SwitchLabels
               {
                $$ = $2;
               }
               | VAR SwitchLabels
               {
                    switch_label_array[$2++] = $1;
                    $$ = $2;
               }

                   // 1        2         3          4
BranchInstruction : Predicate BRANCH_OP ExecSize TargetLabel
         {
             pCisaBuilder->CISA_create_branch_instruction($1.cisa_gen_opnd, $2, $3.emask, $3.exec_size, $4, CISAlineno);
         };
         | Predicate BRANCH_OP ExecSize
         {
             pCisaBuilder->CISA_Create_Ret($1.cisa_gen_opnd, $2, $3.emask, $3.exec_size, CISAlineno);
         };
         | SWITCHJMP_OP ExecSize VecSrcOperand_G_I_IMM LPAREN SwitchLabels RPAREN
         {
            pCisaBuilder->CISA_create_switch_instruction($1, $2.exec_size, $3.cisa_gen_opnd, (int)$5, switch_label_array, CISAlineno);
         }
         //  1          2         3       4        5         6
         | Predicate  FCALL   ExecSize SymbolName NUMBER NUMBER
         {
            pCisaBuilder->CISA_create_fcall_instruction($1.cisa_gen_opnd, $2, $3.emask, $3.exec_size, $4, (unsigned)$5, (unsigned)$6, CISAlineno);
         }
         // 1           2       3       4                   5       6
         | Predicate IFCALL ExecSize VecSrcOperand_G_I_IMM NUMBER NUMBER
         {
            pCisaBuilder->CISA_create_ifcall_instruction($1.cisa_gen_opnd, $3.emask, $3.exec_size,
            $4.cisa_gen_opnd, (unsigned)$5, (unsigned)$6, CISAlineno);
         }
         // 1       2          3
         | FADDR  SymbolName VecDstOperand_G_I
         {
            pCisaBuilder->CISA_create_faddr_instruction($2, $3.cisa_gen_opnd, CISAlineno);
         }

       // 1          2
FILE : FILE_OP STRING_LITERAL
        {
            pCisaBuilder->CISA_create_FILE_instruction($1, $2);
        }

LOC : LOC_OP NUMBER
        {
            pCisaBuilder->CISA_create_LOC_instruction($1, (unsigned)$2);
        };
        //              1             2            3       4       5      6            7               8           9
RawSendInstruction: Predicate  RAW_SEND_STRING  ExecSize HEX_NUMBER NUMBER NUMBER VecSrcOperand_G_IMM RawOperand RawOperand
        {
            pCisaBuilder->CISA_create_raw_send_instruction(ISA_RAW_SEND, false, $3.emask, $3.exec_size, $1.cisa_gen_opnd, (unsigned)$4, (unsigned char)$5, (unsigned char)$6, $7.cisa_gen_opnd, $8.cisa_gen_opnd, $9.cisa_gen_opnd, CISAlineno);
        };
        //    1             2               3       4       5      6            7               8           9
        | Predicate  RAW_SENDC_STRING  ExecSize HEX_NUMBER NUMBER NUMBER VecSrcOperand_G_IMM RawOperand RawOperand
        {
            pCisaBuilder->CISA_create_raw_send_instruction(ISA_RAW_SEND, true, $3.emask, $3.exec_size, $1.cisa_gen_opnd, (unsigned)$4, (unsigned char)$5, (unsigned char)$6, $7.cisa_gen_opnd, $8.cisa_gen_opnd, $9.cisa_gen_opnd, CISAlineno);
        }

        //            1                        2
LifetimeStartInst: LIFETIME_START_OP        VAR
        {
            pCisaBuilder->CISA_create_lifetime_inst((unsigned char)0, $2, CISAlineno);
        };

        //            1                        2
LifetimeEndInst:  LIFETIME_END_OP            VAR
        {
            pCisaBuilder->CISA_create_lifetime_inst((unsigned char)1, $2, CISAlineno);
        };
        //              1             2           3        4       5      6          7              8                  9               10         11        12
RawSendsInstruction: Predicate RAW_SENDS_STRING ElemNum ElemNum  ElemNum ElemNum ExecSize VecSrcOperand_G_IMM   VecSrcOperand_G_IMM RawOperand RawOperand RawOperand
        {
            pCisaBuilder->CISA_create_raw_sends_instruction(ISA_RAW_SENDS, false, false, $7.emask, $7.exec_size, $1.cisa_gen_opnd, $8.cisa_gen_opnd, (unsigned char)$3, (unsigned char)$4,
                (unsigned char)$5, (unsigned char)$6, $9.cisa_gen_opnd, $10.cisa_gen_opnd, $11.cisa_gen_opnd, $12.cisa_gen_opnd, CISAlineno);
        };
        //    1             2               3        4       5      6          7              8                  9               10         11        12
        | Predicate RAW_SENDS_EOT_STRING ElemNum ElemNum  ElemNum ElemNum ExecSize VecSrcOperand_G_IMM   VecSrcOperand_G_IMM RawOperand RawOperand RawOperand
        {
            pCisaBuilder->CISA_create_raw_sends_instruction(ISA_RAW_SENDS, false, true, $7.emask, $7.exec_size, $1.cisa_gen_opnd, $8.cisa_gen_opnd, (unsigned char)$3, (unsigned char)$4,
                (unsigned char)$5, (unsigned char)$6, $9.cisa_gen_opnd, $10.cisa_gen_opnd, $11.cisa_gen_opnd, $12.cisa_gen_opnd, CISAlineno);
        };
        //    1             2              3       4       5        6        7                         8             9          10        11
        | Predicate  RAW_SENDSC_STRING  ElemNum ElemNum ElemNum ExecSize VecSrcOperand_G_IMM VecSrcOperand_G_IMM RawOperand RawOperand RawOperand
        {
            pCisaBuilder->CISA_create_raw_sends_instruction(ISA_RAW_SENDS, true, false, $6.emask, $6.exec_size, $1.cisa_gen_opnd, $7.cisa_gen_opnd, 0, (unsigned char)$3,
                (unsigned char)$4, (unsigned char)$5, $8.cisa_gen_opnd, $9.cisa_gen_opnd, $10.cisa_gen_opnd, $11.cisa_gen_opnd, CISAlineno);
        };
        //    1             2                  3       4       5        6        7                         8             9          10        11
        | Predicate  RAW_SENDSC_EOT_STRING  ElemNum ElemNum ElemNum ExecSize VecSrcOperand_G_IMM VecSrcOperand_G_IMM RawOperand RawOperand RawOperand
        {
            pCisaBuilder->CISA_create_raw_sends_instruction(ISA_RAW_SENDS, true, true, $6.emask, $6.exec_size, $1.cisa_gen_opnd, $7.cisa_gen_opnd, 0, (unsigned char)$3,
                (unsigned char)$4, (unsigned char)$5, $8.cisa_gen_opnd, $9.cisa_gen_opnd, $10.cisa_gen_opnd, $11.cisa_gen_opnd, CISAlineno);
        };

NO_OPND_INST: CACHE_FLUSH_OP
             {
                pCisaBuilder->CISA_create_NO_OPND_instruction($1);
              };
              | WAIT_OP VecSrcOperand_G_IMM
              {
                pCisaBuilder->CISA_create_wait_instruction($2.cisa_gen_opnd);
              };
              | YIELD_OP
              {
                  pCisaBuilder->CISA_create_yield_instruction($1);
              };
              | FENCE_GLOBAL_OP
              {
                    pCisaBuilder->CISA_create_fence_instruction($1, 0x0);
              }
              | FENCE_GLOBAL_OP FENCE_OPTIONS
              {
                    pCisaBuilder->CISA_create_fence_instruction($1, $2);
              }
              | FENCE_LOCAL_OP
              {
                    pCisaBuilder->CISA_create_fence_instruction($1, 0x20);
              }
              | FENCE_LOCAL_OP FENCE_OPTIONS
              {
                    pCisaBuilder->CISA_create_fence_instruction($1, $2 | 0x20);
              }
              | FENCE_SW_OP
              {
                    pCisaBuilder->CISA_create_fence_instruction($1, 0x80);
              }



OwordModifier:  {$$ = false;}
              | OWORD_MODIFIER
                {$$ = $1;};

TargetLabel: VAR
            { $$ = $1; };
           | STRING_LITERAL
            { $$ = $1; };

SymbolName: VAR
            { $$ = $1; };

/* ------------- Scope ------------- */

ScopeOp: LBRACE
    {
        pCisaBuilder->CISA_push_decl_scope();
    }
    |  RBRACE
    {
        pCisaBuilder->CISA_pop_decl_scope();
    };

/* ------ predicate and Modifiers ------ */

Predicate :   /* empty */
            {
             $$.cisa_gen_opnd = NULL;
            }
          | LPAREN PredState VAR PredCntrl RPAREN
            {
                char upppered[20];  //FIXME, there should be single name in the dumpling, lower/Upper changes should not be allowed.
                int str_len = strlen($3);
                for (int i = 0; i < str_len; i++) {
                    upppered[i] = toupper($3[i]);
                }
                upppered[str_len]='\0';
                $$.cisa_gen_opnd = pCisaBuilder->CISA_create_predicate_operand(upppered, MODIFIER_NONE, $2.cisa_state, $4, CISAlineno);
            };

PredState :   /* empty */
            {
                $$.cisa_state = PredState_NO_INVERSE;
                //$$.state = PredState_Plus;
            }
          | BANG
            {
                $$.cisa_state = PredState_INVERSE;
                //$$.state = PredState_Minus;
            };

PredCntrl :   /* empty */
            {$$ = "";}
          | PRED_CNTL  /* any2h, any4h, ... all2h, all4h, ... */
            {$$ = $1;};

InstModifier :  /* empty */
            {$$ = false;}
           | SAT     /* .sat */
           {$$ = true;};

SrcModifier :
      SRCMOD_NEG    {$$.mod = MODIFIER_NEG;}
    | SRCMOD_ABS    {$$.mod = MODIFIER_ABS;}
    | SRCMOD_NEGABS {$$.mod = MODIFIER_NEG_ABS;}
    | SRCMOD_NOT    {$$.mod = MODIFIER_NOT;}

ConditionalModifier :   /* empty */
            {
                //$$.mod = Mod_cond_undef;
                $$.cisa_mod = ISA_CMP_UNDEF;
             }
          | COND_MOD
          {
            //$$.mod = Get_G4_CondModifier_From_Common_ISA_CondModifier($1);
            $$.cisa_mod = $1;
          };

/* ------ operands groups ------ */

/* ------ DST -----------*/
VecDstOperand_A : DstAddrOperand
                { $$ = $1; $$.type = OPERAND_ADDRESS; };

VecDstOperand_G : DstGeneralOperand
                { $$ = $1; $$.type = OPERAND_GENERAL; };

VecDstOperand_G_I : DstGeneralOperand
                { $$ = $1; $$.type = OPERAND_GENERAL; }
                | DstIndirectOperand
                { $$ = $1; $$.type = OPERAND_INDIRECT; };

/* ------ SRC -----------*/
VecSrcOperand_G_I_IMM_A : SrcImmOperand
                  { $$ = $1; $$.type = OPERAND_IMMEDIATE; }
                | SrcIndirectOperand_1
                  { $$ = $1; $$.type = OPERAND_INDIRECT; }
                | SrcIndirectOperand
                  { $$ = $1; $$.type = OPERAND_INDIRECT; }
                | SrcGeneralOperand
                  { $$ = $1; $$.type = OPERAND_GENERAL; };
                | SrcGeneralOperand_1
                  { $$ = $1; $$.type = OPERAND_GENERAL; };
                | SrcAddrOperand
                  { $$ = $1; $$.type = OPERAND_ADDRESS; }
                | AddrOfOperand
                  { $$ = $1; $$.type = OPERAND_ADDRESSOF; };

VecSrcOperand_G_I_IMM : SrcImmOperand
                  { $$ = $1; $$.type = OPERAND_IMMEDIATE; }
                | SrcIndirectOperand_1
                  { $$ = $1; $$.type = OPERAND_INDIRECT; }
                | SrcIndirectOperand
                  { $$ = $1; $$.type = OPERAND_INDIRECT; }
                | SrcGeneralOperand
                  { $$ = $1; $$.type = OPERAND_GENERAL; }
                | SrcGeneralOperand_1
                  { $$ = $1; $$.type = OPERAND_GENERAL; }

VecSrcOperand_G_IMM : SrcImmOperand
                  { $$ = $1; $$.type = OPERAND_IMMEDIATE; }
                | SrcGeneralOperand
                  { $$ = $1; $$.type = OPERAND_GENERAL; }
                | SrcGeneralOperand_1
                  { $$ = $1; $$.type = OPERAND_GENERAL; }
                | AddrOfOperand
                  { $$ = $1; $$.type = OPERAND_ADDRESSOF; };

VecSrcOperand_A_G : SrcGeneralOperand
                  { $$ = $1; $$.type = OPERAND_GENERAL; }
                  | SrcGeneralOperand_1
                  { $$ = $1; $$.type = OPERAND_GENERAL; }
                  | SrcAddrOperand
                  { $$ = $1; $$.type = OPERAND_ADDRESS; }
                  | AddrOfOperand
                  { $$ = $1; $$.type = OPERAND_ADDRESSOF; };

VecSrcOpndSimple :   VAR TwoDimOffset
                   {
                     // Simple Vector operand with no Modifier that has an
                     // implicit src region = <1,1,0>
                     TRACE("\n** VecSrcOpndSimple general operand");
                     $$.type = OPERAND_GENERAL;
                     $$.cisa_gen_opnd = pCisaBuilder->CISA_create_gen_src_operand($1, 1, 1, 0, $2.row, $2.elem, MODIFIER_NONE, CISAlineno);
                   };

         //   1     2        3     4    5
VMEOpndIME : LPAREN NUMBER COMMA NUMBER RPAREN
            {
                $$.streamMode = (unsigned char)$2;
                $$.searchCtrl = (unsigned char)$4;
            };
         //   1            2                3             4             5           6             7
VMEOpndFBR : LPAREN VecSrcOperand_G_I_IMM COMMA VecSrcOperand_G_I_IMM COMMA VecSrcOperand_G_I_IMM RPAREN
            {
                //$$.fbrMbMode = $2.opnd;
                //$$.fbrSubMbShape = $4.opnd;
                //$$.fbrSubPredMode = $6.opnd;

                $$.cisa_fbrMbMode_opnd = $2.cisa_gen_opnd;
                $$.cisa_fbrSubMbShape_opnd = $4.cisa_gen_opnd;
                $$.cisa_fbrSubPredMode_opnd = $6.cisa_gen_opnd;
            };

SrcStateOperand : {
                    //$$.dcl = NULL;
                    $$.type = S_OPND_ERROR;
                 }
               | VAR LPAREN NUMBER RPAREN
                {
                    $$.offset = (unsigned char)$3;
                    $$.cisa_gen_opnd = pCisaBuilder->CISA_create_state_operand($1, (unsigned char)$3, CISAlineno, false);
                }

DstStateOperand : {
                    //$$.dcl = NULL;
                    $$.type = S_OPND_ERROR;
                 }
               | VAR LPAREN NUMBER RPAREN
                {
                    $$.offset = (unsigned char)$3;
                    $$.cisa_gen_opnd = pCisaBuilder->CISA_create_state_operand($1, (unsigned char)$3, CISAlineno, true);
                }
/* ------------ Operands ------------ */

/*  ------  Dst operands ----- */
               //  1
DstAddrOperand : AddrVar
               {
                   $$.cisa_gen_opnd = pCisaBuilder->CISA_set_address_operand($1.cisa_decl, $1.elem, $1.row, true);
               };

RawOperand : /* empty */ {
                            //$$.dcl=NULL;
                            $$.offset = 0;
                            $$.cisa_gen_opnd = NULL;
                         }
              | VAR DOT NUMBER
               {
                  TRACE("\n** Raw operand");
                  /*
                        Handles a case like in dword_write where
                        a src is NULL "V0"
                  */
                  if(strcmp($1, "V0")==0)
                  {
                    //$$.dcl = NULL;
                    $$.offset = 0;
                  }else
                  {
                    //variable_declaration_and_type_check($1, GENERAL_VAR);

                    //$$.dcl = pBuilder->dclpool.lookupDeclare($1);
                    $$.offset = (unsigned short)$3;
                  }

                  $$.cisa_gen_opnd = pCisaBuilder->CISA_create_RAW_operand($1, (unsigned short)$3, CISAlineno);
               };
               | NULL_VAR DOT NUMBER
               {
                    //$$.dcl = NULL;
                    $$.offset = 0;
                    $$.cisa_gen_opnd = pCisaBuilder->CISA_create_RAW_NULL_operand(CISAlineno);
               }

RawOperandArray : /* empty */
               {
                    $$ = 0;
               };
               | RawOperandArray RawOperand
               {
                    rawOperandArray[$1++] = (VISA_RawOpnd*) $2.cisa_gen_opnd;
                    $$ = $1;
               }

                  //   1         2        3
DstGeneralOperand :  VAR TwoDimOffset DstRegion
                  {
                      TRACE("\n** Dest general operand");

                      //VISA_Type data_type = variable_declaration_and_type_check($1, GENERAL_VAR);

                      $$.cisa_gen_opnd = pCisaBuilder->CISA_dst_general_operand($1, $2.row, $2.elem, (unsigned short)$3, CISAlineno);
                      if ($$.cisa_gen_opnd == nullptr)
                          YYABORT; // error is already reported
                  };

                    //   1           2           3
DstIndirectOperand: IndirectVar IndirectRegion DataType
                  {
                      $$.cisa_gen_opnd = pCisaBuilder->CISA_create_indirect_dst($1.cisa_decl, MODIFIER_NONE, $1.row, $1.elem, $1.immOff, $2.h_stride, $3);
                      if ($$.cisa_gen_opnd == nullptr)
                          YYABORT; // error is already reported
                  };


/*  ------  Src Operands -------------- */
               //1         2
AddrOfOperand : AMP AddressableVar
             {
                 $$.cisa_gen_opnd = pCisaBuilder->CISA_set_address_expression($2.cisa_decl, 0);
             }
            | AMP AddressableVar MINUS Exp
             {
                 $$.cisa_gen_opnd = pCisaBuilder->CISA_set_address_expression($2.cisa_decl, (-1) * (short)$4);
             }
            | AMP AddressableVar PLUS Exp
             {
                 $$.cisa_gen_opnd = pCisaBuilder->CISA_set_address_expression($2.cisa_decl, (short)$4);
             };

               //  1
SrcAddrOperand : AddrVar
               {
                  $$.cisa_gen_opnd = pCisaBuilder->CISA_set_address_operand($1.cisa_decl, $1.elem, $1.row, false);
                  if ($$.cisa_gen_opnd == nullptr)
                      YYABORT; // error is already reported
               };

                  //   1          2         3
SrcGeneralOperand :  VAR TwoDimOffset SrcRegion
                  {
                      //$$.opnd = pBuilder->CISA_src_general_operand($1, $3.rgn, Mod_src_undef, $2.row, $2.elem, CISAlineno);
                      $$.cisa_gen_opnd = pCisaBuilder->CISA_create_gen_src_operand($1, $3.v_stride, $3.width, $3.h_stride, $2.row, $2.elem, MODIFIER_NONE, CISAlineno);
                      if ($$.cisa_gen_opnd == nullptr)
                          YYABORT; // error is already reported
                  };

                    //   1          2         3          4
SrcGeneralOperand_1 : SrcModifier VAR TwoDimOffset SrcRegion
                  {
                      //$$.opnd = pBuilder->CISA_src_general_operand($2, $4.rgn, $1.srcMod, $3.row, $3.elem, CISAlineno);
                      $$.cisa_gen_opnd = pCisaBuilder->CISA_create_gen_src_operand($2, $4.v_stride, $4.width, $4.h_stride, $3.row, $3.elem, $1.mod, CISAlineno);
                      if ($$.cisa_gen_opnd == nullptr)
                          YYABORT; // error is already reported
                  };

SrcImmOperand: Imm
              | DFImm
              | FpImm
              | HFImm;

                    //   1           2           3
SrcIndirectOperand: IndirectVar IndirectRegion DataType
                  {
                      //G4_SrcRegRegion src(Mod_src_undef, $1.acc, $1.opnd, $1.row, $1.elem, $2.rgn, GetGenTypeFromVISAType($3), "");
                      //src.setImmAddrOff($1.immOff);
                      //$$.opnd = pBuilder->createSrcRegRegion(src);
                      $$.cisa_gen_opnd = pCisaBuilder->CISA_create_indirect($1.cisa_decl, MODIFIER_NONE, $1.row, $1.elem, $1.immOff, $2.v_stride, $2.width, $2.h_stride, $3);
                      if ($$.cisa_gen_opnd == nullptr)
                          YYABORT; // error is already reported
                  };

                    //   1           2           3            4
SrcIndirectOperand_1: SrcModifier IndirectVar IndirectRegion DataType
                  {
                      //G4_SrcRegRegion src($1.srcMod, $2.acc, $2.opnd, $2.row, $2.elem, $3.rgn, GetGenTypeFromVISAType($4), "");
                      //src.setImmAddrOff($2.immOff);
                      //$$.opnd = pBuilder->createSrcRegRegion(src);
                      $$.cisa_gen_opnd = pCisaBuilder->CISA_create_indirect($2.cisa_decl, $1.mod, $2.row, $2.elem, $2.immOff, $3.v_stride, $3.width, $3.h_stride, $4);
                      if ($$.cisa_gen_opnd == nullptr)
                          YYABORT; // error is already reported
                  };

/* -------- regions ----------- */
DstRegion :  LANGLE NUMBER RANGLE    /* <HorzStride> */
             {
                 MUST_HOLD(($2 == 0 || $2 == 1 || $2 == 2 || $2 == 4),
                         "HorzStride must be 0, 1, 2, or 4");
                 $$ = $2;
             };

SrcRegion : /* empty */
           {
                $$.v_stride = 0;
                $$.width = 0;
                $$.h_stride = 0;
                //$$.rgn = NULL;
            }
          | LANGLE Exp SEMI Exp COMMA Exp RANGLE   /* <VertStride;Width,HorzStride> */
           {
               MUST_HOLD(($2 == 0 || $2 == 1 || $2 == 2 || $2 == 4 || $2 == 8 || $2 == 16 || $2 == 32),
                         "VertStride must be 0, 1, 2, 4, 8, 16, or 32");
               MUST_HOLD(($4 == 0 || $4 == 1 || $4 == 2 || $4 == 4 || $4 == 8 || $4 == 16),
                         "Width must be 0, 1, 2, 4, 8 or 16");
               MUST_HOLD(($6 == 0 || $6 == 1 || $6 == 2 || $6 == 4),
                         "HorzStride must be 0, 1, 2, or 4");
               $$.v_stride = (unsigned)$2;
               $$.width = (unsigned)$4;
               $$.h_stride = (unsigned)$6;
               //$$.rgn = pBuilder->rgnpool.createRegion($2, $4, $6);
           };

IndirectRegion : Region    {$$ = $1;}
               | RegionWH  {$$ = $1;}
               | RegionV   {$$ = $1;};

Region :   /* empty */
           {
                $$.v_stride = -1;
                $$.width = -1;
                $$.h_stride = -1;
                //$$.rgn = NULL;
            }
       | LANGLE Exp SEMI Exp COMMA Exp RANGLE   /* <VertStride;Width,HorzStride> */
           {
               MUST_HOLD(($2 == 0 || $2 == 1 || $2 == 2 || $2 == 4 || $2 == 8 || $2 == 16 || $2 == 32),
                         "VertStride must be 0, 1, 2, 4, 8, 16, or 32");
               MUST_HOLD(($4 == 0 || $4 == 1 || $4 == 2 || $4 == 4 || $4 == 8 || $4 == 16),
                         "Width must be 0, 1, 2, 4, 8 or 16");
               MUST_HOLD(($6 == 0 || $6 == 1 || $6 == 2 || $6 == 4),
                         "HorzStride must be 0, 1, 2, or 4");
               $$.v_stride = (unsigned int)$2;
               $$.width = (unsigned)$4;
               $$.h_stride = (unsigned)$6;
               //$$.rgn = pBuilder->rgnpool.createRegion($2, $4, $6);
           };

RegionWH : LANGLE Exp COMMA Exp RANGLE   /* <Width,HorzStride> */
           {
               MUST_HOLD(($2 == 0 || $2 == 1 || $2 == 2 || $2 == 4 || $2 == 8 || $2 == 16),
                         "Width must be 0, 1, 2, 4, 8 or 16");
               MUST_HOLD(($4 == 0 || $4 == 1 || $4 == 2 || $4 == 4),
                         "HorzStride must be 0, 1, 2, or 4");
               $$.v_stride = -1;
               $$.width = (unsigned)$2;
               $$.h_stride = (unsigned)$4;
               //$$.rgn = pBuilder->rgnpool.createRegion(UNDEFINED_SHORT, $2, $4);
           };

RegionV : LANGLE Exp RANGLE   /* <HorzStride> */
         {
             MUST_HOLD(($2 == 0 || $2 == 1 || $2 == 2 || $2 == 4),
                         "HorzStride must be 0, 1, 2, or 4");

             $$.v_stride = -1;
             $$.width = -1;
             $$.h_stride = (unsigned)$2;
             //$$.rgn = pBuilder->rgnpool.createRegion(UNDEFINED_SHORT, UNDEFINED_SHORT, (uint16_t)$2);
         };



IndirectVar : IND_LBRACK AddrParam RBRACK
             {
                 TRACE(" The variable of the indirect operand \n");
                 $$ = $2;
                 //$$.acc = IndirGRF;
             };

AddrParam : AddrVar ImmAddrOffset
             {
                 //$$.opnd = $1.opnd;
                 $$.cisa_decl = $1.cisa_decl;
                 $$.row = $1.row;
                 $$.elem = $1.elem;
                 $$.immOff = (int)$2;
             };

ImmAddrOffset :   /* empty */
                {$$ = 0;}   /* default to 0 */
              | COMMA Exp     /* need to chech whether the number is between -512 ... 511 */
                {
                    MUST_HOLD(($2 <= 1023 && $2 >= -1024), "imm addr offset must be -1024 .. 1023");
                    $$ = $2;
                };

TwoDimOffset : LPAREN Exp COMMA Exp RPAREN
          {
              $$.row = (int)$2;
              $$.elem = (int)$4;
          };

AddrVar :  VAR
          {
              TRACE("\n** Address operand");
              $$.cisa_decl = pCisaBuilder->CISA_find_decl($1);
              if (!$$.cisa_decl)
                  PARSE_ERROR("unbound variable");
              $$.row = 0;
              $$.elem = 0;
          }
         |  VAR LPAREN Exp RPAREN
          {
              TRACE("\n** Address operand");

              $$.cisa_decl = pCisaBuilder->CISA_find_decl($1);
              if (!$$.cisa_decl)
                  PARSE_ERROR("unbound variable");
              $$.row = 1;
              $$.elem = (int)$3;
          }
          // 1   2   3   4   5    6     7
         |  VAR LPAREN Exp RPAREN LANGLE NUMBER RANGLE
          {
              TRACE("\n** Address operand");

              $$.cisa_decl = pCisaBuilder->CISA_find_decl($1);
              if (!$$.cisa_decl)
                  PARSE_ERROR("unbound variable");
              $$.row = (int)$6;
              $$.elem = (int)$3;
          }
         |  VAR LPAREN Exp COMMA Exp RPAREN
          {
              TRACE("\n** Address operand");
              $$.cisa_decl = pCisaBuilder->CISA_find_decl($1);
              if (!$$.cisa_decl)
                  PARSE_ERROR("unbound variable");
              $$.row = (int)$3;
              $$.elem = (int)$5;
          };

AddressableVar : VAR {
              // Both GENERAL_VAR and SURFACE_VAR are addressable
              //$$.opnd = pBuilder->getRegVar($1);
              $$.cisa_decl = pCisaBuilder->CISA_find_decl($1);
              if (!$$.cisa_decl)
                  PARSE_ERROR("unbound variable");
              $$.row = 0;
              $$.elem = 0;
          };

PlaneID :   /* empty */
            {$$ = 0;}
          | NUMBER
            {
                MUST_HOLD(($1 < 0xF), "PlaneID must less than 0xF");
                $$ = $1;
            }
          ;

/* -----------register size ------------------------------*/
SIMDMode : { $$ = 0; }
         | LPAREN NUMBER RPAREN
           {
               MUST_HOLD(($2 == 8 || $2 == 16),
                         "SIMD mode can only be 8 or 16");
               $$ = $2;
           };


RTWRITE_MODE : { $$ = 0; }
            | RTWRITE_OPTION
            {
                $$ = $1;
            };

/* ----------- Execution size -------------- */

ElemNum : DOT NUMBER
           {
               TRACE("\n** Element Number");
               $$ = $2;
           };

ExecSize :   /* empty */
           {
                $$.exec_size = UNDEFINED_EXEC_SIZE;
                $$.emask = vISA_EMASK_M1;
           }
         | LPAREN ExecSizeInt RPAREN
           {
               TRACE("\n** Execution Size ");
               $$.emask = vISA_EMASK_M1;
               $$.exec_size = (int)$2;
           };
         | LPAREN VAR COMMA ExecSizeInt RPAREN
           {
               /* */
               TRACE("\n** Execution Size With Offset ");
               if (!ParseEMask(pCisaBuilder, $2, $$.emask)) {
                    PARSE_ERROR("invalid execution offset info");
               }
               $$.exec_size = (int)$4;
           };

ExecSizeInt : NUMBER {
           if ($1 != 1 && $1 != 2 && $1 != 4 && $1 != 8 && $1 != 16 && $1 != 32) {
                PARSE_ERROR("invalid execution size");
           }
           $$ = (unsigned short)$1;
        }

/* ------ imm operand ----------------------------------- */
Imm : Exp DataType
      {
#if 0
      $$ = pBuilder->createImmWithLowerType($1, $2 );
#else
      //for CISA binary builder need original type. Don't want to modify all the G4_Imm
      //data structures now.
      //$$.opnd = pBuilder->createImm($1, GetGenTypeFromVISAType($2) );
      $$.cisa_gen_opnd = pCisaBuilder->CISA_create_immed($1, $2, CISAlineno);
#endif
      };

Exp : AbstractNum   { $$ = $1; }
    | Exp PLUS  Exp { $$ = $1 + $3; }
    | Exp MINUS Exp { $$ = $1 - $3; }
    | Exp TIMES Exp { $$ = $1 * $3; }
    | Exp SLASH Exp {
            if ($3 == 0)
                CISAerror(pCisaBuilder, "division by 0");
            $$ = $1 / $3;
        }
    | MINUS Exp %prec NEG  { $$ = -$2; }
    | LPAREN Exp RPAREN    { $$ = $2; };

AbstractNum : NUMBER
              {$$ = $1;}
            | HEX_NUMBER
              {$$ = $1;}
            ;

DoubleFloat :   DOUBLEFLOAT
                {   $$ = $1; }
               | NUMBER DFTYPE
               {
                    /* "1:df" means 5e-324 */
                    int64_t number = $1;
                    double *fp = (double *)&number;
                    $$ = *fp;
               }
               | HEX_NUMBER DFTYPE       /* to support 0x7ff0000000000000:df */
               {
                    int64_t number = $1;
                    double *fp = (double *)&number;
                    $$ = *fp;
               };

FloatPoint :    NUMBER DOT NUMBER
                {   char floatstring[256];
                    sprintf_s(floatstring, sizeof(floatstring), "%" PRId64 ".%" PRId64, $1, $3);
                    $$ = atof(floatstring);
                }
              | FLOATINGPOINT
                {   $$ = $1; }
               | NUMBER DOT
                {
                    $$ = $1*1.0f;
                }
               | NUMBER FTYPE
               {
                    /* "1:f" means 1.4e-45 */
                    int number = (int)$1;
                    float *fp = (float *)&number;
                    $$ = *fp;
               }
               | HEX_NUMBER FTYPE       /* to support 0x3f800000:f */
               {
                    int number = (int)$1;
                    float *fp = (float *)&number;
                    $$ = *fp;
               };

DFImm : DoubleFloat
       {
         //$$.opnd = pBuilder->createDFImm($1);
         $$.cisa_gen_opnd = pCisaBuilder->CISA_create_float_immed($1, ISA_TYPE_DF, CISAlineno);
       }
       | MINUS DoubleFloat
       {
         //$$.opnd = pBuilder->createDFImm($2 * (-1));
         $$.cisa_gen_opnd = pCisaBuilder->CISA_create_float_immed($2 * (-1), ISA_TYPE_DF, CISAlineno);
       }
       ;

FpImm : FloatPoint
       {
           //$$.opnd = pBuilder->createImm($1);
           $$.cisa_gen_opnd = pCisaBuilder->CISA_create_float_immed($1, ISA_TYPE_F, CISAlineno);
       }
       | MINUS FloatPoint
       {
           //$$.opnd = pBuilder->createImm($2 * (-1));
           $$.cisa_gen_opnd = pCisaBuilder->CISA_create_float_immed($2 * (-1), ISA_TYPE_F, CISAlineno);
       } ;

HFImm : HEX_NUMBER HFTYPE
       {
           $$.cisa_gen_opnd = pCisaBuilder->CISA_create_immed((unsigned short)$1, ISA_TYPE_HF, CISAlineno);
      }

/* ------ data types ------------------------------------ */
DataType: ITYPE {$$ = $1;}
        | DFTYPE {$$ = $1;}
        | FTYPE {$$ = $1;}
        | VTYPE {$$ = $1;};
        | HFTYPE {$$ = $1;};

/* ----- Others ----------------------------------------- */
CommentLine : COMMENT_LINE
            {
                //pBuilder->addComment($1);
            };


PIXEL_NULL_MASK_ENABLE: { $$ = false; }
         | PIXEL_NULL_MASK /* .pixel_null_mask */
         {
             $$ = true;
             TRACE("** PIXEL_NULL_MASK ");
         };

CPS_ENABLE: { $$ = false; }
         | CPS /* .cps */
         {
             $$ = true;
             TRACE("** CPS LOD compensation a");
         };

NON_UNIFORM_SAMPLER_ENABLE: { $$ = false; }
         | NON_UNIFORM_SAMPLER /* .divS */
         {
             $$ = true;
             TRACE("** non-uniform sampler state");
         };

IS_ATOMIC16: { $$ = false ; }
         | DOT NUMBER /* .16 */
         {
             MUST_HOLD(($2 == 16), "Only supports 16.");
             $$ = true;
             TRACE("** atomic 16");
         };

ATOMIC_BITWIDTH: { $$ = 32; } |DOT NUMBER
        {
            MUST_HOLD(($2 == 16 || $2 == 64 ), "Only supports 16/64.");
            TRACE("\n** atomic NUMBER");
            $$ = (unsigned short)$2;
        };

%%

void CISAerror(CISA_IR_Builder* pCisaBuilder, char const *s)
{
    /*
    int yytype = YYTRANSLATE (yychar);
    if (strcmp(yytname[yytype], "NUMBER") == 0)
        fprintf (stderr, "\nLine %d: %s, number: %" PRId64 "\n", CISAlineno,  s, yylval.number);
     else if (strcmp(yytname[yytype], "VAR") == 0)
        fprintf (stderr, "\nLine %d: %s, symbol: %s\n", CISAlineno,  s, yylval.string);
     else
        fprintf (stderr, "\nLine %d: %s\n", CISAlineno,  s);
        */

    pCisaBuilder->RecordParseError(CISAlineno, s);
}

static bool ParseAlign(CISA_IR_Builder* pCisaBuilder, const char *sym, VISA_Align &value)
{
    if (strcmp(sym, "byte") == 0) {
        value = ALIGN_BYTE;
    } else if (strcmp(sym, "word") == 0) {
        value = ALIGN_WORD;
    } else if (strcmp(sym, "dword") == 0) {
        value = ALIGN_DWORD;
    } else if (strcmp(sym, "qword") == 0) {
        value = ALIGN_QWORD;
    } else if (strcmp(sym, "oword") == 0) {
        value = ALIGN_OWORD;
    } else if (strcmp(sym, "GRF") == 0) {
        value = ALIGN_GRF;
    } else if (strcmp(sym, "GRFx2") == 0) {
        value = ALIGN_2_GRF;
    } else {
        value = ALIGN_UNDEF;
        return false;
    }
    return true;
}

static bool ParseEMask(
    CISA_IR_Builder* pCisaBuilder,
    const char* sym,
    VISA_EMask_Ctrl &emask)
{
    if (strcmp(sym, "NoMask") == 0) {
        emask = vISA_EMASK_M1_NM;
        return true;
    }
    for (int i = 0; i < vISA_NUM_EMASK +1; i++)
    {
        if (!strcmp(emask_str[i], sym))
        {
            emask = (VISA_EMask_Ctrl)i;
            return true;
        }
    }

    emask = vISA_NUM_EMASK;
    return false;
}

