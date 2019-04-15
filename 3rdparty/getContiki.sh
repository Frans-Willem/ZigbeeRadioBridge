#!/bin/bash
RELDIR=$(cd $(dirname $BASH_SOURCE) && pwd)
git clone https://github.com/contiki-os/contiki.git $RELDIR/contiki
