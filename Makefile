

CPP=g++
CPPFLAGS= -g -Wall -DDEBUG -fPIC -I.

LIB_SRCS = $(wildcard lib/*.cpp)
LIB_OBJS = $(LIB_SRCS:.cpp=.o)

SRCS = $(wildcard ./*.cpp)
OBJS = $(SRCS:.cpp=.o)


all: ruralis2 libruralis2.so

libruralis2.so : $(LIB_OBJS)
	ar rcs $@ $^

#####################  実行ファイル生成
EXEC_OBJS = main.o test.o
ruralis2: $(EXEC_OBJS) libruralis2.so
	g++ -o $@ $(EXEC_OBJS) -L. -lruralis2 -lsqlite3 


#####################

%.o: %.cpp
	$(CPP) $(CPPFLAGS) -o $@ -c $<

clean:
	rm $(EXEC_OBJS)
	find . -name "*~" -delete
	rm $(LIB_OBJS)
#	rm ruralis libruralis.so

