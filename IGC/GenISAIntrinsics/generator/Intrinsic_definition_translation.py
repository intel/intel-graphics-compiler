# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

import os
import sys
import re
import json
import importlib.util
import argparse
from Intrinsic_definition_objects import *
from Intrinsic_utils import file_path, Path

def translate_type_definition(type_description):
    if type_description == None:
        return
    if isinstance(type_description, int):
        return TypeDefinition(TypeID.ArgumentReference, index=type_description)
    if isinstance(type_description, list):
        internal_types = []
        for type_str in type_description:
            internal_types.append(translate_type_definition(type_str))
        return TypeDefinition(TypeID.Struct, internal_types=internal_types)
    elif type_description.startswith('any'):
        if type_description == 'anyvector':
            return TypeDefinition(TypeID.Vector, internal_type=TypeDefinition(TypeID.Any))
        elif type_description == 'anyint':
            return TypeDefinition(TypeID.Integer)
        elif type_description == 'anyfloat':
            return TypeDefinition(TypeID.Float)
        elif type_description == 'anyptr':
            return TypeDefinition(TypeID.Pointer, internal_type=TypeDefinition(TypeID.Any))
        else:
            default_type = translate_type_definition(re.search("any(:(.*))?", type_description).group(2))
            return TypeDefinition(TypeID.Any, internal_type=default_type)
    elif type_description == 'void':
        return TypeDefinition(TypeID.Void)
    elif type_description.startswith('bool'):
        type_def = TypeDefinition(TypeID.Integer, bit_width=1)
        match = re.search("bool([0-9]+)", type_description)
        if match is not None:
            type_def = TypeDefinition(TypeID.Vector, num_elements=int(match.group(1)), internal_type=type_def)
        return type_def
    elif type_description.startswith('char'):
        type_def = TypeDefinition(TypeID.Integer, bit_width=8)
        match = re.search("char([0-9]+)", type_description)
        if match is not None:
            type_def = TypeDefinition(TypeID.Vector, num_elements=int(match.group(1)), internal_type=type_def)
        return type_def
    elif type_description.startswith('short'):
        type_def = TypeDefinition(TypeID.Integer, bit_width=16)
        match = re.search("short([0-9]+)", type_description)
        if match is not None:
            type_def = TypeDefinition(TypeID.Vector, num_elements=int(match.group(1)), internal_type=type_def)
        return type_def
    elif type_description.startswith('int'):
        type_def = TypeDefinition(TypeID.Integer, bit_width=32)
        match = re.search("int([0-9]+)", type_description)
        if match is not None:
            type_def = TypeDefinition(TypeID.Vector, num_elements=int(match.group(1)), internal_type=type_def)
        return type_def
    elif type_description.startswith('long'):
        type_def = TypeDefinition(TypeID.Integer, bit_width=64)
        match = re.search("long([0-9]+)", type_description)
        if match is not None:
            type_def = TypeDefinition(TypeID.Vector, num_elements=int(match.group(1)), internal_type=type_def)
        return type_def
    elif type_description.startswith('half'):
        type_def = TypeDefinition(TypeID.Float, bit_width=16)
        match = re.search("half([0-9]+)", type_description)
        if match is not None:
            type_def = TypeDefinition(TypeID.Vector, num_elements=int(match.group(1)), internal_type=type_def)
        return type_def
    elif type_description.startswith('float'):
        type_def = TypeDefinition(TypeID.Float, bit_width=32)
        match = re.search("float([0-9]+)", type_description)
        if match is not None:
            type_def = TypeDefinition(TypeID.Vector, num_elements=int(match.group(1)), internal_type=type_def)
        return type_def
    elif type_description.startswith('double'):
        type_def = TypeDefinition(TypeID.Float, bit_width=32)
        match = re.search("double([0-9]+)", type_description)
        if match is not None:
            type_def = TypeDefinition(TypeID.Vector, num_elements=int(match.group(1)), internal_type=type_def)
        return type_def
    elif type_description.startswith('ptr_private'):
        # E2 <- E == IIT_PTR + AddressSpace == 0 (implicit for IIT_PTR) + 2 == IIT_I8
        return TypeDefinition(TypeID.Pointer, address_space=AddressSpace.Private,
                                       internal_type=TypeDefinition(TypeID.Integer, bit_width=8))
    elif type_description == 'ptr_global':
        # <27>12 (inefficient encoding) <- 27 == IIT_ANYPTR + AddressSpace == 1 (explicit for IIT_ANYPTR) + 2 == IIT_I8
        return TypeDefinition(TypeID.Pointer, address_space=AddressSpace.Global,
                                       internal_type=TypeDefinition(TypeID.Integer, bit_width=8))
    elif type_description == 'ptr_constant':
        # <27>22 (inefficient encoding) <- 27 == IIT_ANYPTR + AddressSpace == 2 (explicit for IIT_ANYPTR) + 2 == IIT_I8
        return TypeDefinition(TypeID.Pointer, address_space=AddressSpace.Constant,
                                       internal_type=TypeDefinition(TypeID.Integer, bit_width=8))
    elif type_description == 'ptr_local':
        # <27>32 (inefficient encoding) <- 27 == IIT_ANYPTR + AddressSpace == 3 (explicit for IIT_ANYPTR) + 2 == IIT_I8
        return TypeDefinition(TypeID.Pointer, address_space=AddressSpace.Local,
                                       internal_type=TypeDefinition(TypeID.Integer, bit_width=8))
    elif type_description == 'ptr_generic':
        # <27>42 (inefficient encoding) <- 27 == IIT_ANYPTR + AddressSpace == 4 (explicit for IIT_ANYPTR) + 2 == IIT_I8
        return TypeDefinition(TypeID.Pointer, address_space=AddressSpace.Generic,
                                       internal_type=TypeDefinition(TypeID.Integer, bit_width=8))
    else:
        return None

def translate_attribute_list(attribute):
    if ',' in attribute:
        attributes = attribute.split(",")
        return set(ID for attribute in attributes for ID in translate_attribute_list(attribute))
    attribute_map = {
        "None": set([ AttributeID.NoUnwind ]),
        "NoMem": set([ AttributeID.NoUnwind, AttributeID.ReadNone ]),
        "ReadMem": set([ AttributeID.NoUnwind, AttributeID.ReadOnly ]),
        "ReadArgMem": set([ AttributeID.NoUnwind, AttributeID.ArgMemOnly, AttributeID.ReadOnly ]),
        "WriteArgMem": set([ AttributeID.NoUnwind, AttributeID.WriteOnly, AttributeID.ArgMemOnly ]),
        "WriteMem": set([ AttributeID.NoUnwind, AttributeID.WriteOnly ]),
        "ReadWriteArgMem": set([ AttributeID.NoUnwind, AttributeID.ArgMemOnly ]),
        "NoReturn": set([ AttributeID.NoUnwind, AttributeID.NoReturn ]),
        "NoDuplicate": set([ AttributeID.NoUnwind, AttributeID.NoDuplicate ]),
        "Convergent": set([ AttributeID.NoUnwind, AttributeID.Convergent ]),
        "InaccessibleMemOnly": set([ AttributeID.NoUnwind, AttributeID.InaccessibleMemOnly ])
    }
    return attribute_map[attribute]

def generate_type_definitions_from_modules(inputs):
    intrinsics = dict()
    for el in inputs:
        spec = importlib.util.spec_from_file_location(Path(el).stem, el)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        intrinsics.update(module.Imported_Intrinsics)

    intrinsic_definitions = []
    for key, value in intrinsics.items():
        name = key
        comment = value[0]
        func_type_def = value[1]
        return_type_str = func_type_def[0]
        if isinstance(return_type_str, list):
            type_strs = [x[0] for x in return_type_str]
            comments = '\n'.join(['Member {}: {}'.format(idx, x[1]) for idx, x in enumerate(return_type_str) if x[1] != ''])
            return_type =  ArgumentTypeDefinition(translate_type_definition(type_strs), comments)
        else:
            return_type =  ArgumentTypeDefinition(translate_type_definition(return_type_str[0]), return_type_str[1])
        argument_type_strs = func_type_def[1]
        argument_types = []
        for type_str in argument_type_strs:
            argument_types.append(ArgumentTypeDefinition(translate_type_definition(type_str[0]), type_str[1]))
        attributes = translate_attribute_list(func_type_def[2])
        intrinsic_definitions.append(IntrinsicDefinition(name, comment, return_type, argument_types, attributes))
    return intrinsic_definitions

if __name__ == '__main__':
    def main(args):
        parser = argparse.ArgumentParser(description='Translate from past IGC format to new IGC format.')
        parser.add_argument("inputs", nargs='+', help="the source path to the file with intrinsic defintions (past IGC format)",
                        type=file_path)
        parser.add_argument("--output", help="the destination path for the file with intrinsic definitions (current IGC format)",
                        type=str)
        parser.add_argument("-v", "--verbose", help="print intrinsic definitions in the current IGC format to the console",
                        action="store_true")

        args = parser.parse_args(args)

        intrinsic_definitions = generate_type_definitions_from_modules(args.inputs)
        serializable_obj = [el.to_dict() for el in intrinsic_definitions]
        ret_json_txt = json.dumps(serializable_obj, indent=2)

        if args.verbose:
            print(ret_json_txt)

        if args.output:
            with open(args.output, 'w') as f:
                f.write(ret_json_txt)

    main(sys.argv)