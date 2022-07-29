script_path=$(dirname "$(readlink -f "$0")")"/"
cd $script_path
mkdir build
cd build
cmake ..
make
