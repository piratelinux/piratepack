#!/bin/bash

set -e

curdir="$(pwd)"
cd ../..
maindir="$(pwd)"
cd ..
basedir="$(pwd)"
cd
homedir="$(pwd)"
localdir="$homedir"/.piratepack/theme

if hash xfconf-query 2>/dev/null
then
    xfconf-query -n -c xfce4-desktop -p "/backdrop/screen0/monitor0/image-path" -t "string" -s "$basedir/share/theme/background.jpg"
    curimage="$(xfconf-query -c xfce4-desktop -p /backdrop/screen0/monitor0/image-path -t string)"

    if [[ "$curimage" == "$basedir/background.jpg" ]]
    then
	echo "$basedir/background.jpg" >> "$localdir"/.installed
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
	ln -s "$basedir"/share/graphics/backgrounds/"$line" "$homedir"/Pictures/pirate/"$line"
    done < <(find * maxdepth 0 2>> /dev/null)
    echo "$homedir"/Pictures/pirate >> "$localdir"/.installed
fi
