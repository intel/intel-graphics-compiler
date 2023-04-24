;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check that dp4a legalization does nothing

; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK-LABEL: @ssdp4a

define void @ssdp4a(<32 x i32> %0, <32 x i32> %1, <32 x i32> %2) {
; CHECK:      %4 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %0, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %5 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %1, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %6 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %2, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %7 = tail call <2 x i32> @llvm.genx.ssdp4a.v2i32.v2i32.v2i32.v2i32(<2 x i32> %4, <2 x i32> %5, <2 x i32> %6)
; CHECK-NEXT: ret void

  %4 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %0, i32 0, i32 2, i32 1, i16 0, i32 undef)
  %5 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %1, i32 0, i32 2, i32 1, i16 0, i32 undef)
  %6 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %2, i32 0, i32 2, i32 1, i16 0, i32 undef)

  %7 = tail call <2 x i32> @llvm.genx.ssdp4a.v2i32.v2i32.v2i32.v2i32(<2 x i32> %4, <2 x i32> %5, <2 x i32> %6)

  ret void
}

; CHECK-LABEL: @sudp4a

define void @sudp4a(<32 x i32> %0, <32 x i32> %1, <32 x i32> %2) {
; CHECK:      %4 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %0, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %5 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %1, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %6 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %2, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %7 = tail call <2 x i32> @llvm.genx.sudp4a.v2i32.v2i32.v2i32.v2i32(<2 x i32> %4, <2 x i32> %5, <2 x i32> %6)
; CHECK-NEXT: ret void

  %4 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %0, i32 0, i32 2, i32 1, i16 0, i32 undef)
  %5 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %1, i32 0, i32 2, i32 1, i16 0, i32 undef)
  %6 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %2, i32 0, i32 2, i32 1, i16 0, i32 undef)

  %7 = tail call <2 x i32> @llvm.genx.sudp4a.v2i32.v2i32.v2i32.v2i32(<2 x i32> %4, <2 x i32> %5, <2 x i32> %6)

  ret void
}

; CHECK-LABEL: @usdp4a

define void @usdp4a(<32 x i32> %0, <32 x i32> %1, <32 x i32> %2) {
; CHECK:      %4 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %0, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %5 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %1, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %6 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %2, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %7 = tail call <2 x i32> @llvm.genx.usdp4a.v2i32.v2i32.v2i32.v2i32(<2 x i32> %4, <2 x i32> %5, <2 x i32> %6)
; CHECK-NEXT: ret void

  %4 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %0, i32 0, i32 2, i32 1, i16 0, i32 undef)
  %5 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %1, i32 0, i32 2, i32 1, i16 0, i32 undef)
  %6 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %2, i32 0, i32 2, i32 1, i16 0, i32 undef)

  %7 = tail call <2 x i32> @llvm.genx.usdp4a.v2i32.v2i32.v2i32.v2i32(<2 x i32> %4, <2 x i32> %5, <2 x i32> %6)

  ret void
}

; CHECK-LABEL: @uudp4a

define void @uudp4a(<32 x i32> %0, <32 x i32> %1, <32 x i32> %2) {
; CHECK:      %4 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %0, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %5 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %1, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %6 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %2, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %7 = tail call <2 x i32> @llvm.genx.uudp4a.v2i32.v2i32.v2i32.v2i32(<2 x i32> %4, <2 x i32> %5, <2 x i32> %6)
; CHECK-NEXT: ret void

  %4 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %0, i32 0, i32 2, i32 1, i16 0, i32 undef)
  %5 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %1, i32 0, i32 2, i32 1, i16 0, i32 undef)
  %6 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %2, i32 0, i32 2, i32 1, i16 0, i32 undef)

  %7 = tail call <2 x i32> @llvm.genx.uudp4a.v2i32.v2i32.v2i32.v2i32(<2 x i32> %4, <2 x i32> %5, <2 x i32> %6)

  ret void
}

; CHECK-LABEL: @ssdp4a.sat

define void @ssdp4a.sat(<32 x i32> %0, <32 x i32> %1, <32 x i32> %2) {
; CHECK:      %4 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %0, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %5 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %1, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %6 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %2, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %7 = tail call <2 x i32> @llvm.genx.ssdp4a.sat.v2i32.v2i32.v2i32.v2i32(<2 x i32> %4, <2 x i32> %5, <2 x i32> %6)
; CHECK-NEXT: ret void

  %4 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %0, i32 0, i32 2, i32 1, i16 0, i32 undef)
  %5 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %1, i32 0, i32 2, i32 1, i16 0, i32 undef)
  %6 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %2, i32 0, i32 2, i32 1, i16 0, i32 undef)

  %7 = tail call <2 x i32> @llvm.genx.ssdp4a.sat.v2i32.v2i32.v2i32.v2i32(<2 x i32> %4, <2 x i32> %5, <2 x i32> %6)

  ret void
}

; CHECK-LABEL: @sudp4a.sat

define void @sudp4a.sat(<32 x i32> %0, <32 x i32> %1, <32 x i32> %2) {
; CHECK:      %4 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %0, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %5 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %1, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %6 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %2, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %7 = tail call <2 x i32> @llvm.genx.sudp4a.sat.v2i32.v2i32.v2i32.v2i32(<2 x i32> %4, <2 x i32> %5, <2 x i32> %6)
; CHECK-NEXT: ret void

  %4 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %0, i32 0, i32 2, i32 1, i16 0, i32 undef)
  %5 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %1, i32 0, i32 2, i32 1, i16 0, i32 undef)
  %6 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %2, i32 0, i32 2, i32 1, i16 0, i32 undef)

  %7 = tail call <2 x i32> @llvm.genx.sudp4a.sat.v2i32.v2i32.v2i32.v2i32(<2 x i32> %4, <2 x i32> %5, <2 x i32> %6)

  ret void
}

; CHECK-LABEL: @usdp4a.sat

define void @usdp4a.sat(<32 x i32> %0, <32 x i32> %1, <32 x i32> %2) {
; CHECK:      %4 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %0, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %5 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %1, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %6 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %2, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %7 = tail call <2 x i32> @llvm.genx.usdp4a.sat.v2i32.v2i32.v2i32.v2i32(<2 x i32> %4, <2 x i32> %5, <2 x i32> %6)
; CHECK-NEXT: ret void

  %4 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %0, i32 0, i32 2, i32 1, i16 0, i32 undef)
  %5 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %1, i32 0, i32 2, i32 1, i16 0, i32 undef)
  %6 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %2, i32 0, i32 2, i32 1, i16 0, i32 undef)

  %7 = tail call <2 x i32> @llvm.genx.usdp4a.sat.v2i32.v2i32.v2i32.v2i32(<2 x i32> %4, <2 x i32> %5, <2 x i32> %6)

  ret void
}

; CHECK-LABEL: @uudp4a.sat

define void @uudp4a.sat(<32 x i32> %0, <32 x i32> %1, <32 x i32> %2) {
; CHECK:      %4 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %0, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %5 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %1, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %6 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %2, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %7 = tail call <2 x i32> @llvm.genx.uudp4a.sat.v2i32.v2i32.v2i32.v2i32(<2 x i32> %4, <2 x i32> %5, <2 x i32> %6)
; CHECK-NEXT: ret void

  %4 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %0, i32 0, i32 2, i32 1, i16 0, i32 undef)
  %5 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %1, i32 0, i32 2, i32 1, i16 0, i32 undef)
  %6 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> %2, i32 0, i32 2, i32 1, i16 0, i32 undef)

  %7 = tail call <2 x i32> @llvm.genx.uudp4a.sat.v2i32.v2i32.v2i32.v2i32(<2 x i32> %4, <2 x i32> %5, <2 x i32> %6)

  ret void
}

declare <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32>, i32, i32, i32, i16, i32)

declare <2 x i32> @llvm.genx.ssdp4a.v2i32.v2i32.v2i32.v2i32(<2 x i32>, <2 x i32>, <2 x i32>)
declare <2 x i32> @llvm.genx.sudp4a.v2i32.v2i32.v2i32.v2i32(<2 x i32>, <2 x i32>, <2 x i32>)
declare <2 x i32> @llvm.genx.usdp4a.v2i32.v2i32.v2i32.v2i32(<2 x i32>, <2 x i32>, <2 x i32>)
declare <2 x i32> @llvm.genx.uudp4a.v2i32.v2i32.v2i32.v2i32(<2 x i32>, <2 x i32>, <2 x i32>)
declare <2 x i32> @llvm.genx.ssdp4a.sat.v2i32.v2i32.v2i32.v2i32(<2 x i32>, <2 x i32>, <2 x i32>)
declare <2 x i32> @llvm.genx.sudp4a.sat.v2i32.v2i32.v2i32.v2i32(<2 x i32>, <2 x i32>, <2 x i32>)
declare <2 x i32> @llvm.genx.usdp4a.sat.v2i32.v2i32.v2i32.v2i32(<2 x i32>, <2 x i32>, <2 x i32>)
declare <2 x i32> @llvm.genx.uudp4a.sat.v2i32.v2i32.v2i32.v2i32(<2 x i32>, <2 x i32>, <2 x i32>)
