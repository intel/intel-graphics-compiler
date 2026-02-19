;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify --igc-lsc-funcs-translation -platformdg2 -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LSCFuncsResolution
; ------------------------------------------------

; Test checks that lsc builtins are lowered

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_lsc(i32* %base) {
; CHECK-LABEL: @test_lsc(
; CHECK:    [[TMP1:%.*]] = call i32 @llvm.genx.GenISA.LSCLoad.i32.p0i32(i32* [[BASE:%.*]], i32 48, i32 3, i32 1, i32 2)
; CHECK:    [[TMP2:%.*]] = call i32 @llvm.genx.GenISA.LSCLoad.i32.p0i32(i32* [[BASE]], i32 64, i32 3, i32 1, i32 0)
; CHECK:    [[TMP3:%.*]] = call i32 @llvm.genx.GenISA.LSCAtomicInts.i32.p0i32.i32.i32(i32* [[BASE]], i32 8, i32 0, i32 0, i32 2, i32 0)
; CHECK:    call void @llvm.genx.GenISA.LSCStore.p0i32.i32(i32* [[BASE]], i32 0, i32 [[TMP1]], i32 3, i32 1, i32 2)
; CHECK:    call void @llvm.genx.GenISA.LSCStore.p0i32.i32(i32* [[BASE]], i32 32, i32 [[TMP2]], i32 3, i32 1, i32 0)
; CHECK:    call void @llvm.genx.GenISA.LSCPrefetch.p0i32(i32* [[BASE]], i32 96, i32 3, i32 1, i32 2)
; CHECK:    call void @llvm.genx.GenISA.LSCFence(i32 3, i32 0, i32 0)
; CHECK:    ret void
;
  %1 = call i32 @__builtin_IB_lsc_load_global_uint(i32* %base, i32 12, i32 2)
  %2 = call i32 @__builtin_IB_lsc_load_local_uint(i32* %base, i32 16)
  %3 = call i32 @__builtin_IB_lsc_atomic_inc_global_uint(i32* %base, i32 8, i32 0)
  call void @__builtin_IB_lsc_store_global_uint(i32* %base, i32 0, i32 %1, i32 2)
  call void @__builtin_IB_lsc_store_local_uint(i32* %base, i32 8, i32 %2)
  call void @__builtin_IB_lsc_prefetch_global_uint(i32* %base, i32 24, i32 2)
  call void @__builtin_IB_lsc_fence_local()
  ret void
}

declare i32 @__builtin_IB_lsc_load_global_uint(i32*, i32, i32)
declare i32 @__builtin_IB_lsc_load_local_uint(i32*, i32)
declare i32 @__builtin_IB_lsc_atomic_inc_global_uint(i32*, i32, i32)
declare void @__builtin_IB_lsc_store_global_uint(i32*, i32, i32, i32)
declare void @__builtin_IB_lsc_store_local_uint(i32*, i32, i32)
declare void @__builtin_IB_lsc_prefetch_global_uint(i32*, i32, i32)
declare i1 @__builtin_IB_lsc_load_status_global_uint(i32*, i32, i32)
declare void @__builtin_IB_lsc_fence_local()

!igc.functions = !{!0}

!0 = !{void (i32*)* @test_lsc, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
