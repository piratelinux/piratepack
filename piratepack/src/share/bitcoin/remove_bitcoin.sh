#!/bin/bash

set -e

curdir="$(pwd)"

cd
homedir="$(pwd)"
localdir="$homedir/.piratepack/bitcoin"

if [ -f "$localdir"/.installed ]
then
    while read line
    do
        if [ -e "$line" ]
        then
            chmod -R u+rw "$line"
            rm -rf "$line"
        fi
    done <"$localdir"/.installed
fi
