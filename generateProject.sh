ehco "cmake is looking for g++12. My linux distro only came with 10 or 11 by default," 
echo " so make sure to install g++12 to use c++23 features." 
cmake -S src -B build -DCMAKE_CXX_COMPILER=/usr/bin/g++-12
