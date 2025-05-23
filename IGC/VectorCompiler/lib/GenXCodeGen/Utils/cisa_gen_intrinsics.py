#!/usr/bin/python3

# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2019-2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

"""
Usage: cisa_gen_intrinsics.py <input_file> <output_path>

This script gets intrinsics description from JSON file specified by <input_file> argument
and generates two files GenXIntrinsicInfoTable.inc and GenXIntrinsicsBuildMap.inc into
path specified by <output_path> argument.

JSON file must contain following mandatory fields: INTRINSICS, OPCODE_GEN and ARGUMENTS_GEN.

*** Field INTRINSICS
  Contains description of all intrinsics. Each intrinsic is described in following format:
  intrinsic_name : {
    opc: VISA opcode corresponding to the intrinsic
    gen_opc: optional field, it aims to distinguish generators of complex opcodes which may
             contain sub-opcode field
    OPTIONS: list of intrinsics options. Currently, supported only 'disable' value, which means
             that intrinsic will be skipped at all.
    <ARGUMENT>: see description below
  }

  Each argument is a [key: list] format, where key is a name of Argument, list is a command
  for generator.
  First field of generator command is a generator name, it tells how to generate code for
  fetching an argument value. Each argument generator is described in ARGUMENTS_GEN map.

  For example:
      "Surface": ["GENERAL", "UNSIGNED", 10],
  Here GENERAL is generator name by which will be determined (from "ARGUMENTS_GEN") what code
  to generate for getting argument value.
  Generated code:
      auto Surface = CreateOperand(II::ArgInfo(UNSIGNED | 10));
  or for GenXIntrinsicInfoTable.inc:
      GENERAL | UNSIGNED | 10,

  To add new intrinsic you need to add new description into INTRINSICS map. If it contains
  opcode which is absent in opcode_map you also need to add item for new opcode to OPCODE_GEN.

  For example, lets add new intrinsic with new opcode and one new argument generator(NEW_PREDICATION):
  "INTRINSICS":
    "genx_new": {
      "opc": "ISA_NEW",
      "exec_size": ["EXECSIZE_FROM_ARG", 1],
      "pred": ["NEW_PREDICATION", 1],
      "DataOrder": ["BYTE", 5],
      "Surface": ["GENERAL", "UNSIGNED", 10],
      "DstData": ["RAW", 0],
      "Src1Data": ["NULLRAW"]
    },
  "OPCODE_GEN":
    ISA_NEW: "CISA_CALL(Kernel->AppendNew(exec_size, pred, DataOrder, Src1Data, DstData, Surface));"
  "ARGUMENTS_GEN":
    "NEW_PREDICATION": "CreateNewPredication(II::ArgInfo({args}))",
  Also we need to add new function or lambda with name CreateNewPredication to GenXCisaBuilder.cpp

*** Field ARGUMENTS_GEN
  It is needed only to generate CISA building code (GenXIntrinsicsBuildMap.inc)
  Pattern keys that can be used inside generator:
  args   - string with arguments that are passed to ArgInfo constructor.
  value1 - first value in argument list, needed for LITERAL generator
  dst    - name of a variable to which will be assigned argument value

*** Field OPCODE_GEN
  It is needed only to generate CISA building code (GenXIntrinsicsBuildMap.inc)
  Final part of generated code for a single intrinsic is a call of Finalizer's function that builds
  instruction itself. So, all items of this map is just map from opcode to the build function.
  Opcode may be not real VISA opcode, for example, ISA_VA_SKL_PLUS has different functions to build
  instructions with different signatures, which depends of its sub-opcode. Thus there are maybe
  compound opcodes for such cases.
"""

import sys
import re
import json
from collections import OrderedDict


HEADER = '''/******************************************************************************
 * AUTOGENERATED FILE, DO NOT EDIT!
 * Generated by GenXUtilBuild project
 */
'''

def open_and_delete_comments(dscr_filename):
    with open(dscr_filename, "r") as jsonfile:
        data = jsonfile.readlines()
    jsonwithoutcomments = filter(lambda line: "//" not in line, data)
    stringjson = "".join(jsonwithoutcomments)
    return stringjson;

def generate(dscr_filename, out_path):
    special_keys = ('gen_opc', 'OPTIONS')
    descr = json.loads(open_and_delete_comments(dscr_filename), object_pairs_hook=OrderedDict)
    opcode_gen = descr['OPCODE_GEN']
    arguments_gen = descr['ARGUMENTS_GEN']
    intrinsics = descr['INTRINSICS']

    # Convert list to function call string
    # Example: [ Func, arg1, arg2] to Func(arg1, arg2)
    def gen2str(value):
        if isinstance(value, list):
            args = []
            for v in value[1:]:
                args.append(gen2str(v))
            return "{}({})".format(value[0], ', '.join(args))
        return str(value)

    # Recursively search regex in lists
    def gen_search(value, regex):
        if isinstance(value, list):
            for v in value:
                if gen_search(v, regex):
                    return True
            return False
        return bool(re.search(regex, value))

    def isstrinst(opc_gen):
        isalt = True
        if sys.version_info[0] >= 3:
            isalt = isinstance(opc_gen, bytes)
        else:
            isalt = isinstance(opc_gen, unicode)
        return bool(isinstance(opc_gen, str) or isalt)

    def getIntrinsicName(name):
        if name.startswith('genx_'):
            return f'GenXIntrinsic::{name}'
        if name.startswith('vc::InternalIntrinsic'):
            return name
        return f'Intrinsic::{name}'

    def getIntrinsicInfoStr(name, intr):
        intr = list(filter(lambda arg: arg[0] not in special_keys, intr.items()))
        args = []
        for key, value in intr:
            if key in ('opc'):
                args.append('LITERAL | {}'.format("(unsigned int)" + value))
            elif isinstance(value, list):
                should_skip_operand = lambda x: x == 'RAW_OPERANDS' or ('BUILD_ONLY::') in str(x)
                args.append(' | '.join(["(unsigned int)" + str(x) for x in value if not should_skip_operand(x)]))

        return '{{ {}, {{ {} }} }}'.format(getIntrinsicName(name), ', '.join(args))

    should_skip_intrinsic = lambda intr: 'OPTIONS' in intr and 'disable' in intr['OPTIONS']
    intrinsics = list(filter(lambda intr: not should_skip_intrinsic(intr[1]), intrinsics.items()))

    with open(out_path + '/GenXIntrinsicInfoTable.inc', 'w') as file:
        file.write(HEADER)
        file.write(',\n'.join([getIntrinsicInfoStr(*intr) for intr in intrinsics]))

    def analyseForBuildMap(x):
        if isstrinst(x) and 'BUILD_ONLY::' not in str(x):
            return 'II::' + x
        elif 'BUILD_ONLY::' in str(x):
            return str(x).rsplit('BUILD_ONLY::',1)[1]
        else:
            return str(x)

    with open(out_path + '/GenXIntrinsicsBuildMap.inc', 'w') as file:
        file.write(HEADER)
        file.write('switch(IntrinID) {\n\n')

        for name, intr in intrinsics:
            gen_opc = intr.get('gen_opc')
            if not gen_opc:
                gen_opc = intr['opc']

            opc_gen = opcode_gen.get(gen_opc)
            if not opc_gen:
                print(intr)
                raise RuntimeError("Instruction generator not found")
            if isstrinst(opc_gen):
                opc_gen = [opc_gen]
            assert isinstance(opc_gen, list)

            file.write('  case {}: {{\n'.format(getIntrinsicName(name)))

            for key, value in intr.items():
                if key in special_keys:
                    continue

                # no_assign means that there is no variable that need to be assigned
                no_assign = key in ('twoaddr', 'nobarrier')

                # skip items that are not exist in generator string
                if not no_assign and not gen_search(opc_gen, r'\b%s\b'%key):
                    continue

                if key == 'opc':
                    replace = value
                elif isinstance(value, list):
                    replace = arguments_gen.get(value[0])
                    if not replace:
                        print(value)
                        raise RuntimeError('Key not found!')
                    if not replace:
                        continue
                    context = { 'value1': value[1] if len(value) > 1 else None, 'dst': key,
                                'args': '{}'.format(' | ').join(
                                [analyseForBuildMap(x) for x in value if x != 'RAW_OPERANDS']) }
                    if isinstance(replace, list):
                        replace = [x.format(**context) for x in replace]
                    else:
                        replace = replace.format(**context)
                else:
                    replace = value
                assert replace, 'Unknown token'

                if isinstance(replace, list):
                    for replace_item in replace:
                        file.write('    ' + replace_item + ';\n')
                else:
                    assign = '' if no_assign else 'auto ' + key + ' = '
                    file.write('    ' + assign + replace + ';\n')

            for g in opc_gen:
                file.write('    ' + gen2str(g) + ';\n')
            file.write('  } break;\n\n')

        file.write('''  default:
    vc::diagnose(CI->getContext(), "GenXKernelBuilder", "Unsupported intrinsic!", CI);
    break;
}''')

def main():
    if len(sys.argv) > 1 and sys.argv[1] == '--help':
        print(__doc__)
        sys.exit(0)
    assert len(sys.argv) > 2, "Missing arguments! Usage: cisa_gen_intrinsics.py <INPUT_FILE> <OUTPUT_PATH>"
    generate(sys.argv[1], sys.argv[2])

if __name__ == '__main__':
    main()
