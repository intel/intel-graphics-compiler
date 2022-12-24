<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

# Execution Model

The virtual ISA machine model is based on the Single Instruction
Multiple Data (SIMD) architecture, where in each clock cycle the
execution unit performs the same operation on different data. Virtual
ISA instructions include an execution size item specifying the number of
SIMD channels for this instruction. The execution size determines the
size of the region for source and destination operands in an
instruction. The actual data elements used in the instruction are
determined by the region of the operand. Three bits are used to encode
the execution size, and valid values are:


| Binary Encoding|   | Execution size    |
| --- | --- | --- |
| 0b000           | 1  | 1 element (scalar) |
| 0b001           | 2  | 2 elements         |
| 0b010           | 4  | 4 elements         |
| 0b011           | 8  | 8 elements         |
| 0b100           | 16 | 16 elements        |
| 0b101           | 32 | 32 elements        |


Virtual ISA instructions typically have one destination operand and
one-to-three source operands. The operands must reside in register
space; special instructions exist to transfer data between register
storage and surface memory. Virtual ISA instructions support
region-based register addressing as well as both direct and
indirect variable accesses.

Concepts
========

A virtual ISA object contains one or more kernel/function objects as
well as any number of file-scope variable declarations. A kernel may be
invoked only from the host program, while a function may be invoked by a
kernel or another function. Each kernel and function object has its own
single name space, and variables declared within a kernel or function is
only visiable to that object.

To facilitate separate compilation, a kernel and the
functions/file-scope variables it uses do not have to be placed in the
same vISA file. Virtual ISA follows the C linkage model, and file-scope
variables and functions may have one of the three linkage specifiers:

-   **External:** This declaration is visible to all other vISA objects
    that may be linked with this vISA object. Somewhere in the entire
    vISA program there should be exactly one definition (global linkage)
    for this function or variable.
-   **Static:** This declaration is visible to other kernels and
    functions in the same vISA object. This also serves as a definition
    for the entity.
-   **Global:** This is both a declaration and a definition for the
    function or variable, and it is visible to the entire program. Each
    global definition in a vISA program must have a unique name.

Thread Organization
===================

At runtime, a virtual ISA kernel may be instantiated with user-specified
number of threads, with each thread executing an instance of the kernel
on GEN hardware. A kernel terminates when all of its threads have
finished execution. Each thread has its own dedicated register space
called the General Register File (GRF). Each GRF register consists of 32
bytes of data; the actual register size for each platform may also be
queried from the vISA finalizer.

For PVC platform, GRF register is 64 bytes.

As such, all virtual ISA variables, including the file-scope variables,
are thread private, and threads communicate through special memory
access instructions to the surfaces, which are shared among all threads.

In the **media** execution mode, a 2-dimensional index space is defined
when a kernel is launched, and each thread is assigned a unique pair (x,
y) as its global ID. Kernel code can read the values of this unique
thread identifier through special variables. The index space can
optionally be associated with a dependency pattern that defines
the execution order of the threads \[5\].

An alternative **GPGPU** thread-group based execution model is also
available. Instead of having a single index space, threads can be
further organized into thread groups. A thread group is defined as a
3-dimensional index space with fixed size (i.e., every thread group in a
kernel dispatch has the same size), and each thread within a group is
assigned a unique tuple of local id. Thread groups are similarly
organized into a 3-dimensional index space, and each thread group is
also assigned a unique pair of group id. A thread can therefore be
uniquely identified globally by a combination of its local id and thread
group id. Threads in the same thread group may communicate and
synchronize with each other through the shared local memory.
Kernel code can read the values of a thread's
local and group identifier as well as the size of the group through
special pre-defined variables.

Control Flow
============

Execution Mask
--------------

Each GEN EU consists of 32 SIMD execution masks which are initialized at
program entry based on the dispatch SIMD size (one of 8/16/32). During
execution the mask may be disabled and re-enabled by control flow
instructions to simulate divergent control flow where each channel may
execute a different code path depending on the results of branch
instructions.

As following table shows, each vISA instruction contains
an execution mask (EM) field to provide explicit control over the
enabled channels for the instruction.


| Binary Encoding for execution mask control| EM Offset| Text Format|
| --- | --- | --- |
| 0b0000                                     | 0         | M1          |
| 0b0001                                     | 4         | M2          |
| 0b0010                                     | 8         | M3          |
| 0b0011                                     | 12        | M4          |
| 0b0100                                     | 16        | M5          |
| 0b0101                                     | 20        | M6          |
| 0b0110                                     | 24        | M7          |
| 0b0111                                     | 28        | M8          |
| 0b1000                                     | None      | M1_NM       |
| 0b1001                                     | None      | M2_NM       |
| 0b1010                                     | None      | M3_NM       |
| 0b1011                                     | None      | M4_NM       |
| 0b1100                                     | None      | M5_NM       |
| 0b1101                                     | None      | M6_NM       |
| 0b1110                                     | None      | M7_NM       |
| 0b1111                                     | None      | M8_NM       |


Other values are reserved. *EM\[i\]... EM\[i+k\]* will be applied to the
instruction, where *i* is the starting offset and k the execution size.
It is an error if the starting offset selected by the mask control is
not aligned to the execution size, or if *i + k* is greater than or
equal to the SIMD control flow block's execution size. Program control
flow may also be explicitly overridden with the **NM** (NoMask) control,
which causes the instruction to ignore the EMs and write to every
channel subject to predication. The M1-M8 mask control is specified
together with {NoMask} as they are also used to control the predicate
variable offset.

Branch Instructions
-------------------

A goto instruction takes a label as its target, and the corresponding
label instruction marks the start of a basic block. For a forward goto
where the target label is lexically after the instruction, execution
continues to the next instruction but with all active and predicated
channels disabled; such channels will be re-activated when the program
reaches the target label. If there are no active channels due to
predication, execution will jump to the next program point where some
channels are waiting to be re-activated (typically the target label of
this or another goto instruction).

For a backward goto where the target label is lexically before the
instruction, execution jumps to the label while the active and
non-predicated channels are disabled; such channels will be reactivated
when the program reaches the instruction after the goto. If there are no
active and predicated channels at the goto, execution will continue to
the next instruction.

Following example shows use of goto instructions
implementing if-else-endif and do-while loop.

```
    cmp.ne (8)  P1 V1(0,0)<8;8,1>  0
    (!P1) goto (8) ELSE1      ...
      cmp.gt (8) P2  V2(0,0)<8;8,1>  1:ud
      (!P2) goto ENDIF2
        addr_add (1) A1(0)<1>  A0(0)<1>  4:uw  {NoMask}
        mov (8) [A1(0), 0]<1>  0:ud
      ENDIF2:
    goto (8) ENDIF1
    ELSE1:
    ...
    ENDIF1:

    LOOP_START:
      ...
      cmp.eq (16) P2 V2(0,0)<8;8,1>  0:ud
        (P2) goto (16) LOOP_END
      ...
      cmp.gt (16)  P1 V1(0,0)<8;8,1>  0:ud
    (P1) goto (16) LOOP_START
    LOOP_END:
```

In the common special case where the branch is guaranteed to be
non-divergent, a **jump** instruction may be used instead.

Subroutine and Function Calls
-----------------------------

A subroutine is defined as a sequence of instructions that can only be
entered with a "call" instruction outside the sequence and exited
through a "ret" instruction in the sequence. A subroutine must start
with a "subroutine" instruction and must be terminated with a "ret"
instruction, though early return is permitted. A kernel or function may
include any number of subroutines, all of which share the same symbol
table as the enclosing kernel or function. A subroutine may invoke other
subroutines, although recursion is not permitted.

A kernel or function may invoke another function through the "fcall"
instruction. Argument passing is achieved through a single special "arg"
variable. The fcall instruction
initializes the arg variable in the called function with the value of
the arg variable in the calling function, and also destroys the contents
of the arg variable in the caller during the process. If the size of the
function arguments exceeds that of the arg variable, overflow arguments
may be passed via memory, e.g., through the call stack. The vISA
specification does not contain any provisions about a program's call
stack; it is the vISA program's responsibility to allocate stack
memory, partition it among the concurrent threads, and detect stack
overflow. It is also up to the vISA program to initialize the built-in
stack pointer and frame pointer to the appropriate offset for each
thread. The fcall instruction also copies the values of SP and FP from
the caller to the callee.

The "fret" instruction terminates the current function and returns
control to its caller. The return value is passed through the special
"retval" variable. The fret instruction copies the value of the retval
variable from the called function to that of the calling function. It
also copies the value of the stack and frame pointer variable from the
callee to the caller. Recursive calls are permitted both directly and
indirectly.

Both subroutine and function calls are permitted inside divergent
control flow. To support divergent calls, a per-kernel 32-bit **call
mask** vector is maintained. At the entrypoint of a subroutine or
function, both the execution mask and the call mask are initialized to
be the set of active channels at the call site subject to predication.
At a "ret" or "fret" instruction, the active channels that are
predicated have their call mask and execution mask bit turned off, as
the channel will no longer execute the subroutine/function body. If the
call mask becomes all zero, execution returns to the caller, and the
call mask is restored to its previous value before the call instruction.

Mixing Uniform and Divergent Control Flow
-----------------------------------------

A kernel may use both uniform (jump) and divergent (goto) branch
instructions, but it is the kernel's responsibility to ensure that the
execution will eventually converge for the goto instructions. In
practical terms, it means jump and goto instructions should be properly
nested so that an uniform branch will never jump over a label where some
channels may be waiting to be re-activated (i.e., a re-convergence
point). Similarly, a jump or goto instruction may not be used to
enter/exit a subroutine.

Memory Access
=============

Memory access in vISA is performed through special read/write
instructions, and there are three address models.

Surface-Based Access
--------------------

In this model, memory access is done through *surfaces*, which are
linear, 2D, or 3D memory objects created by the host. Each surface has
parameters such as its format, layout, location, and size, which are
collectively stored as a surface state object in graphics memory. GEN
provides a 256-entry binding table where each entry contains a pointer
to the surface state of the underlying surface. A vISA surface variable
stores the value of the binding table index (BTI). It can either be
passed in as a kernel argument or explicitly assigned in the kernel
depending on the runtime and compiler ABI.

A surface-based vISA memory access instruction takes a surface variable
as well as positive offsets into the surface. The hardware looks up the
surface's base address from its surface state pointed to by the BTI and
applies the offsets to form the final graphics memory addresses. A
memory load or store instruction may impose restrictions on the format
and dimension of the surface, e.g., some instructions may only access
linear buffer while others may only operate on 2D surfaces. It is the
program's responsibility to ensure that the surface being accessed
satisfies the instruction's requirements.

Similarly, GEN provides a 32-entry sampler state binding table where
each entry points to a sampler state object. A vISA sampler variable
stores the value of the sampler binding table index, and like surface
variables it may either be passed in or explicitly assigned in the
kernel.

A special reserved "T252" surface variable can be used to support the
bindless surface model. In the bindless model, the T252 variable
contains the actual graphics memory address of the surface state object.
When the special variable is used in a surface-based load or store
instruction, the hardware obtains the surface's base address from the
surface state object located at the address of the T252 variable. It is
again the program's responsibility to ensure that at each access T252
contains the correct surface state address. Unless otherwise specified,
the bindless surface variable may be used in every surface-based memory
access instruction. The special reserved "S31" sampler variable serves
the same purpose for bindless sampler operations.

Shared Local Memory (SLM)
-------------------------

The shared local memory (SLM) is a high-bandwidth memory for threads in
a thread group to share data. Data in the SLM have the same lifetime as
a thread group; its contents are uninitialized at creation, and its
contents disappear when the thread group finishes execution. All threads
in a thread group may read and write to any address in the SLM. Only a
subset of memory operations is supported for SLM, and an SLM access is
indicated by the use of the special surface variable T0 in those
instructions. Threads in the same thread
group can synchronize their SLM accesses through the barrier
instruction, which guarantees memory ordering by ensuring that a
thread's accesses prior to the barrier have been performed with respect
to all other threads in the same group before the thread can continue
execution. In other words, the barrier instruction also serves as a
memory barrier for all SLM accesses. SLM is allocated in dedicated
memory storage and will never conflict with host-created surfaces.
Out-of-bound SLM accesses have undefined behavior.

Shared Virtual Memory (SVM)
---------------------------

In this model, the host may designate part (or all) of its virtual
address space to be shared with the kernel, and the kernel can directly
access any virtual address without a surface variable. This allows host
code to share data structures containing embedded pointers with the
kernel, as the CPU and GPU can access shared data using the same virtual
address. Shared virtual memory access is done exclusively through the
SVM instruction, which takes virtual addresses instead of surface
variables as operands. The runtime system guarantees that SVM region in
the virtual address space will not overlap with that of host-created
surfaces.

Memory Model
------------

Memory access instructions issued by the same thread are guaranteed to
appear as if they execute in program order, with the following
exceptions:

-   If a single instruction performs multiple memory accesses, the order
    for which the accesses are performed is not guaranteed. For example,
    a scatter write instruction is capable of writing to 16 addresses at
    once, and if it attempts to write the same address with different
    values, the result is undefined.
-   Sampler instructions are not consistent with memory access
    instructions. If a kernel has a sampler instruction that follows a
    memory write instruction to the same surface, it must use the
    ***fence*** instruction to flush the sampler cache in order to
    ensure that the sampler message returns the value from the write.

Different threads may simultaneously access memory as long as the
program is data-race free; the behavior of a program with data race is
undefined. vISA supports a number of synchronization primitives
including barrier, wait, and atomic operations, which can be used to
prevent data races by imposing a temporal order on memory accesses to
the same dword. The fence instruction should be applied when there are
data dependencies between threads, as it ensures that all previous reads
and writes from the issuing thread have been globally observed.

The host and the kernel may also simultaneously access SVM, but the
behavior is undefined if there is a data race between them. Data
dependencies between the host and the kernel through SVM should
similarly be resolved with the fence instruction, and host-kernel
synchronization may be achieved through the SVM atomic write operations.
