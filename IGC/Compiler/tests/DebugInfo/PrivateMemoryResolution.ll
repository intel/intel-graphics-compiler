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
; RUN: igc_opt -igc-private-mem-resolution -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that PrivateMemoryResolution pass handles
;; variable and line debug info.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target triple = "igil_32_GEN8"

define void @test(i32 %index, float addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase) #0 {
entry:
  %cond = icmp eq i32 %index, 0
  br i1 %cond, label %BBTrue, label %BBFalse

; CHECK: entry:
; CHECK: [[simdLaneId16:%[a-zA-Z0-9_.]+]] = call i16 @genx.GenISA.simdLaneId()
; CHECK: [[simdLaneId:%[a-zA-Z0-9_.]+]] = zext i16 [[simdLaneId16]] to i32
; CHECK: [[simdSize:%[a-zA-Z0-9_.]+]] = call i32 @genx.GenISA.simdSize()
; CHECK: [[totalPrivateMemPerThread:%[a-zA-Z0-9_.]+]] = mul i32 [[simdSize]], 80
; CHECK: [[r0_5:%[a-zA-Z0-9_.]+]] = extractelement <8 x i32> %r0, i32 5
; CHECK: [[threadId:%[a-zA-Z0-9_.]+]] = and i32 [[r0_5]], 511
; CHECK: [[perThreadOffset:%[a-zA-Z0-9_.]+]] = mul i32 [[threadId]], [[totalPrivateMemPerThread]]

BBTrue:
  %a = alloca [10 x float], align 4, !dbg !1
  call void @llvm.dbg.declare(metadata [10 x float]* %a, metadata !5), !dbg !2
  %arrayidx1 = getelementptr inbounds [10 x float], [10 x float]* %a, i32 0, i32 %index, !dbg !2
  %res1 = load float, float* %arrayidx1, align 4, !dbg !2
  store float %res1, float addrspace(1)* %dst, align 4, !dbg !2
  br label %BBend

; CHECK: BBTrue:
; CHECK: [[a_SIMDBufferOffset:%[a-zA-Z0-9_.]+]] = mul i32 [[simdSize]], 0, !dbg !1
; CHECK: [[a_bufferOffsetForThread:%[a-zA-Z0-9_.]+]] = add i32 [[perThreadOffset]], [[a_SIMDBufferOffset]], !dbg !1
; CHECK: [[perLaneOffset:%[a-zA-Z0-9_.]+]] = mul i32 %simdLaneId, 40, !dbg !1
; CHECK: [[a_totalOffset:%[a-zA-Z0-9_.]+]] = add i32 [[a_bufferOffsetForThread]], [[perLaneOffset]], !dbg !1
; CHECK: [[a_privateBufferGEP:%[a-zA-Z0-9_.]+]] = getelementptr i8, i8* %privateBase, i32 [[a_totalOffset]], !dbg !1
; CHECK: [[a_privateBuffer:%[a-zA-Z0-9_.]+]] = bitcast i8* [[a_privateBufferGEP]] to [10 x float]*, !dbg !1

; CHECK: void @llvm.dbg.declare(metadata [10 x float]* [[a_privateBuffer]], metadata !5), !dbg !2
; CHECK: [[arrayidx1:%[a-zA-Z0-9_.]+]] = getelementptr inbounds [10 x float], [10 x float]* [[a_privateBuffer]], i32 0, i32 %index, !dbg !2

BBFalse:
  %b = alloca [10 x float], align 4, !dbg !3
  call void @llvm.dbg.declare(metadata [10 x float]* %b, metadata !6), !dbg !4
  %arrayidx2 = getelementptr inbounds [10 x float], [10 x float]* %b, i32 0, i32 %index, !dbg !4
  %res2 = load float, float* %arrayidx2, align 4, !dbg !4
  store float %res2, float addrspace(1)* %dst, align 4, !dbg !4
  br label %BBend

; CHECK: BBFalse:
; CHECK: [[b_SIMDBufferOffset:%[a-zA-Z0-9_.]+]] = mul i32 [[simdSize]], 40, !dbg !3
; CHECK: [[b_bufferOffsetForThread:%[a-zA-Z0-9_.]+]] = add i32 [[perThreadOffset]], [[b_SIMDBufferOffset]], !dbg !3
; CHECK: [[perLaneOffset:%[a-zA-Z0-9_.]+]] = mul i32 %simdLaneId, 40, !dbg !3
; CHECK: [[b_totalOffset:%[a-zA-Z0-9_.]+]] = add i32 [[b_bufferOffsetForThread]], [[perLaneOffset]], !dbg !3
; CHECK: [[b_privateBufferGEP:%[a-zA-Z0-9_.]+]] = getelementptr i8, i8* %privateBase, i32 [[b_totalOffset]], !dbg !3
; CHECK: [[b_privateBuffer:%[a-zA-Z0-9_.]+]] = bitcast i8* [[b_privateBufferGEP]] to [10 x float]*, !dbg !3

; CHECK: void @llvm.dbg.declare(metadata [10 x float]* [[b_privateBuffer]], metadata !6), !dbg !4
; CHECK: [[arrayidx2:%[a-zA-Z0-9_.]+]] = getelementptr inbounds [10 x float], [10 x float]* [[b_privateBuffer]], i32 0, i32 %index, !dbg !4

BBend:
  ret void
}

declare void @llvm.dbg.declare(metadata, metadata) #1

attributes #0 = { alwaysinline nounwind }
attributes #1 = { nounwind readnone }

;; This hack named metadata is needed to assure metadata order
!hack_order = !{!0, !1, !2, !3, !4, !5, !6, !7, !8, !9, !10, !11, !12, !13}

!igc.functions = !{!13}

!0 = !{ }
!1 = !{i32 3, i32 0, !0, null}
!2 = !{i32 4, i32 0, !0, null}
!3 = !{i32 5, i32 0, !0, null}
!4 = !{i32 6, i32 0, !0, null}
!5 = !{i32 786688, !0, !"a", !0, i32 3, !0, i32 0, i32 0}
!6 = !{i32 786688, !0, !"b", !0, i32 3, !0, i32 0, i32 0}
!7 = !{!"function_type", i32 0}
!8 = !{i32 0}   ;; R0
!9 = !{i32 1}   ;; PAYLOAD_HEADER
!10 = !{i32 12} ;; PRIVATE_BASE
!11 = !{!"implicit_arg_desc", !8, !9, !10}
!12 = !{!7, !11}
!13 = !{void (i32, float addrspace(1)*, <8 x i32>, <8 x i32>, i8*)* @test, !12}
