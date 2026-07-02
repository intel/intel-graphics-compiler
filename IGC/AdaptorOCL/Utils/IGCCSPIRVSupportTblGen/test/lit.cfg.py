# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

# -*- Python -*-

import lit.formats
import os
import sys

from lit.llvm import llvm_config
from lit.llvm.subst import ToolSubst

if config.igc_lit_common_dir:
    sys.path.append(config.igc_lit_common_dir)
    from igc_lit_helpers import VerboseUnsupportedShTest
else:
    VerboseUnsupportedShTest = lit.formats.ShTest

config.name = 'spirv-extension-support-tblgen'

config.test_format = VerboseUnsupportedShTest(not llvm_config.use_lit_shell)

config.suffixes = ['.td']

config.excludes = ['CMakeLists.txt']

config.test_source_root = os.path.dirname(__file__)

config.test_exec_root = os.path.join(config.test_run_dir, 'test_output')

llvm_config.use_default_substitutions()

tool_dirs = [
  config.spirv_tblgen_dir,
  config.llvm_tools_dir]

tools = [ToolSubst('not'),
         ToolSubst('igcc-spirv-support-tblgen')]

llvm_config.add_tool_substitutions(tools, tool_dirs)

# Path used by tests to -I the shared SPIRVExtensions_Common.td.
config.substitutions.append(
    ('%spirv_extensions_inc', config.spirv_extensions_inc))
