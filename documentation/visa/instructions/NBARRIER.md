<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  NBARRIER = 0x60

## Format

| | | | | | |
| --- | --- | --- | --- | --- | --- |
| 0x60(NBARRIER) | Mode | Id | Type | Num_producers | Num_consumers |


## Semantics


```

      named barrier synchronization for a subgroup of threads within a thread group.
```

## Description





```
    Named barrier synchronization on barrier <id>. It enables general consumer-producer synchronization. Each barrier
    has the number of producers and the number of consumers. NBARRIER.signal has two forms: baseline one and general one.
    The general one takes both <num_producers> and <num_consumers>, as well as the type <type>. <type> indicates whether
    the calling thread is a producer-consumer thread (0), producer-only thread (1), or consumer-only thread (2).
    The baseline form takes a single <num_threads>, which is used for producer-consumer type with both <num_producers>
    and <num_consumers> being the same <num_threads>.

    A signal operation notifies the other threads that this thread has reached the barrier named <id>. Only consumer threads
    can issue wait operation to wait for all the other producer and consumer threads to reach this barrier before this thread
    can resume execution. All threads that participate in this phase of barrier <id> (the total of <num_producers> and
    <num_consumers> threads) must have identical values of <id>, <num_threads>, <num_producers>, and <num_consumers>.
    The barrier <id> is free and may be reused, potentially by a different subset of threads, once all threads complete
    their wait operation.

```


- **Mode(ub):**

  - Bit[0]: indicates whether this instruction is a barrier signal, wait

    - 0b0:  wait
    - 0b1:  signal

- **Id(scalar):** scalar variable storing the barrier id. Must have type UB. Valid values are  [0-31]


- **Type(scalar):** Type of this named barrier. This field is ignored for nbarrier.wait. Must have type UW. Valid values are  [0-2]


- **Num_producers(scalar):** number of producer threads that participate in this barrier. This field is ignored for nbarrier.wait. Must have type UB. Valid values are  [1-thread_group_size]


- **Num_consumers(scalar):** number of consumer threads that participate in this barrier. This field is ignored for nbarrier.wait. Must have type UB. Valid values are  [1-thread_group_size]


#### Properties




## Text
```



    NBARRIER.wait <id>

    NBARRIER.signal <id> <num_threads>                             // baseline
    NBARRIER.signal <id> <type> <num_producers> <num_consumers>    // general
```
## Notes





      This instruction may only be used with the thread-group execution model.

