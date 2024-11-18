;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, pvc-supported
; RUN: llvm-as %s -o %t.bc
; RUN: ocloc -device pvc -llvm_input -options "-vc-codegen -ze-collect-cost-info -igc_opts 'ShaderDumpEnable=1, DumpToCustomDir=%t'" -output_no_suffix -file %t.bc
; RUN: cat %t/*.zeinfo | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"

%struct.state = type { i8, i8 addrspace(1) *, float }

@data = internal global <8 x i64> undef, align 64, !spirv.Decorations !0 #0

; Function Attrs: nounwind
declare void @llvm.genx.vstore.v8i64.p0v8i64(<8 x i64>, <8 x i64>*)

; CHECK: payload_arguments:
; CHECK: - arg_type:        arg_bypointer
; CHECK-NEXT:    offset:          40
; CHECK-NEXT:    size:            8
; CHECK-NEXT:    arg_index:       1
; CHECK-NEXT:    addrmode:        stateless
; CHECK-NEXT:    addrspace:       global
; CHECK-NEXT:    access_type:     readwrite
; CHECK: - arg_type:        arg_byvalue
; CHECK-NEXT:    offset:          16
; CHECK-NEXT:    size:            1
; CHECK-NEXT:    arg_index:       0
; CHECK-NEXT:    source_offset:   0
; CHECK: - arg_type:        arg_byvalue
; CHECK-NEXT:    offset:          24
; CHECK-NEXT:    size:            8
; CHECK-NEXT:    arg_index:       0
; CHECK-NEXT:    source_offset:   8
; CHECK-NEXT:    is_ptr:          true
; CHECK: - arg_type:        arg_byvalue
; CHECK-NEXT:    offset:          32
; CHECK-NEXT:    size:            4
; CHECK-NEXT:    arg_index:       0
; CHECK-NEXT:    source_offset:   16

; Function Attrs: nounwind
define dllexport spir_kernel void @foo(%struct.state* byval(%struct.state) "VCArgumentIOKind"="0" %_arg_, i32 addrspace(1)* "VCArgumentIOKind"="0" %_arg_1) #1 {
entry:
  %0 = getelementptr inbounds %struct.state, %struct.state* %_arg_, i64 0, i32 1
  %1 = load i8 addrspace(1)*, i8 addrspace(1)** %0, align 8
  %2 = ptrtoint i8 addrspace(1)* %1 to i64
  %3 = tail call <8 x i64> @llvm.vc.internal.lsc.load.ugm.v8i64.i1.v2i8.i64(i1 true, i8 3, i8 4, i8 5, <2 x i8> zeroinitializer, i64 0, i64 %2, i16 1, i32 0, <8 x i64> undef)
  tail call void @llvm.genx.vstore.v8i64.p0v8i64(<8 x i64> %3, <8 x i64>* nonnull @data)
  ret void
}

declare <8 x i64> @llvm.vc.internal.lsc.load.ugm.v8i64.i1.v2i8.i64(i1, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <8 x i64>) #3

attributes #0 = { "VCByteOffset"="0" "VCGlobalVariable" "VCVolatile" "genx_byte_offset"="0" "genx_volatile" }
attributes #1 = { noinline nounwind "VCFunction" "VCNamedBarrierCount"="0" "VCSLMSize"="0" }

!spirv.Source = !{!0}
!opencl.spir.version = !{!1}
!opencl.ocl.version = !{!2}
!opencl.used.extensions = !{!3}
!opencl.used.optional.core.features = !{!3}
!spirv.Generator = !{!4}

!0 = !{i32 0, i32 100000}
!1 = !{i32 1, i32 2}
!2 = !{i32 1, i32 0}
!3 = !{}
!4 = !{i16 6, i16 14}
