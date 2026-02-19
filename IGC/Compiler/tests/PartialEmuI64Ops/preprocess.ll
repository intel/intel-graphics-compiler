;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; run: igc_opt --platformdg2 --enable-debugify --igc-PartialEmuI64Ops -S < %s 2>&1 | FileCheck %s
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --platformdg2 --igc-PartialEmuI64Ops -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PartialEmuI64Ops : preprocess part
; ------------------------------------------------

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

; Fails on debug
; COM: check-not WARNING
; COM: check CheckModuleDebugify: PASS

define void @test_uadd(i64 %src1, i64 %src2) {
; CHECK-LABEL: @test_uadd(
; CHECK:    [[TMP1:%.*]] = bitcast i64 %src1 to <2 x i32>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%.*]] = bitcast i64 %src2 to <2 x i32>
; CHECK:    [[TMP5:%.*]] = extractelement <2 x i32> [[TMP4]], i32 0
; CHECK:    [[TMP6:%.*]] = extractelement <2 x i32> [[TMP4]], i32 1
; CHECK:    [[TMP7:%.*]] = call { i32, i32 } @llvm.genx.GenISA.add.pair(i32 [[TMP2]], i32 [[TMP3]], i32 [[TMP5]], i32 [[TMP6]])
; CHECK:    [[TMP8:%.*]] = extractvalue { i32, i32 } [[TMP7]], 0
; CHECK:    [[TMP9:%.*]] = extractvalue { i32, i32 } [[TMP7]], 1
; CHECK:    [[TMP10:%.*]] = insertelement <2 x i32> undef, i32 [[TMP8]], i32 0
; CHECK:    [[TMP11:%.*]] = insertelement <2 x i32> [[TMP10]], i32 [[TMP9]], i32 1
; CHECK:    [[TMP12:%.*]] = bitcast <2 x i32> [[TMP11]] to i64
; CHECK:    [[TMP13:%.*]] = icmp ult i32 [[TMP8]], [[TMP2]]
; CHECK:    [[TMP14:%.*]] = icmp eq i32 [[TMP9]], [[TMP3]]
; CHECK:    [[TMP15:%.*]] = and i1 [[TMP14]], [[TMP13]]
; CHECK:    [[TMP16:%.*]] = icmp ult i32 [[TMP9]], [[TMP3]]
; CHECK:    [[TMP17:%.*]] = or i1 [[TMP15]], [[TMP16]]
; CHECK:    call void @use.i1(i1 [[TMP17]])
; CHECK:    call void @use.i64(i64 [[TMP12]])
; CHECK:    ret void
;
  %1 = call { i64, i1 } @llvm.uadd.with.overflow.i64(i64 %src1, i64 %src2)
  %2 = extractvalue { i64, i1 } %1, 1
  %3 = extractvalue { i64, i1 } %1, 0
  call void @use.i1(i1 %2)
  call void @use.i64(i64 %3)
  ret void
}

declare void @use.i64(i64)
declare void @use.i1(i1)


define void @test_load(i64 addrspace(1)** %src) {
; CHECK-LABEL: @test_load(
; CHECK:    [[TMP1:%.*]] = bitcast i64 addrspace(1)** %src to i64*
; CHECK:    [[TMP2:%.*]] = load i64, i64* [[TMP1]], align 8
; CHECK:    [[TMP3:%.*]] = inttoptr i64 [[TMP2]] to i64 addrspace(1)*
; CHECK:    call void @use.p64(i64 addrspace(1)* [[TMP3]])
; CHECK:    ret void
;
  %1 = load i64 addrspace(1)*, i64 addrspace(1)** %src
  call void @use.p64(i64 addrspace(1)* %1)
  ret void
}
declare void @use.p64(i64 addrspace(1)*)

define void @test_cmp(i64 addrspace(1)* %src1, i64 addrspace(1)* %src2) {
; CHECK-LABEL: @test_cmp(
; CHECK:    [[TMP1:%.*]] = ptrtoint i64 addrspace(1)* %src1 to i64
; CHECK:    [[TMP2:%.*]] = call { i32, i32 } @llvm.genx.GenISA.ptr.to.pair.p1i64(i64 addrspace(1)* %src1)
; CHECK:    [[TMP3:%.*]] = extractvalue { i32, i32 } [[TMP2]], 0
; CHECK:    [[TMP4:%.*]] = extractvalue { i32, i32 } [[TMP2]], 1
; CHECK:    [[TMP5:%.*]] = ptrtoint i64 addrspace(1)* %src2 to i64
; CHECK:    [[TMP6:%.*]] = call { i32, i32 } @llvm.genx.GenISA.ptr.to.pair.p1i64(i64 addrspace(1)* %src2)
; CHECK:    [[TMP7:%.*]] = extractvalue { i32, i32 } [[TMP6]], 0
; CHECK:    [[TMP8:%.*]] = extractvalue { i32, i32 } [[TMP6]], 1
; CHECK:    [[TMP9:%.*]] = icmp eq i32 [[TMP3]], [[TMP7]]
; CHECK:    [[TMP10:%.*]] = icmp eq i32 [[TMP4]], [[TMP8]]
; CHECK:    [[TMP11:%.*]] = and i1 [[TMP10]], [[TMP9]]
; CHECK:    call void @use.i1(i1 [[TMP11]])
; CHECK:    ret void
;
  %1 = icmp eq i64 addrspace(1)* %src1, %src2
  call void @use.i1(i1 %1)
  ret void
}

define void @test_select(i64 addrspace(1)* %src1, i64 addrspace(1)* %src2, i1 %cond) {
; CHECK-LABEL: @test_select(
; CHECK:    [[TMP1:%.*]] = ptrtoint i64 addrspace(1)* %src1 to i64
; CHECK:    [[TMP2:%.*]] = call { i32, i32 } @llvm.genx.GenISA.ptr.to.pair.p1i64(i64 addrspace(1)* %src1)
; CHECK:    [[TMP3:%.*]] = extractvalue { i32, i32 } [[TMP2]], 0
; CHECK:    [[TMP4:%.*]] = extractvalue { i32, i32 } [[TMP2]], 1
; CHECK:    [[TMP5:%.*]] = ptrtoint i64 addrspace(1)* %src2 to i64
; CHECK:    [[TMP6:%.*]] = call { i32, i32 } @llvm.genx.GenISA.ptr.to.pair.p1i64(i64 addrspace(1)* %src2)
; CHECK:    [[TMP7:%.*]] = extractvalue { i32, i32 } [[TMP6]], 0
; CHECK:    [[TMP8:%.*]] = extractvalue { i32, i32 } [[TMP6]], 1
; CHECK:    [[TMP9:%.*]] = select i1 [[COND:%.*]], i32 [[TMP3]], i32 [[TMP7]]
; CHECK:    [[TMP10:%.*]] = select i1 [[COND]], i32 [[TMP4]], i32 [[TMP8]]
; CHECK:    [[TMP11:%.*]] = insertelement <2 x i32> undef, i32 [[TMP9]], i32 0
; CHECK:    [[TMP12:%.*]] = insertelement <2 x i32> [[TMP11]], i32 [[TMP10]], i32 1
; CHECK:    [[TMP13:%.*]] = bitcast <2 x i32> [[TMP12]] to i64
; CHECK:    [[TMP14:%.*]] = inttoptr i64 [[TMP13]] to i64 addrspace(1)*
; CHECK:    call void @use.p64(i64 addrspace(1)* [[TMP14]])
; CHECK:    ret void
;
  %1 = select i1 %cond, i64 addrspace(1)* %src1, i64 addrspace(1)* %src2
  call void @use.p64(i64 addrspace(1)* %1)
  ret void
}

define void @test_ptrtoint(i64 addrspace(1)* %src) {
; CHECK-LABEL: @test_ptrtoint(
; CHECK:    [[TMP1:%.*]] = ptrtoint i64 addrspace(1)* %src to i64
; CHECK:    call void @use.i64(i64 [[TMP1]])
; CHECK:    ret void
;
  %1 = ptrtoint i64 addrspace(1)* %src to i64
  call void @use.i64(i64 %1)
  ret void
}

define void @test_inttoptr(i64 %src) {
; CHECK-LABEL: @test_inttoptr(
; CHECK:    [[TMP1:%.*]] = inttoptr i64 %src to i64 addrspace(1)*
; CHECK:    call void @use.p64(i64 addrspace(1)* [[TMP1]])
; CHECK:    ret void
;
  %1 = inttoptr i64 %src to i64 addrspace(1)*
  call void @use.p64(i64 addrspace(1)* %1)
  ret void
}

; Function Attrs: nounwind readnone speculatable
declare { i64, i1 } @llvm.uadd.with.overflow.i64(i64, i64) #0


define void @test_store(i64 addrspace(1)* %src1, i64 addrspace(1)** %src2) {
; CHECK-LABEL: @test_store(
; CHECK-NEXT:    [[TMP1:%.*]] = bitcast i64 addrspace(1)** [[SRC2:%.*]] to i64*
; CHECK-NEXT:    [[TMP2:%.*]] = ptrtoint i64 addrspace(1)* [[SRC1:%.*]] to i64
; CHECK-NEXT:    store i64 [[TMP2]], i64* [[TMP1]], align 8
; CHECK-NEXT:    ret void
;
  store i64 addrspace(1)* %src1, i64 addrspace(1)** %src2, align 8
  ret void
}



!igc.functions = !{!0, !3, !4, !5, !6, !7, !8}

!0 = !{void (i64, i64)* @test_uadd, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{void (i64 addrspace(1)**)* @test_load, !1}
!4 = !{void (i64 addrspace(1)*, i64 addrspace(1)*)* @test_cmp, !1}
!5 = !{void (i64 addrspace(1)*, i64 addrspace(1)*, i1)* @test_select, !1}
!6 = !{void (i64 addrspace(1)*)* @test_ptrtoint, !1}
!7 = !{void (i64)* @test_inttoptr, !1}
!8 = !{void (i64 addrspace(1)*, i64 addrspace(1)**)*  @test_store, !1}
