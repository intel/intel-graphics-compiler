#!/bin/bash
set -e 
set -o igncr 

function fatal()
{
	echo "ERROR: $1"
	exit 1
}

if [ ! -f GED.zip ]; then
   fatal "could not find ./GED.zip"
fi

echo "Removing old GED"
rm -rf GED_external GED_internal

echo "Unzipping GED.zip"
../bin/7za x -p12345678 GED.zip > /dev/null

echo "Renaming Directories"
mv GED-internal GED_internal
mv GED-external GED_external
cat GED_internal/Source/common/version.cpp

echo "Removing non-essential files"
rm -rf GED_internal/Config
rm -rf GED_external/Config

# $1 = is dir: e.g. GED/build/autogen-ia32
# $2 = the GED group ("autogenia32"  =>  used to make "set(GED_autogenia32_cpp..."
function generate_cmake_files()
{
	echo "Creating $1/CMakeLists.txt"
	pushd $1 > /dev/nullq
	# don't need the make file
	rm -f $1/makefile

	echo "# $1" > CMakeLists.txt
	echo "set(GED_$2_cpp" >>CMakeLists.txt
	# create the .cpp files list
	find . -name "*.cpp" | sed 's/\./  ${CMAKE_CURRENT_SOURCE_DIR}/'>>CMakeLists.txt
	echo "  PARENT_SCOPE" >>CMakeLists.txt
	echo ")" >>CMakeLists.txt

	echo "# $1" >> CMakeLists.txt
	echo "set(GED_$2_h" >>CMakeLists.txt
	# create the .h files list
	find . -name "*.h" | sed 's/\./  ${CMAKE_CURRENT_SOURCE_DIR}/'>>CMakeLists.txt
	echo "  PARENT_SCOPE" >>CMakeLists.txt
	echo ")" >>CMakeLists.txt

	popd > /dev/null
}

generate_cmake_files "GED_internal/build/autogen-ia32" "autogenia32"
generate_cmake_files "GED_internal/build/autogen-intel64" "autogenintel64"
generate_cmake_files "GED_internal/Source/common" "common"
generate_cmake_files "GED_internal/Source/ged/xcoder" "xcoder"

generate_cmake_files "GED_external/build/autogen-ia32" "autogenia32"
generate_cmake_files "GED_external/build/autogen-intel64" "autogenintel64"
generate_cmake_files "GED_external/Source/common" "common"
generate_cmake_files "GED_external/Source/ged/xcoder" "xcoder"

cp "RootCmakeLists.txt" "GED_internal/CMakeLists.txt"
cp "RootCmakeLists.txt" "GED_external/CMakeLists.txt"
