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
; CHECK-NEXT:      - argNo:           2
; CHECK-NEXT:        byteOffset:      0
; CHECK-NEXT:        sizeInBytes:     4
; CHECK-NEXT:        isInDirect:      false

; CHECK:          kcm_loop_count_exps:
; CHECK-NEXT:      - factor:          0.25
; CHECK-NEXT:        argsym_index:    0
; CHECK-NEXT:        C:               0
; CHECK-NEXT:      - factor:          0.0625
; CHECK-NEXT:        argsym_index:    0
; CHECK-NEXT:        C:               0
; CHECK-NEXT:      - factor:          0
; CHECK-NEXT:        argsym_index:    -1
; CHECK-NEXT:        C:               225
; CHECK-NEXT:      - factor:          1
; CHECK-NEXT:        argsym_index:    0
; CHECK-NEXT:        C:               0
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
; CHECK-COUNT-5:   - cycle:           {{.*}}

; COM: IR represents the following kernel:
; COM: kernel(__global unsigned *A, __global unsigned *B, unsigned C) {
; COM:   for (unsigned i = 0; i < C; i += 4)
; COM:     A[i] = B[i];
; COM:   for (unsigned i = 0; i < C; i += 16)
; COM:     A[i] = B[i];
; COM:   for (unsigned i = 32; i <= 256; ++i)
; COM:     A[i] = B[i];
; COM:   for (unsigned i = 0; i < C; ++i)
; COM:     for (unsigned j = i; j < 32; ++j)
; COM:       A[i] = B[j];
; COM: }

define spir_kernel void @kernel(i32 addrspace(1)* "VCArgumentIOKind"="0" %A, i32 addrspace(1)* "VCArgumentIOKind"="0" %B, i32 "VCArgumentIOKind"="0" %C) #0 !spirv.ParameterDecorations !5 !intel_reqd_sub_group_size !8 {
entry:
  %cmp31.not = icmp eq i32 %C, 0
  br i1 %cmp31.not, label %for.body17.preheader, label %for.body

for.body17.preheader:                             ; preds = %for.body6, %entry
  br label %for.body17

for.body:                                         ; preds = %entry, %for.body
  %i.032 = phi i32 [ %add, %for.body ], [ 0, %entry ]
  %idxprom = zext i32 %i.032 to i64
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %B, i64 %idxprom
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %A, i64 %idxprom
  store i32 %0, i32 addrspace(1)* %arrayidx2, align 4
  %add = add i32 %i.032, 4
  %cmp = icmp ult i32 %add, %C
  br i1 %cmp, label %for.body, label %for.body6

for.body6:                                        ; preds = %for.body, %for.body6
  %i3.030 = phi i32 [ %add12, %for.body6 ], [ 0, %for.body ]
  %idxprom7 = zext i32 %i3.030 to i64
  %arrayidx8 = getelementptr inbounds i32, i32 addrspace(1)* %B, i64 %idxprom7
  %1 = load i32, i32 addrspace(1)* %arrayidx8, align 4
  %arrayidx10 = getelementptr inbounds i32, i32 addrspace(1)* %A, i64 %idxprom7
  store i32 %1, i32 addrspace(1)* %arrayidx10, align 4
  %add12 = add i32 %i3.030, 16
  %cmp5 = icmp ult i32 %add12, %C
  br i1 %cmp5, label %for.body6, label %for.body17.preheader

for.cond25.preheader:                             ; preds = %for.body17
  br i1 %cmp31.not, label %for.end40, label %for.cond28.preheader.lr.ph

for.cond28.preheader.lr.ph:                       ; preds = %for.cond25.preheader
  %wide.trip.count = zext i32 %C to i64
  br label %for.cond28.preheader

for.body17:                                       ; preds = %for.body17.preheader, %for.body17
  %indvars.iv36 = phi i64 [ %indvars.iv.next37, %for.body17 ], [ 32, %for.body17.preheader ]
  %arrayidx19 = getelementptr inbounds i32, i32 addrspace(1)* %B, i64 %indvars.iv36
  %2 = load i32, i32 addrspace(1)* %arrayidx19, align 4
  %arrayidx21 = getelementptr inbounds i32, i32 addrspace(1)* %A, i64 %indvars.iv36
  store i32 %2, i32 addrspace(1)* %arrayidx21, align 4
  %indvars.iv.next37 = add nuw nsw i64 %indvars.iv36, 1
  %exitcond38.not = icmp eq i64 %indvars.iv.next37, 257
  br i1 %exitcond38.not, label %for.cond25.preheader, label %for.body17

for.cond28.preheader:                             ; preds = %for.cond28.preheader.lr.ph, %for.inc38
  %indvars.iv = phi i64 [ 0, %for.cond28.preheader.lr.ph ], [ %indvars.iv.next, %for.inc38 ]
  %cmp2923 = icmp ult i64 %indvars.iv, 32
  br i1 %cmp2923, label %for.body30.lr.ph, label %for.inc38

for.body30.lr.ph:                                 ; preds = %for.cond28.preheader
  %arrayidx34 = getelementptr inbounds i32, i32 addrspace(1)* %A, i64 %indvars.iv
  br label %for.body30

for.body30:                                       ; preds = %for.body30.lr.ph, %for.body30
  %indvars.iv33 = phi i64 [ %indvars.iv, %for.body30.lr.ph ], [ %indvars.iv.next34, %for.body30 ]
  %arrayidx32 = getelementptr inbounds i32, i32 addrspace(1)* %B, i64 %indvars.iv33
  %3 = load i32, i32 addrspace(1)* %arrayidx32, align 4
  store i32 %3, i32 addrspace(1)* %arrayidx34, align 4
  %indvars.iv.next34 = add nuw nsw i64 %indvars.iv33, 1
  %lftr.wideiv1 = trunc i64 %indvars.iv.next34 to i32
  %exitcond = icmp eq i32 %lftr.wideiv1, 32
  br i1 %exitcond, label %for.inc38, label %for.body30

for.inc38:                                        ; preds = %for.body30, %for.cond28.preheader
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond35.not = icmp eq i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond35.not, label %for.end40, label %for.cond28.preheader

for.end40:                                        ; preds = %for.inc38, %for.cond25.preheader
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
!5 = !{!6, !6, !6}
!6 = !{!7}
!7 = !{i32 5625, i32 0}
!8 = !{i32 1}
!9 = !{!10}
!10 = !{i32 44, i32 8}
!11 = !{!12}
!12 = !{i32 44, i32 4}
