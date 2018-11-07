; RUN: igc_opt -igc-const-prop -S %s -o %t.ll | FileCheck %s --input-file=%t.ll

declare float @llvm.sqrt.f32(float %a)

; CHECK-LABEL: sqrt
; CHECK: ret float 0x3B53988E20000000
define float @sqrt() {
entry:
  %0 = call float @llvm.sqrt.f32(float 0x36B8000000000000)
  ret float %0
}

declare float @llvm.genx.GenISA.rsq.f32(float %a)

; CHECK-LABEL: rsq
; CHECK: ret float 0x448A20BD80000000
define float @rsq() {
entry:
  %0 = call float @llvm.genx.GenISA.rsq.f32(float 0x36B8000000000000)
  ret float %0
}

declare float @llvm.maxnum.f32(float %a, float %b)

; CHECK-LABEL: max
; CHECK: ret float 0x46B8000000000000
define float @max() {
entry:
  %0 = call float @llvm.maxnum.f32(float 0x36B8000000000000, float 0x46B8000000000000)
  ret float %0
}

declare float @llvm.minnum.f32(float %a, float %b)

; CHECK-LABEL: min
; CHECK: ret float 0x36B8000000000000
define float @min() {
entry:
  %0 = call float @llvm.minnum.f32(float 0x36B8000000000000, float 0x46B8000000000000)
  ret float %0
}

declare float @llvm.genx.GenISA.fsat.f32(float %a)

; CHECK-LABEL: fsat
; CHECK: ret float 0x36B8000000000000
define float @fsat() {
entry:
  %0 = call float @llvm.genx.GenISA.fsat.f32(float 0x36B8000000000000)
  ret float %0
}

declare float @llvm.cos.f32(float %a)

; CHECK-LABEL: cos
; CHECK: ret float 1.000000e+00
define float @cos() {
entry:
  %0 = call float @llvm.cos.f32(float 0x0000000000000000)
  ret float %0
}

declare float @llvm.sin.f32(float %a)

; CHECK-LABEL: sin
; CHECK: ret float 0x36B8000000000000
define float @sin() {
entry:
  %0 = call float @llvm.sin.f32(float 0x36B8000000000000)
  ret float %0
}

declare float @llvm.log.f32(float %a)

; CHECK-LABEL: log
; CHECK: ret float 0xC0598B8A60000000
define float @log() {
entry:
  %0 = call float @llvm.log.f32(float 0x36B8000000000000)
  ret float %0
}

declare float @llvm.exp.f32(float %a)

; CHECK-LABEL: exp
; CHECK: ret float 1.000000e+00
define float @exp() {
entry:
  %0 = call float @llvm.exp.f32(float 0x36B8000000000000)
  ret float %0
}

declare float @llvm.pow.f32(float %a, float %b)

; CHECK-LABEL: pow
; CHECK: ret float 1.000000e+00
define float @pow() {
entry:
  %0 = call float @llvm.pow.f32(float 0x36B8000000000000, float 0x0000000000000000)
  ret float %0
}

declare float @llvm.floor.f32(float %a)

; CHECK-LABEL: floor
; CHECK: ret float 0.000000e+00
define float @floor() {
entry:
  %0 = call float @llvm.floor.f32(float 0x36B8000000000000)
  ret float %0
}

declare float @llvm.ceil.f32(float %a)

; CHECK-LABEL: ceil
; CHECK: ret float 1.000000e+00
define float @ceil() {
entry:
  %0 = call float @llvm.ceil.f32(float 0x36B8000000000000)
  ret float %0
}

; CHECK-LABEL: foldcmp
; CHECK: ret i1 false
define i1 @foldcmp(i32 %a) {
entry:
  %0 = extractelement <4 x i32> <i32 1, i32 2, i32 3, i32 4>, i32 3
  %1 = icmp eq i32 0, %0
  ret i1 %1
}

; CHECK-LABEL: extractInsert
; CHECK: ret i32 5
define i32 @extractInsert(i32 %a) {
entry:
  %0 = insertelement <4 x i32> <i32 1, i32 2, i32 3, i32 4>, i32 5, i32 3
  %1 = extractelement <4 x i32> %0, i32 3
  ret i32 %1
}

; CHECK-LABEL: extractInsert2
; CHECK: ret i32 4
define i32 @extractInsert2(i32 %a) {
entry:
  %0 = insertelement <4 x i32> <i32 1, i32 2, i32 3, i32 4>, i32 5, i32 2
  %1 = extractelement <4 x i32> %0, i32 3
  ret i32 %1
}

; CHECK-LABEL: extractInsert3
; CHECK: ret i32 5
define i32 @extractInsert3(i32 %a) {
entry:
  %0 = insertelement <4 x i32> <i32 1, i32 2, i32 3, i32 4>, i32 5, i32 3
  %1 = insertelement <4 x i32> %0, i32 5, i32 2
  %2 = extractelement <4 x i32> %1, i32 3
  ret i32 %2
}

; CHECK-LABEL: extractInsert4
; CHECK: ret i32 3
define i32 @extractInsert4(<4 x i1> %a) {
entry:
  %0 = select <4 x i1> %a, <4 x i32> <i32 1, i32 2, i32 3, i32 4>, <4 x i32> <i32 5, i32 6, i32 3, i32 8>
  %1 = extractelement <4 x i32> %0, i32 2
  ret i32 %1
}