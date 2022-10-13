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

## Disabling compilation passes

This functionality allows disabling specific passes from IGC compilation runtime. IGC debug build is required.

Disabling some of the passes from the compilation stack can cause a compilation crash, as some passes are essential.

### `ShaderPassDisable` flag overview
#### Print out information about compilation passes
Print all passes' IDs, names, and their occurrence number with flag `IGC_ShaderDisplayAllPassesNames=1`. Note: this does not correspond 1:1 to what the shader dumps are called.

Example output:

```bash
Pass number Pass name                                         Pass occurrence
0           CheckInstrTypes                                   1
1           Types Legalization Pass                           1
2           Target Library Information                        1
3           Dead Code Elimination                             1
...
...
...
225         Layout                                            1
226         TimeStatsCounter Start/Stop                       6
227         EmitPass                                          1
228         EmitPass                                          2
229         EmitPass                                          3
230         DebugInfoPass                                     1
```
#### `ShaderPassDisable` flag syntax

```bash
ShaderPassDisable="TOKEN1;TOKEN2;..."
TOKEN:
* Pass number                           PASS_NUMBER                                 example: 14
* Range                                 PASS_NUMBER_START-PASS_NUMBER_STOP          example: 10-50
* Open range                            PASS_NUMBER_START-                          example: 60-
* Pass name                             PASS_NAME                                   example: BreakConstantExprPass
* Specific pass occurrence              PASS_NAME:occurrence                        example: BreakConstantExprPass:0
* Specific pass occurrences range       PASS_NAME:OCCURRANCE_START-OCCURRANCE_STOP  example: BreakConstantExprPass:2-4
* Specific pass occurrences open range  PASS_NAME:OCCURRANCE_START-                 example: BreakConstantExprPass:3-
```

Example:
```bash
IGC_ShaderPassDisable="9;17-19;239-;Error Check;ResolveOCLAtomics:2;Dead Code Elimination:3-5;BreakConstantExprPass:7-"
You want to skip pass ID 9
You want to skip passes from 17 to 19
You want to skip passes from 239 to end
You want to skip all occurrence of Error Check
You want to skip pass ResolveOCLAtomics occurrence 2
You want to skip pass Dead Code Elimination from occurrence 3 to occurrence 5
You want to skip pass BreakConstantExprPass from occurrence 7 to end
Pass number    Pass name                                         Pass occurrence     skippedBy
9              LowerInvokeSIMD                                   1                   PassNumber
17             CorrectlyRoundedDivSqrt                           1                   Range
18             CodeAssumption                                    1                   Range
19             JointMatrixFuncsResolutionPass                    1                   Range
32             Error Check                                       1                   PassName
65             ResolveOCLAtomics                                 2                   SpecificPassOccurrence
75             Dead Code Elimination                             3                   SpecificPassOccurrenceRange
77             Error Check                                       2                   PassName
121            Dead Code Elimination                             4                   SpecificPassOccurrenceRange
143            Dead Code Elimination                             5                   SpecificPassOccurrenceRange
145            BreakConstantExprPass                             7                   SpecificPassOccurrenceOpenRange
181            BreakConstantExprPass                             8                   SpecificPassOccurrenceOpenRange
183            BreakConstantExprPass                             9                   SpecificPassOccurrenceOpenRange
189            BreakConstantExprPass                             10                  SpecificPassOccurrenceOpenRange
214            BreakConstantExprPass                             11                  SpecificPassOccurrenceOpenRange
221            BreakConstantExprPass                             12                  SpecificPassOccurrenceOpenRange
239            Layout                                            1                   OpenRange
240            TimeStatsCounter Start/Stop                       6                   OpenRange
241            EmitPass                                          1                   OpenRange
242            EmitPass                                          2                   OpenRange
243            EmitPass                                          3                   OpenRange
244            DebugInfoPass                                     1                   OpenRange
```
#### Implementation of `ShaderPassDisable`
Implementation of `ShaderPassDisable` flag can be found [here](https://github.com/intel/intel-graphics-compiler/blob/master/IGC/common/LLVMUtils.cpp#L81)


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
