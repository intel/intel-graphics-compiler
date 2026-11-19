;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, llvm-16-plus
;
; Verify that the predicated loads created by the PHI split keep the original load's !lsc.cache.ctrl and
; !nontemporal metadata, and add none that the original did not have.
;
; RUN: igc_opt --opaque-pointers --ocl --platformdg2 --igc-split-phis-of-alloca-pointers \
; RUN:   --regkey EnablePHIOfAllocaPtrSplit=1 --regkey EnableSOAAcrossCalls=1 -S %s | FileCheck %s

; Plain load in a successor of the merge block (predicated split) with both kinds of metadata.
;
; CHECK-LABEL: define spir_kernel void @both(
; CHECK:     use:
; CHECK:       call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr %e.remat, i64 4, i1 %{{.*}}, float poison), !nontemporal [[$NT:![0-9]+]], !lsc.cache.ctrl [[$CC5:![0-9]+]]{{$}}
; CHECK:       call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr %ng.remat, i64 4, i1 %{{.*}}, float poison), !nontemporal [[$NT]], !lsc.cache.ctrl [[$CC5]]{{$}}
define spir_kernel void @both(ptr addrspace(1) %out, ptr addrspace(1) %node, i32 %i, i1 %c, i1 %c2) {
entry:
  %a = alloca [64 x float], align 4
  br i1 %c, label %then, label %else
then:
  %e = getelementptr inbounds float, ptr %a, i32 %i
  br label %merge
else:
  %ng = addrspacecast ptr addrspace(1) %node to ptr
  br label %merge
merge:
  %p = phi ptr [ %e, %then ], [ %ng, %else ]
  br i1 %c2, label %use, label %done
use:
  %v = load float, ptr %p, align 4, !lsc.cache.ctrl !10, !nontemporal !11
  store float %v, ptr addrspace(1) %out, align 4
  br label %done
done:
  ret void
}

; Only !lsc.cache.ctrl: no !nontemporal is added.
;
; CHECK-LABEL: define spir_kernel void @cache_ctrl_only(
; CHECK:     use:
; CHECK:       call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr %e.remat, i64 4, i1 %{{.*}}, float poison), !lsc.cache.ctrl [[$CC3:![0-9]+]]{{$}}
; CHECK:       call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr %ng.remat, i64 4, i1 %{{.*}}, float poison), !lsc.cache.ctrl [[$CC3]]{{$}}
define spir_kernel void @cache_ctrl_only(ptr addrspace(1) %out, ptr addrspace(1) %node, i32 %i, i1 %c, i1 %c2) {
entry:
  %a = alloca [64 x float], align 4
  br i1 %c, label %then, label %else
then:
  %e = getelementptr inbounds float, ptr %a, i32 %i
  br label %merge
else:
  %ng = addrspacecast ptr addrspace(1) %node to ptr
  br label %merge
merge:
  %p = phi ptr [ %e, %then ], [ %ng, %else ]
  br i1 %c2, label %use, label %done
use:
  %v = load float, ptr %p, align 4, !lsc.cache.ctrl !12
  store float %v, ptr addrspace(1) %out, align 4
  br label %done
done:
  ret void
}

; Only !nontemporal: no !lsc.cache.ctrl is added.
;
; CHECK-LABEL: define spir_kernel void @nontemporal_only(
; CHECK:     use:
; CHECK:       call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr %e.remat, i64 4, i1 %{{.*}}, float poison), !nontemporal [[$NT]]{{$}}
; CHECK:       call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr %ng.remat, i64 4, i1 %{{.*}}, float poison), !nontemporal [[$NT]]{{$}}
define spir_kernel void @nontemporal_only(ptr addrspace(1) %out, ptr addrspace(1) %node, i32 %i, i1 %c, i1 %c2) {
entry:
  %a = alloca [64 x float], align 4
  br i1 %c, label %then, label %else
then:
  %e = getelementptr inbounds float, ptr %a, i32 %i
  br label %merge
else:
  %ng = addrspacecast ptr addrspace(1) %node to ptr
  br label %merge
merge:
  %p = phi ptr [ %e, %then ], [ %ng, %else ]
  br i1 %c2, label %use, label %done
use:
  %v = load float, ptr %p, align 4, !nontemporal !11
  store float %v, ptr addrspace(1) %out, align 4
  br label %done
done:
  ret void
}

; An existing predicated load split by the predicated path keeps its metadata (and its merge value).
;
; CHECK-LABEL: define spir_kernel void @predicated_load_predicated_split(
; CHECK:     use:
; CHECK:       call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr %e.remat, i64 4, i1 %{{.*}}, float 1.000000e+00), !nontemporal [[$NT]], !lsc.cache.ctrl [[$CC5]]{{$}}
; CHECK:       call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr %ng.remat, i64 4, i1 %{{.*}}, float 1.000000e+00), !nontemporal [[$NT]], !lsc.cache.ctrl [[$CC5]]{{$}}
define spir_kernel void @predicated_load_predicated_split(ptr addrspace(1) %out, ptr addrspace(1) %node, i32 %i, i1 %c, i1 %c2, i1 %pr) {
entry:
  %a = alloca [64 x float], align 4
  br i1 %c, label %then, label %else
then:
  %e = getelementptr inbounds float, ptr %a, i32 %i
  br label %merge
else:
  %ng = addrspacecast ptr addrspace(1) %node to ptr
  br label %merge
merge:
  %p = phi ptr [ %e, %then ], [ %ng, %else ]
  br i1 %c2, label %use, label %done
use:
  %v = call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr %p, i64 4, i1 %pr, float 1.0), !lsc.cache.ctrl !10, !nontemporal !11
  store float %v, ptr addrspace(1) %out, align 4
  br label %done
done:
  ret void
}

; An existing predicated load in the merge block is rebuilt in each predecessor (non-predicated path) and keeps its
; metadata there too.
;
; CHECK-LABEL: define spir_kernel void @predicated_load_hoisted(
; CHECK:     l:
; CHECK:       call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr %el, i64 4, i1 %pr, float 1.000000e+00), !nontemporal [[$NT]], !lsc.cache.ctrl [[$CC5]]{{$}}
; CHECK:     r:
; CHECK:       call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr %er, i64 4, i1 %pr, float 1.000000e+00), !nontemporal [[$NT]], !lsc.cache.ctrl [[$CC5]]{{$}}
define spir_kernel void @predicated_load_hoisted(ptr addrspace(1) %out, i32 %i, i32 %j, i1 %c, i1 %pr) {
entry:
  %a = alloca [64 x float], align 4
  %b = alloca [64 x float], align 4
  br i1 %c, label %l, label %r
l:
  %el = getelementptr inbounds float, ptr %a, i32 %i
  %gl = addrspacecast ptr %el to ptr addrspace(4)
  br label %merge
r:
  %er = getelementptr inbounds float, ptr %b, i32 %j
  %gr = addrspacecast ptr %er to ptr addrspace(4)
  br label %merge
merge:
  %p = phi ptr addrspace(4) [ %gl, %l ], [ %gr, %r ]
  %v = call float @llvm.genx.GenISA.PredicatedLoad.f32.p4.f32(ptr addrspace(4) %p, i64 4, i1 %pr, float 1.0), !lsc.cache.ctrl !10, !nontemporal !11
  store float %v, ptr addrspace(1) %out, align 4
  ret void
}

; CHECK-DAG: [[$NT]] = !{i32 1}
; CHECK-DAG: [[$CC5]] = !{i32 5}
; CHECK-DAG: [[$CC3]] = !{i32 3}

declare float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr, i64, i1, float)
declare float @llvm.genx.GenISA.PredicatedLoad.f32.p4.f32(ptr addrspace(4), i64, i1, float)

!igc.functions = !{!0, !3, !4, !5, !6}
!0 = !{ptr @both, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @cache_ctrl_only, !1}
!4 = !{ptr @nontemporal_only, !1}
!5 = !{ptr @predicated_load_predicated_split, !1}
!6 = !{ptr @predicated_load_hoisted, !1}
!10 = !{i32 5}
!11 = !{i32 1}
!12 = !{i32 3}
