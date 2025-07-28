# vaq-make

vaq-make is a Makefile generator designed to work with C/C++ programs.

vaq-make uses a special language called VMake.

I made this project to learn a bit more about domain-specific languages, and because I thought it would be fun to implement one. In order to demonstrate that this program works, my end goal is to be able to rewrite this project's `CMakeLists.txt` file using VMake.

## Usage

This project provides a `vaq-make` executable. The grammar for the VMake language can be found in the documentation. The `vaq-make` executable's usage is as follows:

```
Usage: vaq-make [vmake_file] [source_directory] [build_directory]
```

`vmake_file` is preferably a file with a `.vmake` extension.
`source_directory` is the directory where the source files are located, and `build_directory` is the directory where the Makefile should be generated.

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

### Bootstrapping

If you have faith in `vaq-make` and expect it to work, you can try building `vaq-make` with `vaq-make`. Since no releases are provided, you first have to build `vaq-make` using CMake (refer to the steps above for that). Once you have a `vaq-make` executable, you can run the following commands, assuming you've cloned the repository and are in the root directory:

```shell
mkdir build-vmake
./build/vaq-make VMake.vmake . build-vmake/
cd build-vmake
make
```

If no errors occurred, you should have your very own bootstrapped `vaq-make` executable! Use at your own discretion...

## Documentation

Further documentation can be found in [doc/](doc/)
