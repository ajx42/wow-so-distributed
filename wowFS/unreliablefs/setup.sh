#!/bin/bash


#Make local and remote mirror directories
mkdir /tmp/wowfs
mkdir /tmp/wowfs_local
mkdir /tmp/wowfs_remote
mkdir /tmp/wowfs_remote/subdir

#Mount FUSE fs.
./build/unreliablefs/unreliablefs /tmp/wowfs -basedir=/tmp/wowfs_local -seed=1618680646

#Write config file
cat << EOF > /tmp/wowfs/unreliablefs.conf
[errinj_noop]
op_regexp = .*
path_regexp = .*
probability = 0
EOF

cat << EOF > /tmp/wowfs_remote/sample.file
this is a sample
file
EOF

cat << EOF > /tmp/wowfs_remote/subdir/other.file
this is a 
second sample
file
EOF

#Unmount file system
#umount /tmp/fs
