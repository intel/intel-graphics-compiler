;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify  -igc-resolve-inline-locals -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; InlineLocalsResolution
; ------------------------------------------------
; Reduced from OCL kernel, with changed global variable @a addrspace:
;
; static __global int* a;
; static __global int* b;
; __kernel void test_inline(global int * table)
; {
;  int wg = work_group_any(table[0]);
;  a[0] = wg;
;  b[0] = wg;
; }
;
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

; CHECK: @a = {{.*}}, section "localSLM",
@a = external dso_local addrspace(3) global i32 addrspace(3)*, align 8

define spir_kernel void @test_inline() {
entry:
; CHECK-LABEL: @test_inline(
; CHECK:    [[WG:%.*]] = alloca i32, align 4
; CHECK:    [[MEMPOOLCAST:%.*]] = bitcast [4 x i8] addrspace(3)* @GenSLM.LocalMemPoolOnGetMemPoolPtr to i8 addrspace(3)*
; CHECK:    [[TMP3:%.*]] = bitcast i8 addrspace(3)* [[MEMPOOLCAST]] to i32 addrspace(3)*
; CHECK:    store i32 0, i32 addrspace(3)* [[TMP3]], align 4
; CHECK:    [[TMP5:%.*]] = load i32, i32* [[WG]], align 4
; CHECK:    [[TMP6:%.*]] = load i32 addrspace(3)*, i32 addrspace(3)* addrspace(3)* @a, align 8
; CHECK:    [[ARRAYIDX1:%.*]] = getelementptr inbounds i32, i32 addrspace(3)* [[TMP6]], i64 0
; CHECK:    store i32 [[TMP5]], i32 addrspace(3)* [[ARRAYIDX1]], align 4
; CHECK:    ret void
  %wg = alloca i32, align 4
  %call.i = call spir_func i8 addrspace(3)* @__builtin_IB_AllocLocalMemPool(i1 false, i32 1, i32 4)
  %0 = bitcast i8 addrspace(3)* %call.i to i32 addrspace(3)*
  store i32 0, i32 addrspace(3)* %0, align 4
  %1 = load i32, i32* %wg, align 4
  %2 = load i32 addrspace(3)*, i32 addrspace(3)* addrspace(3)* @a, align 8
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(3)* %2, i64 0
  store i32 %1, i32 addrspace(3)* %arrayidx1, align 4
  ret void
}

declare spir_func i8 addrspace(3)* @__builtin_IB_AllocLocalMemPool(i1, i32, i32) local_unnamed_addr

