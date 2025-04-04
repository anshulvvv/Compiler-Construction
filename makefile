CC = gcc


CFLAGS = -g


TARGET = stage1exe


run:
#	gdb --args ./stage1exe "t2.txt" "parse_out.txt"
#	valgrind --track-origins=yes ./stage1exe "t2.txt" "parse_out.txt"
	./stage1exe "t1.txt" "parse_out.txt"