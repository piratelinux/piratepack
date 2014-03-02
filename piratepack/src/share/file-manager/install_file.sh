#!/bin/bash

set -e

curdir="$(pwd)"
targetname="$1"
tmpdir="$2"

cd "$tmpdir"

targetdir="$targetname"_"pirate"
if [ -d "$targetdir" ]
then
    cd "$targetdir"
    chmod -R u+r *
    if [ -d "$targetname" ]
    then
	cp "$targetname".tar.gz ../../"$targetname"
	cp "$targetname".tar.gz.asc ../../"$targetname"
	cd ../..
	if [ ! -d backup ]
	then
	    mkdir backup
	    chmod u+rwx backup
	fi
	numbackup="$(ls backup/$targetname_*.pirate 2>> /dev/null | wc -l)"
	cp tmp/backup.pirate backup/"$targetname"_"$(($numbackup + 1)).pirate"
	cd "$curdir"
	cd ../"$targetname"
	"./install"_"$targetname"_"file.sh"
    fi
fi
