This c++ code distribution is in active development and intended for research purposes only. The code in src/cpp/TPG represents a TPG library which you can link to an application-specific main program. For more information contact stephen.kelly@dal.ca. The following provides a quick start for Linux. See code and scripts for further documentation. 

_This forked version of the code is a cleaned-up version for performance comparison with [Gegelati](https://github.com/gegelati/gegelati)._

## DEPENDENCIES (outside what is likely already installed)

```
sudo apt install scons g++ libbz2-dev
```

The Arcade Learning Environment (ALE) https://github.com/mgbellemare/Arcade-Learning-Environment

This project was last updated based on ALE v0.5.1. To fetch it, simply open the directory where this git project was cloned and type the following commands.

```
git submodule init
git submodule update
```

The ALE lib source code at version 0.5.1 will be cloned within the `ale_0.5.1` subdir.

To build the ALE:
```
cd ale_0.5.1
mkdir build && cd build
sudo apt-get install libsdl1.2-dev libsdl-gfx1.2-dev libsdl-image1.2-dev cmake
cmake -DUSE_SDL=ON -DUSE_RLGLUE=OFF -DBUILD_EXAMPLES=ON ..
make 
```

## SETTING UP THE ENVIRONEMNT

Navigate to the base folder for this distrobution and type:

    export TPGPATH=$(pwd)

The Sconscripts in each subdiretory of src/cpp should be modified to reference the actual location of the ALE

For linking with the ALE, add something like the following to your login script:

    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/asimonu/Bureau/Gegelati/skellcoTPG/ale_0.5.1


## COMPILING

    To compile the ALE and this program with a single command do the following :
    ./build.sh

    Otherwise you can compile this program manually :

    cd $TPGPATH
    scons --opt

## RUNNING A SINGLE-TASK EXPERIMENT

    cd runALE
    ./tpg-runner.sh ms_pacman 7


RUNNING A MULTI-TASK EXPERIMENT

    cd runALEMTRL
    ./tpg-runner.sh roms-3-02.txt 7
