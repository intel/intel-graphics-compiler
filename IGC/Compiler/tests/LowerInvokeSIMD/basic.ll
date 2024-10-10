;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -enable-debugify -igc-lower-invoke-simd -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LowerInvokeSIMD
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test(ptr addrspace(1) %src, ptr addrspace(1) %src1) {
; CHECK-LABEL: @test(
; CHECK:  entry:
; CHECK:    [[TMP0:%.*]] = addrspacecast ptr addrspace(1) [[SRC:%.*]] to ptr addrspace(4)
; CHECK:    [[TMP1:%.*]] = call ptr addrspace(4) @llvm.genx.GenISA.WaveShuffleIndex.p4(ptr addrspace(4) [[TMP0]], i32 0, i32 0)
; CHECK:    [[TMP2:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 14, i32 0, i32 0)
; CHECK:    [[TMP3:%.*]] = call spir_func i32 @_Z23__regcall3(ptr addrspace(4) [[TMP1]], i32 13, i32 [[TMP2]])
; CHECK:    store i32 [[TMP3]], ptr addrspace(1) [[SRC1:%.*]], align 4
; CHECK:    ret void
;
entry:
  %0 = addrspacecast ptr addrspace(1) %src to ptr addrspace(4)
  %1 = call spir_func i32 @_Z21__builtin_invoke_simd(ptr @_Z23__regcall3, ptr addrspace(4) %0, i32 13, i32 14)
  store i32 %1, ptr addrspace(1) %src1, align 4
  ret void
}

declare spir_func i32 @_Z21__builtin_invoke_simd(ptr, ptr addrspace(4), i32, i32)
declare spir_func <16 x i32> @_Z23__regcall3(ptr addrspace(4), <16 x i32>, i32)
