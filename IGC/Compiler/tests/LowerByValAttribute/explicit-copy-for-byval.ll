;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - --igc-lower-byval-attribute | FileCheck %s

; ------------------------------------------------
; LegalizeFunctionSignatures
; ------------------------------------------------

; This test verifies if an explicit copy (alloca + memcpy) is generated in a caller for
; pointer arguments with `byval` attribute. According to the LLVM documentation:
;
; "The attribute implies that a hidden copy of the pointee is made between the caller
;  and the callee, so the callee is unable to modify the value in the caller."

%structtype = type { i32, [10 x i32], i32 }

@fptr = internal global void (%structtype*)* @f2, align 8

define spir_kernel void @kernel(%structtype* byval(%structtype) %s, i32 addrspace(1)* %out) {

; CHECK: [[ALLOCA0:%.*]] = alloca %structtype
; CHECK: [[DST0:%.*]] = bitcast %structtype* [[ALLOCA0]] to i8*
; CHECK: [[SRC0:%.*]] = bitcast %structtype* %s to i8*
; CHECK: call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 [[DST0]], i8* align 4 [[SRC0]], i64 48, i1 false)
; CHECK: call void @f0(%structtype* byval(%structtype) [[ALLOCA0]])
  call void @f0(%structtype* byval(%structtype) %s)

; CHECK: [[ALLOCA1:%.*]] = alloca %structtype
; CHECK: [[DST1:%.*]] = bitcast %structtype* [[ALLOCA1]] to i8*
; CHECK: [[SRC1:%.*]] = bitcast %structtype addrspace(4)* %generic_ptr to i8 addrspace(4)*
; CHECK: call void @llvm.memcpy.p0i8.p4i8.i64(i8* align 4 [[DST1]], i8 addrspace(4)* align 4 [[SRC1]], i64 48, i1 false)
; CHECK: [[ASC:%.*]] = addrspacecast %structtype* [[ALLOCA1]] to %structtype addrspace(4)*
; CHECK: call void @f1(%structtype addrspace(4)* byval(%structtype) [[ASC]])
  %generic_ptr = addrspacecast %structtype* %s to %structtype addrspace(4)*
  call void @f1(%structtype addrspace(4)* byval(%structtype) %generic_ptr)

; CHECK: [[ALLOCA2:%.*]] = alloca %structtype
; CHECK: [[DST2:%.*]] = bitcast %structtype* [[ALLOCA2]] to i8*
; CHECK: [[SRC2:%.*]] = bitcast %structtype* %s to i8*
; CHECK: call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 [[DST2]], i8* align 4 [[SRC2]], i64 48, i1 false)
; CHECK: call void %f2ptr(%structtype* byval(%structtype) [[ALLOCA2]])
  %f2ptr = load void (%structtype*)*, void (%structtype*)** @fptr, align 8
  call void %f2ptr(%structtype* byval(%structtype) %s)

; COM: @f3 function parameter is `readonly`, so copy is not necessary
; CHECK-NOT: alloca %structtype
; CHECK: call i32 @f3(%structtype* byval(%structtype) %s)
  %v = call i32 @f3(%structtype* byval(%structtype) %s)
  store i32 %v, i32 addrspace(1)* %out, align 4

; COM: Don't generate an explicit copy, if `byval` argument is undef, to avoid null pointer dereference
; CHECK-NOT: alloca %structtype
; CHECK: call void @f0(%structtype* byval(%structtype) undef)
  call void @f0(%structtype* byval(%structtype) undef)

  ret void
}

define spir_func void @f0(%structtype* byval(%structtype) %src) #0 {
  %1 = getelementptr inbounds %structtype, %structtype* %src, i64 0, i32 0
  store i32 222, i32* %1, align 4
  ret void
}

define spir_func void @f1(%structtype addrspace(4)* byval(%structtype) %src) #0 {
  %1 = getelementptr inbounds %structtype, %structtype addrspace(4)* %src, i64 0, i32 0
  store i32 222, i32 addrspace(4)* %1, align 4
  ret void
}

define spir_func void @f2(%structtype* byval(%structtype) %src) "referenced-indirectly" {
  %1 = getelementptr inbounds %structtype, %structtype* %src, i64 0, i32 0
  store i32 222, i32* %1, align 4
  ret void
}

define spir_func i32 @f3(%structtype* readonly byval(%structtype) %src) {
  %1 = getelementptr inbounds %structtype, %structtype* %src, i64 0, i32 0
  %2 = load i32, i32* %1, align 4
  ret i32 %2
}

attributes #0 = { noinline }
