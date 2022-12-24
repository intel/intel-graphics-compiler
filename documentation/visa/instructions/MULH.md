<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  MULH = 0x0d

## Format

| | | | | | |
| --- | --- | --- | --- | --- | --- |
| 0x0d(MULH) | Exec_size | Pred | Dst | Src0 | Src1 |


## Semantics


```

                    for (i = 0; i < exec_size; ++i){
                      if (ChEn[i]) {
                        dst[n] = src0[i] * src1[i]; high 32 bits of the result
                      }
                    }
```

## Description





```
    Performs component-wise multiplication of <src0> and <src1> and stores the high dword of the results into <dst>.

    Dst stores the high 32-bit of the results of multiplying two 32-bit integers. Dst, src0, and src1 must have the same integer type.
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


- **Dst(vec_operand):** The destination operand. Operand class: general,indirect


- **Src0(vec_operand):** The first source operand. Operand class: general,indirect,immediate


- **Src1(vec_operand):** The second source operand. Operand class: general,indirect,immediate


#### Properties
- **Supported Types:** D,UD
- **Source Modifier:** arithmetic




## Text
```
[(<P>)] MULH (<exec_size>) <dst> <src0> <src1>
```

## Notes





    This is used to together with the mul instruction to produce the full results of UD/D integer multiplication.  It should be mapped to the GEN mach instruction.

