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
; RUN: igc_opt -igc-wi-func-resolution -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that WIFunctResolution pass handles line debug info.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target triple = "igil_32_GEN8"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This function checks the following cases:
;;   get_global_size
;;   get_local_size
;;   get_global_offset
;;   get_enqueued_local_size
;;   get_num_groups
;;   get_group_id(i32 0)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Function Attrs: alwaysinline nounwind
define void @test(i32* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %numGroups, <3 x i32> %globalSize, <3 x i32> %localSize, <3 x i32> %enqueuedLocalSize) #0 {
  %res1 = call i64 @__builtin_IB_get_global_size(i32 0) #0, !dbg !1
  %res2 = call i32 @__builtin_IB_get_local_size(i32 1) #0, !dbg !2
  %res3 = call i32 @__builtin_IB_get_global_offset(i32 2) #0, !dbg !3
  %res4 = call i32 @__builtin_IB_get_enqueued_local_size(i32 1) #0, !dbg !4
  %res5 = call i32 @__builtin_IB_get_num_groups(i32 0) #0, !dbg !5
  %res6 = call i32 @__builtin_IB_get_group_id(i32 2) #0, !dbg !6

  %res1.32 = trunc i64 %res1 to i32
  %sum1 = add i32 %res1.32, %res2
  %sum2 = add i32 %res3, %res4
  %sum3 = add i32 %res5, %res6

  %mask = and i32 %sum1, %sum2
  %res = and i32 %mask, %sum3
  store i32 %res, i32* %dst, align 4
  ret void

; CHECK-NOT: call i64 @__builtin_IB_get_global_size
; CHECK: [[global_size:%[a-zA-Z0-9]+]] = extractelement <3 x i32> %globalSize, i32 0, !dbg !1
; CHECK: [[global_size64:%[a-zA-Z0-9]+]] = zext i32 [[global_size]] to i64, !dbg !1

; CHECK-NOT: call i32 @__builtin_IB_get_local_size
; CHECK: [[local_size:%[a-zA-Z0-9]+]] = extractelement <3 x i32> %localSize, i32 1, !dbg !2

; CHECK-NOT: call i32 @__builtin_IB_get_global_offset
; CHECK: [[global_offset:%[a-zA-Z0-9]+]] = extractelement <8 x i32> %payloadHeader, i32 2, !dbg !3

; CHECK-NOT: call i32 @__builtin_IB_get_enqueued_local_size
; CHECK: [[enqueued_local_size:%[a-zA-Z0-9]+]] = extractelement <3 x i32> %enqueuedLocalSize, i32 1, !dbg !4

; CHECK-NOT: call i32 @__builtin_IB_get_num_groups
; CHECK: [[num_groups:%[a-zA-Z0-9]+]] = extractelement <3 x i32> %numGroups, i32 0, !dbg !5

; CHECK-NOT: call i32 @__builtin_IB_get_group_id
; CHECK: [[cmp_dim:%[a-zA-Z0-9]+]] = icmp eq i32 2, 0, !dbg !6
; CHECK: [[temp_offset_r0:%[a-zA-Z0-9]+]] = select i1 [[cmp_dim]], i32 1, i32 5, !dbg !6
; CHECK: [[offset_r0:%[a-zA-Z0-9]+]] = add i32 2, [[temp_offset_r0]], !dbg !6
; CHECK: [[group_id:%[a-zA-Z0-9]+]] = extractelement <8 x i32> %r0, i32 [[offset_r0]], !dbg !6

; CHECK: [[global_size32:%[a-zA-Z0-9.]+]] = trunc i64 [[global_size64]] to i32
; CHECK: [[test_sum1:%[a-zA-Z0-9]+]] = add i32 [[global_size32]], [[local_size]]
; CHECK: [[test_sum2:%[a-zA-Z0-9]+]] = add i32 [[global_offset]], [[enqueued_local_size]]
; CHECK: [[test_sum3:%[a-zA-Z0-9]+]] = add i32 [[num_groups]], [[group_id]]
}

declare i64 @__builtin_IB_get_global_size(i32)
declare i32 @__builtin_IB_get_local_size(i32)
declare i32 @__builtin_IB_get_global_offset(i32)
declare i32 @__builtin_IB_get_enqueued_local_size(i32)
declare i32 @__builtin_IB_get_num_groups(i32)
declare i32 @__builtin_IB_get_group_id(i32)

attributes #0 = { nounwind }

;; This hack named metadata is needed to assure metadata order
!hack_order = !{!0, !1, !2, !3, !4, !5, !6, !7, !8, !9, !10, !16, !11, !12, !13, !14, !15}

!igc.functions = !{!15}

!0 = !{}
!1 = !{i32 5, i32 0, !0, null}
!2 = !{i32 6, i32 0, !0, null}
!3 = !{i32 7, i32 0, !0, null}
!4 = !{i32 8, i32 0, !0, null}
!5 = !{i32 9, i32 0, !0, null}
!6 = !{i32 10, i32 0, !0, null}
!7 = !{i32 0}  ;; R0
!8 = !{i32 1}  ;; PAYLOAD_HEADER
!9 = !{i32 3}  ;; NUM_GROUPS
!10 = !{i32 4} ;; GLOBAL_SIZE
!16 = !{i32 5} ;; LOCAL_SIZE
!11 = !{i32 6} ;; ENQUEUED_LOCAL_WORK_SIZE
!12 = !{!"function_type", i32 0}
!13 = !{!"implicit_arg_desc", !7, !8, !9, !10, !16, !11}
!14 = !{!12, !13}
!15 = !{void (i32*, <8 x i32>, <8 x i32>, <3 x i32>, <3 x i32>, <3 x i32>, <3 x i32>)* @test, !14}
