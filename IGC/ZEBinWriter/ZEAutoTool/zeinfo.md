# ZE Info

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
| Attribute | Type | Description |
| ----- | ----- | ----- |
| kernels | KernelsTy | vector |
<!--- Container --->

# Kernel Attributes
| Attribute | Type | Description |
| ----- | ----- | ------ |
| name | str | |
| execution_env | ExecutionEnv | |
| payload_arguments | PayloadArgumentsTy | vector |
| per_thread_payload_arguments | PerThreadPayloadArgumentsTy | vector |
| binding_table_indices | BindingTableIndicesTy | vector |
| per_thread_memory_buffers | PerThreadMemoryBuffersTy | vector |
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

function attribute represents a non-kernel function's information. A ze_info
section may contains more than one function's attributes, each is
represented in a function attribute. The name attribute in function represent the
kernel's name.

function attributes may only present when the function can be externally or
indirectly called.

The attributes that are supported in function are: **name** and **Memory Buffer**.

## Execution Environment

~~~
execution_env:
  - attribute_seq
~~~

### Supported attributes in execution environment:
If an attribute is **Required**, it must be present in exection_env. If it's **Optional** and it's not present, the **Default** value is used.

| Attribute | Type | Required/Optional | Default | Description |
| ------ | ------ |  ------ | ------ | ------ |
| actual_kernel_start_offset | int32 | Required | | |
| barrier_count | int32 | Optional | 0 | Number of barriers |
| disable_mid_thread_preemption | bool | Optional | false | |
| grf_count | int32 | Required | | |
| has_4gb_buffers | bool | Optional | false | Compiled for greater than 4GB buffers |
| has_device_enqueue| bool | Optional | false | |
| has_fence_for_image_access | bool | Optional | false | |
| has_global_atomics | bool | Optional | false | |
| has_multi_scratch_spaces | bool | Optional | false | |
| has_no_stateless_write | bool | Optional | false | |
| hw_preemption_mode | int32 | Optional | -1 | |
| offset_to_skip_per_thread_data_load | int32 | Optional | 0 | |
| offset_to_skip_set_ffid_gp | int32 | Optional | 0 | |
| required_sub_group_size | int32 | Optional | 0 | The value is given by users in kernel attributes "intel_reqd_sub_group_size" |
| required_work_group_size | int32x3 | Optional | [0, 0, 0] | The value of this key is a sequence of three int32, for example [256, 2, 1]. The values are given by users in kernel attributes "reqd_work_group_size" |
| simd_size | int32 | Required | | Valid value {1, 8, 16, 32} |
| slm_size | int32 | Optional | 0 | SLM size in bytes |
| subgroup_independent_forward_progress | bool | Optional | false | |
| work_group_walk_order_dimensions | int32x3 | Optional | [0, 1, 2] | The value of this key is a sequence of three int32. Valid values are x: [0, 0, 0] , xy: [0, 1, 0], xyz: [0, 1, 2], yx: [1, 0, 0], zyx: [2, 1, 0] |
<!--- ExecutionEnv -->

## Payload Arguments

### Supported attributes in payload arguments:
If an attribute is **Required**, it must be present in payload arguments. If it's **Optional** and it's not present, the **Default** value is used.

| Attribute | Type | Required/Optional | Default | Description |
| ------ | ------ | ------ | ------ |
| arg_type | <argument_type> | Required | | |
| offset | int32 | Required | | |
| size | int32 | Required | | |
| arg_index | int32 | Optional | -1 | Present when arg_type is "arg_bypointer" or "arg_byvalue"|
| addrmode | <memory_addressing_mode> | Optional | | Present when arg_type is "arg_bypointer" |
| addrspace | <address_space> | Optional | | Present when arg_type is "arg_bypointer" |
| access_type | <access_type> | Optional | | Present when arg_type is "arg_bypointer" |
<!--- PayloadArgument PayloadArguments -->

## Per Thread Payload Arguments

| Attribute | Type | Description |
| ------ | ------ | ------ |
| arg_type | <argument_type> | |
| offset | int32 | |
| size | int32 | |
<!--- PerThreadPayloadArgument PerThreadPayloadArguments -->

## Binding Table Indices

| Attribute | Type | Description |
| ------ | ------ | ------ |
| bti_value | int32 | |
| arg_index | int32 | |
<!--- BindingTableIndex BindingTableIndices -->


### Supported argument types:

| Argument Type | Size | Description |
| ----- | ------ | ------ |
| packed_local_ids | int32x3 | Compacted local id x, y, z for simd1 kernel |
| local_id | int16 x N x n | N is the simd_size <br> n is the number of dimensions derived from work_group_walk_order_dimensions <br> Per id have to be GRF aligned |
| local_size | int32x3 | |
| group_size | int32x3 | |
| global_id_offset | int32x3 | |
| private_base_stateless | int64 | |
| arg_byvalue | | |
| arg_bypointer | | |
<!--- <argument_type> ArgType -->

arg_byvalue and arg_bypointer are user arguments that are explcitly passed in from the applications. Other kinds of arguments are implicit arguments that are passed in by runtime.

### Supported memory addressing modes:

| Memory Addressing Mode | Description |
| ----- | ----- |
| stateless | |
| stateful | If an argument has stateful memory addressing mode, its binding table index will be specified in a **binding_table_indexes** with the same arg_index |
| bindless | |
| shared_local_memory | |
<!--- <memory_addressing_mode> ArgAddrMode -->

### Supported address spaces:

| Addresss Space | Description |
| ----- | ----- |
| global | |
| local | |
| constant | |
| image | |
| sampler  | |
<!--- <address_space> ArgAddrSpace -->

### Supported access types:

| Access Type | Description |
| ----- | ----- |
| readonly | |
| writeonly | |
| readwrite | |
<!--- <access_type> ArgAccessType -->

## Per Thread Memory Buffer

|  | Type | Description |
| ----- | ----- | ----- |
| type | <allocation_type> | |
| usage | <memory_usage> | |
| size | int32 | |
<!--- PerThreadMemoryBuffer PerThreadMemoryBuffers -->

Memory buffer required by Compiler generated stacks. Size in bytes.

### Supported allocation types:

| Allocation Type | Description |
| ----- | ----- |
| global | Only the memory usage "private_base" can have global type <br> The base address of the global buffer will be passed in by payload_argument with private_base type
| scratch | Scratch could be bindless surface or stateless <br> The base offset of this scratch will be passed in r0.5. If more than one scratch buffer is requested, the scrach index of each is set by convention |
| slm | |
<!--- <allocation_type> MemBufferType -->

### Supported memory usages:

| Memory Usage | Description |
| ----- | ----- |
| private_space | Memory space for private variables allocation and arguments passing for stack call |
| spill_fill_space | Memory space for register spill/fill and caller/callee saved for stack call |
| single_space | All compiler required memory space (privates, arguments passing, spill/fill, call/callee saved) are allocated in one single buffer |
<!--- <memory_usage> MemBufferUsage -->



