#!/bin/bash

set -e
set -o pipefail

script_path=$(dirname "$(readlink -f "$0")")""

mkcd() {
    mkdir -p $1
    cd $1
}

cmakebuild() {
    mkcd "$script_path/client/build"
    cmake ..
}

build() {
    mkcd "$script_path/client/build"
    make
}

agentpath() {
    if [ $1 == "main" ]; then
        echo "$script_path/client/build/main"
    else
        echo "$script_path/release/$1"
    fi
}

mappath() {
    echo "$script_path/map/$1"
}

run() {
    cd server/
    java -jar hideandseek-*.jar \
        --first-team="$(agentpath $1)" \
        --second-team="$(agentpath $2)" \
        "$(mappath $3).yml" \
        "$(mappath $3).json" \
        | tee logs/stdout.txt
}

buildrun() {
    ( build )
    ( run $@ )
}

publish() {
    cp "$(agentpath main)" "$(agentpath $1)"
}

$@