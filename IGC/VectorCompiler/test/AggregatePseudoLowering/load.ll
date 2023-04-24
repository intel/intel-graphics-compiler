;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXAggregatePseudoLowering -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

%simple_st = type { i32, i64 }
%v8_uniform_Data = type { i8*, i64, i32, i8 }
%folded_st = type { i8, %simple_st }
%complex_st = type { %folded_st, i64* }
%st_with_array = type { i8, [2 x <10 x i32>] }

define dllexport void @simple_load() {
; CHECK-LABEL: simple_load

  %simple.ptr = alloca %simple_st
  %res = load %simple_st, %simple_st* %simple.ptr
; CHECK: %[[SIMPLE_GEP_A:[^ ]+]] = getelementptr inbounds %simple_st, %simple_st* %simple.ptr, i32 0, i32 0
; CHECK: %[[SIMPLE_RES_A:[^ ]+]] = load i32, i32* %[[SIMPLE_GEP_A]]
; CHECK: %[[SIMPLE_GEP_B:[^ ]+]] = getelementptr inbounds %simple_st, %simple_st* %simple.ptr, i32 0, i32 1
; CHECK: %[[SIMPLE_RES_B:[^ ]+]] = load i64, i64* %[[SIMPLE_GEP_B]]

; COM: combine the aggregate back
; CHECK: %[[SIMPLE_RES_0:[^ ]+]] = insertvalue %simple_st undef, i32 %[[SIMPLE_RES_A]], 0
; CHECK: %res = insertvalue %simple_st %[[SIMPLE_RES_0]], i64 %[[SIMPLE_RES_B]], 1

  %simple.user = extractvalue %simple_st %res, 0
; CHECK: %simple.user = extractvalue %simple_st %res, 0
  ret void
}

define dllexport void @ispc_load() {
; CHECK-LABEL: ispc_load

  %ispc.ptr = alloca %v8_uniform_Data
  %res = load %v8_uniform_Data, %v8_uniform_Data* %ispc.ptr
; CHECK: %[[ISPC_GEP_A:[^ ]+]] = getelementptr inbounds %v8_uniform_Data, %v8_uniform_Data* %ispc.ptr, i32 0, i32 0
; CHECK: %[[ISPC_RES_A:[^ ]+]] = load i8*, i8** %[[ISPC_GEP_A]]
; CHECK: %[[ISPC_GEP_B:[^ ]+]] = getelementptr inbounds %v8_uniform_Data, %v8_uniform_Data* %ispc.ptr, i32 0, i32 1
; CHECK: %[[ISPC_RES_B:[^ ]+]] = load i64, i64* %[[ISPC_GEP_B]]
; CHECK: %[[ISPC_GEP_C:[^ ]+]] = getelementptr inbounds %v8_uniform_Data, %v8_uniform_Data* %ispc.ptr, i32 0, i32 2
; CHECK: %[[ISPC_RES_C:[^ ]+]] = load i32, i32* %[[ISPC_GEP_C]]
; CHECK: %[[ISPC_GEP_D:[^ ]+]] = getelementptr inbounds %v8_uniform_Data, %v8_uniform_Data* %ispc.ptr, i32 0, i32 3
; CHECK: %[[ISPC_RES_D:[^ ]+]] = load i8, i8* %[[ISPC_GEP_D]]

; COM: combine the aggregate back
; CHECK: %[[ISPC_RES_0:[^ ]+]] = insertvalue %v8_uniform_Data undef, i8* %[[ISPC_RES_A]], 0
; CHECK: %[[ISPC_RES_1:[^ ]+]] = insertvalue %v8_uniform_Data %[[ISPC_RES_0]], i64 %[[ISPC_RES_B]], 1
; CHECK: %[[ISPC_RES_2:[^ ]+]] = insertvalue %v8_uniform_Data %[[ISPC_RES_1]], i32 %[[ISPC_RES_C]], 2
; CHECK: %res = insertvalue %v8_uniform_Data %[[ISPC_RES_2]], i8 %[[ISPC_RES_D]], 3

  %ispc.user.a = extractvalue %v8_uniform_Data %res, 0
  %ispc.user.b = extractvalue %v8_uniform_Data %res, 1
; CHECK: %ispc.user.a = extractvalue %v8_uniform_Data %res, 0
; CHECK: %ispc.user.b = extractvalue %v8_uniform_Data %res, 1

  ret void
}

define dllexport void @folded_structure_load() {
; CHECK-LABEL: folded_structure_load

  %ptr = alloca %folded_st
  %res = load %folded_st, %folded_st* %ptr
; CHECK: %[[GEP_A:[^ ]+]] = getelementptr inbounds %folded_st, %folded_st* %ptr, i32 0, i32 0
; CHECK: %[[RES_A:[^ ]+]] = load i8, i8* %[[GEP_A]]
; CHECK: %[[GEP_B:[^ ]+]] = getelementptr inbounds %folded_st, %folded_st* %ptr, i32 0, i32 1, i32 0
; CHECK: %[[RES_B:[^ ]+]] = load i32, i32* %[[GEP_B]]
; CHECK: %[[GEP_C:[^ ]+]] = getelementptr inbounds %folded_st, %folded_st* %ptr, i32 0, i32 1, i32 1
; CHECK: %[[RES_C:[^ ]+]] = load i64, i64* %[[GEP_C]]

; COM: combine the aggregate back
; CHECK: %[[RES_0:[^ ]+]] = insertvalue %folded_st undef, i8 %[[RES_A]], 0
; CHECK: %[[RES_1:[^ ]+]] = insertvalue %folded_st %[[RES_0]], i32 %[[RES_B]], 1, 0
; CHECK: %res = insertvalue %folded_st %[[RES_1]], i64 %[[RES_C]], 1, 1

  %user = extractvalue %folded_st %res, 0
; CHECK: %user = extractvalue %folded_st %res, 0
  ret void
}

define dllexport void @complex_load() {
; CHECK-LABEL: complex_load

  %ptr = alloca %complex_st
  %res = load %complex_st, %complex_st* %ptr
; CHECK: %[[GEP_A:[^ ]+]] = getelementptr inbounds %complex_st, %complex_st* %ptr, i32 0, i32 0, i32 0
; CHECK: %[[RES_A:[^ ]+]] = load i8, i8* %[[GEP_A]]
; CHECK: %[[GEP_B:[^ ]+]] = getelementptr inbounds %complex_st, %complex_st* %ptr, i32 0, i32 0, i32 1, i32 0
; CHECK: %[[RES_B:[^ ]+]] = load i32, i32* %[[GEP_B]]
; CHECK: %[[GEP_C:[^ ]+]] = getelementptr inbounds %complex_st, %complex_st* %ptr, i32 0, i32 0, i32 1, i32 1
; CHECK: %[[RES_C:[^ ]+]] = load i64, i64* %[[GEP_C]]
; CHECK: %[[GEP_D:[^ ]+]] = getelementptr inbounds %complex_st, %complex_st* %ptr, i32 0, i32 1
; CHECK: %[[RES_D:[^ ]+]] = load i64*, i64** %[[GEP_D]]

; COM: combine the aggregate back
; CHECK: %[[RES_0:[^ ]+]] = insertvalue %complex_st undef, i8 %[[RES_A]], 0, 0
; CHECK: %[[RES_1:[^ ]+]] = insertvalue %complex_st %[[RES_0]], i32 %[[RES_B]], 0, 1, 0
; CHECK: %[[RES_2:[^ ]+]] = insertvalue %complex_st %[[RES_1]], i64 %[[RES_C]], 0, 1, 1
; CHECK: %res = insertvalue %complex_st %[[RES_2]], i64* %[[RES_D]], 1

  %user = extractvalue %complex_st %res, 0
; CHECK: %user = extractvalue %complex_st %res, 0
  ret void
}

define dllexport void @structure_with_array_load() {
; CHECK-LABEL: structure_with_array_load

  %ptr = alloca %st_with_array
  %res = load %st_with_array, %st_with_array* %ptr
; CHECK: %[[GEP_A:[^ ]+]] = getelementptr inbounds %st_with_array, %st_with_array* %ptr, i32 0, i32 0
; CHECK: %[[RES_A:[^ ]+]] = load i8, i8* %[[GEP_A]]
; CHECK: %[[GEP_B:[^ ]+]] = getelementptr inbounds %st_with_array, %st_with_array* %ptr, i32 0, i32 1, i32 0
; CHECK: %[[RES_B:[^ ]+]] = load <10 x i32>, <10 x i32>* %[[GEP_B]]
; CHECK: %[[GEP_C:[^ ]+]] = getelementptr inbounds %st_with_array, %st_with_array* %ptr, i32 0, i32 1, i32 1
; CHECK: %[[RES_C:[^ ]+]] = load <10 x i32>, <10 x i32>* %[[GEP_C]]

; COM: combine the aggregate back
; CHECK: %[[RES_0:[^ ]+]] = insertvalue %st_with_array undef, i8 %[[RES_A]], 0
; CHECK: %[[RES_1:[^ ]+]] = insertvalue %st_with_array %[[RES_0]], <10 x i32> %[[RES_B]], 1, 0
; CHECK: %res = insertvalue %st_with_array %[[RES_1]], <10 x i32> %[[RES_C]], 1, 1

  %user = extractvalue %st_with_array %res, 0
; CHECK: %user = extractvalue %st_with_array %res, 0
  ret void
}
