<!---======================= begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

# Intel&reg; Graphics Compiler for OpenCL&trade;

## Ubuntu build section

### 1. Prepare workspace

Building IGC needs flex, bison, libz and cmake version at least 3.13.4. You can install required packages on Ubuntu using this command:

```shell
$ sudo apt-get install flex bison libz-dev cmake
```

Some of the incoming git operations will try to download and apply patches. For this purpose it is necessary to setup git credentials if they are not already in the git configuration:
```shell
$ git config --global user.name "FirstName LastName"
$ git config --global user.email "your@email.com"
```

### 2. Install LLVM, LLD, Clang and OpenCL Clang

> **Important notice**
> Every LLVM/LLD/Clang version brings some restrictions and in some cases needs different configuration. Please refer to [LLVM/LLD/Clang version information](#LLVM/Clang-version-information) section for more information.

In this step you need to prepare VC Intrinsics, SPIRV-LLVM Translator, LLVM, LLD, OpenCL-Clang libraries and Clang for IGC.
It can be done either by using packaged releases or building those yourself:

#### Use preinstalled packages

For **LLVM**, **LLD** and **Clang** packages please visit this [link](https://apt.llvm.org/) to download and install desired version.
For `apt` package manager you can use this command:
```shell
$ sudo apt-get install llvm-11 llvm-11-dev clang-11 liblld-11 liblld-11-dev libllvmspirvlib11 libllvmspirvlib-dev
```
As of now **OpenCL Clang** is still needed to be built and installed manually. Sources are available [here](https://github.com/intel/opencl-clang). You can use out-of-tree build method with LLVM and Clang preinstalled.
**VC Intrinsics** is a lightweight library that is built from sources with IGC and there is no package for it.

Installing LLVM, Clang, and OpenCL Clang components means you no longer have to download their sources alongside IGC, so the workspace tree in the next step may look like this:
```
<workspace>
      |- igc                          https://github.com/intel/intel-graphics-compiler
      |- vc-intrinsics                https://github.com/intel/vc-intrinsics
      |- SPIRV-Tools                  https://github.com/KhronosGroup/SPIRV-Tools
      |- SPIRV-Headers                https://github.com/KhronosGroup/SPIRV-Headers
```
Additionaly, you can use **SPIRV-Tools** prebuild package. In order to do that be sure to pass to cmake command `IGC_OPTION__SPIRV_TOOLS_MODE=Prebuilds`.
When **SPIRV-Tools** are set to Prebuilds you may also use **SPIRV-Headers** prebuild package with the cmake option `IGC_OPTION__USE_PREINSTALLED_SPIRV_HEADERS=ON`.
Mind that until the issue https://github.com/KhronosGroup/SPIRV-Tools/issues/3909 will not be resolved, we support SPIRV_Tools only as a shared lib, and we encourage to build SPIRV-Tools prebuild with SPIRV_TOOLS_BUILD_STATIC=OFF flag.

Moreover, OpenCL Clang and Vector Compiler share the SPIRV-LLVM Translator library. SPIRV-LLVM Translator cannot be built if OpenCL-Clang is taken as prebuilt from system. This can lead to problems with linking.

#### Build from sources

Download all dependencies and create workspace.
You can use following commands for setup:

```shell
$ cd <workspace>
$ git clone https://github.com/intel/vc-intrinsics vc-intrinsics
$ git clone -b llvmorg-14.0.5 https://github.com/llvm/llvm-project llvm-project
$ git clone -b ocl-open-140 https://github.com/intel/opencl-clang llvm-project/llvm/projects/opencl-clang
$ git clone -b llvm_release_140 https://github.com/KhronosGroup/SPIRV-LLVM-Translator llvm-project/llvm/projects/llvm-spirv
$ git clone https://github.com/KhronosGroup/SPIRV-Tools.git SPIRV-Tools
$ git clone https://github.com/KhronosGroup/SPIRV-Headers.git SPIRV-Headers
```
These commands will set up a workspace with LLVM 14. If you wish to use any other version please refer to the [component revision table](#Revision-table)

Correct directory tree looks like this:
```
<workspace>
    |- igc              https://github.com/intel/intel-graphics-compiler
    |- vc-intrinsics    https://github.com/intel/vc-intrinsics
    |- SPIRV-Tools      https://github.com/KhronosGroup/SPIRV-Tools
    |- SPIRV-Headers    https://github.com/KhronosGroup/SPIRV-Headers
    |- llvm-project     https://github.com/llvm/llvm-project
        |- llvm/projects/opencl-clang    https://github.com/intel/opencl-clang
        |- llvm/projects/llvm-spirv      https://github.com/KhronosGroup/SPIRV-LLVM-Translator
```

All dependencies will be built in the next step.

#### Additional notes on build modes

There are several flags for these builds modes that you can pass to
cmake command.

- `IGC_OPTION__LLVM_PREFERRED_VERSION` -- sets version of LLVM that
  will be used by IGC (defaults to "14.0.5").
- `IGC_OPTION__LLVM_MODE` -- select LLVM mode for IGC to use. Possible
values are: **Source**, **Prebuilds** or empty (that is
default). **Source** mode uses LLVM sources to build LLVM in-tree with
IGC. In **Prebuilds** mode IGC will search for prebuild LLVM
package. If mode is empty then default search procedure will happen:
IGC will try **Source** mode first, then fall back to **Prebuilds**
mode. Each mode has suboptions to control IGC behavior.

**Source** mode has the following suboptions:
  - `IGC_OPTION__LLVM_STOCK_SOURCES` -- whether non-patched LLVM will
  be used or not (by default OFF).
  - `IGC_OPTION__LLVM_SOURCES_DIR` -- path to llvm sources when
  building with LLVM sources (by default IGC takes whatever can be
  found on the same directory level like in example above).

**Prebuilds** mode has only one suboption (that is default CMake
  variable):
  - `LLVM_ROOT` -- additional paths to search for LLVM -- these are
  searched before system paths.

For more detailed info about every mode see:
1. **Source** mode (see [source build](#build-from-sources));
1. **Prebuilds** mode (see [build with packages](#use-preinstalled-packages)).

### 3. Build and install IGC

1. Download sources:
```shell
$ cd <workspace>
$ git clone https://github.com/intel/intel-graphics-compiler igc
  [If using specific release]
$ cd igc && git checkout -b tag igc-<version>
```

2. Prepare workspace and build

If you are using [Use preinstalled packages](#use-preinstalled-packages) method IGC will link with installed dependencies dynamically.
If you are using [Build from sources](#build-from-sources) method IGC will automatically build all dependencies (provided that the workspace structure is preserved) and link statically to LLVM and OpenCL Clang.

You can use following commands to build IGC:

```shell
$ cd <workspace>
$ mkdir build
$ cd build
$ cmake ../igc
$ make -j`nproc`
```

3. Install IGC:
```shell
$ sudo make install
```

***

## LLVM/LLD/Clang version information

### Version Overview

| Version          | Product quality |
|:----------------:|-----------------|
| LLVM 15          | Experimental    |
| LLVM 14          | **Production**  |
| LLVM 12 and older | Experimental    |

| Terminology       | Description |
|-------------------|-|
| Production        | Current production version; Builds are verified internally |
| Experimental      | No quality expectations |

### Revision table

LLVM version determines what branches are used when building dependencies.
When checking out the components refer to the following table, replace **XX** with the LLVM version used:

| Repository name       | Version specific | Branch               | LLVM 14 example  |
|-----------------------|:----------------:|----------------------|------------------|
| llvm-project          | -                | release/**XX**.x     | release/14.x     |
| vc-intrinsics         | no               | master               | master           |
| SPIRV-Tools           | no               | master               | master           |
| SPIRV-Headers         | no               | master               | master           |
| SPIRV-LLVM-Translator | yes              | llvm_release_**XX**0 | llvm_release_140 |
| opencl-clang          | yes              | ocl-open-**XX**0     | ocl-open-140     |

### LLVM/LLD/Clang version specific caveats

Some LLVM versions require special steps to build successfully.

#### LLVM7/Clang7

In the **OpenCL Clang** project there are patches for Clang.
If the Clang you are using to build IGC does not have these patches (for example, when you are using prebuilt packages) it is necessary to add ```-DVME_TYPES_DEFINED=FALSE``` to IGC CMake flags.

VectorComplier must be disabled by adding ```-DIGC_BUILD__VC_ENABLED=OFF``` to CMake flags.

#### LLVM8/Clang8

We recommend building LLVM8/Clang8 from sources instead for using prebuilds, because packaged Clang8 is missing these patches:
* [0001-OpenCL-Change-type-of-block-pointer-for-OpenCL.patch](https://github.com/intel/opencl-clang/blob/ocl-open-80/patches/clang/0001-OpenCL-Change-type-of-block-pointer-for-OpenCL.patch)
* [0002-OpenCL-Simplify-LLVM-IR-generated-for-OpenCL-blocks.patch](https://github.com/intel/opencl-clang/blob/ocl-open-80/patches/clang/0002-OpenCL-Simplify-LLVM-IR-generated-for-OpenCL-blocks.patch)
* [0003-OpenCL-Fix-assertion-due-to-blocks.patch](https://github.com/intel/opencl-clang/blob/ocl-open-80/patches/clang/0003-OpenCL-Fix-assertion-due-to-blocks.patch)

which are needed for [enqueue_kernel](https://www.khronos.org/registry/OpenCL/sdk/2.0/docs/man/xhtml/enqueue_kernel.html).

VectorComplier must be disabled by adding ```-DIGC_BUILD__VC_ENABLED=OFF``` to CMake flags.

#### LLVM9/Clang9 to LLVM11/Clang11

**Deprecated/Experimental**
We can no longer provide LLVM9-11/Clang9-11 conformance/performance guarantees.

#### LLVM12/Clang12 to LLVM13/Clang13

**Experimental**
There are no LLVM12-13/Clang12-13 conformance/performance guarantees.

#### LLVM14/Clang14

No additional steps are needed.

#### LLVM15/Clang15

**Experimental**
There are no LLVM15/Clang15 conformance/performance guarantees.