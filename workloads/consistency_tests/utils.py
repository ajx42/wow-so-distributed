import os
import logging
import getpass
import paramiko
import sys

def setup_logging(case_no: int, client_id: str):
    fname = f'/tmp/Client{client_id.upper()}_test{case_no}.log'
    logging.basicConfig(filename=fname, 
                        format='%(asctime)s %(message)s', 
                        level=logging.DEBUG)

def setup_ssh(host: str):
    username = getpass.getuser()
    home_dir = os.path.expanduser('~')
    sshkey_fname = f'{home_dir}/.ssh/id_rsa'

    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    print(f'Connect {username}@{host}')
    # test ssh connection
    client.connect(hostname=host, username=username, key_filename=sshkey_fname)
    # stdin, stdout, stderr =client.exec_command("ls -al")
    # print(stdout.readlines())
    # print(stderr.readlines())
    # client.close()
    return client

def check_file_exists_on_client(client, fname):
    sftp = client.open_sftp()
    try:
        sftp.stat(fname)
        return True
    except IOError:
        return False

def check_read_content(read_str, expected_str):
    if len(read_str) != len(expected_str):
        logging.info("read_len:%d, expected_len:%d", len(read_str), len(expected_str))
        sys.exit(1)
    for idx, c in enumerate(read_str):
        if c != expected_str[idx]:
            logging.info("idx:%d, read char:%c, expected char:%c", idx, c, expected_str[idx])
            sys.exit(1)

    logging.info("Finish Read")

def get_script_path(dir, client_id, test_case):
    return f'{dir}/test{test_case}_client{client_id.upper()}.py'