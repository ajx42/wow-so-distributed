#! /usr/bin/env python3

import logging
import os
import utils
import sys
import test6_info as INFO
import time
import subprocess

'''
This is ClientB.
'''

signal = INFO.b_signal

def run_test():
    logging.info(f'START fname:{INFO.FNAME}')

    # bring wowfs up
    subprocess.Popen(['bash', f"{INFO.TEST_SCRIPT_DIR}/test6_crash_setup.sh"])
    time.sleep(3)

    # wait for signal
    while True:
        if os.path.exists(signal):
            break
        time.sleep(1)
    
    if not os.path.exists(INFO.FNAME):
        logging.info('FNAME not exist')
        sys.exit(1)
    
    # let's try to open, write, and close (crash)
    fd = os.open(INFO.FNAME, os.O_WRONLY)
    os.write(fd, str.encode("1"*32768, 'utf-8'))
    try:
        os.close(fd) # crash!!!
    except:
        logging.info("finish writing and closing. wowfs is now down")
        pass

    # remove signal file
    os.remove(signal)

    # wait for signal
    while True:
        if os.path.exists(signal):
            break
        time.sleep(1)
    
    # bring wowfs up again
    subprocess.Popen(['bash', f"{INFO.TEST_SCRIPT_DIR}/test6_no_crash_setup.sh"])
    time.sleep(3)
    logging.info("wowfs back up again")
    
    # let's see if we can fetch the newest file
    fd = os.open(INFO.FNAME, os.O_RDONLY)
    read_str = os.read(fd, 32768).decode('utf-8')
    utils.check_read_content(read_str, "2"*32768)
    os.close(fd)

    os.remove(signal)

    logging.info('OK')


if __name__ == '__main__':
    utils.setup_logging(INFO.TEST_CASE_NO, 'B')
    run_test()
