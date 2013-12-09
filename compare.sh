#!/bin/bash

#ARGS="-p513 -n10000"
#ARGS="-t2 -p1000 -n1000000"
#ARGS="-t2 -f100 -p1000 -n1000000"
#ARGS="-n10000000 -t1"
#ARGS="-t1 -p100 -f10000 -n1000000"
ARGS=""

V="-v"

make proj3
echo "running ben..."
./ben $V $ARGS > ben_out.txt
echo "running proj3..."
./proj3 $V $ARGS > my_out.txt
echo "running diff..."
diff ben_out.txt my_out.txt > diff.txt
echo "running head..."
head diff.txt