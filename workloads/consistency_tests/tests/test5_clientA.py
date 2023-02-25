#! /usr/bin/env python3

import os
import utils
import sys
import time
from pathlib import Path
import logging
import test5_info as INFO
'''
This is ClientA.
'''
logging.getLogger("paramiko").setLevel(logging.WARNING)

def run_test():
    server_ssh_client = utils.setup_ssh(INFO.SERVER)
    b_ssh_client = utils.setup_ssh(INFO.CLIENT_B)

    # clean up
    if os.path.exists(INFO.TEST_DATA_DIR):
        os.system(f"rm -rf {INFO.TEST_DATA_DIR}")
    
    # create test data dir and test file
    os.mkdir(INFO.TEST_DATA_DIR)
    Path(INFO.FNAME).touch()

    # write and crash
    fd = os.open(INFO.FNAME, os.O_WRONLY)
    os.write(fd, str.encode("0"*32768, 'utf-8'))
    try:
        os.close(fd) # crash here
    except:
        print("Server crash occurred!!!")
        pass

    # wait until server is back up
    server_ssh_client.exec_command(f"~/wow-so-distributed/build/wowRPC/server")
    time.sleep(10) # have to sleep for a while for some reason
    
    # read, should see local cache
    fd = os.open(INFO.FNAME, os.O_RDONLY)
    read_str = os.read(fd, 32768).decode('utf-8')
    utils.check_read_content(read_str, "0"*32768)

    # shut down server
    server_ssh_client.exec_command(f"kill -9 $(pgrep -f wowRPC/server)")

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
    
    logging.info("OK")

if __name__ == '__main__':
    utils.setup_logging(INFO.TEST_CASE_NO, 'A')
    run_test()
