; UNSUPPORTED: system-windows
; REQUIRES: llvm-spirv, regkeys, dg2-supported, llvm-14-plus

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=0 %s -o %t.bc
; TODO: Currently llvm-spirv fails with this test when run with -opaque-pointers=1. Change once fixed.
; RUN: llvm-spirv %t.bc -opaque-pointers=0 --spirv-ext=+SPV_INTEL_usm_storage_classes -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'EnableOpaquePointersBackend=1,ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

; RUN: llvm-spirv %t.bc -opaque-pointers=0 -o %t.no_usm.spv
; RUN: ocloc compile -spirv_input -file %t.no_usm.spv -device dg2 -options " -igc_opts 'EnableOpaquePointersBackend=1,ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM-NO-USM

; LLVM with typed pointers/default pointer typing:
; RUN: llvm-as -opaque-pointers=0 %s -o %t.bc
; RUN: llvm-spirv %t.bc -opaque-pointers=0 --spirv-ext=+SPV_INTEL_usm_storage_classes -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

; RUN: llvm-spirv %t.bc -opaque-pointers=0 -o %t.no_usm.spv
; RUN: ocloc compile -spirv_input -file %t.no_usm.spv -device dg2 -options " -igc_opts 'ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM-NO-USM

; ModuleID = 'intel_usm_addrspaces.cpp'
source_filename = "intel_usm_addrspaces.cpp"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spir64-unknown-unknown"

%"class._ZTSZ4mainE3$_0.anon" = type { i8 }

; Function Attrs: norecurse nounwind
define spir_kernel void @_ZTSZ4mainE11fake_kernel() #0 !kernel_arg_addr_space !4 !kernel_arg_access_qual !4 !kernel_arg_type !4 !kernel_arg_base_type !4 !kernel_arg_type_qual !4 {
entry:
  %0 = alloca %"class._ZTSZ4mainE3$_0.anon", align 1
  %1 = bitcast %"class._ZTSZ4mainE3$_0.anon"* %0 to i8*
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %1) #4
  %2 = addrspacecast %"class._ZTSZ4mainE3$_0.anon"* %0 to %"class._ZTSZ4mainE3$_0.anon" addrspace(4)*
  call spir_func void @"_ZZ4mainENK3$_0clEv"(%"class._ZTSZ4mainE3$_0.anon" addrspace(4)* %2)
  %3 = bitcast %"class._ZTSZ4mainE3$_0.anon"* %0 to i8*
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %3) #4
  ret void
}

; Function Attrs: argmemonly nounwind willreturn
declare void @llvm.lifetime.start.p0i8(i64 immarg, i8* nocapture) #1

; Function Attrs: inlinehint norecurse nounwind
define internal spir_func void @"_ZZ4mainENK3$_0clEv"(%"class._ZTSZ4mainE3$_0.anon" addrspace(4)* %this) #2 align 2 {
entry:
  %this.addr = alloca %"class._ZTSZ4mainE3$_0.anon" addrspace(4)*, align 8
  store %"class._ZTSZ4mainE3$_0.anon" addrspace(4)* %this, %"class._ZTSZ4mainE3$_0.anon" addrspace(4)** %this.addr, align 8, !tbaa !5
  call spir_func void @_Z6usagesv()
  ret void
}

; Function Attrs: argmemonly nounwind willreturn
declare void @llvm.lifetime.end.p0i8(i64 immarg, i8* nocapture) #1

; Function Attrs: norecurse nounwind
define spir_func void @_Z6usagesv() #3 {
entry:
; CHECK-LLVM: %DEVICE = alloca {{.*}} addrspace(5){{.*}}, align 8
; CHECK-LLVM-NO-USM: %DEVICE = alloca {{.*}} addrspace(1){{.*}}, align 8
  %DEVICE = alloca i32 addrspace(5)*, align 8

; CHECK-LLVM: %HOST = alloca {{.*}} addrspace(6){{.*}}, align 8
; CHECK-LLVM-NO-USM: %HOST = alloca {{.*}} addrspace(1){{.*}}, align 8
  %HOST = alloca i32 addrspace(6)*, align 8

  %0 = bitcast i32 addrspace(5)** %DEVICE to i8*
  call void @llvm.lifetime.start.p0i8(i64 8, i8* %0) #4
  %1 = bitcast i32 addrspace(6)** %HOST to i8*
  call void @llvm.lifetime.start.p0i8(i64 8, i8* %1) #4

; CHECK-LLVM: %[[DLOAD_E:[0-9]+]] = load {{.*}}addrspace(5){{.*}}, {{.*}}{{addrspace\(5\)|ptr}}{{.*}} %DEVICE, align 8
; CHECK-LLVM-NO-USM: %[[DLOAD_NE:[0-9]+]] = load {{.*}}addrspace(1){{.*}}, {{.*}}{{addrspace\(1\)|ptr}}{{.*}} %DEVICE, align 8
  %2 = load i32 addrspace(5)*, i32 addrspace(5)** %DEVICE, align 8, !tbaa !5

; CHECK-LLVM: addrspacecast {{.*}} addrspace(5){{.*}} %[[DLOAD_E]] to {{.*}} addrspace(4){{.*}}
; CHECK-LLVM-NO-USM: addrspacecast {{.*}} addrspace(1){{.*}} %[[DLOAD_NE]] to {{.*}} addrspace(4){{.*}}
  %3 = addrspacecast i32 addrspace(5)* %2 to i32 addrspace(4)*
  call spir_func void @_Z3fooPi(i32 addrspace(4)* %3)

; CHECK-LLVM: %[[HLOAD_E:[0-9]+]] = load {{.*}} addrspace(6){{.*}}, {{.*}}{{addrspace\(6\)|ptr}}{{.*}} %HOST, align 8
; CHECK-LLVM-NO-USM: %[[HLOAD_NE:[0-9]+]] = load {{.*}}addrspace(1){{.*}}, {{.*}}{{addrspace\(1\)|ptr}}{{.*}} %HOST, align 8
  %4 = load i32 addrspace(6)*, i32 addrspace(6)** %HOST, align 8, !tbaa !5

; CHECK-LLVM: addrspacecast {{.*}} addrspace(6){{.*}} %[[HLOAD_E]] to {{.*}} addrspace(4){{.*}}
; CHECK-LLVM-NO-USM: addrspacecast {{.*}} addrspace(1){{.*}} %[[HLOAD_NE]] to {{.*}} addrspace(4){{.*}}
  %5 = addrspacecast i32 addrspace(6)* %4 to i32 addrspace(4)*
  call spir_func void @_Z3fooPi(i32 addrspace(4)* %5)
  %6 = bitcast i32 addrspace(6)** %HOST to i8*
  call void @llvm.lifetime.end.p0i8(i64 8, i8* %6) #4
  %7 = bitcast i32 addrspace(5)** %DEVICE to i8*
  call void @llvm.lifetime.end.p0i8(i64 8, i8* %7) #4

  ret void
}

; Function Attrs: norecurse nounwind
define spir_func void @_Z3fooPi(i32 addrspace(4)* %Data) #3 {
entry:
  %Data.addr = alloca i32 addrspace(4)*, align 8
  store i32 addrspace(4)* %Data, i32 addrspace(4)** %Data.addr, align 8, !tbaa !5
  ret void
}

; Function Attrs: norecurse nounwind
define spir_func void @_Z3booPii(i32 addrspace(1)* %arg_glob, i32 addrspace(5)* %arg_dev) #3 !kernel_arg_addr_space !9 {
entry:
  %arg_glob.addr = alloca i32 addrspace(1)*, align 4
  %arg_dev.addr = alloca i32 addrspace(5)*, align 4
  store i32 addrspace(1)* %arg_glob, i32 addrspace(1)** %arg_glob.addr, align 4
  store i32 addrspace(5)* %arg_dev, i32 addrspace(5)** %arg_dev.addr, align 4
  %0 = load i32 addrspace(1)*, i32 addrspace(1)** %arg_glob.addr, align 4
; CHECK-LLVM: addrspacecast {{.*}} addrspace(1){{.*}} %{{[0-9]+}} to {{.*}} addrspace(5){{.*}}
; CHECK-LLVM-NO-USM-NOT: addrspacecast {{.*}} addrspace(1){{.*}} %{{[0-9]+}} to {{.*}} addrspace(5){{.*}}
  %1 = addrspacecast i32 addrspace(1)* %0 to i32 addrspace(5)*
  store i32 addrspace(5)* %1, i32 addrspace(5)** %arg_dev.addr, align 4
  ret void
}

; Function Attrs: norecurse nounwind
define spir_func void @_Z3gooPii(i32 addrspace(1)* %arg_glob, i32 addrspace(6)* %arg_host) #3 !kernel_arg_addr_space !10 {
entry:
  %arg_glob.addr = alloca i32 addrspace(1)*, align 4
  %arg_host.addr = alloca i32 addrspace(6)*, align 4
  store i32 addrspace(1)* %arg_glob, i32 addrspace(1)** %arg_glob.addr, align 4
  store i32 addrspace(6)* %arg_host, i32 addrspace(6)** %arg_host.addr, align 4
  %0 = load i32 addrspace(1)*, i32 addrspace(1)** %arg_glob.addr, align 4
; CHECK-LLVM: addrspacecast {{.*}} addrspace(1){{.*}} %{{[0-9]+}} to {{.*}} addrspace(6){{.*}}
; CHECK-LLVM-NO-USM-NOT: addrspacecast {{.*}} addrspace(1){{.*}} %{{[0-9]+}} to {{.*}} addrspace(6){{.*}}
  %1 = addrspacecast i32 addrspace(1)* %0 to i32 addrspace(6)*
  store i32 addrspace(6)* %1, i32 addrspace(6)** %arg_host.addr, align 4
  ret void
}

; Function Attrs: norecurse nounwind
define spir_func void @_Z3zooPii(i32 addrspace(1)* %arg_glob, i32 addrspace(5)* %arg_dev) #3 !kernel_arg_addr_space !9 {
entry:
  %arg_glob.addr = alloca i32 addrspace(1)*, align 4
  %arg_dev.addr = alloca i32 addrspace(5)*, align 4
  store i32 addrspace(1)* %arg_glob, i32 addrspace(1)** %arg_glob.addr, align 4
  store i32 addrspace(5)* %arg_dev, i32 addrspace(5)** %arg_dev.addr, align 4
  %0 = load i32 addrspace(5)*, i32 addrspace(5)** %arg_dev.addr, align 4
; CHECK-LLVM: addrspacecast {{.*}} addrspace(5){{.*}} %{{[0-9]+}} to {{.*}} addrspace(1){{.*}}
; CHECK-LLVM-NO-USM-NOT: addrspacecast {{.*}} addrspace(5){{.*}} %{{[0-9]+}} to {{.*}} addrspace(1){{.*}}
  %1 = addrspacecast i32 addrspace(5)* %0 to i32 addrspace(1)*
  store i32 addrspace(1)* %1, i32 addrspace(1)** %arg_glob.addr, align 4
  ret void
}

; Function Attrs: norecurse nounwind
define spir_func void @_Z3mooPii(i32 addrspace(1)* %arg_glob, i32 addrspace(6)* %arg_host) #3 !kernel_arg_addr_space !10 {
entry:
  %arg_glob.addr = alloca i32 addrspace(1)*, align 4
  %arg_host.addr = alloca i32 addrspace(6)*, align 4
  store i32 addrspace(1)* %arg_glob, i32 addrspace(1)** %arg_glob.addr, align 4
  store i32 addrspace(6)* %arg_host, i32 addrspace(6)** %arg_host.addr, align 4
  %0 = load i32 addrspace(6)*, i32 addrspace(6)** %arg_host.addr, align 4
; CHECK-LLVM: addrspacecast {{.*}} addrspace(6){{.*}} %{{[0-9]+}} to {{.*}} addrspace(1){{.*}}
; CHECK-LLVM-NO-USM-NOT: addrspacecast {{.*}} addrspace(6){{.*}} %{{[0-9]+}} to {{.*}} addrspace(1){{.*}}
  %1 = addrspacecast i32 addrspace(6)* %0 to i32 addrspace(1)*
  store i32 addrspace(1)* %1, i32 addrspace(1)** %arg_glob.addr, align 4
  ret void
}

attributes #0 = { norecurse nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "sycl-module-id"="intel_usm_addrspaces.cpp" "uniform-work-group-size"="true" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { argmemonly nounwind willreturn }
attributes #2 = { inlinehint norecurse nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { norecurse nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nounwind }

!llvm.module.flags = !{!0}
!opencl.spir.version = !{!1}
!spirv.Source = !{!2}
!llvm.ident = !{!3}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, i32 2}
!2 = !{i32 4, i32 100000}
!3 = !{!"clang version 11.0.0"}
!4 = !{}
!5 = !{!6, !6, i64 0}
!6 = !{!"any pointer", !7, i64 0}
!7 = !{!"omnipotent char", !8, i64 0}
!8 = !{!"Simple C++ TBAA"}
!9 = !{i32 1, i32 5}
!10 = !{i32 1, i32 6}
