C++ = g++

ifndef os
   os = LINUX
endif

ifndef arch
   arch = AMD64
endif

CCFLAGS = -Wall -w -D$(os) -I../src -finline-functions -O3 -std=c++17 -I../../Simple-Web-Server -I/usr/include/node/

LDFLAGS = -L../src -ludt -lstdc++ -lssl -lcrypto -lpthread -lm 

DIR = $(shell pwd)

APP = recvfile

all: $(APP)

%.o: %.cpp
	$(C++) $(CCFLAGS) $< -c

recvfile: recvfile.o
	$(C++) $^ -o $@ $(LDFLAGS)

clean:
	rm -f *.o $(APP)

install:
	export PATH=$(DIR):$$PATH
