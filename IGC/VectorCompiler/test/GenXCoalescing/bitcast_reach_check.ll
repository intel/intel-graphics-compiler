;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXModule -GenXUnbalingWrapper -GenXNumberingWrapper \
; RUN:  -GenXLiveRangesWrapper -GenXCoalescingWrapper -march=genx64 \
; RUN:  -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; %bitcast.0 interferes call.i65.esimd since %call.i55.esimd (which has %bitcast.0 use) and %call.i66.esimd are baled.
; That leads to phicopy and twoaddrcopy. Check whether GenXUnbaling handle this.

; The CFG made to meet the following:
; %z.sroa.1 (which a user of %call) NOTREACHES %call.i65.esimd through %call.
; %z.sroa.1 REACHES %call.i65.esimd through %bitcast.0.

; Function Attrs: nounwind readonly
declare <16 x i64> @llvm.genx.lsc.load.stateless.v16i64.v1i1.v1i64(<1 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <1 x i64>, i32) #0

; Function Attrs: nounwind readnone
declare <1 x double> @llvm.genx.rdregionf.v1f64.v16f64.i16(<16 x double>, i32, i32, i32, i16, i32) #1

; Function Attrs: nounwind readnone
declare <16 x double> @llvm.genx.wrregionf.v16f64.v1f64.i16.v1i1(<16 x double>, <1 x double>, i32, i32, i32, i16, i32, <1 x i1>) #1

; Function Attrs: nounwind
declare void @llvm.genx.lsc.store.stateless.v1i1.v1i64.v16f64(<1 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <1 x i64>, <16 x double>, i32) #2

define dllexport spir_kernel void @getrf(i8 addrspace(1)* %_arg_a, i64 %_arg_lda, i8 addrspace(1)* %_arg_ipiv, i64 %splat) local_unnamed_addr #3 {
entry:
  br label %for.body

for.body:
  %splat.splatinsert = bitcast i64 %splat to <1 x i64>
  %call = tail call <16 x i64> @llvm.genx.lsc.load.stateless.v16i64.v1i1.v1i64(<1 x i1> <i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 6, i8 2, i8 0, <1 x i64> %splat.splatinsert, i32 0)
  %sycl_load = tail call <16 x i64> @llvm.genx.lsc.load.stateless.v16i64.v1i1.v1i64(<1 x i1> <i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 6, i8 2, i8 0, <1 x i64> <i64 256>, i32 0)
  %sycl_load.double = bitcast <16 x i64> %sycl_load to <16 x double>
  %trunc.body = trunc i64 %_arg_lda to i1
  br i1 %trunc.body, label %if.if, label %for.body.crit_edge

for.body.crit_edge:
  br label %for.end

if.if:
  %cmp.if.if = icmp eq i64 %_arg_lda, 4
  br i1 %cmp.if.if, label %if.if.1, label %for.end

if.if.1:
  %cmp.if.if.1 = icmp eq i64 %_arg_lda, 5
  br i1 %cmp.if.if.1, label %if.else, label %if.end

if.else:
  %bitcast.0 = bitcast <16 x i64> %call to <16 x double>
  %call.i55.esimd = tail call <1 x double> @llvm.genx.rdregionf.v1f64.v16f64.i16(<16 x double> %bitcast.0, i32 0, i32 1, i32 1, i16 0, i32 0)
  %call.i61.esimd = tail call <1 x double> @llvm.genx.rdregionf.v1f64.v16f64.i16(<16 x double> %sycl_load.double, i32 0, i32 1, i32 1, i16 120, i32 0)

  ; COM: No twoaddrcopy.
  ; CHECK: call.i65.esimd = tail call <16 x double> @llvm.genx.wrregionf.v16f64.v1f64.i16.v1i1(<16 x double> %bitcast.0, <1 x double> %call.i61.esimd, i32 0, i32 1, i32 1, i16 0, i32 0, <1 x i1> <i1 true>)
  %call.i65.esimd = tail call <16 x double> @llvm.genx.wrregionf.v16f64.v1f64.i16.v1i1(<16 x double> %bitcast.0, <1 x double> %call.i61.esimd, i32 0, i32 1, i32 1, i16 0, i32 0, <1 x i1> <i1 true>)
  %bitcast.1.0 = bitcast <16 x double> %call.i65.esimd to <16 x i64>
  %call.i66.esimd = tail call <16 x double> @llvm.genx.wrregionf.v16f64.v1f64.i16.v1i1(<16 x double> %sycl_load.double, <1 x double> %call.i55.esimd, i32 0, i32 1, i32 1, i16 120, i32 0, <1 x i1> <i1 true>)
  tail call spir_func void @sycl_store(<1 x i64> %splat.splatinsert, <16 x double> %call.i66.esimd)
  %cmp.if.else = icmp eq i64 %_arg_lda, 6
  br i1 %cmp.if.else, label %for.body, label %if.end

if.end:
  ; COM: No phicopy.
  ; CHECK: %z.sroa.0 = phi <16 x i64> [ %call, %if.if.1 ], [ %bitcast.1.0, %if.else ]
  %z.sroa.0 = phi <16 x i64> [ %call, %if.if.1 ], [ %bitcast.1.0, %if.else ]
  %z.sroa.0.d = bitcast <16 x i64> %z.sroa.0 to <16 x double>
  tail call spir_func void @sycl_store(<1 x i64> %splat.splatinsert, <16 x double> %z.sroa.0.d)
  br label %exit

for.end:
  %z.sroa.1 = phi <16 x i64> [ %call, %for.body.crit_edge ], [ %sycl_load, %if.if ]
  %z.sroa.1.d = bitcast <16 x i64> %z.sroa.1 to <16 x double>
  tail call spir_func void @sycl_store(<1 x i64> %splat.splatinsert, <16 x double> %z.sroa.1.d)
  br label %exit

exit:
  ret void
}

; Function Attrs: noinline nounwind
define internal void @sycl_store(<1 x i64> %offset, <16 x double> %tostore) unnamed_addr #4 {
entry:
  tail call void @llvm.genx.lsc.store.stateless.v1i1.v1i64.v16f64(<1 x i1> <i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 6, i8 2, i8 0, <1 x i64> <i64 1024>, <16 x double> %tostore, i32 0)
  ret void
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind readnone }
attributes #2 = { nounwind }
attributes #3 = { nounwind "CMGenxMain" "oclrt"="1" }
attributes #4 = { noinline nounwind }

!genx.kernels = !{!7}
!genx.kernel.internal = !{!12}

!4 = !{}
!7 = !{void (i8 addrspace(1)*, i64, i8 addrspace(1)*, i64)* @getrf, !"getrf", !8, i32 0, !9, !10, !11, i32 0}
!8 = !{i32 0, i32 0, i32 0, i32 96}
!9 = !{i32 136, i32 144, i32 152, i32 128}
!10 = !{i32 0, i32 0, i32 0}
!11 = !{!"svmptr_t", !"", !"svmptr_t"}
!12 = !{void (i8 addrspace(1)*, i64, i8 addrspace(1)*, i64)* @getrf, !13, !14, !4, !15}
!13 = !{i32 0, i32 0, i32 0, i32 0}
!14 = !{i32 0, i32 1, i32 2, i32 3}
!15 = !{i32 255, i32 -1, i32 255, i32 255}
