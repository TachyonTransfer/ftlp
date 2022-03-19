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

APP = tachyon

all: $(APP)

%.o: %.cpp
	$(C++) $(CCFLAGS) $< -c

tachyon:
	g++ -c -o tachyon.o tachyon.cpp -fpermissive -fPIC -I../src -I/usr/include/node/ -finline-functions -O0 -std=c++17  -L../src/libudt.a -lstdc++ -lpthread -lm
	ar rcs libtachyon.a tachyon.o ../src/*.o

clean:
	rm -f *.o $(APP)
	rm *.a

install:
	export PATH=$(DIR):$$PATH
