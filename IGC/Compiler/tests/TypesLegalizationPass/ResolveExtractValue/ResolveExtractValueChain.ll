;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt %s --opaque-pointers -S -o - -types-legalization-pass -dce | FileCheck %s
; REQUIRES: llvm-14-plus


; CHECK-LABEL: define spir_kernel void @test_ResolveExtractValueChain()
; CHECK:    %call = call spir_func %nested.nested.struct.type @get_struct()
; CHECK-NEXT:    [[TMP1:%.*]] = extractvalue %nested.nested.struct.type %call, 1, 0, 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractvalue %nested.nested.struct.type %call, 1, 1, 0
; CHECK-NEXT:    call spir_func void @use_i32(i32 [[TMP1]])
; CHECK-NEXT:    call spir_func void @use_i32(i32 [[TMP2]])
; CHECK-NEXT:    ret void
;

%simple.struct.type = type { i32, float }
%nested.struct.type = type { [2 x i32], %simple.struct.type }
%nested.nested.struct.type = type { i32, %nested.struct.type }

define spir_kernel void @test_ResolveExtractValueChain()
{
  %call = call spir_func %nested.nested.struct.type @get_struct()
  %1 = extractvalue %nested.nested.struct.type %call, 1
  %2 = extractvalue %nested.struct.type %1, 0
  %3 = extractvalue [2 x i32] %2, 0
  call spir_func void @use_i32(i32 %3)
  %4 = extractvalue %nested.struct.type %1, 1, 0
  call spir_func void @use_i32(i32 %4)
  ret void
}

declare spir_func %nested.nested.struct.type @get_struct()
declare spir_func void @use_i32(i32)
