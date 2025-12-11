#!/usr/bin/env python

# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2022-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

import os
import sys
import re
import importlib
import functools

# Compatibility with Python 3.X
if sys.version_info[0] >= 3:
    global reduce
    reduce = functools.reduce


OverloadedTypes = ["any","anyint","anyfloat","anyptr","anyvector"]
VectorTypes = ["2","4","8","16"]
DestSizes = ["","","21","22","23","24"]

type_map = \
{
    "void":"0",
    "bool":"1",
    "char":"2",
    "short":"3",
    "int":"4",
    "long":"5",
    "half":"6",
    "float":"7",
    "double":"8",
    "2":"9",
    "4":"A",
    "8":"B",
    "16":"C",
    "32":"D",
}

pointerTypesi8_map = \
{
    "ptr_private":"E2",
    "ptr_global":"<27>12",
    "ptr_constant":"<27>22",
    "ptr_local":"<27>32",
    "ptr_generic":"<27>42",
}

any_map = \
{
    "any":0,
    "anyint":1,
    "anyfloat":2,
    "anyvector":3,
    "anyptr":4,
}

vararg_val = "<29>"

# FIXME: Once multiple attribute entries per intrinsic are supported within
# 'createAttributeTable()', drop this translation table as well as the caller function,
# turning to explicit specification of "NoUnwind" in the intrinsic definitions.
attribute_map = {
    "None":                set(["NoUnwind"]),
    "NoReturn":            set(["NoUnwind","NoReturn"]),
    "NoDuplicate":         set(["NoUnwind","NoDuplicate"]),
    "Convergent":          set(["NoUnwind","Convergent"]),
    "SideEffects":         set(["NoUnwind"]),
    "NoWillReturn":        set(["NoUnwind"]),
}

def getAttributeList(Attrs):
    """
    Takes a list of attribute names, calculates the union,
    and returns a list of the the given attributes
    """
    s = reduce(lambda acc, v: attribute_map[v] | acc, Attrs, set())
    return ['Attribute::'+x for x in sorted(s)]

def getAttributeListWithWillReturn(Attrs):
    """
    Like getAttributeList, but adds WillReturn unless explicitly disabled
    by NoWillReturn or NoReturn.
    """
    s = reduce(lambda acc, v: attribute_map[v] | acc, Attrs, set())
    attr = []
    is_return = "NoWillReturn" not in Attrs
    for x in sorted(s):
        attr.append('AB.addAttribute(Attribute::' + x + ')')
        if x == "NoReturn": is_return = False
    if is_return: attr.append('AB.addAttribute(Attribute::WillReturn)')
    return attr

Intrinsics = dict()
parse = sys.argv

for i in range(len(parse)):
    #Populate the dictionary with the appropriate Intrinsics
    if i != 0:
        if (".py" in parse[i]):
            module = importlib.import_module(os.path.split(parse[i])[1].replace(".py",""))
            Intrinsics.update(module.Imported_Intrinsics)

# Output file is always last
outputFile = parse[-1]


def ik_compare(ikl, ikr):
  ikl = ikl.replace("_",".")
  ikr = ikr.replace("_",".")
  if ikl < ikr:
    return -1
  elif ikl > ikr:
    return 1
  else:
    return 0
# NOTE: the ordering does matter here as lookupLLVMIntrinsicByName depend on it
ID_array = sorted(Intrinsics, key = functools.cmp_to_key(ik_compare))

def emitPrefix():
    f = open(outputFile,"w")
    f.write("// VisualStudio defines setjmp as _setjmp\n"
            "#if defined(_MSC_VER) && defined(setjmp) && \\\n"
            "                         !defined(setjmp_undefined_for_msvc)\n"
            "#  pragma push_macro(\"setjmp\")\n"
            "#  undef setjmp\n"
            "#  define setjmp_undefined_for_msvc\n"
            "#endif\n\n")
    f.close()

def createTargetData():
    f = open(outputFile,"a")
    f.write("// Target mapping\n"
            "#ifdef GET_INTRINSIC_TARGET_DATA\n")
    f.write(
      "struct IntrinsicTargetInfo {\n"
      "  llvm::StringLiteral Name;\n"
      "  size_t Offset;\n"
      "  size_t Count;\n"
      "};\n"
      "static constexpr IntrinsicTargetInfo TargetInfos[] = {\n"
      "  {llvm::StringLiteral(\"\"), 0, 0},\n"
      "  {llvm::StringLiteral(\"vc.internal\"), 0, " + str(len(ID_array)) + "},\n"
      "};\n")
    f.write("#endif\n\n")
    f.close()

def generateEnums():
    f = open(outputFile,"a")
    f.write("// Enum values for Intrinsics.h\n"
            "#ifdef GET_INTRINSIC_ENUM_VALUES\n")
    for i in range(len(ID_array)):
        pretty_indent = 40 - len(ID_array[i])
        f.write( ID_array[i]+",")
        f.write((" "*pretty_indent)+'// llvm.vc.internal.'+ID_array[i].replace("_",".")+'\n')
    f.write("#endif\n\n")
    f.close()

def generateIDArray():
    f = open(outputFile,"a")
    f.write("// Intrinsic ID to name table\n"
            "#ifdef GET_INTRINSIC_NAME_TABLE\n")
    for i in range(len(ID_array)):
        f.write('  ("llvm.vc.internal.'+ID_array[i].replace("_",".")+'"),\n')
    f.write("#endif\n\n")
    f.close()

def createOverloadTable():
    f = open(outputFile,"a")
    f.write("// Intrinsic ID to overload bitset\n"
            "#ifdef GET_INTRINSIC_OVERLOAD_TABLE\n"
            "static const uint8_t OTable[] = {\n  0")
    for i in range(len(ID_array)):
        if ((i+1)%8 == 0):
            f.write(",\n  0")
        isOverloadable = False
        genISA_Intrinsic = Intrinsics[ID_array[i]]
        for key in genISA_Intrinsic:
            val = genISA_Intrinsic[key]
            if isinstance(val,list):
                for z in range(len(val)):
                    if isinstance(val[z],int):
                        continue
                    elif "any" in val[z]:
                        isOverloadable = True
                        break
            else:
                if "any" in val:
                    isOverloadable = True
                    break
        if isOverloadable:
            f.write(" | (1U<<" + str((i+1)%8) + ")")
    f.write("\n};\n\n")
    f.write("IGC_ASSERT( ((id / 8) < (sizeof(OTable) / sizeof(OTable[0]))) && "
            "\"Overload Table index overflow\");\n");
    f.write("return (OTable[id/8] & (1 << (id%8))) != 0;\n")
    f.write("#endif\n\n")
    f.close()

def createOverloadRetTable():
    f = open(outputFile,"a")
    f.write("// Is ret overloaded\n"
            "#ifdef GET_INTRINSIC_OVERLOAD_RET_TABLE\n"
            "switch(IntrinID) {\n"
            "default:\n"
            "  return false;\n")
    for i in range(len(ID_array)):
        genISA_Intrinsic = Intrinsics[ID_array[i]]
        isOverloadable = False
        ret = genISA_Intrinsic["result"]
        if "any" in ret:
            isOverloadable = True
        elif isinstance(ret, list):
            for j in range(len(ret)):
                if "any" in ret[j]:
                    isOverloadable = True
        if isOverloadable:
            f.write("case InternalIntrinsic::" + ID_array[i] + ":\n")
        isOverloadable = False
    f.write("  return true;\n")
    f.write("}\n")
    f.write("#endif\n\n")
    f.close()

def createOverloadArgsTable():
    f = open(outputFile,"a")
    f.write("// Is arg overloaded\n"
            "#ifdef GET_INTRINSIC_OVERLOAD_ARGS_TABLE\n"
            "switch(IntrinID) {\n"
            "default: IGC_ASSERT_EXIT_MESSAGE(0, \"Unknown intrinsic ID\");\n")
    for i in range(len(ID_array)):
        f.write("case InternalIntrinsic::" + ID_array[i]+": ")
        argNums = []
        genISA_Intrinsic = Intrinsics[ID_array[i]]
        args  = genISA_Intrinsic["arguments"]
        if isinstance(args,list):
            for z in range(len(args)):
                if isinstance(args[z],int):
                    continue
                elif "any" in args[z]:
                    argNums.append(z)
        else:
            if "any" in args:
                argNums.append(0)
        if not argNums:
            f.write("\n   return false;\n")
        else:
            f.write("{\n    switch(ArgNum) {\n"
                    "   default: return false;\n")
            for arg in argNums:
                f.write("   case " + str(arg) + ": return true;\n")
            f.write("   }\n}\n")
    f.write("}\n")
    f.write("#endif\n\n")
    f.close()

def addAnyTypes(value,argNum):
    return_val = str()
    default_value = str()
    if "any:" in value:
        default_value = value[4:] #get the default value encoded after the "any" type
        value = "any"
    calculated_num = (argNum << 3) | any_map[value]
    if calculated_num < 16:
        return_val = hex(calculated_num).upper()[2:]
    else:
        return_val = "<" + str(calculated_num) + ">" #Can't represent in hex we will need to use long table
    return_val = "F" + return_val
    if default_value:
        encoded_default_value = encodeTypeString([default_value], str(), [])[0] #encode the default value
        return_val = return_val + encoded_default_value
    return return_val

def addVectorTypes(source):
    vec_str = str()
    for vec in range(len(VectorTypes)):
        if VectorTypes[vec] in source:
            vec_str = type_map[source.split(VectorTypes[vec])[0]]
            vec_str = type_map[VectorTypes[vec]] + vec_str
            break
    return vec_str

def encodeTypeString(array_of_types,type_string,array_of_anys):
    for j in range(len(array_of_types)):
        if isinstance(array_of_types[j],int):
            type_string += array_of_anys[array_of_types[j]]
        elif array_of_types[j] in type_map:
            type_string += type_map[array_of_types[j]]
        elif array_of_types[j] == "vararg":
            type_string += vararg_val
        else: #vector or any case
            if "any" in array_of_types[j]:
                new_string = addAnyTypes(array_of_types[j], len(array_of_anys))
                type_string += new_string
                array_of_anys.append(new_string)
            elif "ptr_" in array_of_types[j]:
                type_string += pointerTypesi8_map[array_of_types[j]]
            else:
                type_string += addVectorTypes(array_of_types[j])
    return [type_string,array_of_anys]


def createTypeTable():
    IIT_Basic = []
    IIT_Long = []
    # For the first part we will create the basic type table
    for i in range(len(ID_array)):
        genISA_Intrinsic = Intrinsics[ID_array[i]] # This is our array of types
        dest = genISA_Intrinsic['result']
        source_list = genISA_Intrinsic['arguments']
        anyArgs_array = []
        type_string = str()

        #Start with Destination
        if isinstance(dest,str):
            dest = [dest]
        else:
            if len(dest) > 1:
                type_string = "<" + DestSizes[len(dest)] + ">"

        dest_result = encodeTypeString(dest,type_string,anyArgs_array)
        type_string = dest_result[0]
        anyArgs_array = dest_result[1]

        #Next we go over the Source
        source_result = encodeTypeString(source_list,type_string,anyArgs_array)
        type_string = source_result[0]

        array_of_longs = re.findall("(?<=\<)(.*?)(?=\>)",type_string) #Search for my long values <>
        type_string = re.sub("(<)(.*?)(>)",".",type_string) #Replace long_nums for now with .
        IIT_Basic.append(["0x"+type_string[::-1],array_of_longs]) #Reverse the string before appending and add array of longs


    # Now we will create the table for entries that take up more than 4 bytes
    pos_counter = 0 #Keeps track of the position in the Long Encoding table
    for i in range(len(IIT_Basic)):
        isGreaterThan10 = len(IIT_Basic[i][0]) >= 10
        isLongArrayUsed = len(IIT_Basic[i][1]) > 0
        if isGreaterThan10 or isLongArrayUsed:
            hex_list = list(reversed(IIT_Basic[i][0][2:])) #remove "0x"
            if len(hex_list) == 8 and not isLongArrayUsed and int(hex_list[-1],16) < 8: #checks if bit 32 is used
                continue;
            IIT_Basic[i][0] = "(1U<<31) | " + str(pos_counter)
            long_counter = 0
            for j in range(len(hex_list)):
                if hex_list[j] == ".": #Now to replace the "." with an actual number
                    IIT_Long.append(int(IIT_Basic[i][1][long_counter]))
                    long_counter += 1
                else:
                    IIT_Long.append(int(hex_list[j],16)) # convert hex to int
                pos_counter += 1
            IIT_Long.append(-1) #keeps track of new line add at the end
            pos_counter += 1

    #Write the IIT_Table
    f = open(outputFile,"a")
    f.write("// Global intrinsic function declaration type table.\n"
            "#ifdef GET_INTRINSIC_GENERATOR_GLOBAL\n"
            "static const unsigned IIT_Table[] = {\n  ")
    for i in range(len(IIT_Basic)): #write out the IIT_Table
        f.write(str(IIT_Basic[i][0]) + ", ")
        if i%8 == 7:
            f.write("\n  ")
    f.write("\n};\n\n")

    #Write the IIT_LongEncodingTable
    f.write("static const unsigned char IIT_LongEncodingTable[] = {\n  /* 0 */ ")
    for i in range(len(IIT_Long)):
        newline = False
        if IIT_Long[i] == -1:
            IIT_Long[i] = 0
            newline = True
        f.write(str(IIT_Long[i]) + ", ")
        if newline and i != len(IIT_Long)-1:
            f.write("\n  /* "+ str(i+1) + " */ ")
    f.write("\n  255\n};\n\n#endif\n\n")
    f.close()

def createAttributeTable():
    f = open(outputFile,"a")
    # FIXME: Re-implement without the restriction on the number of explicit attrkind
    # specifications per intrinsic. Nothing blocks us from creating an in-scope 'static'
    # map of ArrayRef<AttrKind> per intrinsic ID. AttrBuilder / AttributeList classes
    # provide enough capabilities for convenient LLVMContext-ualization of that array,
    # and subsequent addition of memory attribute(s).
    f.write("// Add parameter attributes that are not common to all intrinsics.\n"
            "#ifdef GET_INTRINSIC_ATTRIBUTES\n"
            "AttributeList InternalIntrinsic::getAttributes(LLVMContext &C, InternalIntrinsic::ID id) {\n"
            "  static const uint8_t IntrinsicsToAttributesMap[] = {\n")
    attribute_Array = []
    for i in range(len(ID_array)):
        found = False
        intrinsic_attribute = Intrinsics[ID_array[i]]['attributes'] #This is the location of that attribute
        for j in range(len(attribute_Array)):
            if intrinsic_attribute == attribute_Array[j]:
                found = True
                f.write("    " + str(j+1) + ", // llvm.vc.internal." + ID_array[i].replace("_",".") + "\n")
                break
        if not found:
            f.write("    " + str(len(attribute_Array)+1) + ", // llvm.vc.internal." + ID_array[i].replace("_",".") + "\n")
            attribute_Array.append(intrinsic_attribute)
    f.write("  };\n\n")

    f.write("  using MemoryEffectsTy = IGCLLVM::MemoryEffects;\n"
            "  static const MemoryEffectsTy MemoryFXPerIntrinsicMap[] = {\n")
    # Iterate over intrinsic definitions
    for i in range(len(ID_array)):
        memory_effects_entry = Intrinsics[ID_array[i]].get('memory_effects')
        if not memory_effects_entry:
            memory_effects_def = "MemoryEffectsTy::unknown()"
        else:
            # TODO: Extend the code for LLVM 16+ support of multiple access-per-location entries
            memory_effects_def = "MemoryEffectsTy("
            memory_location = memory_effects_entry.get('location')
            memory_access = memory_effects_entry.get('access')
            if memory_location:
                memory_effects_def += "IGCLLVM::ExclusiveIRMemLoc::{loc}".format(loc=memory_location)
                if memory_access:
                    memory_effects_def += ", "
            if memory_access:
                memory_effects_def += "IGCLLVM::ModRefInfo::{acc})".format(acc=memory_access)
        f.write("    " + memory_effects_def + ", // llvm.vc.internal." + ID_array[i].replace("_",".") + "\n")
    f.write("  };\n\n")

    f.write("  unsigned AttrIdx = id - 1 - InternalIntrinsic::not_internal_intrinsic;\n"
            "  #ifndef NDEBUG\n"
            "  const size_t AttrMapNum = sizeof(IntrinsicsToAttributesMap)/sizeof(IntrinsicsToAttributesMap[0]);\n"
            "  IGC_ASSERT(AttrIdx < AttrMapNum && \"invalid attribute index\");\n"
            "  #endif // NDEBUG\n")

    f.write("  AttrBuilder AB(C);\n"
            "  if (id != 0) {\n"
            "    switch(IntrinsicsToAttributesMap[AttrIdx]) {\n"
            "    default: IGC_ASSERT_EXIT_MESSAGE(0, \"Invalid attribute number\");\n")

    for i in range(len(attribute_Array)): #Building case statements
        Attrs = getAttributeList([x.strip() for x in attribute_Array[i].split(',')])
        AttrsWithWillReturn = getAttributeListWithWillReturn([x.strip() for x in attribute_Array[i].split(',')])
        f.write("""    case {num}: {{
      #if LLVM_VERSION_MAJOR >= 16
      {attrs_will};
      #else
      const Attribute::AttrKind Attrs[] = {{{attrs}}};
      for (auto Kind : Attrs) {{
        AB.addAttribute(Kind);
      }}
      #endif
      break;
      }}\n""".format(num=i+1, attrs_will='; '.join(AttrsWithWillReturn), attrs=','.join(Attrs)))

    f.write("    }\n"
            "  }\n"
            "  MemoryEffectsTy ME = MemoryFXPerIntrinsicMap[AttrIdx];\n"
            "  AB.merge(ME.getAsAttrBuilder(C));\n"
            "  return AttributeList::get(C, AttributeList::FunctionIndex, AB);\n"
            "}\n"
            "#endif // GET_INTRINSIC_ATTRIBUTES\n\n")
    f.close()

def createPlatformTable():
    key = 'target'

    # Collect all the required target features
    targetFeatures = set()
    for desc in Intrinsics.values():
        feat = [x[1:] if x[0] == '!' else x for x in desc.get(key, [])]
        targetFeatures.update(feat)

    # Create bit mask for each target feature
    targetFeatureMap = {feature: 1 << i for i, feature in enumerate(sorted(targetFeatures))}

    # Generate the tables
    tableRequired = []
    tableForbidden = []
    for i in range(len(ID_array)):
        desc = Intrinsics[ID_array[i]]
        features = desc.get(key, [])

        required = [feature for feature in features if not feature.startswith('!')]
        tableRequired.append(reduce(lambda acc, feature: acc | targetFeatureMap[feature], required, 0))

        forbidden = [feature[1:] for feature in features if feature.startswith('!')]
        tableForbidden.append(reduce(lambda acc, feature: acc | targetFeatureMap[feature], forbidden, 0))

    numFeatures = len(targetFeatureMap)
    assert numFeatures <= 64, "Too many target features"

    storageType = 'uint64_t'
    if numFeatures <= 8:
        storageType = 'uint8_t'
    elif numFeatures <= 16:
        storageType = 'uint16_t'
    elif numFeatures <= 32:
        storageType = 'uint32_t'

    # Write the table to the output file
    with open(outputFile, 'a') as f:
        f.write("// Platform table\n"
                "#ifdef GET_INTRINSIC_TARGET_FEATURE_TABLE\n"
                f"static const {storageType} RequiredTargetFeatures[] = " "{\n")

        for i, mask in enumerate(tableRequired):
            f.write("  0x{:x}, // llvm.vc.internal.{}\n".format(mask, ID_array[i].replace("_", ".")))

        f.write("};\n"
                f"static const {storageType} ForbiddenTargetFeatures[] = " "{\n")

        for i, mask in enumerate(tableForbidden):
            f.write("  0x{:x}, // llvm.vc.internal.{}\n".format(mask, ID_array[i].replace("_", ".")))

        f.write("};\n"
                "#endif // GET_INTRINSIC_TARGET_FEATURE_TABLE\n\n")

        f.write("// Checker for platform features\n"
                "#ifdef GET_INTRINSIC_TARGET_FEATURE_CHECKER\n"
                "unsigned Index = ID - 1 - vc::InternalIntrinsic::not_internal_intrinsic;\n"
                "auto MaskRequired = RequiredTargetFeatures[Index];\n"
                "auto MaskForbidden = ForbiddenTargetFeatures[Index];\n")

        for feature, mask in targetFeatureMap.items():
            f.write(f"if ((MaskRequired & {mask}) && !{feature}()) return false;\n"
                    f"if ((MaskForbidden & {mask}) && {feature}()) return false;\n")
        f.write("#endif // GET_INTRINSIC_TARGET_FEATURE_CHECKER\n\n")


def emitSuffix():
    f = open(outputFile,"a")
    f.write("#if defined(_MSC_VER) && defined(setjmp_undefined_for_msvc)\n"
            "// let's return it to _setjmp state\n"
            "#  pragma pop_macro(\"setjmp\")\n"
            "#  undef setjmp_undefined_for_msvc\n"
            "#endif\n\n")
    f.close()

# main functions in order
emitPrefix()
createTargetData()
generateEnums()
generateIDArray()
createOverloadTable()
createOverloadArgsTable()
createOverloadRetTable()
createTypeTable()
createAttributeTable()
createPlatformTable()
emitSuffix()
