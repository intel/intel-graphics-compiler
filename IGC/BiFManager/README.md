# BiF package

The OpenCL C and SPIR-V built-in functions (BiF) are implemented in OpenCL C
under `BiFModule/`, compiled to LLVM bitcode at build time, and embedded in IGC
as the `OCLBiFImpl.bifbc` resource. When IGC compiles a kernel, `BIImport`
links the built-ins the kernel calls into the kernel module with
`BiFManagerHandler::LinkBiF`.

Every BiF source is a module of its own. Compiling a kernel loads only the
modules that define the built-ins it calls and the modules those depend on.
Loading a module with `getLazyBitcodeModule` parses all of its declarations,
so loading less bitcode is what keeps the import fast.

## Build

`BiFModule/cmake/BiFModuleBuild.cmake` creates the commands that build the
package:

1. `BiFMCRestore.cmake` restores the package from the prebuild cache if the
   cache has it for the checksum of the sources and tools. The steps below
   then do nothing (`BiFMCRun.cmake`).
2. Every source listed in `BiFModule/cmake/BiFSources.cmake` is compiled to
   its own module, in parallel by the build tool. The sources are in groups,
   for example `IGC_BIF_SPIRV_SOURCES`, and each group has its own triple,
   include directories and defines, and a header force-included into each of
   its sources:

   | Group | Header |
   |---|---|
   | SPIR-V, SPIR-V size_t | `Implementation/IBiF_Header.h` |
   | OpenCL C, pre-release, OpenCL C size_t | `Languages/OpenCL/IBiF_Header.h` |
   | BFloat16 | `Implementation/bfloat/IBiF_BFloat_Header.h` |

   The size_t groups are compiled for pointer size 32 and 64.
3. `BiFManager-bin -symbols` lists the functions and global variables each
   module defines and declares. It loads the modules one at a time, each in
   its own `LLVMContext`, so that it fits into the memory of a 32-bit build.
4. `BiFModule/depgraph.py` builds the dependency graph of the modules: a
   module depends on the modules that define what it declares. The modules for
   both pointer sizes form one graph with the size32 modules and another one
   with the size64 modules. Both graphs must be acyclic; a cycle fails the
   build. It writes the graphs and the sorted hashes of the built-in names
   with their modules to `OCLBiFImpl.h`, which `BiFManagerHandler.cpp`
   includes.
5. `BiFManager-bin -pack` marks every function and global with `!igc_bif`,
   makes them linkonce except the globals used by other modules, mangles the
   names if `BIF_MANGLE` is defined, and writes the modules to
   `OCLBiFImpl.bifbc`.

### Writing BiF sources

- A `.cl` file is a module; a `.h` file is included. Do not `#include` a
  `.cl` file.
- Every source compiles on its own, with only its group header
  force-included. Declarations, macros and `static inline` helpers shared by
  more sources belong in headers. A function used by other sources is defined
  in one source and declared where it is used.
- Keep all overloads of a built-in in one source, the source of its family,
  e.g. every `trunc` in `Implementation/Math/trunc.cl`.
- Sources must not depend on each other in a cycle. If they do, move the
  functions that close the cycle to the source of their family or to a source
  of their own.
- A new source has to be added to its group in `BiFSources.cmake`.
- Clang inlines only within a source. Calls between sources are inlined by
  IGC, which makes every linked built-in always-inline (see
  `ProcessFuncAttributes`).

## Compilation

`BiFManagerHandler::LinkBiF`:

1. finds the functions the kernel calls whose name hash is in `OCLBiFImpl.h`;
2. starting from the modules defining them, follows the dependency graph and
   collects all reachable modules, plus the first module of the kernel's
   pointer size;
3. loads these modules lazily, materializes the functions reachable from the
   kernel, marks them `OclBuiltin`, and drops the other bodies;
4. links the modules into the kernel module with `OverrideFromSrc`.
