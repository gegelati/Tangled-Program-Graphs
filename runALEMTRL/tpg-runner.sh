#!/bin/bash

#start from scratch
rom=$1
seed=$2
echo "Starting run $seed..."
./genScript.pl sbb 1 $seed
../build/release/cpp/tpgALEMultiTask2/tpgALEMultiTask2 -T 1000000 -S 1 -s $seed -O 5 -r $rom 1> tpg.$rom.$seed.std 2> tpg.$rom.$seed.err &

#replay elite Multi-Task
#rom=$1
#dirchkp=$2
#dirstd=$3
#seeds=$(ls $dirchkp | cut -d '.' -f 4 | sort -n | uniq)
#for seed in $seeds; do
#   t=$(ls $dirchkp/cp.*.-1.$seed.0.rslt.tgz | awk -F"cp." '{print $2}' | cut -d '.' -f 1 | sort -n  | tail -n 1)
#   tar xzvf $dirchkp/cp.$t.-1.$seed.0.rslt.tgz -C checkpoints
#   team=$(grep "TPG::setEliteTeamsMT t $t " $dirstd/tpg.$rom.$seed.std | awk -F" id " '{print $2}' | awk '{print $1}')
#   echo "Starting run $seed $team $t ..."
#   ./genScript.pl sbb 1 $seed
#   ../build/release/cpp/tpgALEMultiTask2/tpgALEMultiTask2 -S 10 -R $team -f -1 -C 0 -t $t -T 1000000 -s $seed -O 5 -r $rom  1> tpg.$rom.$seed.std 2> tpg.$rom.$seed.err &
#done

##pickup
#rom=$1
#dirChkp=$2
#seeds=$(ls $dirChkp | grep tgz | cut -d '.' -f 4 | sort -n | uniq)
#for seed in $seeds; do
#   t=$(ls $dirChkp/cp.*.-1.$seed.0.rslt.tgz | cut -d '.' -f 2 | sort -n | tail -n 1)
#   #t=$(ls $dirChkp/cp.*.-1.$seed.0.rslt | awk -F"cp." '{print $2}' | cut -d '.' -f 1 | sort -n  | tail -n 1)
#   tar xzvf $dirChkp/cp.$t.-1.$seed.0.rslt.tgz -C checkpoints
#   #cp $dirChkp/cp.$t.-1.$seed.0.rslt checkpoints
#   echo "Starting run $seed $team $t ..."
#   ./genScript.pl sbb 1 $seed
#   ../build/release/cpp/tpgALEMultiTask2/tpgALEMultiTask2 -S 10 -C 0 -t $t -T 1000000 -s $seed -O 5 -r $rom  1>> tpg.$rom.$seed.std 2>> tpg.$rom.$seed.err &
#done

