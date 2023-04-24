;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPostLegalization -march=genx64 -mcpu=Gen9 -mtriple=spir64 -S < %s | FileCheck %s

;; Test legalization of constants as return values (constant loader).

target datalayout = "e-p:64:64-i64:64-n8:16:32"

define <3 x i64> @legalize_return_integer() {
; CHECK-LABEL: @legalize_return_integer(
; CHECK-NEXT:    [[SPLIT1:%.+]] = call <3 x i64> @llvm.genx.wrregioni.v3i64.v2i64.i16.i1(<3 x i64> undef, <2 x i64> zeroinitializer, i32 2, i32 2, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT:    [[SPLIT2:%.+]] = call <3 x i64> @llvm.genx.wrregioni.v3i64.v1i64.i16.i1(<3 x i64> [[SPLIT1:%.+]], <1 x i64> zeroinitializer, i32 1, i32 1, i32 1, i16 16, i32 undef, i1 true)
; CHECK-NEXT:    ret <3 x i64> [[SPLIT2:%.+]]
  ret <3 x i64> zeroinitializer
}

define <3 x double> @legalize_return_double() {
; CHECK-LABEL: @legalize_return_double(
; CHECK-NEXT:    [[SPLIT1:%.+]] = call <3 x double> @llvm.genx.wrregionf.v3f64.v2f64.i16.i1(<3 x double> undef, <2 x double> zeroinitializer, i32 2, i32 2, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT:    [[SPLIT2:%.+]] = call <3 x double> @llvm.genx.wrregionf.v3f64.v1f64.i16.i1(<3 x double> [[SPLIT1]], <1 x double> zeroinitializer, i32 1, i32 1, i32 1, i16 16, i32 undef, i1 true)
; CHECK-NEXT:    ret <3 x double> [[SPLIT2]]
  ret <3 x double> zeroinitializer
}

define <3 x i8*> @legalize_return_nullptr_vec() {
; CHECK-LABEL: @legalize_return_nullptr_vec(
; CHECK-NEXT:    [[SPLIT1:%.+]] = call <3 x i8*> @llvm.genx.wrregioni.v3p0i8.v2p0i8.i16.i1(<3 x i8*> undef, <2 x i8*> zeroinitializer, i32 2, i32 2, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT:    [[SPLIT2:%.+]] = call <3 x i8*> @llvm.genx.wrregioni.v3p0i8.v1p0i8.i16.i1(<3 x i8*> [[SPLIT1]], <1 x i8*> zeroinitializer, i32 1, i32 1, i32 1, i16 16, i32 undef, i1 true)
; CHECK-NEXT:    ret <3 x i8*> [[SPLIT2]]
  ret <3 x i8*> zeroinitializer
}

define { <4 x i32> } @const_struct_return() {
; CHECK-LABEL: @const_struct_return
; CHECK-NEXT: [[CONST:%.+]] = call <1 x i32> @llvm.genx.constanti.v1i32(<1 x i32> <i32 1>)
; CHECK-NEXT: [[SPLAT:%.+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v1i32.i16(<1 x i32> [[CONST]], i32 0, i32 4, i32 0, i16 0, i32 undef)
; CHECK-NEXT: [[STRUCT:%.+]] = insertvalue { <4 x i32> } undef, <4 x i32> [[SPLAT]], 0
; CHECK-NEXT: ret { <4 x i32> } [[STRUCT]]
  ret { <4 x i32> } { <4 x i32> <i32 1, i32 1, i32 1, i32 1> }
}

define { <16 x i32> } @const_struct_return_big() {
; CHECK-LABEL: @const_struct_return_big
; CHECK-NEXT: [[CONST1:%.+]] = call <8 x i32> @llvm.genx.constanti.v8i32(<8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>)
; CHECK-NEXT: [[SPLIT1:%.+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> [[CONST1]], i32 8, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[CONST2:%.+]] = call <8 x i32> @llvm.genx.constanti.v8i32(<8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 10, i32 9, i32 8, i32 7>)
; CHECK-NEXT: [[SPLIT2:%.+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> [[SPLIT1]], <8 x i32> [[CONST2]], i32 8, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-NEXT: [[VAL:%.+]] = insertvalue { <16 x i32> } undef, <16 x i32> [[SPLIT2]], 0
; CHECK-NEXT: ret { <16 x i32> } [[VAL]]
  ret { <16 x i32> } { <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 10, i32 9, i32 8, i32 7> }
}

; For huge const
define { <16 x i32> } @const_struct_add_return_big() {
; CHECK-LABEL: @const_struct_add_return_big
; CHECK-NEXT: [[CONST1:%.+]] = call <8 x i32> @llvm.genx.constanti.v8i32(<8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>)
; CHECK-NEXT: [[SPLIT1:%.+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> [[CONST1]], i32 8, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[PRE_CONST_ADD:%.+]] = call <8 x i32> @llvm.genx.constanti.v8i32(<8 x i32> <i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8>)
; CHECK-NEXT: [[ADD:%.+]] = add <8 x i32> [[CONST1]], [[PRE_CONST_ADD]]
; CHECK-NEXT: [[SPLIT2:%.+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> [[SPLIT1]], <8 x i32> [[ADD]], i32 8, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-NEXT: [[VAL:%.+]] = insertvalue { <16 x i32> } undef, <16 x i32> [[SPLIT2]], 0
; CHECK-NEXT: ret { <16 x i32> } [[VAL]]
  ret { <16 x i32> } { <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15> }
}

define { <16 x i32> } @const_big_undef_return() {
; COM: undef values are left "as is"
; CHECK-LABEL: @const_big_undef_return
; CHECK-NEXT: ret { <16 x i32> } undef
  ret { <16 x i32> } undef
}

define { <3 x i32> } @legalize_struct_const_return() {
; COM: ConstantLoader properly loads <3 x i32> vector, however PostLegalization
; COM: also runs simplifyRegionInsts, thats folds the sequence of wrregioni O^o
; COM: Though, the goal is archived, since ret does not have a constant as
; COM: it's operand
; CHECK-LABEL: @legalize_struct_const_return
; CHECK-NEXT: [[STRUCT:%.+]] = insertvalue { <3 x i32> } undef, <3 x i32> <i32 1, i32 1, i32 1>, 0
; CHECK-NEXT: ret { <3 x i32> } [[STRUCT]]
  ret { <3 x i32> } { <3 x i32> <i32 1, i32 1, i32 1> }
}
