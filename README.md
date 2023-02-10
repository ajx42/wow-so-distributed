# wow-so-distributed
CS 739 Projects: never centralized, always distributed

### Build instructions

```shell
mkdir build
cd build
cmake ../ -DCMAKE_INSTALL_PREFIX=.
cmake --build . --parallel 16
cmake --install .
```
After this the binaries will end up in: `build/bin/`

