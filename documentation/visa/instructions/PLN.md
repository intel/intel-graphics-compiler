<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  PLANE = 0x6b

## Format

| | | | | | |
| --- | --- | --- | --- | --- | --- |
| 0x6b(PLANE) | Exec_size | Pred | Dst | Src0 | Src1 |


## Semantics


```

                    for (i = 0; i < exec_size; ++i){
                      if (exec_size == 8) {
                        u = src1[0:7];
                        v = src1[8:15];
                      }
                      else { // exec_size == 16
                      if ( i < 8 ) {
                        u = src1[0:7];
                        v = src1[8:15];
                      }
                      else {
                        u = src1[16:23];
                        v = src1[24:31];
                      }
                      if (ChEn[i]) {
                        dst[i] = src0[0] * u[i] + src0[1] * v[i] + src0[3];
                      }
                    }
```

## Description





```
    Component-wise plane instruction (w = p*u + q*v + r where u and v are vectors and p, q, r are scalars).  <src0> provides the value of the scalars, while <src1> contains both the u (the first 8 elements) and v (the next 8 elements) vector.  For simd16 plane, an additional 16 elements from <src1> will be used for u and v.
```


- **Exec_size(ub):** Execution size

  - Bit[2..0]: size of the region for source and destination operands

    - 0b011:  8 elements
    - 0b100:  16 elements
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


- **Src0(vec_operand):** The first source operand. Operand class: general,indirect


- **Src1(vec_operand):** The second source operand. Operand class: general,indirect


#### Properties
- **Supported Types:** F
- **Saturation:** Yes


#### Operand type maps
- **Type map**
  -  **Dst types:** F
  -  **Src types:** F


## Text
```
[(<P>)] PLANE (<exec_size>) <dst> <src0> <src1>
```

## Notes





    Src0 must be 16 byte aligned, and its region parameters are ignored. Src1 must be GRF-aligned, and its region parameters are ignored.

