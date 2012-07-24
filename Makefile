WARNINGS=-Wall -W 
DEBUG?= -g 
OPTIMIZATION=-O3
SOURCES=$(PWD)/src/chatServer.cc
SOURCES+=$(PWD)/protobuf/zmgchatbuf.pb.cc
#SOURCES+=$(PWD)/../hiredispp/hiredispp.cpp
CLNT_SOURCES=$(PWD)/src/chatClient.cc
CLNT_SOURCES+=$(PWD)/protobuf/zmgchatbuf.pb.cc
CC=g++
LDFLAGS+=-lhiredis -lprotobuf -lzmq
RM=rm

CFLAGS += -I/usr/local/include
#CFLAGS += -I$(PWD)/../hiredispp

chatServer:
	$(CC) $(CFLAGS) $(DEBUG) $(WARNINGS) $(SOURCES) $(LDFLAGS) -o chatServer
chatClient:
	$(CC) $(DEBUG) $(WARNINGS) $(CLNT_SOURCES) $(LDFLAGS) -o chatClient

all:	chatServer chatClient
clean:	
	$(RM) chatServer* chatClient*
