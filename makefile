SOURCES=main.c parser.c tokens.c tools.c ast.c mempool.c trie.c hashmap.c ast_compiler.c lky_object.c arraylist.c lky_machine.c lkyobj_builtin.c bytecode_analyzer.c lky_gc.c
CFLAGS=-lm -g -pg
COLOR=-fdiagnostics-color=always 
CC=gcc

guts: lanky.l lanky.y
	bison -d -o parser.c lanky.y -v
	lex -o tokens.c lanky.l

glory: $(SOURCES)
	$(CC) -o lanky $(SOURCES) $(CFLAGS)

all: guts glory

clean:
	rm lanky bnm machine arrtests parser.c parser.h tokens.c

arraytests: arrtests.c arraylist.c
	gcc -o arrtests arrtests.c arraylist.c -g

bnm: binary_file_maker.c
	gcc -o bnm binary_file_maker.c lky_object.c lkyobj_builtin.c arraylist.c

machine: lky_machine.c lky_object.c lkyobj_builtin.c arraylist.c bin_main.c
	$(CC) -o machine lky_machine.c hashmap.c arraylist.c trie.c lky_object.c lkyobj_builtin.c bin_main.c $(CFLAGS)
