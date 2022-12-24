<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  DPASW = 0x1c

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x1c(DPASW) | Exec_size | Dst | Src0 | Src1 | Src2 | W |
|             | A         | SD  | RC   |      |      |   |


## Semantics


```

       DPASW is a DPAS wide instruction, a variant of DPAS. It is a specific instruction
       for XEHP when EU fusion is present. All the fields are defined the same as in DPAS.
       Refer DPAS for field definition. Here only difference from DPAS is described.

       DPAS wide differs from DPAS on that DPASW shares the data contents of the src2 register
       read from the GRF of one of the fused EUs among the two fused DPAS pipelines in a Fusion
       thread group. In doing so, the GRF bandwidth requirement is reduced for dpas pipe. For this
       to work, hardware threads must be paired. In OpenCL terminology, this means that the number
       of threads within a work group must be an even number.

       Let (EU0, EU1) is the paired fusion thread group. The values of **Src2** is defined as below.
       Note that if both EUs provide the same amount of data, which one is EU0 does not matter. But
       if they provide a different amount of data, which one is EU0 should matter, and thread ID
       could be used to decide which one is EU0 (need to verify).

       Src2SizeInBytes = ((Src2PrecisionInBits * OPS_PER_CHAN)/8) * 8 * RC;
                       = (Src2PrecisionInBits * OPS_PER_CHAN) * RC;
       NGrf = ((Src2SizeInBytes + 32 - 1) / 32;
       NGrf_EU0 = (NGrf + 1)/2;
       NGrf_EU1 = NGrf - NGrf_EU0;

       for (i=0; i < NGrf_EU0; ++i)
          Src2.R[i] = EU0.Src2.R[i];
       for (i=0; i < NGrf_EU1; ++i)
          Src2.R[NGrf_EU0 + i] = EU1.Src2.R[i];

       Using Src2, the DPAS algorithm works exactly for DPASW. Refer DPAS for details.
```

## Description






       Here only Src2 sharing is shown. The table lists what is provided from which EU.
       All the other should be the same as DPAS. Refer DPAS for detail.

      .. table::Src2 data provided by EU0 and EU1
        :align: center

      +-----+------------------------------------------------------------------------------------+
      | RC  |                         Src2.type                                                  |
      +-----+---------------------------+----------------------------+---------------------------+
      |     | | bf, hf,                 | | 4-bit & OPS_PER_CHAN=4,  |  | 2-bit & OPS_PER_CHAN=4 |
      |     | | 8-bit & OPS_PER_CHAN=4, | | 2-bit & OPS_PER_CHAN=8   |                           |
      |     | | 4-bit & OPS_PER_CHAN=8  |                            |                           |
      +-----+---------------------------+----------------------------+---------------------------+
      |  8  | Src2.R[0] = EU0.Src2.R[0] | Src2.R[0] =  EU0.Src2.R[0] | Src2.R[0] = EU0.Src2.R[0] |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[1] = EU0.Src2.R[1] | Src2.R[1] =  EU0.Src2.R[1] | Src2.R[1] = EU1.Src2.R[0] |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[2] = EU0.Src2.R[2] | Src2.R[2] =  EU1.Src2.R[0] |                           |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[3] = EU0.Src2.R[3] | Src2.R[3] =  EU1.Src2.R[1] |                           |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[4] = EU1.Src2.R[0] |                            |                           |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[5] = EU1.Src2.R[1] |                            |                           |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[6] = EU1.Src2.R[2] |                            |                           |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[7] = EU1.Src2.R[3] |                            |                           |
      +-----+---------------------------+----------------------------+---------------------------+
      |  7  | Src2.R[0] = EU0.Src2.R[0] | Src2.R[0] =  EU0.Src2.R[0] | Src2.R[0] = EU0.Src2.R[0] |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[1] = EU0.Src2.R[1] | Src2.R[1] =  EU0.Src2.R[1] | Src2.R[1] = EU1.Src2.R[0] |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[2] = EU0.Src2.R[2] | Src2.R[2] =  EU1.Src2.R[0] |                           |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[3] = EU0.Src2.R[3] | Src2.R[3] =  EU1.Src2.R[1] |                           |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[4] = EU1.Src2.R[0] |                            |                           |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[5] = EU1.Src2.R[1] |                            |                           |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[6] = EU1.Src2.R[2] |                            |                           |
      +-----+---------------------------+----------------------------+---------------------------+
      |  6  | Src2.R[0] = EU0.Src2.R[0] | Src2.R[0] =  EU0.Src2.R[0] | Src2.R[0] = EU0.Src2.R[0] |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[1] = EU0.Src2.R[1] | Src2.R[1] =  EU0.Src2.R[1] | Src2.R[1] = EU1.Src2.R[0] |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[2] = EU0.Src2.R[2] | Src2.R[2] =  EU1.Src2.R[0] |                           |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[3] = EU1.Src2.R[0] |                            |                           |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[4] = EU1.Src2.R[1] |                            |                           |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[5] = EU1.Src2.R[2] |                            |                           |
      +-----+---------------------------+----------------------------+---------------------------+
      |  5  | Src2.R[0] = EU0.Src2.R[0] | Src2.R[0] =  EU0.Src2.R[0] | Src2.R[0] = EU0.Src2.R[0] |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[1] = EU0.Src2.R[1] | Src2.R[1] =  EU0.Src2.R[1] | Src2.R[1] = EU1.Src2.R[0] |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[2] = EU0.Src2.R[2] | Src2.R[2] =  EU1.Src2.R[0] |                           |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[3] = EU1.Src2.R[0] |                            |                           |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[4] = EU1.Src2.R[1] |                            |                           |
      +-----+---------------------------+----------------------------+---------------------------+
      |  4  | Src2.R[0] = EU0.Src2.R[0] | Src2.R[0] =  EU0.Src2.R[0] | Src2.R[0] = EU0.Src2.R[0] |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[1] = EU0.Src2.R[1] | Src2.R[1] =  EU0.Src2.R[1] |                           |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[2] = EU1.Src2.R[0] |                            |                           |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[3] = EU1.Src2.R[1] |                            |                           |
      +-----+---------------------------+----------------------------+---------------------------+
      |  3  | Src2.R[0] = EU0.Src2.R[0] | Src2.R[0] =  EU0.Src2.R[0] | Src2.R[0] = EU0.Src2.R[0] |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[1] = EU0.Src2.R[1] | Src2.R[1] =  EU0.Src2.R[1] |                           |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[2] = EU1.Src2.R[0] |                            |                           |
      +-----+---------------------------+----------------------------+---------------------------+
      |  2  | Src2.R[0] = EU0.Src2.R[0] | Src2.R[0] =  EU0.Src2.R[0] | Src2.R[0] = EU0.Src2.R[0] |
      |     +---------------------------+----------------------------+---------------------------+
      |     | Src2.R[1] = EU1.Src2.R[0] |                            |                           |
      +-----+---------------------------+----------------------------+---------------------------+
      |  1  | Src2.R[0] = EU0.Src2.R[0] | Src2.R[0] =  EU0.Src2.R[0] | Src2.R[0] = EU0.Src2.R[0] |
      +-----+---------------------------+----------------------------+---------------------------+


      From this table, we can see that some of configurations do not share data from both EUs.
      Thus, when using this instruction, make sure to use only configurations that do share Src2 data.


- **Exec_size(ub):** Execution size

  - Bit[2..0]: size of the region for source and destination operands

    - {XEHP}0b011:  8 elements
    - {PVC}0b100:  16 elements
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

- **Dst(raw_operand):** The destination operand. Must have type D,UD,F


- **Src0(raw_operand):** Its type is D, UD, or F (should be the same as Dst's). It could be a null operand, meaning it's zero


- **Src1(raw_operand):** The field **W** further defines its element precision. Must have type D,UD


- **Src2(vec_operand):** The field **A** defines its  element precision. Must have type D,UD. Operand class: general


- **W(ub):** Integer operand precision for Src1


- **A(ub):** Integer operand precision for Src2


- **SD(ub):** Systolic depth


- **RC(ub):** Repeat Count


#### Properties
- **Source Modifier:** false




## Text
```




      DPASW.W.A.SD.RC    (Exec_size) <dst> <src0> <src1> <src2>
```
## Notes






      Refer DPAS for details. Note that PVC does not have DPASW.

