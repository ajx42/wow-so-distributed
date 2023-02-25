#! /usr/bin/env python3

import logging
import os
import utils
import sys
import test5_info as INFO
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

    if not os.path.exists(INFO.FNAME):
        logging.info('FNAME not exist')
        sys.exit(1)
    
    # should have 0 size
    stat = os.stat(INFO.FNAME)
    if stat.st_size != 0:
        logging.info(f"File not empty: size {stat.st_size}")
        sys.exit(1)

    # remove signal file
    os.remove(signal)
    logging.info('OK')


if __name__ == '__main__':
    utils.setup_logging(INFO.TEST_CASE_NO, 'B')
    run_test()
