# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

from typing import List, Set, Optional, Union
from enum import Enum
import yaml

def generate_anchor(self, node):
    if node.tag == TypeDefinition.yaml_tag:
        return node.anchor_name
    else:
        res = super(yaml.Dumper, self).generate_anchor(node)
        return res

yaml.Dumper.generate_anchor = generate_anchor

def ignore_aliases(self, data):
    if isinstance(data, TypeDefinition):
        return False
    else:
        return True

yaml.Dumper.ignore_aliases = ignore_aliases

def custom_represent_data(self, data):
    res = super(yaml.Dumper, self).represent_data(data)
    if isinstance(data, TypeDefinition):
        res.anchor_name = str(data)
    return res

yaml.Dumper.represent_data = custom_represent_data

def increase_indent(self, flow=False, indentless=False):
    """Ensure that lists items are always indented."""
    return super(yaml.Dumper, self).increase_indent(
        flow=flow,
        indentless=False)
yaml.Dumper.increase_indent = increase_indent

class QuotedString(str):  # just subclass the built-in str
    pass

def quoted_scalar(dumper, data):  # a representer to force quotations on scalars
    return dumper.represent_scalar('tag:yaml.org,2002:str', data, style='"')

# add the QuotedString custom type with a forced quotation representer to your dumper
yaml.add_representer(QuotedString, quoted_scalar)

class TypeID(Enum):
    Void = 0
    Integer = 1
    Float = 2
    Vector = 3
    Struct = 4
    Pointer = 5
    Any = 6
    Reference = 7

    def __str__(self):
        return self.name

    @classmethod
    def from_str(cls, value : str):
        for key, val in cls.__members__.items():
            if key == value:
                return val
        else:
            raise ValueError("{value} is not present in {cls.__name__}")

def TypeID_representer(dumper, data):
    return dumper.represent_scalar(u'!TypeID', u'%s' % str(data), style='"')

yaml.add_representer(TypeID, TypeID_representer)

def TypeID_constructor(loader, node):
    value = loader.construct_scalar(node)
    return TypeID.from_str(value)

yaml.SafeLoader.add_constructor(u'!TypeID', TypeID_constructor)

class AddressSpace(Enum):
    Undefined = 0
    Private = 1
    Global = 2
    Constant = 3
    Local = 4
    Generic = 5

    def __int__(self):
        return self.value - 1

    def __str__(self):
        return self.name

    def __repr__(self):
        return '%s("%s")' % (self.__class__.__name__, self)

    @classmethod
    def from_str(cls, value : str):
        for key, val in cls.__members__.items():
            if key == value:
                return val
        else:
            raise ValueError("{value} is not present in {cls.__name__}")

def AddressSpace_representer(dumper, data):
    return dumper.represent_scalar(u'!AddressSpace', u'%s' % str(data), style='"')

yaml.add_representer(AddressSpace, AddressSpace_representer)

def AddressSpace_constructor(loader, node):
    value = loader.construct_scalar(node)
    return AddressSpace.from_str(value)

yaml.SafeLoader.add_constructor(u'!AddressSpace', AddressSpace_constructor)

class AttributeID(Enum):
    NoUnwind = 0
    NoReturn = 1
    NoDuplicate = 2
    Convergent = 3
    WillReturn = 4

    def __str__(self):
        return self.name

    def __repr__(self):
        return '%s("%s")' % (self.__class__.__name__, self)

    @classmethod
    def from_str(cls, value : str):
        for key, val in cls.__members__.items():
            if key == value:
                return val
        else:
            raise ValueError("{value} is not present in {cls.__name__}")

def AttributeID_representer(dumper, data):
    return dumper.represent_scalar(u'!AttributeID', u'%s' % str(data), style='"')

yaml.add_representer(AttributeID, AttributeID_representer)

def AttributeID_constructor(loader, node):
    value = loader.construct_scalar(node)
    return AttributeID.from_str(value)

yaml.SafeLoader.add_constructor(u'!AttributeID', AttributeID_constructor)

class ParamAttributeID(Enum):
    ByRef = 0
    ByVal = 1
    StructRet = 2
    NoCapture = 3

    def __str__(self):
        return self.name

    def __repr__(self):
        return '%s("%s")' % (self.__class__.__name__, self)

    @classmethod
    def from_str(cls, value : str):
        for key, val in cls.__members__.items():
            if key == value:
                return val
        else:
            raise ValueError("{value} is not present in {cls.__name__}")

def ParamAttributeID_representer(dumper, data):
    return dumper.represent_scalar(u'!ParamAttributeID', u'%s' % str(data), style='"')

yaml.add_representer(ParamAttributeID, ParamAttributeID_representer)

def ParamAttributeID_constructor(loader, node):
    value = loader.construct_scalar(node)
    return ParamAttributeID.from_str(value)

yaml.SafeLoader.add_constructor(u'!ParamAttributeID', ParamAttributeID_constructor)

class MemoryLocation(Enum):
    ArgMem = 0
    InaccessibleMem = 1
    # Other = 2
    # TODO: Support InaccessibleOrArgMem in a way compatible with LLVM <16 and 16+

    def __str__(self):
        return self.name

    def __repr__(self):
        return '%s("%s")' % (self.__class__.__name__, self)

    @classmethod
    def from_str(cls, value : str):
        for key, val in cls.__members__.items():
            if key == value:
                return val
        else:
            raise ValueError("{value} is not present in {cls.__name__}")

def MemoryLocation_representer(dumper, data):
    return dumper.represent_scalar(u'!MemoryLocation', u'%s' % str(data), style='"')

yaml.add_representer(MemoryLocation, MemoryLocation_representer)

def MemoryLocation_constructor(loader, node):
    value = loader.construct_scalar(node)
    return MemoryLocation.from_str(value)

yaml.SafeLoader.add_constructor(u'!MemoryLocation', MemoryLocation_constructor)

# The naming for enum values is aligned with LLVM 16 MemoryEffects/ModRefInfo syntax.
# TODO: Consider adding a static constructor that would translate from "read / write"
# semantics respresented as strings. This could improve readibility for YAML intrinsic
# definitions, as the syntax would map directly onto LLVM 16 IR layout (i.e. onto the
# unified 'memory' attribute syntax).
class MemoryAccessType(Enum):
    NoModRef = 0 # LLVM 16: memory(none)
    Ref = 1      # LLVM 16: memory(read)
    Mod = 2      # LLVM 16: memory(write)
    ModRef = 3   # LLVM 16: memory(readwrite)

    def __str__(self):
        return self.name

    def __repr__(self):
        return '%s("%s")' % (self.__class__.__name__, self)

    @classmethod
    def from_str(cls, value : str):
        for key, val in cls.__members__.items():
            if key == value:
                return val
        else:
            raise ValueError("{value} is not present in {cls.__name__}")

def MemoryAccessType_representer(dumper, data):
    return dumper.represent_scalar(u'!MemoryAccessType', u'%s' % str(data), style='"')

yaml.add_representer(MemoryAccessType, MemoryAccessType_representer)

def MemoryAccessType_constructor(loader, node):
    value = loader.construct_scalar(node)
    return MemoryAccessType.from_str(value)

yaml.SafeLoader.add_constructor(u'!MemoryAccessType', MemoryAccessType_constructor)

class SafeYAMLObject(yaml.YAMLObject):
    yaml_loader = yaml.SafeLoader

class TypeDefinition(SafeYAMLObject):
    yaml_tag = u'TypeDefinition'

    def __init__(self, typeID : TypeID, bit_width : int  = 0, num_elements : int = 0,
                address_space : AddressSpace = AddressSpace.Undefined, internal_type = None,
                index : int = 0, internal_types = None):
        self.ID = typeID
        if self.ID == TypeID.Integer:
            self.bit_width = bit_width
        elif self.ID == TypeID.Float:
            self.bit_width = bit_width
        elif self.ID == TypeID.Any:
            self.default_type = internal_type
        elif self.ID == TypeID.Vector:
            self.num_elements = num_elements
            self.element_type = internal_type
        elif self.ID == TypeID.Pointer:
            self.address_space = address_space
            self.pointed_type = internal_type
        elif self.ID == TypeID.Struct:
            self.member_types = internal_types
        elif self.ID == TypeID.Reference:
            self.index = index

    def __str__(self):
        if self.ID == TypeID.Integer:
            if self.bit_width == 0:
                return "any_int"
            else:
                return "i{}".format(self.bit_width)
        elif self.ID == TypeID.Float:
            if self.bit_width == 0:
                return "any_float"
            else:
                return "f{}".format(self.bit_width)
        elif self.ID == TypeID.Any:
            if self.default_type:
                return "any_{}_".format(self.default_type)
            else:
                return "any"
        elif self.ID == TypeID.Vector:
            if self.element_type and self.num_elements:
                return "v{}_{}_".format(self.num_elements, self.element_type)
            elif self.element_type and self.num_elements == 0:
                return "v_{}_".format(self.element_type)
            elif not self.element_type and self.num_elements:
                return "v{}_any_".format(self.num_elements)
            else:
                return "any_vector"
        elif self.ID == TypeID.Pointer:
            addr_space = int(self.address_space)
            if addr_space >= 0 and self.pointed_type:
                return "p{}_{}_".format(addr_space, self.pointed_type)
            elif self.pointed_type and addr_space < 0:
                return "p_{}_".format(self.pointed_type)
            elif not self.pointed_type and addr_space <= 0:
                return "p{}_any_".format(addr_space)
            else:
                return "any_pointer"
        elif self.ID == TypeID.Struct:
            if len(self.member_types) > 0:
                return "s_{}_".format('-'.join([str(m) for m in self.member_types]))
            else:
                return "any_struct"
        elif self.ID == TypeID.Reference:
            return "ref_{}_".format(self.index)
        return "void"

    def __repr__(self):
        if self.ID == TypeID.Integer:
            return "%s(ID=%r, bit_width=%r)" % (
                self.__class__.__name__, self.ID, self.bit_width)
        elif self.ID == TypeID.Float:
            return "%s(ID=%r, bit_width=%r)" % (
                self.__class__.__name__, self.ID, self.bit_width)
        elif self.ID == TypeID.Any:
            return "%s(ID=%r, default_type=%r)" % (
                self.__class__.__name__, self.ID, self.default_type)
        elif self.ID == TypeID.Vector:
            return "%s(ID=%r, num_elements=%r, element_type=%r)" % (
                self.__class__.__name__, self.ID, self.num_elements, self.element_type)
        elif self.ID == TypeID.Pointer:
            return "%s(ID=%r, address_space=%r, pointed_type=%r)" % (
                self.__class__.__name__, self.ID, self.address_space, self.pointed_type)
        elif self.ID == TypeID.Struct:
            return "%s(ID=%r, member_types=%r)" % (
                self.__class__.__name__, self.ID, self.member_types)
        elif self.ID == TypeID.Reference:
            return "%s(ID=%r, index=%r)" % (
                self.__class__.__name__, self.ID, self.index)
        return "%s(ID=%r)" % (
            self.__class__.__name__, self.ID)

    def __eq__(self, other):
        if isinstance(other, TypeDefinition) and self.ID == other.ID:
            if self.ID == TypeID.Integer:
                return self.bit_width == other.bit_width
            elif self.ID == TypeID.Float:
                return self.bit_width == other.bit_width
            elif self.ID == TypeID.Any:
                return self.default_type == other.default_type
            elif self.ID == TypeID.Vector:
                return self.num_elements == other.num_elements and self.element_type == other.element_type
            elif self.ID == TypeID.Pointer:
                return self.address_space == other.address_space and self.pointed_type == other.pointed_type
            elif self.ID == TypeID.Struct:
                return tuple(self.member_types) ==  tuple(other.member_types)
            elif self.ID == TypeID.Reference:
                return self.index == other.index
            return True
        else:
            return False

    def __lt__(self, other):
        if isinstance(other, TypeDefinition) and self.ID == other.ID:
            if self.ID == TypeID.Integer:
                return self.bit_width < other.bit_width
            elif self.ID == TypeID.Float:
                return self.bit_width < other.bit_width
            elif self.ID == TypeID.Any:
                if self.default_type and other.default_type:
                    return self.default_type < other.default_type
                return not self.default_type
            elif self.ID == TypeID.Vector:
                return self.element_type < other.element_type if self.element_type != other.element_type else self.num_elements < other.num_elements
            elif self.ID == TypeID.Pointer:
                return int(self.address_space) < int(other.address_space) if int(self.address_space) != int(other.address_space) else self.pointed_type < other.pointed_type
            elif self.ID == TypeID.Struct:
                if len(self.member_types) == len(other.member_types):
                    diffrent_types = [(self.member_types[i], other.member_types[i]) for i in range(len(self.member_types))]
                    return diffrent_types[0][0] < diffrent_types[0][1]
                else:
                   return len(self.member_types) < len(other.member_types)
            elif self.ID == TypeID.Reference:
                return self.index < other.index
            return True
        else:
            return self.ID.value < other.ID.value

    def __ne__(self, other):
        return not self.__eq__(other)

    def __hash__(self):
        if self.ID == TypeID.Integer:
            return hash((self.ID, self.bit_width))
        elif self.ID == TypeID.Float:
            return hash((self.ID, self.bit_width))
        elif self.ID == TypeID.Any:
            return hash((self.ID, self.default_type))
        elif self.ID == TypeID.Vector:
            return hash((self.ID, self.num_elements, self.element_type))
        elif self.ID == TypeID.Pointer:
            return hash((self.ID, self.address_space, self.pointed_type))
        elif self.ID == TypeID.Struct:
            return hash((self.ID,  tuple(self.member_types)))
        elif self.ID == TypeID.Reference:
            return hash((self.ID, self.index))
        elif self.ID == TypeID.Void:
            return hash((self.ID))
        else:
            assert(0)
            return 0

    def to_dict(self):
        if self.ID == TypeID.Integer:
            res = {
                "ID":  str(self.ID),
                "bit_width": self.bit_width
            }
        elif self.ID == TypeID.Float:
            res = {
                "ID":  str(self.ID),
                "bit_width": self.bit_width
            }
        elif self.ID == TypeID.Vector:
            res = {
                "ID":  str(self.ID),
                "num_elements": self.num_elements,
                "element_type": self.element_type.to_dict()
            }
        elif self.ID == TypeID.Pointer:
            res = {
                "ID":  str(self.ID),
                "address_space": str(self.address_space),
                "pointed_type": self.pointed_type.to_dict()
            }
        elif self.ID == TypeID.Struct:
            res = {
                "ID":  str(self.ID),
                "member_types": [ el.to_dict()for el in self.member_types ]
            }
        elif self.ID == TypeID.Reference:
            res = {
                "ID":  str(self.ID),
                "index": self.index
            }
        elif self.ID == TypeID.Void:
            res = {
                "ID":  str(self.ID)
            }
        elif self.ID == TypeID.Any:
            res = {
                "ID":  str(self.ID),
                "default_type": self.default_type.to_dict() if self.default_type != None else None
            }
        return res

    @staticmethod
    def from_dict(json_dct : dict):
        ID = TypeID.from_str(json_dct['ID'])
        if ID == TypeID.Integer:
            return TypeDefinition(ID, bit_width=json_dct['bit_width'])
        elif ID == TypeID.Float:
            return TypeDefinition(ID, bit_width=json_dct['bit_width'])
        elif ID == TypeID.Vector:
            internal_type = TypeDefinition.from_dict(json_dct['element_type'])
            return TypeDefinition(ID, num_elements=json_dct['num_elements'], internal_type=internal_type)
        elif ID == TypeID.Pointer:
            internal_type = TypeDefinition.from_dict(json_dct['pointed_type'])
            address_space = AddressSpace.from_str(json_dct['address_space'])
            return TypeDefinition(ID, address_space=address_space, internal_type=internal_type)
        elif ID == TypeID.Struct:
            member_types = [TypeDefinition.from_dict(el) for el in json_dct['member_types']]
            return TypeDefinition(ID, internal_types=member_types)
        elif ID == TypeID.Reference:
            return TypeDefinition(ID, index=json_dct['index'])
        elif ID == TypeID.Any:
            internal_type = TypeDefinition.from_dict(json_dct['default_type']) if json_dct['default_type'] else None
            return TypeDefinition(ID, internal_type=internal_type)

class ArgumentDefinition(SafeYAMLObject):

    yaml_tag = u'ArgumentDefinition'

    @classmethod
    def from_yaml(cls, loader, node):
        arg_dict = loader.construct_mapping(node, deep=True)
        return cls(**arg_dict)

    def __init__(self, name : str, type_definition : TypeDefinition, comment : str, param_attr : ParamAttributeID = None):
        self.name = name
        self.type_definition = type_definition
        self.comment = QuotedString(comment)
        if param_attr:
            self.param_attr = param_attr

    def __repr__(self):
        repr_str = "name=%r" % (self.name)
        repr_str += ", type_definition=%r" % (self.type_definition)
        repr_str += ", comment=%r" % (self.comment)
        if hasattr(self, 'param_attr'):
            repr_str += ", param_attr=%r" % str(self.param_attr)
        return "%s(%r)" % (
            self.__class__.__name__, repr_str)

    def to_dict(self):
        res =  {
            "name": self.name,
            "type_definition": self.type_definition.to_dict(),
            "comment": self.comment
        }
        if hasattr(self, 'param_attr'):
            res["param_attr"] = str(self.param_attr)
        return res

    @staticmethod
    def from_dict(json_dct : dict):
        type_definition = TypeDefinition.from_dict(json_dct['type_definition'])
        return ArgumentDefinition(json_dct['name'], type_definition, json_dct['comment'], json_dct['param_attr'])

class MemoryRestriction(SafeYAMLObject):

    yaml_tag = u'MemoryRestriction'

    @classmethod
    def from_yaml(cls, loader, node):
        arg_dict = loader.construct_mapping(node, deep=True)
        return cls(**arg_dict)

    def __init__(self, memory_location : Optional[MemoryLocation] = None,
                 memory_access : MemoryAccessType = MemoryAccessType['ModRef']):
        self.memory_location = memory_location
        self.memory_access = memory_access

    def __repr__(self):
        memory_loc_str = ("memory_location=%r, " % (self.memory_location)
                              if self.memory_location else ")"
                         )
        memory_acc_str = "memory_access=%r" % (self.memory_access)
        return "%s(%r%r)" % (self.__class__.__name__, memory_loc_str, memory_acc_str)

    def to_dict(self):
        res = {
            "memory_access": memory_access.__str__()
        }
        if memory_location != None:
            res["memory_location"] = memory_location.__str__()
        return res

    @staticmethod
    def from_dict(json_dct : dict):
        loc_entry = json_dct.get("memory_location")
        memory_location = MemoryLocation.from_str(loc_entry) if loc_entry else None
        acc_entry = json_dct.get("memory_access")
        if acc_entry != None:
            return MemoryRestriction(memory_location, MemoryAccessType.from_str(acc_entry))
        return MemoryRestriction(memory_location)

class ReturnDefinition(SafeYAMLObject):

    yaml_tag = u'ReturnDefinition'

    @classmethod
    def from_yaml(cls, loader, node):
        arg_dict = loader.construct_mapping(node, deep=True)
        return cls(**arg_dict)

    def __init__(self, type_definition : TypeDefinition, comment : str):
        self.type_definition = type_definition
        self.comment = QuotedString(comment)

    def __repr__(self):
        return "%s(type_definition=%r, comment=%r)" % (
            self.__class__.__name__, self.type_definition, self.comment)

    def to_dict(self):
        res =  {
            "type_definition": self.type_definition.to_dict(),
            "comment": self.comment
        }
        return res

    @staticmethod
    def from_dict(json_dct : dict):
        type_definition = TypeDefinition.from_dict(json_dct['type_definition'])
        return ReturnDefinition(type_definition, json_dct['comment'])

class IntrinsicDefinition(SafeYAMLObject):

    yaml_tag = u'IntrinsicDefinition'

    AttributeSet = Set[Union[AttributeID
    ]]

    @classmethod
    def from_yaml(cls, loader, node):
        arg_dict = loader.construct_mapping(node, deep=True)
        res = cls(**arg_dict)
        return res

    def __init__(self, name : str, comment : str, return_definition : ReturnDefinition,
                 arguments : List[ArgumentDefinition], attributes : AttributeSet,
                 memory_effects : List[MemoryRestriction] = [ MemoryRestriction() ]):
        self.name = QuotedString(name)
        self.comment = QuotedString(comment)
        self.return_definition = return_definition
        self.arguments = arguments

        self.attributes = [attr for attr in attributes if isinstance(attr, AttributeID)]
        self.attributes = sorted(self.attributes, key=lambda x: x.__str__())

        if len(memory_effects) > 1:
            # TODO: To support LLVM 16-style memory effects per multiple locations,
            # we'll need to gather multiple MemoryRestriction entries into a
            # 'location <- access' dict.
            raise TypeError("Multiple MemoryRestriction entries cannot be supported"
                            "until IGC fully switches to LLVM 16")
        self.memory_effects = memory_effects

    def __repr__(self):
        fmt_string = "%s(name=%r, comment=%r, return_definition=%r, arguments=%r, attributes=%r"
        fmt_string += ", memory_effects=%r)"
        return fmt_string % (
            self.__class__.__name__, self.name, self.comment, self.return_definition,
            self.arguments,
            self.attributes,
            self.memory_effects)

    def to_dict(self):
        res = {
            "name": self.name,
            "comment": self.comment,
            "return_definition": self.return_definition.to_dict(),
            "arguments":[ el.to_dict() for el in self.arguments],
            "attributes": [str(el) for el in self.attributes],
            "memory_effects": [ el.to_dict() for el in self.memory_effects ]
        }
        return res

    @staticmethod
    def from_dict(json_dct : dict):
        return_definition = ArgumentDefinition.from_dict(json_dct['return_definition'])
        arguments = []
        for arg in json_dct['arguments']:
            arguments.append(ArgumentDefinition.from_dict(arg))
        attributes = set(AttributeID.from_str(el) for el in json_dct['attributes'])
        memory_effects_entry = json_dct.get('memory_effects')
        memory_effects = (list(MemoryRestriction.from_dict(el) for el in memory_effects_entry)
                              if memory_effects_entry else []
                         )
        return IntrinsicDefinition(json_dct['name'], json_dct['comment'], return_definition,
                                   arguments,
                                   attributes
                                   , memory_effects)

class PrimitiveArgumentDefinition(SafeYAMLObject):

    yaml_tag = u'PrimitiveArgumentDefinition'

    @classmethod
    def from_yaml(cls, loader, node):
        arg_dict = loader.construct_mapping(node, deep=True)
        return cls(**arg_dict)

    def __init__(self, name : str, comment : str):
        self.name = name
        self.comment = comment

    def __repr__(self):
        return "%s(name=%r, comment=%r)" % (
            self.__class__.__name__, self.name, self.comment)

    def to_dict(self):
        res =  {
            "name": self.name.to_dict(),
            "comment": self.comment.to_dict()
        }
        return res

    @staticmethod
    def from_dict(json_dct : dict):
        return PrimitiveArgumentDefinition(json_dct['name'], json_dct['commnet'])

class IntrinsicPrimitive(SafeYAMLObject):

    yaml_tag = u'IntrinsicPrimitive'

    @classmethod
    def from_yaml(cls, loader, node):
        arg_dict = loader.construct_mapping(node, deep=True)
        return cls(**arg_dict)

    def __init__(self, name : str, comment : str, arguments : List[PrimitiveArgumentDefinition]):
        self.name = QuotedString(name)
        self.comment = QuotedString(comment)
        self.arguments = arguments

    def __repr__(self):
        return "%s(name=%r, comment=%r, arguments=%r)" % (
            self.__class__.__name__, self.name, self.comment, self.arguments)

    def to_dict(self):
        res = {
            "name": self.name,
            "comment": self.comment,
            "arguments":[ el.to_dict() for el in self.arguments]
        }
        return res

    @staticmethod
    def from_dict(json_dct : dict):
        arguments = []
        for arg in json_dct['arguments']:
            arguments.append(PrimitiveArgumentDefinition.from_dict(arg))
        return IntrinsicDefinition(json_dct['name'], json_dct['comment'], arguments)

class InternalGrammar(SafeYAMLObject):

    yaml_tag = u'InternalGrammar'

    def __init__(self, types : List[TypeDefinition], intrinsics : List[IntrinsicDefinition]):
        self.types = types
        self.intrinsics = intrinsics

    def __repr__(self):
        return "%s(types=%r, intrinsics=%r)" % (
            self.__class__.__name__, self.types, self.intrinsics)

    def to_dict(self):
        res = {
            "types": [ el.to_dict()for el in self.types ],
            "intrinsics": [ el.to_dict()for el in self.intrinsics ]
        }
        return res

    @staticmethod
    def from_dict(json_dct : dict):
        types = []
        for arg in json_dct['types']:
            types.append(TypeDefinition.from_dict(arg))
        intrinsics = []
        for arg in json_dct['intrinsics']:
            intrinsics.append(IntrinsicDefinition.from_dict(arg))
        return InternalGrammar(types, intrinsics)

if __name__ == '__main__':
    import sys
    import argparse
    from Intrinsic_utils import *
    import json

    def main(args):
        parser = argparse.ArgumentParser(description='Recreate a file with IGC intrinsic definitions.')
        parser.add_argument("input", help="the source path to the file with intrinsic defintions")
        parser.add_argument('--input_format',
                    default='yaml',
                    choices=['yaml', 'json'],
                    help='the data representation format of the input')
        parser.add_argument('--output_format',
                    default='yaml',
                    choices=['yaml', 'json'],
                    help='the data representation format of the output')
        parser.add_argument("--output", help="the destination path for the file with intrinsic definitions",
                        type=str)
        parser.add_argument("-v", "--verbose", help="print intrinsic definitions in the current IGC format to the console",
                        action="store_true")
        parser.add_argument("-l", "--license_header", help="attaches a license header to the output file",
                        action="store_true")

        args = parser.parse_args(args[1:])

        with open(args.input) as f:
            try:
                if args.input_format == 'json':
                    internal_grammar = InternalGrammar.from_dict(json.load(f))

                else:
                    internal_grammar = yaml.safe_load(f)
            except Exception as err:
                print("Error on loading data from: {}\n{}".format(args.input, err))

        if args.output_format == 'json':
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
