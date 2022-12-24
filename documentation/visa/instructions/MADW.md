<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  MADW = 0x91

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x91(MADW) | Exec_size | Pred | Dst | Src0 | Src1 | Src2 |


## Semantics


```
            for (i = 0; i < exec_size; ++i){
              if (ChEn[i]) {
                dst[i] = src0[i] * src1[i] + src2[i]; // dst0[i] = Low32(dst[i]), dst1[i] = High32(dst[i])
              }
            }
```

## Description





```
    Performs component-wise multiply add of <src0>, <src1>, and <src2> and stores the 64-bit results in <dst>. This is used to produce full results of D/UD interger
    multiply add.

    Dst stores the full 64-bit of the results of multiplying two 32-bit integers and adding 32-bit integer(32b*32b+32b->64b). The low 32b of results are
    stored in the lower GRFs and the high 32b of results are stored in the high GRFs.

    It should be mapped to GEN mul/mach/addc/add instructions. Specifically:
        if src2 is not imme0, then expand MADW((dst_hi32, dst_lo32) = src0 * src1 + src2) to:
            mul  (16) acc0.0<1>:d    src0<1;1,0>:d    src1<2;1,0>:uw
            mach (16) dst_hi32<1>:d  src0<1;1,0>:d    src1<1;1,0>:d
            addc (16) dst_lo32<1>:d  acc0.0<1;1,0>:d  src2<1;1,0>:d     // Low 32 bit results. dst_lo32 is GRF-aligned.
            add  (16) dst_hi32<1>:d  acc0.0<1;1,0>:d  dst_hi32<1;1,0>:d // High 32 bit results. dst_hi32 is GRF-aligned.
        otherwise, expand to:
            mul  (16) acc0.0<1>:d    src0<1;1,0>:d    src1<2;1,0>:uw
            mach (16) dst_hi32<1>:d  src0<1;1,0>:d    src1<1;1,0>:d // Low 32 bit results. dst_lo32 is GRF-aligned.
            mov  (16) dst_lo32<1>:d  acc0.0<1;1,0>:d                // High 32 bit results. dst_lo32 is GRF-aligned.
```


- **Exec_size(ub):** Execution size

  - Bit[2..0]: size of the region for source and destination operands. Max supported execution size for PVC+ is SIMD16, and for pre-PVC is SIMD8.

    - 0b000:  1 element(scalar)
    - 0b001:  2 elements
    - 0b010:  4 elements
    - 0b011:  8 elements
    - {PVC+}0b100:  16 elements
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


- **Dst(vec_operand):** The destination operand. Must be GRF aligned.. Operand class: general,indirect


- **Src0(vec_operand):** The first source operand. Operand class: general,indirect,immediate


- **Src1(vec_operand):** The second source operand. Operand class: general,indirect,immediate


- **Src2(vec_operand):** The third source operand. Operand class: general,indirect,immediate


#### Properties
- **Supported Types:** D,UD
- **Source Modifier:** arithmetic




## Text
```
[(<P>)] MADW (<exec_size>) <dst> <src0> <src1> <src2>
```

## Notes





    Madw is doing SOA layout, so low-32 and high-32 results are packed. The dst stride should be 1 for better performance. Otherwise, vISA will generate extra mov instructions to fix the dst stride issue.

