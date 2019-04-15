#!/bin/bash
OLDPWD=$(pwd)
RELDIR=$(cd $(dirname $BASH_SOURCE) && pwd)
cd $RELDIR
rm -rf sdcc
svn co -r 9092 svn://svn.code.sf.net/p/sdcc/code/trunk/sdcc sdcc
sed -i 's/^\(MODELS = .*\)$/\1 huge/g' sdcc/device/lib/incl.mk
sed -i 's/^\(TARGETS\s*+=\s*models\s*\)small\(-.*\)$/\1model\2/g' sdcc/device/lib/Makefile.in
sed -i 's/-Werror//g' sdcc/support/sdbinutils/bfd/configure
sed -i 's/-Werror//g' sdcc/support/sdbinutils/bfd/warning.m4
sed -i 's/-Werror//g' sdcc/support/sdbinutils/binutils/configure
rm -rf sdcc-install
mkdir sdcc-install
cd sdcc-install
TARGETPREFIX=$(pwd)
cd ../sdcc
./configure --disable-gbz80-port --disable-z80-port --disable-ds390-port --disable-ds400-port --disable-pic14-port --disable-pic16-port --disable-hc08-port --disable-r2k-port --disable-z180-port --disable-sdcdb --disable-ucsim --prefix=$TARGETPREFIX
make
make install
cd $OLDPWD
