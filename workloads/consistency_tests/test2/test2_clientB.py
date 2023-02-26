#! /usr/bin/env python3

import logging
import os
import sys
import pathlib
sys.path.append(str(pathlib.Path(__file__).parent.parent.resolve()))
import utils
import test2_info as INFO
import time
'''
This is ClientB.
'''

signal = INFO.b_signal

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
        logging.info('FNAME not exist')
        sys.exit(1)
    fd = os.open(INFO.FNAME, os.O_RDONLY)
    read_len = 32768
    read_str = os.read(fd, read_len).decode('utf-8')
    utils.check_read_content(read_str, '0' * read_len)
    os.close(fd) # should not flush, as there is no writej

    # remove signal file
    os.remove(signal)
    logging.info('OK')


if __name__ == '__main__':
    utils.setup_logging(INFO.TEST_CASE_NO, 'B')
    run_test()
