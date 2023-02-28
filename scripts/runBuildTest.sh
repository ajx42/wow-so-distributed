#!/bin/bash

timestamp=$(date +%Y%m%d_%H%M%S)

results_dir=$(pwd)/testResults_build/${timestamp}
mkdir -p $results_dir


stdout_file=$results_dir/stdout.txt
stderr_file=$results_dir/stderr.txt

cd /tmp/wowfs

echo "Cloning leveldb..."
git clone https://github.com/google/leveldb.git
cd leveldb

echo "Init submodules..."
git submodule update --init
mkdir build

echo "Configure cmake..."
cd build
cmake ..

echo "Running build tests..."
for i in $(seq 1 32); do
    echo "NEXT_ENTRY ============ ${i}" >> $stdout_file 2>> $stderr_file
    make clean
    /usr/bin/time --verbose make -j $i >> $stdout_file 2>> $stderr_file
done
