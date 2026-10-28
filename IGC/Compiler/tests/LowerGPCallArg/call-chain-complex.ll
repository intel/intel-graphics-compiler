;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers %s -S -o - -igc-lower-gp-arg | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers %s -S -o - -igc-lower-gp-arg | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED %}

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"


; CHECK-LABEL: define void @f0
define void @f0(i32 addrspace(1)* %global_ptr, i32 addrspace(3)* %local_ptr) {
  ; CHECK-TYPED-NOT: addrspacecast i32 addrspace(1)* %a0 to i32 addrspace(4)*
  ; CHECK-OPAQUE-NOT: addrspacecast ptr addrspace(1) %a0 to ptr addrspace(4)
  ; CHECK-TYPED-NOT: addrspacecast i32 addrspace(1)* %a1 to i32 addrspace(4)*
  ; CHECK-OPAQUE-NOT: addrspacecast ptr addrspace(1) %a1 to ptr addrspace(4)
  %global_as_generic = addrspacecast i32 addrspace(1)* %global_ptr to i32 addrspace(4)*
  %local_as_generic = addrspacecast i32 addrspace(3)* %local_ptr to i32 addrspace(4)*

  ; CHECK-TYPED: call void @f1(i32 addrspace(1)* %global_ptr, i32 addrspace(3)* %local_ptr)
  ; CHECK-OPAQUE: call void @f1(ptr addrspace(1) %global_ptr, ptr addrspace(3) %local_ptr)
  call void @f1(i32 addrspace(4)* %global_as_generic, i32 addrspace(4)* %local_as_generic)

  ret void
}

; CHECK-TYPED: define void @f1(i32 addrspace(1)* %[[GLOBAL_PTR:.*]], i32 addrspace(3)* %[[LOCAL_PTR:.*]])
; CHECK-OPAQUE: define void @f1(ptr addrspace(1) %[[GLOBAL_PTR:.*]], ptr addrspace(3) %[[LOCAL_PTR:.*]])
define void @f1(i32 addrspace(4)* %global_as_generic, i32 addrspace(4)* %local_as_generic) {
  ; CHECK-TYPED-DAG: %[[GLOBAL_AS_GENERIC:.*]] = addrspacecast i32 addrspace(1)* %[[GLOBAL_PTR]] to i32 addrspace(4)*
  ; CHECK-OPAQUE-DAG: %[[GLOBAL_AS_GENERIC:.*]] = addrspacecast ptr addrspace(1) %[[GLOBAL_PTR]] to ptr addrspace(4)
  ; CHECK-TYPED-DAG: %[[LOCAL_AS_GENERIC:.*]] = addrspacecast i32 addrspace(3)* %[[LOCAL_PTR]] to i32 addrspace(4)*
  ; CHECK-OPAQUE-DAG: %[[LOCAL_AS_GENERIC:.*]] = addrspacecast ptr addrspace(3) %[[LOCAL_PTR]] to ptr addrspace(4)
  %gid = call spir_func i32 @get_global_id()
  %cond = icmp ne i32 %gid, 0
  ; CHECK-TYPED: %[[GLOBAL_OR_LOCAL:.*]] = select i1 %cond, i32 addrspace(4)* %[[GLOBAL_AS_GENERIC]], i32 addrspace(4)* %[[LOCAL_AS_GENERIC]]
  ; CHECK-OPAQUE: %[[GLOBAL_OR_LOCAL:.*]] = select i1 %cond, ptr addrspace(4) %[[GLOBAL_AS_GENERIC]], ptr addrspace(4) %[[LOCAL_AS_GENERIC]]
  %global_or_local = select i1 %cond, i32 addrspace(4)* %global_as_generic, i32 addrspace(4)* %local_as_generic
  ; CHECK-TYPED: call void @f2(i32 addrspace(4)* %[[GLOBAL_OR_LOCAL]], i32 addrspace(3)* %[[LOCAL_PTR]])
  ; CHECK-OPAQUE: call void @f2(ptr addrspace(4) %[[GLOBAL_OR_LOCAL]], ptr addrspace(3) %[[LOCAL_PTR]])
  call void @f2(i32 addrspace(4)* %global_or_local, i32 addrspace(4)* %local_as_generic)

  ret void
}

; CHECK-TYPED: define void @f2(i32 addrspace(4)* %[[GLOBAL_OR_LOCAL:.*]], i32 addrspace(3)* %[[LOCAL_PTR:.*]])
; CHECK-OPAQUE: define void @f2(ptr addrspace(4) %[[GLOBAL_OR_LOCAL:.*]], ptr addrspace(3) %[[LOCAL_PTR:.*]])
define void @f2(i32 addrspace(4)* %global_or_local, i32 addrspace(4)* %local_as_generic) {
  %private_alloca = alloca i32
  ; CHECK-TYPED: store i32 0, i32 addrspace(4)* %[[GLOBAL_OR_LOCAL]], align 4
  ; CHECK-OPAQUE: store i32 0, ptr addrspace(4) %[[GLOBAL_OR_LOCAL]], align 4
  store i32 0, i32 addrspace(4)* %global_or_local, align 4
  ; CHECK-TYPED: store i32 0, i32 addrspace(3)* %[[LOCAL_PTR]], align 4
  ; CHECK-OPAQUE: store i32 0, ptr addrspace(3) %[[LOCAL_PTR]], align 4
  store i32 0, i32 addrspace(4)* %local_as_generic, align 4
  %gid = call spir_func i32 @get_global_id()
  %cond = icmp ne i32 %gid, 0
  br i1 %cond, label %if, label %elif
if:
  ; CHECK-TYPED: %[[LOCAL_PTR_GEP:.*]] = getelementptr inbounds i32, i32 addrspace(3)* %[[LOCAL_PTR]], i64 4
  ; CHECK-OPAQUE: %[[LOCAL_PTR_GEP:.*]] = getelementptr inbounds i32, ptr addrspace(3) %[[LOCAL_PTR]], i64 4
  %local_as_generic_gep = getelementptr inbounds i32, i32 addrspace(4)* %local_as_generic, i64 4
  br label %exit
elif:
  br label %exit
exit:
  ; CHECK-TYPED: %local_or_null = phi i32 addrspace(3)* [ %[[LOCAL_PTR_GEP]], %if ], [ null, %elif ]
  ; CHECK-OPAQUE: %local_or_null = phi ptr addrspace(3) [ %[[LOCAL_PTR_GEP]], %if ], [ null, %elif ]
  %local_or_null = phi i32 addrspace(4)* [ %local_as_generic_gep, %if ], [ null, %elif ]
  ; CHECK-TYPED-NOT: addrspacecast i32* %private_alloca to i32 addrspace(4)*
  ; CHECK-OPAQUE-NOT: addrspacecast ptr %private_alloca to ptr addrspace(4)
  %private_as_generic = addrspacecast i32* %private_alloca to i32 addrspace(4)*
  ; CHECK-TYPED: call void @f3(i32 addrspace(3)* %local_or_null, i32* %private_alloca)
  ; CHECK-OPAQUE: call void @f3(ptr addrspace(3) %local_or_null, ptr %private_alloca)
  call void @f3(i32 addrspace(4)* %local_or_null, i32 addrspace(4)* %private_as_generic)

  ret void
}

; CHECK-TYPED: define void @f3(i32 addrspace(3)* %[[LOCAL_PTR:.*]], i32* %[[PRIVATE_PTR:.*]])
; CHECK-OPAQUE: define void @f3(ptr addrspace(3) %[[LOCAL_PTR:.*]], ptr %[[PRIVATE_PTR:.*]])
define void @f3(i32 addrspace(4)* %local_as_generic, i32 addrspace(4)* %private_as_generic) {
  ; CHECK-TYPED: store i32 0, i32 addrspace(3)* %[[LOCAL_PTR]], align 4
  ; CHECK-OPAQUE: store i32 0, ptr addrspace(3) %[[LOCAL_PTR]], align 4
  store i32 0, i32 addrspace(4)* %local_as_generic, align 4
  ; CHECK-TYPED: store i32 0, i32* %[[PRIVATE_PTR]], align 4
  ; CHECK-OPAQUE: store i32 0, ptr %[[PRIVATE_PTR]], align 4
  store i32 0, i32 addrspace(4)* %private_as_generic, align 4

  ret void
}

declare spir_func i32 @get_global_id()

!igc.functions = !{!0, !3, !4, !5}

; CHECK-TYPED: !{void (i32 addrspace(1)*, i32 addrspace(3)*)* @f0, !1}
; CHECK-OPAQUE: !{ptr @f0, !1}
; CHECK-TYPED: !{void (i32 addrspace(1)*, i32 addrspace(3)*)* @f1, !1}
; CHECK-OPAQUE: !{ptr @f1, !1}
; CHECK-TYPED: !{void (i32 addrspace(4)*, i32 addrspace(3)*)* @f2, !1}
; CHECK-OPAQUE: !{ptr @f2, !1}
; CHECK-TYPED: !{void (i32 addrspace(3)*, i32*)* @f3, !1}
; CHECK-OPAQUE: !{ptr @f3, !1}
!0 = !{void (i32 addrspace(1)*, i32 addrspace(3)*)* @f0, !1}
!3 = !{void (i32 addrspace(4)*, i32 addrspace(4)*)* @f1, !1}
!4 = !{void (i32 addrspace(4)*, i32 addrspace(4)*)* @f2, !1}
!5 = !{void (i32 addrspace(4)*, i32 addrspace(4)*)* @f3, !1}

!1 = !{!2}
!2 = !{!"function_type", i32 0}
