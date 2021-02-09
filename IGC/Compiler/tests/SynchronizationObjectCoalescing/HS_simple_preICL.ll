;===================== begin_copyright_notice ==================================

;Copyright (c) 2017 Intel Corporation

;Permission is hereby granted, free of charge, to any person obtaining a
;copy of this software and associated documentation files (the
;"Software"), to deal in the Software without restriction, including
;without limitation the rights to use, copy, modify, merge, publish,
;distribute, sublicense, and/or sell copies of the Software, and to
;permit persons to whom the Software is furnished to do so, subject to
;the following conditions:

;The above copyright notice and this permission notice shall be included
;in all copies or substantial portions of the Software.

;THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
;OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
;MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
;IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
;CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
;TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
;SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


;======================= end_copyright_notice ==================================
; RUN: igc_opt %s -S -o - -platformskl -inpuths -igc-synchronization-object-coalescing | FileCheck %s
;

target datalayout = "e-p:32:32:32-p1:64:64:64-p2:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a0:0:32-n8:16:32-S32"


define void @f3(<8 x i32> %r0, i8* %privateBase) #5 {
GlobalScopeInitialization:
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 true, i1 false)
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false)
  call void @llvm.genx.GenISA.threadgroupbarrier()
  ret void
}
; CHECK-LABEL: define void @f3
; CHECK:      call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 true, i1 true)
; CHECK-NEXT: call void @llvm.genx.GenISA.threadgroupbarrier()


; Function Attrs: nounwind readnone
declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32) #1

; Function Attrs: nounwind readnone
declare i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32) #1

; Function Attrs: convergent nounwind
declare void @llvm.genx.GenISA.memoryfence(i1, i1, i1, i1, i1, i1, i1) #3

; Function Attrs: convergent nounwind
declare void @llvm.genx.GenISA.threadgroupbarrier() #3

attributes #2 = { nounwind }
attributes #3 = { convergent nounwind }
attributes #5 = { "null-pointer-is-valid"="true" }

!igc.functions = !{!3}

!3 = !{void (<8 x i32>, i8*)* @f3, !5}
!5 = !{!6, !7}
!6 = !{!"function_type", i32 0}
!7 = !{!"implicit_arg_desc", !8, !9}
!8 = !{i32 0}
!9 = !{i32 12}
