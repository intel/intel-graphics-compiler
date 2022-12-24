<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  FCCALL = 0x33

## Format

| | | | |
| --- | --- | --- | --- |
| 0x33(FCCALL) | Exec_size | Pred | Label |


## Semantics




          FC Calls to a kernel identified as label, which is resolved by FC linker. Note that FCCALL is an alias to CALL (same opcode).

## Description





```
    Calls another kernel identified by <label>. FCCALL is the same as CALL with the same opcode(0x33), except that the label of FCCALL is FC label, not subroutine label.


```


- **Exec_size(ub):** Execution size

  - Bit[2..0]: size of the region for source and destination operands

    - 0b000:  1 element (scalar)
    - 0b001:  2 elements
    - 0b010:  4 elements
    - 0b011:  8 elements
    - 0b100:  16 elements
    - 0b101:  32 elements
  - Bit[7..4]: execution mask (explicit control over the enabled channels)

    - 0b0000:  M1
    - 0b0001:  M2
    - 0b0010:  M3
    - 0b0011:  M4
    - 0b0100:  M5
    - 0b0101:  M6
    - 0b0110:  M7
    - 0b0111:  M8
    - 0b1000:  M1_NM
    - 0b1001:  M2_NM
    - 0b1010:  M3_NM
    - 0b1011:  M4_NM
    - 0b1100:  M5_NM
    - 0b1101:  M6_NM
    - 0b1110:  M7_NM
    - 0b1111:  M8_NM

- **Pred(uw):** Predication control


- **Label(uw):** Index of the label variable for FC call's target. It must be a kernel label that is absent in the current compilation unit. FC linker will resolve it later.


#### Properties
- **Source Modifier:** false




## Text
```
FCCALL (<exec_size>) <label>
```

## Notes





    Dynamic call through a variable is not supported.

