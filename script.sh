#!/bin/bash

set -e
set -o pipefail

script_path=$(dirname "$(readlink -f "$0")")
repo_url="https://raw.githubusercontent.com/SharifAIChallenge/AIC22-Game/main"

white='\e[0m'
red='\e[0;31m'
green='\e[0;32m'
blue='\e[0;34m'

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
    echo 'nothing here' > logs/last_game.txt
    java -jar hideandseek-*.jar \
        --first-team="$(agentpath $1)" \
        --second-team="$(agentpath $2)" \
        "$(mappath $3)/map.yml" \
        "$(mappath $3)/map.json" \
        | tee logs/stdout.txt
    post_game_check $@
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

package() {
    cd client/
    ls | grep -Ev 'build$' | xargs zip -r "$script_path/client.zip"
}

post_game_check() {
    if grep 'GameException' logs/stdout.txt &>/dev/null; then
        logbad 'server exception in stdout.txt (maybe timeout?)'
        grep --color 'GameException' logs/stdout.txt
    else
        loggood 'no server exception'
    fi
    sdate=$(stat -c '%Y' logs/last_game.txt)
    for f in $(ls logs/client/); do
        file="logs/client/$f"
        cdate=$(stat -c '%Y' $file)
        if [[ $cdate < $sdate ]]; then
            loginfo "old ${file}"
        elif grep 'balance mismatch!' $file &>/dev/null; then
            amounts=$(mktemp)
            grep 'balance mismatch!' $file | awk '{ print $4 - $3 }' | sort -n | cat > $amounts
            min_amount=$(head -n1 $amounts)
            max_amount=$(tail -n1 $amounts)
            rm $amounts
            logbad "balance mismatch agent=$(head -n1 $file) margin=($min_amount,$max_amount)"
        else
            loggood "balance matches agent=$(head -n1 $file)"
        fi
    done
    inspect logs/server.log $1 $2
}

inspect() {
    if grep 'FIRST_WINS' $1 &>/dev/null; then
        loginfo "first team wins ($2)"
    else
        loginfo "second team wins ($3)"
    fi
    turn=$(grep 'toTurnNumber' $1 | tail -n1 | sed -E 's/.*toTurnNumber":"([0-9]*).*/\1/g')
    loginfo "game ends in $turn turns"
}

logbad() {
    echo -e "$red [BAD] $@ $white"
}

loggood() {
    echo -e "$green [GOOD] $@ $white"
}

loginfo() {
    echo -e "$blue [INFO] $@ $white"
}

$@
