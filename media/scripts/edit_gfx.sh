#! /bin/bash

# merge
convert pacman-l2.13x13.png pacman-r2.13x13.png pacman-u2.13x13.png pacman-d2.13x13.png +append pacman-l2r2u2d2.13x13.png 

# cut out single images to remove gap
fname=ghost-blinky4.14p2x14.png; for i in 0 1 2 3 ; do let x=${i}*14+${i}*2; convert $fname -crop 14x14+${x}+0 `basename $fname .png`-${i}.png ; done
fname=ghost-clyde4.14p2x14.png; for i in 0 1 2 3 ; do let x=${i}*14+${i}*2; convert $fname -crop 14x14+${x}+0 `basename $fname .png`-${i}.png ; done
fname=ghost-eyes4.14p2x14.png; for i in 0 1 2 3 ; do let x=${i}*14+${i}*2; convert $fname -crop 14x14+${x}+0 `basename $fname .png`-${i}.png ; done
fname=ghost-inky4.14p2x14.png; for i in 0 1 2 3 ; do let x=${i}*14+${i}*2; convert $fname -crop 14x14+${x}+0 `basename $fname .png`-${i}.png ; done
fname=ghost-pinky4.14p2x14.png; for i in 0 1 2 3 ; do let x=${i}*14+${i}*2; convert $fname -crop 14x14+${x}+0 `basename $fname .png`-${i}.png ; done

# merge single images to gap-less
fname=ghost-blinky4; fnames= ; for i in 0 1 2 3 ; do fnames="${fnames} ${fname}.14p2x14-${i}.png" ; done ; convert $fnames +append ${fname}.14x14.png 
fname=ghost-clyde4; fnames= ; for i in 0 1 2 3 ; do fnames="${fnames} ${fname}.14p2x14-${i}.png" ; done ; convert $fnames +append ${fname}.14x14.png 
fname=ghost-eyes4; fnames= ; for i in 0 1 2 3 ; do fnames="${fnames} ${fname}.14p2x14-${i}.png" ; done ; convert $fnames +append ${fname}.14x14.png 
fname=ghost-inky4; fnames= ; for i in 0 1 2 3 ; do fnames="${fnames} ${fname}.14p2x14-${i}.png" ; done ; convert $fnames +append ${fname}.14x14.png 
fname=ghost-pinky4; fnames= ; for i in 0 1 2 3 ; do fnames="${fnames} ${fname}.14p2x14-${i}.png" ; done ; convert $fnames +append ${fname}.14x14.png 

# merge them in one file
convert ghost-blinky4.14x14.png ghost-clyde4.14x14.png ghost-inky4.14x14.png ghost-pinky4.14x14.png ghost-eyes4.14x14.png -append ghosts-bcipe.5x14x14.png
