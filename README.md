ProtoT - Fun 3D programming prototyping environment
Copyright (c) 2016 - Martin Felis <martin@fysx.org

mkdir build/
cd build/
cmake ../ -DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF
make -j4
cd src/modules/
cp ../../../src/modules/autorebuild.sh .
./autorebuild.sh

press ctrl-z

bg

cd -

./protot

Edit code in ../src/modules/

Enjoy!
