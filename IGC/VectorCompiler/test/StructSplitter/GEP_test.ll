;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXStructSplitter -vc-struct-splitting=1 -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

; CHECK-DAG:   %[[G_F:[^ ]+]] = type { %[[C_F:[^ ]+]], float, %struct.A }
; CHECK-DAG:   %[[C_F]] = type { [5 x %struct.A], float, %struct.A }
; CHECK-DAG:   %[[G_I:[^ ]+]] = type { %[[C_I:[^ ]+]], %struct.B }
; CHECK-DAG:   %[[C_I]] = type { i32, [5 x %struct.B], %struct.B }

; CHECK-LABEL: main
; CHECK:       %[[G_F_AL:[^ ]+]] = alloca %[[G_F]]
; CHECK:       %[[G_I_AL:[^ ]+]] = alloca %[[G_I]]
; CHECK:       %[[C_F_GEP:[^ ]+]] = getelementptr %[[G_F]], %[[G_F]]* %[[G_F_AL]], i32 0, i32 0
; CHECK:       %[[C_I_GEP:[^ ]+]] = getelementptr %[[G_I]], %[[G_I]]* %[[G_I_AL]], i32 0, i32 0
; CHECK:       %[[D_F_GEP:[^ ]+]] = getelementptr %[[G_F]], %[[G_F]]* %[[G_F_AL]], i32 0, i32 2
; CHECK:       %[[D_I_GEP:[^ ]+]] = getelementptr %[[G_I]], %[[G_I]]* %[[G_I_AL]], i32 0, i32 1
; CHECK:       %[[aA_GEP:[^ ]+]] = getelementptr %[[C_F]], %[[C_F]]* %[[C_F_GEP]], i32 0, i32 0
; CHECK:       %[[A_GEP:[^ ]+]] = getelementptr inbounds [5 x %struct.A], [5 x %struct.A]* %[[aA_GEP]], i64 0, i64 3
; CHECK:       %[[aF_GEP:[^ ]+]] = getelementptr inbounds %struct.A, %struct.A* %[[A_GEP]], i32 0, i32 1
; CHECK:       %[[F_GEP:[^ ]+]] = getelementptr inbounds [5 x float], [5 x float]* %[[aF_GEP]], i64 0, i64 1
; CHECK:       %[[F1_GEP:[^ ]+]] = getelementptr %[[C_F]], %[[C_F]]* %[[C_F_GEP]], i32 0, i32 1
; CHECK:       %[[D_F1_GEP:[^ ]+]] = getelementptr %[[C_F]], %[[C_F]]* %[[C_F_GEP]], i32 0, i32 2
; CHECK:       %[[D_I1_GEP:[^ ]+]] = getelementptr %[[C_I]], %[[C_I]]* %[[C_I_GEP]], i32 0, i32 2

; CHECK:       %[[aI_GEP:[^ ]+]] = getelementptr inbounds %struct.B, %struct.B* %[[D_I_GEP]], i32 0, i32 1
; CHECK:       %[[I_GEP:[^ ]+]] = getelementptr inbounds [5 x i32], [5 x i32]* %[[aI_GEP]], i64 0, i64 2
; CHECK:       %[[aB_GEP:[^ ]+]] = getelementptr %[[C_I]], %[[C_I]]* %[[C_I_GEP]], i32 0, i32 1
; CHECK:       %[[B_GEP:[^ ]+]] = getelementptr inbounds [5 x %struct.B], [5 x %struct.B]* %[[aB_GEP]], i64 0, i64 0
; CHECK:       %[[I1_GEP:[^ ]+]] = getelementptr inbounds %struct.B, %struct.B* %[[B_GEP]], i32 0, i32 0



%struct.G = type { %struct.C, float, %struct.D }
%struct.C = type { [5 x %struct.A], i32, float, [5 x %struct.B], %struct.D }
%struct.A = type { float, [5 x float] }
%struct.B = type { i32, [5 x i32] }
%struct.D = type { %struct.A, %struct.B }

define dllexport spir_kernel void @main() #1 {
entry:
  %g = alloca %struct.G, align 4
  %c = getelementptr inbounds %struct.G, %struct.G* %g, i32 0, i32 0
  %dd = getelementptr inbounds %struct.G, %struct.G* %g, i32 0, i32 2
  %unuseda = alloca %struct.A, align 4
  %aA = getelementptr inbounds %struct.C, %struct.C* %c, i32 0, i32 0
  %arrayidx = getelementptr inbounds [5 x %struct.A], [5 x %struct.A]* %aA, i64 0, i64 3
  %af = getelementptr inbounds %struct.A, %struct.A* %arrayidx, i32 0, i32 1
  %arrayidx1 = getelementptr inbounds [5 x float], [5 x float]* %af, i64 0, i64 1
  %f = getelementptr inbounds %struct.C, %struct.C* %c, i32 0, i32 2
  %d = getelementptr inbounds %struct.C, %struct.C* %c, i32 0, i32 4
  %b = getelementptr inbounds %struct.D, %struct.D* %dd, i32 0, i32 1
  %ai = getelementptr inbounds %struct.B, %struct.B* %b, i32 0, i32 1
  %arrayidx2 = getelementptr inbounds [5 x i32], [5 x i32]* %ai, i64 0, i64 2
  %aB = getelementptr inbounds %struct.C, %struct.C* %c, i32 0, i32 3
  %arrayidx3 = getelementptr inbounds [5 x %struct.B], [5 x %struct.B]* %aB, i64 0, i64 0
  %i = getelementptr inbounds %struct.B, %struct.B* %arrayidx3, i32 0, i32 0
  ret void
}
