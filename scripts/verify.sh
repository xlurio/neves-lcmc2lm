#! /bin/bash

set -xe

caustin --suffix '.c' --suffix '.h' --suffix '.cpp' --suffix '.hpp' --suffix '.inc' \
    --exclude 'build' --exclude 'assets' --exclude 'scripts'
cd build && make && cd ..
