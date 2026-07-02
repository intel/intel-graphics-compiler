# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

# -*- Python -*-

import lit.formats
import lit.util
import os
import sys

from lit.llvm import llvm_config
from lit.llvm.subst import ToolSubst
from lit.llvm.subst import FindTool

if config.igc_lit_common_dir:
    sys.path.append(config.igc_lit_common_dir)
    from igc_lit_helpers import VerboseUnsupportedShTest
else:
    VerboseUnsupportedShTest = lit.formats.ShTest

# Configuration file for the 'lit' test runner.

# name: The name of this test suite.
config.name = 'vc-opt'

# testFormat: The test format to use to interpret tests.
config.test_format = VerboseUnsupportedShTest(not llvm_config.use_lit_shell)

# suffixes: A list of file extensions to treat as test files.
config.suffixes = ['.ll']

# excludes: A list of directories  and files to exclude from the testsuite.
config.excludes = ['CMakeLists.txt']

# test_source_root: The root path where tests are located.
config.test_source_root = os.path.dirname(__file__)

# test_exec_root: The root path where tests should be run.
config.test_exec_root = os.path.join(config.test_run_dir, 'test_output')

shared_library_path_env = 'PATH' if 'system-windows' in config.available_features else 'LD_LIBRARY_PATH'

llvm_config.with_environment(shared_library_path_env, config.cm_opt_lib_dir, append_path=True)

llvm_config.use_default_substitutions()

config.substitutions.append(('%PATH%', config.environment['PATH']))
config.substitutions.append(('%VC_PRINTF_OCL_BIF_TYPED_PTRS%', '{}/VCBiFPrintfOCL64.typed.opt.bc'.format(config.vc_bif_binary_dir)))
config.substitutions.append(('%VC_PRINTF_OCL_BIF_OPAQUE_PTRS%', '{}/VCBiFPrintfOCL64.opaque.opt.bc'.format(config.vc_bif_binary_dir)))
config.substitutions.append(('%VC_SPIRV_BIF_TYPED_PTRS%', '{}/VCSPIRVBuiltins64.typed.opt.bc'.format(config.vc_bif_binary_dir)))
config.substitutions.append(('%VC_SPIRV_BIF_OPAQUE_PTRS%', '{}/VCSPIRVBuiltins64.opaque.opt.bc'.format(config.vc_bif_binary_dir)))

platforms = config.vc_platform_list.split(";")
for platform in platforms:
  bif_file_typed_ptrs = '{}/VCBuiltins64_{}.typed.vccg.bc'.format(config.vc_bif_binary_dir, platform)
  bif_file_opaque_ptrs = '{}/VCBuiltins64_{}.opaque.vccg.bc'.format(config.vc_bif_binary_dir, platform)
  if config.opaque_pointers_enabled:
    bif_file_default = bif_file_opaque_ptrs
  else:
    bif_file_default = bif_file_typed_ptrs
  config.substitutions.append(('%VC_BIF_{}%'.format(platform), bif_file_default))
  config.substitutions.append(('%VC_BIF_{}_TYPED_PTRS%'.format(platform), bif_file_typed_ptrs))
  config.substitutions.append(('%VC_BIF_{}_OPAQUE_PTRS%'.format(platform), bif_file_opaque_ptrs))

tool_dirs = [
  config.cm_opt_bin_dir,
  config.oneapi_readelf_dir,
  config.llvm_tools_dir]

asan_runtime_lib = getattr(config, 'asan_runtime_lib', '') if 'system-windows' not in config.available_features else ''

def _make_tool_subst(name, command, extra_args):
  extra_args = list(extra_args or [])
  if asan_runtime_lib and isinstance(command, FindTool) and command.name in ('opt', 'llc'):
    resolved = lit.util.which(command.name, os.pathsep.join(tool_dirs)) or command.name
    return ToolSubst(name, command='env',
                     extra_args=['LD_PRELOAD={}'.format(asan_runtime_lib), resolved] + extra_args)
  return ToolSubst(name, command=command, extra_args=extra_args)


# Add extra args for opt to remove boilerplate from tests.
vc_extra_args_legacy_pm = ['-load', config.llvm_plugin]
vc_extra_args_new_pm = ['-load-pass-plugin', config.llvm_new_pm_plugin]

extra_args_typed_legacy = vc_extra_args_legacy_pm+[config.opaque_pointers_disable_opt]
extra_args_opaque_legacy = vc_extra_args_legacy_pm+[config.opaque_pointers_enable_opt]
extra_args_default = vc_extra_args_legacy_pm+[config.opaque_pointers_default_arg_opt]
extra_args_typed_new_pm = vc_extra_args_new_pm+[config.opaque_pointers_disable_opt]
extra_args_opaque_new_pm = vc_extra_args_new_pm+[config.opaque_pointers_enable_opt]

llvm_ver = int(config.llvm_version)
_OPT, _NOT, _LLC, _NOP = FindTool('opt'), FindTool('not'), FindTool('llc'), ': ||'


def _enabled_tool(tool, is_enabled):
  return tool if is_enabled else _NOP


# Opaque-pointer BiFs are only built for LLVM >= 15.
# Typed-pointer BiFs are only built for LLVM < 17.
opaque_bifs_available = (llvm_ver >= 15)
typed_bifs_available = (llvm_ver < 17)

command_opt_legacy_typed  = _enabled_tool(_OPT, llvm_ver < 16)
command_not_legacy_typed  = _enabled_tool(_NOT, llvm_ver < 16)
command_opt_legacy_opaque = _enabled_tool(_OPT, llvm_ver < 16 and opaque_bifs_available)
command_not_legacy_opaque = _enabled_tool(_NOT, llvm_ver < 16 and opaque_bifs_available)
command_opt_new_pm_typed  = _enabled_tool(_OPT, llvm_ver == 16)
command_not_new_pm_typed  = _enabled_tool(_NOT, llvm_ver == 16)
command_opt_new_pm_opaque = _enabled_tool(_OPT, llvm_ver >= 16)
command_not_new_pm_opaque = _enabled_tool(_NOT, llvm_ver >= 16)
command_opt_typed         = _enabled_tool(_OPT, llvm_ver < 17)
command_not_typed         = _enabled_tool(_NOT, llvm_ver < 17)
command_opt_opaque        = _enabled_tool(_OPT, opaque_bifs_available)
command_not_opaque        = _enabled_tool(_NOT, opaque_bifs_available)
command_llc_typed         = _enabled_tool(_LLC, llvm_ver < 17)
command_llc_opaque        = _enabled_tool(_LLC, opaque_bifs_available)

command_opt_default = command_opt_new_pm_opaque if llvm_ver >= 16 else command_opt_legacy_typed


# Use one of the %opt version explicitly to override the default setting in the
# course of LITs' migration to opaque pointers.

opt_tool_typed_ptrs = _make_tool_subst('%opt_typed_ptrs', command_opt_typed, extra_args_typed_legacy)
opt_tool_opaque_ptrs = _make_tool_subst('%opt_opaque_ptrs', command_opt_opaque, extra_args_opaque_legacy)

opt_tool_legacy_typed = _make_tool_subst('%opt_legacy_typed', command_opt_legacy_typed, extra_args_typed_legacy)
opt_tool_legacy_opaque = _make_tool_subst('%opt_legacy_opaque', command_opt_legacy_opaque, extra_args_opaque_legacy)

opt_tool_new_pm_typed = _make_tool_subst('%opt_new_pm_typed', command_opt_new_pm_typed, extra_args_typed_new_pm)
opt_tool_new_pm_opaque = _make_tool_subst('%opt_new_pm_opaque', command_opt_new_pm_opaque, extra_args_opaque_new_pm)

not_opt_tool_typed_ptrs    = ToolSubst('%not_opt_typed_ptrs',    command=command_not_typed,         extra_args=['%opt_typed_ptrs'])
not_opt_tool_opaque_ptrs   = ToolSubst('%not_opt_opaque_ptrs',   command=command_not_opaque,        extra_args=['%opt_opaque_ptrs'])
not_opt_tool_legacy_typed  = ToolSubst('%not_opt_legacy_typed',  command=command_not_legacy_typed,  extra_args=['%opt_legacy_typed'])
not_opt_tool_legacy_opaque = ToolSubst('%not_opt_legacy_opaque', command=command_not_legacy_opaque, extra_args=['%opt_legacy_opaque'])
not_opt_tool_new_pm_typed  = ToolSubst('%not_opt_new_pm_typed',  command=command_not_new_pm_typed,  extra_args=['%opt_new_pm_typed'])
not_opt_tool_new_pm_opaque = ToolSubst('%not_opt_new_pm_opaque', command=command_not_new_pm_opaque, extra_args=['%opt_new_pm_opaque'])

llc_tool_typed_ptrs = _make_tool_subst('%llc_typed_ptrs', command_llc_typed, extra_args_typed_legacy)
llc_tool_opaque_ptrs = _make_tool_subst('%llc_opaque_ptrs', command_llc_opaque, extra_args_opaque_legacy)

opt_tool_old_pm = _make_tool_subst('%opt', command_opt_default, extra_args_default)

tools = [ToolSubst('not'),
         not_opt_tool_typed_ptrs,
         not_opt_tool_opaque_ptrs,
         not_opt_tool_legacy_opaque,
         not_opt_tool_legacy_typed,
         not_opt_tool_new_pm_opaque,
         not_opt_tool_new_pm_typed,
         opt_tool_old_pm,
         opt_tool_typed_ptrs,
         opt_tool_opaque_ptrs,
         opt_tool_legacy_opaque,
         opt_tool_legacy_typed,
         opt_tool_new_pm_opaque,
         opt_tool_new_pm_typed,
         llc_tool_typed_ptrs,
         llc_tool_opaque_ptrs,
         _make_tool_subst('llc', FindTool('llc'), vc_extra_args_legacy_pm+[config.opaque_pointers_default_arg_opt]),
         ToolSubst('oneapi-readelf', unresolved='ignore'),
         ToolSubst('llvm-dwarfdump'),
         ToolSubst('%igc-lld', command=FindTool('ld.lld'))]

if llvm_ver >= 17:
  config.substitutions.append(('%use_old_pass_manager%', '-bugpoint-enable-legacy-pm'))
else:
  config.substitutions.append(('%use_old_pass_manager%', '-enable-new-pm=0'))

if llvm_ver < 12:
  config.available_features.add('llvm_11_or_less')
else:
  config.available_features.add('llvm_12_or_greater')

if llvm_ver >= 15:
  config.available_features.add('llvm_15_or_greater')

if llvm_ver >= 16:
  config.available_features.add('llvm_16_or_greater')

if config.oneapi_readelf_dir:
  config.available_features.add('oneapi-readelf')

llvm_config.add_tool_substitutions(tools, tool_dirs)
