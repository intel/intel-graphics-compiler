<!---======================= begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

# VCB (VC BiF Compiler)

VCB is a tool for precompilation of BiF Modules. The primary usage is
to precompile emulation BiF.

VCB is statically linked with VCCodeGen library, so it can access
target-specific transformations.

It can be extended to other types of BiF Modules if necessary.

Currently, there are 2 primary modes of operation:

1. Compilation of input LLVM IR for a particular target.
Command line usage:

```
vcb -o out.bc -cpu <PLATFORM> <INPUT_FILE>

# PLATFORM - name of target (SKL, BDW, etc).
# INPUT_FILE - path to generic bitcode file.
```


2. Generation of code that allows VC compiler to obtain target-specific
precompiled emulation library.

```
vcb -BiFUnique -o out.cpp --symb <BifSymbol> <CONFIG_PATH>

# CONFIG_PATH - path to configuration file that contain information about
#               where to find platform-speicifc BiF for the requested
#               architectures.
# BifSymbol - prefix to use by internal variables in the generated code.
```

