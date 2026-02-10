;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -LSC-Controls-Analysis-pass -LSC-Cache-Hints  -S < %s | FileCheck %s
; ------------------------------------------------
; LSCControlsAnalysisPass
; ------------------------------------------------

; Test checks that md is updated with cache control

define void @test_store(i32 %a, ptr addrspace(23) %b) {
; CHECK-LABEL: @test_store(
; CHECK-NEXT:    store i32 %a, ptr addrspace(23) %b, align 4, !enable.vmask [[VMASK_MD:![0-9]*]], !lsc.cache.ctrl [[CACHE_MD:![0-9]*]]
; CHECK-NEXT:    ret void
;
  store i32 %a, ptr addrspace(23) %b, align 4
  ret void
}

define void @test_load(ptr addrspace(23) %a) {
; CHECK-LABEL: @test_load(
; CHECK-NEXT:    [[TMP1:%.*]] = load i32, ptr addrspace(23) %a, align 4, !lsc.cache.ctrl [[CACHE_MD]]
; CHECK-NEXT:    ret void
;
  load i32, ptr addrspace(23) %a, align 4
  ret void
}

define void @test_genx_ldraw(ptr addrspace(23) %a) {
; CHECK-LABEL: @test_genx_ldraw(
; CHECK-NEXT:    [[TMP1:%.*]] = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p23(ptr addrspace(23) %a, i32 16, i32 16, i1 false), !lsc.cache.ctrl [[CACHE_MD]]
; CHECK-NEXT:    ret void
;
  call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p23(ptr addrspace(23) %a, i32 16, i32 16, i1 false)
  ret void
}

define void @test_genx_storeraw(ptr addrspace(23) %a) {
; CHECK-LABEL: @test_genx_storeraw(
; CHECK-NEXT:    call void @llvm.genx.GenISA.storeraw.indexed.p23.f32(ptr addrspace(23) %a, i32 13, float 2.000000e+00, i32 13, i1 false), !lsc.cache.ctrl [[CACHE_MD]]
; CHECK-NEXT:    ret void
;
  call void @llvm.genx.GenISA.storeraw.indexed.p23.f32(ptr addrspace(23) %a, i32 13 , float 2.0, i32 13, i1 false)
  ret void
}

define void @test_load123423(ptr addrspace(1) %a, ptr addrspace(2) %b, ptr addrspace(3) %c, ptr addrspace(4) %d, ptr addrspace(23) %e) {
; CHECK-LABEL: @test_load123423(
; CHECK-NEXT:    [[TMPA:%.*]] = load i32, ptr addrspace(1) %a, align 4, !lsc.cache.ctrl [[CACHE_MD1:![0-9]*]]
; CHECK-NEXT:    [[TMPB:%.*]] = load i32, ptr addrspace(2) %b, align 4
; CHECK-NEXT:    [[TMPC:%.*]] = load i32, ptr addrspace(3) %c, align 4, !lsc.cache.ctrl [[CACHE_MD2:![0-9]*]]
; CHECK-NEXT:    [[TMPD:%.*]] = load i32, ptr addrspace(4) %d, align 4
; CHECK-NEXT:    [[TMPE:%.*]] = load i32, ptr addrspace(23) %e, align 4, !lsc.cache.ctrl [[CACHE_MD]]
; CHECK-NEXT:    ret void
  load i32, ptr addrspace(1) %a, align 4
  load i32, ptr addrspace(2) %b, align 4
  load i32, ptr addrspace(3) %c, align 4
  load i32, ptr addrspace(4) %d, align 4
  load i32, ptr addrspace(23) %e, align 4
  ret void
}

declare void @llvm.genx.GenISA.storeraw.indexed.p23.f32(ptr addrspace(23), i32, float, i32, i1)
declare i32 @llvm.genx.GenISA.ldraw.indexed.i32.p23(ptr addrspace(23), i32, i32, i1)

!IGCMetadata = !{!0}
!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"rtInfo", !2}
!2 = !{!"SWStackAddrspace", i32 23}
!3 = !{!"forceLscCacheList", !4, !5, !6, !7}
!4 = !{!"forceLscCacheListMap[0]", i32 3}
!5 = !{!"forceLscCacheListValue[0]", i32 7}
!6 = !{!"forceLscCacheListMap[1]", i32 1}
!7 = !{!"forceLscCacheListValue[1]", i32 9}
