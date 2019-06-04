# Intel® Graphics Compiler for OpenCL™

## Ubuntu build section

### 1. Prepare workspace

Building IGC needs flex, bison, libz and cmake version at least 3.4.3. You can install required packages on Ubuntu using this command:

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

In this step you need to prepare LLVM, OpenCL-Clang libraries and Clang for IGC.
It can be done either by using packaged releases or building those yourself:

#### Use preinstalled packages

For **LLVM** and **Clang** packages please visit this [link](https://apt.llvm.org/) to download and install desired version.
As of now **OpenCL Clang** is still needed to be built and installed manually. Sources are available [here](https://github.com/intel/opencl-clang). You can use out-of-tree build method with LLVM and Clang preinstalled.

Installing these three components (LLVM, Clang, and OpenCL Clang) means you no longer have to download their sources alongside IGC, so the workspace tree in the next step may look like this:
```
<workspace>
      |- igc                          https://github.com/intel/intel-graphics-compiler
```

#### Build from sources

Download all dependencies and create workspace folder as below:
```
<workspace>
      |- igc                          https://github.com/intel/intel-graphics-compiler
      |- llvm_patches                 https://github.com/intel/llvm-patches
      |- llvm_source                  https://github.com/llvm-mirror/llvm
            |- projects/opencl-clang  https://github.com/intel/opencl-clang
            |- projects/llvm-spirv    https://github.com/KhronosGroup/SPIRV-LLVM-Translator
            |- tools/clang            https://github.com/llvm-mirror/clang
```

You can use following commands:
```shell
$ cd <workspace>
$ git clone -b release_80 https://github.com/llvm-mirror/llvm llvm_source
$ git clone -b release_80 https://github.com/llvm-mirror/clang llvm_source/tools/clang
$ git clone -b ocl-open-80 https://github.com/intel/opencl-clang llvm_source/projects/opencl-clang
$ git clone -b llvm_release_80 https://github.com/KhronosGroup/SPIRV-LLVM-Translator llvm_source/projects/llvm-spirv
$ git clone https://github.com/intel/llvm-patches llvm_patches
```

Make sure to specify correct branch for desired version. In this example we use LLVM8/Clang8.
All dependencies will be build in the next step.

### 3. Build and install IGC

1. Download sources:
```shell
$ cd <workspace>
$ git clone https://github.com/intel/intel-graphics-compiler igc
  [If using specific release]
$ cd igc && git checkout -b tag igc_release_2019-01-15
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

You can either use prebuilt packages or build from sources:
```shell
$ cd <workspace>
$ git clone -b master https://github.com/llvm-mirror/llvm llvm_source
$ git clone -b master https://github.com/llvm-mirror/clang llvm_source/tools/clang
$ git clone -b master https://github.com/intel/opencl-clang llvm_source/projects/opencl-clang
$ git clone -b master https://github.com/KhronosGroup/SPIRV-LLVM-Translator llvm_source/projects/llvm-spirv
$ git clone https://github.com/intel/llvm-patches llvm_patches
$ git clone https://github.com/intel/intel-graphics-compiler igc
```

Keep in mind that this configuration is experimental and problems with compilation and functionality are to be expected.

Latest known configuration that compiles successfully:

```
<workspace>
      |- igc                          (master/8cb64241)
      |- llvm_patches                 (master/cac8d77)
      |- llvm_source                  (master/541ca56bcf2)
            |- projects/opencl-clang  (master/faa242c)
            |- projects/llvm-spirv    (master/365675f)
            |- tools/clang            (master/203bf9fe94)
```

- https://github.com/intel/intel-graphics-compiler/commit/8cb64241
- https://github.com/intel/llvm-patches/commit/cac8d77
- https://github.com/llvm-mirror/llvm/commit/541ca56bcf2
- https://github.com/intel/opencl-clang/commit/faa242c
- https://github.com/KhronosGroup/SPIRV-LLVM-Translator/commit/365675f
- https://github.com/llvm-mirror/clang/commit/203bf9fe94