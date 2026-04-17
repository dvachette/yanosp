#!/bin/bash
set -e

TARGET=x86_64-elf
PREFIX=/usr/local/cross

BINUTILS_VERSION=2.42
GCC_VERSION=14.1.0

BINUTILS_URL="https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.gz"
GCC_URL="https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.gz"

JOBS=$(nproc)
BUILD_DIR=/tmp/cross-build

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "==> Downloading binutils ${BINUTILS_VERSION}..."
wget -c "$BINUTILS_URL"
tar -xzf binutils-${BINUTILS_VERSION}.tar.gz

echo "==> Downloading GCC ${GCC_VERSION}..."
wget -c "$GCC_URL"
tar -xzf gcc-${GCC_VERSION}.tar.gz

echo "==> Compiling binutils"
mkdir -p build-binutils && cd build-binutils
../binutils-${BINUTILS_VERSION}/configure \
    --target="$TARGET" \
    --prefix="$PREFIX" \
    --with-sysroot \
    --disable-nls \
    --disable-werror
make -j"$JOBS"
sudo make install
cd ..

echo "==> Compiling GCC"
mkdir -p build-gcc && cd build-gcc
../gcc-${GCC_VERSION}/configure \
    --target="$TARGET" \
    --prefix="$PREFIX" \
    --disable-nls \
    --enable-languages=c \
    --without-headers
make -j"$JOBS" all-gcc
make -j"$JOBS" all-target-libgcc
sudo make install-gcc
sudo make install-target-libgcc
cd ..

echo "==> Done."
echo "Add this to your .zshrc or .bashrc :"
echo 'export PATH="/usr/local/cross/bin:$PATH"'