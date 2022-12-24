<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  FBL = 0x2e

## Format

| | | | | |
| --- | --- | --- | --- | --- |
| 0x2e(FBL) | Exec_size | Pred | Dst | Src0 |


## Semantics


```

                    for (i = 0; i < exec_size; ++i) {
                      if (ChEn[i]) {
                        UD cnt = 0;
                        UD udScalar = src0[i];
                        while ( (udScalar & 1 ) == 0 && cnt != 32 ) {
                          cnt ++;
                          udScalar = udScalar >> 1;
                        }
                        if ( src0[i] == 0x0 ) {
                          dst[i] = 0xFFFFFFFF;
                        else {
                          dst[i] = cnt;
                        }
                      }
                    }
```

## Description





```
    Performs component-wise counting of the trailing zeros in <src0> and stores the result into <dst>. If src0 has no bit set, the value 0xFFFFFFFF is returned.
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


#### Properties
- **Supported Types:** UD
- **Source Modifier:** false




## Text
```
[(<P>)] FBL (<exec_size>) <dst> <src0>
```

## Notes





