;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, pvc-supported, llvm-14-plus

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=1 %s -o %t.bc
; RUN: ocloc -device pvc -llvm_input -options "-vc-codegen -ze-collect-cost-info -igc_opts 'EnableOpaquePointersBackend=1, ShaderDumpEnable=1, DumpToCustomDir=%t'" -output_no_suffix -file %t.bc
; RUN: cat %t/*.zeinfo | FileCheck %s

; LLVM with typed pointers:
; RUN: llvm-as -opaque-pointers=0 %s -o %t.bc
; RUN: ocloc -device pvc -llvm_input -options "-vc-codegen -ze-collect-cost-info -igc_opts 'ShaderDumpEnable=1, DumpToCustomDir=%t'" -output_no_suffix -file %t.bc
; RUN: cat %t/*.zeinfo | FileCheck %s

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; CHECK:      kernels_cost_info:
; CHECK-NEXT:     - name:            kernel

; CHECK:          kcm_args_sym:
; CHECK-NEXT:      - argNo:           3
; CHECK-NEXT:        byteOffset:      0
; CHECK-NEXT:        sizeInBytes:     4
; CHECK-NEXT:        isInDirect:      false
; CHECK-NEXT:      - argNo:           2
; CHECK-NEXT:        byteOffset:      0
; CHECK-NEXT:        sizeInBytes:     4
; CHECK-NEXT:        isInDirect:      false
; CHECK-NEXT:      - argNo:           1
; CHECK-NEXT:        byteOffset:      120
; CHECK-NEXT:        sizeInBytes:     8
; CHECK-NEXT:        isInDirect:      true

; CHECK:          kcm_loop_count_exps:
; CHECK-NEXT:      - factor:          1
; CHECK-NEXT:        argsym_index:    0
; CHECK-NEXT:        C:               0
; CHECK-NEXT:      - factor:          -0.25
; CHECK-NEXT:        argsym_index:    1
; CHECK-NEXT:        C:               3.75
; CHECK-NEXT:      - factor:          -1
; CHECK-NEXT:        argsym_index:    1
; CHECK-NEXT:        C:               -20
; CHECK-NEXT:      - factor:          1
; CHECK-NEXT:        argsym_index:    2
; CHECK-NEXT:        C:               0
; CHECK-NEXT:      - factor:          0
; CHECK-NEXT:        argsym_index:    -1
; CHECK-NEXT:        C:               127
; CHECK-NEXT:      - factor:          0
; CHECK-NEXT:        argsym_index:    -1
; CHECK-NEXT:        C:               0

; COM: The loop costs are estimated by finalizer. Only verify that
; COM: that the number of blocks equals the number of loops + 1.
; CHECK:          Kcm_loop_costs:
; CHECK-NEXT:      - cycle:           {{.*}}
; CHECK-NEXT:        bytes_loaded:    {{.*}}
; CHECK-NEXT:        bytes_stored:    {{.*}}
; CHECK-NEXT:        num_loops:       {{.*}}
; CHECK-COUNT-6:   - cycle:           {{.*}}

; COM: IR represents the following kernel:
; COM:  kernel(__global int *A, __global int *B, int C, int D) {
; COM:    for (int i = 0; i < D; ++i)
; COM:      A[0] = B[0];
; COM:    for (int i = 15; i > C; i -= 4)
; COM:      A[0] = B[0];
; COM:    for (int i = C; i > 2 * C + 20; --i)
; COM:      A[0] = B[0];
; COM:    for (int i = 0; i < B[30]; ++i)
; COM:      A[0] = B[0];
; COM:    for (int i = 0; i < 256; i += 2)
; COM:      A[0] = B[0];
; COM:    for (int i = C; i < D; ++i)
; COM:      A[0] = B[0];
; COM:  }

define spir_kernel void @kernel(i32 addrspace(1)* "VCArgumentIOKind"="0" %A, i32 addrspace(1)* "VCArgumentIOKind"="0" %B, i32 "VCArgumentIOKind"="0" %C, i32 "VCArgumentIOKind"="0" %D) #0 !spirv.ParameterDecorations !5 !intel_reqd_sub_group_size !8 {
entry:
  %cmp31 = icmp sgt i32 %D, 0
  br i1 %cmp31, label %for.body, label %for.cond3.preheader

for.cond3.preheader:                              ; preds = %for.body, %entry
  %cmp429 = icmp slt i32 %C, 15
  br i1 %cmp429, label %for.body5, label %for.cond11.preheader

for.body:                                         ; preds = %entry, %for.body
  %i.032 = phi i32 [ %inc, %for.body ], [ 0, %entry ]
  %0 = load i32, i32 addrspace(1)* %B, align 4
  store i32 %0, i32 addrspace(1)* %A, align 4
  %inc = add nuw nsw i32 %i.032, 1, !spirv.Decorations !14
  %exitcond33.not = icmp eq i32 %inc, %D
  br i1 %exitcond33.not, label %for.cond3.preheader, label %for.body

for.cond11.preheader:                             ; preds = %for.body5, %for.cond3.preheader
  %mul = shl nsw i32 %C, 1
  %add = add nsw i32 %mul, 20, !spirv.Decorations !14
  %cmp1227 = icmp slt i32 %add, %C
  br i1 %cmp1227, label %for.body13, label %for.cond19.preheader

for.body5:                                        ; preds = %for.cond3.preheader, %for.body5
  %i2.030 = phi i32 [ %sub, %for.body5 ], [ 15, %for.cond3.preheader ]
  %1 = load i32, i32 addrspace(1)* %B, align 4
  store i32 %1, i32 addrspace(1)* %A, align 4
  %sub = add nsw i32 %i2.030, -4
  %cmp4 = icmp sgt i32 %sub, %C
  br i1 %cmp4, label %for.body5, label %for.cond11.preheader

for.cond19.preheader:                             ; preds = %for.body13, %for.cond11.preheader
  %arrayidx20 = getelementptr inbounds i32, i32 addrspace(1)* %B, i64 30
  %2 = load i32, i32 addrspace(1)* %arrayidx20, align 4
  %cmp2125 = icmp sgt i32 %2, 0
  br i1 %cmp2125, label %for.body22, label %for.body31.preheader

for.body31.preheader:                             ; preds = %for.body22, %for.cond19.preheader
  br label %for.body31

for.body13:                                       ; preds = %for.cond11.preheader, %for.body13
  %i10.028 = phi i32 [ %dec, %for.body13 ], [ %C, %for.cond11.preheader ]
  %3 = load i32, i32 addrspace(1)* %B, align 4
  store i32 %3, i32 addrspace(1)* %A, align 4
  %dec = add nsw i32 %i10.028, -1, !spirv.Decorations !14
  %cmp12 = icmp sgt i32 %dec, %add
  br i1 %cmp12, label %for.body13, label %for.cond19.preheader

for.body22:                                       ; preds = %for.cond19.preheader, %for.body22
  %i18.026 = phi i32 [ %inc26, %for.body22 ], [ 0, %for.cond19.preheader ]
  %4 = load i32, i32 addrspace(1)* %B, align 4
  store i32 %4, i32 addrspace(1)* %A, align 4
  %inc26 = add nuw nsw i32 %i18.026, 1, !spirv.Decorations !14
  %5 = load i32, i32 addrspace(1)* %arrayidx20, align 4
  %cmp21 = icmp slt i32 %inc26, %5
  br i1 %cmp21, label %for.body22, label %for.body31.preheader

for.cond38.preheader:                             ; preds = %for.body31
  %cmp3922 = icmp slt i32 %C, %D
  br i1 %cmp3922, label %for.body40, label %for.end45

for.body31:                                       ; preds = %for.body31.preheader, %for.body31
  %i28.024 = phi i32 [ %add35, %for.body31 ], [ 0, %for.body31.preheader ]
  %6 = load i32, i32 addrspace(1)* %B, align 4
  store i32 %6, i32 addrspace(1)* %A, align 4
  %add35 = add nuw nsw i32 %i28.024, 2, !spirv.Decorations !14
  %cmp30 = icmp ult i32 %i28.024, 254
  br i1 %cmp30, label %for.body31, label %for.cond38.preheader

for.body40:                                       ; preds = %for.cond38.preheader, %for.body40
  %i37.023 = phi i32 [ %inc44, %for.body40 ], [ %C, %for.cond38.preheader ]
  %7 = load i32, i32 addrspace(1)* %B, align 4
  store i32 %7, i32 addrspace(1)* %A, align 4
  %inc44 = add nsw i32 %i37.023, 1, !spirv.Decorations !14
  %exitcond.not = icmp eq i32 %inc44, %D
  br i1 %exitcond.not, label %for.end45, label %for.body40

for.end45:                                        ; preds = %for.body40, %for.cond38.preheader
  ret void
}

attributes #0 = { noinline nounwind "VCFunction" "VCNamedBarrierCount"="0" "VCSLMSize"="0" }

!spirv.MemoryModel = !{!0}
!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!1}
!opencl.spir.version = !{!2}
!opencl.ocl.version = !{!1}
!opencl.used.extensions = !{!3}
!opencl.used.optional.core.features = !{!3}
!spirv.Generator = !{!4}

!0 = !{i32 2, i32 2}
!1 = !{i32 0, i32 0}
!2 = !{i32 1, i32 2}
!3 = !{}
!4 = !{i16 6, i16 14}
!5 = !{!6, !6, !6, !6}
!6 = !{!7}
!7 = !{i32 5625, i32 0}
!8 = !{i32 1}
!9 = !{!10}
!10 = !{i32 44, i32 8}
!11 = !{!12}
!12 = !{i32 44, i32 4}
!13 = !{!14}
!14 = !{i32 4469}
