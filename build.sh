cd ale_0.5.1
mkdir build
cd build
sudo apt-get install libsdl1.2-dev libsdl-gfx1.2-dev libsdl-image1.2-dev cmake
cmake -DUSE_SDL=ON -DUSE_RLGLUE=OFF -DBUILD_EXAMPLES=ON ..
make 
cd ../..

export TPGPATH=$(pwd)
rm -R build_release
scons
