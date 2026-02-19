;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-add-implicit-args -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that AddImplicitArgs pass handles variable debug info.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target triple = "igil_32_GEN8"

define void @test(float addrspace(1)* %dst) #0 {
entry:
  call void @llvm.dbg.value(metadata float addrspace(1)* %dst, i64 0, metadata !9, metadata !DIExpression()), !dbg !12
  store float 1.000000e+00, float addrspace(1)* %dst, align 4
  ret void

; CHECK: define void @test(float addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader)
; CHECK: call void @llvm.dbg.value({{.*}})
}

declare void @llvm.dbg.value(metadata, i64, metadata, metadata) #1

attributes #0 = { alwaysinline nounwind }
attributes #1 = { nounwind readnone }

!igc.functions = !{!7}

!2 = !{!"function_type", i32 0}
!3 = !{i32 0}  ;; R0
!4 = !{i32 1}  ;; PAYLOAD_HEADER
!5 = !{!"implicit_arg_desc", !3, !4}
!6 = !{!2, !5}
!7 = !{void (float addrspace(1)*)* @test, !6}

!llvm.module.flags = !{!8}
!8 = !{i32 2, !"Debug Info Version", i32 3}

!10 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)
!11 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !10, size: 64)
!9 = !DILocalVariable(name: "", line: 144, scope: !13, file: !15, type: !11)
!12 = !DILocation(line: 0, column: 0, scope: !13)

!13 = distinct !DISubprogram(unit: !14)

!14 = distinct !DICompileUnit(language: DW_LANG_C, file: !15)

!15 = !DIFile(filename: "<stdin>", directory: "")

!llvm.dbg.cu = !{!14}
