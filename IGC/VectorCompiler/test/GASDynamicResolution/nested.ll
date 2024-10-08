;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXGASDynamicResolution -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXGASDynamicResolution -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-p3:32:32-i64:64-n8:16:32:64"

define spir_func float @nested(float addrspace(4)* addrspace(4)* %gen_ptr_ptr) {
entry:
  %gen_ptr = load float addrspace(4)*, float addrspace(4)* addrspace(4)* %gen_ptr_ptr, align 8
  ; CHECK-TYPED-PTRS: %[[P4P4I64:.*]] = ptrtoint float addrspace(4)* addrspace(4)* %gen_ptr_ptr to i64
  ; CHECK-OPAQUE-PTRS: %[[P4P4I64:.*]] = ptrtoint ptr addrspace(4) %gen_ptr_ptr to i64
  ; CHECK: %[[P4P4V2I32:.*]] = bitcast i64 %[[P4P4I64:.*]] to <2 x i32>
  ; CHECK: %[[TAG_1:.*]] = extractelement <2 x i32> %[[P4P4V2I32:.*]], i64 1
  ; CHECK: %isLocalTag = icmp eq i32 %[[TAG_1:.*]], 1073741824
  ; CHECK: br i1 %isLocalTag, label %LocalBlock, label %GlobalBlock

  ; CHECK: LocalBlock:
  ; CHECK-TYPED-PTRS: %[[LOCAL_PTR_PTR:.*]] = addrspacecast float addrspace(4)* addrspace(4)* %gen_ptr_ptr to float addrspace(4)* addrspace(3)*
  ; CHECK-TYPED-PTRS: %localLoad = load float addrspace(4)*, float addrspace(4)* addrspace(3)* %[[LOCAL_PTR_PTR:.*]], align 8
  ; CHECK-OPAQUE-PTRS: %[[LOCAL_PTR_PTR:.*]] = addrspacecast ptr addrspace(4) %gen_ptr_ptr to ptr addrspace(3)
  ; CHECK-OPAQUE-PTRS: %localLoad = load ptr addrspace(4), ptr addrspace(3) %[[LOCAL_PTR_PTR:.*]], align 8

  ; CHECK: GlobalBlock:
  ; CHECK-TYPED-PTRS: %[[GLOBAL_PTR_PTR:.*]] = addrspacecast float addrspace(4)* addrspace(4)* %gen_ptr_ptr to float addrspace(4)* addrspace(1)*
  ; CHECK-TYPED-PTRS: %globalLoad = load float addrspace(4)*, float addrspace(4)* addrspace(1)* %[[GLOBAL_PTR_PTR:.*]], align 8
  ; CHECK-OPAQUE-PTRS: %[[GLOBAL_PTR_PTR:.*]] = addrspacecast ptr addrspace(4) %gen_ptr_ptr to ptr addrspace(1)
  ; CHECK-OPAQUE-PTRS: %globalLoad = load ptr addrspace(4), ptr addrspace(1) %[[GLOBAL_PTR_PTR:.*]], align 8

  ; CHECK-TYPED-PTRS: %gen_ptr = phi float addrspace(4)* [ %localLoad, %LocalBlock ], [ %globalLoad, %GlobalBlock ]
  ; CHECK-OPAQUE-PTRS: %gen_ptr = phi ptr addrspace(4) [ %localLoad, %LocalBlock ], [ %globalLoad, %GlobalBlock ]

  %v = load float, float addrspace(4)* %gen_ptr, align 4
  ; CHECK-TYPED-PTRS: %[[P4I64:.*]] = ptrtoint float addrspace(4)* %gen_ptr to i64
  ; CHECK-OPAQUE-PTRS: %[[P4I64:.*]] = ptrtoint ptr addrspace(4) %gen_ptr to i64
  ; CHECK: %[[P4V2I32:.*]] = bitcast i64 %[[P4I64:.*]] to <2 x i32>
  ; CHECK: %[[TAG_2:.*]] = extractelement <2 x i32> %[[P4V2I32:.*]], i64 1
  ; CHECK: %[[IS_LOCAL_TAG:.*]] = icmp eq i32 %[[TAG_2:.*]], 1073741824
  ; CHECK: br i1 %[[IS_LOCAL_TAG:.*]], label %[[LOCAL:.*]], label %[[GLOBAL:.*]]

  ; CHECK-TYPED-PTRS: %[[LOCAL_PTR:.*]] = addrspacecast float addrspace(4)* %gen_ptr to float addrspace(3)*
  ; CHECK-TYPED-PTRS: %[[LOCAL_LOAD:.*]] = load float, float addrspace(3)* %[[LOCAL_PTR:.*]], align 4
  ; CHECK-OPAQUE-PTRS: %[[LOCAL_PTR:.*]] = addrspacecast ptr addrspace(4) %gen_ptr to ptr addrspace(3)
  ; CHECK-OPAQUE-PTRS: %[[LOCAL_LOAD:.*]] = load float, ptr addrspace(3) %[[LOCAL_PTR:.*]], align 4

  ; CHECK-TYPED-PTRS: %[[GLOBAL_PTR:.*]] = addrspacecast float addrspace(4)* %gen_ptr to float addrspace(1)*
  ; CHECK-TYPED-PTRS: %[[GLOBAL_LOAD:.*]] = load float, float addrspace(1)* %[[GLOBAL_PTR:.*]], align 4
  ; CHECK-OPAQUE-PTRS: %[[GLOBAL_PTR:.*]] = addrspacecast ptr addrspace(4) %gen_ptr to ptr addrspace(1)
  ; CHECK-OPAQUE-PTRS: %[[GLOBAL_LOAD:.*]] = load float, ptr addrspace(1) %[[GLOBAL_PTR:.*]], align 4

  ; CHECK: %v = phi float [ %[[LOCAL_LOAD:.*]], %[[LOCAL:.*]] ], [ %[[GLOBAL_LOAD:.*]], %[[GLOBAL:.*]] ]
  ret float %v
  ; CHECK ret float %v
}
