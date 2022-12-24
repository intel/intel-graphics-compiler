<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  SRND = 0x1e

## Format

| | | | | | |
| --- | --- | --- | --- | --- | --- |
| 0x1e(SRND) | Exec_size | Pred | Dst | Src0 | Src1 |


## Semantics


```

                    for (i = 0; i < exec_size; ++i){
                      dst[i] = stochasticRounding(src0[i], src1[i]);
                    }
```

## Description





```
  Supported rounding:  HF -> BF8;  F -> HF.  Note that BF8 is denoted as UB as vISA has no BF8 type.

  The srnd instruction takes component-wise source data and performs rounding with a random number. The key of this rounding algorithm is to add a random number to the source data mantissa and get an intermedia result. The intermediate result (mantissa)will be normalized and truncate to get detination mantissa and exponent.

  The acutal random bit used in HW is Src1[7:0] for HF to BF8 and Src1[12:0] for F to HF, respectively. Those 8 or 13 bits are added to mantissa of Src0 (Src1[0:0] alignes with Mantisa[0:0]). NaN and Inf handling conforms to IEEE. Denoms are retained always.

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


- **Dst(vec_operand):** The destination operand. Operand class: general


- **Src0(vec_operand):** The operand to be rounded. Operand class: general


- **Src1(vec_operand):** Random number used for rounding. Operand class: general,immediate


#### Properties
- **Supported Types:**  F, HF,UB
- **Source Modifier:** false


#### Operand type maps
- **Type map**
  -  **Dst types:** HF
  -  **Src types:** F
- **Type map**
  -  **Dst types:** UB
  -  **Src types:** HF


## Text
```
SRND (<exec_size>) <dst> <src0> <src1>
```

## Notes





  - Dst type is eithetr UB (as BF8) or HF; Src0 and Src1 have the same type: HF or F.
  - No predicate, no saturation, no source modifier, no mask.

