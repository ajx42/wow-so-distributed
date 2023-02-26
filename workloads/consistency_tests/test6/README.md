# Test 6
## How To Run
* Modify host addresses `test6_info.py`.
* Make sure CLIENT_A can ssh into CLIENT_B.
* Make sure all hosts have the files in this directory.
* Run wowRPC server on the server.
* Run wowFS on **ONLY CLIENT_A**.
* Run `test6_clientA.py`
* Check `/tmp/Client${A, B}_test6` on corresponding hosts for result.

## Test Timeline
| A        | B                  | Expected Behavior |
|----------|--------------------|-------------------|
| open()   |                    |                   |
| write(0) |                    |                   |
| close()  |                    |                   |
|          | open()             |                   |
|          | write(1)           |                   |
|          | close()            | B crashes         |
| open()   |                    |                   |
| read()   |                    | Sees 0            |
| close()  |                    |                   |
| open()   |                    |                   |
| write(2) |                    |                   |
| close()  |                    |                   |
|          | Completes reboot   |                   |
|          | open()             |                   |
|          | read()             | Sees 2            |
|          | close()            |                   |
