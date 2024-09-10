;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXDetectPointerArg -march=genx64 -mcpu=XeHPC -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"

%struct.state = type { i8, [3 x i64], float }

@data = internal global <8 x i64> undef, align 64, !spirv.Decorations !0 #0

; Function Attrs: nounwind
declare void @llvm.genx.vstore.v8i64.p0v8i64(<8 x i64>, <8 x i64>*) #1

; Function Attrs: nounwind
define dllexport spir_kernel void @foo(%struct.state* byval(%struct.state) %_arg_, i32 addrspace(1)* %_arg_1, i64 %impl.arg.private.base, i8 %__arg_lin__arg_.0, i64 %__arg_lin__arg_.8, i64 %__arg_lin__arg_.16, i64 %__arg_lin__arg_.24, float %__arg_lin__arg_.32) #1 {
entry:
  %_arg_.linearization = alloca %struct.state, align 8
  %_arg_.linearization.i8 = bitcast %struct.state* %_arg_.linearization to i8*
  %0 = getelementptr i8, i8* %_arg_.linearization.i8, i32 0
  store i8 %__arg_lin__arg_.0, i8* %0, align 1
  %1 = getelementptr i8, i8* %_arg_.linearization.i8, i32 8
  %2 = bitcast i8* %1 to i64*
  store i64 %__arg_lin__arg_.8, i64* %2, align 8
  %3 = getelementptr i8, i8* %_arg_.linearization.i8, i32 16
  %4 = bitcast i8* %3 to i64*
  store i64 %__arg_lin__arg_.16, i64* %4, align 8
  %5 = getelementptr i8, i8* %_arg_.linearization.i8, i32 24
  %6 = bitcast i8* %5 to i64*
  store i64 %__arg_lin__arg_.24, i64* %6, align 8
  %7 = getelementptr i8, i8* %_arg_.linearization.i8, i32 32
  %8 = bitcast i8* %7 to float*
  store float %__arg_lin__arg_.32, float* %8, align 4
  %pidx = getelementptr inbounds %struct.state, %struct.state* %_arg_.linearization, i64 0, i32 0
  %idx = load i8, i8* %pidx, align 4
  %ext = zext i8 %idx to i32
  %pint = getelementptr inbounds %struct.state, %struct.state* %_arg_.linearization, i64 0, i32 1, i32 %ext
  %9 = load i64, i64* %pint, align 8
  %10 = tail call <8 x i64> @llvm.vc.internal.lsc.load.ugm.v8i64.i1.v2i8.i64(i1 true, i8 3, i8 4, i8 5, <2 x i8> zeroinitializer, i64 0, i64 %9, i16 1, i32 0, <8 x i64> undef)
  tail call void @llvm.genx.vstore.v8i64.p0v8i64(<8 x i64> %10, <8 x i64>* nonnull @data)
  ret void
}

declare <8 x i64> @llvm.vc.internal.lsc.load.ugm.v8i64.i1.v2i8.i64(i1, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <8 x i64>) #2

attributes #0 = { "VCByteOffset"="0" "VCGlobalVariable" "VCVolatile" "genx_byte_offset"="0" "genx_volatile" }
attributes #1 = { nounwind "CMGenxMain" "oclrt"="1" "target-cpu"="XeHPC" }
attributes #2 = { "target-cpu"="XeHPC" }

!spirv.Source = !{!0}
!opencl.spir.version = !{!1}
!opencl.ocl.version = !{!2}
!opencl.used.extensions = !{!3}
!opencl.used.optional.core.features = !{!3}
!spirv.Generator = !{!4}
!genx.kernels = !{!5}
!genx.kernel.internal = !{!10}

; CHECK: !genx.kernels = !{![[KERNEL:[0-9]+]]}
; CHECK: ![[KERNEL]] = !{void (%struct.state*, i32 addrspace(1)*, i64, i8, i64, i64, i64, float)* @foo, !"foo", !{{[0-9]+}}, i32 0, !{{[0-9]+}}, !{{[0-9]+}}, ![[NODE:[0-9]+]], i32 0}
; CHECK: ![[NODE]] = !{!"svmptr_t", !"svmptr_t", !"", !"", !"svmptr_t", !"svmptr_t", !"svmptr_t", !""}

!0 = !{i32 0, i32 100000}
!1 = !{i32 1, i32 2}
!2 = !{i32 1, i32 0}
!3 = !{}
!4 = !{i16 6, i16 14}
!5 = !{void (%struct.state*, i32 addrspace(1)*, i64, i8, i64, i64, i64, float)* @foo, !"foo", !6, i32 0, !7, !8, !9, i32 0}
!6 = !{i32 112, i32 0, i32 96, i32 104, i32 104, i32 104, i32 104, i32 104}
!7 = !{i32 -1, i32 136, i32 64, i32 96, i32 104, i32 112, i32 120, i32 128}
!8 = !{i32 0, i32 0}
!9 = !{!"svmptr_t", !"svmptr_t"}
!10 = !{void (%struct.state*, i32 addrspace(1)*, i64, i8, i64, i64, i64, float)* @foo, !11, !12, !13, null}
!11 = !{i32 0, i32 0, i32 0, i32 0, i32 8, i32 16, i32 24, i32 32}
!12 = !{i32 0, i32 1, i32 2, i32 0, i32 0, i32 0, i32 0, i32 0}
!13 = !{!14}
!14 = !{i32 0, !15}
!15 = !{!16, !17, !18, !19, !20}
!16 = !{i32 3, i32 0}
!17 = !{i32 4, i32 8}
!18 = !{i32 5, i32 16}
!19 = !{i32 6, i32 24}
!20 = !{i32 7, i32 32}
