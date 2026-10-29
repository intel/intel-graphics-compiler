;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt %s --opaque-pointers -igc-emit-visa -platformbmg -simd-mode 32 -regkey DumpVISAASMToConsole,AddVISADumpDeclarationsToEnd | FileCheck %s --check-prefixes=CHECK,CHECK-BMG
; RUN: igc_opt %s --opaque-pointers -igc-emit-visa -platformpvc -simd-mode 32 -regkey DumpVISAASMToConsole,AddVISADumpDeclarationsToEnd | FileCheck %s --check-prefixes=CHECK,CHECK-PVC

; load i8 -> zext to i32 -> phi in the same block (single-block loop).
; DeSSA puts the zext and the phi in one variable, so no phi copy is emitted.
; If the zext were remapped to the d8u32 load result, the phi variable would
; never be written. So the load result is copied into the phi variable instead.
;
; %prev:  zext -> phi
; %prevf: zext -> bitcast -> phi
;
; BMG: d8u32 zero-extends the byte, so one mov from the load result to the
; phi variable.
; PVC: d8u32 zero-extension is not guaranteed, so first a narrow byte copy
; (stride <4;1,0>), then a mov into the phi variable.

; CHECK-LABEL: .kernel "test"

; CHECK:           _test_{{[0-9]+}}_loop:
; CHECK:           lsc_store.ugm (M1, 32) {{.*}} [[PREV:[A-Za-z0-9_]+]]:d32
; CHECK:           lsc_store.ugm (M1, 32) {{.*}} [[PREVF:[A-Za-z0-9_]+]]:d32

; CHECK-BMG:       lsc_load.ugm (M1, 32) [[L0:[A-Za-z0-9_]+]]:d8u32
; CHECK-BMG-NEXT:  mov (M1, 32) [[PREV]](0,0)<1> [[L0]](0,0)<1;1,0>
; CHECK-BMG-NOT:   <4;1,0>
; CHECK-BMG:       lsc_load.ugm (M1, 32) [[L1:[A-Za-z0-9_]+]]:d8u32
; CHECK-BMG-NEXT:  mov (M1, 32) [[PREVFA:[A-Za-z0-9_]+]](0,0)<1> [[L1]](0,0)<1;1,0>
; CHECK-BMG-NOT:   <4;1,0>

; CHECK-PVC:       lsc_load.ugm (M1, 32) [[L0:[A-Za-z0-9_]+]]:d8u32
; CHECK-PVC-NEXT:  mov (M1, 32) [[B0:[A-Za-z0-9_]+]](0,0)<1> [[L0A:[A-Za-z0-9_]+]](0,0)<4;1,0>
; CHECK-PVC-NEXT:  mov (M1, 32) [[PREV]](0,0)<1> [[B0A:[A-Za-z0-9_]+]](0,0)<1;1,0>
; CHECK-PVC:       lsc_load.ugm (M1, 32) [[L1:[A-Za-z0-9_]+]]:d8u32
; CHECK-PVC-NEXT:  mov (M1, 32) [[B1:[A-Za-z0-9_]+]](0,0)<1> [[L1A:[A-Za-z0-9_]+]](0,0)<4;1,0>
; CHECK-PVC-NEXT:  mov (M1, 32) [[PREVFA:[A-Za-z0-9_]+]](0,0)<1> [[B1A:[A-Za-z0-9_]+]](0,0)<1;1,0>

; CHECK:           goto {{.*}} _test_{{[0-9]+}}_loop

; CHECK-DAG:       .decl [[PREV]] v_type=G type=d num_elts=32
; CHECK-DAG:       .decl [[PREVF]] v_type=G type=f num_elts=32
; CHECK-DAG:       .decl [[PREVFA]] v_type=G type=d num_elts=32 {{.*}} alias=<[[PREVF]], 0>
; CHECK-PVC-DAG:   .decl [[L0A]] v_type=G type=b {{.*}} alias=<[[L0]], 0>
; CHECK-PVC-DAG:   .decl [[B0A]] v_type=G type=ub {{.*}} alias=<[[B0]], 0>
; CHECK-PVC-DAG:   .decl [[L1A]] v_type=G type=b {{.*}} alias=<[[L1]], 0>
; CHECK-PVC-DAG:   .decl [[B1A]] v_type=G type=ub {{.*}} alias=<[[B1]], 0>

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test(ptr addrspace(1) %src, ptr addrspace(1) %dst, i64 %n, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ) #0 {
entry:
  %lid = zext i16 %localIdX to i64
  br label %loop

loop:
  %i = phi i64 [ 0, %entry ], [ %i.next, %loop ]
  %prev = phi i32 [ 0, %entry ], [ %z0, %loop ]
  %prevf = phi float [ 0.0, %entry ], [ %f1, %loop ]
  ; Last uses of the phis, before the zexts are defined: no interference.
  %off = add i64 %lid, %i
  %doff = shl i64 %off, 3
  %q0 = getelementptr i8, ptr addrspace(1) %dst, i64 %doff
  store i32 %prev, ptr addrspace(1) %q0, align 4
  %q1 = getelementptr i8, ptr addrspace(1) %q0, i64 4
  store float %prevf, ptr addrspace(1) %q1, align 4
  %p0 = getelementptr i8, ptr addrspace(1) %src, i64 %off
  %b0 = load i8, ptr addrspace(1) %p0, align 1
  %z0 = zext i8 %b0 to i32
  %p1 = getelementptr i8, ptr addrspace(1) %p0, i64 64
  %b1 = load i8, ptr addrspace(1) %p1, align 1
  %z1 = zext i8 %b1 to i32
  %f1 = bitcast i32 %z1 to float
  %i.next = add i64 %i, 1
  %cmp = icmp ult i64 %i.next, %n
  br i1 %cmp, label %loop, label %exit

exit:
  ret void
}
attributes #0 = { nounwind null_pointer_is_valid }

!igc.functions = !{!0}
!IGCMetadata = !{!20}

!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

!20 = !{!"ModuleMD", !21}
!21 = !{!"FuncMD", !22, !23}
!22 = !{!"FuncMDMap[0]", ptr @test}
!23 = !{!"FuncMDValue[0]", !24, !60}
!24 = !{!"resAllocMD", !25}
!25 = !{!"argAllocMDList", !26, !27, !28, !29, !30, !31, !32, !33, !34}
!26 = !{!"argAllocMDListVec[0]", !40, !41, !42}
!27 = !{!"argAllocMDListVec[1]", !40, !41, !42}
!28 = !{!"argAllocMDListVec[2]", !40, !41, !42}
!29 = !{!"argAllocMDListVec[3]", !40, !41, !42}
!30 = !{!"argAllocMDListVec[4]", !40, !41, !42}
!31 = !{!"argAllocMDListVec[5]", !40, !41, !42}
!32 = !{!"argAllocMDListVec[6]", !40, !41, !42}
!33 = !{!"argAllocMDListVec[7]", !40, !41, !42}
!34 = !{!"argAllocMDListVec[8]", !40, !41, !42}
!40 = !{!"type", i32 0}
!41 = !{!"extensionType", i32 -1}
!42 = !{!"indexType", i32 -1}
!43 = !{!"argId", i32 0}
!44 = !{!"implicitArgInfoListVec[0]", !43}
!45 = !{!"argId", i32 1}
!46 = !{!"implicitArgInfoListVec[1]", !45}
!47 = !{!"argId", i32 7}
!48 = !{!"implicitArgInfoListVec[2]", !47}
!49 = !{!"argId", i32 8}
!50 = !{!"implicitArgInfoListVec[3]", !49}
!51 = !{!"argId", i32 9}
!52 = !{!"implicitArgInfoListVec[4]", !51}
!53 = !{!"argId", i32 10}
!54 = !{!"implicitArgInfoListVec[5]", !53}
!60 = !{!"implicitArgInfoList", !44, !46, !48, !50, !52, !54}
