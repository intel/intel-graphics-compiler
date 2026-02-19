;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-kernel-function-cloning -S < %s | FileCheck %s
; ------------------------------------------------
; KernelFunctionCloning
; ------------------------------------------------
; This test is reduced from ocl test kernel:
; __kernel void bar(__global int *dst, int src)
; {
;     dst[0] = src;
;
; }
;
; __kernel void foo(__global int *srcA)
; {
;     int src = srcA[1];
;     bar(srcA, src);
; }
;
; ------------------------------------------------

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: noinline nounwind
define spir_kernel void @bar(i32 addrspace(1)* %dst, i32 %src) #0 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %src.addr = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  store i32 %src, i32* %src.addr, align 4
  %0 = load i32, i32* %src.addr, align 4
  %1 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %1, i64 0
  store i32 %0, i32 addrspace(1)* %arrayidx, align 4
  ret void
}


; Function Attrs: noinline nounwind
define spir_kernel void @foo(i32 addrspace(1)* %srcA) #0 {
; CHECK-LABEL: @foo(
; CHECK:  entry:
; CHECK:    [[SRCA_ADDR:%.*]] = alloca i32 addrspace(1)*, align 8
; CHECK:    [[SRC:%.*]] = alloca i32, align 4
; CHECK:    store i32 addrspace(1)* [[SRCA:%.*]], i32 addrspace(1)** [[SRCA_ADDR]], align 8
; CHECK:    [[TMP0:%.*]] = load i32 addrspace(1)*, i32 addrspace(1)** [[SRCA_ADDR]], align 8
; CHECK:    [[ARRAYIDX:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* [[TMP0]], i64 1
; CHECK:    [[TMP1:%.*]] = load i32, i32 addrspace(1)* [[ARRAYIDX]], align 4
; CHECK:    store i32 [[TMP1]], i32* [[SRC]], align 4
; CHECK:    [[TMP2:%.*]] = load i32 addrspace(1)*, i32 addrspace(1)** [[SRCA_ADDR]], align 8
; CHECK:    [[TMP3:%.*]] = load i32, i32* [[SRC]], align 4
; CHECK:    call spir_kernel void @bar.1(i32 addrspace(1)* [[TMP2]], i32 [[TMP3]]) #0
; CHECK:    ret void
;
entry:
  %srcA.addr = alloca i32 addrspace(1)*, align 8
  %src = alloca i32, align 4
  store i32 addrspace(1)* %srcA, i32 addrspace(1)** %srcA.addr, align 8
  %0 = load i32 addrspace(1)*, i32 addrspace(1)** %srcA.addr, align 8
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 1
  %1 = load i32, i32 addrspace(1)* %arrayidx, align 4
  store i32 %1, i32* %src, align 4
  %2 = load i32 addrspace(1)*, i32 addrspace(1)** %srcA.addr, align 8
  %3 = load i32, i32* %src, align 4
  call spir_kernel void @bar(i32 addrspace(1)* %2, i32 %3) #0
  ret void
}

; CHECK-LABEL: define internal spir_kernel void @bar.1(
; CHECK:  entry:
; CHECK:    [[DST_ADDR:%.*]] = alloca i32 addrspace(1)*, align 8
; CHECK:    [[SRC_ADDR:%.*]] = alloca i32, align 4
; CHECK:    store i32 addrspace(1)* [[DST:%.*]], i32 addrspace(1)** [[DST_ADDR]], align 8
; CHECK:    store i32 [[SRC:%.*]], i32* [[SRC_ADDR]], align 4
; CHECK:    [[TMP0:%.*]] = load i32, i32* [[SRC_ADDR]], align 4
; CHECK:    [[TMP1:%.*]] = load i32 addrspace(1)*, i32 addrspace(1)** [[DST_ADDR]], align 8
; CHECK:    [[ARRAYIDX:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* [[TMP1]], i64 0
; CHECK:    store i32 [[TMP0]], i32 addrspace(1)* [[ARRAYIDX]], align 4
; CHECK:    ret void
;

attributes #0 = { noinline nounwind }

!IGCMetadata = !{!2}
!igc.functions = !{!11, !14}

!2 = !{!"ModuleMD", !3}
!3 = !{!"FuncMD", !4, !5, !9, !10}
!4 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, i32)* @bar}
!5 = !{!"FuncMDValue[0]", !6, !7, !8}
!6 = !{!"funcArgs"}
!7 = !{!"functionType", !"KernelFunction"}
!8 = !{!"isCloned", i1 false}
!9 = !{!"FuncMDMap[1]", void (i32 addrspace(1)*)* @foo}
!10 = !{!"FuncMDValue[1]", !6, !7, !8}
!11 = !{void (i32 addrspace(1)*, i32)* @bar, !12}
!12 = !{!13}
!13 = !{!"function_type", i32 0}
!14 = !{void (i32 addrspace(1)*)* @foo, !12}
