C++ = g++

ifndef os
   os = LINUX
endif

ifndef arch
   arch = AMD64
endif

CCFLAGS = -Wall -D$(os) -I../src -finline-functions -O3

LDFLAGS = -L../src -ludt -lstdc++ -lpthread -lm -lssl -lcrypto

DIR = $(shell pwd)

APP = appserver appclient sendfile recvfile

all: $(APP)

%.o: %.cpp
	$(C++) $(CCFLAGS) $< -c

appserver: appserver.o
	$(C++) $^ -o $@ $(LDFLAGS)
appclient: appclient.o
	$(C++) $^ -o $@ $(LDFLAGS)
sendfile: sendfile.o
	$(C++) $^ -o $@ $(LDFLAGS)
recvfile: recvfile.o
	$(C++) $^ -o $@ $(LDFLAGS)

clean:
	rm -f *.o $(APP)

install:
	export PATH=$(DIR):$$PATH
