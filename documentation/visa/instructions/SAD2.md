<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  SAD2 = 0x16

## Format

| | | | | | |
| --- | --- | --- | --- | --- | --- |
| 0x16(SAD2) | Exec_size | Pred | Dst | Src0 | Src1 |


## Semantics


```

                    for (i = 0; i < exec_size; i += 1){
                      if (ChEn[i]) {
                        dst[i] = abs( src0[i] - src1[i] ) + abs ( src0[i+1] - src1[i+1] );
                        dst[i+1] = undefined;
                      }
                    }
```

## Description





```
    Performs a two-wide sum-of-absolute-difference operation on a 2-tuple basis of <src0> and <src1>, and stores the scalar result to the first channel per 2-tuple in <dst>.
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


- **Dst(vec_operand):** The destination operand. Must have type W,UW. Operand class: general,indirect


- **Src0(vec_operand):** The first source operand. Must have type B,UB. Operand class: general,indirect,immediate


- **Src1(vec_operand):** The second source operand. Must have type B,UB. Operand class: general,indirect,immediate


#### Properties
- **Saturation:** Yes
- **Source Modifier:** arithmetic




## Text
```
[(<P>)] SAD2[.sat] (<exec_size>) <dst> <src0> <src1>
```

## Notes





```
    The even elements of the <dst> will contain the correct data, while the odd element are updated with undefined values.
```

