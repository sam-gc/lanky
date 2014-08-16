Lanky
=====

A new interpreted language implemented in C.

## Background
The interpreter uses Flex and Bison to do tokenization and parsing. The grammar is hopefully relatively simple. I borrowed some of the Bison and Flex code from [this tutorial](http://gnuu.org/2009/09/18/writing-your-own-toy-compiler/4/); that project uses C++ with LLVM. My goal is to learn about interpreters and create a marginally useful language with which to play around.

## Compilation
There are two main parts: the `guts` (Flex and Bison) and the `glory` (the AST builder and interpreter). Each can be built from the makefile, and both can be built with `all`. Finally, a `clean` option is included to remove the auto-generated files from Bison and Flex (these should not be in the repo itself) and the object file.

## Usage (subject to extreme change)
Right now, when you startd the program you can enter code. As soon as it hits the end of the line (by typing Control^D, for example) the tree is compiled to Lanky bytecode. In the earliest of early alpha stages, Lanky would walk the tree and interpret what it encountered. After comparing a Lanky loop that counted from 0 to 1,000,000 (printing each value along the way) to a similar loop in Python, I was horrified by how much slower Lanky was performing. Thus I decided to build a bytecode interpreter that emulates a stack machine (much like the CPython and JVM implementations). The VM has only recently (as of today, July 25) been able to perform the same operations as the original tree walking interpreter. Implementation details are in the following section.

## Bytecode Interpreter (Virtual Machine)
As of the most recent version, the VM has a dozen or so opcodes; this number will surely increase as I implement basic language features. Loops and if/elif/else constructs are fully implemented. Right now I am trying to implement functions, but this will be changing hugely as I introduce objects and closures. Each of the builtin Lanky types (strings, doubles, integers, and functions) can all be serialized to a binary representation and reloaded. Right now, when you run the `lanky` program, you are prompted to enter code (or you can supply `lanky` with a file) and `lanky` compiles the code to bytecode and sends it to the VM. Before it is interpreted, however, it is saved to an example binary file (aptly named `test`) which contains everything needed for the VM to run. The structure of that file is also subject to extreme change. If you would like to interpret that binary file directly, use the makefile to build `machine`. This program takes a filename as its first argument and interprets the bytecode in that file. If you use the `-s` switch, it will instead print a human-readable list of opcodes. Keep in mind that everything is still very _very_ broken.
