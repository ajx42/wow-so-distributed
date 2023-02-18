#!/bin/bash


#Make local and remote mirror directories
mkdir /tmp/wowfs
mkdir /tmp/wowfs_local
mkdir /tmp/wowfs_remote
mkdir /tmp/wowfs_remote/subdir

chmod 777 /tmp/wowfs
chmod 777 /tmp/wowfs_local
chmod 777 /tmp/wowfs_remote
chmod 777 /tmp/wowfs_remote/subdir

#Write config file
#cat << EOF > /tmp/wowfs_local/unreliablefs.conf
#[errinj_noop]
#op_regexp = .*
#path_regexp = .*
#probability = 0
#EOF

cat << EOF > /tmp/wowfs_remote/sample.file
this is a sample
file
EOF

cat << EOF > /tmp/wowfs_remote/subdir/other.file
this is a 
second sample
file
EOF

#Mount FUSE fs.
./build/wowFS/unreliablefs/wowfs /tmp/wowfs -basedir=/tmp/wowfs_local -seed=1618680646 -d

#Start up file server
# ./build/wowRPC/server
