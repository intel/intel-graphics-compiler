<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  NBARRIER = 0x60

## Format

| | | | |
| --- | --- | --- | --- |
| 0x60(NBARRIER) | Mode | Id | Num_threads |


## Semantics


```

      named barrier synchronization for a subgroup of threads within a thread group.
```

## Description





```
    Named barrier synchronization on barrier <id>. A signal operation notifies the other threads that this thread has
    reached the barrier named <id>. A wait operation waits for all the other threads to reach this barrier before this
    thread can resume execution. All threads that participate in this phase of barrier <id> (a total of <num_threads>
    threads) must have identical values of <id> and <num_threads>. The barrier <id> is free and may be reused,
    potentially by a different subset of threads, once all threads complete their wait operation.
```


- **Mode(ub):**

  - Bit[0]: indicates whether this instruction is a barrier signal or wait

    - 0b0:  wait
    - 0b1:  signal

- **Id(scalar):** scalar variable storing the barrier id. Must have type UB. Valid values are  [0-31]


- **Num_threads(scalar):** number of threads that participate in this barrier. This field is ignored for nbarrier.wait but must be present in the binary. Must have type UB. Valid values are  [1-thread_group_size]


#### Properties




## Text
```



    NBARRIER.wait <id>    // barrier wait

    NBARRIER.signal <num_threads> <id> // barrier signal
```
## Notes





      This instruction may only be used with the thread-group execution model.

