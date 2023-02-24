#! /usr/bin/env python3

import logging
import os
import utils
import sys
import test2_info as INFO
import time
'''
This is ClientA2.
'''

signal = INFO.a2_signal

def run_test():
    logging.info(f'START fname:{INFO.FNAME}')

    # wait for signal
    while True:
        if os.path.exists(signal):
            break
        time.sleep(1)

    # open, read (should be all 0), close
    # check in FNAME exists
    if not os.path.exists(INFO.FNAME):
        logging.info(f'{INFO.FNAME} not exist')
        sys.exit(1)
    fd = os.open(INFO.FNAME, os.O_RDONLY)
    read_len = 32768
    read_str = os.read(fd, read_len).decode('utf-8')
    utils.check_read_content(read_str, '0' * read_len)
    os.close(fd) # should not flush, as there is no writej

    # wait for next signal
    os.remove(signal)
    print("removed signal")
    while True:
        if os.path.exists(signal):
            break
        else:
            logging.info("waiting for signal")
            time.sleep(1)

    # open, read (should be all 1), close
    if not os.path.exists(INFO.FNAME):
        logging.info('FNAME not exist')
        sys.exit(1)
    fd = os.open(INFO.FNAME, os.O_RDONLY)
    read_len = 32768
    read_str = os.read(fd, read_len).decode('utf-8')
    utils.check_read_content(read_str, '1' * read_len)
    os.close(fd) # should not flush, as there is no writej

    # remove signal file
    os.remove(signal)
    logging.info('OK')


if __name__ == '__main__':
    utils.setup_logging(INFO.TEST_CASE_NO, 'A2')
    run_test()
