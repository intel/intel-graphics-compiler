;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-const-prop -igc-serialize-metadata -S < %s | FileCheck %s
; RUN: igc_opt --typed-pointers --igc-const-prop --override-enable-simplify-gep=1 -igc-serialize-metadata -S < %s | FileCheck %s --check-prefix=OVERRIDE
; ------------------------------------------------
; IGCConstProp
; ------------------------------------------------

declare void @use_float(float*)

define spir_kernel void @test_func(i32 addrspace(1)* %d, i32 addrspace(1)* %s) {
; CHECK-LABEL: @test_func(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = load i32, i32 addrspace(1)* [[D:%.*]]
; CHECK-NEXT:    [[TMP1:%.*]] = add i32 4, [[TMP0]]
; CHECK-NEXT:    [[TMP2:%.*]] = add i32 5, [[TMP1]]
; CHECK-NEXT:    [[TMP3:%.*]] = add i32 [[TMP2]], 6
; CHECK-NEXT:    [[TMP4:%.*]] = add i32 15, [[TMP3]]
; CHECK-NEXT:    [[TMP5:%.*]] = load i32, i32 addrspace(1)* [[S:%.*]]
; CHECK-NEXT:    [[TMP6:%.*]] = add i32 [[TMP5]], 7
; CHECK-NEXT:    [[TMP7:%.*]] = add i32 [[TMP1]], [[TMP6]]
; CHECK-NEXT:    [[TMP8:%.*]] = alloca [1024 x float], align 4
; CHECK-NEXT:    [[TMP9:%.*]] = getelementptr inbounds [1024 x float], [1024 x float]* [[TMP8]], i32 0, i32 [[TMP7]]
; CHECK-NEXT:    call void @use_float(float* [[TMP9]])
; CHECK-NEXT:    [[TMP10:%.*]] = getelementptr inbounds [1024 x float], [1024 x float]* [[TMP8]], i32 0, i32 [[TMP4]]
; CHECK-NEXT:    call void @use_float(float* [[TMP10]])
; CHECK-NEXT:    [[TMP11:%.*]] = getelementptr inbounds [1024 x float], [1024 x float]* [[TMP8]], i32 0, i32 3
; CHECK-NEXT:    call void @use_float(float* [[TMP11]])
; CHECK-NEXT:    ret void
;
; OVERRIDE-LABEL: @test_func(
; OVERRIDE-NEXT:  entry:
; OVERRIDE-NEXT:    [[TMP0:%.*]] = load i32, i32 addrspace(1)* [[D:%.*]]
; OVERRIDE-NEXT:    [[TMP1:%.*]] = add i32 1, 2
; OVERRIDE-NEXT:    [[TMP2:%.*]] = add i32 3, 3
; OVERRIDE-NEXT:    [[TMP3:%.*]] = add i32 [[TMP0]], 4
; OVERRIDE-NEXT:    [[TMP4:%.*]] = add i32 4, [[TMP0]]
; OVERRIDE-NEXT:    [[TMP5:%.*]] = add i32 [[TMP0]], 9
; OVERRIDE-NEXT:    [[TMP6:%.*]] = add i32 5, [[TMP3]]
; OVERRIDE-NEXT:    [[TMP7:%.*]] = add i32 6, 9
; OVERRIDE-NEXT:    [[TMP8:%.*]] = add i32 [[TMP0]], 15
; OVERRIDE-NEXT:    [[TMP9:%.*]] = add i32 [[TMP5]], 6
; OVERRIDE-NEXT:    [[TMP10:%.*]] = add i32 [[TMP0]], 30
; OVERRIDE-NEXT:    [[TMP11:%.*]] = add i32 15, [[TMP8]]
; OVERRIDE-NEXT:    [[TMP12:%.*]] = load i32, i32 addrspace(1)* [[S:%.*]]
; OVERRIDE-NEXT:    [[TMP13:%.*]] = add i32 [[TMP12]], 7
; OVERRIDE-NEXT:    [[TMP14:%.*]] = add i32 [[TMP0]], [[TMP12]]
; OVERRIDE-NEXT:    [[TMP15:%.*]] = add i32 [[TMP14]], 11
; OVERRIDE-NEXT:    [[TMP16:%.*]] = add i32 [[TMP3]], [[TMP13]]
; OVERRIDE-NEXT:    [[TMP17:%.*]] = alloca [1024 x float], align 4
; OVERRIDE-NEXT:    [[TMP18:%.*]] = getelementptr inbounds [1024 x float], [1024 x float]* [[TMP17]], i32 0, i32 [[TMP15]]
; OVERRIDE-NEXT:    call void @use_float(float* [[TMP18]])
; OVERRIDE-NEXT:    [[TMP19:%.*]] = getelementptr inbounds [1024 x float], [1024 x float]* [[TMP17]], i32 0, i32 [[TMP10]]
; OVERRIDE-NEXT:    call void @use_float(float* [[TMP19]])
; OVERRIDE-NEXT:    [[TMP20:%.*]] = getelementptr inbounds [1024 x float], [1024 x float]* [[TMP17]], i32 0, i32 3
; OVERRIDE-NEXT:    call void @use_float(float* [[TMP20]])
; OVERRIDE-NEXT:    ret void
;
entry:
  %0 = load i32, i32 addrspace(1)* %d
  %1 = add i32 1, 2
  %2 = add i32 %1, 3
  %3 = add i32 4, %0
  %4 = add i32 5, %3
  %5 = add i32 %2, 9
  %6 = add i32 %4, 6
  %7 = add i32 %5, %6
  %8 = load i32, i32 addrspace(1)* %s
  %9 = add i32 %8, 7
  %10 = add i32 %3, %9

  %11 = alloca[1024 x float], align 4

  %12 = getelementptr inbounds [1024 x float], [1024 x float]* %11, i32 0, i32 %10
  call void @use_float(float* %12)

  %13 = getelementptr inbounds [1024 x float], [1024 x float]* %11, i32 0, i32 %7
  call void @use_float(float* %13)

  %14 = getelementptr inbounds [1024 x float], [1024 x float]* %11, i32 0, i32 %1
  call void @use_float(float* %14)

  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!4}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, i32 addrspace(1)*)* @test_func}
!3 = !{!"FuncMDValue[0]"}
!4 = !{void (i32 addrspace(1)*, i32 addrspace(1)*)* @test_func, !5}
!5 = !{!6}
!6 = !{!"function_type", i32 0}
