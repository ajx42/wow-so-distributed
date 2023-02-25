#!/bin/sh

WorkloadsPath="./workloads/filebench_workloads/"
FileBenchPath="./workloads/filebench/build/bin/filebench"

WowFSFileBenchPath="/tmp/wowfs/filebench/"

#FileBench config file list
TestList=( 
filemicro_create.f 
filemicro_createfiles.f 
filemicro_createrand.f
filemicro_delete.f
filemicro_rread.f
filemicro_rwritedsync.f
filemicro_seqread.f
filemicro_seqwrite.f
filemicro_statfile.f
filemicro_writefsync.f
fileserver.f
mongo.f
varmail.f
webserver.f
)

#Used for coloring output
Green='\033[0;32m'
Red='\033[0;31m'
Color_Off='\033[0m'

#Takes a .f filename as an argument and executes it
function runTest {
    echo -n "${1}... "
    
    echo "NEXT_ENTRY ============" >> $2 
    echo $1 >> $2 

    echo "NEXT_ENTRY ============" >> $3 
    echo $1 >> $3 

    ${FileBenchPath} -f ${WorkloadsPath}${1} >> $2 2>> $3 

    if [ $? == 0 ]
    then
        echo -e "${Green}Pass${Color_Off}"
    else
        echo -e "${Red}Fail${Color_Off}"
    fi
}

echo "Running FileBench workloads..."

timestamp=$(date +%Y%m%d_%H%M%S)

results_dir=./testResults/${timestamp}
mkdir -p $results_dir

stdout_file=$results_dir/stdout.txt
stderr_file=$results_dir/stderr.txt

for Test in "${TestList[@]}" 
do
    rm -rf $WowFSFileBenchPath
    runTest $Test $stdout_file $stderr_file

    #rm -rf $WowFSFileBenchPath
    #runTest $Test $stdout_file $stderr_file

    #rm -rf $WowFSFileBenchPath
    #runTest $Test $stdout_file $stderr_file
done

python3 ./scripts/parseFileBench.py $stdout_file
