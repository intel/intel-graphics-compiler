#!/bin/sh

#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set -e
# UBUNTU_VERSION   supported value [ 18, 20 ]                                                default 20
# LLVM_VERSION     supported value [ 10, 11, 12 ]                                            default 11
# COMPILER         supported value [ gcc, clang ]                                            default gcc
# OWN_CMAKE_FLAGS  not suported but can be use as WA (each flag should be with -D prefix)    default empty
# example run:     UBUNTU_VERSION=ubuntu2004 LLVM_VERSION=11 COMPILER=gcc sh /home/buildIGC.sh

echo "====================BUILD IGC========================="
echo "[Build Status] build script started"
if [ -z ${UBUNTU_VERSION+x} ]; then
    echo "[Build Status] UBUNTU_VERSION is unset, use default 20";
    UBUNTU_VERSION="20.04"
else
    echo "[Build Status] UBUNTU_VERSION = ${UBUNTU_VERSION}"
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
if [ -z ${IGC_SHA+x} ]; then
    echo "[Build Status] IGC_SHA is unset, use default master";
    OWN_CMAKE_FLAGS="master"
else
    echo "[Build Status] IGC_SHA = ${IGC_SHA}"
fi

apt-get update
apt-get install -y flex bison libz-dev cmake curl wget build-essential git software-properties-common unzip file
echo "[Build Status] flex bison libz-dev cmake curl wget build-essential git software-properties-common unzip file INSTALLED"

if [ "$UBUNTU_VERSION" = "18.04" ]; then
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

if [ "$UBUNTU_VERSION" = "18.04" ] && [ "$LLVM_VERSION" = "11" ]; then
    LLVM_VERSION_PREFERRED="$LLVM_VERSION".1.0
else
    LLVM_VERSION_PREFERRED="$LLVM_VERSION".0.0
fi
echo "[Build Status] LLVM_VERSION_PREFERRED = $LLVM_VERSION_PREFERRED"

echo "[Build Status] Prepare install OpenCL Clang"
dpkg -i ./igc-official-release/*.deb
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

CONFIG_VARS="-DIGC_OPTION__LLVM_MODE=Prebuilds -DIGC_OPTION__LLVM_PREFERRED_VERSION=$LLVM_VERSION_PREFERRED"
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
