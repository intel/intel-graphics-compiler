This directory contains builtin implementations for Joint/Cooperative Matrix SYCL API.
These are internal builtins and are not meant to be used directly from outside the IGC. JointMatrixFuncsResolutionPass.cpp translates external JointMatrix/CooperativeMatrix API calls into these builtins.

# Links
SYCL API documentation: [sycl_ext_intel_matrix](https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_matrix/sycl_ext_intel_matrix.asciidoc), [sycl_ext_oneapi_matrix](https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_matrix/sycl_ext_oneapi_matrix.asciidoc)

End to end SYCL tests: [test-e2e/Matrix](https://github.com/intel/llvm/tree/sycl/sycl/test-e2e/Matrix)

# Files in this directory
## IBiF_matrix.cl
This file contains OpenCL C builtins with hand-coded implementations.

## IBiF_matrix_generator.cpp
This file is a compile-time generator of OpenCL C matrix load and store builtins. Previously, these builtins were hand-coded with the help of C-like macros.
Due to the combinatory explosion of possible combinations, these macros became complex and were replaced with a separate this generator.
This transition brings us closer to a possible future where we could generate matrix load/store implementations directly in JointMatrixFuncsResolutionPass.cpp using llvm IR builder.

### Working with IBiF_matrix_generator.cpp
The output file from this generator is not committed to the Git repository.
This generator is run automatically when IGC is built.

On Linux, we can expect its output to be generated under the path:
`build/igc/BiFModule/Languages/OpenCL/PreRelease/Matrix/IBiF_matrix_generated.h`

### Running IBiF_matrix_generator.cpp locally
To test generator changes locally without building the entire IGC project, use the following commands.
#### Linux
```
gcc IBiF_matrix_generator.cpp && ./a.out
```
```
clang IBiF_matrix_generator.cpp && ./a.out
```
#### Windows
```
cl IBiF_matrix_generator.cpp && IBiF_matrix_generator.exe
```
#### clang-format
To make inspecting the output file `IBiF_matrix_generated.h` easier it's recommended to run it through `clang-format` utility.
```
clang-format -i IBiF_matrix_generated.h
```
