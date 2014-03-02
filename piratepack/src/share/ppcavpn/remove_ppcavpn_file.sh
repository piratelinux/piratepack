#!/bin/bash

set -e

curdir="$(pwd)"
cd
homedir="$(pwd)"
localdir="$homedir/.piratepack/ppcavpn"
cd "$localdir"

if [ -f ppcavpn/.installed ]
then
    while read line    
    do    
	gconftool-2 --recursive-unset "$line"
    done <ppcavpn/.installed     
fi

chmod -R u+rw ppcavpn
rm -rf ppcavpn
