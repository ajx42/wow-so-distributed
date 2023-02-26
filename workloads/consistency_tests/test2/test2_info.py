import os
CLIENT_A1="c220g1-030822.wisc.cloudlab.us"
CLIENT_A2="c220g1-030822.wisc.cloudlab.us"
CLIENT_B="c220g1-030825.wisc.cloudlab.us"
MOUNT_POINT="/tmp/wowfs"

TEST_DATA_DIR = MOUNT_POINT + '/test_consistency'
TEST_SCRIPT_DIR = "~/wow-so-distributed/workloads/consistency_tests/test2"
TEST_CASE_NO = 2
FNAME = f'{TEST_DATA_DIR}/case{TEST_CASE_NO}'

a2_signal = "/tmp/WAKEUP_A2"
b_signal = "/tmp/WAKEUP_B"
