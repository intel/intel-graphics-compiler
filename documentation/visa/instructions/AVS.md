<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  AVS = 0x43

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x43(AVS) | Channels | Sampler   | Surface | U_offset            | V_offset | DeltaU |
|           | DeltaV   | u2d       | groupID | verticalBlockNumber | cntrl    | v2d    |
|           | execMode | IEFBypass | Dst     |                     |          |        |


## Semantics


```

      AVS functionality
```

## Description






- **Channels(ub):**

  - Bit[3..0]: determines the write masks for the RGBA channel, with R being bit 0 and A bit 3. At least one channel must be enabled (i.e., "0000" is not allowed)


- **Sampler(ub):** Index of the sampler variable


- **Surface(ub):** Index of the surface variable


- **U_offset(scalar):** the normalized x coordinate of pixel 0. Must have type F


- **V_offset(scalar):** the normalized y coordinate of pixel 0. Must have type F


- **DeltaU(scalar):** the difference in coordinates for adjacent pixels in the X direction. Must have type F


- **DeltaV(scalar):** the difference in coordinates for adjacent pixels in the Y direction. Must have type F


- **u2d(scalar):** Defines the change in the delta U for adjacent pixels in the X direction. Must have type F


- **groupID(scalar):** This parameter will be used to group messages for reorder for sample_8x8 messages. For all messages with the same Group ID they must be have the following in common: Surface state, Sampler State, GRFID, M0, and M1 except for Block number. Must have type UD


- **verticalBlockNumber(scalar):** This field will specify the vertical block offset 16x4 block being sent for this sample_8x8 messages. This will be equal to the vertical pixel offset from the  given address pixel 0 V address divided by 4.. Must have type UD

      e.g.,

        A 16x16 macro-block can be processed with 4 16x4 blocks. They can
        share one Group ID, Pix0U, and Pix0V for the group. Top block has
        VBN=0, the next below has VBN=1, and so on. Here Pix0U, and Pix0V
        are address for the pixel at the top left of the group, not the
        block origin.

- **cntrl(ub):** Output format control. Valid values are:

  - 0: "16-bit full". Two bytes will be returned for each pixel
  - 1: "16-bit chrominance downsampled". Like the previous one, except only even pixels are returned for R and B channels
  - 2: "8-bit full". One byte is returned for each pixel
  - 3: "8-bit chrominance downsampled". Like the previous one, except only even pixels are returned for R and B channels

- **v2d(scalar):** Defines the change in the delta V for adjacent pixels in the Y direction. Must have type F


- **execMode(ub):** Determines the number of pixels returned. Valid values are:

  - 0: 16x4
  - 1: 8x4
  - 2: 16x8
  - 3: 4x4

- **IEFBypass(scalar):** Enables IEFBypass if bit 0 is zero. Must have type UB


- **Dst(raw_operand):** The general variable storing the result of the sample. The actual data returned is determined by a combination of <channel>, <cntrl>, <execMode>, as well as whether output shuffle is enabled in the sampler state


#### Properties




## Text
```



    AVS.<channel> <sampler> <surface> <u_offset> <v_offset> <deltaU> <deltaV> <groupID> <verticalBlockNumber> <cntrl> <v2d> <execMode> <IEFBypass> <dst>
```
## Notes





```
    -  If output shuffle is off:

            +-------------+-------------+-------------+-------------+
            | R[0:N]      | G[0:N]      | B[0:N]      | A[0:N]      |
            +-------------+-------------+-------------+-------------+
            | R[64:127]   | G[64:127]   | B[64:127]   | A[64:127]   |
            +-------------+-------------+-------------+-------------+

            Where R[0] corresponds to the first element in <dst>. N is equal to the
            number of pixels specified by the execMode except for 16x8, for which N
            is 63. For 16x8 mode, an additional 64 pixels will be delivered. The
            disabled channels will be skipped with no gap in m. <cntrl> determines
            the pixel channel size as well as whether the odd pixels will be skipped
            for the R and B channels.

    -  If output shuffle is on:

            The format of 16x4 mode becomes

            +--------------------------------+--------------------------------+--------------------------------+--------------------------------+
            | R[0:7][16:23][32:39][48:55]    | G[0:7][16:23][32:39][48:55]    | B[0:7][16:23][32:39][48:55]    | A[0:7][16:23][32:39][48:55]    |
            +--------------------------------+--------------------------------+--------------------------------+--------------------------------+
            | R[8:15][24:31][40:47][56:63]   | G[8:15][24:31][40:47][56:63]   | B[8:15][24:31][40:47][56:63]   | A[8:15][24:31][40:47][56:63]   |
            +--------------------------------+--------------------------------+--------------------------------+--------------------------------+

            The disabled channels will be skipped with no gap in m. The cntrl field
            determines the pixel channel size as well as whether the odd pixels will
            be skipped for the R and B channels.

            The format for 8x4 and 4x4 is the same as before, while 16x8 is not
            supported.

            When <cntrl> is "8-bit chrominance downsampled", if either R or B
            channel (but not both) are disabled, the two channels will not be
            skipped in m, but the disabled channel is not written to m. If both R
            and B channels are disabled they will still be skipped.

    - **SIMD Control Flow:** channel enable is ignored.
```

