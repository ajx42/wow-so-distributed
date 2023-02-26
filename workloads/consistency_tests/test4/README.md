# Test 4
## How To Run
* Modify host addresses `test4_info.py`.
* Make sure CLIENT_A can ssh into CLIENT_B.
* Make sure all hosts have the files in this directory.
* Run wowRPC server on the server.
* Run wowFS on all clients.
* Run `test4_clientA.py`
* Check `/tmp/Client${A1, B}_test4` on corresponding hosts for result.

## Test Timeline
| A        | B        | Expected Behavior    |
|----------|----------|----------------------|
| open()   |          |                      |
| write(0) |          |                      |
| close()  |          |                      |
|          | delete() |                      |
| open()   |          |                      |
| fstat()  |          | File should be empty |

## Note
* Currently WowFS does not pass this test case. CLIENT_A still sees the content it wrote in local cache.
