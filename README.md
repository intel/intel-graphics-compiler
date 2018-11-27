# Intel(R) Graphics Compiler for OpenCL(TM)

## Introduction

The Intel(R) Graphics Compiler for OpenCL(TM) is an llvm based compiler for
OpenCL(TM) targeting Intel Gen graphics hardware architecture.

Please refer to http://01.org/compute-runtime for additional details regarding
 Intel's motivation and intentions wrt OpenCL support in the open source.


## License

The Intel(R) Graphics Compute Runtime for OpenCL(TM) is distributed under the MIT.

You may obtain a copy of the License at:

https://opensource.org/licenses/MIT

## Dependencies

* Common Clang - https://github.com/intel/opencl-clang
* Clang Source - https://github.com/llvm-mirror/clang
* Khronos OpenCL Headers - https://github.com/KhronosGroup/OpenCL-Headers
* LLVM Source -  https://github.com/llvm-mirror/llvm

## Supported Linux versions

IGC is supported on the following 32/64 bits Linux operating systems:

* Ubuntu 14.04, 16.04, 17.04

## Building

1. Install prerequisites

Building IGC needs flex, bison, clang 4.0, cmake version later than 3.4.3 and
 libz.  You can install required packages on ubuntu 16.04 like below:
```
$ sudo apt-get install flex bison clang-4.0 libz-dev cmake
```

2. Download all dependencies and create workspace folder as below:
```
workspace
      |- clang_source           https://github.com/llvm-mirror/clang
      |- common_clang           https://github.com/intel/opencl-clang
      |- llvm_patches           https://github.com/intel/llvm-patches
      |- llvm_source            https://github.com/llvm-mirror/llvm
      |- igc                    https://github.com/intel/intel-graphics-compiler
      |- opencl_headers         https://github.com/KhronosGroup/OpenCL-Headers

Example command:
$ git clone -b release_40 https://github.com/llvm-mirror/clang clang_source
$ git clone https://github.com/intel/opencl-clang common_clang
$ git clone https://github.com/intel/llvm-patches llvm_patches
$ git clone -b release_40 https://github.com/llvm-mirror/llvm llvm_source
$ git clone https://github.com/intel/intel-graphics-compiler igc
$ git clone https://github.com/KhronosGroup/OpenCL-Headers opencl_headers
```

3. Under workspace create a build folder.  For example:
```
$ mkdir build
```

4. Build IGC using commands:
```
$ cd build
$ cmake -DIGC_OPTION__OUTPUT_DIR=../igc-install/Release \
    -DCMAKE_BUILD_TYPE=Release -DIGC_OPTION__ARCHITECTURE_TARGET=Linux64 \
    ../igc/IGC
$ make -j`nproc`
```

## Supported Platforms

* Intel Core Processors supporting Gen8 graphics devices
* Intel Core Processors supporting Gen9 graphics devices
* Intel Core Processors supporting Gen10 graphics devices
* Intel Atom Processors supporting Gen9 graphics devices

## How to provide feedback
Please submit an issue using native github.com interface: https://github.com/intel/intel-graphics-compiler/issues.

## How to contribute

Create a pull request on github.com with your patch. Make sure your change is
cleanly building. A maintainer will contact you if there are questions or concerns.

