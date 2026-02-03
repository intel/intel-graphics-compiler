;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --typed-pointers %s -S -o - -opt-atomics-pass | FileCheck %s

declare i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1i32.p1i32(i32 addrspace(1)*, i32 addrspace(1)*, i32, i32)
declare i32 @llvm.genx.GenISA.icmpxchgatomicrawA64.i32.p1i32.p1i32(i32 addrspace(1)*, i32 addrspace(1)*, i32, i32)

define spir_kernel void @kernel1(i32 addrspace(1)* %src) {
entry:
  br label %back
back:
  %call1 = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1i32.p1i32(i32 addrspace(1)* %src, i32 addrspace(1)* %src, i32 0, i32 9)
  %bc1 = bitcast i32 %call1 to float
  %operation = fadd fast float %bc1, -2.000000e+00
  %bc2 = bitcast float %operation to i32
  %call2 = call i32 @llvm.genx.GenISA.icmpxchgatomicrawA64.i32.p1i32.p1i32(i32 addrspace(1)* %src, i32 addrspace(1)* %src, i32 %call1, i32 %bc2)
  %cmp = icmp eq i32 %call1, %call2
  br i1 %cmp, label %exit, label %back
exit:
  ret void
}

!igc.functions = !{!0}
!0 = !{void (i32 addrspace(1)*)* @kernel1, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

; CHECK-LABEL: @kernel1(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    br label %back
; CHECK:       back:
; CHECK-NEXT:    [[TMP0:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK-NEXT:    [[TMP1:%.*]] = zext i16 [[TMP0]] to i32
; CHECK-NEXT:    [[TMP2:%.*]] = call float @llvm.genx.GenISA.WaveAll.f32(float -2.000000e+00, i8 9, i1 true, i32 0)
; CHECK-NEXT:    [[TMP3:%.*]] = icmp eq i32 [[TMP1]], 0
; CHECK-NEXT:    br i1 [[TMP3]], label %[[TMP4:.*]], label [[EXIT:%.*]]
; CHECK:       [[TMP4]]:
; CHECK-NEXT:    [[CALL1:%.*]] = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1i32.p1i32(i32 addrspace(1)* [[SRC:%.*]], i32 addrspace(1)* [[SRC]], i32 0, i32 9)
; CHECK-NEXT:    [[BC1:%.*]] = bitcast i32 [[CALL1]] to float
; CHECK-NEXT:    [[OPERATION:%.*]] = fadd fast float [[BC1]], [[TMP2]]
; CHECK-NEXT:    [[BC2:%.*]] = bitcast float [[OPERATION]] to i32
; CHECK-NEXT:    [[CALL2:%.*]] = call i32 @llvm.genx.GenISA.icmpxchgatomicrawA64.i32.p1i32.p1i32(i32 addrspace(1)* [[SRC]], i32 addrspace(1)* [[SRC]], i32 [[CALL1]], i32 [[BC2]])
; CHECK-NEXT:    [[CMP:%.*]] = icmp eq i32 [[CALL1]], [[CALL2]]
; CHECK-NEXT:    br i1 [[CMP]], label [[EXIT]], label [[TMP4:%.*]]
; CHECK:       exit:
; CHECK-NEXT:    ret void
;

define spir_kernel void @kernel2(i32 addrspace(1)* %src) {
entry:
  br label %back
back:
  %call1 = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1i32.p1i32(i32 addrspace(1)* %src, i32 addrspace(1)* %src, i32 0, i32 9)
  %bc1 = bitcast i32 %call1 to float
  %operation = fadd float %bc1, -2.000000e+00
  %bc2 = bitcast float %operation to i32
  %call2 = call i32 @llvm.genx.GenISA.icmpxchgatomicrawA64.i32.p1i32.p1i32(i32 addrspace(1)* %src, i32 addrspace(1)* %src, i32 %call1, i32 %bc2)
  %cmp = icmp eq i32 %call1, %call2
  br i1 %cmp, label %exit, label %back
exit:
  ret void
}

!igc.functions = !{!3}
!3 = !{void (i32 addrspace(1)*)* @kernel2, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 0}

; Optimization should not be applied in this case. Since the flag for fast math was not passed.
;
; CHECK-LABEL: define spir_kernel void @kernel2(i32 addrspace(1)* %src) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    br label %back
; CHECK:       back:
; CHECK-NEXT:    %call1 = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1i32.p1i32(i32 addrspace(1)* %src, i32 addrspace(1)* %src, i32 0, i32 9)
; CHECK-NEXT:    %bc1 = bitcast i32 %call1 to float
; CHECK-NEXT:    %operation = fadd float %bc1, -2.000000e+00
; CHECK-NEXT:    %bc2 = bitcast float %operation to i32
; CHECK-NEXT:    %call2 = call i32 @llvm.genx.GenISA.icmpxchgatomicrawA64.i32.p1i32.p1i32(i32 addrspace(1)* %src, i32 addrspace(1)* %src, i32 %call1, i32 %bc2)
; CHECK-NEXT:    %cmp = icmp eq i32 %call1, %call2
; CHECK-NEXT:    br i1 %cmp, label %exit, label %back
; CHECK:       exit:
; CHECK-NEXT:    ret void
; CHECK-NEXT:  }
;
