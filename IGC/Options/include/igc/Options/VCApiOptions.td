/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file defines API options accepted by vector compiler.

// VC API options {{
let Flags = [VCApiOption] in {

// Main dispatch option.
def vc_codegen : PlainFlag<"vc-codegen">,
  HelpText<"Enable vector codegenerator">;

def : ZeFlag<"kernel-debug-enable">,
  Alias<emit_debug>, HelpText<"Alias for -g">;

def vc_optimize : PlainSeparate<"optimize">,
  HelpText<"Set optimization level to either 'none' or 'full'">,
  MetaVarName<"<level>">;
def : PlainJoined<"optimize=">, Alias<vc_optimize>,
  HelpText<"Alias for -optimize">;

def : PlainFlag<"no-optimize">, Alias<vc_optimize>,
  AliasArgs<["none"]>, HelpText<"Alias for -optimize=none">;

// This option switches off vector splitting in parts to better fit
// into GRF. It may have both positive and negative consequences for
// performance so shall be used with extreme caution.
def no_vector_decomposition : ZeFlag<"no-vector-decomposition">,
  HelpText<"Disable vector decomposition pass">;
// Compatibility alias.
def : PlainFlag<"no_vector_decomposition">, Alias<no_vector_decomposition>,
  HelpText<"Alias for -ze-no-vector-decomposition"> {
  let Flags = ApiHelper.VCBackwardCompatFlags;
}
def : PlainFlag<"no-vector-decomposition">, Alias<no_vector_decomposition>,
  HelpText<"Alias for -ze-no-vector-decomposition">;

def vc_fno_jump_tables : PlainFlag<"fno-jump-tables">,
  HelpText<"Disable the use of jump tables for lowering switches">;

def vc_ftranslate_legacy_memory_intrinsics :
  PlainFlag<"ftranslate-legacy-memory-intrinsics">,
  HelpText<"Legalize old memory intrinsics">;

def vc_stateless_private_size : PlainJoined<"stateless-stack-mem-size=">,
  HelpText<"Stateless memory amount per thread">;

def Xfinalizer : PlainSeparate<"Xfinalizer">,
  HelpText<"Pass <arg> to the visa finalizer">, MetaVarName<"<arg>">;
}
// }} VC API options

// Igcmc compatibility {{
let Flags = [IgcmcApiOption] in {

def igcmc : PlainFlag<"cmc">,
  HelpText<"Enable igcmc compatible mode; incompatible with -vc-codegen">;

def igcmc_visaopts : PlainJoined<"visaopts=">,
  HelpText<"Options for finalizer in form \"opt1 opt2 opt3...\"">;

def igcmc_stack_size : PlainJoined<"stack-mem-size=">,
  Alias<vc_stateless_private_size>,
  HelpText<"Control stack memory size (in bytes)">;

}
// }} Igcmc compatibility
