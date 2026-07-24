;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus, debug
; RUN: igc_opt --platformpvc -debug -igc-subgroup-2dblockio-resolution -S %s 2>&1 | FileCheck %s
; ------------------------------------------------
; Subgroup2DBlockIoResolutionPass -- isEntryFunc() igc_bif guard
; ------------------------------------------------
; A built-in library function carrying !igc_bif metadata is a callee, never a
; shader entry. Its FuncMD entry can default to KernelFunction (here @bif_helper
; is typed KernelFunction), which would otherwise make isEntryFunc() report it
; as an entry. Verify the entry-function query skips the igc_bif builtin and
; resolves the real kernel (@main) by walking up to its caller instead.

; CHECK:      - CHECK PARENT FUNCTION: main
; CHECK-NEXT: - FOUND ENTRY FUNCTION: main
define spir_func void @bif_helper(i8 addrspace(1)* %input, <2 x i32> %coord, i8* %output) !igc_bif !10 {
entry:
    call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 1, i32 2, i8 addrspace(1)* %input, i32 512, i32 46, i32 512, <2 x i32> %coord, i8* %output)
    ret void
}

define spir_kernel void @main(i8 addrspace(1)* %input, <2 x i32> %coord, i8* %output) {
entry:
    call spir_func void @bif_helper(i8 addrspace(1)* %input, <2 x i32> %coord, i8* %output)
    ret void
}

declare spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32, i32, i32, i32, i8 addrspace(1)*, i32, i32, i32, <2 x i32>, i8*)

!IGCMetadata = !{!7}
!igc.functions = !{!0}
!0 = !{ptr @main, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"functionType", !"KernelFunction"}
!4 = !{!"FuncMDMap[0]", ptr @main}
!5 = !{!"FuncMDValue[0]", !3}
!6 = !{!"FuncMD", !4, !5, !8, !9}
!7 = !{!"ModuleMD", !6}
!8 = !{!"FuncMDMap[1]", ptr @bif_helper}
!9 = !{!"FuncMDValue[1]", !3}
!10 = !{}
