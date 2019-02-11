# Intel(R) Graphics Compiler for OpenCL(TM)

## Introduction

The Intel(R) Graphics Compiler for OpenCL(TM) provides few features:
1. LLVM based runtime compiler ("online" compiler) for OpenCL(TM)
targeting Intel Gen graphics hardware architecture.
2. Compiler tools to generate shaders for Intel Gen graphics hardware architecture from
different representations including, but not limited to, Gen Assembler Code.

Please refer to http://01.org/compute-runtime for additional details regarding
Intel's motivation and intentions wrt OpenCL support in the open source.

## License

The Intel(R) Graphics Compute Runtime for OpenCL(TM) is distributed under the MIT.

You may obtain a copy of the License at:

https://opensource.org/licenses/MIT

## Dependencies

* Cmake (>=3.4.3) - https://cmake.org
* Common Clang - https://github.com/intel/opencl-clang
* LLVM and Clang - https://github.com/llvm/llvm-project

## Building

1. Install LLVM and Clang

Building IGC requires LLVM infrastructure and Clang installed on the system. In case
of self-building of these projects, the minimal build to satisfy IGC dependencies would be:

```sh
git clone https://github.com/llvm/llvm-project.git
mkdir llvm-projects/build && cd llvm-projects/build
cmake -DLLVM_ENABLE_PROJECTS="clang" -DCMAKE_INSTALL_PREFIX=/path/to/install ..
make
make install
```

If LLVM and Clang were installed into custom location, you may need to provide some environment
and/or build variables to properly detect them on the system. For more details, refer to LLVM
documentation: http://llvm.org/docs/GettingStarted.html 

For IGC build powered by cmake, consider to provide:
* LLVM_DIR pointing to the location of LLVMConfig.cmake, typically $PREFIX/lib/cmake/llvm
* PATH pointing to the location of clang-X, where X equals ${LLVM_VERSION_MAJOR} obtained from cmake find_package(LLVM)

2. Install Common Clang

Comon Clang is another major IGC dependency to present on the system. In case of self-building
follow this instruction:

```sh
# build and install common clang dependency
git clone https://github.com/KhronosGroup/SPIRV-LLVM-Translator.git
mkdir SPIRV-LLVM-Translator/build && cd SPIRV-LLVM-Translator
cmake -DLLVM_DIR=$PREFIX/lib/cmake/llvm -DCMAKE_CXX_FLAGS="-fPIC" -DCMAKE_INSTALL_PREFIX=/path/to/install ..
make && make install

# build and install common clang
git clone https://github.com/intel/opencl-clang
mkdir opencl-clang/build && cd opencl-clang/build
cmake -DLLVM_DIR=$PREFIX/lib/cmake/llvm -DCMAKE_INSTALL_PREFIX=/path/to/install ..
make && make install
```

For more details refer to common clang documentation: https://github.com/intel/opencl-clang

3. Build and install IGC

Finally, build and install IGC with:

```sh
git clone https://github.com/intel/intel-graphics-compiler
mkdir intel-graphics-compiler/build && cd intel-graphics-compiler/build
export PATH=$PREFIX/bin:$PATH  # to fetch clang-X
cmake \
  -DLLVM_DIR=$PREFIX/lib/cmake/llvm \
  -DCMAKE_INSTALL_PREFIX=/path/to/install \
  ..
make && make install
```

In case of standard system-wide installation of LLVM and clang, `-DLLVM_DIR` and `export PATH` can be skipped.

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
