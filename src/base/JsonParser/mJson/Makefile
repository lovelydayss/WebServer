VPATH=src:test:include
OUTPATH=./out

nlohmann_hpp=nlohmann/json.hpp 

nlohmann=test_nlohmann.o
mJson=test_mJson.o mJson.o

# 使用 CFLAGS 控制 Makefile 自动推导标志
CPPFLAGS=-g -std=c++11
CC=g++

test_mJson : $(mJson)
	g++ $(CPPFLAGS) $(mJson) -o $(OUTPATH)/test_mJson
	mv ./*.o $(OUTPATH)

test_nlohmann:$(nlohmann)
	g++ $(CPPFLAGS) $(nlohmann) -o $(OUTPATH)/test_nlohmann
	mv ./*.o $(OUTPATH)

test_nlohmann.o:$(nlohmann_hpp)

test_mJson.o:mJson.h doctest.h
mJson.o:mJson.h

.PHONY : clean
clean :
	rm -rf out/*