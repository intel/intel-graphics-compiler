;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-promote-bools -S %s -o - | FileCheck %s

%struct = type { [4 x <8 x i1*>], [4 x <8 x i1>*]* }

; CHECK:        [[NEW_STRUCT:%struct.[0-9]+]] = type { [4 x <8 x i8*>], [4 x <8 x i8>*]* }


define spir_func void @scalar_alloca() {
  %allocated = alloca i1, align 1
  ret void
}

; CHECK:        define spir_func void @scalar_alloca()
; CHECK-NEXT:   %allocated = alloca i8, align 1


define spir_func void @struct_alloca() {
  %allocated = alloca %struct, align 64
  ret void
}

; CHECK:        define spir_func void @struct_alloca()
; CHECK-NEXT:   %allocated = alloca [[NEW_STRUCT]], align 64
