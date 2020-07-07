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

#ifdef _MSC_VER
#pragma warning(default: 4005)
#endif

#include "CISA.tab.hpp"

#ifdef _DEBUG
#define TRACE(str) fprintf(yyout, str); ECHO; fprintf(yyout, "\n")
#else
#define TRACE(str)
#endif
static VISA_Type               str2type(const char *str, int str_len);
static VISA_Cond_Mod           str2cond(const char *str);
static VISAAtomicOps           str2atomic_opcode(const char *op_str);
static ISA_Opcode              str2opcode(const char* op_str);
static VISASampler3DSubOpCode  str2SampleOpcode(const char* str);
static int64_t                 hexToInt(const char *hex_str, int str_len);
static MEDIA_LD_mod            mediaMode(const char* str);
static OutputFormatControl     avs_control(const char* str);
static AVSExecMode             avsExecMode(const char* str);
static unsigned char           FENCEOptions(const char *str);
static COMMON_ISA_VME_OP_MODE  VMEType(const char *str);
static CHANNEL_OUTPUT_FORMAT   Get_Channel_Output(const char* str);
static void                    appendStringLiteralChar(char c, char *buf, size_t *len);

#ifdef _MSC_VER
#include <io.h>
#pragma warning(disable:4102; disable:4244; disable:4267)
#endif
%}

%option yylineno

%x   eat_comment
%x   string_literal

%%

([ \t]*\n[ \t]*)+ {
      TRACE("\n** DELIMITER");
      return STMT_DELIM;
    }
([ \t]*"\\n"[ \t]*)+ {
        TRACE("\n** DELIMITER");
        return STMT_DELIM;
    }
"//"[^\n]* {
        TRACE("\n** COMMENT TEXT");
        CISAlval.string = strdup(yytext);
        return COMMENT_LINE;
    }

"/*"  BEGIN(eat_comment);
<eat_comment>[^*]* /* eat anything that is not a * */
<eat_comment>"*"+[^*/]*  /* eat up '*'s not followed by '/'s */
<eat_comment>"*"+"/"  BEGIN(INITIAL);


\" {CISAlval.strlit.len = 0; CISAlval.strlit.decoded[0] = 0; BEGIN(string_literal);}
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
    \"                        {CISAlval.string = strdup(CISAlval.strlit.decoded); BEGIN(INITIAL); return STRING_LITERAL;}
    .                         {
    /* important: this must succeed the exit rule above (\"); lex prefers the first match */
        appendStringLiteralChar(yytext[0],CISAlval.strlit.decoded,&CISAlval.strlit.len);
    }
}


"*" {TRACE("\n** TIMES ");  return TIMES;}
"+" {TRACE("\n** PLUS ");   return PLUS;}
"-" {TRACE("\n** MINUS ");  return MINUS;}
"=" {TRACE("\n** EQUALS ");  return EQUALS;}
"&" {TRACE("\n** AMP ");  return AMP;}
"~" {TRACE("\n** TILDE ");  return TILDE;}
"!" {TRACE("\n** BANG "); return BANG;}

"." {TRACE("\n** COLON "); return DOT;}
"," {TRACE("\n** COMMA "); return COMMA;}
";" {TRACE("\n** SEMI "); return SEMI;}
":" {TRACE("\n** COLON "); return COLON;}
"/" {TRACE("\n** SLASH"); return SLASH;}

"(" {TRACE("\n** LPAREN "); return LPAREN;}
")" {TRACE("\n** RPAREN "); return RPAREN;}
"{" {TRACE("\n** LBRACE "); return LBRACE;}
"}" {TRACE("\n** RBRACE "); return RBRACE;}
"<" {TRACE("\n** LANGLE "); return LANGLE;}
">" {TRACE("\n** RANGLE "); return RANGLE;}
"r[" {
        TRACE("\n** Register Indirect LEFT bracket");
        CISAlval.string = strdup(yytext);
        return IND_LBRACK;
    }
"[" {
        TRACE("\n** LEFT bracket");
        CISAlval.string = strdup(yytext);
        return LBRACK;
    }

"]"  {
        TRACE("\n** RIGHT bracket");
        CISAlval.string = strdup(yytext);
        return RBRACK;
    }


".version"          {TRACE("\n** VERSION "); return DIRECTIVE_VERSION;}
".entry"            {TRACE("\n** ENTRY "); return DIRECTIVE_ENTRY;}
".decl"             {TRACE("\n** DECLARE "); return DIRECTIVE_DECL;}
".funcdecl"         {TRACE("\n** FUNCTION DECLARE "); return FUNC_DIRECTIVE_DECL;}
".attr"             {TRACE("\n** ATTR "); return DIRECTIVE_ATTR;}
".kernel_attr"      {TRACE("\n** KERNEL ATTR "); return DIRECTIVE_KERNEL_ATTR;}
".input"            {TRACE("\n** INPUT "); return DIRECTIVE_INPUT;}
".parameter"        {TRACE("\n** PARAMETER "); return DIRECTIVE_PARAMETER;}
".loc"              {TRACE("\n** LOC "); return DIRECTIVE_LOC;}
".function"         {TRACE("\n** FUNCTION "); return DIRECTIVE_FUNC;}
".global_function"  {TRACE("\n** GLOBAL FUNCTION "); return DIRECTIVE_GLOBAL_FUNC;}
".kernel"           {TRACE("\n** KERNEL NAME DIRECTIVE "); return DIRECTIVE_KERNEL;}

"."implicit[a-zA-Z0-9_\-$@?]* {
        TRACE("\n**  IMPLICIT_INPUT ");
        CISAlval.string = strdup(yytext);
        CISAlval.string[yyleng] = '\0';
        return IMPLICIT_INPUT;
    }

"."(add|sub|inc|dec|min|max|xchg|cmpxchg|and|or|xor|minsint|maxsint|fmax|fmin|fcmpwr)   {
        TRACE("\n** Atomic Operations ");
        CISAlval.atomic_op = str2atomic_opcode(yytext + 1);
        return ATOMIC_SUB_OP;
    }

not|cbit|fbh|fbl|bfrev {
        TRACE("\n** Unary Logic INST ");
        CISAlval.opcode = str2opcode(yytext);
        return UNARY_LOGIC_OP;
    }

bfe {
      TRACE("\n** Ternary Logic INST ");
      CISAlval.opcode = str2opcode(yytext);
      return TERNARY_LOGIC_OP;
  }

bfi {
      TRACE("\n** Quaternary Logic INST ");
      CISAlval.opcode = str2opcode(yytext);
      return QUATERNARY_LOGIC_OP;
}


inv|log|exp|sqrt|rsqrt|sin|cos|sqrtm {
         TRACE("\n** 2 operands math INST ");
         CISAlval.opcode = str2opcode(yytext);
         return MATH2_OP;
    }

div|mod|pow|divm {
        TRACE("\n** 3 operands math INST ");
        CISAlval.opcode = str2opcode(yytext);
        return MATH3_OP;
    }

frc|lzd|rndd|rndu|rnde|rndz {
        TRACE("\n** Frc INST ");
        CISAlval.opcode = str2opcode(yytext);
        return ARITH2_OP;
    }

add|avg|dp2|dp3|dp4|dph|line|mul|pow|mulh|sad2|plane {
        TRACE("\n** MATH INST ");
        CISAlval.opcode = str2opcode(yytext);
        return ARITH3_OP;
    }

mad|lrp|sad2add {
        TRACE("\n** MATH INST ");
        CISAlval.opcode = str2opcode(yytext);
        return ARITH4_OP;
    }

and|or|xor|shl|shr|asr {
        TRACE("\n** Binary Logic INST ");
        CISAlval.opcode = str2opcode(yytext);
        return BINARY_LOGIC_OP;
    }

rol|ror {
        TRACE("\n** Binary Logic INST ");
        CISAlval.opcode = str2opcode(yytext);
        return BINARY_LOGIC_OP;
    }


addc|subb {
        TRACE("\n** MATH INST ");
        CISAlval.opcode = str2opcode(yytext);
        return ARITH4_OP2;
    }

asin|acos|atan {
        TRACE("\n** ANTI TRIGONOMETRIC INST ");
        CISAlval.opcode = str2opcode(yytext);
        return ANTI_TRIG_OP;
    }

addr_add   {
        TRACE("\n** Addr add INST ");
        CISAlval.opcode = str2opcode(yytext);
        return ADDR_ADD_OP;
    }

sel {
        TRACE("\n** Mod INST ");
        CISAlval.opcode = str2opcode(yytext);
        return SEL_OP;
    }

min {
        TRACE("\n** MIN INST ");
        CISAlval.opcode = ISA_FMINMAX;
        return MIN_OP;
    }

max {
        TRACE("\n** MAX INST ");
        CISAlval.opcode = ISA_FMINMAX;
        return MAX_OP;
    }

mov {
        TRACE("\n** MOV INST ");
        CISAlval.opcode = str2opcode(yytext);
        return MOV_OP;
    }

movs {
        TRACE("\n** MOVS INST ");
        CISAlval.opcode = str2opcode(yytext);
        return MOVS_OP;
    }

setp {
        TRACE("\n** SETP INST ");
        CISAlval.opcode = str2opcode(yytext);
        return SETP_OP;
    }

cmp {
        TRACE("\n** compare INST ");
        CISAlval.opcode = str2opcode(yytext);
        return CMP_OP;
    }

svm_block_ld|svm_block_st|svm_scatter|svm_gather|svm_gather4scaled|svm_scatter4scaled|svm_atomic {
        TRACE("\n** svm INST ");
        /// XXX: Piggyback svm sub-opcode as an opcode.
        if (!strcmp(yytext, "svm_gather4scaled")) {CISAlval.opcode = (ISA_Opcode)SVM_GATHER4SCALED; return SVM_GATHER4SCALED_OP;}
        if (!strcmp(yytext, "svm_scatter4scaled")) {CISAlval.opcode = (ISA_Opcode)SVM_SCATTER4SCALED; return SVM_SCATTER4SCALED_OP;}
        if (!strcmp(yytext, "svm_block_ld")) CISAlval.opcode = (ISA_Opcode)SVM_BLOCK_LD;
        if (!strcmp(yytext, "svm_block_st")) CISAlval.opcode = (ISA_Opcode)SVM_BLOCK_ST;
        if (!strcmp(yytext, "svm_scatter" )) { CISAlval.opcode = (ISA_Opcode)SVM_SCATTER; return SVM_SCATTER_OP; }
        if (!strcmp(yytext, "svm_gather"  )) { CISAlval.opcode = (ISA_Opcode)SVM_GATHER; return SVM_SCATTER_OP; }
        if (!strcmp(yytext, "svm_atomic"  )) { CISAlval.opcode = (ISA_Opcode)SVM_ATOMIC; return SVM_ATOMIC_OP; }
        return SVM_OP;
    }


oword_ld|oword_st|oword_ld_unaligned {
        TRACE("\n** oword_load INST ");
        CISAlval.opcode = str2opcode(yytext);
        return OWORD_OP;
    }


media_ld|media_st {
        TRACE("\n** media INST ");
        CISAlval.opcode = str2opcode(yytext);
        return MEDIA_OP;
    }

gather|scatter {
        TRACE("\n** gather/scatter INST ");
        CISAlval.opcode = str2opcode(yytext);
        return SCATTER_OP;
    }

gather4_typed|scatter4_typed {
        TRACE("\n** gather/scatter typed INST ");
        CISAlval.opcode = str2opcode(yytext);
        return SCATTER_TYPED_OP;
    }

gather_scaled|scatter_scaled {
        TRACE("\n** scaled gather/scatter INST ");
        CISAlval.opcode = str2opcode(yytext);
        return SCATTER_SCALED_OP;
    }

gather4_scaled|scatter4_scaled {
        TRACE("\n** scaled gather/scatter INST ");
        CISAlval.opcode = str2opcode(yytext);
        return SCATTER4_SCALED_OP;
    }

barrier {
        TRACE("\n** barrier INST ");
        CISAlval.opcode = str2opcode(yytext);
        return BARRIER_OP;
    }

sbarrier\.signal {
        TRACE("\n** sbarrier.signal INST ");
        CISAlval.opcode = ISA_SBARRIER;
        return SBARRIER_SIGNAL;
    }

sbarrier\.wait {
        TRACE("\n** sbarrier.wait INST ");
        CISAlval.opcode = ISA_SBARRIER;
        return SBARRIER_WAIT;
    }

sampler_cache_flush {
        TRACE("\n** sampler_cache_flush INST ");
        CISAlval.opcode = str2opcode(yytext);
        return CACHE_FLUSH_OP;
    }

wait {
        TRACE("\n** wait INST ");
        CISAlval.opcode = str2opcode(yytext);
        return WAIT_OP;
    }

fence_global {
        TRACE("\n** fence global INST ");
        CISAlval.opcode = str2opcode("fence");
        return FENCE_GLOBAL_OP;
    }
fence_local {
        TRACE("\n** fence local INST ");
        CISAlval.opcode = str2opcode("fence");
        return FENCE_LOCAL_OP;
    }

fence_sw {
        TRACE("\n** fence SW INST ");
        CISAlval.opcode = str2opcode("fence");
        return FENCE_SW_OP;
    }

yield {
        TRACE("\n** yield INST ");
        CISAlval.opcode = str2opcode(yytext);
        return YIELD_OP;
    }

untyped_atomic {
        TRACE("\n** atomic INST ");
        CISAlval.opcode = str2opcode(yytext);
        return ATOMIC_OP;
    }

dword_atomic {
        TRACE("\n** atomic INST ");
        CISAlval.opcode = str2opcode(yytext);
        return DWORD_ATOMIC_OP;
    }

typed_atomic {
        TRACE("\n** typed atomic INST ");
        CISAlval.opcode = str2opcode(yytext);
        return TYPED_ATOMIC_OP;
    }

sample|load {
        TRACE("\n** sample INST ");
        CISAlval.opcode = str2opcode(yytext);
        return SAMPLE_OP;
    }
sample_unorm {
        TRACE("\n** sample INST ");
        CISAlval.opcode = str2opcode(yytext);
        return SAMPLE_UNORM_OP;
    }

vme_ime {
        TRACE("\n** VME_IME INST ");
        CISAlval.opcode = str2opcode(yytext);
        return VME_IME_OP;
    }
vme_sic {
        TRACE("\n** VME_SIC INST ");
        CISAlval.opcode = str2opcode(yytext);
        return VME_SIC_OP;
    }
vme_fbr {
        TRACE("\n** VME_FBR INST ");
        CISAlval.opcode = str2opcode(yytext);
        return VME_FBR_OP;
    }

jmp|call|ret|fret|fcall|goto {
        TRACE("\n** branch INST ");
        CISAlval.opcode = str2opcode(yytext);
        return BRANCH_OP;
    }

ifcall {
        TRACE("\n** indirect call INST ");
        CISAlval.opcode = ISA_IFCALL;
        return IFCALL;
    }

faddr {
        TRACE("\n** function address INST ");
        CISAlval.opcode = ISA_FADDR;
        return FADDR;
    }

switchjmp {
        TRACE("\n** branch INST ");
        CISAlval.opcode = str2opcode(yytext);
        return SWITCHJMP_OP;
    }

raw_send {
       TRACE("\n** RAW_SEND ");
       CISAlval.opcode = ISA_RAW_SEND;
       return RAW_SEND_STRING;
    }

raw_sendc {
        TRACE("\n** RAW_SENDC ");
        CISAlval.opcode = ISA_RAW_SEND;
        return RAW_SENDC_STRING;
    }

raw_sends {
        TRACE("\n** RAW_SENDS ");
        CISAlval.opcode = ISA_RAW_SENDS;
        return RAW_SENDS_STRING;
    }

raw_sends_eot {
        TRACE("\n** RAW_SENDS_EOT ");
        CISAlval.opcode = ISA_RAW_SENDS;
        return RAW_SENDS_EOT_STRING;
    }

raw_sendsc {
        TRACE("\n** RAW_SENDSC ");
        CISAlval.opcode = ISA_RAW_SENDS;
        return RAW_SENDSC_STRING;
    }

raw_sendsc_eot {
        TRACE("\n** RAW_SENDSC_EOT ");
        CISAlval.opcode = ISA_RAW_SENDS;
        return RAW_SENDSC_EOT_STRING;
    }


avs {
        TRACE("\n** AVS INST ");
        CISAlval.opcode = str2opcode(yytext);
        return AVS_OP;
    }

FILE {
        TRACE("\n** FILE ");
        CISAlval.opcode = str2opcode("file");
        return FILE_OP;
    }

LOC {
        TRACE("\n** LOC ");
        CISAlval.opcode = str2opcode("loc");
        return LOC_OP;
    }

sample_3d|sample_b|sample_l|sample_c|sample_d|sample_b_c|sample_l_c|sample_d_c|sample_lz|sample_c_lz {
        TRACE("\n** SAMPLE_3D ");
        CISAlval.sample3DOp = str2SampleOpcode(yytext);
        return SAMPLE_3D_OP;
    }

load_3d|load_mcs|load_2dms_w|load_lz {
        TRACE("\n** LOAD_3D ");
        CISAlval.sample3DOp = str2SampleOpcode(yytext);
        return LOAD_3D_OP;
    }

sample4|sample4_c|sample4_po|sample4_po_c {
        TRACE("\n** SAMPLE4_3D ");
        CISAlval.sample3DOp = str2SampleOpcode(yytext);
        return SAMPLE4_3D_OP;
    }

resinfo {
        TRACE("\n** RESINFO_3D ");
        CISAlval.opcode = str2opcode("info_3d");
        return RESINFO_OP_3D;
    }

sampleinfo {
        TRACE("\n** SAMPLEINFO_3D ");
        CISAlval.opcode = str2opcode("info_3d");
        return SAMPLEINFO_OP_3D;
    }

rt_write_3d {
        TRACE("\n** RTWRITE_3D ");
        CISAlval.opcode = str2opcode("rt_write_3d");
        return RTWRITE_OP_3D;
    }

urb_write_3d {
        TRACE("\n** URBWRITE_3D ");
        CISAlval.opcode = str2opcode("urb_write_3d");
        return URBWRITE_OP_3D;
    }

lifetime"."start {
        TRACE("\n** Lifetime.start ");
        CISAlval.opcode = str2opcode("lifetime");
        return LIFETIME_START_OP;
    }

lifetime"."end {
        TRACE("\n** Lifetime.end ");
        CISAlval.opcode = str2opcode("lifetime");
        return LIFETIME_END_OP;
    }

^[a-zA-Z_$@?][a-zA-Z0-9_\-$@?]*: {
        TRACE("\n**  LABEL ");
        CISAlval.string = strdup(yytext);
        CISAlval.string[yyleng - 1] = '\0';
        return LABEL;
    }

0x[[:xdigit:]]+ {
        TRACE("\n** HEX NUMBER ");
        CISAlval.number = hexToInt(yytext+2, yyleng-2);
        return HEX_NUMBER;
    }

"."(nomod|modified|top|bottom|top_mod|bottom_mod) {
        TRACE("\n** MEDIA MODE :");
        CISAlval.media_mode = mediaMode(yytext+1);
        return MEDIA_MODE;
    }

AVS_(16|8)_(FULL|DOWN_SAMPLE) {
      TRACE("\n** Output Format Control ");
      CISAlval.cntrl = avs_control(yytext);
      return CNTRL;
    }

AVS_(4|8|16)x(4|8) {
      TRACE("\n** AVS Exec Mode ");
      CISAlval.execMode = avsExecMode(yytext);
      return EXECMODE;
    }

"."mod {
        TRACE("\n** O MODE :");
        CISAlval.oword_mod = true;
        return OWORD_MODIFIER;
    }



[0-9]+         {
        TRACE("\n** NUMBER ");
        CISAlval.number = atoi(yytext);
        return NUMBER;
    }

[0-9]+"."[0-9]+":f" {
        TRACE("\n** FLOAT ");
        CISAlval.fp = atof(yytext);
        return FLOATINGPOINT;
    }

([0-9]+|[0-9]+"."[0-9]+)"e"("+"|"-")[0-9]+":f" {
        TRACE("\n** FLOAT ");
        CISAlval.fp = atof(yytext);
        return FLOATINGPOINT;
    }

[0-9]+"."[0-9]+":df" {
        TRACE("\n** DOUBLE ");
        CISAlval.fp = atof(yytext);
        return DOUBLEFLOAT;
    }

([0-9]+|[0-9]+"."[0-9]+)"e"("+"|"-")[0-9]+":df" {
        TRACE("\n** DOUBLE ");
        CISAlval.fp = atof(yytext);
        return DOUBLEFLOAT;
    }

qSLMSize[ ]*=[ ]* {TRACE("\n** SLM Size "); CISAlval.string = "SLMSize"; return SLM_SIZE;}

qFlagRegNum[ ]*=[ ]* {TRACE("\n** Flag regisetr number "); CISAlval.string = "FlagRegNum"; return FLAG_REG_NAME;}

qSurfaceUsage[ ]*=[ ]* {TRACE("\n** Surface Usage number "); CISAlval.string = "SurfaceUsage"; return SURF_USE_NAME;}

phyReg[ ]*=[ ]* {TRACE("\n** Physical Register "); return PHYSICAL_REGISTER;}

num_elts[ ]*=[ ]* {TRACE("\n** Number of element "); return NUM_ELTS;}

v_name[ ]*=[ ]* {TRACE("\n** V_NAME of element "); return V_NAME_TOKEN;}

align[ ]*=[ ]* {TRACE("\n** Alignment "); return ALIGN;}

offset[ ]*=[ ]* {TRACE("\n** Offset "); return OFFSET;}

size[ ]*=[ ]* {TRACE("\n** Size "); return SIZE;}

alias[ ]*=[ ]* {TRACE("\n** ALIAS "); return ALIAS;}


v_type[ ]*=[ ]*F {
        TRACE("\n** General variable type ");
        CISAlval.string = strdup(yytext);
        return F_CLASS;
    }

v_type[ ]*=[ ]*G {
        TRACE("\n** General variable type ");
        CISAlval.string = strdup(yytext);
        return G_CLASS;
    }

v_type[ ]*=[ ]*A {
        TRACE("\n** Address variable type ");
        CISAlval.string = strdup(yytext);
        return A_CLASS;
    }

v_type[ ]*=[ ]*P {
        TRACE("\n** Predicate variable type ");
        CISAlval.string = strdup(yytext);
        return P_CLASS;
    }

v_type[ ]*=[ ]*S {
        TRACE("\n** Sampler variable type ");
        CISAlval.string = strdup(yytext);
        return S_CLASS;
    }

v_type[ ]*=[ ]*T {
        TRACE("\n** Surface variable type ");
        CISAlval.string = strdup(yytext);
        return T_CLASS;
    }

type[ ]*=[ ]*(ud|d|uw|w|ub|b|df|f|bool|uq|q|UD|D|UW|W|UB|B|DF|F|Bool|BOOL|UQ|Q|hf|HF)  {
        TRACE("\n** TYPE ");
        CISAlval.type = str2type(yytext, yyleng);
        return DECL_DATA_TYPE;
    }

2GRF {
        /* other cases are handled as VAR */
        TRACE("\n** AlignType - 2GRF ");
        CISAlval.align = ALIGN_2_GRF;
        // fprintf(stderr, "%s", "2GRF symbol is deprecated; please use GRFx2");
        return ALIGNTYPE_2GRF;
}

"(-)"    {TRACE("\n** SRCMOD_NEG "); return SRCMOD_NEG;}
"(abs)"  {TRACE("\n** SRCMOD_ABS "); return SRCMOD_ABS;}
"(-abs)" {TRACE("\n** SRCMOD_NEGABS "); return SRCMOD_NEGABS;}
"(~)"    {TRACE("\n** SRCMOD_NOT "); return SRCMOD_NOT;}

".sat"   {TRACE("\n** SAT ");  return SAT;}

".pixel_null_mask" {
        TRACE("\n** PIXEL_NULL_MASK");
        return PIXEL_NULL_MASK;
    }

".cps" {
        TRACE("\n** CPS LOD Compensation enable");
        return CPS;
    }

".divS" {
        TRACE("\n** non-uniform Sampler State");
        return NON_UNIFORM_SAMPLER;
    }


"."(eq|ne|gt|ge|lt|le|EQ|NE|GT|GE|LT|LE) {
        TRACE("\n** COND_MOD ");
        CISAlval.mod = str2cond(yytext+1);
        return COND_MOD;
    }

:(df|DF)  {
        TRACE("\n** DFTYPE ");
        CISAlval.type = str2type(yytext, yyleng);
        return DFTYPE;
    }

:(f|F)      {
        TRACE("\n** FTYPE ");
        CISAlval.type = str2type(yytext, yyleng);
        return FTYPE;
    }

:(hf|HF)  {
        TRACE("\n** HFTYPE ");
        CISAlval.type = str2type(yytext, yyleng);
        return HFTYPE;
    }

:(ud|d|uw|w|ub|b|bool|UD|D|UW|W|UB|B|BOOL|Bool|q|uq|Q|UQ|hf|HF)  {
        TRACE("\n** DATA TYPE ");
        CISAlval.type = str2type(yytext, yyleng);
        return ITYPE;
    }

:(v|vf|V|VF|uv)  {
        TRACE("\n** VTYPE ");
        CISAlval.type = str2type(yytext, yyleng);
        return VTYPE;
    }




"."((R|r)((G|g)?(B|b)?(A|a)?)|(G|g)((B|b)?(A|a)?)|(B|b)((A|a)?)|(A|a))  {
        TRACE("\n** CHANNEL MASK ");
        CISAlval.s_channel = ChannelMask::createFromString(yytext+1).getAPI();
        return SAMPLER_CHANNEL;
    }

"."(16-full|16-downsampled|8-full|8-downsampled) {
        TRACE("\n** OUTPUT_FORMAT ");
        CISAlval.s_channel_output = Get_Channel_Output(yytext+1);
        return CHANNEL_OUTPUT;
    }

"."("<"[a-zA-Z]+">")+ {
        TRACE("\n** RTWRITE OPTION ");
        CISAlval.string = strdup(yytext+1);
        return RTWRITE_OPTION;
    }

"."(0|A)(0|B)(0|G)(0|R) {
        TRACE("\n** SLM CHANNELS ");
        CISAlval.s_channel = ChannelMask::createFromString(yytext+1).getAPI();
        return SLM_CHANNEL;
    }

"."(inter|intra|both) {
        TRACE("\n** VME_TYPE ");
        CISAlval.VME_type = VMEType(yytext+1);
        return VME_TYPE;
    }

"."(any|all) {
        TRACE("\n** PRED_CNTL ");
        CISAlval.string = strdup(yytext+1);
        return PRED_CNTL;
    }


V0 {
        TRACE("\n** NULL VAR ");
        CISAlval.string = strdup(yytext);
        return NULL_VAR;
    }

%[[:alpha:]_][[:alnum:]_]* {
        TRACE("\n** Predefined Var ");
        CISAlval.string = strdup(yytext);
        return VAR;
}

[[:alpha:]_][[:alnum:]_]* {
        TRACE("\n** VAR ");
        CISAlval.string = strdup(yytext);
        return VAR;
    }





[^ \t\n]       {TRACE("\n** SPACE END"); return *yytext;}

"."(E?I?S?C?R?(L1)?)     {
        TRACE("\n** FENCE Options ");

        CISAlval.fence_options = FENCEOptions(yytext+1);
        return FENCE_OPTIONS;
    }

[ \n\t]+"\\"\n {TRACE("\n** Multiple instructions in a line");}

'<EOF>' {
        TRACE("\n** End Of File");
        CISAlval.file_end = true;
        return FILE_EOF;
    }
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

    YY_FATAL_ERROR("Invalid Hex Digit");

    return 0;
}

// convert hex string to int
static int64_t hexToInt(const char *hex_str, int str_len)
{
    if (str_len > 16) { // make sure is within 32 bits
        YY_FATAL_ERROR("Hex number larger than 8 digits");
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

static VISASampler3DSubOpCode str2SampleOpcode(const char *str)
{
    for (int i = 0; i < ISA_NUM_OPCODE; i++)
        if (strcmp(SAMPLE_OP_3D_NAME[i], str) == 0)
            return (VISASampler3DSubOpCode) i;

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

static unsigned char FENCEOptions(const char *str)
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

static COMMON_ISA_VME_OP_MODE VMEType(const char* str)
{
    for (int i = 0; i < VME_OP_MODE_NUM; i++)
        if (!strcmp(vme_op_mode_str[i], str))
            return (COMMON_ISA_VME_OP_MODE)i;

    YY_FATAL_ERROR("Invalid Media Mode");

    return VME_OP_MODE_NUM;
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

    YY_FATAL_ERROR("Invalid channel output format ");
    YY_FATAL_ERROR(str);
    return CHANNEL_16_BIT_FULL;
}

static void appendStringLiteralChar(char c, char *buf, size_t *len)
{
    if (*len == sizeof(CISAlval.strlit)) {
        YY_FATAL_ERROR("string literal too long");
    }
    buf[(*len)++] = c;
    buf[*len] = 0;
}
