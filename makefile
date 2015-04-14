COMPILER_SOURCES=$(wildcard src/compiler/*.c)
INTERPRETER_SOURCES=$(wildcard src/interpreter/*.c)
STD_SOURCES=$(wildcard src/stdlib/*.c)

COMP_OBJ_FILES=$(addprefix obj/compiler/,$(notdir $(COMPILER_SOURCES:.c=.o)))
INT_OBJ_FILES=$(addprefix obj/interpreter/,$(notdir $(INTERPRETER_SOURCES:.c=.o)))
STD_OBJ_FILES=$(addprefix obj/stdlib/,$(notdir $(STD_SOURCES:.c=.o)))

CFLAGS=-g -D_GNU_SOURCE -DUSE_COLOR -gdwarf-3 -Isrc/interpreter -Isrc/compiler -Isrc/grammar -Isrc/stdlib -rdynamic -std=c99 -fdiagnostics-color=auto -Wall
#CFLAGS=-Isrc/interpreter -Isrc/compiler -Isrc/grammar -Isrc/stdlib -rdynamic -O3 -fdiagnostics-color=auto -std=c99 -D_GNU_SOURCE -Wall
LDFLAGS=-lm -lreadline -ldl
COLOR=-fdiagnostics-color=always
CC=gcc
MKDIR=mkdir -p

all: lanky

guts: src/grammar/lanky.l src/grammar/lanky.y
	bison -d -o src/grammar/parser.c src/grammar/lanky.y -v
	lex -o src/grammar/tokens.c src/grammar/lanky.l

glory: $(COMP_OBJ_FILES) $(INT_OBJ_FILES) $(STD_OBJ_FILES) src/main.c src/grammar/parser.c src/grammar/tokens.c
	$(CC) $(CFLAGS) -o lanky $^ $(LDFLAGS)     

bottled: $(COMP_OBJ_FILES) $(INT_OBJ_FILES) $(STD_OBJ_FILES) src/bottled_main.c src/grammar/parser.c src/grammar/tokens.c lky_bottled.c
	$(CC) $(CFLAGS) -o bottled $^ $(LDFLAGS)     

clean:
	rm -f lanky machine test
	rm -rf obj
	rm -f src/grammar/parser.* src/grammar/tokens.c
	rm -f extensions/*.o
	rm -f extensions/*.so

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
