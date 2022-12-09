#
# Makefile for TINY
# Gnu C Version
# K. Louden 2/3/98
#

CC = gcc

CFLAGS =

OBJS = main.o util.o scan.o parse.o symtab.o analyze.o code.o cgen.o

tiny: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o tiny

SCAN_OBJS = main.o util.o scan.o
tiny_only_scan: $(SCAN_OBJS)
	$(CC) $(CFLAGS) $(SCAN_OBJS) -o tiny_only_scan

LEX_OBJS = main.o util.o lex.yy.o
tiny_scan_by_lex: $(LEX_OBJS)
	$(CC) $(CFLAGS) $(LEX_OBJS) -lfl -o tiny_scan_by_lex

PARSE_OBJS = main.o util.o scan.o parse.o
tiny_only_parse: $(PARSE_OBJS)
	$(CC) $(CFLAGS) $(PARSE_OBJS) -o tiny_only_parse 

lex.yy.o: lex.yy.c globals.h
	$(CC) $(CFLAGS) -c lex.yy.c

main.o: main.c globals.h util.h scan.h parse.h analyze.h cgen.h
	$(CC) $(CFLAGS) -c main.c 

util.o: util.c util.h globals.h
	$(CC) $(CFLAGS) -c util.c

scan.o: scan.c scan.h util.h globals.h
	$(CC) $(CFLAGS) -c scan.c

parse.o: parse.c parse.h scan.h globals.h util.h
	$(CC) $(CFLAGS) -c parse.c

symtab.o: symtab.c symtab.h
	$(CC) $(CFLAGS) -c symtab.c

analyze.o: analyze.c globals.h symtab.h analyze.h
	$(CC) $(CFLAGS) -c analyze.c

code.o: code.c code.h globals.h
	$(CC) $(CFLAGS) -c code.c

cgen.o: cgen.c globals.h symtab.h code.h cgen.h
	$(CC) $(CFLAGS) -c cgen.c

clean:
	-rm tiny
	-rm tm
	-rm $(OBJS)

tm: tm.c
	$(CC) $(CFLAGS) tm.c -o tm

all: tiny tm

