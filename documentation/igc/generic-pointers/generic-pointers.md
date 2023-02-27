# Generic Address Space

One of the features of OpenCL 2.0 is the [generic address space](https://man.opencl.org/genericAddressSpace.html). Prior to OpenCL 2.0, the programmer had to specify an address space of what a pointer points to when that pointer was declared or the pointer was passed as an argument to a function. In OpenCL 2.0, if a named address space is not specified, it is treated as a generic address space by default meaning it can point to any of the named address spaces.

Generic address space allows programmers to write address space independent code which in many cases implies avoidance of code duplication. To demonstrate this, let's say that we want to write a function that prints the n<sup>th</sup> element of a buffer pointed by a pointer. In OpenCL 1.2 the code would look as follows:

```c
void printElement(global int* ptr, unsigned n)
{
    printf("Element[%d] = %d\n", n, ptr[n]);
}

void printElement(local int* ptr, unsigned n)
{
    printf("Element[%d] = %d\n", n, ptr[n]);
}

void printElement(private int* ptr, unsigned n)
{
    printf("Element[%d] = %d\n", n, ptr[n]);
}
```

Since a pointer can point to a memory residing in either `global`, `local`, or `private` address space, we are forced to implement three overloaded functions.

In OpenCL 2.0, the same functionality can be implemented with just a single function:

```c
void printElement(int* ptr, unsigned n)  // OpenCL2.0, no address space is treated as generic address space
{
    printf("Element[%d] = %d\n", n, ptr[n]);
}
```

## How Generic Pointers Are Represented In LLVM

In LLVM, each address space has its number assigned, so that address spaces can be easily distinguished from each other. Here is the enum which assigns numbers to address spaces in IGC:

```c
enum ADDRESS_SPACE : unsigned int
{
    ADDRESS_SPACE_PRIVATE = 0,
    ADDRESS_SPACE_GLOBAL = 1,
    ADDRESS_SPACE_CONSTANT = 2,
    ADDRESS_SPACE_LOCAL = 3,
    ADDRESS_SPACE_GENERIC = 4,
};
```

Generic address space pointers are created when a kernel source code contains a cast (either implicit or explicit) from a named address space to a generic address space. An example of implicit address space casts could look as follows:

```c
void printElement(int* ptr, unsigned n)  // OpenCL2.0, no address space is treated as generic address space
{
    printf("Element[%d] = %d\n", n, ptr[n]);
}

void kernel K(global int* ptr)
{
    printElement(ptr);  // <-- implicit address space cast from global to generic
}
```

LLVM code snippet representing a call to `printElement` function would look as below:

```llvm
  %ptr_as_generic = addrspacecast i32* %ptr to i32 addrspace(4)*                   ; i32* %ptr is a private address space pointer. LLVM treats addrspace(0) as default, therefore skips printing it.
  call spir_func void @printElement(i32 addrspace(4)* %ptr_as_generic, i32 0)
```

It's worth acknowledging that an `addrspacecast` instruction is the only way to create a generic address space pointer.

## How IGC Handles Generic Pointer Memory Accesses

Intel GPUs don't support generic pointers natively, therefore the entire burden of handling them lies directly on the software. Since there are no send instructions operating on a generic pointer, IGC must replace every `load`/`store` instruction with a sequence of instructions. The sequence is based on the tagging mechanism.

### Generic Pointer Tagging

Every time a generic pointer is created, it must be marked with the so-called `tag` which represents the address space of the pointer that it was cast from. We can think of a tag as data that stores information about the underlying, named address space of a pointer. Let's take a look at the following example of casting a private address space pointer to a generic address space pointer:

```llvm
%generic_ptr = addrspacecast i32* %private_ptr to i32 addrspace(4)*
```

Since tagging happens when `addrspacecast` instructions get emitted as VISA code, here is a VISA code snippet which represents creation of above generic address space pointer:

```c
and (M1, 16) V0042(0,0)<1> private_ptr(0,0)<1;1,0> 0x1fffffffffffffff:uq    // clear [61:63] bits
or (M1, 16) V0042(0,0)<1> V0042(0,0)<1;1,0> 0x2000000000000000:uq           // set [61:63] bits to 001
mov (M1, 16) generic_ptr(0,0)<1> V0042(0,0)<1;1,0>
```

As you can see, `addrspacecast` has been transformed into a sequence of `and`, `or` and `mov` instructions. Since an address is in a canonical form, bits `[61:63]` may be set either to `111` or `000` depending on a value of 47<sup>th</sup> bit, so it is necessary to clear them up before setting a proper tag. The `or` instruction is crucial here since it is responsible for setting a tag. One may ask: why do we change the memory address? Since GPU memory addresses are in a canonical form, even if they are 64-bit width, the actual virtual address takes only 48 bits that are sign-extended to 64-bits. IGC took advantage of it and reserved the three highest bits for a tag. Above `or` instruction is nothing more than setting `[61:63]` bits to a tag specific for a private address space.

Each address space has its own tag value assigned:

```c
private:  001
local:    010
global:   000/111
```

### Clearing A Generic Pointer Tag

Every time a generic pointer is cast back to a named address space, `[61:63]` bits of an address must be restored by clearing a tag, so that memory operation is executed on an original address. Please take a look at the example below:

```llvm
%private_ptr = addrspacecast i32 addrspace(4)* %generic_ptr to i32*
```

To preserve the canonical form (47<sup>th</sup> bit is replicated to the upper bits) of an address, clearing a tag is done by merging bits `[56:59]`, which we assume are in canonical form, into bits `[60:63]`.

```c
shl (M1, 16) V0068(0,0)<1> generic_ptr(0,0)<1;1,0> 0x4:d
asr (M1, 16) V0068(0,0)<1> V0068(0,0)<1;1,0> 0x4:d
mov (M1, 16) private_ptr(0,0)<1> V0068(0,0)<1;1,0>
```

### Resolving Generic Address Space Pointer Accesses At Runtime

Note: To understand this section, it is necessary to comprehendingly read section [Generic Pointer Tagging](#generic-pointer-tagging).

Since all generic pointers are tagged by addrspacecast during creation, each generic pointer should contain information about its underlying, named address space at bits `[61:63]`. The information can be used to resolve generic pointer memory accesses to a sequence of instructions, that are legal from a hardware point of view.

```llvm
  %1 = load i32, i32 addrspace(4)* %ptr, align 4
```

Assuming that no generic pointer related optimizations have been applied, the `load` instruction above would be resolved to the following sequence of instructions:

```llvm
  %1 = ptrtoint i32 addrspace(4)* %ptr to i64
  %2 = lshr i64 %1, 61  ; tag
  switch i64 %2, label %GlobalBlock [
    i64 1, label %PrivateBlock  ; 001
    i64 2, label %LocalBlock    ; 010
  ]

PrivateBlock:                                     ; preds = %entry
  %3 = addrspacecast i32 addrspace(4)* %ptr to i32*
  %privateLoad = load i32, i32* %3, align 4
  br label %6

LocalBlock:                                       ; preds = %entry
  %4 = addrspacecast i32 addrspace(4)* %ptr to i32 addrspace(3)*
  %localLoad = load i32, i32 addrspace(3)* %4, align 4
  br label %6

GlobalBlock:                                      ; preds = %entry
  %5 = addrspacecast i32 addrspace(4)* %ptr to i32 addrspace(1)*
  %globalLoad = load i32, i32 addrspace(1)* %5, align 4
  br label %6

6:                                                ; preds = %GlobalBlock, %LocalBlock, %PrivateBlock
  %7 = phi i32 [ %privateLoad, %PrivateBlock ], [ %localLoad, %LocalBlock ], [ %globalLoad, %GlobalBlock ]
```

This is the moment when all the dots connect. The sequence above represents a switch statement which is based on a value of a tag. There is one switch case per address space. Each switch case contains an `addrspacecast` instruction from generic to either global, local, or private addrspace and the corresponding `load` instruction which is not operating on a generic pointer anymore, so it can be transformed to a legal send instruction.

### Generic Address Space Optimizations

It should be acknowledged that such an expanded switch statement described in the section [Resolving Generic Address Space Pointer Accesses At Runtime](#resolving-generic-address-space-pointer-accesses-at-runtime) must get generated for each generic address space memory operation. One memory operation is transformed into three branches, so the negative performance overhead is huge. IGC tries its best to avoid the necessity to generate the switch statement by implementing several optimizations:

- Propagating named address space from `addrspacecast` instructions to their users to eliminate as many generic pointer uses as possible. This optimization is spread across multiple passes: `InferAddressSpacesPass`, `ResolveGASPass`, `createLowerGPCallArg`, `GASRetValuePropagatorPass`. More details in [Resolving Generic Address Space Pointer At Compile-Time](#resolving-generic-address-space-pointer-at-compile-time) section,
- Allocating private memory in a global buffer, so there is no need to distinguish between memory accesses to private and global memory. More details in [Private Memory Allocated In A Global Buffer](#private-memory-allocated-in-a-global-buffer).

#### Resolving Generic Address Space Pointer At Compile-Time

If it can be proved that a particular generic address space pointer never points to more than one address space, then all memory instructions that operate on it can be converted to instructions that operate on a named address space, thus avoiding the generation of a performance costly switch statement.

Here is a simple example:

```llvm
%generic_ptr = addrspacecast i32 addrspace(1)* %global_ptr to i32 addrspace(4)*
%v = load i32, i32 addrspace(4)* %generic_ptr, align 4
```

The above `load` instruction operates on a generic address space pointer which is always created from a global address space pointer, so it would be unoptimal to generate a switch statement for the `load` instruction, as only one switch case would be visited. IGC has the ability to propagate a named address space from `addrspacecast` up to its users to eliminate as many generic pointer uses as possible. The more memory operations are reached during the named address space propagation, the more efficient code is produced in the end. The above llvm code snipped would be propagated to the following sequence of instructions:

```llvm
%generic_ptr = addrspacecast i32 addrspace(1)* %global_ptr to i32 addrspace(4)*
%back_to_global_ptr = addrspacecast i32 addrspace(4)* %generic_ptr to i32 addrspace(1)*
%v = load i32, i32 addrspace(1)* %back_to_global_ptr, align 4
```

As you may notice, `load` instruction no longer operates on a generic address space pointer, so the performance overhead associated with the switch statement has been eliminated. This is only a trivial example. In a real case scenarios, compiled code is much more complex to the extent that generic address space needs to be propagated through `alloca` instructions, function calls etc.

#### Private Memory Allocated In A Global Buffer

To minimize the negative performance implications caused by [Resolving Generic Address Space Pointer Accesses At Runtime](#resolving-generic-address-space-pointer-accesses-at-runtime), IGC allocates private memory in a global address space when generic pointers are used in a kernel. This allows private memory operations to be treated as global memory operations, so there is no need to distinguish between them.

It gives the following optimization opportunities:

- Generating private branch for switch statement generated during runtime generic address space resolution can be avoided. Please find details in [Dynamic Generic Address Space Resolution Without Branch For Private Memory](#dynamic-generic-address-space-resolution-without-branch-for-private-memory) section,
- If local memory is not pointed by any generic pointer, all generic address space memory operations can be statically resolved to global memory operations. In other words, generating branches for private and local memory can be avoided. Please find details in [Static Resolution Of Generic Pointer Memory Accesses When Local Memory Is Not Used](#static-resolution-of-generic-pointer-memory-accesses-when-local-memory-is-not-used) section.

##### Dynamic Generic Address Space Resolution Without Branch For Private Memory

If private memory is allocated in a global address space, the following load operation:

```llvm
%v = load i32, i32 addrspace(4)* %ptr, align 4
```

can be dynamically resolved with avoidance of private branch generation:

```llvm
  %1 = ptrtoint i32 addrspace(4)* %ptr to i64
  %2 = lshr i64 %1, 61  ; tag
  switch i64 %2, label %GlobalBlock [
    i64 2, label %LocalBlock    ; 010
  ]

LocalBlock:                                       ; preds = %entry
  %4 = addrspacecast i32 addrspace(4)* %ptr to i32 addrspace(3)*
  %localLoad = load i32, i32 addrspace(3)* %4, align 4
  br label %6

GlobalBlock:                                      ; preds = %entry
  %5 = addrspacecast i32 addrspace(4)* %ptr to i32 addrspace(1)*
  %globalLoad = load i32, i32 addrspace(1)* %5, align 4
  br label %6

6:                                                ; preds = %GlobalBlock, %LocalBlock, %PrivateBlock
  %7 = phi i32 [ %localLoad, %LocalBlock ], [ %globalLoad, %GlobalBlock ]
```

##### Static Resolution Of Generic Pointer Memory Accesses When Local Memory Is Not Used

If a compiler detects that there are no `addrspacecast` instructions from the local address space to generic address space in a compiled kernel, then it is guaranteed that no generic pointers point to a local memory. Combining this information with the fact that the [Private Memory Allocated In A Global Buffer](#private-memory-allocated-in-a-global-buffer) optimization is enabled gives a guarantee that all generic pointers point to a global address space.

If both conditions are met, IGC can resolve all generic pointer memory accesses without generating a performance-killing switch statement described in [Resolving Generic Address Space Pointer Accesses At Runtime](#resolving-generic-address-space-pointer-accesses-at-runtime). All generic memory accesses can simply be statically resolved to a global memory accesses:

```llvm
%global_ptr = addrspacecast i32 addrspace(4)* %generic_ptr to i32 addrspace(1)*
%v = load i32, i32 addrspace(1)* %global_ptr, align 4
```

### Generic Address Space Explicit Casts

In some cases a programmer may want to write a function that operates in a generic fashion but there are some operations needed for a specific address space. In such a case, there are built-ins to help for these portions of a function. The functions `to_global()`, `to_local()`, `to_private()` can be used to cast a generic pointer to the respective address space. If for some reason these functions are not able to cast a pointer to the respective address space they will return NULL. This allows the programmer to know if a pointer can be treated as if it points to the respective address space or not.

```c
void F(int* generic_ptr)
{
    // ... generic code

    if(to_global(generic_ptr))
    {
        // ... code specific for global address space
    }
    else if(to_local(generic_ptr))
    {
        // ... code specific for local address space
    }
    else if(to_private(generic_ptr))
    {
        // ..code specific for private address space
    }

    // ... generic code
}

void kernel K(global int* buffer)
{
    F(buffer);
}
```

If the compiler cannot manage to resolve these builtins at compile-time by named address space propagation, appropriate llvm code sequence must be generated to handle a logic of these builtins at runtime. To achievie this, IGC generates ifelse statement which depends on a tagging mechanism described in depth in section [Generic Pointer Tagging](#generic-pointer-tagging).

Here is an example of the llvm code sequence that gets generated for `to_private` builtin function:

```llvm
  %1 = ptrtoint i8 addrspace(4)* %generic_ptr to i64
  %2 = lshr i64 %1, 61
  %cmpTag = icmp eq i64 %2, 1   ; private: 001
  br i1 %cmpTag, label %IfBlock, label %ElseBlock

IfBlock:                                          ; preds = %entry
  %3 = addrspacecast i8 addrspace(4)* %1 to i8*
  br label %4

ElseBlock:                                        ; preds = %entry
  br label %4

4:                                                ; preds = %ElseBlock, %IfBlock
  %call = phi i8* [ %3, %IfBlock ], [ null, %ElseBlock ]
```

These builtins imposes on the compiler to distinguish between generic pointer initialized with a private and a global pointers. It forces IGC to implement a special behavior when [Private Memory Allocated In A Global Buffer](#private-memory-allocated-in-a-global-buffer) optimization is enabled.

#### Special Behaviour When Private Memory Is Allocated In A Global Buffer While Explicit Casts Are Used In A Kernel

One might think that when explicit casts force to distinguish private from global pointers, then [Private Memory Allocated In A Global Buffer](#private-memory-allocated-in-a-global-buffer) optimization should be disabled since it implicates treating private and global accesses as they operate in the same address space. That seems reasonable, but it would implicitate the necessity to generate private branch described in ["Dynamic generic address space resolution without branch for private memory"](#dynamic-generic-address-space-resolution-without-branch-for-private-memory).

To avoid performance slippage when explicit casts are used in a kernel, IGC still keeps [Private Memory Allocated In A Global Buffer](#private-memory-allocated-in-a-global-buffer) optimization enabled, but it needs to follow these steps to keep the code fully functional:

1. **Private pointers tagging must be enabled so that explicit casts can distinguish them from global pointers**.

    ```llvm
    %generic_ptr = addrspacecast i32* %private_ptr to i32 addrspace(4)*   ;  tag set to 001 due to presence of explicit casts in a kernel
    %v = load i32, i32 addrspace(4)* %generic_ptr, align 4
    ```

2. **Enable [Clearing A Generic Pointer Tag](#clearing-a-generic-pointer-tag) for generic pointers casted back to global address space**.

    Since IGC uses original values of `[61:63]` bits of an address (either `000` or `111`) as a tag for global pointers, clearing them when casting generic pointer back to a global pointer is not necessary by default. But since IGC may apply [Static Resolution Of Generic Pointer Memory Accesses When Local Memory Is Not Used](#static-resolution-of-generic-pointer-memory-accesses-when-local-memory-is-not-used) optimization, it is possible that a generic pointer created from a private pointer may be transformed back to a global address space, thereby not clearing a private tag before executing a load operation. Therefore, to avoid executing a load instruction with a tagged pointer, the tag must be cleared when casting a pointer from the generic to the global address space:

    ```llvm
    %generic_ptr = addrspacecast i32* %private_ptr to i32 addrspace(4)*                ;  tag set to 001 due to presence of explicit casts in a kernel
    %global_ptr = addrspacecast i32 addrspace(4)* %generic_ptr to i32 addrspace(1)*    ;  addrspacecast inserted by "Static Resolution Of Generic Pointer Memory Accesses When Local Memory Is Not Used"
                                                                                       ;    tag must be cleared to avoid executing a load instruction with a tagged pointer
    %v = load i32, i32 addrspace(1)* %global_ptr, align 4
    ```
