;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXGASDynamicResolution -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

define spir_func float @nested(float addrspace(4)* addrspace(4)* %gen_ptr_ptr) {
entry:
  %gen_ptr = load float addrspace(4)*, float addrspace(4)* addrspace(4)* %gen_ptr_ptr, align 8
  ; CHECK: %[[P4P4I64:.*]] = ptrtoint float addrspace(4)* addrspace(4)* %gen_ptr_ptr to i64
  ; CHECK: %[[P4P4V2I32:.*]] = bitcast i64 %[[P4P4I64:.*]] to <2 x i32>
  ; CHECK: %[[TAG_1:.*]] = extractelement <2 x i32> %[[P4P4V2I32:.*]], i64 1
  ; CHECK: %isLocalTag = icmp eq i32 %[[TAG_1:.*]], 1073741824
  ; CHECK: br i1 %isLocalTag, label %LocalBlock, label %GlobalBlock

  ; CHECK: LocalBlock:
  ; CHECK:   %[[LOCAL_PTR_PTR:.*]] = addrspacecast float addrspace(4)* addrspace(4)* %gen_ptr_ptr to float addrspace(4)* addrspace(3)*
  ; CHECK:   %localLoad = load float addrspace(4)*, float addrspace(4)* addrspace(3)* %[[LOCAL_PTR_PTR:.*]], align 8

  ; CHECK: GlobalBlock:
  ; CHECK:   %[[GLOBAL_PTR_PTR:.*]] = addrspacecast float addrspace(4)* addrspace(4)* %gen_ptr_ptr to float addrspace(4)* addrspace(1)*
  ; CHECK:   %globalLoad = load float addrspace(4)*, float addrspace(4)* addrspace(1)* %[[GLOBAL_PTR_PTR:.*]], align 8

  ; CHECK: %gen_ptr = phi float addrspace(4)* [ %localLoad, %LocalBlock ], [ %globalLoad, %GlobalBlock ]

  %v = load float, float addrspace(4)* %gen_ptr, align 4
  ; CHECK: %[[P4I64:.*]] = ptrtoint float addrspace(4)* %gen_ptr to i64
  ; CHECK: %[[P4V2I32:.*]] = bitcast i64 %[[P4I64:.*]] to <2 x i32>
  ; CHECK: %[[TAG_2:.*]] = extractelement <2 x i32> %[[P4V2I32:.*]], i64 1
  ; CHECK: %[[IS_LOCAL_TAG:.*]] = icmp eq i32 %[[TAG_2:.*]], 1073741824
  ; CHECK: br i1 %[[IS_LOCAL_TAG:.*]], label %[[LOCAL:.*]], label %[[GLOBAL:.*]]

  ; CHECK:  %[[LOCAL_PTR:.*]] = addrspacecast float addrspace(4)* %gen_ptr to float addrspace(3)*
  ; CHECK:  %[[LOCAL_LOAD:.*]] = load float, float addrspace(3)* %[[LOCAL_PTR:.*]], align 4

  ; CHECK:  %[[GLOBAL_PTR:.*]] = addrspacecast float addrspace(4)* %gen_ptr to float addrspace(1)*
  ; CHECK:  %[[GLOBAL_LOAD:.*]] = load float, float addrspace(1)* %[[GLOBAL_PTR:.*]], align 4

  ; CHECK: %v = phi float [ %[[LOCAL_LOAD:.*]], %[[LOCAL:.*]] ], [ %[[GLOBAL_LOAD:.*]], %[[GLOBAL:.*]] ]
  ret float %v
  ; CHECK ret float %v
}