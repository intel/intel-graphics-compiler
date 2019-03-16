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

typedef struct _cmc_jit_info {
  size_t binary_size;
  void *binary;
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
