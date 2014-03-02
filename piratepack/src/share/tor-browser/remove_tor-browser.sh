#!/bin/bash

set -e

cd
homedir="$(pwd)"
localdir="$homedir/.piratepack/tor-browser"

if [ -f "$localdir/.installed" ]
then
    while read line    
    do    
	if [ -e "$line" ]
	then 
	    chmod -R u+rw "$line"
	    rm -rf "$line"
	fi
    done <"$localdir/.installed" 
fi

set +e

chmod -Rf u+rw "$localdir"/* "$localdir"/.[!.]* "$localdir"/..[!.]* "$localdir"/...*
rm -rf "$localdir"/* "$localdir"/.[!.]* "$localdir"/..[!.]* "$localdir"/...*

rm -f .local/share/applications/tor-browser.desktop
rm -f .local/share/icons/tor-browser.png
rm -f .local/share/applications/tor-instance.desktop
rm -f .local/share/icons/tor-instance.png
rm -f .local/share/applications/tor-irc.desktop
rm -f .local/share/icons/tor-irc.png

set -e
