/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cm-cl/atomic.h>
#include <cm-cl/exec.h>

using namespace cm;

int __vc_assert_print(__constant const char *fmt, ...);

extern "C" _Noreturn CM_NODEBUG CM_INLINE void
__devicelib_assert_fail(__generic const char *expr, __generic const char *file,
                        int32_t line, __generic const char *func, uint64_t gid0,
                        uint64_t gid1, uint64_t gid2, uint64_t lid0,
                        uint64_t lid1, uint64_t lid2) {
  __vc_assert_print("Assert called: %s.\n"
                    "File %s, Line %u, Function %s, "
                    "gid(%lu, %lu, %lu), lid(%lu, %lu, %lu).\n",
                    expr, file, line, func, gid0, gid1, gid2, lid0, lid1, lid2);
  atomic::execute<atomic::operation::store, memory_order_release,
                  memory_scope_all_devices>(cm::detail::assert_flags(), 1u);
  cm::detail::__cm_cl_debugtrap();
  cm::detail::__cm_cl_trap();
}
