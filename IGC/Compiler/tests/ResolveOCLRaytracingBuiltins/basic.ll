;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -enable-debugify --igc-resolve-ocl-raytracing-builtins -platformdg2 -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,%RT_CHECK_PREFIX%
; ------------------------------------------------
; ResolveOCLRaytracingBuiltins
; ------------------------------------------------

; Test reduced from ocl kernel and checks builtins lowering

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

%struct.rtglobals_t = type opaque
%struct.rtfence_t = type opaque

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @rt_kernel(%struct.rtglobals_t addrspace(1)* %rt) {
; CHECK-LABEL: @rt_kernel(
; CHECK:  entry:
; CHECK:    [[RT_ADDR:%.*]] = alloca [[STRUCT_RTGLOBALS_T:%.*]] addrspace(1)*, align 8
; CHECK:    [[TBTD:%.*]] = alloca i32 addrspace(4)*, align 8
; CHECK:    [[GBTD:%.*]] = alloca i32 addrspace(4)*, align 8
; CHECK:    [[FC:%.*]] = alloca %struct.rtfence_t*, align 8
; CHECK:    store [[STRUCT_RTGLOBALS_T]] addrspace(1)* [[RT:%.*]], [[STRUCT_RTGLOBALS_T]] addrspace(1)** [[RT_ADDR]], align 8
; CHECK:    [[TMP0:%.*]] = load [[STRUCT_RTGLOBALS_T]] addrspace(1)*, [[STRUCT_RTGLOBALS_T]] addrspace(1)** [[RT_ADDR]], align 8
; CHECK:    [[TMP1:%.*]] = ptrtoint [[STRUCT_RTGLOBALS_T]] addrspace(1)* [[TMP0]] to i64
; CHECK:    [[TMP2:%.*]] = add i64 [[TMP1]], 0
; CHECK:    [[TMP3:%.*]] = inttoptr i64 [[TMP2]] to i64 addrspace(1)*
; CHECK:    [[RTMEMBASEPTR:%.*]] = load i64, i64 addrspace(1)* [[TMP3]]
; CHECK:    [[TMP4:%.*]] = ptrtoint [[STRUCT_RTGLOBALS_T]] addrspace(1)* [[TMP0]] to i64
; CHECK:    [[TMP5:%.*]] = add i64 [[TMP4]], 16
; CHECK:    [[TMP6:%.*]] = inttoptr i64 [[TMP5]] to i32 addrspace(1)*
; CHECK:    [[STACKSIZEPERRAY:%.*]] = load i32, i32 addrspace(1)* [[TMP6]]
; CHECK:    [[TMP7:%.*]] = ptrtoint [[STRUCT_RTGLOBALS_T]] addrspace(1)* [[TMP0]] to i64
; CHECK:    [[TMP8:%.*]] = add i64 [[TMP7]], 20
; CHECK:    [[TMP9:%.*]] = inttoptr i64 [[TMP8]] to i32 addrspace(1)*
; CHECK:    [[NUMDSSRTSTACKS:%.*]] = load i32, i32 addrspace(1)* [[TMP9]]
; CHECK:    [[TMP10:%.*]] = call i32 @llvm.genx.GenISA.dual.subslice.id()
; CHECK:    [[TMP11:%.*]] = zext i32 [[STACKSIZEPERRAY]] to i64
; CHECK:    [[TMP12:%.*]] = zext i32 [[NUMDSSRTSTACKS]] to i64
; CHECK:    [[TMP13:%.*]] = zext i32 [[TMP10]] to i64
; CHECK:    [[TMP14:%.*]] = mul i64 [[TMP13]], [[TMP12]]
; CHECK:    [[TMP15:%.*]] = call i16 @llvm.genx.GenISA.AsyncStackID()
; CHECK:    [[TMP16:%.*]] = zext i16 [[TMP15]] to i64
; CHECK:    [[TMP17:%.*]] = add i64 [[TMP14]], [[TMP16]]
; CHECK:    [[TMP18:%.*]] = mul i64 [[TMP17]], 64
; CHECK:    [[TMP19:%.*]] = mul i64 [[TMP18]], [[TMP11]]
; CHECK:    [[TMP20:%.*]] = add i64 [[TMP19]], [[RTMEMBASEPTR]]
; CHECK:    [[TMP21:%.*]] = inttoptr i64 [[TMP20]] to i8 addrspace(4)*
; CHECK:    [[TMP22:%.*]] = bitcast i8 addrspace(4)* [[TMP21]] to i32 addrspace(4)*
; CHECK:    store i32 addrspace(4)* [[TMP22]], i32 addrspace(4)** [[TBTD]], align 8
; CHECK:    [[TMP23:%.*]] = load [[STRUCT_RTGLOBALS_T]] addrspace(1)*, [[STRUCT_RTGLOBALS_T]] addrspace(1)** [[RT_ADDR]], align 8
; CHECK:    [[TMP24:%.*]] = ptrtoint [[STRUCT_RTGLOBALS_T]] addrspace(1)* [[TMP23]] to i64
; CHECK:    [[TMP25:%.*]] = add i64 [[TMP24]], 0
; CHECK:    [[TMP26:%.*]] = inttoptr i64 [[TMP25]] to i64 addrspace(1)*
; CHECK:    [[RTMEMBASEPTR1:%.*]] = load i64, i64 addrspace(1)* [[TMP26]]
; CHECK:    [[TMP27:%.*]] = ptrtoint [[STRUCT_RTGLOBALS_T]] addrspace(1)* [[TMP23]] to i64
; CHECK:    [[TMP28:%.*]] = add i64 [[TMP27]], 16
; CHECK:    [[TMP29:%.*]] = inttoptr i64 [[TMP28]] to i32 addrspace(1)*
; CHECK:    [[STACKSIZEPERRAY2:%.*]] = load i32, i32 addrspace(1)* [[TMP29]]
; CHECK:    [[TMP30:%.*]] = ptrtoint [[STRUCT_RTGLOBALS_T]] addrspace(1)* [[TMP23]] to i64
; CHECK:    [[TMP31:%.*]] = add i64 [[TMP30]], 20
; CHECK:    [[TMP32:%.*]] = inttoptr i64 [[TMP31]] to i32 addrspace(1)*
; CHECK:    [[NUMDSSRTSTACKS3:%.*]] = load i32, i32 addrspace(1)* [[TMP32]]
; CHECK:    [[TMP33:%.*]] = call i32 @llvm.genx.GenISA.dual.subslice.id()
; CHECK:    [[TMP34:%.*]] = zext i32 [[STACKSIZEPERRAY2]] to i64
; CHECK:    [[TMP35:%.*]] = zext i32 [[NUMDSSRTSTACKS3]] to i64
; CHECK:    [[TMP36:%.*]] = zext i32 [[TMP33]] to i64
; CHECK:    [[TMP37:%.*]] = mul i64 [[TMP36]], [[TMP35]]
; CHECK:    [[TMP38:%.*]] = mul i64 [[TMP37]], 64
; CHECK:    [[TMP39:%.*]] = mul i64 [[TMP38]], [[TMP34]]
; CHECK:    [[TMP40:%.*]] = add i64 [[TMP39]], [[RTMEMBASEPTR1]]
; CHECK:    [[TMP41:%.*]] = inttoptr i64 [[TMP40]] to i8 addrspace(4)*
; CHECK:    [[TMP42:%.*]] = bitcast i8 addrspace(4)* [[TMP41]] to i32 addrspace(4)*
; CHECK:    store i32 addrspace(4)* [[TMP42]], i32 addrspace(4)** [[GBTD]], align 8
; CHECK:    [[TMP43:%.*]] = load [[STRUCT_RTGLOBALS_T]] addrspace(1)*, [[STRUCT_RTGLOBALS_T]] addrspace(1)** [[RT_ADDR]], align 8
; CHECK:    call void @llvm.genx.GenISA.LSCFence(i32 0, i32 1, i32 0)
; CHECK-RT-NEXT:    [[TMP44:%.*]] = call i32 @llvm.genx.GenISA.TraceRaySync.p1struct.rtglobals_t.i32([[STRUCT_RTGLOBALS_T]] addrspace(1)* [[TMP43]], i32 257)
; CHECK:    [[TMP45:%.*]] = inttoptr i32 [[TMP44]] to %struct.rtfence_t*
; CHECK:    store %struct.rtfence_t* [[TMP45]], %struct.rtfence_t** [[FC]], align 8
; CHECK:    [[TMP46:%.*]] = load %struct.rtfence_t*, %struct.rtfence_t** [[FC]], align 8
; CHECK:    [[TMP47:%.*]] = ptrtoint %struct.rtfence_t* [[TMP46]] to i32
; CHECK:    call void @llvm.genx.GenISA.ReadTraceRaySync.i32(i32 [[TMP47]])
; CHECK:    ret void
;
entry:
  %rt.addr = alloca %struct.rtglobals_t addrspace(1)*, align 8
  %tbtd = alloca i32 addrspace(4)*, align 8
  %gbtd = alloca i32 addrspace(4)*, align 8
  %fc = alloca %struct.rtfence_t*, align 8
  store %struct.rtglobals_t addrspace(1)* %rt, %struct.rtglobals_t addrspace(1)** %rt.addr, align 8
  %0 = load %struct.rtglobals_t addrspace(1)*, %struct.rtglobals_t addrspace(1)** %rt.addr, align 8
  %call.i = call spir_func i8 addrspace(4)* @__builtin_IB_intel_get_thread_btd_stack(%struct.rtglobals_t addrspace(1)* %0)
  %1 = bitcast i8 addrspace(4)* %call.i to i32 addrspace(4)*
  store i32 addrspace(4)* %1, i32 addrspace(4)** %tbtd, align 8
  %2 = load %struct.rtglobals_t addrspace(1)*, %struct.rtglobals_t addrspace(1)** %rt.addr, align 8
  %call.i1 = call spir_func i8 addrspace(4)* @__builtin_IB_intel_get_global_btd_stack(%struct.rtglobals_t addrspace(1)* %2)
  %3 = bitcast i8 addrspace(4)* %call.i1 to i32 addrspace(4)*
  store i32 addrspace(4)* %3, i32 addrspace(4)** %gbtd, align 8
  %4 = load %struct.rtglobals_t addrspace(1)*, %struct.rtglobals_t addrspace(1)** %rt.addr, align 8
  %call.i2 = call spir_func %struct.rtfence_t* @__builtin_IB_intel_dispatch_trace_ray_query(%struct.rtglobals_t addrspace(1)* %4, i32 1, i32 1)
  store %struct.rtfence_t* %call.i2, %struct.rtfence_t** %fc, align 8
  %5 = load %struct.rtfence_t*, %struct.rtfence_t** %fc, align 8
  call spir_func void @__builtin_IB_intel_rt_sync(%struct.rtfence_t* %5)
  ret void
}

declare spir_func i8 addrspace(4)* @__builtin_IB_intel_get_thread_btd_stack(%struct.rtglobals_t addrspace(1)*) local_unnamed_addr
declare spir_func i8 addrspace(4)* @__builtin_IB_intel_get_global_btd_stack(%struct.rtglobals_t addrspace(1)*) local_unnamed_addr
declare spir_func %struct.rtfence_t* @__builtin_IB_intel_dispatch_trace_ray_query(%struct.rtglobals_t addrspace(1)*, i32, i32) local_unnamed_addr
declare spir_func void @__builtin_IB_intel_rt_sync(%struct.rtfence_t*) local_unnamed_addr


!igc.functions = !{!3}

!3 = !{void (%struct.rtglobals_t addrspace(1)*)* @rt_kernel, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"implicit_arg_desc", !7, !8, !9}
!7 = !{i32 0}
!8 = !{i32 1}
!9 = !{i32 12}
