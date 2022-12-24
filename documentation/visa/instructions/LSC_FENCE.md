<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  LSC_FENCE = 0x8b

## Format

| | | | | | |
| --- | --- | --- | --- | --- | --- |
| 0x8b(LSC_FENCE) | ExecSize | Pred | LscSFID | LscFenceOp | LscScope |


## Semantics


```

    wait_for_pending_accesses_to_complete(LscSFID, LscFenceOp, LscScope)
    Dest.data = 0 // Dest. is implicit within vISA
```

## Description





    Wait until all previous accesses by this thread to this dataport are observable in the scope specified.  Then optionally flush the cache.


- **ExecSize(ub):** Execution size.  Should always be 1 and no mask


- **Pred(uw):** Predication control


- **LscSFID(ub):** The specific LSC shared function to fence

      .. _table_LSC_SFID:

      .. table:: **LSC_SFID**

        | Value | Symbol | vISA Enum | Notes                                            |
        | --- | ---| ---| ---| ---|
        | 0x0   | ugm    | LSC_UGM   | Untyped global memory                            |
        | 0x1   | ugml   | LSC_UGML  | Low-bandwidth untyped global memory (cross tile) |
        | 0x2   | tgm    | LSC_TGM   | Typed global memory                              |
        | 0x3   | slm    | LSC_SLM   | Shared local memory                              |

- **LscFenceOp(ub):** The fence operation to apply

      .. _table_LSC_FENCE_OP:

      .. table:: **LSC_FENCE_OP**

        | Value | Symbol     | vISA Enum               | Notes                                                                                                           |
        | --- | ---| ---| ---| ---|
        | 0x0   | none       | LSC_FENCE_OP_NONE       | No operation                                                                                                    |
        | 0x1   | evict      | LSC_FENCE_OP_EVICT      | Dirty lines will be evicted and invalidated from the L1 cache.  All clean lines will be invalidated             |
        | 0x2   | invalidate | LSC_FENCE_OP_INVALIDATE | Invalidate all Clean lines in the cache.  Don't evict the dirty lines (they stay in M state)                    |
        | 0x3   | discard    | LSC_FENCE_OP_DISCARD    | Direct and clean lines are discarded (invalidated) without eviction                                             |
        | 0x4   | clean      | LSC_FENCE_OP_CLEAN      | Dirty lines are written to memory, but retained in cache in the "Clean" state.  Clean lines are not invalidated |
        | 0x5   | flushl3    | LSC_FENCE_OP_FLUSHL3    | Flush L3 only                                                                                                   |

- **LscScope(ub):** The scope that this operation should apply to

      .. _table_LSC_SCOPE:

      .. table:: **LSC_SCOPE**

        | Value | Symbol | vISA Enum        | Notes                                                                                                                                                                                                                   |
        | --- | ---| ---| ---| ---|
        | 0x0   | group  | LSC_SCOPE_GROUP  | Flush out to the threadgroup's scope                                                                                                                                                                                    |
        | 0x1   | local  | LSC_SCOPE_LOCAL  | Flush out to the local scope (DSSs)                                                                                                                                                                                     |
        | 0x2   | tile   | LSC_SCOPE_TILE   | Tile (out to several DSSs)                                                                                                                                                                                              |
        | 0x3   | gpu    | LSC_SCOPE_GPU    | The entire GPU (out to the GPUs LLC)                                                                                                                                                                                    |
        | 0x4   | gpus   | LSC_SCOPE_GPUS   | All GPUs in the system (memory shared by all GPUs)                                                                                                                                                                      |
        | 0x5   | system | LSC_SCOPE_SYSTEM | Wait until all previous memory transactions from this thread are observed at the system level                                                                                                                           |
        | 0x6   | sysacq | LSC_SCOPE_SYSACQ | For GPUs that do not follow PCIe write ordering for downstream writes targeting device memory, a fence message with this scope will commit to device memory all downstream and peer writes that have reached the device |

#### Properties




## Text
```



  LSC_FENCE.<LscSFID>.<LscFenceOp>.<LscScope>
```
## Examples



```

            // Fences across the entire system for untyped global memory
            lsc_fence.ugm.clean.sysrel

            // Fences SLM
            lsc_fence.slm.clean.group
```
## Notes





