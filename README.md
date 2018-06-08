# cmake-sdl2-template
Minimal C++ with SDL2 project using CMake build process management.

## Requirements
CMake is the tool used here in order to manage build processes, so it must be available beforehand.
In Debian-based distributions, CMake is available through the package manager.
However, it is very likely that your OS repositories are not updated with one of CMake's latest version.

So, I would recommend to build it and install it manually if you want the latest stable version.
Otherwise you can just use the package manager for an easier install procedure.

### Ubuntu - Build and install CMake manually (recommended)
Uninstall any previous version (if needed):
```
sudo apt remove cmake
sudo apt purge --auto-remove cmake
```

Download, build and install the desired version, from [the official CMake webpage](https://cmake.org/download/). 
You should set the CMAKE_VERSION and CMAKE_BUILD variables as needed:
```
export CMAKE_VERSION=3.11
export CMAKE_BUILD=3
mkdir -p ~/3rdparty
cd ~/3rdparty
wget https://cmake.org/files/v$CMAKE_VERSION/cmake-$CMAKE_VERSION.$CMAKE_BUILD.tar.gz
tar -xvzf cmake-$CMAKE_VERSION.$CMAKE_BUILD.tar.gz
rm cmake-$CMAKE_VERSION.$CMAKE_BUILD.tar.gz
cd cmake-$CMAKE_VERSION.$CMAKE_BUILD
./bootstrap
make -j4
sudo make install
```

### Ubuntu - Install CMake through package manager (easier, but may be a bit outdated)
```
sudo apt-get install cmake
```

## How to use
In order to make an out-of-place build with a given generator, you can follow this procedure:
First of all, clone the repo to a desired location:
```
mkdir -p ~/workspace
cd ~/workspace
git clone git@github.com:azarrias/cmake-sdl2-template.git
```
Then create a directory for the out-of-source build and execute cmake as needed.
In this example, Eclipse will be the IDE of choice:
```
cd ~/workspace/cmake-sdl2-template
mkdir build && cd build
cmake ../src -G "Eclipse CDT4 - Unix Makefiles" -D CMAKE_BUILD_TYPE=Debug -D CMAKE_ECLIPSE_VERSION=4.7.2
```
To set up the project in Eclipse:
1. Start Eclipse (it does not matter where you set your workspace).
2. Uncheck the Project -> Build Automatically option.
3. Import... -> Existing Projects into Workspace.
4. Select the build path, i.e. ~/workspace/cmake-sdl2-template/build. Eclipse should be able to find the project.
5. Project -> Build Project
