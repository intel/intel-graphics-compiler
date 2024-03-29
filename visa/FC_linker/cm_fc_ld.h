/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/**
 * Runtime interface of CM Fast Compposite Linker.
 */

#pragma once

#ifndef _CM_FC_LD_H_
#define _CM_FC_LD_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup CM Fast Composite Runtime Linking/Combining interfaces
 *
 * @{
 */

/**
 * @brief Return value of CM fast composite linking interface.
 */
enum cm_fc_error {
  CM_FC_OK = 0,       /**< The operation succeeds. */
  CM_FC_FAILURE = -1, /**< The operation fails. */
  CM_FC_NOBUFS = -2   /**< The operation fails due to insufficient
                           buffer space. */
};

/**
 * @brief Link type of a given fast composite kernel.
 */
enum cm_fc_link_type {
  CM_FC_LINK_TYPE_NONE = 0,   /**< The kernel doesn't call anything. */
  CM_FC_LINK_TYPE_CALLER = 1, /**< The kernel calles a caller kernel. */
  CM_FC_LINK_TYPE_CALLEE = 2  /**< The kernel is called, i.e. a callable
                                   kernel. */
};

/**
 * @brief Return the callee name from the specified patch info.
 *
 * @param buf       The buffer containing CM patch info.
 * @param size      The size of @p buf.
 * @param out_name  The buffer to be filled with caller's name string.
 * @param out_size  Specifies the size in bytes of @p out_name. If CM_FC_OK or
 *                  CM_FC_INVALID_VALUE is returned, it's also set to the real
 *                  size size in bytes used or required for @p out_name.
 */
int cm_fc_get_callee_info(const char *buf, size_t size, void *C,
                          cm_fc_link_type *out_link_type);

/**
 * @brief The structure holding patch info and binary of a kernel to be linked.
 */
typedef struct {
  const char *patch_buf;  /**< The buffer containing patch info. */
  size_t patch_size;      /**< The size of that patch info. */
  const char *binary_buf; /**< The buffer contain binary. */
  size_t binary_size;     /**< The sizeo of that binary. */
} cm_fc_kernel_t;

/**
 * @brief Combine the given kernels.
 *
 * @param num_kernels   The number of kernels to be combined/linked.
 * @param kernels       The array of kernels to be combined/linked.
 * @param out_buf       The buffer to be filled with the combined binary.
 * @param out_size      Specifies the size in bytes of @p out_buf. If CM_FC_OK
 *                      or CM_FC_INVALID_VALUE is returned, it's also set to
 *                      the real size in bytes used or required for @p out_buf.
 * @param options       The additional linking options which is organized as
 *                      space-separated string terminated by null character.
 */
int cm_fc_combine_kernels(size_t num_kernels, cm_fc_kernel_t *kernels,
                          char *out_buf, size_t *out_size, const char *options);

#ifdef __cplusplus
}
#endif

#endif /* _CM_FC_LD_H_ */
