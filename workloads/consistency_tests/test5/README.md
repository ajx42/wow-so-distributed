# Test 5
## How To Run
* Modify host addresses `test5_info.py`.
* Make sure CLIENT_A can ssh into CLIENT_B and SERVER.
* Make sure all hosts have the files in this directory.
* Run wowRPC server on the server with the flag `--withCrashOnWrite`.
* Run wowFS on all clients.
* Run `test5_clientA.py`
* Check `/tmp/Client${A, B}_test5` on corresponding hosts for result.

## Test Timeline
| A        | B      | Server                            | Expected Behavior    |
|----------|--------|-----------------------------------|----------------------|
| open()   |        |                                   |                      |
| write(0) |        |                                   |                      |
| close()  |        | Crashes before renaming temp file |                      |
|          |        | Completes reboot                  |                      |
| open()   |        |                                   |                      |
| read()   |        |                                   | Sees 0 (local cache) |
|          | stat() |                                   | Sees empty file      |
| close()  |        |                                   |                      |

## Note
* Ideally, CLIENT_A should see an empty file. But wowFS only guarantees that file is not corrupted on server.
