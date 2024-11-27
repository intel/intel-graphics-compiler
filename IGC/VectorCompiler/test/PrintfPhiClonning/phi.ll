;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXPrintfPhiClonning -GenXVerify -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s
; RUN: %opt_llvm-15 %use_old_pass_manager% -GenXPrintfPhiClonning -GenXVerify -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s
; RUN: %opt_llvm-16 %use_old_pass_manager% %pass_pref%GenXPrintfPhiClonning,GenXVerify -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"

@str1 = internal unnamed_addr addrspace(2) constant [72 x i8] c"ERROR FOR_S_INSVIRMEM: Issue allocating chardesc.address in for_concat\0A\00", align 1
@str2 = internal unnamed_addr addrspace(2) constant [69 x i8] c"ERROR FOR_S_INSVIRMEM: Issue freeing chardesc.address in for_concat\0A\00", align 1


declare spir_func i32 @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)*)

; CHECK-LABEL: @if_then_one_phi
define dllexport spir_kernel void @if_then_one_phi(i1 %cond) {
  br i1 %cond, label %1, label %2
1:
  br label %2
2:
; CHECK-NOT: phi {{.*}} @str1
; CHECK-NOT: phi {{.*}} @str2
  %3 = phi [72 x i8] addrspace(2)* [ @str1, %1 ], [ bitcast ([69 x i8] addrspace(2)* @str2 to [72 x i8] addrspace(2)*), %0 ]
  %4 = bitcast [72 x i8] addrspace(2)* %3 to i8 addrspace(2)*
  %5 = call spir_func i32 @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)* %4)
  br label %6

6:
  ; With such cases, where string comes from another function - we can't do anything
  %7 = inttoptr i32 %5 to i8*
  %8 = addrspacecast i8* %7 to i8 addrspace(2)*
  %9 = call spir_func i32 @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)* %8)
  ret void
}

; CHECK-LABEL: @if_then_six_phi
define dllexport spir_kernel void @if_then_six_phi(i1 %cond) {
  br i1 %cond, label %1, label %6 ; 0
1:
  br i1 %cond, label %6, label %2 ; 1
2:
  br i1 %cond, label %3, label %6 ; 2
3:
  br i1 %cond, label %4, label %6 ; 3
4:
  br i1 %cond, label %6, label %5 ; 3
5:
  br label %6 ; 5

6:
; CHECK-NOT: phi {{.*}} @str1
; CHECK-NOT: phi {{.*}} @str2
  %7 = phi [72 x i8] addrspace(2)* [ @str1, %1 ],
           [ bitcast ([69 x i8] addrspace(2)* @str2 to [72 x i8] addrspace(2)*), %0 ],
           [ @str1, %2 ],
           [ @str1, %3 ],
           [ @str1, %4 ],
           [ @str1, %5 ]
  %8 = bitcast [72 x i8] addrspace(2)* %7 to i8 addrspace(2)*
  %9 = call spir_func i32 @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)* %8)
  ret void
}

; CHECK-LABEL: @if_then_multiply_print
define dllexport spir_kernel void @if_then_multiply_print(i1 %cond) {
  br i1 %cond, label %1, label %2
1:
  br label %2
2:
; CHECK-NOT: phi {{.*}} @str1
; CHECK-NOT: phi {{.*}} @str2
  %3 = phi [72 x i8] addrspace(2)* [ @str1, %1 ], [ bitcast ([69 x i8] addrspace(2)* @str2 to [72 x i8] addrspace(2)*), %0 ]
  %4 = phi [72 x i8] addrspace(2)* [ @str1, %1 ], [ bitcast ([69 x i8] addrspace(2)* @str2 to [72 x i8] addrspace(2)*), %0 ]
  %5 = phi [72 x i8] addrspace(2)* [ @str1, %1 ], [ bitcast ([69 x i8] addrspace(2)* @str2 to [72 x i8] addrspace(2)*), %0 ]
  %6 = bitcast [72 x i8] addrspace(2)* %3 to i8 addrspace(2)*
  %7 = call spir_func i32 @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)* %6)
  %8 = bitcast [72 x i8] addrspace(2)* %3 to i8 addrspace(2)*
  %9 = call spir_func i32 @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)* %8)
  %10 = bitcast [72 x i8] addrspace(2)* %3 to i8 addrspace(2)*
  %11 = call spir_func i32 @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)* %10)
  br label %12
12:
  %13 = inttoptr i32 %11 to i8*
  %14 = addrspacecast i8* %13 to i8 addrspace(2)*
  %15 = call spir_func i32 @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)* %14)
  ret void
}


; CHECK-LABEL: @if_then_phi_in_res
define dllexport spir_kernel void @if_then_phi_in_res(i1 %cond, i32 %in1, i32 %in2, i32 %in3) {
  br i1 %cond, label %1, label %2
1:
  br i1 %cond, label %2, label %7
2:
; CHECK-NOT: phi {{.*}} @str1
; CHECK-NOT: phi {{.*}} @str2
  %3 = phi [72 x i8] addrspace(2)* [ @str1, %1 ], [ bitcast ([69 x i8] addrspace(2)* @str2 to [72 x i8] addrspace(2)*), %0 ]
  %4 = phi i32 [ %in1, %1 ], [ %in3, %0 ]
  %5 = bitcast [72 x i8] addrspace(2)* %3 to i8 addrspace(2)*
  %6 = call spir_func i32 @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)* %5)
  br label %7

7:
; CHECK: phi i32 [ %in1, {{.*}} ], [ %in2, {{.*}} ], [ %in3, {{.*}} ]
  %8 = phi i32 [ %4, %2 ], [ %in2, %1 ]
; CHECK: phi i32 {{.*}} %in2, {{.*}}
  %9 = phi i32 [ %6, %2 ], [ %in2, %1 ]
  %10 = inttoptr i32 %9 to i8*
  %11 = addrspacecast i8* %10 to i8 addrspace(2)*
  ; With such cases, where string comes from another function - we can't do anything
  ; ... = call spir_func i32 @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)* %11)
  ret void
}
