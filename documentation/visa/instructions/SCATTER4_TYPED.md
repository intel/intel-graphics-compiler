<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  SCATTER4_TYPED = 0x4c

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x4c(SCATTER4_TYPED) | Exec_size | Pred | Channels | Surface | U | V |
|                      | R         | LOD  | Src      |         |   |   |


## Semantics


```

      UD ch_pos = 0;
      for (c = 0; c < 4; ++c) {
        if (ch_mask[c] == 1) {
          for (i = 0; i < exec_size; ++i) {
            if (ChEn[i]) {
              UD ch_start = ch_pos * max(exec_size, GRF_SIZE / 4);  // GRF_SIZE is the register size in bytes
              *(&surface[u][v][r] + c) = src[ch_start+i]; //one pixel, with type conversion
            }
          }
          ch_pos++;
        }
      }
```

## Description





```
    Performs <exec_size>*<num_enabled_channels> dword scattered write into <surface>, using the values from <src>.
```


- **Exec_size(ub):** Execution size

  - Bit[2..0]: size of the region for source and destination operands

    - 0b011:  8 elements
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


- **Channels(ub):**

  - Bit[3..0]: determines the write masks for the RGBA channel, with R being bit 0 and A bit 3. At least one channel must be enabled (i.e., "0000" is not allowed)


- **Surface(ub):** Index of the surface variable.  It must be a 1D, 2D, or 3D surface.

            - T0 (SLM): no
            - T5 (stateless): no

- **U(raw_operand):** The first exec_size elements contain the U offset. Must have type UD


- **V(raw_operand):** The first exec_size elements contain the V offset. Must have type UD


- **R(raw_operand):** The first exec_size elements contain the R offset. Must have type UD


- **LOD(raw_operand):** The first exec_size elements contain the LOD. Must have type UD


- **Src(raw_operand):**  The values to be written. For each enabled channel in RGBA order, exec_size elements will be written to the surface subject to predication. The next enabled channel will get its data from the next register. Must have type UD,D,F


#### Properties
- **Out-of-bound Access:** On write: data is dropped.




## Text
```



    [(<P>)] SCATTER4_TYPED.<channels> (<exec_size>) <surface> <u> <v> <r> <lod> <src>

    //<channels> is one of R, G, B, A, RG, RB, RA, RGB, RGBA, GB, GA, GBA, BA
```
## Notes






    .. table:: Type conversion from register to surface is performed based on the following rules:
      :align: center

      +---------------------+------------------------+--------------------------------------------------------------------------------------------------+
      | Src Data Type       | Surface Format Type    | Write Conversion                                                                                 |
      +---------------------+------------------------+--------------------------------------------------------------------------------------------------+
      |         F           |           FLOAT        |IEEE float conversion. Round to even and denormalize if destination is narrower.                  |
      +---------------------+------------------------+--------------------------------------------------------------------------------------------------+
      |         F           |     SNORM, UNORM       |Convert IEEE float to fixed point. Round to even and clamp to min/max if destination is narrower. |
      +---------------------+------------------------+--------------------------------------------------------------------------------------------------+
      |         D           |         SINT           |Clamp to min/max if destination is narrower.                                                      |
      +---------------------+------------------------+--------------------------------------------------------------------------------------------------+
      |         UD          |         UINT           |Clamp to min/max if destination is narrower.                                                      |
      +---------------------+------------------------+--------------------------------------------------------------------------------------------------+


    The behavior is undefined if more than one channel writes to the same address.
    If an offset operand is not applicable for the surface accessed (e.g., V and R for 1D surfaces), it should be set to V0 (the null variable).

