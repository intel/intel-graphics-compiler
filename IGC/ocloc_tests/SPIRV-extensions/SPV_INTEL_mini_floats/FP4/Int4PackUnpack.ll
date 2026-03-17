;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, cri-supported
; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv --spirv-ext=+SPV_INTEL_int4,+SPV_INTEL_float4
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'ForceOCLSIMDWidth=32,DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)
declare dso_local spir_func <3 x i4> @_Z38__builtin_spirv_ConvertFP16ToE2M1INTELDv3_Dh(<3 x half>)

define spir_kernel void @test_shufflevector_uniform(<3 x half> addrspace(1)* %input, <4 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "test_shufflevector_uniform"
; CHECK: dnscl.hftoe2m1.mode0.rne (M1_NM, 1) [[DST:[A-z0-9]*]].0 [[INPUT0:[A-z0-9]*]].0 [[INPUT1:[A-z0-9]*]].0 %null.0
; CHECK-NOT: dnscl.hftoe2m1
; CHECK-DAG: // .decl [[INPUT0]] v_type=G type=ud num_elts=1
; CHECK-DAG: // .decl [[INPUT1]] v_type=G type=ud num_elts=1
; CHECK-DAG: // .decl [[DST]] v_type=G type=ud num_elts=1
  %privateMem = alloca <3 x i4>, align 2

  %inputVal = load <3 x half>, <3 x half> addrspace(1)* %input, align 8
  %conv = call <3 x i4> @_Z38__builtin_spirv_ConvertFP16ToE2M1INTELDv3_Dh(<3 x half> %inputVal)

  ; Test edge case where both unpack and pack use odd number of elements (3)
  %reversed = shufflevector <3 x i4> %conv, <3 x i4> undef, <3 x i32> <i32 2, i32 1, i32 0>
  store <3 x i4> %reversed, <3 x i4>* %privateMem, align 2

  ; Store data in global memory so the kernel doesn't get optimized out
  %loaded = load <3 x i4>, <3 x i4>* %privateMem, align 2
  %finalOutput = shufflevector <3 x i4> %loaded, <3 x i4> zeroinitializer, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  store <4 x i4> %finalOutput, <4 x i4> addrspace(1)* %output, align 2

  ret void
}

define spir_kernel void @test_shufflevector_non_uniform(<3 x half> addrspace(1)* %input, <4 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "test_shufflevector_non_uniform"
; CHECK: dnscl.hftoe2m1.mode0.rne (M1, 32) [[DST:[A-z0-9]*]].0 [[INPUT0:[A-z0-9]*]].0 [[INPUT1:[A-z0-9]*]].0 %null.0
; CHECK-NOT: dnscl.hftoe2m1
; CHECK-DAG: // .decl [[INPUT0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[INPUT1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST]] v_type=G type=ud num_elts=32
  %privateMem = alloca <3 x i4>, align 2

  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <3 x half>, <3 x half> addrspace(1)* %input, i64 %gid
  %outputAddr = getelementptr <4 x i4>, <4 x i4> addrspace(1)* %output, i64 %gid

  %inputVal = load <3 x half>, <3 x half> addrspace(1)* %inputAddr, align 8
  %conv = call <3 x i4> @_Z38__builtin_spirv_ConvertFP16ToE2M1INTELDv3_Dh(<3 x half> %inputVal)

  ; Test edge case where both unpack and pack use odd number of elements (3)
  %reversed = shufflevector <3 x i4> %conv, <3 x i4> undef, <3 x i32> <i32 2, i32 1, i32 0>
  store <3 x i4> %reversed, <3 x i4>* %privateMem, align 2

  ; Store data in global memory so the kernel doesn't get optimized out
  %loaded = load <3 x i4>, <3 x i4>* %privateMem, align 2
  %finalOutput = shufflevector <3 x i4> %loaded, <3 x i4> zeroinitializer, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  store <4 x i4> %finalOutput, <4 x i4> addrspace(1)* %outputAddr, align 2
  ret void
}
