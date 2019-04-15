#!/bin/bash
RELDIR=$(cd $(dirname $BASH_SOURCE) && pwd)
rm -rf "$RELDIR/pythonfixup"
mkdir "$RELDIR/pythonfixup"
ln -s $(which python2) "$RELDIR/pythonfixup/python"
export PATH=$RELDIR/pythonfixup:$RELDIR/sdcc-install/bin:$PATH

