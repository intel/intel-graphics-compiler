;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXEmulate -march=genx64 -mtriple=spir64-unkonwn-unknown \
; RUN: -mcpu=XeLPG -S < %s | FileCheck %s

declare <2 x double> @llvm.sqrt.v2f64(<2 x double>)
declare <2 x double> @llvm.genx.sqrt.v2f64(<2 x double>)
declare <2 x double> @llvm.genx.ieee.sqrt.v2f64(<2 x double>)

define dllexport spir_kernel void @test_kernel(<2 x double> %x) {
  ; CHECK: = call <2 x double> @__cm_intrinsic_impl_fsqrt__ieee__v2______(<2 x double> %x)
  %1 = call <2 x double> @llvm.sqrt.v2f64(<2 x double> %x)
  ; CHECK: = call <2 x double> @__cm_intrinsic_impl_fsqrt__ieee__v2__nnan____(<2 x double> %x)
  %2 = call nnan <2 x double> @llvm.sqrt.v2f64(<2 x double> %x)
  ; CHECK: = call <2 x double> @__cm_intrinsic_impl_fsqrt__ieee__v2____ninf__(<2 x double> %x)
  %3 = call ninf <2 x double> @llvm.sqrt.v2f64(<2 x double> %x)
  ; CHECK: = call <2 x double> @__cm_intrinsic_impl_fsqrt__ieee__v2__nnan__ninf__(<2 x double> %x)
  %4 = call nnan ninf <2 x double> @llvm.sqrt.v2f64(<2 x double> %x)
  ; CHECK: = call <2 x double> @__cm_intrinsic_impl_fsqrt__ieee__v2______nsz(<2 x double> %x)
  %5 = call nsz <2 x double> @llvm.sqrt.v2f64(<2 x double> %x)
  ; CHECK: = call <2 x double> @__cm_intrinsic_impl_fsqrt__ieee__v2__nnan____nsz(<2 x double> %x)
  %6 = call nnan nsz <2 x double> @llvm.sqrt.v2f64(<2 x double> %x)
  ; CHECK: = call <2 x double> @__cm_intrinsic_impl_fsqrt__ieee__v2____ninf__nsz(<2 x double> %x)
  %7 = call ninf nsz <2 x double> @llvm.sqrt.v2f64(<2 x double> %x)
  ; CHECK: = call <2 x double> @__cm_intrinsic_impl_fsqrt__ieee__v2__nnan__ninf__nsz(<2 x double> %x)
  %8 = call nnan ninf nsz <2 x double> @llvm.sqrt.v2f64(<2 x double> %x)
  ; CHECK: = call <2 x double> @__cm_intrinsic_impl_fsqrt__fast__v2______(<2 x double> %x)
  %9 = call afn <2 x double> @llvm.sqrt.v2f64(<2 x double> %x)
  ; CHECK: = call <2 x double> @__cm_intrinsic_impl_fsqrt__fast__v2__nnan__ninf__nsz(<2 x double> %x)
  %10 = call fast <2 x double> @llvm.sqrt.v2f64(<2 x double> %x)

  ; CHECK: = call <2 x double> @__cm_intrinsic_impl_fsqrt__fast__v2______(<2 x double> %x)
  %11 = call <2 x double> @llvm.genx.sqrt.v2f64(<2 x double> %x)
  ; CHECK: = call <2 x double> @__cm_intrinsic_impl_fsqrt__ieee__v2______(<2 x double> %x)
  %12 = call <2 x double> @llvm.genx.ieee.sqrt.v2f64(<2 x double> %x)

  ret void
}

; COM: The presence of these __cm_intrinsic_* funcitions is a HACK to trick VC
; COM: backend into thinking that we have emulation routines
define <2 x double> @__cm_intrinsic_impl_fsqrt__ieee__v2______(<2 x double> %x) #0 {
  ret <2 x double> zeroinitializer
}
define <2 x double> @__cm_intrinsic_impl_fsqrt__ieee__v2__nnan____(<2 x double> %x) #0 {
  ret <2 x double> zeroinitializer
}
define <2 x double> @__cm_intrinsic_impl_fsqrt__ieee__v2____ninf__(<2 x double> %x) #0 {
  ret <2 x double> zeroinitializer
}
define <2 x double> @__cm_intrinsic_impl_fsqrt__ieee__v2__nnan__ninf__(<2 x double> %x) #0 {
  ret <2 x double> zeroinitializer
}
define <2 x double> @__cm_intrinsic_impl_fsqrt__ieee__v2______nsz(<2 x double> %x) #0 {
  ret <2 x double> zeroinitializer
}
define <2 x double> @__cm_intrinsic_impl_fsqrt__ieee__v2__nnan____nsz(<2 x double> %x) #0 {
  ret <2 x double> zeroinitializer
}
define <2 x double> @__cm_intrinsic_impl_fsqrt__ieee__v2____ninf__nsz(<2 x double> %x) #0 {
  ret <2 x double> zeroinitializer
}
define <2 x double> @__cm_intrinsic_impl_fsqrt__ieee__v2__nnan__ninf__nsz(<2 x double> %x) #0 {
  ret <2 x double> zeroinitializer
}
define <2 x double> @__cm_intrinsic_impl_fsqrt__fast__v2______(<2 x double> %x) #0 {
  ret <2 x double> zeroinitializer
}
define <2 x double> @__cm_intrinsic_impl_fsqrt__fast__v2__nnan__ninf__nsz(<2 x double> %x) #0 {
  ret <2 x double> zeroinitializer
}

attributes #0 = { "VC.Emulation.Routine" }
