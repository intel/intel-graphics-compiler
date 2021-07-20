<!---======================= begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

# Intel&reg; Graphics Compiler for OpenCL&trade;

## Producing shader dumps

Shader dumps (further abbreviated as *dumps*) are intermediate compilation snapshots created during IGC runtime. They are the main IGC debugging tool. For example: producing dumps from two consecutive builds and comparing them will show exactly what effect the change has had on the compilation process.

### Enabling the functionality

To produce shader dumps you need to set an environmental variable:
```bash
export IGC_ShaderDumpEnable=1
```
When this is set simply run any application that uses IGC to compile shaders and IGC will produce dumps in the default directory `/tmp/IntelIGC`.

To compile a standalone `*.cl` shader file use `ocloc` compiler frontend executable from [Intel Compute Runtime](https://github.com/intel/compute-runtime) repository.

### List of dump products

Shader dumps include (in order of creation):

| Product | Description |
|-|-|
| `*.cl` file | Original shader file. Passed to `clang` wrapped with `opencl-clang`. |
| `*.spv` file | SPIR-V binary produced by `opencl-clang`. Passed to IGC's baked-in [SPIRVReader](https://github.com/intel/intel-graphics-compiler/blob/master/IGC/AdaptorOCL/SPIRV/SPIRVReader.cpp). |
| `*.spvasm` file | Disassembled SPIR-V binary only for debugging purposes. |
| `*_beforeUnification.ll` file | Result of `*.spv` translation. Start of IGC compilation. |
| `*_afterUnification.ll` file | Result of all unification passes. |
| `*_optimized.ll` file | Result of all optimization passes. |
| `*_codegen.ll` file | Result of all codegen passes. |
| `*.visa.ll` files | Mappings between LLVM IR &LeftRightArrow; vISA. May not be legal LLVM IR representation. |
| `*.visaasm` files | vISA assembly file. |
| `*.asm` files | Kernels in assembly form. |
| `*.isa` files | Kernels' binaries. |

There are also additional products that are not a part of compilation pipeline:

| Additional products | Description |
|-|-|
|`*_options.txt` file | Options passed to `clBuildProgram`. |
|`*.cos` files | "Patch tokens" - information about kernels that is passed between IGC and Intel Compute Runtime. |

#### Notes
1. There is a separate SPIR-V translator for `*.cl` &RightArrow; `*.spv` translation (`opencl-clang`'s) and another for `*.spv` &RightArrow; `*.ll` translation (baked into into IGC).
2. vISA is an intermediate language for IGC backend.

### Additional options

To customize the process use configuration flags. Here are some of them:

|Flag|Description|
|-|-|
|`IGC_DumpToCustomDir`| Path to a directory to produce dumps to. |
|`IGC_ShaderDumpEnableAll`| Dump LLVM passes between stages. |
|`IGC_ShaderDumpPidDisable`| Do not append PID to dumps directory. |

You can learn more about dumping options in configuration flags documentation [here](https://github.com/intel/intel-graphics-compiler/blob/master/documentation/configuration_flags.md).

## Shader overriding

This is a more advanced debugging functionality that allows overriding most of the compilation steps with own dumps during IGC runtime. This option requires Debug IGC build.

### Basic overview

After producing the dumps once a step can be modified manually and then injected into the compiler pipeline during next executions.

Let us say we would like to modify the `*_afterUnification.ll` step produced by some particular IGC compilation to see how the compilation proceeded. We can patch IGC itself but it may be difficult or not worth the effort. A simpler solution is to use shader overriding and inject modified `*_afterUnification.ll` dump.

### Enabling overriding

To enable overrides you need to set an environmental variable:
```bash
export IGC_ShaderOverride=1
```

### Overriding dumps

To inject a dump into a compilation pipeline:
1. Produce shader dumps normally.
2. Choose a dump to be injected and modify it however is needed.
3. Copy modified dump to the `/tmp/IntelIGC/ShaderOverride` directory.
4. The hash in the name of the dump being injected needs to be the same as the dumps produced normally by the program.

The next time a compilation is run IGC will take files from this directory and override its running compilation. If the process is successful a log message `OVERRIDEN: ...` will be printed to `stdout`.
