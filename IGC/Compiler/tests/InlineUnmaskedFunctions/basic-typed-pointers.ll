;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -inline-unmasked -S < %s | FileCheck %s
; ------------------------------------------------
; InlineUnmaskedFunctions
; ------------------------------------------------

define void @test(i32 %src, i32* %dst) {
; CHECK-LABEL: @test(
; CHECK:    [[TMP1:%.*]] = load i32, i32* [[DST:%.*]], align 4
; CHECK:    call void @llvm.genx.GenISA.UnmaskedRegionBegin()
; CHECK:    [[TMP2:%.*]] = load i32, i32* [[DST]], align 4
; CHECK:    [[TMP3:%.*]] = add i32 [[TMP1]], [[TMP2]]
; CHECK:    store i32 [[TMP3]], i32* [[DST]], align 4
; CHECK:    call void @llvm.genx.GenISA.UnmaskedRegionEnd()
; CHECK:    [[TMP4:%.*]] = load i32, i32* [[DST]], align 4
; CHECK:    store i32 [[TMP4]], i32* [[DST]], align 4
; CHECK:    ret void
;
  %1 = load i32, i32* %dst, align 4
  call void @foo(i32 %1, i32* %dst)
  %2 = load i32, i32* %dst, align 4
  store i32 %2, i32* %dst, align 4
  ret void
}

define void @foo(i32 %s1, i32* %d1) #0 {
  %1 = load i32, i32* %d1, align 4
  %2 = add i32 %s1, %1
  store i32 %2, i32* %d1, align 4
  ret void
}

attributes #0 = { "sycl-unmasked" }

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3, !9, !10}
!2 = !{!"FuncMDMap[0]", void (i32, i32*)* @test}
!3 = !{!"FuncMDValue[0]", !4, !5}
!4 = !{!"localOffsets"}
!5 = !{!"workGroupWalkOrder", !6, !7, !8}
!6 = !{!"dim0", i32 0}
!7 = !{!"dim1", i32 0}
!8 = !{!"dim2", i32 0}
!9 = !{!"FuncMDMap[1]", void (i32, i32*)* @foo}
!10 = !{!"FuncMDValue[1]", !4, !5}
