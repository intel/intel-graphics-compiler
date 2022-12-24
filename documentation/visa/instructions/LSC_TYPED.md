<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  LSC_TYPED = 0x8a

## Format


### CONDITION

- LscSubOp=lsc_atomic_and, lsc_atomic_fadd, lsc_atomic_fcas, lsc_atomic_fmax, lsc_atomic_fmin, lsc_atomic_fsub, lsc_atomic_iadd, lsc_atomic_icas, lsc_atomic_idec, lsc_atomic_iinc, lsc_atomic_isub, lsc_atomic_load, lsc_atomic_or, lsc_atomic_smax, lsc_atomic_smin, lsc_atomic_store, lsc_atomic_umax, lsc_atomic_umin, lsc_atomic_xor, lsc_load_quad, lsc_load_status, lsc_read_surface_info, lsc_store_quad, lsc_store_uncompressed


| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x8a(LSC_TYPED) | LscSubOp                   | ExecSize\* | Predicate\* | CachingL1        | CachingL3    | AddrType |
|                 | AddrSize\*                 | DataSize\* | DataOrder\* | DataElemsPerAddr\* | ChMask\*     | Surface  |
|                 | DstData                    | Src0AddrsU\* | Src0AddrsV\* | Src0AddrsR\*     | Src0AddrsLOD\* | Src1Data |
|                 | Src2Data\*                 |            |            |                  |              |          |


### CONDITION

- LscSubOp=lsc_load_block2d, lsc_store_block2d


| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x8a(LSC_TYPED) | LscSubOp              | CachingL1 | CachingL3 | AddrType  | BlockWidth\* | BlockHeight\* |
|                 | Surface               | DstData   | Src0AddrX\* | Src0AddrY\* | Src1Data   |             |


## Semantics



The `lsc_load_quad` and `lsc_store_quad` LSC_UNTYPED versions,
except the address calculation uses surface state and the U, V, R, and LOD coordinates
to calculate the surface address (based on the surface state).
Note, the data and address types chosen must match the values in the surface state.

Atomics and `lsc_store_uncompressed` are identical in behavior to LSC_UNTYPED with a
scale of 1 and immediate offset of 0.

LOAD_BLOCK2D:
```
            reg_pitch = BlockWidth < 4 ? 4 : round_up_pow2(BlockWidth);
            for (u = 0; u < BlockHeight; u++) {
                for (v = 0; v < BlockWidth/(SURF_STATEpixel_size); v++) {
                    dest[u*reg_pitch + v] = (SURF_STATEDATA_TYPE *)(2D_TILER(SURF_STATE_TABLE[bsso|bti+btp], u, v));
                }
            }
```

STORE_BLOCK2D:
```
            reg_pitch = BlockWidth < 4 ? 4 : round_up_pow2(BlockWidth);
            for (u = 0; u < BlockHeight; u++) {
                for (v = 0; v < BlockWidth/(SURF_STATEpixel_size); v++) {
                    (SURF_STATEDATA_TYPE *)(2D_TILER(SURF_STATE_TABLE[bsso|bti+btp], u, v)) = Src1Data[u*reg_pitch + v];
                }
            }
```

## Description





A typed generic LSC operation (load, store, or atomic).

Performs a typed LSC operation on a set of surface coordinates for a given surface
and either stores the result into `DstData` (for a load) or writes out
`Src1Data` (for a store).  Atomics perform the operation and can read back a
result in a single operation.  Load operations may specify the null
register (`V0`) to prefetch data only.

The Typed 2D Block messages allow loads and stores from/to one rectangular block of
a Typed 2D surface. The message header contains the block parameters
(Block start X, Y, Width, Height). The surface parameters
(Surface Height, Width, Pitch, Format) are fetched from the surface state.


- **LscSubOp(ub):** The specific LSC operation being called

      .. _table_LSC_SubOp_Typed:

      .. table:: **LSC Operation (LSC_TYPED) Encoding**

        | Symbol                 | SubOp | Name                        | Extra atomic arguments |
        | --- | ---| ---| ---| ---|
        | lsc_load_quad          | 0x02  | vector load w. XYZW ChEn    | N/A                    |
        | lsc_load_block2d       | 0x03  | 2D block load               | N/A                    |
        | lsc_store_quad         | 0x06  | vector store w. XYZW ChEn   | N/A                    |
        | lsc_store_block2d      | 0x07  | 2D block store              | N/A                    |
        | lsc_atomic_iinc        | 0x08  | atomic integer increment    | 0                      |
        | lsc_atomic_idec        | 0x09  | atomic integer decrement    | 0                      |
        | lsc_atomic_load        | 0x0A  | atomic load                 | 0                      |
        | lsc_atomic_store       | 0x0B  | atomic store                | 1                      |
        | lsc_atomic_iadd        | 0x0C  | atomic integer add          | 1                      |
        | lsc_atomic_isub        | 0x0D  | atomic integer subtract     | 1                      |
        | lsc_atomic_smin        | 0x0E  | atomic signed int min       | 1                      |
        | lsc_atomic_smax        | 0x0F  | atomic signed int max       | 1                      |
        | lsc_atomic_umin        | 0x10  | atomic unsigned int min     | 1                      |
        | lsc_atomic_umax        | 0x11  | atomic unsigned int max     | 1                      |
        | lsc_atomic_icas        | 0x12  | atomic int compare and swap | 2                      |
        | lsc_atomic_fadd        | 0x13  | floating-point add          | 1                      |
        | lsc_atomic_fsub        | 0x14  | floating-point subtract     | 1                      |
        | lsc_atomic_fmin        | 0x15  | floating-point min          | 1                      |
        | lsc_atomic_fmax        | 0x16  | floating-point max          | 1                      |
        | lsc_atomic_fcas        | 0x17  | floating-point CAS          | 2                      |
        | lsc_atomic_and         | 0x18  | logical (bitwise) AND       | 1                      |
        | lsc_atomic_or          | 0x19  | logical (bitwise) OR        | 1                      |
        | lsc_atomic_xor         | 0x1A  | logical (bitwise) XOR       | 1                      |
        | lsc_load_status        | 0x1B  | load address status bitset  | N/A                    |
        | lsc_store_uncompressed | 0x1C  | load address status bitset  | N/A                    |
        | lsc_read_surface_info  | 0x1E  | load surface information    | N/A                    |

- **ExecSize(ub):** Execution size.  This should be no larger than SIMD8 for DG2 and SIMD16 for PVC. Not needed for block2d as it is always SIMD1.


- **Predicate(uw):** Predication control. Not needed for block2d.


- **CachingL1(ub):** Same as in LSC_UNTYPED

  - 0x0: default  (.df)
  - 0x1: uncached  (.uc)
  - 0x2: cached  (.ca)
  - 0x3: writeback  (.wb)
  - 0x4: writethrough  (.wt)
  - 0x5: streaming  (.st)
  - 0x6: read-invalidate  (.ri)

- **CachingL3(ub):** Same as in LSC_UNTYPED

  - 0x0: default  (.df)
  - 0x1: uncached  (.uc)
  - 0x2: cached  (.ca)
  - 0x3: writeback  (.wb)
  - 0x4: writethrough  (.wt)
  - 0x5: streaming  (.st)
  - 0x6: read-invalidate  (.ri)

- **AddrType(ub):** Same as in LSC_UNTYPED


- **AddrSize(ub):** Same as in LSC_UNTYPED. Not needed for block2d.


- **DataSize(ub):** Same as in LSC_UNTYPED. Not needed for block2d.


- **DataOrder(ub):** Same as in LSC_UNTYPED. Not needed for block2d.


- **DataElemsPerAddr(ub):** Same as in LSC_UNTYPED. Not needed for block2d.


- **ChMask(ub):** Same as in LSC_UNTYPED. Not needed for block2d as it is always no mask.


- **BlockWidth(uw):** Only for block2d; this immediate value holds the 2D region's width in bytes(for block2d only). Valid value is [1, 64].


- **BlockHeight(uw):** Only for block2d; this immediate value holds the 2D region's height in number of data elements(for block2d only). Valid value is [1, 64].


- **Surface(vec_operand):** Same as in LSC_UNTYPED


- **DstData(raw_operand):** Same as in LSC_UNTYPED. For block2d, each row starts at row_id*register_pitch bytes and BlockWidth bytes are returned.


- **Src0AddrsU(raw_operand):** The the vector register holding U coordinate address operands. Not for block2d.


- **Src0AddrsV(raw_operand):** The the vector register holding V coordinate address operands.  This should be the null register for 1D surfaces. Not for block2d.


- **Src0AddrsR(raw_operand):** The the vector register holding R coordinate address operands.  This should be the null register for 1D or 2D surfaces. Not for block2d.


- **Src0AddrsLOD(raw_operand):** The the vector register holding LOD address operands.  This should be the null register for 1D, 2D, and 3D surfaces. Not for block2d.


- **Src0AddrX(vec_operand):** the 2d tile's base X position in bytes to load or store (for block2d only).


- **Src0AddrY(vec_operand):** the 2d tile's base Y position in bytes to load or store (for block2d only).


- **Src1Data(raw_operand):** Same as in LSC_UNTYPED. For block2d, each row starts at row_id*register_pitch bytes and BlockWidth bytes are written to the surface for each row.


- **Src2Data(raw_operand):** Same as in LSC_UNTYPED. Not for block2d.


#### Properties




## Text
```



```
    [ ( Pred ) ] lsc_load_quad.tgm [ .CachingL1 [ .CachingL3 ] ] [ ( ExecSize ) ]
          DstData :   (DataSize x DataElemsPerAddr DataOrder)
          AddrType [ Src0AddrsU , Src0AddrsV , Src0AddrsR , Src0AddrsLOD ]: AddrSize

    [ ( Pred ) ] lsc_store_quad.tgm [ .CachingL1 [ .CachingL3 ] ] [ ( ExecSize ) ]
          AddrType [ Src0AddrsU , Src0AddrsV , Src0AddrsR , Src0AddrsLOD ]: AddrSize
          Src1Data :   (DataSize x DataElemsPerAddr DataOrder)

    [ ( Pred ) ] lsc_atomic*.tgm [ .CachingL1 [ .CachingL3 ] ] [ ( ExecSize ) ]
          DstData :   (DataSize x DataElemsPerAddr DataOrder)
          AddrType [ Src0AddrsU ]: AddrSize
          Src1Data
          Src2Data

    lsc_load_block2d.SFID [ .CachingL1 [ .CachingL3 ] ]
          DstData : BlockWidth x BlockHeight
          AddrType [ SurfaceBase , SurfaceWidth , SurfaceHeight , SurfacePitch , Src0AddrX , Src0AddrY ]

    lsc_store_block2d.SFID [ .CachingL1 [ .CachingL3 ] ]
          AddrType [ Src0AddrX , Src0AddrY ]
          Src1Data : BlockWidth x BlockHeight




```
```
## Examples




            // typed load from BTI surface 0x4 with U = V12, V = V13, R = V14 (and no LOD)
            lsc_load_quad.tgm      V20:d32.xyzw  bti(0x4)[V12,V13,V14]:a64

            // typed store of channels x and z to BTI surface 0x4
            // with U = V12, V = V13 (and no R or LOD)
            // (uses default caching)
            lsc_store_quad.tgm     bti(0x4)[V12,V13]:a64  V13:d32.xz

            // Atomically increment the values in V13.
            lsc_atomic_inc.tgm     V14:d32  bti(0x0)[V12,V13]:a64  V0  V0

            // Typed 2d block load with height 64 bytes and height 2 bytes from BIT surface 0x0
            // The block's top coordinate (Y) is OFF_Y and the left (X) is OFF_X.
            lsc_load_block2d.tgm    VDATA:64x2    bti(0x0)[OFF_X,OFF_Y]
            //
            // Typed 2d block store operation. The parameters are similar to above.
            lsc_store_block2d.tgm   bit(0x0)[OFF_X,OFF_Y]   VDATA:64x2


## Notes





        An atomic that uses 0 extra arguments uses the null register for `Src1Data` and `Src2Data`.
        An atomic that uses 1 extra argument sets only `Src1Data` and leaves `Src2Data` null.
        Ternary atomics (those using 2 extra arguments) set both `Src1Data` and `Src2Data`.

        The following table lists the legal width/height combination and their pitch for block2d load/store:
        +---------------------+------------------------+---------------------+
        | Block Width (bytes) | Register Pitch (bytes) | Block Height (rows) |
        +---------------------+------------------------+---------------------+
        |     1-4             |       4                |      1-64           |
        +---------------------+------------------------+---------------------+
        |     5-8             |       8                |      1-32           |
        +---------------------+------------------------+---------------------+
        |     9-16            |       16               |      1-16           |
        +---------------------+------------------------+---------------------+
        |     17-32           |       32               |      1-8            |
        +---------------------+------------------------+---------------------+
        |     33-63           |       64               |      1-4            |
        +---------------------+------------------------+---------------------+

