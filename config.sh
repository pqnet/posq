SYSTEM_HEADER_PROJECTS="libc kernel"
PROJECTS="libc kernel"
 
export MAKE=${MAKE:-make}
export HOST=${HOST:-$(./default-host.sh)}

export TOOLCHAIN_ROOT=/home/pq/devel/clang/current

# export AR=${HOST}-ar
# export AS=${HOST}-as
# export CC=${HOST}-gcc
export AR="$TOOLCHAIN_ROOT/bin/llvm-ar"
export AS="$TOOLCHAIN_ROOT/bin/llvm-as --target=${HOST}"
export CC="$TOOLCHAIN_ROOT/bin/clang --target=${HOST}"
export CXX="$TOOLCHAIN_ROOT/bin/clang++ --target=${HOST}"
 
export PREFIX=$TOOLCHAIN_ROOT
export EXEC_PREFIX=$PREFIX
export BOOTDIR=/boot
export LIBDIR=$EXEC_PREFIX/lib
export INCLUDEDIR=$PREFIX/include
 
export CPPFLAGS=''
export CFLAGS='-O2 -g'
export CXXFLAGS="$CFLAGS"
export LDFLAGS=""
 
# Configure the cross-compiler to use the desired system root.

export SYSROOT="$(pwd)/sysroot"
export CC="$CC -D__ELF__ -D_LIBCPP_HAS_NO_THREADS --sysroot=$SYSROOT -I$INCLUDEDIR -I$INCLUDEDIR/c++/v1"
export CXX="$CXX -D__ELF__ -D_LIBCPP_HAS_NO_THREADS --sysroot=$SYSROOT -I$INCLUDEDIR -I$INCLUDEDIR/c++/v1"
# export SYSROOT="$TOOLCHAIN_ROOT"

# Work around that the -elf gcc targets doesn't have a system include directory
# because it was configured with --without-headers rather than --with-sysroot.
#if echo "$HOST" | grep -Eq -- '-elf($|-)'; then
#  export CC="$CC -isystem=$INCLUDEDIR"
#fi