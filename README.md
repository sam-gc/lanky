Lanky
=====

# A new interpreted language implemented in C.


## Background
The interpreter uses Flex and Bison to do tokenization and parsing. The grammar is hopefully relatively similar. I borrowed some of the Bison and Flex code from [this tutorial](http://gnuu.org/2009/09/18/writing-your-own-toy-compiler/4/); that project uses C++ with LLVM. My goal is to learn about interpreters and create a marginally useful language with which to play around.

## Compilation
There are two main parts: the `guts` (Flex and Bison) and the `glory` (the AST builder and interpreter). Each can be built from the makefile, and both can be built with `all`. Finally, a `clean` option is included to remove the auto-generated files from Bison and Flex (these should not be in the repo itself) and the object file.

## Usage (subject to extreme change)
Right now, when you start the program you can enter code. As soon as it hits the end of the line (by typing Control^D, for example) it interprets the resulting abstract syntax tree. For now, each "statement" has its value printed, but this whole section is subject to big changes.
