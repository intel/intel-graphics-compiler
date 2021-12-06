#!/bin/sh

#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set -e
# BUILD_OS         supported value [ ubuntu1804, ubuntu2004 ]                                default ubuntu2004
# LLVM_VERSION     supported value [ 10, 11, 12 ]                                            default 11
# COMPILER         supported value [ gcc, clang ]                                            default gcc
# OWN_CMAKE_FLAGS  not suported but can be use as WA (each flag should be with -D prefix)    default empty
# example run:     BUILD_OS=ubuntu2004 LLVM_VERSION=11 COMPILER=gcc sh /home/buildIGC.sh

echo "====================BUILD IGC========================="
echo "[Build Status] build script started"
if [ -z ${BUILD_OS+x} ]; then
    echo "[Build Status] BUILD_OS is unset, use default ubuntu2004";
    BUILD_OS="ubuntu2004"
else
    echo "[Build Status] BUILD_OS = ${BUILD_OS}"
fi
if [ -z ${LLVM_VERSION+x} ]; then
    echo "[Build Status] LLVM_VERSION is unset, use default 11";
    LLVM_VERSION="11"
else
    echo "[Build Status] LLVM_VERSION = ${LLVM_VERSION}"
fi
if [ -z ${COMPILER+x} ]; then
    echo "[Build Status] COMPILER is unset, use default gcc";
    COMPILER="gcc"
else
    echo "[Build Status] COMPILER = ${COMPILER}"
fi
if [ -z ${OWN_CMAKE_FLAGS+x} ]; then
    echo "[Build Status] OWN_CMAKE_FLAGS is unset, use default EMPTY";
    OWN_CMAKE_FLAGS=""
else
    echo "[Build Status] OWN_CMAKE_FLAGS = ${OWN_CMAKE_FLAGS}"
fi


apt-get update
apt-get install -y flex bison libz-dev cmake curl wget build-essential git software-properties-common unzip
apt-get update
echo "[Build Status] flex bison libz-dev cmake curl wget build-essential git software-properties-common INSTALLED"

if [ "$BUILD_OS" = "ubuntu1804" ]; then
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | apt-key add -
    apt-add-repository "deb https://apt.kitware.com/ubuntu/ bionic main"
    apt update
    apt-get install -y cmake
    echo "[Build Status] new cmake on Ubuntu18.04 INSTALLED"
    wget --no-check-certificate -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -
    add-apt-repository "deb http://apt.llvm.org/bionic/   llvm-toolchain-bionic-$LLVM_VERSION  main"
    apt update
    echo '[Build Status] LLVM "$LLVM_VERSION" on Ubuntu18.04 PREPARED'
fi
apt-get install -y llvm-"$LLVM_VERSION" llvm-"$LLVM_VERSION"-dev clang-"$LLVM_VERSION" liblld-"$LLVM_VERSION" liblld-"$LLVM_VERSION"-dev
echo "[Build Status] LLVM INSTALLED"

if [ "$BUILD_OS" = "ubuntu1804" ] && [ "$LLVM_VERSION" = "11" ]; then
    LLVM_VERSION_PREFERRED="$LLVM_VERSION".1.0
else
    LLVM_VERSION_PREFERRED="$LLVM_VERSION".0.0
fi
echo "[Build Status] LLVM_VERSION_PREFERRED = $LLVM_VERSION_PREFERRED"

mkdir workspace
cd workspace

echo "[Build Status] build and install SPIRV-LLVM-Translator"
/usr/bin/git clone --branch llvm_release_"$LLVM_VERSION"0 https://github.com/KhronosGroup/SPIRV-LLVM-Translator
mkdir SPIRV-LLVM-Translator/build && cd SPIRV-LLVM-Translator/build
cmake .. -DBASE_LLVM_VERSION="$LLVM_VERSION_PREFERRED"
make llvm-spirv -j`nproc`
make install
cd ../..
echo "[Build Status] SPIRV-LLVM-Translator INSTALLED"

/usr/bin/git version
/usr/bin/git clone https://github.com/intel/intel-graphics-compiler ./igc
cd igc
echo "[Build Status] IGC commit hash below:"
/usr/bin/git log -1 --format='%H'
/usr/bin/git clone https://github.com/intel/vc-intrinsics ../vc-intrinsics
/usr/bin/git clone https://github.com/KhronosGroup/SPIRV-Headers.git ../SPIRV-Headers
/usr/bin/git clone https://github.com/KhronosGroup/SPIRV-Tools.git ../SPIRV-Tools
echo "[Build Status] All necessary repository CLONED"
mkdir build
cd build
curl -s https://api.github.com/repos/intel/intel-graphics-compiler/releases/latest | grep browser_download_url | egrep 'opencl_|core_' | cut -d '"' -f 4 | wget -qi -
dpkg -i *.deb
echo "[Build Status] Old IGC with opencl-clang downloaded and INSTALLED, WA to install opencl-clang"

CONFIG_VARS="-DIGC_OPTION__LLVM_MODE=Prebuilds -DIGC_OPTION__LLVM_PREFERRED_VERSION=$LLVM_VERSION_PREFERRED"
case $COMPILER in
    "clang")
        CONFIG_VARS="$CONFIG_VARS -DCMAKE_C_COMPILER=clang-$LLVM_VERSION -DCMAKE_CXX_COMPILER=clang++-$LLVM_VERSION"
        ;;
esac
CONFIG_VARS="$CONFIG_VARS $OWN_CMAKE_FLAGS"
echo "[Build Status] CONFIG_VARS = $CONFIG_VARS"

cmake ../ $CONFIG_VARS
echo "[Build Status] Cmake created"

make -j`nproc`
echo "[Build Status] make DONE"
echo "====================BUILD IGC========================="
