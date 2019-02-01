# CVM
The first Virual Machine for CMS language.

[![Build Status](https://api.travis-ci.org/CVM-Projects/CVM.svg?branch=master)](https://travis-ci.org/CVM-Projects/CVM)

## Requirement

Languages:
- C++

Build Tools:
- C++ Compiler supports C++ 17.
- CMake
- GNU Make

Libraries:
- [libgmp](https://gmplib.org) (Linux), [libmpir](http://mpir.org) (Windows)
- [ChillMagic/PrivateLibrary](https://github.com/ChillMagic/PrivateLibrary)
- [CVM-Projects/LCMM](https://github.com/CVM-Projects/LCMM)

## Build

```
mkdir build
cd build
cmake ..
make
```

## Run

```
./cvm ../test.cms
```

## License

MIT License

## Open Source License

- [GMP](https://gmplib.org), [MPIR](http://mpir.org) ([LGPLv3](./licenses/COPYING.LESSERv3))
