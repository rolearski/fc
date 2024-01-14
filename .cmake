#!/bin/bash 

#make clean
rm -f CMakeCache.txt
find . -name CMakeFiles | xargs rm -Rf

# -DVMIME_HAVE_TLS_SUPPORT=OFF
# -DLIBRESSL_VERSION_NUMBER=3

#cmake -DBOOST_ROOT=$HOME/opt/boost_1_70_0 -Wno-dev -Wno-misleading-indentation \
##cmake -DBOOST_ROOT=$HOME/opt/boost_1_74_0 -Wno-dev -DCMAKE_BUILD_TYPE=Debug .
#cmake -DCMAKE_BUILD_TYPE=Debug -Wno-dev .
#cmake -DCPP_STANDARD=-std=c++2b -Wno-dev -DCMAKE_BUILD_TYPE=Debug  .
#cmake -DCPP_STANDARD=-std=c++20 -Wno-dev  -DCMAKE_BUILD_TYPE=Debug  .
#cmake -DCPP_STANDARD=-std=c++20 -Wno-dev -DCMAKE_BUILD_TYPE=Debug  .
#cmake -DCPP_STANDARD=-std=c++17 -Wno-dev -DCMAKE_BUILD_TYPE=Debug  .

cmake -Wno-dev -DSKIP_TESTS=1 -DCMAKE_BUILD_TYPE=Debug  .


