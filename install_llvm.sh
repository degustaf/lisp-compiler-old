#! /bin/bash

set -e

LLVM_VERSION=$1
LLVM_FILE=llvm-$LLVM_VERSION.src
LLVM_TAR=$LLVM_FILE.tar.xz

cd ..
# curl "http://releases.llvm.org/$LLVM_VERSION/$LLVM_TAR" -o "$LLVM_TAR"
curl -O "http://releases.llvm.org/$LLVM_VERSION/$LLVM_TAR"
tar -xJf "$LLVM_TAR"
rm "$LLVM_TAR"
cd "$LLVM_FILE"
mkdir build
cd build
cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=~/llvm ..
make -j install-llvm-config installhdrs install-LLVMSource install-LLVMSupport
