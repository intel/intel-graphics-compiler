;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-device-enqueue-func-resolution -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; DeviceEnqueueFuncsResolution
; ------------------------------------------------

define spir_kernel void @test_device_queue(i8 addrspace(1)* %deviceEnqueueDefaultDeviceQueue) {
; CHECK-LABEL: @test_device_queue(
; CHECK:    [[TMP1:%.*]] = bitcast i8 addrspace(1)* %deviceEnqueueDefaultDeviceQueue to i32 addrspace(1)*
; CHECK:    call void @use.p1i32(i32 addrspace(1)* [[TMP1]])
; CHECK:    ret void
;
  %1 = call i8 addrspace(1)* @__builtin_IB_get_default_device_queue()
  %2 = bitcast i8 addrspace(1)* %1 to i32 addrspace(1)*
  call void @use.p1i32(i32 addrspace(1)* %2)
  ret void
}

define spir_kernel void @test_device_eventpool(i8 addrspace(1)* %deviceEnqueueEventPool) {
; CHECK-LABEL: @test_device_eventpool(
; CHECK:    [[TMP1:%.*]] = bitcast i8 addrspace(1)* %deviceEnqueueEventPool to i32 addrspace(1)*
; CHECK:    call void @use.p1i32(i32 addrspace(1)* [[TMP1]])
; CHECK:    ret void
;
  %1 = call i8 addrspace(1)* @__builtin_IB_get_event_pool()
  %2 = bitcast i8 addrspace(1)* %1 to i32 addrspace(1)*
  call void @use.p1i32(i32 addrspace(1)* %2)
  ret void
}

define spir_kernel void @test_device_maxworkgroup(i32 %deviceEnqueueMaxWorkgroupSize) {
; CHECK-LABEL: @test_device_maxworkgroup(
; CHECK:    call void @use.i32(i32 %deviceEnqueueMaxWorkgroupSize)
; CHECK:    ret void
;
  %1 = call i32 @__builtin_IB_get_max_workgroup_size()
  call void @use.i32(i32 %1)
  ret void
}

define spir_kernel void @test_device_parentevent(i32 %deviceEnqueueParentEvent) {
; CHECK-LABEL: @test_device_parentevent(
; CHECK:    call void @use.i32(i32 %deviceEnqueueParentEvent)
; CHECK:    ret void
;
  %1 = call i32 @__builtin_IB_get_parent_event()
  call void @use.i32(i32 %1)
  ret void
}

define spir_kernel void @test_device_wg_multiple(i32 %deviceEnqueuePreferedWorkgroupMultiple) {
; CHECK-LABEL: @test_device_wg_multiple(
; CHECK:    call void @use.i32(i32 %deviceEnqueuePreferedWorkgroupMultiple)
; CHECK:    ret void
;
  %1 = call i32 @__builtin_IB_get_prefered_workgroup_multiple()
  call void @use.i32(i32 %1)
  ret void
}

declare void @use.p1i32(i32 addrspace(1)*)
declare void @use.i32(i32)

declare i8 addrspace(1)* @__builtin_IB_get_default_device_queue()
declare i8 addrspace(1)* @__builtin_IB_get_event_pool()
declare i32 @__builtin_IB_get_max_workgroup_size()
declare i32 @__builtin_IB_get_parent_event()
declare i32 @__builtin_IB_get_prefered_workgroup_multiple()

!igc.functions = !{!0, !5, !9, !13, !17}

!0 = !{void (i8 addrspace(1)*)* @test_device_queue, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

!5 = !{void (i8 addrspace(1)*)* @test_device_eventpool, !6}
!6 = !{!2}

!9 = !{void (i32)* @test_device_maxworkgroup, !10}
!10 = !{!2}

!13 = !{void (i32)* @test_device_parentevent, !14}
!14 = !{!2}

!17 = !{void (i32)* @test_device_wg_multiple, !18}
!18 = !{!2}
!21 = !{!"argId", i32 37}
!22 = !{!"implicitArgInfoListVec[0]", !21}
!23 = !{!"implicitArgInfoList", !22}
!24 = !{!"argId", i32 38}
!25 = !{!"implicitArgInfoListVec[0]", !24}
!26 = !{!"implicitArgInfoList", !25}
!27 = !{!"argId", i32 39}
!28 = !{!"implicitArgInfoListVec[0]", !27}
!29 = !{!"implicitArgInfoList", !28}
!30 = !{!"argId", i32 40}
!31 = !{!"implicitArgInfoListVec[0]", !30}
!32 = !{!"implicitArgInfoList", !31}
!33 = !{!"argId", i32 41}
!34 = !{!"implicitArgInfoListVec[0]", !33}
!35 = !{!"implicitArgInfoList", !34}
!36 = !{!"FuncMDMap[0]", void (i8 addrspace(1)*)* @test_device_queue}
!37 = !{!"FuncMDValue[0]", !23}
!38 = !{!"FuncMDMap[1]", void (i8 addrspace(1)*)* @test_device_eventpool}
!39 = !{!"FuncMDValue[1]", !26}
!40 = !{!"FuncMDMap[2]", void (i32)* @test_device_maxworkgroup}
!41 = !{!"FuncMDValue[2]", !29}
!42 = !{!"FuncMDMap[3]", void (i32)* @test_device_parentevent}
!43 = !{!"FuncMDValue[3]", !32}
!44 = !{!"FuncMDMap[4]", void (i32)* @test_device_wg_multiple}
!45 = !{!"FuncMDValue[4]", !35}
!46 = !{!"FuncMD", !36, !37, !38, !39, !40, !41, !42, !43, !44, !45}
!47 = !{!"ModuleMD", !46}
!IGCMetadata = !{!47}
