COMPILER_SOURCES=$(wildcard src/compiler/*.c)
INTERPRETER_SOURCES=$(wildcard src/interpreter/*.c)
STD_SOURCES=$(wildcard src/stdlib/*.c)

COMP_OBJ_FILES=$(addprefix obj/compiler/,$(notdir $(COMPILER_SOURCES:.c=.o)))
INT_OBJ_FILES=$(addprefix obj/interpreter/,$(notdir $(INTERPRETER_SOURCES:.c=.o)))
STD_OBJ_FILES=$(addprefix obj/stdlib/,$(notdir $(STD_SOURCES:.c=.o)))

CFLAGS=-g -Isrc/interpreter -Isrc/compiler -Isrc/grammar -Isrc/stdlib
LDFLAGS=-lm
COLOR=-fdiagnostics-color=always 
CC=gcc
MKDIR=mkdir -p

all: lanky machine

guts: src/grammar/lanky.l src/grammar/lanky.y
	bison -d -o src/grammar/parser.c src/grammar/lanky.y -v
	lex -o src/grammar/tokens.c src/grammar/lanky.l

glory: $(COMP_OBJ_FILES) $(INT_OBJ_FILES) $(STD_OBJ_FILES) src/main.c src/grammar/parser.c src/grammar/tokens.c
	$(CC) $(CFLAGS) -o lanky $^ $(LDFLAGS)     

clean:
	rm -f lanky machine test
	rm -rf obj
	rm -f src/grammar/parser.* src/grammar/tokens.c

machine: $(INT_OBJ_FILES) $(STD_OBJ_FILES) src/bin_main.c
	$(CC) $(CFLAGS) -o machine $^ $(LDFLAGS)

obj/compiler/%.o: src/compiler/%.c
	$(MKDIR) obj/compiler/
	$(CC) $(CFLAGS) -c -o $@ $<

obj/interpreter/%.o: src/interpreter/%.c 
	$(MKDIR) obj/interpreter/
	$(CC) $(CFLAGS) -c -o $@ $<

obj/stdlib/%.o: src/stdlib/%.c
	$(MKDIR) obj/stdlib/
	$(CC) $(CFLAGS) -c -o $@ $<

lanky: guts glory
