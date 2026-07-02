# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

# -*- Python -*-

import sys
import lit.Test
import lit.formats

_SUCCINCT = any(
    a == '--succinct' or (a.startswith('-') and not a.startswith('--') and 's' in a)
    for a in sys.argv
)


class VerboseUnsupportedShTest(lit.formats.ShTest):
    """ShTest that emits the unsatisfied REQUIRES/UNSUPPORTED conditions as a
    lit note, making the reason visible alongside the UNSUPPORTED result line.
    Suppressed in succinct mode to match the behaviour of the UNSUPPORTED line
    itself."""

    def execute(self, test, litConfig):
        result = super().execute(test, litConfig)
        if result.code == lit.Test.UNSUPPORTED and result.output and not _SUCCINCT:
            litConfig.note('%s: %s' % (test.getFullName(), result.output))
        return result
