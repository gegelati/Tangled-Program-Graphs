This c++ code distribution is in active development and intended for research purposes only. 
For more information contact stephen.kelly@dal.ca

The following provides a quick start for Linux. See code and scripts for further documentation. 

DEPENDENCIES (outside what is like already installed)

scons
g++
libbz2-dev

SETTING UP THE ENVIRONEMNT

Navigate to the base folder for this distrobution and type:

export TPGPATH=$(pwd)

COMPILING

cd $TPGPATH
scons --opt

RUNNING AN EXPERIMENT

cd runALE
./tpg-runner.sh ms_pacman 7

