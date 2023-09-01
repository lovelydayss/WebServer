VPATH=../base/ThreadPool/src:../base/ThreadPool/src/Utils/ThreadPool:./src
 
object=UThreadPool.o http_conn.o main.o

# 使用 CXXFLAGS 控制 Makefile 自动推导标志
CXXFLAGS=-g -std=c++11

all : $(object)
	g++ $(CXXFLAGS) $(object) -o out

main.o : http_conn.h ThreadPool.h
http_conn.o : http_conn.h
UThreadPool.o : UThreadPool.h

.PHONY : clean
clean :
	rm -rf ./*.o out
