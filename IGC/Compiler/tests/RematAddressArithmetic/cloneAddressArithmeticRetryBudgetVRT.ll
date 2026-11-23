;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: regkeys

; RUN: split-file %s %t
; RUN: igc_opt --opaque-pointers %t/band.ll -S -o - -igc-clone-address-arithmetic --platformPtl --regkey=RematRPELimit=5 \
; RUN:   | FileCheck %s --check-prefix=BAND
; RUN: igc_opt --opaque-pointers %t/band.ll -S -o - -igc-clone-address-arithmetic --platformPtl --regkey=RematRPELimit=5 \
; RUN:   --regkey=RematRetryRollback=0 | FileCheck %s --check-prefix=BAND-KEEP
; RUN: igc_opt --opaque-pointers %t/ceiling.ll -S -o - -igc-clone-address-arithmetic --platformPtl --regkey=RematRPELimit=5 \
; RUN:   | FileCheck %s --check-prefix=CEILING-KEEP
; RUN: igc_opt --opaque-pointers %t/ceiling.ll -S -o - -igc-clone-address-arithmetic --platformPtl --regkey=RematRPELimit=5 \
; RUN:   --regkey=ForceVRTGRFCeiling=96 | FileCheck %s --check-prefix=CEILING

; Xe3 merged VRT table: {96,10} {128,8} {160,6} {192,5} {256,4}. Cloning the chains keeps both PHIs of each chain live
; up to its store. @band goes from 99 to 130 GRF, out of the 128 band into fewer threads, and is rolled back.
; BAND-NOT: add i64 %l0, %m0, !remat
; BAND-KEEP: add i64 %l0, %m0, !remat

; @ceiling goes from 97 to 104 GRF. That stays in the 128 band, unless the GRF ceiling is 96.
; CEILING-KEEP: add i64 %l0, %m0, !remat
; CEILING-NOT: add i64 %l0, %m0, !remat

;--- band.ll
define spir_kernel void @band(ptr addrspace(1) %p, ptr addrspace(1) %q, i1 %c) {
entry:
  %f = load <15 x i64>, ptr addrspace(1) %q, align 8
  %x0 = load i64, ptr addrspace(1) %q, align 8
  %y0 = load i64, ptr addrspace(1) %p, align 8
  %x1 = load i64, ptr addrspace(1) %q, align 8
  %y1 = load i64, ptr addrspace(1) %p, align 8
  %x2 = load i64, ptr addrspace(1) %q, align 8
  %y2 = load i64, ptr addrspace(1) %p, align 8
  %x3 = load i64, ptr addrspace(1) %q, align 8
  %y3 = load i64, ptr addrspace(1) %p, align 8
  %x4 = load i64, ptr addrspace(1) %q, align 8
  %y4 = load i64, ptr addrspace(1) %p, align 8
  %x5 = load i64, ptr addrspace(1) %q, align 8
  %y5 = load i64, ptr addrspace(1) %p, align 8
  %x6 = load i64, ptr addrspace(1) %q, align 8
  %y6 = load i64, ptr addrspace(1) %p, align 8
  %x7 = load i64, ptr addrspace(1) %q, align 8
  %y7 = load i64, ptr addrspace(1) %p, align 8
  %x8 = load i64, ptr addrspace(1) %q, align 8
  %y8 = load i64, ptr addrspace(1) %p, align 8
  %x9 = load i64, ptr addrspace(1) %q, align 8
  %y9 = load i64, ptr addrspace(1) %p, align 8
  %x10 = load i64, ptr addrspace(1) %q, align 8
  %y10 = load i64, ptr addrspace(1) %p, align 8
  %x11 = load i64, ptr addrspace(1) %q, align 8
  %y11 = load i64, ptr addrspace(1) %p, align 8
  %x12 = load i64, ptr addrspace(1) %q, align 8
  %y12 = load i64, ptr addrspace(1) %p, align 8
  %x13 = load i64, ptr addrspace(1) %q, align 8
  %y13 = load i64, ptr addrspace(1) %p, align 8
  %x14 = load i64, ptr addrspace(1) %q, align 8
  %y14 = load i64, ptr addrspace(1) %p, align 8
  %x15 = load i64, ptr addrspace(1) %q, align 8
  %y15 = load i64, ptr addrspace(1) %p, align 8
  br i1 %c, label %t, label %j
t:
  br label %j
j:
  %l0 = phi i64 [ %x0, %entry ], [ %y0, %t ]
  %m0 = phi i64 [ %y0, %entry ], [ %x0, %t ]
  %l1 = phi i64 [ %x1, %entry ], [ %y1, %t ]
  %m1 = phi i64 [ %y1, %entry ], [ %x1, %t ]
  %l2 = phi i64 [ %x2, %entry ], [ %y2, %t ]
  %m2 = phi i64 [ %y2, %entry ], [ %x2, %t ]
  %l3 = phi i64 [ %x3, %entry ], [ %y3, %t ]
  %m3 = phi i64 [ %y3, %entry ], [ %x3, %t ]
  %l4 = phi i64 [ %x4, %entry ], [ %y4, %t ]
  %m4 = phi i64 [ %y4, %entry ], [ %x4, %t ]
  %l5 = phi i64 [ %x5, %entry ], [ %y5, %t ]
  %m5 = phi i64 [ %y5, %entry ], [ %x5, %t ]
  %l6 = phi i64 [ %x6, %entry ], [ %y6, %t ]
  %m6 = phi i64 [ %y6, %entry ], [ %x6, %t ]
  %l7 = phi i64 [ %x7, %entry ], [ %y7, %t ]
  %m7 = phi i64 [ %y7, %entry ], [ %x7, %t ]
  %l8 = phi i64 [ %x8, %entry ], [ %y8, %t ]
  %m8 = phi i64 [ %y8, %entry ], [ %x8, %t ]
  %l9 = phi i64 [ %x9, %entry ], [ %y9, %t ]
  %m9 = phi i64 [ %y9, %entry ], [ %x9, %t ]
  %l10 = phi i64 [ %x10, %entry ], [ %y10, %t ]
  %m10 = phi i64 [ %y10, %entry ], [ %x10, %t ]
  %l11 = phi i64 [ %x11, %entry ], [ %y11, %t ]
  %m11 = phi i64 [ %y11, %entry ], [ %x11, %t ]
  %l12 = phi i64 [ %x12, %entry ], [ %y12, %t ]
  %m12 = phi i64 [ %y12, %entry ], [ %x12, %t ]
  %l13 = phi i64 [ %x13, %entry ], [ %y13, %t ]
  %m13 = phi i64 [ %y13, %entry ], [ %x13, %t ]
  %l14 = phi i64 [ %x14, %entry ], [ %y14, %t ]
  %m14 = phi i64 [ %y14, %entry ], [ %x14, %t ]
  %l15 = phi i64 [ %x15, %entry ], [ %y15, %t ]
  %m15 = phi i64 [ %y15, %entry ], [ %x15, %t ]
  %a0 = add i64 %l0, %m0
  %g0 = getelementptr i64, ptr addrspace(1) %p, i64 %a0
  %a1 = add i64 %l1, %m1
  %g1 = getelementptr i64, ptr addrspace(1) %p, i64 %a1
  %a2 = add i64 %l2, %m2
  %g2 = getelementptr i64, ptr addrspace(1) %p, i64 %a2
  %a3 = add i64 %l3, %m3
  %g3 = getelementptr i64, ptr addrspace(1) %p, i64 %a3
  %a4 = add i64 %l4, %m4
  %g4 = getelementptr i64, ptr addrspace(1) %p, i64 %a4
  %a5 = add i64 %l5, %m5
  %g5 = getelementptr i64, ptr addrspace(1) %p, i64 %a5
  %a6 = add i64 %l6, %m6
  %g6 = getelementptr i64, ptr addrspace(1) %p, i64 %a6
  %a7 = add i64 %l7, %m7
  %g7 = getelementptr i64, ptr addrspace(1) %p, i64 %a7
  %a8 = add i64 %l8, %m8
  %g8 = getelementptr i64, ptr addrspace(1) %p, i64 %a8
  %a9 = add i64 %l9, %m9
  %g9 = getelementptr i64, ptr addrspace(1) %p, i64 %a9
  %a10 = add i64 %l10, %m10
  %g10 = getelementptr i64, ptr addrspace(1) %p, i64 %a10
  %a11 = add i64 %l11, %m11
  %g11 = getelementptr i64, ptr addrspace(1) %p, i64 %a11
  %a12 = add i64 %l12, %m12
  %g12 = getelementptr i64, ptr addrspace(1) %p, i64 %a12
  %a13 = add i64 %l13, %m13
  %g13 = getelementptr i64, ptr addrspace(1) %p, i64 %a13
  %a14 = add i64 %l14, %m14
  %g14 = getelementptr i64, ptr addrspace(1) %p, i64 %a14
  %a15 = add i64 %l15, %m15
  %g15 = getelementptr i64, ptr addrspace(1) %p, i64 %a15
  %w0 = load i64, ptr addrspace(1) %q, align 8
  %w1 = load i64, ptr addrspace(1) %q, align 8
  %w2 = load i64, ptr addrspace(1) %q, align 8
  %w3 = load i64, ptr addrspace(1) %q, align 8
  %w4 = load i64, ptr addrspace(1) %q, align 8
  %w5 = load i64, ptr addrspace(1) %q, align 8
  %w6 = load i64, ptr addrspace(1) %q, align 8
  %w7 = load i64, ptr addrspace(1) %q, align 8
  %w8 = load i64, ptr addrspace(1) %q, align 8
  %w9 = load i64, ptr addrspace(1) %q, align 8
  %w10 = load i64, ptr addrspace(1) %q, align 8
  %w11 = load i64, ptr addrspace(1) %q, align 8
  %w12 = load i64, ptr addrspace(1) %q, align 8
  %w13 = load i64, ptr addrspace(1) %q, align 8
  %w14 = load i64, ptr addrspace(1) %q, align 8
  %w15 = load i64, ptr addrspace(1) %q, align 8
  store i64 %w0, ptr addrspace(1) %g0, align 8
  store i64 %w1, ptr addrspace(1) %g1, align 8
  store i64 %w2, ptr addrspace(1) %g2, align 8
  store i64 %w3, ptr addrspace(1) %g3, align 8
  store i64 %w4, ptr addrspace(1) %g4, align 8
  store i64 %w5, ptr addrspace(1) %g5, align 8
  store i64 %w6, ptr addrspace(1) %g6, align 8
  store i64 %w7, ptr addrspace(1) %g7, align 8
  store i64 %w8, ptr addrspace(1) %g8, align 8
  store i64 %w9, ptr addrspace(1) %g9, align 8
  store i64 %w10, ptr addrspace(1) %g10, align 8
  store i64 %w11, ptr addrspace(1) %g11, align 8
  store i64 %w12, ptr addrspace(1) %g12, align 8
  store i64 %w13, ptr addrspace(1) %g13, align 8
  store i64 %w14, ptr addrspace(1) %g14, align 8
  store i64 %w15, ptr addrspace(1) %g15, align 8
  store i64 %w0, ptr addrspace(1) %q, align 8
  store i64 %w1, ptr addrspace(1) %q, align 8
  store i64 %w2, ptr addrspace(1) %q, align 8
  store i64 %w3, ptr addrspace(1) %q, align 8
  store i64 %w4, ptr addrspace(1) %q, align 8
  store i64 %w5, ptr addrspace(1) %q, align 8
  store i64 %w6, ptr addrspace(1) %q, align 8
  store i64 %w7, ptr addrspace(1) %q, align 8
  store i64 %w8, ptr addrspace(1) %q, align 8
  store i64 %w9, ptr addrspace(1) %q, align 8
  store i64 %w10, ptr addrspace(1) %q, align 8
  store i64 %w11, ptr addrspace(1) %q, align 8
  store i64 %w12, ptr addrspace(1) %q, align 8
  store i64 %w13, ptr addrspace(1) %q, align 8
  store i64 %w14, ptr addrspace(1) %q, align 8
  store i64 %w15, ptr addrspace(1) %q, align 8
  store <15 x i64> %f, ptr addrspace(1) %q, align 8
  ret void
}

;--- ceiling.ll
define spir_kernel void @ceiling(ptr addrspace(1) %p, ptr addrspace(1) %q, i1 %c) {
entry:
  %f = load <38 x i64>, ptr addrspace(1) %q, align 8
  %x0 = load i64, ptr addrspace(1) %q, align 8
  %y0 = load i64, ptr addrspace(1) %p, align 8
  %x1 = load i64, ptr addrspace(1) %q, align 8
  %y1 = load i64, ptr addrspace(1) %p, align 8
  %x2 = load i64, ptr addrspace(1) %q, align 8
  %y2 = load i64, ptr addrspace(1) %p, align 8
  %x3 = load i64, ptr addrspace(1) %q, align 8
  %y3 = load i64, ptr addrspace(1) %p, align 8
  br i1 %c, label %t, label %j
t:
  br label %j
j:
  %l0 = phi i64 [ %x0, %entry ], [ %y0, %t ]
  %m0 = phi i64 [ %y0, %entry ], [ %x0, %t ]
  %l1 = phi i64 [ %x1, %entry ], [ %y1, %t ]
  %m1 = phi i64 [ %y1, %entry ], [ %x1, %t ]
  %l2 = phi i64 [ %x2, %entry ], [ %y2, %t ]
  %m2 = phi i64 [ %y2, %entry ], [ %x2, %t ]
  %l3 = phi i64 [ %x3, %entry ], [ %y3, %t ]
  %m3 = phi i64 [ %y3, %entry ], [ %x3, %t ]
  %a0 = add i64 %l0, %m0
  %g0 = getelementptr i64, ptr addrspace(1) %p, i64 %a0
  %a1 = add i64 %l1, %m1
  %g1 = getelementptr i64, ptr addrspace(1) %p, i64 %a1
  %a2 = add i64 %l2, %m2
  %g2 = getelementptr i64, ptr addrspace(1) %p, i64 %a2
  %a3 = add i64 %l3, %m3
  %g3 = getelementptr i64, ptr addrspace(1) %p, i64 %a3
  %w0 = load i64, ptr addrspace(1) %q, align 8
  %w1 = load i64, ptr addrspace(1) %q, align 8
  %w2 = load i64, ptr addrspace(1) %q, align 8
  %w3 = load i64, ptr addrspace(1) %q, align 8
  store i64 %w0, ptr addrspace(1) %g0, align 8
  store i64 %w1, ptr addrspace(1) %g1, align 8
  store i64 %w2, ptr addrspace(1) %g2, align 8
  store i64 %w3, ptr addrspace(1) %g3, align 8
  store i64 %w0, ptr addrspace(1) %q, align 8
  store i64 %w1, ptr addrspace(1) %q, align 8
  store i64 %w2, ptr addrspace(1) %q, align 8
  store i64 %w3, ptr addrspace(1) %q, align 8
  store <38 x i64> %f, ptr addrspace(1) %q, align 8
  ret void
}
