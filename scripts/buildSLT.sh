#!/bin/sh

#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set -e
# UBUNTU_VERSION   supported value [ 20, 22 ]                                                default 20
# LLVM_VERSION     supported value [ 10, 11, 12, 13, 14, 15]                                 default 11
# OWN_CMAKE_FLAGS  not suported but can be use as WA (each flag should be with -D prefix)    default empty
# example run:     UBUNTU_VERSION=ubuntu2004 LLVM_VERSION=11 sh /home/buildSLT.sh

echo "====================BUILD SPIRV-LLVM-Translator========================="
echo "[Build Status] build script started"
if [ -z ${UBUNTU_VERSION+x} ]; then
    echo "[Build Status] UBUNTU_VERSION is unset, use default 20";
    UBUNTU_VERSION="20.04"
else
    echo "[Build Status] UBUNTU_VERSION = ${UBUNTU_VERSION}"
fi
if [ -z ${LLVM_VERSION+x} ]; then
    echo "[Build Status] LLVM_VERSION is unset, use default 14";
    LLVM_VERSION="14"
else
    echo "[Build Status] LLVM_VERSION = ${LLVM_VERSION}"
fi

apt-get update
apt-get install -y flex bison libz-dev cmake curl wget build-essential git software-properties-common unzip lsb-release
echo "[Build Status] flex bison libz-dev cmake curl wget build-essential git software-properties-common INSTALLED"

if [ "$UBUNTU_VERSION" = "20.04" ]; then
    echo "[Build Status] Download new cmake version for Ubuntu 20.04";
    apt-get purge -y --auto-remove cmake
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
    apt-add-repository "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main"
    apt-get update
    apt-get install -y cmake
fi

if ([ "$UBUNTU_VERSION" = "20.04" ] && [ "$LLVM_VERSION" -ge 14 ]) || ([ "$UBUNTU_VERSION" = "22.04" ] && [ "$LLVM_VERSION" -ge 15 ])
then
    echo "[Build Status] Retrieve the LLVM archive signature for LLVM $LLVM_VERSION on Ubuntu $UBUNTU_VERSION";
    wget -q https://apt.llvm.org/llvm-snapshot.gpg.key
    apt-key add llvm-snapshot.gpg.key
    case "$UBUNTU_VERSION" in
        20.04) OS_HANDLE=focal;;
        22.04) OS_HANDLE=jammy;;
    esac
    add-apt-repository "deb http://apt.llvm.org/$OS_HANDLE/ llvm-toolchain-$OS_HANDLE-$LLVM_VERSION main"
fi

apt-get install -y llvm-"$LLVM_VERSION" llvm-"$LLVM_VERSION"-dev clang-"$LLVM_VERSION" liblld-"$LLVM_VERSION" liblld-"$LLVM_VERSION"-dev
echo "[Build Status] LLVM INSTALLED"

LLVM_VERSION_PREFERRED="$LLVM_VERSION".0.0
echo "[Build Status] LLVM_VERSION_PREFERRED = $LLVM_VERSION_PREFERRED"

echo "[Build Status] build and install SPIRV-LLVM-Translator"
/usr/bin/git clone --branch llvm_release_"$LLVM_VERSION"0 https://github.com/KhronosGroup/SPIRV-LLVM-Translator
cd SPIRV-LLVM-Translator
echo 'set(CPACK_GENERATOR "DEB")' >> CMakeLists.txt && echo 'set(CPACK_DEBIAN_PACKAGE_MAINTAINER "David Doria") # required' >> CMakeLists.txt && echo 'include(CPack)' >> CMakeLists.txt
mkdir build && cd build
cmake .. -DBASE_LLVM_VERSION="$LLVM_VERSION_PREFERRED"
make llvm-spirv -j`nproc`
cpack
echo "[Build Status] SPIRV-LLVM-Translator Packed"
