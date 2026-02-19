;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-promote-sub-byte -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

%struct_without_bools = type { i8 }
%struct = type { [4 x <8 x i1*>], [4 x <8 x i1>*]* }
%inner = type { i1 }
%struct2 = type { i32, i1, %inner }

; CHECK:        %struct_without_bools = type { i8 }
; CHECK:        %struct = type { [4 x <8 x i8*>], [4 x <8 x i8>*]* }
; CHECK:        %struct2 = type { i32, i8, %inner }
; CHECK:        %inner = type { i8 }

define spir_func void @fun_struct_without_bools(i1 %input1, %struct_without_bools %input2) {
  ret void
}

; CHECK:        define spir_func void @fun_struct_without_bools(i8 %input1, %struct_without_bools %input2)


define spir_func i1 @callee0(i1 %input) {
  ret i1 %input
}

; CHECK:        define spir_func i8 @callee0(i8 %input)
; CHECK-NEXT:   ret i8 %input


define spir_func i1 @callee1(%struct addrspace(1)* %input_struct)
{
  %1 = load %struct, %struct addrspace(1)* %input_struct
  ret i1 true
}

; CHECK:        define spir_func i8 @callee1(%struct addrspace(1)* %input_struct)
; CHECK-NEXT:   %1 = load %struct, %struct addrspace(1)* %input_struct
; CHECK-NEXT:   ret i8 1


define spir_func %struct2 @callee2(%struct2 %input_struct) {
  ret %struct2 %input_struct
}

; CHECK:        define spir_func %struct2 @callee2(%struct2 %input_struct) {
; CHECK-NEXT:   %1 = extractvalue %struct2 %input_struct, 0
; CHECK-NEXT:   %2 = insertvalue %struct2 undef, i32 %1, 0
; CHECK-NEXT:   %3 = extractvalue %struct2 %input_struct, 1
; CHECK-NEXT:   %4 = insertvalue %struct2 %2, i8 %3, 1
; CHECK-NEXT:   %5 = extractvalue %struct2 %input_struct, 2
; CHECK-NEXT:   %6 = extractvalue %inner %5, 0
; CHECK-NEXT:   %7 = insertvalue %inner undef, i8 %6, 0
; CHECK-NEXT:   %8 = insertvalue %struct2 %4, %inner %7, 2
; CHECK-NEXT:   %9 = extractvalue %struct2 %8, 0
; CHECK-NEXT:   %10 = insertvalue %struct2 poison, i32 %9, 0
; CHECK-NEXT:   %11 = extractvalue %struct2 %8, 1
; CHECK-NEXT:   %12 = trunc i8 %11 to i1
; CHECK-NEXT:   %13 = zext i1 %12 to i8
; CHECK-NEXT:   %14 = insertvalue %struct2 %10, i8 %13, 1
; CHECK-NEXT:   %15 = extractvalue %struct2 %8, 2
; CHECK-NEXT:   %16 = extractvalue %inner %15, 0
; CHECK-NEXT:   %17 = trunc i8 %16 to i1
; CHECK-NEXT:   %18 = zext i1 %17 to i8
; CHECK-NEXT:   %19 = insertvalue %inner poison, i8 %18, 0
; CHECK-NEXT:   %20 = insertvalue %struct2 %14, %inner %19, 2
; CHECK-NEXT:   ret %struct2 %20


define spir_func [ 2 x i1 ] @callee3( [ 2 x i1 ] %input_array) {
  ret [ 2 x i1 ] %input_array
}

; CHECK:        define spir_func [2 x i8] @callee3([2 x i8] %input_array) {
; CHECK-NEXT:   %1 = extractvalue [2 x i8] %input_array, 0
; CHECK-NEXT:   %2 = insertvalue [2 x i8] undef, i8 %1, 0
; CHECK-NEXT:   %3 = extractvalue [2 x i8] %input_array, 1
; CHECK-NEXT:   %4 = insertvalue [2 x i8] %2, i8 %3, 1
; CHECK-NEXT:   %5 = extractvalue [2 x i8] %4, 0
; CHECK-NEXT:   %6 = trunc i8 %5 to i1
; CHECK-NEXT:   %7 = zext i1 %6 to i8
; CHECK-NEXT:   %8 = insertvalue [2 x i8] poison, i8 %7, 0
; CHECK-NEXT:   %9 = extractvalue [2 x i8] %4, 1
; CHECK-NEXT:   %10 = trunc i8 %9 to i1
; CHECK-NEXT:   %11 = zext i1 %10 to i8
; CHECK-NEXT:   %12 = insertvalue [2 x i8] %8, i8 %11, 1
; CHECK-NEXT:   ret [2 x i8] %12


define spir_func [ 2 x %inner ] @calle4( [ 2 x %inner ] %input_array) {
  ret [ 2 x %inner ] %input_array
}

; CHECK:        define spir_func [2 x %inner] @calle4([2 x %inner] %input_array) {
; CHECK-NEXT:   %1 = extractvalue [2 x %inner] %input_array, 0
; CHECK-NEXT:   %2 = extractvalue %inner %1, 0
; CHECK-NEXT:   %3 = insertvalue %inner undef, i8 %2, 0
; CHECK-NEXT:   %4 = insertvalue [2 x %inner] undef, %inner %3, 0
; CHECK-NEXT:   %5 = extractvalue [2 x %inner] %input_array, 1
; CHECK-NEXT:   %6 = extractvalue %inner %5, 0
; CHECK-NEXT:   %7 = insertvalue %inner undef, i8 %6, 0
; CHECK-NEXT:   %8 = insertvalue [2 x %inner] %4, %inner %7, 1
; CHECK-NEXT:   %9 = extractvalue [2 x %inner] %8, 0
; CHECK-NEXT:   %10 = extractvalue %inner %9, 0
; CHECK-NEXT:   %11 = trunc i8 %10 to i1
; CHECK-NEXT:   %12 = zext i1 %11 to i8
; CHECK-NEXT:   %13 = insertvalue %inner poison, i8 %12, 0
; CHECK-NEXT:   %14 = insertvalue [2 x %inner] poison, %inner %13, 0
; CHECK-NEXT:   %15 = extractvalue [2 x %inner] %8, 1
; CHECK-NEXT:   %16 = extractvalue %inner %15, 0
; CHECK-NEXT:   %17 = trunc i8 %16 to i1
; CHECK-NEXT:   %18 = zext i1 %17 to i8
; CHECK-NEXT:   %19 = insertvalue %inner poison, i8 %18, 0
; CHECK-NEXT:   %20 = insertvalue [2 x %inner] %14, %inner %19, 1
; CHECK-NEXT:   ret [2 x %inner] %20

define spir_func void @caller(i1 %input, %struct addrspace(1)* %input_struct) {
  %1 = call i1 @callee0(i1 false)
  %2 = call i1 @callee0(i1 true)
  %3 = call i1 @callee0(i1 %input)
  %4 = call i1 @callee1(%struct addrspace(1)* %input_struct)
  %5 = call %struct2 @callee2( %struct2 { i32 5, i1 false, %inner { i1 true } })
  %6 = call [ 2 x i1 ] @callee3( [ 2 x i1 ] [ i1 true, i1 false])
  %7 = call [ 2 x %inner ] @calle4( [ 2 x %inner ] [ %inner { i1 true }, %inner { i1 false} ])
  ret void
}

; CHECK:        define spir_func void @caller(i8 %input, %struct addrspace(1)* %input_struct)
; CHECK-NEXT:   %1 = call i8 @callee0(i8 0)
; CHECK-NEXT:   %2 = call i8 @callee0(i8 1)
; CHECK-NEXT:   %3 = call i8 @callee0(i8 %input)
; CHECK-NEXT:   %4 = call i8 @callee1(%struct addrspace(1)* %input_struct)
; CHECK-NEXT:   %5 = call %struct2 @callee2(%struct2 { i32 5, i8 0, %inner { i8 1 } })
; CHECK-NEXT:   %6 = call [2 x i8] @callee3([2 x i8] c"\01\00")
; CHECK-NEXT:   %7 = call [2 x %inner] @calle4([2 x %inner] [%inner { i8 1 }, %inner zeroinitializer])
; CHECK-NEXT:   ret void
