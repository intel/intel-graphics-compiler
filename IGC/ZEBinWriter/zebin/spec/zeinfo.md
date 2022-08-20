<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

# ZE Info
Version 1.14

## Grammar

~~~
attribute_seq   -> attribute, attribute_seq
                   attribute

attribute       -> attribute_key : attribute_value

attribute_value -> { attribute_seq }
                   literal
~~~

## Type

All **literals** have one of the following types:

- int64: typedef int64_t     zeinfo_int64_t
- int32: typedef int32_t     zeinfo_int32_t
- bool: typedef bool        zeinfo_bool_t
- str: typedef std::string zeinfo_str_t

# Container
| Attribute | Type | Required/Optional | Description |
| ----- | ----- | ----- | ----- |
| version | str | Required | ZE Info version number. See Version above. |
| kernels | KernelsTy | Required | vector |
| functions | FunctionsTy | Optional | vector |
| global_host_access_table | HostAccessesTy | Optional | vector |
<!--- Container --->

# Kernel Attributes
| Attribute | Type | Required/Optional | Description |
| ----- | ----- | ------ | ----- |
| name | str | Required | |
| execution_env | ExecutionEnv | Required | |
| payload_arguments | PayloadArgumentsTy | Optional | vector |
| per_thread_payload_arguments | PerThreadPayloadArgumentsTy | Optional | vector |
| binding_table_indices | BindingTableIndicesTy | Optional | vector |
| per_thread_memory_buffers | PerThreadMemoryBuffersTy | Optional | vector |
| experimental_properties | ExperimentalProperties | Optional | A set of experimental attributes. |
| debug_env | DebugEnv | Optional | |
<!--- Kernel Kernels --->

A ze_info section may contain more than one kernel's attributes, each is
represented in a kernel attribute. The name attribute in kernel represent the
kernel's name.

The attributes that are supported in kernel are: **name**,
**Execution Environment**, **Payload Data** and **Memory Buffer**.

# Function Attributes

~~~
functions:
  - name: "string"
    attribute_seq
~~~

| Attribute | Type | Required/Optional | Description |
| ----- | ----- | ------ | ----- |
| name | str | Required | |
| execution_env | ExecutionEnv | Required | |
<!--- Function Functions --->

Function attribute represents a non-kernel function's information. A ze_info
section may contains more than one function's attributes, each is
represented in a function attribute. The name attribute in function represent the
function name.

Function attributes may only present when the function can be externally called.

The attributes that are supported in function are: **name** and **Execution Environment**.

## Execution Environment

~~~
execution_env:
  - attribute_seq
~~~

### Supported attributes in execution environment:
This section defines the execution_env attribute.
If an attribute is **Required**, it must be present in exection_env. If it's **Optional** and it's not present, the **Default** value is used.

| Attribute | Type | Required/Optional | Default | Description |
| ------ | ------ |  ------ | ------ | ------ |
| barrier_count | int32 | Optional | 0 | Number of barriers |
| disable_mid_thread_preemption | bool | Optional | false | |
| grf_count | int32 | Required | | |
| has_4gb_buffers | bool | Optional | false | Compiled for greater than 4GB buffers |
| has_device_enqueue| bool | Optional | false | |
| has_dpas | bool | Optional | false | |
| has_fence_for_image_access | bool | Optional | false | |
| has_global_atomics | bool | Optional | false | |
| has_multi_scratch_spaces | bool | Optional | false | |
| has_no_stateless_write | bool | Optional | false | |
| has_stack_calls | bool | Optional | false | When this value is true, it indicates that program uses stack calls |
| require_disable_eufusion | bool | Optional | false | When this value is true, it indicates that program requires EU fusion disable |
| inline_data_payload_size | int32 | Optional | 0 | Size of inline data in cross-thread-payload in byte. The value is 0 when inline data is disabled (default). |
| offset_to_skip_per_thread_data_load | int32 | Optional | 0 | |
| offset_to_skip_set_ffid_gp | int32 | Optional | 0 | |
| required_sub_group_size | int32 | Optional | 0 | The value is given by users in kernel attributes "intel_reqd_sub_group_size" |
| required_work_group_size | int32x3 | Optional | [0, 0, 0] | The value of this key is a sequence of three int32, for example [256, 2, 1]. The values are given by users in kernel attributes "reqd_work_group_size" |
| simd_size | int32 | Required | | Valid value {1, 8, 16, 32} |
| slm_size | int32 | Optional | 0 | SLM size in bytes |
| subgroup_independent_forward_progress | bool | Optional | false | |
| thread_scheduling_mode | <thread_scheduling_mode> | Optional | | Suggested thread arbitration policy. |
| work_group_walk_order_dimensions | int32x3 | Optional | [0, 1, 2] | The value of this key is a sequence of three int32. Valid values are x: [0, 0, 0] , xy: [0, 1, 0], xyz: [0, 1, 2], yx: [1, 0, 0], zyx: [2, 1, 0] |
<!--- ExecutionEnv -->

### Supported thread scheduling mode:
Supported <thread_scheduling_mode> of execution_env.

| Thread Scheduling Mode | Description |
| ----- | ----- |
| age_based | |
| round_robin | |
| round_robin_stall | |
<!--- <thread_scheduling_mode> ArgThreadSchedulingMode -->

## Payload Arguments
This section defines payload_arguments attribute.
There are two kinds of payload arguments: **Payload Argument** and **Per Thread Payload Arguments**.
The Payload arguments defined here include explicit user arguments of a kernel, such as payload_arguments with arg_type that is arg_byvalue or arg_bypointer,
and implicit arguments inserted by the compiler, such as arguments with local_size arg_type.

### Supported attributes in payload arguments:
If an attribute is **Required**, it must be present in payload arguments. If it's **Optional** and it's not present, it's either not applicable to the specific argument_type, or the **Default** value is used.

| Attribute | Type | Required/Optional | Default | Description |
| ------ | ------ | ------ | ------ | ----- |
| arg_type | <argument_type> | Required | | |
| offset | int32 | Required | | |
| size | int32 | Required | | |
| arg_index | int32 | Optional | -1 | Present when arg_type is "arg_bypointer", "arg_byvalue" or "buffer_offset". For "arg_bypointer" and "arg_byvalue", this is the kernel argument index. For "buffer_offset", this is the index of the associated kernel argument. |
| addrmode | <memory_addressing_mode> | Optional | | Present when arg_type is "arg_bypointer" |
| addrspace | <address_space> | Optional | | Present when arg_type is "arg_bypointer" |
| access_type | <access_type> | Optional | | Present when arg_type is "arg_bypointer" |
| sampler_index | int32 | Optional | -1 | Present when arg_type is "arg_bypointer" and address_space is "sampler" |
| source_offset | int32 | Optional | -1 | Present when arg_type is "arg_byvalue" and the arg is a flattened aggregate element |
| slm_alignment | int32 | Optional | 0 | Present when arg_type is "arg_bypointer", addrmode is "slm" and address_space is "local" |
<!--- PayloadArgument PayloadArguments -->

### Supported argument types:
Supported <argument_type> of payload_arguments or per_thread_payload_arguments.

| Argument Type | Size | Description |
| ----- | ------ | ------ |
| packed_local_ids | int16x3 | Compacted local id x, y, z for simd1 kernel |
| local_id | int16 x N x n | N is the simd_size <br> n is the number of dimensions derived from work_group_walk_order_dimensions <br> Per id have to be GRF aligned |
| local_size | int32x3 | Number of work-items in a group |
| group_count | int32x3 | Number of group |
| work_dimensions | int32 | Work dimensions |
| global_size | int32x3 | OpenCL specific feature. The total number of work-items in each dimension |
| enqueued_local_size | int32x3 | OpenCL specific feature. The size returned by OCL get_enqueued_local_size API  |
| global_id_offset | int32x3 | |
| private_base_stateless | int64 | The base address of private buffer specified at per_thread_memory_buffers |
| buffer_offset | | The extra offset for buffer reference to satisfy the alignment requirement of stateful memory access. |
| printf_buffer | | The address of printf_buffer which holds the printf strings information. |
| implicit_arg_buffer | int64 | The base address of implicit arg buffer |
| arg_byvalue | | Explicit kernel argument |
| arg_bypointer | | Explicit kernel argument |
<!--- <argument_type> ArgType -->

arg_byvalue and arg_bypointer are user arguments that are explicitly passed in from the applications. Other kinds of arguments are implicit arguments that are passed in by runtime.

### Supported memory addressing modes:
Supported <memory_addressing_mode> of payload_arguments.

| Memory Addressing Mode | Description |
| ----- | ----- |
| stateless | |
| stateful | If an argument has stateful memory addressing mode, its binding table index will be specified in a **binding_table_indexes** with the same arg_index |
| bindless | |
| slm | |
<!--- <memory_addressing_mode> ArgAddrMode -->

### Supported address spaces:
Supported <address_space> of payload_arguments.

| Address Space | Description |
| ----- | ----- |
| global | |
| local | |
| constant | |
| image | |
| sampler  | |
<!--- <address_space> ArgAddrSpace -->

### Supported access types:
Supported <access_type> of payload_arguments.

| Access Type | Description |
| ----- | ----- |
| readonly | |
| writeonly | |
| readwrite | |
<!--- <access_type> ArgAccessType -->

## Per Thread Payload Arguments
This section defines per_thread_payload_arguments attribute.
Per Thread Payload Arguments are implicit arguments inserted by the compiler. They are allocated per-thread.

| Attribute | Type | Description |
| ------ | ------ | ------ |
| arg_type | <argument_type> | |
| offset | int32 | |
| size | int32 | |
<!--- PerThreadPayloadArgument PerThreadPayloadArguments -->

## Binding Table Indices
This section defines binding_table_indices attribute.
Binding table index of the corresponding payload_argument.
The payload_argument must have **arg_bypointer** arg_type and **stateful** addrmode

| Attribute | Type | Description |
| ------ | ------ | ------ |
| bti_value | int32 | |
| arg_index | int32 | |
<!--- BindingTableIndex BindingTableIndices -->

## Per Thread Memory Buffer
This section defines the per_thread_memory_buffers attribute,
which indicate the memory buffer required by the Compiler.

| Attribute | Type | Required/Optional | Default | Description |
| ----- | ----- | ----- | ----- | ----- |
| type           | <allocation_type> | Required | | |
| usage          | <memory_usage>    | Required | | |
| size           | int32             | Required | | the buffer size in byte |
| slot           | int32             | Optional | 0     | Present when type is "scratch". Indicate the slot id of this scratch buffer. |
| is_simt_thread | bool              | Optional | false | Present when type is "global". Indicate if the global buffer is allocated per-SIMT-thread. If set to false, the buffer is allocated per-hardware-thread |
<!--- PerThreadMemoryBuffer PerThreadMemoryBuffers -->

### Supported allocation types:
Supported <allocation_type> of Per Thread Memory Buffer.

| Allocation Type | Description |
| ----- | ----- |
| global | Only the memory usage "private_space" can have global type <br> The base address of the global buffer will be passed in by payload_argument with private_base_stateless type
| scratch | Scratch could be bindless surface or stateless <br> The base offset of this scratch will be passed in r0.5. If more than one scratch buffer is requested, the scratch index of each is set by convention |
| slm | |
<!--- <allocation_type> MemBufferType -->

### Supported memory usages:
Supported <memory_usage> of Per Thread Memory Buffer.

| Memory Usage | Description |
| ----- | ----- |
| private_space | Memory space for private variables allocation and arguments passing for stack call |
| spill_fill_space | Memory space for register spill/fill and caller/callee saved for stack call |
| single_space | All compiler required memory space (privates, arguments passing, spill/fill, call/callee saved) are allocated in one single buffer |
<!--- <memory_usage> MemBufferUsage -->

## Experimental Properties
This section defines experimental_properties of a kernel/function.
The experimental attributes are experimental information for early evaluation or experiment.
They must not affect the kernel execution correctness and are subject to be changed or removed.

| Attribute | Type | Required/Optional | Default | Description |
| ------ | ------ | ------ | ------ | ------ |
| has_non_kernel_arg_load   | int32 | Optional | -1 | If this kernel/function contains load that cannot be traced back to kernel arguments. 0 if false, 1 if true, -1 if not applicable.   |
| has_non_kernel_arg_store  | int32 | Optional | -1 | If this kernel/function contains store that cannot be traced back to kernel arguments. 0 if false, 1 if true, -1 if not applicable.  |
| has_non_kernel_arg_atomic | int32 | Optional | -1 | If this kernel/function contains atomic that cannot be traced back to kernel arguments. 0 if false, 1 if true, -1 if not applicable. |
<!--- ExperimentalProperties -->

## Debug Environment
This section defines debug_env attributes to represent all debug related
information. Currently only SIP surface information is provided.

| Attribute | Type | Required/Optional | Default | Description |
| ------ | ------ | ------ | ------ | ------ |
| sip_surface_bti | int32 | Optional | -1 | |
| sip_surface_offset | int32 | Optional | -1 | |
<!--- DebugEnv -->

## Host Access Attributes
This section defines mapping between device and host name of a global variable.

| Attribute | Type | Description |
| ----- | ----- | ----- |
| device_name | str | Mangled global variable name derived from Name parameter LinkageAttributes. |
| host_name | str | User-specified global variable name derived from HostAccesINTEL decoration. Used to access global variable from host API calls. |
<!--- HostAccess HostAccesses --->

## Versioning
Format: \<_Major number_\>.\<_Minor number_\>
- Major number: Increase when non-backward-compatible features are added. For example, rename attributes or remove attributes.
- Minor number: Increase when backward-compatible features are added. For example, add new attributes.

## Change Note
- **Version 1.14**: Add slm_alignment to payload argument.
- **Version 1.13**: Add functions with the name and execution env.
- **Version 1.12**: Add global_host_access_table to container.
- **Version 1.11**: Add require_disable_eufusion attribute.
- **Version 1.10**: Add thread_scheduling_mode to execution_env.
- **Version 1.9**: Add source_offset to payload argument.
- **Version 1.8**: Add inline_data_payload_size to execution_env.
- **Version 1.7**: Add debug_env to kernel.
- **Version 1.6**: Remove actual_kernel_start_offset from execution environment.
- **Version 1.5**: Add payload_argument type work_dimensions.
- **Version 1.4**: Add sampler_index to payload arguments.
- **Version 1.3**: Add printf_buffer to argument_type.
- **Version 1.2**: Add buffer_offset to argument_type.
- **Version 1.1**: Add experimental_properties to kernel.
- **Version 1.0**: Add version number. Add slot to per_thread_memory_buffers. Rename shared_local_memory to slm in memory_addressing_mode.
