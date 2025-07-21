# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

import sys
import json
import argparse
from typing import List
from Intrinsic_definition_objects import *
from Intrinsic_utils import *
from itertools import takewhile

from Intrinsic_definition_translation import generate_intrinsic_definitions_from_modules

class IntrinsicLookupTableEntry:
    def __init__(self, id : str, lookup_name : str, common_prefix_len : int):
        self.id = id
        self.lookup_name = lookup_name
        self.common_prefix_len = common_prefix_len

class IntrinsicFormatter:
    use_comments = False

    @staticmethod
    def get_lookup_table(intrinsic_definitions : List[IntrinsicDefinition]):
        lookup_table = []
        for el in intrinsic_definitions:
            lookup_table.append(IntrinsicLookupTableEntry(el.name, IntrinsicFormatter.get_lookup_name(el.name), 0))
        lookup_table.sort(key=lambda x: x.lookup_name[:-1])
        for i in range(1, len(lookup_table)):
            prev = lookup_table[i - 1].lookup_name
            curr = lookup_table[i].lookup_name
            lookup_table[i].common_prefix_len =  sum(1 for _ in takewhile(lambda x: x[0] == x[1], zip(prev, curr)))
            lookup_table[i - 1].common_prefix_len = max(
                lookup_table[i - 1].common_prefix_len,
                lookup_table[i].common_prefix_len)
        return lookup_table

    def get_intrinsic_lookup_table_entry_initialization_list(lookup_table_entry : IntrinsicLookupTableEntry, is_last):
        lookup_name = lookup_table_entry.lookup_name
        output = '{{ llvm::GenISAIntrinsic::ID::{}, {}, "{}" }}'.format(
            lookup_table_entry.id,
            lookup_table_entry.common_prefix_len,
            lookup_table_entry.lookup_name)
        if not is_last:
            output = "{},".format(output)
        return output

    @staticmethod
    def get_argument_name(argument, index):
        output = "Arg{}".format(index)
        if hasattr(argument, 'name'):
            output = argument.name
        if (index == 0):
            output = "{} = 0".format(output)
        output = "{},".format(output)
        return output

    @staticmethod
    def get_intrinsic_suffix(name):
        name = name[len("GenISA_"):]
        output = name.replace("_", ".")
        return output

    @staticmethod
    def format_name(name):
        output = '"{}"'.format(name)
        return output

    @staticmethod
    def get_lookup_name(name):
        name = name[len("GenISA_"):] + '@'
        return name

    @staticmethod
    def get_type_definition(type_def):
        output = "Unknown"
        if type_def == None:
            output = "EmptyTypeHolderT"
        elif type_def.ID == TypeID.Integer:
            output = "IntegerTypeHolderT<{}>".format(type_def.bit_width)
        elif type_def.ID == TypeID.Float:
            output = "FloatTypeHolderT<{}>".format(type_def.bit_width)
        elif type_def.ID == TypeID.Vector:
            assert(type_def.element_type)
            element_type_name = IntrinsicFormatter.get_type_definition(type_def.element_type)
            if type_def.num_elements > 0:
                output = "VectorTypeHolderT<{}, {}>".format(
                    element_type_name,
                    type_def.num_elements)
            else:
                output = "VectorTypeHolderT<{}>".format(
                    element_type_name)
        elif type_def.ID == TypeID.Pointer:
            assert(type_def.pointed_type)
            assert(type_def.address_space)
            pointed_type_name = IntrinsicFormatter.get_type_definition(type_def.pointed_type)
            address_space = int(type_def.address_space)
            if address_space >= 0:
                output = "PointerTypeHolderT<{}, {}>".format(
                    pointed_type_name,
                    address_space)
            else:
                output = "PointerTypeHolderT<{}>".format(
                    pointed_type_name)
        elif type_def.ID == TypeID.Struct:
            output = "StructTypeHolderT<MemberTypeListHolderT<{}>>".format(
                ", ".join([ "{}".format(IntrinsicFormatter.get_type_definition(member_type)) for member_type in type_def.member_types ]))
        elif type_def.ID == TypeID.Reference:
            output = "ReferenceTypeHolderT<{}>".format(type_def.index)
        elif type_def.ID == TypeID.Void:
            output = "EmptyTypeHolderT"
        elif type_def.ID == TypeID.Any:
            if type_def.default_type:
                default_type_name = IntrinsicFormatter.get_type_definition(type_def.default_type)
                output = "AnyTypeHolderT<{}>".format(default_type_name)
            else:
                output = "AnyTypeHolderT<>"
        return output

    @classmethod
    def get_argument_entry(cls, arg, is_last):
        output = cls.get_type_definition(arg.type_definition)
        output = "{}::scType".format(output)
        if hasattr(arg, 'param_attr'):
            # a param attribute is supported only for pointer function arguments to preserve their pointee types
            if arg.type_definition.ID != TypeID.Pointer:
                sys.exit(1)
            output += ", {}".format(cls.get_attribute_entry(arg.param_attr, is_last=True))
        output = "ArgumentDescription({})".format(output)
        if not is_last:
            output = "{},".format(output)
        return output

    @classmethod
    def get_comment(cls ,comment):
        if cls.use_comments:
            output = '"{}"'.format(comment.strip())
            return output
        return '""'

    @staticmethod
    def get_argument_comment(comment, is_last):
        output = IntrinsicFormatter.get_comment(comment.strip())
        if not is_last:
            output = "{},".format(output)
        return output

    @staticmethod
    def get_attribute_entry(attribute, is_last):
        output = 'llvm::Attribute::{}'.format(attribute)
        if not is_last:
            output = "{},".format(output)
        return output

    @staticmethod
    def get_param_attribute_entry(param_attribute, is_last):
        output = 'std::make_pair(llvm::Attribute::{}, {})'.format(param_attribute.name, param_attribute.index)
        if not is_last:
            output = "{},".format(output)
        return output

    @staticmethod
    def get_memory_effects_instance(memory_restriction : MemoryRestriction):
        modref_arg = "IGCLLVM::ModRefInfo::{}".format(memory_restriction.memory_access)
        if not memory_restriction.memory_location:
            return "IGCLLVM::MemoryEffects({})".format(modref_arg)
        loc_arg = "IGCLLVM::ExclusiveIRMemLoc::{}".format(memory_restriction.memory_location)
        return "IGCLLVM::MemoryEffects({}, {})".format(loc_arg, modref_arg)

    @staticmethod
    def get_memory_effects_from_restrictions(memory_restrictions : List[MemoryRestriction]):
        output = __class__.get_memory_effects_instance(memory_restrictions[0])
        # NB: The code below can take effect once IGC is fully switched to LLVM 16,
        #     and IntrinsicDefinition is adjusted to support multiple MemoryRestriction
        #     entries per each unique location
        #
        # for entry in memory_restrictions[1:]:
        #     output += " & {}".format(get_memory_effects_instance(entry))
        return output

    @staticmethod
    def get_prefix():
        prefix = 'llvm.genx.GenISA.'
        return prefix

def generate_intrinsic_defintion_files(intrinsic_definitions : List[IntrinsicDefinition], output_directory : str, use_comments):
    intrinsic_ids = set()
    unique_intrinsic_definitions = []
    for intrinsic_def in intrinsic_definitions:
        if intrinsic_def.name in intrinsic_ids:
            print("WARNING: The following intrinsic definition is repeated: {}.".format(intrinsic_def.name))
            continue
        unique_intrinsic_definitions.append(intrinsic_def)
        intrinsic_ids.add(intrinsic_def.name)
    intrinsic_definitions = unique_intrinsic_definitions

    template_lookup = TemplateLookup(directories=[r'.'])
    template = Template(filename=r'templates/GenIntrinsicEnum.h.mako',
                        lookup=template_lookup)
    output_file_path = os.path.join(
        output_directory, from_template_name_to_destination_name(template.filename))
    write_to_file_using_template(output_file_path, template, intrinsic_definitions=intrinsic_definitions)

    template = Template(filename=r'templates/GenIntrinsicDescription.h.mako',
                        lookup=template_lookup)
    output_file_path = os.path.join(
        output_directory, from_template_name_to_destination_name(template.filename))
    write_to_file_using_template(output_file_path, template, intrinsic_definitions=intrinsic_definitions)

    template = Template(filename=r'templates/GenIntrinsicDefinition.h.mako',
                        lookup=template_lookup)
    output_file_path = os.path.join(
        output_directory, from_template_name_to_destination_name(template.filename))
    write_to_file_using_template(output_file_path, template, intrinsic_definitions=intrinsic_definitions)

    template = Template(filename=r'templates/GenIntrinsicDefinition.cpp.mako',
                        lookup=template_lookup)
    output_file_path = os.path.join(
        output_directory, from_template_name_to_destination_name(template.filename))
    write_to_file_using_template(output_file_path, template, intrinsic_definitions=intrinsic_definitions, use_comments=use_comments)

    template = Template(filename=r'templates/GenIntrinsicLookupTable.h.mako',
                        lookup=template_lookup)
    output_file_path = os.path.join(
        output_directory, from_template_name_to_destination_name(template.filename))
    write_to_file_using_template(output_file_path, template, intrinsic_definitions=intrinsic_definitions)

if __name__ == '__main__':
    def main(args):
        parser = argparse.ArgumentParser(description='Generates IGC intrinsic files.')
        parser.add_argument("inputs", nargs='+', help="the source path to the file with intrinsic defintions (current IGC format)",
                        type=file_path)
        parser.add_argument("--output", help="the directory for the files with intrinsic definitions",
                        type=dir_path)
        parser.add_argument("--use_comments", action='store_true')

        args = parser.parse_args(args[1:])
        intrinsic_definitions = []
        for el in args.inputs:
            json_ext = '.json'
            py_ext = '.py'
            file_ext = Path(el).suffix
            if file_ext == json_ext:
                with open(el) as f:
                    try:
                        intrinsic_definitions.extend(InternalGrammar.from_dict(json.load(f)).intrinsics)
                    except Exception as err:
                        print("Error on loading data from: {}\n{}".format(el, err))
            elif file_ext == py_ext:
                intrinsic_definitions.extend(generate_intrinsic_definitions_from_modules(el))
            else:
                with open(el) as f:
                    try:
                        intrinsic_definitions.extend(yaml.safe_load(f).intrinsics)
                    except Exception as err:
                        print("Error on loading data from: {}\n{}".format(el, err))

        if len(intrinsic_definitions) > 0:
            generate_intrinsic_defintion_files(intrinsic_definitions, args.output, args.use_comments)

    main(sys.argv)