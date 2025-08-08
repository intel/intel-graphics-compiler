<!--
/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/ -->
# IGC API Options list
This is an auto-generated list of options gathered from [ApiOptions.td](https://github.com/intel/intel-graphics-compiler/blob/master/IGC/Options/include/igc/Options/ApiOptions.td) file using [LLVM TableGen](https://llvm.org/docs/TableGen/).

The "Kind" column describes the type of Option as defined in [OptParser.td](https://github.com/llvm-mirror/llvm/blob/master/include/llvm/Option/OptParser.td#L24-L50) refer to it for further information.

## Table of contents
- [IGC API Options list](#igc-api-options-list)
  - [Table of contents](#table-of-contents)
  - [Legend](#legend)
  - [Option Lists](#option-lists)
    - [IGCApiOption](#igcapioption)
    - [IgcmcApiOption](#igcmcapioption)
    - [VCApiOption](#vcapioption)
    - [References](#references)

## Legend
The `cl-` and `ze-` prefixes correspond to OpenCL and Level Zero specific options. For more in-depth information regarding OpenCL options consult the OpenCL documentation[\[1\]](#references).

| Type | Example | Expands to |
|:---- | :---- | :---- |
| `<>` - Required choice | `-<cl-\|ze->option` | `-cl-option`; `-ze-option` |
| `[]`  - Optional choice | `-[cl-]option` | `-cl-option`; `-option` |
## Option Lists
### IGCApiOption
| Name | Description | Kind |
|:---- | :---- | :----: |
|`-D`| Manually define macros. | `KIND_JOINED_OR_SEPARATE` |
|`-g`| Enable generation of debug information and enables kernel debug | `KIND_FLAG` |
|`-I`| Add directory to the list of directories which will be searched for header files. | `KIND_JOINED_OR_SEPARATE` |
|`-s`| Strip all symbol table and debug informaton from the output binary. | `KIND_SEPARATE` |
|`-w`| Remove all warning messages. | `KIND_FLAG` |
|`-x`| Manualy provide type of file. Takes only spir or spir64 as argument. | `KIND_SEPARATE` |
|`-[cl-\|ze-]collect-cost-info`| Find argument symbols, calculate loop count and loop cost expressions | `KIND_FLAG` |
|`-cl-denorms-are-zero`| Controls how single precision and double precision denormalized numbers are handled. If specified as a build option, the single precision denormalized numbers may be flushed to zero; double precision denormalized numbers may also be flushed to zero if the optional extension for double precision is supported | `KIND_FLAG` |
|`-[cl-\|ze-]disable-compaction`| Disable compaction pass in finalizer. This pass is used to decide whether to use a compacted, i.e. shorter encoding of machine instructions wherever ISA allows. | `KIND_FLAG` |
|`-[cl-\|ze-]emit-lib-compile-errors`|  | `KIND_FLAG` |
|`-[cl-\|ze-]enable-auto-large-GRF-mode`| Use compiler heuristics to determine number of GRF | `KIND_FLAG` |
|`-[cl-\|ze-]enable-ieee-float-exception-trap`| This flag enables the IEEE exception trap bit in Control Register in the prolog of the kernel. | `KIND_FLAG` |
|`-[cl-\|ze-]exp-register-file-size`| Set amount of registers used by regalloc | `KIND_SEPARATE` |
|`-cl-fast-relaxed-math`| Sets the optimization options -cl-finite-math-only and -cl-unsafe-math-optimizations. This option causes the preprocessor macro __FAST_RELAXED_MATH__ to be defined in the OpenCL program. | `KIND_FLAG` |
|`-cl-finite-math-only`| Allow optimizations for floating-point arithmetic that assume that arguments and results are not NaNs, +Inf, -Inf. This option may violate the OpenCL numerical compliance requirements for single precision and double precision floating-point, as well as edge case behavior. | `KIND_FLAG` |
|`-[cl-\|ze-]fp32-correctly-rounded-divide-sqrt`| Allows an application to specify that single precision floating-point divide (x/y and 1/x) and sqrt used in the program source are correctly rounded. | `KIND_FLAG` |
|`-[cl-\|ze-]fp64-gen-conv-emu`|  | `KIND_FLAG` |
|`-[cl-\|ze-]fp64-gen-emu`|  | `KIND_FLAG` |
|`-[cl-\|ze-]greater-than-4GB-buffer-required`| When this flag is present, it indicates that any OpenCL buffers can be more than 4GB in size. If it is absent, all buffers are not more than 4GB in size. | `KIND_FLAG` |
|`-[cl-\|ze-]gtpin-grf-info`| Enable vISA grf-info interface for gtpin structure generation | `KIND_FLAG` |
|`-[cl-\|ze-]gtpin-indir-ref`| Ask finalizer to provide list of registers used by indirect operand per %ip | `KIND_FLAG` |
|`-[cl-\|ze-]gtpin-rera`| Enables vISA re_ra interface for gtpin structure generation | `KIND_FLAG` |
|`-[cl-\|ze-]gtpin-scratch-area-size`| Set gtpin scratch area size | `KIND_SEPARATE` |
|`-cl-intel-debug-info`|  | `KIND_FLAG` |
|`-cl-intel-disable-a64WA`|  | `KIND_FLAG` |
|`-cl-kernel-arg-info`| Allow the compiler to store information about the arguments of a kernel(s) in the program executable. The argument information stored includes the argument name, its type, the address space and access qualifiers used. | `KIND_FLAG` |
|`-[cl-\|ze-]large-grf-kernel`| -ze-opt-large-grf-kernel <string> tells IGC to use large GRF size if kernel name contains <string> regardless of module-level options. | `KIND_SEPARATE` |
|`-[cl-\|ze-]large-register-file`| Increase number of available GRF | `KIND_FLAG` |
|`-<cl-\|ze->library-compilation`|  | `KIND_FLAG` |
|`-[cl-\|ze-]library-compile-simd`| Select SIMD size for library compilations [8\|16\|32] | `KIND_SEPARATE` |
|`-cl-mad-enable`| Allow a * b + c to be replaced by a mad instruction. | `KIND_FLAG` |
|`-cl-match-sincospi`|  | `KIND_FLAG` |
|`-[cl-\|ze-]no-fusedCallWA`|  | `KIND_FLAG` |
|`-[cl-\|ze-]no-local-to-generic`|  | `KIND_FLAG` |
|`-cl-no-signed-zeros`| Allow optimizations for floating-point arithmetic that ignore the signedness of zero. | `KIND_FLAG` |
|`-[cl-\|ze-]no-subgroup-ifp`| This indicates that kernels in this program do not require sub-groups to make independent forward progress. | `KIND_FLAG` |
|`-[cl-\|ze-]opt-disable`| Turns off optimizations | `KIND_FLAG` |
|`-[cl-\|ze-]poison-unsupported-fp64-kernels`|  | `KIND_FLAG` |
|`-[cl-\|ze-]regular-grf-kernel`| -ze-opt-regular-grf-kernel <string> tells IGC to use regular GRF size if kernel name contains <string >regardless of module-level options. | `KIND_SEPARATE` |
|`-[cl-\|ze-]reqd-eu-thread-count`|  | `KIND_SEPARATE` |
|`-cl-single-precision-constant`| Forces implicit conversions of double-precision floating-point literals to single precision. | `KIND_FLAG` |
|`-[cl-\|ze-]skip-fde`|  | `KIND_FLAG` |
|`-[cl-\|ze-]static-profile-guided-trimming`| Enable static analysis in the kernel trimming. | `KIND_FLAG` |
|`-cl-std=`| Determine the language version to be accepted by the compiler. | `KIND_JOINED` |
|`-cl-strict-aliasing`| Allows the compiler to assume the strictest aliasing rules. | `KIND_FLAG` |
|`-[cl-\|ze-]take-global-address`|  | `KIND_FLAG` |
|`-[cl-\|ze-]uniform-work-group-size`| This requires that the global work-size be a multiple of the work-group size specified to clEnqueueNDRangeKernel. | `KIND_FLAG` |
|`-cl-unsafe-math-optimizations`| Allow optimizations for floating-point arithmetic that (a) assume that arguments and results are valid, (b) may violate the IEEE 754 standard, (c) assume relaxed OpenCL numerical compliance requirements as defined in the unsafe math optimization section of the OpenCL C or OpenCL SPIR-V Environment specifications, and (d) may violate edge case behavior in the OpenCL C or OpenCL SPIR-V Environment specifications. | `KIND_FLAG` |
|`-dump-opt-llvm=`| Dump the llvm output to the specified file. | `KIND_JOINED` |
|`-dwarf-column-info`|  | `KIND_FLAG` |
|`-gline-tables-only`| Generate only line table debug information. | `KIND_FLAG` |
|`-igc_opts`| Pass IGC options delimited by ',' or ' '. | `KIND_JOINED_OR_SEPARATE` |
|`-profiler`|  | `KIND_FLAG` |
|`-spir-std=`| Specify the SPIR version. | `KIND_JOINED` |
|`-triple`|  | `KIND_SEPARATE` |
|`-Werror`| Treat every warning as an error. | `KIND_FLAG` |
|`-Xfinalizer`| Pass <arg> to the visa finalizer | `KIND_SEPARATE` |
|`-[cl-\|ze-]128-GRF-per-thread`|  | `KIND_FLAG` |
|`-[cl-\|ze-]256-GRF-per-thread`|  | `KIND_FLAG` |
|`-[cl-\|ze-]exp-register-file-size=`| Alias for -ze-exp-register-file-size | `KIND_JOINED` |
|`-[cl-\|ze-]gtpin-scratch-area-size=`| Alias for -ze-gtpin-scratch-area-size | `KIND_JOINED` |
|`-<cl-\|ze->intel-128-GRF-per-thread`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-256-GRF-per-thread`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-collect-cost-info`| Find argument symbols, calculate loop count and loop cost expressions | `KIND_FLAG` |
|`-<cl-\|ze->intel-disable-compaction`| Disable compaction pass in finalizer. This pass is used to decide whether to use a compacted, i.e. shorter encoding of machine instructions wherever ISA allows. | `KIND_FLAG` |
|`-<cl-\|ze->intel-emit-lib-compile-errors`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-enable-auto-large-GRF-mode`| Use compiler heuristics to determine number of GRF | `KIND_FLAG` |
|`-<cl-\|ze->intel-enable-ieee-float-exception-trap`| This flag enables the IEEE exception trap bit in Control Register in the prolog of the kernel. | `KIND_FLAG` |
|`-<cl-\|ze->intel-exp-register-file-size`| Set amount of registers used by regalloc | `KIND_SEPARATE` |
|`-<cl-\|ze->intel-exp-register-file-size=`| Alias for -ze-exp-register-file-size | `KIND_JOINED` |
|`-<cl-\|ze->intel-fp32-correctly-rounded-divide-sqrt`| Allows an application to specify that single precision floating-point divide (x/y and 1/x) and sqrt used in the program source are correctly rounded. | `KIND_FLAG` |
|`-<cl-\|ze->intel-fp64-gen-conv-emu`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-fp64-gen-emu`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-greater-than-4GB-buffer-required`| When this flag is present, it indicates that any OpenCL buffers can be more than 4GB in size. If it is absent, all buffers are not more than 4GB in size. | `KIND_FLAG` |
|`-<cl-\|ze->intel-gtpin-grf-info`| Enable vISA grf-info interface for gtpin structure generation | `KIND_FLAG` |
|`-<cl-\|ze->intel-gtpin-indir-ref`| Ask finalizer to provide list of registers used by indirect operand per %ip | `KIND_FLAG` |
|`-<cl-\|ze->intel-gtpin-rera`| Enables vISA re_ra interface for gtpin structure generation | `KIND_FLAG` |
|`-<cl-\|ze->intel-gtpin-scratch-area-size`| Set gtpin scratch area size | `KIND_SEPARATE` |
|`-<cl-\|ze->intel-gtpin-scratch-area-size=`| Alias for -ze-gtpin-scratch-area-size | `KIND_JOINED` |
|`-<cl-\|ze->intel-large-grf-kernel`| -ze-opt-large-grf-kernel <string> tells IGC to use large GRF size if kernel name contains <string> regardless of module-level options. | `KIND_SEPARATE` |
|`-<cl-\|ze->intel-large-register-file`| Increase number of available GRF | `KIND_FLAG` |
|`-<cl-\|ze->intel-library-compilation`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-library-compile-simd`| Select SIMD size for library compilations [8\|16\|32] | `KIND_SEPARATE` |
|`-<cl-\|ze->intel-library-compile-simd=`| Select SIMD size for library compilations [8\|16\|32] | `KIND_JOINED` |
|`-<cl-\|ze->intel-no-fusedCallWA`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-no-local-to-generic`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-no-subgroup-ifp`| This indicates that kernels in this program do not require sub-groups to make independent forward progress. | `KIND_FLAG` |
|`-<cl-\|ze->intel-opt-disable`| Turns off optimizations | `KIND_FLAG` |
|`-<cl-\|ze->intel-poison-unsupported-fp64-kernels`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-regular-grf-kernel`| -ze-opt-regular-grf-kernel <string> tells IGC to use regular GRF size if kernel name contains <string >regardless of module-level options. | `KIND_SEPARATE` |
|`-<cl-\|ze->intel-reqd-eu-thread-count`|  | `KIND_SEPARATE` |
|`-<cl-\|ze->intel-reqd-eu-thread-count=`|  | `KIND_JOINED` |
|`-<cl-\|ze->intel-skip-fde`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-static-profile-guided-trimming`| Enable static analysis in the kernel trimming. | `KIND_FLAG` |
|`-<cl-\|ze->intel-take-global-address`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-uniform-work-group-size`| This requires that the global work-size be a multiple of the work-group size specified to clEnqueueNDRangeKernel. | `KIND_FLAG` |
|`-[cl-\|ze-]library-compile-simd=`| Select SIMD size for library compilations [8\|16\|32] | `KIND_JOINED` |
|`-ze-opt-128-GRF-per-thread`|  | `KIND_FLAG` |
|`-ze-opt-256-GRF-per-thread`|  | `KIND_FLAG` |
|`-ze-opt-collect-cost-info`| Find argument symbols, calculate loop count and loop cost expressions | `KIND_FLAG` |
|`-ze-opt-disable-compaction`| Disable compaction pass in finalizer. This pass is used to decide whether to use a compacted, i.e. shorter encoding of machine instructions wherever ISA allows. | `KIND_FLAG` |
|`-ze-opt-emit-lib-compile-errors`|  | `KIND_FLAG` |
|`-ze-opt-enable-auto-large-GRF-mode`| Use compiler heuristics to determine number of GRF | `KIND_FLAG` |
|`-ze-opt-enable-ieee-float-exception-trap`| This flag enables the IEEE exception trap bit in Control Register in the prolog of the kernel. | `KIND_FLAG` |
|`-ze-opt-exp-register-file-size`| Set amount of registers used by regalloc | `KIND_SEPARATE` |
|`-ze-opt-exp-register-file-size=`| Alias for -ze-exp-register-file-size | `KIND_JOINED` |
|`-ze-opt-fp32-correctly-rounded-divide-sqrt`| Allows an application to specify that single precision floating-point divide (x/y and 1/x) and sqrt used in the program source are correctly rounded. | `KIND_FLAG` |
|`-ze-opt-fp64-gen-conv-emu`|  | `KIND_FLAG` |
|`-ze-opt-fp64-gen-emu`|  | `KIND_FLAG` |
|`-ze-opt-greater-than-4GB-buffer-required`| When this flag is present, it indicates that any OpenCL buffers can be more than 4GB in size. If it is absent, all buffers are not more than 4GB in size. | `KIND_FLAG` |
|`-ze-opt-gtpin-grf-info`| Enable vISA grf-info interface for gtpin structure generation | `KIND_FLAG` |
|`-ze-opt-gtpin-indir-ref`| Ask finalizer to provide list of registers used by indirect operand per %ip | `KIND_FLAG` |
|`-ze-opt-gtpin-rera`| Enables vISA re_ra interface for gtpin structure generation | `KIND_FLAG` |
|`-ze-opt-gtpin-scratch-area-size`| Set gtpin scratch area size | `KIND_SEPARATE` |
|`-ze-opt-gtpin-scratch-area-size=`| Alias for -ze-gtpin-scratch-area-size | `KIND_JOINED` |
|`-ze-opt-large-grf-kernel`| -ze-opt-large-grf-kernel <string> tells IGC to use large GRF size if kernel name contains <string> regardless of module-level options. | `KIND_SEPARATE` |
|`-ze-opt-large-register-file`| Increase number of available GRF | `KIND_FLAG` |
|`-ze-opt-library-compilation`|  | `KIND_FLAG` |
|`-ze-opt-library-compile-simd`| Select SIMD size for library compilations [8\|16\|32] | `KIND_SEPARATE` |
|`-ze-opt-library-compile-simd=`| Select SIMD size for library compilations [8\|16\|32] | `KIND_JOINED` |
|`-ze-opt-no-fusedCallWA`|  | `KIND_FLAG` |
|`-ze-opt-no-local-to-generic`|  | `KIND_FLAG` |
|`-ze-opt-no-subgroup-ifp`| This indicates that kernels in this program do not require sub-groups to make independent forward progress. | `KIND_FLAG` |
|`-ze-opt-opt-disable`| Turns off optimizations | `KIND_FLAG` |
|`-ze-opt-poison-unsupported-fp64-kernels`|  | `KIND_FLAG` |
|`-ze-opt-regular-grf-kernel`| -ze-opt-regular-grf-kernel <string> tells IGC to use regular GRF size if kernel name contains <string >regardless of module-level options. | `KIND_SEPARATE` |
|`-ze-opt-reqd-eu-thread-count`|  | `KIND_SEPARATE` |
|`-ze-opt-reqd-eu-thread-count=`|  | `KIND_JOINED` |
|`-ze-opt-skip-fde`|  | `KIND_FLAG` |
|`-ze-opt-static-profile-guided-trimming`| Enable static analysis in the kernel trimming. | `KIND_FLAG` |
|`-ze-opt-take-global-address`|  | `KIND_FLAG` |
|`-ze-opt-uniform-work-group-size`| This requires that the global work-size be a multiple of the work-group size specified to clEnqueueNDRangeKernel. | `KIND_FLAG` |
|`-[cl-\|ze-]reqd-eu-thread-count=`|  | `KIND_JOINED` |

### IgcmcApiOption
| Name | Description | Kind |
|:---- | :---- | :----: |
|`-cmc`| Enable igcmc compatible mode; incompatible with -vc-codegen | `KIND_FLAG` |
|`-doubleGRF`| Alias for -ze-opt-large-register-file | `KIND_FLAG` |
|`-no_vector_decomposition`| Alias for -ze-no-vector-decomposition | `KIND_FLAG` |
|`-stack-mem-size=`| Control stack memory size (in bytes) | `KIND_JOINED` |
|`-visaopts=`| Options for finalizer in form  | `KIND_JOINED` |

### VCApiOption
| Name | Description | Kind |
|:---- | :---- | :----: |
|`-g`| Enable generation of debug information and enables kernel debug | `KIND_FLAG` |
|`-[cl-\|ze-]collect-cost-info`| Find argument symbols, calculate loop count and loop cost expressions | `KIND_FLAG` |
|`-[cl-\|ze-]disable-compaction`| Disable compaction pass in finalizer. This pass is used to decide whether to use a compacted, i.e. shorter encoding of machine instructions wherever ISA allows. | `KIND_FLAG` |
|`-[cl-\|ze-]enable-auto-large-GRF-mode`| Use compiler heuristics to determine number of GRF | `KIND_FLAG` |
|`-[cl-\|ze-]exp-register-file-size`| Set amount of registers used by regalloc | `KIND_SEPARATE` |
|`-[cl-\|ze-]gtpin-grf-info`| Enable vISA grf-info interface for gtpin structure generation | `KIND_FLAG` |
|`-[cl-\|ze-]gtpin-indir-ref`| Ask finalizer to provide list of registers used by indirect operand per %ip | `KIND_FLAG` |
|`-[cl-\|ze-]gtpin-rera`| Enables vISA re_ra interface for gtpin structure generation | `KIND_FLAG` |
|`-[cl-\|ze-]gtpin-scratch-area-size`| Set gtpin scratch area size | `KIND_SEPARATE` |
|`-[cl-\|ze-]large-register-file`| Increase number of available GRF | `KIND_FLAG` |
|`-<cl-\|ze->library-compilation`|  | `KIND_FLAG` |
|`-[cl-\|ze-]library-compile-simd`| Select SIMD size for library compilations [8\|16\|32] | `KIND_SEPARATE` |
|`-[cl-\|ze-]no-fusedCallWA`|  | `KIND_FLAG` |
|`-[cl-\|ze-]opt-disable`| Turns off optimizations | `KIND_FLAG` |
|`-[cl-\|ze-]skip-fde`|  | `KIND_FLAG` |
|`-codegen-optimize`| Set codegen optimization level to either 'none' or 'full' | `KIND_SEPARATE` |
|`-codegen-optimize=`| Alias for -codegen-optimize | `KIND_JOINED` |
|`-depressurizer-flag-grf-tolerance`| Threshold for disabling flag pressure reduction | `KIND_SEPARATE` |
|`-depressurizer-flag-grf-tolerance=`| Alias for -depressurizer-flag-grf-tolerance <value> | `KIND_JOINED` |
|`-depressurizer-grf-threshold`| Threshold for GRF pressure reduction | `KIND_SEPARATE` |
|`-depressurizer-grf-threshold=`| Alias for -depressurizer-grf-threshold <value> | `KIND_JOINED` |
|`-disable-finalizer-msg`| Turns off critical messages from cISA Builder | `KIND_FLAG` |
|`-double-GRF`| Alias for -ze-opt-large-register-file | `KIND_FLAG` |
|`-doubleGRF`| Alias for -ze-opt-large-register-file | `KIND_FLAG` |
|`-ffp-contract=`| Form fused FP ops: fast (everywhere) \| on (nowhere except insertion fma) \| off (never fuse) | `KIND_JOINED` |
|`-fno-jump-tables`| Disable the use of jump tables for lowering switches | `KIND_FLAG` |
|`-fno-struct-splitting`| Disable StructSplitter pass | `KIND_FLAG` |
|`-ftranslate-legacy-memory-intrinsics`| Legalize old memory intrinsics | `KIND_FLAG` |
|`-no-optimize`| Alias for -optimize=none | `KIND_FLAG` |
|`-[ze-]no-vector-decomposition`| Alias for -ze-no-vector-decomposition | `KIND_FLAG` |
|`-no_vector_decomposition`| Alias for -ze-no-vector-decomposition | `KIND_FLAG` |
|`-optimize`| Set optimization level to either 'none' or 'full' | `KIND_SEPARATE` |
|`-optimize=`| Alias for -optimize | `KIND_JOINED` |
|`-stateless-stack-mem-size=`| Stateless memory amount per thread | `KIND_JOINED` |
|`-vc-codegen`| Enable vector codegenerator | `KIND_FLAG` |
|`-vc-disable-indvars-opt`| Disable induction variable optimization | `KIND_FLAG` |
|`-vc-disable-non-overlapping-region-opt`| Disable non-overlapping region optimization | `KIND_FLAG` |
|`-vc-enable-preemption`| Enable preemption for VC path | `KIND_FLAG` |
|`-vc-use-bindless-buffers`| Enable bindless buffer access | `KIND_FLAG` |
|`-vc-use-bindless-images`| Enable bindless image access | `KIND_FLAG` |
|`-vc-use-plain-2d-images`| Treat image2d_t annotation as non-media image | `KIND_FLAG` |
|`-Xfinalizer`| Pass <arg> to the visa finalizer | `KIND_SEPARATE` |
|`-[cl-\|ze-]exp-register-file-size=`| Alias for -ze-exp-register-file-size | `KIND_JOINED` |
|`-[cl-\|ze-]gtpin-scratch-area-size=`| Alias for -ze-gtpin-scratch-area-size | `KIND_JOINED` |
|`-<cl-\|ze->intel-collect-cost-info`| Find argument symbols, calculate loop count and loop cost expressions | `KIND_FLAG` |
|`-<cl-\|ze->intel-disable-compaction`| Disable compaction pass in finalizer. This pass is used to decide whether to use a compacted, i.e. shorter encoding of machine instructions wherever ISA allows. | `KIND_FLAG` |
|`-<cl-\|ze->intel-enable-auto-large-GRF-mode`| Use compiler heuristics to determine number of GRF | `KIND_FLAG` |
|`-<cl-\|ze->intel-exp-register-file-size`| Set amount of registers used by regalloc | `KIND_SEPARATE` |
|`-<cl-\|ze->intel-exp-register-file-size=`| Alias for -ze-exp-register-file-size | `KIND_JOINED` |
|`-<cl-\|ze->intel-gtpin-grf-info`| Enable vISA grf-info interface for gtpin structure generation | `KIND_FLAG` |
|`-<cl-\|ze->intel-gtpin-indir-ref`| Ask finalizer to provide list of registers used by indirect operand per %ip | `KIND_FLAG` |
|`-<cl-\|ze->intel-gtpin-rera`| Enables vISA re_ra interface for gtpin structure generation | `KIND_FLAG` |
|`-<cl-\|ze->intel-gtpin-scratch-area-size`| Set gtpin scratch area size | `KIND_SEPARATE` |
|`-<cl-\|ze->intel-gtpin-scratch-area-size=`| Alias for -ze-gtpin-scratch-area-size | `KIND_JOINED` |
|`-<cl-\|ze->intel-large-register-file`| Increase number of available GRF | `KIND_FLAG` |
|`-<cl-\|ze->intel-library-compilation`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-library-compile-simd`| Select SIMD size for library compilations [8\|16\|32] | `KIND_SEPARATE` |
|`-<cl-\|ze->intel-library-compile-simd=`| Select SIMD size for library compilations [8\|16\|32] | `KIND_JOINED` |
|`-<cl-\|ze->intel-no-fusedCallWA`|  | `KIND_FLAG` |
|`-<cl-\|ze->intel-opt-disable`| Turns off optimizations | `KIND_FLAG` |
|`-<cl-\|ze->intel-skip-fde`|  | `KIND_FLAG` |
|`-ze-kernel-debug-enable`| Alias for -g | `KIND_FLAG` |
|`-[cl-\|ze-]library-compile-simd=`| Select SIMD size for library compilations [8\|16\|32] | `KIND_JOINED` |
|`-ze-opt-collect-cost-info`| Find argument symbols, calculate loop count and loop cost expressions | `KIND_FLAG` |
|`-ze-opt-disable-compaction`| Disable compaction pass in finalizer. This pass is used to decide whether to use a compacted, i.e. shorter encoding of machine instructions wherever ISA allows. | `KIND_FLAG` |
|`-ze-opt-enable-auto-large-GRF-mode`| Use compiler heuristics to determine number of GRF | `KIND_FLAG` |
|`-ze-opt-exp-register-file-size`| Set amount of registers used by regalloc | `KIND_SEPARATE` |
|`-ze-opt-exp-register-file-size=`| Alias for -ze-exp-register-file-size | `KIND_JOINED` |
|`-ze-opt-gtpin-grf-info`| Enable vISA grf-info interface for gtpin structure generation | `KIND_FLAG` |
|`-ze-opt-gtpin-indir-ref`| Ask finalizer to provide list of registers used by indirect operand per %ip | `KIND_FLAG` |
|`-ze-opt-gtpin-rera`| Enables vISA re_ra interface for gtpin structure generation | `KIND_FLAG` |
|`-ze-opt-gtpin-scratch-area-size`| Set gtpin scratch area size | `KIND_SEPARATE` |
|`-ze-opt-gtpin-scratch-area-size=`| Alias for -ze-gtpin-scratch-area-size | `KIND_JOINED` |
|`-ze-opt-large-register-file`| Increase number of available GRF | `KIND_FLAG` |
|`-ze-opt-library-compilation`|  | `KIND_FLAG` |
|`-ze-opt-library-compile-simd`| Select SIMD size for library compilations [8\|16\|32] | `KIND_SEPARATE` |
|`-ze-opt-library-compile-simd=`| Select SIMD size for library compilations [8\|16\|32] | `KIND_JOINED` |
|`-ze-opt-no-fusedCallWA`|  | `KIND_FLAG` |
|`-ze-opt-opt-disable`| Turns off optimizations | `KIND_FLAG` |
|`-ze-opt-skip-fde`|  | `KIND_FLAG` |
### References
[1]: [OpenCL documentation](https://registry.khronos.org/OpenCL/specs/3.0-unified/html/OpenCL_API.html#compiler-options)