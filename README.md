### wow-so-distributed
CS 739 Projects: never centralized, always distributed

# wowFS

The `WOW FILE SYSTEM` is a remote file system with largely AFS like semantics built using `FUSE` and `GRPC`.

### Build instructions

The entire project software can be built as follows:

```shell
mkdir build
cd build
cmake ../ -DCMAKE_INSTALL_PREFIX=.
cmake --build . --parallel 16
cmake --install .
```

### Project Organisation
The project is organised as follows:
1. `wowFS`: `UnreliableFS` and `WowManager` to handle integration of other `wowFS` components and FUSE based `UnreliableFS`.
2. `wowRPC`: `GRPC` based software layer to handle client and file server coordination.
3. `wowUtils`: Other software components namely `WowCache` and `WowLogger`.
4. `wowAB`: The `wowConcert` distributed test coordination framework.
5. `workloads`: Filebench, consistency tests, and durability scenarios.
