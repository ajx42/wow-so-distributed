#!/bin/sh

WorkloadsPath="./workloads/filebench_workloads/"
FileBenchPath="./workloads/filebench/build/bin/filebench"

#FileBench config file list
TestList=( filemicro_create.f )
#filemicro_createfiles.f 
#filemicro_createrand.f
#filemicro_delete.f
#filemicro_rread.f
#filemicro_rwritedsync.f
#filemicro_seqread.f
#filemicro_seqwrite.f
#filemicro_statfile.f
#filemicro_writefsync.f
#fileserver.f
#mongo.f
#set_dir.py
#varmail.f
#webserver.f
#)

#Used for coloring output
Green='\033[0;32m'
Red='\033[0;31m'
Color_Off='\033[0m'

#Takes a .f filename as an argument and executes it
function runTest {
    echo -n "${1}... "

    ${FileBenchPath} -f ${WorkloadsPath}${1}

    if [ $? == 0 ]
    then
        echo -e "${Green}Pass${Color_Off}"
    else
        echo -e "${Red}Fail${Color_Off}"
    fi
}

echo "Running FileBench workloads..."


for Test in "${TestList[@]}" 
do
    runTest $Test
    runTest $Test
    runTest $Test
done
