#!/bin/bash

set -e

curdir="$(pwd)"
maindir="$1"
maindir_fin="$2"

if [ -d "$maindir"/bin ] && [ ! -e "$maindir"/bin/polipo ]
then
    polipo_bin="$(which polipo)"
    ln -s "$polipo_bin" "$maindir"/bin/polipo
fi

if [ -d "$maindir"/bin ] && [ ! -e "$maindir"/bin/tor ]
then
    tor_bin="$(which tor)"
    ln -s "$tor_bin" "$maindir"/bin/tor
fi

if [ -d "$maindir"/bin ] && [ ! -e "$maindir"/bin/vidalia ]
then
    vidalia_bin="$(which vidalia)"
    ln -s "$vidalia_bin" "$maindir"/bin/vidalia
fi

echo "socksParentProxy = localhost:9050" > .polipo_tor
echo "diskCacheRoot=\"\"" >> .polipo_tor
echo "disableLocalInterface=true" >> .polipo_tor

cp ".polipo_tor" "$maindir/share/tor-browser/"

echo "TorExecutable=""$maindir_fin"/bin/tor >> .vidalia/vidalia.conf

cp -r ".vidalia" "$maindir/share/tor-browser/"

awk '{sub(/[$]maindir/,"'"$maindir_fin"'"); print}' tor-instance > tor-instance_tmp
mv tor-instance_tmp tor-instance

awk '{sub(/[$]maindir/,"'"$maindir_fin"'"); print}' tor-browser > tor-browser_tmp
mv tor-browser_tmp tor-browser

awk '{sub(/[$]maindir/,"'"$maindir_fin"'"); print}' tor-irc > tor-irc_tmp
mv tor-irc_tmp tor-irc

issue="$(cat /etc/issue)"

if [[ "$issue" == *"Ubuntu"* ]] || [[ "$issue" == *"Debian"* ]]
then
    if [[ "$issue" != *"Ubuntu"*"11.10"* ]] && [[ "$issue" != *"Ubuntu"*"12."* ]]
    then
	awk '{sub(/purple[.]tar[.]gz/,"'"purple_old.tar.gz"'"); print}' tor-irc > tor-irc_tmp
	mv tor-irc_tmp tor-irc
    fi
fi

mkdir "$maindir"/share/tor-browser_build

cp tor-browser  "$maindir"/share/tor-browser_build/
chmod a+x "$maindir"/share/tor-browser_build/tor-browser

cp tor-instance  "$maindir"/share/tor-browser_build/
chmod a+x "$maindir"/share/tor-browser_build/tor-instance

cp tor-irc  "$maindir"/share/tor-browser_build/
chmod a+x "$maindir"/share/tor-browser_build/tor-irc

if [ -d "$maindir"/bin ] && [ ! -e "$maindir"/bin/tor-browser ]
then
    ln -s "$maindir"/share/tor-browser_build/tor-browser "$maindir"/bin/tor-browser
fi

if [ -d "$maindir"/bin ] && [ ! -e "$maindir"/bin/tor-instance ]
then
    ln -s "$maindir"/share/tor-browser_build/tor-instance "$maindir"/bin/tor-instance
fi

if [ -d "$maindir"/bin ] && [ ! -e "$maindir"/bin/tor-irc ]
then
    ln -s "$maindir"/share/tor-browser_build/tor-irc "$maindir"/bin/tor-irc
fi

mkdir jondo
mv jondofox_linux_bsd_en-US.tar.bz2 jondo/
cd jondo
tar -xjf jondofox_linux_bsd_en-US.tar.bz2
cd jondofox_linux_bsd
tar -czf profile.tar.gz profile
cp profile.tar.gz "$maindir"/share/tor-browser/

cd "$curdir"

echo "Exec=$maindir_fin/bin/tor-browser" >> tor-browser.desktop
cp tor-browser.desktop "$maindir/share/tor-browser/"
cp tor-browser.png "$maindir/share/tor-browser/"

echo "Exec=$maindir_fin/bin/tor-instance" >> tor-instance.desktop
cp tor-instance.desktop "$maindir/share/tor-browser/"
cp tor-instance.png "$maindir/share/tor-browser/"

echo "Exec=$maindir_fin/bin/tor-irc" >> tor-irc.desktop
cp tor-irc.desktop "$maindir/share/tor-browser/"
cp tor-irc.png "$maindir/share/tor-browser/"
