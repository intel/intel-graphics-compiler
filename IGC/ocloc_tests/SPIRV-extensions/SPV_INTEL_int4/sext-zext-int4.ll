;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; sext/zext i4 are lowered to GenISA.Int4VectorUnpack, which emits shl+asr / asr
; on bytes for sext and and / shr for zext. asr only replicates the sign bit on
; a signed type, so the sext operands must be :b; on :ub the result would be
; zero-extended.

; REQUIRES: llvm-spirv, cri-supported
; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv --spirv-ext=+SPV_INTEL_int4
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'ForceOCLSIMDWidth=16,DumpASMToConsole=1'" | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)

; CHECK-LABEL: //.kernel test_sext
; CHECK-NOT: {{(shr|asr) .*:ub}}
; CHECK-DAG: shl (16|M0) {{r[0-9.]+<[0-9]+>}}:b {{r[0-9.]+<1;1,0>}}:b 0x4:uw
; CHECK-DAG: asr (16|M0) {{r[0-9.]+<[0-9]+>}}:b {{r[0-9.]+<1;1,0>}}:b 0x4:uw
; CHECK-DAG: asr (16|M0) {{r[0-9.]+<[0-9]+>}}:b {{r[0-9.]+<1;1,0>}}:b 0x4:uw
; CHECK-NOT: {{(shr|asr) .*:ub}}
define spir_kernel void @test_sext(<2 x i4> addrspace(1)* %input, <2 x i32> addrspace(1)* %output) {
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <2 x i4>, <2 x i4> addrspace(1)* %input, i64 %gid
  %outputAddr = getelementptr <2 x i32>, <2 x i32> addrspace(1)* %output, i64 %gid
  %v = load <2 x i4>, <2 x i4> addrspace(1)* %inputAddr, align 1
  %s = sext <2 x i4> %v to <2 x i32>
  store <2 x i32> %s, <2 x i32> addrspace(1)* %outputAddr, align 8
  ret void
}

; CHECK-LABEL: //.kernel test_zext
; CHECK-NOT: asr
; CHECK-DAG: and (16|M0) {{r[0-9.]+<[0-9]+>}}:ub {{r[0-9.]+<1;1,0>}}:ub 0xF:uw
; CHECK-DAG: shr (16|M0) {{r[0-9.]+<[0-9]+>}}:ub {{r[0-9.]+<1;1,0>}}:ub 0x4:uw
; CHECK-NOT: asr
define spir_kernel void @test_zext(<2 x i4> addrspace(1)* %input, <2 x i32> addrspace(1)* %output) {
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <2 x i4>, <2 x i4> addrspace(1)* %input, i64 %gid
  %outputAddr = getelementptr <2 x i32>, <2 x i32> addrspace(1)* %output, i64 %gid
  %v = load <2 x i4>, <2 x i4> addrspace(1)* %inputAddr, align 1
  %z = zext <2 x i4> %v to <2 x i32>
  store <2 x i32> %z, <2 x i32> addrspace(1)* %outputAddr, align 8
  ret void
}
