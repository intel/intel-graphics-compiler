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
; RUN: opt -igc-opencl-printf-resolution -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that OpenCLPrintfResolution pass handles line debug info.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a64:64:64-f80:128:128-n8:16:32:64"
target triple = "igil_64_GEN8"

@.str = private addrspace(2) unnamed_addr constant [30 x i8] c"%v4f, %p, %d, %ld, %d, %d, %s\00", align 1
@.str1 = private addrspace(2) unnamed_addr constant [12 x i8] c"string-test\00", align 1
@.str2 = private addrspace(2) unnamed_addr constant [14 x i8] c"second printf\00", align 1

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Check these cases:
;; 1st printf checks these types: float4, int*, int, long, char, short, char*
;; 2nd printf takes only format string,
;;            and checks that pass works fine with more than one printf.
;; The module checks 64bit as it covers more cases in the implementation
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define void @test(<4 x float> %src1, i32 addrspace(1)* %src2, i64 %src3, i8 signext %src4, i16 signext %src5, i8 addrspace(1)* %printfBuffer) #0 {
entry:
  %arrayidx = getelementptr inbounds i32 addrspace(1)* %src2, i32 0, !dbg !1
  %0 = load i32 addrspace(1)* %arrayidx, align 4, !dbg !1
  %conv = sext i8 %src4 to i32, !dbg !1
  %conv1 = sext i16 %src5 to i32, !dbg !1
  %1 = getelementptr inbounds [30 x i8] addrspace(2)* @.str, i32 0, i32 0
  %2 = getelementptr inbounds [12 x i8] addrspace(2)* @.str1, i32 0, i32 0
  %call = call i32 (i8 addrspace(2)*, ...)* @printf(i8 addrspace(2)* %1, <4 x float> %src1, i32 addrspace(1)* %src2, i32 %0, i64 %src3, i32 %conv, i32 %conv1, i8 addrspace(2)* %2), !dbg !2
  br label %BB

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 1st printf checks
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; CHECK-NOT: call i32 (i8 addrspace(2)*, ...)* @printf
; CHECK: [[vec_elem:%[a-zA-Z0-9_]+]] = extractelement <4 x float> %src1, i32 0, !dbg !2
; CHECK: [[vec_elem1:%[a-zA-Z0-9_]+]] = extractelement <4 x float> %src1, i32 1, !dbg !2
; CHECK: [[vec_elem2:%[a-zA-Z0-9_]+]] = extractelement <4 x float> %src1, i32 2, !dbg !2
; CHECK: [[vec_elem3:%[a-zA-Z0-9_]+]] = extractelement <4 x float> %src1, i32 3, !dbg !2
; CHECK: [[inst3:%[a-zA-Z0-9_]+]] = ptrtoint i32 addrspace(1)* %src2 to i64, !dbg !2
; CHECK: [[write_offset:%[a-zA-Z0-9_]+]] = call i32 @__builtin_IB_atomic_add_global_u32(i8 addrspace(1)* %printfBuffer, i32 92), !dbg !2
; CHECK: [[end_offset:%[a-zA-Z0-9_]+]] = add i32 [[write_offset]], 92, !dbg !2
; CHECK: [[write_offset4:%[a-zA-Z0-9_]+]] = zext i32 [[write_offset]] to i64, !dbg !2
; CHECK: [[buffer_ptr:%[a-zA-Z0-9_]+]] = ptrtoint i8 addrspace(1)* %printfBuffer to i64, !dbg !2
; CHECK: [[write_offset5:%[a-zA-Z0-9_]+]] = add i64 [[buffer_ptr]], [[write_offset4]], !dbg !2
; CHECK: [[inst4:%[a-zA-Z0-9_]+]] = icmp ule i32 [[end_offset]], 4194304, !dbg !2
; CHECK: br i1 [[inst4]], label %[[write_offset_true:[a-zA-Z0-9_]+]], label %[[write_offset_false:[a-zA-Z0-9_]+]], !dbg !2

; CHECK: [[write_offset_true]]:                                ; preds = %entry

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Handle of format string
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; CHECK: [[write_offset_ptr:%[a-zA-Z0-9_]+]] = inttoptr i64 [[write_offset5]] to i32 addrspace(1)*, !dbg !2
; CHECK: store i32 0, i32 addrspace(1)* [[write_offset_ptr]], align 1, !dbg !2
; CHECK: [[write_offset6:%[a-zA-Z0-9_]+]] = add i64 [[write_offset5]], 4, !dbg !2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Handle of type float4 %v4f
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; CHECK: [[write_offset_ptr7:%[a-zA-Z0-9_]+]] = inttoptr i64 [[write_offset6]] to i32 addrspace(1)*, !dbg !2
; CHECK: store i32 4, i32 addrspace(1)* [[write_offset_ptr7]], align 1, !dbg !2
; CHECK: [[write_offset8:%[a-zA-Z0-9_]+]] = add i64 [[write_offset6]], 4, !dbg !2
; CHECK: [[write_offset_ptr9:%[a-zA-Z0-9_]+]] = inttoptr i64 [[write_offset8]] to float addrspace(1)*, !dbg !2
; CHECK: store float [[vec_elem]], float addrspace(1)* [[write_offset_ptr9]], align 1, !dbg !2
; CHECK: [[write_offset10:%[a-zA-Z0-9_]+]] = add i64 [[write_offset8]], 4, !dbg !2
; CHECK: [[write_offset_ptr11:%[a-zA-Z0-9_]+]] = inttoptr i64 [[write_offset10]] to i32 addrspace(1)*, !dbg !2
; CHECK: store i32 4, i32 addrspace(1)* [[write_offset_ptr11]], align 1, !dbg !2
; CHECK: [[write_offset12:%[a-zA-Z0-9_]+]] = add i64 [[write_offset10]], 4, !dbg !2
; CHECK: [[write_offset_ptr13:%[a-zA-Z0-9_]+]] = inttoptr i64 [[write_offset12]] to float addrspace(1)*, !dbg !2
; CHECK: store float [[vec_elem1]], float addrspace(1)* [[write_offset_ptr13]], align 1, !dbg !2
; CHECK: [[write_offset14:%[a-zA-Z0-9_]+]] = add i64 [[write_offset12]], 4, !dbg !2
; CHECK: [[write_offset_ptr15:%[a-zA-Z0-9_]+]] = inttoptr i64 [[write_offset14]] to i32 addrspace(1)*, !dbg !2
; CHECK: store i32 4, i32 addrspace(1)* [[write_offset_ptr15]], align 1, !dbg !2
; CHECK: [[write_offset16:%[a-zA-Z0-9_]+]] = add i64 [[write_offset14]], 4, !dbg !2
; CHECK: [[write_offset_ptr17:%[a-zA-Z0-9_]+]] = inttoptr i64 [[write_offset16]] to float addrspace(1)*, !dbg !2
; CHECK: store float [[vec_elem2]], float addrspace(1)* [[write_offset_ptr17]], align 1, !dbg !2
; CHECK: [[write_offset18:%[a-zA-Z0-9_]+]] = add i64 [[write_offset16]], 4, !dbg !2
; CHECK: [[write_offset_ptr19:%[a-zA-Z0-9_]+]] = inttoptr i64 [[write_offset18]] to i32 addrspace(1)*, !dbg !2
; CHECK: store i32 4, i32 addrspace(1)* [[write_offset_ptr19]], align 1, !dbg !2
; CHECK: [[write_offset20:%[a-zA-Z0-9_]+]] = add i64 [[write_offset18]], 4, !dbg !2
; CHECK: [[write_offset_ptr21:%[a-zA-Z0-9_]+]] = inttoptr i64 [[write_offset20]] to float addrspace(1)*, !dbg !2
; CHECK: store float [[vec_elem3]], float addrspace(1)* [[write_offset_ptr21]], align 1, !dbg !2
; CHECK: [[write_offset22:%[a-zA-Z0-9_]+]] = add i64 [[write_offset20]], 4, !dbg !2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Handle of type int* %p
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; CHECK: [[write_offset_ptr23:%[a-zA-Z0-9_]+]] = inttoptr i64 [[write_offset22]] to i32 addrspace(1)*, !dbg !2
; CHECK: store i32 7, i32 addrspace(1)* [[write_offset_ptr23]], align 1, !dbg !2
; CHECK: [[write_offset24:%[a-zA-Z0-9_]+]] = add i64 [[write_offset22]], 4, !dbg !2
; CHECK: [[write_offset_ptr25:%[a-zA-Z0-9_]+]] = inttoptr i64 [[write_offset24]] to i64 addrspace(1)*, !dbg !2
; CHECK: store i64 [[inst3]], i64 addrspace(1)* [[write_offset_ptr25]], align 1, !dbg !2
; CHECK: [[write_offset26:%[a-zA-Z0-9_]+]] = add i64 [[write_offset24]], 8, !dbg !2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Handle of type int %d
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; CHECK: [[write_offset_ptr27:%[a-zA-Z0-9_]+]] = inttoptr i64 [[write_offset26]] to i32 addrspace(1)*, !dbg !2
; CHECK: store i32 3, i32 addrspace(1)* [[write_offset_ptr27]], align 1, !dbg !2
; CHECK: [[write_offset28:%[a-zA-Z0-9_]+]] = add i64 [[write_offset26]], 4, !dbg !2
; CHECK: [[write_offset_ptr29:%[a-zA-Z0-9_]+]] = inttoptr i64 [[write_offset28]] to i32 addrspace(1)*, !dbg !2
; CHECK: store i32 %0, i32 addrspace(1)* [[write_offset_ptr29]], align 1, !dbg !2
; CHECK: [[write_offset30:%[a-zA-Z0-9_]+]] = add i64 [[write_offset28]], 4, !dbg !2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Handle of type long %ld
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; CHECK: [[write_offset_ptr31:%[a-zA-Z0-9_]+]] = inttoptr i64 [[write_offset30]] to i32 addrspace(1)*, !dbg !2
; CHECK: store i32 6, i32 addrspace(1)* [[write_offset_ptr31]], align 1, !dbg !2
; CHECK: [[write_offset32:%[a-zA-Z0-9_]+]] = add i64 [[write_offset30]], 4, !dbg !2
; CHECK: [[write_offset_ptr33:%[a-zA-Z0-9_]+]] = inttoptr i64 [[write_offset32]] to i64 addrspace(1)*, !dbg !2
; CHECK: store i64 %src3, i64 addrspace(1)* [[write_offset_ptr33]], align 1, !dbg !2
; CHECK: [[write_offset34:%[a-zA-Z0-9_]+]] = add i64 [[write_offset32]], 8, !dbg !2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Handle of type char %d
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; CHECK: [[write_offset_ptr35:%[a-zA-Z0-9_]+]] = inttoptr i64 [[write_offset34]] to i32 addrspace(1)*, !dbg !2
; CHECK: store i32 3, i32 addrspace(1)* [[write_offset_ptr35]], align 1, !dbg !2
; CHECK: [[write_offset36:%[a-zA-Z0-9_]+]] = add i64 [[write_offset34]], 4, !dbg !2
; CHECK: [[write_offset_ptr37:%[a-zA-Z0-9_]+]] = inttoptr i64 [[write_offset36]] to i32 addrspace(1)*, !dbg !2
; CHECK: store i32 %conv, i32 addrspace(1)* [[write_offset_ptr37]], align 1, !dbg !2
; CHECK: [[write_offset38:%[a-zA-Z0-9_]+]] = add i64 [[write_offset36]], 4, !dbg !2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Handle of type short %d
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; CHECK: [[write_offset_ptr39:%[a-zA-Z0-9_]+]] = inttoptr i64 [[write_offset38]] to i32 addrspace(1)*, !dbg !2
; CHECK: store i32 3, i32 addrspace(1)* [[write_offset_ptr39]], align 1, !dbg !2
; CHECK: [[write_offset40:%[a-zA-Z0-9_]+]] = add i64 [[write_offset38]], 4, !dbg !2
; CHECK: [[write_offset_ptr41:%[a-zA-Z0-9_]+]] = inttoptr i64 [[write_offset40]] to i32 addrspace(1)*, !dbg !2
; CHECK: store i32 %conv1, i32 addrspace(1)* [[write_offset_ptr41]], align 1, !dbg !2
; CHECK: [[write_offset42:%[a-zA-Z0-9_]+]] = add i64 [[write_offset40]], 4, !dbg !2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Handle of type char* %s
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; CHECK: [[write_offset_ptr43:%[a-zA-Z0-9_]+]] = inttoptr i64 [[write_offset42]] to i32 addrspace(1)*, !dbg !2
; CHECK: store i32 5, i32 addrspace(1)* [[write_offset_ptr43]], align 1, !dbg !2
; CHECK: [[write_offset44:%[a-zA-Z0-9_]+]] = add i64 [[write_offset42]], 4, !dbg !2
; CHECK: [[write_offset_ptr45:%[a-zA-Z0-9_]+]] = inttoptr i64 [[write_offset44]] to i32 addrspace(1)*, !dbg !2
; CHECK: store i32 1, i32 addrspace(1)* [[write_offset_ptr45]], align 1, !dbg !2
; CHECK: [[write_offset46:%[a-zA-Z0-9_]+]] = add i64 [[write_offset44]], 4, !dbg !2
; CHECK: br label %[[bblockJoin:[a-zA-Z0-9_]+]], !dbg !2

; CHECK: [[write_offset_false]]:                               ; preds = %entry
; CHECK: [[end_offset47:%[a-zA-Z0-9_]+]] = add i32 [[write_offset]], 4, !dbg !2
; CHECK: [[inst5:%[a-zA-Z0-9_]+]] = icmp ule i32 [[end_offset47]], 4194304, !dbg !2
; CHECK: br i1 [[inst5]], label %[[write_error_string:[a-zA-Z0-9_]+]], label %[[bblockFalseJoin:[a-zA-Z0-9_]+]], !dbg !2

; CHECK: [[write_error_string]]:                               ; preds = %[[write_offset_false]]

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Handle of error message string
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; CHECK: [[write_offset_ptr48:%[a-zA-Z0-9_]+]] = inttoptr i64 [[write_offset5]] to i32 addrspace(1)*, !dbg !2
; CHECK: store i32 -1, i32 addrspace(1)* [[write_offset_ptr48]], align 1, !dbg !2
; CHECK: br label %[[bblockFalseJoin]], !dbg !2

; CHECK: [[bblockFalseJoin]]:                                  ; preds = %[[write_error_string]], %[[write_offset_false]]
; CHECK: br label %[[bblockJoin]], !dbg !2

; CHECK: [[bblockJoin]]:                                       ; preds = %[[bblockFalseJoin]], %[[write_offset_true]]
; CHECK: [[printf_ret_val:%[a-zA-Z0-9_]+]] = select i1 %4, i32 0, i32 -1, !dbg !2

; CHECK: BB:
BB:
  %3 = getelementptr inbounds [14 x i8] addrspace(2)* @.str2, i32 0, i32 0
  %call2 = call i32 (i8 addrspace(2)*, ...)* @printf(i8 addrspace(2)* %3), !dbg !3

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 2nd printf checks
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; CHECK-NOT: call i32 (i8 addrspace(2)*, ...)* @printf
; CHECK: [[inst6:%[a-zA-Z0-9_]+]] = getelementptr inbounds [14 x i8] addrspace(2)* @.str2, i32 0, i32 0
; CHECK: [[write_offset49:%[a-zA-Z0-9_]+]] = call i32 @__builtin_IB_atomic_add_global_u32(i8 addrspace(1)* %printfBuffer, i32 4), !dbg !3
; CHECK: [[end_offset50:%[a-zA-Z0-9_]+]] = add i32 [[write_offset49]], 4, !dbg !3
; CHECK: [[write_offset51:%[a-zA-Z0-9_]+]] = zext i32 [[write_offset49]] to i64, !dbg !3
; CHECK: [[buffer_ptr52:%[a-zA-Z0-9_]+]] = ptrtoint i8 addrspace(1)* %printfBuffer to i64, !dbg !3
; CHECK: [[write_offset53:%[a-zA-Z0-9_]+]] = add i64 [[buffer_ptr52]], [[write_offset51]], !dbg !3
; CHECK: [[inst7:%[a-zA-Z0-9_]+]] = icmp ule i32 [[end_offset50]], 4194304, !dbg !3
; CHECK: br i1 [[inst7]], label %[[write_offset_true55:[a-zA-Z0-9_]+]], label %[[write_offset_false56:[a-zA-Z0-9_]+]], !dbg !3

; CHECK: [[write_offset_true55]]:                              ; preds = %BB

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Handle of format string
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; CHECK: [[write_offset_ptr57:%[a-zA-Z0-9_]+]] = inttoptr i64 [[write_offset53]] to i32 addrspace(1)*, !dbg !3
; CHECK: store i32 2, i32 addrspace(1)* [[write_offset_ptr57]], align 1, !dbg !3
; CHECK: [[write_offset58:%[a-zA-Z0-9_]+]] = add i64 [[write_offset53]], 4, !dbg !3
; CHECK: br label %[[bblockJoin54:[a-zA-Z0-9_]+]], !dbg !3

; CHECK: [[write_offset_false56]]:                             ; preds = %BB
; CHECK: [[end_offset59:%[a-zA-Z0-9_]+]] = add i32 [[write_offset49]], 4, !dbg !3
; CHECK: [[inst8:%[a-zA-Z0-9_]+]] = icmp ule i32 [[end_offset59]], 4194304, !dbg !3
; CHECK: br i1 [[inst8]], label %[[write_error_string60:[a-zA-Z0-9_]+]], label %[[bblockFalseJoin61:[a-zA-Z0-9_]+]], !dbg !3

; CHECK: [[write_error_string60]]:                             ; preds = %[[write_offset_false56]]

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Handle of error message string
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; CHECK: [[write_offset_ptr62:%[a-zA-Z0-9_]+]] = inttoptr i64 [[write_offset53]] to i32 addrspace(1)*, !dbg !3
; CHECK: store i32 -1, i32 addrspace(1)* [[write_offset_ptr62]], align 1, !dbg !3
; CHECK: br label %[[bblockFalseJoin61]], !dbg !3

; CHECK: [[bblockFalseJoin61]]:                                ; preds = %[[write_error_string60]], %[[write_offset_false56]]
; CHECK: br label %[[bblockJoin54]], !dbg !3

; CHECK: [[bblockJoin54]]:                                     ; preds = %[[bblockFalseJoin61]], %[[write_offset_true55]]
; CHECK: [[printf_ret_val63:%[a-zA-Z0-9_]+]] = select i1 [[inst7]], i32 0, i32 -1, !dbg !3
; CHECK: ret void
  ret void
}

declare i32 @printf(i8 addrspace(2)*, ...)

attributes #0 = { nounwind }


;; This hack named metadata is needed to assure metadata order
!hack_order = !{!0, !1, !2, !3, !4, !5, !6, !7, !8, !9, !10, !11}

!igc.functions = !{!8}
!igc.inline.programscope.offsets = !{!9, !10, !11}

!0 = metadata !{}
!1 = metadata !{i32 5, i32 0, metadata !0, null}
!2 = metadata !{i32 6, i32 0, metadata !0, null}
!3 = metadata !{i32 7, i32 0, metadata !0, null}
!4 = metadata !{metadata !"function_type", i32 0}
!5 = metadata !{i32 13} ;; PRINTF_BUFFER
!6 = metadata !{metadata !"implicit_arg_desc", metadata !5}
!7 = metadata !{metadata !4, metadata !6}
!8 = metadata !{void (<4 x float>, i32 addrspace(1)*, i64, i8, i16, i8 addrspace(1)*)* @test, metadata !7}
!9 = metadata !{[12 x i8] addrspace(2)* @.str1, i32 30}
!10 = metadata !{[30 x i8] addrspace(2)* @.str, i32 0}
!11 = metadata !{[14 x i8] addrspace(2)* @.str2, i32 42}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; metadata describes the strings used in the above printfs
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; CHECK: !printf.strings.test = !{[[metadata12:![a-zA-Z0-9_]+]], [[metadata13:![a-zA-Z0-9_]+]], [[metadata14:![a-zA-Z0-9_]+]]}

; CHECK: [[metadata12]] = metadata !{i32 0, metadata !"%v4f, %p, %d, %ld, %d, %d, %s"}
; CHECK: [[metadata13]] = metadata !{i32 1, metadata !"string-test"}
; CHECK: [[metadata14]] = metadata !{i32 2, metadata !"second printf"}
