#!/bin/bash

#start from scratch
rom=$1
seed=$2
echo "Starting run $seed..."
./genScript.pl sbb 1 $seed
../build/release/cpp/tpgALEMTRL/tpgALEMTRL -T 1000000 -S 1 -s $seed -O 5 -r $rom 1> tpg.$rom.$seed.std 2> tpg.$rom.$seed.err &

