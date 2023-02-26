# Test 2
## How To Run
* Modify host addresses `test2_info.py`.
* Make sure CLIENT_A1 can ssh into CLIENT_A2 and CLIENT_B.
* Make sure all hosts have the files in this directory.
* Run wowRPC server on the server.
* Run wowFS on all clients.
* Run `test2_clientA1.py`
* Check `/tmp/Client${A1, A2, B}_test2` on corresponding hosts for result.

## Test Timeline
| A1       | A2      | B       | Expected Behavior |
|----------|---------|---------|-------------------|
| open()   |         |         |                   |
| write(0) |         |         |                   |
| close()  |         |         |                   |
|          | open()  |         |                   |
|          | read()  |         | Sees 0            |
|          | close() |         |                   |
| open()   |         |         |                   |
| write(1) |         |         |                   |
|          | open()  |         |                   |
|          | read()  |         | Sees 1            |
|          | close() |         |                   |
|          |         | open()  |                   |
|          |         | read()  | Sees 0            |
|          |         | close() |                   |
| close()  |         |         |                   |