; UNSUPPORTED: system-windows
; REQUIRES: llvm-spirv, regkeys, dg2-supported, llvm-14-plus

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=1 %s -o %t.bc
; RUN: llvm-spirv %t.bc -opaque-pointers=1 --spirv-ext=+SPV_INTEL_function_pointers,+SPV_INTEL_masked_gather_scatter -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'EnableOpaquePointersBackend=1,ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

; LLVM with typed pointers/default pointer typing:
; RUN: llvm-as -opaque-pointers=0 %s -o %t.bc
; RUN: llvm-spirv %t.bc -opaque-pointers=0 --spirv-ext=+SPV_INTEL_function_pointers,+SPV_INTEL_masked_gather_scatter -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir-unknown-unknown"

; Function Attrs: noinline norecurse nounwind readnone
define internal i32 @_Z2f1u2CMvb32_j(i32 %x) {

entry:
  ret i32 %x
}
; Function Attrs: noinline norecurse nounwind readnone
define internal i32 @_Z2f2u2CMvb32_j(i32 %x) {
entry:
  ret i32 %x
}

; Function Attrs: noinline nounwind
define dllexport void @vadd() {
entry:
; CHECK-LLVM: [[FUNCS:%.*]] = alloca <2 x {{i32 \(i32\)\*|ptr}}>, align 16, !spirv.Decorations [[MOD1:![0-9]+]]
; CHECK-LLVM: store <2 x {{i32 \(i32\)\*|ptr}}> <{{i32 \(i32\)\*|ptr}} @_Z2f1u2CMvb32_j, {{i32 \(i32\)\*|ptr}} @_Z2f2u2CMvb32_j>, {{<2 x i32 \(i32\)\*>\*|ptr}} [[FUNCS]], align 16
  %Funcs = alloca <2 x i32 (i32)*>, align 16
  %0 = insertelement <2 x i32 (i32)*> undef, i32 (i32)* @_Z2f1u2CMvb32_j, i32 0
  %1 = insertelement <2 x i32 (i32)*> %0, i32 (i32)* @_Z2f2u2CMvb32_j, i32 1
  store <2 x i32 (i32)*> %1, <2 x i32 (i32)*>* %Funcs, align 16
  ret void
}

; CHECK-LLVM: [[MOD1]] = !{[[MOD2:![0-9]+]]}
; CHECK-LLVM: [[MOD2]] = !{i32 44, i32 16}