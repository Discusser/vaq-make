# vaq-make

vaq-make is a Makefile generator designed to work with C/C++ programs.

vaq-make uses a special language called VMake.

I made this project to learn a bit more about domain-specific languages, and because I thought it would be fun to implement one. In order to demonstrate that this program works, my end goal is to be able to rewrite this project's `CMakeLists.txt` file using VMake.

## Usage

This projects provides a `vaq-make` executable. One command line argument can be passed: the name of the file to be processed. This is expected to be a VMake file (optionally with the `.vmake` extension, even though that doesn't matter). The grammar for the VMake language can be found in the documentation.

## Building

To build from source, run the following commands:

```shell
git clone https://github.com/Discusser/vaq-make.git
cd vaq-make
mkdir build
cd build
cmake -B . -S ..
make
```

If you want to build in debug mode, you can pass `-DCMAKE_BUILD_TYPE=Debug` to `cmake`.

## Documentation

Further documentation can be found in [doc/](doc/)
