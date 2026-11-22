;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; insertelement at a per-lane 64-bit index. EmitPass aliases the index as uw to compute the element's byte offset;
; each lane's low word sits in every fourth word of a 64-bit index, not every second as for a 32-bit one, or lane n
; uses lane n/2's index. The same holds when the index is clamped first.

; REQUIRES: regkeys, bmg-supported

; RUN: llvm-as %OPAQUE_PTR_FLAG% < %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device bmg -options "-igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-G1"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: .kernel "insert_i64_index"
; CHECK: mul (M1, 16) {{V[0-9]+}}(0,0)<1> {{V[0-9]+}}(0,0)<32;8,4> 0x40:uw

define spir_kernel void @insert_i64_index(ptr addrspace(1) %dst) {
entry:
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %idx = and i64 %gid, 3
  %val = uitofp i64 %gid to float
  %vec = insertelement <4 x float> <float -1.0, float -1.0, float -1.0, float -1.0>, float %val, i64 %idx
  %p = getelementptr inbounds <4 x float>, ptr addrspace(1) %dst, i64 %gid
  store <4 x float> %vec, ptr addrspace(1) %p, align 16
  ret void
}

declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)

; CHECK-LABEL: .kernel "insert_i64_index_clamped"
; CHECK: and (M1, 16) {{[A-Za-z0-9_]+}}(0,0)<1> {{[A-Za-z0-9_]+}}(0,0)<4;1,0> 0x3:uw

define spir_kernel void @insert_i64_index_clamped(ptr addrspace(1) %dst, ptr addrspace(1) %idxs) {
entry:
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %ip = getelementptr inbounds i64, ptr addrspace(1) %idxs, i64 %gid
  %idx = load i64, ptr addrspace(1) %ip, align 8
  %val = uitofp i64 %gid to float
  %vec = insertelement <4 x float> <float -1.0, float -1.0, float -1.0, float -1.0>, float %val, i64 %idx
  %p = getelementptr inbounds <4 x float>, ptr addrspace(1) %dst, i64 %gid
  store <4 x float> %vec, ptr addrspace(1) %p, align 16
  ret void
}
