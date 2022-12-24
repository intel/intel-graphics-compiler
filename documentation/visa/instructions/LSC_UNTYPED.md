<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  LSC_UNTYPED = 0x89

## Format


### CONDITION

- LscSubOp=lsc_apndctr_atomic_add, lsc_apndctr_atomic_sub


| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x89(LSC_UNTYPED) | LscSubOp                   | Exec_size | Pred      | LscSFID          | CachingL1 | CachingL3 |
|                   | AddrType\*                 | DataSize  | DataOrder | DataElemsPerAddr\* | ChMask\*  | Surface\* |
|                   | DstData                    | Src0Data\* |           |                  |           |           |


### CONDITION

- LscSubOp=lsc_atomic_and, lsc_atomic_fadd, lsc_atomic_fcas, lsc_atomic_fmax, lsc_atomic_fmin, lsc_atomic_fsub, lsc_atomic_iadd, lsc_atomic_icas, lsc_atomic_idec, lsc_atomic_iinc, lsc_atomic_isub, lsc_atomic_load, lsc_atomic_or, lsc_atomic_smax, lsc_atomic_smin, lsc_atomic_store, lsc_atomic_umax, lsc_atomic_umin, lsc_atomic_xor, lsc_load, lsc_load_quad, lsc_load_status, lsc_store, lsc_store_quad, lsc_store_uncompressed


| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x89(LSC_UNTYPED) | LscSubOp                   | Exec_size | Pred          | LscSFID  | CachingL1 | CachingL3 |
|                   | AddrType\*                 | AddrScale\* | AddrImmOffset\* | AddrSize\* | DataSize  | DataOrder |
|                   | DataElemsPerAddr\*         | ChMask\*  | Surface\*     | DstData  | Src0Addrs\* | Src1Data\* |
|                   | Src2Data\*                 |           |               |          |           |           |


### CONDITION

- LscSubOp=lsc_load_block2d, lsc_store_block2d


| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x89(LSC_UNTYPED) | LscSubOp              | Exec_size   | Pred         | LscSFID       | CachingL1    | CachingL3 |
|                   | DataSize              | DataOrder   | Blocks\*     | BlockWidth\*  | BlockHeight\* | VNNI\*    |
|                   | DstData               | SurfaceBase\* | SurfaceWidth\* | SurfaceHeight\* | SurfacePitch\* | Src0AddrX\* |
|                   | Src0AddrY\*           | Src1Data\*  |              |               |              |           |


### CONDITION

- LscSubOp=lsc_load_strided, lsc_store_strided


| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x89(LSC_UNTYPED) | LscSubOp              | Exec_size | Pred          | LscSFID  | CachingL1    | CachingL3     |
|                   | AddrType\*            | AddrScale\* | AddrImmOffset\* | AddrSize\* | DataSize     | DataOrder     |
|                   | DataElemsPerAddr\*    | ChMask\*  | Surface\*     | DstData  | Src0AddrBase\* | Src0AddrPitch\* |
|                   | Src1Data\*            |           |               |          |              |               |


## Semantics


```
                    struct grf_data_sized {
                       // 8 elements for :d64, 16 for :d32, 32 for :d16, 64 for :d8 (half of this for DG2)
                       DataSize elems[GRF_SIZE_BYTES/sizeof(DataSize)];
                    } DstData, Src1Data, Src2Data;
                    union grf_addr_size {
                       // 8 elements for :a64, 16 for :a32, 32 for :a16 (half of this for DG2)
                       AddrSize addrs[GRF_SIZE_BYTES/sizeof(AddrSize)];
                    } src0;

      LOAD:
                    for (n = 0; n < ExecSize; n++) {
                      if (Msg.ChEn[n]) {
                        for (v = 0; v < vect_size; v++) {
                          grf_sized src = AddrScale*(Surface + Src0Addrs[n]) + AddrImmOffset
                          auto datum = src.elems[v];
                          if (DataOrder == LSC_DATA_ORDER_TRANSPOSED) {
                            DstData[n].elems[v] = datum
                          } else {
                            DstData[v].elems[n] = datum
                          }
                        }
                      }
                    }

      LOAD_QUAD:
                    for (n = 0; n < ExecSize; n++) {
                      if (Msg.ChEn[n]) {
                        for (m = v = 0; v < 4; v++) {
                          if (ChMask[v]) {
                            grf_sized src = AddrScale*(Surface + Src0Addrs[n]) + AddrImmOffset
                            DstData[m].elems[n] = src.elems[v]
                            m++;
                          }
                        }
                      }
                    }

      LOAD_STRIDED:
                    for (n = 0; n < ExecSize; n++) {
                      if (Msg.ChEn[n]) {
                        for (v = 0; v < vect_size; v++) {
                          auto addr = AddrScale*(Base + offset + Src0Addrs) + n*Src0AddrPitch + AddrImmOffset
                          if (DataOrder == LSC_DATA_ORDER_TRANSPOSED) {
                            dst[n].elems[v] = addr->elems[v]
                          } else {
                            dst[v].elems[n] = addr->elems[v]
                          }
                        }
                      }
                    }

      LOAD_STATUS:
                    for (n = 0; n < ExecSize; n++) {
                      s = 1;
                      if (Msg.ChEn[n]) {
                        auto addr = AddrScale*(Surface + Src0Addrs[n] + AddrImmOffset)
                        for (v = 0; v < vect_size; v++) {
                          if (s) {
                            s = IsTRTT_VALID_PAGE(addr.elems[v])
                          }
                        }
                      }
                      dest.bit[n] = MsgChEn[n] & s
                    }

      LOAD_BLOCK2D:
          // let T be the element data type (e.g. uint32_t for d32)
          // src0.* is see block2d array source payload register
          const int B = Blocks, H = BlockHeight, W = BlockWidth; // values -1-encoded
          const int X = Src0AddrX, Y = Src0AddrY; // these are not -1-encoded
          const bool transpose =  DataOrder == LSC_DATA_ORDER_TRANSPOSED, transform = VNNI;
          const int elems_per_dword = 4/sizeof(T);
          const int elems_per_reg = GRF_SIZE_BYTES/sizeof(T);

          auto store_elem_to_grf = [&](int dst_off, T value) {
            int r = dst_off / elems_per_reg, sr = dst_off % elems_per_reg;
            dst[r].elems[sr] = value;
          };
          const char *block_start = src0.SurfaceBaseAddr + src0.Y * src0.SurfacePitch;

          // 'b' is the block, 'u' represents the Y-axis, and 'v' the X-axis
          auto load_elem = [&](int dst_off, int b, int y, int x) {
            T *row = (T *)(SurfaceBase + y * SurfacePitch);
            store_elem_to_grf(dst_off, row[X + b * W + x]);
          };

          // loads a row span or column span of elements into GRF ...
          auto load_row_span = [&](int grf_off, int b, int y, int x, int n) {
            for (int i = 0; i < n; i++) load_elem(grf_off + i, b, y, x + i);
          };
          auto load_col_span = [&](int grf_off, int b, int y, int x, int n) {
            for (int i = 0; i < n; i++) load_elem(grf_off + i, b, y + i, x);
          };
          auto fill_grf_zeros = [&](int dst_off, int n) {
            for (int i = 0; i < n; i++) store_elem_to_grf(dst_off + i, 0);
          };

          const int grf_row_size = transpose ? H : W;
          const int grf_row_pitch = round_up_pow2(grf_row_size);
          const int grf_block_size = grf_row_pitch * (transpose ? W : H);
          const int grf_block_pitch = round_to_multiple_of(elems_per_reg, grf_block_size);

          for (int b = 0; b < B; b++) {
            if (!transpose && !transform) {
            for (int y = 0; y < H; y++) {
              int grf_row_start = b*grf_block_pitch + y*grf_row_pitch;
              load_row_span(grf_row_start, b, y, 0, W);
              fill_grf_zeros(grf_row_start + W, grf_row_pitch - W);
            } // for H
            } else if (!transpose && transform) {
            for (int y = 0; y < H; y += elems_per_dword) {
              for (int x = 0; x < W; x++) {
              load_col_span(
                b*grf_block_pitch + y*grf_row_pitch + x*elems_per_dword,
                b, y, x,
                elems_per_dword);
              } // for W
              // pad out the row in GRF with zeros
              fill_grf_zeros(
              b*grf_block_pitch + y*grf_row_pitch + W*elems_per_dword,
              elems_per_dword*(grf_row_pitch - W));
            } // for H
            } else if (transpose && !transform) {
            for (int x = 0; x < W; x++) {
              load_col_span(
              b*grf_block_pitch + x*grf_row_pitch,
              b, 0, x,
              H);
              fill_grf_zeros(b*grf_block_pitch + x*grf_row_pitch + H, grf_row_pitch - H);
            }
            } else if (transpose && transform) {
            for (int x = 0; x < W; x += elems_per_dword) {
              for (int y = 0; y < H; y++) {
              load_row_span(
                b*grf_block_pitch + x*grf_row_pitch + y*elems_per_dword,
                b, y, x,
                elems_per_dword);
              } // for H
              // pad out the row in GRF with zeros
              fill_grf_zeros(
              b*grf_block_pitch + x*grf_row_pitch + H*elems_per_dword,
              elems_per_dword * (grf_row_pitch - H));
            } // for W
            }
            // pad out GRF for this block
            fill_grf_zeros(
            b*grf_block_pitch + grf_block_size,
            grf_block_pitch - grf_block_size);
          } // for B

      STORE:
      STORE_UNCOMPRESSED:
                    for (n = 0; n < ExecSize; n++) {
                      if (Msg.ChEn[n]) {
                        for (v = 0; v < vect_size; v++) {
                          auto addr = AddrScale*(Surface + Src0Addrs[n] + AddrImmOffset)
                          if (DataOrder == LSC_DATA_ORDER_TRANSPOSED) {
                            *addr = DstData[n].elems[v]
                          } else {
                            *addr = DstData[v].elems[n]
                          }
                        }
                      }
                    }

      STORE_QUAD:
                    for (n = 0; n < ExecSize; n++) {
                      if (Msg.ChEn[n]) {
                        for (m = v = 0; v < 4; v++) {
                          if (ChMask[v]) {
                            (*(AddrScale*(Surface + Src0Addrs[n] + AddrImmOffset))).elems[v] = Src1Data[m].elems[n]
                            m++
                          }
                        }
                      }
                    }

      STORE_STRIDED:
                    auto pitch = v*DataSize // vISA picks default pitch to make a packed block operation
                    for (n = 0; n < ExecSize; n++) {
                      if (Msg.ChEn[n]) {
                        for (v = 0; v < vect_size; v++) {
                          auto addr = AddrScale*(Surface + Src0Addrs[n] + AddrImmOffset) + n*Src0AddrPitch + AddrImmOffset
                          if (DataOrder == LSC_DATA_ORDER_TRANSPOSED) {
                            addr.elems[v] = Src1Data[n].elems[v];
                          } else {
                            addr.elems[v] = Src1Data[v].elems[n];
                          }
                        }
                      }
                    }

      STORE_BLOCK2D:
                    // similar to load block2d, but accesses are stores instead of loads
                    // and only a single block is permitted
                    for (n = 0; n < BlockHeight; n++) {
                      for (v = 0; v < BlockWidth; v++) {
                        reg_pitch = round_up_pow2(BlockHeight);
                        (SurfaceBase+(Src0AddrY+(n*SurfacePitch)+Src0AddrX)).elems[v] =
                          Src1Data.elems[(v*reg_pitch)+n];
                      }
                    }

      AppendCounterAtomicAdd:
                  ctr_address = SURFACE_STATE[state_offset].Aux_surface_address
                    for (n = 0; n < ExecSize; n++) {
                      if (Msg.ChEn[n]) {
                        old = (ctr_address).data_size[0];
                        (ctr_address).data_size[0] = old + src0Data[0].data_size[n];
                        dest[0].data_size[n] = old;
                      }
                    }

      AppendCounterAtomicSub:
                  ctr_address = SURFACE_STATE[state_offset].Aux_surface_address
                    for (n = 0; n < ExecSize; n++) {
                      if (Msg.ChEn[n]) {
                        old = (ctr_address).data_size[0];
                        (ctr_address).data_size[0] = old - src0Data[0].data_size[n];
                        dest[0].data_size[n] = old;
                      }
                    }
```

## Description






This set of instructions performs an LSC operation on a set of addresses
on from a given surface (if applicable) or set of address and either stores
the result into `DstData` (for a load) or writes it out (for a store)
from `Src1Data`.  Unused operands will be set to the null register `V0`
in the binary format.  Most load operations permit the null register `V0`
to implement a prefetch only (the load stops at the cache).  Most operations
support an immediate offset and scaling factor.  In some cases, hardware can
fuse this into the operation.

The basic load and store operations `lsc_load` and `lsc_store`
are regular gather/scatter accesses.  They take a set of addresses and load
data for each address.  These operations can load several elements (a "vector")
per address.  Vector elements are returned in structure-of-array order or SIMT
order with successive elements in successive registers.  For non-transpose
data orderings the number of elements for each component in the vector
will be equal to the `ExecSize`.

These messages support a special alternate `transpose` mode (data order flag)
and enables one to emulate block loads and stores.
In this mode a single address is taken and loads successive vector components
across a regster (or block of them).  Note, transpose only supports SIMD1.

Atomic operations (except for append counter atomic) are also gather/scatter
operation.  They operate on the addresses in `Src0Addrs` and possibly with data
in `Src1Data` and `Src2Data`, and can read back the result to `DstData`
(or use `%null` to avoid the writeback).  For operations that do not use
`Src1Data` or `Src2Data`, specify the null register `%null`.  Transpose is
not permitted for atomic operations.


The `lsc_load_strided` and `lsc_store_strided` operations take a single
address and a stride and generate an arithmetic (linear) sequence to
compute each SIMT address to load.  Similar to the `lsc_load`/`lsc_store`
operation, strided operations are are also SIMT operations and use the
execution mask to annul inactive lanes just like any other scatter
gather operation.  These operations can be more bus-efficient since they
only need pass a single register for the address rather than a large set.
The stride (or pitch) defaults to the data width times the vector size,
implying a block ("packed") load.  However, it can be specified to other
values to enable various load patterns (e.g. 0 would be a broadcast load
with each SIMT lane loading the same value).  This operation is not supported
on all platforms.  Check with hardware documentation.

The `lsc_load_quad` and `lsc_store_quad` operations allow loading
a subset of four semi-contiguous elements (X, Y, Z, and W).
For instance, a programmer can load X, Z, and W omitting Y with the data
suffix `.xzw`.  Otherwise, they are similar to `lsc_load` and `lsc_store`
in behavior.  If loading contiguous data, prefer the regular `lsc_load`
and `lsc_store` operations with vector sizes 1, 2, 3, or 4.

The block2d operations load 2d blocks from a surface potentially transposing
the blocks and also potentially VNNI transforming them as well during transfer
to the register file.  The load form of this operation includes a block count
(or "array length") parameter to enable the user to fully pad out registers.
The store operation only stores one block.   See external documentation
for how these operations function as well as the constraints.

The `lsc_load_status` operation loads a status word about each address
given indicating the disposition of that address.

Append counter atomic operations lack an address and perform the operation
directly on the `Surface` with data from `Src0Data`.  They can read back
the result to `DstData` or avoid the writeback if `%null` is given as the
destination.  The surface `AddrType` must be stateful(i.e. BTI, SSS, or BSS).


NOTE: all operations have per-platform constraints.  The vISA backend will
emulate in some cases and raise errors in other cases.   However, not all
platform constrains can be checked.  The user should read external documentation
regarding the LSC messages to understand the per-platform behavior.




- **LscSubOp(ub):** The specific LSC operation being called

      .. _table_LSC_SubOp:

      .. table:: **LSC Operation Encoding**

        | Symbol                 | SubOp | Name                        | Extra atomic arguments |
        | --- | ---| ---| ---| ---|
        | lsc_load               | 0x00  | vector gathering load       | N/A                    |
        | lsc_load_strided       | 0x01  | vector strided block load   | N/A                    |
        | lsc_load_quad          | 0x02  | vector load w. XYZW ChEn    | N/A                    |
        | lsc_load_block2d       | 0x03  | 2D block load               | N/A                    |
        | lsc_store              | 0x04  | vector scattering store     | N/A                    |
        | lsc_store_strided      | 0x05  | vector strided block store  | N/A                    |
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
        | lsc_apndctr_atomic_add | 0x28  | append counter atomic add   | 1                      |
        | lsc_apndctr_atomic_sub | 0x29  | append counter atomic sub   | 1                      |

- **Exec_size(ub):** Execution size.  SIMT messages (non-transpose) should

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
        use the implicit execution system of the underlying hardware
        (SIMD16 for DG2 and SIMD32 for PVC).  Block operations or transpose operations
        should be SIMD1

- **Pred(uw):** Predication control


- **LscSFID(ub):** The specific LSC unit this operation applies to.


        ==============  ==================  =======================================================
        Symbol          vISA Enum           Notes
        ==============  ==================  =======================================================
        `.ugm`          `LSC_UGM`           Untyped global memory.
        `.ugml`         `LSC_UGML`          Low-bandwidth untyped global memory (cross tile).
        `.slm`          `LSC_SLM`           Shared local memory.
        ==============  ==================  =======================================================

- **CachingL1(ub):** Caching behavior for L1.

  - 0x0: default  (.df)
  - 0x1: uncached  (.uc)
  - 0x2: cached  (.ca)
  - 0x3: writeback  (.wb)
  - 0x4: writethrough  (.wt)
  - 0x5: streaming  (.st)
  - 0x6: read-invalidate  (.ri)

        Only certain combinations of this with CachingL3 are valid on hardware.
        The table below defines the allowable combinations in PVC.

        ====  ====  =======  ==========================================================================
        L1    L3    Applies  Notes
        ====  ====  =======  ==========================================================================
        .df   .df   both     default behavior on both L1 and L3 (L3 uses MOCS settings)
        .uc   .uc   both     uncached (bypass) both L1 and L3
        .st   .uc   both     streaming L1 / bypass L3
        .uc   .ca   loads    bypass L1 / cache in L3
        .ca   .uc   loads    cache in L1 / bypass L3
        .ca   .ca   loads    cache in both L1 and L3
        .st   .ca   loads    streaming L1 / cache in L3
        .ri   .ca   loads    read-invalidate (e.g. last-use) on L1 loads / cache in L3
        .uc   .wb   stores   bypass L1/ writeback L3
        .wt   .uc   stores   writethrough L1 / bypass L3
        .wt   .wb   stores   writethrough L1 / writeback L3
        .st   .wb   stores   streaming L1 / writeback L3
        .wb   .wb   stores   writeback both L1 and L3
        ====  ====  =======  ==========================================================================

        `These values must be set to default (.df) for SLM accesses`.

- **CachingL3(ub):** Caching behavior for L3

  - 0x0: default  (.df)
  - 0x1: uncached  (.uc)
  - 0x2: cached  (.ca)
  - 0x3: writeback  (.wb)
  - 0x4: writethrough  (.wt)
  - 0x5: streaming  (.st)
  - 0x6: read-invalidate  (.ri)

- **AddrType(ub):** The addressing mode type (model). Valid values are:

  - 0: invalid
  - 1: flat
  - 2: bss
  - 3: ss
  - 4: bti

- **AddrScale(uw):** An optional scale factor. The compiler may be able to fuse this into the message if it is the data type size (times the vector), otherwise additional instructions are generated to honor the semantics


- **AddrImmOffset(d):** An optional immediate offset added to each address.  The compiler may be able to fuse this add into the message, otherwise additional instructions are generated to honor the semantics


- **AddrSize(ub):** The address size enumeration. Valid values are:

  - 0: invalid
  - 1: a16
  - 2: a32
  - 3: a64

- **DataSize(ub):** The datum size per channel. Valid values are:

  - 0: invalid
  - 1: d8
  - 2: d16
  - 3: d32
  - 4: d64
  - 5: d8u32
  - 6: d16u32
  - 7: d16u32h

- **DataOrder(ub):** Indicates if the data is transposed during the transfer from register to memory


- **DataElemsPerAddr(ub):** The number of elements to load per address (vector size)

      .. _table_LSC_DataElemsPerAddr:

      .. table:: **LSC Operation Encoding**

        | Encoding | Meaning | Syntax                  |
        | --- | ---| ---| ---|
        | 0        | INVALID | invalid                 |
        | 1        | 1       | `..x1` (or omit suffix) |
        | 2        | 2       | `..x2`                  |
        | 3        | 3       | `..x3`                  |
        | 4        | 4       | `..x4`                  |
        | 5        | 8       | `..x8`                  |
        | 6        | 16      | `..x16`                 |
        | 7        | 32      | `..x32`                 |
        | 8        | 64      | `..x64`                 |

- **ChMask(ub):** Only valid for `lsc_load_quad` and `lsc_store_quad`, this holds channel-enable bits for the X, Y, Z, and W channels


- **Blocks(ub):** Only for block2d; this holds the count of 2d blocks to load into GRF (the array size)


- **BlockWidth(uw):** Only for block2d; this immediate value holds the 2D region's width (in elements)


- **BlockHeight(uw):** Only for block2d; this immediate value holds the 2D region's height (in elements)


- **VNNI(ub):** Only present in block2d operations.  This performs a VNNI transform during the access


- **Surface(vec_operand):** An optional surface to use for this operation.  This can be an immediate or a register.  Use the immediate value 0 for flat operations.  BTIs with known values can use an immediate.


- **DstData(raw_operand):** The register to store data loaded into for loads.  The null register may be used in case of a prefetch-load where none is needed.  This is null for stores or atomics without a return value


- **Src0Addrs(raw_operand):** The the vector register holding address operands


- **Src0AddrBase(raw_operand):** Only for strided; this holds the base address


- **Src0AddrPitch(vec_operand):** Only for strided; this holds the offset of successive elements relative to base address.  A value of 0 would be a broadcast load.  A value equal to the data type size would emulate a block (packed) load


- **SurfaceBase(vec_operand):** Surface state 64b surface base address (in bytes) (for block2d only).


- **SurfaceWidth(vec_operand):** Surface state surface width (in bytes) minus 1 (for block2d only).


- **SurfaceHeight(vec_operand):** Surface state surface height (in bytes) minus 1 (for block2d only).


- **SurfacePitch(vec_operand):** Surface state surface pitch (in bytes) (for block2d only).


- **Src0AddrX(vec_operand):** the 2d tile's base X position in elements to load or store (for block2d only).


- **Src0AddrY(vec_operand):** the 2d tile's base Y position in elements to load or store (for block2d only).


- **Src0Data(raw_operand):** This is for append counter atomic operations only as there are no address payload for them, so the Src0 is the data argument.


- **Src1Data(raw_operand):** This will hold the null register for load operations and the data payload for stores.  For atomics it holds the first argument of the atomic operation and is null if the atomic op is unary


- **Src2Data(raw_operand):** This will hold the null register for all but ternary atomic ops


#### Properties




## Text
```



```
      [ ( Pred ) ] lsc_load.SFID [ .CachingL1 [ .CachingL3 ] ]  ( ExecSize )
                DstData :   (DataSize x DataElemsPerAddr DataOrder)
                AddrType [ [ AddrScale * ] Src0Addrs [ + AddrImmOffset ] ]: AddrSize

      [ ( Pred ) ] lsc_load_strided.SFID [ .CachingL1 [ .CachingL3 ] ] [ ( ExecSize ) ]
                DstData :   (DataSize x DataElemsPerAddr DataOrder)
                AddrType [ [ AddrScale * ] Src0Addrs [ + AddrImmOffset ] [ , Src0AddrStride ] ]: AddrSize

      [ ( Pred ) ] lsc_load_block2d.SFID [ .CachingL1 [ .CachingL3 ] ] ( ExecSize )
                DstData : DataSize . Blocks x BlockWidth x BlockHeight [ nt ] [ nt ]
                AddrType [ SurfaceBase , SurfaceWidth , SurfaceHeight , SurfacePitch , Src0AddrX , Src0AddrY ]

      [ ( Pred ) ] lsc_store. SFID [ .CachingL1 [ .CachingL3 ] ] [ ( ExecSize ) ]
                AddrType [ [ AddrScale * ] Src0Addrs [ + AddrImmOffset ] ]: AddrSize
                Src1Data :  (DataSize x DataElemsPerAddr DataOrder)

      [ ( Pred ) ] lsc_store_strided.SFID [ .CachingL1 [ .CachingL3 ] ] ( ExecSize )
                AddrType [ [ AddrScale * ] Src0Addrs [ + AddrImmOffset ] [ , Src0AddrStride ] ]: AddrSize
                Src1Data :  (DataSize x DataElemsPerAddr DataOrder)

      [ ( Pred ) ] lsc_store_block2d.SFID [ .CachingL1 [ .CachingL3 ] ] ( ExecSize )
                AddrType [ SurfaceBase , SurfaceWidth , SurfaceHeight , SurfacePitch , Src0AddrX , Src0AddrY ]
                Src1Data : DataSize . [ 1x ] BlockWidth x BlockHeight [ nt ] [ nt ]

      [ ( Pred ) ] lsc_atomic*.SFID [ .CachingL1 [ .CachingL3 ] ] ( ExecSize )
                DstData :   (DataSize x DataElemsPerAddr DataOrder)
                AddrType [ [ AddrScale * ] Src0Addrs [ + AddrImmOffset ] ]: AddrSize
                Src1Data
                Src2Data
      [ ( Pred ) ] lsc_apndctr_atomic_*.SFID [ .CachingL1 [ .CachingL3 ] ] ( ExecSize )
                DstData :   (DataSize x DataElemsPerAddr DataOrder)
                AddrType [ state_offset ]
                Src0Data




```
```
## Examples



```

            // flat (stateless) 64b address load from V12 of 32b values (:d32) into V13,
            // bypassing L1 and L3 (.uc); add 0x100 to each address
            lsc_load.ugm.uc.uc  (M1,32) V13:d32    flat[V12+0x100]:a64



            // SLM load using 16b addresses from V12; each address loads a vector of
            // 4 x 32b elements (e.g. float4).
            // Vector elements are stored in successive registers (SIMT).
            lsc_load.slm   (M1,32) V13:d32x4  flat[V12]:a16

            // A prefetch command from 64b flat (stateless) addresses given in V12 of 32b
            // data values (1 per channel).
            // (uses default caching)
            lsc_load.ugm  (M1,32) null:d32  flat[V12]:a64
            lsc_load.ugm  (M1,32) V0:d32    flat[V12]:a64 // V0 is the null register in vISA

            // A32 transposed (array-of-struct order) load of 16 x 32b elements into V13
            // from binding table (bti) surface #4.
            // (uses default caching)
            lsc_load.ugm          (M1_NM,1)  V13:d32x16t  bti(0x4)[V12]:a32
            //
            // identical to the above
            lsc_load_strided.ugm  (M1_NM,16) V13:d32      bti(0x4)[V12]:a32
            //
            // A "block" load using a single DW address in V12, loads ExecSize elements
            // into V13.  A strided load only passes out a single register, thus improving
            // bandwidth from EU to LSC over the regular gathering load.
            lsc_load_strided.ugm   (M1,32) V13:d32  flat[V12]:a32
            // As above, but using a custom stride of 0x100 between elements.
            lsc_load_strided.ugm   (M1,32) V13:d32  flat[V12,0x100]:a32
            // A broadcast load from SLM (each channel loads the same 32b value)
            lsc_load_strided.slm   (M1,32) V13:d32  flat[V12,0x0]:a32



            // A flat (stateless) 64b address store to V12 of 32b data elements from V13.
            // (uses default caching)
            lsc_store.ugm     (M1,32) flat[V12]:a64  V13:d32


            // An SLM store to V12 of four SIMT 32b data elements from V13.
            // This is similar to an HDC untyped surface write with all four
            // channels enabled.
            lsc_store.slm     (M1,32) flat[V12]:a32  V13:d32x4


            // A 2d block load of two 2d blocks of bytes with height 16 and width 32
            // The data is loaded (n)on-transposed and (n)on-VNNI transformed.
            // The block's top coordinate (Y) is OFF_Y and the left (X) is OFF_X.
            // The 64b surface base is VSURF_BASE, V_SURF_W is the surface width,
            // V_SURF_H is the surface height, and VSURF_P is the surface pitch.
            lsc_load_block2d.ugm  (M1_NM,1)  VDATA:d8.2x16x32nn    flat[VSURF_BASE,VSURF_W,V_SURF_H,SURF_P,OFF_X,OFF_Y]
            //
            // Similar to above, but one 2d block of 16b-words with height 64 and width 16.
            // The data is loaded (t)ransposed, but (n)ot VNNI transformed as indicated
            // by the "tn" suffix.
            lsc_load_block2d.ugm  (M1_NM,1) VDATA:d16.1x32x16tn   flat[VSURF_BASE,VSURF_W,V_SURF_H,SURF_P,OFF_X,OFF_Y]
            //
            // As above but non-transposed and VNNI transformed
            lsc_load_block2d.ugm  (M1_NM,1) VDATA:d16.1x16x32nt   flat[VSURF_BASE,VSURF_W,V_SURF_H,SURF_P,OFF_X,OFF_Y]
            //
            // An LSC store block2d operation.  The address parameters are similar to above.
            lsc_store_block2d.ugm (M1_NM,1)  flat[VSURF_BASE,VSURF_W,V_SURF_H,SURF_P,OFF_X,OFF_Y]  VDATA:d16.16x32nn

            // Atomic add unsigned int of V10 to Surface(BTI is 0xA0)'s append_counter in memory and returns
            // the old value to VDATA.
      lsc_apndctr_atomic_add.ugm  (M1,32) VDATA:d32 bti(0xA0) V10:d32
```
## Notes





