#!/bin/sh

iconslimit=63 # max number of icons on screenshot

die() { echo "$@"; exit 1; }

case "$1" in *.jisp) iconset="$1"; ;; *) die "First argument has to be jisp file"; ;; esac;

dir=$(mktemp -d);
unzip "$iconset" -d $dir &>/dev/null

imgdir="${dir}/$(basename "$iconset" .jisp)"

# xmlstarlet
IFS=$'\n'
icons=($(xml sel -t -v "//icondef/icon/object/text()" $imgdir/icondef.xml | sed 's|gif$|gif[0]|' | head -n $iconslimit))
unset IFS

# imagemagick
cd $imgdir; magick montage -background '#ffffff00' -geometry +1+1 "${icons[@]}" $dir/screenshot.png

rm -rf $imgdir
echo $dir/screenshot.png
