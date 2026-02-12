/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

%{
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#ifdef _MSC_VER
// To disable warning for duplicate macros definitions
// such as INT8_MAX in lex.CISA.c with one from stdint.h
#pragma warning(disable: 4005)
#endif

#include "visa_igc_common_header.h"
#include "Common_ISA_framework.h"
#include "VISAKernel.h"
#include "BuildCISAIR.h"

#ifdef _MSC_VER
// Re-enable macros redefinition check.
#pragma warning(default: 4005)
#endif

#include "CISA.tab.hpp"

#define MAX_STRLIT_SIZE 4096
#define TRACE(S) \
    do { \
      if (CISAout && pBuilder->debugParse()) \
          fprintf(CISAout, "line %d: %-32s \"%s\"\n", CISAlineno, S, yytext); \
    } while (0)

static VISA_Type               str2type(const char *str, int str_len);
static VISA_Cond_Mod           str2cond(const char *str);
static VISAAtomicOps           str2atomic_opcode(const char *op_str);
static ISA_Opcode              str2opcode(const char* op_str);
static GenPrecision            str2Precision(const char *str, int str_len);
static LSC_OP                  str2lscop(const char *str);
static LSC_DATA_SIZE           decodeDataSizePrefix(const char *str,int *off);
static LSC_DATA_ELEMS          decodeDataElems(const char *str,int *off);
static VISASampler3DSubOpCode  str2SampleOpcode(const char* str, TARGET_PLATFORM platform);
static int64_t                 hexToInt(const char *hex_str, int str_len);
static MEDIA_LD_mod            mediaMode(const char* str);
static OutputFormatControl     avs_control(const char* str);
static AVSExecMode             avsExecMode(const char* str);
static unsigned char           FENCEOptions(const char* str);
static CHANNEL_OUTPUT_FORMAT   Get_Channel_Output(const char* str);
static void                    appendStringLiteralChar(char c, char *buf, size_t *len);
static LFSR_FC                 str2lfsrfc(const char* str);
static DNSCL_CONVERT_TYPE str2DnsclConvertType(const char *str, int *off);
static DNSCL_MODE str2DnsclMode(const char *str, int *off);
static DNSCL_RND_MODE str2DnsclRoundMode(const char *str, int *off);

static bool buildOptionPresent = false;
void resetGlobalVariables() {
    buildOptionPresent = false;
}
%}

%option yylineno
%option nounput

%x   eat_comment
%x   string_literal

%%

\n {
      return NEWLINE;
   }

"//Build option:"[ \t]*\"([^\"\n]*)\"[ \t]* {
    if(pBuilder->getOptions()->getOption(vISA_ParseBuildOptions)) {
        if(buildOptionPresent)
            YY_FATAL_ERROR("more then one instance of build options");
        buildOptionPresent = true;
        TRACE("** BUILD_OPTION\n");
        CISAlval.string = strdup(yytext);
        return BUILD_OPTION_LINE;
    }
}

[ \t] { /*drop non-newline spaces*/}
"//"[^\n]* {
        // drop comments
        // TRACE("** COMMENT\n");
        // CISAlval.string = strdup(yytext);
        // return COMMENT_LINE;
    }

"/*"  BEGIN(eat_comment);
<eat_comment>[^*]* /* eat anything that is not a * */
<eat_comment>"*"+[^*/]*  /* eat up '*'s not followed by '/'s */
<eat_comment>"*"+"/"  BEGIN(INITIAL);


\" {
     CISAlval.strlit.len = 0;
     CISAlval.strlit.decoded = (char *) malloc(sizeof(char) * MAX_STRLIT_SIZE);
     if (!CISAlval.strlit.decoded)
       YY_FATAL_ERROR("unable to allocate memory to lex strlit");
     CISAlval.strlit.decoded[0] = 0;
     BEGIN(string_literal);
}
<string_literal>{
    \n                        YY_FATAL_ERROR("lexical error: newline in string literal");
    <<EOF>>                   YY_FATAL_ERROR("lexical error: unterminated string (reached EOF)");
    \\a                       {appendStringLiteralChar('\a',CISAlval.strlit.decoded,&CISAlval.strlit.len);}
    \\b                       {appendStringLiteralChar('\b',CISAlval.strlit.decoded,&CISAlval.strlit.len);}
    \\e                       {appendStringLiteralChar(0x1B,CISAlval.strlit.decoded,&CISAlval.strlit.len);}
    \\f                       {appendStringLiteralChar('\f',CISAlval.strlit.decoded,&CISAlval.strlit.len);}
    \\n                       {appendStringLiteralChar('\n',CISAlval.strlit.decoded,&CISAlval.strlit.len);}
    \\r                       {appendStringLiteralChar('\r',CISAlval.strlit.decoded,&CISAlval.strlit.len);}
    \\t                       {appendStringLiteralChar('\t',CISAlval.strlit.decoded,&CISAlval.strlit.len);}
    \\v                       {appendStringLiteralChar('\v',CISAlval.strlit.decoded,&CISAlval.strlit.len);}
    \\"'"                     {appendStringLiteralChar('\'',CISAlval.strlit.decoded,&CISAlval.strlit.len);}
    \\"\""                    {appendStringLiteralChar('\"',CISAlval.strlit.decoded,&CISAlval.strlit.len);}
    \\"?"                     {appendStringLiteralChar('?',CISAlval.strlit.decoded,&CISAlval.strlit.len);}
    \\\\                      {appendStringLiteralChar('\\',CISAlval.strlit.decoded,&CISAlval.strlit.len);}
    \\[0-9]{1,3} {
        int val = 0;
        for (int i = 1; i < yyleng; i++)
            val = 8*val + yytext[i] - '0';
        appendStringLiteralChar(val,CISAlval.strlit.decoded,&CISAlval.strlit.len);
    }
    \\x[0-9A-Fa-f]{1,2} {
        int val = 0;
        for (int i = 2; i < yyleng; i++) {
            int dig =
                yytext[i] >= '0' && yytext[i] <= '9' ? yytext[i] - '0' :
                yytext[i] >= 'a' && yytext[i] <= 'f' ? yytext[i] - 'a' + 10 :
                                                       yytext[i] - 'A' + 10;
            val = 16*val + dig;
        }
        appendStringLiteralChar(val,CISAlval.strlit.decoded,&CISAlval.strlit.len);
    }
    \\.                       YY_FATAL_ERROR("lexical error: illegal escape sequence");
    \"  {
          char *tmp = strdup(CISAlval.strlit.decoded);
          free(CISAlval.strlit.decoded);
          CISAlval.string = tmp;
          BEGIN(INITIAL);
          return STRING_LIT;
    }
    .                         {
    /* important: this must succeed the exit rule above (\"); lex prefers the first match */
        appendStringLiteralChar(yytext[0],CISAlval.strlit.decoded,&CISAlval.strlit.len);
    }
}


"*" {TRACE("** TIMES");  return TIMES;}
"+" {TRACE("** PLUS");   return PLUS;}
"-" {TRACE("** MINUS");  return MINUS;}
"=" {TRACE("** EQUALS"); return EQUALS;}
"&" {TRACE("** AMP");    return AMP;}
"^" {TRACE("** CIRC");   return CIRC;}
"|" {TRACE("** PIPE");   return PIPE;}
"~" {TRACE("** TILDE");  return TILDE;}
"!" {TRACE("** BANG");   return BANG;}

"<=" {TRACE("** LEQ"); return LEQ;}
">=" {TRACE("** GEQ"); return GEQ;}
"==" {TRACE("** EQ");  return  EQ;}
"!=" {TRACE("** NEQ"); return NEQ;}


"." {TRACE("** DOT");     return DOT;}
"," {TRACE("** COMMA");   return COMMA;}
";" {TRACE("** SEMI");    return SEMI;}
":" {TRACE("** COLON");   return COLON;}
"/" {TRACE("** SLASH");    return SLASH;}
"%" {TRACE("** PERCENT");  return PERCENT;}
"?" {TRACE("** QUESTION"); return QUESTION;}

"(" {TRACE("** LPAREN"); return LPAREN;}
")" {TRACE("** RPAREN"); return RPAREN;}
"{" {TRACE("** LBRACE"); return LBRACE;}
"}" {TRACE("** RBRACE"); return RBRACE;}
"<" {TRACE("** LANGLE"); return LANGLE;}
">" {TRACE("** RANGLE"); return RANGLE;}
"<<"  {TRACE("** SHL");  return SHL;}
">>"  {TRACE("** SHRS"); return SHRS;}
">>>" {TRACE("** SHRZ"); return SHRZ;}

"r[" {TRACE("** IND_LBRACK"); return IND_LBRACK;}
"["  {TRACE("** LBRACK"); return LBRACK;}
"]"  {TRACE("** RBRACK"); return RBRACK;}


".version"          {TRACE("** VERSION"); return DIRECTIVE_VERSION;}
".decl"             {TRACE("** DECLARE"); return DIRECTIVE_DECL;}
".funcdecl"         {TRACE("** FUNCTION DECLARE"); return DIRECTIVE_FUNCDECL;}
".kernel_attr"      {TRACE("** KERNEL ATTR"); return DIRECTIVE_KERNEL_ATTR;}
".input"            {TRACE("** INPUT"); return DIRECTIVE_INPUT;}
"."implicit[a-zA-Z0-9_\-$@?]* {
        TRACE("**  DIRECTIVE_IMPLICIT");
        char* yytext_dup = strdup(yytext);
        yytext_dup[yyleng] = '\0';
        CISAlval.string = yytext_dup;
        return DIRECTIVE_IMPLICIT;
    }
".function"         {TRACE("** FUNCTION"); return DIRECTIVE_FUNC;}
".global_function"  {TRACE("** GLOBAL FUNCTION"); return DIRECTIVE_GLOBAL_FUNC;}
".kernel"           {TRACE("** KERNEL NAME DIRECTIVE"); return DIRECTIVE_KERNEL;}

num_elts[ ]*= {TRACE("** NUM_ELTS_EQ"); return NUM_ELTS_EQ;}
align[ ]*=    {TRACE("** ALIGN_EQ");    return ALIGN_EQ;}
offset[ ]*=   {TRACE("** OFFSET_EQ");   return OFFSET_EQ;}
size[ ]*=     {TRACE("** SIZE_EQ");     return SIZE_EQ;}
alias[ ]*=    {TRACE("** ALIAS_EQ");    return ALIAS_EQ;}
attrs[ ]*=     {TRACE("** ATTR_EQ");     return ATTR_EQ;}
v_name[ ]*=   {TRACE("** V_NAME_EQ");   return V_NAME_EQ;}

v_type[ ]*=[ ]*G {TRACE("** V_TYPE_EQ_G"); return V_TYPE_EQ_G;}
v_type[ ]*=[ ]*A {TRACE("** V_TYPE_EQ_A"); return V_TYPE_EQ_A;}
v_type[ ]*=[ ]*P {TRACE("** V_TYPE_EQ_P"); return V_TYPE_EQ_P;}
v_type[ ]*=[ ]*S {TRACE("** V_TYPE_EQ_S"); return V_TYPE_EQ_S;}
v_type[ ]*=[ ]*T {TRACE("** V_TYPE_EQ_T"); return V_TYPE_EQ_T;}


"."(add|sub|inc|dec|min|max|xchg|cmpxchg|and|or|xor|minsint|maxsint|fmax|fmin|fcmpwr)   {
        TRACE("** Atomic Operations");
        CISAlval.atomic_op = str2atomic_opcode(yytext + 1);
        return ATOMIC_SUB_OP;
    }

not|cbit|fbh|fbl|bfrev {
        TRACE("** Unary Logic INST");
        CISAlval.opcode = str2opcode(yytext);
        return UNARY_LOGIC_OP;
    }

bfe {
      TRACE("** Ternary Logic INST");
      CISAlval.opcode = str2opcode(yytext);
      return TERNARY_LOGIC_OP;
  }

bfi {
      TRACE("** Quaternary Logic INST");
      CISAlval.opcode = str2opcode(yytext);
      return QUATERNARY_LOGIC_OP;
}


inv|log|exp|sqrt|rsqrt|sin|cos|sqrtm {
         TRACE("** 2 operand math INST");
         CISAlval.opcode = str2opcode(yytext);
         return MATH2_OP;
    }

tanh|sigm {
         TRACE("** 2 operand math INST");
         CISAlval.opcode = str2opcode(yytext);
         return MATH2_OP;
}
div|mod|pow|divm {
        TRACE("** 3 operand math INST");
        CISAlval.opcode = str2opcode(yytext);
        return MATH3_OP;
    }

frc|lzd|rndd|rndu|rnde|rndz {
        TRACE("** ARITH2_OP");
        CISAlval.opcode = str2opcode(yytext);
        return ARITH2_OP;
    }

add|avg|dp2|dp3|dp4|dph|line|mul|pow|mulh|sad2|plane {
        TRACE("** ARITH3_OP");
        CISAlval.opcode = str2opcode(yytext);
        return ARITH3_OP;
    }

mad|lrp|sad2add|madw {
        TRACE("** ARITH4_OP");
        CISAlval.opcode = str2opcode(yytext);
        return ARITH4_OP;
    }

and|or|xor|shl|shr|asr {
        TRACE("** BINARY_LOGIC_OP");
        CISAlval.opcode = str2opcode(yytext);
        return BINARY_LOGIC_OP;
    }

rol|ror {
        TRACE("** BINARY_LOGIC_OP");
        CISAlval.opcode = str2opcode(yytext);
        return BINARY_LOGIC_OP;
    }

breakpoint {
        TRACE("** BREAKPOINT_OP");
        CISAlval.opcode = str2opcode(yytext);
        return BREAKPOINT_OP;
}

shfl_idx4 {
        TRACE("** FP4/INT4 to FP8/FP16 conversion INST");
        CISAlval.opcode = str2opcode(yytext);
        return SHFL_IDX4_OP;
}

lfsr"."(b32|b16v2|b8v4) {
        TRACE("** LFSR");
        CISAlval.lfsr_fctrl = str2lfsrfc(yytext + 5);
        return LFSR_SUB_OP;
}

bdpas"."bf"."bf"."8"."8 {
        TRACE("** BDPAS");
        CISAlval.opcode = ISA_BDPAS;
        CISAlval.dpas_info.src1Precision = GenPrecision::BF16;
        CISAlval.dpas_info.src2Precision = GenPrecision::BF16;
        CISAlval.dpas_info.depth = 8;
        CISAlval.dpas_info.count = 8;
        return BDPAS_OP;
}
bdpas"."hf"."hf"."8"."8 {
        TRACE("** BDPAS");
        CISAlval.opcode = ISA_BDPAS;
        CISAlval.dpas_info.src1Precision = GenPrecision::FP16;
        CISAlval.dpas_info.src2Precision = GenPrecision::FP16;
        CISAlval.dpas_info.depth = 8;
        CISAlval.dpas_info.count = 8;
        return BDPAS_OP;
}
bdpas"."[bh]f8"."[bh]f8"."8"."8 {
        TRACE("** BDPAS");
        CISAlval.opcode = ISA_BDPAS;
        CISAlval.dpas_info.src1Precision = str2Precision(yytext + 6, 3);
        CISAlval.dpas_info.src2Precision = str2Precision(yytext + 10, 3);
        CISAlval.dpas_info.depth = 8;
        CISAlval.dpas_info.count = 8;
        return BDPAS_OP;
}
bdpas"."e2m1"."e2m1"."8"."8 {
        TRACE("** BDPAS");
        CISAlval.opcode = ISA_BDPAS;
        CISAlval.dpas_info.src1Precision = str2Precision(yytext + 6, 4);
        CISAlval.dpas_info.src2Precision = str2Precision(yytext + 11, 4);
        CISAlval.dpas_info.depth = 8;
        CISAlval.dpas_info.count = 8;
        return BDPAS_OP;
}
dpas"."[eE]2[mM]1"."[eE]2[mM]1"."(1|2|4|8)"."[12345678] {
        TRACE("** DPAS");
        CISAlval.opcode = ISA_DPAS;
        CISAlval.dpas_info.src1Precision = str2Precision(yytext + 5, 4);
        CISAlval.dpas_info.src2Precision = str2Precision(yytext + 10, 4);
        CISAlval.dpas_info.depth = (yytext[15] - '0');
        CISAlval.dpas_info.count = (yytext[17] - '0');
        return DPAS_OP;
}

dnscl"."(bftoe2m1|bftoint4|hftoe2m1|hftoint4)"."mode(0|1|2|3)"."(srnd|rne) {
        TRACE("** DNSCL");
        int offset = 6;
        CISAlval.dnscl_info.type = str2DnsclConvertType(yytext, &offset);
        offset++;
        CISAlval.dnscl_info.mode = str2DnsclMode(yytext, &offset);
        offset++;
        CISAlval.dnscl_info.rnd_mode = str2DnsclRoundMode(yytext, &offset);
        return DNSCL_OP;
}
dpas"."tf32"."tf32"."(1|2|4|8)"."[12345678] {
        TRACE("** DPAS");
        CISAlval.opcode = ISA_DPAS;
        CISAlval.dpas_info.src1Precision = GenPrecision::TF32;
        CISAlval.dpas_info.src2Precision = GenPrecision::TF32;
        CISAlval.dpas_info.depth = (yytext[15] - '0');
        CISAlval.dpas_info.count = (yytext[17] - '0');
        return DPAS_OP;
    }
dpas"."[bBhH][fF][8]"."[bBhH][fF][8]"."(1|2|4|8)"."[12345678] {
        TRACE("** DPAS");
        CISAlval.opcode = ISA_DPAS;
        CISAlval.dpas_info.src1Precision = str2Precision(yytext + 5, 3);
        CISAlval.dpas_info.src2Precision = str2Precision(yytext + 9, 3);
        CISAlval.dpas_info.depth = (yytext[13] - '0');
        CISAlval.dpas_info.count = (yytext[15] - '0');
        return DPAS_OP;
    }

dp4a {
        TRACE("** ARITH4_OP");
        CISAlval.opcode = str2opcode(yytext);
        return ARITH4_OP;
    }

dpas"."[sSuUbBHh][1248fF]"."[sSuUbBHh][1248fF]"."[1248]"."[12345678] {
        TRACE("** DPAS");
        CISAlval.opcode = ISA_DPAS;
        CISAlval.dpas_info.src1Precision = str2Precision(yytext + 5, 2);
        CISAlval.dpas_info.src2Precision = str2Precision(yytext + 8, 2);
        CISAlval.dpas_info.depth = (yytext[11] - '0');
        CISAlval.dpas_info.count = (yytext[13] - '0');
        return DPAS_OP;
    }

dpasw"."[sSuUbBhH][1248fF]"."[sSuUbBhH][1248fF]"."[1248]"."[12345678] {
        TRACE("** DPASW");
        CISAlval.opcode = ISA_DPASW;
        CISAlval.dpas_info.src1Precision = str2Precision(yytext + 6, 2);
        CISAlval.dpas_info.src2Precision = str2Precision(yytext + 9, 2);
        CISAlval.dpas_info.depth = (yytext[12] - '0');
        CISAlval.dpas_info.count = (yytext[14] - '0');
        return DPAS_OP;
    }

add3 {
        TRACE("** ADD3 INST");
        CISAlval.opcode = str2opcode(yytext);
        return ARITH4_OP;
    }

add3\.o {
        TRACE("** ADD3O INST");
        CISAlval.opcode = str2opcode(yytext);
        return ARITH4_OP;
    }

bfn"."x[[:xdigit:]]+ {
        TRACE("** BFN INST");
        CISAlval.bfn_info.func_ctrl = (uint8_t)hexToInt(yytext+5, yyleng-5);
        return BFN_OP;
    }

qw_gather|qw_scatter {
        TRACE("** qword gather/scatter INST");
        CISAlval.opcode = str2opcode(yytext);
        return QW_SCATTER_OP;
    }

"."(fadd|fsub) {
        TRACE("** Float Atomic add/sub");
        CISAlval.atomic_op = str2atomic_opcode(yytext + 1);
        return ATOMIC_SUB_OP;
}

fcvt {
    TRACE("** special float (bf8, etc) conversion INST");
    CISAlval.opcode = str2opcode(yytext);
    return FCVT_OP;
}

srnd {
    TRACE("** srnd INST");
    CISAlval.opcode = str2opcode(yytext);
    return ARITH3_OP;
}

addc|subb {
        TRACE("** MATH INST");
        CISAlval.opcode = str2opcode(yytext);
        return ARITH4_OP2;
    }

invm|rsqtm {
        TRACE("** MATH MACRO INST");
        CISAlval.opcode = str2opcode(yytext);
        return ARITH5_OP2;
    }

asin|acos|atan {
        TRACE("** ANTI TRIGONOMETRIC INST");
        CISAlval.opcode = str2opcode(yytext);
        return ANTI_TRIG_OP;
    }

addr_add   {
        TRACE("** Addr add INST");
        CISAlval.opcode = str2opcode(yytext);
        return ADDR_ADD_OP;
    }

sel {
        TRACE("** Mod INST");
        CISAlval.opcode = str2opcode(yytext);
        return SEL_OP;
    }

min {
        TRACE("** MIN INST");
        CISAlval.opcode = ISA_FMINMAX;
        return MIN_OP;
    }

max {
        TRACE("** MAX INST");
        CISAlval.opcode = ISA_FMINMAX;
        return MAX_OP;
    }

mov {
        TRACE("** MOV INST");
        CISAlval.opcode = str2opcode(yytext);
        return MOV_OP;
    }

movs {
        TRACE("** MOVS INST");
        CISAlval.opcode = str2opcode(yytext);
        return MOVS_OP;
    }

setp {
        TRACE("** SETP INST");
        CISAlval.opcode = str2opcode(yytext);
        return SETP_OP;
    }

cmp {
        TRACE("** compare INST");
        CISAlval.opcode = str2opcode(yytext);
        return CMP_OP;
    }

svm_block_ld|svm_block_st|svm_scatter|svm_gather|svm_gather4scaled|svm_scatter4scaled|svm_atomic {
        TRACE("** svm INST");
        /// XXX: Piggyback svm sub-opcode as an opcode.
        if (!strcmp(yytext, "svm_gather4scaled")) {CISAlval.opcode = (ISA_Opcode)SVM_GATHER4SCALED; return SVM_GATHER4SCALED_OP;}
        if (!strcmp(yytext, "svm_scatter4scaled")) {CISAlval.opcode = (ISA_Opcode)SVM_SCATTER4SCALED; return SVM_SCATTER4SCALED_OP;}
        if (!strcmp(yytext, "svm_block_ld")) CISAlval.opcode = (ISA_Opcode)SVM_BLOCK_LD;
        if (!strcmp(yytext, "svm_block_st")) CISAlval.opcode = (ISA_Opcode)SVM_BLOCK_ST;
        if (!strcmp(yytext, "svm_scatter" )) {CISAlval.opcode = (ISA_Opcode)SVM_SCATTER; return SVM_SCATTER_OP;}
        if (!strcmp(yytext, "svm_gather"  )) {CISAlval.opcode = (ISA_Opcode)SVM_GATHER; return SVM_SCATTER_OP;}
        if (!strcmp(yytext, "svm_atomic"  )) {CISAlval.opcode = (ISA_Opcode)SVM_ATOMIC; return SVM_ATOMIC_OP;}
        return SVM_OP;
    }

lsc_load|lsc_load_quad|lsc_load_status {
        TRACE("** lsc_load* INST");
        CISAlval.lsc_subOpcode = str2lscop(yytext);
        // this is set in the parser based on the SFID
        // CISAlval.lsc_opcode = ISA_LSC_UNTYPED OR ISA_LSC_TYPED;
        return LSC_LOAD_MNEMONIC;
    }
lsc_load_quad_status {
        TRACE("** lsc_load* INST");
        CISAlval.lsc_subOpcode = str2lscop(yytext);
        // this is set in the parser based on the SFID
        // CISAlval.lsc_opcode = ISA_LSC_UNTYPED OR ISA_LSC_TYPED;
        return LSC_LOAD_MNEMONIC;
    }
lsc_load_quad_msrt {
    TRACE("** lsc_load* INST");
        CISAlval.lsc_subOpcode = str2lscop(yytext);
        // this is set in the parser based on the SFID
        // CISAlval.lsc_opcode = ISA_LSC_UNTYPED OR ISA_LSC_TYPED;
        return LSC_LOAD_MSRT_MNEMONIC;
    }
lsc_load_strided {
        TRACE("** lsc_load_strided INST");
        CISAlval.lsc_subOpcode = str2lscop(yytext);
        // this is set in the parser based on the SFID
        // CISAlval.lsc_opcode = ISA_LSC_UNTYPED OR ISA_LSC_TYPED;
        return LSC_LOAD_STRIDED_MNEMONIC;
    }
lsc_load_block2d {
        TRACE("** lsc_load_block2d INST");
        CISAlval.lsc_subOpcode = str2lscop(yytext);
        // this is set in the parser based on the SFID
        // CISAlval.lsc_opcode = ISA_LSC_UNTYPED OR ISA_LSC_TYPED;
        return LSC_LOAD_BLOCK2D_MNEMONIC;
    }

lsc_store|lsc_store_quad|lsc_store_uncompressed|lsc_ccs_update {
        TRACE("** lsc_store* INST");
        CISAlval.lsc_subOpcode = str2lscop(yytext);
        // this is set in the parser based on the SFID
        // CISAlval.lsc_opcode = ISA_LSC_UNTYPED OR ISA_LSC_TYPED;
        return LSC_STORE_MNEMONIC;
    }
lsc_store_quad_msrt {
        TRACE("** lsc_store* INST");
        CISAlval.lsc_subOpcode = str2lscop(yytext);
        // this is set in the parser based on the SFID
        // CISAlval.lsc_opcode = ISA_LSC_UNTYPED OR ISA_LSC_TYPED;
        return LSC_STORE_MSRT_MNEMONIC;
    }
lsc_store_strided {
        TRACE("** lsc_store_strided INST");
        CISAlval.lsc_subOpcode = str2lscop(yytext);
        // this is set in the parser based on the SFID
        // CISAlval.lsc_opcode = ISA_LSC_UNTYPED;
        return LSC_STORE_STRIDED_MNEMONIC;
    }
lsc_store_block2d {
        TRACE("** lsc_store_block2d INST");
        CISAlval.lsc_subOpcode = str2lscop(yytext);
        // this is set in the parser based on the SFID
        // CISAlval.lsc_opcode = ISA_LSC_UNTYPED OR ISA_LSC_TYPED;
        return LSC_STORE_BLOCK2D_MNEMONIC;
    }

lsc_atomic_iinc|lsc_atomic_idec|lsc_atomic_iadd|lsc_atomic_load|lsc_atomic_store|lsc_atomic_iadd|lsc_atomic_isub|lsc_atomic_smin|lsc_atomic_smax|lsc_atomic_umin|lsc_atomic_umax|lsc_atomic_icas|lsc_atomic_fadd|lsc_atomic_fsub|lsc_atomic_fmin|lsc_atomic_fmax|lsc_atomic_fcas|lsc_atomic_and|lsc_atomic_xor|lsc_atomic_or {
        TRACE("** lsc_atomic INST");
        CISAlval.lsc_subOpcode = str2lscop(yytext);
        // this is set in the parser based on the SFID
        // CISAlval.lsc_opcode = ISA_LSC_UNTYPED;
        return LSC_ATOMIC_MNEMONIC;
    }

lsc_apndctr_atomic_add|lsc_apndctr_atomic_sub|lsc_apndctr_atomic_store {
      TRACE("** lsc_apndctr_atomic INST");
      CISAlval.lsc_subOpcode = str2lscop(yytext);
      // this is set in the parser based on the SFID
      // CISAlval.lsc_opcode = ISA_LSC_UNTYPED;
      return LSC_ATOMIC_MNEMONIC;
    }

lsc_atomic_bfadd|lsc_atomic_bfsub|lsc_atomic_bfmin|lsc_atomic_bfmax|lsc_atomic_bfcas {
      TRACE("** lsc_atomic_bf INST");
      CISAlval.lsc_subOpcode = str2lscop(yytext);
      return LSC_ATOMIC_MNEMONIC;
    }

lsc_extended_cache_ctrl {
      TRACE("** lsc_extended_cache_ctrl INST");
      CISAlval.lsc_subOpcode = str2lscop(yytext);
      return LSC_EXTENDED_CACHE_CTRL_MNEMONIC;
}
lsc_read_state_info {
        TRACE("** lsc_read_state_info INST");
        CISAlval.lsc_subOpcode = LSC_READ_STATE_INFO;
        return LSC_READ_STATE_INFO_MNEMONIC;
    }
lsc_fence {
        TRACE("** lsc_fence INST");
        CISAlval.lsc_opcode = ISA_LSC_FENCE;
        return LSC_FENCE_MNEMONIC;
    }

nbarrier\.signal {
        TRACE("** nbarrier.signal INST");
        CISAlval.opcode = ISA_NBARRIER;
        return NBARRIER_SIGNAL;
    }

nbarrier\.wait {
        TRACE("** nbarrier.wait INST");
        CISAlval.opcode = ISA_NBARRIER;
        return NBARRIER_WAIT;
    }

oword_ld|oword_st|oword_ld_unaligned {
        TRACE("** oword_load INST");
        CISAlval.opcode = str2opcode(yytext);
        return OWORD_OP;
    }

media_ld|media_st {
        TRACE("** media INST");
        CISAlval.opcode = str2opcode(yytext);
        return MEDIA_OP;
    }

gather|scatter {
        TRACE("** gather/scatter INST");
        CISAlval.opcode = str2opcode(yytext);
        return SCATTER_OP;
    }

gather4_typed|scatter4_typed {
        TRACE("** gather/scatter typed INST");
        CISAlval.opcode = str2opcode(yytext);
        return SCATTER_TYPED_OP;
    }

gather_scaled|scatter_scaled {
        TRACE("** scaled gather/scatter INST");
        CISAlval.opcode = str2opcode(yytext);
        return SCATTER_SCALED_OP;
    }

gather4_scaled|scatter4_scaled {
        TRACE("** scaled gather/scatter INST");
        CISAlval.opcode = str2opcode(yytext);
        return SCATTER4_SCALED_OP;
    }

barrier {
        TRACE("** barrier INST");
        CISAlval.opcode = str2opcode(yytext);
        return BARRIER_OP;
    }

sbarrier\.signal {
        TRACE("** sbarrier.signal INST");
        CISAlval.opcode = ISA_SBARRIER;
        return SBARRIER_SIGNAL;
    }

sbarrier\.wait {
        TRACE("** sbarrier.wait INST");
        CISAlval.opcode = ISA_SBARRIER;
        return SBARRIER_WAIT;
    }

sampler_cache_flush {
        TRACE("** sampler_cache_flush INST");
        CISAlval.opcode = str2opcode(yytext);
        return CACHE_FLUSH_OP;
    }

wait {
        TRACE("** wait INST");
        CISAlval.opcode = str2opcode(yytext);
        return WAIT_OP;
    }

fence_global {
        TRACE("** fence global INST");
        CISAlval.opcode = str2opcode("fence");
        return FENCE_GLOBAL_OP;
    }
fence_local {
        TRACE("** fence local INST");
        CISAlval.opcode = str2opcode("fence");
        return FENCE_LOCAL_OP;
    }

fence_sw {
        TRACE("** fence SW INST");
        CISAlval.opcode = str2opcode("fence");
        return FENCE_SW_OP;
    }

yield {
        TRACE("** yield INST");
        CISAlval.opcode = str2opcode(yytext);
        return YIELD_OP;
    }

dword_atomic {
        TRACE("** atomic INST");
        CISAlval.opcode = str2opcode(yytext);
        return DWORD_ATOMIC_OP;
    }

typed_atomic {
        TRACE("** typed atomic INST");
        CISAlval.opcode = str2opcode(yytext);
        return TYPED_ATOMIC_OP;
    }

sample|load {
        TRACE("** sample INST");
        CISAlval.opcode = str2opcode(yytext);
        return SAMPLE_OP;
    }
sample_unorm {
        TRACE("** sample INST");
        CISAlval.opcode = str2opcode(yytext);
        return SAMPLE_UNORM_OP;
    }

vme_ime {
        TRACE("** VME_IME INST");
        CISAlval.opcode = str2opcode(yytext);
        return VME_IME_OP;
    }
vme_sic {
        TRACE("** VME_SIC INST");
        CISAlval.opcode = str2opcode(yytext);
        return VME_SIC_OP;
    }
vme_fbr {
        TRACE("** VME_FBR INST");
        CISAlval.opcode = str2opcode(yytext);
        return VME_FBR_OP;
    }

jmp|goto {
        TRACE("** branch INST");
        CISAlval.opcode = str2opcode(yytext);
        return BRANCH_OP;
    }

ret|fret {
        TRACE("** return INST");
        CISAlval.opcode = str2opcode(yytext);
        return RET_OP;
}

call {
        TRACE("** call INST");
        CISAlval.cisa_call.opcode = ISA_CALL;
        CISAlval.cisa_call.is_fccall = false;
        return CALL_OP;
}

fccall {
        TRACE("** fccall INST");
        CISAlval.cisa_call.opcode = ISA_CALL;
        CISAlval.cisa_call.is_fccall = true;
        return CALL_OP;
}

fcall {
   TRACE("** function call INST");
        CISAlval.opcode = ISA_FCALL;
        return FCALL;
}

ifcall {
        TRACE("** indirect call INST");
        CISAlval.opcode = ISA_IFCALL;
        return IFCALL;
    }

faddr {
        TRACE("** function address INST");
        CISAlval.opcode = ISA_FADDR;
        return FADDR;
    }

switchjmp {
        TRACE("** branch INST");
        CISAlval.opcode = str2opcode(yytext);
        return SWITCHJMP_OP;
    }

raw_send {
       TRACE("** RAW_SEND");
       CISAlval.opcode = ISA_RAW_SEND;
       return RAW_SEND_STRING;
    }

raw_sendc {
        TRACE("** RAW_SENDC");
        CISAlval.opcode = ISA_RAW_SEND;
        return RAW_SENDC_STRING;
    }

raw_sends {
        TRACE("** RAW_SENDS");
        CISAlval.opcode = ISA_RAW_SENDS;
        return RAW_SENDS_STRING;
    }

raw_sends_eot {
        TRACE("** RAW_SENDS_EOT");
        CISAlval.opcode = ISA_RAW_SENDS;
        return RAW_SENDS_EOT_STRING;
    }

raw_sendsc {
        TRACE("** RAW_SENDSC");
        CISAlval.opcode = ISA_RAW_SENDS;
        return RAW_SENDSC_STRING;
    }

raw_sendsc_eot {
        TRACE("** RAW_SENDSC_EOT");
        CISAlval.opcode = ISA_RAW_SENDS;
        return RAW_SENDSC_EOT_STRING;
    }

raw_sendg {
      TRACE("** RAW_SENDG");
      CISAlval.opcode = ISA_RAW_SENDG;
      return RAW_SENDG_STRING;
    }
raw_sendg_eot {
      TRACE("** RAW_SENDG_EOT");
      CISAlval.opcode = ISA_RAW_SENDG;
      return RAW_SENDG_EOT_STRING;
    }
raw_sendgc {
      TRACE("** RAW_SENDGC");
      CISAlval.opcode = ISA_RAW_SENDG;
      return RAW_SENDGC_STRING;
    }
raw_sendgc_eot {
      TRACE("** RAW_SENDGC_EOT");
      CISAlval.opcode = ISA_RAW_SENDG;
      return RAW_SENDGC_EOT_STRING;
    }

avs {
        TRACE("** AVS INST");
        CISAlval.opcode = str2opcode(yytext);
        return AVS_OP;
    }

(FILE|\.file) {
        // FIXME: need to retire FILE and LOC because
        // they will confict with identifiers
        // retain .file and migrate to that
        TRACE("** FILE");
        CISAlval.opcode = str2opcode("file");
        return FILE_OP;
    }

(LOC|\.loc) {
        // FIXME: same as FILE above...
        TRACE("** LOC");
        CISAlval.opcode = str2opcode("loc");
        return LOC_OP;
    }

sample_3d|sample_b|sample_l|sample_c|sample_d|sample_b_c|sample_l_c|sample_d_c|sample_lz|sample_c_lz {
        TRACE("** SAMPLE_3D");
        CISAlval.sample3DOp = str2SampleOpcode(yytext, pBuilder->getPlatform());
        return SAMPLE_3D_OP;
    }

sample_po|sample_po_b|sample_po_l|sample_po_c|sample_po_d|sample_po_l_c|sample_po_lz|sample_po_c_lz {
        TRACE("** SAMPLE_3D");
        CISAlval.sample3DOp = str2SampleOpcode(yytext, pBuilder->getPlatform());
        return SAMPLE_3D_OP;
    }

sample_po_b_c|sample_po_d_c {
        TRACE("** SAMPLE_3D");
        CISAlval.sample3DOp = str2SampleOpcode(yytext, pBuilder->getPlatform());
        return SAMPLE_3D_OP;
    }

sample_mlod|sample_c_mlod|sample_d_c_mlod|lod {
        TRACE("** SAMPLE_3D");
        CISAlval.sample3DOp = str2SampleOpcode(yytext, pBuilder->getPlatform());
        return SAMPLE_3D_OP;
    }

load_3d|load_mcs|load_2dms_w|load_lz {
        TRACE("** LOAD_3D");
        CISAlval.sample3DOp = str2SampleOpcode(yytext, pBuilder->getPlatform());
        return LOAD_3D_OP;
    }

load_l {
        TRACE("** LOAD_3D");
        CISAlval.sample3DOp = str2SampleOpcode(yytext, pBuilder->getPlatform());
        return LOAD_3D_OP;
    }

sample4|sample4_c {
        TRACE("** SAMPLE4_3D");
        CISAlval.sample3DOp = str2SampleOpcode(yytext, pBuilder->getPlatform());
        return SAMPLE4_3D_OP;
    }

sample4_po|sample4_po_c {
        TRACE("** SAMPLE4_3D");
        CISAlval.sample3DOp = str2SampleOpcode(yytext, pBuilder->getPlatform());
        return SAMPLE4_3D_OP;
    }

sample4_po_b|sample4_po_l|sample4_po_l_c|sample4_po_i|sample4_po_i_c {
        TRACE("** SAMPLE4_3D");
        CISAlval.sample3DOp = str2SampleOpcode(yytext, pBuilder->getPlatform());
        return SAMPLE4_3D_OP;
    }

sample4_i|sample4_l|sample4_b|sample4_i_c|sample4_l_c {
        TRACE("** SAMPLE4_3D");
        CISAlval.sample3DOp = str2SampleOpcode(yytext, pBuilder->getPlatform());
        return SAMPLE4_3D_OP;
    }

resinfo {
        TRACE("** RESINFO_3D");
        CISAlval.opcode = str2opcode("info_3d");
        return RESINFO_OP_3D;
    }

sampleinfo {
        TRACE("** SAMPLEINFO_3D");
        CISAlval.opcode = str2opcode("info_3d");
        return SAMPLEINFO_OP_3D;
    }

rt_write_3d {
        TRACE("** RTWRITE_3D");
        CISAlval.opcode = str2opcode("rt_write_3d");
        return RTWRITE_OP_3D;
    }


urb_write_3d {
        TRACE("** URBWRITE_3D");
        CISAlval.opcode = str2opcode("urb_write_3d");
        return URBWRITE_OP_3D;
    }

lifetime"."start {
        TRACE("** Lifetime.start");
        CISAlval.opcode = str2opcode("lifetime");
        return LIFETIME_START_OP;
    }

lifetime"."end {
        TRACE("** Lifetime.end");
        CISAlval.opcode = str2opcode("lifetime");
        return LIFETIME_END_OP;
    }

^[a-zA-Z_$@?][a-zA-Z0-9_\-$@?]*: {
        TRACE("**  LABEL");
        char* yytext_dup = strdup(yytext);
        yytext_dup[yyleng - 1] = '\0';
        CISAlval.string = yytext_dup;
        return LABEL;
    }



"."(nomod|modified|top|bottom|top_mod|bottom_mod) {
        TRACE("** MEDIA MODE :");
        CISAlval.media_mode = mediaMode(yytext+1);
        return MEDIA_MODE;
    }

AVS_(16|8)_(FULL|DOWN_SAMPLE) {
      TRACE("** Output Format Control");
      CISAlval.cntrl = avs_control(yytext);
      return CNTRL;
    }

AVS_(4|8|16)x(4|8) {
      TRACE("** AVS Exec Mode");
      CISAlval.execMode = avsExecMode(yytext);
      return EXECMODE;
    }

"."mod {
        TRACE("** O MODE :");
        CISAlval.oword_mod = true;
        return OWORD_MODIFIER;
    }

[0-9]+         {
        TRACE("** DEC_LIT");
        CISAlval.intval = atoi(yytext);
        return DEC_LIT;
    }

0[xX][[:xdigit:]]+ {
        TRACE("** HEX_LIT");
        CISAlval.intval = hexToInt(yytext+2, yyleng-2);
        return HEX_LIT;
    }

[0-9]+"."[0-9]+":f" {
        TRACE("** F32_LIT");
        CISAlval.fltval = (float)atof(yytext);
        return F32_LIT;
    }

([0-9]+|[0-9]+"."[0-9]+)[eE]("+"|"-")[0-9]+":f" {
        TRACE("** F32_LIT");
        CISAlval.fltval = (float)atof(yytext);
        return F32_LIT;
    }

[0-9]+"."[0-9]+":df" {
        TRACE("** F64_LIT");
        CISAlval.fltval = atof(yytext);
        return F64_LIT;
    }

([0-9]+|[0-9]+"."[0-9]+)[eE]("+"|"-")[0-9]+":df" {
        TRACE("** F64_LIT");
        CISAlval.fltval = atof(yytext);
        return F64_LIT;
    }



type[ ]*=[ ]*(ud|d|uw|w|ub|b|df|f|bool|uq|q|UD|D|UW|W|UB|B|DF|F|Bool|BOOL|UQ|Q|hf|HF|bf|BF) {
        TRACE("** TYPE");
        CISAlval.type = str2type(yytext, yyleng);
        return DECL_DATA_TYPE;
    }

2GRF {
        /* other cases are handled as VAR */
        TRACE("** AlignType - 2GRF");
        CISAlval.align = ALIGN_2_GRF;
        // fprintf(stderr, "%s", "2GRF symbol is deprecated; please use GRFx2");
        return ALIGN_KEYWORD;
}

32word {
        /* other cases are handled as VAR */
        TRACE("** AlignType - 32word");
        CISAlval.align = ALIGN_32WORD;
        return ALIGN_KEYWORD;
}
64word {
        /* other cases are handled as VAR */
        TRACE("** AlignType - 64word");
        CISAlval.align = ALIGN_64WORD;
        return ALIGN_KEYWORD;
}

"(-)"    {TRACE("** SRCMOD_NEG"); return SRCMOD_NEG;}
"(abs)"  {TRACE("** SRCMOD_ABS"); return SRCMOD_ABS;}
"(-abs)" {TRACE("** SRCMOD_NEGABS"); return SRCMOD_NEGABS;}
"(~)"    {TRACE("** SRCMOD_NOT"); return SRCMOD_NOT;}

".sat"   {TRACE("** SAT");  return SAT;}

".pixel_null_mask" {
        TRACE("** PIXEL_NULL_MASK");
        return PIXEL_NULL_MASK;
    }

".cps" {
        TRACE("** CPS LOD Compensation enable");
        return CPS;
    }

".divS" {
        TRACE("** non-uniform Sampler State");
        return NON_UNIFORM_SAMPLER;
    }


"."(eq|ne|gt|ge|lt|le|EQ|NE|GT|GE|LT|LE) {
        TRACE("** COND_MOD");
        CISAlval.cond_mod = str2cond(yytext+1);
        return COND_MOD;
    }

:(df|DF)  {
        TRACE("** DFTYPE");
        CISAlval.type = str2type(yytext, yyleng);
        return DFTYPE;
    }

:(f|F)      {
        TRACE("** FTYPE");
        CISAlval.type = str2type(yytext, yyleng);
        return FTYPE;
    }

:(hf|HF)  {
        TRACE("** HFTYPE");
        CISAlval.type = str2type(yytext, yyleng);
        return HFTYPE;
    }

:(bf|BF)  {
        TRACE("** BFTYPE");
        CISAlval.type = str2type(yytext, yyleng);
        return BFTYPE;
    }

:(ud|d|uw|w|ub|b|bool|UD|D|UW|W|UB|B|BOOL|Bool|q|uq|Q|UQ|hf|HF)  {
        TRACE("** DATA TYPE");
        CISAlval.type = str2type(yytext, yyleng);
        return ITYPE;
    }

:(v|vf|V|VF|uv)  {
        TRACE("** VTYPE");
        CISAlval.type = str2type(yytext, yyleng);
        return VTYPE;
    }

:a(64|32|32s|32u|16) {
        TRACE("** LSC_ADDR_SIZE_TK");

        if (yytext[2]  == '6')
          CISAlval.lsc_addr_size = LSC_ADDR_SIZE_64b;
        else if (yytext[2]  == '3') {
          CISAlval.lsc_addr_size = LSC_ADDR_SIZE_32b;
          CISAlval.lsc_addr_size =
            yytext[4] == 's' ? LSC_ADDR_SIZE_32bS :
            yytext[4] == 'u' ? LSC_ADDR_SIZE_32bU :
                               LSC_ADDR_SIZE_32b;
        }
        else
          CISAlval.lsc_addr_size = LSC_ADDR_SIZE_16b;
        return LSC_ADDR_SIZE_TK;
    }


:((d64|d32|d16|d8|d8u32|d8c32|d16u32|d16c32|d16u32h|d16c32h)|(u64|u32|u16|u8|u8c32|u16c32|u16c32h))(x(64|32|16|8|4|3|2|1))?t? {
        TRACE("** LSC_DATA_SHAPE_TK");

        int off = 1;
        LSC_DATA_SIZE dsz = decodeDataSizePrefix(yytext,&off);
        LSC_DATA_ELEMS vsz = decodeDataElems(yytext,&off);
        LSC_DATA_ORDER trans = LSC_DATA_ORDER_NONTRANSPOSE;
        if (yytext[off] == 't') {
            trans = LSC_DATA_ORDER_TRANSPOSE;
        }

        CISAlval.lsc_data_shape.size = dsz;
        CISAlval.lsc_data_shape.elems = vsz;
        CISAlval.lsc_data_shape.order = trans;

        return LSC_DATA_SHAPE_TK;
    }

:((d64|d32|d16|d8|d8c32|d16c32|d16c32h)|(u64|u32|u16|u8|u8c32|u16c32|u16c32h))\.((xy?z?w?)|(yz?w?)|(zw?)|(w)) {
        TRACE("** LSC_DATA_SHAPE_TK_CHMASK");
        int off = 1; // if there's an x (vector suffix), it starts here
        LSC_DATA_SIZE dsz = decodeDataSizePrefix(yytext, &off);
        off++; // skip the .
        int chmask = 0;
        while (off < yyleng) {
            switch (yytext[off++]) {
            case 'x': chmask |= LSC_DATA_CHMASK_X; break;
            case 'y': chmask |= LSC_DATA_CHMASK_Y; break;
            case 'z': chmask |= LSC_DATA_CHMASK_Z; break;
            case 'w': chmask |= LSC_DATA_CHMASK_W; break;
            default: break; // unreachable
            }
        }
        CISAlval.lsc_data_shape.size = dsz;
        CISAlval.lsc_data_shape.order = LSC_DATA_ORDER_NONTRANSPOSE;
        CISAlval.lsc_data_shape.chmask = chmask;
        return LSC_DATA_SHAPE_TK_CHMASK;
    }

:((d64|d32|d16|d8|d8c32|d16c32|d16c32h)|(u64|u32|u16|u8|u8c32|u16c32|u16c32h))"."([1-9][0-9]*x)?([1-9][0-9]*x)([1-9][0-9]*)(n|t)?(n|t)? {
        TRACE("** LSC_DATA_SHAPE_TK_BLOCK2D");

        int off = 0;
        off++; // skip 'd' prefix

        LSC_DATA_SIZE dsz = decodeDataSizePrefix(yytext, &off);

        auto parseNextInt =
            [&]() {
                if (!isdigit(yytext[off]))
                    YY_FATAL_ERROR("LEXICAL SPEC ERROR (should be digit)");
                int val = 0;
                while (isdigit(yytext[off]))
                    val = 10*val + yytext[off++] - '0';
                return val;
            };

        off++; // skip '.'
        // e.g.  d8.2x32x32nn or d8.32x64 (implicitly .1x32x64)

        int numBlocks = 1;
        int width = parseNextInt(); // width
        off++; // 'x'
        int height = parseNextInt(); // num blocks or height
        if (yytext[off] == 'x') {
            // e.g. 2x32x32...: arrlen x width x height
            off++;
            numBlocks = width;
            width = height;
            height = parseNextInt();
        } // else: short form: e.g. 32x32 (width x height)
        LSC_DATA_ORDER dord = LSC_DATA_ORDER_NONTRANSPOSE;
        bool vnni = false;
        if (yytext[off] == 'n' || yytext[off] == 't') {
            dord = yytext[off] == 't' ?
                LSC_DATA_ORDER_TRANSPOSE : LSC_DATA_ORDER_NONTRANSPOSE;
            off++;
            if (yytext[off] == 'n' || yytext[off] == 't') {
                vnni = yytext[off] == 't';
                off++;
            }
        }
        if (yytext[off] != 0)
            YY_FATAL_ERROR("LEXICAL SPEC ERROR (should NUL)");

        CISAlval.lsc_data_shape2d.size = dsz;
        CISAlval.lsc_data_shape2d.order = dord;
        CISAlval.lsc_data_shape2d.blocks = numBlocks;
        CISAlval.lsc_data_shape2d.width = width;
        CISAlval.lsc_data_shape2d.height = height;
        CISAlval.lsc_data_shape2d.vnni = vnni;

        return LSC_DATA_SHAPE_TK_BLOCK2D;
    }

"."(df|uc|ca|wb|wt|st|cc|ri) {
        if (strcmp(yytext+1,"df") == 0) {
            CISAlval.lsc_caching_opt = LSC_CACHING_DEFAULT;
        } else if (strcmp(yytext+1,"uc") == 0) {
            CISAlval.lsc_caching_opt = LSC_CACHING_UNCACHED;
        } else if (strcmp(yytext+1,"ca") == 0) {
            CISAlval.lsc_caching_opt = LSC_CACHING_CACHED;
        } else if (strcmp(yytext+1,"wb") == 0) {
            CISAlval.lsc_caching_opt = LSC_CACHING_WRITEBACK;
        } else if (strcmp(yytext+1,"wt") == 0) {
            CISAlval.lsc_caching_opt = LSC_CACHING_WRITETHROUGH;
        } else if (strcmp(yytext+1,"st") == 0) {
            CISAlval.lsc_caching_opt = LSC_CACHING_STREAMING;
        } else if (strcmp(yytext+1,"cc") == 0) {
            CISAlval.lsc_caching_opt = LSC_CACHING_CONSTCACHED;
        } else { /* ri */
            CISAlval.lsc_caching_opt = LSC_CACHING_READINVALIDATE;
        }
        return LSC_CACHING_OPT;
    }

"."(set|reset) {
      if (strcmp(yytext+1, "reset") == 0) {
        CISAlval.lsc_cache_ctrl_operation = CCOP_DIRTY_RESET;
      }
      return LSC_CACHE_CTRL_OPERATION_TYPE;
  }

"."(64B|128B|192B|256B) {
      if (strcmp(yytext+1, "64B") == 0) {
        CISAlval.lsc_cache_ctrl_size = CCSIZE_64B;
      } else {
        CISAlval.lsc_cache_ctrl_size = CCSIZE_64B;
      }
      return LSC_CACHE_CTRL_SIZE_TYPE;
}

".ov"   {TRACE("** OV"); return OV;}
".msaa" {TRACE("** MSAA"); return MSAA;}
flat   {return LSC_AM_FLAT;}
bti    {return LSC_AM_BTI;}
ss     {return LSC_AM_SS;}
bss    {return LSC_AM_BSS;}
arg    {return LSC_AM_ARG;}
surf   {return LSC_AM_SURF;}

".none" {
        CISAlval.lsc_fence_op = LSC_FENCE_OP_NONE;
        return LSC_FENCE_OP_TYPE;
    }
".evict" {
        CISAlval.lsc_fence_op = LSC_FENCE_OP_EVICT;
        return LSC_FENCE_OP_TYPE;
    }
".invalidate" {
        CISAlval.lsc_fence_op = LSC_FENCE_OP_INVALIDATE;
        return LSC_FENCE_OP_TYPE;
    }
".discard" {
        CISAlval.lsc_fence_op = LSC_FENCE_OP_DISCARD;
        return LSC_FENCE_OP_TYPE;
    }
".clean" {
        CISAlval.lsc_fence_op = LSC_FENCE_OP_CLEAN;
        return LSC_FENCE_OP_TYPE;
    }
".flushl3" {
        CISAlval.lsc_fence_op = LSC_FENCE_OP_FLUSHL3;
        return LSC_FENCE_OP_TYPE;
    }
".type6" {
        CISAlval.lsc_fence_op = LSC_FENCE_OP_TYPE6;
        return LSC_FENCE_OP_TYPE;
    }

".group" {
        CISAlval.lsc_scope = LSC_SCOPE_GROUP;
        return LSC_FENCE_SCOPE;
    }
".local" {
        CISAlval.lsc_scope = LSC_SCOPE_LOCAL;
        return LSC_FENCE_SCOPE;
    }
".tile" {
        CISAlval.lsc_scope = LSC_SCOPE_TILE;
        return LSC_FENCE_SCOPE;
    }
".gpu" {
        CISAlval.lsc_scope = LSC_SCOPE_GPU;
        return LSC_FENCE_SCOPE;
    }
".gpus" {
        CISAlval.lsc_scope = LSC_SCOPE_GPUS;
        return LSC_FENCE_SCOPE;
    }
".sysrel" {
        CISAlval.lsc_scope = LSC_SCOPE_SYSREL;
        return LSC_FENCE_SCOPE;
    }
".sysacq" {
        CISAlval.lsc_scope = LSC_SCOPE_SYSACQ;
        return LSC_FENCE_SCOPE;
    }

".ugml" {
        CISAlval.lsc_sfid = LSC_UGML;
        return LSC_SFID_UNTYPED_TOKEN;
    }
".ugm" {
        CISAlval.lsc_sfid = LSC_UGM;
        return LSC_SFID_UNTYPED_TOKEN;
    }
".slm" {
        CISAlval.lsc_sfid = LSC_SLM;
        return LSC_SFID_UNTYPED_TOKEN;
    }
".tgm" {
        CISAlval.lsc_sfid = LSC_TGM;
        return LSC_SFID_TYPED_TOKEN;
    }
".aligned" {
        return SVM_ALIGNED;
    }
".unaligned" {
        return SVM_UNALIGNED;
    }



"."((R|r)((G|g)?(B|b)?(A|a)?)|(G|g)((B|b)?(A|a)?)|(B|b)((A|a)?)|(A|a))  {
        TRACE("** CHANNEL MASK");
        CISAlval.s_channel = ChannelMask::createFromString(yytext+1).getAPI();
        return SAMPLER_CHANNEL;
    }

"."(16-full|16-downsampled|8-full|8-downsampled) {
        TRACE("** OUTPUT_FORMAT");
        CISAlval.s_channel_output = Get_Channel_Output(yytext+1);
        return CHANNEL_OUTPUT;
    }

"."("<"[a-zA-Z]+">")+ {
        TRACE("** RTWRITE OPTION");
        CISAlval.string = strdup(yytext+1);
        return RTWRITE_OPTION;
    }

".any" {
        TRACE("** PRED_CNTL (.any)");
        CISAlval.pred_ctrl = PRED_CTRL_ANY;
        return PRED_CNTL;
    }
".all" {
        TRACE("** PRED_CNTL (.all)");
        CISAlval.pred_ctrl = PRED_CTRL_ALL;
        return PRED_CNTL;
    }


%null {
        TRACE("** Built-in %%null");
        CISAlval.string = strdup(yytext);
        return BUILTIN_NULL;
    }

%sizeof {
        TRACE("** Built-in %%sizeof");
        return BUILTIN_SIZEOF;
    }

%DispatchSimd {
        TRACE("** Built-in %%DispatchSimd");
        return BUILTIN_DISPATCH_SIMD_SIZE;
    }


%[[:alpha:]_][[:alnum:]_]* {
        // this matches %null, but lex prefers the first pattern
        TRACE("** Builtin-in variable");
        CISAlval.string = strdup(yytext);
        return BUILTIN;
    }

[[:alpha:]_][[:alnum:]_]* {
        TRACE("** IDENTIFIER");
        CISAlval.string = strdup(yytext);
        return IDENT;
    }

".uniform" {
        TRACE("** UNIFORM");
        CISAlval.string = strdup(yytext);
        return UNIFORM;
    }




[^ \t\n]       {TRACE("** SPACE END"); return *yytext;}

"."(E?I?S?C?R?(L1)?)     {
        TRACE("** FENCE Options");

        CISAlval.fence_options = FENCEOptions(yytext+1);
        return FENCE_OPTIONS;
    }

[ \n\t]+"\\"\n {TRACE("** Multiple instructions in a line");}


%%

int yywrap() { return 1;}

// convert "ud", "w" to Type_UD Type_W
static VISA_Type str2type(const char *str, int str_len)
{
    // find the starting of the type string
    int i;
    char *ty_str;
    char lowered[20];

    //lower the chars
    for (i = 0; i < str_len; i++) {
        lowered[i] = tolower(str[i]);
    }
    lowered[i] = '\0';
    ty_str = lowered + str_len;

    while (*ty_str != ' ' &&
           *ty_str != '=' &&
           *ty_str != ':' &&
           ty_str != lowered )
       ty_str--;

    ty_str++;

    // match string
    for (int i = 0; i < ISA_TYPE_NUM; i++) {
        if (strcmp(CISATypeTable[i].typeName, ty_str) == 0)
            return (VISA_Type)i;
    }

    return ISA_TYPE_NUM;
}


static GenPrecision str2Precision(const char *str, int str_len)
{
    if (str_len == 2) {
        char c0 = tolower(str[0]);
        char c1 = str[1];
        if (c0 == 's') {
            switch (c1) {
            default: break; // fall-thru
            case '1' : return GenPrecision::S1;
            case '2' : return GenPrecision::S2;
            case '4' : return GenPrecision::S4;
            case '8' : return GenPrecision::S8;
            }
        } else if (c0 == 'u') {
            switch (c1) {
            default: break; // fall-thru
            case '1' : return GenPrecision::U1;
            case '2' : return GenPrecision::U2;
            case '4' : return GenPrecision::U4;
            case '8' : return GenPrecision::U8;
            }
        } else if (c0 == 'b') {
            c1 = tolower(c1);
            if (c1 == 'f') {
               return GenPrecision::BF16;
            }
        } else if (c0 == 'h') {
            c1 = tolower(c1);
            if (c1 == 'f') {
                return GenPrecision::FP16;
            }
        }
    }
    else if (str_len == 3) {
      char c0 = tolower(str[0]);
      char c1 = tolower(str[1]);
      char c2 = tolower(str[2]);
      if (c0 == 'b' && c1 == 'f' && c2 == '8')
        return GenPrecision::BF8;
      else if (c0 == 'h' && c1 == 'f' && c2 == '8')
        return GenPrecision::HF8;
    }
    else if (str_len == 4) {
      char c0 = tolower(str[0]);
      char c1 = tolower(str[1]);
      char c2 = tolower(str[2]);
      char c3 = tolower(str[3]);
      if (c0 == 'e' && c1 == '2' && c2 == 'm' && c3 == '1')
        return GenPrecision::E2M1;
    }

    YY_FATAL_ERROR("Invalid Gen Precision");

    return GenPrecision::INVALID;
}

static LSC_OP str2lscop(const char *str)
{
    // could get called for typed, but untyped has all subops and
    // will return the correct subopcode.
    const auto &lsc_op = CISA_INST_table[ISA_LSC_UNTYPED];
    return (LSC_OP)lsc_op.getSubInstDescByName(str).subOpcode;
}

static LSC_DATA_SIZE decodeDataSizePrefix(const char *yytext, int *off) {
    LSC_DATA_SIZE dsz = LSC_DATA_SIZE_INVALID;
    *off += 1; // 'd' or 'u'
    if (strncmp(yytext + *off,"8u32", 4) == 0 ||
        strncmp(yytext + *off,"8c32", 4) == 0) {
        dsz = LSC_DATA_SIZE_8c32b;
        *off += 4;
    } else if (strncmp(yytext + *off, "16u32h", 6) == 0 ||
               strncmp(yytext + *off, "16c32h", 6) == 0) {
        // must be above "u16c32" case (longest match first)
        dsz = LSC_DATA_SIZE_16c32bH;
        *off += 6;
    } else if (strncmp(yytext + *off, "16u32", 5) == 0 ||
               strncmp(yytext + *off, "16c32", 5) == 0) {
        dsz = LSC_DATA_SIZE_16c32b;
        *off += 5;
    } else if (strncmp(yytext + *off, "64", 2) == 0) {
        dsz = LSC_DATA_SIZE_64b;
        *off += 2;
    } else if (strncmp(yytext + *off, "32", 2) == 0) {
        dsz = LSC_DATA_SIZE_32b;
        *off += 2;
    } else if (strncmp(yytext + *off, "16", 2) == 0) {
        dsz = LSC_DATA_SIZE_16b;
        *off += 2;
    } else if (strncmp(yytext + *off, "8", 1) == 0) {
        dsz = LSC_DATA_SIZE_8b;
        *off += 1;
    } else {
        YY_FATAL_ERROR("decodeDataSizePrefix: lexical spec error (the pattern is busted)");
    }
    return dsz;
}

static LSC_DATA_ELEMS decodeDataElems(const char *str, int *off)
{
    LSC_DATA_ELEMS vsz = LSC_DATA_ELEMS_1;
    if (yytext[*off] == 'x') {
        *off += 1;
        if (strncmp(yytext + *off,"64", 2) == 0) {
            vsz = LSC_DATA_ELEMS_64;
            *off += 2;
        } else if (strncmp(yytext + *off,"32", 2) == 0) {
            vsz = LSC_DATA_ELEMS_32;
            *off += 2;
        } else if (strncmp(yytext + *off,"16", 2) == 0) {
            vsz = LSC_DATA_ELEMS_16;
            *off += 2;
        } else if (yytext[*off] == '8') {
            vsz = LSC_DATA_ELEMS_8;
            *off += 1;
        } else if (yytext[*off] == '4') {
            vsz = LSC_DATA_ELEMS_4;
            *off += 1;
        } else if (yytext[*off] == '3') {
            vsz = LSC_DATA_ELEMS_3;
            *off += 1;
        } else if (yytext[*off] == '2') {
            vsz = LSC_DATA_ELEMS_2;
            *off += 1;
        } else if (yytext[*off] == '1') {
            vsz = LSC_DATA_ELEMS_1;
            *off += 1;
        } else {
            YY_FATAL_ERROR("decodeDataElems: lexical spec error (the pattern is busted)");
        }
    }
    return vsz;
}

// convert "z" to Mod_z
static VISA_Cond_Mod str2cond(const char *str)
{
    for (int i = 0; i < ISA_CMP_UNDEF; i++)
        if (strcmp(Rel_op_str[i], str) == 0)
            return (VISA_Cond_Mod)i;

    YY_FATAL_ERROR("Invalid Data Type");

    return ISA_CMP_UNDEF;
}

static unsigned hexCharToDigit(char d)
{
    if (d >= '0' && d <= '9')
        return d - '0';
    else if (d >= 'a' && d <= 'f')
        return d - 'a' + 10;
    else if (d >= 'A' && d <= 'F')
        return d - 'A' + 10;

    YY_FATAL_ERROR("lexical error: invalid hex digit");

    return 0;
}

// convert hex string to int
static int64_t hexToInt(const char *hex_str, int str_len)
{
    if (str_len > 16) { // make sure is within 32 bits
        YY_FATAL_ERROR("lexical error: hex literal too long");
    }

    uint64_t result = 0;

    // starting from the last digit
    for (int i = 0; i < str_len; i++)
        result += (uint64_t)hexCharToDigit(*(hex_str+str_len-1-i)) << (i*4);

    return (int64_t)result;
}

// convert str to its corresponding opcode
static ISA_Opcode str2opcode(const char *op_str)
{
    for (int i = 0; i < ISA_NUM_OPCODE; i++)
        if (strcmp(ISA_Inst_Table[i].str, op_str) == 0)
            return ISA_Inst_Table[i].op;

    YY_FATAL_ERROR("Invalid OpCode");

    return ISA_RESERVED_0;
}

static VISASampler3DSubOpCode str2SampleOpcode(const char *str, TARGET_PLATFORM platform)
{
    VISASampler3DSubOpCode op = getSampleOpFromName(str, platform);
    if (op >= 0)
        return op;

    YY_FATAL_ERROR("Invalid 3D Sample OpCode");

    return VISA_3D_TOTAL_NUM_OPS;
}

static VISAAtomicOps str2atomic_opcode(const char *op_str)
{
    for (unsigned i = 0; i < ATOMIC_UNDEF; ++i)
        if (strcmp(CISAAtomicOpNames[i], op_str) == 0)
            return static_cast<VISAAtomicOps>(i);

    YY_FATAL_ERROR("Invalid Atomic OpCode");

    return ATOMIC_UNDEF;
}

// convert str to its corresponding media load mode
static MEDIA_LD_mod mediaMode(const char *str)
{
    for (int i = 0; i < MEDIA_LD_Mod_NUM; i++)
        if (!strcmp(media_ld_mod_str[i], str))
            return (MEDIA_LD_mod)i;

    YY_FATAL_ERROR("Invalid Medial Mode");

    return MEDIA_LD_nomod;
}

// convert str to its corresponding avs output format control
static OutputFormatControl avs_control(const char* str)
{
    for (int i = 0; i < 4; i++)
        if (!strcmp(avs_control_str[i], str))
            return (OutputFormatControl)i;

    YY_FATAL_ERROR("Invalid AVS Control");

    return AVS_16_FULL;
}

static AVSExecMode avsExecMode(const char* str)
{
    for (int i = 0; i < 3; i++)
        if (!strcmp(avs_exec_mode[i], str))
            return (AVSExecMode)i;

    YY_FATAL_ERROR("Invalid AVS Exec Mode");

    return AVS_16x4;
}

static unsigned char FENCEOptions(const char* str)
{
    int count = strlen(str);
    unsigned char result=0;
    /*
        Bit 0: commit enable
        Bit 1: flush instruction cache if set.
        Bit 2: flush sampler cache if set.
        Bit 3: flush constant cache if set.
        Bit 4: flush read-write cache if set.
        Bit 5: reserved (global/SLM is determined by opcode)
        Bit 6: flush L1
    */
    for(int i = 0; i < count; i++)
    {
        if (str[i] == 'E')
        {
            result |= 1;
        }
        else if(str[i] == 'I')
        {
            result |= (1<<1);
        }
        else if(str[i] == 'S')
        {
            result |= (1<<2);
        }
        else if(str[i] == 'C')
        {
            result |= (1<<3);
        }
        else if(str[i] == 'R')
        {
            result |= (1<<4);
        }
        else if (str[i] == 'L' && i + 1 < count && str[i+1] == '1')
        {
            result |= (1<<6);
        }
    }

    return result;
}

static CHANNEL_OUTPUT_FORMAT Get_Channel_Output(const char* str)
{
    for (int i = 0; i < CHANNEL_OUTPUT_NUM; i++)
    {
        if (!strcmp(sampler_channel_output_str[i], str))
        {
            return (CHANNEL_OUTPUT_FORMAT)i;
        }
    }

    YY_FATAL_ERROR("Invalid channel output format\n");
    YY_FATAL_ERROR(str);
    return CHANNEL_16_BIT_FULL;
}

static void appendStringLiteralChar(char c, char *buf, size_t *len)
{
    if (*len == MAX_STRLIT_SIZE) {
        YY_FATAL_ERROR("string literal too long");
    }
    buf[(*len)++] = c;
    buf[*len] = 0;
}
static LFSR_FC str2lfsrfc(const char* str)
{
    if(!strcmp(str, "b32"))
        return LFSR_FC::B32;
    else if (!strcmp(str, "b16v2"))
        return LFSR_FC::B16V2;
    else if (!strcmp(str, "b8v4"))
        return LFSR_FC::B8V4;

    YY_FATAL_ERROR("Invalid lfsr func control");
    return LFSR_FC::INVALID;
}

static DNSCL_CONVERT_TYPE str2DnsclConvertType(const char *str, int *offset) {
    DNSCL_CONVERT_TYPE type = DNSCL_CONVERT_TYPE::INVALID;
    if (strncmp(yytext + *offset,"bftoe2m1", 8) == 0)
        type = DNSCL_CONVERT_TYPE::BFTOE2M1;
    else if (strncmp(yytext + *offset,"bftoint4", 8) == 0)
        type = DNSCL_CONVERT_TYPE::BFTOINT4;
    else if (strncmp(yytext + *offset,"hftoe2m1", 8) == 0)
        type = DNSCL_CONVERT_TYPE::HFTOE2M1;
    else if (strncmp(yytext + *offset,"hftoint4", 8) == 0)
        type = DNSCL_CONVERT_TYPE::HFTOINT4;
    else
        YY_FATAL_ERROR("Invalid dnscl conversion type");

    if (type != DNSCL_CONVERT_TYPE::INVALID)
        *offset += 8;

    return type;
}

static DNSCL_MODE str2DnsclMode(const char *str, int *offset){
    DNSCL_MODE mode = DNSCL_MODE::INVALID;
    if (strncmp(yytext + *offset,"mode0", 5) == 0)
        mode = DNSCL_MODE::MODE0;
    else if (strncmp(yytext + *offset,"mode1", 5) == 0)
        mode = DNSCL_MODE::MODE1;
    else if (strncmp(yytext + *offset,"mode2", 5) == 0)
        mode = DNSCL_MODE::MODE2;
    else if (strncmp(yytext + *offset,"mode3", 5) == 0)
        mode = DNSCL_MODE::MODE3;
    else
        YY_FATAL_ERROR("Invalid dnscl mode");

    if (mode != DNSCL_MODE::INVALID)
        *offset += 5;

    return mode;
}

static DNSCL_RND_MODE str2DnsclRoundMode(const char *str, int *offset){
    if (strncmp(yytext + *offset,"srnd", 4) == 0) {
        *offset += 4;
        return DNSCL_RND_MODE::STOCHASTIC_ROUND;
    } else if (strncmp(yytext + *offset,"rne", 3) == 0) {
        *offset += 3;
        return DNSCL_RND_MODE::RNE;
    } else {
        YY_FATAL_ERROR("Invalid dnscl rounding mode");
        return DNSCL_RND_MODE::INVALID;
    }
}
