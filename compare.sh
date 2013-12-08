#!/bin/bash

#ARGS="-p513 -n10000"
#ARGS="-t2 -p1000 -n1000000"
ARGS="-t2 -f100 -p1000 -n1000000"

make proj3
echo "running ben..."
./ben -v $ARGS > ben_out.txt
echo "running proj3..."
./proj3 -v $ARGS > my_out.txt
echo "running diff..."
diff ben_out.txt my_out.txt > diff.txt
echo "running head..."
head diff.txt