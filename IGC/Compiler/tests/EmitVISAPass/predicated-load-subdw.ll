;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus, regkeys
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
  ; CHECK: mov (M1_NM, 1) [[G_ALIAS0]](0,0)<0> 0x0:b
  ; CHECK: (P1) lsc_load.ugm (M1_NM, 1)  [[GATHER0]]:d8u32  flat[
  ; CHECK: mov (M1_NM, 1) res0(0,0)<1> [[G_ALIAS0]](0,0)<0;1,0>
  %res0 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1.i8(ptr addrspace(1) %in, i64 1, i1 %p, i8 0)

  ; do predicated load, then do predicated copy of result to merge value which is used as dest.
  ; CHECK: (P1) lsc_load.ugm (M1_NM, 1)  [[GATHER1]]:d8u32  flat[
  ; CHECK: (P1) mov (M1_NM, 1) mVi8(0,0)<1> [[G_ALIAS1]](0,0)<0;1,0>
  %mergeV = add i32 %predicate, 5
  %mVi8 = trunc i32 %mergeV to i8
  %res1 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1.i8(ptr addrspace(1) %in, i64 1, i1 %p, i8 %mVi8)

  ret void
}

declare i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1.i8(ptr addrspace(1), i64, i1, i8)

!IGCMetadata = !{!0}
!igc.functions = !{!3}

!0 = !{!"ModuleMD", !132}
!3 = !{ptr @test, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 0}
!132 = !{!"FuncMD", !133, !134}
!133 = !{!"FuncMDMap[0]", ptr @test}
!134 = !{!"FuncMDValue[0]", !167}
!167 = !{!"resAllocMD", !171}
!171 = !{!"argAllocMDList", !172}
!172 = !{!"argAllocMDListVec[0]", !173}
!173 = !{!"type", i32 0}
