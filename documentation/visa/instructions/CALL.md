<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  CALL = 0x33

## Format

| | | | |
| --- | --- | --- | --- |
| 0x33(CALL) | Exec_size | Pred | Label |


## Semantics


```

                    Calls the subroutine at <label>
```

## Description





```
    Calls the subroutine <label> based on the predicate value.

    * If <exec_size> is one:

      * Execution jumps to the subroutine if the predicate is true. The call mask will be initialized to all ones at subroutine entry. Scalar calls must be marked with {NoMask}.

    * If <exec_size> is greater than one:

      * The call is executed if any of the active channels are predicated. At subroutine entry, the call mask will be initialized to the set of active channels that are predicated.

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


- **Label(uw):** Index of the label variable for the subroutine. It must be a function label


#### Properties
- **Source Modifier:** false




## Text
```
[(<P>)] CALL (<exec_size>) <label>
```

## Notes





    Dynamic subroutine call through a variable is not supported.

