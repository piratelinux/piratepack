#!/bin/bash

set -e

curdir="$(pwd)"
cd
homedir="$(pwd)"
localdir="$homedir/.piratepack/ppcavpn"
cd "$localdir"


if [ -d ppcavpn  ]
then
    cd "$curdir"
    ./remove_ppcavpn_file.sh
fi

cd "$localdir"

chmod u+r ppcavpn.tar.gz
tar -xzf ppcavpn.tar.gz
cd ppcavpn

keyfile="$(find *.key)"
keyfilelen=${#keyfile}
keyfilelensub=$(($keyfilelen - 4))
clientid="${keyfile:0:$keyfilelensub}"

num="1"

while true
do

dirs="$(gconftool --all-dirs /system/networking/connections)"

if [[ "$dirs" == *"$num"* ]]
then
    let num=num+1
    continue
fi

sourcedir="$(pwd)"
targetdir="/system/networking/connections/$num"

gconftool-2 --type string --set "$targetdir"/connection/id "PPCA VPN"
gconftool-2 --type string --set "$targetdir"/connection/name "connection"
gconftool-2 --type string --set "$targetdir"/connection/type "vpn"
gconftool-2 --type string --set "$targetdir"/connection/uuid "$(uuidgen)"

gconftool-2 --type list --list-type int --set "$targetdir"/ipv4/addresses "[]"
gconftool-2 --type list --list-type int --set "$targetdir"/ipv4/addresses "[]"
gconftool-2 --type string --set "$targetdir"/ipv4/method "auto"
gconftool-2 --type string --set "$targetdir"/ipv4/name "ipv4"
gconftool-2 --type list --list-type int --set "$targetdir"/ipv4/routes "[]"

gconftool-2 --type string --set "$targetdir"/vpn/ca "$sourcedir/ca.crt"
gconftool-2 --type string --set "$targetdir"/vpn/cert "$sourcedir/$clientid.crt"
gconftool-2 --type string --set "$targetdir"/vpn/comp-lzo "yes"
gconftool-2 --type string --set "$targetdir"/vpn/connection-type "tls"
gconftool-2 --type string --set "$targetdir"/vpn/key "$sourcedir/$clientid.key"
gconftool-2 --type string --set "$targetdir"/vpn/port "443"
gconftool-2 --type string --set "$targetdir"/vpn/proto-tcp "yes"
gconftool-2 --type string --set "$targetdir"/vpn/remote "vpn.ostra.ca"
gconftool-2 --type string --set "$targetdir"/vpn/service-type "org.freedesktop.NetworkManager.openvpn"

echo "$targetdir" >> .installed

break

done

cd ..
chmod -R a-w ppcavpn
