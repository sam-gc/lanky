Lanky
=====

A new interpreted language implemented in C.

## Background
The interpreter uses Flex and Bison to do tokenization and parsing. The grammar is hopefully relatively simple. I borrowed some of the Bison and Flex code from [this tutorial](http://gnuu.org/2009/09/18/writing-your-own-toy-compiler/4/); that project uses C++ with LLVM. My goal is to learn about interpreters and create a marginally useful language with which to play around.

## Standard Library
There is a consistently growing standard library (largely made up of C native functions). I haven't had time to create a proper wiki page or website to document the standard library (and it is constantly changing) but if you type in the REPL "Meta.helpStdlib();" you will get a readout of a more-or-less up to date layout of the standard library.

## Compilation
There are two main parts: the `guts` (Flex and Bison) and the `glory` (the AST builder/compiler and bytecode interpreter). Each can be built from the makefile, and both can be built with `all` or `lanky`. Finally, a `clean` option is included to remove the auto-generated files from Bison and Flex (these should not be in the repo itself) and the object files.

## Usage (subject to extreme change)
Several months ago, Lanky got a full-blown REPL. Tab completion only works for files (default libreadline behavior) but I shall be updating that later. As soon as a line is entered, it is parsed into an abstract syntax tree. In the earliest of early alpha stages, Lanky would walk the tree and interpret what it encountered. After comparing a Lanky loop that counted from 0 to 1,000,000 (printing each value along the way) to a similar loop in Python, I was horrified by how much slower Lanky was performing. Thus I decided to build a bytecode interpreter that emulates a stack machine (much like the CPython and JVM implementations). Implementation details are in the following section.

## Bytecode Interpreter (Virtual Machine)
As of the most recent version, the VM has a couuple dozen or so opcodes. Loops and if/elif/else constructs are fully implemented. Each of the builtin Lanky types (strings, doubles, integers, and functions) can all be serialized to a binary representation and reloaded. Right now, when you run the `lanky` program, you are greeted by the REPL (or you can supply `lanky` with a file) and `lanky` compiles the text to bytecode and sends it to the VM. If you use the `-s` switch, it will instead print a human-readable list of opcodes. The interpreter has matured quite a lot in recent commits (let's call it a ~~second~~ ~~fourth~~ sixth grader as opposed to a kindergartner) and can run all of the examples in the `examples` folder without exception/memory errors.

## The state of the memory management
Previous iterations of the Lanky interpreter used reference counting as the primary means of memory management. This was working fine (albeit a touch messy) until I started implementing closures and objects. Due to the nature of closures, strong reference cycles were being created. The same was true for classes, as I dislike Python's use of an added parameter for methods when referencing `self`. As such I recently switched to using a garbage collector. It is highly rudimentary (stop-the-world naïve mark and sweep) but it actually performs close enough to the reference counting to be acceptable. I plan to do lots of work on the garbage collector and allocator later, but for now it works and it is modular enough to be easily replaced with a new system in the future. For now I am going to continue maturing Lanky as a language since the memory management is beginning to be *good enough*. I'm sure there are still tons of bugs to work out but for now I am happy with the state of things.
