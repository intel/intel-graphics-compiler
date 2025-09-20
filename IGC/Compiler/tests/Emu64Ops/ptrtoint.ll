;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --platformdg2 --igc-emu64ops -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; Emu64Ops
; ------------------------------------------------

; CHECK-LABEL: @test_ptrtoint
; CHECK:  [[TMP1:%[A-z0-9]*]] = inttoptr i32 %offset1 to ptr addrspace(2490368)
; CHECK:  [[TMP2:%[A-z0-9]*]] = call ptr addrspace(1) @llvm.genx.GenISA.ldraw.indexed.p1.p2490368p1(ptr addrspace(2490368) [[TMP1]], i32 %offset2, i32 8, i1 false)
; CHECK:  [[TMP3:%[A-z0-9]*]] = call { i32, i32 } @llvm.genx.GenISA.ptr.to.pair.p1(ptr addrspace(1) [[TMP2]])
; CHECK:  [[TMP4:%[A-z0-9]*]] = extractvalue { i32, i32 } [[TMP3]], 0
; CHECK:  [[TMP5:%[A-z0-9]*]] = extractvalue { i32, i32 } [[TMP3]], 1
; CHECK:  [[TMP6:%[A-z0-9]*]] = insertelement <2 x i32> undef, i32 [[TMP4]], i32 0
; CHECK:  [[TMP7:%[A-z0-9]*]] = insertelement <2 x i32> [[TMP6]], i32 [[TMP5]], i32 1
; CHECK:  [[TMP8:%[A-z0-9]*]] = bitcast <2 x i32> [[TMP7]] to i64

define void @test_ptrtoint(i32 %offset1, i32 %offset2) {
  %1 = inttoptr i32 %offset1 to ptr addrspace(2490368)
  %2 = call ptr addrspace(1) @llvm.genx.GenISA.ldraw.indexed.p1.p2490368p1(ptr addrspace(2490368) %1, i32 %offset2, i32 8, i1 false)
  %3 = ptrtoint ptr addrspace(1) %2 to i64
  call void @use.i64(i64 %3)
  ret void
}

declare ptr addrspace(1) @llvm.genx.GenISA.ldraw.indexed.p1.p2490368p1(ptr addrspace(2490368), i32, i32, i1) #0

declare void @use.i64(i64)

attributes #0 = { mustprogress nofree norecurse nosync nounwind readnone willreturn }

!igc.functions = !{!0}

!0 = !{ptr @test_ptrtoint, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

