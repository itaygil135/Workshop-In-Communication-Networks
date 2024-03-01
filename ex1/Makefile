CC=g++
SRC=server.cpp client.cpp benchmark.h
BJS=$(SRC:.cpp=.o)
TARGETS=server client

TAR=tar
TARFLAGS=-cvf
TARNAME=ex1.tar
TARSRCS=$(SRC) Makefile README

all:$(TARGETS)

server: server.cpp
	$(CC) $^ -o server

client: client.cpp
	$(CC) $^ -o client

clean:
	$(RM) $(TARGETS) *~ *core
tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)
