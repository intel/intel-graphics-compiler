<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  RT_READ = 0x90

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x90(RT_READ) | Exec_size | Pred | Mode | Surface | SI | Dst |


## Semantics


```

  This message takes 16 or 32 pixels for reads to a render target. This message is intended only for use by pixel shader kernels for reading data from render targets.
  A SIMD16 writeback message consists of 4 destination registers and SIMD32 writeback consists of 8 registers.
```

## Description






- **Exec_size(ub):** Execution size

  - Bit[2..0]: size of the region for source and destination operands

    - 0b100:  16 elements
    - 0b101:  32 elements
  - Bit[7..4]: execution mask (explicit control over the enabled channels)

    - 0b0000:  M1
    - 0b0100:  M5
    - 0b1000:  M1_NM
    - 0b1100:  M5_NM

- **Pred(uw):** Predication control


- **Mode(uw):**  Various bits control the parameters of the RT read.

  - Bit[0]: "PS" whether Per Sample Bit is set

  - Bit[1]: "SI" whether Sample Index is used


- **Surface(ub):** Surface variable index


- **SI(vec_operand):** SI Index. Present only of SI bit is set in Mode


- **Dst(raw_operand):** The raw operand of a general variable storing the results of the read


#### Properties




## Text
```





[(<P>)] RT_READ[.<Mode>] (<Exec_size>) <Surface> [<SI>]

// <Mode> is of the form [<PS><SI>] and may be in any order.
```

