# SDB Debugger

Note: This project is linux only

- `cd build` => run all of these from inside sdb/build. Make need to `mkdir build` if one doesn't already exist from inside `/sdb`
- `cmake ..` => Runs vcpgkg, may need to run `cmake .. -DCMAKE_TOOLCHAIN_FILE=/full/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake --fresh`
- `cmake --build .`     => Builds executables when inside the /build dir
- `cmake --build build` => Builds executables when inside the project root dir
- `./tools/sdb` to run the exe (yes, from inside `./build`)
- `./tests` to run the test suite from inside `./build/tests`
- From the root dir, `cmake --build build && cd build/test && ./tests && cd ../..` to more easily run everything from root

## Usage
- TODO: Fill out once it is ready
- When manually testing with `hello_sdb`, the entry address useful for testing a breakpoint is `0x555555555147`

- `objdump -d .../hello_sdb` to get main and the syscall
- `cat /proc/<pid>/maps` to get memory space.
- <main syscall address> + memory space start - 0x1000 (size of space) to get the entry breakpoint
