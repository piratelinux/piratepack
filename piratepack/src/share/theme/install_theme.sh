#!/bin/bash

set -e

curdir="$(pwd)"
cd ../..
maindir="$(pwd)"
cd
homedir="$(pwd)"
localdir="$homedir"/.piratepack/theme

if hash xfconf-query 2>/dev/null
then
    xfconf-query -n -c xfce4-desktop -p "/backdrop/screen0/monitor0/image-path" -t "string" -s "$curdir/background.jpg"
    curimage="$(xfconf-query -c xfce4-desktop -p /backdrop/screen0/monitor0/image-path -t string)"

    if [[ "$curimage" == "$curdir/background.jpg" ]]
    then
	echo "$curdir/background.jpg" >> "$localdir"/.installed
    fi
fi

cd

mkdir -p Pictures
cd Pictures
if [ ! -e pirate ]
then
    mkdir pirate
    cd "$maindir"/share/graphics/backgrounds
    while read -r line
    do
	ln -s "$maindir"/share/graphics/backgrounds/"$line" "$homedir"/Pictures/pirate/"$line"
    done < <(find * maxdepth 0 2>> /dev/null)
    echo "$homedir"/Pictures/pirate >> "$localdir"/.installed
fi
