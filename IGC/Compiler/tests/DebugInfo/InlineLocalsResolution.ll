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
; RUN: opt -igc-resolve-inline-locals -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that InlineLocalsResolution pass handles variable debug info.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a64:64:64-f80:128:128-n8:16:32:64"
target triple = "igil_32_GEN8"

define void @test1(float addrspace(3)* %dst) #0 {
entry:
  call void @llvm.dbg.value(metadata !{float addrspace(3)* %dst}, i64 0, metadata !3), !dbg !1
  store float 1.000000e+00, float addrspace(3)* %dst, align 4, !dbg !2
  ret void

; CHECK: [[localToChar:%[a-zA-Z0-9_]+]] = bitcast float addrspace(3)* %dst to i8 addrspace(3)*
; CHECK: [[movedLocal:%[a-zA-Z0-9_]+]] = getelementptr i8 addrspace(3)* [[localToChar]], i32 0
; CHECK: [[charToLocal:%[a-zA-Z0-9_]+]] = bitcast i8 addrspace(3)* [[movedLocal]] to float addrspace(3)*
; CHECK: call void @llvm.dbg.value(metadata !{float addrspace(3)* [[charToLocal]]}, i64 0, metadata !3), !dbg !1
; CHECK: store float 1.000000e+00, float addrspace(3)* [[charToLocal]], align 4, !dbg !2
}


define void @test2(float addrspace(3)* %dst, i8 addrspace(1)* %localMemStatelessWindowStartAddr, i8 addrspace(1)* %localMemStatelessWindowSize) #0 {
entry:
  call void @llvm.dbg.value(metadata !{float addrspace(3)* %dst}, i64 0, metadata !6), !dbg !4
  %intval = ptrtoint float addrspace(3)* %dst to i32
  %floatval = bitcast i32 %intval to float
  store float %floatval, float addrspace(3)* %dst, align 4, !dbg !5
  ret void

; CHECK: [[localToChar:%[a-zA-Z0-9_]+]] = bitcast float addrspace(3)* %dst to i8 addrspace(3)*
; CHECK: [[movedLocal:%[a-zA-Z0-9_]+]] = getelementptr i8 addrspace(3)* [[localToChar]], i32 0
; CHECK: [[startPtrToInt:%[a-zA-Z0-9_]+]] = ptrtoint i8 addrspace(1)* %localMemStatelessWindowStartAddr to i32
; CHECK: [[varPtrToInt:%[a-zA-Z0-9_]+]] = ptrtoint i8 addrspace(3)* [[movedLocal]] to i32
; CHECK: [[addStartPtr:%[a-zA-Z0-9_]+]] = add i32 [[startPtrToInt]], [[varPtrToInt]]
; CHECK: [[argWithBase:%[a-zA-Z0-9_]+]] = inttoptr i32 [[addStartPtr]] to float addrspace(3)*
; CHECK: call void @llvm.dbg.value(metadata !{float addrspace(3)* [[argWithBase]]}, i64 0, metadata !6), !dbg !4
; CHECK: store float %floatval, float addrspace(3)* [[argWithBase]], align 4, !dbg !5
}


declare void @llvm.dbg.value(metadata, i64, metadata) #1

attributes #0 = { alwaysinline nounwind }
attributes #1 = { nounwind readnone }

;; This hack named metadata is needed to assure metadata order
!hack_order = !{!0, !1, !2, !3, !4, !5, !6, !7, !8, !9, !10, !11, !12, !13, !14}

!igc.functions = !{!12, !14}

!0 = metadata !{}
!1 = metadata !{i32 1, i32 0, metadata !0, null}
!2 = metadata !{i32 3, i32 0, metadata !0, null}
!3 = metadata !{i32 786689, metadata !0, metadata !"dst", metadata !0, i32 16777217, metadata !0, i32 0, i32 0}

!4 = metadata !{i32 5, i32 0, metadata !0, null}
!5 = metadata !{i32 7, i32 0, metadata !0, null}
!6 = metadata !{i32 786689, metadata !0, metadata !"dst2", metadata !0, i32 16777217, metadata !0, i32 0, i32 0}

!7 = metadata !{metadata !"function_type", i32 0}
!8 = metadata !{i32 39} ;; LOCAL_MEMORY_STATELESS_WINDOW_START_ADDRESS
!9 = metadata !{i32 40} ;; LOCAL_MEMORY_STATELESS_WINDOW_SIZE
!10 = metadata !{metadata !"implicit_arg_desc", metadata !8, metadata !9}
!11 = metadata !{metadata !7, metadata !10}
!12 = metadata !{void (float addrspace(3)*, i8 addrspace(1)*, i8 addrspace(1)*)* @test2, metadata !11}
!13 = metadata !{metadata !7}
!14 = metadata !{void (float addrspace(3)*)* @test1, metadata !13}


