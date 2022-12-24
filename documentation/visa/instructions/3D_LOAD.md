<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  3D_LOAD = 0x6e

## Format


### CONDITION

- field_operand_type(Si)=UD
- Op.op=ld2dms_w


| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x6e(3D_LOAD) | Op           | Exec_size | Pred   | Channels | Aoffimmi | Surface |
|               | Dst          | NumParams | Si\*   | Mcsl\*   | Msch\*   | U       |
|               | V            | Lod       | R      |          |          |         |


### CONDITION

- field_operand_type(Si)=UW
- Op.op=ld2dms_w


| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x6e(3D_LOAD) | Op           | Exec_size | Pred   | Channels | Aoffimmi | Surface |
|               | Dst          | NumParams | Si\*   | Msc0\*   | Msc1\*   | Msc2\*  |
|               | Msc3\*       | U         | V      | Lod      | R        |         |


### CONDITION

- Op.op=ld, ld_lz, ld_mcs


| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x6e(3D_LOAD) | Op         | Exec_size | Pred | Channels | Aoffimmi | Surface |
|               | Dst        | NumParams | U    | V        | Lod      | R       |


## Semantics


```

      Load a surface through the sampler interface
```

## Description





```
    Loads data from <surface> at the given integer texel addresses.
```


- **Op(uw):**

  - Bit[7..0]: encodes the sampler operation

    - 0b00000111:  ld
    - 0b00011010:  ld_lz
    - 0b00011100:  ld2dms_w
    - 0b00011101:  ld_mcs
  - Bit[8]: pixel null mask enable. Specifies whether the writeback message will include an extra phase indicating the pixel null mask


- **Exec_size(ub):** Execution size

  - Bit[2..0]: size of the region for source and destination operands

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


- **Channels(ub):**

  - Bit[3..0]: determines the write masks for the RGBA channel, with R being bit 0 and A bit 3. At least one channel must be enabled (i.e., "0000" is not allowed)


- **Aoffimmi(scalar):**  A UW representing the _aoffimmi modifier with the following format:. Must have type UW

  - Bit[3..0]: stores the R offset. Valid values are [-8-7]

  - Bit[7..4]: stores the V offset. Valid values are [-8-7]

  - Bit[11..8]: stores the U offset. Valid values are [-8-7]

  - Bit[15..12]: reserved. Must be zero


- **Surface(ub):** Index of the surface variable


- **Dst(raw_operand):** The result of the sample. Must have type HF,F,W,UW,D,UD


- **NumParams(ub):** number of additional parameters for this instruction. Valid values are  [1-15]


- **Si(raw_operand):** The first <exec_size> elements contain the sample index, which is clamped to the number of samples on the surface. Must have type UD,UW


- **Mcsl(raw_operand):** The first <exec_size> elements contain the low half of the 64-bit  MCS value


- **Msch(raw_operand):** The first <exec_size> elements contain the high half of the 64-bit   MCS value


- **Msc0(raw_operand):** The first <exec_size> elements contain bit[0-15] of the 64-bit MCS value


- **Msc1(raw_operand):** The first <exec_size> elements contain bit[16-31] of the 64-bit MCS value


- **Msc2(raw_operand):** The first <exec_size> elements contain bit[32-47] of the 64-bit MCS  value


- **Msc3(raw_operand):** The first <exec_size> elements contain bit[48-63] of the 64-bit MCS value


- **U(raw_operand):** The first <exec_size> elements contain the X pixel address


- **V(raw_operand):** The first <exec_size> elements contain for


            -  1D surfaces: ignored
            -  1D_array surfaces: the array index
            -  Other surface: the Y pixel address

- **Lod(raw_operand):** The first <exec_size> elements contain the LOD


- **R(raw_operand):** The first <exec_size> elements contain for

            -  2D_array surfaces: the array index
            -  3D surfaces: the Z pixel address.
            -  Other surfaces: ignored

#### Properties




## Text
```




{-PVC}For pre-PVC platforms:

    [(<P>)] LOAD_3D[.pixel_null_mask].<Channels> (Exec_size) <Aoffimmi> <Surface> <Dst> <u> <v> <lod> <r>

    [(<P>)] LOAD_LZ[.pixel_null_mask].<Channels> (Exec_size) <Aoffimmi> <Surface> <Dst> <u> <v> <r>

    [(<P>)] LOAD_MCS[.pixel_null_mask].<Channels> (Exec_size) <Aoffimmi> <Surface> <Dst>  <u> <v> <r> <lod>

    [(<P>)] LOAD_2DMS_W[.pixel_null_mask].<Channels> (Exec_size) <Aoffimmi> <Surface> <Dst> <si> <mcsl> <mcsh> <u> <v> <r> <lod>






    // instruction specific parameters may vary
```
## Notes





```

For each enabled channel <exec_size> elements are returned in RGBA order, with the disabled channels skipped in the results. Only the enabled pixels are returned in <dst>. Each channel's return data start in the next GRF; if <exec_size> * sizeof(dst_type) is smaller than the register size, the remaining portions of the register have undefined values.

For all operations, if <pixel_null_mask> is set, an additional GRF is returned after the sampler data, with <exec_size> bits in the first DWord containing the pixel null mask values. This field has the bit for all pixels set to 1 except those pixels in which a null page was source for at least one texel.


Extra parameters (after NumParams) for this instruction. All operands must have the same type, either UD or UW. Refer to the table below for the order of parameters for each instruction. It is permitted to skip the trailing paremeters; the missing parameters will have a value of 0.


The table below summarizes the additional arguments for each of the load operations.

    +----------------------------------+----------------------------------------------------------------------------+
    | Operation                        | Parameters                                                                 |
    |                                  +-------+--------+--------+--------+--------+-----+-------+-----+-------+----+
    |                                  | 0     | 1      | 2      | 3      | 4      | 5   | 6     | 7   | 8     | 9  |
    +----------------------------------+-------+--------+--------+--------+--------+-----+-------+-----+-------+----+
    | ld                               | u     | v      | lod    | r      |        |     |       |     |       |    |
    +----------------------------------+-------+--------+--------+--------+--------+-----+-------+-----+-------+----+
    | ld_lz                            | u     | v      | r      |        |        |     |       |     |       |    |
    +----------------------------------+-------+--------+--------+--------+--------+-----+-------+-----+-------+----+
    | ld2dms_w (type UD)               | si    | mcsl   | mcsh   | u      | v      | r   | lod   |     |       |    |
    +----------------------------------+-------+--------+--------+--------+--------+-----+-------+-----+-------+----+
    | ld2dms_w (type UW) **{ICLLP+}**  | si    | mcs0   | mcs1   | msc2   | msc3   | u   | v     | r   | lod   |    |
    +----------------------------------+-------+--------+--------+--------+--------+-----+-------+-----+-------+----+
    | ld_mcs                           | u     | v      | r      | lod    |        |     |       |     |       |    |
    +----------------------------------+-------+--------+--------+--------+--------+-----+-------+-----+-------+----+

The table below describes the meanings of the u, v, r parameters based on the surface type.

    +----------------+-------------------------------+-------------------------------+-------------------------------+
    | Surface Type   | u                             | v                             | r                             |
    +----------------+-------------------------------+-------------------------------+-------------------------------+
    | 1D/1D_ARRAY    | unnormalized 'x' coordinate   | unnormalized array index      | ignored                       |
    +----------------+-------------------------------+-------------------------------+-------------------------------+
    | 2D/2D_ARRAY    | unnormalized 'x' coordinate   | unnormalized 'y' coordinate   | unnormalized array index      |
    +----------------+-------------------------------+-------------------------------+-------------------------------+
    | 3D             | unnormalized 'x' coordinate   | unnormalize 'y' coordinate    | unnormalized 'z' coordinate   |
    +----------------+-------------------------------+-------------------------------+-------------------------------+
```

