<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

# vISA instructions (per platform)

| | | | | | |
| --- | --- | --- | --- | --- | --- |
|                    |BDW|SKL|BXT|ICLLP|TGLLP|
|                    |8.0|9.0|9.0|11.0 |12.1 |
|3D_LOAD             | Y | Y | Y | Y   | Y   |
|3D_SAMPLE           | Y | Y | Y | Y   | Y   |
|3D_SAMPLE4          | Y | Y | Y | Y   | Y   |
|ADD                 | Y | Y | Y | Y   | Y   |
|ADD3                |   |   |   |     |     |
|ADD3O               |   |   |   |     |     |
|ADDC                | Y | Y | Y | Y   | Y   |
|ADDR_ADD            | Y | Y | Y | Y   | Y   |
|AND                 | Y | Y | Y | Y   | Y   |
|ASR                 | Y | Y | Y | Y   | Y   |
|AVG                 | Y | Y | Y | Y   | Y   |
|AVS                 | Y | Y | Y | Y   |     |
|BARRIER             | Y | Y | Y | Y   | Y   |
|BFE                 | Y | Y | Y | Y   | Y   |
|BFI                 | Y | Y | Y | Y   | Y   |
|BFN                 |   |   |   |     |     |
|BFREV               | Y | Y | Y | Y   | Y   |
|CACHE_FLUSH         | Y | Y | Y | Y   | Y   |
|CALL                | Y | Y | Y | Y   | Y   |
|CBIT                | Y | Y | Y | Y   | Y   |
|CMP                 | Y | Y | Y | Y   | Y   |
|COS                 | Y | Y | Y | Y   | Y   |
|DIV                 | Y | Y | Y | Y   | Y   |
|DIVM                |   | Y |   |     |     |
|DP4A                |   |   |   |     | Y   |
|DPAS                |   |   |   |     |     |
|DPASW               |   |   |   |     |     |
|DWORD_ATOMIC        | Y | Y | Y | Y   | Y   |
|EXP                 | Y | Y | Y | Y   | Y   |
|FADDR               | Y | Y | Y | Y   | Y   |
|FBH                 | Y | Y | Y | Y   | Y   |
|FBL                 | Y | Y | Y | Y   | Y   |
|FCALL               | Y | Y | Y | Y   | Y   |
|FCVT                |   |   |   |     |     |
|FENCE               | Y | Y | Y | Y   | Y   |
|FILE                | Y | Y | Y | Y   | Y   |
|FRC                 | Y | Y | Y | Y   | Y   |
|FRET                | Y | Y | Y | Y   | Y   |
|GATHER4_SCALED      | Y | Y | Y | Y   | Y   |
|GATHER4_TYPED       | Y | Y | Y | Y   | Y   |
|GATHER_SCALED       | Y | Y | Y | Y   | Y   |
|GOTO                | Y | Y | Y | Y   | Y   |
|IFCALL              | Y | Y | Y | Y   | Y   |
|INFO                | Y | Y | Y | Y   | Y   |
|INV                 | Y | Y | Y | Y   | Y   |
|JMP                 | Y | Y | Y | Y   | Y   |
|LABEL               | Y | Y | Y | Y   | Y   |
|LIFETIME            | Y | Y | Y | Y   | Y   |
|LOC                 | Y | Y | Y | Y   | Y   |
|LOG                 | Y | Y | Y | Y   | Y   |
|LRP                 | Y | Y | Y |     |     |
|LSC_FENCE           |   |   |   |     |     |
|LSC_TYPED           |   |   |   |     |     |
|LSC_UNTYPED         |   |   |   |     |     |
|LZD                 | Y | Y | Y | Y   | Y   |
|MAD                 | Y | Y | Y | Y   | Y   |
|MADW                | Y | Y | Y | Y   | Y   |
|MEDIA_LD            | Y | Y | Y | Y   | Y   |
|MEDIA_ST            | Y | Y | Y | Y   | Y   |
|MIN_MAX             | Y | Y | Y | Y   | Y   |
|MOD                 | Y | Y | Y | Y   | Y   |
|MOV                 | Y | Y | Y | Y   | Y   |
|MOVS                | Y | Y | Y | Y   | Y   |
|MUL                 | Y | Y | Y | Y   | Y   |
|MULH                | Y | Y | Y | Y   | Y   |
|NBARRIER            |   |   |   |     |     |
|NOT                 | Y | Y | Y | Y   | Y   |
|OR                  | Y | Y | Y | Y   | Y   |
|OWORD_LD            | Y | Y | Y | Y   | Y   |
|OWORD_LD_UNALIGNED  | Y | Y | Y | Y   | Y   |
|OWORD_ST            | Y | Y | Y | Y   | Y   |
|PLN                 | Y | Y | Y | Y   |     |
|POW                 | Y | Y | Y | Y   | Y   |
|QW_GATHER           |   |   |   |     |     |
|QW_SCATTER          |   |   |   |     |     |
|RAW_SEND            | Y | Y | Y | Y   | Y   |
|RAW_SENDS           | Y | Y | Y | Y   | Y   |
|RET                 | Y | Y | Y | Y   | Y   |
|RNDD                | Y | Y | Y | Y   | Y   |
|RNDE                | Y | Y | Y | Y   | Y   |
|RNDU                | Y | Y | Y | Y   | Y   |
|RNDZ                | Y | Y | Y | Y   | Y   |
|ROL                 |   |   |   | Y   | Y   |
|ROR                 |   |   |   | Y   | Y   |
|RSQRT               | Y | Y | Y | Y   | Y   |
|RT_WRITE            | Y | Y | Y | Y   | Y   |
|SAD2                | Y | Y | Y |     |     |
|SAD2ADD             | Y | Y | Y |     |     |
|SAMPLE_UNORM        | Y | Y | Y | Y   | Y   |
|SBARRIER            | Y | Y | Y | Y   | Y   |
|SCATTER4_SCALED     | Y | Y | Y | Y   | Y   |
|SCATTER4_TYPED      | Y | Y | Y | Y   | Y   |
|SCATTER_SCALED      | Y | Y | Y | Y   | Y   |
|SEL                 | Y | Y | Y | Y   | Y   |
|SETP                | Y | Y | Y | Y   | Y   |
|SHL                 | Y | Y | Y | Y   | Y   |
|SHR                 | Y | Y | Y | Y   | Y   |
|SIN                 | Y | Y | Y | Y   | Y   |
|SQRT                | Y | Y | Y | Y   | Y   |
|SQRTM               |   | Y |   |     |     |
|SRND                |   |   |   |     |     |
|SUBB                | Y | Y | Y | Y   | Y   |
|SUBROUTINE          | Y | Y | Y | Y   | Y   |
|SVM_ATOMIC          | Y | Y | Y | Y   | Y   |
|SVM_BLOCK_LD        | Y | Y | Y | Y   | Y   |
|SVM_BLOCK_ST        | Y | Y | Y | Y   | Y   |
|SVM_GATHER          | Y | Y | Y | Y   | Y   |
|SVM_GATHER4_SCALED  | Y | Y | Y | Y   | Y   |
|SVM_SCATTER         | Y | Y | Y | Y   | Y   |
|SVM_SCATTER4_SCALED | Y | Y | Y | Y   | Y   |
|SWITCHJMP           | Y | Y | Y | Y   |     |
|TYPED_ATOMIC        | Y | Y | Y | Y   | Y   |
|URB_WRITE           | Y | Y | Y | Y   | Y   |
|VME_FBR             | Y | Y | Y | Y   |     |
|VME_IDM             | Y | Y | Y | Y   |     |
|VME_IME             | Y | Y | Y | Y   |     |
|VME_SIC             | Y | Y | Y | Y   |     |
|WAIT                | Y | Y | Y | Y   | Y   |
|XOR                 | Y | Y | Y | Y   | Y   |
|YIELD               | Y | Y | Y | Y   | Y   |
