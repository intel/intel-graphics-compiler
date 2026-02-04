/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenPublic.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"

#include <map>
#include <vector>
#include <cstddef>
#include <array>

namespace llvm {
class Argument;
class DataLayout;
class Function;
class MDNode;
class Value;
class StringRef;
} // namespace llvm

namespace IGC {
class ImplicitArg;
/// @brief  KernelArg is used for representing the different OpenCL kernel arguments
///         This class is used for arguments allocation

class KernelArg {
public:
  /// @brief  Type of kernel arguments
  enum class ArgType : int32_t {
    // Argument types that should be allocated
    Begin = 0,
    Default = Begin,
    IMPLICIT_R0,
    R1,

    IMPLICIT_PAYLOAD_HEADER, // known as INPUT_HEADER in USC
    IMPLICIT_GLOBAL_OFFSET,  // previously packed in payload header

    PTR_LOCAL,
    PTR_GLOBAL,
    PTR_CONSTANT,
    PTR_DEVICE_QUEUE,

    CONSTANT_REG,
    RUNTIME_VALUE,

    IMPLICIT_CONSTANT_BASE,
    IMPLICIT_GLOBAL_BASE,
    IMPLICIT_PRIVATE_BASE,

    IMPLICIT_PRINTF_BUFFER,
    IMPLICIT_SYNC_BUFFER,
    IMPLICIT_RT_GLOBAL_BUFFER,

    IMPLICIT_BUFFER_OFFSET,

    IMPLICIT_WORK_DIM,
    IMPLICIT_NUM_GROUPS,
    IMPLICIT_GLOBAL_SIZE,
    IMPLICIT_LOCAL_SIZE,
    IMPLICIT_ENQUEUED_LOCAL_WORK_SIZE,

    IMPLICIT_IMAGE_HEIGHT,
    IMPLICIT_IMAGE_WIDTH,
    IMPLICIT_IMAGE_DEPTH,
    IMPLICIT_IMAGE_NUM_MIP_LEVELS,
    IMPLICIT_IMAGE_CHANNEL_DATA_TYPE,
    IMPLICIT_IMAGE_CHANNEL_ORDER,
    IMPLICIT_IMAGE_SRGB_CHANNEL_ORDER,
    IMPLICIT_IMAGE_ARRAY_SIZE,
    IMPLICIT_IMAGE_NUM_SAMPLES,
    IMPLICIT_SAMPLER_ADDRESS,
    IMPLICIT_SAMPLER_NORMALIZED,
    IMPLICIT_INLINE_SAMPLER,

    // VME
    IMPLICIT_VME_MB_BLOCK_TYPE,
    IMPLICIT_VME_SUBPIXEL_MODE,
    IMPLICIT_VME_SAD_ADJUST_MODE,
    IMPLICIT_VME_SEARCH_PATH_TYPE,

    // Device Enqueue
    IMPLICIT_DEVICE_ENQUEUE_DEFAULT_DEVICE_QUEUE,
    IMPLICIT_DEVICE_ENQUEUE_EVENT_POOL,
    IMPLICIT_DEVICE_ENQUEUE_MAX_WORKGROUP_SIZE,
    IMPLICIT_DEVICE_ENQUEUE_PARENT_EVENT,
    IMPLICIT_DEVICE_ENQUEUE_PREFERED_WORKGROUP_MULTIPLE,
    IMPLICIT_DEVICE_ENQUEUE_DATA_PARAMETER_OBJECT_ID,
    IMPLICIT_DEVICE_ENQUEUE_DISPATCHER_SIMD_SIZE,

    // Generic address space
    IMPLICIT_LOCAL_MEMORY_STATELESS_WINDOW_START_ADDRESS,
    IMPLICIT_LOCAL_MEMORY_STATELESS_WINDOW_SIZE,
    IMPLICIT_PRIVATE_MEMORY_STATELESS_SIZE,

    IMPLICIT_LOCAL_ID_X,
    IMPLICIT_LOCAL_ID_Y,
    IMPLICIT_LOCAL_ID_Z,

    // Buffer bounds checking
    IMPLICIT_BUFFER_SIZE,

    // Raytracing args
    RT_STACK_ID,

    // STAGE_IN_GRID runtime values
    IMPLICIT_STAGE_IN_GRID_ORIGIN,
    IMPLICIT_STAGE_IN_GRID_SIZE,

    IMPLICIT_BINDLESS_OFFSET,

    IMPLICIT_ARG_BUFFER,

    IMPLICIT_ASSERT_BUFFER,
    IMPLICIT_INDIRECT_DATA_POINTER,
    IMPLICIT_SCRATCH_POINTER,
    IMPLICIT_REGION_GROUP_SIZE,
    IMPLICIT_REGION_GROUP_WG_COUNT,
    IMPLICIT_REGION_GROUP_BARRIER_BUFFER,

    // Argument types that shouldn't be allocated
    NOT_TO_ALLOCATE,
    SAMPLER = NOT_TO_ALLOCATE,
    IMAGE_1D,
    IMAGE_1D_BUFFER,
    IMAGE_2D,
    IMAGE_2D_DEPTH,
    IMAGE_2D_MSAA,
    IMAGE_2D_MSAA_DEPTH,
    IMAGE_3D,
    IMAGE_CUBE,
    IMAGE_CUBE_DEPTH,
    IMAGE_1D_ARRAY,
    IMAGE_2D_ARRAY,
    IMAGE_2D_DEPTH_ARRAY,
    IMAGE_2D_MSAA_ARRAY,
    IMAGE_2D_MSAA_DEPTH_ARRAY,
    IMAGE_CUBE_ARRAY,
    IMAGE_CUBE_DEPTH_ARRAY,

    // Address space decoded Args
    BINDLESS_SAMPLER,
    BINDLESS_IMAGE_1D,
    BINDLESS_IMAGE_1D_BUFFER,
    BINDLESS_IMAGE_2D,
    BINDLESS_IMAGE_2D_DEPTH,
    BINDLESS_IMAGE_2D_MSAA,
    BINDLESS_IMAGE_2D_MSAA_DEPTH,
    BINDLESS_IMAGE_3D,
    BINDLESS_IMAGE_CUBE,
    BINDLESS_IMAGE_CUBE_DEPTH,
    BINDLESS_IMAGE_1D_ARRAY,
    BINDLESS_IMAGE_2D_ARRAY,
    BINDLESS_IMAGE_2D_DEPTH_ARRAY,
    BINDLESS_IMAGE_2D_MSAA_ARRAY,
    BINDLESS_IMAGE_2D_MSAA_DEPTH_ARRAY,
    BINDLESS_IMAGE_CUBE_ARRAY,
    BINDLESS_IMAGE_CUBE_DEPTH_ARRAY,

    STRUCT,
    End,
  };

  enum AccessQual { NONE, READ_ONLY, WRITE_ONLY, READ_WRITE };

  KernelArg(ArgType argType, AccessQual accessQual, unsigned int allocateSize, unsigned int elemAllocateSize,
            size_t align, bool isConstantBuf, const llvm::Argument *arg, unsigned int associatedArgNo);

  /// @brief  Constructor.
  ///         Constructs a kernel argument information for explicit arguments
  /// @param  arg         The LLVM explicit kernel argument
  /// @param  DL          The DataLayout, used to determine allocation size
  /// @param  typeStr     The OpenCL type information for the kernel this argument belongs to
  /// @param  qualStr     The OpenCL access qualifier information for the kernel this argument belongs to
  /// @param  location_index The location_index for buffer
  /// @param  location_count The location_count for buffer
  /// @param  needBindlessHandle   The presence of bindless resources in the shader
  /// @param  isEmulationArgument  The information, whether this is an emulation argument (IAB)
  /// @param  isScalarAsPointer The information if scalar argument is used as pointer
  KernelArg(const llvm::Argument *arg, const llvm::DataLayout *DL, const llvm::StringRef typeStr,
            const llvm::StringRef qualstr, int location_index, int location_count, bool needBindlessHandle,
            bool isEmulationArgument, bool isScalarAsPointer);

  /// @brief  Constructor.
  ///         Constructs a kernel argument information for implicit arguments
  /// @param  implicitArg The implicit kernel argument
  /// @param  DL          The DataLayout, used to determine allocation size and alignment
  /// @param  arg         The LLVM kernel argument associated with this implicit argument
  /// @param  imageArgNo  The argument number of the associated image argument
  ///                     This param has meaning only for image dimension implicit arguments
  /// @param  structArgOffset  The argument offset in the associated struct argument
  ///                     This param has meaning only for implicit arguments associated
  ///                     with aggregation explicit argument
  /// @param  isScalarAsPointer The information if pointer is implicit argument associated with
  ///                     aggregation explicit argument provided by value.
  KernelArg(const ImplicitArg &implicitArg, const llvm::DataLayout *DL, const llvm::Argument *arg,
            unsigned int ExplicitArgNo, unsigned int structArgOffset, bool isScalarAsPointer, unsigned int GRFSize);

  /// @brief  Getter functions
  ArgType getArgType() const;
  AccessQual getAccessQual() const;
  unsigned int getNumComponents() const;
  unsigned int getAllocateSize() const;
  unsigned int getSize() const;
  unsigned int getElemAllocateSize() const;
  size_t getAlignment() const;
  bool isConstantBuf() const;
  bool needsAllocation() const;
  const llvm::Argument *getArg() const;
  unsigned int getAssociatedArgNo() const;
  unsigned int getStructArgOffset() const;
  unsigned int getLocationIndex() const;
  unsigned int getLocationCount() const;
  bool typeAlwaysNeedsAllocation() const;
  bool getImgAccessedFloatCoords() const { return m_imageInfo.accessedByFloatCoord; }
  bool getImgAccessedIntCoords() const { return m_imageInfo.accessedByIntCoord; }
  bool isImplicitArg() const { return m_implicitArgument; }
  bool isEmulationArgument() const { return m_isEmulationArgument; }
  bool isScalarAsPointer() const { return m_isScalarAsPointer; }

  /// @brief  Setter functions
  void setImgAccessedFloatCoords(bool val) { m_imageInfo.accessedByFloatCoord = val; }
  void setImgAccessedIntCoords(bool val) { m_imageInfo.accessedByIntCoord = val; }

  /// @brief  Calculates the kernel arg type for the given explicit argument
  /// @param  arg         The explicit kernel argument
  /// @param  typeStr    The OpenCL type information for the kernel this argument belongs to
  /// @return The kernel argument type of the given explicit argument
  static ArgType calcArgType(const llvm::Argument *arg, const llvm::StringRef typeStr);

  struct BufferArgType {
    KernelArg::ArgType type = KernelArg::ArgType::End;
    bool isSampler = false;
  };
  /// @brief  Calculates the kernel arg type for buffer
  /// @param  arg         The explicit kernel argument
  /// @param  typeStr     The OpenCL type information for the kernel this argument belongs to
  /// @return Pair of the kernel argument type of the given explicit argument and whether the type is really SAMPLER
  static BufferArgType getBufferType(const llvm::Argument *arg, const llvm::StringRef typeStr);

  /// @brief  Checks whether the given argument is an image
  /// @param  arg           The kernel argument
  /// @param  typeStr       The OpenCL type information for the kernel this argument belongs to
  /// @param  imageArgType  If this is an image, the argtype of this image
  /// @return true is the given argument is an image, false otherwise
  static bool isImage(const llvm::Argument *arg, const llvm::StringRef typeStr, ArgType &imageArgType);
  static bool isSampler(const llvm::Argument *arg, const llvm::StringRef typeStr);

  /// @brief  Checks whether the given argument is a sampler
  /// @return true is argument type is PTR_LOCAL | PTR_GLOBAL | PTR_CONSTANT | PTR_DEVICE_QUEUE
  bool isArgPtrType();

  /// @brief  Checks whether the given argument is an implicit local id
  /// @return true is argument type is IMPLICIT_LOCAL_ID_X | IMPLICIT_LOCAL_ID_Y | IMPLICIT_LOCAL_ID_Z
  bool isImplicitLocalId();

private:
  /// @brief  Calculates the allocation size needed for the given explicit argument
  /// @param  arg         The kernel argument
  /// @param  DL          The DataLayout, used to determine allocation size
  /// @return The allocation size needed for the given explicit argument
  unsigned int calcAllocateSize(const llvm::Argument *arg, const llvm::DataLayout *DL) const;

  /// @brief  Calculates the alignment needed for the given explicit argument
  /// @param  arg         The kernel argument
  /// @param  DL          The DataLayout, used to determine alignment size
  /// @return The alignment needed for the given explicit argument
  alignment_t calcAlignment(const llvm::Argument *arg, const llvm::DataLayout *DL) const;

  /// @brief  Calculates the allocation size needed for one vector element of the given
  ///         explicit argument. IF the argument is scalar, it will return the allocation
  ///         size of the whole element.
  /// @param  arg         The kernel argument
  /// @param  DL          The DataLayout, used to determine allocation size
  /// @return The allocation size needed for one vector element of the given explicit argument
  unsigned int calcElemAllocateSize(const llvm::Argument *arg, const llvm::DataLayout *DL) const;

  /// @brief  Calculates the kernel arg type for the given implicit argument
  /// @param  arg         The implicit kernel argument
  /// @return The kernel argument type of the given implicit argument
  ArgType calcArgType(const ImplicitArg &arg) const;

  /// @brief  Calculates the access qualifier for the given explicit argument
  /// @param  arg         The explicit kernel argument
  /// @param  qualStr  The OpenCL access qualifier information for the kernel this argument belongs to
  /// @return The kernel argument type of the given explicit argument
  AccessQual calcAccessQual(const llvm::Argument *arg, const llvm::StringRef qualStr) const;

  /// @brief  Calculates the argument number of the argument associated with the given implicit argument.
  ///         For implicit image arguments it will return the arg number of the image associated with the implicit arg.
  ///         For implicit non-image arguments it will return the arg number of the implicit argument itself.
  /// @param  implicitArg The implicit kernel argument
  /// @param  arg         The kernel argument
  /// @param  imageArgNo  If this implicit kernel argument is associated with an image or a sampler,
  ///                     the argument number of that image/sampler.
  /// @return The kernel argument type of the given implicit argument
  unsigned int calcAssociatedArgNo(const ImplicitArg &implicitArg, const llvm::Argument *arg,
                                   unsigned int ExplicitArgNo) const;

  /// @brief  Checks whether the given argument is a sampler
  /// @param  arg         The kernel argument
  /// @param  typeStr    The OpenCL type information for the kernel this argument belongs to
  /// @return true is the given argument is a sampler, false otherwise
  static bool isBindlessSampler(const llvm::Argument *arg, const llvm::StringRef typeStr);

private:
  /// @brief  Is this an explicit or implicit argument
  bool m_implicitArgument;
  /// @brief  The argument type
  ArgType m_argType;
  /// @brief  The argument access qualifier
  AccessQual m_accessQual;
  /// @brief  The number of bytes needed for allocating the argument
  unsigned int m_allocateSize;
  /// @brief  The number of bytes needed for allocating one vector element of the argument
  unsigned int m_elemAllocateSize;
  /// @brief  The argument's alignment
  ///         Must be declared after m_argType and m_allocateSize!
  ///         (Order of initialization)
  size_t m_align;
  /// @brief  Indicates whether the argument is used in calculating the constant buffer length
  bool m_isConstantBuf;
  /// @brief  The LLVM argument that represents this kernel argument
  const llvm::Argument *m_arg;
  /// @brief  The argument number of the associated argument
  ///         For image dimension/BUFFER_OFFSET arguments this will return the argument number
  ///         of the assocaited image.
  ///         For other arguments this will return the argument number of the LLVM argument
  unsigned int m_associatedArgNo;
  /// @brief  The argument struct offset in the associated struct explicit argument
  ///         For struct byvalue arguments this will return the struct offset
  ///         For other arguments this will return -1
  unsigned int m_structArgOffset;

  /// @brief  The Location Index is the value passed from the frontend for buffers only.
  int m_locationIndex;

  /// @brief  The Location Index is the value passed from the frontend for buffers only.
  int m_locationCount;

  /// @brief Indicates if resource if of needs an allocation
  bool m_needsAllocation;

  /// @brief Indicates if resource is an emulation argument (IAB)
  bool m_isEmulationArgument;

  /// @brief Indicates if scalar argument is used as pointer
  bool m_isScalarAsPointer;

  /// @brief
  struct {
    bool accessedByFloatCoord;
    bool accessedByIntCoord;
  } m_imageInfo;

public:
  /// @brief  If this argument has multiple data fields (aka, structs) then m_next points to
  ///         the subsequent field in the struct.
  std::vector<KernelArg> m_subArguments;
};

/// @brief  KernelArgsOrder class is used to define an order in which CISA variables are mapped to
///         a physical grf "payload" locations

class KernelArgsOrder {
public:
  /// @brief  Predefined input layouts
  enum class InputType : uint32_t {
    CURBE,
    INDIRECT,
    INDEPENDENT,
  };

private:
  /// @brief  Order of a payload arguments in a physical grf locations
  ///         Index is an explicitly int32_t casted KernelArg::ArgType
  ///         Value is a requested position
  std::array<uint32_t, static_cast<int32_t>(KernelArg::ArgType::End)> m_position;

  /// @brief  Verifies that passed array defines order for all Argument Types
  /// @param  order
  /// @param  sent    Sentinel
  bool VerifyOrder(std::array<KernelArg::ArgType, static_cast<int32_t>(KernelArg::ArgType::End)> &order,
                   KernelArg::ArgType sent);

  /// @brief  Suppose that you have a 3 arguments: a, b, c and you want them shuffled to
  ///         b c a
  ///         1 2 0 value
  ///         0 1 2 index
  ///         That way c < a has to return true
  ///
  ///         Procedure would fill in a m_position with a
  ///         2 0 1 value
  ///         0 1 2 index
  ///         That way when you would like to obtain a position of an argument you need to call
  ///         PI(a) = 2;
  /// @param  order
  void TransposeGenerateOrder(std::array<KernelArg::ArgType, static_cast<int32_t>(KernelArg::ArgType::End)> &order);

public:
  /// @brief  Constructor
  ///         Fills in an m_position array
  /// @param  type    One of the predefined grf layouts
  explicit KernelArgsOrder(InputType type);

  /// @brief  Returns true if the first argument is considered to go before the second in the
  //          strict weak ordering it defines, and false otherwise
  /// @param  lhs
  /// @param  rhs
  bool operator()(const KernelArg::ArgType &lhs, const KernelArg::ArgType &rhs) const;
};

/// @brief  KernelArgs represent all the explicit and implicit kernel arguments and used for payload allocation

class KernelArgs {
  // Types
public:
  /// @brief  AllocationArgs maps between each kernel argument type and all the arguments of that type
  typedef std::map<KernelArg::ArgType, std::vector<KernelArg>, KernelArgsOrder> AllocationArgs;

public:
  /// @brief  KernelArgs::const_iterator enables constant iteration over the kernel arguments
  ///         This enables iteration over a container of containers
  class const_iterator {
  public:
    enum IterPos {
      BEGIN,
      END,
    };

    /// Constructor
    /// @param  args       The allocation args structure
    /// @param  pos        ENUM of initial iterator position: BEGIN or END
    const_iterator(AllocationArgs &args, IterPos pos);

    /// @brief  Advances the iterator to the next element
    /// @return The iterator, pointing to the next element
    const_iterator &operator++();

    /// @brief  Returns the element the iterator points to
    /// @return The element the iterator points to
    const KernelArg &operator*();

    /// @brief  Checks whether this iterator and the given iterator are different
    ///         by checking if they point to the same element
    /// @param  iterator    An iterator to compare this iterator
    /// @return true if the iterators pare different, false otherwise
    bool operator!=(const const_iterator &iterator) const;

    /// @brief  Checks whether this iterator and the given iterator are same
    ///         by checking if they point to the same element
    /// @param  iterator    An iterator to compare this iterator
    /// @return true if the iterators are same, false otherwise
    bool operator==(const const_iterator &iterator) const;

  private:
    AllocationArgs::const_iterator m_major;
    AllocationArgs::const_iterator m_majorEnd;
    std::vector<KernelArg>::const_iterator m_minor;
    bool m_empty;
  };

  // Member functions

public:
  /// @brief  Constructor.
  ///         Constructs the function's explicit and implicit kernel arguments information
  /// @param  F           The function for which to construct the kernel arguments
  /// @param  DL          The DataLayout
  /// @param  pMdUtils    The Metadata Utils instance for accessing metadata information
  /// @param  layout      One of the predefined payload layout types
  KernelArgs(const llvm::Function &F, const llvm::DataLayout *DL, IGCMD::MetaDataUtils *pMdUtils,
             ModuleMetaData *moduleMD, unsigned int GRFSize,
             KernelArgsOrder::InputType layout = KernelArgsOrder::InputType::INDEPENDENT);

  /// @brief  Returns a constant iterator to the beginning of the kernel arguments
  /// @return A constant iterator to the beginning of the kernel arguments
  const_iterator begin();

  /// @brief  Returns a constant iterator to the end of the kernel arguments
  /// @return A constant iterator to the end of the kernel arguments
  const_iterator end();

  /// #brief Check if we need to insert dummy per-thread data for OpenCL
  ///
  void checkForZeroPerThreadData();

  /// @brief  Checks if there are any kernel arguments
  /// @return true if there are no arguments, false otherwise
  bool empty();

private:
  /// @brief  Check if the given argument needs to be allocated and add it to the allocation args container.
  /// @param  kernelArg   The kernel argument that might need to be allocated
  void addAllocationArg(KernelArg &kernelArg);

  /// @brief  Returns OpenCL type info metadata for the given kernel
  /// @param  F           The kernel for which to return the type info metadata
  /// @return The type info metadata of the given kernel
  llvm::MDNode *getTypeInfoMD(const llvm::Function &F);

  /// @brief  Returns OpenCL access qualifiers info metadata for the given kernel
  /// @param  F           The kernel for which to return the access info metadata
  /// @return The access qualifiers info metadata of the given kernel
  llvm::MDNode *getAccessInfoMD(const llvm::Function &F);

  /// @brief  Returns the opencl kernel metadata associated with F
  /// @param  F           The kernel for which to return the access info metadata
  /// @param  index       The index of the metadata type we require
  /// @return The metadata node associated with F and index
  llvm::MDNode *getKernelMD(const llvm::Function &F, int index);

  // Members
private:
  /// @brief  Order function which defines a payload layout being used
  KernelArgsOrder m_KernelArgsOrder;
  /// @brief  Contains all the kernel arguments that need to be allocated or annotated, sorted by their type
  AllocationArgs m_args;
};

} // namespace IGC
