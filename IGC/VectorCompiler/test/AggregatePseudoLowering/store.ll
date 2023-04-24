;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXAggregatePseudoLowering -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

%simple_st = type { i32, i64 }
%folded_st = type { i8, %simple_st }
%complex_st = type { %folded_st, i64* }
%st_with_array = type { i8, [2 x <10 x i32>] }

define dllexport void @simple_store() {
; CHECK-LABEL: simple_store

  %res.ptr = alloca %simple_st

  store %simple_st { i32 12, i64 85 }, %simple_st* %res.ptr
; CHECK: %[[SIMPLE_A:[^ ]+]] = extractvalue %simple_st { i32 12, i64 85 }, 0
; CHECK: %[[SIMPLE_B:[^ ]+]] = extractvalue %simple_st { i32 12, i64 85 }, 1
; CHECK: %[[SIMPLE_GEP_A:[^ ]+]] = getelementptr inbounds %simple_st, %simple_st* %res.ptr, i32 0, i32 0
; CHECK: store i32 %[[SIMPLE_A]], i32* %[[SIMPLE_GEP_A]]
; CHECK: %[[SIMPLE_GEP_B:[^ ]+]] = getelementptr inbounds %simple_st, %simple_st* %res.ptr, i32 0, i32 1
; CHECK: store i64 %[[SIMPLE_B]], i64* %[[SIMPLE_GEP_B]]

  ret void
}

define dllexport void @arg_store(%simple_st %st) {
; CHECK-LABEL: arg_store

  %res.ptr = alloca %simple_st

  store %simple_st %st, %simple_st* %res.ptr
; CHECK: %[[ARG_A:[^ ]+]] = extractvalue %simple_st %st, 0
; CHECK: %[[ARG_B:[^ ]+]] = extractvalue %simple_st %st, 1
; CHECK: %[[ARG_GEP_A:[^ ]+]] = getelementptr inbounds %simple_st, %simple_st* %res.ptr, i32 0, i32 0
; CHECK: store i32 %[[ARG_A]], i32* %[[ARG_GEP_A]]
; CHECK: %[[ARG_GEP_B:[^ ]+]] = getelementptr inbounds %simple_st, %simple_st* %res.ptr, i32 0, i32 1
; CHECK: store i64 %[[ARG_B]], i64* %[[ARG_GEP_B]]

  ret void
}

define dllexport void @folded_store(%folded_st %st) {
; CHECK-LABEL: folded_store

  %res.ptr = alloca %folded_st

  store %folded_st %st, %folded_st* %res.ptr
; CHECK: %[[ARG_A:[^ ]+]] = extractvalue %folded_st %st, 0
; CHECK: %[[ARG_B:[^ ]+]] = extractvalue %folded_st %st, 1, 0
; CHECK: %[[ARG_C:[^ ]+]] = extractvalue %folded_st %st, 1, 1
; CHECK: %[[ARG_GEP_A:[^ ]+]] = getelementptr inbounds %folded_st, %folded_st* %res.ptr, i32 0, i32 0
; CHECK: store i8 %[[ARG_A]], i8* %[[ARG_GEP_A]]
; CHECK: %[[ARG_GEP_B:[^ ]+]] = getelementptr inbounds %folded_st, %folded_st* %res.ptr, i32 0, i32 1, i32 0
; CHECK: store i32 %[[ARG_B]], i32* %[[ARG_GEP_B]]
; CHECK: %[[ARG_GEP_C:[^ ]+]] = getelementptr inbounds %folded_st, %folded_st* %res.ptr, i32 0, i32 1, i32 1
; CHECK: store i64 %[[ARG_C]], i64* %[[ARG_GEP_C]]

  ret void
}

define dllexport void @complex_store(%complex_st %st) {
; CHECK-LABEL: complex_store

  %res.ptr = alloca %complex_st

  store %complex_st %st, %complex_st* %res.ptr
; CHECK: %[[ARG_A:[^ ]+]] = extractvalue %complex_st %st, 0, 0
; CHECK: %[[ARG_B:[^ ]+]] = extractvalue %complex_st %st, 0, 1, 0
; CHECK: %[[ARG_C:[^ ]+]] = extractvalue %complex_st %st, 0, 1, 1
; CHECK: %[[ARG_D:[^ ]+]] = extractvalue %complex_st %st, 1
; CHECK: %[[ARG_GEP_A:[^ ]+]] = getelementptr inbounds %complex_st, %complex_st* %res.ptr, i32 0, i32 0, i32 0
; CHECK: store i8 %[[ARG_A]], i8* %[[ARG_GEP_A]]
; CHECK: %[[ARG_GEP_B:[^ ]+]] = getelementptr inbounds %complex_st, %complex_st* %res.ptr, i32 0, i32 0, i32 1, i32 0
; CHECK: store i32 %[[ARG_B]], i32* %[[ARG_GEP_B]]
; CHECK: %[[ARG_GEP_C:[^ ]+]] = getelementptr inbounds %complex_st, %complex_st* %res.ptr, i32 0, i32 0, i32 1, i32 1
; CHECK: store i64 %[[ARG_C]], i64* %[[ARG_GEP_C]]
; CHECK: %[[ARG_GEP_D:[^ ]+]] = getelementptr inbounds %complex_st, %complex_st* %res.ptr, i32 0, i32 1
; CHECK: store i64* %[[ARG_D]], i64** %[[ARG_GEP_D]]

  ret void
}

define dllexport void @structure_with_array_store(%st_with_array %st) {
; CHECK-LABEL: structure_with_array_store

  %res.ptr = alloca %st_with_array

  store %st_with_array %st, %st_with_array* %res.ptr
; CHECK: %[[ARG_A:[^ ]+]] = extractvalue %st_with_array %st, 0
; CHECK: %[[ARG_B:[^ ]+]] = extractvalue %st_with_array %st, 1, 0
; CHECK: %[[ARG_C:[^ ]+]] = extractvalue %st_with_array %st, 1, 1
; CHECK: %[[ARG_GEP_A:[^ ]+]] = getelementptr inbounds %st_with_array, %st_with_array* %res.ptr, i32 0, i32 0
; CHECK: store i8 %[[ARG_A]], i8* %[[ARG_GEP_A]]
; CHECK: %[[ARG_GEP_B:[^ ]+]] = getelementptr inbounds %st_with_array, %st_with_array* %res.ptr, i32 0, i32 1, i32 0
; CHECK: store <10 x i32> %[[ARG_B]], <10 x i32>* %[[ARG_GEP_B]]
; CHECK: %[[ARG_GEP_C:[^ ]+]] = getelementptr inbounds %st_with_array, %st_with_array* %res.ptr, i32 0, i32 1, i32 1
; CHECK: store <10 x i32> %[[ARG_C]], <10 x i32>* %[[ARG_GEP_C]]

  ret void
}
