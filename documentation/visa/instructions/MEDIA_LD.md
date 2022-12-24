<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  MEDIA_LD = 0x37

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x37(MEDIA_LD) | Modifiers | Surface | Plane | Block_width | Block_height | X_offset |
|                | Y_offset  | Dst     |       |             |              |          |


## Semantics


```

      UD reg_pitch = block_width < 4 ? 4 : nextPowerOf2(block_width);
      for (i = 0; i < block_height; ++i) {
        for (j = 0; j < block_width; ++j) {
          dst[i*reg_pitch+j] = surface[y_offset+i][x_offset+j]; // one byte
        }
      }
```

## Description





```
    Reads a rectangular block of data from <surface> and stores the values in <dst>.
```


- **Modifiers(ub):**

  - Bit[2..0]: encodes the modifiers for the read

    - 0b000:  no modifiers
    - 0b010:  top_field -- for interleaved surfaces, indicates only the top field surface data are needed
    - 0b011:  bottom_field -- for interleaved surfaces, indicates only the bottom field surface data are needed.

- **Surface(ub):** Index of the surface variable.  It must be a 2D surface.

            - T0(SLM): no
            - T5(stateless): no

- **Plane(ub):** The index to the sub-plane as defined in different surface formats. See [5] for a description of the planes represented by each index value. Valid values are  [0], [3]


- **Block_width(ub):** Width in bytes of the block being accessed. Valid values are  [1], [64]


- **Block_height(ub):** Height in rows of blocks being accessed. Valid values are  [1], [64]


- **X_offset(scalar):** The X byte offset of the upper left corner of the block into the surface. Must have type UD


- **Y_offset(scalar):** The Y byte offset of the upper left corner of the block into the surface. Must have type UD


- **Dst(raw_operand):** Return value. Each row starts at row_id * <register_pitch> bytes, and <block_width> bytes are returned


#### Properties
- **Out-of-bound Access:** On read: implementation defined behavior based on surface format.




## Text
```



    MEDIA_LD.<mods> (<block_width>, <block_height>) <surface> <plane> <x_offset> <y_offset> <dst> //<mods> is the bits for the modifiers field.
```
## Notes





    top_field and bottom_field modifiers are used to provide support for interleaved textures, by controlling the vertical line stride and vertical line stride offset of a media read.

    .. _table_MediaWidthHeightCombinations:

    .. table:: **Legal width/height combination and their pitch:**

      | Block Width (bytes) | Register Pitch (bytes) | Maximum Block Height (rows) |
      | --- | ---| ---| ---|
      | 1-4                 | 4                      | 64                          |
      | 5-8                 | 8                      | 32                          |
      | 9-16                | 16                     | 16                          |
      | 17-32               | 32                     | 8                           |
      | 33-64               | 64                     | 4                           |

