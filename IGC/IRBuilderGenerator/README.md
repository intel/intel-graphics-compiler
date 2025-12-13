# IRBuilderGenerator

A code generation tool that automatically generates C++ IR Builder accessor code from annotated C++ functions compiled to LLVM bitcode.

## Overview

IRBuilderGenerator allows you to write high-level C++ code with special annotations and automatically generate low-level LLVM IRBuilder calls. This is useful for:

- Generating complex sequences of IR instructions from readable C++ templates
- Automatically handling address space parameterization
- Maintaining consistency between structure definitions and IR generation code

**How it works:**
1. Write C++ functions with clang annotations (`[[clang::annotate(...)]]`)
2. Compile to LLVM bitcode using clang
3. Run IRBuilderGenerator to produce public/private header files with IRBuilder code
4. Include generated headers in your IRBuilder-based code

## Quick Start

### 1. Create Your Source File

Create a C++ file (e.g., `reflection.cpp`) with annotated functions:

```cpp
#include "BuilderUtils.h"
#include "AutoGenMyStruct.h"

// Hook declarations - these will be replaced with actual implementations
namespace hook {
namespace bi {
  PUREBUILTIN uint32_t getGlobalValue();
  BUILTIN void performSideEffect();
} // namespace bi
namespace fn {
  BUILTIN uint32_t getUserFunction();
} // namespace fn
} // namespace hook

// Generate a type builder function
TYPEOF MyStruct _gettype_MyStruct() {
  return {};
}

// Generate IRBuilder instruction sequence
CREATE_PRIVATE auto _accessField(MyStructAS MyStruct *ptr) {
  return ptr->field;
}

// Call hook functions that will be passed as template parameters
CREATE_PRIVATE uint32_t _processData(uint32_t value) {
  uint32_t global = hook::bi::getGlobalValue();
  return hook::fn::getUserFunction(value, global);
}
```

**Key Macros:**
- `TYPEOF` → `[[clang::annotate("type-of")]]` - Generates type builder
- `CREATE_PRIVATE` → `[[clang::annotate("create")]] [[clang::annotate("private")]]` - Generates private IRBuilder function
- `CREATE_PUBLIC` → `[[clang::annotate("create")]] [[clang::annotate("public")]]` - Generates public IRBuilder function
- `ALIGNOF` → `[[clang::annotate("align-of")]]` - Records alignment requirements
- `IMPL` → Implementation function (not code-generated)

### 2. Create Address Space Descriptor (Optional)

Create a YAML file (e.g., `AddressSpaces.yaml`) to define custom address spaces:

```yaml
address_spaces:
  - name: MyDataAS
    description: My custom data address space
    constant: false

  - name: MyConstAS
    description: Read-only constant data
    constant: true
```

This generates address space defines and marks loads from constant spaces as invariant.

### 3. Set Up CMakeLists.txt

```cmake
include(IRBuilderGeneratorCodeGen)

set(MY_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/reflection.cpp)
set(MY_YAML ${CMAKE_CURRENT_SOURCE_DIR}/AddressSpaces.yaml)
set(MY_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})

set(MY_INCLUDE_DIRS
    ${MY_OUTPUT_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    # Add other include directories as needed
)

set(MY_DEPENDENCIES
    ${MY_SOURCE}
    ${MY_YAML}
    # Add header dependencies
)

generate_irbuilder_headers(
    NAME MyProject
    SOURCE_FILE ${MY_SOURCE}
    YAML_PATH ${MY_YAML}
    OUTPUT_DIR ${MY_OUTPUT_DIR}
    SETUP_MODE "OPENSOURCE"
    INCLUDE_DIRS ${MY_INCLUDE_DIRS}
    DEPENDS ${MY_DEPENDENCIES}
)

add_library(MyProjectIRBG STATIC
    ${GENERATED_HEADERS}
    ${MY_SOURCE}
)

set_source_files_properties(
    ${GENERATED_HEADERS}
    ${MY_SOURCE}
    PROPERTIES HEADER_FILE_ONLY TRUE
)

target_include_directories(MyProjectIRBG PUBLIC
    ${MY_OUTPUT_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_dependencies(MyProjectIRBG IRBuilderGenerator)
```

### 4. Use Generated Headers

The tool generates three headers in `OUTPUT_DIR`:
- `AutoGenMyProjectPrivate.h` - Private scope functions
- `AutoGenMyProjectPublic.h` - Public scope functions
- `AutoGenAddressSpaces.h` - Address space defines (if YAML provided)

The generated code is designed to be included in a **CRTP (Curiously Recurring Template Pattern) mixin class**. This allows the same generated functionality to be composed into multiple different IRBuilder subclasses.

#### CRTP Mixin Pattern

First, create a CRTP mixin header that includes the generated files:

```cpp
// MyProjectBuilder.h
#pragma once

namespace llvm {

/// CRTP mixin providing MyProject IRBuilder functionality.
///
/// Derived classes must:
/// 1. Inherit from an IRBuilder (e.g., IRBuilder<>)
/// 2. Implement: const MyContext& getCtx() const
template <typename Derived>
class MyProjectBuilder {
protected:
  Derived& derived() { return static_cast<Derived&>(*this); }
  const Derived& derived() const { return static_cast<const Derived&>(*this); }

protected:
#include "AutoGenMyProjectPrivate.h"
public:
#include "AutoGenMyProjectPublic.h"
};

} // namespace llvm
```

Then, use the mixin in your builder class:

```cpp
// MyBuilder.h
#include "MyProjectBuilder.h"

class MyBuilder : public IRBuilder<>,
                  public MyProjectBuilder<MyBuilder> {
  // Allow the CRTP base class to access private members
  friend class MyProjectBuilder<MyBuilder>;

  const MyContext& Ctx;

public:
  // Required by generated code - provides access to context
  const MyContext& getCtx() const { return Ctx; }

  MyBuilder(LLVMContext &C, const MyContext& Ctx)
      : IRBuilder<>(C), Ctx(Ctx) {}

  // Now you can call the generated functions:
  void example(Module &M) {
    Type *myType = _gettype_MyStruct(M);
    Value *result = _accessField(structPtr);
  }
};
```

#### CRTP Requirements

The generated code assumes the following interface on the derived class:

1. **`getCtx()`** - Returns a context object with:
   - `getLLVMContext()` - Returns `LLVMContext*`
   - `getModule()` - Returns `Module*`

2. **IRBuilder methods** - The derived class must inherit from an IRBuilder (e.g., `IRBuilder<>`, `IGCIRBuilder<>`)

3. **Friend declaration** - If generated code calls private methods of your builder, add:
   ```cpp
   friend class MyProjectBuilder<MyBuilder>;
   ```

#### How Generated Code Uses CRTP

The generated code uses `derived()` to access IRBuilder methods:

```cpp
// Generated code example:
auto* _add(Value* arg_a, Value* arg_b) {
  auto* V_0 = derived().CreateAdd(arg_a, arg_b);  // IRBuilder method
  return V_0;
}
```

For static methods like `checkAlign`, the generated code uses `Derived::`:

```cpp
// Generated code for type builders:
IGC_ASSERT_MESSAGE(Derived::checkAlign(M, ...), "type not aligned!");
```

## Annotation Reference

### Function Annotations

#### `[[clang::annotate("type-of")]]`
Generates a function that builds the LLVM type. If the parameter is a pointer, it builds the pointee type.

```cpp
TYPEOF MyStruct _gettype_MyStruct() { return {}; }
// Generates: static Type* _gettype_MyStruct(Module &M)
```

#### `[[clang::annotate("create")]]`
Generates IRBuilder instruction sequences from the function body.

```cpp
CREATE_PRIVATE auto _add(uint32_t a, uint32_t b) {
  return a + b;
}
// Generates: auto* _add(Value* arg_a, Value* arg_b) {
//   auto* V_0 = CreateAdd(arg_a, arg_b);
//   return V_0;
// }
```

#### `[[clang::annotate("align-of")]]`
Records alignment requirements for types. Used to inject padding during type construction.

```cpp
ALIGNOF MyStruct _alignof_MyStruct() { return {}; }
```

#### `[[clang::annotate("private")]]` / `[[clang::annotate("public")]]`
Controls whether functions appear in private or public headers. Use `--scope=private` or `--scope=public` when invoking the tool.

### Hook System

Functions in the `hook` namespace are replaced during code generation:

#### `hook::bi::*` - Built-in Functions
Generates calls to methods on the derived builder class via `derived()`. These methods must be implemented in your builder class.

```cpp
namespace hook {
namespace bi {
  PUREBUILTIN uint32_t getValue();
  BUILTIN void performAction();
}
}

CREATE_PRIVATE void _example() {
  uint32_t x = hook::bi::getValue();
  hook::bi::performAction();
}
// Generates:
// void _example() {
//   auto* V_0 = derived().getValue();      // Called via derived()
//   derived().performAction();              // Called via derived()
// }
```

#### `hook::fn::*` - Function Parameters
Adds templated function parameters that are passed from the caller. These are called directly (not via `derived()`) since they are local function parameters.

```cpp
namespace hook {
namespace fn {
  BUILTIN uint32_t userCallback(uint32_t);
}
}

CREATE_PRIVATE uint32_t _process(uint32_t value) {
  return hook::fn::userCallback(value);
}
// Generates:
// template <typename FnType0>
// auto* _process(Value* arg_value, FnType0 userCallback) {
//   auto* V_0 = userCallback(arg_value);   // Called directly (template param)
//   return V_0;
// }
```

### Supported Instructions

The tool handles most LLVM instructions including:
- Arithmetic: `add`, `sub`, `mul`, `div`, etc.
- Memory: `load`, `store`, `getelementptr`, `alloca`
- Casts: `bitcast`, `ptrtoint`, `inttoptr`, `zext`, `sext`, etc.
- Comparisons: `icmp`, `fcmp`
- Control flow: `br`, `ret`, `phi`, `select`
- Vectors: `extractelement`, `insertelement`, `shufflevector`
- Intrinsics: `llvm.*` intrinsics (more may be supported as needed)
- Calls: Function calls via hook system

**Unsupported:**
- `addrspacecast` (blocked explicitly)

### TypeHoles

Struct types containing "TypeHole" in their name become template parameters:

```cpp
struct TypeHoleT {};

struct MyContainer {
  TypeHoleT element;
};

TYPEOF MyContainer _gettype_MyContainer() { return {}; }
// Generates:
// static Type* _gettype_MyContainer(Module &M, Type* TypeHoleT)
```

## YAML Address Space Schema

The YAML file defines custom address spaces with sequential numbering starting at 100.

```yaml
address_spaces:
  - name: <ADDRESS_SPACE_NAME>      # C++ macro name (e.g., RTSAS)
    description: <DESCRIPTION>       # Human-readable description
    constant: <true|false>           # If true, loads are marked invariant
```

### Example

```yaml
address_spaces:
  - name: RTSAS
    description: RTStack
    constant: false

  - name: RTGAS
    description: RTGlobals
    constant: true

  - name: RTShadowAS
    description: Shadow Memory
    constant: false
```

This generates `AutoGenDesc.h`:

```cpp
struct AddrspaceInfo {
  unsigned AS;
  bool Constant;
  constexpr AddrspaceInfo(unsigned AS, bool Constant) : AS(AS), Constant(Constant) {}
};

constexpr AddrspaceInfo ReservedAS[] = {
    AddrspaceInfo{ 100, false }, // RTSAS: RTStack
    AddrspaceInfo{ 101, true  }, // RTGAS: RTGlobals
    AddrspaceInfo{ 102, false }, // RTShadowAS: Shadow Memory
};

#if defined(__clang__)
// RTStack
#define RTSAS __attribute__((address_space(ReservedAS[0].AS)))
// RTGlobals
#define RTGAS __attribute__((address_space(ReservedAS[1].AS)))
// Shadow Memory
#define RTShadowAS __attribute__((address_space(ReservedAS[2].AS)))
#else
#define RTSAS
#define RTGAS
#define RTShadowAS
#endif // __clang__
```

Use these in your source file:

```cpp
CREATE_PRIVATE auto _loadGlobal(RTGAS uint64_t *ptr) {
  return *ptr;  // Load will be marked as invariant
}
```

The tool parameterizes pointer address spaces in generated code, allowing runtime address space substitution.

## CMake Function Reference

### `generate_irbuilder_headers()`

Defined in `cmake/IRBuilderGeneratorCodeGen.cmake`.

#### Required Parameters

- **`NAME`** - Base name for generated headers (e.g., "MyProject")
  - Generates: `AutoGen${NAME}Private.h`, `AutoGen${NAME}Public.h`

- **`SOURCE_FILE`** - Path to input C++ file with annotations
  - Example: `${CMAKE_CURRENT_SOURCE_DIR}/reflection.cpp`

- **`OUTPUT_DIR`** - Directory where headers will be generated
  - Example: `${CMAKE_CURRENT_BINARY_DIR}`

#### Optional Parameters

- **`YAML_PATH`** - Path to address space descriptor YAML
  - If provided, generates `AutoGen${YAML_NAME}.h` descriptor header
  - Example: `${CMAKE_CURRENT_SOURCE_DIR}/AddressSpaces.yaml`

- **`SETUP_MODE`** - Build configuration mode
  - Values: `"OPENSOURCE"` (default)
  - Controls preprocessor defines and name mangling

- **`INCLUDE_DIRS`** - List of include directories for clang compilation
  - Example: `${MY_INCLUDES} ${LLVM_INCLUDES}`

- **`DEPENDS`** - List of files that trigger regeneration when modified
  - Should include source file, YAML, and any headers it depends on
  - Example: `${MY_SOURCE} ${MY_YAML} ${MY_HEADERS}`

#### Output Variables

Sets `GENERATED_HEADERS` in parent scope containing paths to:
- `${OUTPUT_DIR}/AutoGen${NAME}Private.h`
- `${OUTPUT_DIR}/AutoGen${NAME}Public.h`
- `${OUTPUT_DIR}/AutoGen${YAML_NAME}.h` (if YAML_PATH provided)

#### Compilation Process

The function performs three steps:

1. **Compile to Bitcode**: Uses clang to compile source to `.bc` file
   - Target: `x86_64-pc-windows`
   - Standard: C++17
   - Optimization: `-O2` (required for proper code generation)
   - Applies `SETUP_MODE` preprocessor defines

2. **Generate Private Header**: Runs `IRBuilderGenerator --scope=private`

3. **Generate Public Header**: Runs `IRBuilderGenerator --scope=public`

4. **Generate Descriptor** (if YAML provided): Runs `IRBuilderGenerator --gen-desc`

## Complete Example

See `AdaptorCommon/RayTracing/RTStackReflectionIRBG/` for a comprehensive production example that demonstrates:

- Complex struct type generation
- Address space parameterization
- Hook system integration with IRBuilder methods
- Template function parameters
- Alignment handling
- CRTP mixin pattern for composable IRBuilder functionality

Key files:
- `reflection.cpp` - Source with annotated functions
- `Desc.yaml` - 5 custom address spaces (RTSAS, RTGAS, RTShadowAS, SWStackAS, SWHotZoneAS)
- `CMakeLists.txt` - Complete build setup
- `RTStackReflectionBuilder.h` - CRTP mixin class that includes generated headers
- `RTBuilder.h` - Consumer that inherits from the CRTP mixin

## Tips

**CRTP Pattern**: The generated code uses `derived()` to call IRBuilder methods, making it compatible with CRTP mixin classes. This allows the same generated code to be reused across different builder classes.

**Name Sanitization**: All generated names are prefixed with `_igc_` and special characters are replaced with underscores to avoid collisions with macros and keywords.

**Multi-Block Functions**: Functions with control flow automatically generate BasicBlock creation and phi node patching code.

**Optimization Required**: The source must be compiled with optimizations (`-O2`) for proper code generation. Debug builds may not work correctly.

**Address Space Substitution**: When using YAML-defined address spaces, the generated code parameterizes pointer address spaces, allowing you to pass different address space values at runtime via template parameters or function arguments.

**Constants**: Constant values (integers, floats, vectors) are regenerated in the IRBuilder code using appropriate `derived().getInt*()`, `llvm::ConstantFP::get()`, etc. calls.

**Namespace Qualification**: Generated code uses fully-qualified `llvm::` prefixes for LLVM types like `llvm::ConstantFP`, `llvm::APFloat`, `llvm::ConstantVector`, `llvm::UndefValue`, and `llvm::Constant` to ensure compatibility regardless of `using namespace` declarations.
