;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check that bti is assigned to opaque pointer state arguments.

; RUN: opt %use_old_pass_manager% -GenXBTIAssignment -march=genx64 -mcpu=Gen9 -S < %s | FileCheck --check-prefix RAW %s
; RUN: opt %use_old_pass_manager% -GenXBTIAssignment -instcombine -march=genx64 -mcpu=Gen9 -S < %s | FileCheck --check-prefix CLEAN %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

declare void @use_value(i32)

%buf_rw_t = type opaque
%ocl.sampler_t = type opaque
%ocl.image2d_ro_t = type opaque
%ocl.image2d_rw_t = type opaque

; RAW-LABEL: @simple(
; CLEAN-LABEL: @simple(
define dllexport spir_kernel void @simple(%buf_rw_t* %surf, %ocl.sampler_t* %samp) #0 {
; RAW:        [[SURF_INT:%[^ ]+]] = ptrtoint %buf_rw_t* null to i32
; RAW-NEXT:   [[SAMP_INT:%[^ ]+]] = ptrtoint %ocl.sampler_t* null to i32
; RAW-NEXT:   call void @use_value(i32 [[SURF_INT]])
; RAW-NEXT:   call void @use_value(i32 [[SAMP_INT]])

; COM: Inst combine cleans extra casts.
; CLEAN:      call void @use_value(i32 0)
; CLEAN-NEXT: call void @use_value(i32 0)
  %surf.int = ptrtoint %buf_rw_t* %surf to i32
  %samp.int = ptrtoint %ocl.sampler_t* %samp to i32
  call void @use_value(i32 %surf.int)
  call void @use_value(i32 %samp.int)
  ret void
}

; RAW-LABEL: @mixed_srv_uav(
; CLEAN-LABEL: @mixed_srv_uav(
define dllexport spir_kernel void @mixed_srv_uav(%ocl.image2d_ro_t* %image_ro,
                                                 %ocl.image2d_rw_t* %image_rw,
                                                 %buf_rw_t* %buf,
                                                 %ocl.image2d_ro_t* %image_ro2) #0 {
; RAW:        [[SURF_INT1:%[^ ]+]] = ptrtoint %ocl.image2d_ro_t* null to i32
; RAW-NEXT:   [[SURF_INT2:%[^ ]+]] = ptrtoint %ocl.image2d_rw_t* inttoptr (i32 2 to %ocl.image2d_rw_t*) to i32
; RAW-NEXT:   [[SURF_INT3:%[^ ]+]] = ptrtoint %buf_rw_t* inttoptr (i32 3 to %buf_rw_t*) to i32
; RAW-NEXT:   [[SURF_INT4:%[^ ]+]] = ptrtoint %ocl.image2d_ro_t* inttoptr (i32 1 to %ocl.image2d_ro_t*) to i32
; RAW-NEXT:   call void @use_value(i32 [[SURF_INT1]])
; RAW-NEXT:   call void @use_value(i32 [[SURF_INT2]])
; RAW-NEXT:   call void @use_value(i32 [[SURF_INT3]])
; RAW-NEXT:   call void @use_value(i32 [[SURF_INT4]])

; COM: Inst combine cleanup.
; CLEAN:      call void @use_value(i32 0)
; CLEAN-NEXT: call void @use_value(i32 2)
; CLEAN-NEXT: call void @use_value(i32 3)
; CLEAN-NEXT: call void @use_value(i32 1)
  %surf.int.1 = ptrtoint %ocl.image2d_ro_t* %image_ro to i32
  %surf.int.2 = ptrtoint %ocl.image2d_rw_t* %image_rw to i32
  %surf.int.3 = ptrtoint %buf_rw_t* %buf to i32
  %surf.int.4 = ptrtoint %ocl.image2d_ro_t* %image_ro2 to i32
  call void @use_value(i32 %surf.int.1)
  call void @use_value(i32 %surf.int.2)
  call void @use_value(i32 %surf.int.3)
  call void @use_value(i32 %surf.int.4)
  ret void
}


attributes #0 = { "CMGenxMain" }

!genx.kernels = !{!0, !5}
!genx.kernel.internal = !{!4, !9}
; CHECK: !genx.kernel.internal = !{[[SIMPLE_NODE:![0-9]+]], [[MIXED_NODE:![0-9]+]]}
; CHECK-DAG: [[SIMPLE_NODE]] = !{void (%buf_rw_t*, %ocl.sampler_t*)* @simple, null, null, null, [[SIMPLE_BTIS:![0-9]+]]}
; CHECK-DAG: [[SIMPLE_BTIS]] = !{i32 0, i32 0}
; CHECK-DAG: [[MIXED_NODE]] = !{void (%ocl.image2d_ro_t*, %ocl.image2d_rw_t*, %buf_rw_t*, %ocl.image2d_ro_t*)* @mixed_srv_uav, null, null, null, [[MIXED_BTIS:![0-9]+]]}
; CHECK-DAG: [[MIXED_BTIS]] = !{i32 0, i32 2, i32 3, i32 1}

!0 = !{void (%buf_rw_t*, %ocl.sampler_t*)* @simple, !"simple", !1, i32 0, i32 0, !2, !3, i32 0}
!1 = !{i32 2, i32 1}
!2 = !{i32 0, i32 0}
!3 = !{!"buffer_t read_write", !"sampler_t"}
!4 = !{void (%buf_rw_t*, %ocl.sampler_t*)* @simple, null, null, null, null}

!5 = !{void (%ocl.image2d_ro_t*, %ocl.image2d_rw_t*, %buf_rw_t*, %ocl.image2d_ro_t*)* @mixed_srv_uav, !"mixed_srv_uav", !6, i32 0, i32 0, !7, !8, i32 0}
!6 = !{i32 2, i32 2, i32 2, i32 2}
!7 = !{i32 0, i32 0, i32 0, i32 0}
!8 = !{!"image2d_t read_only", !"image2d_t read_write", !"buffer_t", !"image2d_t read_only"}
!9 = !{void (%ocl.image2d_ro_t*, %ocl.image2d_rw_t*, %buf_rw_t*, %ocl.image2d_ro_t*)* @mixed_srv_uav, null, null, null, null}

