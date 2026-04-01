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

class IntrinsicLookupTableEntry:
    def __init__(self, id : str, lookup_name : str, common_prefix_len : int):
        self.id = id
        self.lookup_name = lookup_name
        self.common_prefix_len = common_prefix_len

class IntrinsicFormatter:
    use_comments = False
    llvm_major_version = 0

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
    def _format_bound(bound):
        """Format a single ValueBound for C++ codegen.
        bound can be:
          - int            -> LiteralBoundHolderT<N>
          - ValueBound     -> dispatches on kind (literal / property_ref)
          - dict with 'index' and 'extraction' -> PropertyRefBoundHolderT<idx, PropertyExtractionKind::...>
        """
        if isinstance(bound, ValueBound):
            if bound.is_ref:
                return "PropertyRefBoundHolderT<{}, PropertyExtractionKind::{}>".format(
                    bound.index, bound.extraction)
            return "LiteralBoundHolderT<{}>".format(bound.value)
        if isinstance(bound, dict):
            return "PropertyRefBoundHolderT<{}, PropertyExtractionKind::{}>".format(
                bound['index'], bound['extraction'])
        return "LiteralBoundHolderT<{}>".format(bound)

    @staticmethod
    def _format_constraint(num_elements):
        """Format a ValueConstraint holder for C++ codegen.
        num_elements can be:
          - int              -> exact literal (0 = any); returns None so caller uses short form
          - list [lo, hi]    -> range
          - dict             -> exact property ref
          - ValueConstraint  -> dispatches on kind
        """
        if isinstance(num_elements, ValueConstraint):
            if num_elements.is_any:
                return None
            if num_elements.is_exact and num_elements.low.is_literal:
                return None  # caller will use the short-form literal path
            if num_elements.low == num_elements.high:
                # Exact match on a property ref
                return "ExactConstraintHolderT<{}>".format(
                    IntrinsicFormatter._format_bound(num_elements.low))
            return "RangeConstraintHolderT<{}, {}>".format(
                IntrinsicFormatter._format_bound(num_elements.low),
                IntrinsicFormatter._format_bound(num_elements.high))
        if isinstance(num_elements, list):
            lo = IntrinsicFormatter._format_bound(num_elements[0])
            hi = IntrinsicFormatter._format_bound(num_elements[1])
            if isinstance(lo, int) and isinstance(hi, int):
                return None
            return "RangeConstraintHolderT<{}, {}>".format(lo, hi)
        if isinstance(num_elements, dict):
            return "ExactConstraintHolderT<{}>".format(
                IntrinsicFormatter._format_bound(num_elements))
        return None

    @staticmethod
    def get_type_definition(type_def):
        output = "Unknown"
        if type_def == None:
            output = "EmptyTypeHolderT"
        elif type_def.ID == TypeID.Integer:
            nb = type_def.bit_width
            constraint = IntrinsicFormatter._format_constraint(nb)
            if constraint is not None:
                output = "IntegerTypeConstrainedHolderT<{}>".format(constraint)
            elif isinstance(nb, ValueConstraint):
                # ValueConstraint that resolved to exact literal or any
                if nb.is_exact:
                    output = "IntegerTypeHolderT<{}>".format(nb.low.value)
                else:
                    output = "IntegerTypeHolderT<0>"
            elif isinstance(nb, list):
                output = "IntegerTypeHolderT<{}, {}>".format(nb[0], nb[1])
            elif isinstance(nb, int) and nb > 0:
                output = "IntegerTypeHolderT<{}>".format(nb)
            else:
                output = "IntegerTypeHolderT<0>"
        elif type_def.ID == TypeID.Float:
            output = "FloatTypeHolderT<{}>".format(type_def.bit_width)
        elif type_def.ID == TypeID.Vector:
            assert(type_def.element_type)
            element_type_name = IntrinsicFormatter.get_type_definition(type_def.element_type)
            ne = type_def.num_elements
            constraint = IntrinsicFormatter._format_constraint(ne)
            if constraint is not None:
                # Complex constraint (range or property ref) - use VectorTypeConstrainedHolderT
                output = "VectorTypeConstrainedHolderT<{}, {}>".format(
                    element_type_name, constraint)
            elif isinstance(ne, ValueConstraint):
                # ValueConstraint that resolved to exact literal or any
                if ne.is_exact:
                    output = "VectorTypeHolderT<{}, {}>".format(element_type_name, ne.low.value)
                else:
                    output = "VectorTypeHolderT<{}>".format(element_type_name)
            elif isinstance(ne, list):
                output = "VectorTypeHolderT<{}, {}, {}>".format(element_type_name, ne[0], ne[1])
            elif isinstance(ne, int) and ne > 0:
                # Exact literal - use short form VectorTypeHolderT<T, N>
                output = "VectorTypeHolderT<{}, {}>".format(element_type_name, ne)
            else:
                # Any (0)
                output = "VectorTypeHolderT<{}>".format(element_type_name)
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
            output = "StructTypeHolderT<{}>".format(
                ", ".join([ "{}".format(IntrinsicFormatter.get_type_definition(member_type)) for member_type in type_def.member_types ]))
        elif type_def.ID == TypeID.Reference:
            extraction = type_def.extraction if hasattr(type_def, 'extraction') else ReferenceExtractionType.NoExtraction
            if extraction != ReferenceExtractionType.NoExtraction:
                output = "ReferenceTypeHolderT<{}, ReferenceExtractionType::{}>".format(type_def.index, extraction)
            else:
                output = "ReferenceTypeHolderT<{}>".format(type_def.index)
        elif type_def.ID == TypeID.Void:
            output = "EmptyTypeHolderT"
        elif type_def.ID == TypeID.Any:
            if type_def.default_type:
                default_type_name = IntrinsicFormatter.get_type_definition(type_def.default_type)
                output = "AnyTypeHolderT<{}>".format(default_type_name)
            else:
                output = "AnyTypeHolderT<>"
        elif type_def.ID == TypeID.TypeList:
            entries = []
            for t in type_def.type_list:
                entries.append(IntrinsicFormatter.get_type_definition(t))
            output = "TypeListTypeHolderT<{}>".format(", ".join(entries))
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

        # list of valid captures is defined in ModRef.h
        if arg.capture != None:
            validCaptureValues = []

            if IntrinsicFormatter.llvm_major_version >= 22:
                validCaptureValues = ["None", "AddressIsNull", "Address", "ReadProvenance", "Provenance", "All"]
            else:
                validCaptureValues = ["None"]

            if arg.capture in validCaptureValues:
                output += ", {}".format(cls.get_capture_entry(arg.capture, is_last=True))
            else:
                raise Exception("[ERROR] Invalid capture value '{}'. Available capture value(s): {}. Below LLVM 22 only 'None' is available.".format(arg.capture, validCaptureValues))

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
    def get_capture_entry(capture_value, is_last):
        output = "IGCLLVM::CaptureComponents::{}".format(capture_value)
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

def parse_llvm_version(llvm_full_version):
    if ("." in llvm_full_version):
        return tuple(map(int, llvm_full_version.split(".")))
    else:
        raise Exception("[ERROR] Invalid LLVM version provided '{}'. Expected something like '16.0.6'.".format(llvm_full_version))

def generate_intrinsic_defintion_files(intrinsic_definitions : List[IntrinsicDefinition], output_directory : str, use_comments, llvm_major_version : int):
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
    write_to_file_using_template(output_file_path, template, intrinsic_definitions=intrinsic_definitions, llvm_major_version = llvm_major_version)

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
        parser.add_argument("--llvm_version", help="the LLVM version to use", type=str, required=True)

        args = parser.parse_args(args[1:])

        print("[INFO] IntrinsicGenerator.py LLVM FULL VERSION: '{}'".format(args.llvm_version))
        LLVM_MAJOR_VERSION = parse_llvm_version(llvm_full_version=args.llvm_version)[0]
        print("[INFO] IntrinsicGenerator.py LLVM MAJOR VERSION: '{}'".format(LLVM_MAJOR_VERSION))

        intrinsic_definitions = []
        for el in args.inputs:
            json_ext = '.json'
            file_ext = Path(el).suffix
            if file_ext == json_ext:
                with open(el) as f:
                    try:
                        intrinsic_definitions.extend(InternalGrammar.from_dict(json.load(f)).intrinsics)
                    except Exception as err:
                        print("Error on loading data from: {}\n{}".format(el, err))
            else:
                with open(el) as f:
                    try:
                        intrinsic_definitions.extend(yaml.safe_load(f).intrinsics)
                    except Exception as err:
                        print("Error on loading data from: {}\n{}".format(el, err))

        if len(intrinsic_definitions) > 0:
            generate_intrinsic_defintion_files(intrinsic_definitions, args.output, args.use_comments, LLVM_MAJOR_VERSION)

    main(sys.argv)