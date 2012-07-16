#!/bin/bash

# This script can be used to decompress and merge one or more image files
# in to blobs of raw uncompressed frames used by ADTF.
#
# One frame:
# ./ imgs2rgbablob.sh wallpaper.jpg
#
# Multiple frames (animation)
# ./ imgs2rgbablob.sh frame01.jpg frame02.jpg frame03.jpg

[ $# -eq 0 ] && { echo "Usage: $0 file1 file2 fileN"; exit 1; }

wd="/tmp/$(basename $0)"
output=()
frame=0

rm -rf $wd
mkdir -p $wd

for f in "$@"
do
    fname=$(basename $f)
    base="${fname%.[^.]*}"

    out="$wd/$base.rgba"
    output[$frame]=$out
    echo "frame $frame: $f -> ${output[$frame]}"
    convert $f rgba:$out
    let "frame += 1"
done

cat ${output[*]} > rgbablob.raw

rm -rf $wd

