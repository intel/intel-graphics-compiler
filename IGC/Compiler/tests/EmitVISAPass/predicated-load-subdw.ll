;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt --opaque-pointers -platformbmg -igc-emit-visa %s -regkey DumpVISAASMToConsole | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; Verifies that predicated loads are emitted correctly for subDW loads

define spir_kernel void @test(ptr addrspace(1) align 1 %in, i32 %predicate) {
entry:
  ; calculated predicate
  %p = icmp slt i32 0, %predicate

  ; CHECK: .decl [[G_ALIAS0:.*]] v_type=G type=b num_elts=4 align=wordx32 alias=<[[GATHER0:.*]], 0>
  ; CHECK: .decl [[G_ALIAS1:.*]] v_type=G type=b num_elts=4 align=wordx32 alias=<[[GATHER1:.*]], 0>

  ; copy merge value. do predicated load, copy result
  ; CHECK: mov (M1_NM, 1) [[G_ALIAS0]](0,0)<1> 0x0:b
  ; CHECK: (P1.any) lsc_load.ugm (M1_NM, 1)  [[GATHER0]]:d8u32  flat[
  ; CHECK: mov (M1_NM, 1) res0(0,0)<1> [[G_ALIAS0]](0,0)<0;1,0>
  %res0 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1.i8(ptr addrspace(1) %in, i64 1, i1 %p, i8 0)

  ; do predicated load, copy result
  ; CHECK: (P1.any) lsc_load.ugm (M1_NM, 1)  [[GATHER1]]:d8u32  flat[
  ; CHECK: mov (M1_NM, 1) {{[A-z0-9_]*}}(0,0)<1> [[G_ALIAS1]](0,0)<0;1,0>
  %mergeV = add i32 %predicate, 5
  %mVi8 = trunc i32 %mergeV to i8
  %res1 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1.i8(ptr addrspace(1) %in, i64 1, i1 %p, i8 %mVi8)
  store i8 %res0, ptr addrspace(1) %in
  %in1 = getelementptr i8, ptr addrspace(1) %in, i32 1
  store i8 %res1, ptr addrspace(1) %in1

  ret void
}


; Uniform address with a divergent predicate: the scalar load has to run if any
; lane is enabled, and each lane selects between the loaded and the merge value.
define spir_kernel void @test_non_uniform_pred(ptr addrspace(1) align 2 %in, ptr addrspace(1) align 2 %out, i16 %localIdX) {
entry:
  %idx = zext i16 %localIdX to i32
  %p = icmp slt i32 0, %idx
  %idx64 = zext i32 %idx to i64

  ; CHECK-LABEL: .kernel "test_non_uniform_pred"
  ; CHECK: .decl [[G_ALIAS2:.*]] v_type=G type=w num_elts=2 align=wordx32 alias=<[[GATHER2:.*]], 0>
  ; CHECK: .decl [[G_ALIAS3:.*]] v_type=G type=w num_elts=2 align=wordx32 alias=<[[GATHER3:.*]], 0>

  ; CHECK: mov (M1, 32) res0(0,0)<1> 0x0:w
  ; CHECK: (P1.any) lsc_load.ugm (M1_NM, 1)  [[GATHER2]]:d16u32  flat[
  ; CHECK: (P1) mov (M1, 32) res0(0,0)<1> [[G_ALIAS2]](0,0)<0;1,0>
  %res0 = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1.i16(ptr addrspace(1) %in, i64 2, i1 %p, i16 0)
  %out0 = getelementptr i16, ptr addrspace(1) %out, i64 %idx64
  store i16 %res0, ptr addrspace(1) %out0, align 2

  %mVi16 = trunc i32 %idx to i16
  ; Merge value is reused as the destination.
  ; CHECK: mov (M1, 32) mVi16(0,0)<1> idx(0,0)<1;1,0>
  ; CHECK: (P1.any) lsc_load.ugm (M1_NM, 1)  [[GATHER3]]:d16u32  flat[
  ; CHECK: (P1) mov (M1, 32) mVi16(0,0)<1> [[G_ALIAS3]](0,0)<0;1,0>
  %res1 = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1.i16(ptr addrspace(1) %in, i64 2, i1 %p, i16 %mVi16)
  %idx1 = add i64 %idx64, 32
  %out1 = getelementptr i16, ptr addrspace(1) %out, i64 %idx1
  store i16 %res1, ptr addrspace(1) %out1, align 2

  ret void
}

declare i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1.i8(ptr addrspace(1), i64, i1, i8)
declare i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1.i16(ptr addrspace(1), i64, i1, i16)

!IGCMetadata = !{!0}
!igc.functions = !{!3, !6}

!0 = !{!"ModuleMD", !132}
!3 = !{ptr @test, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 0}
!6 = !{ptr @test_non_uniform_pred, !4}
!132 = !{!"FuncMD", !133, !134, !135, !136}
!133 = !{!"FuncMDMap[0]", ptr @test}
!134 = !{!"FuncMDValue[0]", !167}
!135 = !{!"FuncMDMap[1]", ptr @test_non_uniform_pred}
!136 = !{!"FuncMDValue[1]", !137, !141}
!137 = !{!"resAllocMD", !138}
!138 = !{!"argAllocMDList", !139, !140, !144}
!139 = !{!"argAllocMDListVec[0]", !173}
!140 = !{!"argAllocMDListVec[1]", !173}
!141 = !{!"implicitArgInfoList", !142}
!142 = !{!"implicitArgInfoListVec[0]", !143}
!143 = !{!"argId", i32 8}
!144 = !{!"argAllocMDListVec[2]", !173}
!167 = !{!"resAllocMD", !171}
!171 = !{!"argAllocMDList", !172}
!172 = !{!"argAllocMDListVec[0]", !173}
!173 = !{!"type", i32 0}
