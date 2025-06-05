;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --typed-pointers -debugify -prepare-loads-stores -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PrepareLoadsStoresPass
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_load_i64(i64 addrspace(1)* %a) {
; CHECK-LABEL: @test_load_i64(
; CHECK:    [[TMP1:%.*]] = bitcast i64 addrspace(1)* %a to <2 x i32> addrspace(1)*
; CHECK:    [[TMP2:%.*]] = call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p1v2i32.v2i32(<2 x i32> addrspace(1)* [[TMP1]], i64 8, i1 true, <2 x i32> bitcast (<1 x i64> <i64 2> to <2 x i32>))
; CHECK:    [[TMP3:%.*]] = bitcast <2 x i32> [[TMP2]] to i64
; CHECK:    call void @use.i64(i64 [[TMP3]])
; CHECK:    ret void
;
  %1 = call i64 @llvm.genx.GenISA.PredicatedLoad.i64.p1i64.i64(i64 addrspace(1)* %a, i64 8, i1 true, i64 2)
  call void @use.i64(i64 %1)
  ret void
}

define void @test_load_v2i64(<2 x i64> addrspace(1)* %a) {
; CHECK-LABEL: @test_load_v2i64(
; CHECK:    [[TMP1:%.*]] = bitcast <2 x i64> addrspace(1)* %a to <4 x i32> addrspace(1)*
; CHECK:    [[TMP2:%.*]] = call <4 x i32> @llvm.genx.GenISA.PredicatedLoad.v4i32.p1v4i32.v4i32(<4 x i32> addrspace(1)* [[TMP1]], i64 8, i1 true, <4 x i32> bitcast (<2 x i64> <i64 3, i64 4> to <4 x i32>))
; CHECK:    [[TMP3:%.*]] = bitcast <4 x i32> [[TMP2]] to <2 x i64>
; CHECK:    call void @use.v2i64(<2 x i64> [[TMP3]])
; CHECK:    ret void
;
  %1 = call <2 x i64> @llvm.genx.GenISA.PredicatedLoad.v2i64.p1v2i64.v2i64(<2 x i64> addrspace(1)* %a, i64 8, i1 true, <2 x i64> <i64 3, i64 4>)
  call void @use.v2i64(<2 x i64> %1)
  ret void
}

declare void @use.i64(i64)
declare void @use.v2i64(<2 x i64>)

; Function Attrs: nounwind readonly
declare i64 @llvm.genx.GenISA.PredicatedLoad.i64.p1i64.i64(i64 addrspace(1)*, i64, i1, i64) #0
; Function Attrs: nounwind readonly
declare <2 x i64> @llvm.genx.GenISA.PredicatedLoad.v2i64.p1v2i64.v2i64(<2 x i64> addrspace(1)*, i64, i1, <2 x i64>) #0

attributes #0 = { nounwind readonly }

define void @test_store_i64(i64 addrspace(1)* %a, i64 %b) {
; CHECK-LABEL: @test_store_i64(
; CHECK:    [[TMP1:%.*]] = bitcast i64 %b to <2 x i32>
; CHECK:    [[TMP2:%.*]] = bitcast i64 addrspace(1)* %a to <2 x i32> addrspace(1)*
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p1v2i32.v2i32(<2 x i32> addrspace(1)* [[TMP2]], <2 x i32> [[TMP1]], i64 8, i1 true)
; CHECK:    ret void
;
  call void @llvm.genx.GenISA.PredicatedStore.p1i64.i64(i64 addrspace(1)* %a, i64 %b, i64 8, i1 true)
  ret void
}

define void @test_store_v2i64(<2 x i64> addrspace(1)* %a, <2 x i64> %b) {
; CHECK-LABEL: @test_store_v2i64(
; CHECK:    [[TMP1:%.*]] = bitcast <2 x i64> %b to <4 x i32>
; CHECK:    [[TMP2:%.*]] = bitcast <2 x i64> addrspace(1)* %a to <4 x i32> addrspace(1)*
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p1v4i32.v4i32(<4 x i32> addrspace(1)* [[TMP2]], <4 x i32> [[TMP1]], i64 8, i1 true)
; CHECK:    ret void
;
  call void @llvm.genx.GenISA.PredicatedStore.p1v2i64.v2i64(<2 x i64> addrspace(1)* %a, <2 x i64> %b, i64 8, i1 true)
  ret void
}

declare void @llvm.genx.GenISA.PredicatedStore.p1i64.i64(i64 addrspace(1)*, i64, i64, i1)
declare void @llvm.genx.GenISA.PredicatedStore.p1v2i64.v2i64(<2 x i64> addrspace(1)*, <2 x i64>, i64, i1)

; Verify that no transformations are applied to the aggregate type
define void @test_load_aggregate({i64, i64} addrspace(1)* %a) {
; CHECK-LABEL: @test_load_aggregate
; CHECK:    call { i64, i64 } @llvm.genx.GenISA.PredicatedLoad.s.p1s.s({ i64, i64 } addrspace(1)* %a, i64 8, i1 true, { i64, i64 } { i64 3, i64 4 })
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p1s.s({ i64, i64 } addrspace(1)* %a, { i64, i64 }
; CHECK:    ret void
;
  %1 = call { i64, i64 } @llvm.genx.GenISA.PredicatedLoad.s.p1s.s({ i64, i64 } addrspace(1)* %a, i64 8, i1 true, { i64, i64 } { i64 3, i64 4 })
  call void @llvm.genx.GenISA.PredicatedStore.p1s.s({ i64, i64 } addrspace(1)* %a, { i64, i64 } %1, i64 8, i1 true)
  ret void
}

declare { i64, i64 } @llvm.genx.GenISA.PredicatedLoad.s.p1s.s({ i64, i64 } addrspace(1)*, i64, i1, { i64, i64 }) #0
declare void @llvm.genx.GenISA.PredicatedStore.p1s.s({ i64, i64 } addrspace(1)*, { i64, i64 }, i64, i1)
