;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers %s -S -o - -igc-sink-gep-constant | FileCheck %s
; On opaque pointers we could end up with such group:
;
;  %C  = getelementptr inbounds %class.MC_Vector, %class.MC_Vector* %.privateBuffer, i64 0, i32 1
;  %C2 = getelementptr inbounds %class.MC_Vector, %class.MC_Vector* %.privateBuffer, i64 0, i32 1
;  %A  = getelementptr inbounds %class.MC_Particle, %class.MC_Particle* %.privateBuffer, i64 0, i32 4
;  %A2 = getelementptr inbounds %class.MC_Particle, %class.MC_Particle* %.privateBuffer, i64 0, i32 4
;
; The problem is that we have type mismatch here, and later, when such group is being processed it tries
; to access indices that aren't available in given type e.g 8th index of 3 element struct.
; Such problematic group should be splitted into two different groups and it solves that problem.
;
;  %C  = getelementptr inbounds %class.MC_Vector, %class.MC_Vector* %.privateBuffer, i64 0, i32 1
;  %C2 = getelementptr inbounds %class.MC_Vector, %class.MC_Vector* %.privateBuffer, i64 0, i32 1
;
;  %A  = getelementptr inbounds %class.MC_Particle, %class.MC_Particle* %.privateBuffer, i64 0, i32 4
;  %A2 = getelementptr inbounds %class.MC_Particle, %class.MC_Particle* %.privateBuffer, i64 0, i32 4
;

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%class.MC_Particle = type { %class.MC_Vector, %class.MC_Vector, %class.MC_Vector, i64, i64 }
%class.MC_Vector = type { double, double }

; CHECK: if.cond:
; [[GEP0:%.*]] = getelementptr %class.MC_Vector, ptr %.privateBuffer, i64 0
; [[GEP1:%.*]] = getelementptr %class.MC_Particle, ptr %.privateBuffer, i64 0
; [[GEP2:%.*]] = getelementptr %class.MC_Particle, ptr %.privateBuffer, i64 0, i32 1
; CHECK: else.cond:
; [[GEP3:%.*]] = getelementptr %class.MC_Vector, ptr %.privateBuffer, i64 0
; [[GEP4:%.*]] = getelementptr %class.MC_Particle, ptr %.privateBuffer, i64 0
; [[GEP5:%.*]] = getelementptr %class.MC_Particle, ptr %.privateBuffer, i64 0, i32 1

; [[PHI1:%.*]] = phi ptr [ [[GEP0]], %if.cond ], [ [[GEP3]], %else.cond ]
; [[PHI2:%.*]] = phi ptr [ [[GEP1]], %if.cond ], [ [[GEP4]], %else.cond ]
; [[PHI3:%.*]] = phi ptr [ [[GEP2]], %if.cond ], [ [[GEP5]], %else.cond ]

; CHECK-NOT: error

define void @f0(ptr %.privateBuffer, i1 %cond) {
entry:
  br i1 %cond, label %if.cond, label %else.cond
if.cond:
  %A  = getelementptr inbounds %class.MC_Particle, ptr %.privateBuffer, i64 0, i32 4
  %B  = getelementptr inbounds %class.MC_Particle, ptr %.privateBuffer, i64 0, i32 1, i32 1
  %C  = getelementptr inbounds %class.MC_Vector, ptr %.privateBuffer, i64 0, i32 1
  br label %exit
else.cond:
  %A2  = getelementptr inbounds %class.MC_Particle, ptr %.privateBuffer, i64 0, i32 4
  %B2  = getelementptr inbounds %class.MC_Particle, ptr %.privateBuffer, i64 0, i32 1, i32 1
  %C2  = getelementptr inbounds %class.MC_Vector, ptr %.privateBuffer, i64 0, i32 1
  br label %exit
exit:
  %PhiA1 = phi i64* [ %A, %if.cond ], [ %A2, %else.cond ]
  %PhiA2 = phi i64* [ %A, %if.cond ], [ %A2, %else.cond ]
  %PhiB1 = phi i64* [ %B, %if.cond ], [ %B2, %else.cond ]

  %PhiB2 = phi i64* [ %B, %if.cond ], [ %B2, %else.cond ]
  %PhiC1 = phi i64* [ %C, %if.cond ], [ %C2, %else.cond ]
  %PhiC2 = phi i64* [ %C, %if.cond ], [ %C2, %else.cond ]
  
  %LoadC1 = load i64, i64* %PhiC1, align 4
  %LoadC2 = load i64, i64* %PhiC2, align 4
  
  %LoadA1 = load i64, i64* %PhiA1, align 4
  %LoadA2 = load i64, i64* %PhiA2, align 4

  %LoadB1 = load i64, i64* %PhiB1, align 4
  %LoadB2 = load i64, i64* %PhiB2, align 4
  ret void
}
