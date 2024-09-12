;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -remove-loop-dependency -S %s 2>&1 | FileCheck %s

; Test checks the basic functionality of the pass

define spir_kernel void @test(i32 %index) {
entry:
  br label %header

header:                                           ; preds = %header, %entry
; CHECK:    [[VEC_INORDER_1:%.*]] = phi <4 x i32> [ zeroinitializer, [[ENTRY:%.*]] ], [ zeroinitializer, %header ]
; CHECK-NEXT:    [[VEC_OUTORDER_1:%.*]] = phi <4 x i32> [ zeroinitializer, [[ENTRY]] ], [ zeroinitializer, %header ]
; CHECK-NEXT:    [[VEC_PARTIAL_1:%.*]] = phi <4 x i32> [ zeroinitializer, [[ENTRY]] ], [ zeroinitializer, %header ]
; CHECK-NEXT:    [[VEC_SAMEIND_1:%.*]] = phi <4 x i32> [ zeroinitializer, [[ENTRY]] ], [ zeroinitializer, %header ]
; CHECK-NEXT:    [[VEC_USED_1:%.*]] = phi <4 x i32> [ zeroinitializer, [[ENTRY]] ], [ zeroinitializer, %header ]
; CHECK-NEXT:    [[VEC_NOTLOOP_1:%.*]] = phi <4 x i32> [ zeroinitializer, [[ENTRY]] ], [ zeroinitializer, %header ]
; CHECK-NEXT:    [[VEC_NOTCONSTIND_1:%.*]] = phi <4 x i32> [ zeroinitializer, [[ENTRY]] ], [ zeroinitializer, %header ]
; CHECK-NEXT:    [[VEC_OUTOFRANGE_1:%.*]] = phi <4 x i32> [ zeroinitializer, [[ENTRY]] ], [ zeroinitializer, %header ]
; CHECK-NEXT:    [[SCALAR:%.*]] = phi i32 [ 0, [[ENTRY]] ], [ 0, %header ]
  %vec.inorder.1 = phi <4 x i32> [ zeroinitializer, %entry ], [ zeroinitializer, %header ]
  %vec.outorder.1 = phi <4 x i32> [ zeroinitializer, %entry ], [ zeroinitializer, %header ]
  %vec.partial.1 = phi <4 x i32> [ zeroinitializer, %entry ], [ zeroinitializer, %header ]
  %vec.sameind.1 = phi <4 x i32> [ zeroinitializer, %entry ], [ zeroinitializer, %header ]
  %vec.used.1 = phi <4 x i32> [ zeroinitializer, %entry ], [ zeroinitializer, %header ]
  %vec.notloop.1 = phi <4 x i32> [ zeroinitializer, %entry ], [ zeroinitializer, %header ]
  %vec.notconstind.1 = phi <4 x i32> [ zeroinitializer, %entry ], [ zeroinitializer, %header ]
  %vec.outofrange.1 = phi <4 x i32> [ zeroinitializer, %entry ], [ zeroinitializer, %header ]
  %scalar = phi i32 [ 0, %entry ], [ 0, %header ]

; whole vector is overwritten, so can replace with undef
; CHECK:    [[VEC_INORDER_2:%.*]] = insertelement <4 x i32> undef, i32 5, i32 0
; CHECK-NEXT:    [[VEC_INORDER_3:%.*]] = insertelement <4 x i32> [[VEC_INORDER_2]], i32 5, i32 1
; CHECK-NEXT:    [[VEC_INORDER_4:%.*]] = insertelement <4 x i32> [[VEC_INORDER_3]], i32 5, i32 2
; CHECK-NEXT:    [[VEC_INORDER_5:%.*]] = insertelement <4 x i32> [[VEC_INORDER_4]], i32 5, i32 3
  %vec.inorder.2 = insertelement <4 x i32> %vec.inorder.1, i32 5, i32 0
  %vec.inorder.3 = insertelement <4 x i32> %vec.inorder.2, i32 5, i32 1
  %vec.inorder.4 = insertelement <4 x i32> %vec.inorder.3, i32 5, i32 2
  %vec.inorder.5 = insertelement <4 x i32> %vec.inorder.4, i32 5, i32 3

; not whole vector is overwritten
; CHECK:    [[VEC_OUTOFRANGE_2:%.*]] = insertelement <4 x i32> [[VEC_OUTOFRANGE_1]], i32 5, i32 0
; CHECK-NEXT:    [[VEC_OUTOFRANGE_3:%.*]] = insertelement <4 x i32> [[VEC_OUTOFRANGE_2]], i32 5, i32 1
; CHECK-NEXT:    [[VEC_OUTOFRANGE_4:%.*]] = insertelement <4 x i32> [[VEC_OUTOFRANGE_3]], i32 5, i32 2
; CHECK-NEXT:    [[VEC_OUTOFRANGE_5:%.*]] = insertelement <4 x i32> [[VEC_OUTOFRANGE_4]], i32 5, i32 5
  %vec.outofrange.2 = insertelement <4 x i32> %vec.outofrange.1, i32 5, i32 0
  %vec.outofrange.3 = insertelement <4 x i32> %vec.outofrange.2, i32 5, i32 1
  %vec.outofrange.4 = insertelement <4 x i32> %vec.outofrange.3, i32 5, i32 2
  %vec.outofrange.5 = insertelement <4 x i32> %vec.outofrange.4, i32 5, i32 5

; we don't know if whole vector is overwritten
; CHECK:    [[VEC_NOTCONSTIND_2:%.*]] = insertelement <4 x i32> [[VEC_NOTCONSTIND_1]], i32 5, i32 0
; CHECK-NEXT:    [[VEC_NOTCONSTIND_3:%.*]] = insertelement <4 x i32> [[VEC_NOTCONSTIND_2]], i32 5, i32 1
; CHECK-NEXT:    [[VEC_NOTCONSTIND_4:%.*]] = insertelement <4 x i32> [[VEC_NOTCONSTIND_3]], i32 5, i32 [[INDEX:%.*]]
; CHECK-NEXT:    [[VEC_NOTCONSTIND_5:%.*]] = insertelement <4 x i32> [[VEC_NOTCONSTIND_4]], i32 5, i32 3
  %vec.notconstind.2 = insertelement <4 x i32> %vec.notconstind.1, i32 5, i32 0
  %vec.notconstind.3 = insertelement <4 x i32> %vec.notconstind.2, i32 5, i32 1
  %vec.notconstind.4 = insertelement <4 x i32> %vec.notconstind.3, i32 5, i32 %index
  %vec.notconstind.5 = insertelement <4 x i32> %vec.notconstind.4, i32 5, i32 3

; whole vector is overwritten, just different order of indexes, replace with undef
; CHECK:    [[VEC_OUTORDER_2:%.*]] = insertelement <4 x i32> undef, i32 6, i32 2
; CHECK-NEXT:    [[VEC_OUTORDER_3:%.*]] = insertelement <4 x i32> [[VEC_OUTORDER_2]], i32 6, i32 0
; CHECK-NEXT:    [[VEC_OUTORDER_4:%.*]] = insertelement <4 x i32> [[VEC_OUTORDER_3]], i32 6, i32 3
; CHECK-NEXT:    [[VEC_OUTORDER_5:%.*]] = insertelement <4 x i32> [[VEC_OUTORDER_4]], i32 6, i32 1
  %vec.outorder.2 = insertelement <4 x i32> %vec.outorder.1, i32 6, i32 2
  %vec.outorder.3 = insertelement <4 x i32> %vec.outorder.2, i32 6, i32 0
  %vec.outorder.4 = insertelement <4 x i32> %vec.outorder.3, i32 6, i32 3
  %vec.outorder.5 = insertelement <4 x i32> %vec.outorder.4, i32 6, i32 1

; not whole vector is overwritten
; CHECK:    [[VEC_PARTIAL_2:%.*]] = insertelement <4 x i32> [[VEC_PARTIAL_1]], i32 5, i32 0
; CHECK-NEXT:    [[VEC_PARTIAL_3:%.*]] = insertelement <4 x i32> [[VEC_PARTIAL_2]], i32 5, i32 1
; CHECK-NEXT:    [[VEC_PARTIAL_4:%.*]] = insertelement <4 x i32> [[VEC_PARTIAL_3]], i32 5, i32 2
  %vec.partial.2 = insertelement <4 x i32> %vec.partial.1, i32 5, i32 0
  %vec.partial.3 = insertelement <4 x i32> %vec.partial.2, i32 5, i32 1
  %vec.partial.4 = insertelement <4 x i32> %vec.partial.3, i32 5, i32 2

; not whole vector is overwritten
; CHECK:    [[VEC_SAMEIND_2:%.*]] = insertelement <4 x i32> [[VEC_SAMEIND_1]], i32 5, i32 0
; CHECK-NEXT:    [[VEC_SAMEIND_3:%.*]] = insertelement <4 x i32> [[VEC_SAMEIND_2]], i32 5, i32 1
; CHECK-NEXT:    [[VEC_SAMEIND_4:%.*]] = insertelement <4 x i32> [[VEC_SAMEIND_3]], i32 5, i32 3
; CHECK-NEXT:    [[VEC_SAMEIND_5:%.*]] = insertelement <4 x i32> [[VEC_SAMEIND_4]], i32 5, i32 3
  %vec.sameind.2 = insertelement <4 x i32> %vec.sameind.1, i32 5, i32 0
  %vec.sameind.3 = insertelement <4 x i32> %vec.sameind.2, i32 5, i32 1
  %vec.sameind.4 = insertelement <4 x i32> %vec.sameind.3, i32 5, i32 3
  %vec.sameind.5 = insertelement <4 x i32> %vec.sameind.4, i32 5, i32 3

; vector may be used in the loop before it is overwritten
; CHECK:    [[VEC_USED_2:%.*]] = insertelement <4 x i32> [[VEC_USED_1]], i32 5, i32 0
; CHECK-NEXT:    [[VEC_USED_3:%.*]] = insertelement <4 x i32> [[VEC_USED_2]], i32 5, i32 1
; CHECK-NEXT:    [[VEC_USED_4:%.*]] = insertelement <4 x i32> [[VEC_USED_3]], i32 5, i32 2
; CHECK-NEXT:    [[VEC_USED_5:%.*]] = insertelement <4 x i32> [[VEC_USED_4]], i32 5, i32 3
; CHECK-NEXT:    [[USED_HERE:%.*]] = extractelement <4 x i32> [[VEC_USED_4]], i32 3
  %vec.used.2 = insertelement <4 x i32> %vec.used.1, i32 5, i32 0
  %vec.used.3 = insertelement <4 x i32> %vec.used.2, i32 5, i32 1
  %vec.used.4 = insertelement <4 x i32> %vec.used.3, i32 5, i32 2
  %vec.used.5 = insertelement <4 x i32> %vec.used.4, i32 5, i32 3
  %used.here = extractelement <4 x i32> %vec.used.4, i32 3

; vector is not completely overwritten in the loop
; CHECK:    [[VEC_NOTLOOP_2:%.*]] = insertelement <4 x i32> [[VEC_NOTLOOP_1]], i32 5, i32 0
; CHECK-NEXT:    [[VEC_NOTLOOP_3:%.*]] = insertelement <4 x i32> [[VEC_NOTLOOP_2]], i32 5, i32 1
; CHECK-NEXT:    [[VEC_NOTLOOP_4:%.*]] = insertelement <4 x i32> [[VEC_NOTLOOP_3]], i32 5, i32 2
  %vec.notloop.2 = insertelement <4 x i32> %vec.notloop.1, i32 5, i32 0
  %vec.notloop.3 = insertelement <4 x i32> %vec.notloop.2, i32 5, i32 1
  %vec.notloop.4 = insertelement <4 x i32> %vec.notloop.3, i32 5, i32 2

; scalar
; CHECK:    [[INC:%.*]] = add i32 [[SCALAR]], 1
  %inc = add i32 %scalar, 1
  br i1 true, label %header, label %exit

exit:                                             ; preds = %header
; CHECK:    [[VEC_NOTLOOP_5:%.*]] = insertelement <4 x i32> [[VEC_NOTLOOP_4]], i32 5, i32 3
  %vec.notloop.5 = insertelement <4 x i32> %vec.notloop.4, i32 5, i32 3
  ret void
}

!igc.functions = !{}
