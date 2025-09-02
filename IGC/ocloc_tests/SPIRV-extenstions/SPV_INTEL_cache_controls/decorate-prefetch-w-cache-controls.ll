; UNSUPPORTED: system-windows
; REQUIRES: llvm-spirv, regkeys, dg2-supported, spirv-promote, llvm-14-plus

; LLVM with typed pointers/default pointer typing:
; RUN: llvm-as -opaque-pointers=0 %s -o %t.bc
; TODO: Currently llvm-spirv fails with this test when run with -opaque-pointers=1. Change once fixed.
; RUN: llvm-spirv %t.bc -opaque-pointers=0 --spirv-ext=+SPV_INTEL_cache_controls -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'EnableOpaquePointersBackend=1,ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

; LLVM with typed pointers/default pointer typing:
; RUN: llvm-as -opaque-pointers=0 %s -o %t.bc
; RUN: llvm-spirv %t.bc -opaque-pointers=0 --spirv-ext=+SPV_INTEL_cache_controls -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spir64-unknown-unknown"

$_ZTSZ4mainEUlvE_ = comdat any

; https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_cache_controls.asciidoc
; These strings are:
; {CacheControlLoadINTEL_Token:\22CacheLevel,CacheControl\22}
@.str.1 = private unnamed_addr addrspace(1) constant [16 x i8] c"../prefetch.hpp\00", section "llvm.metadata"
@.str.9 = private unnamed_addr addrspace(1) constant [13 x i8] c"{6442:\220,1\22}\00", section "llvm.metadata"
@.str.10 = private unnamed_addr addrspace(1) constant [13 x i8] c"{6442:\221,1\22}\00", section "llvm.metadata"
@.str.11 = private unnamed_addr addrspace(1) constant [13 x i8] c"{6442:\222,3\22}\00", section "llvm.metadata"

; these CHECK-SPIRV check that prefetch's arg is decorated with the appropriate
; CacheLevel and CacheControl values.

; Check that the appropriate !spirv.Decorations are preserved after reverse
; translation

; CHECK-LLVM: %[[CALL1:.*]] = call spir_func {{i8|ptr}} addrspace(1){{.*}} @_Z41__spirv_GenericCastToPtrExplicit_ToGlobal{{.*}}
; CHECK-LLVM: %[[CALL1P4:.*]] = addrspacecast {{i8|ptr}} addrspace(1){{.*}} %[[CALL1]] to {{i8|ptr}} addrspace(4){{.*}}, !spirv.Decorations ![[MD:.*]]
; CHECK-LLVM: call spir_func void @_Z20__spirv_ocl_prefetch{{.*}}({{i8|ptr}} addrspace(4){{.*}} %[[CALL1P4]], i64 1)
; CHECK-LLVM: %[[CALL2:.*]] = call spir_func {{i8|ptr}} addrspace(1){{.*}} @_Z41__spirv_GenericCastToPtrExplicit_ToGlobal{{.*}}
; CHECK-LLVM: %[[CALL2P4:.*]] = addrspacecast {{i8|ptr}} addrspace(1){{.*}} %[[CALL2]] to {{i8|ptr}} addrspace(4){{.*}}, !spirv.Decorations ![[MD]]
; CHECK-LLVM: call spir_func void @_Z20__spirv_ocl_prefetch{{.*}}({{i8|ptr}} addrspace(4){{.*}} %[[CALL2P4]], i64 1)
; CHECK-LLVM: %[[CALL3:.*]] = call spir_func {{i8|ptr}} addrspace(1){{.*}} @_Z41__spirv_GenericCastToPtrExplicit_ToGlobal{{.*}}
; CHECK-LLVM: %[[CALL3P4:.*]] = addrspacecast {{i8|ptr}} addrspace(1){{.*}} %[[CALL3]] to {{i8|ptr}} addrspace(4){{.*}}, !spirv.Decorations ![[MD]]
; CHECK-LLVM: call spir_func void @_Z20__spirv_ocl_prefetch{{.*}}({{i8|ptr}} addrspace(4){{.*}} %[[CALL3P4]], i64 2)


; Function Attrs: convergent norecurse nounwind
define weak_odr dso_local spir_kernel void @_ZTSZ4mainEUlvE_(i8 addrspace(1)* noundef align 1 %_arg_dataPtr) local_unnamed_addr comdat !srcloc !5 !kernel_arg_buffer_location !6 !sycl_fixed_targets !7 !sycl_kernel_omit_args !8 {
entry:
  %0 = addrspacecast i8 addrspace(1)* %_arg_dataPtr to i8 addrspace(4)*
  %call.i.i.i.i = tail call spir_func noundef i8 addrspace(1)* @_Z41__spirv_GenericCastToPtrExplicit_ToGlobalPvi(i8 addrspace(4)* noundef %0, i32 noundef 5)
  %call.i.i.i.i.p4 = addrspacecast i8 addrspace(1)* %call.i.i.i.i to i8 addrspace(4)*
  %ptr1 = getelementptr inbounds [13 x i8], [13 x i8] addrspace(1)* @.str.9, i64 0, i64 0
  %ptr2 = getelementptr inbounds [16 x i8], [16 x i8] addrspace(1)* @.str.1, i64 0, i64 0
  %ptr1.p4 = addrspacecast i8 addrspace(1)* %ptr1 to i8 addrspace(4)*
  %ptr1.p0 = addrspacecast i8 addrspace(4)* %ptr1.p4 to i8*
  %ptr2.p4 = addrspacecast i8 addrspace(1)* %ptr2 to i8 addrspace(4)*
  %ptr2.p0 = addrspacecast i8 addrspace(4)* %ptr2.p4 to i8*
  %1 = tail call i8 addrspace(4)* @llvm.ptr.annotation.p4.p1(i8 addrspace(4)* %call.i.i.i.i.p4, i8* %ptr1.p0, i8* %ptr2.p0, i32 76)
  tail call spir_func void @_Z20__spirv_ocl_prefetchPU3AS1Kcm(i8 addrspace(4)* noundef %1, i64 noundef 1)
  %arrayidx3.i = getelementptr inbounds i8, i8 addrspace(4)* %0, i64 1
  %call.i.i.i13.i = tail call spir_func noundef i8 addrspace(1)* @_Z41__spirv_GenericCastToPtrExplicit_ToGlobalPvi(i8 addrspace(4)* noundef %arrayidx3.i, i32 noundef 5)
  %call.i.i.i13.i.p4 = addrspacecast i8 addrspace(1)* %call.i.i.i13.i to i8 addrspace(4)*
  %ptr3 = getelementptr inbounds [13 x i8], [13 x i8] addrspace(1)* @.str.10, i64 0, i64 0
  %ptr3.p4 = addrspacecast i8 addrspace(1)* %ptr3 to i8 addrspace(4)*
  %ptr3.p0 = addrspacecast i8 addrspace(4)* %ptr3.p4 to i8*
  %2 = tail call i8 addrspace(4)* @llvm.ptr.annotation.p4.p1(i8 addrspace(4)* %call.i.i.i13.i.p4, i8* %ptr3.p0, i8* %ptr2.p0, i32 80)
  tail call spir_func void @_Z20__spirv_ocl_prefetchPU3AS1Kcm(i8 addrspace(4)* noundef %2, i64 noundef 1)
  %arrayidx7.i = getelementptr inbounds i8, i8 addrspace(4)* %0, i64 2
  %call.i.i.i16.i = tail call spir_func noundef i8 addrspace(1)* @_Z41__spirv_GenericCastToPtrExplicit_ToGlobalPvi(i8 addrspace(4)* noundef %arrayidx7.i, i32 noundef 5)
  %call.i.i.i16.i.p4 = addrspacecast i8 addrspace(1)* %call.i.i.i16.i to i8 addrspace(4)*
  %ptr4 = getelementptr inbounds [13 x i8], [13 x i8] addrspace(1)* @.str.11, i64 0, i64 0
  %ptr4.p4 = addrspacecast i8 addrspace(1)* %ptr4 to i8 addrspace(4)*
  %ptr4.p0 = addrspacecast i8 addrspace(4)* %ptr4.p4 to i8*
  %3 = tail call i8 addrspace(4)* @llvm.ptr.annotation.p4.p1(i8 addrspace(4)* %call.i.i.i16.i.p4, i8* %ptr4.p0, i8* %ptr2.p0, i32 80)
  tail call spir_func void @_Z20__spirv_ocl_prefetchPU3AS1Kcm(i8 addrspace(4)* noundef %3, i64 noundef 2)
  ret void
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: readwrite)
declare i8 addrspace(4)* @llvm.ptr.annotation.p4.p1(i8 addrspace(4)*, i8*, i8*, i32)

; Function Attrs: convergent nounwind
declare dso_local spir_func void @_Z20__spirv_ocl_prefetchPU3AS1Kcm(i8 addrspace(4)* noundef, i64 noundef) local_unnamed_addr

; Function Attrs: convergent mustprogress nofree nounwind willreturn memory(none)
declare dso_local spir_func noundef i8 addrspace(1)* @_Z41__spirv_GenericCastToPtrExplicit_ToGlobalPvi(i8 addrspace(4)* noundef, i32 noundef) local_unnamed_addr

!llvm.module.flags = !{!0, !1}
!opencl.spir.version = !{!2}
!spirv.Source = !{!3}
!llvm.ident = !{!4}

; CHECK-LLVM: ![[MD]] = !{![[MD1:.*]]}
; CHECK-LLVM: ![[MD1]] = !{i32 5635, !""}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"frame-pointer", i32 2}
!2 = !{i32 1, i32 2}
!3 = !{i32 4, i32 100000}
!4 = !{!"clang version 18.0.0"}
!5 = !{i32 1522}
!6 = !{i32 -1}
!7 = !{}
!8 = !{i1 false}
