#!/bin/bash

for i in orig/*.wav ; do 
    bname=`basename $i .wav`
    dname=`dirname $i`

    sox -v 0.25 ${dname}/${bname}.wav -c 1 -r 44100 ${bname}2.wav
    lame ${bname}2.wav ${bname}.mp3
    rm -f ${bname}2.wav
done
