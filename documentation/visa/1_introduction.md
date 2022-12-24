<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

# Introduction

Purpose / Scope
===============

This document provides a specification of the common instruction set
architecture (ISA) for Intel Graphics Media Accelerators (GEN)
architecture. The virtual ISA specification defines a portable and
secure byte code intermediate representation (IR) for compilers and
assembly developers that wish to target Gen. High-level compilers for
languages such as C++ and CM compile the source kernel program into
virtual ISA byte code. At runtime, the just-in-time (JIT) virtual ISA
finalizer translates the virtual ISA byte code into native GEN
instructions for GPU execution.

The virtual ISA has the following goals:

-   Provide a portable ISA that spans multiple GEN platforms.
-   Incur low JIT-translation overhead when translating from virtual ISA
    to GEN binary.
-   Achieve performance that is comparable to that of the hand-written
    GEN assembly kernels.
-   Expose most of GEN hardware's capabilities, specifically its shared
    function units such as sampler and video motion estimation.
-   Provide a code distribution format for kernels executing on Gen.

Virtual ISA and Gen
===================

The virtual ISA is designed to be executed on GEN starting from the
Broadwell generation. As such, it makes many assumptions about the
availability of hardware features such as SIMD instructions, regions,
and data port messages. In particular, the virtual ISA adopts the
fundamental data types used in Gen, and most virtual ISA instructions
have a corresponding GEN instruction. Notable differences between a
virtual ISA and a GEN instruction are:

1.  The virtual ISA instructions are variable-length unlike the GEN
    instructions.
2.  The virtual ISA instructions use virtual variables as their
    operands. The general-purpose and architectural registers are
    replaced by variables that are grouped into different categories
    based on their functionality (General, Address, Predicate, etc.),
    and there are generally no limtis on the number of variables.
    Certain architecture registers such as the accumulator are also not
    exposed.
3.  Instead of a single send instruction, virtual ISA provides
    higher-level instructions represent accesses to different functional
    units (load/store, sampler, barrier, etc.). A special raw_send
    instruction can be used to directly access shared function
    capabilities that are not available otherwise.
4.  The virtual ISA relaxes the GEN register region restrictions, in
    particular abstracting away most of the platform specific region
    alignment rules.
5.  Each virtual ISA instruction operand is not associated with a type
    but will instead obtain their type from the variable declarations.

While the virtual ISA specification strives to be platform independent,
not all of its features will be supported on every platform due to
hardware limitations. Such exceptions are marked with the notation
"{platform}" in the specification, where "platform" is the list of GEN
platforms that support the feature. When a virtual ISA program exercises
a feature that is not supported by the target platform, the vISA
finalizer will return an error.

Document Structure
==================

The organization of this document is as follows:

-   Section [1](2_datatypes.md) describes the data types and
    type conversion rules.
-   Section [2](3_execution_model.md) describes the execution model for a virtual
    ISA object.
-   Section [3](4_visa_header.md) describes the virtual ISA object format as
    well as the format of various global symbol tables.
-   Section [4](5_operands.md) describes the format of instruction
    operands.
-   Section [5](6_instructions.md) describes the instruction set of the virtual
-   Section [6](7_appendix_debug_information.md) describes the virtual ISA debug information.
    ISA.
-   Section [7](8_appendix_visa_assembly_syntax.md) describes the virtual ISA assembly syntax.

Definitions, Acronyms, and Abbreviation
=======================================

|     |     |
| --- | --- |
| SIMD | Single Instruction Multiple Data |
| GPU | Graphics Processing Unit |
| GenX | Graphics core generations for Intel Graphics Media Accelerators \[1\] |
| Host | IA-32 and Intel 64 architecture processors |
| Device | Intel GenX GPU |
| Kernel | A program that can be executed on GenX hardware |
| Thread | An instance of a kernel program that is executed on a GenX hardware |
| LSB | Least Significant Bit |
| GRF | General Register File, a set of general-purpose registers available in GenX. |
| DWORD | Double-word, represents 4 bytes for GenX |
| VME | Video Motion Estimation |
| BDW | Intel Broadwell processor microarchitecture |
| SKL | Intel Skylake processor microarchitecture |
| ICLLP | Intel IceLake processor microarchitecture (low power) |
| TGLLP | Intel TigerLake processor microarchitecture (low power) |
 | XEHP | Intel Arctic Sound processor microarchitecture |
  | PVC  | Intel PonteVecchio processor microarchitecture |

References and Related Information
==================================

\[1\] Intel Graphics Media Accelerator Developer's Guide,
<http://software.intel.com/en-us/articles/intel-graphics-media-accelerator-developers-guide/>.

\[2\] United States Patent 7257695, 2007.

\[3\] Intel C++ Compiler User and Reference Guides,
<http://www.intel.com/cd/software/products/asmo-na/eng/347618.htm>.

\[4\] Intel Media SDK Reference Manual, Supplement A: Frame VME
Emulation Library for Intel SNB Graphics, April 8, 2009 (Version 1.21s).

\[5\] CM(C for Metal) Runtime API Specification, Version 3.0.
