#!/bin/bash

curdir="$(pwd)"
cd ../..
maindir="$(pwd)"
cd
homedir="$(pwd)"
localdir="$homedir"/.piratepack/bitcoin

if [ -d "$maindir/share/cwallet_build" ]
then
    if [ ! -d .local ]
    then mkdir .local
    fi
    cd .local
    if [ ! -d share ]
    then mkdir share
    fi
    cd share
    if [ ! -d icons ]
    then mkdir icons
    fi
    cp "$maindir/share/cwallet_build/icon.png" icons
    if [ ! -d applications ]
    then mkdir applications
    fi
    cp "$curdir"/cwallet.desktop "$localdir"
    echo "Exec=$maindir/bin/cwallet-gui --addpath $maindir/bin" >> "$localdir"/cwallet.desktop
    mv "$localdir/cwallet.desktop" applications
fi
