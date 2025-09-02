; UNSUPPORTED: system-windows
; REQUIRES: llvm-spirv, regkeys, dg2-supported, llvm-15-or-older

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_masked_gather_scatter -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

; CHECK-LLVM: alloca <4 x i8 addrspace(4)*>
; CHECK-LLVM-NEXT: alloca <4 x i8 addrspace(4)*>
; CHECK-LLVM-NEXT: load <4 x i8 addrspace(4)*>, <4 x i8 addrspace(4)*>*
; CHECK-LLVM-NEXT: store <4 x i8 addrspace(4)*> %[[#]], <4 x i8 addrspace(4)*>*
; CHECK-LLVM-NEXT: bitcast <4 x i8 addrspace(4)*> %[[#]] to <4 x i32 addrspace(4)*>
; CHECK-LLVM-NEXT: addrspacecast <4 x i32 addrspace(4)*> %{{.*}} to <4 x i32 addrspace(1)*>
; CHECK-LLVM-NEXT: call spir_func <4 x i32 addrspace(1)*> @boo(<4 x i32 addrspace(1)*>
; CHECK-LLVM-NEXT: getelementptr inbounds i32, <4 x i32 addrspace(1)*> %{{.*}}, i32 1

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir"

; Function Attrs: nounwind readnone
define spir_kernel void @foo() {
entry:
  %arg1 = alloca <4 x i8 addrspace(4)*>
  %arg2 = alloca <4 x i8 addrspace(4)*>
  %0 = load <4 x i8 addrspace(4)*>, <4 x i8 addrspace(4)*>* %arg1
  store <4 x i8 addrspace(4)*> %0, <4 x i8 addrspace(4)*>* %arg2
  %tmp1 = bitcast <4 x i8 addrspace(4)*> %0 to <4 x i32 addrspace(4)*>
  %tmp2 = addrspacecast <4 x i32 addrspace(4)*> %tmp1 to  <4 x i32 addrspace(1)*>
  %tmp3 = call <4 x i32 addrspace(1)*> @boo(<4 x i32 addrspace(1)*> %tmp2)
  %tmp4 = getelementptr inbounds i32, <4 x i32 addrspace(1)*> %tmp3, i32 1
  %tmp5 = addrspacecast <4 x i32 addrspace(4)*> %tmp1 to <4 x i8 addrspace(1)*>
  ret void
}

declare <4 x i32 addrspace(1)*> @boo(<4 x i32 addrspace(1)*> %a)

!llvm.module.flags = !{!0}
!opencl.spir.version = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, i32 2}
