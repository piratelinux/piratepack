#!/bin/bash

set -e

curdir="$(pwd)"

cd
homedir="$(pwd)"
localdir="$homedir/.piratepack/theme"

issue=$(cat /etc/issue)

if [ -f "$localdir"/.installed ]
then
    while read line
    do

	if [[ "$line" == "$homedir"/Pictures/pirate ]]
	then
	    rm -rf "$line"
	    continue
	fi

	if hash xfconf-query 2>/dev/null
	then
	    if [[ "$(xfconf-query -c xfce4-desktop -p /backdrop/screen0/monitor0/image-path -t string)" == "$line" ]]
            then
		xfconf-query -c xfce4-desktop -p "/backdrop/screen0/monitor0/image-path" -t "string" -s "/usr/share/backgrounds/xfce/xfce-blue.jpg"
            fi
	fi
    done <"$localdir"/.installed
fi

set +e
chmod -Rf u+rw "$localdir"/* "$localdir"/.[!.]* "$localdir"/...*
rm -rf "$localdir"/* "$localdir"/.[!.]* "$localdir"/...*
set -e
