<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

 

## Opcode

  SVM = 0x4e

## Format

| | |
| --- | --- |
| 0x4e(SVM) | SVM_ID | various |


## Semantics




                    Share virtual memory access

## Description


   This instruction encompasses a set of shared virtual memory accesses.  Each operation is distinguished by their SVM operation id, which immediately follows the SVM opcode.  Behavior for out-of-bounds SVM access is implementation-defined.

- **SVM_ID(ub):** Id for the SVM operation

- **various(unknown):** see each operation for details

#### Properties




## Notes


