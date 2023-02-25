#! /usr/bin/env python3

import logging
import os
import utils
import sys
import test3_info as INFO
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

    # open, read (should be all 0), write, close
    # check in FNAME exists
    if not os.path.exists(INFO.FNAME):
        logging.info('FNAME not exist')
        sys.exit(1)
    fd = os.open(INFO.FNAME, os.O_RDWR)
    read_len = 32768
    read_str = os.read(fd, read_len).decode('utf-8')
    utils.check_read_content(read_str, '0' * read_len)

    os.lseek(fd, 0, 0)
    os.write(fd, str.encode("1"*32768, 'utf-8'))
    os.close(fd)

    # remove signal file
    os.remove(signal)
    logging.info('OK')


if __name__ == '__main__':
    utils.setup_logging(INFO.TEST_CASE_NO, 'B')
    run_test()
