#!/bin/bash

set -e

verified="0"
file="$1"
targetname="$2"
targetdir="$targetname"_"pirate"

chmod u+r "$file"
mv "$file" original.pirate
cp original.pirate backup.pirate
chmod u-w backup.pirate
tar -xzf original.pirate

if [ -d "$targetdir" ]
then
    chmod u+rwx "$targetdir"
    cd "$targetdir"
    chmod -R u+r *
    if [ -f "$targetname.tar.gz" ] && [ -f "$targetname.tar.gz.asc" ] && [ ! -d "$targetname" ]
    then
	#todo gpg verify
	tar -xzf "$targetname.tar.gz"
	if [ -d "$targetname" ]
	then
	    verified="1";
	fi
    fi
fi

if [[ "$verified" == "1" ]]
then
    echo "out:verified"
else
    echo "err:notauthentic"
fi
