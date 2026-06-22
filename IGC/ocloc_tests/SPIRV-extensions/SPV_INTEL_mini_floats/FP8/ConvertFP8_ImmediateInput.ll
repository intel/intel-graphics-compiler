;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; FP8 conversions fed with immediate (compile-time constant) operands.

; REQUIRES: llvm-spirv, cri-supported
; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_fp_conversions,+SPV_KHR_bfloat16,+SPV_EXT_float8 -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" > %t.visaasm 2>&1
; RUN: FileCheck %s -check-prefix=CHECK-HF2E4M3 < %t.visaasm
; RUN: FileCheck %s -check-prefix=CHECK-HF2E5M2 < %t.visaasm
; RUN: FileCheck %s -check-prefix=CHECK-HF2E4M3SAT < %t.visaasm
; RUN: FileCheck %s -check-prefix=CHECK-HF2E5M2SAT < %t.visaasm
; RUN: FileCheck %s -check-prefix=CHECK-E4M32HF < %t.visaasm
; RUN: FileCheck %s -check-prefix=CHECK-E5M22HF < %t.visaasm
; RUN: FileCheck %s -check-prefix=CHECK-BF2E4M3 < %t.visaasm
; RUN: FileCheck %s -check-prefix=CHECK-BF2E5M2 < %t.visaasm
; RUN: FileCheck %s -check-prefix=CHECK-BF2E4M3SAT < %t.visaasm
; RUN: FileCheck %s -check-prefix=CHECK-BF2E5M2SAT < %t.visaasm
; RUN: FileCheck %s -check-prefix=CHECK-E4M32BF < %t.visaasm
; RUN: FileCheck %s -check-prefix=CHECK-E5M22BF < %t.visaasm

; RUN: llvm-spirv --to-text %t.spv -o %t.spt
; RUN: FileCheck < %t.spt %s -check-prefix=CHECK-SPV
; CHECK-SPV: TypeFloat [[HALF:[0-9]+]] 16
; CHECK-SPV: TypeFloat [[E4M3:[0-9]+]] 8 4214
; CHECK-SPV: TypeFloat [[E5M2:[0-9]+]] 8 4215
; CHECK-SPV: TypeFloat [[BFLOAT:[0-9]+]] 16 0
; CHECK-SPV: TypeVector [[V2BFLOAT:[0-9]+]] [[BFLOAT]] 2
; CHECK-SPV: TypeVector [[V2E4M3:[0-9]+]] [[E4M3]] 2
; CHECK-SPV: TypeVector [[V2E5M2:[0-9]+]] [[E5M2]] 2
; CHECK-SPV: FConvert [[E4M3]]
; CHECK-SPV: FConvert [[E5M2]]
; CHECK-SPV: ClampConvertFToFINTEL [[E4M3]]
; CHECK-SPV: ClampConvertFToFINTEL [[E5M2]]
; CHECK-SPV: FConvert [[HALF]]
; CHECK-SPV: FConvert [[HALF]]
; CHECK-SPV: FConvert [[V2E4M3]]
; CHECK-SPV: FConvert [[V2E5M2]]
; CHECK-SPV: ClampConvertFToFINTEL [[V2E4M3]]
; CHECK-SPV: ClampConvertFToFINTEL [[V2E5M2]]
; CHECK-SPV: FConvert [[V2BFLOAT]]
; CHECK-SPV: FConvert [[V2BFLOAT]]

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"
declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)

; CHECK-HF2E4M3-LABEL: .kernel "ConvertFP16ToE4M3_immediate"
; CHECK-HF2E4M3: .decl [[DST:[A-Za-z0-9_]+]] v_type=G type=b num_elts=1
; CHECK-HF2E4M3: .decl [[SRC:[A-Za-z0-9_]+]] v_type=G type=hf num_elts=1
; CHECK-HF2E4M3: mov (M1_NM, 1) [[SRC]](0,0)<1> 0x3c00:hf
; CHECK-HF2E4M3: fcvt (M1_NM, 1) [[DST]](0,0)<1> [[SRC]](0,0)<0;1,0>
declare spir_func signext i8 @_Z36__builtin_spirv_ConvertFP16ToE4M3EXTDh(half)
define spir_kernel void @ConvertFP16ToE4M3_immediate(i8 addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %conv = call spir_func signext i8 @_Z36__builtin_spirv_ConvertFP16ToE4M3EXTDh(half 0xH3C00)
  %arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %outbuf, i64 %call
  store i8 %conv, i8 addrspace(1)* %arrayidx, align 1
  ret void
}

; CHECK-HF2E5M2-LABEL: .kernel "ConvertFP16ToE5M2_immediate"
; CHECK-HF2E5M2: .decl [[DST:[A-Za-z0-9_]+]] v_type=G type=ub num_elts=1
; CHECK-HF2E5M2: .decl [[SRC:[A-Za-z0-9_]+]] v_type=G type=hf num_elts=1
; CHECK-HF2E5M2: mov (M1_NM, 1) [[SRC]](0,0)<1> 0x3c00:hf
; CHECK-HF2E5M2: fcvt (M1_NM, 1) [[DST]](0,0)<1> [[SRC]](0,0)<0;1,0>
declare spir_func signext i8 @_Z36__builtin_spirv_ConvertFP16ToE5M2EXTDh(half)
define spir_kernel void @ConvertFP16ToE5M2_immediate(i8 addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %conv = call spir_func signext i8 @_Z36__builtin_spirv_ConvertFP16ToE5M2EXTDh(half 0xH3C00)
  %arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %outbuf, i64 %call
  store i8 %conv, i8 addrspace(1)* %arrayidx, align 1
  ret void
}

; CHECK-HF2E4M3SAT-LABEL: .kernel "ClampConvertFP16ToE4M3_immediate"
; CHECK-HF2E4M3SAT: .decl [[DST:[A-Za-z0-9_]+]] v_type=G type=b num_elts=1
; CHECK-HF2E4M3SAT: .decl [[SRC:[A-Za-z0-9_]+]] v_type=G type=hf num_elts=1
; CHECK-HF2E4M3SAT: mov (M1_NM, 1) [[SRC]](0,0)<1> 0x3c00:hf
; CHECK-HF2E4M3SAT: fcvt.sat (M1_NM, 1) [[DST]](0,0)<1> [[SRC]](0,0)<0;1,0>
declare spir_func signext i8 @_Z43__builtin_spirv_ClampConvertFP16ToE4M3INTELDh(half)
define spir_kernel void @ClampConvertFP16ToE4M3_immediate(i8 addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %conv = call spir_func signext i8 @_Z43__builtin_spirv_ClampConvertFP16ToE4M3INTELDh(half 0xH3C00)
  %arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %outbuf, i64 %call
  store i8 %conv, i8 addrspace(1)* %arrayidx, align 1
  ret void
}

; CHECK-HF2E5M2SAT-LABEL: .kernel "ClampConvertFP16ToE5M2_immediate"
; CHECK-HF2E5M2SAT: .decl [[DST:[A-Za-z0-9_]+]] v_type=G type=ub num_elts=1
; CHECK-HF2E5M2SAT: .decl [[SRC:[A-Za-z0-9_]+]] v_type=G type=hf num_elts=1
; CHECK-HF2E5M2SAT: mov (M1_NM, 1) [[SRC]](0,0)<1> 0x3c00:hf
; CHECK-HF2E5M2SAT: fcvt.sat (M1_NM, 1) [[DST]](0,0)<1> [[SRC]](0,0)<0;1,0>
declare spir_func signext i8 @_Z43__builtin_spirv_ClampConvertFP16ToE5M2INTELDh(half)
define spir_kernel void @ClampConvertFP16ToE5M2_immediate(i8 addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %conv = call spir_func signext i8 @_Z43__builtin_spirv_ClampConvertFP16ToE5M2INTELDh(half 0xH3C00)
  %arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %outbuf, i64 %call
  store i8 %conv, i8 addrspace(1)* %arrayidx, align 1
  ret void
}

; CHECK-E4M32HF-LABEL: .kernel "ConvertE4M3ToFP16_immediate"
; CHECK-E4M32HF: .decl [[DST:[A-Za-z0-9_]+]] v_type=G type=hf num_elts=1
; CHECK-E4M32HF: .decl [[TMP:[A-Za-z0-9_]+]] v_type=G type=b num_elts=1
; CHECK-E4M32HF: mov (M1_NM, 1) [[TMP]](0,0)<1> 0x38:b
; CHECK-E4M32HF: fcvt (M1_NM, 1) [[DST]](0,0)<1> [[TMP]](0,0)<0;1,0>
declare spir_func half @_Z36__builtin_spirv_ConvertE4M3ToFP16EXTc(i8 signext)
define spir_kernel void @ConvertE4M3ToFP16_immediate(half addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %conv = call spir_func half @_Z36__builtin_spirv_ConvertE4M3ToFP16EXTc(i8 signext 56)
  %arrayidx = getelementptr inbounds half, half addrspace(1)* %outbuf, i64 %call
  store half %conv, half addrspace(1)* %arrayidx, align 2
  ret void
}

; CHECK-E5M22HF-LABEL: .kernel "ConvertE5M2ToFP16_immediate"
; CHECK-E5M22HF: .decl [[DST:[A-Za-z0-9_]+]] v_type=G type=hf num_elts=1
; CHECK-E5M22HF: .decl [[TMP:[A-Za-z0-9_]+]] v_type=G type=b num_elts=1
; CHECK-E5M22HF: .decl [[ALIAS:[A-Za-z0-9_]+]] v_type=G type=ub num_elts=1 alias=<[[TMP]], 0>
; CHECK-E5M22HF: mov (M1_NM, 1) [[TMP]](0,0)<1> 0x3c:b
; CHECK-E5M22HF: fcvt (M1_NM, 1) [[DST]](0,0)<1> [[ALIAS]](0,0)<0;1,0>
declare spir_func half @_Z36__builtin_spirv_ConvertE5M2ToFP16EXTc(i8 signext)
define spir_kernel void @ConvertE5M2ToFP16_immediate(half addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %conv = call spir_func half @_Z36__builtin_spirv_ConvertE5M2ToFP16EXTc(i8 signext 60)
  %arrayidx = getelementptr inbounds half, half addrspace(1)* %outbuf, i64 %call
  store half %conv, half addrspace(1)* %arrayidx, align 2
  ret void
}

; CHECK-BF2E4M3-LABEL: .kernel "ConvertBF16ToE4M3_v2_immediate"
; CHECK-BF2E4M3: .decl [[SRC:[A-Za-z0-9_]+]] v_type=G type=hf num_elts=2
; CHECK-BF2E4M3: .decl [[DST:[A-Za-z0-9_]+]] v_type=G type=b num_elts=2
; CHECK-BF2E4M3: mov (M1_NM, 1) [[IMM:[A-Za-z0-9_]+]](0,0)<1> 0x3f80:w
; CHECK-BF2E4M3: mov (M1_NM, 1) [[IMM]](0,1)<1> 0x4000:w
; CHECK-BF2E4M3: fcvt (M1_NM, 2) [[DST]](0,0)<1> [[SRC]](0,0)<2;2,1>
declare spir_func <2 x i8> @_Z36__builtin_spirv_ConvertBF16ToE4M3EXTDv2_DF16b(<2 x bfloat>)
define spir_kernel void @ConvertBF16ToE4M3_v2_immediate(<2 x i8> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %conv = call spir_func <2 x i8> @_Z36__builtin_spirv_ConvertBF16ToE4M3EXTDv2_DF16b(<2 x bfloat> <bfloat 0xR3F80, bfloat 0xR4000>)
  %arrayidx = getelementptr inbounds <2 x i8>, <2 x i8> addrspace(1)* %outbuf, i64 %call
  store <2 x i8> %conv, <2 x i8> addrspace(1)* %arrayidx, align 2
  ret void
}

; CHECK-BF2E5M2-LABEL: .kernel "ConvertBF16ToE5M2_v2_immediate"
; CHECK-BF2E5M2: .decl [[SRC:[A-Za-z0-9_]+]] v_type=G type=hf num_elts=2
; CHECK-BF2E5M2: .decl [[DST:[A-Za-z0-9_]+]] v_type=G type=ub num_elts=2
; CHECK-BF2E5M2: mov (M1_NM, 1) [[IMM:[A-Za-z0-9_]+]](0,0)<1> 0x3f80:w
; CHECK-BF2E5M2: mov (M1_NM, 1) [[IMM]](0,1)<1> 0x4000:w
; CHECK-BF2E5M2: fcvt (M1_NM, 2) [[DST]](0,0)<1> [[SRC]](0,0)<2;2,1>
declare spir_func <2 x i8> @_Z36__builtin_spirv_ConvertBF16ToE5M2EXTDv2_DF16b(<2 x bfloat>)
define spir_kernel void @ConvertBF16ToE5M2_v2_immediate(<2 x i8> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %conv = call spir_func <2 x i8> @_Z36__builtin_spirv_ConvertBF16ToE5M2EXTDv2_DF16b(<2 x bfloat> <bfloat 0xR3F80, bfloat 0xR4000>)
  %arrayidx = getelementptr inbounds <2 x i8>, <2 x i8> addrspace(1)* %outbuf, i64 %call
  store <2 x i8> %conv, <2 x i8> addrspace(1)* %arrayidx, align 2
  ret void
}

; CHECK-BF2E4M3SAT-LABEL: .kernel "ClampConvertBF16ToE4M3_v2_immediate"
; CHECK-BF2E4M3SAT: .decl [[SRC:[A-Za-z0-9_]+]] v_type=G type=hf num_elts=2
; CHECK-BF2E4M3SAT: .decl [[DST:[A-Za-z0-9_]+]] v_type=G type=b num_elts=2
; CHECK-BF2E4M3SAT: mov (M1_NM, 1) [[IMM:[A-Za-z0-9_]+]](0,0)<1> 0x3f80:w
; CHECK-BF2E4M3SAT: mov (M1_NM, 1) [[IMM]](0,1)<1> 0x4000:w
; CHECK-BF2E4M3SAT: fcvt.sat (M1_NM, 2) [[DST]](0,0)<1> [[SRC]](0,0)<2;2,1>
declare spir_func <2 x i8> @_Z43__builtin_spirv_ClampConvertBF16ToE4M3INTELDv2_DF16b(<2 x bfloat>)
define spir_kernel void @ClampConvertBF16ToE4M3_v2_immediate(<2 x i8> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %conv = call spir_func <2 x i8> @_Z43__builtin_spirv_ClampConvertBF16ToE4M3INTELDv2_DF16b(<2 x bfloat> <bfloat 0xR3F80, bfloat 0xR4000>)
  %arrayidx = getelementptr inbounds <2 x i8>, <2 x i8> addrspace(1)* %outbuf, i64 %call
  store <2 x i8> %conv, <2 x i8> addrspace(1)* %arrayidx, align 2
  ret void
}

; CHECK-BF2E5M2SAT-LABEL: .kernel "ClampConvertBF16ToE5M2_v2_immediate"
; CHECK-BF2E5M2SAT: .decl [[SRC:[A-Za-z0-9_]+]] v_type=G type=hf num_elts=2
; CHECK-BF2E5M2SAT: .decl [[DST:[A-Za-z0-9_]+]] v_type=G type=ub num_elts=2
; CHECK-BF2E5M2SAT: mov (M1_NM, 1) [[IMM:[A-Za-z0-9_]+]](0,0)<1> 0x3f80:w
; CHECK-BF2E5M2SAT: mov (M1_NM, 1) [[IMM]](0,1)<1> 0x4000:w
; CHECK-BF2E5M2SAT: fcvt.sat (M1_NM, 2) [[DST]](0,0)<1> [[SRC]](0,0)<2;2,1>
declare spir_func <2 x i8> @_Z43__builtin_spirv_ClampConvertBF16ToE5M2INTELDv2_DF16b(<2 x bfloat>)
define spir_kernel void @ClampConvertBF16ToE5M2_v2_immediate(<2 x i8> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %conv = call spir_func <2 x i8> @_Z43__builtin_spirv_ClampConvertBF16ToE5M2INTELDv2_DF16b(<2 x bfloat> <bfloat 0xR3F80, bfloat 0xR4000>)
  %arrayidx = getelementptr inbounds <2 x i8>, <2 x i8> addrspace(1)* %outbuf, i64 %call
  store <2 x i8> %conv, <2 x i8> addrspace(1)* %arrayidx, align 2
  ret void
}

; CHECK-E4M32BF-LABEL: .kernel "ConvertE4M3ToBF16_v2_immediate"
; CHECK-E4M32BF: .decl [[DST:[A-Za-z0-9_]+]] v_type=G type=hf num_elts=2
; CHECK-E4M32BF: .decl [[SRC:[A-Za-z0-9_]+]] v_type=G type=b num_elts=2
; CHECK-E4M32BF: mov (M1_NM, 1) [[SRC]](0,0)<1> 0x38:b
; CHECK-E4M32BF: mov (M1_NM, 1) [[SRC]](0,1)<1> 0x3c:b
; CHECK-E4M32BF: fcvt (M1_NM, 2) [[DST]](0,0)<1> [[SRC]](0,0)<2;2,1>
declare spir_func <2 x bfloat> @_Z36__builtin_spirv_ConvertE4M3ToBF16EXTDv2_c(<2 x i8>)
define spir_kernel void @ConvertE4M3ToBF16_v2_immediate(<2 x bfloat> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %conv = call spir_func <2 x bfloat> @_Z36__builtin_spirv_ConvertE4M3ToBF16EXTDv2_c(<2 x i8> <i8 56, i8 60>)
  %arrayidx = getelementptr inbounds <2 x bfloat>, <2 x bfloat> addrspace(1)* %outbuf, i64 %call
  store <2 x bfloat> %conv, <2 x bfloat> addrspace(1)* %arrayidx, align 4
  ret void
}

; CHECK-E5M22BF-LABEL: .kernel "ConvertE5M2ToBF16_v2_immediate"
; CHECK-E5M22BF: .decl [[DST:[A-Za-z0-9_]+]] v_type=G type=hf num_elts=2
; CHECK-E5M22BF: .decl [[IMM:[A-Za-z0-9_]+]] v_type=G type=b num_elts=2
; CHECK-E5M22BF: .decl [[SRC:[A-Za-z0-9_]+]] v_type=G type=ub num_elts=2
; CHECK-E5M22BF: mov (M1_NM, 1) [[IMM]](0,0)<1> 0x38:b
; CHECK-E5M22BF: mov (M1_NM, 1) [[IMM]](0,1)<1> 0x3c:b
; CHECK-E5M22BF: fcvt (M1_NM, 2) [[DST]](0,0)<1> [[SRC]](0,0)<2;2,1>
declare spir_func <2 x bfloat> @_Z36__builtin_spirv_ConvertE5M2ToBF16EXTDv2_c(<2 x i8>)
define spir_kernel void @ConvertE5M2ToBF16_v2_immediate(<2 x bfloat> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %conv = call spir_func <2 x bfloat> @_Z36__builtin_spirv_ConvertE5M2ToBF16EXTDv2_c(<2 x i8> <i8 56, i8 60>)
  %arrayidx = getelementptr inbounds <2 x bfloat>, <2 x bfloat> addrspace(1)* %outbuf, i64 %call
  store <2 x bfloat> %conv, <2 x bfloat> addrspace(1)* %arrayidx, align 4
  ret void
}
