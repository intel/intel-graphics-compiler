;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: ;;;;;;;;;; RUNNERS ;;;;;;;;;;

; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=XeHPC -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=XeHPC -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s


; COM: ;;;;;;;;;; CHECKERS ;;;;;;;;;;

; CHECK-NOT: .decl V{{[0-9]+}} v_type=G type={{(d|ud)}} num_elts={{[0-9]+}} alias=<%sr0, 0>
; CHECK-NOT: .decl V{{[0-9]+}} v_type=G type={{(d|ud)}} num_elts={{[0-9]+}} alias=<%cr0, 0>
; CHECK-NOT: .decl V{{[0-9]+}} v_type=G type={{(d|ud)}} num_elts={{[0-9]+}} alias=<%msg0, 0>

; CHECK: add (M1, 1) [[TMP:V[0-9]+]](0,0)<1> %sr0(0,0)<0;1,0> %cr0(0,0)<0;1,0>
; CHECK: or (M1, 1) [[TMP]](0,0)<1> %msg0(0,0)<0;1,0> [[TMP]](0,0)<0;1,0>

; COM: ;;;;;;;;;; KERNEL ;;;;;;;;;;


declare void @llvm.genx.oword.st.v4i32(i32, i32, <4 x i32>)
declare i32 @llvm.genx.read.predef.reg.i32.i32(i32, i32)

define dllexport spir_kernel void @kernel(i32 %dst) local_unnamed_addr #1 {
  %sr0 = call i32 @llvm.genx.read.predef.reg.i32.i32(i32 13, i32 undef)
  %cr0 = call i32 @llvm.genx.read.predef.reg.i32.i32(i32 14, i32 undef)
  %msg0 = call i32 @llvm.genx.read.predef.reg.i32.i32(i32 20, i32 undef)

  %a = add i32 %sr0, %cr0
  %b = or i32 %msg0, %a

  %ins = insertelement <4 x i32> undef, i32 %b, i32 0
  %splat = shufflevector <4 x i32> %ins, <4 x i32> undef, <4 x i32> zeroinitializer

  tail call void @llvm.genx.oword.st.v4i32(i32 %dst, i32 0, <4 x i32> %splat)
  ret void
}

attributes #1 = { noinline nounwind "CMGenxMain" }
attributes #2 = { nofree nosync nounwind willreturn }

!genx.kernels = !{!4}
!genx.kernel.internal = !{!8}

!1 = !{i32 0}
!4 = !{void (i32)* @kernel, !"kernel", !5, i32 0, !6, !1, !7, i32 0}
!5 = !{i32 2}
!6 = !{i32 64}
!7 = !{!"buffer_t"}
!8 = !{void (i32)* @kernel, null, null, null, null}
