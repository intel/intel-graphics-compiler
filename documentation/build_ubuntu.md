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
Every LLVM/Clang version brings some restrictions and in some cases needs different configuration. Please refer to [LLVM/Clang caveats](#LLVM/Clang-version-specific-caveats) section for more information.

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

Download all dependencies and create workspace folder as below:
```
<workspace>
      |- igc                               https://github.com/intel/intel-graphics-compiler
      |- vc-intrinsics                     https://github.com/intel/vc-intrinsics
      |- llvm_patches                      https://github.com/intel/llvm-patches
      |- llvm-project                      https://github.com/llvm/llvm-project
            |- llvm/projects/opencl-clang  https://github.com/intel/opencl-clang
            |- llvm/projects/llvm-spirv    https://github.com/KhronosGroup/SPIRV-LLVM-Translator
            |- llvm/tools/clang            
```

You can use following commands:
```shell
$ cd <workspace>
$ git clone https://github.com/intel/vc-intrinsics vc-intrinsics
$ git clone -b release/10.x https://github.com/llvm/llvm-project llvm-project
$ git clone -b ocl-open-100 https://github.com/intel/opencl-clang llvm-project/llvm/projects/opencl-clang
$ git clone -b llvm_release_100 https://github.com/KhronosGroup/SPIRV-LLVM-Translator llvm-project/llvm/projects/llvm-spirv
$ git clone https://github.com/intel/llvm-patches llvm_patches
$ mv llvm-project/clang llvm-project/llvm/tools/
```

Make sure to specify correct branch for desired version. In this example we use LLVM10/Clang10.
All dependencies will be build in the next step.

If you have problems with LLVM patching during IGC build, you can try listed steps:
```shell
$ cd llvm-project
$ git checkout -b llvmorg-10.0.0
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

## LLVM/Clang version specific caveats

If you are using LLVM version < 9 be sure to provide cmake an option disabling VC
```shell
$ cmake .... -DIGC_BUILD__VC_ENABLED=OFF
```

### LLVM7/Clang7

In the **OpenCL Clang** project there are patches for Clang.
If the Clang you are using to build IGC does not have these patches (for example, when you are using prebuilt packages) it is necessary to add ```-DVME_TYPES_DEFINED=FALSE``` to IGC cmake flags.

### LLVM8/Clang8

We recommend building LLVM8/Clang8 from sources instead for using prebuilds, because packaged Clang8 is missing these patches:
* [0001-OpenCL-Change-type-of-block-pointer-for-OpenCL.patch](https://github.com/intel/opencl-clang/blob/ocl-open-80/patches/clang/0001-OpenCL-Change-type-of-block-pointer-for-OpenCL.patch)
* [0002-OpenCL-Simplify-LLVM-IR-generated-for-OpenCL-blocks.patch](https://github.com/intel/opencl-clang/blob/ocl-open-80/patches/clang/0002-OpenCL-Simplify-LLVM-IR-generated-for-OpenCL-blocks.patch)
* [0003-OpenCL-Fix-assertion-due-to-blocks.patch](https://github.com/intel/opencl-clang/blob/ocl-open-80/patches/clang/0003-OpenCL-Fix-assertion-due-to-blocks.patch)

which are needed for [enqueue_kernel](https://www.khronos.org/registry/OpenCL/sdk/2.0/docs/man/xhtml/enqueue_kernel.html).
	
### LLVM9/Clang9

No additional steps are needed.

### LLVM10/Clang10

No additional steps are needed.

### LLVM11/Clang11 (TBD)

You can either use prebuilt packages or build from sources:
```shell
$ cd <workspace>
$ git clone https://github.com/intel/vc-intrinsics vc-intrinsics
$ git clone -b release/11.x https://github.com/llvm/llvm-project llvm-project
$ git clone -b ocl-open-110 https://github.com/intel/opencl-clang llvm-project/llvm/projects/opencl-clang
$ git clone -b llvm_release_110 https://github.com/KhronosGroup/SPIRV-LLVM-Translator llvm-project/llvm/projects/llvm-spirv
$ git clone https://github.com/intel/llvm-patches llvm_patches
$ git clone https://github.com/intel/intel-graphics-compiler igc
$ mv llvm-project/clang llvm-project/llvm/tools/
```

Keep in mind that this configuration is experimental and problems with compilation and functionality are to be expected.

Latest known configuration that compiles successfully:

```
<workspace>
      |- igc                               (master TBD)
      |- vc-intrinsics                     (master c8c52b5fb14b33e32de9df573b7de186a0c97c94)
      |- llvm_patches                      (master c4a03454d55ed786138128bfd73fba491b31ca7c)
      |- llvm-project                      (release/11.x 0b56e5490dc33e4e7a4fdd837e642f72a2659189)
            |- llvm/projects/opencl-clang  (ocl-open-110 05133d9fb4e08b8d3251d6484a3bf2da4c4f62ed)
            |- llvm/projects/llvm-spirv    (llvm_release_110 73dfc6d450ad0438558bee20505c426319f7624b)
            |- llvm/tools/clang            (release/11.x 0b56e5490dc33e4e7a4fdd837e642f72a2659189)
```
