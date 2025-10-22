;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-process-bi-code-assumption -S < %s 2>&1 | FileCheck %s

; Test get_global_id pattern produced by ocloc.

; CHECK-LABEL: @pattern_1
; CHECK-NEXT:    %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
; CHECK-NEXT:    %2 = trunc i64 %1 to i32
; CHECK-NEXT:    %3 = zext i32 %2 to i64
; CHECK-NEXT:    %4 = insertelement <3 x i64> undef, i64 %3, i32 0
; CHECK-NEXT:    %5 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 1)
; CHECK-NEXT:    %6 = trunc i64 %5 to i32
; CHECK-NEXT:    %7 = zext i32 %6 to i64
; CHECK-NEXT:    %8 = insertelement <3 x i64> %4, i64 %7, i32 1
; CHECK-NEXT:    %9 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 2)
; CHECK-NEXT:    %10 = trunc i64 %9 to i32
; CHECK-NEXT:    %11 = zext i32 %10 to i64
; CHECK-NEXT:    %12 = insertelement <3 x i64> %8, i64 %11, i32 2
;
; CHECK-NEXT:    %13 = extractelement <3 x i64> %12, i32 0
; CHECK-NEXT:    %14 = icmp ult i64 %13, 2147483648
; CHECK-NEXT:    call void @llvm.assume(i1 %14)
; CHECK-NEXT:    %15 = extractelement <3 x i64> %12, i32 1
; CHECK-NEXT:    %16 = icmp ult i64 %15, 2147483648
; CHECK-NEXT:    call void @llvm.assume(i1 %16)
; CHECK-NEXT:    %17 = extractelement <3 x i64> %12, i32 2
; CHECK-NEXT:    %18 = icmp ult i64 %17, 2147483648
; CHECK-NEXT:    call void @llvm.assume(i1 %18)
;
; CHECK-NEXT:    ret void
define void @pattern_1() {

  %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %2 = insertelement <3 x i64> undef, i64 %1, i32 0
  %3 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 1)
  %4 = insertelement <3 x i64> %2, i64 %3, i32 1
  %5 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 2)
  %6 = insertelement <3 x i64> %4, i64 %5, i32 2

  %7 = extractelement <3 x i64> %6, i32 0
  %8 = icmp ult i64 %7, 2147483648
  call void @llvm.assume(i1 %8)
  %9 = extractelement <3 x i64> %6, i32 1
  %10 = icmp ult i64 %9, 2147483648
  call void @llvm.assume(i1 %10)
  %11 = extractelement <3 x i64> %6, i32 2
  %12 = icmp ult i64 %11, 2147483648
  call void @llvm.assume(i1 %12)

  ret void
}

; CHECK-LABEL: @pattern_2
; CHECK-NEXT:    %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
; CHECK-NEXT:    %2 = trunc i64 %1 to i32
; CHECK-NEXT:    %3 = zext i32 %2 to i64
; CHECK-NEXT:    %4 = insertelement <3 x i64> undef, i64 %3, i32 0
; CHECK-NEXT:    %5 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 1)
; CHECK-NEXT:    %6 = trunc i64 %5 to i32
; CHECK-NEXT:    %7 = zext i32 %6 to i64
; CHECK-NEXT:    %8 = insertelement <3 x i64> %4, i64 %7, i32 1
; CHECK-NEXT:    %9 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 2)
; CHECK-NEXT:    %10 = trunc i64 %9 to i32
; CHECK-NEXT:    %11 = zext i32 %10 to i64
; CHECK-NEXT:    %12 = insertelement <3 x i64> %8, i64 %11, i32 2
;
; CHECK-NEXT:    %13 = extractelement <3 x i64> %12, i32 0
; CHECK-NEXT:    %14 = select i1 true, i64 %13, i64 0
; CHECK-NEXT:    %15 = icmp ult i64 %14, 2147483648
; CHECK-NEXT:    call void @llvm.assume(i1 %15)
; CHECK-NEXT:    %16 = extractelement <3 x i64> %12, i32 1
; CHECK-NEXT:    %17 = select i1 true, i64 %16, i64 0
; CHECK-NEXT:    %18 = icmp ult i64 %17, 2147483648
; CHECK-NEXT:    call void @llvm.assume(i1 %18)
; CHECK-NEXT:    %19 = extractelement <3 x i64> %12, i32 2
; CHECK-NEXT:    %20 = select i1 true, i64 %19, i64 0
; CHECK-NEXT:    %21 = icmp ult i64 %20, 2147483648
;
; CHECK-NEXT:    call void @llvm.assume(i1 %21)
; CHECK-NEXT:    ret void
define void @pattern_2() {

  %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %2 = insertelement <3 x i64> undef, i64 %1, i32 0
  %3 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 1)
  %4 = insertelement <3 x i64> %2, i64 %3, i32 1
  %5 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 2)
  %6 = insertelement <3 x i64> %4, i64 %5, i32 2

  %7 = extractelement <3 x i64> %6, i32 0
  %8 = select i1 true, i64 %7, i64 0
  %9 = icmp ult i64 %8, 2147483648
  call void @llvm.assume(i1 %9)
  %10 = extractelement <3 x i64> %6, i32 1
  %11 = select i1 true, i64 %10, i64 0
  %12 = icmp ult i64 %11, 2147483648
  call void @llvm.assume(i1 %12)
  %13 = extractelement <3 x i64> %6, i32 2
  %14 = select i1 true, i64 %13, i64 0
  %15 = icmp ult i64 %14, 2147483648
  call void @llvm.assume(i1 %15)

  ret void
}

; CHECK-LABEL: @pattern_no_match
;
; CHECK-NEXT:    %1 = call spir_func i64 @other_function()
; CHECK-NEXT:    %2 = insertelement <3 x i64> undef, i64 %1, i32 0
; CHECK-NEXT:    %3 = call spir_func i64 @other_function()
; CHECK-NEXT:    %4 = insertelement <3 x i64> %2, i64 %3, i32 1
; CHECK-NEXT:    %5 = call spir_func i64 @other_function()
; CHECK-NEXT:    %6 = insertelement <3 x i64> %4, i64 %5, i32 2
;
; CHECK-NEXT:    %7 = extractelement <3 x i64> %6, i32 0
; CHECK-NEXT:    %8 = icmp ult i64 %7, 2147483648
; CHECK-NEXT:    call void @llvm.assume(i1 %8)
; CHECK-NEXT:    %9 = extractelement <3 x i64> %6, i32 1
; CHECK-NEXT:    %10 = icmp ult i64 %9, 2147483648
; CHECK-NEXT:    call void @llvm.assume(i1 %10)
; CHECK-NEXT:    %11 = extractelement <3 x i64> %6, i32 2
; CHECK-NEXT:    %12 = icmp ult i64 %11, 2147483648
; CHECK-NEXT:    call void @llvm.assume(i1 %12)
;
; CHECK-NEXT:    ret void
define void @pattern_no_match() {

  %1 = call spir_func i64 @other_function()
  %2 = insertelement <3 x i64> undef, i64 %1, i32 0
  %3 = call spir_func i64 @other_function()
  %4 = insertelement <3 x i64> %2, i64 %3, i32 1
  %5 = call spir_func i64 @other_function()
  %6 = insertelement <3 x i64> %4, i64 %5, i32 2

  %7 = extractelement <3 x i64> %6, i32 0
  %8 = icmp ult i64 %7, 2147483648
  call void @llvm.assume(i1 %8)
  %9 = extractelement <3 x i64> %6, i32 1
  %10 = icmp ult i64 %9, 2147483648
  call void @llvm.assume(i1 %10)
  %11 = extractelement <3 x i64> %6, i32 2
  %12 = icmp ult i64 %11, 2147483648
  call void @llvm.assume(i1 %12)

  ret void
}

declare void @llvm.assume(i1)
declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)
declare i64 @other_function()
