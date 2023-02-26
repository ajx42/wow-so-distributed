# Test 3
## How To Run
* Modify host addresses `test3_info.py`.
* Make sure CLIENT_A1 can ssh into CLIENT_A2 and CLIENT_B.
* Make sure all hosts have the files in this directory.
* Run wowRPC server on the server.
* Run wowFS on all clients.
* Run `test3_clientA1.py`
* Check `/tmp/Client${A1, A2, B}_test3` on corresponding hosts for result.

## Test Timeline
| A1       | A2      | B        | Expected Behavior |
|----------|---------|----------|-------------------|
| open()   |         |          |                   |
| write(0) |         |          |                   |
| close()  |         |          |                   |
| open()   |         |          |                   |
|          |         | open()   |                   |
|          |         | read()   | Sees 0            |
|          |         | write(1) |                   |
|          |         | close()  |                   |
|          | open()  |          |                   |
|          | read()  |          | Sees 1            |
|          | close() |          |                   |
| read()   |         |          | Sees 1            |
| close()  |         |          |                   |
