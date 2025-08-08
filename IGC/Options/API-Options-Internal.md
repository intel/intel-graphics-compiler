<!--
/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/ -->
# IGC API Options list
This is an auto-generated list of options gathered from [InternalOptions.td](https://github.com/intel/intel-graphics-compiler/blob/master/IGC/Options/include/igc/Options/InternalOptions.td) file using [LLVM TableGen](https://llvm.org/docs/TableGen/).

The "Kind" column describes the type of Option as defined in [OptParser.td](https://github.com/llvm-mirror/llvm/blob/master/include/llvm/Option/OptParser.td#L24-L50) refer to it for further information.

## Table of contents
1. [IGCInternalOption](#igcinternaloption)
2. [VCInternalOption](#vcinternaloption)

## Legend
The `cl-` and `ze-` prefixes correspond to OpenCL and Level Zero specific options. For more in-depth information regarding OpenCL options consult the OpenCL documentation[\[1\]](#references).

| Type | Example | Expands to |
|:---- | :---- | :---- |
| `<>` - Required choice | `-<cl-\|ze->option` | `-cl-option`; `-ze-option` |
| `[]`  - Optional choice | `-[cl-]option` | `-cl-option`; `-option` |
## Option Lists
### IGCInternalOption
| Name | Description | Kind |
|:---- | :---- | :----: |
|`-D`| Manually define macros. | `KIND_JOINED_OR_SEPARATE` |
|`-[cl-\|ze-]buffer-bounds-checking`| Enable buffer bounds checking | `KIND_FLAG` |
|`-[cl-\|ze-]buffer-offset-arg-required`| Tell IGC to always use buffer offset. It is valid only if -intel-has-buffer-offset-arg is present. | `KIND_FLAG` |
|`-[cl-\|ze-]compile-one-at-time`| Enables llvm::module splitting to compile only one kernel at a time. | `KIND_FLAG` |
|`-[cl-\|ze-]disable-a64WA`|  | `KIND_FLAG` |
|`-[cl-\|ze-]disable-noMaskWA`|  | `KIND_FLAG` |
|`-[cl-\|ze-]disable-recompilation`|  | `KIND_FLAG` |
|`-[cl-\|ze-]disable-sendwarwa`| Disable SendWAR WA PVC platform only | `KIND_FLAG` |
|`-[cl-\|ze-]disableEUFusion`|  | `KIND_FLAG` |
|`-[cl-\|ze-]emit-zebin-visa-sections`| Add vISA asm as sections in ZeBin | `KIND_FLAG` |
|`-[cl-\|ze-]enable-auto-large-GRF-mode`| Use compiler heuristics to determine number of GRF. | `KIND_FLAG` |
|`-[cl-\|ze-]enable-divergent-barrier-handling`| Enable the divergent barrier pass. | `KIND_FLAG` |
|`-[cl-\|ze-]exclude-ir-from-zebin`| Exclude SPIR-V section from files generated in ZEBIN format. | `KIND_FLAG` |
|`-[cl-\|ze-]exp-register-file-size`| Set amount of registers used by regalloc. | `KIND_SEPARATE` |
|`-cl-ext=`|  | `KIND_JOINED` |
|`-[cl-\|ze-]fail-on-spill`|  | `KIND_FLAG` |
|`-[cl-\|ze-]force-disable-4GB-buffer`|  | `KIND_FLAG` |
|`-[cl-\|ze-]force-emu-int32divrem`| Use emulation fp64-based emulation functions if fp64 is supported natively. | `KIND_FLAG` |
|`-[cl-\|ze-]force-emu-sp-int32divrem`| Force the fp64-based emulation regardless of native support. | `KIND_FLAG` |
|`-[cl-\|ze-]force-enable-a64WA`|  | `KIND_FLAG` |
|`-[cl-\|ze-]force-global-mem-allocation`|  | `KIND_FLAG` |
|`-[cl-\|ze-]fp64-gen-emu`| Enable full FP64 emulation. | `KIND_FLAG` |
|`-[cl-\|ze-]functonControl`|  | `KIND_SEPARATE` |
|`-[cl-\|ze-]greater-than-4GB-buffer-required`| When this flag is present, it indicates that any OpenCL buffers can be more than 4GB in size. If it is absent,  all buffers are not more than 4GB in size. | `KIND_FLAG` |
|`-[cl-\|ze-]has-buffer-offset-arg`| This flag, together with *[-cl-intel\|-ze-opt]-greater-than-4GB-buffer-required* is used to convert stateless memory accesses, called messages or load/store, into stateful ones. The OpenCL runtime can create a surface whose base is either *buffer_base* or *buffer_base + buffer_offset*, based on whether *buffer_offset* is used. | `KIND_FLAG` |
|`-[cl-\|ze-]has-positive-pointer-offset`| For any load and store (aka message) whose address = *ptrArg + offset*, where *ptrArg* is a kernel pointer argument, offset is assumed to be non-negative if this flag is present. | `KIND_FLAG` |
|`-[cl-\|ze-]high-accuracy-nolut-math`| Enbales experimental high accuracy implementations of transcendentals. | `KIND_FLAG` |
|`-[cl-\|ze-]ignoreBFRounding`| Folds BF operands into mul/add/cmp operations. | `KIND_FLAG` |
|`-[cl-\|ze-]include-sip-csr`|  | `KIND_FLAG` |
|`-[cl-\|ze-]include-sip-kernel-debug`|  | `KIND_FLAG` |
|`-[cl-\|ze-]include-sip-kernel-local-debug`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-include-sip-csr`|  | `KIND_FLAG` |
|`-[cl-\|ze-]kernel-debug-enable`|  | `KIND_FLAG` |
|`-[cl-\|ze-]ldstcombine`|  | `KIND_SEPARATE` |
|`-[cl-\|ze-]ldstcombine_max-loadbytes`|  | `KIND_SEPARATE` |
|`-[cl-\|ze-]ldstcombine_max_storebytes`|  | `KIND_SEPARATE` |
|`-[cl-\|ze-]load-cache-default`|  | `KIND_SEPARATE` |
|`-[cl-\|ze-]minimum-valid-address-checking`| Set minimal valid address | `KIND_SEPARATE` |
|`-[cl-\|ze-]no-prera-scheduling`|  | `KIND_FLAG` |
|`-[cl-\|ze-]no-spill`|  | `KIND_FLAG` |
|`-[cl-\|ze-]num-thread-per-eu`| Overrides the current number of threads value defined by the user's command line option for the entire module or the compiler choice by heuristics. | `KIND_SEPARATE` |
|`-<cl-\|ze->private-memory-minimal-siper-thread`| When this flag is present, it guarantees that size of private memory allocated per thread can not be less then the given value. Constraint: <SIZE> >= 0. | `KIND_SEPARATE` |
|`-[cl-\|ze-]replace-global-offsets-by-zero`| OpenCL's global IDs are assumed to start from the origin at global offsets (offset_x, offset_y, offset_z) When this flag is present, it indicates that the global offsets are (0,0,0). | `KIND_FLAG` |
|`-<cl-\|ze->scratch-space-private-memory-minimal-siper-thread`| When this flag is present, it guarantees that size of scratch space private memory allocated per thread can not be less then the given value. Constraint: <SIZE> >= 0. | `KIND_SEPARATE` |
|`-[cl-\|ze-]skip-reloc-add`|  | `KIND_FLAG` |
|`-[cl-\|ze-]store-cache-default`|  | `KIND_SEPARATE` |
|`-[cl-\|ze-]use-32bit-ptr-arith`|  | `KIND_FLAG` |
|`-[cl-\|ze-]use-bindless-advanced-mode`|  | `KIND_FLAG` |
|`-[cl-\|ze-]use-bindless-buffers`| Use bindless mode for buffers | `KIND_FLAG` |
|`-[cl-\|ze-]use-bindless-images`| Use bindless mode for images | `KIND_FLAG` |
|`-[cl-\|ze-]use-bindless-legacy-mode`|  | `KIND_FLAG` |
|`-[cl-\|ze-]use-bindless-mode`|  | `KIND_FLAG` |
|`-[cl-\|ze-]use-bindless-printf`|  | `KIND_FLAG` |
|`-[cl-\|ze-]vector-coalesing`|  | `KIND_SEPARATE` |
|`-emit-visa-only`| Compile until vISA | `KIND_FLAG` |
|`-cl-force-std`| Force a specific OpenCL C version. | `KIND_FLAG` |
|`-m32`|  | `KIND_FLAG` |
|`-m64`|  | `KIND_FLAG` |
|`-cl-oversion=`|  | `KIND_JOINED` |
|`-[cl-\|ze-]128-GRF-per-thread`|  | `KIND_FLAG` |
|`-[cl-\|ze-]256-GRF-per-thread`|  | `KIND_FLAG` |
|`-[cl-\|ze-]exp-register-file-size=`| Alias for -ze-exp-register-file-size. | `KIND_JOINED` |
|`-<cl-\|ze->intel-128-GRF-per-thread`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-256-GRF-per-thread`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-buffer-bounds-checking`| Enable buffer bounds checking | `KIND_FLAG` |
|`-<cl-\|ze->intel-buffer-offset-arg-required`| Tell IGC to always use buffer offset. It is valid only if -intel-has-buffer-offset-arg is present. | `KIND_FLAG` |
|`-<cl-\|ze->intel-compile-one-at-time`| Enables llvm::module splitting to compile only one kernel at a time. | `KIND_FLAG` |
|`-<cl-\|ze->intel-disable-a64WA`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-disable-noMaskWA`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-disable-recompilation`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-disable-sendwarwa`| Disable SendWAR WA PVC platform only | `KIND_FLAG` |
|`-<cl-\|ze->intel-disableEUFusion`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-emit-zebin-visa-sections`| Add vISA asm as sections in ZeBin | `KIND_FLAG` |
|`-<cl-\|ze->intel-enable-auto-large-GRF-mode`| Use compiler heuristics to determine number of GRF. | `KIND_FLAG` |
|`-<cl-\|ze->intel-enable-divergent-barrier-handling`| Enable the divergent barrier pass. | `KIND_FLAG` |
|`-<cl-\|ze->intel-exclude-ir-from-zebin`| Exclude SPIR-V section from files generated in ZEBIN format. | `KIND_FLAG` |
|`-<cl-\|ze->intel-exp-register-file-size`| Set amount of registers used by regalloc. | `KIND_SEPARATE` |
|`-<cl-\|ze->intel-exp-register-file-size=`| Alias for -ze-exp-register-file-size. | `KIND_JOINED` |
|`-<cl-\|ze->intel-fail-on-spill`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-force-disable-4GB-buffer`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-force-emu-int32divrem`| Use emulation fp64-based emulation functions if fp64 is supported natively. | `KIND_FLAG` |
|`-<cl-\|ze->intel-force-emu-sp-int32divrem`| Force the fp64-based emulation regardless of native support. | `KIND_FLAG` |
|`-<cl-\|ze->intel-force-enable-a64WA`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-force-global-mem-allocation`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-fp64-gen-emu`| Enable full FP64 emulation. | `KIND_FLAG` |
|`-<cl-\|ze->intel-functonControl`|  | `KIND_SEPARATE` |
|`-<cl-\|ze->intel-greater-than-4GB-buffer-required`| When this flag is present, it indicates that any OpenCL buffers can be more than 4GB in size. If it is absent,  all buffers are not more than 4GB in size. | `KIND_FLAG` |
|`-<cl-\|ze->intel-has-buffer-offset-arg`| This flag, together with *[-cl-intel\|-ze-opt]-greater-than-4GB-buffer-required* is used to convert stateless memory accesses, called messages or load/store, into stateful ones. The OpenCL runtime can create a surface whose base is either *buffer_base* or *buffer_base + buffer_offset*, based on whether *buffer_offset* is used. | `KIND_FLAG` |
|`-<cl-\|ze->intel-has-positive-pointer-offset`| For any load and store (aka message) whose address = *ptrArg + offset*, where *ptrArg* is a kernel pointer argument, offset is assumed to be non-negative if this flag is present. | `KIND_FLAG` |
|`-<cl-\|ze->intel-high-accuracy-nolut-math`| Enbales experimental high accuracy implementations of transcendentals. | `KIND_FLAG` |
|`-<cl-\|ze->intel-ignoreBFRounding`| Folds BF operands into mul/add/cmp operations. | `KIND_FLAG` |
|`-<cl-\|ze->intel-include-sip-kernel-debug`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-include-sip-kernel-local-debug`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-kernel-debug-enable`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-ldstcombine`|  | `KIND_SEPARATE` |
|`-<cl-\|ze->intel-ldstcombine-max-loadbytes=`|  | `KIND_JOINED` |
|`-<cl-\|ze->intel-ldstcombine-max-storebytes=`|  | `KIND_JOINED` |
|`-<cl-\|ze->intel-ldstcombine=`|  | `KIND_JOINED` |
|`-<cl-\|ze->intel-ldstcombine_max-loadbytes`|  | `KIND_SEPARATE` |
|`-<cl-\|ze->intel-ldstcombine_max_storebytes`|  | `KIND_SEPARATE` |
|`-<cl-\|ze->intel-load-cache-default`|  | `KIND_SEPARATE` |
|`-<cl-\|ze->intel-load-cache-default=`|  | `KIND_JOINED` |
|`-<cl-\|ze->intel-minimum-valid-address-checking`| Set minimal valid address | `KIND_SEPARATE` |
|`-<cl-\|ze->intel-no-prera-scheduling`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-no-spill`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-num-thread-per-eu`| Overrides the current number of threads value defined by the user's command line option for the entire module or the compiler choice by heuristics. | `KIND_SEPARATE` |
|`-<cl-\|ze->intel-private-memory-minimal-siper-thread`| When this flag is present, it guarantees that size of private memory allocated per thread can not be less then the given value. Constraint: <SIZE> >= 0. | `KIND_SEPARATE` |
|`-<cl-\|ze->intel-replace-global-offsets-by-zero`| OpenCL's global IDs are assumed to start from the origin at global offsets (offset_x, offset_y, offset_z) When this flag is present, it indicates that the global offsets are (0,0,0). | `KIND_FLAG` |
|`-<cl-\|ze->intel-scratch-space-private-memory-minimal-siper-thread`| When this flag is present, it guarantees that size of scratch space private memory allocated per thread can not be less then the given value. Constraint: <SIZE> >= 0. | `KIND_SEPARATE` |
|`-<cl-\|ze->intel-skip-reloc-add`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-store-cache-default`|  | `KIND_SEPARATE` |
|`-<cl-\|ze->intel-store-cache-default=`|  | `KIND_JOINED` |
|`-<cl-\|ze->intel-use-32bit-ptr-arith`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-use-bindless-advanced-mode`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-use-bindless-buffers`| Use bindless mode for buffers | `KIND_FLAG` |
|`-<cl-\|ze->intel-use-bindless-images`| Use bindless mode for images | `KIND_FLAG` |
|`-<cl-\|ze->intel-use-bindless-legacy-mode`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-use-bindless-mode`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-use-bindless-printf`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-vector-coalesing`|  | `KIND_SEPARATE` |
|`-<cl-\|ze->intel-vector-coalesing=`|  | `KIND_JOINED` |
|`-[cl-\|ze-]ldstcombine-max-loadbytes=`|  | `KIND_JOINED` |
|`-[cl-\|ze-]ldstcombine-max-storebytes=`|  | `KIND_JOINED` |
|`-[cl-\|ze-]ldstcombine=`|  | `KIND_JOINED` |
|`-[cl-\|ze-]load-cache-default=`|  | `KIND_JOINED` |
|`-ze-opt-128-GRF-per-thread`|  | `KIND_FLAG` |
|`-ze-opt-256-GRF-per-thread`|  | `KIND_FLAG` |
|`-ze-opt-buffer-bounds-checking`| Enable buffer bounds checking | `KIND_FLAG` |
|`-ze-opt-buffer-offset-arg-required`| Tell IGC to always use buffer offset. It is valid only if -intel-has-buffer-offset-arg is present. | `KIND_FLAG` |
|`-ze-opt-compile-one-at-time`| Enables llvm::module splitting to compile only one kernel at a time. | `KIND_FLAG` |
|`-ze-opt-disable-a64WA`|  | `KIND_FLAG` |
|`-ze-opt-disable-noMaskWA`|  | `KIND_FLAG` |
|`-ze-opt-disable-recompilation`|  | `KIND_FLAG` |
|`-ze-opt-disable-sendwarwa`| Disable SendWAR WA PVC platform only | `KIND_FLAG` |
|`-ze-opt-disableEUFusion`|  | `KIND_FLAG` |
|`-ze-opt-emit-zebin-visa-sections`| Add vISA asm as sections in ZeBin | `KIND_FLAG` |
|`-ze-opt-enable-auto-large-GRF-mode`| Use compiler heuristics to determine number of GRF. | `KIND_FLAG` |
|`-ze-opt-enable-divergent-barrier-handling`| Enable the divergent barrier pass. | `KIND_FLAG` |
|`-ze-opt-exclude-ir-from-zebin`| Exclude SPIR-V section from files generated in ZEBIN format. | `KIND_FLAG` |
|`-ze-opt-exp-register-file-size`| Set amount of registers used by regalloc. | `KIND_SEPARATE` |
|`-ze-opt-exp-register-file-size=`| Alias for -ze-exp-register-file-size. | `KIND_JOINED` |
|`-ze-opt-fail-on-spill`|  | `KIND_FLAG` |
|`-ze-opt-force-disable-4GB-buffer`|  | `KIND_FLAG` |
|`-ze-opt-force-emu-int32divrem`| Use emulation fp64-based emulation functions if fp64 is supported natively. | `KIND_FLAG` |
|`-ze-opt-force-emu-sp-int32divrem`| Force the fp64-based emulation regardless of native support. | `KIND_FLAG` |
|`-ze-opt-force-enable-a64WA`|  | `KIND_FLAG` |
|`-ze-opt-force-global-mem-allocation`|  | `KIND_FLAG` |
|`-ze-opt-fp64-gen-emu`| Enable full FP64 emulation. | `KIND_FLAG` |
|`-ze-opt-functonControl`|  | `KIND_SEPARATE` |
|`-ze-opt-greater-than-4GB-buffer-required`| When this flag is present, it indicates that any OpenCL buffers can be more than 4GB in size. If it is absent,  all buffers are not more than 4GB in size. | `KIND_FLAG` |
|`-ze-opt-has-buffer-offset-arg`| This flag, together with *[-cl-intel\|-ze-opt]-greater-than-4GB-buffer-required* is used to convert stateless memory accesses, called messages or load/store, into stateful ones. The OpenCL runtime can create a surface whose base is either *buffer_base* or *buffer_base + buffer_offset*, based on whether *buffer_offset* is used. | `KIND_FLAG` |
|`-ze-opt-has-positive-pointer-offset`| For any load and store (aka message) whose address = *ptrArg + offset*, where *ptrArg* is a kernel pointer argument, offset is assumed to be non-negative if this flag is present. | `KIND_FLAG` |
|`-ze-opt-high-accuracy-nolut-math`| Enbales experimental high accuracy implementations of transcendentals. | `KIND_FLAG` |
|`-ze-opt-ignoreBFRounding`| Folds BF operands into mul/add/cmp operations. | `KIND_FLAG` |
|`-ze-opt-include-sip-csr`|  | `KIND_FLAG` |
|`-ze-opt-include-sip-kernel-debug`|  | `KIND_FLAG` |
|`-ze-opt-include-sip-kernel-local-debug`|  | `KIND_FLAG` |
|`-ze-opt-kernel-debug-enable`|  | `KIND_FLAG` |
|`-ze-opt-ldstcombine`|  | `KIND_SEPARATE` |
|`-ze-opt-ldstcombine-max-loadbytes=`|  | `KIND_JOINED` |
|`-ze-opt-ldstcombine-max-storebytes=`|  | `KIND_JOINED` |
|`-ze-opt-ldstcombine=`|  | `KIND_JOINED` |
|`-ze-opt-ldstcombine_max-loadbytes`|  | `KIND_SEPARATE` |
|`-ze-opt-ldstcombine_max_storebytes`|  | `KIND_SEPARATE` |
|`-ze-opt-load-cache-default`|  | `KIND_SEPARATE` |
|`-ze-opt-load-cache-default=`|  | `KIND_JOINED` |
|`-ze-opt-minimum-valid-address-checking`| Set minimal valid address | `KIND_SEPARATE` |
|`-ze-opt-no-prera-scheduling`|  | `KIND_FLAG` |
|`-ze-opt-no-spill`|  | `KIND_FLAG` |
|`-ze-opt-num-thread-per-eu`| Overrides the current number of threads value defined by the user's command line option for the entire module or the compiler choice by heuristics. | `KIND_SEPARATE` |
|`-ze-opt-private-memory-minimal-size-per-thread`| When this flag is present, it guarantees that size of private memory allocated per thread can not be less then the given value. Constraint: <SIZE> >= 0. | `KIND_SEPARATE` |
|`-ze-opt-replace-global-offsets-by-zero`| OpenCL's global IDs are assumed to start from the origin at global offsets (offset_x, offset_y, offset_z) When this flag is present, it indicates that the global offsets are (0,0,0). | `KIND_FLAG` |
|`-ze-opt-scratch-space-private-memory-minimal-size-per-thread`| When this flag is present, it guarantees that size of scratch space private memory allocated per thread can not be less then the given value. Constraint: <SIZE> >= 0. | `KIND_SEPARATE` |
|`-ze-opt-skip-reloc-add`|  | `KIND_FLAG` |
|`-ze-opt-store-cache-default`|  | `KIND_SEPARATE` |
|`-ze-opt-store-cache-default=`|  | `KIND_JOINED` |
|`-ze-opt-use-32bit-ptr-arith`|  | `KIND_FLAG` |
|`-ze-opt-use-bindless-advanced-mode`|  | `KIND_FLAG` |
|`-ze-opt-use-bindless-buffers`| Use bindless mode for buffers | `KIND_FLAG` |
|`-ze-opt-use-bindless-images`| Use bindless mode for images | `KIND_FLAG` |
|`-ze-opt-use-bindless-legacy-mode`|  | `KIND_FLAG` |
|`-ze-opt-use-bindless-mode`|  | `KIND_FLAG` |
|`-ze-opt-use-bindless-printf`|  | `KIND_FLAG` |
|`-ze-opt-vector-coalesing`|  | `KIND_SEPARATE` |
|`-ze-opt-vector-coalesing=`|  | `KIND_JOINED` |
|`-[cl-\|ze-]store-cache-default=`|  | `KIND_JOINED` |
|`-[cl-\|ze-]vector-coalesing=`|  | `KIND_JOINED` |

### VCInternalOption
| Name | Description | Kind |
|:---- | :---- | :----: |
|`-binary-format`| Set in which format should be generated binary; values: 'cm' or 'ze' | `KIND_SEPARATE` |
|`-binary-format=`| Alias for -binary-format <value> | `KIND_JOINED` |
|`-[cl-\|ze-]buffer-bounds-checking`| Enable buffer bounds checking | `KIND_FLAG` |
|`-[cl-\|ze-]disable-sendwarwa`| Disable SendWAR WA PVC platform only | `KIND_FLAG` |
|`-[cl-\|ze-]emit-zebin-visa-sections`| Add vISA asm as sections in ZeBin | `KIND_FLAG` |
|`-[cl-\|ze-]minimum-valid-address-checking`| Set minimal valid address | `KIND_SEPARATE` |
|`-[cl-\|ze-]use-bindless-buffers`| Use bindless mode for buffers | `KIND_FLAG` |
|`-[cl-\|ze-]use-bindless-images`| Use bindless mode for images | `KIND_FLAG` |
|`-dump-asm`| Dump assembly (visaasm, asm, etc.) | `KIND_FLAG` |
|`-dump-isa-binary`| Dump isa binary after finalization pass | `KIND_FLAG` |
|`-dump-llvm-ir`| Dump llvm IR after SPIRV reading, optimizations and codegen | `KIND_FLAG` |
|`-emit-visa-only`| Compile until vISA | `KIND_FLAG` |
|`-flush-l3-for-global`| Enable flushing L3 cache for globals | `KIND_FLAG` |
|`-freset-llvm-stats`| Reset performance metrics before compilation | `KIND_FLAG` |
|`-freset-time-report`| Reset timing summary before compilation | `KIND_FLAG` |
|`-ftime-report`| Print timing summary of each stage of compilation | `KIND_FLAG` |
|`-gpu-scope-fence-on-single-tile`| Allow the use of  | `KIND_FLAG` |
|`-help`| Display available API options | `KIND_FLAG` |
|`-help-internal`| Display available internal options | `KIND_FLAG` |
|`-llvm-options`| Additional options forwarded to llvm CommandLine global option parser | `KIND_SEPARATE` |
|`-llvm-options=`| Alias for -llvm-options | `KIND_JOINED` |
|`-mdump_asm`| Alias for -dump-asm | `KIND_FLAG` |
|`-print-stats`| Print performance metrics and statistics | `KIND_FLAG` |
|`-runtime`| Set runtime for which binary should be generated; values: 'cm' or 'ze' | `KIND_SEPARATE` |
|`-runtime=`| Alias for -runtime <value> | `KIND_JOINED` |
|`-stats-file`| Filename to write statistics to | `KIND_SEPARATE` |
|`-stats-file=`| Alias for -stats-file | `KIND_JOINED` |
|`-target-features`| Auxiliary target features | `KIND_SEPARATE` |
|`-target-features=`| Alias for -target-features | `KIND_JOINED` |
|`-vc-disable-debuggable-kernels`| Disable emission of debuggable kernels for legacy path | `KIND_FLAG` |
|`-vc-ignore-loop-unroll-threshold-on-pragma`| Ignore loop unroll threshold on pragma for VC | `KIND_FLAG` |
|`-vc-interop-subgroup-size`| Set subgroup size used for cross-module calls/returns | `KIND_SEPARATE` |
|`-vc-loop-unroll-threshold`| Loop unroll threshold for VC | `KIND_SEPARATE` |
|`-vc-loop-unroll-threshold=`| Alias for -vc-loop-unroll-threshold <value> | `KIND_JOINED` |
|`-vc-report-lsc-stores-with-non-default-l1-cache-controls`| Ignore loop unroll threshold on pragma for VC | `KIND_FLAG` |
|`-<cl-\|ze->intel-buffer-bounds-checking`| Enable buffer bounds checking | `KIND_FLAG` |
|`-<cl-\|ze->intel-disable-sendwarwa`| Disable SendWAR WA PVC platform only | `KIND_FLAG` |
|`-<cl-\|ze->intel-emit-zebin-visa-sections`| Add vISA asm as sections in ZeBin | `KIND_FLAG` |
|`-<cl-\|ze->intel-minimum-valid-address-checking`| Set minimal valid address | `KIND_SEPARATE` |
|`-<cl-\|ze->intel-use-bindless-buffers`| Use bindless mode for buffers | `KIND_FLAG` |
|`-<cl-\|ze->intel-use-bindless-images`| Use bindless mode for images | `KIND_FLAG` |
|`-ze-opt-buffer-bounds-checking`| Enable buffer bounds checking | `KIND_FLAG` |
|`-ze-opt-disable-sendwarwa`| Disable SendWAR WA PVC platform only | `KIND_FLAG` |
|`-ze-opt-emit-zebin-visa-sections`| Add vISA asm as sections in ZeBin | `KIND_FLAG` |
|`-ze-opt-minimum-valid-address-checking`| Set minimal valid address | `KIND_SEPARATE` |
|`-ze-opt-use-bindless-buffers`| Use bindless mode for buffers | `KIND_FLAG` |
|`-ze-opt-use-bindless-images`| Use bindless mode for images | `KIND_FLAG` |
### References
[1]: [OpenCL documentation](https://registry.khronos.org/OpenCL/specs/3.0-unified/html/OpenCL_API.html#compiler-options)