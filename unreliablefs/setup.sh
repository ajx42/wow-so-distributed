#!/bin/bash


#Make local and remote mirror directories
mkdir /tmp/fs
mkdir /tmp/unreliable

#Mount FUSE fs.
./build/unreliablefs/unreliablefs /tmp/fs -basedir=/tmp/unreliable -seed=1618680646

#Write config file
cat << EOF > /tmp/fs/unreliablefs.conf
[errinj_noop]
op_regexp = .*
path_regexp = .*
probability = 0
EOF

#Unmount file system
#umount /tmp/fs
