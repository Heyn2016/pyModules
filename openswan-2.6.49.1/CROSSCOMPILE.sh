#!/bin/sh
#
# cross compile example
#


#export PREFIX=/volquad/arm-4.0.2
export PREFIX=/home/he/share/IOT_BOX/02_SourceLibrary/ToolChain/gcc-linaro
export DESTDIR=/tmp/openswan.arm

export ARCH=arm
export CC=$PREFIX/bin/arm-linux-gnueabihf-gcc
export GCC=$PREFIX/bin/arm-linux-gnueabihf-gcc
export LD=$PREFIX/bin/arm-linux-gnueabihf-ld
export RANLIB=$PREFIX/bin/arm-linux-gnueabihf-ranlib
export AR=$PREFIX/bin/arm-linux-gnueabihf-ar
export AS=$PREFIX/bin/arm-linux-gnueabihf-as
export STRIP=$PREFIX/bin/arm-linux-gnueabihf-strip
export LD_LIBRARY_PATH=$PREFIX/lib/gcc/arm-linux-gnueabihf/5.2.1/
export PATH=$PATH:$PREFIX/bin
export USERCOMPILE="-Wl,-elf2flt -DCOMPILER_HAS_NO_PRINTF_LIKE -O3 -g ${PORTDEFINE} -I$PREFIX/arm-linux-gnueabihf/inc -L$PREFIX/lib/gcc-lib -DGCC_LINT -DLEAK_DETECTIVE -Dlinux -D__linux__"
export WERROR=' ' 

export CPATH=/home/he/share/02_SourceLibrary/gmp-6.1.2/gmp/include
export LIBRARY_PATH=/home/he/share/02_SourceLibrary/gmp-6.1.2/gmp/lib

#now you can run:
# make programs
#and binaries will appear in OBJ.linux.$ARCH/
#and run:
# make install
#and the install will go into $DESTDIR/

# note: the arm_tools I had failed to compile PRINTF_LIKE(x), so the code
# for that was  ifdef'ed with #ifndef COMPILER_HAS_NO_PRINTF_LIKE statements.
# Add -DCOMPILER_HAS_NO_PRINTF_LIKE to enable the workaround.

# EXECUTABLE FILE FORMAT
#
# Some uClibc/busybox combinations use different executable files formats from
# ELF. This is configured during Linux kernel build. One common format is
# the BLFT file format. Do not manually convert ELF binaries to BLTF using
# elf2flt as that will create invalid binaries. Instead add -Wl,-elf2flt to
# your flags (CFLAGS / LDFLAGS / USERCOMPILE)
