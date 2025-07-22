<!---======================= begin_copyright_notice ============================

Copyright (C) 2019-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Ubuntu build section

### 1. Prepare workspace

Building IGC needs flex, bison, libz and cmake version at least 3.13.4. You can install required packages on Ubuntu using this command:

```shell
sudo apt-get install flex bison libz-dev cmake libc6 libstdc++6 python3-pip
sudo python3 -m pip install mako
```

Some of the incoming git operations will try to download and apply patches. For this purpose it is necessary to setup git credentials if they are not already in the git configuration:
```shell
git config --global user.name "FirstName LastName"
git config --global user.email "your@email.com"
```

### 2. Install LLVM, LLD, Clang and OpenCL Clang

> **Important notice**
> Every LLVM/LLD/Clang version brings some restrictions and in some cases needs different configuration. Please refer to [LLVM/LLD/Clang version information](#LLVM/Clang-version-information) section for more information.

In this step you need to prepare VC Intrinsics, SPIRV-LLVM Translator, LLVM, LLD, OpenCL-Clang libraries and Clang for IGC.
It can be done either by using packaged releases or building those yourself:

#### Use preinstalled packages

**Configuration using preinstalled packages is not recommended and is not being validated.**

IGC can use preinstalled LLVM, however while building with LLVM from sources it applies additional patches addressing certain workloads.
Also, depending on the Ubuntu release, SPIRV-LLVM-Translator available through apt may be older than required.

<details>

For **LLVM**, **LLD** and **Clang** packages please visit this [link](https://apt.llvm.org/) to download and install desired version.
For `apt` package manager you can use this command:
```shell
sudo apt-get install llvm-15 llvm-15-dev clang-15 liblld-15 liblld-15-dev libllvmspirvlib15 libllvmspirvlib-15-dev
```
As of now **OpenCL Clang** is still needed to be built and installed manually. Sources are available [here](https://github.com/intel/opencl-clang). You can use out-of-tree build method with LLVM and Clang preinstalled.
**VC Intrinsics** is a lightweight library that is built from sources with IGC and there is no package for it.

Installing LLVM, Clang, and OpenCL Clang components means you no longer have to download their sources alongside IGC, so the workspace tree in the next step may look like this:
```
IGC_WORKSPACE_DIR
      |- igc                          https://github.com/intel/intel-graphics-compiler
      |- vc-intrinsics                https://github.com/intel/vc-intrinsics
      |- SPIRV-Tools                  https://github.com/KhronosGroup/SPIRV-Tools
      |- SPIRV-Headers                https://github.com/KhronosGroup/SPIRV-Headers
```
Additionaly, you can use **SPIRV-Tools** prebuild package. In order to do that be sure to pass to cmake command `IGC_OPTION__SPIRV_TOOLS_MODE=Prebuilds`.
When **SPIRV-Tools** are set to Prebuilds you may also use **SPIRV-Headers** prebuild package with the cmake option `IGC_OPTION__USE_PREINSTALLED_SPIRV_HEADERS=ON`.
Mind that until the issue https://github.com/KhronosGroup/SPIRV-Tools/issues/3909 will not be resolved, we support SPIRV_Tools only as a shared lib, and we encourage to build SPIRV-Tools prebuild with SPIRV_TOOLS_BUILD_STATIC=OFF flag.

Moreover, OpenCL Clang and Vector Compiler share the SPIRV-LLVM Translator library. SPIRV-LLVM Translator cannot be built if OpenCL-Clang is taken as prebuilt from system. This can lead to problems with linking.

</details>

#### Build from sources

Create workspace and ```cd``` into it:

```shell
export IGC_WORKSPACE_DIR=/your/desired/path
mkdir -p $IGC_WORKSPACE_DIR && cd $IGC_WORKSPACE_DIR
```
Download all dependencies
```shell
git clone https://github.com/intel/vc-intrinsics vc-intrinsics
git clone -b llvmorg-15.0.7 https://github.com/llvm/llvm-project llvm-project
git clone -b ocl-open-150 https://github.com/intel/opencl-clang llvm-project/llvm/projects/opencl-clang
git clone -b llvm_release_150 https://github.com/KhronosGroup/SPIRV-LLVM-Translator llvm-project/llvm/projects/llvm-spirv
git clone https://github.com/KhronosGroup/SPIRV-Tools.git SPIRV-Tools
git clone https://github.com/KhronosGroup/SPIRV-Headers.git SPIRV-Headers
```
These commands will set up a workspace with LLVM 15. If you wish to use any other version please refer to the [component revision table](#Revision-table)

Correct directory tree looks like this:
```
IGC_WORSPACE_DIR
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
  will be used by IGC (defaults to "15.0.7").
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
git clone https://github.com/intel/intel-graphics-compiler igc
```
If you want to use a specific IGC release, for example [v2.14.1](https://github.com/intel/intel-graphics-compiler/releases/tag/v2.14.1), do the following:
```shell
cd igc
git fetch --all --tags --prune
git checkout tags/v2.14.1 -b 2.14.1
```
After checkout you can name your new branch however you want, above is only an example.
To get a list of all releases go [here](https://github.com/intel/intel-graphics-compiler/releases).

2. Prepare workspace and build

If you are using [Use preinstalled packages](#use-preinstalled-packages) method IGC will link with installed dependencies dynamically.
If you are using [Build from sources](#build-from-sources) method IGC will automatically build all dependencies (provided that the workspace structure is preserved) and link statically to LLVM and OpenCL Clang.

You can use following commands to build IGC:

```shell
cd $IGC_WORKSPACE_DIR
mkdir build && cd build
cmake ../igc
make -j`nproc`
```

3. Install IGC:
```shell
sudo make install
```

#### Additional notes on OpenCL LIT tests run
If you have installed [intel-opencl-icd](https://github.com/intel/compute-runtime), you can pass the following CMake flags to run the integrated OpenCL LIT test-suite when building IGC, or separately afterwards:

```shell
cd $IGC_WORKSPACE_DIR
cmake -DIGC_OPTION__ENABLE_OCLOC_LIT_TESTS=ON -DSPIRV_SKIP_EXECUTABLES=OFF ../igc
make check-ocloc -j`nproc`
```

Also note that these tests require **Debug** IGC build.

***

## LLVM/LLD/Clang version information

### Version Overview

| Version          | Product quality |
|:----------------:|-----------------|
| LLVM 16/Clang 16          |  Experimental    |
| LLVM 15/Clang 15          | **Production**   |
| LLVM 14/Clang 14 and older| Experimental     |

| Terminology       | Description |
|-------------------|-|
| Production        | Current production version; Builds are verified internally |
| Experimental      | No quality expectations |

### Revision table

LLVM version determines what branches are used when building dependencies.
When checking out the components refer to the following table, replace **XX** with the LLVM version used:

| Repository name       | Version specific | Branch               | LLVM 15 example  |
|-----------------------|:----------------:|----------------------|------------------|
| llvm-project          | -                | release/**XX**.x     | release/15.x     |
| vc-intrinsics         | no               | master               | master           |
| SPIRV-Tools           | no               | master               | master           |
| SPIRV-Headers         | no               | master               | master           |
| SPIRV-LLVM-Translator | yes              | llvm_release_**XX**0 | llvm_release_150 |
| opencl-clang          | yes              | ocl-open-**XX**0     | ocl-open-150     |
