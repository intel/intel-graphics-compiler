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
from Intrinsic_utils import *

def translate_type_definition(type_description):
    if type_description == None:
        return
    if isinstance(type_description, int):
        return TypeDefinition(TypeID.Reference, index=type_description)
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
    if isinstance(attribute, list):
        return set(ID for x in attribute for ID in translate_attribute_list(x))
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
        "InaccessibleMemOnly": set([ AttributeID.NoUnwind, AttributeID.InaccessibleMemOnly ]),
        "WillReturn": set([ AttributeID.WillReturn ]),
        "WriteOnly": set([ AttributeID.WriteOnly ]),
    }
    return attribute_map[attribute]

def topological_sort(input_list : List[TypeDefinition]) -> List[TypeDefinition]:
    provided = set()
    def is_provided(type_def : TypeDefinition):
        if type_def.ID == TypeID.Any:
            return not type_def.default_type or type_def.default_type in provided
        elif type_def.ID == TypeID.Vector:
            return not type_def.element_type or type_def.element_type in provided
        elif type_def.ID == TypeID.Pointer:
            return not type_def.pointed_type or type_def.pointed_type in provided
        elif type_def.ID == TypeID.Struct:
            return len(type_def.member_types) == 0 or all([member_type in provided for member_type in type_def.member_types])
        return True
    copied_input = set(input_list)
    res = []
    while copied_input:
        found_val = None
        for val in input_list:
            if val in copied_input and is_provided(val):
                found_val = val
                break
        else:
            assert(0)
            break
        provided.add(found_val)
        copied_input.remove(found_val)
        res.append(found_val)
    return res

def get_unique_types_list(intrinsic_definitions : List[IntrinsicDefinition]):
    types = {}

    for intrinsic in intrinsic_definitions:
        if intrinsic.return_definition.type_definition in types:
            intrinsic.return_definition.type_definition = types[intrinsic.return_definition.type_definition]
        else:
            types[intrinsic.return_definition.type_definition] = intrinsic.return_definition.type_definition
        for arg in intrinsic.arguments:
            if arg.type_definition in types:
                arg.type_definition = types[arg.type_definition]
            else:
                types[arg.type_definition] = arg.type_definition

    res = list(types.values())

    def process_type(type_def : TypeDefinition):
        if type_def.ID == TypeID.Any:
            if not type_def.default_type:
                return
            if type_def.default_type in types:
                if type_def.default_type in types:
                    type_def.default_type = types[type_def.default_type]
                else:
                    types[type_def.default_type] = type_def.default_type
                    process_type(type_def.default_type)
        elif type_def.ID == TypeID.Vector:
            if not type_def.element_type:
                return
            if type_def.element_type in types:
                if type_def.element_type in types:
                    type_def.element_type = types[type_def.element_type]
                else:
                    types[type_def.element_type] = type_def.element_type
                    process_type(type_def.element_type)
        elif type_def.ID == TypeID.Pointer:
            if not type_def.pointed_type:
                return
            if type_def.pointed_type in types:
                if type_def.pointed_type in types:
                    type_def.pointed_type = types[type_def.pointed_type]
                else:
                    types[type_def.pointed_type] = type_def.pointed_type
                    process_type(type_def.pointed_type)
        elif type_def.ID == TypeID.Struct:
            if len(type_def.member_types) == 0:
                return
            for i in range(len(type_def.member_types)):
                if type_def.member_types[i] in types:
                    if type_def.member_types[i] in types:
                        type_def.member_types[i] = types[type_def.member_types[i]]
                    else:
                        types[type_def.member_types[i]] = type_def.member_types[i]
                        process_type(type_def.member_types[i])

    for type_def in res:
        process_type(type_def)

    res = list(types.values())
    res.sort()
    res = topological_sort(res)

    return res

def generate_intrinsic_definitions_from_modules(*inputs):
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
        return_definition_str = func_type_def[0]
        if isinstance(return_definition_str, list):
            type_strs = [x[0] for x in return_definition_str]
            comments = '\n'.join(['Member {}: {}'.format(idx, x[1]) for idx, x in enumerate(return_definition_str) if x[1] != ''])
            return_definition =  ReturnDefinition(translate_type_definition(type_strs), comments)
        else:
            return_definition =  ReturnDefinition(translate_type_definition(return_definition_str[0]), return_definition_str[1])
        argument_type_strs = func_type_def[1]
        arguments = []
        for index, type_str in enumerate(argument_type_strs):
            arguments.append(ArgumentDefinition("Arg{}".format(index),translate_type_definition(type_str[0]), type_str[1]))
        attributes = translate_attribute_list(func_type_def[2:])
        intrinsic_definitions.append(IntrinsicDefinition(name, comment, return_definition, arguments, attributes))
    return intrinsic_definitions

if __name__ == '__main__':
    def main(args):
        parser = argparse.ArgumentParser(description='Translate from past IGC format to new IGC format.')
        parser.add_argument("inputs", nargs='+', help="the source path to the file with intrinsic defintions (past IGC format)",
                        type=file_path)
        parser.add_argument('--format',
                    default='yaml',
                    choices=['yaml', 'json'],
                    help='the data representation format of the output')
        parser.add_argument("--output", help="the destination path for the file with intrinsic definitions (current IGC format)",
                        type=str)
        parser.add_argument("-v", "--verbose", help="print intrinsic definitions in the current IGC format to the console",
                        action="store_true")
        parser.add_argument("-l", "--license_header", help="attaches a license header to the output file",
                        action="store_true")
        parser.add_argument("-u", "--update", help="consider the current content of the output file",
                        action="store_true")

        args = parser.parse_args(args[1:])

        intrinsic_definitions = generate_intrinsic_definitions_from_modules(*args.inputs)

        if args.update and os.path.isfile(args.output):
            with open(args.output) as f:
                json_ext = '.json'
                file_ext = Path(args.output).suffix
                try:
                    if file_ext == json_ext:
                        intrinsic_definitions.extend(InternalGrammar.from_dict(json.load(f)).intrinsics)

                    else:
                        intrinsic_definitions.extend(yaml.safe_load(f).intrinsics)
                except Exception as err:
                    print("Error on loading data from: {}\n{}".format(args.output, err))
            intrinsic_ids = set()
            unique_intrinsic_definitions = []
            for intrinsic_def in intrinsic_definitions:
                if intrinsic_def.name in intrinsic_ids:
                    print("WARNING: The following intrinsic definition is repeated: {}.".format(intrinsic_def.name))
                    continue
                unique_intrinsic_definitions.append(intrinsic_def)
                intrinsic_ids.add(intrinsic_def.name)
            intrinsic_definitions = unique_intrinsic_definitions

        types = get_unique_types_list(intrinsic_definitions)
        internal_grammar = InternalGrammar(types, intrinsic_definitions)

        if args.format == 'json':
            text = json.dumps(internal_grammar.to_dict(), indent=2)
        else:
            text = yaml.dump(internal_grammar, default_flow_style = False, allow_unicode = True, encoding = None,
                        sort_keys = False, indent=4)

        if args.verbose:
            print(text)

        if args.output:
            if args.license_header:
                template_lookup = TemplateLookup(directories=[r'.'])
                template = Template(filename=r'templates/intrinsic_definition.mako',
                                    lookup=template_lookup)
                output_file_path = args.output
                write_to_file_using_template(output_file_path, template, content=text)
            else:
                with open(args.output, 'w') as f:
                    f.write(text)

    main(sys.argv)
