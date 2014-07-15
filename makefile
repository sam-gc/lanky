SOURCES=main.c parser.c tokens.c tools.c ast.c ast_binary_ops.c ast_interpreter.c
CFLAGS=-lm

guts: lanky.l lanky.y
	bison -d -o parser.c lanky.y
	lex -o tokens.c lanky.l

glory: $(SOURCES)
	gcc -o lanky $(SOURCES) $(CFLAGS) -g

all: guts glory

clean:
	rm lanky parser.c parser.h tokens.c