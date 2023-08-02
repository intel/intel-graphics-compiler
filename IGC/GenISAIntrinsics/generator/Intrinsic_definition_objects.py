# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

from typing import List, Set
from enum import Enum

class TypeID(Enum):
    Void = 0,
    Integer = 1,
    Float = 2,
    Vector = 3,
    Struct = 4,
    Pointer = 5,
    Any = 6,
    ArgumentReference = 7

    def __str__(self):
        return self.name

    @classmethod
    def from_str(cls, value : str):
        for key, val in cls.__members__.items():
            if key == value:
                return val
        else:
            raise ValueError("{value} is not present in {cls.__name__}")

class AddressSpace(Enum):
    Undefined = 0,
    Private = 1,
    Global = 2,
    Constant = 3,
    Local = 4,
    Generic = 5

    def __int__(self):
        return self.value[0] - 1

    @classmethod
    def from_str(cls, value : str):
        for key, val in cls.__members__.items():
            if key == value:
                return val
        else:
            raise ValueError("{value} is not present in {cls.__name__}")

class AttributeID(Enum):
    NoUnwind = 0,
    ReadNone = 1,
    ReadOnly = 2,
    ArgMemOnly = 3,
    WriteOnly = 4,
    NoReturn = 5,
    NoDuplicate = 6,
    Convergent = 7,
    InaccessibleMemOnly = 8

    def __str__(self):
        return self.name

    @classmethod
    def from_str(cls, value : str):
        for key, val in cls.__members__.items():
            if key == value:
                return val
        else:
            raise ValueError("{value} is not present in {cls.__name__}")

class TypeDefinition:
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
        elif self.ID == TypeID.ArgumentReference:
            self.index = index

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
        elif self.ID == TypeID.ArgumentReference:
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
        elif ID == TypeID.ArgumentReference:
            return TypeDefinition(ID, index=json_dct['index'])
        elif ID == TypeID.Any:
            internal_type = TypeDefinition.from_dict(json_dct['default_type']) if json_dct['default_type'] else None
            return TypeDefinition(ID, internal_type=internal_type)

class ArgumentTypeDefinition:
    def __init__(self, type_definition : TypeDefinition, comment : str):
        self.type_definition = type_definition
        self.comment = comment

    def to_dict(self):
        res =  {
            "type_definition": self.type_definition.to_dict(),
            "comment": self.comment
        }
        return res

    @staticmethod
    def from_dict(json_dct : dict):
        type_definition = TypeDefinition.from_dict(json_dct['type_definition'])
        return ArgumentTypeDefinition(type_definition, json_dct['comment'])

class IntrinsicDefinition:
    def __init__(self, name : str, comment : str, return_type : ArgumentTypeDefinition,
                 argument_types : List[ArgumentTypeDefinition], attributes : Set[AttributeID]):
        self.name = name
        self.comment = comment
        self.return_type = return_type
        self.argument_types = argument_types
        self.attributes = sorted(list(attributes), key=lambda x: x.__str__())

    def to_dict(self):
        res = {
            "name": self.name,
            "comment": self.comment,
            "return_type": self.return_type.to_dict(),
            "argument_types":[ el.to_dict() for el in self.argument_types],
            "attributes": [str(el) for el in self.attributes]
        }
        return res

    @staticmethod
    def from_dict(json_dct : dict):
        return_type = ArgumentTypeDefinition.from_dict(json_dct['return_type'])
        argument_types = []
        for arg in json_dct['argument_types']:
            argument_types.append(ArgumentTypeDefinition.from_dict(arg))
        attributes = set(AttributeID.from_str(el) for el in json_dct['attributes'])
        return IntrinsicDefinition(json_dct['name'], json_dct['comment'], return_type,
                                   argument_types, attributes)