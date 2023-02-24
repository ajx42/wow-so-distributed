#! /usr/bin/env python3

import os
import utils
import sys
import time
from pathlib import Path
import logging
import test2_info as INFO
'''
This is ClientA1.
'''
logging.getLogger("paramiko").setLevel(logging.WARNING)

def run_test():
    a2_ssh_client = utils.setup_ssh(INFO.CLIENT_A2)
    b_ssh_client = utils.setup_ssh(INFO.CLIENT_B)

    # clean up
    if os.path.exists(INFO.TEST_DATA_DIR):
        os.system(f"rm -rf {INFO.TEST_DATA_DIR}")
    
    # create test data dir and test file
    os.mkdir(INFO.TEST_DATA_DIR)
    Path(INFO.FNAME).touch()

    fd = os.open(INFO.FNAME, os.O_WRONLY)
    os.write(fd, str.encode("0"*32768, 'utf-8'))
    os.close(fd)

    # call A2
    print("Calling client A2")
    a2_ssh_client.exec_command(f"rm {INFO.a2_signal}")
    a2_ssh_client.exec_command(
        f"python {utils.get_script_path(INFO.TEST_SCRIPT_DIR, 'A2', INFO.TEST_CASE_NO)}"
        )
    a2_ssh_client.exec_command(f"touch {INFO.a2_signal}")


    # wait until client_A2 finish
    while True:
        if utils.check_file_exists_on_client(a2_ssh_client, INFO.a2_signal):
            print("waiting...")
            time.sleep(1)
        else:
            break
    print('Client a2 finished')

    # open and write
    fd = os.open(INFO.FNAME, os.O_WRONLY)
    os.write(fd, str.encode("1"*32768, 'utf-8'))

    # signal client_A2 again
    a2_ssh_client.exec_command(f"touch {INFO.a2_signal}")

    # wait until client_A2 finish
    while True:
        if utils.check_file_exists_on_client(a2_ssh_client, INFO.a2_signal):
            time.sleep(1)
        else:
            break

    # signal client_B
    b_ssh_client.exec_command(f"rm {INFO.b_signal}")
    b_ssh_client.exec_command(
        f"python {utils.get_script_path(INFO.TEST_SCRIPT_DIR, 'B', INFO.TEST_CASE_NO)}"
        )
    b_ssh_client.exec_command(f"touch {INFO.b_signal}")

    # wait until client_B finish
    while True:
        if utils.check_file_exists_on_client(b_ssh_client, INFO.b_signal):
            time.sleep(1)
        else:
            break
    print('Client b finished')

    
    # close file
    os.close(fd)

    # done
    logging.info("OK")

if __name__ == '__main__':
    utils.setup_logging(INFO.TEST_CASE_NO, 'A1')
    run_test()
