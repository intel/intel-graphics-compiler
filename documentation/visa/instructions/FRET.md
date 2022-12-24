<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  FRET = 0x68

## Format

| | | |
| --- | --- | --- |
| 0x68(FRET) | Exec_size | Pred |


## Semantics


```

                    Returns from a function (FCALL).
```

## Description





```
    Conditionally returns from the current function.

    If <exec_size> is one: Execution returns to the caller if predicate is true. Scalar returns must be marked with {NoMask}.

    If <exec_size> is greater than one: Active channels that are predicated have their call mask and execution mask bit turned off. If the call mask becomes all zero, execution returns to the caller and the call mask is restored to its previous value before the call.

    If return is executed, this instruction copies the value of the retval variable (%retval) from the callee to the caller. It also copies the value of stack (%sp) and frame pointer (%fp) from the callee to the caller.
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


#### Properties




## Text
```
[(<P>)] FRET (<exec_size>)
```

## Notes





    Early return is permitted.

