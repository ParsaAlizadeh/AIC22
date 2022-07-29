# Install GRPC:

- Install Prerequisites

```
sudo apt-get install build-essential autoconf libtool pkg-config
```

- If you don't have cmake install via:
```
sudo apt-get install cmake
```

- set install directory for grpc
```
INSTALL_DIR=$HOME
```

- clone grpc repository

```
cd $INSTALL_DIR
git clone --recurse-submodules -b v1.45.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc
```

- install protobuf

```
cd $INSTALL_DIR/grpc/third_party/protobuf
./autogen.sh
 ./configure
make
make check
sudo make install
sudo ldconfig # refresh shared library cache.
```

- install grpc

```
cd ../..
mkdir -p cmake/build
pushd cmake/build
cmake -DgRPC_INSTALL=ON \
  -DgRPC_BUILD_TESTS=OFF \
  -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
  ../..
make 
make install
popd
```

- test if you can compile an example

```
cd $INSTALL_DIR/grpc/examples/cpp/helloworld
mkdir -p cmake/build
cd cmake/build
cmake ../..
make
```

# Install yaml-cpp:
```
# first cd to the directory you want to install yaml-cpp (for example ~)
cd ~
git clone https://github.com/jbeder/yaml-cpp.git --branch yaml-cpp-0.6.0 && \
cd yaml-cpp && \
mkdir build && \
cd build && \
cmake .. && \
sudo make install
```

# Compile:

- in the project directory
```
./build.sh
```

# Run:

- execute "main" in the ./build folder (with proper command line arguments)

