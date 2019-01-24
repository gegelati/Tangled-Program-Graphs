This c++ code distribution is in active development and intended for research purposes only. The code in src/cpp/TPG represents a TPG library which you can link to an application-specific main program. For more information contact stephen.kelly@dal.ca. The following provides a quick start for Linux. See code and scripts for further documentation. 

DEPENDENCIES (outside what is like already installed)

scons, g++, libbz2-dev

The Arcade Learning Environment (ALE) https://github.com/mgbellemare/Arcade-Learning-Environment

SETTING UP THE ENVIRONEMNT

Navigate to the base folder for this distrobution and type:

    export TPGPATH=$(pwd)

The Sconscripts in each subdiretory of src/cpp should be modified to reference the actual location of the ALE

For linking with the ALE, add something like the following to your login script:

    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/PATH/TO/ALE

COMPILING

    cd $TPGPATH
    scons --opt

RUNNING A SINGLE-TASK EXPERIMENT

    cd runALE
    ./tpg-runner.sh ms_pacman 7


RUNNING A MULTI-TASK EXPERIMENT

    cd runALEMTRL
    ./tpg-runner.sh roms-3-02.txt 7
