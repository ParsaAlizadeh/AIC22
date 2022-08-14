#!/bin/bash

set -e
set -o pipefail

script_path=$(dirname "$(readlink -f "$0")")
repo_url="https://raw.githubusercontent.com/SharifAIChallenge/AIC22-Game/main"

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
    echo "$script_path/map/map$1"
}

run() {
    cd server/
    rm -f logs/details.log
    java -jar hideandseek-*.jar \
        --first-team="$(agentpath $1)" \
        --second-team="$(agentpath $2)" \
        "$(mappath $3)/map.yml" \
        "$(mappath $3)/map.json" \
        | tee logs/stdout.txt
    mv logs/server.log "logs/server_$(date +"%Y_%m_%d_%H_%M_%S").log"
}

buildrun() {
    ( build )
    ( run $@ )
}

publish() {
    cp "$(agentpath main)" "$(agentpath $1)"
}

fetchmap() {
    mkdir -p "$(mappath $1)"
    curl "$repo_url/Maps/map$1/map.json" -o "$(mappath $1)/map.json"
    curl "$repo_url/Maps/map$1/map.yml" -o "$(mappath $1)/map.yml"
}

visual() {
    ./visual/AIC22-Graphic.x86_64
}

$@
