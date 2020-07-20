import sys
import csv
import re
import pandas as pd
import os
from io import StringIO


ZEINFO_HPP = "ZEInfo.hpp"
ZEINFOYAML_HPP = "ZEInfoYAML.hpp"
ZEINFOYAML_CPP = "ZEInfoYAML.cpp"

COPYRIGHT = """/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/\n"""

ZEINFO_HPP_HEADER = """//===- ZEInfo.hpp -----------------------------------------------*- C++ -*-===//
// ZE Binary Utilitis
//
// \\file
// This file declares the struct representation of .ze.info section
//===----------------------------------------------------------------------===//\n
#ifndef ZE_INFO_HPP
#define ZE_INFO_HPP

#include <string>
#include <vector>

namespace zebin {\n
"""

YAML_CPP_HEADER = """//===- ZEInfoYAML.cpp -----------------------------------------------*- C++ -*-===//
// ZE Binary Utilitis
//
// file
//===----------------------------------------------------------------------===//\n
#include <ZEInfoYAML.hpp>
using namespace zebin;
using namespace llvm::yaml;\n
"""

YAML_HPP_HEADER = """//===- ZEInfoYAML.hpp -------------------------------------------*- C++ -*-===//
// ZE Binary Utilitis
//
// \\file
// This file declares the mapping between zeInfo structs and YAML
//===----------------------------------------------------------------------===//

#ifndef ZE_INFO_YAML_HPP
#define ZE_INFO_YAML_HPP

#include <ZEInfo.hpp>

#ifndef ZEBinStandAloneBuild
#include \"common/LLVMWarningsPush.hpp\"
#endif

#include \"llvm/Support/YAMLTraits.h\"
#ifndef ZEBinStandAloneBuild
#include \"common/LLVMWarningsPop.hpp\"
#endif
"""


"""
Given row of pandas dataframe, convert type item to zeinfo type
Passes in 'types' to ensure no new types are in the tables
"""
def convert_types(row, types):
    if row["Type"] in types or row["Type"].split("x")[0] in types or "<" in row["Type"]:
        if "<" in row["Type"]:
            return "zeinfo_str_t"
        if "x" not in row["Type"]:
            return "zeinfo_" + row["Type"] + "_t"
        else:
            return "std::vector<zeinfo_" + row["Type"].split("x")[0] + "_t>"

    if "vector" not in row["Description"]:
        return "zeInfo" + row["Type"]
        #sys.exit(row["Type"])
    return row["Type"]


"""
Create struct code in ZEInfo.hpp
"""
def format_zeinfo_struct(row):
    if "vector" in row["Type"]:
        return "    " + row["Type"] + " "+ row[row.index[0]] + ";\n"
    if row["Required/Optional"] == "Optional" and row["Default"] != "":
        return "    " + row["Type"] + " "+ row[row.index[0]] + " = " + row["Default"] + ";\n"
    if "bool" in row["Type"]:
        return "    " + row["Type"] + " "+ row[row.index[0]] + " = false;\n"
    if "str" in row["Type"]:
        return "    " + row["Type"] + " "+ row[row.index[0]] + ";\n"
    return "    " + row["Type"] + " "+ row[row.index[0]] + " = 0;\n"


"""
Create YAML mappings in ZEInfoYAML.cpp
"""
def format_zeinfoyaml_cpp_mapping(row):
    first = row[row.index[0]]
    if "str" in row["Type"]:
        return "    io.mapOptional(" + '"' + first + '"' + ", info." + first + ", std::string());\n"
    if "vector" in row["Type"]:
        return "    io.mapOptional(" + '"' + first + '"' + ", info." + first + ");\n"

    if row["Required/Optional"] == "Optional":
        default_val = row.Default
        return "    io.mapOptional(" + '"' + first + '"' + ", info." + first + ", " + default_val + ");\n"
    return "    io.mapRequired(" + '"' + first + '"' + ", info." + first + ");\n"


"""
src_lines: list of lines from the given zeinfo .md file
start: a string indicating the start of a block of text to be collected
stop: a string indicating the end of text collection
indicator: all lines containing the indicator will be collected in the text block
Returns block of text in a list format and the index in src_lines that the
collection stopped at
"""
def get_text_block(src_lines, start, stop, indicator):
    index = 0
    pounds = 0
    while index < len(src_lines):
        line = src_lines[index]
        if line.startswith(start):
            pounds = line.count(start)
            break
        index += 1
    if index == (len(src_lines) - 1):
        sys.exit("Cannot find" + start)

    index += 1
    text_block = []
    while index < len(src_lines):
        line = src_lines[index]
        if line.startswith("#"):
            break
        if line.startswith(stop) and text_block != []:
            break
        if indicator in line:
            text_block.append(line)
        index += 1

    return text_block, index, pounds


"""
Parse block of text in table format into a pandas dataframe
Returns dataframe and comment below the table
"""
def generate_pandas_df(src_lines):
    file_len = len(src_lines)
    comment = src_lines[file_len - 1]
    comment = comment[1:len(comment) - 1]
    src_lines = src_lines[:(file_len - 1)]
    file_str = "".join(src_lines)
    pandas_file = StringIO(file_str)

    df = pd.read_csv(pandas_file, delimiter="|")
    df = df.loc[:, ~df.columns.str.contains("^Unnamed")]
    df = df.iloc[1:]
    df.columns = map(str.strip, df.columns.values)
    df[df.columns] = df.apply(lambda x: x.str.strip())

    return df, comment


"""
Writes struct lines to ZEInfo.hpp
"""
def pandas_create_zeinfo_hpp(df, struct_name, output_file):
    df["Code"] = df.apply(format_zeinfo_struct, axis=1)

    struct_name = "struct zeInfo" + struct_name + "\n{\n"
    output_file.write(struct_name)
    for item in df["Code"].values:
        output_file.write(item)
    output_file.write("};\n")


"""
Write YAML lines to ZEInfoYAML.cpp
"""
def pandas_create_zeinfoyaml_cpp(df, struct_name, types, output_file):
    if "Required/Optional" not in df.columns:
        df["Required/Optional"] = "Required"
        df["Default"] = ""

    df["Type"] = df.apply(convert_types, axis=1, args=(types,))

    df["Code"] = df.apply(format_zeinfoyaml_cpp_mapping, axis=1)
    struct_line = "void MappingTraits<zeInfo" + struct_name + ">::mapping(IO& io, zeInfo" + struct_name + "& info)\n{\n"
    output_file.write(struct_line)
    for item in df["Code"].values:
        output_file.write(item)
    output_file.write("}\n")


"""
Parses top layer tables and writes to ZEInfo.hpp
Stores all struct names and whether they are a vector in yaml_hpp_args
"""
def pandas_parse_top_layers(df, types, output_file, struct_name, check_vectors):

    for index, row in df.iterrows():
        if "vector" in row["Description"]:
            if row["Type"][:-2] in check_vectors.keys():
                vector_str = "typedef std::vector<zeInfo" + \
                check_vectors[row["Type"][:-2]] + "> " + row["Type"] + ";\n"
                output_file.write(vector_str)
            else:
                sys.exit(row["Type"][:-2] + " not correctly declared as vector")

    struct_line = "struct zeInfo" + struct_name + "\n{\n"
    output_file.write(struct_line)

    # write struct lines
    for index, row in df.iterrows():
        struct_data = "    " + row["Type"] + " " + row[row.index[0]] + ";\n"
        output_file.write(struct_data)

    output_file.write("};\n")


"""
Write PreDefinedAttrGetter lines to ZEInfo.hpp
"""
def pandas_attr_getter(df, class_name, enum_lines, static_lines):
    enum_name = "    enum class " + class_name + " {\n"
    static_name = "    static zeinfo_str_t get(" + class_name + " val) {\n        switch(val) {\n"
    enum_lines.append(enum_name)
    static_lines.append(static_name)


    name_list = df.iloc[:, 0].values
    for index in range(len(name_list)):
        item = name_list[index]
        static_lines.append("        case " + class_name + "::" + item + ":\n            return \"" + item + "\";\n")

        if index != len(name_list) - 1:
            item = "        " + item + ",\n"
        else:
            item = "        " + item + "\n"
        enum_lines.append(item)
    enum_lines.append("    };\n")
    static_lines.append("        default:\n            break;\n        }\n        return \"\";\n")
    static_lines.append("    }\n")


"""
Writes code to ZEInfo.hpp and ZEInfoYAML.cpp
"""
def create_zeinfo_hpp_yaml_cpp(src_lines, folder):
    hpp_path = os.path.join(folder, ZEINFO_HPP)
    zeinfo_hpp = open(hpp_path, "a")

    zeinfo_hpp.write(ZEINFO_HPP_HEADER)

    cpp_path = os.path.join(folder, ZEINFOYAML_CPP)
    yaml_cpp = open(cpp_path, "a")
    yaml_cpp.write(YAML_CPP_HEADER)


    type_block, index, pounds = get_text_block(src_lines, "## Type", "\n", "-")
    valid_types = []
    src_lines = src_lines[index:]
    # check if type block has correct format
    for item in type_block:
        colon = item.find(":")
        if colon == -1:
            sys.exit("Incorrect format") #line number

        type_str = item[(colon + 1): len(item) - 1].strip() + ";\n"
        zeinfo_hpp.write(type_str)
        valid_types.append(item[:colon].strip()[2:])


    kernel_lines = []
    check_vectors = {} # maps plural struct form to singular struct form
    enum_lines = [] #PreDefinedAttrGetter enum classes
    static_lines = [] # PreDefinedAttrGetter static get
    top_layers_df = []
    while len(src_lines) > 0:
        text_block, index, pounds = get_text_block(src_lines, "#", "\n", "|")
        if index < len(src_lines):
            text_block.append(src_lines[index - 1])
        src_lines = src_lines[index:]
        if len(text_block) > 1:
            df, comment = generate_pandas_df(text_block)
            comm_list = comment.split(" ")
            struct_name = comm_list[1]
            if "<" not in comment: # indicates struct
                if len(comm_list) == 4:
                    check_vectors[comm_list[2]] = comm_list[1]
                else:
                    check_vectors[comm_list[1]] = ""
                pandas_create_zeinfoyaml_cpp(df, struct_name, valid_types, yaml_cpp)
                if struct_name == "Container":
                    container_df = [(df, "Container")]
                elif pounds == 1:
                    top_layers_df.append((df, struct_name))
                else:
                    pandas_create_zeinfo_hpp(df, struct_name, zeinfo_hpp)
            else:
                pandas_attr_getter(df, comment.split(" ")[2], enum_lines, static_lines)


    for pair in top_layers_df + container_df:
        df, struct_name = pair
        pandas_parse_top_layers(df, valid_types, zeinfo_hpp, struct_name, check_vectors)


    yaml_cpp.close()


    zeinfo_hpp.write("struct PreDefinedAttrGetter{\n")
    for item in enum_lines + static_lines:
        zeinfo_hpp.write(item)

    zeinfo_hpp.write("};\n") #PreDefinedAttrGetter
    zeinfo_hpp.write("}\n") # zebin
    zeinfo_hpp.write("#endif")

    zeinfo_hpp.close()

    return check_vectors


"""
Writes to ZEInfoYAML.hpp
"""
def create_yaml_hpp(check_vectors, folder):
    file = os.path.join(folder, ZEINFOYAML_HPP)
    output_file = open(file, "a")
    output_file.write(YAML_HPP_HEADER)

    for item in check_vectors.keys():
        singular = check_vectors[item]
        if singular != "":
            output_file.write("LLVM_YAML_IS_SEQUENCE_VECTOR(zebin::zeInfo" + singular + ")\n")

    output_file.write("namespace llvm {\n    namespace yaml{\n")

    #yaml_hpp_args.append(["Container", ""])
    for item in check_vectors.keys():
        singular = check_vectors[item]
        if singular != "":
            struct = "zeInfo" + singular
        else:
            struct = "zeInfo" + item
        output_file.write("        template<>\n        struct MappingTraits<zebin::" + struct + "> {\n")
        output_file.write("            static void mapping(IO& io, zebin::" + struct + "& info);\n        };\n")

    output_file.write("    }\n}\n#endif") # namespace yaml, namespace llvm, ZE_INFO_YAML_HPP
    output_file.close()


def main(argv):
    source_file = argv[0]
    dir_path = os.path.dirname(os.path.realpath(__file__))
    zeinfo_files = [ZEINFO_HPP, ZEINFOYAML_HPP, ZEINFOYAML_CPP]

    src_lines = []
    with open(source_file, "r") as input_file:
        for line in input_file:
            src_lines.append(line)
    public = os.path.join(dir_path, "Public")
    os.makedirs(public, exist_ok = True)
    folder = "Public"
    for file in zeinfo_files:
        folder = os.path.join(dir_path, folder)
        file = os.path.join(folder, file)

        output_file = open(file, "w")
        output_file.write(COPYRIGHT)
        output_file.close()

    check_vectors = create_zeinfo_hpp_yaml_cpp(src_lines, folder)
    create_yaml_hpp(check_vectors, folder)


if __name__ == "__main__":
    main(sys.argv[1:])
