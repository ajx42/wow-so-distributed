#! /usr/bin/env python3

import logging
import os
import utils
import sys
import test4_info as INFO
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
    
    # delete file
    os.remove(INFO.FNAME)

    # remove signal file
    os.remove(signal)
    logging.info('OK')


if __name__ == '__main__':
    utils.setup_logging(INFO.TEST_CASE_NO, 'B')
    run_test()
