This code distribution is in active development and intended for research purposes only. For more information contact stephen.kelly@dal.ca


DEPENDENCIES

scons
g++
libbz2-dev

sudo apt-get -y install scons g++ libbz2-dev 

#ALE
sudo apt-get install libsdl1.2-dev libsdl-gfx1.2-dev libsdl-image1.2-dev cmake
$ mkdir build && cd build
$ cmake -DUSE_SDL=ON -DUSE_RLGLUE=OFF -DBUILD_EXAMPLES=ON ..
$ make -j 4
