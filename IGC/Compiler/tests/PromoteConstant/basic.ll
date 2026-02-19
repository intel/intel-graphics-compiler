;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify -PromoteConstant -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PromoteConstant
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

@a = private addrspace(2) constant [4 x i32] [i32 -13, i32 42, i32 13, i32 -42], align 1
@b = private addrspace(2) constant [16 x i32] [i32 -13, i32 42, i32 13, i32 -42, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12], align 1

; Function Attrs: convergent noinline nounwind
define spir_kernel void @test_promoteconst(i32 %a, i32* %b) #0 {
; CHECK-LABEL: @test_promoteconst(
; CHECK:  entry:
; CHECK:    [[TMP0:%.*]] = getelementptr inbounds [4 x i32], [4 x i32] addrspace(2)* @a, i32 0, i32 [[A:%[A-z0-9]*]]
; CHECK:    [[TMP1:%.*]] = getelementptr inbounds [16 x i32], [16 x i32] addrspace(2)* @b, i32 0, i32 [[A]]
; CHECK:    br label %bb1
; CHECK:  bb1:
; CHECK:    [[TMP2:%.*]] = icmp eq i32 [[A]], 0
; CHECK:    [[TMP3:%.*]] = select i1 [[TMP2]], i32 -13, i32 42
; CHECK:    [[TMP4:%.*]] = icmp eq i32 [[A]], 2
; CHECK:    [[TMP5:%.*]] = select i1 [[TMP4]], i32 13, i32 -42
; CHECK:    [[TMP6:%.*]] = icmp slt i32 [[A]], 2
; CHECK:    [[TMP7:%.*]] = select i1 [[TMP6]], i32 [[TMP3]], i32 [[TMP5]]
; CHECK:    [[TMP8:%.*]] = extractelement <16 x i8> <i8 -13, i8 42, i8 13, i8 -42, i8 1, i8 2, i8 3, i8 4, i8 5, i8 6, i8 7, i8 8, i8 9, i8 10, i8 11, i8 12>, i32 [[A]]
; CHECK:    [[TMP9:%.*]] = sext i8 [[TMP8]] to i32
; CHECK:    [[TMP10:%.*]] = icmp sgt i32 [[TMP7]], [[TMP9]]
; CHECK:    br i1 [[TMP10]], label %bb1, label %end
; CHECK:  end:
; CHECK:    store i32 [[TMP9]], i32* [[B:%.*]]
; CHECK:    ret void
;
entry:
  %0 = getelementptr inbounds [4 x i32], [4 x i32] addrspace(2)* @a, i32 0, i32 %a
  %1 = getelementptr inbounds [16 x i32], [16 x i32] addrspace(2)* @b, i32 0, i32 %a
  br label %bb1

bb1:                                              ; preds = %bb1, %entry
  %2 = load i32, i32 addrspace(2)* %0
  %3 = load i32, i32 addrspace(2)* %1
  %4 = icmp sgt i32 %2, %3
  br i1 %4, label %bb1, label %end

end:                                              ; preds = %bb1
  store i32 %3, i32* %b
  ret void
}
