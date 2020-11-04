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

### 2. Install LLVM, Clang and OpenCL Clang

> <span style="color: firebrick; font-weight: 600;">Important notice</span>
Every LLVM/Clang version brings some restrictions and in some cases needs different configuration. Please refer to [LLVM/Clang version information](#LLVM/Clang-version-information) section for more information.

In this step you need to prepare VC Intrinsics, SPIRV-LLVM Translator, LLVM, OpenCL-Clang libraries and Clang for IGC.
It can be done either by using packaged releases or building those yourself:

#### Use preinstalled packages

For **LLVM** and **Clang** packages please visit this [link](https://apt.llvm.org/) to download and install desired version.
As of now **OpenCL Clang** is still needed to be built and installed manually. Sources are available [here](https://github.com/intel/opencl-clang). You can use out-of-tree build method with LLVM and Clang preinstalled.
**VC Intrinsics** is a lightweight library that is built from sources with IGC and there is no package for it.
Currently, in this mode **SPIRV-LLVM Translator** sources should be present too.

Installing LLVM, Clang, and OpenCL Clang components means you no longer have to download their sources alongside IGC, so the workspace tree in the next step may look like this:
```
<workspace>
      |- igc                          https://github.com/intel/intel-graphics-compiler
      |- vc-intrinsics                https://github.com/intel/vc-intrinsics
      |- llvm-spirv                   https://github.com/KhronosGroup/SPIRV-LLVM-Translator
```

#### Build from sources

Download all dependencies and create workspace.
You can use following commands for setup:

```shell
$ cd <workspace>
$ git clone https://github.com/intel/vc-intrinsics vc-intrinsics
$ git clone -b release/10.x https://github.com/llvm/llvm-project llvm-project
$ git clone -b ocl-open-100 https://github.com/intel/opencl-clang llvm-project/llvm/projects/opencl-clang
$ git clone -b llvm_release_100 https://github.com/KhronosGroup/SPIRV-LLVM-Translator llvm-project/llvm/projects/llvm-spirv
$ git clone https://github.com/intel/llvm-patches llvm_patches
```
These commands will set up a workspace with LLVM 10. If you wish to use any other version please refer to the [component revision table](#Revision-table)

Correct directory tree looks like this:
```
<workspace>
    |- igc              https://github.com/intel/intel-graphics-compiler
    |- vc-intrinsics    https://github.com/intel/vc-intrinsics
    |- llvm_patches     https://github.com/intel/llvm-patches
    |- llvm-project     https://github.com/llvm/llvm-project
        |- llvm/projects/opencl-clang    https://github.com/intel/opencl-clang
        |- llvm/projects/llvm-spirv      https://github.com/KhronosGroup/SPIRV-LLVM-Translator
```

All dependencies will be build in the next step.

If you have problems with LLVM patching during IGC build, you can try listed steps:
```shell
$ cd llvm-project
$ git checkout -b tag llvmorg-10.0.0
```

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
$ cmake ../igc/IGC
$ make -j`nproc`
```

3. Install IGC:
```shell
$ sudo make install
```

***

## LLVM/Clang version information

### Version Overview

| Version          | Product quality |
|:----------------:|-----------------|
| LLVM 11          | Experimental    |
| LLVM 10          | **Production**  |
| LLVM 9 and older | Experimental    |

| Terminology       | Description |
|-------------------|-|
| Production        | Current production version; Builds are verified internally |
| Experimental      | No quality expectations |

### Revision table

LLVM version determines what branches are used when building dependencies.
When checking out the components refer to the following table, replace **XX** with the LLVM version used:

| Repository name       | Version specific | Branch               | LLVM 10 example  |
|-----------------------|:----------------:|----------------------|------------------|
| llvm-project          | -                | release/**XX**.x     | release/10.x     |
| vc-intrinsics         | no               | master               | master           |
| llvm_patches          | no               | master               | master           |
| SPIRV-LLVM-Translator | yes              | llvm_release_**XX**0 | llvm_release_100 |
| opencl-clang          | yes              | ocl-open-**XX**0     | ocl-open-100     |

### LLVM/Clang version specific caveats

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

#### LLVM9/Clang9

No additional steps are needed.

#### LLVM10/Clang10

No additional steps are needed.

#### LLVM11/Clang11

**This configuration is experimental and problems with compilation and functionality are to be expected.**

Latest known configuration that compiles successfully:

| Component | Branch | Revision |
|-|-|-|
| igc           | master       | 7d11ff43f42564fdfe2753b4d008abfd56ec9671 |
| vc-intrinsics | master       | eabcd2022cf868a658b257b8ea6ad62acbbe7dc5 |
| llvm_patches  | master       | d8b63ab67d688db9e60bca469d58f6aa3ec6b2a1 |
| llvm-project  | release/11.x | llvmorg-11.0.0 |
| opencl-clang  | ocl-open-110 | cdacb8a1dba95e8ebc5d948c0e0e574f87b1e861 |
| SPIRV-LLVM-Translator | llvm_release_110 | d6dc999eee381158a26f048a333467c9ce7e77f2 |
