<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  RT_WRITE = 0x71

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x71(RT_WRITE) | Exec_size | Pred | Mode      | Surface      | Header  | SI |
|                | CPS       | RTI  | NumParams | Source0Alpha | oMask   | R  |
|                | G         | B    | A         | SourceDepth  | Stencil |    |


## Semantics


```

    Render target write
```

## Description






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


- **Mode(uw):**  Various bits control the parameters of the RT write.

  - Bit[1..0]: MBZ

  - Bit[2]: "Render Target Index" whether RTI is present. RTI is zero if this bit is not set.

  - Bit[3]: "s0a" whether source0 alpha is present

  - Bit[4]: "oM" whether oMask is present

  - Bit[5]: "z" hether depth is present

  - Bit[6]: "s" whether stencil is present

  - Bit[7]: "LRTW" whether the message is the last RT write.

  - Bit[8]: "CPS" whether CPS counter is enabled

  - Bit[9]: "PS" is Per Sample Bit is set

  - Bit[10]: "CM" Is Coarse Mode is set

  - Bit[11]: "SI" whether Sample Index should be used.


  - Bit[12]: "nullRT" whether null render target is set


- **Surface(ub):** Surface variable index


- **Header(raw_operand):** Header of the RT write message


- **SI(vec_operand):** SI Index. Present only of SI bit is set in Mode.


- **CPS(vec_operand):** CPS Counter. Present only if CPS bit is set in Mode


- **RTI(scalar):** Render target index. Present only if RTI bit is set in mode. Must have type UB. Valid values are  [0-7]


- **NumParams(ub):** Number of operands in to RT Write


- **Source0Alpha(raw_operand):** source 0 alpha. Present only if the "s0a" bit is set. Must have type HF,F


- **oMask(raw_operand):** oMask. Present only if the "oM" bit is set. Must have type UW


- **R(raw_operand):** red color values. Must have type HF,F


- **G(raw_operand):** green color values. Must have type HF,F


- **B(raw_operand):** blue color values. Must have type HF,F


- **A(raw_operand):** alpha color values. Must have type HF,F


- **SourceDepth(raw_operand):** Present only if the "z" bit is set. Must have type F


- **Stencil(raw_operand):** Present only if the "s" bit is set. The stencil values are packed into the first dword (and second dword for simd16). Must have type UB


#### Properties




## Text
```





[(<P>)] RT_WRITE[.<Mode>] (<Exec_size>) <Surface> [<CPS>] [<RT_index>] [<sOA>] [<oM>] <R> <G> <B> <A> [<Depth>] [<Stencil>]

// <Mode> is of the form [<A><O><CPS><PS><CM><SI><ST><LRTW><RTI><Z><NULLRT>] and may be in any order
```
## Notes





```
        Some of the operands (after NumParams) are present only if their corresponding bit in <mode> is set. All operands except for oM, z, and s must have the same data type; the behavior is undefined if some operands have HF type and others have F type.
```

