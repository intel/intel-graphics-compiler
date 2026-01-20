#!/bin/sh

#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2022-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set -e
# UBUNTU_VERSION   supported value [ 22.04, 24.04 ]                                          default 24.04
# LLVM_VERSION     supported value [ 14, 15, 16 ]                                            default 16
# COMPILER         supported value [ gcc, clang ]                                            default gcc
# OWN_CMAKE_FLAGS  not suported but can be use as WA (each flag should be with -D prefix)    default empty
# example run:     UBUNTU_VERSION=24.04 LLVM_VERSION=16 COMPILER=gcc sh /home/buildIGC.sh

echo "====================BUILD IGC========================="
echo "[Build Status] build script started"
if [ -z ${UBUNTU_VERSION+x} ]; then
    echo "[Build Status] UBUNTU_VERSION is unset, use default 24.04";
    UBUNTU_VERSION="24.04"
else
    echo "[Build Status] UBUNTU_VERSION = ${UBUNTU_VERSION}"
fi
if [ -z ${LLVM_VERSION+x} ]; then
    echo "[Build Status] LLVM_VERSION is unset, use default 16";
    LLVM_VERSION="16"
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
if [ -z ${IGC_SHA+x} ]; then
    echo "[Build Status] IGC_SHA is unset, use default master";
    IGC_SHA="master"
else
    echo "[Build Status] IGC_SHA = ${IGC_SHA}"
fi

apt-get update
apt-get install -y flex bison libz-dev cmake curl wget build-essential git software-properties-common unzip file lsb-release python3-mako libc6 libstdc++6 libzstd-dev
echo "[Build Status] flex bison libz-dev cmake curl wget build-essential git software-properties-common unzip file INSTALLED"

if ([ "$UBUNTU_VERSION" = "22.04" ] && [ "$LLVM_VERSION" -ge 16 ])
then
    echo "[Build Status] Retrieve the LLVM archive signature for LLVM $LLVM_VERSION on Ubuntu $UBUNTU_VERSION";
    wget -q https://apt.llvm.org/llvm-snapshot.gpg.key
    apt-key add llvm-snapshot.gpg.key
    add-apt-repository "deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy-$LLVM_VERSION main"
fi

apt-get install -y llvm-"$LLVM_VERSION" llvm-"$LLVM_VERSION"-dev clang-"$LLVM_VERSION" liblld-"$LLVM_VERSION" liblld-"$LLVM_VERSION"-dev
echo "[Build Status] LLVM INSTALLED"

LLVM_VERSION_PREFERRED="$LLVM_VERSION".0.0
echo "[Build Status] LLVM_VERSION_PREFERRED = $LLVM_VERSION_PREFERRED"

echo "[Build Status] Prepare install OpenCL Clang"
dpkg -i ./igc-official-release/*.deb
if [ -f "/usr/local/lib/libopencl-clang2.so.$LLVM_VERSION" ] && [ ! -f "/usr/local/lib/libopencl-clang.so" ]; then
    # Symlink to a library name CMake is set up to handle until either
    # CMake is updated or the library name is changed back.
    ln -s /usr/local/lib/libopencl-clang2.so.$LLVM_VERSION /usr/local/lib/libopencl-clang.so
fi
echo "[Build Status] OpenCL Clang INSTALLED"


echo "[Build Status] Install SPIRV-LLVM-Translator"
dpkg -i ./build-SPIRV-LLVM-Translator/*.deb
mv /usr/lib/libLLVMSPIRVLib.a /usr/local/lib/libLLVMSPIRVLib.a # WA Cpack wrongly pack deb file
echo "[Build Status] SPIRV-LLVM-Translator INSTALLED"

/usr/bin/git version
/usr/bin/git config --global --add safe.directory /workspace/igc
cd workspace/igc
echo "[Build Status] IGC commit hash below:"
echo "[Build Status] All necessary repository Ready"

# Set LLVM build options
CONFIG_VARS="-DIGC_OPTION__LLVM_MODE=Prebuilds -DIGC_OPTION__LLVM_PREFERRED_VERSION=$LLVM_VERSION_PREFERRED"

# Disable BF16 support for build with unpatched, upstream clang
CONFIG_VARS="${CONFIG_VARS} -DIGC_OPTION__ENABLE_BF16_BIF=OFF"

case $COMPILER in
    "clang")
        CONFIG_VARS="$CONFIG_VARS -DCMAKE_C_COMPILER=clang-$LLVM_VERSION -DCMAKE_CXX_COMPILER=clang++-$LLVM_VERSION"
        ;;
esac
CONFIG_VARS="$CONFIG_VARS $OWN_CMAKE_FLAGS"
echo "[Build Status] CONFIG_VARS = $CONFIG_VARS"

mkdir build && cd build
cmake ../ $CONFIG_VARS
echo "[Build Status] Cmake created"

make -j`nproc`
echo "[Build Status] make DONE"
cpack
mkdir DEB-FILES && mv ./*.deb ./DEB-FILES
if [ -f ./*.ddeb ]; then
    mkdir DEB-FILES/dbgsym
    mv ./*.ddeb ./DEB-FILES/dbgsym
fi
echo "[Build Status] cpack DONE"
echo "====================BUILD IGC========================="
