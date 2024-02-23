;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: ;;;;;;;;;; RUNNERS ;;;;;;;;;;

; RUN: llc %s -march=genx64 -mcpu=XeHPC -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s

; RUN: not llc %s -march=genx64 -mcpu=XeHPCVG -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null 2>&1 \
; RUN: | FileCheck --check-prefix=CHECK-XeHPCVG %s

; COM: ;;;;;;;;;; CHECKERS ;;;;;;;;;;

; CHECK: dpas.s8.s8.8.1 (M1, 16) V{{[0-9]+}}.0 %null.0 V{{[0-9]+}}.0 V{{[0-9]+}}(0,0)

; CHECK-XeHPCVG: GenXCisaBuilder failed for:
; CHECK-XeHPCVG-SAME: %acc = call <16 x i32> @llvm.genx.dpas.nosrc0.v16i32.v128i32.v8i32(<128 x i32> %src1, <8 x i32> %src2, i32 17303560)
; CHECK-XeHPCVG-SAME: Intrinsic is not supported by <XeHPCVG> platform

; COM: ;;;;;;;;;; KERNEL ;;;;;;;;;;

declare <16 x i32> @llvm.genx.dpas.nosrc0.v16i32.v128i32.v8i32(<128 x i32>, <8 x i32>, i32)

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @the_test(i32 addrspace(1)* %buffer) local_unnamed_addr {
  %psrc1 = bitcast i32 addrspace(1)* %buffer to <128 x i32> addrspace(1)*
  %src1 = load <128 x i32>, <128 x i32> addrspace(1)* %psrc1
  %psrc2 = bitcast i32 addrspace(1)* %buffer to <8 x i32> addrspace(1)*
  %src2 = load <8 x i32>, <8 x i32> addrspace(1)* %psrc2

  %acc = call <16 x i32> @llvm.genx.dpas.nosrc0.v16i32.v128i32.v8i32(<128 x i32> %src1, <8 x i32> %src2, i32 17303560)

  %pacc = bitcast i32 addrspace(1)* %buffer to <16 x i32> addrspace(1)*
  store <16 x i32> %acc, <16 x i32> addrspace(1)* %pacc

  ret void
}

!spirv.Source = !{!0}
!opencl.spir.version = !{!1}
!opencl.ocl.version = !{!0}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!2}
!spirv.Generator = !{!3}
!genx.kernels = !{!4}
!genx.kernel.internal = !{!8}

!0 = !{i32 0}
!1 = !{i32 1, i32 2}
!2 = !{}
!3 = !{i16 6, i16 14}
!4 = !{void (i32 addrspace(1)*)* @the_test, !"the_test", !5, i32 0, !6, !0, !7, i32 0}
!5 = !{i32 0}
!6 = !{i32 64}
!7 = !{!"svmptr_t"}
!8 = !{void (i32 addrspace(1)*)* @the_test, null, null, null, null}
