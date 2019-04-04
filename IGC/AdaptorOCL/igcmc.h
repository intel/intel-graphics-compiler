#ifndef _IGCMC_H_
#define _IGCMC_H_

#include <stdint.h>

#ifndef DLL_EXPORT
  #ifdef _WIN32
    #define __EXPORT__ __declspec(dllexport)
  #else
    #define __EXPORT__ __attribute__((visibility("default")))
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _cmc_error_t {
  CMC_SUCCESS                  = 0,
  CMC_ERROR                    = 1,
  CMC_ERROR_READING_SPIRV      = 2,
  CMC_ERROR_BROKEN_INPUT_IR    = 3,
  CMC_ERROR_IN_LOADING_TARGET  = 4,
  CMC_ERROR_IN_COMPILING_IR    = 5
} cmc_error_t;

typedef struct _cmc_kernel_info {
  /// The kernel name.
  const char *name;

  /// The number of kernel arguments with descriptors.
  unsigned num_arg_desc;

  /// The kernel argument descriptors.
  const char **arg_desc;

} cmc_kernel_info;

typedef struct _cmc_jit_info {
  /// The vISA binary size in bytes.
  size_t binary_size;

  /// The vISA binary data.
  void *binary;

  /// The vISA major version.
  unsigned visa_major_version;

  /// The vISA minor version.
  unsigned visa_minor_version;

  /// The number of kernels in this binary.
  unsigned num_kernels;

  /// The kernel infomation for each kernel.
  cmc_kernel_info *kernel_info;

  /// The context for this compilation. This opaque data holds all memory
  /// allocations that will be freed in the end.
  void *context;

} cmc_jit_info;

__EXPORT__ cmc_error_t cmc_load_and_compile(const char *input,
                                            size_t input_size,
                                            const char *const options,
                                            cmc_jit_info **output);

__EXPORT__ const char *cmc_get_error_string(cmc_error_t err);

__EXPORT__ cmc_error_t cmc_free_jit_info(cmc_jit_info *output);

#ifdef __cplusplus
}
#endif

#endif // _IGCMC_H_
