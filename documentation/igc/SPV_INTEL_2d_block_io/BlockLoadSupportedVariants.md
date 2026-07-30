# Supported 2D block I/O variants

This document lists the variants supported by IGC for the following
instructions defined by
[`SPV_INTEL_2d_block_io`](https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_2d_block_io.html):

- `OpSubgroup2DBlockLoadINTEL`
- `OpSubgroup2DBlockLoadTransformINTEL`
- `OpSubgroup2DBlockLoadTransposeINTEL`
- `OpSubgroup2DBlockStoreINTEL`
- `OpSubgroup2DBlockPrefetchINTEL`

The SPIR-V extension permits device-specific restrictions on block dimensions.
Therefore, the extension specification remains the authoritative source for the
instruction semantics and general restrictions, while this document defines
the configurations supported by IGC.

## `OpSubgroup2DBlockLoadINTEL`

All Block Height values shown in a row are supported for that Element Size,
Block Width, Block Count, and SIMD configuration.

### Variants supported by XE HPC+ (PVC, BMG, LNL, PTL, NVL S, NVL P, CRI)

| Element Size (bytes) | Block Width (elements) | Block Height(s) (rows) | Block Count(s) | SIMD16 | SIMD32 |
|---------------------:|-----------------------:|:----------------------:|:--------------:|:------:|:------:|
| 1 | 4  | 1, 2, 4, 8     | 1       | ✓ | ✓ |
| 1 | 4  | 16, 32         | 1, 2, 4 | ✓ | ✓ |
| 1 | 8  | 1, 2, 4        | 1       | ✓ | ✓ |
| 1 | 8  | 8, 16, 32      | 1, 2, 4 | ✓ | ✓ |
| 1 | 16 | 1, 2           | 1       | ✓ | ✓ |
| 1 | 16 | 1              | 2, 4    | ✓ |   |
| 1 | 16 | 4, 8, 16, 32   | 1, 2, 4 | ✓ | ✓ |
| 1 | 32 | 1, 2, 4, 8, 16, 32 | 1, 2 | ✓ | ✓ |
| 1 | 64 | 1, 2, 4, 8, 16, 32 | 1    | ✓ | ✓ |
| 2 | 2  | 1, 2, 4, 8     | 1       | ✓ | ✓ |
| 2 | 2  | 1, 2, 4, 8     | 2, 4    |   | ✓ |
| 2 | 2  | 16, 32         | 1, 2, 4 | ✓ | ✓ |
| 2 | 4  | 1, 2, 4        | 1       | ✓ | ✓ |
| 2 | 4  | 1, 2, 4        | 2, 4    |   | ✓ |
| 2 | 4  | 8, 16, 32      | 1, 2, 4 | ✓ | ✓ |
| 2 | 8  | 1, 2           | 1       | ✓ | ✓ |
| 2 | 8  | 1, 2           | 2, 4    |   | ✓ |
| 2 | 8  | 4, 8, 16, 32   | 1, 2, 4 | ✓ | ✓ |
| 2 | 16 | 1, 2, 4, 8, 16, 32 | 1, 2 | ✓ | ✓ |
| 2 | 32 | 1, 2, 4, 8, 16, 32 | 1    | ✓ | ✓ |
| 4 | 1  | 1, 2, 4, 8, 16 | 1, 2 | ✓ |   |
| 4 | 1  | 32             | 1, 2 | ✓ | ✓ |
| 4 | 2  | 1, 2, 4, 8     | 1, 2 | ✓ |   |
| 4 | 2  | 16, 32         | 1, 2 | ✓ | ✓ |
| 4 | 4  | 1, 2, 4        | 1, 2 | ✓ |   |
| 4 | 4  | 8, 16, 32      | 1, 2 | ✓ | ✓ |
| 4 | 8  | 1, 2           | 1, 2 | ✓ |   |
| 4 | 8  | 4, 8, 16, 32   | 1, 2 | ✓ | ✓ |
| 4 | 16 | 1              | 1    | ✓ |   |
| 4 | 16 | 2, 4, 8, 16, 32 | 1  | ✓ | ✓ |
| 8 | 8  | 2              | 1 | ✓ |   |
| 8 | 8  | 4, 8, 16, 32  | 1 | ✓ | ✓ |
| 8 | 4  | 4              | 1 | ✓ |   |
| 8 | 4  | 8, 16, 32     | 1 | ✓ | ✓ |
| 8 | 2  | 8              | 1 | ✓ |   |
| 8 | 2  | 16, 32        | 1 | ✓ | ✓ |
| 8 | 1  | 16             | 1 | ✓ |   |
| 8 | 1  | 32             | 1 | ✓ | ✓ |

## `OpSubgroup2DBlockLoadTransformINTEL`

### Variants supported by XE HPC+ (PVC, BMG, LNL, PTL, NVL S, NVL P, CRI)

| Element Size (bytes) | Block Width (elements) | Block Height (rows) | Block Count(s) | SIMD16 | SIMD32 |
|---------------------:|-----------------------:|--------------------:|:--------------:|:------:|:------:|
| 1 | 4  | 4  | 1, 2, 4 | ✓ |   |
| 1 | 8  | 4  | 1, 2, 4 | ✓ |   |
| 1 | 16 | 4  | 1, 2, 4 | ✓ |   |
| 1 | 4  | 8  | 1, 2, 4 | ✓ |   |
| 1 | 8  | 8  | 1, 2, 4 | ✓ |   |
| 1 | 16 | 8  | 1, 2, 4 | ✓ | ✓ |
| 1 | 4  | 16 | 1, 2, 4 | ✓ |   |
| 1 | 8  | 16 | 1, 2, 4 | ✓ | ✓ |
| 1 | 16 | 16 | 1, 2, 4 | ✓ | ✓ |
| 1 | 4  | 32 | 1, 2, 4 | ✓ | ✓ |
| 1 | 8  | 32 | 1, 2, 4 | ✓ | ✓ |
| 1 | 16 | 32 | 1, 2, 4 | ✓ | ✓ |
| 2 | 2  | 2  | 1, 2, 4 | ✓ |   |
| 2 | 4  | 2  | 1, 2, 4 | ✓ |   |
| 2 | 8  | 2  | 1, 2, 4 | ✓ |   |
| 2 | 16 | 2  | 1, 2    | ✓ |   |
| 2 | 2  | 4  | 1, 2, 4 | ✓ |   |
| 2 | 4  | 4  | 1, 2, 4 | ✓ |   |
| 2 | 8  | 4  | 1, 2, 4 | ✓ |   |
| 2 | 16 | 4  | 1, 2    | ✓ | ✓ |
| 2 | 2  | 8  | 1, 2, 4 | ✓ |   |
| 2 | 4  | 8  | 1, 2, 4 | ✓ |   |
| 2 | 8  | 8  | 1, 2, 4 | ✓ | ✓ |
| 2 | 16 | 8  | 1, 2    | ✓ | ✓ |
| 2 | 2  | 16 | 1, 2, 4 | ✓ |   |
| 2 | 4  | 16 | 1, 2, 4 | ✓ | ✓ |
| 2 | 8  | 16 | 1, 2, 4 | ✓ | ✓ |
| 2 | 16 | 16 | 1, 2    | ✓ | ✓ |
| 2 | 2  | 32 | 1, 2, 4 | ✓ | ✓ |
| 2 | 4  | 32 | 1, 2, 4 | ✓ | ✓ |
| 2 | 8  | 32 | 1, 2, 4 | ✓ | ✓ |
| 2 | 16 | 32 | 1, 2    | ✓ | ✓ |

### Variants supported by XE2+ (BMG, LNL, PTL, NVL S, NVL P, CRI)

| Element Size (bytes) | Block Width (elements) | Block Height (rows) | Block Count(s) | SIMD16 | SIMD32 |
|---------------------:|-----------------------:|--------------------:|:--------------:|:------:|:------:|
| 1 | 32 | 4  | 1, 2 | ✓ | ✓ |
| 1 | 64 | 4  | 1    | ✓ | ✓ |
| 1 | 32 | 8  | 1, 2 | ✓ | ✓ |
| 1 | 64 | 8  | 1    | ✓ | ✓ |
| 1 | 32 | 16 | 1, 2 | ✓ | ✓ |
| 1 | 64 | 16 | 1    | ✓ | ✓ |
| 1 | 32 | 32 | 1, 2 | ✓ | ✓ |
| 1 | 64 | 32 | 1    | ✓ | ✓ |
| 2 | 32 | 2  | 1    | ✓ | ✓ |
| 2 | 32 | 4  | 1    | ✓ | ✓ |
| 2 | 32 | 8  | 1    | ✓ | ✓ |
| 2 | 32 | 16 | 1    | ✓ | ✓ |

For each platform group, configurations not listed in the applicable tables
are not supported.

## `OpSubgroup2DBlockLoadTransposeINTEL`

All Block Height values shown in a row are supported for that Element Size and
Block Width.

### Variants supported by XE HPC+ (PVC, BMG, LNL, PTL, NVL S, NVL P, CRI)

| Element Size (bytes) | Block Width (elements) | Block Height(s) (rows) | Block Count(s) | SIMD16 | SIMD32 |
|---------------------:|-----------------------:|:----------------------:|:--------------:|:------:|:------:|
| 4 | 1 | 1, 2, 4, 8, 16 | 1 | ✓ |   |
| 4 | 1 | 32             | 1 | ✓ | ✓ |
| 4 | 2 | 1, 2, 4, 8     | 1 | ✓ |   |
| 4 | 2 | 16, 32         | 1 | ✓ | ✓ |
| 4 | 4 | 1, 2, 4        | 1 | ✓ |   |
| 4 | 4 | 8, 16, 32      | 1 | ✓ | ✓ |
| 4 | 8 | 1, 2           | 1 | ✓ |   |
| 4 | 8 | 4, 8, 16, 32   | 1 | ✓ | ✓ |
| 8 | 2 | 8              | 1 | ✓ |   |
| 8 | 4 | 8              | 1 | ✓ | ✓ |

### Additional variants supported by XE3P+ (NVL P, CRI)

| Element Size (bytes) | Block Width (elements) | Block Height(s) (rows) | Block Count(s) | SIMD16 | SIMD32 |
|---------------------:|-----------------------:|:----------------------:|:--------------:|:------:|:------:|
| 4 | 16 | 1               | 1 | ✓ |   |
| 4 | 16 | 2, 4, 8, 16, 32 | 1 | ✓ | ✓ |
| 8 | 8  | 8               | 1 | ✓ | ✓ |

For each platform group, configurations not listed in the applicable tables
are not supported.

## `OpSubgroup2DBlockStoreINTEL`

All Block Height values shown in a row are supported for that Element Size and
Block Width.

### Variants supported by XE HPC+ (PVC, BMG, LNL, PTL, NVL S, NVL P, CRI)

| Element Size (bytes) | Block Width (elements) | Block Height(s) (rows) | Block Count(s) | SIMD16 | SIMD32 |
|---------------------:|-----------------------:|:----------------------:|:--------------:|:------:|:------:|
| 1 | 4  | 1, 2, 4, 8 | 1 | ✓ | ✓ |
| 1 | 8  | 1, 2, 4, 8 | 1 | ✓ | ✓ |
| 1 | 16 | 1, 2, 4, 8 | 1 | ✓ | ✓ |
| 1 | 32 | 1, 2, 4, 8 | 1 | ✓ | ✓ |
| 1 | 64 | 1, 2, 4, 8 | 1 | ✓ | ✓ |
| 2 | 2  | 1, 2, 4, 8 | 1 | ✓ | ✓ |
| 2 | 4  | 1, 2, 4, 8 | 1 | ✓ | ✓ |
| 2 | 8  | 1, 2, 4, 8 | 1 | ✓ | ✓ |
| 2 | 16 | 1, 2, 4, 8 | 1 | ✓ | ✓ |
| 2 | 32 | 1, 2, 4, 8 | 1 | ✓ | ✓ |
| 4 | 1  | 1, 2, 4, 8 | 1 | ✓ | ✓ |
| 4 | 2  | 1, 2, 4, 8 | 1 | ✓ | ✓ |
| 4 | 4  | 1, 2, 4, 8 | 1 | ✓ | ✓ |
| 4 | 8  | 1, 2, 4, 8 | 1 | ✓ | ✓ |
| 4 | 16 | 1, 2, 4, 8 | 1 | ✓ | ✓ |
| 8 | 1  | 1, 2, 4, 8 | 1 | ✓ | ✓ |
| 8 | 2  | 1, 2, 4, 8 | 1 | ✓ | ✓ |
| 8 | 4  | 1, 2, 4, 8 | 1 | ✓ | ✓ |
| 8 | 8  | 1, 2, 4, 8 | 1 | ✓ | ✓ |

Configurations not listed in this table are not supported.

## `OpSubgroup2DBlockPrefetchINTEL`

All combinations of the Block Height values and Block Count values shown in a
row are supported for that Element Size and Block Width.

### Variants supported by XE HPC+ (PVC, BMG, LNL, PTL, NVL S, NVL P, CRI)

| Element Size (bytes) | Block Width (elements) | Block Height(s) (rows) | Block Count(s) | SIMD16 | SIMD32 |
|---------------------:|-----------------------:|:----------------------:|:--------------:|:------:|:------:|
| 1 | 4  | 1, 2, 4, 8, 16, 32 | 1, 2, 4 | ✓ | ✓ |
| 1 | 8  | 1, 2, 4, 8, 16, 32 | 1, 2, 4 | ✓ | ✓ |
| 1 | 16 | 1, 2, 4, 8, 16, 32 | 1, 2, 4 | ✓ | ✓ |
| 1 | 32 | 1, 2, 4, 8, 16, 32 | 1, 2    | ✓ | ✓ |
| 1 | 64 | 1, 2, 4, 8, 16, 32 | 1       | ✓ | ✓ |
| 2 | 2  | 1, 2, 4, 8, 16, 32 | 1, 2, 4 | ✓ | ✓ |
| 2 | 4  | 1, 2, 4, 8, 16, 32 | 1, 2, 4 | ✓ | ✓ |
| 2 | 8  | 1, 2, 4, 8, 16, 32 | 1, 2, 4 | ✓ | ✓ |
| 2 | 16 | 1, 2, 4, 8, 16, 32 | 1, 2    | ✓ | ✓ |
| 2 | 32 | 1, 2, 4, 8, 16, 32 | 1       | ✓ | ✓ |
| 4 | 1  | 1, 2, 4, 8, 16, 32 | 1, 2 | ✓ | ✓ |
| 4 | 2  | 1, 2, 4, 8, 16, 32 | 1, 2 | ✓ | ✓ |
| 4 | 4  | 1, 2, 4, 8, 16, 32 | 1, 2 | ✓ | ✓ |
| 4 | 8  | 1, 2, 4, 8, 16, 32 | 1, 2 | ✓ | ✓ |
| 4 | 16 | 1, 2, 4, 8, 16, 32 | 1    | ✓ | ✓ |
| 8 | 1  | 1, 2, 4, 8, 16, 32 | 1 | ✓ | ✓ |
| 8 | 2  | 1, 2, 4, 8, 16, 32 | 1 | ✓ | ✓ |
| 8 | 4  | 1, 2, 4, 8, 16, 32 | 1 | ✓ | ✓ |
| 8 | 8  | 1, 2, 4, 8, 16, 32 | 1 | ✓ | ✓ |

### Additional variants supported by XE3P+ (NVL P, CRI)

| Element Size (bytes) | Block Width (elements) | Block Height(s) (rows) | Block Count(s) | SIMD16 | SIMD32 |
|---------------------:|-----------------------:|:----------------------:|:--------------:|:------:|:------:|
| 1 | 64  | 1, 2, 4, 8, 16, 32 | 4 | ✓ | ✓ |
| 1 | 128 | 1, 2, 4, 8, 16, 32 | 2 | ✓ | ✓ |
| 1 | 256 | 1, 2, 4, 8, 16, 32 | 1 | ✓ | ✓ |
| 2 | 32  | 1, 2, 4, 8, 16, 32 | 4 | ✓ | ✓ |
| 2 | 64  | 1, 2, 4, 8, 16, 32 | 2 | ✓ | ✓ |
| 2 | 128 | 1, 2, 4, 8, 16, 32 | 1 | ✓ | ✓ |
| 4 | 16  | 1, 2, 4, 8, 16, 32 | 4 | ✓ | ✓ |
| 4 | 32  | 1, 2, 4, 8, 16, 32 | 2 | ✓ | ✓ |
| 4 | 64  | 1, 2, 4, 8, 16, 32 | 1 | ✓ | ✓ |
| 8 | 8   | 1, 2, 4, 8, 16, 32 | 4 | ✓ | ✓ |
| 8 | 16  | 1, 2, 4, 8, 16, 32 | 2 | ✓ | ✓ |
| 8 | 32  | 1, 2, 4, 8, 16, 32 | 1 | ✓ | ✓ |

For each platform group, configurations not listed in the applicable tables
are not supported.


## Terminology

The table columns correspond to the instruction operands as follows:

- **Element Size** is the size of one block element, in bytes. The 8-bit elements from the source data are represented by an Element Size of 1.
- **Block Width** is the number of elements in each block row.
- **Block Height** is the number of rows in each block.
- **Block Count** is the number of blocks loaded. When more than one block is loaded, the blocks are arranged in row-major order.

All other requirements from the extension specification, including alignment,
memory geometry, operand type, and full-subgroup requirements, continue to
apply to every configuration listed above.
