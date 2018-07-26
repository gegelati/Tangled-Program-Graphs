#!/bin/bash

#start from scratch
rom=$1
seed=$2
echo "Starting run $seed..."
./genScript.pl sbb 1 $seed
../build/release/cpp/tpgALE/tpgALE -T 1000000 -s $seed -f 0 -r $rom 1> tpg.$rom.$seed.std 2> tpg.$rom.$seed.err &

