#! /usr/bin/env python3

import os
import sys
import pathlib
sys.path.append(str(pathlib.Path(__file__).parent.parent.resolve()))
import utils
import time
from pathlib import Path
import logging
import test6_info as INFO
'''
This is ClientA.
'''
logging.getLogger("paramiko").setLevel(logging.WARNING)

def run_test():
    b_ssh_client = utils.setup_ssh(INFO.CLIENT_B)

    # clean up
    if os.path.exists(INFO.TEST_DATA_DIR):
        os.system(f"rm -rf {INFO.TEST_DATA_DIR}")
    
    # create test data dir and test file
    os.mkdir(INFO.TEST_DATA_DIR)
    Path(INFO.FNAME).touch()

    # write
    fd = os.open(INFO.FNAME, os.O_WRONLY)
    os.write(fd, str.encode("0"*32768, 'utf-8'))
    os.close(fd) 

    # signal client B
    b_ssh_client.exec_command(f"rm {INFO.b_signal}")
    b_ssh_client.exec_command(
        f"python {utils.get_script_path(INFO.TEST_SCRIPT_DIR, 'B', INFO.TEST_CASE_NO)}"
        )
    b_ssh_client.exec_command(f"touch {INFO.b_signal}")

    # wait until client_B finish
    while True:
        if utils.check_file_exists_on_client(b_ssh_client, INFO.b_signal):
            print("waiting...")
            time.sleep(1)
        else:
            break
    print('Client b finished')

    # client b is now dead
    # should read 0
    fd = os.open(INFO.FNAME, os.O_RDONLY)
    read_str = os.read(fd, 32768).decode('utf-8')
    utils.check_read_content(read_str, "0"*32768)
    os.close(fd)

    # write something
    fd = os.open(INFO.FNAME, os.O_WRONLY)
    os.write(fd, str.encode("2"*32768, 'utf-8'))
    os.close(fd) 

    # call client b back up
    b_ssh_client.exec_command(f"touch {INFO.b_signal}")

    while True:
        if utils.check_file_exists_on_client(b_ssh_client, INFO.b_signal):
            print("waiting...")
            time.sleep(1)
        else:
            break
    print('Client b finished')

    logging.info("OK")

if __name__ == '__main__':
    utils.setup_logging(INFO.TEST_CASE_NO, 'A')
    run_test()
