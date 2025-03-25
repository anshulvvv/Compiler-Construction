CC = gcc


CFLAGS = -g


TARGET = stage1exe


OBJS = lexer.o parser.o
# add parser.o later

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) driver.c $(OBJS) -o $(TARGET)

lexer.o: lexerDef.h lexer.h lexer.c
	$(CC) $(CFLAGS) -c lexer.c

parser.o: parserDef.h parser.h parser.c
	$(CC) $(CFLAGS) -c parser.c

# driver.o: driver.c
# 	$(CC) $(CFLAGS) -c driver.c

clean:
	rm -f *.o
	rm -f stage1exe

clearCache:
	rm -f *Cache
	rm -f *.o
	rm -f stage1exe

run:
	make clean
	make
#	gdb --args ./stage1exe "t2.txt" "parse_out.txt"
#	valgrind --track-origins=yes ./stage1exe "t2.txt" "parse_out.txt"
	./stage1exe "t1.txt" "parse_out.txt"