# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

import os
import sys
import re
import importlib

# Compatibility with Python 3.X
if sys.version_info[0] >= 3:
    global reduce
    import functools
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

attribute_map = {
    "None":                set(["NoUnwind"]),
    "NoMem":               set(["NoUnwind","ReadNone"]),
    "ReadMem":             set(["NoUnwind","ReadOnly"]),
    "ReadArgMem":          set(["NoUnwind","ReadOnly","ArgMemOnly"]),
    "WriteArgMem":         set(["NoUnwind","WriteOnly","ArgMemOnly"]),
    "WriteMem":            set(["NoUnwind","WriteOnly"]),
    "ReadWriteArgMem":     set(["NoUnwind","ArgMemOnly"]),
    "NoReturn":            set(["NoUnwind","NoReturn"]),
    "NoDuplicate":         set(["NoUnwind","NoDuplicate"]),
    "Convergent":          set(["NoUnwind","Convergent"]),
    "InaccessibleMemOnly": set(["NoUnwind","InaccessibleMemOnly"]),
}

# order taken from IntrinsicEmitter::EmitAttributes to match attribute order used for llvm intrinsics
attribute_order = ("NoUnwind", "NoReturn", "NoDuplicate", "Convergent", "ReadNone", "WriteOnly", "ReadOnly", "ArgMemOnly", "InaccessibleMemOnly")

def getAttributeList(Attrs):
    """
    Takes a list of attribute names, calculates the union,
    and returns a list of the the given attributes
    """
    s = reduce(lambda acc, v: attribute_map[v] | acc, Attrs, set())
    # sort attributes to generate the same code each time
    l = sorted(list(s), key=attribute_order.index)
    return ['Attribute::'+x for x in l]

Intrinsics = dict()
parse = sys.argv

turnOnComments = True;

for i in range(len(parse)):
    #Populate the dictionary with the appropriate Intrinsics
    if i != 0:
        if parse[i] == "false":
            turnOnComments = False;
        if (".py" in parse[i]):
            module = importlib.import_module(os.path.split(parse[i])[1].replace(".py",""))
            Intrinsics.update(module.Imported_Intrinsics)

# Output file is always last
outputFile = parse[-1]

ID_array = sorted(Intrinsics) #This is the ID order of the Intrinsics

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

def generateEnums():
    f = open(outputFile,"a")
    f.write("// Enum values for Intrinsics.h\n"
            "#ifdef GET_INTRINSIC_ENUM_VALUES\n")
    for i in range(len(ID_array)):
        pretty_indent = 40 - len(ID_array[i])
        f.write("  " + ID_array[i]+",")
        f.write((" "*pretty_indent)+'// llvm.genx.'+ID_array[i].replace("_",".")+'\n')
    f.write("#endif\n\n")
    f.close()

def generateIDArray():
    f = open(outputFile,"a")
    f.write("// Intrinsic ID to name table\n"
            "#ifdef GET_INTRINSIC_NAME_TABLE\n")
    for i in range(len(ID_array)):
        f.write('  ')
        f.write('"llvm.genx.'+ID_array[i].replace("_",".")+'"')
        f.write(',\n')
    f.write("#endif\n\n")
    f.close()

def numberofCharacterMatches(array_of_strings):
    other_array = []
    if isinstance(array_of_strings,list):
        for i in range(len(array_of_strings)):
            final_num = 0
            char_string = str()
            for j in range(len(array_of_strings[i])):
                char_string += array_of_strings[i][j]
                matching = [s for s in array_of_strings if char_string == s[:len(char_string)]]
                if len(matching) <= 1:
                    break
                else:
                    final_num += 1
            other_array.append([final_num-7,array_of_strings[i][7:]]) #Subtract 7 because of GenISA_
    return other_array


def sortedIntrinsicsOnLenth():
    final_array = []
    special_array = numberofCharacterMatches(ID_array)
    for i in range(len(special_array)):
        pair = [special_array[i][1] + "@","GenISAIntrinsic::GenISA_"+special_array[i][1]]
        final_array.append([special_array[i][0],pair])

    f = open(outputFile,"a")
    f.write("// Sorted by length table\n"
            "#ifdef GET_FUNCTION_RECOGNIZER\n\n"
            "struct IntrinsicEntry\n"
            "{\n"
            "   unsigned num;\n"
            "   GenISAIntrinsic::ID id;\n"
            "   const char* str;\n};\n\n"
            "static const std::array<IntrinsicEntry,"+str(len(final_array))+"> LengthTable = {{\n")
    for i in range(len(final_array)):
        #Go through and write each element
        f.write("{ "+str(final_array[i][0])+", "+str(final_array[i][1][1])+", ")
        f.write("\""+str(final_array[i][1][0])+"\"")
        f.write("}")
        if i != len(final_array) - 1:
            f.write(", ")
        if i%2 == 0:
            f.write("\n")
    f.write("}};\n\n")

    #Now to write the algorithm to search
    f.write("std::string input_name(Name);\n"
            "unsigned start = 0;\n"
            "unsigned end = "+str(len(final_array))+";\n"
            "unsigned initial_size = end;\n"
            "unsigned cur_pos = (start + end) / 2;\n"
            "char letter;\n"
            "char input_letter;\n"
            "bool isError = false;\n"
            "bool bump = false;\n"
            "unsigned start_index = std::string(\"llvm.genx.GenISA.\").size();\n"
            "for (unsigned i = 0; i < Len; i++)\n"
            "{\n"
            "    input_letter = input_name[start_index + i];\n"
            "    unsigned counter = 0;\n"
            "    while (1)\n"
            "    {\n"
            "        if (counter == initial_size || cur_pos >= initial_size)\n"
            "        {\n"
            "            isError = true;\n"
            "            break;\n"
            "        }\n"
            "        counter++;\n"
            "        letter = LengthTable[cur_pos].str[i];\n"
            "        if (letter == input_letter)\n"
            "        {\n"
            "            if (LengthTable[cur_pos].num == i)\n"
            "                return LengthTable[cur_pos].id;\n"
            "            bump = true;\n"
            "            break;\n"
            "        }\n"
            "        else if (input_letter == '\\0' && letter == '@')\n"
            "            return LengthTable[cur_pos].id;\n"
            "        else if (input_letter == '.' && letter == '_')\n"
            "            break;\n"
            "        else if (input_letter == '.' && letter == '@')\n"
            "        {\n"
            "            unsigned original_cur_pos = cur_pos;\n"
            "            while (1)\n"
            "            {\n"
            "                if (cur_pos >= initial_size || LengthTable[cur_pos].num < i)\n"
            "                    return LengthTable[original_cur_pos].id;\n"
            "                if (LengthTable[cur_pos].str[i] == '_')\n"
            "                    break;\n"
            "                cur_pos += 1;\n"
            "            }\n"
            "            break;\n"
            "        }\n"
            "        else if ((bump && letter < input_letter) || letter == '@')\n"
            "        {\n"
            "            cur_pos += 1;\n"
            "            continue;\n"
            "        }\n"
            "        else if (bump && letter > input_letter)\n"
            "        {\n"
            "            cur_pos -= 1;\n"
            "            continue;\n"
            "        }\n"
            "        else if (letter < input_letter)\n"
            "            start = cur_pos;\n"
            "        else\n"
            "            end = cur_pos;\n"
            "        cur_pos = (start + end) / 2;\n"
            "    }\n"
            "    if (isError)\n"
            "        break;\n"
            "}\n")
    f.write("\n#endif\n\n")
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
        genISA_Intrinsic = Intrinsics[ID_array[i]][1]
        for j in range(len(genISA_Intrinsic)):
            if isinstance(genISA_Intrinsic[j],list):
                for z in range(len(genISA_Intrinsic[j])):
                    if "any" in genISA_Intrinsic[j][z][0]:
                        isOverloadable = True
                        break
            else:
                if "any" in genISA_Intrinsic[j][0]:
                    isOverloadable = True
                    break
        if isOverloadable:
            f.write(" | (1U<<" + str((i+1)%8) + ")")
    f.write("\n};\n\n")
    f.write("return (OTable[id/8] & (1 << (id%8))) != 0;\n")
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
        encoded_default_value = encodeTypeString([(default_value, "")], str(), [])[0] #encode the default value
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
        if isinstance(array_of_types[j][0],int):
            type_string += array_of_anys[array_of_types[j][0]]
        elif array_of_types[j][0] in type_map:
            type_string += type_map[array_of_types[j][0]]
        else: #vector or any case
            if "any" in array_of_types[j][0]:
                new_string = addAnyTypes(array_of_types[j][0], len(array_of_anys))
                type_string += new_string
                array_of_anys.append(new_string)
            elif "ptr_" in array_of_types[j][0]:
                type_string += pointerTypesi8_map[array_of_types[j][0]]
            else:
                type_string += addVectorTypes(array_of_types[j][0])
    return [type_string,array_of_anys]


def createTypeTable():
    IIT_Basic = []
    IIT_Long = []
    # For the first part we will create the basic type table
    for i in range(len(ID_array)):
        genISA_Intrinsic = Intrinsics[ID_array[i]][1] # This is our array of types
        dest = genISA_Intrinsic[0]
        source_list = genISA_Intrinsic[1]
        anyArgs_array = []
        type_string = str()

        #Start with Destination
        if isinstance(dest[0],str):
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
    f.write("// Add parameter attributes that are not common to all intrinsics.\n"
            "#ifdef GET_INTRINSIC_ATTRIBUTES\n"
            "static AttributeList getAttributes(LLVMContext &C, GenISAIntrinsic::ID id) {\n"
            "  static const uint8_t IntrinsicsToAttributesMap[] = {\n")
    attribute_Array = []
    for i in range(len(ID_array)):
        found = False
        intrinsic_attribute = Intrinsics[ID_array[i]][1][2] #This is the location of that attribute
        for j in range(len(attribute_Array)):
            if intrinsic_attribute == attribute_Array[j]:
                found = True
                f.write("    " + str(j+1) + ", // llvm.genx." + ID_array[i].replace("_",".") + "\n")
                break
        if not found:
            f.write("    " + str(len(attribute_Array)+1) + ", // llvm.genx." + ID_array[i].replace("_",".") + "\n")
            attribute_Array.append(intrinsic_attribute)
    f.write("  };\n\n")

    f.write("  AttributeList AS[1];\n" #Currently only allowed to have one attribute per instrinsic
            "  unsigned NumAttrs = 0;\n"
            "  if (id != 0) {\n"
            "    switch(IntrinsicsToAttributesMap[id - Intrinsic::num_intrinsics]) {\n"
            "    default: IGC_ASSERT_EXIT_MESSAGE(0, \"Invalid attribute number\");\n")

    for i in range(len(attribute_Array)): #Building case statements
        Attrs = getAttributeList([x.strip() for x in attribute_Array[i].split(',')])
        f.write("""    case {num}: {{
      const Attribute::AttrKind Atts[] = {{{attrs}}};
      AS[0] = AttributeList::get(C, AttributeList::FunctionIndex, Atts);
      NumAttrs = 1;
      break;
      }}\n""".format(num=i+1, attrs=','.join(Attrs)))
    f.write("    }\n"
            "  }\n"
            "  return AttributeList::get(C, makeArrayRef(AS, NumAttrs));\n"
            "}\n"
            "#endif // GET_INTRINSIC_ATTRIBUTES\n\n")
    f.close()

def compileComments():
    f = open(outputFile,"a")
    f.write("//Comments for each Intrinsic\n"
            "#ifdef GET_INTRINSIC_COMMENTS_TABLE\n")
    f.write("static const std::array<IntrinsicComments,"+str(len(ID_array))+"> IDComments = {{\n")
    for i in range(len(ID_array)):
        func_desc = Intrinsics[ID_array[i]][0]
        output_array = Intrinsics[ID_array[i]][1][0]
        input_array = Intrinsics[ID_array[i]][1][1]
        f.write('{"'+func_desc+'", {')
        if len(output_array) == 2 and isinstance(output_array[0], str):
            f.write('"'+output_array[1]+'"')
        else:
            if len(output_array) == 0:
                f.write('"none"')
            else:
                for o in range(len(output_array)):
                    f.write('"'+output_array[o][1]+'"')
                    if o != (len(output_array) - 1):
                        f.write(",")
        f.write('}, {')
        if len(input_array) == 0:
            f.write('"none"')
        else:
            for input in range(len(input_array)):
                f.write('"'+input_array[input][1]+'"')
                if input != (len(input_array) - 1):
                    f.write(",")
        f.write('}}')
        if i != (len(ID_array) - 1):
            f.write(",\n")
    f.write('}};\n')
    f.write("return IDComments[id-Intrinsic::num_intrinsics - 1];\n")
    f.write("#endif //GET_INTRINSIC_COMMENTS_TABLE\n")
    f.close()

def emitSuffix():
    f = open(outputFile,"a")
    f.write("#if defined(_MSC_VER) && defined(setjmp_undefined_for_msvc)\n"
            "// let's return it to _setjmp state\n"
            "#  pragma pop_macro(\"setjmp\")\n"
            "#  undef setjmp_undefined_for_msvc\n"
            "#endif\n\n")
    f.close()

#main functions in order
emitPrefix()
generateEnums()
generateIDArray()
createOverloadTable()
sortedIntrinsicsOnLenth()
createTypeTable()
createAttributeTable()
if turnOnComments:
    compileComments()
else:
    f = open(outputFile,"a")
    f.write("#ifdef GET_INTRINSIC_COMMENTS_TABLE\n")
    f.write('return {"", {""}, {""}};\n')
    f.write("#endif //GET_INTRINSIC_COMMENTS_TABLE\n\n")
    f.close()
emitSuffix()