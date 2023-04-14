 
object=test.o leptjson.o

# 使用 CFLAGS 控制 Makefile 自动推导标志
CFLAGS=-g -std=c89
CC=gcc

all : $(object)
	gcc $(CFLAGS) $(object) -o test_out


test.o:leptjson.h
leptjson.o:leptjson.h


.PHONY : clean
clean :
	rm -rf ./*.o test_out