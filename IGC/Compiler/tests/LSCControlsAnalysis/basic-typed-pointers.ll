;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -LSC-Controls-Analysis-pass  -S < %s | FileCheck %s
; ------------------------------------------------
; LSCControlsAnalysisPass
; ------------------------------------------------

; Test checks that md is updated with cache control

define void @test_store(i32 %a, i32 addrspace(23)* %b) {
; CHECK-LABEL: @test_store(
; CHECK-NEXT:    store i32 %a, i32 addrspace(23)* %b, align 4, !enable.vmask [[VMASK_MD:![0-9]*]], !lsc.cache.ctrl [[CACHE_MD:![0-9]*]]
; CHECK-NEXT:    ret void
;
  store i32 %a, i32 addrspace(23)* %b, align 4
  ret void
}

define void @test_load(i32 addrspace(23)* %a) {
; CHECK-LABEL: @test_load(
; CHECK-NEXT:    [[TMP1:%.*]] = load i32, i32 addrspace(23)* %a, align 4, !lsc.cache.ctrl [[CACHE_MD]]
; CHECK-NEXT:    ret void
;
  load i32, i32 addrspace(23)* %a, align 4
  ret void
}

define void @test_genx_ldraw(i32 addrspace(23)* %a) {
; CHECK-LABEL: @test_genx_ldraw(
; CHECK-NEXT:    [[TMP1:%.*]] = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p23i32(i32 addrspace(23)* %a, i32 16, i32 16, i1 false), !lsc.cache.ctrl [[CACHE_MD]]
; CHECK-NEXT:    ret void
;
  call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p23i32(i32 addrspace(23)* %a, i32 16, i32 16, i1 false)
  ret void
}

define void @test_genx_storeraw(i32 addrspace(23)* %a) {
; CHECK-LABEL: @test_genx_storeraw(
; CHECK-NEXT:    call void @llvm.genx.GenISA.storeraw.indexed.p23i32.f32(i32 addrspace(23)* %a, i32 13, float 2.000000e+00, i32 13, i1 false), !lsc.cache.ctrl [[CACHE_MD]]
; CHECK-NEXT:    ret void
;
  call void @llvm.genx.GenISA.storeraw.indexed.p23i32.f32(i32 addrspace(23)* %a, i32 13 , float 2.0, i32 13, i1 false)
  ret void
}


declare void @llvm.genx.GenISA.storeraw.indexed.p23i32.f32(i32 addrspace(23)*, i32, float, i32, i1)
declare i32 @llvm.genx.GenISA.ldraw.indexed.i32.p23i32(i32 addrspace(23)*, i32, i32, i1)

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"rtInfo", !2}
!2 = !{!"SWStackAddrspace", i32 23}
