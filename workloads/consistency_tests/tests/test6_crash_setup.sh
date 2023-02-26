#!/bin/bash

sudo ~/wow-so-distributed/scripts/cleanup.sh 

#Make local and remote mirror directories
mkdir /tmp/wowfs
mkdir /tmp/wowfs_local

chmod 777 /tmp/wowfs
chmod 777 /tmp/wowfs_local

#Write config file
cat << EOF > /tmp/wowfs_local/unreliablefs.conf
[errinj_wow_crash]
op_regexp = flush
path_regexp = test_consistency/case6
probability = 100
EOF

#Mount FUSE fs.
~/wow-so-distributed/build/wowFS/unreliablefs/wowfs /tmp/wowfs -basedir=/tmp/wowfs_local -seed=1618680646 -server_address=c220g1-030827.wisc.cloudlab.us:50051 -d

