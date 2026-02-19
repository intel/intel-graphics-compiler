;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -types-legalization-pass | FileCheck %s

define spir_kernel void @f0([120004 x double] addrspace(1)* addrspace(1)* %arg) {
entry:
  %ptr = load [120004 x double] addrspace(1)*, [120004 x double] addrspace(1)* addrspace(1)* %arg, align 8
  store [120004 x double] zeroinitializer, [120004 x double] addrspace(1)* %ptr, align 8
  ; CHECK:    [[TMP0:%.*]] = bitcast [120004 x double] addrspace(1)* %ptr to i8 addrspace(1)*
  ; CHECK:    call void @llvm.memset.p1i8.i64(i8 addrspace(1)* align 8 [[TMP0]], i8 0, i64 960032, i1 false)
  ret void
}


define spir_kernel void @f1([10 x double] addrspace(1)* %arg) {
entry:
  store [10 x double] [double 1.000000e+00, double 2.000000e+00, double 3.000000e+00, double 4.000000e+00, double 5.000000e+00, double 6.000000e+00, double 7.000000e+00, double 8.000000e+00, double 9.000000e+00, double 10.000000e+00], [10 x double] addrspace(1)* %arg, align 8
  ; CHECK:    [[TMP0:%.*]] = getelementptr [10 x double], [10 x double] addrspace(1)* [[ARG:%.*]], i32 0, i32 0
  ; CHECK:    store double 1.000000e+00, double addrspace(1)* [[TMP0]]
  ; CHECK:    [[TMP1:%.*]] = getelementptr [10 x double], [10 x double] addrspace(1)* [[ARG]], i32 0, i32 1
  ; CHECK:    store double 2.000000e+00, double addrspace(1)* [[TMP1]]
  ; CHECK:    [[TMP2:%.*]] = getelementptr [10 x double], [10 x double] addrspace(1)* [[ARG]], i32 0, i32 2
  ; CHECK:    store double 3.000000e+00, double addrspace(1)* [[TMP2]]
  ; CHECK:    [[TMP3:%.*]] = getelementptr [10 x double], [10 x double] addrspace(1)* [[ARG]], i32 0, i32 3
  ; CHECK:    store double 4.000000e+00, double addrspace(1)* [[TMP3]]
  ; CHECK:    [[TMP4:%.*]] = getelementptr [10 x double], [10 x double] addrspace(1)* [[ARG]], i32 0, i32 4
  ; CHECK:    store double 5.000000e+00, double addrspace(1)* [[TMP4]]
  ; CHECK:    [[TMP5:%.*]] = getelementptr [10 x double], [10 x double] addrspace(1)* [[ARG]], i32 0, i32 5
  ; CHECK:    store double 6.000000e+00, double addrspace(1)* [[TMP5]]
  ; CHECK:    [[TMP6:%.*]] = getelementptr [10 x double], [10 x double] addrspace(1)* [[ARG]], i32 0, i32 6
  ; CHECK:    store double 7.000000e+00, double addrspace(1)* [[TMP6]]
  ; CHECK:    [[TMP7:%.*]] = getelementptr [10 x double], [10 x double] addrspace(1)* [[ARG]], i32 0, i32 7
  ; CHECK:    store double 8.000000e+00, double addrspace(1)* [[TMP7]]
  ; CHECK:    [[TMP8:%.*]] = getelementptr [10 x double], [10 x double] addrspace(1)* [[ARG]], i32 0, i32 8
  ; CHECK:    store double 9.000000e+00, double addrspace(1)* [[TMP8]]
  ; CHECK:    [[TMP9:%.*]] = getelementptr [10 x double], [10 x double] addrspace(1)* [[ARG]], i32 0, i32 9
  ; CHECK:    store double 1.000000e+01, double addrspace(1)* [[TMP9]]
  ret void
}

define spir_kernel void @f2([10 x double] addrspace(1)* %arg) {
entry:
  store [10 x double] [double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00], [10 x double] addrspace(1)* %arg, align 8
  ; CHECK:    [[TMP0:%.*]] = getelementptr [10 x double], [10 x double] addrspace(1)* [[ARG:%.*]], i32 0, i32 0
  ; CHECK:    store double 1.000000e+00, double addrspace(1)* [[TMP0]]
  ; CHECK:    [[TMP1:%.*]] = getelementptr [10 x double], [10 x double] addrspace(1)* [[ARG]], i32 0, i32 1
  ; CHECK:    store double 1.000000e+00, double addrspace(1)* [[TMP1]]
  ; CHECK:    [[TMP2:%.*]] = getelementptr [10 x double], [10 x double] addrspace(1)* [[ARG]], i32 0, i32 2
  ; CHECK:    store double 1.000000e+00, double addrspace(1)* [[TMP2]]
  ; CHECK:    [[TMP3:%.*]] = getelementptr [10 x double], [10 x double] addrspace(1)* [[ARG]], i32 0, i32 3
  ; CHECK:    store double 1.000000e+00, double addrspace(1)* [[TMP3]]
  ; CHECK:    [[TMP4:%.*]] = getelementptr [10 x double], [10 x double] addrspace(1)* [[ARG]], i32 0, i32 4
  ; CHECK:    store double 1.000000e+00, double addrspace(1)* [[TMP4]]
  ; CHECK:    [[TMP5:%.*]] = getelementptr [10 x double], [10 x double] addrspace(1)* [[ARG]], i32 0, i32 5
  ; CHECK:    store double 1.000000e+00, double addrspace(1)* [[TMP5]]
  ; CHECK:    [[TMP6:%.*]] = getelementptr [10 x double], [10 x double] addrspace(1)* [[ARG]], i32 0, i32 6
  ; CHECK:    store double 1.000000e+00, double addrspace(1)* [[TMP6]]
  ; CHECK:    [[TMP7:%.*]] = getelementptr [10 x double], [10 x double] addrspace(1)* [[ARG]], i32 0, i32 7
  ; CHECK:    store double 1.000000e+00, double addrspace(1)* [[TMP7]]
  ; CHECK:    [[TMP8:%.*]] = getelementptr [10 x double], [10 x double] addrspace(1)* [[ARG]], i32 0, i32 8
  ; CHECK:    store double 1.000000e+00, double addrspace(1)* [[TMP8]]
  ; CHECK:    [[TMP9:%.*]] = getelementptr [10 x double], [10 x double] addrspace(1)* [[ARG]], i32 0, i32 9
  ; CHECK:    store double 1.000000e+00, double addrspace(1)* [[TMP9]]
  ret void
}

define spir_kernel void @f3([8 x <2 x double>] addrspace(1)* %arg) {
entry:
  store [8 x <2 x double>] [<2 x double> <double 0.000000e+00, double 0.000000e+00>, <2 x double> <double 0.000000e+00, double 0.000000e+00>, <2 x double> <double 0.000000e+00, double 0.000000e+00>, <2 x double> <double 0.000000e+00, double 0.000000e+00>, <2 x double> <double 0.000000e+00, double 0.000000e+00>, <2 x double> <double 0.000000e+00, double 0.000000e+00>, <2 x double> <double 0.000000e+00, double 0.000000e+00>, <2 x double> <double 0.000000e+00, double 0.000000e+00>], [8 x <2 x double>] addrspace(1)* %arg, align 8
  ; CHECK:    [[TMP0:%.*]] = getelementptr [8 x <2 x double>], [8 x <2 x double>] addrspace(1)* [[ARG:%.*]], i32 0, i32 0
  ; CHECK:    store <2 x double> zeroinitializer, <2 x double> addrspace(1)* [[TMP0]]
  ; CHECK:    [[TMP1:%.*]] = getelementptr [8 x <2 x double>], [8 x <2 x double>] addrspace(1)* [[ARG]], i32 0, i32 1
  ; CHECK:    store <2 x double> zeroinitializer, <2 x double> addrspace(1)* [[TMP1]]
  ; CHECK:    [[TMP2:%.*]] = getelementptr [8 x <2 x double>], [8 x <2 x double>] addrspace(1)* [[ARG]], i32 0, i32 2
  ; CHECK:    store <2 x double> zeroinitializer, <2 x double> addrspace(1)* [[TMP2]]
  ; CHECK:    [[TMP3:%.*]] = getelementptr [8 x <2 x double>], [8 x <2 x double>] addrspace(1)* [[ARG]], i32 0, i32 3
  ; CHECK:    store <2 x double> zeroinitializer, <2 x double> addrspace(1)* [[TMP3]]
  ; CHECK:    [[TMP4:%.*]] = getelementptr [8 x <2 x double>], [8 x <2 x double>] addrspace(1)* [[ARG]], i32 0, i32 4
  ; CHECK:    store <2 x double> zeroinitializer, <2 x double> addrspace(1)* [[TMP4]]
  ; CHECK:    [[TMP5:%.*]] = getelementptr [8 x <2 x double>], [8 x <2 x double>] addrspace(1)* [[ARG]], i32 0, i32 5
  ; CHECK:    store <2 x double> zeroinitializer, <2 x double> addrspace(1)* [[TMP5]]
  ; CHECK:    [[TMP6:%.*]] = getelementptr [8 x <2 x double>], [8 x <2 x double>] addrspace(1)* [[ARG]], i32 0, i32 6
  ; CHECK:    store <2 x double> zeroinitializer, <2 x double> addrspace(1)* [[TMP6]]
  ; CHECK:    [[TMP7:%.*]] = getelementptr [8 x <2 x double>], [8 x <2 x double>] addrspace(1)* [[ARG]], i32 0, i32 7
  ; CHECK:    store <2 x double> zeroinitializer, <2 x double> addrspace(1)* [[TMP7]]
  ret void
}

%simple.struct.type = type { i64, float }
%nested.struct.type = type { i32, %simple.struct.type }
%double.nested.struct.type = type { i32, %nested.struct.type }


define float @cover.dead.loads(%double.nested.struct.type* byval(%double.nested.struct.type) %struct.instance) #0
; CHECK-LABEL: @cover.dead.loads(
; CHECK-NEXT:    [[PTR_TO_INNER_STRUCT:%.*]] = getelementptr [[DOUBLE_NESTED_STRUCT_TYPE:%.*]], %double.nested.struct.type* [[STRUCT_INSTANCE:%.*]], i32 0, i32 1
; CHECK-NEXT:    [[TMP1:%.*]] = getelementptr [[NESTED_STRUCT_TYPE:%.*]], %nested.struct.type* [[PTR_TO_INNER_STRUCT]], i32 0, i32 1, i32 1
; CHECK-NEXT:    [[TMP2:%.*]] = load float, float* [[TMP1]], align 4
; CHECK-NEXT:    [[RESULT:%.*]] = fmul float [[TMP2]], 4.000000e+00
; CHECK-NEXT:    ret float [[RESULT]]
;
{
  %ptr.to.inner.struct = getelementptr %double.nested.struct.type, %double.nested.struct.type* %struct.instance, i32 0, i32 1
  %inner.struct.value = load %nested.struct.type, %nested.struct.type* %ptr.to.inner.struct
  %inner.struct.value.float = extractvalue %nested.struct.type %inner.struct.value, 1, 1
  %result = fmul float %inner.struct.value.float, 4.0
  ret float %result
}

attributes #0 = { optnone noinline }
