#! /usr/bin/env python3

import os
import utils
import sys
import time
from pathlib import Path
import logging
import test4_info as INFO
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

    fd = os.open(INFO.FNAME, os.O_WRONLY)
    os.write(fd, str.encode("W0M"*32768, 'utf-8'))
    os.close(fd)
    time.sleep(3)


    # signal client_B
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

    # open again
    fd = os.open(INFO.FNAME, os.O_RDONLY)
    # check if file is empty
    stat = os.fstat(fd)

    # check if the file is empty
    if stat.st_size != 0:
        logging.info(f"File not empty: size {stat.st_size}")
        sys.exit(1)

    # close file
    os.close(fd)

    # done
    logging.info("OK")

if __name__ == '__main__':
    utils.setup_logging(INFO.TEST_CASE_NO, 'A')
    run_test()
