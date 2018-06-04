#!/bin/bash

#start from scratch
rom=$1
seed=$2
echo "Starting run $seed..."
./genScript.pl sbb 1 $seed
../build/release/cpp/tpgALE/tpgALE -T 1000000 -s $seed -f 0 -r $rom 1> tpg.$rom.$seed.std 2> tpg.$rom.$seed.err &

##replay training champion
#dirChkp=$2
#dirStd=$1
#seed=$3
#viz=$4
#rom=$(ls $dirStd | grep "\.$seed.std" | cut -d '.' -f 2 | tr -d '\n')
#t=$(ls $dirChkp | grep "\.$seed.0.rslt.tgz" | cut -d '.' -f 2 | sort -n | tail -n 1)
#tar xzvf $dirChkp/cp.$t.-1.$seed.0.rslt.tgz -C checkpoints
#team=$(grep "tminfo t $t " $dirStd/tpg.$rom.$seed.std | awk -F" id " '{print $2}' | awk '{print $1}')
#score=$(grep "tminfo t $t " $dirStd/tpg.$rom.$seed.std | awk -F" meanOutcomes " '{print $2}' | awk '{print $1}')
#echo "Replay seed $seed tm $team t $t score $score ..."
#./genScript.pl sbb 1 $seed
#
#if [ $viz -gt 0 ]; then 
#   ../build/release/cpp/tpgALE/tpgALE -V -R $team -C 0 -t $t -s $seed -O 5 -f 0 -r $rom 1> tpg.$rom.$seed.$team.std 2> tpg.$rom.$seed.$team.err &
#else
#   ../build/release/cpp/tpgALE/tpgALE -R $team -C 0 -t $t -s $seed -O 5 -f 0 -r $rom 1> tpg.$rom.$seed.$team.std 2> tpg.$rom.$seed.$team.err &
#fi

##pickup multiple runs for a single rom
#rom=$1
#seeds=$(ls checkpoints/cp.*.tgz | cut -d '.' -f 4 | sort -n | uniq)
#for seed in $seeds; do
#   #./genScript.pl sbb 1 $seed
#   t=$(ls checkpoints/cp.*.-1.$seed.0.rslt.tgz | cut -d '.' -f 2 | sort -n | tail -n 1)
#   tar xzvf checkpoints/cp.$t.-1.$seed.0.rslt.tgz -C checkpoints
#   echo "Starting run seed $seed t $t rom $rom"
#   ../build/release/cpp/tpgALE/tpgALE -C 0 -t $t -T 1000000 -s $seed -O 5 -f 0 -r $rom  1>> tpg.$rom.$seed.p.std 2>> tpg.$rom.$seed.p.err &
#done
