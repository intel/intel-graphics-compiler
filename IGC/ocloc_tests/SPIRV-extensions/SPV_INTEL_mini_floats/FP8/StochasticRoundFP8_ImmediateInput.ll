;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, cri-supported
; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_fp_conversions,+SPV_KHR_bfloat16,+SPV_EXT_float8 -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1'" > %t.visaasm 2>&1
; RUN: FileCheck %s -check-prefix=CHECK-HF2E5M2 < %t.visaasm
; RUN: FileCheck %s -check-prefix=CHECK-HF2E4M3 < %t.visaasm
; RUN: FileCheck %s -check-prefix=CHECK-BF2E5M2 < %t.visaasm
; RUN: FileCheck %s -check-prefix=CHECK-BF2E4M3 < %t.visaasm

; Test if conversion opcodes are present in spirv disassembly.
; RUN: llvm-spirv --to-text %t.spv -o %t.spt
; RUN: FileCheck < %t.spt %s -check-prefix=CHECK-SPV
; CHECK-SPV: TypeFloat [[E5M2:[0-9]+]] 8 4215
; CHECK-SPV: TypeFloat [[E4M3:[0-9]+]] 8 4214
; CHECK-SPV: StochasticRoundFToFINTEL [[E5M2]]
; CHECK-SPV: StochasticRoundFToFINTEL [[E4M3]]
; CHECK-SPV: StochasticRoundFToFINTEL [[E5M2]]
; CHECK-SPV: StochasticRoundFToFINTEL [[E4M3]]

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"
declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)

; CHECK-HF2E5M2-LABEL: .kernel "StochasticRoundFP16ToE5M2_immediate"
; CHECK-HF2E5M2-DAG: lfsr.b8v4
; CHECK-HF2E5M2-DAG: mov (M1_NM, 1) [[TMP:[A-Za-z0-9_]+]](0,0)<1> 0x3c00:hf
; CHECK-HF2E5M2-DAG: srnd (M1_NM, 1) {{[A-Za-z0-9_]+}}(0,0)<1> [[TMP]](0,0)<1;1,0> {{[A-Za-z0-9_]+}}(0,0)<1;1,0>
; CHECK-HF2E5M2-DAG: .decl [[TMP]] v_type=G type=hf num_elts=1
declare spir_func signext i8 @_Z46__builtin_spirv_StochasticRoundFP16ToE5M2INTELDhi(half, i32)
define spir_kernel void @StochasticRoundFP16ToE5M2_immediate(i8 addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %conv = call spir_func signext i8 @_Z46__builtin_spirv_StochasticRoundFP16ToE5M2INTELDhi(half 0xH3C00, i32 305419896)
  %arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %outbuf, i64 %call
  store i8 %conv, i8 addrspace(1)* %arrayidx, align 1
  ret void
}

; CHECK-HF2E4M3-LABEL: .kernel "StochasticRoundFP16ToE4M3_immediate"
; CHECK-HF2E4M3-DAG: lfsr.b8v4
; CHECK-HF2E4M3-DAG: mov (M1_NM, 1) [[TMP:[A-Za-z0-9_]+]](0,0)<1> 0x3c00:hf
; CHECK-HF2E4M3-DAG: srnd (M1_NM, 1) {{[A-Za-z0-9_]+}}(0,0)<1> [[TMP]](0,0)<1;1,0> {{[A-Za-z0-9_]+}}(0,0)<1;1,0>
; CHECK-HF2E4M3-DAG: .decl [[TMP]] v_type=G type=hf num_elts=1
declare spir_func signext i8 @_Z46__builtin_spirv_StochasticRoundFP16ToE4M3INTELDhi(half, i32)
define spir_kernel void @StochasticRoundFP16ToE4M3_immediate(i8 addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %conv = call spir_func signext i8 @_Z46__builtin_spirv_StochasticRoundFP16ToE4M3INTELDhi(half 0xH3C00, i32 305419896)
  %arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %outbuf, i64 %call
  store i8 %conv, i8 addrspace(1)* %arrayidx, align 1
  ret void
}

; CHECK-BF2E5M2-LABEL: .kernel "StochasticRoundBF16ToE5M2_immediate"
; CHECK-BF2E5M2-DAG: lfsr.b8v4
; CHECK-BF2E5M2-DAG: mov (M1_NM, 1) [[TMP:[A-Za-z0-9_]+]](0,0)<1> 0x3f80:w
; CHECK-BF2E5M2-DAG: srnd (M1_NM, 1) {{[A-Za-z0-9_]+}}(0,0)<1> [[IN:[A-Za-z0-9_]+]](0,0)<1;1,0> {{[A-Za-z0-9_]+}}(0,0)<1;1,0>
; CHECK-BF2E5M2-DAG: .decl [[IN]] v_type=G type=bf num_elts=1 align=word alias=<[[TMP]], 0>
declare spir_func signext i8 @_Z46__builtin_spirv_StochasticRoundBF16ToE5M2INTELDF16bi(bfloat, i32)
define spir_kernel void @StochasticRoundBF16ToE5M2_immediate(i8 addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %conv = call spir_func signext i8 @_Z46__builtin_spirv_StochasticRoundBF16ToE5M2INTELDF16bi(bfloat 0xR3F80, i32 305419896)
  %arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %outbuf, i64 %call
  store i8 %conv, i8 addrspace(1)* %arrayidx, align 1
  ret void
}

; CHECK-BF2E4M3-LABEL: .kernel "StochasticRoundBF16ToE4M3_immediate"
; CHECK-BF2E4M3-DAG: lfsr.b8v4
; CHECK-BF2E4M3-DAG: mov (M1_NM, 1) [[TMP:[A-Za-z0-9_]+]](0,0)<1> 0x3f80:w
; CHECK-BF2E4M3-DAG: srnd (M1_NM, 1) {{[A-Za-z0-9_]+}}(0,0)<1> [[IN:[A-Za-z0-9_]+]](0,0)<1;1,0> {{[A-Za-z0-9_]+}}(0,0)<1;1,0>
; CHECK-BF2E4M3-DAG: .decl [[IN]] v_type=G type=bf num_elts=1 align=word alias=<[[TMP]], 0>
declare spir_func signext i8 @_Z46__builtin_spirv_StochasticRoundBF16ToE4M3INTELDF16bi(bfloat, i32)
define spir_kernel void @StochasticRoundBF16ToE4M3_immediate(i8 addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %conv = call spir_func signext i8 @_Z46__builtin_spirv_StochasticRoundBF16ToE4M3INTELDF16bi(bfloat 0xR3F80, i32 305419896)
  %arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %outbuf, i64 %call
  store i8 %conv, i8 addrspace(1)* %arrayidx, align 1
  ret void
}
