<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  3D_SAMPLE4 = 0x6f

## Format


### CONDITION

- Op.op=gather4, gather4_i


| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x6f(3D_SAMPLE4) | Op            | Exec_size | Pred      | Src_channel | Aoffimmi | Sampler |
|                  | Surface       | Dst       | NumParams | U           | V        | R       |
|                  | Ai\*          |           |           |             |          |         |


### CONDITION

- Op.op=gather4_b


| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x6f(3D_SAMPLE4) | Op            | Exec_size | Pred      | Src_channel | Aoffimmi | Sampler |
|                  | Surface       | Dst       | NumParams | Bias\*      | U        | V       |
|                  | R             | Ai\*      |           |             |          |         |


### CONDITION

- Op.op=gather4_c


| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x6f(3D_SAMPLE4) | Op            | Exec_size | Pred      | Src_channel | Aoffimmi | Sampler |
|                  | Surface       | Dst       | NumParams | Ref\*       | U        | V       |
|                  | R             | Ai\*      |           |             |          |         |


### CONDITION

- Op.op=gather4_i_c


| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x6f(3D_SAMPLE4) | Op              | Exec_size | Pred      | Src_channel | Aoffimmi | Sampler |
|                  | Surface         | Dst       | NumParams | Ref\*       | U        | V       |
|                  | R               |           |           |             |          |         |


### CONDITION

- Op.op=gather4_l


| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x6f(3D_SAMPLE4) | Op            | Exec_size | Pred      | Src_channel | Aoffimmi | Sampler |
|                  | Surface       | Dst       | NumParams | Lod\*       | U        | V       |
|                  | R             | Ai\*      |           |             |          |         |


### CONDITION

- Op.op=gather4_po


| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x6f(3D_SAMPLE4) | Op             | Exec_size | Pred      | Src_channel | Aoffimmi | Sampler |
|                  | Surface        | Dst       | NumParams | U           | V        | Offu\*  |
|                  | Offv\*         | R         | OffUV\*   |             |          |         |


### CONDITION

- Op.op=gather4_po_b


| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x6f(3D_SAMPLE4) | Op               | Exec_size | Pred      | Src_channel | Aoffimmi | Sampler |
|                  | Surface          | Dst       | NumParams | BiasOffUV\* | U        | V       |
|                  | R                |           |           |             |          |         |


### CONDITION

- Op.op=gather4_po_c


| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x6f(3D_SAMPLE4) | Op               | Exec_size | Pred      | Src_channel | Aoffimmi | Sampler |
|                  | Surface          | Dst       | NumParams | Ref\*       | U        | V       |
|                  | Offu\*           | Offv\*    | R         | OffUV_R\*   |          |         |


### CONDITION

- Op.op=gather4_po_i


| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x6f(3D_SAMPLE4) | Op               | Exec_size | Pred      | Src_channel | Aoffimmi | Sampler |
|                  | Surface          | Dst       | NumParams | U           | V        | R       |
|                  | OffUV\*          |           |           |             |          |         |


### CONDITION

- Op.op=gather4_po_i_c


| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x6f(3D_SAMPLE4) | Op                 | Exec_size | Pred      | Src_channel | Aoffimmi | Sampler |
|                  | Surface            | Dst       | NumParams | Ref\*       | U        | V       |
|                  | R                  | OffUV_R\* |           |             |          |         |


### CONDITION

- Op.op=gather4_po_l


| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x6f(3D_SAMPLE4) | Op               | Exec_size | Pred      | Src_channel | Aoffimmi | Sampler |
|                  | Surface          | Dst       | NumParams | LodOffUV\*  | U        | V       |
|                  | R                |           |           |             |          |         |


### CONDITION

- Op.op=gather4_po_l_c


| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x6f(3D_SAMPLE4) | Op                 | Exec_size | Pred      | Src_channel | Aoffimmi | Sampler |
|                  | Surface            | Dst       | NumParams | Ref\*       | LodOffUV\* | U       |
|                  | V                  | R         |           |             |          |         |


## Semantics


```

    Sample the surface using bilinear filtering, and return four samples.
```

## Description





```
    Sample <surface> using bilinear filtering, and return four samples for each pixel in <dst>.
```


- **Op(uw):**

  - Bit[7..0]: encodes the sampler operation

    - 0b00001000:  gather4
    - 0b00010000:  gather4_c
    - 0b00010001:  gather4_po
    - 0b00010010:  gather4_po_c
    - 0b00001101:  gather4_l
    - 0b00001110:  gather4_i
    - 0b00010101:  gather4_i_c
    - 0b00001111:  gather4_b
    - 0b00101101:  gather4_po_l
    - 0b00101110:  gather4_po_b
    - 0b00101111:  gather4_po_i
    - 0b00110101:  gather4_po_i_c
    - 0b00110111:  gather4_po_l_c
  - Bit[8]: pixel null mask enable. Specifies whether the writeback message will include an extra phase indicating the pixel null mask.


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


- **Src_channel(ub):**

  - Bit[1..0]: controls the source channel to be sampled

    - 0b00:  R
    - 0b01:  G
    - 0b10:  B
    - 0b11:  A

- **Aoffimmi(scalar):**  A UW representing the _aoffimmi modifier with the following format:. Must have type UW

  - Bit[3..0]: stores the R offset. Valid values are [-8-7]

  - Bit[7..4]: stores the V offset. Valid values are [-8-7]

  - Bit[11..8]: stores the U offset. Valid values are [-8-7]

  - Bit[15..12]: reserved. Must be zero


- **Sampler(ub):** Index of the sampler variable


- **Surface(ub):** Index of the surface variable


- **Dst(raw_operand):** The result of the sample. The four samples map to the RGBA channels as follows:. Must have type HF,F,W,UW,D,UD


        +-------------------------+--------------------------+
        | upper left sample = A   | upper right sample = B   |
        +-------------------------+--------------------------+
        | lower left sample = R   | lower right sample = G   |
        +-------------------------+--------------------------+

- **NumParams(ub):** number of additional parameters for this instruction. Valid values are  [1-15]


- **Ref(raw_operand):** The first <exec_size> elements contain the reference value that is compared against the red channel of the sampled surface


- **Lod(raw_operand):** The first <exec_size> elements contain the LOD


- **LodOffUV(raw_operand):** The first <exec_size> elements contain the LOD parameter combined with U and V offsets


- **Bias(raw_operand):** The first <exec_size> elements contain the LOD bias, with range of [-16.0, 16.0]


- **BiasOffUV(raw_operand):** The first <exec_size> elements contain the LOD bias combined with U, and V offsets


- **U(raw_operand):** The first <exec_size> elements contain the X pixel address


- **V(raw_operand):** The first <exec_size> elements contain for

            -  1D_array surfaces: the unnormalized array index
            -  Other surface: the normalized y coordinate

- **Offu(raw_operand):** The first <exec_size> elements contain the pixel offset from U. Must have type D


- **Offv(raw_operand):** The first <exec_size> elements contain the pixel offset from  V. Must have type D


- **R(raw_operand):** The first <exec_size> elements contain for

            -  2D_array: the unnormalized array index.
            -  3D and cube: the normalized z coordinate

- **OffUV(raw_operand):** Programmable U, and V offsets combined into one parameter


- **OffUV_R(raw_operand):** Programmable U, and V offsets combined into one parameter


- **Ai(raw_operand):** The first <exec_size> elements contain the array index for a cube surface


#### Properties




## Text
```



{-PVC}For pre-PVC platforms:

    [(<P>)] SAMPLE4[.pixel_null_mask].<Src_channel> (Exec_size) <Aoffimmi> <Sampler> <Surface> <Dst> <u> <v> <r> <ai>

    [(<P>)] SAMPLE4_C[.pixel_null_mask].<Src_channel> (Exec_size) <Aoffimmi> <Sampler> <Surface> <Dst> <ref> <u> <v> <r> <ai>

    [(<P>)] SAMPLE4_PO[.pixel_null_mask].<Src_channel> (Exec_size) <Aoffimmi> <Sampler> <Surface> <Dst> <u> <v> <offu> <offv> <r>

    [(<P>)] SAMPLE4_PO_C[.pixel_null_mask].<Src_channel> (Exec_size) <Aoffimmi> <Sampler> <Surface> <Dst> <ref> <u> <v> <offu> <offv> <r>

    [(<P>)] SAMPLE4_b[.pixel_null_mask].<Src_channel> (Exec_size) <Aoffimmi> <Sampler> <Surface> <Dst> <Bias> <u> <v> <r> <ai>

    [(<P>)] SAMPLE4_l[.pixel_null_mask].<Src_channel> (Exec_size) <Aoffimmi> <Sampler> <Surface> <Dst> <Lod> <u> <v> <r> <ai>

    [(<P>)] SAMPLE4_i[.pixel_null_mask].<Src_channel> (Exec_size) <Aoffimmi> <Sampler> <Surface> <Dst> <u> <v> <r> <ai>









    // instruction specific parameters may vary
```
## Notes





```

For each enabled channel <exec_size> elements are returned in RGBA order, with the disabled channels skipped in the results. Only the enabled pixels are returned in <dst>. Each channel's return data start in the next GRF; if <exec_size> * sizeof(dst_type) is smaller than the register size, the remaining portions of the register have undefined values.

For all operations, if <pixel_null_mask> is set, an additional GRF is returned after the sampler data, with <exec_size> bits in the first DWord containing the pixel null mask values. This field has the bit for all pixels set to 1 except those pixels in which a null page was source for at least one texel.


Extra parameters (after NumParams) for this instruction are required only for certain operations and surface types.

  - **{pre-ICLLP}** All operands must have type F.
  - **{ICLLP+}** All operands must have the same type, which can be either HF or F.
  - It is permitted to skip the trailing parameters; the missing parameters will have the value of 0.

The table below summarizes the additional arguments for each of the sample4 operations.

        +------------------+-------------------------------------------------+
        | Operation        |          Parameters                             |
        |                  +----------+-----+--------+--------+--------+-----+
        |                  | 0        | 1   | 2      | 3      | 4      | 5   |
        +------------------+----------+-----+--------+--------+--------+-----+
        | gather4          | u        | v   | r      | ai     |        |     |
        +------------------+----------+-----+--------+--------+--------+-----+
        | gather4_c        | ref      | u   | v      | r      | ai     |     |
        +------------------+----------+-----+--------+--------+--------+-----+
        | gather4_po       | u        | v   | offu   | offv   | r      |     |
        +------------------+----------+-----+--------+--------+--------+-----+
        | gather4_po_c     | ref      | u   | v      | offu   | offv   | r   |
        +------------------+----------+-----+--------+--------+--------+-----+
        | gather4_b        | bias     | u   | v      | r      | ai     |     |
        +------------------+----------+-----+--------+--------+--------+-----+
        | gather4_po_l     | lodOffUV | u   | v      | r      | ai     |     |
        +------------------+----------+-----+--------+--------+--------+-----+
        | gather4_po_i     | u        | v   | r      | OffUV  |        |     |
        +------------------+----------+-----+--------+--------+--------+-----+
```

