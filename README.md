JavaScript Backend for LLVM
===========================

Prototype JS backend for LLVM.

Build Instructions
------------------

    ./setup.sh
    cd build
    make llc

This builds `bin/llc`.

Usage
-----

We want to have PNaCl-ized LLVM IR as input, as that is easier to compile.

[Instructions to install PNaCl.](https://developers.google.com/native-client/sdk/download)
`llvm-js-2` was tested against `pepper_29`.

Assuming the `pwd` is `build`, do the following:

    pnacl-clang -c hello_world.c
    ../pnacl-wrapper hello_world.o

Now to compile to JavaScript:

    bin/llc -march=js -filetype=asm hello_world.o -o hello_world.js
