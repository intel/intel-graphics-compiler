;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, regkeys, pvc-supported
; UNSUPPORTED: legacy-translator, sys32

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_predicated_io -o %t.spv
; RUN: llvm-spirv %t.spv -o %t.spt --to-text
; RUN: FileCheck < %t.spt %s

target triple = "spir64-unknown-unknown"

; CHECK: Capability PredicatedIOINTEL
; CHECK: Extension "SPV_INTEL_predicated_io"

; CHECK-DAG: TypeBool [[#BoolTy:]]
; CHECK-DAG: TypeInt [[#I8Ty:]] 8 0
; CHECK-DAG: TypeInt [[#I32Ty:]] 32 0
; CHECK-DAG: TypeFloat [[#F32Ty:]] 32
; CHECK-DAG: TypeVector [[#V4I32Ty:]] [[#I32Ty]] 4

; Test basic predicated load
; CHECK: PredicatedLoadINTEL [[#I32Ty]] [[#]] [[#]] [[#]] [[#]]
define spir_kernel void @test_predicated_load_basic(i32 addrspace(1)* %ptr, i1 %pred) {
entry:
  %result = call i32 @_Z27__spirv_PredicatedLoadINTELPU3AS1ibi(i32 addrspace(1)* %ptr, i1 %pred, i32 42)
  ret void
}

; Test predicated load with memory operands
; CHECK: PredicatedLoadINTEL [[#I32Ty]] {{[0-9]+}} {{[0-9]+}} {{[0-9]+}} {{[0-9]+}}
define spir_kernel void @test_predicated_load_memop(i32 addrspace(1)* %ptr, i1 %pred) {
entry:
  %result = call i32 @_Z27__spirv_PredicatedLoadINTELPU3AS1ibij(i32 addrspace(1)* %ptr, i1 %pred, i32 42, i32 4)
  ret void
}

; Test predicated load with alignment
; CHECK: PredicatedLoadINTEL [[#I32Ty]] {{[0-9]+}} {{[0-9]+}} {{[0-9]+}} {{[0-9]+}} {{[0-9]+}} {{[0-9]+}}
define spir_kernel void @test_predicated_load_aligned(i32 addrspace(1)* %ptr, i1 %pred) {
entry:
  %result = call i32 @_Z27__spirv_PredicatedLoadINTELPU3AS1ibijj(i32 addrspace(1)* %ptr, i1 %pred, i32 42, i32 2, i32 16)
  ret void
}

; Test basic predicated store
; CHECK: PredicatedStoreINTEL {{[0-9]+}} {{[0-9]+}} {{[0-9]+}}
define spir_kernel void @test_predicated_store_basic(i32 addrspace(1)* %ptr, i1 %pred) {
entry:
  call void @_Z28__spirv_PredicatedStoreINTELPU3AS1iib(i32 addrspace(1)* %ptr, i32 123, i1 %pred)
  ret void
}

; Test predicated store with memory operands
; CHECK: PredicatedStoreINTEL {{[0-9]+}} {{[0-9]+}} {{[0-9]+}} {{[0-9]+}}
define spir_kernel void @test_predicated_store_memop(i32 addrspace(1)* %ptr, i1 %pred) {
entry:
  call void @_Z28__spirv_PredicatedStoreINTELPU3AS1iibj(i32 addrspace(1)* %ptr, i32 123, i1 %pred, i32 4)
  ret void
}

; Test predicated store with alignment
; CHECK: PredicatedStoreINTEL {{[0-9]+}} {{[0-9]+}} {{[0-9]+}} {{[0-9]+}} {{[0-9]+}}
define spir_kernel void @test_predicated_store_aligned(i32 addrspace(1)* %ptr, i1 %pred) {
entry:
  call void @_Z28__spirv_PredicatedStoreINTELPU3AS1iibjj(i32 addrspace(1)* %ptr, i32 123, i1 %pred, i32 2, i32 16)
  ret void
}

; Test with different data types
; CHECK: PredicatedLoadINTEL [[#I8Ty]] {{[0-9]+}} {{[0-9]+}} {{[0-9]+}} {{[0-9]+}}
define spir_kernel void @test_predicated_load_i8(i8 addrspace(1)* %ptr, i1 %pred) {
entry:
  %result = call i8 @_Z27__spirv_PredicatedLoadINTELPU3AS1cbc(i8 addrspace(1)* %ptr, i1 %pred, i8 0)
  ret void
}

; CHECK: PredicatedLoadINTEL [[#F32Ty]] {{[0-9]+}} {{[0-9]+}} {{[0-9]+}} {{[0-9]+}}
define spir_kernel void @test_predicated_load_float(float addrspace(1)* %ptr, i1 %pred) {
entry:
  %result = call float @_Z27__spirv_PredicatedLoadINTELPU3AS1fbf(float addrspace(1)* %ptr, i1 %pred, float 0.0)
  ret void
}

; Test vector types
; CHECK: PredicatedLoadINTEL [[#V4I32Ty]] {{[0-9]+}} {{[0-9]+}} {{[0-9]+}} {{[0-9]+}}
define spir_kernel void @test_predicated_load_vector(<4 x i32> addrspace(1)* %ptr, i1 %pred) {
entry:
  %result = call <4 x i32> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_ibDv4_i(<4 x i32> addrspace(1)* %ptr, i1 %pred, <4 x i32> zeroinitializer)
  ret void
}

; Function declarations
declare i8 @_Z27__spirv_PredicatedLoadINTELPU3AS1cbc(i8 addrspace(1)*, i1, i8)
declare i32 @_Z27__spirv_PredicatedLoadINTELPU3AS1ibi(i32 addrspace(1)*, i1, i32)
declare i32 @_Z27__spirv_PredicatedLoadINTELPU3AS1ibij(i32 addrspace(1)*, i1, i32, i32)
declare i32 @_Z27__spirv_PredicatedLoadINTELPU3AS1ibijj(i32 addrspace(1)*, i1, i32, i32, i32)
declare float @_Z27__spirv_PredicatedLoadINTELPU3AS1fbf(float addrspace(1)*, i1, float)
declare <4 x i32> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_ibDv4_i(<4 x i32> addrspace(1)*, i1, <4 x i32>)

declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1iib(i32 addrspace(1)*, i32, i1)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1iibj(i32 addrspace(1)*, i32, i1, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1iibjj(i32 addrspace(1)*, i32, i1, i32, i32)

!opencl.ocl.version = !{!0}
!0 = !{i32 2, i32 0}
